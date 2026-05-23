#include "codegen/binary/internal.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int code_generator_binary_emit_profile_cstring_table(
    CodeGenerator *generator, BinaryEmitter *emitter, size_t rdata_section,
    const char *table_symbol, const char *const *values, size_t count,
    char **out_name_symbols) {
  size_t table_offset = 0;

  if (!binary_emitter_align_section(emitter, rdata_section, 8, 0)) {
    return 0;
  }

  if (!binary_emitter_append_zeros(emitter, rdata_section, count * sizeof(uint64_t),
                                   &table_offset) ||
      !binary_emitter_define_symbol(emitter, table_symbol, BINARY_SYMBOL_GLOBAL,
                                    rdata_section, table_offset,
                                    count * sizeof(uint64_t))) {
    return 0;
  }

  for (size_t i = 0; i < count; i++) {
    const char *value = values[i] ? values[i] : "?";
    size_t literal_offset = 0;
    size_t length = strlen(value);
    unsigned char terminator = 0;
    int written = 0;

    written = snprintf(NULL, 0, "methprof_%s_%zu", table_symbol, i);
    if (written <= 0) {
      return 0;
    }

    out_name_symbols[i] = malloc((size_t)written + 1u);
    if (!out_name_symbols[i]) {
      return 0;
    }
    snprintf(out_name_symbols[i], (size_t)written + 1u, "methprof_%s_%zu",
             table_symbol, i);

    if (!binary_emitter_append_bytes(emitter, rdata_section, value, length,
                                     &literal_offset) ||
        !binary_emitter_append_bytes(emitter, rdata_section, &terminator, 1,
                                     NULL) ||
        !binary_emitter_define_symbol(emitter, out_name_symbols[i],
                                      BINARY_SYMBOL_LOCAL, rdata_section,
                                      literal_offset, length + 1) ||
        !binary_emitter_add_relocation(
            emitter, rdata_section, table_offset + i * sizeof(uint64_t),
            BINARY_RELOCATION_ADDR64, out_name_symbols[i], 0)) {
      return 0;
    }
  }

  return 1;
}

int code_generator_binary_emit_profile_tables(CodeGenerator *generator) {
  BinaryEmitter *emitter = NULL;
  IRProgram *program = NULL;
  size_t rdata_section = 0;
  size_t data_section = 0;
  size_t function_count = 0;
  char **name_symbols = NULL;
  char **file_symbols = NULL;
  const char **profile_names = NULL;
  const char **profile_files = NULL;
  uint64_t *profile_lines = NULL;

  if (!generator || !generator->profile_runtime || !generator->ir_program) {
    return 1;
  }

  program = generator->ir_program;
  function_count = program->profile_entry_count;
  if (function_count == 0) {
    return 1;
  }

  profile_names = calloc(function_count, sizeof(const char *));
  profile_files = calloc(function_count, sizeof(const char *));
  profile_lines = calloc(function_count, sizeof(uint64_t));
  name_symbols = calloc(function_count, sizeof(char *));
  file_symbols = calloc(function_count, sizeof(char *));
  if (!profile_names || !profile_files || !profile_lines || !name_symbols ||
      !file_symbols) {
    free(profile_names);
    free(profile_files);
    free(profile_lines);
    free(name_symbols);
    free(file_symbols);
    code_generator_set_error(generator,
                             "Out of memory while emitting profile tables");
    return 0;
  }

  for (size_t i = 0; i < function_count; i++) {
    IRProfileEntry *entry = &program->profile_entries[i];
    profile_names[i] = entry->name ? entry->name : "?";
    profile_files[i] = entry->filename ? entry->filename : "?";
    profile_lines[i] = entry->line;
  }

  emitter = code_generator_get_binary_emitter(generator);
  if (!emitter) {
    code_generator_set_error(generator, "Binary emitter is not initialized");
    goto fail;
  }

  rdata_section = binary_emitter_get_or_create_section(
      emitter, ".rdata", BINARY_SECTION_RDATA, 0, 8);
  if (rdata_section == (size_t)-1) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to create .rdata section");
    goto fail;
  }

  if (!code_generator_binary_emit_profile_cstring_table(
          generator, emitter, rdata_section, "mettle_profile_names",
          profile_names, function_count, name_symbols) ||
      !code_generator_binary_emit_profile_cstring_table(
          generator, emitter, rdata_section, "mettle_profile_files",
          profile_files, function_count, file_symbols)) {
    if (!generator->has_error) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to emit profile string tables");
    }
    goto fail;
  }

  {
    size_t lines_offset = 0;
    if (!binary_emitter_align_section(emitter, rdata_section, 8, 0) ||
        !binary_emitter_append_bytes(emitter, rdata_section, profile_lines,
                                     function_count * sizeof(uint64_t),
                                     &lines_offset) ||
        !binary_emitter_define_symbol(emitter, "mettle_profile_lines",
                                      BINARY_SYMBOL_GLOBAL, rdata_section,
                                      lines_offset,
                                      function_count * sizeof(uint64_t))) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to emit profile line table");
      goto fail;
    }
  }

  data_section = binary_emitter_get_or_create_section(
      emitter, ".data", BINARY_SECTION_DATA, 0, 8);
  if (data_section == (size_t)-1) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to create .data section");
    goto fail;
  }

  {
    uint64_t count_value = (uint64_t)function_count;
    size_t count_offset = 0;

    if (!binary_emitter_append_bytes(emitter, data_section, &count_value,
                                     sizeof(count_value), &count_offset) ||
        !binary_emitter_define_symbol(emitter, "mettle_profile_name_count",
                                      BINARY_SYMBOL_GLOBAL, data_section,
                                      count_offset, sizeof(count_value))) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to emit profile name count");
      goto fail;
    }
  }

  for (size_t i = 0; i < function_count; i++) {
    free(name_symbols[i]);
    free(file_symbols[i]);
  }
  free(name_symbols);
  free(file_symbols);
  free(profile_names);
  free(profile_files);
  free(profile_lines);
  return 1;

fail:
  if (name_symbols) {
    for (size_t i = 0; i < function_count; i++) {
      free(name_symbols[i]);
    }
    free(name_symbols);
  }
  if (file_symbols) {
    for (size_t i = 0; i < function_count; i++) {
      free(file_symbols[i]);
    }
    free(file_symbols);
  }
  free(profile_names);
  free(profile_files);
  free(profile_lines);
  return 0;
}
