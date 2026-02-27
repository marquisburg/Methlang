# MethASM Language Reference

MethASM is a typed, assembly-inspired language that compiles to x86-64 assembly. This reference is split into focused documents for cohesion and clarity.

## Table of Contents

1. [Lexical Structure](lexical-structure.md)
2. [Types](types.md)
3. [Declarations](declarations.md)
4. [Expressions](expressions.md)
5. [Control Flow](control-flow.md)
6. [Modules](modules.md)
7. [Imports](imports.md)
8. [Standard Library](standard-library.md)
9. [Garbage Collector](garbage-collector.md)
10. [C Interoperability](c-interop.md)
11. [Compilation](compilation.md)
12. [Quick Reference](quick-reference.md)
13. [Known Limitations](known-limitations.md)

## Overview

MethASM provides explicit typing (all variables and function parameters have declared types), structured control flow (`if`, `while`, `for`, `switch` with `break`/`continue`), structs and enums, pointers and arrays with bounds-aware indexing, C interop via `extern`, a module system for importing other `.masm` files, and garbage collection for heap allocation via `new` with conservative GC runtime.

The compiler emits NASM-compatible x86-64 assembly. Linking uses the platform C runtime and any required libraries (e.g. `ws2_32` for networking on Windows).
