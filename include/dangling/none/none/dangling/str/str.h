#ifndef LDG_STR_STR_H
#define LDG_STR_STR_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_ASCII_MAX 0x7F
#define LDG_ASCII_PRINTABLE_MIN 32
#define LDG_ASCII_PRINTABLE_MAX 126
#define LDG_ASCII_NONPRINT_CHAR '.'
#define LDG_ASCII_DELETE 127
#define LDG_ASCII_CASE_GAP 32
#define LDG_ASCII_DIGIT_ZERO '0'
#define LDG_STR_BASE_MIN 2
#define LDG_STR_BASE_MAX 36

#define LDG_DJB2_HASH_INIT 5381

#define LDG_FNV1A_32_OFFSET 2166136261U
#define LDG_FNV1A_32_PRIME 16777619U
#define LDG_FNV1A_64_OFFSET 14695981039346656037ULL
#define LDG_FNV1A_64_PRIME 1099511628211ULL

LDG_EXPORT uint32_t ldg_str_rbrcpy(char *dst, const char *src, uint64_t abssize);

static inline uint8_t ldg_char_space_is(uint8_t character)
{
    if (character == ' ' || character == '\t' || character == '\n') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static inline uint8_t ldg_char_digit_is(uint8_t character)
{
    if (character >= '0' && character <= '9') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static inline uint8_t ldg_char_alpha_is(uint8_t character)
{
    if (character >= 'a' && character <= 'z') { return LDG_TRUTH_TRUE; }

    if (character >= 'A' && character <= 'Z') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static inline uint8_t ldg_char_alnum_is(uint8_t character)
{
    if (ldg_char_alpha_is(character) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (ldg_char_digit_is(character) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static inline uint8_t ldg_char_hex_is(uint8_t character)
{
    if (ldg_char_digit_is(character) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (character >= 'a' && character <= 'f') { return LDG_TRUTH_TRUE; }

    if (character >= 'A' && character <= 'F') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static inline uint8_t ldg_char_upper_is(uint8_t character)
{
    if (character >= 'A' && character <= 'Z') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static inline uint8_t ldg_char_lower_is(uint8_t character)
{
    if (character >= 'a' && character <= 'z') { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static inline uint8_t ldg_char_printable_is(uint8_t character)
{
    if (character >= LDG_ASCII_PRINTABLE_MIN && character <= LDG_ASCII_PRINTABLE_MAX) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static inline uint8_t ldg_char_control_is(uint8_t character)
{
    if (character < LDG_ASCII_PRINTABLE_MIN) { return LDG_TRUTH_TRUE; }

    if (character == LDG_ASCII_DELETE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static inline uint8_t ldg_char_punctuation_is(uint8_t character)
{
    if (ldg_char_printable_is(character) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (character == ' ') { return LDG_TRUTH_FALSE; }

    if (ldg_char_alnum_is(character) == LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static inline char ldg_char_upper_get(char character)
{
    if (ldg_char_lower_is((uint8_t)character) == LDG_TRUTH_TRUE) { return (char)(character - LDG_ASCII_CASE_GAP); }

    return character;
}

static inline char ldg_char_lower_get(char character)
{
    if (ldg_char_upper_is((uint8_t)character) == LDG_TRUTH_TRUE) { return (char)(character + LDG_ASCII_CASE_GAP); }

    return character;
}

LDG_EXPORT uint32_t ldg_str_uint_parse(const char *str, uint32_t base, uint64_t max, uint64_t *out);
LDG_EXPORT uint32_t ldg_str_int64_parse(const char *str, uint32_t base, uint64_t max, int64_t *out);
LDG_EXPORT uint32_t ldg_str_len_get(const char *str, uint64_t max, uint64_t *out);
LDG_EXPORT uint32_t ldg_str_alloc_cpy(const char *src, uint64_t max, char **out);
LDG_EXPORT uint32_t ldg_str_concatenate(char *dst, const char *src, uint64_t cap);
LDG_EXPORT uint32_t ldg_str_char_find(const char *str, char tgt, uint64_t max, uint64_t *out_idx);
LDG_EXPORT uint32_t ldg_str_char_last_find(const char *str, char tgt, uint64_t max, uint64_t *out_idx);
LDG_EXPORT uint32_t ldg_str_substring_find(const char *text, const char *pattern, uint64_t max, uint64_t *out_idx);
LDG_EXPORT uint32_t ldg_str_cmp(const char *lhs, const char *rhs, uint64_t max, uint32_t *out);
LDG_EXPORT uint32_t ldg_str_case_cmp(const char *lhs, const char *rhs, uint64_t max, uint32_t *out);
LDG_EXPORT uint32_t ldg_str_tok_get(const char *str, const char *delimiter, uint64_t *cursor, uint64_t max, uint64_t *out_start, uint64_t *out_len);

LDG_EXPORT uint32_t ldg_byte_to_hex(byte_t val, char out[3]);
LDG_EXPORT uint32_t ldg_dword_to_hex(dword_t val, char *buff);
LDG_EXPORT uint32_t ldg_str_to_dec(const char *str, dword_t *out);
LDG_EXPORT uint32_t ldg_hex_to_nipple(char c, byte_t *nipple);
LDG_EXPORT uint32_t ldg_hex_to_dword(const char *str, dword_t *out);
LDG_EXPORT uint32_t ldg_hex_to_bytes(const char *hex, byte_t *out, uint64_t max);
LDG_EXPORT uint8_t ldg_hex_str_is(const char *str);
LDG_EXPORT uint32_t ldg_str_base64_encode(const uint8_t *raw, uint64_t len, char *out, uint64_t cap, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_str_base64_decode(const char *text, uint64_t len, uint8_t *out, uint64_t cap, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_hash_fnv1a_32_compute(const uint8_t *data, uint64_t len, uint32_t *out);
LDG_EXPORT uint32_t ldg_hash_fnv1a_64_compute(const uint8_t *data, uint64_t len, uint64_t *out);

#endif
