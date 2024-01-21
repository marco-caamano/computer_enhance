#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>

#include "libdecoder.h"

/* 8086 can address up to 1MB of Memory, but since we 
 * are not going to use the segment registers we will
 * limit to the 16bit address space of 64KB addressable
 * memory
 */
#define MEMORY_SIZE 64*1024

#define STR_BUFFER_SIZE     512

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

bool verbose = false;
bool trace = false;

uint8_t memory[MEMORY_SIZE] = {0};

uint16_t registers[MAX_REG] = { 0 };

uint16_t segment_registers[MAX_SEG_REG] = { 0 };


struct reg_definition_s {
    enum register_e reg;
    char *name;
    uint16_t mask;
    uint8_t shift;
};

#define REG_DEF_AX { REG_AX, "ax", 0xFFFF, 0 }
#define REG_DEF_AL { REG_AX, "al", 0x00FF, 0 }
#define REG_DEF_AH { REG_AX, "ah", 0xFF00, 8 }
#define REG_DEF_BX { REG_BX, "bx", 0xFFFF, 0 }
#define REG_DEF_BL { REG_BX, "bl", 0x00FF, 0 }
#define REG_DEF_BH { REG_BX, "bh", 0xFF00, 8 }
#define REG_DEF_CX { REG_CX, "cx", 0xFFFF, 0 }
#define REG_DEF_CL { REG_CX, "cl", 0x00FF, 0 }
#define REG_DEF_CH { REG_CX, "ch", 0xFF00, 8 }
#define REG_DEF_DX { REG_DX, "dx", 0xFFFF, 0 }
#define REG_DEF_DL { REG_DX, "dl", 0x00FF, 0 }
#define REG_DEF_DH { REG_DX, "dh", 0xFF00, 8 }
#define REG_DEF_SP { REG_SP, "sp", 0xFFFF, 0 }
#define REG_DEF_BP { REG_BP, "bp", 0xFFFF, 0 }
#define REG_DEF_SI { REG_SI, "si", 0xFFFF, 0 }
#define REG_DEF_DI { REG_DI, "di", 0xFFFF, 0 }

struct reg_definition_s reg_item_map[MAX_REG] = {
    REG_DEF_AX,
    REG_DEF_AL,
    REG_DEF_AH,
    REG_DEF_BX,
    REG_DEF_BL,
    REG_DEF_BH,
    REG_DEF_CX,
    REG_DEF_CL,
    REG_DEF_CH,
    REG_DEF_DX,
    REG_DEF_DL,
    REG_DEF_DH,
    REG_DEF_SP,
    REG_DEF_BP,
    REG_DEF_SI,
    REG_DEF_DI,
};

/* Instruction Pointer */
uint16_t ip = 0;

/*
 * FLAGS
 * ZF   Zero Flag
 * PF   Parity Flag
 * SF   Signed Flag
 * OF   Overflow Flag
 * CF   Carry Flag
 * AF   Auxiliary Carry Flag
 */
#define ZERO_FLAG_MASK          0x40
#define ZERO_FLAG_SHIFT         6
#define SIGNED_FLAG_MASK        0x80
#define SIGNED_FLAG_SHIFT       7
#define ZERO_AND_SIGNED_MASK    0xC0

#define REGISTER_SIGNED_MASK    0x8000

uint16_t flags = 0;

#define IS_Z_SET()  (((flags & ZERO_FLAG_MASK)>>ZERO_FLAG_SHIFT)==0x1)
#define IS_S_SET()  (((flags & SIGNED_FLAG_MASK)>>SIGNED_FLAG_SHIFT)==0x1)

void eval_flags(uint16_t value) {
    if (value==0) {
        // set zero flag
        flags |= (1<<ZERO_FLAG_SHIFT);
    } else {
        // clear zero flag
        flags &= ~ZERO_FLAG_MASK;
    }
    if ((value & REGISTER_SIGNED_MASK) != 0) {
        // set signed flag
        flags |= (1<<SIGNED_FLAG_SHIFT);
    } else {
        // clear signed flag
        flags &= ~SIGNED_FLAG_MASK;
    }
}

char *get_flags(uint16_t value) {
    // for now just shift down flags and eval 2bits
    uint8_t flags = (value & ZERO_AND_SIGNED_MASK) >> ZERO_FLAG_SHIFT;
    switch (flags) {
        case 0x0:
            return "";
        case 0x1:
            return "Z";
        case 0x2:
            return "S";
        case 0x3:
            return "SZ";
        default:
            ERROR("Invalid Flag\n");
            break;
    }
}

void dump_nonzoer_memory(void) {
    // only dump when non-zero
    for (int offset=0; offset<(MEMORY_SIZE/sizeof(uint16_t)); offset+=2) {
        if (memory[offset]!=0) {
            printf("[ %d ] = 0x%04x (%d)\n", offset, memory[offset], memory[offset]);
        }
    }
}


uint16_t read_register(struct reg_definition_s item) {
    uint16_t mask = item.mask;
    uint8_t shift = item.shift;
    return (registers[item.reg] & mask) >> shift;
}

uint16_t write_register(struct reg_definition_s dst_item, uint16_t value) {
    uint16_t mask = dst_item.mask;
    uint8_t shift = dst_item.shift;
    uint16_t data = registers[dst_item.reg] & ~mask;
    data |= (value<<shift) & mask;
    registers[dst_item.reg] = data;
    return registers[dst_item.reg];
}

void parse_sub_cmp_inst(struct decoded_instruction_s *inst, bool is_sub) {
    uint16_t dst_val;
    uint16_t src_val;
    uint16_t value;

    switch (inst->dst_type) {
        case TYPE_REGISTER:
            struct reg_definition_s dst_reg = reg_item_map[inst->dst_register];
            switch (inst->src_type) {
                case TYPE_REGISTER:
                    struct reg_definition_s src_reg = reg_item_map[inst->src_register];
                    dst_val = read_register(dst_reg);
                    src_val = read_register(src_reg);
                    value = dst_val - src_val;
                    eval_flags(value);
                    if (is_sub) {
                        write_register(dst_reg, value);
                        printf("%s %s, %s \t\t; %s:0x%04x -> 0x%04x \t", 
                            inst->op_name, 
                            get_register_name(inst->dst_register), 
                            get_register_name(inst->src_register), 
                            get_register_name(inst->dst_register), 
                            dst_val,
                            value);
                    } else {
                        printf("%s %s, %s \t\t;\t\t\t ", 
                            inst->op_name, 
                            get_register_name(inst->dst_register), 
                            get_register_name(inst->src_register));
                    }
                    break;
                case TYPE_DATA:
                    dst_val = read_register(dst_reg);
                    src_val = inst->src_data;
                    value = dst_val - src_val;
                    eval_flags(value);
                    if (is_sub) {
                        write_register(dst_reg, value);
                        printf("%s %s, %d \t\t; %s:0x%04x -> 0x%04x \t", 
                            inst->op_name, 
                            get_register_name(inst->dst_register), 
                            src_val,
                            get_register_name(inst->dst_register), 
                            dst_val,
                            value);
                    } else {
                        printf("%s %s, %d \t\t; \t\t\t", 
                            inst->op_name, 
                            get_register_name(inst->dst_register), 
                            src_val);
                    }
                    break;
                default:
                    ERROR("Unhandled src_type[%d]\n", inst->src_type);
                    break;
            }
            break;
        default:
            ERROR("Unhandled dst_type[%d]\n", inst->dst_type);
            break;
    }
    // advance IP register
    ip += inst->inst_num_bytes;
}

void parse_add_inst(struct decoded_instruction_s *inst) {
    uint16_t dst_val;
    uint16_t src_val;
    uint16_t value;

    switch (inst->dst_type) {
        case TYPE_REGISTER:
            struct reg_definition_s dst_reg = reg_item_map[inst->dst_register];
            dst_val = read_register(dst_reg);
            switch (inst->src_type) {
                case TYPE_DATA:
                    src_val = inst->src_data;
                    value = dst_val + src_val;
                    eval_flags(value);
                    write_register(dst_reg, value);
                    printf("%s %s, %d \t\t; %s:0x%04x -> 0x%04x\t", 
                        inst->op_name, 
                        get_register_name(inst->dst_register), 
                        src_val,
                        get_register_name(inst->dst_register), 
                        dst_val,
                        value);
                    break;
                case TYPE_REGISTER:
                    src_val = read_register(reg_item_map[inst->src_register]);
                    value = dst_val + src_val;
                    eval_flags(value);
                    write_register(dst_reg, value);
                    printf("%s %s, %s \t\t; %s:0x%04x -> 0x%04x\t", 
                        inst->op_name,
                        get_register_name(inst->dst_register),
                        get_register_name(inst->src_register),
                        get_register_name(inst->dst_register),
                        dst_val,
                        value);
                    break;
                case TYPE_EFFECTIVE_ADDRESS:
                    uint16_t src_addr = 0;
                    const char *src_reg1_str = NULL;
                    const char *src_reg2_str = NULL;
                    char output[STR_BUFFER_SIZE] = {0};
                    if (inst->src_effective_reg1 != MAX_REG) {
                        src_addr += read_register(reg_item_map[inst->src_effective_reg1]);
                        src_reg1_str = get_register_name(inst->src_effective_reg1);
                    }
                    if (inst->src_effective_reg2 != MAX_REG) {
                        src_addr += read_register(reg_item_map[inst->src_effective_reg2]);
                        src_reg2_str = get_register_name(inst->src_effective_reg2);
                    }
                    src_addr += inst->dst_effective_displacement;
                    value = dst_val + memory[src_addr];
                    write_register(dst_reg, value);
                    if (inst->src_effective_displacement!=0) {
                        snprintf((char *)&output, STR_BUFFER_SIZE, "%s %s, [%s%s%s%s%d] \t\t; \t\t\t", inst->op_name,
                            get_register_name(inst->dst_register),
                            src_reg1_str ? src_reg1_str : "",
                            src_reg2_str ? "+" : "",
                            src_reg2_str ? src_reg2_str : "",
                            (inst->src_effective_displacement>0) ? "+" : "",
                            inst->src_effective_displacement);
                    } else {
                        snprintf((char *)&output, STR_BUFFER_SIZE, "%s %s, [%s%s%s] \t; %s:0x%04x -> 0x%04x\t", inst->op_name,
                            get_register_name(inst->dst_register),
                            src_reg1_str ? src_reg1_str : "",
                            src_reg2_str ? "+" : "",
                            src_reg2_str ? src_reg2_str : "",
                            get_register_name(inst->dst_register),
                            dst_val, value);
                    }
                    printf("%s", output);
                    break;
                default:
                    ERROR("Unhandled src_type[%d]\n", inst->src_type);
                    break;
            }
            break;
        default:
            ERROR("Unhandled dst_type[%d]\n", inst->dst_type);
            break;
    }
    // advance IP register
    ip += inst->inst_num_bytes;
}

void parse_mov_inst(struct decoded_instruction_s *inst) {
    uint16_t dst_val;
    uint16_t value;

    switch (inst->dst_type) {
        case TYPE_REGISTER:
            struct reg_definition_s dst_reg = reg_item_map[inst->dst_register];
            dst_val = read_register(dst_reg);
            switch (inst->src_type) {
                case TYPE_REGISTER:
                    struct reg_definition_s src_reg = reg_item_map[inst->src_register];
                    value = read_register(src_reg);
                    write_register(dst_reg, value);
                    printf("%s %s, %s \t\t; %s:0x%04x -> 0x%04x \t", inst->op_name,
                        get_register_name(inst->dst_register), 
                        get_register_name(inst->src_register), 
                        get_register_name(inst->dst_register), 
                        dst_val, value);
                    break;
                case TYPE_SEGMENT_REGISTER:
                    value = segment_registers[inst->src_seg_register];
                    write_register(dst_reg, value);
                    printf("%s %s, %s \t; %s:0x%04x -> 0x%04x", inst->op_name, 
                        get_register_name(inst->dst_register), 
                        get_segment_register_name(inst->src_seg_register), 
                        get_register_name(inst->dst_register), 
                        dst_val, value);
                    break;
                case TYPE_DATA:
                    write_register(dst_reg, inst->src_data);
                    printf("%s %s, %d \t\t; %s:0x%04x -> 0x%04x\t", inst->op_name, 
                        get_register_name(inst->dst_register), 
                        inst->src_data, 
                        get_register_name(inst->dst_register), 
                        dst_val, inst->src_data&0xFFFF);
                    break;
                case TYPE_DIRECT_ADDRESS:
                    value = memory[inst->src_direct_address];
                    write_register(dst_reg, value);
                    printf("%s %s, [%d] \t\t; %s:0x%04x -> 0x%04x\t", inst->op_name, 
                        get_register_name(inst->dst_register), 
                        inst->src_direct_address, 
                        get_register_name(inst->dst_register), 
                        dst_val, value);
                    break;
                case TYPE_EFFECTIVE_ADDRESS:
                    uint16_t src_addr = 0;
                    const char *src_reg1_str = NULL;
                    const char *src_reg2_str = NULL;
                    char output[STR_BUFFER_SIZE] = {0};
                    if (inst->src_effective_reg1 != MAX_REG) {
                        src_addr += read_register(reg_item_map[inst->src_effective_reg1]);
                        src_reg1_str = get_register_name(inst->src_effective_reg1);
                    }
                    if (inst->src_effective_reg2 != MAX_REG) {
                        src_addr += read_register(reg_item_map[inst->src_effective_reg2]);
                        src_reg2_str = get_register_name(inst->src_effective_reg2);
                    }
                    src_addr += inst->dst_effective_displacement;
                    value = memory[src_addr];
                    write_register(dst_reg, value);
                    if (inst->src_effective_displacement!=0) {
                        snprintf((char *)&output, STR_BUFFER_SIZE, "%s %s, [%s%s%s%s%d] \t\t; \t\t\t", inst->op_name,
                            get_register_name(inst->dst_register),
                            src_reg1_str ? src_reg1_str : "",
                            src_reg2_str ? "+" : "",
                            src_reg2_str ? src_reg2_str : "",
                            (inst->src_effective_displacement>0) ? "+" : "",
                            inst->src_effective_displacement);
                        
                    } else {
                        snprintf((char *)&output, STR_BUFFER_SIZE, "%s %s, [%s%s%s] \t; %s:0x%04x -> 0x%04x\t", inst->op_name,
                            get_register_name(inst->dst_register),
                            src_reg1_str ? src_reg1_str : "",
                            src_reg2_str ? "+" : "",
                            src_reg2_str ? src_reg2_str : "",
                            get_register_name(inst->dst_register),
                            dst_val, value);
                    }
                    printf("%s", output);
                    break;
                default:
                    ERROR("Unhandled src_type[%d]\n", inst->src_type);
                    break;
            }
            break;
        case TYPE_SEGMENT_REGISTER:
            switch (inst->src_type) {
                case TYPE_REGISTER:
                    struct reg_definition_s src_reg = reg_item_map[inst->src_register];
                    dst_val = segment_registers[inst->dst_seg_register];
                    segment_registers[inst->dst_seg_register] = read_register(src_reg);
                    printf("%s %s, %s \t; %s:0x%04x -> 0x%04x", inst->op_name,
                        get_segment_register_name(inst->dst_seg_register),
                        get_register_name(inst->src_register),
                        get_segment_register_name(inst->dst_seg_register),
                        dst_val, read_register(src_reg));
                    break;
                case TYPE_SEGMENT_REGISTER:
                    dst_val = segment_registers[inst->dst_seg_register];
                    segment_registers[inst->dst_seg_register] = segment_registers[inst->src_seg_register];
                    printf("%s %s, %s \t; %s:0x%04x -> 0x%04x", inst->op_name,
                        get_segment_register_name(inst->dst_seg_register),
                        get_segment_register_name(inst->src_seg_register),
                        get_segment_register_name(inst->dst_seg_register),
                        dst_val, segment_registers[inst->dst_seg_register]);
                    break;
                case TYPE_DATA:
                    dst_val = segment_registers[inst->dst_seg_register];
                    segment_registers[inst->dst_seg_register] = inst->src_data;
                    printf("%s %s, %d \t; %s:0x%04x -> 0x%04x", inst->op_name,
                        get_segment_register_name(inst->dst_seg_register),
                        inst->src_data,
                        get_segment_register_name(inst->dst_seg_register),
                        dst_val, inst->src_data);
                    break;
                default:
                    ERROR("Unhandled src_type[%d]\n", inst->src_type);
                    break;
            }
            break;
        case TYPE_DIRECT_ADDRESS:
            switch (inst->src_type) {
                case TYPE_DATA:
                    dst_val = memory[inst->dst_direct_address];
                    memory[inst->dst_direct_address] = inst->src_data;
                    printf("%s [%d], %d \t\t; [0x%04x]:0x%04x -> 0x%04x", inst->op_name,
                        inst->dst_direct_address,
                        inst->src_data,
                        inst->dst_direct_address,
                        dst_val, inst->src_data);
                    break;
                default:
                    ERROR("Unhandled src_type[%d]\n", inst->src_type);
                    break;
            }
            break;
        case TYPE_EFFECTIVE_ADDRESS:
            uint16_t dst_addr = 0;
            const char *dst_reg1_str = NULL;
            const char *dst_reg2_str = NULL;
            char output[STR_BUFFER_SIZE] = {0};
            if (inst->dst_effective_reg1 != MAX_REG) {
                dst_addr += read_register(reg_item_map[inst->dst_effective_reg1]);
                dst_reg1_str = get_register_name(inst->dst_effective_reg1);
            }
            if (inst->dst_effective_reg2 != MAX_REG) {
                dst_addr += read_register(reg_item_map[inst->dst_effective_reg2]);
                dst_reg2_str = get_register_name(inst->dst_effective_reg2);
            }
            dst_addr += inst->dst_effective_displacement;
            switch (inst->src_type) {
                case TYPE_REGISTER:
                    value = read_register(reg_item_map[inst->src_register]);
                    dst_val = memory[dst_addr];
                    memory[dst_addr] = value;
                    if (inst->dst_effective_displacement!=0) {
                        snprintf((char *)&output, STR_BUFFER_SIZE, "%s [%s%s%s%s%d], %s \t\t; [%d]:0x%04x -> 0x%04x",
                            inst->op_name,
                            dst_reg1_str ? dst_reg1_str : "",
                            dst_reg2_str ? "+" : "",
                            dst_reg2_str ? dst_reg2_str : "",
                            (inst->dst_effective_displacement>0) ? "+" : "",
                            inst->dst_effective_displacement,
                            get_register_name(inst->src_register),
                            dst_addr, dst_val, value);
                        
                    } else {
                        snprintf((char *)&output, STR_BUFFER_SIZE, "%s [%s%s%s], %s \t\t; [%d]:0x%04x -> 0x%04x",
                            inst->op_name,
                            dst_reg1_str ? dst_reg1_str : "",
                            dst_reg2_str ? "+" : "",
                            dst_reg2_str ? dst_reg2_str : "",
                            get_register_name(inst->src_register),
                            dst_addr, dst_val, value);
                    }
                    printf("%s", output);
                    break;
                case TYPE_DATA:
                    dst_val = memory[dst_addr];
                    memory[dst_addr] = inst->src_data;
                    if (inst->dst_effective_displacement!=0) {
                        snprintf((char *)&output, STR_BUFFER_SIZE, "%s [%s%s%s%s%d], %d \t\t; \t\t\t", inst->op_name,
                            dst_reg1_str ? dst_reg1_str : "",
                            dst_reg2_str ? "+" : "",
                            dst_reg2_str ? dst_reg2_str : "",
                            (inst->dst_effective_displacement>0) ? "+" : "",
                            inst->dst_effective_displacement, inst->src_data);
                        
                    } else {
                        snprintf((char *)&output, STR_BUFFER_SIZE, "%s [%s%s%s], %d \t\t; \t\t\t", inst->op_name,
                            dst_reg1_str ? dst_reg1_str : "",
                            dst_reg2_str ? "+" : "",
                            dst_reg2_str ? dst_reg2_str : "", inst->src_data);
                    }
                    printf("%s", output);
                    break;
                default:
                    ERROR("Unhandled src_type[%d]\n", inst->src_type);
                    break;
            }
            break;
        default:
            ERROR("Unhandled dst_type[%d]\n", inst->dst_type);
            break;
    }
    // advance IP register
    ip += inst->inst_num_bytes;
}

void parse_jmp_inst(struct decoded_instruction_s *inst) {
    // advance IP register
    ip += inst->inst_num_bytes;
    switch (inst->op) {
        case JNZ_INST:
            if (!IS_Z_SET()) {
                // jump on not zero
                ip += inst->src_data;
            }
            printf("%s %d \t\t;\t\t\t ", inst->op_name, (inst->src_data + inst->inst_num_bytes));
            break;
        default:
            ERROR("Unsupported Instruction[%s]\n", inst->op_name);
            break;
    }
}


void usage(void) {
    fprintf(stderr, "8086 Instruction Simulator Usage:\n");
    fprintf(stderr, "-h         This help dialog.\n");
    fprintf(stderr, "-i <file>  Path to file to parse.\n");
    fprintf(stderr, "-o <file>  Path to output file. Dumps the contents of the memory to the file at the end if the run.\n");
    fprintf(stderr, "-m         Dump NonZero Memory to console at the end of the run.\n");
    fprintf(stderr, "-v         Enable verbose output.\n");
    fprintf(stderr, "-t         Enable trace output. Add line numbers to verbose logs.\n");
}

int main (int argc, char *argv[]) {
    int opt;
    char *input_file = NULL;
    char *output_file = NULL;
    bool dump_memory = false;
    bool dump_file = false;
    FILE *in_fp = NULL;
    FILE *out_fp = NULL;
    size_t bytes_available;
    struct decoded_instruction_s instruction = {};

    while( (opt = getopt(argc, argv, "hi:mo:vt")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);
                break;
            
            case 'i':
                input_file = strdup(optarg);
                break;

            case 'o':
                dump_file = true;
                output_file = strdup(optarg);
                break;

            case 'm':
                dump_memory = true;
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
    if (dump_file) {
        LOG("Using Output Filename    [%s]\n", output_file);
    }
    if (dump_memory) {
        LOG("Dumping NonZero Memory Enabled\n");
    }
    LOG("\n\n");

    in_fp = fopen(input_file, "r");
    if (!in_fp) {
        fprintf(stderr, "ERROR input_file fopen failed [%d][%s]\n", errno, strerror(errno));
        exit(1);
    }

    LOG("Opened Input file[%s] OK\n", input_file);

    bytes_available = fread((uint8_t *)&memory, 1, MEMORY_SIZE, in_fp);
    fclose(in_fp);

    LOG("Read [%zu] bytes from file\n\n", bytes_available);

    LOG("Starting Simulation:\n");
    LOG("--------------------\n");

    ip = 0;

    while (ip < bytes_available) {
        uint8_t *ptr = (uint8_t *)&memory + ip;
        uint8_t prev_ip = ip;
        uint16_t pref_flags = flags;

        size_t consumed = decode_bitstream((uint8_t *)&memory + ip, bytes_available, verbose, &instruction);
        if (consumed == 0) {
            ERROR("Failed to decode instruction at byte[%zu]->[0x%02x]\n", ptr-memory, *ptr);
        }

        // dump instruction internals
        dump_instruction(&instruction, verbose);
        LOG("Found [%s][%s] Instruction, parsing...\n", instruction.op_name, instruction.name);

        switch (instruction.op) {
            case MOV_INST:
                parse_mov_inst(&instruction);
                break;
            case SUB_INST:
                parse_sub_cmp_inst(&instruction, true);
                break;
            case CMP_INST:
                parse_sub_cmp_inst(&instruction, false);
                break;
            case ADD_INST:
                parse_add_inst(&instruction);
                break;
            case JNO_INST:
            case JB_INST:
            case JNB_INST:
            case JE_INST:
            case JNZ_INST:
            case JBE_INST:
            case JA_INST:
            case JS_INST:
            case JNS_INST:
            case JP_INST:
            case JNP_INST:
            case JL_INST:
            case JNL_INST:
            case JLE_INST:
            case JG_INST:
            case LOOPNZ_INST:
            case LOOPZ_INST:
            case LOOP_INST:
            case JCXZ_INST:
                parse_jmp_inst(&instruction);
                break;
                break;
            default:
                ERROR("Unsupported instruction [%s][%s]\n", instruction.name, instruction.op_name);
        }
        printf("\tip:0x%04x -> 0x%04x", prev_ip, ip);
        if (pref_flags!=flags) {
            printf("\tFlags:[%s]->[%s]\n", get_flags(pref_flags), get_flags(flags));
        } else {
            printf("\tFlags:[%s]\n", get_flags(flags));
        }
    }
    
    LOG("\ndecode_bitstream consumed all %zu bytes from file\n", bytes_available);

    printf("\n");
    printf("Final Registers:\n");
    printf("----------------\n");
    printf("\tax: 0x%04x (%d)\n", registers[REG_AX], registers[REG_AX]);
    printf("\tbx: 0x%04x (%d)\n", registers[REG_BX], registers[REG_BX]);
    printf("\tcx: 0x%04x (%d)\n", registers[REG_CX], registers[REG_CX]);
    printf("\tdx: 0x%04x (%d)\n", registers[REG_DX], registers[REG_DX]);
    printf("\tsp: 0x%04x (%d)\n", registers[REG_SP], registers[REG_SP]);
    printf("\tbp: 0x%04x (%d)\n", registers[REG_BP], registers[REG_BP]);
    printf("\tsi: 0x%04x (%d)\n", registers[REG_SI], registers[REG_SI]);
    printf("\tdi: 0x%04x (%d)\n", registers[REG_DI], registers[REG_DI]);
    printf("\n");
    for (enum segment_register_e seg_reg=SEG_REG_ES; seg_reg<MAX_SEG_REG; seg_reg++) {
        printf("\t%s: 0x%04x (%d)\n", get_segment_register_name(seg_reg), segment_registers[seg_reg], segment_registers[seg_reg]);
    }

    printf("\n");

    printf("\tip: 0x%04x (%d)\n\n", ip, ip);

    printf("\tFlags:[%s]\n", get_flags(flags));

    if (dump_memory) {
        printf("\n");
        printf("NonZero Memory:\n");
        printf("---------------\n");
        dump_nonzoer_memory();
    }

    if (dump_file) {
        out_fp = fopen(output_file, "w");
        if (!out_fp) {
            fprintf(stderr, "ERROR output_file fopen failed [%d][%s]\n", errno, strerror(errno));
            exit(1);
        }

        LOG("Opened Output file[%s] OK\n", output_file);

        bytes_available = fwrite((uint8_t *)&memory, 1, MEMORY_SIZE, out_fp);
        printf("\n\nWrote %zu bytes to [%s]\n", bytes_available, output_file);

        fclose(in_fp);
    }

    printf("\n\n");

    return 0;
}
