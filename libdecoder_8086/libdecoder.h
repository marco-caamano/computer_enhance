/*
 * 8086 Instruction Decoder Library
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <aio.h>

#define LOG(...) {                  \
        if (verbose) {              \
            printf(__VA_ARGS__);    \
        }                           \
    }

#define ERROR(...) {                    \
        fprintf(stderr, "; !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"); \
        fprintf(stderr, __VA_ARGS__);   \
        fprintf(stderr, "; !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"); \
        exit(1);                        \
    }

/*
 * 8086 Instructions can have up to 6 bytes in length
 *
 * 1st Byte:    OpCode Byte
 * 2nd Byte:    Register Fields Byte (optional)
 * 3rd Byte:    Low Displacement Byte Data (optional)
 * 4th Byte:    High Displacement Byte Data (optional)
 * 5th Byte:    Low Data Byte (optional)
 * 6th Byte:    High Data Byte (optional)
 * 
 * Bit Flags may be present in the OpCode Byte or in the Register Field Byte
 * 
 */

#define BIT_FLAG        0x1
#define D_BIT_SHIFT     1
#define S_BIT_SHIFT     1
#define MOD_FIELD_MASK  0x3
#define MOD_FIELD_SHIFT 6
#define REG_FIELD_MASK  0x7
#define OP_ENCODE_SHIFT 3
#define OP_ENCODE_MASK  0x7
#define RM_FIELD_MASK   0x7
#define RM_FIELD_SHIFT  0
#define SR_FIELD_MASK   0x3
#define SR_FIELD_SHIFT  3

/*
 * 8086 General Purpose Registers
 */
enum register_e {
    REG_AX,     // Register A 16bit
    REG_AL,     // Register A low 8 bit
    REG_AH,     // Register A high 8 but

    REG_BX,     // Register B 16bit
    REG_BL,     // Register B low 8 bit
    REG_BH,     // Register B high 8 but

    REG_CX,     // Register C 16bit
    REG_CL,     // Register C low 8 bit
    REG_CH,     // Register C high 8 but

    REG_DX,     // Register D 16bit
    REG_DL,     // Register D low 8 bit
    REG_DH,     // Register D high 8 but

    REG_SP,     // Register SP 16 bit
    REG_BP,     // Register BP 16 bit
    REG_SI,     // Register SI 16 bit
    REG_DI,     // Register DI 16 bit
    MAX_REG,    // Invalid Register
};

/*
 * 8086 Segment Registers
 */
enum segment_register_e {
    SEG_REG_ES, // Segment Register ES 16bit
    SEG_REG_CS, // Segment Register CS 16bit
    SEG_REG_SS, // Segment Register SS 16bit
    SEG_REG_DS, // Segment Register DS 16bit
    MAX_SEG_REG,
};

/*
 * 8086 Instructions
 */
enum instructions_e {
    MOV_INST,   // mov op
    ADD_INST,   // add op
    SUB_INST,   // sub op
    CMP_INST,   // cmp
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
    MAX_INST
};

/*
 * Type of Source/Destination of Operation Instruction
 */
enum operand_type_e {
    TYPE_REGISTER,              // Source/Destination is a register
    TYPE_SEGMENT_REGISTER,      // Source/Destination is a segment register
    TYPE_DIRECT_ADDRESS,        // Source/Destination is a direct address
    TYPE_EFFECTIVE_ADDRESS,     // Source/Destination is an effective address calculation
    TYPE_DATA,                  // Immediate Data from bitstream
    MAX_TYPE
};

/*
 * Decoded 8086 Instruction
 */
struct decoded_instruction_s {
    enum instructions_e op;                     // Instruction encoded in opcode bitstream
    const char *name;                           // Long Name for OP
    
    enum operand_type_e src_type;               // Source Type
    enum register_e src_register;               // Source Register when src_type is TYPE_REGISTER
    enum segment_register_e src_seg_register;   // Source Register when src_type is TYPE_SEGMENT_REGISTER
    uint16_t src_direct_address;                // Source Register when src_type is TYPE_DIRECT_ADDRESS
    enum register_e src_effective_reg1;         // First Source Register when src_type is TYPE_EFFECTIVE_ADDRESS
    enum register_e src_effective_reg2;         // Second Source Register when src_type is TYPE_EFFECTIVE_ADDRESS
    int16_t src_effective_displacement;         // Displacement when src_type is TYPE_EFFECTIVE_ADDRESS
    int16_t src_data;                           // Source Data for command
    bool src_needs_byte_decorator;              // Source Data needs to specify byte decorator
    bool src_needs_word_decorator;              // Source Data needs to specify word decorator

    enum operand_type_e dst_type;               // Destination Type
    enum register_e dst_register;               // Destination Register when dst_type is TYPE_REGISTER
    enum segment_register_e dst_seg_register;   // Destination Register when dst_type is TYPE_SEGMENT_REGISTER
    uint16_t dst_direct_address;                // Destination Register when dst_type is TYPE_DIRECT_ADDRESS
    enum register_e dst_effective_reg1;         // First Destination Register when dst_type is TYPE_EFFECTIVE_ADDRESS
    enum register_e dst_effective_reg2;         // Second Destination Register when dst_type is TYPE_EFFECTIVE_ADDRESS
    int16_t dst_effective_displacement;         // Displacement when dst_type is TYPE_EFFECTIVE_ADDRESS
    bool dst_needs_byte_decorator;              // Destination Data needs to specify byte decorator
    bool dst_needs_word_decorator;              // Destination Data needs to specify word decorator

    bool inst_is_short_w_data;                  // Decoded Instruction is a short instruction with data <inst_nemonic> <data>


};



size_t decode_bitstream(uint8_t *ptr, size_t bytes_available, bool is_verbose, struct decoded_instruction_s *inst_result);

void print_decoded_instruction(struct decoded_instruction_s *inst);
