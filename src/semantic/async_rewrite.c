#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "async_rewrite.h"
#include "../string_intern.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  ASTNode *context_decl;
  ASTNode *body_decl;
  ASTNode *entry_decl;
} AsyncGeneratedDecls;

static void async_rewrite_report(ErrorReporter *reporter, SourceLocation location,
                                 const char *format, ...) {
  if (!reporter || !format) {
    return;
  }

  va_list args;
  va_start(args, format);
  va_list args_copy;
  va_copy(args_copy, args);
  int size = vsnprintf(NULL, 0, format, args_copy);
  va_end(args_copy);
  if (size < 0) {
    va_end(args);
    return;
  }

  char *message = malloc((size_t)size + 1);
  if (!message) {
    va_end(args);
    return;
  }

  vsnprintf(message, (size_t)size + 1, format, args);
  va_end(args);
  error_reporter_add_error(reporter, ERROR_SEMANTIC, location, message);
  free(message);
}

static ASTNode *async_make_block(void) {
  return ast_create_program();
}

static int async_block_append(ASTNode *block, ASTNode *statement) {
  if (!block || !statement || block->type != AST_PROGRAM) {
    return 0;
  }

  Program *program = (Program *)block->data;
  if (!program) {
    return 0;
  }

  ASTNode **grown = realloc(program->declarations,
                            (program->declaration_count + 1) * sizeof(ASTNode *));
  if (!grown) {
    return 0;
  }

  program->declarations = grown;
  program->declarations[program->declaration_count++] = statement;
  ast_add_child(block, statement);
  return 1;
}

static ASTNode *async_make_return(ASTNode *value, SourceLocation location) {
  ASTNode *node = ast_create_node(AST_RETURN_STATEMENT, location);
  if (!node) {
    return NULL;
  }

  if (value) {
    ReturnStatement *ret = malloc(sizeof(ReturnStatement));
    if (!ret) {
      ast_destroy_node(node);
      return NULL;
    }
    ret->value = value;
    node->data = ret;
    ast_add_child(node, value);
  }

  return node;
}

static ASTNode *async_make_if(ASTNode *condition, ASTNode *then_branch,
                              ASTNode *else_branch,
                              SourceLocation location) {
  if (!condition || !then_branch) {
    return NULL;
  }

  ASTNode *node = ast_create_node(AST_IF_STATEMENT, location);
  if (!node) {
    return NULL;
  }

  IfStatement *if_stmt = calloc(1, sizeof(IfStatement));
  if (!if_stmt) {
    ast_destroy_node(node);
    return NULL;
  }

  if_stmt->condition = condition;
  if_stmt->then_branch = then_branch;
  if_stmt->else_branch = else_branch;
  node->data = if_stmt;
  ast_add_child(node, condition);
  ast_add_child(node, then_branch);
  if (else_branch) {
    ast_add_child(node, else_branch);
  }
  return node;
}

static ASTNode *async_make_identifier(const char *name,
                                      SourceLocation location) {
  return ast_create_identifier(name, location);
}

static ASTNode *async_make_deref_member(const char *base_name,
                                        const char *member_name,
                                        SourceLocation location) {
  ASTNode *identifier = ast_create_identifier(base_name, location);
  if (!identifier) {
    return NULL;
  }

  ASTNode *deref = ast_create_unary_expression("*", identifier, location);
  if (!deref) {
    ast_destroy_node(identifier);
    return NULL;
  }

  ASTNode *member = ast_create_member_access(deref, member_name, location);
  if (!member) {
    ast_destroy_node(deref);
    return NULL;
  }

  return member;
}

static ASTNode *async_make_cast(ASTNode *operand, const char *type_name,
                                SourceLocation location) {
  if (!operand) {
    return NULL;
  }
  return ast_create_cast_expression(type_name, operand, location);
}

static ASTNode *async_make_call(const char *name, ASTNode **arguments,
                                size_t argument_count,
                                SourceLocation location) {
  return ast_create_call_expression(name, arguments, argument_count, location);
}

static ASTNode *async_make_field_assignment(const char *base_name,
                                            const char *member_name,
                                            ASTNode *value,
                                            SourceLocation location) {
  ASTNode *target = async_make_deref_member(base_name, member_name, location);
  if (!target) {
    ast_destroy_node(value);
    return NULL;
  }
  return ast_create_field_assignment(target, value, location);
}

static int async_is_void_type_name(const char *type_name) {
  return !type_name || strcmp(type_name, "void") == 0;
}

static char *async_strdup_printf(const char *format, ...) {
  if (!format) {
    return NULL;
  }

  va_list args;
  va_start(args, format);
  va_list args_copy;
  va_copy(args_copy, args);
  int size = vsnprintf(NULL, 0, format, args_copy);
  va_end(args_copy);
  if (size < 0) {
    va_end(args);
    return NULL;
  }

  char *buffer = malloc((size_t)size + 1);
  if (!buffer) {
    va_end(args);
    return NULL;
  }

  vsnprintf(buffer, (size_t)size + 1, format, args);
  va_end(args);
  return buffer;
}

static ASTNode *async_make_pointer_difference(ASTNode *lhs, ASTNode *rhs,
                                              SourceLocation location) {
  ASTNode *lhs_int = async_make_cast(lhs, "int64", location);
  ASTNode *rhs_int = async_make_cast(rhs, "int64", location);
  if (!lhs_int || !rhs_int) {
    ast_destroy_node(lhs_int);
    ast_destroy_node(rhs_int);
    return NULL;
  }
  return ast_create_binary_expression(lhs_int, "-", rhs_int, location);
}

static ASTNode *async_make_context_pointer_decl(const char *ctx_var_name,
                                                const char *ctx_type_name,
                                                ASTNode *source_expr,
                                                SourceLocation location) {
  char *pointer_type = async_strdup_printf("%s*", ctx_type_name);
  if (!pointer_type) {
    ast_destroy_node(source_expr);
    return NULL;
  }

  ASTNode *initializer = async_make_cast(source_expr, pointer_type, location);
  if (!initializer) {
    free(pointer_type);
    ast_destroy_node(source_expr);
    return NULL;
  }

  ASTNode *declaration = ast_create_var_declaration(ctx_var_name, pointer_type,
                                                    initializer, location);
  free(pointer_type);
  if (!declaration) {
    ast_destroy_node(initializer);
  }
  return declaration;
}

static int async_node_contains_await(ASTNode *node) {
  if (!node) {
    return 0;
  }

  if (node->type == AST_UNARY_EXPRESSION && node->data) {
    UnaryExpression *unary = (UnaryExpression *)node->data;
    if (unary->operator && strcmp(unary->operator, "await") == 0) {
      return 1;
    }
  }

  for (size_t i = 0; i < node->child_count; i++) {
    if (async_node_contains_await(node->children[i])) {
      return 1;
    }
  }
  return 0;
}

static int async_build_generated_decls(ASTNode *function_node,
                                       size_t ordinal,
                                       AsyncGeneratedDecls *out_generated,
                                       ErrorReporter *reporter,
                                       AsyncRewriteModel model) {
  if (!function_node || function_node->type != AST_FUNCTION_DECLARATION ||
      !out_generated) {
    return 0;
  }

  FunctionDeclaration *function = (FunctionDeclaration *)function_node->data;
  if (!function || !function->name || !function->body) {
    return 1;
  }

  SourceLocation location = function_node->location;

  if (model == ASYNC_REWRITE_MODEL_COROUTINE &&
      async_node_contains_await(function->body)) {
    async_rewrite_report(
        reporter, location,
        "Coroutine async model currently supports async functions without "
        "internal await (function '%s')",
        function->name);
    return 0;
  }
  const char *payload_type = function->return_type ? function->return_type : "void";
  const char *stored_result_type =
      async_is_void_type_name(payload_type) ? "int8" : payload_type;

  char *ctx_name =
      async_strdup_printf("__meth_async_ctx_%s_%zu", function->name, ordinal);
  char *body_name =
      async_strdup_printf("__meth_async_body_%s_%zu", function->name, ordinal);
  char *entry_name =
      async_strdup_printf("__meth_async_entry_%s_%zu", function->name, ordinal);
  char *future_return_type = async_strdup_printf("Future<%s>", payload_type);
  if (!ctx_name || !body_name || !entry_name || !future_return_type) {
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Out of memory while generating async declarations");
    return 0;
  }

  size_t extra_fields = function->parameter_count + 8;
  char **field_names = calloc(extra_fields, sizeof(char *));
  char **field_types = calloc(extra_fields, sizeof(char *));
  if (!field_names || !field_types) {
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    free(field_names);
    free(field_types);
    async_rewrite_report(reporter, location,
                         "Out of memory while generating async context fields");
    return 0;
  }

  size_t field_count = 0;
  field_names[field_count] = "thread_handle";
  field_types[field_count++] = "int64";
  field_names[field_count] = "state";
  field_types[field_count++] = "int32";
  field_names[field_count] = "cancel_requested";
  field_types[field_count++] = "int32";
  field_names[field_count] = "result_offset";
  field_types[field_count++] = "int64";
  field_names[field_count] = "result_size";
  field_types[field_count++] = "int64";
  field_names[field_count] = "entry_fn";
  field_types[field_count++] =
      (model == ASYNC_REWRITE_MODEL_COROUTINE)
          ? "fn(cstring,int64,int32,int32)->int32"
          : "fn(cstring)->uint32";

  for (size_t i = 0; i < function->parameter_count; i++) {
    field_names[field_count] = async_strdup_printf("arg_%zu", i);
    field_types[field_count] = function->parameter_types[i];
    if (!field_names[field_count]) {
      for (size_t j = 6; j < field_count; j++) {
        free(field_names[j]);
      }
      free(field_names);
      free(field_types);
      free(ctx_name);
      free(body_name);
      free(entry_name);
      free(future_return_type);
      async_rewrite_report(reporter, location,
                           "Out of memory while naming async argument fields");
      return 0;
    }
    field_count++;
  }

  field_names[field_count] = "result_value";
  field_types[field_count++] = (char *)stored_result_type;
  field_names[field_count] = "result_end";
  field_types[field_count++] = "int8";

  ASTNode *context_decl = ast_create_struct_declaration(
      ctx_name, field_names, field_types, field_count, NULL, 0, location);
  for (size_t i = 6; i < 6 + function->parameter_count; i++) {
    free(field_names[i]);
  }
  free(field_names);
  free(field_types);
  if (!context_decl) {
    async_rewrite_report(reporter, location,
                         "Failed to create generated async context '%s'",
                         ctx_name);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    return 0;
  }

  ASTNode *body_decl = ast_clone_node(function_node);
  if (!body_decl || body_decl->type != AST_FUNCTION_DECLARATION ||
      !body_decl->data) {
    ast_destroy_node(context_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to clone async function body for '%s'",
                         function->name);
    return 0;
  }

  FunctionDeclaration *body_function = (FunctionDeclaration *)body_decl->data;
  body_function->name = (char *)string_intern(body_name);
  body_function->is_async = 0;
  body_function->is_exported = 0;
  body_function->is_extern = 0;
  body_function->link_name = NULL;

  ASTNode *entry_block = async_make_block();
  if (!entry_block) {
    ast_destroy_node(context_decl);
    ast_destroy_node(body_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to allocate async entry block for '%s'",
                         function->name);
    return 0;
  }

  ASTNode *entry_ctx_decl = async_make_context_pointer_decl(
      "__meth_async_ctx", ctx_name, ast_create_identifier("raw", location),
      location);
  if (!entry_ctx_decl || !async_block_append(entry_block, entry_ctx_decl)) {
    ast_destroy_node(entry_block);
    ast_destroy_node(context_decl);
    ast_destroy_node(body_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to build async entry context for '%s'",
                         function->name);
    return 0;
  }

  ASTNode *cancel_condition = ast_create_binary_expression(
      async_make_deref_member("__meth_async_ctx", "cancel_requested", location),
      "!=", ast_create_number_literal(0, location), location);
  ASTNode *cancel_then = async_make_block();
  if (!cancel_condition || !cancel_then) {
    ast_destroy_node(entry_block);
    ast_destroy_node(context_decl);
    ast_destroy_node(body_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to build async cancellation preamble for '%s'",
                         function->name);
    return 0;
  }

  ASTNode *cancel_finish_arg =
      async_make_cast(async_make_identifier("__meth_async_ctx", location),
                      "cstring", location);
  ASTNode *cancel_finish_args[] = {cancel_finish_arg};
  ASTNode *cancel_finish =
      async_make_call("__meth_async_finish", cancel_finish_args, 1, location);
  ASTNode *cancel_return = async_make_return(
      ast_create_number_literal(
          model == ASYNC_REWRITE_MODEL_COROUTINE ? 1 : 0, location),
      location);
  if (!cancel_finish || !cancel_return ||
      !async_block_append(cancel_then, cancel_finish) ||
      !async_block_append(cancel_then, cancel_return)) {
    ast_destroy_node(cancel_condition);
    ast_destroy_node(cancel_then);
    ast_destroy_node(entry_block);
    ast_destroy_node(context_decl);
    ast_destroy_node(body_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to emit async cancellation guard for '%s'",
                         function->name);
    return 0;
  }

  ASTNode *cancel_if =
      async_make_if(cancel_condition, cancel_then, NULL, location);
  if (!cancel_if || !async_block_append(entry_block, cancel_if)) {
    ast_destroy_node(cancel_if);
    ast_destroy_node(entry_block);
    ast_destroy_node(context_decl);
    ast_destroy_node(body_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to append async cancellation guard for '%s'",
                         function->name);
    return 0;
  }

  ASTNode **body_call_args = NULL;
  if (function->parameter_count > 0) {
    body_call_args = calloc(function->parameter_count, sizeof(ASTNode *));
    if (!body_call_args) {
      ast_destroy_node(entry_block);
      ast_destroy_node(context_decl);
      ast_destroy_node(body_decl);
      free(ctx_name);
      free(body_name);
      free(entry_name);
      free(future_return_type);
      async_rewrite_report(reporter, location,
                           "Out of memory while building async call arguments");
      return 0;
    }
    for (size_t i = 0; i < function->parameter_count; i++) {
      char *arg_field = async_strdup_printf("arg_%zu", i);
      body_call_args[i] =
          arg_field ? async_make_deref_member("__meth_async_ctx", arg_field,
                                              location)
                    : NULL;
      free(arg_field);
      if (!body_call_args[i]) {
        for (size_t j = 0; j < i; j++) {
          ast_destroy_node(body_call_args[j]);
        }
        free(body_call_args);
        ast_destroy_node(entry_block);
        ast_destroy_node(context_decl);
        ast_destroy_node(body_decl);
        free(ctx_name);
        free(body_name);
        free(entry_name);
        free(future_return_type);
        async_rewrite_report(reporter, location,
                             "Failed to build async argument load for '%s'",
                             function->name);
        return 0;
      }
    }
  }

  ASTNode *body_call =
      async_make_call(body_name, body_call_args, function->parameter_count,
                      location);
  free(body_call_args);
  if (!body_call) {
    ast_destroy_node(entry_block);
    ast_destroy_node(context_decl);
    ast_destroy_node(body_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to create async body call for '%s'",
                         function->name);
    return 0;
  }

  if (async_is_void_type_name(payload_type)) {
    if (!async_block_append(entry_block, body_call)) {
      ast_destroy_node(body_call);
      ast_destroy_node(entry_block);
      ast_destroy_node(context_decl);
      ast_destroy_node(body_decl);
      free(ctx_name);
      free(body_name);
      free(entry_name);
      free(future_return_type);
      async_rewrite_report(reporter, location,
                           "Failed to append async void body call for '%s'",
                           function->name);
      return 0;
    }
  } else {
    ASTNode *store_result = async_make_field_assignment(
        "__meth_async_ctx", "result_value", body_call, location);
    if (!store_result || !async_block_append(entry_block, store_result)) {
      ast_destroy_node(store_result);
      ast_destroy_node(entry_block);
      ast_destroy_node(context_decl);
      ast_destroy_node(body_decl);
      free(ctx_name);
      free(body_name);
      free(entry_name);
      free(future_return_type);
      async_rewrite_report(reporter, location,
                           "Failed to store async result for '%s'",
                           function->name);
      return 0;
    }
  }

  ASTNode *finish_arg =
      async_make_cast(async_make_identifier("__meth_async_ctx", location),
                      "cstring", location);
  ASTNode *finish_args[] = {finish_arg};
  ASTNode *finish_call =
      async_make_call("__meth_async_finish", finish_args, 1, location);
  ASTNode *final_return = async_make_return(
      ast_create_number_literal(
          model == ASYNC_REWRITE_MODEL_COROUTINE ? 1 : 0, location),
      location);
  if (!finish_call || !final_return ||
      !async_block_append(entry_block, finish_call) ||
      !async_block_append(entry_block, final_return)) {
    ast_destroy_node(finish_call);
    ast_destroy_node(final_return);
    ast_destroy_node(entry_block);
    ast_destroy_node(context_decl);
    ast_destroy_node(body_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to finish async entry function for '%s'",
                         function->name);
    return 0;
  }

  char *entry_param_names_pool[] = {"raw"};
  char *entry_param_types_pool[] = {"cstring"};
  char *entry_param_names_coro[] = {"raw", "wake_token", "wake_kind",
                                    "wake_result"};
  char *entry_param_types_coro[] = {"cstring", "int64", "int32", "int32"};
  char **entry_param_names =
      (model == ASYNC_REWRITE_MODEL_COROUTINE) ? entry_param_names_coro
                                               : entry_param_names_pool;
  char **entry_param_types =
      (model == ASYNC_REWRITE_MODEL_COROUTINE) ? entry_param_types_coro
                                               : entry_param_types_pool;
  size_t entry_param_count =
      (model == ASYNC_REWRITE_MODEL_COROUTINE) ? 4u : 1u;
  const char *entry_return_type =
      (model == ASYNC_REWRITE_MODEL_COROUTINE) ? "int32" : "uint32";
  ASTNode *entry_decl = ast_create_function_declaration(
      entry_name, entry_param_names, entry_param_types, entry_param_count,
      entry_return_type, entry_block, location);
  if (!entry_decl || !entry_decl->data) {
    ast_destroy_node(entry_block);
    ast_destroy_node(context_decl);
    ast_destroy_node(body_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to create async entry declaration for '%s'",
                         function->name);
    return 0;
  }

  FunctionDeclaration *entry_function = (FunctionDeclaration *)entry_decl->data;
  entry_function->is_exported = 0;
  entry_function->is_async = 0;
  entry_function->is_extern = 0;

  ASTNode *wrapper_block = async_make_block();
  if (!wrapper_block) {
    ast_destroy_node(entry_decl);
    ast_destroy_node(context_decl);
    ast_destroy_node(body_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to allocate async wrapper block for '%s'",
                         function->name);
    return 0;
  }

  char *wrapper_ctx_type = async_strdup_printf("%s*", ctx_name);
  ASTNode *wrapper_ctx_decl =
      wrapper_ctx_type
          ? ast_create_var_declaration("__meth_async_ctx", wrapper_ctx_type,
                                       ast_create_new_expression(ctx_name, location),
                                       location)
          : NULL;
  free(wrapper_ctx_type);
  if (!wrapper_ctx_decl || !async_block_append(wrapper_block, wrapper_ctx_decl)) {
    ast_destroy_node(wrapper_ctx_decl);
    ast_destroy_node(wrapper_block);
    ast_destroy_node(entry_decl);
    ast_destroy_node(context_decl);
    ast_destroy_node(body_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to build async wrapper context for '%s'",
                         function->name);
    return 0;
  }

  ASTNode *init_zero_assignments[3];
  init_zero_assignments[0] = async_make_field_assignment(
      "__meth_async_ctx", "thread_handle", ast_create_number_literal(0, location),
      location);
  init_zero_assignments[1] = async_make_field_assignment(
      "__meth_async_ctx", "state", ast_create_number_literal(0, location),
      location);
  init_zero_assignments[2] = async_make_field_assignment(
      "__meth_async_ctx", "cancel_requested", ast_create_number_literal(0, location),
      location);
  for (size_t i = 0; i < 3; i++) {
    if (!init_zero_assignments[i] ||
        !async_block_append(wrapper_block, init_zero_assignments[i])) {
      for (size_t j = i; j < 3; j++) {
        ast_destroy_node(init_zero_assignments[j]);
      }
      ast_destroy_node(wrapper_block);
      ast_destroy_node(entry_decl);
      ast_destroy_node(context_decl);
      ast_destroy_node(body_decl);
      free(ctx_name);
      free(body_name);
      free(entry_name);
      free(future_return_type);
      async_rewrite_report(reporter, location,
                           "Failed to initialize async wrapper header for '%s'",
                           function->name);
      return 0;
    }
  }

  ASTNode *result_offset = async_make_pointer_difference(
      ast_create_unary_expression(
          "&", async_make_deref_member("__meth_async_ctx", "result_value", location),
          location),
      async_make_identifier("__meth_async_ctx", location), location);
  ASTNode *result_size = async_make_pointer_difference(
      ast_create_unary_expression(
          "&", async_make_deref_member("__meth_async_ctx", "result_end", location),
          location),
      ast_create_unary_expression(
          "&", async_make_deref_member("__meth_async_ctx", "result_value", location),
          location),
      location);
  ASTNode *result_offset_assign = async_make_field_assignment(
      "__meth_async_ctx", "result_offset", result_offset, location);
  ASTNode *result_size_assign = async_make_field_assignment(
      "__meth_async_ctx", "result_size", result_size, location);
  if (!result_offset_assign || !result_size_assign ||
      !async_block_append(wrapper_block, result_offset_assign) ||
      !async_block_append(wrapper_block, result_size_assign)) {
    ast_destroy_node(result_offset_assign);
    ast_destroy_node(result_size_assign);
    ast_destroy_node(wrapper_block);
    ast_destroy_node(entry_decl);
    ast_destroy_node(context_decl);
    ast_destroy_node(body_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to initialize async result metadata for '%s'",
                         function->name);
    return 0;
  }

  ASTNode *entry_address =
      ast_create_unary_expression("&", ast_create_identifier(entry_name, location),
                                  location);
  ASTNode *entry_assign = async_make_field_assignment("__meth_async_ctx",
                                                      "entry_fn", entry_address,
                                                      location);
  if (!entry_assign || !async_block_append(wrapper_block, entry_assign)) {
    ast_destroy_node(entry_assign);
    ast_destroy_node(wrapper_block);
    ast_destroy_node(entry_decl);
    ast_destroy_node(context_decl);
    ast_destroy_node(body_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to store async entry pointer for '%s'",
                         function->name);
    return 0;
  }

  for (size_t i = 0; i < function->parameter_count; i++) {
    char *arg_field = async_strdup_printf("arg_%zu", i);
    ASTNode *arg_value =
        ast_create_identifier(function->parameter_names[i], location);
    ASTNode *arg_assign =
        arg_field ? async_make_field_assignment("__meth_async_ctx", arg_field,
                                                arg_value, location)
                  : NULL;
    free(arg_field);
    if (!arg_assign || !async_block_append(wrapper_block, arg_assign)) {
      ast_destroy_node(arg_assign);
      ast_destroy_node(wrapper_block);
      ast_destroy_node(entry_decl);
      ast_destroy_node(context_decl);
      ast_destroy_node(body_decl);
      free(ctx_name);
      free(body_name);
      free(entry_name);
      free(future_return_type);
      async_rewrite_report(reporter, location,
                           "Failed to capture async argument for '%s'",
                           function->name);
      return 0;
    }
  }

  ASTNode *start_call = NULL;
  if (model == ASYNC_REWRITE_MODEL_COROUTINE) {
    ASTNode *task_create_args[2];
    ASTNode *task_entry_address = ast_create_unary_expression(
        "&", ast_create_identifier(entry_name, location), location);
    task_create_args[0] = task_entry_address;
    task_create_args[1] =
        async_make_cast(async_make_identifier("__meth_async_ctx", location),
                        "cstring", location);
    ASTNode *task_create = async_make_call("__meth_coro_task_create",
                                           task_create_args, 2, location);
    start_call = async_make_field_assignment("__meth_async_ctx", "thread_handle",
                                             task_create, location);
  } else {
    ASTNode *start_arg =
        async_make_cast(async_make_identifier("__meth_async_ctx", location),
                        "cstring", location);
    ASTNode *start_args[] = {start_arg};
    start_call = async_make_call("__meth_async_start", start_args, 1, location);
  }

  ASTNode *schedule_call = NULL;
  if (model == ASYNC_REWRITE_MODEL_COROUTINE) {
    ASTNode *schedule_args[4];
    schedule_args[0] =
        async_make_deref_member("__meth_async_ctx", "thread_handle", location);
    schedule_args[1] = ast_create_number_literal(0, location);
    schedule_args[2] = ast_create_number_literal(0, location);
    schedule_args[3] = ast_create_number_literal(0, location);
    schedule_call = async_make_call("__meth_coro_task_schedule", schedule_args, 4,
                                    location);
  }

  ASTNode *wrapper_return = async_make_return(
      async_make_cast(ast_create_identifier("__meth_async_ctx", location),
                      future_return_type, location),
      location);
  if (!start_call || !wrapper_return ||
      !async_block_append(wrapper_block, start_call) ||
      (schedule_call && !async_block_append(wrapper_block, schedule_call)) ||
      !async_block_append(wrapper_block, wrapper_return)) {
    ast_destroy_node(start_call);
    ast_destroy_node(schedule_call);
    ast_destroy_node(wrapper_return);
    ast_destroy_node(wrapper_block);
    ast_destroy_node(entry_decl);
    ast_destroy_node(context_decl);
    ast_destroy_node(body_decl);
    free(ctx_name);
    free(body_name);
    free(entry_name);
    free(future_return_type);
    async_rewrite_report(reporter, location,
                         "Failed to finalize async wrapper for '%s'",
                         function->name);
    return 0;
  }

  if (function->body) {
    ast_destroy_node(function->body);
  }
  free(function_node->children);
  function_node->children = NULL;
  function_node->child_count = 0;
  function->body = wrapper_block;
  function->return_type = (char *)string_intern(future_return_type);
  function->is_async = 0;
  ast_add_child(function_node, wrapper_block);

  out_generated->context_decl = context_decl;
  out_generated->body_decl = body_decl;
  out_generated->entry_decl = entry_decl;

  free(ctx_name);
  free(body_name);
  free(entry_name);
  free(future_return_type);
  return 1;
}

int async_rewrite_program(ASTNode *program, ErrorReporter *reporter,
                          AsyncRewriteModel model) {
  if (!program || program->type != AST_PROGRAM) {
    return 0;
  }

  Program *program_data = (Program *)program->data;
  if (!program_data || program_data->declaration_count == 0) {
    return 1;
  }

  size_t async_count = 0;
  for (size_t i = 0; i < program_data->declaration_count; i++) {
    ASTNode *decl = program_data->declarations[i];
    if (!decl || decl->type != AST_FUNCTION_DECLARATION || !decl->data) {
      continue;
    }
    FunctionDeclaration *function = (FunctionDeclaration *)decl->data;
    if (function->is_async && function->body) {
      async_count++;
    }
  }

  if (async_count == 0) {
    return 1;
  }

  size_t new_capacity = program_data->declaration_count + async_count * 3;
  ASTNode **new_declarations = calloc(new_capacity, sizeof(ASTNode *));
  ASTNode **new_children = calloc(new_capacity, sizeof(ASTNode *));
  if (!new_declarations || !new_children) {
    free(new_declarations);
    free(new_children);
    if (reporter) {
      error_reporter_add_error(reporter, ERROR_INTERNAL,
                               source_location_create(0, 0),
                               "Out of memory while rewriting async declarations");
    }
    return 0;
  }

  size_t new_count = 0;
  size_t async_ordinal = 0;
  for (size_t i = 0; i < program_data->declaration_count; i++) {
    ASTNode *decl = program_data->declarations[i];
    if (!decl || decl->type != AST_FUNCTION_DECLARATION || !decl->data) {
      new_declarations[new_count] = decl;
      new_children[new_count] = decl;
      new_count++;
      continue;
    }

    FunctionDeclaration *function = (FunctionDeclaration *)decl->data;
    if (!function->is_async || !function->body) {
      new_declarations[new_count] = decl;
      new_children[new_count] = decl;
      new_count++;
      continue;
    }

    AsyncGeneratedDecls generated = {0};
    if (!async_build_generated_decls(decl, async_ordinal++, &generated, reporter,
                                     model)) {
      free(new_declarations);
      free(new_children);
      return 0;
    }

    new_declarations[new_count] = generated.context_decl;
    new_children[new_count++] = generated.context_decl;
    new_declarations[new_count] = generated.body_decl;
    new_children[new_count++] = generated.body_decl;
    new_declarations[new_count] = generated.entry_decl;
    new_children[new_count++] = generated.entry_decl;
    new_declarations[new_count] = decl;
    new_children[new_count++] = decl;
  }

  free(program_data->declarations);
  free(program->children);
  program_data->declarations = new_declarations;
  program_data->declaration_count = new_count;
  program->children = new_children;
  program->child_count = new_count;
  return 1;
}
