# Hello World (GUI)

A native Windows GUI hello world for the
[Mettle](https://github.com/user/mettle) programming language by Vinnie Falco.
Mettle makes it remarkably easy to create native Win32 desktop applications
with clean, readable code and zero external dependencies.

This example opens a window and paints "Hello, world!" using the Mettle
standard UI library.

## What It Does

- Initializes the Mettle UI subsystem (`ui_init`)
- Creates a native Win32 window with `ui_window_create`
- Registers a `window_proc` callback to handle window messages
- Paints a blue header bar with white "Hello, world!" text
- Runs the Win32 message loop until the window is closed

## How It Works

Mettle's `std/ui` library wraps the Win32 API into a clean set of functions:

| Function | Purpose |
|---|---|
| `ui_init()` | Register the window class and initialize GDI |
| `ui_window_create(...)` | Create and position the window |
| `ui_begin_paint` / `ui_end_paint` | Start and finish a paint cycle |
| `ui_fill_rect_color` | Fill a rectangle with a solid color |
| `ui_draw_text_color` | Draw text with foreground and background colors |
| `ui_window_show` | Make the window visible |
| `ui_run_message_loop` | Dispatch messages until `WM_QUIT` |

The `window_proc` function handles three messages:

- **WM_PAINT** - calls `on_paint` to draw the window contents
- **WM_DESTROY** - calls `ui_quit` to end the message loop
- Everything else is forwarded to `ui_def_window_proc`

## Build

From the repository root:

```powershell
bin\mettle.exe --build --linker internal examples\hello_ui\hello_ui.mettle -o examples\hello_ui\hello_ui.exe
```

## Run

```powershell
examples\hello_ui\hello_ui.exe
```

A window will appear with a blue header bar displaying "Hello, world!" in
white text.

## Author

Vinnie Falco
