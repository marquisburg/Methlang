#include "gc.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GC_ROOT_SLOT_CAPACITY 4096
#define GC_PAGE_SHIFT 12
#define GC_PAGE_SIZE ((uintptr_t)1u << GC_PAGE_SHIFT)
#define GC_PAGE_BUCKET_COUNT 2048
#define GC_TLAB_DEFAULT_SIZE (64u * 1024u)
#define GC_TLAB_SMALL_ALLOC_MAX 2048u

typedef struct GCThread GCThread;
typedef struct GCTlabChunk GCTlabChunk;

typedef struct GCAllocation {
  size_t size;
  int marked;
  int from_tlab;
  GCTlabChunk *tlab_chunk;
  struct GCAllocation *next;
  // Payload follows this struct.
} GCAllocation;

typedef struct GCMarkStack {
  GCAllocation **items;
  size_t count;
  size_t capacity;
} GCMarkStack;

typedef struct GCPageAllocNode {
  GCAllocation *allocation;
  struct GCPageAllocNode *next;
} GCPageAllocNode;

typedef struct GCPageBucket {
  uintptr_t page_number;
  GCPageAllocNode *allocations;
  struct GCPageBucket *next;
} GCPageBucket;

struct GCTlabChunk {
  uint8_t *memory;
  size_t size;
  size_t live_allocation_count;
  GCTlabChunk *next;
};

struct GCThread {
  int attached;
  void *stack_base;
  _Atomic(void *) safepoint_rsp;
  _Atomic unsigned int parked_epoch;
  uint8_t *tlab_cursor;
  uint8_t *tlab_limit;
  GCTlabChunk *tlab_active_chunk;
  GCTlabChunk *tlab_chunks;
  GCThread *next;
};

static GCAllocation *g_allocations = NULL;
static void **g_root_slots[GC_ROOT_SLOT_CAPACITY];
static size_t g_root_count = 0;
static size_t g_allocation_count = 0;
static size_t g_allocated_bytes = 0;
static uintptr_t g_heap_min = UINTPTR_MAX;
static uintptr_t g_heap_max = 0;
static size_t g_collection_threshold = 1024 * 1024;
static const size_t GC_MIN_COLLECTION_THRESHOLD = 4096;
static int g_is_collecting = 0;
static atomic_flag g_gc_lock = ATOMIC_FLAG_INIT;

static GCThread *g_threads = NULL;
static size_t g_attached_thread_count = 0;
static _Thread_local GCThread *g_current_thread = NULL;
static GCTlabChunk *g_detached_tlab_chunks = NULL;

static _Atomic unsigned int g_stw_requested = 0;
static _Atomic unsigned int g_stw_epoch = 0;
static _Atomic size_t g_stw_parked_count = 0;

static GCPageBucket *g_page_buckets[GC_PAGE_BUCKET_COUNT];

static void gc_tlab_free_chunk_list(GCTlabChunk *chunk) {
  while (chunk) {
    GCTlabChunk *next = chunk->next;
    free(chunk->memory);
    free(chunk);
    chunk = next;
  }
}

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

static size_t gc_align_up(size_t value, size_t align) {
  if (align == 0) {
    return value;
  }
  size_t rem = value % align;
  if (rem == 0) {
    return value;
  }
  size_t extra = align - rem;
  if (SIZE_MAX - value < extra) {
    return SIZE_MAX;
  }
  return value + extra;
}

static uintptr_t gc_payload_start(const GCAllocation *allocation) {
  return (uintptr_t)(allocation + 1);
}

static uintptr_t gc_payload_end(const GCAllocation *allocation) {
  return gc_saturating_add_uintptr(gc_payload_start(allocation), allocation->size);
}

static size_t gc_page_hash(uintptr_t page_number) {
  return (size_t)((page_number * 11400714819323198485ull) &
                  (GC_PAGE_BUCKET_COUNT - 1));
}

static GCPageBucket *gc_page_bucket_find(uintptr_t page_number) {
  size_t bucket_index = gc_page_hash(page_number);
  GCPageBucket *bucket = g_page_buckets[bucket_index];
  while (bucket) {
    if (bucket->page_number == page_number) {
      return bucket;
    }
    bucket = bucket->next;
  }
  return NULL;
}

static GCPageBucket *gc_page_bucket_find_or_create(uintptr_t page_number) {
  size_t bucket_index = gc_page_hash(page_number);
  GCPageBucket *bucket = g_page_buckets[bucket_index];
  while (bucket) {
    if (bucket->page_number == page_number) {
      return bucket;
    }
    bucket = bucket->next;
  }

  GCPageBucket *created = (GCPageBucket *)calloc(1, sizeof(GCPageBucket));
  if (!created) {
    return NULL;
  }
  created->page_number = page_number;
  created->next = g_page_buckets[bucket_index];
  g_page_buckets[bucket_index] = created;
  return created;
}

static int gc_page_table_insert_allocation(GCAllocation *allocation) {
  if (!allocation || allocation->size == 0) {
    return 1;
  }

  uintptr_t start = gc_payload_start(allocation);
  uintptr_t end = gc_payload_end(allocation);
  if (end <= start) {
    return 0;
  }

  uintptr_t first_page = start >> GC_PAGE_SHIFT;
  uintptr_t last_page = (end - 1u) >> GC_PAGE_SHIFT;

  for (uintptr_t page = first_page; page <= last_page; page++) {
    GCPageBucket *bucket = gc_page_bucket_find_or_create(page);
    if (!bucket) {
      return 0;
    }

    GCPageAllocNode *node = bucket->allocations;
    while (node) {
      if (node->allocation == allocation) {
        break;
      }
      node = node->next;
    }
    if (node) {
      continue;
    }

    node = (GCPageAllocNode *)malloc(sizeof(GCPageAllocNode));
    if (!node) {
      return 0;
    }
    node->allocation = allocation;
    node->next = bucket->allocations;
    bucket->allocations = node;
  }

  return 1;
}

static void gc_page_table_remove_allocation(GCAllocation *allocation) {
  if (!allocation || allocation->size == 0) {
    return;
  }

  uintptr_t start = gc_payload_start(allocation);
  uintptr_t end = gc_payload_end(allocation);
  if (end <= start) {
    return;
  }

  uintptr_t first_page = start >> GC_PAGE_SHIFT;
  uintptr_t last_page = (end - 1u) >> GC_PAGE_SHIFT;

  for (uintptr_t page = first_page; page <= last_page; page++) {
    size_t bucket_index = gc_page_hash(page);
    GCPageBucket *bucket = g_page_buckets[bucket_index];
    GCPageBucket *prev_bucket = NULL;

    while (bucket) {
      if (bucket->page_number == page) {
        GCPageAllocNode **node_ptr = &bucket->allocations;
        GCPageAllocNode *node = bucket->allocations;
        while (node) {
          if (node->allocation == allocation) {
            *node_ptr = node->next;
            free(node);
            node = *node_ptr;
            continue;
          }
          node_ptr = &node->next;
          node = node->next;
        }

        if (!bucket->allocations) {
          if (prev_bucket) {
            prev_bucket->next = bucket->next;
          } else {
            g_page_buckets[bucket_index] = bucket->next;
          }
          free(bucket);
        }
        break;
      }

      prev_bucket = bucket;
      bucket = bucket->next;
    }
  }
}

static void gc_page_table_destroy(void) {
  for (size_t i = 0; i < GC_PAGE_BUCKET_COUNT; i++) {
    GCPageBucket *bucket = g_page_buckets[i];
    while (bucket) {
      GCPageBucket *next_bucket = bucket->next;
      GCPageAllocNode *node = bucket->allocations;
      while (node) {
        GCPageAllocNode *next_node = node->next;
        free(node);
        node = next_node;
      }
      free(bucket);
      bucket = next_bucket;
    }
    g_page_buckets[i] = NULL;
  }
}

static GCAllocation *gc_find_allocation_containing(uintptr_t value) {
  uintptr_t page_number = value >> GC_PAGE_SHIFT;
  GCPageBucket *bucket = gc_page_bucket_find(page_number);
  if (!bucket) {
    return NULL;
  }

  GCPageAllocNode *node = bucket->allocations;
  while (node) {
    GCAllocation *allocation = node->allocation;
    if (allocation) {
      uintptr_t payload_start = gc_payload_start(allocation);
      uintptr_t payload_end = gc_payload_end(allocation);
      if (value >= payload_start && value < payload_end) {
        return allocation;
      }
    }
    node = node->next;
  }
  return NULL;
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

static void gc_recompute_heap_bounds(void) {
  uintptr_t min_addr = UINTPTR_MAX;
  uintptr_t max_addr = 0;
  GCAllocation *current = g_allocations;

  while (current) {
    uintptr_t start = gc_payload_start(current);
    uintptr_t end = gc_payload_end(current);
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

static void gc_thread_free_tlab_chunks(GCThread *thread) {
  if (!thread) {
    return;
  }
  gc_tlab_free_chunk_list(thread->tlab_chunks);
  thread->tlab_chunks = NULL;
  thread->tlab_active_chunk = NULL;
  thread->tlab_cursor = NULL;
  thread->tlab_limit = NULL;
}

static void gc_detached_chunk_list_prepend_locked(GCTlabChunk *chunks) {
  if (!chunks) {
    return;
  }
  GCTlabChunk *tail = chunks;
  while (tail->next) {
    tail = tail->next;
  }
  tail->next = g_detached_tlab_chunks;
  g_detached_tlab_chunks = chunks;
}

static void gc_tlab_reset_live_counts_locked(void) {
  GCThread *thread = g_threads;
  while (thread) {
    GCTlabChunk *chunk = thread->tlab_chunks;
    while (chunk) {
      chunk->live_allocation_count = 0;
      chunk = chunk->next;
    }
    thread = thread->next;
  }

  GCTlabChunk *chunk = g_detached_tlab_chunks;
  while (chunk) {
    chunk->live_allocation_count = 0;
    chunk = chunk->next;
  }
}

static void gc_tlab_reclaim_empty_chunks_locked(void) {
  GCThread *thread = g_threads;
  while (thread) {
    GCTlabChunk **chunk_ptr = &thread->tlab_chunks;
    GCTlabChunk *chunk = thread->tlab_chunks;

    while (chunk) {
      if (chunk->live_allocation_count == 0) {
        GCTlabChunk *dead_chunk = chunk;
        *chunk_ptr = chunk->next;
        chunk = *chunk_ptr;

        if (thread->tlab_active_chunk == dead_chunk) {
          thread->tlab_active_chunk = NULL;
          thread->tlab_cursor = NULL;
          thread->tlab_limit = NULL;
        }

        free(dead_chunk->memory);
        free(dead_chunk);
        continue;
      }

      chunk_ptr = &chunk->next;
      chunk = chunk->next;
    }

    thread = thread->next;
  }

  GCTlabChunk **detached_chunk_ptr = &g_detached_tlab_chunks;
  GCTlabChunk *detached_chunk = g_detached_tlab_chunks;
  while (detached_chunk) {
    if (detached_chunk->live_allocation_count == 0) {
      GCTlabChunk *dead_chunk = detached_chunk;
      *detached_chunk_ptr = detached_chunk->next;
      detached_chunk = *detached_chunk_ptr;
      free(dead_chunk->memory);
      free(dead_chunk);
      continue;
    }

    detached_chunk_ptr = &detached_chunk->next;
    detached_chunk = detached_chunk->next;
  }
}

static GCThread *gc_get_or_create_current_thread_locked(void) {
  if (g_current_thread) {
    return g_current_thread;
  }

  GCThread *thread = (GCThread *)calloc(1, sizeof(GCThread));
  if (!thread) {
    return NULL;
  }
  atomic_store_explicit(&thread->safepoint_rsp, NULL, memory_order_relaxed);
  atomic_store_explicit(&thread->parked_epoch, 0u, memory_order_relaxed);
  thread->next = g_threads;
  g_threads = thread;
  g_current_thread = thread;
  return thread;
}

static void gc_attach_current_thread_locked(void *stack_base) {
  GCThread *thread = gc_get_or_create_current_thread_locked();
  if (!thread) {
    fprintf(stderr, "Fatal error: Out of memory while attaching GC thread\n");
    exit(1);
  }

  if (!thread->attached) {
    thread->attached = 1;
    g_attached_thread_count++;
  }
  if (stack_base) {
    if (!thread->stack_base ||
        (uintptr_t)stack_base > (uintptr_t)thread->stack_base) {
      thread->stack_base = stack_base;
    }
  }
}

static void gc_detach_current_thread_locked(void) {
  GCThread *thread = g_current_thread;
  if (!thread) {
    return;
  }

  if (thread->attached) {
    thread->attached = 0;
    if (g_attached_thread_count > 0) {
      g_attached_thread_count--;
    }
  }

  thread->stack_base = NULL;
  atomic_store_explicit(&thread->safepoint_rsp, NULL, memory_order_relaxed);
  atomic_store_explicit(&thread->parked_epoch, 0u, memory_order_relaxed);

  if (thread->tlab_chunks) {
    gc_detached_chunk_list_prepend_locked(thread->tlab_chunks);
    thread->tlab_chunks = NULL;
    thread->tlab_active_chunk = NULL;
    thread->tlab_cursor = NULL;
    thread->tlab_limit = NULL;
  }

  GCThread **thread_ptr = &g_threads;
  while (*thread_ptr) {
    if (*thread_ptr == thread) {
      *thread_ptr = thread->next;
      break;
    }
    thread_ptr = &(*thread_ptr)->next;
  }

  g_current_thread = NULL;
  free(thread);
}

static void *gc_tlab_allocate_locked(GCThread *thread, size_t total_size,
                                     GCTlabChunk **out_chunk) {
  if (!thread) {
    return NULL;
  }
  if (out_chunk) {
    *out_chunk = NULL;
  }

  size_t aligned = gc_align_up(total_size, sizeof(void *));
  if (aligned == SIZE_MAX) {
    return NULL;
  }

  if (thread->tlab_cursor && thread->tlab_limit && thread->tlab_active_chunk &&
      (size_t)(thread->tlab_limit - thread->tlab_cursor) >= aligned) {
    void *result = (void *)thread->tlab_cursor;
    thread->tlab_cursor += aligned;
    if (out_chunk) {
      *out_chunk = thread->tlab_active_chunk;
    }
    return result;
  }

  size_t chunk_size = GC_TLAB_DEFAULT_SIZE;
  if (chunk_size < aligned) {
    chunk_size = aligned;
  }

  GCTlabChunk *chunk = (GCTlabChunk *)malloc(sizeof(GCTlabChunk));
  if (!chunk) {
    return NULL;
  }

  chunk->memory = (uint8_t *)malloc(chunk_size);
  if (!chunk->memory) {
    free(chunk);
    return NULL;
  }

  chunk->size = chunk_size;
  chunk->live_allocation_count = 0;
  chunk->next = thread->tlab_chunks;
  thread->tlab_chunks = chunk;
  thread->tlab_active_chunk = chunk;

  thread->tlab_cursor = chunk->memory;
  thread->tlab_limit = chunk->memory + chunk->size;

  void *result = (void *)thread->tlab_cursor;
  thread->tlab_cursor += aligned;
  if (out_chunk) {
    *out_chunk = chunk;
  }
  return result;
}

static void gc_scan_words(uintptr_t start, uintptr_t end_exclusive,
                          GCMarkStack *stack);

static void mark_pointer(void *ptr, GCMarkStack *stack) {
  if (!ptr) {
    return;
  }

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

static void gc_scan_thread_stack(void *current_rsp, void *stack_base,
                                 GCMarkStack *stack) {
  if (!current_rsp || !stack_base) {
    return;
  }

  uintptr_t stack_start = (uintptr_t)current_rsp;
  uintptr_t stack_end = (uintptr_t)stack_base;
  if (stack_start > stack_end) {
    uintptr_t temp = stack_start;
    stack_start = stack_end;
    stack_end = temp;
  }
  gc_scan_words(stack_start, stack_end, stack);
}

static void gc_wait_for_world_stop(size_t expected_parked_count) {
  while (atomic_load_explicit(&g_stw_parked_count, memory_order_acquire) <
         expected_parked_count) {
  }
}

static void gc_begin_stop_the_world_locked(void *current_rsp,
                                           unsigned int *out_epoch,
                                           size_t *out_expected_parked) {
  unsigned int epoch =
      atomic_fetch_add_explicit(&g_stw_epoch, 1u, memory_order_acq_rel) + 1u;

  size_t attached_count = 0;
  GCThread *thread = g_threads;
  while (thread) {
    if (thread->attached) {
      attached_count++;
    }
    thread = thread->next;
  }

  atomic_store_explicit(&g_stw_parked_count, 0, memory_order_release);

  GCThread *self = g_current_thread;
  if (self && self->attached) {
    atomic_store_explicit(&self->safepoint_rsp, current_rsp, memory_order_release);
    atomic_store_explicit(&self->parked_epoch, epoch, memory_order_release);
    atomic_fetch_add_explicit(&g_stw_parked_count, 1, memory_order_acq_rel);
  }

  atomic_store_explicit(&g_stw_requested, 1u, memory_order_release);
  *out_epoch = epoch;
  *out_expected_parked = attached_count;
}

static void gc_end_stop_the_world_locked(void) {
  GCThread *self = g_current_thread;
  if (self) {
    atomic_store_explicit(&self->safepoint_rsp, NULL, memory_order_release);
  }
  atomic_store_explicit(&g_stw_requested, 0u, memory_order_release);
}

void gc_safepoint(void *current_rsp) {
  uintptr_t sp_marker = 0;
  if (!current_rsp) {
    current_rsp = (void *)&sp_marker;
  }

  gc_lock();
  gc_attach_current_thread_locked(current_rsp);
  gc_unlock();

  if (atomic_load_explicit(&g_stw_requested, memory_order_acquire) == 0u) {
    return;
  }

  GCThread *thread = g_current_thread;
  if (!thread || !thread->attached) {
    return;
  }

  while (atomic_load_explicit(&g_stw_requested, memory_order_acquire) != 0u) {
    unsigned int epoch = atomic_load_explicit(&g_stw_epoch, memory_order_acquire);
    unsigned int parked_epoch =
        atomic_load_explicit(&thread->parked_epoch, memory_order_acquire);
    if (parked_epoch != epoch) {
      atomic_store_explicit(&thread->safepoint_rsp, current_rsp, memory_order_release);
      atomic_store_explicit(&thread->parked_epoch, epoch, memory_order_release);
      atomic_fetch_add_explicit(&g_stw_parked_count, 1, memory_order_acq_rel);
    }
  }

  atomic_store_explicit(&thread->safepoint_rsp, NULL, memory_order_release);
}

static void gc_collect_locked(void *current_rsp);

static void gc_maybe_collect_before_alloc(size_t requested_size,
                                          void *current_rsp) {
  if (g_is_collecting) {
    return;
  }

  size_t projected_bytes = g_allocated_bytes;
  if (!gc_try_add_size(projected_bytes, requested_size, &projected_bytes)) {
    projected_bytes = SIZE_MAX;
  }

  if (projected_bytes < g_collection_threshold) {
    return;
  }

  if (!current_rsp) {
    uintptr_t sp_marker = 0;
    current_rsp = (void *)&sp_marker;
  }
  gc_collect_locked(current_rsp);

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
      return;
    }
  }

  if (g_root_count >= GC_ROOT_SLOT_CAPACITY) {
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
  gc_safepoint(NULL);
  gc_lock();
  gc_detach_current_thread_locked();
  gc_unlock();
  return 0;
}

void *gc_alloc(size_t size) {
  if (size == 0) {
    return NULL;
  }

  uintptr_t sp_marker = 0;
  gc_safepoint((void *)&sp_marker);

  gc_lock();

  while (atomic_load_explicit(&g_stw_requested, memory_order_acquire) != 0u) {
    gc_unlock();
    gc_safepoint((void *)&sp_marker);
    gc_lock();
  }

  gc_attach_current_thread_locked((void *)&sp_marker);

  // Conservative stack scanning can retain dead objects when stale pointer
  // values survive in reused stack slots across gc_alloc invocations.
  // Scrub a bounded region in this frame before deciding whether to collect.
  {
    volatile uintptr_t scrub[128];
    for (size_t i = 0; i < (sizeof(scrub) / sizeof(scrub[0])); i++) {
      scrub[i] = 0;
    }
  }

  void *collect_rsp = (void *)&sp_marker;
#if defined(__GNUC__) || defined(__clang__)
  void *frame = __builtin_frame_address(0);
  if (frame) {
    collect_rsp = frame;
  }
#endif
  gc_maybe_collect_before_alloc(size, collect_rsp);

  size_t total_size = 0;
  if (!gc_try_add_size(sizeof(GCAllocation), size, &total_size)) {
    fprintf(stderr, "Fatal error: Allocation size overflow during gc_alloc\n");
    gc_unlock();
    exit(1);
  }

  GCThread *thread = g_current_thread;
  GCAllocation *alloc = NULL;
  int use_tlab = (total_size <= GC_TLAB_SMALL_ALLOC_MAX);
  int allocated_from_tlab = 0;
  GCTlabChunk *allocation_chunk = NULL;
  if (use_tlab) {
    void *slot = gc_tlab_allocate_locked(thread, total_size, &allocation_chunk);
    if (slot) {
      alloc = (GCAllocation *)slot;
      allocated_from_tlab = 1;
    }
  }

  if (!alloc) {
    alloc = (GCAllocation *)malloc(total_size);
  }

  if (!alloc && !g_is_collecting) {
    gc_collect_locked(collect_rsp);
    if (use_tlab) {
      allocation_chunk = NULL;
      void *slot = gc_tlab_allocate_locked(thread, total_size, &allocation_chunk);
      if (slot) {
        alloc = (GCAllocation *)slot;
        allocated_from_tlab = 1;
      }
    }
    if (!alloc) {
      alloc = (GCAllocation *)malloc(total_size);
      allocated_from_tlab = 0;
    }
  }

  if (!alloc) {
    fprintf(stderr, "Fatal error: Out of memory during gc_alloc\n");
    gc_unlock();
    exit(1);
  }
  if (allocated_from_tlab && !allocation_chunk) {
    fprintf(stderr, "Fatal error: Missing TLAB chunk metadata in gc_alloc\n");
    gc_unlock();
    exit(1);
  }

  alloc->size = size;
  alloc->marked = 0;
  alloc->from_tlab = allocated_from_tlab;
  alloc->tlab_chunk = allocated_from_tlab ? allocation_chunk : NULL;

  size_t next_allocated_bytes = 0;
  if (!gc_try_add_size(g_allocated_bytes, size, &next_allocated_bytes)) {
    fprintf(stderr,
            "Fatal error: allocated-byte accounting overflow during gc_alloc\n");
    gc_unlock();
    exit(1);
  }

  alloc->next = g_allocations;
  g_allocations = alloc;
  g_allocation_count++;
  g_allocated_bytes = next_allocated_bytes;

  uintptr_t payload_start = gc_payload_start(alloc);
  uintptr_t payload_end = gc_payload_end(alloc);
  if (payload_start < g_heap_min) {
    g_heap_min = payload_start;
  }
  if (payload_end > g_heap_max) {
    g_heap_max = payload_end;
  }

  if (!gc_page_table_insert_allocation(alloc)) {
    fprintf(stderr, "Fatal error: Out of memory while indexing GC page table\n");
    gc_unlock();
    exit(1);
  }

  memset((void *)(alloc + 1), 0, size);
  void *result = (void *)(alloc + 1);
  gc_unlock();
  return result;
}

static void gc_collect_locked(void *current_rsp) {
  if (!current_rsp) {
    return;
  }
  if (g_is_collecting) {
    return;
  }
  if (!g_allocations) {
    return;
  }
  if (!g_current_thread || !g_current_thread->attached) {
    return;
  }

  g_is_collecting = 1;

  unsigned int stw_epoch = 0;
  size_t expected_parked = 0;
  gc_begin_stop_the_world_locked(current_rsp, &stw_epoch, &expected_parked);
  (void)stw_epoch;

  gc_unlock();
  gc_wait_for_world_stop(expected_parked);
  gc_lock();

  GCAllocation *current = g_allocations;
  while (current) {
    current->marked = 0;
    current = current->next;
  }

  GCMarkStack mark_stack = {0};

  for (size_t i = 0; i < g_root_count; i++) {
    void **slot = g_root_slots[i];
    if (slot) {
      mark_pointer(*slot, &mark_stack);
    }
  }

  GCThread *thread = g_threads;
  while (thread) {
    if (thread->attached && thread->stack_base) {
      void *stack_rsp = NULL;
      if (thread == g_current_thread) {
        stack_rsp = current_rsp;
      } else {
        stack_rsp =
            atomic_load_explicit(&thread->safepoint_rsp, memory_order_acquire);
      }
      if (stack_rsp) {
        gc_scan_thread_stack(stack_rsp, thread->stack_base, &mark_stack);
      }
    }
    thread = thread->next;
  }

  GCAllocation *marked_allocation = NULL;
  while ((marked_allocation = gc_mark_stack_pop(&mark_stack)) != NULL) {
    uintptr_t payload_start = gc_payload_start(marked_allocation);
    uintptr_t payload_end = gc_payload_end(marked_allocation);
    gc_scan_words(payload_start, payload_end, &mark_stack);
  }

  gc_mark_stack_destroy(&mark_stack);

  gc_tlab_reset_live_counts_locked();

  GCAllocation **prev_ptr = &g_allocations;
  current = g_allocations;

  while (current) {
    if (!current->marked) {
      GCAllocation *unreached = current;
      current = current->next;
      *prev_ptr = current;

      g_allocation_count--;
      g_allocated_bytes -= unreached->size;
      gc_page_table_remove_allocation(unreached);

      if (!unreached->from_tlab) {
        free(unreached);
      }
    } else {
      if (current->from_tlab && current->tlab_chunk) {
        current->tlab_chunk->live_allocation_count++;
      }
      prev_ptr = &current->next;
      current = current->next;
    }
  }

  gc_tlab_reclaim_empty_chunks_locked();
  gc_recompute_heap_bounds();
  gc_end_stop_the_world_locked();
  g_is_collecting = 0;
}

void gc_collect(void *current_rsp) {
  gc_lock();
  gc_attach_current_thread_locked(current_rsp);
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

size_t gc_get_tlab_chunk_count(void) {
  gc_lock();
  size_t count = 0;
  GCThread *thread = g_threads;
  while (thread) {
    GCTlabChunk *chunk = thread->tlab_chunks;
    while (chunk) {
      count++;
      chunk = chunk->next;
    }
    thread = thread->next;
  }
  GCTlabChunk *detached_chunk = g_detached_tlab_chunks;
  while (detached_chunk) {
    count++;
    detached_chunk = detached_chunk->next;
  }
  gc_unlock();
  return count;
}

void gc_shutdown(void) {
  gc_lock();

  GCAllocation *current = g_allocations;
  while (current) {
    GCAllocation *next = current->next;
    if (!current->from_tlab) {
      free(current);
    }
    current = next;
  }
  g_allocations = NULL;

  gc_page_table_destroy();

  GCThread *thread = g_threads;
  while (thread) {
    GCThread *next_thread = thread->next;
    gc_thread_free_tlab_chunks(thread);
    free(thread);
    thread = next_thread;
  }
  g_threads = NULL;
  g_current_thread = NULL;

  gc_tlab_free_chunk_list(g_detached_tlab_chunks);
  g_detached_tlab_chunks = NULL;

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
  g_collection_threshold = 1024 * 1024;

  atomic_store_explicit(&g_stw_requested, 0u, memory_order_release);
  atomic_store_explicit(&g_stw_epoch, 0u, memory_order_release);
  atomic_store_explicit(&g_stw_parked_count, 0u, memory_order_release);

  gc_unlock();
}
