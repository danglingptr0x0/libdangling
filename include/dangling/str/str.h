#ifndef LDG_STR_STR_H
#define LDG_STR_STR_H

#include <stdint.h>
#include <stddef.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_ASCII_PRINTABLE_MIN 32
#define LDG_ASCII_PRINTABLE_MAX 126
#define LDG_ASCII_NONPRINT_CHAR '.'

#define LDG_DJB2_HASH_INIT 5381

LDG_EXPORT uint32_t ldg_strrbrcpy(char *dst, const char *src, size_t abssize);

static inline int ldg_char_space_is(char c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

static inline int ldg_char_digit_is(char c)
{
    return c >= '0' && c <= '9';
}

static inline int ldg_char_alpha_is(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline int ldg_char_alnum_is(char c)
{
    return ldg_char_alpha_is(c) || ldg_char_digit_is(c);
}

static inline int ldg_char_hex_is(char c)
{
    return ldg_char_digit_is(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

LDG_EXPORT void ldg_byte_to_hex(byte_t val, char out[3]);
LDG_EXPORT void ldg_dword_to_hex(dword_t val, char *buff);
LDG_EXPORT void ldg_str_to_dec(const char *str, dword_t *out);
LDG_EXPORT void ldg_hex_to_nipple(char c, byte_t *nipple);
LDG_EXPORT void ldg_hex_to_dword(const char *str, dword_t *out);
LDG_EXPORT void ldg_hex_to_bytes(const char *hex, byte_t *out, size_t max);
LDG_EXPORT int ldg_hex_str_is(const char *str);

#endif
