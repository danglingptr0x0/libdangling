#include <dangling/str/str.h>
#include <dangling/core/err.h>
#include <dangling/core/arith.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/secure.h>

static uint32_t str_digit_val_get(char character, uint32_t base, uint32_t *out)
{
    uint32_t val = UINT32_MAX;

    if (ldg_char_digit_is((uint8_t)character) == LDG_TRUTH_TRUE) { val = (uint32_t)(character - LDG_ASCII_DIGIT_ZERO); }
    else if (ldg_char_lower_is((uint8_t)character) == LDG_TRUTH_TRUE) { val = (uint32_t)(character - 'a') + LDG_BASE_DEC; }
    else if (ldg_char_upper_is((uint8_t)character) == LDG_TRUTH_TRUE) { val = (uint32_t)(character - 'A') + LDG_BASE_DEC; }

    if (val >= base) { return LDG_ERR_STR_INVALID; }

    *out = val;

    return LDG_ERR_AOK;
}

uint32_t ldg_str_uint_parse(const char *str, uint32_t base, uint64_t max, uint64_t *out)
{
    uint64_t idx = 0;
    uint64_t acc = 0;
    uint32_t digit = 0;
    uint32_t seen = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(!str)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(base < LDG_STR_BASE_MIN || base > LDG_STR_BASE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (idx < max && ldg_char_space_is((uint8_t)str[idx]) == LDG_TRUTH_TRUE) { idx++; }

    while (idx < max && str[idx] != LDG_STR_TERM)
    {
        ret = str_digit_val_get(str[idx], base, &digit);
        if (ret != LDG_ERR_AOK) { break; }

        if (LDG_UNLIKELY(acc > (UINT64_MAX - digit) / base)) { return LDG_ERR_OVERFLOW; }

        acc = (acc * base) + digit;
        seen++;
        idx++;
    }

    if (seen == 0) { return LDG_ERR_STR_INVALID; }

    *out = acc;

    return LDG_ERR_AOK;
}

uint32_t ldg_str_int64_parse(const char *str, uint32_t base, uint64_t max, int64_t *out)
{
    uint64_t idx = 0;
    uint64_t magnitude = 0;
    uint64_t limit = 0;
    uint8_t neg = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(!str)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (idx < max && ldg_char_space_is((uint8_t)str[idx]) == LDG_TRUTH_TRUE) { idx++; }

    if (idx < max && (str[idx] == '-' || str[idx] == '+'))
    {
        if (str[idx] == '-') { neg = LDG_TRUTH_TRUE; }

        idx++;
    }

    ret = ldg_str_uint_parse(str + idx, base, max - idx, &magnitude);
    if (ret != LDG_ERR_AOK) { return ret; }

    limit = (uint64_t)INT64_MAX;
    if (neg == LDG_TRUTH_TRUE) { limit = (uint64_t)INT64_MAX + LDG_STR_TERM_SIZE; }

    if (LDG_UNLIKELY(magnitude > limit)) { return LDG_ERR_OVERFLOW; }

    if (neg == LDG_TRUTH_TRUE)
    {
        *out = -(int64_t)(magnitude - LDG_STR_TERM_SIZE) - LDG_STR_TERM_SIZE;

        return LDG_ERR_AOK;
    }

    *out = (int64_t)magnitude;

    return LDG_ERR_AOK;
}

uint32_t ldg_str_len_get(const char *str, uint64_t max, uint64_t *out)
{
    uint64_t len = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(!str)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (len < max && str[len] != LDG_STR_TERM) { len++; }

    *out = len;

    if (len >= max) { return LDG_ERR_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t ldg_str_alloc_cpy(const char *src, uint64_t max, char **out)
{
    void *raw = 0x0;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_str_len_get(src, max, &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_alloc(len + LDG_STR_TERM_SIZE, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ldg_mem_cpy(raw, src, len) != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(raw);

        return LDG_ERR_MEM_BAD;
    }

    ((char *)raw)[len] = LDG_STR_TERM;

    *out = (char *)raw;

    return LDG_ERR_AOK;
}

uint32_t ldg_str_concatenate(char *dst, const char *src, uint64_t cap)
{
    uint64_t dst_len = 0;
    uint64_t src_idx = 0;

    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (dst_len < cap && dst[dst_len] != LDG_STR_TERM) { dst_len++; }

    if (LDG_UNLIKELY(dst_len >= cap)) { return LDG_ERR_STR_INVALID; }

    while (src[src_idx] != LDG_STR_TERM && (dst_len + src_idx + LDG_STR_TERM_SIZE) < cap)
    {
        dst[dst_len + src_idx] = src[src_idx];
        src_idx++;
    }

    dst[dst_len + src_idx] = LDG_STR_TERM;

    if (src[src_idx] != LDG_STR_TERM) { return LDG_ERR_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t ldg_str_char_find(const char *str, char tgt, uint64_t max, uint64_t *out_idx)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!out_idx)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT64_MAX;

    if (LDG_UNLIKELY(!str)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (idx < max && str[idx] != LDG_STR_TERM)
    {
        if (str[idx] == tgt)
        {
            *out_idx = idx;

            return LDG_ERR_AOK;
        }

        idx++;
    }

    if (tgt == LDG_STR_TERM && idx < max)
    {
        *out_idx = idx;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_str_char_last_find(const char *str, char tgt, uint64_t max, uint64_t *out_idx)
{
    uint64_t idx = 0;
    uint64_t found = UINT64_MAX;

    if (LDG_UNLIKELY(!out_idx)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT64_MAX;

    if (LDG_UNLIKELY(!str)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (idx < max && str[idx] != LDG_STR_TERM)
    {
        if (str[idx] == tgt) { found = idx; }

        idx++;
    }

    if (tgt == LDG_STR_TERM && idx < max) { found = idx; }

    if (found == UINT64_MAX) { return LDG_ERR_NOT_FOUND; }

    *out_idx = found;

    return LDG_ERR_AOK;
}

uint32_t ldg_str_substring_find(const char *text, const char *pattern, uint64_t max, uint64_t *out_idx)
{
    uint64_t text_len = 0;
    uint64_t pattern_len = 0;
    uint64_t start = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!out_idx)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT64_MAX;

    if (LDG_UNLIKELY(!text || !pattern)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (text_len < max && text[text_len] != LDG_STR_TERM) { text_len++; }

    while (pattern[pattern_len] != LDG_STR_TERM) { pattern_len++; }

    if (pattern_len == 0)
    {
        *out_idx = 0;

        return LDG_ERR_AOK;
    }

    if (pattern_len > text_len) { return LDG_ERR_NOT_FOUND; }

    for (start = 0; start <= (text_len - pattern_len); start++)
    {
        for (idx = 0; idx < pattern_len; idx++) { if (text[start + idx] != pattern[idx]) { break; } }

        if (idx == pattern_len)
        {
            *out_idx = start;

            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_str_cmp(const char *lhs, const char *rhs, uint64_t max, uint32_t *out)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_ARITH_ORD_EQ;

    if (LDG_UNLIKELY(!lhs || !rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < max; idx++)
    {
        if ((uint8_t)lhs[idx] < (uint8_t)rhs[idx])
        {
            *out = LDG_ARITH_ORD_LT;

            return LDG_ERR_AOK;
        }

        if ((uint8_t)lhs[idx] > (uint8_t)rhs[idx])
        {
            *out = LDG_ARITH_ORD_GT;

            return LDG_ERR_AOK;
        }

        if (lhs[idx] == LDG_STR_TERM) { return LDG_ERR_AOK; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_str_case_cmp(const char *lhs, const char *rhs, uint64_t max, uint32_t *out)
{
    uint64_t idx = 0;
    char lhs_fold = 0;
    char rhs_fold = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_ARITH_ORD_EQ;

    if (LDG_UNLIKELY(!lhs || !rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < max; idx++)
    {
        lhs_fold = ldg_char_lower_get(lhs[idx]);
        rhs_fold = ldg_char_lower_get(rhs[idx]);

        if ((uint8_t)lhs_fold < (uint8_t)rhs_fold)
        {
            *out = LDG_ARITH_ORD_LT;

            return LDG_ERR_AOK;
        }

        if ((uint8_t)lhs_fold > (uint8_t)rhs_fold)
        {
            *out = LDG_ARITH_ORD_GT;

            return LDG_ERR_AOK;
        }

        if (lhs_fold == LDG_STR_TERM) { return LDG_ERR_AOK; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_str_tok_get(const char *str, const char *delimiter, uint64_t *cursor, uint64_t max, uint64_t *out_start, uint64_t *out_len)
{
    uint64_t pos = 0;
    uint64_t begin = 0;
    uint64_t hit = UINT64_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out_start || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_start = 0;
    *out_len = 0;

    if (LDG_UNLIKELY(!str || !delimiter || !cursor)) { return LDG_ERR_FUNC_ARG_NULL; }

    pos = *cursor;

    while (pos < max && str[pos] != LDG_STR_TERM)
    {
        ret = ldg_str_char_find(delimiter, str[pos], max, &hit);

        if (ret != LDG_ERR_AOK) { break; }

        pos++;
    }

    if (pos >= max || str[pos] == LDG_STR_TERM)
    {
        *cursor = pos;

        return LDG_ERR_EOF;
    }

    begin = pos;

    while (pos < max && str[pos] != LDG_STR_TERM)
    {
        ret = ldg_str_char_find(delimiter, str[pos], max, &hit);

        if (ret == LDG_ERR_AOK) { break; }

        pos++;
    }

    *out_start = begin;
    *out_len = pos - begin;
    *cursor = pos;

    return LDG_ERR_AOK;
}

static void str_overlap_copy(char *dst, const char *src, uint64_t len)
{
    uint64_t idx = 0;

    if (dst < src)
    {
        for (idx = 0; idx < len; idx++) { dst[idx] = src[idx]; }

        return;
    }

    idx = len;
    while (idx > 0)
    {
        idx--;
        dst[idx] = src[idx];
    }
}

uint32_t ldg_str_rbrcpy(char *dst, const char *src, uint64_t abssize)
{
    uint64_t src_len = 0;
    uint8_t overlap = 0;

    if (LDG_UNLIKELY(!dst || !src || abssize == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (src_len < abssize && src[src_len] != LDG_STR_TERM && (uint8_t)src[src_len] <= LDG_ASCII_MAX) { src_len++; }
    if (LDG_UNLIKELY(src_len >= abssize)) { return LDG_ERR_STR_TRUNC; }

    overlap = (dst < src + src_len) && (src < dst + abssize);

    if (overlap) { str_overlap_copy(dst, src, src_len); }
    else { if (ldg_mem_secure_cpy(dst, src, src_len) != LDG_ERR_AOK) { return LDG_ERR_MEM_BAD; } }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(dst + src_len, abssize - src_len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    return overlap ? LDG_ERR_STR_OVERLAP : LDG_ERR_AOK;
}

uint32_t ldg_byte_to_hex(byte_t val, char out[3])
{
    const char *diggies = "0123456789ABCDEF";

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = diggies[(val >> 4) & 0xF];
    out[1] = diggies[val & 0xF];
    out[2] = '\0';

    return LDG_ERR_AOK;
}

uint32_t ldg_dword_to_hex(dword_t val, char *buff)
{
    uint32_t i = 0;
    byte_t nipple = 0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (i = 0; i < 8; i++)
    {
        nipple = (val >> ((7 - i) * 4)) & 0xF;
        buff[i] = (char)((nipple < 10) ? ('0' + nipple) : ('A' + nipple - 10));
    }

    buff[8] = '\0';

    return LDG_ERR_AOK;
}

uint32_t ldg_str_to_dec(const char *str, dword_t *out)
{
    dword_t tmp = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!str || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    while (*str)
    {
        if (*str < '0' || *str > '9') { *out = 0; return LDG_ERR_FUNC_ARG_INVALID; }

        ret = ldg_arith_uint32_mul(*out, 10, &tmp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { *out = 0; return LDG_ERR_OVERFLOW; }

        ret = ldg_arith_uint32_add(tmp, (dword_t)(*str - '0'), out);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { *out = 0; return LDG_ERR_OVERFLOW; }

        str++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_hex_to_nipple(char c, byte_t *nipple)
{
    if (LDG_UNLIKELY(!nipple)) { return LDG_ERR_FUNC_ARG_NULL; }

    *nipple = 0;

    if (c >= '0' && c <= '9') { *nipple = (byte_t)(c - '0'); }
    else if (c >= 'A' && c <= 'F') { *nipple = (byte_t)(c - 'A' + 10); }
    else if (c >= 'a' && c <= 'f') { *nipple = (byte_t)(c - 'a' + 10); }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_hex_to_dword(const char *str, dword_t *out)
{
    byte_t nipple = 0;
    uint32_t digit_cunt = 0;

    if (LDG_UNLIKELY(!str || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) { str += 2; }

    while (*str)
    {
        if (LDG_UNLIKELY(ldg_hex_to_nipple(*str, &nipple) != LDG_ERR_AOK)) { *out = 0; return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(digit_cunt >= 8)) { *out = 0; return LDG_ERR_OVERFLOW; }

        *out = (*out << 4) | nipple;
        digit_cunt++;
        str++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_hex_to_bytes(const char *hex, byte_t *out, uint64_t max)
{
    uint64_t i = 0;
    byte_t high = 0;
    byte_t low = 0;

    if (LDG_UNLIKELY(!hex || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (*hex && *(hex + 1) && i < max)
    {
        if (ldg_hex_to_nipple(*hex, &high) != LDG_ERR_AOK) { break; }

        if (ldg_hex_to_nipple(*(hex + 1), &low) != LDG_ERR_AOK) { break; }

        out[i++] = (uint8_t)((high << 4) | low);
        hex += 2;
    }

    while (i < max) { out[i++] = 0; }

    return LDG_ERR_AOK;
}

uint8_t ldg_hex_str_is(const char *str)
{
    byte_t nipple = 0;

    if (LDG_UNLIKELY(!str)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (str[0] != '0' || (str[1] != 'x' && str[1] != 'X')) { return LDG_TRUTH_FALSE; }

    if (!str[2]) { return LDG_TRUTH_FALSE; }

    str += 2;

    while (*str)
    {
        if (ldg_hex_to_nipple(*str, &nipple) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

        str++;
    }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_hash_fnv1a_32_compute(const uint8_t *data, uint64_t len, uint32_t *out)
{
    uint32_t hash = LDG_FNV1A_32_OFFSET;
    uint64_t idx = 0;

    if (!data || !out) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    for (idx = 0; idx < len; idx++)
    {
        hash ^= data[idx];
        hash *= LDG_FNV1A_32_PRIME;
    }

    *out = hash;

    return LDG_ERR_AOK;
}

uint32_t ldg_hash_fnv1a_64_compute(const uint8_t *data, uint64_t len, uint64_t *out)
{
    uint64_t hash = LDG_FNV1A_64_OFFSET;
    uint64_t idx = 0;

    if (!data || !out) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    for (idx = 0; idx < len; idx++)
    {
        hash ^= data[idx];
        hash *= LDG_FNV1A_64_PRIME;
    }

    *out = hash;

    return LDG_ERR_AOK;
}
