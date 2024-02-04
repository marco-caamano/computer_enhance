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

static void calculate_ms_interval(int ms_interval) {
    uint64_t start, end, elapsed;
    printf("\nSleep for %dms\n", ms_interval);
    start = GET_CPU_TICKS();
    usleep(ms_interval*1000);
    end = GET_CPU_TICKS();
    elapsed = end - start;
    printf("Start [%lu] -> End [%lu] => Elapsed [%lu] | Elapsed Time [%lu]ms\n", start, end, elapsed, get_ms_from_cpu_ticks(elapsed));
}

void usage(void) {
    fprintf(stderr, "Data Generator Usage:\n");
    fprintf(stderr, "-h         This help dialog.\n");
    fprintf(stderr, "-w <ms>    Use <ms> in the Calculation, defaults to 1000ms (1s).\n");
}


int main (int argc, char *argv[]) {
    int opt;
    int wait_ms = 1000;

    while( (opt = getopt(argc, argv, "hw:")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;

            case 'w':
                wait_ms = atoi(optarg);
                if (wait_ms==0) {
                    ERROR("Failed to convert parameter[%s] to integer\n", optarg);
                }
                break;

            default:
                fprintf(stderr, "ERROR Invalid command line option\n");
                usage();
                exit(1);
                break;
        }
    }

    printf("==============\n");
    printf("RDTSC TEST\n");
    printf("==============\n");

    printf("CPU Freq: %lu (guessed using wait_interval %d ms)\n", guess_cpu_freq(wait_ms), wait_ms);

    calculate_ms_interval(100);
    calculate_ms_interval(50);
    calculate_ms_interval(8);

    printf("\n\n");
    
    return 0;
}
