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
  generator->last_runtime_location_line = 0;
  generator->last_runtime_location_column = 0;

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
      char *probe_label = code_generator_generate_label(generator, "stack_probe");
      if (!probe_label) {
        code_generator_set_error(generator,
                                 "Out of memory while creating stack probe label");
        return;
      }

      // Probe guard pages before large stack subtraction on Win64 without
      // depending on libgcc's ___chkstk_ms symbol.
      code_generator_emit(generator, "    mov rax, %d\n", aligned_stack_size);
      code_generator_emit(generator, "%s:\n", probe_label);
      code_generator_emit(generator, "    cmp rax, %d\n",
                          WINDOWS_STACK_PROBE_PAGE_SIZE);
      code_generator_emit(generator, "    jbe %s_done\n", probe_label);
      code_generator_emit(generator, "    sub rsp, %d\n",
                          WINDOWS_STACK_PROBE_PAGE_SIZE);
      code_generator_emit(generator, "    test byte [rsp], 0\n");
      code_generator_emit(generator, "    sub rax, %d\n",
                          WINDOWS_STACK_PROBE_PAGE_SIZE);
      code_generator_emit(generator, "    jmp %s\n", probe_label);
      code_generator_emit(generator, "%s_done:\n", probe_label);
      code_generator_emit(
          generator,
          "    sub rsp, rax    ; Allocate remaining stack bytes (probed)\n");
      code_generator_emit(generator, "    test byte [rsp], 0\n");
      code_generator_emit(generator,
                          "    ; Allocated %d bytes on stack (probed)\n",
          aligned_stack_size);
      free(probe_label);
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

void code_generator_function_epilogue(CodeGenerator *generator,
                                      Type *return_type) {
  if (!generator) {
    return;
  }

  code_generator_emit(generator, "    ; Function epilogue\n");

  if (return_type && code_generator_is_floating_point_type(return_type)) {
    if (return_type->kind == TYPE_FLOAT32) {
      code_generator_emit(
          generator, "    movd xmm0, eax  ; Float return value in xmm0\n");
    } else {
      code_generator_emit(
          generator, "    movq xmm0, rax  ; Float return value in xmm0\n");
    }
  }

  // Restore stack pointer to base pointer (cleans up local variables)
  code_generator_emit(generator, "    mov rsp, rbp  ; Restore stack pointer\n");

  // Restore old base pointer
  code_generator_emit(generator,
                      "    pop rbp         ; Restore old base pointer\n");

  // Return to caller
  code_generator_emit(generator, "    ret               ; Return to caller\n");
}

