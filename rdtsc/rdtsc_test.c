#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#ifdef _WIN32
#include <Windows.h>
#include <synchapi.h>
#else
#include <getopt.h>
#include <unistd.h>
#endif
#include <errno.h>

#include "rdtsc_utils.h"

#define MY_ERROR(...) {                    \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

static void calculate_ms_interval(int ms_interval) {
    uint64_t start, end, elapsed;
    printf("\nSleep for %dms\n", ms_interval);
    start = GET_CPU_TICKS();
#ifdef _WIN32
    Sleep(ms_interval);
#else
    usleep(ms_interval*1000);
#endif
    end = GET_CPU_TICKS();
    elapsed = end - start;
    printf("Start [%" PRIu64 "] -> End [%" PRIu64 "] => Elapsed [%" PRIu64 "] | Elapsed Time [%" PRIu64 "]ms\n", start, end, elapsed, get_ms_from_cpu_ticks(elapsed));
}

void usage(void) {
    fprintf(stderr, "Data Generator Usage:\n");
    fprintf(stderr, "-h         This help dialog.\n");
    fprintf(stderr, "-w <ms>    Use <ms> in the Calculation, defaults to 1000ms (1s).\n");
}


int main (int argc, char *argv[]) {
    int opt;
    int wait_ms = 1000;

#ifdef _WIN32
    for (int index=1; index<argc; ++index) {
        if (strcmp(argv[index], "-h")==0) {
            usage();
            exit(0);
        } else if (strcmp(argv[index], "-w")==0) {
            // must have at least index+2 arguments to contain a file
            if (argc<index+2) {
                printf("MY_ERROR: missing output file parameter\n");
                usage();
                exit(1);
            }
            wait_ms = atoi(argv[index+1]);
            // since we consume the next parameter then skip it
            ++index;
        }
    }
#else
    while( (opt = getopt(argc, argv, "hw:")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;

            case 'w':
                wait_ms = atoi(optarg);
                if (wait_ms==0) {
                    MY_ERROR("Failed to convert parameter[%s] to integer\n", optarg);
                }
                break;

            default:
                fprintf(stderr, "MY_ERROR Invalid command line option\n");
                usage();
                exit(1);
                break;
        }
    }
#endif

    printf("==============\n");
    printf("RDTSC TEST\n");
    printf("==============\n");

    printf("OSTimerFreq: %" PRIu64 "\n", GetOSTimerFreq());

    printf("CPU Freq: %" PRIu64 " (guessed using wait_interval %d ms)\n", guess_cpu_freq(wait_ms), wait_ms);

    calculate_ms_interval(100);
    calculate_ms_interval(50);
    calculate_ms_interval(8);

    printf("\n\n");
    
    return 0;
}
