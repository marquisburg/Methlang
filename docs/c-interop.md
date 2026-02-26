# C Interoperability

MethASM can call C functions and access C globals. This document describes the C interop facilities and conventions.

## Calling C Functions

Declare C functions with `extern function`. Use the `= "symbol"` suffix to specify the C link name when it differs from the MethASM name. Parameters and return types must match the C ABI. On Windows, the Microsoft x64 ABI applies. On Linux and macOS, the System V AMD64 ABI applies.

```masm
extern function puts(msg: cstring) -> int32 = "puts";
extern function malloc(size: int64) -> cstring = "malloc";

function main() -> int32 {
  puts("Hello");
  var p: cstring = malloc(100);
  return 0;
}
```

## cstring

`cstring` is an alias for `uint8*`. Use it for C `char*`, `void*`, or opaque pointers. `cstring` and `uint8*` are interchangeable. When passing a MethASM `string` to a C function that expects `char*`, use `s.chars` or the `cstr` helper from `std/io`.

## Passing Structs to C

Structs are laid out in declaration order. For C interop, define the struct to match the C layout exactly. Field order, types, and alignment must be compatible. Padding between fields follows the target ABI. Avoid passing large structs by value to C; the ABI may pass them by pointer. When a C API expects a pointer to a struct, pass `&my_struct` or a `T*` variable. On MS x64, the first four arguments go in RCX, RDX, R8, R9; structs larger than 8 bytes are often passed by pointer.

```masm
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
gcc -nostartfiles main.o gc.o -o main -lws2_32 -lkernel32
```

Use `-nostartfiles` when the program provides its own entry point (e.g. `_start` that calls `main`).
