# Compilation

This document describes how to compile Mettle programs and the available compiler options.

## Compiler Usage

```bash
mettle [options] <input.mettle>
mettle help [topic]
mettle docs [topic]
```

The input file is the main source file. Imports are resolved relative to it. By default the compiler produces assembly (`output.s`). On Windows, `--build` emits a COFF object and links an executable with the native PE linker; `--emit-asm` selects the legacy NASM assembly path instead.

`std/...` imports use the stdlib bundled with the compiler by default. You do not need to copy `stdlib/` into every project directory. Use `--stdlib <dir>` only when you want to override the bundled stdlib.

## Built-In Help and Docs

The compiler includes topic-oriented help commands:

- `mettle help` prints CLI usage.
- `mettle help build` explains the Windows build flow.
- `mettle help heap` explains how the bundled heap/runtime object is linked (`gc` remains an alias).
- `mettle docs` lists the main documentation entry points and their paths.

Available topics: `build`, `heap`, `gc`, `interop`, `stdlib`, `web`.

## Options

`-o <file>` output assembly/object file (default `output.s`, or executable path when used with `--build`). `-i <file>` input file (alternative to positional argument). `-I <dir>` add import search directory (repeatable). `--stdlib <dir>` set stdlib root (default auto-detects bundled stdlib near the compiler binary, then falls back to `./stdlib`). `--build` build an executable on Windows (native COFF object + internal PE linker by default). `--emit-obj` emit a COFF object (default with `--build`). `--emit-asm` with `--build`, emit assembly and use NASM instead of native COFF. `--linker <internal|auto|gcc|msvc>` choose the Windows linker path (**default: `internal`**). `--link-arg <arg>` pass an extra linker argument in `--build` mode for additional DLLs/import libraries. `--prelude` auto-import `std/prelude` (std/io, std/math, std/conv, std/mem, std/process, std/net). `-d`/`--debug` debug mode and embedded runtime crash traceback support. `-g`/`--debug-symbols` generate debug symbols. `-l`/`--line-mapping` source line mapping. `-s`/`--stack-trace` embeds runtime crash traceback support without the rest of debug mode. `-O`/`--optimize` enable optimizations. `-r`/`--release` enables `-O`, strips assembly comments, removes unreachable functions, and disables generated runtime null/bounds checks in IR lowering. `--strip-comments` omit emitted assembly comments. `-h`/`--help` print usage. See [Imports](imports.md) for path resolution and `-I`/`--stdlib` details.

## Compilation Pipeline

The compiler runs these phases in order:

1. **Lexing** - tokenize source
2. **Parsing** - build AST
3. **Import resolution** - resolve and inline `import` directives
4. **Monomorphization** - expand generic functions and structs into concrete instantiations
5. **Type checking** - semantic analysis and symbol resolution
6. **IR lowering** - convert AST to intermediate representation
7. **Optimization** (optional, `-O`) - copy/constant propagation, integer folding/simplification, branch cleanup, unreachable IR cleanup, and control-flow/codegen branch peepholes
8. **Code generation** - emit x86-64 assembly or, on Windows with `--emit-obj`, a COFF object

`--release` uses the same optimization pipeline as `-O` and additionally lowers without runtime null/bounds trap checks. Use `-O` for optimized builds that still keep those generated checks.

### Token String Views

Lexer tokens carry a `StringView` (`data` pointer + `length`) in addition to a null-terminated `value` string. This gives parser and diagnostics code direct token extents without requiring repeated `strlen`.

### String Interning

Identifier-like token text is interned: each distinct string is stored once in a global hash table, and subsequent occurrences reuse the same pointer. This reduces memory for repeated names and enables fast pointer-first equality checks in semantic structures.

The AST and symbol/type metadata intern name-bearing strings (identifier names, member names, type names, and type parameter names). The intern table is process-global for a compilation run and is cleared after compilation, so interned pointers are not reused across compiler invocations.

## Build Pipeline

### Recommended Windows Flow

1. Default end-to-end build: `mettle --build main.mettle -o main.exe`
2. Optional extra libraries: `mettle --build main.mettle -o main.exe --link-arg -lcustomdll`
3. Legacy assembly path: `mettle --build --emit-asm main.mettle -o main.exe`
4. External linker fallback: `mettle --build --linker auto main.mettle -o main.exe`

`--build` keeps compilation inside Mettle's COFF object emitter, bundled runtime objects, and internal PE linker. That path does not require `NASM`, `gcc`, or `link.exe` for the target executable. The internal linker probes common Win32 DLLs directly (`kernel32`, `user32`, `gdi32`, `advapi32`, `ws2_32`, `ucrtbase`, and `msvcrt`), so `std/win32`, `std/thread`, and `std/net` work without hand-written C bridge objects or default import-library flags. `--linker auto` tries the internal linker first and falls back to external linkers if needed. `--emit-asm` selects the NASM assembly path instead. Two optional helper objects ship with the Mettle installation — `crash_handler.o` (linked only for `-d`/`-s`/`-g` or IR null/bounds traps) and `atomics.o` (linked only when `std/thread` interlocked atomics are referenced) — and `--build` pulls them in automatically when needed.

### Manual Assembly/Link Flow

1. Compile: `mettle main.mettle -o main.s`
2. Assemble: `nasm -f win64 main.s -o main.o` (or `-f elf64` on Linux)
3. Link: `gcc -nostartfiles main.o -o main -lkernel32` (plus libraries such as `-lws2_32` for networking). Use `-nostartfiles` so Mettle's entry point (`mainCRTStartup`) is used instead of the C runtime's.

The emitted entry point does not call any Mettle runtime initialization. Programs that do not use `-d`/`-s` crash tracebacks or `std/thread` interlocked atomics link **zero** Mettle runtime objects, even when they use `new` or string concatenation.

Link the relevant helper object(s) only when your program references their symbols:

- `crash_handler.o` if the program references `mettle_crash_*` (compiled with `-d`, `-s`, `-g`, or with IR null/bounds traps left enabled).
- `atomics.o` if the program references `mettle_atomic_*` (any use of `std/thread`'s `atomic_compare_exchange_i32` / `_exchange_i32` / `_inc_i32` / `_dec_i32`).

```bash
gcc -nostartfiles main.o \
    path/to/runtime/crash_handler.o \
    path/to/runtime/atomics.o \
    -o main -lkernel32
```

Omit either object when the corresponding symbols are not referenced.

For concurrency, import `std/thread` (Windows) or `std/thread_posix` and call `CreateThread`/`pthread_create` directly — Mettle no longer has built-in `async`/`spawn`/`Channel<T>` keywords.

**Programs with `main(argc, argv)`:** If your entry point has the signature `function main(argc: int32, argv: cstring*) -> int32`, Windows startup emits a direct CRT `__getmainargs` call. No Mettle argv shim is required.

The output format depends on the target. Use `-f win64` for Windows, `-f elf64` for Linux. NASM is required for assembly; install from https://www.nasm.us/ if needed. On Linux and macOS, use `make` to build the compiler and run tests. The web server example in `web/` is Windows-only (Winsock). See [Standard Library](standard-library.md#platform-support) for Linux support details.

## Compiler Diagnostics

Compile-time errors and warnings are printed with a stable error code, the
source location, a code snippet with a caret pointing at the offending span
(plus one line of surrounding context), and — where possible — a `help:`
suggestion. Output is colorized on a TTY and respects `NO_COLOR`,
`CLICOLOR`/`CLICOLOR_FORCE`, and `TERM=dumb`. Up to 100 diagnostics are
reported per run rather than stopping at the first.

Error codes: `E0001` lexical, `E0002` syntax, `E0003` semantic, `E0004`
type, `E0005` scope, `E0006` I/O, `E0007` internal. They are stable across
versions and useful for `grep` and documentation.

### "Did you mean?" suggestions

When you reference an undefined variable or function, the compiler searches
every name visible in the current scope chain and, if one is a close match
(case-insensitive Levenshtein distance within a length-scaled threshold),
suggests it:

```text
error[E0003]: Undefined variable 'countr'
  --> app.mettle:5:10
  |
4 |   var counter: int32 = 41;
5 |   return countr + 1;
  |          ^^^^^^
6 | }
   = help: did you mean 'counter'? (or declare 'countr' before using it)
```

The suggestion is scope-aware: only symbols actually reachable from the
error site are considered. If nothing is close enough, the diagnostic falls
back to the generic "declare it before using it" guidance, so unrelated
names never produce a misleading suggestion.

## Runtime Crash Tracebacks

Compile with `-s` or `-d` to embed runtime crash traceback support in the generated program. This adds failure-path-only metadata for Mettle function names and source locations and installs a crash handler at program startup. The crash handler is **cross-platform**: Windows uses a Structured Exception Handler, and POSIX (Linux/macOS) uses a `sigaction` handler running on an alternate signal stack. Both produce the same symbolized stack-trace format from the same embedded debug-info tables.

- `-s` enables embedded runtime crash tracebacks without the rest of debug mode.
- `-d` enables debug output and also implies embedded runtime crash tracebacks.
- `--release` still disables generated null/bounds runtime checks, so only native crashes remain traceable there.

A fatal fault prints the exception/signal, the faulting address (with a hint when it is a null-pointer dereference), and a symbolized stack trace. Compiler-generated null-dereference and array-bounds traps print the same traceback shape. Frames without registered Mettle debug info (libc/CRT) show as `<unknown>`, the same on both platforms.

Windows example (native access violation):

```text
Unhandled runtime exception 0xC0000005 (access violation)
Exception address: 0x00007FF7DFD71046
write access violation at 0x0000000000000001
Stack trace:
  #0 leaf_crash at app.mettle:2:3 (0x00007FF7DFD71046)
  #1 intermediate at app.mettle:6:3 (0x00007FF7DFD71080)
  #2 main at app.mettle:10:3 (0x00007FF7DFD710A0)
```

POSIX example (null dereference via SIGSEGV):

```text
Unhandled runtime signal 11 (segmentation fault (invalid memory access))
Faulting address: 0x0000000000000000  (null pointer dereference)
Fault instruction: 0x000057AD4388733D
Stack trace:
  #0 compute_total at app.mettle:12:5 (0x000057AD4388733D)
  #1 main at app.mettle:20:3 (0x000057AD438873CE)
```

Compiler-generated runtime traps are formatted the same on both platforms:

```text
Fatal error: Null pointer dereference
Stack trace:
  #0 main at app.mettle:9:10 (0x00007FF7DFD71046)
```

The signal handler runs on a dedicated alternate stack so that a stack-overflow `SIGSEGV` can still be reported rather than silently re-faulting, and uses only async-signal-safe primitives.

## Web Server Example

The `web/` directory contains a complete HTTP server example. Its build script uses the native Windows object/internal-link path. Build and run:

```bash
.\web\build.bat
```

Then open http://localhost:5000 in a browser.

### Web Server Reliability Notes

When modifying `web/server.mettle` or `web/index.html`, use these rules to avoid runtime crashes:

1. Prefer streaming responses with `send_all` instead of building large dynamic response buffers on the stack.
2. If you use `Content-Length`, compute it exactly and ensure any header buffer is large enough for worst-case digits and header text.
3. If you do not need keep-alive, send `Connection: close` and write header + body separately (valid HTTP/1.1, simpler, safer).
4. Keep per-request local buffers bounded and explicit (`recv_buf`, temporary header/data buffers), and avoid unchecked `memcpy` into fixed arrays.
5. After any web change, run a quick smoke test on all paths:
   - `GET /` returns `200`
   - `GET /health` returns `200` + `OK`
   - Unknown path returns `404`

These checks catch the common failure mode where only one route (often `/`) crashes due to response construction bugs.

## Stack Safety Diagnostics

The compiler emits a warning for unusually large function stack frames (currently 256 KiB). This is intended as an early signal for stack overflow risk in deeply nested calls or thread stacks with limited reserve.

On Windows x64, Mettle now emits stack probing (`___chkstk_ms`) for large frame allocations (>4 KiB) before subtracting `rsp`, to avoid guard-page skips.

The warning threshold is currently fixed and may become configurable in a future release.

## Compiler debugging

Use explicit dump flags to inspect the compiler pipeline. Artifacts are written
next to `-o` (or under `--dump-dir` when set) using the output stem:

| Flag | Output |
|------|--------|
| `--dump-ir` | `{stem}.ir.before.mettle` and `{stem}.ir.after.mettle` (same as `--dump-ir=before,after`) |
| `--dump-ir=before` / `--dump-ir=after` | Selected stage only |
| `--dump-ir-passes` | `{stem}.ir.pass-<label>.mettle` after optimization milestones (requires `-O`) |
| `--dump-ast` | `{stem}.ast.mettle` after type checking |
| `--dump-mono` | `{stem}.mono.mettle` index plus `{stem}.mono.<mangled>.mettle` per expansion |
| `--dump-dir <dir>` | Directory for all dump files |

IR dumps include source locations (`; @file:line:col`), basic-block labels, and
parameter types. `-d`/`--debug` also enables `--dump-ir=before,after` plus debug
symbol generation.

```powershell
mettle --dump-ir -O app.mettle -o app.s
mettle --dump-ir-passes -O app.mettle -o app.s
mettle --dump-ast app.mettle -o app.s
mettle --dump-mono tests/test_generics_multiple_instantiations.mettle -o out.s
mettle help debug
```

Helper scripts live under `tools/debug/` (`dump-compiler-artifacts.ps1`,
`diff-ir.ps1`, `disasm-obj.ps1`).

With `--emit-obj` or `--build` (internal linker), `-g` embeds binary DWARF 4
sections (`.debug_info`, `.debug_abbrev`, `.debug_line`, `.debug_str`,
`.debug_frame`) in COFF objects and the linked PE for GDB/LLDB. Locals and
parameters kept in GP registers by the optimizer (for example `r12`–`r15`) are
described with `DW_OP_regN` location expressions; stack-homed symbols use
`DW_OP_fbreg`. The assembly path still writes a human-readable `.dwarf` sidecar
when `-g` is used without `--emit-obj`. Runtime stack-trace tables (`-s`) are not
yet supported on the object path.

## Testing

The test suite compiles and runs a set of programs. Run:

```powershell
.\tests\run_tests.ps1
.\tests\run_tests.ps1 -BuildCompiler
.\tests\run_tests.ps1 -SkipRuntime
```

`-BuildCompiler` rebuilds the compiler before running. `-SkipRuntime` skips optional runtime executable tests.
