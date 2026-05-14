# Known Limitations

This document lists current limitations of the Methlang language and compiler.

No block comments. Only line comments (`//`) are supported.

No top-level constant expressions. Use functions that return constant values instead.

Traits and constrained generics currently support only marker-style bounds with explicit `impl Trait for Type;` declarations and a single inline bound per type parameter such as `T: Addable`. Trait methods, multiple bounds, and `where` clauses are not implemented yet. The keyword `where` is reserved by the lexer but is not accepted in the grammar.

`match` on tagged enums is implemented as a **statement** only. There is no `match` expression form that yields a value.

Tagged-enum constructors are currently function-like. Payload variants use `Some(x)`, and payloadless variants use empty call syntax such as `None()`.

`switch` case values must be compile-time constant integer expressions. Range-style cases (e.g. `case 1..10`) are not supported.

Optimization passes are limited. The `-O` flag enables some optimizations.

Struct-by-value passing to functions can have ABI quirks; prefer pointers for large structs.

No pointer arithmetic with `ptr + n`. Use indexing `ptr[i]` instead, which scales by element size.

Null pointer dereference is diagnosed for constant nulls such as `*0`. Runtime null checks are emitted for dynamic dereference and pointer-based indexing in normal builds, but are disabled in `--release`. Pointers originating from C or inline assembly can still be invalid in ways the compiler cannot prove.

Fixed-size array indexing is checked at compile time for constant indices and guarded at runtime for dynamic indices in normal builds; those runtime guards are disabled in `--release`. Pointer indexing is still unchecked for bounds because the compiler does not know the pointee extent.

No compound assignment (`+=`, `-=`, `*=`, `/=`). Use `x = x + 1` instead of `x += 1`.

No labeled `break` or `continue` (e.g. `break outer`). Use flags or restructure nested loops.

Deferred calls capture variables by reference, not by value. In loops, copy the current value into a temporary first if the deferred call should see the declaration-time value.

`errdefer` is function-only and convention-based. It is valid only inside functions, and any non-zero explicit return value is treated as an error.

`await` is blocking under the default **`pool`** async model (`--async-model pool` or omitted): it waits on an executor worker thread and does not suspend the current function as a stackless coroutine. The experimental **`coroutine`** model (`--async-model coroutine`) lowers to the stackless `meth_coro_*` runtime; it is not a complete language-level non-blocking I/O story yet.

`coroutine` lowering now uses a generic CFG-level async rewrite path and no longer depends on pattern-specific/fallback branches for internal `await` bodies. The coroutine path is still experimental and evolving, especially around optimization quality and diagnostics.

By default, async uses a bounded worker-pool executor, not a user-facing coroutine scheduler. Optional **`coroutine`** lowering targets stackless tasks and can use Windows IOCP primitives from the bundled runtime; there is still no full portable reactor in the language for all I/O.

Blocking `await` can still deadlock on cyclic wait patterns (for example, futures waiting on each other in a cycle). The runtime mitigates common nested-await starvation, but it is not a full deadlock-proof scheduler.

Cancellation is cooperative only. `cancel(future)` sets a flag; the async task must poll `cancelled()` and exit on its own.

Async runtime shutdown is explicit for embedders. Call `meth_async_runtime_shutdown(...)` before `gc_shutdown()`. If a task ignores cooperative cancellation forever, graceful drain/abort can time out instead of forcing unsafe thread termination.

Coroutine frame GC visibility currently comes from lifted frame locals stored as fields on the heap async context plus conservative scanning. A separate precise coroutine root-map format is not implemented yet.

Unreachable code analysis is currently block-local and conservative; some dead paths in complex control-flow may not be diagnosed yet.

String concatenation via `+` is now supported, but it allocates via the GC runtime (`gc_alloc`). Use `methlang --build` or otherwise link the bundled GC runtime before using `string + string`.

Managed pointers that cross into C remain a hazard. The compiler now warns when a managed struct pointer is passed to an `extern function` or stored in an `extern` variable, but C code that retains such pointers must still register the storage slot with `gc_register_root`.

No conditional imports. All `import` directives are unconditional; there is no platform or flag-based import.

`std/net` and the web server example are Windows-only (Winsock2). Use POSIX socket externs for networking on Linux.
