#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "import_resolver.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_absolute_path(const char *path) {
  if (!path || path[0] == '\0') {
    return 0;
  }
#ifdef _WIN32
  if (isalpha((unsigned char)path[0]) && path[1] == ':' &&
      (path[2] == '\\' || path[2] == '/')) {
    return 1;
  }
  if (path[0] == '\\' || path[0] == '/') {
    return 1;
  }
  return 0;
#else
  return path[0] == '/';
#endif
}

static char *canonicalize_path(const char *path) {
  if (!path) {
    return NULL;
  }
#ifdef _WIN32
  char *full = _fullpath(NULL, path, 0);
  if (full) {
    return full;
  }
#else
  char *full = realpath(path, NULL);
  if (full) {
    return full;
  }
#endif
  return strdup(path);
}

static int path_has_extension(const char *path) {
  if (!path) {
    return 0;
  }

  const char *last_slash = strrchr(path, '/');
  const char *last_backslash = strrchr(path, '\\');
  const char *last_sep =
      (last_slash > last_backslash) ? last_slash : last_backslash;
  const char *last_dot = strrchr(path, '.');

  return last_dot && (!last_sep || last_dot > last_sep);
}

static char *directory_from_file_path(const char *file_path) {
  if (!file_path) {
    return strdup(".");
  }

  const char *last_slash = strrchr(file_path, '/');
  const char *last_backslash = strrchr(file_path, '\\');
  const char *last_sep =
      (last_slash > last_backslash) ? last_slash : last_backslash;

  if (!last_sep) {
    return strdup(".");
  }

  size_t len = (size_t)(last_sep - file_path + 1);
  char *dir = malloc(len + 1);
  if (!dir) {
    return NULL;
  }

  memcpy(dir, file_path, len);
  dir[len] = '\0';
  return dir;
}

static char *join_paths(const char *left, const char *right) {
  if (!right) {
    return NULL;
  }
  if (!left || left[0] == '\0') {
    return strdup(right);
  }

  size_t left_len = strlen(left);
  size_t right_len = strlen(right);
  int has_sep = (left_len > 0 &&
                 (left[left_len - 1] == '/' || left[left_len - 1] == '\\'));

  char *joined = malloc(left_len + right_len + (has_sep ? 1 : 2));
  if (!joined) {
    return NULL;
  }

  memcpy(joined, left, left_len);
  if (!has_sep) {
#ifdef _WIN32
    joined[left_len++] = '\\';
#else
    joined[left_len++] = '/';
#endif
  }
  memcpy(joined + left_len, right, right_len);
  joined[left_len + right_len] = '\0';

  return joined;
}

static int file_exists_readable(const char *path) {
  if (!path) {
    return 0;
  }

  FILE *file = fopen(path, "r");
  if (!file) {
    return 0;
  }

  fclose(file);
  return 1;
}

static char *resolve_candidate_path(const char *candidate_base) {
  if (!candidate_base || candidate_base[0] == '\0') {
    return NULL;
  }

  if (file_exists_readable(candidate_base)) {
    return canonicalize_path(candidate_base);
  }

  if (!path_has_extension(candidate_base)) {
    size_t len = strlen(candidate_base);
    char *with_ext = malloc(len + 6);
    if (!with_ext) {
      return NULL;
    }
    memcpy(with_ext, candidate_base, len);
    memcpy(with_ext + len, ".masm", 6);

    if (file_exists_readable(with_ext)) {
      char *resolved = canonicalize_path(with_ext);
      free(with_ext);
      return resolved;
    }

    free(with_ext);
  }

  return NULL;
}

static int import_uses_std_namespace(const char *import_path) {
  if (!import_path) {
    return 0;
  }
  return strncmp(import_path, "std/", 4) == 0 ||
         strncmp(import_path, "std\\", 4) == 0;
}

typedef struct {
  char **visited_files;
  size_t count;
  size_t capacity;
  // Import chain for error reporting (stack of file paths)
  char **import_chain;
  size_t chain_depth;
  size_t chain_capacity;
  ErrorReporter *reporter;
  const ImportResolverOptions *options;
} ImportContext;

static char *resolve_import_path(ImportContext *ctx,
                                 const char *current_file_path,
                                 const char *import_path) {
  if (!import_path || import_path[0] == '\0') {
    return NULL;
  }

  if (is_absolute_path(import_path)) {
    return resolve_candidate_path(import_path);
  }

  if (ctx && ctx->options && ctx->options->stdlib_directory &&
      import_uses_std_namespace(import_path)) {
    char *std_candidate =
        join_paths(ctx->options->stdlib_directory, import_path);
    if (std_candidate) {
      char *resolved = resolve_candidate_path(std_candidate);
      free(std_candidate);
      if (resolved) {
        return resolved;
      }
    }
  }

  char *current_dir = directory_from_file_path(current_file_path);
  if (current_dir) {
    char *local_candidate = join_paths(current_dir, import_path);
    if (local_candidate) {
      char *resolved = resolve_candidate_path(local_candidate);
      free(local_candidate);
      free(current_dir);
      if (resolved) {
        return resolved;
      }
    } else {
      free(current_dir);
    }
  }

  if (ctx && ctx->options && ctx->options->import_directories) {
    for (size_t i = 0; i < ctx->options->import_directory_count; i++) {
      const char *include_dir = ctx->options->import_directories[i];
      if (!include_dir || include_dir[0] == '\0') {
        continue;
      }

      char *include_candidate = join_paths(include_dir, import_path);
      if (!include_candidate) {
        continue;
      }

      char *resolved = resolve_candidate_path(include_candidate);
      free(include_candidate);
      if (resolved) {
        return resolved;
      }
    }
  }

  return resolve_candidate_path(import_path);
}

static char *read_file_content(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file)
    return NULL;
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  if (size < 0) {
    fclose(file);
    return NULL;
  }
  fseek(file, 0, SEEK_SET);
  char *buffer = malloc(size + 1);
  if (!buffer) {
    fclose(file);
    return NULL;
  }
  size_t bytes_read = fread(buffer, 1, size, file);
  if (bytes_read < (size_t)size && ferror(file)) {
    free(buffer);
    fclose(file);
    return NULL;
  }
  buffer[bytes_read] = '\0';
  fclose(file);
  return buffer;
}

// Check if a declaration is exported (for filtering during import)
static int is_declaration_exported(ASTNode *decl) {
  if (!decl || !decl->data)
    return 0;

  if (decl->type == AST_FUNCTION_DECLARATION) {
    return ((FunctionDeclaration *)decl->data)->is_exported;
  }
  if (decl->type == AST_STRUCT_DECLARATION) {
    return ((StructDeclaration *)decl->data)->is_exported;
  }
  if (decl->type == AST_ENUM_DECLARATION) {
    return ((EnumDeclaration *)decl->data)->is_exported;
  }
  if (decl->type == AST_VAR_DECLARATION) {
    return ((VarDeclaration *)decl->data)->is_exported;
  }
  return 0;
}

// Return values:
//   1: added
//   0: already visited
//  -1: internal failure (e.g. allocation failure)
static int add_visited_file(ImportContext *ctx, const char *path) {
  if (!ctx || !path) {
    return -1;
  }

  for (size_t i = 0; i < ctx->count; i++) {
    if (strcmp(ctx->visited_files[i], path) == 0) {
      return 0; // Already visited
    }
  }

  if (ctx->count >= ctx->capacity) {
    size_t new_capacity = ctx->capacity == 0 ? 8 : ctx->capacity * 2;
    char **new_files =
        realloc(ctx->visited_files, new_capacity * sizeof(char *));
    if (!new_files) {
      return -1;
    }
    ctx->visited_files = new_files;
    ctx->capacity = new_capacity;
  }

  ctx->visited_files[ctx->count] = strdup(path);
  if (!ctx->visited_files[ctx->count]) {
    return -1;
  }
  ctx->count++;
  return 1;
}

static int push_import_chain(ImportContext *ctx, const char *path) {
  if (!ctx || !path) {
    return 0;
  }

  if (ctx->chain_depth >= ctx->chain_capacity) {
    size_t new_capacity =
        ctx->chain_capacity == 0 ? 8 : ctx->chain_capacity * 2;
    char **new_chain =
        realloc(ctx->import_chain, new_capacity * sizeof(char *));
    if (!new_chain) {
      return 0;
    }
    ctx->import_chain = new_chain;
    ctx->chain_capacity = new_capacity;
  }

  ctx->import_chain[ctx->chain_depth] = strdup(path);
  if (!ctx->import_chain[ctx->chain_depth]) {
    return 0;
  }
  ctx->chain_depth++;
  return 1;
}

static void pop_import_chain(ImportContext *ctx) {
  if (ctx && ctx->chain_depth > 0) {
    ctx->chain_depth--;
    free(ctx->import_chain[ctx->chain_depth]);
    ctx->import_chain[ctx->chain_depth] = NULL;
  }
}

// Build a human-readable import chain string like "main.masm -> utils.masm ->
// math.masm"
static char *format_import_chain(ImportContext *ctx) {
  if (!ctx || ctx->chain_depth == 0)
    return strdup("");

  // Calculate required length
  size_t total_len = 0;
  for (size_t i = 0; i < ctx->chain_depth; i++) {
    total_len += strlen(ctx->import_chain[i]);
    if (i < ctx->chain_depth - 1)
      total_len += 4; // " -> "
  }

  char *chain_str = malloc(total_len + 1);
  if (!chain_str)
    return strdup("");

  chain_str[0] = '\0';
  for (size_t i = 0; i < ctx->chain_depth; i++) {
    strcat(chain_str, ctx->import_chain[i]);
    if (i < ctx->chain_depth - 1) {
      strcat(chain_str, " -> ");
    }
  }

  return chain_str;
}

static void process_import_strs_in_node(ImportContext *ctx, ASTNode *node,
                                        const char *current_file_path,
                                        int *had_error) {
  if (!node)
    return;

  if (node->type == AST_IMPORT_STR) {
    ImportStrExpression *import_str = (ImportStrExpression *)node->data;
    char *full_path =
        resolve_import_path(ctx, current_file_path, import_str->file_path);

    if (!full_path) {
      if (ctx->reporter) {
        char *chain = format_import_chain(ctx);
        char error_msg[1024];
        snprintf(error_msg, sizeof(error_msg),
                 "Could not resolve embedded file '%s' (import chain: %s)",
                 import_str->file_path, chain);
        error_reporter_add_error(ctx->reporter, ERROR_IO, node->location,
                                 error_msg);
        free(chain);
      }
      *had_error = 1;
      return;
    }

    char *source = read_file_content(full_path);
    if (!source) {
      if (ctx->reporter) {
        char *chain = format_import_chain(ctx);
        char error_msg[1024];
        snprintf(error_msg, sizeof(error_msg),
                 "Could not read embedded file '%s' (import chain: %s)",
                 import_str->file_path, chain);
        error_reporter_add_error(ctx->reporter, ERROR_IO, node->location,
                                 error_msg);
        free(chain);
      }
      free(full_path);
      *had_error = 1;
      return;
    }

    free(full_path);

    // Free old data and change node type in-place
    free(import_str->file_path);
    free(import_str);

    StringLiteral *string_literal = malloc(sizeof(StringLiteral));
    string_literal->value = source;

    node->type = AST_STRING_LITERAL;
    node->data = string_literal;
    return;
  }

  // Traverse via typed data structures to avoid cycles in children[].
  switch (node->type) {
  case AST_PROGRAM: {
    Program *prog = (Program *)node->data;
    if (prog) {
      for (size_t i = 0; i < prog->declaration_count; i++) {
        process_import_strs_in_node(ctx, prog->declarations[i],
                                    current_file_path, had_error);
      }
    }
    break;
  }
  case AST_VAR_DECLARATION: {
    VarDeclaration *var = (VarDeclaration *)node->data;
    if (var && var->initializer) {
      process_import_strs_in_node(ctx, var->initializer, current_file_path,
                                  had_error);
    }
    break;
  }
  case AST_FUNCTION_DECLARATION: {
    FunctionDeclaration *func = (FunctionDeclaration *)node->data;
    if (func && func->body) {
      process_import_strs_in_node(ctx, func->body, current_file_path,
                                  had_error);
    }
    break;
  }
  case AST_FUNCTION_CALL: {
    CallExpression *call = (CallExpression *)node->data;
    if (call) {
      for (size_t i = 0; i < call->argument_count; i++) {
        process_import_strs_in_node(ctx, call->arguments[i], current_file_path,
                                    had_error);
      }
    }
    break;
  }
  case AST_ASSIGNMENT: {
    Assignment *assign = (Assignment *)node->data;
    if (assign && assign->value) {
      process_import_strs_in_node(ctx, assign->value, current_file_path,
                                  had_error);
    }
    break;
  }
  case AST_RETURN_STATEMENT: {
    // Return value is child[0] if present
    if (node->child_count > 0) {
      process_import_strs_in_node(ctx, node->children[0], current_file_path,
                                  had_error);
    }
    break;
  }
  case AST_BINARY_EXPRESSION: {
    BinaryExpression *bin = (BinaryExpression *)node->data;
    if (bin) {
      process_import_strs_in_node(ctx, bin->left, current_file_path, had_error);
      process_import_strs_in_node(ctx, bin->right, current_file_path,
                                  had_error);
    }
    break;
  }
  case AST_UNARY_EXPRESSION: {
    UnaryExpression *un = (UnaryExpression *)node->data;
    if (un && un->operand) {
      process_import_strs_in_node(ctx, un->operand, current_file_path,
                                  had_error);
    }
    break;
  }
  case AST_IF_STATEMENT:
  case AST_WHILE_STATEMENT:
  case AST_FOR_STATEMENT: {
    // These use children[] but don't create cycles
    for (size_t i = 0; i < node->child_count; i++) {
      process_import_strs_in_node(ctx, node->children[i], current_file_path,
                                  had_error);
    }
    break;
  }
  case AST_SWITCH_STATEMENT: {
    SwitchStatement *sw = (SwitchStatement *)node->data;
    if (sw) {
      if (sw->expression) {
        process_import_strs_in_node(ctx, sw->expression, current_file_path,
                                    had_error);
      }
      for (size_t i = 0; i < sw->case_count; i++) {
        process_import_strs_in_node(ctx, sw->cases[i], current_file_path,
                                    had_error);
      }
    }
    break;
  }
  case AST_CASE_CLAUSE: {
    CaseClause *cc = (CaseClause *)node->data;
    if (cc) {
      if (cc->value) {
        process_import_strs_in_node(ctx, cc->value, current_file_path,
                                    had_error);
      }
      if (cc->body) {
        process_import_strs_in_node(ctx, cc->body, current_file_path,
                                    had_error);
      }
    }
    break;
  }
  default:
    // Leaf nodes or nodes that can't contain import_str (identifiers,
    // literals, member access, etc.)
    break;
  }
}

static ASTNode *process_imports_recursive(ImportContext *ctx, ASTNode *program,
                                          const char *current_file_path,
                                          int *had_error, int is_nested) {
  (void)is_nested; // Reserved for future use (e.g. filtering main from nested
                   // imports)
  if (!program || program->type != AST_PROGRAM)
    return program;
  Program *prog_data = (Program *)program->data;

  ASTNode **new_declarations = NULL;
  size_t new_declaration_count = 0;
  size_t new_capacity = 0;

#define ADD_DECL(decl_node)                                                    \
  do {                                                                         \
    if (new_declaration_count >= new_capacity) {                               \
      size_t grown_capacity = new_capacity == 0 ? 16 : new_capacity * 2;       \
      ASTNode **grown =                                                        \
          realloc(new_declarations, grown_capacity * sizeof(ASTNode *));       \
      if (!grown) {                                                            \
        *had_error = 1;                                                        \
        goto process_imports_cleanup;                                          \
      }                                                                        \
      new_capacity = grown_capacity;                                           \
      new_declarations = grown;                                                \
    }                                                                          \
    new_declarations[new_declaration_count++] = (decl_node);                   \
  } while (0)

  for (size_t i = 0; i < prog_data->declaration_count; i++) {
    ASTNode *decl = prog_data->declarations[i];
    if (decl->type == AST_IMPORT) {
      ImportDeclaration *import_decl = (ImportDeclaration *)decl->data;
      char *full_path =
          resolve_import_path(ctx, current_file_path, import_decl->module_name);

      if (!full_path) {
        if (ctx->reporter) {
          char *chain = format_import_chain(ctx);
          char error_msg[1024];
          snprintf(error_msg, sizeof(error_msg),
                   "Could not resolve imported file '%s' (import chain: %s)",
                   import_decl->module_name, chain);
          error_reporter_add_error(ctx->reporter, ERROR_IO, decl->location,
                                   error_msg);
          free(chain);
        }
        ast_destroy_node(decl);
        *had_error = 1;
        continue;
      }

      int visit_status = add_visited_file(ctx, full_path);
      if (visit_status < 0) {
        if (ctx->reporter) {
          error_reporter_add_error(
              ctx->reporter, ERROR_INTERNAL, decl->location,
              "Failed to track visited imports (out of memory)");
        }
        free(full_path);
        ast_destroy_node(decl);
        *had_error = 1;
        continue;
      }

      if (visit_status == 0) {
        // Circular dependency or already imported; report a warning with chain
        if (ctx->reporter) {
          char *chain = format_import_chain(ctx);
          char error_msg[1024];
          snprintf(
              error_msg, sizeof(error_msg),
              "Circular or duplicate import of '%s' (import chain: %s -> %s)",
              import_decl->module_name, chain, import_decl->module_name);
          error_reporter_add_warning(ctx->reporter, ERROR_IO, decl->location,
                                     error_msg);
          free(chain);
        }
        free(full_path);
        ast_destroy_node(decl);
        continue;
      }

      if (!push_import_chain(ctx, import_decl->module_name)) {
        free(full_path);
        ast_destroy_node(decl);
        *had_error = 1;
        continue;
      }

      char *source = read_file_content(full_path);
      if (!source) {
        if (ctx->reporter) {
          char *chain = format_import_chain(ctx);
          char error_msg[1024];
          snprintf(error_msg, sizeof(error_msg),
                   "Could not read imported file '%s' (import chain: %s)",
                   import_decl->module_name, chain);
          error_reporter_add_error(ctx->reporter, ERROR_IO, decl->location,
                                   error_msg);
          free(chain);
        }
        pop_import_chain(ctx);
        free(full_path);
        ast_destroy_node(decl);
        *had_error = 1;
        continue;
      }

      Lexer *lexer = lexer_create(source);
      Parser *parser = parser_create_with_error_reporter(lexer, ctx->reporter);
      ASTNode *imported_program = parser_parse_program(parser);

      if (parser->had_error || !imported_program) {
        if (ctx->reporter) {
          char *chain = format_import_chain(ctx);
          char error_msg[1024];
          snprintf(error_msg, sizeof(error_msg),
                   "Parse error in imported file '%s' (import chain: %s)",
                   import_decl->module_name, chain);
          error_reporter_add_error(ctx->reporter, ERROR_SYNTAX, decl->location,
                                   error_msg);
          free(chain);
        }
        *had_error = 1;
        if (imported_program) {
          ast_destroy_node(imported_program);
        }
      } else {
        // Recursively resolve imports in the imported module
        imported_program = process_imports_recursive(ctx, imported_program,
                                                     full_path, had_error, 1);
        process_import_strs_in_node(ctx, imported_program, full_path,
                                    had_error);

        if (imported_program) {
          Program *imported_prog_data = (Program *)imported_program->data;
          int has_any_export = 0;

          // Check if the module uses export at all
          for (size_t j = 0; j < imported_prog_data->declaration_count; j++) {
            if (is_declaration_exported(imported_prog_data->declarations[j])) {
              has_any_export = 1;
              break;
            }
          }

          for (size_t j = 0; j < imported_prog_data->declaration_count; j++) {
            ASTNode *imp_decl = imported_prog_data->declarations[j];

            if (has_any_export) {
              // Module uses export: only include exported declarations
              if (is_declaration_exported(imp_decl)) {
                ADD_DECL(imp_decl);
              } else {
                ast_destroy_node(imp_decl);
              }
            } else {
              // No export keyword used at all: include everything
              // (backwards compat)
              ADD_DECL(imp_decl);
            }
          }

          // Cleanup imported program AST container
          free(imported_prog_data->declarations);
          free(imported_prog_data);
          free(imported_program->children);
          free(imported_program);
        }
      }

      parser_destroy(parser);
      lexer_destroy(lexer);
      free(source);
      pop_import_chain(ctx);
      free(full_path);

      ast_destroy_node(decl); // destroy the AST_IMPORT node itself

    } else {
      ADD_DECL(decl);
    }
  }

#undef ADD_DECL

process_imports_cleanup:
  // Replace old declarations in current program
  free(prog_data->declarations);
  prog_data->declarations = new_declarations;
  prog_data->declaration_count = new_declaration_count;

  // Update children array to match declarations perfectly
  free(program->children);
  if (new_declaration_count > 0) {
    program->children = malloc(new_declaration_count * sizeof(ASTNode *));
    if (!program->children) {
      *had_error = 1;
      program->child_count = 0;
      return program;
    }
    program->child_count = new_declaration_count;
    for (size_t i = 0; i < new_declaration_count; i++) {
      program->children[i] = new_declarations[i];
    }
  } else {
    program->children = NULL;
    program->child_count = 0;
  }

  return program;
}

int resolve_imports_with_options(ASTNode *program, const char *base_path,
                                 ErrorReporter *reporter,
                                 const ImportResolverOptions *options) {
  if (!program || program->type != AST_PROGRAM)
    return 0;

  ImportContext ctx;
  ctx.visited_files = NULL;
  ctx.count = 0;
  ctx.capacity = 0;
  ctx.import_chain = NULL;
  ctx.chain_depth = 0;
  ctx.chain_capacity = 0;
  ctx.reporter = reporter;
  ctx.options = options;

  // Register the root file as visited + push onto chain
  char *abs_base = canonicalize_path(base_path);
  if (abs_base) {
    add_visited_file(&ctx, abs_base);
    free(abs_base);
  }

  // Extract filename for readable chain
  const char *last_slash = strrchr(base_path, '/');
  const char *last_backslash = strrchr(base_path, '\\');
  const char *last_sep =
      (last_slash > last_backslash) ? last_slash : last_backslash;
  const char *base_name = last_sep ? last_sep + 1 : base_path;
  if (!push_import_chain(&ctx, base_name)) {
    for (size_t i = 0; i < ctx.count; i++) {
      free(ctx.visited_files[i]);
    }
    free(ctx.visited_files);
    free(ctx.import_chain);
    return 0;
  }

  int had_error = 0;
  process_imports_recursive(&ctx, program, base_path, &had_error, 0);
  process_import_strs_in_node(&ctx, program, base_path, &had_error);

  // Cleanup
  pop_import_chain(&ctx);
  for (size_t i = 0; i < ctx.count; i++) {
    free(ctx.visited_files[i]);
  }
  free(ctx.visited_files);
  free(ctx.import_chain);

  return !had_error;
}

int resolve_imports(ASTNode *program, const char *base_path,
                    ErrorReporter *reporter) {
  ImportResolverOptions options = {0};
  return resolve_imports_with_options(program, base_path, reporter, &options);
}
