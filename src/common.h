#ifndef METTLE_COMMON_H
#define METTLE_COMMON_H

#include <stddef.h>
#include <stdarg.h>

#define METTLE_FNV1A_OFFSET_BASIS ((size_t)1469598103934665603ULL)
#define METTLE_FNV1A_PRIME        ((size_t)1099511628211ULL)

char *mettle_strdup(const char *text);
size_t mettle_fnv1a_hash(const char *str);
void mettle_set_error(char **dest, const char *fmt, ...);
void mettle_free_string(char *str);
void mettle_free_string_array(char **values, size_t count);

#endif
