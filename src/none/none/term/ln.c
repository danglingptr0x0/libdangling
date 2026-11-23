#include <stdint.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>
#include <dangling/mem/cpy.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>
#include <dangling/term/ln.h>

#define TERM_LN_POOL_ALLOC_CUNT 7
#define TERM_LN_TEXT_BUFF_CUNT 5
#define TERM_LN_PREV_OFF 1
#define TERM_LN_RING_STEP 1
#define TERM_LN_CANDIDATE_SOLE 1
#define TERM_LN_HIST_NEWEST 0
#define TERM_LN_POS_FIRST 0
#define TERM_LN_CH_MIN 0x20
#define TERM_LN_CH_MAX 0x7E
#define TERM_LN_CASE_OFF 0x20
#define TERM_LN_UPPER_FIRST 'A'
#define TERM_LN_UPPER_LAST 'Z'
#define TERM_LN_CTRL_OFF 0x60
#define TERM_LN_KEY_SEARCH 'r'
#define TERM_LN_KEY_WORD_DROP 'w'
#define TERM_LN_KEY_CLEAR 'c'
#define TERM_LN_KEY_GHOST_WORD 'l'
#define TERM_LN_KEY_HOME 'a'
#define TERM_LN_KEY_END 'e'
#define TERM_LN_KEY_DROP_HD 'u'
#define TERM_LN_KEY_DROP_TAIL 'k'
#define TERM_LN_KEY_EOF 'd'
#define TERM_LN_KEY_SEARCH_CANCEL 'g'
#define TERM_LN_SEARCH_FMT "bck-i-search: %.170s"
#define TERM_LN_SEARCH_NONE "bck-i-search: no match"
#define TERM_LN_CANDIDATE_SEL_FMT "%u32 of %u32%s; tab cycles, shift+tab back"
#define TERM_LN_CANDIDATE_LIST_FMT "%u32%s candidates; tab cycles"
#define TERM_LN_MATCHES_FMT "%u32 matches; tab to complete"
#define TERM_LN_MORE_MARK "+"
#define TERM_LN_MORE_NONE ""
#define TERM_LN_VAR_LEAD '$'
#define TERM_LN_ADDR_LEAD_FIRST '0'
#define TERM_LN_ADDR_LEAD_LAST 'x'
#define TERM_LN_ADDR_LEAD_AT 1
#define TERM_LN_ASSIGN '='
#define TERM_LN_MEMBER_SEP '.'
#define TERM_LN_PATH_SEP '/'
#define TERM_LN_PATH_SEP_ALT '\\'
#define TERM_LN_CURSOR_WIDTH 1
#define TERM_LN_CANDIDATE_GAP 2
#define TERM_LN_HINT_GAP 1
#define TERM_LN_ROW_CUNT_LN 1
#define TERM_LN_COL_BASE 1
#define TERM_LN_OFF_NONE 0
#define TERM_LN_ENCODE_ROW_MAX 10
#define TERM_LN_SEQ_CAP 16
#define TERM_LN_SEQ_UP_FMT "\x1B[%u32A"
#define TERM_LN_SEQ_COL_FMT "\x1B[%u32G"
#define TERM_LN_ERASE_BELOW "\r\x1B[J"
#define TERM_LN_ROW_END "\r\n"
#define TERM_LN_WRAP_SETTLE "\x20\b"
#define TERM_LN_GHOST_ON "\x1B[90m"
#define TERM_LN_GHOST_OFF "\x1B[39m"
#define TERM_LN_SEL_ON "\x1B[7m"
#define TERM_LN_SEL_OFF "\x1B[27m"
#define TERM_LN_GAP "\x20\x20"

typedef struct term_ln_sink
{
    ldg_byte_t *out;
    uint64_t cap;
    uint64_t len;
} term_ln_sink_t;

static uint32_t term_ln_len_get(const char *text, uint32_t cap)
{
    uint32_t len = 0;

    while (len < cap && text[len] != LDG_STR_TERM) { len++; }

    return len;
}

static uint32_t term_ln_candidate_len_get(const char *text)
{
    uint32_t len = 0;

    while (len < LDG_TERM_LN_CANDIDATE_CAP && text[len] != LDG_STR_TERM && text[len] != LDG_TERM_LN_CANDIDATE_LABEL_SEP) { len++; }

    return len;
}

static uint32_t term_ln_word_start_get(const ldg_term_ln_t *ln, uint32_t pos)
{
    while (pos > 0 && ln->text[pos - TERM_LN_PREV_OFF] == LDG_ASCII_SPACE) { pos--; }

    while (pos > 0 && ln->text[pos - TERM_LN_PREV_OFF] != LDG_ASCII_SPACE) { pos--; }

    return pos;
}

static uint32_t term_ln_word_end_get(const ldg_term_ln_t *ln, uint32_t pos)
{
    while (pos < ln->len && ln->text[pos] == LDG_ASCII_SPACE) { pos++; }

    while (pos < ln->len && ln->text[pos] != LDG_ASCII_SPACE) { pos++; }

    return pos;
}

static uint32_t term_ln_span_swap(ldg_term_ln_t *ln, uint32_t start, uint32_t end, const char *text, uint32_t text_len)
{
    uint32_t tail = ln->len - end;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(start + text_len + tail + LDG_STR_TERM_SIZE > ln->cap)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_mov(ln->text + start + text_len, ln->text + end, (uint64_t)tail + LDG_STR_TERM_SIZE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (text_len > 0)
    {
        ret = ldg_mem_cpy(ln->text + start, text, text_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ln->len = start + text_len + tail;
    ln->cursor = start + text_len;

    return LDG_ERR_AOK;
}

static uint32_t term_ln_text_swap(ldg_term_ln_t *ln, const char *text)
{
    uint32_t ret = 0;

    ret = term_ln_span_swap(ln, TERM_LN_POS_FIRST, ln->len, text, term_ln_len_get(text, ln->cap));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ln->cursor = ln->len;

    return LDG_ERR_AOK;
}

static void term_ln_modes_close(ldg_term_ln_t *ln)
{
    ln->candidate_on = LDG_TRUTH_FALSE;
    ln->candidate_sel = LDG_TERM_LN_SEL_NONE;
    ln->candidate_cunt = 0;
    ln->search_on = LDG_TRUTH_FALSE;
    ln->search_sel = LDG_TERM_LN_SEL_NONE;
    ln->search_len = 0;
    ln->search[0] = LDG_STR_TERM;
}

static uint8_t term_ln_prefix_is(const char *text, const char *prefix, uint32_t prefix_len)
{
    uint32_t idx = 0;

    for (idx = 0; idx < prefix_len; idx++) { if (text[idx] == LDG_STR_TERM || text[idx] != prefix[idx]) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static uint32_t term_ln_hist_get(const ldg_term_ln_t *ln, uint32_t idx, const char **out)
{
    *out = 0x0;

    if (!ln->hist_cb) { return LDG_ERR_BOUNDS; }

    return ln->hist_cb(ln->hist_user, idx, out);
}

static uint32_t term_ln_hist_cunt_get(const ldg_term_ln_t *ln, uint32_t *out)
{
    const char *text = 0x0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    *out = 0;

    while (cunt < UINT32_MAX)
    {
        ret = term_ln_hist_get(ln, cunt, &text);
        if (ret == LDG_ERR_BOUNDS || (ret == LDG_ERR_AOK && !text)) { break; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        cunt++;
    }

    *out = cunt;

    return LDG_ERR_AOK;
}

static uint32_t term_ln_hist_match_get(const ldg_term_ln_t *ln, uint32_t idx, uint8_t *out_match)
{
    const char *text = 0x0;
    uint64_t at = 0;
    uint32_t ret = 0;

    *out_match = LDG_TRUTH_FALSE;

    ret = term_ln_hist_get(ln, idx, &text);
    if (ret == LDG_ERR_BOUNDS) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (!text) { return LDG_ERR_AOK; }

    ret = ldg_str_substring_find(text, ln->search, ln->cap, &at);
    if (ret == LDG_ERR_NOT_FOUND) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_match = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t term_ln_search_hint_set(ldg_term_ln_t *ln)
{
    const char *match = 0x0;
    uint64_t hint_len = 0;
    uint32_t ret = 0;

    if (ln->search_sel == LDG_TERM_LN_SEL_NONE) { return ldg_str_rbrcpy(ln->hint, TERM_LN_SEARCH_NONE, LDG_TERM_LN_HINT_CAP); }

    ret = term_ln_hist_get(ln, ln->search_sel, &match);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_str_fmt(ln->hint, LDG_TERM_LN_HINT_CAP, &hint_len, TERM_LN_SEARCH_FMT, match);
}

static uint32_t term_ln_label_hint_set(ldg_term_ln_t *ln, const char *candidate)
{
    uint32_t val_len = term_ln_candidate_len_get(candidate);

    if (candidate[val_len] != LDG_TERM_LN_CANDIDATE_LABEL_SEP) { return LDG_ERR_AOK; }

    return ldg_str_rbrcpy(ln->hint, candidate + val_len + LDG_STR_TERM_SIZE, LDG_TERM_LN_HINT_CAP);
}

static uint32_t term_ln_candidates_fill(ldg_term_ln_t *ln, uint32_t len)
{
    uint32_t at = len;
    uint32_t ret = 0;

    ln->candidate_cunt = 0;
    ln->candidate_more = LDG_TRUTH_FALSE;
    ln->candidate_at = len;

    if (!ln->candidate_cb) { return LDG_ERR_AOK; }

    ret = ln->candidate_cb(ln->candidate_user, ln->text, len, ln->candidate, ln->candidate_max, &ln->candidate_cunt, &at, &ln->candidate_more);
    if (ret != LDG_ERR_AOK)
    {
        ln->candidate_cunt = 0;
        ln->candidate_more = LDG_TRUTH_FALSE;

        return LDG_ERR_AOK;
    }

    if (ln->candidate_cunt > ln->candidate_max)
    {
        ln->candidate_cunt = ln->candidate_max;
        ln->candidate_more = LDG_TRUTH_TRUE;
    }

    ln->candidate_at = (at <= len) ? at : len;

    return LDG_ERR_AOK;
}

static uint32_t term_ln_verb_hint_set(ldg_term_ln_t *ln)
{
    uint32_t verb_len = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    while (verb_len < ln->len && ln->text[verb_len] != LDG_ASCII_SPACE) { verb_len++; }

    if (verb_len == 0 || verb_len == ln->len) { return LDG_ERR_AOK; }

    ret = term_ln_candidates_fill(ln, verb_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ln->candidate_cunt; idx++)
    {
        if (term_ln_candidate_len_get(ln->candidate[idx]) != verb_len || term_ln_prefix_is(ln->candidate[idx], ln->text, verb_len) != LDG_TRUTH_TRUE) { continue; }

        ret = term_ln_label_hint_set(ln, ln->candidate[idx]);
        break;
    }

    ln->candidate_cunt = 0;

    return ret;
}

static uint32_t term_ln_ghost_update(ldg_term_ln_t *ln)
{
    const char *text = 0x0;
    uint64_t hint_len = 0;
    uint32_t word_len = 0;
    uint32_t val_len = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    ln->ghost[0] = LDG_STR_TERM;
    ln->ghost_len = 0;
    ln->hint[0] = LDG_STR_TERM;

    if (ln->search_on == LDG_TRUTH_TRUE) { return term_ln_search_hint_set(ln); }

    if (ln->candidate_on == LDG_TRUTH_TRUE && ln->candidate_sel != LDG_TERM_LN_SEL_NONE) { return ldg_str_fmt(ln->hint, LDG_TERM_LN_HINT_CAP, &hint_len, TERM_LN_CANDIDATE_SEL_FMT, ln->candidate_sel + TERM_LN_RING_STEP, ln->candidate_cunt, (ln->candidate_more == LDG_TRUTH_TRUE) ? TERM_LN_MORE_MARK : TERM_LN_MORE_NONE); }

    if (ln->candidate_on == LDG_TRUTH_TRUE) { return ldg_str_fmt(ln->hint, LDG_TERM_LN_HINT_CAP, &hint_len, TERM_LN_CANDIDATE_LIST_FMT, ln->candidate_cunt, (ln->candidate_more == LDG_TRUTH_TRUE) ? TERM_LN_MORE_MARK : TERM_LN_MORE_NONE); }

    if (ln->len == 0 || ln->cursor != ln->len) { return LDG_ERR_AOK; }

    ret = term_ln_candidates_fill(ln, ln->len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ln->candidate_cunt == TERM_LN_CANDIDATE_SOLE && ln->candidate_more != LDG_TRUTH_TRUE) { ret = term_ln_label_hint_set(ln, ln->candidate[0]); }
    else if (ln->candidate_cunt > TERM_LN_CANDIDATE_SOLE) { ret = ldg_str_fmt(ln->hint, LDG_TERM_LN_HINT_CAP, &hint_len, TERM_LN_MATCHES_FMT, ln->candidate_cunt); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    word_len = ln->len - ln->candidate_at;
    val_len = (ln->candidate_cunt > 0) ? term_ln_candidate_len_get(ln->candidate[0]) : TERM_LN_OFF_NONE;

    if (ln->ghost_hidden == LDG_TRUTH_TRUE)
    {
        ln->candidate_cunt = 0;

        return LDG_ERR_AOK;
    }

    for (idx = 0; term_ln_hist_get(ln, idx, &text) == LDG_ERR_AOK && text; idx++)
    {
        if (term_ln_prefix_is(text, ln->text, ln->len) != LDG_TRUTH_TRUE || text[ln->len] == LDG_STR_TERM) { continue; }

        ln->candidate_cunt = 0;

        ret = ldg_str_rbrcpy(ln->ghost, text + ln->len, ln->cap);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ln->ghost_len = term_ln_len_get(ln->ghost, ln->cap);

        return LDG_ERR_AOK;
    }

    if (word_len > 0 && val_len > word_len && term_ln_prefix_is(ln->candidate[0], ln->text + ln->candidate_at, word_len) == LDG_TRUTH_TRUE)
    {
        ret = ldg_mem_cpy(ln->ghost, ln->candidate[0] + word_len, val_len - word_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ln->ghost_len = val_len - word_len;
        ln->ghost[ln->ghost_len] = LDG_STR_TERM;
    }

    ln->candidate_cunt = 0;

    if (ln->hint[0] == LDG_STR_TERM) { return term_ln_verb_hint_set(ln); }

    return LDG_ERR_AOK;
}

static uint32_t term_ln_ghost_take(ldg_term_ln_t *ln, uint8_t whole)
{
    uint32_t take = 0;

    if (ln->ghost_len == 0 || ln->cursor != ln->len) { return LDG_ERR_AOK; }

    take = ln->ghost_len;

    if (whole != LDG_TRUTH_TRUE)
    {
        take = 0;
        while (take < ln->ghost_len && ln->ghost[take] == LDG_ASCII_SPACE) { take++; }

        while (take < ln->ghost_len && ln->ghost[take] != LDG_ASCII_SPACE && ln->ghost[take] != TERM_LN_PATH_SEP && ln->ghost[take] != TERM_LN_PATH_SEP_ALT) { take++; }

        if (take < ln->ghost_len && (ln->ghost[take] == TERM_LN_PATH_SEP || ln->ghost[take] == TERM_LN_PATH_SEP_ALT)) { take++; }
    }

    return term_ln_span_swap(ln, ln->len, ln->len, ln->ghost, take);
}

static uint32_t term_ln_hist_step(ldg_term_ln_t *ln, uint8_t older)
{
    const char *text = 0x0;
    uint32_t ret = 0;

    if (older == LDG_TRUTH_TRUE)
    {
        if (ln->hist_sel == LDG_TERM_LN_SEL_NONE)
        {
            if (term_ln_hist_get(ln, TERM_LN_HIST_NEWEST, &text) != LDG_ERR_AOK || !text) { return LDG_ERR_AOK; }

            ret = ldg_mem_cpy(ln->draft, ln->text, (uint64_t)ln->len + LDG_STR_TERM_SIZE);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ln->hist_sel = TERM_LN_HIST_NEWEST;

            return term_ln_text_swap(ln, text);
        }

        if (term_ln_hist_get(ln, ln->hist_sel + TERM_LN_RING_STEP, &text) != LDG_ERR_AOK || !text) { return LDG_ERR_AOK; }

        ln->hist_sel++;

        return term_ln_text_swap(ln, text);
    }

    if (ln->hist_sel == LDG_TERM_LN_SEL_NONE) { return LDG_ERR_AOK; }

    if (ln->hist_sel == TERM_LN_HIST_NEWEST)
    {
        ln->hist_sel = LDG_TERM_LN_SEL_NONE;

        return term_ln_text_swap(ln, ln->draft);
    }

    ln->hist_sel--;

    ret = term_ln_hist_get(ln, ln->hist_sel, &text);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK || !text)) { return LDG_ERR_BOUNDS; }

    return term_ln_text_swap(ln, text);
}

static uint32_t term_ln_search_run(ldg_term_ln_t *ln, uint32_t start)
{
    uint32_t cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t match = LDG_TRUTH_FALSE;

    ln->search_sel = LDG_TERM_LN_SEL_NONE;

    if (ln->search_len == 0) { return LDG_ERR_AOK; }

    ret = term_ln_hist_cunt_get(ln, &cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < cunt; idx++)
    {
        ret = term_ln_hist_match_get(ln, (start + idx) % cunt, &match);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (match != LDG_TRUTH_TRUE) { continue; }

        ln->search_sel = (start + idx) % cunt;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_AOK;
}

static uint32_t term_ln_search_resync(ldg_term_ln_t *ln)
{
    uint32_t ret = 0;
    uint8_t match = LDG_TRUTH_FALSE;

    if (ln->search_sel == LDG_TERM_LN_SEL_NONE) { return term_ln_search_run(ln, TERM_LN_HIST_NEWEST); }

    if (ln->search_len > 0)
    {
        ret = term_ln_hist_match_get(ln, ln->search_sel, &match);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (match == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }
    }

    return term_ln_search_run(ln, ln->search_sel);
}

static uint32_t term_ln_search_key(ldg_term_ln_t *ln, uint8_t newer)
{
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t match = LDG_TRUTH_FALSE;

    if (ln->search_on != LDG_TRUTH_TRUE)
    {
        term_ln_modes_close(ln);
        ln->search_on = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (newer != LDG_TRUTH_TRUE) { return term_ln_search_run(ln, (ln->search_sel == LDG_TERM_LN_SEL_NONE) ? TERM_LN_HIST_NEWEST : ln->search_sel + TERM_LN_RING_STEP); }

    if (ln->search_sel == LDG_TERM_LN_SEL_NONE) { return LDG_ERR_AOK; }

    for (idx = ln->search_sel; idx > 0; idx--)
    {
        ret = term_ln_hist_match_get(ln, idx - TERM_LN_PREV_OFF, &match);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (match != LDG_TRUTH_TRUE) { continue; }

        ln->search_sel = idx - TERM_LN_PREV_OFF;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_AOK;
}

static uint32_t term_ln_submit(ldg_term_ln_t *ln)
{
    const char *match = 0x0;
    uint32_t ret = 0;

    if (ln->search_on == LDG_TRUTH_TRUE && ln->search_sel == LDG_TERM_LN_SEL_NONE)
    {
        term_ln_modes_close(ln);

        return LDG_ERR_AOK;
    }

    if (ln->search_on == LDG_TRUTH_TRUE)
    {
        ret = term_ln_hist_get(ln, ln->search_sel, &match);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK || !match)) { return LDG_ERR_BOUNDS; }

        ret = term_ln_text_swap(ln, match);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ln->candidate_on == LDG_TRUTH_TRUE && ln->candidate_sel != LDG_TERM_LN_SEL_NONE)
    {
        term_ln_modes_close(ln);

        return LDG_ERR_AOK;
    }

    term_ln_modes_close(ln);

    while (ln->len > 0 && (ln->text[ln->len - TERM_LN_PREV_OFF] == LDG_ASCII_SPACE || ln->text[ln->len - TERM_LN_PREV_OFF] == LDG_ASCII_CR || ln->text[ln->len - TERM_LN_PREV_OFF] == LDG_ASCII_LF)) { ln->len--; }

    ln->text[ln->len] = LDG_STR_TERM;
    ln->cursor = ln->len;

    if (ln->len == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_cpy(ln->taken, ln->text, (uint64_t)ln->len + LDG_STR_TERM_SIZE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ln->rdy = LDG_TRUTH_TRUE;
    ln->finish_due = LDG_TRUTH_TRUE;
    ln->hist_sel = LDG_TERM_LN_SEL_NONE;
    ln->text[0] = LDG_STR_TERM;
    ln->len = 0;
    ln->cursor = 0;

    return LDG_ERR_AOK;
}

static uint32_t term_ln_candidate_common_get(const ldg_term_ln_t *ln)
{
    uint32_t common = term_ln_candidate_len_get(ln->candidate[0]);
    uint32_t idx = 0;
    uint32_t at = 0;

    for (idx = 0; idx < ln->candidate_cunt; idx++)
    {
        at = 0;
        while (at < common && ln->candidate[idx][at] == ln->candidate[0][at]) { at++; }

        common = at;
    }

    return common;
}

static uint8_t term_ln_gap_wanted_is(const ldg_term_ln_t *ln)
{
    const char *word = ln->text + ln->candidate_at;
    uint32_t word_len = ln->len - ln->candidate_at;
    char last = 0;

    if (ln->len == 0 || word_len == 0) { return LDG_TRUTH_FALSE; }

    if (word[0] == TERM_LN_VAR_LEAD) { return LDG_TRUTH_FALSE; }

    if (word_len > TERM_LN_ADDR_LEAD_AT && word[0] == TERM_LN_ADDR_LEAD_FIRST && word[TERM_LN_ADDR_LEAD_AT] == TERM_LN_ADDR_LEAD_LAST) { return LDG_TRUTH_FALSE; }

    last = ln->text[ln->len - TERM_LN_PREV_OFF];

    if (last == TERM_LN_PATH_SEP || last == TERM_LN_PATH_SEP_ALT || last == TERM_LN_MEMBER_SEP || last == TERM_LN_ASSIGN) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t term_ln_tab(ldg_term_ln_t *ln, uint8_t back)
{
    uint32_t common = 0;
    uint32_t ret = 0;
    char gap = LDG_ASCII_SPACE;

    ln->cursor = ln->len;

    if (ln->candidate_on != LDG_TRUTH_TRUE)
    {
        ret = term_ln_candidates_fill(ln, ln->len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ln->candidate_cunt == 0) { return LDG_ERR_AOK; }

        common = term_ln_candidate_common_get(ln);

        if (common > ln->cursor - ln->candidate_at)
        {
            ret = term_ln_span_swap(ln, ln->candidate_at, ln->cursor, ln->candidate[0], common);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (ln->candidate_cunt > TERM_LN_CANDIDATE_SOLE || ln->candidate_more == LDG_TRUTH_TRUE)
        {
            ln->candidate_on = LDG_TRUTH_TRUE;
            ln->candidate_sel = LDG_TERM_LN_SEL_NONE;

            return LDG_ERR_AOK;
        }

        ln->candidate_cunt = 0;

        if (term_ln_gap_wanted_is(ln) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

        return term_ln_span_swap(ln, ln->len, ln->len, &gap, sizeof(char));
    }

    if (ln->candidate_sel == LDG_TERM_LN_SEL_NONE) { ln->candidate_sel = (back == LDG_TRUTH_TRUE) ? ln->candidate_cunt - TERM_LN_RING_STEP : TERM_LN_POS_FIRST; }
    else if (back == LDG_TRUTH_TRUE) { ln->candidate_sel = (ln->candidate_sel + ln->candidate_cunt - TERM_LN_RING_STEP) % ln->candidate_cunt; }
    else { ln->candidate_sel = (ln->candidate_sel + TERM_LN_RING_STEP) % ln->candidate_cunt; }

    return term_ln_span_swap(ln, ln->candidate_at, ln->cursor, ln->candidate[ln->candidate_sel], term_ln_candidate_len_get(ln->candidate[ln->candidate_sel]));
}

static uint32_t term_ln_ch_put(ldg_term_ln_t *ln, uint32_t codepoint)
{
    char ch = (char)codepoint;

    if (codepoint < TERM_LN_CH_MIN || codepoint > TERM_LN_CH_MAX) { return LDG_ERR_AOK; }

    if (ln->search_on == LDG_TRUTH_TRUE)
    {
        if (ln->search_len + LDG_STR_TERM_SIZE >= ln->cap) { return LDG_ERR_AOK; }

        ln->search[ln->search_len] = ch;
        ln->search_len++;
        ln->search[ln->search_len] = LDG_STR_TERM;

        return term_ln_search_resync(ln);
    }

    ln->candidate_on = LDG_TRUTH_FALSE;
    ln->hist_sel = LDG_TERM_LN_SEL_NONE;

    if (ln->len + LDG_STR_TERM_SIZE >= ln->cap) { return LDG_ERR_AOK; }

    return term_ln_span_swap(ln, ln->cursor, ln->cursor, &ch, sizeof(char));
}

static uint32_t term_ln_erase(ldg_term_ln_t *ln, uint8_t fwd)
{
    if (ln->search_on == LDG_TRUTH_TRUE)
    {
        if (ln->search_len == 0) { return LDG_ERR_AOK; }

        ln->search_len--;
        ln->search[ln->search_len] = LDG_STR_TERM;

        return term_ln_search_resync(ln);
    }

    ln->candidate_on = LDG_TRUTH_FALSE;

    if (fwd == LDG_TRUTH_TRUE)
    {
        if (ln->cursor >= ln->len) { return LDG_ERR_AOK; }

        return term_ln_span_swap(ln, ln->cursor, ln->cursor + TERM_LN_PREV_OFF, ln->text, TERM_LN_OFF_NONE);
    }

    if (ln->cursor == 0) { return LDG_ERR_AOK; }

    return term_ln_span_swap(ln, ln->cursor - TERM_LN_PREV_OFF, ln->cursor, ln->text, TERM_LN_OFF_NONE);
}

static uint32_t term_ln_ctrl_key(ldg_term_ln_t *ln, uint32_t codepoint, uint8_t shift, uint8_t *out_consumed)
{
    if (codepoint < TERM_LN_CH_MIN) { codepoint += TERM_LN_CTRL_OFF; }

    if (codepoint >= TERM_LN_UPPER_FIRST && codepoint <= TERM_LN_UPPER_LAST) { codepoint += TERM_LN_CASE_OFF; }

    *out_consumed = LDG_TRUTH_TRUE;

    if (codepoint == TERM_LN_KEY_SEARCH) { return term_ln_search_key(ln, shift); }

    if (codepoint == TERM_LN_KEY_EOF) { return LDG_ERR_EOF; }

    if (codepoint == TERM_LN_KEY_SEARCH_CANCEL)
    {
        term_ln_modes_close(ln);

        return LDG_ERR_AOK;
    }

    if (codepoint == TERM_LN_KEY_WORD_DROP && ln->search_on == LDG_TRUTH_TRUE)
    {
        while (ln->search_len > 0 && ln->search[ln->search_len - TERM_LN_PREV_OFF] == LDG_ASCII_SPACE) { ln->search_len--; }

        while (ln->search_len > 0 && ln->search[ln->search_len - TERM_LN_PREV_OFF] != LDG_ASCII_SPACE) { ln->search_len--; }

        ln->search[ln->search_len] = LDG_STR_TERM;

        return term_ln_search_resync(ln);
    }

    if (codepoint == TERM_LN_KEY_CLEAR)
    {
        term_ln_modes_close(ln);
        ln->hist_sel = LDG_TERM_LN_SEL_NONE;

        return term_ln_span_swap(ln, TERM_LN_POS_FIRST, ln->len, ln->text, TERM_LN_OFF_NONE);
    }

    if (ln->search_on == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (codepoint == TERM_LN_KEY_GHOST_WORD) { return term_ln_ghost_take(ln, LDG_TRUTH_FALSE); }

    if (codepoint == TERM_LN_KEY_HOME || codepoint == TERM_LN_KEY_END)
    {
        term_ln_modes_close(ln);
        ln->cursor = (codepoint == TERM_LN_KEY_HOME) ? TERM_LN_POS_FIRST : ln->len;

        return LDG_ERR_AOK;
    }

    if (codepoint == TERM_LN_KEY_DROP_HD)
    {
        term_ln_modes_close(ln);

        return term_ln_span_swap(ln, TERM_LN_POS_FIRST, ln->cursor, ln->text, TERM_LN_OFF_NONE);
    }

    if (codepoint == TERM_LN_KEY_DROP_TAIL)
    {
        term_ln_modes_close(ln);

        return term_ln_span_swap(ln, ln->cursor, ln->len, ln->text, TERM_LN_OFF_NONE);
    }

    if (codepoint == TERM_LN_KEY_WORD_DROP)
    {
        term_ln_modes_close(ln);

        return term_ln_span_swap(ln, term_ln_word_start_get(ln, ln->cursor), ln->cursor, ln->text, TERM_LN_OFF_NONE);
    }

    *out_consumed = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint32_t term_ln_named_key(ldg_term_ln_t *ln, const ldg_term_key_event_t *event, uint8_t *out_consumed)
{
    uint8_t ctrl = ((event->mods & LDG_TERM_MOD_CTRL) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t shift = ((event->mods & LDG_TERM_MOD_SHIFT) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t listed = LDG_TRUTH_FALSE;

    *out_consumed = LDG_TRUTH_TRUE;

    if (event->key == LDG_TERM_KEY_ENTER) { return term_ln_submit(ln); }

    if (event->key == LDG_TERM_KEY_TAB) { return term_ln_tab(ln, shift); }

    if (event->key == LDG_TERM_KEY_BACKSPACE && ctrl == LDG_TRUTH_TRUE) { return term_ln_ctrl_key(ln, TERM_LN_KEY_WORD_DROP, shift, out_consumed); }

    if (event->key == LDG_TERM_KEY_BACKSPACE) { return term_ln_erase(ln, LDG_TRUTH_FALSE); }

    if (event->key == LDG_TERM_KEY_DELETE) { return term_ln_erase(ln, LDG_TRUTH_TRUE); }

    if (event->key == LDG_TERM_KEY_ESCAPE)
    {
        if (ln->candidate_on != LDG_TRUTH_TRUE && ln->search_on != LDG_TRUTH_TRUE)
        {
            *out_consumed = (ln->ghost_len > 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
            ln->ghost_hidden = LDG_TRUTH_TRUE;
        }

        term_ln_modes_close(ln);

        return LDG_ERR_AOK;
    }

    if (ln->search_on == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (event->key == LDG_TERM_KEY_UP || event->key == LDG_TERM_KEY_PAGE_UP || event->key == LDG_TERM_KEY_DOWN || event->key == LDG_TERM_KEY_PAGE_DOWN)
    {
        term_ln_modes_close(ln);

        return term_ln_hist_step(ln, (event->key == LDG_TERM_KEY_UP || event->key == LDG_TERM_KEY_PAGE_UP) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);
    }

    if (event->key == LDG_TERM_KEY_RHS || event->key == LDG_TERM_KEY_END)
    {
        listed = ln->candidate_on;
        term_ln_modes_close(ln);

        if (listed == LDG_TRUTH_TRUE && event->key == LDG_TERM_KEY_RHS && ctrl != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

        if (ln->cursor == ln->len) { return term_ln_ghost_take(ln, (ctrl == LDG_TRUTH_TRUE) ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE); }

        if (event->key == LDG_TERM_KEY_END) { ln->cursor = ln->len; }
        else if (ctrl == LDG_TRUTH_TRUE) { ln->cursor = term_ln_word_end_get(ln, ln->cursor); }
        else { ln->cursor++; }

        return LDG_ERR_AOK;
    }

    if (event->key == LDG_TERM_KEY_LHS || event->key == LDG_TERM_KEY_HOME)
    {
        term_ln_modes_close(ln);

        if (event->key == LDG_TERM_KEY_HOME) { ln->cursor = TERM_LN_POS_FIRST; }
        else if (ctrl == LDG_TRUTH_TRUE) { ln->cursor = term_ln_word_start_get(ln, ln->cursor); }
        else if (ln->cursor > 0) { ln->cursor--; }

        return LDG_ERR_AOK;
    }

    *out_consumed = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_ln_key_put(ldg_term_ln_t *ln, const ldg_term_key_event_t *event, uint8_t *out_consumed)
{
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!ln || !event || !out_consumed)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_consumed = LDG_TRUTH_FALSE;

    if (event->kind == LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    ln->ghost_hidden = LDG_TRUTH_FALSE;

    if (event->key != LDG_TERM_KEY_NONE) { ret = term_ln_named_key(ln, event, out_consumed); }
    else if ((event->mods & LDG_TERM_MOD_CTRL) != 0 || (event->codepoint != 0 && event->codepoint < TERM_LN_CH_MIN)) { ret = term_ln_ctrl_key(ln, event->codepoint, ((event->mods & LDG_TERM_MOD_SHIFT) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, out_consumed); }
    else if ((event->mods & (LDG_TERM_MOD_ALT | LDG_TERM_MOD_SUPER | LDG_TERM_MOD_META | LDG_TERM_MOD_HYPER)) == 0)
    {
        *out_consumed = LDG_TRUTH_TRUE;

        if (event->text_cunt == 0) { ret = term_ln_ch_put(ln, ((event->mods & LDG_TERM_MOD_SHIFT) != 0 && event->shifted != 0) ? event->shifted : event->codepoint); }

        for (idx = 0; idx < event->text_cunt && idx < LDG_TERM_KEY_TEXT_MAX && ret == LDG_ERR_AOK; idx++) { ret = term_ln_ch_put(ln, event->text[idx]); }
    }

    if (ret != LDG_ERR_AOK) { return ret; }

    return term_ln_ghost_update(ln);
}

uint32_t ldg_term_ln_conf_default_get(ldg_term_ln_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->ln_cap = LDG_TERM_LN_CAP_DEFAULT;
    out->candidate_max = LDG_TERM_LN_CANDIDATE_MAX_DEFAULT;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_ln_pool_size_get(const ldg_term_ln_conf_t *conf, uint64_t *out)
{
    uint64_t total = 0;

    if (LDG_UNLIKELY(!conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(conf->ln_cap < LDG_TERM_LN_CAP_MIN || conf->candidate_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    total = (uint64_t)sizeof(ldg_term_ln_t);
    total += (uint64_t)TERM_LN_POOL_ALLOC_CUNT * (uint64_t)LDG_MEM_POOL_VAR_ALIGN;
    total += (uint64_t)conf->ln_cap * (uint64_t)TERM_LN_TEXT_BUFF_CUNT;
    total += (uint64_t)conf->candidate_max * (uint64_t)LDG_TERM_LN_CANDIDATE_CAP;

    *out = total;

    return LDG_ERR_AOK;
}

static uint32_t term_ln_buff_take(ldg_mem_pool_t *pool, uint64_t size, void **out)
{
    uint32_t ret = 0;

    ret = ldg_mem_pool_alloc(pool, size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

uint32_t ldg_term_ln_create(ldg_mem_pool_t *pool, const ldg_term_ln_conf_t *conf, ldg_term_ln_t **out)
{
    ldg_term_ln_t *ln = 0x0;
    uint64_t need = 0;
    uint64_t cap = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    ret = ldg_term_ln_pool_size_get(conf, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = term_ln_buff_take(pool, sizeof(ldg_term_ln_t), (void **)&ln);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cap = (uint64_t)conf->ln_cap;

    ret = term_ln_buff_take(pool, cap, (void **)&ln->text);
    if (ret == LDG_ERR_AOK) { ret = term_ln_buff_take(pool, cap, (void **)&ln->ghost); }

    if (ret == LDG_ERR_AOK) { ret = term_ln_buff_take(pool, cap, (void **)&ln->draft); }

    if (ret == LDG_ERR_AOK) { ret = term_ln_buff_take(pool, cap, (void **)&ln->search); }

    if (ret == LDG_ERR_AOK) { ret = term_ln_buff_take(pool, cap, (void **)&ln->taken); }

    if (ret == LDG_ERR_AOK) { ret = term_ln_buff_take(pool, (uint64_t)conf->candidate_max * (uint64_t)LDG_TERM_LN_CANDIDATE_CAP, (void **)&ln->candidate); }

    if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(ln->prompt, LDG_TERM_LN_PROMPT_DEFAULT, LDG_TERM_LN_PROMPT_CAP); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ln->cap = conf->ln_cap;
    ln->candidate_max = conf->candidate_max;
    ln->hist_sel = LDG_TERM_LN_SEL_NONE;
    ln->search_sel = LDG_TERM_LN_SEL_NONE;
    ln->candidate_sel = LDG_TERM_LN_SEL_NONE;
    *out = ln;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_ln_uncreate(ldg_term_ln_t **ln)
{
    if (LDG_UNLIKELY(!ln || !*ln)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ln = 0x0;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_ln_rst(ldg_term_ln_t *ln)
{
    if (LDG_UNLIKELY(!ln)) { return LDG_ERR_FUNC_ARG_NULL; }

    term_ln_modes_close(ln);
    ln->text[0] = LDG_STR_TERM;
    ln->ghost[0] = LDG_STR_TERM;
    ln->hint[0] = LDG_STR_TERM;
    ln->len = 0;
    ln->cursor = 0;
    ln->ghost_len = 0;
    ln->hist_sel = LDG_TERM_LN_SEL_NONE;
    ln->draw_row = 0;
    ln->rdy = LDG_TRUTH_FALSE;
    ln->ghost_hidden = LDG_TRUTH_FALSE;
    ln->finish_due = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_ln_text_get(const ldg_term_ln_t *ln, const char **out, uint32_t *out_len)
{
    if (LDG_UNLIKELY(!ln || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ln->text;
    *out_len = ln->len;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_ln_text_set(ldg_term_ln_t *ln, const char *text)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ln || !text)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(term_ln_len_get(text, ln->cap) >= ln->cap)) { return LDG_ERR_OVERFLOW; }

    term_ln_modes_close(ln);
    ln->hist_sel = LDG_TERM_LN_SEL_NONE;

    ret = term_ln_text_swap(ln, text);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return term_ln_ghost_update(ln);
}

uint32_t ldg_term_ln_take(ldg_term_ln_t *ln, char *out, uint64_t cap)
{
    uint32_t len = 0;

    if (LDG_UNLIKELY(!ln || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ln->rdy != LDG_TRUTH_TRUE) { return LDG_ERR_EMPTY; }

    len = term_ln_len_get(ln->taken, ln->cap);
    if (LDG_UNLIKELY((uint64_t)len + LDG_STR_TERM_SIZE > cap)) { return LDG_ERR_OVERFLOW; }

    ln->rdy = LDG_TRUTH_FALSE;

    return ldg_mem_cpy(out, ln->taken, (uint64_t)len + LDG_STR_TERM_SIZE);
}

uint32_t ldg_term_ln_prompt_set(ldg_term_ln_t *ln, const char *prompt)
{
    if (LDG_UNLIKELY(!ln || !prompt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(term_ln_len_get(prompt, LDG_TERM_LN_PROMPT_CAP) >= LDG_TERM_LN_PROMPT_CAP)) { return LDG_ERR_OVERFLOW; }

    return ldg_str_rbrcpy(ln->prompt, prompt, LDG_TERM_LN_PROMPT_CAP);
}

uint32_t ldg_term_ln_hist_cb_set(ldg_term_ln_t *ln, ldg_term_ln_hist_cb_t cb, void *user)
{
    if (LDG_UNLIKELY(!ln)) { return LDG_ERR_FUNC_ARG_NULL; }

    ln->hist_cb = cb;
    ln->hist_user = user;
    ln->hist_sel = LDG_TERM_LN_SEL_NONE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_ln_candidate_cb_set(ldg_term_ln_t *ln, ldg_term_ln_candidate_cb_t cb, void *user)
{
    if (LDG_UNLIKELY(!ln)) { return LDG_ERR_FUNC_ARG_NULL; }

    ln->candidate_cb = cb;
    ln->candidate_user = user;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_ln_view_get(const ldg_term_ln_t *ln, ldg_term_ln_view_t *out)
{
    if (LDG_UNLIKELY(!ln || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->prompt = ln->prompt;
    out->text = ln->text;
    out->ghost = ln->ghost;
    out->hint = ln->hint;
    out->candidate = (const char (*)[LDG_TERM_LN_CANDIDATE_CAP]) ln->candidate;
    out->prompt_len = term_ln_len_get(ln->prompt, LDG_TERM_LN_PROMPT_CAP);
    out->text_len = ln->len;
    out->ghost_len = ln->ghost_len;
    out->hint_len = term_ln_len_get(ln->hint, LDG_TERM_LN_HINT_CAP);
    out->cursor = ln->cursor;
    out->candidate_cunt = (ln->candidate_on == LDG_TRUTH_TRUE) ? ln->candidate_cunt : TERM_LN_OFF_NONE;
    out->candidate_sel = ln->candidate_sel;
    out->search_on = ln->search_on;
    out->candidate_on = ln->candidate_on;
    out->candidate_more = ln->candidate_more;

    if (ln->search_on == LDG_TRUTH_TRUE)
    {
        out->text = ln->search;
        out->text_len = ln->search_len;
        out->cursor = ln->search_len;
    }

    return LDG_ERR_AOK;
}

static uint32_t term_ln_pad(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t cunt, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg)
{
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;
    char blank = LDG_ASCII_SPACE;

    for (idx = 0; idx < cunt && ret == LDG_ERR_AOK; idx++) { ret = ldg_term_render_text_draw(render, col + idx, row, &blank, sizeof(char), fg, bg); }

    return ret;
}

static uint32_t term_ln_span_draw(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt, const char *text, uint32_t len, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg)
{
    uint32_t shown = (len < col_cunt) ? len : col_cunt;
    uint32_t ret = 0;

    if (shown > 0)
    {
        ret = ldg_term_render_text_draw(render, col, row, text, shown, fg, bg);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return term_ln_pad(render, col + shown, row, col_cunt - shown, fg, bg);
}

static uint32_t term_ln_candidates_draw(const ldg_term_ln_view_t *view, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt, uint32_t row_cunt, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *ghost_fg, const ldg_term_render_color_t *bg)
{
    uint32_t width = 0;
    uint32_t per_row = 0;
    uint32_t row_first = 0;
    uint32_t len = 0;
    uint32_t val_len = 0;
    uint32_t idx = 0;
    uint32_t grid_row = 0;
    uint32_t grid_col = 0;
    uint32_t ret = LDG_ERR_AOK;
    uint8_t sel = LDG_TRUTH_FALSE;

    for (idx = 0; idx < view->candidate_cunt; idx++)
    {
        len = term_ln_len_get(view->candidate[idx], LDG_TERM_LN_CANDIDATE_CAP);
        if (len > width) { width = len; }
    }

    width += TERM_LN_CANDIDATE_GAP;
    if (width > col_cunt) { width = col_cunt; }

    per_row = col_cunt / width;

    if (view->candidate_sel != LDG_TERM_LN_SEL_NONE && view->candidate_sel / per_row >= row_cunt) { row_first = view->candidate_sel / per_row - row_cunt + TERM_LN_ROW_CUNT_LN; }

    for (idx = 0; idx < row_cunt && ret == LDG_ERR_AOK; idx++) { ret = term_ln_pad(render, col, row + idx, col_cunt, fg, bg); }

    for (idx = row_first * per_row; idx < view->candidate_cunt && ret == LDG_ERR_AOK; idx++)
    {
        grid_row = idx / per_row - row_first;
        grid_col = idx % per_row;

        if (grid_row >= row_cunt) { break; }

        len = term_ln_len_get(view->candidate[idx], LDG_TERM_LN_CANDIDATE_CAP);
        if (len > width) { len = width; }

        val_len = term_ln_candidate_len_get(view->candidate[idx]);
        if (val_len > len) { val_len = len; }

        sel = (idx == view->candidate_sel) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (val_len > 0) { ret = ldg_term_render_text_draw(render, col + grid_col * width, row + grid_row, view->candidate[idx], val_len, sel ? bg : fg, sel ? fg : bg); }

        if (ret == LDG_ERR_AOK && val_len + TERM_LN_CURSOR_WIDTH < len) { ret = ldg_term_render_text_draw(render, col + grid_col * width + val_len + TERM_LN_CURSOR_WIDTH, row + grid_row, view->candidate[idx] + val_len + TERM_LN_CURSOR_WIDTH, len - val_len - TERM_LN_CURSOR_WIDTH, ghost_fg, bg); }
    }

    return ret;
}

uint32_t ldg_term_ln_draw(const ldg_term_ln_t *ln, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt, uint32_t row_cunt, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *ghost_fg, const ldg_term_render_color_t *bg)
{
    ldg_term_ln_view_t view = LDG_STRUCT_ZERO_INIT;
    uint32_t above = 0;
    uint32_t text_cols = 0;
    uint32_t off = 0;
    uint32_t shown = 0;
    uint32_t ghost_shown = 0;
    uint32_t text_col = 0;
    uint32_t ln_row = 0;
    uint32_t ret = 0;
    char cursor_ch = LDG_ASCII_SPACE;

    if (LDG_UNLIKELY(!ln || !render || !fg || !ghost_fg || !bg)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_term_ln_view_get(ln, &view);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(col_cunt <= view.prompt_len + TERM_LN_CURSOR_WIDTH)) { return LDG_ERR_FUNC_ARG_INVALID; }

    above = row_cunt - TERM_LN_ROW_CUNT_LN;
    ln_row = row + above;

    if (above > 0 && view.candidate_on == LDG_TRUTH_TRUE && view.candidate_cunt > 0)
    {
        ret = term_ln_candidates_draw(&view, render, col, row, col_cunt, above, fg, ghost_fg, bg);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    text_cols = col_cunt - view.prompt_len;
    off = (view.cursor >= text_cols) ? view.cursor - text_cols + TERM_LN_CURSOR_WIDTH : TERM_LN_OFF_NONE;
    shown = (view.text_len - off < text_cols) ? view.text_len - off : text_cols;
    ghost_shown = (view.ghost_len < text_cols - shown) ? view.ghost_len : text_cols - shown;
    text_col = col + view.prompt_len;

    ret = ldg_term_render_text_draw(render, col, ln_row, view.prompt, view.prompt_len, fg, bg);
    if (ret == LDG_ERR_AOK) { ret = term_ln_span_draw(render, text_col, ln_row, shown, view.text + off, shown, fg, bg); }

    if (ret == LDG_ERR_AOK) { ret = term_ln_span_draw(render, text_col + shown, ln_row, text_cols - shown, view.ghost, ghost_shown, ghost_fg, bg); }

    if (ret == LDG_ERR_AOK && view.hint_len > 0 && shown + ghost_shown + TERM_LN_HINT_GAP + TERM_LN_CURSOR_WIDTH + view.hint_len <= text_cols) { ret = ldg_term_render_text_draw(render, col + col_cunt - view.hint_len, ln_row, view.hint, view.hint_len, ghost_fg, bg); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (view.cursor < view.text_len) { cursor_ch = view.text[view.cursor]; }
    else if (view.ghost_len > 0) { cursor_ch = view.ghost[0]; }

    return ldg_term_render_text_draw(render, text_col + view.cursor - off, ln_row, &cursor_ch, sizeof(char), bg, fg);
}

static uint32_t term_ln_sink_put(term_ln_sink_t *sink, const char *text, uint64_t len)
{
    uint32_t ret = 0;

    if (len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(sink->len + len > sink->cap)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_cpy(sink->out + sink->len, text, len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sink->len += len;

    return LDG_ERR_AOK;
}

static uint32_t term_ln_sink_seq_put(term_ln_sink_t *sink, const char *fmt, uint32_t val)
{
    char seq[TERM_LN_SEQ_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t seq_len = 0;
    uint32_t ret = 0;

    ret = ldg_str_fmt(seq, TERM_LN_SEQ_CAP, &seq_len, fmt, val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return term_ln_sink_put(sink, seq, seq_len);
}

static uint32_t term_ln_candidates_encode(const ldg_term_ln_view_t *view, term_ln_sink_t *sink, uint32_t col_cunt, uint32_t *rows)
{
    uint32_t width = 0;
    uint32_t per_row = 0;
    uint32_t row_first = 0;
    uint32_t len = 0;
    uint32_t val_len = 0;
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    for (idx = 0; idx < view->candidate_cunt; idx++)
    {
        len = term_ln_len_get(view->candidate[idx], LDG_TERM_LN_CANDIDATE_CAP);
        if (len > width) { width = len; }
    }

    width += TERM_LN_CANDIDATE_GAP;
    per_row = (width < col_cunt) ? col_cunt / width : TERM_LN_CANDIDATE_SOLE;

    if (view->candidate_sel != LDG_TERM_LN_SEL_NONE && view->candidate_sel / per_row >= TERM_LN_ENCODE_ROW_MAX) { row_first = view->candidate_sel / per_row - TERM_LN_ENCODE_ROW_MAX + TERM_LN_ROW_CUNT_LN; }

    for (idx = row_first * per_row; idx < view->candidate_cunt && idx < (row_first + TERM_LN_ENCODE_ROW_MAX) * per_row && ret == LDG_ERR_AOK; idx++)
    {
        if (idx % per_row == 0)
        {
            ret = term_ln_sink_put(sink, TERM_LN_ROW_END, sizeof(TERM_LN_ROW_END) - LDG_STR_TERM_SIZE);
            (*rows)++;
        }

        len = term_ln_len_get(view->candidate[idx], LDG_TERM_LN_CANDIDATE_CAP);
        if (len + TERM_LN_CANDIDATE_GAP >= col_cunt) { len = col_cunt - TERM_LN_CANDIDATE_GAP - TERM_LN_COL_BASE; }

        val_len = term_ln_candidate_len_get(view->candidate[idx]);
        if (val_len > len) { val_len = len; }

        if (ret == LDG_ERR_AOK && idx == view->candidate_sel) { ret = term_ln_sink_put(sink, TERM_LN_SEL_ON, sizeof(TERM_LN_SEL_ON) - LDG_STR_TERM_SIZE); }

        if (ret == LDG_ERR_AOK) { ret = term_ln_sink_put(sink, view->candidate[idx], val_len); }

        if (ret == LDG_ERR_AOK && idx == view->candidate_sel) { ret = term_ln_sink_put(sink, TERM_LN_SEL_OFF, sizeof(TERM_LN_SEL_OFF) - LDG_STR_TERM_SIZE); }

        if (ret == LDG_ERR_AOK && val_len + TERM_LN_CURSOR_WIDTH < len)
        {
            ret = term_ln_sink_put(sink, TERM_LN_GHOST_ON, sizeof(TERM_LN_GHOST_ON) - LDG_STR_TERM_SIZE);
            if (ret == LDG_ERR_AOK) { ret = term_ln_sink_put(sink, TERM_LN_GAP, TERM_LN_CURSOR_WIDTH); }

            if (ret == LDG_ERR_AOK) { ret = term_ln_sink_put(sink, view->candidate[idx] + val_len + TERM_LN_CURSOR_WIDTH, len - val_len - TERM_LN_CURSOR_WIDTH); }

            if (ret == LDG_ERR_AOK) { ret = term_ln_sink_put(sink, TERM_LN_GHOST_OFF, sizeof(TERM_LN_GHOST_OFF) - LDG_STR_TERM_SIZE); }
        }

        if (ret == LDG_ERR_AOK) { ret = term_ln_sink_put(sink, TERM_LN_GAP, sizeof(TERM_LN_GAP) - LDG_STR_TERM_SIZE); }
    }

    return ret;
}

uint32_t ldg_term_ln_encode(ldg_term_ln_t *ln, uint32_t col_cunt, ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    ldg_term_ln_view_t view = LDG_STRUCT_ZERO_INIT;
    term_ln_sink_t sink = LDG_STRUCT_ZERO_INIT;
    uint32_t total = 0;
    uint32_t rows = 0;
    uint32_t tgt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ln || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    ret = ldg_term_ln_view_get(ln, &view);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(col_cunt <= view.prompt_len + TERM_LN_CANDIDATE_GAP + TERM_LN_COL_BASE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    sink.out = out;
    sink.cap = cap;

    if (ln->draw_row > 0) { ret = term_ln_sink_seq_put(&sink, TERM_LN_SEQ_UP_FMT, ln->draw_row); }

    if (ret == LDG_ERR_AOK) { ret = term_ln_sink_put(&sink, TERM_LN_ERASE_BELOW, sizeof(TERM_LN_ERASE_BELOW) - LDG_STR_TERM_SIZE); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ln->finish_due == LDG_TRUTH_TRUE)
    {
        ret = term_ln_sink_put(&sink, view.prompt, view.prompt_len);
        if (ret == LDG_ERR_AOK) { ret = term_ln_sink_put(&sink, ln->taken, term_ln_len_get(ln->taken, ln->cap)); }

        if (ret == LDG_ERR_AOK) { ret = term_ln_sink_put(&sink, TERM_LN_ROW_END, sizeof(TERM_LN_ROW_END) - LDG_STR_TERM_SIZE); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ln->finish_due = LDG_TRUTH_FALSE;
        ln->draw_row = 0;
        *out_len = sink.len;

        return LDG_ERR_AOK;
    }

    ret = term_ln_sink_put(&sink, view.prompt, view.prompt_len);
    if (ret == LDG_ERR_AOK) { ret = term_ln_sink_put(&sink, view.text, view.text_len); }

    if (ret == LDG_ERR_AOK && view.ghost_len > 0) { ret = term_ln_sink_put(&sink, TERM_LN_GHOST_ON, sizeof(TERM_LN_GHOST_ON) - LDG_STR_TERM_SIZE); }

    if (ret == LDG_ERR_AOK) { ret = term_ln_sink_put(&sink, view.ghost, view.ghost_len); }

    if (ret == LDG_ERR_AOK && view.ghost_len > 0) { ret = term_ln_sink_put(&sink, TERM_LN_GHOST_OFF, sizeof(TERM_LN_GHOST_OFF) - LDG_STR_TERM_SIZE); }

    total = view.prompt_len + view.text_len + view.ghost_len;

    if (ret == LDG_ERR_AOK && total % col_cunt == 0) { ret = term_ln_sink_put(&sink, TERM_LN_WRAP_SETTLE, sizeof(TERM_LN_WRAP_SETTLE) - LDG_STR_TERM_SIZE); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    rows = total / col_cunt;

    if (view.candidate_on == LDG_TRUTH_TRUE && view.candidate_cunt > 0) { ret = term_ln_candidates_encode(&view, &sink, col_cunt, &rows); }
    else if (view.hint_len > 0)
    {
        ret = term_ln_sink_put(&sink, TERM_LN_ROW_END, sizeof(TERM_LN_ROW_END) - LDG_STR_TERM_SIZE);
        if (ret == LDG_ERR_AOK) { ret = term_ln_sink_put(&sink, TERM_LN_GHOST_ON, sizeof(TERM_LN_GHOST_ON) - LDG_STR_TERM_SIZE); }

        if (ret == LDG_ERR_AOK) { ret = term_ln_sink_put(&sink, view.hint, (view.hint_len < col_cunt) ? view.hint_len : col_cunt - TERM_LN_COL_BASE); }

        if (ret == LDG_ERR_AOK) { ret = term_ln_sink_put(&sink, TERM_LN_GHOST_OFF, sizeof(TERM_LN_GHOST_OFF) - LDG_STR_TERM_SIZE); }

        rows++;
    }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    tgt = (view.prompt_len + view.cursor) / col_cunt;

    if (rows > tgt) { ret = term_ln_sink_seq_put(&sink, TERM_LN_SEQ_UP_FMT, rows - tgt); }

    if (ret == LDG_ERR_AOK) { ret = term_ln_sink_seq_put(&sink, TERM_LN_SEQ_COL_FMT, (view.prompt_len + view.cursor) % col_cunt + TERM_LN_COL_BASE); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ln->draw_row = tgt;
    *out_len = sink.len;

    return LDG_ERR_AOK;
}
