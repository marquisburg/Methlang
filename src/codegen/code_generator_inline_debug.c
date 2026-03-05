#include "code_generator_internal.h"

static void code_generator_emit_debug_string(CodeGenerator *generator,
                                             const char *label,
                                             const char *value) {
  if (!generator || !label) {
    return;
  }

  code_generator_emit_to_global_buffer(generator, "%s:\n", label);
  if (!value || value[0] == '\0') {
    code_generator_emit_to_global_buffer(generator, "    db 0\n");
    return;
  }
  code_generator_emit_escaped_string_bytes(generator, value, 1);
}

static void code_generator_record_runtime_location(CodeGenerator *generator,
                                                   size_t source_line,
                                                   size_t source_column,
                                                   const char *filename,
                                                   const char *label) {
  if (!generator || !generator->debug_info || !label || !generator->current_function_name ||
      source_line == 0) {
    return;
  }

  debug_info_add_runtime_location_mapping(
      generator->debug_info, generator->current_function_name, label, filename,
      source_line, source_column);
}

// Inline assembly implementation functions
void code_generator_generate_inline_assembly(CodeGenerator *generator,
                                             ASTNode *inline_asm) {
  if (!generator || !inline_asm || inline_asm->type != AST_INLINE_ASM) {
    return;
  }

  InlineAsm *asm_data = (InlineAsm *)inline_asm->data;
  if (!asm_data || !asm_data->assembly_code) {
    return;
  }

  code_generator_emit(generator, "    ; Begin inline assembly block\n");

  // Preserve registers that might be clobbered by inline assembly
  code_generator_preserve_registers_for_inline_asm(generator);

  // Emit the inline assembly code directly
  code_generator_emit(generator, "%s\n", asm_data->assembly_code);

  // Restore preserved registers
  code_generator_restore_registers_after_inline_asm(generator);

  code_generator_emit(generator, "    ; End inline assembly block\n");
}

void code_generator_preserve_registers_for_inline_asm(
    CodeGenerator *generator) {
  if (!generator) {
    return;
  }

  code_generator_emit(generator,
                      "    ; Preserve registers for inline assembly\n");

  // Preserve caller-saved registers that might be used by surrounding code
  // This is a conservative approach - in a full implementation, we'd analyze
  // the inline assembly to determine which registers it actually uses
  code_generator_emit(generator, "    push rax           ; Preserve RAX\n");
  code_generator_emit(generator, "    push rcx           ; Preserve RCX\n");
  code_generator_emit(generator, "    push rdx           ; Preserve RDX\n");
  code_generator_emit(generator, "    push r8            ; Preserve R8\n");
  code_generator_emit(generator, "    push r9            ; Preserve R9\n");
  code_generator_emit(generator, "    push r10           ; Preserve R10\n");
  code_generator_emit(generator, "    push r11           ; Preserve R11\n");
}

void code_generator_restore_registers_after_inline_asm(
    CodeGenerator *generator) {
  if (!generator) {
    return;
  }

  code_generator_emit(generator,
                      "    ; Restore registers after inline assembly\n");

  // Restore in reverse order
  code_generator_emit(generator, "    pop r11            ; Restore R11\n");
  code_generator_emit(generator, "    pop r10            ; Restore R10\n");
  code_generator_emit(generator, "    pop r9             ; Restore R9\n");
  code_generator_emit(generator, "    pop r8             ; Restore R8\n");
  code_generator_emit(generator, "    pop rdx            ; Restore RDX\n");
  code_generator_emit(generator, "    pop rcx            ; Restore RCX\n");
  code_generator_emit(generator, "    pop rax            ; Restore RAX\n");
}

// Debug info integration functions
void code_generator_add_debug_symbol(CodeGenerator *generator, const char *name,
                                     DebugSymbolType type,
                                     const char *type_name, size_t line,
                                     size_t column) {
  if (!generator || !generator->debug_info || !name) {
    return;
  }

  debug_info_add_symbol(generator->debug_info, name, type, type_name, line,
                        column);
}

void code_generator_add_line_mapping(CodeGenerator *generator,
                                     size_t source_line, size_t source_column,
                                     const char *filename) {
  if (!generator || !generator->debug_info) {
    return;
  }

  debug_info_add_line_mapping(generator->debug_info, source_line, source_column,
                              generator->current_assembly_line, filename);
}

void code_generator_emit_debug_label(CodeGenerator *generator,
                                     size_t source_line) {
  if (!generator || !generator->generate_debug_info) {
    return;
  }
  (void)source_line;

  char *label = code_generator_generate_label(generator, "dbg_line");
  if (!label) {
    return;
  }
  code_generator_emit(generator, "%s:\n", label);
  free(label);
}

void code_generator_emit_runtime_location_marker(CodeGenerator *generator,
                                                 size_t source_line,
                                                 size_t source_column,
                                                 const char *filename) {
  if (!generator || !generator->debug_info || source_line == 0) {
    return;
  }

  if (generator->last_runtime_location_line == source_line &&
      generator->last_runtime_location_column == source_column) {
    return;
  }

  char *label = code_generator_generate_label(generator, "methdbg_loc");
  if (!label) {
    return;
  }

  code_generator_emit(generator, "%s:\n", label);
  code_generator_record_runtime_location(generator, source_line, source_column,
                                         filename, label);
  generator->last_runtime_location_line = source_line;
  generator->last_runtime_location_column = source_column;
  free(label);
}

void code_generator_add_runtime_function_mapping(CodeGenerator *generator,
                                                 const char *function_name,
                                                 const char *start_label,
                                                 const char *end_label,
                                                 size_t source_line,
                                                 size_t source_column,
                                                 const char *filename) {
  if (!generator || !generator->debug_info || !function_name || !start_label ||
      !end_label) {
    return;
  }

  debug_info_add_runtime_function_mapping(generator->debug_info, function_name,
                                          start_label, end_label, filename,
                                          source_line, source_column);
}

void code_generator_emit_runtime_debug_tables(CodeGenerator *generator) {
  if (!generator || !generator->debug_info) {
    return;
  }

  DebugInfo *debug_info = generator->debug_info;
  if (debug_info->runtime_function_count == 0 &&
      debug_info->runtime_location_count == 0) {
    return;
  }

  code_generator_emit_to_global_buffer(
      generator, "\n; Embedded Meth runtime debug metadata\n");
  code_generator_emit_to_global_buffer(generator, "align 8\n");
  char **function_name_labels = NULL;
  char **function_file_labels = NULL;
  char **location_name_labels = NULL;
  char **location_file_labels = NULL;

  if (debug_info->runtime_function_count > 0) {
    function_name_labels =
        calloc(debug_info->runtime_function_count, sizeof(char *));
    function_file_labels =
        calloc(debug_info->runtime_function_count, sizeof(char *));
    if (!function_name_labels || !function_file_labels) {
      free(function_name_labels);
      free(function_file_labels);
      return;
    }
  }

  if (debug_info->runtime_location_count > 0) {
    location_name_labels =
        calloc(debug_info->runtime_location_count, sizeof(char *));
    location_file_labels =
        calloc(debug_info->runtime_location_count, sizeof(char *));
    if (!location_name_labels || !location_file_labels) {
      free(function_name_labels);
      free(function_file_labels);
      free(location_name_labels);
      free(location_file_labels);
      return;
    }
  }

  for (size_t i = 0; i < debug_info->runtime_function_count; i++) {
    RuntimeFunctionMapping *mapping = &debug_info->runtime_functions[i];
    function_name_labels[i] =
        code_generator_generate_label(generator, "methdbg_func_name");
    function_file_labels[i] =
        code_generator_generate_label(generator, "methdbg_func_file");
    if (!function_name_labels[i] || !function_file_labels[i]) {
      goto cleanup;
    }

    code_generator_emit_debug_string(generator, function_name_labels[i],
                                     mapping->function_name);
    code_generator_emit_debug_string(generator, function_file_labels[i],
                                     mapping->filename);
  }

  for (size_t i = 0; i < debug_info->runtime_location_count; i++) {
    RuntimeLocationMapping *mapping = &debug_info->runtime_locations[i];
    location_name_labels[i] =
        code_generator_generate_label(generator, "methdbg_loc_name");
    location_file_labels[i] =
        code_generator_generate_label(generator, "methdbg_loc_file");
    if (!location_name_labels[i] || !location_file_labels[i]) {
      goto cleanup;
    }

    code_generator_emit_debug_string(generator, location_name_labels[i],
                                     mapping->function_name);
    code_generator_emit_debug_string(generator, location_file_labels[i],
                                     mapping->filename);
  }

  code_generator_emit_to_global_buffer(generator, "meth_debug_functions:\n");
  for (size_t i = 0; i < debug_info->runtime_function_count; i++) {
    RuntimeFunctionMapping *mapping = &debug_info->runtime_functions[i];
    code_generator_emit_to_global_buffer(
        generator,
        "    dq %s, %s, %s, %s, %zu, %zu\n",
        mapping->start_label, mapping->end_label, function_name_labels[i],
        function_file_labels[i], mapping->line, mapping->column);
  }

  code_generator_emit_to_global_buffer(generator, "meth_debug_locations:\n");
  for (size_t i = 0; i < debug_info->runtime_location_count; i++) {
    RuntimeLocationMapping *mapping = &debug_info->runtime_locations[i];
    code_generator_emit_to_global_buffer(
        generator,
        "    dq %s, %s, %s, %zu, %zu\n",
        mapping->address_label, location_name_labels[i], location_file_labels[i],
        mapping->line, mapping->column);
  }

cleanup:
  if (function_name_labels) {
    for (size_t i = 0; i < debug_info->runtime_function_count; i++) {
      free(function_name_labels[i]);
      free(function_file_labels[i]);
    }
  }
  if (location_name_labels) {
    for (size_t i = 0; i < debug_info->runtime_location_count; i++) {
      free(location_name_labels[i]);
      free(location_file_labels[i]);
    }
  }
  free(function_name_labels);
  free(function_file_labels);
  free(location_name_labels);
  free(location_file_labels);
}

