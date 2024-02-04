#include <stdint.h>
#include <x86intrin.h>

#define GET_CPU_TICKS()  __rdtsc()

uint64_t guess_cpu_freq(int wait_ms);
uint64_t get_ms_from_cpu_ticks(uint64_t elapsed_cpu_ticks);
