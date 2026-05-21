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

**No LLVM. No C backend.**
Mettle is its own compiler end to end: source in, its own IR and x86-64
backend out. Nothing about how your code becomes instructions is hidden behind
someone else's optimizer.

**Performance.**
Compiles straight to x86-64 with no runtime, emitting NASM assembly or, on
Windows, a COFF object directly. Generics monomorphize at compile time, casts
and pointer access are explicit, and `--release` strips bounds and null checks.
You can predict the instructions your code becomes.

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
- C interop via `extern` and `cstring`
- Transitional heap allocator shim for `new` and heap-backed string concatenation (calloc-backed; no GC, no runtime tracing)
- Standard library modules for I/O, conversion, networking, process, threading (thin Win32/pthread wrappers), and more
- Developer-friendly diagnostics: stable error codes, source snippets with carets, and scope-aware "did you mean?" suggestions for typos
- Cross-platform symbolized crash tracebacks (Windows SEH and POSIX signal handlers)

## Quick Start (Windows)

1. Build the compiler:

```powershell
.\build.bat
```

1. Build an executable with the native Windows path:

```powershell
.\bin\mettle.exe --build hello.mettle -o hello.exe
.\hello.exe
```

This path does not require `NASM`, `gcc`, or `link.exe` for the target build.

No project-local `stdlib/` folder is required. The compiler auto-loads the stdlib bundled with the Mettle installation/build output. Use `--stdlib <dir>` only when you want to override that.

For production builds, use `--release`:

```powershell
.\bin\mettle.exe --build --release hello.mettle -o hello.exe
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
.\bin\mettle.exe help heap
.\bin\mettle.exe help build
.\bin\mettle.exe docs
```

Available topics: `build`, `heap`, `gc`, `interop`, `stdlib`, `web`.

## Runtime and Linking Notes

- `mettle --build` uses the bundled runtime objects plus Mettle's internal PE linker on Windows.
- `mettle --build --linker auto` tries the internal linker first and falls back to external linkers if needed.
- `mettle --build --emit-asm` selects the legacy NASM assembly path.
- If you use the manual assembly/link flow, link bundled `runtime/gc.o` from your Mettle installation when using `new` or string concatenation.
- If you use async features, also link bundled `runtime/async_runtime.o`.
- Compile with `-s` to embed runtime crash traceback support, or use `-d` to enable it alongside normal debug output.
- On Windows, embedded crash tracebacks report native exception codes such as `0xC0000005` and compiler-generated runtime traps with Mettle function/source frames.
- The internal PE linker resolves common Win32 DLLs directly; use `--link-arg` only for additional DLLs/import libraries.
- `gc_thread_attach` and `gc_thread_detach` remain compatibility no-ops for older worker-thread code.

Example runtime crash output:

```text
Unhandled runtime exception 0xC0000005 (access violation)
Exception address: 0x00007FF7DFD71046
write access violation at 0x0000000000000001
Stack trace:
  #0 leaf_crash at app.mettle:2:3 (0x00007FF7DFD71046)
  #1 main at app.mettle:8:3 (0x00007FF7DFD71080)
```

## How it compiles

"Straight to x86-64" means there is no third-party code generator in the path.
Mettle does not lower to LLVM IR, and it does not emit C and shell out to a C
compiler. The compiler owns every phase from source text to machine code:

1. **Lexing** - tokenize source
2. **Parsing** - build the AST
3. **Import resolution** - resolve and inline `import` directives
4. **Monomorphization** - expand generics into concrete instantiations
5. **Type checking** - semantic analysis and symbol resolution
6. **IR lowering** - convert the AST to Mettle's own intermediate representation
7. **Optimization** (optional, `-O`) - propagation, folding, branch and codegen peepholes
8. **Code generation** - emit x86-64

The final phase has two outputs. By default it writes **NASM assembly**, which
NASM then assembles. On Windows, `--emit-obj` skips the assembler entirely and
writes a **Win64 COFF object** directly from the same codegen; `--build
--linker internal` then links it with the built-in PE linker, so a complete
executable needs no NASM, gcc, or `link.exe` on the machine.

Because Mettle controls IR lowering and codegen, the things that affect
performance are visible decisions rather than a black box: monomorphization
removes generic dispatch before codegen, the optimizer's peepholes map known IR
shapes to specific instruction sequences, and `--release` lowers without the
generated null/bounds trap checks that normal builds emit.

See [docs/compilation.md](docs/compilation.md) and [docs/lexical-structure.md](docs/lexical-structure.md) for the full pipeline, token model, and diagnostics.

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
- Heap allocator runtime: [docs/heap-allocator-runtime.md](docs/heap-allocator-runtime.md)
- Standard library: [docs/standard-library.md](docs/standard-library.md)

## Quick Dev Commands

```powershell
.\build.bat
.\tests\run_tests.ps1
.\tests\run_tests.ps1 -BuildCompiler
.\web\build.bat
```
