# MethASM Installer

This directory contains the files needed to build a native Windows installer (`.exe`) for MethASM.

## Features
- Copies `methasm.exe`, `stdlib\`, and `src\runtime\` to the installation directory (`C:\Program Files\MethASM`).
- Installs `masm-build.bat` plus `masm-pkg` (exe + wrapper) for dependency management and project builds.
- Automatically adds MethASM's `bin\` directory to your user `%PATH%`.
- Registers the `.masm` file extension with MethASM's icon.
- Provides a clean uninstaller that removes MethASM and cleans up your `%PATH%`.

## How to Build the Installer
1. Download and install [Inno Setup](https://jrsoftware.org/isinfo.php) (the QuickStart Pack or the standard installer). 
2. Open `MethASM.iss` in the Inno Setup Compiler.
3. Click "Compile" (or press Ctrl+F9).
4. A `out\MethASM-Setup.exe` file will be generated in `installer/out`.

You can now distribute `MethASM-Setup.exe`!

## Usage after installation
Once installed, users can simply run this command from anywhere:
```bat
masm-build.bat my_program.masm
```
This automatically leverages `methasm`, `nasm`, and `gcc` (must be separately installed and available on `%PATH%`) to produce `my_program.exe` cleanly.

Package manager usage after install:
```bat
masm-pkg add mylib ..\shared\mylib
masm-pkg install
masm-pkg build
```
