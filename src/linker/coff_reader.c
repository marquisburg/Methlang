#include "linker/coff_reader.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COFF_FILE_HEADER_SIZE 20u
#define COFF_SECTION_HEADER_SIZE 40u
#define COFF_SYMBOL_SIZE 18u
#define COFF_RELOCATION_SIZE 10u

#define COFF_RELOC_AMD64_ADDR64 0x0001u
#define COFF_RELOC_AMD64_ADDR32NB 0x0003u
#define COFF_RELOC_AMD64_REL32 0x0004u
#define COFF_RELOC_AMD64_SECREL 0x000Bu

static char *coff_reader_strdup(const char *value) {
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

static char *coff_reader_dup_bytes_trimmed(const unsigned char *bytes,
                                           size_t byte_count) {
  size_t length = byte_count;
  char *copy = NULL;

  while (length > 0 && bytes[length - 1] == '\0') {
    length--;
  }

  copy = malloc(length + 1);
  if (!copy) {
    return NULL;
  }

  if (length > 0) {
    memcpy(copy, bytes, length);
  }
  copy[length] = '\0';
  return copy;
}

static char *coff_reader_dup_cstring_range(const unsigned char *bytes,
                                           size_t byte_count) {
  size_t length = 0;
  char *copy = NULL;

  while (length < byte_count && bytes[length] != '\0') {
    length++;
  }
  if (length == byte_count) {
    return NULL;
  }

  copy = malloc(length + 1);
  if (!copy) {
    return NULL;
  }

  if (length > 0) {
    memcpy(copy, bytes, length);
  }
  copy[length] = '\0';
  return copy;
}

static void coff_reader_set_error(char **error_message_out, const char *format,
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

  copy = coff_reader_strdup(buffer);
  if (!copy) {
    return;
  }

  free(*error_message_out);
  *error_message_out = copy;
}

static uint16_t coff_reader_u16(const unsigned char *data) {
  return (uint16_t)(data[0] | ((uint16_t)data[1] << 8));
}

static int16_t coff_reader_i16(const unsigned char *data) {
  return (int16_t)coff_reader_u16(data);
}

static uint32_t coff_reader_u32(const unsigned char *data) {
  return (uint32_t)(data[0] | ((uint32_t)data[1] << 8) |
                    ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24));
}

static int coff_reader_read_file(const char *filename, unsigned char **data_out,
                                 size_t *size_out,
                                 char **error_message_out) {
  FILE *file = NULL;
  long file_size = 0;
  size_t bytes_read = 0;
  unsigned char *data = NULL;

  if (!filename || !data_out || !size_out) {
    coff_reader_set_error(error_message_out,
                          "Invalid arguments while reading COFF file");
    return 0;
  }

  file = fopen(filename, "rb");
  if (!file) {
    coff_reader_set_error(error_message_out, "Failed to open '%s': %s",
                          filename, strerror(errno));
    return 0;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    coff_reader_set_error(error_message_out,
                          "Failed to seek to end of '%s'", filename);
    fclose(file);
    return 0;
  }

  file_size = ftell(file);
  if (file_size < 0) {
    coff_reader_set_error(error_message_out,
                          "Failed to determine size of '%s'", filename);
    fclose(file);
    return 0;
  }

  if (fseek(file, 0, SEEK_SET) != 0) {
    coff_reader_set_error(error_message_out,
                          "Failed to rewind '%s'", filename);
    fclose(file);
    return 0;
  }

  data = malloc((size_t)file_size);
  if (!data && file_size != 0) {
    coff_reader_set_error(error_message_out,
                          "Out of memory while loading '%s'", filename);
    fclose(file);
    return 0;
  }

  bytes_read = fread(data, 1, (size_t)file_size, file);
  fclose(file);
  if (bytes_read != (size_t)file_size) {
    free(data);
    coff_reader_set_error(error_message_out, "Failed to read '%s'", filename);
    return 0;
  }

  *data_out = data;
  *size_out = (size_t)file_size;
  return 1;
}

static int coff_reader_range_ok(size_t file_size, uint32_t offset,
                                size_t length) {
  size_t start = (size_t)offset;

  if (start > file_size) {
    return 0;
  }
  if (length > file_size - start) {
    return 0;
  }

  return 1;
}

static char *coff_reader_string_from_table(const CoffObject *object,
                                           uint32_t offset) {
  size_t remaining = 0;

  if (!object || !object->string_table || object->string_table_size < 4u) {
    return NULL;
  }
  if (offset < 4u || offset >= object->string_table_size) {
    return NULL;
  }

  remaining = (size_t)(object->string_table_size - offset);
  return coff_reader_dup_cstring_range(object->string_table + offset,
                                       remaining);
}

static char *coff_reader_parse_section_name(const unsigned char field[8],
                                            const CoffObject *object) {
  uint32_t offset = 0;
  size_t i = 0;

  if (field[0] == '/') {
    for (i = 1; i < 8 && field[i] != '\0'; i++) {
      if (!isdigit((unsigned char)field[i])) {
        return coff_reader_dup_bytes_trimmed(field, 8);
      }
      offset = (offset * 10u) + (uint32_t)(field[i] - '0');
    }
    return coff_reader_string_from_table(object, offset);
  }

  return coff_reader_dup_bytes_trimmed(field, 8);
}

static char *coff_reader_parse_symbol_name(const unsigned char field[8],
                                           const CoffObject *object) {
  uint32_t zero_prefix = coff_reader_u32(field);
  uint32_t offset = coff_reader_u32(field + 4);

  if (zero_prefix == 0u && offset != 0u) {
    return coff_reader_string_from_table(object, offset);
  }

  return coff_reader_dup_bytes_trimmed(field, 8);
}

static int coff_reader_parse_string_table(CoffObject *object,
                                          const unsigned char *file_data,
                                          size_t file_size,
                                          char **error_message_out) {
  size_t string_table_offset = 0;
  uint32_t string_table_size = 0;

  if (!object) {
    return 0;
  }

  string_table_offset =
      (size_t)object->pointer_to_symbol_table +
      ((size_t)object->symbol_count * COFF_SYMBOL_SIZE);
  if (string_table_offset > file_size) {
    coff_reader_set_error(error_message_out,
                          "COFF string table offset is out of range");
    return 0;
  }
  if (string_table_offset == file_size) {
    return 1;
  }
  if (!coff_reader_range_ok(file_size, (uint32_t)string_table_offset, 4u)) {
    coff_reader_set_error(error_message_out,
                          "COFF string table header is truncated");
    return 0;
  }

  string_table_size = coff_reader_u32(file_data + string_table_offset);
  if (string_table_size == 0u) {
    return 1;
  }
  if (string_table_size < 4u) {
    coff_reader_set_error(error_message_out,
                          "COFF string table size is invalid");
    return 0;
  }
  if (!coff_reader_range_ok(file_size, (uint32_t)string_table_offset,
                            string_table_size)) {
    coff_reader_set_error(error_message_out,
                          "COFF string table extends past end of file");
    return 0;
  }

  object->string_table = malloc(string_table_size);
  if (!object->string_table) {
    coff_reader_set_error(error_message_out,
                          "Out of memory while copying COFF string table");
    return 0;
  }

  memcpy(object->string_table, file_data + string_table_offset,
         string_table_size);
  object->string_table_size = string_table_size;
  return 1;
}

static int coff_reader_parse_sections(CoffObject *object,
                                      const unsigned char *file_data,
                                      size_t file_size,
                                      char **error_message_out) {
  size_t section_table_offset = COFF_FILE_HEADER_SIZE;
  size_t i = 0;

  if (!object) {
    return 0;
  }
  if (object->section_count == 0u) {
    return 1;
  }
  if (!coff_reader_range_ok(file_size, (uint32_t)section_table_offset,
                            (size_t)object->section_count *
                                COFF_SECTION_HEADER_SIZE)) {
    coff_reader_set_error(error_message_out,
                          "COFF section table is truncated");
    return 0;
  }

  object->sections = calloc(object->section_count, sizeof(CoffSection));
  if (!object->sections) {
    coff_reader_set_error(error_message_out,
                          "Out of memory while allocating COFF sections");
    return 0;
  }

  for (i = 0; i < object->section_count; i++) {
    const unsigned char *header =
        file_data + section_table_offset + (i * COFF_SECTION_HEADER_SIZE);
    CoffSection *section = &object->sections[i];

    section->name = coff_reader_parse_section_name(header, object);
    if (!section->name) {
      coff_reader_set_error(error_message_out,
                            "Failed to resolve COFF section name %zu", i + 1u);
      return 0;
    }

    section->kind = coff_section_kind_from_name(section->name);
    section->virtual_size = coff_reader_u32(header + 8);
    section->virtual_address = coff_reader_u32(header + 12);
    section->size_of_raw_data = coff_reader_u32(header + 16);
    section->pointer_to_raw_data = coff_reader_u32(header + 20);
    section->pointer_to_relocations = coff_reader_u32(header + 24);
    section->pointer_to_line_numbers = coff_reader_u32(header + 28);
    section->number_of_relocations = coff_reader_u16(header + 32);
    section->number_of_line_numbers = coff_reader_u16(header + 34);
    section->characteristics = coff_reader_u32(header + 36);

    if (section->size_of_raw_data > 0u) {
      if (!coff_reader_range_ok(file_size, section->pointer_to_raw_data,
                                section->size_of_raw_data)) {
        coff_reader_set_error(error_message_out,
                              "Section '%s' raw data is out of range",
                              section->name);
        return 0;
      }

      section->raw_data = malloc(section->size_of_raw_data);
      if (!section->raw_data) {
        coff_reader_set_error(error_message_out,
                              "Out of memory while copying section '%s'",
                              section->name);
        return 0;
      }

      memcpy(section->raw_data, file_data + section->pointer_to_raw_data,
             section->size_of_raw_data);
    }

    if (section->number_of_relocations > 0u) {
      size_t relocation_bytes =
          (size_t)section->number_of_relocations * COFF_RELOCATION_SIZE;
      size_t r = 0;

      if (!coff_reader_range_ok(file_size, section->pointer_to_relocations,
                                relocation_bytes)) {
        coff_reader_set_error(error_message_out,
                              "Section '%s' relocation table is out of range",
                              section->name);
        return 0;
      }

      section->relocations =
          calloc(section->number_of_relocations, sizeof(CoffRelocation));
      if (!section->relocations) {
        coff_reader_set_error(error_message_out,
                              "Out of memory while allocating relocations for "
                              "section '%s'",
                              section->name);
        return 0;
      }

      section->relocation_count = section->number_of_relocations;
      for (r = 0; r < section->relocation_count; r++) {
        const unsigned char *relocation =
            file_data + section->pointer_to_relocations +
            (r * COFF_RELOCATION_SIZE);

        section->relocations[r].virtual_address = coff_reader_u32(relocation);
        section->relocations[r].symbol_table_index =
            coff_reader_u32(relocation + 4);
        section->relocations[r].type = coff_reader_u16(relocation + 8);
      }
    }
  }

  return 1;
}

static int coff_reader_parse_symbols(CoffObject *object,
                                     const unsigned char *file_data,
                                     size_t file_size,
                                     char **error_message_out) {
  size_t symbol_table_bytes = 0;
  size_t i = 0;
  size_t aux_remaining = 0;
  uint32_t primary_symbol_index = 0;

  if (!object) {
    return 0;
  }
  if (object->symbol_count == 0u) {
    return 1;
  }

  symbol_table_bytes = (size_t)object->symbol_count * COFF_SYMBOL_SIZE;
  if (!coff_reader_range_ok(file_size, object->pointer_to_symbol_table,
                            symbol_table_bytes)) {
    coff_reader_set_error(error_message_out,
                          "COFF symbol table is out of range");
    return 0;
  }

  object->symbols = calloc(object->symbol_count, sizeof(CoffSymbol));
  if (!object->symbols) {
    coff_reader_set_error(error_message_out,
                          "Out of memory while allocating COFF symbols");
    return 0;
  }

  for (i = 0; i < object->symbol_count; i++) {
    const unsigned char *entry =
        file_data + object->pointer_to_symbol_table + (i * COFF_SYMBOL_SIZE);
    CoffSymbol *symbol = &object->symbols[i];

    symbol->raw_index = (uint32_t)i;
    symbol->primary_symbol_index = (uint32_t)i;

    if (aux_remaining > 0u) {
      symbol->is_auxiliary = 1;
      symbol->primary_symbol_index = primary_symbol_index;
      aux_remaining--;
      continue;
    }

    symbol->name = coff_reader_parse_symbol_name(entry, object);
    if (!symbol->name && (entry[0] != 0 || entry[1] != 0 || entry[2] != 0 ||
                          entry[3] != 0 || entry[4] != 0 || entry[5] != 0 ||
                          entry[6] != 0 || entry[7] != 0)) {
      coff_reader_set_error(error_message_out,
                            "Failed to resolve COFF symbol name at index %zu",
                            i);
      return 0;
    }

    symbol->value = coff_reader_u32(entry + 8);
    symbol->section_number = coff_reader_i16(entry + 12);
    symbol->type = coff_reader_u16(entry + 14);
    symbol->storage_class = entry[16];
    symbol->auxiliary_count = entry[17];

    if ((size_t)symbol->auxiliary_count > object->symbol_count - i - 1u) {
      coff_reader_set_error(error_message_out,
                            "Symbol '%s' has truncated auxiliary records",
                            symbol->name ? symbol->name : "<unnamed>");
      return 0;
    }

    if (symbol->auxiliary_count > 0u) {
      const unsigned char *aux_entry = entry + COFF_SYMBOL_SIZE;

      symbol->has_auxiliary_record = 1;
      symbol->aux_section_length = coff_reader_u32(aux_entry);
      symbol->aux_section_relocation_count = coff_reader_u16(aux_entry + 4);
      symbol->aux_section_line_number_count = coff_reader_u16(aux_entry + 6);
      aux_remaining = symbol->auxiliary_count;
      primary_symbol_index = (uint32_t)i;
    }
  }

  return 1;
}

int coff_object_read(const char *filename, CoffObject **object_out,
                     char **error_message_out) {
  unsigned char *file_data = NULL;
  size_t file_size = 0;
  CoffObject *object = NULL;
  int ok = 0;

  if (object_out) {
    *object_out = NULL;
  }
  if (error_message_out) {
    free(*error_message_out);
    *error_message_out = NULL;
  }

  if (!filename || !object_out) {
    coff_reader_set_error(error_message_out,
                          "Invalid arguments while parsing COFF object");
    return 0;
  }

  if (!coff_reader_read_file(filename, &file_data, &file_size,
                             error_message_out)) {
    return 0;
  }

  if (file_size < COFF_FILE_HEADER_SIZE) {
    coff_reader_set_error(error_message_out,
                          "COFF file '%s' is smaller than the file header",
                          filename);
    goto cleanup;
  }

  object = calloc(1, sizeof(CoffObject));
  if (!object) {
    coff_reader_set_error(error_message_out,
                          "Out of memory while creating COFF object");
    goto cleanup;
  }

  object->machine = coff_reader_u16(file_data);
  object->section_count = coff_reader_u16(file_data + 2);
  object->time_date_stamp = coff_reader_u32(file_data + 4);
  object->pointer_to_symbol_table = coff_reader_u32(file_data + 8);
  object->symbol_count = coff_reader_u32(file_data + 12);
  object->size_of_optional_header = coff_reader_u16(file_data + 16);
  object->characteristics = coff_reader_u16(file_data + 18);

  if (object->machine != COFF_MACHINE_AMD64) {
    coff_reader_set_error(error_message_out,
                          "Unsupported COFF machine 0x%04X (expected 0x%04X)",
                          object->machine, COFF_MACHINE_AMD64);
    goto cleanup;
  }

  if (!coff_reader_parse_string_table(object, file_data, file_size,
                                      error_message_out) ||
      !coff_reader_parse_sections(object, file_data, file_size,
                                  error_message_out) ||
      !coff_reader_parse_symbols(object, file_data, file_size,
                                 error_message_out)) {
    goto cleanup;
  }

  ok = 1;

cleanup:
  free(file_data);
  if (!ok) {
    coff_object_destroy(object);
    return 0;
  }

  *object_out = object;
  return 1;
}

void coff_object_destroy(CoffObject *object) {
  size_t i = 0;

  if (!object) {
    return;
  }

  if (object->sections) {
    for (i = 0; i < object->section_count; i++) {
      free(object->sections[i].name);
      free(object->sections[i].raw_data);
      free(object->sections[i].relocations);
    }
  }

  if (object->symbols) {
    for (i = 0; i < object->symbol_count; i++) {
      free(object->symbols[i].name);
    }
  }

  free(object->sections);
  free(object->symbols);
  free(object->string_table);
  free(object);
}

const CoffSection *coff_object_find_section_by_kind(const CoffObject *object,
                                                    CoffSectionKind kind) {
  size_t i = 0;

  if (!object) {
    return NULL;
  }

  for (i = 0; i < object->section_count; i++) {
    if (object->sections[i].kind == kind) {
      return &object->sections[i];
    }
  }

  return NULL;
}

const CoffSymbol *coff_object_find_symbol(const CoffObject *object,
                                          const char *name) {
  size_t i = 0;

  if (!object || !name) {
    return NULL;
  }

  for (i = 0; i < object->symbol_count; i++) {
    const CoffSymbol *symbol = &object->symbols[i];
    if (symbol->is_auxiliary || !symbol->name) {
      continue;
    }
    if (strcmp(symbol->name, name) == 0) {
      return symbol;
    }
  }

  return NULL;
}

static int coff_section_name_matches_prefix(const char *name,
                                            const char *prefix) {
  size_t prefix_length = 0u;
  char suffix = '\0';

  if (!name || !prefix) {
    return 0;
  }

  prefix_length = strlen(prefix);
  if (strncmp(name, prefix, prefix_length) != 0) {
    return 0;
  }

  suffix = name[prefix_length];
  return suffix == '\0' || suffix == '$' || suffix == '.';
}

CoffSectionKind coff_section_kind_from_name(const char *name) {
  if (!name || name[0] != '.') {
    return COFF_SECTION_KIND_UNKNOWN;
  }

  if (coff_section_name_matches_prefix(name, ".text")) {
    return COFF_SECTION_KIND_TEXT;
  }
  if (coff_section_name_matches_prefix(name, ".rdata")) {
    return COFF_SECTION_KIND_RDATA;
  }
  if (coff_section_name_matches_prefix(name, ".data")) {
    return COFF_SECTION_KIND_DATA;
  }
  if (coff_section_name_matches_prefix(name, ".bss")) {
    return COFF_SECTION_KIND_BSS;
  }

  return COFF_SECTION_KIND_UNKNOWN;
}

const char *coff_section_kind_name(CoffSectionKind kind) {
  switch (kind) {
  case COFF_SECTION_KIND_TEXT:
    return ".text";
  case COFF_SECTION_KIND_RDATA:
    return ".rdata";
  case COFF_SECTION_KIND_DATA:
    return ".data";
  case COFF_SECTION_KIND_BSS:
    return ".bss";
  case COFF_SECTION_KIND_UNKNOWN:
  default:
    return "<unknown>";
  }
}

const char *coff_relocation_type_name(uint16_t type) {
  switch (type) {
  case COFF_RELOC_AMD64_ADDR64:
    return "ADDR64";
  case COFF_RELOC_AMD64_ADDR32NB:
    return "ADDR32NB";
  case COFF_RELOC_AMD64_REL32:
    return "REL32";
  case COFF_RELOC_AMD64_SECREL:
    return "SECREL";
  default:
    return "UNKNOWN";
  }
}
