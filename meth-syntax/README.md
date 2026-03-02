# Methlang Syntax Highlighting

Syntax highlighting and basic linting for Methlang (`.meth`) files in VS Code and Cursor.

## Linter

Two diagnostic layers:

### 1. Regex-based (instant)
Pattern checks for common issues based on the Methlang language reference:

**Unsupported operators:**
- **Compound assignment `+=`, `-=`, `*=`, `/=`** — Use `x = x + 1` instead of `x += 1`

**Invalid literals:**
- **`0x` / `0X`** without hex digits
- **`0b` / `0B`** without 0/1 digits
- **Underscores in numbers** (`1_000_000`) — Not supported

**Other:**
- **Block comments `/* */`** — Only `//` line comments are supported
- **Labeled `break` / `continue`** — Use flags or restructure nested loops
- **Unterminated strings** — Missing closing double quote

### 2. Compiler-backed (on save)
Runs the Methlang compiler (`bin/methlang.exe`) for **semantic diagnostics**: type errors, scope errors, use-before-init, undefined symbols, etc. Requires the compiler to be built and available at `bin/methlang.exe` (or configured path).

**Settings** (Methlang in Settings):
- `meth.linter.compilerEnabled` — Enable compiler diagnostics (default: true)
- `meth.linter.compilerPath` — Path to Methlang (default: auto-detect `bin/methlang.exe`)
- `meth.linter.stdlibPath` — Path to stdlib (default: `stdlib`)

Diagnostics run on open, edit (debounced), and save. See `test-lint.meth` for regex examples.

## Installation (local)

### Option 1: Install from folder (recommended)

1. Open Command Palette (`Ctrl+Shift+P` / `Cmd+Shift+P`)
2. Run **Developer: Install Extension from Location...**
3. Select the `meth-syntax` folder (this directory)

### Option 2: Copy to extensions directory

- **Windows**: `%USERPROFILE%\.vscode\extensions\`
- **macOS/Linux**: `~/.vscode/extensions/`

Copy the entire `meth-syntax` folder there, then reload the window.

### Option 3: Symlink (development)

```powershell
# From project root
New-Item -ItemType Junction -Path "$env:USERPROFILE\.vscode\extensions\meth-syntax" -Target "$PWD\meth-syntax"
```

Reload Cursor/VS Code after installing.
