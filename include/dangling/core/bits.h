#ifndef LDG_CORE_BITS_H
#define LDG_CORE_BITS_H

#define LDG_BYTE_BITS 8
#define LDG_BYTE_MASK 0xFFU
#define LDG_BYTE_SHIFT_3 24
#define LDG_BYTE_SHIFT_2 16
#define LDG_BYTE_SHIFT_1 8

#define LDG_NIBBLE_BITS 4
#define LDG_NIBBLE_MASK 0xFU

#define LDG_WORD_BYTES 4
#define LDG_WORD_MASK (LDG_WORD_BYTES - 1)

#define LDG_IS_POW2(x) ((x) != 0 && ((x) & ((x) - 1)) == 0)

#endif
