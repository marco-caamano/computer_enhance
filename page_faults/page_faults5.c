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

#include "rdtsc_utils.h"

/*
 * Single Pass write all the entire pages. Measure
 * Performance
 */

#define PAGE_SIZE   4096

#define MY_ERROR(...) {                 \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

void usage(void) {
    fprintf(stderr, "Page Faults 5\n");
    fprintf(stderr, "Single Pass write all the entire pages. Measure Performance\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "-h             This help dialog.\n");
    fprintf(stderr, "-n <num>       Use <num> PAGES to test.\n");
    fprintf(stderr, "-r             Set reverse sweep of pages\n");
}

int main (int argc, char *argv[]) {
    int opt;
    int num_pages = 0;
    bool reverse = false;
    size_t buffer_size;
    uint8_t *buffer;

    printf("=====================================================\n");
    printf("Page Faults 5: Single Pass PageFault Performance test\n");
    printf("=====================================================\n");

#ifdef _WIN32
    for (int index=1; index<argc; ++index) {
        if (strcmp(argv[index], "-h")==0) {
            usage();
            exit(0);
        } else if (strcmp(argv[index], "-n")==0) {
            // must have at least index+2 arguments to contain a file
            if (argc<index+2) {
                printf("ERROR: missing input file parameter\n");
                usage();
                exit(1);
            }
            num_pages = atoi(argv[index+1]);
            // since we consume the next parameter then skip it
            ++index;
        } else if (strcmp(argv[index], "-r")==0) {
            reverse = true;
        }
    }
#else
    while( (opt = getopt(argc, argv, "hn:r")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;

            case 'n':
                num_pages = atoi(optarg);
                break;

            case 'r':
                reverse = true;
                break;

            default:
                fprintf(stderr, "MY_ERROR Invalid command line option\n");
                usage();
                exit(1);
                break;
        }
    }
#endif

    if (num_pages==0) {
        fprintf(stderr, "ERROR missing number of pages to test with\n");
        usage();
        exit(1);
    }

    buffer_size = num_pages * PAGE_SIZE;

    printf("[%s] Buffer Size                        [%zu] bytes\n", __FUNCTION__, buffer_size);
    printf("[%s] Num Pages                          [%u] pages\n", __FUNCTION__, num_pages);

    // allocate memory
#ifdef _WIN32
    buffer = (uint8_t *)VirtualAlloc(0, buffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#else
    buffer = mmap(0, buffer_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#endif
    if (!buffer) {
         MY_ERROR("Allocation failed for size[%zu]\n", buffer_size);
    }

    // Test
    uint64_t data = 0x5a5a5a5a5a5a5a5a;
    uint64_t *ptr;
    uint64_t bytes_written = 0;
    
    uint64_t start = GET_CPU_TICKS();
    for (uint32_t i=0; i<num_pages; i++) {
        if (reverse) {
            ptr = (uint64_t *)(buffer + (num_pages-i-1)*PAGE_SIZE);
            for (uint32_t j=0; j<(PAGE_SIZE/sizeof(uint64_t)); j++) {
                *ptr++ = data;
                bytes_written += sizeof(uint64_t);
            }
        } else {
            ptr = (uint64_t *)(buffer + i*PAGE_SIZE);
            for (uint32_t j=0; j<(PAGE_SIZE/sizeof(uint64_t)); j++) {
                *ptr++ = data;
                bytes_written += sizeof(uint64_t);
            }
        }
    }
    uint64_t elapsed_ticks = GET_CPU_TICKS() - start;

    printf("\n\n");

    printf("Bytes Written[%" PRIu64"] ", bytes_written);
    print_data_speed(bytes_written, elapsed_ticks);

    // release memory
#ifdef _WIN32
    VirtualFree(buffer, 0, MEM_RELEASE);
#else
    munmap(buffer, buffer_size);
#endif
    buffer = 0;
   
    printf("\n\n");
    printf("Test Completed OK\n\n");

    return 0;
}
