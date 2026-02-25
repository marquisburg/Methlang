# Pull Request: Implement IR (Intermediate Representation) into Main

## Summary

Introduces a real compiler IR stage between semantic analysis and assembly emission. The main backend path for function bodies is now **IR-first**: AST is lowered to linear IR after type checking, and code generation consumes IR instructions to emit x86-64 assembly. This provides a stable seam for future optimization and backend retargeting.

**Branch:** `development` → `main`

---

## Motivation

Previously, the code generator walked the AST directly to emit assembly. This PR:

- **Decouples** the semantic representation from the backend, enabling future optimization passes and alternative targets.
- **Explicitly models** control flow (labels, jumps, branches) and data flow (temps, symbols, literals) in a linear instruction stream.
- **Improves maintainability** by splitting lowering logic from emission logic and modularizing the codegen pipeline.

---

## Changes Overview

### New Components

| Component | Description |
|-----------|-------------|
| `src/ir/ir.h` | IR data model: `IRProgram`, `IRFunction`, `IRInstruction`, `IROperand`, opcodes |
| `src/ir/ir.c` | IR construction, destruction, and dump utilities |
| `src/ir/ir_lowering.c` | AST-to-IR lowering (expressions, statements, control flow) |
| `src/codegen/code_generator_ir.c` | IR-to-assembly emission for function bodies |

### Refactored Codegen

The monolithic `code_generator.c` was split into focused modules:

- `code_generator_calls.c` — function and method call emission
- `code_generator_flow.c` — control flow (if/while/for/switch) and IR function dispatch
- `code_generator_ops.c` — binary/unary operations, load/store
- `code_generator_stack.c` — stack frame management
- `code_generator_variables.c` — local/global variable handling
- `code_generator_inline_debug.c` — debug symbol and line mapping
- `code_generator_internal.h` — shared internal declarations

### Build System

- **Makefile:** Added `IR_SOURCES` and `obj/ir` directory; IR sources compiled into the build.
- **build.bat:** Added IR compilation step and `obj\ir` directory creation.

### Compilation Pipeline (`main.c`)

- After semantic analysis, `ir_lower_program()` lowers the AST to `IRProgram`.
- `code_generator_set_ir_program()` passes the IR to the code generator.
- In debug or optimize mode (`-d` or `-O`), IR is dumped to `<output>.ir` for inspection.

---

## IR Design

### Structure

- **IRProgram** → **IRFunction** → **IRInstruction**
- Operands: temps, symbols, literals (int/float/string), labels
- Explicit control-flow: `label`, `jump`, `branch_zero`, `branch_eq`

### Opcodes

| Opcode | Purpose |
|--------|---------|
| `label`, `jump`, `branch_zero`, `branch_eq` | Control flow |
| `declare_local`, `assign` | Local variables |
| `address_of`, `load`, `store` | Memory operations |
| `binary`, `unary` | Arithmetic/logic |
| `call`, `new` | Function calls, heap allocation |
| `return`, `inline_asm` | Returns, inline assembly |
| `eval_expr`, `ast_stmt` | Fallback for unsupported constructs |

### What It Is

- Function-level linear IR
- Lowered from AST after type checking
- Consumed by codegen function-by-function
- Dumpable to `<output>.ir` in debug/optimize mode

### What It Is Not

- **Not SSA** — temps are mutable storage slots, no phi nodes
- **Not machine IR** — no register allocation at IR level
- **Not fully normalized** — some constructs still use `ast_ref` fallback (`eval_expr` / `ast_stmt`)

---

## Backend Coverage

- Control flow (`if`/`while`/`for`/`switch`/`break`/`continue`) emitted from IR
- Local declarations, assignment, branches, labels, returns emitted from IR
- Memory operations (`addr_of`, `load`, `store`) for struct fields, pointer dereference, indexed access
- Heap allocation (`new`) modeled as explicit IR
- Integer binary/unary ops lowered to pure IR when backend-supported
- Function calls emitted from IR; method/object calls may still fall back where not normalized
- Floating-point or unsupported operator shapes use safe fallback paths

---

## Testing

- Existing test suite (`tests/run_tests.ps1`) validates compiler positives, negatives, assembly syntax, and GC runtime.
- IR implementation preserves behavior; no new test failures expected.
- Manual smoke tests (e.g. `test_gc_alloc.masm`, `test_control_flow.masm`, etc.) should pass.

**Recommended:** Run full suite before merge:

```powershell
.\tests\run_tests.ps1 -BuildCompiler
```

---

## File Changes Summary

| Category | Files |
|----------|-------|
| **New** | `src/ir/ir.h`, `src/ir/ir.c`, `src/ir/ir_lowering.c`, `src/codegen/code_generator_ir.c`, `src/codegen/code_generator_calls.c`, `src/codegen/code_generator_flow.c`, `src/codegen/code_generator_ops.c`, `src/codegen/code_generator_stack.c`, `src/codegen/code_generator_variables.c`, `src/codegen/code_generator_inline_debug.c`, `src/codegen/code_generator_internal.h` |
| **Modified** | `src/codegen/code_generator.c`, `src/codegen/code_generator.h`, `src/main.c`, `Makefile`, `build.bat`, `README.md` |

---

## Documentation

`README.md` updated with an "Intermediate Representation (IR)" section describing:

- IR structure and opcodes
- Current backend coverage
- Limitations and fallback behavior
- Practical interpretation for contributors

---

## Known Limitations

- Some instructions carry `ast_ref` for fallback when constructs are not yet representable in IR.
- Method-call/object-call lowering may still fall back to AST paths.
- Global declarations are not yet first-class IR operations.
- No optimization pipeline yet; IR favors incremental safety over maximal normalization.

---

## Checklist

- [ ] All tests pass (`run_tests.ps1 -BuildCompiler`)
- [ ] Build succeeds on Windows (`build.bat`) and Linux/macOS (`make`)
- [ ] IR dump works with `-d` or `-O` (e.g. `methasm -d input.masm -o out.s` → `out.ir`)
- [ ] README IR section reviewed for accuracy

---

## Commits

1. `c293b24` — refactor codegen
2. `9c7160b` — IR added
3. `4abaf7e` — IR implemented into code gen
4. `d88e222` — Improve and fix IR
