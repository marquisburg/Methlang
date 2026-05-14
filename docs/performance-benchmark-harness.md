# Performance Benchmark Harness (Meth vs C)

This project ships a dedicated benchmark harness for comparing MethASM output against C implementations.

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

Quiet mode (used by `web/server.meth`):

```powershell
.\tools\benchmark\run-benchmarks.ps1 -Quiet
```

## How It Works

1. Reads suites from `docs/benchmarks/harness.json`.
2. Runs each suite `build.bat` (builds both Meth and C versions).
3. Executes both binaries and parses `Time: <N> ms` from output.
4. Computes `relative = meth_ms / c_ms` when C timing exists.
5. Writes results to `docs/benchmarks/latest.json` and mirrors to `web/benchmarks.json`.

## Server Integration

`web/server.meth` serves `/benchmarks.json` from the harness output and refreshes the harness periodically (5-minute throttle), so benchmark pages fetch live harness data instead of static hand-edited JSON.

## Adding a Benchmark

Add a new entry in `docs/benchmarks/harness.json`:

```json
{
  "name": "my_bench",
  "description": "What this benchmark measures",
  "build_script": "examples/my_bench/build.bat",
  "meth_exe": "examples/my_bench/my_bench.exe",
  "c_exe": "examples/my_bench/my_bench_c.exe"
}
```

The executable outputs should include a line matching `Time: <N> ms`.
