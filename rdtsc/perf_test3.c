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
    FACTORIAL,
};

void test_a(void);
int factorial(int num);

int factorial(int num) {
    TAG_FUNCTION_START(FACTORIAL);
    if (num==0) return 1;
    int result = num*factorial(num-1);
    TAG_FUNCTION_END(FACTORIAL);
    return result;
}

void test_a(void) {
    TAG_FUNCTION_START(TEST_A);
    sleep(3);
    TAG_FUNCTION_END(TEST_A);
}

int main (int argc, char *argv[]) {
    TAG_PROGRAM_START();

    printf("==========\n");
    printf("PERF TEST3\n");
    printf("==========\n");

    for (int i=0; i<10; i++) {
        printf("%d!=%d\n", i, factorial(i));
    }

    test_a();

    TAG_PROGRAM_END();

    report_program_timings();

    return 0;
}
