#ifndef LDG_STR_STR_H
#define LDG_STR_STR_H

#include <stdint.h>
#include <stddef.h>
#include <dangling/core/macros.h>

#define LDG_ASCII_PRINTABLE_MIN 32
#define LDG_ASCII_PRINTABLE_MAX 126
#define LDG_ASCII_NONPRINT_CHAR '.'

#define LDG_DJB2_HASH_INIT 5381

LDG_EXPORT int32_t ldg_strrbrcpy(char *dst, const char *src, size_t abssize);

#endif
