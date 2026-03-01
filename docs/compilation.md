# Compilation

This document describes how to compile MethASM programs and the available compiler options.

## Compiler Usage

```bash
methasm [options] <input.masm>
```

The input file is the main source file. Imports are resolved relative to it. The compiler produces assembly (default `output.s`).

## Options

`-o <file>` output assembly file (default `output.s`). `-i <file>` input file (alternative to positional argument). `-I <dir>` add import search directory (repeatable). `--stdlib <dir>` set stdlib root (default `stdlib`). `--prelude` auto-import `std/prelude`. `-d`/`--debug` debug mode. `-g`/`--debug-symbols` generate debug symbols. `-l`/`--line-mapping` source line mapping. `-O`/`--optimize` enable optimizations. `-h`/`--help` print usage. See [Imports](imports.md) for path resolution and `-I`/`--stdlib` details.

## Compilation Pipeline

The compiler runs these phases in order:

1. **Lexing** - tokenize source
2. **Parsing** - build AST
3. **Import resolution** - resolve and inline `import` directives
4. **Monomorphization** - expand generic functions and structs into concrete instantiations
5. **Type checking** - semantic analysis and symbol resolution
6. **IR lowering** - convert AST to intermediate representation
7. **Optimization** (optional, `-O`) - constant folding and other passes
8. **Code generation** - emit x86-64 assembly

### Token String Views

Lexer tokens carry a `StringView` (`data` pointer + `length`) in addition to a null-terminated `value` string. This gives parser and diagnostics code direct token extents without requiring repeated `strlen`.

### String Interning

Identifier-like token text is interned: each distinct string is stored once in a global hash table, and subsequent occurrences reuse the same pointer. This reduces memory for repeated names and enables fast pointer-first equality checks in semantic structures.

The AST and symbol/type metadata intern name-bearing strings (identifier names, member names, type names, and type parameter names). The intern table is process-global for a compilation run and is cleared after compilation, so interned pointers are not reused across compiler invocations.

## Build Pipeline

1. Compile: `methasm main.masm -o main.s`
2. Assemble: `nasm -f win64 main.s -o main.o` (or `-f elf64` on Linux)
3. Link: `gcc -nostartfiles main.o gc.o -o main -lkernel32` (plus libraries such as `-lws2_32` for networking). Use `-nostartfiles` so MethASM's entry point (`mainCRTStartup`) is used instead of the C runtime's. If your program uses `new`, compile and link `src/runtime/gc.c`. See [Garbage Collector](garbage-collector.md).

**Programs with `main(argc, argv)`:** If your entry point has the signature `function main(argc: int32, argv: cstring*) -> int32`, you must also compile and link `src/runtime/masm_entry.c` so the runtime can obtain command-line arguments. On Windows, link with `-lshell32` as well: `gcc -nostartfiles main.o gc.o masm_entry.o -o main -lkernel32 -lshell32`.

The output format depends on the target. Use `-f win64` for Windows, `-f elf64` for Linux. NASM is required for assembly; install from https://www.nasm.us/ if needed. On Linux and macOS, use `make` to build the compiler and run tests. The web server example in `web/` is Windows-only (Winsock). See [Standard Library](standard-library.md#platform-support) for Linux support details.

## Web Server Example

The `web/` directory contains a complete HTTP server example. Build and run:

```bash
cd web
.\build.bat
.\server.exe
```

Then open http://localhost:5000 in a browser.

### Web Server Reliability Notes

When modifying `web/server.masm` or `web/index.html`, use these rules to avoid runtime crashes:

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

On Windows x64, MethASM now emits stack probing (`___chkstk_ms`) for large frame allocations (>4 KiB) before subtracting `rsp`, to avoid guard-page skips.

The warning threshold is currently fixed and may become configurable in a future release.

## Testing

The test suite compiles and runs a set of programs. Run:

```powershell
.\tests\run_tests.ps1
.\tests\run_tests.ps1 -BuildCompiler
.\tests\run_tests.ps1 -SkipRuntime
```

`-BuildCompiler` rebuilds the compiler before running. `-SkipRuntime` skips the GC runtime executable test.

