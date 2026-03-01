#include "type_checker.h"
#include "../error/error_reporter.h"
#include "symbol_table.h"
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Type *type_checker_parse_array_type(TypeChecker *checker,
                                           const char *name) {
  if (!checker || !name)
    return NULL;

  const char *lbracket = strchr(name, '[');
  const char *rbracket = lbracket ? strchr(lbracket, ']') : NULL;
  if (!lbracket || !rbracket || rbracket[1] != '\0') {
    return NULL;
  }

  size_t base_len = (size_t)(lbracket - name);
  if (base_len == 0) {
    return NULL;
  }

  char *base_name = malloc(base_len + 1);
  if (!base_name) {
    return NULL;
  }
  memcpy(base_name, name, base_len);
  base_name[base_len] = '\0';

  Type *base_type = type_checker_get_type_by_name(checker, base_name);
  free(base_name);
  if (!base_type) {
    return NULL;
  }

  const char *size_start = lbracket + 1;
  if (size_start == rbracket) {
    return NULL;
  }

  errno = 0;
  char *end_ptr = NULL;
  unsigned long long array_size_ull = strtoull(size_start, &end_ptr, 10);
  if (errno != 0 || !end_ptr || end_ptr != rbracket || array_size_ull == 0 ||
      array_size_ull > SIZE_MAX) {
    return NULL;
  }

  size_t array_size = (size_t)array_size_ull;
  if (base_type->size > 0 && array_size > SIZE_MAX / base_type->size) {
    return NULL;
  }

  Type *array_type = type_create(TYPE_ARRAY, name);
  if (!array_type) {
    return NULL;
  }

  array_type->base_type = base_type;
  array_type->array_size = array_size;
  array_type->size = base_type->size * array_size;
  array_type->alignment = base_type->alignment;

  return array_type;
}

static Type *type_checker_parse_pointer_type(TypeChecker *checker,
                                             const char *name) {
  if (!checker || !name) {
    return NULL;
  }

  size_t name_len = strlen(name);
  size_t pointer_depth = 0;
  while (name_len > 0 && name[name_len - 1] == '*') {
    pointer_depth++;
    name_len--;
  }

  if (pointer_depth == 0 || name_len == 0) {
    return NULL;
  }

  char *base_name = malloc(name_len + 1);
  if (!base_name) {
    return NULL;
  }
  memcpy(base_name, name, name_len);
  base_name[name_len] = '\0';

  Type *base_type = type_checker_get_type_by_name(checker, base_name);
  free(base_name);
  if (!base_type) {
    return NULL;
  }

  Type *current = base_type;
  for (size_t i = 0; i < pointer_depth; i++) {
    const char *current_name = current && current->name ? current->name : "ptr";
    size_t pointer_name_len = strlen(current_name) + 2;
    char *pointer_name = malloc(pointer_name_len);
    if (!pointer_name) {
      return NULL;
    }
    snprintf(pointer_name, pointer_name_len, "%s*", current_name);

    Type *pointer_type = type_create(TYPE_POINTER, pointer_name);
    free(pointer_name);
    if (!pointer_type) {
      return NULL;
    }

    pointer_type->base_type = current;
    pointer_type->size = 8;
    pointer_type->alignment = 8;
    current = pointer_type;
  }

  return current;
}

static int type_checker_types_equal(const Type *lhs, const Type *rhs) {
  if (lhs == rhs) {
    return 1;
  }
  if (!lhs || !rhs) {
    return 0;
  }
  if (lhs->kind != rhs->kind) {
    return 0;
  }

  switch (lhs->kind) {
  case TYPE_POINTER:
    return type_checker_types_equal(lhs->base_type, rhs->base_type);
  case TYPE_ARRAY:
    return lhs->array_size == rhs->array_size &&
           type_checker_types_equal(lhs->base_type, rhs->base_type);
  case TYPE_STRUCT:
    if (lhs->name && rhs->name) {
      return strcmp(lhs->name, rhs->name) == 0;
    }
    return lhs->name == rhs->name;
  case TYPE_FUNCTION_POINTER:
    // Function pointer types with same signature are equal
    if (lhs->fn_param_count != rhs->fn_param_count) {
      return 0;
    }
    // Check return type
    if (!type_checker_types_equal(lhs->fn_return_type, rhs->fn_return_type)) {
      return 0;
    }
    // Check parameter types
    for (size_t i = 0; i < lhs->fn_param_count; i++) {
      if (!type_checker_types_equal(lhs->fn_param_types[i],
                                    rhs->fn_param_types[i])) {
        return 0;
      }
    }
    return 1;
  default:
    return 1;
  }
}

static int type_checker_is_lvalue_expression(ASTNode *expression) {
  if (!expression) {
    return 0;
  }

  switch (expression->type) {
  case AST_IDENTIFIER:
  case AST_MEMBER_ACCESS:
  case AST_INDEX_EXPRESSION:
    return 1;
  case AST_UNARY_EXPRESSION: {
    UnaryExpression *unary = (UnaryExpression *)expression->data;
    return unary && unary->operator && strcmp(unary->operator, "*") == 0;
  }
  default:
    return 0;
  }
}

static int type_checker_eval_integer_constant(ASTNode *expression,
                                              long long *out_value) {
  if (!expression || !out_value) {
    return 0;
  }

  switch (expression->type) {
  case AST_NUMBER_LITERAL: {
    NumberLiteral *literal = (NumberLiteral *)expression->data;
    if (!literal || literal->is_float) {
      return 0;
    }
    *out_value = literal->int_value;
    return 1;
  }

  case AST_UNARY_EXPRESSION: {
    UnaryExpression *unary_expr = (UnaryExpression *)expression->data;
    long long operand = 0;
    if (!unary_expr || !unary_expr->operator || !unary_expr->operand ||
        !type_checker_eval_integer_constant(unary_expr->operand, &operand)) {
      return 0;
    }

    if (strcmp(unary_expr->operator, "+") == 0) {
      *out_value = operand;
      return 1;
    }
    if (strcmp(unary_expr->operator, "-") == 0) {
      *out_value = -operand;
      return 1;
    }
    return 0;
  }

  case AST_BINARY_EXPRESSION: {
    BinaryExpression *binary_expr = (BinaryExpression *)expression->data;
    long long left = 0;
    long long right = 0;
    if (!binary_expr || !binary_expr->operator || !binary_expr->left ||
        !binary_expr->right ||
        !type_checker_eval_integer_constant(binary_expr->left, &left) ||
        !type_checker_eval_integer_constant(binary_expr->right, &right)) {
      return 0;
    }

    if (strcmp(binary_expr->operator, "+") == 0) {
      *out_value = left + right;
      return 1;
    }
    if (strcmp(binary_expr->operator, "-") == 0) {
      *out_value = left - right;
      return 1;
    }
    if (strcmp(binary_expr->operator, "*") == 0) {
      *out_value = left * right;
      return 1;
    }
    if (strcmp(binary_expr->operator, "/") == 0) {
      if (right == 0) {
        return 0;
      }
      *out_value = left / right;
      return 1;
    }
    if (strcmp(binary_expr->operator, "%") == 0) {
      if (right == 0) {
        return 0;
      }
      *out_value = left % right;
      return 1;
    }
    return 0;
  }

  default:
    return 0;
  }
}

static int type_checker_ast_contains_node_type(ASTNode *node,
                                               ASTNodeType target_type) {
  if (!node) {
    return 0;
  }
  if (node->type == target_type) {
    return 1;
  }
  for (size_t i = 0; i < node->child_count; i++) {
    if (type_checker_ast_contains_node_type(node->children[i], target_type)) {
      return 1;
    }
  }
  return 0;
}

static int type_checker_is_null_pointer_constant(ASTNode *expression) {
  long long value = 0;
  return type_checker_eval_integer_constant(expression, &value) && value == 0;
}

static int type_checker_is_gc_managed_pointer_type(Type *type) {
  return type && type->kind == TYPE_POINTER && type->base_type &&
         type->base_type->kind == TYPE_STRUCT;
}

static void type_checker_warn_gc_escape_to_c(TypeChecker *checker,
                                             ASTNode *argument,
                                             const char *callee_name) {
  if (!checker || !checker->error_reporter || !argument || !callee_name) {
    return;
  }

  char message[512];
  snprintf(message, sizeof(message),
           "Managed pointer passed to extern function '%s' may escape GC "
           "visibility; register C-held slots with gc_register_root",
           callee_name);
  error_reporter_add_warning(checker->error_reporter, ERROR_SEMANTIC,
                             argument->location, message);
}

static void type_checker_init_tracker_reset(TypeChecker *checker) {
  if (!checker) {
    return;
  }
  for (size_t i = 0; i < checker->tracked_var_count; i++) {
    free(checker->tracked_var_names[i]);
  }
  checker->tracked_var_count = 0;
  checker->tracked_scope_count = 0;
  checker->tracked_scope_depth = 0;
}

static int type_checker_init_tracker_ensure_var_capacity(TypeChecker *checker) {
  if (!checker) {
    return 0;
  }
  if (checker->tracked_var_count < checker->tracked_var_capacity) {
    return 1;
  }

  size_t new_capacity = checker->tracked_var_capacity == 0
                            ? 16
                            : checker->tracked_var_capacity * 2;
  char **new_names =
      realloc(checker->tracked_var_names, new_capacity * sizeof(char *));
  unsigned char *new_initialized = realloc(
      checker->tracked_var_initialized, new_capacity * sizeof(unsigned char));
  int *new_depths =
      realloc(checker->tracked_var_scope_depth, new_capacity * sizeof(int));
  if (!new_names || !new_initialized || !new_depths) {
    free(new_names);
    free(new_initialized);
    free(new_depths);
    return 0;
  }

  checker->tracked_var_names = new_names;
  checker->tracked_var_initialized = new_initialized;
  checker->tracked_var_scope_depth = new_depths;
  checker->tracked_var_capacity = new_capacity;
  return 1;
}

static int
type_checker_init_tracker_ensure_scope_capacity(TypeChecker *checker) {
  if (!checker) {
    return 0;
  }
  if (checker->tracked_scope_count < checker->tracked_scope_capacity) {
    return 1;
  }

  size_t new_capacity = checker->tracked_scope_capacity == 0
                            ? 8
                            : checker->tracked_scope_capacity * 2;
  size_t *new_markers =
      realloc(checker->tracked_scope_markers, new_capacity * sizeof(size_t));
  if (!new_markers) {
    return 0;
  }
  checker->tracked_scope_markers = new_markers;
  checker->tracked_scope_capacity = new_capacity;
  return 1;
}

static int type_checker_init_tracker_enter_scope(TypeChecker *checker) {
  if (!checker || !type_checker_init_tracker_ensure_scope_capacity(checker)) {
    return 0;
  }
  checker->tracked_scope_markers[checker->tracked_scope_count++] =
      checker->tracked_var_count;
  checker->tracked_scope_depth++;
  return 1;
}

static void type_checker_init_tracker_exit_scope(TypeChecker *checker) {
  if (!checker || checker->tracked_scope_count == 0) {
    return;
  }
  size_t marker =
      checker->tracked_scope_markers[checker->tracked_scope_count - 1];
  checker->tracked_scope_count--;
  while (checker->tracked_var_count > marker) {
    size_t idx = checker->tracked_var_count - 1;
    free(checker->tracked_var_names[idx]);
    checker->tracked_var_names[idx] = NULL;
    checker->tracked_var_initialized[idx] = 0;
    checker->tracked_var_scope_depth[idx] = 0;
    checker->tracked_var_count--;
  }
  if (checker->tracked_scope_depth > 0) {
    checker->tracked_scope_depth--;
  }
}

static int type_checker_init_tracker_declare(TypeChecker *checker,
                                             const char *name,
                                             int initialized) {
  if (!checker || !name) {
    return 0;
  }
  if (!type_checker_init_tracker_ensure_var_capacity(checker)) {
    return 0;
  }

  char *name_copy = strdup(name);
  if (!name_copy) {
    return 0;
  }

  size_t idx = checker->tracked_var_count++;
  checker->tracked_var_names[idx] = name_copy;
  checker->tracked_var_initialized[idx] = initialized ? 1 : 0;
  checker->tracked_var_scope_depth[idx] = checker->tracked_scope_depth;
  return 1;
}

static long long type_checker_init_tracker_find(TypeChecker *checker,
                                                const char *name) {
  if (!checker || !name) {
    return -1;
  }
  for (size_t i = checker->tracked_var_count; i > 0; i--) {
    size_t idx = i - 1;
    if (checker->tracked_var_names[idx] &&
        strcmp(checker->tracked_var_names[idx], name) == 0) {
      return (long long)idx;
    }
  }
  return -1;
}

static int type_checker_init_tracker_is_initialized(TypeChecker *checker,
                                                    const char *name,
                                                    int *known) {
  if (known) {
    *known = 0;
  }
  long long idx = type_checker_init_tracker_find(checker, name);
  if (idx < 0) {
    return 0;
  }
  if (known) {
    *known = 1;
  }
  return checker->tracked_var_initialized[idx] ? 1 : 0;
}

static void type_checker_init_tracker_set_initialized(TypeChecker *checker,
                                                      const char *name) {
  long long idx = type_checker_init_tracker_find(checker, name);
  if (idx >= 0) {
    checker->tracked_var_initialized[idx] = 1;
  }
}

static unsigned char *type_checker_init_tracker_capture(TypeChecker *checker,
                                                        size_t *count) {
  if (count) {
    *count = 0;
  }
  if (!checker) {
    return NULL;
  }
  if (count) {
    *count = checker->tracked_var_count;
  }
  if (checker->tracked_var_count == 0) {
    return NULL;
  }

  unsigned char *snapshot =
      malloc(checker->tracked_var_count * sizeof(unsigned char));
  if (!snapshot) {
    return NULL;
  }
  memcpy(snapshot, checker->tracked_var_initialized,
         checker->tracked_var_count * sizeof(unsigned char));
  return snapshot;
}

static void type_checker_init_tracker_restore(TypeChecker *checker,
                                              const unsigned char *snapshot,
                                              size_t count) {
  if (!checker || !snapshot) {
    return;
  }
  size_t limit =
      count < checker->tracked_var_count ? count : checker->tracked_var_count;
  memcpy(checker->tracked_var_initialized, snapshot,
         limit * sizeof(unsigned char));
}

static int type_checker_statement_guarantees_termination(ASTNode *statement) {
  if (!statement) {
    return 0;
  }

  switch (statement->type) {
  case AST_RETURN_STATEMENT:
  case AST_BREAK_STATEMENT:
  case AST_CONTINUE_STATEMENT:
    return 1;
  case AST_IF_STATEMENT: {
    IfStatement *if_stmt = (IfStatement *)statement->data;
    if (!if_stmt || !if_stmt->then_branch || !if_stmt->else_branch) {
      return 0;
    }
    if (!type_checker_statement_guarantees_termination(if_stmt->then_branch)) {
      return 0;
    }
    for (size_t i = 0; i < if_stmt->else_if_count; i++) {
      if (!if_stmt->else_ifs[i].body ||
          !type_checker_statement_guarantees_termination(
              if_stmt->else_ifs[i].body)) {
        return 0;
      }
    }
    return type_checker_statement_guarantees_termination(if_stmt->else_branch);
  }
  case AST_PROGRAM: {
    for (size_t i = 0; i < statement->child_count; i++) {
      if (type_checker_statement_guarantees_termination(
              statement->children[i])) {
        return 1;
      }
    }
    return 0;
  }
  default:
    return 0;
  }
}

static const char *type_checker_decl_link_name(const char *name, int is_extern,
                                               const char *link_name) {
  if (!is_extern) {
    return name;
  }
  if (link_name && link_name[0] != '\0') {
    return link_name;
  }
  return name;
}

static const char *type_checker_symbol_link_name(const Symbol *symbol) {
  if (!symbol) {
    return NULL;
  }
  if (symbol->is_extern && symbol->link_name && symbol->link_name[0] != '\0') {
    return symbol->link_name;
  }
  return symbol->name;
}

static int type_checker_link_name_matches_symbol(const Symbol *symbol,
                                                 const char *decl_name,
                                                 int decl_is_extern,
                                                 const char *decl_link_name) {
  const char *existing = type_checker_symbol_link_name(symbol);
  const char *incoming =
      type_checker_decl_link_name(decl_name, decl_is_extern, decl_link_name);
  if (!existing || !incoming) {
    return existing == incoming;
  }
  return strcmp(existing, incoming) == 0;
}

TypeChecker *type_checker_create(SymbolTable *symbol_table) {
  return type_checker_create_with_error_reporter(symbol_table, NULL);
}

TypeChecker *
type_checker_create_with_error_reporter(SymbolTable *symbol_table,
                                        ErrorReporter *error_reporter) {
  TypeChecker *checker = malloc(sizeof(TypeChecker));
  if (!checker)
    return NULL;

  checker->symbol_table = symbol_table;
  checker->has_error = 0;
  checker->error_message = NULL;
  checker->error_reporter = error_reporter;
  checker->current_function = NULL;
  checker->current_function_decl = NULL;
  checker->loop_depth = 0;
  checker->switch_depth = 0;
  checker->tracked_var_names = NULL;
  checker->tracked_var_initialized = NULL;
  checker->tracked_var_scope_depth = NULL;
  checker->tracked_var_count = 0;
  checker->tracked_var_capacity = 0;
  checker->tracked_scope_markers = NULL;
  checker->tracked_scope_count = 0;
  checker->tracked_scope_capacity = 0;
  checker->tracked_scope_depth = 0;

  // Initialize built-in type pointers to NULL
  checker->builtin_int8 = NULL;
  checker->builtin_int16 = NULL;
  checker->builtin_int32 = NULL;
  checker->builtin_int64 = NULL;
  checker->builtin_uint8 = NULL;
  checker->builtin_uint16 = NULL;
  checker->builtin_uint32 = NULL;
  checker->builtin_uint64 = NULL;
  checker->builtin_float32 = NULL;
  checker->builtin_float64 = NULL;
  checker->builtin_string = NULL;
  checker->builtin_cstring = NULL;
  checker->builtin_void = NULL;

  // Initialize built-in types
  type_checker_init_builtin_types(checker);

  return checker;
}

void type_checker_destroy(TypeChecker *checker) {
  if (checker) {
    // Clean up built-in types
    type_destroy(checker->builtin_int8);
    type_destroy(checker->builtin_int16);
    type_destroy(checker->builtin_int32);
    type_destroy(checker->builtin_int64);
    type_destroy(checker->builtin_uint8);
    type_destroy(checker->builtin_uint16);
    type_destroy(checker->builtin_uint32);
    type_destroy(checker->builtin_uint64);
    type_destroy(checker->builtin_float32);
    type_destroy(checker->builtin_float64);
    type_destroy(checker->builtin_string);
    type_destroy(checker->builtin_cstring);
    type_destroy(checker->builtin_void);

    for (size_t i = 0; i < checker->tracked_var_count; i++) {
      free(checker->tracked_var_names[i]);
    }
    free(checker->tracked_var_names);
    free(checker->tracked_var_initialized);
    free(checker->tracked_var_scope_depth);
    free(checker->tracked_scope_markers);

    free(checker->error_message);
    free(checker);
  }
}

static int type_checker_register_function_signature(TypeChecker *checker,
                                                    ASTNode *declaration) {
  if (!checker || !declaration ||
      declaration->type != AST_FUNCTION_DECLARATION) {
    return 0;
  }

  FunctionDeclaration *func_decl = (FunctionDeclaration *)declaration->data;
  if (!func_decl || !func_decl->name)
    return 0;

  Symbol *existing =
      symbol_table_lookup_current_scope(checker->symbol_table, func_decl->name);
  if (existing)
    return 1;

  Type *return_type = NULL;
  if (func_decl->return_type) {
    return_type =
        type_checker_get_type_by_name(checker, func_decl->return_type);
    if (!return_type)
      return 0;
  } else {
    return_type = checker->builtin_void;
  }

  Type **param_types = NULL;
  if (func_decl->parameter_count > 0) {
    param_types = malloc(func_decl->parameter_count * sizeof(Type *));
    if (!param_types)
      return 0;
    for (size_t i = 0; i < func_decl->parameter_count; i++) {
      param_types[i] =
          type_checker_get_type_by_name(checker, func_decl->parameter_types[i]);
      if (!param_types[i]) {
        free(param_types);
        return 0;
      }
    }
  }

  char **param_names_copy = NULL;
  if (func_decl->parameter_count > 0) {
    param_names_copy = malloc(func_decl->parameter_count * sizeof(char *));
    if (!param_names_copy) {
      free(param_types);
      return 0;
    }
    for (size_t i = 0; i < func_decl->parameter_count; i++) {
      param_names_copy[i] = strdup(func_decl->parameter_names[i]);
    }
  }

  Symbol *func_symbol =
      symbol_create(func_decl->name, SYMBOL_FUNCTION, return_type);
  if (!func_symbol) {
    for (size_t i = 0; i < func_decl->parameter_count; i++)
      free(param_names_copy[i]);
    free(param_names_copy);
    free(param_types);
    return 0;
  }

  func_symbol->data.function.parameter_count = func_decl->parameter_count;
  func_symbol->data.function.parameter_names = param_names_copy;
  func_symbol->data.function.parameter_types = param_types;
  func_symbol->data.function.return_type = return_type;
  func_symbol->is_extern = func_decl->is_extern;
  if (func_decl->is_extern) {
    const char *effective_link_name = type_checker_decl_link_name(
        func_decl->name, func_decl->is_extern, func_decl->link_name);
    func_symbol->link_name =
        effective_link_name ? strdup(effective_link_name) : NULL;
    if (!func_symbol->link_name) {
      symbol_destroy(func_symbol);
      return 0;
    }
  }
  func_symbol->is_initialized = 0;
  func_symbol->is_forward_declaration = 1;

  if (!symbol_table_declare_forward(checker->symbol_table, func_symbol)) {
    symbol_destroy(func_symbol);
    return 0;
  }

  return 1;
}

int type_checker_check_program(TypeChecker *checker, ASTNode *program) {
  if (!checker || !program || program->type != AST_PROGRAM) {
    return 0;
  }

  Program *prog = (Program *)program->data;
  if (!prog)
    return 0;

  // Pass 1: Register struct and enum types
  for (size_t i = 0; i < prog->declaration_count; i++) {
    ASTNode *decl = prog->declarations[i];
    if (decl && decl->type == AST_STRUCT_DECLARATION) {
      if (!type_checker_process_struct_declaration(checker, decl)) {
        return 0;
      }
    } else if (decl && decl->type == AST_ENUM_DECLARATION) {
      if (!type_checker_process_enum_declaration(checker, decl)) {
        return 0;
      }
    }
  }

  // Pass 2: Register all function signatures so any function can call any other
  for (size_t i = 0; i < prog->declaration_count; i++) {
    ASTNode *decl = prog->declarations[i];
    if (decl && decl->type == AST_FUNCTION_DECLARATION) {
      type_checker_register_function_signature(checker, decl);
    }
  }

  // Pass 3: Process all declarations (type-check function bodies, etc.)
  for (size_t i = 0; i < prog->declaration_count; i++) {
    ASTNode *decl = prog->declarations[i];
    if (decl && decl->type != AST_STRUCT_DECLARATION &&
        decl->type != AST_ENUM_DECLARATION) {
      if (!type_checker_process_declaration(checker, decl)) {
        return 0;
      }
    }
  }

  return 1;
}

static Type *type_checker_infer_type_internal(TypeChecker *checker,
                                              ASTNode *expression);

Type *type_checker_infer_type(TypeChecker *checker, ASTNode *expression) {
  if (!checker || !expression)
    return NULL;

  if (expression->resolved_type) {
    return expression->resolved_type;
  }

  Type *type = type_checker_infer_type_internal(checker, expression);
  expression->resolved_type = type;
  return type;
}

static Type *type_checker_infer_type_internal(TypeChecker *checker,
                                              ASTNode *expression) {
  if (!checker || !expression)
    return NULL;

  switch (expression->type) {
  case AST_NUMBER_LITERAL: {
    NumberLiteral *literal = (NumberLiteral *)expression->data;
    if (literal->is_float) {
      // Floating literals default to float64
      return checker->builtin_float64;
    } else {
      // Integer literals default to int32
      return checker->builtin_int32;
    }
  }

  case AST_STRING_LITERAL:
    // String literals are string type
    return checker->builtin_string;

  case AST_IDENTIFIER: {
    Identifier *id = (Identifier *)expression->data;
    Symbol *symbol = symbol_table_lookup(checker->symbol_table, id->name);
    if (!symbol) {
      type_checker_report_undefined_symbol(checker, expression->location,
                                           id->name, "variable");
      return NULL;
    }
    if (checker->current_function &&
        (symbol->kind == SYMBOL_VARIABLE || symbol->kind == SYMBOL_PARAMETER) &&
        symbol->scope && symbol->scope->type != SCOPE_GLOBAL) {
      int skip_uninit_check =
          symbol->type && (symbol->type->kind == TYPE_ARRAY ||
                           symbol->type->kind == TYPE_STRUCT ||
                           symbol->type->kind == TYPE_STRING);
      int known = 0;
      int initialized =
          type_checker_init_tracker_is_initialized(checker, id->name, &known);
      if (!skip_uninit_check && known && !initialized) {
        type_checker_set_error_at_location(
            checker, expression->location,
            "Variable '%s' may be used before initialization", id->name);
        return NULL;
      }
    }
    return symbol->type;
  }

  case AST_BINARY_EXPRESSION: {
    BinaryExpression *binop = (BinaryExpression *)expression->data;
    return type_checker_check_binary_expression(checker, binop,
                                                expression->location);
  }

  case AST_UNARY_EXPRESSION: {
    UnaryExpression *unop = (UnaryExpression *)expression->data;
    if (!unop || !unop->operator || !unop->operand) {
      type_checker_set_error_at_location(checker, expression->location,
                                         "Invalid unary expression");
      return NULL;
    }

    if (strcmp(unop->operator, "&") == 0) {
      // Check if operand is an identifier that refers to a function
      if (unop->operand->type == AST_IDENTIFIER) {
        Identifier *id = (Identifier *)unop->operand->data;
        if (id && id->name) {
          Symbol *sym = symbol_table_lookup(checker->symbol_table, id->name);
          if (sym && sym->kind == SYMBOL_FUNCTION) {
            // Taking address of a function - create function pointer type
            Type **param_types = sym->data.function.parameter_types;
            size_t param_count = sym->data.function.parameter_count;
            Type *return_type = sym->data.function.return_type;
            if (!return_type) {
              return_type = checker->builtin_void;
            }
            Type *fp_type = type_create_function_pointer(
                param_types, param_count, return_type);
            if (!fp_type) {
              type_checker_set_error_at_location(
                  checker, expression->location,
                  "Failed to create function pointer type");
              return NULL;
            }
            return fp_type;
          }
        }
      }

      // Not a function reference - treat as regular address-of
      if (!type_checker_is_lvalue_expression(unop->operand)) {
        type_checker_set_error_at_location(
            checker, unop->operand->location,
            "Address-of operator requires an assignable expression");
        return NULL;
      }

      Type *operand_type = type_checker_infer_type(checker, unop->operand);
      if (!operand_type) {
        return NULL;
      }

      const char *operand_name =
          operand_type->name ? operand_type->name : "unknown";
      size_t pointer_name_len = strlen(operand_name) + 2;
      char *pointer_name = malloc(pointer_name_len);
      if (!pointer_name) {
        type_checker_set_error_at_location(checker, expression->location,
                                           "Memory allocation failed");
        return NULL;
      }
      snprintf(pointer_name, pointer_name_len, "%s*", operand_name);

      Type *pointer_type = type_checker_get_type_by_name(checker, pointer_name);
      free(pointer_name);
      if (!pointer_type) {
        type_checker_set_error_at_location(checker, expression->location,
                                           "Failed to resolve pointer type");
        return NULL;
      }

      return pointer_type;
    }

    if (strcmp(unop->operator, "*") == 0) {
      Type *operand_type = type_checker_infer_type(checker, unop->operand);
      if (!operand_type) {
        return NULL;
      }
      if (type_checker_is_null_pointer_constant(unop->operand)) {
        type_checker_set_error_at_location(checker, expression->location,
                                           "Null pointer dereference");
        return NULL;
      }
      if (operand_type->kind != TYPE_POINTER || !operand_type->base_type) {
        type_checker_set_error_at_location(
            checker, expression->location,
            "Dereference operator requires a pointer operand");
        return NULL;
      }
      return operand_type->base_type;
    }

    Type *operand_type = type_checker_infer_type(checker, unop->operand);
    if (!operand_type) {
      return NULL;
    }

    if (strcmp(unop->operator, "+") == 0 || strcmp(unop->operator, "-") == 0) {
      if (!type_checker_is_numeric_type(operand_type)) {
        type_checker_report_type_mismatch(checker, unop->operand->location,
                                          "numeric type", operand_type->name);
        return NULL;
      }
      return operand_type;
    }

    if (strcmp(unop->operator, "~") == 0) {
      if (!type_checker_is_integer_type(operand_type)) {
        type_checker_report_type_mismatch(checker, unop->operand->location,
                                          "integer type", operand_type->name);
        return NULL;
      }
      return operand_type;
    }

    if (strcmp(unop->operator, "!") == 0) {
      if (!type_checker_is_numeric_type(operand_type) &&
          operand_type->kind != TYPE_POINTER) {
        type_checker_report_type_mismatch(checker, unop->operand->location,
                                          "numeric or pointer type",
                                          operand_type->name);
        return NULL;
      }
      // Logical NOT always produces an int32 (0 or 1)
      return checker->builtin_int32;
    }

    type_checker_set_error_at_location(checker, expression->location,
                                       "Unsupported unary operator '%s'",
                                       unop->operator);
    return NULL;
  }

  case AST_FUNCTION_CALL: {
    CallExpression *call = (CallExpression *)expression->data;
    Symbol *func_symbol =
        symbol_table_lookup(checker->symbol_table, call->function_name);
    if (!func_symbol) {
      type_checker_report_undefined_symbol(checker, expression->location,
                                           call->function_name, "function");
      return NULL;
    }

    /* Variable with function pointer type can be called like a function */
    if (func_symbol->kind == SYMBOL_VARIABLE && func_symbol->type &&
        func_symbol->type->kind == TYPE_FUNCTION_POINTER) {
      call->is_indirect_call = 1;
      Type *fp_type = func_symbol->type;
      if (call->argument_count != fp_type->fn_param_count) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg),
                 "Function pointer expects %llu arguments, got %llu",
                 (unsigned long long)fp_type->fn_param_count,
                 (unsigned long long)call->argument_count);
        type_checker_set_error_at_location(checker, expression->location,
                                           error_msg);
        return NULL;
      }
      for (size_t i = 0; i < call->argument_count; i++) {
        Type *arg_type = type_checker_infer_type(checker, call->arguments[i]);
        if (!arg_type)
          return NULL;
        Type *param_type = fp_type->fn_param_types[i];
        int is_null =
            (param_type && param_type->kind == TYPE_POINTER &&
             type_checker_is_null_pointer_constant(call->arguments[i]));
        if (!is_null &&
            !type_checker_is_assignable(checker, param_type, arg_type)) {
          type_checker_report_type_mismatch(checker,
                                            call->arguments[i]->location,
                                            param_type->name, arg_type->name);
          return NULL;
        }
      }
      return fp_type->fn_return_type;
    }

    if (func_symbol->kind != SYMBOL_FUNCTION) {
      const char *symbol_type =
          (func_symbol->kind == SYMBOL_VARIABLE) ? "variable"
          : (func_symbol->kind == SYMBOL_STRUCT) ? "struct"
                                                 : "symbol";
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg), "'%s' is a %s, not a function",
               call->function_name, symbol_type);
      type_checker_set_error_at_location(checker, expression->location,
                                         error_msg);
      return NULL;
    }

    // Check argument count
    if (call->argument_count != func_symbol->data.function.parameter_count) {
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg),
               "Function '%s' expects %llu arguments, got %llu",
               call->function_name,
               (unsigned long long)func_symbol->data.function.parameter_count,
               (unsigned long long)call->argument_count);
      type_checker_set_error_at_location(checker, expression->location,
                                         error_msg);
      return NULL;
    }

    // Check each argument type
    for (size_t i = 0; i < call->argument_count; i++) {
      Type *arg_type = type_checker_infer_type(checker, call->arguments[i]);
      if (!arg_type) {
        // Error already set by type inference
        return NULL;
      }

      Type *param_type = func_symbol->data.function.parameter_types[i];
      int is_null_pointer_arg =
          (param_type && param_type->kind == TYPE_POINTER &&
           type_checker_is_null_pointer_constant(call->arguments[i]));
      // Allow implicit string literal -> cstring coercion.
      // A string literal's .chars is always a valid null-terminated pointer.
      int is_string_to_cstring =
          (param_type && param_type->name &&
           strcmp(param_type->name, "cstring") == 0 &&
           call->arguments[i]->type == AST_STRING_LITERAL);
      if (!is_null_pointer_arg && !is_string_to_cstring &&
          !type_checker_is_assignable(checker, param_type, arg_type)) {
        type_checker_report_type_mismatch(checker, call->arguments[i]->location,
                                          param_type->name, arg_type->name);
        return NULL;
      }

      if (func_symbol->is_extern &&
          type_checker_is_gc_managed_pointer_type(arg_type)) {
        type_checker_warn_gc_escape_to_c(checker, call->arguments[i],
                                         call->function_name);
      }
    }

    return func_symbol->data.function.return_type;
  }

  case AST_FUNC_PTR_CALL: {
    FuncPtrCall *fp_call = (FuncPtrCall *)expression->data;
    if (!fp_call || !fp_call->function) {
      type_checker_set_error_at_location(checker, expression->location,
                                         "Invalid function pointer call");
      return NULL;
    }

    Type *func_type = type_checker_infer_type(checker, fp_call->function);
    if (!func_type) {
      return NULL;
    }

    /* If expression is identifier resolving to a function, synthesize function
     * pointer type */
    if (func_type->kind != TYPE_FUNCTION_POINTER &&
        fp_call->function->type == AST_IDENTIFIER) {
      Identifier *id = (Identifier *)fp_call->function->data;
      Symbol *sym = symbol_table_lookup(checker->symbol_table, id->name);
      if (sym && sym->kind == SYMBOL_FUNCTION) {
        Type **param_types = sym->data.function.parameter_types;
        size_t param_count = sym->data.function.parameter_count;
        Type *return_type = sym->data.function.return_type;
        if (!return_type)
          return_type = checker->builtin_void;
        func_type =
            type_create_function_pointer(param_types, param_count, return_type);
        if (!func_type) {
          type_checker_set_error_at_location(
              checker, expression->location,
              "Failed to create function pointer type");
          return NULL;
        }
      }
    }

    if (func_type->kind != TYPE_FUNCTION_POINTER) {
      type_checker_set_error_at_location(
          checker, expression->location,
          "Cannot call non-function-pointer expression");
      return NULL;
    }

    // Check argument count
    if (fp_call->argument_count != func_type->fn_param_count) {
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg),
               "Function pointer expects %llu arguments, got %llu",
               (unsigned long long)func_type->fn_param_count,
               (unsigned long long)fp_call->argument_count);
      type_checker_set_error_at_location(checker, expression->location,
                                         error_msg);
      return NULL;
    }

    // Check each argument type
    for (size_t i = 0; i < fp_call->argument_count; i++) {
      Type *arg_type = type_checker_infer_type(checker, fp_call->arguments[i]);
      if (!arg_type) {
        return NULL;
      }

      Type *param_type = func_type->fn_param_types[i];
      int is_null_pointer_arg =
          (param_type && param_type->kind == TYPE_POINTER &&
           type_checker_is_null_pointer_constant(fp_call->arguments[i]));
      if (!is_null_pointer_arg &&
          !type_checker_is_assignable(checker, param_type, arg_type)) {
        type_checker_report_type_mismatch(checker,
                                          fp_call->arguments[i]->location,
                                          param_type->name, arg_type->name);
        return NULL;
      }
    }

    // Return the function pointer's return type
    return func_type->fn_return_type;
  }

  case AST_MEMBER_ACCESS: {
    MemberAccess *member = (MemberAccess *)expression->data;
    Type *object_type = type_checker_infer_type(checker, member->object);
    if (object_type && (object_type->kind == TYPE_STRUCT ||
                        object_type->kind == TYPE_STRING)) {
      // Look up the field type in the struct
      Type *field_type = type_get_field_type(object_type, member->member);
      if (field_type) {
        return field_type;
      } else {
        // Field not found in struct - this is an error
        SourceLocation location = expression->location;
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg),
                 "Field '%s' not found in type '%s'", member->member,
                 object_type->name);
        type_checker_set_error_at_location(checker, location, error_msg);
        return NULL;
      }
    } else if (object_type) {
      // Trying to access member on non-struct type
      SourceLocation location = expression->location;
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg),
               "Cannot access field on non-struct type '%s'",
               object_type->name);
      type_checker_set_error_at_location(checker, location, error_msg);
      return NULL;
    }
    return NULL;
  }

  case AST_INDEX_EXPRESSION: {
    ArrayIndexExpression *idx = (ArrayIndexExpression *)expression->data;
    if (!idx || !idx->array || !idx->index) {
      type_checker_set_error_at_location(checker, expression->location,
                                         "Invalid array indexing expression");
      return NULL;
    }

    Type *array_type = type_checker_infer_type(checker, idx->array);
    if (!array_type) {
      return NULL;
    }

    Type *index_type = type_checker_infer_type(checker, idx->index);
    if (!index_type) {
      return NULL;
    }

    if (!type_checker_is_integer_type(index_type)) {
      type_checker_report_type_mismatch(checker, idx->index->location,
                                        "integer type", index_type->name);
      return NULL;
    }

    if (array_type->kind == TYPE_ARRAY || array_type->kind == TYPE_POINTER) {
      if (!array_type->base_type) {
        type_checker_set_error_at_location(checker, expression->location,
                                           "Indexed type has no element type");
        return NULL;
      }
      if (array_type->kind == TYPE_POINTER &&
          type_checker_is_null_pointer_constant(idx->array)) {
        type_checker_set_error_at_location(checker, idx->array->location,
                                           "Null pointer dereference");
        return NULL;
      }
      if (array_type->kind == TYPE_ARRAY) {
        long long constant_index = 0;
        if (type_checker_eval_integer_constant(idx->index, &constant_index)) {
          if (constant_index < 0 ||
              (unsigned long long)constant_index >=
                  (unsigned long long)array_type->array_size) {
            type_checker_set_error_at_location(
                checker, idx->index->location,
                "Array index %lld is out of bounds for '%s' (size %zu)",
                constant_index, array_type->name ? array_type->name : "array",
                array_type->array_size);
            return NULL;
          }
        }
      }
      return array_type->base_type;
    }

    type_checker_set_error_at_location(checker, expression->location,
                                       "Cannot index non-array type '%s'",
                                       array_type->name);
    return NULL;
  }

  case AST_ASSIGNMENT: {
    Assignment *assignment = (Assignment *)expression->data;
    if (assignment && assignment->value) {
      return type_checker_infer_type(checker, assignment->value);
    }
    return NULL;
  }

  case AST_NEW_EXPRESSION: {
    NewExpression *new_expr = (NewExpression *)expression->data;
    if (!new_expr || !new_expr->type_name) {
      type_checker_set_error_at_location(checker, expression->location,
                                         "Invalid 'new' expression");
      return NULL;
    }

    // Look up the type by name
    Symbol *type_symbol =
        symbol_table_lookup(checker->symbol_table, new_expr->type_name);
    if (!type_symbol || type_symbol->kind != SYMBOL_STRUCT) {
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg),
               "Struct type '%s' not found for allocation",
               new_expr->type_name);
      type_checker_set_error_at_location(checker, expression->location,
                                         error_msg);
      return NULL;
    }

    size_t pointer_name_len = strlen(new_expr->type_name) + 2;
    char *pointer_name = malloc(pointer_name_len);
    if (!pointer_name) {
      type_checker_set_error_at_location(checker, expression->location,
                                         "Memory allocation failed");
      return NULL;
    }
    snprintf(pointer_name, pointer_name_len, "%s*", new_expr->type_name);

    Type *pointer_type = type_checker_get_type_by_name(checker, pointer_name);
    free(pointer_name);
    if (!pointer_type) {
      type_checker_set_error_at_location(checker, expression->location,
                                         "Failed to resolve pointer type");
      return NULL;
    }

    return pointer_type;
  }

  case AST_CAST_EXPRESSION: {
    CastExpression *cast_expr = (CastExpression *)expression->data;
    if (!cast_expr || !cast_expr->type_name || !cast_expr->operand) {
      type_checker_set_error_at_location(checker, expression->location,
                                         "Invalid cast expression");
      return NULL;
    }

    Type *target_type =
        type_checker_get_type_by_name(checker, cast_expr->type_name);
    if (!target_type) {
      type_checker_set_error_at_location(checker, expression->location,
                                         "Unknown target type for cast");
      return NULL;
    }

    Type *operand_type = type_checker_infer_type(checker, cast_expr->operand);
    if (!operand_type) {
      return NULL; // Error already reported
    }

    if (!type_checker_is_cast_valid(operand_type, target_type)) {
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg),
               "Cannot cast from type '%s' to type '%s'", operand_type->name,
               target_type->name);
      type_checker_set_error_at_location(checker, expression->location,
                                         error_msg);
      return NULL;
    }

    return target_type;
  }

  default:
    return NULL;
  }
}

int type_checker_are_compatible(Type *type1, Type *type2) {
  if (!type1 || !type2)
    return 0;

  if (type_checker_types_equal(type1, type2)) {
    return 1;
  }

  if (type1->kind == TYPE_POINTER || type2->kind == TYPE_POINTER ||
      type1->kind == TYPE_ARRAY || type2->kind == TYPE_ARRAY ||
      type1->kind == TYPE_STRUCT || type2->kind == TYPE_STRUCT) {
    return 0;
  }

  // Check for implicit numeric conversions
  return type_checker_is_implicitly_convertible(type1, type2) ||
         type_checker_is_implicitly_convertible(type2, type1);
}

// Built-in type system functions implementation

void type_checker_init_builtin_types(TypeChecker *checker) {
  if (!checker)
    return;

  // Create built-in integer types
  checker->builtin_int8 = type_create(TYPE_INT8, "int8");
  checker->builtin_int16 = type_create(TYPE_INT16, "int16");
  checker->builtin_int32 = type_create(TYPE_INT32, "int32");
  checker->builtin_int64 = type_create(TYPE_INT64, "int64");

  // Create built-in unsigned integer types
  checker->builtin_uint8 = type_create(TYPE_UINT8, "uint8");
  checker->builtin_uint16 = type_create(TYPE_UINT16, "uint16");
  checker->builtin_uint32 = type_create(TYPE_UINT32, "uint32");
  checker->builtin_uint64 = type_create(TYPE_UINT64, "uint64");

  // Create built-in floating-point types
  checker->builtin_float32 = type_create(TYPE_FLOAT32, "float32");
  checker->builtin_float64 = type_create(TYPE_FLOAT64, "float64");

  // C interop alias: cstring -> uint8*
  checker->builtin_cstring = type_create(TYPE_POINTER, "cstring");
  if (checker->builtin_cstring) {
    checker->builtin_cstring->base_type = checker->builtin_uint8;
    checker->builtin_cstring->size = 8;
    checker->builtin_cstring->alignment = 8;
  }

  // Create built-in string type backed by a uint8* and length
  checker->builtin_string = type_create(TYPE_STRING, "string");
  if (checker->builtin_string) {
    checker->builtin_string->size = 16;
    checker->builtin_string->alignment = 8;

    checker->builtin_string->field_count = 2;
    checker->builtin_string->field_names = malloc(2 * sizeof(char *));
    checker->builtin_string->field_types = malloc(2 * sizeof(Type *));
    checker->builtin_string->field_offsets = malloc(2 * sizeof(size_t));

    checker->builtin_string->field_names[0] = strdup("chars");
    checker->builtin_string->field_types[0] =
        type_create(TYPE_POINTER, "uint8*");
    checker->builtin_string->field_types[0]->base_type = checker->builtin_uint8;
    checker->builtin_string->field_types[0]->size = 8;
    checker->builtin_string->field_types[0]->alignment = 8;
    checker->builtin_string->field_offsets[0] = 0;

    checker->builtin_string->field_names[1] = strdup("length");
    checker->builtin_string->field_types[1] = checker->builtin_uint64;
    checker->builtin_string->field_offsets[1] = 8;
  }

  // Create built-in void type
  checker->builtin_void = type_create(TYPE_VOID, "void");
  if (checker->builtin_void) {
    checker->builtin_void->size = 0;
    checker->builtin_void->alignment = 1;
  }
}

Type *type_checker_get_builtin_type(TypeChecker *checker, TypeKind kind) {
  if (!checker)
    return NULL;

  switch (kind) {
  case TYPE_INT8:
    return checker->builtin_int8;
  case TYPE_INT16:
    return checker->builtin_int16;
  case TYPE_INT32:
    return checker->builtin_int32;
  case TYPE_INT64:
    return checker->builtin_int64;
  case TYPE_UINT8:
    return checker->builtin_uint8;
  case TYPE_UINT16:
    return checker->builtin_uint16;
  case TYPE_UINT32:
    return checker->builtin_uint32;
  case TYPE_UINT64:
    return checker->builtin_uint64;
  case TYPE_FLOAT32:
    return checker->builtin_float32;
  case TYPE_FLOAT64:
    return checker->builtin_float64;
  case TYPE_STRING:
    return checker->builtin_string;
  case TYPE_VOID:
    return checker->builtin_void;
  default:
    return NULL;
  }
}

static Type *type_checker_parse_function_pointer_type(TypeChecker *checker,
                                                      const char *name) {
  if (!checker || !name) {
    return NULL;
  }

  // Check if it's a function pointer type: fn(param1,param2)->returntype
  if (strlen(name) < 4 || strncmp(name, "fn(", 3) != 0) {
    return NULL;
  }

  // Find the -> that separates params from return type
  const char *arrow = strstr(name, ")->");
  if (!arrow) {
    return NULL;
  }

  // Parse parameter types
  const char *params_start = name + 3; // skip "fn("
  const char *params_end = arrow;
  size_t params_len = params_end - params_start;

  Type **param_types = NULL;
  size_t param_count = 0;

  if (params_len > 0) {
    // Parse comma-separated parameter types
    char *params_copy = malloc(params_len + 1);
    if (!params_copy) {
      return NULL;
    }
    memcpy(params_copy, params_start, params_len);
    params_copy[params_len] = '\0';

    // Count parameters by counting commas
    param_count = 1;
    for (size_t i = 0; i < params_len; i++) {
      if (params_copy[i] == ',') {
        param_count++;
      }
    }

    param_types = calloc(param_count, sizeof(Type *));
    if (!param_types) {
      free(params_copy);
      return NULL;
    }

    // Parse each parameter type
    char *param_start = params_copy;
    size_t param_idx = 0;
    for (size_t i = 0; i <= params_len; i++) {
      if (params_copy[i] == ',' || params_copy[i] == '\0') {
        params_copy[i] = '\0';
        Type *param_type = type_checker_get_type_by_name(checker, param_start);
        if (!param_type) {
          free(params_copy);
          free(param_types);
          return NULL;
        }
        if (param_idx < param_count) {
          param_types[param_idx++] = param_type;
        }
        param_start = params_copy + i + 1;
      }
    }

    free(params_copy);
  }

  // Parse return type
  const char *return_type_start = arrow + 3; // skip ")->"
  Type *return_type = type_checker_get_type_by_name(checker, return_type_start);
  if (!return_type) {
    // Default to void if return type not found
    return_type = checker->builtin_void;
  }

  Type *fp_type =
      type_create_function_pointer(param_types, param_count, return_type);
  if (!fp_type) {
    free(param_types);
    return NULL;
  }

  return fp_type;
}

Type *type_checker_get_type_by_name(TypeChecker *checker, const char *name) {
  if (!checker || !name)
    return NULL;

  // Check built-in types by name
  if (strcmp(name, "int8") == 0)
    return checker->builtin_int8;
  if (strcmp(name, "int16") == 0)
    return checker->builtin_int16;
  if (strcmp(name, "int32") == 0)
    return checker->builtin_int32;
  if (strcmp(name, "int64") == 0)
    return checker->builtin_int64;
  if (strcmp(name, "uint8") == 0)
    return checker->builtin_uint8;
  if (strcmp(name, "uint16") == 0)
    return checker->builtin_uint16;
  if (strcmp(name, "uint32") == 0)
    return checker->builtin_uint32;
  if (strcmp(name, "uint64") == 0)
    return checker->builtin_uint64;
  if (strcmp(name, "float32") == 0)
    return checker->builtin_float32;
  if (strcmp(name, "float64") == 0)
    return checker->builtin_float64;
  if (strcmp(name, "string") == 0)
    return checker->builtin_string;
  if (strcmp(name, "cstring") == 0)
    return checker->builtin_cstring;
  if (strcmp(name, "void") == 0)
    return checker->builtin_void;

  // Check for function pointer types: fn(param1,param2)->returntype
  if (strncmp(name, "fn(", 3) == 0) {
    Type *fp_type = type_checker_parse_function_pointer_type(checker, name);
    if (fp_type) {
      return fp_type;
    }
  }

  if (strchr(name, '[') && strchr(name, ']')) {
    Type *array_type = type_checker_parse_array_type(checker, name);
    if (array_type) {
      return array_type;
    }
  }

  if (strchr(name, '*')) {
    Type *pointer_type = type_checker_parse_pointer_type(checker, name);
    if (pointer_type) {
      return pointer_type;
    }
  }

  // Check for user-defined types in symbol table
  Symbol *struct_symbol = symbol_table_lookup(checker->symbol_table, name);
  if (struct_symbol && (struct_symbol->kind == SYMBOL_STRUCT ||
                        struct_symbol->kind == SYMBOL_ENUM)) {
    return struct_symbol->type;
  }

  return NULL;
}

int type_checker_is_integer_type(Type *type) {
  if (!type)
    return 0;

  switch (type->kind) {
  case TYPE_INT8:
  case TYPE_INT16:
  case TYPE_INT32:
  case TYPE_INT64:
  case TYPE_UINT8:
  case TYPE_UINT16:
  case TYPE_UINT32:
  case TYPE_UINT64:
  case TYPE_ENUM:
    return 1;
  default:
    return 0;
  }
}

int type_checker_is_floating_type(Type *type) {
  if (!type)
    return 0;

  switch (type->kind) {
  case TYPE_FLOAT32:
  case TYPE_FLOAT64:
    return 1;
  default:
    return 0;
  }
}

int type_checker_is_numeric_type(Type *type) {
  return type_checker_is_integer_type(type) ||
         type_checker_is_floating_type(type);
}

size_t type_checker_get_type_size(Type *type) {
  if (!type)
    return 0;
  return type->size;
}

size_t type_checker_get_type_alignment(Type *type) {
  if (!type)
    return 1;
  return type->alignment;
}

// Type inference and promotion functions implementation

Type *type_checker_promote_types(TypeChecker *checker, Type *left, Type *right,
                                 const char *operator) {
  if (!checker || !left || !right || !operator)
    return NULL;

  // For comparison operators, result is always int32 (boolean represented as
  // int)
  if (strcmp(operator, "==") == 0 || strcmp(operator, "!=") == 0 ||
      strcmp(operator, "<") == 0 || strcmp(operator, "<=") == 0 ||
      strcmp(operator, ">") == 0 || strcmp(operator, ">=") == 0) {
    return checker->builtin_int32;
  }

  // For arithmetic operators, promote to larger type
  if (strcmp(operator, "+") == 0 || strcmp(operator, "-") == 0 ||
      strcmp(operator, "*") == 0 || strcmp(operator, "/") == 0 ||
      strcmp(operator, "%") == 0) {

    // If either operand is floating-point, result is floating-point
    if (type_checker_is_floating_type(left) ||
        type_checker_is_floating_type(right)) {
      return type_checker_get_larger_type(checker, left, right);
    }

    // Both are integers, promote to larger integer type
    if (type_checker_is_integer_type(left) &&
        type_checker_is_integer_type(right)) {
      return type_checker_get_larger_type(checker, left, right);
    }
  }

  // For logical operators, result is int32 (boolean)
  if (strcmp(operator, "&&") == 0 || strcmp(operator, "||") == 0) {
    return checker->builtin_int32;
  }

  // Default: return left type
  return left;
}

Type *type_checker_get_larger_type(TypeChecker *checker, Type *type1,
                                   Type *type2) {
  if (!checker || !type1 || !type2)
    return NULL;

  int rank1 = type_checker_get_type_rank(type1);
  int rank2 = type_checker_get_type_rank(type2);

  // Return the type with higher rank
  return (rank1 >= rank2) ? type1 : type2;
}

int type_checker_get_type_rank(Type *type) {
  if (!type)
    return -1;

  // Type promotion ranking (higher number = higher rank)
  switch (type->kind) {
  case TYPE_INT8:
  case TYPE_UINT8:
    return 1;
  case TYPE_INT16:
  case TYPE_UINT16:
    return 2;
  case TYPE_INT32:
  case TYPE_UINT32:
    return 3;
  case TYPE_FLOAT32:
    return 4;
  case TYPE_INT64:
  case TYPE_UINT64:
    return 5;
  case TYPE_FLOAT64:
    return 6;
  case TYPE_STRING:
    return 10; // Special case - strings don't promote with numbers
  default:
    return 0;
  }
}

Type *type_checker_infer_variable_type(TypeChecker *checker,
                                       ASTNode *initializer) {
  if (!checker || !initializer)
    return NULL;

  // Use the general type inference function
  return type_checker_infer_type(checker, initializer);
}

// Type compatibility and conversion functions implementation

int type_checker_is_cast_valid(Type *from, Type *to) {
  if (!from || !to)
    return 0;

  if (type_checker_types_equal(from, to))
    return 1;

  // Numeric ↔ numeric
  if (type_checker_is_numeric_type(from) && type_checker_is_numeric_type(to))
    return 1;

  // Pointer ↔ pointer
  if (from->kind == TYPE_POINTER && to->kind == TYPE_POINTER)
    return 1;

  // Integer ↔ pointer
  if ((type_checker_is_integer_type(from) && to->kind == TYPE_POINTER) ||
      (from->kind == TYPE_POINTER && type_checker_is_integer_type(to))) {
    return 1;
  }

  // Pointer ↔ function pointer
  if ((from->kind == TYPE_POINTER && to->kind == TYPE_FUNCTION_POINTER) ||
      (from->kind == TYPE_FUNCTION_POINTER && to->kind == TYPE_POINTER)) {
    return 1;
  }

  // Integer ↔ function pointer
  if ((type_checker_is_integer_type(from) &&
       to->kind == TYPE_FUNCTION_POINTER) ||
      (from->kind == TYPE_FUNCTION_POINTER &&
       type_checker_is_integer_type(to))) {
    return 1;
  }

  // Function pointer ↔ function pointer
  if (from->kind == TYPE_FUNCTION_POINTER &&
      to->kind == TYPE_FUNCTION_POINTER) {
    return 1;
  }

  return 0;
}

// Type compatibility and conversion functions implementation

int type_checker_is_assignable(TypeChecker *checker, Type *dest_type,
                               Type *src_type) {
  if (!checker || !dest_type || !src_type)
    return 0;

  if (type_checker_types_equal(dest_type, src_type)) {
    return 1;
  }

  /* Allow int8* (e.g. from &array[0] for int8[]) to cstring (uint8*) for C interop */
  if (dest_type->kind == TYPE_POINTER && src_type->kind == TYPE_POINTER &&
      dest_type->name && strcmp(dest_type->name, "cstring") == 0 &&
      src_type->base_type && src_type->base_type->name &&
      strcmp(src_type->base_type->name, "int8") == 0) {
    return 1;
  }

  /* Allow array to pointer decay (T[N] to T*) for function arguments */
  if (dest_type->kind == TYPE_POINTER && src_type->kind == TYPE_ARRAY &&
      dest_type->base_type && src_type->base_type &&
      type_checker_types_equal(dest_type->base_type, src_type->base_type)) {
    return 1;
  }

  if (dest_type->kind == TYPE_POINTER || src_type->kind == TYPE_POINTER ||
      dest_type->kind == TYPE_ARRAY || src_type->kind == TYPE_ARRAY ||
      dest_type->kind == TYPE_STRUCT || src_type->kind == TYPE_STRUCT) {
    return 0;
  }

  // Check for safe implicit conversions
  return type_checker_is_implicitly_convertible(src_type, dest_type);
}

int type_checker_is_implicitly_convertible(Type *from_type, Type *to_type) {
  if (!from_type || !to_type)
    return 0;

  // Same type is always convertible
  if (from_type->kind == to_type->kind) {
    return type_checker_types_equal(from_type, to_type);
  }

  // Integer to integer conversions, including narrowing.
  if (type_checker_is_integer_type(from_type) &&
      type_checker_is_integer_type(to_type)) {
    return 1;
  }

  // Integer to floating point conversions
  if (type_checker_is_integer_type(from_type) &&
      type_checker_is_floating_type(to_type)) {
    return 1; // Generally safe
  }

  // Floating point to floating point conversions, including narrowing.
  if (type_checker_is_floating_type(from_type) &&
      type_checker_is_floating_type(to_type)) {
    return 1;
  }

  // No other implicit conversions are allowed
  return 0;
}

int type_checker_validate_function_call(TypeChecker *checker,
                                        CallExpression *call,
                                        Symbol *func_symbol) {
  if (!checker || !call || !func_symbol)
    return 0;

  // Check argument count
  if (call->argument_count != func_symbol->data.function.parameter_count) {
    type_checker_set_error(
        checker, "Function '%s' expects %llu arguments, got %llu",
        call->function_name,
        (unsigned long long)func_symbol->data.function.parameter_count,
        (unsigned long long)call->argument_count);
    return 0;
  }

  // Check each argument type
  for (size_t i = 0; i < call->argument_count; i++) {
    Type *arg_type = type_checker_infer_type(checker, call->arguments[i]);
    if (!arg_type) {
      type_checker_set_error(
          checker, "Cannot infer type for argument %zu in call to '%s'", i + 1,
          call->function_name);
      return 0;
    }

    Type *param_type = func_symbol->data.function.parameter_types[i];
    int is_null_pointer_arg =
        (param_type && param_type->kind == TYPE_POINTER &&
         type_checker_is_null_pointer_constant(call->arguments[i]));
    // Allow implicit string literal -> cstring coercion.
    int is_string_to_cstring = (param_type && param_type->name &&
                                strcmp(param_type->name, "cstring") == 0 &&
                                call->arguments[i]->type == AST_STRING_LITERAL);
    if (!is_null_pointer_arg && !is_string_to_cstring &&
        !type_checker_is_assignable(checker, param_type, arg_type)) {
      type_checker_set_error(
          checker,
          "Argument %zu in call to '%s': cannot convert from '%s' to '%s'",
          i + 1, call->function_name,
          arg_type->name ? arg_type->name : "unknown",
          param_type->name ? param_type->name : "unknown");
      return 0;
    }
  }

  return 1;
}

int type_checker_validate_assignment(TypeChecker *checker, Type *dest_type,
                                     ASTNode *src_expr) {
  if (!checker || !dest_type || !src_expr)
    return 0;

  Type *src_type = type_checker_infer_type(checker, src_expr);
  if (!src_type) {
    if (!checker->has_error) {
      type_checker_set_error(checker, "Cannot infer type of assignment value");
    }
    return 0;
  }

  if (dest_type->kind == TYPE_POINTER &&
      type_checker_is_null_pointer_constant(src_expr)) {
    return 1;
  }

  if (!type_checker_is_assignable(checker, dest_type, src_type)) {
    type_checker_set_error(
        checker, "Cannot assign value of type '%s' to variable of type '%s'",
        src_type->name ? src_type->name : "unknown",
        dest_type->name ? dest_type->name : "unknown");
    return 0;
  }

  return 1;
}

void type_checker_set_error(TypeChecker *checker, const char *format, ...) {
  if (!checker || !format)
    return;

  // Free previous error message
  free(checker->error_message);

  // Calculate required buffer size
  va_list args1, args2;
  va_start(args1, format);
  va_copy(args2, args1);

  int size = vsnprintf(NULL, 0, format, args1);
  va_end(args1);

  if (size < 0) {
    checker->error_message = NULL;
    checker->has_error = 1;
    va_end(args2);
    return;
  }

  // Allocate and format the message
  checker->error_message = malloc(size + 1);
  if (checker->error_message) {
    vsnprintf(checker->error_message, size + 1, format, args2);
  }

  va_end(args2);
  checker->has_error = 1;
}

// Struct type processing functions

int type_checker_process_struct_declaration(TypeChecker *checker,
                                            ASTNode *struct_decl) {
  if (!checker || !struct_decl || struct_decl->type != AST_STRUCT_DECLARATION) {
    return 0;
  }

  StructDeclaration *decl = (StructDeclaration *)struct_decl->data;
  if (!decl || !decl->name) {
    return 0;
  }

  // Check if struct already exists
  Symbol *existing =
      symbol_table_lookup_current_scope(checker->symbol_table, decl->name);
  if (existing) {
    type_checker_report_duplicate_declaration(checker, struct_decl->location,
                                              decl->name);
    return 0;
  }

  // Resolve field types
  Type **field_types = malloc(decl->field_count * sizeof(Type *));
  if (!field_types)
    return 0;

  for (size_t i = 0; i < decl->field_count; i++) {
    field_types[i] =
        type_checker_get_type_by_name(checker, decl->field_types[i]);
    if (!field_types[i]) {
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg),
               "Unknown type '%s' in struct field", decl->field_types[i]);
      type_checker_set_error_at_location(checker, struct_decl->location,
                                         error_msg);
      free(field_types);
      return 0;
    }
  }

  // Create struct type
  Type *struct_type = type_create_struct(decl->name, decl->field_names,
                                         field_types, decl->field_count);
  if (!struct_type) {
    free(field_types);
    return 0;
  }

  // Create and register struct symbol
  Symbol *struct_symbol = symbol_create(decl->name, SYMBOL_STRUCT, struct_type);
  if (!struct_symbol) {
    type_destroy(struct_type);
    free(field_types);
    return 0;
  }

  if (!symbol_table_declare(checker->symbol_table, struct_symbol)) {
    symbol_destroy(struct_symbol);
    free(field_types);
    return 0;
  }

  free(field_types);
  return 1;
}

int type_checker_process_enum_declaration(TypeChecker *checker,
                                          ASTNode *enum_decl_node) {
  if (!checker || !enum_decl_node ||
      enum_decl_node->type != AST_ENUM_DECLARATION) {
    return 0;
  }

  EnumDeclaration *enum_decl = (EnumDeclaration *)enum_decl_node->data;
  if (!enum_decl || !enum_decl->name) {
    type_checker_set_error_at_location(checker, enum_decl_node->location,
                                       "Invalid enum declaration");
    return 0;
  }

  // Check for duplicate type declaration
  if (type_checker_get_type_by_name(checker, enum_decl->name)) {
    type_checker_set_error_at_location(checker, enum_decl_node->location,
                                       "Type '%s' already declared",
                                       enum_decl->name);
    return 0;
  }

  // Create enum type (aliased to INT64 for simplicity in MethASM)
  Type *new_enum_type = type_create(TYPE_ENUM, enum_decl->name);
  if (!new_enum_type) {
    type_checker_set_error_at_location(checker, enum_decl_node->location,
                                       "Failed to create enum type");
    return 0;
  }

  // Create and register the enum type symbol
  Symbol *enum_symbol =
      symbol_create(enum_decl->name, SYMBOL_ENUM, new_enum_type);
  if (!enum_symbol) {
    type_destroy(new_enum_type);
    return 0;
  }

  if (!symbol_table_declare(checker->symbol_table, enum_symbol)) {
    symbol_destroy(enum_symbol);
    return 0;
  }

  long long current_val = 0;

  for (size_t i = 0; i < enum_decl->variant_count; i++) {
    EnumVariant *variant = &enum_decl->variants[i];

    if (variant->value) {
      ASTNode *val_node = variant->value;
      if (val_node->type == AST_NUMBER_LITERAL) {
        current_val = ((NumberLiteral *)val_node->data)->int_value;
      } else if (val_node->type == AST_UNARY_EXPRESSION &&
                 ((UnaryExpression *)val_node->data)->operand->type ==
                     AST_NUMBER_LITERAL &&
                 strcmp(((UnaryExpression *)val_node->data)->operator, "-") ==
                     0) {
        current_val = -((NumberLiteral *)((UnaryExpression *)val_node->data)
                            ->operand->data)
                           ->int_value;
      } else {
        type_checker_set_error_at_location(
            checker, val_node->location,
            "Enum variant initializer must be a constant integer");
        return 0;
      }
    }

    // Check if variant name is already taken
    if (symbol_table_lookup_current_scope(checker->symbol_table,
                                          variant->name)) {
      type_checker_report_duplicate_declaration(
          checker, enum_decl_node->location, variant->name);
      return 0;
    }

    Symbol *sym = symbol_create(variant->name, SYMBOL_CONSTANT, new_enum_type);
    if (!sym) {
      return 0;
    }
    sym->data.constant.value = current_val;
    sym->is_initialized = 1;
    symbol_table_insert(checker->symbol_table, sym);

    current_val++;
  }

  return 1;
}

int type_checker_process_declaration(TypeChecker *checker,
                                     ASTNode *declaration) {
  if (!checker || !declaration) {
    return 0;
  }

  switch (declaration->type) {
  case AST_DEFER_STATEMENT:
    type_checker_set_error_at_location(checker, declaration->location,
                                       "Defer statement outside of a function");
    return 0;

  case AST_ERRDEFER_STATEMENT:
    type_checker_set_error_at_location(
        checker, declaration->location,
        "Errdefer statement outside of a function");
    return 0;
  case AST_VAR_DECLARATION: {
    VarDeclaration *var_decl = (VarDeclaration *)declaration->data;
    if (!var_decl || !var_decl->name) {
      type_checker_set_error_at_location(checker, declaration->location,
                                         "Invalid variable declaration");
      return 0;
    }

    if (var_decl->link_name && !var_decl->is_extern) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Link-name suffix is only allowed on extern declarations");
      return 0;
    }

    Scope *current_scope =
        symbol_table_get_current_scope(checker->symbol_table);
    if (var_decl->is_extern &&
        (!current_scope || current_scope->type != SCOPE_GLOBAL)) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Extern declarations are only allowed at top level");
      return 0;
    }

    if (var_decl->is_extern && var_decl->initializer) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Extern variable '%s' cannot have an initializer", var_decl->name);
      return 0;
    }
    if (var_decl->is_extern && !var_decl->type_name) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Extern variable '%s' requires an explicit type annotation",
          var_decl->name);
      return 0;
    }

    Type *var_type = NULL;

    // If type is explicitly specified, resolve it
    if (var_decl->type_name) {
      var_type = type_checker_get_type_by_name(checker, var_decl->type_name);
      if (!var_type) {
        type_checker_report_undefined_symbol(checker, declaration->location,
                                             var_decl->type_name, "type");
        return 0;
      }
    }

    // If there's an initializer, validate it
    if (var_decl->initializer) {
      Type *init_type = type_checker_infer_type(checker, var_decl->initializer);
      if (!init_type) {
        if (!checker->has_error) {
          type_checker_set_error_at_location(
              checker, var_decl->initializer->location,
              "Cannot infer type of initializer for variable '%s'",
              var_decl->name);
        }
        return 0;
      }
      if (var_type) {
        // Type specified: validate assignment compatibility
        if (!(var_type->kind == TYPE_POINTER &&
              type_checker_is_null_pointer_constant(var_decl->initializer)) &&
            !type_checker_is_assignable(checker, var_type, init_type)) {
          type_checker_report_type_mismatch(checker,
                                            var_decl->initializer->location,
                                            var_type->name, init_type->name);
          return 0;
        }
      } else {
        // Type inference: use initializer type
        var_type = init_type;
      }
    } else if (!var_type) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Variable '%s' must have either a type annotation or an initializer",
          var_decl->name);
      return 0;
    }

    // Check for duplicate declaration in current scope.
    Symbol *existing = symbol_table_lookup_current_scope(checker->symbol_table,
                                                         var_decl->name);
    if (existing) {
      if (existing->kind != SYMBOL_VARIABLE) {
        type_checker_report_duplicate_declaration(
            checker, declaration->location, var_decl->name);
        return 0;
      }
      if (existing->is_extern != var_decl->is_extern) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Variable '%s' redeclared with conflicting extern/non-extern "
            "linkage",
            var_decl->name);
        return 0;
      }
      if (!var_decl->is_extern) {
        type_checker_report_duplicate_declaration(
            checker, declaration->location, var_decl->name);
        return 0;
      }
      if (!type_checker_types_equal(existing->type, var_type)) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Extern variable '%s' redeclared with conflicting type",
            var_decl->name);
        return 0;
      }
      if (!type_checker_link_name_matches_symbol(existing, var_decl->name,
                                                 var_decl->is_extern,
                                                 var_decl->link_name)) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Extern variable '%s' redeclared with conflicting link name",
            var_decl->name);
        return 0;
      }
      return 1;
    }

    // Create and declare the symbol
    Symbol *var_symbol =
        symbol_create(var_decl->name, SYMBOL_VARIABLE, var_type);
    if (!var_symbol) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Failed to create symbol for variable '%s'", var_decl->name);
      return 0;
    }

    var_symbol->is_extern = var_decl->is_extern;
    if (var_decl->is_extern) {
      const char *effective_link_name = type_checker_decl_link_name(
          var_decl->name, var_decl->is_extern, var_decl->link_name);
      var_symbol->link_name =
          effective_link_name ? strdup(effective_link_name) : NULL;
      if (!var_symbol->link_name) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Failed to allocate link name for extern variable '%s'",
            var_decl->name);
        symbol_destroy(var_symbol);
        return 0;
      }
    }

    if (!symbol_table_declare(checker->symbol_table, var_symbol)) {
      type_checker_report_duplicate_declaration(checker, declaration->location,
                                                var_decl->name);
      symbol_destroy(var_symbol);
      return 0;
    }

    if (checker->current_function && !var_decl->is_extern) {
      Scope *declare_scope =
          symbol_table_get_current_scope(checker->symbol_table);
      if (declare_scope && declare_scope->type != SCOPE_GLOBAL) {
        int track_definite_init =
            !(var_type &&
              (var_type->kind == TYPE_ARRAY || var_type->kind == TYPE_STRUCT ||
               var_type->kind == TYPE_STRING));
        if (track_definite_init) {
          if (!type_checker_init_tracker_declare(
                  checker, var_decl->name, var_decl->initializer != NULL)) {
            type_checker_set_error_at_location(
                checker, declaration->location,
                "Out of memory while tracking initialization state for '%s'",
                var_decl->name);
            return 0;
          }
        }
      }
    }

    return 1;
  }

  case AST_FUNCTION_DECLARATION: {
    FunctionDeclaration *func_decl = (FunctionDeclaration *)declaration->data;
    if (!func_decl || !func_decl->name) {
      type_checker_set_error_at_location(checker, declaration->location,
                                         "Invalid function declaration");
      return 0;
    }

    if (func_decl->link_name && !func_decl->is_extern) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Link-name suffix is only allowed on extern declarations");
      return 0;
    }

    Scope *current_scope =
        symbol_table_get_current_scope(checker->symbol_table);
    if (func_decl->is_extern &&
        (!current_scope || current_scope->type != SCOPE_GLOBAL)) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Extern declarations are only allowed at top level");
      return 0;
    }
    if (func_decl->is_extern && func_decl->body) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Extern function '%s' must not have a body", func_decl->name);
      return 0;
    }

    // Resolve return type
    Type *return_type = NULL;
    if (func_decl->return_type) {
      return_type =
          type_checker_get_type_by_name(checker, func_decl->return_type);
      if (!return_type) {
        type_checker_report_undefined_symbol(checker, declaration->location,
                                             func_decl->return_type, "type");
        return 0;
      }
    } else {
      return_type = checker->builtin_void;
    }

    // Resolve parameter types and check for duplicate parameter names
    Type **param_types = NULL;
    if (func_decl->parameter_count > 0) {
      param_types = malloc(func_decl->parameter_count * sizeof(Type *));
      if (!param_types) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Memory allocation failed for function parameters");
        return 0;
      }

      // Check for duplicate parameter names
      for (size_t i = 0; i < func_decl->parameter_count; i++) {
        for (size_t j = i + 1; j < func_decl->parameter_count; j++) {
          if (strcmp(func_decl->parameter_names[i],
                     func_decl->parameter_names[j]) == 0) {
            type_checker_report_duplicate_declaration(
                checker, declaration->location, func_decl->parameter_names[i]);
            free(param_types);
            return 0;
          }
        }
      }

      for (size_t i = 0; i < func_decl->parameter_count; i++) {
        param_types[i] = type_checker_get_type_by_name(
            checker, func_decl->parameter_types[i]);
        if (!param_types[i]) {
          type_checker_report_undefined_symbol(checker, declaration->location,
                                               func_decl->parameter_types[i],
                                               "type");
          free(param_types);
          return 0;
        }
      }
    }

    // Copy parameter names so function symbols own their metadata.
    char **param_names_copy = NULL;
    if (func_decl->parameter_count > 0) {
      param_names_copy = malloc(func_decl->parameter_count * sizeof(char *));
      if (!param_names_copy) {
        if (param_types)
          free(param_types);
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Memory allocation failed for function parameter names");
        return 0;
      }
      for (size_t i = 0; i < func_decl->parameter_count; i++) {
        param_names_copy[i] = strdup(func_decl->parameter_names[i]);
        if (!param_names_copy[i]) {
          for (size_t j = 0; j < i; j++) {
            free(param_names_copy[j]);
          }
          free(param_names_copy);
          if (param_types)
            free(param_types);
          type_checker_set_error_at_location(
              checker, declaration->location,
              "Memory allocation failed for parameter name copy");
          return 0;
        }
      }
    }

    // Create function symbol
    Symbol *func_symbol =
        symbol_create(func_decl->name, SYMBOL_FUNCTION, return_type);
    if (!func_symbol) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Memory allocation failed for function symbol");
      if (param_names_copy) {
        for (size_t i = 0; i < func_decl->parameter_count; i++) {
          free(param_names_copy[i]);
        }
        free(param_names_copy);
      }
      if (param_types)
        free(param_types);
      return 0;
    }

    // Set function-specific data
    func_symbol->data.function.parameter_count = func_decl->parameter_count;
    func_symbol->data.function.parameter_names = param_names_copy;
    func_symbol->data.function.parameter_types = param_types;
    func_symbol->data.function.return_type = return_type;
    func_symbol->is_extern = func_decl->is_extern;
    if (func_decl->is_extern) {
      const char *effective_link_name = type_checker_decl_link_name(
          func_decl->name, func_decl->is_extern, func_decl->link_name);
      func_symbol->link_name =
          effective_link_name ? strdup(effective_link_name) : NULL;
      if (!func_symbol->link_name) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Failed to allocate link name for extern function '%s'",
            func_decl->name);
        symbol_destroy(func_symbol);
        return 0;
      }
    }

    Symbol *existing_before = symbol_table_lookup_current_scope(
        checker->symbol_table, func_decl->name);
    int is_resolving_forward =
        (existing_before && existing_before->kind == SYMBOL_FUNCTION &&
         existing_before->is_forward_declaration);

    if (existing_before && existing_before->kind != SYMBOL_FUNCTION) {
      type_checker_report_duplicate_declaration(checker, declaration->location,
                                                func_decl->name);
      symbol_destroy(func_symbol);
      return 0;
    }

    if (existing_before && existing_before->kind == SYMBOL_FUNCTION) {
      if (existing_before->is_extern != func_decl->is_extern) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Function '%s' redeclared with conflicting extern/non-extern "
            "linkage",
            func_decl->name);
        symbol_destroy(func_symbol);
        return 0;
      }
      if ((existing_before->is_extern || func_decl->is_extern) &&
          !type_checker_link_name_matches_symbol(
              existing_before, func_decl->name, func_decl->is_extern,
              func_decl->link_name)) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Function '%s' redeclared with conflicting link name",
            func_decl->name);
        symbol_destroy(func_symbol);
        return 0;
      }
    }

    // Forward declaration: no body
    if (!func_decl->body) {
      func_symbol->is_initialized = 0;
      if (!symbol_table_declare_forward(checker->symbol_table, func_symbol)) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Invalid or conflicting forward declaration for function '%s'",
            func_decl->name);
        symbol_destroy(func_symbol);
        return 0;
      }
      if (is_resolving_forward) {
        symbol_destroy(func_symbol);
      }
      return 1;
    }

    func_symbol->is_initialized = 1;
    if (!symbol_table_resolve_forward_declaration(checker->symbol_table,
                                                  func_symbol)) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Function definition for '%s' does not match existing declaration",
          func_decl->name);
      symbol_destroy(func_symbol);
      return 0;
    }

    if (is_resolving_forward) {
      checker->current_function = existing_before;
      symbol_destroy(func_symbol); // not inserted, existing symbol was updated
      func_symbol = existing_before;
    } else {
      checker->current_function = func_symbol;
    }
    checker->current_function_decl = declaration;

    // Enter a new scope for the function body
    symbol_table_enter_scope(checker->symbol_table, SCOPE_FUNCTION);
    type_checker_init_tracker_reset(checker);
    if (!type_checker_init_tracker_enter_scope(checker)) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Out of memory while initializing flow analysis scope");
      symbol_table_exit_scope(checker->symbol_table);
      return 0;
    }

    // Add parameters to the new scope
    Type **active_param_types =
        checker->current_function->data.function.parameter_types;
    if (func_decl->parameter_count > 0) {
      for (size_t i = 0; i < func_decl->parameter_count; i++) {
        Symbol *param_symbol =
            symbol_create(func_decl->parameter_names[i], SYMBOL_VARIABLE,
                          active_param_types ? active_param_types[i] : NULL);
        if (!param_symbol) {
          type_checker_set_error_at_location(
              checker, declaration->location,
              "Failed to create parameter symbol");
          symbol_table_exit_scope(checker->symbol_table);
          return 0;
        }
        if (!symbol_table_declare(checker->symbol_table, param_symbol)) {
          type_checker_report_duplicate_declaration(
              checker, declaration->location, func_decl->parameter_names[i]);
          symbol_destroy(param_symbol);
          type_checker_init_tracker_reset(checker);
          symbol_table_exit_scope(checker->symbol_table);
          return 0;
        }
        if (!type_checker_init_tracker_declare(
                checker, func_decl->parameter_names[i], 1)) {
          type_checker_set_error_at_location(
              checker, declaration->location,
              "Out of memory while tracking parameter initialization");
          type_checker_init_tracker_reset(checker);
          symbol_table_exit_scope(checker->symbol_table);
          return 0;
        }
      }
    }

    // Process the function body
    if (func_decl->body &&
        !type_checker_check_statement(checker, func_decl->body)) {
      // Error already reported
      type_checker_init_tracker_reset(checker);
      symbol_table_exit_scope(checker->symbol_table);
      return 0;
    }

    type_checker_init_tracker_exit_scope(checker);
    type_checker_init_tracker_reset(checker);

    // Exit the function's scope
    symbol_table_exit_scope(checker->symbol_table);

    // Reset the current function in the type checker
    checker->current_function = NULL;
    checker->current_function_decl = NULL;

    return 1;
  }

  case AST_METHOD_DECLARATION:
    // Method declarations are handled within struct processing
    // This case shouldn't normally be reached during standalone processing
    return 1;

  case AST_INLINE_ASM:
    // Top-level inline assembly is permitted.
    return 1;

  case AST_ASSIGNMENT: {
    Assignment *assignment = (Assignment *)declaration->data;
    if (!assignment || !assignment->value) {
      type_checker_set_error_at_location(checker, declaration->location,
                                         "Invalid assignment statement");
      return 0;
    }

    // Complex assignment target: obj.field = value or arr[i] = value
    if (assignment->target) {
      if (assignment->target->type == AST_MEMBER_ACCESS) {
        MemberAccess *member = (MemberAccess *)assignment->target->data;
        if (!member || !member->object || !member->member) {
          type_checker_set_error_at_location(checker,
                                             assignment->target->location,
                                             "Invalid field assignment target");
          return 0;
        }

        Type *object_type = type_checker_infer_type(checker, member->object);
        if (!object_type) {
          return 0;
        }

        if (object_type->kind != TYPE_STRUCT) {
          char error_msg[512];
          snprintf(error_msg, sizeof(error_msg),
                   "Cannot assign field '%s' on non-struct type '%s'",
                   member->member, object_type->name);
          type_checker_set_error_at_location(
              checker, assignment->target->location, error_msg);
          return 0;
        }

        Type *field_type = type_get_field_type(object_type, member->member);
        if (!field_type) {
          char error_msg[512];
          snprintf(error_msg, sizeof(error_msg),
                   "Field '%s' not found in struct '%s'", member->member,
                   object_type->name);
          type_checker_set_error_at_location(
              checker, assignment->target->location, error_msg);
          return 0;
        }

        Type *value_type = type_checker_infer_type(checker, assignment->value);
        if (!value_type) {
          if (!checker->has_error) {
            type_checker_set_error_at_location(
                checker, assignment->value->location,
                "Cannot infer type of assignment value");
          }
          return 0;
        }

        if (!(field_type->kind == TYPE_POINTER &&
              type_checker_is_null_pointer_constant(assignment->value)) &&
            !type_checker_is_assignable(checker, field_type, value_type)) {
          type_checker_report_type_mismatch(checker,
                                            assignment->value->location,
                                            field_type->name, value_type->name);
          return 0;
        }

        return 1;
      } else if (assignment->target->type == AST_INDEX_EXPRESSION) {
        ArrayIndexExpression *target_index =
            (ArrayIndexExpression *)assignment->target->data;
        if (!target_index || !target_index->array || !target_index->index) {
          type_checker_set_error_at_location(checker,
                                             assignment->target->location,
                                             "Invalid array assignment target");
          return 0;
        }

        Type *target_array_type =
            type_checker_infer_type(checker, target_index->array);
        if (!target_array_type) {
          return 0;
        }
        if (target_array_type->kind == TYPE_ARRAY) {
          long long constant_index = 0;
          if (type_checker_eval_integer_constant(target_index->index,
                                                 &constant_index)) {
            if (constant_index < 0 ||
                (unsigned long long)constant_index >=
                    (unsigned long long)target_array_type->array_size) {
              type_checker_set_error_at_location(
                  checker, target_index->index->location,
                  "Array index %lld is out of bounds for '%s' (size %zu)",
                  constant_index,
                  target_array_type->name ? target_array_type->name : "array",
                  target_array_type->array_size);
              return 0;
            }
          }
        }

        Type *element_type =
            type_checker_infer_type(checker, assignment->target);
        if (!element_type) {
          return 0;
        }

        Type *value_type = type_checker_infer_type(checker, assignment->value);
        if (!value_type) {
          if (!checker->has_error) {
            type_checker_set_error_at_location(
                checker, assignment->value->location,
                "Cannot infer type of assignment value");
          }
          return 0;
        }

        if (!(element_type->kind == TYPE_POINTER &&
              type_checker_is_null_pointer_constant(assignment->value)) &&
            !type_checker_is_assignable(checker, element_type, value_type)) {
          type_checker_report_type_mismatch(
              checker, assignment->value->location, element_type->name,
              value_type->name);
          return 0;
        }

        return 1;
      } else if (assignment->target->type == AST_UNARY_EXPRESSION) {
        UnaryExpression *target_unary =
            (UnaryExpression *)assignment->target->data;
        if (!target_unary || !target_unary->operator ||
            strcmp(target_unary->operator, "*") != 0) {
          type_checker_set_error_at_location(checker,
                                             assignment->target->location,
                                             "Invalid assignment target");
          return 0;
        }

        Type *target_type =
            type_checker_infer_type(checker, assignment->target);
        if (!target_type) {
          return 0;
        }

        Type *value_type = type_checker_infer_type(checker, assignment->value);
        if (!value_type) {
          if (!checker->has_error) {
            type_checker_set_error_at_location(
                checker, assignment->value->location,
                "Cannot infer type of assignment value");
          }
          return 0;
        }

        if (!(target_type->kind == TYPE_POINTER &&
              type_checker_is_null_pointer_constant(assignment->value)) &&
            !type_checker_is_assignable(checker, target_type, value_type)) {
          type_checker_report_type_mismatch(
              checker, assignment->value->location, target_type->name,
              value_type->name);
          return 0;
        }

        return 1;
      }

      type_checker_set_error_at_location(checker, assignment->target->location,
                                         "Invalid assignment target");
      return 0;
    }

    // Simple variable assignment: name = value
    if (!assignment->variable_name) {
      type_checker_set_error_at_location(checker, declaration->location,
                                         "Invalid assignment statement");
      return 0;
    }

    // Look up the variable
    Symbol *var_symbol =
        symbol_table_lookup(checker->symbol_table, assignment->variable_name);
    if (!var_symbol) {
      type_checker_report_undefined_symbol(checker, declaration->location,
                                           assignment->variable_name,
                                           "variable");
      return 0;
    }

    if (var_symbol->kind != SYMBOL_VARIABLE &&
        var_symbol->kind != SYMBOL_PARAMETER) {
      char error_msg[512];
      const char *symbol_type =
          (var_symbol->kind == SYMBOL_FUNCTION) ? "function"
          : (var_symbol->kind == SYMBOL_STRUCT) ? "struct"
                                                : "symbol";
      snprintf(error_msg, sizeof(error_msg),
               "'%s' is a %s and cannot be assigned to",
               assignment->variable_name, symbol_type);
      type_checker_set_error_at_location(checker, declaration->location,
                                         error_msg);
      return 0;
    }

    // Infer the type of the assignment value
    Type *value_type = type_checker_infer_type(checker, assignment->value);
    if (!value_type) {
      if (!checker->has_error) {
        type_checker_set_error_at_location(
            checker, assignment->value->location,
            "Cannot infer type of assignment value");
      }
      return 0;
    }

    if (var_symbol->is_extern &&
        type_checker_is_gc_managed_pointer_type(value_type) &&
        checker->error_reporter) {
      error_reporter_add_warning(
          checker->error_reporter, ERROR_SEMANTIC, assignment->value->location,
          "Managed pointer stored in extern variable may escape GC visibility; "
          "register the C-held slot with gc_register_root");
    }

    // Validate assignment compatibility
    if (!(var_symbol->type->kind == TYPE_POINTER &&
          type_checker_is_null_pointer_constant(assignment->value)) &&
        !type_checker_is_assignable(checker, var_symbol->type, value_type)) {
      type_checker_report_type_mismatch(checker, assignment->value->location,
                                        var_symbol->type->name,
                                        value_type->name);
      return 0;
    }

    if (checker->current_function && var_symbol->scope &&
        var_symbol->scope->type != SCOPE_GLOBAL) {
      type_checker_init_tracker_set_initialized(checker,
                                                assignment->variable_name);
    }

    return 1;
  }

  default:
    type_checker_set_error_at_location(
        checker, declaration->location,
        "Unsupported top-level construct in declaration context");
    return 0;
  }
}

// Enhanced error reporting functions

void type_checker_set_error_at_location(TypeChecker *checker,
                                        SourceLocation location,
                                        const char *format, ...) {
  if (!checker || !format)
    return;

  checker->has_error = 1;
  free(checker->error_message);

  va_list args;
  va_start(args, format);

  // Calculate required buffer size
  va_list args_copy;
  va_copy(args_copy, args);
  int size = vsnprintf(NULL, 0, format, args_copy);
  va_end(args_copy);

  if (size > 0) {
    checker->error_message = malloc(size + 1);
    if (checker->error_message) {
      vsnprintf(checker->error_message, size + 1, format, args);
    }
  }

  // If we have an error reporter, add the error to it
  if (checker->error_reporter) {
    char *message = checker->error_message;
    SourceSpan span = source_span_from_location(location, 1);
    error_reporter_add_error_with_span(checker->error_reporter, ERROR_SEMANTIC,
                                       span, message);
  }

  va_end(args);
}

void type_checker_report_type_mismatch(TypeChecker *checker,
                                       SourceLocation location,
                                       const char *expected,
                                       const char *actual) {
  if (!checker || !expected || !actual)
    return;

  char error_msg[512];
  snprintf(error_msg, sizeof(error_msg),
           "Type mismatch: expected '%s', found '%s'", expected, actual);

  checker->has_error = 1;
  free(checker->error_message);
  checker->error_message = strdup(error_msg);

  if (checker->error_reporter) {
    const char *suggestion =
        error_reporter_suggest_for_type_mismatch(expected, actual);
    SourceSpan span = source_span_from_location(location, 1);
    if (suggestion) {
      error_reporter_add_error_with_span_and_suggestion(
          checker->error_reporter, ERROR_TYPE, span, error_msg, suggestion);
    } else {
      error_reporter_add_error_with_span(checker->error_reporter, ERROR_TYPE,
                                         span, error_msg);
    }
  }
}

void type_checker_report_undefined_symbol(TypeChecker *checker,
                                          SourceLocation location,
                                          const char *symbol_name,
                                          const char *symbol_type) {
  if (!checker || !symbol_name || !symbol_type)
    return;

  char error_msg[512];
  snprintf(error_msg, sizeof(error_msg), "Undefined %s '%s'", symbol_type,
           symbol_name);

  checker->has_error = 1;
  free(checker->error_message);
  checker->error_message = strdup(error_msg);

  if (checker->error_reporter) {
    char suggestion[256];
    snprintf(suggestion, sizeof(suggestion), "declare '%s' before using it",
             symbol_name);
    SourceSpan span = source_span_from_location(location, strlen(symbol_name));
    error_reporter_add_error_with_span_and_suggestion(
        checker->error_reporter, ERROR_SEMANTIC, span, error_msg, suggestion);
  }
}

void type_checker_report_duplicate_declaration(TypeChecker *checker,
                                               SourceLocation location,
                                               const char *symbol_name) {
  if (!checker || !symbol_name)
    return;

  char error_msg[512];
  snprintf(error_msg, sizeof(error_msg), "Duplicate declaration of '%s'",
           symbol_name);

  checker->has_error = 1;
  free(checker->error_message);
  checker->error_message = strdup(error_msg);

  if (checker->error_reporter) {
    char suggestion[256];
    snprintf(suggestion, sizeof(suggestion),
             "use a different name or remove the duplicate declaration");
    SourceSpan span = source_span_from_location(location, strlen(symbol_name));
    error_reporter_add_error_with_span_and_suggestion(
        checker->error_reporter, ERROR_SEMANTIC, span, error_msg, suggestion);
  }
}

void type_checker_report_scope_violation(TypeChecker *checker,
                                         SourceLocation location,
                                         const char *symbol_name,
                                         const char *violation_type) {
  if (!checker || !symbol_name || !violation_type)
    return;

  char error_msg[512];
  snprintf(error_msg, sizeof(error_msg), "Scope violation: %s '%s'",
           violation_type, symbol_name);

  checker->has_error = 1;
  free(checker->error_message);
  checker->error_message = strdup(error_msg);

  if (checker->error_reporter) {
    char suggestion[256];
    if (strcmp(violation_type,
               "cannot access local variable from outer scope") == 0) {
      snprintf(suggestion, sizeof(suggestion),
               "declare '%s' in the current scope or pass it as a parameter",
               symbol_name);
    } else if (strcmp(violation_type, "cannot redeclare parameter") == 0) {
      snprintf(suggestion, sizeof(suggestion), "use a different variable name");
    } else {
      snprintf(suggestion, sizeof(suggestion), "check the scope rules for '%s'",
               symbol_name);
    }
    SourceSpan span = source_span_from_location(location, strlen(symbol_name));
    error_reporter_add_error_with_span_and_suggestion(
        checker->error_reporter, ERROR_SCOPE, span, error_msg, suggestion);
  }
}

// Validation functions for semantic analysis

// Statement and expression validation functions

int type_checker_check_statement(TypeChecker *checker, ASTNode *statement) {
  if (!checker || !statement)
    return 0;

  switch (statement->type) {
  case AST_DEFER_STATEMENT: {
    if (!checker->current_function) {
      type_checker_set_error_at_location(
          checker, statement->location,
          "Defer statement outside of a function");
      return 0;
    }

    DeferStatement *defer_stmt = (DeferStatement *)statement->data;
    if (!defer_stmt || !defer_stmt->statement) {
      type_checker_set_error_at_location(checker, statement->location,
                                         "Invalid defer statement");
      return 0;
    }

    switch (defer_stmt->statement->type) {
    case AST_FUNCTION_CALL:
    case AST_ASSIGNMENT:
    case AST_PROGRAM:
      break;
    default:
      type_checker_set_error_at_location(
          checker, defer_stmt->statement->location,
          "Deferred statement must be a function call, assignment, or block");
      return 0;
    }

    return type_checker_check_statement(checker, defer_stmt->statement);
  }

  case AST_ERRDEFER_STATEMENT: {
    if (!checker->current_function) {
      type_checker_set_error_at_location(
          checker, statement->location,
          "Errdefer statement outside of a function");
      return 0;
    }

    DeferStatement *defer_stmt = (DeferStatement *)statement->data;
    if (!defer_stmt || !defer_stmt->statement) {
      type_checker_set_error_at_location(checker, statement->location,
                                         "Invalid errdefer statement");
      return 0;
    }

    switch (defer_stmt->statement->type) {
    case AST_FUNCTION_CALL:
    case AST_ASSIGNMENT:
    case AST_PROGRAM:
      break;
    default:
      type_checker_set_error_at_location(checker,
                                         defer_stmt->statement->location,
                                         "Errdeferred statement must be a "
                                         "function call, assignment, or block");
      return 0;
    }

    return type_checker_check_statement(checker, defer_stmt->statement);
  }
  case AST_VAR_DECLARATION:
  case AST_FUNCTION_DECLARATION:
  case AST_STRUCT_DECLARATION:
  case AST_ASSIGNMENT:
    // These are handled by process_declaration
    return type_checker_process_declaration(checker, statement);

  case AST_FUNCTION_CALL: {
    // Function call as statement (no return value used)
    Type *return_type = type_checker_infer_type(checker, statement);
    return return_type != NULL; // Error already reported if NULL
  }

  case AST_RETURN_STATEMENT: {
    ReturnStatement *ret_stmt = (ReturnStatement *)statement->data;
    if (ret_stmt && ret_stmt->value) {
      // Check if return value type matches function return type
      Type *value_type = type_checker_infer_type(checker, ret_stmt->value);
      if (!value_type) {
        // Error already reported by type_checker_infer_type if it failed
        // Only set generic error if no specific error was set
        if (!checker->has_error) {
          type_checker_set_error_at_location(
              checker, ret_stmt->value->location,
              "Cannot infer type of return value");
        }
        return 0;
      }

      if (checker->current_function) {
        Type *func_return_type =
            checker->current_function->data.function.return_type;
        if (!(func_return_type->kind == TYPE_POINTER &&
              type_checker_is_null_pointer_constant(ret_stmt->value)) &&
            !type_checker_is_assignable(checker, func_return_type,
                                        value_type)) {
          type_checker_report_type_mismatch(checker, ret_stmt->value->location,
                                            func_return_type->name,
                                            value_type->name);
          return 0;
        }

        if (checker->current_function_decl &&
            type_checker_ast_contains_node_type(checker->current_function_decl,
                                                AST_ERRDEFER_STATEMENT)) {
          long long constant_value = 0;
          if (type_checker_eval_integer_constant(ret_stmt->value,
                                                 &constant_value) &&
              constant_value != 0) {
            error_reporter_add_warning(
                checker->error_reporter, ERROR_SEMANTIC,
                ret_stmt->value->location,
                "Non-zero constant return in function with errdefer will "
                "trigger errdefer by convention");
          }
        }
      } else {
        type_checker_set_error_at_location(
            checker, statement->location,
            "Return statement outside of a function");
        return 0;
      }
    }
    return 1;
  }

  case AST_IF_STATEMENT: {
    IfStatement *if_stmt = (IfStatement *)statement->data;
    if (!if_stmt || !if_stmt->condition) {
      type_checker_set_error_at_location(checker, statement->location,
                                         "Invalid if statement");
      return 0;
    }

    // Check condition type
    Type *condition_type = type_checker_infer_type(checker, if_stmt->condition);
    if (!condition_type) {
      return 0; // Error already reported
    }

    // Condition should be a numeric type (treated as boolean)
    if (!type_checker_is_numeric_type(condition_type)) {
      type_checker_report_type_mismatch(checker, if_stmt->condition->location,
                                        "numeric type", condition_type->name);
      return 0;
    }

    size_t init_snapshot_count = 0;
    unsigned char *init_snapshot =
        type_checker_init_tracker_capture(checker, &init_snapshot_count);
    if (checker->tracked_var_count > 0 && !init_snapshot) {
      type_checker_set_error_at_location(
          checker, statement->location,
          "Out of memory while analyzing variable initialization flow");
      return 0;
    }

    // Check then branch
    if (if_stmt->then_branch &&
        !type_checker_check_statement(checker, if_stmt->then_branch)) {
      free(init_snapshot);
      return 0;
    }
    type_checker_init_tracker_restore(checker, init_snapshot,
                                      init_snapshot_count);

    for (size_t i = 0; i < if_stmt->else_if_count; i++) {
      Type *elif_cond_type =
          type_checker_infer_type(checker, if_stmt->else_ifs[i].condition);
      if (!elif_cond_type) {
        free(init_snapshot);
        return 0;
      }
      if (!type_checker_is_numeric_type(elif_cond_type)) {
        type_checker_report_type_mismatch(
            checker, if_stmt->else_ifs[i].condition->location, "numeric type",
            elif_cond_type->name);
        free(init_snapshot);
        return 0;
      }
      if (if_stmt->else_ifs[i].body &&
          !type_checker_check_statement(checker, if_stmt->else_ifs[i].body)) {
        free(init_snapshot);
        return 0;
      }
      type_checker_init_tracker_restore(checker, init_snapshot,
                                        init_snapshot_count);
    }

    // Check else branch if present
    if (if_stmt->else_branch &&
        !type_checker_check_statement(checker, if_stmt->else_branch)) {
      free(init_snapshot);
      return 0;
    }
    type_checker_init_tracker_restore(checker, init_snapshot,
                                      init_snapshot_count);
    free(init_snapshot);

    return 1;
  }

  case AST_WHILE_STATEMENT: {
    WhileStatement *while_stmt = (WhileStatement *)statement->data;
    if (!while_stmt || !while_stmt->condition) {
      type_checker_set_error_at_location(checker, statement->location,
                                         "Invalid while statement");
      return 0;
    }

    // Check condition type
    Type *condition_type =
        type_checker_infer_type(checker, while_stmt->condition);
    if (!condition_type) {
      return 0; // Error already reported
    }

    // Condition should be a numeric type (treated as boolean)
    if (!type_checker_is_numeric_type(condition_type)) {
      type_checker_report_type_mismatch(checker,
                                        while_stmt->condition->location,
                                        "numeric type", condition_type->name);
      return 0;
    }

    size_t init_snapshot_count = 0;
    unsigned char *init_snapshot =
        type_checker_init_tracker_capture(checker, &init_snapshot_count);
    if (checker->tracked_var_count > 0 && !init_snapshot) {
      type_checker_set_error_at_location(
          checker, statement->location,
          "Out of memory while analyzing variable initialization flow");
      return 0;
    }

    checker->loop_depth++;
    if (while_stmt->body &&
        !type_checker_check_statement(checker, while_stmt->body)) {
      checker->loop_depth--;
      free(init_snapshot);
      return 0;
    }
    checker->loop_depth--;
    type_checker_init_tracker_restore(checker, init_snapshot,
                                      init_snapshot_count);
    free(init_snapshot);

    return 1;
  }

  case AST_FOR_STATEMENT: {
    ForStatement *for_stmt = (ForStatement *)statement->data;
    if (!for_stmt) {
      type_checker_set_error_at_location(checker, statement->location,
                                         "Invalid for statement");
      return 0;
    }

    symbol_table_enter_scope(checker->symbol_table, SCOPE_BLOCK);
    if (!type_checker_init_tracker_enter_scope(checker)) {
      type_checker_set_error_at_location(
          checker, statement->location,
          "Out of memory while entering initialization analysis scope");
      symbol_table_exit_scope(checker->symbol_table);
      return 0;
    }

    if (for_stmt->initializer) {
      int init_ok = 0;
      if (for_stmt->initializer->type == AST_VAR_DECLARATION ||
          for_stmt->initializer->type == AST_ASSIGNMENT ||
          for_stmt->initializer->type == AST_FUNCTION_CALL) {
        init_ok = type_checker_check_statement(checker, for_stmt->initializer);
      } else {
        init_ok = type_checker_check_expression(checker, for_stmt->initializer);
      }
      if (!init_ok) {
        type_checker_init_tracker_exit_scope(checker);
        symbol_table_exit_scope(checker->symbol_table);
        return 0;
      }
    }

    size_t post_init_snapshot_count = 0;
    unsigned char *post_init_snapshot =
        type_checker_init_tracker_capture(checker, &post_init_snapshot_count);
    if (checker->tracked_var_count > 0 && !post_init_snapshot) {
      type_checker_set_error_at_location(
          checker, statement->location,
          "Out of memory while analyzing variable initialization flow");
      type_checker_init_tracker_exit_scope(checker);
      symbol_table_exit_scope(checker->symbol_table);
      return 0;
    }

    if (for_stmt->condition) {
      Type *cond_type = type_checker_infer_type(checker, for_stmt->condition);
      if (!cond_type) {
        free(post_init_snapshot);
        type_checker_init_tracker_exit_scope(checker);
        symbol_table_exit_scope(checker->symbol_table);
        return 0;
      }
      if (!type_checker_is_numeric_type(cond_type)) {
        type_checker_report_type_mismatch(checker,
                                          for_stmt->condition->location,
                                          "numeric type", cond_type->name);
        free(post_init_snapshot);
        type_checker_init_tracker_exit_scope(checker);
        symbol_table_exit_scope(checker->symbol_table);
        return 0;
      }
    }

    if (for_stmt->increment &&
        !type_checker_check_expression(checker, for_stmt->increment)) {
      free(post_init_snapshot);
      type_checker_init_tracker_exit_scope(checker);
      symbol_table_exit_scope(checker->symbol_table);
      return 0;
    }

    checker->loop_depth++;
    if (for_stmt->body &&
        !type_checker_check_statement(checker, for_stmt->body)) {
      checker->loop_depth--;
      free(post_init_snapshot);
      type_checker_init_tracker_exit_scope(checker);
      symbol_table_exit_scope(checker->symbol_table);
      return 0;
    }
    checker->loop_depth--;

    type_checker_init_tracker_restore(checker, post_init_snapshot,
                                      post_init_snapshot_count);
    free(post_init_snapshot);
    type_checker_init_tracker_exit_scope(checker);
    symbol_table_exit_scope(checker->symbol_table);
    return 1;
  }

  case AST_SWITCH_STATEMENT: {
    SwitchStatement *switch_stmt = (SwitchStatement *)statement->data;
    if (!switch_stmt || !switch_stmt->expression) {
      type_checker_set_error_at_location(checker, statement->location,
                                         "Invalid switch statement");
      return 0;
    }

    Type *switch_type =
        type_checker_infer_type(checker, switch_stmt->expression);
    if (!switch_type) {
      return 0;
    }
    if (!type_checker_is_integer_type(switch_type)) {
      type_checker_report_type_mismatch(checker,
                                        switch_stmt->expression->location,
                                        "integer type", switch_type->name);
      return 0;
    }

    size_t init_snapshot_count = 0;
    unsigned char *init_snapshot =
        type_checker_init_tracker_capture(checker, &init_snapshot_count);
    if (checker->tracked_var_count > 0 && !init_snapshot) {
      type_checker_set_error_at_location(
          checker, statement->location,
          "Out of memory while analyzing variable initialization flow");
      return 0;
    }

    long long *case_values = NULL;
    size_t case_value_count = 0;
    int seen_default = 0;

    if (switch_stmt->case_count > 0) {
      case_values = malloc(switch_stmt->case_count * sizeof(long long));
      if (!case_values) {
        type_checker_set_error_at_location(
            checker, statement->location,
            "Memory allocation failed in switch validation");
        return 0;
      }
    }

    checker->switch_depth++;
    for (size_t i = 0; i < switch_stmt->case_count; i++) {
      ASTNode *case_node = switch_stmt->cases ? switch_stmt->cases[i] : NULL;
      if (!case_node || case_node->type != AST_CASE_CLAUSE) {
        type_checker_set_error_at_location(checker, statement->location,
                                           "Invalid case clause in switch");
        checker->switch_depth--;
        free(init_snapshot);
        free(case_values);
        return 0;
      }

      CaseClause *case_clause = (CaseClause *)case_node->data;
      if (!case_clause) {
        type_checker_set_error_at_location(checker, case_node->location,
                                           "Invalid case clause");
        checker->switch_depth--;
        free(init_snapshot);
        free(case_values);
        return 0;
      }

      if (case_clause->is_default) {
        if (seen_default) {
          type_checker_set_error_at_location(
              checker, case_node->location,
              "Switch may only contain one default clause");
          checker->switch_depth--;
          free(init_snapshot);
          free(case_values);
          return 0;
        }
        seen_default = 1;
      } else {
        if (!case_clause->value) {
          type_checker_set_error_at_location(
              checker, case_node->location,
              "Case clause is missing a value expression");
          checker->switch_depth--;
          free(init_snapshot);
          free(case_values);
          return 0;
        }

        Type *case_type = type_checker_infer_type(checker, case_clause->value);
        if (!case_type) {
          checker->switch_depth--;
          free(init_snapshot);
          free(case_values);
          return 0;
        }
        if (!type_checker_is_integer_type(case_type)) {
          type_checker_report_type_mismatch(checker,
                                            case_clause->value->location,
                                            "integer type", case_type->name);
          checker->switch_depth--;
          free(init_snapshot);
          free(case_values);
          return 0;
        }
        if (!type_checker_is_assignable(checker, switch_type, case_type)) {
          type_checker_report_type_mismatch(checker,
                                            case_clause->value->location,
                                            switch_type->name, case_type->name);
          checker->switch_depth--;
          free(init_snapshot);
          free(case_values);
          return 0;
        }

        long long case_value = 0;
        if (!type_checker_eval_integer_constant(case_clause->value,
                                                &case_value)) {
          type_checker_set_error_at_location(
              checker, case_clause->value->location,
              "Case value must be a compile-time integer constant expression");
          checker->switch_depth--;
          free(init_snapshot);
          free(case_values);
          return 0;
        }

        for (size_t j = 0; j < case_value_count; j++) {
          if (case_values[j] == case_value) {
            type_checker_set_error_at_location(
                checker, case_clause->value->location,
                "Duplicate case value '%lld' in switch", case_value);
            checker->switch_depth--;
            free(init_snapshot);
            free(case_values);
            return 0;
          }
        }
        case_values[case_value_count++] = case_value;
      }

      if (!case_clause->body) {
        type_checker_set_error_at_location(checker, case_node->location,
                                           "Case clause must have a body");
        checker->switch_depth--;
        free(init_snapshot);
        free(case_values);
        return 0;
      }

      if (!type_checker_check_statement(checker, case_clause->body)) {
        checker->switch_depth--;
        free(init_snapshot);
        free(case_values);
        return 0;
      }
      type_checker_init_tracker_restore(checker, init_snapshot,
                                        init_snapshot_count);
    }
    checker->switch_depth--;
    type_checker_init_tracker_restore(checker, init_snapshot,
                                      init_snapshot_count);
    free(init_snapshot);
    free(case_values);
    return 1;
  }

  case AST_BREAK_STATEMENT:
    if (checker->loop_depth <= 0 && checker->switch_depth <= 0) {
      type_checker_set_error_at_location(
          checker, statement->location,
          "'break' can only be used inside a loop or switch");
      return 0;
    }
    return 1;

  case AST_CONTINUE_STATEMENT:
    if (checker->loop_depth <= 0) {
      type_checker_set_error_at_location(
          checker, statement->location,
          "'continue' can only be used inside a loop");
      return 0;
    }
    return 1;

  case AST_INLINE_ASM:
    // Inline assembly is passed through without type checking
    return 1;
    break;

  case AST_PROGRAM: {
    // A block of statements
    Program *block = (Program *)statement->data;
    if (block) {
      // Enter a new nested scope
      symbol_table_enter_scope(checker->symbol_table, SCOPE_BLOCK);
      if (!type_checker_init_tracker_enter_scope(checker)) {
        type_checker_set_error_at_location(
            checker, statement->location,
            "Out of memory while entering initialization analysis scope");
        symbol_table_exit_scope(checker->symbol_table);
        return 0;
      }

      int reached_terminator = 0;
      for (size_t i = 0; i < statement->child_count; i++) {
        ASTNode *child = statement->children[i];
        if (reached_terminator && checker->error_reporter && child) {
          error_reporter_add_warning(
              checker->error_reporter, ERROR_SEMANTIC, child->location,
              "Unreachable code: statement will never execute");
        }
        if (!type_checker_check_statement(checker, statement->children[i])) {
          type_checker_init_tracker_exit_scope(checker);
          symbol_table_exit_scope(checker->symbol_table);
          return 0;
        }
        if (type_checker_statement_guarantees_termination(child)) {
          reached_terminator = 1;
        }
      }

      type_checker_init_tracker_exit_scope(checker);
      symbol_table_exit_scope(checker->symbol_table);
    }
    return 1;
  }
  default:
    // Unknown statement type
    type_checker_set_error_at_location(checker, statement->location,
                                       "Unknown statement type");
    return 0;
  }
}

int type_checker_check_expression(TypeChecker *checker, ASTNode *expression) {
  if (!checker || !expression)
    return 0;

  // Use type inference to validate the expression
  Type *expr_type = type_checker_infer_type(checker, expression);
  return expr_type != NULL; // Error already reported if NULL
}

// Enhanced binary expression type checking
Type *type_checker_check_binary_expression(TypeChecker *checker,
                                           BinaryExpression *binop,
                                           SourceLocation location) {
  if (!checker || !binop)
    return NULL;

  Type *left_type = type_checker_infer_type(checker, binop->left);
  Type *right_type = type_checker_infer_type(checker, binop->right);

  if (!left_type || !right_type) {
    return NULL; // Error already reported
  }

  const char *op = binop->operator;

  // String concatenation
  if (strcmp(op, "+") == 0) {
    if (left_type == checker->builtin_string &&
        right_type == checker->builtin_string) {
      return checker->builtin_string;
    }
  }

  // Arithmetic operators require numeric types
  if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || strcmp(op, "*") == 0 ||
      strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {

    if (!type_checker_is_numeric_type(left_type)) {
      type_checker_report_type_mismatch(checker, binop->left->location,
                                        "numeric type", left_type->name);
      return NULL;
    }

    if (!type_checker_is_numeric_type(right_type)) {
      type_checker_report_type_mismatch(checker, binop->right->location,
                                        "numeric type", right_type->name);
      return NULL;
    }

    // Modulo operator requires integer types
    if (strcmp(op, "%") == 0) {
      if (!type_checker_is_integer_type(left_type)) {
        type_checker_report_type_mismatch(checker, binop->left->location,
                                          "integer type", left_type->name);
        return NULL;
      }

      if (!type_checker_is_integer_type(right_type)) {
        type_checker_report_type_mismatch(checker, binop->right->location,
                                          "integer type", right_type->name);
        return NULL;
      }
    }

    return type_checker_promote_types(checker, left_type, right_type, op);
  }

  // Bitwise operators
  if (strcmp(op, "&") == 0 || strcmp(op, "|") == 0 || strcmp(op, "^") == 0 ||
      strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) {
    if (!type_checker_is_integer_type(left_type)) {
      type_checker_report_type_mismatch(checker, binop->left->location,
                                        "integer type", left_type->name);
      return NULL;
    }
    if (!type_checker_is_integer_type(right_type)) {
      type_checker_report_type_mismatch(checker, binop->right->location,
                                        "integer type", right_type->name);
      return NULL;
    }
    return type_checker_promote_types(checker, left_type, right_type, op);
  }

  // Comparison operators
  if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 || strcmp(op, "<") == 0 ||
      strcmp(op, "<=") == 0 || strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
    int is_equality = (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0);
    int left_is_pointer = left_type->kind == TYPE_POINTER;
    int right_is_pointer = right_type->kind == TYPE_POINTER;

    if (left_is_pointer || right_is_pointer) {
      if (!is_equality) {
        type_checker_set_error_at_location(
            checker, location,
            "Pointer ordering comparisons are not supported");
        return NULL;
      }

      int left_is_null = type_checker_is_null_pointer_constant(binop->left);
      int right_is_null = type_checker_is_null_pointer_constant(binop->right);
      int comparable = (left_is_pointer && right_is_pointer &&
                        type_checker_types_equal(left_type, right_type)) ||
                       (left_is_pointer && right_is_null) ||
                       (right_is_pointer && left_is_null);

      if (!comparable) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Cannot compare '%s' with '%s'",
                 left_type->name, right_type->name);
        type_checker_set_error_at_location(checker, location, error_msg);
        return NULL;
      }

      return checker->builtin_int32;
    }

    // Both operands should be comparable (same type or compatible)
    if (!type_checker_are_compatible(left_type, right_type)) {
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg), "Cannot compare '%s' with '%s'",
               left_type->name, right_type->name);
      type_checker_set_error_at_location(checker, location, error_msg);
      return NULL;
    }

    return checker->builtin_int32; // Comparison result is boolean (int32)
  }

  // Logical operators
  if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
    // Both operands should be numeric (treated as boolean)
    if (!type_checker_is_numeric_type(left_type)) {
      type_checker_report_type_mismatch(checker, binop->left->location,
                                        "numeric type", left_type->name);
      return NULL;
    }

    if (!type_checker_is_numeric_type(right_type)) {
      type_checker_report_type_mismatch(checker, binop->right->location,
                                        "numeric type", right_type->name);
      return NULL;
    }

    return checker->builtin_int32; // Logical result is boolean (int32)
  }

  // Unknown operator
  char error_msg[512];
  snprintf(error_msg, sizeof(error_msg), "Unknown binary operator '%s'", op);
  type_checker_set_error_at_location(checker, location, error_msg);
  return NULL;
}
