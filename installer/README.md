# Methlang Installer

This directory contains the files needed to build a native Windows installer (`.exe`) for Methlang.

## Features
- Copies `methlang.exe`, `stdlib\`, and `runtime\` to the installation directory (`C:\Program Files\Methlang`).
- Installs `meth-build.bat` for project builds.
- Optionally adds Methlang's `bin\` directory to the system `%PATH%` (installer task, enabled by default).
- Optionally registers the `.meth` file extension with Methlang's icon and open command.
- Provides a clean uninstaller that removes Methlang and cleans up your `%PATH%`.

## How to Build the Installer
1. Download and install [Inno Setup](https://jrsoftware.org/isinfo.php) (the QuickStart Pack or the standard installer). 
2. Open `Methlang.iss` in the Inno Setup Compiler.
3. Click "Compile" (or press Ctrl+F9).
4. A `out\Methlang-Setup.exe` file will be generated in `installer/out`.

You can now distribute `Methlang-Setup.exe`!

## Usage after installation
Once installed, users can simply run this command from anywhere:
```bat
meth-build.bat my_program.meth
```
This automatically leverages `Methlang`, `nasm`, and `gcc` (must be separately installed and available on `%PATH%`) to produce `my_program.exe` cleanly.
