/*
 * C Word Count benchmark - counterpart to word_count.mettle
 *
 * Parses a raw byte buffer to count words (non-empty runs of non-whitespace).
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

static int is_space(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int64_t word_count(const unsigned char *buf, int64_t len) {
    int64_t count = 0;
    int in_word = 0;
    for (int64_t i = 0; i < len; i++) {
        unsigned char c = buf[i];
        if (is_space(c)) {
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
    const char *template = "a b ";
    const size_t template_len = 4;

    unsigned char *buf = (unsigned char *)malloc((size_t)buf_size);
    if (!buf) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    for (int64_t pos = 0; pos < buf_size; ) {
        size_t chunk = (size_t)(buf_size - pos);
        if (chunk > template_len) {
            chunk = template_len;
        }
        memcpy(buf + pos, template, chunk);
        pos += (int64_t)chunk;
    }

    int64_t wc = word_count(buf, buf_size);

    const int passes = 500;
    uint64_t t0 = get_time_ms();
    int64_t total = 0;
    for (int p = 0; p < passes; p++) {
        total += word_count(buf, buf_size);
    }
    uint64_t t1 = get_time_ms();
    uint64_t elapsed_ms = t1 - t0;

    printf("Word count: 256 KB buffer (a b pattern)\n");
    printf("Words = %" PRId64 "\n", wc);
    printf("Benchmark: 500 passes (word_count)\n");
    printf("Total words = %" PRId64 "\n", total);
    printf("Time: %" PRIu64 " ms\n", elapsed_ms);

    uint64_t per_pass_us = elapsed_ms * 1000 / (uint64_t)passes;
    printf("Per pass: ~%" PRIu64 " us\n", per_pass_us);

    free(buf);
    return 0;
}
