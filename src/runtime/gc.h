#ifndef METTLE_GC_H
#define METTLE_GC_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  const void *start_address;
  const void *end_address;
  const char *function_name;
  const char *filename;
  uintptr_t line;
  uintptr_t column;
} MethRuntimeFunctionInfo;

typedef struct {
  const void *address;
  const char *function_name;
  const char *filename;
  uintptr_t line;
  uintptr_t column;
} MethRuntimeLocationInfo;

void meth_runtime_debug_install_crash_handler(void);
void meth_runtime_debug_register_image(const MethRuntimeFunctionInfo *functions,
                                       size_t function_count,
                                       const MethRuntimeLocationInfo *locations,
                                       size_t location_count);
void meth_runtime_debug_trap(const char *message, const void *program_counter,
                             const void *frame_pointer);

/* Interlocked atomic helpers used by std/thread.mettle. */
int32_t meth_atomic_compare_exchange_i32(int32_t *target, int32_t exchange,
                                         int32_t comparand);
int32_t meth_atomic_exchange_i32(int32_t *target, int32_t value);
int32_t meth_atomic_inc_i32(int32_t *target);
int32_t meth_atomic_dec_i32(int32_t *target);

#endif /* METTLE_GC_H */
