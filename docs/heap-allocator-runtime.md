# Heap Allocation

Mettle does not ship a garbage collector or a heap manager.

`new`, array literals that need heap storage, and string concatenation allocate
zeroed memory by emitting a direct C runtime call:

```asm
calloc(1, size)
```

There is no tracing, bookkeeping, root scanning, safepoints, shutdown sweep, or
Mettle heap runtime object. The operating system reclaims this memory at process
exit unless user code explicitly manages a buffer through `std/mem`.

## Runtime Objects

Ordinary heap allocation does not link a Mettle runtime object. Two optional
helper objects ship with the compiler and are pulled in only when their symbols
are actually referenced:

- `crash_handler.o` — installs the SEH/sigaction crash handler and provides
  `mettle_crash_trap`. Linked when an emitted object references
  `mettle_crash_*` (i.e. compiled with `-d`, `-s`, `-g`, or with IR null/bounds
  traps left enabled in non-release builds).
- `atomics.o` — thin platform wrappers over Win32 Interlocked* / GCC `__sync_*`
  intrinsics. Linked when an emitted object references `mettle_atomic_*` (any
  use of `std/thread` atomic helpers).

A program that uses `new` or string concatenation but does not use crash
tracebacks or thread atomics links zero Mettle runtime objects.

## When to Use `new`

Use `new` when you want a zero-initialized heap object:

```mettle
struct Point { x: int32; y: int32; }

function main() -> int32 {
  var p: Point* = new Point;
  p.x = 10;
  p.y = 20;
  return p.x + p.y;
}
```

Use `std/mem` (`malloc`/`calloc`/`realloc`/`free`) for buffers with explicit
lifetime, C interop, or long-running ownership.

## Linking Without `--build`

Manual assembly/linking only needs the platform C runtime:

```bash
mettle main.mettle -o main.s
nasm -f win64 main.s -o main.o
gcc -nostartfiles main.o -o main -lkernel32
```

Add `runtime/crash_handler.o` only for runtime tracebacks (`-d`/`-s`/IR
traps); add `runtime/atomics.o` only when `std/thread` atomic helpers are
used.
