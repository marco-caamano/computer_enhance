#include <stdint.h>
#include <stdio.h>
#include <x86intrin.h>

#define GET_CPU_TICKS()  __rdtsc()

#define TAG_PROGRAM_START() {\
    profile_program_start = GET_CPU_TICKS(); \
}

#define TAG_PROGRAM_END() {\
    profile_program_end = GET_CPU_TICKS(); \
    report_profile_results(); \
}

#if 1
// #ifdef PROFILER
/*
 * If we are in a recursive function:
 *      profile_index == index
 * Then do not modify data, let the outermost
 * function call measure the time, but do keep
 * count of times the function is called for 
 * averages
*/
#define TAG_DATA_BLOCK_START(index, block_name, byte_count) { \
        if (index == profile_index) { \
            profile_data[index].recursive_count++; \
        } else { \
            profile_data[index].name = block_name; \
            profile_data[index].start_rdtsc = GET_CPU_TICKS(); \
            profile_data[index].processed_byte_count = byte_count; \
            if (profile_index != -1) { \
                profile_data[index].parent_index = profile_index;\
            } else {\
                profile_data[index].parent_index = -1; \
            } \
            profile_index = index; \
        } \
}
#define TAG_BLOCK_START(index, block_name) TAG_DATA_BLOCK_START(index, block_name, 0)
#define TAG_FUNCTION_START(index)   TAG_DATA_BLOCK_START(index, __FUNCTION__, 0)
#define TAG_DATA_FUNCTION_START(index, byte_count)   TAG_DATA_BLOCK_START(index, __FUNCTION__, byte_count)

#define TAG_BLOCK_END(index) { \
        if (profile_data[index].recursive_count==0) { \
            profile_data[index].total_ticks +=  GET_CPU_TICKS() - profile_data[index].start_rdtsc; \
            profile_data[index].count++; \
            profile_data[index].start_rdtsc = 0; \
            if (profile_data[index].parent_index != -1) { \
                profile_index = profile_data[index].parent_index; \
                profile_data[profile_index].children_ticks += profile_data[index].total_ticks; \
            } else { \
                profile_index = -1; \
            } \
        } else { \
            profile_data[index].recursive_count--; \
        } \
}
#define TAG_FUNCTION_END(...)     TAG_BLOCK_END(__VA_ARGS__)

#else

#define TAG_DATA_BLOCK_START(index, block_name, byte_count)     {}
#define TAG_BLOCK_START(index, block_name)                      {}
#define TAG_FUNCTION_START(index)                               {}
#define TAG_DATA_FUNCTION_START(index, byte_count)              {}
#define TAG_BLOCK_END(index)                    {}
#define TAG_FUNCTION_END(...)                   {}

#endif

#define TIMING_DATA_SIZE            4096

struct profile_block {
    const char *name;
    int parent_index;
    uint64_t start_rdtsc;
    uint64_t total_ticks;
    uint64_t count;
    uint64_t children_ticks;
    int recursive_count;
    uint64_t processed_byte_count;
};

extern int profile_index;
extern uint64_t profile_program_start;
extern uint64_t profile_program_end;
extern struct profile_block profile_data[];

uint64_t guess_cpu_freq(int wait_ms);
uint64_t get_ms_from_cpu_ticks(uint64_t elapsed_cpu_ticks);

void report_profile_results(void);

