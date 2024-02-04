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

#define ERROR(...) {                    \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

static uint64_t GetOSTimerFreq(void) {
    // How many ticks are in a seconds for the time we are using
    // gettimeofday returns values in microseconds so 10^6 ticks/second
	return 1000000;
}

static uint64_t ReadOSTimer(void)
{
	// NOTE(casey): The "struct" keyword is not necessary here when compiling in C++,
	// but just in case anyone is using this file from C, I include it.
	struct timeval Value;
	gettimeofday(&Value, 0);
	
	uint64_t Result = GetOSTimerFreq()*(uint64_t)Value.tv_sec + (uint64_t)Value.tv_usec;
	return Result;
}

/* NOTE(casey): This does not need to be "inline", it could just be "static"
   because compilers will inline it anyway. But compilers will warn about 
   static functions that aren't used. So "inline" is just the simplest way 
   to tell them to stop complaining about that. */
uint64_t ReadCPUTimer(void)
{
	// NOTE(casey): If you were on ARM, you would need to replace __rdtsc
	// with one of their performance counter read instructions, depending
	// on which ones are available on your platform.
	
	return __rdtsc();
}

uint64_t GuessCPUFreq(int wait_ms) {
    uint64_t OSFreq = GetOSTimerFreq();
    uint64_t WaitTimerTicks = wait_ms * (OSFreq / 1000); // milliseconds to microseconds
    uint64_t CPUStart = ReadCPUTimer();
	uint64_t OSStart = ReadOSTimer();
	uint64_t OSEnd = 0;
	uint64_t OSElapsed = 0;
    uint64_t CPUFreq = 0;
	while(OSElapsed < WaitTimerTicks) {
		OSEnd = ReadOSTimer();
		OSElapsed = OSEnd - OSStart;
	}
    uint64_t CPUEnd = ReadCPUTimer();
	uint64_t CPUElapsed = CPUEnd - CPUStart;
    CPUFreq = 0;
    if(OSElapsed) {
		CPUFreq = OSFreq * CPUElapsed / OSElapsed;
	}
    return CPUFreq;
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
    printf("RDTSC\n");
    printf("==============\n");

    uint64_t OSFreq = GetOSTimerFreq();
	printf("    OS Freq: %lu\n", OSFreq);

    uint64_t CPUStart = ReadCPUTimer();
	uint64_t OSStart = ReadOSTimer();
	uint64_t OSEnd = 0;
	uint64_t OSElapsed = 0;
    uint64_t WaitTimerTicks = wait_ms * (OSFreq / 1000); // milliseconds to microseconds
	while(OSElapsed < WaitTimerTicks) {
		OSEnd = ReadOSTimer();
		OSElapsed = OSEnd - OSStart;
	}
    uint64_t CPUEnd = ReadCPUTimer();
	uint64_t CPUElapsed = CPUEnd - CPUStart;
    uint64_t CPUFreq = 0;
    if(OSElapsed) {
		CPUFreq = OSFreq * CPUElapsed / OSElapsed;
	}
	
	printf("   OS Timer: %lu -> %lu = %lu elapsed\n", OSStart, OSEnd, OSElapsed);
	printf(" OS Seconds: %.4f\n", (double)OSElapsed/(double)OSFreq);
    printf("  CPU Timer: %lu -> %lu = %lu elapsed\n", CPUStart, CPUEnd, CPUElapsed);
    printf("   CPU Freq: %lu (guessed)\n", CPUFreq);

    printf("-----------------------------------------------------------\n");
    printf("   CPU Freq: %lu (guessed using function)\n", GuessCPUFreq(wait_ms));
    printf("-----------------------------------------------------------\n");

    return 0;
}
