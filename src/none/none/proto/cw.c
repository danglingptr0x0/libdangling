#include <stdint.h>

#include <dangling/proto/cw.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define CW_ASCII_LOWER_FIRST 'a'
#define CW_ASCII_LOWER_LAST 'z'
#define CW_ASCII_CASE_DELTA ('a' - 'A')
#define CW_GAP_NONE 0

const ldg_proto_cw_sym_t LDG_PROTO_CW_TABLE[LDG_PROTO_CW_SYM_CUNT] =
{
    { "A", ".-" },
    { "B", "-..." },
    { "C", "-.-." },
    { "D", "-.." },
    { "E", "." },
    { "F", "..-." },
    { "G", "--." },
    { "H", "...." },
    { "I", ".." },
    { "J", ".---" },
    { "K", "-.-" },
    { "L", ".-.." },
    { "M", "--" },
    { "N", "-." },
    { "O", "---" },
    { "P", ".--." },
    { "Q", "--.-" },
    { "R", ".-." },
    { "S", "..." },
    { "T", "-" },
    { "U", "..-" },
    { "V", "...-" },
    { "W", ".--" },
    { "X", "-..-" },
    { "Y", "-.--" },
    { "Z", "--.." },
    { "0", "-----" },
    { "1", ".----" },
    { "2", "..---" },
    { "3", "...--" },
    { "4", "....-" },
    { "5", "....." },
    { "6", "-...." },
    { "7", "--..." },
    { "8", "---.." },
    { "9", "----." },
    { ".", ".-.-.-" },
    { ",", "--..--" },
    { ":", "---..." },
    { "?", "..--.." },
    { "'", ".----." },
    { "-", "-....-" },
    { "/", "-..-." },
    { "(", "-.--." },
    { ")", "-.--.-" },
    { "\"", ".-..-." },
    { "=", "-...-" },
    { "+", ".-.-." },
    { "@", ".--.-." },
    { "<AR>", ".-.-." },
    { "<SK>", "...-.-" },
    { "<BT>", "-...-" },
    { "<KN>", "-.--." },
    { "<AS>", ".-..." },
    { "<SOS>", "...---..." },
    { "<HH>", "........" }
};

static char cw_upper_get(char ch)
{
    if (ch >= CW_ASCII_LOWER_FIRST && ch <= CW_ASCII_LOWER_LAST) { return (char)(ch - CW_ASCII_CASE_DELTA); }

    return ch;
}

static uint8_t cw_span_eq_is(const char *lhs, uint64_t len, const char *rhs)
{
    uint64_t idx = 0;

    while (idx < len)
    {
        if (rhs[idx] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

        if (lhs[idx] != rhs[idx]) { return LDG_TRUTH_FALSE; }

        idx++;
    }

    if (rhs[len] != LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t cw_sym_by_text_get(const char *text, uint64_t len, uint32_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!text || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (len >= LDG_PROTO_CW_TEXT_MAX) { return LDG_ERR_NOT_FOUND; }

    while (idx < LDG_PROTO_CW_SYM_CUNT)
    {
        if (cw_span_eq_is(text, len, LDG_PROTO_CW_TABLE[idx].text) == LDG_TRUTH_TRUE)
        {
            *out = idx;
            return LDG_ERR_AOK;
        }

        idx++;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t cw_sym_by_pattern_get(const char *pattern, uint64_t len, uint32_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pattern || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (len > LDG_PROTO_CW_ELEM_MAX) { return LDG_ERR_NOT_FOUND; }

    while (idx < LDG_PROTO_CW_SYM_CUNT)
    {
        if (cw_span_eq_is(pattern, len, LDG_PROTO_CW_TABLE[idx].pattern) == LDG_TRUTH_TRUE)
        {
            *out = idx;
            return LDG_ERR_AOK;
        }

        idx++;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t cw_ch_append(char *out, uint64_t out_size, uint64_t *pos, char ch)
{
    if (LDG_UNLIKELY(*pos + LDG_STR_TERM_SIZE >= out_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    out[*pos] = ch;
    (*pos)++;
    out[*pos] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t cw_str_append(char *out, uint64_t out_size, uint64_t *pos, const char *str)
{
    uint64_t idx = 0;
    uint32_t ret = 0;

    while (str[idx] != LDG_STR_TERM)
    {
        ret = cw_ch_append(out, out_size, pos, str[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        idx++;
    }

    return LDG_ERR_AOK;
}

static uint32_t cw_word_sep_append(char *out, uint64_t out_size, uint64_t *pos)
{
    uint32_t ret = 0;

    ret = cw_ch_append(out, out_size, pos, LDG_PROTO_CW_WORD_SEP);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return cw_ch_append(out, out_size, pos, LDG_PROTO_CW_CHAR_SEP);
}

static uint32_t cw_key_append(ldg_proto_cw_key_t *out, uint64_t cap, uint64_t *cunt, uint8_t on, uint64_t dur)
{
    ldg_proto_cw_key_t entry = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(*cunt >= cap)) { return LDG_ERR_FULL; }

    entry.dur = dur;
    entry.on = on;
    out[*cunt] = entry;
    (*cunt)++;

    return LDG_ERR_AOK;
}

uint64_t ldg_proto_cw_dit_get(uint32_t wpm)
{
    if (LDG_UNLIKELY(wpm == 0)) { return UINT64_MAX; }

    return ((uint64_t)LDG_SECS_PER_MIN * (uint64_t)LDG_NS_PER_SEC) / ((uint64_t)LDG_PROTO_CW_PARIS_WORD_DIT * (uint64_t)wpm);
}

uint32_t ldg_proto_cw_text_to_pattern_convert(const char *text, char *out, uint64_t out_size, uint64_t *out_len)
{
    char sym_text[LDG_PROTO_CW_TEXT_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t pos = 0;
    uint64_t idx = 0;
    uint64_t span = 0;
    uint32_t sym_idx = 0;
    uint32_t ret = 0;
    uint8_t wrote_is = LDG_TRUTH_FALSE;
    uint8_t word_break_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!text || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    while (text[idx] != LDG_STR_TERM)
    {
        if (text[idx] == LDG_PROTO_CW_CHAR_SEP)
        {
            if (wrote_is == LDG_TRUTH_TRUE) { word_break_is = LDG_TRUTH_TRUE; }

            idx++;
            continue;
        }

        span = 0;

        if (text[idx] == LDG_PROTO_CW_PROSIGN_OPEN)
        {
            sym_text[span] = LDG_PROTO_CW_PROSIGN_OPEN;
            span++;
            idx++;

            while (span < LDG_PROTO_CW_TEXT_MAX)
            {
                if (LDG_UNLIKELY(text[idx] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_INVALID; }

                sym_text[span] = cw_upper_get(text[idx]);
                span++;
                idx++;

                if (sym_text[span - LDG_STR_TERM_SIZE] == LDG_PROTO_CW_PROSIGN_CLOSE) { break; }
            }
        }
        else
        {
            sym_text[span] = cw_upper_get(text[idx]);
            span++;
            idx++;
        }

        ret = cw_sym_by_text_get(sym_text, span, &sym_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (wrote_is == LDG_TRUTH_TRUE)
        {
            ret = cw_ch_append(out, out_size, &pos, LDG_PROTO_CW_CHAR_SEP);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (word_break_is == LDG_TRUTH_TRUE)
            {
                ret = cw_word_sep_append(out, out_size, &pos);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        word_break_is = LDG_TRUTH_FALSE;

        ret = cw_str_append(out, out_size, &pos, LDG_PROTO_CW_TABLE[sym_idx].pattern);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        wrote_is = LDG_TRUTH_TRUE;
    }

    *out_len = pos;

    return LDG_ERR_AOK;
}

uint32_t ldg_proto_cw_pattern_to_text_convert(const char *pattern, char *out, uint64_t out_size, uint64_t *out_len)
{
    uint64_t pos = 0;
    uint64_t idx = 0;
    uint64_t span = 0;
    uint32_t sym_idx = 0;
    uint32_t ret = 0;
    uint8_t wrote_is = LDG_TRUTH_FALSE;
    uint8_t word_break_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!pattern || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    while (pattern[idx] != LDG_STR_TERM)
    {
        if (pattern[idx] == LDG_PROTO_CW_CHAR_SEP)
        {
            idx++;
            continue;
        }

        if (pattern[idx] == LDG_PROTO_CW_WORD_SEP)
        {
            if (wrote_is == LDG_TRUTH_TRUE) { word_break_is = LDG_TRUTH_TRUE; }

            idx++;
            continue;
        }

        span = 0;

        while (pattern[idx + span] == LDG_PROTO_CW_DIT_CH || pattern[idx + span] == LDG_PROTO_CW_DAH_CH) { span++; }

        if (LDG_UNLIKELY(span == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        ret = cw_sym_by_pattern_get(&pattern[idx], span, &sym_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (word_break_is == LDG_TRUTH_TRUE)
        {
            ret = cw_ch_append(out, out_size, &pos, LDG_PROTO_CW_CHAR_SEP);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        word_break_is = LDG_TRUTH_FALSE;

        ret = cw_str_append(out, out_size, &pos, LDG_PROTO_CW_TABLE[sym_idx].text);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        wrote_is = LDG_TRUTH_TRUE;
        idx += span;
    }

    *out_len = pos;

    return LDG_ERR_AOK;
}

uint32_t ldg_proto_cw_pattern_to_key_convert(const char *pattern, const ldg_proto_cw_timing_t *timing, ldg_proto_cw_key_t *out, uint64_t cap, uint64_t *out_cunt)
{
    uint64_t idx = 0;
    uint64_t pending_gap = CW_GAP_NONE;
    uint64_t char_gap = 0;
    uint64_t word_gap = 0;
    uint64_t gap = 0;
    uint64_t dur = 0;
    uint32_t ret = 0;
    uint8_t wrote_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!pattern || !timing || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(timing->dit == 0 || timing->gap_dit == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    char_gap = timing->gap_dit * LDG_PROTO_CW_CHAR_GAP_DIT;
    word_gap = timing->gap_dit * LDG_PROTO_CW_WORD_GAP_DIT;

    while (pattern[idx] != LDG_STR_TERM)
    {
        if (pattern[idx] == LDG_PROTO_CW_CHAR_SEP)
        {
            if (wrote_is == LDG_TRUTH_TRUE && pending_gap < char_gap) { pending_gap = char_gap; }

            idx++;
            continue;
        }

        if (pattern[idx] == LDG_PROTO_CW_WORD_SEP)
        {
            if (wrote_is == LDG_TRUTH_TRUE) { pending_gap = word_gap; }

            idx++;
            continue;
        }

        if (LDG_UNLIKELY(pattern[idx] != LDG_PROTO_CW_DIT_CH && pattern[idx] != LDG_PROTO_CW_DAH_CH)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (wrote_is == LDG_TRUTH_TRUE)
        {
            gap = pending_gap;
            if (gap == CW_GAP_NONE) { gap = timing->dit * LDG_PROTO_CW_ELEM_GAP_DIT; }

            ret = cw_key_append(out, cap, out_cunt, LDG_TRUTH_FALSE, gap);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        dur = timing->dit;
        if (pattern[idx] == LDG_PROTO_CW_DAH_CH) { dur = timing->dit * LDG_PROTO_CW_DAH_DIT; }

        ret = cw_key_append(out, cap, out_cunt, LDG_TRUTH_TRUE, dur);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        pending_gap = CW_GAP_NONE;
        wrote_is = LDG_TRUTH_TRUE;
        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_proto_cw_key_to_pattern_convert(const ldg_proto_cw_key_t *key, uint64_t cunt, const ldg_proto_cw_timing_t *timing, char *out, uint64_t out_size, uint64_t *out_len)
{
    uint64_t pos = 0;
    uint64_t idx = 0;
    uint64_t dah_min = 0;
    uint64_t word_min = 0;
    uint32_t ret = 0;
    uint8_t wrote_is = LDG_TRUTH_FALSE;
    uint8_t char_break_is = LDG_TRUTH_FALSE;
    uint8_t word_break_is = LDG_TRUTH_FALSE;
    char elem = LDG_PROTO_CW_DIT_CH;

    if (LDG_UNLIKELY(!key || !timing || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    if (LDG_UNLIKELY(timing->dit == 0 || timing->gap_dit == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dah_min = timing->dit * LDG_PROTO_CW_DAH_THRESHOLD_DIT;
    word_min = timing->gap_dit * LDG_PROTO_CW_WORD_THRESHOLD_DIT;

    while (idx < cunt)
    {
        if (key[idx].on == LDG_TRUTH_TRUE)
        {
            if (char_break_is == LDG_TRUTH_TRUE && wrote_is == LDG_TRUTH_TRUE)
            {
                ret = cw_ch_append(out, out_size, &pos, LDG_PROTO_CW_CHAR_SEP);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                if (word_break_is == LDG_TRUTH_TRUE)
                {
                    ret = cw_word_sep_append(out, out_size, &pos);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }
            }

            char_break_is = LDG_TRUTH_FALSE;
            word_break_is = LDG_TRUTH_FALSE;

            elem = LDG_PROTO_CW_DIT_CH;
            if (key[idx].dur >= dah_min) { elem = LDG_PROTO_CW_DAH_CH; }

            ret = cw_ch_append(out, out_size, &pos, elem);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            wrote_is = LDG_TRUTH_TRUE;
            idx++;
            continue;
        }

        if (key[idx].dur >= dah_min) { char_break_is = LDG_TRUTH_TRUE; }

        if (key[idx].dur >= word_min) { word_break_is = LDG_TRUTH_TRUE; }

        idx++;
    }

    *out_len = pos;

    return LDG_ERR_AOK;
}
