
<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="mettle-syntax/icons/mettle-dark.svg">
    <source media="(prefers-color-scheme: light)" srcset="mettle.svg">
    <img src="mettle.svg" alt="Mettle" width="128" height="128">
  </picture>
</p>

<h1 align="center">Mettle</h1>

<p align="center">
  <b>A statically typed systems language that compiles straight to x86-64.</b>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/license-Apache--2.0-blue.svg" alt="License: Apache-2.0">
  <img src="https://img.shields.io/badge/target-x86--64-orange.svg" alt="Target: x86-64">
  <img src="https://img.shields.io/badge/platforms-Windows%20%7C%20Linux-success.svg" alt="Platforms: Windows | Linux">
  <img src="https://img.shields.io/badge/runtime-none-lightgrey.svg" alt="Runtime: none">
</p>

Mettle is a low-level language with a compiler it owns end to end: source, IR,
optimization, and x86-64 codegen with native object output. COFF plus a
built-in PE linker on Windows, and ELF with a self-contained `_start` on Linux.

No LLVM. No C backend. No managed runtime.

## Why It Exists

- Direct x86-64 output you can reason about
- Static types, pointers, structs, enums, generics, and function pointers
- `defer` and `errdefer` for simple, reliable cleanup
- C interop through `extern` and `cstring`, with no FFI runtime
- A bundled standard library for I/O, conversion, networking, processes, and threads
- Strong diagnostics with source snippets, stable error codes, and typo suggestions
- `--build` mode that produces native executables directly: COFF and the
  built-in PE linker on Windows, ELF and `ld` on Linux, with no NASM, gcc, or
  `link.exe` needed

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
./bin/mettle --build hello.mettle -o hello
./hello
```

The Linux `--build` path emits a native ELF object with the compiler's own
`_start` and links it with `ld` into a statically linked executable, with no
libc, CRT, or assembler. Programs that use the standard library (`std/io`,
`std/bench`) are not yet supported on Linux and fail at link time.

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
