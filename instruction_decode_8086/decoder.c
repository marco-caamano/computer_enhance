#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_SIZE 4096

// mov 100010DW => 10 0010
#define MOV_BITSTREAM   0x22

char *registers_w0[] = {
    "AL",
    "CL",
    "DL",
    "BL",
    "AH",
    "CH",
    "DH",
    "BH"
};

char *registers_w1[] = {
    "AX",
    "CX",
    "DX",
    "BX",
    "SP",
    "BP",
    "SI",
    "DI"
};

void usage(void) {
    fprintf(stderr, "8086 Instruction Decoder Usage:\n");
    fprintf(stderr, "-h         This help dialog.\n");
    fprintf(stderr, "-i <file>  Path to file to parse.\n");
    fprintf(stderr, "-o <file>  Path to file to generate output.\n");
    fprintf(stderr, "-v         Enable verbose output.\n");
}

int main (int argc, char *argv[]) {
    int opt;
    char *input_file = NULL;
    char *output_file = NULL;
    bool verbose = false;
    uint8_t *buffer = NULL;
    FILE *in_fp = NULL;
    FILE *out_fp = NULL;
    size_t ret;

    printf("========================\n");
    printf("8086 Instruction Decoder\n");
    printf("========================\n");

    while( (opt = getopt(argc, argv, "hi:o:v")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;
            
            case 'i':
                input_file = strdup(optarg);
                break;

            case 'o':
                output_file = strdup(optarg);
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

    if (!input_file) {
        fprintf(stderr, "ERROR Missing Input File\n");
        usage();
        exit(1);
    }
    if (!output_file) {
        fprintf(stderr, "ERROR Missing Output File\n");
        usage();
        exit(1);
    }

    printf("Using Input Filename     [%s]\n", input_file);
    printf("Using Output Filename    [%s]\n", output_file);
    printf("Using Verbose            [%s]\n", verbose ? "True" : "False");
    printf("\n\n");

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
    printf("Opened Input file[%s] OK\n", input_file);

    out_fp = fopen(output_file, "w");
    if (!in_fp) {
        fprintf(stderr, "ERROR output_file fopen failed [%d][%s]\n", errno, strerror(errno));
        exit(1);
    }
    printf("Opened Output file[%s] OK\n", output_file);

    fprintf(out_fp, "; 8086 Instruction Parser Result Output\n");
    fprintf(out_fp, "bits 16\n\n");

    ret = -1;
    while ( (ret = fread(buffer, 1, BUFFER_SIZE, in_fp)) != 0) {
        uint8_t *ptr = buffer;
        printf("Read [%zu] bytes from file\n", ret);

        // 8086 instructions can be encoded from 1 to 6 bytes
        // so we inspect on a per byte basis
        for (int i=0; i<ret; i++) {
            // is the next byte in the buffer a mov instruction?
            // just inspect bits 8->2 for MOV code
            if ( (*ptr>>2) == MOV_BITSTREAM) {
                uint8_t bit_d = (*ptr & 0x2)>>1;
                uint8_t bit_w = *ptr & 0x1;
                printf("\t[0x%x] Found MOV bitstream | D[%d] W[%d] | ", *ptr, bit_d, bit_w);
                ptr++;
                i++;
                // consume 2nd byte
                uint8_t mod = (*ptr >> 6) & 0x3;
                uint8_t reg = (*ptr >> 3) & 0x7;
                uint8_t r_m = (*ptr >> 0) & 0x7;
                printf("2nd Byte[0x%x] - MOD[0x%x] REG[0x%x] R/M[0x%x]\n", *ptr, mod, reg, r_m);
                if (mod != 0x3) {
                    printf("\t\tFound MOD is not Register Mode, skipping decode...\n");
                } else {
                    char *destination = NULL;
                    char *source = NULL;
                    // get destination/source
                    if (bit_d == 0x1) {
                        // destination specificed in REG field
                        if (bit_w == 0x1) {
                            destination = registers_w1[reg];
                        } else {
                            destination = registers_w0[reg];
                        }
                        // source specificed in R/M field
                        if (bit_w == 0x1) {
                            source = registers_w1[r_m];
                        } else {
                            source = registers_w0[r_m];
                        }
                    } else {
                        // destination specificed in R/M field
                        if (bit_w == 0x1) {
                            destination = registers_w1[r_m];
                        } else {
                            destination = registers_w0[r_m];
                        }
                        // source specificed in REG field
                        if (bit_w == 0x1) {
                            source = registers_w1[reg];
                        } else {
                            source = registers_w0[reg];
                        }
                    }
                    printf("\t\tmov %s,%s\n", destination, source);
                    fprintf(out_fp, "mov %s,%s\n", destination, source);
                }
            } else {
                printf("\t[0x%x] not a MOV instruction, continue search...\n", *ptr);
            }
            ptr++;
        }
        
 
    }
    printf("Read [%zu] bytes from file\n", ret);

    fclose(in_fp);
    fclose(out_fp);

    return 0;
}