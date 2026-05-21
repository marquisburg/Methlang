# Performance Benchmark Harness (Mettle vs C)

This project ships a dedicated benchmark harness for comparing Mettle output against C implementations.

## Files

- Harness config: `docs/benchmarks/harness.json`
- Harness runner: `tools/benchmark/run-benchmarks.ps1`
- Canonical output: `docs/benchmarks/latest.json`
- Web mirror output: `web/benchmarks.json`

## Run

```powershell
.\tools\benchmark\run-benchmarks.ps1
```

If you need to rebuild the compiler first:

```powershell
.\tools\benchmark\run-benchmarks.ps1 -BuildCompiler
```

Quiet mode (used by `web/server.mettle`):

```powershell
.\tools\benchmark\run-benchmarks.ps1 -Quiet
```

More runs or extra warmup:

```powershell
.\tools\benchmark\run-benchmarks.ps1 -Runs 7 -Warmup 2
```

## Compare Rust vs C

Rust counterparts live next to each C example (`fib.rs`, `grep.rs`, …) and use the same algorithms, workload sizes, and `Time: <N> us` output as the C builds.

```powershell
.\tools\benchmark\compare-rust.ps1
.\tools\benchmark\compare-rust.ps1 -Runs 7 -Benchmark grep,fib
```

Requires `rustc` and `gcc` on `PATH`. Results are written to `docs/benchmarks/rust-vs-c.json` with runtime, compile-time, and binary-size ratios (Rust relative to C).

## Compare Mettle Versions

Use the version comparison harness when codegen or IR work needs an old-vs-new
runtime answer instead of the Mettle-vs-C site data:

```powershell
.\tools\benchmark\compare-mettle-versions.ps1
```

By default it exports a clean copy of `HEAD`, builds that compiler as the old
version, builds the current working tree as the new version, compiles the
benchmarks from `docs/benchmarks/harness.json` with both compilers, then runs
each executable five times in alternating old/new order.

Useful narrower runs:

```powershell
.\tools\benchmark\compare-mettle-versions.ps1 -Runs 7 -Benchmark collatz,sum_squares
.\tools\benchmark\compare-mettle-versions.ps1 -OldCompilerPath C:\old\mettle.exe -NewCompilerPath .\bin\mettle.exe -SkipBuildCurrent
```

Outputs are written under `bin\benchmark-compare`:

- `mettle-version-summary.md`
- `mettle-version-summary.csv`
- `mettle-version-runs.csv`
- `mettle-version-compare.json`

## How It Works

1. Reads suites from `docs/benchmarks/harness.json`.
2. Compiles each Mettle and C binary (timed; same flags as the example `build.bat` scripts).
3. Records executable sizes on disk.
4. Warms up each binary, then runs it several times (default 5 runs, 1 warmup).
5. Parses `Time: <N> us` from output (QueryPerformanceCounter in the benchmarks).
6. Takes the median runtime per binary and reports runtime, compile-time, and size ratios vs C.
7. Writes results to `docs/benchmarks/latest.json` and mirrors to `web/benchmarks.json`.

## Server Integration

`web/server.mettle` serves `/benchmarks.json` from the harness output and refreshes the harness periodically (5-minute throttle), so benchmark pages fetch live harness data instead of static hand-edited JSON.

## Adding a Benchmark

Add a new entry in `docs/benchmarks/harness.json`:

```json
{
  "name": "my_bench",
  "description": "What this benchmark measures",
  "build_script": "examples/my_bench/build.bat",
  "mettle_exe": "examples/my_bench/my_bench.exe",
  "c_exe": "examples/my_bench/my_bench_c.exe"
}
```

The executable outputs should include a line matching `Time: <N> us` (microseconds from `std/bench` or `examples/bench_time.h`).
