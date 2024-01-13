#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "libdecoder.h"
#include "instruction_bitstream.h"

// 100010DW register/memory to/from register
struct opcode_bitstream_s mov1_op = {
    .op = MOV_INST,
    .name = "Register/Memory to/from Register MOV",
    .opcode = 0x88,
    .opcode_bitmask = 0xFC,
    .byte1_has_d_flag = true,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = true,
    .reg_field_shift = 3,
    .byte2_has_mod_field = true,
    .byte2_has_rm_field = true,
};

// 1100011W immediate to register/memory
struct opcode_bitstream_s mov2_op = {
    .op = MOV_INST,
    .name = "Immediate to Register/Memory MOV",
    .opcode = 0xC6,
    .opcode_bitmask = 0xFE,
    .byte1_has_d_flag = false,
    .byte1_has_w_flag = true,
    .w_flag_shift = 0,
    .byte1_has_reg_field = false,
    .byte2_has_reg_field = false,
    .reg_field_shift = 0,
    .byte2_has_mod_field = true,
    .byte2_has_rm_field = true,
};

struct opcode_bitstream_s *op_cmds[] = {
    &mov1_op,
    &mov2_op,
};


size_t parse_instruction(uint8_t *ptr, bool verbose) {
    LOG("; Testing byte[0x%x] for 8086 instructions\n", *ptr);
    size_t op_cmd_size = sizeof(op_cmds) / sizeof(struct opcode_bitstream_s *);
    for (size_t i=0; i<op_cmd_size; i++) {
        LOG(";\tTesting for OP[0x%x]\n", op_cmds[i]->opcode);
        if ( (*ptr & op_cmds[i]->opcode_bitmask) == op_cmds[i]->opcode ) {
            LOG(";\tMatched OP[0x%x][%s]\n", op_cmds[i]->opcode, op_cmds[i]->name);
        }
    }

    return 1;
}

size_t decode_bitstream(uint8_t *buffer, size_t len, bool verbose) {
    size_t bytes_available = len;
    size_t bytes_consumed = 0;
    size_t total_consumed = 0;
    uint8_t *ptr = buffer;

    LOG("[%s:%d] buffer[%p] size[%zu] verbose[%s]\n", __FUNCTION__, __LINE__, buffer, len, verbose ? "True" : "False");

    while ( total_consumed < bytes_available) {
        bytes_consumed = parse_instruction(ptr, verbose);
        ptr += bytes_consumed;
        total_consumed += bytes_consumed;
        LOG("; Consumed %zu bytes | total consumed %zu\n", bytes_consumed, total_consumed);
    }

    return total_consumed;
}