#include "gc.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#ifndef STATUS_HEAP_CORRUPTION
#define STATUS_HEAP_CORRUPTION ((DWORD)0xC0000374)
#endif
#ifndef STATUS_STACK_BUFFER_OVERRUN
#define STATUS_STACK_BUFFER_OVERRUN ((DWORD)0xC0000409)
#endif
#endif

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
#if defined(_WIN32) || defined(_WIN64)
static DWORD g_current_thread_tls_index = TLS_OUT_OF_INDEXES;
#else
static _Thread_local GCThread *g_current_thread = NULL;
#endif
static GCTlabChunk *g_detached_tlab_chunks = NULL;

static _Atomic unsigned int g_stw_requested = 0;
static _Atomic unsigned int g_stw_epoch = 0;
static _Atomic size_t g_stw_parked_count = 0;

static GCPageBucket *g_page_buckets[GC_PAGE_BUCKET_COUNT];
static const MethRuntimeFunctionInfo *g_runtime_debug_functions = NULL;
static size_t g_runtime_debug_function_count = 0;
static const MethRuntimeLocationInfo *g_runtime_debug_locations = NULL;
static size_t g_runtime_debug_location_count = 0;

#if defined(_WIN32) || defined(_WIN64)
static volatile LONG g_runtime_debug_handler_installed = 0;
static volatile LONG g_runtime_debug_in_handler = 0;
static PVOID g_runtime_debug_vectored_handler = NULL;

static int gc_current_thread_tls_ensure_locked(void) {
  if (g_current_thread_tls_index != TLS_OUT_OF_INDEXES) {
    return 1;
  }

  DWORD tls_index = TlsAlloc();
  if (tls_index == TLS_OUT_OF_INDEXES) {
    return 0;
  }

  g_current_thread_tls_index = tls_index;
  return 1;
}

static GCThread *gc_current_thread_get(void) {
  if (g_current_thread_tls_index == TLS_OUT_OF_INDEXES) {
    return NULL;
  }
  return (GCThread *)TlsGetValue(g_current_thread_tls_index);
}

static int gc_current_thread_set_locked(GCThread *thread) {
  if (g_current_thread_tls_index == TLS_OUT_OF_INDEXES) {
    if (!thread) {
      return 1;
    }
    if (!gc_current_thread_tls_ensure_locked()) {
      return 0;
    }
  }
  return TlsSetValue(g_current_thread_tls_index, thread) ? 1 : 0;
}

static void meth_runtime_write_stderr_bytes(const char *text, size_t length) {
  if (!text || length == 0) {
    return;
  }

  HANDLE stderr_handle = GetStdHandle(STD_ERROR_HANDLE);
  if (stderr_handle && stderr_handle != INVALID_HANDLE_VALUE) {
    DWORD written = 0;
    WriteFile(stderr_handle, text, (DWORD)length, &written, NULL);
  } else {
    OutputDebugStringA(text);
  }
}

static void meth_runtime_write_stderr(const char *text) {
  if (!text) {
    return;
  }
  meth_runtime_write_stderr_bytes(text, strlen(text));
}

static void meth_runtime_write_decimal_uintptr(uintptr_t value) {
  char buffer[32];
  size_t index = sizeof(buffer);
  buffer[--index] = '\0';

  do {
    buffer[--index] = (char)('0' + (value % 10u));
    value /= 10u;
  } while (value != 0 && index > 0);

  meth_runtime_write_stderr(buffer + index);
}

static void meth_runtime_write_hex_uintptr(uintptr_t value, size_t width) {
  static const char digits[] = "0123456789ABCDEF";
  char buffer[2 + (sizeof(uintptr_t) * 2) + 1];
  size_t index = sizeof(buffer);
  buffer[--index] = '\0';

  size_t digit_count = 0;
  do {
    buffer[--index] = digits[value & 0xFu];
    value >>= 4u;
    digit_count++;
  } while ((value != 0 || digit_count < width) && index > 2);

  buffer[--index] = 'x';
  buffer[--index] = '0';
  meth_runtime_write_stderr(buffer + index);
}

static void meth_runtime_write_pointer(const void *value) {
  meth_runtime_write_hex_uintptr((uintptr_t)value, sizeof(uintptr_t) * 2);
}

static const char *meth_runtime_exception_name(DWORD code) {
  switch (code) {
  case EXCEPTION_ACCESS_VIOLATION:
    return "access violation";
  case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    return "array bounds exceeded";
  case EXCEPTION_BREAKPOINT:
    return "breakpoint";
  case EXCEPTION_DATATYPE_MISALIGNMENT:
    return "datatype misalignment";
  case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    return "floating-point divide by zero";
  case EXCEPTION_ILLEGAL_INSTRUCTION:
    return "illegal instruction";
  case EXCEPTION_IN_PAGE_ERROR:
    return "in-page error";
  case EXCEPTION_INT_DIVIDE_BY_ZERO:
    return "integer divide by zero";
  case EXCEPTION_STACK_OVERFLOW:
    return "stack overflow";
  case STATUS_HEAP_CORRUPTION:
    return "heap corruption";
  case STATUS_STACK_BUFFER_OVERRUN:
    return "stack buffer overrun";
  default:
    return "unknown exception";
  }
}

static int meth_runtime_address_is_readable(const void *address, size_t length) {
  if (!address || length == 0) {
    return 0;
  }

  MEMORY_BASIC_INFORMATION info;
  if (VirtualQuery(address, &info, sizeof(info)) == 0) {
    return 0;
  }
  if (info.State != MEM_COMMIT) {
    return 0;
  }
  if ((info.Protect & PAGE_GUARD) != 0 || info.Protect == PAGE_NOACCESS) {
    return 0;
  }

  uintptr_t start = (uintptr_t)address;
  uintptr_t region_start = (uintptr_t)info.BaseAddress;
  uintptr_t region_end = region_start + info.RegionSize;
  return start >= region_start && start + length <= region_end;
}

static const MethRuntimeFunctionInfo *
meth_runtime_find_function(uintptr_t program_counter) {
  for (size_t i = 0; i < g_runtime_debug_function_count; i++) {
    const MethRuntimeFunctionInfo *info = &g_runtime_debug_functions[i];
    uintptr_t start = (uintptr_t)info->start_address;
    uintptr_t end = (uintptr_t)info->end_address;
    if (program_counter >= start && program_counter < end) {
      return info;
    }
  }
  return NULL;
}

static const MethRuntimeLocationInfo *
meth_runtime_find_location(uintptr_t program_counter,
                           const MethRuntimeFunctionInfo *function_info) {
  const MethRuntimeLocationInfo *best = NULL;
  uintptr_t best_address = 0;
  uintptr_t function_start = function_info ? (uintptr_t)function_info->start_address : 0;
  uintptr_t function_end =
      function_info ? (uintptr_t)function_info->end_address : UINTPTR_MAX;

  for (size_t i = 0; i < g_runtime_debug_location_count; i++) {
    const MethRuntimeLocationInfo *info = &g_runtime_debug_locations[i];
    uintptr_t address = (uintptr_t)info->address;
    if (address < function_start || address >= function_end) {
      continue;
    }
    if (address <= program_counter && (!best || address >= best_address)) {
      best = info;
      best_address = address;
    }
  }

  return best;
}

static void meth_runtime_print_frame(size_t index, uintptr_t program_counter) {
  const MethRuntimeFunctionInfo *function_info =
      meth_runtime_find_function(program_counter);
  const MethRuntimeLocationInfo *location_info =
      meth_runtime_find_location(program_counter, function_info);
  const char *function_name = "<unknown>";
  const char *filename = NULL;
  uintptr_t line = 0;
  uintptr_t column = 0;

  if (location_info) {
    function_name =
        location_info->function_name ? location_info->function_name : function_name;
    filename = location_info->filename;
    line = location_info->line;
    column = location_info->column;
  } else if (function_info) {
    function_name =
        function_info->function_name ? function_info->function_name : function_name;
    filename = function_info->filename;
    line = function_info->line;
    column = function_info->column;
  }

  if (filename && line > 0) {
    meth_runtime_write_stderr("  #");
    meth_runtime_write_decimal_uintptr((uintptr_t)index);
    meth_runtime_write_stderr(" ");
    meth_runtime_write_stderr(function_name);
    meth_runtime_write_stderr(" at ");
    meth_runtime_write_stderr(filename);
    meth_runtime_write_stderr(":");
    meth_runtime_write_decimal_uintptr(line);
    meth_runtime_write_stderr(":");
    meth_runtime_write_decimal_uintptr(column);
    meth_runtime_write_stderr(" (");
    meth_runtime_write_pointer((void *)program_counter);
    meth_runtime_write_stderr(")\r\n");
  } else {
    meth_runtime_write_stderr("  #");
    meth_runtime_write_decimal_uintptr((uintptr_t)index);
    meth_runtime_write_stderr(" ");
    meth_runtime_write_stderr(function_name);
    meth_runtime_write_stderr(" (");
    meth_runtime_write_pointer((void *)program_counter);
    meth_runtime_write_stderr(")\r\n");
  }
}

static void meth_runtime_print_trace_from_frame(uintptr_t program_counter,
                                                uintptr_t frame_pointer) {
  meth_runtime_write_stderr("Stack trace:\r\n");
  meth_runtime_print_frame(0, program_counter);

  uintptr_t current_frame = frame_pointer;
  for (size_t index = 1; index < 32; index++) {
    uintptr_t next_frame = 0;
    uintptr_t return_address = 0;

    if (!current_frame ||
        !meth_runtime_address_is_readable((const void *)current_frame,
                                          sizeof(uintptr_t) * 2)) {
      break;
    }

    next_frame = *((uintptr_t *)current_frame);
    return_address = *(((uintptr_t *)current_frame) + 1);

    if (next_frame <= current_frame || return_address == 0) {
      break;
    }

    meth_runtime_print_frame(index, return_address - 1u);
    current_frame = next_frame;
  }
}

static void meth_runtime_terminate_with_code(UINT exit_code) {
  HANDLE process = GetCurrentProcess();
  TerminateProcess(process, exit_code);
}

static LONG WINAPI
meth_runtime_unhandled_exception_filter(EXCEPTION_POINTERS *exception_info) {
  if (!exception_info || !exception_info->ExceptionRecord) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  if (InterlockedExchange(&g_runtime_debug_in_handler, 1) != 0) {
    meth_runtime_terminate_with_code(1);
  }

  const EXCEPTION_RECORD *record = exception_info->ExceptionRecord;
  const CONTEXT *context = exception_info->ContextRecord;
  uintptr_t program_counter = (uintptr_t)record->ExceptionAddress;
  uintptr_t frame_pointer = context ? (uintptr_t)context->Rbp : 0;

  meth_runtime_write_stderr("Unhandled runtime exception ");
  meth_runtime_write_hex_uintptr((uintptr_t)record->ExceptionCode, 8);
  meth_runtime_write_stderr(" (");
  meth_runtime_write_stderr(
      meth_runtime_exception_name(record->ExceptionCode));
  meth_runtime_write_stderr(")\r\n");
  meth_runtime_write_stderr("Exception address: ");
  meth_runtime_write_pointer((void *)program_counter);
  meth_runtime_write_stderr("\r\n");

  if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
      record->NumberParameters >= 2) {
    const char *operation = "read";
    if (record->ExceptionInformation[0] == 1u) {
      operation = "write";
    } else if (record->ExceptionInformation[0] == 8u) {
      operation = "execute";
    }
    meth_runtime_write_stderr(operation);
    meth_runtime_write_stderr(" access violation at ");
    meth_runtime_write_pointer((void *)record->ExceptionInformation[1]);
    meth_runtime_write_stderr("\r\n");
  }

  meth_runtime_print_trace_from_frame(program_counter, frame_pointer);
  meth_runtime_terminate_with_code(1);
  return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void meth_runtime_debug_register_image(const MethRuntimeFunctionInfo *functions,
                                       size_t function_count,
                                       const MethRuntimeLocationInfo *locations,
                                       size_t location_count) {
  g_runtime_debug_functions = functions;
  g_runtime_debug_function_count = function_count;
  g_runtime_debug_locations = locations;
  g_runtime_debug_location_count = location_count;
}

static void gc_fatal_error(const char *message) {
#if defined(_WIN32) || defined(_WIN64)
  meth_runtime_write_stderr(message ? message : "Fatal runtime error");
  meth_runtime_write_stderr("\r\n");
  meth_runtime_terminate_with_code(1);
#else
  fprintf(stderr, "%s\n", message ? message : "Fatal runtime error");
  exit(1);
#endif
}

void meth_runtime_debug_install_crash_handler(void) {
#if defined(_WIN32) || defined(_WIN64)
  if (InterlockedCompareExchange(&g_runtime_debug_handler_installed, 1, 0) == 0) {
    g_runtime_debug_vectored_handler =
        AddVectoredExceptionHandler(1, meth_runtime_unhandled_exception_filter);
    SetUnhandledExceptionFilter(meth_runtime_unhandled_exception_filter);
  }
#endif
}

void meth_runtime_debug_trap(const char *message, const void *program_counter,
                             const void *frame_pointer) {
#if defined(_WIN32) || defined(_WIN64)
  if (InterlockedExchange(&g_runtime_debug_in_handler, 1) != 0) {
    meth_runtime_terminate_with_code(1);
    return;
  }

  meth_runtime_write_stderr(message ? message : "Fatal runtime trap");
  meth_runtime_write_stderr("\r\n");
  meth_runtime_print_trace_from_frame((uintptr_t)program_counter,
                                      (uintptr_t)frame_pointer);
  meth_runtime_terminate_with_code(1);
#else
  if (message && message[0] != '\0') {
    fprintf(stderr, "%s\n", message);
  } else {
    fprintf(stderr, "Fatal runtime trap\n");
  }
  (void)program_counter;
  (void)frame_pointer;
  exit(1);
#endif
}

int32_t meth_atomic_compare_exchange_i32(int32_t *target, int32_t exchange,
                                         int32_t comparand) {
#if defined(_WIN32) || defined(_WIN64)
  return (int32_t)InterlockedCompareExchange((volatile LONG *)target,
                                             (LONG)exchange, (LONG)comparand);
#else
  return (int32_t)__sync_val_compare_and_swap(target, comparand, exchange);
#endif
}

int32_t meth_atomic_exchange_i32(int32_t *target, int32_t value) {
#if defined(_WIN32) || defined(_WIN64)
  return (int32_t)InterlockedExchange((volatile LONG *)target, (LONG)value);
#else
  return (int32_t)__sync_lock_test_and_set(target, value);
#endif
}

int32_t meth_atomic_inc_i32(int32_t *target) {
#if defined(_WIN32) || defined(_WIN64)
  return (int32_t)InterlockedIncrement((volatile LONG *)target);
#else
  return (int32_t)__sync_add_and_fetch(target, 1);
#endif
}

int32_t meth_atomic_dec_i32(int32_t *target) {
#if defined(_WIN32) || defined(_WIN64)
  return (int32_t)InterlockedDecrement((volatile LONG *)target);
#else
  return (int32_t)__sync_sub_and_fetch(target, 1);
#endif
}

static void gc_tlab_free_chunk_list(GCTlabChunk *chunk) {
  while (chunk) {
    GCTlabChunk *next = chunk->next;
    free(chunk->memory);
    free(chunk);
    chunk = next;
  }
}

#if !defined(_WIN32) && !defined(_WIN64)
static GCThread *gc_current_thread_get(void) {
  return g_current_thread;
}

static int gc_current_thread_set_locked(GCThread *thread) {
  g_current_thread = thread;
  return 1;
}
#endif

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
  GCThread *current_thread = gc_current_thread_get();
  if (current_thread) {
    return current_thread;
  }

  GCThread *thread = (GCThread *)calloc(1, sizeof(GCThread));
  if (!thread) {
    return NULL;
  }
  atomic_store_explicit(&thread->safepoint_rsp, NULL, memory_order_relaxed);
  atomic_store_explicit(&thread->parked_epoch, 0u, memory_order_relaxed);
  thread->next = g_threads;
  g_threads = thread;
  if (!gc_current_thread_set_locked(thread)) {
    g_threads = thread->next;
    free(thread);
    return NULL;
  }
  return thread;
}

static void gc_attach_current_thread_locked(void *stack_base) {
  GCThread *thread = gc_get_or_create_current_thread_locked();
  if (!thread) {
    gc_fatal_error("Fatal error: Out of memory while attaching GC thread");
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
  GCThread *thread = gc_current_thread_get();
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

  (void)gc_current_thread_set_locked(NULL);
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
    gc_fatal_error("Fatal error: Out of memory during GC mark phase");
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

  GCThread *self = gc_current_thread_get();
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
  GCThread *self = gc_current_thread_get();
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

  GCThread *thread = gc_current_thread_get();
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
    gc_unlock();
    gc_fatal_error("Fatal error: Exceeded maximum GC root slots");
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
    gc_unlock();
    gc_fatal_error("Fatal error: Allocation size overflow during gc_alloc");
  }

  GCThread *thread = gc_current_thread_get();
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
    gc_unlock();
    gc_fatal_error("Fatal error: Out of memory during gc_alloc");
  }
  if (allocated_from_tlab && !allocation_chunk) {
    gc_unlock();
    gc_fatal_error("Fatal error: Missing TLAB chunk metadata in gc_alloc");
  }

  alloc->size = size;
  alloc->marked = 0;
  alloc->from_tlab = allocated_from_tlab;
  alloc->tlab_chunk = allocated_from_tlab ? allocation_chunk : NULL;

  size_t next_allocated_bytes = 0;
  if (!gc_try_add_size(g_allocated_bytes, size, &next_allocated_bytes)) {
    gc_unlock();
    gc_fatal_error(
        "Fatal error: allocated-byte accounting overflow during gc_alloc");
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
    gc_unlock();
    gc_fatal_error("Fatal error: Out of memory while indexing GC page table");
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
  GCThread *self = gc_current_thread_get();
  if (!self || !self->attached) {
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
      if (thread == self) {
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
  (void)gc_current_thread_set_locked(NULL);
#if defined(_WIN32) || defined(_WIN64)
  if (g_current_thread_tls_index != TLS_OUT_OF_INDEXES) {
    TlsFree(g_current_thread_tls_index);
    g_current_thread_tls_index = TLS_OUT_OF_INDEXES;
  }
#endif

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

  g_runtime_debug_functions = NULL;
  g_runtime_debug_function_count = 0;
  g_runtime_debug_locations = NULL;
  g_runtime_debug_location_count = 0;

  gc_unlock();
}
