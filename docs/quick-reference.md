# Quick Reference

Short examples for common use cases.

## Minimal Program

```mettle
function main() -> int32 {
  return 0;
}
```

## With Imports

```mettle
import "std/io";

function main() -> int32 {
  println("Hello, Mettle!");
  return 0;
}
```

See [Imports](imports.md) for path resolution and `import_str`.

## With Prelude

```mettle
// Compile with: mettle --prelude main.mettle -o main.s
function main() -> int32 {
  println("Hello");
  return 0;
}
```

## With Extern

```mettle
extern function puts(msg: cstring) -> int32 = "puts";

function main() -> int32 {
  puts("Hello");
  return 0;
}
```

## With Async and Await

By default (`--async-model pool` or omitted), `async` calls return `Future<T>` and `await` blocks until the payload is ready. Use `--async-model coroutine` for experimental stackless lowering (see [Async and Sync Execution](async.md)).

```mettle
async fn add_one(x: int32) -> int32 {
  return x + 1;
}

function main() -> int32 {
  var future: Future<int32> = add_one(41);
  return await future;
}
```

## With Enum and Switch

```mettle
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

## With Explicit Casts

```mettle
function main() -> int32 {
  var f: float64 = 3.14;
  var i: int32 = (int32)f;
  
  var p: int32* = (int32*)0;
  var address: int64 = (int64)p;
  
  return i;
}
```

## With GC and Structs

Uses `new` for heap allocation. `mettle --build` links the bundled GC/runtime automatically on Windows. Async programs use the same bundled runtime flow. See [Garbage Collector](garbage-collector.md) and [Async and Sync Execution](async.md).

```mettle
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

## With Generics

Generic functions and structs with compile-time monomorphization. See [Declarations](declarations.md#generic-functions) and [Types](types.md#generic-type-parameters).

```mettle
struct Pair<A, B> {
  first: A;
  second: B;
}

function swap<T>(a: T*, b: T*) -> void {
  var tmp: T = *a;
  *a = *b;
  *b = tmp;
}

function main() -> int32 {
  var p: Pair<int32, int32>;
  p.first = 10;
  p.second = 20;
  swap<int32>(&p.first, &p.second);
  return p.first + p.second;
}
```
