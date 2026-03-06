#ifndef BINARY_EMITTER_H
#define BINARY_EMITTER_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
  BINARY_TARGET_FORMAT_COFF_WIN64 = 0,
} BinaryTargetFormat;

typedef enum {
  BINARY_SECTION_TEXT = 0,
  BINARY_SECTION_RDATA,
  BINARY_SECTION_DATA,
  BINARY_SECTION_BSS,
} BinarySectionKind;

typedef enum {
  BINARY_SYMBOL_LOCAL = 0,
  BINARY_SYMBOL_GLOBAL,
  BINARY_SYMBOL_EXTERNAL,
} BinarySymbolBinding;

typedef enum {
  BINARY_RELOCATION_REL32 = 0,
  BINARY_RELOCATION_ADDR64,
  BINARY_RELOCATION_ADDR32NB,
  BINARY_RELOCATION_SECTION_REL32,
} BinaryRelocationKind;

typedef struct {
  char *name;
  BinarySectionKind kind;
  uint32_t characteristics;
  size_t alignment;
  unsigned char *data;
  size_t size;
  size_t capacity;
  size_t virtual_size;
} BinarySection;

typedef struct {
  char *name;
  BinarySymbolBinding binding;
  size_t section_index;
  size_t value;
  size_t size;
} BinarySymbol;

typedef struct {
  size_t section_index;
  size_t offset;
  BinaryRelocationKind kind;
  char *symbol_name;
  int32_t addend;
} BinaryRelocation;

typedef struct {
  BinaryTargetFormat target_format;
  BinarySection *sections;
  size_t section_count;
  size_t section_capacity;
  BinarySymbol *symbols;
  size_t symbol_count;
  size_t symbol_capacity;
  BinaryRelocation *relocations;
  size_t relocation_count;
  size_t relocation_capacity;
  char *error_message;
} BinaryEmitter;

BinaryEmitter *binary_emitter_create(BinaryTargetFormat target_format);
void binary_emitter_destroy(BinaryEmitter *emitter);
void binary_emitter_reset(BinaryEmitter *emitter);

size_t binary_emitter_get_or_create_section(BinaryEmitter *emitter,
                                            const char *name,
                                            BinarySectionKind kind,
                                            uint32_t characteristics,
                                            size_t alignment);
BinarySection *binary_emitter_get_section(BinaryEmitter *emitter,
                                          size_t section_index);
const BinarySection *binary_emitter_get_section_const(
    const BinaryEmitter *emitter, size_t section_index);

int binary_emitter_align_section(BinaryEmitter *emitter, size_t section_index,
                                 size_t alignment, unsigned char fill_byte);
int binary_emitter_append_bytes(BinaryEmitter *emitter, size_t section_index,
                                const void *data, size_t size,
                                size_t *offset_out);
int binary_emitter_append_zeros(BinaryEmitter *emitter, size_t section_index,
                                size_t size, size_t *offset_out);
int binary_emitter_set_section_virtual_size(BinaryEmitter *emitter,
                                            size_t section_index,
                                            size_t virtual_size);

int binary_emitter_define_symbol(BinaryEmitter *emitter, const char *name,
                                 BinarySymbolBinding binding,
                                 size_t section_index, size_t value,
                                 size_t size);
int binary_emitter_declare_external(BinaryEmitter *emitter, const char *name);
const BinarySymbol *binary_emitter_find_symbol(const BinaryEmitter *emitter,
                                               const char *name);

int binary_emitter_add_relocation(BinaryEmitter *emitter, size_t section_index,
                                  size_t offset, BinaryRelocationKind kind,
                                  const char *symbol_name, int32_t addend);

const char *binary_emitter_get_error(const BinaryEmitter *emitter);
int binary_emitter_write_object_file(BinaryEmitter *emitter,
                                     const char *filename);

#endif // BINARY_EMITTER_H
