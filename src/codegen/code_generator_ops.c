#include "code_generator_internal.h"
#include <stdlib.h>
#include <string.h>

static int code_generator_get_type_storage_size(Type *type);
static void code_generator_emit_store_value_at_address(CodeGenerator *generator,
                                                       int element_size);
static void
code_generator_emit_load_value_from_address(CodeGenerator *generator,
                                            int element_size);
static void code_generator_emit_runtime_trap(CodeGenerator *generator,
                                             const char *message);
static void code_generator_emit_null_check(CodeGenerator *generator,
                                           const char *context);
static void code_generator_emit_bounds_check(CodeGenerator *generator,
                                             Type *array_type);
static int code_generator_generate_array_element_address(
    CodeGenerator *generator, ASTNode *array_expr, ASTNode *index_expr);
static int code_generator_generate_lvalue_address(CodeGenerator *generator,
                                                  ASTNode *target,
                                                  Type **out_target_type);

static int code_generator_is_signed_integer_type(Type *type) {
  if (!type) {
    return 0;
  }

  switch (type->kind) {
  case TYPE_INT8:
  case TYPE_INT16:
  case TYPE_INT32:
  case TYPE_INT64:
    return 1;
  default:
    return 0;
  }
}

const char *code_generator_get_subregister_name(x86Register reg,
                                                 int width_bits) {
  switch (reg) {
  case REG_RAX:
    return (width_bits == 8)   ? "al"
           : (width_bits == 16) ? "ax"
           : (width_bits == 32) ? "eax"
                                : "rax";
  case REG_RBX:
    return (width_bits == 8)   ? "bl"
           : (width_bits == 16) ? "bx"
           : (width_bits == 32) ? "ebx"
                                : "rbx";
  case REG_RCX:
    return (width_bits == 8)   ? "cl"
           : (width_bits == 16) ? "cx"
           : (width_bits == 32) ? "ecx"
                                : "rcx";
  case REG_RDX:
    return (width_bits == 8)   ? "dl"
           : (width_bits == 16) ? "dx"
           : (width_bits == 32) ? "edx"
                                : "rdx";
  case REG_RSI:
    return (width_bits == 8)   ? "sil"
           : (width_bits == 16) ? "si"
           : (width_bits == 32) ? "esi"
                                : "rsi";
  case REG_RDI:
    return (width_bits == 8)   ? "dil"
           : (width_bits == 16) ? "di"
           : (width_bits == 32) ? "edi"
                                : "rdi";
  case REG_R8:
    return (width_bits == 8)   ? "r8b"
           : (width_bits == 16) ? "r8w"
           : (width_bits == 32) ? "r8d"
                                : "r8";
  case REG_R9:
    return (width_bits == 8)   ? "r9b"
           : (width_bits == 16) ? "r9w"
           : (width_bits == 32) ? "r9d"
                                : "r9";
  case REG_R10:
    return (width_bits == 8)   ? "r10b"
           : (width_bits == 16) ? "r10w"
           : (width_bits == 32) ? "r10d"
                                : "r10";
  case REG_R11:
    return (width_bits == 8)   ? "r11b"
           : (width_bits == 16) ? "r11w"
           : (width_bits == 32) ? "r11d"
                                : "r11";
  case REG_R12:
    return (width_bits == 8)   ? "r12b"
           : (width_bits == 16) ? "r12w"
           : (width_bits == 32) ? "r12d"
                                : "r12";
  case REG_R13:
    return (width_bits == 8)   ? "r13b"
           : (width_bits == 16) ? "r13w"
           : (width_bits == 32) ? "r13d"
                                : "r13";
  case REG_R14:
    return (width_bits == 8)   ? "r14b"
           : (width_bits == 16) ? "r14w"
           : (width_bits == 32) ? "r14d"
                                : "r14";
  case REG_R15:
    return (width_bits == 8)   ? "r15b"
           : (width_bits == 16) ? "r15w"
           : (width_bits == 32) ? "r15d"
                                : "r15";
  default:
    return NULL;
  }
}

static void code_generator_emit_runtime_trap(CodeGenerator *generator,
                                             const char *message) {
  if (!generator || !message) {
    return;
  }

  char *message_label = code_generator_generate_label(generator, "runtime_msg");
  if (!message_label) {
    free(message_label);
    code_generator_set_error(
        generator, "Out of memory while creating runtime trap labels");
    return;
  }

  CallingConventionSpec *conv_spec =
      generator->register_allocator
          ? generator->register_allocator->calling_convention
          : NULL;
  const char *first_param_reg = "rdi";
  if (conv_spec && conv_spec->int_param_count > 0) {
    const char *candidate =
        code_generator_get_register_name(conv_spec->int_param_registers[0]);
    if (candidate) {
      first_param_reg = candidate;
    }
  }

  code_generator_emit_to_global_buffer(generator, "%s:\n", message_label);
  if (!code_generator_emit_escaped_string_bytes(generator, message, 1)) {
    free(message_label);
    return;
  }
  code_generator_emit_to_global_buffer(generator, "\n");

  if (!code_generator_emit_extern_symbol(generator, "puts") ||
      !code_generator_emit_extern_symbol(generator, "exit")) {
    free(message_label);
    return;
  }
  code_generator_emit(generator,
                      "    lea %s, [rel %s]  ; runtime trap message\n",
                      first_param_reg, message_label);
  if (conv_spec && conv_spec->convention == CALLING_CONV_MS_X64) {
    code_generator_emit(generator,
                        "    sub rsp, %d      ; Shadow space for puts\n",
                        conv_spec->shadow_space_size);
    code_generator_emit(generator, "    call puts\n");
    code_generator_emit(generator, "    add rsp, %d\n",
                        conv_spec->shadow_space_size);
    code_generator_emit(generator, "    mov ecx, 1\n");
    code_generator_emit(generator,
                        "    sub rsp, %d      ; Shadow space for exit\n",
                        conv_spec->shadow_space_size);
    code_generator_emit(generator, "    call exit\n");
    code_generator_emit(generator, "    add rsp, %d\n",
                        conv_spec->shadow_space_size);
  } else {
    code_generator_emit(generator, "    call puts\n");
    code_generator_emit(generator, "    mov edi, 1\n");
    code_generator_emit(generator, "    call exit\n");
  }

  free(message_label);
}

static void code_generator_emit_null_check(CodeGenerator *generator,
                                           const char *context) {
  if (!generator) {
    return;
  }

  char *continue_label = code_generator_generate_label(generator, "nonnull");
  if (!continue_label) {
    code_generator_set_error(generator,
                             "Out of memory while creating null-check label");
    return;
  }

  code_generator_emit(generator, "    test rax, rax\n");
  code_generator_emit(generator, "    jnz %s\n", continue_label);
  code_generator_emit_runtime_trap(
      generator, context ? context : "Fatal error: Null pointer dereference");
  code_generator_emit(generator, "%s:\n", continue_label);
  free(continue_label);
}

static void code_generator_emit_bounds_check(CodeGenerator *generator,
                                             Type *array_type) {
  if (!generator || !array_type || array_type->kind != TYPE_ARRAY) {
    return;
  }

  char *continue_label = code_generator_generate_label(generator, "in_bounds");
  if (!continue_label) {
    code_generator_set_error(generator,
                             "Out of memory while creating bounds-check label");
    return;
  }

  code_generator_emit(generator, "    cmp rax, %zu\n", array_type->array_size);
  code_generator_emit(generator, "    jb %s\n", continue_label);
  code_generator_emit_runtime_trap(generator,
                                   "Fatal error: Array index out of bounds");
  code_generator_emit(generator, "%s:\n", continue_label);
  free(continue_label);
}

// Expression and assignment implementation functions
void code_generator_generate_binary_operation(CodeGenerator *generator,
                                              ASTNode *left, const char *op,
                                              ASTNode *right) {
  if (!generator || !left || !op || !right) {
    code_generator_set_error(generator, "Malformed binary operation");
    return;
  }

  code_generator_emit(generator, "    ; Binary operation: %s\n", op);

  Type *left_type = code_generator_infer_expression_type(generator, left);
  Type *right_type = code_generator_infer_expression_type(generator, right);

  if (left_type == generator->type_checker->builtin_string &&
      right_type == generator->type_checker->builtin_string &&
      strcmp(op, "+") == 0) {

    code_generator_emit(generator, "    ; String concatenation (+)\n");
    code_generator_generate_expression(generator, left);
    code_generator_emit(generator,
                        "    push rax           ; Save left string ptr\n");

    code_generator_generate_expression(generator, right);
    code_generator_emit(generator,
                        "    mov r10, rax       ; right string ptr -> r10\n");
    code_generator_emit(generator,
                        "    pop rax            ; left string ptr -> rax\n");

    // Calculate total length
    code_generator_emit(generator, "    mov rcx, [rax + 8] ; len1\n");
    code_generator_emit(generator, "    add rcx, [r10 + 8] ; len1 + len2\n");

    // Save ptrs and length
    code_generator_emit(generator,
                        "    sub rsp, 24        ; Save concat state\n");
    code_generator_emit(generator, "    mov [rsp], r10     ; right ptr\n");
    code_generator_emit(generator, "    mov [rsp + 8], rax ; left ptr\n");
    code_generator_emit(generator, "    mov [rsp + 16], rcx ; total_len\n");

    // gc_alloc(total_len + 17)
    const char *size_register = "rdi";
    CallingConventionSpec *conv_spec =
        generator->register_allocator
            ? generator->register_allocator->calling_convention
            : NULL;
    if (conv_spec && conv_spec->int_param_count > 0) {
      const char *cand =
          code_generator_get_register_name(conv_spec->int_param_registers[0]);
      if (cand)
        size_register = cand;
    }
    code_generator_emit(generator, "    mov %s, rcx\n", size_register);
    code_generator_emit(generator, "    add %s, 17\n", size_register);

    // Call gc_alloc with ABI-safe alignment and shadow space.
    if (conv_spec && conv_spec->convention == CALLING_CONV_MS_X64) {
      code_generator_emit(
          generator,
          "    sub rsp, %d      ; 32-byte shadow + 8-byte align pad\n",
          conv_spec->shadow_space_size + 8);
    }
    code_generator_emit(generator, "    extern gc_alloc\n    call gc_alloc\n");
    if (conv_spec && conv_spec->convention == CALLING_CONV_MS_X64) {
      code_generator_emit(generator, "    add rsp, %d\n",
                          conv_spec->shadow_space_size + 8);
    }

    // Restore values
    code_generator_emit(generator, "    mov rcx, [rsp + 16] ; total_len\n");
    code_generator_emit(generator, "    mov rdx, [rsp + 8] ; left ptr\n");
    code_generator_emit(generator, "    mov rsi, [rsp]    ; right ptr\n");
    code_generator_emit(generator, "    add rsp, 24\n");

    // Populate struct
    code_generator_emit(generator, "    lea r8, [rax + 16]\n");
    code_generator_emit(generator, "    mov [rax], r8\n");
    code_generator_emit(generator, "    mov [rax + 8], rcx\n");

    // Generate labels
    char *label_left_done =
        code_generator_generate_label(generator, "concat_left_done");
    char *label_left_loop =
        code_generator_generate_label(generator, "concat_left_loop");
    char *label_right_done =
        code_generator_generate_label(generator, "concat_right_done");
    char *label_right_loop =
        code_generator_generate_label(generator, "concat_right_loop");

    // Copy left
    code_generator_emit(generator, "    mov r9, [rdx + 8]  ; left len\n");
    code_generator_emit(generator, "    mov rdi, [rdx]     ; left chars\n");
    code_generator_emit(generator, "    test r9, r9\n");
    code_generator_emit(generator, "    jz %s\n", label_left_done);
    code_generator_emit(generator, "%s:\n", label_left_loop);
    code_generator_emit(generator, "    mov r11b, [rdi]\n");
    code_generator_emit(generator, "    mov [r8], r11b\n");
    code_generator_emit(generator, "    inc rdi\n    inc r8\n    dec r9\n");
    code_generator_emit(generator, "    jnz %s\n", label_left_loop);
    code_generator_emit(generator, "%s:\n", label_left_done);

    // Copy right
    code_generator_emit(generator, "    mov r9, [rsi + 8]  ; right len\n");
    code_generator_emit(generator, "    mov rdi, [rsi]     ; right chars\n");
    code_generator_emit(generator, "    test r9, r9\n");
    code_generator_emit(generator, "    jz %s\n", label_right_done);
    code_generator_emit(generator, "%s:\n", label_right_loop);
    code_generator_emit(generator, "    mov r11b, [rdi]\n");
    code_generator_emit(generator, "    mov [r8], r11b\n");
    code_generator_emit(generator, "    inc rdi\n    inc r8\n    dec r9\n");
    code_generator_emit(generator, "    jnz %s\n", label_right_loop);
    code_generator_emit(generator, "%s:\n", label_right_done);

    // null term
    code_generator_emit(generator, "    mov byte [r8], 0\n");

    free(label_left_done);
    free(label_left_loop);
    free(label_right_done);
    free(label_right_loop);
    return;
  }

  int is_float = code_generator_is_floating_point_type(left_type) ||
                 code_generator_is_floating_point_type(right_type);

  if (is_float) {
    // Generate and push left operand (float)
    code_generator_generate_expression(generator, left); // result in xmm0
    code_generator_emit(
        generator, "    sub rsp, 8          ; Make space for float on stack\n");
    code_generator_emit(generator,
                        "    movsd [rsp], xmm0  ; Save left operand (float)\n");

    // Generate right operand (float)
    code_generator_generate_expression(generator, right); // result in xmm0

    // Pop left operand into xmm1
    code_generator_emit(
        generator, "    movsd xmm1, [rsp]  ; Restore left operand (float)\n");
    code_generator_emit(generator,
                        "    add rsp, 8          ; Clean up stack\n");

    const char *instruction = code_generator_get_arithmetic_instruction(op, 1);
    if (instruction) {
      code_generator_emit(generator, "    %s xmm1, xmm0      ; %s operation\n",
                          instruction, op);
      code_generator_emit(generator,
                          "    movsd xmm0, xmm1   ; Move result to xmm0\n");
    } else {
      // Handle float comparisons
      if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
          strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
          strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
        code_generator_emit(generator,
                            "    ucomisd xmm1, xmm0 ; Compare floats\n");
        const char *set_instruction = "nop";
        if (strcmp(op, "==") == 0)
          set_instruction = "sete";
        else if (strcmp(op, "!=") == 0)
          set_instruction = "setne";
        else if (strcmp(op, "<") == 0)
          set_instruction = "setb";
        else if (strcmp(op, "<=") == 0)
          set_instruction = "setbe";
        else if (strcmp(op, ">") == 0)
          set_instruction = "seta";
        else if (strcmp(op, ">=") == 0)
          set_instruction = "setae";

        code_generator_emit(generator,
                            "    %s al             ; Set AL on condition\n",
                            set_instruction);
        code_generator_emit(generator,
                            "    movzx rax, al     ; Zero-extend AL to RAX\n");
      } else {
        code_generator_set_error(
            generator, "Unsupported floating-point operator '%s'", op);
      }
    }
  } else {
    // Integer operations
    code_generator_generate_expression(generator, left);
    code_generator_emit(generator,
                        "    push rax           ; Save left operand\n");

    code_generator_generate_expression(generator, right);
    code_generator_emit(generator,
                        "    mov r10, rax     ; Move right operand to R10\n");

    code_generator_emit(generator,
                        "    pop rax            ; Restore left operand\n");

    const char *instruction = code_generator_get_arithmetic_instruction(op, 0);
    if (instruction) {
      if (strcmp(op, "/") == 0) {
        code_generator_emit(
            generator,
            "    cqo                  ; Sign-extend RAX to RDX:RAX\n");
        code_generator_emit(generator,
                            "    idiv r10           ; Divide RDX:RAX by R10\n");
      } else if (strcmp(op, "%") == 0) {
        code_generator_emit(
            generator,
            "    cqo                  ; Sign-extend RAX to RDX:RAX\n");
        code_generator_emit(generator,
                            "    idiv r10           ; Divide RDX:RAX by R10\n");
        code_generator_emit(generator,
                            "    mov rax, rdx     ; Move remainder to RAX\n");
      } else if (strcmp(op, "<<") == 0) {
        code_generator_emit(
            generator, "    mov rcx, r10       ; Move shift amount to CL\n");
        code_generator_emit(generator, "    shl rax, cl        ; Shift left\n");
      } else if (strcmp(op, ">>") == 0) {
        code_generator_emit(
            generator, "    mov rcx, r10       ; Move shift amount to CL\n");
        code_generator_emit(
            generator, "    sar rax, cl        ; Shift right arithmetic\n");
      } else {
        code_generator_emit(generator, "    %s rax, r10      ; %s operation\n",
                            instruction, op);
      }
    } else {
      // Handle comparison and logical operators
      code_generator_emit(generator, "    cmp rax, r10     ; Compare\n");
      const char *set_instruction = "nop";
      if (strcmp(op, "==") == 0)
        set_instruction = "sete";
      else if (strcmp(op, "!=") == 0)
        set_instruction = "setne";
      else if (strcmp(op, "<") == 0)
        set_instruction = "setl";
      else if (strcmp(op, "<=") == 0)
        set_instruction = "setle";
      else if (strcmp(op, ">") == 0)
        set_instruction = "setg";
      else if (strcmp(op, ">=") == 0)
        set_instruction = "setge";

      if (strcmp(set_instruction, "nop") != 0) {
        code_generator_emit(generator,
                            "    %s al            ; Set AL on condition\n",
                            set_instruction);
        code_generator_emit(generator,
                            "    movzx rax, al    ; Zero-extend AL to RAX\n");
      } else if (strcmp(op, "&&") == 0) {
        code_generator_emit(generator, "    and rax, r10     ; Logical AND\n");
        code_generator_emit(
            generator,
            "    setne al           ; Set AL if result is not zero\n");
        code_generator_emit(generator,
                            "    movzx rax, al    ; Zero-extend AL to RAX\n");
      } else if (strcmp(op, "||") == 0) {
        code_generator_emit(generator, "    or rax, r10      ; Logical OR\n");
        code_generator_emit(
            generator,
            "    setne al           ; Set AL if result is not zero\n");
        code_generator_emit(generator,
                            "    movzx rax, al    ; Zero-extend AL to RAX\n");
      } else {
        code_generator_set_error(generator, "Unknown binary operator '%s'", op);
      }
    }
  }
}
void code_generator_generate_unary_operation(CodeGenerator *generator,
                                             const char *op, ASTNode *operand) {
  if (!generator || !op || !operand) {
    code_generator_set_error(generator, "Malformed unary operation");
    return;
  }

  code_generator_emit(generator, "    ; Unary operation: %s\n", op);

  if (strcmp(op, "&") == 0) {
    Type *target_type = NULL;
    if (!code_generator_generate_lvalue_address(generator, operand,
                                                &target_type)) {
      return;
    }
  } else if (strcmp(op, "*") == 0) {
    Type *operand_type =
        code_generator_infer_expression_type(generator, operand);
    code_generator_generate_expression(generator, operand);
    if (generator->has_error) {
      return;
    }

    if (!operand_type || operand_type->kind != TYPE_POINTER ||
        !operand_type->base_type) {
      code_generator_set_error(generator,
                               "Dereference requires a pointer operand");
      return;
    }

    if (operand_type->base_type->kind == TYPE_STRUCT ||
        operand_type->base_type->kind == TYPE_ARRAY) {
      code_generator_set_error(
          generator,
          "Aggregate dereference values are not supported in this context");
      return;
    }

    code_generator_emit_null_check(generator,
                                   "Fatal error: Null pointer dereference");
    if (generator->has_error) {
      return;
    }

    int element_size =
        code_generator_get_type_storage_size(operand_type->base_type);
    code_generator_emit_load_value_from_address(generator, element_size);
  } else {
    // Generate operand (result in RAX)
    code_generator_generate_expression(generator, operand);
    if (generator->has_error) {
      return;
    }
  }

  if (strcmp(op, "-") == 0) {
    code_generator_emit(generator, "    neg rax            ; Negate\n");
  } else if (strcmp(op, "!") == 0) {
    code_generator_emit(generator, "    test rax, rax    ; Test for zero\n");
    code_generator_emit(generator, "    setz al            ; Set AL if zero\n");
    code_generator_emit(generator,
                        "    movzx rax, al    ; Zero-extend AL to RAX\n");
  } else if (strcmp(op, "~") == 0) {
    code_generator_emit(generator, "    not rax            ; Bitwise NOT\n");
  } else if (strcmp(op, "+") == 0) {
    // Unary plus - no operation needed
    code_generator_emit(generator, "    ; Unary plus (no-op)\n");
  } else if (strcmp(op, "&") == 0 || strcmp(op, "*") == 0) {
    // Already handled above.
  } else {
    code_generator_set_error(generator, "Unknown unary operator '%s'", op);
  }
}
void code_generator_generate_assignment_statement(CodeGenerator *generator,
                                                  ASTNode *assignment) {
  if (!generator || !assignment || assignment->type != AST_ASSIGNMENT) {
    return;
  }

  Assignment *assign_data = (Assignment *)assignment->data;
  if (!assign_data || !assign_data->value) {
    return;
  }

  if (assign_data->variable_name) {
    // Simple variable assignment: name = expr
    code_generator_emit(generator, "    ; Assignment to %s\n",
                        assign_data->variable_name);

    // Generate the value expression (result in RAX)
    code_generator_generate_expression(generator, assign_data->value);

    // Store the result to the variable
    code_generator_store_variable(generator, assign_data->variable_name, "rax");
  } else if (assign_data->target) {
    Type *target_type = NULL;
    code_generator_generate_expression(generator, assign_data->value);
    if (generator->has_error) {
      return;
    }

    code_generator_emit(generator,
                        "    push rax           ; Save assigned value\n");
    if (!code_generator_generate_lvalue_address(generator, assign_data->target,
                                                &target_type)) {
      return;
    }

    if (!target_type) {
      code_generator_set_error(generator,
                               "Unable to resolve assignment target type");
      return;
    }
    if (target_type->kind == TYPE_STRUCT || target_type->kind == TYPE_ARRAY) {
      code_generator_set_error(generator,
                               "Aggregate assignment is not supported");
      return;
    }

    int element_size = code_generator_get_type_storage_size(target_type);
    code_generator_emit(generator, "    pop rcx            ; Restore value\n");
    code_generator_emit_store_value_at_address(generator, element_size);
  } else {
    code_generator_set_error(generator, "Invalid assignment target");
  }
}

void code_generator_load_variable(CodeGenerator *generator,
                                  const char *variable_name) {
  if (!generator || !variable_name) {
    return;
  }

  code_generator_emit(generator, "    ; Load variable: %s\n", variable_name);

  Symbol *symbol = symbol_table_lookup(generator->symbol_table, variable_name);
  if (symbol &&
      (symbol->kind == SYMBOL_VARIABLE || symbol->kind == SYMBOL_PARAMETER)) {
    const char *resolved_name = variable_name;
    if (symbol->is_extern) {
      resolved_name =
          code_generator_get_link_symbol_name(generator, variable_name);
      if (!resolved_name) {
        code_generator_set_error(generator, "Invalid extern symbol name '%s'",
                                 variable_name);
        return;
      }
      if (!code_generator_emit_extern_symbol(generator, resolved_name)) {
        return;
      }
    }

    if (symbol->type && symbol->type->kind == TYPE_ARRAY) {
      if (symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
        code_generator_emit(generator,
                            "    lea rax, [rel %s]  ; Array base address\n",
                            resolved_name);
      } else {
        int offset = symbol->data.variable.memory_offset;
        code_generator_emit(
            generator, "    lea rax, [rbp - %d]  ; Local array base\n", offset);
      }
      return;
    }

    if (symbol->type && symbol->type->kind == TYPE_STRING) {
      // Strings are represented as pointers to {chars, length} records in
      // expressions. Globals and locals differ in storage layout.
      if (symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
        code_generator_emit(
            generator, "    lea rax, [rel %s]  ; Address of global string\n",
            resolved_name);
      } else if (symbol->kind == SYMBOL_PARAMETER) {
        // String parameters are currently homed as pointers to string records.
        code_generator_emit(
            generator, "    mov rax, qword [rbp - %d]  ; String param ptr\n",
            symbol->data.variable.memory_offset);
      } else {
        int offset = symbol->data.variable.memory_offset;
        code_generator_emit(
            generator, "    lea rax, [rbp - %d]  ; Address of local string\n",
            offset);
      }
      return;
    }

    int value_size = 8;
    if (symbol->type && symbol->type->size > 0 && symbol->type->size <= 8) {
      value_size = (int)symbol->type->size;
    }
    int signed_integer = code_generator_is_signed_integer_type(symbol->type);

    if (symbol->data.variable.is_in_register) {
      x86Register reg = (x86Register)symbol->data.variable.register_id;
      const char *reg64 = code_generator_get_subregister_name(reg, 64);
      if (!reg64) {
        code_generator_set_error(generator, "Invalid register for variable '%s'",
                                 variable_name);
        return;
      }

      if (value_size == 1) {
        const char *reg8 = code_generator_get_subregister_name(reg, 8);
        if (!reg8) {
          code_generator_set_error(generator,
                                   "Invalid 8-bit register for variable '%s'",
                                   variable_name);
          return;
        }
        if (signed_integer) {
          code_generator_emit(generator, "    movsx rax, %s      ; From register "
                                         "(signed int8)\n",
                              reg8);
        } else {
          code_generator_emit(generator, "    movzx rax, %s      ; From register "
                                         "(uint8)\n",
                              reg8);
        }
      } else if (value_size == 2) {
        const char *reg16 = code_generator_get_subregister_name(reg, 16);
        if (!reg16) {
          code_generator_set_error(generator,
                                   "Invalid 16-bit register for variable '%s'",
                                   variable_name);
          return;
        }
        if (signed_integer) {
          code_generator_emit(generator, "    movsx rax, %s      ; From register "
                                         "(signed int16)\n",
                              reg16);
        } else {
          code_generator_emit(generator, "    movzx rax, %s      ; From register "
                                         "(uint16)\n",
                              reg16);
        }
      } else if (value_size == 4) {
        const char *reg32 = code_generator_get_subregister_name(reg, 32);
        if (!reg32) {
          code_generator_set_error(generator,
                                   "Invalid 32-bit register for variable '%s'",
                                   variable_name);
          return;
        }
        if (signed_integer) {
          code_generator_emit(generator, "    movsxd rax, %s      ; From register "
                                         "(signed int32)\n",
                              reg32);
        } else {
          code_generator_emit(generator, "    mov eax, %s      ; From register "
                                         "(uint32)\n",
                              reg32);
        }
      } else {
        code_generator_emit(generator, "    mov rax, %s      ; From register\n",
                            reg64);
      }
    } else {
      if (symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
        if (value_size == 1) {
          if (signed_integer) {
            code_generator_emit(
                generator,
                "    movsx rax, byte [rel %s]  ; From global memory (signed)\n",
                resolved_name);
          } else {
            code_generator_emit(
                generator,
                "    movzx rax, byte [rel %s]  ; From global memory\n",
                resolved_name);
          }
        } else if (value_size == 2) {
          if (signed_integer) {
            code_generator_emit(
                generator,
                "    movsx rax, word [rel %s]  ; From global memory (signed)\n",
                resolved_name);
          } else {
            code_generator_emit(
                generator,
                "    movzx rax, word [rel %s]  ; From global memory\n",
                resolved_name);
          }
        } else if (value_size == 4) {
          code_generator_emit(
              generator,
              "    movsxd rax, dword [rel %s]  ; From global memory\n",
              resolved_name);
        } else {
          code_generator_emit(
              generator, "    mov rax, qword [rel %s]  ; From global memory\n",
              resolved_name);
        }
      } else {
        // Local variable or parameter on stack
        int offset = symbol->data.variable.memory_offset;
        if (value_size == 1) {
          if (signed_integer) {
            code_generator_emit(generator,
                                "    movsx rax, byte [rbp - %d]  ; From stack "
                                "[rbp - %d] (signed)\n",
                                offset, offset);
          } else {
            code_generator_emit(generator,
                                "    movzx rax, byte [rbp - %d]  ; From stack "
                                "[rbp - %d]\n",
                                offset, offset);
          }
        } else if (value_size == 2) {
          if (signed_integer) {
            code_generator_emit(generator,
                                "    movsx rax, word [rbp - %d]  ; From stack "
                                "[rbp - %d] (signed)\n",
                                offset, offset);
          } else {
            code_generator_emit(generator,
                                "    movzx rax, word [rbp - %d]  ; From stack "
                                "[rbp - %d]\n",
                                offset, offset);
          }
        } else if (value_size == 4) {
          code_generator_emit(generator,
                              "    movsxd rax, dword [rbp - %d]  ; From stack "
                              "[rbp - %d]\n",
                              offset, offset);
        } else {
          code_generator_emit(generator,
                              "    mov rax, qword [rbp - %d]  ; From stack "
                              "[rbp - %d]\n",
                              offset, offset);
        }
      }
    }
  } else {
    code_generator_set_error(generator,
                             "Undefined variable '%s' during code generation",
                             variable_name);
  }
}

void code_generator_store_variable(CodeGenerator *generator,
                                   const char *variable_name,
                                   const char *source_reg) {
  if (!generator || !variable_name || !source_reg) {
    return;
  }

  code_generator_emit(generator, "    ; Store to variable: %s\n",
                      variable_name);

  Symbol *symbol = symbol_table_lookup(generator->symbol_table, variable_name);
  if (symbol &&
      (symbol->kind == SYMBOL_VARIABLE || symbol->kind == SYMBOL_PARAMETER)) {
    const char *resolved_name = variable_name;
    if (symbol->is_extern) {
      resolved_name =
          code_generator_get_link_symbol_name(generator, variable_name);
      if (!resolved_name) {
        code_generator_set_error(generator, "Invalid extern symbol name '%s'",
                                 variable_name);
        return;
      }
      if (!code_generator_emit_extern_symbol(generator, resolved_name)) {
        return;
      }
    }

    if (symbol->type && symbol->type->kind == TYPE_ARRAY) {
      code_generator_set_error(generator,
                               "Cannot assign directly to array variable '%s'",
                               variable_name);
      return;
    }

    int value_size = 8;
    if (symbol->type && symbol->type->size > 0 && symbol->type->size <= 8) {
      value_size = (int)symbol->type->size;
    }

    if (strcmp(source_reg, "rax") != 0) {
      code_generator_emit(generator, "    mov rax, %s\n", source_reg);
      source_reg = "rax";
    }

    if (symbol->type && symbol->type->kind == TYPE_STRING) {
      if (symbol->kind == SYMBOL_PARAMETER) {
        // Parameters store a pointer to a string record in their home slot.
        code_generator_emit(
            generator, "    mov qword [rbp - %d], rax  ; String param ptr\n",
            symbol->data.variable.memory_offset);
        return;
      }

      // Source value in rax is a pointer to {chars, length}; copy the full
      // 16-byte record into destination storage.
      if (symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
        code_generator_emit(generator,
                            "    mov rcx, [rax]       ; string chars\n");
        code_generator_emit(generator, "    mov [rel %s], rcx\n",
                            resolved_name);
        code_generator_emit(generator,
                            "    mov rcx, [rax + 8]   ; string length\n");
        code_generator_emit(generator, "    mov [rel %s + 8], rcx\n",
                            resolved_name);
      } else {
        int offset = symbol->data.variable.memory_offset;
        int length_offset = offset - 8;
        if (length_offset <= 0) {
          code_generator_set_error(
              generator, "Invalid local string layout for '%s' (offset=%d)",
              variable_name, offset);
          return;
        }
        code_generator_emit(generator,
                            "    mov rcx, [rax]       ; string chars\n");
        code_generator_emit(generator, "    mov [rbp - %d], rcx\n", offset);
        code_generator_emit(generator,
                            "    mov rcx, [rax + 8]   ; string length\n");
        code_generator_emit(generator, "    mov [rbp - %d], rcx\n",
                            length_offset);
      }
      return;
    }

    if (symbol->data.variable.is_in_register) {
      x86Register reg = (x86Register)symbol->data.variable.register_id;
      const char *reg64 = code_generator_get_subregister_name(reg, 64);
      if (!reg64) {
        code_generator_set_error(generator, "Invalid register for variable '%s'",
                                 variable_name);
        return;
      }

      if (value_size == 1) {
        const char *reg8 = code_generator_get_subregister_name(reg, 8);
        if (!reg8) {
          code_generator_set_error(generator,
                                   "Invalid 8-bit register for variable '%s'",
                                   variable_name);
          return;
        }
        code_generator_emit(generator, "    mov %s, al       ; To register "
                                       "(int8/uint8)\n",
                            reg8);
      } else if (value_size == 2) {
        const char *reg16 = code_generator_get_subregister_name(reg, 16);
        if (!reg16) {
          code_generator_set_error(generator,
                                   "Invalid 16-bit register for variable '%s'",
                                   variable_name);
          return;
        }
        code_generator_emit(generator, "    mov %s, ax       ; To register "
                                       "(int16/uint16)\n",
                            reg16);
      } else if (value_size == 4) {
        const char *reg32 = code_generator_get_subregister_name(reg, 32);
        if (!reg32) {
          code_generator_set_error(generator,
                                   "Invalid 32-bit register for variable '%s'",
                                   variable_name);
          return;
        }
        code_generator_emit(generator, "    mov %s, eax       ; To register "
                                       "(int32/uint32)\n",
                            reg32);
      } else {
        code_generator_emit(generator, "    mov %s, %s       ; To register\n",
                            reg64, source_reg);
      }
    } else {
      if (symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
        if (value_size == 1) {
          code_generator_emit(generator,
                              "    mov byte [rel %s], al  ; To global memory\n",
                              resolved_name);
        } else if (value_size == 2) {
          code_generator_emit(generator,
                              "    mov word [rel %s], ax  ; To global memory\n",
                              resolved_name);
        } else if (value_size == 4) {
          code_generator_emit(
              generator, "    mov dword [rel %s], eax  ; To global memory\n",
              resolved_name);
        } else {
          code_generator_emit(
              generator, "    mov qword [rel %s], rax  ; To global memory\n",
              resolved_name);
        }
      } else {
        // Local variable or parameter on stack
        int offset = symbol->data.variable.memory_offset;
        if (value_size == 1) {
          code_generator_emit(
              generator, "    mov byte [rbp - %d], al  ; To stack [rbp - %d]\n",
              offset, offset);
        } else if (value_size == 2) {
          code_generator_emit(
              generator, "    mov word [rbp - %d], ax  ; To stack [rbp - %d]\n",
              offset, offset);
        } else if (value_size == 4) {
          code_generator_emit(
              generator,
              "    mov dword [rbp - %d], eax  ; To stack [rbp - "
              "%d]\n",
              offset, offset);
        } else {
          code_generator_emit(
              generator,
              "    mov qword [rbp - %d], rax  ; To stack [rbp - "
              "%d]\n",
              offset, offset);
        }
      }
    }
  } else {
    code_generator_set_error(
        generator, "Cannot store to undefined variable '%s'", variable_name);
  }
}
void code_generator_load_string_literal(CodeGenerator *generator,
                                        const char *string_value) {
  if (!generator || !string_value) {
    return;
  }

  code_generator_emit(generator, "    ; String literal (%zu bytes)\n",
                      strlen(string_value));

  // Generate a unique label for this string
  char *label = code_generator_generate_label(generator, "str_chars");
  char *label_struct = code_generator_generate_label(generator, "str_struct");
  if (label && label_struct) {
    // Load string address into RAX
    code_generator_emit(generator,
                        "    lea rax, [rel %s]  ; Load string struct address\n",
                        label_struct);

    // Add string to global variables buffer for data section
    code_generator_emit_to_global_buffer(generator, "%s:\n", label);
    if (!code_generator_emit_escaped_string_bytes(generator, string_value, 1)) {
      free(label);
      free(label_struct);
      return;
    }

    code_generator_emit_to_global_buffer(generator, "    align 8\n");
    code_generator_emit_to_global_buffer(generator, "%s:\n", label_struct);
    code_generator_emit_to_global_buffer(generator, "    dq %s\n", label);
    code_generator_emit_to_global_buffer(generator, "    dq %zu\n",
                                         strlen(string_value));
    code_generator_emit_to_global_buffer(generator, "\n");

    free(label);
    free(label_struct);
  } else {
    if (label)
      free(label);
    if (label_struct)
      free(label_struct);
  }
}

void code_generator_load_string_literal_as_cstring(CodeGenerator *generator,
                                                   const char *string_value) {
  if (!generator || !string_value) {
    return;
  }

  code_generator_emit(generator,
                      "    ; String literal as cstring (%zu bytes)\n",
                      strlen(string_value));

  char *label = code_generator_generate_label(generator, "str_chars");
  if (label) {
    code_generator_emit(
        generator,
        "    lea rax, [rel %s]  ; Load cstring (raw chars) address\n", label);
    code_generator_emit_to_global_buffer(generator, "%s:\n", label);
    if (!code_generator_emit_escaped_string_bytes(generator, string_value, 1)) {
      free(label);
      return;
    }
    code_generator_emit_to_global_buffer(generator, "\n");
    free(label);
  }
}

int code_generator_get_operator_precedence(const char *op) {
  if (!op)
    return 0;

  // Higher numbers = higher precedence
  if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {
    return 6;
  } else if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) {
    return 5;
  } else if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
             strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
    return 4;
  } else if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
    return 3;
  } else if (strcmp(op, "&&") == 0) {
    return 2;
  } else if (strcmp(op, "||") == 0) {
    return 1;
  }
  return 0;
}

const char *code_generator_get_arithmetic_instruction(const char *op,
                                                      int is_float) {
  if (!op)
    return NULL;

  if (is_float) {
    // Floating point instructions (for future implementation)
    if (strcmp(op, "+") == 0)
      return "addsd";
    if (strcmp(op, "-") == 0)
      return "subsd";
    if (strcmp(op, "*") == 0)
      return "mulsd";
    if (strcmp(op, "/") == 0)
      return "divsd";
    return NULL;
  } else {
    // Integer instructions
    if (strcmp(op, "+") == 0)
      return "add";
    if (strcmp(op, "-") == 0)
      return "sub";
    if (strcmp(op, "*") == 0)
      return "imul";
    // Division and modulo are handled specially via idiv in the IR backend;
    // return a sentinel so the caller enters the arithmetic branch.
    if (strcmp(op, "/") == 0)
      return "idiv";
    if (strcmp(op, "%") == 0)
      return "idiv";
    if (strcmp(op, "&") == 0)
      return "and"; // Bitwise AND
    if (strcmp(op, "|") == 0)
      return "or"; // Bitwise OR
    if (strcmp(op, "^") == 0)
      return "xor"; // Bitwise XOR
    if (strcmp(op, "<<") == 0)
      return "shl"; // Handled specifically in code_generator_emit
    if (strcmp(op, ">>") == 0)
      return "sar"; // Handled specifically in code_generator_emit
    return NULL;
  }
}

// Struct and method implementation functions
void code_generator_generate_struct_declaration(CodeGenerator *generator,
                                                ASTNode *struct_declaration) {
  if (!generator || !struct_declaration ||
      struct_declaration->type != AST_STRUCT_DECLARATION) {
    return;
  }

  StructDeclaration *struct_data =
      (StructDeclaration *)struct_declaration->data;
  if (!struct_data || !struct_data->name) {
    return;
  }

  code_generator_emit(generator, "    ; Struct declaration: %s\n",
                      struct_data->name);

  // Look up struct type in symbol table
  Symbol *struct_symbol =
      symbol_table_lookup(generator->symbol_table, struct_data->name);
  if (struct_symbol && struct_symbol->kind == SYMBOL_STRUCT &&
      struct_symbol->type) {
    Type *struct_type = struct_symbol->type;

    // Calculate and set struct layout
    code_generator_calculate_struct_layout(generator, struct_type);

    code_generator_emit(generator, "    ; Struct %s: size=%zu, alignment=%zu\n",
                        struct_data->name, struct_type->size,
                        struct_type->alignment);

    // Generate method implementations if any
    if (struct_data->methods && struct_data->method_count > 0) {
      for (size_t i = 0; i < struct_data->method_count; i++) {
        if (struct_data->methods[i] &&
            struct_data->methods[i]->type == AST_FUNCTION_DECLARATION) {
          // Mangle the method name: StructName_methodname
          FunctionDeclaration *method_data =
              (FunctionDeclaration *)struct_data->methods[i]->data;
          if (method_data && method_data->name) {
            char *original_name = method_data->name;
            char mangled[256];
            snprintf(mangled, sizeof(mangled), "%s_%s", struct_data->name,
                     original_name);
            method_data->name = strdup(mangled);
            free(original_name);

            code_generator_emit(generator, "    ; Method %s of struct %s\n",
                                mangled, struct_data->name);
            code_generator_generate_function(generator,
                                             struct_data->methods[i]);
          }
        }
      }
    }
  } else {
    code_generator_set_error(
        generator, "Struct type '%s' not found during code generation",
        struct_data->name ? struct_data->name : "<unknown>");
  }
}

void code_generator_generate_method_call(CodeGenerator *generator,
                                         ASTNode *method_call,
                                         ASTNode *object) {
  if (!generator || !method_call || !object) {
    code_generator_set_error(generator, "Invalid method call AST node");
    return;
  }

  // Method calls are handled as special function calls with implicit "this"
  CallExpression *call_data = (CallExpression *)method_call->data;
  if (!call_data) {
    code_generator_set_error(generator, "Malformed method call expression");
    return;
  }

  code_generator_emit(generator, "    ; Method call: %s\n",
                      call_data->function_name);

  // Generate the object expression to get "this" pointer
  code_generator_emit(generator, "    ; Generate 'this' pointer\n");
  code_generator_generate_expression(generator, object);

  // Save "this" pointer as first parameter
  code_generator_emit(generator,
                      "    push rax           ; Save 'this' pointer\n");

  // Get calling convention for parameter passing
  CallingConventionSpec *conv_spec =
      generator->register_allocator->calling_convention;
  if (conv_spec && conv_spec->int_param_count > 0) {
    // Move "this" to first parameter register
    const char *first_param_reg =
        code_generator_get_register_name(conv_spec->int_param_registers[0]);
    if (first_param_reg) {
      code_generator_emit(
          generator,
          "    pop %s             ; 'this' in first parameter register\n",
          first_param_reg);
    }
  }

  // Generate other parameters starting from second parameter position
  if (call_data->arguments && call_data->argument_count > 0) {
    for (size_t i = 0; i < call_data->argument_count; i++) {
      if (call_data->arguments[i]) {
        // Generate argument (starts at parameter index 1 since 0 is "this")
        code_generator_generate_parameter(generator, call_data->arguments[i],
                                          (int)i + 1, NULL, NULL);
      }
    }
  }

  // Determine the mangled method name (StructName_methodname)
  char mangled_name[256];
  const char *struct_name = NULL;

  if (object->type == AST_IDENTIFIER) {
    Identifier *id_data = (Identifier *)object->data;
    if (id_data && id_data->name) {
      Symbol *obj_symbol =
          symbol_table_lookup(generator->symbol_table, id_data->name);
      if (obj_symbol && obj_symbol->type && obj_symbol->type->name) {
        struct_name = obj_symbol->type->name;
      }
    }
  }

  if (struct_name) {
    snprintf(mangled_name, sizeof(mangled_name), "%s_%s", struct_name,
             call_data->function_name);
  } else {
    snprintf(mangled_name, sizeof(mangled_name), "%s",
             call_data->function_name);
  }

  // Generate the actual method call with mangled name
  code_generator_emit(generator, "    call %s\n", mangled_name);
  code_generator_emit(generator, "    ; Return value in rax\n");
}

void code_generator_generate_member_access(CodeGenerator *generator,
                                           ASTNode *member_access) {
  if (!generator || !member_access ||
      member_access->type != AST_MEMBER_ACCESS) {
    code_generator_set_error(generator, "Invalid member access AST node");
    return;
  }

  MemberAccess *access_data = (MemberAccess *)member_access->data;
  if (!access_data || !access_data->object || !access_data->member) {
    code_generator_set_error(generator, "Malformed member access expression");
    return;
  }

  Type *object_type =
      code_generator_infer_expression_type(generator, access_data->object);
  if (object_type && object_type->kind == TYPE_STRING) {
    int field_offset = code_generator_get_field_offset(generator, object_type,
                                                       access_data->member);
    if (field_offset < 0) {
      code_generator_set_error(generator,
                               "Cannot determine field offset for '%s'",
                               access_data->member);
      return;
    }
    Type *field_type = type_get_field_type(object_type, access_data->member);
    if (!field_type) {
      code_generator_set_error(generator,
                               "Cannot resolve member access target type");
      return;
    }
    int field_size = code_generator_get_type_storage_size(field_type);
    code_generator_generate_expression(generator, access_data->object);
    if (generator->has_error) {
      return;
    }
    if (field_offset > 0) {
      code_generator_emit(generator, "    add rax, %d       ; String field\n",
                          field_offset);
    }
    code_generator_emit_load_value_from_address(generator, field_size);
    return;
  }

  Type *field_type = NULL;
  if (!code_generator_generate_lvalue_address(generator, member_access,
                                              &field_type)) {
    return;
  }
  if (!field_type) {
    code_generator_set_error(generator,
                             "Cannot resolve member access target type");
    return;
  }
  if (field_type->kind == TYPE_STRUCT || field_type->kind == TYPE_ARRAY) {
    code_generator_set_error(generator,
                             "Aggregate member values are not supported");
    return;
  }

  int field_size = code_generator_get_type_storage_size(field_type);
  code_generator_emit_load_value_from_address(generator, field_size);
}

static int code_generator_get_type_storage_size(Type *type) {
  if (!type || type->size == 0) {
    return 8;
  }
  if (type->size == 1 || type->size == 2 || type->size == 4 ||
      type->size == 8) {
    return (int)type->size;
  }
  return 8;
}

static void code_generator_emit_store_value_at_address(CodeGenerator *generator,
                                                       int element_size) {
  if (!generator) {
    return;
  }

  switch (element_size) {
  case 1:
    code_generator_emit(generator, "    mov byte [rax], cl\n");
    break;
  case 2:
    code_generator_emit(generator, "    mov word [rax], cx\n");
    break;
  case 4:
    code_generator_emit(generator, "    mov dword [rax], ecx\n");
    break;
  default:
    code_generator_emit(generator, "    mov qword [rax], rcx\n");
    break;
  }
}

static void
code_generator_emit_load_value_from_address(CodeGenerator *generator,
                                            int element_size) {
  if (!generator) {
    return;
  }

  switch (element_size) {
  case 1:
    code_generator_emit(generator, "    movzx rax, byte [rax]\n");
    break;
  case 2:
    code_generator_emit(generator, "    movzx rax, word [rax]\n");
    break;
  case 4:
    code_generator_emit(generator, "    mov eax, dword [rax]\n");
    break;
  default:
    code_generator_emit(generator, "    mov rax, qword [rax]\n");
    break;
  }
}

static int code_generator_generate_array_element_address(
    CodeGenerator *generator, ASTNode *array_expr, ASTNode *index_expr) {
  if (!generator || !array_expr || !index_expr) {
    return 0;
  }

  Type *array_type =
      code_generator_infer_expression_type(generator, array_expr);
  if (!array_type ||
      (array_type->kind != TYPE_ARRAY && array_type->kind != TYPE_POINTER) ||
      !array_type->base_type) {
    code_generator_set_error(generator,
                             "Indexing requires an array or pointer");
    return 0;
  }

  int element_size =
      code_generator_get_type_storage_size(array_type->base_type);

  code_generator_generate_expression(generator, array_expr);
  if (array_type->kind == TYPE_POINTER) {
    code_generator_emit_null_check(generator,
                                   "Fatal error: Null pointer dereference");
    if (generator->has_error) {
      return 0;
    }
  }
  code_generator_emit(generator, "    push rax           ; Save array base\n");
  code_generator_generate_expression(generator, index_expr);
  if (array_type->kind == TYPE_ARRAY) {
    code_generator_emit_bounds_check(generator, array_type);
    if (generator->has_error) {
      return 0;
    }
  }
  code_generator_emit(generator,
                      "    pop rcx            ; Restore array base\n");
  if (element_size > 1) {
    code_generator_emit(generator, "    imul rax, rax, %d\n", element_size);
  }
  code_generator_emit(generator, "    add rax, rcx       ; Element address\n");

  return element_size;
}

static int code_generator_generate_lvalue_address(CodeGenerator *generator,
                                                  ASTNode *target,
                                                  Type **out_target_type) {
  if (!generator || !target) {
    return 0;
  }

  if (out_target_type) {
    *out_target_type = NULL;
  }

  switch (target->type) {
  case AST_IDENTIFIER: {
    Identifier *id = (Identifier *)target->data;
    if (!id || !id->name) {
      code_generator_set_error(generator, "Invalid identifier lvalue");
      return 0;
    }

    Symbol *symbol = symbol_table_lookup(generator->symbol_table, id->name);
    if (!symbol ||
        (symbol->kind != SYMBOL_VARIABLE && symbol->kind != SYMBOL_PARAMETER)) {
      code_generator_set_error(generator, "Undefined lvalue identifier '%s'",
                               id->name);
      return 0;
    }

    if (out_target_type) {
      *out_target_type = symbol->type;
    }

    if (symbol->data.variable.is_in_register) {
      code_generator_set_error(
          generator, "Cannot take address of register-allocated variable '%s'",
          id->name);
      return 0;
    }

    if (symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
      const char *resolved_name =
          code_generator_get_link_symbol_name(generator, id->name);
      if (!resolved_name) {
        code_generator_set_error(generator, "Invalid extern symbol name '%s'",
                                 id->name);
        return 0;
      }
      if (symbol->is_extern &&
          !code_generator_emit_extern_symbol(generator, resolved_name)) {
        return 0;
      }
      code_generator_emit(generator,
                          "    lea rax, [rel %s]  ; Address of global\n",
                          resolved_name);
    } else {
      int offset = symbol->data.variable.memory_offset;
      if (symbol->kind == SYMBOL_PARAMETER && symbol->type &&
          symbol->type->kind == TYPE_STRING) {
        // String parameters are homed as pointers to string records.
        code_generator_emit(
            generator,
            "    mov rax, qword [rbp - %d]  ; String param record ptr\n",
            offset);
      } else {
        code_generator_emit(
            generator, "    lea rax, [rbp - %d]  ; Address of local\n", offset);
      }
    }
    return 1;
  }

  case AST_MEMBER_ACCESS: {
    MemberAccess *access = (MemberAccess *)target->data;
    if (!access || !access->object || !access->member) {
      code_generator_set_error(generator, "Invalid member lvalue");
      return 0;
    }

    Type *object_type = NULL;
    int object_is_string_param = 0;
    if (access->object && access->object->type == AST_IDENTIFIER) {
      Identifier *obj_id = (Identifier *)access->object->data;
      if (obj_id && obj_id->name) {
        Symbol *obj_symbol =
            symbol_table_lookup(generator->symbol_table, obj_id->name);
        if (obj_symbol && obj_symbol->kind == SYMBOL_PARAMETER &&
            obj_symbol->type && obj_symbol->type->kind == TYPE_STRING) {
          object_is_string_param = 1;
        }
      }
    }

    if (!code_generator_generate_lvalue_address(generator, access->object,
                                                &object_type)) {
      return 0;
    }
    if (!object_type || (object_type->kind != TYPE_STRUCT &&
                         object_type->kind != TYPE_STRING)) {
      code_generator_set_error(
          generator, "Member access requires struct or string object");
      return 0;
    }

    if (object_is_string_param) {
      // String parameters are homed as pointers to string records, so member
      // access needs one dereference before applying field offsets.
      code_generator_emit(
          generator, "    mov rax, qword [rax]  ; Deref string param record\n");
    }

    int field_offset =
        code_generator_get_field_offset(generator, object_type, access->member);
    if (field_offset < 0) {
      code_generator_set_error(
          generator, "Cannot determine field offset for '%s'", access->member);
      return 0;
    }
    if (field_offset > 0) {
      code_generator_emit(generator, "    add rax, %d       ; Field address\n",
                          field_offset);
    }

    if (out_target_type) {
      *out_target_type = type_get_field_type(object_type, access->member);
    }
    return 1;
  }

  case AST_INDEX_EXPRESSION: {
    ArrayIndexExpression *idx = (ArrayIndexExpression *)target->data;
    if (!idx || !idx->array || !idx->index) {
      code_generator_set_error(generator, "Invalid index lvalue");
      return 0;
    }

    Type *array_type =
        code_generator_infer_expression_type(generator, idx->array);
    if (!array_type ||
        (array_type->kind != TYPE_ARRAY && array_type->kind != TYPE_POINTER) ||
        !array_type->base_type) {
      code_generator_set_error(generator,
                               "Indexing requires array or pointer type");
      return 0;
    }

    code_generator_generate_array_element_address(generator, idx->array,
                                                  idx->index);
    if (generator->has_error) {
      return 0;
    }

    if (out_target_type) {
      *out_target_type = array_type->base_type;
    }
    return 1;
  }

  case AST_UNARY_EXPRESSION: {
    UnaryExpression *unary = (UnaryExpression *)target->data;
    if (!unary || !unary->operator || !unary->operand ||
        strcmp(unary->operator, "*") != 0) {
      code_generator_set_error(generator, "Invalid unary lvalue target");
      return 0;
    }

    Type *operand_type =
        code_generator_infer_expression_type(generator, unary->operand);
    if (!operand_type || operand_type->kind != TYPE_POINTER ||
        !operand_type->base_type) {
      code_generator_set_error(generator,
                               "Dereference assignment requires pointer type");
      return 0;
    }

    code_generator_generate_expression(generator, unary->operand);
    if (generator->has_error) {
      return 0;
    }

    code_generator_emit_null_check(generator,
                                   "Fatal error: Null pointer dereference");
    if (generator->has_error) {
      return 0;
    }

    if (out_target_type) {
      *out_target_type = operand_type->base_type;
    }
    return 1;
  }

  default:
    code_generator_set_error(generator, "Expression is not assignable");
    return 0;
  }
}

void code_generator_generate_array_index(CodeGenerator *generator,
                                         ASTNode *index_expression) {
  if (!generator || !index_expression ||
      index_expression->type != AST_INDEX_EXPRESSION) {
    code_generator_set_error(generator, "Invalid array index AST node");
    return;
  }

  ArrayIndexExpression *idx = (ArrayIndexExpression *)index_expression->data;
  if (!idx || !idx->array || !idx->index) {
    code_generator_set_error(generator, "Malformed array index expression");
    return;
  }

  Type *element_type = NULL;
  if (!code_generator_generate_lvalue_address(generator, index_expression,
                                              &element_type)) {
    return;
  }
  if (!element_type) {
    code_generator_set_error(generator, "Cannot resolve indexed element type");
    return;
  }
  if (element_type->kind == TYPE_STRUCT || element_type->kind == TYPE_ARRAY) {
    code_generator_set_error(generator,
                             "Aggregate index values are not supported");
    return;
  }

  int element_size = code_generator_get_type_storage_size(element_type);
  code_generator_emit_load_value_from_address(generator, element_size);
}

void code_generator_calculate_struct_layout(CodeGenerator *generator,
                                            Type *struct_type) {
  if (!generator || !struct_type || struct_type->kind != TYPE_STRUCT) {
    return;
  }

  if (!struct_type->field_names || !struct_type->field_types ||
      struct_type->field_count == 0) {
    struct_type->size = 0;
    struct_type->alignment = 1;
    return;
  }

  // Allocate field offsets array if not already allocated
  if (!struct_type->field_offsets) {
    struct_type->field_offsets =
        malloc(struct_type->field_count * sizeof(size_t));
    if (!struct_type->field_offsets) {
      return;
    }
  }

  size_t current_offset = 0;
  size_t max_alignment = 1;

  // Calculate field offsets with proper alignment
  for (size_t i = 0; i < struct_type->field_count; i++) {
    Type *field_type = struct_type->field_types[i];
    if (!field_type) {
      continue;
    }

    size_t field_size = field_type->size;
    size_t field_alignment = field_type->alignment;

    if (field_alignment == 0) {
      field_alignment = code_generator_calculate_struct_alignment(field_size);
    }

    // Update maximum alignment
    if (field_alignment > max_alignment) {
      max_alignment = field_alignment;
    }

    // Align current offset to field alignment
    if (field_alignment > 1) {
      current_offset =
          (current_offset + field_alignment - 1) & ~(field_alignment - 1);
    }

    // Set field offset
    struct_type->field_offsets[i] = current_offset;

    // Advance offset by field size
    current_offset += field_size;
  }

  // Align total struct size to maximum field alignment
  if (max_alignment > 1) {
    current_offset =
        (current_offset + max_alignment - 1) & ~(max_alignment - 1);
  }

  struct_type->size = current_offset;
  struct_type->alignment = max_alignment;
}

int code_generator_get_field_offset(CodeGenerator *generator, Type *struct_type,
                                    const char *field_name) {
  if (!generator || !struct_type || !field_name ||
      struct_type->kind != TYPE_STRUCT) {
    return -1;
  }

  if (!struct_type->field_names || !struct_type->field_offsets ||
      struct_type->field_count == 0) {
    return -1;
  }

  // Search for field by name
  for (size_t i = 0; i < struct_type->field_count; i++) {
    if (struct_type->field_names[i] &&
        strcmp(struct_type->field_names[i], field_name) == 0) {
      return (int)struct_type->field_offsets[i];
    }
  }

  return -1; // Field not found
}

int code_generator_calculate_struct_alignment(int field_size) {
  // Standard alignment rules for x86-64
  // Fields align to their natural size, but with maximum alignment of 8
  if (field_size >= 8) {
    return 8;
  } else if (field_size > 4) {
    return 8; // Fields larger than 4 bytes align to 8
  } else if (field_size > 2) {
    return 4; // Fields larger than 2 bytes align to 4
  } else if (field_size > 1) {
    return 2; // Fields larger than 1 byte align to 2
  } else {
    return 1;
  }
}
