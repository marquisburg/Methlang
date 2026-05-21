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

This is the main source code repository for Mettle. It contains the compiler, standard library, tests, and documentation.

> [!TIP]
> **New to Mettle?** Start with the **[Language Reference](docs/LANGUAGE.md)** for a guided tour, or run `mettle help` for built-in CLI docs (`build`, `runtime`, `interop`, `stdlib`, `web`).

## Why Mettle?

**No LLVM. No C backend.**
Mettle is its own compiler end to end: source in, its own IR and x86-64
backend out. Nothing about how your code becomes instructions is hidden behind
someone else's optimizer.

**No runtime.**
No garbage collector, no async scheduler, no heap manager, no managed thread
pool, no startup support code that every program is forced to link. A normal
Mettle program links libc and nothing else; `new` and string concatenation
emit a direct `calloc(1, n)` call. See the [Runtime Model](docs/runtime-model.md)
for the full picture.

**Performance.**
Compiles straight to x86-64, emitting NASM assembly or, on Windows, a COFF
object directly. Generics monomorphize at compile time, casts and pointer
access are explicit, and `--release` strips bounds and null checks. You can
predict the instructions your code becomes.

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

- Compiles to x86-64 NASM assembly or directly to Windows COFF objects
- Strong typing with pointers, arrays, structs, enums, function pointers
- Control flow: `if`, `while`, `for`, `switch`, `match`, `defer`, `errdefer`, labeled `break`/`continue`
- Compound assignment (`+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`); line (`//`) and nesting block (`/* */`) comments
- C interop via `extern` and `cstring` — no binding layer, no FFI runtime
- Standard library modules for I/O, conversion, networking, process, threading (thin Win32/pthread wrappers), and more
- Developer-friendly diagnostics: stable error codes, source snippets with carets, scope-aware "did you mean?" suggestions for typos
- Cross-platform symbolized crash tracebacks (Windows SEH + POSIX signal handlers), opt-in via `-d` / `-s`
- Built-in PE linker on Windows; emits executables with no external linker required

## Runtime Model (short version)

The "Runtime: none" badge means there is no language runtime in the traditional sense. Every Mettle program links libc and nothing else, unless it opts into one of two narrowly-scoped helper objects:

| Helper | Linked when… | What it does |
|---|---|---|
| `crash_handler.o` | Built with `-d`, `-s`, `-g`, or in non-release mode with IR null/bounds traps active | Installs SEH/sigaction handler; provides `mettle_crash_trap` for compiler-generated checks; prints symbolized backtraces |
| `atomics.o` | Imports `std/thread` interlocked atomics | Wraps Win32 `Interlocked*` / GCC `__sync_*` intrinsics as callable symbols |

Both ship with the compiler under `bin/runtime/` (or the installer's `runtime/` directory). `mettle --build` links them on demand by scanning emitted objects for `mettle_crash_*` / `mettle_atomic_*` symbol references — you don't pass them on the command line.

For the full story (what gets emitted at each call boundary, the manual-link flow, why there are two files instead of one), see [docs/runtime-model.md](docs/runtime-model.md).

## Quick Start (Windows)

1. Build the compiler:

```powershell
.\build.bat
```

2. Build an executable with the native Windows path:

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

`--release` enables `-O`, strips assembly comments, removes unreachable functions, and lowers without generated null/bounds trap checks. Programs built with `--release` link no Mettle helper objects.

3. Optional: emit assembly only:

```powershell
.\bin\mettle.exe hello.mettle -o hello.s
nasm -f win64 hello.s -o hello.o
gcc -nostartfiles hello.o -o hello.exe -lkernel32
.\hello.exe
```

Use `-nostartfiles` so Mettle's entry point (`mainCRTStartup`) is used instead of the C runtime entry. Manual assembly/linking is mainly for advanced cases; the default workflow is `mettle --build`.

## Quick Start (Linux)

```bash
make
./bin/mettle hello.mettle -o hello.s
nasm -f elf64 hello.s -o hello.o
gcc -nostartfiles hello.o -o hello
./hello
```

## Toolchain

- Mettle compiler (`bin/mettle.exe` on Windows, `bin/mettle` on Linux)
- NASM assembler — only needed for the legacy assembly path (`--emit-asm` or manual flow)
- System C toolchain/linker (`gcc`/`clang`) — only needed for external-link fallback and manual flows
- `--build` mode uses none of the above on Windows; the compiler emits COFF objects and links them with a built-in PE linker

## Built-In Help

Use the CLI help/docs commands to jump to the right topic quickly:

```powershell
.\bin\mettle.exe help
.\bin\mettle.exe help runtime
.\bin\mettle.exe help build
.\bin\mettle.exe docs
```

Available topics: `build`, `runtime` (alias `heap`/`gc`), `interop`, `stdlib`, `web`.

## Crash Traceback Example

Compile with `-s` to embed crash-traceback support (or `-d` for full debug output):

```powershell
.\bin\mettle.exe --build -s app.mettle -o app.exe
```

When the program faults, you get a symbolized stack:

```text
Unhandled runtime exception 0xC0000005 (access violation)
Exception address: 0x00007FF7DFD71046
write access violation at 0x0000000000000001
Stack trace:
  #0 leaf_crash at app.mettle:2:3 (0x00007FF7DFD71046)
  #1 main at app.mettle:8:3 (0x00007FF7DFD71080)
```

On Windows the underlying mechanism is SEH; on POSIX it is a `sigaction` handler on an alternate signal stack. Both produce the same backtrace format. See [docs/runtime-model.md](docs/runtime-model.md#crash_handlero--symbolized-crash-tracebacks) for the full mechanism.

## How it compiles

"Straight to x86-64" means there is no third-party code generator in the path.
Mettle does not lower to LLVM IR, and it does not emit C and shell out to a C
compiler. The compiler owns every phase from source text to machine code:

1. **Lexing** — tokenize source
2. **Parsing** — build the AST
3. **Import resolution** — resolve and inline `import` directives
4. **Monomorphization** — expand generics into concrete instantiations
5. **Type checking** — semantic analysis and symbol resolution
6. **IR lowering** — convert the AST to Mettle's own intermediate representation
7. **Optimization** (optional, `-O`) — propagation, folding, branch and codegen peepholes
8. **Code generation** — emit x86-64

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

- `src/`
  - `lexer/`, `parser/`, `semantic/`, `ir/`, `codegen/`, `linker/` — compiler pipeline
  - `runtime/` — the two opt-in helper objects: `crash_handler.{c,h}` and `atomics.{c,h}`. See [docs/runtime-model.md](docs/runtime-model.md)
  - `debug/`, `error/` — debug info and diagnostics
- `stdlib/` — standard library modules and platform helper C shims
- `tests/` — compiler, linker, and runtime test suite
- `web/` — web server demo
- `docs/` — language and tooling documentation
- `installer/` — Inno Setup installer scripts (Windows)

## Documentation

- Language reference: [docs/LANGUAGE.md](docs/LANGUAGE.md)
- Runtime model: [docs/runtime-model.md](docs/runtime-model.md)
- Compilation pipeline: [docs/compilation.md](docs/compilation.md)
- Lexical structure: [docs/lexical-structure.md](docs/lexical-structure.md)
- Types: [docs/types.md](docs/types.md)
- Expressions: [docs/expressions.md](docs/expressions.md)
- Control flow: [docs/control-flow.md](docs/control-flow.md)
- C interop: [docs/c-interop.md](docs/c-interop.md)
- Heap allocation semantics: [docs/heap-allocation.md](docs/heap-allocation.md)
- Standard library: [docs/standard-library.md](docs/standard-library.md)
- Known limitations: [docs/known-limitations.md](docs/known-limitations.md)

## Quick Dev Commands

```powershell
.\build.bat
.\tests\run_tests.ps1
.\tests\run_tests.ps1 -BuildCompiler
.\web\build.bat
```
