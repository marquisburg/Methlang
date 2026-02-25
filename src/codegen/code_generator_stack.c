#include "code_generator_internal.h"
#include <stdlib.h>
#include <string.h>

// Stack frame management functions
void code_generator_function_prologue(CodeGenerator *generator,
                                      const char *function_name,
                                      int stack_size) {
  // Save current function name and reset stack tracking
  free(generator->current_function_name);
  if (function_name) {
    size_t len = strlen(function_name) + 1;
    generator->current_function_name = malloc(len);
    if (generator->current_function_name) {
      memcpy(generator->current_function_name, function_name, len);
    }
  } else {
    generator->current_function_name = NULL;
  }
  generator->function_stack_size = stack_size;
  generator->current_stack_offset = 0; // Reset for new function

  // Generate function label
  code_generator_emit(generator, "\n%s:\n", function_name);

  // Standard x86-64 function prologue
  code_generator_emit(generator,
                      "    push rbp        ; Save old base pointer\n");
  code_generator_emit(generator, "    mov rbp, rsp  ; Set new base pointer\n");

  // Get calling convention for callee-saved register handling
  CallingConventionSpec *conv_spec =
      generator->register_allocator->calling_convention;

  // Save callee-saved registers that will be used by this function
  // This is a simplified approach - in practice, we'd only save registers we
  // actually use
  if (conv_spec && conv_spec->callee_saved_registers) {
    code_generator_emit(generator, "    ; Save callee-saved registers\n");
    for (size_t i = 0; i < conv_spec->callee_saved_count; i++) {
      x86Register reg = conv_spec->callee_saved_registers[i];
      const char *reg_name = code_generator_get_register_name(reg);
      if (reg_name && reg != REG_RBP && reg != REG_RSP) {
        // Only save if we might use this register (simplified check)
        if (reg == REG_RBX || reg == REG_R12 || reg == REG_R13 ||
            reg == REG_R14 || reg == REG_R15) {
          code_generator_emit(
              generator, "    push %s         ; Save callee-saved register\n",
              reg_name);
          generator->function_stack_size += 8; // Track additional stack usage
        }
      }
    }
  }

  // Allocate stack space for local variables
  // Ensure 16-byte alignment for the stack frame
  int aligned_stack_size = stack_size;
  if (aligned_stack_size > 0) {
    // Round up to nearest 16-byte boundary
    aligned_stack_size = (aligned_stack_size + 15) & ~15;
    code_generator_emit(
        generator,
        "    sub rsp, %d    ; Allocate %d bytes on stack (aligned)\n",
        aligned_stack_size, aligned_stack_size);
    generator->function_stack_size = aligned_stack_size;
  }

  // Initialize local variables to zero if needed
  if (aligned_stack_size > 0) {
    code_generator_emit(generator,
                        "    ; Zero-initialize local variable space\n");
    code_generator_emit(generator,
                        "    mov rdi, rsp  ; Destination for memset\n");
    code_generator_emit(generator,
                        "    mov rax, 0     ; Value to set (zero)\n");
    code_generator_emit(generator, "    mov rcx, %d    ; Number of bytes\n",
                        aligned_stack_size);
    code_generator_emit(generator,
                        "    rep stosb         ; Zero-fill the stack space\n");
  }
}

void code_generator_function_epilogue(CodeGenerator *generator) {
  if (!generator) {
    return;
  }

  code_generator_emit(generator, "    ; Function epilogue\n");

  // Get calling convention for callee-saved register handling
  CallingConventionSpec *conv_spec =
      generator->register_allocator->calling_convention;

  // Restore stack pointer to base pointer (cleans up local variables)
  code_generator_emit(generator, "    mov rsp, rbp  ; Restore stack pointer\n");

  // Restore callee-saved registers in reverse order
  if (conv_spec && conv_spec->callee_saved_registers) {
    code_generator_emit(generator, "    ; Restore callee-saved registers\n");
    // Restore in reverse order of saving
    for (int i = (int)conv_spec->callee_saved_count - 1; i >= 0; i--) {
      x86Register reg = conv_spec->callee_saved_registers[i];
      const char *reg_name = code_generator_get_register_name(reg);
      if (reg_name && reg != REG_RBP && reg != REG_RSP) {
        // Only restore if we saved this register
        if (reg == REG_RBX || reg == REG_R12 || reg == REG_R13 ||
            reg == REG_R14 || reg == REG_R15) {
          code_generator_emit(
              generator,
              "    pop %s          ; Restore callee-saved register\n",
              reg_name);
        }
      }
    }
  }

  // Restore old base pointer
  code_generator_emit(generator,
                      "    pop rbp         ; Restore old base pointer\n");

  // Return to caller
  code_generator_emit(generator, "    ret               ; Return to caller\n");
}

