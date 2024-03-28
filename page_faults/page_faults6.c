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
 * Single Pass touch each page of allocated memory and
 * see the effects on PageFaults. Only Touch one byte
 * per page. Write out results to csv file for plotting.
 * Add pointer breakdown to results
 */

#define PAGE_SIZE   4096

#define MY_ERROR(...) {                 \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

struct pointer_breakdown {
    uint16_t l4;
    uint16_t l3;
    uint16_t l2;
    uint16_t l1;
    uint16_t offset;
};

struct pointer_breakdown get_pointer_breakdown(uint64_t ptr) {
    struct pointer_breakdown data = {0};
    data.offset = (uint16_t)(ptr & 0xFFF);
    data.l1 = (uint16_t)( (ptr>>12) & 0x1FF);
    data.l2 = (uint16_t)( (ptr>>21) & 0x1FF);
    data.l3 = (uint16_t)( (ptr>>30) & 0x1FF);
    data.l4 = (uint16_t)( (ptr>>39) & 0x1FF);
    return data;
}

void usage(void) {
    fprintf(stderr, "Page Faults 3 Usage:\n");
    fprintf(stderr, "-h             This help dialog.\n");
    fprintf(stderr, "-n <num>       Use <num> PAGES to test.\n");
    fprintf(stderr, "-r             Set reverse sweep of pages\n");
}

int main (int argc, char *argv[]) {
    int opt;
    int num_pages = 0;
    bool reverse = false;
    char *filename;
    FILE *fp;
    size_t buffer_size;
    uint8_t *buffer;

    printf("=========================================\n");
    printf("Page Faults 3: Single Pass PageFault test\n");
    printf("=========================================\n");

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

    if (reverse) {
        filename = "page_fault6_reverse_data.csv";
    } else {
        filename = "page_fault6_data.csv";
    }

    printf("[%s] Using CSV Output File [%s]\n", __FUNCTION__, filename);
    fp = fopen(filename, "w");
    if (!fp) {
        MY_ERROR("Failed to open file [%d][%s]\n", errno, strerror(errno));
    }
    printf("[%s] File Opened OK\n", __FUNCTION__);
    fprintf(fp, "Page,PageFaults,StartFaults,EndFaults,Pointer,L4,L3,L2,L1,Offset\n");

    buffer_size = num_pages * PAGE_SIZE;

    printf("[%s] MMAP Buffer Size                   [%zu] bytes\n", __FUNCTION__, buffer_size);
    printf("[%s] Num Pages                          [%u] pages\n", __FUNCTION__, num_pages);

    // allocate memory
#ifdef _WIN32
    buffer = (uint8_t *)VirtualAlloc(0, buffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#else
    buffer = mmap(0, buffer_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#endif
    if (!buffer) {
         MY_ERROR("mmap     failed for size[%zu]\n", buffer_size);
    }

    // Test
    uint8_t data = 0x5a;
    uint8_t *ptr;
    uint32_t page;
    
    for (uint32_t i=0; i<num_pages; i++) {
        uint64_t start_page_faults = ReadOSPageFaultCount();
        if (reverse) {
            page = num_pages-i;
            ptr = (buffer + (num_pages-i-1)*PAGE_SIZE);
        } else {
            page = i;
            ptr = (buffer + i*PAGE_SIZE);
        }
        *ptr = data;
        uint64_t end_page_faults = ReadOSPageFaultCount();
        uint32_t delta = (uint32_t)(end_page_faults - start_page_faults);
        struct pointer_breakdown ptr_data = get_pointer_breakdown((uint64_t)ptr);
        printf("Page[%u] | PageFaults: %u (%" PRIu64 " - %" PRIu64 ") | Pointer[0x%p] L4[%u] L3[%u] L2[%u] L1[%u] Offset[%u]\n", page, delta, end_page_faults, start_page_faults, ptr, ptr_data.l4, ptr_data.l3, ptr_data.l2, ptr_data.l1, ptr_data.offset);
        fprintf(fp, "%u,%u,%" PRIu64 ",%" PRIu64 ",0x%p,%u,%u,%u,%u,%u\n", page, delta, end_page_faults, start_page_faults, ptr, ptr_data.l4, ptr_data.l3, ptr_data.l2, ptr_data.l1, ptr_data.offset);
    }

    // release memory
#ifdef _WIN32
    VirtualFree(buffer, 0, MEM_RELEASE);
#else
    munmap(buffer, buffer_size);
#endif
    buffer = 0;
   
    printf("\n\n");
    printf("Test Completed OK\n\n");

    fclose(fp);

    return 0;
}
