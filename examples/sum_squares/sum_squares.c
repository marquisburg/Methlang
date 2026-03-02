/*
 * C Sum-of-Squares benchmark - counterpart to sum_squares.meth
 *
 * Computes 1² + 2² + ... + n² for n=100000.
 * Benchmarks 50 full passes.
 *
 * Build: build.bat (or: gcc -O2 -o sum_squares_c.exe sum_squares.c -lkernel32)
 * Run: sum_squares_c.exe
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

static int64_t sum_squares(int64_t n) {
    int64_t sum = 0;
    for (int64_t i = 1; i <= n; i++) {
        sum += i * i;
    }
    return sum;
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
    printf("Sum of squares: 1²+2²+...+100000²\n");

    int64_t result = sum_squares(100000);
    printf("Sum = %" PRId64 "\n", result);

    const int passes = 500;
    printf("Benchmark: 500 passes (sum_squares 100000 each)\n");

    uint64_t t0 = get_time_ms();
    int64_t bench_sum = 0;
    for (int p = 0; p < passes; p++) {
        bench_sum += sum_squares(100000);
    }
    uint64_t t1 = get_time_ms();
    uint64_t elapsed_ms = t1 - t0;

    printf("Bench sum = %" PRId64 "\n", bench_sum);
    printf("Time: %" PRIu64 " ms\n", elapsed_ms);

    uint64_t per_pass_us = elapsed_ms * 1000 / passes;
    printf("Per pass: ~%" PRIu64 " us\n", per_pass_us);

    return 0;
}
