/*
 * 8086 Instruction Decoder Library
 *
 */
#include <stdint.h>
#include <stdbool.h>

int decode_bitstream(uint8_t *buffer, size_t len, bool verbose);