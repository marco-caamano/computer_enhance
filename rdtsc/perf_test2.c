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
    SLEEP(3);
    test_b();
    test_d();
    TAG_FUNCTION_END(TEST_A);
}

void test_b(void) {
    TAG_FUNCTION_START(TEST_B);
    SLEEP(2);
    test_c();
    TAG_FUNCTION_END(TEST_B);
}

void test_c(void) {
    TAG_FUNCTION_START(TEST_C);
    SLEEP(1);
    TAG_FUNCTION_END(TEST_C);
}

void test_d(void) {
    TAG_FUNCTION_START(TEST_D);
    SLEEP(1);
    test_e();
    TAG_FUNCTION_END(TEST_D);
}

void test_e(void) {
    TAG_FUNCTION_START(TEST_E);
    SLEEP(1);
    TAG_FUNCTION_END(TEST_E);
}

int main (int argc, char *argv[]) {
    TAG_PROGRAM_START();

    printf("==========\n");
    printf("PERF TEST2\n");
    printf("==========\n");

    SLEEP(2);

    test_a();

    TAG_PROGRAM_END();

    return 0;
}
