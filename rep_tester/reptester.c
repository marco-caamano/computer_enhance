#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#ifndef _WIN32
#include <getopt.h>
#include <unistd.h>
#endif
#include <errno.h>
#include <time.h>

#if _WIN32
#include <intrin.h>
#include <windows.h>
#else
#include <x86intrin.h>
#include <sys/time.h>
#endif

#include "reptester.h"
#include "rdtsc_utils.h"

void rep_tester(struct rep_tester_config *test_info, void *context) {
    uint32_t rep_counter = 1;
    bool test_done = false;

    if (test_info->env_setup) {
        // printf("[%s:%d] Calling EnvSetUp\n", __FUNCTION__, __LINE__);
        test_info->env_setup(context);
    }

    uint64_t test_start_ticks = GET_CPU_TICKS();

    if (!test_info->silent) {
        printf("\nRunning Test [%s]\n", test_info->test_name);
        printf("=================================================\n");
    }

    for (;;) {
        if (!test_info->silent) {
            printf("Run [%05"PRIu32"] ", rep_counter);
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

        uint64_t elapse_ticks = GET_CPU_TICKS() - test_start_ticks;
        uint32_t elapsed_seconds = (uint32_t)(get_ms_from_cpu_ticks(elapse_ticks)/1000);

        if (test_info->end_of_test_eval) {
            // use TestEval to determine if test is done
            // printf("[%s:%d] Calling TestEval\n", __FUNCTION__, __LINE__);
            test_done = test_info->end_of_test_eval(context);
            if (test_done) {
                if (!test_info->silent) {
                    printf("\n=================================================\n");
                }
                printf("Test has run for [%" PRIu32 "]iterations during [%" PRIu32 "]seconds, TestEval returned test Done. Test Run completed\n", rep_counter, elapsed_seconds);
                break;
            }
        } else {
            // use time to determine if test is done
            if ( elapsed_seconds > test_info->test_runtime_seconds) {
                if (!test_info->silent) {
                    printf("\n=================================================\n");
                }
                printf("Test[%s] has run for [%" PRIu32 "]iterations during [%" PRIu32 "]seconds, interval was set for [%" PRIu32 "]seconds. Test Run completed\n", test_info->test_name, rep_counter, elapsed_seconds, test_info->test_runtime_seconds);
                break;
            }
        }
        if (!test_info->silent) {
            printf("\r            \r");
        }

        rep_counter++;
    }


    if (test_info->env_teardown) {
        // printf("[%s:%d] Calling EnvTearDown\n", __FUNCTION__, __LINE__);
        test_info->env_teardown(context);
    }

    if (test_info->print_stats) {
        //printf("[%s:%d] Calling PrintStats\n", __FUNCTION__, __LINE__);
        test_info->print_stats(context);
    }

    // printf("Test[%s] Run Completed. Executed test [%" PRIu32 "] times\n\n", test_info->test_name, rep_counter);

}


void rep_tester_run(struct rep_tester_config test_info[], int count) {
    for (int i=0; i<count; ++i) {
        rep_tester(&test_info[i], test_info[i].context);
    }
}