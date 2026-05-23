#ifndef METTLE_TEST_HELPERS_H
#define METTLE_TEST_HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_ENTRY_SYMBOL "mainCRTStartup"
#define TEST_TARGET_DATA_SYMBOL "target_data"
#define PE_MACHINE_AMD64 0x8664u
#define PE_OPTIONAL_MAGIC_PE32PLUS 0x020Bu

static inline int report_failure(const char *test_name, const char *message) {
  fprintf(stderr, "FAIL: %s - %s\n", test_name, message);
  return 1;
}

static inline char *join_path(const char *dir, const char *file) {
  size_t dir_len = strlen(dir);
  size_t file_len = strlen(file);
  char *path = malloc(dir_len + 1 + file_len + 1);
  if (!path) return NULL;
  memcpy(path, dir, dir_len);
  path[dir_len] = '/';
  memcpy(path + dir_len + 1, file, file_len + 1);
  return path;
}

static inline uint32_t test_read_u32(const unsigned char *data) {
  return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
         ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static inline uint64_t test_read_u64(const unsigned char *data) {
  return (uint64_t)data[0] | ((uint64_t)data[1] << 8) |
         ((uint64_t)data[2] << 16) | ((uint64_t)data[3] << 24) |
         ((uint64_t)data[4] << 32) | ((uint64_t)data[5] << 40) |
         ((uint64_t)data[6] << 48) | ((uint64_t)data[7] << 56);
}

#endif
