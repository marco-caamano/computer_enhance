#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <getopt.h>
#include <unistd.h>
#include <sys/mman.h>
#endif
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "reptester.h"
#include "rdtsc_utils.h"

#define PAGE_SIZE   4096

#define MY_ERROR(...) {                    \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

struct test_context {
    char *name;
    size_t buffer_size;
    uint8_t *buffer;
    uint32_t touch_size;
    uint32_t max_touch_size;
    bool is_test_done;
    uint32_t start_pagefault_count;
};


void env_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    printf("[%s] name[%s]\n", __FUNCTION__, ctx->name);

    // ctx->buffer_size = 1024*1024*1024;
    ctx->buffer_size = 1024*1024;

    ctx->touch_size = 0;
    ctx->max_touch_size = ctx->buffer_size / PAGE_SIZE;
    ctx->is_test_done = false;
    printf("[%s] MMAP Buffer Size                   [%zu] bytes\n", __FUNCTION__, ctx->buffer_size);
    printf("[%s] Max Touch Size                     [%u] pages\n", __FUNCTION__, ctx->max_touch_size);
    printf("\n");
}

void env_teardown(void *context) {
    // struct test_context *ctx = (struct test_context *)context;
    // printf("[%s] name[%s]\n", __FUNCTION__, ctx->name);
}

void test_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    // allocate memory
#ifdef _WIN32
    ctx->buffer = (uint8_t *)VirtualAlloc(0, ctx->buffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#else
    ctx->buffer = mmap(0, ctx->buffer_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#endif
    if (!ctx->buffer) {
         MY_ERROR("mmap     failed for size[%zu]\n", ctx->buffer_size);
    }

    ctx->start_pagefault_count = ReadOSPageFaultCount();
}

void test_teardown(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    uint32_t current_page_faults = (uint32_t)ReadOSPageFaultCount();

    uint32_t elapsed_page_faults = current_page_faults - ctx->start_pagefault_count;
    
    printf("[%s] TouchSize[%u] | PageFaults: %u (%u - %u) \n", __FUNCTION__, ctx->touch_size, elapsed_page_faults, current_page_faults, ctx->start_pagefault_count);

    ctx->touch_size++;
    if (ctx->touch_size > ctx->max_touch_size) {
        // we are done
        ctx->is_test_done = true;
    }

    // release memory
#ifdef _WIN32
    VirtualFree(ctx->buffer, 0, MEM_RELEASE);
#else
    munmap(ctx->buffer, ctx->buffer_size);
#endif
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

#ifdef _WIN32
    for (int index=1; index<argc; ++index) {
        if (strcmp(argv[index], "-h")==0) {
            usage();
            exit(0);
        }
    }
#else
    while( (opt = getopt(argc, argv, "ht:")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;

            default:
                fprintf(stderr, "MY_ERROR Invalid command line option\n");
                usage();
                exit(1);
                break;
        }
    }
#endif
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
