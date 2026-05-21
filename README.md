

# Mettle

**A statically typed systems language that compiles straight to x86-64.**



Mettle is a low-level language with a compiler it owns end to end: source, IR,
optimization, x86-64 codegen, and on Windows, native COFF output plus a built-in
PE linker.

No LLVM. No C backend. No managed runtime.

## Why It Exists

- Direct x86-64 output you can reason about
- Static types, pointers, structs, enums, generics, and function pointers
- `defer` and `errdefer` for simple, reliable cleanup
- C interop through `extern` and `cstring`, with no FFI runtime
- A bundled standard library for I/O, conversion, networking, processes, and threads
- Strong diagnostics with source snippets, stable error codes, and typo suggestions
- Windows `--build` mode that needs no NASM, gcc, or `link.exe`

## Quick Start

Build the compiler:

```powershell
.\build.bat
```

Build and run a Mettle program on Windows:

```powershell
.\bin\mettle.exe --build hello.mettle -o hello.exe
.\hello.exe
```

Build and run on Linux:

```bash
make
./bin/mettle hello.mettle -o hello.s
nasm -f elf64 hello.s -o hello.o
gcc -nostartfiles hello.o -o hello
./hello
```

Production build:

```powershell
.\bin\mettle.exe --build --release hello.mettle -o hello.exe
```

## Help

```powershell
.\bin\mettle.exe help
.\bin\mettle.exe help build
.\bin\mettle.exe help runtime
.\bin\mettle.exe docs
```

## Documentation

- [Language reference](docs/LANGUAGE.md)
- [Runtime model](docs/runtime-model.md)
- [Compilation pipeline](docs/compilation.md)
- [Standard library](docs/standard-library.md)
- [Known limitations](docs/known-limitations.md)

## Repository

- `src/` - compiler, codegen, linker, diagnostics, and runtime helpers
- `stdlib/` - standard library modules
- `tests/` - compiler, linker, and runtime tests
- `docs/` - language and tooling documentation

## Development

```powershell
.\build.bat
.\tests\run_tests.ps1
.\tests\run_tests.ps1 -BuildCompiler
```

