#include <stdint.h>
#include <stdio.h>

typedef void reptester_function(void *context);
typedef bool reptester_eval_function(void *context);

struct rep_tester_config {
    reptester_function *env_setup;              // Run Only Once at start of test program
    reptester_function *test_setup;             // Run at the start of each test run
    reptester_function *test_main;              // Main Code for the test run
    reptester_function *test_teardown;          // Run at the end of each test run
    reptester_function *env_teardown;           // Run Only Ince at the end of test program
    reptester_function *print_stats;            // Run at the end of the test run will print profiling stats
    reptester_eval_function *end_of_test_eval;  // Instead of a runtime call this function to determine if we should stop running
    uint32_t test_runtime_seconds;              // Number of second to run for. 0 for infinite loop
    bool silent;                                // Run Silent Loop, do not printout the loop counter
};


void rep_tester(struct rep_tester_config *test_info, void *context);

