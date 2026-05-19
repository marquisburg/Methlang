#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "gc.h"
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>
#endif

int32_t __meth_async_start(const char *ctx) {
  return meth_async_start(ctx);
}

int32_t __meth_async_finish(const char *ctx) {
  return meth_async_finish(ctx);
}

int32_t __meth_async_wait(const char *ctx) {
  return meth_async_wait(ctx);
}

int32_t __meth_async_state(const char *ctx) {
  return meth_async_state(ctx);
}

int32_t __meth_async_cancel(const char *ctx) {
  return meth_async_cancel(ctx);
}

int32_t __meth_async_current_cancelled(void) {
  return meth_async_current_cancelled();
}

int32_t __meth_coro_iocp_runtime_init(void) {
  return meth_coro_iocp_runtime_init();
}

int32_t __meth_coro_iocp_runtime_shutdown(void) {
  return meth_coro_iocp_runtime_shutdown();
}

int32_t __meth_coro_iocp_runtime_register_socket(int64_t socket_handle,
                                                 uintptr_t token) {
  return meth_coro_iocp_runtime_register_socket(socket_handle, token);
}

int32_t __meth_coro_iocp_runtime_post_wake(uintptr_t token, int32_t result) {
  return meth_coro_iocp_runtime_post_wake(token, result);
}

int32_t __meth_coro_iocp_runtime_poll(int32_t timeout_ms, uintptr_t *out_token,
                                      int32_t *out_kind, int32_t *out_result) {
  return meth_coro_iocp_runtime_poll(timeout_ms, out_token, out_kind, out_result);
}

int64_t __meth_coro_task_create(MethCoroStepFn step_fn, void *state) {
  return meth_coro_task_create(step_fn, state);
}

int32_t __meth_coro_task_destroy(int64_t task_handle) {
  return meth_coro_task_destroy(task_handle);
}

int32_t __meth_coro_task_schedule(int64_t task_handle, uintptr_t wake_token,
                                  int32_t wake_kind, int32_t wake_result) {
  return meth_coro_task_schedule(task_handle, wake_token, wake_kind, wake_result);
}

int32_t __meth_coro_task_bind_token(int64_t task_handle, uintptr_t token) {
  return meth_coro_task_bind_token(task_handle, token);
}

int32_t __meth_coro_task_run_one(int32_t timeout_ms) {
  return meth_coro_task_run_one(timeout_ms);
}

int32_t __meth_coro_task_is_done(int64_t task_handle) {
  return meth_coro_task_is_done(task_handle);
}

typedef uint32_t (*MethAsyncEntryFn)(const char *ctx);

#ifdef _WIN32
typedef struct MethCoroIocpRuntime {
  SRWLOCK lock;
  HANDLE port;
} MethCoroIocpRuntime;

static MethCoroIocpRuntime g_meth_coro_iocp_runtime = {SRWLOCK_INIT, NULL};

typedef struct MethCoroTask {
  MethCoroStepFn step_fn;
  void *state;
  int32_t done;
  int32_t queued;
  uintptr_t wake_token;
  int32_t wake_kind;
  int32_t wake_result;
  struct MethCoroTask *next_ready;
  struct MethCoroTask *next_all;
} MethCoroTask;

typedef struct MethCoroTokenBinding {
  uintptr_t token;
  int64_t task_handle;
  struct MethCoroTokenBinding *next;
} MethCoroTokenBinding;

typedef struct MethCoroTaskRuntime {
  SRWLOCK lock;
  MethCoroTask *all_head;
  MethCoroTask *ready_head;
  MethCoroTask *ready_tail;
  MethCoroTokenBinding *token_bindings;
} MethCoroTaskRuntime;

static MethCoroTaskRuntime g_meth_coro_task_runtime = {SRWLOCK_INIT, NULL, NULL,
                                                       NULL, NULL};

static MethCoroTask *meth_coro_find_task_locked(int64_t task_handle) {
  MethCoroTask *task = g_meth_coro_task_runtime.all_head;
  while (task) {
    if ((int64_t)(intptr_t)task == task_handle) {
      return task;
    }
    task = task->next_all;
  }
  return NULL;
}

static int64_t meth_coro_resolve_task_from_token_locked(uintptr_t token) {
  MethCoroTokenBinding *binding = g_meth_coro_task_runtime.token_bindings;
  while (binding) {
    if (binding->token == token) {
      return binding->task_handle;
    }
    binding = binding->next;
  }
  return (int64_t)(intptr_t)token;
}

int32_t meth_coro_iocp_runtime_init(void) {
  AcquireSRWLockExclusive(&g_meth_coro_iocp_runtime.lock);
  if (g_meth_coro_iocp_runtime.port) {
    ReleaseSRWLockExclusive(&g_meth_coro_iocp_runtime.lock);
    return 1;
  }

  HANDLE port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
  if (!port) {
    ReleaseSRWLockExclusive(&g_meth_coro_iocp_runtime.lock);
    return 0;
  }

  g_meth_coro_iocp_runtime.port = port;
  ReleaseSRWLockExclusive(&g_meth_coro_iocp_runtime.lock);
  return 1;
}

int32_t meth_coro_iocp_runtime_shutdown(void) {
  HANDLE port = NULL;
  AcquireSRWLockExclusive(&g_meth_coro_iocp_runtime.lock);
  port = g_meth_coro_iocp_runtime.port;
  g_meth_coro_iocp_runtime.port = NULL;
  ReleaseSRWLockExclusive(&g_meth_coro_iocp_runtime.lock);

  if (port) {
    (void)CloseHandle(port);
  }
  return 1;
}

int32_t meth_coro_iocp_runtime_register_socket(int64_t socket_handle,
                                               uintptr_t token) {
  HANDLE port = NULL;
  AcquireSRWLockExclusive(&g_meth_coro_iocp_runtime.lock);
  port = g_meth_coro_iocp_runtime.port;
  ReleaseSRWLockExclusive(&g_meth_coro_iocp_runtime.lock);
  if (!port) {
    return 0;
  }

  HANDLE associated =
      CreateIoCompletionPort((HANDLE)(uintptr_t)socket_handle, port,
                             (ULONG_PTR)token, 0);
  return associated != NULL;
}

int32_t meth_coro_iocp_runtime_post_wake(uintptr_t token, int32_t result) {
  HANDLE port = NULL;
  AcquireSRWLockExclusive(&g_meth_coro_iocp_runtime.lock);
  port = g_meth_coro_iocp_runtime.port;
  ReleaseSRWLockExclusive(&g_meth_coro_iocp_runtime.lock);
  if (!port) {
    return 0;
  }

  return PostQueuedCompletionStatus(port, (DWORD)result, (ULONG_PTR)token, NULL) !=
         0;
}

int32_t meth_coro_iocp_runtime_poll(int32_t timeout_ms, uintptr_t *out_token,
                                    int32_t *out_kind, int32_t *out_result) {
  HANDLE port = NULL;
  AcquireSRWLockExclusive(&g_meth_coro_iocp_runtime.lock);
  port = g_meth_coro_iocp_runtime.port;
  ReleaseSRWLockExclusive(&g_meth_coro_iocp_runtime.lock);
  if (!port) {
    return 0;
  }

  DWORD bytes = 0;
  ULONG_PTR key = 0;
  LPOVERLAPPED overlapped = NULL;
  DWORD wait_ms = (timeout_ms < 0) ? INFINITE : (DWORD)timeout_ms;
  BOOL ok = GetQueuedCompletionStatus(port, &bytes, &key, &overlapped, wait_ms);
  if (!ok && overlapped == NULL) {
    DWORD err = GetLastError();
    if (err == WAIT_TIMEOUT || err == ERROR_ABANDONED_WAIT_0) {
      return 0;
    }
    if (out_kind) {
      *out_kind = METH_CORO_IOCP_EVENT_IO_ERROR;
    }
    if (out_result) {
      *out_result = (int32_t)err;
    }
    if (out_token) {
      *out_token = (uintptr_t)key;
    }
    return 1;
  }

  if (out_token) {
    *out_token = (uintptr_t)key;
  }
  if (ok) {
    if (out_kind) {
      *out_kind =
          (overlapped == NULL) ? METH_CORO_IOCP_EVENT_WAKE : METH_CORO_IOCP_EVENT_IO;
    }
    if (out_result) {
      *out_result = (int32_t)bytes;
    }
    return 1;
  }

  if (out_kind) {
    *out_kind = METH_CORO_IOCP_EVENT_IO_ERROR;
  }
  if (out_result) {
    *out_result = (int32_t)GetLastError();
  }
  return 1;
}

int64_t meth_coro_task_create(MethCoroStepFn step_fn, void *state) {
  if (!step_fn) {
    return 0;
  }

  MethCoroTask *task = (MethCoroTask *)calloc(1, sizeof(MethCoroTask));
  if (!task) {
    return 0;
  }

  task->step_fn = step_fn;
  task->state = state;
  task->done = 0;
  task->queued = 0;

  AcquireSRWLockExclusive(&g_meth_coro_task_runtime.lock);
  task->next_all = g_meth_coro_task_runtime.all_head;
  g_meth_coro_task_runtime.all_head = task;
  ReleaseSRWLockExclusive(&g_meth_coro_task_runtime.lock);
  return (int64_t)(intptr_t)task;
}

int32_t meth_coro_task_destroy(int64_t task_handle) {
  if (!task_handle) {
    return 0;
  }

  MethCoroTask *target = NULL;
  AcquireSRWLockExclusive(&g_meth_coro_task_runtime.lock);

  MethCoroTask **all_cursor = &g_meth_coro_task_runtime.all_head;
  while (*all_cursor) {
    if ((int64_t)(intptr_t)(*all_cursor) == task_handle) {
      target = *all_cursor;
      *all_cursor = target->next_all;
      break;
    }
    all_cursor = &(*all_cursor)->next_all;
  }

  if (!target) {
    ReleaseSRWLockExclusive(&g_meth_coro_task_runtime.lock);
    return 0;
  }

  MethCoroTask **ready_cursor = &g_meth_coro_task_runtime.ready_head;
  MethCoroTask *prev = NULL;
  while (*ready_cursor) {
    if (*ready_cursor == target) {
      MethCoroTask *next = target->next_ready;
      *ready_cursor = next;
      if (g_meth_coro_task_runtime.ready_tail == target) {
        g_meth_coro_task_runtime.ready_tail = prev;
      }
      break;
    }
    prev = *ready_cursor;
    ready_cursor = &(*ready_cursor)->next_ready;
  }

  MethCoroTokenBinding **token_cursor = &g_meth_coro_task_runtime.token_bindings;
  while (*token_cursor) {
    MethCoroTokenBinding *binding = *token_cursor;
    if (binding->task_handle == task_handle) {
      *token_cursor = binding->next;
      free(binding);
      continue;
    }
    token_cursor = &binding->next;
  }
  ReleaseSRWLockExclusive(&g_meth_coro_task_runtime.lock);

  free(target);
  return 1;
}

int32_t meth_coro_task_schedule(int64_t task_handle, uintptr_t wake_token,
                                int32_t wake_kind, int32_t wake_result) {
  if (!task_handle) {
    return 0;
  }

  int32_t scheduled = 0;
  AcquireSRWLockExclusive(&g_meth_coro_task_runtime.lock);
  MethCoroTask *task = meth_coro_find_task_locked(task_handle);
  if (task && !task->done) {
    task->wake_token = wake_token;
    task->wake_kind = wake_kind;
    task->wake_result = wake_result;
    if (!task->queued) {
      task->queued = 1;
      task->next_ready = NULL;
      if (g_meth_coro_task_runtime.ready_tail) {
        g_meth_coro_task_runtime.ready_tail->next_ready = task;
      } else {
        g_meth_coro_task_runtime.ready_head = task;
      }
      g_meth_coro_task_runtime.ready_tail = task;
    }
    scheduled = 1;
  }
  ReleaseSRWLockExclusive(&g_meth_coro_task_runtime.lock);
  return scheduled;
}

int32_t meth_coro_task_bind_token(int64_t task_handle, uintptr_t token) {
  if (!task_handle || token == 0) {
    return 0;
  }

  int32_t bound = 0;
  AcquireSRWLockExclusive(&g_meth_coro_task_runtime.lock);
  MethCoroTask *task = meth_coro_find_task_locked(task_handle);
  if (task && !task->done) {
    MethCoroTokenBinding *binding = g_meth_coro_task_runtime.token_bindings;
    while (binding) {
      if (binding->token == token) {
        binding->task_handle = task_handle;
        bound = 1;
        break;
      }
      binding = binding->next;
    }

    if (!bound) {
      MethCoroTokenBinding *created =
          (MethCoroTokenBinding *)calloc(1, sizeof(MethCoroTokenBinding));
      if (created) {
        created->token = token;
        created->task_handle = task_handle;
        created->next = g_meth_coro_task_runtime.token_bindings;
        g_meth_coro_task_runtime.token_bindings = created;
        bound = 1;
      }
    }
  }
  ReleaseSRWLockExclusive(&g_meth_coro_task_runtime.lock);
  return bound;
}

static MethCoroTask *meth_coro_pop_ready_task(void) {
  MethCoroTask *task = NULL;
  AcquireSRWLockExclusive(&g_meth_coro_task_runtime.lock);
  task = g_meth_coro_task_runtime.ready_head;
  if (task) {
    g_meth_coro_task_runtime.ready_head = task->next_ready;
    if (!g_meth_coro_task_runtime.ready_head) {
      g_meth_coro_task_runtime.ready_tail = NULL;
    }
    task->next_ready = NULL;
    task->queued = 0;
  }
  ReleaseSRWLockExclusive(&g_meth_coro_task_runtime.lock);
  return task;
}

int32_t meth_coro_task_run_one(int32_t timeout_ms) {
  MethCoroTask *task = meth_coro_pop_ready_task();
  if (!task) {
    uintptr_t token = 0;
    int32_t kind = 0;
    int32_t result = 0;
    int32_t has_event =
        meth_coro_iocp_runtime_poll(timeout_ms, &token, &kind, &result);
    if (!has_event) {
      return 0;
    }
    if (token != 0) {
      int64_t task_handle = 0;
      AcquireSRWLockExclusive(&g_meth_coro_task_runtime.lock);
      task_handle = meth_coro_resolve_task_from_token_locked(token);
      ReleaseSRWLockExclusive(&g_meth_coro_task_runtime.lock);
      (void)meth_coro_task_schedule(task_handle, token, kind, result);
    }
    task = meth_coro_pop_ready_task();
    if (!task) {
      return 0;
    }
  }

  int32_t complete =
      task->step_fn(task->state, task->wake_token, task->wake_kind, task->wake_result);

  AcquireSRWLockExclusive(&g_meth_coro_task_runtime.lock);
  MethCoroTask *alive = meth_coro_find_task_locked((int64_t)(intptr_t)task);
  if (!alive) {
    ReleaseSRWLockExclusive(&g_meth_coro_task_runtime.lock);
    return 1;
  }
  if (complete != 0) {
    alive->done = 1;
  }
  ReleaseSRWLockExclusive(&g_meth_coro_task_runtime.lock);
  return 1;
}

int32_t meth_coro_task_is_done(int64_t task_handle) {
  if (!task_handle) {
    return 0;
  }
  int32_t done = 0;
  AcquireSRWLockExclusive(&g_meth_coro_task_runtime.lock);
  MethCoroTask *task = meth_coro_find_task_locked(task_handle);
  if (task) {
    done = task->done != 0;
  }
  ReleaseSRWLockExclusive(&g_meth_coro_task_runtime.lock);
  return done;
}
#else
/*
 * POSIX reactor (Linux/macOS).
 *
 * This is the portable counterpart to the Windows IOCP reactor. It is built
 * on poll(2) plus a self-pipe so that meth_coro_iocp_runtime_post_wake() can
 * be called from any thread and observed by a blocking poll() in the runner
 * thread. The public API and the EVENT_WAKE/EVENT_IO/EVENT_IO_ERROR semantics
 * intentionally match the IOCP backend so the language-level coroutine
 * lowering is platform agnostic.
 *
 * Registered "sockets" are arbitrary readable/writable file descriptors; a
 * delivered event reports EVENT_IO when the fd is ready and EVENT_IO_ERROR
 * (with errno-style result) when poll reports POLLERR/POLLNVAL/POLLHUP.
 */

typedef struct MethPosixWakeNode {
  uintptr_t token;
  int32_t result;
  struct MethPosixWakeNode *next;
} MethPosixWakeNode;

typedef struct MethPosixRegistration {
  int fd;
  uintptr_t token;
  struct MethPosixRegistration *next;
} MethPosixRegistration;

typedef struct MethPosixReactor {
  pthread_mutex_t lock;
  int initialized;
  int wake_pipe[2]; /* [0] read end, [1] write end */
  MethPosixWakeNode *wake_head;
  MethPosixWakeNode *wake_tail;
  MethPosixRegistration *registrations;
} MethPosixReactor;

static MethPosixReactor g_meth_posix_reactor = {
    PTHREAD_MUTEX_INITIALIZER, 0, {-1, -1}, NULL, NULL, NULL};

static int meth_posix_set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    return 0;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

int32_t meth_coro_iocp_runtime_init(void) {
  pthread_mutex_lock(&g_meth_posix_reactor.lock);
  if (g_meth_posix_reactor.initialized) {
    pthread_mutex_unlock(&g_meth_posix_reactor.lock);
    return 1;
  }

  if (pipe(g_meth_posix_reactor.wake_pipe) != 0) {
    g_meth_posix_reactor.wake_pipe[0] = -1;
    g_meth_posix_reactor.wake_pipe[1] = -1;
    pthread_mutex_unlock(&g_meth_posix_reactor.lock);
    return 0;
  }

  /*
   * The read end must be non-blocking so the runner can drain it without
   * stalling; the write end stays blocking so a wake is never silently lost
   * under pressure (a brief write stall is acceptable and bounded).
   */
  if (!meth_posix_set_nonblocking(g_meth_posix_reactor.wake_pipe[0])) {
    (void)close(g_meth_posix_reactor.wake_pipe[0]);
    (void)close(g_meth_posix_reactor.wake_pipe[1]);
    g_meth_posix_reactor.wake_pipe[0] = -1;
    g_meth_posix_reactor.wake_pipe[1] = -1;
    pthread_mutex_unlock(&g_meth_posix_reactor.lock);
    return 0;
  }

  g_meth_posix_reactor.wake_head = NULL;
  g_meth_posix_reactor.wake_tail = NULL;
  g_meth_posix_reactor.registrations = NULL;
  g_meth_posix_reactor.initialized = 1;
  pthread_mutex_unlock(&g_meth_posix_reactor.lock);
  return 1;
}

int32_t meth_coro_iocp_runtime_shutdown(void) {
  pthread_mutex_lock(&g_meth_posix_reactor.lock);
  if (!g_meth_posix_reactor.initialized) {
    pthread_mutex_unlock(&g_meth_posix_reactor.lock);
    return 1;
  }

  if (g_meth_posix_reactor.wake_pipe[0] >= 0) {
    (void)close(g_meth_posix_reactor.wake_pipe[0]);
  }
  if (g_meth_posix_reactor.wake_pipe[1] >= 0) {
    (void)close(g_meth_posix_reactor.wake_pipe[1]);
  }
  g_meth_posix_reactor.wake_pipe[0] = -1;
  g_meth_posix_reactor.wake_pipe[1] = -1;

  MethPosixWakeNode *wake = g_meth_posix_reactor.wake_head;
  while (wake) {
    MethPosixWakeNode *next = wake->next;
    free(wake);
    wake = next;
  }
  g_meth_posix_reactor.wake_head = NULL;
  g_meth_posix_reactor.wake_tail = NULL;

  MethPosixRegistration *reg = g_meth_posix_reactor.registrations;
  while (reg) {
    MethPosixRegistration *next = reg->next;
    free(reg);
    reg = next;
  }
  g_meth_posix_reactor.registrations = NULL;

  g_meth_posix_reactor.initialized = 0;
  pthread_mutex_unlock(&g_meth_posix_reactor.lock);
  return 1;
}

int32_t meth_coro_iocp_runtime_register_socket(int64_t socket_handle,
                                               uintptr_t token) {
  int fd = (int)socket_handle;
  if (fd < 0) {
    return 0;
  }

  pthread_mutex_lock(&g_meth_posix_reactor.lock);
  if (!g_meth_posix_reactor.initialized) {
    pthread_mutex_unlock(&g_meth_posix_reactor.lock);
    return 0;
  }

  /* Re-registering a known fd just refreshes its token, matching the IOCP
   * behaviour where re-association with the same key is idempotent. */
  MethPosixRegistration *reg = g_meth_posix_reactor.registrations;
  while (reg) {
    if (reg->fd == fd) {
      reg->token = token;
      pthread_mutex_unlock(&g_meth_posix_reactor.lock);
      return 1;
    }
    reg = reg->next;
  }

  MethPosixRegistration *created =
      (MethPosixRegistration *)calloc(1, sizeof(MethPosixRegistration));
  if (!created) {
    pthread_mutex_unlock(&g_meth_posix_reactor.lock);
    return 0;
  }
  created->fd = fd;
  created->token = token;
  created->next = g_meth_posix_reactor.registrations;
  g_meth_posix_reactor.registrations = created;
  pthread_mutex_unlock(&g_meth_posix_reactor.lock);
  return 1;
}

int32_t meth_coro_iocp_runtime_post_wake(uintptr_t token, int32_t result) {
  MethPosixWakeNode *node =
      (MethPosixWakeNode *)calloc(1, sizeof(MethPosixWakeNode));
  if (!node) {
    return 0;
  }
  node->token = token;
  node->result = result;
  node->next = NULL;

  pthread_mutex_lock(&g_meth_posix_reactor.lock);
  if (!g_meth_posix_reactor.initialized ||
      g_meth_posix_reactor.wake_pipe[1] < 0) {
    pthread_mutex_unlock(&g_meth_posix_reactor.lock);
    free(node);
    return 0;
  }

  if (g_meth_posix_reactor.wake_tail) {
    g_meth_posix_reactor.wake_tail->next = node;
  } else {
    g_meth_posix_reactor.wake_head = node;
  }
  g_meth_posix_reactor.wake_tail = node;
  int write_fd = g_meth_posix_reactor.wake_pipe[1];
  pthread_mutex_unlock(&g_meth_posix_reactor.lock);

  /* One byte per queued wake keeps the pipe readable until every queued
   * token has been drained by poll(). EINTR is retried; a full pipe is
   * harmless because a single readable byte already guarantees a wakeup. */
  unsigned char signal_byte = 1;
  ssize_t written;
  do {
    written = write(write_fd, &signal_byte, 1);
  } while (written < 0 && errno == EINTR);
  return 1;
}

int32_t meth_coro_iocp_runtime_poll(int32_t timeout_ms, uintptr_t *out_token,
                                    int32_t *out_kind, int32_t *out_result) {
  pthread_mutex_lock(&g_meth_posix_reactor.lock);
  if (!g_meth_posix_reactor.initialized) {
    pthread_mutex_unlock(&g_meth_posix_reactor.lock);
    return 0;
  }

  /* A queued wake takes priority and is delivered without entering poll(). */
  if (g_meth_posix_reactor.wake_head) {
    MethPosixWakeNode *node = g_meth_posix_reactor.wake_head;
    g_meth_posix_reactor.wake_head = node->next;
    if (!g_meth_posix_reactor.wake_head) {
      g_meth_posix_reactor.wake_tail = NULL;
    }
    int read_fd = g_meth_posix_reactor.wake_pipe[0];
    pthread_mutex_unlock(&g_meth_posix_reactor.lock);

    unsigned char drain;
    ssize_t got;
    do {
      got = read(read_fd, &drain, 1);
    } while (got < 0 && errno == EINTR);

    if (out_token) {
      *out_token = node->token;
    }
    if (out_kind) {
      *out_kind = METH_CORO_IOCP_EVENT_WAKE;
    }
    if (out_result) {
      *out_result = node->result;
    }
    free(node);
    return 1;
  }

  /* Build the poll set: wake-pipe read end first, then registered fds. */
  size_t registration_count = 0;
  MethPosixRegistration *reg = g_meth_posix_reactor.registrations;
  while (reg) {
    registration_count++;
    reg = reg->next;
  }

  size_t nfds = registration_count + 1;
  struct pollfd *fds = (struct pollfd *)calloc(nfds, sizeof(struct pollfd));
  uintptr_t *tokens = (uintptr_t *)calloc(nfds, sizeof(uintptr_t));
  if (!fds || !tokens) {
    free(fds);
    free(tokens);
    pthread_mutex_unlock(&g_meth_posix_reactor.lock);
    return 0;
  }

  fds[0].fd = g_meth_posix_reactor.wake_pipe[0];
  fds[0].events = POLLIN;
  tokens[0] = 0;

  size_t index = 1;
  reg = g_meth_posix_reactor.registrations;
  while (reg && index < nfds) {
    fds[index].fd = reg->fd;
    fds[index].events = POLLIN | POLLOUT;
    tokens[index] = reg->token;
    index++;
    reg = reg->next;
  }
  pthread_mutex_unlock(&g_meth_posix_reactor.lock);

  int poll_timeout = (timeout_ms < 0) ? -1 : timeout_ms;
  int ready;
  do {
    ready = poll(fds, (nfds_t)nfds, poll_timeout);
  } while (ready < 0 && errno == EINTR);

  if (ready <= 0) {
    free(fds);
    free(tokens);
    return 0; /* timeout or poll error: treat as idle */
  }

  /* The wake pipe is reported as a WAKE with the next queued token. */
  if (fds[0].revents & POLLIN) {
    pthread_mutex_lock(&g_meth_posix_reactor.lock);
    MethPosixWakeNode *node = g_meth_posix_reactor.wake_head;
    if (node) {
      g_meth_posix_reactor.wake_head = node->next;
      if (!g_meth_posix_reactor.wake_head) {
        g_meth_posix_reactor.wake_tail = NULL;
      }
    }
    int read_fd = g_meth_posix_reactor.wake_pipe[0];
    pthread_mutex_unlock(&g_meth_posix_reactor.lock);

    unsigned char drain;
    ssize_t got;
    do {
      got = read(read_fd, &drain, 1);
    } while (got < 0 && errno == EINTR);

    if (node) {
      if (out_token) {
        *out_token = node->token;
      }
      if (out_kind) {
        *out_kind = METH_CORO_IOCP_EVENT_WAKE;
      }
      if (out_result) {
        *out_result = node->result;
      }
      free(node);
      free(fds);
      free(tokens);
      return 1;
    }
    /* Spurious readability with no queued node: fall through to fd scan. */
  }

  for (size_t i = 1; i < nfds; i++) {
    short revents = fds[i].revents;
    if (revents == 0) {
      continue;
    }
    if (revents & (POLLERR | POLLNVAL | POLLHUP)) {
      if (out_token) {
        *out_token = tokens[i];
      }
      if (out_kind) {
        *out_kind = METH_CORO_IOCP_EVENT_IO_ERROR;
      }
      if (out_result) {
        *out_result = (revents & POLLNVAL) ? EBADF : ECONNRESET;
      }
      free(fds);
      free(tokens);
      return 1;
    }
    if (revents & (POLLIN | POLLOUT)) {
      if (out_token) {
        *out_token = tokens[i];
      }
      if (out_kind) {
        *out_kind = METH_CORO_IOCP_EVENT_IO;
      }
      if (out_result) {
        *out_result = 0;
      }
      free(fds);
      free(tokens);
      return 1;
    }
  }

  free(fds);
  free(tokens);
  return 0;
}

/*
 * POSIX coroutine task scheduler.
 *
 * A direct pthread-mutex port of the Windows scheduler above: identical
 * ready-queue, token-binding, and lifecycle semantics so the language-level
 * lowering behaves the same on both platforms.
 */

typedef struct MethCoroTask {
  MethCoroStepFn step_fn;
  void *state;
  int32_t done;
  int32_t queued;
  uintptr_t wake_token;
  int32_t wake_kind;
  int32_t wake_result;
  struct MethCoroTask *next_ready;
  struct MethCoroTask *next_all;
} MethCoroTask;

typedef struct MethCoroTokenBinding {
  uintptr_t token;
  int64_t task_handle;
  struct MethCoroTokenBinding *next;
} MethCoroTokenBinding;

typedef struct MethCoroTaskRuntime {
  pthread_mutex_t lock;
  MethCoroTask *all_head;
  MethCoroTask *ready_head;
  MethCoroTask *ready_tail;
  MethCoroTokenBinding *token_bindings;
} MethCoroTaskRuntime;

static MethCoroTaskRuntime g_meth_coro_task_runtime = {
    PTHREAD_MUTEX_INITIALIZER, NULL, NULL, NULL, NULL};

static MethCoroTask *meth_coro_find_task_locked(int64_t task_handle) {
  MethCoroTask *task = g_meth_coro_task_runtime.all_head;
  while (task) {
    if ((int64_t)(intptr_t)task == task_handle) {
      return task;
    }
    task = task->next_all;
  }
  return NULL;
}

static int64_t meth_coro_resolve_task_from_token_locked(uintptr_t token) {
  MethCoroTokenBinding *binding = g_meth_coro_task_runtime.token_bindings;
  while (binding) {
    if (binding->token == token) {
      return binding->task_handle;
    }
    binding = binding->next;
  }
  return (int64_t)(intptr_t)token;
}

int64_t meth_coro_task_create(MethCoroStepFn step_fn, void *state) {
  if (!step_fn) {
    return 0;
  }

  MethCoroTask *task = (MethCoroTask *)calloc(1, sizeof(MethCoroTask));
  if (!task) {
    return 0;
  }

  task->step_fn = step_fn;
  task->state = state;
  task->done = 0;
  task->queued = 0;

  pthread_mutex_lock(&g_meth_coro_task_runtime.lock);
  task->next_all = g_meth_coro_task_runtime.all_head;
  g_meth_coro_task_runtime.all_head = task;
  pthread_mutex_unlock(&g_meth_coro_task_runtime.lock);
  return (int64_t)(intptr_t)task;
}

int32_t meth_coro_task_destroy(int64_t task_handle) {
  if (!task_handle) {
    return 0;
  }

  MethCoroTask *target = NULL;
  pthread_mutex_lock(&g_meth_coro_task_runtime.lock);

  MethCoroTask **all_cursor = &g_meth_coro_task_runtime.all_head;
  while (*all_cursor) {
    if ((int64_t)(intptr_t)(*all_cursor) == task_handle) {
      target = *all_cursor;
      *all_cursor = target->next_all;
      break;
    }
    all_cursor = &(*all_cursor)->next_all;
  }

  if (!target) {
    pthread_mutex_unlock(&g_meth_coro_task_runtime.lock);
    return 0;
  }

  MethCoroTask **ready_cursor = &g_meth_coro_task_runtime.ready_head;
  MethCoroTask *prev = NULL;
  while (*ready_cursor) {
    if (*ready_cursor == target) {
      MethCoroTask *next = target->next_ready;
      *ready_cursor = next;
      if (g_meth_coro_task_runtime.ready_tail == target) {
        g_meth_coro_task_runtime.ready_tail = prev;
      }
      break;
    }
    prev = *ready_cursor;
    ready_cursor = &(*ready_cursor)->next_ready;
  }

  MethCoroTokenBinding **token_cursor =
      &g_meth_coro_task_runtime.token_bindings;
  while (*token_cursor) {
    MethCoroTokenBinding *binding = *token_cursor;
    if (binding->task_handle == task_handle) {
      *token_cursor = binding->next;
      free(binding);
      continue;
    }
    token_cursor = &binding->next;
  }
  pthread_mutex_unlock(&g_meth_coro_task_runtime.lock);

  free(target);
  return 1;
}

int32_t meth_coro_task_schedule(int64_t task_handle, uintptr_t wake_token,
                                int32_t wake_kind, int32_t wake_result) {
  if (!task_handle) {
    return 0;
  }

  int32_t scheduled = 0;
  pthread_mutex_lock(&g_meth_coro_task_runtime.lock);
  MethCoroTask *task = meth_coro_find_task_locked(task_handle);
  if (task && !task->done) {
    task->wake_token = wake_token;
    task->wake_kind = wake_kind;
    task->wake_result = wake_result;
    if (!task->queued) {
      task->queued = 1;
      task->next_ready = NULL;
      if (g_meth_coro_task_runtime.ready_tail) {
        g_meth_coro_task_runtime.ready_tail->next_ready = task;
      } else {
        g_meth_coro_task_runtime.ready_head = task;
      }
      g_meth_coro_task_runtime.ready_tail = task;
    }
    scheduled = 1;
  }
  pthread_mutex_unlock(&g_meth_coro_task_runtime.lock);
  return scheduled;
}

int32_t meth_coro_task_bind_token(int64_t task_handle, uintptr_t token) {
  if (!task_handle || token == 0) {
    return 0;
  }

  int32_t bound = 0;
  pthread_mutex_lock(&g_meth_coro_task_runtime.lock);
  MethCoroTask *task = meth_coro_find_task_locked(task_handle);
  if (task && !task->done) {
    MethCoroTokenBinding *binding = g_meth_coro_task_runtime.token_bindings;
    while (binding) {
      if (binding->token == token) {
        binding->task_handle = task_handle;
        bound = 1;
        break;
      }
      binding = binding->next;
    }

    if (!bound) {
      MethCoroTokenBinding *created =
          (MethCoroTokenBinding *)calloc(1, sizeof(MethCoroTokenBinding));
      if (created) {
        created->token = token;
        created->task_handle = task_handle;
        created->next = g_meth_coro_task_runtime.token_bindings;
        g_meth_coro_task_runtime.token_bindings = created;
        bound = 1;
      }
    }
  }
  pthread_mutex_unlock(&g_meth_coro_task_runtime.lock);
  return bound;
}

static MethCoroTask *meth_coro_pop_ready_task(void) {
  MethCoroTask *task = NULL;
  pthread_mutex_lock(&g_meth_coro_task_runtime.lock);
  task = g_meth_coro_task_runtime.ready_head;
  if (task) {
    g_meth_coro_task_runtime.ready_head = task->next_ready;
    if (!g_meth_coro_task_runtime.ready_head) {
      g_meth_coro_task_runtime.ready_tail = NULL;
    }
    task->next_ready = NULL;
    task->queued = 0;
  }
  pthread_mutex_unlock(&g_meth_coro_task_runtime.lock);
  return task;
}

int32_t meth_coro_task_run_one(int32_t timeout_ms) {
  MethCoroTask *task = meth_coro_pop_ready_task();
  if (!task) {
    uintptr_t token = 0;
    int32_t kind = 0;
    int32_t result = 0;
    int32_t has_event =
        meth_coro_iocp_runtime_poll(timeout_ms, &token, &kind, &result);
    if (!has_event) {
      return 0;
    }
    if (token != 0) {
      int64_t task_handle = 0;
      pthread_mutex_lock(&g_meth_coro_task_runtime.lock);
      task_handle = meth_coro_resolve_task_from_token_locked(token);
      pthread_mutex_unlock(&g_meth_coro_task_runtime.lock);
      (void)meth_coro_task_schedule(task_handle, token, kind, result);
    }
    task = meth_coro_pop_ready_task();
    if (!task) {
      return 0;
    }
  }

  int32_t complete = task->step_fn(task->state, task->wake_token,
                                   task->wake_kind, task->wake_result);

  pthread_mutex_lock(&g_meth_coro_task_runtime.lock);
  MethCoroTask *alive = meth_coro_find_task_locked((int64_t)(intptr_t)task);
  if (!alive) {
    pthread_mutex_unlock(&g_meth_coro_task_runtime.lock);
    return 1;
  }
  if (complete != 0) {
    alive->done = 1;
  }
  pthread_mutex_unlock(&g_meth_coro_task_runtime.lock);
  return 1;
}

int32_t meth_coro_task_is_done(int64_t task_handle) {
  if (!task_handle) {
    return 0;
  }
  int32_t done = 0;
  pthread_mutex_lock(&g_meth_coro_task_runtime.lock);
  MethCoroTask *task = meth_coro_find_task_locked(task_handle);
  if (task) {
    done = task->done != 0;
  }
  pthread_mutex_unlock(&g_meth_coro_task_runtime.lock);
  return done;
}
#endif

typedef struct MethAsyncHeader {
  void *thread_handle;
  int32_t state;
  int32_t cancel_requested;
  int64_t result_offset;
  int64_t result_size;
  MethAsyncEntryFn entry_fn;
} MethAsyncHeader;

#define METH_ASYNC_DEFAULT_WORKERS_FALLBACK 4
#define METH_ASYNC_MAX_WORKERS 256
#define METH_ASYNC_MAX_QUEUE_CAPACITY 65536

#ifdef _WIN32
static INIT_ONCE g_meth_async_tls_once = INIT_ONCE_STATIC_INIT;
static DWORD g_meth_async_tls_header_index = TLS_OUT_OF_INDEXES;
static DWORD g_meth_async_tls_worker_index = TLS_OUT_OF_INDEXES;

static BOOL CALLBACK meth_async_init_tls(PINIT_ONCE once, PVOID parameter,
                                         PVOID *context) {
  (void)once;
  (void)parameter;
  (void)context;
  g_meth_async_tls_header_index = TlsAlloc();
  if (g_meth_async_tls_header_index == TLS_OUT_OF_INDEXES) {
    return FALSE;
  }

  g_meth_async_tls_worker_index = TlsAlloc();
  if (g_meth_async_tls_worker_index == TLS_OUT_OF_INDEXES) {
    TlsFree(g_meth_async_tls_header_index);
    g_meth_async_tls_header_index = TLS_OUT_OF_INDEXES;
    return FALSE;
  }

  return TRUE;
}

static int meth_async_tls_ensure_initialized(void) {
  return InitOnceExecuteOnce(&g_meth_async_tls_once, meth_async_init_tls, NULL,
                             NULL) != 0 &&
         g_meth_async_tls_header_index != TLS_OUT_OF_INDEXES &&
         g_meth_async_tls_worker_index != TLS_OUT_OF_INDEXES;
}

static MethAsyncHeader *meth_async_current_get(void) {
  if (!meth_async_tls_ensure_initialized()) {
    return NULL;
  }
  return (MethAsyncHeader *)TlsGetValue(g_meth_async_tls_header_index);
}

static void meth_async_current_set(MethAsyncHeader *header) {
  if (!meth_async_tls_ensure_initialized()) {
    return;
  }
  (void)TlsSetValue(g_meth_async_tls_header_index, header);
}

static int meth_async_pool_worker_get(void) {
  if (!meth_async_tls_ensure_initialized()) {
    return 0;
  }
  return (int)(intptr_t)TlsGetValue(g_meth_async_tls_worker_index);
}

static void meth_async_pool_worker_set(int value) {
  if (!meth_async_tls_ensure_initialized()) {
    return;
  }
  (void)TlsSetValue(g_meth_async_tls_worker_index,
                    (LPVOID)(intptr_t)(value ? 1 : 0));
}

static int32_t meth_async_atomic_load_i32(volatile int32_t *target) {
  return (int32_t)InterlockedCompareExchange((volatile LONG *)target, 0, 0);
}

static void meth_async_atomic_store_i32(volatile int32_t *target,
                                        int32_t value) {
  (void)InterlockedExchange((volatile LONG *)target, (LONG)value);
}
#else
static _Thread_local MethAsyncHeader *g_meth_async_current = NULL;
static _Thread_local int g_meth_async_pool_worker = 0;

static MethAsyncHeader *meth_async_current_get(void) {
  return g_meth_async_current;
}

static void meth_async_current_set(MethAsyncHeader *header) {
  g_meth_async_current = header;
}

static int meth_async_pool_worker_get(void) {
  return g_meth_async_pool_worker;
}

static void meth_async_pool_worker_set(int value) {
  g_meth_async_pool_worker = value ? 1 : 0;
}

static int32_t meth_async_atomic_load_i32(volatile int32_t *target) {
  return atomic_load((_Atomic int32_t *)target);
}

static void meth_async_atomic_store_i32(volatile int32_t *target,
                                        int32_t value) {
  atomic_store((_Atomic int32_t *)target, value);
}
#endif

static void meth_async_zero_result(MethAsyncHeader *header) {
  if (!header || header->result_offset < 0 || header->result_size <= 0) {
    return;
  }

  memset(((unsigned char *)header) + header->result_offset, 0,
         (size_t)header->result_size);
}

typedef struct MethAsyncExecutor {
  MethAsyncHeader **queue;
#ifdef _WIN32
  HANDLE *worker_handles;
#else
  pthread_t *worker_handles;
#endif
  int32_t queue_capacity;
  int32_t queue_head;
  int32_t queue_size;
  int32_t worker_count;
  int32_t configured_worker_count;
  int32_t configured_queue_capacity;
  int32_t initialized;
  int32_t init_failed;
  int32_t total_worker_threads_started;
  int32_t live_worker_threads;
  int32_t outstanding_tasks;
  /*
   * Lifecycle state machine (mutated only while holding executor lock):
   * RUNNING -> DRAINING -> STOPPING -> STOPPED
   * RUNNING -> STOPPING -> STOPPED
   */
  int32_t state;
} MethAsyncExecutor;

static MethAsyncExecutor g_meth_async_executor = {
    .state = METH_ASYNC_RUNTIME_STATE_RUNNING,
};

#ifdef _WIN32
static SRWLOCK g_meth_async_executor_lock = SRWLOCK_INIT;
static CONDITION_VARIABLE g_meth_async_executor_cond = CONDITION_VARIABLE_INIT;
#else
static pthread_mutex_t g_meth_async_executor_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_meth_async_executor_cond = PTHREAD_COND_INITIALIZER;
#endif

static void meth_async_execute_task(MethAsyncHeader *header);

static void meth_async_executor_lock(void) {
#ifdef _WIN32
  AcquireSRWLockExclusive(&g_meth_async_executor_lock);
#else
  (void)pthread_mutex_lock(&g_meth_async_executor_lock);
#endif
}

static void meth_async_executor_unlock(void) {
#ifdef _WIN32
  ReleaseSRWLockExclusive(&g_meth_async_executor_lock);
#else
  (void)pthread_mutex_unlock(&g_meth_async_executor_lock);
#endif
}

static void meth_async_executor_wait_locked(void) {
#ifdef _WIN32
  (void)SleepConditionVariableSRW(&g_meth_async_executor_cond,
                                  &g_meth_async_executor_lock, INFINITE, 0);
#else
  (void)pthread_cond_wait(&g_meth_async_executor_cond, &g_meth_async_executor_lock);
#endif
}

static int meth_async_executor_wait_timeout_locked(int32_t timeout_ms) {
#ifdef _WIN32
  if (timeout_ms < 0) {
    return SleepConditionVariableSRW(&g_meth_async_executor_cond,
                                     &g_meth_async_executor_lock, INFINITE,
                                     0) != 0;
  }
  if (timeout_ms == 0) {
    return 0;
  }
  return SleepConditionVariableSRW(&g_meth_async_executor_cond,
                                   &g_meth_async_executor_lock,
                                   (DWORD)timeout_ms, 0) != 0;
#else
  if (timeout_ms < 0) {
    (void)pthread_cond_wait(&g_meth_async_executor_cond, &g_meth_async_executor_lock);
    return 1;
  }
  if (timeout_ms == 0) {
    return 0;
  }

  struct timespec ts;
  (void)clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += (time_t)(timeout_ms / 1000);
  ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
  if (ts.tv_nsec >= 1000000000L) {
    ts.tv_sec += 1;
    ts.tv_nsec -= 1000000000L;
  }
  int rc = pthread_cond_timedwait(&g_meth_async_executor_cond,
                                  &g_meth_async_executor_lock, &ts);
  return rc == 0;
#endif
}

static void meth_async_executor_wake_all_locked(void) {
#ifdef _WIN32
  WakeAllConditionVariable(&g_meth_async_executor_cond);
#else
  (void)pthread_cond_broadcast(&g_meth_async_executor_cond);
#endif
}

static uint64_t meth_async_now_ms(void) {
#ifdef _WIN32
  return (uint64_t)GetTickCount64();
#else
  struct timespec ts;
  (void)clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((uint64_t)ts.tv_sec * 1000ULL) + ((uint64_t)ts.tv_nsec / 1000000ULL);
#endif
}

static int32_t meth_async_remaining_ms(uint64_t deadline_ms) {
  if (deadline_ms == UINT64_MAX) {
    return -1;
  }

  uint64_t now = meth_async_now_ms();
  if (now >= deadline_ms) {
    return 0;
  }

  uint64_t remain = deadline_ms - now;
  if (remain > (uint64_t)INT32_MAX) {
    return INT32_MAX;
  }
  return (int32_t)remain;
}

static int32_t meth_async_default_worker_count(void) {
#ifdef _WIN32
  DWORD count = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
  if (count == 0) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    count = info.dwNumberOfProcessors;
  }
  int32_t workers =
      (count > 0 && count <= (DWORD)INT32_MAX)
          ? (int32_t)count
          : METH_ASYNC_DEFAULT_WORKERS_FALLBACK;
#else
  long count = sysconf(_SC_NPROCESSORS_ONLN);
  int32_t workers =
      (count > 0 && count <= (long)INT32_MAX)
          ? (int32_t)count
          : METH_ASYNC_DEFAULT_WORKERS_FALLBACK;
#endif

  if (workers < 1) {
    workers = 1;
  }
  if (workers > METH_ASYNC_MAX_WORKERS) {
    workers = METH_ASYNC_MAX_WORKERS;
  }
  return workers;
}

static int meth_async_parse_positive_env_i32(const char *name, int32_t *out_value) {
  if (!name || !out_value) {
    return 0;
  }

  const char *value = getenv(name);
  if (!value || value[0] == '\0') {
    return 0;
  }

  char *end = NULL;
  long long parsed = strtoll(value, &end, 10);
  if (end == value || *end != '\0' || parsed <= 0 || parsed > INT32_MAX) {
    return 0;
  }

  *out_value = (int32_t)parsed;
  return 1;
}

static int32_t meth_async_default_queue_capacity(int32_t worker_count) {
  int64_t capacity = (int64_t)worker_count * 8;
  if (capacity < 32) {
    capacity = 32;
  }
  if (capacity > METH_ASYNC_MAX_QUEUE_CAPACITY) {
    capacity = METH_ASYNC_MAX_QUEUE_CAPACITY;
  }
  return (int32_t)capacity;
}

static int32_t meth_async_resolve_worker_count_locked(void) {
  int32_t worker_count = g_meth_async_executor.configured_worker_count;
  if (worker_count <= 0) {
    (void)meth_async_parse_positive_env_i32("METH_ASYNC_WORKERS", &worker_count);
  }
  if (worker_count <= 0) {
    worker_count = meth_async_default_worker_count();
  }
  if (worker_count > METH_ASYNC_MAX_WORKERS) {
    worker_count = METH_ASYNC_MAX_WORKERS;
  }
  return worker_count;
}

static int32_t meth_async_resolve_queue_capacity_locked(int32_t worker_count) {
  int32_t queue_capacity = g_meth_async_executor.configured_queue_capacity;
  if (queue_capacity <= 0) {
    (void)meth_async_parse_positive_env_i32("METH_ASYNC_QUEUE_CAPACITY",
                                            &queue_capacity);
  }
  if (queue_capacity <= 0) {
    queue_capacity = meth_async_default_queue_capacity(worker_count);
  }
  if (queue_capacity > METH_ASYNC_MAX_QUEUE_CAPACITY) {
    queue_capacity = METH_ASYNC_MAX_QUEUE_CAPACITY;
  }
  if (queue_capacity < 1) {
    queue_capacity = 1;
  }
  return queue_capacity;
}

static void meth_async_queue_push_locked(MethAsyncHeader *header) {
  if (!header || !g_meth_async_executor.queue ||
      g_meth_async_executor.queue_capacity <= 0) {
    return;
  }

  int32_t tail = (g_meth_async_executor.queue_head + g_meth_async_executor.queue_size) %
                 g_meth_async_executor.queue_capacity;
  g_meth_async_executor.queue[tail] = header;
  g_meth_async_executor.queue_size++;
}

static MethAsyncHeader *meth_async_queue_pop_locked(void) {
  if (!g_meth_async_executor.queue || g_meth_async_executor.queue_size <= 0) {
    return NULL;
  }

  MethAsyncHeader *header =
      g_meth_async_executor.queue[g_meth_async_executor.queue_head];
  g_meth_async_executor.queue[g_meth_async_executor.queue_head] = NULL;
  g_meth_async_executor.queue_head =
      (g_meth_async_executor.queue_head + 1) %
      g_meth_async_executor.queue_capacity;
  g_meth_async_executor.queue_size--;
  return header;
}

static int meth_async_queue_remove_locked(MethAsyncHeader *target) {
  if (!target || !g_meth_async_executor.queue || g_meth_async_executor.queue_size <= 0) {
    return 0;
  }

  int32_t found = -1;
  for (int32_t i = 0; i < g_meth_async_executor.queue_size; i++) {
    int32_t index =
        (g_meth_async_executor.queue_head + i) % g_meth_async_executor.queue_capacity;
    if (g_meth_async_executor.queue[index] == target) {
      found = i;
      break;
    }
  }

  if (found < 0) {
    return 0;
  }

  for (int32_t i = found; i < g_meth_async_executor.queue_size - 1; i++) {
    int32_t from_index =
        (g_meth_async_executor.queue_head + i + 1) %
        g_meth_async_executor.queue_capacity;
    int32_t to_index =
        (g_meth_async_executor.queue_head + i) % g_meth_async_executor.queue_capacity;
    g_meth_async_executor.queue[to_index] = g_meth_async_executor.queue[from_index];
  }

  int32_t tail_index =
      (g_meth_async_executor.queue_head + g_meth_async_executor.queue_size - 1) %
      g_meth_async_executor.queue_capacity;
  g_meth_async_executor.queue[tail_index] = NULL;
  g_meth_async_executor.queue_size--;
  return 1;
}

static void meth_async_set_state_locked(MethAsyncHeader *header, int32_t state) {
  if (!header) {
    return;
  }

  int32_t previous = meth_async_atomic_load_i32((volatile int32_t *)&header->state);
  meth_async_atomic_store_i32((volatile int32_t *)&header->state, state);
  if (previous < 2 && state >= 2 && g_meth_async_executor.outstanding_tasks > 0) {
    g_meth_async_executor.outstanding_tasks--;
  }
}

static void meth_async_cancel_locked(MethAsyncHeader *header) {
  if (!header) {
    return;
  }

  meth_async_atomic_store_i32((volatile int32_t *)&header->cancel_requested, 1);
  if (g_meth_async_executor.initialized &&
      meth_async_atomic_load_i32((volatile int32_t *)&header->state) == 0 &&
      meth_async_queue_remove_locked(header)) {
    meth_async_set_state_locked(header, 3);
  }
}

static void meth_async_abort_queued_locked(void) {
  MethAsyncHeader *queued = NULL;
  while ((queued = meth_async_queue_pop_locked()) != NULL) {
    meth_async_atomic_store_i32((volatile int32_t *)&queued->cancel_requested, 1);
    meth_async_set_state_locked(queued, 3);
  }
}

#ifdef _WIN32
static DWORD WINAPI meth_async_worker_main(LPVOID parameter) {
#else
static void *meth_async_worker_main(void *parameter) {
#endif
  (void)parameter;
  (void)gc_thread_attach();
  meth_async_pool_worker_set(1);

  while (1) {
    MethAsyncHeader *header = NULL;

    meth_async_executor_lock();
    while (g_meth_async_executor.queue_size <= 0) {
      if (!g_meth_async_executor.initialized ||
          g_meth_async_executor.state >= METH_ASYNC_RUNTIME_STATE_STOPPING) {
        if (g_meth_async_executor.live_worker_threads > 0) {
          g_meth_async_executor.live_worker_threads--;
        }
        meth_async_executor_wake_all_locked();
        meth_async_executor_unlock();
        meth_async_pool_worker_set(0);
        meth_async_current_set(NULL);
        (void)gc_thread_detach();
#ifdef _WIN32
        return 0u;
#else
        return NULL;
#endif
      }
      meth_async_executor_wait_locked();
    }

    header = meth_async_queue_pop_locked();
    meth_async_executor_wake_all_locked();
    meth_async_executor_unlock();

    if (header) {
      meth_async_execute_task(header);
    }
  }
}

static int meth_async_start_worker_thread(int32_t index) {
#ifdef _WIN32
  HANDLE thread = CreateThread(NULL, 0, meth_async_worker_main, NULL, 0, NULL);
  if (!thread) {
    return 0;
  }
  g_meth_async_executor.worker_handles[index] = thread;
  return 1;
#else
  pthread_t thread;
  if (pthread_create(&thread, NULL, meth_async_worker_main, NULL) != 0) {
    return 0;
  }
  g_meth_async_executor.worker_handles[index] = thread;
  return 1;
#endif
}

static int meth_async_executor_init_locked(void) {
  if (g_meth_async_executor.initialized) {
    return g_meth_async_executor.state == METH_ASYNC_RUNTIME_STATE_RUNNING;
  }
  if (g_meth_async_executor.init_failed ||
      g_meth_async_executor.state != METH_ASYNC_RUNTIME_STATE_RUNNING) {
    return 0;
  }

  int32_t worker_count = meth_async_resolve_worker_count_locked();
  int32_t queue_capacity = meth_async_resolve_queue_capacity_locked(worker_count);
  MethAsyncHeader **queue =
      (MethAsyncHeader **)calloc((size_t)queue_capacity, sizeof(MethAsyncHeader *));
  if (!queue) {
    g_meth_async_executor.init_failed = 1;
    return 0;
  }

#ifdef _WIN32
  HANDLE *handles =
      (HANDLE *)calloc((size_t)worker_count, sizeof(HANDLE));
#else
  pthread_t *handles =
      (pthread_t *)calloc((size_t)worker_count, sizeof(pthread_t));
#endif
  if (!handles) {
    free(queue);
    g_meth_async_executor.init_failed = 1;
    return 0;
  }

  g_meth_async_executor.queue = queue;
  g_meth_async_executor.worker_handles = handles;
  g_meth_async_executor.queue_capacity = queue_capacity;
  g_meth_async_executor.queue_head = 0;
  g_meth_async_executor.queue_size = 0;
  g_meth_async_executor.outstanding_tasks = 0;
  g_meth_async_executor.state = METH_ASYNC_RUNTIME_STATE_RUNNING;
  g_meth_async_executor.initialized = 1;

  int32_t started_workers = 0;
  for (; started_workers < worker_count; started_workers++) {
    if (!meth_async_start_worker_thread(started_workers)) {
      break;
    }
  }

  if (started_workers <= 0) {
    free(g_meth_async_executor.worker_handles);
    g_meth_async_executor.worker_handles = NULL;
    free(g_meth_async_executor.queue);
    g_meth_async_executor.queue = NULL;
    g_meth_async_executor.queue_capacity = 0;
    g_meth_async_executor.initialized = 0;
    g_meth_async_executor.init_failed = 1;
    return 0;
  }

  g_meth_async_executor.worker_count = started_workers;
  g_meth_async_executor.total_worker_threads_started = started_workers;
  g_meth_async_executor.live_worker_threads = started_workers;
  meth_async_executor_wake_all_locked();
  return 1;
}

static int meth_async_executor_ensure_initialized(void) {
  int ok = 0;
  meth_async_executor_lock();
  ok = meth_async_executor_init_locked();
  meth_async_executor_unlock();
  return ok;
}

static void meth_async_execute_task(MethAsyncHeader *header) {
  if (!header) {
    return;
  }

  MethAsyncHeader *previous = meth_async_current_get();
  meth_async_current_set(header);

  int execute_body = 0;

  meth_async_executor_lock();
  if (meth_async_atomic_load_i32((volatile int32_t *)&header->state) < 2) {
    int cancelled =
        meth_async_atomic_load_i32((volatile int32_t *)&header->cancel_requested) != 0;
    int stopping =
        g_meth_async_executor.state >= METH_ASYNC_RUNTIME_STATE_STOPPING;
    if (cancelled || stopping) {
      meth_async_set_state_locked(header, 3);
    } else {
      meth_async_set_state_locked(header, 1);
      execute_body = 1;
    }
  }
  meth_async_executor_unlock();

  if (execute_body && header->entry_fn) {
    header->entry_fn((const char *)header);
  }

  meth_async_executor_lock();
  if (meth_async_atomic_load_i32((volatile int32_t *)&header->state) < 2) {
    int32_t final_state =
        (meth_async_atomic_load_i32((volatile int32_t *)&header->cancel_requested) != 0 ||
         g_meth_async_executor.state >= METH_ASYNC_RUNTIME_STATE_STOPPING)
            ? 3
            : 2;
    meth_async_set_state_locked(header, final_state);
  }
  meth_async_executor_wake_all_locked();
  meth_async_executor_unlock();

  meth_async_current_set(previous);
}

static int meth_async_executor_enqueue(MethAsyncHeader *header) {
  if (!header) {
    return 0;
  }

  while (1) {
    MethAsyncHeader *assist = NULL;

    meth_async_executor_lock();
    if (!g_meth_async_executor.initialized || !g_meth_async_executor.queue ||
        g_meth_async_executor.queue_capacity <= 0 ||
        g_meth_async_executor.state != METH_ASYNC_RUNTIME_STATE_RUNNING) {
      meth_async_executor_unlock();
      return 0;
    }

    while (g_meth_async_executor.queue_size >= g_meth_async_executor.queue_capacity) {
      if (g_meth_async_executor.state != METH_ASYNC_RUNTIME_STATE_RUNNING) {
        meth_async_executor_unlock();
        return 0;
      }

      if (meth_async_pool_worker_get()) {
        assist = meth_async_queue_pop_locked();
        if (assist) {
          meth_async_executor_wake_all_locked();
          break;
        }
      }
      meth_async_executor_wait_locked();
    }

    if (!assist &&
        g_meth_async_executor.queue_size < g_meth_async_executor.queue_capacity &&
        g_meth_async_executor.state == METH_ASYNC_RUNTIME_STATE_RUNNING) {
      meth_async_queue_push_locked(header);
      g_meth_async_executor.outstanding_tasks++;
      meth_async_executor_wake_all_locked();
      meth_async_executor_unlock();
      return 1;
    }

    meth_async_executor_unlock();
    if (assist) {
      meth_async_execute_task(assist);
    }
  }
}

int32_t meth_async_runtime_configure(int32_t worker_count,
                                     int32_t queue_capacity) {
  if (worker_count < 0 || queue_capacity < 0) {
    return 0;
  }

  meth_async_executor_lock();
  if (g_meth_async_executor.initialized || g_meth_async_executor.init_failed ||
      g_meth_async_executor.state != METH_ASYNC_RUNTIME_STATE_RUNNING) {
    meth_async_executor_unlock();
    return 0;
  }

  g_meth_async_executor.configured_worker_count = worker_count;
  g_meth_async_executor.configured_queue_capacity = queue_capacity;
  meth_async_executor_unlock();
  return 1;
}

int32_t meth_async_runtime_worker_count(void) {
  if (!meth_async_executor_ensure_initialized()) {
    return 0;
  }

  meth_async_executor_lock();
  int32_t value = g_meth_async_executor.worker_count;
  meth_async_executor_unlock();
  return value;
}

int32_t meth_async_runtime_queue_capacity(void) {
  if (!meth_async_executor_ensure_initialized()) {
    return 0;
  }

  meth_async_executor_lock();
  int32_t value = g_meth_async_executor.queue_capacity;
  meth_async_executor_unlock();
  return value;
}

int32_t meth_async_runtime_queued_task_count(void) {
  if (!meth_async_executor_ensure_initialized()) {
    return 0;
  }

  meth_async_executor_lock();
  int32_t value = g_meth_async_executor.queue_size;
  meth_async_executor_unlock();
  return value;
}

int32_t meth_async_runtime_total_worker_threads_started(void) {
  if (!meth_async_executor_ensure_initialized()) {
    return 0;
  }

  meth_async_executor_lock();
  int32_t value = g_meth_async_executor.total_worker_threads_started;
  meth_async_executor_unlock();
  return value;
}

int32_t meth_async_runtime_live_worker_threads(void) {
  meth_async_executor_lock();
  int32_t value = g_meth_async_executor.live_worker_threads;
  meth_async_executor_unlock();
  return value;
}

int32_t meth_async_runtime_outstanding_task_count(void) {
  meth_async_executor_lock();
  int32_t value = g_meth_async_executor.outstanding_tasks;
  meth_async_executor_unlock();
  return value;
}

int32_t meth_async_runtime_state(void) {
  meth_async_executor_lock();
  int32_t value = g_meth_async_executor.state;
  meth_async_executor_unlock();
  return value;
}

int32_t meth_async_runtime_shutdown(int32_t kind, int32_t timeout_ms) {
  if (kind != METH_ASYNC_SHUTDOWN_ABORT && kind != METH_ASYNC_SHUTDOWN_DRAIN) {
    return 0;
  }

  uint64_t deadline_ms =
      timeout_ms < 0 ? UINT64_MAX : (meth_async_now_ms() + (uint64_t)timeout_ms);

  meth_async_executor_lock();

  if (g_meth_async_executor.state == METH_ASYNC_RUNTIME_STATE_STOPPED) {
    meth_async_executor_unlock();
    return 1;
  }

  if (!g_meth_async_executor.initialized) {
    g_meth_async_executor.state = METH_ASYNC_RUNTIME_STATE_STOPPED;
    g_meth_async_executor.init_failed = 1;
    meth_async_executor_wake_all_locked();
    meth_async_executor_unlock();
    return 1;
  }

  if (kind == METH_ASYNC_SHUTDOWN_DRAIN &&
      g_meth_async_executor.state == METH_ASYNC_RUNTIME_STATE_RUNNING) {
    g_meth_async_executor.state = METH_ASYNC_RUNTIME_STATE_DRAINING;
  } else {
    g_meth_async_executor.state = METH_ASYNC_RUNTIME_STATE_STOPPING;
    meth_async_abort_queued_locked();
  }
  meth_async_executor_wake_all_locked();

  while (1) {
    if (g_meth_async_executor.state == METH_ASYNC_RUNTIME_STATE_DRAINING) {
      if (g_meth_async_executor.outstanding_tasks <= 0 &&
          g_meth_async_executor.queue_size <= 0) {
        g_meth_async_executor.state = METH_ASYNC_RUNTIME_STATE_STOPPING;
        meth_async_abort_queued_locked();
        meth_async_executor_wake_all_locked();
      }
    }

    if (g_meth_async_executor.state == METH_ASYNC_RUNTIME_STATE_STOPPING &&
        g_meth_async_executor.live_worker_threads <= 0) {
      break;
    }

    int32_t remaining_ms = meth_async_remaining_ms(deadline_ms);
    if (remaining_ms == 0) {
      if (g_meth_async_executor.state == METH_ASYNC_RUNTIME_STATE_DRAINING) {
        g_meth_async_executor.state = METH_ASYNC_RUNTIME_STATE_STOPPING;
        meth_async_abort_queued_locked();
        meth_async_executor_wake_all_locked();
        continue;
      }
      meth_async_executor_unlock();
      return 0;
    }

    (void)meth_async_executor_wait_timeout_locked(remaining_ms);
  }

  int32_t worker_count = g_meth_async_executor.worker_count;
#ifdef _WIN32
  HANDLE *worker_handles = g_meth_async_executor.worker_handles;
#else
  pthread_t *worker_handles = g_meth_async_executor.worker_handles;
#endif

  g_meth_async_executor.worker_handles = NULL;
  meth_async_executor_unlock();

  for (int32_t i = 0; i < worker_count; i++) {
#ifdef _WIN32
    if (worker_handles && worker_handles[i]) {
      (void)WaitForSingleObject(worker_handles[i], INFINITE);
      CloseHandle(worker_handles[i]);
    }
#else
    if (worker_handles && worker_handles[i]) {
      (void)pthread_join(worker_handles[i], NULL);
    }
#endif
  }

  free(worker_handles);

  meth_async_executor_lock();
  free(g_meth_async_executor.queue);
  g_meth_async_executor.queue = NULL;
  g_meth_async_executor.queue_capacity = 0;
  g_meth_async_executor.queue_head = 0;
  g_meth_async_executor.queue_size = 0;
  g_meth_async_executor.worker_count = 0;
  g_meth_async_executor.live_worker_threads = 0;
  g_meth_async_executor.total_worker_threads_started = 0;
  g_meth_async_executor.outstanding_tasks = 0;
  g_meth_async_executor.initialized = 0;
  g_meth_async_executor.init_failed = 1;
  g_meth_async_executor.state = METH_ASYNC_RUNTIME_STATE_STOPPED;
  meth_async_executor_wake_all_locked();
  meth_async_executor_unlock();
  return 1;
}

int32_t meth_async_runtime_reset(void) {
  meth_async_executor_lock();
  if (g_meth_async_executor.state != METH_ASYNC_RUNTIME_STATE_STOPPED ||
      g_meth_async_executor.initialized ||
      g_meth_async_executor.live_worker_threads != 0) {
    meth_async_executor_unlock();
    return 0;
  }

  g_meth_async_executor.init_failed = 0;
  g_meth_async_executor.state = METH_ASYNC_RUNTIME_STATE_RUNNING;
  meth_async_executor_unlock();
  return 1;
}

int32_t meth_async_start(const char *ctx) {
  MethAsyncHeader *header = (MethAsyncHeader *)ctx;
  if (!header || !header->entry_fn) {
    return 0;
  }

  meth_async_zero_result(header);
  meth_async_atomic_store_i32((volatile int32_t *)&header->state, 0);
  meth_async_atomic_store_i32((volatile int32_t *)&header->cancel_requested, 0);
  header->thread_handle = (void *)&g_meth_async_executor;

  if (!meth_async_executor_ensure_initialized()) {
    meth_async_atomic_store_i32((volatile int32_t *)&header->state, 3);
    return 0;
  }

  if (!meth_async_executor_enqueue(header)) {
    meth_async_atomic_store_i32((volatile int32_t *)&header->state, 3);
    return 0;
  }

  return 1;
}

int32_t meth_async_finish(const char *ctx) {
  MethAsyncHeader *header = (MethAsyncHeader *)ctx;
  if (!header) {
    return 0;
  }

  meth_async_executor_lock();
  int32_t final_state =
      (meth_async_atomic_load_i32((volatile int32_t *)&header->cancel_requested) != 0 ||
       g_meth_async_executor.state >= METH_ASYNC_RUNTIME_STATE_STOPPING)
          ? 3
          : 2;
  meth_async_set_state_locked(header, final_state);
  meth_async_executor_wake_all_locked();
  meth_async_executor_unlock();
  return 1;
}

int32_t meth_async_cancel(const char *ctx) {
  MethAsyncHeader *header = (MethAsyncHeader *)ctx;
  if (!header) {
    return 0;
  }

  meth_async_executor_lock();
  meth_async_cancel_locked(header);
  meth_async_executor_wake_all_locked();
  meth_async_executor_unlock();
  return 1;
}

int32_t meth_async_current_cancelled(void) {
  MethAsyncHeader *header = meth_async_current_get();
  if (!header) {
    return 0;
  }

  meth_async_executor_lock();
  int32_t cancelled =
      meth_async_atomic_load_i32((volatile int32_t *)&header->cancel_requested) != 0 ||
      g_meth_async_executor.state >= METH_ASYNC_RUNTIME_STATE_STOPPING;
  meth_async_executor_unlock();
  return cancelled;
}

static int meth_async_header_uses_coro_task(MethAsyncHeader *header) {
  return header && header->thread_handle &&
         header->thread_handle != (void *)&g_meth_async_executor;
}

static void meth_async_coro_release_task_handle(MethAsyncHeader *header) {
  if (!header || !header->thread_handle) {
    return;
  }

  int64_t task_handle = (int64_t)(intptr_t)header->thread_handle;
  header->thread_handle = NULL;
  if (task_handle != 0) {
    (void)meth_coro_task_destroy(task_handle);
  }
}

int32_t meth_async_wait(const char *ctx) {
  MethAsyncHeader *header = (MethAsyncHeader *)ctx;
  if (!header) {
    return 0;
  }

  if (meth_async_header_uses_coro_task(header)) {
    while (1) {
      int32_t state = meth_async_atomic_load_i32((volatile int32_t *)&header->state);
      if (state >= 2) {
        meth_async_coro_release_task_handle(header);
        return state == 2;
      }

      int32_t step = meth_coro_task_run_one(10);
      if (step < 0) {
        return 0;
      }
    }
  }

  if (!meth_async_executor_ensure_initialized()) {
    return meth_async_atomic_load_i32((volatile int32_t *)&header->state) == 2;
  }

  MethAsyncHeader *current = meth_async_current_get();
  if (current && current == header) {
    return meth_async_atomic_load_i32((volatile int32_t *)&header->state) == 2;
  }

  while (1) {
    MethAsyncHeader *assist = NULL;

    meth_async_executor_lock();
    int32_t state = meth_async_atomic_load_i32((volatile int32_t *)&header->state);
    if (state >= 2) {
      meth_async_executor_unlock();
      return state == 2;
    }

    if (g_meth_async_executor.state >= METH_ASYNC_RUNTIME_STATE_STOPPING) {
      meth_async_executor_unlock();
      return 0;
    }

    current = meth_async_current_get();
    if (current && current != header &&
        meth_async_atomic_load_i32((volatile int32_t *)&current->cancel_requested) !=
            0 &&
        meth_async_atomic_load_i32((volatile int32_t *)&header->cancel_requested) ==
            0) {
      meth_async_cancel_locked(header);
      meth_async_executor_wake_all_locked();
    }

    if (g_meth_async_executor.state == METH_ASYNC_RUNTIME_STATE_RUNNING &&
        meth_async_pool_worker_get() && current && current != header &&
        g_meth_async_executor.queue_size > 0) {
      assist = meth_async_queue_pop_locked();
      meth_async_executor_wake_all_locked();
    }

    if (!assist) {
      meth_async_executor_wait_locked();
      meth_async_executor_unlock();
      continue;
    }

    meth_async_executor_unlock();
    meth_async_execute_task(assist);
  }
}

int32_t meth_async_state(const char *ctx) {
  MethAsyncHeader *header = (MethAsyncHeader *)ctx;
  if (!header) {
    return 0;
  }
  return meth_async_atomic_load_i32((volatile int32_t *)&header->state);
}
