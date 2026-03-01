#ifndef METHASM_GC_H
#define METHASM_GC_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Initialize the garbage collector.
 *
 * Captures the base of the stack to anchor the root scanning phase.
 * Handled automatically in the entry point built by MethASM.
 *
 * @param stack_base Pointer to the bottom (highest address) of the stack.
 */
void gc_init(void *stack_base);

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

#endif /* METHASM_GC_H */
