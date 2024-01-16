#include <stdint.h>
#include <stdbool.h>

/*
 * OpCode Bitstream struct
 *
 * Describes the Instruction Bitstream to match and what fields/flags 
 * are present and where can they be extracted
 */
struct opcode_bitstream_s {
    const enum instructions_e op;           // Instruction encoded in opcode bitstream
    const char *name;                       // Long Name for OP
    const uint8_t opcode;                   // bits to match for opcode
    const uint8_t opcode_bitmask;           // bitmask to apply to opcode byte before comparrison against opcode
    const bool op_has_register_byte;        // operation has 2nd byte with register fields
    const bool op_has_opt_disp_bytes;       // operation has optional displacement bytes (optional 16bit displacement)
    const bool op_has_data_bytes;           // operation has data bytes (optional 16bit data)
    const bool op_has_address_bytes;        // operation has 2 bytes for direct address

    const bool byte1_has_d_flag;            // D Flag is present in byte1
    const bool byte1_has_s_flag;            // S Flag is present in byte1

    const bool byte1_has_w_flag;            // W Flag is present in byte1
    const uint8_t w_flag_shift;             // Shift for W Flag

    const bool byte1_has_reg_field;         // REG field is present in byte1
    const bool byte2_has_reg_field;         // REG field is present in byte2
    const uint8_t reg_field_shift;          // Shift for REG Field, depending on which byte it is located

    const bool byte2_has_mod_field;         // MOD field is present in byte2 | Shift is always the same
    const bool byte2_has_rm_field;          // RM field is present in byte2 | Shift is always the same
    const bool byte2_has_sr_field;          // SR segment field is present in byte2 | Shift is always the same
    const bool byte2_has_op_encode_field;   // OP Encode Field (ADD/SUB/CMP) is present in byte2 | Shift is always the same

    const bool sr_is_target;                // When type is Segment Register is the destination the segment register

    const bool op_has_hardcoded_dst;        // Operation has a hardcoded destination register
    const enum register_e hard_dst_reg;     // Hardcoded Destination Register
    const bool op_has_hardcoded_src;        // Operation has a hardcoded source register
    const enum register_e hard_src_reg;     // Hardcoded Source Register

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
    .byte1_has_s_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = true,
    .reg_field_shift = 3,
    .byte2_has_mod_field = true,
    .byte2_has_rm_field = true,
    .byte2_has_sr_field = false,
    .byte2_has_op_encode_field = false,
    .sr_is_target = false,
    .op_has_hardcoded_dst = false,
    .hard_dst_reg = MAX_REG,
    .op_has_hardcoded_src = false,
    .hard_src_reg = MAX_REG,
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
    .byte1_has_s_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = false,
    .reg_field_shift = 0,
    .byte2_has_mod_field = true,
    .byte2_has_rm_field = true,
    .byte2_has_sr_field = false,
    .byte2_has_op_encode_field = false,
    .sr_is_target = false,
    .op_has_hardcoded_dst = false,
    .hard_dst_reg = MAX_REG,
    .op_has_hardcoded_src = false,
    .hard_src_reg = MAX_REG,
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
    .byte1_has_s_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 3,
    .byte1_has_reg_field = true,
    .byte2_has_reg_field = false,
    .reg_field_shift = 0,
    .byte2_has_mod_field = false,
    .byte2_has_rm_field = false,
    .byte2_has_sr_field = false,
    .byte2_has_op_encode_field = false,
    .sr_is_target = false,
    .op_has_hardcoded_dst = false,
    .hard_dst_reg = MAX_REG,
    .op_has_hardcoded_src = false,
    .hard_src_reg = MAX_REG,
};

// 1010000W Memory to Accumulator
struct opcode_bitstream_s mov4_op = {
    .op = MOV_INST,
    .name = "Memory to Accumulator MOV",
    .opcode = 0xA0,
    .opcode_bitmask = 0xFE,
    .op_has_register_byte = false,
    .op_has_opt_disp_bytes = false,
    .op_has_data_bytes = false,
    .op_has_address_bytes = true,
    .byte1_has_d_flag = false,
    .byte1_has_s_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = false,
    .reg_field_shift = 0,
    .byte2_has_mod_field = false,
    .byte2_has_rm_field = false,
    .byte2_has_sr_field = false,
    .byte2_has_op_encode_field = false,
    .sr_is_target = false,
    .op_has_hardcoded_dst = true,
    .hard_dst_reg = REG_AX,
    .op_has_hardcoded_src = false,
    .hard_src_reg = MAX_REG,
};

// 1010000W Memory to Accumulator
struct opcode_bitstream_s mov5_op = {
    .op = MOV_INST,
    .name = "Accumulator to Memory MOV",
    .opcode = 0xA2,
    .opcode_bitmask = 0xFE,
    .op_has_register_byte = false,
    .op_has_opt_disp_bytes = false,
    .op_has_data_bytes = false,
    .op_has_address_bytes = true,
    .byte1_has_d_flag = false,
    .byte1_has_s_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = false,
    .reg_field_shift = 0,
    .byte2_has_mod_field = false,
    .byte2_has_rm_field = false,
    .byte2_has_sr_field = false,
    .byte2_has_op_encode_field = false,
    .sr_is_target = false,
    .op_has_hardcoded_dst = false,
    .hard_dst_reg = MAX_REG,
    .op_has_hardcoded_src = true,
    .hard_src_reg = REG_AX,
};

// 000000DW Add Reg/Memory with register to either
struct opcode_bitstream_s add1_op = {
    .op = ADD_INST,
    .name = "Reg/Memory with register to either ADD",
    .opcode = 0x00,
    .opcode_bitmask = 0xFC,
    .op_has_register_byte = true,
    .op_has_opt_disp_bytes = true,
    .op_has_data_bytes = false,
    .op_has_address_bytes = false,
    .byte1_has_d_flag = true,
    .byte1_has_s_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = true,
    .reg_field_shift = 3,
    .byte2_has_mod_field = true,
    .byte2_has_rm_field = true,
    .byte2_has_sr_field = false,
    .byte2_has_op_encode_field = false,
    .sr_is_target = false,
    .op_has_hardcoded_dst = false,
    .hard_dst_reg = MAX_REG,
    .op_has_hardcoded_src = false,
    .hard_src_reg = MAX_REG,
};

// 100000SW Immediate to Register/Memory ADD/SUB/CMP
struct opcode_bitstream_s multi1_op = {
    .op = MAX_INST,
    .name = "Immediate to register/memory MultiInstruction ADD/SUB/CMP",
    .opcode = 0x80,
    .opcode_bitmask = 0xFC,
    .op_has_register_byte = true,
    .op_has_opt_disp_bytes = true,
    .op_has_data_bytes = true,
    .op_has_address_bytes = false,
    .byte1_has_d_flag = false,
    .byte1_has_s_flag = true,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = false,
    .reg_field_shift = 3,
    .byte2_has_mod_field = true,
    .byte2_has_rm_field = true,
    .byte2_has_sr_field = false,
    .byte2_has_op_encode_field = true,
    .sr_is_target = false,
    .op_has_hardcoded_dst = false,
    .hard_dst_reg = MAX_REG,
    .op_has_hardcoded_src = false,
    .hard_src_reg = MAX_REG,
};

//  0000010W Immediate to Accumulator ADD
struct opcode_bitstream_s add2_op = {
    .op = ADD_INST,
    .name = "Immediate to Accumulator ADD",
    .opcode = 0x04,
    .opcode_bitmask = 0xFE,
    .op_has_register_byte = false,
    .op_has_opt_disp_bytes = false,
    .op_has_data_bytes = true,
    .op_has_address_bytes = false,
    .byte1_has_d_flag = false,
    .byte1_has_s_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = false,
    .reg_field_shift = 0,
    .byte2_has_mod_field = false,
    .byte2_has_rm_field = false,
    .byte2_has_sr_field = false,
    .byte2_has_op_encode_field = false,
    .sr_is_target = false,
    .op_has_hardcoded_dst = true,
    .hard_dst_reg = REG_AX,
    .op_has_hardcoded_src = false,
    .hard_src_reg = MAX_REG,
};

// 001010DW SUB Reg/Memory with register to either
struct opcode_bitstream_s sub1_op = {
    .op = SUB_INST,
    .name = "Reg/Memory with register to either SUB",
    .opcode = 0x28,
    .opcode_bitmask = 0xFC,
    .op_has_register_byte = true,
    .op_has_opt_disp_bytes = true,
    .op_has_data_bytes = false,
    .op_has_address_bytes = false,
    .byte1_has_d_flag = true,
    .byte1_has_s_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = true,
    .reg_field_shift = 3,
    .byte2_has_mod_field = true,
    .byte2_has_rm_field = true,
    .byte2_has_sr_field = false,
    .byte2_has_op_encode_field = false,
    .sr_is_target = false,
    .op_has_hardcoded_dst = false,
    .hard_dst_reg = MAX_REG,
    .op_has_hardcoded_src = false,
    .hard_src_reg = MAX_REG,
};

// 0010110W Immediate to Accumulator SUB
struct opcode_bitstream_s sub2_op = {
    .op = SUB_INST,
    .name = "Immediate to Accumulator SUB",
    .opcode = 0x2C,
    .opcode_bitmask = 0xFE,
    .op_has_register_byte = false,
    .op_has_opt_disp_bytes = false,
    .op_has_data_bytes = true,
    .op_has_address_bytes = false,
    .byte1_has_d_flag = false,
    .byte1_has_s_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = false,
    .reg_field_shift = 0,
    .byte2_has_mod_field = false,
    .byte2_has_rm_field = false,
    .byte2_has_sr_field = false,
    .byte2_has_op_encode_field = false,
    .sr_is_target = false,
    .op_has_hardcoded_dst = true,
    .hard_dst_reg = REG_AX,
    .op_has_hardcoded_src = false,
    .hard_src_reg = MAX_REG,
};

// 001110DW SUB Reg/Memory with register to either
struct opcode_bitstream_s cmp1_op = {
    .op = CMP_INST,
    .name = "Reg/Memory with register to either CMP",
    .opcode = 0x38,
    .opcode_bitmask = 0xFC,
    .op_has_register_byte = true,
    .op_has_opt_disp_bytes = true,
    .op_has_data_bytes = false,
    .op_has_address_bytes = false,
    .byte1_has_d_flag = true,
    .byte1_has_s_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = true,
    .reg_field_shift = 3,
    .byte2_has_mod_field = true,
    .byte2_has_rm_field = true,
    .byte2_has_sr_field = false,
    .byte2_has_op_encode_field = false,
    .sr_is_target = false,
    .op_has_hardcoded_dst = false,
    .hard_dst_reg = MAX_REG,
    .op_has_hardcoded_src = false,
    .hard_src_reg = MAX_REG,
};

// 0011110W Immediate with Accumulator CMP
struct opcode_bitstream_s cmp2_op = {
    .op = CMP_INST,
    .name = "Immediate to Accumulator SUB",
    .opcode = 0x3C,
    .opcode_bitmask = 0xFE,
    .op_has_register_byte = false,
    .op_has_opt_disp_bytes = false,
    .op_has_data_bytes = true,
    .op_has_address_bytes = false,
    .byte1_has_d_flag = false,
    .byte1_has_s_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = false,
    .reg_field_shift = 0,
    .byte2_has_mod_field = false,
    .byte2_has_rm_field = false,
    .byte2_has_sr_field = false,
    .byte2_has_op_encode_field = false,
    .sr_is_target = false,
    .op_has_hardcoded_dst = true,
    .hard_dst_reg = REG_AX,
    .op_has_hardcoded_src = false,
    .hard_src_reg = MAX_REG,
};

struct opcode_bitstream_s *op_cmds[] = {
    &mov1_op,
    &mov2_op,
    &mov3_op,
    &mov4_op,
    &mov5_op,
    &add1_op,
    &multi1_op,
    &add2_op,
    &sub1_op,
    &sub2_op,
    &cmp1_op,
    &cmp2_op,
};
