# Heap Allocator Runtime

Mettle no longer ships a tracing garbage collector. The historical `gc_*` API remains as the bundled heap allocation runtime ABI because generated code already calls `gc_alloc`, `gc_init`, and `gc_shutdown`.

`new` and string concatenation allocate zeroed heap memory through `gc_alloc`. Allocations are tracked so `gc_shutdown` can release them at process teardown, but unreachable objects are not collected during execution.

## When to Use `new`

Use `new` when you want a zero-initialized heap object whose lifetime is effectively process/runtime lifetime:

```mettle
struct Point { x: int32; y: int32; }

function main() -> int32 {
  var p: Point* = new Point;
  p.x = 10;
  p.y = 20;
  return p.x + p.y;
}
```

Use C `malloc`/`calloc`/`realloc`/`free` from `std/mem` when you need explicit lifetime control, C interop ownership, reusable buffers, or hot paths where teardown-only allocation is not appropriate.

## Linking

### Recommended Windows Flow

When your program uses `new`, string concatenation, async features, or runtime crash tracebacks, use:

```bat
mettle --build main.mettle -o main.exe
```

This links the bundled runtime objects automatically.

### Manual Flow

If you use the manual assembly/link pipeline, link the bundled runtime object. Async programs also need `async_runtime.o`:

```bash
mettle main.mettle -o main.s
nasm -f win64 main.s -o main.o
gcc -nostartfiles main.o path/to/runtime/gc.o -o main -lkernel32
```

```bash
gcc -nostartfiles main.o path/to/runtime/gc.o path/to/runtime/async_runtime.o -o main -lkernel32
```

The compiler emits calls to `gc_alloc` for `new` expressions and heap-backed string helpers. The entry point calls `gc_init` before `main` and `gc_shutdown` before process exit.

## Allocation Semantics

`gc_alloc(size)` allocates `size` payload bytes plus a small runtime header. The payload is zeroed before it is returned. For `new T`, that means pointer fields start as null and integer fields start as 0.

`gc_alloc(0)` returns null. For nonzero sizes, allocation failure is fatal: the runtime prints an error and exits the process. It does not return null on out-of-memory.

There is no mark phase, sweep phase, root scan, safepoint, or stop-the-world pause. `gc_collect` and `gc_collect_now` are compatibility no-ops.

## Runtime API

The runtime (`gc.h`) exposes these functions:

| Function | Purpose |
|----------|---------|
| `gc_init(void *stack_base)` | Compatibility initialization; called by the generated entry point |
| `gc_alloc(size_t size)` | Allocate zeroed tracked memory |
| `gc_collect(void *current_rsp)` | Compatibility no-op |
| `gc_collect_now(void)` | Compatibility no-op |
| `gc_safepoint(void *current_rsp)` | Compatibility no-op |
| `gc_register_root(void **root_slot)` | Compatibility no-op |
| `gc_unregister_root(void **root_slot)` | Compatibility no-op |
| `gc_thread_attach(void)` | Compatibility no-op |
| `gc_thread_detach(void)` | Compatibility no-op |
| `gc_set_collection_threshold(size_t bytes)` | Store a diagnostic threshold value; does not trigger collection |
| `gc_get_collection_threshold(void)` | Read the diagnostic threshold value |
| `gc_get_allocation_count(void)` | Number of tracked allocations since last shutdown |
| `gc_get_allocated_bytes(void)` | Total tracked payload bytes since last shutdown |
| `gc_get_tlab_chunk_count(void)` | Always returns 0; the TLAB allocator was removed |
| `gc_shutdown(void)` | Free all tracked allocations and reset runtime state |

`gc_get_allocation_count` and `gc_get_allocated_bytes` are diagnostics. Since no collection runs, they represent retained allocations until `gc_shutdown`.

`gc_shutdown` is safe to call multiple times. After the first call, the heap list is empty and later calls are effectively no-ops.

## Threads and Async

Allocation is synchronized internally. `gc_thread_attach`, `gc_thread_detach`, and `gc_safepoint` remain exported for older generated code and stdlib wrappers, but they no longer maintain collector thread state.

Async task contexts are allocated through the same heap runtime. Embedders should still shut down the async runtime before calling `gc_shutdown` so worker threads are joined before process-owned runtime memory is released.

## Debugging Memory Growth

If memory grows without bound, inspect allocation sites rather than root visibility:

- Search IR or assembly for `gc_alloc` calls to find `new` expressions and heap-backed string operations.
- Use `gc_get_allocation_count()` and `gc_get_allocated_bytes()` to log retained heap growth.
- Prefer `std/mem` allocation plus explicit `free` for buffers and C-owned lifetimes.

The `-d`/`--debug` dump flags can help locate unexpected allocation. See [Compilation](compilation.md#compiler-debugging).
