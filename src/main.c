#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "main.h"
#include "codegen/binary_emitter.h"
#include "linker/pe_emitter.h"
#include "string_intern.h"
#include "ir/ir.h"
#include "ir/ir_optimize.h"
#include "semantic/import_resolver.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

static int directory_exists(const char *path) {
  if (!path || path[0] == '\0') {
    return 0;
  }
#ifdef _WIN32
  struct _stat st;
  return _stat(path, &st) == 0 && (st.st_mode & _S_IFDIR) != 0;
#else
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

static char *join_paths(const char *left, const char *right) {
  if (!left || !right) {
    return NULL;
  }

  size_t left_len = strlen(left);
  size_t right_len = strlen(right);
  int has_sep = left_len > 0 &&
                (left[left_len - 1] == '/' || left[left_len - 1] == '\\');
  size_t total = left_len + right_len + (has_sep ? 1 : 2);

  char *joined = malloc(total);
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

static char *directory_from_path(const char *path) {
  if (!path || path[0] == '\0') {
    return NULL;
  }

  const char *last_slash = strrchr(path, '/');
  const char *last_backslash = strrchr(path, '\\');
  const char *last_sep =
      (last_slash > last_backslash) ? last_slash : last_backslash;
  if (!last_sep) {
    return NULL;
  }

  size_t len = (size_t)(last_sep - path);
  char *dir = malloc(len + 1);
  if (!dir) {
    return NULL;
  }

  memcpy(dir, path, len);
  dir[len] = '\0';
  return dir;
}

static char *get_executable_path(const char *argv0) {
#ifdef _WIN32
  char *program_path = NULL;
  if (_get_pgmptr(&program_path) == 0 && program_path &&
      program_path[0] != '\0') {
    return strdup(program_path);
  }
  if (argv0 && argv0[0] != '\0') {
    return strdup(argv0);
  }
  return NULL;
#elif defined(__APPLE__)
  uint32_t size = 0;
  if (_NSGetExecutablePath(NULL, &size) != -1 || size == 0) {
    return NULL;
  }
  char *buffer = malloc((size_t)size + 1);
  if (!buffer) {
    return NULL;
  }
  if (_NSGetExecutablePath(buffer, &size) != 0) {
    free(buffer);
    return NULL;
  }
  buffer[size] = '\0';
  return buffer;
#else
  char buffer[PATH_MAX + 1];
  ssize_t len = readlink("/proc/self/exe", buffer, PATH_MAX);
  if (len > 0) {
    buffer[len] = '\0';
    return strdup(buffer);
  }
  if (argv0 && argv0[0] != '\0') {
    return strdup(argv0);
  }
  return NULL;
#endif
}

static char *infer_default_stdlib_directory(const char *argv0) {
  char *exe_path = get_executable_path(argv0);
  char *exe_dir = directory_from_path(exe_path);

  if (exe_dir) {
    char *parent_dir = join_paths(exe_dir, "..");
    if (parent_dir) {
      char *packaged_stdlib = join_paths(parent_dir, "stdlib");
      free(parent_dir);
      if (packaged_stdlib && directory_exists(packaged_stdlib)) {
        free(exe_path);
        free(exe_dir);
        return packaged_stdlib;
      }
      free(packaged_stdlib);
    }

    char *local_stdlib = join_paths(exe_dir, "stdlib");
    if (local_stdlib && directory_exists(local_stdlib)) {
      free(exe_path);
      free(exe_dir);
      return local_stdlib;
    }
    free(local_stdlib);
  }

  free(exe_path);
  free(exe_dir);

  if (directory_exists("stdlib")) {
    return strdup("stdlib");
  }

  return strdup("stdlib");
}

static char *infer_default_runtime_directory(const char *argv0) {
  char *exe_path = get_executable_path(argv0);
  char *exe_dir = directory_from_path(exe_path);

  if (exe_dir) {
    char *parent_dir = join_paths(exe_dir, "..");
    if (parent_dir) {
      char *packaged_runtime = join_paths(parent_dir, "runtime");
      free(parent_dir);
      if (packaged_runtime && directory_exists(packaged_runtime)) {
        free(exe_path);
        free(exe_dir);
        return packaged_runtime;
      }
      free(packaged_runtime);
    }

    char *local_runtime = join_paths(exe_dir, "runtime");
    if (local_runtime && directory_exists(local_runtime)) {
      free(exe_path);
      free(exe_dir);
      return local_runtime;
    }
    free(local_runtime);
  }

  free(exe_path);
  free(exe_dir);

  if (directory_exists("runtime")) {
    return strdup("runtime");
  }

  return NULL;
}

static char *infer_default_docs_directory(const char *argv0) {
  char *exe_path = get_executable_path(argv0);
  char *exe_dir = directory_from_path(exe_path);

  if (exe_dir) {
    char *parent_dir = join_paths(exe_dir, "..");
    if (parent_dir) {
      char *packaged_docs = join_paths(parent_dir, "docs");
      free(parent_dir);
      if (packaged_docs && directory_exists(packaged_docs)) {
        free(exe_path);
        free(exe_dir);
        return packaged_docs;
      }
      free(packaged_docs);
    }

    char *local_docs = join_paths(exe_dir, "docs");
    if (local_docs && directory_exists(local_docs)) {
      free(exe_path);
      free(exe_dir);
      return local_docs;
    }
    free(local_docs);
  }

  free(exe_path);
  free(exe_dir);

  if (directory_exists("docs")) {
    return strdup("docs");
  }

  return NULL;
}

static void print_doc_reference(const char *argv0, const char *relative_path) {
  char *docs_dir = infer_default_docs_directory(argv0);
  if (docs_dir && relative_path) {
    char *full_path = join_paths(docs_dir, relative_path);
    if (full_path) {
      printf("Doc: %s\n", full_path);
      free(full_path);
      free(docs_dir);
      return;
    }
  }

  if (relative_path) {
    printf("Doc: docs/%s\n", relative_path);
  }
  free(docs_dir);
}

static int print_help_topic(const char *program_name, const char *argv0,
                            const char *topic) {
  if (!topic || topic[0] == '\0') {
    print_usage(program_name);
    return 0;
  }

  if (strcmp(topic, "build") == 0 || strcmp(topic, "compile") == 0) {
    printf("Build help\n");
    printf("  methlang --build app.meth -o app.exe\n");
    printf("  Builds an executable directly on Windows.\n");
    printf("  Uses NASM, then tries the selected linker backend.\n");
    printf("  Default is --linker auto (internal, then gcc, then link.exe).\n");
    printf("  Use --linker internal to force the native PE linker.\n");
    printf("  Add repeatable linker flags with --link-arg <arg>.\n");
    printf("  Example: methlang --build web\\\\server.meth -o web\\\\server.exe "
           "--linker internal --link-arg -lws2_32\n");
    print_doc_reference(argv0, "compilation.md");
    return 0;
  }

  if (strcmp(topic, "gc") == 0 || strcmp(topic, "runtime") == 0) {
    printf("GC help\n");
    printf("  The .s file contains calls to gc_alloc/gc_init, not the GC "
           "implementation itself.\n");
    printf("  methlang --build links the bundled runtime automatically.\n");
    printf("  Manual assembly/linking still requires the bundled runtime "
           "objects.\n");
    printf("  If you use new or GC-backed string concatenation, use "
           "--build or link gc.o manually.\n");
    print_doc_reference(argv0, "garbage-collector.md");
    return 0;
  }

  if (strcmp(topic, "interop") == 0 || strcmp(topic, "c") == 0) {
    printf("C interop help\n");
    printf("  Declare external C functions with extern function.\n");
    printf("  Use --link-arg for extra linker libraries in --build mode.\n");
    printf("  Example: methlang --build main.meth -o main.exe --linker internal "
           "--link-arg -lws2_32\n");
    print_doc_reference(argv0, "c-interop.md");
    return 0;
  }

  if (strcmp(topic, "stdlib") == 0) {
    printf("Stdlib help\n");
    printf("  std/... imports resolve against the bundled stdlib by default.\n");
    printf("  Override with --stdlib <dir> only when you need a custom root.\n");
    print_doc_reference(argv0, "standard-library.md");
    return 0;
  }

  if (strcmp(topic, "web") == 0) {
    printf("Web example help\n");
    printf("  Build the demo server with .\\\\web\\\\build.bat\n");
    printf("  That now delegates to methlang --build with --link-arg "
           "-lws2_32.\n");
    print_doc_reference(argv0, "compilation.md");
    return 0;
  }

  if (strcmp(topic, "docs") == 0 || strcmp(topic, "topics") == 0) {
    printf("Help topics: build, gc, interop, stdlib, web\n");
    print_doc_reference(argv0, "LANGUAGE.md");
    return 0;
  }

  fprintf(stderr, "Error: Unknown help topic '%s'\n", topic);
  fprintf(stderr, "Available topics: build, gc, interop, stdlib, web\n");
  return 1;
}

static int validate_lexical_phase(const char *source, ErrorReporter *reporter) {
  if (!source) {
    return 0;
  }

  Lexer *lexer = lexer_create(source);
  if (!lexer) {
    if (reporter) {
      error_reporter_add_error(reporter, ERROR_INTERNAL,
                               source_location_create(0, 0),
                               "Failed to initialize lexer for lexical "
                               "validation");
    }
    return 0;
  }

  int has_lexical_error = 0;

  while (1) {
    Token token = lexer_next_token(lexer);
    if (token.type == TOKEN_ERROR) {
      has_lexical_error = 1;
      if (reporter) {
        SourceLocation location =
            source_location_create(token.line, token.column);
        error_reporter_add_error(reporter, ERROR_LEXICAL, location,
                                 token.value ? token.value
                                             : "Unknown lexical error");
      }
    }

    if (token.type == TOKEN_EOF) {
      token_destroy(&token);
      break;
    }

    token_destroy(&token);
  }

  lexer_destroy(lexer);
  return !has_lexical_error;
}

static char *build_sidecar_filename(const char *base_filename,
                                    const char *suffix) {
  if (!base_filename || !suffix) {
    return NULL;
  }

  size_t base_len = strlen(base_filename);
  size_t suffix_len = strlen(suffix);
  char *path = malloc(base_len + suffix_len + 1);
  if (!path) {
    return NULL;
  }

  memcpy(path, base_filename, base_len);
  memcpy(path + base_len, suffix, suffix_len);
  path[base_len + suffix_len] = '\0';
  return path;
}

static char *replace_extension(const char *path, const char *extension) {
  if (!path || !extension) {
    return NULL;
  }

  const char *last_slash = strrchr(path, '/');
  const char *last_backslash = strrchr(path, '\\');
  const char *last_sep =
      (last_slash > last_backslash) ? last_slash : last_backslash;
  const char *last_dot = strrchr(path, '.');
  size_t stem_len =
      (last_dot && (!last_sep || last_dot > last_sep)) ? (size_t)(last_dot - path)
                                                       : strlen(path);
  size_t ext_len = strlen(extension);

  char *result = malloc(stem_len + ext_len + 1);
  if (!result) {
    return NULL;
  }

  memcpy(result, path, stem_len);
  memcpy(result + stem_len, extension, ext_len);
  result[stem_len + ext_len] = '\0';
  return result;
}

static char *default_executable_filename(const char *input_filename) {
  if (!input_filename || input_filename[0] == '\0') {
    return NULL;
  }

  return replace_extension(input_filename, ".exe");
}

static const char *linker_mode_name(LinkerMode mode) {
  switch (mode) {
  case LINKER_MODE_INTERNAL:
    return "internal";
  case LINKER_MODE_GCC:
    return "gcc";
  case LINKER_MODE_MSVC:
    return "msvc";
  case LINKER_MODE_AUTO:
  default:
    return "auto";
  }
}

static int parse_linker_mode(const char *text, LinkerMode *mode_out) {
  if (!text || !mode_out) {
    return 0;
  }

  if (strcmp(text, "auto") == 0) {
    *mode_out = LINKER_MODE_AUTO;
    return 1;
  }
  if (strcmp(text, "internal") == 0) {
    *mode_out = LINKER_MODE_INTERNAL;
    return 1;
  }
  if (strcmp(text, "gcc") == 0) {
    *mode_out = LINKER_MODE_GCC;
    return 1;
  }
  if (strcmp(text, "msvc") == 0 || strcmp(text, "link") == 0) {
    *mode_out = LINKER_MODE_MSVC;
    return 1;
  }

  return 0;
}

#ifdef _WIN32
typedef struct {
  char **items;
  size_t count;
  size_t capacity;
} StringList;

static void string_list_destroy(StringList *list) {
  size_t i = 0u;

  if (!list) {
    return;
  }

  for (i = 0u; i < list->count; i++) {
    free(list->items[i]);
  }

  free(list->items);
  memset(list, 0, sizeof(*list));
}

static int string_list_contains(const StringList *list, const char *value) {
  size_t i = 0u;

  if (!list || !value) {
    return 0;
  }

  for (i = 0u; i < list->count; i++) {
    if (list->items[i] && strcmp(list->items[i], value) == 0) {
      return 1;
    }
  }

  return 0;
}

static int string_list_append_owned(StringList *list, char *value) {
  char **grown = NULL;
  size_t new_capacity = 0u;

  if (!list || !value) {
    free(value);
    return 0;
  }
  if (string_list_contains(list, value)) {
    free(value);
    return 1;
  }

  if (list->count == list->capacity) {
    new_capacity = list->capacity ? list->capacity * 2u : 4u;
    grown = realloc(list->items, new_capacity * sizeof(char *));
    if (!grown) {
      free(value);
      return 0;
    }
    list->items = grown;
    list->capacity = new_capacity;
  }

  list->items[list->count++] = value;
  return 1;
}

static int string_list_append_copy(StringList *list, const char *value) {
  char *copy = NULL;

  if (!value) {
    return 0;
  }

  copy = strdup(value);
  if (!copy) {
    return 0;
  }

  return string_list_append_owned(list, copy);
}

static int path_exists_windows(const char *path) {
  return path && path[0] != '\0' && _access(path, 0) == 0;
}

static int text_ends_with_ignore_case(const char *text, const char *suffix) {
  size_t text_length = 0u;
  size_t suffix_length = 0u;
  size_t i = 0u;

  if (!text || !suffix) {
    return 0;
  }

  text_length = strlen(text);
  suffix_length = strlen(suffix);
  if (suffix_length > text_length) {
    return 0;
  }

  for (i = 0u; i < suffix_length; i++) {
    unsigned char left =
        (unsigned char)text[text_length - suffix_length + i];
    unsigned char right = (unsigned char)suffix[i];
    if (tolower(left) != tolower(right)) {
      return 0;
    }
  }

  return 1;
}

static char *normalize_link_library_name(const char *argument,
                                         const char *extension) {
  size_t length = 0u;
  size_t extension_length = 0u;
  char *normalized = NULL;

  if (!argument || !extension) {
    return NULL;
  }

  length = strlen(argument);
  extension_length = strlen(extension);
  if (text_ends_with_ignore_case(argument, extension)) {
    return strdup(argument);
  }

  normalized = malloc(length + extension_length + 1u);
  if (!normalized) {
    return NULL;
  }

  memcpy(normalized, argument, length);
  memcpy(normalized + length, extension, extension_length + 1u);
  return normalized;
}

static int resolve_import_library_path(const char *library_name,
                                       const StringList *search_directories,
                                       StringList *resolved_paths) {
  char *candidate = NULL;
  char *env_copy = NULL;
  char *token = NULL;

  if (!library_name || !resolved_paths) {
    return 0;
  }

  if (strchr(library_name, '\\') || strchr(library_name, '/') ||
      strchr(library_name, ':') || path_exists_windows(library_name)) {
    return string_list_append_copy(resolved_paths, library_name);
  }

  if (search_directories) {
    size_t i = 0u;
    for (i = 0u; i < search_directories->count; i++) {
      candidate = join_paths(search_directories->items[i], library_name);
      if (!candidate) {
        return 0;
      }
      if (path_exists_windows(candidate)) {
        return string_list_append_owned(resolved_paths, candidate);
      }
      free(candidate);
      candidate = NULL;
    }
  }

  env_copy = getenv("LIB") ? strdup(getenv("LIB")) : NULL;
  token = env_copy ? strtok(env_copy, ";") : NULL;
  while (token) {
    candidate = join_paths(token, library_name);
    if (!candidate) {
      free(env_copy);
      return 0;
    }
    if (path_exists_windows(candidate)) {
      free(env_copy);
      return string_list_append_owned(resolved_paths, candidate);
    }
    free(candidate);
    candidate = NULL;
    token = strtok(NULL, ";");
  }
  free(env_copy);

  return string_list_append_copy(resolved_paths, library_name);
}

static int collect_internal_link_imports(const CompilerOptions *options,
                                         int include_shell32,
                                         StringList *import_library_paths,
                                         StringList *import_dll_names,
                                         char **error_message_out) {
  size_t i = 0u;
  StringList search_directories = {0};

  if (error_message_out) {
    *error_message_out = NULL;
  }
  if (!import_library_paths || !import_dll_names) {
    return 0;
  }

  if (!string_list_append_copy(import_dll_names, "kernel32.dll") ||
      !string_list_append_copy(import_dll_names, "msvcrt.dll") ||
      (include_shell32 &&
       !string_list_append_copy(import_dll_names, "shell32.dll"))) {
    if (error_message_out) {
      *error_message_out = strdup("Out of memory while preparing internal linker defaults");
    }
    string_list_destroy(&search_directories);
    return 0;
  }

  if (!options) {
    string_list_destroy(&search_directories);
    return 1;
  }

  for (i = 0u; i < options->link_argument_count; i++) {
    const char *argument = options->link_arguments[i];

    if (!argument || argument[0] == '\0') {
      continue;
    }
    if (strncmp(argument, "-L", 2) == 0 && argument[2] != '\0') {
      if (!string_list_append_copy(&search_directories, argument + 2)) {
        if (error_message_out) {
          *error_message_out = strdup("Out of memory while storing internal linker search directories");
        }
        string_list_destroy(&search_directories);
        return 0;
      }
      continue;
    }
  }

  for (i = 0u; i < options->link_argument_count; i++) {
    const char *argument = options->link_arguments[i];
    char *normalized = NULL;

    if (!argument || argument[0] == '\0') {
      continue;
    }
    if (strncmp(argument, "-L", 2) == 0 && argument[2] != '\0') {
      continue;
    }
    if (strncmp(argument, "-l", 2) == 0 && argument[2] != '\0') {
      normalized = normalize_link_library_name(argument + 2u, ".dll");
      if (!normalized) {
        if (error_message_out) {
          *error_message_out = strdup("Out of memory while preparing internal linker DLL imports");
        }
        string_list_destroy(&search_directories);
        return 0;
      }
      if (!string_list_append_owned(import_dll_names, normalized)) {
        if (error_message_out) {
          *error_message_out = strdup("Out of memory while preparing internal linker DLL imports");
        }
        string_list_destroy(&search_directories);
        return 0;
      }
      continue;
    }
    if (text_ends_with_ignore_case(argument, ".lib")) {
      if (!resolve_import_library_path(argument, &search_directories,
                                       import_library_paths)) {
        if (error_message_out) {
          *error_message_out = strdup("Out of memory while preparing internal linker import libraries");
        }
        string_list_destroy(&search_directories);
        return 0;
      }
    }
  }

  string_list_destroy(&search_directories);
  return 1;
}

static int object_has_undefined_symbol_prefix(const char *object_path,
                                              const char *prefix) {
  CoffObject *object = NULL;
  char *error_message = NULL;
  size_t i = 0u;
  int found = 0;

  if (!object_path || !prefix) {
    return 0;
  }

  if (!coff_object_read(object_path, &object, &error_message)) {
    free(error_message);
    return 1;
  }

  for (i = 0u; i < object->symbol_count; i++) {
    const CoffSymbol *symbol = &object->symbols[i];
    if (symbol->is_auxiliary || symbol->section_number != 0 || !symbol->name) {
      continue;
    }
    if (strncmp(symbol->name, prefix, strlen(prefix)) == 0) {
      found = 1;
      break;
    }
  }

  free(error_message);
  coff_object_destroy(object);
  return found;
}

static int object_needs_gc_runtime(const char *object_path) {
  return object_has_undefined_symbol_prefix(object_path, "gc_") ||
         object_has_undefined_symbol_prefix(object_path, "meth_runtime_");
}

static int object_needs_methlang_entry(const char *object_path) {
  return object_has_undefined_symbol_prefix(object_path, "methlang_entry_");
}

static int append_argument_text(char *buffer, size_t buffer_size, size_t *offset,
                                const char *text) {
  if (!buffer || !offset || !text) {
    return 0;
  }

  size_t text_len = strlen(text);
  if (*offset + text_len >= buffer_size) {
    return 0;
  }

  memcpy(buffer + *offset, text, text_len);
  *offset += text_len;
  buffer[*offset] = '\0';
  return 1;
}

static int append_quoted_argument(char *buffer, size_t buffer_size,
                                  size_t *offset, const char *argument) {
  if (!append_argument_text(buffer, buffer_size, offset, "\"")) {
    return 0;
  }
  if (!append_argument_text(buffer, buffer_size, offset, argument)) {
    return 0;
  }
  return append_argument_text(buffer, buffer_size, offset, "\"");
}

static int append_gcc_link_arguments(char *buffer, size_t buffer_size,
                                     size_t *offset,
                                     const CompilerOptions *options) {
  if (!options) {
    return 1;
  }

  for (size_t i = 0; i < options->link_argument_count; i++) {
    const char *arg = options->link_arguments[i];
    if (!arg || arg[0] == '\0') {
      continue;
    }
    if (!append_argument_text(buffer, buffer_size, offset, " ")) {
      return 0;
    }
    if (!append_argument_text(buffer, buffer_size, offset, arg)) {
      return 0;
    }
  }

  return 1;
}

static int append_msvc_link_argument(char *buffer, size_t buffer_size,
                                     size_t *offset, const char *argument) {
  if (!argument || argument[0] == '\0') {
    return 1;
  }

  if (strncmp(argument, "-l", 2) == 0 && argument[2] != '\0') {
    if (!append_argument_text(buffer, buffer_size, offset, " ")) {
      return 0;
    }
    if (!append_argument_text(buffer, buffer_size, offset, argument + 2)) {
      return 0;
    }
    return append_argument_text(buffer, buffer_size, offset, ".lib");
  }

  if (strncmp(argument, "-L", 2) == 0 && argument[2] != '\0') {
    if (!append_argument_text(buffer, buffer_size, offset, " /LIBPATH:\"")) {
      return 0;
    }
    if (!append_argument_text(buffer, buffer_size, offset, argument + 2)) {
      return 0;
    }
    return append_argument_text(buffer, buffer_size, offset, "\"");
  }

  if (!append_argument_text(buffer, buffer_size, offset, " ")) {
    return 0;
  }
  return append_argument_text(buffer, buffer_size, offset, argument);
}

static int append_msvc_link_arguments(char *buffer, size_t buffer_size,
                                      size_t *offset,
                                      const CompilerOptions *options) {
  if (!options) {
    return 1;
  }

  for (size_t i = 0; i < options->link_argument_count; i++) {
    if (!append_msvc_link_argument(buffer, buffer_size, offset,
                                   options->link_arguments[i])) {
      return 0;
    }
  }

  return 1;
}

static int run_system_command(const char *command) {
  if (!command || command[0] == '\0') {
    return 0;
  }
  return system(command);
}

static int windows_tool_exists(const char *tool_name) {
  if (!tool_name || tool_name[0] == '\0') {
    return 0;
  }

  size_t command_len = strlen(tool_name) + 32;
  char *command = malloc(command_len);
  if (!command) {
    return 0;
  }

  snprintf(command, command_len, "where %s >nul 2>&1", tool_name);
  int result = run_system_command(command);
  free(command);
  return result == 0;
}

static int run_nasm_assemble(const char *asm_filename,
                             const char *object_filename) {
  size_t nasm_len = strlen(asm_filename) + strlen(object_filename) + 64;
  char *nasm_command = malloc(nasm_len);
  if (!nasm_command) {
    fprintf(stderr, "Error: Failed to allocate NASM command\n");
    return 1;
  }

  snprintf(nasm_command, nasm_len, "nasm -f win64 \"%s\" -o \"%s\"",
           asm_filename, object_filename);
  int result = run_system_command(nasm_command);
  free(nasm_command);
  if (result != 0) {
    fprintf(stderr, "Error: NASM assembly step failed\n");
    return 1;
  }
  return 0;
}

static int methlang_build_with_gcc(const char *object_filename,
                                   const char *executable_filename,
                                   const char *gc_object,
                                   const char *entry_object,
                                   const CompilerOptions *options) {
  size_t gcc_len = strlen(object_filename) + strlen(gc_object) +
                   strlen(executable_filename) + 160;
  if (entry_object && entry_object[0] != '\0') {
    gcc_len += strlen(entry_object);
  }
  if (options) {
    for (size_t i = 0; i < options->link_argument_count; i++) {
      if (options->link_arguments[i]) {
        gcc_len += strlen(options->link_arguments[i]) + 1;
      }
    }
  }

  char *gcc_command = malloc(gcc_len);
  if (!gcc_command) {
    fprintf(stderr, "Error: Failed to allocate GCC command\n");
    return 1;
  }

  size_t offset = 0;
  if (entry_object && entry_object[0] != '\0') {
    if (!append_argument_text(gcc_command, gcc_len, &offset,
                              "gcc -nostartfiles ") ||
        !append_quoted_argument(gcc_command, gcc_len, &offset, object_filename) ||
        !append_argument_text(gcc_command, gcc_len, &offset, " ") ||
        !append_quoted_argument(gcc_command, gcc_len, &offset, gc_object) ||
        !append_argument_text(gcc_command, gcc_len, &offset, " ") ||
        !append_quoted_argument(gcc_command, gcc_len, &offset, entry_object) ||
        !append_argument_text(gcc_command, gcc_len, &offset, " -o ") ||
        !append_quoted_argument(gcc_command, gcc_len, &offset,
                                executable_filename) ||
        !append_argument_text(gcc_command, gcc_len, &offset,
                              " -lkernel32 -lshell32") ||
        !append_gcc_link_arguments(gcc_command, gcc_len, &offset, options)) {
      free(gcc_command);
      fprintf(stderr, "Error: Failed to build GCC command\n");
      return 1;
    }
  } else {
    if (!append_argument_text(gcc_command, gcc_len, &offset,
                              "gcc -nostartfiles ") ||
        !append_quoted_argument(gcc_command, gcc_len, &offset, object_filename) ||
        !append_argument_text(gcc_command, gcc_len, &offset, " ") ||
        !append_quoted_argument(gcc_command, gcc_len, &offset, gc_object) ||
        !append_argument_text(gcc_command, gcc_len, &offset, " -o ") ||
        !append_quoted_argument(gcc_command, gcc_len, &offset,
                                executable_filename) ||
        !append_argument_text(gcc_command, gcc_len, &offset, " -lkernel32") ||
        !append_gcc_link_arguments(gcc_command, gcc_len, &offset, options)) {
      free(gcc_command);
      fprintf(stderr, "Error: Failed to build GCC command\n");
      return 1;
    }
  }

  int result = run_system_command(gcc_command);
  free(gcc_command);
  if (result != 0) {
    fprintf(stderr, "Warning: GCC link step failed\n");
    return 1;
  }
  return 0;
}

static int methlang_build_with_link(const char *object_filename,
                                    const char *executable_filename,
                                    const char *gc_object,
                                    const char *entry_object,
                                    const CompilerOptions *options) {
  size_t link_len = strlen(object_filename) + strlen(executable_filename) +
                    strlen(gc_object) + 256;
  if (entry_object && entry_object[0] != '\0') {
    link_len += strlen(entry_object) + 16;
  }
  if (options) {
    for (size_t i = 0; i < options->link_argument_count; i++) {
      if (options->link_arguments[i]) {
        link_len += strlen(options->link_arguments[i]) + 16;
      }
    }
  }

  char *link_command = malloc(link_len);
  if (!link_command) {
    fprintf(stderr, "Error: Failed to allocate MSVC link command\n");
    return 1;
  }

  size_t offset = 0;
  if (entry_object && entry_object[0] != '\0') {
    if (!append_argument_text(
            link_command, link_len, &offset,
            "link.exe /nologo /entry:mainCRTStartup /subsystem:console /out:") ||
        !append_quoted_argument(link_command, link_len, &offset,
                                executable_filename) ||
        !append_argument_text(link_command, link_len, &offset, " ") ||
        !append_quoted_argument(link_command, link_len, &offset, object_filename) ||
        !append_argument_text(link_command, link_len, &offset, " ") ||
        !append_quoted_argument(link_command, link_len, &offset, gc_object) ||
        !append_argument_text(link_command, link_len, &offset, " ") ||
        !append_quoted_argument(link_command, link_len, &offset, entry_object) ||
        !append_argument_text(link_command, link_len, &offset,
                              " kernel32.lib shell32.lib msvcrt.lib") ||
        !append_msvc_link_arguments(link_command, link_len, &offset, options)) {
      free(link_command);
      fprintf(stderr, "Error: Failed to build MSVC link command\n");
      return 1;
    }
  } else {
    if (!append_argument_text(
            link_command, link_len, &offset,
            "link.exe /nologo /entry:mainCRTStartup /subsystem:console /out:") ||
        !append_quoted_argument(link_command, link_len, &offset,
                                executable_filename) ||
        !append_argument_text(link_command, link_len, &offset, " ") ||
        !append_quoted_argument(link_command, link_len, &offset, object_filename) ||
        !append_argument_text(link_command, link_len, &offset, " ") ||
        !append_quoted_argument(link_command, link_len, &offset, gc_object) ||
        !append_argument_text(link_command, link_len, &offset,
                              " kernel32.lib msvcrt.lib") ||
        !append_msvc_link_arguments(link_command, link_len, &offset, options)) {
      free(link_command);
      fprintf(stderr, "Error: Failed to build MSVC link command\n");
      return 1;
    }
  }

  int result = run_system_command(link_command);
  free(link_command);
  if (result != 0) {
    fprintf(stderr, "Warning: MSVC link.exe step failed\n");
    return 1;
  }
  return 0;
}

static int write_internal_startup_object(const char *path) {
  BinaryEmitter *emitter = NULL;
  size_t text = 0u;
  static const unsigned char code[] = {
      0x48u, 0x83u, 0xECu, 0x28u, 0xE8u, 0x00u, 0x00u, 0x00u,
      0x00u, 0x89u, 0xC1u, 0xE8u, 0x00u, 0x00u, 0x00u, 0x00u};
  int result = 1;

  emitter = binary_emitter_create(BINARY_TARGET_FORMAT_COFF_WIN64);
  if (!emitter) {
    return 1;
  }

  text = binary_emitter_get_or_create_section(emitter, ".text", BINARY_SECTION_TEXT,
                                              0, 16u);
  if (text == (size_t)-1 ||
      !binary_emitter_append_bytes(emitter, text, code, sizeof(code), NULL) ||
      !binary_emitter_define_symbol(emitter, "mainCRTStartup", BINARY_SYMBOL_GLOBAL,
                                    text, 0u, sizeof(code)) ||
      !binary_emitter_declare_external(emitter, "main") ||
      !binary_emitter_declare_external(emitter, "ExitProcess") ||
      !binary_emitter_add_relocation(emitter, text, 5u, BINARY_RELOCATION_REL32,
                                     "main", 0) ||
      !binary_emitter_add_relocation(emitter, text, 12u, BINARY_RELOCATION_REL32,
                                     "ExitProcess", 0) ||
      !binary_emitter_write_object_file(emitter, path)) {
    goto cleanup;
  }

  result = 0;

cleanup:
  binary_emitter_destroy(emitter);
  return result;
}

static int methlang_link_internal(const char **object_paths,
                                  size_t object_count,
                                  const char *executable_filename,
                                  int include_shell32,
                                  const CompilerOptions *options) {
  LinkResolutionOptions resolution_options = {"mainCRTStartup", 16u, 1};
  LinkResolution *resolution = NULL;
  PeEmissionOptions emission_options = {0};
  StringList import_library_paths = {0};
  StringList import_dll_names = {0};
  char *error_message = NULL;
  int result = 1;

  if (!object_paths || object_count == 0u || !executable_filename) {
    fprintf(stderr, "Error: Missing inputs for internal linker\n");
    return 1;
  }

  if (!collect_internal_link_imports(options, include_shell32,
                                     &import_library_paths, &import_dll_names,
                                     &error_message)) {
    fprintf(stderr, "Error: %s\n",
            error_message ? error_message
                          : "Failed to prepare internal linker imports");
    free(error_message);
    string_list_destroy(&import_library_paths);
    string_list_destroy(&import_dll_names);
    return 1;
  }

  if (!link_resolution_build(object_paths, object_count, &resolution_options,
                             &resolution, &error_message)) {
    fprintf(stderr, "Warning: Internal linker symbol resolution failed: %s\n",
            error_message ? error_message : "unknown error");
    goto cleanup;
  }

  emission_options.import_library_paths =
      (const char **)import_library_paths.items;
  emission_options.import_library_count = import_library_paths.count;
  emission_options.import_dll_names = (const char **)import_dll_names.items;
  emission_options.import_dll_count = import_dll_names.count;
  if (!pe_emit_executable(resolution, executable_filename, &emission_options,
                          &error_message)) {
    fprintf(stderr, "Warning: Internal linker PE emission failed: %s\n",
            error_message ? error_message : "unknown error");
    goto cleanup;
  }

  result = 0;

cleanup:
  free(error_message);
  string_list_destroy(&import_library_paths);
  string_list_destroy(&import_dll_names);
  link_resolution_destroy(resolution);
  return result;
}

static int methlang_link_object_with_gcc(const char *object_filename,
                                         const char *executable_filename,
                                         const char *gc_object,
                                         const CompilerOptions *options) {
  size_t gcc_len = strlen(object_filename) + strlen(gc_object) +
                   strlen(executable_filename) + 160;
  if (options) {
    for (size_t i = 0; i < options->link_argument_count; i++) {
      if (options->link_arguments[i]) {
        gcc_len += strlen(options->link_arguments[i]) + 1;
      }
    }
  }

  char *gcc_command = malloc(gcc_len);
  if (!gcc_command) {
    fprintf(stderr, "Error: Failed to allocate GCC command\n");
    return 1;
  }

  size_t offset = 0;
  if (!append_argument_text(gcc_command, gcc_len, &offset, "gcc ") ||
      !append_quoted_argument(gcc_command, gcc_len, &offset, object_filename) ||
      !append_argument_text(gcc_command, gcc_len, &offset, " ") ||
      !append_quoted_argument(gcc_command, gcc_len, &offset, gc_object) ||
      !append_argument_text(gcc_command, gcc_len, &offset, " -o ") ||
      !append_quoted_argument(gcc_command, gcc_len, &offset,
                              executable_filename) ||
      !append_argument_text(gcc_command, gcc_len, &offset,
                            " -mconsole -lkernel32") ||
      !append_gcc_link_arguments(gcc_command, gcc_len, &offset, options)) {
    free(gcc_command);
    fprintf(stderr, "Error: Failed to build GCC object link command\n");
    return 1;
  }

  int result = run_system_command(gcc_command);
  free(gcc_command);
  if (result != 0) {
    fprintf(stderr, "Warning: GCC object link step failed\n");
    return 1;
  }
  return 0;
}

static int methlang_link_object_with_link(const char *object_filename,
                                          const char *executable_filename,
                                          const char *gc_object,
                                          const CompilerOptions *options) {
  size_t link_len = strlen(object_filename) + strlen(executable_filename) +
                    strlen(gc_object) + 256;
  if (options) {
    for (size_t i = 0; i < options->link_argument_count; i++) {
      if (options->link_arguments[i]) {
        link_len += strlen(options->link_arguments[i]) + 16;
      }
    }
  }

  char *link_command = malloc(link_len);
  if (!link_command) {
    fprintf(stderr, "Error: Failed to allocate MSVC link command\n");
    return 1;
  }

  size_t offset = 0;
  if (!append_argument_text(link_command, link_len, &offset,
                            "link.exe /nologo /subsystem:console /out:") ||
      !append_quoted_argument(link_command, link_len, &offset,
                              executable_filename) ||
      !append_argument_text(link_command, link_len, &offset, " ") ||
      !append_quoted_argument(link_command, link_len, &offset, object_filename) ||
      !append_argument_text(link_command, link_len, &offset, " ") ||
      !append_quoted_argument(link_command, link_len, &offset, gc_object) ||
      !append_argument_text(link_command, link_len, &offset,
                            " kernel32.lib msvcrt.lib") ||
      !append_msvc_link_arguments(link_command, link_len, &offset, options)) {
    free(link_command);
    fprintf(stderr, "Error: Failed to build MSVC object link command\n");
    return 1;
  }

  int result = run_system_command(link_command);
  free(link_command);
  if (result != 0) {
    fprintf(stderr, "Warning: MSVC object link step failed\n");
    return 1;
  }
  return 0;
}

static int methlang_build_executable(const char *asm_filename,
                                     const char *executable_filename,
                                     const char *runtime_directory,
                                     const CompilerOptions *options) {
  LinkerMode linker_mode =
      options ? options->linker_mode : LINKER_MODE_AUTO;
  int has_gcc = 0;
  int has_link = 0;

  if (!asm_filename || !executable_filename || !runtime_directory) {
    fprintf(stderr, "Error: Missing build inputs for executable generation\n");
    return 1;
  }

  if (!windows_tool_exists("nasm")) {
    fprintf(stderr, "Error: nasm not found in PATH. Please install NASM.\n");
    return 1;
  }

  has_gcc = (linker_mode == LINKER_MODE_AUTO || linker_mode == LINKER_MODE_GCC)
                ? windows_tool_exists("gcc")
                : 0;
  has_link =
      (linker_mode == LINKER_MODE_AUTO || linker_mode == LINKER_MODE_MSVC)
          ? windows_tool_exists("link.exe")
          : 0;
  if (linker_mode == LINKER_MODE_GCC && !has_gcc) {
    fprintf(stderr, "Error: gcc was requested with --linker gcc but was not found.\n");
    return 1;
  }
  if (linker_mode == LINKER_MODE_MSVC && !has_link) {
    fprintf(stderr,
            "Error: link.exe was requested with --linker msvc but was not found.\n");
    return 1;
  }
  char *gcc_object_filename = replace_extension(executable_filename, ".o");
  char *msvc_object_filename = replace_extension(executable_filename, ".obj");
  char *gc_gcc_object = join_paths(runtime_directory, "gc.o");
  char *entry_gcc_object = join_paths(runtime_directory, "methlang_entry.o");
  char *gc_msvc_object = join_paths(runtime_directory, "gc.obj");
  char *entry_msvc_object = join_paths(runtime_directory, "methlang_entry.obj");
  if (!gcc_object_filename || !msvc_object_filename || !gc_gcc_object ||
      !entry_gcc_object || !gc_msvc_object || !entry_msvc_object) {
    fprintf(stderr, "Error: Failed to allocate build paths\n");
    free(gcc_object_filename);
    free(msvc_object_filename);
    free(gc_gcc_object);
    free(entry_gcc_object);
    free(gc_msvc_object);
    free(entry_msvc_object);
    return 1;
  }

  if (_access(gc_gcc_object, 0) != 0 && _access(gc_msvc_object, 0) != 0) {
    fprintf(stderr,
            "Error: Bundled GC runtime object not found in '%s'\n",
            runtime_directory);
    free(gcc_object_filename);
    free(msvc_object_filename);
    free(gc_gcc_object);
    free(entry_gcc_object);
    free(gc_msvc_object);
    free(entry_msvc_object);
    return 1;
  }

  int build_result = 1;

  if (linker_mode == LINKER_MODE_INTERNAL || linker_mode == LINKER_MODE_AUTO) {
    const char *object_paths[3];
    size_t object_count = 0u;
    const char *gc_object = NULL;
    const char *entry_object = NULL;
    int needs_gc = 0;
    int needs_entry = 0;

    if (run_nasm_assemble(asm_filename, msvc_object_filename) != 0) {
      goto cleanup;
    }
    needs_gc = object_needs_gc_runtime(msvc_object_filename);
    needs_entry = object_needs_methlang_entry(msvc_object_filename);
    if (needs_gc) {
      gc_object = (_access(gc_msvc_object, 0) == 0) ? gc_msvc_object : gc_gcc_object;
    }
    if (needs_entry) {
      entry_object =
          (_access(entry_msvc_object, 0) == 0)
              ? entry_msvc_object
              : ((_access(entry_gcc_object, 0) == 0) ? entry_gcc_object : NULL);
    }

    object_paths[object_count++] = msvc_object_filename;
    if (gc_object) {
      object_paths[object_count++] = gc_object;
    }
    if (entry_object) {
      object_paths[object_count++] = entry_object;
    }

    if (methlang_link_internal(object_paths, object_count, executable_filename,
                               entry_object != NULL, options) == 0) {
      build_result = 0;
      goto cleanup;
    }

    if (linker_mode == LINKER_MODE_INTERNAL) {
      fprintf(stderr, "Error: Internal linker failed to produce an executable\n");
      goto cleanup;
    }
    if (!has_gcc && !has_link) {
      fprintf(stderr,
              "Error: Internal linker failed and no external fallback linker is "
              "available.\n");
      goto cleanup;
    }
    fprintf(stderr,
            "Warning: Internal linker failed in auto mode, falling back to "
            "external linkers\n");
  }

  if (has_gcc && linker_mode != LINKER_MODE_MSVC) {
    if (run_nasm_assemble(asm_filename, gcc_object_filename) == 0) {
      const char *entry_object =
          (_access(entry_gcc_object, 0) == 0) ? entry_gcc_object : NULL;
      if (methlang_build_with_gcc(gcc_object_filename, executable_filename,
                                  gc_gcc_object, entry_object, options) == 0) {
        build_result = 0;
        goto cleanup;
      }
    }
  }

  if (has_link && linker_mode != LINKER_MODE_GCC) {
    if (run_nasm_assemble(asm_filename, msvc_object_filename) == 0) {
      const char *gc_object =
          (_access(gc_msvc_object, 0) == 0) ? gc_msvc_object : gc_gcc_object;
      const char *entry_object =
          (_access(entry_msvc_object, 0) == 0)
              ? entry_msvc_object
              : ((_access(entry_gcc_object, 0) == 0) ? entry_gcc_object : NULL);
      if (methlang_build_with_link(msvc_object_filename, executable_filename,
                                   gc_object, entry_object, options) == 0) {
        build_result = 0;
        goto cleanup;
      }
    }
  }

  fprintf(stderr,
          "Error: Failed to link executable with the available linker backends\n");

cleanup:
  free(gcc_object_filename);
  free(msvc_object_filename);
  free(gc_gcc_object);
  free(entry_gcc_object);
  free(gc_msvc_object);
  free(entry_msvc_object);
  return build_result;
}

static int methlang_link_object_file(const char *object_filename,
                                     const char *executable_filename,
                                     const char *runtime_directory,
                                     const CompilerOptions *options) {
  LinkerMode linker_mode =
      options ? options->linker_mode : LINKER_MODE_AUTO;
  int has_gcc = 0;
  int has_link = 0;

  if (!object_filename || !executable_filename || !runtime_directory) {
    fprintf(stderr, "Error: Missing build inputs for executable generation\n");
    return 1;
  }

  has_gcc = (linker_mode == LINKER_MODE_AUTO || linker_mode == LINKER_MODE_GCC)
                ? windows_tool_exists("gcc")
                : 0;
  has_link =
      (linker_mode == LINKER_MODE_AUTO || linker_mode == LINKER_MODE_MSVC)
          ? windows_tool_exists("link.exe")
          : 0;
  if (linker_mode == LINKER_MODE_GCC && !has_gcc) {
    fprintf(stderr, "Error: gcc was requested with --linker gcc but was not found.\n");
    return 1;
  }
  if (linker_mode == LINKER_MODE_MSVC && !has_link) {
    fprintf(stderr,
            "Error: link.exe was requested with --linker msvc but was not found.\n");
    return 1;
  }
  char *gc_gcc_object = join_paths(runtime_directory, "gc.o");
  char *gc_msvc_object = join_paths(runtime_directory, "gc.obj");
  if (!gc_gcc_object || !gc_msvc_object) {
    fprintf(stderr, "Error: Failed to allocate build paths\n");
    free(gc_gcc_object);
    free(gc_msvc_object);
    return 1;
  }

  if (_access(gc_gcc_object, 0) != 0 && _access(gc_msvc_object, 0) != 0) {
    fprintf(stderr,
            "Error: Bundled GC runtime object not found in '%s'\n",
            runtime_directory);
    free(gc_gcc_object);
    free(gc_msvc_object);
    return 1;
  }

  int build_result = 1;

  if (linker_mode == LINKER_MODE_INTERNAL || linker_mode == LINKER_MODE_AUTO) {
    const char *object_paths[3];
    const char *gc_object = NULL;
    char *startup_object = replace_extension(executable_filename, ".startup.obj");
    size_t object_count = 0u;
    int needs_gc = object_needs_gc_runtime(object_filename);
    int startup_ready = 0;

    if (!startup_object) {
      if (linker_mode == LINKER_MODE_INTERNAL || (!has_gcc && !has_link)) {
        fprintf(stderr,
                "Error: Failed to allocate internal-linker startup object path\n");
        goto cleanup;
      }
      fprintf(stderr,
              "Warning: Failed to allocate internal-linker startup object path, "
              "falling back to external linkers\n");
    } else if (write_internal_startup_object(startup_object) != 0) {
      if (linker_mode == LINKER_MODE_INTERNAL || (!has_gcc && !has_link)) {
        fprintf(stderr,
                "Error: Failed to generate internal-linker startup object\n");
        free(startup_object);
        goto cleanup;
      }
      fprintf(stderr,
              "Warning: Failed to generate internal-linker startup object, "
              "falling back to external linkers\n");
    } else {
      startup_ready = 1;
    }

    if (startup_ready) {
      if (needs_gc) {
        gc_object =
            (_access(gc_msvc_object, 0) == 0) ? gc_msvc_object : gc_gcc_object;
      }

      object_paths[object_count++] = startup_object;
      object_paths[object_count++] = object_filename;
      if (gc_object) {
        object_paths[object_count++] = gc_object;
      }

      if (methlang_link_internal(object_paths, object_count, executable_filename, 0,
                                 options) == 0) {
        build_result = 0;
      } else if (linker_mode == LINKER_MODE_INTERNAL) {
        fprintf(stderr, "Error: Internal linker failed to produce an executable\n");
      } else if (!has_gcc && !has_link) {
        fprintf(stderr,
                "Error: Internal linker failed and no external fallback linker is "
                "available.\n");
      } else {
        fprintf(stderr,
                "Warning: Internal linker failed in auto mode, falling back to "
                "external linkers\n");
      }
    }

    if (startup_object) {
      if (startup_ready) {
        _unlink(startup_object);
      }
      free(startup_object);
    }

    if (build_result == 0 || linker_mode == LINKER_MODE_INTERNAL ||
        (!has_gcc && !has_link)) {
      goto cleanup;
    }
  }

  if (has_gcc && linker_mode != LINKER_MODE_MSVC) {
    if (methlang_link_object_with_gcc(object_filename, executable_filename,
                                      gc_gcc_object, options) == 0) {
      build_result = 0;
      goto cleanup;
    }
  }

  if (has_link && linker_mode != LINKER_MODE_GCC) {
    const char *gc_object =
        (_access(gc_msvc_object, 0) == 0) ? gc_msvc_object : gc_gcc_object;
    if (methlang_link_object_with_link(object_filename, executable_filename,
                                       gc_object, options) == 0) {
      build_result = 0;
      goto cleanup;
    }
  }

  fprintf(stderr,
          "Error: Failed to link executable with the available linker backends\n");

cleanup:
  free(gc_gcc_object);
  free(gc_msvc_object);
  return build_result;
}
#endif

static int add_import_directory(CompilerOptions *options, const char *path) {
  if (!options || !path || path[0] == '\0') {
    return 0;
  }

  size_t next_count = options->import_directory_count + 1;
  const char **grown = realloc((void *)options->import_directories,
                               next_count * sizeof(const char *));
  if (!grown) {
    return 0;
  }

  grown[options->import_directory_count] = path;
  options->import_directories = grown;
  options->import_directory_count = next_count;
  return 1;
}

static int add_link_argument(CompilerOptions *options, const char *argument) {
  if (!options || !argument || argument[0] == '\0') {
    return 0;
  }

  size_t next_count = options->link_argument_count + 1;
  const char **grown = realloc((void *)options->link_arguments,
                               next_count * sizeof(const char *));
  if (!grown) {
    return 0;
  }

  grown[options->link_argument_count] = argument;
  options->link_arguments = grown;
  options->link_argument_count = next_count;
  return 1;
}

int main(int argc, char *argv[]) {
  CompilerOptions options = {0};
  char *auto_stdlib_directory = NULL;
  char *auto_runtime_directory = NULL;
  char *build_output_filename = NULL;
  char *assembly_output_filename = NULL;
  char *object_output_filename = NULL;
  int build_executable = 0;
  int output_filename_explicit = 0;
  options.output_filename = "output.s"; // Default output filename
  options.debug_format = "dwarf";

  if (argc >= 2) {
    if (strcmp(argv[1], "help") == 0) {
      return print_help_topic(argv[0], argv[0], argc >= 3 ? argv[2] : NULL);
    }
    if (strcmp(argv[1], "docs") == 0) {
      if (argc >= 3) {
        return print_help_topic(argv[0], argv[0], argv[2]);
      }
      printf("Methlang documentation topics: build, gc, interop, stdlib, web\n");
      print_doc_reference(argv[0], "LANGUAGE.md");
      print_doc_reference(argv[0], "compilation.md");
      print_doc_reference(argv[0], "garbage-collector.md");
      return 0;
    }
  }

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      options.input_filename = argv[++i];
    } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
      options.output_filename = argv[++i];
      output_filename_explicit = 1;
    } else if (strcmp(argv[i], "-I") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: Missing import directory after '-I'\n");
        return 1;
      }
      if (!add_import_directory(&options, argv[++i])) {
        fprintf(stderr, "Error: Failed to add import directory\n");
        return 1;
      }
    } else if (strncmp(argv[i], "-I", 2) == 0 && argv[i][2] != '\0') {
      if (!add_import_directory(&options, argv[i] + 2)) {
        fprintf(stderr, "Error: Failed to add import directory\n");
        return 1;
      }
    } else if (strcmp(argv[i], "--stdlib") == 0 && i + 1 < argc) {
      options.stdlib_directory = argv[++i];
    } else if (strcmp(argv[i], "--build") == 0) {
      build_executable = 1;
    } else if (strcmp(argv[i], "--emit-obj") == 0) {
      options.emit_object = 1;
    } else if (strcmp(argv[i], "--linker") == 0 && i + 1 < argc) {
      if (!parse_linker_mode(argv[++i], &options.linker_mode)) {
        fprintf(stderr,
                "Error: Unknown linker mode '%s' (expected auto, internal, gcc, or msvc)\n",
                argv[i]);
        return 1;
      }
    } else if (strcmp(argv[i], "--linker") == 0) {
      fprintf(stderr, "Error: Missing linker mode after '--linker'\n");
      return 1;
    } else if (strcmp(argv[i], "--link-arg") == 0 && i + 1 < argc) {
      if (!add_link_argument(&options, argv[++i])) {
        fprintf(stderr, "Error: Failed to add linker argument\n");
        return 1;
      }
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
      options.debug_mode = 1;
      options.generate_debug_symbols = 1;
      options.generate_line_mapping = 1;
      options.generate_stack_trace_support = 1;
    } else if (strcmp(argv[i], "-g") == 0 ||
               strcmp(argv[i], "--debug-symbols") == 0) {
      options.generate_debug_symbols = 1;
    } else if (strcmp(argv[i], "-l") == 0 ||
               strcmp(argv[i], "--line-mapping") == 0) {
      options.generate_line_mapping = 1;
    } else if (strcmp(argv[i], "-s") == 0 ||
               strcmp(argv[i], "--stack-trace") == 0) {
      options.generate_stack_trace_support = 1;
    } else if (strcmp(argv[i], "--debug-format") == 0 && i + 1 < argc) {
      options.debug_format = argv[++i];
    } else if (strcmp(argv[i], "-O") == 0 ||
               strcmp(argv[i], "--optimize") == 0) {
      options.optimize = 1;
    } else if (strcmp(argv[i], "-r") == 0 ||
               strcmp(argv[i], "--release") == 0) {
      options.release = 1;
      options.optimize = 1;
      options.strip_asm_comments = 1;
    } else if (strcmp(argv[i], "--strip-comments") == 0) {
      options.strip_asm_comments = 1;
    } else if (strcmp(argv[i], "--prelude") == 0) {
      options.prelude = 1;
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    } else if (!options.input_filename) {
      options.input_filename = argv[i];
    } else {
      fprintf(stderr, "Error: Unknown or misplaced argument '%s'\n", argv[i]);
      print_usage(argv[0]);
      return 1;
    }
  }

  if (!options.input_filename) {
    fprintf(stderr, "Error: No input file specified.\n");
    print_usage(argv[0]);
    free((void *)options.import_directories);
    free((void *)options.link_arguments);
    return 1;
  }

  if (!output_filename_explicit && options.emit_object) {
    options.output_filename = "output.obj";
  }

  if (!options.stdlib_directory) {
    auto_stdlib_directory = infer_default_stdlib_directory(argv[0]);
    if (auto_stdlib_directory) {
      options.stdlib_directory = auto_stdlib_directory;
    }
  }

  auto_runtime_directory = infer_default_runtime_directory(argv[0]);

  if (build_executable) {
#ifndef _WIN32
    fprintf(stderr,
            "Error: --build is currently supported only on Windows\n");
    free((void *)options.import_directories);
    free((void *)options.link_arguments);
    free(auto_stdlib_directory);
    free(auto_runtime_directory);
    return 1;
#else
    if (!auto_runtime_directory) {
      fprintf(stderr,
              "Error: Could not locate bundled runtime directory for --build\n");
      free((void *)options.import_directories);
      free((void *)options.link_arguments);
      free(auto_stdlib_directory);
      free(auto_runtime_directory);
      return 1;
    }

    if (output_filename_explicit) {
      build_output_filename = strdup(options.output_filename);
    } else {
      build_output_filename = default_executable_filename(options.input_filename);
    }
    if (!build_output_filename) {
      fprintf(stderr, "Error: Failed to determine executable output path\n");
      free((void *)options.import_directories);
      free((void *)options.link_arguments);
      free(auto_stdlib_directory);
      free(auto_runtime_directory);
      return 1;
    }

    if (options.emit_object) {
      object_output_filename = replace_extension(build_output_filename, ".obj");
      if (!object_output_filename) {
        fprintf(stderr, "Error: Failed to determine object output path\n");
        free(build_output_filename);
        free((void *)options.import_directories);
        free((void *)options.link_arguments);
        free(auto_stdlib_directory);
        free(auto_runtime_directory);
        return 1;
      }
      options.output_filename = object_output_filename;
    } else {
      assembly_output_filename = replace_extension(build_output_filename, ".s");
      if (!assembly_output_filename) {
        fprintf(stderr, "Error: Failed to determine assembly output path\n");
        free(build_output_filename);
        free((void *)options.import_directories);
        free((void *)options.link_arguments);
        free(auto_stdlib_directory);
        free(auto_runtime_directory);
        return 1;
      }
      options.output_filename = assembly_output_filename;
    }
#endif
  }

  int result =
      compile_file(options.input_filename, options.output_filename, &options);
  if (result == 0 && build_executable) {
#ifdef _WIN32
    if (options.emit_object) {
      result = methlang_link_object_file(options.output_filename,
                                         build_output_filename,
                                         auto_runtime_directory, &options);
    } else {
      result = methlang_build_executable(options.output_filename,
                                         build_output_filename,
                                         auto_runtime_directory, &options);
    }
    if (result == 0) {
      printf("Built executable '%s'\n", build_output_filename);
    }
#endif
  } else if (result == 0 && auto_runtime_directory && !options.debug_mode) {
    fprintf(stderr,
            "Note: bundled runtime detected at '%s'. Use --build to assemble "
            "and link the packaged GC/runtime automatically.\n",
            auto_runtime_directory);
  }
  free((void *)options.import_directories);
  free((void *)options.link_arguments);
  free(auto_stdlib_directory);
  free(auto_runtime_directory);
  free(build_output_filename);
  free(assembly_output_filename);
  free(object_output_filename);
  string_intern_clear();
  return result;
}

int compile_file(const char *input_filename, const char *output_filename,
                 CompilerOptions *options) {
  // Read input file
  char *source = read_file(input_filename);
  if (!source) {
    fprintf(stderr, "Error: Could not read file '%s'\n", input_filename);
    return 1;
  }

  ErrorReporter *error_reporter = error_reporter_create(input_filename, source);
  if (!error_reporter) {
    fprintf(stderr, "Error: Could not initialize error reporter\n");
    free(source);
    return 1;
  }

  if (!validate_lexical_phase(source, error_reporter)) {
    error_reporter_print_errors(error_reporter);
    error_reporter_destroy(error_reporter);
    free(source);
    return 1;
  }

  // Initialize compiler components
  Lexer *lexer = lexer_create(source);
  Parser *parser = NULL;
  SymbolTable *symbol_table = symbol_table_create();
  TypeChecker *type_checker = NULL;
  RegisterAllocator *register_allocator = register_allocator_create();
  ASTNode *program = NULL;

  // Initialize debug info if debug mode is enabled
  DebugInfo *debug_info = NULL;
  CodeGenerator *code_generator = NULL;
  IRProgram *ir_program = NULL;
  char *ir_error_message = NULL;

  if (!lexer || !symbol_table || !register_allocator) {
    error_reporter_add_error(error_reporter, ERROR_INTERNAL,
                             source_location_create(0, 0),
                             "Failed to initialize compiler components");
    error_reporter_print_errors(error_reporter);
    if (lexer)
      lexer_destroy(lexer);
    if (symbol_table)
      symbol_table_destroy(symbol_table);
    if (register_allocator)
      register_allocator_destroy(register_allocator);
    error_reporter_destroy(error_reporter);
    free(source);
    return 1;
  }

  parser = parser_create_with_error_reporter(lexer, error_reporter);
  type_checker =
      type_checker_create_with_error_reporter(symbol_table, error_reporter);
  if (!parser || !type_checker) {
    error_reporter_add_error(error_reporter, ERROR_INTERNAL,
                             source_location_create(0, 0),
                             "Failed to initialize parser or type checker");
    error_reporter_print_errors(error_reporter);
    if (parser)
      parser_destroy(parser);
    if (type_checker)
      type_checker_destroy(type_checker);
    register_allocator_destroy(register_allocator);
    symbol_table_destroy(symbol_table);
    lexer_destroy(lexer);
    error_reporter_destroy(error_reporter);
    free(source);
    return 1;
  }

  if (options->debug_mode || options->generate_debug_symbols ||
      options->generate_line_mapping || options->generate_stack_trace_support) {
    debug_info = debug_info_create(input_filename, output_filename);
    if (!debug_info) {
      error_reporter_add_error(error_reporter, ERROR_INTERNAL,
                               source_location_create(0, 0),
                               "Failed to initialize debug information");
      error_reporter_print_errors(error_reporter);
      parser_destroy(parser);
      type_checker_destroy(type_checker);
      register_allocator_destroy(register_allocator);
      symbol_table_destroy(symbol_table);
      lexer_destroy(lexer);
      error_reporter_destroy(error_reporter);
      free(source);
      return 1;
    }
    code_generator = code_generator_create_with_debug(
        symbol_table, type_checker, register_allocator, debug_info);
  } else {
    code_generator =
        code_generator_create(symbol_table, type_checker, register_allocator);
  }

  if (!code_generator) {
    error_reporter_add_error(error_reporter, ERROR_INTERNAL,
                             source_location_create(0, 0),
                             "Failed to initialize code generator");
    error_reporter_print_errors(error_reporter);
    parser_destroy(parser);
    type_checker_destroy(type_checker);
    register_allocator_destroy(register_allocator);
    symbol_table_destroy(symbol_table);
    lexer_destroy(lexer);
    if (debug_info)
      debug_info_destroy(debug_info);
    error_reporter_destroy(error_reporter);
    free(source);
    return 1;
  }

  code_generator_set_emit_asm_comments(code_generator,
                                       options->strip_asm_comments ? 0 : 1);
  code_generator_set_eliminate_unreachable_functions(
      code_generator, options->release ? 1 : 0);

  int result = 0;

  if (options->emit_object) {
    if (options->debug_mode || options->generate_debug_symbols ||
        options->generate_line_mapping ||
        options->generate_stack_trace_support) {
      fprintf(stderr,
              "Error: direct object emission does not yet support debug "
              "metadata or runtime trace instrumentation\n");
      result = 1;
      goto cleanup;
    }
    code_generator_set_backend_mode(code_generator,
                                    CODEGEN_BACKEND_BINARY_OBJECT);
  }

  // Parse the source code
  program = parser_parse_program(parser);
  if (!program || parser->had_error ||
      error_reporter_has_errors(error_reporter)) {
    if (error_reporter_has_errors(error_reporter)) {
      error_reporter_print_errors(error_reporter);
    } else {
      fprintf(stderr, "Parse error: %s\n",
              parser->error_message ? parser->error_message : "Unknown error");
    }
    result = 1;
    goto cleanup;
  }

  // Resolve imports (flatten imported module ASTs into the main program)
  ImportResolverOptions import_options = {0};
  if (options) {
    import_options.import_directories = options->import_directories;
    import_options.import_directory_count = options->import_directory_count;
    import_options.stdlib_directory =
        (options->stdlib_directory && options->stdlib_directory[0] != '\0')
            ? options->stdlib_directory
            : "stdlib";
  } else {
    import_options.stdlib_directory = "stdlib";
  }

  // Auto-inject the standard prelude only when --prelude was specified.
  if (options->prelude) {
    Program *prog_data = (Program *)program->data;
    SourceLocation prelude_loc = {0, 0};
    ASTNode *prelude_import =
        ast_create_import_declaration("std/prelude", prelude_loc);
    if (prelude_import) {
      // Prepend the prelude import before all user declarations.
      ASTNode **grown =
          realloc(prog_data->declarations,
                  (prog_data->declaration_count + 1) * sizeof(ASTNode *));
      if (grown) {
        memmove(grown + 1, grown,
                prog_data->declaration_count * sizeof(ASTNode *));
        grown[0] = prelude_import;
        prog_data->declarations = grown;
        prog_data->declaration_count++;
        ast_add_child(program, prelude_import);
      } else {
        ast_destroy_node(prelude_import);
      }
    }
  }

  if (!resolve_imports_with_options(program, input_filename, error_reporter,
                                    &import_options)) {
    if (error_reporter_has_errors(error_reporter)) {
      error_reporter_print_errors(error_reporter);
    } else {
      fprintf(stderr, "Import resolution error\n");
    }
    result = 1;
    goto cleanup;
  }

  // Monomorphize generics (before type checking)
  monomorphize_program(program);

  // Type checking
  if (!type_checker_check_program(type_checker, program)) {
    if (error_reporter_has_errors(error_reporter)) {
      error_reporter_print_errors(error_reporter);
    } else {
      fprintf(stderr, "Type error: %s\n",
              type_checker->error_message ? type_checker->error_message
                                          : "Unknown error");
    }
    result = 1;
    goto cleanup;
  }

  // Lower to the compiler IR before backend code generation.
  int emit_runtime_checks = options->release ? 0 : 1;
  ir_program =
      ir_lower_program(program, type_checker, symbol_table, &ir_error_message,
                       emit_runtime_checks);
  if (!ir_program) {
    fprintf(stderr, "IR lowering error: %s\n",
            ir_error_message ? ir_error_message : "Unknown error");
    result = 1;
    goto cleanup;
  }

  if (options->optimize) {
    if (!ir_optimize_program(ir_program)) {
      fprintf(stderr, "IR optimization error\n");
      result = 1;
      goto cleanup;
    }
  }

  code_generator_set_ir_program(code_generator, ir_program);

  if (options->debug_mode || (options->optimize && !options->release)) {
    char *ir_output = build_sidecar_filename(output_filename, ".ir");
    if (!ir_output) {
      fprintf(stderr,
              "Warning: Failed to allocate IR output filename for '%s'\n",
              output_filename);
    } else {
      FILE *ir_file = fopen(ir_output, "w");
      if (!ir_file) {
        fprintf(stderr, "Warning: Could not create IR file '%s': %s\n",
                ir_output, strerror(errno));
      } else {
        if (!ir_program_dump(ir_program, ir_file)) {
          fprintf(stderr, "Warning: Failed to write IR dump to '%s'\n",
                  ir_output);
        }
        fclose(ir_file);
        if (options->debug_mode) {
          printf("Generated IR dump: %s\n", ir_output);
        }
      }
      free(ir_output);
    }
  }

  // Generate code
  if (!code_generator_generate_program(code_generator, program)) {
    fprintf(stderr, "Code generation error: %s\n",
            (code_generator && code_generator->error_message)
                ? code_generator->error_message
                : "Unknown error");
    result = 1;
    goto cleanup;
  }

  if (options->emit_object) {
    BinaryEmitter *binary_emitter =
        code_generator_get_binary_emitter(code_generator);
    if (!binary_emitter_write_object_file(binary_emitter, output_filename)) {
      fprintf(stderr, "Error: Could not create object file '%s': %s\n",
              output_filename,
              binary_emitter_get_error(binary_emitter)
                  ? binary_emitter_get_error(binary_emitter)
                  : "Unknown error");
      result = 1;
      goto cleanup;
    }
  } else {
    // Write output file
    FILE *output_file = fopen(output_filename, "w");
    if (!output_file) {
      fprintf(stderr, "Error: Could not create output file '%s': %s\n",
              output_filename, strerror(errno));
      result = 1;
      goto cleanup;
    }

    char *generated_code = code_generator_get_output(code_generator);
    fprintf(output_file, "%s", generated_code);
    fclose(output_file);
  }

  // Generate debug information files if requested
  if (debug_info) {
    if (options->debug_mode || options->generate_debug_symbols ||
        options->generate_line_mapping) {
      const char *format =
          (options->debug_format && options->debug_format[0] != '\0')
              ? options->debug_format
              : "dwarf";
      const char *suffix = ".dwarf";

      if (strcasecmp(format, "stabs") == 0) {
        suffix = ".stabs";
      } else if (strcasecmp(format, "map") == 0) {
        suffix = ".map";
      } else if (strcasecmp(format, "dwarf") != 0) {
        fprintf(stderr,
                "Warning: Unknown debug format '%s', defaulting to dwarf\n",
                format);
      }

      char *debug_output = build_sidecar_filename(output_filename, suffix);
      if (!debug_output) {
        fprintf(stderr,
                "Error: Failed to allocate debug output filename for '%s'\n",
                output_filename);
        result = 1;
        goto cleanup;
      }

      if (strcasecmp(format, "stabs") == 0) {
        debug_info_generate_stabs(debug_info, debug_output);
      } else if (strcasecmp(format, "map") == 0) {
        debug_info_generate_debug_map(debug_info, debug_output);
      } else {
        debug_info_generate_dwarf(debug_info, debug_output);
      }

      if (options->debug_mode) {
        printf("Generated debug info: %s\n", debug_output);
      }
      free(debug_output);
    }

    if (options->generate_stack_trace_support && options->debug_mode) {
      printf("Embedded runtime stack trace support enabled\n");
    }
  }

  if (options->debug_mode) {
    if (error_reporter->count > 0) {
      error_reporter_print_errors(error_reporter);
    }
    printf("Successfully compiled '%s' to '%s'\n", input_filename,
           output_filename);
  } else if (error_reporter->count > 0) {
    // Surface non-fatal diagnostics (e.g. circular/duplicate import warnings)
    // even on successful compilation.
    error_reporter_print_errors(error_reporter);
  }

cleanup:
  // Clean up resources
  if (program)
    ast_destroy_node(program);
  if (ir_program)
    ir_program_destroy(ir_program);
  free(ir_error_message);
  code_generator_destroy(code_generator);
  register_allocator_destroy(register_allocator);
  type_checker_destroy(type_checker);
  symbol_table_destroy(symbol_table);
  parser_destroy(parser);
  lexer_destroy(lexer);
  if (debug_info)
    debug_info_destroy(debug_info);
  error_reporter_destroy(error_reporter);
  free(source);

  return result;
}

void print_usage(const char *program_name) {
  printf("Usage: %s [options] <input.meth>\n", program_name);
  printf("       %s help [topic]\n", program_name);
  printf("       %s docs [topic]\n", program_name);
  printf("Options:\n");
  printf("  -i <file>           Input file\n");
  printf("  -o <file>           Output file (default: output.s, or output.obj "
         "with --emit-obj)\n");
  printf("  -I <dir>            Add import search directory (repeatable)\n");
  printf("  --stdlib <dir>      Set stdlib root directory (default: auto-detect "
         "bundled stdlib, then ./stdlib)\n");
  printf("  --build             Compile, assemble, and link to an executable "
         "(Windows)\n");
  printf("  --emit-obj          Emit a Win64 COFF object directly "
         "(experimental subset)\n");
  printf("  --linker <mode>     Linker backend: auto, internal, gcc, or msvc "
         "(default: %s)\n",
         linker_mode_name(LINKER_MODE_AUTO));
  printf("  --link-arg <arg>    Pass an extra linker argument (repeatable; "
         "use with --build)\n");
  printf("  -d, --debug         Enable debug output and symbols\n");
  printf("  -g, --debug-symbols Generate debug symbols\n");
  printf("  -l, --line-mapping  Generate source line mapping\n");
  printf("  -s, --stack-trace   Embed runtime crash traceback support\n");
  printf("  --debug-format <fmt> Debug format: dwarf, stabs, or map (default: "
         "dwarf)\n");
  printf("  -O, --optimize      Enable optimizations\n");
  printf("  -r, --release       Optimize for size (enables -O, strips comments, "
         "and drops unreachable functions)\n");
  printf("  --strip-comments    Omit emitted assembly comments\n");
  printf("  --prelude           Auto-import the standard prelude (std/io, "
         "std/net, etc.)\n");
  printf("  -h, --help          Show this help message\n");
  printf("Topics: build, gc, interop, stdlib, web\n");
}

char *read_file(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    return NULL;
  }

  // Get file size
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return NULL;
  }
  long size = ftell(file);
  if (size < 0) {
    fclose(file);
    return NULL;
  }
  if (fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return NULL;
  }

  // Allocate buffer and read file
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
