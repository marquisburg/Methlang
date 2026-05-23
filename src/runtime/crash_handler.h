#ifndef METTLE_CRASH_HANDLER_H
#define METTLE_CRASH_HANDLER_H

#include <stddef.h>
#include <stdint.h>

/* Cross-platform symbolized crash-traceback support.
 *
 * The compiler emits calls to these symbols only when `-d`, `-s`, or `-g` is
 * active (or when an IR null/bounds check is lowered, which uses
 * mettle_crash_trap). Programs compiled without those flags do not link
 * this object.
 *
 * Windows uses a vectored SEH filter; POSIX uses a sigaction handler on an
 * alternate signal stack. Both produce the same symbolized backtrace format
 * from the same embedded debug-info tables. */

typedef struct {
  const void *start_address;
  const void *end_address;
  const char *function_name;
  const char *filename;
  uintptr_t line;
  uintptr_t column;
} MettleCrashFunctionInfo;

typedef struct {
  const void *address;
  const char *function_name;
  const char *filename;
  uintptr_t line;
  uintptr_t column;
} MettleCrashLocationInfo;

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
const char *mettle_crash_exception_name(DWORD code);
#endif

void mettle_crash_install(void);
void mettle_crash_register_image(const MettleCrashFunctionInfo *functions,
                                       size_t function_count,
                                       const MettleCrashLocationInfo *locations,
                                       size_t location_count);
void mettle_crash_trap(const char *message, const void *program_counter,
                             const void *frame_pointer);
void mettle_crash_write_stderr_bytes(const char *text, size_t length);
void mettle_crash_write_stderr(const char *text);

#endif /* METTLE_CRASH_HANDLER_H */
