#include "code_generator_internal.h"
#include <ctype.h>
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
  generator->emit_asm_comments = 1;
  generator->eliminate_unreachable_functions = 0;
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

void code_generator_set_emit_asm_comments(CodeGenerator *generator, int enable) {
  if (!generator) {
    return;
  }
  generator->emit_asm_comments = enable ? 1 : 0;
}

void code_generator_set_eliminate_unreachable_functions(
    CodeGenerator *generator, int enable) {
  if (!generator) {
    return;
  }
  generator->eliminate_unreachable_functions = enable ? 1 : 0;
}

static char *code_generator_strip_asm_comments(const char *text) {
  if (!text) {
    return NULL;
  }

  size_t length = strlen(text);
  char *clean = malloc(length + 1);
  if (!clean) {
    return NULL;
  }

  size_t src = 0;
  size_t dst = 0;
  while (src < length) {
    size_t line_start = src;
    size_t line_end = src;
    while (line_end < length && text[line_end] != '\n') {
      line_end++;
    }

    size_t first_non_space = line_start;
    while (first_non_space < line_end &&
           (text[first_non_space] == ' ' || text[first_non_space] == '\t')) {
      first_non_space++;
    }

    if (!(first_non_space < line_end && text[first_non_space] == ';')) {
      int in_quotes = 0;
      size_t comment_start = line_end;
      int has_comment = 0;
      for (size_t i = line_start; i < line_end; i++) {
        if (text[i] == '"') {
          in_quotes = !in_quotes;
          continue;
        }
        if (!in_quotes && text[i] == ';') {
          comment_start = i;
          has_comment = 1;
          break;
        }
      }

      size_t copy_end = comment_start;
      if (has_comment) {
        while (copy_end > line_start &&
               (text[copy_end - 1] == ' ' || text[copy_end - 1] == '\t')) {
          copy_end--;
        }
      }

      if (copy_end > line_start) {
        size_t chunk = copy_end - line_start;
        memcpy(clean + dst, text + line_start, chunk);
        dst += chunk;
      }
    }

    if (line_end < length && text[line_end] == '\n') {
      clean[dst++] = '\n';
      src = line_end + 1;
    } else {
      src = line_end;
    }
  }

  clean[dst] = '\0';
  return clean;
}

static int code_generator_append_text(CodeGenerator *generator, const char *text,
                                      size_t text_len, int is_global_buffer) {
  if (!generator || !text) {
    return 0;
  }

  char **target_buffer = is_global_buffer ? &generator->global_variables_buffer
                                          : &generator->output_buffer;
  size_t *target_size =
      is_global_buffer ? &generator->global_variables_size : &generator->buffer_size;
  size_t *target_capacity = is_global_buffer ? &generator->global_variables_capacity
                                             : &generator->buffer_capacity;

  if (*target_size + text_len + 1 > *target_capacity) {
    size_t new_capacity = (*target_capacity == 0) ? 1024 : (*target_capacity * 2);
    while (new_capacity < *target_size + text_len + 1) {
      new_capacity *= 2;
    }

    char *new_buffer = realloc(*target_buffer, new_capacity);
    if (!new_buffer) {
      code_generator_set_error(generator,
                               "Out of memory while expanding output buffer");
      return 0;
    }

    *target_buffer = new_buffer;
    *target_capacity = new_capacity;
  }

  memcpy(*target_buffer + *target_size, text, text_len);
  *target_size += text_len;
  (*target_buffer)[*target_size] = '\0';

  if (!is_global_buffer && generator->generate_debug_info) {
    for (size_t i = 0; i < text_len; i++) {
      if (text[i] == '\n') {
        generator->current_assembly_line++;
      }
    }
  }

  return 1;
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
  if (required_size < 0) {
    va_end(args);
    code_generator_set_error(generator, "Failed to format assembly output");
    return;
  }

  char *rendered = malloc((size_t)required_size + 1);
  if (!rendered) {
    va_end(args);
    code_generator_set_error(generator,
                             "Out of memory while formatting assembly output");
    return;
  }

  vsnprintf(rendered, (size_t)required_size + 1, format, args);

  va_end(args);

  const char *to_append = rendered;
  char *cleaned = NULL;
  if (!generator->emit_asm_comments) {
    cleaned = code_generator_strip_asm_comments(rendered);
    if (!cleaned) {
      free(rendered);
      code_generator_set_error(generator,
                               "Out of memory while stripping assembly comments");
      return;
    }
    to_append = cleaned;
  }

  code_generator_append_text(generator, to_append, strlen(to_append), 0);

  free(cleaned);
  free(rendered);
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
  if (required_size < 0) {
    va_end(args);
    code_generator_set_error(generator, "Failed to format global output");
    return;
  }

  char *rendered = malloc((size_t)required_size + 1);
  if (!rendered) {
    va_end(args);
    code_generator_set_error(generator,
                             "Out of memory while formatting global output");
    return;
  }
  vsnprintf(rendered, (size_t)required_size + 1, format, args);

  va_end(args);

  // Global data (db/dq payloads) is emitted incrementally in small chunks.
  // Do not strip comments here; filtering at chunk granularity can corrupt
  // literal ';' bytes inside quoted data (for example URLs).
  code_generator_append_text(generator, rendered, strlen(rendered), 1);

  free(rendered);
}

