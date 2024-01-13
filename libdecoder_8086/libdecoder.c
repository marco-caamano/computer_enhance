#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "libdecoder.h"
#include "instruction_bitstream.h"

int decode_bitstream(uint8_t *buffer, size_t len, bool verbose) {
    printf("[%s:%d] buffer[%p] size[%zu] verbose[%s]\n", __FUNCTION__, __LINE__, buffer, len, verbose ? "True" : "False");
    return 0;
}