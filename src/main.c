#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "main.h"
#include "ir/ir.h"
#include "ir/ir_optimize.h"
#include "semantic/import_resolver.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char *argv[]) {
  CompilerOptions options = {0};
  options.output_filename = "output.s"; // Default output filename
  options.debug_format = "dwarf";

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      options.input_filename = argv[++i];
    } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
      options.output_filename = argv[++i];
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
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
      options.debug_mode = 1;
      options.generate_debug_symbols = 1;
      options.generate_line_mapping = 1;
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
    return 1;
  }

  int result =
      compile_file(options.input_filename, options.output_filename, &options);
  free((void *)options.import_directories);
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

  int result = 0;

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
  ir_program =
      ir_lower_program(program, type_checker, symbol_table, &ir_error_message);
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

  if (options->debug_mode || options->optimize) {
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

    if (options->generate_stack_trace_support) {
      char *stack_trace_output =
          build_sidecar_filename(output_filename, ".stacktrace.s");
      if (!stack_trace_output) {
        fprintf(
            stderr,
            "Error: Failed to allocate stack trace output filename for '%s'\n",
            output_filename);
        result = 1;
        goto cleanup;
      }
      debug_info_generate_stack_trace_code(debug_info, stack_trace_output);
      if (options->debug_mode) {
        printf("Generated stack trace support: %s\n", stack_trace_output);
      }
      free(stack_trace_output);
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
  printf("Usage: %s [options] <input.masm>\n", program_name);
  printf("Options:\n");
  printf("  -i <file>           Input file\n");
  printf("  -o <file>           Output file (default: output.s)\n");
  printf("  -I <dir>            Add import search directory (repeatable)\n");
  printf("  --stdlib <dir>      Set stdlib root directory (default: stdlib)\n");
  printf("  -d, --debug         Enable debug output and symbols\n");
  printf("  -g, --debug-symbols Generate debug symbols\n");
  printf("  -l, --line-mapping  Generate source line mapping\n");
  printf("  -s, --stack-trace   Generate stack trace support\n");
  printf("  --debug-format <fmt> Debug format: dwarf, stabs, or map (default: "
         "dwarf)\n");
  printf("  -O, --optimize      Enable optimizations\n");
  printf("  --prelude           Auto-import the standard prelude (std/io, "
         "std/net, etc.)\n");
  printf("  -h, --help          Show this help message\n");
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
