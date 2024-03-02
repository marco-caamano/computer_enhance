#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>

#include "reptester.h"
#include "rdtsc_utils.h"

#define PAGE_SIZE   4096

#define ERROR(...) {                    \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

struct test_context {
    char *name;
    size_t buffer_size;
    uint8_t *buffer;
    uint64_t touch_size;
    uint64_t max_touch_size;
    bool is_test_done;
    uint64_t start_pagefault_count;
};

uint64_t get_page_faults(void) {
    struct rusage usage = {};
    
    int ret = getrusage(RUSAGE_SELF, &usage);
    if (ret != 0) {
        ERROR("Failed to call getrusage errno(%d)[%s]\n", errno, strerror(errno));
    }

    return (usage.ru_minflt + usage.ru_majflt);
}

void env_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    printf("[%s] name[%s]\n", __FUNCTION__, ctx->name);

    // ctx->buffer_size = 1024*1024*1024;
    ctx->buffer_size = 1024*1024;

    ctx->touch_size = 0;
    ctx->max_touch_size = ctx->buffer_size / PAGE_SIZE;
    ctx->is_test_done = false;
    printf("[%s] MMAP Buffer Size                   [%lu] bytes\n", __FUNCTION__, ctx->buffer_size);
    printf("[%s] Max Touch Size                     [%lu] pages\n", __FUNCTION__, ctx->max_touch_size);
    printf("\n");
}

void env_teardown(void *context) {
    struct test_context *ctx = (struct test_context *)context;
    printf("[%s] name[%s]\n", __FUNCTION__, ctx->name);
}

void test_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    // allocate memory
    ctx->buffer = mmap(0, ctx->buffer_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (!ctx->buffer) {
         ERROR("mmap     failed for size[%zu]\n", ctx->buffer_size);
    }

    ctx->start_pagefault_count = get_page_faults();
}

void test_teardown(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    uint64_t current_page_faults = get_page_faults();

    uint64_t elapsed_page_faults = current_page_faults - ctx->start_pagefault_count;
    
    printf("[%s] TouchSize[%lu] | PageFaults: %lu (%lu - %lu) \n", __FUNCTION__, ctx->touch_size, elapsed_page_faults, current_page_faults, ctx->start_pagefault_count);

    ctx->touch_size++;
    if (ctx->touch_size > ctx->max_touch_size) {
        // we are done
        ctx->is_test_done = true;
    }

    // release memory
    munmap(ctx->buffer, ctx->buffer_size);
    ctx->buffer = 0;
}

void test_main(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    uint64_t data = 0x5a5a5a5a5a5a5a5a;
    uint32_t count = (ctx->touch_size*PAGE_SIZE) / sizeof(uint64_t);
    uint64_t *ptr = (uint64_t *)ctx->buffer;

    for (uint32_t i=0; i<count; i++) {
        *ptr = data | i;
        ptr++;
    }
}



bool test_eval_done(void *context) {
    struct test_context *ctx = (struct test_context *)context;
    return ctx->is_test_done;
}

void usage(void) {
    fprintf(stderr, "Rep Test 1 Usage:\n");
    fprintf(stderr, "-h             This help dialog.\n");
    fprintf(stderr, "-t <runtime>   Set runtime in seconds. (defaults to 10seconds)\n");
}

int main (int argc, char *argv[]) {
    int opt;

    while( (opt = getopt(argc, argv, "ht:")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;

            default:
                fprintf(stderr, "ERROR Invalid command line option\n");
                usage();
                exit(1);
                break;
        }
    }

    printf("=============\n");
    printf("Page Faults 1\n");
    printf("=============\n");


    struct rep_tester_config foo = {};
    foo.env_setup = env_setup;
    foo.test_setup = test_setup;
    foo.test_main = test_main;
    foo.test_teardown = test_teardown;
    foo.env_teardown = env_teardown;
    foo.end_of_test_eval = test_eval_done;
    foo.silent = true;

    struct test_context my_context = {};
    my_context.name = "PageFaults_incremental";

    printf("\n\n");

    rep_tester(&foo, &my_context);
    
    printf("\n\n");


    return 0;
}
