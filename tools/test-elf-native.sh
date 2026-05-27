#!/usr/bin/env bash
# Native-ELF backend regression test for Linux.
#
# Compiles small Mettle programs to standalone, statically linked ELF
# executables (the compiler's own _start, no libc/CRT) and asserts their exit
# codes. Covers SysV argument passing (register + stack overflow), recursion,
# argc/argv off the kernel stack, and the full `mettle --build` driver.
#
# Expects the compiler at bin/mettle (build it with `make`). Requires gcc + ld.
# Usage: tools/test-elf-native.sh [path-to-mettle]
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
METTLE="${1:-$ROOT/bin/mettle}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

if [ ! -x "$METTLE" ]; then
  echo "error: compiler not found at $METTLE (run 'make' first)" >&2
  exit 2
fi

fail=0

# run_case <name> <expected-exit> <source> [run-args...]
run_case() {
  local name="$1" want="$2" src="$3"; shift 3
  printf '%s' "$src" > "$WORK/$name.mettle"
  if ! "$METTLE" --build "$WORK/$name.mettle" -o "$WORK/$name.bin" \
        >"$WORK/$name.log" 2>&1; then
    echo "[$name] BUILD FAILED"; sed 's/^/    /' "$WORK/$name.log"; fail=1; return
  fi
  # Confirm it really is a standalone static ELF, not something libc-linked.
  if ! file "$WORK/$name.bin" | grep -q "statically linked"; then
    echo "[$name] WARNING: not statically linked"
  fi
  "$WORK/$name.bin" "$@"; local got=$?
  if [ "$got" = "$want" ]; then
    echo "[$name] PASS (exit $got)"
  else
    echo "[$name] FAIL got $got want $want"; fail=1
  fi
}

run_case loop 45 'function compute(n: int32) -> int32 {
  var acc: int32 = 0; var i: int32 = 0;
  while (i < n) { acc = acc + i; i = i + 1; }
  return acc;
}
function main() -> int32 { return compute(10); }'

run_case recursion 55 'function fib(n: int32) -> int32 {
  if (n < 2) { return n; }
  return fib(n - 1) + fib(n - 2);
}
function main() -> int32 { return fib(10); }'

# 8 integer args: 6 in SysV registers, 2 spilled to the stack.
run_case stackargs 36 'function sum8(a: int64, b: int64, c: int64, d: int64, e: int64, f: int64, g: int64, h: int64) -> int64 {
  return a + b + c + d + e + f + g + h;
}
function main() -> int32 { return (int32)sum8(1, 2, 3, 4, 5, 6, 7, 8); }'

# argc read off the kernel stack at _start; 3 args + argv[0] => 4.
run_case argcount 4 'function main(argc: int32, argv: int8**) -> int32 { return argc; }' a b c

if [ "$fail" = 0 ]; then
  echo "ALL NATIVE ELF TESTS PASSED"
else
  echo "SOME NATIVE ELF TESTS FAILED"
fi
exit $fail
