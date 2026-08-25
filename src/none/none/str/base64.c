#include <stdint.h>

#include <dangling/str/str.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_STR_BASE64_UPPER_MAX 25
#define LDG_STR_BASE64_LOWER_BASE 26
#define LDG_STR_BASE64_LOWER_MAX 51
#define LDG_STR_BASE64_DIGIT_BASE 52
#define LDG_STR_BASE64_DIGIT_MAX 61
#define LDG_STR_BASE64_PLUS_VAL 62
#define LDG_STR_BASE64_SLASH_VAL 63
#define LDG_STR_BASE64_GROUP_IN 3
#define LDG_STR_BASE64_GROUP_OUT 4
#define LDG_STR_BASE64_BITS 6
#define LDG_STR_BASE64_MASK 0x3F
#define LDG_STR_BASE64_BYTE_BITS 8
#define LDG_STR_BASE64_BYTE_MASK 0xFF
#define LDG_STR_BASE64_PAIR 2
#define LDG_STR_BASE64_PAD '='
#define LDG_STR_BASE64_DIGIT_LOW '0'
#define LDG_STR_BASE64_DIGIT_HIGH '9'
#define LDG_STR_BASE64_INVALID UINT8_MAX

static uint8_t str_base64_sym_get(uint8_t val)
{
    if (val <= LDG_STR_BASE64_UPPER_MAX) { return (uint8_t)('A' + val); }

    if (val <= LDG_STR_BASE64_LOWER_MAX) { return (uint8_t)('a' + (val - LDG_STR_BASE64_LOWER_BASE)); }

    if (val <= LDG_STR_BASE64_DIGIT_MAX) { return (uint8_t)(LDG_STR_BASE64_DIGIT_LOW + (val - LDG_STR_BASE64_DIGIT_BASE)); }

    if (val == LDG_STR_BASE64_PLUS_VAL) { return (uint8_t)'+'; }

    return (uint8_t)'/';
}

static uint8_t str_base64_val_get(uint8_t sym)
{
    if (sym >= 'A' && sym <= 'Z') { return (uint8_t)(sym - 'A'); }

    if (sym >= 'a' && sym <= 'z') { return (uint8_t)((sym - 'a') + LDG_STR_BASE64_LOWER_BASE); }

    if (sym >= LDG_STR_BASE64_DIGIT_LOW && sym <= LDG_STR_BASE64_DIGIT_HIGH) { return (uint8_t)((sym - LDG_STR_BASE64_DIGIT_LOW) + LDG_STR_BASE64_DIGIT_BASE); }

    if (sym == '+') { return LDG_STR_BASE64_PLUS_VAL; }

    if (sym == '/') { return LDG_STR_BASE64_SLASH_VAL; }

    return LDG_STR_BASE64_INVALID;
}

uint32_t ldg_str_base64_encode(const uint8_t *raw, uint64_t len, char *out, uint64_t cap, uint64_t *out_len)
{
    uint64_t idx = 0;
    uint64_t written = 0;
    uint32_t blk = 0;
    uint32_t span = 0;
    uint32_t part = 0;

    if (LDG_UNLIKELY(!raw || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (idx < len)
    {
        blk = 0;
        span = 0;
        part = 0;

        while (span < LDG_STR_BASE64_GROUP_IN && idx < len)
        {
            blk = (blk << LDG_STR_BASE64_BYTE_BITS) | raw[idx];
            idx++;
            span++;
        }

        while (span < LDG_STR_BASE64_GROUP_IN)
        {
            blk = blk << LDG_STR_BASE64_BYTE_BITS;
            span++;
            part++;
        }

        for (span = 0; span < LDG_STR_BASE64_GROUP_OUT; span++)
        {
            if (LDG_UNLIKELY(written >= cap)) { return LDG_ERR_FULL; }

            if (span + part >= LDG_STR_BASE64_GROUP_OUT) { out[written] = LDG_STR_BASE64_PAD; }
            else { out[written] = (char)str_base64_sym_get((uint8_t)((blk >> ((LDG_STR_BASE64_GROUP_IN - span) * LDG_STR_BASE64_BITS)) & LDG_STR_BASE64_MASK)); }

            written++;
        }
    }

    if (LDG_UNLIKELY(written >= cap)) { return LDG_ERR_FULL; }

    out[written] = LDG_STR_TERM;
    *out_len = written;

    return LDG_ERR_AOK;
}

uint32_t ldg_str_base64_decode(const char *text, uint64_t len, uint8_t *out, uint64_t cap, uint64_t *out_len)
{
    uint64_t idx = 0;
    uint64_t written = 0;
    uint32_t blk = 0;
    uint32_t span = 0;
    uint8_t val = 0;

    if (LDG_UNLIKELY(!text || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < len; idx++)
    {
        if (text[idx] == LDG_STR_BASE64_PAD) { break; }

        val = str_base64_val_get((uint8_t)text[idx]);
        if (val == LDG_STR_BASE64_INVALID) { continue; }

        blk = (blk << LDG_STR_BASE64_BITS) | val;
        span++;

        if (span == LDG_STR_BASE64_GROUP_OUT)
        {
            if (LDG_UNLIKELY(written + LDG_STR_BASE64_GROUP_IN > cap)) { return LDG_ERR_FULL; }

            out[written] = (uint8_t)((blk >> (LDG_STR_BASE64_BYTE_BITS * LDG_STR_BASE64_PAIR)) & LDG_STR_BASE64_BYTE_MASK);
            written++;
            out[written] = (uint8_t)((blk >> LDG_STR_BASE64_BYTE_BITS) & LDG_STR_BASE64_BYTE_MASK);
            written++;
            out[written] = (uint8_t)(blk & LDG_STR_BASE64_BYTE_MASK);
            written++;

            blk = 0;
            span = 0;
        }
    }

    if (span == LDG_STR_BASE64_GROUP_IN)
    {
        if (LDG_UNLIKELY(written + LDG_STR_BASE64_PAIR > cap)) { return LDG_ERR_FULL; }

        blk = blk << LDG_STR_BASE64_BITS;
        out[written] = (uint8_t)((blk >> (LDG_STR_BASE64_BYTE_BITS * LDG_STR_BASE64_PAIR)) & LDG_STR_BASE64_BYTE_MASK);
        written++;
        out[written] = (uint8_t)((blk >> LDG_STR_BASE64_BYTE_BITS) & LDG_STR_BASE64_BYTE_MASK);
        written++;
    }
    else if (span == LDG_STR_BASE64_PAIR)
    {
        if (LDG_UNLIKELY(written >= cap)) { return LDG_ERR_FULL; }

        blk = blk << (LDG_STR_BASE64_BITS * LDG_STR_BASE64_PAIR);
        out[written] = (uint8_t)((blk >> (LDG_STR_BASE64_BYTE_BITS * LDG_STR_BASE64_PAIR)) & LDG_STR_BASE64_BYTE_MASK);
        written++;
    }

    *out_len = written;

    return LDG_ERR_AOK;
}
