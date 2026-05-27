# Mettle Installer

This directory contains the files needed to build a native Windows installer (`.exe`) for Mettle.

## Features
- Copies `mettle.exe`, `stdlib\`, and `runtime\` to the installation directory (`C:\Program Files\Mettle`).
- Installs the two opt-in helper objects (`crash_handler.o` and `atomics.o`) so `--build` can link them on demand when a program uses crash tracebacks or `std/thread` interlocked atomics. Heap allocation and `main(argc, argv)` startup require no Mettle helper object.
- Installs `mettle-build.bat` as a thin wrapper over `mettle --build`.
- Optionally adds Mettle's `bin\` directory to the system `%PATH%` (installer task, enabled by default).
- Optionally registers the `.mettle` file extension with Mettle's icon and open command.
- Provides a clean uninstaller that removes Mettle and cleans up your `%PATH%`.

## How to Build the Installer
1. Download and install [Inno Setup](https://jrsoftware.org/isinfo.php) (the QuickStart Pack or the standard installer).
2. If you changed `mettle-syntax/icons/*.svg`, regenerate wizard artwork (requires [ImageMagick](https://imagemagick.org)):
   ```powershell
   cd installer
   .\build-assets.ps1
   ```
3. Open `Mettle.iss` in the Inno Setup Compiler (or run `iscc Mettle.iss`).
4. A `out\Mettle-Setup.exe` file will be generated in `installer/out`.

Branding uses `mettle-dark.svg` on the installer sidebar and `mettle-light.svg` in the wizard header chip. Committed `assets/*.bmp` files are what CI compiles; rerun the script when icons change.

You can now distribute `Mettle-Setup.exe`!

## Usage after installation
Once installed, users can simply run this command from anywhere:
```bat
mettle --build my_program.mettle
```
This automatically leverages `Mettle`, `nasm`, and then `gcc` or `link.exe` to produce `my_program.exe` cleanly.

`mettle-build.bat my_program.mettle` forwards to `mettle --build`.

For quick discovery from the installed CLI:

```bat
mettle help
mettle help runtime
mettle docs
```
