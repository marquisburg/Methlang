# MethASM

MethASM is a compiled, typed, assembly-like language for x86-64.
It is designed to stay low-level while adding stronger semantics than raw asm.
Source files use the `.masm` extension and compile to NASM-compatible assembly.
The compiler includes lexing, parsing, semantic/type analysis, IR lowering, and codegen.
The language supports structs, arrays, pointers, function pointers, generics, modules, explicit casting, and C interop.
Control flow includes `if`, `while`, `for`, `switch`, `defer`, and `errdefer`.
Programs can use a conservative runtime GC for `new` and GC-backed string concatenation.
Standard library modules cover I/O, conversion, networking, process, threading, and more.
Current output is x86-64 assembly for NASM object formats.
This repository contains the compiler, runtime, standard library, tests, and web demo.

## Hello World

```masm
import "std/io";

function main() -> int32 {
  println("Hello, MethASM!");
  return 0;
}
```

Build/run (Windows example):

```powershell
.\bin\methasm.exe hello.masm -o hello.s
nasm -f win64 hello.s -o hello.o
gcc -nostartfiles hello.o src\runtime\gc.c -o hello.exe -lkernel32
.\hello.exe
```

Use `-nostartfiles` so MethASM's entry point (`mainCRTStartup`) is used instead of the C runtime's.

## Targets and Toolchain

- Targets:
  - `win64` object format (`nasm -f win64`) for Windows x64
  - `elf64` object format (`nasm -f elf64`) for Linux x64
- Compiler output: x86-64 Intel/NASM-style assembly
- Required tools:
  - MethASM compiler (`bin/methasm.exe` or locally built binary)
  - NASM assembler
  - C toolchain/linker (GCC/MinGW on Windows, GCC/Clang on Linux)
- Runtime note:
  - Link `src/runtime/gc.c` when using `new` or string concatenation
  - For GC use from worker threads, attach/detach threads with `gc_thread_attach`/`gc_thread_detach`
  - Safepoint register spilling defaults to `xmm0..xmm15`; build compiler with `METHASM_SAFEPOINT_SPILL_XMM31` for `xmm0..xmm31` coverage
  - Networking examples may require extra system libs (for example `-lws2_32` on Windows)

## Docs

- Language reference index: [docs/LANGUAGE.md](docs/LANGUAGE.md)
- Garbage collector guide: [docs/garbage-collector.md](docs/garbage-collector.md)
- Compilation details: [docs/compilation.md](docs/compilation.md)
- Standard library overview: [docs/standard-library.md](docs/standard-library.md)

## Quick Dev Commands

```powershell
.\build.bat
.\tests\run_tests.ps1
.\tests\run_tests.ps1 -BuildCompiler
```
