#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "libdecoder.h"
#include "instruction_bitstream.h"

#define STR_BUFFER_SIZE         32
#define DUMP_STR_BUFFER_SIZE    3*STR_BUFFER_SIZE

// This must match against instructions_e enum
static const char *instruction_name[] = {
    "mov",
    "add",
    "sub",
    "cmp",
    "jo",
    "jno",
    "jb",
    "jnb",
    "je",
    "jnz",
    "jbe",
    "ja",
    "js",
    "jns",
    "jp",
    "jnp",
    "jl",
    "jnl",
    "jle",
    "jg",
    "loopnz",
    "loopz",
    "loop",
    "jcxz",
};

/*
 * Some Operations have the same signature but differ
 * in the op_encoding in the 2nd byte (3bits replaceing REG field)
 * 
 * OP Mappings
 * 000 ADD
 * 001 MAX_INST - so far unknown
 * 010 ADC
 * 011 SBB
 * 100 MAX_INST - so far unknown
 * 101 SUB
 * 110 MAX_INST - so far unknown
 * 111 CMP
 * 
 * Mapping Just ADD, SUB and CMP
 */
static const enum instructions_e op_encoding[] = {
    ADD_INST,   // ADD
    MAX_INST,   // Unknown
    MAX_INST,   // ADC
    MAX_INST,   // SBB
    MAX_INST,   // Unknown
    SUB_INST,   // SUB
    MAX_INST,   // Unkown
    CMP_INST,   // CMP
};

static const char *register_name[] = {
    "ax",
    "al",
    "ah",
    "bx",
    "bl",
    "bh",
    "cx",
    "cl",
    "ch",
    "dx",
    "dl",
    "dh",
    "sp",
    "bp",
    "si",
    "di",
    "INVALID"
};

static const enum register_e register_map[8][2] = {
    { REG_AL, REG_AX }, 
    { REG_CL, REG_CX }, 
    { REG_DL, REG_DX }, 
    { REG_BL, REG_BX }, 
    { REG_AH, REG_SP }, 
    { REG_CH, REG_BP }, 
    { REG_DH, REG_SI }, 
    { REG_BH, REG_DI }
};

static const char *segment_register_name[] = {
    "es",
    "cs",
    "ss",
    "ds",
    "INVALID"
};

static const enum segment_register_e segment_register_map[] = {
    SEG_REG_ES,
    SEG_REG_CS,
    SEG_REG_SS,
    SEG_REG_DS
};

/*
 * When building the effective address depending on mod
 * map these two registers to form the base of the effective
 * address calculation:
 * 
 * { REG_BX, REG_DI }  ==>   bx + di
 * 
 * In the case of MAX_REG then ignore this member
 * 
 * { REG_DI, MAX_REG } ==>   di
 * 
 */
static const enum register_e mod_effective_address_map[8][2] = {
    { REG_BX, REG_SI },
    { REG_BX, REG_DI },
    { REG_BP, REG_SI },
    { REG_BP, REG_DI },
    { REG_SI, MAX_REG },
    { REG_DI, MAX_REG },
    { REG_BP, MAX_REG },
    { REG_BX, MAX_REG }
};

static char *byte_count_str[] = {
    "",
    "1st",
    "2nd",
    "3rd",
    "4th",
    "5th",
    "6th"
};

static char *type_str[] = {
    "Register",
    "Segment Register",
    "Direct Address",
    "Effective Address",
    "Data",
    "Invalid",
};

static enum instructions_e jmp_inst_map[] = {
    JO_INST,    // jo
    JNO_INST,   // jno
    JB_INST,    // jb
    JNB_INST,   // jnb
    JE_INST,    // je
    JNZ_INST,   // jnz
    JBE_INST,   // jbe
    JA_INST,    // ja
    JS_INST,    // js
    JNS_INST,   // jns
    JP_INST,    // jp
    JNP_INST,   // jnp
    JL_INST,    // jl
    JNL_INST,   // jnl
    JLE_INST,   // jle
    JG_INST,    // jg
};

static enum instructions_e loop_inst_map[] = {
    LOOPNZ_INST,    // loopnz
    LOOPZ_INST,     // loopz
    LOOP_INST,      // loop
    JCXZ_INST,      // jcxz
};


const char *get_register_name(enum register_e reg) {
    if (reg>MAX_REG) {
        return "INVALID";
    }
    return register_name[reg];
}


const char *get_segment_register_name(enum segment_register_e seg_reg) {
    if (seg_reg>MAX_SEG_REG) {
        return "INVALID";
    }
    return segment_register_name[seg_reg];
}


/*
 * Reset the Instruction to initial values
 * Avoid using bad data
 */
void reset_instruction(struct decoded_instruction_s *inst) {
    inst->op = MAX_INST;
    inst->name = "";
    inst->op_name = "";
    inst->src_type = MAX_TYPE;
    inst->src_register = MAX_REG;
    inst->src_seg_register = MAX_SEG_REG;
    inst->src_direct_address = 0;
    inst->src_effective_reg1 = MAX_REG;
    inst->src_effective_reg2 = MAX_REG;
    inst->src_effective_displacement = 0;
    inst->src_data = 0;
    inst->src_needs_byte_decorator = false;
    inst->src_needs_word_decorator = false;
    inst->dst_type = MAX_TYPE;
    inst->dst_register = MAX_REG;
    inst->dst_seg_register = MAX_SEG_REG;
    inst->dst_direct_address = 0;
    inst->dst_effective_reg1 = MAX_REG;
    inst->dst_effective_reg2 = MAX_REG;
    inst->dst_effective_displacement = 0;
    inst->dst_needs_byte_decorator = false;
    inst->dst_needs_word_decorator = false;
    inst->inst_is_short_w_data = false;
}

void dump_instruction(struct decoded_instruction_s *inst, bool verbose) {
    LOG("; Dump Instruction:\n");
    LOG("; op                          %s\n", instruction_name[inst->op]);
    LOG("; op_name                     %s\n", inst->op_name);
    LOG("; description                 %s\n", inst->name);
    LOG("; src_type                    %s\n", type_str[inst->src_type]);
    LOG("; src_register                %s\n", register_name[inst->src_register]);
    LOG("; src_seg_register            %s\n", segment_register_name[inst->src_seg_register]);
    LOG("; src_direct_address          0x%04x\n", inst->src_direct_address);
    LOG("; src_effective_reg1          %s\n", register_name[inst->src_effective_reg1]);
    LOG("; src_effective_reg2          %s\n", register_name[inst->src_effective_reg2]);
    LOG("; src_effective_displacement  0x%04x\n", inst->src_effective_displacement);
    LOG("; src_data                    0x%04x\n", inst->src_data);
    LOG("; src_needs_byte_decorator    %s\n", inst->src_needs_byte_decorator ? "True" : "False");
    LOG("; src_needs_word_decorator    %s\n", inst->src_needs_word_decorator ? "True" : "False");
    LOG("; dst_type                    %s\n", type_str[inst->dst_type]);
    LOG("; dst_register                %s\n", register_name[inst->dst_register]);
    LOG("; dst_seg_register            %s\n", segment_register_name[inst->dst_seg_register]);
    LOG("; dst_direct_address          0x%04x\n", inst->dst_direct_address);
    LOG("; dst_effective_reg1          %s\n", register_name[inst->dst_effective_reg1]);
    LOG("; dst_effective_reg2          %s\n", register_name[inst->dst_effective_reg2]);
    LOG("; dst_effective_displacement  0x%04x\n", inst->dst_effective_displacement);
    LOG("; dst_needs_byte_decorator    %s\n", inst->dst_needs_byte_decorator ? "True" : "False");
    LOG("; dst_needs_word_decorator    %s\n", inst->dst_needs_word_decorator ? "True" : "False");
    LOG("; inst_is_short_w_data        %s\n", inst->inst_is_short_w_data ? "True" : "False");
}


void print_decoded_instruction(struct decoded_instruction_s *inst) {
    char dump_buffer[3*DUMP_STR_BUFFER_SIZE] = {0};
    char dst_buffer[STR_BUFFER_SIZE] = {0};
    char src_buffer[STR_BUFFER_SIZE] = {0};

    if (inst->inst_is_short_w_data) {
        // special case if the instruction is a short op with data then just print that out
        snprintf((char *)&dump_buffer, DUMP_STR_BUFFER_SIZE, "%s %d", 
                instruction_name[inst->op], inst->src_data);
        printf("%s\n", dump_buffer);
        return;
    }

    switch (inst->dst_type) {
        case TYPE_REGISTER:
            snprintf((char *)&dst_buffer, STR_BUFFER_SIZE, "%s", 
                register_name[inst->dst_register]);
            break;
        case TYPE_SEGMENT_REGISTER:
            snprintf((char *)&dst_buffer, STR_BUFFER_SIZE, "%s", 
                segment_register_name[inst->dst_seg_register]);
            break;
        case TYPE_DIRECT_ADDRESS:
            snprintf((char *)&dst_buffer, STR_BUFFER_SIZE, "[ %d ]", 
                inst->dst_direct_address);
            break;
        case TYPE_EFFECTIVE_ADDRESS:
            if (inst->dst_effective_reg2<MAX_REG) {
                // we have a valid second effective register
                if (inst->dst_effective_displacement != 0) {
                    char *op = "+";
                    if (inst->dst_effective_displacement<0) {
                        op = "-";
                    }
                    snprintf((char *)&dst_buffer, STR_BUFFER_SIZE, 
                        "[ %s + %s %s %d ]",
                        register_name[inst->dst_effective_reg1],
                        register_name[inst->dst_effective_reg2],
                        op,
                        inst->dst_effective_displacement);
                } else {
                    snprintf((char *)&dst_buffer, STR_BUFFER_SIZE,
                        "[ %s + %s ]", 
                        register_name[inst->dst_effective_reg1],
                        register_name[inst->dst_effective_reg2]);
                }
            } else {
                // we only have the first effective register
                if (inst->dst_effective_displacement != 0) {
                    char *op = "+";
                    if (inst->dst_effective_displacement<0) {
                        op = "";
                    }
                    snprintf((char *)&dst_buffer, STR_BUFFER_SIZE,
                        "[ %s %s %d ]",
                        register_name[inst->dst_effective_reg1],
                        op,
                        inst->dst_effective_displacement);
                } else {
                    snprintf((char *)&dst_buffer, STR_BUFFER_SIZE,
                        "[ %s ]", 
                        register_name[inst->dst_effective_reg1]);
                }
            }
            break;
        default:
            ERROR("Invalid dst_type[%d] Type\n", inst->dst_type);
            break;
    }

    switch (inst->src_type) {
        case TYPE_REGISTER:
            snprintf((char *)&src_buffer, STR_BUFFER_SIZE, "%s",
                register_name[inst->src_register]);
            break;
        case TYPE_SEGMENT_REGISTER:
            snprintf((char *)&src_buffer, STR_BUFFER_SIZE, "%s",
                segment_register_name[inst->src_seg_register]);
            break;
        case TYPE_DIRECT_ADDRESS:
            snprintf((char *)&src_buffer, STR_BUFFER_SIZE, "[ %d ]",
                inst->src_direct_address);
            break;
        case TYPE_EFFECTIVE_ADDRESS:
            if (inst->src_effective_reg2<MAX_REG) {
                // we have a valid second effective register
                if (inst->src_effective_displacement != 0) {
                    char *op = "+";
                    if (inst->src_effective_displacement<0) {
                        op = "";
                    }
                    snprintf((char *)&src_buffer, STR_BUFFER_SIZE,
                        "[ %s + %s %s %d ]",
                        register_name[inst->src_effective_reg1],
                        register_name[inst->src_effective_reg2],
                        op,
                        inst->src_effective_displacement);
                } else {
                    snprintf((char *)&src_buffer, STR_BUFFER_SIZE,
                        "[ %s + %s ]", 
                        register_name[inst->src_effective_reg1],
                        register_name[inst->src_effective_reg2]);
                }
            } else {
                // we only have the first effective register
                if (inst->src_effective_displacement != 0) {
                    char *op = "+";
                    if (inst->src_effective_displacement<0) {
                        op = "";
                    }
                    snprintf((char *)&src_buffer, STR_BUFFER_SIZE,
                        "[ %s %s %d ]",
                        register_name[inst->src_effective_reg1],
                        op,
                        inst->src_effective_displacement);
                } else {
                    snprintf((char *)&src_buffer, STR_BUFFER_SIZE,
                        "[ %s ]", 
                        register_name[inst->src_effective_reg1]);
                }
            }
            break;
        case TYPE_DATA:
            snprintf((char *)&src_buffer, STR_BUFFER_SIZE, "%d",
                inst->src_data);
            break;
        default:
            ERROR("Invalid src_type[%d] Type\n", inst->src_type);
            break;
    }
    char *src_decorator = "";
    char *dst_decorator = "";
    if (inst->src_needs_byte_decorator) {
        src_decorator = "byte ";
    } else if (inst->src_needs_word_decorator) {
        src_decorator = "word ";
    }
    if (inst->dst_needs_byte_decorator) {
        dst_decorator = "byte ";
    } else if (inst->dst_needs_word_decorator) {
        dst_decorator = "word ";
    }

    // put it all together
    snprintf((char *)&dump_buffer, DUMP_STR_BUFFER_SIZE, "%s %s%s, %s%s", 
        instruction_name[inst->op], dst_decorator, dst_buffer, src_decorator, src_buffer);
    printf("%s\n", dump_buffer);
}

bool is_dst_16bit(struct decoded_instruction_s *inst) {
    // the only 8bit destination address are dst_type TYPE_REGISTER and the low or high registers
    switch (inst->dst_type) {
        case TYPE_REGISTER:
            switch (inst->dst_register) {
                case REG_AX:
                case REG_BX:
                case REG_CX:
                case REG_DX:
                case REG_SP:
                case REG_BP:
                case REG_SI:
                case REG_DI:
                    return true;
                    break;
                default:
                    // this catches all AL, AH, BL, ...
                    return false;
                    break;
            }
            break;
        case TYPE_SEGMENT_REGISTER:
        case TYPE_DIRECT_ADDRESS:
        case TYPE_EFFECTIVE_ADDRESS:
            return true;
            break;
        case TYPE_DATA:
            ERROR("Sanity Error: TYPE_DATA is an Invalid dst_type\n");
            break;
        default:
            ERROR("Sanity Error: Invalid dst_type[%d]\n", inst->dst_type);
            break;
    }
}

/*
 * Extract Displacement from bitstream
 *
 * Returns number of bytes consumed, so the caller
 * can skip ahead to process the next unprocessed bytes
 * in the buffer
 */
size_t extract_displacement(uint8_t *ptr, uint8_t bit_w, int16_t *displacement, size_t prev_consumed_bytes, bool verbose) {
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
 * Extract Direct Address from bitstream
 *
 * Returns number of bytes consumed, so the caller
 * can skip ahead to process the next unprocessed bytes
 * in the buffer
 */
size_t extract_direct_address(uint8_t *ptr, uint16_t *direct_address, size_t prev_consumed_bytes, bool verbose) {
    int consumed_bytes = 0;
    uint8_t direct_address_low = 0;
    uint8_t direct_address_high = 0;

    // direct address is 16bit

    // consume direct_address_low byte
    ptr++;
    consumed_bytes++;
    prev_consumed_bytes++;
    direct_address_low = *ptr;
    LOG("; [0x%02x] %s Byte - direct_address_low[0x%02x] \n", *ptr, byte_count_str[prev_consumed_bytes], direct_address_low);
    
    // consume direct_address_high byte
    ptr++;
    consumed_bytes++;
    prev_consumed_bytes++;
    direct_address_high = *ptr;
    LOG("; [0x%02x] %s Byte - direct_address_high[0x%02x]\n", *ptr, byte_count_str[prev_consumed_bytes], direct_address_high);
    *direct_address = (direct_address_high << 8) | direct_address_low;
    LOG(";        Direct Address [0x%04x][%d]\n", *direct_address, *direct_address);

    return consumed_bytes;
}

/*
 * Extract Data from bitstream
 *
 * Returns number of bytes consumed, so the caller
 * can skip ahead to process the next unprocessed bytes
 * in the buffer
 */
int extract_data(uint8_t *ptr, bool is_8bit, bool is_signed_extended, int16_t *data, int prev_consumed_bytes, bool verbose) {
    int consumed_bytes = 0;
    uint8_t data_low = 0;
    uint8_t data_high = 0;

    // consume data_low byte
    ptr++;
    consumed_bytes++;
    prev_consumed_bytes++;
    data_low = *ptr;

    if (is_8bit) {
        if (is_signed_extended) {
            // Read 8bits, sign extend
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
            // Read 8bits no sign extension
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

size_t parse_instruction(uint8_t *ptr, struct opcode_bitstream_s *cmd,  bool verbose, struct decoded_instruction_s *inst_result) {
    size_t consumed_bytes = 1;
    size_t consumed = 0;
    uint8_t d_bit = 0;
    uint8_t s_bit = 0;
    uint8_t w_bit = 0;
    uint8_t reg_field = 0;
    uint8_t op_encode_field = 0;
    uint8_t mod_field = 0;
    uint8_t rm_field = 0;
    uint8_t sr_field = 0;
    uint8_t first_byte = *ptr;
    bool has_d_bit = false;
    bool has_s_bit = false;
    bool has_w_bit = false;
    bool has_mod = false;
    bool has_reg = false;
    bool has_rm = false;
    bool has_sr = false;
    
    int16_t displacement = 0;

    reset_instruction(inst_result);

    if (cmd->byte2_has_op_encode_field) {
        // multi-instruction it is mapped on op_code in 2nd Byte
        LOG("; [0x%02x] %s Byte - Found [%s] Op ", *ptr, byte_count_str[consumed_bytes], cmd->name);
    } else {
        // regular instruction
        LOG("; [0x%02x] %s Byte - Found Op[%s][%s] ", *ptr, byte_count_str[consumed_bytes], instruction_name[cmd->op], cmd->name);
    }

    inst_result->op = cmd->op;
    inst_result->op_name = instruction_name[cmd->op];
    inst_result->name = cmd->name;

    // extract any stuff from 1st byte
    if (cmd->byte1_has_d_flag) {
        d_bit = (*ptr >> D_BIT_SHIFT ) & BIT_FLAG;
        has_d_bit = true;
        LOG("D[%d] ", d_bit);
    }
    if (cmd->byte1_has_s_flag) {
        s_bit = (*ptr >> D_BIT_SHIFT ) & BIT_FLAG;
        has_s_bit = true;
        LOG("S[%d] ", s_bit);
    }
    if (cmd->byte1_has_w_flag) {
        w_bit = (*ptr >> cmd->w_flag_shift) & BIT_FLAG;
        has_w_bit = true;
        LOG("W[%d] ", w_bit);
    }
    if (cmd->byte1_has_reg_field) {
        reg_field = (*ptr >> cmd->reg_field_shift) & REG_FIELD_MASK;
        has_reg = true;
        LOG("REG[0x%x] ", reg_field);
    }
    LOG("\n");

    // move to second byte
    if (cmd->op_has_register_byte) {
        ptr++;
        consumed_bytes++;
        LOG("; 2nd Byte[0x%02x] ", *ptr);
        if (cmd->byte2_has_mod_field) {
            mod_field = (*ptr >> MOD_FIELD_SHIFT) & MOD_FIELD_MASK;
            has_mod = true;
            LOG("MOD[0x%x] ", mod_field);
        }
        if (cmd->byte2_has_reg_field) {
            reg_field = (*ptr >> cmd->reg_field_shift) & REG_FIELD_MASK;
            has_reg = true;
            LOG("REG[0x%x] ", reg_field);
        }
        if (cmd->byte2_has_op_encode_field) {
            op_encode_field = (*ptr >> OP_ENCODE_SHIFT) & OP_ENCODE_MASK;
            inst_result->op = op_encoding[op_encode_field];
            inst_result->op_name = instruction_name[inst_result->op];
            LOG("OP_ENCODE[0x%x][%s] ", op_encode_field, instruction_name[inst_result->op]);
        }
        if (cmd->byte2_has_rm_field) {
            rm_field = (*ptr >> RM_FIELD_SHIFT) & RM_FIELD_MASK;
            has_rm = true;
            LOG("RM[0x%x] ", rm_field);
        }
        if (cmd->byte2_has_sr_field) {
            sr_field = (*ptr >> SR_FIELD_SHIFT) & SR_FIELD_MASK;
            has_sr = true;
            LOG("SR[0x%x] ", sr_field);
        }
        LOG("\n");
    }

    LOG("; has_d_bit[%s] ", has_d_bit ? "True" : "False");
    LOG(" has_s_bit[%s] ", has_s_bit ? "True" : "False");
    LOG(" has_w_bit[%s] ", has_w_bit ? "True" : "False");
    LOG(" has_mod[%s] ", has_mod ? "True" : "False");
    LOG(" has_reg[%s] ", has_reg ? "True" : "False");
    LOG(" has_rm[%s] ", has_rm ? "True" : "False");
    LOG(" has_sr[%s] ", has_sr ? "True" : "False");
    LOG("\n");

    bool is_direct_access = false;
    bool is_register_mode = false;
    bool has_displacement = false;
    bool displacemen_is_16bit = false;

    if (has_sr) {
        // when we are targetting segment registers op is 16bit
        // since w_bit is not present in segment ops force it
        w_bit = 0x1;
    }

    if (has_mod) {
        // sanity check rm_field should always acompany mod field
        if (!has_rm) {
            ERROR("Failed sanity check. has_mod[%s] and has_rm[%s] is an invalid combination\n", has_mod ? "True" : "False", has_rm ? "True" : "False");
        }
        switch (mod_field) {
            case 0x0:
                if (rm_field == 0x6) {
                    // special case R/M is 110
                    // 16 bit displacement follows
                    // source is the direct address
                    is_direct_access = true;
                    has_displacement = true;
                    displacemen_is_16bit = true;
                } else {
                    // Memory Mode, No Displacement    
                }
                break;
            case 0x1:
                // Memory Mode, 8bit displacement follows
                // extract displacement
                // force bit_w to extract displacement as it is 8bits
                has_displacement = true;
                displacemen_is_16bit = false;
                break;
            case 0x2:
                // Memory Mode, 16bit displacement follows
                // force extract 16bit displacement
                has_displacement = true;
                displacemen_is_16bit = true;
                break;
            case 0x3:
                // Register Mode, No Displacement
                // get destination/source
                is_register_mode = true;
                has_displacement = false;
                break;
        }

        if (is_direct_access) {
            // special case R/M is 110
            // 16 bit displacement follows
            if (cmd->op_has_data_bytes) {
                // if the OP has data bytes then this is the source
                // and the dst is derived from direct address
                inst_result->src_type = TYPE_DATA;
                inst_result->dst_type = TYPE_DIRECT_ADDRESS;
                consumed = extract_direct_address(ptr, &inst_result->dst_direct_address, consumed_bytes, verbose);
                consumed_bytes += consumed;
                ptr += consumed;
            } else {
                // source is the direct address
                inst_result->src_type = TYPE_DIRECT_ADDRESS;
                consumed = extract_direct_address(ptr, &inst_result->src_direct_address, consumed_bytes, verbose);
                consumed_bytes += consumed;
                ptr += consumed;
                if (has_sr) {
                    // destination specified in SR field
                    inst_result->dst_type = TYPE_SEGMENT_REGISTER;
                    inst_result->dst_seg_register = segment_register_map[sr_field];
                } else {
                    // destination specificed in REG field
                    inst_result->dst_type = TYPE_REGISTER;
                    inst_result->dst_register = register_map[reg_field][w_bit];
                }
            }
        } else if (is_register_mode) {
            // Register Mode, No Displacement
            // get destination/source
            if (has_sr) {
                if (cmd->sr_is_target) {
                    // Segment register is the destination
                    inst_result->dst_type = TYPE_SEGMENT_REGISTER;
                    inst_result->dst_seg_register = segment_register_map[sr_field];
                    // Source is specified in RM field
                    inst_result->src_type = TYPE_REGISTER;
                    inst_result->src_register = register_map[rm_field][w_bit];
                } else {
                    // Segment register is the source
                    inst_result->src_type = TYPE_SEGMENT_REGISTER;
                    inst_result->src_seg_register = segment_register_map[sr_field];
                    // Destination is specified in RM field
                    inst_result->dst_type = TYPE_REGISTER;
                    inst_result->dst_register = register_map[rm_field][w_bit];
                }
            } else {
                if (has_d_bit) {
                    if (d_bit == 0x1) {
                        // destination specificed in REG field
                        inst_result->dst_type = TYPE_REGISTER;
                        inst_result->dst_register = register_map[reg_field][w_bit];
                        // Source is specified in RM field
                        inst_result->src_type = TYPE_REGISTER;
                        inst_result->src_register = register_map[rm_field][w_bit];
                    } else {
                        // destination specificed in R/M field
                        inst_result->dst_type = TYPE_REGISTER;
                        inst_result->dst_register = register_map[rm_field][w_bit];
                        // source specified in REG field
                        inst_result->src_type = TYPE_REGISTER;
                        inst_result->src_register = register_map[reg_field][w_bit];
                    }
                } else {
                    // no d_bit then dst is specified in RM
                    // destination specificed in R/M field
                    inst_result->dst_type = TYPE_REGISTER;
                    inst_result->dst_register = register_map[rm_field][w_bit];
                    // Source is Data
                    inst_result->src_type = TYPE_DATA;
                }
            }
        } else {
            // Memory Mode
            displacement = 0;
            if (has_displacement) {
                consumed = extract_displacement(ptr, displacemen_is_16bit, &displacement, consumed_bytes, verbose);
                consumed_bytes += consumed;
                ptr += consumed;
            }
            if (has_sr) {
                if (cmd->sr_is_target) {
                    // Segment register is the destination
                    inst_result->dst_type = TYPE_SEGMENT_REGISTER;
                    inst_result->dst_seg_register = segment_register_map[sr_field];
                    // Source is the effective address
                    inst_result->src_type = TYPE_EFFECTIVE_ADDRESS;
                    inst_result->src_effective_reg1 = mod_effective_address_map[rm_field][0];
                    inst_result->src_effective_reg2 = mod_effective_address_map[rm_field][1];
                    inst_result->src_effective_displacement = displacement;
                } else {
                    // Segment register is the source
                    inst_result->src_type = TYPE_SEGMENT_REGISTER;
                    inst_result->src_seg_register = segment_register_map[sr_field];
                    // Destination is the effective address
                    inst_result->dst_type = TYPE_EFFECTIVE_ADDRESS;
                    inst_result->dst_effective_reg1 = mod_effective_address_map[rm_field][0];
                    inst_result->dst_effective_reg2 = mod_effective_address_map[rm_field][1];
                    inst_result->dst_effective_displacement = displacement;
                }
            } else if (has_d_bit) {
                if (d_bit == 0x1) {
                    // destination specificed in REG field
                    inst_result->dst_type = TYPE_REGISTER;
                    inst_result->dst_register = register_map[reg_field][w_bit];
                    // source specified in effective address table
                    inst_result->src_type = TYPE_EFFECTIVE_ADDRESS;
                    inst_result->src_effective_reg1 = mod_effective_address_map[rm_field][0];
                    inst_result->src_effective_reg2 = mod_effective_address_map[rm_field][1];
                    inst_result->src_effective_displacement = displacement;
                } else {
                    // destination specificed in R/M field
                    inst_result->dst_type = TYPE_EFFECTIVE_ADDRESS;
                    inst_result->dst_effective_reg1 = mod_effective_address_map[rm_field][0];
                    inst_result->dst_effective_reg2 = mod_effective_address_map[rm_field][1];
                    inst_result->dst_effective_displacement = displacement;
                    // source specified in REG field
                    inst_result->src_type = TYPE_REGISTER;
                    inst_result->src_register = register_map[reg_field][w_bit];
                }
            } else if (!has_d_bit && !has_reg && cmd->op_has_data_bytes) {
                // OP has destination in R/M field and source is the data
                // destination specificed in R/M field
                inst_result->dst_type = TYPE_EFFECTIVE_ADDRESS;
                inst_result->dst_effective_reg1 = mod_effective_address_map[rm_field][0];
                inst_result->dst_effective_reg2 = mod_effective_address_map[rm_field][1];
                inst_result->dst_effective_displacement = displacement;
                // source is the data
                inst_result->src_type = TYPE_DATA;
            }
        }
    } else if (has_reg && has_w_bit) {
        // There is no MOD field, but there is a REG field and w_bit
        // destination specificed in REG field
        inst_result->dst_type = TYPE_REGISTER;
        inst_result->dst_register = register_map[reg_field][w_bit];
        // Source is Direct Data from the bitstream
        inst_result->src_type = TYPE_DATA;
    }

    if (cmd->op_has_hardcoded_dst) {
        // OP has a hardcoded destination
        inst_result->dst_type = TYPE_REGISTER;
        if (has_w_bit && w_bit==0x0 && cmd->hard_dst_reg<REG_SP) {
            // Add 1 offset to target low byte of the register
            inst_result->dst_register = cmd->hard_dst_reg + 1;
        } else {
            inst_result->dst_register = cmd->hard_dst_reg;
        }
        if (cmd->op_has_address_bytes) {
            // source is the direct address
            inst_result->src_type = TYPE_DIRECT_ADDRESS;
            consumed = extract_direct_address(ptr, &inst_result->src_direct_address, consumed_bytes, verbose);
            consumed_bytes += consumed;
            ptr += consumed;
        } else if (cmd->op_has_data_bytes) {
            // Source is Data
            inst_result->src_type = TYPE_DATA;
        }
    }

    if (cmd->op_has_hardcoded_src && cmd->op_has_address_bytes) {
        // OP has a hardcoded source
        inst_result->src_type = TYPE_REGISTER;
        inst_result->src_register = cmd->hard_src_reg;
        // source is the direct address
        inst_result->dst_type = TYPE_DIRECT_ADDRESS;
        consumed = extract_direct_address(ptr, &inst_result->dst_direct_address, consumed_bytes,verbose);
        consumed_bytes += consumed;
        ptr += consumed;
    }

    // we leave the data_bytes check to the end
    // as we depend on decisions made before this point
    if (cmd->op_has_data_bytes) {
        bool is_8bit = false;
        bool is_signed_extended = false;
        if (!has_w_bit) {
            ERROR("Sanity Check failed, op_has_data_bytes is True but OP does not have a W bit\n");
        }
        if (has_s_bit) {
            /* 
             * When S Bit is present we have the following cases
             * S W
             * 0 0  unsigned  8bit data -> no sign extension
             * 0 1  unsigned 16bit data
             * 1 0  signed    8bit data -> sign extend
             * 1 1  signed   16bit data
             */
            uint8_t sw_field = (s_bit<<1) | w_bit;
            switch (sw_field) {
                case 0x0:
                    // 0 0  unsigned  8bit data -> no sign extension
                    is_8bit = true;
                    is_signed_extended = false;
                    if (is_dst_16bit(inst_result)) {
                        inst_result->dst_needs_byte_decorator = true;
                    }
                    break;
                case 0x1:
                    // 0 1  unsigned 16bit data
                    is_8bit = false;
                    is_signed_extended = false;
                    break;
                case 0x2:
                    // 1 0  signed 8bit data -> sign extend
                    is_8bit = true;
                    is_signed_extended = true;
                    break;
                case 0x3:
                    // 1 1  signed 16bit data, but only 8bit in the bitstream
                    is_8bit = true;
                    is_signed_extended = true;
                    if (is_dst_16bit(inst_result)) {
                        inst_result->dst_needs_word_decorator = true;
                    }
                    break;
                default:
                    ERROR("Invalid value for sw_field\n");
                    break;
            }
        } else {
            // we decide based on w_bit
            if (w_bit==0x1) {
                //16bit data
                is_8bit = false;
                is_signed_extended = false;
                inst_result->src_needs_word_decorator = true;
            } else {
                // 8bit sign extend
                is_8bit = true;
                is_signed_extended = true;
                if (is_dst_16bit(inst_result)) {
                    // destination is 16bit but source is 8bit
                    // add decorator to source data
                    inst_result->src_needs_byte_decorator = true;
                }
            }
        }
        // extract data
        consumed = extract_data(ptr, is_8bit, is_signed_extended, &inst_result->src_data, consumed_bytes, verbose);
        consumed_bytes += consumed;
        ptr += consumed;
    }

    if (cmd->is_short_instruction) {
        inst_result->inst_is_short_w_data = true;
        // Short instruction encode the offset in the next byte
        // extract data
        bool is_8bit = true;
        bool is_signed_extended = false;
        consumed = extract_data(ptr, is_8bit, is_signed_extended, &inst_result->src_data, consumed_bytes, verbose);
        consumed_bytes += consumed;
        ptr += consumed;

        if ((first_byte & JMP_INST_MASK)==JMP_INST_ID) {
            uint8_t jmp_field = first_byte & 0xF;
            inst_result->op = jmp_inst_map[jmp_field];
            inst_result->op_name = instruction_name[inst_result->op];
            LOG("; JMP instruction jmp_field[0x%x] decoded to [%s] offset[%d] ", jmp_field, instruction_name[inst_result->op], inst_result->src_data);
        } else if ((first_byte & LOOP_INST_MASK)==LOOP_INST_ID) {
            uint8_t loop_field = first_byte & 0x3;
            inst_result->op = loop_inst_map[loop_field];
            inst_result->op_name = instruction_name[inst_result->op];
            LOG("; JMP instruction loop_field[0x%x] decoded to [%s] offset[%d] ", loop_field, instruction_name[inst_result->op], inst_result->src_data);
        }


    }

    return consumed_bytes;
}

size_t decode_bitstream(uint8_t *ptr, size_t bytes_available, bool verbose, struct decoded_instruction_s *inst_result) {
    size_t op_cmd_size = sizeof(op_cmds) / sizeof(struct opcode_bitstream_s *);

    LOG("\n; [%s:%d] ptr[%p] bytes_available[%zu] verbose[%s]\n", __FUNCTION__, __LINE__, ptr, bytes_available, verbose ? "True" : "False");

    LOG("; Testing byte[0x%x] for 8086 instructions\n", *ptr);
    for (size_t i=0; i<op_cmd_size; i++) {
        LOG("; Testing for OP[0x%x]\n", op_cmds[i]->opcode);
        if ( (*ptr & op_cmds[i]->opcode_bitmask) == op_cmds[i]->opcode ) {
            LOG("; Matched OP[0x%x][%s]\n", op_cmds[i]->opcode, op_cmds[i]->name);
            return parse_instruction(ptr, op_cmds[i], verbose, inst_result);
        }
    }
    LOG("; Failed to match byte[0x%x] for 8086 instructions\n", *ptr);
  
    return 0;
}