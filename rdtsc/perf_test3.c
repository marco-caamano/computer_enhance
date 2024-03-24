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
#define MSLEEP(x) Sleep(x)
#else
#include <getopt.h>
#include <unistd.h>
#define SLEEP(x) sleep(x)
#define MSLEEP(x) usleep(x*1000)
#endif

#include "rdtsc_utils.h"

enum perf_blocks_e {
    TEST_A,
    FACTORIAL,
};

void test_a(void);
int factorial(int num);

int factorial(int num) {
    int result;
    TAG_FUNCTION_START(FACTORIAL);
    MSLEEP(3);
    if (num==0) {
        result = 1;
        goto exit;
    }
    result = num*factorial(num-1);
exit:
    TAG_FUNCTION_END(FACTORIAL);
    return result;
}

void test_a(void) {
    TAG_FUNCTION_START(TEST_A);
    SLEEP(1);
    TAG_FUNCTION_END(TEST_A);
}

int main (int argc, char *argv[]) {
    TAG_PROGRAM_START();

    printf("==========\n");
    printf("PERF TEST3\n");
    printf("==========\n\n");

    for (int i=0; i<10; i++) {
        printf("%d!=%d\n", i, factorial(i));
    }

    test_a();

    TAG_PROGRAM_END();

    return 0;
}
