# Async and Sync Execution

Mettle supports both ordinary synchronous functions and asynchronous functions. **Async lowering is selectable** at compile time:

- **`pool` (default)** — `async` creates a `Future<T>` handle, work runs on a **bounded worker-pool executor**, and `await` **blocks an OS thread** until the future completes. This is the **stable** model described throughout most of this document.
- **`coroutine` (experimental)** — the compiler rewrites `async`/`await` toward **stackless task frames** and the **`meth_coro_*` runtime** (see [Coroutine reactor preview](#coroutine-reactor-preview-c-runtime-api)). The reactor is now **portable**: Windows uses IOCP and POSIX (Linux/macOS) uses a `poll(2)` + self-pipe backend exposing the **same API and `EVENT_WAKE`/`EVENT_IO`/`EVENT_IO_ERROR` semantics**, so the stackless lowering behaves identically on both. Scope, language-level ergonomics, optimization, and diagnostics are still evolving; use **`--async-model coroutine`** only when you intentionally need this path.

Pass **`--async-model pool`** or **`--async-model coroutine`** to the compiler (`mettle`). Omitting the flag is equivalent to **`pool`**.

The **`pool`** model is **not** a coroutine/event-loop model: it is explicit futures, blocking waits, cooperative cancellation, and a bounded pool of long-lived worker threads.

## Quick Comparison

| Topic | Synchronous function | Asynchronous function |
|------|------|------|
| Declaration | `function f(...) -> T` or `fn f(...) -> T` | `async function f(...) -> T` or `async fn f(...) -> T` |
| Call result | `T` | `Future<T>` |
| Execution start | Runs immediately on the caller thread | Evaluates arguments, allocates task context, enqueues task on the async executor |
| Wait behavior | Caller stays inside the call until return | Caller gets a future immediately; `await` waits later |
| Runtime requirement | None beyond normal program/runtime needs | Requires the bundled async runtime and GC runtime (coroutine model also relies on coroutine/IOCP pieces in the same async runtime object where applicable) |

## Declaring Functions

Mettle accepts both `function` and `fn` spelling for ordinary functions:

```mettle
function add(a: int32, b: int32) -> int32 {
  return a + b;
}

fn mul(a: int32, b: int32) -> int32 {
  return a * b;
}
```

Async functions add the `async` keyword in front of the declaration:

```mettle
async function load_count() -> int32 {
  return 42;
}

async fn load_value() -> int32 {
  return 7;
}
```

The source-level return type of an async function names the **payload type**, not the future handle type. In the example above:

- `load_count()` has expression type `Future<int32>`
- `await load_count()` has expression type `int32`

For `async fn work() -> void`, the call expression has type `Future<void>`.

## Sync Call Semantics

Synchronous calls are ordinary direct calls:

- The callee runs on the caller's current thread.
- The caller does not continue until the callee returns.
- Parameters are passed according to the normal ABI rules.
- There is no implicit task allocation, scheduling, or cancellation.

```mettle
function inc(x: int32) -> int32 {
  return x + 1;
}

function main() -> int32 {
  return inc(41);  // direct call, immediate result
}
```

## Async Call Semantics

The following describes the **`pool`** lowering model (default). The **`coroutine`** model uses a different rewrite (stackless tasks); see [Coroutine reactor preview](#coroutine-reactor-preview-c-runtime-api).

An async call still evaluates its arguments at the call site, but instead of returning the payload immediately it:

1. Evaluates the arguments.
2. Allocates a runtime-owned async context on the managed heap.
3. Copies argument values into that context.
4. Enqueues work on the async executor.
5. Returns a `Future<T>` handle immediately.

```mettle
async fn add_one(x: int32) -> int32 {
  return x + 1;
}

function main() -> int32 {
  var future: Future<int32> = add_one(41);
  return await future;
}
```

Important details:

- Scalar and struct arguments are copied into the async context.
- Pointer arguments copy the pointer value only; the pointee is still shared.
- The future is an opaque handle to shared task state. Treat it as a runtime object, not a user-defined struct.

## `Future<T>`

`Future<T>` is the built-in future/promise-shaped type used by async calls.

Use it when you want to:

- store an async result for later,
- pass an in-flight task to another function,
- request cancellation with `cancel(future)`,
- wait explicitly with `await future`.

```mettle
async fn fetch() -> int32 {
  return 5;
}

function use_future() -> int32 {
  var f: Future<int32> = fetch();
  var x: int32 = await f;
  return x;
}
```

Current contract:

- `Future<T>` is pointer-sized on x86-64.
- It carries task state plus storage for the eventual result.
- The payload type `T` is part of the static type system.
- `await` requires a `Future<T>` operand.

The implementation currently allows low-level casts between futures, pointers, and integers, but ordinary code should treat futures as opaque handles.

## `await`

`await` is a unary expression operator.

```mettle
async fn add_one(x: int32) -> int32 {
  return x + 1;
}

function main() -> int32 {
  var value: int32 = await add_one(41);
  return value;
}
```

Behavior (**`pool`** model):

- `await future` blocks until the future completes on the executor.
- The expression result is the future payload type `T`.
- `await` is valid in both synchronous and asynchronous functions.
- Under **`pool`**, `await` does **not** suspend the current function as a stackless coroutine; it **blocks the current OS thread** (caller thread in sync functions, executor worker thread in async functions).

That last point is the most important distinction from many async languages when using **`pool`**:

- in a synchronous function, `await` blocks the caller thread;
- in an async function, `await` blocks that async worker thread.

There is no central reactor and no cooperative resumption of the caller stack in the **`pool`** model, and no promise chaining hidden behind the scenes.

With **`--async-model coroutine`**, lowering targets the stackless task runtime instead; consult tests and runtime APIs for current capabilities.

## Call Combinations

| Caller | Callee | Result |
|------|------|------|
| sync | sync | direct call on caller thread |
| sync | async | future returned immediately; task is queued to the executor |
| sync | async + `await` | caller thread blocks until task completes |
| async | sync | direct call on the async worker thread |
| async | async | child future returned immediately; child task is queued |
| async | async + `await` | parent worker thread blocks while waiting for the child |

This makes Mettle async predictable, but it also means `await` can still block worker threads.

## Executor Configuration

The async runtime uses a bounded worker-pool executor:

- `METH_ASYNC_WORKERS` sets the worker thread count `N`.
- `METH_ASYNC_QUEUE_CAPACITY` sets the maximum queued task count.
- If not set, `N` defaults to logical CPU count (bounded internally), and queue capacity defaults to a bounded multiple of `N`.
- For embedding/tests, `meth_async_runtime_configure(worker_count, queue_capacity)` can set both values before the first async task starts.

### Runtime Lifecycle and Shutdown

The executor now has explicit lifecycle states:

- `RUNNING` (`METH_ASYNC_RUNTIME_STATE_RUNNING`): accepts new `meth_async_start`, workers execute queued work.
- `DRAINING` (`METH_ASYNC_RUNTIME_STATE_DRAINING`): no new starts; existing accepted tasks are allowed to finish.
- `STOPPING` (`METH_ASYNC_RUNTIME_STATE_STOPPING`): no new starts; shutdown in progress.
- `STOPPED` (`METH_ASYNC_RUNTIME_STATE_STOPPED`): workers joined and runtime resources released.

Use `meth_async_runtime_shutdown(kind, timeout_ms)`:

- `METH_ASYNC_SHUTDOWN_ABORT`:
  rejects new work immediately, purges queued tasks as canceled terminal tasks, wakes waiters, requests cooperative cancellation for in-flight tasks (`cancelled()` becomes true while stopping), joins workers, then reaches `STOPPED`.
- `METH_ASYNC_SHUTDOWN_DRAIN`:
  rejects new work, waits for accepted tasks to reach terminal state, then stops workers and reaches `STOPPED`.
  If the drain deadline is reached, runtime falls back to `STOPPING`/abort behavior and returns failure if shutdown still cannot complete before timeout.

`timeout_ms < 0` means wait indefinitely. Repeated shutdown calls are idempotent and return success once already `STOPPED`.

`meth_async_runtime_outstanding_task_count()` reports accepted-but-not-terminal tasks. Drain completion uses this accounting (plus empty queue) under the executor lock.

After `STOPPED`, `meth_async_start` is rejected until `meth_async_runtime_reset()` is called. Reset is mainly intended for embedders/tests that need explicit runtime re-init in a single process.

Teardown call behavior:

- `meth_async_start(...)` returns failure in `DRAINING`, `STOPPING`, or `STOPPED`.
- `meth_async_wait(...)` keeps normal behavior in `RUNNING`/`DRAINING`.
- Once runtime is `STOPPING`, `meth_async_wait(...)` returns failure for non-terminal futures so waiters do not block forever during teardown.

How `N` affects behavior:

- At most `N` async tasks execute at the same time.
- Additional async calls queue up to the configured capacity.
- `await` still blocks the caller thread; increasing `N` can reduce queue wait under load but does not make `await` non-blocking.

### Queue Bounds and Backpressure

The queue is bounded to avoid unbounded memory growth under overload. When the queue is full, producers block until capacity is available.

- This preserves `Future<T>` semantics (tasks are not dropped).
- Backpressure is explicit and memory-safe.

## Cancellation

Cancellation is part of the same runtime model as futures. It is **cooperative**, not preemptive.

Built-in helpers:

- `cancel(future)` requests cancellation for a future and returns an `int32` status.
- `cancelled()` returns non-zero when the current async task has a pending cancellation request.

When `cancelled()` is called outside an active async task, it returns `0`.

Example:

```mettle
async fn worker() -> int32 {
  while (cancelled() == 0) {
    // do a chunk of work
  }

  return 0;
}

function main() -> int32 {
  var future: Future<int32> = worker();
  cancel(future);
  return await future;
}
```

Key rules:

- `cancel(future)` sets a cancellation flag on the target future.
- If the task is still queued and has not started, cancellation finalizes it without running the async body.
- If the task is already running, the runtime does not forcibly stop the worker thread.
- A running task must observe `cancelled()` and decide how to stop.
- `await` still waits for the future to reach a terminal state.

If an async task never checks `cancelled()`, a cancellation request is just a flag; the work keeps running until it returns normally.

### Cancellation Propagation While Awaiting

When one async task waits on another, the runtime propagates cancellation from the waiting task to the awaited child future while blocked in `await`.

That means this pattern works as expected:

```mettle
async fn child() -> int32 {
  while (cancelled() == 0) {
  }
  return 0;
}

async fn parent() -> int32 {
  var f: Future<int32> = child();
  return await f;
}
```

If the parent future is canceled while blocked in `await f`, the runtime forwards the cancellation request to `f`.

## Threading Contract

The current threading story is:

- async tasks are executed by a bounded pool of worker OS threads,
- every future represents shared task state reachable from multiple parts of the program,
- every `await` is still a blocking wait on that future,
- cancellation is a shared flag checked cooperatively.

This implies several practical consequences:

- Async work is parallel-capable up to `N` workers.
- Under **`pool`**, async is heavier than a stackless coroutine model; one async call is not "just a suspended stack frame".
- Awaiting from an async function can still tie up a worker thread.
- The **`pool`** model does not include a built-in async I/O reactor in the language runtime (see [Coroutine reactor preview](#coroutine-reactor-preview-c-runtime-api) for the portable reactor primitives used with the coroutine path).

### Blocking `await` Deadlock Hazard

Because `await` blocks, naive pools can deadlock when all workers are waiting on queued tasks. The runtime mitigates the common nested-await starvation case by allowing a blocked pool worker to help execute queued tasks while it waits in normal `RUNNING` mode.

During shutdown (`DRAINING`/`STOPPING`), this assist behavior is disabled so teardown does not execute unrelated queued work while lifecycle transitions are in progress.

This mitigation makes typical parent-await-child patterns safe even with `METH_ASYNC_WORKERS=1`.

Still forbidden/unsafe patterns (**`pool`**):

- Cyclic waits (for example task A waits on B while B waits on A) can still deadlock.
- Under **`pool`**, designs that need **non-blocking** suspension are not supported; **`await` remains blocking** on an OS thread.

### Sendability and Cross-Thread Use

`Future<T>` is designed as a runtime handle, so it can be stored and passed around like other pointer-sized values. In practice, this means you can hand a future to code running on another thread or store it in heap data structures.

The safe mental model is:

- the future handle is shared state,
- the task result belongs to the future,
- cancellation and waiting operate on that shared handle.

Typical code should still keep ownership simple: create a future, pass it deliberately, and avoid inventing ad-hoc aliasing patterns unless you really need them.

## GC and Runtime Interaction

Async execution is tied to the runtime:

- async task contexts are allocated on the managed heap,
- the async runtime object is required for `async`, `await`, `cancel`, and `cancelled`,
- executor worker threads attach to the GC automatically.

This means:

- `mettle --build` is the easiest way to build async code on Windows,
- manual link flows must include the bundled runtime objects,
- user-created threads still need explicit `gc_thread_attach()` / `gc_thread_detach()`.

Worker teardown always runs `gc_thread_detach()` on worker-thread exit paths. For embedding, shut down async runtime before `gc_shutdown()` so worker detach/join is complete before GC global teardown.

## Coroutine reactor preview (C runtime API)

These symbols live in the bundled async runtime. They back the **experimental** **`coroutine`** async lowering (`--async-model coroutine`) and are also available to **embedders** driving the reactor and stackless tasks from C.

A **portable** reactor API is the foundation for stackless scheduling. The
same functions and event semantics are implemented on every platform; only
the OS mechanism underneath differs:

- **Windows** — backed by an I/O completion port (IOCP).
- **POSIX (Linux/macOS)** — backed by `poll(2)` with a self-pipe for
  cross-thread wakes.

The `meth_coro_iocp_runtime_*` names are kept for source/ABI compatibility
even though the implementation is no longer IOCP-specific:

- `meth_coro_iocp_runtime_init()`
- `meth_coro_iocp_runtime_shutdown()`
- `meth_coro_iocp_runtime_register_socket(socket_handle, token)` — on POSIX,
  `socket_handle` is any pollable file descriptor cast to int64.
- `meth_coro_iocp_runtime_post_wake(token, result)`
- `meth_coro_iocp_runtime_poll(timeout_ms, out_token, out_kind, out_result)`

Event kinds are:

- `METH_CORO_IOCP_EVENT_WAKE`
- `METH_CORO_IOCP_EVENT_IO`
- `METH_CORO_IOCP_EVENT_IO_ERROR`

An experimental stackless task-frame scheduler API is also available:

- `meth_coro_task_create(step_fn, state)`
- `meth_coro_task_schedule(task_handle, wake_token, wake_kind, wake_result)`
- `meth_coro_task_bind_token(task_handle, token)`
- `meth_coro_task_run_one(timeout_ms)`
- `meth_coro_task_is_done(task_handle)`
- `meth_coro_task_destroy(task_handle)`

The scheduler is resumable-state based (no per-task native thread stack) and interoperates with reactor wake tokens. The scheduler is itself portable (Windows uses an SRW lock, POSIX a pthread mutex; behaviour is identical). With **`--async-model coroutine`**, the compiler **does** rewrite Meth `async`/`await` to use this machinery (experimental). You can also call these APIs directly from C without going through Meth `async`.

Current scheduling semantics:

- `meth_coro_task_schedule` is level-triggered per task queue slot; if a task is already queued, wake metadata is updated in place (latest wake wins).
- `meth_coro_task_run_one` dispatches the explicit ready queue first, then polls the reactor.
- Reactor token dispatch resolves via `meth_coro_task_bind_token`; if no binding exists, the token value is interpreted as a direct task handle for compatibility.

## Build and Link Requirements

### Recommended Windows Build

Use:

```powershell
.\bin\mettle.exe --build --emit-obj --linker internal app.mettle -o app.exe
```

or the simpler auto path:

```powershell
.\bin\mettle.exe --build app.mettle -o app.exe
```

When the program uses async features, the build links the bundled async runtime automatically.

### Manual Assembly/Link Flow

If you assemble and link manually, async programs need both the GC runtime and the async runtime:

```bash
mettle app.mettle -o app.s
nasm -f win64 app.s -o app.o
gcc -nostartfiles app.o path/to/runtime/gc.o path/to/runtime/async_runtime.o -o app -lkernel32
```

If your entry point is `main(argc, argv)`, also link `mettle_entry.o` as documented in [Compilation](compilation.md).

On POSIX toolchains, the bundled async runtime uses a pthread-backed implementation for **both** the `pool` executor and the `coroutine` reactor/scheduler. Link pthread support (`-lpthread`, or as required by your system toolchain). The coroutine reactor additionally relies only on standard POSIX `poll(2)`, `pipe(2)`, and `fcntl(2)` — no `epoll`/`kqueue` dependency, so it builds unmodified on Linux and macOS.

## Common Patterns

### Start Now, Await Later

```mettle
async fn load_a() -> int32 { return 10; }
async fn load_b() -> int32 { return 32; }

function main() -> int32 {
  var a: Future<int32> = load_a();
  var b: Future<int32> = load_b();
  return await a + await b;
}
```

### Sync Wrapper Over Async Work

```mettle
async fn compute() -> int32 {
  return 42;
}

function compute_sync() -> int32 {
  return await compute();
}
```

This is valid, but remember that the wrapper is still blocking.

### Cooperative Shutdown Loop

```mettle
async fn service() -> int32 {
  while (cancelled() == 0) {
    // poll, process, or sleep
  }
  return 0;
}
```

## What each model is (and is not)

### `pool` (default)

The **`pool`** implementation is intentionally simple. It is **not**:

- a non-blocking event loop,
- stackless coroutine suspension for Meth `async`/`await` (use **`coroutine`** lowering for that direction),
- a full green-thread/work-stealing coroutine scheduler,
- a preemptive cancellation system,
- a typed cancellation/error propagation system.

Think of **`pool`** as **typed futures + bounded worker pool + blocking await + cooperative cancel**.

### `coroutine` (experimental)

The **`coroutine`** path **is** a stackless-style lowering tied to **`meth_coro_*`** and a portable reactor (IOCP on Windows, `poll(2)` on POSIX). The runtime layer is now cross-platform and test-covered on both backends. It is still **not** a finished, language-level non-blocking `await` story for all I/O; the runtime is portable, but higher-level coverage, ergonomics, optimization, and diagnostics are still growing.

Treat **`coroutine`** as **preview**: expect rough edges compared to **`pool`**.

Current internal lowering behavior:

- Coroutine mode uses a generic CFG-level async rewrite path for arbitrary async bodies (not a pattern-only matrix).
- Suspend points are collected from `await` sites and persisted as coroutine-frame metadata (`resume_state`, `suspend_count`) in the generated async context.
- Locals that must survive suspension are carried via the heap async context, so values remain available across resumes.

GC/root visibility note:

- Coroutine-frame visibility still relies on the generated context-object field layout and current runtime scanning behavior; a separate standalone root-map artifact is not exposed as a user-facing format yet.

## See Also

- [Declarations](declarations.md) for async function syntax
- [Types](types.md) for `Future<T>`
- [Expressions](expressions.md) for `await`, `cancel`, and `cancelled`
- [Compilation](compilation.md) for runtime/build requirements
- [Garbage Collector](garbage-collector.md) for thread attachment details
- [Known Limitations](known-limitations.md) for current async caveats
