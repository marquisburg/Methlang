# Standard Library

The standard library lives under `stdlib/`. Modules are imported by path. The `std/` prefix is resolved under the stdlib root (default `stdlib`).

## Platform Support

The compiler and most stdlib modules work on Linux and Windows. The compiler emits `elf64` assembly on Linux and `win64` on Windows. Use `make` to build the compiler on Linux and macOS; use `build.bat` on Windows.

**Cross-platform modules:** `std/io`, `std/mem`, `std/math`, `std/conv`, and `std/process` use the C runtime and work on both Linux and Windows.

**Windows-only:** `std/net` provides Winsock2 bindings and `std/thread` provides Win32 threading bindings. They do not work on Linux. Programs that import `std/net`/`std/thread` or use `--prelude` (which includes `std/net`) will fail to link on Linux with undefined references to Win32 symbols. For networking/threading on Linux, use `std/net_posix` and `std/thread_posix` respectively.

**Linux/macOS:** `std/net_posix` provides POSIX socket bindings and `std/thread_posix` provides pthread-based threading. These modules require linking with `stdlib/posix_helpers.c`.

## std/io

Console and file I/O. `puts` writes a null-terminated string and appends a newline; `putchar` writes a single character; `getchar` reads one. `print` and `println` write a cstring (println adds a newline). `print_err` and `println_err` write to stderr (for error messages). `print_int` and `println_int` write an integer in decimal. `cstr(s: string) -> cstring` converts a Methlang string to a C string for passing to C functions. File operations: `fopen`, `fclose`, `fread`, `fwrite`, `fputs`, `fgets`, `fflush`. File handles are `cstring` (opaque `FILE*`). Stream accessors: `get_stdin`, `get_stdout`, `get_stderr`.

## std/mem

Memory management. C runtime functions: `malloc`, `calloc`, `realloc`, `free`, `memset`, `memcpy`, `memmove`, `memcmp`. Helpers: `alloc_zeroed` (allocate and zero-initialize), `buf_dup` (allocate and copy a buffer). Use `malloc` for buffers, C interop, or when the GC is not linked. Use `new` for Methlang struct instances that should be garbage-collected. See [Garbage Collector](garbage-collector.md).

## std/math

Math utilities. `abs` (C runtime). `min`, `max`, `clamp` (integer operations on int64).

## std/conv

Conversions and character classification. C runtime: `atoi`, `atol`. Digit helpers: `digit_to_char`, `char_to_digit`. Classification: `is_digit`, `is_upper`, `is_lower`, `is_alpha`, `is_alnum`, `is_space`. Case conversion: `to_lower`, `to_upper`. String utilities: `strlen`, `streq`, `strncmp(buf, offset, needle, buf_len)`.

## std/process

Process control. `exit` terminates the program with an exit code. `rand`, `srand` for pseudo-random numbers.

## std/system

Process spawning. `system(cmd: cstring) -> int32` runs a shell command via the C runtime. Use for invoking Methlang, nasm, gcc, git, curl, etc. On Windows invokes cmd.exe; on Linux invokes sh -c.

## std/dir

Directory and file operations. Requires linking `stdlib/dir_helpers.c`. `dir_exists(path: cstring) -> int32` returns 1 if the path is an existing directory. `dir_create(path: cstring) -> int32` creates a directory (returns 0 on success). `file_exists(path: cstring) -> int32` returns 1 if the path is an existing regular file. `getcwd(buf: cstring, size: int32) -> int32` fills `buf` with the current working directory (returns 0 on success). Cross-platform (Windows and Linux).

## std/http

HTTP fetch (MVP). `http_fetch_to_file(url: cstring, output_path: cstring) -> int32` downloads a URL to a file using curl. Requires curl in PATH. Returns curl's exit code (0 = success).

## std/net

Winsock2 bindings for Windows only. Does not work on Linux. Requires linking with `-lws2_32`. Constants include address/socket/protocol values (`AF_INET`, `SOCK_STREAM`, `IPPROTO_TCP`) and common socket options (`SOL_SOCKET`, `SO_REUSEADDR`) plus shutdown values (`SD_RECEIVE`, `SD_SEND`, `SD_BOTH`).

Core functions: `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`, `shutdown`, `closesocket`, `setsockopt`. Lifecycle: `net_init`, `net_cleanup`, `net_last_error`.

`net_init`/`net_cleanup` are thread-safe and reference-counted. Multiple threads can call `net_init` safely; Winsock startup happens once and cleanup happens when the last caller releases via `net_cleanup`. Extra cleanup calls are treated as no-op for robustness.

Convenience helpers:
- `socket_tcp`, `socket_udp`
- `sockaddr_in(ip, port)`, `sockaddr_in_any(port)`
- `set_reuseaddr(sock, enabled)`
- `send_all(sock, buf, len)` (looping send until full write or error)
- `net_is_initialized()`

For HTTP responses, prefer sending header and body in separate `send_all` calls. If you omit `Content-Length`, include `Connection: close` and close the socket after sending.

## std/net_posix

POSIX socket bindings for Linux and macOS. Does not work on Windows. Socket functions are in libc on both platforms; no extra link flags are required for basic sockets. However, this module requires `stdlib/posix_helpers.c` for thread-safe errno access and atomic spin-lock operations.

Link command:
```bash
# Linux
gcc -o myapp output.s stdlib/posix_helpers.c -lpthread
# macOS  
gcc -o myapp output.s stdlib/posix_helpers.c
```

Constants include address/socket/protocol values (`AF_INET_POSIX`, `SOCK_STREAM_POSIX`, `IPPROTO_TCP_POSIX`) and socket options (`SOL_SOCKET_POSIX`, `SO_REUSEADDR_POSIX`). Note: macOS uses different values for `SOL_SOCKET` (0xFFFF) and `SO_REUSEADDR` (4) than Linux (1 and 2).

Core functions: `socket`, `close_fd`, `posix_bind`, `posix_listen`, `posix_accept`, `posix_connect`, `posix_send`, `posix_recv`, `posix_shutdown`, `posix_setsockopt`. Lifecycle: `net_posix_init`, `net_posix_cleanup`, `net_posix_last_error`.

`net_posix_init`/`net_posix_cleanup` are thread-safe and reference-counted for API compatibility with `std/net`, though POSIX sockets don't require initialization.

Convenience wrappers:
- `socket_tcp_posix`, `socket_udp_posix`
- `sockaddr_in_posix(ip, port)`, `sockaddr_in_any_posix(port)`
- `set_reuseaddr_posix(sock, enabled)`
- `send_all_posix(sock, buf, len)` (looping send until full write or error)
- `net_posix_is_initialized()`

The function names are prefixed with `posix_` to avoid conflicts with the Windows `std/net` module when writing cross-platform code.

## std/thread

Windows Win32 thread primitives. Includes:
- Thread APIs: `CreateThread`, `WaitForSingleObject`, `CloseHandle`, `GetCurrentThreadId`, `Sleep`
- Mutex APIs: `CreateMutexA`, `ReleaseMutex` with wrappers (`mutex_create`, `mutex_lock`, `mutex_unlock`, `mutex_close`)
- Atomics: `InterlockedCompareExchange`, `InterlockedExchange`, `InterlockedIncrement`, `InterlockedDecrement` with wrapper helpers
- Spin lock helpers: `spin_try_lock`, `spin_lock`, `spin_unlock` for short critical sections

`CreateThread` accepts a function pointer `fn(cstring) -> uint32` for the thread entry. Pass `&my_thread_proc` directly; no C bridge is required. See [Types](types.md#function-pointer-type) for function pointer syntax.

## std/thread_posix

pthread-based threading for Linux and macOS. Includes `pthread_create`, `pthread_join`, mutexes, and atomics. Requires linking with `stdlib/posix_helpers.c` and `-lpthread` on Linux. `pthread_create` accepts a function pointer `fn(cstring) -> cstring` for the thread entry; pass `&my_thread_proc` directly.

## std/prelude

The prelude re-exports `std/io`, `std/math`, `std/conv`, `std/mem`, `std/process`, and `std/net`. Use with `--prelude` to automatically import these modules without explicit `import` statements. The prelude is opt-in; it is not loaded by default. On Linux, `--prelude` will fail at link time because it pulls in `std/net` (Windows-only). Use explicit imports instead.

```bash
Methlang --prelude main.meth -o main.s
```


