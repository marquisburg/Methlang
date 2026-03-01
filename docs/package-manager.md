# MethASM Package Manager (masm-pkg)

The MethASM package manager (`masm-pkg`) manages dependencies and builds MethASM projects. It is written in MethASM itself and is local-only: no network, no git, no registry.

## Building masm-pkg

From the project root:

```powershell
.\tools\masm-pkg\build.bat
```

This produces `tools\masm-pkg\masm-pkg.exe` and publishes `bin\masm-pkg.exe`. Requirements:

- MethASM compiler (`bin\methasm.exe`)
- NASM
- GCC
- `masm_entry.o` and `-lshell32` (for `main(argc, argv)`)

## Commands

| Command | Description |
|---------|-------------|
| `masm-pkg add <name> <path>` | Add a direct dependency to `masm.deps` |
| `masm-pkg remove <name>` | Remove a direct dependency from `masm.deps` |
| `masm-pkg list` | Resolve and list all dependencies (direct + transitive) |
| `masm-pkg install` | Resolve and install all dependencies to cache |
| `masm-pkg build` | Resolve/install deps, then run methasm + nasm + gcc |

## Project Dependency File: masm.deps

`masm.deps` lists direct dependencies in the project root:

```text
mylib=..\shared\mylib
utils=.\vendor\utils
```

- `name` must match `[A-Za-z0-9_-]+`
- `path` must be a local directory path
- Empty lines and `#` comments are ignored

## Package Manifest: masm.pkg

Each package directory can define `masm.pkg`:

```text
name=mylib
version=1.2.0
dep=..\shared\corelib
dep=C:\Projects\local\logger
```

- `name` is required for transitive packages
- `version` is optional (defaults to `0.0.0-local`)
- `dep` can be repeated
- `dep` paths are local only (relative to package root or absolute)

Resolution is recursive from `masm.deps` through `dep` entries.

## Conflict Rules

If two resolved packages share the same `name` but differ in `version` or `path`, resolution fails with a conflict error.

## Lockfile: masm.lock

`install` and `build` write `masm.lock`:

```text
# masm-pkg lockfile (local-only)
# name=version|path
mylib=1.2.0|..\shared\mylib
corelib=0.4.1|..\shared\corelib
```

## Package Cache

Installed packages are copied to `.masm/packages/<name>/` using `xcopy` on Windows.

## Build Pipeline

`masm-pkg build` performs:

1. Resolve dependencies from `masm.deps` + `masm.pkg`
2. Install dependencies into `.masm/packages`
3. `methasm "main.masm" -o "main.s" --stdlib "<auto-detected>" -I ".masm\packages"`
4. `nasm -f win64 "main.s" -o "main.o"`
5. `gcc -nostartfiles "main.o" "<gc.c>" "<masm_entry.c>" -o "main.exe" -lkernel32 -lshell32`

Path fallback is local-only:
- Compiler: `bin\methasm.exe`, then `..\bin\methasm.exe`, then `methasm` in PATH
- Stdlib: `stdlib`, then `..\stdlib`
- Runtime C files: `src\runtime\...`, then `..\src\runtime\...`

## Example Workflow

```powershell
# 1. Add direct dependencies
masm-pkg add mylib ..\shared\mylib

# 2. Resolve/install all dependencies
masm-pkg install

# 3. Inspect dependency graph and install state
masm-pkg list

# 4. Build project
masm-pkg build
```

## Limitations

- Local-only source model (no remote download)
- Windows install copy uses `xcopy`
- Single-target output convention (`main.s`, `main.o`, `main.exe`)
