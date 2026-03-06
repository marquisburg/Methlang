#include "linker/relocation.h"
#include "linker/symbol_resolve.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COFF_RELOC_AMD64_ADDR64 0x0001u
#define COFF_RELOC_AMD64_ADDR32NB 0x0003u
#define COFF_RELOC_AMD64_REL32 0x0004u
#define COFF_RELOC_AMD64_SECREL 0x000Bu

typedef struct {
  const char *name;
  size_t merged_section_index;
  size_t merged_offset;
  uint64_t virtual_address;
} RelocationTarget;

static char *relocation_strdup(const char *value) {
  size_t length = 0;
  char *copy = NULL;

  if (!value) {
    return NULL;
  }

  length = strlen(value);
  copy = malloc(length + 1);
  if (!copy) {
    return NULL;
  }

  memcpy(copy, value, length + 1);
  return copy;
}

static void relocation_set_error(char **error_message_out, const char *format,
                                 ...) {
  char buffer[512];
  va_list args;
  char *copy = NULL;

  if (!error_message_out) {
    return;
  }

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  copy = relocation_strdup(buffer);
  if (!copy) {
    return;
  }

  free(*error_message_out);
  *error_message_out = copy;
}

static uint32_t relocation_read_u32(const unsigned char *bytes) {
  return (uint32_t)(bytes[0] | ((uint32_t)bytes[1] << 8) |
                    ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24));
}

static uint64_t relocation_read_u64(const unsigned char *bytes) {
  return (uint64_t)bytes[0] | ((uint64_t)bytes[1] << 8) |
         ((uint64_t)bytes[2] << 16) | ((uint64_t)bytes[3] << 24) |
         ((uint64_t)bytes[4] << 32) | ((uint64_t)bytes[5] << 40) |
         ((uint64_t)bytes[6] << 48) | ((uint64_t)bytes[7] << 56);
}

static void relocation_write_u32(unsigned char *bytes, uint32_t value) {
  bytes[0] = (unsigned char)(value & 0xFFu);
  bytes[1] = (unsigned char)((value >> 8) & 0xFFu);
  bytes[2] = (unsigned char)((value >> 16) & 0xFFu);
  bytes[3] = (unsigned char)((value >> 24) & 0xFFu);
}

static void relocation_write_u64(unsigned char *bytes, uint64_t value) {
  bytes[0] = (unsigned char)(value & 0xFFu);
  bytes[1] = (unsigned char)((value >> 8) & 0xFFu);
  bytes[2] = (unsigned char)((value >> 16) & 0xFFu);
  bytes[3] = (unsigned char)((value >> 24) & 0xFFu);
  bytes[4] = (unsigned char)((value >> 32) & 0xFFu);
  bytes[5] = (unsigned char)((value >> 40) & 0xFFu);
  bytes[6] = (unsigned char)((value >> 48) & 0xFFu);
  bytes[7] = (unsigned char)((value >> 56) & 0xFFu);
}

static int relocation_resolve_target(const LinkResolution *resolution,
                                     const LinkedInputObject *input,
                                     uint32_t symbol_index,
                                     RelocationTarget *target_out,
                                     char **error_message_out) {
  const LinkedObjectSymbol *object_symbol = NULL;
  const LinkedSymbol *global_symbol = NULL;

  if (!resolution || !input || !target_out) {
    return 0;
  }
  if (symbol_index >= input->symbol_count) {
    relocation_set_error(error_message_out,
                         "Relocation refers to symbol index %u outside the "
                         "object symbol table",
                         symbol_index);
    return 0;
  }

  object_symbol = &input->symbols[symbol_index];
  if (object_symbol->name) {
    target_out->name = object_symbol->name;
  } else {
    target_out->name = "<unnamed>";
  }

  if (object_symbol->is_auxiliary) {
    relocation_set_error(error_message_out,
                         "Relocation refers to auxiliary symbol '%s'",
                         target_out->name);
    return 0;
  }

  if (object_symbol->is_defined &&
      object_symbol->merged_section_index != LINKED_SECTION_INDEX_NONE) {
    target_out->merged_section_index = object_symbol->merged_section_index;
    target_out->merged_offset = object_symbol->merged_offset;
    target_out->virtual_address = object_symbol->virtual_address;
    return 1;
  }

  global_symbol = link_resolution_find_symbol(resolution, object_symbol->name);
  if (!global_symbol || !global_symbol->is_defined ||
      global_symbol->merged_section_index == LINKED_SECTION_INDEX_NONE) {
    relocation_set_error(error_message_out,
                         "Relocation target '%s' is unresolved",
                         target_out->name);
    return 0;
  }

  target_out->merged_section_index = global_symbol->merged_section_index;
  target_out->merged_offset = global_symbol->merged_offset;
  target_out->virtual_address = global_symbol->virtual_address;
  return 1;
}

int link_apply_relocations(LinkResolution *resolution,
                           const LinkRelocationOptions *options,
                           char **error_message_out) {
  uint64_t image_base = 0;
  size_t object_index = 0;

  if (error_message_out) {
    free(*error_message_out);
    *error_message_out = NULL;
  }
  if (!resolution) {
    relocation_set_error(error_message_out,
                         "Link resolution is required before applying relocations");
    return 0;
  }
  if (options) {
    image_base = options->image_base;
  }
  (void)image_base;

  for (object_index = 0; object_index < resolution->object_count;
       object_index++) {
    const LinkedInputObject *input = &resolution->objects[object_index];
    size_t section_index = 0;

    if (!input->object) {
      continue;
    }

    for (section_index = 0; section_index < input->object->section_count;
         section_index++) {
      const CoffSection *source_section = &input->object->sections[section_index];
      size_t merged_section_index = LINKED_SECTION_INDEX_NONE;
      size_t relocation_index = 0;

      if (section_index < input->object->section_count) {
        merged_section_index = input->section_merged_indices[section_index];
      }
      if (source_section->relocation_count == 0u) {
        continue;
      }
      if (merged_section_index == LINKED_SECTION_INDEX_NONE) {
        relocation_set_error(error_message_out,
                             "Section '%s' has relocations but was not merged",
                             source_section->name ? source_section->name
                                                  : "<unknown>");
        return 0;
      }

      for (relocation_index = 0;
           relocation_index < source_section->relocation_count;
           relocation_index++) {
        const CoffRelocation *relocation =
            &source_section->relocations[relocation_index];
        LinkedSection *merged = &resolution->sections[merged_section_index];
        RelocationTarget target = {0};
        size_t patch_offset = input->section_merged_offsets[section_index] +
                              (size_t)relocation->virtual_address;
        uint64_t patch_address = merged->virtual_address + patch_offset;
        int64_t addend = 0;
        int64_t value = 0;
        size_t width = 0;

        if (!relocation_resolve_target(resolution, input,
                                       relocation->symbol_table_index, &target,
                                       error_message_out)) {
          return 0;
        }

        switch (relocation->type) {
        case COFF_RELOC_AMD64_ADDR64:
          width = 8u;
          break;
        case COFF_RELOC_AMD64_ADDR32NB:
        case COFF_RELOC_AMD64_REL32:
        case COFF_RELOC_AMD64_SECREL:
          width = 4u;
          break;
        default:
          relocation_set_error(error_message_out,
                               "Unsupported relocation type %s for symbol '%s'",
                               coff_relocation_type_name(relocation->type),
                               target.name);
          return 0;
        }

        if (patch_offset + width > merged->size) {
          relocation_set_error(error_message_out,
                               "Relocation for symbol '%s' writes past merged "
                               "section '%s'",
                               target.name, merged->name);
          return 0;
        }

        if (width == 8u) {
          addend = (int64_t)relocation_read_u64(merged->data + patch_offset);
        } else {
          addend = (int64_t)(int32_t)relocation_read_u32(merged->data + patch_offset);
        }

        switch (relocation->type) {
        case COFF_RELOC_AMD64_REL32:
          value = (int64_t)target.virtual_address + addend -
                  (int64_t)(patch_address + 4u);
          if (value < INT32_MIN || value > INT32_MAX) {
            relocation_set_error(error_message_out,
                                 "REL32 relocation for symbol '%s' is out of range",
                                 target.name);
            return 0;
          }
          relocation_write_u32(merged->data + patch_offset, (uint32_t)(int32_t)value);
          break;
        case COFF_RELOC_AMD64_ADDR64:
          value = (int64_t)target.virtual_address + addend;
          if (value < 0) {
            relocation_set_error(error_message_out,
                                 "ADDR64 relocation for symbol '%s' is negative",
                                 target.name);
            return 0;
          }
          relocation_write_u64(merged->data + patch_offset, (uint64_t)value);
          break;
        case COFF_RELOC_AMD64_ADDR32NB:
          if (image_base != 0u && target.virtual_address >= image_base) {
            value = (int64_t)(target.virtual_address - image_base) + addend;
          } else {
            value = (int64_t)target.virtual_address + addend;
          }
          if (value < 0 || (uint64_t)value > UINT32_MAX) {
            relocation_set_error(error_message_out,
                                 "ADDR32NB relocation for symbol '%s' is out of range",
                                 target.name);
            return 0;
          }
          relocation_write_u32(merged->data + patch_offset, (uint32_t)value);
          break;
        case COFF_RELOC_AMD64_SECREL:
          value = (int64_t)target.merged_offset + addend;
          if (value < 0 || (uint64_t)value > UINT32_MAX) {
            relocation_set_error(error_message_out,
                                 "SECREL relocation for symbol '%s' is out of range",
                                 target.name);
            return 0;
          }
          relocation_write_u32(merged->data + patch_offset, (uint32_t)value);
          break;
        default:
          break;
        }
      }
    }
  }

  return 1;
}
