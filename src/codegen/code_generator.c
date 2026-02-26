#include "code_generator_internal.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CodeGenerator *code_generator_create(SymbolTable *symbol_table,
                                     TypeChecker *type_checker,
                                     RegisterAllocator *allocator) {
  CodeGenerator *generator = malloc(sizeof(CodeGenerator));
  if (!generator)
    return NULL;

  generator->symbol_table = symbol_table;
  generator->type_checker = type_checker;
  generator->register_allocator = allocator;
  generator->debug_info = NULL;
  generator->output_file = NULL;
  generator->output_buffer = malloc(4096);
  generator->buffer_size = 0;
  generator->buffer_capacity = 4096;
  generator->current_label_id = 0;
  generator->current_stack_offset = 0;
  generator->function_stack_size = 0;
  generator->current_function_name = NULL;
  generator->global_variables_buffer = malloc(2048);
  generator->global_variables_size = 0;
  generator->global_variables_capacity = 2048;
  generator->current_assembly_line = 1;
  generator->break_label_stack = NULL;
  generator->continue_label_stack = NULL;
  generator->control_flow_stack_size = 0;
  generator->control_flow_stack_capacity = 0;
  generator->generate_debug_info = 0;
  generator->has_error = 0;
  generator->error_message = NULL;
  generator->ir_program = NULL;
  generator->extern_symbols = NULL;
  generator->extern_symbol_count = 0;
  generator->extern_symbol_capacity = 0;

  if (!generator->output_buffer || !generator->global_variables_buffer) {
    free(generator->output_buffer);
    free(generator->global_variables_buffer);
    free(generator);
    return NULL;
  }

  generator->output_buffer[0] = '\0';
  generator->global_variables_buffer[0] = '\0';

  return generator;
}

CodeGenerator *code_generator_create_with_debug(SymbolTable *symbol_table,
                                                TypeChecker *type_checker,
                                                RegisterAllocator *allocator,
                                                DebugInfo *debug_info) {
  CodeGenerator *generator =
      code_generator_create(symbol_table, type_checker, allocator);
  if (!generator)
    return NULL;

  generator->debug_info = debug_info;
  generator->generate_debug_info = 1;

  return generator;
}

void code_generator_destroy(CodeGenerator *generator) {
  if (generator) {
    for (size_t i = 0; i < generator->control_flow_stack_size; i++) {
      free(generator->break_label_stack[i]);
      free(generator->continue_label_stack[i]);
    }
    free(generator->break_label_stack);
    free(generator->continue_label_stack);
    free(generator->output_buffer);
    free(generator->current_function_name);
    free(generator->global_variables_buffer);
    free(generator->error_message);
    if (generator->extern_symbols) {
      for (size_t i = 0; i < generator->extern_symbol_count; i++) {
        free(generator->extern_symbols[i]);
      }
    }
    free(generator->extern_symbols);
    free(generator);
  }
}

void code_generator_set_ir_program(CodeGenerator *generator,
                                   IRProgram *ir_program) {
  if (!generator) {
    return;
  }
  generator->ir_program = ir_program;
}

void code_generator_set_error(CodeGenerator *generator,
                                     const char *format, ...) {
  if (!generator || !format) {
    return;
  }

  if (generator->has_error) {
    return;
  }

  generator->has_error = 1;
  free(generator->error_message);
  generator->error_message = NULL;

  va_list args;
  va_start(args, format);

  va_list args_copy;
  va_copy(args_copy, args);
  int size = vsnprintf(NULL, 0, format, args_copy);
  va_end(args_copy);

  if (size > 0) {
    generator->error_message = malloc((size_t)size + 1);
    if (generator->error_message) {
      vsnprintf(generator->error_message, (size_t)size + 1, format, args);
    }
  }

  va_end(args);
}

void code_generator_emit(CodeGenerator *generator, const char *format, ...) {
  if (!generator || !format || generator->has_error) {
    return;
  }

  va_list args;
  va_start(args, format);

  // Calculate required size
  va_list args_copy;
  va_copy(args_copy, args);
  int required_size = vsnprintf(NULL, 0, format, args_copy);
  va_end(args_copy);

  // Ensure buffer has enough space
  if (generator->buffer_size + required_size + 1 > generator->buffer_capacity) {
    size_t new_capacity = generator->buffer_capacity * 2;
    while (new_capacity < generator->buffer_size + required_size + 1) {
      new_capacity *= 2;
    }

    char *new_buffer = realloc(generator->output_buffer, new_capacity);
    if (!new_buffer) {
      code_generator_set_error(generator,
                               "Out of memory while expanding output buffer");
      va_end(args);
      return;
    }

    generator->output_buffer = new_buffer;
    generator->buffer_capacity = new_capacity;
  }

  // Append to buffer
  int written = vsnprintf(generator->output_buffer + generator->buffer_size,
                          generator->buffer_capacity - generator->buffer_size,
                          format, args);

  if (written > 0) {
    generator->buffer_size += written;

    // Count newlines to track assembly line numbers for debug info
    if (generator->generate_debug_info) {
      const char *text =
          generator->output_buffer + generator->buffer_size - written;
      for (int i = 0; i < written; i++) {
        if (text[i] == '\n') {
          generator->current_assembly_line++;
        }
      }
    }
  }

  va_end(args);
}

char *code_generator_get_output(CodeGenerator *generator) {
  return generator->output_buffer;
}

const char *code_generator_get_link_symbol_name(CodeGenerator *generator,
                                                const char *symbol_name) {
  if (!symbol_name || symbol_name[0] == '\0') {
    return NULL;
  }
  if (!generator || !generator->symbol_table) {
    return symbol_name;
  }

  Symbol *symbol = symbol_table_lookup(generator->symbol_table, symbol_name);
  if (symbol && symbol->is_extern && symbol->link_name &&
      symbol->link_name[0] != '\0') {
    return symbol->link_name;
  }
  return symbol_name;
}

int code_generator_emit_extern_symbol(CodeGenerator *generator,
                                      const char *symbol_name) {
  if (!generator || !symbol_name || symbol_name[0] == '\0') {
    return 0;
  }

  for (size_t i = 0; i < generator->extern_symbol_count; i++) {
    if (generator->extern_symbols[i] &&
        strcmp(generator->extern_symbols[i], symbol_name) == 0) {
      return 1;
    }
  }

  if (generator->extern_symbol_count >= generator->extern_symbol_capacity) {
    size_t new_capacity = generator->extern_symbol_capacity == 0
                              ? 8
                              : generator->extern_symbol_capacity * 2;
    char **new_items = realloc(generator->extern_symbols,
                               new_capacity * sizeof(char *));
    if (!new_items) {
      code_generator_set_error(generator,
                               "Out of memory while tracking extern symbols");
      return 0;
    }
    generator->extern_symbols = new_items;
    generator->extern_symbol_capacity = new_capacity;
  }

  generator->extern_symbols[generator->extern_symbol_count] =
      strdup(symbol_name);
  if (!generator->extern_symbols[generator->extern_symbol_count]) {
    code_generator_set_error(generator,
                             "Out of memory while storing extern symbol name");
    return 0;
  }
  generator->extern_symbol_count++;
  code_generator_emit(generator, "    extern %s\n", symbol_name);
  return !generator->has_error;
}

int code_generator_emit_escaped_string_bytes(CodeGenerator *generator,
                                             const char *value,
                                             int include_null_terminator) {
  if (!generator || !value) {
    return 0;
  }

  code_generator_emit_to_global_buffer(generator, "    db ");

  int emitted_component = 0;
  int in_quoted_segment = 0;

  const unsigned char *bytes = (const unsigned char *)value;
  while (*bytes) {
    unsigned char c = *bytes++;
    int is_printable_safe = (c >= 32 && c <= 126 && c != '"' && c != '\\');

    if (is_printable_safe) {
      if (!in_quoted_segment) {
        if (emitted_component) {
          code_generator_emit_to_global_buffer(generator, ", ");
        }
        code_generator_emit_to_global_buffer(generator, "\"");
        in_quoted_segment = 1;
      }
      code_generator_emit_to_global_buffer(generator, "%c", c);
      emitted_component = 1;
      continue;
    }

    if (in_quoted_segment) {
      code_generator_emit_to_global_buffer(generator, "\"");
      in_quoted_segment = 0;
    }

    if (emitted_component) {
      code_generator_emit_to_global_buffer(generator, ", ");
    }
    code_generator_emit_to_global_buffer(generator, "%u", (unsigned int)c);
    emitted_component = 1;
  }

  if (in_quoted_segment) {
    code_generator_emit_to_global_buffer(generator, "\"");
  }

  if (include_null_terminator) {
    if (emitted_component) {
      code_generator_emit_to_global_buffer(generator, ", ");
    }
    code_generator_emit_to_global_buffer(generator, "0");
    emitted_component = 1;
  }

  if (!emitted_component) {
    // Empty non-terminated strings are still emitted as a zero byte.
    code_generator_emit_to_global_buffer(generator, "0");
  }

  code_generator_emit_to_global_buffer(generator, "\n");
  return !generator->has_error;
}


char *code_generator_generate_label(CodeGenerator *generator,
                                    const char *prefix) {
  char *label = malloc(64);
  if (label) {
    snprintf(label, 64, "L%s%d", prefix, generator->current_label_id++);
  }
  return label;
}

// Assembly helper functions
void code_generator_emit_data_section(CodeGenerator *generator) {
  code_generator_emit(generator, "section .data\n");
  code_generator_emit(generator, "; Global variables will be placed here\n\n");
}

void code_generator_emit_text_section(CodeGenerator *generator) {
  code_generator_emit(generator, "section .text\n");
  code_generator_emit(generator, "; Code section\n\n");
}

void code_generator_emit_global_symbol(CodeGenerator *generator,
                                       const char *symbol) {
  code_generator_emit(generator, "global %s\n", symbol);
}

void code_generator_emit_instruction(CodeGenerator *generator,
                                     const char *mnemonic,
                                     const char *operands) {
  if (operands && strlen(operands) > 0) {
    code_generator_emit(generator, "    %s %s\n", mnemonic, operands);
  } else {
    code_generator_emit(generator, "    %s\n", mnemonic);
  }
}

// Helper function to emit to global variables buffer
void code_generator_emit_to_global_buffer(CodeGenerator *generator, const char *format,
                                  ...) {
  if (!generator || !format || generator->has_error) {
    return;
  }

  va_list args;
  va_start(args, format);

  // Calculate required size
  va_list args_copy;
  va_copy(args_copy, args);
  int required_size = vsnprintf(NULL, 0, format, args_copy);
  va_end(args_copy);

  // Ensure buffer has enough space
  if (generator->global_variables_size + required_size + 1 >
      generator->global_variables_capacity) {
    size_t new_capacity = generator->global_variables_capacity * 2;
    while (new_capacity <
           generator->global_variables_size + required_size + 1) {
      new_capacity *= 2;
    }

    char *new_buffer =
        realloc(generator->global_variables_buffer, new_capacity);
    if (!new_buffer) {
      code_generator_set_error(
          generator, "Out of memory while expanding global buffer");
      va_end(args);
      return;
    }

    generator->global_variables_buffer = new_buffer;
    generator->global_variables_capacity = new_capacity;
  }

  // Append to buffer
  int written = vsnprintf(
      generator->global_variables_buffer + generator->global_variables_size,
      generator->global_variables_capacity - generator->global_variables_size,
      format, args);

  if (written > 0) {
    generator->global_variables_size += written;
  }

  va_end(args);
}

