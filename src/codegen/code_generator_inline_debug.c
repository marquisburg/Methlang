#include "code_generator_internal.h"

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

  code_generator_emit(generator, "L%zu:\n", source_line);
  generator->current_assembly_line++;
}

