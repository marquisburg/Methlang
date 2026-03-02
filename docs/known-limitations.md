# Known Limitations

This document lists current limitations of the Methlang language and compiler.

No block comments. Only line comments (`//`) are supported.

No top-level constant expressions. Use functions that return constant values instead.

Generic type parameters are unconstrained. Operations inside generic bodies must be valid for all possible type arguments; the type checker validates at instantiation time. No generic constraints or trait bounds.

`switch` case values must be compile-time constant integer expressions. Range-style cases (e.g. `case 1..10`) are not supported.

Optimization passes are limited. The `-O` flag enables some optimizations.

Struct-by-value passing to functions can have ABI quirks; prefer pointers for large structs.

Prelude is opt-in (`--prelude`) and not loaded by default.


No pointer arithmetic with `ptr + n`. Use indexing `ptr[i]` instead, which scales by element size.

Null pointer dereference is diagnosed for constant nulls such as `*0`. Runtime null checks are emitted for dynamic dereference and pointer-based indexing in normal builds, but are disabled in `--release`. Pointers originating from C or inline assembly can still be invalid in ways the compiler cannot prove.

Fixed-size array indexing is checked at compile time for constant indices and guarded at runtime for dynamic indices in normal builds; those runtime guards are disabled in `--release`. Pointer indexing is still unchecked for bounds because the compiler does not know the pointee extent.

No compound assignment (`+=`, `-=`, `*=`, `/=`). Use `x = x + 1` instead of `x += 1`.

No labeled `break` or `continue` (e.g. `break outer`). Use flags or restructure nested loops.

Deferred calls capture variables by reference, not by value. In loops, copy the current value into a temporary first if the deferred call should see the declaration-time value.

`errdefer` is function-only and convention-based. It is valid only inside functions, and any non-zero explicit return value is treated as an error.

Unreachable code analysis is currently block-local and conservative; some dead paths in complex control-flow may not be diagnosed yet.

String concatenation via `+` is now supported, but it allocates via the GC runtime (`gc_alloc`). Link `gc.c` and initialize the runtime before using `string + string`.

Managed pointers that cross into C remain a hazard. The compiler now warns when a managed struct pointer is passed to an `extern function` or stored in an `extern` variable, but C code that retains such pointers must still register the storage slot with `gc_register_root`.

No conditional imports. All `import` directives are unconditional; there is no platform or flag-based import.

`std/net` and the web server example are Windows-only (Winsock2). Use POSIX socket externs for networking on Linux.
