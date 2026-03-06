#include "linker/coff_reader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COFF_RELOC_AMD64_REL32 0x0004u

static int report_failure(const char *message, const char *detail) {
  if (detail && detail[0] != '\0') {
    fprintf(stderr, "%s: %s\n", message, detail);
  } else {
    fprintf(stderr, "%s\n", message);
  }
  return 1;
}

static const CoffSection *find_section_with_relocations(const CoffObject *object) {
  size_t i = 0;

  if (!object) {
    return NULL;
  }

  for (i = 0; i < object->section_count; i++) {
    if (object->sections[i].relocation_count > 0u) {
      return &object->sections[i];
    }
  }

  return NULL;
}

static int expect_basic_meth_object(const char *path) {
  CoffObject *object = NULL;
  char *error_message = NULL;
  const CoffSection *text = NULL;
  const CoffSymbol *main_symbol = NULL;
  int result = 1;

  if (!coff_object_read(path, &object, &error_message)) {
    result = report_failure("Failed to parse Methlang object", error_message);
    goto cleanup;
  }

  if (object->machine != COFF_MACHINE_AMD64) {
    result = report_failure("Unexpected COFF machine", path);
    goto cleanup;
  }
  if (object->section_count == 0u) {
    result = report_failure("Object has no sections", path);
    goto cleanup;
  }

  text = coff_object_find_section_by_kind(object, COFF_SECTION_KIND_TEXT);
  if (!text) {
    result = report_failure("Object is missing a .text section", path);
    goto cleanup;
  }
  if (text->size_of_raw_data == 0u || !text->raw_data) {
    result = report_failure(".text section payload was not loaded", path);
    goto cleanup;
  }

  main_symbol = coff_object_find_symbol(object, "main");
  if (!main_symbol || main_symbol->is_auxiliary) {
    result = report_failure("Object is missing symbol 'main'", path);
    goto cleanup;
  }
  result = 0;

cleanup:
  free(error_message);
  coff_object_destroy(object);
  return result;
}

static int expect_rel32_call(const char *path, const char *symbol_name) {
  CoffObject *object = NULL;
  char *error_message = NULL;
  const CoffSection *section = NULL;
  int found = 0;
  int result = 1;
  size_t i = 0;

  if (!coff_object_read(path, &object, &error_message)) {
    result = report_failure("Failed to parse relocation object", error_message);
    goto cleanup;
  }

  section = find_section_with_relocations(object);
  if (!section) {
    result = report_failure("Object did not contain any relocations", path);
    goto cleanup;
  }

  for (i = 0; i < section->relocation_count; i++) {
    const CoffRelocation *relocation = &section->relocations[i];
    const CoffSymbol *target = NULL;

    if (relocation->symbol_table_index >= object->symbol_count) {
      result = report_failure("Relocation referred to an invalid symbol index",
                              path);
      goto cleanup;
    }
    if (relocation->type != COFF_RELOC_AMD64_REL32) {
      continue;
    }

    target = &object->symbols[relocation->symbol_table_index];
    if (!target->is_auxiliary && target->name &&
        strcmp(target->name, symbol_name) == 0) {
      found = 1;
      break;
    }
  }

  if (!found) {
    result = report_failure("Did not find expected REL32 relocation",
                            symbol_name);
    goto cleanup;
  }
  result = 0;

cleanup:
  free(error_message);
  coff_object_destroy(object);
  return result;
}

static int expect_long_symbol(const char *path, const char *symbol_name) {
  CoffObject *object = NULL;
  char *error_message = NULL;
  const CoffSymbol *symbol = NULL;
  int result = 1;

  if (!coff_object_read(path, &object, &error_message)) {
    result = report_failure("Failed to parse long-name object", error_message);
    goto cleanup;
  }

  if (object->string_table_size <= 4u) {
    result = report_failure("Object did not expose a non-empty string table",
                            path);
    goto cleanup;
  }

  symbol = coff_object_find_symbol(object, symbol_name);
  if (!symbol) {
    result = report_failure("Object is missing expected long symbol",
                            symbol_name);
    goto cleanup;
  }
  if (strlen(symbol_name) <= 8u) {
    result = report_failure("Test bug: expected a long symbol name", symbol_name);
    goto cleanup;
  }
  result = 0;

cleanup:
  free(error_message);
  coff_object_destroy(object);
  return result;
}

static int expect_gcc_object(const char *path) {
  CoffObject *object = NULL;
  char *error_message = NULL;
  const CoffSection *text = NULL;
  int result = 1;

  if (!coff_object_read(path, &object, &error_message)) {
    result = report_failure("Failed to parse GCC object", error_message);
    goto cleanup;
  }

  text = coff_object_find_section_by_kind(object, COFF_SECTION_KIND_TEXT);
  if (!text) {
    result = report_failure("GCC object is missing a text-like section", path);
    goto cleanup;
  }
  if (object->symbol_count == 0u) {
    result = report_failure("GCC object did not contain any symbols", path);
    goto cleanup;
  }
  result = 0;

cleanup:
  free(error_message);
  coff_object_destroy(object);
  return result;
}

int main(int argc, char **argv) {
  if (argc != 5) {
    fprintf(stderr,
            "Usage: %s <basic.obj> <reloc.obj> <long.obj> <gcc.o>\n",
            argv[0]);
    return 1;
  }

  if (expect_basic_meth_object(argv[1]) != 0 ||
      expect_rel32_call(argv[2], "callee") != 0 ||
      expect_long_symbol(argv[3],
                         "reader_target_symbol_long_name") != 0 ||
      expect_rel32_call(argv[3], "reader_target_symbol_long_name") != 0 ||
      expect_gcc_object(argv[4]) != 0) {
    return 1;
  }

  return 0;
}
