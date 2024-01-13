#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include "libdecoder.h"

#define BUFFER_SIZE 1024*1024

void usage(void) {
    fprintf(stderr, "8086 Instruction Decoder Usage:\n");
    fprintf(stderr, "-h         This help dialog.\n");
    fprintf(stderr, "-i <file>  Path to file to parse.\n");
    fprintf(stderr, "-v         Enable verbose output.\n");
}



int main (int argc, char *argv[]) {
    int opt;
    char *input_file = NULL;
    uint8_t *buffer = NULL;
    size_t bytes_available = 0;
    bool verbose = false;
    FILE *in_fp = NULL;

    while( (opt = getopt(argc, argv, "hi:v")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;
            
            case 'i':
                input_file = strdup(optarg);
                break;

            case 'v':
                verbose = true;
                break;

            default:
                fprintf(stderr, "ERROR Invalid command line option\n");
                usage();
                exit(1);
                break;
        }
    }

    LOG("=============================\n");
    LOG("Test 8086 Instruction Decoder\n");
    LOG("=============================\n");

    if (!input_file) {
        fprintf(stderr, "ERROR Missing Input File\n");
        usage();
        exit(1);
    }

    LOG("Using Input Filename     [%s]\n", input_file);
    LOG("\n\n");

    buffer = malloc(BUFFER_SIZE);
    if (!buffer) {
        fprintf(stderr, "ERROR malloc failed\n");
        exit(1);
    }
    
    in_fp = fopen(input_file, "r");
    if (!in_fp) {
        fprintf(stderr, "ERROR input_file fopen failed [%d][%s]\n", errno, strerror(errno));
        exit(1);
    }
    LOG("Opened Input file[%s] OK\n", input_file);

    bytes_available = fread(buffer, 1, BUFFER_SIZE, in_fp);
    fclose(in_fp);

    LOG("Read [%zu] bytes from file\n\n", bytes_available);

    size_t consumed = decode_bitstream(buffer, bytes_available, verbose);
    if (consumed == bytes_available) {
        LOG("decode_bitstream consumed all %zu bytes from file\n", bytes_available);
    } else {
        ERROR("decode_bitstream failed to consume all %zu bytes from file (consumed %zu bytes)\n", bytes_available, consumed);
    }

    return 0;
}
