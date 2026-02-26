#include "code_generator_internal.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static int code_generator_push_control_labels(CodeGenerator *generator,
                                              const char *break_label,
                                              const char *continue_label) {
  if (!generator) {
    return 0;
  }

  if (generator->control_flow_stack_size >=
      generator->control_flow_stack_capacity) {
    size_t new_capacity = generator->control_flow_stack_capacity == 0
                              ? 8
                              : generator->control_flow_stack_capacity * 2;
    char **new_break_stack = malloc(new_capacity * sizeof(char *));
    char **new_continue_stack = malloc(new_capacity * sizeof(char *));
    if (!new_break_stack || !new_continue_stack) {
      free(new_break_stack);
      free(new_continue_stack);
      code_generator_set_error(generator,
                               "Out of memory for control-flow label stack");
      return 0;
    }
    for (size_t i = 0; i < generator->control_flow_stack_size; i++) {
      new_break_stack[i] = generator->break_label_stack[i];
      new_continue_stack[i] = generator->continue_label_stack[i];
    }
    free(generator->break_label_stack);
    free(generator->continue_label_stack);
    generator->break_label_stack = new_break_stack;
    generator->continue_label_stack = new_continue_stack;
    generator->control_flow_stack_capacity = new_capacity;
  }

  size_t slot = generator->control_flow_stack_size++;
  generator->break_label_stack[slot] = break_label ? strdup(break_label) : NULL;
  generator->continue_label_stack[slot] =
      continue_label ? strdup(continue_label) : NULL;
  return 1;
}

static void code_generator_pop_control_labels(CodeGenerator *generator) {
  if (!generator || generator->control_flow_stack_size == 0) {
    return;
  }

  size_t slot = generator->control_flow_stack_size - 1;
  free(generator->break_label_stack[slot]);
  free(generator->continue_label_stack[slot]);
  generator->break_label_stack[slot] = NULL;
  generator->continue_label_stack[slot] = NULL;
  generator->control_flow_stack_size--;
}

static const char *
code_generator_current_break_label(CodeGenerator *generator) {
  if (!generator || generator->control_flow_stack_size == 0) {
    return NULL;
  }
  return generator->break_label_stack[generator->control_flow_stack_size - 1];
}

static const char *
code_generator_current_continue_label(CodeGenerator *generator) {
  if (!generator || generator->control_flow_stack_size == 0) {
    return NULL;
  }
  for (size_t i = generator->control_flow_stack_size; i > 0; i--) {
    const char *label = generator->continue_label_stack[i - 1];
    if (label) {
      return label;
    }
  }
  return NULL;
}

static int code_generator_eval_integer_constant(ASTNode *expression,
                                                long long *out_value) {
  if (!expression || !out_value) {
    return 0;
  }

  switch (expression->type) {
  case AST_NUMBER_LITERAL: {
    NumberLiteral *literal = (NumberLiteral *)expression->data;
    if (!literal || literal->is_float) {
      return 0;
    }
    *out_value = literal->int_value;
    return 1;
  }

  case AST_UNARY_EXPRESSION: {
    UnaryExpression *unary_expr = (UnaryExpression *)expression->data;
    long long operand = 0;
    if (!unary_expr || !unary_expr->operator || !unary_expr->operand ||
        !code_generator_eval_integer_constant(unary_expr->operand, &operand)) {
      return 0;
    }
    if (strcmp(unary_expr->operator, "+") == 0) {
      *out_value = operand;
      return 1;
    }
    if (strcmp(unary_expr->operator, "-") == 0) {
      *out_value = -operand;
      return 1;
    }
    return 0;
  }

  case AST_BINARY_EXPRESSION: {
    BinaryExpression *binary_expr = (BinaryExpression *)expression->data;
    long long left = 0;
    long long right = 0;
    if (!binary_expr || !binary_expr->operator || !binary_expr->left ||
        !binary_expr->right ||
        !code_generator_eval_integer_constant(binary_expr->left, &left) ||
        !code_generator_eval_integer_constant(binary_expr->right, &right)) {
      return 0;
    }

    if (strcmp(binary_expr->operator, "+") == 0) {
      *out_value = left + right;
      return 1;
    }
    if (strcmp(binary_expr->operator, "-") == 0) {
      *out_value = left - right;
      return 1;
    }
    if (strcmp(binary_expr->operator, "*") == 0) {
      *out_value = left * right;
      return 1;
    }
    if (strcmp(binary_expr->operator, "/") == 0) {
      if (right == 0) {
        return 0;
      }
      *out_value = left / right;
      return 1;
    }
    if (strcmp(binary_expr->operator, "%") == 0) {
      if (right == 0) {
        return 0;
      }
      *out_value = left % right;
      return 1;
    }
    return 0;
  }

  default:
    return 0;
  }
}

static IRFunction *code_generator_find_ir_function(CodeGenerator *generator,
                                                   const char *name) {
  if (!generator || !generator->ir_program || !name) {
    return NULL;
  }

  for (size_t i = 0; i < generator->ir_program->function_count; i++) {
    IRFunction *function = generator->ir_program->functions[i];
    if (function && function->name && strcmp(function->name, name) == 0) {
      return function;
    }
  }
  return NULL;
}

int code_generator_generate_program(CodeGenerator *generator,
                                    ASTNode *program) {
  if (!generator || !program) {
    return 0;
  }

  if (program->type != AST_PROGRAM) {
    code_generator_set_error(generator, "Expected AST_PROGRAM root node");
    return 0;
  }
  if (!generator->ir_program) {
    code_generator_set_error(generator,
                             "IR program not attached to code generator");
    return 0;
  }

  // Generate assembly header
  code_generator_emit(generator, "; Generated by MethASM\n");
  code_generator_emit(generator, "; x86-64 Assembly Output\n\n");

  // Generate text section first
  code_generator_emit_text_section(generator);

  // First pass: collect global variables and handle top-level inline assembly
  Program *program_data = (Program *)program->data;
  if (program_data) {
    code_generator_emit(generator,
                        "; First pass: processing %zu declarations\n",
                        program_data->declaration_count);
    for (size_t i = 0; i < program_data->declaration_count; i++) {
      ASTNode *declaration = program_data->declarations[i];
      code_generator_emit(generator,
                          "; Declaration %zu type: %d (AST_INLINE_ASM = %d)\n",
                          i, declaration->type, AST_INLINE_ASM);

      if (declaration->type == AST_VAR_DECLARATION) {
        VarDeclaration *var_data = (VarDeclaration *)declaration->data;
        if (var_data && var_data->is_extern) {
          const char *extern_name =
              code_generator_get_link_symbol_name(generator, var_data->name);
          if (!code_generator_emit_extern_symbol(generator, extern_name)) {
            return 0;
          }
        } else {
          code_generator_generate_global_variable(generator, declaration);
        }
      } else if (declaration->type == AST_INLINE_ASM) {
        // Top-level inline assembly - emit directly without register
        // preservation
        InlineAsm *asm_data = (InlineAsm *)declaration->data;
        if (asm_data && asm_data->assembly_code) {
          code_generator_emit(generator, "; Top-level inline assembly found\n");
          code_generator_emit(generator, "; Assembly: %s\n",
                              asm_data->assembly_code);
          code_generator_emit(generator, "%s\n", asm_data->assembly_code);
          code_generator_emit(generator, "\n");
        } else {
          code_generator_emit(generator,
                              "; Inline assembly node but no data\n");
        }
      }

      if (generator->has_error) {
        return 0;
      }
    }
  } else {
    code_generator_emit(generator, "; No program data found\n");
  }

  // Emit the collected global variables in data section
  if (generator->global_variables_size > 0) {
    code_generator_emit(generator, "\n; Data section for global variables\n");
    code_generator_emit(generator, "section .data\n");
    code_generator_emit(generator, "%s", generator->global_variables_buffer);
  }

  // Ensure all emitted functions and entrypoint stubs are placed in executable
  // code section even when globals switched the active section to .data/.bss.
  code_generator_emit(generator, "\nsection .text\n");

  // Second pass: process functions and other declarations
  if (program_data) {
    for (size_t i = 0; i < program_data->declaration_count; i++) {
      ASTNode *declaration = program_data->declarations[i];

      switch (declaration->type) {
      case AST_FUNCTION_DECLARATION: {
        FunctionDeclaration *function_data =
            declaration ? (FunctionDeclaration *)declaration->data : NULL;
        if (!function_data || !function_data->name) {
          code_generator_set_error(generator,
                                   "Malformed function declaration in AST");
          break;
        }
        if (function_data->is_extern) {
          const char *extern_name = code_generator_get_link_symbol_name(
              generator, function_data->name);
          if (!code_generator_emit_extern_symbol(generator, extern_name)) {
            break;
          }
          break;
        }
        if (!function_data->body) {
          break;
        }
        IRFunction *ir_function =
            code_generator_find_ir_function(generator, function_data->name);
        if (!ir_function) {
          code_generator_set_error(generator,
                                   "No IR body found for function '%s'",
                                   function_data->name);
          break;
        }
        if (!code_generator_generate_function_from_ir(generator, declaration,
                                                      ir_function)) {
          break;
        }
      } break;
      case AST_VAR_DECLARATION:
        // Already handled in first pass
        break;
      case AST_STRUCT_DECLARATION:
        // Generate struct type layout information
        code_generator_generate_struct_declaration(generator, declaration);
        break;
      case AST_INLINE_ASM:
        // Already handled in first pass
        break;
      default:
        // Other declarations
        break;
      }

      if (generator->has_error) {
        return 0;
      }
    }
  }

  int has_main = 0;
  if (program_data) {
    for (size_t i = 0; i < program_data->declaration_count; i++) {
      ASTNode *declaration = program_data->declarations[i];
      if (!declaration || declaration->type != AST_FUNCTION_DECLARATION) {
        continue;
      }

      FunctionDeclaration *func_data = (FunctionDeclaration *)declaration->data;
      if (func_data && func_data->name &&
          strcmp(func_data->name, "main") == 0) {
        has_main = 1;
        break;
      }
    }
  }

  // Generate process entry point.
  CallingConventionSpec *entry_conv_spec =
      generator->register_allocator->calling_convention;
  if (!entry_conv_spec) {
    code_generator_set_error(
        generator, "No calling convention configured for entry point");
    return 0;
  }

  // Determine the first integer parameter register for runtime calls
  const char *first_param_reg = "rdi"; // SysV default
  if (entry_conv_spec->int_param_count > 0) {
    const char *candidate = code_generator_get_register_name(
        entry_conv_spec->int_param_registers[0]);
    if (candidate) {
      first_param_reg = candidate;
    }
  }

  int is_ms_x64 = (entry_conv_spec->convention == CALLING_CONV_MS_X64);

  code_generator_emit(generator, "\n; Default program entry point\n");
  if (is_ms_x64) {
    // Microsoft x64: use mainCRTStartup as entry symbol
    code_generator_emit(generator, "global mainCRTStartup\n");
    code_generator_emit(generator, "mainCRTStartup:\n");
    code_generator_emit(generator,
                        "    sub rsp, 40      ; Shadow space + alignment\n");
  } else {
    // System V: use _start as entry symbol
    code_generator_emit(generator, "global _start\n");
    code_generator_emit(generator, "_start:\n");
  }

  code_generator_emit(generator,
                      "    ; Initialize garbage collector runtime\n");
  if (is_ms_x64) {
    // Anchor GC to the caller-visible stack top (before our 40-byte reserve).
    code_generator_emit(generator, "    lea %s, [rsp + 40]\n", first_param_reg);
  } else {
    code_generator_emit(generator, "    mov %s, rsp\n", first_param_reg);
  }
  code_generator_emit(generator, "    extern gc_init\n");
  code_generator_emit(generator, "    call gc_init\n");

  // Register global root slots for pointer-like globals.
  if (program_data) {
    int emitted_gc_root_extern = 0;
    for (size_t i = 0; i < program_data->declaration_count; i++) {
      ASTNode *declaration = program_data->declarations[i];
      if (!declaration || declaration->type != AST_VAR_DECLARATION) {
        continue;
      }

      VarDeclaration *var_data = (VarDeclaration *)declaration->data;
      if (!var_data || !var_data->name) {
        continue;
      }
      if (var_data->is_extern) {
        continue;
      }

      int should_register = 0;
      Symbol *symbol =
          symbol_table_lookup(generator->symbol_table, var_data->name);
      if (symbol && symbol->type) {
        switch (symbol->type->kind) {
        case TYPE_POINTER:
        case TYPE_ARRAY:
        case TYPE_STRING:
        case TYPE_STRUCT:
          should_register = 1;
          break;
        default:
          should_register = 0;
          break;
        }
      } else {
        code_generator_set_error(
            generator,
            "Global variable '%s' is missing type information during "
            "root-registration",
            var_data->name);
        return 0;
      }

      if (!should_register) {
        continue;
      }

      if (!emitted_gc_root_extern) {
        code_generator_emit(generator, "    extern gc_register_root\n");
        emitted_gc_root_extern = 1;
      }

      code_generator_emit(generator, "    lea %s, [rel %s]\n",
                          first_param_reg, var_data->name);
      code_generator_emit(generator, "    call gc_register_root\n");
    }
  }

  if (has_main) {
    code_generator_emit(generator, "    ; Call user main function\n");
    code_generator_emit(generator, "    call main\n");
    if (is_ms_x64) {
      code_generator_emit(
          generator, "    mov [rsp + 32], rax ; Preserve main return code\n");
    } else {
      code_generator_emit(generator,
                          "    push rax         ; Preserve main return code\n");
    }
    code_generator_emit(generator, "    extern gc_shutdown\n");
    code_generator_emit(generator, "    call gc_shutdown\n");
    if (is_ms_x64) {
      code_generator_emit(
          generator, "    mov %s, [rsp + 32] ; Use main return as exit code\n",
          first_param_reg);
    } else {
      code_generator_emit(
          generator, "    pop rdi          ; Use main return as exit code\n");
    }
  } else {
    code_generator_emit(generator, "    extern gc_shutdown\n");
    code_generator_emit(generator, "    call gc_shutdown\n");
    code_generator_emit(generator,
                        "    mov %s, 0       ; Default exit status\n",
                        first_param_reg);
  }

  if (is_ms_x64) {
    // Windows: call ExitProcess from kernel32
    code_generator_emit(generator, "    extern ExitProcess\n");
    code_generator_emit(generator, "    call ExitProcess\n");
  } else {
    // Linux: use syscall to exit
    code_generator_emit(generator, "    mov rax, 60    ; sys_exit\n");
    code_generator_emit(generator, "    syscall\n");
  }

  return generator->has_error ? 0 : 1;
}

// Forward declaration
void code_generator_generate_statement(CodeGenerator *generator,
                                       ASTNode *statement);

void code_generator_register_function_parameters(CodeGenerator *generator,
                                                 FunctionDeclaration *func_data,
                                                 int parameter_home_size) {
  if (!generator || !func_data) {
    return;
  }

  code_generator_emit(generator, "    ; Registering %zu function parameters\n",
                      func_data->parameter_count);

  CallingConventionSpec *conv_spec =
      generator->register_allocator->calling_convention;
  size_t int_param_reg_index = 0;
  size_t float_param_reg_index = 0;
  int stack_offset = 16; // Start after return address and saved RBP

  for (size_t i = 0; i < func_data->parameter_count; i++) {
    const char *param_name = func_data->parameter_names[i];
    const char *type_name = func_data->parameter_types[i];

    Type *resolved_param_type = NULL;
    if (type_name) {
      resolved_param_type =
          type_checker_get_type_by_name(generator->type_checker, type_name);
    }
    if (!resolved_param_type) {
      resolved_param_type =
          type_checker_get_type_by_name(generator->type_checker, "int64");
    }

    Symbol *param_symbol =
        symbol_table_lookup_current_scope(generator->symbol_table, param_name);
    if (!param_symbol) {
      param_symbol =
          symbol_create(param_name, SYMBOL_PARAMETER, resolved_param_type);
      if (!param_symbol ||
          !symbol_table_declare(generator->symbol_table, param_symbol)) {
        symbol_destroy(param_symbol);
        code_generator_set_error(generator, "Failed to register parameter '%s'",
                                 param_name ? param_name : "<unnamed>");
        return;
      }
    } else if (!param_symbol->type) {
      param_symbol->type = resolved_param_type;
    }

    // Determine if parameter is in register or on stack
    int is_in_register = 0;
    int register_id = -1;
    int incoming_stack_offset = 0;

    Type *param_type = param_symbol->type;
    int is_float = code_generator_is_floating_point_type(param_type);

    if (is_float && float_param_reg_index < conv_spec->float_param_count) {
      is_in_register = 1;
      register_id = conv_spec->float_param_registers[float_param_reg_index++];
    } else if (!is_float && int_param_reg_index < conv_spec->int_param_count) {
      is_in_register = 1;
      register_id = conv_spec->int_param_registers[int_param_reg_index++];
    } else {
      // Parameter is on the stack
      incoming_stack_offset = stack_offset;
      stack_offset += 8; // Assuming 8-byte parameters
    }

    // Materialize all parameters into stable stack homes so '&param' is
    // valid.
    int home_offset = (int)((i + 1) * 8);
    if (home_offset > parameter_home_size) {
      code_generator_set_error(generator,
                               "Parameter home slot overflow for '%s'",
                               param_name ? param_name : "<unnamed>");
      return;
    }

    param_symbol->data.variable.is_in_register = 0;
    param_symbol->data.variable.memory_offset = home_offset;

    if (is_in_register) {
      const char *reg_name =
          code_generator_get_register_name((x86Register)register_id);
      if (!reg_name) {
        code_generator_set_error(generator,
                                 "Invalid register for parameter '%s'",
                                 param_name ? param_name : "<unnamed>");
        return;
      }

      if (is_float) {
        if (param_type && param_type->size == 4) {
          code_generator_emit(generator,
                              "    movss [rbp - %d], %s  ; Home param '%s'\n",
                              home_offset, reg_name, param_name);
        } else {
          code_generator_emit(generator,
                              "    movsd [rbp - %d], %s  ; Home param '%s'\n",
                              home_offset, reg_name, param_name);
        }
      } else {
        code_generator_emit(generator,
                            "    mov [rbp - %d], %s  ; Home param '%s'\n",
                            home_offset, reg_name, param_name);
      }

      code_generator_emit(generator,
                          "    ; Parameter '%s' arrived in register %s\n",
                          param_name, reg_name);
    } else {
      code_generator_emit(generator,
                          "    mov rax, [rbp + %d]  ; Load stack param '%s'\n",
                          incoming_stack_offset, param_name);
      code_generator_emit(generator,
                          "    mov [rbp - %d], rax  ; Home param '%s'\n",
                          home_offset, param_name);
      code_generator_emit(generator,
                          "    ; Parameter '%s' arrived on stack [rbp + %d]\n",
                          param_name, incoming_stack_offset);
    }
  }
}

void code_generator_generate_function(CodeGenerator *generator,
                                      ASTNode *function) {
  if (!generator || !function || function->type != AST_FUNCTION_DECLARATION) {
    return;
  }

  FunctionDeclaration *func_data = (FunctionDeclaration *)function->data;
  if (!func_data || !func_data->name) {
    return;
  }

  // Enter a new scope for the function
  symbol_table_enter_scope(generator->symbol_table, SCOPE_FUNCTION);

  // Add debug symbol for function
  if (generator->generate_debug_info) {
    code_generator_add_debug_symbol(
        generator, func_data->name, DEBUG_SYMBOL_FUNCTION,
        func_data->return_type, function->location.line,
        function->location.column);

    // Add line mapping for function start
    code_generator_add_line_mapping(generator, function->location.line,
                                    function->location.column,
                                    generator->debug_info->source_filename);
  }

  code_generator_emit(generator, "\nglobal %s\n", func_data->name);

  // Pre-pass to register all local variables and calculate stack size
  int stack_size = 0;
  int parameter_home_size = 0;
  if (func_data->parameter_count > 0) {
    if (func_data->parameter_count > (size_t)(INT_MAX / 8)) {
      code_generator_set_error(
          generator, "Too many parameters in function '%s'", func_data->name);
      symbol_table_exit_scope(generator->symbol_table);
      return;
    }
    parameter_home_size = (int)(func_data->parameter_count * 8);
    stack_size += parameter_home_size;
  }

  if (func_data->body) {
    Program *body_prog = (Program *)func_data->body->data;
    if (body_prog) {
      for (size_t i = 0; i < body_prog->declaration_count; i++) {
        ASTNode *stmt = body_prog->declarations[i];
        if (stmt && stmt->type == AST_VAR_DECLARATION) {
          VarDeclaration *var_decl = (VarDeclaration *)stmt->data;
          if (var_decl) {
            Type *var_type = NULL;
            if (var_decl->type_name) {
              var_type = type_checker_get_type_by_name(generator->type_checker,
                                                       var_decl->type_name);
            } else if (var_decl->initializer) {
              var_type = type_checker_infer_type(generator->type_checker,
                                                 var_decl->initializer);
            }
            if (!var_type) {
              var_type = type_checker_get_type_by_name(generator->type_checker,
                                                       "int64");
            }

            int var_size = 0;
            if (var_type && var_type->size > 0) {
              var_size = (int)var_type->size;
            } else {
              var_size = code_generator_calculate_variable_size(
                  generator, var_decl->type_name);
            }
            if (var_size <= 0) {
              var_size = 8;
            }
            stack_size += var_size;

            // Register symbol without generating code yet.
            Symbol *existing = symbol_table_lookup_current_scope(
                generator->symbol_table, var_decl->name);
            if (!existing) {
              Symbol *var_symbol =
                  symbol_create(var_decl->name, SYMBOL_VARIABLE, var_type);
              if (var_symbol &&
                  !symbol_table_declare(generator->symbol_table, var_symbol)) {
                symbol_destroy(var_symbol);
              }
            }
          }
        }
      }
    }
  }

  // Generate function prologue
  code_generator_function_prologue(generator, func_data->name, stack_size);

  // Reserve lower stack slots for parameter homes.
  generator->current_stack_offset = parameter_home_size;

  // Register parameters in symbol table
  code_generator_register_function_parameters(generator, func_data,
                                              parameter_home_size);

  // Generate function body
  if (func_data->body) {
    code_generator_generate_statement(generator, func_data->body);
  }

  // Add a label for the function exit
  code_generator_emit(generator, "L%s_exit:\n", func_data->name);

  // Generate function epilogue
  code_generator_function_epilogue(generator);

  // Exit the function's scope
  symbol_table_exit_scope(generator->symbol_table);
}

void code_generator_generate_statement(CodeGenerator *generator,
                                       ASTNode *statement) {
  if (!generator || !statement || generator->has_error) {
    return;
  }

  // Add line mapping for statement if debug info is enabled
  if (generator->generate_debug_info && statement->location.line > 0) {
    code_generator_add_line_mapping(generator, statement->location.line,
                                    statement->location.column,
                                    generator->debug_info->source_filename);
    code_generator_emit_debug_label(generator, statement->location.line);
  }

  switch (statement->type) {
  case AST_PROGRAM: // This is a block
  {
    Program *block = (Program *)statement->data;
    if (block) {
      for (size_t i = 0; i < block->declaration_count; i++) {
        code_generator_generate_statement(generator, block->declarations[i]);
      }
    }
  } break;
  case AST_VAR_DECLARATION: {
    // Local variable declaration inside function
    code_generator_generate_local_variable(generator, statement);
  } break;

  case AST_ASSIGNMENT: {
    // Assignment statement
    code_generator_generate_assignment_statement(generator, statement);
  } break;

  case AST_FUNCTION_CALL: {
    // Function call as statement (not expression)
    code_generator_generate_function_call(generator, statement);
  } break;

  case AST_RETURN_STATEMENT: {
    ReturnStatement *return_data = (ReturnStatement *)statement->data;
    code_generator_emit(generator, "    ; Return statement\n");

    if (return_data && return_data->value) {
      // Generate the return value expression
      code_generator_generate_expression(generator, return_data->value);
      // Result is already in RAX (the return register for integers)
      code_generator_emit(generator, "    ; Return value in rax\n");
    } else {
      // Void return - no value to return
      code_generator_emit(generator, "    ; Void return\n");
    }

    // Jump to function epilogue (or inline it)
    code_generator_emit(generator, "    jmp L%s_exit\n",
                        generator->current_function_name);
  } break;

  case AST_INLINE_ASM: {
    // Inline assembly within function
    code_generator_generate_inline_assembly(generator, statement);
  } break;

  case AST_BINARY_EXPRESSION: {
    // Binary expression as a statement (value is discarded)
    code_generator_generate_expression(generator, statement);
  } break;

  case AST_IF_STATEMENT: {
    IfStatement *if_data = (IfStatement *)statement->data;
    if (if_data && if_data->condition && if_data->then_branch) {
      char *else_label = code_generator_generate_label(generator, "if_else");
      char *end_label = code_generator_generate_label(generator, "if_end");
      if (!else_label || !end_label)
        break;

      code_generator_generate_expression(generator, if_data->condition);
      code_generator_emit(generator,
                          "    test rax, rax      ; Test condition\n");
      code_generator_emit(generator, "    jz %s              ; Jump if false\n",
                          else_label);

      code_generator_generate_statement(generator, if_data->then_branch);
      code_generator_emit(generator, "    jmp %s              ; Skip else\n",
                          end_label);

      code_generator_emit(generator, "%s:\n", else_label);
      if (if_data->else_branch) {
        code_generator_generate_statement(generator, if_data->else_branch);
      }

      code_generator_emit(generator, "%s:\n", end_label);
      free(else_label);
      free(end_label);
    } else {
      code_generator_set_error(generator, "Malformed if statement");
    }
  } break;

  case AST_WHILE_STATEMENT: {
    WhileStatement *while_data = (WhileStatement *)statement->data;
    if (while_data && while_data->condition && while_data->body) {
      char *loop_start = code_generator_generate_label(generator, "while");
      char *loop_end = code_generator_generate_label(generator, "while_end");
      if (!loop_start || !loop_end)
        break;

      if (!code_generator_push_control_labels(generator, loop_end,
                                              loop_start)) {
        free(loop_start);
        free(loop_end);
        break;
      }

      code_generator_emit(generator, "%s:\n", loop_start);
      code_generator_generate_expression(generator, while_data->condition);
      code_generator_emit(generator,
                          "    test rax, rax      ; Test condition\n");
      code_generator_emit(
          generator, "    jz %s              ; Exit loop if false\n", loop_end);

      code_generator_generate_statement(generator, while_data->body);
      code_generator_emit(generator, "    jmp %s              ; Loop back\n",
                          loop_start);

      code_generator_emit(generator, "%s:\n", loop_end);
      code_generator_pop_control_labels(generator);
      free(loop_start);
      free(loop_end);
    } else {
      code_generator_set_error(generator, "Malformed while statement");
    }
  } break;

  case AST_FOR_STATEMENT: {
    ForStatement *for_data = (ForStatement *)statement->data;
    if (!for_data || !for_data->body) {
      code_generator_set_error(generator, "Malformed for statement");
      break;
    }

    char *loop_cond = code_generator_generate_label(generator, "for_cond");
    char *loop_step = code_generator_generate_label(generator, "for_step");
    char *loop_end = code_generator_generate_label(generator, "for_end");
    if (!loop_cond || !loop_step || !loop_end) {
      free(loop_cond);
      free(loop_step);
      free(loop_end);
      break;
    }

    if (!code_generator_push_control_labels(generator, loop_end, loop_step)) {
      free(loop_cond);
      free(loop_step);
      free(loop_end);
      break;
    }

    if (for_data->initializer) {
      if (for_data->initializer->type == AST_VAR_DECLARATION ||
          for_data->initializer->type == AST_ASSIGNMENT ||
          for_data->initializer->type == AST_FUNCTION_CALL ||
          for_data->initializer->type == AST_PROGRAM) {
        code_generator_generate_statement(generator, for_data->initializer);
      } else {
        code_generator_generate_expression(generator, for_data->initializer);
      }
    }

    code_generator_emit(generator, "%s:\n", loop_cond);
    if (for_data->condition) {
      code_generator_generate_expression(generator, for_data->condition);
      code_generator_emit(generator,
                          "    test rax, rax      ; Test for-loop condition\n");
      code_generator_emit(generator, "    jz %s              ; Exit for-loop\n",
                          loop_end);
    }

    code_generator_generate_statement(generator, for_data->body);

    code_generator_emit(generator, "%s:\n", loop_step);
    if (for_data->increment) {
      if (for_data->increment->type == AST_ASSIGNMENT) {
        code_generator_generate_statement(generator, for_data->increment);
      } else {
        code_generator_generate_expression(generator, for_data->increment);
      }
    }
    code_generator_emit(generator, "    jmp %s              ; Next iteration\n",
                        loop_cond);
    code_generator_emit(generator, "%s:\n", loop_end);

    code_generator_pop_control_labels(generator);
    free(loop_cond);
    free(loop_step);
    free(loop_end);
  } break;

  case AST_SWITCH_STATEMENT: {
    SwitchStatement *switch_data = (SwitchStatement *)statement->data;
    if (!switch_data || !switch_data->expression) {
      code_generator_set_error(generator, "Malformed switch statement");
      break;
    }

    char *switch_end = code_generator_generate_label(generator, "switch_end");
    if (!switch_end) {
      break;
    }
    if (!code_generator_push_control_labels(generator, switch_end, NULL)) {
      free(switch_end);
      break;
    }

    code_generator_generate_expression(generator, switch_data->expression);
    code_generator_emit(generator,
                        "    mov r10, rax       ; Save switch value\n");

    char **case_labels = NULL;
    size_t case_count = switch_data->case_count;
    size_t default_index = (size_t)-1;
    if (case_count > 0) {
      case_labels = malloc(case_count * sizeof(char *));
      if (!case_labels) {
        code_generator_pop_control_labels(generator);
        free(switch_end);
        code_generator_set_error(generator,
                                 "Out of memory while generating switch");
        break;
      }
      for (size_t i = 0; i < case_count; i++) {
        case_labels[i] = NULL;
      }
    }

    for (size_t i = 0; i < case_count; i++) {
      ASTNode *case_node = switch_data->cases ? switch_data->cases[i] : NULL;
      CaseClause *case_clause =
          case_node ? (CaseClause *)case_node->data : NULL;
      case_labels[i] = code_generator_generate_label(generator, "case");
      if (!case_labels[i]) {
        code_generator_set_error(generator,
                                 "Failed to allocate label for switch case");
        break;
      }
      if (case_clause && case_clause->is_default) {
        default_index = i;
        continue;
      }
      if (case_clause && case_clause->value) {
        long long value = 0;
        if (!code_generator_eval_integer_constant(case_clause->value, &value)) {
          code_generator_set_error(
              generator, "Case value must be a compile-time integer constant");
          break;
        }

        code_generator_emit(generator, "    cmp r10, %lld\n", value);
        code_generator_emit(generator, "    je %s\n", case_labels[i]);
      } else {
        code_generator_set_error(generator, "Malformed switch case");
        break;
      }
    }

    if (generator->has_error) {
      code_generator_pop_control_labels(generator);
      if (case_labels) {
        for (size_t i = 0; i < case_count; i++) {
          free(case_labels[i]);
        }
        free(case_labels);
      }
      free(switch_end);
      break;
    }

    if (default_index != (size_t)-1 && case_labels &&
        case_labels[default_index]) {
      code_generator_emit(generator, "    jmp %s\n",
                          case_labels[default_index]);
    } else {
      code_generator_emit(generator, "    jmp %s\n", switch_end);
    }

    for (size_t i = 0; i < case_count; i++) {
      ASTNode *case_node = switch_data->cases ? switch_data->cases[i] : NULL;
      CaseClause *case_clause =
          case_node ? (CaseClause *)case_node->data : NULL;
      if (!case_labels || !case_labels[i] || !case_clause ||
          !case_clause->body) {
        continue;
      }
      code_generator_emit(generator, "%s:\n", case_labels[i]);
      code_generator_generate_statement(generator, case_clause->body);
    }

    code_generator_emit(generator, "%s:\n", switch_end);
    code_generator_pop_control_labels(generator);
    if (case_labels) {
      for (size_t i = 0; i < case_count; i++) {
        free(case_labels[i]);
      }
      free(case_labels);
    }
    free(switch_end);
  } break;

  case AST_BREAK_STATEMENT: {
    const char *break_label = code_generator_current_break_label(generator);
    if (!break_label) {
      code_generator_set_error(generator,
                               "'break' used outside loop/switch in codegen");
      break;
    }
    code_generator_emit(generator, "    jmp %s\n", break_label);
  } break;

  case AST_CONTINUE_STATEMENT: {
    const char *continue_label =
        code_generator_current_continue_label(generator);
    if (!continue_label) {
      code_generator_set_error(generator,
                               "'continue' used outside loop in codegen");
      break;
    }
    code_generator_emit(generator, "    jmp %s\n", continue_label);
  } break;

  default:
    code_generator_set_error(generator, "Unhandled statement type: %d",
                             statement->type);
    break;
  }
}

void code_generator_generate_expression(CodeGenerator *generator,
                                        ASTNode *expression) {
  if (!generator || !expression || generator->has_error) {
    return;
  }

  switch (expression->type) {
  case AST_NUMBER_LITERAL: {
    NumberLiteral *num_data = (NumberLiteral *)expression->data;
    if (num_data) {
      if (num_data->is_float) {
        code_generator_emit(generator, "    ; Float literal: %f\n",
                            num_data->float_value);
        // Load float value into XMM0 register
        char *float_label = code_generator_generate_label(generator, "float");
        if (float_label) {
          code_generator_emit(generator,
                              "    movsd xmm0, [rel %s]  ; Load float from "
                              "memory\n",
                              float_label);

          // Add the float literal to the global data section
          code_generator_emit_to_global_buffer(generator, "%s:\n", float_label);
          code_generator_emit_to_global_buffer(
              generator, "    dq 0x%016llx  ; float64: %f\n",
              *(long long *)&num_data->float_value, num_data->float_value);

          free(float_label);
        }
      } else {
        code_generator_emit(generator, "    ; Integer literal: %lld\n",
                            num_data->int_value);
        code_generator_emit(generator, "    mov rax, %lld\n",
                            num_data->int_value);
      }
    } else {
      code_generator_set_error(generator, "Malformed number literal");
    }
  } break;

  case AST_STRING_LITERAL: {
    StringLiteral *str_data = (StringLiteral *)expression->data;
    if (str_data && str_data->value) {
      code_generator_load_string_literal(generator, str_data->value);
    } else {
      code_generator_set_error(generator, "Malformed string literal");
    }
  } break;

  case AST_IDENTIFIER: {
    Identifier *id_data = (Identifier *)expression->data;
    if (id_data && id_data->name) {
      code_generator_load_variable(generator, id_data->name);
    } else {
      code_generator_set_error(generator, "Malformed identifier expression");
    }
  } break;

  case AST_BINARY_EXPRESSION: {
    BinaryExpression *bin_data = (BinaryExpression *)expression->data;
    if (bin_data && bin_data->left && bin_data->right && bin_data->operator) {
      code_generator_generate_binary_operation(
          generator, bin_data->left, bin_data->operator, bin_data->right);
    } else {
      code_generator_set_error(generator, "Malformed binary expression");
    }
  } break;

  case AST_UNARY_EXPRESSION: {
    UnaryExpression *unary_data = (UnaryExpression *)expression->data;
    if (unary_data && unary_data->operand && unary_data->operator) {
      code_generator_generate_unary_operation(generator, unary_data->operator,
                                              unary_data->operand);
    } else {
      code_generator_set_error(generator, "Malformed unary expression");
    }
  } break;

  case AST_FUNCTION_CALL: {
    // Function call as expression (returns value)
    code_generator_generate_function_call(generator, expression);
  } break;

  case AST_MEMBER_ACCESS: {
    // Member access (struct.field)
    code_generator_generate_member_access(generator, expression);
  } break;

  case AST_INDEX_EXPRESSION: {
    code_generator_generate_array_index(generator, expression);
  } break;

  case AST_NEW_EXPRESSION: {
    NewExpression *new_expr = (NewExpression *)expression->data;
    if (new_expr && new_expr->type_name) {
      code_generator_emit(generator, "    ; Heap allocation: new %s\n",
                          new_expr->type_name);

      // Determine the size of the type being allocated
      int alloc_size = code_generator_calculate_variable_size(
          generator, new_expr->type_name);
      if (alloc_size <= 0) {
        // Fallback size if lookup fails
        alloc_size = 8;
      }

      // Call gc_alloc(alloc_size)
      code_generator_emit(generator, "    mov rdi, %d      ; size in bytes\n",
                          alloc_size);
      code_generator_emit(generator, "    extern gc_alloc\n");
      code_generator_emit(generator, "    call gc_alloc\n");
      // The allocated memory pointer is returned in RAX,
      // ready for variable assignments or immediate struct usage.
    } else {
      code_generator_set_error(generator, "Malformed new-expression");
    }
  } break;

  default:
    code_generator_set_error(generator, "Unhandled expression type: %d",
                             expression->type);
    break;
  }
}
