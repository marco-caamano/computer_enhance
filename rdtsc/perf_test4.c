#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#ifdef _WIN32
#include <Windows.h>
#include <synchapi.h>
#define SLEEP(x) Sleep(x*1000)
#else
#include <getopt.h>
#include <unistd.h>
#define SLEEP(x) sleep(x)
#endif

#include "rdtsc_utils.h"

enum perf_blocks_e {
    TEST_A,
    TEST_B,
    TEST_C,
};

void test_a(void) {
    TAG_FUNCTION_START(TEST_A);
    SLEEP(2);

    TAG_BLOCK_START(TEST_B, "BlockB");
    SLEEP(1);
    TAG_BLOCK_END(TEST_B);

    TAG_BLOCK_START(TEST_C, "BlockC");
    SLEEP(2);
    TAG_BLOCK_END(TEST_C);

    TAG_FUNCTION_END(TEST_A);
}

int main (int argc, char *argv[]) {
    TAG_PROGRAM_START();

    printf("==========\n");
    printf("PERF TEST4\n");
    printf("==========\n\n");

    test_a();

    TAG_PROGRAM_END();

    return 0;
}
