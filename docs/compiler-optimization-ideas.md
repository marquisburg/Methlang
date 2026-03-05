# Compiler Optimization Ideas

This document outlines optimizations you could implement in the Methlang compiler to improve generated code quality, especially for hot loops like `grep_count`.

## Current Optimization Pipeline

From `src/ir/ir_optimize.c`, the compiler runs (with `-O` / `--release`):

1. **ir_inline_small_functions_pass** – Inlines small functions
2. **ir_copy_and_constant_propagation_pass** – Propagates constants and copies
3. **ir_coalesce_single_use_temp_assign_pass** – Coalesces single-use temps
4. **ir_common_subexpression_elimination_pass** – CSE within basic blocks only
5. **ir_constant_and_branch_simplify_pass** – Integer folding, branch simplification
6. **ir_eliminate_dead_temp_writes_pass** – Dead write elimination
7. **ir_remove_redundant_jumps_pass** – Jump cleanup
8. **ir_eliminate_unreachable_straightline_pass** – Unreachable code removal

These run in a fixed-point loop (up to 8 iterations) until no changes.

---

## High-Impact Optimizations for grep_count

### 1. CSE Across Basic Blocks (Global CSE)

**Current:** CSE clears its expression map on every `LABEL`, `JUMP`, `BRANCH_*`, `RETURN`. So CSE only works within straight-line code.

**Problem:** In `grep_count`, expressions like `i + pattern_len`, `buf + i`, and `i + 8 <= len` are computed in the loop body. Each iteration may cross block boundaries (e.g. after a branch). The same computation is repeated.

**Implementation:**
- Build a CFG (control-flow graph) from IR labels/jumps/branches
- Use dominance analysis: if block B dominates block C, expressions computed in B are available in C
- Extend `ir_common_subexpression_elimination_pass` to:
  - Track expressions per block, not just within a block
  - When entering a block, merge available expressions from dominators
  - On merge points (multiple predecessors), only keep expressions available on all paths
- Alternatively: use SSA form and GVN (Global Value Numbering) for a cleaner approach

**Files:** `src/ir/ir_optimize.c` – extend or replace `ir_common_subexpression_elimination_pass`

---

### 2. Loop-Invariant Code Motion (LICM)

**Current:** No LICM. Loop-invariant computations are repeated every iteration.

**Problem:** In `grep_count`:
- `pattern_len` (5) and `len` (262144) are loop invariants
- `i + pattern_len <= len` – the `pattern_len` and `len` parts could be hoisted
- `pattern_u64` is a constant – already hoisted by constant propagation
- The condition `i + 8 <= len` – `len` is invariant

**Implementation:**
- Identify loops (back-edges in CFG)
- For each instruction in the loop body:
  - If all operands are constants or defined outside the loop (or in earlier loop iterations only), the instruction is loop-invariant
  - Move it to the loop preheader (block that dominates the loop header and has only the loop as successor)
- Must be careful: loads (`IR_OP_LOAD`) are only invariant if the address doesn’t change in the loop. `buf + i` changes with `i`, so loads from it are not invariant. But `len` is a parameter – invariant.

**Files:** `src/ir/ir_optimize.c` – new pass `ir_loop_invariant_code_motion_pass`

---

### 3. Induction Variable Simplification / Strength Reduction

**Current:** The loop uses `i` from 0 to len-1. Each iteration computes `buf + i`, `i + pattern_len`, `i + 8`.

**Problem:** Repeated address computation. Could use a pointer that increments instead of `i`.

**Implementation:**
- Detect induction variables: `i` starts at 0, increments by 1 each iteration
- For `buf + i`: replace with a pointer `ptr` that starts at `buf` and increments by 1 (or by element size)
- Reduces: `buf[i]` → `*ptr`; `buf + i` → `ptr`; `ptr++` instead of `i++`
- Requires updating the loop condition: `i < len` → `ptr < buf_end` where `buf_end = buf + len`

**Files:** `src/ir/ir_optimize.c` – new pass, or integrate into LICM/loop optimization

---

### 4. Extend CSE to LOAD/ADDRESS_OF

**Current:** `ir_instruction_is_cse_candidate` excludes `LOAD` and excludes `*` (dereference) from UNARY. So loads are never CSE’d.

**Problem:** If we load `buf[i]` and use it, and the compiler could prove no store between, a second load of `buf[i]` could reuse the first. In practice, loads are often not CSE’d because of aliasing.

**Implementation:**
- Add LOAD to CSE candidates, but only when:
  - The address operand is a temp/symbol (not a complex expression that might alias)
  - No STORE or CALL between the two loads (conservative: clear on any store/call)
- For `ADDRESS_OF` with non-symbol (e.g. `&buf[i]`), currently excluded. Could allow CSE when the address computation is identical.

**Files:** `src/ir/ir_optimize.c` – `ir_instruction_is_cse_candidate`, `ir_expression_map_*`

---

### 5. Loop Unrolling

**Current:** No loop unrolling.

**Problem:** 262K iterations – each has loop overhead (increment, compare, branch).

**Implementation:**
- Detect simple counted loops: `i = 0; while (i < n) { body; i++; }`
- Unroll by 2, 4, or 8: duplicate the body, adjust the increment and bound
- Reduces branch and increment overhead
- Risk: code size growth; may hurt I-cache

**Files:** `src/ir/ir_optimize.c` – new pass `ir_loop_unroll_pass`

---

### 6. Peephole Optimizations (Assembly-Level)

**Current:** The docs mention "control-flow/codegen branch peepholes" but the codegen may not have many.

**Implementation:**
- After emitting assembly, scan for patterns:
  - `cmp X, 0` followed by `jz` → `test X, X` followed by `jz` (shorter)
  - Redundant moves: `mov rax, rbx` then `mov rcx, rax` → `mov rcx, rbx`
  - Dead code after unconditional jump
- Or do this at IR level: eliminate `ASSIGN x <- y` when `x` is only used once and we can forward `y` instead

**Files:** `src/codegen/code_generator_ir.c` or a new peephole pass

---

### 7. SSA Form and Optimizations

**Current:** IR is in non-SSA form (temps can be assigned multiple times).

**Implementation:**
- Convert to SSA (insert φ nodes at merge points)
- Enables:
  - Simpler and more precise constant propagation
  - Better dead code elimination
  - Cleaner CSE/GVN
- After optimizations, convert back from SSA (insert copies at φ nodes)

**Files:** New `src/ir/ir_ssa.c`, integrate into `ir_optimize.c`

---

## Implementation Order (Suggested)

1. **LICM** – Directly hoists `len`, `pattern_len`, and related conditions. Moderate effort, clear benefit.
2. **CSE across blocks** – Requires CFG and dominance. High effort, high benefit for nested conditionals.
3. **Induction variable / strength reduction** – Good for pointer-bump loops. Medium effort.
4. **Extend CSE to LOAD** – Lower risk, incremental. Add with conservative aliasing.
5. **Loop unrolling** – Easy for simple loops. Tune factor (2, 4, 8) empirically.
6. **SSA** – Large change. Consider after the above.

---

## Debugging / Validation

- Use `--perf-report -O` on `examples/grep/grep.meth` to compare before/after
- Emit IR with `-o out.s.ir` (if supported) to inspect IR changes
- Compare assembly output: `methlang -O grep.meth -o before.s` vs after your pass
- Run `tests/run_tests.ps1` to avoid regressions
- Use `tools/perf/compare-perf-runs.ps1` for benchmark comparison

---

## IR and Codegen Reference

- **IR ops:** `ir.h` – ASSIGN, LOAD, STORE, BINARY, UNARY, ADDRESS_OF, CALL, BRANCH_*, JUMP, etc.
- **Optimization entry:** `ir_optimize_program` in `ir_optimize.c`
- **Pass structure:** Each pass returns 1 on success, sets `*changed` if it modified IR
- **Fixed-point:** Passes run in sequence; the whole sequence repeats until no changes (max 8 iterations)
