/**
 * methlang_entry.c - Provides argc/argv to Methlang main() when it has the
 * signature: function main(argc: int32, argv: cstring*) -> int32
 *
 * On Windows: Parses the command line via GetCommandLineA and fills argc/argv.
 * On Linux: The kernel passes argc in rdi and argv in rsi to _start; we don't
 * use this file - the emitted assembly passes them directly.
 *
 * Link this file when building programs with main(argc, argv).
 */

#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)

#include <windows.h>
#include <shellapi.h>

/* Parse command line into argc/argv. Caller provides pointers to receive them.
 * Allocates argv and each argument string with malloc. Caller does not need to
 * free (process exits); for long-running programs, consider adding cleanup. */
void methlang_entry_get_args(int *argc, char ***argv) {
  if (!argc || !argv) {
    return;
  }

  LPWSTR cmdline_w = GetCommandLineW();
  if (!cmdline_w) {
    *argc = 0;
    *argv = NULL;
    return;
  }

  int nargs = 0;
  LPWSTR *args_w = CommandLineToArgvW(cmdline_w, &nargs);
  if (!args_w || nargs <= 0) {
    *argc = 0;
    *argv = NULL;
    return;
  }

  /* Allocate argv array: (nargs+1) pointers, then NULL terminator */
  char **out_argv = (char **)malloc((size_t)(nargs + 1) * sizeof(char *));
  if (!out_argv) {
    LocalFree(args_w);
    *argc = 0;
    *argv = NULL;
    return;
  }

  for (int i = 0; i < nargs; i++) {
    /* Convert wide to UTF-8. Use WideCharToMultiByte. */
    int wlen = (int)wcslen(args_w[i]);
    int mlen = WideCharToMultiByte(CP_UTF8, 0, args_w[i], wlen, NULL, 0, NULL, NULL);
    if (mlen <= 0) {
      mlen = 1;
    }
    char *buf = (char *)malloc((size_t)mlen + 1);
    if (!buf) {
      for (int j = 0; j < i; j++) {
        free(out_argv[j]);
      }
      free(out_argv);
      LocalFree(args_w);
      *argc = 0;
      *argv = NULL;
      return;
    }
    WideCharToMultiByte(CP_UTF8, 0, args_w[i], wlen, buf, mlen + 1, NULL, NULL);
    buf[mlen] = '\0';
    out_argv[i] = buf;
  }
  out_argv[nargs] = NULL;

  LocalFree(args_w);

  *argc = nargs;
  *argv = out_argv;
}

#else

/* Linux/macOS: The kernel passes argc in rdi and argv in rsi to _start.
 * The emitted assembly passes them directly to main; we don't call this.
 * Provide a no-op stub so the symbol exists when linking (some build
 * configs might link this on Linux too). */
void methlang_entry_get_args(int *argc, char ***argv) {
  (void)argc;
  (void)argv;
  /* Unused on Linux - assembly passes rdi/rsi directly */
}

#endif
