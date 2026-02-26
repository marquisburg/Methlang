# Known Limitations

This document lists current limitations of the MethASM language and compiler.

No block comments. Only line comments (`//`) are supported.

No top-level constant expressions. Use functions that return constant values instead.

`switch` case values must be compile-time constant integer expressions. Range-style cases (e.g. `case 1..10`) are not supported.

Optimization passes are limited. The `-O` flag enables some optimizations.

Struct-by-value passing to functions can have ABI quirks; prefer pointers for large structs.

Prelude is opt-in (`--prelude`) and not loaded by default.

Unary logical NOT (`!`) is not supported. Use comparisons or `== 0` / `!= 0` patterns.


No explicit cast syntax. The compiler may suggest `(type)value` in error messages, but the parser does not support it. Use implicit conversions or restructure the code.

No pointer arithmetic with `ptr + n`. Use indexing `ptr[i]` instead, which scales by element size.

No compound assignment (`+=`, `-=`, `*=`, `/=`). Use `x = x + 1` instead of `x += 1`.

No labeled `break` or `continue` (e.g. `break outer`). Use flags or restructure nested loops.

Unreachable code analysis is currently block-local and conservative; some dead paths in complex control-flow may not be diagnosed yet.

No function pointers. Functions cannot be passed as arguments or stored in variables. For callbacks, use C externs.

No string concatenation with `+`. Allocate a buffer and copy bytes manually.

No conditional imports. All `import` directives are unconditional; there is no platform or flag-based import.

`std/net` and the web server example are Windows-only (Winsock2). Use POSIX socket externs for networking on Linux.
