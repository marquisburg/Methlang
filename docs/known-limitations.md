# Known Limitations

This document lists current limitations of the Mettle language and compiler.

No top-level constant expressions. Use functions that return constant values instead.

Traits and constrained generics support inline bounds, multiple bounds, trailing `where` clauses on functions and structs, explicit impls, and trait method declarations with concrete impl method bodies. Generic trait-method calls on named values are monomorphized to concrete impl functions.

`match` on tagged enums supports both a **statement** form (arm bodies are `{ ... }` blocks) and an **expression** form that yields a value. In expression form each arm body is a single value-yielding expression (e.g. `match (o) { case Some(v): v + 1, default: 0 }`); all arm types must unify and the match must be exhaustive (a `default:` arm or all variants covered) since it must always produce a value.

Tagged-enum constructors are currently function-like. Payload variants use `Some(x)`, and payloadless variants use empty call syntax such as `None()`.

`switch` case values must be compile-time constant integer expressions. Range-style cases (e.g. `case 1..10`) are not supported.

Optimization passes are limited. The `-O` flag enables some optimizations.

Struct-by-value passing to functions can have ABI quirks; prefer pointers for large structs.

No pointer arithmetic with `ptr + n`. Use indexing `ptr[i]` instead, which scales by element size.

Null pointer dereference is diagnosed for constant nulls such as `*0`. Runtime null checks are emitted for dynamic dereference and pointer-based indexing in normal builds, but are disabled in `--release`. Pointers originating from C or inline assembly can still be invalid in ways the compiler cannot prove.

Fixed-size array indexing is checked at compile time for constant indices and guarded at runtime for dynamic indices in normal builds; those runtime guards are disabled in `--release`. Pointer indexing is still unchecked for bounds because the compiler does not know the pointee extent.

Deferred calls capture variables by reference, not by value. In loops, copy the current value into a temporary first if the deferred call should see the declaration-time value.

`errdefer` is function-only and convention-based. It is valid only inside functions, and any non-zero explicit return value is treated as an error.

`await` is blocking under the default `**pool`** async model (`--async-model pool` or omitted): it waits on an executor worker thread and does not suspend the current function as a stackless coroutine. The experimental `**coroutine**` model (`--async-model coroutine`) lowers to the stackless `meth_coro_*` runtime; it is not a complete language-level non-blocking I/O story yet.

`coroutine` lowering now uses a generic CFG-level async rewrite path and no longer depends on pattern-specific/fallback branches for internal `await` bodies. The coroutine path is still experimental and evolving, especially around optimization quality and diagnostics.

By default, async uses a bounded worker-pool executor, not a user-facing coroutine scheduler. Optional `**coroutine**` lowering targets stackless tasks on a **portable reactor** (IOCP on Windows, `poll(2)` + self-pipe on POSIX), with identical API and event semantics on both and runtime-level test coverage on each. The remaining gap is at the **language level**: there is not yet a complete, ergonomic non-blocking `await` story across all I/O kinds built on top of that reactor.

Blocking `await` can still deadlock on cyclic wait patterns (for example, futures waiting on each other in a cycle). The runtime mitigates common nested-await starvation, but it is not a full deadlock-proof scheduler.

Cancellation is cooperative only. `cancel(future)` sets a flag; the async task must poll `cancelled()` and exit on its own.

Async runtime shutdown is explicit for embedders. Call `meth_async_runtime_shutdown(...)` before `gc_shutdown()`. If a task ignores cooperative cancellation forever, graceful drain/abort can time out instead of forcing unsafe thread termination.

Coroutine frame GC visibility currently comes from lifted frame locals stored as fields on the heap async context plus conservative scanning. A separate precise coroutine root-map format is not implemented yet.

Unreachable code analysis is currently block-local and conservative; some dead paths in complex control-flow may not be diagnosed yet.

String concatenation via `+` is now supported, but it allocates via the GC runtime (`gc_alloc`). Use `mettle --build` or otherwise link the bundled GC runtime before using `string + string`.

Managed pointers that cross into C remain a hazard. The compiler now warns when a managed struct pointer is passed to an `extern function` or stored in an `extern` variable, but C code that retains such pointers must still register the storage slot with `gc_register_root`.

No conditional imports. All `import` directives are unconditional; there is no platform or flag-based import.

`std/net` and the web server example are Windows-only (Winsock2). Use POSIX socket externs for networking on Linux.
