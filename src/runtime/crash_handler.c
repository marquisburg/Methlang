#include "crash_handler.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#ifndef STATUS_HEAP_CORRUPTION
#define STATUS_HEAP_CORRUPTION ((DWORD)0xC0000374)
#endif
#ifndef STATUS_STACK_BUFFER_OVERRUN
#define STATUS_STACK_BUFFER_OVERRUN ((DWORD)0xC0000409)
#endif
#else
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#if defined(__linux__) || defined(__APPLE__)
#include <ucontext.h>
#endif
#endif

static const MettleCrashFunctionInfo *g_runtime_debug_functions = NULL;
static size_t g_runtime_debug_function_count = 0;
static const MettleCrashLocationInfo *g_runtime_debug_locations = NULL;
static size_t g_runtime_debug_location_count = 0;

#if defined(_WIN32) || defined(_WIN64)
static volatile LONG g_runtime_debug_handler_installed = 0;
static volatile LONG g_runtime_debug_in_handler = 0;
static PVOID g_runtime_debug_vectored_handler = NULL;
#else
/* POSIX crash-handler state. Updated only from the signal handler / install
   path; kept minimal and async-signal-safe. */
static volatile sig_atomic_t g_runtime_debug_handler_installed = 0;
static volatile sig_atomic_t g_runtime_debug_in_handler = 0;
#endif

/* ---------------------------------------------------------------------------
 * Platform-neutral crash diagnostics
 *
 * The symbolization and frame-walking logic below is identical on every
 * platform: it only reads the registered debug-info tables and walks saved
 * frame pointers. Only three things are platform-specific and isolated:
 *   - mettle_crash_write_stderr_bytes (raw, async-signal-safe stderr write)
 *   - mettle_crash_address_is_readable (probe before dereferencing a frame)
 *   - the fault interception entry point (SEH filter / POSIX sigaction)
 * Keeping the shared code common is what gives Linux/macOS the same
 * symbolized backtraces Windows already produced.
 * ------------------------------------------------------------------------- */

static void mettle_crash_write_stderr_bytes(const char *text, size_t length) {
  if (!text || length == 0) {
    return;
  }

#if defined(_WIN32) || defined(_WIN64)
  HANDLE stderr_handle = GetStdHandle(STD_ERROR_HANDLE);
  if (stderr_handle && stderr_handle != INVALID_HANDLE_VALUE) {
    DWORD written = 0;
    WriteFile(stderr_handle, text, (DWORD)length, &written, NULL);
  } else {
    OutputDebugStringA(text);
  }
#else
  /* write(2) is async-signal-safe; loop over short writes/EINTR. */
  size_t offset = 0;
  while (offset < length) {
    ssize_t n = write(STDERR_FILENO, text + offset, length - offset);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    offset += (size_t)n;
  }
#endif
}

static void mettle_crash_write_stderr(const char *text) {
  if (!text) {
    return;
  }
  mettle_crash_write_stderr_bytes(text, strlen(text));
}

static void mettle_crash_write_decimal_uintptr(uintptr_t value) {
  char buffer[32];
  size_t index = sizeof(buffer);
  buffer[--index] = '\0';

  do {
    buffer[--index] = (char)('0' + (value % 10u));
    value /= 10u;
  } while (value != 0 && index > 0);

  mettle_crash_write_stderr(buffer + index);
}

static void mettle_crash_write_hex_uintptr(uintptr_t value, size_t width) {
  static const char digits[] = "0123456789ABCDEF";
  char buffer[2 + (sizeof(uintptr_t) * 2) + 1];
  size_t index = sizeof(buffer);
  buffer[--index] = '\0';

  size_t digit_count = 0;
  do {
    buffer[--index] = digits[value & 0xFu];
    value >>= 4u;
    digit_count++;
  } while ((value != 0 || digit_count < width) && index > 2);

  buffer[--index] = 'x';
  buffer[--index] = '0';
  mettle_crash_write_stderr(buffer + index);
}

static void mettle_crash_write_pointer(const void *value) {
  mettle_crash_write_hex_uintptr((uintptr_t)value, sizeof(uintptr_t) * 2);
}

#if defined(_WIN32) || defined(_WIN64)
static const char *mettle_crash_exception_name(DWORD code) {
  switch (code) {
  case EXCEPTION_ACCESS_VIOLATION:
    return "access violation";
  case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    return "array bounds exceeded";
  case EXCEPTION_BREAKPOINT:
    return "breakpoint";
  case EXCEPTION_DATATYPE_MISALIGNMENT:
    return "datatype misalignment";
  case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    return "floating-point divide by zero";
  case EXCEPTION_ILLEGAL_INSTRUCTION:
    return "illegal instruction";
  case EXCEPTION_IN_PAGE_ERROR:
    return "in-page error";
  case EXCEPTION_INT_DIVIDE_BY_ZERO:
    return "integer divide by zero";
  case EXCEPTION_STACK_OVERFLOW:
    return "stack overflow";
  case STATUS_HEAP_CORRUPTION:
    return "heap corruption";
  case STATUS_STACK_BUFFER_OVERRUN:
    return "stack buffer overrun";
  default:
    return "unknown exception";
  }
}
#else
/* Process-lifetime pipe used as an async-signal-safe readability probe:
   write(2) returns EFAULT instead of faulting when the buffer is unmapped. */
static int g_runtime_probe_pipe[2] = {-1, -1};

static const char *mettle_crash_signal_name(int signo) {
  switch (signo) {
  case SIGSEGV:
    return "segmentation fault (invalid memory access)";
  case SIGBUS:
    return "bus error (misaligned or invalid memory access)";
  case SIGFPE:
    return "arithmetic exception (e.g. integer divide by zero)";
  case SIGILL:
    return "illegal instruction";
  case SIGABRT:
    return "aborted";
  default:
    return "fatal signal";
  }
}
#endif

static int mettle_crash_address_is_readable(const void *address,
                                            size_t length) {
  if (!address || length == 0) {
    return 0;
  }

#if defined(_WIN32) || defined(_WIN64)
  MEMORY_BASIC_INFORMATION info;
  if (VirtualQuery(address, &info, sizeof(info)) == 0) {
    return 0;
  }
  if (info.State != MEM_COMMIT) {
    return 0;
  }
  if ((info.Protect & PAGE_GUARD) != 0 || info.Protect == PAGE_NOACCESS) {
    return 0;
  }

  uintptr_t start = (uintptr_t)address;
  uintptr_t region_start = (uintptr_t)info.BaseAddress;
  uintptr_t region_end = region_start + info.RegionSize;
  return start >= region_start && start + length <= region_end;
#else
  /* Probe by attempting a non-blocking write of the bytes into a drained
     pipe. The kernel validates the source buffer and returns EFAULT for
     unmapped pages without ever dereferencing them in our process. */
  if (g_runtime_probe_pipe[1] < 0) {
    return 0;
  }
  for (;;) {
    ssize_t n = write(g_runtime_probe_pipe[1], address, length);
    if (n >= 0) {
      /* Drain whatever we just wrote so the pipe never fills. */
      char sink[64];
      ssize_t remaining = n;
      while (remaining > 0) {
        ssize_t got = read(g_runtime_probe_pipe[0], sink,
                           sizeof(sink) < (size_t)remaining ? sizeof(sink)
                                                            : (size_t)remaining);
        if (got <= 0) {
          break;
        }
        remaining -= got;
      }
      return 1;
    }
    if (errno == EINTR) {
      continue;
    }
    /* EFAULT => unreadable; EAGAIN (full pipe) => treat as unknown/stop. */
    return 0;
  }
#endif
}

static const MettleCrashFunctionInfo *
mettle_crash_find_function(uintptr_t program_counter) {
  for (size_t i = 0; i < g_runtime_debug_function_count; i++) {
    const MettleCrashFunctionInfo *info = &g_runtime_debug_functions[i];
    uintptr_t start = (uintptr_t)info->start_address;
    uintptr_t end = (uintptr_t)info->end_address;
    if (program_counter >= start && program_counter < end) {
      return info;
    }
  }
  return NULL;
}

static const MettleCrashLocationInfo *
mettle_crash_find_location(uintptr_t program_counter,
                           const MettleCrashFunctionInfo *function_info) {
  const MettleCrashLocationInfo *best = NULL;
  uintptr_t best_address = 0;
  uintptr_t function_start = function_info ? (uintptr_t)function_info->start_address : 0;
  uintptr_t function_end =
      function_info ? (uintptr_t)function_info->end_address : UINTPTR_MAX;

  for (size_t i = 0; i < g_runtime_debug_location_count; i++) {
    const MettleCrashLocationInfo *info = &g_runtime_debug_locations[i];
    uintptr_t address = (uintptr_t)info->address;
    if (address < function_start || address >= function_end) {
      continue;
    }
    if (address <= program_counter && (!best || address >= best_address)) {
      best = info;
      best_address = address;
    }
  }

  return best;
}

static void mettle_crash_print_frame(size_t index, uintptr_t program_counter) {
  const MettleCrashFunctionInfo *function_info =
      mettle_crash_find_function(program_counter);
  const MettleCrashLocationInfo *location_info =
      mettle_crash_find_location(program_counter, function_info);
  const char *function_name = "<unknown>";
  const char *filename = NULL;
  uintptr_t line = 0;
  uintptr_t column = 0;

  if (location_info) {
    function_name =
        location_info->function_name ? location_info->function_name : function_name;
    filename = location_info->filename;
    line = location_info->line;
    column = location_info->column;
  } else if (function_info) {
    function_name =
        function_info->function_name ? function_info->function_name : function_name;
    filename = function_info->filename;
    line = function_info->line;
    column = function_info->column;
  }

  if (filename && line > 0) {
    mettle_crash_write_stderr("  #");
    mettle_crash_write_decimal_uintptr((uintptr_t)index);
    mettle_crash_write_stderr(" ");
    mettle_crash_write_stderr(function_name);
    mettle_crash_write_stderr(" at ");
    mettle_crash_write_stderr(filename);
    mettle_crash_write_stderr(":");
    mettle_crash_write_decimal_uintptr(line);
    mettle_crash_write_stderr(":");
    mettle_crash_write_decimal_uintptr(column);
    mettle_crash_write_stderr(" (");
    mettle_crash_write_pointer((void *)program_counter);
    mettle_crash_write_stderr(")\r\n");
  } else {
    mettle_crash_write_stderr("  #");
    mettle_crash_write_decimal_uintptr((uintptr_t)index);
    mettle_crash_write_stderr(" ");
    mettle_crash_write_stderr(function_name);
    mettle_crash_write_stderr(" (");
    mettle_crash_write_pointer((void *)program_counter);
    mettle_crash_write_stderr(")\r\n");
  }
}

static void mettle_crash_print_trace_from_frame(uintptr_t program_counter,
                                                uintptr_t frame_pointer) {
  mettle_crash_write_stderr("Stack trace:\r\n");
  mettle_crash_print_frame(0, program_counter);

  uintptr_t current_frame = frame_pointer;
  for (size_t index = 1; index < 32; index++) {
    uintptr_t next_frame = 0;
    uintptr_t return_address = 0;

    if (!current_frame ||
        !mettle_crash_address_is_readable((const void *)current_frame,
                                          sizeof(uintptr_t) * 2)) {
      break;
    }

    next_frame = *((uintptr_t *)current_frame);
    return_address = *(((uintptr_t *)current_frame) + 1);

    if (next_frame <= current_frame || return_address == 0) {
      break;
    }

    mettle_crash_print_frame(index, return_address - 1u);
    current_frame = next_frame;
  }
}

#if defined(_WIN32) || defined(_WIN64)
static void mettle_crash_terminate_with_code(UINT exit_code) {
  HANDLE process = GetCurrentProcess();
  TerminateProcess(process, exit_code);
}

static LONG WINAPI
mettle_crash_unhandled_exception_filter(EXCEPTION_POINTERS *exception_info) {
  if (!exception_info || !exception_info->ExceptionRecord) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  if (InterlockedExchange(&g_runtime_debug_in_handler, 1) != 0) {
    mettle_crash_terminate_with_code(1);
  }

  const EXCEPTION_RECORD *record = exception_info->ExceptionRecord;
  const CONTEXT *context = exception_info->ContextRecord;
  uintptr_t program_counter = (uintptr_t)record->ExceptionAddress;
  uintptr_t frame_pointer = context ? (uintptr_t)context->Rbp : 0;

  mettle_crash_write_stderr("Unhandled runtime exception ");
  mettle_crash_write_hex_uintptr((uintptr_t)record->ExceptionCode, 8);
  mettle_crash_write_stderr(" (");
  mettle_crash_write_stderr(
      mettle_crash_exception_name(record->ExceptionCode));
  mettle_crash_write_stderr(")\r\n");
  mettle_crash_write_stderr("Exception address: ");
  mettle_crash_write_pointer((void *)program_counter);
  mettle_crash_write_stderr("\r\n");

  if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
      record->NumberParameters >= 2) {
    const char *operation = "read";
    if (record->ExceptionInformation[0] == 1u) {
      operation = "write";
    } else if (record->ExceptionInformation[0] == 8u) {
      operation = "execute";
    }
    mettle_crash_write_stderr(operation);
    mettle_crash_write_stderr(" access violation at ");
    mettle_crash_write_pointer((void *)record->ExceptionInformation[1]);
    mettle_crash_write_stderr("\r\n");
  }

  mettle_crash_print_trace_from_frame(program_counter, frame_pointer);
  mettle_crash_terminate_with_code(1);
  return EXCEPTION_EXECUTE_HANDLER;
}
#else /* POSIX */

static void mettle_crash_terminate_with_code(int exit_code) {
  /* _exit(2) is async-signal-safe and skips atexit/stdio flushing, which is
     exactly what we want from inside a fatal signal handler. */
  _exit(exit_code);
}

/* Recover the faulting instruction pointer and frame pointer from the
   signal's machine context so the backtrace starts at the real crash site
   rather than inside the handler. Layout is arch/OS specific; we cover the
   common x86-64 Linux/macOS cases and degrade gracefully otherwise. */
static void mettle_crash_extract_fault_context(void *ucontext_raw,
                                               uintptr_t *out_pc,
                                               uintptr_t *out_fp) {
  *out_pc = 0;
  *out_fp = 0;
#if defined(__linux__) && defined(__x86_64__)
  ucontext_t *uc = (ucontext_t *)ucontext_raw;
  if (uc) {
    *out_pc = (uintptr_t)uc->uc_mcontext.gregs[REG_RIP];
    *out_fp = (uintptr_t)uc->uc_mcontext.gregs[REG_RBP];
  }
#elif defined(__APPLE__) && defined(__x86_64__)
  ucontext_t *uc = (ucontext_t *)ucontext_raw;
  if (uc && uc->uc_mcontext) {
    *out_pc = (uintptr_t)uc->uc_mcontext->__ss.__rip;
    *out_fp = (uintptr_t)uc->uc_mcontext->__ss.__rbp;
  }
#elif defined(__linux__) && defined(__aarch64__)
  ucontext_t *uc = (ucontext_t *)ucontext_raw;
  if (uc) {
    *out_pc = (uintptr_t)uc->uc_mcontext.pc;
    *out_fp = (uintptr_t)uc->uc_mcontext.regs[29];
  }
#else
  (void)ucontext_raw;
#endif
}

static void mettle_crash_crash_signal_handler(int signo, siginfo_t *info,
                                               void *ucontext_raw) {
  /* Reentrancy guard: a fault inside the handler must hard-exit. */
  if (g_runtime_debug_in_handler) {
    mettle_crash_terminate_with_code(128 + signo);
  }
  g_runtime_debug_in_handler = 1;

  mettle_crash_write_stderr("Unhandled runtime signal ");
  mettle_crash_write_decimal_uintptr((uintptr_t)signo);
  mettle_crash_write_stderr(" (");
  mettle_crash_write_stderr(mettle_crash_signal_name(signo));
  mettle_crash_write_stderr(")\n");

  if (info && (signo == SIGSEGV || signo == SIGBUS)) {
    mettle_crash_write_stderr("Faulting address: ");
    mettle_crash_write_pointer(info->si_addr);
    if (info->si_addr == NULL) {
      mettle_crash_write_stderr("  (null pointer dereference)");
    }
    mettle_crash_write_stderr("\n");
  }

  uintptr_t program_counter = 0;
  uintptr_t frame_pointer = 0;
  mettle_crash_extract_fault_context(ucontext_raw, &program_counter,
                                     &frame_pointer);
  if (program_counter != 0) {
    mettle_crash_write_stderr("Fault instruction: ");
    mettle_crash_write_pointer((void *)program_counter);
    mettle_crash_write_stderr("\n");
    mettle_crash_print_trace_from_frame(program_counter, frame_pointer);
  } else {
    mettle_crash_write_stderr(
        "Stack trace unavailable (no machine context for this platform)\n");
  }

  mettle_crash_terminate_with_code(128 + signo);
}
#endif

void mettle_crash_register_image(const MettleCrashFunctionInfo *functions,
                                       size_t function_count,
                                       const MettleCrashLocationInfo *locations,
                                       size_t location_count) {
  g_runtime_debug_functions = functions;
  g_runtime_debug_function_count = function_count;
  g_runtime_debug_locations = locations;
  g_runtime_debug_location_count = location_count;
}

void mettle_crash_install(void) {
#if defined(_WIN32) || defined(_WIN64)
  if (InterlockedCompareExchange(&g_runtime_debug_handler_installed, 1, 0) == 0) {
    g_runtime_debug_vectored_handler =
        AddVectoredExceptionHandler(1, mettle_crash_unhandled_exception_filter);
    SetUnhandledExceptionFilter(mettle_crash_unhandled_exception_filter);
  }
#else
  if (g_runtime_debug_handler_installed) {
    return;
  }
  g_runtime_debug_handler_installed = 1;

  /* Readability-probe pipe (used while walking frame pointers). Failure to
     create it is non-fatal: the trace just degrades to fewer frames. */
  if (g_runtime_probe_pipe[0] < 0) {
    if (pipe(g_runtime_probe_pipe) == 0) {
      (void)fcntl(g_runtime_probe_pipe[0], F_SETFL, O_NONBLOCK);
      (void)fcntl(g_runtime_probe_pipe[1], F_SETFL, O_NONBLOCK);
    } else {
      g_runtime_probe_pipe[0] = -1;
      g_runtime_probe_pipe[1] = -1;
    }
  }

  /* Run the handler on its own stack so SIGSEGV caused by stack overflow can
     still be reported instead of silently re-faulting. A fixed 64 KiB block
     comfortably exceeds MINSIGSTKSZ on supported targets; SIGSTKSZ is not a
     compile-time constant on modern glibc so we cannot size from it. */
  static char alt_stack_storage[64 * 1024];
  stack_t alt_stack;
  alt_stack.ss_sp = alt_stack_storage;
  alt_stack.ss_size = sizeof(alt_stack_storage);
  alt_stack.ss_flags = 0;
  (void)sigaltstack(&alt_stack, NULL);

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = mettle_crash_crash_signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO | SA_ONSTACK;

  /* SIGABRT deliberately omitted from SA_RESETHAND-style chaining: we want a
     clean trace then exit. Each fatal fault gets the same treatment. */
  (void)sigaction(SIGSEGV, &sa, NULL);
  (void)sigaction(SIGBUS, &sa, NULL);
  (void)sigaction(SIGFPE, &sa, NULL);
  (void)sigaction(SIGILL, &sa, NULL);
  (void)sigaction(SIGABRT, &sa, NULL);
#endif
}

void mettle_crash_trap(const char *message, const void *program_counter,
                             const void *frame_pointer) {
#if defined(_WIN32) || defined(_WIN64)
  if (InterlockedExchange(&g_runtime_debug_in_handler, 1) != 0) {
    mettle_crash_terminate_with_code(1);
    return;
  }

  mettle_crash_write_stderr(message ? message : "Fatal runtime trap");
  mettle_crash_write_stderr("\r\n");
  mettle_crash_print_trace_from_frame((uintptr_t)program_counter,
                                      (uintptr_t)frame_pointer);
  mettle_crash_terminate_with_code(1);
#else
  if (g_runtime_debug_in_handler) {
    mettle_crash_terminate_with_code(1);
    return;
  }
  g_runtime_debug_in_handler = 1;

  mettle_crash_write_stderr(
      (message && message[0] != '\0') ? message : "Fatal runtime trap");
  mettle_crash_write_stderr("\n");
  if (program_counter || frame_pointer) {
    mettle_crash_print_trace_from_frame((uintptr_t)program_counter,
                                        (uintptr_t)frame_pointer);
  }
  mettle_crash_terminate_with_code(1);
#endif
}

