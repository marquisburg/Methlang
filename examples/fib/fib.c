/*
 * C Fibonacci benchmark - counterpart to fib.mettle
 *
 * Prints fib(0)..fib(35) in one forward pass, then benchmarks fib(35) x 10M.
 *
 * Build: build.bat (or: gcc -O2 -o fib_c.exe fib.c -lkernel32)
 * Run: fib_c.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "../bench_time.h"

static int64_t fib(int32_t n) {
    if (n <= 1) {
        return (int64_t)n;
    }
    int64_t a = 0;
    int64_t b = 1;
    for (int32_t i = 2; i <= n; i++) {
        int64_t next = a + b;
        a = b;
        b = next;
    }
    return b;
}

int main(void) {
    const int32_t iter = 10000000;

    uint64_t t0 = bench_time_us();
    int64_t bench_sum = 0;
    for (int32_t j = 0; j < iter; j++) {
        bench_sum += fib(35);
    }
    uint64_t elapsed_us = bench_time_us() - t0;

    int64_t check = fib(35);

    printf("Fibonacci 0..35:\n");

    int64_t a = 0;
    int64_t b = 1;
    printf("%" PRId64, a);
    for (int32_t n = 1; n <= 35; n++) {
        printf(" %" PRId64, b);
        int64_t next = a + b;
        a = b;
        b = next;
    }
    printf("\n");

    printf("Benchmark: fib(35) x 10,000,000\n");
    printf("fib(35) = %" PRId64 "\n", check);
    printf("Bench sum mod check = %" PRId64 "\n", bench_sum % check);
    printf("Time: %" PRIu64 " us\n", elapsed_us);

    uint64_t per_call = elapsed_us * 1000 / (uint64_t)iter;
    printf("Per call: ~%" PRIu64 " ns\n", per_call);

    return 0;
}
