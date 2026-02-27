# Quick Reference

Short examples for common use cases.

## Minimal Program

```masm
function main() -> int32 {
  return 0;
}
```

## With Imports

```masm
import "std/io";

function main() -> int32 {
  println("Hello, MethASM!");
  return 0;
}
```

See [Imports](imports.md) for path resolution and `import_str`.

## With Prelude

```masm
// Compile with: methasm --prelude main.masm -o main.s
function main() -> int32 {
  println("Hello");
  return 0;
}
```

## With Extern

```masm
extern function puts(msg: cstring) -> int32 = "puts";

function main() -> int32 {
  puts("Hello");
  return 0;
}
```

## With Enum and Switch

```masm
enum Status { Ok = 0, Error = 1 }

function main() -> int32 {
  var s: Status = Ok;
  switch (s) {
    case 0:
      return 0;
    default:
      return 1;
  }
}
```

## With GC and Structs

Uses `new` for heap allocation. Link `gc.c` when building. See [Garbage Collector](garbage-collector.md).

```masm
struct Point {
  x: int32;
  y: int32;
}

function main() -> int32 {
  var p: Point* = new Point;
  p->x = 10;
  p->y = 20;
  return p->x + p->y;
}
```
