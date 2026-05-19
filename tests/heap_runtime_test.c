#include "../src/runtime/gc.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEST_ASSERT(condition, message)                                         \
  do {                                                                         \
    if (!(condition)) {                                                         \
      fprintf(stderr, "FAIL: %s\n", message);                                  \
      return 0;                                                                \
    }                                                                          \
  } while (0)

static int test_alloc_zeroes_memory_and_tracks_stats(void) {
  gc_shutdown();

  unsigned char *block = (unsigned char *)gc_alloc(32);
  TEST_ASSERT(block != NULL, "gc_alloc should return memory for nonzero size");

  for (size_t i = 0; i < 32; i++) {
    TEST_ASSERT(block[i] == 0, "gc_alloc should return zeroed memory");
  }

  memset(block, 0xA5, 32);
  TEST_ASSERT(gc_get_allocation_count() == 1,
              "allocation count should track allocated blocks");
  TEST_ASSERT(gc_get_allocated_bytes() == 32,
              "allocated bytes should track requested payload bytes");

  gc_shutdown();
  TEST_ASSERT(gc_get_allocation_count() == 0,
              "shutdown should reset allocation count");
  TEST_ASSERT(gc_get_allocated_bytes() == 0,
              "shutdown should reset allocated bytes");
  return 1;
}

static int test_collect_is_compatibility_noop(void) {
  gc_shutdown();

  void *first = gc_alloc(16);
  void *second = gc_alloc(24);
  TEST_ASSERT(first != NULL && second != NULL,
              "gc_alloc should allocate multiple blocks");
  TEST_ASSERT(gc_get_allocation_count() == 2,
              "two live allocations should be tracked");

  gc_collect(NULL);
  gc_collect_now();
  TEST_ASSERT(gc_get_allocation_count() == 2,
              "collection compatibility calls should not free allocations");
  TEST_ASSERT(gc_get_allocated_bytes() == 40,
              "collection compatibility calls should not change byte stats");

  gc_shutdown();
  return 1;
}

static int test_roots_threads_and_safepoints_are_noops(void) {
  gc_shutdown();

  void *slot = gc_alloc(8);
  gc_register_root(&slot);
  gc_safepoint(&slot);
  TEST_ASSERT(gc_thread_attach() == 0, "thread attach should succeed");
  TEST_ASSERT(gc_thread_detach() == 0, "thread detach should succeed");
  gc_unregister_root(&slot);

  TEST_ASSERT(gc_get_allocation_count() == 1,
              "compatibility no-ops should not affect allocation tracking");

  gc_shutdown();
  return 1;
}

static int test_threshold_is_diagnostic_only(void) {
  gc_shutdown();

  gc_set_collection_threshold(1);
  TEST_ASSERT(gc_get_collection_threshold() == 1,
              "threshold setter should retain the diagnostic value");

  void *a = gc_alloc(64);
  void *b = gc_alloc(64);
  TEST_ASSERT(a != NULL && b != NULL, "allocations should succeed");
  TEST_ASSERT(gc_get_allocation_count() == 2,
              "threshold should not trigger collection");
  TEST_ASSERT(gc_get_allocated_bytes() == 128,
              "threshold should not affect byte accounting");
  TEST_ASSERT(gc_get_tlab_chunk_count() == 0,
              "removed TLAB allocator should report zero chunks");

  gc_shutdown();
  TEST_ASSERT(gc_get_collection_threshold() == SIZE_MAX,
              "shutdown should reset threshold diagnostic value");
  return 1;
}

int main(void) {
  int passed = 1;
  passed &= test_alloc_zeroes_memory_and_tracks_stats();
  passed &= test_collect_is_compatibility_noop();
  passed &= test_roots_threads_and_safepoints_are_noops();
  passed &= test_threshold_is_diagnostic_only();
  gc_shutdown();

  if (!passed) {
    return 1;
  }

  printf("Heap runtime tests passed.\n");
  return 0;
}
