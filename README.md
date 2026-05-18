<p align="center">
  <img src="mettle.svg#gh-light-mode-only" alt="Mettle" width="120" height="120">
  <img src="mettle-syntax/icons/mettle-dark.svg#gh-dark-mode-only" alt="Mettle" width="120" height="120">
</p>

<h1 align="center">The Mettle Programming Language</h1>

<p align="center">
  <i>A statically typed, low-level language that compiles straight to x86-64.</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/license-Apache--2.0-blue.svg" alt="License: Apache-2.0">
  <img src="https://img.shields.io/badge/target-x86--64-orange.svg" alt="Target: x86-64">
  <img src="https://img.shields.io/badge/platforms-Windows%20%7C%20Linux-success.svg" alt="Platforms: Windows | Linux">
  <img src="https://img.shields.io/badge/runtime-none-lightgrey.svg" alt="Runtime: none">
</p>

<p align="center">
  <b><a href="docs/LANGUAGE.md">Docs</a></b>
  &nbsp;&nbsp;•&nbsp;&nbsp;
  <b><a href="#built-in-help">Help</a></b>
  &nbsp;&nbsp;•&nbsp;&nbsp;
  <a href="#quick-start-windows">Getting started</a>
  &nbsp;&nbsp;•&nbsp;&nbsp;
  <a href="docs/quick-reference.md">Examples</a>
  &nbsp;&nbsp;•&nbsp;&nbsp;
  <a href="#why-mettle">Why Mettle</a>
</p>

<hr>

This is the main source code repository for Mettle. It contains the compiler, standard library, runtime, and documentation.

> [!TIP]
> **New to Mettle?** Start with the **[Language Reference](docs/LANGUAGE.md)** for a guided tour, or run `mettle help` for built-in CLI docs (`build`, `gc`, `interop`, `stdlib`, `web`).

## Why Mettle?

**Performance.**
Mettle compiles straight to x86-64 with no runtime between your code and the
machine. Generics monomorphize at compile time, casts and pointer access are
explicit, and `--release` strips bounds and null checks. You can predict the
instructions your code becomes.

**Control.**
A low-level language without the busywork: static types, generics, structured
control flow, and `defer` / `errdefer` for scope-bound cleanup, so resource
handling stays correct without `goto cleanup` ladders. `extern` calls into
existing C libraries with no binding layer.

**Self-contained.**
On Windows, `mettle --build` emits native COFF objects and links them with a
built-in PE linker. No NASM, no gcc, no `link.exe` on the machine: one binary
in, one executable out.

## What's in the box

- Compiles to x86-64 NASM assembly and Windows COFF objects
- Strong typing with pointers, arrays, structs, enums, and function pointers
- Control flow: `if`, `while`, `for`, `switch`, `match`, `defer`, `errdefer`, and labeled `break`/`continue`
- Compound assignment (`+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`); line (`//`) and nesting block (`/* */`) comments
- Async execution with `async`, `await`, `Future<T>`, and cooperative cancellation (default **pool** executor; optional experimental **`--async-model coroutine`** with a portable reactor: IOCP on Windows, `poll(2)` on POSIX; see `docs/async.md`)
- C interop via `extern` and `cstring`
- Optional conservative GC runtime for `new` and GC-backed string concatenation
- Standard library modules for I/O, conversion, networking, process, threading, and more
- Developer-friendly diagnostics: stable error codes, source snippets with carets, and scope-aware "did you mean?" suggestions for typos
- Cross-platform symbolized crash tracebacks (Windows SEH and POSIX signal handlers)

## Quick Start (Windows)

1. Build the compiler:

```powershell
.\build.bat
```

1. Build an executable with the native Windows path:

```powershell
.\bin\mettle.exe --build --emit-obj --linker internal hello.mettle -o hello.exe
.\hello.exe
```

This path does not require `NASM`, `gcc`, or `link.exe` for the target build. Plain `--build` still defaults to the assembly-based auto path unless you also pass `--emit-obj`.

No project-local `stdlib/` folder is required. The compiler auto-loads the stdlib bundled with the Mettle installation/build output. Use `--stdlib <dir>` only when you want to override that.

For production builds, use `--release`:

```powershell
.\bin\mettle.exe --build --emit-obj --linker internal --release hello.mettle -o hello.exe
```

`--release` enables `-O`, strips assembly comments, removes unreachable functions, and lowers without generated runtime null/bounds trap checks.

1. Optional: emit assembly only:

```powershell
.\bin\mettle.exe hello.mettle -o hello.s
nasm -f win64 hello.s -o hello.o
gcc -nostartfiles hello.o "$env:ProgramFiles\Mettle\runtime\gc.o" -o hello.exe -lkernel32
.\hello.exe
```

Use `-nostartfiles` so Mettle's entry point (`mainCRTStartup`) is used instead of the C runtime entry. Manual assembly/linking is mainly for advanced cases; the default workflow is `mettle --build`.

## Quick Start (Linux)

```bash
make
./bin/mettle hello.mettle -o hello.s
nasm -f elf64 hello.s -o hello.o
gcc -nostartfiles hello.o /usr/local/runtime/gc.o -o hello
./hello
```

## Toolchain

- Mettle compiler (`bin/mettle.exe` on Windows, `bin/mettle` on Linux)
- NASM assembler for assembly-based builds
- System C toolchain/linker (`gcc`/`clang`) for external-link fallback and manual assembly/object flows

## Built-In Help

Use the CLI help/docs commands to jump to the right topic quickly:

```powershell
.\bin\mettle.exe help
.\bin\mettle.exe help gc
.\bin\mettle.exe help build
.\bin\mettle.exe docs
```

Available topics: `build`, `gc`, `interop`, `stdlib`, `web`.

## Runtime and Linking Notes

- `mettle --build --emit-obj --linker internal` uses the bundled runtime objects plus Mettle's internal PE linker on Windows.
- `mettle --build` in `auto` mode tries the internal linker first and falls back to external linkers if needed.
- If you use the manual assembly/link flow, link bundled `runtime/gc.o` from your Mettle installation when using `new` or string concatenation.
- If you use async features, also link bundled `runtime/async_runtime.o`.
- Compile with `-s` to embed runtime crash traceback support, or use `-d` to enable it alongside normal debug output.
- On Windows, embedded crash tracebacks report native exception codes such as `0xC0000005` and compiler-generated runtime traps with Meth function/source frames.
- The internal PE linker resolves common Win32 DLLs directly; use `--link-arg` only for additional DLLs/import libraries.
- For GC use from worker threads, use `gc_thread_attach` and `gc_thread_detach`.

Example runtime crash output:

```text
Unhandled runtime exception 0xC0000005 (access violation)
Exception address: 0x00007FF7DFD71046
write access violation at 0x0000000000000001
Stack trace:
  #0 leaf_crash at app.mettle:2:3 (0x00007FF7DFD71046)
  #1 main at app.mettle:8:3 (0x00007FF7DFD71080)
```

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
- Async and sync execution: [docs/async.md](docs/async.md)
- C interop: [docs/c-interop.md](docs/c-interop.md)
- Garbage collector: [docs/garbage-collector.md](docs/garbage-collector.md)
- Standard library: [docs/standard-library.md](docs/standard-library.md)

## Quick Dev Commands

```powershell
.\build.bat
.\tests\run_tests.ps1
.\tests\run_tests.ps1 -BuildCompiler
.\web\build.bat
```
