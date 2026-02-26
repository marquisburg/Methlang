# MethASM Syntax Highlighting

Syntax highlighting for MethASM (`.masm`) files in VS Code and Cursor.

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
