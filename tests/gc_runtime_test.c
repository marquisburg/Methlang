#include "../src/runtime/gc.h"
#include <stdint.h>
#include <stdio.h>

#define TEST_ASSERT(cond, msg)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL: %s\n", (msg));                                    \
      return 0;                                                                \
    }                                                                          \
  } while (0)

static void collect_from_here(void) {
  volatile uintptr_t sp_marker = 0;
  gc_collect((void *)&sp_marker);
}

static void *g_explicit_root_slot = NULL;

// Perform an allocation and store the result in the global root slot from a
// separate call frame.  This prevents the gc_alloc return value from lingering
// as a stale spill in the *caller's* stack frame, which would cause a
// false-positive hit during conservative stack scanning.
static void allocate_into_global_root(size_t size) {
  g_explicit_root_slot = gc_alloc(size);
}

static void scrub_stack_words(void) {
  volatile uintptr_t scrub[256];
  for (size_t i = 0; i < (sizeof(scrub) / sizeof(scrub[0])); i++) {
    scrub[i] = 0;
  }
}

static void allocate_temporary_unreachable(size_t size) {
  volatile void *ptr = gc_alloc(size);
  (void)ptr;
}

static int test_collects_unreachable(void) {
  gc_shutdown();
  gc_set_collection_threshold(1 << 20);
#if defined(__GNUC__) || defined(__clang__)
  gc_init(__builtin_frame_address(0));
#else
  volatile uintptr_t stack_base = 0;
  gc_init((void *)&stack_base);
#endif

  allocate_temporary_unreachable(32);
  TEST_ASSERT(gc_get_allocation_count() == 1,
              "expected one tracked allocation");

  scrub_stack_words();
  collect_from_here();
  TEST_ASSERT(gc_get_allocation_count() == 0,
              "unreachable allocation should be collected");
  return 1;
}

static int test_keeps_stack_root(void) {
  gc_shutdown();
  gc_set_collection_threshold(1 << 20);
#if defined(__GNUC__) || defined(__clang__)
  gc_init(__builtin_frame_address(0));
#else
  volatile uintptr_t stack_base = 0;
  gc_init((void *)&stack_base);
#endif

  volatile void *root = gc_alloc(24);
  TEST_ASSERT(root != NULL, "root allocation failed");

  collect_from_here();
  TEST_ASSERT(gc_get_allocation_count() == 1,
              "rooted allocation should survive collection");

  root = NULL;
  scrub_stack_words();
  collect_from_here();
  TEST_ASSERT(gc_get_allocation_count() == 0,
              "allocation should collect after root is removed");
  return 1;
}

static int test_transitive_marking(void) {
  gc_shutdown();
  gc_set_collection_threshold(1 << 20);
#if defined(__GNUC__) || defined(__clang__)
  gc_init(__builtin_frame_address(0));
#else
  volatile uintptr_t stack_base = 0;
  gc_init((void *)&stack_base);
#endif

  void **parent = (void **)gc_alloc(sizeof(void *));
  void *child = gc_alloc(16);
  TEST_ASSERT(parent && child, "transitive allocations failed");

  parent[0] = child;
  child = NULL;

  {
    volatile void *parent_root = parent;
    (void)parent_root;
    collect_from_here();
  }
  TEST_ASSERT(gc_get_allocation_count() == 2,
              "child reachable via parent should be retained");

  parent[0] = NULL;
  scrub_stack_words();
  {
    volatile void *parent_root = parent;
    (void)parent_root;
    collect_from_here();
  }
  TEST_ASSERT(gc_get_allocation_count() == 1,
              "child should collect when parent no longer references it");

  parent = NULL;
  scrub_stack_words();
  collect_from_here();
  TEST_ASSERT(gc_get_allocation_count() <= 1,
              "parent should no longer keep additional objects alive");
  return 1;
}

static int test_interior_pointer_root(void) {
  gc_shutdown();
  gc_set_collection_threshold(1 << 20);
#if defined(__GNUC__) || defined(__clang__)
  gc_init(__builtin_frame_address(0));
#else
  volatile uintptr_t stack_base = 0;
  gc_init((void *)&stack_base);
#endif

  char *block = (char *)gc_alloc(32);
  TEST_ASSERT(block != NULL, "block allocation failed");

  volatile char *interior = block + 5;
  block = NULL;

  {
    volatile const void *interior_root = interior;
    (void)interior_root;
    collect_from_here();
  }
  TEST_ASSERT(gc_get_allocation_count() == 1,
              "interior pointer should retain allocation");

  interior = NULL;
  scrub_stack_words();
  collect_from_here();
  TEST_ASSERT(gc_get_allocation_count() <= 1,
              "interior-rooted block should become collectable");
  return 1;
}

static int test_auto_collection_threshold(void) {
  gc_shutdown();
  gc_set_collection_threshold(4096);
#if defined(__GNUC__) || defined(__clang__)
  gc_init(__builtin_frame_address(0));
#else
  volatile uintptr_t stack_base = 0;
  gc_init((void *)&stack_base);
#endif

  for (int i = 0; i < 320; i++) {
    allocate_temporary_unreachable(64);
  }

  // Auto collection should have run at least once; we should not retain all
  // temporary blocks.
  TEST_ASSERT(gc_get_allocation_count() < 320,
              "auto collection did not run as expected");

  scrub_stack_words();
  collect_from_here();
  TEST_ASSERT(gc_get_allocation_count() < 320,
              "explicit collection should not retain all temporary blocks");
  return 1;
}

static int test_auto_collection_projected_threshold(void) {
  gc_shutdown();
  gc_set_collection_threshold(4096);
#if defined(__GNUC__) || defined(__clang__)
  gc_init(__builtin_frame_address(0));
#else
  volatile uintptr_t stack_base = 0;
  gc_init((void *)&stack_base);
#endif

  allocate_temporary_unreachable(4000);
  TEST_ASSERT(gc_get_allocation_count() == 1,
              "expected one pre-threshold temporary allocation");

  scrub_stack_words();
  volatile void *live = gc_alloc(200);
  TEST_ASSERT(live != NULL, "second allocation failed");
  TEST_ASSERT(
      gc_get_allocation_count() == 1,
      "projected-threshold collection should reclaim unreachable blocks "
      "before allocating");

  live = NULL;
  scrub_stack_words();
  collect_from_here();
  TEST_ASSERT(gc_get_allocation_count() <= 1,
              "live allocation should become collectable after root is "
              "cleared");
  return 1;
}

// Phase-1 helper: allocate into the global root, verify it survives a first
// collection, then return.  Keeping this in a separate call frame means the
// gc_alloc return value and the pointer loaded by TEST_ASSERT are both gone
// from the stack once this function returns.
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
static int
test_explicit_root_phase1(void) {
  allocate_into_global_root(40);
  if (g_explicit_root_slot == NULL) {
    fprintf(stderr, "FAIL: %s\n", "explicit root allocation failed");
    return 0;
  }

  scrub_stack_words();
  collect_from_here();
  if (gc_get_allocation_count() != 1) {
    fprintf(stderr, "FAIL: %s\n",
            "explicitly registered root should retain allocation");
    return 0;
  }
  return 1;
}

static int test_explicit_registered_root(void) {
  gc_shutdown();
  gc_set_collection_threshold(1 << 20);
#if defined(__GNUC__) || defined(__clang__)
  gc_init(__builtin_frame_address(0));
#else
  volatile uintptr_t stack_base = 0;
  gc_init((void *)&stack_base);
#endif

  gc_register_root(&g_explicit_root_slot);

  // Phase 1 – allocate and prove the root keeps it alive.
  if (!test_explicit_root_phase1()) {
    return 0;
  }

  // Phase 2 – unregister the root, clear the global, collect.
  // Because phase-1 ran in a callee frame, no stale pointer remains here.
  gc_unregister_root(&g_explicit_root_slot);
  g_explicit_root_slot = NULL;

  // Aggressively scrub the stack to kill any stale pointer copies that may
  // have been left by callee-save register spills or ABI shadow space.
  {
    volatile uintptr_t scrub[512];
    for (size_t i = 0; i < (sizeof(scrub) / sizeof(scrub[0])); i++) {
      scrub[i] = 0;
    }
  }

  {
    volatile uintptr_t sp_marker = 0;
    gc_collect((void *)&sp_marker);
  }
  TEST_ASSERT(gc_get_allocation_count() == 0,
              "allocation should collect after unregistering explicit root");
  return 1;
}

int main(void) {
  int passed = 1;

  passed &= test_collects_unreachable();
  passed &= test_keeps_stack_root();
  passed &= test_transitive_marking();
  passed &= test_interior_pointer_root();
  passed &= test_auto_collection_threshold();
  passed &= test_auto_collection_projected_threshold();
  passed &= test_explicit_registered_root();

  gc_shutdown();

  if (!passed) {
    return 1;
  }

  printf("GC runtime tests passed.\n");
  return 0;
}
