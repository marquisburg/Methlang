#include "linker/symbol_resolve.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int report_failure(const char *message, const char *detail) {
  if (detail && detail[0] != '\0') {
    fprintf(stderr, "%s: %s\n", message, detail);
  } else {
    fprintf(stderr, "%s\n", message);
  }
  return 1;
}

static int contains_text(const char *text, const char *needle) {
  if (!text || !needle) {
    return 0;
  }
  return strstr(text, needle) != NULL;
}

static int expect_function_merge(const char *entry_obj, const char *provider_obj) {
  const char *paths[2] = {entry_obj, provider_obj};
  LinkResolutionOptions options = {"mainCRTStartup", 16u, 0};
  LinkResolution *resolution = NULL;
  char *error_message = NULL;
  const LinkedSection *text = NULL;
  const LinkedSymbol *entry = NULL;
  const LinkedSymbol *callee = NULL;
  int result = 1;

  if (!link_resolution_build(paths, 2u, &options, &resolution, &error_message)) {
    result = report_failure("Function-merge resolution failed", error_message);
    goto cleanup;
  }

  text = link_resolution_find_section(resolution, COFF_SECTION_KIND_TEXT);
  if (!text || text->contribution_count != 2u || text->size == 0u) {
    result = report_failure("Merged .text section was not constructed as expected",
                            entry_obj);
    goto cleanup;
  }

  entry = link_resolution_find_symbol(resolution, "mainCRTStartup");
  callee = link_resolution_find_symbol(resolution, "add_one");
  if (!entry || !entry->is_defined || !callee || !callee->is_defined) {
    result = report_failure("Merged function symbols were not resolved",
                            "mainCRTStartup/add_one");
    goto cleanup;
  }
  if (resolution->entry_symbol != entry) {
    result = report_failure("Entry-point tracking did not resolve mainCRTStartup",
                            entry_obj);
    goto cleanup;
  }
  if (entry->defining_object_index != 0u || callee->defining_object_index != 1u) {
    result = report_failure("Merged function definitions were assigned to the wrong object",
                            "mainCRTStartup/add_one");
    goto cleanup;
  }
  if (callee->merged_offset <= entry->merged_offset) {
    result = report_failure("Merged function layout did not preserve object order",
                            "add_one should follow mainCRTStartup");
    goto cleanup;
  }

  result = 0;

cleanup:
  free(error_message);
  link_resolution_destroy(resolution);
  return result;
}

static int expect_data_merge(const char *entry_obj, const char *provider_obj) {
  const char *paths[2] = {entry_obj, provider_obj};
  LinkResolutionOptions options = {"mainCRTStartup", 16u, 0};
  LinkResolution *resolution = NULL;
  char *error_message = NULL;
  const LinkedSection *text = NULL;
  const LinkedSection *data = NULL;
  const LinkedSymbol *entry = NULL;
  const LinkedSymbol *shared = NULL;
  int result = 1;

  if (!link_resolution_build(paths, 2u, &options, &resolution, &error_message)) {
    result = report_failure("Data-merge resolution failed", error_message);
    goto cleanup;
  }

  text = link_resolution_find_section(resolution, COFF_SECTION_KIND_TEXT);
  data = link_resolution_find_section(resolution, COFF_SECTION_KIND_DATA);
  if (!text || text->contribution_count != 1u || !data ||
      data->contribution_count != 1u || data->size == 0u) {
    result = report_failure("Merged .text/.data sections were not constructed as expected",
                            provider_obj);
    goto cleanup;
  }

  entry = link_resolution_find_symbol(resolution, "mainCRTStartup");
  shared = link_resolution_find_symbol(resolution, "shared_counter");
  if (!entry || !entry->is_defined || !shared || !shared->is_defined) {
    result = report_failure("Merged data symbols were not resolved",
                            "mainCRTStartup/shared_counter");
    goto cleanup;
  }
  if (shared->defining_object_index != 1u ||
      shared->merged_section_index == LINKED_SECTION_INDEX_NONE) {
    result = report_failure("Merged data symbol metadata is inconsistent",
                            "shared_counter");
    goto cleanup;
  }

  result = 0;

cleanup:
  free(error_message);
  link_resolution_destroy(resolution);
  return result;
}

static int expect_bss_merge(const char *entry_obj, const char *provider_obj) {
  const char *paths[2] = {entry_obj, provider_obj};
  LinkResolutionOptions options = {"mainCRTStartup", 16u, 0};
  LinkResolution *resolution = NULL;
  char *error_message = NULL;
  const LinkedSection *bss = NULL;
  const LinkedSymbol *shared = NULL;
  int result = 1;

  if (!link_resolution_build(paths, 2u, &options, &resolution, &error_message)) {
    result = report_failure("BSS-merge resolution failed", error_message);
    goto cleanup;
  }

  bss = link_resolution_find_section(resolution, COFF_SECTION_KIND_BSS);
  if (!bss || bss->contribution_count != 1u || bss->virtual_size != 8u) {
    result = report_failure("Merged .bss section size was not preserved exactly",
                            provider_obj);
    goto cleanup;
  }

  shared = link_resolution_find_symbol(resolution, "shared_zero");
  if (!shared || !shared->is_defined || shared->merged_offset != 0u) {
    result = report_failure("BSS symbol metadata is inconsistent", "shared_zero");
    goto cleanup;
  }

  result = 0;

cleanup:
  free(error_message);
  link_resolution_destroy(resolution);
  return result;
}

static int expect_duplicate_failure(const char *left_obj, const char *right_obj) {
  const char *paths[2] = {left_obj, right_obj};
  LinkResolution *resolution = NULL;
  char *error_message = NULL;
  int result = 1;

  if (link_resolution_build(paths, 2u, NULL, &resolution, &error_message)) {
    result = report_failure("Duplicate symbol resolution unexpectedly succeeded",
                            "duplicate_symbol");
    goto cleanup;
  }
  if (!contains_text(error_message, "Duplicate external symbol 'duplicate_symbol'")) {
    result = report_failure("Duplicate symbol failure did not mention duplicate_symbol",
                            error_message);
    goto cleanup;
  }

  result = 0;

cleanup:
  free(error_message);
  link_resolution_destroy(resolution);
  return result;
}

static int expect_unresolved_failure(const char *path) {
  const char *paths[1] = {path};
  LinkResolutionOptions options = {"mainCRTStartup", 16u, 0};
  LinkResolution *resolution = NULL;
  char *error_message = NULL;
  int result = 1;

  if (link_resolution_build(paths, 1u, &options, &resolution, &error_message)) {
    result = report_failure("Unresolved-symbol resolution unexpectedly succeeded",
                            "missing_symbol");
    goto cleanup;
  }
  if (!contains_text(error_message, "Unresolved external symbol 'missing_symbol'")) {
    result = report_failure("Unresolved-symbol failure did not mention missing_symbol",
                            error_message);
    goto cleanup;
  }

  result = 0;

cleanup:
  free(error_message);
  link_resolution_destroy(resolution);
  return result;
}

int main(int argc, char **argv) {
  if (argc != 10) {
    fprintf(stderr,
            "Usage: %s <fn-entry.obj> <fn-provider.obj> <data-entry.obj> "
            "<data-provider.obj> <bss-entry.obj> <bss-provider.obj> "
            "<dup-a.obj> <dup-b.obj> <unresolved.obj>\n",
            argv[0]);
    return 1;
  }

  if (expect_function_merge(argv[1], argv[2]) != 0 ||
      expect_data_merge(argv[3], argv[4]) != 0 ||
      expect_bss_merge(argv[5], argv[6]) != 0 ||
      expect_duplicate_failure(argv[7], argv[8]) != 0 ||
      expect_unresolved_failure(argv[9]) != 0) {
    return 1;
  }

  return 0;
}
