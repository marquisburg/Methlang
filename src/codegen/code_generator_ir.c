#include "code_generator_internal.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define STACK_FRAME_WARN_THRESHOLD (256 * 1024)

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

typedef struct {
  char *name;
  size_t use_count;
  int address_taken;
} IRSymbolStatsEntry;

typedef struct {
  IRSymbolStatsEntry *items;
  size_t count;
  size_t capacity;
} IRSymbolStatsMap;

typedef struct {
  char *name;
  size_t use_count;
} IRTempUseEntry;

typedef struct {
  IRTempUseEntry *items;
  size_t count;
  size_t capacity;
} IRTempUseMap;

typedef struct {
  const char *name;
  x86Register reg;
  int save_offset;
} IRPromotedSymbol;

static const x86Register IR_PROMOTION_REGISTERS[] = {
    REG_R12, REG_R13, REG_R14, REG_R15, REG_RBX};

static char *ir_codegen_strdup(const char *text) {
  if (!text) {
    return NULL;
  }
  size_t length = strlen(text) + 1;
  char *copy = malloc(length);
  if (!copy) {
    return NULL;
  }
  memcpy(copy, text, length);
  return copy;
}

static int ir_symbol_stats_map_find(const IRSymbolStatsMap *map,
                                    const char *name) {
  if (!map || !name) {
    return -1;
  }
  for (size_t i = 0; i < map->count; i++) {
    if (map->items[i].name && strcmp(map->items[i].name, name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static IRSymbolStatsEntry *ir_symbol_stats_map_get_or_add(IRSymbolStatsMap *map,
                                                          const char *name) {
  if (!map || !name) {
    return NULL;
  }

  int existing = ir_symbol_stats_map_find(map, name);
  if (existing >= 0) {
    return &map->items[existing];
  }

  if (map->count >= map->capacity) {
    size_t new_capacity = (map->capacity == 0) ? 16 : map->capacity * 2;
    IRSymbolStatsEntry *new_items =
        realloc(map->items, new_capacity * sizeof(IRSymbolStatsEntry));
    if (!new_items) {
      return NULL;
    }
    map->items = new_items;
    map->capacity = new_capacity;
  }

  char *name_copy = ir_codegen_strdup(name);
  if (!name_copy) {
    return NULL;
  }

  IRSymbolStatsEntry *entry = &map->items[map->count++];
  entry->name = name_copy;
  entry->use_count = 0;
  entry->address_taken = 0;
  return entry;
}

static int ir_symbol_stats_map_add_use(IRSymbolStatsMap *map,
                                       const char *name) {
  IRSymbolStatsEntry *entry = ir_symbol_stats_map_get_or_add(map, name);
  if (!entry) {
    return 0;
  }
  entry->use_count++;
  return 1;
}

static int ir_symbol_stats_map_mark_address_taken(IRSymbolStatsMap *map,
                                                  const char *name) {
  IRSymbolStatsEntry *entry = ir_symbol_stats_map_get_or_add(map, name);
  if (!entry) {
    return 0;
  }
  entry->address_taken = 1;
  return 1;
}

static size_t ir_symbol_stats_map_get_use_count(const IRSymbolStatsMap *map,
                                                const char *name) {
  int index = ir_symbol_stats_map_find(map, name);
  if (index < 0) {
    return 0;
  }
  return map->items[index].use_count;
}

static int ir_symbol_stats_map_is_address_taken(const IRSymbolStatsMap *map,
                                                const char *name) {
  int index = ir_symbol_stats_map_find(map, name);
  if (index < 0) {
    return 0;
  }
  return map->items[index].address_taken;
}

static void ir_symbol_stats_map_destroy(IRSymbolStatsMap *map) {
  if (!map) {
    return;
  }
  for (size_t i = 0; i < map->count; i++) {
    free(map->items[i].name);
  }
  free(map->items);
  map->items = NULL;
  map->count = 0;
  map->capacity = 0;
}

static int ir_symbol_stats_map_record_operand(IRSymbolStatsMap *map,
                                              const IROperand *operand) {
  if (!map || !operand || operand->kind != IR_OPERAND_SYMBOL ||
      !operand->name) {
    return 1;
  }
  return ir_symbol_stats_map_add_use(map, operand->name);
}

static int ir_symbol_stats_map_record_instruction(IRSymbolStatsMap *map,
                                                  const IRInstruction *instr) {
  if (!map || !instr) {
    return 0;
  }

  if (!ir_symbol_stats_map_record_operand(map, &instr->dest) ||
      !ir_symbol_stats_map_record_operand(map, &instr->lhs) ||
      !ir_symbol_stats_map_record_operand(map, &instr->rhs)) {
    return 0;
  }

  for (size_t i = 0; i < instr->argument_count; i++) {
    if (!ir_symbol_stats_map_record_operand(map, &instr->arguments[i])) {
      return 0;
    }
  }

  if (instr->op == IR_OP_ADDRESS_OF && instr->lhs.kind == IR_OPERAND_SYMBOL &&
      instr->lhs.name) {
    if (!ir_symbol_stats_map_mark_address_taken(map, instr->lhs.name)) {
      return 0;
    }
  }

  return 1;
}

static int ir_temp_use_map_find(const IRTempUseMap *map, const char *name) {
  if (!map || !name) {
    return -1;
  }
  for (size_t i = 0; i < map->count; i++) {
    if (map->items[i].name && strcmp(map->items[i].name, name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static int ir_temp_use_map_add(IRTempUseMap *map, const char *name) {
  if (!map || !name) {
    return 0;
  }

  int existing = ir_temp_use_map_find(map, name);
  if (existing >= 0) {
    map->items[existing].use_count++;
    return 1;
  }

  if (map->count >= map->capacity) {
    size_t new_capacity = (map->capacity == 0) ? 16 : map->capacity * 2;
    IRTempUseEntry *new_items =
        realloc(map->items, new_capacity * sizeof(IRTempUseEntry));
    if (!new_items) {
      return 0;
    }
    map->items = new_items;
    map->capacity = new_capacity;
  }

  char *name_copy = ir_codegen_strdup(name);
  if (!name_copy) {
    return 0;
  }

  map->items[map->count].name = name_copy;
  map->items[map->count].use_count = 1;
  map->count++;
  return 1;
}

static size_t ir_temp_use_map_get(const IRTempUseMap *map, const char *name) {
  int index = ir_temp_use_map_find(map, name);
  if (index < 0) {
    return 0;
  }
  return map->items[index].use_count;
}

static void ir_temp_use_map_destroy(IRTempUseMap *map) {
  if (!map) {
    return;
  }
  for (size_t i = 0; i < map->count; i++) {
    free(map->items[i].name);
  }
  free(map->items);
  map->items = NULL;
  map->count = 0;
  map->capacity = 0;
}

static int ir_temp_use_map_record_operand(IRTempUseMap *map,
                                          const IROperand *operand) {
  if (!map || !operand || operand->kind != IR_OPERAND_TEMP || !operand->name) {
    return 1;
  }
  return ir_temp_use_map_add(map, operand->name);
}

static int ir_temp_use_map_record_instruction(IRTempUseMap *map,
                                              const IRInstruction *instr) {
  if (!map || !instr) {
    return 0;
  }

  if (instr->op == IR_OP_STORE) {
    if (!ir_temp_use_map_record_operand(map, &instr->dest) ||
        !ir_temp_use_map_record_operand(map, &instr->lhs) ||
        !ir_temp_use_map_record_operand(map, &instr->rhs)) {
      return 0;
    }
  } else {
    if (!ir_temp_use_map_record_operand(map, &instr->lhs) ||
        !ir_temp_use_map_record_operand(map, &instr->rhs)) {
      return 0;
    }
  }

  for (size_t i = 0; i < instr->argument_count; i++) {
    if (!ir_temp_use_map_record_operand(map, &instr->arguments[i])) {
      return 0;
    }
  }

  return 1;
}

static int ir_is_integer_or_pointer_promotable(Type *type) {
  if (!type || type->size == 0 || type->size > 8) {
    return 0;
  }

  switch (type->kind) {
  case TYPE_INT8:
  case TYPE_INT16:
  case TYPE_INT32:
  case TYPE_INT64:
  case TYPE_UINT8:
  case TYPE_UINT16:
  case TYPE_UINT32:
  case TYPE_UINT64:
  case TYPE_POINTER:
  case TYPE_FUNCTION_POINTER:
  case TYPE_ENUM:
    return 1;
  default:
    return 0;
  }
}

static int ir_promoted_symbol_find(IRPromotedSymbol *symbols, size_t count,
                                   const char *name) {
  if (!symbols || !name) {
    return -1;
  }
  for (size_t i = 0; i < count; i++) {
    if (symbols[i].name && strcmp(symbols[i].name, name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static int ir_immediate_fits_signed_32(long long value) {
  return value >= (long long)INT_MIN && value <= (long long)INT_MAX;
}

static Symbol *code_generator_ir_lookup_register_symbol(CodeGenerator *generator,
                                                        const IROperand *operand) {
  if (!generator || !operand || operand->kind != IR_OPERAND_SYMBOL ||
      !operand->name) {
    return NULL;
  }

  Symbol *symbol = symbol_table_lookup(generator->symbol_table, operand->name);
  if (!symbol ||
      (symbol->kind != SYMBOL_VARIABLE && symbol->kind != SYMBOL_PARAMETER) ||
      !symbol->data.variable.is_in_register) {
    return NULL;
  }

  return symbol;
}

static int code_generator_ir_symbol_width_bits(const Symbol *symbol) {
  int size = 8;
  if (symbol && symbol->type && symbol->type->size > 0 && symbol->type->size <= 8) {
    size = (int)symbol->type->size;
  }

  if (size <= 1) {
    return 8;
  }
  if (size == 2) {
    return 16;
  }
  if (size == 4) {
    return 32;
  }
  return 64;
}

static const char *code_generator_ir_symbol_register_name(const Symbol *symbol,
                                                          int width_bits) {
  if (!symbol) {
    return NULL;
  }

  x86Register reg = (x86Register)symbol->data.variable.register_id;
  if (width_bits == 64) {
    return code_generator_get_register_name(reg);
  }
  return code_generator_get_subregister_name(reg, width_bits);
}

static const char *code_generator_ir_binary_register_instruction(const char *op) {
  if (!op) {
    return NULL;
  }
  if (strcmp(op, "+") == 0) {
    return "add";
  }
  if (strcmp(op, "-") == 0) {
    return "sub";
  }
  if (strcmp(op, "&") == 0) {
    return "and";
  }
  if (strcmp(op, "|") == 0) {
    return "or";
  }
  if (strcmp(op, "^") == 0) {
    return "xor";
  }
  return NULL;
}

static int code_generator_try_emit_ir_assign_symbol_to_symbol(
    CodeGenerator *generator, const IRInstruction *instruction, int *emitted) {
  if (emitted) {
    *emitted = 0;
  }
  if (!generator || !instruction) {
    return 0;
  }

  Symbol *dest_symbol =
      code_generator_ir_lookup_register_symbol(generator, &instruction->dest);
  Symbol *lhs_symbol =
      code_generator_ir_lookup_register_symbol(generator, &instruction->lhs);
  if (!dest_symbol || !lhs_symbol) {
    return 1;
  }

  int dest_bits = code_generator_ir_symbol_width_bits(dest_symbol);
  int lhs_bits = code_generator_ir_symbol_width_bits(lhs_symbol);
  if (dest_bits != lhs_bits) {
    return 1;
  }

  const char *dest_reg =
      code_generator_ir_symbol_register_name(dest_symbol, dest_bits);
  const char *lhs_reg =
      code_generator_ir_symbol_register_name(lhs_symbol, lhs_bits);
  if (!dest_reg || !lhs_reg) {
    code_generator_set_error(generator,
                             "Invalid register in IR symbol assign fast path");
    return 0;
  }

  if (strcmp(dest_reg, lhs_reg) != 0) {
    code_generator_emit(generator, "    mov %s, %s\n", dest_reg, lhs_reg);
  }

  if (emitted) {
    *emitted = 1;
  }
  return 1;
}

static int code_generator_try_emit_ir_binary_register_fastpath(
    CodeGenerator *generator, const IRInstruction *instruction, int *emitted) {
  if (emitted) {
    *emitted = 0;
  }
  if (!generator || !instruction || !instruction->text) {
    return 0;
  }
  if (instruction->is_float) {
    return 1;
  }

  const char *arith_instruction =
      code_generator_ir_binary_register_instruction(instruction->text);
  if (!arith_instruction) {
    return 1;
  }

  Symbol *dest_symbol =
      code_generator_ir_lookup_register_symbol(generator, &instruction->dest);
  Symbol *lhs_symbol =
      code_generator_ir_lookup_register_symbol(generator, &instruction->lhs);
  if (!dest_symbol || !lhs_symbol) {
    return 1;
  }

  int bits = code_generator_ir_symbol_width_bits(dest_symbol);
  if (bits != code_generator_ir_symbol_width_bits(lhs_symbol)) {
    return 1;
  }

  const char *dest_reg = code_generator_ir_symbol_register_name(dest_symbol, bits);
  const char *lhs_reg = code_generator_ir_symbol_register_name(lhs_symbol, bits);
  if (!dest_reg || !lhs_reg) {
    code_generator_set_error(generator,
                             "Invalid register in IR binary fast path");
    return 0;
  }

  if (instruction->rhs.kind == IR_OPERAND_INT &&
      ir_immediate_fits_signed_32(instruction->rhs.int_value) &&
      instruction->lhs.kind == IR_OPERAND_SYMBOL &&
      instruction->lhs.name && instruction->dest.kind == IR_OPERAND_SYMBOL &&
      instruction->dest.name &&
      strcmp(instruction->lhs.name, instruction->dest.name) == 0) {
    code_generator_emit(generator, "    %s %s, %lld\n", arith_instruction,
                        dest_reg, instruction->rhs.int_value);
    if (emitted) {
      *emitted = 1;
    }
    return 1;
  }

  Symbol *rhs_symbol =
      code_generator_ir_lookup_register_symbol(generator, &instruction->rhs);
  if (!rhs_symbol || bits != code_generator_ir_symbol_width_bits(rhs_symbol)) {
    return 1;
  }

  const char *rhs_reg = code_generator_ir_symbol_register_name(rhs_symbol, bits);
  if (!rhs_reg) {
    code_generator_set_error(generator,
                             "Invalid RHS register in IR binary fast path");
    return 0;
  }

  if (instruction->dest.kind != IR_OPERAND_SYMBOL || !instruction->dest.name ||
      instruction->rhs.kind != IR_OPERAND_SYMBOL || !instruction->rhs.name) {
    return 1;
  }

  int dest_is_lhs = strcmp(instruction->dest.name, instruction->lhs.name) == 0;
  int dest_is_rhs = strcmp(instruction->dest.name, instruction->rhs.name) == 0;

  if (!dest_is_lhs) {
    if (dest_is_rhs) {
      if (strcmp(instruction->text, "-") == 0) {
        return 1;
      }
      const char *tmp_reg = lhs_reg;
      lhs_reg = rhs_reg;
      rhs_reg = tmp_reg;
    } else {
      code_generator_emit(generator, "    mov %s, %s\n", dest_reg, lhs_reg);
    }
  }
  code_generator_emit(generator, "    %s %s, %s\n", arith_instruction, dest_reg,
                      rhs_reg);

  if (emitted) {
    *emitted = 1;
  }
  return 1;
}

static int ir_binary_operator_is_comparison(const char *op) {
  if (!op) {
    return 0;
  }
  return strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
         strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
         strcmp(op, ">") == 0 || strcmp(op, ">=") == 0;
}

static const char *ir_false_jump_for_comparison(const char *op) {
  if (!op) {
    return NULL;
  }
  if (strcmp(op, "==") == 0) {
    return "jne";
  }
  if (strcmp(op, "!=") == 0) {
    return "je";
  }
  if (strcmp(op, "<") == 0) {
    return "jge";
  }
  if (strcmp(op, "<=") == 0) {
    return "jg";
  }
  if (strcmp(op, ">") == 0) {
    return "jle";
  }
  if (strcmp(op, ">=") == 0) {
    return "jl";
  }
  return NULL;
}

static const char *ir_invert_conditional_jump(const char *jump) {
  if (!jump) {
    return NULL;
  }
  if (strcmp(jump, "je") == 0) {
    return "jne";
  }
  if (strcmp(jump, "jne") == 0) {
    return "je";
  }
  if (strcmp(jump, "jl") == 0) {
    return "jge";
  }
  if (strcmp(jump, "jge") == 0) {
    return "jl";
  }
  if (strcmp(jump, "jle") == 0) {
    return "jg";
  }
  if (strcmp(jump, "jg") == 0) {
    return "jle";
  }
  return NULL;
}

static int code_generator_try_emit_ir_compare_branch_register_fastpath(
    CodeGenerator *generator, const IRInstruction *compare_instruction,
    const char *false_jump, const char *target_label, int *emitted) {
  if (emitted) {
    *emitted = 0;
  }
  if (!generator || !compare_instruction || !false_jump || !target_label) {
    return 0;
  }
  if (compare_instruction->is_float) {
    return 1;
  }

  Symbol *lhs_symbol = code_generator_ir_lookup_register_symbol(
      generator, &compare_instruction->lhs);
  if (!lhs_symbol) {
    return 1;
  }

  int lhs_bits = code_generator_ir_symbol_width_bits(lhs_symbol);
  const char *lhs_reg =
      code_generator_ir_symbol_register_name(lhs_symbol, lhs_bits);
  if (!lhs_reg) {
    code_generator_set_error(
        generator, "Invalid lhs register in IR compare fast path");
    return 0;
  }

  if (compare_instruction->rhs.kind == IR_OPERAND_INT &&
      ir_immediate_fits_signed_32(compare_instruction->rhs.int_value)) {
    code_generator_emit(generator, "    cmp %s, %lld\n", lhs_reg,
                        compare_instruction->rhs.int_value);
    code_generator_emit(generator, "    %s %s\n", false_jump, target_label);
    if (emitted) {
      *emitted = 1;
    }
    return 1;
  }

  Symbol *rhs_symbol = code_generator_ir_lookup_register_symbol(
      generator, &compare_instruction->rhs);
  if (!rhs_symbol ||
      code_generator_ir_symbol_width_bits(rhs_symbol) != lhs_bits) {
    return 1;
  }

  const char *rhs_reg =
      code_generator_ir_symbol_register_name(rhs_symbol, lhs_bits);
  if (!rhs_reg) {
    code_generator_set_error(
        generator, "Invalid rhs register in IR compare fast path");
    return 0;
  }

  code_generator_emit(generator, "    cmp %s, %s\n", lhs_reg, rhs_reg);
  code_generator_emit(generator, "    %s %s\n", false_jump, target_label);
  if (emitted) {
    *emitted = 1;
  }
  return 1;
}

static int ir_function_requires_entry_safepoint(const IRFunction *function) {
  if (!function) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    if (!instruction) {
      continue;
    }

    if (instruction->op == IR_OP_CALL) {
      // Null/bounds trap blocks lower to puts+exit calls. They are cold terminal
      // paths and should not force every hot invocation through entry safepoint
      // spill/restore.
      if (instruction->text &&
          (strcmp(instruction->text, "puts") == 0 ||
           strcmp(instruction->text, "exit") == 0)) {
        continue;
      }
      return 1;
    }

    if (instruction->op == IR_OP_CALL_INDIRECT ||
        instruction->op == IR_OP_NEW ||
        instruction->op == IR_OP_INLINE_ASM) {
      return 1;
    }
  }

  return 0;
}

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

static int code_generator_load_ir_operand_into_register(
    CodeGenerator *generator, const IROperand *operand,
    IRTempTable *temp_table, x86Register target_register) {
  if (!generator) {
    return 0;
  }

  if (!code_generator_load_ir_operand(generator, operand, temp_table)) {
    return 0;
  }

  if (target_register != REG_RAX) {
    const char *target_name = code_generator_get_register_name(target_register);
    if (!target_name) {
      code_generator_set_error(generator, "Invalid target register in IR load");
      return 0;
    }
    code_generator_emit(generator, "    mov %s, rax\n", target_name);
  }

  return 1;
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
  if (!symbol) {
    code_generator_set_error(generator, "Unknown addr_of symbol '%s'",
                             instruction->lhs.name);
    return 0;
  }

  if (symbol->kind == SYMBOL_FUNCTION) {
    const char *func_name =
        code_generator_get_link_symbol_name(generator, instruction->lhs.name);
    if (!func_name) {
      code_generator_set_error(generator,
                               "Invalid function symbol in IR addr_of");
      return 0;
    }
    if (symbol->is_extern &&
        !code_generator_emit_extern_symbol(generator, func_name)) {
      return 0;
    }
    code_generator_emit(generator, "    lea rax, [rel %s]\n", func_name);
    return code_generator_store_ir_destination(generator, &instruction->dest,
                                               temp_table);
  }

  if (symbol->kind != SYMBOL_VARIABLE && symbol->kind != SYMBOL_PARAMETER) {
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
    if (symbol->is_extern &&
        !code_generator_emit_extern_symbol(generator, symbol_name)) {
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

  if (!code_generator_load_ir_operand_into_register(
          generator, &instruction->lhs, temp_table, REG_RCX)) {
    return 0;
  }
  if (!code_generator_load_ir_operand(generator, &instruction->dest,
                                      temp_table)) {
    return 0;
  }
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

static int code_generator_extract_positive_power_of_two(long long value,
                                                        unsigned int *shift_out,
                                                        unsigned long long *mask_out) {
  if (!shift_out || !mask_out || value <= 0) {
    return 0;
  }

  unsigned long long uvalue = (unsigned long long)value;
  if ((uvalue & (uvalue - 1ULL)) != 0ULL) {
    return 0;
  }

  unsigned int shift = 0;
  unsigned long long cursor = uvalue;
  while (cursor > 1ULL) {
    cursor >>= 1ULL;
    shift++;
  }

  *shift_out = shift;
  *mask_out = uvalue - 1ULL;
  return 1;
}

static void code_generator_emit_and_mask(CodeGenerator *generator,
                                         const char *target_register,
                                         unsigned long long mask) {
  if (!generator || !target_register) {
    return;
  }

  if (mask <= 0x7fffffffULL) {
    code_generator_emit(generator, "    and %s, %llu\n", target_register, mask);
  } else {
    code_generator_emit(generator, "    mov r10, 0x%016llx\n", mask);
    code_generator_emit(generator, "    and %s, r10\n", target_register);
  }
}

static int
code_generator_emit_ir_binary_fallback(CodeGenerator *generator,
                                       const IRInstruction *instruction,
                                       IRTempTable *temp_table) {
  if (!generator || !instruction || !instruction->text) {
    return 0;
  }

  const char *op = instruction->text;

  {
    int emitted_fastpath = 0;
    if (!code_generator_try_emit_ir_binary_register_fastpath(
            generator, instruction, &emitted_fastpath)) {
      return 0;
    }
    if (emitted_fastpath) {
      return 1;
    }
  }

  if (!instruction->is_float &&
      (strcmp(op, "/") == 0 || strcmp(op, "%") == 0) &&
      instruction->rhs.kind == IR_OPERAND_INT) {
    unsigned int shift = 0;
    unsigned long long mask = 0;
    if (code_generator_extract_positive_power_of_two(
            instruction->rhs.int_value, &shift, &mask)) {
      if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                          temp_table)) {
        return 0;
      }

      if (strcmp(op, "/") == 0) {
        // Signed division by 2^k with truncation toward zero.
        if (shift != 0) {
          code_generator_emit(generator, "    mov rcx, rax\n");
          code_generator_emit(generator, "    sar rcx, 63\n");
          code_generator_emit_and_mask(generator, "rcx", mask);
          code_generator_emit(generator, "    add rax, rcx\n");
          code_generator_emit(generator, "    sar rax, %u\n", shift);
        }
      } else {
        // Signed remainder by 2^k: r = x - (q << k), q = trunc(x / 2^k).
        if (shift == 0) {
          code_generator_emit(generator, "    mov rax, 0\n");
        } else {
          code_generator_emit(generator, "    mov r11, rax\n");
          code_generator_emit(generator, "    mov rcx, rax\n");
          code_generator_emit(generator, "    sar rcx, 63\n");
          code_generator_emit_and_mask(generator, "rcx", mask);
          code_generator_emit(generator, "    add rax, rcx\n");
          code_generator_emit(generator, "    sar rax, %u\n", shift);
          code_generator_emit(generator, "    shl rax, %u\n", shift);
          code_generator_emit(generator, "    sub r11, rax\n");
          code_generator_emit(generator, "    mov rax, r11\n");
        }
      }

      return code_generator_store_ir_destination(generator, &instruction->dest,
                                                 temp_table);
    }
  }

  if (!code_generator_load_ir_operand_into_register(
          generator, &instruction->rhs, temp_table, REG_R10)) {
    return 0;
  }
  if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                      temp_table)) {
    return 0;
  }

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
    } else if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) {
      code_generator_emit(generator, "    mov rcx, r10\n");
      code_generator_emit(generator, "    %s rax, cl\n", arith);
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

static int code_generator_ir_operand_has_float_type(CodeGenerator *generator,
                                                    const IROperand *operand) {
  if (!generator || !operand) {
    return 0;
  }

  if (operand->kind == IR_OPERAND_SYMBOL && operand->name) {
    Symbol *symbol =
        symbol_table_lookup(generator->symbol_table, operand->name);
    if (symbol && symbol->type &&
        code_generator_is_floating_point_type(symbol->type)) {
      return 1;
    }
  }

  return code_generator_ir_operand_is_float(operand);
}

static int code_generator_ir_call_argument_is_float(
    CodeGenerator *generator, Symbol *function_symbol,
    const IRInstruction *instruction, size_t argument_index) {
  if (!generator || !instruction ||
      argument_index >= instruction->argument_count) {
    return 0;
  }

  if (function_symbol && function_symbol->kind == SYMBOL_FUNCTION &&
      function_symbol->data.function.parameter_types &&
      argument_index < function_symbol->data.function.parameter_count) {
    Type *parameter_type =
        function_symbol->data.function.parameter_types[argument_index];
    if (parameter_type) {
      return code_generator_is_floating_point_type(parameter_type);
    }
  }

  return code_generator_ir_operand_has_float_type(
      generator, &instruction->arguments[argument_index]);
}

static int code_generator_emit_ir_call_argument_stack(CodeGenerator *generator,
                                                      const IROperand *operand,
                                                      IRTempTable *temp_table,
                                                      int stack_slot_offset) {
  if (!code_generator_load_ir_operand(generator, operand, temp_table)) {
    return 0;
  }
  code_generator_emit(generator, "    mov [rsp + %d], rax\n",
                      stack_slot_offset);
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
  if ((function_symbol && function_symbol->is_extern) || !function_symbol) {
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
  size_t ms_param_slot_count = 0;
  if (conv_spec->convention == CALLING_CONV_MS_X64) {
    ms_param_slot_count = conv_spec->int_param_count;
    if (conv_spec->float_param_count < ms_param_slot_count) {
      ms_param_slot_count = conv_spec->float_param_count;
    }
  }
  int stack_argument_count = 0;
  for (size_t i = 0; i < argument_count; i++) {
    is_float[i] = code_generator_ir_call_argument_is_float(
        generator, function_symbol, instruction, i);
    if (conv_spec->convention == CALLING_CONV_MS_X64) {
      if (i >= ms_param_slot_count) {
        goes_on_stack[i] = 1;
        stack_argument_count++;
      }
    } else if (is_float[i]) {
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

  int omit_shadow_space = 0;
  if (conv_spec->convention == CALLING_CONV_MS_X64 && function_symbol &&
      function_symbol->kind == SYMBOL_FUNCTION && !function_symbol->is_extern &&
      argument_count <= ms_param_slot_count) {
    // Internal register-only calls do not need caller home slots.
    omit_shadow_space = 1;
  }
  int shadow_space =
      (conv_spec->convention == CALLING_CONV_MS_X64 && !omit_shadow_space)
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

    if (conv_spec->convention == CALLING_CONV_MS_X64) {
      if (i >= ms_param_slot_count) {
        code_generator_set_error(
            generator, "IR call argument classification mismatch (Win64)");
        free(is_float);
        free(goes_on_stack);
        return 0;
      }
      x86Register target_register = (is_float && is_float[i])
                                        ? conv_spec->float_param_registers[i]
                                        : conv_spec->int_param_registers[i];
      if (!code_generator_emit_ir_call_argument_register(
              generator, &instruction->arguments[i], temp_table,
              target_register, (is_float && is_float[i]) ? 1 : 0)) {
        free(is_float);
        free(goes_on_stack);
        return 0;
      }
    } else {
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
  }

  code_generator_emit(generator, "    call %s\n", call_target);

  if (call_stack_total > 0) {
    code_generator_emit(generator, "    add rsp, %d\n", call_stack_total);
  }

  Type *return_type = NULL;
  if (function_symbol && function_symbol->kind == SYMBOL_FUNCTION &&
      function_symbol->type) {
    return_type = function_symbol->type;
    if (function_symbol->data.function.return_type) {
      return_type = function_symbol->data.function.return_type;
    }
  }
  if (return_type && code_generator_is_floating_point_type(return_type)) {
    code_generator_emit(generator, "    movq rax, xmm0\n");
  }
  code_generator_handle_return_value(generator, return_type);

  free(is_float);
  free(goes_on_stack);
  return !generator->has_error;
}

static int code_generator_emit_ir_cast(CodeGenerator *generator,
                                       const IRInstruction *instruction,
                                       IRTempTable *temp_table) {
  if (!generator || !instruction || !instruction->text) {
    return 0;
  }

  Type *target_type = NULL;
  if (generator->type_checker) {
    target_type = type_checker_get_type_by_name(generator->type_checker,
                                                instruction->text);
  }

  if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                      temp_table)) {
    return 0;
  }

  int source_is_float = instruction->is_float;
  int target_is_float =
      target_type ? code_generator_is_floating_point_type(target_type) : 0;

  int target_is_unsigned = 0;
  if (target_type) {
    if (target_type->kind == TYPE_UINT8 || target_type->kind == TYPE_UINT16 ||
        target_type->kind == TYPE_UINT32 || target_type->kind == TYPE_UINT64) {
      target_is_unsigned = 1;
    }
  }

  int target_size = target_type ? (int)target_type->size : 8;
  if (target_size <= 0)
    target_size = 8;
  if (target_type && (target_type->kind == TYPE_POINTER ||
                      target_type->kind == TYPE_FUNCTION_POINTER)) {
    target_size = 8;
  }

  if (source_is_float && !target_is_float) {
    code_generator_emit(generator, "    movq xmm0, rax\n");
    code_generator_emit(generator, "    cvttsd2si rax, xmm0\n");
  } else if (!source_is_float && target_is_float) {
    code_generator_emit(generator, "    cvtsi2sd xmm0, rax\n");
    code_generator_emit(generator, "    movq rax, xmm0\n");
  } else if (!source_is_float && !target_is_float) {
    if (target_size == 1) {
      if (target_is_unsigned) {
        code_generator_emit(generator, "    movzx rax, al\n");
      } else {
        code_generator_emit(generator, "    movsx rax, al\n");
      }
    } else if (target_size == 2) {
      if (target_is_unsigned) {
        code_generator_emit(generator, "    movzx rax, ax\n");
      } else {
        code_generator_emit(generator, "    movsx rax, ax\n");
      }
    } else if (target_size == 4) {
      if (target_is_unsigned) {
        code_generator_emit(generator, "    mov eax, eax\n");
      } else {
        code_generator_emit(generator, "    movsxd rax, eax\n");
      }
    }
  }

  return code_generator_store_ir_destination(generator, &instruction->dest,
                                             temp_table);
}

static int code_generator_emit_ir_compare_branch_with_jump(
    CodeGenerator *generator, const IRInstruction *compare_instruction,
    const char *jump, const char *target_label, IRTempTable *temp_table) {
  if (!generator || !compare_instruction || !jump || !target_label ||
      !temp_table) {
    return 0;
  }

  {
    int emitted_fastpath = 0;
    if (!code_generator_try_emit_ir_compare_branch_register_fastpath(
            generator, compare_instruction, jump, target_label,
            &emitted_fastpath)) {
      return 0;
    }
    if (emitted_fastpath) {
      return 1;
    }
  }

  if (!code_generator_load_ir_operand_into_register(
          generator, &compare_instruction->rhs, temp_table, REG_R10)) {
    return 0;
  }
  if (!code_generator_load_ir_operand(generator, &compare_instruction->lhs,
                                      temp_table)) {
    return 0;
  }
  code_generator_emit(generator, "    cmp rax, r10\n");
  code_generator_emit(generator, "    %s %s\n", jump, target_label);
  return 1;
}

static int code_generator_emit_ir_compare_branch_zero(
    CodeGenerator *generator, const IRInstruction *compare_instruction,
    const IRInstruction *branch_instruction, IRTempTable *temp_table) {
  if (!generator || !compare_instruction || !branch_instruction ||
      !temp_table || !branch_instruction->text) {
    return 0;
  }

  const char *false_jump =
      ir_false_jump_for_comparison(compare_instruction->text);
  if (!false_jump) {
    return 0;
  }

  return code_generator_emit_ir_compare_branch_with_jump(
      generator, compare_instruction, false_jump, branch_instruction->text,
      temp_table);
}

static const char *code_generator_ir_memory_operand_for_size(int size_bytes);

static int code_generator_emit_ir_branch_zero_jump_over_label(
    CodeGenerator *generator, const IRInstruction *branch_instruction,
    const IRInstruction *jump_instruction, IRTempTable *temp_table) {
  if (!generator || !branch_instruction || !jump_instruction || !temp_table ||
      !jump_instruction->text) {
    return 0;
  }

  if (branch_instruction->lhs.kind == IR_OPERAND_INT) {
    if (branch_instruction->lhs.int_value != 0) {
      code_generator_emit(generator, "    jmp %s\n", jump_instruction->text);
    }
    return 1;
  }

  if (branch_instruction->lhs.kind == IR_OPERAND_SYMBOL &&
      branch_instruction->lhs.name) {
    Symbol *symbol = symbol_table_lookup(generator->symbol_table,
                                         branch_instruction->lhs.name);
    if (symbol &&
        (symbol->kind == SYMBOL_VARIABLE || symbol->kind == SYMBOL_PARAMETER)) {
      if (symbol->data.variable.is_in_register) {
        const char *reg_name = code_generator_get_register_name(
            (x86Register)symbol->data.variable.register_id);
        if (!reg_name) {
          code_generator_set_error(
              generator,
              "Invalid register in IR branch-nonzero fast path");
          return 0;
        }
        code_generator_emit(generator, "    test %s, %s\n", reg_name, reg_name);
        code_generator_emit(generator, "    jnz %s\n", jump_instruction->text);
        return 1;
      }

      if (symbol->data.variable.memory_offset > 0) {
        int size_bytes = 8;
        if (symbol->type && symbol->type->size > 0 && symbol->type->size <= 8) {
          size_bytes = (int)symbol->type->size;
        }
        const char *mem_op = code_generator_ir_memory_operand_for_size(size_bytes);
        code_generator_emit(generator, "    cmp %s [rbp - %d], 0\n", mem_op,
                            symbol->data.variable.memory_offset);
        code_generator_emit(generator, "    jnz %s\n", jump_instruction->text);
        return 1;
      }
    }
  }

  if (branch_instruction->lhs.kind == IR_OPERAND_TEMP &&
      branch_instruction->lhs.name) {
    int offset = ir_temp_table_get_offset(temp_table, branch_instruction->lhs.name);
    if (offset > 0) {
      code_generator_emit(generator, "    cmp qword [rbp - %d], 0\n", offset);
      code_generator_emit(generator, "    jnz %s\n", jump_instruction->text);
      return 1;
    }
  }

  if (!code_generator_load_ir_operand(generator, &branch_instruction->lhs,
                                      temp_table)) {
    return 0;
  }
  code_generator_emit(generator, "    test rax, rax\n");
  code_generator_emit(generator, "    jnz %s\n", jump_instruction->text);
  return 1;
}

static const char *code_generator_ir_memory_operand_for_size(int size_bytes) {
  if (size_bytes <= 1) {
    return "byte";
  }
  if (size_bytes == 2) {
    return "word";
  }
  if (size_bytes == 4) {
    return "dword";
  }
  return "qword";
}

static int code_generator_try_emit_ir_branch_zero_fastpath(
    CodeGenerator *generator, const IRInstruction *instruction,
    IRTempTable *temp_table, int *emitted) {
  if (emitted) {
    *emitted = 0;
  }
  if (!generator || !instruction || !instruction->text || !temp_table) {
    return 0;
  }

  if (instruction->lhs.kind == IR_OPERAND_INT) {
    if (instruction->lhs.int_value == 0) {
      code_generator_emit(generator, "    jmp %s\n", instruction->text);
    }
    if (emitted) {
      *emitted = 1;
    }
    return 1;
  }

  if (instruction->lhs.kind == IR_OPERAND_SYMBOL && instruction->lhs.name) {
    Symbol *symbol =
        symbol_table_lookup(generator->symbol_table, instruction->lhs.name);
    if (symbol &&
        (symbol->kind == SYMBOL_VARIABLE || symbol->kind == SYMBOL_PARAMETER)) {
      if (symbol->data.variable.is_in_register) {
        const char *reg_name = code_generator_get_register_name(
            (x86Register)symbol->data.variable.register_id);
        if (!reg_name) {
          code_generator_set_error(
              generator, "Invalid register in IR branch-zero fast path");
          return 0;
        }
        code_generator_emit(generator, "    test %s, %s\n", reg_name, reg_name);
        code_generator_emit(generator, "    jz %s\n", instruction->text);
        if (emitted) {
          *emitted = 1;
        }
        return 1;
      }

      if (symbol->data.variable.memory_offset > 0) {
        int size_bytes = 8;
        if (symbol->type && symbol->type->size > 0 && symbol->type->size <= 8) {
          size_bytes = (int)symbol->type->size;
        }
        const char *mem_op = code_generator_ir_memory_operand_for_size(size_bytes);
        code_generator_emit(generator, "    cmp %s [rbp - %d], 0\n", mem_op,
                            symbol->data.variable.memory_offset);
        code_generator_emit(generator, "    jz %s\n", instruction->text);
        if (emitted) {
          *emitted = 1;
        }
        return 1;
      }
    }
  }

  if (instruction->lhs.kind == IR_OPERAND_TEMP && instruction->lhs.name) {
    int offset = ir_temp_table_get_offset(temp_table, instruction->lhs.name);
    if (offset > 0) {
      code_generator_emit(generator, "    cmp qword [rbp - %d], 0\n", offset);
      code_generator_emit(generator, "    jz %s\n", instruction->text);
      if (emitted) {
        *emitted = 1;
      }
      return 1;
    }
  }

  return 1;
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
    {
      int emitted_fastpath = 0;
      if (!code_generator_try_emit_ir_branch_zero_fastpath(
              generator, instruction, temp_table, &emitted_fastpath)) {
        return 0;
      }
      if (emitted_fastpath) {
        return 1;
      }
    }
    if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                        temp_table)) {
      return 0;
    }
    code_generator_emit(generator, "    test rax, rax\n");
    code_generator_emit(generator, "    jz %s\n",
                        instruction->text ? instruction->text : "ir_missing");
    return 1;

  case IR_OP_BRANCH_EQ:
    if (!code_generator_load_ir_operand_into_register(
            generator, &instruction->rhs, temp_table, REG_R10)) {
      return 0;
    }
    if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                        temp_table)) {
      return 0;
    }
    code_generator_emit(generator, "    cmp rax, r10\n");
    code_generator_emit(generator, "    je %s\n",
                        instruction->text ? instruction->text : "ir_missing");
    return 1;

  case IR_OP_ASSIGN:
    {
      int emitted_fastpath = 0;
      if (!code_generator_try_emit_ir_assign_symbol_to_symbol(
              generator, instruction, &emitted_fastpath)) {
        return 0;
      }
      if (emitted_fastpath) {
        return 1;
      }
    }
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

  case IR_OP_CALL_INDIRECT: {
    CallingConventionSpec *conv_spec =
        generator->register_allocator->calling_convention;
    if (!conv_spec) {
      code_generator_set_error(generator, "No calling convention configured");
      return 0;
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
        code_generator_set_error(generator, "Out of memory for indirect call");
        return 0;
      }
    }

    int int_reg_cursor = 0;
    int float_reg_cursor = 0;
    size_t ms_param_slot_count = 0;
    if (conv_spec->convention == CALLING_CONV_MS_X64) {
      ms_param_slot_count = conv_spec->int_param_count;
      if (conv_spec->float_param_count < ms_param_slot_count) {
        ms_param_slot_count = conv_spec->float_param_count;
      }
    }
    int stack_argument_count = 0;
    for (size_t i = 0; i < argument_count; i++) {
      is_float[i] = code_generator_ir_call_argument_is_float(generator, NULL,
                                                             instruction, i);
      if (conv_spec->convention == CALLING_CONV_MS_X64) {
        if (i >= ms_param_slot_count) {
          goes_on_stack[i] = 1;
          stack_argument_count++;
        }
      } else if (is_float[i]) {
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

    code_generator_emit(generator, "    ; Indirect function call\n");

    int shadow_space = (conv_spec->convention == CALLING_CONV_MS_X64)
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

    int_reg_cursor = 0;
    float_reg_cursor = 0;
    for (size_t i = 0; i < argument_count; i++) {
      if (goes_on_stack && goes_on_stack[i]) {
        continue;
      }
      if (conv_spec->convention == CALLING_CONV_MS_X64) {
        x86Register target_register = (is_float && is_float[i])
                                          ? conv_spec->float_param_registers[i]
                                          : conv_spec->int_param_registers[i];
        if (!code_generator_emit_ir_call_argument_register(
                generator, &instruction->arguments[i], temp_table,
                target_register, (is_float && is_float[i]) ? 1 : 0)) {
          free(is_float);
          free(goes_on_stack);
          return 0;
        }
      } else {
        if (is_float && is_float[i]) {
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
    }

    if (!code_generator_load_ir_operand(generator, &instruction->lhs,
                                        temp_table)) {
      free(is_float);
      free(goes_on_stack);
      return 0;
    }
    code_generator_emit(generator, "    call rax\n");

    if (call_stack_total > 0) {
      code_generator_emit(generator, "    add rsp, %d\n", call_stack_total);
    }

    free(is_float);
    free(goes_on_stack);
    code_generator_handle_return_value(generator, NULL);
    return code_generator_store_ir_destination(generator, &instruction->dest,
                                               temp_table);
  }

  case IR_OP_NEW:
    return code_generator_emit_ir_new(generator, instruction, temp_table);

  case IR_OP_CAST:
    return code_generator_emit_ir_cast(generator, instruction, temp_table);

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

static int code_generator_add_stack_size(CodeGenerator *generator,
                                         int *stack_size, int amount,
                                         const char *function_name) {
  if (!generator || !stack_size || amount < 0) {
    return 0;
  }
  if (*stack_size > INT_MAX - amount) {
    code_generator_set_error(generator,
                             "Stack frame too large in function '%s' (integer "
                             "overflow while sizing)",
                             function_name ? function_name : "<unknown>");
    return 0;
  }
  *stack_size += amount;
  return 1;
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
  IRSymbolStatsMap symbol_stats = {0};
  IRPromotedSymbol promoted_symbols[sizeof(IR_PROMOTION_REGISTERS) /
                                    sizeof(IR_PROMOTION_REGISTERS[0])] = {0};
  size_t promoted_symbol_count = 0;
  int promoted_save_bytes = 0;
  int stack_offset_base = parameter_home_size;

  for (size_t i = 0; i < ir_function->instruction_count; i++) {
    IRInstruction *instruction = &ir_function->instructions[i];
    if (!instruction) {
      continue;
    }

    if (!ir_symbol_stats_map_record_instruction(&symbol_stats, instruction)) {
      code_generator_set_error(generator,
                               "Out of memory while tracking IR symbol usage");
      ir_temp_table_destroy(&temp_table);
      ir_local_table_destroy(&local_table);
      ir_symbol_stats_map_destroy(&symbol_stats);
      symbol_table_exit_scope(generator->symbol_table);
      return 0;
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
           ir_symbol_stats_map_destroy(&symbol_stats);
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
        ir_symbol_stats_map_destroy(&symbol_stats);
        symbol_table_exit_scope(generator->symbol_table);
        return 0;
      }
      if (!code_generator_add_stack_size(
              generator, &stack_size, local_size + 15, function_data->name)) {
        ir_temp_table_destroy(&temp_table);
        ir_local_table_destroy(&local_table);
        ir_symbol_stats_map_destroy(&symbol_stats);
        symbol_table_exit_scope(generator->symbol_table);
        return 0;
      }
    }

    if (instruction->dest.kind == IR_OPERAND_TEMP && instruction->dest.name) {
      if (!ir_temp_table_add(&temp_table, instruction->dest.name)) {
        code_generator_set_error(generator,
                                 "Out of memory while tracking IR temp '%s'",
                                 instruction->dest.name);
        ir_temp_table_destroy(&temp_table);
        ir_local_table_destroy(&local_table);
        ir_symbol_stats_map_destroy(&symbol_stats);
        symbol_table_exit_scope(generator->symbol_table);
        return 0;
      }
    }
  }
  if (temp_table.count > (size_t)(INT_MAX / (8 + 15))) {
    code_generator_set_error(
        generator, "Stack frame too large in function '%s' (too many temps)",
        function_data->name);
    ir_temp_table_destroy(&temp_table);
    ir_local_table_destroy(&local_table);
    ir_symbol_stats_map_destroy(&symbol_stats);
    symbol_table_exit_scope(generator->symbol_table);
    return 0;
  }
  if (!code_generator_add_stack_size(generator, &stack_size,
                                     (int)(temp_table.count * (8 + 15)),
                                     function_data->name)) {
    ir_temp_table_destroy(&temp_table);
    ir_local_table_destroy(&local_table);
    ir_symbol_stats_map_destroy(&symbol_stats);
    symbol_table_exit_scope(generator->symbol_table);
    return 0;
  }

  for (size_t reg_index = 0;
       reg_index < (sizeof(IR_PROMOTION_REGISTERS) /
                    sizeof(IR_PROMOTION_REGISTERS[0]));
       reg_index++) {
    const char *best_name = NULL;
    size_t best_use_count = 0;

    for (size_t i = 0; i < function_data->parameter_count; i++) {
      const char *name = function_data->parameter_names[i];
      const char *type_name = function_data->parameter_types[i];
      if (!name || ir_promoted_symbol_find(promoted_symbols, promoted_symbol_count,
                                           name) >= 0 ||
          ir_symbol_stats_map_is_address_taken(&symbol_stats, name)) {
        continue;
      }

      Type *param_type = NULL;
      if (type_name) {
        param_type =
            type_checker_get_type_by_name(generator->type_checker, type_name);
      }
      if (!param_type || !ir_is_integer_or_pointer_promotable(param_type)) {
        continue;
      }

      size_t use_count = ir_symbol_stats_map_get_use_count(&symbol_stats, name);
      if (use_count > best_use_count) {
        best_use_count = use_count;
        best_name = name;
      }
    }

    for (size_t i = 0; i < local_table.count; i++) {
      const char *name = local_table.items[i].name;
      if (!name || ir_promoted_symbol_find(promoted_symbols, promoted_symbol_count,
                                           name) >= 0 ||
          ir_symbol_stats_map_is_address_taken(&symbol_stats, name)) {
        continue;
      }

      Symbol *local_symbol =
          symbol_table_lookup_current_scope(generator->symbol_table, name);
      if (!local_symbol || local_symbol->kind != SYMBOL_VARIABLE ||
          !ir_is_integer_or_pointer_promotable(local_symbol->type)) {
        continue;
      }

      size_t use_count = ir_symbol_stats_map_get_use_count(&symbol_stats, name);
      if (use_count > best_use_count) {
        best_use_count = use_count;
        best_name = name;
      }
    }

    // Avoid paying save/restore cost for trivially-used symbols.
    if (!best_name || best_use_count < 3) {
      break;
    }

    promoted_symbols[promoted_symbol_count].name = best_name;
    promoted_symbols[promoted_symbol_count].reg =
        IR_PROMOTION_REGISTERS[reg_index];
    promoted_symbol_count++;
  }

  if (promoted_symbol_count > (size_t)(INT_MAX / 8)) {
    code_generator_set_error(
        generator, "Too many promoted symbols in function '%s'",
        function_data->name);
    ir_temp_table_destroy(&temp_table);
    ir_local_table_destroy(&local_table);
    ir_symbol_stats_map_destroy(&symbol_stats);
    symbol_table_exit_scope(generator->symbol_table);
    return 0;
  }
  promoted_save_bytes = (int)(promoted_symbol_count * 8);
  if (promoted_save_bytes > 0) {
    if (!code_generator_add_stack_size(generator, &stack_size, promoted_save_bytes,
                                       function_data->name)) {
      ir_temp_table_destroy(&temp_table);
      ir_local_table_destroy(&local_table);
      ir_symbol_stats_map_destroy(&symbol_stats);
      symbol_table_exit_scope(generator->symbol_table);
      return 0;
    }
    stack_offset_base += promoted_save_bytes;
    for (size_t i = 0; i < promoted_symbol_count; i++) {
      promoted_symbols[i].save_offset = parameter_home_size + (int)((i + 1) * 8);
    }
  }

  if (generator->type_checker && generator->type_checker->error_reporter &&
      stack_size >= STACK_FRAME_WARN_THRESHOLD) {
    char warning[256];
    snprintf(warning, sizeof(warning),
             "Large stack frame in function '%s' (%d bytes); this increases "
             "stack overflow risk",
             function_data->name, stack_size);
    error_reporter_add_warning(generator->type_checker->error_reporter,
                               ERROR_SEMANTIC, function_declaration->location,
                               warning);
  }

  code_generator_function_prologue(generator, function_data->name, stack_size);
  generator->current_stack_offset = stack_offset_base;
  code_generator_register_function_parameters(generator, function_data,
                                              parameter_home_size);
  for (size_t i = 0; i < promoted_symbol_count; i++) {
    const char *reg_name = code_generator_get_register_name(promoted_symbols[i].reg);
    if (!reg_name) {
      code_generator_set_error(generator,
                               "Invalid promoted register in function '%s'",
                               function_data->name);
      break;
    }
    code_generator_emit(generator, "    mov [rbp - %d], %s\n",
                        promoted_symbols[i].save_offset, reg_name);
  }

  if (ir_function_requires_entry_safepoint(ir_function)) {
    {
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
    if (conv_spec && conv_spec->convention == CALLING_CONV_MS_X64) {
      code_generator_emit(generator,
                          "    ; Spill GPRs so GC sees register-held roots\n");
      code_generator_emit(generator, "    push rax\n");
      code_generator_emit(generator, "    push rbx\n");
      code_generator_emit(generator, "    push rcx\n");
      code_generator_emit(generator, "    push rdx\n");
      code_generator_emit(generator, "    push rsi\n");
      code_generator_emit(generator, "    push rdi\n");
      code_generator_emit(generator, "    push r8\n");
      code_generator_emit(generator, "    push r9\n");
      code_generator_emit(generator, "    push r10\n");
      code_generator_emit(generator, "    push r11\n");
      code_generator_emit(generator, "    push r12\n");
      code_generator_emit(generator, "    push r13\n");
      code_generator_emit(generator, "    push r14\n");
      code_generator_emit(generator, "    push r15\n");
      code_generator_emit(generator,
                          "    ; Spill XMM registers for conservative root scan\n");
#if defined(Methlang_SAFEPOINT_SPILL_XMM31)
      code_generator_emit(generator, "    sub rsp, 512\n");
#else
      code_generator_emit(generator, "    sub rsp, 256\n");
#endif
      code_generator_emit(generator, "    movdqu [rsp + 0], xmm0\n");
      code_generator_emit(generator, "    movdqu [rsp + 16], xmm1\n");
      code_generator_emit(generator, "    movdqu [rsp + 32], xmm2\n");
      code_generator_emit(generator, "    movdqu [rsp + 48], xmm3\n");
      code_generator_emit(generator, "    movdqu [rsp + 64], xmm4\n");
      code_generator_emit(generator, "    movdqu [rsp + 80], xmm5\n");
      code_generator_emit(generator, "    movdqu [rsp + 96], xmm6\n");
      code_generator_emit(generator, "    movdqu [rsp + 112], xmm7\n");
      code_generator_emit(generator, "    movdqu [rsp + 128], xmm8\n");
      code_generator_emit(generator, "    movdqu [rsp + 144], xmm9\n");
      code_generator_emit(generator, "    movdqu [rsp + 160], xmm10\n");
      code_generator_emit(generator, "    movdqu [rsp + 176], xmm11\n");
      code_generator_emit(generator, "    movdqu [rsp + 192], xmm12\n");
      code_generator_emit(generator, "    movdqu [rsp + 208], xmm13\n");
      code_generator_emit(generator, "    movdqu [rsp + 224], xmm14\n");
      code_generator_emit(generator, "    movdqu [rsp + 240], xmm15\n");
#if defined(Methlang_SAFEPOINT_SPILL_XMM31)
      code_generator_emit(generator, "    movdqu [rsp + 256], xmm16\n");
      code_generator_emit(generator, "    movdqu [rsp + 272], xmm17\n");
      code_generator_emit(generator, "    movdqu [rsp + 288], xmm18\n");
      code_generator_emit(generator, "    movdqu [rsp + 304], xmm19\n");
      code_generator_emit(generator, "    movdqu [rsp + 320], xmm20\n");
      code_generator_emit(generator, "    movdqu [rsp + 336], xmm21\n");
      code_generator_emit(generator, "    movdqu [rsp + 352], xmm22\n");
      code_generator_emit(generator, "    movdqu [rsp + 368], xmm23\n");
      code_generator_emit(generator, "    movdqu [rsp + 384], xmm24\n");
      code_generator_emit(generator, "    movdqu [rsp + 400], xmm25\n");
      code_generator_emit(generator, "    movdqu [rsp + 416], xmm26\n");
      code_generator_emit(generator, "    movdqu [rsp + 432], xmm27\n");
      code_generator_emit(generator, "    movdqu [rsp + 448], xmm28\n");
      code_generator_emit(generator, "    movdqu [rsp + 464], xmm29\n");
      code_generator_emit(generator, "    movdqu [rsp + 480], xmm30\n");
      code_generator_emit(generator, "    movdqu [rsp + 496], xmm31\n");
#endif
      code_generator_emit(generator, "    sub rsp, 32\n");
      code_generator_emit(generator, "    mov %s, rsp\n", first_param_reg);
      code_generator_emit(generator, "    extern gc_safepoint\n");
      code_generator_emit(generator, "    call gc_safepoint\n");
      code_generator_emit(generator, "    add rsp, 32\n");
      code_generator_emit(generator, "    movdqu xmm0, [rsp + 0]\n");
      code_generator_emit(generator, "    movdqu xmm1, [rsp + 16]\n");
      code_generator_emit(generator, "    movdqu xmm2, [rsp + 32]\n");
      code_generator_emit(generator, "    movdqu xmm3, [rsp + 48]\n");
      code_generator_emit(generator, "    movdqu xmm4, [rsp + 64]\n");
      code_generator_emit(generator, "    movdqu xmm5, [rsp + 80]\n");
      code_generator_emit(generator, "    movdqu xmm6, [rsp + 96]\n");
      code_generator_emit(generator, "    movdqu xmm7, [rsp + 112]\n");
      code_generator_emit(generator, "    movdqu xmm8, [rsp + 128]\n");
      code_generator_emit(generator, "    movdqu xmm9, [rsp + 144]\n");
      code_generator_emit(generator, "    movdqu xmm10, [rsp + 160]\n");
      code_generator_emit(generator, "    movdqu xmm11, [rsp + 176]\n");
      code_generator_emit(generator, "    movdqu xmm12, [rsp + 192]\n");
      code_generator_emit(generator, "    movdqu xmm13, [rsp + 208]\n");
      code_generator_emit(generator, "    movdqu xmm14, [rsp + 224]\n");
      code_generator_emit(generator, "    movdqu xmm15, [rsp + 240]\n");
#if defined(Methlang_SAFEPOINT_SPILL_XMM31)
      code_generator_emit(generator, "    movdqu xmm16, [rsp + 256]\n");
      code_generator_emit(generator, "    movdqu xmm17, [rsp + 272]\n");
      code_generator_emit(generator, "    movdqu xmm18, [rsp + 288]\n");
      code_generator_emit(generator, "    movdqu xmm19, [rsp + 304]\n");
      code_generator_emit(generator, "    movdqu xmm20, [rsp + 320]\n");
      code_generator_emit(generator, "    movdqu xmm21, [rsp + 336]\n");
      code_generator_emit(generator, "    movdqu xmm22, [rsp + 352]\n");
      code_generator_emit(generator, "    movdqu xmm23, [rsp + 368]\n");
      code_generator_emit(generator, "    movdqu xmm24, [rsp + 384]\n");
      code_generator_emit(generator, "    movdqu xmm25, [rsp + 400]\n");
      code_generator_emit(generator, "    movdqu xmm26, [rsp + 416]\n");
      code_generator_emit(generator, "    movdqu xmm27, [rsp + 432]\n");
      code_generator_emit(generator, "    movdqu xmm28, [rsp + 448]\n");
      code_generator_emit(generator, "    movdqu xmm29, [rsp + 464]\n");
      code_generator_emit(generator, "    movdqu xmm30, [rsp + 480]\n");
      code_generator_emit(generator, "    movdqu xmm31, [rsp + 496]\n");
      code_generator_emit(generator, "    add rsp, 512\n");
#else
      code_generator_emit(generator, "    add rsp, 256\n");
#endif
      code_generator_emit(generator, "    pop r15\n");
      code_generator_emit(generator, "    pop r14\n");
      code_generator_emit(generator, "    pop r13\n");
      code_generator_emit(generator, "    pop r12\n");
      code_generator_emit(generator, "    pop r11\n");
      code_generator_emit(generator, "    pop r10\n");
      code_generator_emit(generator, "    pop r9\n");
      code_generator_emit(generator, "    pop r8\n");
      code_generator_emit(generator, "    pop rdi\n");
      code_generator_emit(generator, "    pop rsi\n");
      code_generator_emit(generator, "    pop rdx\n");
      code_generator_emit(generator, "    pop rcx\n");
      code_generator_emit(generator, "    pop rbx\n");
      code_generator_emit(generator, "    pop rax\n");
    } else {
      code_generator_emit(generator,
                          "    ; Spill GPRs so GC sees register-held roots\n");
      code_generator_emit(generator, "    push rax\n");
      code_generator_emit(generator, "    push rbx\n");
      code_generator_emit(generator, "    push rcx\n");
      code_generator_emit(generator, "    push rdx\n");
      code_generator_emit(generator, "    push rsi\n");
      code_generator_emit(generator, "    push rdi\n");
      code_generator_emit(generator, "    push r8\n");
      code_generator_emit(generator, "    push r9\n");
      code_generator_emit(generator, "    push r10\n");
      code_generator_emit(generator, "    push r11\n");
      code_generator_emit(generator, "    push r12\n");
      code_generator_emit(generator, "    push r13\n");
      code_generator_emit(generator, "    push r14\n");
      code_generator_emit(generator, "    push r15\n");
      code_generator_emit(generator,
                          "    ; Spill XMM registers for conservative root scan\n");
#if defined(Methlang_SAFEPOINT_SPILL_XMM31)
      code_generator_emit(generator, "    sub rsp, 512\n");
#else
      code_generator_emit(generator, "    sub rsp, 256\n");
#endif
      code_generator_emit(generator, "    movdqu [rsp + 0], xmm0\n");
      code_generator_emit(generator, "    movdqu [rsp + 16], xmm1\n");
      code_generator_emit(generator, "    movdqu [rsp + 32], xmm2\n");
      code_generator_emit(generator, "    movdqu [rsp + 48], xmm3\n");
      code_generator_emit(generator, "    movdqu [rsp + 64], xmm4\n");
      code_generator_emit(generator, "    movdqu [rsp + 80], xmm5\n");
      code_generator_emit(generator, "    movdqu [rsp + 96], xmm6\n");
      code_generator_emit(generator, "    movdqu [rsp + 112], xmm7\n");
      code_generator_emit(generator, "    movdqu [rsp + 128], xmm8\n");
      code_generator_emit(generator, "    movdqu [rsp + 144], xmm9\n");
      code_generator_emit(generator, "    movdqu [rsp + 160], xmm10\n");
      code_generator_emit(generator, "    movdqu [rsp + 176], xmm11\n");
      code_generator_emit(generator, "    movdqu [rsp + 192], xmm12\n");
      code_generator_emit(generator, "    movdqu [rsp + 208], xmm13\n");
      code_generator_emit(generator, "    movdqu [rsp + 224], xmm14\n");
      code_generator_emit(generator, "    movdqu [rsp + 240], xmm15\n");
#if defined(Methlang_SAFEPOINT_SPILL_XMM31)
      code_generator_emit(generator, "    movdqu [rsp + 256], xmm16\n");
      code_generator_emit(generator, "    movdqu [rsp + 272], xmm17\n");
      code_generator_emit(generator, "    movdqu [rsp + 288], xmm18\n");
      code_generator_emit(generator, "    movdqu [rsp + 304], xmm19\n");
      code_generator_emit(generator, "    movdqu [rsp + 320], xmm20\n");
      code_generator_emit(generator, "    movdqu [rsp + 336], xmm21\n");
      code_generator_emit(generator, "    movdqu [rsp + 352], xmm22\n");
      code_generator_emit(generator, "    movdqu [rsp + 368], xmm23\n");
      code_generator_emit(generator, "    movdqu [rsp + 384], xmm24\n");
      code_generator_emit(generator, "    movdqu [rsp + 400], xmm25\n");
      code_generator_emit(generator, "    movdqu [rsp + 416], xmm26\n");
      code_generator_emit(generator, "    movdqu [rsp + 432], xmm27\n");
      code_generator_emit(generator, "    movdqu [rsp + 448], xmm28\n");
      code_generator_emit(generator, "    movdqu [rsp + 464], xmm29\n");
      code_generator_emit(generator, "    movdqu [rsp + 480], xmm30\n");
      code_generator_emit(generator, "    movdqu [rsp + 496], xmm31\n");
#endif
      code_generator_emit(generator, "    mov %s, rsp\n", first_param_reg);
      code_generator_emit(generator, "    extern gc_safepoint\n");
      code_generator_emit(generator, "    call gc_safepoint\n");
      code_generator_emit(generator, "    movdqu xmm0, [rsp + 0]\n");
      code_generator_emit(generator, "    movdqu xmm1, [rsp + 16]\n");
      code_generator_emit(generator, "    movdqu xmm2, [rsp + 32]\n");
      code_generator_emit(generator, "    movdqu xmm3, [rsp + 48]\n");
      code_generator_emit(generator, "    movdqu xmm4, [rsp + 64]\n");
      code_generator_emit(generator, "    movdqu xmm5, [rsp + 80]\n");
      code_generator_emit(generator, "    movdqu xmm6, [rsp + 96]\n");
      code_generator_emit(generator, "    movdqu xmm7, [rsp + 112]\n");
      code_generator_emit(generator, "    movdqu xmm8, [rsp + 128]\n");
      code_generator_emit(generator, "    movdqu xmm9, [rsp + 144]\n");
      code_generator_emit(generator, "    movdqu xmm10, [rsp + 160]\n");
      code_generator_emit(generator, "    movdqu xmm11, [rsp + 176]\n");
      code_generator_emit(generator, "    movdqu xmm12, [rsp + 192]\n");
      code_generator_emit(generator, "    movdqu xmm13, [rsp + 208]\n");
      code_generator_emit(generator, "    movdqu xmm14, [rsp + 224]\n");
      code_generator_emit(generator, "    movdqu xmm15, [rsp + 240]\n");
#if defined(Methlang_SAFEPOINT_SPILL_XMM31)
      code_generator_emit(generator, "    movdqu xmm16, [rsp + 256]\n");
      code_generator_emit(generator, "    movdqu xmm17, [rsp + 272]\n");
      code_generator_emit(generator, "    movdqu xmm18, [rsp + 288]\n");
      code_generator_emit(generator, "    movdqu xmm19, [rsp + 304]\n");
      code_generator_emit(generator, "    movdqu xmm20, [rsp + 320]\n");
      code_generator_emit(generator, "    movdqu xmm21, [rsp + 336]\n");
      code_generator_emit(generator, "    movdqu xmm22, [rsp + 352]\n");
      code_generator_emit(generator, "    movdqu xmm23, [rsp + 368]\n");
      code_generator_emit(generator, "    movdqu xmm24, [rsp + 384]\n");
      code_generator_emit(generator, "    movdqu xmm25, [rsp + 400]\n");
      code_generator_emit(generator, "    movdqu xmm26, [rsp + 416]\n");
      code_generator_emit(generator, "    movdqu xmm27, [rsp + 432]\n");
      code_generator_emit(generator, "    movdqu xmm28, [rsp + 448]\n");
      code_generator_emit(generator, "    movdqu xmm29, [rsp + 464]\n");
      code_generator_emit(generator, "    movdqu xmm30, [rsp + 480]\n");
      code_generator_emit(generator, "    movdqu xmm31, [rsp + 496]\n");
      code_generator_emit(generator, "    add rsp, 512\n");
#else
      code_generator_emit(generator, "    add rsp, 256\n");
#endif
      code_generator_emit(generator, "    pop r15\n");
      code_generator_emit(generator, "    pop r14\n");
      code_generator_emit(generator, "    pop r13\n");
      code_generator_emit(generator, "    pop r12\n");
      code_generator_emit(generator, "    pop r11\n");
      code_generator_emit(generator, "    pop r10\n");
      code_generator_emit(generator, "    pop r9\n");
      code_generator_emit(generator, "    pop r8\n");
      code_generator_emit(generator, "    pop rdi\n");
      code_generator_emit(generator, "    pop rsi\n");
      code_generator_emit(generator, "    pop rdx\n");
      code_generator_emit(generator, "    pop rcx\n");
      code_generator_emit(generator, "    pop rbx\n");
      code_generator_emit(generator, "    pop rax\n");
    }
  }
  }

  for (size_t i = 0; i < promoted_symbol_count; i++) {
    const char *name = promoted_symbols[i].name;
    Symbol *symbol =
        name ? symbol_table_lookup_current_scope(generator->symbol_table, name) : NULL;
    if (!symbol ||
        (symbol->kind != SYMBOL_VARIABLE && symbol->kind != SYMBOL_PARAMETER)) {
      code_generator_set_error(
          generator, "Failed to activate promoted symbol '%s' in IR backend",
          name ? name : "<unknown>");
      break;
    }

    int is_parameter = (symbol->kind == SYMBOL_PARAMETER);
    if (symbol->kind == SYMBOL_PARAMETER) {
      code_generator_load_variable(generator, name);
      if (generator->has_error) {
        break;
      }
    }

    symbol->data.variable.register_id = promoted_symbols[i].reg;
    symbol->data.variable.is_in_register = 1;

    if (is_parameter) {
      code_generator_store_variable(generator, name, "rax");
      if (generator->has_error) {
        break;
      }
    }
  }
  if (generator->has_error) {
    ir_temp_table_destroy(&temp_table);
    ir_local_table_destroy(&local_table);
    ir_symbol_stats_map_destroy(&symbol_stats);
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
      ir_symbol_stats_map_destroy(&symbol_stats);
      symbol_table_exit_scope(generator->symbol_table);
      return 0;
    }

    int promoted_index =
        ir_promoted_symbol_find(promoted_symbols, promoted_symbol_count, local->name);
    if (promoted_index >= 0) {
      symbol->data.variable.register_id = promoted_symbols[promoted_index].reg;
      symbol->data.variable.is_in_register = 1;
      symbol->data.variable.memory_offset = 0;
      continue;
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

  IRTempUseMap temp_use_map = {0};
  for (size_t i = 0; i < ir_function->instruction_count; i++) {
    if (!ir_temp_use_map_record_instruction(&temp_use_map,
                                            &ir_function->instructions[i])) {
      code_generator_set_error(generator,
                               "Out of memory while tracking IR temp uses");
      break;
    }
  }
  if (!generator->has_error) {
    for (size_t i = 0; i < ir_function->instruction_count; i++) {
      const IRInstruction *instruction = &ir_function->instructions[i];
      if (i + 1 < ir_function->instruction_count) {
        const IRInstruction *next = &ir_function->instructions[i + 1];
        if (instruction->op == IR_OP_BINARY && !instruction->is_float &&
            instruction->dest.kind == IR_OPERAND_TEMP &&
            instruction->dest.name && ir_binary_operator_is_comparison(instruction->text) &&
            next->op == IR_OP_BRANCH_ZERO &&
            next->lhs.kind == IR_OPERAND_TEMP && next->lhs.name &&
            strcmp(next->lhs.name, instruction->dest.name) == 0 &&
            ir_temp_use_map_get(&temp_use_map, instruction->dest.name) == 1) {
          if (i + 3 < ir_function->instruction_count &&
              next->text &&
              ir_function->instructions[i + 2].op == IR_OP_JUMP &&
              ir_function->instructions[i + 2].text &&
              ir_function->instructions[i + 3].op == IR_OP_LABEL &&
              ir_function->instructions[i + 3].text &&
              strcmp(next->text, ir_function->instructions[i + 3].text) == 0) {
            const char *false_jump =
                ir_false_jump_for_comparison(instruction->text);
            const char *true_jump = ir_invert_conditional_jump(false_jump);
            if (true_jump) {
              if (!code_generator_emit_ir_compare_branch_with_jump(
                      generator, instruction, true_jump,
                      ir_function->instructions[i + 2].text, &temp_table)) {
                break;
              }
              i += 2;
              continue;
            }
          }

          if (!code_generator_emit_ir_compare_branch_zero(
                  generator, instruction, next, &temp_table)) {
            break;
          }
          i++;
          continue;
        }

        if (instruction->op == IR_OP_BRANCH_ZERO && instruction->text &&
            i + 2 < ir_function->instruction_count && next->text &&
            next->op == IR_OP_JUMP &&
            ir_function->instructions[i + 2].op == IR_OP_LABEL &&
            ir_function->instructions[i + 2].text &&
            strcmp(instruction->text, ir_function->instructions[i + 2].text) ==
                0) {
          if (!code_generator_emit_ir_branch_zero_jump_over_label(
                  generator, instruction, next, &temp_table)) {
            break;
          }
          i++;
          continue;
        }
      }

      if (!code_generator_emit_ir_instruction(generator, instruction,
                                              &temp_table)) {
        break;
      }
    }
  }

  code_generator_emit(generator, "L%s_exit:\n", function_data->name);
  for (size_t i = promoted_symbol_count; i > 0; i--) {
    size_t index = i - 1;
    const char *reg_name = code_generator_get_register_name(promoted_symbols[index].reg);
    if (!reg_name) {
      code_generator_set_error(generator,
                               "Invalid promoted register in function '%s'",
                               function_data->name);
      break;
    }
    code_generator_emit(generator, "    mov %s, [rbp - %d]\n", reg_name,
                        promoted_symbols[index].save_offset);
  }
  code_generator_function_epilogue(generator);

  ir_temp_use_map_destroy(&temp_use_map);
  ir_temp_table_destroy(&temp_table);
  ir_local_table_destroy(&local_table);
  ir_symbol_stats_map_destroy(&symbol_stats);
  symbol_table_exit_scope(generator->symbol_table);

  return generator->has_error ? 0 : 1;
}
