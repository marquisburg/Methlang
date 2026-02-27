import os

filepath = r"g:\Projects\MethASM\src\codegen\code_generator_ops.c"
with open(filepath, "r") as f:
    content = f.read()

target1 = """  Type *left_type = code_generator_infer_expression_type(generator, left);
  Type *right_type = code_generator_infer_expression_type(generator, right);
  int is_float = code_generator_is_floating_point_type(left_type) ||
                 code_generator_is_floating_point_type(right_type);

  if (is_float) {"""

repl1 = """  Type *left_type = code_generator_infer_expression_type(generator, left);
  Type *right_type = code_generator_infer_expression_type(generator, right);

  if (left_type == generator->type_checker->builtin_string &&
      right_type == generator->type_checker->builtin_string &&
      strcmp(op, "+") == 0) {
      
      code_generator_emit(generator, "    ; String concatenation (+)\\n");
      code_generator_generate_expression(generator, left);
      code_generator_emit(generator, "    push rax           ; Save left string ptr\\n");
  
      code_generator_generate_expression(generator, right);
      code_generator_emit(generator, "    mov r10, rax       ; right string ptr -> r10\\n");
      code_generator_emit(generator, "    pop rax            ; left string ptr -> rax\\n");
      
      // Calculate total length
      code_generator_emit(generator, "    mov rcx, [rax + 8] ; len1\\n");
      code_generator_emit(generator, "    add rcx, [r10 + 8] ; len1 + len2\\n");

      // Save ptrs and length
      code_generator_emit(generator, "    push r10           ; right ptr\\n");
      code_generator_emit(generator, "    push rax           ; left ptr\\n");
      code_generator_emit(generator, "    push rcx           ; total_len\\n");
      
      // gc_alloc(total_len + 17)
      const char *size_register = "rdi";
      CallingConventionSpec *conv_spec = generator->register_allocator ? generator->register_allocator->calling_convention : NULL;
      if (conv_spec && conv_spec->int_param_count > 0) {
        const char *cand = code_generator_get_register_name(conv_spec->int_param_registers[0]);
        if (cand) size_register = cand;
      }
      code_generator_emit(generator, "    mov %s, rcx\\n", size_register);
      code_generator_emit(generator, "    add %s, 17\\n", size_register);
      
      // Call with proper alignment
      code_generator_emit(generator, "    push rsp\\n    push [rsp]\\n    and rsp, -16\\n");
      if (conv_spec && conv_spec->convention == CALLING_CONV_MS_X64) {
        code_generator_emit(generator, "    sub rsp, %d\\n", conv_spec->shadow_space_size);
      }
      code_generator_emit(generator, "    extern gc_alloc\\n    call gc_alloc\\n");
      if (conv_spec && conv_spec->convention == CALLING_CONV_MS_X64) {
        code_generator_emit(generator, "    add rsp, %d\\n", conv_spec->shadow_space_size);
      }
      code_generator_emit(generator, "    mov rsp, [rsp + 8]\\n");
      
      // Restore values
      code_generator_emit(generator, "    pop rcx            ; total_len\\n");
      code_generator_emit(generator, "    pop rdx            ; left ptr (used to be rax)\\n");
      code_generator_emit(generator, "    pop rsi            ; right ptr (used to be r10)\\n");
      
      // Populate struct
      code_generator_emit(generator, "    lea r8, [rax + 16]\\n");
      code_generator_emit(generator, "    mov [rax], r8\\n");
      code_generator_emit(generator, "    mov [rax + 8], rcx\\n");
      
      // Generate labels
      char *label_left_done = code_generator_generate_label(generator, "concat_left_done");
      char *label_left_loop = code_generator_generate_label(generator, "concat_left_loop");
      char *label_right_done = code_generator_generate_label(generator, "concat_right_done");
      char *label_right_loop = code_generator_generate_label(generator, "concat_right_loop");

      // Copy left
      code_generator_emit(generator, "    mov r9, [rdx + 8]  ; left len\\n");
      code_generator_emit(generator, "    mov rdi, [rdx]     ; left chars\\n");
      code_generator_emit(generator, "    test r9, r9\\n");
      code_generator_emit(generator, "    jz %s\\n", label_left_done);
      code_generator_emit(generator, "%s:\\n", label_left_loop);
      code_generator_emit(generator, "    mov r11b, [rdi]\\n");
      code_generator_emit(generator, "    mov [r8], r11b\\n");
      code_generator_emit(generator, "    inc rdi\\n    inc r8\\n    dec r9\\n");
      code_generator_emit(generator, "    jnz %s\\n", label_left_loop);
      code_generator_emit(generator, "%s:\\n", label_left_done);
      
      // Copy right
      code_generator_emit(generator, "    mov r9, [rsi + 8]  ; right len\\n");
      code_generator_emit(generator, "    mov rdi, [rsi]     ; right chars\\n");
      code_generator_emit(generator, "    test r9, r9\\n");
      code_generator_emit(generator, "    jz %s\\n", label_right_done);
      code_generator_emit(generator, "%s:\\n", label_right_loop);
      code_generator_emit(generator, "    mov r11b, [rdi]\\n");
      code_generator_emit(generator, "    mov [r8], r11b\\n");
      code_generator_emit(generator, "    inc rdi\\n    inc r8\\n    dec r9\\n");
      code_generator_emit(generator, "    jnz %s\\n", label_right_loop);
      code_generator_emit(generator, "%s:\\n", label_right_done);
      
      // null term
      code_generator_emit(generator, "    mov byte [r8], 0\\n");
      
      free(label_left_done); free(label_left_loop);
      free(label_right_done); free(label_right_loop);
      return;
  }

  int is_float = code_generator_is_floating_point_type(left_type) ||
                 code_generator_is_floating_point_type(right_type);

  if (is_float) {"""

assert target1 in content
content = content.replace(target1, repl1)

target2 = """      } else {
        code_generator_emit(generator, "    %s rax, r10      ; %s operation\\n",
                            instruction, op);
      }
    } else {
      // Handle comparison and logical operators"""

repl2 = """      } else if (strcmp(op, "<<") == 0) {
        code_generator_emit(generator, "    mov rcx, r10       ; Move shift amount to CL\\n");
        code_generator_emit(generator, "    shl rax, cl        ; Shift left\\n");
      } else if (strcmp(op, ">>") == 0) {
        code_generator_emit(generator, "    mov rcx, r10       ; Move shift amount to CL\\n");
        code_generator_emit(generator, "    sar rax, cl        ; Shift right arithmetic\\n");
      } else {
        code_generator_emit(generator, "    %s rax, r10      ; %s operation\\n",
                            instruction, op);
      }
    } else {
      // Handle comparison and logical operators"""

assert target2 in content
content = content.replace(target2, repl2)

target3 = """    if (strcmp(op, "&") == 0)
      return "and"; // Bitwise AND
    if (strcmp(op, "|") == 0)
      return "or"; // Bitwise OR
    if (strcmp(op, "^") == 0)
      return "xor"; // Bitwise XOR
    return NULL;"""

repl3 = """    if (strcmp(op, "&") == 0)
      return "and"; // Bitwise AND
    if (strcmp(op, "|") == 0)
      return "or"; // Bitwise OR
    if (strcmp(op, "^") == 0)
      return "xor"; // Bitwise XOR
    if (strcmp(op, "<<") == 0)
      return "shl"; // Handled specifically in code_generator_emit
    if (strcmp(op, ">>") == 0)
      return "sar"; // Handled specifically in code_generator_emit
    return NULL;"""

assert target3 in content
content = content.replace(target3, repl3)

with open(filepath, "w") as f:
    f.write(content)

print("Patched code_generator_ops.c successfully!")
