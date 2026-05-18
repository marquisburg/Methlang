#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "gc.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>
#endif

// ─────────────────────────────────────────────────────────────────────────────
// Thread<T>
//
// A MethThread wraps an OS thread. The spawned function receives args[] and
// writes its return value into result_storage before setting done=1.
// ─────────────────────────────────────────────────────────────────────────────

typedef struct {
#ifdef _WIN32
  HANDLE handle;
#else
  pthread_t handle;
  int valid;
#endif
  volatile int32_t done;
  int64_t result;        // return value of spawned fn (cast to int64)
  void *fn_ptr;          // pointer to the raw function
  int64_t *args;         // argument array (copied in)
  size_t arg_count;
} MethThread;

typedef int64_t (*MethThreadFn0)(void);
typedef int64_t (*MethThreadFn1)(int64_t);
typedef int64_t (*MethThreadFn2)(int64_t, int64_t);
typedef int64_t (*MethThreadFn3)(int64_t, int64_t, int64_t);
typedef int64_t (*MethThreadFn4)(int64_t, int64_t, int64_t, int64_t);

#ifdef _WIN32
static DWORD WINAPI meth_thread_entry(void *arg) {
  MethThread *t = (MethThread *)arg;
  gc_thread_attach();

  switch (t->arg_count) {
  case 0: t->result = ((MethThreadFn0)t->fn_ptr)(); break;
  case 1: t->result = ((MethThreadFn1)t->fn_ptr)(t->args[0]); break;
  case 2: t->result = ((MethThreadFn2)t->fn_ptr)(t->args[0], t->args[1]); break;
  case 3: t->result = ((MethThreadFn3)t->fn_ptr)(t->args[0], t->args[1], t->args[2]); break;
  default:t->result = ((MethThreadFn4)t->fn_ptr)(t->args[0], t->args[1], t->args[2], t->args[3]); break;
  }

  InterlockedExchange((volatile LONG *)&t->done, 1);
  gc_thread_detach();
  return 0;
}
#else
static void *meth_thread_entry(void *arg) {
  MethThread *t = (MethThread *)arg;
  gc_thread_attach();

  switch (t->arg_count) {
  case 0: t->result = ((MethThreadFn0)t->fn_ptr)(); break;
  case 1: t->result = ((MethThreadFn1)t->fn_ptr)(t->args[0]); break;
  case 2: t->result = ((MethThreadFn2)t->fn_ptr)(t->args[0], t->args[1]); break;
  case 3: t->result = ((MethThreadFn3)t->fn_ptr)(t->args[0], t->args[1], t->args[2]); break;
  default:t->result = ((MethThreadFn4)t->fn_ptr)(t->args[0], t->args[1], t->args[2], t->args[3]); break;
  }

  atomic_store((_Atomic int32_t *)&t->done, 1);
  gc_thread_detach();
  return NULL;
}
#endif

// __meth_thread_spawn(fn_ptr, arg0, arg1, ..., arg_count)
// The codegen passes: fn_ptr (int64 = pointer), then each argument, then
// arg_count as the last argument. Returns the MethThread* as int64.
int64_t __meth_thread_spawn(void *fn_ptr, int64_t *args, size_t arg_count) {
  MethThread *t = (MethThread *)calloc(1, sizeof(MethThread));
  if (!t) return 0;

  t->fn_ptr = fn_ptr;
  t->arg_count = arg_count;
  if (arg_count > 0) {
    t->args = (int64_t *)malloc(arg_count * sizeof(int64_t));
    if (!t->args) { free(t); return 0; }
    memcpy(t->args, args, arg_count * sizeof(int64_t));
  }

#ifdef _WIN32
  t->handle = CreateThread(NULL, 0, meth_thread_entry, t, 0, NULL);
  if (!t->handle) { free(t->args); free(t); return 0; }
#else
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  if (pthread_create(&t->handle, &attr, meth_thread_entry, t) != 0) {
    pthread_attr_destroy(&attr);
    free(t->args); free(t); return 0;
  }
  pthread_attr_destroy(&attr);
  t->valid = 1;
#endif

  return (int64_t)(uintptr_t)t;
}

// __meth_thread_join(thread_handle) -> result int64
int64_t __meth_thread_join(int64_t handle) {
  MethThread *t = (MethThread *)(uintptr_t)handle;
  if (!t) return 0;

#ifdef _WIN32
  if (t->handle) {
    WaitForSingleObject(t->handle, INFINITE);
    CloseHandle(t->handle);
    t->handle = NULL;
  }
#else
  if (t->valid) {
    pthread_join(t->handle, NULL);
    t->valid = 0;
  }
#endif

  int64_t result = t->result;
  free(t->args);
  free(t);
  return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mutex<T>
//
// A MethMutex is a heap-allocated OS mutex. lock() returns a Guard which is
// just the mutex pointer itself (the RAII unlock is codegen-emitted at scope
// end via __meth_mutex_unlock).
// ─────────────────────────────────────────────────────────────────────────────

typedef struct {
#ifdef _WIN32
  SRWLOCK srw;
#else
  pthread_mutex_t mtx;
#endif
} MethMutex;

// __meth_mutex_new() -> MethMutex* as int64
int64_t __meth_mutex_new(void) {
  MethMutex *m = (MethMutex *)malloc(sizeof(MethMutex));
  if (!m) return 0;
#ifdef _WIN32
  InitializeSRWLock(&m->srw);
#else
  pthread_mutex_init(&m->mtx, NULL);
#endif
  return (int64_t)(uintptr_t)m;
}

// __meth_mutex_lock(mutex_handle) -> Guard handle (same pointer, for unlock)
int64_t __meth_mutex_lock(int64_t handle) {
  MethMutex *m = (MethMutex *)(uintptr_t)handle;
  if (!m) return 0;
#ifdef _WIN32
  AcquireSRWLockExclusive(&m->srw);
#else
  pthread_mutex_lock(&m->mtx);
#endif
  return handle; // Guard IS the mutex pointer
}

// __meth_mutex_unlock(guard_handle)
int64_t __meth_mutex_unlock(int64_t handle) {
  MethMutex *m = (MethMutex *)(uintptr_t)handle;
  if (!m) return 0;
#ifdef _WIN32
  ReleaseSRWLockExclusive(&m->srw);
#else
  pthread_mutex_unlock(&m->mtx);
#endif
  return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Atomic<T>
//
// An Atomic<T> is a heap-allocated int64_t with sequentially-consistent ops.
// ─────────────────────────────────────────────────────────────────────────────

// __meth_atomic_new(initial_value) -> int64_t* as int64
int64_t __meth_atomic_new(int64_t initial) {
  int64_t *p = (int64_t *)malloc(sizeof(int64_t));
  if (!p) return 0;
  *p = initial;
  return (int64_t)(uintptr_t)p;
}

int64_t __meth_atomic_load(int64_t handle) {
#ifdef _WIN32
  volatile int64_t *p = (volatile int64_t *)(uintptr_t)handle;
  return InterlockedAdd64((volatile LONG64 *)p, 0);
#else
  _Atomic int64_t *p = (_Atomic int64_t *)(uintptr_t)handle;
  return atomic_load_explicit(p, memory_order_seq_cst);
#endif
}

int64_t __meth_atomic_store(int64_t handle, int64_t value) {
#ifdef _WIN32
  volatile LONG64 *p = (volatile LONG64 *)(uintptr_t)handle;
  InterlockedExchange64(p, value);
#else
  _Atomic int64_t *p = (_Atomic int64_t *)(uintptr_t)handle;
  atomic_store_explicit(p, value, memory_order_seq_cst);
#endif
  return 0;
}

int64_t __meth_atomic_fetch_add(int64_t handle, int64_t delta) {
#ifdef _WIN32
  volatile LONG64 *p = (volatile LONG64 *)(uintptr_t)handle;
  return InterlockedExchangeAdd64(p, delta);
#else
  _Atomic int64_t *p = (_Atomic int64_t *)(uintptr_t)handle;
  return atomic_fetch_add_explicit(p, delta, memory_order_seq_cst);
#endif
}

int64_t __meth_atomic_fetch_sub(int64_t handle, int64_t delta) {
#ifdef _WIN32
  volatile LONG64 *p = (volatile LONG64 *)(uintptr_t)handle;
  return InterlockedExchangeAdd64(p, -delta);
#else
  _Atomic int64_t *p = (_Atomic int64_t *)(uintptr_t)handle;
  return atomic_fetch_sub_explicit(p, delta, memory_order_seq_cst);
#endif
}

// Returns old value. If old == expected, stores desired.
int64_t __meth_atomic_cas(int64_t handle, int64_t expected, int64_t desired) {
#ifdef _WIN32
  volatile LONG64 *p = (volatile LONG64 *)(uintptr_t)handle;
  return InterlockedCompareExchange64(p, desired, expected);
#else
  _Atomic int64_t *p = (_Atomic int64_t *)(uintptr_t)handle;
  int64_t exp = expected;
  atomic_compare_exchange_strong_explicit(p, &exp, desired,
                                          memory_order_seq_cst,
                                          memory_order_seq_cst);
  return exp;
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Channel<T>
//
// A bounded/unbounded MPMC channel backed by a mutex + condition variable.
// Elements are int64_t (the universal value representation).
// cap == 0 means unbounded (backed by a growable ring buffer).
// ─────────────────────────────────────────────────────────────────────────────

typedef struct {
  int64_t  *buf;
  size_t    cap;      // allocated slots (0 = unbounded, grows on demand)
  size_t    len;      // number of items currently in buffer
  size_t    head;     // read index
  size_t    tail;     // write index
  int       closed;
#ifdef _WIN32
  SRWLOCK       lock;
  CONDITION_VARIABLE not_empty;
  CONDITION_VARIABLE not_full;
#else
  pthread_mutex_t lock;
  pthread_cond_t  not_empty;
  pthread_cond_t  not_full;
#endif
} MethChannel;

static int meth_chan_grow(MethChannel *ch) {
  size_t new_cap = ch->cap == 0 ? 16 : ch->cap * 2;
  int64_t *new_buf = (int64_t *)malloc(new_cap * sizeof(int64_t));
  if (!new_buf) return 0;
  // Copy elements in order
  for (size_t i = 0; i < ch->len; i++)
    new_buf[i] = ch->buf[(ch->head + i) % (ch->cap ? ch->cap : 1)];
  free(ch->buf);
  ch->buf = new_buf;
  ch->cap = new_cap;
  ch->head = 0;
  ch->tail = ch->len;
  return 1;
}

// __meth_chan_new(capacity) -> MethChannel* as int64
// capacity == 0: unbounded
int64_t __meth_chan_new(int64_t capacity) {
  MethChannel *ch = (MethChannel *)calloc(1, sizeof(MethChannel));
  if (!ch) return 0;

  if (capacity > 0) {
    ch->buf = (int64_t *)malloc((size_t)capacity * sizeof(int64_t));
    if (!ch->buf) { free(ch); return 0; }
    ch->cap = (size_t)capacity;
  }
  // cap==0: unbounded, buf is NULL initially, grows on first send

#ifdef _WIN32
  InitializeSRWLock(&ch->lock);
  InitializeConditionVariable(&ch->not_empty);
  InitializeConditionVariable(&ch->not_full);
#else
  pthread_mutex_init(&ch->lock, NULL);
  pthread_cond_init(&ch->not_empty, NULL);
  pthread_cond_init(&ch->not_full, NULL);
#endif

  return (int64_t)(uintptr_t)ch;
}

// __meth_chan_send(chan_handle, value) — blocks if bounded and full
int64_t __meth_chan_send(int64_t handle, int64_t value) {
  MethChannel *ch = (MethChannel *)(uintptr_t)handle;
  if (!ch) return -1;

#ifdef _WIN32
  AcquireSRWLockExclusive(&ch->lock);
  if (ch->cap > 0) {
    while (ch->len >= ch->cap && !ch->closed)
      SleepConditionVariableSRW(&ch->not_full, &ch->lock, INFINITE, 0);
  }
  if (ch->closed) { ReleaseSRWLockExclusive(&ch->lock); return -1; }
  // Unbounded: grow if needed
  if (ch->cap == 0 || ch->len >= ch->cap) {
    if (!meth_chan_grow(ch)) {
      ReleaseSRWLockExclusive(&ch->lock);
      return -1;
    }
  }
  ch->buf[ch->tail] = value;
  ch->tail = (ch->tail + 1) % ch->cap;
  ch->len++;
  WakeConditionVariable(&ch->not_empty);
  ReleaseSRWLockExclusive(&ch->lock);
#else
  pthread_mutex_lock(&ch->lock);
  if (ch->cap > 0) {
    while (ch->len >= ch->cap && !ch->closed)
      pthread_cond_wait(&ch->not_full, &ch->lock);
  }
  if (ch->closed) { pthread_mutex_unlock(&ch->lock); return -1; }
  if (ch->cap == 0 || ch->len >= ch->cap) {
    if (!meth_chan_grow(ch)) {
      pthread_mutex_unlock(&ch->lock);
      return -1;
    }
  }
  ch->buf[ch->tail] = value;
  ch->tail = (ch->tail + 1) % ch->cap;
  ch->len++;
  pthread_cond_signal(&ch->not_empty);
  pthread_mutex_unlock(&ch->lock);
#endif

  return 0;
}

// __meth_chan_recv(chan_handle) — blocks until a value is available
int64_t __meth_chan_recv(int64_t handle) {
  MethChannel *ch = (MethChannel *)(uintptr_t)handle;
  if (!ch) return 0;

  int64_t value = 0;

#ifdef _WIN32
  AcquireSRWLockExclusive(&ch->lock);
  while (ch->len == 0 && !ch->closed)
    SleepConditionVariableSRW(&ch->not_empty, &ch->lock, INFINITE, 0);
  if (ch->len > 0) {
    value = ch->buf[ch->head];
    ch->head = (ch->head + 1) % (ch->cap ? ch->cap : 1);
    ch->len--;
    WakeConditionVariable(&ch->not_full);
  }
  ReleaseSRWLockExclusive(&ch->lock);
#else
  pthread_mutex_lock(&ch->lock);
  while (ch->len == 0 && !ch->closed)
    pthread_cond_wait(&ch->not_empty, &ch->lock);
  if (ch->len > 0) {
    value = ch->buf[ch->head];
    ch->head = (ch->head + 1) % (ch->cap ? ch->cap : 1);
    ch->len--;
    pthread_cond_signal(&ch->not_full);
  }
  pthread_mutex_unlock(&ch->lock);
#endif

  return value;
}

// __meth_chan_try_recv(chan_handle) — non-blocking, returns 0 if empty
int64_t __meth_chan_try_recv(int64_t handle) {
  MethChannel *ch = (MethChannel *)(uintptr_t)handle;
  if (!ch) return 0;

  int64_t value = 0;

#ifdef _WIN32
  AcquireSRWLockExclusive(&ch->lock);
  if (ch->len > 0) {
    value = ch->buf[ch->head];
    ch->head = (ch->head + 1) % (ch->cap ? ch->cap : 1);
    ch->len--;
    WakeConditionVariable(&ch->not_full);
  }
  ReleaseSRWLockExclusive(&ch->lock);
#else
  pthread_mutex_lock(&ch->lock);
  if (ch->len > 0) {
    value = ch->buf[ch->head];
    ch->head = (ch->head + 1) % (ch->cap ? ch->cap : 1);
    ch->len--;
    pthread_cond_signal(&ch->not_full);
  }
  pthread_mutex_unlock(&ch->lock);
#endif

  return value;
}

// __meth_chan_close(chan_handle) — wake all blocked recvers with zero
int64_t __meth_chan_close(int64_t handle) {
  MethChannel *ch = (MethChannel *)(uintptr_t)handle;
  if (!ch) return 0;
#ifdef _WIN32
  AcquireSRWLockExclusive(&ch->lock);
  ch->closed = 1;
  WakeAllConditionVariable(&ch->not_empty);
  WakeAllConditionVariable(&ch->not_full);
  ReleaseSRWLockExclusive(&ch->lock);
#else
  pthread_mutex_lock(&ch->lock);
  ch->closed = 1;
  pthread_cond_broadcast(&ch->not_empty);
  pthread_cond_broadcast(&ch->not_full);
  pthread_mutex_unlock(&ch->lock);
#endif
  return 0;
}
