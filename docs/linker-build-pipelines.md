# Linker and build pipelines (triage)

This note maps **which “linker” runs** for each Windows `--build` combination so bug reports land in the right subsystem.

## Pipelines

| Command (typical) | Codegen output | Link step | In-tree `src/linker`? |
|------------------|----------------|-----------|----------------------|
| `--build` (no `--emit-obj`, asm backend) | NASM `.s` | NASM → `.o`, then **`mettle_build_with_gcc`** (`gcc -nostartfiles`, optional `mettle_entry.o`) | **No** — MinGW `gcc` / `ld` |
| `--build --emit-obj --linker internal` | COFF `.obj` | **`write_internal_startup_object`** + **`mettle_link_internal`** → PE | **Yes** |
| `--build --emit-obj --linker gcc` | COFF `.obj` | **`mettle_link_object_with_gcc`** (matches asm path: `gcc -nostartfiles`, same libs / entry as `mettle_build_with_gcc`) | **No** |
| `--build --emit-obj --linker msvc` | COFF `.obj` | **`mettle_link_object_with_link`** (`link.exe`) | **No** |

**Regression triage:** If a bug appears only on **asm + `--linker gcc`**, it is **not** explained by `relocation.c` / `pe_emitter.c`. If it appears only on **`--emit-obj --linker internal`**, prioritize **`src/linker`** and the binary emitter’s COFF. If **internal** is correct but **`--emit-obj --linker gcc`** was wrong, suspect **GCC command parity** (startup, `mettle_entry.o`, `-nostartfiles`, libs) in `main.c` before deep-diving relocations.

## Artifacts to capture when reporting

1. Full compiler/link stdout and stderr.
2. For asm path: generated `.s` and NASM-produced `.o`.
3. For emit-obj path: compiler-produced `.obj` (same flags as failure).
4. Exact `mettle` arguments and, if relevant, the `gcc` / `link.exe` command line (see `mettle_*_with_gcc` / `mettle_build_with_gcc` in `main.c`).

Optional: `objdump -x` / `llvm-readobj` on the `.obj` and final PE for symbol and relocation comparison.

## Binary emitter ↔ internal linker relocation map

The object backend records relocations in [`binary_emitter_map_relocation_kind`](g:/Projects/Mettle/src/codegen/binary_emitter.c). The internal linker applies them in [`link_apply_relocations`](g:/Projects/Mettle/src/linker/relocation.c):

| `BinaryRelocationKind` | AMD64 COFF type | Width |
|------------------------|-----------------|-------|
| `BINARY_RELOCATION_REL32` (default) | `COFF_RELOC_AMD64_REL32` | 4 |
| `BINARY_RELOCATION_ADDR64` | `COFF_RELOC_AMD64_ADDR64` | 8 |
| `BINARY_RELOCATION_ADDR32NB` | `COFF_RELOC_AMD64_ADDR32NB` | 4 |
| `BINARY_RELOCATION_SECTION_REL32` | `COFF_RELOC_AMD64_SECREL` | 4 |

Unsupported COFF relocation types in a merged input will fail with a clear error from `link_apply_relocations`.
