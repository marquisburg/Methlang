#include "code_generator_internal.h"
#include <stdlib.h>
#include <string.h>

#define WINDOWS_STACK_PROBE_PAGE_SIZE 4096

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

  // Callee-saved register preservation is intentionally disabled for now.
  // The current frame-slot logic assumes local slots begin at [rbp - 8].
  // Emitting pushes here shifts that layout and can corrupt frame access.

  // Allocate stack space for local variables
  // Ensure 16-byte alignment for the stack frame
  int aligned_stack_size = stack_size;
  if (aligned_stack_size > 0) {
    // Round up to nearest 16-byte boundary
    aligned_stack_size = (aligned_stack_size + 15) & ~15;

#ifdef _WIN32
    if (aligned_stack_size > WINDOWS_STACK_PROBE_PAGE_SIZE) {
      // Probe guard pages before large stack subtraction on Win64.
      // Mirrors compiler-lowered pattern (___chkstk_ms + sub rsp, rax).
      code_generator_emit(generator, "    mov rax, %d\n", aligned_stack_size);
      code_generator_emit(generator, "    extern ___chkstk_ms\n");
      code_generator_emit(generator, "    sub rsp, 32\n");
      code_generator_emit(generator, "    call ___chkstk_ms\n");
      code_generator_emit(generator, "    add rsp, 32\n");
      code_generator_emit(
          generator,
          "    sub rsp, rax    ; Allocate %d bytes on stack (probed)\n",
          aligned_stack_size);
    } else {
      code_generator_emit(
          generator,
          "    sub rsp, %d    ; Allocate %d bytes on stack (aligned)\n",
          aligned_stack_size, aligned_stack_size);
    }
#else
    code_generator_emit(
        generator,
        "    sub rsp, %d    ; Allocate %d bytes on stack (aligned)\n",
        aligned_stack_size, aligned_stack_size);
#endif
    generator->function_stack_size = aligned_stack_size;
  }

  // NOTE: Do not zero-fill the full frame in prologue.
  // Incoming argument registers are consumed immediately after prologue to
  // materialize parameter home slots. A blanket memset here clobbers those
  // registers before homing.
}

void code_generator_function_epilogue(CodeGenerator *generator) {
  if (!generator) {
    return;
  }

  code_generator_emit(generator, "    ; Function epilogue\n");

  // Restore stack pointer to base pointer (cleans up local variables)
  code_generator_emit(generator, "    mov rsp, rbp  ; Restore stack pointer\n");

  // Restore old base pointer
  code_generator_emit(generator,
                      "    pop rbp         ; Restore old base pointer\n");

  // Return to caller
  code_generator_emit(generator, "    ret               ; Return to caller\n");
}

