# Methlang

Methlang is a typed, low-level language that compiles `.meth` source files to x86-64 NASM assembly and, on Windows, native COFF objects for direct PE linking.

It is designed for systems-style control with stronger semantics than raw assembly: structured control flow, static type checking, modules, generics, and C interop.

## Highlights

- Compiles to x86-64 NASM assembly and Windows COFF objects
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

1. Build an executable with the native Windows path:

```powershell
.\bin\methlang.exe --build --emit-obj --linker internal hello.meth -o hello.exe
.\hello.exe
```

This path does not require `NASM`, `gcc`, or `link.exe` for the target build. Plain `--build` still defaults to the assembly-based auto path unless you also pass `--emit-obj`.

No project-local `stdlib/` folder is required. The compiler auto-loads the stdlib bundled with the Methlang installation/build output. Use `--stdlib <dir>` only when you want to override that.

For production builds, use `--release`:

```powershell
.\bin\methlang.exe --build --emit-obj --linker internal --release hello.meth -o hello.exe
```

`--release` enables `-O`, strips assembly comments, removes unreachable functions, and lowers without generated runtime null/bounds trap checks.

1. Optional: emit assembly only:

```powershell
.\bin\methlang.exe hello.meth -o hello.s
nasm -f win64 hello.s -o hello.o
gcc -nostartfiles hello.o "$env:ProgramFiles\Methlang\runtime\gc.o" -o hello.exe -lkernel32
.\hello.exe
```

Use `-nostartfiles` so Methlang's entry point (`mainCRTStartup`) is used instead of the C runtime entry. Manual assembly/linking is mainly for advanced cases; the default workflow is `methlang --build`.

## Quick Start (Linux)

```bash
make
./bin/methlang hello.meth -o hello.s
nasm -f elf64 hello.s -o hello.o
gcc -nostartfiles hello.o /usr/local/runtime/gc.o -o hello
./hello
```

## Toolchain

- Methlang compiler (`bin/methlang.exe` on Windows, `bin/methlang` on Linux)
- NASM assembler for assembly-based builds
- System C toolchain/linker (`gcc`/`clang`) for external-link fallback and manual assembly/object flows

## Built-In Help

Use the CLI help/docs commands to jump to the right topic quickly:

```powershell
.\bin\methlang.exe help
.\bin\methlang.exe help gc
.\bin\methlang.exe help build
.\bin\methlang.exe docs
```

Available topics: `build`, `gc`, `interop`, `stdlib`, `web`.

## Runtime and Linking Notes

- `methlang --build --emit-obj --linker internal` uses the bundled runtime objects plus Methlang's internal PE linker on Windows.
- `methlang --build` in `auto` mode tries the internal linker first and falls back to external linkers if needed.
- If you use the manual assembly/link flow, link bundled `runtime/gc.o` from your Methlang installation when using `new` or string concatenation.
- Compile with `-s` to embed runtime crash traceback support, or use `-d` to enable it alongside normal debug output.
- On Windows, embedded crash tracebacks report native exception codes such as `0xC0000005` and compiler-generated runtime traps with Meth function/source frames.
- Networking examples may require extra libraries (for example `--link-arg -lws2_32` on Windows).
- For GC use from worker threads, use `gc_thread_attach` and `gc_thread_detach`.

Example runtime crash output:

```text
Unhandled runtime exception 0xC0000005 (access violation)
Exception address: 0x00007FF7DFD71046
write access violation at 0x0000000000000001
Stack trace:
  #0 leaf_crash at app.meth:2:3 (0x00007FF7DFD71046)
  #1 main at app.meth:8:3 (0x00007FF7DFD71080)
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
