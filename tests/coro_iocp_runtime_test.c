#include "../src/runtime/gc.h"
#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define TEST_ASSERT(cond, msg)                                                   \
  do {                                                                           \
    if (!(cond)) {                                                               \
      fprintf(stderr, "FAIL: %s\n", (msg));                                    \
      return 0;                                                                  \
    }                                                                            \
  } while (0)

typedef struct PollResult {
  volatile LONG done;
  int32_t poll_rc;
  int32_t kind;
  int32_t result;
  uintptr_t token;
} PollResult;

typedef struct CoroState {
  int64_t task_handle;
  int32_t phase;
  int32_t saw_error;
  int32_t final_value;
  int32_t saw_kind;
  int32_t saw_result;
  uintptr_t saw_token;
} CoroState;

static DWORD WINAPI poll_thread_main(LPVOID parameter) {
  PollResult *poll = (PollResult *)parameter;
  if (!poll) {
    return 0u;
  }

  poll->poll_rc =
      meth_coro_iocp_runtime_poll(1500, &poll->token, &poll->kind, &poll->result);
  InterlockedExchange(&poll->done, 1);
  return 0u;
}

static int test_iocp_init_shutdown_idempotent(void) {
  TEST_ASSERT(meth_coro_iocp_runtime_init() == 1, "iocp init failed");
  TEST_ASSERT(meth_coro_iocp_runtime_init() == 1, "iocp second init failed");
  TEST_ASSERT(meth_coro_iocp_runtime_shutdown() == 1, "iocp shutdown failed");
  TEST_ASSERT(meth_coro_iocp_runtime_shutdown() == 1,
              "iocp second shutdown failed");
  return 1;
}

static int test_iocp_poll_wake(void) {
  PollResult poll = {0};
  HANDLE thread = NULL;

  TEST_ASSERT(meth_coro_iocp_runtime_init() == 1,
              "iocp init failed before poll wake test");

  thread = CreateThread(NULL, 0, poll_thread_main, &poll, 0, NULL);
  TEST_ASSERT(thread != NULL, "failed to create poll thread");

  Sleep(20);
  TEST_ASSERT(meth_coro_iocp_runtime_post_wake((uintptr_t)0x1234u, 77) == 1,
              "post wake failed");

  TEST_ASSERT(WaitForSingleObject(thread, 3000) == WAIT_OBJECT_0,
              "poll thread did not finish");
  CloseHandle(thread);

  TEST_ASSERT(poll.poll_rc == 1, "poll did not return event");
  TEST_ASSERT(poll.kind == METH_CORO_IOCP_EVENT_WAKE,
              "poll returned unexpected event kind");
  TEST_ASSERT(poll.result == 77, "poll returned unexpected result value");
  TEST_ASSERT(poll.token == (uintptr_t)0x1234u,
              "poll returned unexpected token");

  TEST_ASSERT(meth_coro_iocp_runtime_shutdown() == 1,
              "iocp shutdown failed after poll wake test");
  TEST_ASSERT(meth_coro_iocp_runtime_post_wake((uintptr_t)1u, 1) == 0,
              "post wake should fail after shutdown");
  return 1;
}

static int test_iocp_poll_timeout(void) {
  int32_t kind = 0;
  int32_t result = 0;
  uintptr_t token = 0;

  TEST_ASSERT(meth_coro_iocp_runtime_init() == 1,
              "iocp init failed before timeout test");
  TEST_ASSERT(meth_coro_iocp_runtime_poll(10, &token, &kind, &result) == 0,
              "poll timeout should return 0");
  TEST_ASSERT(meth_coro_iocp_runtime_shutdown() == 1,
              "iocp shutdown failed after timeout test");
  return 1;
}

static int32_t test_coro_step(void *state_ptr, uintptr_t wake_token,
                              int32_t wake_kind, int32_t wake_result) {
  CoroState *state = (CoroState *)state_ptr;
  if (!state) {
    return 1;
  }

  if (state->phase == 0) {
    state->phase = 1;
    if (meth_coro_iocp_runtime_post_wake((uintptr_t)state->task_handle, 55) != 1) {
      state->saw_error = 1;
      return 1;
    }
    return 0;
  }

  if (wake_token != (uintptr_t)state->task_handle ||
      wake_kind != METH_CORO_IOCP_EVENT_WAKE || wake_result != 55) {
    state->saw_error = 1;
    return 1;
  }

  state->final_value = 123;
  state->saw_token = wake_token;
  state->saw_kind = wake_kind;
  state->saw_result = wake_result;
  state->phase = 2;
  return 1;
}

static int32_t test_coro_bind_step(void *state_ptr, uintptr_t wake_token,
                                   int32_t wake_kind, int32_t wake_result) {
  CoroState *state = (CoroState *)state_ptr;
  if (!state) {
    return 1;
  }

  if (state->phase == 0) {
    state->phase = 1;
    if (meth_coro_iocp_runtime_post_wake((uintptr_t)0xBEEFu, 88) != 1) {
      state->saw_error = 1;
      return 1;
    }
    return 0;
  }

  state->saw_token = wake_token;
  state->saw_kind = wake_kind;
  state->saw_result = wake_result;
  state->phase = 2;
  return 1;
}

static int32_t test_coro_coalesce_step(void *state_ptr, uintptr_t wake_token,
                                       int32_t wake_kind, int32_t wake_result) {
  CoroState *state = (CoroState *)state_ptr;
  if (!state) {
    return 1;
  }
  state->saw_token = wake_token;
  state->saw_kind = wake_kind;
  state->saw_result = wake_result;
  state->phase = state->phase + 1;
  return 1;
}

static int test_stackless_task_resume_via_iocp(void) {
  CoroState state = {0};
  int spins = 0;

  TEST_ASSERT(meth_coro_iocp_runtime_init() == 1,
              "iocp init failed before stackless task test");

  state.task_handle = meth_coro_task_create(test_coro_step, &state);
  TEST_ASSERT(state.task_handle != 0, "failed to create coroutine task");
  TEST_ASSERT(meth_coro_task_schedule(state.task_handle, 0, 0, 0) == 1,
              "failed to schedule initial coroutine step");

  while (!meth_coro_task_is_done(state.task_handle) && spins < 20) {
    int32_t rc = meth_coro_task_run_one(1000);
    TEST_ASSERT(rc >= 0, "task runner returned error");
    spins++;
  }

  TEST_ASSERT(meth_coro_task_is_done(state.task_handle) == 1,
              "coroutine task did not complete");
  TEST_ASSERT(state.saw_error == 0, "coroutine task saw wake metadata mismatch");
  TEST_ASSERT(state.phase == 2, "coroutine task final phase mismatch");
  TEST_ASSERT(state.final_value == 123, "coroutine task final value mismatch");
  TEST_ASSERT(state.saw_token == (uintptr_t)state.task_handle,
              "coroutine task saw unexpected token");
  TEST_ASSERT(state.saw_kind == METH_CORO_IOCP_EVENT_WAKE,
              "coroutine task saw unexpected kind");
  TEST_ASSERT(state.saw_result == 55, "coroutine task saw unexpected result");

  TEST_ASSERT(meth_coro_task_destroy(state.task_handle) == 1,
              "failed to destroy coroutine task");
  TEST_ASSERT(meth_coro_iocp_runtime_shutdown() == 1,
              "iocp shutdown failed after stackless task test");
  return 1;
}

static int test_stackless_task_bind_external_token(void) {
  CoroState state = {0};
  int spins = 0;

  TEST_ASSERT(meth_coro_iocp_runtime_init() == 1,
              "iocp init failed before bind-token task test");

  state.task_handle = meth_coro_task_create(test_coro_bind_step, &state);
  TEST_ASSERT(state.task_handle != 0, "failed to create bind-token task");
  TEST_ASSERT(meth_coro_task_bind_token(state.task_handle, (uintptr_t)0xBEEFu) == 1,
              "failed to bind external token to task");
  TEST_ASSERT(meth_coro_task_schedule(state.task_handle, 0, 0, 0) == 1,
              "failed to schedule bind-token task start");

  while (!meth_coro_task_is_done(state.task_handle) && spins < 20) {
    int32_t rc = meth_coro_task_run_one(1000);
    TEST_ASSERT(rc >= 0, "task runner returned error in bind-token test");
    spins++;
  }

  TEST_ASSERT(meth_coro_task_is_done(state.task_handle) == 1,
              "bind-token task did not complete");
  TEST_ASSERT(state.saw_error == 0, "bind-token task reported runtime error");
  TEST_ASSERT(state.saw_token == (uintptr_t)0xBEEFu,
              "bind-token task received wrong token");
  TEST_ASSERT(state.saw_kind == METH_CORO_IOCP_EVENT_WAKE,
              "bind-token task received wrong event kind");
  TEST_ASSERT(state.saw_result == 88, "bind-token task received wrong result");

  TEST_ASSERT(meth_coro_task_destroy(state.task_handle) == 1,
              "failed to destroy bind-token task");
  TEST_ASSERT(meth_coro_iocp_runtime_shutdown() == 1,
              "iocp shutdown failed after bind-token task test");
  return 1;
}

static int test_task_schedule_coalesces_latest_wake(void) {
  CoroState state = {0};
  TEST_ASSERT(meth_coro_iocp_runtime_init() == 1,
              "iocp init failed before coalesce test");

  state.task_handle = meth_coro_task_create(test_coro_coalesce_step, &state);
  TEST_ASSERT(state.task_handle != 0, "failed to create coalesce task");
  TEST_ASSERT(meth_coro_task_schedule(state.task_handle, 11u, 7, 41) == 1,
              "first coalesce schedule failed");
  TEST_ASSERT(meth_coro_task_schedule(state.task_handle, 22u, 9, 99) == 1,
              "second coalesce schedule failed");
  TEST_ASSERT(meth_coro_task_run_one(0) == 1, "coalesce run did not execute");

  TEST_ASSERT(state.phase == 1, "coalesce task should run exactly once");
  TEST_ASSERT(state.saw_token == 22u, "coalesce task did not see latest token");
  TEST_ASSERT(state.saw_kind == 9, "coalesce task did not see latest kind");
  TEST_ASSERT(state.saw_result == 99, "coalesce task did not see latest result");
  TEST_ASSERT(meth_coro_task_is_done(state.task_handle) == 1,
              "coalesce task should be done");

  TEST_ASSERT(meth_coro_task_destroy(state.task_handle) == 1,
              "failed to destroy coalesce task");
  TEST_ASSERT(meth_coro_iocp_runtime_shutdown() == 1,
              "iocp shutdown failed after coalesce test");
  return 1;
}

int main(void) {
  int passed = 1;

  passed &= test_iocp_init_shutdown_idempotent();
  passed &= test_iocp_poll_wake();
  passed &= test_iocp_poll_timeout();
  passed &= test_stackless_task_resume_via_iocp();
  passed &= test_stackless_task_bind_external_token();
  passed &= test_task_schedule_coalesces_latest_wake();

  if (!passed) {
    return 1;
  }

  printf("Coroutine IOCP runtime tests passed.\n");
  return 0;
}

#else
int main(void) {
  printf("Coroutine IOCP runtime tests skipped (non-Windows).\n");
  return 0;
}
#endif
