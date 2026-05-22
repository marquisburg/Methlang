#include "code_generator_internal.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BINARY_TEXT_SECTION_ALIGNMENT 16
#define BINARY_FUNCTION_STACK_SLOT_SIZE 8
#define BINARY_WIN64_REGISTER_ARG_COUNT 4
#define BINARY_WIN64_SHADOW_SPACE_SIZE 32

typedef enum {
  BINARY_GP_RAX = 0,
  BINARY_GP_RCX = 1,
  BINARY_GP_RDX = 2,
  BINARY_GP_RBX = 3,
  BINARY_GP_RSP = 4,
  BINARY_GP_RBP = 5,
  BINARY_GP_RSI = 6,
  BINARY_GP_RDI = 7,
  BINARY_GP_R11 = 11,
  BINARY_GP_R8 = 8,
  BINARY_GP_R9 = 9,
  BINARY_GP_R10 = 10,
  BINARY_GP_R12 = 12,
  BINARY_GP_R13 = 13,
  BINARY_GP_R14 = 14,
  BINARY_GP_R15 = 15,
} BinaryGpRegister;

typedef enum {
  BINARY_XMM0 = 0,
  BINARY_XMM1 = 1,
  BINARY_XMM2 = 2,
  BINARY_XMM3 = 3,
} BinaryXmmRegister;

typedef struct {
  unsigned char *data;
  size_t size;
  size_t capacity;
} BinaryCodeBuffer;

typedef struct {
  char *name;
  int offset;
} BinaryNamedSlot;

typedef struct {
  BinaryNamedSlot *items;
  size_t count;
  size_t capacity;
} BinaryNamedSlotTable;

typedef struct {
  char *name;
  size_t offset;
} BinaryLabelEntry;

typedef struct {
  BinaryLabelEntry *items;
  size_t count;
  size_t capacity;
} BinaryLabelTable;

typedef struct {
  char *name;
  size_t displacement_offset;
} BinaryLabelFixup;

typedef struct {
  BinaryLabelFixup *items;
  size_t count;
  size_t capacity;
} BinaryLabelFixupTable;

typedef struct {
  char *symbol_name;
  size_t displacement_offset;
} BinaryCallRelocation;

typedef struct {
  BinaryCallRelocation *items;
  size_t count;
  size_t capacity;
} BinaryCallRelocationTable;

typedef struct {
  size_t *items;
  size_t count;
  size_t capacity;
} BinaryOffsetTable;

typedef struct {
  const char *name;
  const char *target;
} BinarySymbolAliasEntry;

typedef struct {
  BinarySymbolAliasEntry *items;
  size_t count;
  size_t capacity;
} BinarySymbolAliasTable;

typedef struct {
  BinaryCodeBuffer code;
  BinaryNamedSlotTable parameter_slots;
  BinaryNamedSlotTable local_slots;
  BinaryNamedSlotTable temp_slots;
  BinaryNamedSlotTable float64_symbols;
  BinaryNamedSlotTable address_taken_symbols;
  BinaryNamedSlotTable register_symbols;
  BinarySymbolAliasTable symbol_aliases;
  BinaryLabelTable labels;
  BinaryLabelFixupTable label_fixups;
  BinaryCallRelocationTable call_relocations;
  BinaryOffsetTable return_fixups;
  BinaryGpRegister saved_registers[7];
  int saved_register_offsets[7];
  size_t saved_register_count;
  int raw_frame_size;
  int frame_size;
  int return_is_float64;
  /* IEEE-754 width of the function's float return (0/32/64). 0 = not float. */
  int return_float_bits;
  /* Set when the function's return type classifies INDIRECT (struct >8B or
   * non-pow2). The hidden out-pointer lives at [rbp - 8]; IR_OP_RETURN
   * memcpys through it. */
  int returns_indirect;
  /* Byte count of the INDIRECT return struct (0 if not INDIRECT). */
  size_t indirect_return_size;
  /* FIFO of caller-side return-slot rbp offsets, one per IR_OP_CALL whose
   * callee returns INDIRECT. Populated in the function pre-pass, consumed
   * in instruction order by emit_call. */
  int *indirect_return_slot_offsets;
  size_t indirect_return_slot_count;
  size_t indirect_return_slot_capacity;
  size_t indirect_return_slot_cursor;
  /* Side-table: which IR temps currently hold a POINTER to an indirect-
   * returned struct, with the byte size of that struct. Same role as
   * ir_indirect_temp_table in the text-asm path. Names are interned IR
   * strings (borrowed). */
  char **indirect_temp_names;
  size_t *indirect_temp_sizes;
  size_t indirect_temp_count;
  size_t indirect_temp_capacity;
  FunctionDeclaration *function_data;
  const char *function_name;
} BinaryFunctionContext;

static const BinaryGpRegister BINARY_WIN64_INT_PARAM_REGISTERS[] = {
    BINARY_GP_RCX, BINARY_GP_RDX, BINARY_GP_R8, BINARY_GP_R9};
static const BinaryXmmRegister BINARY_WIN64_FLOAT_PARAM_REGISTERS[] = {
    BINARY_XMM0, BINARY_XMM1, BINARY_XMM2, BINARY_XMM3};

typedef struct {
  char *name;
  uint64_t bits;
  long long int_value;
  double float_value;
  int is_float;
  int can_inline_load;
} BinaryGlobalConstEntry;

typedef struct {
  BinaryGlobalConstEntry *items;
  size_t count;
  size_t capacity;
  size_t *slots;
  size_t slot_count;
} BinaryGlobalConstTable;

static BinaryGlobalConstTable g_binary_global_consts = {0};

typedef struct {
  long long int_value;
  double float_value;
  int is_float;
} BinaryNumericConstant;

static size_t binary_string_hash(const char *name) {
  size_t hash = (size_t)1469598103934665603ULL;
  for (const unsigned char *p = (const unsigned char *)name; *p; p++) {
    hash ^= (size_t)*p;
    hash *= (size_t)1099511628211ULL;
  }
  return hash;
}

static char *binary_codegen_strdup(const char *value) {
  if (!value) {
    return NULL;
  }

  size_t length = strlen(value) + 1;
  char *copy = malloc(length);
  if (!copy) {
    return NULL;
  }

  memcpy(copy, value, length);
  return copy;
}

static void binary_global_const_table_reset(void) {
  for (size_t i = 0; i < g_binary_global_consts.count; i++) {
    free(g_binary_global_consts.items[i].name);
  }
  free(g_binary_global_consts.items);
  free(g_binary_global_consts.slots);
  g_binary_global_consts.items = NULL;
  g_binary_global_consts.count = 0;
  g_binary_global_consts.capacity = 0;
  g_binary_global_consts.slots = NULL;
  g_binary_global_consts.slot_count = 0;
}

static int binary_global_const_table_rebuild(size_t needed_count) {
  size_t slot_count = 16;
  size_t *slots = NULL;

  while (slot_count < needed_count * 2) {
    slot_count *= 2;
  }

  slots = calloc(slot_count, sizeof(size_t));
  if (!slots) {
    return 0;
  }

  for (size_t i = 0; i < g_binary_global_consts.count; i++) {
    BinaryGlobalConstEntry *entry = &g_binary_global_consts.items[i];
    if (!entry->name) {
      continue;
    }
    size_t slot = binary_string_hash(entry->name) & (slot_count - 1);
    while (slots[slot] != 0) {
      slot = (slot + 1) & (slot_count - 1);
    }
    slots[slot] = i + 1;
  }

  free(g_binary_global_consts.slots);
  g_binary_global_consts.slots = slots;
  g_binary_global_consts.slot_count = slot_count;
  return 1;
}

static BinaryGlobalConstEntry *
binary_global_const_table_find_entry(const char *name) {
  if (!name) {
    return NULL;
  }

  if (g_binary_global_consts.slots && g_binary_global_consts.slot_count > 0) {
    size_t mask = g_binary_global_consts.slot_count - 1;
    size_t slot = binary_string_hash(name) & mask;
    while (g_binary_global_consts.slots[slot] != 0) {
      size_t index = g_binary_global_consts.slots[slot] - 1;
      BinaryGlobalConstEntry *entry = &g_binary_global_consts.items[index];
      if (entry->name && strcmp(entry->name, name) == 0) {
        return entry;
      }
      slot = (slot + 1) & mask;
    }
    return NULL;
  }

  for (size_t i = 0; i < g_binary_global_consts.count; i++) {
    BinaryGlobalConstEntry *entry = &g_binary_global_consts.items[i];
    if (entry->name && strcmp(entry->name, name) == 0) {
      return entry;
    }
  }
  return NULL;
}

static uint64_t binary_global_const_bits(long long int_value, double float_value,
                                         int is_float) {
  uint64_t bits = 0;
  if (is_float) {
    memcpy(&bits, &float_value, sizeof(bits));
  } else {
    bits = (uint64_t)int_value;
  }
  return bits;
}

static int binary_global_const_table_add(const char *name, long long int_value,
                                         double float_value, int is_float,
                                         int can_inline_load) {
  if (!name) {
    return 0;
  }

  if (!g_binary_global_consts.slots ||
      ((g_binary_global_consts.count + 1) * 10 >=
       g_binary_global_consts.slot_count * 7)) {
    if (!binary_global_const_table_rebuild(g_binary_global_consts.count + 1)) {
      return 0;
    }
  }

  BinaryGlobalConstEntry *existing = binary_global_const_table_find_entry(name);
  if (existing) {
    existing->int_value = int_value;
    existing->float_value = float_value;
    existing->is_float = is_float ? 1 : 0;
    existing->bits =
        binary_global_const_bits(int_value, float_value, existing->is_float);
    existing->can_inline_load = can_inline_load ? 1 : 0;
    return 1;
  }

  if (g_binary_global_consts.count >= g_binary_global_consts.capacity) {
    size_t new_capacity =
        g_binary_global_consts.capacity ? g_binary_global_consts.capacity * 2 : 16;
    BinaryGlobalConstEntry *new_items =
        realloc(g_binary_global_consts.items,
                new_capacity * sizeof(BinaryGlobalConstEntry));
    if (!new_items) {
      return 0;
    }
    g_binary_global_consts.items = new_items;
    g_binary_global_consts.capacity = new_capacity;
  }

  char *name_copy = binary_codegen_strdup(name);
  if (!name_copy) {
    return 0;
  }

  size_t index = g_binary_global_consts.count;
  g_binary_global_consts.items[index].name = name_copy;
  g_binary_global_consts.items[index].int_value = int_value;
  g_binary_global_consts.items[index].float_value = float_value;
  g_binary_global_consts.items[index].is_float = is_float ? 1 : 0;
  g_binary_global_consts.items[index].bits =
      binary_global_const_bits(int_value, float_value, is_float);
  g_binary_global_consts.items[index].can_inline_load =
      can_inline_load ? 1 : 0;
  g_binary_global_consts.count++;

  size_t slot = binary_string_hash(name_copy) & (g_binary_global_consts.slot_count - 1);
  while (g_binary_global_consts.slots[slot] != 0) {
    slot = (slot + 1) & (g_binary_global_consts.slot_count - 1);
  }
  g_binary_global_consts.slots[slot] = index + 1;
  return 1;
}

static int binary_global_const_table_get(const char *name, uint64_t *value_out) {
  if (!name || !value_out) {
    return 0;
  }

  BinaryGlobalConstEntry *entry = binary_global_const_table_find_entry(name);
  if (entry && entry->can_inline_load) {
    *value_out = entry->bits;
    return 1;
  }

  return 0;
}

static int binary_global_const_table_get_numeric(
    const char *name, BinaryNumericConstant *value_out) {
  if (!name || !value_out) {
    return 0;
  }

  BinaryGlobalConstEntry *entry = binary_global_const_table_find_entry(name);
  if (!entry) {
    return 0;
  }

  value_out->int_value = entry->int_value;
  value_out->float_value = entry->float_value;
  value_out->is_float = entry->is_float;
  return 1;
}

static int binary_align_up_int(int value, int alignment, int *result_out) {
  if (!result_out || value < 0 || alignment <= 0) {
    return 0;
  }

  int remainder = value % alignment;
  if (remainder == 0) {
    *result_out = value;
    return 1;
  }
  if (value > INT_MAX - (alignment - remainder)) {
    return 0;
  }

  *result_out = value + (alignment - remainder);
  return 1;
}

/* Name -> IRFunction index for the binary backend.
 *
 * code_generator_find_ir_function_binary used to linear-scan every IR function
 * (strcmp each), and it is called once per emitted function plus once per call
 * and addr-of instruction. That is O(functions^2) and dominated codegen on
 * large programs. We cache an open-addressing hash table keyed on the current
 * ir_program pointer + function_count, rebuilding only when those change. */
typedef struct {
  const char *name; /* borrowed from the IRFunction; not owned */
  IRFunction *function;
} BinaryIRFunctionSlot;

typedef struct {
  BinaryIRFunctionSlot *slots;
  size_t slot_count; /* power of two */
  const IRProgram *program;
  size_t function_count;
} BinaryIRFunctionIndex;

static BinaryIRFunctionIndex g_binary_ir_function_index = {0};

static void binary_ir_function_index_reset(void) {
  free(g_binary_ir_function_index.slots);
  g_binary_ir_function_index.slots = NULL;
  g_binary_ir_function_index.slot_count = 0;
  g_binary_ir_function_index.program = NULL;
  g_binary_ir_function_index.function_count = 0;
}

static size_t binary_ir_function_hash(const char *name) {
  /* FNV-1a */
  size_t hash = (size_t)1469598103934665603ULL;
  for (const unsigned char *p = (const unsigned char *)name; *p; p++) {
    hash ^= (size_t)*p;
    hash *= (size_t)1099511628211ULL;
  }
  return hash;
}

static void binary_ir_function_index_insert(BinaryIRFunctionIndex *index,
                                            IRFunction *function) {
  size_t mask = index->slot_count - 1;
  size_t i = binary_ir_function_hash(function->name) & mask;
  while (index->slots[i].name) {
    /* First definition of a given name wins, matching the old linear scan
     * which returned the earliest matching function. */
    if (strcmp(index->slots[i].name, function->name) == 0) {
      return;
    }
    i = (i + 1) & mask;
  }
  index->slots[i].name = function->name;
  index->slots[i].function = function;
}

/* Returns 1 on success (index ready to query), 0 on allocation failure (caller
 * should fall back to a linear scan rather than miss real functions). */
static int binary_ir_function_index_ensure(const IRProgram *program) {
  if (g_binary_ir_function_index.program == program &&
      g_binary_ir_function_index.function_count == program->function_count &&
      g_binary_ir_function_index.slots) {
    return 1;
  }

  binary_ir_function_index_reset();

  /* Size to >=2x function count, power of two, min 16, to keep load factor
   * under 0.5 and probe chains short. */
  size_t slot_count = 16;
  while (slot_count < program->function_count * 2) {
    slot_count *= 2;
  }

  BinaryIRFunctionSlot *slots =
      calloc(slot_count, sizeof(BinaryIRFunctionSlot));
  if (!slots) {
    return 0;
  }

  g_binary_ir_function_index.slots = slots;
  g_binary_ir_function_index.slot_count = slot_count;
  g_binary_ir_function_index.program = program;
  g_binary_ir_function_index.function_count = program->function_count;

  for (size_t i = 0; i < program->function_count; i++) {
    IRFunction *function = program->functions[i];
    if (function && function->name) {
      binary_ir_function_index_insert(&g_binary_ir_function_index, function);
    }
  }

  return 1;
}

static IRFunction *code_generator_find_ir_function_binary(CodeGenerator *generator,
                                                          const char *name) {
  if (!generator || !generator->ir_program || !name) {
    return NULL;
  }

  const IRProgram *program = generator->ir_program;

  if (binary_ir_function_index_ensure(program)) {
    const BinaryIRFunctionIndex *index = &g_binary_ir_function_index;
    size_t mask = index->slot_count - 1;
    size_t i = binary_ir_function_hash(name) & mask;
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

static int binary_code_buffer_reserve(BinaryCodeBuffer *buffer,
                                      size_t minimum_capacity) {
  if (!buffer) {
    return 0;
  }

  if (buffer->capacity >= minimum_capacity) {
    return 1;
  }

  size_t new_capacity = buffer->capacity == 0 ? 64 : buffer->capacity * 2;
  while (new_capacity < minimum_capacity) {
    new_capacity *= 2;
  }

  unsigned char *grown = realloc(buffer->data, new_capacity);
  if (!grown) {
    return 0;
  }

  buffer->data = grown;
  buffer->capacity = new_capacity;
  return 1;
}

static int binary_code_buffer_append_bytes(BinaryCodeBuffer *buffer,
                                           const void *data, size_t size) {
  if (!buffer || (!data && size != 0)) {
    return 0;
  }

  if (!binary_code_buffer_reserve(buffer, buffer->size + size)) {
    return 0;
  }

  if (size != 0) {
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
  }

  return 1;
}

static int binary_code_buffer_append_u8(BinaryCodeBuffer *buffer,
                                        unsigned char value) {
  return binary_code_buffer_append_bytes(buffer, &value, sizeof(value));
}

static int binary_code_buffer_append_u32(BinaryCodeBuffer *buffer,
                                         uint32_t value) {
  return binary_code_buffer_append_bytes(buffer, &value, sizeof(value));
}

static int binary_code_buffer_append_u64(BinaryCodeBuffer *buffer,
                                         uint64_t value) {
  return binary_code_buffer_append_bytes(buffer, &value, sizeof(value));
}

static void binary_code_buffer_destroy(BinaryCodeBuffer *buffer) {
  if (!buffer) {
    return;
  }

  free(buffer->data);
  buffer->data = NULL;
  buffer->size = 0;
  buffer->capacity = 0;
}

static int binary_named_slot_table_get_offset(const BinaryNamedSlotTable *table,
                                              const char *name) {
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

static int binary_named_slot_table_add(BinaryNamedSlotTable *table,
                                       const char *name, int offset) {
  if (!table || !name || offset <= 0) {
    return 0;
  }

  int existing = binary_named_slot_table_get_offset(table, name);
  if (existing >= 0) {
    return existing == offset;
  }

  if (table->count >= table->capacity) {
    size_t new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    BinaryNamedSlot *grown =
        realloc(table->items, new_capacity * sizeof(BinaryNamedSlot));
    if (!grown) {
      return 0;
    }
    table->items = grown;
    table->capacity = new_capacity;
  }

  char *name_copy = binary_codegen_strdup(name);
  if (!name_copy) {
    return 0;
  }

  table->items[table->count].name = name_copy;
  table->items[table->count].offset = offset;
  table->count++;
  return 1;
}

static void binary_named_slot_table_destroy(BinaryNamedSlotTable *table) {
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

static const char *
binary_symbol_alias_table_get(const BinarySymbolAliasTable *table,
                              const char *name) {
  if (!table || !name) {
    return NULL;
  }

  for (size_t i = 0; i < table->count; i++) {
    if (table->items[i].name && strcmp(table->items[i].name, name) == 0) {
      return table->items[i].target;
    }
  }

  return NULL;
}

static int binary_symbol_alias_table_add(BinarySymbolAliasTable *table,
                                         const char *name,
                                         const char *target) {
  const char *existing = NULL;
  if (!table || !name || !target || name[0] == '\0' ||
      target[0] == '\0') {
    return 0;
  }

  existing = binary_symbol_alias_table_get(table, name);
  if (existing) {
    return strcmp(existing, target) == 0;
  }

  if (table->count >= table->capacity) {
    size_t new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    BinarySymbolAliasEntry *grown =
        realloc(table->items, new_capacity * sizeof(BinarySymbolAliasEntry));
    if (!grown) {
      return 0;
    }
    table->items = grown;
    table->capacity = new_capacity;
  }

  table->items[table->count].name = name;
  table->items[table->count].target = target;
  table->count++;
  return 1;
}

static void binary_symbol_alias_table_destroy(BinarySymbolAliasTable *table) {
  if (!table) {
    return;
  }
  free(table->items);
  table->items = NULL;
  table->count = 0;
  table->capacity = 0;
}

static BinaryLabelEntry *binary_label_table_get(BinaryLabelTable *table,
                                                const char *name) {
  if (!table || !name) {
    return NULL;
  }

  for (size_t i = 0; i < table->count; i++) {
    if (table->items[i].name && strcmp(table->items[i].name, name) == 0) {
      return &table->items[i];
    }
  }

  return NULL;
}

static int binary_label_table_define(BinaryLabelTable *table, const char *name,
                                     size_t offset) {
  if (!table || !name) {
    return 0;
  }

  if (binary_label_table_get(table, name)) {
    return 0;
  }

  if (table->count >= table->capacity) {
    size_t new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    BinaryLabelEntry *grown =
        realloc(table->items, new_capacity * sizeof(BinaryLabelEntry));
    if (!grown) {
      return 0;
    }
    table->items = grown;
    table->capacity = new_capacity;
  }

  char *name_copy = binary_codegen_strdup(name);
  if (!name_copy) {
    return 0;
  }

  table->items[table->count].name = name_copy;
  table->items[table->count].offset = offset;
  table->count++;
  return 1;
}

static void binary_label_table_destroy(BinaryLabelTable *table) {
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

static int binary_label_fixup_table_add(BinaryLabelFixupTable *table,
                                        const char *name,
                                        size_t displacement_offset) {
  if (!table || !name) {
    return 0;
  }

  if (table->count >= table->capacity) {
    size_t new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    BinaryLabelFixup *grown =
        realloc(table->items, new_capacity * sizeof(BinaryLabelFixup));
    if (!grown) {
      return 0;
    }
    table->items = grown;
    table->capacity = new_capacity;
  }

  char *name_copy = binary_codegen_strdup(name);
  if (!name_copy) {
    return 0;
  }

  table->items[table->count].name = name_copy;
  table->items[table->count].displacement_offset = displacement_offset;
  table->count++;
  return 1;
}

static void binary_label_fixup_table_destroy(BinaryLabelFixupTable *table) {
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

static int binary_call_relocation_table_add(BinaryCallRelocationTable *table,
                                            const char *symbol_name,
                                            size_t displacement_offset) {
  if (!table || !symbol_name) {
    return 0;
  }

  if (table->count >= table->capacity) {
    size_t new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    BinaryCallRelocation *grown =
        realloc(table->items, new_capacity * sizeof(BinaryCallRelocation));
    if (!grown) {
      return 0;
    }
    table->items = grown;
    table->capacity = new_capacity;
  }

  char *name_copy = binary_codegen_strdup(symbol_name);
  if (!name_copy) {
    return 0;
  }

  table->items[table->count].symbol_name = name_copy;
  table->items[table->count].displacement_offset = displacement_offset;
  table->count++;
  return 1;
}

static void binary_call_relocation_table_destroy(
    BinaryCallRelocationTable *table) {
  if (!table) {
    return;
  }

  for (size_t i = 0; i < table->count; i++) {
    free(table->items[i].symbol_name);
  }
  free(table->items);
  table->items = NULL;
  table->count = 0;
  table->capacity = 0;
}

static int binary_offset_table_add(BinaryOffsetTable *table, size_t offset) {
  if (!table) {
    return 0;
  }

  if (table->count >= table->capacity) {
    size_t new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    size_t *grown = realloc(table->items, new_capacity * sizeof(size_t));
    if (!grown) {
      return 0;
    }
    table->items = grown;
    table->capacity = new_capacity;
  }

  table->items[table->count++] = offset;
  return 1;
}

static void binary_offset_table_destroy(BinaryOffsetTable *table) {
  if (!table) {
    return;
  }

  free(table->items);
  table->items = NULL;
  table->count = 0;
  table->capacity = 0;
}

static void binary_function_context_destroy(BinaryFunctionContext *context) {
  if (!context) {
    return;
  }

  binary_code_buffer_destroy(&context->code);
  binary_named_slot_table_destroy(&context->parameter_slots);
  binary_named_slot_table_destroy(&context->local_slots);
  binary_named_slot_table_destroy(&context->temp_slots);
  binary_named_slot_table_destroy(&context->float64_symbols);
  binary_named_slot_table_destroy(&context->address_taken_symbols);
  binary_named_slot_table_destroy(&context->register_symbols);
  binary_symbol_alias_table_destroy(&context->symbol_aliases);
  binary_label_table_destroy(&context->labels);
  binary_label_fixup_table_destroy(&context->label_fixups);
  binary_call_relocation_table_destroy(&context->call_relocations);
  binary_offset_table_destroy(&context->return_fixups);
  free(context->indirect_return_slot_offsets);
  free(context->indirect_temp_names);
  free(context->indirect_temp_sizes);
}

static int binary_emit_rex(BinaryCodeBuffer *buffer, int w, int r, int x,
                           int b) {
  unsigned char rex = (unsigned char)(0x40 | (w ? 0x08 : 0) |
                                      (r ? 0x04 : 0) | (x ? 0x02 : 0) |
                                      (b ? 0x01 : 0));
  if (rex == 0x40) {
    return 1;
  }
  return binary_code_buffer_append_u8(buffer, rex);
}

static int binary_emit_push_reg(BinaryCodeBuffer *buffer,
                                BinaryGpRegister reg) {
  if (!buffer) {
    return 0;
  }
  if ((int)reg >= 8 && !binary_emit_rex(buffer, 0, 0, 0, 1)) {
    return 0;
  }
  return binary_code_buffer_append_u8(buffer, (unsigned char)(0x50 + (reg & 7)));
}

static int binary_emit_pop_reg(BinaryCodeBuffer *buffer, BinaryGpRegister reg) {
  if (!buffer) {
    return 0;
  }
  if ((int)reg >= 8 && !binary_emit_rex(buffer, 0, 0, 0, 1)) {
    return 0;
  }
  return binary_code_buffer_append_u8(buffer, (unsigned char)(0x58 + (reg & 7)));
}

static int binary_emit_mov_reg_reg(BinaryCodeBuffer *buffer,
                                   BinaryGpRegister destination,
                                   BinaryGpRegister source) {
  if (!buffer) {
    return 0;
  }
  if (destination == source) {
    return 1;
  }

  if (!binary_emit_rex(buffer, 1, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x8B) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((destination & 7) << 3) |
                                  (source & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_mov_reg_imm32_zero_extend(BinaryCodeBuffer *buffer,
                                                 BinaryGpRegister destination,
                                                 uint32_t immediate) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 0, 0, 0, destination >> 3) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xB8 + (destination & 7))) ||
      !binary_code_buffer_append_u32(buffer, immediate)) {
    return 0;
  }

  return 1;
}

static int binary_emit_xor_reg_reg32(BinaryCodeBuffer *buffer,
                                     BinaryGpRegister reg) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 0, reg >> 3, 0, reg >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x31) ||
      !binary_code_buffer_append_u8(
          buffer,
          (unsigned char)(0xC0 | ((reg & 7) << 3) | (reg & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_test_reg_reg(BinaryCodeBuffer *buffer,
                                    BinaryGpRegister reg);
static int binary_emit_neg_reg(BinaryCodeBuffer *buffer, BinaryGpRegister reg);
static int binary_emit_shift_reg_imm8(BinaryCodeBuffer *buffer,
                                      unsigned char subopcode,
                                      BinaryGpRegister reg,
                                      unsigned char immediate);

static int binary_emit_alu_rsp_imm32(BinaryCodeBuffer *buffer,
                                     unsigned char subopcode,
                                     uint32_t immediate) {
  if (!buffer) {
    return 0;
  }
  if (immediate == 0) {
    return 1;
  }

  int32_t signed_immediate = (int32_t)immediate;
  if (signed_immediate >= INT8_MIN && signed_immediate <= INT8_MAX) {
    if (!binary_emit_rex(buffer, 1, 0, 0, 0) ||
        !binary_code_buffer_append_u8(buffer, 0x83) ||
        !binary_code_buffer_append_u8(
            buffer, (unsigned char)(0xC0 | ((subopcode & 7) << 3) |
                                    (BINARY_GP_RSP & 7))) ||
        !binary_code_buffer_append_u8(buffer,
                                      (unsigned char)(int8_t)signed_immediate)) {
      return 0;
    }
    return 1;
  }

  if (!binary_emit_rex(buffer, 1, 0, 0, 0) ||
      !binary_code_buffer_append_u8(buffer, 0x81) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((subopcode & 7) << 3) |
                                  (BINARY_GP_RSP & 7))) ||
      !binary_code_buffer_append_u32(buffer, immediate)) {
    return 0;
  }

  return 1;
}

static int binary_emit_sub_rsp_imm32(BinaryCodeBuffer *buffer,
                                     uint32_t immediate) {
  return binary_emit_alu_rsp_imm32(buffer, 5, immediate);
}

static int binary_emit_add_rsp_imm32(BinaryCodeBuffer *buffer,
                                     uint32_t immediate) {
  return binary_emit_alu_rsp_imm32(buffer, 0, immediate);
}

static int binary_emit_alu_reg_imm32(BinaryCodeBuffer *buffer,
                                     unsigned char subopcode,
                                     BinaryGpRegister reg, uint32_t immediate) {
  if (!buffer) {
    return 0;
  }
  if ((subopcode == 0 || subopcode == 1 || subopcode == 5 ||
       subopcode == 6) &&
      immediate == 0) {
    return 1;
  }
  if (subopcode == 4 && immediate == UINT32_MAX) {
    return 1;
  }

  int32_t signed_immediate = (int32_t)immediate;
  if (signed_immediate >= INT8_MIN && signed_immediate <= INT8_MAX) {
    if (!binary_emit_rex(buffer, 1, 0, 0, reg >> 3) ||
        !binary_code_buffer_append_u8(buffer, 0x83) ||
        !binary_code_buffer_append_u8(
            buffer,
            (unsigned char)(0xC0 | ((subopcode & 7) << 3) | (reg & 7))) ||
        !binary_code_buffer_append_u8(buffer,
                                      (unsigned char)(int8_t)signed_immediate)) {
      return 0;
    }
    return 1;
  }

  if (!binary_emit_rex(buffer, 1, 0, 0, reg >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x81) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((subopcode & 7) << 3) | (reg & 7))) ||
      !binary_code_buffer_append_u32(buffer, immediate)) {
    return 0;
  }

  return 1;
}

static int binary_emit_add_reg_imm32(BinaryCodeBuffer *buffer,
                                     BinaryGpRegister reg,
                                     uint32_t immediate) {
  return binary_emit_alu_reg_imm32(buffer, 0, reg, immediate);
}

static int binary_emit_sub_reg_imm32(BinaryCodeBuffer *buffer,
                                     BinaryGpRegister reg,
                                     uint32_t immediate) {
  return binary_emit_alu_reg_imm32(buffer, 5, reg, immediate);
}

static int binary_emit_and_reg_imm32(BinaryCodeBuffer *buffer,
                                     BinaryGpRegister reg,
                                     uint32_t immediate) {
  return binary_emit_alu_reg_imm32(buffer, 4, reg, immediate);
}

static int binary_emit_or_reg_imm32(BinaryCodeBuffer *buffer,
                                    BinaryGpRegister reg, uint32_t immediate) {
  return binary_emit_alu_reg_imm32(buffer, 1, reg, immediate);
}

static int binary_emit_xor_reg_imm32(BinaryCodeBuffer *buffer,
                                     BinaryGpRegister reg,
                                     uint32_t immediate) {
  return binary_emit_alu_reg_imm32(buffer, 6, reg, immediate);
}

static int binary_emit_cmp_reg_imm32(BinaryCodeBuffer *buffer,
                                     BinaryGpRegister reg,
                                     uint32_t immediate) {
  if (immediate == 0) {
    return binary_emit_test_reg_reg(buffer, reg);
  }
  return binary_emit_alu_reg_imm32(buffer, 7, reg, immediate);
}

static int binary_emit_mov_reg_imm64(BinaryCodeBuffer *buffer,
                                     BinaryGpRegister destination,
                                     uint64_t immediate) {
  if (!buffer) {
    return 0;
  }
  if (immediate == 0) {
    return binary_emit_xor_reg_reg32(buffer, destination);
  }
  if (immediate <= UINT32_MAX) {
    return binary_emit_mov_reg_imm32_zero_extend(buffer, destination,
                                                (uint32_t)immediate);
  }
  if (immediate >= UINT64_C(0xffffffff80000000)) {
    int32_t signed_immediate = (int32_t)immediate;
    if (!binary_emit_rex(buffer, 1, 0, 0, destination >> 3) ||
        !binary_code_buffer_append_u8(buffer, 0xC7) ||
        !binary_code_buffer_append_u8(
            buffer, (unsigned char)(0xC0 | (destination & 7))) ||
        !binary_code_buffer_append_u32(buffer, (uint32_t)signed_immediate)) {
      return 0;
    }
    return 1;
  }

  if (!binary_emit_rex(buffer, 1, 0, 0, destination >> 3) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xB8 + (destination & 7))) ||
      !binary_code_buffer_append_u64(buffer, immediate)) {
    return 0;
  }

  return 1;
}

static int binary_emit_memory_access_ex(BinaryCodeBuffer *buffer,
                                        int operand_size_prefix, int rex_w,
                                        unsigned char opcode1,
                                        int has_opcode2,
                                        unsigned char opcode2,
                                        BinaryGpRegister reg,
                                        BinaryGpRegister base,
                                        int displacement) {
  if (!buffer) {
    return 0;
  }

  int use_disp8 = displacement >= -128 && displacement <= 127;
  unsigned char mod = use_disp8 ? 1 : 2;
  unsigned char rm = (unsigned char)(base & 7);
  unsigned char modrm =
      (unsigned char)((mod << 6) | ((reg & 7) << 3) |
                      ((rm == (BINARY_GP_RSP & 7)) ? 4 : rm));

  if ((operand_size_prefix &&
       !binary_code_buffer_append_u8(buffer, 0x66)) ||
      !binary_emit_rex(buffer, rex_w, reg >> 3, 0, base >> 3) ||
      !binary_code_buffer_append_u8(buffer, opcode1) ||
      (has_opcode2 && !binary_code_buffer_append_u8(buffer, opcode2)) ||
      !binary_code_buffer_append_u8(buffer, modrm)) {
    return 0;
  }

  if (rm == (BINARY_GP_RSP & 7)) {
    unsigned char sib =
        (unsigned char)((0 << 6) | (4 << 3) | (base & 7));
    if (!binary_code_buffer_append_u8(buffer, sib)) {
      return 0;
    }
  }

  if (use_disp8) {
    return binary_code_buffer_append_u8(buffer, (unsigned char)(int8_t)displacement);
  }

  return binary_code_buffer_append_u32(buffer, (uint32_t)(int32_t)displacement);
}

static int binary_emit_memory_access(BinaryCodeBuffer *buffer,
                                     unsigned char opcode,
                                     BinaryGpRegister reg,
                                     BinaryGpRegister base, int displacement) {
  return binary_emit_memory_access_ex(buffer, 0, 1, opcode, 0, 0, reg, base,
                                      displacement);
}

static int binary_emit_mov_reg_mem(BinaryCodeBuffer *buffer,
                                   BinaryGpRegister destination,
                                   BinaryGpRegister base, int displacement) {
  return binary_emit_memory_access(buffer, 0x8B, destination, base,
                                   displacement);
}

static int binary_emit_mov_mem_reg(BinaryCodeBuffer *buffer,
                                   BinaryGpRegister base, int displacement,
                                   BinaryGpRegister source) {
  return binary_emit_memory_access(buffer, 0x89, source, base, displacement);
}

static int binary_emit_movzx_reg_mem8(BinaryCodeBuffer *buffer,
                                      BinaryGpRegister destination,
                                      BinaryGpRegister base,
                                      int displacement) {
  return binary_emit_memory_access_ex(buffer, 0, 0, 0x0F, 1, 0xB6,
                                      destination, base, displacement);
}

static int binary_emit_movzx_reg_mem16(BinaryCodeBuffer *buffer,
                                       BinaryGpRegister destination,
                                       BinaryGpRegister base,
                                       int displacement) {
  return binary_emit_memory_access_ex(buffer, 0, 0, 0x0F, 1, 0xB7,
                                      destination, base, displacement);
}

static int binary_emit_mov_reg_mem32(BinaryCodeBuffer *buffer,
                                     BinaryGpRegister destination,
                                     BinaryGpRegister base, int displacement) {
  return binary_emit_memory_access_ex(buffer, 0, 0, 0x8B, 0, 0, destination,
                                      base, displacement);
}

static int binary_emit_mov_mem_reg8(BinaryCodeBuffer *buffer,
                                    BinaryGpRegister base, int displacement,
                                    BinaryGpRegister source) {
  return binary_emit_memory_access_ex(buffer, 0, 0, 0x88, 0, 0, source, base,
                                      displacement);
}

static int binary_emit_mov_mem_reg16(BinaryCodeBuffer *buffer,
                                     BinaryGpRegister base, int displacement,
                                     BinaryGpRegister source) {
  return binary_emit_memory_access_ex(buffer, 1, 0, 0x89, 0, 0, source, base,
                                      displacement);
}

static int binary_emit_mov_mem_reg32(BinaryCodeBuffer *buffer,
                                     BinaryGpRegister base, int displacement,
                                     BinaryGpRegister source) {
  return binary_emit_memory_access_ex(buffer, 0, 0, 0x89, 0, 0, source, base,
                                      displacement);
}

static int binary_emit_lea_reg_mem(BinaryCodeBuffer *buffer,
                                   BinaryGpRegister destination,
                                   BinaryGpRegister base, int displacement) {
  return binary_emit_memory_access(buffer, 0x8D, destination, base,
                                   displacement);
}

static int binary_emit_lea_reg_base_index_scale_disp(
    BinaryCodeBuffer *buffer, BinaryGpRegister destination,
    BinaryGpRegister base, BinaryGpRegister index, int scale,
    int displacement) {
  if (!buffer || index == BINARY_GP_RSP) {
    return 0;
  }

  unsigned char scale_bits = 0;
  switch (scale) {
  case 1:
    scale_bits = 0;
    break;
  case 2:
    scale_bits = 1;
    break;
  case 4:
    scale_bits = 2;
    break;
  case 8:
    scale_bits = 3;
    break;
  default:
    return 0;
  }

  int use_disp8 = displacement >= -128 && displacement <= 127;
  unsigned char mod = 0;
  if (displacement == 0 &&
      (base & 7) != (BINARY_GP_RBP & 7)) {
    mod = 0;
  } else {
    mod = use_disp8 ? 1 : 2;
  }

  if (!binary_emit_rex(buffer, 1, destination >> 3, index >> 3, base >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x8D) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)((mod << 6) | ((destination & 7) << 3) | 4)) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)((scale_bits << 6) | ((index & 7) << 3) |
                                  (base & 7)))) {
    return 0;
  }

  if (mod == 1) {
    return binary_code_buffer_append_u8(buffer,
                                        (unsigned char)(int8_t)displacement);
  }
  if (mod == 2) {
    return binary_code_buffer_append_u32(buffer,
                                         (uint32_t)(int32_t)displacement);
  }
  return 1;
}

static int binary_emit_lea_reg_reg(BinaryCodeBuffer *buffer,
                                   BinaryGpRegister destination,
                                   BinaryGpRegister lhs,
                                   BinaryGpRegister rhs) {
  if (rhs != BINARY_GP_RSP) {
    return binary_emit_lea_reg_base_index_scale_disp(buffer, destination, lhs,
                                                    rhs, 1, 0);
  }
  if (lhs != BINARY_GP_RSP) {
    return binary_emit_lea_reg_base_index_scale_disp(buffer, destination, rhs,
                                                    lhs, 1, 0);
  }
  return 0;
}

static int binary_emit_lea_reg_rip_placeholder(BinaryCodeBuffer *buffer,
                                               BinaryGpRegister destination,
                                               size_t *displacement_offset_out) {
  if (!buffer || !displacement_offset_out) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, destination >> 3, 0, 0) ||
      !binary_code_buffer_append_u8(buffer, 0x8D) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0x05 | ((destination & 7) << 3)))) {
    return 0;
  }

  *displacement_offset_out = buffer->size;
  return binary_code_buffer_append_u32(buffer, 0);
}

static int binary_emit_rip_relative_access_ex(
    BinaryCodeBuffer *buffer, int operand_size_prefix, int rex_w,
    unsigned char opcode1, int has_opcode2, unsigned char opcode2,
    BinaryGpRegister reg, size_t *displacement_offset_out) {
  if (!buffer || !displacement_offset_out) {
    return 0;
  }

  if ((operand_size_prefix &&
       !binary_code_buffer_append_u8(buffer, 0x66)) ||
      !binary_emit_rex(buffer, rex_w, reg >> 3, 0, 0) ||
      !binary_code_buffer_append_u8(buffer, opcode1) ||
      (has_opcode2 && !binary_code_buffer_append_u8(buffer, opcode2)) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0x05 | ((reg & 7) << 3)))) {
    return 0;
  }

  *displacement_offset_out = buffer->size;
  return binary_code_buffer_append_u32(buffer, 0);
}

static int binary_emit_mov_reg_rip_mem(BinaryCodeBuffer *buffer,
                                       BinaryGpRegister destination,
                                       size_t *displacement_offset_out) {
  return binary_emit_rip_relative_access_ex(buffer, 0, 1, 0x8B, 0, 0,
                                            destination,
                                            displacement_offset_out);
}

static int binary_emit_mov_reg32_rip_mem(BinaryCodeBuffer *buffer,
                                         BinaryGpRegister destination,
                                         size_t *displacement_offset_out) {
  return binary_emit_rip_relative_access_ex(buffer, 0, 0, 0x8B, 0, 0,
                                            destination,
                                            displacement_offset_out);
}

static int binary_emit_mov_mem_rip_reg8(BinaryCodeBuffer *buffer,
                                        BinaryGpRegister source,
                                        size_t *displacement_offset_out) {
  return binary_emit_rip_relative_access_ex(buffer, 0, 0, 0x88, 0, 0, source,
                                            displacement_offset_out);
}

static int binary_emit_mov_mem_rip_reg16(BinaryCodeBuffer *buffer,
                                         BinaryGpRegister source,
                                         size_t *displacement_offset_out) {
  return binary_emit_rip_relative_access_ex(buffer, 1, 0, 0x89, 0, 0, source,
                                            displacement_offset_out);
}

static int binary_emit_mov_mem_rip_reg32(BinaryCodeBuffer *buffer,
                                         BinaryGpRegister source,
                                         size_t *displacement_offset_out) {
  return binary_emit_rip_relative_access_ex(buffer, 0, 0, 0x89, 0, 0, source,
                                            displacement_offset_out);
}

static int binary_emit_mov_mem_rip_reg(BinaryCodeBuffer *buffer,
                                       BinaryGpRegister source,
                                       size_t *displacement_offset_out) {
  return binary_emit_rip_relative_access_ex(buffer, 0, 1, 0x89, 0, 0, source,
                                            displacement_offset_out);
}

static int binary_emit_movzx_reg_rip_mem8(BinaryCodeBuffer *buffer,
                                          BinaryGpRegister destination,
                                          size_t *displacement_offset_out) {
  return binary_emit_rip_relative_access_ex(buffer, 0, 0, 0x0F, 1, 0xB6,
                                            destination,
                                            displacement_offset_out);
}

static int binary_emit_movzx_reg_rip_mem16(BinaryCodeBuffer *buffer,
                                           BinaryGpRegister destination,
                                           size_t *displacement_offset_out) {
  return binary_emit_rip_relative_access_ex(buffer, 0, 0, 0x0F, 1, 0xB7,
                                            destination,
                                            displacement_offset_out);
}

static int binary_emit_test_reg_reg(BinaryCodeBuffer *buffer,
                                    BinaryGpRegister reg) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, reg >> 3, 0, reg >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x85) ||
      !binary_code_buffer_append_u8(
          buffer,
          (unsigned char)(0xC0 | ((reg & 7) << 3) | (reg & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_cmp_reg_reg(BinaryCodeBuffer *buffer,
                                   BinaryGpRegister lhs,
                                   BinaryGpRegister rhs) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, rhs >> 3, 0, lhs >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x39) ||
      !binary_code_buffer_append_u8(
          buffer,
          (unsigned char)(0xC0 | ((rhs & 7) << 3) | (lhs & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_alu_reg_reg(BinaryCodeBuffer *buffer,
                                   unsigned char opcode,
                                   BinaryGpRegister destination,
                                   BinaryGpRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, source >> 3, 0, destination >> 3) ||
      !binary_code_buffer_append_u8(buffer, opcode) ||
      !binary_code_buffer_append_u8(
          buffer,
          (unsigned char)(0xC0 | ((source & 7) << 3) | (destination & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_imul_reg_reg(BinaryCodeBuffer *buffer,
                                    BinaryGpRegister destination,
                                    BinaryGpRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0xAF) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((destination & 7) << 3) |
                                  (source & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_immediate_positive_power_of_two_i32(int32_t value,
                                                      unsigned char *shift_out) {
  if (!shift_out || value <= 0 || (value & (value - 1)) != 0) {
    return 0;
  }

  unsigned char shift = 0;
  uint32_t uvalue = (uint32_t)value;
  while (uvalue > 1u) {
    uvalue >>= 1u;
    shift++;
  }
  *shift_out = shift;
  return 1;
}

static int binary_emit_imul_reg_reg_small_imm(BinaryCodeBuffer *buffer,
                                              BinaryGpRegister destination,
                                              BinaryGpRegister source,
                                              int32_t immediate) {
  int negate = 0;
  if (immediate < 0) {
    if (immediate == INT32_MIN) {
      return 0;
    }
    negate = 1;
    immediate = -immediate;
  }

  int scale = 0;
  if (immediate == 3) {
    scale = 2;
  } else if (immediate == 5) {
    scale = 4;
  } else if (immediate == 9) {
    scale = 8;
  } else {
    return 0;
  }

  if (!binary_emit_lea_reg_base_index_scale_disp(buffer, destination, source,
                                                 source, scale, 0)) {
    return 0;
  }
  if (negate && !binary_emit_neg_reg(buffer, destination)) {
    return 0;
  }
  return 1;
}

static int binary_emit_imul_reg_reg_imm32(BinaryCodeBuffer *buffer,
                                          BinaryGpRegister destination,
                                          BinaryGpRegister source,
                                          uint32_t immediate) {
  if (!buffer) {
    return 0;
  }
  int32_t signed_immediate = (int32_t)immediate;
  if (signed_immediate == 0) {
    return binary_emit_xor_reg_reg32(buffer, destination);
  }
  if (signed_immediate == 1) {
    return binary_emit_mov_reg_reg(buffer, destination, source);
  }
  if (signed_immediate == -1) {
    return binary_emit_mov_reg_reg(buffer, destination, source) &&
           binary_emit_neg_reg(buffer, destination);
  }

  unsigned char shift = 0;
  if (binary_immediate_positive_power_of_two_i32(signed_immediate,
                                                 &shift)) {
    return binary_emit_mov_reg_reg(buffer, destination, source) &&
           binary_emit_shift_reg_imm8(buffer, 4, destination, shift);
  }
  if (signed_immediate != INT32_MIN &&
      binary_immediate_positive_power_of_two_i32(-signed_immediate,
                                                 &shift)) {
    return binary_emit_mov_reg_reg(buffer, destination, source) &&
           binary_emit_shift_reg_imm8(buffer, 4, destination, shift) &&
           binary_emit_neg_reg(buffer, destination);
  }
  if (binary_emit_imul_reg_reg_small_imm(buffer, destination, source,
                                         signed_immediate)) {
    return 1;
  }

  unsigned char opcode = signed_immediate >= INT8_MIN &&
                                 signed_immediate <= INT8_MAX
                             ? 0x6B
                             : 0x69;
  if (!binary_emit_rex(buffer, 1, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, opcode) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((destination & 7) << 3) |
                                  (source & 7))) ||
      (opcode == 0x6B
           ? !binary_code_buffer_append_u8(
                 buffer, (unsigned char)(int8_t)signed_immediate)
           : !binary_code_buffer_append_u32(buffer, immediate))) {
    return 0;
  }

  return 1;
}

static int binary_emit_unary_reg(BinaryCodeBuffer *buffer,
                                 unsigned char subopcode,
                                 BinaryGpRegister reg) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, 0, 0, reg >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0xF7) ||
      !binary_code_buffer_append_u8(
          buffer,
          (unsigned char)(0xC0 | ((subopcode & 7) << 3) | (reg & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_neg_reg(BinaryCodeBuffer *buffer, BinaryGpRegister reg) {
  return binary_emit_unary_reg(buffer, 3, reg);
}

static int binary_emit_not_reg(BinaryCodeBuffer *buffer, BinaryGpRegister reg) {
  return binary_emit_unary_reg(buffer, 2, reg);
}

static int binary_emit_idiv_reg(BinaryCodeBuffer *buffer,
                                BinaryGpRegister divisor) {
  return binary_emit_unary_reg(buffer, 7, divisor);
}

static int binary_emit_shift_reg_cl(BinaryCodeBuffer *buffer,
                                    unsigned char subopcode,
                                    BinaryGpRegister reg) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, 0, 0, reg >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0xD3) ||
      !binary_code_buffer_append_u8(
          buffer,
          (unsigned char)(0xC0 | ((subopcode & 7) << 3) | (reg & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_shift_reg_imm8(BinaryCodeBuffer *buffer,
                                      unsigned char subopcode,
                                      BinaryGpRegister reg,
                                      unsigned char immediate) {
  if (!buffer) {
    return 0;
  }
  if (immediate == 0) {
    return 1;
  }
  if (immediate == 1) {
    if (!binary_emit_rex(buffer, 1, 0, 0, reg >> 3) ||
        !binary_code_buffer_append_u8(buffer, 0xD1) ||
        !binary_code_buffer_append_u8(
            buffer,
            (unsigned char)(0xC0 | ((subopcode & 7) << 3) | (reg & 7)))) {
      return 0;
    }
    return 1;
  }

  if (!binary_emit_rex(buffer, 1, 0, 0, reg >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0xC1) ||
      !binary_code_buffer_append_u8(
          buffer,
          (unsigned char)(0xC0 | ((subopcode & 7) << 3) | (reg & 7))) ||
      !binary_code_buffer_append_u8(buffer, immediate)) {
    return 0;
  }

  return 1;
}

static int binary_emit_cqo(BinaryCodeBuffer *buffer) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, 0, 0, 0) ||
      !binary_code_buffer_append_u8(buffer, 0x99)) {
    return 0;
  }

  return 1;
}

static int binary_emit_setcc_al(BinaryCodeBuffer *buffer,
                                unsigned char condition_opcode) {
  if (!buffer) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, condition_opcode) ||
      !binary_code_buffer_append_u8(buffer, 0xC0)) {
    return 0;
  }

  return 1;
}

static int binary_emit_movzx_eax_al(BinaryCodeBuffer *buffer) {
  if (!buffer) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0xB6) ||
      !binary_code_buffer_append_u8(buffer, 0xC0)) {
    return 0;
  }

  return 1;
}

static int binary_emit_movzx_eax_ax(BinaryCodeBuffer *buffer) {
  if (!buffer) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0xB7) ||
      !binary_code_buffer_append_u8(buffer, 0xC0)) {
    return 0;
  }

  return 1;
}

static int binary_emit_movsx_rax_al(BinaryCodeBuffer *buffer) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, 0, 0, 0) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0xBE) ||
      !binary_code_buffer_append_u8(buffer, 0xC0)) {
    return 0;
  }

  return 1;
}

static int binary_emit_movsx_rax_ax(BinaryCodeBuffer *buffer) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, 0, 0, 0) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0xBF) ||
      !binary_code_buffer_append_u8(buffer, 0xC0)) {
    return 0;
  }

  return 1;
}

static int binary_emit_movsx_reg_reg8(BinaryCodeBuffer *buffer,
                                      BinaryGpRegister destination,
                                      BinaryGpRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0xBE) ||
      !binary_code_buffer_append_u8(
          buffer,
          (unsigned char)(0xC0 | ((destination & 7) << 3) | (source & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_movsx_reg_reg16(BinaryCodeBuffer *buffer,
                                       BinaryGpRegister destination,
                                       BinaryGpRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0xBF) ||
      !binary_code_buffer_append_u8(
          buffer,
          (unsigned char)(0xC0 | ((destination & 7) << 3) | (source & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_movsxd_rax_eax(BinaryCodeBuffer *buffer) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, 0, 0, 0) ||
      !binary_code_buffer_append_u8(buffer, 0x63) ||
      !binary_code_buffer_append_u8(buffer, 0xC0)) {
    return 0;
  }

  return 1;
}

static int binary_emit_movsxd_reg_reg32(BinaryCodeBuffer *buffer,
                                        BinaryGpRegister destination,
                                        BinaryGpRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 1, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x63) ||
      !binary_code_buffer_append_u8(
          buffer,
          (unsigned char)(0xC0 | ((destination & 7) << 3) | (source & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_mov_eax_eax(BinaryCodeBuffer *buffer) {
  if (!buffer) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0x89) ||
      !binary_code_buffer_append_u8(buffer, 0xC0)) {
    return 0;
  }

  return 1;
}

static int binary_emit_setcc_reg8(BinaryCodeBuffer *buffer,
                                  unsigned char condition_opcode,
                                  BinaryGpRegister reg) {
  if (!buffer) {
    return 0;
  }

  if ((int)reg >= 4) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, condition_opcode) ||
      !binary_code_buffer_append_u8(buffer, (unsigned char)(0xC0 | (reg & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_alu_reg8_reg8(BinaryCodeBuffer *buffer,
                                     unsigned char opcode,
                                     BinaryGpRegister destination,
                                     BinaryGpRegister source) {
  if (!buffer) {
    return 0;
  }

  if ((int)destination >= 4 || (int)source >= 4) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, opcode) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((source & 7) << 3) |
                                  (destination & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_sse_reg_reg(BinaryCodeBuffer *buffer,
                                   unsigned char mandatory_prefix,
                                   int rex_w, unsigned char opcode1,
                                   unsigned char opcode2,
                                   BinaryXmmRegister destination,
                                   BinaryXmmRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, mandatory_prefix) ||
      !binary_emit_rex(buffer, rex_w, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, opcode1) ||
      !binary_code_buffer_append_u8(buffer, opcode2) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((destination & 7) << 3) |
                                  (source & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_movq_xmm_reg(BinaryCodeBuffer *buffer,
                                    BinaryXmmRegister destination,
                                    BinaryGpRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0x66) ||
      !binary_emit_rex(buffer, 1, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0x6E) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((destination & 7) << 3) |
                                  (source & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_movq_reg_xmm(BinaryCodeBuffer *buffer,
                                    BinaryGpRegister destination,
                                    BinaryXmmRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0x66) ||
      !binary_emit_rex(buffer, 1, source >> 3, 0, destination >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0x7E) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((source & 7) << 3) |
                                  (destination & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_pxor_xmm_xmm(BinaryCodeBuffer *buffer,
                                    BinaryXmmRegister destination,
                                    BinaryXmmRegister source) {
  return binary_emit_sse_reg_reg(buffer, 0x66, 0, 0x0F, 0xEF, destination,
                                 source);
}

static int binary_emit_addsd_xmm_xmm(BinaryCodeBuffer *buffer,
                                     BinaryXmmRegister destination,
                                     BinaryXmmRegister source) {
  return binary_emit_sse_reg_reg(buffer, 0xF2, 0, 0x0F, 0x58, destination,
                                 source);
}

static int binary_emit_subsd_xmm_xmm(BinaryCodeBuffer *buffer,
                                     BinaryXmmRegister destination,
                                     BinaryXmmRegister source) {
  return binary_emit_sse_reg_reg(buffer, 0xF2, 0, 0x0F, 0x5C, destination,
                                 source);
}

static int binary_emit_mulsd_xmm_xmm(BinaryCodeBuffer *buffer,
                                     BinaryXmmRegister destination,
                                     BinaryXmmRegister source) {
  return binary_emit_sse_reg_reg(buffer, 0xF2, 0, 0x0F, 0x59, destination,
                                 source);
}

static int binary_emit_divsd_xmm_xmm(BinaryCodeBuffer *buffer,
                                     BinaryXmmRegister destination,
                                     BinaryXmmRegister source) {
  return binary_emit_sse_reg_reg(buffer, 0xF2, 0, 0x0F, 0x5E, destination,
                                 source);
}

static int binary_emit_ucomisd_xmm_xmm(BinaryCodeBuffer *buffer,
                                       BinaryXmmRegister lhs,
                                       BinaryXmmRegister rhs) {
  return binary_emit_sse_reg_reg(buffer, 0x66, 0, 0x0F, 0x2E, lhs, rhs);
}

static int binary_emit_cvttsd2si_reg_xmm(BinaryCodeBuffer *buffer,
                                         BinaryGpRegister destination,
                                         BinaryXmmRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0xF2) ||
      !binary_emit_rex(buffer, 1, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0x2C) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((destination & 7) << 3) |
                                  (source & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_cvtsi2sd_xmm_reg(BinaryCodeBuffer *buffer,
                                        BinaryXmmRegister destination,
                                        BinaryGpRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0xF2) ||
      !binary_emit_rex(buffer, 1, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0x2A) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((destination & 7) << 3) |
                                  (source & 7)))) {
    return 0;
  }

  return 1;
}

/* ---- Single-precision (float32) SSE encoders ----
 * These mirror the double-precision encoders above but use the F3 scalar-
 * single prefix / 32-bit operand forms. They exist so float32 values are
 * computed and converted at single precision instead of being silently
 * widened to double (which corrupts struct layout and ABI). */

/* movd xmm, r32 : 66 0F 6E /r  (no REX.W -> 32-bit GP source) */
static int binary_emit_movd_xmm_reg(BinaryCodeBuffer *buffer,
                                    BinaryXmmRegister destination,
                                    BinaryGpRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0x66) ||
      !binary_emit_rex(buffer, 0, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0x6E) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((destination & 7) << 3) |
                                  (source & 7)))) {
    return 0;
  }

  return 1;
}

/* movd r32, xmm : 66 0F 7E /r  (no REX.W -> 32-bit GP destination) */
static int binary_emit_movd_reg_xmm(BinaryCodeBuffer *buffer,
                                    BinaryGpRegister destination,
                                    BinaryXmmRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0x66) ||
      !binary_emit_rex(buffer, 0, source >> 3, 0, destination >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0x7E) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((source & 7) << 3) |
                                  (destination & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_addss_xmm_xmm(BinaryCodeBuffer *buffer,
                                     BinaryXmmRegister destination,
                                     BinaryXmmRegister source) {
  return binary_emit_sse_reg_reg(buffer, 0xF3, 0, 0x0F, 0x58, destination,
                                 source);
}

static int binary_emit_subss_xmm_xmm(BinaryCodeBuffer *buffer,
                                     BinaryXmmRegister destination,
                                     BinaryXmmRegister source) {
  return binary_emit_sse_reg_reg(buffer, 0xF3, 0, 0x0F, 0x5C, destination,
                                 source);
}

static int binary_emit_mulss_xmm_xmm(BinaryCodeBuffer *buffer,
                                     BinaryXmmRegister destination,
                                     BinaryXmmRegister source) {
  return binary_emit_sse_reg_reg(buffer, 0xF3, 0, 0x0F, 0x59, destination,
                                 source);
}

static int binary_emit_divss_xmm_xmm(BinaryCodeBuffer *buffer,
                                     BinaryXmmRegister destination,
                                     BinaryXmmRegister source) {
  return binary_emit_sse_reg_reg(buffer, 0xF3, 0, 0x0F, 0x5E, destination,
                                 source);
}

/* ucomiss xmm, xmm : NP 0F 2E /r  (no mandatory prefix, so cannot use
 * binary_emit_sse_reg_reg which always emits one). */
static int binary_emit_ucomiss_xmm_xmm(BinaryCodeBuffer *buffer,
                                       BinaryXmmRegister lhs,
                                       BinaryXmmRegister rhs) {
  if (!buffer) {
    return 0;
  }

  if (!binary_emit_rex(buffer, 0, lhs >> 3, 0, rhs >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0x2E) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((lhs & 7) << 3) | (rhs & 7)))) {
    return 0;
  }

  return 1;
}

/* cvttss2si r64, xmm : F3 REX.W 0F 2C /r  (truncating float32 -> int64) */
static int binary_emit_cvttss2si_reg_xmm(BinaryCodeBuffer *buffer,
                                         BinaryGpRegister destination,
                                         BinaryXmmRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0xF3) ||
      !binary_emit_rex(buffer, 1, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0x2C) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((destination & 7) << 3) |
                                  (source & 7)))) {
    return 0;
  }

  return 1;
}

/* cvtsi2ss xmm, r64 : F3 REX.W 0F 2A /r  (int64 -> float32) */
static int binary_emit_cvtsi2ss_xmm_reg(BinaryCodeBuffer *buffer,
                                        BinaryXmmRegister destination,
                                        BinaryGpRegister source) {
  if (!buffer) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0xF3) ||
      !binary_emit_rex(buffer, 1, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0x2A) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((destination & 7) << 3) |
                                  (source & 7)))) {
    return 0;
  }

  return 1;
}

/* cvtss2sd xmm, xmm : F3 0F 5A /r  (widen float32 -> float64) */
static int binary_emit_cvtss2sd_xmm_xmm(BinaryCodeBuffer *buffer,
                                        BinaryXmmRegister destination,
                                        BinaryXmmRegister source) {
  return binary_emit_sse_reg_reg(buffer, 0xF3, 0, 0x0F, 0x5A, destination,
                                 source);
}

/* cvtsd2ss xmm, xmm : F2 0F 5A /r  (narrow float64 -> float32) */
static int binary_emit_cvtsd2ss_xmm_xmm(BinaryCodeBuffer *buffer,
                                        BinaryXmmRegister destination,
                                        BinaryXmmRegister source) {
  return binary_emit_sse_reg_reg(buffer, 0xF2, 0, 0x0F, 0x5A, destination,
                                 source);
}

static int binary_emit_call_placeholder(BinaryCodeBuffer *buffer,
                                        size_t *displacement_offset_out) {
  if (!buffer || !displacement_offset_out) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0xE8)) {
    return 0;
  }

  *displacement_offset_out = buffer->size;
  return binary_code_buffer_append_u32(buffer, 0);
}

static int binary_emit_call_reg(BinaryCodeBuffer *buffer, BinaryGpRegister reg) {
  if (!buffer) {
    return 0;
  }

  if ((int)reg >= 8 && !binary_emit_rex(buffer, 0, 0, 0, 1)) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0xFF) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xD0 | (reg & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_jmp_placeholder(BinaryCodeBuffer *buffer,
                                       size_t *displacement_offset_out) {
  if (!buffer || !displacement_offset_out) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0xE9)) {
    return 0;
  }

  *displacement_offset_out = buffer->size;
  return binary_code_buffer_append_u32(buffer, 0);
}

static int binary_emit_jcc_placeholder(BinaryCodeBuffer *buffer,
                                       unsigned char condition_opcode,
                                       size_t *displacement_offset_out) {
  if (!buffer || !displacement_offset_out) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, condition_opcode)) {
    return 0;
  }

  *displacement_offset_out = buffer->size;
  return binary_code_buffer_append_u32(buffer, 0);
}

static int binary_emit_je_placeholder(BinaryCodeBuffer *buffer,
                                      size_t *displacement_offset_out) {
  return binary_emit_jcc_placeholder(buffer, 0x84, displacement_offset_out);
}

static int binary_emit_ret(BinaryCodeBuffer *buffer) {
  return buffer ? binary_code_buffer_append_u8(buffer, 0xC3) : 0;
}

static int binary_function_context_patch_rel32(BinaryFunctionContext *context,
                                               size_t displacement_offset,
                                               size_t target_offset) {
  if (!context || !context->code.data ||
      displacement_offset + sizeof(int32_t) > context->code.size) {
    return 0;
  }

  long long delta =
      (long long)target_offset - (long long)(displacement_offset + sizeof(int32_t));
  if (delta < INT32_MIN || delta > INT32_MAX) {
    return 0;
  }

  int32_t displacement = (int32_t)delta;
  memcpy(context->code.data + displacement_offset, &displacement,
         sizeof(displacement));
  return 1;
}

static int code_generator_binary_get_parameter_offset(
    BinaryFunctionContext *context, const char *name) {
  return binary_named_slot_table_get_offset(&context->parameter_slots, name);
}

static int code_generator_binary_get_local_offset(BinaryFunctionContext *context,
                                                  const char *name) {
  return binary_named_slot_table_get_offset(&context->local_slots, name);
}

static int code_generator_binary_get_temp_offset(BinaryFunctionContext *context,
                                                 const char *name) {
  return binary_named_slot_table_get_offset(&context->temp_slots, name);
}

static int code_generator_binary_get_symbol_offset(BinaryFunctionContext *context,
                                                   const char *name) {
  int offset = 0;
  if (!context || !name) {
    return -1;
  }

  offset = code_generator_binary_get_parameter_offset(context, name);
  if (offset > 0) {
    return offset;
  }

  return code_generator_binary_get_local_offset(context, name);
}

static int code_generator_binary_resolved_type_is_supported(Type *type,
                                                            int allow_void);

static int code_generator_binary_resolved_type_is_stack_scalar(Type *type) {
  if (!type) {
    return 0;
  }

  if (code_generator_binary_resolved_type_is_supported(type, 0)) {
    return 1;
  }

  return type->kind == TYPE_FLOAT64 && type->size == 8;
}

static int code_generator_binary_type_is_direct_aggregate(Type *type) {
  return type && code_generator_type_is_aggregate(type) &&
         code_generator_abi_classify(type) == ABI_PASS_DIRECT &&
         type->size > 0 && type->size <= 8;
}

static int code_generator_binary_resolved_type_is_float64(Type *type) {
  return type && type->kind == TYPE_FLOAT64 && type->size == 8;
}

/* IEEE-754 width of a resolved type: 32 for float32, 64 for float64, else 0
 * (not a floating type). */
static int code_generator_binary_resolved_type_float_bits(Type *type) {
  if (!type) {
    return 0;
  }
  if (type->kind == TYPE_FLOAT32 && type->size == 4) {
    return 32;
  }
  if (type->kind == TYPE_FLOAT64 && type->size == 8) {
    return 64;
  }
  return 0;
}

static int code_generator_binary_resolved_type_is_abi_supported(Type *type,
                                                                int allow_void) {
  if (!type) {
    return 0;
  }

  if (type->kind == TYPE_STRING) {
    return 1;
  }

  /* Aggregates are supported through the ABI classifier: DIRECT aggregates
   * are raw 1/2/4/8-byte register values; INDIRECT aggregates use hidden
   * pointers. */
  if (code_generator_type_is_aggregate(type)) {
    return 1;
  }

  return code_generator_binary_resolved_type_is_supported(type, allow_void);
}

static Type *code_generator_binary_get_resolved_type(CodeGenerator *generator,
                                                     const char *type_name,
                                                     int allow_void) {
  const char *resolved_name = NULL;

  if (!generator || !generator->type_checker) {
    return NULL;
  }

  resolved_name = type_name;
  if (!resolved_name || resolved_name[0] == '\0') {
    resolved_name = allow_void ? "void" : "int64";
  }

  return type_checker_get_type_by_name(generator->type_checker, resolved_name);
}

static int code_generator_binary_named_type_is_float64(CodeGenerator *generator,
                                                       const char *type_name,
                                                       int allow_void) {
  return code_generator_binary_resolved_type_is_float64(
      code_generator_binary_get_resolved_type(generator, type_name, allow_void));
}

/* Float width (0/32/64) of a named type, e.g. a parameter/local type name. */
static int code_generator_binary_named_type_float_bits(CodeGenerator *generator,
                                                       const char *type_name) {
  if (!type_name || type_name[0] == '\0') {
    return 0;
  }
  return code_generator_binary_resolved_type_float_bits(
      code_generator_binary_get_resolved_type(generator, type_name, 0));
}

static int code_generator_binary_is_marked_float64_symbol(
    const BinaryFunctionContext *context, const char *name) {
  return context && name &&
         binary_named_slot_table_get_offset(&context->float64_symbols, name) >=
             0;
}

/* The float64_symbols table doubles as a float-width map: the stored slot
 * value is the IEEE-754 width (32 or 64) of the named symbol/temp. Width 0
 * means "not recorded". */
static int code_generator_binary_marked_symbol_float_bits(
    const BinaryFunctionContext *context, const char *name) {
  int width = 0;
  if (!context || !name) {
    return 0;
  }
  width = binary_named_slot_table_get_offset(&context->float64_symbols, name);
  return (width == 32 || width == 64) ? width : 0;
}

static int code_generator_binary_mark_float_symbol(
    BinaryFunctionContext *context, const char *name, int bits) {
  if (!context || !name || name[0] == '\0') {
    return 0;
  }
  /* binary_named_slot_table_add fails a re-add with a different value, but a
   * symbol/temp may legitimately be visited by more than one marking pass
   * (declared-type pass and instruction-result pass). The first recorded
   * width is authoritative; treat an already-present entry as success
   * instead of aborting code generation. */
  if (binary_named_slot_table_get_offset(&context->float64_symbols, name) >=
      0) {
    return 1;
  }
  return binary_named_slot_table_add(&context->float64_symbols, name,
                                     (bits == 32) ? 32 : 64);
}

static int code_generator_binary_mark_float64_symbol(
    BinaryFunctionContext *context, const char *name) {
  return code_generator_binary_mark_float_symbol(context, name, 64);
}

static int code_generator_binary_symbol_is_scalar_accessible(
    CodeGenerator *generator, const char *name) {
  Symbol *symbol = NULL;

  if (!generator || !name || !generator->symbol_table) {
    return 1;
  }

  symbol = symbol_table_lookup(generator->symbol_table, name);
  if (!symbol || !symbol->type) {
    return 1;
  }

  /* Indirect parameters: the home slot holds a struct POINTER (8 bytes),
   * which is scalar-accessible even though the symbol's type is aggregate.
   * Downstream consumers use that pointer as the struct's base address. */
  if (symbol->kind == SYMBOL_PARAMETER &&
      symbol->data.variable.is_indirect_param) {
    return 1;
  }

  if (code_generator_binary_type_is_direct_aggregate(symbol->type)) {
    return 1;
  }

  return code_generator_binary_resolved_type_is_stack_scalar(symbol->type);
}

static int code_generator_binary_immediate_fits_signed_32(long long value) {
  return value >= INT32_MIN && value <= INT32_MAX;
}

static int code_generator_binary_extract_positive_power_of_two(
    long long value, unsigned int *shift_out, unsigned long long *mask_out) {
  unsigned long long uvalue = 0;
  unsigned int shift = 0;

  if (!shift_out || !mask_out || value <= 0) {
    return 0;
  }

  uvalue = (unsigned long long)value;
  if ((uvalue & (uvalue - 1ULL)) != 0ULL) {
    return 0;
  }

  while (uvalue > 1ULL) {
    uvalue >>= 1ULL;
    shift++;
  }

  *shift_out = shift;
  *mask_out = ((unsigned long long)value) - 1ULL;
  return 1;
}

static int code_generator_binary_emit_and_mask(BinaryFunctionContext *context,
                                               BinaryGpRegister target_register,
                                               unsigned long long mask) {
  if (!context) {
    return 0;
  }

  if (mask <= 0x7fffffffULL) {
    return binary_emit_and_reg_imm32(&context->code, target_register,
                                     (uint32_t)mask);
  }

  return binary_emit_mov_reg_imm64(&context->code, BINARY_GP_R10, mask) &&
         binary_emit_alu_reg_reg(&context->code, 0x21, target_register,
                                 BINARY_GP_R10);
}

static int code_generator_binary_x86_to_gp_register(x86Register source,
                                                    BinaryGpRegister *out) {
  if (!out) {
    return 0;
  }

  switch (source) {
  case REG_RAX:
    *out = BINARY_GP_RAX;
    return 1;
  case REG_RBX:
    *out = BINARY_GP_RBX;
    return 1;
  case REG_RCX:
    *out = BINARY_GP_RCX;
    return 1;
  case REG_RDX:
    *out = BINARY_GP_RDX;
    return 1;
  case REG_RSI:
    *out = BINARY_GP_RSI;
    return 1;
  case REG_RDI:
    *out = BINARY_GP_RDI;
    return 1;
  case REG_R8:
    *out = BINARY_GP_R8;
    return 1;
  case REG_R9:
    *out = BINARY_GP_R9;
    return 1;
  case REG_R10:
    *out = BINARY_GP_R10;
    return 1;
  case REG_R11:
    *out = BINARY_GP_R11;
    return 1;
  case REG_R12:
    *out = BINARY_GP_R12;
    return 1;
  case REG_R13:
    *out = BINARY_GP_R13;
    return 1;
  case REG_R14:
    *out = BINARY_GP_R14;
    return 1;
  case REG_R15:
    *out = BINARY_GP_R15;
    return 1;
  default:
    return 0;
  }
}

static int code_generator_binary_gp_register_is_win64_nonvolatile(
    BinaryGpRegister reg) {
  return reg == BINARY_GP_RBX || reg == BINARY_GP_RSI ||
         reg == BINARY_GP_RDI || reg == BINARY_GP_R12 ||
         reg == BINARY_GP_R13 || reg == BINARY_GP_R14 ||
         reg == BINARY_GP_R15;
}

static int code_generator_binary_context_add_saved_register(
    BinaryFunctionContext *context, BinaryGpRegister reg) {
  if (!context) {
    return 0;
  }

  for (size_t i = 0; i < context->saved_register_count; i++) {
    if (context->saved_registers[i] == reg) {
      return 1;
    }
  }

  if (context->saved_register_count >=
      sizeof(context->saved_registers) / sizeof(context->saved_registers[0])) {
    return 0;
  }

  context->saved_registers[context->saved_register_count++] = reg;
  return 1;
}

static int code_generator_binary_type_is_gp_promotable(Type *type) {
  if (!type || !code_generator_binary_resolved_type_is_supported(type, 0)) {
    return 0;
  }

  if (code_generator_binary_resolved_type_float_bits(type) != 0 ||
      type->kind == TYPE_STRING || type->kind == TYPE_VOID) {
    return 0;
  }

  return type->size > 0 && type->size <= 8;
}

static int code_generator_binary_instruction_writes_dest(IROpcode op) {
  switch (op) {
  case IR_OP_NOP:
  case IR_OP_LABEL:
  case IR_OP_JUMP:
  case IR_OP_BRANCH_ZERO:
  case IR_OP_BRANCH_EQ:
  case IR_OP_DECLARE_LOCAL:
    return 0;
  default:
    return 1;
  }
}

static size_t code_generator_binary_symbol_write_count(
    const IRFunction *function, const char *name) {
  size_t count = 0;
  if (!function || !name) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    if (!instruction ||
        !code_generator_binary_instruction_writes_dest(instruction->op) ||
        instruction->dest.kind != IR_OPERAND_SYMBOL ||
        !instruction->dest.name) {
      continue;
    }
    if (strcmp(instruction->dest.name, name) == 0) {
      count++;
    }
  }

  return count;
}

static int code_generator_binary_collect_symbol_aliases(
    CodeGenerator *generator, BinaryFunctionContext *context,
    IRFunction *ir_function) {
  if (!generator || !context || !ir_function) {
    return 0;
  }

  for (size_t i = 0; i < ir_function->instruction_count; i++) {
    const IRInstruction *instruction = &ir_function->instructions[i];
    const char *name = NULL;
    const char *target = NULL;
    Symbol *symbol = NULL;
    Symbol *target_symbol = NULL;

    if (!instruction || instruction->op != IR_OP_ASSIGN ||
        instruction->dest.kind != IR_OPERAND_SYMBOL ||
        instruction->lhs.kind != IR_OPERAND_SYMBOL ||
        !instruction->dest.name || !instruction->lhs.name) {
      continue;
    }

    name = instruction->dest.name;
    target = instruction->lhs.name;
    if (strcmp(name, target) == 0 ||
        code_generator_binary_get_local_offset(context, name) <= 0 ||
        code_generator_binary_get_symbol_offset(context, target) <= 0 ||
        code_generator_binary_symbol_write_count(ir_function, name) != 1 ||
        binary_named_slot_table_get_offset(&context->address_taken_symbols,
                                           name) >= 0 ||
        binary_named_slot_table_get_offset(&context->address_taken_symbols,
                                           target) >= 0 ||
        binary_symbol_alias_table_get(&context->symbol_aliases, target)) {
      continue;
    }

    symbol = generator->symbol_table
                 ? symbol_table_lookup(generator->symbol_table, name)
                 : NULL;
    target_symbol = generator->symbol_table
                        ? symbol_table_lookup(generator->symbol_table, target)
                        : NULL;
    if ((symbol && symbol->type &&
         !code_generator_binary_type_is_gp_promotable(symbol->type)) ||
        (target_symbol && target_symbol->type &&
         !code_generator_binary_type_is_gp_promotable(target_symbol->type)) ||
        code_generator_binary_marked_symbol_float_bits(context, name) ||
        code_generator_binary_marked_symbol_float_bits(context, target) ||
        !code_generator_binary_symbol_is_scalar_accessible(generator, name) ||
        !code_generator_binary_symbol_is_scalar_accessible(generator, target)) {
      continue;
    }

    if (!binary_symbol_alias_table_add(&context->symbol_aliases, name,
                                       target)) {
      code_generator_set_error(
          generator,
          "Failed to record local alias '%s' in direct object function '%s'",
          name, context->function_name);
      return 0;
    }
  }

  return 1;
}

static int code_generator_binary_operand_mentions_symbol(
    const IROperand *operand, const char *name) {
  return operand && operand->kind == IR_OPERAND_SYMBOL && operand->name &&
         name && strcmp(operand->name, name) == 0;
}

static int code_generator_binary_operand_mentions_symbol_or_alias(
    const BinaryFunctionContext *context, const IROperand *operand,
    const char *name) {
  const char *alias_target = NULL;
  if (code_generator_binary_operand_mentions_symbol(operand, name)) {
    return 1;
  }
  if (!context || !operand || operand->kind != IR_OPERAND_SYMBOL ||
      !operand->name || !name) {
    return 0;
  }
  alias_target =
      binary_symbol_alias_table_get(&context->symbol_aliases, operand->name);
  return alias_target && strcmp(alias_target, name) == 0;
}

static int code_generator_binary_instruction_in_backward_loop(
    const IRFunction *function, size_t instruction_index) {
  if (!function || instruction_index >= function->instruction_count) {
    return 0;
  }

  for (size_t jump_index = instruction_index + 1;
       jump_index < function->instruction_count; jump_index++) {
    const IRInstruction *jump = &function->instructions[jump_index];
    if (!jump || jump->op != IR_OP_JUMP || !jump->text) {
      continue;
    }

    for (size_t label_index = 0; label_index <= instruction_index;
         label_index++) {
      const IRInstruction *label = &function->instructions[label_index];
      if (label && label->op == IR_OP_LABEL && label->text &&
          strcmp(label->text, jump->text) == 0) {
        return 1;
      }
    }
  }

  return 0;
}

static size_t *code_generator_binary_build_loop_weights(
    const IRFunction *function) {
  if (!function) {
    return NULL;
  }

  size_t count = function->instruction_count;
  size_t *weights = malloc((count ? count : 1) * sizeof(size_t));
  if (!weights) {
    return NULL;
  }

  for (size_t i = 0; i < count; i++) {
    weights[i] = 1;
  }

  /* Weight each instruction by 4^(loop nesting depth) so that values used in
   * inner loops outscore those used only in outer loops. A back-jump to an
   * earlier label marks [label, jump] as one loop body; nested bodies multiply,
   * matching how often the instruction actually executes. Without compounding,
   * a hot innermost temporary (e.g. the insertion-sort scan value) ties with
   * every outer-loop variable and loses the register-promotion contest. */
  for (size_t jump_index = 0; jump_index < count; jump_index++) {
    const IRInstruction *jump = &function->instructions[jump_index];
    if (!jump || jump->op != IR_OP_JUMP || !jump->text) {
      continue;
    }

    for (size_t label_index = 0; label_index < jump_index; label_index++) {
      const IRInstruction *label = &function->instructions[label_index];
      if (!label || label->op != IR_OP_LABEL || !label->text ||
          strcmp(label->text, jump->text) != 0) {
        continue;
      }

      for (size_t i = label_index; i <= jump_index; i++) {
        /* Cap to avoid overflow on pathologically deep nesting; 4^10 already
         * dwarfs any realistic outer-loop score. */
        if (weights[i] <= (size_t)262144) {
          weights[i] *= 4;
        }
      }
      break;
    }
  }

  return weights;
}

static size_t code_generator_binary_function_symbol_score(
    const BinaryFunctionContext *context, const IRFunction *function,
    const char *name, const size_t *loop_weights) {
  size_t score = 0;

  if (!function || !name) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    size_t weight = loop_weights ? loop_weights[i] : 1;
    if (!instruction) {
      continue;
    }

    if (code_generator_binary_operand_mentions_symbol_or_alias(
            context, &instruction->dest, name)) {
      score += weight;
    }
    if (code_generator_binary_operand_mentions_symbol_or_alias(
            context, &instruction->lhs, name)) {
      score += weight;
    }
    if (code_generator_binary_operand_mentions_symbol_or_alias(
            context, &instruction->rhs, name)) {
      score += weight;
    }
    for (size_t arg_index = 0; arg_index < instruction->argument_count;
         arg_index++) {
      if (code_generator_binary_operand_mentions_symbol_or_alias(
              context, &instruction->arguments[arg_index], name)) {
        score += weight;
      }
    }
  }

  return score;
}

static int code_generator_binary_symbol_already_promoted(
    BinaryFunctionContext *context, const char *name) {
  return context && name &&
         binary_named_slot_table_get_offset(&context->register_symbols, name) >=
             0;
}

static int code_generator_binary_symbol_assigned_register(
    CodeGenerator *generator, BinaryFunctionContext *context, const char *name,
    BinaryGpRegister *register_out) {
  Symbol *symbol = NULL;
  BinaryGpRegister mapped = BINARY_GP_RAX;
  int promoted_register = -1;

  if (!generator || !context || !name || !register_out) {
    return 0;
  }

  if (binary_named_slot_table_get_offset(&context->address_taken_symbols,
                                         name) >= 0) {
    return 0;
  }

  if (code_generator_binary_get_symbol_offset(context, name) <= 0) {
    return 0;
  }

  promoted_register =
      binary_named_slot_table_get_offset(&context->register_symbols, name);
  if (promoted_register >= 0) {
    mapped = (BinaryGpRegister)promoted_register;
    if (code_generator_binary_gp_register_is_win64_nonvolatile(mapped)) {
      *register_out = mapped;
      return 1;
    }
  }

  symbol = generator->symbol_table ? symbol_table_lookup(generator->symbol_table,
                                                         name)
                                   : NULL;
  if (!symbol || !symbol->type || !symbol->data.variable.is_in_register) {
    return 0;
  }

  if (!code_generator_binary_resolved_type_is_supported(symbol->type, 0) ||
      code_generator_binary_resolved_type_float_bits(symbol->type) != 0 ||
      symbol->type->kind == TYPE_STRING) {
    return 0;
  }

  if (!code_generator_binary_x86_to_gp_register(
          (x86Register)symbol->data.variable.register_id, &mapped) ||
      !code_generator_binary_gp_register_is_win64_nonvolatile(mapped)) {
    return 0;
  }

  *register_out = mapped;
  return 1;
}

static int code_generator_binary_function_has_calls(const IRFunction *function) {
  if (!function) {
    return 0;
  }
  for (size_t i = 0; i < function->instruction_count; i++) {
    IROpcode op = function->instructions[i].op;
    if (op == IR_OP_CALL || op == IR_OP_CALL_INDIRECT) {
      return 1;
    }
  }
  return 0;
}

static int code_generator_binary_function_can_promote_rsi_rdi(
    CodeGenerator *generator, IRFunction *function, Type *return_type) {
  if (!generator || !function) {
    return 0;
  }

  if (code_generator_abi_classify(return_type) == ABI_PASS_INDIRECT) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    if (!instruction) {
      continue;
    }

    if (instruction->ast_ref || instruction->op == IR_OP_CALL_INDIRECT ||
        instruction->op == IR_OP_INLINE_ASM || instruction->op == IR_OP_NEW) {
      return 0;
    }

    if (instruction->op != IR_OP_CALL || !instruction->text) {
      continue;
    }

    Symbol *callee = generator->symbol_table
                         ? symbol_table_lookup(generator->symbol_table,
                                               instruction->text)
                         : NULL;
    Type *callee_return = NULL;
    if (callee && callee->kind == SYMBOL_FUNCTION) {
      callee_return = callee->data.function.return_type
                          ? callee->data.function.return_type
                          : callee->type;
    }
    if (code_generator_abi_classify(callee_return) == ABI_PASS_INDIRECT) {
      return 0;
    }

    if (callee && callee->kind == SYMBOL_FUNCTION &&
        callee->data.function.parameter_types) {
      for (size_t arg_i = 0; arg_i < instruction->argument_count &&
                             arg_i < callee->data.function.parameter_count;
           arg_i++) {
        Type *arg_type = callee->data.function.parameter_types[arg_i];
        if (code_generator_abi_classify(arg_type) == ABI_PASS_INDIRECT) {
          return 0;
        }
      }
    }
  }

  return 1;
}

static int code_generator_binary_promote_hot_symbols(
    CodeGenerator *generator, BinaryFunctionContext *context,
    FunctionDeclaration *function_data, IRFunction *ir_function) {
  static const BinaryGpRegister promotion_registers[] = {
      BINARY_GP_R12, BINARY_GP_R13, BINARY_GP_R14, BINARY_GP_R15,
      BINARY_GP_RBX, BINARY_GP_RSI, BINARY_GP_RDI};

  if (!generator || !context || !function_data || !ir_function) {
    return 0;
  }

  Type *return_type = code_generator_binary_get_resolved_type(
      generator, function_data->return_type, 1);
  size_t max_promoted =
      sizeof(promotion_registers) / sizeof(promotion_registers[0]);
  if (!code_generator_binary_function_can_promote_rsi_rdi(
          generator, ir_function, return_type) &&
      max_promoted >= 2) {
    max_promoted -= 2;
  }
  size_t promoted_count = 0;
  int function_has_no_calls =
      !code_generator_binary_function_has_calls(ir_function);
  size_t *loop_weights =
      code_generator_binary_build_loop_weights(ir_function);
  if (!loop_weights) {
    code_generator_set_error(
        generator,
        "Failed to allocate loop-weight metadata for direct object function "
        "'%s'",
        function_data->name);
    return 0;
  }

  if (function_has_no_calls) {
    for (size_t insn_i = 0;
         insn_i < ir_function->instruction_count && promoted_count < max_promoted;
         insn_i++) {
      const IRInstruction *insn = &ir_function->instructions[insn_i];
      const IROperand *operands[3];
      size_t op_i = 0;

      if (insn->op != IR_OP_ROTATE_ADD) {
        continue;
      }

      operands[0] = &insn->dest;
      operands[1] = &insn->lhs;
      operands[2] = &insn->rhs;
      for (op_i = 0; op_i < 3 && promoted_count < max_promoted; op_i++) {
        const char *name = operands[op_i]->name;
        Type *type = NULL;
        if (operands[op_i]->kind != IR_OPERAND_SYMBOL || !name ||
            binary_symbol_alias_table_get(&context->symbol_aliases, name) ||
            code_generator_binary_symbol_already_promoted(context, name) ||
            binary_named_slot_table_get_offset(&context->address_taken_symbols,
                                               name) >= 0) {
          continue;
        }

        type = code_generator_binary_get_resolved_type(generator, "int64", 0);
        if (!code_generator_binary_type_is_gp_promotable(type)) {
          continue;
        }

        if (!binary_named_slot_table_add(
                &context->register_symbols, name,
                (int)promotion_registers[promoted_count]) ||
            !code_generator_binary_context_add_saved_register(
                context, promotion_registers[promoted_count])) {
          return 0;
        }
        promoted_count++;
      }
    }
  }

  for (size_t reg_index = promoted_count;
       reg_index < max_promoted;
       reg_index++) {
    const char *best_name = NULL;
    size_t best_score = 0;

    for (size_t i = 0; i < function_data->parameter_count; i++) {
      const char *name = function_data->parameter_names[i];
      Type *type = NULL;
      if (!name || code_generator_binary_symbol_already_promoted(context, name) ||
          binary_symbol_alias_table_get(&context->symbol_aliases, name) ||
          binary_named_slot_table_get_offset(&context->address_taken_symbols,
                                             name) >= 0) {
        continue;
      }

      type = code_generator_binary_get_resolved_type(
          generator,
          function_data->parameter_types ? function_data->parameter_types[i]
                                         : NULL,
          0);
      if (!code_generator_binary_type_is_gp_promotable(type)) {
        continue;
      }

      size_t score =
          code_generator_binary_function_symbol_score(context, ir_function,
                                                      name, loop_weights);
      if (score > best_score) {
        best_score = score;
        best_name = name;
      }
    }

    if (!best_name || best_score < 2) {
      for (size_t i = 0; i < ir_function->instruction_count; i++) {
        const IRInstruction *instruction = &ir_function->instructions[i];
        const char *name = NULL;
        Type *type = NULL;
        if (!instruction || instruction->op != IR_OP_DECLARE_LOCAL ||
            instruction->dest.kind != IR_OPERAND_SYMBOL ||
            !instruction->dest.name) {
          continue;
        }

        name = instruction->dest.name;
        if (code_generator_binary_symbol_already_promoted(context, name) ||
            binary_symbol_alias_table_get(&context->symbol_aliases, name) ||
            binary_named_slot_table_get_offset(&context->address_taken_symbols,
                                               name) >= 0) {
          continue;
        }

        type = code_generator_binary_get_resolved_type(
            generator,
            instruction->text && instruction->text[0] != '\0' ? instruction->text
                                                              : "int64",
            0);
        if (!code_generator_binary_type_is_gp_promotable(type)) {
          continue;
        }

        size_t score =
            code_generator_binary_function_symbol_score(context, ir_function,
                                                        name, loop_weights);
        if (score > best_score) {
          best_score = score;
          best_name = name;
        }
      }
    }

    if (!best_name || best_score < 2) {
      break;
    }

    if (!binary_named_slot_table_add(&context->register_symbols, best_name,
                                     (int)promotion_registers[reg_index]) ||
        !code_generator_binary_context_add_saved_register(
            context, promotion_registers[reg_index])) {
      code_generator_set_error(
          generator,
          "Failed to promote hot symbol '%s' in direct object function '%s'",
          best_name, function_data->name);
      free(loop_weights);
      return 0;
    }
  }

  free(loop_weights);
  return 1;
}

static int code_generator_binary_resolved_type_is_signed_integer(Type *type) {
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

static int code_generator_binary_resolved_type_scalar_size(Type *type) {
  if (!type) {
    return 8;
  }

  if (type->kind == TYPE_POINTER || type->kind == TYPE_FUNCTION_POINTER) {
    return 8;
  }

  if (type->size > 0 && type->size <= 8) {
    return (int)type->size;
  }

  return 8;
}

static int code_generator_binary_resolved_type_is_supported(Type *type,
                                                            int allow_void) {
  if (!type) {
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
  case TYPE_FLOAT32:
  case TYPE_FLOAT64:
  case TYPE_POINTER:
  case TYPE_ENUM:
  case TYPE_FUNCTION_POINTER:
    return type->size <= 8;
  case TYPE_VOID:
    return allow_void;
  default:
    return 0;
  }
}

static int code_generator_binary_type_is_abi_supported(CodeGenerator *generator,
                                                       const char *type_name,
                                                       int allow_void) {
  if (!generator || !generator->type_checker) {
    return 1;
  }

  Type *type =
      code_generator_binary_get_resolved_type(generator, type_name, allow_void);
  if (!type) {
    return 0;
  }

  return code_generator_binary_resolved_type_is_abi_supported(type, allow_void);
}

static int code_generator_binary_type_is_cstring(Type *type) {
  return type && type->kind == TYPE_POINTER && type->name &&
         strcmp(type->name, "cstring") == 0;
}

static int code_generator_binary_type_is_string(Type *type) {
  return type && type->kind == TYPE_STRING;
}

static Type *code_generator_binary_get_operand_type(CodeGenerator *generator,
                                                    const IROperand *operand) {
  Symbol *symbol = NULL;

  if (!generator || !operand) {
    return NULL;
  }

  switch (operand->kind) {
  case IR_OPERAND_STRING:
    return generator->type_checker ? generator->type_checker->builtin_string
                                   : NULL;

  case IR_OPERAND_SYMBOL:
    if (!generator->symbol_table || !operand->name) {
      return NULL;
    }
    symbol = symbol_table_lookup(generator->symbol_table, operand->name);
    return symbol ? symbol->type : NULL;

  default:
    return NULL;
  }
}

static int code_generator_binary_validate_signature(CodeGenerator *generator,
                                                    FunctionDeclaration *function_data,
                                                    IRFunction *ir_function) {
  if (!generator || !function_data || !ir_function) {
    return 0;
  }

  if (function_data->parameter_count != ir_function->parameter_count) {
    code_generator_set_error(
        generator,
        "IR parameter mismatch while lowering direct object function '%s'",
        function_data->name);
    return 0;
  }

  if (!code_generator_binary_type_is_abi_supported(generator,
                                                   function_data->return_type, 1)) {
    code_generator_set_error(
        generator,
        "Direct object backend only supports integer/pointer/string/float64 "
        "returns in function '%s'",
        function_data->name);
    return 0;
  }

  for (size_t i = 0; i < function_data->parameter_count; i++) {
    const char *type_name = function_data->parameter_types
                                ? function_data->parameter_types[i]
                                : NULL;
    if (!code_generator_binary_type_is_abi_supported(generator, type_name, 0)) {
      code_generator_set_error(
          generator,
          "Direct object backend only supports integer/pointer/string/float64 "
          "parameters in function '%s'",
          function_data->name);
      return 0;
    }
  }

  return 1;
}

static int code_generator_binary_instruction_result_is_float64(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction);
static int code_generator_binary_instruction_result_float_bits(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction);
static int code_generator_binary_operand_float_bits(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *operand);

static int code_generator_binary_prepare_function_context(
    CodeGenerator *generator, FunctionDeclaration *function_data,
    IRFunction *ir_function, BinaryFunctionContext *context) {
  if (!generator || !function_data || !ir_function || !context) {
    return 0;
  }

  memset(context, 0, sizeof(*context));
  context->function_data = function_data;
  context->function_name = function_data->name;
  context->return_is_float64 = code_generator_binary_resolved_type_is_float64(
      code_generator_binary_get_resolved_type(generator,
                                              function_data->return_type, 1));
  context->return_float_bits = code_generator_binary_resolved_type_float_bits(
      code_generator_binary_get_resolved_type(generator,
                                              function_data->return_type, 1));

  int parameter_home_size = 0;
  if (function_data->parameter_count >
      (size_t)(INT_MAX / BINARY_FUNCTION_STACK_SLOT_SIZE)) {
    code_generator_set_error(generator,
                             "Too many parameters in function '%s'",
                             function_data->name);
    return 0;
  }
  parameter_home_size =
      (int)(function_data->parameter_count * BINARY_FUNCTION_STACK_SLOT_SIZE);

  for (size_t i = 0; i < ir_function->instruction_count; i++) {
    const IRInstruction *instruction = &ir_function->instructions[i];
    if (!instruction || instruction->op != IR_OP_ADDRESS_OF ||
        instruction->lhs.kind != IR_OPERAND_SYMBOL || !instruction->lhs.name) {
      continue;
    }
    if (!binary_named_slot_table_add(&context->address_taken_symbols,
                                     instruction->lhs.name, 1)) {
      code_generator_set_error(
          generator,
          "Failed to record address-taken symbol metadata in function '%s'",
          function_data->name);
      binary_function_context_destroy(context);
      return 0;
    }
  }

  /* Does this function return INDIRECT? The Win64 ABI passes the hidden
   * out-pointer as the first integer argument, consuming home slot 0 and
   * shifting user-parameter homes up by one. */
  Type *fn_return_type =
      function_data->return_type
          ? code_generator_binary_get_resolved_type(
                generator, function_data->return_type, 1)
          : NULL;
  int has_hidden_return =
      (code_generator_abi_classify(fn_return_type) == ABI_PASS_INDIRECT) ? 1 : 0;
  if (has_hidden_return) {
    /* Account for the extra home slot in parameter_home_size so the frame
     * layout includes room for the hidden pointer. */
    if (function_data->parameter_count >
        (size_t)(INT_MAX / BINARY_FUNCTION_STACK_SLOT_SIZE - 1)) {
      code_generator_set_error(generator,
                               "Too many parameters in function '%s'",
                               function_data->name);
      return 0;
    }
    parameter_home_size += BINARY_FUNCTION_STACK_SLOT_SIZE;
  }
  context->returns_indirect = has_hidden_return;
  context->indirect_return_size =
      has_hidden_return ? code_generator_abi_type_size(fn_return_type) : 0;

  for (size_t i = 0; i < function_data->parameter_count; i++) {
    const char *parameter_name = function_data->parameter_names[i];
    int offset = (int)((i + 1 + (has_hidden_return ? 1 : 0)) *
                       BINARY_FUNCTION_STACK_SLOT_SIZE);
    if (!parameter_name ||
        !binary_named_slot_table_add(&context->parameter_slots, parameter_name,
                                     offset)) {
      code_generator_set_error(
          generator,
          "Failed to allocate parameter slot metadata in function '%s'",
          function_data->name);
      binary_function_context_destroy(context);
      return 0;
    }

    /* Mark INDIRECT parameters on the symbol so load/lvalue paths know to
     * deref the home slot (which holds a pointer, not the struct itself). */
    {
      Type *param_type =
          function_data->parameter_types
              ? code_generator_binary_get_resolved_type(
                    generator, function_data->parameter_types[i], 0)
              : NULL;
      if (code_generator_abi_classify(param_type) == ABI_PASS_INDIRECT) {
        Symbol *param_sym =
            symbol_table_lookup(generator->symbol_table, parameter_name);
        if (param_sym && param_sym->kind == SYMBOL_PARAMETER) {
          param_sym->data.variable.is_indirect_param = 1;
        }
      }
    }

    {
      int param_fbits = code_generator_binary_named_type_float_bits(
          generator, function_data->parameter_types
                         ? function_data->parameter_types[i]
                         : NULL);
      if (param_fbits &&
          !code_generator_binary_mark_float_symbol(context, parameter_name,
                                                   param_fbits)) {
        code_generator_set_error(
            generator,
            "Failed to allocate float parameter metadata in function '%s'",
            function_data->name);
        binary_function_context_destroy(context);
        return 0;
      }
    }
  }

  size_t temp_slot_count = 0;
  size_t local_slot_count = 0;
  int local_storage_size_total = 0;
  for (size_t i = 0; i < ir_function->instruction_count; i++) {
    const IRInstruction *instruction = &ir_function->instructions[i];
    Type *local_type = NULL;
    int local_alignment = 0;
    int local_storage_size = 0;
    int scalar_local = 0;
    int existing_offset = 0;

    if (!instruction || instruction->op != IR_OP_DECLARE_LOCAL) {
      continue;
    }

    if (instruction->dest.kind != IR_OPERAND_SYMBOL || !instruction->dest.name ||
        instruction->dest.name[0] == '\0') {
      code_generator_set_error(
          generator, "Malformed local declaration in function '%s'",
          function_data->name);
      binary_function_context_destroy(context);
      return 0;
    }

    local_type = generator->type_checker
                     ? type_checker_get_type_by_name(generator->type_checker,
                                                     instruction->text)
                     : NULL;
    if (!local_type || local_type->kind == TYPE_VOID || local_type->size == 0) {
      code_generator_set_error(
          generator,
          "Direct object backend does not support local type '%s' in function "
          "'%s'",
          instruction->text ? instruction->text : "<unknown>",
          function_data->name);
      binary_function_context_destroy(context);
      return 0;
    }

    scalar_local = code_generator_binary_resolved_type_is_stack_scalar(local_type) ||
                   code_generator_binary_type_is_direct_aggregate(local_type);
    local_alignment = scalar_local ? BINARY_FUNCTION_STACK_SLOT_SIZE
                                   : (int)local_type->alignment;
    if (local_alignment <= 0) {
      local_alignment = 1;
    }

    local_storage_size = scalar_local ? BINARY_FUNCTION_STACK_SLOT_SIZE
                                      : (int)local_type->size;
    if (local_storage_size <= 0) {
      code_generator_set_error(generator,
                               "Invalid local storage size in function '%s'",
                               function_data->name);
      binary_function_context_destroy(context);
      return 0;
    }

    existing_offset =
        binary_named_slot_table_get_offset(&context->local_slots,
                                           instruction->dest.name);
    if (existing_offset > 0) {
      int local_fbits =
          code_generator_binary_resolved_type_float_bits(local_type);
      if (local_fbits &&
          !code_generator_binary_mark_float_symbol(context,
                                                   instruction->dest.name,
                                                   local_fbits)) {
        code_generator_set_error(
            generator,
            "Failed to allocate float local metadata in function '%s'",
            function_data->name);
        binary_function_context_destroy(context);
        return 0;
      }
      continue;
    }

    if (!binary_align_up_int(local_storage_size_total, local_alignment,
                             &local_storage_size_total) ||
        local_storage_size_total > INT_MAX - local_storage_size) {
      code_generator_set_error(generator, "Stack frame too large in function '%s'",
                               function_data->name);
      binary_function_context_destroy(context);
      return 0;
    }

    local_storage_size_total += local_storage_size;
    local_slot_count++;
    if (local_slot_count > (size_t)(INT_MAX / BINARY_FUNCTION_STACK_SLOT_SIZE)) {
      code_generator_set_error(generator, "Too many locals in function '%s'",
                               function_data->name);
      binary_function_context_destroy(context);
      return 0;
    }

    if (!binary_named_slot_table_add(
            &context->local_slots, instruction->dest.name,
            parameter_home_size + local_storage_size_total)) {
      code_generator_set_error(
          generator,
          "Failed to allocate local slot metadata in function '%s'",
          function_data->name);
      binary_function_context_destroy(context);
      return 0;
    }

    {
      int local_fbits =
          code_generator_binary_resolved_type_float_bits(local_type);
      if (local_fbits &&
          !code_generator_binary_mark_float_symbol(context,
                                                   instruction->dest.name,
                                                   local_fbits)) {
        code_generator_set_error(
            generator,
            "Failed to allocate float local metadata in function '%s'",
            function_data->name);
        binary_function_context_destroy(context);
        return 0;
      }
    }
  }

  for (size_t i = 0; i < ir_function->instruction_count; i++) {
    const IRInstruction *instruction = &ir_function->instructions[i];
    if (!instruction || instruction->dest.kind != IR_OPERAND_TEMP ||
        !instruction->dest.name) {
      continue;
    }

    if (binary_named_slot_table_get_offset(&context->temp_slots,
                                           instruction->dest.name) >= 0) {
      continue;
    }

    temp_slot_count++;
    if (temp_slot_count > (size_t)(INT_MAX / BINARY_FUNCTION_STACK_SLOT_SIZE)) {
      code_generator_set_error(
          generator, "Too many temporaries in function '%s'",
          function_data->name);
      binary_function_context_destroy(context);
      return 0;
    }

    int offset =
        parameter_home_size + local_storage_size_total +
        (int)(temp_slot_count * BINARY_FUNCTION_STACK_SLOT_SIZE);
    if (!binary_named_slot_table_add(&context->temp_slots,
                                     instruction->dest.name, offset)) {
      code_generator_set_error(
          generator, "Failed to allocate temp slot metadata in function '%s'",
          function_data->name);
      binary_function_context_destroy(context);
      return 0;
    }
  }

  for (size_t i = 0; i < ir_function->instruction_count; i++) {
    const IRInstruction *instruction = &ir_function->instructions[i];
    if (!instruction || !instruction->dest.name ||
        (instruction->dest.kind != IR_OPERAND_SYMBOL &&
         instruction->dest.kind != IR_OPERAND_TEMP)) {
      continue;
    }

    {
      int result_fbits = code_generator_binary_instruction_result_float_bits(
          generator, context, instruction);
      if (!result_fbits) {
        continue;
      }
      if (!code_generator_binary_mark_float_symbol(
              context, instruction->dest.name, result_fbits)) {
        code_generator_set_error(
            generator,
            "Failed to allocate float temporary metadata in function '%s'",
            function_data->name);
        binary_function_context_destroy(context);
        return 0;
      }
    }
  }

  if (!code_generator_binary_collect_symbol_aliases(generator, context,
                                                    ir_function)) {
    binary_function_context_destroy(context);
    return 0;
  }

  if (!code_generator_binary_promote_hot_symbols(generator, context,
                                                 function_data, ir_function)) {
    binary_function_context_destroy(context);
    return 0;
  }

  for (size_t i = 0; i < function_data->parameter_count; i++) {
    BinaryGpRegister assigned_register = BINARY_GP_RAX;
    const char *parameter_name = function_data->parameter_names[i];
    if (code_generator_binary_symbol_assigned_register(
            generator, context, parameter_name, &assigned_register) &&
        !code_generator_binary_context_add_saved_register(context,
                                                          assigned_register)) {
      code_generator_set_error(
          generator,
          "Too many callee-saved register-backed symbols in function '%s'",
          function_data->name);
      binary_function_context_destroy(context);
      return 0;
    }
  }

  for (size_t i = 0; i < ir_function->instruction_count; i++) {
    const IRInstruction *instruction = &ir_function->instructions[i];
    BinaryGpRegister assigned_register = BINARY_GP_RAX;
    if (!instruction || instruction->op != IR_OP_DECLARE_LOCAL ||
        instruction->dest.kind != IR_OPERAND_SYMBOL ||
        !instruction->dest.name) {
      continue;
    }
    if (code_generator_binary_symbol_assigned_register(
            generator, context, instruction->dest.name, &assigned_register) &&
        !code_generator_binary_context_add_saved_register(context,
                                                          assigned_register)) {
      code_generator_set_error(
          generator,
          "Too many callee-saved register-backed symbols in function '%s'",
          function_data->name);
      binary_function_context_destroy(context);
      return 0;
    }
  }

  int local_home_size = local_storage_size_total;
  if (!binary_align_up_int(local_home_size, BINARY_FUNCTION_STACK_SLOT_SIZE,
                           &local_home_size)) {
    code_generator_set_error(generator,
                             "Stack frame too large in function '%s'",
                             function_data->name);
    binary_function_context_destroy(context);
    return 0;
  }
  int temp_home_size = (int)(temp_slot_count * BINARY_FUNCTION_STACK_SLOT_SIZE);
  if (parameter_home_size > INT_MAX - local_home_size ||
      parameter_home_size + local_home_size > INT_MAX - temp_home_size) {
    code_generator_set_error(generator,
                             "Stack frame too large in function '%s'",
                             function_data->name);
    binary_function_context_destroy(context);
    return 0;
  }

  /* Reserve a function-level slot for each IR_OP_CALL whose return type is
   * INDIRECT. Each slot's rbp offset goes into context->indirect_return_slot_offsets
   * in instruction order and is consumed by emit_call. */
  int indirect_return_total = 0;
  for (size_t pp_i = 0; pp_i < ir_function->instruction_count; pp_i++) {
    const IRInstruction *pp_insn = &ir_function->instructions[pp_i];
    if (pp_insn->op != IR_OP_CALL || !pp_insn->text) continue;
    Symbol *callee =
        symbol_table_lookup(generator->symbol_table, pp_insn->text);
    Type *ret_t = NULL;
    if (callee && callee->kind == SYMBOL_FUNCTION) {
      ret_t = callee->data.function.return_type
                  ? callee->data.function.return_type
                  : callee->type;
    }
    if (code_generator_abi_classify(ret_t) != ABI_PASS_INDIRECT) continue;
    size_t sz = code_generator_abi_type_size(ret_t);
    int slot_bytes = (int)((sz + 15u) & ~(size_t)15);
    int slot_base_offset =
        parameter_home_size + local_home_size + temp_home_size +
        indirect_return_total + slot_bytes;
    if (context->indirect_return_slot_count >=
        context->indirect_return_slot_capacity) {
      size_t new_cap = context->indirect_return_slot_capacity
                           ? context->indirect_return_slot_capacity * 2
                           : 8;
      int *grown = realloc(context->indirect_return_slot_offsets,
                           new_cap * sizeof(int));
      if (!grown) {
        code_generator_set_error(generator,
                                 "Out of memory recording indirect-return "
                                 "slot in function '%s'",
                                 function_data->name);
        binary_function_context_destroy(context);
        return 0;
      }
      context->indirect_return_slot_offsets = grown;
      context->indirect_return_slot_capacity = new_cap;
    }
    context->indirect_return_slot_offsets[context->indirect_return_slot_count++] =
        slot_base_offset;
    indirect_return_total += slot_bytes;
  }

  int saved_register_home_size =
      (int)(context->saved_register_count * BINARY_FUNCTION_STACK_SLOT_SIZE);
  for (size_t i = 0; i < context->saved_register_count; i++) {
    context->saved_register_offsets[i] =
        parameter_home_size + local_home_size + temp_home_size +
        indirect_return_total +
        (int)((i + 1) * BINARY_FUNCTION_STACK_SLOT_SIZE);
  }

  context->raw_frame_size = parameter_home_size + local_home_size +
                            temp_home_size + indirect_return_total +
                            saved_register_home_size;
  if (!binary_align_up_int(context->raw_frame_size, 16, &context->frame_size)) {
    code_generator_set_error(generator,
                             "Stack frame too large in function '%s'",
                             function_data->name);
    binary_function_context_destroy(context);
    return 0;
  }

  return 1;
}

static int code_generator_binary_declare_external_symbol(
    CodeGenerator *generator, const char *symbol_name) {
  BinaryEmitter *emitter = NULL;

  if (!generator || !symbol_name || symbol_name[0] == '\0') {
    return 0;
  }

  emitter = code_generator_get_binary_emitter(generator);
  if (!emitter) {
    code_generator_set_error(generator, "Binary emitter is not initialized");
    return 0;
  }

  if (!binary_emitter_declare_external(emitter, symbol_name)) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to declare external symbol");
    return 0;
  }

  return 1;
}

static int code_generator_binary_emit_symbol_address(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const char *symbol_name, int declare_external,
    BinaryGpRegister target_register) {
  size_t displacement_offset = 0;

  if (!generator || !context || !symbol_name || symbol_name[0] == '\0') {
    return 0;
  }

  if (declare_external &&
      !code_generator_binary_declare_external_symbol(generator, symbol_name)) {
    return 0;
  }

  if (!binary_emit_lea_reg_rip_placeholder(&context->code, target_register,
                                           &displacement_offset) ||
      !binary_call_relocation_table_add(&context->call_relocations, symbol_name,
                                        displacement_offset)) {
    code_generator_set_error(generator,
                             "Out of memory while emitting symbol reference");
    return 0;
  }

  return 1;
}

static int code_generator_binary_emit_cstring_literal_address(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const char *value, BinaryGpRegister target_register) {
  BinaryEmitter *emitter = NULL;
  size_t rdata_section = 0;
  size_t literal_offset = 0;
  size_t length = 0;
  unsigned char terminator = 0;
  char *label = NULL;
  int success = 0;

  if (!generator || !context || !value) {
    return 0;
  }

  emitter = code_generator_get_binary_emitter(generator);
  if (!emitter) {
    code_generator_set_error(generator, "Binary emitter is not initialized");
    return 0;
  }

  label = code_generator_generate_label(generator, "str_chars");
  if (!label) {
    code_generator_set_error(generator,
                             "Out of memory while creating string label");
    return 0;
  }

  rdata_section = binary_emitter_get_or_create_section(
      emitter, ".rdata", BINARY_SECTION_RDATA, 0, 1);
  if (rdata_section == (size_t)-1) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to create .rdata section");
    goto cleanup;
  }

  length = strlen(value);
  if (!binary_emitter_append_bytes(emitter, rdata_section, value, length,
                                   &literal_offset) ||
      !binary_emitter_append_bytes(emitter, rdata_section, &terminator, 1,
                                   NULL) ||
      !binary_emitter_define_symbol(emitter, label, BINARY_SYMBOL_LOCAL,
                                    rdata_section, literal_offset,
                                    length + 1) ||
      !code_generator_binary_emit_symbol_address(generator, context, label, 0,
                                                 target_register)) {
    if (!generator->has_error) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to emit string literal");
    }
    goto cleanup;
  }

  success = 1;

cleanup:
  free(label);
  return success;
}

static int code_generator_binary_emit_string_literal_value_address(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const char *value, BinaryGpRegister target_register) {
  BinaryEmitter *emitter = NULL;
  BinarySection *section = NULL;
  size_t rdata_section = 0;
  size_t chars_offset = 0;
  size_t struct_offset = 0;
  size_t length = 0;
  unsigned char terminator = 0;
  uint64_t string_length = 0;
  char *chars_label = NULL;
  char *struct_label = NULL;
  int success = 0;

  if (!generator || !context || !value) {
    return 0;
  }

  emitter = code_generator_get_binary_emitter(generator);
  if (!emitter) {
    code_generator_set_error(generator, "Binary emitter is not initialized");
    return 0;
  }

  chars_label = code_generator_generate_label(generator, "str_chars");
  struct_label = code_generator_generate_label(generator, "str_struct");
  if (!chars_label || !struct_label) {
    code_generator_set_error(generator,
                             "Out of memory while creating string labels");
    goto cleanup;
  }

  rdata_section = binary_emitter_get_or_create_section(
      emitter, ".rdata", BINARY_SECTION_RDATA, 0, 8);
  if (rdata_section == (size_t)-1) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to create .rdata section");
    goto cleanup;
  }

  length = strlen(value);
  string_length = (uint64_t)length;
  if (!binary_emitter_append_bytes(emitter, rdata_section, value, length,
                                   &chars_offset) ||
      !binary_emitter_append_bytes(emitter, rdata_section, &terminator, 1,
                                   NULL) ||
      !binary_emitter_define_symbol(emitter, chars_label, BINARY_SYMBOL_LOCAL,
                                    rdata_section, chars_offset, length + 1) ||
      !binary_emitter_align_section(emitter, rdata_section, 8, 0) ||
      !binary_emitter_append_zeros(emitter, rdata_section, 16, &struct_offset)) {
    if (!generator->has_error) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to emit string literal");
    }
    goto cleanup;
  }

  section = binary_emitter_get_section(emitter, rdata_section);
  if (!section || !section->data || struct_offset + 16 > section->size) {
    code_generator_set_error(generator,
                             "Failed to access emitted string literal storage");
    goto cleanup;
  }

  memcpy(section->data + struct_offset + 8, &string_length,
         sizeof(string_length));
  if (!binary_emitter_define_symbol(emitter, struct_label, BINARY_SYMBOL_LOCAL,
                                    rdata_section, struct_offset, 16) ||
      !binary_emitter_add_relocation(emitter, rdata_section, struct_offset,
                                     BINARY_RELOCATION_ADDR64, chars_label, 0) ||
      !code_generator_binary_emit_symbol_address(generator, context, struct_label,
                                                 0, target_register)) {
    if (!generator->has_error) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to emit string literal");
    }
    goto cleanup;
  }

  success = 1;

cleanup:
  free(chars_label);
  free(struct_label);
  return success;
}

static int code_generator_binary_emit_global_string_variable(
    CodeGenerator *generator, const char *link_name, const char *value) {
  BinaryEmitter *emitter = NULL;
  BinarySection *section = NULL;
  size_t data_section = 0;
  size_t rdata_section = 0;
  size_t chars_offset = 0;
  size_t struct_offset = 0;
  size_t length = 0;
  uint64_t string_length = 0;
  unsigned char terminator = 0;
  char *chars_label = NULL;

  if (!generator || !link_name || link_name[0] == '\0') {
    return 0;
  }

  emitter = code_generator_get_binary_emitter(generator);
  if (!emitter) {
    code_generator_set_error(generator, "Binary emitter is not initialized");
    return 0;
  }

  data_section = binary_emitter_get_or_create_section(emitter, ".data",
                                                      BINARY_SECTION_DATA, 0, 8);
  if (data_section == (size_t)-1) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to create .data section");
    return 0;
  }

  if (value) {
    chars_label = code_generator_generate_label(generator, "str_chars");
    if (!chars_label) {
      code_generator_set_error(generator,
                               "Out of memory while creating string labels");
      return 0;
    }

    rdata_section = binary_emitter_get_or_create_section(
        emitter, ".rdata", BINARY_SECTION_RDATA, 0, 8);
    if (rdata_section == (size_t)-1) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to create .rdata section");
      free(chars_label);
      return 0;
    }

    length = strlen(value);
    string_length = (uint64_t)length;
    if (!binary_emitter_append_bytes(emitter, rdata_section, value, length,
                                     &chars_offset) ||
        !binary_emitter_append_bytes(emitter, rdata_section, &terminator, 1,
                                     NULL) ||
        !binary_emitter_define_symbol(emitter, chars_label, BINARY_SYMBOL_LOCAL,
                                      rdata_section, chars_offset,
                                      length + 1)) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to emit global string characters");
      free(chars_label);
      return 0;
    }
  }

  if (!binary_emitter_align_section(emitter, data_section, 8, 0) ||
      !binary_emitter_append_zeros(emitter, data_section, 16, &struct_offset)) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to reserve global string storage");
    free(chars_label);
    return 0;
  }

  section = binary_emitter_get_section(emitter, data_section);
  if (!section || !section->data || struct_offset + 16 > section->size) {
    code_generator_set_error(generator,
                             "Failed to access emitted global string storage");
    free(chars_label);
    return 0;
  }

  if (value) {
    memcpy(section->data + struct_offset + 8, &string_length,
           sizeof(string_length));
    if (!binary_emitter_add_relocation(emitter, data_section, struct_offset,
                                       BINARY_RELOCATION_ADDR64, chars_label,
                                       0)) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to emit global string relocation");
      free(chars_label);
      return 0;
    }
  }

  if (!binary_emitter_define_symbol(emitter, link_name, BINARY_SYMBOL_GLOBAL,
                                    data_section, struct_offset, 16)) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to define global string symbol");
    free(chars_label);
    return 0;
  }

  free(chars_label);
  return 1;
}

static int code_generator_binary_get_access_size(CodeGenerator *generator,
                                                 BinaryFunctionContext *context,
                                                 const IROperand *size_operand) {
  if (!generator || !context || !size_operand || size_operand->kind != IR_OPERAND_INT) {
    code_generator_set_error(generator,
                             "IR memory access width must be integer in "
                             "function '%s'",
                             context ? context->function_name : "<unknown>");
    return 0;
  }

  if (size_operand->int_value <= 0) {
    code_generator_set_error(generator,
                             "Invalid IR memory access width %lld in function "
                             "'%s'",
                             size_operand->int_value, context->function_name);
    return 0;
  }

  return (int)size_operand->int_value;
}

static int code_generator_binary_emit_load_from_address(
    CodeGenerator *generator, BinaryFunctionContext *context,
    BinaryGpRegister address_register, int size, BinaryGpRegister target_register) {
  if (!generator || !context) {
    return 0;
  }

  switch (size) {
  case 1:
    return binary_emit_movzx_reg_mem8(&context->code, target_register,
                                      address_register, 0);
  case 2:
    return binary_emit_movzx_reg_mem16(&context->code, target_register,
                                       address_register, 0);
  case 4:
    return binary_emit_mov_reg_mem32(&context->code, target_register,
                                     address_register, 0);
  case 8:
    return binary_emit_mov_reg_mem(&context->code, target_register,
                                   address_register, 0);
  default:
    code_generator_set_error(
        generator,
        "Direct object backend does not yet support memory loads wider than "
        "8 bytes in function '%s'",
        context->function_name);
    return 0;
  }
}

static int code_generator_binary_emit_store_to_address(
    CodeGenerator *generator, BinaryFunctionContext *context,
    BinaryGpRegister address_register, int size, BinaryGpRegister source_register) {
  if (!generator || !context) {
    return 0;
  }

  switch (size) {
  case 1:
    return binary_emit_mov_mem_reg8(&context->code, address_register, 0,
                                    source_register);
  case 2:
    return binary_emit_mov_mem_reg16(&context->code, address_register, 0,
                                     source_register);
  case 4:
    return binary_emit_mov_mem_reg32(&context->code, address_register, 0,
                                     source_register);
  case 8:
    return binary_emit_mov_mem_reg(&context->code, address_register, 0,
                                   source_register);
  default: {
    /* Multi-byte aggregate (e.g. struct memcpy): rep movsb, RSI=src, RDI=dst,
     * RCX=count. Save non-volatile RSI/RDI on Win64. */
    uint64_t n = (uint64_t)size;
    if (n != (uint64_t)size || n == 0) {
      code_generator_set_error(
          generator,
          "Invalid aggregate store size %d in function '%s'",
          size, context->function_name);
      return 0;
    }
    return binary_emit_push_reg(&context->code, BINARY_GP_RSI) &&
           binary_emit_push_reg(&context->code, BINARY_GP_RDI) &&
           binary_emit_mov_reg_reg(&context->code, BINARY_GP_RSI,
                                    source_register) &&
           binary_emit_mov_reg_reg(&context->code, BINARY_GP_RDI,
                                    address_register) &&
           binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RCX, n) &&
           binary_code_buffer_append_u8(&context->code, 0xF3) &&
           binary_code_buffer_append_u8(&context->code, 0xA4) &&
           binary_emit_pop_reg(&context->code, BINARY_GP_RDI) &&
           binary_emit_pop_reg(&context->code, BINARY_GP_RSI);
  }
  }
}

/* Side-table helpers: which IR temps in the current binary function hold
 * pointers to indirect-returned structs. */
static int binary_indirect_temp_add(BinaryFunctionContext *context,
                                    const char *name, size_t size) {
  if (!context || !name) return 0;
  if (context->indirect_temp_count >= context->indirect_temp_capacity) {
    size_t new_cap =
        context->indirect_temp_capacity ? context->indirect_temp_capacity * 2 : 8;
    char **g_names = realloc(context->indirect_temp_names,
                             new_cap * sizeof(char *));
    if (!g_names) return 0;
    context->indirect_temp_names = g_names;
    size_t *g_sizes = realloc(context->indirect_temp_sizes,
                              new_cap * sizeof(size_t));
    if (!g_sizes) return 0;
    context->indirect_temp_sizes = g_sizes;
    context->indirect_temp_capacity = new_cap;
  }
  context->indirect_temp_names[context->indirect_temp_count] = (char *)name;
  context->indirect_temp_sizes[context->indirect_temp_count] = size;
  context->indirect_temp_count++;
  return 1;
}

static size_t binary_indirect_temp_get(BinaryFunctionContext *context,
                                       const char *name) {
  if (!context || !name) return 0;
  for (size_t i = 0; i < context->indirect_temp_count; i++) {
    const char *n = context->indirect_temp_names[i];
    if (n == name || (n && strcmp(n, name) == 0)) {
      return context->indirect_temp_sizes[i];
    }
  }
  return 0;
}

static int code_generator_binary_parameter_is_indirect(
    CodeGenerator *generator, BinaryFunctionContext *context, const char *name) {
  if (!context || !name) {
    return 0;
  }

  Symbol *symbol = generator && generator->symbol_table
                       ? symbol_table_lookup(generator->symbol_table, name)
                       : NULL;
  if (symbol && symbol->kind == SYMBOL_PARAMETER &&
      symbol->data.variable.is_indirect_param) {
    return 1;
  }

  FunctionDeclaration *function_data = context->function_data;
  if (!function_data || !function_data->parameter_names ||
      !function_data->parameter_types) {
    return 0;
  }

  for (size_t i = 0; i < function_data->parameter_count; i++) {
    const char *parameter_name = function_data->parameter_names[i];
    if (parameter_name && strcmp(parameter_name, name) == 0) {
      Type *parameter_type = code_generator_binary_get_resolved_type(
          generator, function_data->parameter_types[i], 0);
      return code_generator_abi_classify(parameter_type) == ABI_PASS_INDIRECT;
    }
  }

  return 0;
}

static int code_generator_binary_emit_struct_destination_address(
    CodeGenerator *generator, BinaryFunctionContext *context, const char *name,
    BinaryGpRegister target_register) {
  if (!generator || !context || !name || name[0] == '\0') {
    return 0;
  }

  int param_offset = code_generator_binary_get_parameter_offset(context, name);
  if (param_offset > 0) {
    if (code_generator_binary_parameter_is_indirect(generator, context, name)) {
      return binary_emit_mov_reg_mem(&context->code, target_register,
                                     BINARY_GP_RBP, -param_offset);
    }
    return binary_emit_lea_reg_mem(&context->code, target_register,
                                   BINARY_GP_RBP, -param_offset);
  }

  int local_offset = code_generator_binary_get_local_offset(context, name);
  if (local_offset > 0) {
    return binary_emit_lea_reg_mem(&context->code, target_register,
                                   BINARY_GP_RBP, -local_offset);
  }

  Symbol *symbol = generator->symbol_table
                       ? symbol_table_lookup(generator->symbol_table, name)
                       : NULL;
  if (symbol && symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
    const char *resolved = code_generator_get_link_symbol_name(generator, name);
    if (!resolved) {
      code_generator_set_error(generator,
                               "Invalid global symbol for struct destination");
      return 0;
    }
    return code_generator_binary_emit_symbol_address(
        generator, context, resolved, symbol->is_extern, target_register);
  }

  code_generator_set_error(
      generator, "Cannot resolve address of struct destination '%s' in function '%s'",
      name, context->function_name);
  return 0;
}

/* Load the address of an INDIRECT struct operand (arg or return) into
 * `target_register`. Mirrors `code_generator_emit_ir_indirect_arg_source_address`
 * from the text-asm path. */
static int code_generator_binary_emit_indirect_source_address(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *operand, BinaryGpRegister target_register) {
  if (!generator || !context || !operand) {
    return 0;
  }
  if (operand->kind == IR_OPERAND_SYMBOL) {
    if (!operand->name) {
      code_generator_set_error(generator,
                               "Malformed IR symbol operand (indirect arg)");
      return 0;
    }
    int param_offset =
        code_generator_binary_get_parameter_offset(context, operand->name);
    if (param_offset > 0) {
      if (code_generator_binary_parameter_is_indirect(generator, context,
                                                     operand->name)) {
        return binary_emit_mov_reg_mem(&context->code, target_register,
                                       BINARY_GP_RBP, -param_offset);
      }
      return binary_emit_lea_reg_mem(&context->code, target_register,
                                     BINARY_GP_RBP, -param_offset);
    }
    int local_offset = code_generator_binary_get_local_offset(context,
                                                              operand->name);
    if (local_offset > 0) {
      return binary_emit_lea_reg_mem(&context->code, target_register,
                                     BINARY_GP_RBP, -local_offset);
    }
    Symbol *symbol = symbol_table_lookup(generator->symbol_table, operand->name);
    if (!symbol) {
      code_generator_set_error(generator,
                               "Unknown symbol '%s' for indirect call arg",
                               operand->name);
      return 0;
    }
    if (symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
      const char *resolved =
          code_generator_get_link_symbol_name(generator, operand->name);
      if (!resolved) {
        code_generator_set_error(generator,
                                 "Invalid global symbol for indirect arg");
        return 0;
      }
      return code_generator_binary_emit_symbol_address(
          generator, context, resolved, symbol->is_extern, target_register);
    }
    code_generator_set_error(
        generator,
        "Cannot resolve address of struct symbol '%s' in function '%s'",
        operand->name, context->function_name);
    return 0;
  }
  if (operand->kind == IR_OPERAND_TEMP) {
    if (!operand->name) {
      code_generator_set_error(generator,
                               "Malformed IR temp operand (indirect arg)");
      return 0;
    }
    /* If the temp is tagged as an indirect-return pointer, the temp's slot
     * holds the value (a pointer); load it. Otherwise take its address. */
    int offset =
        code_generator_binary_get_temp_offset(context, operand->name);
    if (offset <= 0) {
      code_generator_set_error(generator,
                               "Unknown IR temp '%s' for indirect arg",
                               operand->name);
      return 0;
    }
    if (binary_indirect_temp_get(context, operand->name) > 0) {
      return binary_emit_mov_reg_mem(&context->code, target_register,
                                     BINARY_GP_RBP, -offset);
    }
    return binary_emit_lea_reg_mem(&context->code, target_register,
                                   BINARY_GP_RBP, -offset);
  }
  code_generator_set_error(
      generator, "Indirect call argument must be a struct value (kind=%d)",
      operand->kind);
  return 0;
}

/* Emit `rep movsb` of `size` bytes from [src_addr_reg] to [dst_addr_reg].
 * Does NOT preserve rsi/rdi/rcx — callers that need them must save manually.
 * Used in call-arg memcpy and indirect-return paths where the surrounding
 * code knows rsi/rdi are dead. */
static int code_generator_binary_emit_rep_movsb(
    CodeGenerator *generator, BinaryFunctionContext *context,
    BinaryGpRegister src_addr_reg, BinaryGpRegister dst_addr_reg, size_t size) {
  if (!generator || !context || size == 0) {
    return 0;
  }
  if (!binary_emit_mov_reg_reg(&context->code, BINARY_GP_RSI, src_addr_reg) ||
      !binary_emit_mov_reg_reg(&context->code, BINARY_GP_RDI, dst_addr_reg) ||
      !binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RCX,
                                 (uint64_t)size) ||
      /* cld (DF=0) — ensure forward direction. One byte 0xFC. */
      !binary_code_buffer_append_u8(&context->code, 0xFC) ||
      /* rep movsb: 0xF3 0xA4. */
      !binary_code_buffer_append_u8(&context->code, 0xF3) ||
      !binary_code_buffer_append_u8(&context->code, 0xA4)) {
    return 0;
  }
  return 1;
}

/* rep movsq: RCX = qword count, RSI/RDI = src/dst. Requires 8-byte alignment
 * for correctness on strict platforms; benchmark buffers are int32-aligned. */
static int code_generator_binary_emit_rep_movsq(
    CodeGenerator *generator, BinaryFunctionContext *context,
    BinaryGpRegister src_addr_reg, BinaryGpRegister dst_addr_reg,
    size_t qword_count) {
  if (!generator || !context || qword_count == 0) {
    return 0;
  }
  if (!binary_emit_push_reg(&context->code, BINARY_GP_RSI) ||
      !binary_emit_push_reg(&context->code, BINARY_GP_RDI) ||
      !binary_emit_mov_reg_reg(&context->code, BINARY_GP_RSI, src_addr_reg) ||
      !binary_emit_mov_reg_reg(&context->code, BINARY_GP_RDI, dst_addr_reg) ||
      !binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RCX,
                                 (uint64_t)qword_count) ||
      !binary_code_buffer_append_u8(&context->code, 0xFC) ||
      !binary_code_buffer_append_u8(&context->code, 0xF3) ||
      !binary_emit_rex(&context->code, 1, 0, 0, 0) ||
      !binary_code_buffer_append_u8(&context->code, 0xA5) ||
      !binary_emit_pop_reg(&context->code, BINARY_GP_RDI) ||
      !binary_emit_pop_reg(&context->code, BINARY_GP_RSI)) {
    return 0;
  }
  return 1;
}

static int code_generator_binary_emit_global_symbol_load(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const char *symbol_name, Type *type, int declare_external,
    BinaryGpRegister target_register) {
  size_t displacement_offset = 0;
  int size = code_generator_binary_resolved_type_scalar_size(type);
  int is_signed = code_generator_binary_resolved_type_is_signed_integer(type);

  if (!generator || !context || !symbol_name || symbol_name[0] == '\0') {
    return 0;
  }

  if (declare_external &&
      !code_generator_binary_declare_external_symbol(generator, symbol_name)) {
    return 0;
  }

  switch (size) {
  case 1:
    if ((!binary_emit_movzx_reg_rip_mem8(&context->code, target_register,
                                         &displacement_offset)) ||
        (is_signed &&
         !binary_emit_movsx_reg_reg8(&context->code, target_register,
                                     target_register))) {
      return 0;
    }
    break;
  case 2:
    if ((!binary_emit_movzx_reg_rip_mem16(&context->code, target_register,
                                          &displacement_offset)) ||
        (is_signed &&
         !binary_emit_movsx_reg_reg16(&context->code, target_register,
                                      target_register))) {
      return 0;
    }
    break;
  case 4:
    if (!binary_emit_mov_reg32_rip_mem(&context->code, target_register,
                                       &displacement_offset) ||
        (is_signed &&
         !binary_emit_movsxd_reg_reg32(&context->code, target_register,
                                       target_register))) {
      return 0;
    }
    break;
  case 8:
    if (!binary_emit_mov_reg_rip_mem(&context->code, target_register,
                                     &displacement_offset)) {
      return 0;
    }
    break;
  default:
    code_generator_set_error(
        generator,
        "Direct object backend does not yet support global scalar loads wider "
        "than 8 bytes in function '%s'",
        context->function_name);
    return 0;
  }

  if (!binary_call_relocation_table_add(&context->call_relocations, symbol_name,
                                        displacement_offset)) {
    code_generator_set_error(generator,
                             "Out of memory while emitting global load");
    return 0;
  }

  return 1;
}

static int code_generator_binary_emit_global_symbol_store(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const char *symbol_name, Type *type, int declare_external,
    BinaryGpRegister source_register) {
  size_t displacement_offset = 0;
  int size = code_generator_binary_resolved_type_scalar_size(type);

  if (!generator || !context || !symbol_name || symbol_name[0] == '\0') {
    return 0;
  }

  if (declare_external &&
      !code_generator_binary_declare_external_symbol(generator, symbol_name)) {
    return 0;
  }

  switch (size) {
  case 1:
    if (!binary_emit_mov_mem_rip_reg8(&context->code, source_register,
                                      &displacement_offset)) {
      return 0;
    }
    break;
  case 2:
    if (!binary_emit_mov_mem_rip_reg16(&context->code, source_register,
                                       &displacement_offset)) {
      return 0;
    }
    break;
  case 4:
    if (!binary_emit_mov_mem_rip_reg32(&context->code, source_register,
                                       &displacement_offset)) {
      return 0;
    }
    break;
  case 8:
    if (!binary_emit_mov_mem_rip_reg(&context->code, source_register,
                                     &displacement_offset)) {
      return 0;
    }
    break;
  default:
    code_generator_set_error(
        generator,
        "Direct object backend does not yet support global scalar stores wider "
        "than 8 bytes in function '%s'",
        context->function_name);
    return 0;
  }

  if (!binary_call_relocation_table_add(&context->call_relocations, symbol_name,
                                        displacement_offset)) {
    code_generator_set_error(generator,
                             "Out of memory while emitting global store");
    return 0;
  }

  return 1;
}

static int code_generator_binary_operand_is_known_float64(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *operand) {
  Symbol *symbol = NULL;

  if (!context || !operand) {
    return 0;
  }

  if (operand->kind == IR_OPERAND_FLOAT) {
    return 1;
  }

  if ((operand->kind == IR_OPERAND_SYMBOL || operand->kind == IR_OPERAND_TEMP) &&
      operand->name &&
      code_generator_binary_is_marked_float64_symbol(context, operand->name)) {
    return 1;
  }

  if (operand->kind == IR_OPERAND_SYMBOL && operand->name && generator &&
      generator->symbol_table) {
    symbol = symbol_table_lookup(generator->symbol_table, operand->name);
    return symbol && code_generator_binary_resolved_type_is_float64(symbol->type);
  }

  return 0;
}

/* IEEE-754 width of a value operand: 32, 64, or 0 (not floating). Resolution
 * order: the operand's own IR-carried float_bits (authoritative, set by
 * ir_lowering), then a width recorded for the named symbol/temp, then the
 * declared symbol type. This is the single place backends ask "what float
 * precision is this value" so single vs double is never re-guessed ad hoc. */
static int code_generator_binary_operand_float_bits(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *operand) {
  Symbol *symbol = NULL;

  if (!context || !operand) {
    return 0;
  }

  if (operand->kind == IR_OPERAND_FLOAT) {
    return operand->float_bits == 32 ? 32 : 64;
  }

  if ((operand->kind == IR_OPERAND_SYMBOL ||
       operand->kind == IR_OPERAND_TEMP)) {
    if (operand->float_bits == 32 || operand->float_bits == 64) {
      return operand->float_bits;
    }
    if (operand->name) {
      int marked = code_generator_binary_marked_symbol_float_bits(
          context, operand->name);
      if (marked) {
        return marked;
      }
    }
  }

  if (operand->kind == IR_OPERAND_SYMBOL && operand->name && generator &&
      generator->symbol_table) {
    symbol = symbol_table_lookup(generator->symbol_table, operand->name);
    if (symbol) {
      return code_generator_binary_resolved_type_float_bits(symbol->type);
    }
  }

  return 0;
}

static int code_generator_binary_instruction_result_is_float64(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  Symbol *symbol = NULL;
  Type *function_type = NULL;
  const char *op = NULL;

  if (!context || !instruction) {
    return 0;
  }

  switch (instruction->op) {
  case IR_OP_ASSIGN:
    return code_generator_binary_operand_is_known_float64(generator, context,
                                                          &instruction->lhs);

  case IR_OP_BINARY:
    op = instruction->text;
    return instruction->is_float && op &&
           (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
            strcmp(op, "*") == 0 || strcmp(op, "/") == 0);

  case IR_OP_UNARY:
    op = instruction->text;
    return instruction->is_float && op &&
           (strcmp(op, "+") == 0 || strcmp(op, "-") == 0);

  case IR_OP_CALL:
    symbol = generator && generator->symbol_table && instruction->text
                 ? symbol_table_lookup(generator->symbol_table,
                                       instruction->text)
                 : NULL;
    return symbol && symbol->kind == SYMBOL_FUNCTION &&
           code_generator_binary_resolved_type_is_float64(
               symbol->data.function.return_type);

  case IR_OP_CALL_INDIRECT:
    symbol = generator && generator->symbol_table &&
                     instruction->lhs.kind == IR_OPERAND_SYMBOL &&
                     instruction->lhs.name
                 ? symbol_table_lookup(generator->symbol_table,
                                       instruction->lhs.name)
                 : NULL;
    function_type =
        (symbol && symbol->type && symbol->type->kind == TYPE_FUNCTION_POINTER)
            ? symbol->type
            : NULL;
    return code_generator_binary_resolved_type_is_float64(
        function_type ? function_type->fn_return_type : NULL);

  case IR_OP_CAST:
    return code_generator_binary_named_type_is_float64(generator,
                                                       instruction->text, 0);

  case IR_OP_LOAD:
    /* A value dereferenced from a float* / struct member is floating in the
     * machine sense even though no symbol carries that type. ir_lowering sets
     * is_float on float32/float64 loads; honor it so the destination temp is
     * marked and reaches xmm via movd/movq (bit copy) rather than cvtsi2s*
     * (integer->float conversion of the raw bit pattern). */
    return instruction->is_float;

  default:
    return 0;
  }
}

/* Float width (0/32/64) of an instruction's destination value. Generalizes
 * code_generator_binary_instruction_result_is_float64 so the symbol-marking
 * pass can record single vs double precision per temp/symbol. */
static int code_generator_binary_instruction_result_float_bits(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  Symbol *symbol = NULL;
  Type *function_type = NULL;

  if (!context || !instruction) {
    return 0;
  }

  if (!code_generator_binary_instruction_result_is_float64(generator, context,
                                                           instruction)) {
    return 0;
  }

  switch (instruction->op) {
  case IR_OP_ASSIGN:
    return code_generator_binary_operand_float_bits(generator, context,
                                                    &instruction->lhs);

  case IR_OP_BINARY:
  case IR_OP_UNARY:
  case IR_OP_LOAD:
    return (instruction->float_bits == 32) ? 32 : 64;

  case IR_OP_CALL:
    symbol = generator && generator->symbol_table && instruction->text
                 ? symbol_table_lookup(generator->symbol_table,
                                       instruction->text)
                 : NULL;
    return (symbol && symbol->kind == SYMBOL_FUNCTION)
               ? code_generator_binary_resolved_type_float_bits(
                     symbol->data.function.return_type)
               : 64;

  case IR_OP_CALL_INDIRECT:
    symbol = generator && generator->symbol_table &&
                     instruction->lhs.kind == IR_OPERAND_SYMBOL &&
                     instruction->lhs.name
                 ? symbol_table_lookup(generator->symbol_table,
                                       instruction->lhs.name)
                 : NULL;
    function_type =
        (symbol && symbol->type && symbol->type->kind == TYPE_FUNCTION_POINTER)
            ? symbol->type
            : NULL;
    return function_type ? code_generator_binary_resolved_type_float_bits(
                               function_type->fn_return_type)
                         : 64;

  case IR_OP_CAST: {
    Type *t = generator && generator->type_checker
                  ? type_checker_get_type_by_name(generator->type_checker,
                                                  instruction->text)
                  : NULL;
    return code_generator_binary_resolved_type_float_bits(t);
  }

  default:
    return 64;
  }
}

static int code_generator_binary_emit_operand_load(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *operand, BinaryGpRegister target_register);

static int code_generator_binary_emit_string_symbol_load(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const char *symbol_name, const Symbol *symbol,
    BinaryGpRegister target_register) {
  int offset = 0;

  if (!generator || !context || !symbol_name || symbol_name[0] == '\0') {
    return 0;
  }

  offset = code_generator_binary_get_symbol_offset(context, symbol_name);
  if (offset > 0) {
    if (symbol && symbol->kind == SYMBOL_PARAMETER) {
      return binary_emit_mov_reg_mem(&context->code, target_register,
                                     BINARY_GP_RBP, -offset);
    }
    return binary_emit_lea_reg_mem(&context->code, target_register,
                                   BINARY_GP_RBP, -offset);
  }

  if (symbol && symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
    const char *link_name =
        code_generator_get_link_symbol_name(generator, symbol_name);
    if (!link_name || link_name[0] == '\0') {
      code_generator_set_error(generator,
                               "Invalid global string symbol '%s' in function "
                               "'%s'",
                               symbol_name, context->function_name);
      return 0;
    }
    return code_generator_binary_emit_symbol_address(
        generator, context, link_name, symbol->is_extern, target_register);
  }

  code_generator_set_error(generator,
                           "Unknown string symbol '%s' in function '%s'",
                           symbol_name, context->function_name);
  return 0;
}

/* Materialize an operand into an XMM register at the requested precision
 * (want_bits = 32 or 64).
 *   - A floating operand carries raw IEEE-754 bits in RAX: copy them with
 *     movd (32) or movq (64) according to the operand's OWN width, then
 *     widen/narrow to want_bits with cvtss2sd / cvtsd2ss if they differ.
 *   - An integer operand is converted to float with cvtsi2ss / cvtsi2sd at
 *     want_bits (matches the surrounding float expression's precision). */
static int code_generator_binary_emit_float_operand_to_xmm_bits(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *operand, BinaryXmmRegister target_register,
    int want_bits) {
  int operand_bits = 0;

  if (!generator || !context || !operand) {
    return 0;
  }
  if (want_bits != 32 && want_bits != 64) {
    want_bits = 64;
  }

  if (!code_generator_binary_emit_operand_load(generator, context, operand,
                                               BINARY_GP_RAX)) {
    return 0;
  }

  operand_bits =
      code_generator_binary_operand_float_bits(generator, context, operand);

  if (operand_bits == 32) {
    if (!binary_emit_movd_xmm_reg(&context->code, target_register,
                                  BINARY_GP_RAX)) {
      return 0;
    }
    if (want_bits == 64) {
      return binary_emit_cvtss2sd_xmm_xmm(&context->code, target_register,
                                          target_register);
    }
    return 1;
  }

  if (operand_bits == 64) {
    if (!binary_emit_movq_xmm_reg(&context->code, target_register,
                                  BINARY_GP_RAX)) {
      return 0;
    }
    if (want_bits == 32) {
      return binary_emit_cvtsd2ss_xmm_xmm(&context->code, target_register,
                                          target_register);
    }
    return 1;
  }

  /* Integer value used in a float context: convert at the target precision. */
  if (want_bits == 32) {
    return binary_emit_cvtsi2ss_xmm_reg(&context->code, target_register,
                                        BINARY_GP_RAX);
  }
  return binary_emit_cvtsi2sd_xmm_reg(&context->code, target_register,
                                      BINARY_GP_RAX);
}

static int code_generator_binary_emit_float_operand_to_xmm(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *operand, BinaryXmmRegister target_register) {
  return code_generator_binary_emit_float_operand_to_xmm_bits(
      generator, context, operand, target_register, 64);
}

/* Reinterpret the float bits held in `gp_register` from src_bits precision to
 * dst_bits precision, in place, using XMM0 as scratch. No-op when the widths
 * already match or either side is not a float (src/dst 0). Used by ASSIGN /
 * STORE / RETURN when a float64 value lands in a float32 slot or vice versa. */
static int code_generator_binary_emit_float_reg_convert(
    BinaryFunctionContext *context, BinaryGpRegister gp_register,
    int src_bits, int dst_bits) {
  if (!context || src_bits == 0 || dst_bits == 0 || src_bits == dst_bits) {
    return 1;
  }

  if (src_bits == 64 && dst_bits == 32) {
    return binary_emit_movq_xmm_reg(&context->code, BINARY_XMM0,
                                    gp_register) &&
           binary_emit_cvtsd2ss_xmm_xmm(&context->code, BINARY_XMM0,
                                        BINARY_XMM0) &&
           binary_emit_movd_reg_xmm(&context->code, gp_register, BINARY_XMM0);
  }
  if (src_bits == 32 && dst_bits == 64) {
    return binary_emit_movd_xmm_reg(&context->code, BINARY_XMM0,
                                    gp_register) &&
           binary_emit_cvtss2sd_xmm_xmm(&context->code, BINARY_XMM0,
                                        BINARY_XMM0) &&
           binary_emit_movq_reg_xmm(&context->code, gp_register, BINARY_XMM0);
  }
  return 1;
}

static int code_generator_binary_emit_operand_load(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *operand, BinaryGpRegister target_register) {
  if (!generator || !context || !operand) {
    return 0;
  }

  switch (operand->kind) {
  case IR_OPERAND_NONE:
    return binary_emit_mov_reg_imm64(&context->code, target_register, 0);

  case IR_OPERAND_INT:
    return binary_emit_mov_reg_imm64(&context->code, target_register,
                                     (uint64_t)operand->int_value);

  case IR_OPERAND_FLOAT: {
    if (operand->float_bits == 32) {
      /* Materialize the true 32-bit IEEE-754 single pattern (zero-extended).
       * Encoding it as the low half of a double would store 0 for most
       * values. */
      union {
        float value;
        uint32_t bits;
      } encoded = {0};
      encoded.value = (float)operand->float_value;
      return binary_emit_mov_reg_imm64(&context->code, target_register,
                                       (uint64_t)encoded.bits);
    }
    union {
      double value;
      uint64_t bits;
    } encoded = {0};
    encoded.value = operand->float_value;
    return binary_emit_mov_reg_imm64(&context->code, target_register,
                                     encoded.bits);
  }

  case IR_OPERAND_STRING:
    return code_generator_binary_emit_string_literal_value_address(
        generator, context, operand->name ? operand->name : "",
        target_register);

  case IR_OPERAND_TEMP: {
    int offset = code_generator_binary_get_temp_offset(context, operand->name);
    if (offset <= 0) {
      code_generator_set_error(generator, "Unknown IR temp '%s' in function '%s'",
                               operand->name ? operand->name : "<unnamed>",
                               context->function_name);
      return 0;
    }
    return binary_emit_mov_reg_mem(&context->code, target_register,
                                   BINARY_GP_RBP, -offset);
  }

  case IR_OPERAND_SYMBOL: {
    const char *alias_target =
        binary_symbol_alias_table_get(&context->symbol_aliases, operand->name);
    Symbol *symbol = generator && generator->symbol_table
                         ? symbol_table_lookup(generator->symbol_table,
                                               operand->name)
                         : NULL;
    int offset = code_generator_binary_get_symbol_offset(context, operand->name);
    BinaryGpRegister assigned_register = BINARY_GP_RAX;
    if (alias_target) {
      IROperand aliased = *operand;
      aliased.name = (char *)alias_target;
      return code_generator_binary_emit_operand_load(generator, context,
                                                     &aliased,
                                                     target_register);
    }
    if (symbol && symbol->type && symbol->type->kind == TYPE_STRING) {
      return code_generator_binary_emit_string_symbol_load(
          generator, context, operand->name, symbol, target_register);
    }
    if (code_generator_binary_symbol_assigned_register(
            generator, context, operand->name, &assigned_register)) {
      if (target_register == assigned_register) {
        return 1;
      }
      return binary_emit_mov_reg_reg(&context->code, target_register,
                                     assigned_register);
    }
    if (offset > 0 && symbol &&
        code_generator_binary_type_is_direct_aggregate(symbol->type)) {
      int size = (int)symbol->type->size;
      if (!binary_emit_lea_reg_mem(&context->code, target_register,
                                   BINARY_GP_RBP, -offset) ||
          !code_generator_binary_emit_load_from_address(
              generator, context, target_register, size, target_register)) {
        if (!generator->has_error) {
          code_generator_set_error(
              generator,
              "Out of memory while loading direct aggregate symbol '%s' in "
              "function '%s'",
              operand->name ? operand->name : "<unnamed>",
              context->function_name);
        }
        return 0;
      }
      return 1;
    }
    if (offset <= 0) {
      if (symbol && symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
        const char *link_name =
            code_generator_get_link_symbol_name(generator, operand->name);
        uint64_t const_value = 0;
        if (!link_name || link_name[0] == '\0') {
          code_generator_set_error(generator,
                                   "Invalid global symbol '%s' in function '%s'",
                                   operand->name ? operand->name : "<unnamed>",
                                   context->function_name);
          return 0;
        }
        if (!code_generator_binary_symbol_is_scalar_accessible(generator,
                                                               operand->name)) {
          code_generator_set_error(
              generator,
              "Direct object backend cannot load aggregate global symbol '%s' "
              "directly in function '%s'",
              operand->name ? operand->name : "<unnamed>",
              context->function_name);
          return 0;
        }
        if (binary_global_const_table_get(operand->name, &const_value)) {
          return binary_emit_mov_reg_imm64(&context->code, target_register,
                                           const_value);
        }
        if (!code_generator_binary_emit_global_symbol_load(
                generator, context, link_name, symbol->type, symbol->is_extern,
                target_register)) {
          if (!generator->has_error) {
            code_generator_set_error(
                generator,
                "Out of memory while loading global symbol '%s' in function "
                "'%s'",
                operand->name ? operand->name : "<unnamed>",
                context->function_name);
          }
          return 0;
        }
        return 1;
      }

      code_generator_set_error(
          generator,
          "Direct object backend only supports parameter/local/global symbols "
          "(encountered '%s' in function '%s')",
          operand->name ? operand->name : "<unnamed>", context->function_name);
      return 0;
    }
    if (!code_generator_binary_symbol_is_scalar_accessible(generator,
                                                           operand->name)) {
      code_generator_set_error(
          generator,
          "Direct object backend cannot load aggregate symbol '%s' directly "
          "in function '%s'",
          operand->name ? operand->name : "<unnamed>", context->function_name);
      return 0;
    }
    return binary_emit_mov_reg_mem(&context->code, target_register,
                                   BINARY_GP_RBP, -offset);
  }

  default:
    code_generator_set_error(
        generator,
        "Direct object backend does not support operand kind %d in function "
        "'%s'",
        (int)operand->kind, context->function_name);
    return 0;
  }
}

static int code_generator_binary_emit_memcpy_inline(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  long long byte_count = 0;
  BinaryGpRegister dst_reg = BINARY_GP_RDI;
  BinaryGpRegister src_reg = BINARY_GP_RSI;

  if (!generator || !context || !instruction) {
    return 0;
  }

  if (instruction->rhs.kind == IR_OPERAND_INT) {
    byte_count = instruction->rhs.int_value;
  } else {
    code_generator_set_error(generator,
                             "memcpy_inline requires constant size in '%s'",
                             context->function_name);
    return 0;
  }

  if (byte_count <= 0) {
    return 1;
  }

  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest, dst_reg) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs, src_reg)) {
    return 0;
  }

  if (byte_count >= 64 && (byte_count % 8) == 0) {
    return code_generator_binary_emit_rep_movsq(generator, context, src_reg,
                                                dst_reg,
                                                (size_t)(byte_count / 8));
  }

  return code_generator_binary_emit_rep_movsb(generator, context, src_reg,
                                              dst_reg, (size_t)byte_count);
}

static int code_generator_binary_emit_call_argument_load(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *operand, Type *parameter_type,
    BinaryGpRegister target_register) {
  Type *operand_type = NULL;

  if (!generator || !context || !operand) {
    return 0;
  }

  if (code_generator_binary_type_is_cstring(parameter_type) &&
      operand->kind == IR_OPERAND_STRING) {
    return code_generator_binary_emit_cstring_literal_address(
        generator, context, operand->name ? operand->name : "",
        target_register);
  }

  if (!code_generator_binary_emit_operand_load(generator, context, operand,
                                               target_register)) {
    return 0;
  }

  operand_type = code_generator_binary_get_operand_type(generator, operand);
  if (code_generator_binary_type_is_cstring(parameter_type) &&
      code_generator_binary_type_is_string(operand_type)) {
    return binary_emit_mov_reg_mem(&context->code, target_register,
                                   target_register, 0);
  }

  return 1;
}

/* Load a float register-argument and place it in its Win64 XMM parameter
 * register at the parameter's precision. param_fbits is 32 or 64. The raw
 * IEEE bits arrive in RAX; movd transfers a single, movq a double. */
static int code_generator_binary_emit_float_call_argument(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *operand, Type *parameter_type, int param_fbits,
    BinaryXmmRegister xmm_register) {
  if (!code_generator_binary_emit_call_argument_load(
          generator, context, operand, parameter_type, BINARY_GP_RAX)) {
    return 0;
  }
  if (param_fbits == 32) {
    return binary_emit_movd_xmm_reg(&context->code, xmm_register,
                                    BINARY_GP_RAX);
  }
  return binary_emit_movq_xmm_reg(&context->code, xmm_register,
                                  BINARY_GP_RAX);
}

static int code_generator_binary_emit_local_string_store(
    CodeGenerator *generator, BinaryFunctionContext *context, int offset,
    BinaryGpRegister source_register) {
  BinaryGpRegister scratch =
      source_register == BINARY_GP_R10 ? BINARY_GP_RAX : BINARY_GP_R10;
  int chars_displacement = -offset;
  int length_displacement = 8 - offset;

  if (!generator || !context || offset <= 8) {
    return 0;
  }

  if (!binary_emit_mov_reg_mem(&context->code, scratch, source_register, 0) ||
      !binary_emit_mov_mem_reg(&context->code, BINARY_GP_RBP,
                               chars_displacement, scratch) ||
      !binary_emit_mov_reg_mem(&context->code, scratch, source_register, 8) ||
      !binary_emit_mov_mem_reg(&context->code, BINARY_GP_RBP,
                               length_displacement, scratch)) {
    code_generator_set_error(generator,
                             "Out of memory while storing string value in "
                             "function '%s'",
                             context->function_name);
    return 0;
  }

  return 1;
}

static int code_generator_binary_emit_destination_store(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *destination, BinaryGpRegister source_register) {
  if (!generator || !context || !destination) {
    return 0;
  }

  switch (destination->kind) {
  case IR_OPERAND_NONE:
    return 1;

  case IR_OPERAND_TEMP: {
    int offset =
        code_generator_binary_get_temp_offset(context, destination->name);
    if (offset <= 0) {
      code_generator_set_error(generator, "Unknown IR temp '%s' in function '%s'",
                               destination->name ? destination->name
                                                 : "<unnamed>",
                               context->function_name);
      return 0;
    }
    return binary_emit_mov_mem_reg(&context->code, BINARY_GP_RBP, -offset,
                                   source_register);
  }

  case IR_OPERAND_SYMBOL: {
    Symbol *symbol = generator && generator->symbol_table
                         ? symbol_table_lookup(generator->symbol_table,
                                               destination->name)
                         : NULL;
    int offset =
        code_generator_binary_get_symbol_offset(context, destination->name);
    BinaryGpRegister assigned_register = BINARY_GP_RAX;
    if (symbol && symbol->type && symbol->type->kind == TYPE_STRING) {
      if (offset <= 0) {
        if (symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
          code_generator_set_error(
              generator,
              "Direct object backend does not yet support string global stores "
              "in function '%s'",
              context->function_name);
        } else {
          code_generator_set_error(generator,
                                   "Unknown string symbol '%s' in function '%s'",
                                   destination->name ? destination->name
                                                     : "<unnamed>",
                                   context->function_name);
        }
        return 0;
      }

      if (symbol->kind == SYMBOL_PARAMETER) {
        return binary_emit_mov_mem_reg(&context->code, BINARY_GP_RBP, -offset,
                                       source_register);
      }

      return code_generator_binary_emit_local_string_store(
          generator, context, offset, source_register);
    }
    if (code_generator_binary_symbol_assigned_register(
            generator, context, destination->name, &assigned_register)) {
      if (assigned_register == source_register) {
        return 1;
      }
      return binary_emit_mov_reg_reg(&context->code, assigned_register,
                                     source_register);
    }
    if (offset > 0 && symbol &&
        code_generator_binary_type_is_direct_aggregate(symbol->type)) {
      int size = (int)symbol->type->size;
      BinaryGpRegister address_register =
          source_register == BINARY_GP_R10 ? BINARY_GP_RAX : BINARY_GP_R10;
      if (!binary_emit_lea_reg_mem(&context->code, address_register,
                                   BINARY_GP_RBP, -offset) ||
          !code_generator_binary_emit_store_to_address(
              generator, context, address_register, size, source_register)) {
        if (!generator->has_error) {
          code_generator_set_error(
              generator,
              "Out of memory while storing direct aggregate symbol '%s' in "
              "function '%s'",
              destination->name ? destination->name : "<unnamed>",
              context->function_name);
        }
        return 0;
      }
      return 1;
    }
    if (offset <= 0) {
      if (symbol && symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
        const char *link_name =
            code_generator_get_link_symbol_name(generator, destination->name);
        if (!link_name || link_name[0] == '\0') {
          code_generator_set_error(generator,
                                   "Invalid global symbol '%s' in function '%s'",
                                   destination->name
                                       ? destination->name
                                       : "<unnamed>",
                                   context->function_name);
          return 0;
        }
        if (!code_generator_binary_symbol_is_scalar_accessible(generator,
                                                               destination->name)) {
          code_generator_set_error(
              generator,
              "Direct object backend cannot store aggregate global symbol '%s' "
              "directly in function '%s'",
              destination->name ? destination->name : "<unnamed>",
              context->function_name);
          return 0;
        }
        if (!code_generator_binary_emit_global_symbol_store(
                generator, context, link_name, symbol->type, symbol->is_extern,
                source_register)) {
          if (!generator->has_error) {
            code_generator_set_error(
                generator,
                "Out of memory while storing global symbol '%s' in function "
                "'%s'",
                destination->name ? destination->name : "<unnamed>",
                context->function_name);
          }
          return 0;
        }
        return 1;
      }

      code_generator_set_error(
          generator,
          "Direct object backend only supports stores to "
          "parameter/local/global symbols (encountered '%s' in function '%s')",
          destination->name ? destination->name : "<unnamed>",
          context->function_name);
      return 0;
    }
    if (!code_generator_binary_symbol_is_scalar_accessible(generator,
                                                           destination->name)) {
      code_generator_set_error(
          generator,
          "Direct object backend cannot store aggregate symbol '%s' directly "
          "in function '%s'",
          destination->name ? destination->name : "<unnamed>",
          context->function_name);
      return 0;
    }
    return binary_emit_mov_mem_reg(&context->code, BINARY_GP_RBP, -offset,
                                   source_register);
  }

  default:
    code_generator_set_error(
        generator,
        "Direct object backend does not support destination kind %d in "
        "function '%s'",
        (int)destination->kind, context->function_name);
    return 0;
  }
}

static int code_generator_binary_validate_call(CodeGenerator *generator,
                                               BinaryFunctionContext *context,
                                               const IRInstruction *instruction) {
  if (!generator || !context || !instruction || !instruction->text ||
      instruction->text[0] == '\0') {
    return 0;
  }

  Symbol *symbol = generator->symbol_table
                       ? symbol_table_lookup(generator->symbol_table,
                                             instruction->text)
                       : NULL;
  if (!symbol || symbol->kind != SYMBOL_FUNCTION) {
    return 1;
  }

  if (!code_generator_binary_type_is_abi_supported(
          generator, symbol->data.function.return_type
                         ? symbol->data.function.return_type->name
                         : "int64",
          1)) {
    code_generator_set_error(
        generator,
        "Direct object backend only supports integer/pointer/string/float64 call "
        "returns (callee '%s' in function '%s')",
        instruction->text, context->function_name);
    return 0;
  }

  if (instruction->argument_count != symbol->data.function.parameter_count) {
    code_generator_set_error(
        generator,
        "Call argument mismatch while lowering direct object function '%s'",
        context->function_name);
    return 0;
  }

  for (size_t i = 0; i < symbol->data.function.parameter_count; i++) {
    Type *parameter_type = symbol->data.function.parameter_types
                               ? symbol->data.function.parameter_types[i]
                               : NULL;
    if (parameter_type &&
        !code_generator_binary_resolved_type_is_abi_supported(parameter_type, 0)) {
      code_generator_set_error(
          generator,
          "Direct object backend only supports integer/pointer/string/float64 call "
          "arguments (callee '%s' in function '%s')",
          instruction->text, context->function_name);
      return 0;
    }
  }

  return 1;
}

static int code_generator_binary_emit_runtime_trap_call(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  char *trap_pc_label = NULL;
  size_t displacement_offset = 0;
  const char *trap_symbol = "mettle_crash_trap";

  if (!generator || !context || !instruction || instruction->argument_count == 0) {
    return 0;
  }

  if (!generator->generate_stack_trace_support) {
    const char *puts_symbol = "puts";
    const char *exit_symbol = "exit";
    if (!code_generator_binary_declare_external_symbol(generator, puts_symbol) ||
        !code_generator_binary_declare_external_symbol(generator, exit_symbol)) {
      return 0;
    }
    if (instruction->arguments[0].kind == IR_OPERAND_STRING) {
      if (!code_generator_binary_emit_cstring_literal_address(
              generator, context,
              instruction->arguments[0].name ? instruction->arguments[0].name
                                             : "",
              BINARY_GP_RCX)) {
        return 0;
      }
    } else if (!code_generator_binary_emit_operand_load(
                   generator, context, &instruction->arguments[0],
                   BINARY_GP_RCX)) {
      return 0;
    }
    if (!binary_emit_sub_rsp_imm32(&context->code,
                                   BINARY_WIN64_SHADOW_SPACE_SIZE) ||
        !binary_emit_call_placeholder(&context->code, &displacement_offset) ||
        !binary_call_relocation_table_add(&context->call_relocations,
                                          puts_symbol, displacement_offset) ||
        !binary_emit_add_rsp_imm32(&context->code,
                                   BINARY_WIN64_SHADOW_SPACE_SIZE) ||
        !binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RCX, 1) ||
        !binary_emit_sub_rsp_imm32(&context->code,
                                   BINARY_WIN64_SHADOW_SPACE_SIZE) ||
        !binary_emit_call_placeholder(&context->code, &displacement_offset) ||
        !binary_call_relocation_table_add(&context->call_relocations,
                                          exit_symbol, displacement_offset) ||
        !binary_emit_add_rsp_imm32(&context->code,
                                   BINARY_WIN64_SHADOW_SPACE_SIZE)) {
      if (!generator->has_error) {
        code_generator_set_error(generator,
                                 "Out of memory while emitting runtime trap "
                                 "call in function '%s'",
                                 context->function_name);
      }
      return 0;
    }
    return 1;
  }

  trap_pc_label = code_generator_generate_label(generator, "methdbg_trap_pc");
  if (!trap_pc_label) {
    code_generator_set_error(generator,
                             "Out of memory while creating runtime trap label");
    return 0;
  }

  if (!binary_label_table_define(&context->labels, trap_pc_label,
                                 context->code.size)) {
    code_generator_set_error(
        generator,
        "Failed to define runtime trap label in function '%s'",
        context->function_name);
    free(trap_pc_label);
    return 0;
  }

  if (!code_generator_binary_declare_external_symbol(generator, trap_symbol)) {
    free(trap_pc_label);
    return 0;
  }

  if (instruction->arguments[0].kind == IR_OPERAND_STRING) {
    if (!code_generator_binary_emit_cstring_literal_address(
            generator, context,
            instruction->arguments[0].name ? instruction->arguments[0].name
                                           : "",
            BINARY_GP_RCX)) {
      free(trap_pc_label);
      return 0;
    }
  } else if (!code_generator_binary_emit_operand_load(
                 generator, context, &instruction->arguments[0],
                 BINARY_GP_RCX)) {
    free(trap_pc_label);
    return 0;
  }

  if (!binary_emit_lea_reg_rip_placeholder(&context->code, BINARY_GP_RDX,
                                           &displacement_offset) ||
      !binary_label_fixup_table_add(&context->label_fixups, trap_pc_label,
                                    displacement_offset) ||
      !binary_emit_mov_reg_reg(&context->code, BINARY_GP_R8,
                               BINARY_GP_RBP) ||
      !binary_emit_sub_rsp_imm32(&context->code,
                                 BINARY_WIN64_SHADOW_SPACE_SIZE) ||
      !binary_emit_call_placeholder(&context->code, &displacement_offset) ||
      !binary_call_relocation_table_add(&context->call_relocations, trap_symbol,
                                        displacement_offset) ||
      !binary_emit_add_rsp_imm32(&context->code,
                                 BINARY_WIN64_SHADOW_SPACE_SIZE)) {
    if (!generator->has_error) {
      code_generator_set_error(generator,
                               "Out of memory while emitting runtime trap "
                               "call in function '%s'",
                               context->function_name);
    }
    free(trap_pc_label);
    return 0;
  }

  free(trap_pc_label);
  return 1;
}

static int code_generator_binary_emit_address_of(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  Symbol *symbol = NULL;
  int offset = 0;
  int is_function_symbol = 0;

  if (!generator || !context || !instruction ||
      instruction->lhs.kind != IR_OPERAND_SYMBOL || !instruction->lhs.name) {
    code_generator_set_error(generator,
                             "IR addr_of requires symbol operand in function "
                             "'%s'",
                             context ? context->function_name : "<unknown>");
    return 0;
  }

  symbol = generator->symbol_table
               ? symbol_table_lookup(generator->symbol_table,
                                     instruction->lhs.name)
               : NULL;
  is_function_symbol =
      (symbol && symbol->kind == SYMBOL_FUNCTION) ||
      code_generator_find_ir_function_binary(generator, instruction->lhs.name) !=
          NULL;

  if (is_function_symbol) {
    const char *link_name =
        code_generator_get_link_symbol_name(generator, instruction->lhs.name);
    if (!link_name || link_name[0] == '\0') {
      code_generator_set_error(generator,
                               "Invalid function symbol in IR addr_of");
      return 0;
    }
    if (!code_generator_binary_emit_symbol_address(
            generator, context, link_name, symbol && symbol->is_extern,
            BINARY_GP_RAX)) {
      return 0;
    }
  } else {
    if (symbol && symbol->type && symbol->type->kind == TYPE_STRING) {
      if (!code_generator_binary_emit_string_symbol_load(
              generator, context, instruction->lhs.name, symbol,
              BINARY_GP_RAX)) {
        return 0;
      }
    } else {
    offset =
        code_generator_binary_get_symbol_offset(context, instruction->lhs.name);
    if (offset > 0) {
      int address_ok =
          code_generator_binary_parameter_is_indirect(
              generator, context, instruction->lhs.name)
              ? binary_emit_mov_reg_mem(&context->code, BINARY_GP_RAX,
                                        BINARY_GP_RBP, -offset)
              : binary_emit_lea_reg_mem(&context->code, BINARY_GP_RAX,
                                        BINARY_GP_RBP, -offset);
      if (!address_ok) {
        code_generator_set_error(
            generator,
            "Out of memory while emitting local address in function '%s'",
            context->function_name);
        return 0;
      }
    } else if (symbol && symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
      const char *link_name =
          code_generator_get_link_symbol_name(generator, instruction->lhs.name);
      if (!link_name || link_name[0] == '\0') {
        code_generator_set_error(generator,
                                 "Invalid global symbol in IR addr_of");
        return 0;
      }
      if (!code_generator_binary_emit_symbol_address(
              generator, context, link_name, symbol->is_extern,
              BINARY_GP_RAX)) {
        return 0;
      }
    } else {
      code_generator_set_error(generator,
                               "Unknown addr_of symbol '%s' in function '%s'",
                               instruction->lhs.name, context->function_name);
      return 0;
    }
    }
  }

  return code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX);
}

static int code_generator_binary_load_needs_sign_extend(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *destination, int load_size) {
  Symbol *symbol = NULL;
  (void)context;

  if (load_size != 4 || !destination) {
    return 0;
  }

  if (destination->kind == IR_OPERAND_SYMBOL && destination->name &&
      generator->symbol_table) {
    symbol = symbol_table_lookup(generator->symbol_table, destination->name);
    if (symbol && symbol->type &&
        code_generator_binary_resolved_type_scalar_size(symbol->type) == 4) {
      return code_generator_binary_resolved_type_is_signed_integer(symbol->type);
    }
  }

  if (destination->kind == IR_OPERAND_TEMP && destination->name) {
    return 1;
  }

  return 1;
}

static int code_generator_binary_emit_load(CodeGenerator *generator,
                                           BinaryFunctionContext *context,
                                           const IRInstruction *instruction) {
  int size = 0;

  if (!generator || !context || !instruction) {
    return 0;
  }

  size = code_generator_binary_get_access_size(generator, context,
                                               &instruction->rhs);
  if (size <= 0) {
    return 0;
  }

  /* When the loaded value's destination is a promoted register DR, land the
   * value (and its sign-extension) directly in DR instead of computing in RAX
   * and copying back. Saves the trailing `mov DR, rax` on every pointer
   * dereference whose result is register-resident (the insertion-sort
   * `current = *prev` is exactly this). Falls back to RAX when the dest is
   * memory-homed. (The address operand is handled just below and may also stay
   * in its own register.) */
  BinaryGpRegister value_register = BINARY_GP_RAX;
  int value_in_dest_register =
      !instruction->is_float && instruction->dest.kind == IR_OPERAND_SYMBOL &&
      instruction->dest.name &&
      code_generator_binary_symbol_assigned_register(
          generator, context, instruction->dest.name, &value_register);
  if (!value_in_dest_register) {
    value_register = BINARY_GP_RAX;
  }

  /* If the address operand is itself a promoted pointer register, dereference
   * it directly rather than copying it into RAX first. (`current = *prev` with
   * prev in a register becomes `mov DR, [prev_reg]`.) The address register is
   * only read, never written by the load, so using it in place is safe even
   * when it differs from the value register. */
  BinaryGpRegister address_register = BINARY_GP_RAX;
  int address_in_register =
      instruction->lhs.kind == IR_OPERAND_SYMBOL && instruction->lhs.name &&
      code_generator_binary_symbol_assigned_register(
          generator, context, instruction->lhs.name, &address_register);
  if (!address_in_register) {
    address_register = BINARY_GP_RAX;
    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->lhs,
                                                 BINARY_GP_RAX)) {
      if (!generator->has_error) {
        code_generator_set_error(generator,
                                 "Out of memory while emitting IR load in "
                                 "function '%s'",
                                 context->function_name);
      }
      return 0;
    }
  }

  if (!code_generator_binary_emit_load_from_address(generator, context,
                                                    address_register, size,
                                                    value_register)) {
    if (!generator->has_error) {
      code_generator_set_error(generator,
                               "Out of memory while emitting IR load in "
                               "function '%s'",
                               context->function_name);
    }
    return 0;
  }
  /* x86-64: 32-bit integer loads into the low half zero-extend the register.
   * Signed int32 must sign-extend to int64 when held in a 64-bit slot/register.
   * Skip when dest is int32. */
  if (size == 4 && !instruction->is_float &&
      code_generator_binary_load_needs_sign_extend(generator, context,
                                                   &instruction->dest, size) &&
      !binary_emit_movsxd_reg_reg32(&context->code, value_register,
                                    value_register)) {
    if (!generator->has_error) {
      code_generator_set_error(generator,
                               "Out of memory while emitting IR load in "
                               "function '%s'",
                               context->function_name);
    }
    return 0;
  }

  /* Value already resides in the destination register; no store needed. */
  if (value_in_dest_register) {
    return 1;
  }

  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_RAX)) {
    if (!generator->has_error) {
      code_generator_set_error(generator,
                               "Out of memory while emitting IR load in "
                               "function '%s'",
                               context->function_name);
    }
    return 0;
  }

  return 1;
}

static int code_generator_binary_emit_store(CodeGenerator *generator,
                                            BinaryFunctionContext *context,
                                            const IRInstruction *instruction) {
  int size = 0;

  if (!generator || !context || !instruction) {
    return 0;
  }

  size = code_generator_binary_get_access_size(generator, context,
                                               &instruction->rhs);
  if (size <= 0) {
    return 0;
  }

  /* The stored value lands in RCX by default. If the value operand is itself a
   * promoted, non-float register, store straight from that register and skip
   * the `mov rcx, SR` copy (`*scan = current` with current in a register
   * becomes `mov [addr], current_reg`). Promotion never uses RCX, so the value
   * register can never collide with the RCX default of any other path. */
  BinaryGpRegister value_register = BINARY_GP_RCX;
  int value_in_register =
      !instruction->is_float && instruction->lhs.kind == IR_OPERAND_SYMBOL &&
      instruction->lhs.name &&
      code_generator_binary_symbol_assigned_register(
          generator, context, instruction->lhs.name, &value_register);
  if (!value_in_register) {
    value_register = BINARY_GP_RCX;
    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->lhs,
                                                 BINARY_GP_RCX)) {
      if (!generator->has_error) {
        code_generator_set_error(generator,
                                 "Out of memory while emitting IR store in "
                                 "function '%s'",
                                 context->function_name);
      }
      return 0;
    }
  }

  /* Narrow/widen the value to the destination's float precision when the
   * stored expression's width differs (e.g. float64 expression -> float32
   * member). instruction->float_bits is the destination width. */
  if (instruction->is_float && instruction->float_bits) {
    int value_bits = code_generator_binary_operand_float_bits(
        generator, context, &instruction->lhs);
    if (value_bits &&
        !code_generator_binary_emit_float_reg_convert(
            context, value_register, value_bits, instruction->float_bits)) {
      code_generator_set_error(generator,
                               "Out of memory while converting float store "
                               "precision in function '%s'",
                               context->function_name);
      return 0;
    }
  }

  /* If the store address is a promoted pointer register, store through it
   * directly instead of copying it into RAX first (`*scan = current` with scan
   * in a register becomes `mov [scan_reg], ecx`). The value is already in RCX
   * and the address register is only read, so this is safe even when they
   * differ. */
  BinaryGpRegister store_address_register = BINARY_GP_RAX;
  int store_address_in_register =
      instruction->dest.kind == IR_OPERAND_SYMBOL && instruction->dest.name &&
      code_generator_binary_symbol_assigned_register(
          generator, context, instruction->dest.name, &store_address_register);
  if (!store_address_in_register) {
    store_address_register = BINARY_GP_RAX;
    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->dest,
                                                 BINARY_GP_RAX)) {
      if (!generator->has_error) {
        code_generator_set_error(generator,
                                 "Out of memory while emitting IR store in "
                                 "function '%s'",
                                 context->function_name);
      }
      return 0;
    }
  }

  if (!code_generator_binary_emit_store_to_address(generator, context,
                                                   store_address_register, size,
                                                   value_register)) {
    if (!generator->has_error) {
      code_generator_set_error(generator,
                               "Out of memory while emitting IR store in "
                               "function '%s'",
                               context->function_name);
    }
    return 0;
  }

  return 1;
}

static int code_generator_binary_emit_new(CodeGenerator *generator,
                                          BinaryFunctionContext *context,
                                          const IRInstruction *instruction) {
  size_t displacement_offset = 0;
  const char *allocator_name = "calloc";

  if (!generator || !context || !instruction) {
    return 0;
  }

  if (!code_generator_binary_declare_external_symbol(generator, allocator_name)) {
    return 0;
  }

  if (instruction->rhs.kind == IR_OPERAND_INT && instruction->rhs.int_value > 0) {
    if (!binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RDX,
                                   (uint64_t)instruction->rhs.int_value)) {
      code_generator_set_error(generator,
                                "Out of memory while emitting allocation size");
      return 0;
    }
  } else if (instruction->rhs.kind == IR_OPERAND_NONE ||
             (instruction->rhs.kind == IR_OPERAND_INT &&
               instruction->rhs.int_value <= 0)) {
    if (!binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RDX, 8)) {
      code_generator_set_error(generator,
                                "Out of memory while emitting allocation size");
      return 0;
    }
  } else if (!code_generator_binary_emit_operand_load(
                  generator, context, &instruction->rhs, BINARY_GP_RDX)) {
    return 0;
  }

  if (!binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RCX, 1) ||
      !binary_emit_sub_rsp_imm32(&context->code,
                                  BINARY_WIN64_SHADOW_SPACE_SIZE) ||
      !binary_emit_call_placeholder(&context->code, &displacement_offset) ||
      !binary_call_relocation_table_add(&context->call_relocations,
                                        allocator_name, displacement_offset) ||
      !binary_emit_add_rsp_imm32(&context->code,
                                 BINARY_WIN64_SHADOW_SPACE_SIZE) ||
      !code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_RAX)) {
    if (!generator->has_error) {
      code_generator_set_error(generator,
                               "Out of memory while emitting IR new in "
                               "function '%s'",
                               context->function_name);
    }
    return 0;
  }

  return 1;
}

static int code_generator_binary_emit_cast(CodeGenerator *generator,
                                           BinaryFunctionContext *context,
                                           const IRInstruction *instruction) {
  Type *target_type = NULL;
  int target_is_float = 0;
  int target_is_unsigned = 0;
  int target_size = 8;

  if (!generator || !context || !instruction || !instruction->text) {
    return 0;
  }

  target_type = generator->type_checker
                    ? type_checker_get_type_by_name(generator->type_checker,
                                                    instruction->text)
                    : NULL;
  target_is_float =
      target_type ? code_generator_is_floating_point_type(target_type) : 0;
  if (target_type) {
    target_is_unsigned = target_type->kind == TYPE_UINT8 ||
                         target_type->kind == TYPE_UINT16 ||
                         target_type->kind == TYPE_UINT32 ||
                         target_type->kind == TYPE_UINT64;
    target_size = (int)target_type->size;
    if (target_type->kind == TYPE_POINTER ||
        target_type->kind == TYPE_FUNCTION_POINTER) {
      target_size = 8;
    }
  }
  if (target_size <= 0) {
    target_size = 8;
  }

  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RAX)) {
    return 0;
  }

  /* Source float width is carried on the CAST instruction (set by
   * ir_lowering); target float width is derived from the cast's named type. */
  int src_fbits = (instruction->float_bits == 32) ? 32 : 64;
  int dst_fbits =
      code_generator_binary_resolved_type_float_bits(target_type);

  if (instruction->is_float && !target_is_float) {
    /* float -> int: truncate at the SOURCE precision. */
    if (src_fbits == 32) {
      if (!binary_emit_movd_xmm_reg(&context->code, BINARY_XMM0,
                                    BINARY_GP_RAX) ||
          !binary_emit_cvttss2si_reg_xmm(&context->code, BINARY_GP_RAX,
                                         BINARY_XMM0)) {
        goto emit_failure;
      }
    } else if (!binary_emit_movq_xmm_reg(&context->code, BINARY_XMM0,
                                         BINARY_GP_RAX) ||
               !binary_emit_cvttsd2si_reg_xmm(&context->code, BINARY_GP_RAX,
                                              BINARY_XMM0)) {
      goto emit_failure;
    }
  } else if (!instruction->is_float && target_is_float) {
    /* int -> float: produce a value at the TARGET precision. */
    if (dst_fbits == 32) {
      if (!binary_emit_cvtsi2ss_xmm_reg(&context->code, BINARY_XMM0,
                                        BINARY_GP_RAX) ||
          !binary_emit_movd_reg_xmm(&context->code, BINARY_GP_RAX,
                                    BINARY_XMM0)) {
        goto emit_failure;
      }
    } else if (!binary_emit_cvtsi2sd_xmm_reg(&context->code, BINARY_XMM0,
                                             BINARY_GP_RAX) ||
               !binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                         BINARY_XMM0)) {
      goto emit_failure;
    }
  } else if (instruction->is_float && target_is_float) {
    /* float -> float: convert precision only when the widths differ. */
    if (src_fbits == 32 && dst_fbits == 64) {
      if (!binary_emit_movd_xmm_reg(&context->code, BINARY_XMM0,
                                    BINARY_GP_RAX) ||
          !binary_emit_cvtss2sd_xmm_xmm(&context->code, BINARY_XMM0,
                                        BINARY_XMM0) ||
          !binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                    BINARY_XMM0)) {
        goto emit_failure;
      }
    } else if (src_fbits == 64 && dst_fbits == 32) {
      if (!binary_emit_movq_xmm_reg(&context->code, BINARY_XMM0,
                                    BINARY_GP_RAX) ||
          !binary_emit_cvtsd2ss_xmm_xmm(&context->code, BINARY_XMM0,
                                        BINARY_XMM0) ||
          !binary_emit_movd_reg_xmm(&context->code, BINARY_GP_RAX,
                                    BINARY_XMM0)) {
        goto emit_failure;
      }
    }
    /* same width -> raw bits already correct, nothing to emit. */
  } else if (target_size == 1) {
    if ((target_is_unsigned &&
         !binary_emit_movzx_eax_al(&context->code)) ||
        (!target_is_unsigned &&
         !binary_emit_movsx_rax_al(&context->code))) {
      goto emit_failure;
    }
  } else if (target_size == 2) {
    if ((target_is_unsigned &&
         !binary_emit_movzx_eax_ax(&context->code)) ||
        (!target_is_unsigned &&
         !binary_emit_movsx_rax_ax(&context->code))) {
      goto emit_failure;
    }
  } else if (target_size == 4) {
    if ((target_is_unsigned &&
         !binary_emit_mov_eax_eax(&context->code)) ||
        (!target_is_unsigned &&
         !binary_emit_movsxd_rax_eax(&context->code))) {
      goto emit_failure;
    }
  }

  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }

  return 1;

emit_failure:
  code_generator_set_error(
      generator,
      "Out of memory while emitting IR cast in function '%s'",
      context->function_name);
  return 0;
}

static int code_generator_binary_validate_indirect_call(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  Symbol *symbol = NULL;
  Type *function_type = NULL;

  if (!generator || !context || !instruction ||
      instruction->lhs.kind != IR_OPERAND_SYMBOL || !instruction->lhs.name) {
    return 1;
  }

  symbol = generator->symbol_table
               ? symbol_table_lookup(generator->symbol_table,
                                     instruction->lhs.name)
               : NULL;
  if (!symbol || !symbol->type || symbol->type->kind != TYPE_FUNCTION_POINTER) {
    return 1;
  }

  function_type = symbol->type;
  if (!code_generator_binary_resolved_type_is_abi_supported(
          function_type->fn_return_type, 1)) {
    code_generator_set_error(
        generator,
        "Direct object backend only supports integer/pointer/string/float64 indirect "
        "call returns in function '%s'",
        context->function_name);
    return 0;
  }

  if (instruction->argument_count != function_type->fn_param_count) {
    code_generator_set_error(
        generator,
        "Indirect call argument mismatch while lowering direct object "
        "function '%s'",
        context->function_name);
    return 0;
  }

  for (size_t i = 0; i < function_type->fn_param_count; i++) {
    if (!code_generator_binary_resolved_type_is_abi_supported(
            function_type->fn_param_types[i], 0)) {
      code_generator_set_error(
          generator,
          "Direct object backend only supports integer/pointer/string/float64 "
          "indirect call arguments in function '%s'",
          context->function_name);
      return 0;
    }
  }

  return 1;
}

static int code_generator_binary_emit_call(CodeGenerator *generator,
                                           BinaryFunctionContext *context,
                                           const IRInstruction *instruction) {
  Symbol *function_symbol = NULL;
  IRFunction *target_ir_function = NULL;

  if (!generator || !context || !instruction || !instruction->text ||
      instruction->text[0] == '\0') {
    return 0;
  }

  if (strcmp(instruction->text, "mettle_crash_trap") == 0) {
    return code_generator_binary_emit_runtime_trap_call(generator, context,
                                                        instruction);
  }

  if (!code_generator_binary_validate_call(generator, context, instruction)) {
    return 0;
  }

  function_symbol = generator->symbol_table
                        ? symbol_table_lookup(generator->symbol_table,
                                              instruction->text)
                        : NULL;
  target_ir_function =
      code_generator_find_ir_function_binary(generator, instruction->text);

  /* Per-arg INDIRECT classification and per-call indirect-temp region. The
   * region lives at [rsp + 0 .. indirect_temp_region) within the call's
   * sub-rsp window, followed by shadow space and any stack arg slots. */
  size_t argument_count = instruction->argument_count;
  int *is_indirect_arg =
      argument_count > 0 ? calloc(argument_count, sizeof(int)) : NULL;
  int *indirect_arg_offset =
      argument_count > 0 ? calloc(argument_count, sizeof(int)) : NULL;
  size_t *indirect_arg_size =
      argument_count > 0 ? calloc(argument_count, sizeof(size_t)) : NULL;
  if (argument_count > 0 &&
      (!is_indirect_arg || !indirect_arg_offset || !indirect_arg_size)) {
    free(is_indirect_arg);
    free(indirect_arg_offset);
    free(indirect_arg_size);
    code_generator_set_error(generator,
                             "Out of memory planning indirect args");
    return 0;
  }
  int indirect_temp_region = 0;
  for (size_t i = 0; i < argument_count; i++) {
    Type *param_t =
        function_symbol && function_symbol->kind == SYMBOL_FUNCTION &&
                function_symbol->data.function.parameter_types
            ? function_symbol->data.function.parameter_types[i]
            : NULL;
    if (code_generator_abi_classify(param_t) == ABI_PASS_INDIRECT) {
      is_indirect_arg[i] = 1;
      size_t sz = code_generator_abi_type_size(param_t);
      indirect_arg_size[i] = sz;
      indirect_arg_offset[i] = indirect_temp_region;
      indirect_temp_region += (int)((sz + 7u) & ~(size_t)7);
    }
  }
  if (indirect_temp_region > 0) {
    indirect_temp_region = (indirect_temp_region + 15) & ~15;
  }

  /* INDIRECT-return classification. The hidden out-pointer (Win64: rcx)
   * occupies ABI slot 0 and shifts every user arg up by one. */
  Type *call_return_type = NULL;
  if (function_symbol && function_symbol->kind == SYMBOL_FUNCTION) {
    call_return_type = function_symbol->data.function.return_type
                           ? function_symbol->data.function.return_type
                           : function_symbol->type;
  }
  int return_is_indirect =
      (code_generator_abi_classify(call_return_type) == ABI_PASS_INDIRECT) ? 1
                                                                           : 0;
  size_t hidden_arg_count = return_is_indirect ? 1 : 0;
  int return_slot_rbp_offset = 0;
  if (return_is_indirect) {
    if (context->indirect_return_slot_cursor >=
        context->indirect_return_slot_count) {
      free(is_indirect_arg);
      free(indirect_arg_offset);
      free(indirect_arg_size);
      code_generator_set_error(
          generator,
          "Indirect-return frame slot not assigned for call '%s'",
          instruction->text);
      return 0;
    }
    return_slot_rbp_offset = context->indirect_return_slot_offsets
                                 [context->indirect_return_slot_cursor++];
  }

  /* Effective ABI argument count includes the hidden out-pointer. */
  size_t effective_arg_count = argument_count + hidden_arg_count;
  size_t stack_argument_count =
      effective_arg_count > BINARY_WIN64_REGISTER_ARG_COUNT
          ? effective_arg_count - BINARY_WIN64_REGISTER_ARG_COUNT
          : 0;
  if (stack_argument_count >
      (size_t)(INT_MAX / BINARY_FUNCTION_STACK_SLOT_SIZE)) {
    free(is_indirect_arg);
    free(indirect_arg_offset);
    free(indirect_arg_size);
    code_generator_set_error(generator,
                             "Too many call arguments in function '%s'",
                             context->function_name);
    return 0;
  }

  int call_stack_total = indirect_temp_region +
                         BINARY_WIN64_SHADOW_SPACE_SIZE +
                         (int)(stack_argument_count *
                               BINARY_FUNCTION_STACK_SLOT_SIZE);
  if (!binary_align_up_int(call_stack_total, 16, &call_stack_total)) {
    free(is_indirect_arg);
    free(indirect_arg_offset);
    free(indirect_arg_size);
    code_generator_set_error(generator,
                             "Call frame too large in function '%s'",
                             context->function_name);
    return 0;
  }

  if (call_stack_total > 0 &&
      !binary_emit_sub_rsp_imm32(&context->code, (uint32_t)call_stack_total)) {
    free(is_indirect_arg);
    free(indirect_arg_offset);
    free(indirect_arg_size);
    code_generator_set_error(generator,
                             "Out of memory while emitting call frame");
    return 0;
  }

  /* Materialize INDIRECT args: memcpy each struct into its per-call temp. */
  for (size_t i = 0; i < argument_count; i++) {
    if (!is_indirect_arg[i]) continue;
    /* src into rax */
    if (!code_generator_binary_emit_indirect_source_address(
            generator, context, &instruction->arguments[i], BINARY_GP_RAX)) {
      free(is_indirect_arg);
      free(indirect_arg_offset);
      free(indirect_arg_size);
      return 0;
    }
    /* dst = lea rdx, [rsp + offset] (offset within indirect_temp_region) */
    if (!binary_emit_lea_reg_mem(&context->code, BINARY_GP_RDX,
                                 BINARY_GP_RSP, indirect_arg_offset[i]) ||
        !code_generator_binary_emit_rep_movsb(
            generator, context, BINARY_GP_RAX, BINARY_GP_RDX,
            indirect_arg_size[i])) {
      free(is_indirect_arg);
      free(indirect_arg_offset);
      free(indirect_arg_size);
      code_generator_set_error(generator,
                               "Out of memory copying INDIRECT call arg");
      return 0;
    }
  }

  /* Stack args: skip slot 0 if hidden out-ptr is present (it's a register
   * arg on Win64 anyway). For Win64 every arg has a stack slot above the
   * shadow space if it doesn't fit in registers. The first 4 ABI slots are
   * register; slots >= 4 are stack. */
  for (size_t i = 0; i < argument_count; i++) {
    size_t abi_slot = i + hidden_arg_count;
    if (abi_slot < BINARY_WIN64_REGISTER_ARG_COUNT) continue;
    int slot_offset = indirect_temp_region + BINARY_WIN64_SHADOW_SPACE_SIZE +
                      (int)((abi_slot - BINARY_WIN64_REGISTER_ARG_COUNT) *
                            BINARY_FUNCTION_STACK_SLOT_SIZE);
    if (is_indirect_arg[i]) {
      /* Place &temp into the stack slot. */
      if (!binary_emit_lea_reg_mem(&context->code, BINARY_GP_RAX,
                                   BINARY_GP_RSP, indirect_arg_offset[i]) ||
          !binary_emit_mov_mem_reg(&context->code, BINARY_GP_RSP, slot_offset,
                                   BINARY_GP_RAX)) {
        free(is_indirect_arg);
        free(indirect_arg_offset);
        free(indirect_arg_size);
        code_generator_set_error(generator,
                                 "Out of memory writing INDIRECT stack arg");
        return 0;
      }
      continue;
    }
    Type *parameter_type =
        function_symbol && function_symbol->kind == SYMBOL_FUNCTION &&
                function_symbol->data.function.parameter_types
            ? function_symbol->data.function.parameter_types[i]
            : NULL;
    if (!code_generator_binary_emit_call_argument_load(
            generator, context, &instruction->arguments[i], parameter_type,
            BINARY_GP_RAX) ||
        !binary_emit_mov_mem_reg(&context->code, BINARY_GP_RSP, slot_offset,
                                 BINARY_GP_RAX)) {
      free(is_indirect_arg);
      free(indirect_arg_offset);
      free(indirect_arg_size);
      if (!generator->has_error) {
        code_generator_set_error(generator,
                                 "Out of memory while materializing call args");
      }
      return 0;
    }
  }

  /* Register args. ABI slot = i + hidden_arg_count. */
  for (size_t i = 0; i < argument_count; i++) {
    size_t abi_slot = i + hidden_arg_count;
    if (abi_slot >= BINARY_WIN64_REGISTER_ARG_COUNT) continue;
    if (is_indirect_arg[i]) {
      /* lea reg, [rsp + offset] */
      if (!binary_emit_lea_reg_mem(
              &context->code, BINARY_WIN64_INT_PARAM_REGISTERS[abi_slot],
              BINARY_GP_RSP, indirect_arg_offset[i])) {
        free(is_indirect_arg);
        free(indirect_arg_offset);
        free(indirect_arg_size);
        code_generator_set_error(generator,
                                 "Out of memory loading INDIRECT arg ptr");
        return 0;
      }
      continue;
    }
    Type *parameter_type =
        function_symbol && function_symbol->kind == SYMBOL_FUNCTION &&
                function_symbol->data.function.parameter_types
            ? function_symbol->data.function.parameter_types[i]
            : NULL;
    int param_fbits =
        code_generator_binary_resolved_type_float_bits(parameter_type);
    if ((param_fbits &&
         !code_generator_binary_emit_float_call_argument(
             generator, context, &instruction->arguments[i], parameter_type,
             param_fbits, BINARY_WIN64_FLOAT_PARAM_REGISTERS[abi_slot])) ||
        (!param_fbits &&
         !code_generator_binary_emit_call_argument_load(
             generator, context, &instruction->arguments[i], parameter_type,
             BINARY_WIN64_INT_PARAM_REGISTERS[abi_slot]))) {
      free(is_indirect_arg);
      free(indirect_arg_offset);
      free(indirect_arg_size);
      return 0;
    }
  }

  /* Hidden out-pointer for INDIRECT return: load &return_slot into rcx
   * LAST, after any user-arg memcpy that may have clobbered rcx. The slot
   * lives in the caller's function frame, so it survives the call's
   * stack teardown. */
  if (return_is_indirect) {
    if (!binary_emit_lea_reg_mem(&context->code, BINARY_GP_RCX, BINARY_GP_RBP,
                                 -return_slot_rbp_offset)) {
      free(is_indirect_arg);
      free(indirect_arg_offset);
      free(indirect_arg_size);
      code_generator_set_error(generator,
                               "Out of memory loading hidden out-ptr");
      return 0;
    }
  }
  free(is_indirect_arg);
  free(indirect_arg_offset);
  free(indirect_arg_size);

  size_t displacement_offset = 0;
  const char *link_target =
      code_generator_get_link_symbol_name(generator, instruction->text);
  if (!link_target || link_target[0] == '\0') {
    code_generator_set_error(generator, "Invalid call target '%s'",
                             instruction->text);
    return 0;
  }

  if (!target_ir_function &&
      !code_generator_binary_declare_external_symbol(generator, link_target)) {
    return 0;
  }

  if (!binary_emit_call_placeholder(&context->code, &displacement_offset) ||
      !binary_call_relocation_table_add(&context->call_relocations, link_target,
                                        displacement_offset)) {
    code_generator_set_error(generator,
                             "Out of memory while emitting call relocation");
    return 0;
  }

  if (call_stack_total > 0 &&
      !binary_emit_add_rsp_imm32(&context->code, (uint32_t)call_stack_total)) {
    code_generator_set_error(generator,
                             "Out of memory while restoring call frame");
    return 0;
  }

  /* INDIRECT return: rax should hold the slot address by ABI; re-materialize
   * from our known frame slot for safety (some callees may not preserve
   * exactly; the slot lives in our frame so the lea is always correct). */
  if (return_is_indirect) {
    if (!binary_emit_lea_reg_mem(&context->code, BINARY_GP_RAX, BINARY_GP_RBP,
                                 -return_slot_rbp_offset)) {
      code_generator_set_error(generator,
                               "Out of memory materializing INDIRECT result");
      return 0;
    }
    /* Caller-side disposition: if dest is a struct symbol, memcpy into its
     * storage. If dest is a temp, register the temp in the side-table so
     * downstream IR_OP_ASSIGN / indirect-arg consumption knows the temp
     * carries a pointer-to-struct semantics. */
    if (instruction->dest.kind == IR_OPERAND_SYMBOL && instruction->dest.name) {
      Symbol *dest_sym =
          symbol_table_lookup(generator->symbol_table, instruction->dest.name);
      if (!dest_sym || !dest_sym->type ||
          code_generator_type_is_aggregate(dest_sym->type)) {
        if (!code_generator_binary_emit_struct_destination_address(
                generator, context, instruction->dest.name, BINARY_GP_RDX)) {
          return 0;
        }
        if (!code_generator_binary_emit_rep_movsb(
                generator, context, BINARY_GP_RAX, BINARY_GP_RDX,
                code_generator_abi_type_size(call_return_type))) {
          code_generator_set_error(
              generator, "Out of memory copying INDIRECT call result");
          return 0;
        }
        return 1;
      }
    }
    if (instruction->dest.kind == IR_OPERAND_TEMP && instruction->dest.name) {
      if (!binary_indirect_temp_add(
              context, instruction->dest.name,
              code_generator_abi_type_size(call_return_type))) {
        code_generator_set_error(generator,
                                 "Out of memory tagging INDIRECT-return temp");
        return 0;
      }
    }
    /* Default store (8-byte spill of the pointer) keeps the pointer alive
     * in the temp's slot for downstream consumers. */
    if (!code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX)) {
      return 0;
    }
    return 1;
  }

  if (function_symbol && function_symbol->kind == SYMBOL_FUNCTION) {
    int ret_fbits = code_generator_binary_resolved_type_float_bits(
        function_symbol->data.function.return_type);
    if (((ret_fbits == 32 &&
          !binary_emit_movd_reg_xmm(&context->code, BINARY_GP_RAX,
                                    BINARY_XMM0)) ||
         (ret_fbits == 64 &&
          !binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                    BINARY_XMM0)))) {
      code_generator_set_error(generator,
                               "Out of memory while materializing float call "
                               "return in function '%s'",
                               context->function_name);
      return 0;
    }
  }

  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }

  return 1;
}

static int code_generator_binary_emit_call_indirect(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  Symbol *symbol = NULL;
  Type *function_type = NULL;
  size_t stack_argument_count = 0;
  int call_stack_total = 0;

  if (!generator || !context || !instruction) {
    return 0;
  }

  if (!code_generator_binary_validate_indirect_call(generator, context,
                                                    instruction)) {
    return 0;
  }

  symbol = generator->symbol_table
               ? symbol_table_lookup(generator->symbol_table,
                                     instruction->lhs.name)
               : NULL;
  function_type =
      (symbol && symbol->type && symbol->type->kind == TYPE_FUNCTION_POINTER)
          ? symbol->type
          : NULL;

  stack_argument_count =
      instruction->argument_count > BINARY_WIN64_REGISTER_ARG_COUNT
          ? instruction->argument_count - BINARY_WIN64_REGISTER_ARG_COUNT
          : 0;
  if (stack_argument_count >
      (size_t)(INT_MAX / BINARY_FUNCTION_STACK_SLOT_SIZE)) {
    code_generator_set_error(generator,
                             "Too many indirect call arguments in function "
                             "'%s'",
                             context->function_name);
    return 0;
  }

  call_stack_total = BINARY_WIN64_SHADOW_SPACE_SIZE +
                     (int)(stack_argument_count *
                           BINARY_FUNCTION_STACK_SLOT_SIZE);
  if (!binary_align_up_int(call_stack_total, 16, &call_stack_total)) {
    code_generator_set_error(generator,
                             "Indirect call frame too large in function '%s'",
                             context->function_name);
    return 0;
  }

  if (call_stack_total > 0 &&
      !binary_emit_sub_rsp_imm32(&context->code, (uint32_t)call_stack_total)) {
    code_generator_set_error(generator,
                             "Out of memory while emitting indirect call frame");
    return 0;
  }

  for (size_t i = BINARY_WIN64_REGISTER_ARG_COUNT;
       i < instruction->argument_count; i++) {
    int slot_offset = BINARY_WIN64_SHADOW_SPACE_SIZE +
                      (int)((i - BINARY_WIN64_REGISTER_ARG_COUNT) *
                            BINARY_FUNCTION_STACK_SLOT_SIZE);
    Type *parameter_type =
        function_type && function_type->fn_param_types
            ? function_type->fn_param_types[i]
            : NULL;
    if (!code_generator_binary_emit_call_argument_load(
            generator, context, &instruction->arguments[i], parameter_type,
            BINARY_GP_R10) ||
        !binary_emit_mov_mem_reg(&context->code, BINARY_GP_RSP, slot_offset,
                                 BINARY_GP_R10)) {
      if (!generator->has_error) {
        code_generator_set_error(
            generator, "Out of memory while materializing indirect call args");
      }
      return 0;
    }
  }

  size_t register_argument_count = instruction->argument_count;
  if (register_argument_count > BINARY_WIN64_REGISTER_ARG_COUNT) {
    register_argument_count = BINARY_WIN64_REGISTER_ARG_COUNT;
  }
  for (size_t i = 0; i < register_argument_count; i++) {
    Type *parameter_type =
        function_type && function_type->fn_param_types
            ? function_type->fn_param_types[i]
            : NULL;
    int param_fbits =
        code_generator_binary_resolved_type_float_bits(parameter_type);
    if ((param_fbits &&
         !code_generator_binary_emit_float_call_argument(
             generator, context, &instruction->arguments[i], parameter_type,
             param_fbits, BINARY_WIN64_FLOAT_PARAM_REGISTERS[i])) ||
        (!param_fbits &&
         !code_generator_binary_emit_call_argument_load(
             generator, context, &instruction->arguments[i], parameter_type,
             BINARY_WIN64_INT_PARAM_REGISTERS[i]))) {
      return 0;
    }
  }

  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RAX) ||
      !binary_emit_call_reg(&context->code, BINARY_GP_RAX)) {
    if (!generator->has_error) {
      code_generator_set_error(
          generator,
          "Out of memory while emitting indirect call in function '%s'",
          context->function_name);
    }
    return 0;
  }

  if (call_stack_total > 0 &&
      !binary_emit_add_rsp_imm32(&context->code, (uint32_t)call_stack_total)) {
    code_generator_set_error(
        generator, "Out of memory while restoring indirect call frame");
    return 0;
  }

  if (function_type) {
    int ret_fbits = code_generator_binary_resolved_type_float_bits(
        function_type->fn_return_type);
    if ((ret_fbits == 32 &&
         !binary_emit_movd_reg_xmm(&context->code, BINARY_GP_RAX,
                                   BINARY_XMM0)) ||
        (ret_fbits == 64 &&
         !binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                   BINARY_XMM0))) {
      code_generator_set_error(generator,
                               "Out of memory while materializing float "
                               "indirect call return in function '%s'",
                               context->function_name);
      return 0;
    }
  }

  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }

  return 1;
}

static int code_generator_binary_emit_rotate_add(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryGpRegister reg_next = BINARY_GP_RAX;
  BinaryGpRegister reg_a = BINARY_GP_R10;
  BinaryGpRegister reg_b = BINARY_GP_R11;
  int has_next = 0;
  int has_a = 0;
  int has_b = 0;

  if (!generator || !context || !instruction ||
      instruction->dest.kind != IR_OPERAND_SYMBOL || !instruction->dest.name ||
      instruction->lhs.kind != IR_OPERAND_SYMBOL || !instruction->lhs.name ||
      instruction->rhs.kind != IR_OPERAND_SYMBOL || !instruction->rhs.name) {
    code_generator_set_error(generator, "Malformed IR rotate_add in '%s'",
                             context->function_name);
    return 0;
  }

  has_next = code_generator_binary_symbol_assigned_register(
      generator, context, instruction->dest.name, &reg_next);
  has_a = code_generator_binary_symbol_assigned_register(
      generator, context, instruction->lhs.name, &reg_a);
  has_b = code_generator_binary_symbol_assigned_register(
      generator, context, instruction->rhs.name, &reg_b);

  if (has_next && has_a && has_b) {
    if ((!binary_emit_lea_reg_reg(&context->code, reg_next, reg_a, reg_b) &&
         (!binary_emit_mov_reg_reg(&context->code, reg_next, reg_a) ||
          !binary_emit_alu_reg_reg(&context->code, 0x01, reg_next, reg_b))) ||
        !binary_emit_mov_reg_reg(&context->code, reg_a, reg_b) ||
        !binary_emit_mov_reg_reg(&context->code, reg_b, reg_next)) {
      return 0;
    }
    return 1;
  }

  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RAX)) {
    return 0;
  }
  if (has_b) {
    if (!binary_emit_lea_reg_reg(&context->code, BINARY_GP_RAX,
                                 BINARY_GP_RAX, reg_b) &&
        !binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RAX, reg_b)) {
      return 0;
    }
  } else if (!code_generator_binary_emit_operand_load(generator, context,
                                                      &instruction->rhs,
                                                      BINARY_GP_R10) ||
             (!binary_emit_lea_reg_reg(&context->code, BINARY_GP_RAX,
                                       BINARY_GP_RAX, BINARY_GP_R10) &&
              !binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RAX,
                                       BINARY_GP_R10))) {
    return 0;
  }

  if (!binary_emit_mov_reg_reg(&context->code, BINARY_GP_R11, BINARY_GP_RAX)) {
    return 0;
  }
  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_R11)) {
    return 0;
  }

  if (has_a && has_b) {
    if (!binary_emit_mov_reg_reg(&context->code, reg_a, reg_b)) {
      return 0;
    }
  } else if (has_a) {
    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->rhs,
                                                 BINARY_GP_R10) ||
        !binary_emit_mov_reg_reg(&context->code, reg_a, BINARY_GP_R10)) {
      return 0;
    }
  } else if (!code_generator_binary_emit_operand_load(generator, context,
                                                      &instruction->rhs,
                                                      BINARY_GP_R10) ||
             !code_generator_binary_emit_destination_store(
                 generator, context, &instruction->lhs, BINARY_GP_R10)) {
    return 0;
  }

  if (has_b) {
    if (!binary_emit_mov_reg_reg(&context->code, reg_b, BINARY_GP_R11)) {
      return 0;
    }
  } else if (!code_generator_binary_emit_destination_store(
                 generator, context, &instruction->rhs, BINARY_GP_R11)) {
    return 0;
  }

  return 1;
}

static int code_generator_binary_emit_binary(CodeGenerator *generator,
                                             BinaryFunctionContext *context,
                                             const IRInstruction *instruction) {
  const char *op = NULL;
  unsigned char condition_opcode = 0;
  int is_compare = 0;
  Type *result_type = NULL;

  if (!generator || !context || !instruction || !instruction->text) {
    return 0;
  }

  op = instruction->text;
  result_type = instruction->ast_ref
                    ? code_generator_infer_expression_type(generator,
                                                           instruction->ast_ref)
                    : NULL;
  if (result_type && result_type->kind == TYPE_STRING && strcmp(op, "+") == 0) {
    const char *allocator_name = "calloc";
    size_t displacement_offset = 0;
    size_t loop_fixup = 0;
    char *left_done_label = NULL;
    char *left_loop_label = NULL;
    char *right_done_label = NULL;
    char *right_loop_label = NULL;

    if (!code_generator_binary_declare_external_symbol(generator,
                                                       allocator_name)) {
      return 0;
    }

    left_done_label = code_generator_generate_label(generator, "concat_left_done");
    left_loop_label = code_generator_generate_label(generator, "concat_left_loop");
    right_done_label =
        code_generator_generate_label(generator, "concat_right_done");
    right_loop_label =
        code_generator_generate_label(generator, "concat_right_loop");
    if (!left_done_label || !left_loop_label || !right_done_label ||
        !right_loop_label) {
      code_generator_set_error(generator,
                               "Out of memory while creating concat labels in "
                               "function '%s'",
                               context->function_name);
      free(left_done_label);
      free(left_loop_label);
      free(right_done_label);
      free(right_loop_label);
      return 0;
    }

    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->lhs,
                                                 BINARY_GP_RAX) ||
        !binary_emit_push_reg(&context->code, BINARY_GP_RAX) ||
        !code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->rhs,
                                                 BINARY_GP_R10) ||
        !binary_emit_pop_reg(&context->code, BINARY_GP_RAX) ||
        !binary_emit_mov_reg_mem(&context->code, BINARY_GP_RCX, BINARY_GP_RAX,
                                 8) ||
        !binary_emit_mov_reg_mem(&context->code, BINARY_GP_RDX, BINARY_GP_R10,
                                 8) ||
        !binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RCX,
                                 BINARY_GP_RDX) ||
        !binary_emit_sub_rsp_imm32(&context->code, 24) ||
        !binary_emit_mov_mem_reg(&context->code, BINARY_GP_RSP, 0,
                                 BINARY_GP_R10) ||
        !binary_emit_mov_mem_reg(&context->code, BINARY_GP_RSP, 8,
                                 BINARY_GP_RAX) ||
        !binary_emit_mov_mem_reg(&context->code, BINARY_GP_RSP, 16,
                                 BINARY_GP_RCX) ||
        !binary_emit_add_reg_imm32(&context->code, BINARY_GP_RCX, 17) ||
        !binary_emit_mov_reg_reg(&context->code, BINARY_GP_RDX,
                                 BINARY_GP_RCX) ||
        !binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RCX, 1) ||
        !binary_emit_sub_rsp_imm32(
            &context->code, BINARY_WIN64_SHADOW_SPACE_SIZE + 8) ||
        !binary_emit_call_placeholder(&context->code, &displacement_offset) ||
        !binary_call_relocation_table_add(&context->call_relocations,
                                          allocator_name, displacement_offset) ||
        !binary_emit_add_rsp_imm32(
            &context->code, BINARY_WIN64_SHADOW_SPACE_SIZE + 8) ||
        !binary_emit_mov_reg_mem(&context->code, BINARY_GP_RCX, BINARY_GP_RSP,
                                 16) ||
        !binary_emit_mov_reg_mem(&context->code, BINARY_GP_RDX, BINARY_GP_RSP,
                                 8) ||
        !binary_emit_mov_reg_mem(&context->code, BINARY_GP_R10, BINARY_GP_RSP,
                                 0) ||
        !binary_emit_add_rsp_imm32(&context->code, 24) ||
        !binary_emit_lea_reg_mem(&context->code, BINARY_GP_R8, BINARY_GP_RAX,
                                 16) ||
        !binary_emit_mov_mem_reg(&context->code, BINARY_GP_RAX, 0,
                                 BINARY_GP_R8) ||
        !binary_emit_mov_mem_reg(&context->code, BINARY_GP_RAX, 8,
                                 BINARY_GP_RCX) ||
        !binary_emit_mov_reg_mem(&context->code, BINARY_GP_R9, BINARY_GP_RDX,
                                 8) ||
        !binary_emit_mov_reg_mem(&context->code, BINARY_GP_R11, BINARY_GP_RDX,
                                 0) ||
        !binary_emit_test_reg_reg(&context->code, BINARY_GP_R9) ||
        !binary_emit_je_placeholder(&context->code, &loop_fixup) ||
        !binary_label_fixup_table_add(&context->label_fixups, left_done_label,
                                      loop_fixup) ||
        !binary_label_table_define(&context->labels, left_loop_label,
                                   context->code.size) ||
        !binary_emit_movzx_reg_mem8(&context->code, BINARY_GP_RCX,
                                    BINARY_GP_R11, 0) ||
        !binary_emit_mov_mem_reg8(&context->code, BINARY_GP_R8, 0,
                                  BINARY_GP_RCX) ||
        !binary_emit_add_reg_imm32(&context->code, BINARY_GP_R11, 1) ||
        !binary_emit_add_reg_imm32(&context->code, BINARY_GP_R8, 1) ||
        !binary_emit_sub_reg_imm32(&context->code, BINARY_GP_R9, 1) ||
        !binary_emit_test_reg_reg(&context->code, BINARY_GP_R9) ||
        !binary_emit_je_placeholder(&context->code, &loop_fixup) ||
        !binary_label_fixup_table_add(&context->label_fixups, left_done_label,
                                      loop_fixup) ||
        !binary_emit_jmp_placeholder(&context->code, &loop_fixup) ||
        !binary_label_fixup_table_add(&context->label_fixups, left_loop_label,
                                      loop_fixup) ||
        !binary_label_table_define(&context->labels, left_done_label,
                                   context->code.size) ||
        !binary_emit_mov_reg_mem(&context->code, BINARY_GP_R9, BINARY_GP_R10,
                                 8) ||
        !binary_emit_mov_reg_mem(&context->code, BINARY_GP_R11, BINARY_GP_R10,
                                 0) ||
        !binary_emit_test_reg_reg(&context->code, BINARY_GP_R9) ||
        !binary_emit_je_placeholder(&context->code, &loop_fixup) ||
        !binary_label_fixup_table_add(&context->label_fixups, right_done_label,
                                      loop_fixup) ||
        !binary_label_table_define(&context->labels, right_loop_label,
                                   context->code.size) ||
        !binary_emit_movzx_reg_mem8(&context->code, BINARY_GP_RCX,
                                    BINARY_GP_R11, 0) ||
        !binary_emit_mov_mem_reg8(&context->code, BINARY_GP_R8, 0,
                                  BINARY_GP_RCX) ||
        !binary_emit_add_reg_imm32(&context->code, BINARY_GP_R11, 1) ||
        !binary_emit_add_reg_imm32(&context->code, BINARY_GP_R8, 1) ||
        !binary_emit_sub_reg_imm32(&context->code, BINARY_GP_R9, 1) ||
        !binary_emit_test_reg_reg(&context->code, BINARY_GP_R9) ||
        !binary_emit_je_placeholder(&context->code, &loop_fixup) ||
        !binary_label_fixup_table_add(&context->label_fixups, right_done_label,
                                      loop_fixup) ||
        !binary_emit_jmp_placeholder(&context->code, &loop_fixup) ||
        !binary_label_fixup_table_add(&context->label_fixups, right_loop_label,
                                      loop_fixup) ||
        !binary_label_table_define(&context->labels, right_done_label,
                                   context->code.size) ||
        !binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RCX, 0) ||
        !binary_emit_mov_mem_reg8(&context->code, BINARY_GP_R8, 0,
                                  BINARY_GP_RCX) ||
        !code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX)) {
      if (!generator->has_error) {
        code_generator_set_error(
            generator,
            "Out of memory while emitting string concat in function '%s'",
            context->function_name);
      }
      free(left_done_label);
      free(left_loop_label);
      free(right_done_label);
      free(right_loop_label);
      return 0;
    }

    free(left_done_label);
    free(left_loop_label);
    free(right_done_label);
    free(right_loop_label);
    return 1;
  }

  if (instruction->is_float) {
    int fbits = (instruction->float_bits == 32) ? 32 : 64;
    int arith_ok = 0;
    int reg_move_ok = 0;
    op = instruction->text;
    /* Bring both operands in at the operation's precision so single- and
     * double-precision expressions stay in their own domain. */
    if (!code_generator_binary_emit_float_operand_to_xmm_bits(
            generator, context, &instruction->rhs, BINARY_XMM1, fbits) ||
        !code_generator_binary_emit_float_operand_to_xmm_bits(
            generator, context, &instruction->lhs, BINARY_XMM0, fbits)) {
      goto emit_failure;
    }

    if (strcmp(op, "+") == 0) {
      arith_ok = (fbits == 32)
                     ? binary_emit_addss_xmm_xmm(&context->code, BINARY_XMM0,
                                                 BINARY_XMM1)
                     : binary_emit_addsd_xmm_xmm(&context->code, BINARY_XMM0,
                                                 BINARY_XMM1);
      reg_move_ok =
          (fbits == 32)
              ? binary_emit_movd_reg_xmm(&context->code, BINARY_GP_RAX,
                                         BINARY_XMM0)
              : binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                         BINARY_XMM0);
      if (!arith_ok || !reg_move_ok) {
        goto emit_failure;
      }
    } else if (strcmp(op, "-") == 0) {
      arith_ok = (fbits == 32)
                     ? binary_emit_subss_xmm_xmm(&context->code, BINARY_XMM0,
                                                 BINARY_XMM1)
                     : binary_emit_subsd_xmm_xmm(&context->code, BINARY_XMM0,
                                                 BINARY_XMM1);
      reg_move_ok =
          (fbits == 32)
              ? binary_emit_movd_reg_xmm(&context->code, BINARY_GP_RAX,
                                         BINARY_XMM0)
              : binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                         BINARY_XMM0);
      if (!arith_ok || !reg_move_ok) {
        goto emit_failure;
      }
    } else if (strcmp(op, "*") == 0) {
      arith_ok = (fbits == 32)
                     ? binary_emit_mulss_xmm_xmm(&context->code, BINARY_XMM0,
                                                 BINARY_XMM1)
                     : binary_emit_mulsd_xmm_xmm(&context->code, BINARY_XMM0,
                                                 BINARY_XMM1);
      reg_move_ok =
          (fbits == 32)
              ? binary_emit_movd_reg_xmm(&context->code, BINARY_GP_RAX,
                                         BINARY_XMM0)
              : binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                         BINARY_XMM0);
      if (!arith_ok || !reg_move_ok) {
        goto emit_failure;
      }
    } else if (strcmp(op, "/") == 0) {
      arith_ok = (fbits == 32)
                     ? binary_emit_divss_xmm_xmm(&context->code, BINARY_XMM0,
                                                 BINARY_XMM1)
                     : binary_emit_divsd_xmm_xmm(&context->code, BINARY_XMM0,
                                                 BINARY_XMM1);
      reg_move_ok =
          (fbits == 32)
              ? binary_emit_movd_reg_xmm(&context->code, BINARY_GP_RAX,
                                         BINARY_XMM0)
              : binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                         BINARY_XMM0);
      if (!arith_ok || !reg_move_ok) {
        goto emit_failure;
      }
    } else if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
               strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
               strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
      int cmp_ok = (fbits == 32)
                       ? binary_emit_ucomiss_xmm_xmm(&context->code,
                                                     BINARY_XMM0, BINARY_XMM1)
                       : binary_emit_ucomisd_xmm_xmm(&context->code,
                                                     BINARY_XMM0, BINARY_XMM1);
      if (!cmp_ok) {
        goto emit_failure;
      }

      if (strcmp(op, "==") == 0) {
        if (!binary_emit_setcc_reg8(&context->code, 0x94, BINARY_GP_RAX) ||
            !binary_emit_setcc_reg8(&context->code, 0x9B, BINARY_GP_RCX) ||
            !binary_emit_alu_reg8_reg8(&context->code, 0x20, BINARY_GP_RAX,
                                       BINARY_GP_RCX) ||
            !binary_emit_movzx_eax_al(&context->code)) {
          goto emit_failure;
        }
      } else if (strcmp(op, "!=") == 0) {
        if (!binary_emit_setcc_reg8(&context->code, 0x95, BINARY_GP_RAX) ||
            !binary_emit_setcc_reg8(&context->code, 0x9A, BINARY_GP_RCX) ||
            !binary_emit_alu_reg8_reg8(&context->code, 0x08, BINARY_GP_RAX,
                                       BINARY_GP_RCX) ||
            !binary_emit_movzx_eax_al(&context->code)) {
          goto emit_failure;
        }
      } else if (strcmp(op, "<") == 0) {
        condition_opcode = 0x92;
        is_compare = 1;
      } else if (strcmp(op, "<=") == 0) {
        condition_opcode = 0x96;
        is_compare = 1;
      } else if (strcmp(op, ">") == 0) {
        condition_opcode = 0x97;
        is_compare = 1;
      } else if (strcmp(op, ">=") == 0) {
        condition_opcode = 0x93;
        is_compare = 1;
      }

      if (is_compare &&
          (!binary_emit_setcc_reg8(&context->code, condition_opcode,
                                   BINARY_GP_RAX) ||
           !binary_emit_movzx_eax_al(&context->code))) {
        goto emit_failure;
      }
    } else {
      code_generator_set_error(
          generator,
          "Direct object backend does not yet support float binary operator "
          "'%s' in function '%s'",
          op, context->function_name);
      return 0;
    }

    if (!code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX)) {
      return 0;
    }

    return 1;
  }

  op = instruction->text;
  if (!instruction->is_float &&
      (strcmp(op, "/") == 0 || strcmp(op, "%") == 0) &&
      instruction->rhs.kind == IR_OPERAND_INT) {
    unsigned int shift = 0;
    unsigned long long mask = 0;
    if (code_generator_binary_extract_positive_power_of_two(
            instruction->rhs.int_value, &shift, &mask)) {
      if (!code_generator_binary_emit_operand_load(generator, context,
                                                   &instruction->lhs,
                                                   BINARY_GP_RAX)) {
        return 0;
      }

      if (strcmp(op, "/") == 0) {
        if (shift != 0 &&
            (!binary_emit_mov_reg_reg(&context->code, BINARY_GP_RCX,
                                      BINARY_GP_RAX) ||
             !binary_emit_shift_reg_imm8(&context->code, 7, BINARY_GP_RCX,
                                         63) ||
             !code_generator_binary_emit_and_mask(context, BINARY_GP_RCX,
                                                  mask) ||
             !binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RAX,
                                      BINARY_GP_RCX) ||
             !binary_emit_shift_reg_imm8(&context->code, 7, BINARY_GP_RAX,
                                         (unsigned char)shift))) {
          goto emit_failure;
        }
      } else {
        if (shift == 0) {
          if (!binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RAX, 0)) {
            goto emit_failure;
          }
        } else if (!binary_emit_mov_reg_reg(&context->code, BINARY_GP_R11,
                                            BINARY_GP_RAX) ||
                   !binary_emit_mov_reg_reg(&context->code, BINARY_GP_RCX,
                                            BINARY_GP_RAX) ||
                   !binary_emit_shift_reg_imm8(&context->code, 7,
                                               BINARY_GP_RCX, 63) ||
                   !code_generator_binary_emit_and_mask(context,
                                                        BINARY_GP_RCX, mask) ||
                   !binary_emit_alu_reg_reg(&context->code, 0x01,
                                            BINARY_GP_RAX, BINARY_GP_RCX) ||
                   !binary_emit_shift_reg_imm8(&context->code, 7,
                                               BINARY_GP_RAX,
                                               (unsigned char)shift) ||
                   !binary_emit_shift_reg_imm8(&context->code, 4,
                                               BINARY_GP_RAX,
                                               (unsigned char)shift) ||
                   !binary_emit_alu_reg_reg(&context->code, 0x29,
                                            BINARY_GP_R11, BINARY_GP_RAX) ||
                   !binary_emit_mov_reg_reg(&context->code, BINARY_GP_RAX,
                                            BINARY_GP_R11)) {
          goto emit_failure;
        }
      }

      if (!code_generator_binary_emit_destination_store(generator, context,
                                                        &instruction->dest,
                                                        BINARY_GP_RAX)) {
        return 0;
      }
      return 1;
    }
  }

  if (!instruction->is_float) {
    const IROperand *value_operand = NULL;
    long long immediate = 0;
    int immediate_on_rhs = 0;
    int commutative =
        strcmp(op, "+") == 0 || strcmp(op, "*") == 0 ||
        strcmp(op, "&") == 0 || strcmp(op, "|") == 0 ||
        strcmp(op, "^") == 0 || strcmp(op, "==") == 0 ||
        strcmp(op, "!=") == 0;
    int rhs_immediate_supported =
        commutative || strcmp(op, "-") == 0 || strcmp(op, "<<") == 0 ||
        strcmp(op, ">>") == 0 || strcmp(op, "<") == 0 ||
        strcmp(op, "<=") == 0 || strcmp(op, ">") == 0 ||
        strcmp(op, ">=") == 0;

    if (rhs_immediate_supported && instruction->rhs.kind == IR_OPERAND_INT &&
        code_generator_binary_immediate_fits_signed_32(
            instruction->rhs.int_value)) {
      value_operand = &instruction->lhs;
      immediate = instruction->rhs.int_value;
      immediate_on_rhs = 1;
    } else if (commutative && instruction->lhs.kind == IR_OPERAND_INT &&
               code_generator_binary_immediate_fits_signed_32(
                   instruction->lhs.int_value)) {
      value_operand = &instruction->rhs;
      immediate = instruction->lhs.int_value;
    }

    if (value_operand) {
      int handled = 1;

      /* In-place fast path: when the result symbol is promoted to a register
       * DR, compute `DR = value_operand <op> imm` directly in DR instead of
       * routing through RAX and copying back. This removes the
       * `mov rax,SR; <op> rax,imm; mov DR,rax` triple that dominates loop
       * counters and pointer bumps (i++, scan-=4, ...). Only the arithmetic,
       * bitwise, and shift ops are eligible; the comparison ops below need RAX
       * for setcc/movzx, so they fall through to the RAX path. dest==value
       * (e.g. `i = i + 1`) needs no preparatory move at all. */
      {
        BinaryGpRegister dest_reg = BINARY_GP_RAX;
        int inplace_op =
            (strcmp(op, "+") == 0) ||
            (strcmp(op, "-") == 0 && immediate_on_rhs) ||
            (strcmp(op, "*") == 0) || (strcmp(op, "&") == 0) ||
            (strcmp(op, "|") == 0) || (strcmp(op, "^") == 0) ||
            (((strcmp(op, "<<") == 0) || (strcmp(op, ">>") == 0)) &&
             immediate_on_rhs && immediate >= 0 && immediate < 64);
        if (inplace_op && instruction->dest.kind == IR_OPERAND_SYMBOL &&
            instruction->dest.name &&
            code_generator_binary_symbol_assigned_register(
                generator, context, instruction->dest.name, &dest_reg)) {
          /* Place value_operand into dest_reg. If value_operand is itself a
           * promoted register equal to dest_reg, nothing to do; otherwise load
           * (a register-register mov for promoted operands, a memory/imm load
           * otherwise). emit_operand_load handles all operand kinds and emits
           * nothing when the source already equals the target register. */
          if (!code_generator_binary_emit_operand_load(generator, context,
                                                       value_operand,
                                                       dest_reg)) {
            return 0;
          }

          int ok = 1;
          if (strcmp(op, "+") == 0) {
            ok = binary_emit_add_reg_imm32(&context->code, dest_reg,
                                           (uint32_t)(int32_t)immediate);
          } else if (strcmp(op, "-") == 0) {
            ok = binary_emit_sub_reg_imm32(&context->code, dest_reg,
                                           (uint32_t)(int32_t)immediate);
          } else if (strcmp(op, "*") == 0) {
            ok = binary_emit_imul_reg_reg_imm32(&context->code, dest_reg,
                                                dest_reg,
                                                (uint32_t)(int32_t)immediate);
          } else if (strcmp(op, "&") == 0) {
            ok = binary_emit_and_reg_imm32(&context->code, dest_reg,
                                           (uint32_t)(int32_t)immediate);
          } else if (strcmp(op, "|") == 0) {
            ok = binary_emit_or_reg_imm32(&context->code, dest_reg,
                                          (uint32_t)(int32_t)immediate);
          } else if (strcmp(op, "^") == 0) {
            ok = binary_emit_xor_reg_imm32(&context->code, dest_reg,
                                           (uint32_t)(int32_t)immediate);
          } else { /* << or >> */
            ok = binary_emit_shift_reg_imm8(&context->code,
                                            strcmp(op, "<<") == 0 ? 4 : 7,
                                            dest_reg, (unsigned char)immediate);
          }
          if (!ok) {
            goto emit_failure;
          }
          return 1;
        }
      }

      if (!code_generator_binary_emit_operand_load(generator, context,
                                                   value_operand,
                                                   BINARY_GP_RAX)) {
        return 0;
      }

      if (strcmp(op, "+") == 0) {
        if (!binary_emit_add_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate)) {
          goto emit_failure;
        }
      } else if (strcmp(op, "-") == 0 && immediate_on_rhs) {
        if (!binary_emit_sub_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate)) {
          goto emit_failure;
        }
      } else if (strcmp(op, "*") == 0) {
        if (!binary_emit_imul_reg_reg_imm32(&context->code, BINARY_GP_RAX,
                                            BINARY_GP_RAX,
                                            (uint32_t)(int32_t)immediate)) {
          goto emit_failure;
        }
      } else if (strcmp(op, "&") == 0) {
        if (!binary_emit_and_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate)) {
          goto emit_failure;
        }
      } else if (strcmp(op, "|") == 0) {
        if (!binary_emit_or_reg_imm32(&context->code, BINARY_GP_RAX,
                                      (uint32_t)(int32_t)immediate)) {
          goto emit_failure;
        }
      } else if (strcmp(op, "^") == 0) {
        if (!binary_emit_xor_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate)) {
          goto emit_failure;
        }
      } else if ((strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) &&
                 immediate_on_rhs && immediate >= 0 && immediate < 64) {
        if (!binary_emit_shift_reg_imm8(
                &context->code, strcmp(op, "<<") == 0 ? 4 : 7,
                BINARY_GP_RAX, (unsigned char)immediate)) {
          goto emit_failure;
        }
      } else if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
                 (immediate_on_rhs &&
                  (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
                   strcmp(op, ">") == 0 || strcmp(op, ">=") == 0))) {
        if (strcmp(op, "==") == 0) {
          condition_opcode = 0x94;
        } else if (strcmp(op, "!=") == 0) {
          condition_opcode = 0x95;
        } else if (strcmp(op, "<") == 0) {
          condition_opcode = 0x9C;
        } else if (strcmp(op, "<=") == 0) {
          condition_opcode = 0x9E;
        } else if (strcmp(op, ">") == 0) {
          condition_opcode = 0x9F;
        } else {
          condition_opcode = 0x9D;
        }

        if (!binary_emit_cmp_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate) ||
            !binary_emit_setcc_al(&context->code, condition_opcode) ||
            !binary_emit_movzx_eax_al(&context->code)) {
          goto emit_failure;
        }
      } else {
        handled = 0;
      }

      if (handled) {
        if (!code_generator_binary_emit_destination_store(generator, context,
                                                          &instruction->dest,
                                                          BINARY_GP_RAX)) {
          return 0;
        }
        return 1;
      }
    }
  }

  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_R10) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RAX)) {
    return 0;
  }

  if (strcmp(op, "+") == 0) {
    if (!binary_emit_lea_reg_reg(&context->code, BINARY_GP_RAX, BINARY_GP_RAX,
                                 BINARY_GP_R10) &&
        !binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RAX,
                                 BINARY_GP_R10)) {
      goto emit_failure;
    }
  } else if (strcmp(op, "-") == 0) {
    if (!binary_emit_alu_reg_reg(&context->code, 0x29, BINARY_GP_RAX,
                                 BINARY_GP_R10)) {
      goto emit_failure;
    }
  } else if (strcmp(op, "*") == 0) {
    if (!binary_emit_imul_reg_reg(&context->code, BINARY_GP_RAX,
                                  BINARY_GP_R10)) {
      goto emit_failure;
    }
  } else if (strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {
    if (!binary_emit_cqo(&context->code) ||
        !binary_emit_idiv_reg(&context->code, BINARY_GP_R10)) {
      goto emit_failure;
    }
    if (strcmp(op, "%") == 0 &&
        !binary_emit_mov_reg_reg(&context->code, BINARY_GP_RAX,
                                 BINARY_GP_RDX)) {
      goto emit_failure;
    }
  } else if (strcmp(op, "&") == 0) {
    if (!binary_emit_alu_reg_reg(&context->code, 0x21, BINARY_GP_RAX,
                                 BINARY_GP_R10)) {
      goto emit_failure;
    }
  } else if (strcmp(op, "|") == 0) {
    if (!binary_emit_alu_reg_reg(&context->code, 0x09, BINARY_GP_RAX,
                                 BINARY_GP_R10)) {
      goto emit_failure;
    }
  } else if (strcmp(op, "^") == 0) {
    if (!binary_emit_alu_reg_reg(&context->code, 0x31, BINARY_GP_RAX,
                                 BINARY_GP_R10)) {
      goto emit_failure;
    }
  } else if (strcmp(op, "<<") == 0) {
    if (!binary_emit_mov_reg_reg(&context->code, BINARY_GP_RCX,
                                 BINARY_GP_R10) ||
        !binary_emit_shift_reg_cl(&context->code, 4, BINARY_GP_RAX)) {
      goto emit_failure;
    }
  } else if (strcmp(op, ">>") == 0) {
    if (!binary_emit_mov_reg_reg(&context->code, BINARY_GP_RCX,
                                 BINARY_GP_R10) ||
        !binary_emit_shift_reg_cl(&context->code, 7, BINARY_GP_RAX)) {
      goto emit_failure;
    }
  } else if (strcmp(op, "&&") == 0) {
    if (!binary_emit_alu_reg_reg(&context->code, 0x21, BINARY_GP_RAX,
                                 BINARY_GP_R10) ||
        !binary_emit_test_reg_reg(&context->code, BINARY_GP_RAX) ||
        !binary_emit_setcc_al(&context->code, 0x95) ||
        !binary_emit_movzx_eax_al(&context->code)) {
      goto emit_failure;
    }
  } else if (strcmp(op, "||") == 0) {
    if (!binary_emit_alu_reg_reg(&context->code, 0x09, BINARY_GP_RAX,
                                 BINARY_GP_R10) ||
        !binary_emit_test_reg_reg(&context->code, BINARY_GP_RAX) ||
        !binary_emit_setcc_al(&context->code, 0x95) ||
        !binary_emit_movzx_eax_al(&context->code)) {
      goto emit_failure;
    }
  } else if (strcmp(op, "==") == 0) {
    condition_opcode = 0x94;
    is_compare = 1;
  } else if (strcmp(op, "!=") == 0) {
    condition_opcode = 0x95;
    is_compare = 1;
  } else if (strcmp(op, "<") == 0) {
    condition_opcode = 0x9C;
    is_compare = 1;
  } else if (strcmp(op, "<=") == 0) {
    condition_opcode = 0x9E;
    is_compare = 1;
  } else if (strcmp(op, ">") == 0) {
    condition_opcode = 0x9F;
    is_compare = 1;
  } else if (strcmp(op, ">=") == 0) {
    condition_opcode = 0x9D;
    is_compare = 1;
  } else {
    code_generator_set_error(generator,
                             "Direct object backend does not yet support IR "
                             "binary operator '%s' in function '%s'",
                             op, context->function_name);
    return 0;
  }

  if (is_compare &&
      (!binary_emit_cmp_reg_reg(&context->code, BINARY_GP_RAX,
                                BINARY_GP_R10) ||
       !binary_emit_setcc_al(&context->code, condition_opcode) ||
       !binary_emit_movzx_eax_al(&context->code))) {
    goto emit_failure;
  }

  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }

  return 1;

emit_failure:
  code_generator_set_error(
      generator,
      "Out of memory while emitting IR binary operator in function '%s'",
      context->function_name);
  return 0;
}

static int code_generator_binary_emit_unary(CodeGenerator *generator,
                                            BinaryFunctionContext *context,
                                            const IRInstruction *instruction) {
  const char *op = NULL;

  if (!generator || !context || !instruction || !instruction->text) {
    return 0;
  }

  if (instruction->is_float) {
    int fbits = (instruction->float_bits == 32) ? 32 : 64;
    op = instruction->text;
    if (!code_generator_binary_emit_float_operand_to_xmm_bits(
            generator, context, &instruction->lhs, BINARY_XMM0, fbits)) {
      goto emit_failure;
    }

    if (strcmp(op, "-") == 0) {
      /* Negate as 0 - x at the operand precision. */
      int neg_ok =
          binary_emit_pxor_xmm_xmm(&context->code, BINARY_XMM1, BINARY_XMM1) &&
          (fbits == 32
               ? binary_emit_subss_xmm_xmm(&context->code, BINARY_XMM1,
                                           BINARY_XMM0)
               : binary_emit_subsd_xmm_xmm(&context->code, BINARY_XMM1,
                                           BINARY_XMM0)) &&
          (fbits == 32
               ? binary_emit_movd_reg_xmm(&context->code, BINARY_GP_RAX,
                                          BINARY_XMM1)
               : binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                          BINARY_XMM1));
      if (!neg_ok) {
        goto emit_failure;
      }
    } else if (strcmp(op, "+") == 0) {
      int mv_ok = (fbits == 32)
                      ? binary_emit_movd_reg_xmm(&context->code, BINARY_GP_RAX,
                                                 BINARY_XMM0)
                      : binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                                 BINARY_XMM0);
      if (!mv_ok) {
        goto emit_failure;
      }
    } else {
      code_generator_set_error(
          generator,
          "Direct object backend does not yet support float unary operator "
          "'%s' in function '%s'",
          op, context->function_name);
      return 0;
    }

    if (!code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX)) {
      return 0;
    }

    return 1;
  }

  op = instruction->text;
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RAX)) {
    return 0;
  }

  if (strcmp(op, "-") == 0) {
    if (!binary_emit_neg_reg(&context->code, BINARY_GP_RAX)) {
      goto emit_failure;
    }
  } else if (strcmp(op, "!") == 0) {
    if (!binary_emit_test_reg_reg(&context->code, BINARY_GP_RAX) ||
        !binary_emit_setcc_al(&context->code, 0x94) ||
        !binary_emit_movzx_eax_al(&context->code)) {
      goto emit_failure;
    }
  } else if (strcmp(op, "~") == 0) {
    if (!binary_emit_not_reg(&context->code, BINARY_GP_RAX)) {
      goto emit_failure;
    }
  } else if (strcmp(op, "+") == 0) {
    /* No-op */
  } else {
    code_generator_set_error(generator,
                             "Direct object backend does not yet support IR "
                             "unary operator '%s' in function '%s'",
                             op, context->function_name);
    return 0;
  }

  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }

  return 1;

emit_failure:
  code_generator_set_error(
      generator,
      "Out of memory while emitting IR unary operator in function '%s'",
      context->function_name);
  return 0;
}

/* ---- SSE2 / scalar byte encoders local to the word-count vectorizer ----
 * Kept self-contained so the (verified) instruction encodings live next to
 * the algorithm that depends on them. xmm regs used are 0..6 and GPRs are
 * rax/rcx/rdx + r8..r11, so REX.R/B handling is explicit where r8..r15 or the
 * SSE high regs would need it (here they do not, but the helpers stay
 * general). All return 1 on success, 0 on OOM. */

/* 66 0F <op> /r  — SSE2 packed op, xmm dst, xmm src (regs 0..7). */
static int wcs_sse_66(BinaryCodeBuffer *b, unsigned char op,
                      int dst, int src) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, op) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((dst & 7) << 3) | (src & 7)));
}

/* F3 0F 6F /r — movdqu xmm, [rcx]  (mod=00, rm=001=rcx, no disp). */
static int wcs_movdqu_xmm_rcx(BinaryCodeBuffer *b, int xmm) {
  return binary_code_buffer_append_u8(b, 0xF3) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x6F) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0x00 | ((xmm & 7) << 3) | 0x01));
}

/* 66 0F 6E /r — movd xmm, r32 (here src is always a low GPR 0..2,8,9). */
static int wcs_movd_xmm_reg(BinaryCodeBuffer *b, int xmm, int gpr) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_emit_rex(b, 0, xmm >> 3, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x6E) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((xmm & 7) << 3) | (gpr & 7)));
}

/* 66 0F 70 /r ib — pshufd xmm, xmm, imm8. */
static int wcs_pshufd(BinaryCodeBuffer *b, int dst, int src,
                      unsigned char imm) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x70) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((dst & 7) << 3) | (src & 7))) &&
         binary_code_buffer_append_u8(b, imm);
}

/* 66 0F D7 /r — pmovmskb r32, xmm. dst is a GPR (0..15), src xmm 0..7. */
static int wcs_pmovmskb(BinaryCodeBuffer *b, int gpr, int xmm) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_emit_rex(b, 0, gpr >> 3, 0, xmm >> 3) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0xD7) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((gpr & 7) << 3) | (xmm & 7)));
}

/* F3 0F B8 /r — popcnt r32, r32. */
static int wcs_popcnt(BinaryCodeBuffer *b, int dst, int src) {
  return binary_code_buffer_append_u8(b, 0xF3) &&
         binary_emit_rex(b, 0, dst >> 3, 0, src >> 3) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0xB8) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((dst & 7) << 3) | (src & 7)));
}

/* 0F B6 /r — movzx r32, byte [rcx]. */
static int wcs_movzx_reg_byte_rcx(BinaryCodeBuffer *b, int gpr) {
  return binary_emit_rex(b, 0, gpr >> 3, 0, 0) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0xB6) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0x00 | ((gpr & 7) << 3) | 0x01));
}

/* C1 /4 ib (shl) or /5 ib (shr) — r32, imm8. */
static int wcs_shift_reg_imm(BinaryCodeBuffer *b, int gpr, int is_shr,
                             unsigned char imm) {
  return binary_emit_rex(b, 0, 0, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0xC1) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((is_shr ? 5 : 4) << 3) |
                                (gpr & 7))) &&
         binary_code_buffer_append_u8(b, imm);
}

/* 09 /r — or r32, r32  (dst |= src). */
static int wcs_or_reg_reg(BinaryCodeBuffer *b, int dst, int src) {
  return binary_emit_rex(b, 0, src >> 3, 0, dst >> 3) &&
         binary_code_buffer_append_u8(b, 0x09) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((src & 7) << 3) | (dst & 7)));
}

/* F7 /2 — not r32. */
static int wcs_not_reg(BinaryCodeBuffer *b, int gpr) {
  return binary_emit_rex(b, 0, 0, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0xF7) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | (2 << 3) | (gpr & 7)));
}

/* 23 /r — and r32, r32 (dst &= src). */
static int wcs_and_reg_reg(BinaryCodeBuffer *b, int dst, int src) {
  return binary_emit_rex(b, 0, dst >> 3, 0, src >> 3) &&
         binary_code_buffer_append_u8(b, 0x23) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((dst & 7) << 3) | (src & 7)));
}

/* 89 /r — mov r32, r32 (dst = src). */
static int wcs_mov_reg_reg32(BinaryCodeBuffer *b, int dst, int src) {
  return binary_emit_rex(b, 0, src >> 3, 0, dst >> 3) &&
         binary_code_buffer_append_u8(b, 0x89) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((src & 7) << 3) | (dst & 7)));
}

/* B8+r id — mov r32, imm32. */
static int wcs_mov_reg_imm32(BinaryCodeBuffer *b, int gpr, uint32_t imm) {
  return binary_emit_rex(b, 0, 0, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xB8 + (gpr & 7))) &&
         binary_code_buffer_append_u32(b, imm);
}

/* 48 01 /r — add r64, r64 (dst += src). */
static int wcs_add_reg_reg64(BinaryCodeBuffer *b, int dst, int src) {
  return binary_emit_rex(b, 1, src >> 3, 0, dst >> 3) &&
         binary_code_buffer_append_u8(b, 0x01) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((src & 7) << 3) | (dst & 7)));
}

/* 48 83 /0 ib — add r64, imm8 ; or /5 for sub. */
static int wcs_addsub_reg_imm8(BinaryCodeBuffer *b, int gpr, int is_sub,
                               unsigned char imm) {
  return binary_emit_rex(b, 1, 0, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x83) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((is_sub ? 5 : 0) << 3) |
                                (gpr & 7))) &&
         binary_code_buffer_append_u8(b, imm);
}

/* 48 83 /7 ib — cmp r64, imm8 (sign-extended). */
static int wcs_cmp_reg_imm8(BinaryCodeBuffer *b, int gpr, unsigned char imm) {
  return binary_emit_rex(b, 1, 0, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x83) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | (7 << 3) | (gpr & 7))) &&
         binary_code_buffer_append_u8(b, imm);
}

/* 81 /7 id — cmp r32, imm32 (used for the tail byte compares). */
static int wcs_cmp_reg_imm32(BinaryCodeBuffer *b, int gpr, uint32_t imm) {
  return binary_emit_rex(b, 0, 0, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x81) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | (7 << 3) | (gpr & 7))) &&
         binary_code_buffer_append_u32(b, imm);
}

/* 85 /r — test r32, r32. */
static int wcs_test_reg_reg32(BinaryCodeBuffer *b, int gpr) {
  return binary_emit_rex(b, 0, gpr >> 3, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x85) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((gpr & 7) << 3) | (gpr & 7)));
}

/* 31 /r — xor r32, r32 (zero a reg via self-xor). */
static int wcs_xor_self32(BinaryCodeBuffer *b, int gpr) {
  return binary_emit_rex(b, 0, gpr >> 3, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x31) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((gpr & 7) << 3) | (gpr & 7)));
}

/* Emit a near jcc/jmp with a 32-bit rel placeholder; record the offset of the
 * displacement field so it can be patched once the target is known. cc==0
 * means an unconditional jmp (E9), otherwise 0F 8x. */
static int wcs_jcc(BinaryCodeBuffer *b, unsigned char cc, size_t *disp_off) {
  if (cc == 0) {
    if (!binary_code_buffer_append_u8(b, 0xE9)) return 0;
  } else {
    if (!binary_code_buffer_append_u8(b, 0x0F) ||
        !binary_code_buffer_append_u8(b, cc))
      return 0;
  }
  *disp_off = b->size;
  return binary_code_buffer_append_u32(b, 0);
}

/* Patch a rel32 placeholder so it jumps to the current end of the buffer. */
static int wcs_patch_here(BinaryCodeBuffer *b, size_t disp_off) {
  long long delta =
      (long long)b->size - (long long)(disp_off + 4);
  if (delta < INT32_MIN || delta > INT32_MAX) return 0;
  int32_t d = (int32_t)delta;
  memcpy(b->data + disp_off, &d, 4);
  return 1;
}

/* Patch a rel32 placeholder to jump backward to a recorded target offset. */
static int wcs_patch_to(BinaryCodeBuffer *b, size_t disp_off,
                        size_t target) {
  long long delta = (long long)target - (long long)(disp_off + 4);
  if (delta < INT32_MIN || delta > INT32_MAX) return 0;
  int32_t d = (int32_t)delta;
  memcpy(b->data + disp_off, &d, 4);
  return 1;
}

/* 39 /r — cmp r32, r32 */
static int wcs_cmp_reg_reg32(BinaryCodeBuffer *b, int dst, int src) {
  return binary_emit_rex(b, 0, src >> 3, 0, dst >> 3) &&
         binary_code_buffer_append_u8(b, 0x39) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((src & 7) << 3) | (dst & 7)));
}

/* movdqu xmm, [gpr+0] */
static int wcs_movdqu_xmm_mem(BinaryCodeBuffer *b, int xmm, int gpr) {
  return binary_code_buffer_append_u8(b, 0xF3) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x6F) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0x00 | ((xmm & 7) << 3) | (gpr & 7)));
}

/* 66 0F 7E /r — movd r32, xmm */
static int wcs_movd_reg_xmm(BinaryCodeBuffer *b, int gpr, int xmm) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_emit_rex(b, 0, xmm >> 3, 0, gpr >> 3) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x7E) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((xmm & 7) << 3) | (gpr & 7)));
}

/* 29 /r — sub r32, r32 (dst -= src) */
static int wcs_sub_reg_reg32(BinaryCodeBuffer *b, int dst, int src) {
  return binary_emit_rex(b, 0, src >> 3, 0, dst >> 3) &&
         binary_code_buffer_append_u8(b, 0x29) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | ((src & 7) << 3) | (dst & 7)));
}

/* Lower IR_OP_SIMD_SUM_I32: add sum of base[0..len-1] int32s into dest. */
static int code_generator_binary_emit_simd_sum_i32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t loop_top = 0;
  size_t j_done = 0;
  size_t j_vec = 0;
  size_t j_scalar = 0;

  if (!generator || !context || !instruction) {
    return 0;
  }
  b = &context->code;

  /* rax=sum, rcx=base, edx=i, r8d=len */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_RAX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_R8)) {
    return 0;
  }
  if (!wcs_xor_self32(b, BINARY_GP_RDX)) return 0;

  loop_top = b->size;
  if (!wcs_cmp_reg_reg32(b, BINARY_GP_RDX, BINARY_GP_R8)) return 0;
  if (!wcs_jcc(b, 0x83 /* jae */, &j_done)) return 0;

  /* len - i >= 4 ? */
  if (!wcs_mov_reg_reg32(b, BINARY_GP_R9, BINARY_GP_R8) ||
      !wcs_sub_reg_reg32(b, BINARY_GP_R9, BINARY_GP_RDX) ||
      !wcs_cmp_reg_imm32(b, BINARY_GP_R9, 4) ||
      !wcs_jcc(b, 0x83 /* jae */, &j_vec)) {
    return 0;
  }
  if (!wcs_jcc(b, 0, &j_scalar)) return 0;

  if (!wcs_patch_here(b, j_vec)) return 0;
  if (!wcs_movdqu_xmm_mem(b, 0, BINARY_GP_RCX)) return 0;
  for (int lane = 0; lane < 4; lane++) {
    if (lane > 0 && !wcs_pshufd(b, 0, 0, (unsigned char)lane)) {
      return 0;
    }
    if (!wcs_movd_reg_xmm(b, BINARY_GP_R10, 0) ||
        !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R10, BINARY_GP_R10) ||
        !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R10)) {
      return 0;
    }
  }
  if (!wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 16) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 4)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_scalar)) return 0;
  if (!binary_emit_mov_reg_mem32(b, BINARY_GP_R10, BINARY_GP_RCX, 0) ||
      !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R10, BINARY_GP_R10) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R10) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 4) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 0, 1)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, loop_top)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, j_done)) return 0;

  return code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX);
}

/* 66 0F F4 /r — pmuludq xmm, xmm */
static int wcs_pmuludq(BinaryCodeBuffer *b, int dst, int src) {
  return wcs_sse_66(b, 0xF4, dst, src);
}

/* 66 0F FE /r — paddd xmm, xmm */
static int wcs_paddd(BinaryCodeBuffer *b, int dst, int src) {
  return wcs_sse_66(b, 0xFE, dst, src);
}

/* 66 0F 73 /2 ib — psrlq xmm, imm8 */
static int wcs_psrlq_imm(BinaryCodeBuffer *b, int xmm, unsigned char imm) {
  return binary_code_buffer_append_u8(b, 0x66) &&
         binary_code_buffer_append_u8(b, 0x0F) &&
         binary_code_buffer_append_u8(b, 0x73) &&
         binary_code_buffer_append_u8(
             b, (unsigned char)(0xC0 | (2 << 3) | (xmm & 7))) &&
         binary_code_buffer_append_u8(b, imm);
}

/* Fixed 32x32 int32 matrix multiply — SSE2 4-column kernel, N=32. */
static int code_generator_binary_emit_simd_matmul_n32(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  BinaryCodeBuffer *b = NULL;
  size_t row_loop = 0;
  size_t row_done = 0;
  size_t col_loop = 0;
  size_t col_done = 0;
  size_t k_loop = 0;
  size_t k_done = 0;

  if (!generator || !context || !instruction) {
    return 0;
  }
  b = &context->code;

  /* r12=a, r13=b, r14=c */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_R12) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_R13) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_R14)) {
    return 0;
  }

  if (!wcs_xor_self32(b, BINARY_GP_R15)) return 0; /* row */

  row_loop = b->size;
  if (!wcs_cmp_reg_imm32(b, BINARY_GP_R15, 32) ||
      !wcs_jcc(b, 0x83, &row_done)) return 0;

  if (!wcs_mov_reg_reg32(b, BINARY_GP_R11, BINARY_GP_R15) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R11, 0, 5) ||
      !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R11, BINARY_GP_R11)) return 0;

  if (!wcs_xor_self32(b, BINARY_GP_RBX)) return 0; /* col */

  col_loop = b->size;
  if (!wcs_cmp_reg_imm32(b, BINARY_GP_RBX, 32) ||
      !wcs_jcc(b, 0x83, &col_done)) return 0;

  if (!wcs_sse_66(b, 0xEF, 3, 3)) return 0; /* pxor xmm3, xmm3 */

  if (!wcs_xor_self32(b, BINARY_GP_R8)) return 0; /* k */
  if (!binary_emit_mov_reg_reg(&context->code, BINARY_GP_R9, BINARY_GP_R11)) return 0;
  if (!wcs_mov_reg_reg32(b, BINARY_GP_R10, BINARY_GP_RBX) ||
      !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R10, BINARY_GP_R10)) return 0;

  k_loop = b->size;
  if (!wcs_cmp_reg_imm32(b, BINARY_GP_R8, 32) ||
      !wcs_jcc(b, 0x83, &k_done)) return 0;

  /* av = a[r12 + r9*4] — r11 holds row_base until clobbered for av load */
  if (!binary_emit_lea_reg_base_index_scale_disp(
          b, BINARY_GP_RAX, BINARY_GP_R12, BINARY_GP_R9, 4, 0) ||
      !binary_emit_mov_reg_mem32(b, BINARY_GP_R11, BINARY_GP_RAX, 0) ||
      !wcs_movd_xmm_reg(b, 4, BINARY_GP_R11) ||
      !wcs_pshufd(b, 4, 4, 0x00)) {
    return 0;
  }

  /* b row at r13 + r10*4, 16 bytes */
  if (!binary_emit_lea_reg_base_index_scale_disp(
          b, BINARY_GP_RAX, BINARY_GP_R13, BINARY_GP_R10, 4, 0) ||
      !wcs_movdqu_xmm_mem(b, 2, BINARY_GP_RAX)) {
    return 0;
  }

  if (!wcs_sse_66(b, 0x6F, 0, 2) || /* movdqa xmm0, xmm2 */
      !wcs_sse_66(b, 0x6F, 1, 2) || /* movdqa xmm1, xmm2 */
      !wcs_psrlq_imm(b, 1, 32) ||
      !wcs_pmuludq(b, 1, 4) ||
      !wcs_pmuludq(b, 0, 4) ||
      !wcs_paddd(b, 3, 0) ||
      !wcs_paddd(b, 3, 1)) return 0;

  if (!wcs_addsub_reg_imm8(b, BINARY_GP_R9, 0, 1) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_R10, 0, 32) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_R8, 0, 1)) {
    return 0;
  }
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, k_loop)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, k_done)) return 0;

  /* store xmm3 lanes to c[row*32+col..+3] */
  if (!wcs_mov_reg_reg32(b, BINARY_GP_R11, BINARY_GP_R15) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R11, 0, 5) ||
      !binary_emit_movsxd_reg_reg32(b, BINARY_GP_R11, BINARY_GP_R11) ||
      !binary_emit_lea_reg_base_index_scale_disp(
          b, BINARY_GP_RAX, BINARY_GP_R14, BINARY_GP_R11, 4, 0) ||
      !binary_emit_lea_reg_base_index_scale_disp(
          b, BINARY_GP_RAX, BINARY_GP_RAX, BINARY_GP_RBX, 4, 0)) {
    return 0;
  }
  for (int lane = 0; lane < 4; lane++) {
    if (lane > 0 && !wcs_pshufd(b, 3, 3, (unsigned char)lane)) return 0;
    if (!wcs_movd_reg_xmm(b, BINARY_GP_R11, 3) ||
        !binary_emit_mov_mem_reg32(b, BINARY_GP_RAX, (int)(lane * 4),
                                   BINARY_GP_R11)) {
      return 0;
    }
  }

  if (!wcs_addsub_reg_imm8(b, BINARY_GP_RBX, 0, 4)) return 0;
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, col_loop)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, col_done)) return 0;
  if (!wcs_addsub_reg_imm8(b, BINARY_GP_R15, 0, 1)) return 0;
  {
    size_t j_back = 0;
    if (!wcs_jcc(b, 0, &j_back) || !wcs_patch_to(b, j_back, row_loop)) {
      return 0;
    }
  }

  if (!wcs_patch_here(b, row_done)) return 0;
  return 1;
}

/* Lower IR_OP_COUNT_WORD_STARTS: count maximal non-whitespace runs in
 * buf[0..len-1] (whitespace = 0x20/0x09/0x0A/0x0D), 16 bytes/iter via SSE2,
 * plus a scalar tail. Result is ADDED to count's prior value (the recognizer
 * only fires when the source set count=0 before the loop). Same algorithm as
 * the text backend's code_generator_emit_ir_count_word_starts. */
static int code_generator_binary_emit_count_word_starts(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  if (!generator || !context || !instruction ||
      instruction->dest.kind != IR_OPERAND_SYMBOL ||
      instruction->lhs.kind != IR_OPERAND_SYMBOL ||
      instruction->rhs.kind != IR_OPERAND_SYMBOL) {
    code_generator_set_error(generator, "Malformed count_word_starts in '%s'",
                             context ? context->function_name : "?");
    return 0;
  }

  BinaryCodeBuffer *b = &context->code;

  /* rcx <- buf ; rdx <- len ; rax <- count ; r8d <- 0 (carry). */
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_RDX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_RAX)) {
    return 0;
  }
  if (!wcs_xor_self32(b, BINARY_GP_R8)) return 0;

  /* Broadcast 0x20/0x09/0x0A/0x0D into xmm1..xmm4 (via r9d + movd + pshufd). */
  static const struct { unsigned int pat; int xmm; } CONSTS[4] = {
      {0x20202020u, 1}, {0x09090909u, 2}, {0x0A0A0A0Au, 3}, {0x0D0D0D0Du, 4}};
  for (int i = 0; i < 4; i++) {
    if (!wcs_mov_reg_imm32(b, BINARY_GP_R9, CONSTS[i].pat) ||
        !wcs_movd_xmm_reg(b, CONSTS[i].xmm, BINARY_GP_R9) ||
        !wcs_pshufd(b, CONSTS[i].xmm, CONSTS[i].xmm, 0x00)) {
      return 0;
    }
  }

  /* ---- vector loop: while (rdx >= 16) ---- */
  size_t loop_top = b->size;
  /* cmp rdx, 16 ; jb tail */
  if (!wcs_cmp_reg_imm8(b, BINARY_GP_RDX, 16)) return 0;
  size_t j_to_tail;
  if (!wcs_jcc(b, 0x82 /* jb */, &j_to_tail)) return 0;

  /* xmm0 = chunk ; xmm5 = copy ; xmm0 = (==sp) | (==tab) | (==lf) | (==cr) */
  if (!wcs_movdqu_xmm_rcx(b, 0) ||
      !wcs_sse_66(b, 0x6F, 5, 0) ||           /* movdqa xmm5, xmm0 */
      !wcs_sse_66(b, 0x74, 0, 1) ||           /* pcmpeqb xmm0, xmm1 */
      !wcs_sse_66(b, 0x6F, 6, 5) ||           /* movdqa xmm6, xmm5 */
      !wcs_sse_66(b, 0x74, 6, 2) ||           /* pcmpeqb xmm6, xmm2 */
      !wcs_sse_66(b, 0xEB, 0, 6) ||           /* por xmm0, xmm6 */
      !wcs_sse_66(b, 0x6F, 6, 5) ||
      !wcs_sse_66(b, 0x74, 6, 3) ||           /* pcmpeqb xmm6, xmm3 */
      !wcs_sse_66(b, 0xEB, 0, 6) ||
      !wcs_sse_66(b, 0x6F, 6, 5) ||
      !wcs_sse_66(b, 0x74, 6, 4) ||           /* pcmpeqb xmm6, xmm4 */
      !wcs_sse_66(b, 0xEB, 0, 6)) {
    return 0;
  }
  /* r9d = ws bitmask ; r10d = nw = ~ws & 0xFFFF */
  if (!wcs_pmovmskb(b, BINARY_GP_R9, 0) ||
      !wcs_mov_reg_reg32(b, BINARY_GP_R10, BINARY_GP_R9) ||
      !wcs_not_reg(b, BINARY_GP_R10) ||
      !binary_emit_and_reg_imm32(b, BINARY_GP_R10, 0xFFFF)) {
    return 0;
  }
  /* r11d = prev = (nw<<1) | carry */
  if (!wcs_mov_reg_reg32(b, BINARY_GP_R11, BINARY_GP_R10) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R11, 0, 1) ||
      !wcs_or_reg_reg(b, BINARY_GP_R11, BINARY_GP_R8)) {
    return 0;
  }
  /* new carry r8d = nw bit15 */
  if (!wcs_mov_reg_reg32(b, BINARY_GP_R8, BINARY_GP_R10) ||
      !wcs_shift_reg_imm(b, BINARY_GP_R8, 1, 15) ||
      !binary_emit_and_reg_imm32(b, BINARY_GP_R8, 1)) {
    return 0;
  }
  /* starts = nw & ~prev ; count += popcount(starts) */
  if (!wcs_not_reg(b, BINARY_GP_R11) ||
      !wcs_and_reg_reg(b, BINARY_GP_R11, BINARY_GP_R10) ||
      !binary_emit_and_reg_imm32(b, BINARY_GP_R11, 0xFFFF) ||
      !wcs_popcnt(b, BINARY_GP_R11, BINARY_GP_R11) ||
      !wcs_add_reg_reg64(b, BINARY_GP_RAX, BINARY_GP_R11)) {
    return 0;
  }
  /* rcx += 16 ; rdx -= 16 ; jmp loop_top */
  if (!wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 16) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 1, 16)) {
    return 0;
  }
  size_t j_back;
  if (!wcs_jcc(b, 0, &j_back)) return 0;
  if (!wcs_patch_to(b, j_back, loop_top)) {
    code_generator_set_error(generator, "wcs back-jump out of range");
    return 0;
  }

  /* ---- scalar tail ---- */
  if (!wcs_patch_here(b, j_to_tail)) return 0; /* jb -> here */
  /* if (rdx == 0) goto done */
  if (!wcs_cmp_reg_imm8(b, BINARY_GP_RDX, 0)) return 0;
  size_t j_done_early;
  if (!wcs_jcc(b, 0x84 /* je */, &j_done_early)) return 0;

  size_t tail_top = b->size;
  /* r9d = (uint8)[rcx] */
  if (!wcs_movzx_reg_byte_rcx(b, BINARY_GP_R9)) return 0;
  /* four "is whitespace?" tests -> collect je placeholders */
  size_t j_ws[4];
  static const unsigned int WS[4] = {32u, 9u, 10u, 13u};
  for (int i = 0; i < 4; i++) {
    if (!wcs_cmp_reg_imm32(b, BINARY_GP_R9, WS[i]) ||
        !wcs_jcc(b, 0x84 /* je */, &j_ws[i])) {
      return 0;
    }
  }
  /* non-whitespace: if (carry==0) count++ ; carry=1 */
  if (!wcs_test_reg_reg32(b, BINARY_GP_R8)) return 0;
  size_t j_skip_inc;
  if (!wcs_jcc(b, 0x85 /* jne */, &j_skip_inc)) return 0;
  if (!wcs_addsub_reg_imm8(b, BINARY_GP_RAX, 0, 1)) return 0; /* rax += 1 */
  if (!wcs_patch_here(b, j_skip_inc)) return 0;
  if (!wcs_mov_reg_imm32(b, BINARY_GP_R8, 1)) return 0; /* carry = 1 */
  size_t j_after_class;
  if (!wcs_jcc(b, 0, &j_after_class)) return 0;
  /* whitespace target: carry = 0 */
  for (int i = 0; i < 4; i++) {
    if (!wcs_patch_here(b, j_ws[i])) return 0;
  }
  if (!wcs_xor_self32(b, BINARY_GP_R8)) return 0; /* carry = 0 */
  if (!wcs_patch_here(b, j_after_class)) return 0;
  /* rcx++ ; rdx-- ; if (rdx != 0) goto tail_top */
  if (!wcs_addsub_reg_imm8(b, BINARY_GP_RCX, 0, 1) ||
      !wcs_addsub_reg_imm8(b, BINARY_GP_RDX, 1, 1)) {
    return 0;
  }
  if (!wcs_cmp_reg_imm8(b, BINARY_GP_RDX, 0)) return 0;
  size_t j_tail_back;
  if (!wcs_jcc(b, 0x85 /* jne */, &j_tail_back)) return 0;
  if (!wcs_patch_to(b, j_tail_back, tail_top)) {
    code_generator_set_error(generator, "wcs tail-jump out of range");
    return 0;
  }

  if (!wcs_patch_here(b, j_done_early)) return 0;

  /* count = rax */
  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }
  return 1;
}

static int code_generator_binary_emit_instruction(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  if (!generator || !context || !instruction) {
    return 0;
  }

  switch (instruction->op) {
  case IR_OP_NOP:
    return 1;

  case IR_OP_LABEL:
    if (!instruction->text || instruction->text[0] == '\0') {
      code_generator_set_error(generator, "Malformed IR label in function '%s'",
                               context->function_name);
      return 0;
    }
    if (!binary_label_table_define(&context->labels, instruction->text,
                                   context->code.size)) {
      code_generator_set_error(generator,
                               "Duplicate or invalid IR label '%s' in "
                               "function '%s'",
                               instruction->text, context->function_name);
      return 0;
    }
    return 1;

  case IR_OP_JUMP: {
    size_t displacement_offset = 0;
    if (!instruction->text || instruction->text[0] == '\0') {
      code_generator_set_error(generator,
                               "Malformed IR jump target in function '%s'",
                               context->function_name);
      return 0;
    }
    if (!binary_emit_jmp_placeholder(&context->code, &displacement_offset) ||
        !binary_label_fixup_table_add(&context->label_fixups, instruction->text,
                                      displacement_offset)) {
      code_generator_set_error(generator,
                               "Out of memory while emitting IR jump");
      return 0;
    }
    return 1;
  }

  case IR_OP_BRANCH_ZERO: {
    size_t displacement_offset = 0;
    if (!instruction->text || instruction->text[0] == '\0') {
      code_generator_set_error(
          generator, "Malformed IR branch target in function '%s'",
          context->function_name);
      return 0;
    }
    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->lhs,
                                                 BINARY_GP_RAX) ||
        !binary_emit_test_reg_reg(&context->code, BINARY_GP_RAX) ||
        !binary_emit_je_placeholder(&context->code, &displacement_offset) ||
        !binary_label_fixup_table_add(&context->label_fixups, instruction->text,
                                      displacement_offset)) {
      if (!generator->has_error) {
        code_generator_set_error(generator,
                                 "Out of memory while emitting branch_zero");
      }
      return 0;
    }
    return 1;
  }

  case IR_OP_BRANCH_EQ: {
    size_t displacement_offset = 0;
    if (!instruction->text || instruction->text[0] == '\0') {
      code_generator_set_error(
          generator, "Malformed IR branch target in function '%s'",
          context->function_name);
      return 0;
    }
    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->rhs,
                                                 BINARY_GP_R10) ||
        !code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->lhs,
                                                 BINARY_GP_RAX) ||
        !binary_emit_cmp_reg_reg(&context->code, BINARY_GP_RAX,
                                 BINARY_GP_R10) ||
        !binary_emit_je_placeholder(&context->code, &displacement_offset) ||
        !binary_label_fixup_table_add(&context->label_fixups, instruction->text,
                                      displacement_offset)) {
      if (!generator->has_error) {
        code_generator_set_error(generator,
                                 "Out of memory while emitting branch_eq");
      }
      return 0;
    }
    return 1;
  }

  case IR_OP_ASSIGN: {
    const char *alias_target = NULL;
    if (instruction->dest.kind == IR_OPERAND_SYMBOL &&
        instruction->dest.name) {
      alias_target = binary_symbol_alias_table_get(&context->symbol_aliases,
                                                   instruction->dest.name);
      if (alias_target && instruction->lhs.kind == IR_OPERAND_SYMBOL &&
          instruction->lhs.name &&
          strcmp(alias_target, instruction->lhs.name) == 0) {
        return 1;
      }
    }
    /* Indirect-return propagation: source is a temp tagged as holding a
     * pointer to a struct returned from an INDIRECT-returning call; dest
     * is a struct symbol. Memcpy from *src_ptr into &dest. */
    if (instruction->lhs.kind == IR_OPERAND_TEMP && instruction->lhs.name &&
        instruction->dest.kind == IR_OPERAND_SYMBOL &&
        instruction->dest.name) {
      size_t bytes = binary_indirect_temp_get(context, instruction->lhs.name);
      if (bytes > 0) {
        Symbol *dest_sym = symbol_table_lookup(generator->symbol_table,
                                               instruction->dest.name);
        if (!dest_sym || !dest_sym->type ||
            code_generator_type_is_aggregate(dest_sym->type)) {
          /* Load src pointer (the temp slot stores the pointer value). */
          int src_offset =
              code_generator_binary_get_temp_offset(context,
                                                    instruction->lhs.name);
          if (src_offset <= 0) {
            code_generator_set_error(
                generator,
                "Cannot resolve temp '%s' for INDIRECT-return assign",
                instruction->lhs.name);
            return 0;
          }
          if (!binary_emit_mov_reg_mem(&context->code, BINARY_GP_RAX,
                                       BINARY_GP_RBP, -src_offset) ||
              !code_generator_binary_emit_struct_destination_address(
                  generator, context, instruction->dest.name, BINARY_GP_RDX) ||
              !code_generator_binary_emit_rep_movsb(generator, context,
                                                    BINARY_GP_RAX,
                                                    BINARY_GP_RDX, bytes)) {
            if (!generator->has_error) {
              code_generator_set_error(
                  generator, "Out of memory copying INDIRECT-return assign");
            }
            return 0;
          }
          return 1;
        }
      }
    }
    /* Fast path: assigning into a promoted destination register. Load the
     * source straight into the destination register instead of routing through
     * RAX and copying back (`scan = prev` between two promoted registers
     * collapses to a single `mov DR, SR`, or nothing when DR==SR). Restricted
     * to non-float assigns; float assigns may need a precision conversion that
     * the RAX path handles below. */
    if (!instruction->is_float && instruction->dest.kind == IR_OPERAND_SYMBOL &&
        instruction->dest.name) {
      BinaryGpRegister assign_dest_reg = BINARY_GP_RAX;
      if (code_generator_binary_symbol_assigned_register(
              generator, context, instruction->dest.name, &assign_dest_reg)) {
        if (!code_generator_binary_emit_operand_load(generator, context,
                                                     &instruction->lhs,
                                                     assign_dest_reg)) {
          if (!generator->has_error) {
            code_generator_set_error(generator,
                                     "Out of memory while emitting assign");
          }
          return 0;
        }
        return 1;
      }
    }

    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->lhs,
                                                 BINARY_GP_RAX)) {
      if (!generator->has_error) {
        code_generator_set_error(generator,
                                 "Out of memory while emitting assign");
      }
      return 0;
    }
    /* Convert when a float value is assigned into a destination of a
     * different float precision (instruction->float_bits = target width,
     * set by ir_lowering from the declared/symbol type). */
    if (instruction->is_float && instruction->float_bits) {
      int value_bits = code_generator_binary_operand_float_bits(
          generator, context, &instruction->lhs);
      if (value_bits &&
          !code_generator_binary_emit_float_reg_convert(
              context, BINARY_GP_RAX, value_bits, instruction->float_bits)) {
        code_generator_set_error(generator,
                                 "Out of memory while converting float assign "
                                 "precision in function '%s'",
                                 context->function_name);
        return 0;
      }
    }
    if (!code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX)) {
      if (!generator->has_error) {
        code_generator_set_error(generator,
                                 "Out of memory while emitting assign");
      }
      return 0;
    }
    return 1;
  }

  case IR_OP_ADDRESS_OF:
    return code_generator_binary_emit_address_of(generator, context,
                                                 instruction);

  case IR_OP_LOAD:
    return code_generator_binary_emit_load(generator, context, instruction);

  case IR_OP_STORE:
    return code_generator_binary_emit_store(generator, context, instruction);

  case IR_OP_BINARY:
    return code_generator_binary_emit_binary(generator, context, instruction);

  case IR_OP_ROTATE_ADD:
    return code_generator_binary_emit_rotate_add(generator, context,
                                                 instruction);

  case IR_OP_UNARY:
    return code_generator_binary_emit_unary(generator, context, instruction);

  case IR_OP_CALL:
    return code_generator_binary_emit_call(generator, context, instruction);

  case IR_OP_CALL_INDIRECT:
    return code_generator_binary_emit_call_indirect(generator, context,
                                                    instruction);

  case IR_OP_NEW:
    return code_generator_binary_emit_new(generator, context, instruction);

  case IR_OP_CAST:
    return code_generator_binary_emit_cast(generator, context, instruction);

  case IR_OP_RETURN: {
    size_t displacement_offset = 0;
    /* INDIRECT return: memcpy the source struct through the hidden out-ptr
     * stored at [rbp - 8], then put that pointer into rax. */
    if (context->returns_indirect &&
        instruction->lhs.kind != IR_OPERAND_NONE) {
      if (!code_generator_binary_emit_indirect_source_address(
              generator, context, &instruction->lhs, BINARY_GP_RAX)) {
        return 0;
      }
      /* dst = qword [rbp - 8]; rep movsb. */
      if (!binary_emit_mov_reg_mem(&context->code, BINARY_GP_RDX,
                                   BINARY_GP_RBP, -8) ||
          !code_generator_binary_emit_rep_movsb(generator, context,
                                                BINARY_GP_RAX, BINARY_GP_RDX,
                                                context->indirect_return_size) ||
          !binary_emit_mov_reg_mem(&context->code, BINARY_GP_RAX,
                                   BINARY_GP_RBP, -8)) {
        code_generator_set_error(generator,
                                 "Out of memory emitting indirect return");
        return 0;
      }
      if (!binary_emit_jmp_placeholder(&context->code, &displacement_offset) ||
          !binary_offset_table_add(&context->return_fixups,
                                   displacement_offset)) {
        code_generator_set_error(
            generator, "Out of memory while emitting function return");
        return 0;
      }
      return 1;
    }
    if (instruction->lhs.kind != IR_OPERAND_NONE &&
        !code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->lhs,
                                                 BINARY_GP_RAX)) {
      return 0;
    }
    /* Convert the returned value to the function's float return precision
     * (instruction->float_bits set by ir_lowering) so the epilogue's
     * RAX->XMM0 transfer carries correctly-rounded bits. */
    if (instruction->lhs.kind != IR_OPERAND_NONE && instruction->is_float &&
        instruction->float_bits) {
      int value_bits = code_generator_binary_operand_float_bits(
          generator, context, &instruction->lhs);
      if (value_bits &&
          !code_generator_binary_emit_float_reg_convert(
              context, BINARY_GP_RAX, value_bits, instruction->float_bits)) {
        code_generator_set_error(generator,
                                 "Out of memory while converting float return "
                                 "precision in function '%s'",
                                 context->function_name);
        return 0;
      }
    }
    if (!binary_emit_jmp_placeholder(&context->code, &displacement_offset) ||
        !binary_offset_table_add(&context->return_fixups, displacement_offset)) {
      code_generator_set_error(generator,
                               "Out of memory while emitting function return");
      return 0;
    }
    return 1;
  }

  case IR_OP_DECLARE_LOCAL:
    if (instruction->dest.kind != IR_OPERAND_SYMBOL ||
        !instruction->dest.name || instruction->dest.name[0] == '\0' ||
        code_generator_binary_get_local_offset(context, instruction->dest.name) <=
            0) {
      code_generator_set_error(generator,
                               "Malformed local declaration in function '%s'",
                               context->function_name);
      return 0;
    }
    return 1;

  case IR_OP_COUNT_WORD_STARTS:
    return code_generator_binary_emit_count_word_starts(generator, context,
                                                        instruction);

  case IR_OP_MEMCPY_INLINE:
    return code_generator_binary_emit_memcpy_inline(generator, context,
                                                    instruction);

  case IR_OP_SIMD_SUM_I32:
    return code_generator_binary_emit_simd_sum_i32(generator, context,
                                                   instruction);

  case IR_OP_SIMD_MATMUL_N32:
    return code_generator_binary_emit_simd_matmul_n32(generator, context,
                                                      instruction);

  default:
    code_generator_set_error(
        generator,
        "Direct object backend does not yet support IR opcode %d in "
        "function '%s'",
        (int)instruction->op, context->function_name);
    return 0;
  }
}

/* Windows reserves the stack lazily behind a single guard page: touching the
 * guard page commits it and moves the guard down by one page. A prologue that
 * lowers rsp by more than a page in one `sub` can step *over* the guard page
 * without ever touching it, so the first write into the new frame faults --
 * exactly the crash seen on functions with very large frames (e.g. main() with
 * hundreds of call sites). Microsoft's ABI requires a stack probe for frames
 * larger than a page: touch each page as rsp descends so the guard moves down
 * one page at a time. We do an unrolled probe (no helper call): for each 4 KiB
 * step, `sub rsp, 4096` then write to [rsp], then handle the remainder. RAX is
 * scratch here (prologue runs before any value is live in it). */
#define BINARY_STACK_PAGE_SIZE 4096

static int binary_emit_frame_allocation(BinaryCodeBuffer *code, int frame_size) {
  if (frame_size <= 0) {
    return 1;
  }

  if (frame_size <= BINARY_STACK_PAGE_SIZE) {
    return binary_emit_sub_rsp_imm32(code, (uint32_t)frame_size);
  }

  int remaining = frame_size;
  while (remaining > BINARY_STACK_PAGE_SIZE) {
    if (!binary_emit_sub_rsp_imm32(code, (uint32_t)BINARY_STACK_PAGE_SIZE)) {
      return 0;
    }
    /* Touch the freshly-stepped page so the guard page is hit in order. */
    if (!binary_emit_mov_mem_reg(code, BINARY_GP_RSP, 0, BINARY_GP_RAX)) {
      return 0;
    }
    remaining -= BINARY_STACK_PAGE_SIZE;
  }
  /* Final (sub-page) remainder; touch it too to commit the last page. */
  if (!binary_emit_sub_rsp_imm32(code, (uint32_t)remaining)) {
    return 0;
  }
  return binary_emit_mov_mem_reg(code, BINARY_GP_RSP, 0, BINARY_GP_RAX);
}

static int code_generator_binary_emit_prologue(CodeGenerator *generator,
                                               BinaryFunctionContext *context,
                                               FunctionDeclaration *function_data) {
  if (!generator || !context || !function_data) {
    return 0;
  }

  if (!binary_emit_push_reg(&context->code, BINARY_GP_RBP) ||
      !binary_emit_mov_reg_reg(&context->code, BINARY_GP_RBP, BINARY_GP_RSP)) {
    code_generator_set_error(generator,
                             "Out of memory while emitting function prologue");
    return 0;
  }

  if (!binary_emit_frame_allocation(&context->code, context->frame_size)) {
    code_generator_set_error(generator,
                             "Out of memory while allocating stack frame");
    return 0;
  }

  for (size_t i = 0; i < context->saved_register_count; i++) {
    if (!binary_emit_mov_mem_reg(&context->code, BINARY_GP_RBP,
                                 -context->saved_register_offsets[i],
                                 context->saved_registers[i])) {
      code_generator_set_error(generator,
                               "Out of memory while saving callee registers");
      return 0;
    }
  }

  /* Hidden return out-pointer (Win64 rcx): stash it at the fixed home slot
   * [rbp - 8] before homing user parameters. User-param homes start one slot
   * higher when an INDIRECT return is in use. */
  if (context->returns_indirect) {
    if (!binary_emit_mov_mem_reg(&context->code, BINARY_GP_RBP, -8,
                                 BINARY_GP_RCX)) {
      code_generator_set_error(generator,
                               "Out of memory homing hidden return ptr");
      return 0;
    }
  }

  for (size_t i = 0; i < function_data->parameter_count; i++) {
    const char *parameter_name = function_data->parameter_names[i];
    int parameter_fbits = code_generator_binary_named_type_float_bits(
        generator, function_data->parameter_types
                       ? function_data->parameter_types[i]
                       : NULL);
    BinaryGpRegister assigned_register = BINARY_GP_RAX;
    int parameter_in_register = code_generator_binary_symbol_assigned_register(
        generator, context, parameter_name, &assigned_register);
    int home_offset =
        code_generator_binary_get_parameter_offset(context, parameter_name);
    if (home_offset <= 0) {
      code_generator_set_error(
          generator,
          "Missing parameter home for '%s' in function '%s'",
          parameter_name ? parameter_name : "<unnamed>",
          context->function_name);
      return 0;
    }

    /* When the function returns INDIRECT, the hidden out-pointer occupies
     * ABI slot 0, so user param i occupies ABI slot i+1. */
    size_t abi_slot = i + (context->returns_indirect ? 1 : 0);

    if (parameter_in_register) {
      int home_ok = 1;
      if (abi_slot < BINARY_WIN64_REGISTER_ARG_COUNT) {
        home_ok = binary_emit_mov_reg_reg(
            &context->code, assigned_register,
            BINARY_WIN64_INT_PARAM_REGISTERS[abi_slot]);
      } else {
        int incoming_stack_offset =
            16 + BINARY_WIN64_SHADOW_SPACE_SIZE +
            (int)((abi_slot - BINARY_WIN64_REGISTER_ARG_COUNT) *
                  BINARY_FUNCTION_STACK_SLOT_SIZE);
        home_ok = binary_emit_mov_reg_mem(&context->code, assigned_register,
                                          BINARY_GP_RBP,
                                          incoming_stack_offset);
      }
      if (!home_ok) {
        code_generator_set_error(
            generator, "Out of memory while homing register parameters");
        return 0;
      }
      continue;
    }

    if (abi_slot < BINARY_WIN64_REGISTER_ARG_COUNT) {
      int home_ok = 1;
      if (parameter_fbits) {
        /* Float params arrive in XMM; copy the bits to GP at the param's
         * precision (movd for float32, movq for float64) before homing. */
        home_ok =
            (parameter_fbits == 32
                 ? binary_emit_movd_reg_xmm(
                       &context->code, BINARY_GP_RAX,
                       BINARY_WIN64_FLOAT_PARAM_REGISTERS[abi_slot])
                 : binary_emit_movq_reg_xmm(
                       &context->code, BINARY_GP_RAX,
                       BINARY_WIN64_FLOAT_PARAM_REGISTERS[abi_slot])) &&
            binary_emit_mov_mem_reg(&context->code, BINARY_GP_RBP,
                                    -home_offset, BINARY_GP_RAX);
      } else {
        home_ok = binary_emit_mov_mem_reg(
            &context->code, BINARY_GP_RBP, -home_offset,
            BINARY_WIN64_INT_PARAM_REGISTERS[abi_slot]);
      }
      if (!home_ok) {
        code_generator_set_error(generator,
                                 "Out of memory while homing parameters");
        return 0;
      }
    } else {
      int incoming_stack_offset =
          16 + BINARY_WIN64_SHADOW_SPACE_SIZE +
          (int)((abi_slot - BINARY_WIN64_REGISTER_ARG_COUNT) *
                BINARY_FUNCTION_STACK_SLOT_SIZE);
      if (!binary_emit_mov_reg_mem(&context->code, BINARY_GP_RAX, BINARY_GP_RBP,
                                   incoming_stack_offset) ||
          !binary_emit_mov_mem_reg(&context->code, BINARY_GP_RBP, -home_offset,
                                   BINARY_GP_RAX)) {
        code_generator_set_error(generator,
                                 "Out of memory while homing parameters");
        return 0;
      }
    }
  }

  return 1;
}

static int code_generator_binary_resolve_fixups(CodeGenerator *generator,
                                                BinaryFunctionContext *context,
                                                size_t return_offset) {
  if (!generator || !context) {
    return 0;
  }

  for (size_t i = 0; i < context->label_fixups.count; i++) {
    BinaryLabelFixup *fixup = &context->label_fixups.items[i];
    BinaryLabelEntry *label =
        binary_label_table_get(&context->labels, fixup->name);
    if (!label) {
      code_generator_set_error(
          generator,
          "Undefined IR label '%s' in direct object function '%s'",
          fixup->name ? fixup->name : "<unnamed>", context->function_name);
      return 0;
    }
    if (!binary_function_context_patch_rel32(
            context, fixup->displacement_offset, label->offset)) {
      code_generator_set_error(
          generator,
          "Branch target out of range while lowering function '%s'",
          context->function_name);
      return 0;
    }
  }

  for (size_t i = 0; i < context->return_fixups.count; i++) {
    if (!binary_function_context_patch_rel32(context,
                                             context->return_fixups.items[i],
                                             return_offset)) {
      code_generator_set_error(
          generator,
          "Return target out of range while lowering function '%s'",
          context->function_name);
      return 0;
    }
  }

  return 1;
}

static int code_generator_binary_is_compare_operator(const char *op) {
  return op &&
         (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
          strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
          strcmp(op, ">") == 0 || strcmp(op, ">=") == 0);
}

static int code_generator_binary_compare_false_jcc(const char *op,
                                                   unsigned char *opcode_out) {
  if (!op || !opcode_out) {
    return 0;
  }

  if (strcmp(op, "==") == 0) {
    *opcode_out = 0x85; /* jne */
    return 1;
  }
  if (strcmp(op, "!=") == 0) {
    *opcode_out = 0x84; /* je */
    return 1;
  }
  if (strcmp(op, "<") == 0) {
    *opcode_out = 0x8D; /* jge */
    return 1;
  }
  if (strcmp(op, "<=") == 0) {
    *opcode_out = 0x8F; /* jg */
    return 1;
  }
  if (strcmp(op, ">") == 0) {
    *opcode_out = 0x8E; /* jle */
    return 1;
  }
  if (strcmp(op, ">=") == 0) {
    *opcode_out = 0x8C; /* jl */
    return 1;
  }

  return 0;
}

static int code_generator_binary_operand_uses_temp(const IROperand *operand,
                                                   const char *name) {
  return operand && operand->kind == IR_OPERAND_TEMP && operand->name && name &&
         strcmp(operand->name, name) == 0;
}

static int code_generator_binary_instruction_temp_use_count(
    const IRInstruction *instruction, const char *name) {
  int count = 0;

  if (!instruction || !name) {
    return 0;
  }

  if (code_generator_binary_operand_uses_temp(&instruction->lhs, name)) {
    count++;
  }
  if (code_generator_binary_operand_uses_temp(&instruction->rhs, name)) {
    count++;
  }
  for (size_t i = 0; i < instruction->argument_count; i++) {
    if (code_generator_binary_operand_uses_temp(&instruction->arguments[i],
                                                name)) {
      count++;
    }
  }

  return count;
}

static int code_generator_binary_function_temp_use_count(
    const IRFunction *function, const char *name) {
  int count = 0;

  if (!function || !name) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    count += code_generator_binary_instruction_temp_use_count(
        &function->instructions[i], name);
  }

  return count;
}

static int code_generator_binary_emit_compare_false_branch(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *compare, const char *target_label) {
  unsigned char branch_opcode = 0;
  size_t displacement_offset = 0;
  BinaryGpRegister lhs_register = BINARY_GP_RAX;
  BinaryGpRegister rhs_register = BINARY_GP_R10;
  int lhs_has_register = 0;
  int rhs_has_register = 0;

  if (!generator || !context || !compare || !target_label ||
      target_label[0] == '\0' ||
      !code_generator_binary_compare_false_jcc(compare->text,
                                               &branch_opcode)) {
    return 0;
  }

  lhs_has_register =
      compare->lhs.kind == IR_OPERAND_SYMBOL &&
      code_generator_binary_symbol_assigned_register(
          generator, context, compare->lhs.name, &lhs_register);
  rhs_has_register =
      compare->rhs.kind == IR_OPERAND_SYMBOL &&
      code_generator_binary_symbol_assigned_register(
          generator, context, compare->rhs.name, &rhs_register);

  if (compare->rhs.kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(
          compare->rhs.int_value)) {
    if ((!lhs_has_register &&
         !code_generator_binary_emit_operand_load(generator, context,
                                                  &compare->lhs,
                                                  BINARY_GP_RAX)) ||
        !binary_emit_cmp_reg_imm32(
            &context->code, lhs_has_register ? lhs_register : BINARY_GP_RAX,
            (uint32_t)(int32_t)compare->rhs.int_value)) {
      return 0;
    }
  } else if ((strcmp(compare->text, "==") == 0 ||
              strcmp(compare->text, "!=") == 0) &&
             compare->lhs.kind == IR_OPERAND_INT &&
             code_generator_binary_immediate_fits_signed_32(
                 compare->lhs.int_value)) {
    if ((!rhs_has_register &&
         !code_generator_binary_emit_operand_load(generator, context,
                                                  &compare->rhs,
                                                  BINARY_GP_RAX)) ||
        !binary_emit_cmp_reg_imm32(
            &context->code, rhs_has_register ? rhs_register : BINARY_GP_RAX,
            (uint32_t)(int32_t)compare->lhs.int_value)) {
      return 0;
    }
  } else {
    if (!rhs_has_register &&
        !code_generator_binary_emit_operand_load(generator, context,
                                                 &compare->rhs,
                                                 BINARY_GP_R10)) {
      return 0;
    }
    if (!lhs_has_register &&
        !code_generator_binary_emit_operand_load(generator, context,
                                                 &compare->lhs,
                                                 BINARY_GP_RAX)) {
      return 0;
    }
    if (!binary_emit_cmp_reg_reg(
            &context->code, lhs_has_register ? lhs_register : BINARY_GP_RAX,
            rhs_has_register ? rhs_register : BINARY_GP_R10)) {
      return 0;
    }
  }

  if (!binary_emit_jcc_placeholder(&context->code, branch_opcode,
                                   &displacement_offset) ||
      !binary_label_fixup_table_add(&context->label_fixups, target_label,
                                    displacement_offset)) {
    code_generator_set_error(generator,
                             "Out of memory while emitting compare branch");
    return 0;
  }

  return 1;
}

static int code_generator_binary_emit_integer_binary_to_rax(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  const char *op = NULL;

  if (!generator || !context || !instruction || instruction->is_float ||
      !instruction->text) {
    return 0;
  }

  op = instruction->text;
  if ((strcmp(op, "/") == 0 || strcmp(op, "%") == 0) &&
      instruction->rhs.kind == IR_OPERAND_INT) {
    unsigned int shift = 0;
    unsigned long long mask = 0;
    if (code_generator_binary_extract_positive_power_of_two(
            instruction->rhs.int_value, &shift, &mask)) {
      if (!code_generator_binary_emit_operand_load(generator, context,
                                                   &instruction->lhs,
                                                   BINARY_GP_RAX)) {
        return 0;
      }

      if (strcmp(op, "/") == 0) {
        if (shift != 0 &&
            (!binary_emit_mov_reg_reg(&context->code, BINARY_GP_RCX,
                                      BINARY_GP_RAX) ||
             !binary_emit_shift_reg_imm8(&context->code, 7, BINARY_GP_RCX,
                                         63) ||
             !code_generator_binary_emit_and_mask(context, BINARY_GP_RCX,
                                                  mask) ||
             !binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RAX,
                                      BINARY_GP_RCX) ||
             !binary_emit_shift_reg_imm8(&context->code, 7, BINARY_GP_RAX,
                                         (unsigned char)shift))) {
          code_generator_set_error(
              generator,
              "Out of memory while emitting integer expression chain in "
              "function '%s'",
              context->function_name);
          return 0;
        }
      } else if (shift == 0) {
        if (!binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RAX, 0)) {
          code_generator_set_error(
              generator,
              "Out of memory while emitting integer expression chain in "
              "function '%s'",
              context->function_name);
          return 0;
        }
      } else if (!binary_emit_mov_reg_reg(&context->code, BINARY_GP_R11,
                                          BINARY_GP_RAX) ||
                 !binary_emit_mov_reg_reg(&context->code, BINARY_GP_RCX,
                                          BINARY_GP_RAX) ||
                 !binary_emit_shift_reg_imm8(&context->code, 7, BINARY_GP_RCX,
                                             63) ||
                 !code_generator_binary_emit_and_mask(context, BINARY_GP_RCX,
                                                      mask) ||
                 !binary_emit_alu_reg_reg(&context->code, 0x01,
                                          BINARY_GP_RAX, BINARY_GP_RCX) ||
                 !binary_emit_shift_reg_imm8(&context->code, 7, BINARY_GP_RAX,
                                             (unsigned char)shift) ||
                 !binary_emit_shift_reg_imm8(&context->code, 4, BINARY_GP_RAX,
                                             (unsigned char)shift) ||
                 !binary_emit_alu_reg_reg(&context->code, 0x29,
                                          BINARY_GP_R11, BINARY_GP_RAX) ||
                 !binary_emit_mov_reg_reg(&context->code, BINARY_GP_RAX,
                                          BINARY_GP_R11)) {
        code_generator_set_error(
            generator,
            "Out of memory while emitting integer expression chain in function "
            "'%s'",
            context->function_name);
        return 0;
      }
      return 1;
    }
  }

  if (instruction->rhs.kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(
          instruction->rhs.int_value) &&
      (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
       strcmp(op, "*") == 0 || strcmp(op, "&") == 0 ||
       strcmp(op, "|") == 0 || strcmp(op, "^") == 0 ||
       ((strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) &&
         instruction->rhs.int_value >= 0 && instruction->rhs.int_value < 64))) {
    long long immediate = instruction->rhs.int_value;
    if (immediate == 0 &&
        (strcmp(op, "*") == 0 || strcmp(op, "&") == 0)) {
      return binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RAX, 0);
    }
    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->lhs,
                                                 BINARY_GP_RAX)) {
      return 0;
    }

    if (strcmp(op, "+") == 0) {
      return binary_emit_add_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "-") == 0) {
      return binary_emit_sub_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "*") == 0) {
      return binary_emit_imul_reg_reg_imm32(&context->code, BINARY_GP_RAX,
                                            BINARY_GP_RAX,
                                            (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "&") == 0) {
      return binary_emit_and_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "|") == 0) {
      return binary_emit_or_reg_imm32(&context->code, BINARY_GP_RAX,
                                      (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "^") == 0) {
      return binary_emit_xor_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    return binary_emit_shift_reg_imm8(
        &context->code, strcmp(op, "<<") == 0 ? 4 : 7, BINARY_GP_RAX,
        (unsigned char)immediate);
  }

  if (instruction->lhs.kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(
          instruction->lhs.int_value) &&
      (strcmp(op, "+") == 0 || strcmp(op, "*") == 0 ||
       strcmp(op, "&") == 0 || strcmp(op, "|") == 0 ||
       strcmp(op, "^") == 0)) {
    long long immediate = instruction->lhs.int_value;
    if (immediate == 0 &&
        (strcmp(op, "*") == 0 || strcmp(op, "&") == 0)) {
      return binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RAX, 0);
    }
    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->rhs,
                                                 BINARY_GP_RAX)) {
      return 0;
    }
    if (strcmp(op, "+") == 0) {
      return binary_emit_add_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "*") == 0) {
      return binary_emit_imul_reg_reg_imm32(&context->code, BINARY_GP_RAX,
                                            BINARY_GP_RAX,
                                            (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "&") == 0) {
      return binary_emit_and_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "|") == 0) {
      return binary_emit_or_reg_imm32(&context->code, BINARY_GP_RAX,
                                      (uint32_t)(int32_t)immediate);
    }
    return binary_emit_xor_reg_imm32(&context->code, BINARY_GP_RAX,
                                     (uint32_t)(int32_t)immediate);
  }

  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_R10) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RAX)) {
    return 0;
  }

  if (strcmp(op, "+") == 0) {
    return binary_emit_lea_reg_reg(&context->code, BINARY_GP_RAX,
                                   BINARY_GP_RAX, BINARY_GP_R10) ||
           binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "-") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x29, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "*") == 0) {
    return binary_emit_imul_reg_reg(&context->code, BINARY_GP_RAX,
                                    BINARY_GP_R10);
  }
  if (strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {
    if (!binary_emit_cqo(&context->code) ||
        !binary_emit_idiv_reg(&context->code, BINARY_GP_R10)) {
      return 0;
    }
    if (strcmp(op, "%") == 0) {
      return binary_emit_mov_reg_reg(&context->code, BINARY_GP_RAX,
                                     BINARY_GP_RDX);
    }
    return 1;
  }
  if (strcmp(op, "&") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x21, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "|") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x09, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "^") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x31, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) {
    return binary_emit_mov_reg_reg(&context->code, BINARY_GP_RCX,
                                   BINARY_GP_R10) &&
           binary_emit_shift_reg_cl(&context->code,
                                    strcmp(op, "<<") == 0 ? 4 : 7,
                                    BINARY_GP_RAX);
  }

  return 0;
}

static int code_generator_binary_emit_compare_false_branch_from_rax(
    CodeGenerator *generator, BinaryFunctionContext *context, const char *op,
    const IROperand *rhs, const char *target_label) {
  unsigned char branch_opcode = 0;
  size_t displacement_offset = 0;

  if (!generator || !context || !op || !rhs || !target_label ||
      !code_generator_binary_compare_false_jcc(op, &branch_opcode)) {
    return 0;
  }

  if (rhs->kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(rhs->int_value)) {
    if (!binary_emit_cmp_reg_imm32(&context->code, BINARY_GP_RAX,
                                   (uint32_t)(int32_t)rhs->int_value)) {
      return 0;
    }
  } else if (!code_generator_binary_emit_operand_load(generator, context, rhs,
                                                      BINARY_GP_R10) ||
             !binary_emit_cmp_reg_reg(&context->code, BINARY_GP_RAX,
                                      BINARY_GP_R10)) {
    return 0;
  }

  if (!binary_emit_jcc_placeholder(&context->code, branch_opcode,
                                   &displacement_offset) ||
      !binary_label_fixup_table_add(&context->label_fixups, target_label,
                                    displacement_offset)) {
    code_generator_set_error(
        generator,
        "Out of memory while emitting chained compare branch in function '%s'",
        context->function_name);
    return 0;
  }

  return 1;
}

static int code_generator_binary_try_emit_compare_branch_zero(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *compare = NULL;
  const IRInstruction *branch = NULL;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 1 >= function->instruction_count) {
    return 0;
  }

  compare = &function->instructions[instruction_index];
  branch = &function->instructions[instruction_index + 1];
  if (!compare || compare->op != IR_OP_BINARY || compare->is_float ||
      !code_generator_binary_is_compare_operator(compare->text) ||
      compare->dest.kind != IR_OPERAND_TEMP || !compare->dest.name ||
      branch->op != IR_OP_BRANCH_ZERO ||
      !code_generator_binary_operand_uses_temp(&branch->lhs,
                                               compare->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    compare->dest.name) != 1) {
    return 0;
  }

  if (!code_generator_binary_emit_compare_false_branch(generator, context,
                                                       compare,
                                                       branch->text)) {
    return 0;
  }

  *consumed_out = 2;
  return 1;
}

static int code_generator_binary_chain_producer_supported(const char *op) {
  return op &&
         (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
          strcmp(op, "*") == 0 || strcmp(op, "/") == 0 ||
          strcmp(op, "%") == 0 || strcmp(op, "&") == 0 ||
          strcmp(op, "|") == 0 || strcmp(op, "^") == 0 ||
          strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0);
}

static int code_generator_binary_try_emit_binary_compare_branch_chain(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *producer = NULL;
  const IRInstruction *compare = NULL;
  const IRInstruction *branch = NULL;
  const IROperand *other_operand = NULL;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 2 >= function->instruction_count) {
    return 0;
  }

  producer = &function->instructions[instruction_index];
  compare = &function->instructions[instruction_index + 1];
  branch = &function->instructions[instruction_index + 2];

  if (!producer || producer->op != IR_OP_BINARY || producer->is_float ||
      !code_generator_binary_chain_producer_supported(producer->text) ||
      producer->dest.kind != IR_OPERAND_TEMP || !producer->dest.name ||
      !compare || compare->op != IR_OP_BINARY || compare->is_float ||
      !code_generator_binary_is_compare_operator(compare->text) ||
      compare->dest.kind != IR_OPERAND_TEMP || !compare->dest.name ||
      !branch || branch->op != IR_OP_BRANCH_ZERO ||
      !code_generator_binary_operand_uses_temp(&branch->lhs,
                                               compare->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    producer->dest.name) != 1 ||
      code_generator_binary_function_temp_use_count(function,
                                                    compare->dest.name) != 1) {
    return 0;
  }

  if (code_generator_binary_operand_uses_temp(&compare->lhs,
                                              producer->dest.name)) {
    other_operand = &compare->rhs;
  } else if ((strcmp(compare->text, "==") == 0 ||
              strcmp(compare->text, "!=") == 0) &&
             code_generator_binary_operand_uses_temp(&compare->rhs,
                                                     producer->dest.name)) {
    other_operand = &compare->lhs;
  } else {
    return 0;
  }

  if (!code_generator_binary_emit_integer_binary_to_rax(generator, context,
                                                        producer)) {
    if (!generator->has_error) {
      code_generator_set_error(
          generator,
          "Failed to emit chained integer expression in function '%s'",
          context->function_name);
    }
    return 0;
  }

  if (!code_generator_binary_emit_compare_false_branch_from_rax(
          generator, context, compare->text, other_operand, branch->text)) {
    return 0;
  }

  *consumed_out = 3;
  return 1;
}

static int code_generator_binary_emit_address_add_to_rax(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *address) {
  if (!generator || !context || !address || address->op != IR_OP_BINARY ||
      address->is_float || !address->text ||
      strcmp(address->text, "+") != 0) {
    return 0;
  }

  if (address->rhs.kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(address->rhs.int_value)) {
    return code_generator_binary_emit_operand_load(generator, context,
                                                   &address->lhs,
                                                   BINARY_GP_RAX) &&
           binary_emit_add_reg_imm32(
               &context->code, BINARY_GP_RAX,
               (uint32_t)(int32_t)address->rhs.int_value);
  }
  if (address->lhs.kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(address->lhs.int_value)) {
    return code_generator_binary_emit_operand_load(generator, context,
                                                   &address->rhs,
                                                   BINARY_GP_RAX) &&
           binary_emit_add_reg_imm32(
               &context->code, BINARY_GP_RAX,
               (uint32_t)(int32_t)address->lhs.int_value);
  }

  return code_generator_binary_emit_operand_load(generator, context,
                                                 &address->rhs,
                                                 BINARY_GP_R10) &&
         code_generator_binary_emit_operand_load(generator, context,
                                                 &address->lhs,
                                                 BINARY_GP_RAX) &&
         (binary_emit_lea_reg_reg(&context->code, BINARY_GP_RAX,
                                  BINARY_GP_RAX, BINARY_GP_R10) ||
          binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RAX,
                                  BINARY_GP_R10));
}

static int code_generator_binary_try_emit_address_add_load(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *address = NULL;
  const IRInstruction *load = NULL;
  int size = 0;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 1 >= function->instruction_count) {
    return 0;
  }

  address = &function->instructions[instruction_index];
  load = &function->instructions[instruction_index + 1];
  if (!address || address->op != IR_OP_BINARY || address->is_float ||
      !address->text || strcmp(address->text, "+") != 0 ||
      address->dest.kind != IR_OPERAND_TEMP || !address->dest.name ||
      !load || load->op != IR_OP_LOAD ||
      !code_generator_binary_operand_uses_temp(&load->lhs,
                                               address->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    address->dest.name) != 1) {
    return 0;
  }

  size = code_generator_binary_get_access_size(generator, context, &load->rhs);
  if (size <= 0) {
    return 0;
  }

  if (!code_generator_binary_emit_address_add_to_rax(generator, context,
                                                     address)) {
    return 0;
  }

  if (!code_generator_binary_emit_load_from_address(generator, context,
                                                    BINARY_GP_RAX, size,
                                                    BINARY_GP_RAX)) {
    return 0;
  }
  if (size == 4 && !load->is_float &&
      code_generator_binary_load_needs_sign_extend(generator, context,
                                                   &load->dest, size) &&
      !binary_emit_movsxd_rax_eax(&context->code)) {
    if (!generator->has_error) {
      code_generator_set_error(
          generator,
          "Out of memory while emitting fused address load in function '%s'",
          context->function_name);
    }
    return 0;
  }
  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &load->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }

  *consumed_out = 2;
  return 1;
}

static int code_generator_binary_try_emit_address_add_store(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *address = NULL;
  const IRInstruction *store = NULL;
  int size = 0;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 1 >= function->instruction_count) {
    return 0;
  }

  address = &function->instructions[instruction_index];
  store = &function->instructions[instruction_index + 1];
  if (!address || address->op != IR_OP_BINARY || address->is_float ||
      !address->text || strcmp(address->text, "+") != 0 ||
      address->dest.kind != IR_OPERAND_TEMP || !address->dest.name ||
      !store || store->op != IR_OP_STORE ||
      !code_generator_binary_operand_uses_temp(&store->dest,
                                               address->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    address->dest.name) != 1) {
    return 0;
  }

  size = code_generator_binary_get_access_size(generator, context, &store->rhs);
  if (size <= 0) {
    return 0;
  }

  if (!code_generator_binary_emit_operand_load(generator, context, &store->lhs,
                                               BINARY_GP_RCX)) {
    return 0;
  }

  if (store->is_float && store->float_bits) {
    int value_bits = code_generator_binary_operand_float_bits(
        generator, context, &store->lhs);
    if (value_bits &&
        !code_generator_binary_emit_float_reg_convert(
            context, BINARY_GP_RCX, value_bits, store->float_bits)) {
      code_generator_set_error(generator,
                               "Out of memory while converting float store "
                               "precision in function '%s'",
                               context->function_name);
      return 0;
    }
  }

  if (!code_generator_binary_emit_address_add_to_rax(generator, context,
                                                     address) ||
      !code_generator_binary_emit_store_to_address(generator, context,
                                                   BINARY_GP_RAX, size,
                                                   BINARY_GP_RCX)) {
    return 0;
  }

  *consumed_out = 2;
  return 1;
}

static int code_generator_binary_try_match_scaled_address(
    const IRFunction *function, size_t instruction_index,
    const IRInstruction **address_out, const IROperand **base_out,
    const IROperand **index_out, int *scale_out) {
  const IRInstruction *shift = NULL;
  const IRInstruction *address = NULL;
  int scale = 0;

  if (!function || !address_out || !base_out || !index_out || !scale_out ||
      instruction_index + 1 >= function->instruction_count) {
    return 0;
  }

  shift = &function->instructions[instruction_index];
  address = &function->instructions[instruction_index + 1];
  if (!shift || shift->op != IR_OP_BINARY || shift->is_float ||
      !shift->text || strcmp(shift->text, "<<") != 0 ||
      shift->dest.kind != IR_OPERAND_TEMP || !shift->dest.name ||
      shift->lhs.kind == IR_OPERAND_INT ||
      shift->rhs.kind != IR_OPERAND_INT || shift->rhs.int_value < 0 ||
      shift->rhs.int_value > 3 ||
      code_generator_binary_function_temp_use_count(function,
                                                    shift->dest.name) != 1 ||
      !address || address->op != IR_OP_BINARY || address->is_float ||
      !address->text || strcmp(address->text, "+") != 0 ||
      address->dest.kind != IR_OPERAND_TEMP || !address->dest.name) {
    return 0;
  }

  scale = 1 << (int)shift->rhs.int_value;
  if (code_generator_binary_operand_uses_temp(&address->lhs,
                                              shift->dest.name)) {
    if (address->rhs.kind == IR_OPERAND_INT) {
      return 0;
    }
    *base_out = &address->rhs;
  } else if (code_generator_binary_operand_uses_temp(&address->rhs,
                                                     shift->dest.name)) {
    if (address->lhs.kind == IR_OPERAND_INT) {
      return 0;
    }
    *base_out = &address->lhs;
  } else {
    return 0;
  }

  *address_out = address;
  *index_out = &shift->lhs;
  *scale_out = scale;
  return 1;
}

static int code_generator_binary_emit_scaled_address_to_rax(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *base, const IROperand *index, int scale) {
  if (!generator || !context || !base || !index) {
    return 0;
  }

  return code_generator_binary_emit_operand_load(generator, context, index,
                                                 BINARY_GP_R10) &&
         code_generator_binary_emit_operand_load(generator, context, base,
                                                 BINARY_GP_RAX) &&
         binary_emit_lea_reg_base_index_scale_disp(
             &context->code, BINARY_GP_RAX, BINARY_GP_RAX, BINARY_GP_R10,
             scale, 0);
}

static int code_generator_binary_try_emit_scaled_address_load(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *address = NULL;
  const IRInstruction *load = NULL;
  const IROperand *base = NULL;
  const IROperand *index = NULL;
  int scale = 0;
  int size = 0;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 2 >= function->instruction_count ||
      !code_generator_binary_try_match_scaled_address(
          function, instruction_index, &address, &base, &index, &scale)) {
    return 0;
  }

  load = &function->instructions[instruction_index + 2];
  if (!load || load->op != IR_OP_LOAD ||
      !code_generator_binary_operand_uses_temp(&load->lhs,
                                               address->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    address->dest.name) != 1) {
    return 0;
  }

  size = code_generator_binary_get_access_size(generator, context, &load->rhs);
  if (size <= 0) {
    return 0;
  }

  if (!code_generator_binary_emit_scaled_address_to_rax(
          generator, context, base, index, scale) ||
      !code_generator_binary_emit_load_from_address(generator, context,
                                                    BINARY_GP_RAX, size,
                                                    BINARY_GP_RAX)) {
    return 0;
  }
  if (size == 4 && !load->is_float &&
      code_generator_binary_load_needs_sign_extend(generator, context,
                                                   &load->dest, size) &&
      !binary_emit_movsxd_rax_eax(&context->code)) {
    if (!generator->has_error) {
      code_generator_set_error(
          generator,
          "Out of memory while emitting fused scaled address load in function "
          "'%s'",
          context->function_name);
    }
    return 0;
  }
  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &load->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }

  *consumed_out = 3;
  return 1;
}

static int code_generator_binary_try_emit_scaled_address_store(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *address = NULL;
  const IRInstruction *store = NULL;
  const IROperand *base = NULL;
  const IROperand *index = NULL;
  int scale = 0;
  int size = 0;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 2 >= function->instruction_count ||
      !code_generator_binary_try_match_scaled_address(
          function, instruction_index, &address, &base, &index, &scale)) {
    return 0;
  }

  store = &function->instructions[instruction_index + 2];
  if (!store || store->op != IR_OP_STORE ||
      !code_generator_binary_operand_uses_temp(&store->dest,
                                               address->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    address->dest.name) != 1) {
    return 0;
  }

  size = code_generator_binary_get_access_size(generator, context, &store->rhs);
  if (size <= 0) {
    return 0;
  }

  if (!code_generator_binary_emit_operand_load(generator, context, &store->lhs,
                                               BINARY_GP_RCX)) {
    return 0;
  }

  if (store->is_float && store->float_bits) {
    int value_bits = code_generator_binary_operand_float_bits(
        generator, context, &store->lhs);
    if (value_bits &&
        !code_generator_binary_emit_float_reg_convert(
            context, BINARY_GP_RCX, value_bits, store->float_bits)) {
      code_generator_set_error(generator,
                               "Out of memory while converting float store "
                               "precision in function '%s'",
                               context->function_name);
      return 0;
    }
  }

  if (!code_generator_binary_emit_scaled_address_to_rax(
          generator, context, base, index, scale) ||
      !code_generator_binary_emit_store_to_address(generator, context,
                                                   BINARY_GP_RAX, size,
                                                   BINARY_GP_RCX)) {
    return 0;
  }

  *consumed_out = 3;
  return 1;
}

static int code_generator_binary_try_emit_binary_cast_chain(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *producer = NULL;
  const IRInstruction *cast = NULL;
  Type *target_type = NULL;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 1 >= function->instruction_count) {
    return 0;
  }

  producer = &function->instructions[instruction_index];
  cast = &function->instructions[instruction_index + 1];
  if (!producer || producer->op != IR_OP_BINARY || producer->is_float ||
      !code_generator_binary_chain_producer_supported(producer->text) ||
      producer->dest.kind != IR_OPERAND_TEMP || !producer->dest.name ||
      !cast || cast->op != IR_OP_CAST || cast->is_float || !cast->text ||
      !code_generator_binary_operand_uses_temp(&cast->lhs,
                                               producer->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    producer->dest.name) != 1) {
    return 0;
  }

  target_type = generator->type_checker
                    ? type_checker_get_type_by_name(generator->type_checker,
                                                    cast->text)
                    : NULL;
  if (target_type &&
      (code_generator_is_floating_point_type(target_type) ||
       (target_type->kind != TYPE_POINTER &&
        target_type->kind != TYPE_FUNCTION_POINTER &&
        target_type->size != 8))) {
    return 0;
  }

  if (!code_generator_binary_emit_integer_binary_to_rax(generator, context,
                                                        producer) ||
      !code_generator_binary_emit_destination_store(generator, context,
                                                    &cast->dest,
                                                    BINARY_GP_RAX)) {
    if (!generator->has_error) {
      code_generator_set_error(
          generator,
          "Failed to emit chained integer cast in function '%s'",
          context->function_name);
    }
    return 0;
  }

  *consumed_out = 2;
  return 1;
}

static int code_generator_binary_operator_is_commutative(const char *op) {
  return op &&
         (strcmp(op, "+") == 0 || strcmp(op, "*") == 0 ||
          strcmp(op, "&") == 0 || strcmp(op, "|") == 0 ||
          strcmp(op, "^") == 0);
}

static int code_generator_binary_emit_rax_binary_rhs(
    CodeGenerator *generator, BinaryFunctionContext *context, const char *op,
    const IROperand *rhs) {
  if (!generator || !context || !op || !rhs) {
    return 0;
  }

  if (rhs->kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(rhs->int_value)) {
    long long immediate = rhs->int_value;
    if (immediate == 0 &&
        (strcmp(op, "*") == 0 || strcmp(op, "&") == 0)) {
      return binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RAX, 0);
    }
    if (strcmp(op, "+") == 0) {
      return binary_emit_add_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "-") == 0) {
      return binary_emit_sub_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "*") == 0) {
      return binary_emit_imul_reg_reg_imm32(&context->code, BINARY_GP_RAX,
                                            BINARY_GP_RAX,
                                            (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "&") == 0) {
      return binary_emit_and_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "|") == 0) {
      return binary_emit_or_reg_imm32(&context->code, BINARY_GP_RAX,
                                      (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "^") == 0) {
      return binary_emit_xor_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if ((strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) &&
        immediate >= 0 && immediate < 64) {
      return binary_emit_shift_reg_imm8(
          &context->code, strcmp(op, "<<") == 0 ? 4 : 7, BINARY_GP_RAX,
          (unsigned char)immediate);
    }
  }

  if (!code_generator_binary_emit_operand_load(generator, context, rhs,
                                               BINARY_GP_R10)) {
    return 0;
  }

  if (strcmp(op, "+") == 0) {
    return binary_emit_lea_reg_reg(&context->code, BINARY_GP_RAX,
                                   BINARY_GP_RAX, BINARY_GP_R10) ||
           binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "-") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x29, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "*") == 0) {
    return binary_emit_imul_reg_reg(&context->code, BINARY_GP_RAX,
                                    BINARY_GP_R10);
  }
  if (strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {
    if (!binary_emit_cqo(&context->code) ||
        !binary_emit_idiv_reg(&context->code, BINARY_GP_R10)) {
      return 0;
    }
    if (strcmp(op, "%") == 0) {
      return binary_emit_mov_reg_reg(&context->code, BINARY_GP_RAX,
                                     BINARY_GP_RDX);
    }
    return 1;
  }
  if (strcmp(op, "&") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x21, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "|") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x09, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "^") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x31, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) {
    return binary_emit_mov_reg_reg(&context->code, BINARY_GP_RCX,
                                   BINARY_GP_R10) &&
           binary_emit_shift_reg_cl(&context->code,
                                    strcmp(op, "<<") == 0 ? 4 : 7,
                                    BINARY_GP_RAX);
  }

  return 0;
}

static int code_generator_binary_try_emit_binary_expression_chain(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *producer = NULL;
  const IRInstruction *consumer = NULL;
  const IROperand *rhs = NULL;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 1 >= function->instruction_count) {
    return 0;
  }

  producer = &function->instructions[instruction_index];
  consumer = &function->instructions[instruction_index + 1];
  if (!producer || producer->op != IR_OP_BINARY || producer->is_float ||
      !code_generator_binary_chain_producer_supported(producer->text) ||
      producer->dest.kind != IR_OPERAND_TEMP || !producer->dest.name ||
      !consumer || consumer->op != IR_OP_BINARY || consumer->is_float ||
      code_generator_binary_is_compare_operator(consumer->text) ||
      !code_generator_binary_chain_producer_supported(consumer->text) ||
      code_generator_binary_function_temp_use_count(function,
                                                    producer->dest.name) != 1) {
    return 0;
  }

  if (code_generator_binary_operand_uses_temp(&consumer->lhs,
                                              producer->dest.name)) {
    rhs = &consumer->rhs;
  } else if (code_generator_binary_operator_is_commutative(consumer->text) &&
             code_generator_binary_operand_uses_temp(&consumer->rhs,
                                                     producer->dest.name)) {
    rhs = &consumer->lhs;
  } else {
    return 0;
  }

  if (!code_generator_binary_emit_integer_binary_to_rax(generator, context,
                                                        producer) ||
      !code_generator_binary_emit_rax_binary_rhs(generator, context,
                                                 consumer->text, rhs)) {
    if (!generator->has_error) {
      code_generator_set_error(
          generator,
          "Failed to emit chained integer expression in function '%s'",
          context->function_name);
    }
    return 0;
  }

  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &consumer->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }

  *consumed_out = 2;
  return 1;
}

static int code_generator_emit_binary_function(CodeGenerator *generator,
                                               FunctionDeclaration *function_data,
                                               IRFunction *ir_function) {
  BinaryEmitter *emitter = NULL;
  BinaryFunctionContext context = {0};
  size_t text_section = 0;
  BinarySection *section = NULL;
  size_t function_offset = 0;
  size_t return_offset = 0;

  if (!generator || !function_data || !ir_function) {
    return 0;
  }

  if (!code_generator_binary_validate_signature(generator, function_data,
                                                ir_function)) {
    return 0;
  }

  if (!code_generator_binary_prepare_function_context(generator, function_data,
                                                      ir_function, &context)) {
    return 0;
  }

  if (!code_generator_binary_emit_prologue(generator, &context, function_data)) {
    binary_function_context_destroy(&context);
    return 0;
  }

  for (size_t i = 0; i < ir_function->instruction_count;) {
    size_t consumed = 0;
    if (code_generator_binary_try_emit_binary_compare_branch_chain(
            generator, &context, ir_function, i, &consumed)) {
      i += consumed;
      continue;
    }

    if (code_generator_binary_try_emit_address_add_load(
            generator, &context, ir_function, i, &consumed)) {
      i += consumed;
      continue;
    }

    if (code_generator_binary_try_emit_address_add_store(
            generator, &context, ir_function, i, &consumed)) {
      i += consumed;
      continue;
    }

    if (code_generator_binary_try_emit_scaled_address_load(
            generator, &context, ir_function, i, &consumed)) {
      i += consumed;
      continue;
    }

    if (code_generator_binary_try_emit_scaled_address_store(
            generator, &context, ir_function, i, &consumed)) {
      i += consumed;
      continue;
    }

    if (code_generator_binary_try_emit_binary_cast_chain(
            generator, &context, ir_function, i, &consumed)) {
      i += consumed;
      continue;
    }

    if (code_generator_binary_try_emit_binary_expression_chain(
            generator, &context, ir_function, i, &consumed)) {
      i += consumed;
      continue;
    }

    if (code_generator_binary_try_emit_compare_branch_zero(
            generator, &context, ir_function, i, &consumed)) {
      i += consumed;
      continue;
    }

    if (!code_generator_binary_emit_instruction(
            generator, &context, &ir_function->instructions[i])) {
      binary_function_context_destroy(&context);
      return 0;
    }
    i++;
  }

  return_offset = context.code.size;
  /* Win64 returns floating values in XMM0. The function body leaves the raw
   * return bits in RAX; transfer at the return type's precision. */
  for (size_t i = context.saved_register_count; i > 0; i--) {
    size_t slot = i - 1;
    if (!binary_emit_mov_reg_mem(&context.code, context.saved_registers[slot],
                                 BINARY_GP_RBP,
                                 -context.saved_register_offsets[slot])) {
      code_generator_set_error(generator,
                               "Out of memory while restoring callee registers");
      binary_function_context_destroy(&context);
      return 0;
    }
  }

  if ((context.return_float_bits == 32 &&
       !binary_emit_movd_xmm_reg(&context.code, BINARY_XMM0,
                                 BINARY_GP_RAX)) ||
      (context.return_float_bits == 64 &&
       !binary_emit_movq_xmm_reg(&context.code, BINARY_XMM0,
                                 BINARY_GP_RAX)) ||
      !binary_emit_mov_reg_reg(&context.code, BINARY_GP_RSP, BINARY_GP_RBP) ||
      !binary_emit_pop_reg(&context.code, BINARY_GP_RBP) ||
      !binary_emit_ret(&context.code)) {
    code_generator_set_error(generator,
                             "Out of memory while emitting function epilogue");
    binary_function_context_destroy(&context);
    return 0;
  }

  if (!code_generator_binary_resolve_fixups(generator, &context, return_offset)) {
    binary_function_context_destroy(&context);
    return 0;
  }

  emitter = code_generator_get_binary_emitter(generator);
  if (!emitter) {
    code_generator_set_error(generator, "Binary emitter is not initialized");
    binary_function_context_destroy(&context);
    return 0;
  }

  text_section = binary_emitter_get_or_create_section(
      emitter, ".text", BINARY_SECTION_TEXT, 0, BINARY_TEXT_SECTION_ALIGNMENT);
  if (text_section == (size_t)-1) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to create .text section");
    binary_function_context_destroy(&context);
    return 0;
  }

  if (!binary_emitter_align_section(emitter, text_section,
                                    BINARY_TEXT_SECTION_ALIGNMENT, 0x90)) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to align .text section");
    binary_function_context_destroy(&context);
    return 0;
  }

  section = binary_emitter_get_section(emitter, text_section);
  if (!section) {
    code_generator_set_error(generator, "Failed to access .text section");
    binary_function_context_destroy(&context);
    return 0;
  }

  function_offset = section->size;
  if (!binary_emitter_define_symbol(emitter, function_data->name,
                                    BINARY_SYMBOL_GLOBAL, text_section,
                                    function_offset, context.code.size) ||
      !binary_emitter_append_bytes(emitter, text_section, context.code.data,
                                   context.code.size, NULL)) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to emit function machine code");
    binary_function_context_destroy(&context);
    return 0;
  }

  for (size_t i = 0; i < context.call_relocations.count; i++) {
    BinaryCallRelocation *relocation = &context.call_relocations.items[i];
    if (!binary_emitter_add_relocation(
            emitter, text_section,
            function_offset + relocation->displacement_offset,
            BINARY_RELOCATION_REL32, relocation->symbol_name, 0)) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to record function relocation");
      binary_function_context_destroy(&context);
      return 0;
    }
  }

  binary_function_context_destroy(&context);
  return 1;
}

static int code_generator_binary_numeric_constant_is_float(
    const BinaryNumericConstant *value, ASTNode *expression) {
  if (value && value->is_float) {
    return 1;
  }
  return expression && expression->resolved_type &&
         code_generator_binary_resolved_type_float_bits(
             expression->resolved_type) != 0;
}

static void code_generator_binary_numeric_constant_from_double(
    BinaryNumericConstant *out, double value) {
  out->is_float = 1;
  out->float_value = value;
  out->int_value = (long long)value;
}

static void code_generator_binary_numeric_constant_from_int(
    BinaryNumericConstant *out, long long value) {
  out->is_float = 0;
  out->int_value = value;
  out->float_value = (double)value;
}

static int code_generator_binary_eval_numeric_global_initializer(
    ASTNode *expression, BinaryNumericConstant *out_value) {
  if (!expression || !out_value) {
    return 0;
  }

  switch (expression->type) {
  case AST_NUMBER_LITERAL: {
    NumberLiteral *literal = (NumberLiteral *)expression->data;
    if (!literal) {
      return 0;
    }
    if (literal->is_float) {
      code_generator_binary_numeric_constant_from_double(out_value,
                                                         literal->float_value);
    } else {
      code_generator_binary_numeric_constant_from_int(out_value,
                                                      literal->int_value);
    }
    return 1;
  }

  case AST_IDENTIFIER: {
    Identifier *identifier = (Identifier *)expression->data;
    return identifier && identifier->name &&
           binary_global_const_table_get_numeric(identifier->name, out_value);
  }

  case AST_UNARY_EXPRESSION: {
    UnaryExpression *unary = (UnaryExpression *)expression->data;
    BinaryNumericConstant operand = {0};
    if (!unary || !unary->operator || !unary->operand ||
        !code_generator_binary_eval_numeric_global_initializer(unary->operand,
                                                               &operand)) {
      return 0;
    }

    if (strcmp(unary->operator, "+") == 0) {
      *out_value = operand;
      return 1;
    }
    if (strcmp(unary->operator, "-") == 0) {
      if (code_generator_binary_numeric_constant_is_float(&operand,
                                                          expression)) {
        code_generator_binary_numeric_constant_from_double(
            out_value, -(operand.is_float ? operand.float_value
                                          : (double)operand.int_value));
      } else {
        code_generator_binary_numeric_constant_from_int(out_value,
                                                        -operand.int_value);
      }
      return 1;
    }
    if (strcmp(unary->operator, "!") == 0) {
      int is_zero =
          operand.is_float ? (operand.float_value == 0.0)
                           : (operand.int_value == 0);
      code_generator_binary_numeric_constant_from_int(out_value, is_zero);
      return 1;
    }
    if (strcmp(unary->operator, "~") == 0 && !operand.is_float) {
      code_generator_binary_numeric_constant_from_int(out_value,
                                                      ~operand.int_value);
      return 1;
    }
    return 0;
  }

  case AST_BINARY_EXPRESSION: {
    BinaryExpression *binary = (BinaryExpression *)expression->data;
    BinaryNumericConstant left = {0};
    BinaryNumericConstant right = {0};
    int result_is_float = 0;
    if (!binary || !binary->operator || !binary->left || !binary->right ||
        !code_generator_binary_eval_numeric_global_initializer(binary->left,
                                                               &left) ||
        !code_generator_binary_eval_numeric_global_initializer(binary->right,
                                                               &right)) {
      return 0;
    }

    result_is_float =
        left.is_float || right.is_float ||
        code_generator_binary_numeric_constant_is_float(NULL, expression);
    if (result_is_float) {
      double lhs = left.is_float ? left.float_value : (double)left.int_value;
      double rhs = right.is_float ? right.float_value : (double)right.int_value;
      if (strcmp(binary->operator, "+") == 0) {
        code_generator_binary_numeric_constant_from_double(out_value, lhs + rhs);
      } else if (strcmp(binary->operator, "-") == 0) {
        code_generator_binary_numeric_constant_from_double(out_value, lhs - rhs);
      } else if (strcmp(binary->operator, "*") == 0) {
        code_generator_binary_numeric_constant_from_double(out_value, lhs * rhs);
      } else if (strcmp(binary->operator, "/") == 0) {
        code_generator_binary_numeric_constant_from_double(out_value, lhs / rhs);
      } else if (strcmp(binary->operator, "==") == 0) {
        code_generator_binary_numeric_constant_from_int(out_value, lhs == rhs);
      } else if (strcmp(binary->operator, "!=") == 0) {
        code_generator_binary_numeric_constant_from_int(out_value, lhs != rhs);
      } else if (strcmp(binary->operator, "<") == 0) {
        code_generator_binary_numeric_constant_from_int(out_value, lhs < rhs);
      } else if (strcmp(binary->operator, "<=") == 0) {
        code_generator_binary_numeric_constant_from_int(out_value, lhs <= rhs);
      } else if (strcmp(binary->operator, ">") == 0) {
        code_generator_binary_numeric_constant_from_int(out_value, lhs > rhs);
      } else if (strcmp(binary->operator, ">=") == 0) {
        code_generator_binary_numeric_constant_from_int(out_value, lhs >= rhs);
      } else {
        return 0;
      }
      return 1;
    }

    if (strcmp(binary->operator, "+") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value + right.int_value);
    } else if (strcmp(binary->operator, "-") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value - right.int_value);
    } else if (strcmp(binary->operator, "*") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value * right.int_value);
    } else if (strcmp(binary->operator, "/") == 0) {
      if (right.int_value == 0) {
        return 0;
      }
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value / right.int_value);
    } else if (strcmp(binary->operator, "%") == 0) {
      if (right.int_value == 0) {
        return 0;
      }
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value % right.int_value);
    } else if (strcmp(binary->operator, "==") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value == right.int_value);
    } else if (strcmp(binary->operator, "!=") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value != right.int_value);
    } else if (strcmp(binary->operator, "<") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value < right.int_value);
    } else if (strcmp(binary->operator, "<=") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value <= right.int_value);
    } else if (strcmp(binary->operator, ">") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value > right.int_value);
    } else if (strcmp(binary->operator, ">=") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value >= right.int_value);
    } else if (strcmp(binary->operator, "&&") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value != 0 && right.int_value != 0);
    } else if (strcmp(binary->operator, "||") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value != 0 || right.int_value != 0);
    } else if (strcmp(binary->operator, "&") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value & right.int_value);
    } else if (strcmp(binary->operator, "|") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value | right.int_value);
    } else if (strcmp(binary->operator, "^") == 0) {
      code_generator_binary_numeric_constant_from_int(
          out_value, left.int_value ^ right.int_value);
    } else {
      return 0;
    }
    return 1;
  }

  case AST_CAST_EXPRESSION: {
    CastExpression *cast = (CastExpression *)expression->data;
    BinaryNumericConstant operand = {0};
    int target_float_bits = 0;
    if (!cast || !cast->operand ||
        !code_generator_binary_eval_numeric_global_initializer(cast->operand,
                                                               &operand)) {
      return 0;
    }
    target_float_bits = expression->resolved_type
                            ? code_generator_binary_resolved_type_float_bits(
                                  expression->resolved_type)
                            : 0;
    if (target_float_bits != 0) {
      code_generator_binary_numeric_constant_from_double(
          out_value, operand.is_float ? operand.float_value
                                      : (double)operand.int_value);
    } else {
      code_generator_binary_numeric_constant_from_int(
          out_value, operand.is_float ? (long long)operand.float_value
                                      : operand.int_value);
    }
    return 1;
  }

  default:
    return 0;
  }
}

static int code_generator_emit_binary_global_variable(CodeGenerator *generator,
                                                      VarDeclaration *var_data) {
  BinaryEmitter *emitter = NULL;
  Symbol *symbol = NULL;
  Type *type = NULL;
  const char *link_name = NULL;
  const char *section_name = NULL;
  BinarySectionKind section_kind = BINARY_SECTION_DATA;
  size_t section_index = 0;
  size_t value_offset = 0;
  size_t alignment = 1;
  int size = 0;
  unsigned char bytes[8] = {0};

  if (!generator || !var_data || !var_data->name) {
    return 0;
  }

  emitter = code_generator_get_binary_emitter(generator);
  if (!emitter) {
    code_generator_set_error(generator, "Binary emitter is not initialized");
    return 0;
  }

  symbol = generator->symbol_table
               ? symbol_table_lookup(generator->symbol_table, var_data->name)
               : NULL;
  type = symbol ? symbol->type
                : code_generator_binary_get_resolved_type(generator,
                                                          var_data->type_name,
                                                          0);
  if (!type) {
    code_generator_set_error(
        generator,
        "Direct object backend only supports scalar integer/pointer/string/"
        "float64 global variables (encountered '%s')",
        var_data->name);
    return 0;
  }

  link_name = code_generator_get_link_symbol_name(generator, var_data->name);
  if (!link_name || link_name[0] == '\0') {
    code_generator_set_error(generator, "Invalid global symbol '%s'",
                             var_data->name);
    return 0;
  }

  if (type->kind == TYPE_STRING) {
    const char *initializer_value = NULL;
    StringLiteral *literal = NULL;

    if (var_data->initializer) {
      if (var_data->initializer->type != AST_STRING_LITERAL) {
        code_generator_set_error(
            generator,
            "Direct object backend only supports string-literal global "
            "initializers for string globals (encountered '%s')",
            var_data->name);
        return 0;
      }

      literal = (StringLiteral *)var_data->initializer->data;
      if (!literal) {
        code_generator_set_error(generator,
                                 "Malformed string global initializer '%s'",
                                 var_data->name);
        return 0;
      }
      initializer_value = literal->value ? literal->value : "";
    }

    return code_generator_binary_emit_global_string_variable(generator,
                                                             link_name,
                                                             initializer_value);
  }

  if (!code_generator_binary_resolved_type_is_supported(type, 0)) {
    code_generator_set_error(
        generator,
        "Direct object backend only supports scalar integer/pointer/string/"
        "float64 global variables (encountered '%s')",
        var_data->name);
    return 0;
  }

  size = code_generator_binary_resolved_type_scalar_size(type);
  if (size <= 0 || size > 8) {
    code_generator_set_error(
        generator,
        "Direct object backend only supports global variables up to 8 bytes "
        "(encountered '%s')",
        var_data->name);
    return 0;
  }

  section_kind =
      var_data->initializer ? BINARY_SECTION_DATA : BINARY_SECTION_BSS;
  section_name = section_kind == BINARY_SECTION_DATA ? ".data" : ".bss";
  alignment = type->alignment ? type->alignment : (size_t)size;
  if (alignment == 0) {
    alignment = 1;
  }

  section_index = binary_emitter_get_or_create_section(
      emitter, section_name, section_kind, 0, alignment);
  if (section_index == (size_t)-1) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to create global variable section");
    return 0;
  }

  if (!binary_emitter_align_section(emitter, section_index, alignment, 0)) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to align global variable section");
    return 0;
  }

  if (var_data->initializer) {
    BinaryNumericConstant constant = {0};
    int float_bits = code_generator_binary_resolved_type_float_bits(type);
    if (!code_generator_binary_eval_numeric_global_initializer(
            var_data->initializer, &constant)) {
      code_generator_set_error(
          generator,
          "Direct object backend only supports constant numeric global "
          "initializers "
          "(encountered '%s')",
          var_data->name);
      return 0;
    }

    if (float_bits == 64) {
      double value = constant.is_float ? constant.float_value
                                       : (double)constant.int_value;
      memcpy(bytes, &value, sizeof(value));
    } else if (float_bits == 32) {
      float value = (float)(constant.is_float ? constant.float_value
                                              : (double)constant.int_value);
      memcpy(bytes, &value, sizeof(value));
    } else {
      uint64_t encoded = (uint64_t)constant.int_value;
      if (constant.is_float) {
        code_generator_set_error(
            generator,
            "Direct object backend does not support floating global "
            "initializers for non-float globals (encountered '%s')",
            var_data->name);
        return 0;
      }
      memcpy(bytes, &encoded, (size_t)size);
    }

    if (!binary_emitter_append_bytes(emitter, section_index, bytes, (size_t)size,
                                     &value_offset)) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to emit global initializer");
      return 0;
    }
  } else if (!binary_emitter_append_zeros(emitter, section_index, (size_t)size,
                                          &value_offset)) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to reserve global storage");
    return 0;
  }

  if (!binary_emitter_define_symbol(emitter, link_name, BINARY_SYMBOL_GLOBAL,
                                    section_index, value_offset, (size_t)size)) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to define global variable symbol");
    return 0;
  }

  return 1;
}

static int code_generator_binary_global_is_written(IRProgram *ir_program,
                                                   const char *name) {
  if (!ir_program || !name) {
    return 1;
  }

  for (size_t fn_i = 0; fn_i < ir_program->function_count; fn_i++) {
    IRFunction *function = ir_program->functions[fn_i];
    if (!function) {
      continue;
    }

    for (size_t insn_i = 0; insn_i < function->instruction_count; insn_i++) {
      IRInstruction *instruction = &function->instructions[insn_i];
      if (!instruction) {
        continue;
      }

      if (instruction->dest.kind == IR_OPERAND_SYMBOL &&
          instruction->dest.name && strcmp(instruction->dest.name, name) == 0) {
        return 1;
      }
      if (instruction->op == IR_OP_ADDRESS_OF &&
          instruction->lhs.kind == IR_OPERAND_SYMBOL &&
          instruction->lhs.name && strcmp(instruction->lhs.name, name) == 0) {
        return 1;
      }
      if (instruction->op == IR_OP_STORE &&
          instruction->dest.kind == IR_OPERAND_SYMBOL &&
          instruction->dest.name && strcmp(instruction->dest.name, name) == 0) {
        return 1;
      }
    }
  }

  return 0;
}

static int code_generator_binary_collect_global_constants(
    CodeGenerator *generator, Program *program_data) {
  if (!generator || !program_data) {
    return 0;
  }

  for (size_t i = 0; i < program_data->declaration_count; i++) {
    ASTNode *declaration = program_data->declarations[i];
    if (!declaration || declaration->type != AST_VAR_DECLARATION) {
      continue;
    }

    VarDeclaration *var_data = (VarDeclaration *)declaration->data;
    if (!var_data || !var_data->name || var_data->is_extern ||
        !var_data->initializer) {
      continue;
    }

    Type *type = code_generator_binary_get_resolved_type(generator,
                                                         var_data->type_name, 0);
    if (!type || !code_generator_binary_resolved_type_is_supported(type, 0) ||
        type->kind == TYPE_STRING || type->kind == TYPE_VOID ||
        type->size == 0 || type->size > 8) {
      continue;
    }

    BinaryNumericConstant constant = {0};
    if (!code_generator_binary_eval_numeric_global_initializer(
            var_data->initializer, &constant)) {
      continue;
    }

    if (!binary_global_const_table_add(
            var_data->name, constant.int_value, constant.float_value,
            constant.is_float,
            !code_generator_binary_global_is_written(generator->ir_program,
                                                     var_data->name))) {
      code_generator_set_error(
          generator, "Out of memory while tracking constant global '%s'",
          var_data->name);
      return 0;
    }
  }

  return 1;
}

static int code_generator_declare_binary_externs(CodeGenerator *generator,
                                                 Program *program_data) {
  BinaryEmitter *emitter = NULL;

  if (!generator || !program_data) {
    return 0;
  }

  emitter = code_generator_get_binary_emitter(generator);
  if (!emitter) {
    code_generator_set_error(generator, "Binary emitter is not initialized");
    return 0;
  }

  for (size_t i = 0; i < program_data->declaration_count; i++) {
    ASTNode *declaration = program_data->declarations[i];
    const char *extern_name = NULL;

    if (!declaration) {
      continue;
    }

    if (declaration->type == AST_FUNCTION_DECLARATION) {
      FunctionDeclaration *function_data =
          (FunctionDeclaration *)declaration->data;
      if (!function_data || !function_data->is_extern || !function_data->name) {
        continue;
      }
      extern_name =
          code_generator_get_link_symbol_name(generator, function_data->name);
    } else if (declaration->type == AST_VAR_DECLARATION) {
      VarDeclaration *var_data = (VarDeclaration *)declaration->data;
      if (!var_data || !var_data->is_extern || !var_data->name) {
        continue;
      }
      extern_name =
          code_generator_get_link_symbol_name(generator, var_data->name);
    } else {
      continue;
    }

    if (!binary_emitter_declare_external(emitter, extern_name)) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to declare external symbol");
      return 0;
    }
  }

  return 1;
}

int code_generator_generate_program_binary_object(CodeGenerator *generator,
                                                  ASTNode *program) {
  Program *program_data = NULL;

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
  if (generator->debug_info || generator->generate_debug_info) {
    code_generator_set_error(
        generator,
        "Direct object backend does not yet support debug info emission");
    return 0;
  }

  binary_emitter_reset(generator->binary_emitter);
  program_data = (Program *)program->data;
  if (!program_data) {
    code_generator_set_error(generator, "Program node is missing data");
    return 0;
  }

  binary_global_const_table_reset();
  binary_ir_function_index_reset();
  if (!code_generator_binary_collect_global_constants(generator, program_data)) {
    return 0;
  }

  if (!code_generator_declare_binary_externs(generator, program_data)) {
    return 0;
  }

  for (size_t i = 0; i < program_data->declaration_count; i++) {
    ASTNode *declaration = program_data->declarations[i];
    if (!declaration) {
      continue;
    }

    switch (declaration->type) {
    case AST_FUNCTION_DECLARATION: {
      FunctionDeclaration *function_data =
          (FunctionDeclaration *)declaration->data;
      IRFunction *ir_function = NULL;

      if (!function_data || !function_data->name) {
        code_generator_set_error(generator,
                                 "Malformed function declaration in AST");
        return 0;
      }
      if (function_data->is_extern || !function_data->body) {
        continue;
      }

      ir_function = code_generator_find_ir_function_binary(generator,
                                                           function_data->name);
      if (!ir_function) {
        code_generator_set_error(generator,
                                 "No IR body found for function '%s'",
                                 function_data->name);
        return 0;
      }

      if (!code_generator_emit_binary_function(generator, function_data,
                                               ir_function)) {
        return 0;
      }
    } break;
    case AST_STRUCT_DECLARATION:
    case AST_ENUM_DECLARATION:
      break;
    case AST_VAR_DECLARATION: {
      VarDeclaration *var_data = (VarDeclaration *)declaration->data;
      if (!var_data || !var_data->name) {
        code_generator_set_error(generator,
                                 "Malformed global variable declaration in AST");
        return 0;
      }
      if (var_data->is_extern) {
        break;
      }
      if (!code_generator_emit_binary_global_variable(generator, var_data)) {
        return 0;
      }
    }
      break;
    case AST_INLINE_ASM:
      code_generator_set_error(
          generator,
          "Direct object backend does not yet support inline assembly");
      return 0;
    default:
      code_generator_set_error(
          generator,
          "Direct object backend encountered unsupported declaration type %d",
          declaration->type);
      return 0;
    }
  }

  int ok = generator->has_error ? 0 : 1;
  binary_global_const_table_reset();
  binary_ir_function_index_reset();
  return ok;
}
