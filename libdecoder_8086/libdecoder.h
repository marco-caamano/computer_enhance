/*
 * 8086 Instruction Decoder Library
 *
 */
#include <stdint.h>
#include <stdbool.h>

#define LOG(...) {                  \
        if (verbose) {              \
            printf(__VA_ARGS__);    \
        }                           \
    }

#define ERROR(...) {                    \
        fprintf(stderr, __VA_ARGS__);   \
        exit(1);                        \
    }


size_t decode_bitstream(uint8_t *buffer, size_t len, bool verbose);