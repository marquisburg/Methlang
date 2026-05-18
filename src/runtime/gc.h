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

/**
 * @brief Initialize the garbage collector.
 *
 * Captures the base of the stack to anchor the root scanning phase.
 * Handled automatically in the entry point built by Mettle.
 *
 * @param stack_base Pointer to the bottom (highest address) of the stack.
 */
void gc_init(void *stack_base);
void meth_runtime_debug_install_crash_handler(void);
void meth_runtime_debug_register_image(const MethRuntimeFunctionInfo *functions,
                                       size_t function_count,
                                       const MethRuntimeLocationInfo *locations,
                                       size_t location_count);
void meth_runtime_debug_trap(const char *message, const void *program_counter,
                             const void *frame_pointer);
int32_t meth_atomic_compare_exchange_i32(int32_t *target, int32_t exchange,
                                         int32_t comparand);
int32_t meth_atomic_exchange_i32(int32_t *target, int32_t value);
int32_t meth_atomic_inc_i32(int32_t *target);
int32_t meth_atomic_dec_i32(int32_t *target);
int32_t meth_async_start(const char *ctx);
int32_t meth_async_finish(const char *ctx);
int32_t meth_async_wait(const char *ctx);
int32_t meth_async_state(const char *ctx);
int32_t meth_async_cancel(const char *ctx);
int32_t meth_async_current_cancelled(void);
int32_t meth_async_runtime_configure(int32_t worker_count,
                                     int32_t queue_capacity);
int32_t meth_async_runtime_worker_count(void);
int32_t meth_async_runtime_queue_capacity(void);
int32_t meth_async_runtime_queued_task_count(void);
int32_t meth_async_runtime_total_worker_threads_started(void);
int32_t meth_async_runtime_live_worker_threads(void);
int32_t meth_async_runtime_outstanding_task_count(void);

#define METH_ASYNC_RUNTIME_STATE_RUNNING 0
#define METH_ASYNC_RUNTIME_STATE_DRAINING 1
#define METH_ASYNC_RUNTIME_STATE_STOPPING 2
#define METH_ASYNC_RUNTIME_STATE_STOPPED 3

#define METH_ASYNC_SHUTDOWN_ABORT 1
#define METH_ASYNC_SHUTDOWN_DRAIN 2

/**
 * @brief Shut down the async runtime.
 *
 * `kind` controls behavior:
 * - `METH_ASYNC_SHUTDOWN_ABORT`: reject new work, cancel/purge queued work,
 *   request cooperative cancellation for in-flight work, and stop workers.
 * - `METH_ASYNC_SHUTDOWN_DRAIN`: reject new work, wait for outstanding work to
 *   reach terminal state, then stop workers.
 *
 * `timeout_ms` is a best-effort deadline for the entire operation. Pass a
 * negative value to wait indefinitely.
 *
 * @return 1 on success, 0 on timeout/error.
 */
int32_t meth_async_runtime_shutdown(int32_t kind, int32_t timeout_ms);

/**
 * @brief Return current async runtime lifecycle state.
 */
int32_t meth_async_runtime_state(void);

/**
 * @brief Reset runtime from STOPPED back to RUNNING admission mode.
 *
 * This is intended for tests/embedders that need deterministic re-init across
 * repeated process-lifetime runs.
 */
int32_t meth_async_runtime_reset(void);

#define METH_CORO_IOCP_EVENT_WAKE 1
#define METH_CORO_IOCP_EVENT_IO 2
#define METH_CORO_IOCP_EVENT_IO_ERROR 3

/**
 * @brief Initialize the Windows IOCP reactor runtime (idempotent).
 */
int32_t meth_coro_iocp_runtime_init(void);

/**
 * @brief Shutdown the Windows IOCP reactor runtime (idempotent).
 */
int32_t meth_coro_iocp_runtime_shutdown(void);

/**
 * @brief Associate a socket/handle with the IOCP reactor.
 *
 * @param socket_handle Native socket/handle value cast to int64.
 * @param token User key returned by poll().
 */
int32_t meth_coro_iocp_runtime_register_socket(int64_t socket_handle,
                                               uintptr_t token);

/**
 * @brief Post a synthetic wake event into the reactor queue.
 */
int32_t meth_coro_iocp_runtime_post_wake(uintptr_t token, int32_t result);

/**
 * @brief Poll one IOCP event.
 *
 * Returns 1 when an event was delivered, 0 on timeout/no event.
 * On success:
 * - out_kind is one of METH_CORO_IOCP_EVENT_*.
 * - out_token is the completion key.
 * - out_result is bytes transferred or an OS error code for IO_ERROR.
 */
int32_t meth_coro_iocp_runtime_poll(int32_t timeout_ms, uintptr_t *out_token,
                                    int32_t *out_kind, int32_t *out_result);

typedef int32_t (*MethCoroStepFn)(void *state, uintptr_t wake_token,
                                  int32_t wake_kind, int32_t wake_result);

/**
 * @brief Create/destroy a coroutine task frame.
 *
 * A task is resumable. Its step function returns non-zero when complete.
 * Returns an opaque handle (pointer-sized integer cast).
 */
int64_t meth_coro_task_create(MethCoroStepFn step_fn, void *state);
int32_t meth_coro_task_destroy(int64_t task_handle);

/**
 * @brief Schedule a task for execution with wake metadata.
 */
int32_t meth_coro_task_schedule(int64_t task_handle, uintptr_t wake_token,
                                int32_t wake_kind, int32_t wake_result);

/**
 * @brief Bind an external reactor token to a task handle.
 *
 * On IOCP poll dispatch, bound tokens are translated to target tasks.
 */
int32_t meth_coro_task_bind_token(int64_t task_handle, uintptr_t token);

/**
 * @brief Run at most one resumable task.
 *
 * If no task is ready, this polls IOCP for up to timeout_ms and dispatches by
 * token (token should be a task handle when used with this scheduler).
 *
 * @return 1 if a task step ran, 0 if idle/timeout, -1 on error.
 */
int32_t meth_coro_task_run_one(int32_t timeout_ms);

/**
 * @brief Returns non-zero if task has completed.
 */
int32_t meth_coro_task_is_done(int64_t task_handle);

/**
 * @brief Attach the current thread to the GC runtime.
 *
 * Registers the calling thread so GC runtime bookkeeping remains consistent
 * when allocations happen from worker threads.
 */
int32_t gc_thread_attach(void);

/**
 * @brief Detach the current thread from the GC runtime.
 *
 * Call this before a worker thread exits.
 */
int32_t gc_thread_detach(void);

/**
 * @brief Allocate tracked memory on the heap.
 *
 * @param size The number of bytes to allocate.
 * @return void* Pointer to the allocated memory.
 */
void *gc_alloc(size_t size);

/**
 * @brief Run a garbage collection cycle.
 *
 * Performs a conservative mark-and-sweep starting from the current stack
 * pointer up to the anchor stack base.
 *
 * @param current_rsp The current stack pointer at the time of invocation.
 */
void gc_collect(void *current_rsp);

/**
 * @brief Cooperative safepoint poll for mutator threads.
 *
 * Generated code should call this at safe poll points (for example, function
 * entry) so GC can stop the world and capture each thread stack.
 *
 * @param current_rsp Approximate current stack pointer of caller frame.
 */
void gc_safepoint(void *current_rsp);

/**
 * @brief Register a pointer slot as a GC root.
 *
 * The slot should point to a variable that stores a managed pointer.
 */
void gc_register_root(void **root_slot);

/**
 * @brief Unregister a previously registered root slot.
 */
void gc_unregister_root(void **root_slot);

/**
 * @brief Run a garbage collection cycle using the current stack frame as root.
 *
 * Convenience wrapper around gc_collect().
 */
void gc_collect_now(void);

/**
 * @brief Set the auto-collection threshold in bytes.
 *
 * When tracked allocated bytes reach this threshold, gc_alloc() will attempt
 * a collection before allocating a new block.
 */
void gc_set_collection_threshold(size_t bytes);

/**
 * @brief Get the current auto-collection threshold in bytes.
 */
size_t gc_get_collection_threshold(void);

/**
 * @brief Get the number of currently tracked allocations.
 */
size_t gc_get_allocation_count(void);

/**
 * @brief Get the total bytes currently allocated and tracked by GC.
 */
size_t gc_get_allocated_bytes(void);

/**
 * @brief Get the number of currently retained TLAB chunks across threads.
 *
 * Primarily useful for runtime diagnostics and tests.
 */
size_t gc_get_tlab_chunk_count(void);

/**
 * @brief Free all tracked allocations and reset collector state.
 */
void gc_shutdown(void);

#endif /* METTLE_GC_H */
