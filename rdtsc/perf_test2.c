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
    TEST_D,
    TEST_E,
};

void test_a(void);
void test_b(void);
void test_c(void);
void test_d(void);
void test_e(void);

void test_a(void) {
    TAG_FUNCTION_START(TEST_A);
    sleep(3);
    test_b();
    test_d();
    TAG_FUNCTION_END(TEST_A);
}

void test_b(void) {
    TAG_FUNCTION_START(TEST_B);
    sleep(2);
    test_c();
    TAG_FUNCTION_END(TEST_B);
}

void test_c(void) {
    TAG_FUNCTION_START(TEST_C);
    sleep(1);
    TAG_FUNCTION_END(TEST_C);
}

void test_d(void) {
    TAG_FUNCTION_START(TEST_D);
    sleep(1);
    test_e();
    TAG_FUNCTION_END(TEST_D);
}

void test_e(void) {
    TAG_FUNCTION_START(TEST_E);
    sleep(1);
    TAG_FUNCTION_END(TEST_E);
}

int main (int argc, char *argv[]) {
    TAG_PROGRAM_START();

    printf("==========\n");
    printf("PERF TEST2\n");
    printf("==========\n");

    sleep(2);

    test_a();

    TAG_PROGRAM_END();

    report_program_timings();

    return 0;
}
