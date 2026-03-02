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
    memcpy(with_ext + len, ".meth", 6);

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
  // Fully-resolved module paths that have completed import resolution.
  char **resolved_files;
  size_t resolved_count;
  size_t resolved_capacity;
  // Fully-resolved module paths currently being traversed (recursion stack).
  // Used to detect true circular imports.
  char **active_files;
  size_t active_count;
  size_t active_capacity;
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

static const char *get_declaration_name(ASTNode *decl) {
  if (!decl || !decl->data) {
    return NULL;
  }

  switch (decl->type) {
  case AST_FUNCTION_DECLARATION:
    return ((FunctionDeclaration *)decl->data)->name;
  case AST_STRUCT_DECLARATION:
    return ((StructDeclaration *)decl->data)->name;
  case AST_ENUM_DECLARATION:
    return ((EnumDeclaration *)decl->data)->name;
  case AST_VAR_DECLARATION:
    return ((VarDeclaration *)decl->data)->name;
  default:
    return NULL;
  }
}

// Return values:
//   1: added
//   0: already present
//  -1: internal failure (e.g. allocation failure)
static int path_set_add(char ***paths, size_t *count, size_t *capacity,
                        const char *path);

static void collect_called_function_names(ASTNode *node, char ***names,
                                          size_t *count, size_t *capacity) {
  if (!node || !names || !count || !capacity) {
    return;
  }

  switch (node->type) {
  case AST_PROGRAM: {
    Program *prog = (Program *)node->data;
    if (!prog) {
      return;
    }
    for (size_t i = 0; i < prog->declaration_count; i++) {
      collect_called_function_names(prog->declarations[i], names, count,
                                    capacity);
    }
    return;
  }
  case AST_VAR_DECLARATION: {
    VarDeclaration *var = (VarDeclaration *)node->data;
    if (var && var->initializer) {
      collect_called_function_names(var->initializer, names, count, capacity);
    }
    return;
  }
  case AST_FUNCTION_DECLARATION: {
    FunctionDeclaration *func = (FunctionDeclaration *)node->data;
    if (func && func->body) {
      collect_called_function_names(func->body, names, count, capacity);
    }
    return;
  }
  case AST_FUNCTION_CALL: {
    CallExpression *call = (CallExpression *)node->data;
    if (!call) {
      return;
    }
    if (call->function_name && call->function_name[0] != '\0') {
      (void)path_set_add(names, count, capacity, call->function_name);
    }
    if (call->object) {
      collect_called_function_names(call->object, names, count, capacity);
    }
    for (size_t i = 0; i < call->argument_count; i++) {
      collect_called_function_names(call->arguments[i], names, count, capacity);
    }
    return;
  }
  case AST_FUNC_PTR_CALL: {
    FuncPtrCall *call = (FuncPtrCall *)node->data;
    if (!call) {
      return;
    }
    if (call->function) {
      collect_called_function_names(call->function, names, count, capacity);
    }
    for (size_t i = 0; i < call->argument_count; i++) {
      collect_called_function_names(call->arguments[i], names, count, capacity);
    }
    return;
  }
  case AST_ASSIGNMENT: {
    Assignment *assign = (Assignment *)node->data;
    if (!assign) {
      return;
    }
    if (assign->target) {
      collect_called_function_names(assign->target, names, count, capacity);
    }
    if (assign->value) {
      collect_called_function_names(assign->value, names, count, capacity);
    }
    return;
  }
  case AST_RETURN_STATEMENT:
    if (node->child_count > 0) {
      collect_called_function_names(node->children[0], names, count, capacity);
    }
    return;
  case AST_BINARY_EXPRESSION: {
    BinaryExpression *bin = (BinaryExpression *)node->data;
    if (bin) {
      collect_called_function_names(bin->left, names, count, capacity);
      collect_called_function_names(bin->right, names, count, capacity);
    }
    return;
  }
  case AST_UNARY_EXPRESSION: {
    UnaryExpression *un = (UnaryExpression *)node->data;
    if (un && un->operand) {
      collect_called_function_names(un->operand, names, count, capacity);
    }
    return;
  }
  case AST_MEMBER_ACCESS: {
    MemberAccess *member = (MemberAccess *)node->data;
    if (member && member->object) {
      collect_called_function_names(member->object, names, count, capacity);
    }
    return;
  }
  case AST_INDEX_EXPRESSION: {
    ArrayIndexExpression *idx = (ArrayIndexExpression *)node->data;
    if (idx) {
      collect_called_function_names(idx->array, names, count, capacity);
      collect_called_function_names(idx->index, names, count, capacity);
    }
    return;
  }
  case AST_IF_STATEMENT:
  case AST_WHILE_STATEMENT:
  case AST_FOR_STATEMENT:
    for (size_t i = 0; i < node->child_count; i++) {
      collect_called_function_names(node->children[i], names, count, capacity);
    }
    return;
  case AST_SWITCH_STATEMENT: {
    SwitchStatement *sw = (SwitchStatement *)node->data;
    if (sw) {
      if (sw->expression) {
        collect_called_function_names(sw->expression, names, count, capacity);
      }
      for (size_t i = 0; i < sw->case_count; i++) {
        collect_called_function_names(sw->cases[i], names, count, capacity);
      }
    }
    return;
  }
  case AST_CASE_CLAUSE: {
    CaseClause *cc = (CaseClause *)node->data;
    if (cc) {
      if (cc->value) {
        collect_called_function_names(cc->value, names, count, capacity);
      }
      if (cc->body) {
        collect_called_function_names(cc->body, names, count, capacity);
      }
    }
    return;
  }
  default:
    return;
  }
}

static int path_set_contains(char **paths, size_t count, const char *path) {
  if (!paths || !path) {
    return 0;
  }

  for (size_t i = 0; i < count; i++) {
    if (strcmp(paths[i], path) == 0) {
      return 1;
    }
  }

  return 0;
}

// Return values:
//   1: added
//   0: already present
//  -1: internal failure (e.g. allocation failure)
static int path_set_add(char ***paths, size_t *count, size_t *capacity,
                        const char *path) {
  if (!paths || !count || !capacity || !path) {
    return -1;
  }

  if (path_set_contains(*paths, *count, path)) {
    return 0;
  }

  if (*count >= *capacity) {
    size_t new_capacity = *capacity == 0 ? 8 : *capacity * 2;
    char **new_paths = realloc(*paths, new_capacity * sizeof(char *));
    if (!new_paths) {
      return -1;
    }
    *paths = new_paths;
    *capacity = new_capacity;
  }

  (*paths)[*count] = strdup(path);
  if (!(*paths)[*count]) {
    return -1;
  }
  (*count)++;
  return 1;
}

static void path_set_remove(char **paths, size_t *count, const char *path) {
  if (!paths || !count || !path) {
    return;
  }

  for (size_t i = 0; i < *count; i++) {
    if (strcmp(paths[i], path) == 0) {
      free(paths[i]);
      if (i + 1 < *count) {
        memmove(&paths[i], &paths[i + 1], ((*count) - i - 1) * sizeof(char *));
      }
      (*count)--;
      return;
    }
  }
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

// Build a human-readable import chain string like "main.meth -> utils.meth ->
// math.meth"
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

      if (path_set_contains(ctx->active_files, ctx->active_count, full_path)) {
        if (ctx->reporter) {
          char *chain = format_import_chain(ctx);
          char error_msg[1024];
          snprintf(error_msg, sizeof(error_msg),
                   "Circular import of '%s' (import chain: %s -> %s)",
                   import_decl->module_name, chain, import_decl->module_name);
          error_reporter_add_warning(ctx->reporter, ERROR_IO, decl->location,
                                     error_msg);
          free(chain);
        }
        free(full_path);
        ast_destroy_node(decl);
        continue;
      }

      if (path_set_contains(ctx->resolved_files, ctx->resolved_count,
                            full_path)) {
        // Duplicate import of an already-resolved module is a no-op.
        free(full_path);
        ast_destroy_node(decl);
        continue;
      }

      int add_active_status = path_set_add(&ctx->active_files, &ctx->active_count,
                                           &ctx->active_capacity, full_path);
      if (add_active_status < 0) {
        if (ctx->reporter) {
          error_reporter_add_error(
              ctx->reporter, ERROR_INTERNAL, decl->location,
              "Failed to track active imports (out of memory)");
        }
        free(full_path);
        ast_destroy_node(decl);
        *had_error = 1;
        continue;
      }

      if (!push_import_chain(ctx, import_decl->module_name)) {
        path_set_remove(ctx->active_files, &ctx->active_count, full_path);
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
        path_set_remove(ctx->active_files, &ctx->active_count, full_path);
        pop_import_chain(ctx);
        free(full_path);
        ast_destroy_node(decl);
        *had_error = 1;
        continue;
      }

      Lexer *lexer = lexer_create(source);
      Parser *parser = parser_create_with_error_reporter(lexer, ctx->reporter);
      ASTNode *imported_program = parser_parse_program(parser);

      int import_succeeded = 0;
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
        import_succeeded = 1;
        // Recursively resolve imports in the imported module
        imported_program = process_imports_recursive(ctx, imported_program,
                                                     full_path, had_error, 1);
        process_import_strs_in_node(ctx, imported_program, full_path,
                                    had_error);

        if (imported_program) {
          Program *imported_prog_data = (Program *)imported_program->data;
          int has_any_export = 0;
          int include_all = 0;
          int *include_flags = NULL;
          char **required_names = NULL;
          size_t required_count = 0;
          size_t required_capacity = 0;

          // Check if the module uses export at all
          for (size_t j = 0; j < imported_prog_data->declaration_count; j++) {
            if (is_declaration_exported(imported_prog_data->declarations[j])) {
              has_any_export = 1;
              break;
            }
          }

          if (has_any_export) {
            include_flags =
                calloc(imported_prog_data->declaration_count, sizeof(int));
            if (!include_flags) {
              include_all = 1;
              *had_error = 1;
              if (ctx->reporter) {
                error_reporter_add_error(
                    ctx->reporter, ERROR_INTERNAL, decl->location,
                    "Failed to process exports (out of memory)");
              }
            } else {
              for (size_t j = 0; j < imported_prog_data->declaration_count;
                   j++) {
                ASTNode *imp_decl = imported_prog_data->declarations[j];
                if (!is_declaration_exported(imp_decl)) {
                  continue;
                }
                include_flags[j] = 1;
                collect_called_function_names(imp_decl, &required_names,
                                              &required_count,
                                              &required_capacity);
              }

              int changed = 1;
              while (changed) {
                changed = 0;
                for (size_t j = 0; j < imported_prog_data->declaration_count;
                     j++) {
                  if (include_flags[j]) {
                    continue;
                  }
                  ASTNode *imp_decl = imported_prog_data->declarations[j];
                  const char *name = get_declaration_name(imp_decl);
                  if (!name || !path_set_contains(required_names, required_count,
                                                  name)) {
                    continue;
                  }
                  include_flags[j] = 1;
                  changed = 1;
                  collect_called_function_names(imp_decl, &required_names,
                                                &required_count,
                                                &required_capacity);
                }
              }
            }
          } else {
            include_all = 1;
          }

          for (size_t j = 0; j < imported_prog_data->declaration_count; j++) {
            ASTNode *imp_decl = imported_prog_data->declarations[j];
            int include_decl = include_all;
            if (!include_decl && include_flags) {
              include_decl = include_flags[j];
            }

            if (include_decl) {
              ADD_DECL(imp_decl);
            } else {
              ast_destroy_node(imp_decl);
            }
          }

          if (required_names) {
            for (size_t j = 0; j < required_count; j++) {
              free(required_names[j]);
            }
            free(required_names);
          }
          free(include_flags);

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
      path_set_remove(ctx->active_files, &ctx->active_count, full_path);
      if (import_succeeded) {
        int add_resolved_status = path_set_add(
            &ctx->resolved_files, &ctx->resolved_count, &ctx->resolved_capacity,
            full_path);
        if (add_resolved_status < 0) {
          if (ctx->reporter) {
            error_reporter_add_error(
                ctx->reporter, ERROR_INTERNAL, decl->location,
                "Failed to track resolved imports (out of memory)");
          }
          *had_error = 1;
        }
      }
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
  ctx.resolved_files = NULL;
  ctx.resolved_count = 0;
  ctx.resolved_capacity = 0;
  ctx.active_files = NULL;
  ctx.active_count = 0;
  ctx.active_capacity = 0;
  ctx.import_chain = NULL;
  ctx.chain_depth = 0;
  ctx.chain_capacity = 0;
  ctx.reporter = reporter;
  ctx.options = options;

  // Register the root file as active + push onto chain
  char *abs_base = canonicalize_path(base_path);
  if (abs_base) {
    path_set_add(&ctx.active_files, &ctx.active_count, &ctx.active_capacity,
                 abs_base);
    free(abs_base);
  }

  // Extract filename for readable chain
  const char *last_slash = strrchr(base_path, '/');
  const char *last_backslash = strrchr(base_path, '\\');
  const char *last_sep =
      (last_slash > last_backslash) ? last_slash : last_backslash;
  const char *base_name = last_sep ? last_sep + 1 : base_path;
  if (!push_import_chain(&ctx, base_name)) {
    for (size_t i = 0; i < ctx.resolved_count; i++) {
      free(ctx.resolved_files[i]);
    }
    free(ctx.resolved_files);
    for (size_t i = 0; i < ctx.active_count; i++) {
      free(ctx.active_files[i]);
    }
    free(ctx.active_files);
    free(ctx.import_chain);
    return 0;
  }

  int had_error = 0;
  process_imports_recursive(&ctx, program, base_path, &had_error, 0);
  process_import_strs_in_node(&ctx, program, base_path, &had_error);

  // Cleanup
  pop_import_chain(&ctx);
  for (size_t i = 0; i < ctx.resolved_count; i++) {
    free(ctx.resolved_files[i]);
  }
  free(ctx.resolved_files);
  for (size_t i = 0; i < ctx.active_count; i++) {
    free(ctx.active_files[i]);
  }
  free(ctx.active_files);
  free(ctx.import_chain);

  return !had_error;
}

int resolve_imports(ASTNode *program, const char *base_path,
                    ErrorReporter *reporter) {
  ImportResolverOptions options = {0};
  return resolve_imports_with_options(program, base_path, reporter, &options);
}
