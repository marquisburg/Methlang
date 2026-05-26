# Differential miscompile fuzzer

Makes Mettle hard to miscompile silently. Generates random, UB-free programs,
builds each at **debug** and **release**, runs both, and flags any exit-code
divergence. Debug is the trusted oracle (the optimizer only runs at
`-O`/`--release`), so a divergence is a genuine miscompile, never generator UB.

## Quick start

```sh
# Build the compiler first (./build.bat), then:
python tools/fuzz/fuzz.py --count 500            # seeds 1..500
python tools/fuzz/fuzz.py --count 500 --start 1000
```

Failing seeds are written to `tools/fuzz/repros/<seed>.mettle` (transient; the
seed is in the file header, so regenerate any case with
`python tools/fuzz/genprog.py <seed>`).

## Files

- **genprog.py** — program generator. `main` computes an int64 accumulator,
  masked to 40 bits after every step so signed overflow is impossible. Targets
  the fragile optimizer/backend shapes: counted polynomial loops,
  `arr[i]=f(i)` write-then-readback, unsigned `>> / %`, nested if/else, small
  inlinable helpers.
- **fuzz.py** — the differential driver (the oracle above).
- **irexec.py** — reference interpreter for the `--dump-ir` sidecar. Run the
  **optimized** IR through it to split bug classes:
  - result == debug output  ⇒ optimized IR is correct ⇒ bug is in the
    **backend/codegen**.
  - result == release output (wrong) ⇒ bug is an **IR optimization pass**.
  `--trace` prints final `@var` values to find which variable diverges.
- **reduce.py** — ddmin source reducer (pins the debug exit code as invariant
  so reductions stay semantics-preserving). Slow; direct IR-diff is usually
  faster.

## Bisecting a miscompile to one pass

The compiler honors `METTLE_SKIP_PASS="14,16"` to disable numbered IR
optimization passes (the `DRIVE_PASS(N, ...)` indices in
`src/ir/ir_optimize.c`). Skip suspects until the divergence disappears to
localize the culprit pass.

## History

Built 2026-05-26. First sweep found ~13% of seeds miscompiling. Fixed three
distinct silent bugs (loop-unroller exit jump, backend symbol-alias coalescing,
empty-nested-if diamond removal); fuzzer now clean over 1000 seeds.
