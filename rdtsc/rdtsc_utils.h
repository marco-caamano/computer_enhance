#include <stdint.h>
#include <stdio.h>
#include <x86intrin.h>

#define GET_CPU_TICKS()  __rdtsc()

#define TAG_PROGRAM_START() {\
    rdtsc_program_start = GET_CPU_TICKS(); \
}

#define TAG_PROGRAM_END() {\
    rdtsc_program_end = GET_CPU_TICKS(); \
}

/*
 * If we are in a recursive function:
 *      profile_current_block_index == index
 * Then do not modify data, let the outermost
 * function call measure the time, but do keep
 * count of times the function is called for 
 * averages
*/
#define TAG_FUNCTION_START(index) { \
        rdtsc_timing_data[index].name = __FUNCTION__; \
        rdtsc_timing_data[index].start_rdtsc = GET_CPU_TICKS(); \
        if (profile_current_block_index != -1) { \
            rdtsc_timing_data[index].parent_index = profile_current_block_index;\
        } else {\
            rdtsc_timing_data[index].parent_index = -1; \
        } \
        profile_current_block_index = index; \
}

#define TAG_FUNCTION_END(index) { \
        rdtsc_timing_data[index].total_ticks +=  GET_CPU_TICKS() - rdtsc_timing_data[index].start_rdtsc; \
        rdtsc_timing_data[index].count++; \
        rdtsc_timing_data[index].start_rdtsc = 0; \
        if (rdtsc_timing_data[index].parent_index != -1) { \
            profile_current_block_index = rdtsc_timing_data[index].parent_index; \
            rdtsc_timing_data[profile_current_block_index].children_ticks += rdtsc_timing_data[index].total_ticks; \
        } else { \
            profile_current_block_index = -1; \
        } \
}

#define TIMING_DATA_SIZE            4096

struct timing_block {
    const char *name;
    int parent_index;
    uint64_t start_rdtsc;
    uint64_t total_ticks;
    uint64_t count;
    uint64_t children_ticks;
};

extern int profile_current_block_index;
extern uint64_t rdtsc_program_start;
extern uint64_t rdtsc_program_end;
extern struct timing_block rdtsc_timing_data[];

uint64_t guess_cpu_freq(int wait_ms);
uint64_t get_ms_from_cpu_ticks(uint64_t elapsed_cpu_ticks);

void report_program_timings(void);

