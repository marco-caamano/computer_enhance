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
 */

#define PAGE_SIZE   4096

#define MY_ERROR(...) {                 \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

int main (int argc, char *argv[]) {
    char *filename;
    FILE *fp;
    size_t buffer_size;
    uint8_t *buffer;
    uint32_t max_touch_count;

    printf("=========================================\n");
    printf("Page Faults 3: Single Pass PageFault test\n");
    printf("=========================================\n");

    filename = "page_fault3_data.csv";

    printf("[%s] Using CSV Output File [%s]\n", __FUNCTION__, filename);
    fp = fopen(filename, "w");
    if (!fp) {
        MY_ERROR("Failed to open file [%d][%s]\n", errno, strerror(errno));
    }
    printf("[%s] File Opened OK\n", __FUNCTION__);
    fprintf(fp, "Page,PageFaults,StartFaults,EndFaults\n");

    buffer_size = 5*1024*1024;

    max_touch_count = buffer_size / PAGE_SIZE;
    printf("[%s] MMAP Buffer Size                   [%zu] bytes\n", __FUNCTION__, buffer_size);
    printf("[%s] Max Touch Size                     [%u] pages\n", __FUNCTION__, max_touch_count);

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
    
    for (uint32_t i=0; i<max_touch_count; i++) {
        uint64_t start_page_faults = ReadOSPageFaultCount();
        uint8_t *ptr = (buffer + i*PAGE_SIZE);
        *ptr = data;
        uint64_t end_page_faults = ReadOSPageFaultCount();
        uint32_t delta = (uint32_t)(end_page_faults - start_page_faults);
        printf("Page[%u] | PageFaults: %u (%" PRIu64 " - %" PRIu64 ") \n", i, delta, end_page_faults, start_page_faults);
        fprintf(fp, "%u,%u,%" PRIu64 ",%" PRIu64 "\n", i, delta, end_page_faults, start_page_faults);
    }

    // release memory
#ifdef _WIN32
    VirtualFree(buffer, 0, MEM_RELEASE);
#else
    munmap(buffer, buffer_size);
#endif
    buffer = 0;
   
    printf("\n\n");

    fclose(fp);

    return 0;
}
