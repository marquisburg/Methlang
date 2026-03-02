# Methlang

Methlang is a typed, low-level language that compiles `.meth` source files to x86-64 NASM assembly.

It is designed for systems-style control with stronger semantics than raw assembly: structured control flow, static type checking, modules, generics, and C interop.

## Highlights

- Compiles to x86-64 NASM assembly (`win64`, `elf64` object formats)
- Strong typing with pointers, arrays, structs, enums, and function pointers
- Control flow: `if`, `while`, `for`, `switch`, `defer`, `errdefer`
- C interop via `extern` and `cstring`
- Optional conservative GC runtime for `new` and GC-backed string concatenation
- Standard library modules for I/O, conversion, networking, process, threading, and more

## Hello World

```meth
import "std/io";

function main() -> int32 {
  println("Hello, Methlang!");
  return 0;
}
```

## Quick Start (Windows)

1. Build the compiler:

```powershell
.\build.bat
```

1. Compile source to assembly:

```powershell
.\bin\methlang.exe hello.meth -o hello.s
```

For production builds, use `--release`:

```powershell
.\bin\methlang.exe --release hello.meth -o hello.s
```

`--release` enables `-O`, strips assembly comments, removes unreachable functions, and lowers without generated runtime null/bounds trap checks.

1. Assemble and link:

```powershell
nasm -f win64 hello.s -o hello.o
gcc -nostartfiles hello.o src\runtime\gc.c -o hello.exe -lkernel32
.\hello.exe
```

Use `-nostartfiles` so Methlang's entry point (`mainCRTStartup`) is used instead of the C runtime entry.

## Quick Start (Linux)

```bash
make
./bin/methlang hello.meth -o hello.s
nasm -f elf64 hello.s -o hello.o
gcc -nostartfiles hello.o src/runtime/gc.c -o hello
./hello
```

## Toolchain

- Methlang compiler (`bin/methlang.exe` on Windows, `bin/methlang` on Linux)
- NASM assembler
- System C toolchain/linker (`gcc`/`clang`)

## Runtime and Linking Notes

- Link `src/runtime/gc.c` when using `new` or string concatenation.
- Networking examples may require extra libraries (for example `-lws2_32` on Windows).
- For GC use from worker threads, use `gc_thread_attach` and `gc_thread_detach`.

## Compiler Snapshot

Pipeline:

1. Lexing
2. Parsing
3. Import resolution
4. Monomorphization
5. Type checking
6. IR lowering
7. Optimization (optional)
8. Code generation

Current internal token model includes:

- Token `value` (null-terminated C string for parser/semantic compatibility)
- Token `lexeme` string view (`data + length`) for precise extents
- Global interning for identifier-like names used across lexer/AST/symbol/type metadata

See [docs/compilation.md](docs/compilation.md) and [docs/lexical-structure.md](docs/lexical-structure.md) for details.

## Repository Layout

- `src/` compiler source (lexer, parser, semantic analysis, IR, codegen, runtime support)
- `stdlib/` standard library modules and helper C shims
- `tests/` compiler/runtime test suite
- `web/` web server demo
- `docs/` language and tooling documentation

## Documentation

- Language reference: [docs/LANGUAGE.md](docs/LANGUAGE.md)
- Compilation: [docs/compilation.md](docs/compilation.md)
- Lexical structure: [docs/lexical-structure.md](docs/lexical-structure.md)
- Types: [docs/types.md](docs/types.md)
- Expressions: [docs/expressions.md](docs/expressions.md)
- Control flow: [docs/control-flow.md](docs/control-flow.md)
- C interop: [docs/c-interop.md](docs/c-interop.md)
- Garbage collector: [docs/garbage-collector.md](docs/garbage-collector.md)
- Standard library: [docs/standard-library.md](docs/standard-library.md)

## Quick Dev Commands

```powershell
.\build.bat
.\tests\run_tests.ps1
.\tests\run_tests.ps1 -BuildCompiler
```
