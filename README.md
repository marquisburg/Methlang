<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="mettle-syntax/icons/mettle-dark.svg">
    <source media="(prefers-color-scheme: light)" srcset="mettle.svg">
    <img src="mettle.svg" alt="Mettle" width="128" height="128">
  </picture>
</p>

<h1 align="center">Mettle</h1>

<p align="center">
  <b>Static types. Native x86-64. A compiler you can read.</b>
</p>

<p align="center">
  <a href="https://github.com/The-Mettle-Project/Mettle">GitHub</a> |
  <a href="https://github.com/The-Mettle-Project/Mettle/releases">Releases</a> |
  <a href="docs/LANGUAGE.md">Language reference</a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/license-Apache--2.0-blue.svg" alt="License: Apache-2.0">
  <img src="https://img.shields.io/badge/target-x86--64-orange.svg" alt="Target: x86-64">
  <img src="https://img.shields.io/badge/platforms-Windows%20%7C%20Linux-success.svg" alt="Platforms: Windows | Linux">
  <img src="https://img.shields.io/badge/runtime-none-lightgrey.svg" alt="Runtime: none">
</p>

Mettle is a systems language with a compiler that owns the full path from source to native x86-64. On Windows, `mettle --build` produces a PE executable with a built-in linker. On Linux, it produces ELF and links with the system toolchain. No LLVM, no VM, and no managed runtime.

## At a glance

- Low-level programs with static types, pointers, structs, and enums
- One command from `.mettle` to a native binary on Windows or Linux
- Call C and OS APIs directly; bundled stdlib for I/O, memory, math, and more
- `defer` / `errdefer` for scope cleanup; clear errors with source snippets
- Optional Tracy profiling, runtime timing, and debug stack traces

Windows is the most polished target (internal PE linker, Win32 GUI via `std/ui`). Linux is supported for builds, libc-backed stdlib, and compiler development. Details and caveats: [known limitations](docs/known-limitations.md).

## Hello, native

Save as `hello.mettle`:

```mettle
import "std/io";

function fib(n: int32) -> int64 {
  if (n <= 1) { return n; }
  var a: int64 = 0;
  var b: int64 = 1;
  var i: int32 = 2;
  while (i <= n) {
    var next = a + b;
    a = b;
    b = next;
    i = i + 1;
  }
  return b;
}

function main() -> int32 {
  print("fib(10) = ");
  print_int(fib(10));
  newline();
  return 0;
}
```

```bash
mettle --build hello.mettle -o hello
./hello          # Windows: .\hello.exe
```

Pass `--prelude` to pull in common stdlib modules without explicit imports. Import networking yourself when you need it (`std/net` on Windows; `std/net` or `std/net_posix` on Linux).

## Install

**Linux (x86-64)**

```bash
curl -fsSL https://raw.githubusercontent.com/The-Mettle-Project/Mettle/main/install.sh | sh
```

**Windows (x86-64), PowerShell**

```powershell
irm https://raw.githubusercontent.com/The-Mettle-Project/Mettle/main/install.ps1 | iex
```

Installs to `~/.mettle` (Linux) or `%LOCALAPPDATA%\Mettle` (Windows), updates user PATH, and checks for a C toolchain when linking stdlib programs. No root or admin required. Pin a release: `--version v0.3.0` (Linux) or `-Version v0.3.0` (Windows).

```bash
mettle --version
```

Dev builds from source report `v0.9.0-dev` unless `METTLE_VERSION_RAW` is set at compile time.

## Build from source

**Windows** (gcc or clang):

```powershell
.\build.bat          # default: gcc
.\build.bat clang
```

**Linux / macOS** (build the compiler on the host; codegen targets x86-64 Windows and Linux):

```bash
make                 # bin/mettle + bundled stdlib/ and runtime/
make install         # optional: /usr/local/bin, stdlib, runtime
```

Typical release build:

```powershell
.\bin\mettle.exe --build --release hello.mettle -o hello.exe
```

```bash
./bin/mettle --build --release hello.mettle -o hello
```

Useful flags: `--build` (executable), `--release` / `-O` (optimized), `--emit-asm` (legacy NASM path on Windows), `-d` / `-s` / `-g` (debug and stack traces), `--profile-runtime`, `--tracy`. Full list: `mettle --help` and `mettle help build`.

## Documentation

- [Language reference](docs/LANGUAGE.md)
- [Compilation](docs/compilation.md) (CLI, link pipelines, Tracy, profiling)
- [Imports](docs/imports.md)
- [Runtime model](docs/runtime-model.md)
- [Standard library](docs/standard-library.md)
- [C interop](docs/c-interop.md)
- [Known limitations](docs/known-limitations.md)

`mettle docs` prints paths to these files next to the compiler binary.

## Repository layout

```
src/            compiler (lexer through codegen, linker, diagnostics)
stdlib/         standard library
src/runtime/    optional helper objects (crash traces, atomics, ...)
tests/          regression tests; run_tests.ps1 on Windows
examples/       benchmarks and demos
tools/          ELF tests, benchmarks, fuzz scripts
mettle-syntax/  VS Code / Cursor extension
docs/           language and tooling reference
```

## Examples and benchmarks

Runnable samples live under [`examples/`](examples/). Benchmark suites pair Mettle, C, and Rust; run them with:

```powershell
.\tools\benchmark\run-benchmarks.ps1
```

See [`examples/README.md`](examples/README.md) for `fib`, `grep`, `ui_demo`, `tracy_demo`, and others.

## Development

**Windows** (primary CI: full test suite):

```powershell
.\build.bat
.\tests\run_tests.ps1
.\tests\run_tests.ps1 -BuildCompiler
```

**Linux** (native ELF backend):

```bash
make -j"$(nproc)"
bash tools/test-elf-native.sh
```

Optional: `tools/fuzz/` (nightly workflow). Crash-handler unit test: `make test`.

## Editor support

Install the [`mettle-syntax`](mettle-syntax/) extension for `.mettle` syntax highlighting, snippets, and compiler-backed diagnostics in VS Code or Cursor.

## License

Apache-2.0. See [LICENSE](LICENSE).
