#include "code_generator_internal.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

#define BINARY_TEXT_SECTION_ALIGNMENT 16
#define BINARY_FUNCTION_STACK_SLOT_SIZE 8
#define BINARY_WIN64_REGISTER_ARG_COUNT 4
#define BINARY_WIN64_SHADOW_SPACE_SIZE 32

typedef enum {
  BINARY_GP_RAX = 0,
  BINARY_GP_RCX = 1,
  BINARY_GP_RDX = 2,
  BINARY_GP_RSP = 4,
  BINARY_GP_RBP = 5,
  BINARY_GP_R11 = 11,
  BINARY_GP_R8 = 8,
  BINARY_GP_R9 = 9,
  BINARY_GP_R10 = 10,
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
  BinaryCodeBuffer code;
  BinaryNamedSlotTable parameter_slots;
  BinaryNamedSlotTable local_slots;
  BinaryNamedSlotTable temp_slots;
  BinaryNamedSlotTable float64_symbols;
  BinaryLabelTable labels;
  BinaryLabelFixupTable label_fixups;
  BinaryCallRelocationTable call_relocations;
  BinaryOffsetTable return_fixups;
  int raw_frame_size;
  int frame_size;
  int return_is_float64;
  FunctionDeclaration *function_data;
  const char *function_name;
} BinaryFunctionContext;

static const BinaryGpRegister BINARY_WIN64_INT_PARAM_REGISTERS[] = {
    BINARY_GP_RCX, BINARY_GP_RDX, BINARY_GP_R8, BINARY_GP_R9};
static const BinaryXmmRegister BINARY_WIN64_FLOAT_PARAM_REGISTERS[] = {
    BINARY_XMM0, BINARY_XMM1, BINARY_XMM2, BINARY_XMM3};

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

static IRFunction *code_generator_find_ir_function_binary(CodeGenerator *generator,
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
  binary_label_table_destroy(&context->labels);
  binary_label_fixup_table_destroy(&context->label_fixups);
  binary_call_relocation_table_destroy(&context->call_relocations);
  binary_offset_table_destroy(&context->return_fixups);
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

  if (!binary_emit_rex(buffer, 1, destination >> 3, 0, source >> 3) ||
      !binary_code_buffer_append_u8(buffer, 0x8B) ||
      !binary_code_buffer_append_u8(
          buffer, (unsigned char)(0xC0 | ((destination & 7) << 3) |
                                  (source & 7)))) {
    return 0;
  }

  return 1;
}

static int binary_emit_alu_rsp_imm32(BinaryCodeBuffer *buffer,
                                     unsigned char subopcode,
                                     uint32_t immediate) {
  if (!buffer) {
    return 0;
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

static int binary_emit_mov_reg_imm64(BinaryCodeBuffer *buffer,
                                     BinaryGpRegister destination,
                                     uint64_t immediate) {
  if (!buffer) {
    return 0;
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

static int binary_emit_je_placeholder(BinaryCodeBuffer *buffer,
                                      size_t *displacement_offset_out) {
  if (!buffer || !displacement_offset_out) {
    return 0;
  }

  if (!binary_code_buffer_append_u8(buffer, 0x0F) ||
      !binary_code_buffer_append_u8(buffer, 0x84)) {
    return 0;
  }

  *displacement_offset_out = buffer->size;
  return binary_code_buffer_append_u32(buffer, 0);
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

static int code_generator_binary_resolved_type_is_float64(Type *type) {
  return type && type->kind == TYPE_FLOAT64 && type->size == 8;
}

static int code_generator_binary_resolved_type_is_abi_supported(Type *type,
                                                                int allow_void) {
  if (!type) {
    return 0;
  }

  if (type->kind == TYPE_STRING) {
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

static int code_generator_binary_is_marked_float64_symbol(
    const BinaryFunctionContext *context, const char *name) {
  return context && name &&
         binary_named_slot_table_get_offset(&context->float64_symbols, name) >=
             0;
}

static int code_generator_binary_mark_float64_symbol(
    BinaryFunctionContext *context, const char *name) {
  if (!context || !name || name[0] == '\0') {
    return 0;
  }

  return binary_named_slot_table_add(&context->float64_symbols, name, 1);
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

  return code_generator_binary_resolved_type_is_stack_scalar(symbol->type);
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

  for (size_t i = 0; i < function_data->parameter_count; i++) {
    const char *parameter_name = function_data->parameter_names[i];
    int offset = (int)((i + 1) * BINARY_FUNCTION_STACK_SLOT_SIZE);
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

    if (code_generator_binary_named_type_is_float64(
            generator,
            function_data->parameter_types ? function_data->parameter_types[i]
                                           : NULL,
            0) &&
        !code_generator_binary_mark_float64_symbol(context, parameter_name)) {
      code_generator_set_error(
          generator,
          "Failed to allocate float64 parameter metadata in function '%s'",
          function_data->name);
      binary_function_context_destroy(context);
      return 0;
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

    scalar_local = code_generator_binary_resolved_type_is_stack_scalar(local_type);
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
      if (code_generator_binary_resolved_type_is_float64(local_type) &&
          !code_generator_binary_mark_float64_symbol(context,
                                                     instruction->dest.name)) {
        code_generator_set_error(
            generator,
            "Failed to allocate float64 local metadata in function '%s'",
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

    if (code_generator_binary_resolved_type_is_float64(local_type) &&
        !code_generator_binary_mark_float64_symbol(context,
                                                   instruction->dest.name)) {
      code_generator_set_error(
          generator,
          "Failed to allocate float64 local metadata in function '%s'",
          function_data->name);
      binary_function_context_destroy(context);
      return 0;
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

    if (!code_generator_binary_instruction_result_is_float64(
            generator, context, instruction)) {
      continue;
    }

    if (!code_generator_binary_mark_float64_symbol(context,
                                                   instruction->dest.name)) {
      code_generator_set_error(
          generator,
          "Failed to allocate float64 temporary metadata in function '%s'",
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

  context->raw_frame_size = parameter_home_size + local_home_size + temp_home_size;
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
  default:
    code_generator_set_error(
        generator,
        "Direct object backend does not yet support memory stores wider than "
        "8 bytes in function '%s'",
        context->function_name);
    return 0;
  }
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
    if ((!binary_emit_movzx_reg_rip_mem8(&context->code, BINARY_GP_RAX,
                                         &displacement_offset)) ||
        (is_signed && !binary_emit_movsx_rax_al(&context->code))) {
      return 0;
    }
    break;
  case 2:
    if ((!binary_emit_movzx_reg_rip_mem16(&context->code, BINARY_GP_RAX,
                                          &displacement_offset)) ||
        (is_signed && !binary_emit_movsx_rax_ax(&context->code))) {
      return 0;
    }
    break;
  case 4:
    if (!binary_emit_mov_reg32_rip_mem(&context->code, BINARY_GP_RAX,
                                       &displacement_offset) ||
        (is_signed && !binary_emit_movsxd_rax_eax(&context->code))) {
      return 0;
    }
    break;
  case 8:
    if (!binary_emit_mov_reg_rip_mem(&context->code, BINARY_GP_RAX,
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

  if (target_register != BINARY_GP_RAX &&
      !binary_emit_mov_reg_reg(&context->code, target_register,
                               BINARY_GP_RAX)) {
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

  default:
    return 0;
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

static int code_generator_binary_emit_float_operand_to_xmm(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *operand, BinaryXmmRegister target_register) {
  if (!generator || !context || !operand) {
    return 0;
  }

  if (!code_generator_binary_emit_operand_load(generator, context, operand,
                                               BINARY_GP_RAX)) {
    return 0;
  }

  if (code_generator_binary_operand_is_known_float64(generator, context,
                                                     operand)) {
    return binary_emit_movq_xmm_reg(&context->code, target_register,
                                    BINARY_GP_RAX);
  }

  return binary_emit_cvtsi2sd_xmm_reg(&context->code, target_register,
                                      BINARY_GP_RAX);
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
    Symbol *symbol = generator && generator->symbol_table
                         ? symbol_table_lookup(generator->symbol_table,
                                               operand->name)
                         : NULL;
    int offset = code_generator_binary_get_symbol_offset(context, operand->name);
    if (symbol && symbol->type && symbol->type->kind == TYPE_STRING) {
      return code_generator_binary_emit_string_symbol_load(
          generator, context, operand->name, symbol, target_register);
    }
    if (offset <= 0) {
      if (symbol && symbol->scope && symbol->scope->type == SCOPE_GLOBAL) {
        const char *link_name =
            code_generator_get_link_symbol_name(generator, operand->name);
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
  const char *trap_symbol = "meth_runtime_debug_trap";

  if (!generator || !context || !instruction || instruction->argument_count == 0) {
    return 0;
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
      if (!binary_emit_lea_reg_mem(&context->code, BINARY_GP_RAX,
                                   BINARY_GP_RBP, -offset)) {
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

  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RAX) ||
      !code_generator_binary_emit_load_from_address(generator, context,
                                                    BINARY_GP_RAX, size,
                                                    BINARY_GP_RAX) ||
      !code_generator_binary_emit_destination_store(generator, context,
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

  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RCX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->dest,
                                               BINARY_GP_RAX) ||
      !code_generator_binary_emit_store_to_address(generator, context,
                                                   BINARY_GP_RAX, size,
                                                   BINARY_GP_RCX)) {
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
  const char *allocator_name = "gc_alloc";

  if (!generator || !context || !instruction) {
    return 0;
  }

  if (!code_generator_binary_declare_external_symbol(generator, allocator_name)) {
    return 0;
  }

  if (instruction->rhs.kind == IR_OPERAND_INT && instruction->rhs.int_value > 0) {
    if (!binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RCX,
                                   (uint64_t)instruction->rhs.int_value)) {
      code_generator_set_error(generator,
                               "Out of memory while emitting allocation size");
      return 0;
    }
  } else if (instruction->rhs.kind == IR_OPERAND_NONE ||
             (instruction->rhs.kind == IR_OPERAND_INT &&
              instruction->rhs.int_value <= 0)) {
    if (!binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RCX, 8)) {
      code_generator_set_error(generator,
                               "Out of memory while emitting allocation size");
      return 0;
    }
  } else if (!code_generator_binary_emit_operand_load(
                 generator, context, &instruction->rhs, BINARY_GP_RCX)) {
    return 0;
  }

  if (!binary_emit_sub_rsp_imm32(&context->code,
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

  if (instruction->is_float && !target_is_float) {
    if (!binary_emit_movq_xmm_reg(&context->code, BINARY_XMM0,
                                  BINARY_GP_RAX) ||
        !binary_emit_cvttsd2si_reg_xmm(&context->code, BINARY_GP_RAX,
                                       BINARY_XMM0)) {
      goto emit_failure;
    }
  } else if (!instruction->is_float && target_is_float) {
    if (!binary_emit_cvtsi2sd_xmm_reg(&context->code, BINARY_XMM0,
                                      BINARY_GP_RAX) ||
        !binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                  BINARY_XMM0)) {
      goto emit_failure;
    }
  } else if (instruction->is_float && target_is_float) {
    /* No-op for float64-to-float64 today. */
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

  if (!generator || !context || !instruction || !instruction->text ||
      instruction->text[0] == '\0') {
    return 0;
  }

  if (strcmp(instruction->text, "meth_runtime_debug_trap") == 0) {
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

  size_t stack_argument_count =
      instruction->argument_count > BINARY_WIN64_REGISTER_ARG_COUNT
          ? instruction->argument_count - BINARY_WIN64_REGISTER_ARG_COUNT
          : 0;
  if (stack_argument_count >
      (size_t)(INT_MAX / BINARY_FUNCTION_STACK_SLOT_SIZE)) {
    code_generator_set_error(generator,
                             "Too many call arguments in function '%s'",
                             context->function_name);
    return 0;
  }

  int call_stack_total = BINARY_WIN64_SHADOW_SPACE_SIZE +
                         (int)(stack_argument_count *
                               BINARY_FUNCTION_STACK_SLOT_SIZE);
  if (!binary_align_up_int(call_stack_total, 16, &call_stack_total)) {
    code_generator_set_error(generator,
                             "Call frame too large in function '%s'",
                             context->function_name);
    return 0;
  }

  if (call_stack_total > 0 &&
      !binary_emit_sub_rsp_imm32(&context->code, (uint32_t)call_stack_total)) {
    code_generator_set_error(generator,
                             "Out of memory while emitting call frame");
    return 0;
  }

  for (size_t i = BINARY_WIN64_REGISTER_ARG_COUNT;
       i < instruction->argument_count; i++) {
    int slot_offset = BINARY_WIN64_SHADOW_SPACE_SIZE +
                      (int)((i - BINARY_WIN64_REGISTER_ARG_COUNT) *
                            BINARY_FUNCTION_STACK_SLOT_SIZE);
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
      if (!generator->has_error) {
        code_generator_set_error(generator,
                                 "Out of memory while materializing call args");
      }
      return 0;
    }
  }

  size_t register_argument_count = instruction->argument_count;
  if (register_argument_count > BINARY_WIN64_REGISTER_ARG_COUNT) {
    register_argument_count = BINARY_WIN64_REGISTER_ARG_COUNT;
  }
  for (size_t i = 0; i < register_argument_count; i++) {
    int argument_is_float64 =
        function_symbol && function_symbol->kind == SYMBOL_FUNCTION &&
        function_symbol->data.function.parameter_types &&
        code_generator_binary_resolved_type_is_float64(
            function_symbol->data.function.parameter_types[i]);
    Type *parameter_type =
        function_symbol && function_symbol->kind == SYMBOL_FUNCTION &&
                function_symbol->data.function.parameter_types
            ? function_symbol->data.function.parameter_types[i]
            : NULL;
    if ((argument_is_float64 &&
         (!code_generator_binary_emit_call_argument_load(
              generator, context, &instruction->arguments[i], parameter_type,
              BINARY_GP_RAX) ||
          !binary_emit_movq_xmm_reg(&context->code,
                                    BINARY_WIN64_FLOAT_PARAM_REGISTERS[i],
                                    BINARY_GP_RAX))) ||
        (!argument_is_float64 &&
         !code_generator_binary_emit_call_argument_load(
             generator, context, &instruction->arguments[i], parameter_type,
             BINARY_WIN64_INT_PARAM_REGISTERS[i]))) {
      return 0;
    }
  }

  size_t displacement_offset = 0;
  const char *link_target =
      code_generator_get_link_symbol_name(generator, instruction->text);
  if (!link_target || link_target[0] == '\0') {
    code_generator_set_error(generator, "Invalid call target '%s'",
                             instruction->text);
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

  if (function_symbol && function_symbol->kind == SYMBOL_FUNCTION &&
      code_generator_binary_resolved_type_is_float64(
          function_symbol->data.function.return_type) &&
      !binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                BINARY_XMM0)) {
    code_generator_set_error(generator,
                             "Out of memory while materializing float call "
                             "return in function '%s'",
                             context->function_name);
    return 0;
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
    int argument_is_float64 =
        function_type && function_type->fn_param_types &&
        code_generator_binary_resolved_type_is_float64(
            function_type->fn_param_types[i]);
    Type *parameter_type =
        function_type && function_type->fn_param_types
            ? function_type->fn_param_types[i]
            : NULL;
    if ((argument_is_float64 &&
         (!code_generator_binary_emit_call_argument_load(
              generator, context, &instruction->arguments[i], parameter_type,
              BINARY_GP_RAX) ||
          !binary_emit_movq_xmm_reg(&context->code,
                                    BINARY_WIN64_FLOAT_PARAM_REGISTERS[i],
                                    BINARY_GP_RAX))) ||
        (!argument_is_float64 &&
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

  if (function_type &&
      code_generator_binary_resolved_type_is_float64(
          function_type->fn_return_type) &&
      !binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                BINARY_XMM0)) {
    code_generator_set_error(generator,
                             "Out of memory while materializing float indirect "
                             "call return in function '%s'",
                             context->function_name);
    return 0;
  }

  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &instruction->dest,
                                                    BINARY_GP_RAX)) {
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
    const char *allocator_name = "gc_alloc";
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
    op = instruction->text;
    if (!code_generator_binary_emit_float_operand_to_xmm(
            generator, context, &instruction->rhs, BINARY_XMM1) ||
        !code_generator_binary_emit_float_operand_to_xmm(
            generator, context, &instruction->lhs, BINARY_XMM0)) {
      goto emit_failure;
    }

    if (strcmp(op, "+") == 0) {
      if (!binary_emit_addsd_xmm_xmm(&context->code, BINARY_XMM0,
                                     BINARY_XMM1) ||
          !binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                    BINARY_XMM0)) {
        goto emit_failure;
      }
    } else if (strcmp(op, "-") == 0) {
      if (!binary_emit_subsd_xmm_xmm(&context->code, BINARY_XMM0,
                                     BINARY_XMM1) ||
          !binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                    BINARY_XMM0)) {
        goto emit_failure;
      }
    } else if (strcmp(op, "*") == 0) {
      if (!binary_emit_mulsd_xmm_xmm(&context->code, BINARY_XMM0,
                                     BINARY_XMM1) ||
          !binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                    BINARY_XMM0)) {
        goto emit_failure;
      }
    } else if (strcmp(op, "/") == 0) {
      if (!binary_emit_divsd_xmm_xmm(&context->code, BINARY_XMM0,
                                     BINARY_XMM1) ||
          !binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                    BINARY_XMM0)) {
        goto emit_failure;
      }
    } else if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
               strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
               strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
      if (!binary_emit_ucomisd_xmm_xmm(&context->code, BINARY_XMM0,
                                       BINARY_XMM1)) {
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
  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_R10) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RAX)) {
    return 0;
  }

  if (strcmp(op, "+") == 0) {
    if (!binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RAX,
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
    op = instruction->text;
    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->lhs,
                                                 BINARY_GP_RAX) ||
        !binary_emit_movq_xmm_reg(&context->code, BINARY_XMM0,
                                  BINARY_GP_RAX)) {
      goto emit_failure;
    }

    if (strcmp(op, "-") == 0) {
      if (!binary_emit_pxor_xmm_xmm(&context->code, BINARY_XMM1,
                                    BINARY_XMM1) ||
          !binary_emit_subsd_xmm_xmm(&context->code, BINARY_XMM1,
                                     BINARY_XMM0) ||
          !binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                    BINARY_XMM1)) {
        goto emit_failure;
      }
    } else if (strcmp(op, "+") == 0) {
      if (!binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                    BINARY_XMM0)) {
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

  case IR_OP_ASSIGN:
    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->lhs,
                                                 BINARY_GP_RAX) ||
        !code_generator_binary_emit_destination_store(generator, context,
                                                      &instruction->dest,
                                                      BINARY_GP_RAX)) {
      if (!generator->has_error) {
        code_generator_set_error(generator,
                                 "Out of memory while emitting assign");
      }
      return 0;
    }
    return 1;

  case IR_OP_ADDRESS_OF:
    return code_generator_binary_emit_address_of(generator, context,
                                                 instruction);

  case IR_OP_LOAD:
    return code_generator_binary_emit_load(generator, context, instruction);

  case IR_OP_STORE:
    return code_generator_binary_emit_store(generator, context, instruction);

  case IR_OP_BINARY:
    return code_generator_binary_emit_binary(generator, context, instruction);

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
    if (instruction->lhs.kind != IR_OPERAND_NONE &&
        !code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->lhs,
                                                 BINARY_GP_RAX)) {
      return 0;
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

  default:
    code_generator_set_error(
        generator,
        "Direct object backend does not yet support IR opcode %d in "
        "function '%s'",
        (int)instruction->op, context->function_name);
    return 0;
  }
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

  if (context->frame_size > 0 &&
      !binary_emit_sub_rsp_imm32(&context->code, (uint32_t)context->frame_size)) {
    code_generator_set_error(generator,
                             "Out of memory while allocating stack frame");
    return 0;
  }

  for (size_t i = 0; i < function_data->parameter_count; i++) {
    const char *parameter_name = function_data->parameter_names[i];
    int parameter_is_float64 = code_generator_binary_named_type_is_float64(
        generator,
        function_data->parameter_types ? function_data->parameter_types[i] : NULL,
        0);
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

    if (i < BINARY_WIN64_REGISTER_ARG_COUNT) {
      if ((parameter_is_float64 &&
           (!binary_emit_movq_reg_xmm(&context->code, BINARY_GP_RAX,
                                      BINARY_WIN64_FLOAT_PARAM_REGISTERS[i]) ||
            !binary_emit_mov_mem_reg(&context->code, BINARY_GP_RBP,
                                     -home_offset, BINARY_GP_RAX))) ||
          (!parameter_is_float64 &&
           !binary_emit_mov_mem_reg(&context->code, BINARY_GP_RBP, -home_offset,
                                    BINARY_WIN64_INT_PARAM_REGISTERS[i]))) {
        code_generator_set_error(generator,
                                 "Out of memory while homing parameters");
        return 0;
      }
    } else {
      int incoming_stack_offset =
          16 + BINARY_WIN64_SHADOW_SPACE_SIZE +
          (int)((i - BINARY_WIN64_REGISTER_ARG_COUNT) *
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
                                                ir_function) ||
      !code_generator_binary_prepare_function_context(generator, function_data,
                                                      ir_function, &context)) {
    return 0;
  }

  if (!code_generator_binary_emit_prologue(generator, &context, function_data)) {
    binary_function_context_destroy(&context);
    return 0;
  }

  for (size_t i = 0; i < ir_function->instruction_count; i++) {
    if (!code_generator_binary_emit_instruction(
            generator, &context, &ir_function->instructions[i])) {
      binary_function_context_destroy(&context);
      return 0;
    }
  }

  return_offset = context.code.size;
  if ((context.return_is_float64 &&
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
    if (var_data->initializer->type != AST_NUMBER_LITERAL) {
      code_generator_set_error(
          generator,
          "Direct object backend only supports numeric global initializers "
          "(encountered '%s')",
          var_data->name);
      return 0;
    }

    NumberLiteral *literal = (NumberLiteral *)var_data->initializer->data;
    if (!literal) {
      code_generator_set_error(generator,
                               "Malformed numeric global initializer '%s'",
                               var_data->name);
      return 0;
    }

    if (code_generator_binary_resolved_type_is_float64(type)) {
      union {
        double value;
        uint64_t bits;
      } encoded = {0};
      encoded.value = literal->is_float ? literal->float_value
                                        : (double)literal->int_value;
      memcpy(bytes, &encoded.bits, sizeof(encoded.bits));
    } else {
      uint64_t encoded = (uint64_t)literal->int_value;
      if (literal->is_float) {
        code_generator_set_error(
            generator,
            "Direct object backend only supports float global initializers for "
            "float64 globals (encountered '%s')",
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

  return generator->has_error ? 0 : 1;
}
