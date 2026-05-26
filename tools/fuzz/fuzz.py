#!/usr/bin/env python3
"""Differential miscompile fuzzer for the Mettle compiler.

For each seed it generates a self-contained program (genprog.py), builds it at
debug and at release, runs both, and compares exit codes. Debug is the trusted
oracle: the optimizer only runs at -O/--release, so a debug-vs-release exit-code
divergence is a silent miscompile. A build failure at exactly one level, or a
crash (exit code indicating a fault) at one level only, is also flagged.

Repros are written to tools/fuzz/repros/<seed>.mettle for replay. The seed is
embedded in each program header, so any failing case is fully reproducible with
`python genprog.py <seed>`.

Usage:
  python fuzz.py [--count N] [--start S] [--compiler PATH] [--keep]
"""

import argparse
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import genprog  # noqa: E402

REPRO_DIR = os.path.join(HERE, "repros")
DEFAULT_COMPILER = os.path.join(HERE, "..", "..", "bin", "mettle.exe")


def build(compiler, src, out, release):
    args = [compiler, "--build", "--emit-obj", "--linker", "internal"]
    if release:
        args.append("--release")
    args += [src, "-o", out]
    p = subprocess.run(args, capture_output=True, text=True, timeout=120)
    return p.returncode, (p.stdout + p.stderr)


def run(exe):
    try:
        p = subprocess.run([exe], capture_output=True, text=True, timeout=30)
        return p.returncode, (p.stdout + p.stderr)
    except subprocess.TimeoutExpired:
        return "TIMEOUT", ""


def is_crash(code):
    # Windows fault exit codes are large negatives / 0xC0000005-style values.
    return isinstance(code, int) and (code < 0 or code > 255)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--count", type=int, default=200)
    ap.add_argument("--start", type=int, default=1)
    ap.add_argument("--compiler", default=DEFAULT_COMPILER)
    ap.add_argument("--keep", action="store_true",
                    help="keep generated source for every seed, not just failures")
    args = ap.parse_args()

    os.makedirs(REPRO_DIR, exist_ok=True)
    tmp = os.environ.get("TEMP", "/tmp")
    # PID-unique so multiple fuzz shards can run concurrently without
    # clobbering each other's source/executables.
    tag = os.getpid()
    src = os.path.join(tmp, f"mettle_fuzz_{tag}.mettle")
    dbg = os.path.join(tmp, f"mettle_fuzz_{tag}_dbg.exe")
    rel = os.path.join(tmp, f"mettle_fuzz_{tag}_rel.exe")

    failures = []
    for seed in range(args.start, args.start + args.count):
        prog = genprog.generate(seed)
        with open(src, "w") as f:
            f.write(prog)

        def save_repro(reason):
            path = os.path.join(REPRO_DIR, f"{seed}.mettle")
            with open(path, "w") as f:
                f.write(prog)
            failures.append((seed, reason))
            print(f"  [FAIL seed={seed}] {reason}  -> {path}")

        dbg_bc, dbg_blog = build(args.compiler, src, dbg, release=False)
        rel_bc, rel_blog = build(args.compiler, src, rel, release=True)

        if dbg_bc != 0 and rel_bc != 0:
            # Both reject -> likely a generator bug, not a miscompile. Skip noisily.
            print(f"  [skip seed={seed}] both builds failed (generator issue?)")
            continue
        if (dbg_bc == 0) != (rel_bc == 0):
            save_repro(f"build divergence: debug rc={dbg_bc}, release rc={rel_bc}")
            continue

        dbg_rc, _ = run(dbg)
        rel_rc, _ = run(rel)

        if dbg_rc != rel_rc:
            save_repro(f"EXIT DIVERGENCE: debug={dbg_rc}, release={rel_rc}")
        elif is_crash(dbg_rc) and is_crash(rel_rc):
            save_repro(f"both crash (rc={dbg_rc}) -- possible UB in generator")
        elif args.keep:
            with open(os.path.join(REPRO_DIR, f"ok_{seed}.mettle"), "w") as f:
                f.write(prog)

        if (seed - args.start + 1) % 25 == 0:
            print(f"  ...{seed - args.start + 1}/{args.count} done, "
                  f"{len(failures)} failures")

    print(f"\nDone. {len(failures)} failure(s) out of {args.count} seeds.")
    for seed, reason in failures:
        print(f"  seed {seed}: {reason}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
