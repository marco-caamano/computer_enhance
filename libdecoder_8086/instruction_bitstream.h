#include <stdint.h>
#include <stdbool.h>


/*
 * 8086 Instructions
 */
enum instructions_e {
    MOV_INST,   // mov op
    MAX_INST
};

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

#define MOD_FIELD_MASK  0x3
#define REG_FIELD_MASK  0x7
#define RM_FIELD_MASK   0x7

/*
 * OpCode Bitstream struc
 *
 * Describes the Instruction Bitstream to match and what fields/flags 
 * are present and where can they be extracted
 */
struct opcode_bitstream_s {
    const enum instructions_e op;     // Instruction encoded in opcode bitstream
    const char *name;                 // Long Name for OP
    const uint8_t opcode;             // bits to match for opcode
    const uint8_t opcode_bitmask;     // bitmask to apply to opcode byte before comparrison against opcode

    const bool byte1_has_d_flag;      // D Flag is present in byte1

    const bool byte1_has_w_flag;      // W Flag is present in byte1
    const uint8_t w_flag_shift;       // Shift for W Flag

    const bool byte1_has_reg_field;   // REG field is present in byte1
    const bool byte2_has_reg_field;   // REG field is present in byte2
    const uint8_t reg_field_shift;    // Shift for REG Field, depending on which byte it is located

    const bool byte2_has_mod_field;   // MOD field is present in byte2 | Shift is always the same
    const bool byte2_has_rm_field;    // RM field is present in byte2 | Shift is always the same

};

