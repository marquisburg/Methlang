# Codegen improvements for grep-style hot loops

The IR side is now in good shape — `pattern_matches` is inlined into `grep_count`,
the null-check diamond is hoisted out of the loop, and the call cap keeps
inlining sensible. The remaining gap vs C (~1.65× on grep) is entirely a
**codegen quality** problem in the COFF backend (`src/codegen/code_generator_*.c`,
NOT the assembly emitter).

This document is a punch list of concrete improvements, ranked roughly by
expected win. Read with `objdump -d bin/grep_perf.obj` of a `-O` build for
context; line numbers below reference the dump from the post-inlining build
of `examples/grep/grep.mettle`.

---

## Background — what the inner loop currently looks like

The hot loop in `grep_count` (offsets `8c3`–`aba` of grep_perf.obj) executes
once per input byte, ~1 MB × 2000 passes = 2 G iterations. Per iteration it
runs ~80 instructions, most of which are stack shuffles. The actual *useful*
work per byte is:

```
movzbl (buf+i), c       ; 1 load
cmp c, '\n'             ; 1 cmp
cmp c, 'E'              ; 1 cmp
... fast path branches ...
mov rax, *(buf+i)       ; 1 load (8 bytes, only on 'E')
and rax, mask           ; 1 and
cmp rax, pattern        ; 1 cmp
```

That's ~7 useful instructions per loop iteration. The current codegen emits
~80. Every extra instruction is a register/stack round-trip the optimizer
should have collapsed.

---

## Issue 1 — every IR temp gets its own stack slot

**Severity:** highest. This is the root cause of most other issues.

The IR-to-x86 lowering treats every `%tNN` SSA temp as a stack location. Look
at the read-byte sequence (offsets `8cc`–`8e2`):

```
mov r10, r15            ; load i
mov rax, r12            ; load buf
lea rax, (rax,r10,1)    ; rax = buf+i
movzbl eax, (rax)       ; load byte
mov [rbp-0x30], rax     ; spill %t114
mov rax, [rbp-0x30]     ; immediately reload it
cmp rax, 0xa            ; the cmp we wanted
```

Lines 4 and 5 are pure waste. The temp `%t114` is **used exactly once**
(by the immediately following `cmp`) yet gets stored and reloaded.

The same pattern appears at every `binary` / `load` / `cast` in the loop:
spill the result of a unary computation, then immediately reload it for the
single consumer.

### What's there now

`src/codegen/code_generator_ir.c` lines ~1100–1220 have a "deferred-spill
peephole" plus `pending_spill_*` state and a `rax_cached_temp_offset`. This
is doing one-instruction lookahead — when a load's destination matches a
just-spilled offset, the spill is dropped. That works for trivially-adjacent
patterns but doesn't survive even a 1-instruction reordering (e.g., a `cmp`
between the spill and the use), and it only operates on `rax`.

### What to do

Add a **last-use map** to the IR-level codegen pass. Before lowering, walk
each function once and record, for every `%tNN`:

- `def_index` — the IR index that produced it
- `last_use_index` — the IR index of its final consumer
- `use_count` — total reads

Then in the per-instruction emit:

1. If `use_count == 1` AND `last_use_index == def_index + 1` (or all
   intervening instructions are NOPs/labels), **don't spill at all**. Leave
   the value in `rax` and have the consumer read `rax` directly.
2. If `use_count == 1` AND the temp is consumed before any other temp is
   produced into `rax`, same — skip the spill.
3. Otherwise spill as today.

Files: read in `src/codegen/code_generator_ir.c` (the IR walker). The
last-use scan goes alongside the existing prologue setup; one extra pass over
`function->instructions` per function.

Expected win on grep: ~30–40% reduction in instruction count in the inner
loop. Should be the single biggest gain.

---

## Issue 2 — local variables (`@symbol`) are pinned to memory

**Severity:** high.

Globals like `@buf`, `@i`, `@found`, `@count`, `@c` show up in the IR as
`IR_OPERAND_SYMBOL` and are always emitted as `mov rax, [rbp-N]`. Look at
offsets `8c3`–`8c6`:

```
cmp r13, r15            ; compares len (in r13) to i (in r15)  ← good
```

…but earlier at `8cc`:

```
mov r10, r15            ; load i  — already in r15!
mov rax, r12            ; load buf — already in r12!
lea rax, (rax,r10,1)
```

`i` is in `r15`, `buf` is in `r12`, and the codegen *knows* this (it just
used them in the cmp). But for the address calculation it re-loads them
through generic temp-handling.

### What to do

Promote address-not-taken locals to **virtual registers** at the IR level
(or track them as such in the codegen). The codegen already has *some*
caching (look at `code_generator_binary.c` lines ~1100 and the
`rax_cached_temp_offset` field), but it's tied to RAX only and resets on
every emit.

Concrete approach:

1. Before lowering each function, scan for symbols with `address_taken == 0`
   (helper already exists in `src/ir/ir_optimize.c` as
   `ir_symbol_address_taken` — copy/move it into codegen).
2. Pick up to 8 such symbols (whatever fits in the non-volatile registers
   the function preserves: r12–r15, rbx, rsi, rdi if applicable) and assign
   each one a dedicated home register.
3. Every load/store of that symbol becomes a register move, not a memory op.
4. Spill on call boundaries (the caller-save split is already done; you just
   need to spill the live homed-registers across CALL instructions).

`@i`, `@buf`, `@len`, `@found`, `@count`, and `@c` in grep_count all qualify.
That's 6 symbols → easily fits in r12–r15 + rbx + one more.

Files: `src/codegen/code_generator_ir.c` (symbol lookup) and
`src/codegen/code_generator_variables.c`. The prologue/epilogue already
spills r12–r15+rbx (offsets `85b`–`877`).

Expected win on grep: stacking with Issue 1, gets close to C.

---

## Issue 3 — globals reloaded from RIP every iteration

**Severity:** medium.

Offsets `93e` and `9d4`/`9e2`:

```
mov r10, [rip+...]      ; load @pattern_len from .data
...
mov r10, [rip+...]      ; load @pattern_mask
mov r10, [rip+...]      ; load @pattern_u64
```

These globals are constant for the whole program (`var pattern_u64: uint64 = 0x524F525245;`
is never reassigned). Loading them from memory every iteration costs an L1 hit
each (free in cycles but more importantly, more bytes of icache).

### What to do

This is really an IR-level optimization that the codegen can take advantage of:

1. In the IR optimizer, mark global symbols whose only writes are at program
   initialization (or whose declarations are effectively `const`).
2. The codegen looks up the symbol, sees the initializer is a known integer
   constant, and emits `mov reg, 0x524F525245` (an `imm64`) instead of a
   memory load.

For symbols whose value is too big for `imm32` but the codegen knows the
const at lowering time, a literal load is no worse than a memory load — and
it avoids the indirection.

Even simpler: **after Issue 2 is done**, the symbol homes to a register at
loop entry. The codegen loads the global into the home register once at
function entry, and the loop reuses it.

Files: `src/codegen/code_generator_ir.c` symbol-reference handling.

Expected win on grep: small (~5%). Bigger wins on benchmarks that read more
globals.

---

## Issue 4 — `branch_zero %t` re-tests a value already in flags

**Severity:** low-medium.

Pattern at offsets `9ec`:

```
cmp r10, rax            ; (val & mask) vs pattern  → sets ZF
jne ...                 ; ← branches on ZF directly — good!
```

But many places do:

```
cmp rax, 0xa            ; sets ZF
jne ...                 ; good
...
mov rax, [rbp-0x30]     ; reload result
test rax, rax           ; sets ZF again
jne/je ...
```

The `branch_zero %t` IR op forces the codegen to emit `mov ... ; test ... ;
jXX`. If the value being tested is the result of an immediately-prior `cmp`,
the test is redundant — flags are already set.

### What to do

In the IR-to-codegen emit, when emitting `branch_zero %t`:

1. Check whether `%t`'s producer is a comparison `binary` (==, !=, <, >, <=, >=).
2. If yes, and `%t` is single-use and consumed immediately, **skip the
   spill of the producer** AND **skip the test** — emit the inverted jump
   directly on the cmp's flags.

This is conceptually a peephole over (compare → spill → branch_zero) and
already partially exists — see
`code_generator_binary_try_emit_binary_compare_branch_chain` in
`code_generator_binary.c:7987`. But it's restricted to specific shapes and
doesn't cover the inlined cases.

Files: `code_generator_binary.c:7987` (extend the existing chain).

Expected win on grep: small (~5%), but cumulative with Issue 1.

---

## Issue 5 — call-site stack alignment dance is needlessly verbose

**Severity:** low.

Every CALL site emits:

```
sub rsp, 0x20           ; shadow space
call ...
add rsp, 0x20           ; restore
```

This is correct Win64 ABI but on x64, the shadow space could be allocated
**once in the prologue** for the function as a whole (`sub rsp` is constant-
sized — just add 32 to the frame size). Then every call is one `call`
instruction instead of three.

The current pattern wastes 6 bytes × 3 = 18 bytes of code per call, plus
stalls on the rsp adjustment.

In grep this doesn't matter much (the hot loop has only 1 call site that's
on the cold path), but for benchmarks that call into stdlib (word_count,
print-heavy code) the savings add up.

### What to do

In the prologue (`code_generator_stack.c`), increase the `sub rsp, frame`
amount by `32` (or `8 * max(argc, 4)` for the most-arg-heavy call in the
function). Skip emission of the per-call `sub rsp,0x20 / add rsp,0x20` pair.

Files: `src/codegen/code_generator_stack.c`, `src/codegen/code_generator_calls.c`.

Expected win on grep: ~2%. Bigger on call-heavy benchmarks.

---

## Issue 6 — `lea` is doing nothing useful

**Severity:** trivial / cleanup.

Offsets `8a5`–`8b3` (the hoisted null-check trampoline that LICM put before
the loop):

```
lea rcx, [rip+0]        # 8ac  ← reads its own address as a "format string"
lea rdx, [rip-0xe]      # 8a5  ← reads the trampoline's own start
mov r8, rbp
sub rsp, 0x20
call mettle_crash_trap
```

The `lea`s with `[rip+0]` and `[rip-0xe]` are clearly bogus operand
resolution — they should be loading the address of the null-deref error
string. The arguments are unrelocated. The trap call is currently a no-op
that just trashes flags.

This is reachable only when `buf` is null (which never happens in the
benchmark), so it doesn't affect runtime — but it's a latent bug: when a
null pointer DOES come through, the trap will print garbage.

### What to do

Audit `mettle_crash_trap` call lowering — the symbolic string argument
isn't being relocated. Should produce a proper R_X86_64_REL32 relocation to
the rdata string.

Files: `src/codegen/code_generator_ir.c` (trap-call emission),
`src/codegen/binary_emitter.c` (relocation table — already understands
ADDR32NB/REL32).

Expected win on grep: 0 (cold path). But fix before any user hits a null deref.

---

## Suggested order

1. **Issue 1 (last-use map / skip-spill)** — biggest win, isolated change in
   one file. Do this first.
2. **Issue 2 (symbol-to-register promotion)** — biggest second win, but
   touches more files. After Issue 1 is in, this gets you most of the way to
   parity with C.
3. **Issue 4 (compare→branch fusion extension)** — easy, completes the inner
   loop quality.
4. **Issue 5 (single-prologue shadow space)** — cleanup, helps call-heavy code.
5. **Issue 3 (const-global promotion)** — small win unless Issue 2 doesn't
   land cleanly.
6. **Issue 6 (trap relocation)** — correctness fix, not a perf item.

---

## How to measure

```powershell
.\tools\benchmark\run-benchmarks.ps1 -Runs 7 -Warmup 2
```

System is noisy; the median over 7 runs after 2 warmups is the only reading
worth quoting. Look at `relative` for `grep` in `docs/benchmarks/latest.json`.

Baseline (post-IR-work, pre-codegen-work): **grep ≈ 1.65× of C**.
Goal: **≤ 1.05× of C**.

For sanity-checking individual loop instruction counts:

```bash
objdump -d bin/grep_perf.obj | awk '/^[0-9a-f]+ <grep_count>:/{flag=1} flag{print} /^[0-9a-f]+ <[a-z]/ && !/<grep_count>/{flag++; if(flag>2)exit}' | wc -l
```

Currently ~158 lines. C's grep_count is ~30 instructions. Anything under 50
gets us there.
