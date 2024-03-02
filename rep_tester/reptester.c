#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <x86intrin.h>
#include <sys/time.h>

#include "reptester.h"
#include "rdtsc_utils.h"

#define ERROR(...) {                    \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }


void rep_tester(struct rep_tester_config *test_info, void *context) {
    uint64_t rep_counter = 0;
    bool test_done = false;

    if (test_info->env_setup) {
        // printf("[%s:%d] Calling EnvSetUp\n", __FUNCTION__, __LINE__);
        test_info->env_setup(context);
    }

    uint64_t test_start_ticks = GET_CPU_TICKS();

    if (!test_info->silent) {
        printf("\n==========\n");
    }

    for (;;) {
        if (!test_info->silent) {
            printf("Run [%05lu] ", rep_counter+1);
            fflush(stdout);
        }
        if (test_info->test_setup) {
            // printf("[%s:%d] Calling SetUp\n", __FUNCTION__, __LINE__);
            test_info->test_setup(context);
        }
        if (test_info->test_main) {
            // printf("[%s:%d] Calling Main\n", __FUNCTION__, __LINE__);
            test_info->test_main(context);
        }
        if (test_info->test_teardown) {
            // printf("[%s:%d] Calling TearDown\n", __FUNCTION__, __LINE__);
            test_info->test_teardown(context);
        }
        uint64_t test_runtime_ticks = GET_CPU_TICKS() - test_start_ticks;

        uint64_t current_run_seconds = get_ms_from_cpu_ticks(test_runtime_ticks)/1000;

        if (test_info->end_of_test_eval) {
            // use TestEval to determine if test is done
            // printf("[%s:%d] Calling TestEval\n", __FUNCTION__, __LINE__);
            test_done = test_info->end_of_test_eval(context);
            if (test_done) {
                if (!test_info->silent) {
                    printf("\n==========\n");
                }
                printf("Test has run for [%lu]iterations during [%lu]seconds, TestEval returned test Done. Test Run completed\n\n", rep_counter, current_run_seconds);
                break;
            }
        } else {
            // use time to determine if test is done
            if ( current_run_seconds > test_info->test_runtime_seconds) {
                if (!test_info->silent) {
                    printf("\n==========\n");
                }
                printf("Test has run for [%lu]iterations during [%lu]seconds, interval was set for [%u]seconds. Test Run completed\n\n", rep_counter, current_run_seconds, test_info->test_runtime_seconds);
                break;
            }
        }
        if (!test_info->silent) {
            printf("\r                                                                          \r");
        }

        rep_counter++;
    }


    if (test_info->env_teardown) {
        // printf("[%s:%d] Calling EnvTearDown\n", __FUNCTION__, __LINE__);
        test_info->env_teardown(context);
    }

    if (test_info->print_stats) {
        printf("[%s:%d] Calling PrintStats\n", __FUNCTION__, __LINE__);
        test_info->print_stats(context);
    }

    printf("\n\nTest Run Completed. Executed test [%lu] times\n\n", rep_counter);

}