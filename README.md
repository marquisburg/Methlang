# MethASM

MethASM is a compiler for a typed assembly-like language that targets x86-64 assembly.

[Language Reference](docs/LANGUAGE.md): Index and links to lexical structure, types, declarations, expressions, control flow, modules, standard library, C interop, compilation, and quick reference.

## Project Status

MethASM provides an end-to-end compilation pipeline from `.masm` source to x86-64 assembly output.

- Lexing
- Parsing
- Import resolution
- Monomorphization (generic expansion)
- Semantic and type analysis
- IR lowering and optimization
- Code generation
- Runtime garbage collector integration

Core compiler and runtime paths are implemented and currently validated by project tests.

Current implementation status:

- Core language front-end and backend pipeline are operational.
- Generic functions and structs with compile-time monomorphization.
- Fail-fast diagnostics are enforced across lexer, parser, semantic analysis, and codegen.
- Arrays, pointers, structured types, and major control-flow constructs are implemented.
- `defer` and `errdefer` are implemented in the IR pipeline, including assignments and block statements.
- Runtime GC integration is compiled and exercised by runtime tests.
- String concatenation is implemented (the compiler now accepts `string + string` and emits GC-backed code), and the Windows web server example exercises that path together with `std/net`.
- Dynamic null-dereference and fixed-array bounds traps are emitted by the compiler, and obvious GC-managed pointer escapes to C produce warnings.
- Automated compiler/runtime regression suite is available under `tests/run_tests.ps1`.

## Compiler Guarantees

The compiler uses fail-fast behavior across all major phases.

- Lexical errors report precise line and column and stop compilation.
- Parser recovery is used for diagnostics, but any parser error still causes final failure.
- Semantic errors include source locations and block backend execution.
- Code generation rejects unresolved symbols and unsupported constructs instead of emitting fallback output.
- Unsupported top-level constructs are rejected explicitly.
- **Runtime safety:** Dynamic null dereference and pointer indexing emit traps; fixed-array indexing emits bounds traps. Managed struct pointers passed to `extern function` or stored in `extern` variables produce GC escape warnings.

This reduces silent failures and minimizes incorrect cascading diagnostics.

## Implemented Language Features

- Variable declarations with explicit types and initializer-based inference
- Pointer type annotations (e.g. `int32*`, `Pair*`, `int32**`)
- Dereference (`*p`) and address-of (`&x`) operators
- Pointer-to-struct arrow notation (`p->field`)
- Fixed-size array type annotations (for example `int32[10]`)
- Bitwise operators (`&`, `|`, `^`, `~`, `<<`, `>>`)
- Generic functions and structs (`function f<T>(...)`, `struct S<T> { ... }`)
- Enums with named variants and explicit values
- `string` type with `.chars` and `.length`; string literals
- String concatenation via `+` (GC-backed)
- String literal implicit coercion to `cstring` for C calls
- Module system with `import` and `export`
- `import_str` compile-time file embedding
- Functions and function calls
- Function forward declarations (e.g. `function add(a: int32, b: int32) -> int32;`)
- Function return type syntax with both `->` and `:`
- External C function declarations with optional link names
- External C global declarations with optional link names
- Struct declarations
- Struct member access and assignment
- Struct methods and method calls (`obj.method(args)`)
- Array and pointer indexing (`arr[i]`, `ptr[i]`) and indexed assignment
- `cstring` alias type (`uint8*`)
- `if` and `else`
- `while`
- `for`
- `switch`, `case`, `default`
- `break` and `continue`
- `return`
- `defer` and `errdefer`
- Inline assembly blocks

## Type and Semantic Analysis

- Built-in integer and floating-point types
- Bitwise operator type checking (integer operands for `&`, `|`, `^`, `~`, `<<`, `>>`)
- Pointer type resolution (multi-level `T*`, `T**`, etc.) and base-type inference
- Null pointer constant handling (`0` as valid pointer initializer and in comparisons)
- Fixed-size array type resolution and element type inference
- Struct type registration and lookup
- Method call validation
- Pointer type compatibility (same base type, or pointer vs null)
- Assignment compatibility validation
- Field assignment validation
- Array index expression validation (index type and target type checks)
- Compile-time rejection of constant null dereference (`*0`)
- Compile-time rejection of constant fixed-array out-of-bounds indices
- Loop/switch context checks for `break` and `continue`
- Switch expression validation and compile-time case constant evaluation
- Duplicate `case` detection and default-clause uniqueness checks
- Undefined symbol detection with source locations
- Forward declaration signature compatibility checks in symbol resolution
- Extern/non-extern redeclaration and link-name consistency checks
- Warning on managed struct pointers passed to `extern function` or stored in `extern` variables

## Code Generation

- x86-64 assembly emission (Intel/NASM-style syntax)
- Function prologue and epilogue generation
- Pure IR-first function body emission (IR -> assembly)
- Struct field offset-based access and assignment
- Method call emission (mangled names, `this` as first parameter)
- Pointer dereference and address-of code generation
- Array and pointer element address calculation and typed indexed load/store emission
- Runtime null checks for dynamic dereference and pointer-based indexing
- Runtime bounds checks for dynamic indexing into fixed-size arrays
- Code generation for `if`, `while`, `for`, and `switch` control flow
- Nested control-flow label management for `break` and `continue`
- `_start` entry emission that calls `main` when present
- `extern <symbol>` emission for foreign function/global references
- Hard failure on unresolved symbols or unsupported generation paths
- IR lowering: control flow, assignments, and operations lowered to IR before assembly; `-d` or `-O` dumps IR to `<output>.ir`. See [Compilation](docs/compilation.md).

## Generic Type Parameters

MethASM supports generic functions and structs with compile-time monomorphization. Type parameters are declared in angle brackets `<>` and instantiated at call sites or type declarations.

```masm
struct Pair<A, B> {
  first: A;
  second: B;
}

function swap<T>(a: T*, b: T*) -> void {
  var tmp: T = *a;
  *a = *b;
  *b = tmp;
}

function main() -> int32 {
  var p: Pair<int32, int32>;
  p.first = 10;
  p.second = 20;
  swap<int32>(&p.first, &p.second);
  return p.first + p.second;
}
```

The compiler monomorphizes generics before type checking: each unique instantiation becomes a concrete type or function. See [Declarations](docs/declarations.md#generic-functions) and [Types](docs/types.md#generic-type-parameters) for details.

## C Interop

MethASM supports call-into-C declarations for external functions and globals.

```masm
extern function puts(msg: cstring) -> int32 = "puts";
extern var errno_value: int32 = "errno";
```

- `= "symbol"` is optional; when omitted, the MethASM name is used.
- `cstring` is a built-in alias for `uint8*`.
- ABI follows the platform convention (Microsoft x64 on Windows, System V AMD64 on Linux/macOS).

See [C Interoperability](docs/c-interop.md) for details.

## Garbage Collector

MethASM provides an optional conservative mark-and-sweep garbage collector. Programs that use `new` or string concatenation must link `gc.c`. See [Garbage Collector](docs/garbage-collector.md) for full documentation.

## Known Limitations

See [Known Limitations](docs/known-limitations.md) for the full list. Current highlights:

- Optimization passes are limited.
- Pointer indexing is null-checked but not bounds-checked.
- Deferred statements capture variables by reference, not by value.
- `errdefer` is convention-based: `0` means success.
- No labeled `break` or `continue`.
