#include "codegen/binary/internal.h"

#include "debug/debug_info.h"
#include "runtime/crash_handler.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int binary_debug_label_export_add(BinaryDebugLabelExportTable *table,
                                         const char *name, size_t offset) {
  char *owned_name = NULL;

  if (!table || !name) {
    return 0;
  }

  if (table->count == table->capacity) {
    size_t new_capacity = table->capacity == 0 ? 8 : table->capacity * 2;
    BinaryDebugLabelExport *grown =
        realloc(table->items, new_capacity * sizeof(BinaryDebugLabelExport));
    if (!grown) {
      return 0;
    }
    table->items = grown;
    table->capacity = new_capacity;
  }

  owned_name = strdup(name);
  if (!owned_name) {
    return 0;
  }

  table->items[table->count].name = owned_name;
  table->items[table->count].offset = offset;
  table->count++;
  return 1;
}

int code_generator_binary_record_debug_label_export(
    BinaryFunctionContext *context, const char *name, size_t offset) {
  if (!context || !name) {
    return 0;
  }
  return binary_debug_label_export_add(&context->debug_export_labels, name,
                                       offset);
}

int code_generator_binary_emit_runtime_location_marker(
    CodeGenerator *generator, BinaryFunctionContext *context,
    size_t source_line, size_t source_column, const char *filename) {
  char *label = NULL;

  if (!generator || !context || !generator->debug_info ||
      !generator->generate_stack_trace_support || source_line == 0 ||
      !generator->current_function_name) {
    return 1;
  }

  if (generator->last_runtime_location_line == source_line &&
      generator->last_runtime_location_column == source_column) {
    return 1;
  }

  label = code_generator_generate_label(generator, "mettledbg_loc");
  if (!label) {
    code_generator_set_error(generator,
                             "Out of memory while creating runtime location "
                             "label in function '%s'",
                             context->function_name);
    return 0;
  }

  if (!binary_label_table_define(&context->labels, label, context->code.size)) {
    code_generator_set_error(
        generator,
        "Failed to define runtime location label in function '%s'",
        context->function_name);
    free(label);
    return 0;
  }

  if (!binary_debug_label_export_add(&context->debug_export_labels, label,
                                     context->code.size)) {
    code_generator_set_error(generator,
                             "Out of memory while recording runtime location "
                             "label in function '%s'",
                             context->function_name);
    free(label);
    return 0;
  }

  debug_info_add_runtime_location_mapping(
      generator->debug_info, generator->current_function_name, label, filename,
      source_line, source_column);
  generator->last_runtime_location_line = source_line;
  generator->last_runtime_location_column = source_column;
  free(label);
  return 1;
}

int code_generator_binary_export_debug_symbols(
    CodeGenerator *generator, BinaryFunctionContext *context,
    size_t text_section, size_t function_offset, size_t end_offset) {
  BinaryEmitter *emitter = NULL;

  if (!generator || !context) {
    return 0;
  }

  emitter = code_generator_get_binary_emitter(generator);
  if (!emitter) {
    code_generator_set_error(generator, "Binary emitter is not initialized");
    return 0;
  }

  if (context->runtime_end_label &&
      !binary_emitter_define_symbol(emitter, context->runtime_end_label,
                                    BINARY_SYMBOL_LOCAL, text_section,
                                    function_offset + end_offset, 0)) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to define runtime function end label");
    return 0;
  }

  for (size_t i = 0; i < context->debug_export_labels.count; i++) {
    BinaryDebugLabelExport *entry = &context->debug_export_labels.items[i];
    if (!binary_emitter_define_symbol(emitter, entry->name, BINARY_SYMBOL_LOCAL,
                                      text_section,
                                      function_offset + entry->offset, 0)) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to define runtime location label");
      return 0;
    }
  }

  return 1;
}

static int code_generator_binary_emit_debug_cstring(
    BinaryEmitter *emitter, size_t rdata_section, const char *prefix,
    size_t index, const char *value, char **out_symbol) {
  const char *text = value ? value : "";
  size_t literal_offset = 0;
  size_t length = strlen(text);
  unsigned char terminator = 0;
  int written = 0;

  written = snprintf(NULL, 0, "%s_%zu", prefix, index);
  if (written <= 0) {
    return 0;
  }

  *out_symbol = malloc((size_t)written + 1u);
  if (!*out_symbol) {
    return 0;
  }
  snprintf(*out_symbol, (size_t)written + 1u, "%s_%zu", prefix, index);

  if (!binary_emitter_append_bytes(emitter, rdata_section, text, length,
                                   &literal_offset) ||
      !binary_emitter_append_bytes(emitter, rdata_section, &terminator, 1,
                                   NULL) ||
      !binary_emitter_define_symbol(emitter, *out_symbol, BINARY_SYMBOL_LOCAL,
                                    rdata_section, literal_offset,
                                    length + 1)) {
    return 0;
  }

  return 1;
}

int code_generator_binary_emit_runtime_debug_tables(CodeGenerator *generator) {
  BinaryEmitter *emitter = NULL;
  DebugInfo *debug_info = NULL;
  size_t rdata_section = 0;
  size_t functions_offset = 0;
  size_t locations_offset = 0;
  size_t trap_sites_offset = 0;
  size_t header_offset = 0;
  size_t image_offset = 0;
  char **function_name_symbols = NULL;
  char **function_file_symbols = NULL;
  char **location_name_symbols = NULL;
  char **location_file_symbols = NULL;
  char **trap_source_symbols = NULL;
  char **trap_message_symbols = NULL;
  char **trap_context_symbols = NULL;
  char **trap_name_symbols = NULL;
  char **trap_file_symbols = NULL;

  if (!generator || !generator->debug_info ||
      !generator->generate_stack_trace_support) {
    return 1;
  }

  debug_info = generator->debug_info;
  if (debug_info->runtime_function_count == 0 &&
      debug_info->runtime_location_count == 0 &&
      debug_info->runtime_trap_site_count == 0) {
    return 1;
  }

  emitter = code_generator_get_binary_emitter(generator);
  if (!emitter) {
    code_generator_set_error(generator, "Binary emitter is not initialized");
    return 0;
  }

  rdata_section = binary_emitter_get_or_create_section(
      emitter, ".rdata", BINARY_SECTION_RDATA, 0, 8);
  if (rdata_section == (size_t)-1) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to create .rdata section");
    return 0;
  }

  if (debug_info->runtime_function_count > 0) {
    function_name_symbols =
        calloc(debug_info->runtime_function_count, sizeof(char *));
    function_file_symbols =
        calloc(debug_info->runtime_function_count, sizeof(char *));
    if (!function_name_symbols || !function_file_symbols) {
      goto fail;
    }
  }

  if (debug_info->runtime_location_count > 0) {
    location_name_symbols =
        calloc(debug_info->runtime_location_count, sizeof(char *));
    location_file_symbols =
        calloc(debug_info->runtime_location_count, sizeof(char *));
    if (!location_name_symbols || !location_file_symbols) {
      goto fail;
    }
  }

  if (debug_info->runtime_trap_site_count > 0) {
    trap_source_symbols =
        calloc(debug_info->runtime_trap_site_count, sizeof(char *));
    trap_message_symbols =
        calloc(debug_info->runtime_trap_site_count, sizeof(char *));
    trap_context_symbols =
        calloc(debug_info->runtime_trap_site_count, sizeof(char *));
    trap_name_symbols =
        calloc(debug_info->runtime_trap_site_count, sizeof(char *));
    trap_file_symbols =
        calloc(debug_info->runtime_trap_site_count, sizeof(char *));
    if (!trap_source_symbols || !trap_message_symbols || !trap_context_symbols ||
        !trap_name_symbols || !trap_file_symbols) {
      goto fail;
    }
  }

  for (size_t i = 0; i < debug_info->runtime_function_count; i++) {
    RuntimeFunctionMapping *mapping = &debug_info->runtime_functions[i];
    if (!code_generator_binary_emit_debug_cstring(
            emitter, rdata_section, "mettledbg_func_name", i,
            mapping->function_name, &function_name_symbols[i]) ||
        !code_generator_binary_emit_debug_cstring(
            emitter, rdata_section, "mettledbg_func_file", i, mapping->filename,
            &function_file_symbols[i])) {
      goto fail;
    }
  }

  for (size_t i = 0; i < debug_info->runtime_location_count; i++) {
    RuntimeLocationMapping *mapping = &debug_info->runtime_locations[i];
    if (!code_generator_binary_emit_debug_cstring(
            emitter, rdata_section, "mettledbg_loc_name", i, mapping->function_name,
            &location_name_symbols[i]) ||
        !code_generator_binary_emit_debug_cstring(
            emitter, rdata_section, "mettledbg_loc_file", i, mapping->filename,
            &location_file_symbols[i])) {
      goto fail;
    }
  }

  for (size_t i = 0; i < debug_info->runtime_trap_site_count; i++) {
    RuntimeTrapSiteMapping *mapping = &debug_info->runtime_trap_sites[i];
    if (!code_generator_binary_emit_debug_cstring(
            emitter, rdata_section, "mettledbg_trap_src", i, mapping->source_line,
            &trap_source_symbols[i]) ||
        !code_generator_binary_emit_debug_cstring(
            emitter, rdata_section, "mettledbg_trap_msg", i,
            mapping->message_template, &trap_message_symbols[i]) ||
        !code_generator_binary_emit_debug_cstring(
            emitter, rdata_section, "mettledbg_trap_ctx", i, mapping->static_context,
            &trap_context_symbols[i]) ||
        !code_generator_binary_emit_debug_cstring(
            emitter, rdata_section, "mettledbg_trap_name", i, mapping->function_name,
            &trap_name_symbols[i]) ||
        !code_generator_binary_emit_debug_cstring(
            emitter, rdata_section, "mettledbg_trap_file", i, mapping->filename,
            &trap_file_symbols[i])) {
      goto fail;
    }
  }

  {
    uint32_t header_words[8] = {
        METTLE_CRASH_DEBUG_MAGIC,
        METTLE_CRASH_DEBUG_VERSION,
        (uint32_t)debug_info->runtime_function_count,
        (uint32_t)debug_info->runtime_location_count,
        (uint32_t)debug_info->runtime_trap_site_count,
        0u,
        0u,
        0u,
    };
    if (!binary_emitter_align_section(emitter, rdata_section, 8, 0) ||
        !binary_emitter_append_bytes(emitter, rdata_section, header_words,
                                     sizeof(header_words), &header_offset) ||
        !binary_emitter_define_symbol(emitter, "mettle_debug_header",
                                      BINARY_SYMBOL_GLOBAL, rdata_section,
                                      header_offset, sizeof(header_words))) {
      goto fail;
    }
  }

  if (debug_info->runtime_function_count > 0) {
    size_t row_size = sizeof(MettleCrashFunctionInfo);
    if (!binary_emitter_align_section(emitter, rdata_section, 8, 0) ||
        !binary_emitter_append_zeros(
            emitter, rdata_section,
            debug_info->runtime_function_count * row_size, &functions_offset) ||
        !binary_emitter_define_symbol(emitter, "mettle_debug_functions",
                                      BINARY_SYMBOL_GLOBAL, rdata_section,
                                      functions_offset,
                                      debug_info->runtime_function_count *
                                          row_size)) {
      goto fail;
    }

    for (size_t i = 0; i < debug_info->runtime_function_count; i++) {
      RuntimeFunctionMapping *mapping = &debug_info->runtime_functions[i];
      size_t row_offset = functions_offset + i * row_size;
      uint64_t line = (uint64_t)mapping->line;
      uint64_t column = (uint64_t)mapping->column;

      if (!binary_emitter_add_relocation(
              emitter, rdata_section, row_offset + 0,
              BINARY_RELOCATION_ADDR64, mapping->start_label, 0) ||
          !binary_emitter_add_relocation(
              emitter, rdata_section, row_offset + 8, BINARY_RELOCATION_ADDR64,
              mapping->end_label, 0) ||
          !binary_emitter_add_relocation(
              emitter, rdata_section, row_offset + 16, BINARY_RELOCATION_ADDR64,
              function_name_symbols[i], 0) ||
          !binary_emitter_add_relocation(
              emitter, rdata_section, row_offset + 24, BINARY_RELOCATION_ADDR64,
              function_file_symbols[i], 0)) {
        goto fail;
      }
      {
        BinarySection *section =
            binary_emitter_get_section(emitter, rdata_section);
        if (!section || row_offset + row_size > section->size) {
          goto fail;
        }
        memcpy(section->data + row_offset + 32, &line, sizeof(line));
        memcpy(section->data + row_offset + 40, &column, sizeof(column));
      }
    }
  }

  if (debug_info->runtime_location_count > 0) {
    size_t row_size = sizeof(MettleCrashLocationInfo);
    if (!binary_emitter_align_section(emitter, rdata_section, 8, 0) ||
        !binary_emitter_append_zeros(
            emitter, rdata_section,
            debug_info->runtime_location_count * row_size, &locations_offset) ||
        !binary_emitter_define_symbol(emitter, "mettle_debug_locations",
                                      BINARY_SYMBOL_GLOBAL, rdata_section,
                                      locations_offset,
                                      debug_info->runtime_location_count *
                                          row_size)) {
      goto fail;
    }

    for (size_t i = 0; i < debug_info->runtime_location_count; i++) {
      RuntimeLocationMapping *mapping = &debug_info->runtime_locations[i];
      size_t row_offset = locations_offset + i * row_size;
      uint64_t line = (uint64_t)mapping->line;
      uint64_t column = (uint64_t)mapping->column;

      if (!binary_emitter_add_relocation(
              emitter, rdata_section, row_offset + 0, BINARY_RELOCATION_ADDR64,
              mapping->address_label, 0) ||
          !binary_emitter_add_relocation(
              emitter, rdata_section, row_offset + 8, BINARY_RELOCATION_ADDR64,
              location_name_symbols[i], 0) ||
          !binary_emitter_add_relocation(
              emitter, rdata_section, row_offset + 16, BINARY_RELOCATION_ADDR64,
              location_file_symbols[i], 0)) {
        goto fail;
      }
      {
        BinarySection *section =
            binary_emitter_get_section(emitter, rdata_section);
        if (!section || row_offset + row_size > section->size) {
          goto fail;
        }
        memcpy(section->data + row_offset + 24, &line, sizeof(line));
        memcpy(section->data + row_offset + 32, &column, sizeof(column));
      }
    }
  }

  if (debug_info->runtime_trap_site_count > 0) {
    size_t row_size = sizeof(MettleCrashTrapSiteInfo);
    if (!binary_emitter_align_section(emitter, rdata_section, 8, 0) ||
        !binary_emitter_append_zeros(
            emitter, rdata_section,
            debug_info->runtime_trap_site_count * row_size, &trap_sites_offset) ||
        !binary_emitter_define_symbol(emitter, "mettle_debug_trap_sites",
                                      BINARY_SYMBOL_GLOBAL, rdata_section,
                                      trap_sites_offset,
                                      debug_info->runtime_trap_site_count *
                                          row_size)) {
      goto fail;
    }

    for (size_t i = 0; i < debug_info->runtime_trap_site_count; i++) {
      RuntimeTrapSiteMapping *mapping = &debug_info->runtime_trap_sites[i];
      size_t row_offset = trap_sites_offset + i * row_size;
      uint32_t kind = mapping->kind;
      uint32_t reserved = 0;
      uint64_t line = (uint64_t)mapping->line;
      uint64_t column = (uint64_t)mapping->column;

      if (!binary_emitter_add_relocation(
              emitter, rdata_section, row_offset + 0, BINARY_RELOCATION_ADDR64,
              mapping->address_label, 0) ||
          !binary_emitter_add_relocation(
              emitter, rdata_section, row_offset + 16, BINARY_RELOCATION_ADDR64,
              trap_name_symbols[i], 0) ||
          !binary_emitter_add_relocation(
              emitter, rdata_section, row_offset + 24, BINARY_RELOCATION_ADDR64,
              trap_file_symbols[i], 0) ||
          !binary_emitter_add_relocation(
              emitter, rdata_section, row_offset + 48, BINARY_RELOCATION_ADDR64,
              trap_source_symbols[i], 0) ||
          !binary_emitter_add_relocation(
              emitter, rdata_section, row_offset + 56, BINARY_RELOCATION_ADDR64,
              trap_message_symbols[i], 0) ||
          !binary_emitter_add_relocation(
              emitter, rdata_section, row_offset + 64, BINARY_RELOCATION_ADDR64,
              trap_context_symbols[i], 0)) {
        goto fail;
      }
      {
        BinarySection *section =
            binary_emitter_get_section(emitter, rdata_section);
        if (!section || row_offset + row_size > section->size) {
          goto fail;
        }
        memcpy(section->data + row_offset + 8, &kind, sizeof(kind));
        memcpy(section->data + row_offset + 12, &reserved, sizeof(reserved));
        memcpy(section->data + row_offset + 32, &line, sizeof(line));
        memcpy(section->data + row_offset + 40, &column, sizeof(column));
      }
    }
  }

  {
    size_t image_size = sizeof(MettleCrashDebugImage);
    if (!binary_emitter_align_section(emitter, rdata_section, 8, 0) ||
        !binary_emitter_append_zeros(emitter, rdata_section, image_size,
                                     &image_offset) ||
        !binary_emitter_define_symbol(emitter, "mettle_debug_image",
                                      BINARY_SYMBOL_GLOBAL, rdata_section,
                                      image_offset, image_size)) {
      goto fail;
    }
    if (!binary_emitter_add_relocation(
            emitter, rdata_section, image_offset + 0, BINARY_RELOCATION_ADDR64,
            "mettle_debug_header", 0)) {
      goto fail;
    }
    if (debug_info->runtime_function_count > 0 &&
        !binary_emitter_add_relocation(
            emitter, rdata_section, image_offset + 8, BINARY_RELOCATION_ADDR64,
            "mettle_debug_functions", 0)) {
      goto fail;
    }
    if (debug_info->runtime_location_count > 0 &&
        !binary_emitter_add_relocation(
            emitter, rdata_section, image_offset + 16, BINARY_RELOCATION_ADDR64,
            "mettle_debug_locations", 0)) {
      goto fail;
    }
    if (debug_info->runtime_trap_site_count > 0 &&
        !binary_emitter_add_relocation(
            emitter, rdata_section, image_offset + 24, BINARY_RELOCATION_ADDR64,
            "mettle_debug_trap_sites", 0)) {
      goto fail;
    }
  }

  if (function_name_symbols) {
    for (size_t i = 0; i < debug_info->runtime_function_count; i++) {
      free(function_name_symbols[i]);
      free(function_file_symbols[i]);
    }
  }
  if (location_name_symbols) {
    for (size_t i = 0; i < debug_info->runtime_location_count; i++) {
      free(location_name_symbols[i]);
      free(location_file_symbols[i]);
    }
  }
  if (trap_source_symbols) {
    for (size_t i = 0; i < debug_info->runtime_trap_site_count; i++) {
      free(trap_source_symbols[i]);
      free(trap_message_symbols[i]);
      free(trap_context_symbols[i]);
      free(trap_name_symbols[i]);
      free(trap_file_symbols[i]);
    }
  }
  free(function_name_symbols);
  free(function_file_symbols);
  free(location_name_symbols);
  free(location_file_symbols);
  free(trap_source_symbols);
  free(trap_message_symbols);
  free(trap_context_symbols);
  free(trap_name_symbols);
  free(trap_file_symbols);
  return 1;

fail:
  if (function_name_symbols) {
    for (size_t i = 0; i < debug_info->runtime_function_count; i++) {
      free(function_name_symbols[i]);
      free(function_file_symbols[i]);
    }
  }
  if (location_name_symbols) {
    for (size_t i = 0; i < debug_info->runtime_location_count; i++) {
      free(location_name_symbols[i]);
      free(location_file_symbols[i]);
    }
  }
  if (trap_source_symbols) {
    for (size_t i = 0; i < debug_info->runtime_trap_site_count; i++) {
      free(trap_source_symbols[i]);
      free(trap_message_symbols[i]);
      free(trap_context_symbols[i]);
      free(trap_name_symbols[i]);
      free(trap_file_symbols[i]);
    }
  }
  free(function_name_symbols);
  free(function_file_symbols);
  free(location_name_symbols);
  free(location_file_symbols);
  free(trap_source_symbols);
  free(trap_message_symbols);
  free(trap_context_symbols);
  free(trap_name_symbols);
  free(trap_file_symbols);
  if (!generator->has_error) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to emit runtime debug tables");
  }
  return 0;
}

int code_generator_binary_emit_crash_startup(CodeGenerator *generator) {
  BinaryEmitter *emitter = NULL;
  BinaryCodeBuffer code = {0};
  size_t text_section = 0;
  size_t function_offset = 0;
  size_t install_call_offset = 0;
  size_t register_call_offset = 0;
  size_t lea_image_offset = 0;
  DebugInfo *debug_info = NULL;
  int has_debug_image = 0;
  BinaryCallRelocationTable call_relocations = {0};

  if (!generator || !generator->generate_stack_trace_support) {
    return 1;
  }

  debug_info = generator->debug_info;
  has_debug_image =
      debug_info &&
      (debug_info->runtime_function_count > 0 ||
       debug_info->runtime_location_count > 0 ||
       debug_info->runtime_trap_site_count > 0);

  emitter = code_generator_get_binary_emitter(generator);
  if (!emitter) {
    code_generator_set_error(generator, "Binary emitter is not initialized");
    return 0;
  }

  if (!binary_emit_sub_rsp_imm32(&code, BINARY_WIN64_SHADOW_SPACE_SIZE) ||
      !binary_emit_call_placeholder(&code, &install_call_offset) ||
      !binary_call_relocation_table_add(&call_relocations, "mettle_crash_install",
                                        install_call_offset)) {
    goto fail;
  }

  if (has_debug_image) {
    if (!binary_emit_lea_reg_rip_placeholder(&code, BINARY_GP_RCX,
                                             &lea_image_offset) ||
        !binary_emit_sub_rsp_imm32(&code, BINARY_WIN64_SHADOW_SPACE_SIZE) ||
        !binary_emit_call_placeholder(&code, &register_call_offset) ||
        !binary_call_relocation_table_add(
            &call_relocations, "mettle_crash_register_debug_image",
            register_call_offset) ||
        !binary_emit_add_rsp_imm32(&code, BINARY_WIN64_SHADOW_SPACE_SIZE)) {
      goto fail;
    }
  }

  if (!binary_emit_add_rsp_imm32(&code, BINARY_WIN64_SHADOW_SPACE_SIZE) ||
      !binary_emit_ret(&code)) {
    goto fail;
  }

  text_section = binary_emitter_get_or_create_section(
      emitter, ".text", BINARY_SECTION_TEXT, 0, BINARY_TEXT_SECTION_ALIGNMENT);
  if (text_section == (size_t)-1) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to create .text section");
    goto fail;
  }

  if (!binary_emitter_align_section(emitter, text_section,
                                    BINARY_TEXT_SECTION_ALIGNMENT, 0x90)) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to align .text section");
    goto fail;
  }

  {
    BinarySection *section = binary_emitter_get_section(emitter, text_section);
    if (!section) {
      code_generator_set_error(generator, "Failed to access .text section");
      goto fail;
    }
    function_offset = section->size;
  }

  if (!binary_emitter_define_symbol(emitter, "mettle_crash_startup",
                                    BINARY_SYMBOL_GLOBAL, text_section,
                                    function_offset, code.size) ||
      !binary_emitter_append_bytes(emitter, text_section, code.data, code.size,
                                   NULL)) {
    code_generator_set_error(generator, "%s",
                             binary_emitter_get_error(emitter)
                                 ? binary_emitter_get_error(emitter)
                                 : "Failed to emit crash startup function");
    goto fail;
  }

  if (!code_generator_binary_declare_external_symbol(generator,
                                                     "mettle_crash_install")) {
    goto fail;
  }

  for (size_t i = 0; i < call_relocations.count; i++) {
    BinaryCallRelocation *relocation = &call_relocations.items[i];
    if (!binary_emitter_add_relocation(
            emitter, text_section,
            function_offset + relocation->displacement_offset,
            BINARY_RELOCATION_REL32, relocation->symbol_name, 0)) {
      code_generator_set_error(generator, "%s",
                               binary_emitter_get_error(emitter)
                                   ? binary_emitter_get_error(emitter)
                                   : "Failed to record crash startup relocation");
      goto fail;
    }
  }

  if (has_debug_image) {
    if (!binary_emitter_add_relocation(
            emitter, text_section, function_offset + lea_image_offset,
            BINARY_RELOCATION_REL32, "mettle_debug_image", 0) ||
        !code_generator_binary_declare_external_symbol(
            generator, "mettle_crash_register_debug_image")) {
      goto fail;
    }
  }

  binary_code_buffer_destroy(&code);
  binary_call_relocation_table_destroy(&call_relocations);
  return 1;

fail:
  binary_code_buffer_destroy(&code);
  binary_call_relocation_table_destroy(&call_relocations);
  if (!generator->has_error) {
    code_generator_set_error(generator,
                             "Out of memory while emitting crash startup");
  }
  return 0;
}
