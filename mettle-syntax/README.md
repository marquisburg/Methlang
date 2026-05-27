# Mettle Language Support

VS Code and Cursor support for **Mettle** (`.mettle`) files.

## Features

- TextMate syntax highlighting for current Mettle syntax.
- Nested block comments and line comments.
- Bracket matching, auto-pairs, folding, and indentation rules.
- Snippets for functions, imports, structs, enums, traits, loops, `defer`, `errdefer`, and `match`.
- Rich hover documentation for Mettle keywords, primitive types, strings, imports, traits, generics, defers, control flow, and allocation.
- Contextual hovers for import paths, including best-effort resolution against the workspace, stdlib, and configured include paths.
- Same-file declaration hovers for functions, globals, structs, enums, traits, methods, and impls.
- Fast editor diagnostics for lexer-level mistakes.
- Optional compiler-backed diagnostics for real semantic errors.
- Commands:
  - `Mettle: Lint Active File`
  - `Mettle: Clear Diagnostics`
  - `Mettle: Show Output`

## Compiler Diagnostics

The extension can run `mettle` on open/save and show compiler errors in the Problems panel.

Compiler discovery order:

1. `mettle.linter.compilerPath`, if set.
2. `bin/mettle.exe` or `bin/mettle` found by walking up from the current file.
3. Workspace `bin/mettle.exe`, `bin/mettle`, `mettle.exe`, or `mettle`.
4. `mettle.exe` or `mettle` on `PATH`.

Settings:

| Setting | Default | Meaning |
| --- | --- | --- |
| `mettle.linter.compilerEnabled` | `true` | Run compiler diagnostics. |
| `mettle.linter.compilerPath` | `""` | Absolute path, or path relative to the workspace. |
| `mettle.linter.stdlibPath` | `""` | Optional `--stdlib` override. |
| `mettle.linter.extraIncludePaths` | `[]` | Additional `-I` include directories. |
| `mettle.linter.compilerTimeoutMs` | `10000` | Timeout for one compiler diagnostics run. |

## Language Coverage

The grammar is aligned with the current compiler surface:

- Declarations: `import`, `import_str`, `extern`, `export`, `var`, `function`, `fn`, `struct`, `enum`, `method`, `trait`, `impl`, `where`.
- Control flow: `if`, `else`, `while`, `for`, `switch`, `match`, `case`, `default`, `break`, `continue`, `return`, `defer`, `errdefer`.
- Types: integer and float primitives, `string`, `cstring`, `bool`, `void`, `fn(...) -> ...`, and user types.
- Literals: decimal, hex, binary, float, strings, and character literals.
- Operators: assignment, compound assignment, comparisons, logical operators, bitwise operators, pointer/member access, and casts.
- Inline `asm { ... }` blocks with basic x86 mnemonic/register highlighting.

## Hover Coverage

Hover cards are designed as compact reference docs, not one-line labels. They include syntax examples, gotchas, and the rules that tend to matter while coding:

- Module behavior for `import`, `import_str`, `export`, and `extern`.
- Control-flow semantics for `if`, loops, `switch`, `match`, `break`, `continue`, and `return`.
- Cleanup behavior for `defer` and `errdefer`, including LIFO ordering and error-return convention.
- Type notes for primitives, `string`, `cstring`, `fn`, pointers, structs, enums, traits, `impl`, and `where`.
- Common standard-library and C interop calls from `std/io`, `std/mem`, `std/conv`, `std/process`, and `std/system`.
- Compile-time helpers such as `sizeof`, `alignof`, and `static_assert`.

When hovering a quoted import path, the extension tries to show the resolved file. It uses the current file directory, workspace root, `mettle.linter.stdlibPath`, discovered `stdlib/`, and `mettle.linter.extraIncludePaths`.

## Local Installation

### Install From Folder

1. Open the Command Palette.
2. Run `Developer: Install Extension from Location...`.
3. Pick this `mettle-syntax` directory.
4. Reload the editor window.

### Development Symlink

```powershell
New-Item -ItemType Junction -Path "$env:USERPROFILE\.vscode\extensions\mettle-syntax" -Target "$PWD\mettle-syntax"
```

## Validation

Run the extension self-check:

```powershell
cd mettle-syntax
npm run check
```

The check parses all JSON contribution files, verifies contributed paths exist, verifies command registrations, and guards against known README/package mojibake.

## Minimal Program

```mettle
function main() -> int32 {
  return 0;
}
```

See the repository `docs/` directory for the full language reference.
