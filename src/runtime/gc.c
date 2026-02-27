#include "gc.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct GCAllocation {
  size_t size;
  int marked;
  struct GCAllocation *next;
  // Payload follows immediately after this struct
} GCAllocation;

typedef struct GCMarkStack {
  GCAllocation **items;
  size_t count;
  size_t capacity;
} GCMarkStack;

static GCAllocation *g_allocations = NULL;
static void **g_root_slots[4096];
static size_t g_root_count = 0;
static size_t g_allocation_count = 0;
static size_t g_allocated_bytes = 0;
static uintptr_t g_heap_min = UINTPTR_MAX;
static uintptr_t g_heap_max = 0;
static size_t g_collection_threshold = 1024 * 1024; // 1 MiB default
static const size_t GC_MIN_COLLECTION_THRESHOLD = 4096;
static int g_is_collecting = 0;
static atomic_flag g_gc_lock = ATOMIC_FLAG_INIT;
static size_t g_attached_thread_count = 0;
static _Thread_local int g_thread_attached = 0;
static _Thread_local void *g_thread_stack_base = NULL;

static void gc_lock(void) {
  while (atomic_flag_test_and_set_explicit(&g_gc_lock, memory_order_acquire)) {
  }
}

static void gc_unlock(void) {
  atomic_flag_clear_explicit(&g_gc_lock, memory_order_release);
}

static int gc_try_add_size(size_t a, size_t b, size_t *out) {
  if (!out) {
    return 0;
  }
  if (SIZE_MAX - a < b) {
    return 0;
  }
  *out = a + b;
  return 1;
}

static size_t gc_saturating_mul2_size(size_t value) {
  if (value > (SIZE_MAX / 2)) {
    return SIZE_MAX;
  }
  return value * 2;
}

static uintptr_t gc_saturating_add_uintptr(uintptr_t base, size_t delta) {
  if (UINTPTR_MAX - base < delta) {
    return UINTPTR_MAX;
  }
  return base + (uintptr_t)delta;
}

static void gc_mark_stack_destroy(GCMarkStack *stack) {
  if (!stack) {
    return;
  }
  free(stack->items);
  stack->items = NULL;
  stack->count = 0;
  stack->capacity = 0;
}

static int gc_mark_stack_push(GCMarkStack *stack, GCAllocation *allocation) {
  if (!stack || !allocation) {
    return 0;
  }
  if (stack->count == stack->capacity) {
    size_t new_capacity = (stack->capacity == 0) ? 64 : stack->capacity * 2;
    if (new_capacity < stack->capacity ||
        new_capacity > (SIZE_MAX / sizeof(GCAllocation *))) {
      return 0;
    }
    GCAllocation **new_items = (GCAllocation **)realloc(
        stack->items, new_capacity * sizeof(GCAllocation *));
    if (!new_items) {
      return 0;
    }
    stack->items = new_items;
    stack->capacity = new_capacity;
  }

  stack->items[stack->count++] = allocation;
  return 1;
}

static GCAllocation *gc_mark_stack_pop(GCMarkStack *stack) {
  if (!stack || stack->count == 0) {
    return NULL;
  }
  return stack->items[--stack->count];
}

static GCAllocation *gc_find_allocation_containing(uintptr_t value) {
  GCAllocation *current = g_allocations;
  while (current) {
    uintptr_t payload_start = (uintptr_t)(current + 1);
    uintptr_t payload_end = gc_saturating_add_uintptr(payload_start, current->size);
    if (value >= payload_start && value < payload_end) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

static void gc_recompute_heap_bounds(void) {
  uintptr_t min_addr = UINTPTR_MAX;
  uintptr_t max_addr = 0;
  GCAllocation *current = g_allocations;

  while (current) {
    uintptr_t start = (uintptr_t)(current + 1);
    uintptr_t end = gc_saturating_add_uintptr(start, current->size);
    if (start < min_addr) {
      min_addr = start;
    }
    if (end > max_addr) {
      max_addr = end;
    }
    current = current->next;
  }

  g_heap_min = min_addr;
  g_heap_max = max_addr;
}

static void gc_attach_current_thread_locked(void *stack_base) {
  if (!g_thread_attached) {
    g_thread_attached = 1;
    g_attached_thread_count++;
  }
  if (stack_base) {
    g_thread_stack_base = stack_base;
  }
}

static void gc_detach_current_thread_locked(void) {
  if (!g_thread_attached) {
    return;
  }
  g_thread_attached = 0;
  g_thread_stack_base = NULL;
  if (g_attached_thread_count > 0) {
    g_attached_thread_count--;
  }
}

static void gc_collect_locked(void *current_rsp);

static void gc_maybe_collect_before_alloc(size_t requested_size) {
  if (!g_thread_stack_base) {
    return;
  }
  if (g_is_collecting) {
    return;
  }
  if (g_attached_thread_count > 1) {
    return;
  }

  size_t projected_bytes = g_allocated_bytes;
  if (!gc_try_add_size(projected_bytes, requested_size, &projected_bytes)) {
    projected_bytes = SIZE_MAX;
  }

  if (projected_bytes < g_collection_threshold) {
    return;
  }

  uintptr_t sp_marker = 0;
  gc_collect_locked((void *)&sp_marker);

  // Adapt threshold to reduce repeated collections when heap is still large.
  // Also account for pending requested bytes to avoid collecting again
  // immediately in the same allocation hot path.
  size_t next_threshold = g_allocated_bytes;
  if (!gc_try_add_size(next_threshold, requested_size, &next_threshold)) {
    next_threshold = SIZE_MAX;
  }
  size_t doubled_live_bytes = gc_saturating_mul2_size(g_allocated_bytes);
  if (doubled_live_bytes > next_threshold) {
    next_threshold = doubled_live_bytes;
  }
  if (next_threshold < GC_MIN_COLLECTION_THRESHOLD) {
    next_threshold = GC_MIN_COLLECTION_THRESHOLD;
  }
  g_collection_threshold = next_threshold;
}

void gc_register_root(void **root_slot) {
  if (!root_slot) {
    return;
  }
  gc_lock();

  for (size_t i = 0; i < g_root_count; i++) {
    if (g_root_slots[i] == root_slot) {
      gc_unlock();
      return; // Already registered
    }
  }

  if (g_root_count >= (sizeof(g_root_slots) / sizeof(g_root_slots[0]))) {
    fprintf(stderr, "Fatal error: Exceeded maximum GC root slots\n");
    gc_unlock();
    exit(1);
  }
  g_root_slots[g_root_count++] = root_slot;
  gc_unlock();
}

void gc_unregister_root(void **root_slot) {
  if (!root_slot) {
    return;
  }
  gc_lock();

  for (size_t i = 0; i < g_root_count; i++) {
    if (g_root_slots[i] == root_slot) {
      g_root_count--;
      g_root_slots[i] = g_root_slots[g_root_count];
      g_root_slots[g_root_count] = NULL;
      gc_unlock();
      return;
    }
  }
  gc_unlock();
}

void gc_init(void *stack_base) {
  gc_lock();
  gc_attach_current_thread_locked(stack_base);
  if (g_collection_threshold < GC_MIN_COLLECTION_THRESHOLD) {
    g_collection_threshold = GC_MIN_COLLECTION_THRESHOLD;
  }
  gc_unlock();
}

int32_t gc_thread_attach(void) {
  uintptr_t sp_marker = 0;
  gc_lock();
  gc_attach_current_thread_locked((void *)&sp_marker);
  gc_unlock();
  return 0;
}

int32_t gc_thread_detach(void) {
  gc_lock();
  gc_detach_current_thread_locked();
  gc_unlock();
  return 0;
}

void *gc_alloc(size_t size) {
  if (size == 0)
    return NULL;

  gc_lock();
  gc_attach_current_thread_locked(NULL);
  gc_maybe_collect_before_alloc(size);

  size_t total_size = 0;
  if (!gc_try_add_size(sizeof(GCAllocation), size, &total_size)) {
    fprintf(stderr, "Fatal error: Allocation size overflow during gc_alloc\n");
    exit(1);
  }

  GCAllocation *alloc = (GCAllocation *)malloc(total_size);
  if (!alloc && g_thread_stack_base && !g_is_collecting &&
      g_attached_thread_count <= 1) {
    // Under memory pressure, attempt one collection and retry.
    uintptr_t sp_marker = 0;
    gc_collect_locked((void *)&sp_marker);
    alloc = (GCAllocation *)malloc(total_size);
  }
  if (!alloc) {
    fprintf(stderr, "Fatal error: Out of memory during gc_alloc\n");
    gc_unlock();
    exit(1);
  }

  alloc->size = size;
  alloc->marked = 0;

  size_t next_allocated_bytes = 0;
  if (!gc_try_add_size(g_allocated_bytes, size, &next_allocated_bytes)) {
    fprintf(stderr,
            "Fatal error: allocated-byte accounting overflow during gc_alloc\n");
    gc_unlock();
    exit(1);
  }

  // Prepend to tracking list
  alloc->next = g_allocations;
  g_allocations = alloc;

  g_allocation_count++;
  g_allocated_bytes = next_allocated_bytes;

  uintptr_t payload_start = (uintptr_t)(alloc + 1);
  uintptr_t payload_end = gc_saturating_add_uintptr(payload_start, size);
  if (payload_start < g_heap_min) {
    g_heap_min = payload_start;
  }
  if (payload_end > g_heap_max) {
    g_heap_max = payload_end;
  }

  // Managed allocations start zeroed so uninitialized bytes do not retain
  // random objects during conservative scanning.
  memset((void *)(alloc + 1), 0, size);

  // The caller gets the payload memory, immediately after the header
  void *result = (void *)(alloc + 1);
  gc_unlock();
  return result;
}

// Mark a single pointer if it falls within a known allocation block.
static void mark_pointer(void *ptr, GCMarkStack *stack) {
  if (!ptr)
    return;

  uintptr_t value = (uintptr_t)ptr;
  if (g_heap_min == UINTPTR_MAX || value < g_heap_min || value >= g_heap_max) {
    return;
  }

  GCAllocation *allocation = gc_find_allocation_containing(value);
  if (!allocation || allocation->marked) {
    return;
  }

  allocation->marked = 1;
  if (!gc_mark_stack_push(stack, allocation)) {
    fprintf(stderr, "Fatal error: Out of memory during GC mark phase\n");
    exit(1);
  }
}

static void gc_scan_words(uintptr_t start, uintptr_t end_exclusive,
                          GCMarkStack *stack) {
  if (start >= end_exclusive) {
    return;
  }

  uintptr_t aligned_start =
      (start + (sizeof(void *) - 1)) & ~(uintptr_t)(sizeof(void *) - 1);
  if (aligned_start >= end_exclusive) {
    return;
  }
  if ((end_exclusive - aligned_start) < sizeof(void *)) {
    return;
  }

  void **scan_ptr = (void **)aligned_start;
  while ((uintptr_t)scan_ptr <= (end_exclusive - sizeof(void *))) {
    mark_pointer(*scan_ptr, stack);
    scan_ptr++;
  }
}

static void gc_collect_locked(void *current_rsp) {
  if (!g_thread_stack_base || !current_rsp)
    return;
  if (g_is_collecting)
    return;
  // Without stop-the-world stack capture for all threads, collecting while
  // multiple threads are attached can reclaim live objects from other stacks.
  if (g_attached_thread_count > 1)
    return;
  if (!g_allocations)
    return;

  g_is_collecting = 1;

  // Ensure stack direction is correct (stack grows downwards on x86-64)
  uintptr_t stack_start = (uintptr_t)current_rsp;
  uintptr_t stack_end = (uintptr_t)g_thread_stack_base;

  if (stack_start > stack_end) {
    uintptr_t temp = stack_start;
    stack_start = stack_end;
    stack_end = temp;
  }

  // 1. CLEAR MARKS
  GCAllocation *current = g_allocations;
  while (current) {
    current->marked = 0;
    current = current->next;
  }

  GCMarkStack mark_stack = {0};

  // 2. MARK PHASE (Conservative stack scanning)
  // Mark registered explicit roots first (globals, external runtime roots).
  for (size_t i = 0; i < g_root_count; i++) {
    void **slot = g_root_slots[i];
    if (slot) {
      mark_pointer(*slot, &mark_stack);
    }
  }

  // Conservative stack scanning.
  gc_scan_words(stack_start, stack_end, &mark_stack);

  // Traverse object graph from marked roots.
  GCAllocation *marked_allocation = NULL;
  while ((marked_allocation = gc_mark_stack_pop(&mark_stack)) != NULL) {
    uintptr_t payload_start = (uintptr_t)(marked_allocation + 1);
    uintptr_t payload_end =
        gc_saturating_add_uintptr(payload_start, marked_allocation->size);
    gc_scan_words(payload_start, payload_end, &mark_stack);
  }

  gc_mark_stack_destroy(&mark_stack);

  // 3. SWEEP PHASE
  GCAllocation **prev_ptr = &g_allocations;
  current = g_allocations;

  while (current) {
    if (!current->marked) {
      GCAllocation *unreached = current;
      current = current->next;

      *prev_ptr = current; // Remove from list

      g_allocation_count--;
      g_allocated_bytes -= unreached->size;

      free(unreached);
    } else {
      prev_ptr = &current->next;
      current = current->next;
    }
  }

  gc_recompute_heap_bounds();
  g_is_collecting = 0;
}

void gc_collect(void *current_rsp) {
  gc_lock();
  gc_attach_current_thread_locked(NULL);
  gc_collect_locked(current_rsp);
  gc_unlock();
}

void gc_collect_now(void) {
  uintptr_t sp_marker = 0;
  gc_lock();
  gc_attach_current_thread_locked((void *)&sp_marker);
  gc_collect_locked((void *)&sp_marker);
  gc_unlock();
}

void gc_set_collection_threshold(size_t bytes) {
  gc_lock();
  if (bytes < GC_MIN_COLLECTION_THRESHOLD) {
    g_collection_threshold = GC_MIN_COLLECTION_THRESHOLD;
  } else {
    g_collection_threshold = bytes;
  }
  gc_unlock();
}

size_t gc_get_collection_threshold(void) {
  gc_lock();
  size_t value = g_collection_threshold;
  gc_unlock();
  return value;
}

size_t gc_get_allocation_count(void) {
  gc_lock();
  size_t value = g_allocation_count;
  gc_unlock();
  return value;
}

size_t gc_get_allocated_bytes(void) {
  gc_lock();
  size_t value = g_allocated_bytes;
  gc_unlock();
  return value;
}

void gc_shutdown(void) {
  gc_lock();
  GCAllocation *current = g_allocations;
  while (current) {
    GCAllocation *next = current->next;
    free(current);
    current = next;
  }

  g_allocations = NULL;

  for (size_t i = 0; i < g_root_count; i++) {
    g_root_slots[i] = NULL;
  }
  g_root_count = 0;

  g_allocation_count = 0;
  g_allocated_bytes = 0;
  g_heap_min = UINTPTR_MAX;
  g_heap_max = 0;
  g_is_collecting = 0;
  g_attached_thread_count = 0;
  g_thread_attached = 0;
  g_thread_stack_base = NULL;
  g_collection_threshold = 1024 * 1024;
  gc_unlock();
}
