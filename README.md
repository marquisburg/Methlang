# MethASM

MethASM is a compiler for a typed assembly-like language that targets x86-64 assembly.

[Language Reference](docs/LANGUAGE.md): Index and links to lexical structure, types, declarations, expressions, control flow, modules, standard library, C interop, compilation, and quick reference.

## Project Status

MethASM provides an end-to-end compilation pipeline from `.masm` source to x86-64 assembly output.

- Lexing
- Parsing
- Semantic and type analysis
- Code generation
- Runtime garbage collector integration

Core compiler and runtime paths are implemented and currently validated by project tests.

Current implementation status:

- Core language front-end and backend pipeline are operational.
- Fail-fast diagnostics are enforced across lexer, parser, semantic analysis, and codegen.
- Arrays, pointers, structured types, and major control-flow constructs are implemented.
- Runtime GC integration is compiled and exercised by runtime tests.
- String concatenation is implemented (the compiler now accepts `string + string` and emits GC-backed code), and the Windows web server example exercises that path together with `std/net`.
- Automated compiler/runtime regression suite is available under `tests/run_tests.ps1`.

## Compiler Guarantees

The compiler uses fail-fast behavior across all major phases.

- Lexical errors report precise line and column and stop compilation.
- Parser recovery is used for diagnostics, but any parser error still causes final failure.
- Semantic errors include source locations and block backend execution.
- Code generation rejects unresolved symbols and unsupported constructs instead of emitting fallback output.
- Unsupported top-level constructs are rejected explicitly.

This reduces silent failures and minimizes incorrect cascading diagnostics.

## Implemented Language Features

- Variable declarations with explicit types and initializer-based inference
- Pointer type annotations (e.g. `int32*`, `Pair*`, `int32**`)
- Dereference (`*p`) and address-of (`&x`) operators
- Pointer-to-struct arrow notation (`p->field`)
- Fixed-size array type annotations (for example `int32[10]`)
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
- Inline assembly blocks

## Type and Semantic Analysis

- Built-in integer and floating-point types
- Pointer type resolution (multi-level `T*`, `T**`, etc.) and base-type inference
- Null pointer constant handling (`0` as valid pointer initializer and in comparisons)
- Fixed-size array type resolution and element type inference
- Struct type registration and lookup
- Method call validation
- Pointer type compatibility (same base type, or pointer vs null)
- Assignment compatibility validation
- Field assignment validation
- Array index expression validation (index type and target type checks)
- Loop/switch context checks for `break` and `continue`
- Switch expression validation and compile-time case constant evaluation
- Duplicate `case` detection and default-clause uniqueness checks
- Undefined symbol detection with source locations
- Forward declaration signature compatibility checks in symbol resolution
- Extern/non-extern redeclaration and link-name consistency checks

## Code Generation

- x86-64 assembly emission (Intel/NASM-style syntax)
- Function prologue and epilogue generation
- Pure IR-first function body emission (IR -> assembly)
- Struct field offset-based access and assignment
- Method call emission (mangled names, `this` as first parameter)
- Pointer dereference and address-of code generation
- Array and pointer element address calculation and typed indexed load/store emission
- Code generation for `if`, `while`, `for`, and `switch` control flow
- Nested control-flow label management for `break` and `continue`
- `_start` entry emission that calls `main` when present
- `extern <symbol>` emission for foreign function/global references
- Hard failure on unresolved symbols or unsupported generation paths

## C Interop (v1)

MethASM supports call-into-C declarations for external functions and globals.

Syntax:

```masm
extern function puts(msg: cstring) -> int32 = "puts";
extern var errno_value: int32 = "errno";
```

Notes:

- `= "symbol"` is optional; when omitted, the MethASM declaration name is used.
- `cstring` is a built-in alias for `uint8*`.
- `extern` declarations are top-level only.
- Link-name suffixes (`= "..."`) are valid only on `extern` declarations.
- `extern var` requires an explicit type and cannot have an initializer.
- `import`/`export` behavior is unchanged in this milestone.
- ABI follows the active target platform convention:
  - Windows: Microsoft x64 ABI
  - Linux/macOS: System V AMD64 ABI

## Intermediate Representation (IR)

MethASM now includes a real compiler IR stage between semantic analysis and assembly emission.

What it is:

- Function-level linear IR (`IRProgram` -> `IRFunction` -> `IRInstruction`)
- Explicit control-flow instructions (`label`, `jump`, conditional branches)
- Explicit data-flow operands (temps, symbols, literals, labels)
- Lowered from AST after type checking
- Consumed by codegen function-by-function for backend emission
- Dumpable to `<output>.ir` in debug/optimize mode (`-d` or `-O`)

Current backend coverage:

- Control flow (`if`/`while`/`for`/`switch`/`break`/`continue`) is emitted from explicit IR control-flow instructions.
- Local declarations, assignment, branches, labels, and returns emit directly from IR.
- IR now models lvalue address and memory operations explicitly (`addr_of`, `load`, `store`) for struct fields, pointer dereference, and indexed access.
- Heap allocation is modeled as explicit IR (`new`) instead of AST-side expression fallback.
- Integer binary/unary operations lower to pure IR.
- Type-aware lowering supports specific floating-point calculations with XMM registers.
- Function calls and method calls are emitted directly from pure IR call instructions.

What it is not:

- Not SSA form (temps are mutable storage slots, no phi nodes)
- Not a machine IR or register-allocated IR
- Not currently used to represent global declarations as first-class IR operations

In-Progress Features:
- Constant folding IR optimization pass runs if `-O` flag is enabled.

Practical interpretation:

- The main backend path for function bodies is exclusively IR-driven.
- IR gives a stable seam for future optimization and backend retargeting. It fully isolates semantic type checking from assembly codegen.

## Garbage Collector Runtime

- Heap allocation via `gc_alloc`
- Conservative mark-and-sweep collection
- Iterative mark traversal using a worklist
- Stack root scanning
- Root registration API: `gc_register_root`, `gc_unregister_root` (pointer globals auto-registered)
- Collection controls: `gc_collect`, `gc_collect_now`, `gc_set_collection_threshold`, `gc_get_collection_threshold`
- Runtime cleanup: `gc_shutdown`

## Build

### Windows

```powershell
.\build.bat
```

### Linux and macOS

```bash
make
```

Build output:

- Windows: `bin\methasm.exe`
- Linux and macOS: `bin/methasm`

## Usage

```bash
methasm [options] <input.masm>
```

Options:

- `-i <file>` input file
- `-o <file>` output file
- `-I <dir>` add an import search directory (repeatable)
- `--stdlib <dir>` set stdlib root directory (default: `stdlib`)
- `-d`, `--debug` enable debug mode
- `-O`, `--optimize` enable optimization flag
- `-h`, `--help` print usage

Import resolution behavior:

- Tries the importer file's directory first.
- If the import has no extension, `.masm` is tried automatically.
- `std/...` imports are resolved under the configured stdlib root.
- `-I` directories are searched after local imports.

Example:

```bash
./bin/methasm tests/test_gc_alloc.masm -o out.s
```

## Testing

Run runtime and compiler tests with:

```bash
make test
```

Automated Windows suite (compiler positives, compiler negatives, assembly syntax checks, GC runtime):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\run_tests.ps1
```

Windows convenience wrapper:

```powershell
tests\run_tests.bat
```

Suite options:

- `-BuildCompiler` rebuilds `bin\methasm.exe` before running tests
- `-SkipRuntime` skips the GC runtime executable test

Windows manual runtime test:

```powershell
gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE tests\gc_runtime_test.c src\runtime\gc.c -o bin\gc_runtime_test.exe
.\bin\gc_runtime_test.exe
```

Windows compiler smoke tests:

```powershell
.\bin\methasm.exe test_simple.masm -o out_simple.s
.\bin\methasm.exe tests\test_gc_alloc.masm -o out_gc.s
.\bin\methasm.exe tests\test_array_index.masm -o out_array.s
.\bin\methasm.exe tests\test_control_flow.masm -o out_control_flow.s
.\bin\methasm.exe tests\test_switch_const_expr.masm -o out_switch_const_expr.s
.\bin\methasm.exe tests\test_switch_continue_loop.masm -o out_switch_continue_loop.s
.\bin\methasm.exe tests\test_pointers.masm -o out_pointers.s
.\bin\methasm.exe tests\test_pointer_null.masm -o out_pointer_null.s
.\bin\methasm.exe tests\test_forward_decl_pointer.masm -o out_forward_decl_pointer.s
.\bin\methasm.exe tests\test_pointer_param_address.masm -o out_pointer_param_address.s
```

## Repository Layout

```text
src/
  lexer/      Tokenization
  parser/     Parsing and AST creation
  semantic/   Type checking, symbol table, register allocation
  ir/         AST-to-IR lowering and IR data model
  codegen/    Assembly generation
  runtime/    Garbage collector runtime
  debug/      Debug information helpers
  error/      Error reporting
  main.c      CLI entry and compilation flow

tests/        Language and runtime tests
  run_tests.ps1   Automated compiler/runtime test suite
  run_tests.bat   Windows wrapper for run_tests.ps1
build.bat     Windows build
Makefile      Linux/macOS build and test targets
```

## Known Limitations

- Optimization passes are limited.
- Language surface area is still evolving.
- Some advanced language and backend scenarios are not implemented.
- `switch` case labels currently require compile-time integer constant expressions and do not yet support range-style cases.

## Contributing

Contributions should include:

- A clear problem statement
- Tests that validate behavior changes
- Notes on compatibility impact
