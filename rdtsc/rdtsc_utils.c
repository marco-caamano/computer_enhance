#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>

#include "rdtsc_utils.h"

#define MY_ERROR(...) {                    \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

#define DEFAULT_TEST_FREQ_MS_WAIT   100

int profile_index = -1;
uint64_t profile_program_start = 0;
uint64_t profile_program_end = 0;

struct profile_block profile_data[TIMING_DATA_SIZE] = {};

uint64_t calculated_cpu_freq = 0;

#if _WIN32

#include <intrin.h>
#include <windows.h>
#include <psapi.h>

struct os_metrics
{
    bool Initialized;
    HANDLE ProcessHandle;
};
static struct os_metrics GlobalMetrics;

uint64_t GetOSTimerFreq(void)
{
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	return Freq.QuadPart;
}

uint64_t ReadOSTimer(void)
{
	LARGE_INTEGER Value;
	QueryPerformanceCounter(&Value);
	return Value.QuadPart;
}

uint64_t ReadOSPageFaultCount(void)
{
    PROCESS_MEMORY_COUNTERS_EX MemoryCounters = {};
    MemoryCounters.cb = sizeof(MemoryCounters);

    if(!GlobalMetrics.Initialized) {
        InitializeOSMetrics();
    }

    GetProcessMemoryInfo(GlobalMetrics.ProcessHandle, (PROCESS_MEMORY_COUNTERS *)&MemoryCounters, sizeof(MemoryCounters));
    
    uint64_t Result = MemoryCounters.PageFaultCount;
    return Result;
}

void InitializeOSMetrics(void)
{
    if(!GlobalMetrics.Initialized) {
        GlobalMetrics.Initialized = true;
        GlobalMetrics.ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
    }
}

#else

#include <x86intrin.h>
#include <sys/time.h>
#include <sys/resource.h>

uint64_t GetOSTimerFreq(void)
{
    // How many ticks are in a seconds for the time we are using
    // gettimeofday returns values in microseconds so 10^6 ticks/second
	return 1000000;
}

uint64_t ReadOSTimer(void)
{
	// NOTE(casey): The "struct" keyword is not necessary here when compiling in C++,
	// but just in case anyone is using this file from C, I include it.
	struct timeval Value;
	gettimeofday(&Value, 0);
	
	uint64_t Result = GetOSTimerFreq()*(uint64_t)Value.tv_sec + (uint64_t)Value.tv_usec;
	return Result;
}

uint64_t ReadOSPageFaultCount(void)
{
    // NOTE(casey): The course materials are not tested on MacOS/Linux.
    // This code was contributed to the public github. It may or may not work
    // for your system.

    struct rusage Usage = {};
    getrusage(RUSAGE_SELF, &Usage);

    // ru_minflt  the number of page faults serviced without any I/O activity.
    // ru_majflt  the number of page faults serviced that required I/O activity.
    uint64_t Result = Usage.ru_minflt + Usage.ru_majflt;

    return Result;
}

void InitializeOSMetrics(void)
{
}

#endif

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
        MY_ERROR("Failed to get CPU Frequency\n");
    }

    // return (elapsed_cpu_ticks * GetOSTimerFreq() / 1000) / calculated_cpu_freq;
    return (elapsed_cpu_ticks * 1000) / calculated_cpu_freq;
}

void report_profile_results(void) {
    uint64_t program_elapsed = profile_program_end - profile_program_start;
    printf("Progran Runtime Ticks[%" PRIu64 "](100%%) [%" PRIu64 "]ms CPU Freq: [%" PRIu64 "]\n",
        program_elapsed,
        get_ms_from_cpu_ticks(program_elapsed),
        guess_cpu_freq(100));

    for (int i=0; i<TIMING_DATA_SIZE; i++) {
        if ( profile_data[i].name != NULL ) {
            
            printf("Slot[%d] Name[%s] Ticks[%" PRIu64 "](%03.2f%%) [%" PRIu64 "]ms", i,
                profile_data[i].name,
                profile_data[i].total_ticks,
                ((float)profile_data[i].total_ticks/(float)program_elapsed)*100,
                get_ms_from_cpu_ticks(profile_data[i].total_ticks));
            if (profile_data[i].count>1) {
                uint64_t average = profile_data[i].total_ticks / profile_data[i].count;
                printf(" | NumRuns[%" PRIu64 "] Average Ticks[%" PRIu64 "][%" PRIu64 "]ms", 
                    profile_data[i].count,
                    average,
                    get_ms_from_cpu_ticks(average));
            }
            if (profile_data[i].children_ticks>0) {
                printf(" | Children Ticks[%" PRIu64 "][%" PRIu64 "]ms", 
                    profile_data[i].children_ticks, 
                    get_ms_from_cpu_ticks(profile_data[i].children_ticks));
            }
            if (profile_data[i].processed_byte_count) {
                double megabyte = (double)1024*(double)1024;
                double gigabyte = megabyte*(double)1024;
                double seconds = (double)get_ms_from_cpu_ticks(profile_data[i].total_ticks)/(double)1000;
                double bytes_per_second = (double)profile_data[i].processed_byte_count / seconds;
                double megabytes = (double)profile_data[i].processed_byte_count / (double)megabyte;
                double gigabytes_per_second = bytes_per_second / gigabyte;
                printf(" | %.3fMB at %.2f GB/s", megabytes, gigabytes_per_second);
            }
            printf("\n");
        }
    }
}

#define MEGABYTE    ((double)1024*(double)1024)
#define GIGABYTE    (MEGABYTE*(double)1024)

void print_data_speed(uint64_t total_bytes, uint64_t total_cpu_ticks) {
    double seconds = (double)get_ms_from_cpu_ticks(total_cpu_ticks) / (double)1000;
    double bytes_per_second = (double)total_bytes / seconds;
    double megabytes = (double)total_bytes / MEGABYTE;
    double gigabytes_per_second = bytes_per_second / GIGABYTE;
    printf(" [%" PRIu64 "]Ticks [%" PRIu64 "]ms (%.3fMB at %.2f GB/s) ", total_cpu_ticks, get_ms_from_cpu_ticks(total_cpu_ticks), megabytes, gigabytes_per_second);
}

