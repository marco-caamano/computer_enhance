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

#include "rdtsc_utils.h"

#define ERROR(...) {                    \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

#define DEFAULT_TEST_FREQ_MS_WAIT   100

static uint64_t calculated_cpu_freq = 0;

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

uint64_t guess_cpu_freq(int wait_ms) {
    uint64_t OSFreq = GetOSTimerFreq();
    uint64_t WaitTimerTicks = wait_ms * (OSFreq / 1000); // milliseconds to microseconds
    uint64_t CPUStart = GET_CPU_TICKS();
	uint64_t OSStart = ReadOSTimer();
	uint64_t OSEnd = 0;
	uint64_t OSElapsed = 0;
    uint64_t CPUFreq = 0;
	while(OSElapsed < WaitTimerTicks) {
		OSEnd = ReadOSTimer();
		OSElapsed = OSEnd - OSStart;
	}
    uint64_t CPUEnd = GET_CPU_TICKS();
	uint64_t CPUElapsed = CPUEnd - CPUStart;
    CPUFreq = 0;
    if(OSElapsed) {
		CPUFreq = OSFreq * CPUElapsed / OSElapsed;
	}
    if (calculated_cpu_freq == 0) {
        calculated_cpu_freq = CPUFreq;
    }
    return CPUFreq;
}

uint64_t get_ms_from_cpu_ticks(uint64_t elapsed_cpu_ticks) {
    if (calculated_cpu_freq == 0) {
        // if we have not calculated the cpu freq yet, do so now
        guess_cpu_freq(DEFAULT_TEST_FREQ_MS_WAIT);
    }
    if (calculated_cpu_freq == 0) {
        ERROR("Failed to get CPU Frequency\n");
    }

    return (elapsed_cpu_ticks * GetOSTimerFreq() / 1000) / calculated_cpu_freq;
}


