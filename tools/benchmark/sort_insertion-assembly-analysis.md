# sort_insertion: Mettle vs GCC codegen analysis

**Status:** was 2.79x slower than C; after the register-pressure fix below,
2.55x. The hot inner loop is now fully register-resident (matches GCC's
register usage). Remaining gap is `mov rax,rXX` shuffle overhead, not memory
traffic. See "What was changed" and "What remains" at the end.

## How this was produced

```
bin/mettle.exe --build --emit-obj --linker internal --release \
    examples/sort_insertion/sort_insertion.mettle -o .tmp/sort_mettle.obj
gcc -O3 -c -o .tmp/sort_c.o examples/sort_insertion/sort_insertion.c
objdump -d -M intel .tmp/sort_mettle.obj > .tmp/sort_mettle.asm.txt
objdump -d -M intel .tmp/sort_c.o        > .tmp/sort_c.asm.txt
```

The harness already supports this snapshot path: set `track_asm: true` on the
benchmark in `docs/benchmarks/harness.json` and `Export-MettleAsmSnapshot`
(run-benchmarks.ps1:332) writes `.tmp/bench-asm/sort_insertion_mettle.asm.txt`.
It only disassembles the *Mettle* side — there is no C-side snapshot, which is
the first harness gap (see "Harness improvements" below).

The hot code is the inner `while (j >= 0)` loop of `insertion_sort`. Both the
fill, copy (`memcpy`), and `sum_array` are negligible relative to the O(n²)
sort body across 200 passes.

## The two inner loops side by side

### GCC `-O3` (everything inlined into `main`, `main+0x180`)

```asm
180:  mov    ecx,[rax-0x4]   ; current = data[j]
183:  cmp    r8d,ecx         ; key vs current
186:  jge    280             ; if key >= current -> break (taken path inserts)
18c:  sub    edx,0x1         ; j--
18f:  mov    [rax],ecx       ; data[j+1] = current
191:  sub    rax,0x4         ; scan pointer -= 4
195:  cmp    edx,-1          ; j >= 0 ?
198:  jne    180
```

**8 instructions, zero memory traffic except the two unavoidable array
accesses.** `key` lives in `r8d`, `j` in `edx`, the scan pointer in `rax`,
all in registers for the whole loop. The whole `insertion_sort` + `sum_array`
were inlined into `main`, and `sum_array` was even **SSE2 vectorized**
(`movdqu`/`paddq`, `main+0x1c0`).

### Mettle `--release` (`insertion_sort` at `0x140001730`, inner loop `0x14000179f`)

```asm
14000179f:  test   r14,r14                       ; j >= 0 ?
1400017a2:  jl     0x1400017f0
1400017a8:  mov    rax,r13                        ; scan
1400017ab:  sub    rax,0x4                        ; prev = scan - 4
1400017af:  mov    rdi,rax
1400017b2:  mov    rax,rdi
1400017b5:  mov    eax,DWORD PTR [rax+0x0]         ; current = *prev
1400017b8:  movsxd rax,eax                         ; sign-extend (spurious)
1400017bb:  mov    QWORD PTR [rbp-0x48],rax        ; SPILL current to stack
1400017bf:  mov    rax,QWORD PTR [rbp-0x48]        ; immediately RELOAD it
1400017c3:  cmp    rax,rsi                         ; current vs key
1400017c6:  jg     0x1400017d1
1400017cc:  jmp    0x1400017f0                     ; break
1400017d1:  mov    rcx,QWORD PTR [rbp-0x48]        ; reload current AGAIN
1400017d5:  mov    rax,r13                         ; scan
1400017d8:  mov    DWORD PTR [rax+0x0],ecx         ; *scan = current
1400017db:  mov    rax,rdi                         ; scan = prev
1400017de:  mov    r13,rax
1400017e1:  mov    rax,r14                         ; j
1400017e4:  sub    rax,0x1                         ; j--
1400017e8:  mov    r14,rax
1400017eb:  jmp    0x14000179f
```

**~20 instructions per iteration vs GCC's 8**, with a stack spill/reload of
`current` *inside* the hot loop. That alone roughly accounts for the 2.79x.

## Specific codegen deficiencies (ranked by impact)

### 1. Spill/reload of a live temporary inside the loop — biggest cost
`current` is computed into `rax`, sign-extended, then **stored to `[rbp-0x48]`
and reloaded twice** (`0x1400017bb`, `0x1400017bf`, `0x1400017d1`). The value
never needed to leave a register. This is a register-allocation /
copy-propagation failure: the IR materializes a named local for `current` and
the backend honors it as a stack slot rather than keeping it in a register
across its (short) live range. Two dependent memory ops per iteration on the
critical path.

### 2. No inlining of `insertion_sort` / `sum_array` into the bench loop
GCC inlined both into `main`, which (a) removed call overhead and (b) enabled
vectorizing `sum_array`. Mettle keeps them as `call`s. The bench loop runs
`copy_array` + `insertion_sort` + `sum_array` 200x; each is a real call with
prologue/epilogue. Per the inliner notes in memory ([[grep-inline-and-licm-perf]]),
the inliner exists but is gated — these functions exceed whatever size/shape
gate is in effect. The sort itself is the dominant cost, so inlining it matters
less than #1, but `sum_array` not being inlined+vectorized is free perf left on
the table.

### 3. Redundant `mov rax, rXX` shuffles ("everything routes through rax")
`0x1400017af`–`0x1400017b2` is `mov rdi,rax; mov rax,rdi` — a literal no-op
pair. Similarly `mov rax,r13` / `mov rax,rdi` / `mov rax,r14` appear before
nearly every use. The backend computes into `rax` then copies to/from named
registers instead of operating in place. A peephole/copy-coalescing pass would
delete most of these.

### 4. Spurious `movsxd` sign-extension on every `int32` load
`0x1400017b8 movsxd rax,eax` after each `*prev` load. The Mettle source types
`current` as `int32`, and it is only compared and stored back as 32-bit. The
sign-extend to 64-bit is dead — GCC keeps it in `ecx` with no extension. Comes
from promoting the loaded value to the IR's 64-bit integer width and never
narrowing back.

### 5. `int32` loop counters / pointer math done in 64-bit
`i`/`j` are `int32` in the source but the codegen uses 64-bit `r14`/`r15` with
64-bit `sub`/`cmp`. Not a correctness issue here and not on the critical path,
but it's symptomatic of the same "promote everything to 64-bit" tendency that
produces #4.

### 6. Frame churn at every call site (affects the per-pass call overhead)
Every `call` in this file is bracketed by `sub rsp,0x20` / `add rsp,0x20`
(shadow space) emitted *per call* rather than once in the prologue — e.g.
`0x1400012d7`/`0x1400012e5`. GCC reserves the max outgoing arg area once. Minor
here, but it inflates the non-inlined call path in the bench loop.

## What GCC does that we don't (summary)

| Technique | GCC | Mettle |
|---|---|---|
| Keep loop temps in registers | yes | **spills `current` to stack** |
| Inline leaf functions into hot loop | yes (sort + sum) | no |
| Vectorize `sum_array` reduction | yes (SSE2 `paddq`) | no |
| Copy coalescing / peephole | yes | no (many `mov rax,rXX`) |
| Narrow int32 ops to 32-bit | yes | no (`movsxd` + 64-bit math) |
| Reserve shadow space once | yes | per-call `sub/add rsp,0x20` |

## Highest-leverage fixes (for a later change)

1. **Register-allocate short-lived temporaries / copy-propagate IR locals**
   so values like `current` aren't round-tripped through the stack inside
   loops. This is the single biggest win and likely helps every benchmark, not
   just this one. Touches the IR -> backend register handling in
   `src/codegen/code_generator_binary.c` and the value/temp model in
   `src/ir/ir.c`.
2. **A peephole pass to delete redundant `mov rax,rXX` / `mov rXX,rax`
   pairs and dead `movsxd`.** Cheap, broadly applicable, low risk.
3. **Relax the inliner gate for small leaf functions** (`insertion_sort`,
   `sum_array`) — see existing gating in [[grep-inline-and-licm-perf]].
4. (Stretch) **32-bit operand selection** for `int32`-typed values to drop the
   sign-extends and shrink encodings.

Note from [[vectorizer-design]]: no benchmaxxing — any fix must be a general
codegen improvement, not a pattern-match on this kernel.

## What was changed

**Register-pressure fix (general, shipped).** In
`code_generator_binary_build_loop_weights` the per-instruction hotness weight
was a flat `4` for any loop body, regardless of nesting depth. So an
innermost-loop value (e.g. the insertion-sort `current`/`prev`/`scan`) tied
with every outer-loop variable and lost the 7-register promotion contest —
`main`'s inlined sort spilled *all* eight loop values to the stack, round-
tripping each through memory every iteration. Changed the weight to compound
multiplicatively per nesting level (`*= 4` per enclosing back-edge range, ~4^depth,
capped to avoid overflow). Inner-loop values now dominate the score and get
promoted first. Result: the hot inlined inner loop went from ~8 stack
loads/stores per iteration to **zero** — `current`/`prev`/`scan`/`key`/`j` all
live in r12-r15/rbx/rsi/rdi, exactly like GCC.

This is a general scoring fix, not a kernel pattern-match: any nested hot loop
benefits. Full suite: 252/252 tests pass; no runtime regressions (other
benchmarks within run-to-run noise).

**Investigated and rejected: removing the spurious `movsxd`.** The int32 LOAD
sign-extend looked redundant, but it is *load-bearing*: `emit_cast` for
int32->int64 (`target_size==8`) does NOT re-extend — it trusts the int32 slot
already holds a sign-extended value. Dropping the LOAD-time `movsxd` would
miscompile `(int64)negative_int32`. The existing `symbol_table_lookup`-based
"skip extend for int32 dest" path never fires for locals (lookup returns NULL
for backend-stage local names), which is why it's currently safe — "fixing" it
to fire would introduce the miscompile. Left as-is intentionally.

## What remains (next, higher-risk)

The inner loop is now memory-clean but still ~14 insns vs GCC's 8 because every
binary op routes through RAX: `mov rax,rXX; <op> rax; mov rYY,rax` instead of
operating in place (`<op> rYY,rXX`) or using `lea`. Coalescing these
`mov rax,rXX` shuffles in `code_generator_binary_emit_binary` is the next win
but touches the hot path of every function — defer behind its own full-suite
validation. Also a candidate: the inner-loop `jg target; jmp else` branch pair
(line ~1093) vs GCC's single `jge`.

## Harness improvements to support this kind of analysis

`tools/benchmark/run-benchmarks.ps1` currently:
- snapshots **only** the Mettle disassembly (`Export-MettleAsmSnapshot`,
  line 332); there is no C-side equivalent, so every comparison like this one
  must be done by hand. Add a parallel `Export-CAsmSnapshot` that runs
  `gcc -O3 -S` (or `objdump` on the C obj) into `.tmp/bench-asm/<name>_c.asm.txt`
  when `track_asm` is set.
- has no per-function timing or instruction-count metric. Consider recording
  `mettle_exe_bytes` is already there; an optional `.text` instruction count
  (objdump line count) per benchmark would catch codegen regressions before
  they show up as wall-clock noise.
- `sort_insertion` does not have `track_asm: true` set in
  `docs/benchmarks/harness.json` — enabling it would make the Mettle snapshot
  automatic on every run.
```