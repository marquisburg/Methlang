/*
 * posix_helpers.c – C bridge helpers required by std/net_posix and
 * std/thread_posix on Linux and macOS.
 *
 * Compile and link this file alongside your Methlang-generated assembly:
 *
 *   Linux:
 *     gcc -o myapp output.s posix_helpers.c -lpthread
 *
 *   macOS:
 *     gcc -o myapp output.s posix_helpers.c
 *
 * If you also use the GC runtime, add gc.c:
 *     gcc -o myapp output.s posix_helpers.c src/runtime/gc.c -lpthread
 *
 * Functions provided:
 *   posix_get_errno()           – read errno (thread-local on POSIX)
 *   posix_cas_i32()             – atomic compare-and-swap (int32)
 *   posix_yield()               – yield the CPU (sched_yield)
 *   posix_atomic_exchange_i32() – atomic exchange (int32)
 *   posix_atomic_add_i32()      – atomic fetch-and-add (int32)
 */

#include <errno.h>
#include <sched.h>

/* Return the current value of errno.
 * errno is a thread-local macro on POSIX; reading it here captures the
 * value in the calling thread. */
int posix_get_errno(void) {
    return errno;
}

/* Atomic compare-and-swap.
 * Atomically: if (*ptr == expected) { *ptr = desired; return expected; }
 *             else                  { return *ptr; }
 * Returns the value that was in *ptr before the operation. */
int posix_cas_i32(int *ptr, int expected, int desired) {
    return __sync_val_compare_and_swap(ptr, expected, desired);
}

/* Yield the CPU to another runnable thread.
 * Used by spin-lock helpers to avoid burning CPU while contended. */
void posix_yield(void) {
    sched_yield();
}

/* Atomic exchange: atomically set *ptr = val, return old *ptr. */
int posix_atomic_exchange_i32(int *ptr, int val) {
    return __sync_lock_test_and_set(ptr, val);
}

/* Atomic fetch-and-add: atomically add val to *ptr, return old *ptr. */
int posix_atomic_add_i32(int *ptr, int val) {
    return __sync_fetch_and_add(ptr, val);
}
 * posix_helpers.c – C bridge helpers required by std/net_posix and
 * std/thread_posix on Linux and macOS.
 *
 * Compile and link this file alongside your Methlang-generated assembly:
 *
 *   Linux:
 *     gcc -o myapp output.s posix_helpers.c -lpthread
 *
 *   macOS:
 *     gcc -o myapp output.s posix_helpers.c
 *
 * If you also use the GC runtime, add gc.c:
 *     gcc -o myapp output.s posix_helpers.c src/runtime/gc.c -lpthread
 *
 * Functions provided:
 *   posix_get_errno()           – read errno (thread-local on POSIX)
 *   posix_cas_i32()             – atomic compare-and-swap (int32)
 *   posix_yield()               – yield the CPU (sched_yield)
 *   posix_atomic_exchange_i32() – atomic exchange (int32)
 *   posix_atomic_add_i32()      – atomic fetch-and-add (int32)
 */

#include <errno.h>
#include <sched.h>

/* Return the current value of errno.
 * errno is a thread-local macro on POSIX; reading it here captures the
 * value in the calling thread. */
int posix_get_errno(void) {
    return errno;
}

/* Atomic compare-and-swap.
 * Atomically: if (*ptr == expected) { *ptr = desired; return expected; }
 *             else                  { return *ptr; }
 * Returns the value that was in *ptr before the operation. */
int posix_cas_i32(int *ptr, int expected, int desired) {
    return __sync_val_compare_and_swap(ptr, expected, desired);
}

/* Yield the CPU to another runnable thread.
 * Used by spin-lock helpers to avoid burning CPU while contended. */
void posix_yield(void) {
    sched_yield();
}

/* Atomic exchange: atomically set *ptr = val, return old *ptr. */
int posix_atomic_exchange_i32(int *ptr, int val) {
    return __sync_lock_test_and_set(ptr, val);
}

/* Atomic fetch-and-add: atomically add val to *ptr, return old *ptr. */
int posix_atomic_add_i32(int *ptr, int val) {
    return __sync_fetch_and_add(ptr, val);
}

