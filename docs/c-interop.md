# C Interoperability

Methlang can call C functions and access C globals. This document describes the C interop facilities and conventions.

## Calling C Functions

Declare C functions with `extern function`. Use the `= "symbol"` suffix to specify the C link name when it differs from the Methlang name. Parameters and return types must match the C ABI. On Windows, the Microsoft x64 ABI applies. On Linux and macOS, the System V AMD64 ABI applies.

```meth
extern function puts(msg: cstring) -> int32 = "puts";
extern function malloc(size: int64) -> cstring = "malloc";

function main() -> int32 {
  puts("Hello");
  var p: cstring = malloc(100);
  return 0;
}
```

## cstring

`cstring` is an alias for `uint8*`. Use it for C `char*`, `void*`, or opaque pointers. `cstring` and `uint8*` are interchangeable. When passing a Methlang `string` to a C function that expects `char*`, use `s.chars` or the `cstr` helper from `std/io`.

## Passing Structs to C

Structs are laid out in declaration order. For C interop, define the struct to match the C layout exactly. Field order, types, and alignment must be compatible. Padding between fields follows the target ABI. Avoid passing large structs by value to C; the ABI may pass them by pointer. When a C API expects a pointer to a struct, pass `&my_struct` or a `T*` variable. On MS x64, the first four arguments go in RCX, RDX, R8, R9; structs larger than 8 bytes are often passed by pointer.

```meth
struct SockAddrIn {
  sin_family: int16;
  sin_port: uint16;
  sin_addr: uint32;
  sin_zero: uint8[8];
}
```

## Linking

Link the compiled assembly with the C runtime and any required libraries. For programs using `new` (GC), include `gc.c` in the link. Example:

```bash
# Windows (Winsock2)
gcc -nostartfiles main.o gc.o -o main -lws2_32 -lkernel32
```

Use `-nostartfiles` when the program provides its own entry point (e.g. `_start` that calls `main`).

## POSIX Networking (Linux / macOS)

For networking on Linux or macOS, use `stdlib/std/net_posix.meth`. This module provides POSIX socket bindings and requires the C helper functions in `stdlib/posix_helpers.c` for thread-safe errno access and atomic operations.

**Link command:**

```bash
# Linux
gcc -o myapp output.s stdlib/posix_helpers.c -lpthread

# macOS
gcc -o myapp output.s stdlib/posix_helpers.c
```

**With GC runtime:**

```bash
# Linux
gcc -o myapp output.s stdlib/posix_helpers.c src/runtime/gc.c -lpthread

# macOS
gcc -o myapp output.s stdlib/posix_helpers.c src/runtime/gc.c
```

The socket functions (`socket`, `connect`, `bind`, `listen`, `accept`, `send`, `recv`, `close`) are in libc on both Linux and macOS, so no extra libraries are needed beyond the C runtime. The `posix_helpers.c` file provides:

- `posix_get_errno()` – thread-safe errno access
- `posix_cas_i32()` – atomic compare-and-swap
- `posix_yield()` – CPU yield for spin locks
- `posix_atomic_exchange_i32()` – atomic exchange

**Note:** On macOS, `SOL_SOCKET` is 0xFFFF and `SO_REUSEADDR` is 4. On Linux, they are 1 and 2 respectively. Use the constants from `std/net_posix` (which default to Linux values) or override them at call site for macOS.

## POSIX Networking (Linux / macOS)

For networking on Linux or macOS, use `stdlib/std/net_posix.meth`. This module provides POSIX socket bindings and requires the C helper functions in `stdlib/posix_helpers.c` for thread-safe errno access and atomic operations.

**Link command:**

```bash
# Linux
gcc -o myapp output.s stdlib/posix_helpers.c -lpthread

# macOS
gcc -o myapp output.s stdlib/posix_helpers.c
```

**With GC runtime:**

```bash
# Linux
gcc -o myapp output.s stdlib/posix_helpers.c src/runtime/gc.c -lpthread

# macOS
gcc -o myapp output.s stdlib/posix_helpers.c src/runtime/gc.c
```

The socket functions (`socket`, `connect`, `bind`, `listen`, `accept`, `send`, `recv`, `close`) are in libc on both Linux and macOS, so no extra libraries are needed beyond the C runtime. The `posix_helpers.c` file provides:

- `posix_get_errno()` – thread-safe errno access
- `posix_cas_i32()` – atomic compare-and-swap
- `posix_yield()` – CPU yield for spin locks
- `posix_atomic_exchange_i32()` – atomic exchange

**Note:** On macOS, `SOL_SOCKET` is 0xFFFF and `SO_REUSEADDR` is 4. On Linux, they are 1 and 2 respectively. Use the constants from `std/net_posix` (which default to Linux values) or override them at call site for macOS.


For networking on Linux or macOS, use `stdlib/std/net_posix.meth`. This module provides POSIX socket bindings and requires the C helper functions in `stdlib/posix_helpers.c` for thread-safe errno access and atomic operations.

**Link command:**

```bash
# Linux
gcc -o myapp output.s stdlib/posix_helpers.c -lpthread

# macOS
gcc -o myapp output.s stdlib/posix_helpers.c
```

**With GC runtime:**

```bash
# Linux
gcc -o myapp output.s stdlib/posix_helpers.c src/runtime/gc.c -lpthread

# macOS
gcc -o myapp output.s stdlib/posix_helpers.c src/runtime/gc.c
```

The socket functions (`socket`, `connect`, `bind`, `listen`, `accept`, `send`, `recv`, `close`) are in libc on both Linux and macOS, so no extra libraries are needed beyond the C runtime. The `posix_helpers.c` file provides:
- `posix_get_errno()` – thread-safe errno access
- `posix_cas_i32()` – atomic compare-and-swap
- `posix_yield()` – CPU yield for spin locks
- `posix_atomic_exchange_i32()` – atomic exchange

**Note:** On macOS, `SOL_SOCKET` is 0xFFFF and `SO_REUSEADDR` is 4. On Linux, they are 1 and 2 respectively. Use the constants from `std/net_posix` (which default to Linux values) or override them at call site for macOS.

