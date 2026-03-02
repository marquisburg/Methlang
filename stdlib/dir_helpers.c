/**
 * dir_helpers.c - Cross-platform directory and file operations for Methlang std/dir.
 * Link this when using std/dir.
 */

#include <stdint.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)

#include <windows.h>

int32_t methlang_dir_exists(const char *path) {
  DWORD attrs = GetFileAttributesA(path);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return 0;
  }
  return (attrs & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
}

int32_t methlang_dir_create(const char *path) {
  if (CreateDirectoryA(path, NULL)) {
    return 0;
  }
  return -1;
}

int32_t methlang_file_exists(const char *path) {
  DWORD attrs = GetFileAttributesA(path);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return 0;
  }
  return (attrs & FILE_ATTRIBUTE_DIRECTORY) ? 0 : 1;
}

/* Fills buf with current working directory. Returns 0 on success, -1 on failure. */
int32_t methlang_getcwd(char *buf, int32_t size) {
  if (!buf || size <= 0) {
    return -1;
  }
  DWORD len = GetCurrentDirectoryA((DWORD)size, buf);
  if (len == 0 || len >= (DWORD)size) {
    return -1;
  }
  return 0;
}

#else

#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

int32_t methlang_dir_exists(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0) {
    return 0;
  }
  return S_ISDIR(st.st_mode) ? 1 : 0;
}

int32_t methlang_dir_create(const char *path) {
  if (mkdir(path, 0755) == 0) {
    return 0;
  }
  return -1;
}

int32_t methlang_file_exists(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0) {
    return 0;
  }
  return S_ISREG(st.st_mode) ? 1 : 0;
}

int32_t methlang_getcwd(char *buf, int32_t size) {
  if (!buf || size <= 0) {
    return -1;
  }
  if (getcwd(buf, (size_t)size) == NULL) {
    return -1;
  }
  return 0;
}

#endif
