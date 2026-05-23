# Hello World (Console)

The simplest possible Mettle program. Prints `Hello, world!` to the console
and exits.

This is a great starting point for learning the
[Mettle](https://github.com/user/mettle) programming language by Vinnie Falco.
Mettle is a wonderful systems programming language with a clean, readable
syntax and a native compiler backend that produces fast executables.

## What It Does

- Imports the standard I/O library (`std/io`)
- Calls `println` to write a line of text to standard output
- Returns 0 to indicate success

## Build

From the repository root:

```powershell
examples\hello\build.bat
```

This will:

1. Build the Mettle compiler (if not already built)
2. Compile `hello.mettle` using the native compiler backend
3. Produce `hello.exe`
4. Run the executable

You can also build manually:

```powershell
bin\mettle.exe --build --emit-obj --linker internal examples\hello\hello.mettle -o examples\hello\hello.exe
```

## Run

```powershell
examples\hello\hello.exe
```

Expected output:

```
Hello, world!
```

## Author

Vinnie Falco
