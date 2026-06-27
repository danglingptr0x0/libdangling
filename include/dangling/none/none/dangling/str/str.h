#ifndef LDG_STR_STR_H
#define LDG_STR_STR_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_ASCII_MAX 0x7F
#define LDG_ASCII_PRINTABLE_MIN 32
#define LDG_ASCII_PRINTABLE_MAX 126
#define LDG_ASCII_NONPRINT_CHAR '.'

#define LDG_DJB2_HASH_INIT 5381

#define LDG_FNV1A_32_OFFSET 2166136261U
#define LDG_FNV1A_32_PRIME 16777619U
#define LDG_FNV1A_64_OFFSET 14695981039346656037ULL
#define LDG_FNV1A_64_PRIME 1099511628211ULL

LDG_EXPORT uint32_t ldg_strrbrcpy(char *dst, const char *src, uint64_t abssize);

static inline uint8_t ldg_char_space_is(uint8_t c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

static inline uint8_t ldg_char_digit_is(uint8_t c)
{
    return c >= '0' && c <= '9';
}

static inline uint8_t ldg_char_alpha_is(uint8_t c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline uint8_t ldg_char_alnum_is(uint8_t c)
{
    return ldg_char_alpha_is(c) || ldg_char_digit_is(c);
}

static inline uint8_t ldg_char_hex_is(uint8_t c)
{
    return ldg_char_digit_is(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

LDG_EXPORT uint32_t ldg_byte_to_hex(byte_t val, char out[3]);
LDG_EXPORT uint32_t ldg_dword_to_hex(dword_t val, char *buff);
LDG_EXPORT uint32_t ldg_str_to_dec(const char *str, dword_t *out);
LDG_EXPORT uint32_t ldg_hex_to_nipple(char c, byte_t *nipple);
LDG_EXPORT uint32_t ldg_hex_to_dword(const char *str, dword_t *out);
LDG_EXPORT uint32_t ldg_hex_to_bytes(const char *hex, byte_t *out, uint64_t max);
LDG_EXPORT uint8_t ldg_hex_str_is(const char *str);
LDG_EXPORT uint32_t ldg_hash_fnv1a_32_compute(const uint8_t *data, uint64_t len, uint32_t *out);
LDG_EXPORT uint32_t ldg_hash_fnv1a_64_compute(const uint8_t *data, uint64_t len, uint64_t *out);

#endif
