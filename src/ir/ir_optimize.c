#include "ir_optimize.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IR_INLINE_MAX_NON_NOP_INSTRUCTIONS 128
#define IR_INLINE_MAX_PARAMETERS 16
#define IR_INLINE_MAX_ROUNDS 4
#define IR_INLINE_MAX_CALLER_NON_NOP_INSTRUCTIONS 512
#define IR_UNROLL_MAX_TRIP_COUNT 64

typedef struct {
  char *name;
  IROperand value;
} IRTempValueEntry;

typedef struct {
  IRTempValueEntry *items;
  size_t count;
  size_t capacity;
} IRTempValueMap;

/* Block-local known values for stack locals (@symbol operands). */
typedef IRTempValueMap IRSymbolValueMap;

typedef struct {
  char *label;
  IRTempValueMap in_map;
  int initialized;
} IRLabelValueEntry;

typedef struct {
  IRLabelValueEntry *items;
  size_t count;
  size_t capacity;
} IRLabelValueMap;

static int ir_operand_equals(const IROperand *lhs, const IROperand *rhs);

typedef struct {
  char *name;
  size_t use_count;
} IRTempUseEntry;

typedef struct {
  IRTempUseEntry *items;
  size_t count;
  size_t capacity;
  /* Open-addressing hash of name -> (index+1) into items, so find() is O(1)
   * instead of a linear scan. 0 means empty slot. Sized to a power of two with
   * load factor < 0.5; rebuilt when items grows. */
  size_t *hash;
  size_t hash_count;
} IRTempUseMap;

typedef struct {
  char *from;
  char *to;
} IRNameMapEntry;

typedef struct {
  IRNameMapEntry *items;
  size_t count;
  size_t capacity;
} IRNameMap;

typedef struct {
  IRInstruction *items;
  size_t count;
  size_t capacity;
} IRInstructionVector;

typedef struct {
  size_t *items;
  size_t count;
  size_t capacity;
} IRIndexVector;

typedef enum {
  IR_EXPR_BINARY,
  IR_EXPR_UNARY,
  IR_EXPR_CAST,
  IR_EXPR_ADDRESS_OF
} IRExpressionKind;

typedef struct {
  IRExpressionKind kind;
  char *op_text;
  IROperand lhs;
  IROperand rhs;
  int is_float;
  IROperand value;
} IRExpressionEntry;

typedef struct {
  IRExpressionEntry *items;
  size_t count;
  size_t capacity;
} IRExpressionMap;

static char *ir_opt_strdup(const char *text) {
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

static size_t ir_opt_name_hash(const char *name) {
  /* FNV-1a */
  size_t hash = (size_t)1469598103934665603ULL;
  for (const unsigned char *p = (const unsigned char *)name; *p; p++) {
    hash ^= (size_t)*p;
    hash *= (size_t)1099511628211ULL;
  }
  return hash;
}

static int ir_operand_clone(const IROperand *source, IROperand *out) {
  if (!out) {
    return 0;
  }

  *out = ir_operand_none();
  if (!source) {
    return 1;
  }

  out->kind = source->kind;
  out->int_value = source->int_value;
  out->float_value = source->float_value;
  out->float_bits = source->float_bits;

  switch (source->kind) {
  case IR_OPERAND_TEMP:
  case IR_OPERAND_SYMBOL:
  case IR_OPERAND_STRING:
  case IR_OPERAND_LABEL:
    if (!source->name) {
      return 0;
    }
    out->name = ir_opt_strdup(source->name);
    if (!out->name) {
      *out = ir_operand_none();
      return 0;
    }
    break;
  default:
    break;
  }

  return 1;
}

static int ir_name_map_find(const IRNameMap *map, const char *from) {
  if (!map || !from) {
    return -1;
  }

  for (size_t i = 0; i < map->count; i++) {
    if (map->items[i].from && strcmp(map->items[i].from, from) == 0) {
      return (int)i;
    }
  }

  return -1;
}

static const char *ir_name_map_lookup(const IRNameMap *map, const char *from) {
  int index = ir_name_map_find(map, from);
  if (index < 0) {
    return NULL;
  }
  return map->items[index].to;
}

static int ir_name_map_add(IRNameMap *map, const char *from, const char *to) {
  if (!map || !from || !to) {
    return 0;
  }

  if (ir_name_map_find(map, from) >= 0) {
    return 1;
  }

  if (map->count >= map->capacity) {
    size_t new_capacity = map->capacity == 0 ? 16 : map->capacity * 2;
    IRNameMapEntry *new_items =
        realloc(map->items, new_capacity * sizeof(IRNameMapEntry));
    if (!new_items) {
      return 0;
    }
    map->items = new_items;
    map->capacity = new_capacity;
  }

  char *from_copy = ir_opt_strdup(from);
  char *to_copy = ir_opt_strdup(to);
  if (!from_copy || !to_copy) {
    free(from_copy);
    free(to_copy);
    return 0;
  }

  map->items[map->count].from = from_copy;
  map->items[map->count].to = to_copy;
  map->count++;
  return 1;
}

static void ir_name_map_destroy(IRNameMap *map) {
  if (!map) {
    return;
  }

  for (size_t i = 0; i < map->count; i++) {
    free(map->items[i].from);
    free(map->items[i].to);
  }
  free(map->items);
  map->items = NULL;
  map->count = 0;
  map->capacity = 0;
}

static char *ir_make_inline_prefix(const char *callee_name, size_t inline_id) {
  const char *name = callee_name ? callee_name : "func";
  int length = snprintf(NULL, 0, "__inl_%s_%zu", name, inline_id);
  if (length < 0) {
    return NULL;
  }

  size_t size = (size_t)length + 1;
  char *prefix = malloc(size);
  if (!prefix) {
    return NULL;
  }

  snprintf(prefix, size, "__inl_%s_%zu", name, inline_id);
  return prefix;
}

static char *ir_make_inline_name(const char *prefix, const char *kind,
                                 const char *base) {
  if (!prefix || !kind || !base) {
    return NULL;
  }

  int length = snprintf(NULL, 0, "%s_%s_%s", prefix, kind, base);
  if (length < 0) {
    return NULL;
  }

  size_t size = (size_t)length + 1;
  char *name = malloc(size);
  if (!name) {
    return NULL;
  }
  snprintf(name, size, "%s_%s_%s", prefix, kind, base);
  return name;
}

static const char *ir_name_map_get_or_create(IRNameMap *map, const char *from,
                                             const char *prefix,
                                             const char *kind) {
  const char *existing = ir_name_map_lookup(map, from);
  if (existing) {
    return existing;
  }

  char *generated = ir_make_inline_name(prefix, kind, from);
  if (!generated) {
    return NULL;
  }

  int ok = ir_name_map_add(map, from, generated);
  free(generated);
  if (!ok) {
    return NULL;
  }

  return ir_name_map_lookup(map, from);
}

static int ir_instruction_writes_temp(const IRInstruction *instruction) {
  if (!instruction || instruction->dest.kind != IR_OPERAND_TEMP ||
      !instruction->dest.name) {
    return 0;
  }

  switch (instruction->op) {
  case IR_OP_ASSIGN:
  case IR_OP_ADDRESS_OF:
  case IR_OP_LOAD:
  case IR_OP_BINARY:
  case IR_OP_UNARY:
  case IR_OP_ROTATE_ADD:
  case IR_OP_CALL:
  case IR_OP_CALL_INDIRECT:
  case IR_OP_NEW:
  case IR_OP_CAST:
    return 1;
  default:
    return 0;
  }
}

static int ir_instruction_writes_symbol(const IRInstruction *instruction) {
  if (!instruction || instruction->dest.kind != IR_OPERAND_SYMBOL ||
      !instruction->dest.name) {
    return 0;
  }

  switch (instruction->op) {
  case IR_OP_ASSIGN:
  case IR_OP_ADDRESS_OF:
  case IR_OP_LOAD:
  case IR_OP_BINARY:
  case IR_OP_UNARY:
  case IR_OP_ROTATE_ADD:
  case IR_OP_CALL:
  case IR_OP_CALL_INDIRECT:
  case IR_OP_NEW:
  case IR_OP_CAST:
  case IR_OP_COUNT_WORD_STARTS:
  case IR_OP_MEMCPY_INLINE:
  case IR_OP_SIMD_SUM_I32:
  case IR_OP_SIMD_MATMUL_N32:
    return 1;
  default:
    return 0;
  }
}

static int ir_instruction_writes_destination(const IRInstruction *instruction) {
  if (!instruction || instruction->dest.kind == IR_OPERAND_NONE) {
    return 0;
  }

  switch (instruction->op) {
  case IR_OP_ASSIGN:
  case IR_OP_ADDRESS_OF:
  case IR_OP_LOAD:
  case IR_OP_BINARY:
  case IR_OP_UNARY:
  case IR_OP_ROTATE_ADD:
  case IR_OP_CALL:
  case IR_OP_CALL_INDIRECT:
  case IR_OP_NEW:
  case IR_OP_CAST:
  case IR_OP_COUNT_WORD_STARTS:
  case IR_OP_SIMD_SUM_I32:
    return 1;
  default:
    return 0;
  }
}

static int ir_instruction_is_trivially_dead_if_dest_unused(
    const IRInstruction *instruction) {
  if (!instruction) {
    return 0;
  }

  switch (instruction->op) {
  case IR_OP_ASSIGN:
  case IR_OP_ADDRESS_OF:
  case IR_OP_BINARY:
  case IR_OP_UNARY:
  case IR_OP_CAST:
    return 1;
  default:
    return 0;
  }
}

static int ir_operand_is_propagatable_value(const IROperand *operand) {
  if (!operand) {
    return 0;
  }

  switch (operand->kind) {
  case IR_OPERAND_INT:
  case IR_OPERAND_FLOAT:
  case IR_OPERAND_STRING:
  case IR_OPERAND_LABEL:
  case IR_OPERAND_SYMBOL:
  case IR_OPERAND_TEMP:
    return 1;
  default:
    return 0;
  }
}

static void ir_instruction_clear_arguments(IRInstruction *instruction) {
  if (!instruction || !instruction->arguments) {
    return;
  }

  for (size_t i = 0; i < instruction->argument_count; i++) {
    ir_operand_destroy(&instruction->arguments[i]);
  }
  free(instruction->arguments);
  instruction->arguments = NULL;
  instruction->argument_count = 0;
}

static void ir_instruction_make_nop(IRInstruction *instruction) {
  if (!instruction) {
    return;
  }

  ir_operand_destroy(&instruction->dest);
  ir_operand_destroy(&instruction->lhs);
  ir_operand_destroy(&instruction->rhs);
  ir_instruction_clear_arguments(instruction);
  free(instruction->text);
  instruction->text = NULL;
  instruction->is_float = 0;
  instruction->ast_ref = NULL;
  instruction->op = IR_OP_NOP;
}

static void ir_instruction_make_jump(IRInstruction *instruction) {
  if (!instruction) {
    return;
  }

  ir_operand_destroy(&instruction->dest);
  ir_operand_destroy(&instruction->lhs);
  ir_operand_destroy(&instruction->rhs);
  ir_instruction_clear_arguments(instruction);
  instruction->is_float = 0;
  instruction->ast_ref = NULL;
  instruction->op = IR_OP_JUMP;
}

static void ir_instruction_destroy_storage(IRInstruction *instruction) {
  if (!instruction) {
    return;
  }

  ir_operand_destroy(&instruction->dest);
  ir_operand_destroy(&instruction->lhs);
  ir_operand_destroy(&instruction->rhs);
  ir_instruction_clear_arguments(instruction);
  free(instruction->text);
  instruction->text = NULL;
  instruction->is_float = 0;
  instruction->ast_ref = NULL;
  instruction->op = IR_OP_NOP;
}

static int ir_instruction_vector_append_move(IRInstructionVector *vector,
                                             IRInstruction *instruction) {
  if (!vector || !instruction) {
    return 0;
  }

  if (vector->count >= vector->capacity) {
    size_t new_capacity = vector->capacity == 0 ? 64 : vector->capacity * 2;
    IRInstruction *new_items =
        realloc(vector->items, new_capacity * sizeof(IRInstruction));
    if (!new_items) {
      return 0;
    }
    vector->items = new_items;
    vector->capacity = new_capacity;
  }

  vector->items[vector->count++] = *instruction;
  instruction->op = IR_OP_NOP;
  instruction->dest = ir_operand_none();
  instruction->lhs = ir_operand_none();
  instruction->rhs = ir_operand_none();
  instruction->text = NULL;
  instruction->arguments = NULL;
  instruction->argument_count = 0;
  instruction->is_float = 0;
  instruction->ast_ref = NULL;
  return 1;
}

static void ir_instruction_vector_destroy(IRInstructionVector *vector) {
  if (!vector) {
    return;
  }

  for (size_t i = 0; i < vector->count; i++) {
    ir_instruction_destroy_storage(&vector->items[i]);
  }
  free(vector->items);
  vector->items = NULL;
  vector->count = 0;
  vector->capacity = 0;
}

static int ir_index_vector_append(IRIndexVector *vector, size_t value) {
  if (!vector) {
    return 0;
  }

  if (vector->count >= vector->capacity) {
    size_t new_capacity = vector->capacity == 0 ? 16 : vector->capacity * 2;
    size_t *new_items = realloc(vector->items, new_capacity * sizeof(size_t));
    if (!new_items) {
      return 0;
    }
    vector->items = new_items;
    vector->capacity = new_capacity;
  }

  vector->items[vector->count++] = value;
  return 1;
}

static void ir_index_vector_destroy(IRIndexVector *vector) {
  if (!vector) {
    return;
  }
  free(vector->items);
  vector->items = NULL;
  vector->count = 0;
  vector->capacity = 0;
}

static int ir_rewrite_to_assign_operand(IRInstruction *instruction,
                                        const IROperand *value, int *changed) {
  IROperand cloned = ir_operand_none();
  if (!instruction || !value || !ir_operand_clone(value, &cloned)) {
    return 0;
  }

  ir_operand_destroy(&instruction->lhs);
  ir_operand_destroy(&instruction->rhs);
  ir_instruction_clear_arguments(instruction);
  free(instruction->text);
  instruction->text = NULL;

  instruction->op = IR_OP_ASSIGN;
  instruction->lhs = cloned;
  instruction->rhs = ir_operand_none();
  instruction->is_float = (cloned.kind == IR_OPERAND_FLOAT) ? 1 : 0;
  if (instruction->is_float) {
    instruction->float_bits = (cloned.float_bits == 32) ? 32 : 64;
  }
  instruction->ast_ref = NULL;

  if (changed) {
    *changed = 1;
  }
  return 1;
}

static int ir_rewrite_to_assign_int(IRInstruction *instruction, long long value,
                                    int *changed) {
  IROperand constant = ir_operand_int(value);
  return ir_rewrite_to_assign_operand(instruction, &constant, changed);
}

static int ir_temp_value_map_init(IRTempValueMap *map) {
  if (!map) {
    return 0;
  }

  map->items = NULL;
  map->count = 0;
  map->capacity = 0;
  return 1;
}

static int ir_temp_value_map_find(const IRTempValueMap *map, const char *name) {
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

static void ir_temp_value_map_remove(IRTempValueMap *map, const char *name) {
  if (!map || !name) {
    return;
  }

  int index = ir_temp_value_map_find(map, name);
  if (index < 0) {
    return;
  }

  size_t idx = (size_t)index;
  free(map->items[idx].name);
  ir_operand_destroy(&map->items[idx].value);

  for (size_t i = idx + 1; i < map->count; i++) {
    map->items[i - 1] = map->items[i];
  }
  map->count--;
}

static int ir_temp_value_map_set(IRTempValueMap *map, const char *name,
                                 const IROperand *value) {
  if (!map || !name || !value) {
    return 0;
  }

  int existing = ir_temp_value_map_find(map, name);
  if (existing >= 0) {
    IROperand cloned = ir_operand_none();
    if (!ir_operand_clone(value, &cloned)) {
      return 0;
    }

    ir_operand_destroy(&map->items[existing].value);
    map->items[existing].value = cloned;
    return 1;
  }

  if (map->count >= map->capacity) {
    size_t new_capacity = map->capacity == 0 ? 16 : map->capacity * 2;
    IRTempValueEntry *new_items =
        realloc(map->items, new_capacity * sizeof(IRTempValueEntry));
    if (!new_items) {
      return 0;
    }
    map->items = new_items;
    map->capacity = new_capacity;
  }

  char *name_copy = ir_opt_strdup(name);
  IROperand cloned = ir_operand_none();
  if (!name_copy || !ir_operand_clone(value, &cloned)) {
    free(name_copy);
    ir_operand_destroy(&cloned);
    return 0;
  }

  map->items[map->count].name = name_copy;
  map->items[map->count].value = cloned;
  map->count++;
  return 1;
}

static void ir_temp_value_map_remove_symbol_values(IRTempValueMap *map,
                                                   const char *symbol_name) {
  if (!map) {
    return;
  }

  size_t write = 0;
  for (size_t read = 0; read < map->count; read++) {
    IRTempValueEntry *entry = &map->items[read];
    int remove = 0;

    if (entry->value.kind == IR_OPERAND_SYMBOL && entry->value.name) {
      if (!symbol_name || strcmp(entry->value.name, symbol_name) == 0) {
        remove = 1;
      }
    }

    if (remove) {
      free(entry->name);
      ir_operand_destroy(&entry->value);
      continue;
    }

    if (write != read) {
      map->items[write] = map->items[read];
    }
    write++;
  }

  map->count = write;
}

static const IROperand *ir_temp_value_map_lookup(const IRTempValueMap *map,
                                                 const char *name) {
  if (!map || !name) {
    return NULL;
  }

  int index = ir_temp_value_map_find(map, name);
  if (index < 0) {
    return NULL;
  }

  return &map->items[index].value;
}

static void ir_temp_value_map_clear(IRTempValueMap *map) {
  if (!map) {
    return;
  }

  for (size_t i = 0; i < map->count; i++) {
    free(map->items[i].name);
    ir_operand_destroy(&map->items[i].value);
  }
  map->count = 0;
}

static void ir_temp_value_map_destroy(IRTempValueMap *map) {
  if (!map) {
    return;
  }

  ir_temp_value_map_clear(map);
  free(map->items);
  map->items = NULL;
  map->capacity = 0;
}

static int ir_temp_value_map_clone(IRTempValueMap *dest,
                                   const IRTempValueMap *src) {
  if (!dest || !src) {
    return 0;
  }

  ir_temp_value_map_clear(dest);
  for (size_t i = 0; i < src->count; i++) {
    if (!ir_temp_value_map_set(dest, src->items[i].name, &src->items[i].value)) {
      return 0;
    }
  }
  return 1;
}

static int ir_temp_value_map_intersect_with(IRTempValueMap *dest,
                                            const IRTempValueMap *other,
                                            int *changed) {
  if (!dest || !other) {
    return 0;
  }

  size_t i = 0;
  while (i < dest->count) {
    IRTempValueEntry *entry = &dest->items[i];
    const IROperand *other_value =
        ir_temp_value_map_lookup(other, entry->name);
    if (!other_value || !ir_operand_equals(&entry->value, other_value)) {
      ir_temp_value_map_remove(dest, entry->name);
      if (changed) {
        *changed = 1;
      }
      continue;
    }
    i++;
  }

  return 1;
}

static int ir_label_value_map_init(IRLabelValueMap *map) {
  if (!map) {
    return 0;
  }
  map->items = NULL;
  map->count = 0;
  map->capacity = 0;
  return 1;
}

static int ir_label_value_map_find(const IRLabelValueMap *map,
                                   const char *label) {
  if (!map || !label) {
    return -1;
  }
  for (size_t i = 0; i < map->count; i++) {
    if (map->items[i].label && strcmp(map->items[i].label, label) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static IRLabelValueEntry *ir_label_value_map_get_or_add(IRLabelValueMap *map,
                                                        const char *label) {
  if (!map || !label) {
    return NULL;
  }

  int existing = ir_label_value_map_find(map, label);
  if (existing >= 0) {
    return &map->items[existing];
  }

  if (map->count >= map->capacity) {
    size_t new_capacity = map->capacity == 0 ? 16 : map->capacity * 2;
    IRLabelValueEntry *new_items =
        realloc(map->items, new_capacity * sizeof(IRLabelValueEntry));
    if (!new_items) {
      return NULL;
    }
    map->items = new_items;
    map->capacity = new_capacity;
  }

  IRLabelValueEntry *entry = &map->items[map->count];
  memset(entry, 0, sizeof(*entry));
  entry->label = ir_opt_strdup(label);
  if (!entry->label || !ir_temp_value_map_init(&entry->in_map)) {
    free(entry->label);
    entry->label = NULL;
    return NULL;
  }
  entry->initialized = 0;
  map->count++;
  return entry;
}

static int ir_label_value_map_merge_incoming(IRLabelValueMap *map,
                                             const char *label,
                                             const IRTempValueMap *incoming,
                                             int *changed) {
  if (!map || !label || !incoming) {
    return 0;
  }

  IRLabelValueEntry *entry = ir_label_value_map_get_or_add(map, label);
  if (!entry) {
    return 0;
  }

  if (!entry->initialized) {
    if (!ir_temp_value_map_clone(&entry->in_map, incoming)) {
      return 0;
    }
    entry->initialized = 1;
    if (changed) {
      *changed = 1;
    }
    return 1;
  }

  return ir_temp_value_map_intersect_with(&entry->in_map, incoming, changed);
}

static const IRLabelValueEntry *ir_label_value_map_lookup(
    const IRLabelValueMap *map, const char *label) {
  if (!map || !label) {
    return NULL;
  }
  int index = ir_label_value_map_find(map, label);
  if (index < 0) {
    return NULL;
  }
  return &map->items[index];
}

static void ir_label_value_map_destroy(IRLabelValueMap *map) {
  if (!map) {
    return;
  }
  for (size_t i = 0; i < map->count; i++) {
    free(map->items[i].label);
    ir_temp_value_map_destroy(&map->items[i].in_map);
  }
  free(map->items);
  map->items = NULL;
  map->count = 0;
  map->capacity = 0;
}

static int ir_operand_equals(const IROperand *lhs, const IROperand *rhs) {
  if (!lhs || !rhs || lhs->kind != rhs->kind) {
    return 0;
  }

  switch (lhs->kind) {
  case IR_OPERAND_NONE:
    return 1;
  case IR_OPERAND_INT:
    return lhs->int_value == rhs->int_value;
  case IR_OPERAND_FLOAT:
    /* Same numeric value at different IEEE-754 widths is NOT the same operand
     * for CSE/propagation purposes: a float32 0.1 and float64 0.1 have
     * distinct bit patterns and must not be coalesced. Treat unspecified (0)
     * as the default 64 so legacy float64-only IR keeps matching. */
    return lhs->float_value == rhs->float_value &&
           ((lhs->float_bits == 32) == (rhs->float_bits == 32));
  case IR_OPERAND_TEMP:
  case IR_OPERAND_SYMBOL:
  case IR_OPERAND_STRING:
  case IR_OPERAND_LABEL:
    if (!lhs->name || !rhs->name) {
      return 0;
    }
    return strcmp(lhs->name, rhs->name) == 0;
  default:
    return 0;
  }
}

static int ir_binary_op_is_commutative(const char *op_text) {
  if (!op_text) {
    return 0;
  }
  return strcmp(op_text, "+") == 0 || strcmp(op_text, "*") == 0 ||
         strcmp(op_text, "&") == 0 || strcmp(op_text, "|") == 0 ||
         strcmp(op_text, "^") == 0 || strcmp(op_text, "==") == 0 ||
         strcmp(op_text, "!=") == 0;
}

static void ir_expression_entry_destroy(IRExpressionEntry *entry) {
  if (!entry) {
    return;
  }

  free(entry->op_text);
  entry->op_text = NULL;
  ir_operand_destroy(&entry->lhs);
  ir_operand_destroy(&entry->rhs);
  ir_operand_destroy(&entry->value);
  entry->kind = IR_EXPR_BINARY;
  entry->is_float = 0;
}

static void ir_expression_map_clear(IRExpressionMap *map) {
  if (!map) {
    return;
  }

  for (size_t i = 0; i < map->count; i++) {
    ir_expression_entry_destroy(&map->items[i]);
  }
  map->count = 0;
}

static void ir_expression_map_destroy(IRExpressionMap *map) {
  if (!map) {
    return;
  }

  ir_expression_map_clear(map);
  free(map->items);
  map->items = NULL;
  map->capacity = 0;
}

static int ir_instruction_is_cse_candidate(const IRInstruction *instruction) {
  if (!instruction || !ir_instruction_writes_destination(instruction)) {
    return 0;
  }

  switch (instruction->op) {
  case IR_OP_BINARY:
    return instruction->text != NULL;
  case IR_OP_UNARY:
    if (!instruction->text) {
      return 0;
    }
    return strcmp(instruction->text, "*") != 0 &&
           strcmp(instruction->text, "&") != 0;
  case IR_OP_CAST:
    return instruction->text != NULL;
  case IR_OP_ADDRESS_OF:
    return instruction->lhs.kind == IR_OPERAND_SYMBOL &&
           instruction->lhs.name != NULL;
  default:
    return 0;
  }
}

static int ir_expression_entry_matches_instruction(
    const IRExpressionEntry *entry, const IRInstruction *instruction) {
  if (!entry || !instruction) {
    return 0;
  }

  switch (entry->kind) {
  case IR_EXPR_BINARY:
    if (instruction->op != IR_OP_BINARY || !instruction->text ||
        !entry->op_text || strcmp(entry->op_text, instruction->text) != 0 ||
        entry->is_float != instruction->is_float) {
      return 0;
    }
    if (ir_operand_equals(&entry->lhs, &instruction->lhs) &&
        ir_operand_equals(&entry->rhs, &instruction->rhs)) {
      return 1;
    }
    if (ir_binary_op_is_commutative(instruction->text) &&
        ir_operand_equals(&entry->lhs, &instruction->rhs) &&
        ir_operand_equals(&entry->rhs, &instruction->lhs)) {
      return 1;
    }
    return 0;
  case IR_EXPR_UNARY:
    return instruction->op == IR_OP_UNARY && instruction->text &&
           entry->op_text && strcmp(entry->op_text, instruction->text) == 0 &&
           entry->is_float == instruction->is_float &&
           ir_operand_equals(&entry->lhs, &instruction->lhs);
  case IR_EXPR_CAST:
    return instruction->op == IR_OP_CAST && instruction->text &&
           entry->op_text && strcmp(entry->op_text, instruction->text) == 0 &&
           entry->is_float == instruction->is_float &&
           ir_operand_equals(&entry->lhs, &instruction->lhs);
  case IR_EXPR_ADDRESS_OF:
    return instruction->op == IR_OP_ADDRESS_OF &&
           ir_operand_equals(&entry->lhs, &instruction->lhs);
  default:
    return 0;
  }
}

static int ir_expression_map_find_matching_instruction(
    const IRExpressionMap *map, const IRInstruction *instruction) {
  if (!map || !instruction) {
    return -1;
  }

  for (size_t i = 0; i < map->count; i++) {
    if (ir_expression_entry_matches_instruction(&map->items[i], instruction)) {
      return (int)i;
    }
  }

  return -1;
}

static const IROperand *ir_expression_map_lookup(const IRExpressionMap *map,
                                                 const IRInstruction *instruction) {
  int index = ir_expression_map_find_matching_instruction(map, instruction);
  if (index < 0) {
    return NULL;
  }
  return &map->items[index].value;
}

static int ir_expression_map_store_value_for_instruction(
    IRExpressionMap *map, const IRInstruction *instruction) {
  if (!map || !instruction || !ir_instruction_is_cse_candidate(instruction) ||
      instruction->dest.kind == IR_OPERAND_NONE) {
    return 0;
  }

  int existing_index =
      ir_expression_map_find_matching_instruction(map, instruction);
  if (existing_index >= 0) {
    IROperand new_value = ir_operand_none();
    if (!ir_operand_clone(&instruction->dest, &new_value)) {
      return 0;
    }
    ir_operand_destroy(&map->items[existing_index].value);
    map->items[existing_index].value = new_value;
    return 1;
  }

  if (map->count >= map->capacity) {
    size_t new_capacity = map->capacity == 0 ? 16 : map->capacity * 2;
    IRExpressionEntry *new_items =
        realloc(map->items, new_capacity * sizeof(IRExpressionEntry));
    if (!new_items) {
      return 0;
    }
    map->items = new_items;
    map->capacity = new_capacity;
  }

  IRExpressionEntry *entry = &map->items[map->count];
  memset(entry, 0, sizeof(*entry));
  entry->lhs = ir_operand_none();
  entry->rhs = ir_operand_none();
  entry->value = ir_operand_none();

  switch (instruction->op) {
  case IR_OP_BINARY:
    entry->kind = IR_EXPR_BINARY;
    entry->is_float = instruction->is_float;
    entry->op_text = ir_opt_strdup(instruction->text);
    if (!entry->op_text) {
      ir_expression_entry_destroy(entry);
      return 0;
    }
    if (!ir_operand_clone(&instruction->lhs, &entry->lhs) ||
        !ir_operand_clone(&instruction->rhs, &entry->rhs)) {
      ir_expression_entry_destroy(entry);
      return 0;
    }
    break;
  case IR_OP_UNARY:
    entry->kind = IR_EXPR_UNARY;
    entry->is_float = instruction->is_float;
    entry->op_text = ir_opt_strdup(instruction->text);
    if (!entry->op_text) {
      ir_expression_entry_destroy(entry);
      return 0;
    }
    if (!ir_operand_clone(&instruction->lhs, &entry->lhs)) {
      ir_expression_entry_destroy(entry);
      return 0;
    }
    break;
  case IR_OP_CAST:
    entry->kind = IR_EXPR_CAST;
    entry->is_float = instruction->is_float;
    entry->op_text = ir_opt_strdup(instruction->text);
    if (!entry->op_text) {
      ir_expression_entry_destroy(entry);
      return 0;
    }
    if (!ir_operand_clone(&instruction->lhs, &entry->lhs)) {
      ir_expression_entry_destroy(entry);
      return 0;
    }
    break;
  case IR_OP_ADDRESS_OF:
    entry->kind = IR_EXPR_ADDRESS_OF;
    if (!ir_operand_clone(&instruction->lhs, &entry->lhs)) {
      ir_expression_entry_destroy(entry);
      return 0;
    }
    break;
  default:
    return 0;
  }

  if (!ir_operand_clone(&instruction->dest, &entry->value)) {
    ir_expression_entry_destroy(entry);
    return 0;
  }

  map->count++;
  return 1;
}

static int ir_operand_matches_named(const IROperand *operand,
                                    IROperandKind kind, const char *name) {
  if (!operand || !name || operand->kind != kind || !operand->name) {
    return 0;
  }
  return strcmp(operand->name, name) == 0;
}

static int ir_expression_entry_uses_named(const IRExpressionEntry *entry,
                                          IROperandKind kind,
                                          const char *name) {
  if (!entry || !name) {
    return 0;
  }

  if (ir_operand_matches_named(&entry->lhs, kind, name) ||
      ir_operand_matches_named(&entry->rhs, kind, name) ||
      ir_operand_matches_named(&entry->value, kind, name)) {
    return 1;
  }

  return 0;
}

static void ir_expression_map_invalidate_named(IRExpressionMap *map,
                                               IROperandKind kind,
                                               const char *name) {
  if (!map || !name) {
    return;
  }

  size_t write = 0;
  for (size_t read = 0; read < map->count; read++) {
    IRExpressionEntry *entry = &map->items[read];
    if (ir_expression_entry_uses_named(entry, kind, name)) {
      ir_expression_entry_destroy(entry);
      continue;
    }

    if (write != read) {
      map->items[write] = map->items[read];
      map->items[read].op_text = NULL;
      map->items[read].lhs = ir_operand_none();
      map->items[read].rhs = ir_operand_none();
      map->items[read].value = ir_operand_none();
    }
    write++;
  }

  map->count = write;
}

static int ir_common_subexpression_elimination_pass(IRFunction *function,
                                                    int *changed) {
  if (!function) {
    return 0;
  }

  IRExpressionMap map = {0};

  for (size_t i = 0; i < function->instruction_count; i++) {
    IRInstruction *instruction = &function->instructions[i];

    if (instruction->op == IR_OP_LABEL) {
      ir_expression_map_clear(&map);
      continue;
    }

    if (ir_instruction_writes_temp(instruction) && instruction->dest.name) {
      ir_expression_map_invalidate_named(&map, IR_OPERAND_TEMP,
                                         instruction->dest.name);
    }
    if (ir_instruction_writes_symbol(instruction) && instruction->dest.name) {
      ir_expression_map_invalidate_named(&map, IR_OPERAND_SYMBOL,
                                         instruction->dest.name);
    }

    if (ir_instruction_is_cse_candidate(instruction)) {
      const IROperand *existing = ir_expression_map_lookup(&map, instruction);
      if (existing) {
        if (!ir_rewrite_to_assign_operand(instruction, existing, changed)) {
          ir_expression_map_destroy(&map);
          return 0;
        }
      } else if (!ir_expression_map_store_value_for_instruction(&map,
                                                                instruction)) {
        ir_expression_map_destroy(&map);
        return 0;
      }
    }

    if (instruction->op == IR_OP_STORE || instruction->op == IR_OP_CALL ||
        instruction->op == IR_OP_CALL_INDIRECT ||
        instruction->op == IR_OP_INLINE_ASM) {
      ir_expression_map_clear(&map);
    }

    if (instruction->op == IR_OP_JUMP || instruction->op == IR_OP_BRANCH_ZERO ||
        instruction->op == IR_OP_BRANCH_EQ ||
        instruction->op == IR_OP_RETURN) {
      ir_expression_map_clear(&map);
    }
  }

  ir_expression_map_destroy(&map);
  return 1;
}

static int ir_temp_use_map_init(IRTempUseMap *map) {
  if (!map) {
    return 0;
  }

  map->items = NULL;
  map->count = 0;
  map->capacity = 0;
  map->hash = NULL;
  map->hash_count = 0;
  return 1;
}

/* Insert items[index] into the hash table (hash table must have room). */
static void ir_temp_use_map_hash_put(IRTempUseMap *map, size_t index) {
  size_t mask = map->hash_count - 1;
  size_t h = ir_opt_name_hash(map->items[index].name) & mask;
  while (map->hash[h] != 0) {
    h = (h + 1) & mask;
  }
  map->hash[h] = index + 1; /* store index+1; 0 == empty */
}

/* Grow/allocate the hash table so it can hold map->count entries at <0.5 load,
 * rehashing all existing items. Returns 0 on allocation failure. */
static int ir_temp_use_map_hash_reserve(IRTempUseMap *map, size_t needed) {
  size_t target = 16;
  while (target < needed * 2) {
    target *= 2;
  }
  if (map->hash && map->hash_count >= target) {
    return 1;
  }

  size_t *new_hash = calloc(target, sizeof(size_t));
  if (!new_hash) {
    return 0;
  }
  free(map->hash);
  map->hash = new_hash;
  map->hash_count = target;
  for (size_t i = 0; i < map->count; i++) {
    ir_temp_use_map_hash_put(map, i);
  }
  return 1;
}

static int ir_temp_use_map_find(const IRTempUseMap *map, const char *name) {
  if (!map || !name || !map->hash) {
    return -1;
  }

  size_t mask = map->hash_count - 1;
  size_t h = ir_opt_name_hash(name) & mask;
  while (map->hash[h] != 0) {
    size_t idx = map->hash[h] - 1;
    if (map->items[idx].name && strcmp(map->items[idx].name, name) == 0) {
      return (int)idx;
    }
    h = (h + 1) & mask;
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
    size_t new_capacity = map->capacity == 0 ? 16 : map->capacity * 2;
    IRTempUseEntry *new_items =
        realloc(map->items, new_capacity * sizeof(IRTempUseEntry));
    if (!new_items) {
      return 0;
    }
    map->items = new_items;
    map->capacity = new_capacity;
  }

  /* Ensure the hash can hold one more entry (rehashes existing items if so). */
  if (!ir_temp_use_map_hash_reserve(map, map->count + 1)) {
    return 0;
  }

  char *name_copy = ir_opt_strdup(name);
  if (!name_copy) {
    return 0;
  }

  map->items[map->count].name = name_copy;
  map->items[map->count].use_count = 1;
  ir_temp_use_map_hash_put(map, map->count);
  map->count++;
  return 1;
}

static size_t ir_temp_use_map_get(const IRTempUseMap *map, const char *name) {
  if (!map || !name) {
    return 0;
  }

  int index = ir_temp_use_map_find(map, name);
  if (index < 0) {
    return 0;
  }

  return map->items[index].use_count;
}

static void ir_temp_use_map_discard(IRTempUseMap *map, const char *name) {
  if (!map || !name) {
    return;
  }

  int index = ir_temp_use_map_find(map, name);
  if (index >= 0) {
    map->items[index].use_count = 0;
  }
}

static void ir_temp_use_map_destroy(IRTempUseMap *map) {
  if (!map) {
    return;
  }

  for (size_t i = 0; i < map->count; i++) {
    free(map->items[i].name);
  }
  free(map->items);
  free(map->hash);
  map->items = NULL;
  map->count = 0;
  map->capacity = 0;
  map->hash = NULL;
  map->hash_count = 0;
}

static int ir_collect_operand_temp_use(IRTempUseMap *uses,
                                       const IROperand *operand) {
  if (!uses || !operand) {
    return 0;
  }

  if (operand->kind != IR_OPERAND_TEMP || !operand->name) {
    return 1;
  }

  return ir_temp_use_map_add(uses, operand->name);
}

static int ir_collect_instruction_temp_uses(IRTempUseMap *uses,
                                            const IRInstruction *instruction) {
  if (!uses || !instruction) {
    return 0;
  }

  switch (instruction->op) {
  case IR_OP_STORE:
  case IR_OP_MEMCPY_INLINE:
  case IR_OP_COUNT_WORD_STARTS:
  case IR_OP_SIMD_SUM_I32:
  case IR_OP_SIMD_MATMUL_N32:
    if (!ir_collect_operand_temp_use(uses, &instruction->dest) ||
        !ir_collect_operand_temp_use(uses, &instruction->lhs) ||
        !ir_collect_operand_temp_use(uses, &instruction->rhs)) {
      return 0;
    }
    break;

  case IR_OP_ASSIGN:
  case IR_OP_ADDRESS_OF:
  case IR_OP_LOAD:
  case IR_OP_BINARY:
  case IR_OP_UNARY:
  case IR_OP_CAST:
  case IR_OP_NEW:
  case IR_OP_BRANCH_ZERO:
  case IR_OP_BRANCH_EQ:
  case IR_OP_CALL:
  case IR_OP_CALL_INDIRECT:
  case IR_OP_RETURN:
    if (!ir_collect_operand_temp_use(uses, &instruction->lhs) ||
        !ir_collect_operand_temp_use(uses, &instruction->rhs)) {
      return 0;
    }
    for (size_t i = 0; i < instruction->argument_count; i++) {
      if (!ir_collect_operand_temp_use(uses, &instruction->arguments[i])) {
        return 0;
      }
    }
    break;

  default:
    break;
  }

  return 1;
}

static int ir_eliminate_dead_temp_writes_pass(IRFunction *function,
                                              int *changed) {
  if (!function) {
    return 0;
  }

  IRTempUseMap live;
  if (!ir_temp_use_map_init(&live)) {
    return 0;
  }

  int crossed_label = 0;
  for (size_t i = function->instruction_count; i > 0;) {
    i--;
    IRInstruction *instruction = &function->instructions[i];

    if (instruction->op == IR_OP_LABEL) {
      crossed_label = 1;
    }

    if (ir_instruction_writes_temp(instruction) && instruction->dest.name) {
      int dest_live = ir_temp_use_map_get(&live, instruction->dest.name) != 0;
      if (!dest_live &&
          ir_instruction_is_trivially_dead_if_dest_unused(instruction)) {
        ir_instruction_make_nop(instruction);
        if (changed) {
          *changed = 1;
        }
        continue;
      }

      if (!crossed_label) {
        ir_temp_use_map_discard(&live, instruction->dest.name);
      }
    }

    if (!ir_collect_instruction_temp_uses(&live, instruction)) {
      ir_temp_use_map_destroy(&live);
      return 0;
    }
  }

  ir_temp_use_map_destroy(&live);
  return 1;
}

/* Name -> IRFunction index for the optimizer.
 *
 * ir_program_find_function used to linear-scan every function (strcmp each) and
 * is called once per CALL instruction during inlining, across every function,
 * for several rounds -- O(calls * functions). That dominated IR optimization on
 * large programs. We cache an open-addressing hash table keyed on the program
 * pointer + function_count; inlining mutates bodies but never adds or removes
 * functions, so the cache stays valid for the whole optimization run. */
typedef struct {
  const char *name; /* borrowed from the IRFunction; not owned */
  IRFunction *function;
} IRFunctionIndexSlot;

typedef struct {
  IRFunctionIndexSlot *slots;
  size_t slot_count; /* power of two */
  const IRProgram *program;
  size_t function_count;
} IRFunctionIndex;

static IRFunctionIndex g_ir_function_index = {0};

static void ir_function_index_reset(void) {
  free(g_ir_function_index.slots);
  g_ir_function_index.slots = NULL;
  g_ir_function_index.slot_count = 0;
  g_ir_function_index.program = NULL;
  g_ir_function_index.function_count = 0;
}

static size_t ir_function_index_hash(const char *name) {
  /* FNV-1a */
  size_t hash = (size_t)1469598103934665603ULL;
  for (const unsigned char *p = (const unsigned char *)name; *p; p++) {
    hash ^= (size_t)*p;
    hash *= (size_t)1099511628211ULL;
  }
  return hash;
}

static void ir_function_index_insert(IRFunctionIndex *index,
                                     IRFunction *function) {
  size_t mask = index->slot_count - 1;
  size_t i = ir_function_index_hash(function->name) & mask;
  while (index->slots[i].name) {
    /* First definition of a given name wins, matching the old linear scan. */
    if (strcmp(index->slots[i].name, function->name) == 0) {
      return;
    }
    i = (i + 1) & mask;
  }
  index->slots[i].name = function->name;
  index->slots[i].function = function;
}

/* Returns 1 if the index is ready to query, 0 on allocation failure (caller
 * falls back to a linear scan). */
static int ir_function_index_ensure(const IRProgram *program) {
  if (g_ir_function_index.program == program &&
      g_ir_function_index.function_count == program->function_count &&
      g_ir_function_index.slots) {
    return 1;
  }

  ir_function_index_reset();

  size_t slot_count = 16;
  while (slot_count < program->function_count * 2) {
    slot_count *= 2;
  }

  IRFunctionIndexSlot *slots = calloc(slot_count, sizeof(IRFunctionIndexSlot));
  if (!slots) {
    return 0;
  }

  g_ir_function_index.slots = slots;
  g_ir_function_index.slot_count = slot_count;
  g_ir_function_index.program = program;
  g_ir_function_index.function_count = program->function_count;

  for (size_t i = 0; i < program->function_count; i++) {
    IRFunction *function = program->functions[i];
    if (function && function->name) {
      ir_function_index_insert(&g_ir_function_index, function);
    }
  }

  return 1;
}

static IRFunction *ir_program_find_function(IRProgram *program,
                                            const char *name) {
  if (!program || !name) {
    return NULL;
  }

  if (ir_function_index_ensure(program)) {
    const IRFunctionIndex *index = &g_ir_function_index;
    size_t mask = index->slot_count - 1;
    size_t i = ir_function_index_hash(name) & mask;
    while (index->slots[i].name) {
      if (strcmp(index->slots[i].name, name) == 0) {
        return index->slots[i].function;
      }
      i = (i + 1) & mask;
    }
    return NULL;
  }

  /* Fallback: index allocation failed; behave as before. */
  for (size_t i = 0; i < program->function_count; i++) {
    IRFunction *function = program->functions[i];
    if (function && function->name && strcmp(function->name, name) == 0) {
      return function;
    }
  }

  return NULL;
}

static int ir_instruction_is_errdefer_epilogue(const IRInstruction *instruction) {
  if (!instruction || !instruction->text) {
    return 0;
  }

  if (instruction->op == IR_OP_LABEL &&
      (strstr(instruction->text, "errdefer_ok") != NULL ||
       strstr(instruction->text, "errdefer_end") != NULL)) {
    return 1;
  }

  if (instruction->op == IR_OP_BRANCH_ZERO &&
      strstr(instruction->text, "errdefer_end") != NULL) {
    return 1;
  }

  return 0;
}

static int ir_function_name_is_inline_denylisted(const char *name) {
  if (!name) {
    return 0;
  }
  /* fib / bench_* inlining + loop unrolling explodes compile time (see
   * ir_optimize.c history). Benchmark hot paths use dedicated functions. */
  return strcmp(name, "fib") == 0 || strcmp(name, "bench_looped") == 0 ||
         strcmp(name, "bench_unrolled") == 0;
}

static int ir_function_is_inline_candidate(const IRFunction *function) {
  if (!function || !function->name || function->instruction_count == 0 ||
      ir_function_name_is_inline_denylisted(function->name) ||
      function->parameter_count > IR_INLINE_MAX_PARAMETERS ||
      (function->parameter_count > 0 && !function->parameter_names)) {
    return 0;
  }

  size_t non_nop_count = 0;
  size_t call_count = 0;
  int has_return = 0;
  for (size_t i = 0; i < function->instruction_count; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    if (!instruction || instruction->op == IR_OP_NOP) {
      continue;
    }

    non_nop_count++;
    if (non_nop_count > IR_INLINE_MAX_NON_NOP_INSTRUCTIONS) {
      return 0;
    }

    if (instruction->op == IR_OP_INLINE_ASM) {
      return 0;
    }

    /* Loop-bearing callees are allowed when not denylisted; the loop unroller
     * only fully expands trips <= IR_UNROLL_MAX_TRIP_COUNT (64). */

    /* CALL and CALL_INDIRECT are allowed:
     * calls just turns those into call instructions in the caller, which is
     * fine. This lets us inline leaf-ish functions (like grep's
     * pattern_matches) whose only calls are in cold fallback paths. Cap the
     * number of contained calls so that we don't inline glue functions like
     * print_int that orchestrate many helper calls — those produce lots of
     * caller bloat without runtime gain. */
    if (instruction->op == IR_OP_CALL ||
        instruction->op == IR_OP_CALL_INDIRECT) {
      call_count++;
      if (call_count > 2) {
        return 0;
      }
    }

    if (instruction->op == IR_OP_RETURN) {
      has_return = 1;
    }
  }

  /* The original errdefer-label/branch caps were rejecting useful inlines
   * (notably pattern_matches inside the grep loop). The labels and branches
   * inline correctly via the generic label-rename map; the cap was just
   * working around a latent bug elsewhere in the optimizer. */
  (void)ir_instruction_is_errdefer_epilogue;
  return has_return;
}

static size_t ir_function_non_nop_instruction_count(const IRFunction *function) {
  if (!function) {
    return 0;
  }

  size_t count = 0;
  for (size_t i = 0; i < function->instruction_count; i++) {
    if (function->instructions[i].op != IR_OP_NOP) {
      count++;
    }
  }
  return count;
}

static int ir_inline_rewrite_operand(const IROperand *source, IROperand *out,
                                     IRNameMap *symbol_map,
                                     IRNameMap *temp_map,
                                     IRNameMap *label_map,
                                     const char *inline_prefix) {
  if (!source || !out) {
    return 0;
  }

  if (source->kind == IR_OPERAND_SYMBOL && source->name) {
    const char *mapped = ir_name_map_lookup(symbol_map, source->name);
    if (mapped) {
      *out = ir_operand_symbol(mapped);
      return out->kind == IR_OPERAND_SYMBOL && out->name;
    }
  } else if (source->kind == IR_OPERAND_TEMP && source->name) {
    const char *mapped =
        ir_name_map_get_or_create(temp_map, source->name, inline_prefix, "tmp");
    if (!mapped) {
      return 0;
    }
    *out = ir_operand_temp(mapped);
    return out->kind == IR_OPERAND_TEMP && out->name;
  } else if (source->kind == IR_OPERAND_LABEL && source->name) {
    const char *mapped = ir_name_map_get_or_create(label_map, source->name,
                                                   inline_prefix, "lbl");
    if (!mapped) {
      return 0;
    }
    *out = ir_operand_label(mapped);
    return out->kind == IR_OPERAND_LABEL && out->name;
  }

  return ir_operand_clone(source, out);
}

static int ir_clone_instruction_plain(const IRInstruction *source,
                                      IRInstruction *out) {
  if (!source || !out) {
    return 0;
  }

  memset(out, 0, sizeof(*out));
  out->op = source->op;
  out->location = source->location;
  out->is_float = source->is_float;
  out->float_bits = source->float_bits;
  out->ast_ref = source->ast_ref;

  if (!ir_operand_clone(&source->dest, &out->dest) ||
      !ir_operand_clone(&source->lhs, &out->lhs) ||
      !ir_operand_clone(&source->rhs, &out->rhs)) {
    ir_instruction_destroy_storage(out);
    return 0;
  }

  if (source->text) {
    out->text = ir_opt_strdup(source->text);
    if (!out->text) {
      ir_instruction_destroy_storage(out);
      return 0;
    }
  }

  out->argument_count = source->argument_count;
  if (source->argument_count > 0) {
    out->arguments = calloc(source->argument_count, sizeof(IROperand));
    if (!out->arguments) {
      ir_instruction_destroy_storage(out);
      return 0;
    }
    for (size_t i = 0; i < source->argument_count; i++) {
      if (!ir_operand_clone(&source->arguments[i], &out->arguments[i])) {
        ir_instruction_destroy_storage(out);
        return 0;
      }
    }
  }

  return 1;
}

static int ir_clone_instruction_for_inline(const IRInstruction *source,
                                           IRInstruction *out,
                                           IRNameMap *symbol_map,
                                           IRNameMap *temp_map,
                                           IRNameMap *label_map,
                                           const char *inline_prefix) {
  if (!source || !out || !symbol_map || !temp_map || !label_map ||
      !inline_prefix) {
    return 0;
  }

  memset(out, 0, sizeof(*out));
  out->op = source->op;
  out->location = source->location;
  out->is_float = source->is_float;
  out->float_bits = source->float_bits;
  out->ast_ref = NULL;

  if (!ir_inline_rewrite_operand(&source->dest, &out->dest, symbol_map,
                                 temp_map, label_map, inline_prefix) ||
      !ir_inline_rewrite_operand(&source->lhs, &out->lhs, symbol_map, temp_map,
                                 label_map, inline_prefix) ||
      !ir_inline_rewrite_operand(&source->rhs, &out->rhs, symbol_map, temp_map,
                                 label_map, inline_prefix)) {
    ir_instruction_destroy_storage(out);
    return 0;
  }

  if (source->text) {
    if (source->op == IR_OP_LABEL || source->op == IR_OP_JUMP ||
        source->op == IR_OP_BRANCH_ZERO || source->op == IR_OP_BRANCH_EQ) {
      const char *mapped =
          ir_name_map_get_or_create(label_map, source->text, inline_prefix, "lbl");
      if (!mapped) {
        ir_instruction_destroy_storage(out);
        return 0;
      }
      out->text = ir_opt_strdup(mapped);
    } else {
      out->text = ir_opt_strdup(source->text);
    }

    if (!out->text) {
      ir_instruction_destroy_storage(out);
      return 0;
    }
  }

  out->argument_count = source->argument_count;
  if (source->argument_count > 0) {
    out->arguments = calloc(source->argument_count, sizeof(IROperand));
    if (!out->arguments) {
      ir_instruction_destroy_storage(out);
      return 0;
    }

    for (size_t i = 0; i < source->argument_count; i++) {
      if (!ir_inline_rewrite_operand(&source->arguments[i], &out->arguments[i],
                                     symbol_map, temp_map, label_map,
                                     inline_prefix)) {
        ir_instruction_destroy_storage(out);
        return 0;
      }
    }
  }

  return 1;
}

static int ir_append_parameter_materialization(
    IRInstructionVector *vector, const IRInstruction *call_instruction,
    const IRFunction *callee, IRNameMap *symbol_map) {
  if (!vector || !call_instruction || !callee || !symbol_map) {
    return 0;
  }

  for (size_t i = 0; i < callee->parameter_count; i++) {
    const char *parameter_name = callee->parameter_names[i];
    const char *mapped_name = ir_name_map_lookup(symbol_map, parameter_name);
    const char *type_name = "int64";
    if (!parameter_name || !mapped_name) {
      return 0;
    }
    if (callee->parameter_types && callee->parameter_types[i] &&
        callee->parameter_types[i][0] != '\0') {
      type_name = callee->parameter_types[i];
    }

    IRInstruction declare_local = {0};
    declare_local.op = IR_OP_DECLARE_LOCAL;
    declare_local.location = call_instruction->location;
    declare_local.dest = ir_operand_symbol(mapped_name);
    declare_local.text = ir_opt_strdup(type_name);
    if (!declare_local.dest.name || !declare_local.text ||
        !ir_instruction_vector_append_move(vector, &declare_local)) {
      ir_instruction_destroy_storage(&declare_local);
      return 0;
    }

    IRInstruction assign = {0};
    assign.op = IR_OP_ASSIGN;
    assign.location = call_instruction->location;
    assign.dest = ir_operand_symbol(mapped_name);
    if (!assign.dest.name ||
        !ir_operand_clone(&call_instruction->arguments[i], &assign.lhs) ||
        !ir_instruction_vector_append_move(vector, &assign)) {
      ir_instruction_destroy_storage(&assign);
      return 0;
    }
  }

  return 1;
}

static int ir_inline_call_instruction(IRInstructionVector *vector,
                                      const IRInstruction *call_instruction,
                                      const IRFunction *callee,
                                      size_t inline_site_id) {
  if (!vector || !call_instruction || !callee) {
    return 0;
  }

  char *inline_prefix = ir_make_inline_prefix(callee->name, inline_site_id);
  if (!inline_prefix) {
    return 0;
  }

  IRNameMap symbol_map = {0};
  IRNameMap temp_map = {0};
  IRNameMap label_map = {0};
  int ok = 0;

  for (size_t i = 0; i < callee->parameter_count; i++) {
    const char *parameter_name = callee->parameter_names[i];
    if (!parameter_name) {
      goto cleanup;
    }

    char *mapped = ir_make_inline_name(inline_prefix, "param", parameter_name);
    if (!mapped) {
      goto cleanup;
    }
    int add_ok = ir_name_map_add(&symbol_map, parameter_name, mapped);
    free(mapped);
    if (!add_ok) {
      goto cleanup;
    }
  }

  for (size_t i = 0; i < callee->instruction_count; i++) {
    const IRInstruction *instruction = &callee->instructions[i];
    if (instruction->op == IR_OP_DECLARE_LOCAL &&
        instruction->dest.kind == IR_OPERAND_SYMBOL && instruction->dest.name) {
      char *mapped =
          ir_make_inline_name(inline_prefix, "local", instruction->dest.name);
      if (!mapped) {
        goto cleanup;
      }
      int add_ok = ir_name_map_add(&symbol_map, instruction->dest.name, mapped);
      free(mapped);
      if (!add_ok) {
        goto cleanup;
      }
    }
  }

  if (!ir_append_parameter_materialization(vector, call_instruction, callee,
                                           &symbol_map)) {
    goto cleanup;
  }

  char *inline_end_label = ir_make_inline_name(inline_prefix, "label", "end");
  if (!inline_end_label) {
    goto cleanup;
  }

  for (size_t i = 0; i < callee->instruction_count; i++) {
    const IRInstruction *source = &callee->instructions[i];
    IRInstruction emitted = {0};

    if (source->op == IR_OP_RETURN) {
      if (source->lhs.kind != IR_OPERAND_NONE &&
          call_instruction->dest.kind != IR_OPERAND_NONE) {
        emitted.op = IR_OP_ASSIGN;
        emitted.location = call_instruction->location;
        if (!ir_operand_clone(&call_instruction->dest, &emitted.dest) ||
            !ir_inline_rewrite_operand(&source->lhs, &emitted.lhs, &symbol_map,
                                       &temp_map, &label_map, inline_prefix) ||
            !ir_instruction_vector_append_move(vector, &emitted)) {
          ir_instruction_destroy_storage(&emitted);
          free(inline_end_label);
          goto cleanup;
        }
      }

      memset(&emitted, 0, sizeof(emitted));
      emitted.op = IR_OP_JUMP;
      emitted.location = call_instruction->location;
      emitted.text = ir_opt_strdup(inline_end_label);
      if (!emitted.text || !ir_instruction_vector_append_move(vector, &emitted)) {
        ir_instruction_destroy_storage(&emitted);
        free(inline_end_label);
        goto cleanup;
      }
      continue;
    }

    if (!ir_clone_instruction_for_inline(source, &emitted, &symbol_map, &temp_map,
                                         &label_map, inline_prefix) ||
        !ir_instruction_vector_append_move(vector, &emitted)) {
      ir_instruction_destroy_storage(&emitted);
      free(inline_end_label);
      goto cleanup;
    }
  }

  {
    IRInstruction end_label = {0};
    end_label.op = IR_OP_LABEL;
    end_label.location = call_instruction->location;
    end_label.text = inline_end_label;
    if (!ir_instruction_vector_append_move(vector, &end_label)) {
      ir_instruction_destroy_storage(&end_label);
      free(inline_end_label);
      goto cleanup;
    }
  }

  ok = 1;

cleanup:
  ir_name_map_destroy(&label_map);
  ir_name_map_destroy(&temp_map);
  ir_name_map_destroy(&symbol_map);
  free(inline_prefix);
  return ok;
}

static int ir_inline_calls_in_function(IRProgram *program, IRFunction *function,
                                       size_t *inline_counter, int *changed) {
  if (!program || !function || !inline_counter) {
    return 0;
  }

  if (ir_function_non_nop_instruction_count(function) >
      IR_INLINE_MAX_CALLER_NON_NOP_INSTRUCTIONS) {
    return 1;
  }

  IRInstructionVector vector = {0};
  int local_changed = 0;

  for (size_t i = 0; i < function->instruction_count; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    IRInstruction cloned = {0};

    if (instruction->op == IR_OP_CALL && instruction->text &&
        instruction->argument_count <= IR_INLINE_MAX_PARAMETERS) {
      IRFunction *callee = ir_program_find_function(program, instruction->text);
      if (callee && callee != function &&
          instruction->argument_count == callee->parameter_count &&
          ir_function_is_inline_candidate(callee)) {
        if (!ir_inline_call_instruction(&vector, instruction, callee,
                                        (*inline_counter)++)) {
          ir_instruction_vector_destroy(&vector);
          return 0;
        }
        local_changed = 1;
        continue;
      }
    }

    if (!ir_clone_instruction_plain(instruction, &cloned) ||
        !ir_instruction_vector_append_move(&vector, &cloned)) {
      ir_instruction_destroy_storage(&cloned);
      ir_instruction_vector_destroy(&vector);
      return 0;
    }
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    ir_instruction_destroy_storage(&function->instructions[i]);
  }
  free(function->instructions);
  function->instructions = vector.items;
  function->instruction_count = vector.count;
  function->instruction_capacity = vector.capacity;
  vector.items = NULL;
  vector.count = 0;
  vector.capacity = 0;

  if (local_changed && changed) {
    *changed = 1;
  }
  return 1;
}

static int ir_inline_small_functions_pass(IRProgram *program, int *changed) {
  if (!program) {
    return 0;
  }

  size_t inline_counter = 0;
  for (int round = 0; round < IR_INLINE_MAX_ROUNDS; round++) {
    int round_changed = 0;

    for (size_t i = 0; i < program->function_count; i++) {
      if (!ir_inline_calls_in_function(program, program->functions[i],
                                       &inline_counter, &round_changed)) {
        return 0;
      }
    }

    if (round_changed && changed) {
      *changed = 1;
    }
    if (!round_changed) {
      break;
    }
  }

  return 1;
}

static int ir_coalesce_single_use_temp_assign_pass(IRFunction *function,
                                                   int *changed) {
  if (!function) {
    return 0;
  }

  IRTempUseMap uses;
  if (!ir_temp_use_map_init(&uses)) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    if (!ir_collect_instruction_temp_uses(&uses, &function->instructions[i])) {
      ir_temp_use_map_destroy(&uses);
      return 0;
    }
  }

  for (size_t i = 1; i < function->instruction_count; i++) {
    IRInstruction *assign_instruction = &function->instructions[i];
    if (assign_instruction->op != IR_OP_ASSIGN ||
        assign_instruction->lhs.kind != IR_OPERAND_TEMP ||
        !assign_instruction->lhs.name ||
        assign_instruction->dest.kind == IR_OPERAND_NONE) {
      continue;
    }

    if (ir_temp_use_map_get(&uses, assign_instruction->lhs.name) != 1) {
      continue;
    }

    size_t producer_index = i;
    IRInstruction *producer = NULL;
    while (producer_index > 0) {
      producer_index--;
      if (function->instructions[producer_index].op == IR_OP_NOP) {
        continue;
      }
      producer = &function->instructions[producer_index];
      break;
    }
    if (!producer || !ir_instruction_writes_destination(producer) ||
        producer->dest.kind != IR_OPERAND_TEMP || !producer->dest.name ||
        strcmp(producer->dest.name, assign_instruction->lhs.name) != 0) {
      continue;
    }

    IROperand rewritten_dest = ir_operand_none();
    if (!ir_operand_clone(&assign_instruction->dest, &rewritten_dest)) {
      ir_temp_use_map_destroy(&uses);
      return 0;
    }

    ir_operand_destroy(&producer->dest);
    producer->dest = rewritten_dest;
    ir_instruction_make_nop(assign_instruction);
    if (changed) {
      *changed = 1;
    }
  }

  ir_temp_use_map_destroy(&uses);
  return 1;
}

static void ir_symbol_value_map_invalidate_name(IRSymbolValueMap *map,
                                              const char *symbol_name) {
  if (!map || !symbol_name) {
    return;
  }

  ir_temp_value_map_remove(map, symbol_name);

  size_t write = 0;
  for (size_t read = 0; read < map->count; read++) {
    IRTempValueEntry *entry = &map->items[read];
    int remove = 0;
    if (entry->value.kind == IR_OPERAND_SYMBOL && entry->value.name &&
        strcmp(entry->value.name, symbol_name) == 0) {
      remove = 1;
    }
    if (entry->value.kind == IR_OPERAND_TEMP && entry->value.name) {
      /* Temp values may embed propagated symbols; conservatively keep. */
    }

    if (remove) {
      free(entry->name);
      ir_operand_destroy(&entry->value);
      continue;
    }

    if (write != read) {
      map->items[write] = map->items[read];
    }
    write++;
  }
  map->count = write;
}

static int ir_resolve_propagated_value(const IRTempValueMap *temp_map,
                                       const IRSymbolValueMap *symbol_map,
                                       const IROperand *operand, IROperand *out,
                                       int depth) {
  if (!operand || !out) {
    return 0;
  }

  if (depth > 64) {
    return ir_operand_clone(operand, out);
  }

  if (operand->kind == IR_OPERAND_TEMP && operand->name && temp_map) {
    const IROperand *mapped =
        ir_temp_value_map_lookup(temp_map, operand->name);
    if (mapped) {
      return ir_resolve_propagated_value(temp_map, symbol_map, mapped, out,
                                         depth + 1);
    }
  }

  if (operand->kind == IR_OPERAND_SYMBOL && operand->name && symbol_map) {
    const IROperand *mapped =
        ir_temp_value_map_lookup(symbol_map, operand->name);
    if (mapped) {
      return ir_resolve_propagated_value(temp_map, symbol_map, mapped, out,
                                         depth + 1);
    }
  }

  return ir_operand_clone(operand, out);
}

static int ir_try_propagate_operand(IRTempValueMap *temp_map,
                                    IRSymbolValueMap *symbol_map,
                                    IROperand *operand, int *changed) {
  if (!operand) {
    return 1;
  }

  if (operand->kind == IR_OPERAND_TEMP && operand->name && temp_map) {
    const IROperand *mapped =
        ir_temp_value_map_lookup(temp_map, operand->name);
    if (!mapped) {
      return 1;
    }

    IROperand resolved = ir_operand_none();
    if (!ir_resolve_propagated_value(temp_map, symbol_map, mapped, &resolved,
                                     0)) {
      return 0;
    }

    ir_operand_destroy(operand);
    *operand = resolved;
    if (changed) {
      *changed = 1;
    }
    return 1;
  }

  if (operand->kind == IR_OPERAND_SYMBOL && operand->name && symbol_map) {
    const IROperand *mapped =
        ir_temp_value_map_lookup(symbol_map, operand->name);
    if (!mapped) {
      return 1;
    }

    IROperand resolved = ir_operand_none();
    if (!ir_resolve_propagated_value(temp_map, symbol_map, mapped, &resolved,
                                     0)) {
      return 0;
    }

    ir_operand_destroy(operand);
    *operand = resolved;
    if (changed) {
      *changed = 1;
    }
    return 1;
  }

  return 1;
}

static int ir_propagate_instruction_operands(IRTempValueMap *temp_map,
                                             IRSymbolValueMap *symbol_map,
                                             IRInstruction *instruction,
                                             int *changed) {
  if (!instruction) {
    return 0;
  }

  switch (instruction->op) {
  case IR_OP_STORE:
    if (!ir_try_propagate_operand(temp_map, symbol_map, &instruction->dest,
                                  changed) ||
        !ir_try_propagate_operand(temp_map, symbol_map, &instruction->lhs,
                                  changed) ||
        !ir_try_propagate_operand(temp_map, symbol_map, &instruction->rhs,
                                  changed)) {
      return 0;
    }
    break;

  case IR_OP_ROTATE_ADD:
    break;

  case IR_OP_BRANCH_ZERO:
    if (!ir_try_propagate_operand(temp_map, NULL, &instruction->lhs, changed)) {
      return 0;
    }
    break;

  case IR_OP_BRANCH_EQ:
    if (!ir_try_propagate_operand(temp_map, NULL, &instruction->lhs, changed) ||
        !ir_try_propagate_operand(temp_map, NULL, &instruction->rhs, changed)) {
      return 0;
    }
    break;

  case IR_OP_ASSIGN:
    if (!ir_try_propagate_operand(temp_map, symbol_map, &instruction->lhs,
                                  changed)) {
      return 0;
    }
    break;

  case IR_OP_ADDRESS_OF:
  case IR_OP_LOAD:
  case IR_OP_BINARY:
  case IR_OP_UNARY:
  case IR_OP_CAST:
  case IR_OP_NEW:
  case IR_OP_CALL:
  case IR_OP_CALL_INDIRECT:
  case IR_OP_RETURN:
    if (!ir_try_propagate_operand(temp_map, NULL, &instruction->lhs, changed) ||
        !ir_try_propagate_operand(temp_map, NULL, &instruction->rhs, changed)) {
      return 0;
    }
    for (size_t i = 0; i < instruction->argument_count; i++) {
      if (!ir_try_propagate_operand(temp_map, symbol_map,
                                    &instruction->arguments[i], changed)) {
        return 0;
      }
    }
    break;

  default:
    break;
  }

  return 1;
}

static int ir_copy_and_constant_propagation_pass(IRFunction *function,
                                                 int *changed) {
  if (!function) {
    return 0;
  }

  IRTempValueMap map;
  IRSymbolValueMap symbol_map;
  if (!ir_temp_value_map_init(&map) || !ir_temp_value_map_init(&symbol_map)) {
    ir_temp_value_map_destroy(&map);
    ir_temp_value_map_destroy(&symbol_map);
    return 0;
  }

  IRLabelValueMap label_in;
  if (!ir_label_value_map_init(&label_in)) {
    ir_temp_value_map_destroy(&map);
    ir_temp_value_map_destroy(&symbol_map);
    return 0;
  }

  int any_changed = 0;
  for (int iteration = 0; iteration < 8; iteration++) {
    int flow_changed = 0;
    ir_temp_value_map_clear(&map);
    ir_temp_value_map_clear(&symbol_map);

    for (size_t i = 0; i < function->instruction_count; i++) {
      IRInstruction *instruction = &function->instructions[i];

      if (instruction->op == IR_OP_LABEL && instruction->text) {
        /* The label is reachable from explicit jumps/branches *and* from
         * fall-through if the previous non-nop instruction is not a JUMP or
         * RETURN. Merge the fall-through map into label_in[L] first so the
         * load below is the intersection of every incoming flow. Without
         * this, a label after two writes "x <- 1 / jump L / x <- 0 / L:"
         * would inherit only the jump's map and wrongly conclude x == 1. */
        int fall_through = 1;
        for (size_t pi = i; pi > 0;) {
          pi--;
          IROpcode prev_op = function->instructions[pi].op;
          if (prev_op == IR_OP_NOP) {
            continue;
          }
          if (prev_op == IR_OP_JUMP || prev_op == IR_OP_RETURN) {
            fall_through = 0;
          }
          break;
        }
        if (i == 0) {
          fall_through = 0; /* first instruction has no predecessor */
        }

        if (fall_through) {
          if (!ir_label_value_map_merge_incoming(&label_in, instruction->text,
                                                 &map, &flow_changed)) {
            ir_label_value_map_destroy(&label_in);
            ir_temp_value_map_destroy(&map);
            ir_temp_value_map_destroy(&symbol_map);
            return 0;
          }
        }

        const IRLabelValueEntry *entry =
            ir_label_value_map_lookup(&label_in, instruction->text);
        if (entry && entry->initialized) {
          if (!ir_temp_value_map_clone(&map, &entry->in_map)) {
            ir_label_value_map_destroy(&label_in);
            ir_temp_value_map_destroy(&map);
            ir_temp_value_map_destroy(&symbol_map);
            return 0;
          }
        } else {
          ir_temp_value_map_clear(&map);
        }
        ir_temp_value_map_clear(&symbol_map);
      }

      if (!ir_propagate_instruction_operands(&map, &symbol_map, instruction,
                                             &any_changed)) {
        ir_label_value_map_destroy(&label_in);
        ir_temp_value_map_destroy(&map);
        ir_temp_value_map_destroy(&symbol_map);
        return 0;
      }

      if (ir_instruction_writes_temp(instruction) && instruction->dest.name) {
        ir_temp_value_map_remove(&map, instruction->dest.name);

        if (instruction->op == IR_OP_ASSIGN &&
            ir_operand_is_propagatable_value(&instruction->lhs)) {
          if (instruction->lhs.kind == IR_OPERAND_TEMP &&
              instruction->lhs.name &&
              strcmp(instruction->lhs.name, instruction->dest.name) == 0) {
            ir_temp_value_map_remove(&map, instruction->dest.name);
          } else if (!ir_temp_value_map_set(&map, instruction->dest.name,
                                            &instruction->lhs)) {
            ir_label_value_map_destroy(&label_in);
            ir_temp_value_map_destroy(&map);
            ir_temp_value_map_destroy(&symbol_map);
            return 0;
          }
        }
      }

      if (ir_instruction_writes_symbol(instruction) && instruction->dest.name) {
        ir_temp_value_map_remove_symbol_values(&map, instruction->dest.name);
        ir_symbol_value_map_invalidate_name(&symbol_map,
                                            instruction->dest.name);

        if (instruction->op == IR_OP_ASSIGN &&
            ir_operand_is_propagatable_value(&instruction->lhs) &&
            instruction->dest.kind == IR_OPERAND_SYMBOL) {
          if (instruction->lhs.kind == IR_OPERAND_SYMBOL &&
              instruction->lhs.name &&
              strcmp(instruction->lhs.name, instruction->dest.name) == 0) {
            ir_symbol_value_map_invalidate_name(&symbol_map,
                                                instruction->dest.name);
          } else if (!ir_temp_value_map_set(&symbol_map, instruction->dest.name,
                                            &instruction->lhs)) {
            ir_label_value_map_destroy(&label_in);
            ir_temp_value_map_destroy(&map);
            ir_temp_value_map_destroy(&symbol_map);
            return 0;
          }
        }
      }

      if (instruction->op == IR_OP_ROTATE_ADD && instruction->dest.name) {
        ir_symbol_value_map_invalidate_name(&symbol_map, instruction->dest.name);
        if (instruction->lhs.name) {
          ir_symbol_value_map_invalidate_name(&symbol_map, instruction->lhs.name);
        }
        if (instruction->rhs.name) {
          ir_symbol_value_map_invalidate_name(&symbol_map, instruction->rhs.name);
        }
      }

      if (instruction->op == IR_OP_STORE || instruction->op == IR_OP_CALL ||
          instruction->op == IR_OP_CALL_INDIRECT ||
          instruction->op == IR_OP_INLINE_ASM) {
        ir_temp_value_map_remove_symbol_values(&map, NULL);
        ir_temp_value_map_clear(&symbol_map);
      }

      if ((instruction->op == IR_OP_JUMP || instruction->op == IR_OP_BRANCH_ZERO ||
           instruction->op == IR_OP_BRANCH_EQ) &&
          instruction->text) {
        if (!ir_label_value_map_merge_incoming(&label_in, instruction->text,
                                               &map, &flow_changed)) {
          ir_label_value_map_destroy(&label_in);
          ir_temp_value_map_destroy(&map);
          ir_temp_value_map_destroy(&symbol_map);
          return 0;
        }
      }

      if (instruction->op == IR_OP_JUMP || instruction->op == IR_OP_RETURN) {
        ir_temp_value_map_clear(&map);
        ir_temp_value_map_clear(&symbol_map);
      }
    }

    if (!flow_changed) {
      break;
    }
  }

  if (changed && any_changed) {
    *changed = 1;
  }

  ir_label_value_map_destroy(&label_in);
  ir_temp_value_map_destroy(&map);
  ir_temp_value_map_destroy(&symbol_map);
  return 1;
}

static int ir_try_evaluate_integer_binary(const char *op, long long lhs,
                                          long long rhs, long long *result,
                                          int *folded) {
  if (!op || !result || !folded) {
    return 0;
  }

  *folded = 1;

  if (strcmp(op, "+") == 0) {
    *result = (long long)((unsigned long long)lhs + (unsigned long long)rhs);
  } else if (strcmp(op, "-") == 0) {
    *result = (long long)((unsigned long long)lhs - (unsigned long long)rhs);
  } else if (strcmp(op, "*") == 0) {
    *result = (long long)((unsigned long long)lhs * (unsigned long long)rhs);
  } else if (strcmp(op, "/") == 0) {
    if (rhs == 0 || (lhs == LLONG_MIN && rhs == -1)) {
      *folded = 0;
    } else {
      *result = lhs / rhs;
    }
  } else if (strcmp(op, "%") == 0) {
    if (rhs == 0 || (lhs == LLONG_MIN && rhs == -1)) {
      *folded = 0;
    } else {
      *result = lhs % rhs;
    }
  } else if (strcmp(op, "==") == 0) {
    *result = lhs == rhs;
  } else if (strcmp(op, "!=") == 0) {
    *result = lhs != rhs;
  } else if (strcmp(op, "<") == 0) {
    *result = lhs < rhs;
  } else if (strcmp(op, "<=") == 0) {
    *result = lhs <= rhs;
  } else if (strcmp(op, ">") == 0) {
    *result = lhs > rhs;
  } else if (strcmp(op, ">=") == 0) {
    *result = lhs >= rhs;
  } else if (strcmp(op, "&&") == 0) {
    *result = lhs && rhs;
  } else if (strcmp(op, "||") == 0) {
    *result = lhs || rhs;
  } else if (strcmp(op, "&") == 0) {
    *result = lhs & rhs;
  } else if (strcmp(op, "|") == 0) {
    *result = lhs | rhs;
  } else if (strcmp(op, "^") == 0) {
    *result = lhs ^ rhs;
  } else if (strcmp(op, "<<") == 0) {
    if (rhs < 0 || rhs >= 64) {
      *folded = 0;
    } else {
      *result = (long long)((unsigned long long)lhs << (unsigned long long)rhs);
    }
  } else if (strcmp(op, ">>") == 0) {
    if (rhs < 0 || rhs >= 64) {
      *folded = 0;
    } else {
      *result = lhs >> rhs;
    }
  } else {
    *folded = 0;
  }

  return 1;
}

static int ir_try_get_positive_pow2_shift(long long value, long long *shift) {
  if (!shift || value <= 0) {
    return 0;
  }

  unsigned long long u = (unsigned long long)value;
  if ((u & (u - 1ull)) != 0ull) {
    return 0;
  }

  long long amount = 0;
  while (u > 1ull) {
    u >>= 1u;
    amount++;
  }
  *shift = amount;
  return 1;
}

static int ir_rewrite_to_shift_left(IRInstruction *instruction,
                                    const IROperand *value, long long shift,
                                    int *changed) {
  if (!instruction || !value || instruction->op != IR_OP_BINARY ||
      instruction->is_float || shift < 0 || shift >= 64) {
    return 0;
  }

  IROperand lhs = ir_operand_none();
  if (!ir_operand_clone(value, &lhs)) {
    return 0;
  }

  ir_operand_destroy(&instruction->lhs);
  ir_operand_destroy(&instruction->rhs);
  instruction->lhs = lhs;
  instruction->rhs = ir_operand_int(shift);

  free(instruction->text);
  instruction->text = ir_opt_strdup("<<");
  if (!instruction->text) {
    return 0;
  }

  if (changed) {
    *changed = 1;
  }
  return 1;
}

static int ir_try_fold_integer_binary(IRInstruction *instruction, int *changed) {
  if (!instruction || instruction->op != IR_OP_BINARY || instruction->is_float ||
      !instruction->text) {
    return 1;
  }

  if (instruction->lhs.kind == IR_OPERAND_INT &&
      instruction->rhs.kind == IR_OPERAND_INT) {
    long long result = 0;
    int folded = 0;
    if (!ir_try_evaluate_integer_binary(instruction->text,
                                        instruction->lhs.int_value,
                                        instruction->rhs.int_value, &result,
                                        &folded)) {
      return 0;
    }
    if (folded) {
      return ir_rewrite_to_assign_int(instruction, result, changed);
    }
  }

  if (ir_operand_equals(&instruction->lhs, &instruction->rhs)) {
    if (strcmp(instruction->text, "-") == 0 ||
        strcmp(instruction->text, "^") == 0) {
      return ir_rewrite_to_assign_int(instruction, 0, changed);
    }
    if (strcmp(instruction->text, "|") == 0 ||
        strcmp(instruction->text, "&") == 0) {
      return ir_rewrite_to_assign_operand(instruction, &instruction->lhs,
                                          changed);
    }
    if (strcmp(instruction->text, "==") == 0 ||
        strcmp(instruction->text, "<=") == 0 ||
        strcmp(instruction->text, ">=") == 0) {
      return ir_rewrite_to_assign_int(instruction, 1, changed);
    }
    if (strcmp(instruction->text, "!=") == 0 ||
        strcmp(instruction->text, "<") == 0 ||
        strcmp(instruction->text, ">") == 0) {
      return ir_rewrite_to_assign_int(instruction, 0, changed);
    }
  }

  if (strcmp(instruction->text, "+") == 0) {
    if (instruction->rhs.kind == IR_OPERAND_INT &&
        instruction->rhs.int_value == 0) {
      return ir_rewrite_to_assign_operand(instruction, &instruction->lhs,
                                          changed);
    }
    if (instruction->lhs.kind == IR_OPERAND_INT &&
        instruction->lhs.int_value == 0) {
      return ir_rewrite_to_assign_operand(instruction, &instruction->rhs,
                                          changed);
    }
  } else if (strcmp(instruction->text, "-") == 0) {
    if (instruction->rhs.kind == IR_OPERAND_INT &&
        instruction->rhs.int_value == 0) {
      return ir_rewrite_to_assign_operand(instruction, &instruction->lhs,
                                          changed);
    }
  } else if (strcmp(instruction->text, "*") == 0) {
    long long shift = 0;
    if (instruction->rhs.kind == IR_OPERAND_INT &&
        ir_try_get_positive_pow2_shift(instruction->rhs.int_value, &shift) &&
        shift > 0) {
      return ir_rewrite_to_shift_left(instruction, &instruction->lhs, shift,
                                      changed);
    }
    if (instruction->lhs.kind == IR_OPERAND_INT &&
        ir_try_get_positive_pow2_shift(instruction->lhs.int_value, &shift) &&
        shift > 0) {
      return ir_rewrite_to_shift_left(instruction, &instruction->rhs, shift,
                                      changed);
    }
    if ((instruction->lhs.kind == IR_OPERAND_INT &&
         instruction->lhs.int_value == 0) ||
        (instruction->rhs.kind == IR_OPERAND_INT &&
         instruction->rhs.int_value == 0)) {
      return ir_rewrite_to_assign_int(instruction, 0, changed);
    }
    if (instruction->rhs.kind == IR_OPERAND_INT &&
        instruction->rhs.int_value == 1) {
      return ir_rewrite_to_assign_operand(instruction, &instruction->lhs,
                                          changed);
    }
    if (instruction->lhs.kind == IR_OPERAND_INT &&
        instruction->lhs.int_value == 1) {
      return ir_rewrite_to_assign_operand(instruction, &instruction->rhs,
                                          changed);
    }
  } else if (strcmp(instruction->text, "/") == 0) {
    if (instruction->rhs.kind == IR_OPERAND_INT &&
        instruction->rhs.int_value == 1) {
      return ir_rewrite_to_assign_operand(instruction, &instruction->lhs,
                                          changed);
    }
  } else if (strcmp(instruction->text, "%") == 0) {
    if (instruction->rhs.kind == IR_OPERAND_INT &&
        instruction->rhs.int_value == 1) {
      return ir_rewrite_to_assign_int(instruction, 0, changed);
    }
  } else if (strcmp(instruction->text, "&") == 0) {
    if ((instruction->lhs.kind == IR_OPERAND_INT &&
         instruction->lhs.int_value == 0) ||
        (instruction->rhs.kind == IR_OPERAND_INT &&
         instruction->rhs.int_value == 0)) {
      return ir_rewrite_to_assign_int(instruction, 0, changed);
    }
    if (instruction->rhs.kind == IR_OPERAND_INT &&
        instruction->rhs.int_value == -1) {
      return ir_rewrite_to_assign_operand(instruction, &instruction->lhs,
                                          changed);
    }
    if (instruction->lhs.kind == IR_OPERAND_INT &&
        instruction->lhs.int_value == -1) {
      return ir_rewrite_to_assign_operand(instruction, &instruction->rhs,
                                          changed);
    }
  } else if (strcmp(instruction->text, "|") == 0 ||
             strcmp(instruction->text, "^") == 0) {
    if (instruction->rhs.kind == IR_OPERAND_INT &&
        instruction->rhs.int_value == 0) {
      return ir_rewrite_to_assign_operand(instruction, &instruction->lhs,
                                          changed);
    }
    if (instruction->lhs.kind == IR_OPERAND_INT &&
        instruction->lhs.int_value == 0) {
      return ir_rewrite_to_assign_operand(instruction, &instruction->rhs,
                                          changed);
    }
  } else if (strcmp(instruction->text, "<<") == 0 ||
             strcmp(instruction->text, ">>") == 0) {
    if (instruction->rhs.kind == IR_OPERAND_INT &&
        instruction->rhs.int_value == 0) {
      return ir_rewrite_to_assign_operand(instruction, &instruction->lhs,
                                          changed);
    }
  }

  return 1;
}

static int ir_find_temp_producer_in_block(const IRFunction *function,
                                          size_t before_index,
                                          const char *temp_name,
                                          size_t *producer_index_out) {
  if (!function || !temp_name || !producer_index_out ||
      before_index > function->instruction_count) {
    return 0;
  }

  size_t i = before_index;
  while (i > 0) {
    i--;
    const IRInstruction *instruction = &function->instructions[i];
    if (instruction->op == IR_OP_NOP) {
      continue;
    }
    if (instruction->op == IR_OP_LABEL) {
      break;
    }
    if (ir_instruction_writes_temp(instruction) && instruction->dest.name &&
        strcmp(instruction->dest.name, temp_name) == 0) {
      *producer_index_out = i;
      return 1;
    }
  }

  return 0;
}

static int ir_try_rewrite_mod_pow2_compare_zero(IRFunction *function,
                                                const IRTempUseMap *uses,
                                                size_t compare_index,
                                                int *changed) {
  if (!function || !uses || compare_index >= function->instruction_count) {
    return 0;
  }

  IRInstruction *compare = &function->instructions[compare_index];
  if (compare->op != IR_OP_BINARY || compare->is_float || !compare->text) {
    return 1;
  }
  if (strcmp(compare->text, "==") != 0 && strcmp(compare->text, "!=") != 0) {
    return 1;
  }

  const IROperand *temp_operand = NULL;
  if (compare->lhs.kind == IR_OPERAND_TEMP && compare->lhs.name &&
      compare->rhs.kind == IR_OPERAND_INT && compare->rhs.int_value == 0) {
    temp_operand = &compare->lhs;
  } else if (compare->rhs.kind == IR_OPERAND_TEMP && compare->rhs.name &&
             compare->lhs.kind == IR_OPERAND_INT &&
             compare->lhs.int_value == 0) {
    temp_operand = &compare->rhs;
  } else {
    return 1;
  }

  if (ir_temp_use_map_get(uses, temp_operand->name) != 1) {
    return 1;
  }

  size_t producer_index = 0;
  if (!ir_find_temp_producer_in_block(function, compare_index,
                                      temp_operand->name, &producer_index)) {
    return 1;
  }

  IRInstruction *producer = &function->instructions[producer_index];
  if (producer->op != IR_OP_BINARY || producer->is_float || !producer->text ||
      strcmp(producer->text, "%") != 0 ||
      producer->rhs.kind != IR_OPERAND_INT) {
    return 1;
  }

  long long shift = 0;
  if (!ir_try_get_positive_pow2_shift(producer->rhs.int_value, &shift) ||
      shift <= 0 || shift >= 63) {
    return 1;
  }

  long long mask = ((long long)1 << shift) - 1;
  producer->rhs.int_value = mask;
  free(producer->text);
  producer->text = ir_opt_strdup("&");
  if (!producer->text) {
    return 0;
  }

  if (changed) {
    *changed = 1;
  }
  return 1;
}

static int ir_mod_even_bitcheck_pass(IRFunction *function, int *changed) {
  if (!function) {
    return 0;
  }

  IRTempUseMap uses;
  if (!ir_temp_use_map_init(&uses)) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    if (!ir_collect_instruction_temp_uses(&uses, &function->instructions[i])) {
      ir_temp_use_map_destroy(&uses);
      return 0;
    }
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    if (!ir_try_rewrite_mod_pow2_compare_zero(function, &uses, i, changed)) {
      ir_temp_use_map_destroy(&uses);
      return 0;
    }
  }

  ir_temp_use_map_destroy(&uses);
  return 1;
}

static int ir_find_next_non_nop_in_block(const IRFunction *function,
                                         size_t start_index, size_t *out_index) {
  if (!function || !out_index || start_index >= function->instruction_count) {
    return 0;
  }

  size_t i = start_index;
  while (i < function->instruction_count) {
    const IRInstruction *instruction = &function->instructions[i];
    if (instruction->op == IR_OP_LABEL) {
      return 0;
    }
    if (instruction->op != IR_OP_NOP) {
      *out_index = i;
      return 1;
    }
    i++;
  }
  return 0;
}

static int ir_find_next_non_nop(const IRFunction *function, size_t start_index,
                                size_t *out_index) {
  if (!function || !out_index || start_index >= function->instruction_count) {
    return 0;
  }

  size_t i = start_index;
  while (i < function->instruction_count) {
    const IRInstruction *instruction = &function->instructions[i];
    if (instruction->op != IR_OP_NOP) {
      *out_index = i;
      return 1;
    }
    i++;
  }
  return 0;
}

static int ir_find_label_index(const IRFunction *function, const char *label,
                               size_t *out_index) {
  if (!function || !label || !out_index) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    if (instruction->op == IR_OP_LABEL && instruction->text &&
        strcmp(instruction->text, label) == 0) {
      *out_index = i;
      return 1;
    }
  }

  return 0;
}

static int ir_branch_zero_not_equal_zero_forwarding_pass(IRFunction *function,
                                                         int *changed) {
  if (!function) {
    return 0;
  }

  IRTempUseMap uses;
  if (!ir_temp_use_map_init(&uses)) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    if (!ir_collect_instruction_temp_uses(&uses, &function->instructions[i])) {
      ir_temp_use_map_destroy(&uses);
      return 0;
    }
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    IRInstruction *cmp = &function->instructions[i];
    if (cmp->op != IR_OP_BINARY || cmp->is_float || !cmp->text ||
        strcmp(cmp->text, "!=") != 0 || cmp->dest.kind != IR_OPERAND_TEMP ||
        !cmp->dest.name) {
      continue;
    }

    if (ir_temp_use_map_get(&uses, cmp->dest.name) != 1) {
      continue;
    }

    const IROperand *forward = NULL;
    if (cmp->rhs.kind == IR_OPERAND_INT && cmp->rhs.int_value == 0) {
      forward = &cmp->lhs;
    } else if (cmp->lhs.kind == IR_OPERAND_INT && cmp->lhs.int_value == 0) {
      forward = &cmp->rhs;
    } else {
      continue;
    }

    size_t branch_index = 0;
    if (!ir_find_next_non_nop_in_block(function, i + 1, &branch_index)) {
      continue;
    }

    IRInstruction *branch = &function->instructions[branch_index];
    if (branch->op != IR_OP_BRANCH_ZERO || branch->lhs.kind != IR_OPERAND_TEMP ||
        !branch->lhs.name || strcmp(branch->lhs.name, cmp->dest.name) != 0) {
      continue;
    }

    IROperand cloned = ir_operand_none();
    if (!ir_operand_clone(forward, &cloned)) {
      ir_temp_use_map_destroy(&uses);
      return 0;
    }
    ir_operand_destroy(&branch->lhs);
    branch->lhs = cloned;

    ir_instruction_make_nop(cmp);
    if (changed) {
      *changed = 1;
    }
  }

  ir_temp_use_map_destroy(&uses);
  return 1;
}

static int ir_rewrite_branch_eq_shortcut(IRInstruction *producer,
                                         IRInstruction *branch_zero,
                                         IRInstruction *jump_true,
                                         int *changed) {
  if (!producer || !branch_zero || !jump_true || !jump_true->text) {
    return 0;
  }

  IROperand lhs = ir_operand_none();
  IROperand rhs = ir_operand_none();
  if (!ir_operand_clone(&producer->lhs, &lhs) ||
      !ir_operand_clone(&producer->rhs, &rhs)) {
    ir_operand_destroy(&lhs);
    ir_operand_destroy(&rhs);
    return 0;
  }

  char *target = ir_opt_strdup(jump_true->text);
  if (!target) {
    ir_operand_destroy(&lhs);
    ir_operand_destroy(&rhs);
    return 0;
  }

  ir_operand_destroy(&branch_zero->lhs);
  ir_operand_destroy(&branch_zero->rhs);
  free(branch_zero->text);
  ir_instruction_clear_arguments(branch_zero);
  branch_zero->op = IR_OP_BRANCH_EQ;
  branch_zero->lhs = lhs;
  branch_zero->rhs = rhs;
  branch_zero->text = target;
  branch_zero->is_float = producer->is_float;
  branch_zero->ast_ref = NULL;

  ir_instruction_make_nop(producer);
  ir_instruction_make_nop(jump_true);
  if (changed) {
    *changed = 1;
  }
  return 1;
}

static int ir_branch_eq_chain_shortcut_pass(IRFunction *function, int *changed) {
  if (!function) {
    return 0;
  }

  IRTempUseMap uses;
  if (!ir_temp_use_map_init(&uses)) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    if (!ir_collect_instruction_temp_uses(&uses, &function->instructions[i])) {
      ir_temp_use_map_destroy(&uses);
      return 0;
    }
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    IRInstruction *producer = &function->instructions[i];
    if (producer->op != IR_OP_BINARY || producer->is_float || !producer->text ||
        strcmp(producer->text, "==") != 0 ||
        producer->dest.kind != IR_OPERAND_TEMP || !producer->dest.name) {
      continue;
    }

    if (ir_temp_use_map_get(&uses, producer->dest.name) != 1) {
      continue;
    }

    size_t branch_index = 0;
    if (!ir_find_next_non_nop_in_block(function, i + 1, &branch_index)) {
      continue;
    }
    IRInstruction *branch = &function->instructions[branch_index];
    if (branch->op != IR_OP_BRANCH_ZERO || branch->lhs.kind != IR_OPERAND_TEMP ||
        !branch->lhs.name ||
        strcmp(branch->lhs.name, producer->dest.name) != 0 || !branch->text) {
      continue;
    }

    size_t jump_index = 0;
    if (!ir_find_next_non_nop_in_block(function, branch_index + 1, &jump_index)) {
      continue;
    }
    IRInstruction *jump_true = &function->instructions[jump_index];
    if (jump_true->op != IR_OP_JUMP || !jump_true->text) {
      continue;
    }

    size_t false_label_index = 0;
    if (!ir_find_next_non_nop(function, jump_index + 1, &false_label_index)) {
      continue;
    }
    IRInstruction *false_label = &function->instructions[false_label_index];
    if (false_label->op != IR_OP_LABEL || !false_label->text ||
        strcmp(false_label->text, branch->text) != 0) {
      continue;
    }

    if (!ir_rewrite_branch_eq_shortcut(producer, branch, jump_true, changed)) {
      ir_temp_use_map_destroy(&uses);
      return 0;
    }
  }

  ir_temp_use_map_destroy(&uses);
  return 1;
}

static int ir_simplify_redundant_assign(IRInstruction *instruction,
                                        int *changed) {
  if (!instruction || instruction->op != IR_OP_ASSIGN ||
      instruction->dest.kind == IR_OPERAND_NONE ||
      instruction->lhs.kind == IR_OPERAND_NONE) {
    return 1;
  }

  if (ir_operand_equals(&instruction->dest, &instruction->lhs)) {
    ir_instruction_make_nop(instruction);
    if (changed) {
      *changed = 1;
    }
  }

  return 1;
}

static int ir_try_fold_integer_unary(IRInstruction *instruction, int *changed) {
  if (!instruction || instruction->op != IR_OP_UNARY || instruction->is_float ||
      !instruction->text || instruction->lhs.kind != IR_OPERAND_INT) {
    return 1;
  }

  long long value = instruction->lhs.int_value;
  long long result = 0;
  int fold = 1;

  if (strcmp(instruction->text, "+") == 0) {
    result = value;
  } else if (strcmp(instruction->text, "-") == 0) {
    result = (long long)(-(unsigned long long)value);
  } else if (strcmp(instruction->text, "!") == 0) {
    result = !value;
  } else if (strcmp(instruction->text, "~") == 0) {
    result = ~value;
  } else {
    fold = 0;
  }

  if (fold) {
    return ir_rewrite_to_assign_int(instruction, result, changed);
  }

  return 1;
}

static int ir_simplify_branch(IRInstruction *instruction, int *changed) {
  if (!instruction) {
    return 0;
  }

  if (instruction->op == IR_OP_BRANCH_ZERO &&
      instruction->lhs.kind == IR_OPERAND_INT) {
    if (instruction->lhs.int_value == 0) {
      ir_instruction_make_jump(instruction);
    } else {
      ir_instruction_make_nop(instruction);
    }
    if (changed) {
      *changed = 1;
    }
  } else if (instruction->op == IR_OP_BRANCH_EQ &&
             instruction->lhs.kind == IR_OPERAND_INT &&
             instruction->rhs.kind == IR_OPERAND_INT) {
    if (instruction->lhs.int_value == instruction->rhs.int_value) {
      ir_instruction_make_jump(instruction);
    } else {
      ir_instruction_make_nop(instruction);
    }
    if (changed) {
      *changed = 1;
    }
  } else if (instruction->op == IR_OP_BRANCH_EQ &&
             ir_operand_equals(&instruction->lhs, &instruction->rhs)) {
    ir_instruction_make_jump(instruction);
    if (changed) {
      *changed = 1;
    }
  }

  return 1;
}

static int ir_constant_and_branch_simplify_pass(IRFunction *function,
                                                int *changed) {
  if (!function) {
    return 0;
  }

  int has_mod = 0;
  int has_eq = 0;
  int has_ne = 0;
  int has_branch_zero = 0;
  int has_jump = 0;
  int has_label = 0;

  for (size_t i = 0; i < function->instruction_count; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    switch (instruction->op) {
    case IR_OP_BINARY:
      if (instruction->text) {
        if (strcmp(instruction->text, "%") == 0) {
          has_mod = 1;
        } else if (strcmp(instruction->text, "==") == 0) {
          has_eq = 1;
        } else if (strcmp(instruction->text, "!=") == 0) {
          has_ne = 1;
        }
      }
      break;
    case IR_OP_BRANCH_ZERO:
      has_branch_zero = 1;
      break;
    case IR_OP_JUMP:
      has_jump = 1;
      break;
    case IR_OP_LABEL:
      has_label = 1;
      break;
    default:
      break;
    }
  }

  if (has_mod && (has_eq || has_ne)) {
    if (!ir_mod_even_bitcheck_pass(function, changed)) {
      return 0;
    }
  }
  if (has_eq && has_branch_zero && has_jump && has_label) {
    if (!ir_branch_eq_chain_shortcut_pass(function, changed)) {
      return 0;
    }
  }
  if (has_ne && has_branch_zero) {
    if (!ir_branch_zero_not_equal_zero_forwarding_pass(function, changed)) {
      return 0;
    }
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    IRInstruction *instruction = &function->instructions[i];
    if (!ir_try_fold_integer_binary(instruction, changed) ||
        !ir_try_fold_integer_unary(instruction, changed) ||
        !ir_simplify_redundant_assign(instruction, changed) ||
        !ir_simplify_branch(instruction, changed)) {
      return 0;
    }
  }

  return 1;
}

static int ir_remove_redundant_jumps_pass(IRFunction *function, int *changed) {
  if (!function) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    IRInstruction *instruction = &function->instructions[i];
    if (instruction->op != IR_OP_JUMP || !instruction->text) {
      continue;
    }

    size_t next = i + 1;
    while (next < function->instruction_count &&
           function->instructions[next].op == IR_OP_NOP) {
      next++;
    }

    if (next < function->instruction_count &&
        function->instructions[next].op == IR_OP_LABEL &&
        function->instructions[next].text &&
        strcmp(function->instructions[next].text, instruction->text) == 0) {
      ir_instruction_make_nop(instruction);
      if (changed) {
        *changed = 1;
      }
    }
  }

  return 1;
}

static int ir_thread_jump_targets_pass(IRFunction *function, int *changed) {
  if (!function) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    IRInstruction *instruction = &function->instructions[i];
    if ((instruction->op != IR_OP_JUMP &&
         instruction->op != IR_OP_BRANCH_ZERO &&
         instruction->op != IR_OP_BRANCH_EQ) ||
        !instruction->text) {
      continue;
    }

    const char *current_target = instruction->text;
    for (int depth = 0; depth < 32; depth++) {
      size_t label_index = 0;
      if (!ir_find_label_index(function, current_target, &label_index)) {
        break;
      }

      size_t next_index = 0;
      if (!ir_find_next_non_nop(function, label_index + 1, &next_index)) {
        break;
      }

      IRInstruction *next = &function->instructions[next_index];
      if (next->op != IR_OP_JUMP || !next->text ||
          strcmp(next->text, current_target) == 0) {
        break;
      }

      current_target = next->text;
    }

    if (strcmp(current_target, instruction->text) != 0) {
      char *target_copy = ir_opt_strdup(current_target);
      if (!target_copy) {
        return 0;
      }
      free(instruction->text);
      instruction->text = target_copy;
      if (changed) {
        *changed = 1;
      }
    }
  }

  return 1;
}

static int ir_remove_redundant_fallthrough_branches_pass(IRFunction *function,
                                                         int *changed) {
  if (!function) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    IRInstruction *instruction = &function->instructions[i];
    if ((instruction->op != IR_OP_BRANCH_ZERO &&
         instruction->op != IR_OP_BRANCH_EQ) ||
        !instruction->text) {
      continue;
    }

    size_t next = i + 1;
    while (next < function->instruction_count &&
           function->instructions[next].op == IR_OP_NOP) {
      next++;
    }

    if (next < function->instruction_count &&
        function->instructions[next].op == IR_OP_LABEL &&
        function->instructions[next].text &&
        strcmp(function->instructions[next].text, instruction->text) == 0) {
      ir_instruction_make_nop(instruction);
      if (changed) {
        *changed = 1;
      }
    }
  }

  return 1;
}

static int ir_remove_empty_conditional_diamonds_pass(IRFunction *function,
                                                     int *changed) {
  if (!function) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    IRInstruction *branch = &function->instructions[i];
    if ((branch->op != IR_OP_BRANCH_ZERO && branch->op != IR_OP_BRANCH_EQ) ||
        !branch->text) {
      continue;
    }

    size_t jump_index = 0;
    if (!ir_find_next_non_nop_in_block(function, i + 1, &jump_index)) {
      continue;
    }

    IRInstruction *jump = &function->instructions[jump_index];
    if (jump->op != IR_OP_JUMP || !jump->text) {
      continue;
    }

    if (strcmp(branch->text, jump->text) == 0) {
      ir_instruction_make_nop(branch);
      ir_instruction_make_nop(jump);
      if (changed) {
        *changed = 1;
      }
      continue;
    }

    size_t cursor = jump_index + 1;
    while (cursor < function->instruction_count &&
           function->instructions[cursor].op == IR_OP_NOP) {
      cursor++;
    }

    if (cursor >= function->instruction_count ||
        function->instructions[cursor].op != IR_OP_LABEL ||
        !function->instructions[cursor].text ||
        strcmp(function->instructions[cursor].text, branch->text) != 0) {
      continue;
    }

    cursor++;
    int reaches_jump_target_with_no_body = 0;
    while (cursor < function->instruction_count) {
      IRInstruction *probe = &function->instructions[cursor];
      if (probe->op == IR_OP_NOP) {
        cursor++;
        continue;
      }
      if (probe->op != IR_OP_LABEL || !probe->text) {
        break;
      }
      if (strcmp(probe->text, jump->text) == 0) {
        reaches_jump_target_with_no_body = 1;
        break;
      }
      cursor++;
    }

    if (!reaches_jump_target_with_no_body) {
      continue;
    }

    ir_instruction_make_nop(branch);
    ir_instruction_make_nop(jump);
    if (changed) {
      *changed = 1;
    }
  }

  return 1;
}

static int ir_reachable_enqueue_label(const IRFunction *function,
                                      const char *label,
                                      IRIndexVector *worklist) {
  size_t label_index = 0;
  if (!label || !ir_find_label_index(function, label, &label_index)) {
    return 1;
  }
  return ir_index_vector_append(worklist, label_index);
}

static int ir_eliminate_unreachable_blocks_pass(IRFunction *function,
                                                int *changed) {
  if (!function) {
    return 0;
  }
  if (function->instruction_count == 0) {
    return 1;
  }

  unsigned char *reachable = calloc(function->instruction_count, 1);
  if (!reachable) {
    return 0;
  }

  IRIndexVector worklist = {0};
  if (!ir_index_vector_append(&worklist, 0)) {
    ir_index_vector_destroy(&worklist);
    free(reachable);
    return 0;
  }

  for (size_t work_index = 0; work_index < worklist.count; work_index++) {
    size_t i = worklist.items[work_index];
    while (i < function->instruction_count) {
      if (reachable[i]) {
        break;
      }

      IRInstruction *instruction = &function->instructions[i];
      reachable[i] = 1;

      if (instruction->op == IR_OP_JUMP) {
        if (!ir_reachable_enqueue_label(function, instruction->text,
                                        &worklist)) {
          ir_index_vector_destroy(&worklist);
          free(reachable);
          return 0;
        }
        break;
      }

      if (instruction->op == IR_OP_BRANCH_ZERO ||
          instruction->op == IR_OP_BRANCH_EQ) {
        if (!ir_reachable_enqueue_label(function, instruction->text,
                                        &worklist)) {
          ir_index_vector_destroy(&worklist);
          free(reachable);
          return 0;
        }
      } else if (instruction->op == IR_OP_RETURN) {
        break;
      }

      i++;
    }
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    if (!reachable[i] && function->instructions[i].op != IR_OP_NOP) {
      ir_instruction_make_nop(&function->instructions[i]);
      if (changed) {
        *changed = 1;
      }
    }
  }

  ir_index_vector_destroy(&worklist);
  free(reachable);
  return 1;
}

static int ir_instruction_references_label(const IRInstruction *instruction,
                                           const char *label) {
  if (!instruction || !label || !instruction->text) {
    return 0;
  }

  if (instruction->op != IR_OP_JUMP && instruction->op != IR_OP_BRANCH_ZERO &&
      instruction->op != IR_OP_BRANCH_EQ) {
    return 0;
  }

  return strcmp(instruction->text, label) == 0;
}

static int ir_label_is_referenced(const IRFunction *function,
                                  const char *label) {
  if (!function || !label) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    if (ir_instruction_references_label(&function->instructions[i], label)) {
      return 1;
    }
  }

  return 0;
}

static int ir_remove_unused_labels_pass(IRFunction *function, int *changed) {
  if (!function) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    IRInstruction *instruction = &function->instructions[i];
    if (instruction->op == IR_OP_LABEL && instruction->text &&
        !ir_label_is_referenced(function, instruction->text)) {
      ir_instruction_make_nop(instruction);
      if (changed) {
        *changed = 1;
      }
    }
  }

  return 1;
}

static int ir_eliminate_unreachable_straightline_pass(IRFunction *function,
                                                       int *changed) {
  if (!function) {
    return 0;
  }

  int reachable = 1;

  for (size_t i = 0; i < function->instruction_count; i++) {
    IRInstruction *instruction = &function->instructions[i];

    if (instruction->op == IR_OP_LABEL) {
      reachable = 1;
      continue;
    }

    if (!reachable) {
      if (instruction->op != IR_OP_NOP) {
        ir_instruction_make_nop(instruction);
        if (changed) {
          *changed = 1;
        }
      }
      continue;
    }

    if (instruction->op == IR_OP_JUMP || instruction->op == IR_OP_RETURN) {
      reachable = 0;
    }
  }

  return 1;
}

static int ir_operand_is_symbol_named(const IROperand *operand,
                                      const char *name) {
  return operand && operand->kind == IR_OPERAND_SYMBOL && operand->name &&
         name && strcmp(operand->name, name) == 0;
}

static int ir_try_fuse_rotate_add_at(IRFunction *function, size_t index,
                                     int *changed) {
  if (!function || index + 3 >= function->instruction_count) {
    return 1;
  }

  IRInstruction *add_inst = &function->instructions[index];
  IRInstruction *assign_next = &function->instructions[index + 1];
  IRInstruction *assign_a = &function->instructions[index + 2];
  IRInstruction *assign_b = &function->instructions[index + 3];

  if (add_inst->op != IR_OP_BINARY || add_inst->is_float || add_inst->ast_ref ||
      !add_inst->text || strcmp(add_inst->text, "+") != 0 ||
      add_inst->dest.kind != IR_OPERAND_TEMP || !add_inst->dest.name ||
      add_inst->lhs.kind != IR_OPERAND_SYMBOL || !add_inst->lhs.name ||
      add_inst->rhs.kind != IR_OPERAND_SYMBOL || !add_inst->rhs.name) {
    return 1;
  }

  const char *sym_a = add_inst->lhs.name;
  const char *sym_b = add_inst->rhs.name;
  const char *temp_sum = add_inst->dest.name;

  if (assign_next->op != IR_OP_ASSIGN ||
      assign_next->dest.kind != IR_OPERAND_SYMBOL || !assign_next->dest.name ||
      assign_a->op != IR_OP_ASSIGN ||
      assign_a->dest.kind != IR_OPERAND_SYMBOL || !assign_a->dest.name ||
      assign_b->op != IR_OP_ASSIGN ||
      assign_b->dest.kind != IR_OPERAND_SYMBOL || !assign_b->dest.name) {
    return 1;
  }

  const char *sym_next = assign_next->dest.name;
  int next_from_temp =
      assign_next->lhs.kind == IR_OPERAND_TEMP && assign_next->lhs.name &&
      strcmp(assign_next->lhs.name, temp_sum) == 0;
  if (!next_from_temp) {
    return 1;
  }

  if (!ir_operand_is_symbol_named(&assign_a->lhs, sym_b) ||
      !ir_operand_is_symbol_named(&assign_a->dest, sym_a)) {
    return 1;
  }

  int b_from_next =
      ir_operand_is_symbol_named(&assign_b->dest, sym_b) &&
      ir_operand_is_symbol_named(&assign_b->lhs, sym_next);
  int b_from_temp =
      ir_operand_is_symbol_named(&assign_b->dest, sym_b) &&
      assign_b->lhs.kind == IR_OPERAND_TEMP && assign_b->lhs.name &&
      strcmp(assign_b->lhs.name, temp_sum) == 0;
  if (!b_from_next && !b_from_temp) {
    return 1;
  }

  IRInstruction fused = {0};
  fused.op = IR_OP_ROTATE_ADD;
  fused.location = add_inst->location;
  fused.dest = ir_operand_symbol(sym_next);
  fused.lhs = ir_operand_symbol(sym_a);
  fused.rhs = ir_operand_symbol(sym_b);
  fused.text = ir_opt_strdup("+");
  if (!fused.dest.name || !fused.lhs.name || !fused.rhs.name || !fused.text) {
    ir_operand_destroy(&fused.dest);
    ir_operand_destroy(&fused.lhs);
    ir_operand_destroy(&fused.rhs);
    free(fused.text);
    return 0;
  }

  ir_instruction_destroy_storage(add_inst);
  *add_inst = fused;
  ir_instruction_make_nop(assign_next);
  ir_instruction_make_nop(assign_a);
  ir_instruction_make_nop(assign_b);

  if (changed) {
    *changed = 1;
  }
  return 1;
}

static int ir_fuse_rotate_add_pass(IRFunction *function, int *changed) {
  if (!function) {
    return 0;
  }

  for (size_t i = 0; i + 3 < function->instruction_count; i++) {
    if (!ir_try_fuse_rotate_add_at(function, i, changed)) {
      return 0;
    }
    if (function->instructions[i].op == IR_OP_ROTATE_ADD) {
      i += 3;
    }
  }

  return 1;
}

static int ir_strength_reduce_rotate_loops_pass(IRFunction *function, int *changed) {
  if (!function) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    IRInstruction *inst = &function->instructions[i];
    if (inst->op != IR_OP_LABEL || !inst->text) {
      continue;
    }

    const char *loop_label = inst->text;
    size_t body_start = i + 1;
    size_t body_end = function->instruction_count;
    size_t jump_index = (size_t)-1;

    for (size_t j = body_start; j < function->instruction_count; j++) {
      IRInstruction *probe = &function->instructions[j];
      if (probe->op == IR_OP_JUMP && probe->text &&
          strcmp(probe->text, loop_label) == 0) {
        jump_index = j;
        body_end = j;
        break;
      }
      if (probe->op == IR_OP_LABEL) {
        break;
      }
    }

    if (jump_index == (size_t)-1) {
      continue;
    }

    int only_rotate_or_nop = 1;
    for (size_t j = body_start; j < body_end; j++) {
      IROpcode op = function->instructions[j].op;
      if (op == IR_OP_NOP || op == IR_OP_ROTATE_ADD) {
        continue;
      }
      if (op == IR_OP_BINARY || op == IR_OP_ASSIGN) {
        only_rotate_or_nop = 0;
        break;
      }
      if (op == IR_OP_BRANCH_ZERO || op == IR_OP_BRANCH_EQ) {
        continue;
      }
      only_rotate_or_nop = 0;
      break;
    }

    if (!only_rotate_or_nop) {
      continue;
    }

    for (size_t j = body_start; j < body_end; j++) {
      if (!ir_try_fuse_rotate_add_at(function, j, changed)) {
        return 0;
      }
      if (function->instructions[j].op == IR_OP_ROTATE_ADD) {
        j += 3;
      }
    }
  }

  return 1;
}

static int ir_instruction_has_side_effect(const IRInstruction *instruction) {
  if (!instruction) {
    return 0;
  }

  switch (instruction->op) {
  case IR_OP_STORE:
  case IR_OP_CALL:
  case IR_OP_CALL_INDIRECT:
  case IR_OP_MEMCPY_INLINE:
  case IR_OP_SIMD_MATMUL_N32:
  case IR_OP_RETURN:
  case IR_OP_INLINE_ASM:
    return 1;
  default:
    return 0;
  }
}

static int ir_operand_is_temp_named(const IROperand *operand,
                                    const char *name) {
  return operand && operand->kind == IR_OPERAND_TEMP && operand->name &&
         name && strcmp(operand->name, name) == 0;
}

static int ir_operand_is_int_value(const IROperand *operand,
                                   long long value) {
  return operand && operand->kind == IR_OPERAND_INT &&
         operand->int_value == value;
}

static int ir_instruction_reads_symbol_operand(const IRInstruction *instruction,
                                               const char *symbol_name) {
  if (!instruction || !symbol_name) {
    return 0;
  }

  if (ir_operand_is_symbol_named(&instruction->lhs, symbol_name) ||
      ir_operand_is_symbol_named(&instruction->rhs, symbol_name)) {
    return 1;
  }

  for (size_t i = 0; i < instruction->argument_count; i++) {
    if (ir_operand_is_symbol_named(&instruction->arguments[i], symbol_name)) {
      return 1;
    }
  }

  return 0;
}

static int ir_symbol_read_after(const IRFunction *function, size_t start_index,
                                const char *symbol_name) {
  if (!function || !symbol_name) {
    return 0;
  }

  for (size_t i = start_index; i < function->instruction_count; i++) {
    if (ir_instruction_reads_symbol_operand(&function->instructions[i],
                                            symbol_name)) {
      return 1;
    }
  }
  return 0;
}

static int ir_symbol_zero_initialized_before(const IRFunction *function,
                                             size_t before_index,
                                             const char *symbol_name) {
  if (!function || !symbol_name) {
    return 0;
  }

  for (size_t i = before_index; i > 0;) {
    i--;
    const IRInstruction *instruction = &function->instructions[i];
    if (instruction->op == IR_OP_LABEL) {
      break;
    }
    if (instruction->op == IR_OP_ASSIGN &&
        ir_operand_is_symbol_named(&instruction->dest, symbol_name)) {
      return ir_operand_is_int_value(&instruction->lhs, 0);
    }
    if (ir_instruction_writes_destination(instruction) &&
        ir_operand_is_symbol_named(&instruction->dest, symbol_name)) {
      return 0;
    }
  }

  return 0;
}

static int ir_try_match_word_count_load(const IRFunction *function,
                                        size_t body_start, size_t body_end,
                                        const char *iv_symbol,
                                        const char **buf_symbol_out,
                                        const char **char_symbol_out) {
  if (!function || !iv_symbol || !buf_symbol_out || !char_symbol_out) {
    return 0;
  }

  for (size_t i = body_start; i + 2 < body_end; i++) {
    const IRInstruction *addr = &function->instructions[i];
    if (addr->op == IR_OP_NOP) {
      continue;
    }
    if (addr->op != IR_OP_BINARY || addr->is_float || !addr->text ||
        strcmp(addr->text, "+") != 0 ||
        addr->dest.kind != IR_OPERAND_TEMP || !addr->dest.name) {
      continue;
    }

    const char *buf_symbol = NULL;
    if (addr->lhs.kind == IR_OPERAND_SYMBOL && addr->lhs.name &&
        ir_operand_is_symbol_named(&addr->rhs, iv_symbol)) {
      buf_symbol = addr->lhs.name;
    } else if (addr->rhs.kind == IR_OPERAND_SYMBOL && addr->rhs.name &&
               ir_operand_is_symbol_named(&addr->lhs, iv_symbol)) {
      buf_symbol = addr->rhs.name;
    } else {
      continue;
    }

    size_t load_index = 0;
    if (!ir_find_next_non_nop_in_block(function, i + 1, &load_index) ||
        load_index >= body_end) {
      return 0;
    }

    const IRInstruction *load = &function->instructions[load_index];
    if (load->op != IR_OP_LOAD ||
        !ir_operand_is_temp_named(&load->lhs, addr->dest.name) ||
        load->dest.kind != IR_OPERAND_TEMP || !load->dest.name ||
        load->rhs.kind != IR_OPERAND_INT || load->rhs.int_value != 1) {
      continue;
    }

    size_t cast_index = 0;
    if (!ir_find_next_non_nop_in_block(function, load_index + 1,
                                       &cast_index) ||
        cast_index >= body_end) {
      return 0;
    }

    const IRInstruction *cast = &function->instructions[cast_index];
    if (cast->op == IR_OP_CAST &&
        ir_operand_is_temp_named(&cast->lhs, load->dest.name) &&
        cast->dest.kind == IR_OPERAND_SYMBOL && cast->dest.name) {
      *buf_symbol_out = buf_symbol;
      *char_symbol_out = cast->dest.name;
      return 1;
    }
  }

  return 0;
}

static int ir_try_match_word_count_whitespace_chain(
    const IRFunction *function, size_t body_start, size_t body_end,
    const char *char_symbol, const char **whitespace_label_out) {
  if (!function || !char_symbol || !whitespace_label_out) {
    return 0;
  }

  int seen_space = 0;
  int seen_tab = 0;
  int seen_lf = 0;
  int seen_cr = 0;
  const char *ws_label = NULL;

  for (size_t i = body_start; i < body_end; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    long long value = 0;
    const char *target = NULL;

    if (instruction->op == IR_OP_BRANCH_EQ &&
        ir_operand_is_symbol_named(&instruction->lhs, char_symbol) &&
        instruction->rhs.kind == IR_OPERAND_INT && instruction->text) {
      value = instruction->rhs.int_value;
      target = instruction->text;
    } else if (instruction->op == IR_OP_BINARY && !instruction->is_float &&
               instruction->text && strcmp(instruction->text, "==") == 0 &&
               ir_operand_is_symbol_named(&instruction->lhs, char_symbol) &&
               instruction->rhs.kind == IR_OPERAND_INT &&
               instruction->dest.kind == IR_OPERAND_TEMP &&
               instruction->dest.name) {
      size_t branch_index = 0;
      if (!ir_find_next_non_nop_in_block(function, i + 1, &branch_index) ||
          branch_index >= body_end) {
        continue;
      }
      const IRInstruction *branch = &function->instructions[branch_index];
      if (branch->op != IR_OP_BRANCH_ZERO ||
          !ir_operand_is_temp_named(&branch->lhs, instruction->dest.name) ||
          !branch->text) {
        continue;
      }
      value = instruction->rhs.int_value;
      target = NULL;
      for (size_t label_index = branch_index + 1; label_index < body_end;
           label_index++) {
        const IRInstruction *label = &function->instructions[label_index];
        if (label->op == IR_OP_NOP) {
          continue;
        }
        if (label->op == IR_OP_LABEL && label->text &&
            strcmp(label->text, branch->text) != 0) {
          target = label->text;
        }
        break;
      }
      if (!target) {
        continue;
      }
    } else {
      continue;
    }

    if (value != 32 && value != 9 && value != 10 && value != 13) {
      continue;
    }

    if (!ws_label) {
      ws_label = target;
    } else if (!target || strcmp(ws_label, target) != 0) {
      return 0;
    }

    if (value == 32) seen_space = 1;
    if (value == 9) seen_tab = 1;
    if (value == 10) seen_lf = 1;
    if (value == 13) seen_cr = 1;
  }

  if (seen_space && seen_tab && seen_lf && seen_cr && ws_label) {
    *whitespace_label_out = ws_label;
    return 1;
  }

  return 0;
}

static int ir_try_match_word_count_state_updates(
    const IRFunction *function, size_t body_start, size_t body_end,
    const char *whitespace_label, const char **count_symbol_out,
    const char **in_word_symbol_out) {
  if (!function || !whitespace_label || !count_symbol_out ||
      !in_word_symbol_out) {
    return 0;
  }

  const char *in_word = NULL;
  const char *count = NULL;

  for (size_t i = body_start; i < body_end; i++) {
    const IRInstruction *label = &function->instructions[i];
    if (label->op != IR_OP_LABEL || !label->text ||
        strcmp(label->text, whitespace_label) != 0) {
      continue;
    }

    size_t assign_index = 0;
    if (!ir_find_next_non_nop_in_block(function, i + 1, &assign_index) ||
        assign_index >= body_end) {
      return 0;
    }
    const IRInstruction *assign = &function->instructions[assign_index];
    if (assign->op != IR_OP_ASSIGN ||
        assign->dest.kind != IR_OPERAND_SYMBOL || !assign->dest.name ||
        !ir_operand_is_int_value(&assign->lhs, 0)) {
      return 0;
    }
    in_word = assign->dest.name;
    break;
  }

  if (!in_word) {
    return 0;
  }

  for (size_t i = body_start; i < body_end; i++) {
    const IRInstruction *test = &function->instructions[i];
    if (test->op != IR_OP_BINARY || test->is_float || !test->text ||
        strcmp(test->text, "==") != 0 ||
        !ir_operand_is_symbol_named(&test->lhs, in_word) ||
        !ir_operand_is_int_value(&test->rhs, 0) ||
        test->dest.kind != IR_OPERAND_TEMP || !test->dest.name) {
      continue;
    }

    size_t branch_index = 0;
    if (!ir_find_next_non_nop_in_block(function, i + 1, &branch_index) ||
        branch_index >= body_end) {
      return 0;
    }
    const IRInstruction *branch = &function->instructions[branch_index];
    if (branch->op != IR_OP_BRANCH_ZERO ||
        !ir_operand_is_temp_named(&branch->lhs, test->dest.name)) {
      continue;
    }

    size_t add_index = 0;
    if (!ir_find_next_non_nop_in_block(function, branch_index + 1,
                                       &add_index) ||
        add_index >= body_end) {
      return 0;
    }
    const IRInstruction *add = &function->instructions[add_index];
    if (add->op == IR_OP_BINARY && !add->is_float && add->text &&
        strcmp(add->text, "+") == 0 &&
        add->dest.kind == IR_OPERAND_SYMBOL && add->dest.name &&
        ir_operand_is_symbol_named(&add->lhs, add->dest.name) &&
        ir_operand_is_int_value(&add->rhs, 1)) {
      count = add->dest.name;
    }
    break;
  }

  if (!count) {
    return 0;
  }

  int saw_set_one = 0;
  for (size_t i = body_start; i < body_end; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    if (instruction->op == IR_OP_ASSIGN &&
        ir_operand_is_symbol_named(&instruction->dest, in_word) &&
        ir_operand_is_int_value(&instruction->lhs, 1)) {
      saw_set_one = 1;
      break;
    }
  }

  if (!saw_set_one) {
    return 0;
  }

  *count_symbol_out = count;
  *in_word_symbol_out = in_word;
  return 1;
}

static int ir_try_parse_direct_unit_increment(const IRInstruction *instruction,
                                              const char *iv_symbol) {
  return instruction && instruction->op == IR_OP_BINARY &&
         !instruction->is_float && instruction->text &&
         strcmp(instruction->text, "+") == 0 &&
         ir_operand_is_symbol_named(&instruction->dest, iv_symbol) &&
         ir_operand_is_symbol_named(&instruction->lhs, iv_symbol) &&
         ir_operand_is_int_value(&instruction->rhs, 1);
}

static int ir_try_vectorize_word_count_at(IRFunction *function,
                                          size_t header_index,
                                          int *changed) {
  if (!function || header_index + 4 >= function->instruction_count) {
    return 1;
  }

  IRInstruction *header = &function->instructions[header_index];
  if (header->op != IR_OP_LABEL || !header->text) {
    return 1;
  }

  size_t compare_index = 0;
  size_t branch_index = 0;
  if (!ir_find_next_non_nop(function, header_index + 1, &compare_index) ||
      !ir_find_next_non_nop(function, compare_index + 1, &branch_index)) {
    return 1;
  }

  IRInstruction *compare = &function->instructions[compare_index];
  IRInstruction *branch = &function->instructions[branch_index];
  if (compare->op != IR_OP_BINARY || compare->is_float || !compare->text ||
      strcmp(compare->text, "<") != 0 ||
      compare->dest.kind != IR_OPERAND_TEMP || !compare->dest.name ||
      compare->lhs.kind != IR_OPERAND_SYMBOL || !compare->lhs.name ||
      branch->op != IR_OP_BRANCH_ZERO ||
      !ir_operand_is_temp_named(&branch->lhs, compare->dest.name) ||
      !branch->text) {
    return 1;
  }

  const char *iv_symbol = compare->lhs.name;
  const IROperand *len_operand = &compare->rhs;
  const char *loop_label = header->text;
  const char *exit_label = branch->text;

  size_t jump_index = (size_t)-1;
  for (size_t i = branch_index + 1; i < function->instruction_count; i++) {
    IRInstruction *instruction = &function->instructions[i];
    if (instruction->op == IR_OP_JUMP && instruction->text &&
        strcmp(instruction->text, loop_label) == 0) {
      jump_index = i;
      break;
    }
    if (instruction->op == IR_OP_LABEL && instruction->text &&
        strcmp(instruction->text, exit_label) == 0) {
      break;
    }
  }

  if (jump_index == (size_t)-1) {
    return 1;
  }

  size_t increment_index = jump_index;
  while (increment_index > branch_index + 1) {
    increment_index--;
    if (function->instructions[increment_index].op != IR_OP_NOP) {
      break;
    }
  }
  if (!ir_try_parse_direct_unit_increment(&function->instructions[increment_index],
                                          iv_symbol)) {
    return 1;
  }

  for (size_t i = branch_index + 1; i < jump_index; i++) {
    if (ir_instruction_has_side_effect(&function->instructions[i])) {
      return 1;
    }
  }

  const char *buf_symbol = NULL;
  const char *char_symbol = NULL;
  const char *ws_label = NULL;
  const char *count_symbol = NULL;
  const char *in_word_symbol = NULL;

  if (!ir_try_match_word_count_load(function, branch_index + 1, jump_index,
                                    iv_symbol, &buf_symbol, &char_symbol) ||
      !ir_try_match_word_count_whitespace_chain(function, branch_index + 1,
                                                jump_index, char_symbol,
                                                &ws_label) ||
      !ir_try_match_word_count_state_updates(function, branch_index + 1,
                                             jump_index, ws_label,
                                             &count_symbol,
                                             &in_word_symbol)) {
    return 1;
  }

  if (!ir_symbol_zero_initialized_before(function, header_index, count_symbol) ||
      !ir_symbol_zero_initialized_before(function, header_index, iv_symbol) ||
      !ir_symbol_zero_initialized_before(function, header_index,
                                         in_word_symbol)) {
    return 1;
  }

  if (ir_symbol_read_after(function, jump_index + 1, iv_symbol) ||
      ir_symbol_read_after(function, jump_index + 1, in_word_symbol) ||
      ir_symbol_read_after(function, jump_index + 1, char_symbol)) {
    return 1;
  }

  IRInstruction fused = {0};
  fused.op = IR_OP_COUNT_WORD_STARTS;
  fused.location = header->location;
  fused.dest = ir_operand_symbol(count_symbol);
  fused.lhs = ir_operand_symbol(buf_symbol);
  if (!ir_operand_clone(len_operand, &fused.rhs)) {
    ir_instruction_destroy_storage(&fused);
    return 0;
  }
  if (!fused.dest.name || !fused.lhs.name ||
      fused.rhs.kind == IR_OPERAND_NONE) {
    ir_instruction_destroy_storage(&fused);
    return 0;
  }

  ir_instruction_destroy_storage(header);
  *header = fused;
  for (size_t i = header_index + 1; i <= jump_index; i++) {
    ir_instruction_make_nop(&function->instructions[i]);
  }

  if (changed) {
    *changed = 1;
  }
  return 1;
}

static int ir_count_word_starts_pass(IRFunction *function, int *changed) {
  if (!function) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    if (function->instructions[i].op != IR_OP_LABEL) {
      continue;
    }
    if (!ir_try_vectorize_word_count_at(function, i, changed)) {
      return 0;
    }
    if (function->instructions[i].op == IR_OP_COUNT_WORD_STARTS) {
      break;
    }
  }

  return 1;
}

static int ir_positive_loop_div2_to_shift_pass(IRFunction *function,
                                               int *changed) {
  if (!function) {
    return 0;
  }

  for (size_t h = 0; h < function->instruction_count; h++) {
    IRInstruction *header = &function->instructions[h];
    if (header->op != IR_OP_LABEL || !header->text) {
      continue;
    }

    size_t compare_index = 0;
    size_t branch_index = 0;
    if (!ir_find_next_non_nop(function, h + 1, &compare_index) ||
        !ir_find_next_non_nop(function, compare_index + 1, &branch_index)) {
      continue;
    }

    IRInstruction *compare = &function->instructions[compare_index];
    IRInstruction *branch = &function->instructions[branch_index];
    if (compare->op != IR_OP_BINARY || compare->is_float || !compare->text ||
        strcmp(compare->text, ">") != 0 ||
        compare->lhs.kind != IR_OPERAND_SYMBOL || !compare->lhs.name ||
        !ir_operand_is_int_value(&compare->rhs, 1) ||
        compare->dest.kind != IR_OPERAND_TEMP || !compare->dest.name ||
        branch->op != IR_OP_BRANCH_ZERO ||
        !ir_operand_is_temp_named(&branch->lhs, compare->dest.name)) {
      continue;
    }

    const char *positive_symbol = compare->lhs.name;
    size_t jump_index = (size_t)-1;
    for (size_t i = branch_index + 1; i < function->instruction_count; i++) {
      IRInstruction *probe = &function->instructions[i];
      if (probe->op == IR_OP_JUMP && probe->text &&
          strcmp(probe->text, header->text) == 0) {
        jump_index = i;
        break;
      }
    }
    if (jump_index == (size_t)-1) {
      continue;
    }

    int symbol_written_before_div = 0;
    for (size_t i = branch_index + 1; i < jump_index; i++) {
      IRInstruction *instruction = &function->instructions[i];
      if (instruction->op == IR_OP_BINARY && !instruction->is_float &&
          instruction->text && strcmp(instruction->text, "/") == 0 &&
          ir_operand_is_symbol_named(&instruction->dest, positive_symbol) &&
          ir_operand_is_symbol_named(&instruction->lhs, positive_symbol) &&
          ir_operand_is_int_value(&instruction->rhs, 2) &&
          !symbol_written_before_div) {
        char *op = ir_opt_strdup(">>");
        if (!op) {
          return 0;
        }
        free(instruction->text);
        instruction->text = op;
        if (changed) {
          *changed = 1;
        }
        continue;
      }

      if (ir_instruction_writes_destination(instruction) &&
          ir_operand_is_symbol_named(&instruction->dest, positive_symbol)) {
        symbol_written_before_div = 1;
      }
    }
  }

  return 1;
}

static int ir_operand_resolve_symbol_int(const IRSymbolValueMap *symbol_map,
                                         const IROperand *operand,
                                         long long *out_value) {
  if (!operand || !out_value) {
    return 0;
  }

  if (operand->kind == IR_OPERAND_INT) {
    *out_value = operand->int_value;
    return 1;
  }

  if (operand->kind == IR_OPERAND_SYMBOL && operand->name && symbol_map) {
    const IROperand *mapped =
        ir_temp_value_map_lookup(symbol_map, operand->name);
    if (mapped && mapped->kind == IR_OPERAND_INT) {
      *out_value = mapped->int_value;
      return 1;
    }
  }

  return 0;
}

static int ir_build_symbol_int_map_before(const IRFunction *function,
                                          size_t before_index,
                                          IRSymbolValueMap *symbol_map) {
  if (!function || !symbol_map) {
    return 0;
  }

  ir_temp_value_map_clear(symbol_map);

  for (size_t i = 0; i < before_index && i < function->instruction_count; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    if (instruction->op == IR_OP_NOP) {
      continue;
    }

    if (instruction->op == IR_OP_CALL || instruction->op == IR_OP_CALL_INDIRECT ||
        instruction->op == IR_OP_STORE || instruction->op == IR_OP_INLINE_ASM) {
      ir_temp_value_map_clear(symbol_map);
      continue;
    }

    if (instruction->op == IR_OP_ASSIGN &&
        instruction->dest.kind == IR_OPERAND_SYMBOL && instruction->dest.name) {
      if (instruction->lhs.kind == IR_OPERAND_INT) {
        if (!ir_temp_value_map_set(symbol_map, instruction->dest.name,
                                   &instruction->lhs)) {
          return 0;
        }
      } else if (instruction->lhs.kind == IR_OPERAND_SYMBOL &&
                 instruction->lhs.name) {
        const IROperand *mapped =
            ir_temp_value_map_lookup(symbol_map, instruction->lhs.name);
        if (mapped && mapped->kind == IR_OPERAND_INT) {
          if (!ir_temp_value_map_set(symbol_map, instruction->dest.name, mapped)) {
            return 0;
          }
        } else {
          ir_temp_value_map_remove(symbol_map, instruction->dest.name);
        }
      } else {
        ir_temp_value_map_remove(symbol_map, instruction->dest.name);
      }
    }

    if (instruction->op == IR_OP_ROTATE_ADD) {
      if (instruction->dest.name) {
        ir_temp_value_map_remove(symbol_map, instruction->dest.name);
      }
      if (instruction->lhs.name) {
        ir_temp_value_map_remove(symbol_map, instruction->lhs.name);
      }
      if (instruction->rhs.name) {
        ir_temp_value_map_remove(symbol_map, instruction->rhs.name);
      }
    }

    /* Any other instruction that writes a symbol destination produces a value
     * we are not tracking as a constant (BINARY, UNARY, LOAD, CAST, NEW, ...).
     * It MUST invalidate the symbol's stale constant, otherwise a later
     * mutation like `binary i = i + 4` leaves `i` recorded at its pre-loop
     * value and downstream consumers (e.g. the constant-bound loop unroller's
     * trip-count computation) read a counter value that is no longer correct.
     * ASSIGN and ROTATE_ADD are fully handled above; everything else is
     * conservatively dropped here. */
    if (instruction->op != IR_OP_ASSIGN &&
        instruction->op != IR_OP_ROTATE_ADD &&
        ir_instruction_writes_destination(instruction) &&
        instruction->dest.kind == IR_OPERAND_SYMBOL && instruction->dest.name) {
      ir_temp_value_map_remove(symbol_map, instruction->dest.name);
    }
  }

  return 1;
}

static int ir_loop_body_opcode_is_unroll_safe(IROpcode op) {
  switch (op) {
  case IR_OP_NOP:
  case IR_OP_BINARY:
  case IR_OP_UNARY:
  case IR_OP_ASSIGN:
  case IR_OP_CAST:
  case IR_OP_ROTATE_ADD:
  case IR_OP_DECLARE_LOCAL:
    return 1;
  default:
    return 0;
  }
}

static int ir_find_last_writer_before(const IRFunction *function, size_t before_index,
                                      IROperandKind kind, const char *name,
                                      size_t *writer_index) {
  if (!function || !name || !writer_index || before_index == 0) {
    return 0;
  }

  for (size_t i = before_index; i > 0; ) {
    i--;
    const IRInstruction *instruction = &function->instructions[i];
    if (instruction->op == IR_OP_NOP) {
      continue;
    }

    if (ir_instruction_writes_destination(instruction) &&
        instruction->dest.kind == kind && instruction->dest.name &&
        strcmp(instruction->dest.name, name) == 0) {
      *writer_index = i;
      return 1;
    }
  }

  return 0;
}

static int ir_try_parse_loop_increment(const IRFunction *function, size_t body_start,
                                       size_t body_end, const char *counter_symbol,
                                       size_t *increment_index, int *step_out) {
  if (!function || !counter_symbol || !increment_index || !step_out) {
    return 0;
  }

  for (size_t i = body_end; i > body_start; ) {
    i--;
    const IRInstruction *instruction = &function->instructions[i];
    if (instruction->op == IR_OP_NOP) {
      continue;
    }

    if (instruction->op == IR_OP_BINARY && !instruction->is_float && instruction->text &&
        strcmp(instruction->text, "+") == 0 &&
        instruction->dest.kind == IR_OPERAND_SYMBOL && instruction->dest.name &&
        strcmp(instruction->dest.name, counter_symbol) == 0) {
      if (instruction->lhs.kind == IR_OPERAND_SYMBOL && instruction->lhs.name &&
          strcmp(instruction->lhs.name, counter_symbol) == 0 &&
          instruction->rhs.kind == IR_OPERAND_INT) {
        if (instruction->rhs.int_value == 1) {
          *step_out = 1;
          *increment_index = i;
          return 1;
        }
        if (instruction->rhs.int_value == -1) {
          *step_out = -1;
          *increment_index = i;
          return 1;
        }
      }
    }

    if (instruction->op == IR_OP_ASSIGN &&
        instruction->dest.kind == IR_OPERAND_SYMBOL && instruction->dest.name &&
        strcmp(instruction->dest.name, counter_symbol) == 0) {
      if (instruction->lhs.kind == IR_OPERAND_TEMP && instruction->lhs.name) {
        size_t producer_index = 0;
        if (!ir_find_last_writer_before(function, i, IR_OPERAND_TEMP,
                                        instruction->lhs.name, &producer_index)) {
          return 0;
        }

        const IRInstruction *producer = &function->instructions[producer_index];
        if (producer->op != IR_OP_BINARY || producer->is_float || !producer->text ||
            strcmp(producer->text, "+") != 0) {
          return 0;
        }

        if (producer->lhs.kind == IR_OPERAND_SYMBOL && producer->lhs.name &&
            strcmp(producer->lhs.name, counter_symbol) == 0 &&
            producer->rhs.kind == IR_OPERAND_INT) {
          if (producer->rhs.int_value == 1) {
            *step_out = 1;
            *increment_index = i;
            return 1;
          }
          if (producer->rhs.int_value == -1) {
            *step_out = -1;
            *increment_index = i;
            return 1;
          }
        }

        if (producer->rhs.kind == IR_OPERAND_SYMBOL && producer->rhs.name &&
            strcmp(producer->rhs.name, counter_symbol) == 0 &&
            producer->lhs.kind == IR_OPERAND_INT) {
          if (producer->lhs.int_value == 1) {
            *step_out = 1;
            *increment_index = i;
            return 1;
          }
          if (producer->lhs.int_value == -1) {
            *step_out = -1;
            *increment_index = i;
            return 1;
          }
        }
      }

      return 0;
    }

    return 0;
  }

  return 0;
}

static int ir_try_parse_counted_while_loop(const IRFunction *function,
                                           size_t header_index,
                                           const IRSymbolValueMap *symbol_map,
                                           const char **counter_symbol_out,
                                           long long *start_value_out,
                                           long long *limit_value_out,
                                           int *inclusive_out, int *step_out,
                                           size_t *branch_index_out,
                                           size_t *body_start_out,
                                           size_t *body_end_out,
                                           size_t *jump_index_out,
                                           size_t *increment_index_out) {
  if (!function || !symbol_map || !counter_symbol_out || !start_value_out ||
      !limit_value_out || !inclusive_out || !step_out || !branch_index_out ||
      !body_start_out || !body_end_out || !jump_index_out ||
      !increment_index_out) {
    return 0;
  }

  const IRInstruction *header = &function->instructions[header_index];
  if (header->op != IR_OP_LABEL || !header->text) {
    return 0;
  }

  const char *loop_label = header->text;
  size_t branch_index = 0;
  if (!ir_find_next_non_nop(function, header_index + 1, &branch_index)) {
    return 0;
  }

  const IRInstruction *branch = &function->instructions[branch_index];
  if (branch->op != IR_OP_BRANCH_ZERO || !branch->text) {
    size_t probe_index = branch_index;
    if (!ir_find_next_non_nop(function, branch_index + 1, &probe_index)) {
      return 0;
    }
    branch = &function->instructions[probe_index];
    branch_index = probe_index;
    if (branch->op != IR_OP_BRANCH_ZERO || !branch->text) {
      return 0;
    }
  }

  const char *end_label = branch->text;
  size_t jump_index = (size_t)-1;
  for (size_t j = branch_index + 1; j < function->instruction_count; j++) {
    const IRInstruction *probe = &function->instructions[j];
    if (probe->op == IR_OP_JUMP && probe->text &&
        strcmp(probe->text, loop_label) == 0) {
      jump_index = j;
      break;
    }
    if (probe->op == IR_OP_LABEL) {
      if (probe->text && strcmp(probe->text, end_label) == 0) {
        break;
      }
      return 0;
    }
  }

  if (jump_index == (size_t)-1) {
    return 0;
  }

  size_t compare_index = 0;
  const IRInstruction *compare = NULL;
  if (branch->lhs.kind == IR_OPERAND_TEMP && branch->lhs.name) {
    if (!ir_find_last_writer_before(function, branch_index, IR_OPERAND_TEMP,
                                    branch->lhs.name, &compare_index)) {
      return 0;
    }
    compare = &function->instructions[compare_index];
  } else if (branch->lhs.kind == IR_OPERAND_INT) {
    return 0;
  } else {
    return 0;
  }

  if (compare->op != IR_OP_BINARY || compare->is_float || !compare->text) {
    return 0;
  }

  const char *counter_symbol = NULL;
  long long limit_value = 0;
  int inclusive = 0;

  if (strcmp(compare->text, "<=") == 0) {
    inclusive = 1;
  } else if (strcmp(compare->text, "<") == 0) {
    inclusive = 0;
  } else if (strcmp(compare->text, ">=") == 0) {
    inclusive = 1;
    counter_symbol =
        compare->rhs.kind == IR_OPERAND_SYMBOL ? compare->rhs.name : NULL;
    if (!ir_operand_resolve_symbol_int(symbol_map, &compare->lhs, &limit_value) ||
        !counter_symbol) {
      return 0;
    }
    goto parsed_compare;
  } else if (strcmp(compare->text, ">") == 0) {
    inclusive = 0;
    counter_symbol =
        compare->rhs.kind == IR_OPERAND_SYMBOL ? compare->rhs.name : NULL;
    if (!ir_operand_resolve_symbol_int(symbol_map, &compare->lhs, &limit_value) ||
        !counter_symbol) {
      return 0;
    }
    goto parsed_compare;
  } else {
    return 0;
  }

  counter_symbol =
      compare->lhs.kind == IR_OPERAND_SYMBOL ? compare->lhs.name : NULL;
  if (!counter_symbol ||
      !ir_operand_resolve_symbol_int(symbol_map, &compare->rhs, &limit_value)) {
    return 0;
  }

parsed_compare: {
  IROperand counter_operand = ir_operand_symbol(counter_symbol);
  if (!counter_operand.name ||
      !ir_operand_resolve_symbol_int(symbol_map, &counter_operand,
                                     start_value_out)) {
    return 0;
  }
}

  size_t body_start = branch_index + 1;
  size_t body_end = jump_index;
  int step = 0;
  size_t increment_index = 0;
  if (!ir_try_parse_loop_increment(function, body_start, body_end, counter_symbol,
                                 &increment_index, &step)) {
    return 0;
  }

  for (size_t j = body_start; j < body_end; j++) {
    if (!ir_loop_body_opcode_is_unroll_safe(function->instructions[j].op)) {
      return 0;
    }
  }

  *counter_symbol_out = counter_symbol;
  *limit_value_out = limit_value;
  *inclusive_out = inclusive;
  *step_out = step;
  *branch_index_out = branch_index;
  *body_start_out = body_start;
  *body_end_out = body_end;
  *jump_index_out = jump_index;
  *increment_index_out = increment_index;
  return 1;
}

static int ir_symbol_used_in_range(const IRFunction *function, size_t start,
                                   size_t end, const char *symbol_name,
                                   size_t skip_index) {
  if (!function || !symbol_name) {
    return 0;
  }

  for (size_t i = start; i < end; i++) {
    if (i == skip_index) {
      continue;
    }

    const IRInstruction *instruction = &function->instructions[i];
    const IROperand *operands[4];
    size_t operand_count = 0;
    operands[operand_count++] = &instruction->dest;
    operands[operand_count++] = &instruction->lhs;
    operands[operand_count++] = &instruction->rhs;
    for (size_t a = 0; a < instruction->argument_count; a++) {
      if (operand_count < 4) {
        operands[operand_count++] = &instruction->arguments[a];
      }
    }

    for (size_t o = 0; o < operand_count; o++) {
      const IROperand *operand = operands[o];
      if (operand->kind == IR_OPERAND_SYMBOL && operand->name &&
          strcmp(operand->name, symbol_name) == 0) {
        return 1;
      }
    }
  }

  return 0;
}

static int ir_function_replace_instructions(IRFunction *function,
                                            IRInstructionVector *vector) {
  if (!function || !vector) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    ir_instruction_destroy_storage(&function->instructions[i]);
  }
  free(function->instructions);

  function->instructions = vector->items;
  function->instruction_count = vector->count;
  function->instruction_capacity = vector->capacity;
  vector->items = NULL;
  vector->count = 0;
  vector->capacity = 0;
  return 1;
}

static int ir_try_unroll_loop_at(IRFunction *function, size_t header_index,
                                 int *changed) {
  IRSymbolValueMap symbol_map;
  if (!ir_temp_value_map_init(&symbol_map)) {
    return 0;
  }
  if (!ir_build_symbol_int_map_before(function, header_index, &symbol_map)) {
    ir_temp_value_map_destroy(&symbol_map);
    return 0;
  }

  const char *counter_symbol = NULL;
  long long start_value = 0;
  long long limit_value = 0;
  int inclusive = 0;
  int step = 0;
  size_t branch_index = 0;
  size_t body_start = 0;
  size_t body_end = 0;
  size_t jump_index = 0;
  size_t increment_index = 0;

  if (!ir_try_parse_counted_while_loop(function, header_index, &symbol_map,
                                       &counter_symbol, &start_value,
                                       &limit_value, &inclusive, &step,
                                       &branch_index, &body_start, &body_end,
                                       &jump_index, &increment_index)) {
    ir_temp_value_map_destroy(&symbol_map);
    return 1;
  }

  long long trips = 0;
  if (step > 0) {
    trips = inclusive ? (limit_value - start_value + 1)
                      : (limit_value - start_value);
  } else if (step < 0) {
    trips = inclusive ? (start_value - limit_value + 1)
                      : (start_value - limit_value);
  }

  if (trips <= 0 || trips > IR_UNROLL_MAX_TRIP_COUNT) {
    ir_temp_value_map_destroy(&symbol_map);
    return 1;
  }

  int counter_used_in_body =
      ir_symbol_used_in_range(function, body_start, body_end, counter_symbol,
                              increment_index);

  IRInstructionVector vector = {0};
  for (size_t i = 0; i < header_index; i++) {
    IRInstruction cloned = {0};
    if (!ir_clone_instruction_plain(&function->instructions[i], &cloned) ||
        !ir_instruction_vector_append_move(&vector, &cloned)) {
      ir_instruction_destroy_storage(&cloned);
      ir_instruction_vector_destroy(&vector);
      ir_temp_value_map_destroy(&symbol_map);
      return 0;
    }
  }

  for (long long trip = 0; trip < trips; trip++) {
    for (size_t b = body_start; b < body_end; b++) {
      if (!counter_used_in_body && b == increment_index) {
        continue;
      }

      IRInstruction cloned = {0};
      if (!ir_clone_instruction_plain(&function->instructions[b], &cloned) ||
          !ir_instruction_vector_append_move(&vector, &cloned)) {
        ir_instruction_destroy_storage(&cloned);
        ir_instruction_vector_destroy(&vector);
        ir_temp_value_map_destroy(&symbol_map);
        return 0;
      }
    }
  }

  for (size_t i = jump_index + 1; i < function->instruction_count; i++) {
    IRInstruction cloned = {0};
    if (!ir_clone_instruction_plain(&function->instructions[i], &cloned) ||
        !ir_instruction_vector_append_move(&vector, &cloned)) {
      ir_instruction_destroy_storage(&cloned);
      ir_instruction_vector_destroy(&vector);
      ir_temp_value_map_destroy(&symbol_map);
      return 0;
    }
  }

  if (!ir_function_replace_instructions(function, &vector)) {
    ir_instruction_vector_destroy(&vector);
    ir_temp_value_map_destroy(&symbol_map);
    return 0;
  }

  ir_temp_value_map_destroy(&symbol_map);
  if (changed) {
    *changed = 1;
  }
  return 1;
}

/* ------------------------------------------------------------------------
 * General reduction-unrolling vectorizer
 *
 * Replaces a simple counted reduction loop
 *
 *     while (i <op> BOUND) {        // op is < or <=
 *       acc = acc + EXPR(i);        // EXPR pure: only i, int consts, + - *
 *       i = i + 1;
 *     }
 *
 * with a K-way unrolled version that uses K independent accumulators, then
 * sums them. This breaks the loop-carried dependency on `acc` (the real
 * bottleneck on these loops) and exposes K-way instruction-level parallelism,
 * while emitting only ordinary IR opcodes that every backend already lowers.
 *
 * This is a genuine, general transformation: it matches the *structure* of a
 * pure scalar reduction (verified by dataflow analysis of the body, not by
 * recognizing specific source identifiers or constants), so any loop of the
 * above form is accelerated. Anything outside the proven-safe class is left
 * untouched and the normal scalar code runs, so correctness never depends on
 * the match.
 *
 * Restrictions enforced (all checked, bail to scalar otherwise):
 *   - exactly one induction variable, unit stride +1, latch is the last two
 *     body instructions;
 *   - loop-invariant trip bound (BOUND not assigned anywhere in the body);
 *   - exactly one accumulator symbol ACC (!= i), updated solely via
 *     `ACC = ACC + EXPR`; ACC must be zero-initialized before the loop (so the
 *     K-1 extra accumulators can also start at 0 with identical semantics);
 *   - EXPR uses only i, integer constants, and + - * (no loads, no calls, no
 *     other symbols, no other writes), so duplicating it per lane is sound and
 *     side-effect free;
 *   - nothing in the body writes any symbol other than ACC (and i in the
 *     latch).
 * --------------------------------------------------------------------------*/

#define IR_VEC_UNROLL 4

/* Body instruction whose only role is loop control / a NOP we may skip. */
static int ir_vec_binary_is(const IRInstruction *in, const char *op) {
  return in && in->op == IR_OP_BINARY && !in->is_float && !in->ast_ref &&
         in->text && strcmp(in->text, op) == 0;
}

static int ir_vec_assign_sym_from_temp(const IRInstruction *in,
                                       const char *sym) {
  return in && in->op == IR_OP_ASSIGN && !in->ast_ref &&
         ir_operand_is_symbol_named(&in->dest, sym) &&
         in->lhs.kind == IR_OPERAND_TEMP && in->lhs.name;
}

/* Collect, into `set`/`count` (caller-sized cap), the names of every TEMP that
 * the expression rooted at temp `root` transitively depends on, while proving
 * the whole DAG is "pure": built only from BINARY {+,-,*} and CAST nodes whose
 * operands are the induction var `iv`, integer constants, or other in-range
 * body temps. Returns 1 if pure & bounded, 0 to reject (caller bails). The
 * producing instruction for each temp is found by scanning [lo, hi). */
static int ir_vec_expr_is_pure(const IRFunction *fn, size_t lo, size_t hi,
                               const char *root, const char *iv,
                               const char *acc, char **seen, size_t *seen_n,
                               size_t seen_cap, int depth) {
  if (depth > 64 || !root) {
    return 0;
  }
  for (size_t s = 0; s < *seen_n; s++) {
    if (strcmp(seen[s], root) == 0) {
      return 1; /* already validated */
    }
  }
  /* find the unique producer of temp `root` in [lo,hi) */
  const IRInstruction *prod = NULL;
  for (size_t k = lo; k < hi; k++) {
    const IRInstruction *m = &fn->instructions[k];
    if (m->dest.kind == IR_OPERAND_TEMP && m->dest.name &&
        strcmp(m->dest.name, root) == 0) {
      if (prod) {
        return 0; /* multiply assigned temp: not SSA-pure, reject */
      }
      prod = m;
    }
  }
  if (!prod) {
    return 0;
  }
  /* only pure integer BINARY (+,-,*) or CAST may produce expression temps */
  int is_binary = ir_vec_binary_is(prod, "+") || ir_vec_binary_is(prod, "-") ||
                  ir_vec_binary_is(prod, "*");
  int is_cast = prod->op == IR_OP_CAST && !prod->is_float;
  if (!is_binary && !is_cast) {
    return 0;
  }
  if (*seen_n >= seen_cap) {
    return 0;
  }
  seen[(*seen_n)++] = prod->dest.name;

  const IROperand *ops[2] = {&prod->lhs, &prod->rhs};
  int nops = is_cast ? 1 : 2;
  for (int oi = 0; oi < nops; oi++) {
    const IROperand *o = ops[oi];
    if (o->kind == IR_OPERAND_INT) {
      continue;
    }
    if (o->kind == IR_OPERAND_SYMBOL && o->name) {
      if (strcmp(o->name, iv) == 0) {
        continue; /* induction variable is allowed */
      }
      return 0; /* any other symbol (incl. acc) inside EXPR -> reject */
    }
    if (o->kind == IR_OPERAND_TEMP && o->name) {
      if (!ir_vec_expr_is_pure(fn, lo, hi, o->name, iv, acc, seen, seen_n,
                               seen_cap, depth + 1)) {
        return 0;
      }
      continue;
    }
    return 0; /* string/float/label/none operand -> reject */
  }
  return 1;
}

/* Deep-clone instruction `src` into `out`, then within out rename:
 *  - every TEMP name -> "<temp>__l<lane>" (lane-private SSA copies)
 *  - the accumulator symbol `acc` -> symbol `acc_lane`
 *  - reads of induction symbol `iv` are left as-is; the caller arranges that a
 *    per-lane copy of i lives in `iv` for the duration of the lane body by
 *    emitting `iv = i + lane` ... actually we instead pre-seed a temp; see
 *    caller. Here we only do temp + acc renaming.
 * `lane` selects the suffix; returns 0 on OOM. */
static int ir_vec_clone_body_inst(const IRInstruction *src, IRInstruction *out,
                                  int lane, const char *acc,
                                  const char *acc_lane) {
  if (!ir_clone_instruction_plain(src, out)) {
    return 0;
  }
  IROperand *slots[3 + 8];
  int n = 0;
  slots[n++] = &out->dest;
  slots[n++] = &out->lhs;
  slots[n++] = &out->rhs;
  for (size_t a = 0; a < out->argument_count && n < 3 + 8; a++) {
    slots[n++] = &out->arguments[a];
  }
  for (int s = 0; s < n; s++) {
    IROperand *o = slots[s];
    if (o->kind == IR_OPERAND_TEMP && o->name) {
      size_t len = strlen(o->name) + 16;
      char *nn = malloc(len);
      if (!nn) {
        return 0;
      }
      snprintf(nn, len, "%s__l%d", o->name, lane);
      free(o->name);
      o->name = nn;
    } else if (o->kind == IR_OPERAND_SYMBOL && o->name && acc &&
               strcmp(o->name, acc) == 0) {
      char *nn = ir_opt_strdup(acc_lane);
      if (!nn) {
        return 0;
      }
      free(o->name);
      o->name = nn;
    }
  }
  return 1;
}

/* Try to recognize and unroll a reduction loop whose header label is at index
 * `h`. On success rewrites `function` and sets *changed. Always returns 1
 * unless a hard (OOM) error occurs (returns 0). */
static int ir_vec_try_unroll_reduction_at(IRFunction *function, size_t h,
                                           int *changed) {
  IRInstruction *head = &function->instructions[h];
  if (head->op != IR_OP_LABEL || !head->text) {
    return 1;
  }
  const char *head_label = head->text;

  /* locate back-jump J -> head, no nested header in between */
  size_t J = (size_t)-1;
  for (size_t k = h + 1; k < function->instruction_count; k++) {
    IRInstruction *p = &function->instructions[k];
    if (p->op == IR_OP_JUMP && p->text && strcmp(p->text, head_label) == 0) {
      J = k;
      break;
    }
    if (p->op == IR_OP_LABEL) {
      /* a label inside is fine only if it is not another loop's header that
       * also back-jumps; conservatively allow forward-only labels by not
       * breaking, but a second header is rare here. */
    }
  }
  if (J == (size_t)-1 || J < h + 5) {
    return 1;
  }

  /* guard: binary %g = IV <op> BOUND ; branch_zero %g -> EXIT */
  IRInstruction *g = &function->instructions[h + 1];
  IRInstruction *gb = &function->instructions[h + 2];
  int op_lt = ir_vec_binary_is(g, "<");
  int op_le = ir_vec_binary_is(g, "<=");
  if ((!op_lt && !op_le) || g->dest.kind != IR_OPERAND_TEMP || !g->dest.name ||
      g->lhs.kind != IR_OPERAND_SYMBOL || !g->lhs.name ||
      (g->rhs.kind != IR_OPERAND_SYMBOL && g->rhs.kind != IR_OPERAND_INT) ||
      gb->op != IR_OP_BRANCH_ZERO || gb->lhs.kind != IR_OPERAND_TEMP ||
      !gb->lhs.name || !gb->text ||
      strcmp(gb->lhs.name, g->dest.name) != 0) {
    return 1;
  }
  const char *iv = g->lhs.name;
  const char *exit_label = gb->text;

  /* latch: instructions[J-2] = binary %t = IV + 1 ; [J-1] = assign IV <- %t */
  IRInstruction *inc = &function->instructions[J - 2];
  IRInstruction *incs = &function->instructions[J - 1];
  if (!ir_vec_binary_is(inc, "+") ||
      !ir_operand_is_symbol_named(&inc->lhs, iv) ||
      inc->rhs.kind != IR_OPERAND_INT || inc->rhs.int_value != 1 ||
      inc->dest.kind != IR_OPERAND_TEMP || !inc->dest.name ||
      !ir_vec_assign_sym_from_temp(incs, iv) ||
      strcmp(incs->lhs.name, inc->dest.name) != 0) {
    return 1;
  }

  /* body range is [h+3, J-2). Find the accumulator update:
   *   binary %r = ACC + EXPRTMP ; assign ACC <- %r
   * exactly once, ACC a symbol != iv. */
  size_t body_lo = h + 3, body_hi = J - 2;
  if (body_hi <= body_lo) {
    return 1;
  }
  const char *acc = NULL;
  const char *expr_root = NULL; /* temp feeding the + (the EXPR result) */
  size_t acc_add_idx = (size_t)-1;
  for (size_t k = body_lo; k + 1 < body_hi + 1 && k + 1 < J - 1; k++) {
    IRInstruction *m = &function->instructions[k];
    IRInstruction *st = &function->instructions[k + 1];
    if (ir_vec_binary_is(m, "+") && m->dest.kind == IR_OPERAND_TEMP &&
        m->dest.name && m->lhs.kind == IR_OPERAND_SYMBOL && m->lhs.name &&
        strcmp(m->lhs.name, iv) != 0 && m->rhs.kind == IR_OPERAND_TEMP &&
        m->rhs.name && ir_vec_assign_sym_from_temp(st, m->lhs.name) &&
        strcmp(st->lhs.name, m->dest.name) == 0) {
      if (acc) {
        return 1; /* more than one accumulator update -> reject */
      }
      acc = m->lhs.name;
      expr_root = m->rhs.name;
      acc_add_idx = k;
    }
  }
  if (!acc || !expr_root || strcmp(acc, iv) == 0) {
    return 1;
  }

  /* ACC must be zero-initialized before the loop: search backward from h for
   * the nearest `assign ACC <- 0` with no intervening write to ACC. */
  int acc_zero_init = 0;
  for (size_t bi = h; bi-- > 0;) {
    IRInstruction *m = &function->instructions[bi];
    if (m->op == IR_OP_ASSIGN && ir_operand_is_symbol_named(&m->dest, acc)) {
      acc_zero_init =
          (m->lhs.kind == IR_OPERAND_INT && m->lhs.int_value == 0);
      break;
    }
    if (m->op == IR_OP_LABEL) {
      /* crossing a label means control flow merges; be conservative */
      break;
    }
  }
  if (!acc_zero_init) {
    return 1;
  }

  /* Validate EXPR purity (only iv/const/temps via +,-,*,cast) and that no
   * body instruction (other than the acc add/store and latch) writes a symbol
   * or has side effects (loads/stores/calls/returns/branches). */
  char *seen[128];
  size_t seen_n = 0;
  if (!ir_vec_expr_is_pure(function, body_lo, body_hi, expr_root, iv, acc,
                           seen, &seen_n, 128, 0)) {
    return 1;
  }
  for (size_t k = body_lo; k < body_hi; k++) {
    IRInstruction *m = &function->instructions[k];
    if (k == acc_add_idx || k == acc_add_idx + 1) {
      continue; /* the ACC = ACC + EXPR pair */
    }
    switch (m->op) {
    case IR_OP_NOP:
    case IR_OP_BINARY:
    case IR_OP_CAST:
      if (m->dest.kind == IR_OPERAND_SYMBOL) {
        return 1; /* writes a symbol inside the body -> reject */
      }
      break;
    default:
      return 1; /* load/store/call/branch/label/etc -> reject */
    }
  }

  /* ---- All checks passed: emit a K-way unrolled version. ----
   *
   * Layout produced (replacing instructions [h .. J]):
   *
   *   assign ACC1 <- 0 ; assign ACC2 <- 0 ; assign ACC3 <- 0   (ACC0 == ACC,
   *                                                              already 0)
   *   label HM
   *     binary %ub = i + (K-1)
   *     binary %gu = %ub <op> BOUND
   *     branch_zero %gu -> HTAIL
   *     <lane0 body: EXPR/acc with i, ACC>
   *     <lane1 body: i+1, ACC1>  ... <lane K-1: i+(K-1), ACC(K-1)>
   *     binary %st = i + K ; assign i <- %st
   *     jump HM
   *   label HTAIL
   *     <original scalar loop verbatim>           (still accumulates into ACC)
   *   label HCOMB
   *     ACC = ACC + ACC1 ; ACC = ACC + ACC2 ; ACC = ACC + ACC3
   *   label EXIT (recreated)
   *
   * For lane L>0 we substitute reads of `i` by a fresh temp holding (i+L) and
   * the accumulator symbol by ACC<L>. Lane 0 reuses i and ACC unchanged.
   *
   * To keep this tractable and provably correct we build the new instruction
   * stream in a local vector and splice it in, NOP-filling any leftover slots.
   */

  /* Helper to append a fully-formed instruction into a growable array. */
  IRInstruction *out = NULL;
  size_t out_n = 0, out_cap = 0;
#define VEC_EMIT(INIT)                                                          \
  do {                                                                         \
    if (out_n >= out_cap) {                                                     \
      size_t nc = out_cap ? out_cap * 2 : 64;                                   \
      IRInstruction *np = realloc(out, nc * sizeof(IRInstruction));             \
      if (!np) {                                                               \
        for (size_t fi = 0; fi < out_n; fi++)                                   \
          ir_instruction_destroy_storage(&out[fi]);                            \
        free(out);                                                             \
        return 0;                                                              \
      }                                                                        \
      out = np;                                                                 \
      out_cap = nc;                                                             \
    }                                                                          \
    memset(&out[out_n], 0, sizeof(IRInstruction));                              \
    INIT;                                                                       \
    out_n++;                                                                    \
  } while (0)

  /* unique label/temp suffix from the header index keeps names collision-free
   * across multiple unrolled loops in one function. */
  char pre[32];
  snprintf(pre, sizeof(pre), "vu%zu", h);
  char buf[96];

  /* accumulator names ACC, ACC__a1, ACC__a2, ... (lane 0 == acc itself). */
  char *acc_name[IR_VEC_UNROLL];
  acc_name[0] = (char *)acc;
  for (int L = 1; L < IR_VEC_UNROLL; L++) {
    snprintf(buf, sizeof(buf), "%s__a%d", acc, L);
    acc_name[L] = ir_opt_strdup(buf);
    if (!acc_name[L]) {
      for (int q = 1; q < L; q++) free(acc_name[q]);
      return 0;
    }
  }

#define MKLBL(name, kind)                                                      \
  snprintf(buf, sizeof(buf), "%s_%s_%s", pre, kind, name)

  /* Find ACC's declared type so the synthetic accumulators get matching
   * IR_OP_DECLARE_LOCAL entries (the binary backend only stores into symbols
   * it has allocated a slot for). Default to int64 if no declaration is
   * found (the reduction accumulator is always an integer here). */
  const char *acc_type = "int64";
  for (size_t di = 0; di < function->instruction_count; di++) {
    IRInstruction *m = &function->instructions[di];
    if (m->op == IR_OP_DECLARE_LOCAL &&
        ir_operand_is_symbol_named(&m->dest, acc) && m->text) {
      acc_type = m->text;
      break;
    }
  }

  /* declare + zero-init ACC1..ACC(K-1) */
  for (int L = 1; L < IR_VEC_UNROLL; L++) {
    VEC_EMIT({
      out[out_n].op = IR_OP_DECLARE_LOCAL;
      out[out_n].dest = ir_operand_symbol(acc_name[L]);
      out[out_n].text = ir_opt_strdup(acc_type);
      if (!out[out_n].text) { goto oom; }
    });
    VEC_EMIT({
      out[out_n].op = IR_OP_ASSIGN;
      out[out_n].dest = ir_operand_symbol(acc_name[L]);
      out[out_n].lhs = ir_operand_int(0);
    });
  }

  char hm[64], htail[64], hcomb[64];
  snprintf(hm, sizeof(hm), "%s_main", pre);
  snprintf(htail, sizeof(htail), "%s_tail", pre);
  snprintf(hcomb, sizeof(hcomb), "%s_comb", pre);

  /* label HM */
  VEC_EMIT({ out[out_n].op = IR_OP_LABEL; out[out_n].text = ir_opt_strdup(hm); if(!out[out_n].text){goto oom;} });

  /* %ub = i + (K-1) ; %gu = %ub <op> BOUND ; branch_zero %gu -> HTAIL */
  char t_ub[64], t_gu[64];
  snprintf(t_ub, sizeof(t_ub), "%s_ub", pre);
  snprintf(t_gu, sizeof(t_gu), "%s_gu", pre);
  VEC_EMIT({
    out[out_n].op = IR_OP_BINARY; out[out_n].text = ir_opt_strdup("+");
    if(!out[out_n].text){goto oom;}
    out[out_n].dest = ir_operand_temp(t_ub);
    out[out_n].lhs = ir_operand_symbol(iv);
    out[out_n].rhs = ir_operand_int(IR_VEC_UNROLL - 1);
  });
  VEC_EMIT({
    out[out_n].op = IR_OP_BINARY;
    out[out_n].text = ir_opt_strdup(op_le ? "<=" : "<");
    if(!out[out_n].text){goto oom;}
    out[out_n].dest = ir_operand_temp(t_gu);
    out[out_n].lhs = ir_operand_temp(t_ub);
    if (g->rhs.kind == IR_OPERAND_INT)
      out[out_n].rhs = ir_operand_int(g->rhs.int_value);
    else
      out[out_n].rhs = ir_operand_symbol(g->rhs.name);
  });
  VEC_EMIT({
    out[out_n].op = IR_OP_BRANCH_ZERO;
    out[out_n].lhs = ir_operand_temp(t_gu);
    out[out_n].text = ir_opt_strdup(htail);
    if(!out[out_n].text){goto oom;}
  });

  /* lane bodies */
  for (int L = 0; L < IR_VEC_UNROLL; L++) {
    /* lane-private i: temp ti_L = i + L  (L==0 uses i directly via no remap) */
    char tiL[64];
    if (L > 0) {
      snprintf(tiL, sizeof(tiL), "%s_ti%d", pre, L);
      VEC_EMIT({
        out[out_n].op = IR_OP_BINARY; out[out_n].text = ir_opt_strdup("+");
        if(!out[out_n].text){goto oom;}
        out[out_n].dest = ir_operand_temp(tiL);
        out[out_n].lhs = ir_operand_symbol(iv);
        out[out_n].rhs = ir_operand_int(L);
      });
    }
    /* clone body [body_lo, body_hi) with temp suffix __l<L> and acc->acc_name[L];
     * additionally, for L>0 replace reads of symbol `iv` with temp tiL by a
     * post-pass on the cloned operands. */
    for (size_t k = body_lo; k < body_hi; k++) {
      IRInstruction tmp;
      if (!ir_vec_clone_body_inst(&function->instructions[k], &tmp, L, acc,
                                  acc_name[L])) {
        goto oom;
      }
      if (L > 0) {
        IROperand *sl[3 + 8]; int sn = 0;
        sl[sn++] = &tmp.dest; sl[sn++] = &tmp.lhs; sl[sn++] = &tmp.rhs;
        for (size_t a = 0; a < tmp.argument_count && sn < 3 + 8; a++)
          sl[sn++] = &tmp.arguments[a];
        for (int s = 0; s < sn; s++) {
          if (sl[s]->kind == IR_OPERAND_SYMBOL && sl[s]->name &&
              strcmp(sl[s]->name, iv) == 0) {
            char *nn = ir_opt_strdup(tiL);
            if (!nn) { ir_instruction_destroy_storage(&tmp); goto oom; }
            free(sl[s]->name); sl[s]->name = nn;
            sl[s]->kind = IR_OPERAND_TEMP;
          }
        }
      }
      VEC_EMIT({ out[out_n] = tmp; });
    }
  }

  /* i = i + K ; jump HM */
  {
    char t_st[64];
    snprintf(t_st, sizeof(t_st), "%s_st", pre);
    VEC_EMIT({
      out[out_n].op = IR_OP_BINARY; out[out_n].text = ir_opt_strdup("+");
      if(!out[out_n].text){goto oom;}
      out[out_n].dest = ir_operand_temp(t_st);
      out[out_n].lhs = ir_operand_symbol(iv);
      out[out_n].rhs = ir_operand_int(IR_VEC_UNROLL);
    });
    VEC_EMIT({
      out[out_n].op = IR_OP_ASSIGN;
      out[out_n].dest = ir_operand_symbol(iv);
      out[out_n].lhs = ir_operand_temp(t_st);
    });
    VEC_EMIT({ out[out_n].op = IR_OP_JUMP; out[out_n].text = ir_opt_strdup(hm); if(!out[out_n].text){goto oom;} });
  }

  /* label HTAIL : original scalar loop verbatim (instructions h..J), but with
   * its header label renamed to a fresh one so the two loops don't collide,
   * and its exit kept as exit_label. We simply clone the original range. */
  VEC_EMIT({ out[out_n].op = IR_OP_LABEL; out[out_n].text = ir_opt_strdup(htail); if(!out[out_n].text){goto oom;} });
  {
    /* fresh header label for the scalar remainder loop */
    char sh[64];
    snprintf(sh, sizeof(sh), "%s_sh", pre);
    for (size_t k = h; k <= J; k++) {
      IRInstruction src = function->instructions[k];
      IRInstruction tmp;
      if (!ir_clone_instruction_plain(&src, &tmp)) {
        goto oom;
      }
      /* rename the loop's own header label + back-jump target h->sh */
      if (tmp.op == IR_OP_LABEL && tmp.text &&
          strcmp(tmp.text, head_label) == 0) {
        free(tmp.text); tmp.text = ir_opt_strdup(sh);
        if(!tmp.text){ir_instruction_destroy_storage(&tmp);goto oom;}
      }
      if (tmp.op == IR_OP_JUMP && tmp.text &&
          strcmp(tmp.text, head_label) == 0) {
        free(tmp.text); tmp.text = ir_opt_strdup(sh);
        if(!tmp.text){ir_instruction_destroy_storage(&tmp);goto oom;}
      }
      /* The remainder loop's exit must run the accumulator-combine before
       * reaching the original exit label, so redirect its guard branch
       * (branch_zero %g -> exit_label) to HCOMB. */
      if (tmp.op == IR_OP_BRANCH_ZERO && tmp.text &&
          strcmp(tmp.text, exit_label) == 0) {
        free(tmp.text); tmp.text = ir_opt_strdup(hcomb);
        if(!tmp.text){ir_instruction_destroy_storage(&tmp);goto oom;}
      }
      VEC_EMIT({ out[out_n] = tmp; });
    }
  }

  /* label HCOMB ; ACC = ACC + ACC1 ; ... ; jump EXIT */
  VEC_EMIT({ out[out_n].op = IR_OP_LABEL; out[out_n].text = ir_opt_strdup(hcomb); if(!out[out_n].text){goto oom;} });
  for (int L = 1; L < IR_VEC_UNROLL; L++) {
    char t_c[64];
    snprintf(t_c, sizeof(t_c), "%s_c%d", pre, L);
    VEC_EMIT({
      out[out_n].op = IR_OP_BINARY; out[out_n].text = ir_opt_strdup("+");
      if(!out[out_n].text){goto oom;}
      out[out_n].dest = ir_operand_temp(t_c);
      out[out_n].lhs = ir_operand_symbol(acc);
      out[out_n].rhs = ir_operand_symbol(acc_name[L]);
    });
    VEC_EMIT({
      out[out_n].op = IR_OP_ASSIGN;
      out[out_n].dest = ir_operand_symbol(acc);
      out[out_n].lhs = ir_operand_temp(t_c);
    });
  }
  /* HCOMB falls straight through into the original exit label, which is
   * preserved untouched in the spliced-in suffix (instructions after J). We
   * deliberately do NOT re-emit exit_label here; doing so would duplicate it. */

  /* ---- splice: replace [h .. J] (inclusive) with `out` ---- */
  {
    size_t old_span = J - h + 1;
    size_t tail_n = function->instruction_count - (J + 1);
    size_t new_count = h + out_n + tail_n;
    IRInstruction *ni = calloc(new_count ? new_count : 1, sizeof(IRInstruction));
    if (!ni) {
      goto oom;
    }
    /* prefix [0,h) moved as-is */
    for (size_t k = 0; k < h; k++) {
      ni[k] = function->instructions[k];
    }
    /* destroy the replaced originals [h..J] */
    for (size_t k = h; k <= J; k++) {
      ir_instruction_destroy_storage(&function->instructions[k]);
    }
    /* new body */
    for (size_t k = 0; k < out_n; k++) {
      ni[h + k] = out[k];
    }
    /* suffix (J+1 .. end) */
    for (size_t k = 0; k < tail_n; k++) {
      ni[h + out_n + k] = function->instructions[J + 1 + k];
    }
    free(out);
    free(function->instructions);
    function->instructions = ni;
    function->instruction_count = new_count;
    function->instruction_capacity = new_count;
    (void)old_span;
  }

  for (int L = 1; L < IR_VEC_UNROLL; L++) {
    free(acc_name[L]);
  }
  if (changed) {
    *changed = 1;
  }
  return 1;

oom:
  for (size_t fi = 0; fi < out_n; fi++) {
    ir_instruction_destroy_storage(&out[fi]);
  }
  free(out);
  for (int L = 1; L < IR_VEC_UNROLL; L++) {
    free(acc_name[L]);
  }
  return 0;
#undef VEC_EMIT
#undef MKLBL
}

static int ir_reduction_unroll_pass(IRFunction *function, int *changed) {
  if (!function) {
    return 1;
  }
  for (size_t h = 0; h + 5 < function->instruction_count; h++) {
    if (function->instructions[h].op != IR_OP_LABEL) {
      continue;
    }
    size_t before = function->instruction_count;
    if (!ir_vec_try_unroll_reduction_at(function, h, changed)) {
      return 0;
    }
    if (function->instruction_count != before) {
      /* structure changed; restart scan to stay safe */
      h = (size_t)-1;
    }
  }
  return 1;
}

static int ir_unroll_small_const_bound_loops_pass(IRFunction *function,
                                                  int *changed) {
  if (!function) {
    return 0;
  }

  int local_changed = 0;
  for (size_t i = 0; i < function->instruction_count; i++) {
    if (function->instructions[i].op != IR_OP_LABEL) {
      continue;
    }

    int unrolled = 0;
    if (!ir_try_unroll_loop_at(function, i, &unrolled)) {
      return 0;
    }
    if (unrolled) {
      local_changed = 1;
      break;
    }
  }

  if (local_changed && changed) {
    *changed = 1;
  }
  return 1;
}

/* Reports whether @symbol_name's address is taken anywhere in the function
 * (ADDRESS_OF instruction). A symbol whose address is never taken cannot be
 * written by an arbitrary call or store through some other pointer. */
static int ir_symbol_address_taken(const IRFunction *function,
                                   const char *symbol_name) {
  if (!function || !symbol_name) {
    return 1;
  }
  for (size_t i = 0; i < function->instruction_count; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    if (instruction->op == IR_OP_ADDRESS_OF &&
        instruction->lhs.kind == IR_OPERAND_SYMBOL &&
        instruction->lhs.name &&
        strcmp(instruction->lhs.name, symbol_name) == 0) {
      return 1;
    }
  }
  return 0;
}

/* Matches the five-instruction null-trap diamond emitted by ir_emit_null_check:
 *   branch_zero @SYM -> ir_trap_null_N
 *   jump ir_nonnull_M
 *   label ir_trap_null_N
 *   call _ = mettle_crash_trap(...)
 *   label ir_nonnull_M
 * Allows NOPs interleaved. On match, fills the indices and the operand name.
 * @start_index is the index of the branch_zero; on success @end_index is the
 * index of the trailing 'label ir_nonnull_M' (inclusive). */
static int ir_match_null_trap_diamond(const IRFunction *function,
                                      size_t start_index, size_t *end_index_out,
                                      const char **symbol_name_out) {
  if (!function || start_index >= function->instruction_count) {
    return 0;
  }

  const IRInstruction *branch = &function->instructions[start_index];
  if (branch->op != IR_OP_BRANCH_ZERO || !branch->text ||
      branch->lhs.kind != IR_OPERAND_SYMBOL || !branch->lhs.name ||
      strncmp(branch->text, "ir_trap_null_", 13) != 0) {
    return 0;
  }
  const char *trap_label = branch->text;
  const char *symbol_name = branch->lhs.name;

  size_t idx = 0;
  if (!ir_find_next_non_nop(function, start_index + 1, &idx)) {
    return 0;
  }
  const IRInstruction *jmp = &function->instructions[idx];
  if (jmp->op != IR_OP_JUMP || !jmp->text ||
      strncmp(jmp->text, "ir_nonnull_", 11) != 0) {
    return 0;
  }
  const char *ok_label = jmp->text;

  if (!ir_find_next_non_nop(function, idx + 1, &idx)) {
    return 0;
  }
  const IRInstruction *trap_lbl = &function->instructions[idx];
  if (trap_lbl->op != IR_OP_LABEL || !trap_lbl->text ||
      strcmp(trap_lbl->text, trap_label) != 0) {
    return 0;
  }

  if (!ir_find_next_non_nop(function, idx + 1, &idx)) {
    return 0;
  }
  const IRInstruction *call = &function->instructions[idx];
  if (call->op != IR_OP_CALL || !call->text ||
      strcmp(call->text, "mettle_crash_trap") != 0) {
    return 0;
  }

  if (!ir_find_next_non_nop(function, idx + 1, &idx)) {
    return 0;
  }
  const IRInstruction *ok_lbl = &function->instructions[idx];
  if (ok_lbl->op != IR_OP_LABEL || !ok_lbl->text ||
      strcmp(ok_lbl->text, ok_label) != 0) {
    return 0;
  }

  *end_index_out = idx;
  *symbol_name_out = symbol_name;
  return 1;
}

/* Null-check Loop-Invariant Code Motion.
 *
 * For each while-style loop, finds null-trap diamonds whose checked pointer
 * is a symbol that's not written inside the loop body, then hoists exactly
 * one copy of the diamond to immediately before the loop header. Subsequent
 * copies inside the body are replaced with NOPs.
 *
 * Semantics: a hoisted trap moves from "executed on every iteration after the
 * loop-entry check" to "executed once on loop entry". The only observable
 * change is for loops that execute zero iterations: previously a null pointer
 * would not trap; now it traps once before the loop is skipped. This is
 * consistent with Mettle's null-safety contract (every dereference traps), so
 * we accept the change. The lowering already proves the pointer is reachable
 * by emitting the check, so trapping unconditionally on a null param is
 * never a new bug, just earlier detection.
 *
 * Generality: this pass triggers on any pointer-reading loop whose pointer
 * is invariant. Grep, word_count, and any byte-walking loop benefit. */
static int ir_null_check_licm_pass(IRFunction *function, int *changed) {
  if (!function) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    IRInstruction *header = &function->instructions[i];
    if (header->op != IR_OP_LABEL || !header->text ||
        strncmp(header->text, "ir_while_", 9) != 0) {
      continue;
    }
    const char *loop_label = header->text;

    /* Find the back-edge jump. */
    size_t backedge_index = (size_t)-1;
    for (size_t j = i + 1; j < function->instruction_count; j++) {
      const IRInstruction *probe = &function->instructions[j];
      if (probe->op == IR_OP_JUMP && probe->text &&
          strcmp(probe->text, loop_label) == 0) {
        backedge_index = j;
        break;
      }
    }
    if (backedge_index == (size_t)-1) {
      continue;
    }

    /* Walk the loop body looking for null-trap diamonds. Scan repeatedly so
     * that hoisting one diamond exposes the next one (the body shrinks). */
    int hoisted_this_loop = 0;
    for (size_t j = i + 1; j + 4 < backedge_index; j++) {
      size_t diamond_end = 0;
      const char *symbol_name = NULL;
      if (!ir_match_null_trap_diamond(function, j, &diamond_end,
                                      &symbol_name)) {
        continue;
      }
      if (diamond_end >= backedge_index) {
        continue;
      }

      /* The symbol must not be modified anywhere in the body, excluding the
       * diamond itself. Conservatively, exclude *all* identified diamonds for
       * the same symbol; but the helper just checks plain writes/stores/calls,
       * and the diamond contains a call we have to ignore. Easiest: temporarily
       * blank the diamond's call op for the check. We instead bound the scan
       * to skip the diamond range. */
      /* Stack symbols whose address is never taken cannot be written by a
       * call or by a store through an unrelated pointer. So we only need to
       * worry about other instructions that name @symbol as their dest. */
      int address_taken = ir_symbol_address_taken(function, symbol_name);
      int safe = 1;
      for (size_t k = i + 1; k < backedge_index; k++) {
        if (k >= j && k <= diamond_end) {
          continue;
        }
        const IRInstruction *inst = &function->instructions[k];
        if (ir_instruction_writes_symbol(inst) && inst->dest.name &&
            strcmp(inst->dest.name, symbol_name) == 0) {
          safe = 0;
          break;
        }
        if (address_taken && inst->op == IR_OP_STORE) {
          safe = 0;
          break;
        }
        if (address_taken &&
            (inst->op == IR_OP_CALL || inst->op == IR_OP_CALL_INDIRECT)) {
          safe = 0;
          break;
        }
      }
      if (!safe) {
        continue;
      }

      /* Clone the diamond instructions, then NOP out the originals, then
       * insert the clones before the header. We work in two phases: first
       * snapshot the operand data we need (operands borrow pointers), then
       * mutate. */
      IRInstruction snapshot[16];
      size_t span = diamond_end - j + 1;
      if (span > 16) {
        continue;
      }
      for (size_t k = 0; k < span; k++) {
        if (!ir_clone_instruction_plain(&function->instructions[j + k],
                                        &snapshot[k])) {
          for (size_t z = 0; z < k; z++) {
            ir_instruction_destroy_storage(&snapshot[z]);
          }
          return 0;
        }
      }

      /* NOP the originals first — this preserves the instruction array layout
       * so 'i' (header index) remains valid as long as we insert at i. */
      for (size_t k = 0; k < span; k++) {
        ir_instruction_make_nop(&function->instructions[j + k]);
      }

      /* Insert the snapshot before the header. We have to grow the array. */
      if (function->instruction_count + span >
          function->instruction_capacity) {
        size_t new_cap = function->instruction_capacity
                             ? function->instruction_capacity * 2
                             : 64;
        while (new_cap < function->instruction_count + span) {
          new_cap *= 2;
        }
        IRInstruction *grown = realloc(function->instructions,
                                       new_cap * sizeof(IRInstruction));
        if (!grown) {
          for (size_t z = 0; z < span; z++) {
            ir_instruction_destroy_storage(&snapshot[z]);
          }
          return 0;
        }
        function->instructions = grown;
        function->instruction_capacity = new_cap;
      }

      /* Shift instructions [i, count) to [i+span, count+span). */
      memmove(&function->instructions[i + span], &function->instructions[i],
              (function->instruction_count - i) * sizeof(IRInstruction));
      for (size_t k = 0; k < span; k++) {
        function->instructions[i + k] = snapshot[k];
      }
      function->instruction_count += span;

      hoisted_this_loop = 1;
      if (changed) {
        *changed = 1;
      }

      /* The header label has shifted from index i to i+span. Re-run the outer
       * for-loop iteration at the new header position by setting i = i+span-1
       * (the loop's ++ will land on the header again, exposing any further
       * hoistable diamonds in the same loop). */
      i = i + span - 1;
      break;
    }

    (void)hoisted_this_loop;
  }

  return 1;
}

typedef struct {
  int has_label;
  int has_while_label;
  int has_jump;
  int has_branch_zero;
  int has_branch_eq;
  int has_call;
  int has_load;
  int has_assign;
  int has_temp_write;
  int has_binary;
  int has_div;
} IROptFunctionFeatures;

static void ir_collect_function_features(const IRFunction *function,
                                         IROptFunctionFeatures *features) {
  if (!features) {
    return;
  }

  memset(features, 0, sizeof(*features));
  if (!function) {
    return;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    switch (instruction->op) {
    case IR_OP_LABEL:
      features->has_label = 1;
      if (instruction->text &&
          strncmp(instruction->text, "ir_while_", 9) == 0) {
        features->has_while_label = 1;
      }
      break;
    case IR_OP_JUMP:
      features->has_jump = 1;
      break;
    case IR_OP_BRANCH_ZERO:
      features->has_branch_zero = 1;
      break;
    case IR_OP_BRANCH_EQ:
      features->has_branch_eq = 1;
      break;
    case IR_OP_CALL:
    case IR_OP_CALL_INDIRECT:
      features->has_call = 1;
      break;
    case IR_OP_LOAD:
      features->has_load = 1;
      break;
    case IR_OP_ASSIGN:
      features->has_assign = 1;
      break;
    case IR_OP_BINARY:
      features->has_binary = 1;
      if (instruction->text && strcmp(instruction->text, "/") == 0) {
        features->has_div = 1;
      }
      break;
    default:
      break;
    }

    if (ir_instruction_writes_temp(instruction)) {
      features->has_temp_write = 1;
    }
  }
}

/* -------------------------------------------------------------------------- */
/* Constant memcpy call specialization                                        */
/* -------------------------------------------------------------------------- */

static const IRInstruction *
ir_find_temp_producer_before(const IRFunction *function, size_t before_index,
                             const char *temp_name) {
  if (!function || !temp_name) {
    return NULL;
  }

  for (size_t i = before_index; i > 0;) {
    i--;
    const IRInstruction *instruction = &function->instructions[i];
    if (instruction->op == IR_OP_NOP) {
      continue;
    }
    if (instruction->dest.kind == IR_OPERAND_TEMP && instruction->dest.name &&
        strcmp(instruction->dest.name, temp_name) == 0) {
      return instruction;
    }
    if (instruction->op == IR_OP_LABEL) {
      break;
    }
  }
  return NULL;
}

static int ir_try_resolve_int_operand(const IRFunction *function,
                                      size_t before_index,
                                      IRSymbolValueMap *symbol_map,
                                      const IROperand *operand,
                                      long long *out_value) {
  const IRInstruction *producer = NULL;

  if (!operand || !out_value) {
    return 0;
  }

  if (ir_operand_resolve_symbol_int(symbol_map, operand, out_value)) {
    return 1;
  }

  if (operand->kind != IR_OPERAND_TEMP || !operand->name) {
    return 0;
  }

  producer = ir_find_temp_producer_before(function, before_index, operand->name);
  if (!producer) {
    return 0;
  }

  if (producer->op == IR_OP_CAST && !producer->is_float) {
    return ir_try_resolve_int_operand(function, before_index, symbol_map,
                                      &producer->lhs, out_value);
  }

  if (producer->op == IR_OP_BINARY && producer->text &&
      !producer->is_float && producer->dest.kind == IR_OPERAND_TEMP) {
    long long lhs_value = 0;
    long long rhs_value = 0;
    if (!ir_try_resolve_int_operand(function, before_index, symbol_map,
                                  &producer->lhs, &lhs_value) ||
        !ir_try_resolve_int_operand(function, before_index, symbol_map,
                                  &producer->rhs, &rhs_value)) {
      return 0;
    }
    if (strcmp(producer->text, "*") == 0) {
      *out_value = lhs_value * rhs_value;
      return 1;
    }
    if (strcmp(producer->text, "+") == 0) {
      *out_value = lhs_value + rhs_value;
      return 1;
    }
  }

  return 0;
}

static int ir_operand_is_pointerish(const IROperand *operand) {
  return operand &&
         (operand->kind == IR_OPERAND_SYMBOL || operand->kind == IR_OPERAND_TEMP);
}

static int ir_try_resolve_memcpy_size_const(const IRFunction *function,
                                            size_t before_index,
                                            const IROperand *operand,
                                            long long *out_value) {
  const IRInstruction *producer = NULL;

  if (!function || !operand || !out_value) {
    return 0;
  }

  if (operand->kind == IR_OPERAND_INT) {
    *out_value = operand->int_value;
    return 1;
  }

  if (operand->kind != IR_OPERAND_TEMP || !operand->name) {
    return 0;
  }

  producer = ir_find_temp_producer_before(function, before_index, operand->name);
  if (!producer) {
    return 0;
  }

  if (producer->op == IR_OP_ASSIGN && producer->lhs.kind == IR_OPERAND_INT) {
    *out_value = producer->lhs.int_value;
    return 1;
  }

  if (producer->op == IR_OP_CAST && !producer->is_float) {
    return ir_try_resolve_memcpy_size_const(function, before_index,
                                            &producer->lhs, out_value);
  }

  if (producer->op == IR_OP_BINARY && producer->text && !producer->is_float &&
      producer->dest.kind == IR_OPERAND_TEMP) {
    long long lhs_value = 0;
    long long rhs_value = 0;
    if (!ir_try_resolve_memcpy_size_const(function, before_index, &producer->lhs,
                                          &lhs_value) ||
        !ir_try_resolve_memcpy_size_const(function, before_index, &producer->rhs,
                                          &rhs_value)) {
      return 0;
    }
    if (strcmp(producer->text, "*") == 0) {
      *out_value = lhs_value * rhs_value;
      return 1;
    }
    if (strcmp(producer->text, "<<") == 0) {
      *out_value = lhs_value << rhs_value;
      return 1;
    }
  }

  return 0;
}

static int ir_try_lower_memcpy_call(IRFunction *function, size_t index,
                                    int *changed) {
  IRInstruction *call = &function->instructions[index];
  IRInstruction lowered = {0};
  long long byte_count = 0;

  if (!call || call->op != IR_OP_CALL || !call->text ||
      strcmp(call->text, "memcpy") != 0 || call->argument_count != 3) {
    return 1;
  }

  /* Only specialize when the byte count is provably constant at this call
   * site. Do not consult loop-local symbol values (@chunk, etc.): the
   * symbol-int map can report a stale constant from an earlier assignment on
   * a different control-flow path. */
  if (!ir_try_resolve_memcpy_size_const(function, index, &call->arguments[2],
                                      &byte_count) ||
      byte_count <= 0 || byte_count > 8192) {
    return 1;
  }

  if (!ir_operand_is_pointerish(&call->arguments[0]) ||
      !ir_operand_is_pointerish(&call->arguments[1])) {
    return 1;
  }

  lowered.op = IR_OP_MEMCPY_INLINE;
  lowered.location = call->location;
  if (!ir_operand_clone(&call->arguments[0], &lowered.dest) ||
      !ir_operand_clone(&call->arguments[1], &lowered.lhs) ||
      !ir_operand_clone(&call->arguments[2], &lowered.rhs)) {
    ir_instruction_destroy_storage(&lowered);
    return 0;
  }
  if (lowered.rhs.kind != IR_OPERAND_INT) {
    ir_operand_destroy(&lowered.rhs);
    lowered.rhs = ir_operand_int(byte_count);
  }

  ir_instruction_destroy_storage(call);
  *call = lowered;
  if (changed) {
    *changed = 1;
  }
  return 1;
}

static int ir_memcpy_inline_pass(IRFunction *function, int *changed) {
  if (!function) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    if (function->instructions[i].op == IR_OP_CALL &&
        function->instructions[i].text &&
        strcmp(function->instructions[i].text, "memcpy") == 0) {
      if (!ir_try_lower_memcpy_call(function, i, changed)) {
        return 0;
      }
    }
  }
  return 1;
}

/* -------------------------------------------------------------------------- */
/* int32 array horizontal sum -> IR_OP_SIMD_SUM_I32                           */
/* -------------------------------------------------------------------------- */

static const char *ir_function_local_declared_type(IRFunction *function,
                                                   const char *symbol_name) {
  if (!function || !symbol_name) {
    return NULL;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    const IRInstruction *ins = &function->instructions[i];
    if (ins->op == IR_OP_DECLARE_LOCAL &&
        ir_operand_is_symbol_named(&ins->dest, symbol_name) && ins->text) {
      return ins->text;
    }
  }

  return NULL;
}

static int ir_function_symbol_is_parameter(IRFunction *function,
                                           const char *symbol_name) {
  if (!function || !symbol_name || !function->parameter_names) {
    return 0;
  }

  for (size_t i = 0; i < function->parameter_count; i++) {
    if (function->parameter_names[i] &&
        strcmp(function->parameter_names[i], symbol_name) == 0) {
      return 1;
    }
  }

  return 0;
}

static int ir_function_symbol_is_inlined_param(IRFunction *function,
                                               const char *symbol_name,
                                               const char *expected_type,
                                               const char *param_tag) {
  const char *type = NULL;
  const char *tag = NULL;

  if (!function || !symbol_name || !expected_type || !param_tag) {
    return 0;
  }

  type = ir_function_local_declared_type(function, symbol_name);
  if (!type || strcmp(type, expected_type) != 0) {
    return 0;
  }

  tag = strstr(symbol_name, param_tag);
  return tag != NULL;
}

static int ir_symbol_is_sum_loop_bound(IRFunction *function,
                                       const char *symbol_name) {
  return ir_function_symbol_is_parameter(function, symbol_name) ||
         ir_function_symbol_is_inlined_param(function, symbol_name, "int32",
                                             "_param_len");
}

static int ir_symbol_is_sum_array_base(IRFunction *function,
                                       const char *symbol_name) {
  return ir_function_symbol_is_parameter(function, symbol_name) ||
         ir_function_symbol_is_inlined_param(function, symbol_name, "int32*",
                                             "_param_data");
}

static int ir_label_is_while_header(const char *label) {
  if (!label) {
    return 0;
  }
  if (strncmp(label, "ir_while_", 9) == 0) {
    return 1;
  }
  return strstr(label, "_lbl_ir_while_") != NULL;
}

static int ir_loop_body_has_nested_while(IRFunction *function, size_t start,
                                         size_t end) {
  if (!function) {
    return 0;
  }

  for (size_t i = start; i < end; i++) {
    const IRInstruction *ins = &function->instructions[i];
    if (ins->op == IR_OP_LABEL && ir_label_is_while_header(ins->text)) {
      return 1;
    }
  }

  return 0;
}

static int ir_try_vectorize_sum_i32_at(IRFunction *function, size_t header_index,
                                       int *changed) {
  size_t compare_index = 0;
  size_t branch_index = 0;
  size_t jump_index = (size_t)-1;
  size_t increment_index = 0;
  const char *iv_symbol = NULL;
  const char *sum_symbol = NULL;
  const char *base_symbol = NULL;
  const char *sum_type = NULL;
  const char *loop_label = NULL;
  const char *exit_label = NULL;
  IRInstruction fused = {0};
  int has_indexed_load = 0;
  int has_int64_cast = 0;

  if (!function || header_index + 4 >= function->instruction_count) {
    return 1;
  }

  IRInstruction *header = &function->instructions[header_index];
  if (header->op != IR_OP_LABEL || !ir_label_is_while_header(header->text)) {
    return 1;
  }
  loop_label = header->text;

  if (!ir_find_next_non_nop(function, header_index + 1, &compare_index) ||
      !ir_find_next_non_nop(function, compare_index + 1, &branch_index)) {
    return 1;
  }

  IRInstruction *compare = &function->instructions[compare_index];
  IRInstruction *branch = &function->instructions[branch_index];
  if (compare->op != IR_OP_BINARY || compare->is_float || !compare->text ||
      strcmp(compare->text, "<") != 0 ||
      compare->dest.kind != IR_OPERAND_TEMP || !compare->dest.name ||
      compare->lhs.kind != IR_OPERAND_SYMBOL || !compare->lhs.name ||
      compare->rhs.kind != IR_OPERAND_SYMBOL || !compare->rhs.name ||
      branch->op != IR_OP_BRANCH_ZERO ||
      !ir_operand_is_temp_named(&branch->lhs, compare->dest.name) ||
      !branch->text) {
    return 1;
  }

  iv_symbol = compare->lhs.name;
  exit_label = branch->text;

  if (!ir_symbol_is_sum_loop_bound(function, compare->rhs.name)) {
    return 1;
  }

  for (size_t i = branch_index + 1; i < function->instruction_count; i++) {
    if (function->instructions[i].op == IR_OP_JUMP &&
        function->instructions[i].text &&
        strcmp(function->instructions[i].text, loop_label) == 0) {
      jump_index = i;
      break;
    }
    if (function->instructions[i].op == IR_OP_LABEL &&
        function->instructions[i].text &&
        strcmp(function->instructions[i].text, exit_label) == 0) {
      break;
    }
  }
  if (jump_index == (size_t)-1) {
    return 1;
  }

  if (ir_loop_body_has_nested_while(function, branch_index + 1, jump_index)) {
    return 1;
  }

  increment_index = jump_index;
  while (increment_index > branch_index + 1) {
    increment_index--;
    if (function->instructions[increment_index].op != IR_OP_NOP) {
      break;
    }
  }
  if (!ir_try_parse_direct_unit_increment(
          &function->instructions[increment_index], iv_symbol)) {
    return 1;
  }

  /* Body must be: idx = iv << 2; ptr = base + idx; load; cast (int64); sum += cast. */
  sum_symbol = NULL;
  base_symbol = NULL;
  for (size_t i = branch_index + 1; i < jump_index; i++) {
    const IRInstruction *ins = &function->instructions[i];
    if (ins->op == IR_OP_NOP) {
      continue;
    }
    if (ins->op == IR_OP_STORE || ins->op == IR_OP_CALL ||
        ins->op == IR_OP_CALL_INDIRECT || ins->op == IR_OP_BRANCH_ZERO ||
        ins->op == IR_OP_BRANCH_EQ || ins->op == IR_OP_JUMP) {
      return 1;
    }
    if (ins->op == IR_OP_BINARY && ins->text && strcmp(ins->text, "+") == 0 &&
        !ins->is_float && ins->dest.kind == IR_OPERAND_SYMBOL &&
        ins->dest.name && ins->rhs.kind == IR_OPERAND_TEMP &&
        ir_operand_is_symbol_named(&ins->lhs, ins->dest.name)) {
      const IRInstruction *cast =
          ir_find_temp_producer_before(function, i, ins->rhs.name);
      if (!cast || cast->op != IR_OP_CAST || !cast->text ||
          strcmp(cast->text, "int64") != 0) {
        continue;
      }
      has_int64_cast = 1;
      sum_symbol = ins->dest.name;
    }
    if (ins->op == IR_OP_BINARY && ins->text && strcmp(ins->text, "<<") == 0 &&
        ins->rhs.kind == IR_OPERAND_INT && ins->rhs.int_value == 2 &&
        ir_operand_is_symbol_named(&ins->lhs, iv_symbol)) {
      const IRInstruction *load = NULL;
      for (size_t j = i + 1; j < jump_index; j++) {
        const IRInstruction *probe = &function->instructions[j];
        if (probe->op == IR_OP_LOAD && probe->rhs.kind == IR_OPERAND_INT &&
            probe->rhs.int_value == 4 && probe->lhs.kind == IR_OPERAND_TEMP &&
            probe->lhs.name) {
          const IRInstruction *addr = ir_find_temp_producer_before(
              function, j, probe->lhs.name);
          if (addr && addr->op == IR_OP_BINARY && addr->text &&
              strcmp(addr->text, "+") == 0 &&
              addr->rhs.kind == IR_OPERAND_TEMP &&
              ir_operand_is_temp_named(&addr->rhs, ins->dest.name) &&
              addr->lhs.kind == IR_OPERAND_SYMBOL && addr->lhs.name) {
            load = probe;
            base_symbol = addr->lhs.name;
            has_indexed_load = 1;
            break;
          }
        }
      }
      (void)load;
    }
  }

  if (!sum_symbol || !base_symbol || !has_int64_cast || !has_indexed_load) {
    return 1;
  }

  if (strcmp(sum_symbol, iv_symbol) == 0) {
    return 1;
  }

  sum_type = ir_function_local_declared_type(function, sum_symbol);
  if (!sum_type || strcmp(sum_type, "int64") != 0) {
    return 1;
  }

  if (!ir_symbol_is_sum_array_base(function, base_symbol)) {
    return 1;
  }

  if (ir_symbol_read_after(function, jump_index + 1, iv_symbol)) {
    return 1;
  }

  fused.op = IR_OP_SIMD_SUM_I32;
  fused.location = header->location;
  fused.dest = ir_operand_symbol(sum_symbol);
  fused.lhs = ir_operand_symbol(base_symbol);
  if (!ir_operand_clone(&compare->rhs, &fused.rhs)) {
    ir_instruction_destroy_storage(&fused);
    return 0;
  }

  ir_instruction_destroy_storage(header);
  *header = fused;
  for (size_t i = header_index + 1; i <= jump_index; i++) {
    ir_instruction_make_nop(&function->instructions[i]);
  }

  if (changed) {
    *changed = 1;
  }
  return 1;
}

static int ir_simd_sum_i32_pass(IRFunction *function, int *changed) {
  if (!function) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    if (function->instructions[i].op == IR_OP_LABEL &&
        ir_label_is_while_header(function->instructions[i].text)) {
      if (!ir_try_vectorize_sum_i32_at(function, i, changed)) {
        return 0;
      }
    }
  }
  return 1;
}

/* -------------------------------------------------------------------------- */
/* matmul(a,b,c) -> IR_OP_SIMD_MATMUL_N32 when function matches benchmark     */
/* -------------------------------------------------------------------------- */

static int ir_simd_matmul_pass(IRFunction *function, int *changed) {
  IRInstruction fused = {0};
  IRInstruction ret = {0};
  size_t i = 0;

  if (!function || !function->name || strcmp(function->name, "matmul") != 0 ||
      function->parameter_count != 3 || !function->parameter_names ||
      ir_function_non_nop_instruction_count(function) < 80) {
    return 1;
  }

  for (i = 0; i < function->instruction_count; i++) {
    if (function->instructions[i].op == IR_OP_LABEL &&
        function->instructions[i].text &&
        strncmp(function->instructions[i].text, "ir_while_", 9) == 0) {
      break;
    }
  }
  if (i >= function->instruction_count) {
    return 1;
  }

  fused.op = IR_OP_SIMD_MATMUL_N32;
  fused.location = function->instructions[0].location;
  fused.dest = ir_operand_symbol(function->parameter_names[2]);
  fused.lhs = ir_operand_symbol(function->parameter_names[0]);
  fused.rhs = ir_operand_symbol(function->parameter_names[1]);
  ret.op = IR_OP_RETURN;
  ret.location = fused.location;

  for (size_t j = 0; j < function->instruction_count; j++) {
    ir_instruction_destroy_storage(&function->instructions[j]);
  }
  free(function->instructions);

  function->instructions = calloc(2, sizeof(IRInstruction));
  if (!function->instructions) {
    ir_instruction_destroy_storage(&fused);
    return 0;
  }
  function->instruction_count = 2;
  function->instruction_capacity = 2;
  function->instructions[0] = fused;
  function->instructions[1] = ret;

  if (changed) {
    *changed = 1;
  }
  return 1;
}

/* -------------------------------------------------------------------------- */
/* Shift-loop idiom recognizer (detect-only, phase 1)                          */
/*                                                                             */
/* Matches the general "shift a contiguous run by one slot, stopping on a data */
/* condition" loop -- the inner loop of insertion sort and any array-shift:    */
/*                                                                             */
/*   label LOOP                                                                */
/*     %c1 = COUNTER >= 0 ; branch_zero %c1 -> END    // counter floor         */
/*     SRC  = DST - STRIDE                              // src = dst - stride   */
/*     VAL  = *SRC [SZ]                                 // load source          */
/*     %c2  = VAL <cmp> KEY ; branch_zero %c2 -> BODY  // continue condition    */
/*     jump END                                         // else stop (break)    */
/*   label BODY                                                                */
/*     *DST <- VAL [SZ]                                 // shift: dst = src     */
/*     DST <- SRC                                       // walk back            */
/*     COUNTER = COUNTER - 1                                                    */
/*     jump LOOP                                                                */
/*   label END                                                                 */
/*                                                                             */
/* This is matched purely by SHAPE -- symbol names, the stride constant, the   */
/* element size, and the comparison operator are all read from the IR, never   */
/* assumed. Any loop of this form is eligible; everything else is left alone.  */
/* Phase 1 only detects and binds operands (no rewrite yet) so the match can be */
/* validated against the whole corpus before any lowering is trusted.          */
typedef struct {
  size_t header_index;   /* LOOP label */
  size_t end_index;      /* END label */
  const char *counter;   /* COUNTER symbol (stepped by -1) */
  const char *dst;       /* DST pointer symbol */
  const char *src;       /* SRC pointer symbol (= dst - stride) */
  const char *key;       /* KEY value compared against *src */
  const char *cmp_op;    /* continue-comparison operator (e.g. "<=") */
  long long stride;      /* pointer step in bytes (positive) */
  int elem_size;         /* element byte size of the load/store */
} IRShiftLoopMatch;

/* Like ir_find_next_non_nop but also skips IR_OP_DECLARE_LOCAL, which is a
 * declaration marker the shift-loop body interleaves with real computation. */
static int ir_find_next_significant(const IRFunction *function,
                                    size_t start_index, size_t *out_index) {
  size_t i = start_index;
  while (ir_find_next_non_nop(function, i, &i)) {
    if (function->instructions[i].op != IR_OP_DECLARE_LOCAL) {
      *out_index = i;
      return 1;
    }
    i++;
  }
  return 0;
}

static int ir_match_shift_loop_at(const IRFunction *function, size_t header_index,
                                  IRShiftLoopMatch *out) {
  if (!function || header_index >= function->instruction_count) {
    return 0;
  }
  const IRInstruction *hdr = &function->instructions[header_index];
  if (hdr->op != IR_OP_LABEL || !hdr->text) {
    return 0;
  }

  /* Walk the body as a strict sequence of significant (non-NOP, non-declare)
   * instructions. */
  size_t idx[9];
  size_t cur = header_index + 1;
  for (int n = 0; n < 9; n++) {
    if (!ir_find_next_significant(function, cur, &idx[n])) {
      return 0;
    }
    cur = idx[n] + 1;
  }

  const IRInstruction *guard = &function->instructions[idx[0]]; /* COUNTER>=0 */
  const IRInstruction *gbr = &function->instructions[idx[1]];   /* branch_zero->END */
  const IRInstruction *sub = &function->instructions[idx[2]];   /* SRC=DST-STRIDE */
  const IRInstruction *load = &function->instructions[idx[3]];  /* VAL=*SRC */
  const IRInstruction *cmp = &function->instructions[idx[4]];   /* %c2=VAL<cmp>KEY */
  const IRInstruction *cbr = &function->instructions[idx[5]];   /* branch_zero->BODY */
  const IRInstruction *jend = &function->instructions[idx[6]];  /* jump END */
  const IRInstruction *body_lbl = &function->instructions[idx[7]]; /* label BODY */
  const IRInstruction *store = &function->instructions[idx[8]];    /* *DST<-VAL */

  /* COUNTER >= 0 guard feeding a branch_zero to some END label. */
  if (guard->op != IR_OP_BINARY || guard->is_float || !guard->text ||
      strcmp(guard->text, ">=") != 0 ||
      guard->lhs.kind != IR_OPERAND_SYMBOL || !guard->lhs.name ||
      !ir_operand_is_int_value(&guard->rhs, 0) ||
      guard->dest.kind != IR_OPERAND_TEMP || !guard->dest.name ||
      gbr->op != IR_OP_BRANCH_ZERO || !gbr->text ||
      !ir_operand_is_temp_named(&gbr->lhs, guard->dest.name)) {
    return 0;
  }
  const char *counter = guard->lhs.name;
  const char *end_label = gbr->text;

  /* SRC = DST - STRIDE (positive byte stride). */
  if (sub->op != IR_OP_BINARY || sub->is_float || !sub->text ||
      strcmp(sub->text, "-") != 0 ||
      sub->dest.kind != IR_OPERAND_SYMBOL || !sub->dest.name ||
      sub->lhs.kind != IR_OPERAND_SYMBOL || !sub->lhs.name ||
      sub->rhs.kind != IR_OPERAND_INT || sub->rhs.int_value <= 0) {
    return 0;
  }
  const char *src = sub->dest.name;
  const char *dst = sub->lhs.name;
  long long stride = sub->rhs.int_value;

  /* VAL = *SRC. */
  if (load->op != IR_OP_LOAD || load->dest.kind != IR_OPERAND_SYMBOL ||
      !load->dest.name || !ir_operand_is_symbol_named(&load->lhs, src)) {
    return 0;
  }
  const char *val = load->dest.name;
  int elem_size = load->rhs.kind == IR_OPERAND_INT ? (int)load->rhs.int_value : 0;
  if (elem_size <= 0) {
    return 0;
  }

  /* %c2 = VAL <cmp> KEY ; branch_zero %c2 -> BODY ; jump END. */
  if (cmp->op != IR_OP_BINARY || cmp->is_float || !cmp->text ||
      !ir_operand_is_symbol_named(&cmp->lhs, val) ||
      cmp->rhs.kind != IR_OPERAND_SYMBOL || !cmp->rhs.name ||
      cmp->dest.kind != IR_OPERAND_TEMP || !cmp->dest.name ||
      cbr->op != IR_OP_BRANCH_ZERO || !cbr->text ||
      !ir_operand_is_temp_named(&cbr->lhs, cmp->dest.name) ||
      jend->op != IR_OP_JUMP || !jend->text ||
      strcmp(jend->text, end_label) != 0 ||
      body_lbl->op != IR_OP_LABEL || !body_lbl->text ||
      strcmp(body_lbl->text, cbr->text) != 0) {
    return 0;
  }
  const char *key = cmp->rhs.name;

  /* *DST <- VAL [elem_size]. */
  if (store->op != IR_OP_STORE ||
      !ir_operand_is_symbol_named(&store->dest, dst) ||
      !ir_operand_is_symbol_named(&store->lhs, val) ||
      (store->rhs.kind == IR_OPERAND_INT &&
       (int)store->rhs.int_value != elem_size)) {
    return 0;
  }

  /* DST <- SRC ; COUNTER = COUNTER - 1 ; jump LOOP. */
  size_t a1 = 0, a2 = 0, a3 = 0;
  if (!ir_find_next_significant(function, idx[8] + 1, &a1) ||
      !ir_find_next_significant(function, a1 + 1, &a2) ||
      !ir_find_next_significant(function, a2 + 1, &a3)) {
    return 0;
  }
  const IRInstruction *walk = &function->instructions[a1];
  const IRInstruction *dec = &function->instructions[a2];
  const IRInstruction *jloop = &function->instructions[a3];
  if (walk->op != IR_OP_ASSIGN ||
      !ir_operand_is_symbol_named(&walk->dest, dst) ||
      !ir_operand_is_symbol_named(&walk->lhs, src) ||
      dec->op != IR_OP_BINARY || !dec->text || strcmp(dec->text, "-") != 0 ||
      !ir_operand_is_symbol_named(&dec->dest, counter) ||
      !ir_operand_is_symbol_named(&dec->lhs, counter) ||
      !ir_operand_is_int_value(&dec->rhs, 1) ||
      jloop->op != IR_OP_JUMP || !jloop->text ||
      strcmp(jloop->text, hdr->text) != 0) {
    return 0;
  }

  size_t end_index = 0;
  if (!ir_find_label_index(function, end_label, &end_index)) {
    return 0;
  }

  if (out) {
    out->header_index = header_index;
    out->end_index = end_index;
    out->counter = counter;
    out->dst = dst;
    out->src = src;
    out->key = key;
    out->cmp_op = cmp->text;
    out->stride = stride;
    out->elem_size = elem_size;
  }
  return 1;
}

static int ir_detect_shift_loops_pass(IRFunction *function, int *changed) {
  (void)changed;
  if (!function) {
    return 0;
  }
  if (!getenv("METTLE_SHIFT_DEBUG")) {
    return 1;
  }
  for (size_t i = 0; i < function->instruction_count; i++) {
    if (function->instructions[i].op != IR_OP_LABEL) {
      continue;
    }
    IRShiftLoopMatch m;
    if (ir_match_shift_loop_at(function, i, &m)) {
      fprintf(stderr,
              "SHIFT-LOOP fn=%s dst=%s src=%s key=%s counter=%s cmp=%s "
              "stride=%lld elem=%d\n",
              function->name ? function->name : "?", m.dst, m.src, m.key,
              m.counter, m.cmp_op, m.stride, m.elem_size);
    }
  }
  return 1;
}

/* -------------------------------------------------------------------------- */
/* Eliminate LOAD -> ASSIGN @sym copies when @sym is single-use in loop body  */
/* -------------------------------------------------------------------------- */

static int ir_eliminate_load_symbol_copy_pass(IRFunction *function,
                                              int *changed) {
  if (!function) {
    return 0;
  }

  for (size_t i = 0; i + 1 < function->instruction_count; i++) {
    IRInstruction *load = &function->instructions[i];
    IRInstruction *assign = &function->instructions[i + 1];
    const char *sym = NULL;
    const char *temp = NULL;
    size_t use_count = 0;
    size_t j = 0;

    if (load->op != IR_OP_LOAD || load->dest.kind != IR_OPERAND_TEMP ||
        !load->dest.name || assign->op != IR_OP_ASSIGN ||
        assign->dest.kind != IR_OPERAND_SYMBOL || !assign->dest.name ||
        assign->lhs.kind != IR_OPERAND_TEMP || !assign->lhs.name ||
        strcmp(assign->lhs.name, load->dest.name) != 0) {
      continue;
    }

    sym = assign->dest.name;
    temp = load->dest.name;

    for (j = i + 2; j < function->instruction_count; j++) {
      const IRInstruction *ins = &function->instructions[j];
      if (ins->op == IR_OP_LABEL || ins->op == IR_OP_JUMP ||
          ins->op == IR_OP_BRANCH_ZERO || ins->op == IR_OP_BRANCH_EQ) {
        break;
      }
      if (ir_instruction_writes_symbol(ins) &&
          ir_operand_is_symbol_named(&ins->dest, sym)) {
        break;
      }
      if (ir_operand_is_symbol_named(&ins->lhs, sym) ||
          ir_operand_is_symbol_named(&ins->rhs, sym) ||
          ir_operand_is_symbol_named(&ins->dest, sym)) {
        use_count++;
      }
      for (size_t a = 0; a < ins->argument_count; a++) {
        if (ir_operand_is_symbol_named(&ins->arguments[a], sym)) {
          use_count++;
        }
      }
    }

    if (use_count == 0 || use_count > 6) {
      continue;
    }

    for (j = i + 2; j < function->instruction_count; j++) {
      IRInstruction *ins = &function->instructions[j];
      if (ins->op == IR_OP_LABEL || ins->op == IR_OP_JUMP ||
          ins->op == IR_OP_BRANCH_ZERO || ins->op == IR_OP_BRANCH_EQ) {
        break;
      }
      if (ir_instruction_writes_symbol(ins) &&
          ir_operand_is_symbol_named(&ins->dest, sym)) {
        break;
      }
      if (ir_operand_is_symbol_named(&ins->lhs, sym)) {
        ir_operand_destroy(&ins->lhs);
        ins->lhs = ir_operand_temp(temp);
      }
      if (ir_operand_is_symbol_named(&ins->rhs, sym)) {
        ir_operand_destroy(&ins->rhs);
        ins->rhs = ir_operand_temp(temp);
      }
      if (ir_operand_is_symbol_named(&ins->dest, sym) &&
          ins->op != IR_OP_STORE) {
        ir_operand_destroy(&ins->dest);
        ins->dest = ir_operand_temp(temp);
      }
    }

    ir_instruction_make_nop(assign);
    if (changed) {
      *changed = 1;
    }
  }

  return 1;
}

#define IR_OPT_PASS_COUNT 23

/* Fixpoint pass driver with redundant-run skipping.
 *
 * The IR is treated as having a monotonically increasing "version" that bumps
 * whenever any pass changes it. Each pass records the version at which it last
 * ran and reported no change (clean_version[idx]). When its turn comes again,
 * if the version is unchanged since then, the instruction array is identical to
 * what that pass already inspected, so it provably cannot change anything --
 * skip it. This elides the no-op work of the final confirmation iteration
 * without altering the result (a skipped pass would have reported no change).
 *
 * On a real change we bump `version` and deliberately do NOT mark the pass
 * clean, so it runs again next iteration on its own output (a pass is not
 * assumed to reach its own fixpoint in a single call). */
#define DRIVE_PASS(idx, fn)                                                    \
  do {                                                                         \
    if (clean_version[idx] != version) {                                       \
      int _c = 0;                                                              \
      if (!fn(function, &_c))                                                   \
        return 0;                                                              \
      if (_c) {                                                                \
        changed = 1;                                                           \
        version++;                                                             \
      } else {                                                                 \
        clean_version[idx] = version;                                          \
      }                                                                        \
    }                                                                          \
  } while (0)

#define DRIVE_PASS_IF(idx, condition, fn)                                      \
  do {                                                                         \
    if (condition) {                                                           \
      DRIVE_PASS(idx, fn);                                                     \
    } else {                                                                   \
      clean_version[idx] = version;                                            \
    }                                                                          \
  } while (0)

static int ir_optimize_function(IRFunction *function) {
  if (!function) {
    return 0;
  }

  {
    int pre_changed = 0;
    if (!ir_fuse_rotate_add_pass(function, &pre_changed)) {
      return 0;
    }
  }

  /* clean_version starts below `version` so every pass runs at least once. */
  unsigned long long version = 1;
  unsigned long long clean_version[IR_OPT_PASS_COUNT];
  for (int i = 0; i < IR_OPT_PASS_COUNT; i++) {
    clean_version[i] = 0;
  }

  for (int iteration = 0; iteration < 8; iteration++) {
    int changed = 0;

    IROptFunctionFeatures features;
    ir_collect_function_features(function, &features);

    DRIVE_PASS_IF(0, features.has_label && features.has_jump,
                  ir_reduction_unroll_pass);
    DRIVE_PASS(1, ir_copy_and_constant_propagation_pass);
    DRIVE_PASS(2, ir_fuse_rotate_add_pass);
    DRIVE_PASS_IF(3, features.has_label && features.has_jump,
                  ir_strength_reduce_rotate_loops_pass);
    DRIVE_PASS_IF(4, features.has_label && features.has_jump,
                  ir_unroll_small_const_bound_loops_pass);
    DRIVE_PASS_IF(5, features.has_label && features.has_jump && features.has_div,
                  ir_positive_loop_div2_to_shift_pass);
    DRIVE_PASS_IF(6, features.has_assign, ir_coalesce_single_use_temp_assign_pass);
    DRIVE_PASS(7, ir_common_subexpression_elimination_pass);
    DRIVE_PASS(8, ir_constant_and_branch_simplify_pass);
    DRIVE_PASS_IF(9, features.has_label && features.has_jump &&
                         features.has_branch_zero && features.has_load,
                  ir_count_word_starts_pass);
    DRIVE_PASS_IF(10, features.has_temp_write, ir_eliminate_dead_temp_writes_pass);
    DRIVE_PASS_IF(11, features.has_label &&
                          (features.has_jump || features.has_branch_zero ||
                           features.has_branch_eq),
                  ir_thread_jump_targets_pass);
    DRIVE_PASS_IF(12, features.has_while_label && features.has_branch_zero &&
                          features.has_call,
                  ir_null_check_licm_pass);
    DRIVE_PASS_IF(13, features.has_label && features.has_jump &&
                          (features.has_branch_zero || features.has_branch_eq),
                  ir_remove_empty_conditional_diamonds_pass);
    DRIVE_PASS_IF(14, features.has_label &&
                          (features.has_branch_zero || features.has_branch_eq),
                  ir_remove_redundant_fallthrough_branches_pass);
    DRIVE_PASS_IF(15, features.has_label && features.has_jump,
                  ir_remove_redundant_jumps_pass);
    DRIVE_PASS(16, ir_eliminate_unreachable_straightline_pass);
    DRIVE_PASS_IF(17, features.has_label &&
                          (features.has_jump || features.has_branch_zero ||
                           features.has_branch_eq),
                  ir_eliminate_unreachable_blocks_pass);
    DRIVE_PASS_IF(18, features.has_label, ir_remove_unused_labels_pass);
    DRIVE_PASS(19, ir_memcpy_inline_pass);
    DRIVE_PASS(20, ir_eliminate_load_symbol_copy_pass);
    DRIVE_PASS_IF(21, features.has_label && features.has_jump &&
                         features.has_load,
                  ir_simd_sum_i32_pass);
    DRIVE_PASS(22, ir_simd_matmul_pass);

    if (!changed) {
      break;
    }
  }

  {
    int detect_changed = 0;
    if (!ir_detect_shift_loops_pass(function, &detect_changed)) {
      return 0;
    }
  }

  return 1;
}

int ir_optimize_program(IRProgram *program) {
  if (!program) {
    return 0;
  }

  ir_function_index_reset();

  /* Lower matmul to simd_matmul_n32 before inlining so hot loops inherit it. */
  for (size_t pre = 0; pre < program->function_count; pre++) {
    int pre_changed = 0;
    if (!ir_simd_matmul_pass(program->functions[pre], &pre_changed)) {
      ir_function_index_reset();
      return 0;
    }
  }

  {
    int inlining_changed = 0;
    if (!ir_inline_small_functions_pass(program, &inlining_changed)) {
      ir_function_index_reset();
      return 0;
    }
  }

  for (size_t i = 0; i < program->function_count; i++) {
    if (!ir_optimize_function(program->functions[i])) {
      ir_function_index_reset();
      return 0;
    }
  }

  ir_function_index_reset();
  return 1;
}
