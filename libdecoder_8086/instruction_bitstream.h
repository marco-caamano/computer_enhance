#include <stdint.h>
#include <stdbool.h>

/*
 * OpCode Bitstream struct
 *
 * Describes the Instruction Bitstream to match and what fields/flags 
 * are present and where can they be extracted
 */
struct opcode_bitstream_s {
    const enum instructions_e op;     // Instruction encoded in opcode bitstream
    const char *name;                 // Long Name for OP
    const uint8_t opcode;             // bits to match for opcode
    const uint8_t opcode_bitmask;     // bitmask to apply to opcode byte before comparrison against opcode
    const bool op_has_register_byte;  // operation has 2nd byte with register fields
    const bool op_has_opt_disp_bytes; // operation has optional displacement bytes (optional 16bit displacement)
    const bool op_has_data_bytes;     // operation has data bytes (optional 16bit data)
    const bool op_has_address_bytes;  // operation has 2 bytes for direct address

    const bool byte1_has_d_flag;      // D Flag is present in byte1

    const bool byte1_has_w_flag;      // W Flag is present in byte1
    const uint8_t w_flag_shift;       // Shift for W Flag

    const bool byte1_has_reg_field;   // REG field is present in byte1
    const bool byte2_has_reg_field;   // REG field is present in byte2
    const uint8_t reg_field_shift;    // Shift for REG Field, depending on which byte it is located

    const bool byte2_has_mod_field;   // MOD field is present in byte2 | Shift is always the same
    const bool byte2_has_rm_field;    // RM field is present in byte2 | Shift is always the same
    const bool byte2_has_sr_field;    // SR segment field is present in byte2 | Shift is always the same

    const bool sr_is_target;          // When type is Segment Register is the destination the segment register

};

// 100010DW register/memory to/from register
struct opcode_bitstream_s mov1_op = {
    .op = MOV_INST,
    .name = "Register/Memory to/from Register MOV",
    .opcode = 0x88,
    .opcode_bitmask = 0xFC,
    .op_has_register_byte = true,
    .op_has_opt_disp_bytes = true,
    .op_has_data_bytes = false,
    .op_has_address_bytes = false,
    .byte1_has_d_flag = true,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = true,
    .reg_field_shift = 3,
    .byte2_has_mod_field = true,
    .byte2_has_rm_field = true,
    .byte2_has_sr_field = false,
    .sr_is_target = false,
};

// 1100011W immediate to register/memory
struct opcode_bitstream_s mov2_op = {
    .op = MOV_INST,
    .name = "Immediate to Register/Memory MOV",
    .opcode = 0xC6,
    .opcode_bitmask = 0xFE,
    .op_has_register_byte = true,
    .op_has_opt_disp_bytes = true,
    .op_has_data_bytes = true,
    .op_has_address_bytes = false,
    .byte1_has_d_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = false,
    .reg_field_shift = 0,
    .byte2_has_mod_field = true,
    .byte2_has_rm_field = true,
    .byte2_has_sr_field = false,
    .sr_is_target = false,
};

// 1011WREG immediate to register
struct opcode_bitstream_s mov3_op = {
    .op = MOV_INST,
    .name = "Immediate to Register MOV",
    .opcode = 0xB0,
    .opcode_bitmask = 0xF0,
    .op_has_register_byte = false,
    .op_has_opt_disp_bytes = false,
    .op_has_data_bytes = true,
    .op_has_address_bytes = false,
    .byte1_has_d_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 3,
    .byte1_has_reg_field = true,
    .byte2_has_reg_field = false,
    .reg_field_shift = 0,
    .byte2_has_mod_field = false,
    .byte2_has_rm_field = false,
    .byte2_has_sr_field = false,
    .sr_is_target = false,
};

struct opcode_bitstream_s *op_cmds[] = {
    &mov1_op,
    &mov2_op,
    &mov3_op,
};
