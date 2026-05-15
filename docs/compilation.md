# Compilation

This document describes how to compile Methlang programs and the available compiler options.

## Compiler Usage

```bash
methlang [options] <input.meth>
methlang help [topic]
methlang docs [topic]
```

The input file is the main source file. Imports are resolved relative to it. By default the compiler produces assembly (`output.s`). On Windows, `--emit-obj` produces a COFF object, and `--build` produces an executable.

`std/...` imports use the stdlib bundled with the compiler by default. You do not need to copy `stdlib/` into every project directory. Use `--stdlib <dir>` only when you want to override the bundled stdlib.

## Built-In Help and Docs

The compiler includes topic-oriented help commands:

- `methlang help` prints CLI usage.
- `methlang help build` explains the Windows build flow.
- `methlang help gc` explains how the bundled GC/runtime is linked.
- `methlang docs` lists the main documentation entry points and their paths.

Available topics: `build`, `gc`, `interop`, `stdlib`, `web`.

## Options

`-o <file>` output assembly/object file (default `output.s`, or executable path when used with `--build`). `-i <file>` input file (alternative to positional argument). `-I <dir>` add import search directory (repeatable). `--stdlib <dir>` set stdlib root (default auto-detects bundled stdlib near the compiler binary, then falls back to `./stdlib`). `--async-model <pool|coroutine>` choose async lowering (**default: `pool`**). See [Async and Sync Execution](async.md). `--emit-obj` emit a COFF object on Windows instead of assembly. `--build` build an executable on Windows. `--linker <auto|internal|gcc|msvc>` choose the Windows linker path. `--link-arg <arg>` pass an extra linker argument in `--build` mode for additional DLLs/import libraries. `--prelude` auto-import `std/prelude` (std/io, std/math, std/conv, std/mem, std/process, std/net). `-d`/`--debug` debug mode and embedded runtime crash traceback support. `-g`/`--debug-symbols` generate debug symbols. `-l`/`--line-mapping` source line mapping. `-s`/`--stack-trace` embeds runtime crash traceback support without the rest of debug mode. `-O`/`--optimize` enable optimizations. `-r`/`--release` enables `-O`, strips assembly comments, removes unreachable functions, and disables generated runtime null/bounds checks in IR lowering. `--strip-comments` omit emitted assembly comments. `-h`/`--help` print usage. See [Imports](imports.md) for path resolution and `-I`/`--stdlib` details.

## Compilation Pipeline

The compiler runs these phases in order:

1. **Lexing** - tokenize source
2. **Parsing** - build AST
3. **Import resolution** - resolve and inline `import` directives
4. **Monomorphization** - expand generic functions and structs into concrete instantiations
5. **Async rewrite** - transform `async function` / `async fn` per `--async-model`: **`pool`** (default) lowers to `Future<T>` + executor worker entry points; **`coroutine`** lowers to stackless task machinery through a generic CFG-level path for async bodies (experimental)
6. **Type checking** - semantic analysis and symbol resolution
7. **IR lowering** - convert AST to intermediate representation
8. **Optimization** (optional, `-O`) - copy/constant propagation, integer folding/simplification, branch cleanup, unreachable IR cleanup, and control-flow/codegen branch peepholes
9. **Code generation** - emit x86-64 assembly or, on Windows with `--emit-obj`, a COFF object

`--release` uses the same optimization pipeline as `-O` and additionally lowers without runtime null/bounds trap checks. Use `-O` for optimized builds that still keep those generated checks.

### Token String Views

Lexer tokens carry a `StringView` (`data` pointer + `length`) in addition to a null-terminated `value` string. This gives parser and diagnostics code direct token extents without requiring repeated `strlen`.

### String Interning

Identifier-like token text is interned: each distinct string is stored once in a global hash table, and subsequent occurrences reuse the same pointer. This reduces memory for repeated names and enables fast pointer-first equality checks in semantic structures.

The AST and symbol/type metadata intern name-bearing strings (identifier names, member names, type names, and type parameter names). The intern table is process-global for a compilation run and is cleared after compilation, so interned pointers are not reused across compiler invocations.

## Build Pipeline

### Recommended Windows Flow

1. Native object/internal-link build: `methlang --build --emit-obj --linker internal main.meth -o main.exe`
2. Optional extra libraries: `methlang --build --emit-obj --linker internal main.meth -o main.exe --link-arg -lcustomdll`
3. Assembly/auto path: `methlang --build main.meth -o main.exe`

`--build --emit-obj --linker internal` keeps the target build inside Methlang's object emitter, bundled runtime objects, and internal PE linker. That path does not require `NASM`, `gcc`, or `link.exe` for the target executable. The internal linker probes common Win32 DLLs directly (`kernel32`, `user32`, `gdi32`, `advapi32`, `ws2_32`, `ucrtbase`, and `msvcrt`), so `std/win32`, `std/thread`, and `std/net` work without hand-written C bridge objects or default import-library flags. `--build` with `--linker auto` tries the internal linker first and falls back to external linkers if needed. If you do not pass `--emit-obj`, the build still goes through assembly and requires `NASM`. The packaged runtime is part of the Methlang installation/build output; you do not need to add `gc.c` or `async_runtime.c` to each project manually.

### Async Executor Runtime Tuning

The async runtime now uses a bounded worker-pool executor. You can tune it at runtime:

- `METH_ASYNC_WORKERS=<N>` sets the pool worker count.
- `METH_ASYNC_QUEUE_CAPACITY=<M>` sets bounded queue capacity.

If neither is set, defaults are chosen by the runtime (worker count from logical CPUs, bounded queue capacity derived from worker count).

For embedding or C interop, call `meth_async_runtime_configure(worker_count, queue_capacity)` before the first async task starts.

For deterministic teardown in embedders/tests, use:

- `meth_async_runtime_shutdown(METH_ASYNC_SHUTDOWN_DRAIN, timeout_ms)` for graceful completion.
- `meth_async_runtime_shutdown(METH_ASYNC_SHUTDOWN_ABORT, timeout_ms)` for immediate stop/purge.
- `meth_async_runtime_reset()` only if you intentionally need re-init after `STOPPED` in the same process lifetime.

Call async shutdown before `gc_shutdown()` so worker threads are detached and joined before GC global state is released.

### Manual Assembly/Link Flow

1. Compile: `methlang main.meth -o main.s`
2. Assemble: `nasm -f win64 main.s -o main.o` (or `-f elf64` on Linux)
3. Link: `gcc -nostartfiles main.o gc.o -o main -lkernel32` (plus libraries such as `-lws2_32` for networking). Use `-nostartfiles` so Methlang's entry point (`mainCRTStartup`) is used instead of the C runtime's. If your program uses `new`, string concatenation, or async features, link the bundled runtime objects from your Methlang installation/build output. See [Garbage Collector](garbage-collector.md) and [Async and Sync Execution](async.md).

Programs that use async features also need the bundled async runtime object:

```bash
gcc -nostartfiles main.o path/to/runtime/gc.o path/to/runtime/async_runtime.o -o main -lkernel32
```

On POSIX toolchains, the bundled async runtime uses a pthread-backed implementation. Add pthread linkage as required by your environment.

**Programs with `main(argc, argv)`:** If your entry point has the signature `function main(argc: int32, argv: cstring*) -> int32`, you must also link bundled `runtime/methlang_entry.o` from your Methlang installation/build output. On Windows, link with `-lshell32` as well: `gcc -nostartfiles main.o gc.o methlang_entry.o -o main -lkernel32 -lshell32`.

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
  --> app.meth:5:10
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

Compile with `-s` or `-d` to embed runtime crash traceback support in the generated program. This adds failure-path-only metadata for Meth function names and source locations and installs a crash handler at program startup. The crash handler is **cross-platform**: Windows uses a Structured Exception Handler, and POSIX (Linux/macOS) uses a `sigaction` handler running on an alternate signal stack. Both produce the same symbolized stack-trace format from the same embedded debug-info tables.

- `-s` enables embedded runtime crash tracebacks without the rest of debug mode.
- `-d` enables debug output and also implies embedded runtime crash tracebacks.
- `--release` still disables generated null/bounds runtime checks, so only native crashes remain traceable there.

A fatal fault prints the exception/signal, the faulting address (with a hint when it is a null-pointer dereference), and a symbolized stack trace. Compiler-generated null-dereference and array-bounds traps print the same traceback shape. Frames without registered Meth debug info (libc/CRT) show as `<unknown>`, the same on both platforms.

Windows example (native access violation):

```text
Unhandled runtime exception 0xC0000005 (access violation)
Exception address: 0x00007FF7DFD71046
write access violation at 0x0000000000000001
Stack trace:
  #0 leaf_crash at app.meth:2:3 (0x00007FF7DFD71046)
  #1 intermediate at app.meth:6:3 (0x00007FF7DFD71080)
  #2 main at app.meth:10:3 (0x00007FF7DFD710A0)
```

POSIX example (null dereference via SIGSEGV):

```text
Unhandled runtime signal 11 (segmentation fault (invalid memory access))
Faulting address: 0x0000000000000000  (null pointer dereference)
Fault instruction: 0x000057AD4388733D
Stack trace:
  #0 compute_total at app.meth:12:5 (0x000057AD4388733D)
  #1 main at app.meth:20:3 (0x000057AD438873CE)
```

Compiler-generated runtime traps are formatted the same on both platforms:

```text
Fatal error: Null pointer dereference
Stack trace:
  #0 main at app.meth:9:10 (0x00007FF7DFD71046)
```

The signal handler runs on a dedicated alternate stack so that a stack-overflow `SIGSEGV` can still be reported rather than silently re-faulting, and uses only async-signal-safe primitives.

## Web Server Example

The `web/` directory contains a complete HTTP server example. Its build script uses the native Windows object/internal-link path. Build and run:

```bash
.\web\build.bat
```

Then open http://localhost:5000 in a browser.

### Web Server Reliability Notes

When modifying `web/server.meth` or `web/index.html`, use these rules to avoid runtime crashes:

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

On Windows x64, Methlang now emits stack probing (`___chkstk_ms`) for large frame allocations (>4 KiB) before subtracting `rsp`, to avoid guard-page skips.

The warning threshold is currently fixed and may become configurable in a future release.

## Testing

The test suite compiles and runs a set of programs. Run:

```powershell
.\tests\run_tests.ps1
.\tests\run_tests.ps1 -BuildCompiler
.\tests\run_tests.ps1 -SkipRuntime
```

`-BuildCompiler` rebuilds the compiler before running. `-SkipRuntime` skips the GC runtime executable test.
