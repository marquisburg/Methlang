# Types

MethASM is statically typed. Every variable and function parameter has an explicit type. This document describes the type system.

## Type Sizes and Alignment

The following sizes and alignments apply on x86-64. Use these when laying out structs for C interop or manual memory management.

| Type | Size | Alignment |
|------|------|-----------|
| `int8`, `uint8` | 1 | 1 |
| `int16`, `uint16` | 2 | 2 |
| `int32`, `uint32`, `float32` | 4 | 4 |
| `int64`, `uint64`, `float64`, pointers, enums | 8 | 8 |
| `string` | 16 | 8 |

Struct and array sizes are derived from their fields and element types. Pointers and enums are 8 bytes.

## Primitive Types

Signed integers: `int8`, `int16`, `int32`, `int64` (1, 2, 4, 8 bytes). Unsigned integers: `uint8`, `uint16`, `uint32`, `uint64` (1, 2, 4, 8 bytes). Floating-point: `float32`, `float64` (4, 8 bytes, IEEE 754). Sizes and representations follow the target platform (x86-64).

**Integer overflow:** The compiler emits native x86-64 arithmetic instructions. Signed integer overflow wraps (two's complement); there is no trap or runtime check. Unsigned overflow wraps modulo 2^n. Assembly programmers can rely on wrap-around behavior.

**Integer literal default type:** When the context does not disambiguate, integer literals default to `int32`. Floating-point literals default to `float64`. Examples: `42` has type `int32`; `3.14` has type `float64`. In expressions like `var x: int64 = 42`, the literal is implicitly converted to the expected type.

## Pointer Types

Pointers use the `*` suffix. A pointer holds the address of a value of the base type. Multi-level pointers are supported.

```masm
var p: int32*;        // pointer to int32
var pp: int32**;      // pointer to pointer to int32
var sp: MyStruct*;    // pointer to struct
```

The null pointer is written `0`. Pointers and `0` are comparable for equality.

**Pointer arithmetic:** The expression `ptr + n` is not supported. Use indexing instead: `ptr[i]` computes the address of the i-th element and advances by the size of the pointed-to type (C semantics). For `int32* p`, the expression `p[1]` accesses the next int32 (4 bytes). For byte-level access, use `uint8*` or `cstring`; then `ptr[i]` advances by 1 byte.

**Null dereference:** Dereferencing a null pointer (`*p` when `p` is 0) produces undefined behavior. On typical systems it results in a crash (access violation). The compiler does not insert null checks.

## Function Pointer Types

Function pointers are first-class values that can be stored, passed as arguments, and called indirectly. They enable callbacks and function references.

### Function Pointer Type Syntax

Function pointer types use the `fn` keyword with parameter types and return type:

```masm
var fp: fn(int32, int32) -> int32;  // pointer to function taking (int32, int32) returning int32
var void_fn: fn() -> void;           // pointer to function taking nothing returning nothing
```

### Taking Function Addresses

Use the address-of operator `&` to create a function pointer:

```masm
function add(a: int32, b: int32) -> int32 {
  return a + b;
}

var fp: fn(int32, int32) -> int32;
fp = &add;  // & takes the address of a function
```

### Calling Through Function Pointers

Call a function pointer like a regular function:

```masm
var result: int32 = fp(3, 4);  // calls the function pointed to by fp
```

### Function Pointer Use Cases

Function pointers are useful for callbacks, strategy patterns, and C interop:

```masm
// Callback pattern
function apply(op: fn(int32, int32) -> int32, a: int32, b: int32) -> int32 {
  return op(a, b);
}

function main() -> int32 {
  return apply(&add, 5, 3);  // passes add as callback
}
```

**Type equality:** Two function pointer types are equal if they have the same parameter types and return type. `fn(int32) -> int32` is compatible with `fn(int32) -> int32` but not with `fn(int32, int32) -> int32`.

## Array Types

Fixed-size arrays use `[N]` where N is a constant. Arrays are value types; the elements are laid out contiguously. Indexing is zero-based.

```masm
var arr: int32[10];
var buf: uint8[256];
```

**Out-of-bounds indexing:** The compiler rejects constant out-of-bounds indexes for fixed-size arrays (for example `arr[10]` on `int32[10]`). There is no runtime bounds checking, and non-constant indices are not proven safe at compile time. Indexing with an out-of-range value still produces undefined behavior.

**Use before initialization:** Local scalar and pointer variables must be assigned before first read. A use like `var x: int32; return x;` is a compile error.

**Passing arrays to functions:** Arrays are not passed by value (they can be large). Pass a pointer to the first element: `&arr[0]` or `&buf[0]`. The function parameter should have type `T*` (e.g. `int32*`, `uint8*`). Taking the address of an array with `&arr` yields a pointer to the whole array; for function calls expecting `T*`, use `&arr[0]`.

## Built-in Alias Types

`cstring` is an alias for `uint8*`. It is used for C interop: null-terminated strings, `void*`, and opaque pointers. `cstring` and `uint8*` are interchangeable. Use `cstring` when calling C functions that expect `char*` or `void*`.

`string` is a built-in struct with two fields: `.chars` (pointer to the character data) and `.length` (uint64, byte count). String literals have type `string`. The `string` type is distinct from `cstring`; use `s.chars` or the `cstr` helper from `std/io` to obtain a `cstring` for C calls.

**Creating strings at runtime:** There is no built-in constructor. To build a `string` from a `cstring` and length, assign the fields: `s.chars = ptr; s.length = len`. The `string` does not own the buffer; the caller is responsible for the lifetime of the data pointed to by `.chars`.

**String assignment:** Assigning one `string` to another copies the 16-byte struct (the `.chars` pointer and `.length`). Both values then refer to the same underlying buffer. No deep copy of the character data occurs. To share a buffer, assignment is sufficient; to copy contents, allocate a new buffer and copy bytes (e.g. via `malloc` and `memcpy` from `std/mem`).

## Struct Types

Structs group named fields. Fields are laid out in declaration order with appropriate alignment for the target. Structs can define methods (see [Declarations](declarations.md)).

```masm
struct Point {
  x: int32;
  y: int32;
}

struct SockAddrIn {
  sin_family: int16;
  sin_port: uint16;
  sin_addr: uint32;
  sin_zero: uint8[8];
}
```

For C interop, match the C struct layout exactly (field order, types, padding).

## Enum Types

Enums define a named type and a set of variants, each with an integer value. Variants without an explicit value continue from the previous variant (0 if first). Variant names are in scope after the enum is defined; use them directly (e.g. `North`, not `Direction.North`).

```masm
enum Direction {
  North,        // 0
  East = 2,     // 2
  South,        // 3 (previous + 1)
  West = -5     // -5
}

var a: Direction = North;
var b: Direction = East;
```

**Underlying type:** Enums use `int64` as the underlying representation. This affects struct layout and C interop: a struct field of enum type is 8 bytes, aligned to 8.

**Casting integers to enums:** Implicit narrowing allows assigning an integer to an enum variable when the types are compatible (e.g. `var d: Direction = 2`). For values read from C APIs or switch results, assign directly when the integer type narrows to the enum or use an explicit cast (e.g. `(Direction)val`) to force the conversion.

Enums can be compared with integers and used in `switch` cases. They can be exported for use in other modules (see [Declarations](declarations.md)).

## Generic Type Parameters

Functions and structs can be generic. Type parameters are declared in angle brackets: `function f<T>(...)` or `struct S<T> { ... }`. Instantiation uses the same syntax: `f<int32>(args)` or `var x: Pair<int32, float64>`.

```masm
struct Pair<A, B> {
  first: A;
  second: B;
}

function identity<T>(x: T) -> T {
  return x;
}

var p: Pair<int32, int32>;           // struct instantiation
var n: int32 = identity<int32>(42);  // function call with type args
```

The compiler performs **monomorphization** before type checking: each unique instantiation becomes a concrete type or function. There is no runtime generics; all type parameters are resolved at compile time.

**Constraints:** Type parameters are unconstrained. Operations inside generic bodies must be valid for all possible type arguments. For example, `a + b` in `function add<T>(a: T, b: T) -> T` requires that `T` supports `+`; the type checker validates this when the generic is instantiated.

## Type Conversions

Widening conversions (e.g. `int32` to `int64`, `float32` to `float64`) are implicit. Narrowing conversions (e.g. `int32` to `int16`, `int64` to `int8`) are allowed implicitly for integer-to-integer and float-to-float. There is no implicit conversion between integers and floats, or between pointers and integers, except that `0` is valid as a null pointer initializer and in pointer comparisons.

**Explicit casts:** MethASM provides an explicit cast syntax `(Type)expr`. This can be used to convert between numeric types, pointer types, and between integers and pointers. It is especially useful for pointer reinterpretation (e.g. treating `int32*` as `uint8*` for byte access) or converting floats to integers:

```masm
var p: int32*;
var bytes: uint8* = (uint8*)p;

var f: float64 = 3.14;
var i: int32 = (int32)f;
```

See [Expressions](expressions.md) for more details on cast conversions and evaluation behavior.
