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

struct test_context {
    char *name;
    char *filename;
    size_t filesize;
    uint8_t *buffer;
    FILE *fp;
    uint64_t min_cpu_ticks;
    uint64_t max_cpu_ticks;
    uint64_t faults_before;
    uint64_t faults_after;
};

void env_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;
    struct stat statbuf = {};
    int ret;

    printf("[%s] name[%s]\n", __FUNCTION__, ctx->name);
    ret = stat(ctx->filename, &statbuf);
    if (ret != 0) {
        MY_ERROR("Unable to get filestats[%d][%s]\n", errno, strerror(errno));
    }
    printf("[%s] Using input_file              [%s]\n", __FUNCTION__, ctx->filename);
    printf("[%s]   File Size                   [%lu] bytes\n", __FUNCTION__, statbuf.st_size);
#ifndef _WIN32
    printf("[%s]   IO Block Size               [%lu] bytes\n", __FUNCTION__, statbuf.st_blksize);
    printf("[%s]   Allocated 512B Blocks       [%lu]\n", __FUNCTION__, statbuf.st_blocks);
#endif

    ctx->filesize = statbuf.st_size;
    ctx->buffer = malloc(ctx->filesize);

    if (!ctx->buffer) {
         MY_ERROR("Malloc failed for size[%zu]\n", ctx->filesize);
    }
    printf("[%s] Allocated Buffer              [%zu] bytes @ [%p]\n", __FUNCTION__, ctx->filesize, ctx->buffer);

#if _WIN32
    ctx->fp = fopen(ctx->filename, "rb");
#else
    ctx->fp = fopen(ctx->filename, "r");
#endif
    if (!ctx->fp) {
        MY_ERROR("Failed to open file [%d][%s]\n", errno, strerror(errno));
    }
    printf("[%s] File Opened OK\n", __FUNCTION__);
}

void env_teardown(void *context) {
    struct test_context *ctx = (struct test_context *)context;
    printf("[%s] name[%s]\n", __FUNCTION__, ctx->name);

    fclose(ctx->fp);

    free(ctx->buffer);
    ctx->buffer = 0;
}

void test_setup(void *context) {
    struct test_context *ctx = (struct test_context *)context;
    // reset file to start
    int ret = fseek(ctx->fp, 0, SEEK_SET);
    if (ret!=0) {
        MY_ERROR("Fseek Failed\n");
    }
}

void test_main(void *context) {
    bool new_new_line = false;
    struct test_context *ctx = (struct test_context *)context;

    // Sample PageFaults Before
    ctx->faults_before = ReadOSPageFaultCount();

    // Measure Actual Process
    uint64_t start = GET_CPU_TICKS();
    size_t items_read = fread((void *)ctx->buffer, ctx->filesize, 1, ctx->fp);
    uint64_t elapsed_ticks = GET_CPU_TICKS() - start;

    // Sample PageFaults After
    ctx->faults_after  = ReadOSPageFaultCount();

    if (items_read != 1) {
        MY_ERROR("Failed to read expected items [1] instead read[%zu] | ferror(%d)\n", items_read, ferror(ctx->fp));
    }

    if (ctx->min_cpu_ticks==0 || elapsed_ticks < ctx->min_cpu_ticks) {
        ctx->min_cpu_ticks = elapsed_ticks;
        printf(" | New MinTime");
        print_data_speed(ctx->filesize, elapsed_ticks);
        new_new_line = true;
    }
    if (elapsed_ticks > ctx->max_cpu_ticks) {
        ctx->max_cpu_ticks = elapsed_ticks;
        printf(" | New MaxTime");
        print_data_speed(ctx->filesize, elapsed_ticks);
        new_new_line = true;
    }

    if (new_new_line) {
        printf("\n");
    }
}

void test_teardown(void *context) {
    // struct test_context *ctx = (struct test_context *)context;
}


void print_stats(void *context) {
    struct test_context *ctx = (struct test_context *)context;

    printf("[%s] name[%s]\n", __FUNCTION__, ctx->name);
    printf("\n");

    printf("[%s] Slowest Speed", __FUNCTION__);
    print_data_speed(ctx->filesize, ctx->max_cpu_ticks);
    printf("\n\n");

    printf("[%s] Fastest Speed", __FUNCTION__);
    print_data_speed(ctx->filesize, ctx->min_cpu_ticks);
    printf("\n\n");

    printf("[%s] PageFautls: %" PRIu32 "\n", __FUNCTION__, (uint32_t)(ctx->faults_after - ctx->faults_before));

    printf("\n");
}




void usage(void) {
    fprintf(stderr, "Rep Test 1 Usage:\n");
    fprintf(stderr, "-h             This help dialog.\n");
    fprintf(stderr, "-i <filename>  Use <filename> as input.\n");
    fprintf(stderr, "-t <runtime>   Set runtime in seconds. (defaults to 10seconds)\n");
}

int main (int argc, char *argv[]) {
    int opt;
    char *filename = NULL;
    int runtime = 10;

#ifdef _WIN32
    for (int index=1; index<argc; ++index) {
        if (strcmp(argv[index], "-h")==0) {
            usage();
            exit(0);
        } else if (strcmp(argv[index], "-i")==0) {
            // must have at least index+2 arguments to contain a file
            if (argc<index+2) {
                printf("ERROR: missing input file parameter\n");
                usage();
                exit(1);
            }
            filename = strdup(argv[index+1]);
            // since we consume the next parameter then skip it
            ++index;
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
#else
    while( (opt = getopt(argc, argv, "hi:t:")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;

            case 'i':
                filename = strdup(optarg);
                break;

            case 't':
                runtime = atoi(optarg);
                break;

            default:
                fprintf(stderr, "MY_ERROR Invalid command line option\n");
                usage();
                exit(1);
                break;
        }
    }
#endif

    if (!filename) {
        MY_ERROR("Must pass a filename\n");
    }

    printf("==============\n");
    printf("REP Test 1\n");
    printf("==============\n");


    printf("Using filename  [%s]\n", filename);
    printf("Using runtime   [%d]seconds\n", runtime);

    struct rep_tester_config foo = {};
    foo.env_setup = env_setup;
    foo.test_setup = test_setup;
    foo.test_main = test_main;
    foo.test_teardown = test_teardown;
    foo.env_teardown = env_teardown;
    foo.print_stats = print_stats;
    foo.test_runtime_seconds = runtime;

    struct test_context my_context = {};
    my_context.name = "FreadTest1";
    my_context.filename = filename;


    printf("\n\n");

    rep_tester(&foo, &my_context);
    
    printf("\n\n");


    return 0;
}
