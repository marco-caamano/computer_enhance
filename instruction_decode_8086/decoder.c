#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#define LOG(...) {                  \
        if (verbose) {              \
            printf(__VA_ARGS__);    \
        }                           \
    }

#define BUFFER_SIZE 4096

// mov 100010DW => 10 0010
#define MOV_REGMEM_REG_INSTRUCTION          0x22
// mov 1011WREG => 1011
#define MOV_IMMEDIATE_TO_REG_INSTRUCTION    0xb

bool verbose = false;
FILE *in_fp = NULL;
FILE *out_fp = NULL;

char *register_map[8][2] = {
    { "AL", "AX" }, 
    { "CL", "CX" }, 
    { "DL", "DX" }, 
    { "BL", "BX" }, 
    { "AH", "SP" }, 
    { "CH", "BP" }, 
    { "DH", "SI" }, 
    { "BH", "DI" }
};

char *mov_source_effective_address[] = {
    "BX + SI",
    "BX + DI",
    "BP + SI",
    "BP + DI",
    "SI",
    "DI",
    "BP",
    "BX"
};

void usage(void) {
    fprintf(stderr, "8086 Instruction Decoder Usage:\n");
    fprintf(stderr, "-h         This help dialog.\n");
    fprintf(stderr, "-i <file>  Path to file to parse.\n");
    fprintf(stderr, "-o <file>  Path to file to generate output.\n");
    fprintf(stderr, "-v         Enable verbose output.\n");
}

/*
 * Process a MOV immediate to register
 *
 * Returns number of bytes where consumed, so the caller
 * can skip ahead to process the next unprocessed bytes
 * in the buffer
 */
int process_mov_immediate_to_register(uint8_t *ptr) {
    uint8_t data_low = 0;
    uint8_t data_high = 0;
    int16_t data = 0;
    int consumed_bytes = 1;
    uint8_t bit_w = (*ptr>>3) & 0x1;
    uint8_t reg = (*ptr & 0x7);
    char *reg_str = register_map[reg][bit_w];
    LOG("; [0x%x] Found IMMEDIATE TO REG MOV bitstream | W[%d] REG[0x%x][%s] | ", *ptr, bit_w, reg, reg_str);
    // consume 2nd byte
    ptr++;
    consumed_bytes++;
    data_low = *ptr;
    LOG("Data Low[0x%02x] ", data_low);
    if (bit_w == 1) {
        // consume 3rd byte
        ptr++;
        consumed_bytes++;
        data_high = *ptr;
        LOG("Data High[0x%02x] ", data_high);
    }
    data = (data_high << 8) | data_low;
    LOG("| Data [0x%04x][%d]\n", data, data);
    
    
    printf("mov %s,%d\n", reg_str, data);
    fprintf(out_fp, "mov %s,%d\n", reg_str, data);
    return consumed_bytes;
}

/*
 * Process a MOV register/memory to/from register
 *
 * Returns number of bytes where consumed, so the caller
 * can skip ahead to process the next unprocessed bytes
 * in the buffer
 */
int process_mov_regmem_reg_inst(uint8_t *ptr) {
    uint8_t data_low = 0;
    uint8_t data_high = 0;
    int16_t data = 0;
    uint8_t bit_d = (*ptr & 0x2)>>1;
    uint8_t bit_w = *ptr & 0x1;
    char *destination = NULL;
    char *source = NULL;
    int consumed_bytes = 1;
    LOG("; [0x%x] Found REG/MEM TO/FROM REGISTER MOV bitstream | D[%d] W[%d]\n", *ptr, bit_d, bit_w);
    // consume 2nd byte
    ptr++;
    uint8_t mod = (*ptr >> 6) & 0x3;
    uint8_t reg = (*ptr >> 3) & 0x7;
    uint8_t r_m = (*ptr >> 0) & 0x7;
    consumed_bytes++;
    LOG("; [0x%x] 2nd Byte - MOD[0x%x] REG[0x%x] R/M[0x%x]\n", *ptr, mod, reg, r_m);
    switch (mod) {
        case 0x0:
            // Memory Mode, No Displacement
            // * except when R/M is 110 then 16bit displacement follows
            if (r_m == 0x6) {
                // special case R/M is 110
                // 16 bit displacement follows
                ptr++;
                data_low = *ptr;
                LOG("; [0x%x] 3rd Byte - Data Low[0x%02x] \n", data_low, data_low);
                ptr++;
                data_high = *ptr;
                LOG("; [0x%x] 4th Byte - Data High[0x%02x] | ", data_high, data_high);
                data = (data_high << 8) | data_low;
                LOG(" Data [0x%04x][%d]\n", data, data);
                consumed_bytes+=2;
                // destination specificed in REG field
                destination = register_map[reg][bit_w];
                // source is the direct address
                printf("mov %s, [%d]\n", destination, data);
                fprintf(out_fp, "mov %s, [%d]\n", destination, data);
            } else {
                if (bit_d == 0x1) {
                    // destination specificed in REG field
                    destination = register_map[reg][bit_w];
                    // source specified in effective address table
                    source = mov_source_effective_address[r_m];
                    printf("mov %s, [%s]\n", destination, source);
                    fprintf(out_fp, "mov %s, [%s]\n", destination, source);
                } else {
                    // destination specificed in R/M field
                    destination = mov_source_effective_address[r_m];
                    // source specified in REG field
                    source = register_map[reg][bit_w];
                    printf("mov [%s], %s\n", destination, source);
                    fprintf(out_fp, "mov [%s], %s\n", destination, source);
                }
            }
            break;
        case 0x1:
            // Memory Mode, 8bit displacement follows
            ptr++;
            consumed_bytes++;
            // check if we need to do 8bit to 16bit signed extension
            if ((*ptr>>7) == 0x1 ) {
                // MSB bit is set, then do signed extension
                data = (0xFF<<8) | *ptr;
                LOG("; [0x%x] 3rd Byte - Data[0x%x] signed extended to [%d]\n", *ptr, *ptr, data);
            } else {
                data = *ptr;
                LOG("; [0x%x] 3rd Byte - Data[0x%x][%d]\n", data, data, data);
            }
            if (bit_d == 0x1) {
                // destination specificed in REG field
                destination = register_map[reg][bit_w];
                // source specified in effective address table
                source = mov_source_effective_address[r_m];
                if (data==0) {
                    // don't output " + 0" in the effective address calculation
                    printf("mov %s, [%s]\n", destination, source);
                    fprintf(out_fp, "mov %s, [%s]\n", destination, source);
                } else {
                    printf("mov %s, [%s + %d]\n", destination, source, data);
                    fprintf(out_fp, "mov %s, [%s + %d]\n", destination, source, data);
                }
            } else {
                // destination specificed in R/M field
                destination = mov_source_effective_address[r_m];
                // source specified in REG field
                source = register_map[reg][bit_w];
                if (data==0) {
                    // don't output " + 0" in the effective address calculation
                    printf("mov [%s], %s\n", destination, source);
                    fprintf(out_fp, "mov [%s], %s\n", destination, source);
                } else {
                    printf("mov [%s + %d], %s\n", destination, data, source);
                    fprintf(out_fp, "mov [%s + %d], %s\n", destination,  data, source);
                }
            }
            break;
        case 0x2:
            // Memory Mode, 16bit displacement follows
            ptr++;
            data_low = *ptr;
            LOG("; [0x%x] 3rd Byte - Data Low[0x%02x] \n ", data_low, data_low);
            ptr++;
            data_high = *ptr;
            LOG(" [0x%x] 4th Byte - Data High[0x%02x] | ", data_high, data_high);
            data = (data_high << 8) | data_low;
            LOG(" Data [0x%04x][%d]\n", data, data);
            consumed_bytes+=2;
            if (bit_d == 0x1) {
                // destination specificed in REG field
                destination = register_map[reg][bit_w];
                // source specified in effective address table
                source = mov_source_effective_address[r_m];
                if (data==0) {
                    // don't output " + 0" in the effective address calculation
                    printf("mov %s, [%s]\n", destination, source);
                    fprintf(out_fp, "mov %s, [%s]\n", destination, source);
                } else {
                    printf("mov %s, [%s + %d]\n", destination, source, data);
                    fprintf(out_fp, "mov %s, [%s + %d]\n", destination, source, data);
                }
            } else {
                // destination specificed in R/M field
                destination = mov_source_effective_address[r_m];
                // source specified in REG field
                source = register_map[reg][bit_w];
                if (data==0) {
                    // don't output " + 0" in the effective address calculation
                    printf("mov [%s], %s\n", destination, source);
                    fprintf(out_fp, "mov [%s], %s\n", destination, source);
                } else {
                    printf("mov [%s + %d], %s\n", destination, data, source);
                    fprintf(out_fp, "mov [%s + %d], %s\n", destination,  data, source);
                }
            }
            break;
        case 0x3:
            // Register Mode, No Displacement
            // get destination/source
            if (bit_d == 0x1) {
                // destination specificed in REG field
                destination = register_map[reg][bit_w];
                // source specificed in R/M field
                source = register_map[r_m][bit_w];
            } else {
                // destination specificed in R/M field
                destination = register_map[r_m][bit_w];
                // source specificed in REG field
                source = register_map[reg][bit_w];
            }
            printf("mov %s,%s\n", destination, source);
            fprintf(out_fp, "mov %s,%s\n", destination, source);
            break;
    }
    return consumed_bytes;
}

int main (int argc, char *argv[]) {
    int opt;
    char *input_file = NULL;
    char *output_file = NULL;    
    uint8_t *buffer = NULL;
    size_t bytes_available;

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

    LOG("========================\n");
    LOG("8086 Instruction Decoder\n");
    LOG("========================\n");

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

    LOG("Using Input Filename     [%s]\n", input_file);
    LOG("Using Output Filename    [%s]\n", output_file);
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

    out_fp = fopen(output_file, "w");
    if (!in_fp) {
        fprintf(stderr, "ERROR output_file fopen failed [%d][%s]\n", errno, strerror(errno));
        exit(1);
    }
    LOG("Opened Output file[%s] OK\n\n", output_file);

    LOG("Decoding Results:\n");
    LOG("-----------------\n");


    printf("; 8086 Instruction Decoder Result Output\n\n");
    fprintf(out_fp, "; 8086 Instruction Decoder Result Output\n\n");
    printf("bits 16\n\n");
    fprintf(out_fp, "bits 16\n\n");

    bytes_available = -1;
    while ( (bytes_available = fread(buffer, 1, BUFFER_SIZE, in_fp)) != 0) {
        uint8_t *ptr = buffer;
        LOG("; Read [%zu] bytes from file\n\n", bytes_available);

        // 8086 instructions can be encoded from 1 to 6 bytes
        // so we inspect on a per byte basis
        while (bytes_available>0) {
            int consumed_bytes = 0;
            if ((*ptr>>4) == MOV_IMMEDIATE_TO_REG_INSTRUCTION) {
                // found immediate to register move
                consumed_bytes = process_mov_immediate_to_register(ptr);
            } else if ( (*ptr>>2) == MOV_REGMEM_REG_INSTRUCTION) {
                // found move register/memory to/from register
                consumed_bytes = process_mov_regmem_reg_inst(ptr);
            } else {
                LOG("; [0x%x] not a recognized instruction, continue search...\n", *ptr);
                consumed_bytes = 1;
            }
            LOG("\n");
            ptr+=consumed_bytes;
            bytes_available -= consumed_bytes;
        }
    }
    LOG("; Read [%zu] bytes from file\n", bytes_available);

    fclose(in_fp);
    fclose(out_fp);

    return 0;
}