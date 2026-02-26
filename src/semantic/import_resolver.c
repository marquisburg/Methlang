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

// Path resolution helper
static char *resolve_path(const char *base_path, const char *import_path) {
  // If import_path is absolute, return it directly
#ifdef _WIN32
  if (import_path[0] && isalpha(import_path[0]) && import_path[1] == ':' &&
      (import_path[2] == '\\' || import_path[2] == '/')) {
    return strdup(import_path);
  }
#else
  if (import_path[0] == '/') {
    return strdup(import_path);
  }
#endif

  // Join base_path and import_path
  size_t base_len = strlen(base_path);
  size_t import_len = strlen(import_path);
  char *full_path =
      malloc(base_len + import_len + 2); // 1 for slash, 1 for null

  // Find last slash in base_path to get directory
  const char *last_slash = strrchr(base_path, '/');
  const char *last_backslash = strrchr(base_path, '\\');
  const char *last_sep =
      (last_slash > last_backslash) ? last_slash : last_backslash;

  if (last_sep) {
    size_t dir_len = last_sep - base_path + 1;
    strncpy(full_path, base_path, dir_len);
    strcpy(full_path + dir_len, import_path);
  } else {
    strcpy(full_path, import_path);
  }

  return full_path;
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
  return 0;
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
} ImportContext;

static int add_visited_file(ImportContext *ctx, const char *path) {
  for (size_t i = 0; i < ctx->count; i++) {
    if (strcmp(ctx->visited_files[i], path) == 0) {
      return 0; // Already visited
    }
  }

  if (ctx->count >= ctx->capacity) {
    ctx->capacity = ctx->capacity == 0 ? 8 : ctx->capacity * 2;
    ctx->visited_files =
        realloc(ctx->visited_files, ctx->capacity * sizeof(char *));
  }
  ctx->visited_files[ctx->count++] = strdup(path);
  return 1;
}

static void push_import_chain(ImportContext *ctx, const char *path) {
  if (ctx->chain_depth >= ctx->chain_capacity) {
    ctx->chain_capacity =
        ctx->chain_capacity == 0 ? 8 : ctx->chain_capacity * 2;
    ctx->import_chain =
        realloc(ctx->import_chain, ctx->chain_capacity * sizeof(char *));
  }
  ctx->import_chain[ctx->chain_depth++] = strdup(path);
}

static void pop_import_chain(ImportContext *ctx) {
  if (ctx->chain_depth > 0) {
    ctx->chain_depth--;
    free(ctx->import_chain[ctx->chain_depth]);
    ctx->import_chain[ctx->chain_depth] = NULL;
  }
}

// Build a human-readable import chain string like "main.masm -> utils.masm ->
// math.masm"
static char *format_import_chain(ImportContext *ctx) {
  if (ctx->chain_depth == 0)
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
      new_capacity = new_capacity == 0 ? 16 : new_capacity * 2;                \
      new_declarations =                                                       \
          realloc(new_declarations, new_capacity * sizeof(ASTNode *));         \
    }                                                                          \
    new_declarations[new_declaration_count++] = (decl_node);                   \
  } while (0)

  for (size_t i = 0; i < prog_data->declaration_count; i++) {
    ASTNode *decl = prog_data->declarations[i];
    if (decl->type == AST_IMPORT) {
      ImportDeclaration *import_decl = (ImportDeclaration *)decl->data;
      char *full_path =
          resolve_path(current_file_path, import_decl->module_name);

      if (!add_visited_file(ctx, full_path)) {
        // Circular dependency or already imported — report a warning with chain
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

      push_import_chain(ctx, import_decl->module_name);

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
              // No export keyword used at all: include everything (backwards
              // compat)
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

  // Replace old declarations in current program
  free(prog_data->declarations);
  prog_data->declarations = new_declarations;
  prog_data->declaration_count = new_declaration_count;

  // Update children array to match declarations perfectly
  free(program->children);
  if (new_declaration_count > 0) {
    program->children = malloc(new_declaration_count * sizeof(ASTNode *));
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

int resolve_imports(ASTNode *program, const char *base_path,
                    ErrorReporter *reporter) {
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

  // Register the root file as visited + push onto chain
  char *abs_base = resolve_path(base_path, "");
  add_visited_file(&ctx, abs_base);
  free(abs_base);

  // Extract filename for readable chain
  const char *last_slash = strrchr(base_path, '/');
  const char *last_backslash = strrchr(base_path, '\\');
  const char *last_sep =
      (last_slash > last_backslash) ? last_slash : last_backslash;
  const char *base_name = last_sep ? last_sep + 1 : base_path;
  push_import_chain(&ctx, base_name);

  int had_error = 0;
  process_imports_recursive(&ctx, program, base_path, &had_error, 0);

  // Cleanup
  pop_import_chain(&ctx);
  for (size_t i = 0; i < ctx.count; i++) {
    free(ctx.visited_files[i]);
  }
  free(ctx.visited_files);
  free(ctx.import_chain);

  return !had_error;
}
