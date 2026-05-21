#ifndef METTLE_ATOMICS_H
#define METTLE_ATOMICS_H

#include <stdint.h>

/* Interlocked atomic helpers used by std/thread.mettle. Thin platform wrappers
 * over Win32 Interlocked* / GCC __sync_* intrinsics. Linked only when an object
 * actually references one of these symbols. */
int32_t mettle_atomic_compare_exchange_i32(int32_t *target, int32_t exchange,
                                         int32_t comparand);
int32_t mettle_atomic_exchange_i32(int32_t *target, int32_t value);
int32_t mettle_atomic_inc_i32(int32_t *target);
int32_t mettle_atomic_dec_i32(int32_t *target);

#endif /* METTLE_ATOMICS_H */
