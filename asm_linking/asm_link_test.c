#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#ifdef _WIN32
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#else
#error "This Version is only for WIN32"
#endif
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "reptester.h"
#include "rdtsc_utils.h"

#define MY_ERROR(...) {                 \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

#define BUFFER_SIZE       1*1024*1024*1024

void MOVAllBytesASM(uint64_t Count, uint8_t *Data);
void NOPAllBytesASM(uint64_t Count);
void CMPAllBytesASM(uint64_t Count);
void DECAllBytesASM(uint64_t Count);

struct test_context {
    char *name;
    size_t datasize;
    uint8_t *buffer;
    uint64_t min_cpu_ticks;
    uint64_t max_cpu_ticks;
};


// ===================================================================================
// Write Bytes
// ===================================================================================
void writebytes_main(void *context) {
    int bytes_read = 0;
    bool result;
    struct test_context *ctx = (struct test_context *)context;

    uint8_t *ptr = ctx->buffer;

    // Measure Actual Process
    uint64_t start = GET_CPU_TICKS();

    for (size_t index=0; index<ctx->datasize; ++index) {
        ptr[index] = (uint8_t)index;
    }

    uint64_t elapsed_ticks = GET_CPU_TICKS() - start;

    if (ctx->min_cpu_ticks==0 || elapsed_ticks < ctx->min_cpu_ticks) {
        ctx->min_cpu_ticks = elapsed_ticks;
        printf(" | New MinTime");
        print_data_speed(ctx->datasize, elapsed_ticks);
    }
}

// ===================================================================================
// Move All Bytes
// ===================================================================================
void move_all_bytes_main(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    // Measure Actual Process
    uint64_t start = GET_CPU_TICKS();
    MOVAllBytesASM(ctx->datasize, ctx->buffer);
    uint64_t elapsed_ticks = GET_CPU_TICKS() - start;

    if (ctx->min_cpu_ticks==0 || elapsed_ticks < ctx->min_cpu_ticks) {
        ctx->min_cpu_ticks = elapsed_ticks;
        printf(" | New MinTime");
        print_data_speed(ctx->datasize, elapsed_ticks);
    }
}

// ===================================================================================
// NOP All Bytes
// ===================================================================================
void nop_all_bytes_main(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    // Measure Actual Process
    uint64_t start = GET_CPU_TICKS();
    NOPAllBytesASM(ctx->datasize);
    uint64_t elapsed_ticks = GET_CPU_TICKS() - start;

    if (ctx->min_cpu_ticks==0 || elapsed_ticks < ctx->min_cpu_ticks) {
        ctx->min_cpu_ticks = elapsed_ticks;
        printf(" | New MinTime");
        print_data_speed(ctx->datasize, elapsed_ticks);
    }
}

// ===================================================================================
// CMP All Bytes
// ===================================================================================
void cmp_all_bytes_main(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    // Measure Actual Process
    uint64_t start = GET_CPU_TICKS();
    CMPAllBytesASM(ctx->datasize);
    uint64_t elapsed_ticks = GET_CPU_TICKS() - start;

    if (ctx->min_cpu_ticks==0 || elapsed_ticks < ctx->min_cpu_ticks) {
        ctx->min_cpu_ticks = elapsed_ticks;
        printf(" | New MinTime");
        print_data_speed(ctx->datasize, elapsed_ticks);
    }
}

// ===================================================================================
// Dec All Bytes
// ===================================================================================
void dec_all_bytes_main(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    // Measure Actual Process
    uint64_t start = GET_CPU_TICKS();
    DECAllBytesASM(ctx->datasize);
    uint64_t elapsed_ticks = GET_CPU_TICKS() - start;

    if (ctx->min_cpu_ticks==0 || elapsed_ticks < ctx->min_cpu_ticks) {
        ctx->min_cpu_ticks = elapsed_ticks;
        printf("\r                                                                                              \r");
        printf("Run [%05"PRIu32"] ", 0);
        printf("| New MinTime");
        print_data_speed(ctx->datasize, elapsed_ticks);
        printf("------\r");
    }
}

// ===================================================================================
// ===================================================================================

void print_stats(void *context) {
    // printf("[%s] context [0x%p]\n", __FUNCTION__, context);
    struct test_context *ctx = (struct test_context *)context;
    uint64_t cpu_freq = guess_cpu_freq(100);

    // printf("Slowest Speed");
    // print_data_speed(ctx->datasize, ctx->max_cpu_ticks);
    // printf("\n");
    printf("Fastest Speed");
    print_data_speed(ctx->datasize, ctx->min_cpu_ticks);
    printf("\n");

    double bps = get_bps(ctx->datasize, ctx->min_cpu_ticks);
    printf("Bytes Per Second [%f] bps\n", bps);
    double cycles_per_byte = cpu_freq / bps;
    printf("Cycles per Byte [%f] \n", cycles_per_byte);
}


void usage(void) {
    fprintf(stderr, "Asm Linking Example Usage:\n");
    fprintf(stderr, "-h             This help dialog.\n");
    fprintf(stderr, "-t <runtime>   Set runtime in seconds. (defaults to 10seconds)\n");
}

int main (int argc, char *argv[]) {
    int runtime = 10;
    int ret;
    uint8_t *buffer = NULL;

    for (int index=1; index<argc; ++index) {
        if (strcmp(argv[index], "-h")==0) {
            usage();
            exit(0);
        } else if (strcmp(argv[index], "-t")==0) {
            // must have at least index+2 arguments to contain a file
            if (argc<index+2) {
                printf("ERROR: missing output file parameter\n");
                usage();
                exit(1);
            }
            runtime = atoi(argv[index+1]);
            // since we consume the next parameter then skip it
            ++index;
        }
    }

    printf("==========================\n");
    printf("File Read Repetition Tests\n");
    printf("==========================\n");

    printf("Using runtime   [%d]seconds\n", runtime);

    buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
         MY_ERROR("Malloc failed for size[%d]\n", BUFFER_SIZE);
    }

    // ===================================================================================
    // Write Bytes test
    // ===================================================================================
    struct test_context writebytes_context = {0};
    writebytes_context.name = "write_bytes";
    writebytes_context.datasize = BUFFER_SIZE;
    writebytes_context.buffer = buffer;

    struct rep_tester_config write_bytes_test = {0};
    write_bytes_test.test_name = writebytes_context.name;
    write_bytes_test.test_main = writebytes_main;
    write_bytes_test.print_stats = print_stats;
    write_bytes_test.test_runtime_seconds = runtime;
    write_bytes_test.context = (void *)&writebytes_context;

    // ===================================================================================
    // Move All Bytes test
    // ===================================================================================
    struct test_context moveallbytes_context = {0};
    moveallbytes_context.name = "move_all_bytes";
    moveallbytes_context.datasize = BUFFER_SIZE;
    moveallbytes_context.buffer = buffer;

    struct rep_tester_config move_all_bytes_test = {0};
    move_all_bytes_test.test_name = moveallbytes_context.name;
    move_all_bytes_test.test_main = move_all_bytes_main;
    move_all_bytes_test.print_stats = print_stats;
    move_all_bytes_test.test_runtime_seconds = runtime;
    move_all_bytes_test.context = (void *)&moveallbytes_context;

    // ===================================================================================
    // NOP All Bytes test
    // ===================================================================================
    struct test_context nopallbytes_context = {0};
    nopallbytes_context.name = "nop_all_bytes";
    nopallbytes_context.datasize = BUFFER_SIZE;
    nopallbytes_context.buffer = buffer;

    struct rep_tester_config nop_all_bytes_test = {0};
    nop_all_bytes_test.test_name = nopallbytes_context.name;
    nop_all_bytes_test.test_main = nop_all_bytes_main;
    nop_all_bytes_test.print_stats = print_stats;
    nop_all_bytes_test.test_runtime_seconds = runtime;
    nop_all_bytes_test.context = (void *)&nopallbytes_context;

    // ===================================================================================
    // CMP All Bytes test
    // ===================================================================================
    struct test_context cmpallbytes_context = {0};
    cmpallbytes_context.name = "cmp_all_bytes";
    cmpallbytes_context.datasize = BUFFER_SIZE;
    cmpallbytes_context.buffer = buffer;

    struct rep_tester_config cmp_all_bytes_test = {0};
    cmp_all_bytes_test.test_name = cmpallbytes_context.name;
    cmp_all_bytes_test.test_main = cmp_all_bytes_main;
    cmp_all_bytes_test.print_stats = print_stats;
    cmp_all_bytes_test.test_runtime_seconds = runtime;
    cmp_all_bytes_test.context = (void *)&cmpallbytes_context;

    // ===================================================================================
    // Dec All Bytes test
    // ===================================================================================
    struct test_context decallbytes_context = {0};
    decallbytes_context.name = "dec_all_bytes";
    decallbytes_context.datasize = BUFFER_SIZE;
    decallbytes_context.buffer = buffer;

    struct rep_tester_config dec_all_bytes_test = {0};
    dec_all_bytes_test.test_name = decallbytes_context.name;
    dec_all_bytes_test.test_main = dec_all_bytes_main;
    dec_all_bytes_test.print_stats = print_stats;
    dec_all_bytes_test.test_runtime_seconds = runtime;
    dec_all_bytes_test.context = (void *)&decallbytes_context;

    // ===================================================================================
    // Run Tests
    // ===================================================================================

    struct rep_tester_config tests[] = {
        write_bytes_test,
        move_all_bytes_test,
        nop_all_bytes_test,
        cmp_all_bytes_test,
        dec_all_bytes_test
    };

    int count = (int) (sizeof(tests)/sizeof(struct rep_tester_config));
    printf("Count tests: %d\n", count);
    for (int i=0; i<count; ++i) {
        printf("[%d] Test [%s] context[0x%p]\n", i, tests[i].test_name, tests[i].context);
    }

    rep_tester_run(tests, count);

    // ===================================================================================

    printf("\n\n");
    printf("=========================\n");
    printf("Summary\n");
    printf("=========================\n\n");

    uint64_t cpu_freq = guess_cpu_freq(100);
    printf("CPU Frequency [%" PRIu64 "] (estimated)\n", cpu_freq);

    for (int i=0; i<count; ++i) {
        printf("[%d] Test [%s] context[0x%p]\n", i, tests[i].test_name, tests[i].context);
        print_stats(tests[i].context);
        printf("\n");
    }

    printf("\n\n");

    free(buffer);

    return 0;
}
