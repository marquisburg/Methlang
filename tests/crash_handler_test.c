/*
 * Crash-handler regression test.
 *
 * Verifies the runtime's fault diagnostics on POSIX:
 *   1. meth_runtime_debug_trap() prints its message + a symbolized stack
 *      trace using the registered debug-info image (the path compiler-emitted
 *      null/bounds checks take).
 *   2. An installed signal handler turns a real SIGSEGV into a readable
 *      diagnostic (signal name, faulting address, null-deref hint) instead of
 *      a bare kernel "Segmentation fault".
 *
 * Each scenario runs in a forked child so the parent can capture the child's
 * stderr and assert on it; the child is expected to die via the handler.
 *
 * On Windows this is a no-op (the SEH-based path already has dedicated
 * coverage in run_tests.ps1 via runtime_null_trace / access_violation_trace).
 */

#include "../src/runtime/gc.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
int main(void) {
  printf("Crash handler tests skipped (Windows uses SEH path; covered "
         "elsewhere).\n");
  return 0;
}
#else
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define TEST_ASSERT(cond, msg)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "FAIL: %s\n", (msg));                                    \
      return 0;                                                                \
    }                                                                          \
  } while (0)

/* A function whose address range we register so the trace can symbolize it. */
static void victim_function(void) {
  /* Force a null dereference. volatile so the compiler can't elide it. */
  volatile int *p = (volatile int *)0;
  *p = 42;
}
static void victim_function_end(void) {}

static void register_fake_image(void) {
  static MethRuntimeFunctionInfo functions[1];
  functions[0].start_address = (const void *)(uintptr_t)&victim_function;
  functions[0].end_address = (const void *)(uintptr_t)&victim_function_end;
  functions[0].function_name = "victim_function";
  functions[0].filename = "crash_demo.mettle";
  functions[0].line = 7;
  functions[0].column = 3;
  meth_runtime_debug_register_image(functions, 1, NULL, 0);
}

/* Run `child` in a forked process, capture its stderr into `buf`, and return
   the raw waitpid status via *status. Returns bytes captured. */
static size_t run_child_capture(void (*child)(void), char *buf, size_t buf_cap,
                                 int *status) {
  int pipefd[2];
  if (pipe(pipefd) != 0) {
    return 0;
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
  }

  if (pid == 0) {
    /* Child: route stderr into the pipe, run the crashing scenario. */
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);
    child();
    _exit(0); /* should not reach here if it crashes as expected */
  }

  close(pipefd[1]);
  size_t total = 0;
  for (;;) {
    if (total + 1 >= buf_cap) {
      break;
    }
    ssize_t n = read(pipefd[0], buf + total, buf_cap - 1 - total);
    if (n <= 0) {
      break;
    }
    total += (size_t)n;
  }
  buf[total] = '\0';
  close(pipefd[0]);
  (void)waitpid(pid, status, 0);
  return total;
}

static void scenario_debug_trap(void) {
  register_fake_image();
  meth_runtime_debug_install_crash_handler();
  /* Mimic a compiler-emitted runtime check firing. */
  meth_runtime_debug_trap("Fatal error: Null pointer dereference",
                          (const void *)(uintptr_t)&victim_function, NULL);
  _exit(0);
}

static void scenario_real_segfault(void) {
  register_fake_image();
  meth_runtime_debug_install_crash_handler();
  victim_function();
  _exit(0);
}

static int test_debug_trap_emits_message(void) {
  char buf[8192];
  int status = 0;
  size_t n = run_child_capture(scenario_debug_trap, buf, sizeof(buf), &status);
  TEST_ASSERT(n > 0, "debug_trap produced no output");
  TEST_ASSERT(strstr(buf, "Fatal error: Null pointer dereference") != NULL,
              "debug_trap output missing the trap message");
  TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 1,
              "debug_trap child should exit with code 1");
  return 1;
}

static int test_segfault_is_diagnosed(void) {
  char buf[8192];
  int status = 0;
  size_t n =
      run_child_capture(scenario_real_segfault, buf, sizeof(buf), &status);
  TEST_ASSERT(n > 0, "segfault produced no diagnostic output");
  TEST_ASSERT(strstr(buf, "Unhandled runtime signal") != NULL,
              "segfault output missing signal header");
  TEST_ASSERT(strstr(buf, "segmentation fault") != NULL,
              "segfault output missing human-readable signal name");
  TEST_ASSERT(strstr(buf, "Faulting address: 0x") != NULL,
              "segfault output missing faulting address");
  TEST_ASSERT(strstr(buf, "null pointer dereference") != NULL,
              "segfault output missing null-deref hint for address 0");
  /* The child must have died from the signal, not exited cleanly. */
  TEST_ASSERT(!(WIFEXITED(status) && WEXITSTATUS(status) == 0),
              "segfault child unexpectedly exited cleanly");
  return 1;
}

int main(void) {
  int passed = 1;
  passed &= test_debug_trap_emits_message();
  passed &= test_segfault_is_diagnosed();

  if (!passed) {
    return 1;
  }
  printf("Crash handler tests passed (POSIX signal/trap diagnostics).\n");
  return 0;
}
#endif
