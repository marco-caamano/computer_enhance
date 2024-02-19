#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include "rdtsc_utils.h"

#define ERROR(...) {                    \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

enum perf_blocks_e {
    TEST_A,
    TEST_B,
    TEST_C,
};

void test_a(void) {
    TAG_FUNCTION_START(TEST_A);
    sleep(2);

    TAG_BLOCK_START(TEST_B, "BlockB");
    sleep(1);
    TAG_BLOCK_END(TEST_B);

    TAG_BLOCK_START(TEST_C, "BlockC");
    sleep(2);
    TAG_BLOCK_END(TEST_C);

    TAG_FUNCTION_END(TEST_A);
}

int main (int argc, char *argv[]) {
    TAG_PROGRAM_START();

    printf("==========\n");
    printf("PERF TEST3\n");
    printf("==========\n\n");

    test_a();

    TAG_PROGRAM_END();

    report_program_timings();

    return 0;
}
