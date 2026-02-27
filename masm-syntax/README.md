# MethASM Syntax Highlighting

Syntax highlighting and basic linting for MethASM (`.masm`) files in VS Code and Cursor.

## Linter

Two diagnostic layers:

### 1. Regex-based (instant)
Pattern checks for common issues based on the MethASM language reference:

**Unsupported operators:**
- **Modulo `%`** — Use a helper or inline logic
- **Unary logical NOT `!`** — Use `== 0` or `!= 0` for comparisons
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
Runs the MethASM compiler (`bin/methasm.exe`) for **semantic diagnostics**: type errors, scope errors, use-before-init, undefined symbols, etc. Requires the compiler to be built and available at `bin/methasm.exe` (or configured path).

**Settings** (MethASM in Settings):
- `masm.linter.compilerEnabled` — Enable compiler diagnostics (default: true)
- `masm.linter.compilerPath` — Path to methasm (default: auto-detect `bin/methasm.exe`)
- `masm.linter.stdlibPath` — Path to stdlib (default: `stdlib`)

Diagnostics run on open, edit (debounced), and save. See `test-lint.masm` for regex examples.

## Installation (local)

### Option 1: Install from folder (recommended)

1. Open Command Palette (`Ctrl+Shift+P` / `Cmd+Shift+P`)
2. Run **Developer: Install Extension from Location...**
3. Select the `masm-syntax` folder (this directory)

### Option 2: Copy to extensions directory

- **Windows**: `%USERPROFILE%\.vscode\extensions\`
- **macOS/Linux**: `~/.vscode/extensions/`

Copy the entire `masm-syntax` folder there, then reload the window.

### Option 3: Symlink (development)

```powershell
# From project root
New-Item -ItemType Junction -Path "$env:USERPROFILE\.vscode\extensions\masm-syntax" -Target "$PWD\masm-syntax"
```

Reload Cursor/VS Code after installing.
