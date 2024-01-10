#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

/* 8086 can address up to 1MB of Memory
 * We allocate a 1MB buffer and load the instructions 
 * to the start of memory and then start executing from
 * there
 */
#define MEMORY_SIZE 1024*1024

#define LOG(...) {                                  \
        if (verbose) {                              \
            if (trace) {                            \
                printf("; [line:%d] ", __LINE__);   \
            }                                       \
            printf(__VA_ARGS__);                    \
        }                                           \
    }

#define ERROR(...) {                    \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }

#define OUTPUT(...) {               \
    printf(__VA_ARGS__);            \
}

#define DUMP_SEGMENT_REGS() {               \
    if (verbose && trace) {                 \
        for (int i=0; i<4; i++) {           \
            printf("; \t%s: 0x%04x (%d)\n", \
                segment_register_map[i],    \
                segment_registers[i],       \
                segment_registers[i]);      \
        }                                   \
    }                                       \
}

// mov 100010DW => 10 0010
#define MOV_REGMEM_REG_INSTRUCTION          0x22
// mov 1100011W => 110 0011
#define MOV_IMMEDIATE_TO_REGMEM_INSTRUCTION 0x63
// mov 1011WREG => 1011
#define MOV_IMMEDIATE_TO_REG_INSTRUCTION    0x0b
// mov 1010000W => 101 0000
#define MOV_MEMORY_TO_ACCUMULATOR           0x50
// mov 1010001W => 101 0001
#define MOV_ACCUMULATOR_TO_MEMORY           0x51
// mov 10001110 => 1000 1110
#define MOV_REGMEM_TO_SEGMENT_INSTRUCTION   0x8e
// mov 10001100 => 1000 1100
#define MOV_SEGMENT_TO_REGMEM_INSTRUCTION   0x8c

bool verbose = false;
bool trace = false;
FILE *in_fp = NULL;

enum registers_e {
    AX,
    BX,
    CX,
    DX,
    SP,
    BP,
    SI,
    DI,
    MAX_REGS
};

uint16_t registers[MAX_REGS] = { 0 };

struct reg_definition_s {
    enum registers_e reg;
    char *name;
    uint16_t mask;
    uint8_t shift;
};

#define REG_DEF_AX { AX, "ax", 0xFFFF, 0 }
#define REG_DEF_AL { AX, "al", 0x00FF, 0 }
#define REG_DEF_AH { AX, "ah", 0xFF00, 8 }
#define REG_DEF_BX { BX, "bx", 0xFFFF, 0 }
#define REG_DEF_BL { BX, "bl", 0x00FF, 0 }
#define REG_DEF_BH { BX, "bh", 0xFF00, 8 }
#define REG_DEF_CX { CX, "cx", 0xFFFF, 0 }
#define REG_DEF_CL { CX, "cl", 0x00FF, 0 }
#define REG_DEF_CH { CX, "ch", 0xFF00, 8 }
#define REG_DEF_DX { DX, "dx", 0xFFFF, 0 }
#define REG_DEF_DL { DX, "dl", 0x00FF, 0 }
#define REG_DEF_DH { DX, "dh", 0xFF00, 8 }
#define REG_DEF_SP { SP, "sp", 0xFFFF, 0 }
#define REG_DEF_BP { BP, "bp", 0xFFFF, 0 }
#define REG_DEF_SI { SI, "si", 0xFFFF, 0 }
#define REG_DEF_DI { DI, "di", 0xFFFF, 0 }

struct reg_definition_s register_map[8][2] = {
    { REG_DEF_AL, REG_DEF_AX },
    { REG_DEF_CL, REG_DEF_CX },
    { REG_DEF_DL, REG_DEF_DX },
    { REG_DEF_BL, REG_DEF_BX },
    { REG_DEF_AH, REG_DEF_SP },
    { REG_DEF_CH, REG_DEF_BP },
    { REG_DEF_DH, REG_DEF_SI },
    { REG_DEF_BH, REG_DEF_DI },
};

char *mov_source_effective_address[] = {
    "bx + si",
    "bx + di",
    "bp + si",
    "bp + di",
    "si",
    "di",
    "bp",
    "bx"
};

char *segment_register_map[] = {
    "es",
    "cs",
    "ss",
    "ds"
};

uint16_t segment_registers[4] = { 0 };

char *byte_count_str[] = {
    "",
    "1st",
    "2nd",
    "3rd",
    "4th",
    "5th",
    "6th"
};

uint16_t read_register(struct reg_definition_s item) {
    uint16_t mask = item.mask;
    uint8_t shift = item.shift;
    return (registers[item.reg] & mask) >> shift;
}

uint16_t write_register(struct reg_definition_s item, uint16_t value) {
    uint16_t mask = item.mask;
    uint8_t shift = item.shift;
    uint16_t data = registers[item.reg] & ~mask;
    // printf("\t - mask 0x%x | shift %d | register[%s][0x%x] | data 0x%x\n", mask, shift, item.name, registers[item.reg], data);
    data |= (value<<shift) & mask;
    registers[item.reg] = data;
    return registers[item.reg];
}

void usage(void) {
    fprintf(stderr, "8086 Instruction Simulator Usage:\n");
    fprintf(stderr, "-h         This help dialog.\n");
    fprintf(stderr, "-i <file>  Path to file to parse.\n");
    fprintf(stderr, "-v         Enable verbose output.\n");
    fprintf(stderr, "-t         Enable trace output. Add line numbers to verbose logs.\n");
}

/*
 * Extract Displacement from bitstream
 *
 * Returns number of bytes consumed, so the caller
 * can skip ahead to process the next unprocessed bytes
 * in the buffer
 */
int extract_displacement(uint8_t *ptr, int bit_w, int16_t *displacement, int prev_consumed_bytes) {
    int consumed_bytes = 0;
    uint8_t displacement_low = 0;
    uint8_t displacement_high = 0;

    // consume displacement_low byte
    ptr++;
    consumed_bytes++;
    prev_consumed_bytes++;
    displacement_low = *ptr;
    

    if (bit_w == 0x0) {
        // 8 bit displacement check if we need to do 8bit to 16bit signed extension
        if ((*ptr>>7) == 0x1 ) {
            // MSB bit is set, then do signed extension
            *displacement = (0xFF<<8) | *ptr;
            LOG("; [0x%02x] %s Byte - Displacement[0x%x] signed extended to [%d]\n", *ptr, byte_count_str[prev_consumed_bytes], *ptr, *displacement);
        } else {
            *displacement = *ptr;
            LOG("; [0x%02x] %s Byte - Displacement[0x%x][%d]\n", *ptr,  byte_count_str[prev_consumed_bytes], *displacement, *displacement);
        }
    } else {
        // 16bit displacement
        LOG("; [0x%02x] %s Byte - Displacement Low[0x%02x] \n", *ptr, byte_count_str[prev_consumed_bytes], displacement_low);
        // consume displacement_high byte
        ptr++;
        consumed_bytes++;
        prev_consumed_bytes++;
        displacement_high = *ptr;
        LOG("; [0x%02x] %s Byte - Displacement High[0x%02x]\n", *ptr, byte_count_str[prev_consumed_bytes], displacement_high);
        *displacement = (displacement_high << 8) | displacement_low;
        LOG(";        Displacement [0x%04x][%d]\n", *displacement, *displacement);
    }

    return consumed_bytes;
}

/*
 * Extract Displacement from bitstream
 *
 * Returns number of bytes consumed, so the caller
 * can skip ahead to process the next unprocessed bytes
 * in the buffer
 */
int extract_data(uint8_t *ptr, int bit_w, int16_t *data, int prev_consumed_bytes) {
    int consumed_bytes = 0;
    uint8_t data_low = 0;
    uint8_t data_high = 0;

    // consume data_low byte
    ptr++;
    consumed_bytes++;
    prev_consumed_bytes++;
    data_low = *ptr;

    if (bit_w == 0x0) {
        // 8 bit data check if we need to do 8bit to 16bit signed extension
        if ((*ptr>>7) == 0x1 ) {
            // MSB bit is set, then do signed extension
            *data = (0xFF<<8) | *ptr;
            LOG("; [0x%02x] %s Byte - Data[0x%x] signed extended to [%d]\n", *ptr, byte_count_str[prev_consumed_bytes], *ptr, *data);
        } else {
            *data = *ptr;
            LOG("; [0x%02x] %s Byte - Data[0x%x][%d]\n", *ptr,  byte_count_str[prev_consumed_bytes], *data, *data);
        }
    } else {
        // 16 bit data
        LOG("; [0x%02x] %s Byte - data_low[0x%02x] \n", *ptr, byte_count_str[prev_consumed_bytes], data_low);
        // consume data_high byte
        ptr++;
        consumed_bytes++;
        prev_consumed_bytes++;
        data_high = *ptr;
        LOG("; [0x%02x] %s Byte - data_high[0x%02x]\n", *ptr, byte_count_str[prev_consumed_bytes], data_high);
        *data = (data_high << 8) | data_low;
        LOG(";        Data [0x%04x][%d]\n", *data, *data);
    }

    return consumed_bytes;
}


/*
 * Extract Displacement from bitstream
 *
 * Returns number of bytes consumed, so the caller
 * can skip ahead to process the next unprocessed bytes
 * in the buffer
 */
int explicit_extract_data(uint8_t *ptr, int bit_s, int bit_w, int16_t *data, int prev_consumed_bytes) {
    int consumed_bytes = 0;
    uint8_t op = (bit_s<<1) | bit_w;
    uint8_t data_low = 0;
    uint8_t data_high = 0;

    // consume data_low byte
    ptr++;
    consumed_bytes++;
    prev_consumed_bytes++;
    data_low = *ptr;

    // Explicit Extact of Data according to bits S and W
    // s w
    // 0 0  8 bit no sign extension
    // 0 1  16 bit data
    // 1 0  sign extend to 16 bit
    // 1 1  sign extend to 16 bit
    switch (op) {
        case 0x0:
            // 8 bit no sign extension
            *data = *ptr;
            LOG("; [0x%02x] %s Byte - Data[0x%x][%d]\n", *ptr,  byte_count_str[prev_consumed_bytes], *data, *data);
            break;
        case 0x1:
            // 16 bit data
            LOG("; [0x%02x] %s Byte - data_low[0x%02x] \n", *ptr, byte_count_str[prev_consumed_bytes], data_low);
            // consume data_high byte
            ptr++;
            consumed_bytes++;
            prev_consumed_bytes++;
            data_high = *ptr;
            LOG("; [0x%02x] %s Byte - data_high[0x%02x]\n", *ptr, byte_count_str[prev_consumed_bytes], data_high);
            *data = (data_high << 8) | data_low;
            LOG(";        Data [0x%04x][%d]\n", *data, *data);
            break;
        case 0x2:
        case 0x3:
            // sign extend to 16 bit
            // 8 bit data check if we need to do 8bit to 16bit signed extension
            if ((*ptr>>7) == 0x1 ) {
                // MSB bit is set, then do signed extension
                *data = (0xFF<<8) | *ptr;
                LOG("; [0x%02x] %s Byte - Data[0x%x] signed extended to [%d]\n", *ptr, byte_count_str[prev_consumed_bytes], *ptr, *data);
            } else {
                *data = *ptr;
                LOG("; [0x%02x] %s Byte - Data[0x%x][%d]\n", *ptr,  byte_count_str[prev_consumed_bytes], *data, *data);
            }
            break;
    }
    return consumed_bytes;
}

/*
 * Process a Accumulator MOV
 *
 * Returns number of bytes consumed, so the caller
 * can skip ahead to process the next unprocessed bytes
 * in the buffer
 */
int process_mov_accumulator_inst(uint8_t *ptr, int op_type) {
    uint8_t address_low = 0;
    uint8_t address_high = 0;
    int16_t address = 0;
    int consumed_bytes = 1;
    uint8_t bit_w = (*ptr & 0x1);

    LOG("; [0x%02x] %s Byte - Found ACCUMULATOR MOV bitstream | W[%d]\n", *ptr, byte_count_str[consumed_bytes], bit_w);

    // consume address_low byte
    ptr++;
    consumed_bytes++;
    address_low = *ptr;
    LOG("; [0x%02x] %s Byte - address_low[0x%02x]\n", *ptr, byte_count_str[consumed_bytes], address_low);
    if (bit_w == 1) {
        // consume address_high byte
        ptr++;
        consumed_bytes++;
        address_high = *ptr;
        LOG("; [0x%02x] %s Byte - address_high[0x%02x]\n", *ptr, byte_count_str[consumed_bytes], address_high);
    }
    address = (address_high << 8) | address_low;
    LOG("; Address [0x%04x][%d]\n", address, address);

    if (op_type == MOV_MEMORY_TO_ACCUMULATOR) {
        printf("mov ax, [ %d ]\n", address);
    } else {
        // MOV_ACCUMULATOR_TO_MEMORY
        printf("mov [ %d ], ax\n", address);
    }

    return consumed_bytes;
}

/*
 * Process a MOV immediate to register
 *
 * Returns number of bytes consumed, so the caller
 * can skip ahead to process the next unprocessed bytes
 * in the buffer
 */
int process_mov_immediate_to_register_memory(uint8_t *ptr) {
    int16_t data = 0;
    int16_t displacement = 0;
    int consumed = 0;
    int consumed_bytes = 1;
    uint8_t bit_w = *ptr & 0x1;
    char *data_type = "byte";
    char *destination = NULL;

    LOG("; [0x%02x] %s Byte - Found IMMEDIATE TO REG/MEM MOV bitstream | W[%d]\n", *ptr, byte_count_str[consumed_bytes], bit_w);

    // consume 2nd byte
    ptr++;
    consumed_bytes++;
    uint8_t mod = (*ptr >> 6) & 0x3;
    uint8_t r_m = (*ptr >> 0) & 0x7;
    
    if (mod == 0x3) {
        destination = register_map[r_m][bit_w].name;
    } else {
        destination = mov_source_effective_address[r_m];
    }
    LOG("; [0x%02x] %s Byte - MOD[0x%x] R/M[0x%x] destination[%s]\n", *ptr, byte_count_str[consumed_bytes], mod, r_m, destination);

    switch (mod) {
        case 0x0:
            // Memory Mode, No Displacement
            // * except when R/M is 110 then 16bit displacement follows
            if (r_m == 0x6) {
                // special case R/M is 110
                // 16 bit displacement follows
                consumed = extract_displacement(ptr, bit_w, &displacement, consumed_bytes);
                consumed_bytes += consumed;
                ptr += consumed;
            }

            // extract data
            consumed = extract_data(ptr, bit_w, &data, consumed_bytes);
            consumed_bytes += consumed;
            ptr += consumed;

            if (displacement == 0) {
                printf("mov [%s], %s %d\n", destination, data_type, data);
            } else {
                printf("mov [%s + %d], %s %d\n", destination, displacement, data_type, data);
            }
            break;

        case 0x1:
            // Memory Mode, 8bit displacement follows

            // extract displacement
            consumed = extract_displacement(ptr, bit_w, &displacement, consumed_bytes);
            consumed_bytes += consumed;
            ptr += consumed;

            // extract data
            consumed = extract_data(ptr, bit_w, &data, consumed_bytes);
            consumed_bytes += consumed;
            ptr += consumed;

            if (displacement == 0) {
                printf("mov [%s], %s %d\n", destination, data_type, data);
            } else {
                printf("mov [%s + %d], %s %d\n", destination, displacement, data_type, data);
            }
            break;

        case 0x2:
            // Memory Mode, 16bit displacement follows

            // extract displacement
            consumed = extract_displacement(ptr, bit_w, &displacement, consumed_bytes);
            consumed_bytes += consumed;
            ptr += consumed;

            // extract data
            consumed = extract_data(ptr, bit_w, &data, consumed_bytes);
            consumed_bytes += consumed;
            ptr += consumed;

            if (bit_w == 0x1) {
                // 16 bit data
                data_type = "word";
            }

            if (displacement == 0) {
                printf("mov [%s], %s %d\n", destination, data_type, data);
            } else {
                printf("mov [%s + %d], %s %d\n", destination, displacement, data_type, data);
            }
            break;

        case 0x3:
            // Register Mode, No Displacement

            // extract data
            consumed = extract_data(ptr, bit_w, &data, consumed_bytes);
            consumed_bytes += consumed;
            ptr += consumed;

            if (bit_w == 0x1) {
                // 16 bit data
                data_type = "word";
            }

            if (bit_w == 0x1) {
                // 16 bit data
                data_type = "word";
            }

            printf("mov [%s], %s %d\n", destination, data_type, data);
            break;
    }

    return consumed_bytes;
}

/*
 * Process a MOV immediate to register
 *
 * Returns number of bytes consumed, so the caller
 * can skip ahead to process the next unprocessed bytes
 * in the buffer
 */
int process_mov_immediate_to_register(uint8_t *ptr) {
    int16_t data = 0;
    int consumed = 0;
    int consumed_bytes = 1;
    uint8_t bit_w = (*ptr>>3) & 0x1;
    uint8_t reg = (*ptr & 0x7);
    char *reg_str = register_map[reg][bit_w].name;

    LOG("; [0x%02x] %s Byte - Found IMMEDIATE TO REG MOV bitstream | W[%d] REG[0x%x][%s]\n", *ptr, byte_count_str[consumed_bytes], bit_w, reg, reg_str);

    // extract data
    consumed = extract_data(ptr, bit_w, &data, consumed_bytes);
    consumed_bytes += consumed;
    ptr += consumed;

    struct reg_definition_s item = register_map[reg][bit_w];
    uint16_t val_before = registers[item.reg];
    write_register(item, data);

    if (trace) {
        printf("[%s:%d] mov %s,%d ; %s:0x%04x->0x%04x\n", __FUNCTION__, __LINE__, reg_str, data, item.name, val_before, registers[item.reg]);
    } else {
        printf("mov %s,%d ; %s:0x%04x->0x%04x\n", reg_str, data, item.name, val_before, registers[item.reg]);
    }
    return consumed_bytes;
}

/*
 * Process a MOV register/memory to/from register
 *
 * Returns number of bytes consumed, so the caller
 * can skip ahead to process the next unprocessed bytes
 * in the buffer
 */
int process_regmem_tpo_reg_op_inst(uint8_t *ptr, int op_type) {
    uint8_t bit_d = 0;
    uint8_t bit_w = 0;
    char *destination = NULL;
    char *source = NULL;
    int consumed = 0;
    int consumed_bytes = 1;
    int16_t displacement = 0;
    char *op_type_str = NULL;
    char *op = NULL;
    uint8_t mod = 0;
    uint8_t reg = 0;
    uint8_t r_m = 0;
    uint8_t sr = 0;
    bool is_segment_op = false;

    switch (op_type) {
        case MOV_REGMEM_REG_INSTRUCTION:
            op_type_str = "MOV_REGMEM_REG_INSTRUCTION";
            op = "mov";
            break;

        case MOV_REGMEM_TO_SEGMENT_INSTRUCTION:
            op_type_str = "MOV_REGMEM_TO_SEGMENT_INSTRUCTION";
            op = "mov";
            is_segment_op = true;
            break;

        case MOV_SEGMENT_TO_REGMEM_INSTRUCTION:
            op_type_str = "MOV_SEGMENT_TO_REGMEM_INSTRUCTION";
            op = "mov";
            is_segment_op = true;
            break;

        default:
            ERROR("[%s:%d] ERROR: Bad OP[0x%x]\n", __FUNCTION__, __LINE__, op_type);
    }

    if (is_segment_op) {
        LOG("; [0x%02x] %s Byte - Found %s bitstream\n", *ptr, byte_count_str[consumed_bytes], op_type_str);
    } else {
        bit_d = (*ptr & 0x2)>>1;
        bit_w = *ptr & 0x1;
        LOG("; [0x%02x] %s Byte - Found %s bitstream | D[%d] W[%d]\n", *ptr, byte_count_str[consumed_bytes], op_type_str, bit_d, bit_w);
    }

    // consume 2nd byte
    ptr++;
    mod = (*ptr >> 6) & 0x3;
    if (is_segment_op) {
        sr = (*ptr >> 3) & 0x3;
    } else {
        reg = (*ptr >> 3) & 0x7;
    }
    r_m = (*ptr >> 0) & 0x7;
    consumed_bytes++;
    if (is_segment_op) {
        LOG("; [0x%02x] 2nd Byte - MOD[0x%x] SR[0x%x] R/M[0x%x]\n", *ptr, mod, sr, r_m);
    } else {
        LOG("; [0x%02x] 2nd Byte - MOD[0x%x] REG[0x%x] R/M[0x%x]\n", *ptr, mod, reg, r_m);
    }

    switch (mod) {
        case 0x0:
            // Memory Mode, No Displacement
            // * except when R/M is 110 then 16bit displacement follows
            if (r_m == 0x6) {
                // special case R/M is 110
                // 16 bit displacement follows
                // extract displacement
                consumed = extract_displacement(ptr, bit_w, &displacement, consumed_bytes);
                consumed_bytes += consumed;
                ptr += consumed;
                // destination specificed in REG field
                if (is_segment_op) {
                    destination = segment_register_map[sr];
                } else {
                    destination = register_map[reg][bit_w].name;
                }
                // source is the direct address
                OUTPUT("%s %s, [%d]\n", op, destination, displacement);
            } else {
                if (is_segment_op) {
                    if (op_type == MOV_REGMEM_TO_SEGMENT_INSTRUCTION) {
                        destination = segment_register_map[sr];
                        source = mov_source_effective_address[r_m];
                        OUTPUT("%s %s, [%s]\n", op, destination, source);
                    } else {
                        // MOV_SEGMENT_TO_REGMEM_INSTRUCTION
                        source = segment_register_map[sr];
                        destination = mov_source_effective_address[r_m];
                        OUTPUT("%s [%s], %s\n", op, destination, source);
                    }
                } else {
                    if (bit_d == 0x1) {
                        // destination specificed in REG field
                        destination = register_map[reg][bit_w].name;
                        // source specified in effective address table
                        source = mov_source_effective_address[r_m];
                        OUTPUT("%s %s, [%s]\n", op, destination, source);
                    } else {
                        // destination specificed in R/M field
                        destination = mov_source_effective_address[r_m];
                        // source specified in REG field
                        source = register_map[reg][bit_w].name;
                        OUTPUT("%s [%s], %s\n", op, destination, source);
                    }
                }
            }
            break;
        case 0x1:
            // Memory Mode, 8bit displacement follows
            // extract displacement
            // force bit_w to extract displacement as it is 8bits
            consumed = extract_displacement(ptr, 0, &displacement, consumed_bytes);
            consumed_bytes += consumed;
            ptr += consumed;
            if (is_segment_op) {
                if (op_type == MOV_REGMEM_TO_SEGMENT_INSTRUCTION) {
                    destination = segment_register_map[sr];
                    source = register_map[r_m][1].name;
                    if (displacement==0) {
                        // don't output " + 0" in the effective address calculation
                        OUTPUT("%s %s, [%s]\n", op, destination, source);
                    } else {
                        OUTPUT("%s %s, [%s + %d]\n", op, destination, source, displacement);
                    }
                } else {
                    // MOV_SEGMENT_TO_REGMEM_INSTRUCTION
                    source = segment_register_map[sr];
                    destination = register_map[r_m][1].name;
                    if (displacement==0) {
                        // don't output " + 0" in the effective address calculation
                        OUTPUT("%s [%s], %s\n", op, destination, source);
                    } else {
                        OUTPUT("%s [%s + %d], %s\n", op, destination, displacement, source);
                    }
                }
            } else {
                if (bit_d == 0x1) {
                    // destination specificed in REG field
                    destination = register_map[reg][bit_w].name;
                    // source specified in effective address table
                    source = mov_source_effective_address[r_m];
                    if (displacement==0) {
                        // don't output " + 0" in the effective address calculation
                        OUTPUT("%s %s, [%s]\n", op, destination, source);
                    } else {
                        OUTPUT("%s %s, [%s + %d]\n", op, destination, source, displacement);
                    }
                } else {
                    // destination specificed in R/M field
                    destination = mov_source_effective_address[r_m];
                    // source specified in REG field
                    source = register_map[reg][bit_w].name;
                    if (displacement==0) {
                        // don't output " + 0" in the effective address calculation
                        OUTPUT("%s [%s], %s\n", op, destination, source);
                    } else {
                        OUTPUT("%s [%s + %d], %s\n", op, destination, displacement, source);
                    }
                }
            }
            break;
        case 0x2:
            // Memory Mode, 16bit displacement follows
            // force extract 16bit displacement
            consumed = extract_displacement(ptr, 1, &displacement, consumed_bytes);
            consumed_bytes += consumed;
            ptr += consumed;

            if (is_segment_op) {
                if (op_type == MOV_REGMEM_TO_SEGMENT_INSTRUCTION) {
                    destination = segment_register_map[sr];
                    source = register_map[r_m][1].name;
                    if (displacement==0) {
                        // don't output " + 0" in the effective address calculation
                        OUTPUT("%s %s, [%s]\n", op, destination, source);
                    } else {
                        OUTPUT("%s %s, [%s + %d]\n", op, destination, source, displacement);
                    }
                } else {
                    // MOV_SEGMENT_TO_REGMEM_INSTRUCTION
                    source = segment_register_map[sr];
                    destination = register_map[r_m][1].name;
                    if (displacement==0) {
                        // don't output " + 0" in the effective address calculation
                        OUTPUT("%s [%s], %s\n", op, destination, source);
                    } else {
                        OUTPUT("%s [%s + %d], %s\n", op, destination, displacement, source);
                    }
                }
            } else {
                if (bit_d == 0x1) {
                    // destination specificed in REG field
                    destination = register_map[reg][bit_w].name;
                    // source specified in effective address table
                    source = mov_source_effective_address[r_m];
                    if (displacement==0) {
                        // don't output " + 0" in the effective address calculation
                        OUTPUT("%s %s, [%s]\n", op, destination, source);
                    } else {
                        OUTPUT("%s %s, [%s + %d]\n", op, destination, source, displacement);
                    }
                } else {
                    // destination specificed in R/M field
                    destination = mov_source_effective_address[r_m];
                    // source specified in REG field
                    source = register_map[reg][bit_w].name;
                    if (displacement==0) {
                        // don't output " + 0" in the effective address calculation
                        OUTPUT("%s [%s], %s\n", op, destination, source);
                    } else {
                        OUTPUT("%s [%s + %d], %s\n", op, destination, displacement, source);
                    }
                }
            }
            break;
        case 0x3:
            // Register Mode, No Displacement
            // get destination/source
            struct reg_definition_s dst_item;
            struct reg_definition_s src_item;
            uint16_t src_val;
            uint16_t dst_val_before;
            if (is_segment_op) {
                if (op_type == MOV_REGMEM_TO_SEGMENT_INSTRUCTION) {
                    DUMP_SEGMENT_REGS();
                    destination = segment_register_map[sr];
                    source = register_map[r_m][1].name;
                    // update registers
                    dst_val_before = segment_registers[sr];
                    src_val = registers[register_map[r_m][1].reg] ;
                    segment_registers[sr] = src_val;
                    printf("%s %s,%s ; %s:0x%04x->0x%04x \n", op, destination, source, destination, dst_val_before, segment_registers[sr]);
                    DUMP_SEGMENT_REGS();
                } else {
                    // MOV_SEGMENT_TO_REGMEM_INSTRUCTION
                    DUMP_SEGMENT_REGS();
                    source = segment_register_map[sr];
                    destination = register_map[r_m][1].name;
                    // update registers
                    dst_val_before = registers[register_map[r_m][1].reg];
                    src_val = segment_registers[sr];
                    registers[register_map[r_m][1].reg] = src_val;
                    printf("%s %s,%s ; %s:0x%04x->0x%04x \n", op, destination, source, destination, dst_val_before, registers[register_map[r_m][1].reg]);
                    DUMP_SEGMENT_REGS();
                }
            } else {
                if (bit_d == 0x1) {
                    // destination specificed in REG field
                    dst_item = register_map[reg][bit_w];
                    // source specificed in R/M field
                    src_item = register_map[r_m][bit_w];
                } else {
                    // destination specificed in R/M field
                    dst_item = register_map[r_m][bit_w];
                    // source specificed in REG field
                    src_item = register_map[reg][bit_w];
                }
                // update registers
                dst_val_before = registers[dst_item.reg];
                src_val = read_register(src_item);
                write_register(dst_item, src_val);
                if (trace) {
                    printf("[%s:%d] %s %s,%s ; %s:0x%04x->0x%04x\n", __FUNCTION__, __LINE__, op, dst_item.name, src_item.name, dst_item.name, dst_val_before, registers[dst_item.reg]);
                } else {
                    printf("%s %s,%s ; %s:0x%04x->0x%04x\n", op, dst_item.name, src_item.name, dst_item.name, dst_val_before, registers[dst_item.reg]);
                }
                }
            break;
    }
    return consumed_bytes;
}

int main (int argc, char *argv[]) {
    int opt;
    char *input_file = NULL;
    uint8_t *memory = NULL;
    size_t bytes_available;

    while( (opt = getopt(argc, argv, "hi:vt")) != -1) {
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

            case 't':
                trace = true;
                break;

            default:
                fprintf(stderr, "ERROR Invalid command line option\n");
                usage();
                exit(1);
                break;
        }
    }

    LOG("==========================\n");
    LOG("8086 Instruction Simulator\n");
    LOG("==========================\n");

    if (!input_file) {
        fprintf(stderr, "ERROR Missing Input File\n");
        usage();
        exit(1);
    }

    LOG("Using Input Filename     [%s]\n", input_file);
    LOG("\n\n");

    memory = malloc(MEMORY_SIZE);
    if (!memory) {
        fprintf(stderr, "ERROR malloc failed\n");
        exit(1);
    }
    
    in_fp = fopen(input_file, "r");
    if (!in_fp) {
        fprintf(stderr, "ERROR input_file fopen failed [%d][%s]\n", errno, strerror(errno));
        exit(1);
    }
    LOG("Opened Input file[%s] OK\n", input_file);

    // load up to 1MB of ops from file
    // if there is extra data we don't load it
    bytes_available = fread(memory, 1, MEMORY_SIZE, in_fp);
    fclose(in_fp);
    LOG("; Read [%zu] bytes from file\n\n", bytes_available);

    LOG("Starting Simulation:\n");
    LOG("--------------------\n");

    uint8_t *ptr = memory;

    // 8086 instructions can be encoded from 1 to 6 bytes
    // so we inspect on a per byte basis
    while (bytes_available>0) {
        int consumed_bytes = 0;
        if ((*ptr>>1) == MOV_IMMEDIATE_TO_REGMEM_INSTRUCTION) {
            // found immediate to register/memory
            consumed_bytes = process_mov_immediate_to_register_memory(ptr);
        } else if ((*ptr>>4) == MOV_IMMEDIATE_TO_REG_INSTRUCTION) {
            // found immediate to register move
            consumed_bytes = process_mov_immediate_to_register(ptr);
        } else if ( (*ptr>>2) == MOV_REGMEM_REG_INSTRUCTION) {
            // found move register/memory to/from register
            consumed_bytes = process_regmem_tpo_reg_op_inst(ptr, MOV_REGMEM_REG_INSTRUCTION);
        } else if ((*ptr>>1) == MOV_MEMORY_TO_ACCUMULATOR) {
            // found memory to accumulator move
            consumed_bytes = process_mov_accumulator_inst(ptr, MOV_MEMORY_TO_ACCUMULATOR);
        } else if ((*ptr>>1) == MOV_ACCUMULATOR_TO_MEMORY) {
            // found memory to accumulator move
            consumed_bytes = process_mov_accumulator_inst(ptr, MOV_ACCUMULATOR_TO_MEMORY);
        } else if (*ptr == MOV_REGMEM_TO_SEGMENT_INSTRUCTION) {
            // found move register/memory to/from segment register
            consumed_bytes = process_regmem_tpo_reg_op_inst(ptr, MOV_REGMEM_TO_SEGMENT_INSTRUCTION);
        } else if (*ptr == MOV_SEGMENT_TO_REGMEM_INSTRUCTION) {
            // found move register/memory to/from segment register
            consumed_bytes = process_regmem_tpo_reg_op_inst(ptr, MOV_SEGMENT_TO_REGMEM_INSTRUCTION);
        } else {
            ERROR("; [0x%02x] not a recognized instruction, aborting...\n", *ptr);
        }
        LOG("\n");
        ptr+=consumed_bytes;
        bytes_available -= consumed_bytes;
    }

    printf("\n");
    printf("Registers:\n");
    printf("----------\n");
    printf("\tax: 0x%04x (%d)\n", registers[AX], registers[AX]);
    printf("\tbx: 0x%04x (%d)\n", registers[BX], registers[BX]);
    printf("\tcx: 0x%04x (%d)\n", registers[CX], registers[CX]);
    printf("\tdx: 0x%04x (%d)\n", registers[DX], registers[DX]);
    printf("\tsp: 0x%04x (%d)\n", registers[SP], registers[SP]);
    printf("\tbp: 0x%04x (%d)\n", registers[BP], registers[BP]);
    printf("\tsi: 0x%04x (%d)\n", registers[SI], registers[SI]);
    printf("\tdi: 0x%04x (%d)\n", registers[DI], registers[DI]);
    printf("\n");
    printf("Segment Registers:\n");
    printf("------------------\n");
    for (int i=0; i<4; i++) {
        printf("\t%s: 0x%04x (%d)\n", segment_register_map[i], segment_registers[i], segment_registers[i]);
    }
    printf("\n\n");

    return 0;
}
