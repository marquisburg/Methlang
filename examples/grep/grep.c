/*
 * C Grep benchmark - counterpart to grep.meth
 *
 * Counts lines containing "ERROR" in a 256 KB log buffer, 200 passes.
 *
 * Build: build.bat (or: gcc -O2 -o grep_c.exe grep.c -lkernel32)
 * Run: grep_c.exe
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

#define PATTERN "ERROR"
#define PATTERN_LEN 5

static int64_t grep_count(const char *buf, int64_t len) {
    int64_t count = 0;
    int found = 0;
    for (int64_t i = 0; i < len; i++) {
        int c = (unsigned char)buf[i];
        if (c == '\n') {
            if (found) count++;
            found = 0;
        } else {
            if (!found && i + PATTERN_LEN <= len && c == 'E') {
                if (memcmp(buf + i, PATTERN, PATTERN_LEN) == 0) {
                    found = 1;
                }
            }
        }
    }
    if (found) count++;
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

    const char *block = "INFO 12345\nERROR 12345\nINFO 12345\nINFO 12345\nINFO 1234\n";
    const size_t block_len = 55;
    for (int64_t pos = 0; pos < buf_size; pos += (int64_t)block_len) {
        size_t chunk = (size_t)(buf_size - pos);
        if (chunk > block_len) chunk = block_len;
        memcpy(buf + pos, block, chunk);
    }

    printf("Grep: count lines containing ERROR (256 KB log buffer)\n");

    int64_t matches = grep_count(buf, buf_size);
    printf("Matches = %" PRId64 "\n", matches);

    const int passes = 200;
    printf("Benchmark: 200 passes (grep_count)\n");

    uint64_t t0 = get_time_ms();
    int64_t total = 0;
    for (int p = 0; p < passes; p++) {
        total += grep_count(buf, buf_size);
    }
    uint64_t t1 = get_time_ms();
    uint64_t elapsed_ms = t1 - t0;

    printf("Total matches = %" PRId64 "\n", total);
    printf("Time: %" PRIu64 " ms\n", elapsed_ms);

    uint64_t per_pass_us = elapsed_ms * 1000 / passes;
    printf("Per pass: ~%" PRIu64 " us\n", per_pass_us);

    free(buf);
    return 0;
}
