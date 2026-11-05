#ifndef LDG_PROTO_CW_H
#define LDG_PROTO_CW_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_PROTO_CW_ELEM_MAX 10
#define LDG_PROTO_CW_TEXT_MAX 6
#define LDG_PROTO_CW_SYM_CUNT 56

#define LDG_PROTO_CW_DIT_CH '.'
#define LDG_PROTO_CW_DAH_CH '-'
#define LDG_PROTO_CW_CHAR_SEP ' '
#define LDG_PROTO_CW_WORD_SEP '/'
#define LDG_PROTO_CW_PROSIGN_OPEN '<'
#define LDG_PROTO_CW_PROSIGN_CLOSE '>'

#define LDG_PROTO_CW_PARIS_WORD_DIT 50
#define LDG_PROTO_CW_ELEM_GAP_DIT 1
#define LDG_PROTO_CW_DAH_DIT 3
#define LDG_PROTO_CW_CHAR_GAP_DIT 3
#define LDG_PROTO_CW_WORD_GAP_DIT 7
#define LDG_PROTO_CW_DAH_THRESHOLD_DIT 2
#define LDG_PROTO_CW_WORD_THRESHOLD_DIT 5

typedef struct ldg_proto_cw_timing
{
    uint64_t dit;
    uint64_t gap_dit;
} ldg_proto_cw_timing_t;

typedef struct ldg_proto_cw_key
{
    uint64_t dur;
    uint8_t on;
    uint8_t pudding[7];
} ldg_proto_cw_key_t;

typedef struct ldg_proto_cw_sym
{
    char text[LDG_PROTO_CW_TEXT_MAX];
    char pattern[LDG_PROTO_CW_ELEM_MAX + LDG_STR_TERM_SIZE];
} ldg_proto_cw_sym_t;

LDG_EXPORT extern const ldg_proto_cw_sym_t LDG_PROTO_CW_TABLE[LDG_PROTO_CW_SYM_CUNT];

LDG_EXPORT uint64_t ldg_proto_cw_dit_get(uint32_t wpm);
LDG_EXPORT uint32_t ldg_proto_cw_text_to_pattern_convert(const char *text, char *out, uint64_t out_size, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_proto_cw_pattern_to_text_convert(const char *pattern, char *out, uint64_t out_size, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_proto_cw_pattern_to_key_convert(const char *pattern, const ldg_proto_cw_timing_t *timing, ldg_proto_cw_key_t *out, uint64_t cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_proto_cw_key_to_pattern_convert(const ldg_proto_cw_key_t *key, uint64_t cunt, const ldg_proto_cw_timing_t *timing, char *out, uint64_t out_size, uint64_t *out_len);

#endif
