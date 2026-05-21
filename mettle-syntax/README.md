# Mettle Syntax Highlighting

VS Code / Cursor extension for **Mettle** (`.mettle`) files: grammar-based highlighting, sensible editing defaults (brackets, indentation, folding), and a two-stage linter (fast regex rules + optional compiler diagnostics).

## What is Mettle?

Mettle is a **typed, assembly-inspired language** that compiles to **x86-64** assembly. Programs use explicit types, structs, pointers, control flow familiar from C-family languages, and a standard library for I/O, math, and more. The full language reference lives in the main repository under [`docs/`](../docs/LANGUAGE.md).

| Topic | Document |
|--------|----------|
| Comments, identifiers, keywords, literals, operators | [Lexical structure](../docs/lexical-structure.md) |
| Integer/float/pointer/string types, generics | [Types](../docs/types.md) |
| Functions, structs, enums, methods, `extern` | [Declarations](../docs/declarations.md) |
| Operators, casts, calls | [Expressions](../docs/expressions.md) |
| `if`, loops, `switch`, `defer` / `errdefer` | [Control flow](../docs/control-flow.md) |
| Files, visibility | [Modules](../docs/modules.md) |
| `import`, search paths, `import_str` | [Imports](../docs/imports.md) |
| `std/...` modules | [Standard library](../docs/standard-library.md) |
| Heap, `new`, runtime linking | [Heap allocator runtime](../docs/heap-allocator-runtime.md) |
| `extern`, C strings | [C interoperability](../docs/c-interop.md) |
| `mettle` CLI, `-I`, `--stdlib`, `--build` | [Compilation](../docs/compilation.md) |
| Cheat sheet | [Quick reference](../docs/quick-reference.md) |
| Gaps vs other languages | [Known limitations](../docs/known-limitations.md) |

From the compiler you can also run **`mettle docs`** (and **`mettle help`**) for bundled topic lists and paths.

## Extension features

### Syntax highlighting

The TextMate grammar (`syntaxes/meth.tmLanguage.json`) scopes Mettle source as `source.mettle` so themes can color keywords, types, strings, comments, and assembly blocks consistently.

### Editor behavior (`language-configuration.json`)

- **Line comments** — `//` (toggle with your editor’s line-comment command).
- **Bracket matching** — `{ }`, `[ ]`, `( )`.
- **Auto-closing / surrounding** — braces, brackets, parentheses, and string quotes.
- **Folding** — brace-region folding using `{` / `}` markers.
- **Indentation** — increases after `{` or `(` when the line looks incomplete; decreases on lines starting with `}` or `)`.

### Diagnostics (linter)

1. **Regex-based (on open and while typing, debounced)**  
   Fast checks aligned with [known limitations](../docs/known-limitations.md) and the lexer:
   - No compound assignment (`+=`, `-=`, `*=`, `/=`)
   - Invalid `0x` / `0b` literals, underscores inside numbers
   - Block comments `/* */` (only `//` is valid)
   - Labeled `break` / `continue`
   - Unterminated double-quoted strings  

2. **Compiler-backed (on save)**  
   Invokes the **Mettle** executable for real semantic diagnostics: types, scopes, undefined symbols, use-before-init, etc.

Regex diagnostics refresh as you edit; **compiler diagnostics run when you save** the file (if enabled).

### Settings (Mettle)

| Setting | Default | Meaning |
|---------|---------|---------|
| `meth.linter.compilerEnabled` | `true` | Run the compiler for semantic diagnostics on save. |
| `meth.linter.compilerPath` | *(empty)* | Path to `mettle` (relative to workspace or absolute). Empty: try workspace `bin/mettle(.exe)`, then `mettle` on `PATH`. |
| `meth.linter.stdlibPath` | *(empty)* | Optional `--stdlib` override; empty uses the compiler’s normal stdlib resolution. |

The linter invokes the compiler similarly to a check build, e.g. `-i <file> -o <temp> -I <file-dir> -I <workspace>` and optional `--stdlib`. See [Compilation](../docs/compilation.md) for CLI details.

## Installation (local)

### Option 1: Install from folder (recommended)

1. Command Palette (`Ctrl+Shift+P` / `Cmd+Shift+P`)
2. **Developer: Install Extension from Location...**
3. Choose this `meth-syntax` directory

### Option 2: Copy to extensions directory

- **Windows:** `%USERPROFILE%\.vscode\extensions\`
- **macOS/Linux:** `~/.vscode/extensions/`

Copy the whole `meth-syntax` folder, then reload the window.

### Option 3: Symlink (development)

```powershell
# From repo root
New-Item -ItemType Junction -Path "$env:USERPROFILE\.vscode\extensions\meth-syntax" -Target "$PWD\meth-syntax"
```

Reload the editor after installing.

## Trying the linter

Open [`test-lint.mettle`](test-lint.mettle) to see regex rules in action. For compiler diagnostics, save a `.mettle` file with the Mettle binary available (workspace build or `PATH`).

## Minimal Mettle program

```mettle
function main() -> int32 {
  return 0;
}
```

More examples: [Quick reference](../docs/quick-reference.md).
