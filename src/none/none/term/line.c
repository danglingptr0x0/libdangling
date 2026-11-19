#include <stdint.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>
#include <dangling/mem/cpy.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>
#include <dangling/term/line.h>

#define TERM_LINE_POOL_ALLOC_CUNT 7
#define TERM_LINE_TEXT_BUFF_CUNT 5
#define TERM_LINE_PREV_OFF 1
#define TERM_LINE_RING_STEP 1
#define TERM_LINE_CANDIDATE_SOLE 1
#define TERM_LINE_HIST_NEWEST 0
#define TERM_LINE_POS_FIRST 0
#define TERM_LINE_CH_MIN 0x20
#define TERM_LINE_CH_MAX 0x7E
#define TERM_LINE_CASE_OFF 0x20
#define TERM_LINE_UPPER_FIRST 'A'
#define TERM_LINE_UPPER_LAST 'Z'
#define TERM_LINE_CTRL_OFF 0x60
#define TERM_LINE_KEY_SEARCH 'r'
#define TERM_LINE_KEY_WORD_DROP 'w'
#define TERM_LINE_KEY_CLEAR 'c'
#define TERM_LINE_KEY_GHOST_WORD 'l'
#define TERM_LINE_KEY_HOME 'a'
#define TERM_LINE_KEY_END 'e'
#define TERM_LINE_KEY_DROP_HD 'u'
#define TERM_LINE_KEY_DROP_TAIL 'k'
#define TERM_LINE_KEY_EOF 'd'
#define TERM_LINE_KEY_SEARCH_CANCEL 'g'
#define TERM_LINE_SEARCH_FMT "bck-i-search: %.170s"
#define TERM_LINE_SEARCH_NONE "bck-i-search: no match"
#define TERM_LINE_CANDIDATE_SEL_FMT "%u32 of %u32%s; tab cycles, shift+tab back"
#define TERM_LINE_CANDIDATE_LIST_FMT "%u32%s candidates; tab cycles"
#define TERM_LINE_MATCHES_FMT "%u32 matches; tab to complete"
#define TERM_LINE_MORE_MARK "+"
#define TERM_LINE_MORE_NONE ""
#define TERM_LINE_VAR_LEAD '$'
#define TERM_LINE_ADDR_LEAD_FIRST '0'
#define TERM_LINE_ADDR_LEAD_LAST 'x'
#define TERM_LINE_ADDR_LEAD_AT 1
#define TERM_LINE_ASSIGN '='
#define TERM_LINE_MEMBER_SEP '.'
#define TERM_LINE_PATH_SEP '/'
#define TERM_LINE_PATH_SEP_ALT '\\'
#define TERM_LINE_CURSOR_WIDTH 1
#define TERM_LINE_CANDIDATE_GAP 2
#define TERM_LINE_HINT_GAP 1
#define TERM_LINE_ROW_CUNT_LINE 1
#define TERM_LINE_COL_BASE 1
#define TERM_LINE_OFF_NONE 0
#define TERM_LINE_ENCODE_ROW_MAX 10
#define TERM_LINE_SEQ_CAP 16
#define TERM_LINE_SEQ_UP_FMT "\x1B[%u32A"
#define TERM_LINE_SEQ_COL_FMT "\x1B[%u32G"
#define TERM_LINE_ERASE_BELOW "\r\x1B[J"
#define TERM_LINE_ROW_END "\r\n"
#define TERM_LINE_WRAP_SETTLE "\x20\b"
#define TERM_LINE_GHOST_ON "\x1B[90m"
#define TERM_LINE_GHOST_OFF "\x1B[39m"
#define TERM_LINE_SEL_ON "\x1B[7m"
#define TERM_LINE_SEL_OFF "\x1B[27m"
#define TERM_LINE_GAP "\x20\x20"

typedef struct term_line_sink
{
    ldg_byte_t *out;
    uint64_t cap;
    uint64_t len;
} term_line_sink_t;

static uint32_t term_line_len_get(const char *text, uint32_t cap)
{
    uint32_t len = 0;

    while (len < cap && text[len] != LDG_STR_TERM) { len++; }

    return len;
}

static uint32_t term_line_candidate_len_get(const char *text)
{
    uint32_t len = 0;

    while (len < LDG_TERM_LINE_CANDIDATE_CAP && text[len] != LDG_STR_TERM && text[len] != LDG_TERM_LINE_CANDIDATE_LABEL_SEP) { len++; }

    return len;
}

static uint32_t term_line_word_start_get(const ldg_term_line_t *line, uint32_t pos)
{
    while (pos > 0 && line->text[pos - TERM_LINE_PREV_OFF] == LDG_ASCII_SPACE) { pos--; }

    while (pos > 0 && line->text[pos - TERM_LINE_PREV_OFF] != LDG_ASCII_SPACE) { pos--; }

    return pos;
}

static uint32_t term_line_word_end_get(const ldg_term_line_t *line, uint32_t pos)
{
    while (pos < line->len && line->text[pos] == LDG_ASCII_SPACE) { pos++; }

    while (pos < line->len && line->text[pos] != LDG_ASCII_SPACE) { pos++; }

    return pos;
}

static uint32_t term_line_span_swap(ldg_term_line_t *line, uint32_t start, uint32_t end, const char *text, uint32_t text_len)
{
    uint32_t tail = line->len - end;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(start + text_len + tail + LDG_STR_TERM_SIZE > line->cap)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_mov(line->text + start + text_len, line->text + end, (uint64_t)tail + LDG_STR_TERM_SIZE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (text_len > 0)
    {
        ret = ldg_mem_cpy(line->text + start, text, text_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    line->len = start + text_len + tail;
    line->cursor = start + text_len;

    return LDG_ERR_AOK;
}

static uint32_t term_line_text_swap(ldg_term_line_t *line, const char *text)
{
    uint32_t ret = 0;

    ret = term_line_span_swap(line, TERM_LINE_POS_FIRST, line->len, text, term_line_len_get(text, line->cap));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    line->cursor = line->len;

    return LDG_ERR_AOK;
}

static void term_line_modes_close(ldg_term_line_t *line)
{
    line->candidate_on = LDG_TRUTH_FALSE;
    line->candidate_sel = LDG_TERM_LINE_SEL_NONE;
    line->candidate_cunt = 0;
    line->search_on = LDG_TRUTH_FALSE;
    line->search_sel = LDG_TERM_LINE_SEL_NONE;
    line->search_len = 0;
    line->search[0] = LDG_STR_TERM;
}

static uint8_t term_line_prefix_is(const char *text, const char *prefix, uint32_t prefix_len)
{
    uint32_t idx = 0;

    for (idx = 0; idx < prefix_len; idx++) { if (text[idx] == LDG_STR_TERM || text[idx] != prefix[idx]) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static uint32_t term_line_hist_get(const ldg_term_line_t *line, uint32_t idx, const char **out)
{
    *out = 0x0;

    if (!line->hist_cb) { return LDG_ERR_BOUNDS; }

    return line->hist_cb(line->hist_user, idx, out);
}

static uint32_t term_line_hist_cunt_get(const ldg_term_line_t *line)
{
    const char *text = 0x0;
    uint32_t cunt = 0;

    while (cunt < UINT32_MAX && term_line_hist_get(line, cunt, &text) == LDG_ERR_AOK && text) { cunt++; }

    return cunt;
}

static uint32_t term_line_search_hint_set(ldg_term_line_t *line)
{
    const char *match = 0x0;
    uint64_t hint_len = 0;
    uint32_t ret = 0;

    if (line->search_sel == LDG_TERM_LINE_SEL_NONE) { return ldg_str_rbrcpy(line->hint, TERM_LINE_SEARCH_NONE, LDG_TERM_LINE_HINT_CAP); }

    ret = term_line_hist_get(line, line->search_sel, &match);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_str_fmt(line->hint, LDG_TERM_LINE_HINT_CAP, &hint_len, TERM_LINE_SEARCH_FMT, match);
}

static uint32_t term_line_label_hint_set(ldg_term_line_t *line, const char *candidate)
{
    uint32_t val_len = term_line_candidate_len_get(candidate);

    if (candidate[val_len] != LDG_TERM_LINE_CANDIDATE_LABEL_SEP) { return LDG_ERR_AOK; }

    return ldg_str_rbrcpy(line->hint, candidate + val_len + LDG_STR_TERM_SIZE, LDG_TERM_LINE_HINT_CAP);
}

static uint32_t term_line_candidates_fill(ldg_term_line_t *line, uint32_t len)
{
    uint32_t at = len;
    uint32_t ret = 0;

    line->candidate_cunt = 0;
    line->candidate_more = LDG_TRUTH_FALSE;
    line->candidate_at = len;

    if (!line->candidate_cb) { return LDG_ERR_AOK; }

    ret = line->candidate_cb(line->candidate_user, line->text, len, line->candidate, line->candidate_max, &line->candidate_cunt, &at, &line->candidate_more);
    if (ret != LDG_ERR_AOK)
    {
        line->candidate_cunt = 0;
        line->candidate_more = LDG_TRUTH_FALSE;

        return LDG_ERR_AOK;
    }

    if (line->candidate_cunt > line->candidate_max)
    {
        line->candidate_cunt = line->candidate_max;
        line->candidate_more = LDG_TRUTH_TRUE;
    }

    line->candidate_at = (at <= len) ? at : len;

    return LDG_ERR_AOK;
}

static uint32_t term_line_verb_hint_set(ldg_term_line_t *line)
{
    uint32_t verb_len = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    while (verb_len < line->len && line->text[verb_len] != LDG_ASCII_SPACE) { verb_len++; }

    if (verb_len == 0 || verb_len == line->len) { return LDG_ERR_AOK; }

    ret = term_line_candidates_fill(line, verb_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < line->candidate_cunt; idx++)
    {
        if (term_line_candidate_len_get(line->candidate[idx]) != verb_len || term_line_prefix_is(line->candidate[idx], line->text, verb_len) != LDG_TRUTH_TRUE) { continue; }

        ret = term_line_label_hint_set(line, line->candidate[idx]);
        break;
    }

    line->candidate_cunt = 0;

    return ret;
}

static uint32_t term_line_ghost_update(ldg_term_line_t *line)
{
    const char *text = 0x0;
    uint64_t hint_len = 0;
    uint32_t word_len = 0;
    uint32_t val_len = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    line->ghost[0] = LDG_STR_TERM;
    line->ghost_len = 0;
    line->hint[0] = LDG_STR_TERM;

    if (line->search_on == LDG_TRUTH_TRUE) { return term_line_search_hint_set(line); }

    if (line->candidate_on == LDG_TRUTH_TRUE && line->candidate_sel != LDG_TERM_LINE_SEL_NONE) { return ldg_str_fmt(line->hint, LDG_TERM_LINE_HINT_CAP, &hint_len, TERM_LINE_CANDIDATE_SEL_FMT, line->candidate_sel + TERM_LINE_RING_STEP, line->candidate_cunt, (line->candidate_more == LDG_TRUTH_TRUE) ? TERM_LINE_MORE_MARK : TERM_LINE_MORE_NONE); }

    if (line->candidate_on == LDG_TRUTH_TRUE) { return ldg_str_fmt(line->hint, LDG_TERM_LINE_HINT_CAP, &hint_len, TERM_LINE_CANDIDATE_LIST_FMT, line->candidate_cunt, (line->candidate_more == LDG_TRUTH_TRUE) ? TERM_LINE_MORE_MARK : TERM_LINE_MORE_NONE); }

    if (line->len == 0 || line->cursor != line->len) { return LDG_ERR_AOK; }

    ret = term_line_candidates_fill(line, line->len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (line->candidate_cunt == TERM_LINE_CANDIDATE_SOLE && line->candidate_more != LDG_TRUTH_TRUE) { ret = term_line_label_hint_set(line, line->candidate[0]); }
    else if (line->candidate_cunt > TERM_LINE_CANDIDATE_SOLE) { ret = ldg_str_fmt(line->hint, LDG_TERM_LINE_HINT_CAP, &hint_len, TERM_LINE_MATCHES_FMT, line->candidate_cunt); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_STR_TRUNC)) { return ret; }

    word_len = line->len - line->candidate_at;
    val_len = (line->candidate_cunt > 0) ? term_line_candidate_len_get(line->candidate[0]) : TERM_LINE_OFF_NONE;

    if (line->ghost_hidden == LDG_TRUTH_TRUE)
    {
        line->candidate_cunt = 0;

        return LDG_ERR_AOK;
    }

    for (idx = 0; term_line_hist_get(line, idx, &text) == LDG_ERR_AOK && text; idx++)
    {
        if (term_line_prefix_is(text, line->text, line->len) != LDG_TRUTH_TRUE || text[line->len] == LDG_STR_TERM) { continue; }

        line->candidate_cunt = 0;

        ret = ldg_str_rbrcpy(line->ghost, text + line->len, line->cap);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        line->ghost_len = term_line_len_get(line->ghost, line->cap);

        return LDG_ERR_AOK;
    }

    if (word_len > 0 && val_len > word_len && term_line_prefix_is(line->candidate[0], line->text + line->candidate_at, word_len) == LDG_TRUTH_TRUE)
    {
        ret = ldg_mem_cpy(line->ghost, line->candidate[0] + word_len, val_len - word_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        line->ghost_len = val_len - word_len;
        line->ghost[line->ghost_len] = LDG_STR_TERM;
    }

    line->candidate_cunt = 0;

    if (line->hint[0] == LDG_STR_TERM) { return term_line_verb_hint_set(line); }

    return LDG_ERR_AOK;
}

static uint32_t term_line_ghost_take(ldg_term_line_t *line, uint8_t whole)
{
    uint32_t take = 0;

    if (line->ghost_len == 0 || line->cursor != line->len) { return LDG_ERR_AOK; }

    take = line->ghost_len;

    if (whole != LDG_TRUTH_TRUE)
    {
        take = 0;
        while (take < line->ghost_len && line->ghost[take] == LDG_ASCII_SPACE) { take++; }

        while (take < line->ghost_len && line->ghost[take] != LDG_ASCII_SPACE && line->ghost[take] != TERM_LINE_PATH_SEP && line->ghost[take] != TERM_LINE_PATH_SEP_ALT) { take++; }

        if (take < line->ghost_len && (line->ghost[take] == TERM_LINE_PATH_SEP || line->ghost[take] == TERM_LINE_PATH_SEP_ALT)) { take++; }
    }

    return term_line_span_swap(line, line->len, line->len, line->ghost, take);
}

static uint32_t term_line_hist_step(ldg_term_line_t *line, uint8_t older)
{
    const char *text = 0x0;
    uint32_t ret = 0;

    if (older == LDG_TRUTH_TRUE)
    {
        if (line->hist_sel == LDG_TERM_LINE_SEL_NONE)
        {
            if (term_line_hist_get(line, TERM_LINE_HIST_NEWEST, &text) != LDG_ERR_AOK || !text) { return LDG_ERR_AOK; }

            ret = ldg_mem_cpy(line->draft, line->text, (uint64_t)line->len + LDG_STR_TERM_SIZE);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            line->hist_sel = TERM_LINE_HIST_NEWEST;

            return term_line_text_swap(line, text);
        }

        if (term_line_hist_get(line, line->hist_sel + TERM_LINE_RING_STEP, &text) != LDG_ERR_AOK || !text) { return LDG_ERR_AOK; }

        line->hist_sel++;

        return term_line_text_swap(line, text);
    }

    if (line->hist_sel == LDG_TERM_LINE_SEL_NONE) { return LDG_ERR_AOK; }

    if (line->hist_sel == TERM_LINE_HIST_NEWEST)
    {
        line->hist_sel = LDG_TERM_LINE_SEL_NONE;

        return term_line_text_swap(line, line->draft);
    }

    line->hist_sel--;

    ret = term_line_hist_get(line, line->hist_sel, &text);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK || !text)) { return LDG_ERR_BOUNDS; }

    return term_line_text_swap(line, text);
}

static uint32_t term_line_search_run(ldg_term_line_t *line, uint32_t start)
{
    const char *text = 0x0;
    uint64_t at = 0;
    uint32_t cunt = 0;
    uint32_t idx = 0;

    line->search_sel = LDG_TERM_LINE_SEL_NONE;

    if (line->search_len == 0) { return LDG_ERR_AOK; }

    cunt = term_line_hist_cunt_get(line);

    for (idx = 0; idx < cunt; idx++)
    {
        if (term_line_hist_get(line, (start + idx) % cunt, &text) != LDG_ERR_AOK || !text) { continue; }

        if (ldg_str_substring_find(text, line->search, line->cap, &at) != LDG_ERR_AOK) { continue; }

        line->search_sel = (start + idx) % cunt;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_AOK;
}

static uint32_t term_line_search_resync(ldg_term_line_t *line)
{
    const char *text = 0x0;
    uint64_t at = 0;

    if (line->search_sel == LDG_TERM_LINE_SEL_NONE) { return term_line_search_run(line, TERM_LINE_HIST_NEWEST); }

    if (line->search_len > 0 && term_line_hist_get(line, line->search_sel, &text) == LDG_ERR_AOK && text && ldg_str_substring_find(text, line->search, line->cap, &at) == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    return term_line_search_run(line, line->search_sel);
}

static uint32_t term_line_search_key(ldg_term_line_t *line, uint8_t newer)
{
    const char *text = 0x0;
    uint64_t at = 0;
    uint32_t idx = 0;

    if (line->search_on != LDG_TRUTH_TRUE)
    {
        term_line_modes_close(line);
        line->search_on = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (newer != LDG_TRUTH_TRUE) { return term_line_search_run(line, (line->search_sel == LDG_TERM_LINE_SEL_NONE) ? TERM_LINE_HIST_NEWEST : line->search_sel + TERM_LINE_RING_STEP); }

    if (line->search_sel == LDG_TERM_LINE_SEL_NONE) { return LDG_ERR_AOK; }

    for (idx = line->search_sel; idx > 0; idx--)
    {
        if (term_line_hist_get(line, idx - TERM_LINE_PREV_OFF, &text) != LDG_ERR_AOK || !text || ldg_str_substring_find(text, line->search, line->cap, &at) != LDG_ERR_AOK) { continue; }

        line->search_sel = idx - TERM_LINE_PREV_OFF;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_AOK;
}

static uint32_t term_line_submit(ldg_term_line_t *line)
{
    const char *match = 0x0;
    uint32_t ret = 0;

    if (line->search_on == LDG_TRUTH_TRUE && line->search_sel == LDG_TERM_LINE_SEL_NONE)
    {
        term_line_modes_close(line);

        return LDG_ERR_AOK;
    }

    if (line->search_on == LDG_TRUTH_TRUE)
    {
        ret = term_line_hist_get(line, line->search_sel, &match);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK || !match)) { return LDG_ERR_BOUNDS; }

        ret = term_line_text_swap(line, match);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (line->candidate_on == LDG_TRUTH_TRUE && line->candidate_sel != LDG_TERM_LINE_SEL_NONE)
    {
        term_line_modes_close(line);

        return LDG_ERR_AOK;
    }

    term_line_modes_close(line);

    while (line->len > 0 && (line->text[line->len - TERM_LINE_PREV_OFF] == LDG_ASCII_SPACE || line->text[line->len - TERM_LINE_PREV_OFF] == LDG_ASCII_CR || line->text[line->len - TERM_LINE_PREV_OFF] == LDG_ASCII_LF)) { line->len--; }

    line->text[line->len] = LDG_STR_TERM;
    line->cursor = line->len;

    if (line->len == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_cpy(line->taken, line->text, (uint64_t)line->len + LDG_STR_TERM_SIZE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    line->rdy = LDG_TRUTH_TRUE;
    line->finish_due = LDG_TRUTH_TRUE;
    line->hist_sel = LDG_TERM_LINE_SEL_NONE;
    line->text[0] = LDG_STR_TERM;
    line->len = 0;
    line->cursor = 0;

    return LDG_ERR_AOK;
}

static uint32_t term_line_candidate_common_get(const ldg_term_line_t *line)
{
    uint32_t common = term_line_candidate_len_get(line->candidate[0]);
    uint32_t idx = 0;
    uint32_t at = 0;

    for (idx = 0; idx < line->candidate_cunt; idx++)
    {
        at = 0;
        while (at < common && line->candidate[idx][at] == line->candidate[0][at]) { at++; }

        common = at;
    }

    return common;
}

static uint8_t term_line_gap_wanted_is(const ldg_term_line_t *line)
{
    const char *word = line->text + line->candidate_at;
    uint32_t word_len = line->len - line->candidate_at;
    char last = 0;

    if (line->len == 0 || word_len == 0) { return LDG_TRUTH_FALSE; }

    if (word[0] == TERM_LINE_VAR_LEAD) { return LDG_TRUTH_FALSE; }

    if (word_len > TERM_LINE_ADDR_LEAD_AT && word[0] == TERM_LINE_ADDR_LEAD_FIRST && word[TERM_LINE_ADDR_LEAD_AT] == TERM_LINE_ADDR_LEAD_LAST) { return LDG_TRUTH_FALSE; }

    last = line->text[line->len - TERM_LINE_PREV_OFF];

    if (last == TERM_LINE_PATH_SEP || last == TERM_LINE_PATH_SEP_ALT || last == TERM_LINE_MEMBER_SEP || last == TERM_LINE_ASSIGN) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t term_line_tab(ldg_term_line_t *line, uint8_t back)
{
    uint32_t common = 0;
    uint32_t ret = 0;
    char gap = LDG_ASCII_SPACE;

    line->cursor = line->len;

    if (line->candidate_on != LDG_TRUTH_TRUE)
    {
        ret = term_line_candidates_fill(line, line->len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (line->candidate_cunt == 0) { return LDG_ERR_AOK; }

        common = term_line_candidate_common_get(line);

        if (common > line->cursor - line->candidate_at)
        {
            ret = term_line_span_swap(line, line->candidate_at, line->cursor, line->candidate[0], common);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (line->candidate_cunt > TERM_LINE_CANDIDATE_SOLE || line->candidate_more == LDG_TRUTH_TRUE)
        {
            line->candidate_on = LDG_TRUTH_TRUE;
            line->candidate_sel = LDG_TERM_LINE_SEL_NONE;

            return LDG_ERR_AOK;
        }

        line->candidate_cunt = 0;

        if (term_line_gap_wanted_is(line) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

        return term_line_span_swap(line, line->len, line->len, &gap, sizeof(char));
    }

    if (line->candidate_sel == LDG_TERM_LINE_SEL_NONE) { line->candidate_sel = (back == LDG_TRUTH_TRUE) ? line->candidate_cunt - TERM_LINE_RING_STEP : TERM_LINE_POS_FIRST; }
    else if (back == LDG_TRUTH_TRUE) { line->candidate_sel = (line->candidate_sel + line->candidate_cunt - TERM_LINE_RING_STEP) % line->candidate_cunt; }
    else { line->candidate_sel = (line->candidate_sel + TERM_LINE_RING_STEP) % line->candidate_cunt; }

    return term_line_span_swap(line, line->candidate_at, line->cursor, line->candidate[line->candidate_sel], term_line_candidate_len_get(line->candidate[line->candidate_sel]));
}

static uint32_t term_line_ch_put(ldg_term_line_t *line, uint32_t codepoint)
{
    char ch = (char)codepoint;

    if (codepoint < TERM_LINE_CH_MIN || codepoint > TERM_LINE_CH_MAX) { return LDG_ERR_AOK; }

    if (line->search_on == LDG_TRUTH_TRUE)
    {
        if (line->search_len + LDG_STR_TERM_SIZE >= line->cap) { return LDG_ERR_AOK; }

        line->search[line->search_len] = ch;
        line->search_len++;
        line->search[line->search_len] = LDG_STR_TERM;

        return term_line_search_resync(line);
    }

    line->candidate_on = LDG_TRUTH_FALSE;
    line->hist_sel = LDG_TERM_LINE_SEL_NONE;

    if (line->len + LDG_STR_TERM_SIZE >= line->cap) { return LDG_ERR_AOK; }

    return term_line_span_swap(line, line->cursor, line->cursor, &ch, sizeof(char));
}

static uint32_t term_line_erase(ldg_term_line_t *line, uint8_t fwd)
{
    if (line->search_on == LDG_TRUTH_TRUE)
    {
        if (line->search_len == 0) { return LDG_ERR_AOK; }

        line->search_len--;
        line->search[line->search_len] = LDG_STR_TERM;

        return term_line_search_resync(line);
    }

    line->candidate_on = LDG_TRUTH_FALSE;

    if (fwd == LDG_TRUTH_TRUE)
    {
        if (line->cursor >= line->len) { return LDG_ERR_AOK; }

        return term_line_span_swap(line, line->cursor, line->cursor + TERM_LINE_PREV_OFF, line->text, TERM_LINE_OFF_NONE);
    }

    if (line->cursor == 0) { return LDG_ERR_AOK; }

    return term_line_span_swap(line, line->cursor - TERM_LINE_PREV_OFF, line->cursor, line->text, TERM_LINE_OFF_NONE);
}

static uint32_t term_line_ctrl_key(ldg_term_line_t *line, uint32_t codepoint, uint8_t shift, uint8_t *out_consumed)
{
    if (codepoint < TERM_LINE_CH_MIN) { codepoint += TERM_LINE_CTRL_OFF; }

    if (codepoint >= TERM_LINE_UPPER_FIRST && codepoint <= TERM_LINE_UPPER_LAST) { codepoint += TERM_LINE_CASE_OFF; }

    *out_consumed = LDG_TRUTH_TRUE;

    if (codepoint == TERM_LINE_KEY_SEARCH) { return term_line_search_key(line, shift); }

    if (codepoint == TERM_LINE_KEY_EOF) { return LDG_ERR_EOF; }

    if (codepoint == TERM_LINE_KEY_SEARCH_CANCEL)
    {
        term_line_modes_close(line);

        return LDG_ERR_AOK;
    }

    if (codepoint == TERM_LINE_KEY_WORD_DROP && line->search_on == LDG_TRUTH_TRUE)
    {
        while (line->search_len > 0 && line->search[line->search_len - TERM_LINE_PREV_OFF] == LDG_ASCII_SPACE) { line->search_len--; }

        while (line->search_len > 0 && line->search[line->search_len - TERM_LINE_PREV_OFF] != LDG_ASCII_SPACE) { line->search_len--; }

        line->search[line->search_len] = LDG_STR_TERM;

        return term_line_search_resync(line);
    }

    if (codepoint == TERM_LINE_KEY_CLEAR)
    {
        term_line_modes_close(line);
        line->hist_sel = LDG_TERM_LINE_SEL_NONE;

        return term_line_span_swap(line, TERM_LINE_POS_FIRST, line->len, line->text, TERM_LINE_OFF_NONE);
    }

    if (line->search_on == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (codepoint == TERM_LINE_KEY_GHOST_WORD) { return term_line_ghost_take(line, LDG_TRUTH_FALSE); }

    if (codepoint == TERM_LINE_KEY_HOME || codepoint == TERM_LINE_KEY_END)
    {
        term_line_modes_close(line);
        line->cursor = (codepoint == TERM_LINE_KEY_HOME) ? TERM_LINE_POS_FIRST : line->len;

        return LDG_ERR_AOK;
    }

    if (codepoint == TERM_LINE_KEY_DROP_HD)
    {
        term_line_modes_close(line);

        return term_line_span_swap(line, TERM_LINE_POS_FIRST, line->cursor, line->text, TERM_LINE_OFF_NONE);
    }

    if (codepoint == TERM_LINE_KEY_DROP_TAIL)
    {
        term_line_modes_close(line);

        return term_line_span_swap(line, line->cursor, line->len, line->text, TERM_LINE_OFF_NONE);
    }

    if (codepoint == TERM_LINE_KEY_WORD_DROP)
    {
        term_line_modes_close(line);

        return term_line_span_swap(line, term_line_word_start_get(line, line->cursor), line->cursor, line->text, TERM_LINE_OFF_NONE);
    }

    *out_consumed = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint32_t term_line_named_key(ldg_term_line_t *line, const ldg_term_key_event_t *event, uint8_t *out_consumed)
{
    uint8_t ctrl = ((event->mods & LDG_TERM_MOD_CTRL) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t shift = ((event->mods & LDG_TERM_MOD_SHIFT) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t listed = LDG_TRUTH_FALSE;

    *out_consumed = LDG_TRUTH_TRUE;

    if (event->key == LDG_TERM_KEY_ENTER) { return term_line_submit(line); }

    if (event->key == LDG_TERM_KEY_TAB) { return term_line_tab(line, shift); }

    if (event->key == LDG_TERM_KEY_BACKSPACE && ctrl == LDG_TRUTH_TRUE) { return term_line_ctrl_key(line, TERM_LINE_KEY_WORD_DROP, shift, out_consumed); }

    if (event->key == LDG_TERM_KEY_BACKSPACE) { return term_line_erase(line, LDG_TRUTH_FALSE); }

    if (event->key == LDG_TERM_KEY_DELETE) { return term_line_erase(line, LDG_TRUTH_TRUE); }

    if (event->key == LDG_TERM_KEY_ESCAPE)
    {
        if (line->candidate_on != LDG_TRUTH_TRUE && line->search_on != LDG_TRUTH_TRUE)
        {
            *out_consumed = (line->ghost_len > 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
            line->ghost_hidden = LDG_TRUTH_TRUE;
        }

        term_line_modes_close(line);

        return LDG_ERR_AOK;
    }

    if (line->search_on == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (event->key == LDG_TERM_KEY_UP || event->key == LDG_TERM_KEY_PAGE_UP || event->key == LDG_TERM_KEY_DOWN || event->key == LDG_TERM_KEY_PAGE_DOWN)
    {
        term_line_modes_close(line);

        return term_line_hist_step(line, (event->key == LDG_TERM_KEY_UP || event->key == LDG_TERM_KEY_PAGE_UP) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE);
    }

    if (event->key == LDG_TERM_KEY_RHS || event->key == LDG_TERM_KEY_END)
    {
        listed = line->candidate_on;
        term_line_modes_close(line);

        if (listed == LDG_TRUTH_TRUE && event->key == LDG_TERM_KEY_RHS && ctrl != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

        if (line->cursor == line->len) { return term_line_ghost_take(line, (ctrl == LDG_TRUTH_TRUE) ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE); }

        if (event->key == LDG_TERM_KEY_END) { line->cursor = line->len; }
        else if (ctrl == LDG_TRUTH_TRUE) { line->cursor = term_line_word_end_get(line, line->cursor); }
        else { line->cursor++; }

        return LDG_ERR_AOK;
    }

    if (event->key == LDG_TERM_KEY_LHS || event->key == LDG_TERM_KEY_HOME)
    {
        term_line_modes_close(line);

        if (event->key == LDG_TERM_KEY_HOME) { line->cursor = TERM_LINE_POS_FIRST; }
        else if (ctrl == LDG_TRUTH_TRUE) { line->cursor = term_line_word_start_get(line, line->cursor); }
        else if (line->cursor > 0) { line->cursor--; }

        return LDG_ERR_AOK;
    }

    *out_consumed = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_line_key_put(ldg_term_line_t *line, const ldg_term_key_event_t *event, uint8_t *out_consumed)
{
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!line || !event || !out_consumed)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_consumed = LDG_TRUTH_FALSE;

    if (event->kind == LDG_TERM_KEY_KIND_RELEASE) { return LDG_ERR_AOK; }

    line->ghost_hidden = LDG_TRUTH_FALSE;

    if (event->key != LDG_TERM_KEY_NONE) { ret = term_line_named_key(line, event, out_consumed); }
    else if ((event->mods & LDG_TERM_MOD_CTRL) != 0 || (event->codepoint != 0 && event->codepoint < TERM_LINE_CH_MIN)) { ret = term_line_ctrl_key(line, event->codepoint, ((event->mods & LDG_TERM_MOD_SHIFT) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, out_consumed); }
    else if ((event->mods & (LDG_TERM_MOD_ALT | LDG_TERM_MOD_SUPER | LDG_TERM_MOD_META | LDG_TERM_MOD_HYPER)) == 0)
    {
        *out_consumed = LDG_TRUTH_TRUE;

        if (event->text_cunt == 0) { ret = term_line_ch_put(line, ((event->mods & LDG_TERM_MOD_SHIFT) != 0 && event->shifted != 0) ? event->shifted : event->codepoint); }

        for (idx = 0; idx < event->text_cunt && idx < LDG_TERM_KEY_TEXT_MAX && ret == LDG_ERR_AOK; idx++) { ret = term_line_ch_put(line, event->text[idx]); }
    }

    if (ret != LDG_ERR_AOK) { return ret; }

    return term_line_ghost_update(line);
}

uint32_t ldg_term_line_conf_default_get(ldg_term_line_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->line_cap = LDG_TERM_LINE_CAP_DEFAULT;
    out->candidate_max = LDG_TERM_LINE_CANDIDATE_MAX_DEFAULT;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_line_pool_size_get(const ldg_term_line_conf_t *conf, uint64_t *out)
{
    uint64_t total = 0;

    if (LDG_UNLIKELY(!conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(conf->line_cap < LDG_TERM_LINE_CAP_MIN || conf->candidate_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    total = (uint64_t)sizeof(ldg_term_line_t);
    total += (uint64_t)TERM_LINE_POOL_ALLOC_CUNT * (uint64_t)LDG_MEM_POOL_VAR_ALIGN;
    total += (uint64_t)conf->line_cap * (uint64_t)TERM_LINE_TEXT_BUFF_CUNT;
    total += (uint64_t)conf->candidate_max * (uint64_t)LDG_TERM_LINE_CANDIDATE_CAP;

    *out = total;

    return LDG_ERR_AOK;
}

static uint32_t term_line_buff_take(ldg_mem_pool_t *pool, uint64_t size, void **out)
{
    uint32_t ret = 0;

    ret = ldg_mem_pool_alloc(pool, size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

uint32_t ldg_term_line_create(ldg_mem_pool_t *pool, const ldg_term_line_conf_t *conf, ldg_term_line_t **out)
{
    ldg_term_line_t *line = 0x0;
    uint64_t need = 0;
    uint64_t cap = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    ret = ldg_term_line_pool_size_get(conf, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = term_line_buff_take(pool, sizeof(ldg_term_line_t), (void **)&line);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cap = (uint64_t)conf->line_cap;

    ret = term_line_buff_take(pool, cap, (void **)&line->text);
    if (ret == LDG_ERR_AOK) { ret = term_line_buff_take(pool, cap, (void **)&line->ghost); }

    if (ret == LDG_ERR_AOK) { ret = term_line_buff_take(pool, cap, (void **)&line->draft); }

    if (ret == LDG_ERR_AOK) { ret = term_line_buff_take(pool, cap, (void **)&line->search); }

    if (ret == LDG_ERR_AOK) { ret = term_line_buff_take(pool, cap, (void **)&line->taken); }

    if (ret == LDG_ERR_AOK) { ret = term_line_buff_take(pool, (uint64_t)conf->candidate_max * (uint64_t)LDG_TERM_LINE_CANDIDATE_CAP, (void **)&line->candidate); }

    if (ret == LDG_ERR_AOK) { ret = ldg_str_rbrcpy(line->prompt, LDG_TERM_LINE_PROMPT_DEFAULT, LDG_TERM_LINE_PROMPT_CAP); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    line->cap = conf->line_cap;
    line->candidate_max = conf->candidate_max;
    line->hist_sel = LDG_TERM_LINE_SEL_NONE;
    line->search_sel = LDG_TERM_LINE_SEL_NONE;
    line->candidate_sel = LDG_TERM_LINE_SEL_NONE;
    *out = line;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_line_uncreate(ldg_term_line_t **line)
{
    if (LDG_UNLIKELY(!line || !*line)) { return LDG_ERR_FUNC_ARG_NULL; }

    *line = 0x0;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_line_rst(ldg_term_line_t *line)
{
    if (LDG_UNLIKELY(!line)) { return LDG_ERR_FUNC_ARG_NULL; }

    term_line_modes_close(line);
    line->text[0] = LDG_STR_TERM;
    line->ghost[0] = LDG_STR_TERM;
    line->hint[0] = LDG_STR_TERM;
    line->len = 0;
    line->cursor = 0;
    line->ghost_len = 0;
    line->hist_sel = LDG_TERM_LINE_SEL_NONE;
    line->draw_row = 0;
    line->rdy = LDG_TRUTH_FALSE;
    line->ghost_hidden = LDG_TRUTH_FALSE;
    line->finish_due = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_line_text_get(const ldg_term_line_t *line, const char **out, uint32_t *out_len)
{
    if (LDG_UNLIKELY(!line || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = line->text;
    *out_len = line->len;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_line_text_set(ldg_term_line_t *line, const char *text)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!line || !text)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(term_line_len_get(text, line->cap) >= line->cap)) { return LDG_ERR_OVERFLOW; }

    term_line_modes_close(line);
    line->hist_sel = LDG_TERM_LINE_SEL_NONE;

    ret = term_line_text_swap(line, text);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return term_line_ghost_update(line);
}

uint32_t ldg_term_line_take(ldg_term_line_t *line, char *out, uint64_t cap)
{
    uint32_t len = 0;

    if (LDG_UNLIKELY(!line || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (line->rdy != LDG_TRUTH_TRUE) { return LDG_ERR_EMPTY; }

    len = term_line_len_get(line->taken, line->cap);
    if (LDG_UNLIKELY((uint64_t)len + LDG_STR_TERM_SIZE > cap)) { return LDG_ERR_OVERFLOW; }

    line->rdy = LDG_TRUTH_FALSE;

    return ldg_mem_cpy(out, line->taken, (uint64_t)len + LDG_STR_TERM_SIZE);
}

uint32_t ldg_term_line_prompt_set(ldg_term_line_t *line, const char *prompt)
{
    if (LDG_UNLIKELY(!line || !prompt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(term_line_len_get(prompt, LDG_TERM_LINE_PROMPT_CAP) >= LDG_TERM_LINE_PROMPT_CAP)) { return LDG_ERR_OVERFLOW; }

    return ldg_str_rbrcpy(line->prompt, prompt, LDG_TERM_LINE_PROMPT_CAP);
}

uint32_t ldg_term_line_hist_cb_set(ldg_term_line_t *line, ldg_term_line_hist_cb_t cb, void *user)
{
    if (LDG_UNLIKELY(!line)) { return LDG_ERR_FUNC_ARG_NULL; }

    line->hist_cb = cb;
    line->hist_user = user;
    line->hist_sel = LDG_TERM_LINE_SEL_NONE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_line_candidate_cb_set(ldg_term_line_t *line, ldg_term_line_candidate_cb_t cb, void *user)
{
    if (LDG_UNLIKELY(!line)) { return LDG_ERR_FUNC_ARG_NULL; }

    line->candidate_cb = cb;
    line->candidate_user = user;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_line_view_get(const ldg_term_line_t *line, ldg_term_line_view_t *out)
{
    if (LDG_UNLIKELY(!line || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->prompt = line->prompt;
    out->text = line->text;
    out->ghost = line->ghost;
    out->hint = line->hint;
    out->candidate = (const char (*)[LDG_TERM_LINE_CANDIDATE_CAP]) line->candidate;
    out->prompt_len = term_line_len_get(line->prompt, LDG_TERM_LINE_PROMPT_CAP);
    out->text_len = line->len;
    out->ghost_len = line->ghost_len;
    out->hint_len = term_line_len_get(line->hint, LDG_TERM_LINE_HINT_CAP);
    out->cursor = line->cursor;
    out->candidate_cunt = (line->candidate_on == LDG_TRUTH_TRUE) ? line->candidate_cunt : TERM_LINE_OFF_NONE;
    out->candidate_sel = line->candidate_sel;
    out->search_on = line->search_on;
    out->candidate_on = line->candidate_on;
    out->candidate_more = line->candidate_more;

    if (line->search_on == LDG_TRUTH_TRUE)
    {
        out->text = line->search;
        out->text_len = line->search_len;
        out->cursor = line->search_len;
    }

    return LDG_ERR_AOK;
}

static uint32_t term_line_pad(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t cunt, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg)
{
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;
    char blank = LDG_ASCII_SPACE;

    for (idx = 0; idx < cunt && ret == LDG_ERR_AOK; idx++) { ret = ldg_term_render_text_draw(render, col + idx, row, &blank, sizeof(char), fg, bg); }

    return ret;
}

static uint32_t term_line_span_draw(ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt, const char *text, uint32_t len, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg)
{
    uint32_t shown = (len < col_cunt) ? len : col_cunt;
    uint32_t ret = 0;

    if (shown > 0)
    {
        ret = ldg_term_render_text_draw(render, col, row, text, shown, fg, bg);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return term_line_pad(render, col + shown, row, col_cunt - shown, fg, bg);
}

static uint32_t term_line_candidates_draw(const ldg_term_line_view_t *view, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt, uint32_t row_cunt, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *ghost_fg, const ldg_term_render_color_t *bg)
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
        len = term_line_len_get(view->candidate[idx], LDG_TERM_LINE_CANDIDATE_CAP);
        if (len > width) { width = len; }
    }

    width += TERM_LINE_CANDIDATE_GAP;
    if (width > col_cunt) { width = col_cunt; }

    per_row = col_cunt / width;

    if (view->candidate_sel != LDG_TERM_LINE_SEL_NONE && view->candidate_sel / per_row >= row_cunt) { row_first = view->candidate_sel / per_row - row_cunt + TERM_LINE_ROW_CUNT_LINE; }

    for (idx = 0; idx < row_cunt && ret == LDG_ERR_AOK; idx++) { ret = term_line_pad(render, col, row + idx, col_cunt, fg, bg); }

    for (idx = row_first * per_row; idx < view->candidate_cunt && ret == LDG_ERR_AOK; idx++)
    {
        grid_row = idx / per_row - row_first;
        grid_col = idx % per_row;

        if (grid_row >= row_cunt) { break; }

        len = term_line_len_get(view->candidate[idx], LDG_TERM_LINE_CANDIDATE_CAP);
        if (len > width) { len = width; }

        val_len = term_line_candidate_len_get(view->candidate[idx]);
        if (val_len > len) { val_len = len; }

        sel = (idx == view->candidate_sel) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (val_len > 0) { ret = ldg_term_render_text_draw(render, col + grid_col * width, row + grid_row, view->candidate[idx], val_len, sel ? bg : fg, sel ? fg : bg); }

        if (ret == LDG_ERR_AOK && val_len + TERM_LINE_CURSOR_WIDTH < len) { ret = ldg_term_render_text_draw(render, col + grid_col * width + val_len + TERM_LINE_CURSOR_WIDTH, row + grid_row, view->candidate[idx] + val_len + TERM_LINE_CURSOR_WIDTH, len - val_len - TERM_LINE_CURSOR_WIDTH, ghost_fg, bg); }
    }

    return ret;
}

uint32_t ldg_term_line_draw(const ldg_term_line_t *line, ldg_term_render_t *render, uint32_t col, uint32_t row, uint32_t col_cunt, uint32_t row_cunt, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *ghost_fg, const ldg_term_render_color_t *bg)
{
    ldg_term_line_view_t view = LDG_STRUCT_ZERO_INIT;
    uint32_t above = 0;
    uint32_t text_cols = 0;
    uint32_t off = 0;
    uint32_t shown = 0;
    uint32_t ghost_shown = 0;
    uint32_t text_col = 0;
    uint32_t line_row = 0;
    uint32_t ret = 0;
    char cursor_ch = LDG_ASCII_SPACE;

    if (LDG_UNLIKELY(!line || !render || !fg || !ghost_fg || !bg)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_term_line_view_get(line, &view);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(col_cunt <= view.prompt_len + TERM_LINE_CURSOR_WIDTH)) { return LDG_ERR_FUNC_ARG_INVALID; }

    above = row_cunt - TERM_LINE_ROW_CUNT_LINE;
    line_row = row + above;

    if (above > 0 && view.candidate_on == LDG_TRUTH_TRUE && view.candidate_cunt > 0)
    {
        ret = term_line_candidates_draw(&view, render, col, row, col_cunt, above, fg, ghost_fg, bg);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    text_cols = col_cunt - view.prompt_len;
    off = (view.cursor >= text_cols) ? view.cursor - text_cols + TERM_LINE_CURSOR_WIDTH : TERM_LINE_OFF_NONE;
    shown = (view.text_len - off < text_cols) ? view.text_len - off : text_cols;
    ghost_shown = (view.ghost_len < text_cols - shown) ? view.ghost_len : text_cols - shown;
    text_col = col + view.prompt_len;

    ret = ldg_term_render_text_draw(render, col, line_row, view.prompt, view.prompt_len, fg, bg);
    if (ret == LDG_ERR_AOK) { ret = term_line_span_draw(render, text_col, line_row, shown, view.text + off, shown, fg, bg); }

    if (ret == LDG_ERR_AOK) { ret = term_line_span_draw(render, text_col + shown, line_row, text_cols - shown, view.ghost, ghost_shown, ghost_fg, bg); }

    if (ret == LDG_ERR_AOK && view.hint_len > 0 && shown + ghost_shown + TERM_LINE_HINT_GAP + TERM_LINE_CURSOR_WIDTH + view.hint_len <= text_cols) { ret = ldg_term_render_text_draw(render, col + col_cunt - view.hint_len, line_row, view.hint, view.hint_len, ghost_fg, bg); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (view.cursor < view.text_len) { cursor_ch = view.text[view.cursor]; }
    else if (view.ghost_len > 0) { cursor_ch = view.ghost[0]; }

    return ldg_term_render_text_draw(render, text_col + view.cursor - off, line_row, &cursor_ch, sizeof(char), bg, fg);
}

static uint32_t term_line_sink_put(term_line_sink_t *sink, const char *text, uint64_t len)
{
    uint32_t ret = 0;

    if (len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(sink->len + len > sink->cap)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_cpy(sink->out + sink->len, text, len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sink->len += len;

    return LDG_ERR_AOK;
}

static uint32_t term_line_sink_seq_put(term_line_sink_t *sink, const char *fmt, uint32_t val)
{
    char seq[TERM_LINE_SEQ_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t seq_len = 0;
    uint32_t ret = 0;

    ret = ldg_str_fmt(seq, TERM_LINE_SEQ_CAP, &seq_len, fmt, val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return term_line_sink_put(sink, seq, seq_len);
}

static uint32_t term_line_candidates_encode(const ldg_term_line_view_t *view, term_line_sink_t *sink, uint32_t col_cunt, uint32_t *rows)
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
        len = term_line_len_get(view->candidate[idx], LDG_TERM_LINE_CANDIDATE_CAP);
        if (len > width) { width = len; }
    }

    width += TERM_LINE_CANDIDATE_GAP;
    per_row = (width < col_cunt) ? col_cunt / width : TERM_LINE_CANDIDATE_SOLE;

    if (view->candidate_sel != LDG_TERM_LINE_SEL_NONE && view->candidate_sel / per_row >= TERM_LINE_ENCODE_ROW_MAX) { row_first = view->candidate_sel / per_row - TERM_LINE_ENCODE_ROW_MAX + TERM_LINE_ROW_CUNT_LINE; }

    for (idx = row_first * per_row; idx < view->candidate_cunt && idx < (row_first + TERM_LINE_ENCODE_ROW_MAX) * per_row && ret == LDG_ERR_AOK; idx++)
    {
        if (idx % per_row == 0)
        {
            ret = term_line_sink_put(sink, TERM_LINE_ROW_END, sizeof(TERM_LINE_ROW_END) - LDG_STR_TERM_SIZE);
            (*rows)++;
        }

        len = term_line_len_get(view->candidate[idx], LDG_TERM_LINE_CANDIDATE_CAP);
        if (len + TERM_LINE_CANDIDATE_GAP >= col_cunt) { len = col_cunt - TERM_LINE_CANDIDATE_GAP - TERM_LINE_COL_BASE; }

        val_len = term_line_candidate_len_get(view->candidate[idx]);
        if (val_len > len) { val_len = len; }

        if (ret == LDG_ERR_AOK && idx == view->candidate_sel) { ret = term_line_sink_put(sink, TERM_LINE_SEL_ON, sizeof(TERM_LINE_SEL_ON) - LDG_STR_TERM_SIZE); }

        if (ret == LDG_ERR_AOK) { ret = term_line_sink_put(sink, view->candidate[idx], val_len); }

        if (ret == LDG_ERR_AOK && idx == view->candidate_sel) { ret = term_line_sink_put(sink, TERM_LINE_SEL_OFF, sizeof(TERM_LINE_SEL_OFF) - LDG_STR_TERM_SIZE); }

        if (ret == LDG_ERR_AOK && val_len + TERM_LINE_CURSOR_WIDTH < len)
        {
            ret = term_line_sink_put(sink, TERM_LINE_GHOST_ON, sizeof(TERM_LINE_GHOST_ON) - LDG_STR_TERM_SIZE);
            if (ret == LDG_ERR_AOK) { ret = term_line_sink_put(sink, TERM_LINE_GAP, TERM_LINE_CURSOR_WIDTH); }

            if (ret == LDG_ERR_AOK) { ret = term_line_sink_put(sink, view->candidate[idx] + val_len + TERM_LINE_CURSOR_WIDTH, len - val_len - TERM_LINE_CURSOR_WIDTH); }

            if (ret == LDG_ERR_AOK) { ret = term_line_sink_put(sink, TERM_LINE_GHOST_OFF, sizeof(TERM_LINE_GHOST_OFF) - LDG_STR_TERM_SIZE); }
        }

        if (ret == LDG_ERR_AOK) { ret = term_line_sink_put(sink, TERM_LINE_GAP, sizeof(TERM_LINE_GAP) - LDG_STR_TERM_SIZE); }
    }

    return ret;
}

uint32_t ldg_term_line_encode(ldg_term_line_t *line, uint32_t col_cunt, ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    ldg_term_line_view_t view = LDG_STRUCT_ZERO_INIT;
    term_line_sink_t sink = LDG_STRUCT_ZERO_INIT;
    uint32_t total = 0;
    uint32_t rows = 0;
    uint32_t tgt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!line || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    ret = ldg_term_line_view_get(line, &view);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(col_cunt <= view.prompt_len + TERM_LINE_CANDIDATE_GAP + TERM_LINE_COL_BASE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    sink.out = out;
    sink.cap = cap;

    if (line->draw_row > 0) { ret = term_line_sink_seq_put(&sink, TERM_LINE_SEQ_UP_FMT, line->draw_row); }

    if (ret == LDG_ERR_AOK) { ret = term_line_sink_put(&sink, TERM_LINE_ERASE_BELOW, sizeof(TERM_LINE_ERASE_BELOW) - LDG_STR_TERM_SIZE); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (line->finish_due == LDG_TRUTH_TRUE)
    {
        ret = term_line_sink_put(&sink, view.prompt, view.prompt_len);
        if (ret == LDG_ERR_AOK) { ret = term_line_sink_put(&sink, line->taken, term_line_len_get(line->taken, line->cap)); }

        if (ret == LDG_ERR_AOK) { ret = term_line_sink_put(&sink, TERM_LINE_ROW_END, sizeof(TERM_LINE_ROW_END) - LDG_STR_TERM_SIZE); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        line->finish_due = LDG_TRUTH_FALSE;
        line->draw_row = 0;
        *out_len = sink.len;

        return LDG_ERR_AOK;
    }

    ret = term_line_sink_put(&sink, view.prompt, view.prompt_len);
    if (ret == LDG_ERR_AOK) { ret = term_line_sink_put(&sink, view.text, view.text_len); }

    if (ret == LDG_ERR_AOK && view.ghost_len > 0) { ret = term_line_sink_put(&sink, TERM_LINE_GHOST_ON, sizeof(TERM_LINE_GHOST_ON) - LDG_STR_TERM_SIZE); }

    if (ret == LDG_ERR_AOK) { ret = term_line_sink_put(&sink, view.ghost, view.ghost_len); }

    if (ret == LDG_ERR_AOK && view.ghost_len > 0) { ret = term_line_sink_put(&sink, TERM_LINE_GHOST_OFF, sizeof(TERM_LINE_GHOST_OFF) - LDG_STR_TERM_SIZE); }

    total = view.prompt_len + view.text_len + view.ghost_len;

    if (ret == LDG_ERR_AOK && total % col_cunt == 0) { ret = term_line_sink_put(&sink, TERM_LINE_WRAP_SETTLE, sizeof(TERM_LINE_WRAP_SETTLE) - LDG_STR_TERM_SIZE); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    rows = total / col_cunt;

    if (view.candidate_on == LDG_TRUTH_TRUE && view.candidate_cunt > 0) { ret = term_line_candidates_encode(&view, &sink, col_cunt, &rows); }
    else if (view.hint_len > 0)
    {
        ret = term_line_sink_put(&sink, TERM_LINE_ROW_END, sizeof(TERM_LINE_ROW_END) - LDG_STR_TERM_SIZE);
        if (ret == LDG_ERR_AOK) { ret = term_line_sink_put(&sink, TERM_LINE_GHOST_ON, sizeof(TERM_LINE_GHOST_ON) - LDG_STR_TERM_SIZE); }

        if (ret == LDG_ERR_AOK) { ret = term_line_sink_put(&sink, view.hint, (view.hint_len < col_cunt) ? view.hint_len : col_cunt - TERM_LINE_COL_BASE); }

        if (ret == LDG_ERR_AOK) { ret = term_line_sink_put(&sink, TERM_LINE_GHOST_OFF, sizeof(TERM_LINE_GHOST_OFF) - LDG_STR_TERM_SIZE); }

        rows++;
    }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    tgt = (view.prompt_len + view.cursor) / col_cunt;

    if (rows > tgt) { ret = term_line_sink_seq_put(&sink, TERM_LINE_SEQ_UP_FMT, rows - tgt); }

    if (ret == LDG_ERR_AOK) { ret = term_line_sink_seq_put(&sink, TERM_LINE_SEQ_COL_FMT, (view.prompt_len + view.cursor) % col_cunt + TERM_LINE_COL_BASE); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    line->draw_row = tgt;
    *out_len = sink.len;

    return LDG_ERR_AOK;
}
