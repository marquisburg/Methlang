#include "compiler_crash.h"
#include "compiler_context.h"
#include "../runtime/crash_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#endif

static int g_compiler_crash_installed = 0;
static int g_compiler_in_ice_handler = 0;
static char **g_compiler_argv = NULL;
static int g_compiler_argc = 0;

static void mettle_compiler_write_backtrace(FILE *output) {
  if (!output) {
    return;
  }

  fprintf(output, "\nCompiler backtrace:\n");

#if defined(_WIN32) || defined(_WIN64)
  void *frames[64];
  USHORT frame_count = 0;
  HANDLE process = GetCurrentProcess();
  char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
  SYMBOL_INFO *symbol = (SYMBOL_INFO *)symbol_buffer;

  SymInitialize(process, NULL, TRUE);
  frame_count = CaptureStackBackTrace(0, 64, frames, NULL);
  symbol->MaxNameLen = MAX_SYM_NAME;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

  for (USHORT i = 0; i < frame_count; i++) {
    DWORD64 displacement = 0;
    if (SymFromAddr(process, (DWORD64)(uintptr_t)frames[i], &displacement,
                    symbol)) {
      fprintf(output, "  %s\n", symbol->Name);
    } else {
      fprintf(output, "  %p\n", frames[i]);
    }
  }
  SymCleanup(process);
#else
  void *frames[64];
  int frame_count = backtrace(frames, 64);
  char **symbols = backtrace_symbols(frames, frame_count);
  if (symbols) {
    for (int i = 0; i < frame_count; i++) {
      fprintf(output, "  %s\n", symbols[i]);
    }
    free(symbols);
  }
#endif
}

void mettle_compiler_ice_report(const char *reason, const char *detail) {
  if (g_compiler_in_ice_handler) {
    fprintf(stderr, "Mettle internal compiler error (recursive)\n");
    return;
  }

  g_compiler_in_ice_handler = 1;
  mettle_compiler_ctx_write_snapshot();
  mettle_compiler_ctx_write_report(stderr, reason, detail);
  mettle_compiler_write_backtrace(stderr);
  g_compiler_in_ice_handler = 0;
}

void mettle_compiler_ice(const char *reason) {
  mettle_compiler_ice_report(reason, NULL);
  abort();
}

#if defined(_WIN32) || defined(_WIN64)
static LONG WINAPI mettle_compiler_unhandled_exception_filter(
    EXCEPTION_POINTERS *info) {
  char detail[64];

  if (g_compiler_in_ice_handler) {
    ExitProcess(3);
  }

  if (!info || !info->ExceptionRecord) {
    mettle_compiler_ice_report("fatal exception", NULL);
    ExitProcess(3);
  }

  snprintf(detail, sizeof(detail), "0x%08lX",
           (unsigned long)info->ExceptionRecord->ExceptionCode);
  mettle_compiler_ice_report(
      mettle_crash_exception_name(info->ExceptionRecord->ExceptionCode),
      detail);
  ExitProcess(3);
  return EXCEPTION_EXECUTE_HANDLER;
}
#else
static void mettle_compiler_signal_handler(int signo, siginfo_t *info,
                                           void *ucontext_raw) {
  char detail[64];
  const char *reason = "fatal signal";

  (void)ucontext_raw;

  if (g_compiler_in_ice_handler) {
    _exit(128 + signo);
  }

  switch (signo) {
  case SIGSEGV:
    reason = "segmentation fault";
    break;
  case SIGBUS:
    reason = "bus error";
    break;
  case SIGFPE:
    reason = "floating-point exception";
    break;
  case SIGILL:
    reason = "illegal instruction";
    break;
  case SIGABRT:
    reason = "abort";
    break;
  default:
    break;
  }

  if (info && info->si_addr) {
    snprintf(detail, sizeof(detail), "at %p", info->si_addr);
    mettle_compiler_ice_report(reason, detail);
  } else {
    mettle_compiler_ice_report(reason, NULL);
  }
  _exit(128 + signo);
}
#endif

void mettle_compiler_crash_install(int argc, char **argv) {
  if (g_compiler_crash_installed) {
    return;
  }
  g_compiler_crash_installed = 1;
  g_compiler_argc = argc;
  g_compiler_argv = argv;

#if defined(_WIN32) || defined(_WIN64)
  SetUnhandledExceptionFilter(mettle_compiler_unhandled_exception_filter);
#else
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = mettle_compiler_signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGBUS, &sa, NULL);
  sigaction(SIGFPE, &sa, NULL);
  sigaction(SIGILL, &sa, NULL);
  sigaction(SIGABRT, &sa, NULL);
#endif
}
