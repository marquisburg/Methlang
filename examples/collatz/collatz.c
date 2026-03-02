/*
 * C Collatz benchmark - counterpart to collatz.meth
 *
 * Collatz sequence: n -> n/2 (if even) or 3n+1 (if odd), until n==1.
 * Counts steps for n=1..100000, sums them. Benchmarks 10 full passes.
 *
 * Build: build.bat (or: gcc -O2 -o collatz_c.exe collatz.c -lkernel32)
 * Run: collatz_c.exe
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

/* Returns number of steps to reach 1 from n (n >= 1) */
static int64_t collatz_steps(int64_t n) {
    int64_t count = 0;
    int64_t x = n;
    while (x > 1) {
        if (x % 2 == 0) {
            x = x / 2;
        } else {
            x = 3 * x + 1;
        }
        count++;
    }
    return count;
}

#ifdef _WIN32
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
    printf("Collatz: sum of steps for n=1..100000\n");

    int64_t sum = 0;
    for (int64_t n = 1; n <= 100000; n++) {
        sum += collatz_steps(n);
    }
    printf("Sum (1..100000) = %" PRId64 "\n", sum);

    const int passes = 10;
    printf("Benchmark: 10 passes (sum steps 1..100000 each)\n");

    uint64_t t0 = get_time_ms();
    int64_t bench_sum = 0;
    for (int p = 0; p < passes; p++) {
        for (int64_t i = 1; i <= 100000; i++) {
            bench_sum += collatz_steps(i);
        }
    }
    uint64_t t1 = get_time_ms();
    uint64_t elapsed_ms = t1 - t0;

    printf("Bench sum = %" PRId64 "\n", bench_sum);
    printf("Time: %" PRIu64 " ms\n", elapsed_ms);

    uint64_t per_pass_us = elapsed_ms * 1000 / passes;
    printf("Per pass: ~%" PRIu64 " us\n", per_pass_us);

    return 0;
}
