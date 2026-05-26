/**
 * dir_helpers.c - Cross-platform directory and file operations for Mettle std/dir.
 * Link this when using std/dir.
 */

#include <stdint.h>
#include <string.h>

static int32_t mettle_append_md_path(char *paths_buf, int32_t paths_size, int32_t *offset,
                                     int32_t *count, int32_t max_files, const char *rel_path) {
  int32_t rel_len;
  if (!paths_buf || !offset || !count || !rel_path || *count >= max_files) {
    return 0;
  }
  rel_len = (int32_t)strlen(rel_path);
  if (rel_len <= 0) {
    return 0;
  }
  if (*offset + rel_len + 1 > paths_size) {
    return 0;
  }
  memcpy(paths_buf + *offset, rel_path, (size_t)rel_len + 1);
  *offset += rel_len + 1;
  (*count)++;
  return 1;
}

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)

#include <windows.h>

int32_t mettle_dir_exists(const char *path) {
  DWORD attrs = GetFileAttributesA(path);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return 0;
  }
  return (attrs & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
}

int32_t mettle_dir_create(const char *path) {
  if (CreateDirectoryA(path, NULL)) {
    return 0;
  }
  return -1;
}

int32_t mettle_file_exists(const char *path) {
  DWORD attrs = GetFileAttributesA(path);
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    return 0;
  }
  return (attrs & FILE_ATTRIBUTE_DIRECTORY) ? 0 : 1;
}

int32_t mettle_getcwd(char *buf, int32_t size) {
  if (!buf || size <= 0) {
    return -1;
  }
  DWORD len = GetCurrentDirectoryA((DWORD)size, buf);
  if (len == 0 || len >= (DWORD)size) {
    return -1;
  }
  return 0;
}

static void mettle_scan_md_files(const char *root_dir, const char *rel_prefix, char *paths_buf,
                                 int32_t paths_size, int32_t *offset, int32_t *count,
                                 int32_t max_files) {
  char search_path[MAX_PATH];
  char full_path[MAX_PATH];
  char rel_path[MAX_PATH];
  WIN32_FIND_DATAA entry;
  HANDLE handle;
  size_t root_len;
  size_t name_len;

  if (!root_dir || !paths_buf || !offset || !count) {
    return;
  }

  root_len = strlen(root_dir);
  if (root_len + 3 >= sizeof(search_path)) {
    return;
  }
  memcpy(search_path, root_dir, root_len);
  search_path[root_len] = '\\';
  search_path[root_len + 1] = '*';
  search_path[root_len + 2] = '\0';

  handle = FindFirstFileA(search_path, &entry);
  if (handle == INVALID_HANDLE_VALUE) {
    return;
  }

  do {
    if (strcmp(entry.cFileName, ".") == 0 || strcmp(entry.cFileName, "..") == 0) {
      continue;
    }
    if (root_len + 1 + strlen(entry.cFileName) + 1 >= sizeof(full_path)) {
      continue;
    }
    memcpy(full_path, root_dir, root_len);
    full_path[root_len] = '\\';
    strcpy(full_path + root_len + 1, entry.cFileName);

    if (rel_prefix && rel_prefix[0] != '\0') {
      if (strlen(rel_prefix) + 1 + strlen(entry.cFileName) + 1 >= sizeof(rel_path)) {
        continue;
      }
      strcpy(rel_path, rel_prefix);
      strcat(rel_path, "/");
      strcat(rel_path, entry.cFileName);
    } else {
      if (strlen(entry.cFileName) + 1 >= sizeof(rel_path)) {
        continue;
      }
      strcpy(rel_path, entry.cFileName);
    }

    if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      mettle_scan_md_files(full_path, rel_path, paths_buf, paths_size, offset, count, max_files);
      continue;
    }

    name_len = strlen(entry.cFileName);
    if (name_len < 4) {
      continue;
    }
    if (_stricmp(entry.cFileName + name_len - 3, ".md") != 0) {
      continue;
    }
    mettle_append_md_path(paths_buf, paths_size, offset, count, max_files, rel_path);
  } while (FindNextFileA(handle, &entry));

  FindClose(handle);
}

#else

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

int32_t mettle_dir_exists(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0) {
    return 0;
  }
  return S_ISDIR(st.st_mode) ? 1 : 0;
}

int32_t mettle_dir_create(const char *path) {
  if (mkdir(path, 0755) == 0) {
    return 0;
  }
  return -1;
}

int32_t mettle_file_exists(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0) {
    return 0;
  }
  return S_ISREG(st.st_mode) ? 1 : 0;
}

int32_t mettle_getcwd(char *buf, int32_t size) {
  if (!buf || size <= 0) {
    return -1;
  }
  if (getcwd(buf, (size_t)size) == NULL) {
    return -1;
  }
  return 0;
}

static void mettle_scan_md_files(const char *root_dir, const char *rel_prefix, char *paths_buf,
                                 int32_t paths_size, int32_t *offset, int32_t *count,
                                 int32_t max_files) {
  char full_path[4096];
  char rel_path[4096];
  DIR *dir;
  struct dirent *entry;
  size_t root_len;
  size_t name_len;

  if (!root_dir || !paths_buf || !offset || !count) {
    return;
  }

  dir = opendir(root_dir);
  if (!dir) {
    return;
  }

  root_len = strlen(root_dir);
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    if (root_len + 1 + strlen(entry->d_name) + 1 >= sizeof(full_path)) {
      continue;
    }
    memcpy(full_path, root_dir, root_len);
    full_path[root_len] = '/';
    strcpy(full_path + root_len + 1, entry->d_name);

    if (rel_prefix && rel_prefix[0] != '\0') {
      if (strlen(rel_prefix) + 1 + strlen(entry->d_name) + 1 >= sizeof(rel_path)) {
        continue;
      }
      strcpy(rel_path, rel_prefix);
      strcat(rel_path, "/");
      strcat(rel_path, entry->d_name);
    } else {
      if (strlen(entry->d_name) + 1 >= sizeof(rel_path)) {
        continue;
      }
      strcpy(rel_path, entry->d_name);
    }

    {
      struct stat st;
      if (stat(full_path, &st) != 0) {
        continue;
      }
      if (S_ISDIR(st.st_mode)) {
        mettle_scan_md_files(full_path, rel_path, paths_buf, paths_size, offset, count, max_files);
        continue;
      }
      if (!S_ISREG(st.st_mode)) {
        continue;
      }
    }

    name_len = strlen(entry->d_name);
    if (name_len < 4) {
      continue;
    }
    if (strcasecmp(entry->d_name + name_len - 3, ".md") != 0) {
      continue;
    }
    mettle_append_md_path(paths_buf, paths_size, offset, count, max_files, rel_path);
  }

  closedir(dir);
}

#endif

int32_t mettle_dir_list_md_files(const char *root_dir, char *paths_buf, int32_t paths_size,
                                 int32_t max_files) {
  int32_t offset = 0;
  int32_t count = 0;

  if (!root_dir || !paths_buf || paths_size <= 1 || max_files <= 0) {
    return 0;
  }

  paths_buf[0] = '\0';
  if (!mettle_dir_exists(root_dir)) {
    return 0;
  }

  mettle_scan_md_files(root_dir, "", paths_buf, paths_size, &offset, &count, max_files);
  return count;
}
