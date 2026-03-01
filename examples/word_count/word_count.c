/*
 * C Word Count benchmark - counterpart to word_count.masm
 *
 * Parses an in-memory buffer character-by-character to count words.
 * Exercises: malloc, buffer iteration, character classification, branching.
 *
 * Build: build.bat (or: gcc -O2 -o word_count_c.exe word_count.c -lkernel32)
 * Run: word_count_c.exe
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/time.h>
#endif

static int64_t word_count(const char *buf, int64_t len) {
    int64_t count = 0;
    int in_word = 0;
    for (int64_t i = 0; i < len; i++) {
        int c = (unsigned char)buf[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            in_word = 0;
        } else {
            if (!in_word) {
                count++;
            }
            in_word = 1;
        }
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
    const int64_t buf_size = 262144;
    char *buf = (char *)malloc((size_t)buf_size);
    if (!buf) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    const char *template = "a b ";
    const size_t tlen = 4;
    for (int64_t pos = 0; pos < buf_size; pos += tlen) {
        size_t chunk = (size_t)(buf_size - pos < (int64_t)tlen ? buf_size - pos : tlen);
        memcpy(buf + pos, template, chunk);
    }

    printf("Word count: 256 KB buffer (a b pattern)\n");

    int64_t wc = word_count(buf, buf_size);
    printf("Words = %" PRId64 "\n", wc);

    const int passes = 500;
    printf("Benchmark: 500 passes (word_count)\n");

    uint64_t t0 = get_time_ms();
    int64_t total = 0;
    for (int p = 0; p < passes; p++) {
        total += word_count(buf, buf_size);
    }
    uint64_t t1 = get_time_ms();
    uint64_t elapsed_ms = t1 - t0;

    printf("Total words = %" PRId64 "\n", total);
    printf("Time: %" PRIu64 " ms\n", elapsed_ms);

    uint64_t per_pass_us = elapsed_ms * 1000 / passes;
    printf("Per pass: ~%" PRIu64 " us\n", per_pass_us);

    free(buf);
    return 0;
}
