# Import System

Methlang provides two mechanisms for bringing external content into your program at compile time: **module imports** (`import`) and **embedded file imports** (`import_str`). Both use the same path resolution rules.

## Module Import: `import`

The `import` directive loads another Methlang source file and makes its declarations available in the current program.

```meth
import "std/io";
import "path/to/module";
import "shared_math";   // .meth added if omitted
```

- **Path format:** A string literal. Forward slashes (`/`) or backslashes (`\`) are accepted on Windows.
- **Extension:** If the path has no extension, `.meth` is appended automatically. `"std/io"` resolves to `std/io.meth`.
- **Scope:** Imported declarations are flattened into the **global scope**. There are no namespaces; use distinct names to avoid collisions.

### What Can Be Imported

Only **exported** declarations are visible when a module uses `export`. If a module does not use `export`, all of its declarations are visible (backward compatibility). See [Modules](modules.md) for export rules.

## Embedded File Import: `import_str`

The `import_str` expression reads a file at compile time and embeds its contents as a `string` value. Use it for HTML, config files, or any text data.

```meth
var PAGE_CONTENT: string = import_str "index.html";
var config: string = import_str "config.txt";
```

- **Path format:** Same as `import` — a string literal.
- **Extension:** No automatic extension. The path is used as given. `"index.html"` resolves to `index.html`, not `index.html.meth`.
- **Result:** The file contents become a `string` with `.chars` (pointer to data) and `.length` (byte count). The string is null-terminated for C interop.

`import_str` can appear anywhere a string literal is valid: variable initializers, function call arguments, etc.

## Path Resolution Order

Both `import` and `import_str` resolve paths in this order:

1. **Absolute paths** — Used as-is. On Windows: `C:\path\to\file` or `\path\to\file`. On Unix: `/path/to/file`.

2. **Stdlib** — If the path starts with `std/` (or `std\` on Windows), it is resolved under the stdlib root (default `stdlib`, overridable with `--stdlib`).
   - `import "std/io"` → `stdlib/std/io.meth`
   - `import_str "std/template.html"` → `stdlib/std/template.html`

3. **Relative to importing file** — Relative to the directory of the file containing the import.
   - In `web/server.meth`, `import_str "index.html"` → `web/index.html`
   - In `main.meth`, `import "lib/utils"` → `lib/utils.meth` (relative to `main.meth`’s directory)

4. **`-I` directories** — Each directory added with `-I` is searched in command-line order.
   - `methlang -I lib -I vendor main.meth` — `lib` is searched before `vendor`.
   - First match wins: if both `lib/foo.meth` and `vendor/foo.meth` exist, `lib/foo.meth` is used.

5. **Fallback** — The path is tried as-is (e.g. current working directory).

### Path Separators

On Windows, both `/` and `\` are accepted. Use `/` for portability across platforms.

## Compiler Options

| Option | Description |
|--------|--------------|
| `-I <dir>` | Add import search directory. Repeatable. |
| `--stdlib <dir>` | Set stdlib root (default: `stdlib`). |
| `-i <file>` | Input file (entry point). Imports are resolved from this file. |

**Example:**

```bash
methlang -I tests/lib -I vendor main.meth -o output.s
```

This allows `import "shared_math"` to resolve to `tests/lib/shared_math.meth` when `tests/lib` is on the search path.

## Duplicate and Circular Imports

- **Duplicate imports:** If the same file is imported multiple times (e.g. via different import paths), it is included only once. A warning is emitted:
  ```
  Circular or duplicate import of 'std/io' (import chain: main.meth -> utils.meth -> std/io)
  ```

- **Circular imports:** Cycles (A → B → A) are detected. The second import of an already-visited file is skipped. The import chain in the message shows the path that led to the cycle.

## Error Messages and Import Chain

When a path cannot be resolved, the compiler reports the **import chain** — the sequence of files that led to the failed import:

```
Could not resolve imported file 'lib/math' (import chain: main.meth -> utils.meth)
```

```
Could not resolve embedded file 'templates/forum.css' (import chain: web/server.meth)
```

```
Could not read embedded file 'missing.txt' (import chain: main.meth)
```

The chain helps debug transitive import issues and missing files.

## Single Entry Point

The compiler takes one entry point (the file passed via `-i` or as a positional argument). All imports are resolved transitively from that file. You do not compile each file separately; a single invocation processes the full dependency graph and produces one assembly output.

## See Also

- [Modules](modules.md) — Export rules, visibility, re-exports
- [Compilation](compilation.md) — Build pipeline, options
- [Standard Library](standard-library.md) — Stdlib layout, `std/` modules
