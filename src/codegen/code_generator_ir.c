#include "code_generator_internal.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *name;
  int offset;
} IRTempSlot;

typedef struct {
  IRTempSlot *items;
  size_t count;
  size_t capacity;
} IRTempTable;

typedef struct {
  char *name;
  const char *type_name;
  SourceLocation location;
  int size;
  int alignment;
} IRLocalSlot;

typedef struct {
  IRLocalSlot *items;
  size_t count;
  size_t capacity;
} IRLocalTable;

static int ir_local_table_add(IRLocalTable *table, const char *name,
                              const char *type_name, SourceLocation location,
                              int size, int alignment) {
  if (!table || !name || size <= 0) {
    return 0;
  }

  for (size_t i = 0; i < table->count; i++) {
    if (table->items[i].name && strcmp(table->items[i].name, name) == 0) {
      return 1;
    }
  }

  if (table->count >= table->capacity) {
    size_t new_capacity = table->capacity == 0 ? 16 : table->capacity * 2;
    IRLocalSlot *new_items =
        realloc(table->items, new_capacity * sizeof(IRLocalSlot));
    if (!new_items) {
      return 0;
    }
    table->items = new_items;
    table->capacity = new_capacity;
  }

  size_t name_len = strlen(name) + 1;
  char *name_copy = malloc(name_len);
  if (!name_copy) {
    return 0;
  }
  memcpy(name_copy, name, name_len);

  IRLocalSlot *slot = &table->items[table->count++];
  slot->name = name_copy;
  slot->type_name = type_name;
  slot->location = location;
  slot->size = size;
  slot->alignment = alignment;
  return 1;
}

static void ir_local_table_destroy(IRLocalTable *table) {
  if (!table) {
    return;
  }
  for (size_t i = 0; i < table->count; i++) {
    free(table->items[i].name);
  }
  free(table->items);
  table->items = NULL;
  table->count = 0;
  table->capacity = 0;
}

static int ir_temp_table_get_offset(IRTempTable *table, const char *name) {
  if (!table || !name) {
    return -1;
  }
  for (size_t i = 0; i < table->count; i++) {
    if (table->items[i].name && strcmp(table->items[i].name, name) == 0) {
      return table->items[i].offset;
    }
  }
  return -1;
}

static int ir_temp_table_add(IRTempTable *table, const char *name) {
  if (!table || !name) {
    return 0;
  }

  if (ir_temp_table_get_offset(table, name) >= 0) {
    return 1;
  }

  if (table->count >= table->capacity) {
    size_t new_capacity = table->capacity == 0 ? 32 : table->capacity * 2;
    IRTempSlot *new_items =
        realloc(table->items, new_capacity * sizeof(IRTempSlot));
    if (!new_items) {
      return 0;
    }
    table->items = new_items;
    table->capacity = new_capacity;
  }

  size_t name_len = strlen(name) + 1;
  char *name_copy = malloc(name_len);
  if (!name_copy) {
    return 0;
  }
  memcpy(name_copy, name, name_len);

  table->items[table->count].name = name_copy;
  table->items[table->count].offset = -1;
  table->count++;
  return 1;
}

static void ir_temp_table_destroy(IRTempTable *table) {
  if (!table) {
    return;
  }
  for (size_t i = 0; i < table->count; i++) {
    free(table->items[i].name);
  }
  free(table->items);
  table->items = NULL;
  table->count = 0;
  table->capacity = 0;
}

static int code_generator_load_ir_operand(CodeGenerator *generator,
                                          const IROperand *operand,
                                          IRTempTable *temp_table) {
  if (!generator || !operand) {
    return 0;
  }

  switch (operand->kind) {
  case IR_OPERAND_NONE:
    code_generator_emit(generator, "    mov rax, 0\n");
    return 1;

  case IR_OPERAND_INT:
    code_generator_emit(generator, "    mov rax, %lld\n", operand->int_value);
    return 1;

  case IR_OPERAND_FLOAT: {
    union {
      double value;
      unsigned long long bits;
    } converter;
    converter.value = operand->float_value;
    code_generator_emit(generator, "    mov rax, 0x%016llx\n", converter.bits);
    return 1;
  }

  case IR_OPERAND_STRING:
    if (!operand->name) {
      code_generator_set_error(generator, "Malformed IR string operand");
      return 0;
    }
    code_generator_load_string_literal(generator, operand->name);
    return 1;

  case IR_OPERAND_SYMBOL:
    if (!operand->name) {
      code_generator_set_error(generator, "Malformed IR symbol operand");
      return 0;
    }
    code_generator_load_variable(generator, operand->name);
    return !generator->has_error;

  case IR_OPERAND_TEMP: {
    if (!operand->name) {
      code_generator_set_error(generator, "Malformed IR temp operand");
      return 0;
    }
    int offset = ir_temp_table_get_offset(temp_table, operand->name);
    if (offset <= 0) {
      code_generator_set_error(generator, "Unknown IR temp '%s'",
                               operand->name);
      return 0;
    }
    code_generator_emit(generator, "    mov rax, [rbp - %d]\n", offset);
    return 1;
  }

  case IR_OPERAND_LABEL:
    if (!operand->name) {
      code_generator_set_error(generator, "Malformed IR label operand");
      return 0;
    }
    code_generator_emit(generator, "    lea rax, [rel %s]\n", operand->name);
    return 1;

  default:
    code_generator_set_error(generator, "Unhandled IR operand kind: %d",
                             operand->kind);
    return 0;
  }
}

static int code_generator_store_ir_destination(CodeGenerator *generator,
                                               const IROperand *destination,
                                               IRTempTable *temp_table) {
  if (!generator || !destination) {
    return 0;
  }

  switch (destination->kind) {
  case IR_OPERAND_NONE:
    return 1;

  case IR_OPERAND_SYMBOL:
    if (!destination->name) {
      code_generator_set_error(generator, "Malformed IR destination symbol");
      return 0;
    }
    code_generator_store_variable(generator, destination->name, "rax");
    return !generator->has_error;

  case IR_OPERAND_TEMP: {
    if (!destination->name) {
      code_generator_set_error(generator, "Malformed IR destination temp");
      return 0;
    }
    int offset = ir_temp_table_get_offset(temp_table, destination->name);
    if (offset <= 0) {
      code_generator_set_error(generator, "Unknown IR temp '%s'",
                               destination->name);
      return 0;
    }
    code_generator_emit(generator, "    mov [rbp - %d], rax\n", offset);
    return 1;
  }

  default:
    code_generator_set_error(generator, "Invalid IR destination kind: %d",
                             destination->kind);
    return 0;
  }
}

static int code_generator_get_ir_access_size(CodeGenerator *generator,
                                             const IROperand *size_operand) {
  if (!generator || !size_operand || size_operand->kind != IR_OPERAND_INT) {
    code_generator_set_error(generator,
                             "IR memory access width must be integer");
    return 0;
  }

  long long size = size_operand->int_value;
  if (size > 0) {
    return (int)size;
  }

  code_generator_set_error(generator, "Invalid IR memory access width: %lld",
                           size);
  return 0;
}

static void code_generator_emit_ir_load_from_address(CodeGenerator *generator,
                                                     int size) {
  if (!generator) {
    return;
  }

  switch (size) {
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

static void code_generator_emit_ir_store_to_address(CodeGenerator *generator,
                                                    int size) {
  if (!generator) {
    return;
  }

  if (size > 8) {
    code_generator_emit(generator, "    push rsi\n");
    code_generator_emit(generator, "    push rdi\n");
    code_generator_emit(generator, "    mov rsi, rcx\n");
    code_generator_emit(generator, "    mov rdi, rax\n");
    code_generator_emit(generator, "    mov rcx, %d\n", size);
    code_generator_emit(generator, "    rep movsb\n");
    code_generator_emit(generator, "    pop rdi\n");
    code_generator_emit(generator, "    pop rsi\n");
    return;
  }

  switch (size) {
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

static int code_generator_emit_ir_address_of(CodeGenerator *generator,
                                             const IRInstruction *instruction,
                                             IRTempTable *temp_table) {
  if (!generator || !instruction ||
      instruction->lhs.kind != IR_OPERAND_SYMBOL || !instruction->lhs.name) {
    code_generator_set_error(generator, "IR addr_of requires symbol operand");
    return 0;
  }

  Symbol *symbol =
      symbol_table_lookup(generator->symbol_table, instruction->lhs.name);
  if (!symbol ||
      (symbol->kind != SYMBOL_VARIABLE && symbol->kind != SYMBOL_PARAMETER)) {
    code_generator_set_error(generator, "Unknown addr_of symbol '%s'",
                             instruction->lhs.name);
    return 0;
  }

  if (symbol->data.variable.is_in_register) {
    code_generator_set_error(
        generator, "IR addr_of cannot target register-allocated symbol '%s'",
        instruction->lhs.name);
    return 0;
  }

  if (symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
    const char *symbol_name =
        code_generator_get_link_symbol_name(generator, instruction->lhs.name);
    if (!symbol_name) {
      code_generator_set_error(generator,
                               "Invalid global symbol in IR addr_of");
      return 0;
    }
    if (symbol->is_extern && !code_generator_emit_extern_symbol(generator, symbol_name)) {
      return 0;
    }
    code_generator_emit(generator, "    lea rax, [rel %s]\n", symbol_name);
  } else {
    code_generator_emit(generator, "    lea rax, [rbp - %d]\n",
                        symbol->data.variable.memory_offset);
  }

  return code_generator_store_ir_destination(generator, &instruction->dest,
                                             temp_table);
}

static int code_generator_emit_ir_load(CodeGenerator *generator,
                                       const IRInstruction *instruction,
                                       IRTempTable *temp_table) {
  if (!generator || !instruction) {
    return 0;
  }

  int size = code_generator_get_ir_access_size(generator, &instruction->rhs);
  if (size <= 0) {
    return 0;
  }

  if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                      temp_table)) {
    return 0;
  }

  code_generator_emit_ir_load_from_address(generator, size);
  return code_generator_store_ir_destination(generator, &instruction->dest,
                                             temp_table);
}

static int code_generator_emit_ir_store(CodeGenerator *generator,
                                        const IRInstruction *instruction,
                                        IRTempTable *temp_table) {
  if (!generator || !instruction) {
    return 0;
  }

  int size = code_generator_get_ir_access_size(generator, &instruction->rhs);
  if (size <= 0) {
    return 0;
  }

  if (!code_generator_load_ir_operand(generator, &instruction->dest,
                                      temp_table)) {
    return 0;
  }
  code_generator_emit(generator, "    push rax\n");

  if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                      temp_table)) {
    return 0;
  }
  code_generator_emit(generator, "    mov rcx, rax\n");
  code_generator_emit(generator, "    pop rax\n");
  code_generator_emit_ir_store_to_address(generator, size);
  return 1;
}

static int code_generator_emit_ir_new(CodeGenerator *generator,
                                      const IRInstruction *instruction,
                                      IRTempTable *temp_table) {
  if (!generator || !instruction) {
    return 0;
  }

  int allocation_size = 8;
  if (instruction->rhs.kind == IR_OPERAND_INT &&
      instruction->rhs.int_value > 0) {
    allocation_size = (int)instruction->rhs.int_value;
  }

  const char *size_register = "rdi";
  CallingConventionSpec *conv_spec =
      generator->register_allocator
          ? generator->register_allocator->calling_convention
          : NULL;
  if (conv_spec && conv_spec->int_param_count > 0) {
    const char *candidate =
        code_generator_get_register_name(conv_spec->int_param_registers[0]);
    if (candidate) {
      size_register = candidate;
    }
  }

  code_generator_emit(generator, "    ; IR new: %s (%d bytes)\n",
                      instruction->text ? instruction->text : "type",
                      allocation_size);

  if (instruction->rhs.kind == IR_OPERAND_TEMP) {
    int offset = ir_temp_table_get_offset(temp_table, instruction->rhs.name);
    code_generator_emit(generator, "    mov %s, [rbp - %d]\n", size_register,
                        offset);
  } else {
    code_generator_emit(generator, "    mov %s, %d\n", size_register,
                        allocation_size);
  }
  code_generator_emit(generator, "    extern gc_alloc\n");
  code_generator_emit(generator, "    call gc_alloc\n");
  return code_generator_store_ir_destination(generator, &instruction->dest,
                                             temp_table);
}

static int
code_generator_emit_ir_binary_fallback(CodeGenerator *generator,
                                       const IRInstruction *instruction,
                                       IRTempTable *temp_table) {
  if (!generator || !instruction || !instruction->text) {
    return 0;
  }

  if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                      temp_table)) {
    return 0;
  }
  code_generator_emit(generator, "    push rax\n");

  if (!code_generator_load_ir_operand(generator, &instruction->rhs,
                                      temp_table)) {
    return 0;
  }
  code_generator_emit(generator, "    mov r10, rax\n");
  code_generator_emit(generator, "    pop rax\n");

  const char *op = instruction->text;
  if (instruction->is_float) {
    code_generator_emit(generator, "    movq xmm0, rax\n");
    code_generator_emit(generator, "    movq xmm1, r10\n");

    if (strcmp(op, "+") == 0) {
      code_generator_emit(generator, "    addsd xmm0, xmm1\n");
      code_generator_emit(generator, "    movq rax, xmm0\n");
    } else if (strcmp(op, "-") == 0) {
      code_generator_emit(generator, "    subsd xmm0, xmm1\n");
      code_generator_emit(generator, "    movq rax, xmm0\n");
    } else if (strcmp(op, "*") == 0) {
      code_generator_emit(generator, "    mulsd xmm0, xmm1\n");
      code_generator_emit(generator, "    movq rax, xmm0\n");
    } else if (strcmp(op, "/") == 0) {
      code_generator_emit(generator, "    divsd xmm0, xmm1\n");
      code_generator_emit(generator, "    movq rax, xmm0\n");
    } else if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
               strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
               strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
      code_generator_emit(generator, "    ucomisd xmm0, xmm1\n");
      if (strcmp(op, "==") == 0) {
        code_generator_emit(generator, "    sete al\n");
        code_generator_emit(generator, "    setnp cl\n");
        code_generator_emit(generator, "    and al, cl\n");
      } else if (strcmp(op, "!=") == 0) {
        code_generator_emit(generator, "    setne al\n");
        code_generator_emit(generator, "    setp cl\n");
        code_generator_emit(generator, "    or al, cl\n");
      } else if (strcmp(op, "<") == 0) {
        code_generator_emit(generator, "    setb al\n");
      } else if (strcmp(op, "<=") == 0) {
        code_generator_emit(generator, "    setbe al\n");
      } else if (strcmp(op, ">") == 0) {
        code_generator_emit(generator, "    seta al\n");
      } else if (strcmp(op, ">=") == 0) {
        code_generator_emit(generator, "    setae al\n");
      }
      code_generator_emit(generator, "    movzx rax, al\n");
    } else {
      code_generator_set_error(generator,
                               "Unsupported float binary operator '%s'", op);
      return 0;
    }
    return code_generator_store_ir_destination(generator, &instruction->dest,
                                               temp_table);
  }

  const char *arith = code_generator_get_arithmetic_instruction(op, 0);
  if (arith) {
    if (strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {
      code_generator_emit(generator, "    cqo\n");
      code_generator_emit(generator, "    idiv r10\n");
      if (strcmp(op, "%") == 0) {
        code_generator_emit(generator, "    mov rax, rdx\n");
      }
    } else {
      code_generator_emit(generator, "    %s rax, r10\n", arith);
    }
    return code_generator_store_ir_destination(generator, &instruction->dest,
                                               temp_table);
  }

  code_generator_emit(generator, "    cmp rax, r10\n");
  if (strcmp(op, "==") == 0) {
    code_generator_emit(generator, "    sete al\n");
  } else if (strcmp(op, "!=") == 0) {
    code_generator_emit(generator, "    setne al\n");
  } else if (strcmp(op, "<") == 0) {
    code_generator_emit(generator, "    setl al\n");
  } else if (strcmp(op, "<=") == 0) {
    code_generator_emit(generator, "    setle al\n");
  } else if (strcmp(op, ">") == 0) {
    code_generator_emit(generator, "    setg al\n");
  } else if (strcmp(op, ">=") == 0) {
    code_generator_emit(generator, "    setge al\n");
  } else if (strcmp(op, "&&") == 0) {
    code_generator_emit(generator, "    and rax, r10\n");
    code_generator_emit(generator, "    setne al\n");
  } else if (strcmp(op, "||") == 0) {
    code_generator_emit(generator, "    or rax, r10\n");
    code_generator_emit(generator, "    setne al\n");
  } else if (strcmp(op, "<<") == 0) {
    code_generator_emit(generator, "    mov rcx, r10\n");
    code_generator_emit(generator, "    shl rax, cl\n");
    return code_generator_store_ir_destination(generator, &instruction->dest,
                                               temp_table);
  } else if (strcmp(op, ">>") == 0) {
    code_generator_emit(generator, "    mov rcx, r10\n");
    code_generator_emit(generator, "    sar rax, cl\n");
    return code_generator_store_ir_destination(generator, &instruction->dest,
                                               temp_table);
  } else {
    code_generator_set_error(generator, "Unsupported IR binary operator '%s'",
                             op);
    return 0;
  }
  code_generator_emit(generator, "    movzx rax, al\n");
  return code_generator_store_ir_destination(generator, &instruction->dest,
                                             temp_table);
}

static int
code_generator_emit_ir_unary_fallback(CodeGenerator *generator,
                                      const IRInstruction *instruction,
                                      IRTempTable *temp_table) {
  if (!generator || !instruction || !instruction->text) {
    return 0;
  }

  const char *op = instruction->text;
  if (strcmp(op, "&") == 0) {
    if (instruction->lhs.kind == IR_OPERAND_SYMBOL && instruction->lhs.name) {
      Symbol *symbol =
          symbol_table_lookup(generator->symbol_table, instruction->lhs.name);
      if (!symbol) {
        code_generator_set_error(generator,
                                 "Unknown symbol '%s' in IR unary '&'",
                                 instruction->lhs.name);
        return 0;
      }
      if (symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
        const char *symbol_name = code_generator_get_link_symbol_name(
            generator, instruction->lhs.name);
        if (!symbol_name) {
          code_generator_set_error(generator,
                                   "Invalid global symbol in IR unary '&'");
          return 0;
        }
        if (symbol->is_extern &&
            !code_generator_emit_extern_symbol(generator, symbol_name)) {
          return 0;
        }
        code_generator_emit(generator, "    lea rax, [rel %s]\n", symbol_name);
      } else {
        code_generator_emit(generator, "    lea rax, [rbp - %d]\n",
                            symbol->data.variable.memory_offset);
      }
    } else {
      code_generator_set_error(generator,
                               "IR unary '&' requires symbol operand");
      return 0;
    }
    return code_generator_store_ir_destination(generator, &instruction->dest,
                                               temp_table);
  }

  if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                      temp_table)) {
    return 0;
  }

  if (instruction->is_float) {
    if (strcmp(op, "-") == 0) {
      code_generator_emit(generator, "    movq xmm0, rax\n");
      code_generator_emit(generator, "    pxor xmm1, xmm1\n");
      code_generator_emit(generator, "    subsd xmm1, xmm0\n");
      code_generator_emit(generator, "    movq rax, xmm1\n");
    } else if (strcmp(op, "+") == 0) {
      // No-op for float unary plus
    } else {
      code_generator_set_error(generator,
                               "Unsupported float unary operator '%s'", op);
      return 0;
    }
    return code_generator_store_ir_destination(generator, &instruction->dest,
                                               temp_table);
  }

  if (strcmp(op, "-") == 0) {
    code_generator_emit(generator, "    neg rax\n");
  } else if (strcmp(op, "!") == 0) {
    code_generator_emit(generator, "    test rax, rax\n");
    code_generator_emit(generator, "    setz al\n");
    code_generator_emit(generator, "    movzx rax, al\n");
  } else if (strcmp(op, "~") == 0) {
    code_generator_emit(generator, "    not rax\n");
  } else if (strcmp(op, "+") == 0) {
    // No-op
  } else if (strcmp(op, "*") == 0) {
    code_generator_emit(generator, "    mov rax, [rax]\n");
  } else {
    code_generator_set_error(generator, "Unsupported IR unary operator '%s'",
                             op);
    return 0;
  }

  return code_generator_store_ir_destination(generator, &instruction->dest,
                                             temp_table);
}

static int code_generator_ir_operand_is_float(const IROperand *operand) {
  return operand && operand->kind == IR_OPERAND_FLOAT;
}

static int code_generator_emit_ir_call_argument_stack(CodeGenerator *generator,
                                                      const IROperand *operand,
                                                      IRTempTable *temp_table,
                                                      int stack_slot_offset) {
  if (!code_generator_load_ir_operand(generator, operand, temp_table)) {
    return 0;
  }
  code_generator_emit(generator, "    mov [rsp + %d], rax\n", stack_slot_offset);
  return 1;
}

static int code_generator_emit_ir_call_argument_register(
    CodeGenerator *generator, const IROperand *operand, IRTempTable *temp_table,
    x86Register target_register, int is_float) {
  if (!code_generator_load_ir_operand(generator, operand, temp_table)) {
    return 0;
  }

  const char *register_name = code_generator_get_register_name(target_register);
  if (!register_name) {
    code_generator_set_error(generator,
                             "Invalid parameter register in IR call");
    return 0;
  }

  if (is_float) {
    code_generator_emit(generator, "    movq %s, rax\n", register_name);
  } else {
    code_generator_emit(generator, "    mov %s, rax\n", register_name);
  }
  return 1;
}

static int code_generator_emit_ir_call(CodeGenerator *generator,
                                       const IRInstruction *instruction,
                                       IRTempTable *temp_table) {
  if (!generator || !instruction || !instruction->text) {
    return 0;
  }

  CallingConventionSpec *conv_spec =
      generator->register_allocator->calling_convention;
  if (!conv_spec) {
    code_generator_set_error(generator, "No calling convention configured");
    return 0;
  }
  Symbol *function_symbol =
      symbol_table_lookup(generator->symbol_table, instruction->text);
  const char *call_target =
      code_generator_get_link_symbol_name(generator, instruction->text);
  if (!call_target) {
    code_generator_set_error(generator, "Invalid IR call target");
    return 0;
  }
  if (function_symbol && function_symbol->is_extern) {
    if (!code_generator_emit_extern_symbol(generator, call_target)) {
      return 0;
    }
  }

  size_t argument_count = instruction->argument_count;
  int *is_float = NULL;
  int *goes_on_stack = NULL;
  if (argument_count > 0) {
    is_float = calloc(argument_count, sizeof(int));
    goes_on_stack = calloc(argument_count, sizeof(int));
    if (!is_float || !goes_on_stack) {
      free(is_float);
      free(goes_on_stack);
      code_generator_set_error(
          generator, "Out of memory while planning IR call arguments");
      return 0;
    }
  }

  int int_reg_cursor = 0;
  int float_reg_cursor = 0;
  int stack_argument_count = 0;
  for (size_t i = 0; i < argument_count; i++) {
    is_float[i] =
        code_generator_ir_operand_is_float(&instruction->arguments[i]);
    if (is_float[i]) {
      if (float_reg_cursor < (int)conv_spec->float_param_count) {
        float_reg_cursor++;
      } else {
        goes_on_stack[i] = 1;
        stack_argument_count++;
      }
    } else {
      if (int_reg_cursor < (int)conv_spec->int_param_count) {
        int_reg_cursor++;
      } else {
        goes_on_stack[i] = 1;
        stack_argument_count++;
      }
    }
  }

  code_generator_emit(generator, "    ; IR call: %s (%zu args)\n",
                      instruction->text, argument_count);

  int shadow_space =
      (conv_spec->convention == CALLING_CONV_MS_X64)
          ? conv_spec->shadow_space_size
          : 0;
  int stack_arg_space = stack_argument_count * 8;
  int call_stack_total = shadow_space + stack_arg_space;
  if ((call_stack_total % 16) != 0) {
    call_stack_total += 8;
  }
  if (call_stack_total > 0) {
    code_generator_emit(generator, "    sub rsp, %d\n", call_stack_total);
  }

  // Materialize stack arguments into ABI-defined stack slots.
  int stack_arg_index = 0;
  for (size_t i = 0; i < argument_count; i++) {
    if (!goes_on_stack || !goes_on_stack[i]) {
      continue;
    }
    int slot_offset = shadow_space + (stack_arg_index * 8);
    if (!code_generator_emit_ir_call_argument_stack(
            generator, &instruction->arguments[i], temp_table, slot_offset)) {
      free(is_float);
      free(goes_on_stack);
      return 0;
    }
    stack_arg_index++;
  }

  // Fill register arguments left-to-right.
  int_reg_cursor = 0;
  float_reg_cursor = 0;
  for (size_t i = 0; i < argument_count; i++) {
    if (goes_on_stack && goes_on_stack[i]) {
      continue;
    }

    if (is_float && is_float[i]) {
      if (float_reg_cursor >= (int)conv_spec->float_param_count) {
        code_generator_set_error(
            generator, "IR call argument classification mismatch (float)");
        free(is_float);
        free(goes_on_stack);
        return 0;
      }
      x86Register target_register =
          conv_spec->float_param_registers[float_reg_cursor++];
      if (!code_generator_emit_ir_call_argument_register(
              generator, &instruction->arguments[i], temp_table,
              target_register, 1)) {
        free(is_float);
        free(goes_on_stack);
        return 0;
      }
    } else {
      if (int_reg_cursor >= (int)conv_spec->int_param_count) {
        code_generator_set_error(
            generator, "IR call argument classification mismatch (integer)");
        free(is_float);
        free(goes_on_stack);
        return 0;
      }
      x86Register target_register =
          conv_spec->int_param_registers[int_reg_cursor++];
      if (!code_generator_emit_ir_call_argument_register(
              generator, &instruction->arguments[i], temp_table,
              target_register, 0)) {
        free(is_float);
        free(goes_on_stack);
        return 0;
      }
    }
  }

  code_generator_emit(generator, "    call %s\n", call_target);
  code_generator_emit(generator, "    mov rcx, rax\n");

  if (call_stack_total > 0) {
    code_generator_emit(generator, "    add rsp, %d\n", call_stack_total);
  }
  code_generator_emit(generator, "    mov rax, rcx\n");

  Type *return_type = NULL;
  if (function_symbol && function_symbol->kind == SYMBOL_FUNCTION &&
      function_symbol->type) {
    return_type = function_symbol->type;
  }
  code_generator_handle_return_value(generator, return_type);

  free(is_float);
  free(goes_on_stack);
  return !generator->has_error;
}

static int code_generator_emit_ir_instruction(CodeGenerator *generator,
                                              const IRInstruction *instruction,
                                              IRTempTable *temp_table) {
  if (!generator || !instruction) {
    return 0;
  }

  switch (instruction->op) {
  case IR_OP_NOP:
  case IR_OP_DECLARE_LOCAL:
    return 1;

  case IR_OP_LABEL:
    code_generator_emit(generator, "%s:\n",
                        instruction->text ? instruction->text : "ir_label");
    return 1;

  case IR_OP_JUMP:
    code_generator_emit(generator, "    jmp %s\n",
                        instruction->text ? instruction->text : "ir_missing");
    return 1;

  case IR_OP_BRANCH_ZERO:
    if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                        temp_table)) {
      return 0;
    }
    code_generator_emit(generator, "    test rax, rax\n");
    code_generator_emit(generator, "    jz %s\n",
                        instruction->text ? instruction->text : "ir_missing");
    return 1;

  case IR_OP_BRANCH_EQ:
    if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                        temp_table)) {
      return 0;
    }
    code_generator_emit(generator, "    push rax\n");
    if (!code_generator_load_ir_operand(generator, &instruction->rhs,
                                        temp_table)) {
      return 0;
    }
    code_generator_emit(generator, "    mov r10, rax\n");
    code_generator_emit(generator, "    pop rax\n");
    code_generator_emit(generator, "    cmp rax, r10\n");
    code_generator_emit(generator, "    je %s\n",
                        instruction->text ? instruction->text : "ir_missing");
    return 1;

  case IR_OP_ASSIGN:
    if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                        temp_table)) {
      return 0;
    }
    return code_generator_store_ir_destination(generator, &instruction->dest,
                                               temp_table);

  case IR_OP_ADDRESS_OF:
    return code_generator_emit_ir_address_of(generator, instruction,
                                             temp_table);

  case IR_OP_LOAD:
    return code_generator_emit_ir_load(generator, instruction, temp_table);

  case IR_OP_STORE:
    return code_generator_emit_ir_store(generator, instruction, temp_table);

  case IR_OP_BINARY:
    if (instruction->ast_ref) {
      code_generator_generate_expression(generator, instruction->ast_ref);
      if (generator->has_error) {
        return 0;
      }
      return code_generator_store_ir_destination(generator, &instruction->dest,
                                                 temp_table);
    }
    return code_generator_emit_ir_binary_fallback(generator, instruction,
                                                  temp_table);

  case IR_OP_UNARY:
    if (instruction->ast_ref) {
      code_generator_generate_expression(generator, instruction->ast_ref);
      if (generator->has_error) {
        return 0;
      }
      return code_generator_store_ir_destination(generator, &instruction->dest,
                                                 temp_table);
    }
    return code_generator_emit_ir_unary_fallback(generator, instruction,
                                                 temp_table);

  case IR_OP_CALL:
    if (!code_generator_emit_ir_call(generator, instruction, temp_table)) {
      return 0;
    }
    return code_generator_store_ir_destination(generator, &instruction->dest,
                                               temp_table);

  case IR_OP_NEW:
    return code_generator_emit_ir_new(generator, instruction, temp_table);

  case IR_OP_RETURN:
    if (instruction->lhs.kind != IR_OPERAND_NONE) {
      if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                          temp_table)) {
        return 0;
      }
    }
    code_generator_emit(generator, "    jmp L%s_exit\n",
                        generator->current_function_name
                            ? generator->current_function_name
                            : "function");
    return 1;

  case IR_OP_INLINE_ASM:
    code_generator_emit(generator, "    ; Begin inline assembly block\n");
    code_generator_preserve_registers_for_inline_asm(generator);
    code_generator_emit(generator, "%s\n",
                        instruction->text ? instruction->text : "");
    code_generator_restore_registers_after_inline_asm(generator);
    code_generator_emit(generator, "    ; End inline assembly block\n");
    return 1;
  default:
    code_generator_set_error(generator, "Unhandled IR opcode: %d",
                             instruction->op);
    return 0;
  }
}

int code_generator_generate_function_from_ir(CodeGenerator *generator,
                                             ASTNode *function_declaration,
                                             IRFunction *ir_function) {
  if (!generator || !function_declaration || !ir_function ||
      function_declaration->type != AST_FUNCTION_DECLARATION) {
    return 0;
  }

  FunctionDeclaration *function_data =
      (FunctionDeclaration *)function_declaration->data;
  if (!function_data || !function_data->name) {
    code_generator_set_error(generator, "Malformed function declaration");
    return 0;
  }

  symbol_table_enter_scope(generator->symbol_table, SCOPE_FUNCTION);

  if (generator->generate_debug_info) {
    code_generator_add_debug_symbol(
        generator, function_data->name, DEBUG_SYMBOL_FUNCTION,
        function_data->return_type, function_declaration->location.line,
        function_declaration->location.column);
    code_generator_add_line_mapping(generator,
                                    function_declaration->location.line,
                                    function_declaration->location.column,
                                    generator->debug_info->source_filename);
  }

  code_generator_emit(generator, "\nglobal %s\n", function_data->name);

  int parameter_home_size = 0;
  if (function_data->parameter_count > 0) {
    if (function_data->parameter_count > (size_t)(INT_MAX / 8)) {
      code_generator_set_error(generator,
                               "Too many parameters in function '%s'",
                               function_data->name);
      symbol_table_exit_scope(generator->symbol_table);
      return 0;
    }
    parameter_home_size = (int)(function_data->parameter_count * 8);
  }

  int stack_size = parameter_home_size;
  IRTempTable temp_table = {0};
  IRLocalTable local_table = {0};

  for (size_t i = 0; i < ir_function->instruction_count; i++) {
    IRInstruction *instruction = &ir_function->instructions[i];
    if (!instruction) {
      continue;
    }

    if (instruction->op == IR_OP_DECLARE_LOCAL &&
        instruction->dest.kind == IR_OPERAND_SYMBOL &&
        instruction->dest.name != NULL) {
      const char *type_name =
          (instruction->text && instruction->text[0] != '\0')
              ? instruction->text
              : "int64";
      Type *local_type =
          type_checker_get_type_by_name(generator->type_checker, type_name);
      if (!local_type) {
        local_type =
            type_checker_get_type_by_name(generator->type_checker, "int64");
      }

      int local_size = 0;
      if (local_type && local_type->size > 0) {
        local_size = (int)local_type->size;
      } else {
        local_size = code_generator_calculate_variable_size(
            generator, instruction->text ? instruction->text : "int64");
      }
      if (local_size <= 0) {
        local_size = 8;
      }
      int alignment = (local_size > 4) ? 8 : local_size;
      if (alignment < 1) {
        alignment = 1;
      }

      Symbol *existing = symbol_table_lookup_current_scope(
          generator->symbol_table, instruction->dest.name);
      if (!existing) {
        Symbol *local_symbol =
            symbol_create(instruction->dest.name, SYMBOL_VARIABLE, local_type);
        if (!local_symbol ||
            !symbol_table_declare(generator->symbol_table, local_symbol)) {
          symbol_destroy(local_symbol);
          code_generator_set_error(generator,
                                   "Failed to declare local '%s' in IR backend",
                                   instruction->dest.name);
          ir_temp_table_destroy(&temp_table);
          ir_local_table_destroy(&local_table);
          symbol_table_exit_scope(generator->symbol_table);
          return 0;
        }
      }

      if (!ir_local_table_add(&local_table, instruction->dest.name,
                              instruction->text, instruction->location,
                              local_size, alignment)) {
        code_generator_set_error(generator,
                                 "Out of memory while tracking IR local '%s'",
                                 instruction->dest.name);
        ir_temp_table_destroy(&temp_table);
        ir_local_table_destroy(&local_table);
        symbol_table_exit_scope(generator->symbol_table);
        return 0;
      }
      stack_size += local_size + 15;
    }

    if (instruction->dest.kind == IR_OPERAND_TEMP && instruction->dest.name) {
      if (!ir_temp_table_add(&temp_table, instruction->dest.name)) {
        code_generator_set_error(generator,
                                 "Out of memory while tracking IR temp '%s'",
                                 instruction->dest.name);
        ir_temp_table_destroy(&temp_table);
        ir_local_table_destroy(&local_table);
        symbol_table_exit_scope(generator->symbol_table);
        return 0;
      }
    }
  }
  stack_size += (int)(temp_table.count * (8 + 15));

  code_generator_function_prologue(generator, function_data->name, stack_size);
  generator->current_stack_offset = parameter_home_size;
  code_generator_register_function_parameters(generator, function_data,
                                              parameter_home_size);
  if (generator->has_error) {
    ir_temp_table_destroy(&temp_table);
    ir_local_table_destroy(&local_table);
    symbol_table_exit_scope(generator->symbol_table);
    return 0;
  }

  for (size_t i = 0; i < local_table.count; i++) {
    IRLocalSlot *local = &local_table.items[i];
    Symbol *symbol =
        symbol_table_lookup_current_scope(generator->symbol_table, local->name);
    if (!symbol || symbol->kind != SYMBOL_VARIABLE) {
      code_generator_set_error(
          generator, "Missing local symbol '%s' in IR backend", local->name);
      ir_temp_table_destroy(&temp_table);
      ir_local_table_destroy(&local_table);
      symbol_table_exit_scope(generator->symbol_table);
      return 0;
    }
    int offset = code_generator_allocate_stack_space(generator, local->size,
                                                     local->alignment);
    symbol->data.variable.is_in_register = 0;
    symbol->data.variable.memory_offset = offset;

    if (generator->generate_debug_info) {
      code_generator_add_debug_symbol(
          generator, local->name, DEBUG_SYMBOL_VARIABLE,
          local->type_name ? local->type_name : "unknown", local->location.line,
          local->location.column);
      debug_info_set_symbol_stack_offset(generator->debug_info, local->name,
                                         -offset);
    }
  }

  for (size_t i = 0; i < temp_table.count; i++) {
    temp_table.items[i].offset =
        code_generator_allocate_stack_space(generator, 8, 8);
  }

  for (size_t i = 0; i < ir_function->instruction_count; i++) {
    if (!code_generator_emit_ir_instruction(
            generator, &ir_function->instructions[i], &temp_table)) {
      break;
    }
  }

  code_generator_emit(generator, "L%s_exit:\n", function_data->name);
  code_generator_function_epilogue(generator);

  ir_temp_table_destroy(&temp_table);
  ir_local_table_destroy(&local_table);
  symbol_table_exit_scope(generator->symbol_table);

  return generator->has_error ? 0 : 1;
}
