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

/*
 * OpCode Bitstream struc
 *
 * Describes the Instruction Bitstream to match and what fields/flags 
 * are present and where can they be extracted
 */
struct opcode_bitstream_s {
    enum instructions_e op;     // Instruction encoded in opcode bitstream
    uint8_t opcode;             // bits to match for opcode
    uint8_t opcode_bitmask;     // bitmask to apply to opcode byte before comparrison against opcode
    bool byte1_has_d_flag;      // D Flag is present in byte1

};

