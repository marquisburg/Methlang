# Garbage Collector

Methlang provides an optional conservative mark-and-sweep garbage collector for heap allocation. Programs that use the `new` expression must link the GC runtime (`gc.c`). Programs that use only stack allocation and C `malloc` do not need it.

## When to Use GC

Use `new` when you want managed heap allocation: struct instances, dynamic data structures, or values that outlive the current function. The GC reclaims memory when it is no longer reachable. No explicit `free` is needed.

Use C `malloc` (from `std/mem`) when you need unmanaged memory: buffers for I/O, C interop, or when the GC runtime is intentionally omitted (simple CLI utilities can skip the GC). The Windows web server example now links `gc.c` because it uses GC-backed string concatenation for its responses, but it still mixes stack buffers and `malloc` where appropriate.

**Rule of thumb:** Use `new` for program-level data structures whose lifetime is tied to reachability (trees, graphs, long-lived caches). Use `malloc` for buffers and C interop where you control the lifetime explicitly (I/O buffers, structs passed to C APIs that expect manual free).

## Linking the GC

When your program uses `new`, compile and link the GC runtime:

```bash
Methlang main.meth -o main.s
nasm -f win64 main.s -o main.o
gcc -c src/runtime/gc.c -o gc.o -Isrc
gcc main.o gc.o -o main
```

The compiler emits calls to `gc_alloc` for `new` expressions. The entry point (`_start`) calls `gc_init` with the stack base before invoking `main`. See [Compilation](compilation.md) for the full pipeline.

## Algorithm

The GC uses **conservative mark-and-sweep**:

1. **Mark:** Starting from roots (stack, registers, registered roots), treat every word that looks like a pointer into the heap as a root. Mark all reachable allocations.
2. **Sweep:** Free allocations that were not marked.

Collection is triggered in two ways: (1) **threshold-based**, when `gc_alloc` would exceed the collection threshold (see [Collection Threshold](#collection-threshold)); (2) **explicit**, by calling `gc_collect_now()` (or `gc_collect` with the current stack pointer). The API table lists these. Use `gc_collect_now` to force a collection at a specific point (e.g. between phases, or before a latency-sensitive critical section).

**"Looks like a pointer into the heap"** means: a word-sized value that falls within the address range of a known GC allocation. The GC maintains heap bounds (`g_heap_min`, `g_heap_max`); any stack or heap word whose value lies in that range is treated as a potential pointer and the corresponding allocation is marked. No type information is needed, but integers that happen to look like heap addresses can cause false retention.

**Performance:** Mark-and-sweep pauses all execution during collection. There is no incremental or concurrent collection. For latency-sensitive programs (e.g. the web server), this is one reason to avoid `new` and use `malloc` instead.

"Conservative" means the GC may retain some unreachable memory if a non-pointer value happens to look like a valid heap address. This is a trade-off for simplicity and C ABI compatibility. No object headers or type metadata are required in the language.

## Roots

The GC scans each attached thread stack from a captured current stack pointer (safepoint) up to that thread's stack base. It also uses **registered roots**: pointer slots that the runtime knows contain managed pointers.

- **Global variables** that hold pointers are automatically registered as roots.
- **Local variables** are covered by stack scanning. They do not need `gc_register_root` because the stack scan finds them. The stack is scanned word-by-word; any local that holds a managed pointer is discovered automatically.
- **Pointers stored inside structs on the heap** are traced automatically during the mark phase. When an allocation is marked, its payload is scanned for pointer-sized values; any value that falls within heap bounds is treated as a pointer and the target allocation is marked. So a struct field that holds a pointer to another GC object is followed without explicit registration.
- For pointers stored in **non-stack, non-heap** locations (e.g. C globals, static variables in C code), use `gc_register_root` and `gc_unregister_root` so the GC can find them.

**Example (registering a root from Methlang):** When C code stores a managed pointer in a variable the GC cannot otherwise see, declare the extern and pass the address of that variable. The parameter is a double pointer because the GC needs the address of the slot containing the pointer, not the pointer itself, so it can read an updated value if the variable is reassigned.

```meth
struct Data { x: int32; y: int32; }

var c_storage: Data* = 0;  // C code will store a managed pointer here
extern function gc_register_root(slot: Data**) = "gc_register_root";
extern function gc_unregister_root(slot: Data**) = "gc_unregister_root";

function init() {
  gc_register_root(&c_storage);
  // ... C code may now store a pointer in c_storage; GC will find it
}

function cleanup() {
  gc_unregister_root(&c_storage);
}
```

Use the same pointer type for the slot parameter as the variable you are registering (e.g. `Data**` for `var c_storage: Data*`).

## Allocation

`new T` allocates `sizeof(T)` bytes via `gc_alloc`. The memory is **zeroed** before the pointer is returned. This avoids uninitialized pointer-shaped values that could confuse the conservative scanner. Zeroed memory means pointer fields start as null and integer fields start as 0, the expected default for `new`. See [Expressions](expressions.md#allocation) and [Types](types.md) for details.

**Allocation failure:** If `gc_alloc` cannot satisfy a request, it runs a collection and retries. If allocation still fails, it prints a fatal error and exits. It does not return null.

## Collection Threshold

By default, when total tracked allocations reach 1 MiB, `gc_alloc` triggers a collection before allocating. You can adjust this:

```c
// From C or extern
gc_set_collection_threshold(2 * 1024 * 1024);  // 2 MiB
size_t current = gc_get_collection_threshold();
```

The threshold check uses **projected bytes**: collection runs when `current_allocated + requested_size` exceeds the threshold, not just when `current_allocated` does. This prevents OOM scenarios where a single large allocation would push the heap over the limit without triggering a collection first. A naive implementation that only checked current bytes could fail to collect in time.

The minimum threshold is 4 KiB. Lower thresholds collect more often (less memory use, more CPU); higher thresholds collect less often (more memory, less CPU).

## Runtime API

The GC runtime (`gc.h`) exposes these functions for advanced use:

| Function | Purpose |
|----------|---------|
| `gc_init(void *stack_base)` | Initialize; called by entry point |
| `gc_alloc(size_t size)` | Allocate tracked memory |
| `gc_collect(void *current_rsp)` | Run a collection cycle |
| `gc_collect_now(void)` | Convenience: collect using current stack |
| `gc_safepoint(void *current_rsp)` | Cooperative poll point for stop-the-world |
| `gc_register_root(void **root_slot)` | Register a pointer slot as root |
| `gc_unregister_root(void **root_slot)` | Unregister a root |
| `gc_set_collection_threshold(size_t bytes)` | Set auto-collection threshold |
| `gc_get_collection_threshold(void)` | Get current threshold |
| `gc_get_allocation_count(void)` | Number of tracked allocations |
| `gc_get_allocated_bytes(void)` | Total tracked bytes |
| `gc_get_tlab_chunk_count(void)` | Diagnostic: retained TLAB chunks |
| `gc_shutdown(void)` | Free all allocations, reset state |

`gc_get_allocation_count` and `gc_get_allocated_bytes` are for diagnostics and tuning. Use them to identify unexpected allocation or to tune the collection threshold.

`gc_shutdown` is safe to call multiple times. After the first call, the heap is empty and subsequent calls are effectively no-ops. Call it once at program exit if you want to free all tracked memory before terminating.

## Programs Without GC

Programs that do not use `new` or string concatenation do not need to link `gc.c`. The Windows forum server (`web/server.meth`) now calls `gc_init` and relies on `string + string`, so it must link the GC runtime (`gc.o`); simple CLI utilities that stick to stack and `malloc` can still be built without it.

If you use `new` but forget to link `gc.c`, the linker will report undefined references:

```
undefined reference to `gc_alloc'
undefined reference to `gc_init'
```

Resolve by adding `gc.o` to the link command.

## GC and Threads

The runtime supports cooperative multi-threaded collection:

- Threads that allocate with GC should call `gc_thread_attach()` when they start and `gc_thread_detach()` before exit.
- Attached threads should reach `gc_safepoint()` periodically so stop-the-world collection can capture thread stacks.
- Collection is still stop-the-world (not concurrent/incremental), but allocation and collection are synchronized internally.

`gc_thread_detach()` now reclaims thread bookkeeping immediately. The detached thread record does not stay resident until `gc_shutdown`. Any TLAB chunks that still contain live objects remain tracked by the runtime and are reclaimed when fully dead (or at shutdown).

### Safepoint Register Visibility

Conservative correctness depends on roots being visible to the scanner at safepoints.

- Baseline x86-64 mode spills GPRs and `xmm0..xmm15` around generated safepoint calls.
- Optional wider spill mode supports AVX-512 register files: define `Methlang_SAFEPOINT_SPILL_XMM31` when building the compiler to spill `xmm0..xmm31`.

If your deployment baseline is AVX2-era machines, `xmm0..xmm15` is sufficient.

## Performance Characteristics

Collections are typically fast for small heaps (milliseconds or less) but **pause all execution**. There is no incremental or concurrent collection. Pause time grows with heap size and the number of reachable objects.

**Rough order of magnitude:** A "small heap" is on the order of hundreds to a few thousand objects and a few hundred KiB to a few MiB. Beyond roughly 10,000 to 50,000 reachable objects or tens of MiB, collection pauses can become noticeable (tens of milliseconds or more). These numbers depend heavily on object graph depth and pointer density, not just count. A linked list of 50,000 nodes with no branching marks differently than a dense graph of 1,000 nodes each pointing to 100 others. Profile with `gc_get_allocation_count` and `gc_get_allocated_bytes` to find where pauses exceed your threshold.

For programs that allocate heavily or have large object graphs, consider raising the collection threshold to reduce collection frequency, or use `malloc` for hot paths where latency matters.

## Interaction with C Interop

A common pitfall: passing a GC-managed pointer to a C function that stores it somewhere the GC cannot see (e.g. a C global, a static variable, or a struct allocated with `malloc`). When the GC runs, it will not find that pointer. It only scans the stack, registered roots, and heap object payloads. The C-held reference is invisible, so the GC may collect the object while C still holds a dangling pointer.

**Fix:** Use `gc_register_root` to register the C storage location. Pass the address of the variable that holds the pointer: `gc_register_root(&c_global_that_holds_ptr)`. When the GC runs, it will read that slot and follow the pointer. Call `gc_unregister_root` when the C code no longer holds the reference.

## Debugging Memory Issues

If the GC appears to collect too aggressively (use-after-free, premature collection), a managed pointer is likely stored where the GC cannot see it. Register that location with `gc_register_root` or ensure the pointer is on the stack or in a heap object that is reachable.

The compiler emits a warning when a managed struct pointer is passed to an `extern function` or stored in an `extern` variable, because those are common ways to let C retain a GC-managed pointer without registering the storage slot. The warning is heuristic: it does not prove that C will retain the pointer, and it does not eliminate the need to call `gc_register_root` when C-owned storage keeps the reference.

If the GC retains too much memory (growth without bound, high `gc_get_allocated_bytes`), use the diagnostic API to inspect:

- `gc_get_allocation_count()`: number of live objects
- `gc_get_allocated_bytes()`: total bytes retained

Log these during allocation or at collection time to see growth patterns. Conservative retention (integers that look like pointers) can cause extra retention; restructuring data to avoid pointer-sized values in hot structs can help. Raising the threshold reduces collection frequency but does not reduce retention. Only reachability determines what is kept.

The `-d` (debug) flag dumps the IR to `<output>.ir`. Use it to find unexpected `new` expressions. If allocation is higher than expected, search the IR for `gc_alloc` calls to identify which expressions allocate.
