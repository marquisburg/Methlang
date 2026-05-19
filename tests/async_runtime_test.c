#include "../src/runtime/gc.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#define TEST_ASSERT(cond, msg)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL: %s\n", (msg));                                    \
      return 0;                                                                \
    }                                                                          \
  } while (0)

typedef uint32_t (*MethAsyncEntryFn)(const char *ctx);

typedef struct AsyncTestContext {
  void *thread_handle;
  int32_t state;
  int32_t cancel_requested;
  int64_t result_offset;
  int64_t result_size;
  MethAsyncEntryFn entry_fn;
  int32_t arg0;
  int32_t arg1;
  int32_t result_value;
  int8_t result_end;
} AsyncTestContext;

static volatile int32_t g_active_count = 0;
static volatile int32_t g_peak_count = 0;
static volatile int32_t g_running_started = 0;
static volatile int32_t g_queued_body_runs = 0;
static volatile int32_t g_shutdown_abort_queued_runs = 0;
static volatile int32_t g_shutdown_stuck_release = 0;

static int32_t test_atomic_load_i32(volatile int32_t *target) {
  return meth_atomic_compare_exchange_i32((int32_t *)target, 0, 0);
}

static void test_atomic_store_i32(volatile int32_t *target, int32_t value) {
  (void)meth_atomic_exchange_i32((int32_t *)target, value);
}

static void test_sleep_ms(int32_t milliseconds) {
  if (milliseconds <= 0) {
    return;
  }
#ifdef _WIN32
  Sleep((DWORD)milliseconds);
#else
  usleep((unsigned int)milliseconds * 1000u);
#endif
}

static void test_init_context(AsyncTestContext *ctx, MethAsyncEntryFn entry_fn) {
  if (!ctx) {
    return;
  }
  memset(ctx, 0, sizeof(*ctx));
  ctx->result_offset = (int64_t)offsetof(AsyncTestContext, result_value);
  ctx->result_size = (int64_t)(offsetof(AsyncTestContext, result_end) -
                               offsetof(AsyncTestContext, result_value));
  ctx->entry_fn = entry_fn;
}

static void test_update_peak(int32_t observed) {
  while (1) {
    int32_t peak = test_atomic_load_i32(&g_peak_count);
    if (observed <= peak) {
      return;
    }
    if (meth_atomic_compare_exchange_i32((int32_t *)&g_peak_count, observed,
                                         peak) == peak) {
      return;
    }
  }
}

static uint32_t test_add_one_entry(const char *ctx_text) {
  AsyncTestContext *ctx = (AsyncTestContext *)ctx_text;
  ctx->result_value = ctx->arg0 + 1;
  return 0u;
}

static uint32_t test_parallel_entry(const char *ctx_text) {
  AsyncTestContext *ctx = (AsyncTestContext *)ctx_text;

  int32_t active = meth_atomic_inc_i32((int32_t *)&g_active_count);
  test_update_peak(active);

  test_sleep_ms(10);
  ctx->result_value = ctx->arg0 * 2;

  (void)meth_atomic_dec_i32((int32_t *)&g_active_count);
  return 0u;
}

static uint32_t test_running_cancel_entry(const char *ctx_text) {
  AsyncTestContext *ctx = (AsyncTestContext *)ctx_text;

  (void)meth_atomic_inc_i32((int32_t *)&g_running_started);
  while (meth_async_current_cancelled() == 0) {
    test_sleep_ms(1);
  }

  ctx->result_value = 11;
  return 0u;
}

static uint32_t test_queued_cancel_entry(const char *ctx_text) {
  AsyncTestContext *ctx = (AsyncTestContext *)ctx_text;
  (void)meth_atomic_inc_i32((int32_t *)&g_queued_body_runs);
  ctx->result_value = 99;
  return 0u;
}

static uint32_t test_deadlock_child_entry(const char *ctx_text) {
  AsyncTestContext *ctx = (AsyncTestContext *)ctx_text;
  ctx->result_value = ctx->arg0 * 3;
  return 0u;
}

static uint32_t test_deadlock_parent_entry(const char *ctx_text) {
  AsyncTestContext *ctx = (AsyncTestContext *)ctx_text;
  AsyncTestContext child;
  test_init_context(&child, test_deadlock_child_entry);
  child.arg0 = ctx->arg0;

  if (!meth_async_start((const char *)&child)) {
    ctx->result_value = -1;
    return 0u;
  }

  if (!meth_async_wait((const char *)&child)) {
    ctx->result_value = -2;
    return 0u;
  }

  ctx->result_value = child.result_value + 1;
  return 0u;
}

static uint32_t test_shutdown_short_entry(const char *ctx_text) {
  AsyncTestContext *ctx = (AsyncTestContext *)ctx_text;
  test_sleep_ms(5);
  ctx->result_value = ctx->arg0 + 10;
  return 0u;
}

static uint32_t test_shutdown_abort_queued_entry(const char *ctx_text) {
  AsyncTestContext *ctx = (AsyncTestContext *)ctx_text;
  (void)meth_atomic_inc_i32((int32_t *)&g_shutdown_abort_queued_runs);
  ctx->result_value = 777;
  return 0u;
}

static uint32_t test_shutdown_stuck_entry(const char *ctx_text) {
  AsyncTestContext *ctx = (AsyncTestContext *)ctx_text;
  while (test_atomic_load_i32(&g_shutdown_stuck_release) == 0) {
    test_sleep_ms(1);
  }
  ctx->result_value = 5;
  return 0u;
}

static int test_basic_await(void) {
  AsyncTestContext ctx;
  test_init_context(&ctx, test_add_one_entry);
  ctx.arg0 = 41;

  TEST_ASSERT(meth_async_start((const char *)&ctx) == 1, "basic await start failed");
  TEST_ASSERT(meth_async_wait((const char *)&ctx) == 1, "basic await wait failed");
  TEST_ASSERT(ctx.result_value == 42, "basic await payload mismatch");
  return 1;
}

static int test_bounded_workers_under_load(void) {
  const int32_t task_count = 64;
  AsyncTestContext *tasks =
      (AsyncTestContext *)calloc((size_t)task_count, sizeof(AsyncTestContext));
  TEST_ASSERT(tasks != NULL, "task allocation failed");

  test_atomic_store_i32(&g_active_count, 0);
  test_atomic_store_i32(&g_peak_count, 0);

  for (int32_t i = 0; i < task_count; i++) {
    test_init_context(&tasks[i], test_parallel_entry);
    tasks[i].arg0 = i;
    TEST_ASSERT(meth_async_start((const char *)&tasks[i]) == 1,
                "parallel task start failed");
  }

  for (int32_t i = 0; i < task_count; i++) {
    TEST_ASSERT(meth_async_wait((const char *)&tasks[i]) == 1,
                "parallel task wait failed");
    TEST_ASSERT(tasks[i].result_value == i * 2, "parallel task payload mismatch");
  }

  int32_t workers = meth_async_runtime_worker_count();
  int32_t total_threads = meth_async_runtime_total_worker_threads_started();
  int32_t peak = test_atomic_load_i32(&g_peak_count);

  TEST_ASSERT(workers == 1, "configured worker count should remain 1");
  TEST_ASSERT(total_threads <= workers,
              "runtime created more worker threads than configured");
  TEST_ASSERT(peak <= workers, "parallel execution exceeded worker bound");

  free(tasks);
  return 1;
}

static int test_queued_cancellation_behavior(void) {
  AsyncTestContext running;
  AsyncTestContext queued;

  test_atomic_store_i32(&g_running_started, 0);
  test_atomic_store_i32(&g_queued_body_runs, 0);

  test_init_context(&running, test_running_cancel_entry);
  test_init_context(&queued, test_queued_cancel_entry);

  TEST_ASSERT(meth_async_start((const char *)&running) == 1,
              "running cancellation task start failed");

  int started = 0;
  for (int32_t i = 0; i < 500; i++) {
    if (test_atomic_load_i32(&g_running_started) > 0) {
      started = 1;
      break;
    }
    test_sleep_ms(1);
  }
  TEST_ASSERT(started, "running cancellation task did not start");

  TEST_ASSERT(meth_async_start((const char *)&queued) == 1,
              "queued cancellation task start failed");
  TEST_ASSERT(meth_async_runtime_queued_task_count() >= 1,
              "queued task should be visible in executor queue");

  TEST_ASSERT(meth_async_cancel((const char *)&queued) == 1, "queued cancel failed");
  TEST_ASSERT(meth_async_cancel((const char *)&running) == 1, "running cancel failed");

  (void)meth_async_wait((const char *)&running);
  TEST_ASSERT(meth_async_wait((const char *)&queued) == 0,
              "queued canceled task should not report success");
  TEST_ASSERT(test_atomic_load_i32(&g_queued_body_runs) == 0,
              "queued canceled task body should not execute");
  TEST_ASSERT(queued.result_value == 0,
              "queued canceled task payload should remain zeroed");
  return 1;
}

typedef struct WaiterData {
  AsyncTestContext *future;
  volatile int32_t done;
  int32_t wait_result;
  int32_t payload;
} WaiterData;

#ifdef _WIN32
static DWORD WINAPI test_waiter_thread_main(LPVOID parameter) {
  WaiterData *data = (WaiterData *)parameter;
  if (data && data->future) {
    data->wait_result = meth_async_wait((const char *)data->future);
    data->payload = data->future->result_value;
    test_atomic_store_i32(&data->done, 1);
  }
  return 0u;
}

static int test_spawn_waiter_thread(WaiterData *data) {
  HANDLE thread = CreateThread(NULL, 0, test_waiter_thread_main, data, 0, NULL);
  if (!thread) {
    return 0;
  }
  CloseHandle(thread);
  return 1;
}
#else
static void *test_waiter_thread_main(void *parameter) {
  WaiterData *data = (WaiterData *)parameter;
  if (data && data->future) {
    data->wait_result = meth_async_wait((const char *)data->future);
    data->payload = data->future->result_value;
    test_atomic_store_i32(&data->done, 1);
  }
  return NULL;
}

static int test_spawn_waiter_thread(WaiterData *data) {
  pthread_t thread;
  if (pthread_create(&thread, NULL, test_waiter_thread_main, data) != 0) {
    return 0;
  }
  if (pthread_detach(thread) != 0) {
    return 0;
  }
  return 1;
}
#endif

static int test_nested_await_no_deadlock(void) {
  AsyncTestContext parent;
  WaiterData waiter = {0};
  test_init_context(&parent, test_deadlock_parent_entry);
  parent.arg0 = 7;

  TEST_ASSERT(meth_async_start((const char *)&parent) == 1,
              "deadlock parent start failed");
  waiter.future = &parent;
  waiter.done = 0;
  waiter.wait_result = 0;
  waiter.payload = 0;

  TEST_ASSERT(test_spawn_waiter_thread(&waiter) == 1,
              "failed to spawn deadlock waiter thread");

  int completed = 0;
  for (int32_t i = 0; i < 400; i++) {
    if (test_atomic_load_i32(&waiter.done) != 0) {
      completed = 1;
      break;
    }
    test_sleep_ms(10);
  }

  TEST_ASSERT(completed,
              "nested await did not complete; probable worker-pool deadlock");
  TEST_ASSERT(waiter.wait_result == 1, "nested await waiter reported failure");
  TEST_ASSERT(waiter.payload == 22, "nested await payload mismatch");
  return 1;
}

static int test_runtime_prepare(int32_t worker_count, int32_t queue_capacity) {
  int32_t state = meth_async_runtime_state();
  if (state == METH_ASYNC_RUNTIME_STATE_STOPPED) {
    TEST_ASSERT(meth_async_runtime_reset() == 1, "runtime reset failed");
    state = meth_async_runtime_state();
  }
  TEST_ASSERT(state == METH_ASYNC_RUNTIME_STATE_RUNNING,
              "runtime not in RUNNING state before configure");
  TEST_ASSERT(meth_async_runtime_configure(worker_count, queue_capacity) == 1,
              "runtime configure failed");
  return 1;
}

static int test_shutdown_abort_queued_tasks(void) {
  AsyncTestContext running;
  AsyncTestContext queued;
  test_init_context(&running, test_running_cancel_entry);
  test_init_context(&queued, test_shutdown_abort_queued_entry);

  test_atomic_store_i32(&g_running_started, 0);
  test_atomic_store_i32(&g_shutdown_abort_queued_runs, 0);

  TEST_ASSERT(meth_async_start((const char *)&running) == 1,
              "shutdown abort running start failed");

  int started = 0;
  for (int32_t i = 0; i < 500; i++) {
    if (test_atomic_load_i32(&g_running_started) > 0) {
      started = 1;
      break;
    }
    test_sleep_ms(1);
  }
  TEST_ASSERT(started, "shutdown abort running task did not start");

  TEST_ASSERT(meth_async_start((const char *)&queued) == 1,
              "shutdown abort queued start failed");
  TEST_ASSERT(meth_async_runtime_queued_task_count() >= 1,
              "shutdown abort queued task should be enqueued");
  TEST_ASSERT(meth_async_runtime_outstanding_task_count() >= 2,
              "shutdown abort outstanding task count mismatch");

  TEST_ASSERT(meth_async_runtime_shutdown(METH_ASYNC_SHUTDOWN_ABORT, 5000) == 1,
              "shutdown abort did not complete");
  TEST_ASSERT(meth_async_runtime_shutdown(METH_ASYNC_SHUTDOWN_ABORT, 5000) == 1,
              "shutdown abort idempotent call failed");

  TEST_ASSERT(meth_async_wait((const char *)&queued) == 0,
              "shutdown abort queued wait should fail");
  TEST_ASSERT(meth_async_wait((const char *)&running) == 0,
              "shutdown abort running wait should fail");
  TEST_ASSERT(test_atomic_load_i32(&g_shutdown_abort_queued_runs) == 0,
              "shutdown abort should purge queued task body");
  TEST_ASSERT(meth_async_runtime_state() == METH_ASYNC_RUNTIME_STATE_STOPPED,
              "shutdown abort should reach STOPPED");
  TEST_ASSERT(meth_async_runtime_live_worker_threads() == 0,
              "shutdown abort should leave zero live workers");

  AsyncTestContext rejected;
  test_init_context(&rejected, test_add_one_entry);
  rejected.arg0 = 9;
  TEST_ASSERT(meth_async_start((const char *)&rejected) == 0,
              "start should be rejected after STOPPED");
  return 1;
}

static int test_shutdown_drain_short_tasks_complete(void) {
  const int32_t task_count = 12;
  AsyncTestContext tasks[12];
  for (int32_t i = 0; i < task_count; i++) {
    test_init_context(&tasks[i], test_shutdown_short_entry);
    tasks[i].arg0 = i;
    TEST_ASSERT(meth_async_start((const char *)&tasks[i]) == 1,
                "shutdown drain short task start failed");
  }

  TEST_ASSERT(meth_async_runtime_shutdown(METH_ASYNC_SHUTDOWN_DRAIN, 5000) == 1,
              "shutdown drain should complete for short tasks");
  for (int32_t i = 0; i < task_count; i++) {
    TEST_ASSERT(meth_async_wait((const char *)&tasks[i]) == 1,
                "shutdown drain short task wait failed");
    TEST_ASSERT(tasks[i].result_value == i + 10,
                "shutdown drain short task payload mismatch");
  }
  TEST_ASSERT(meth_async_runtime_state() == METH_ASYNC_RUNTIME_STATE_STOPPED,
              "shutdown drain should end in STOPPED");
  TEST_ASSERT(meth_async_runtime_live_worker_threads() == 0,
              "shutdown drain should leave zero live workers");
  return 1;
}

static int test_shutdown_drain_timeout_then_abort(void) {
  AsyncTestContext stuck;
  test_init_context(&stuck, test_shutdown_stuck_entry);
  test_atomic_store_i32(&g_shutdown_stuck_release, 0);

  TEST_ASSERT(meth_async_start((const char *)&stuck) == 1,
              "shutdown drain timeout task start failed");

  TEST_ASSERT(meth_async_runtime_shutdown(METH_ASYNC_SHUTDOWN_DRAIN, 100) == 0,
              "shutdown drain timeout should fail for stuck task");
  TEST_ASSERT(meth_async_runtime_state() == METH_ASYNC_RUNTIME_STATE_STOPPING,
              "shutdown drain timeout should fall back to STOPPING");

  AsyncTestContext rejected;
  test_init_context(&rejected, test_add_one_entry);
  rejected.arg0 = 123;
  TEST_ASSERT(meth_async_start((const char *)&rejected) == 0,
              "start should be rejected while STOPPING");
  TEST_ASSERT(meth_async_wait((const char *)&stuck) == 0,
              "wait should fail while runtime is STOPPING");

  test_atomic_store_i32(&g_shutdown_stuck_release, 1);
  TEST_ASSERT(meth_async_runtime_shutdown(METH_ASYNC_SHUTDOWN_ABORT, 5000) == 1,
              "shutdown abort should complete after releasing stuck task");
  TEST_ASSERT(meth_async_runtime_state() == METH_ASYNC_RUNTIME_STATE_STOPPED,
              "final shutdown should reach STOPPED");
  TEST_ASSERT(meth_async_runtime_live_worker_threads() == 0,
              "final shutdown should leave zero live workers");
  return 1;
}

static void test_init_heap_runtime(void) {
#if defined(__GNUC__) || defined(__clang__)
  gc_init(__builtin_frame_address(0));
#else
  volatile uintptr_t stack_base = 0;
  gc_init((void *)&stack_base);
#endif
}

int main(void) {
  int passed = 1;

  test_init_heap_runtime();
  if (!test_runtime_prepare(1, 4)) {
    fprintf(stderr, "FAIL: unable to configure async runtime before start\n");
    return 1;
  }

  passed &= test_basic_await();
  passed &= test_bounded_workers_under_load();
  passed &= test_queued_cancellation_behavior();
  passed &= test_nested_await_no_deadlock();
  passed &= test_shutdown_abort_queued_tasks();
  passed &= test_runtime_prepare(1, 8);
  passed &= test_shutdown_drain_short_tasks_complete();
  passed &= test_runtime_prepare(1, 4);
  passed &= test_shutdown_drain_timeout_then_abort();

  if (!passed) {
    return 1;
  }

  printf("Async runtime pool tests passed.\n");
  return 0;
}
