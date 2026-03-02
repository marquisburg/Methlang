/*
 * C Fibonacci benchmark - counterpart to fib.meth
 *
 * Computes fib(0) through fib(35) and prints them.
 * Benchmarks fib(35) with 1,000,000 iterations.
 *
 * Build: build.bat (or: gcc -O2 -o fib_c.exe fib.c -lkernel32)
 * Run: fib_c.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/time.h>
#endif

/* Iterative Fibonacci: returns fib(n) for n >= 0 */
static int64_t fib(int32_t n) {
    if (n <= 1) {
        return (int64_t)n;
    }
    int64_t a = 0;
    int64_t b = 1;
    int32_t i = 2;
    while (i <= n) {
        int64_t next = a + b;
        a = b;
        b = next;
        i++;
    }
    return b;
}

#ifdef _WIN32
/* Match Methlang: use GetTickCount64 for apples-to-apples benchmark */
static uint64_t get_time_ms(void) {
    return (uint64_t)GetTickCount64();
}
#else
static uint64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}
#endif

int main(void) {
    printf("Fibonacci 0..35:\n");

    for (int32_t n = 0; n <= 35; n++) {
        printf("%" PRId64 "%s", (int64_t)fib(n), n < 35 ? " " : "");
    }
    printf("\n");

    /* Benchmark: fib(35) x 10,000,000 (matches Methlang, enough for GetTickCount64) */
    const int32_t iter = 10000000;
    printf("Benchmark: fib(35) x 10,000,000\n");

    uint64_t t0 = get_time_ms();
    int64_t dummy = 0;
    for (int32_t j = 0; j < iter; j++) {
        dummy = fib(35);
    }
    uint64_t t1 = get_time_ms();
    uint64_t elapsed_ms = t1 - t0;

    printf("fib(35) = %" PRId64 "\n", dummy);
    printf("Time: %" PRIu64 " ms\n", elapsed_ms);

    uint64_t per_call = elapsed_ms * 1000000 / iter;
    printf("Per call: ~%" PRIu64 " ns\n", per_call);

    return 0;
}
