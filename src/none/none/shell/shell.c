#include <stdint.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>
#include <dangling/mem/cpy.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>
#include <dangling/shell/shell.h>
#include <dangling/shell/cmd.h>

#define SHELL_POOL_ALLOC_CUNT 11
#define SHELL_HIST_BUFF_CUNT 2
#define SHELL_ARG_BUFF_FACTOR 2
#define SHELL_EXEC_BUFF_CUNT (SHELL_ARG_BUFF_FACTOR + 1)
#define SHELL_STACK_TOP_OFF 1
#define SHELL_AOK_LEAF_CUNT 1
#define SHELL_PLAIN_ITEM_CUNT 1
#define SHELL_CMD_NAME_CUNT 1
#define SHELL_WORD_FIRST_CUNT 1
#define SHELL_ARG_CUNT_NONE 0
#define SHELL_NAME_ARG_CUNT 1
#define SHELL_SPAN_NONE 0
#define SHELL_PREV_OFF 1
#define SHELL_VAR_BRACE_AT 1
#define SHELL_ADDR_LEAD_LEN 2
#define SHELL_ADDR_LEAD_FIRST '0'
#define SHELL_ADDR_LEAD_LAST 'x'
#define SHELL_ADDR_LABEL "observed address"
#define SHELL_CANDIDATE_CH_MIN 0x20
#define SHELL_CANDIDATE_CH_MAX 0x7E
#define SHELL_CANDIDATE_VAR_FMT "$%s"
#define SHELL_CANDIDATE_VAR_BRACED_FMT "${%s}"
#define SHELL_CANDIDATE_SET_FMT "%s="
#define SHELL_CANDIDATE_NAME_FMT "%s"
#define SHELL_CH_LIST_SEP ','
#define SHELL_ITEM_FIRST 0
#define SHELL_QUOTE_NONE 0x0
#define SHELL_CH_QUOTE_DOUBLE '"'
#define SHELL_CH_QUOTE_SINGLE '\''
#define SHELL_CH_VAR '$'
#define SHELL_CH_BRACE_OPEN '{'
#define SHELL_CH_BRACE_CLOSE '}'
#define SHELL_CH_PAREN_OPEN '('
#define SHELL_CH_PAREN_CLOSE ')'
#define SHELL_CH_STMT_END ';'
#define SHELL_CH_UNDERSCORE '_'
#define SHELL_MSG_CAP 48
#define SHELL_MSG_UNKNOWN "awwBRN: unknown cmd: "
#define SHELL_MSG_REFUSED "awwBRN: cmd refused\n"
#define SHELL_MSG_LF "\n"
#define SHELL_MSG_NONE ""
#define SHELL_MSG_OK "ok\n"
#define SHELL_MSG_ERR_FMT "err: %u32\n"
#define SHELL_MSG_TRUE "true\n"
#define SHELL_MSG_FALSE "false\n"
#define SHELL_MSG_MAYBE "maybe\n"
#define SHELL_MSG_INDETERMINATE "indeterminate\n"
#define SHELL_SEP_HELP ": "
#define SHELL_SEP_PAIR "="
#define SHELL_CMD_HELP "help"
#define SHELL_CMD_SET "set"
#define SHELL_CMD_UNSET "unset"
#define SHELL_CMD_VARS "vars"
#define SHELL_CMD_ALIASES "aliases"
#define SHELL_CH_ASSIGN '='
#define SHELL_HELP_HELP "help [cmd]; lists every cmd, or the help of one"
#define SHELL_HELP_SET "set name=text; sets a variable, read back as $name or ${name}"
#define SHELL_HELP_UNSET "unset <name>; drops a variable"
#define SHELL_HELP_VARS "vars; lists every variable"
#define SHELL_HELP_ALIASES "aliases; lists every alias; name=text sets one, name= drops it"

typedef enum shell_expr_kind
{
    SHELL_EXPR_LEAF = 0,
    SHELL_EXPR_LEAF_AOK,
    SHELL_EXPR_NOT,
    SHELL_EXPR_AND,
    SHELL_EXPR_XOR,
    SHELL_EXPR_OR,
    SHELL_EXPR_IMPL,
    SHELL_EXPR_IFF,
    SHELL_EXPR_PAREN,
    SHELL_EXPR_AOK_OPEN,
    SHELL_EXPR_KIND_CUNT
} shell_expr_kind_t;

static uint32_t shell_len_get(const char *text, uint32_t cap)
{
    uint32_t len = 0;

    while (len < cap && text[len] != LDG_STR_TERM) { len++; }

    return len;
}

static uint8_t shell_space_is(char ch)
{
    return (ch == LDG_ASCII_SPACE || ch == LDG_ASCII_TAB) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t shell_name_ch_is(char ch)
{
    if (ch == SHELL_CH_UNDERSCORE) { return LDG_TRUTH_TRUE; }

    return (ldg_char_alnum_is((uint8_t)ch) == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t shell_name_valid_is(const char *name, uint32_t len)
{
    uint32_t idx = 0;

    if (len == 0 || len >= LDG_SHELL_NAME_CAP) { return LDG_TRUTH_FALSE; }

    if (ldg_char_digit_is((uint8_t)name[0]) == LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    for (idx = 0; idx < len; idx++) { if (shell_name_ch_is(name[idx]) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static uint8_t shell_span_eq_is(const char *name, const char *text, uint32_t len)
{
    uint32_t idx = 0;

    for (idx = 0; idx < len; idx++) { if (name[idx] == LDG_STR_TERM || name[idx] != text[idx]) { return LDG_TRUTH_FALSE; } }

    return (name[len] == LDG_STR_TERM) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t shell_quote_next_get(uint8_t quote, char ch)
{
    if (quote == SHELL_QUOTE_NONE && (ch == SHELL_CH_QUOTE_DOUBLE || ch == SHELL_CH_QUOTE_SINGLE)) { return (uint8_t)ch; }

    if (quote != SHELL_QUOTE_NONE && (uint8_t)ch == quote) { return SHELL_QUOTE_NONE; }

    return quote;
}

static uint32_t shell_word_end_get(const char *text, uint32_t len, uint32_t pos)
{
    uint8_t quote = SHELL_QUOTE_NONE;

    while (pos < len)
    {
        if (quote == SHELL_QUOTE_NONE && (shell_space_is(text[pos]) == LDG_TRUTH_TRUE || text[pos] == SHELL_CH_PAREN_OPEN || text[pos] == SHELL_CH_PAREN_CLOSE || text[pos] == SHELL_CH_STMT_END)) { break; }

        quote = shell_quote_next_get(quote, text[pos]);
        pos++;
    }

    return pos;
}

static uint32_t shell_word_kind_get(const char *text, uint32_t len)
{
    if (shell_span_eq_is(LDG_SHELL_WORD_NOT, text, len) == LDG_TRUTH_TRUE) { return SHELL_EXPR_NOT; }

    if (shell_span_eq_is(LDG_SHELL_WORD_AND, text, len) == LDG_TRUTH_TRUE) { return SHELL_EXPR_AND; }

    if (shell_span_eq_is(LDG_SHELL_WORD_XOR, text, len) == LDG_TRUTH_TRUE) { return SHELL_EXPR_XOR; }

    if (shell_span_eq_is(LDG_SHELL_WORD_OR, text, len) == LDG_TRUTH_TRUE) { return SHELL_EXPR_OR; }

    if (shell_span_eq_is(LDG_SHELL_WORD_IMPLIES, text, len) == LDG_TRUTH_TRUE) { return SHELL_EXPR_IMPL; }

    if (shell_span_eq_is(LDG_SHELL_WORD_IFF, text, len) == LDG_TRUTH_TRUE) { return SHELL_EXPR_IFF; }

    if (shell_span_eq_is(LDG_SHELL_WORD_AOK, text, len) == LDG_TRUTH_TRUE) { return SHELL_EXPR_AOK_OPEN; }

    return SHELL_EXPR_LEAF;
}

static uint32_t shell_slot_find(const char *names, uint32_t max, const char *name, uint32_t len)
{
    uint32_t slot = 0;

    for (slot = 0; slot < max; slot++) { if (names[(uint64_t)slot * LDG_SHELL_NAME_CAP] != LDG_STR_TERM && shell_span_eq_is(names + (uint64_t)slot * LDG_SHELL_NAME_CAP, name, len) == LDG_TRUTH_TRUE) { return slot; } }

    return LDG_SHELL_SEL_NONE;
}

static uint32_t shell_pair_set(char *names, char *texts, uint32_t max, uint32_t text_cap, const char *name, const char *text)
{
    uint32_t name_len = shell_len_get(name, LDG_SHELL_NAME_CAP);
    uint32_t text_len = shell_len_get(text, text_cap);
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(shell_name_valid_is(name, name_len) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(text_len >= text_cap)) { return LDG_ERR_OVERFLOW; }

    slot = shell_slot_find(names, max, name, name_len);
    if (slot == LDG_SHELL_SEL_NONE) { for (slot = 0; slot < max; slot++) { if (names[(uint64_t)slot * LDG_SHELL_NAME_CAP] == LDG_STR_TERM) { break; } } }

    if (LDG_UNLIKELY(slot >= max)) { return LDG_ERR_FULL; }

    ret = ldg_mem_cpy(names + (uint64_t)slot * LDG_SHELL_NAME_CAP, name, (uint64_t)name_len + LDG_STR_TERM_SIZE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_cpy(texts + (uint64_t)slot * text_cap, text, (uint64_t)text_len + LDG_STR_TERM_SIZE);
}

static uint32_t shell_pair_unset(char *names, uint32_t max, const char *name)
{
    uint32_t slot = shell_slot_find(names, max, name, shell_len_get(name, LDG_SHELL_NAME_CAP));

    if (slot == LDG_SHELL_SEL_NONE) { return LDG_ERR_NOT_FOUND; }

    names[(uint64_t)slot * LDG_SHELL_NAME_CAP] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

static uint32_t shell_cmd_span_find(const ldg_shell_t *shell, const char *text, uint32_t len)
{
    uint32_t idx = 0;

    for (idx = 0; idx < shell->cmd_cunt; idx++) { if (shell_span_eq_is(shell->cmd[idx].name, text, len) == LDG_TRUTH_TRUE) { return idx; } }

    return LDG_SHELL_SEL_NONE;
}

static uint32_t shell_buff_take(ldg_mem_pool_t *pool, uint64_t size, void **out)
{
    uint32_t ret = 0;

    ret = ldg_mem_pool_alloc(pool, size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

static uint8_t shell_truth_pro_is(uint8_t truth)
{
    return (truth == LDG_TRUTH_TRUE || truth == LDG_TRUTH_MAYBE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t shell_truth_con_is(uint8_t truth)
{
    return (truth == LDG_TRUTH_FALSE || truth == LDG_TRUTH_MAYBE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t shell_truth_join_get(uint8_t pro, uint8_t con)
{
    if (pro && con) { return LDG_TRUTH_MAYBE; }

    if (pro) { return LDG_TRUTH_TRUE; }

    if (con) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_INDETERMINATE;
}

static uint8_t shell_truth_op_get(uint32_t kind, uint8_t lhs, uint8_t rhs)
{
    uint8_t lhs_pro = shell_truth_pro_is(lhs);
    uint8_t lhs_con = shell_truth_con_is(lhs);
    uint8_t rhs_pro = shell_truth_pro_is(rhs);
    uint8_t rhs_con = shell_truth_con_is(rhs);
    uint8_t differ = ((lhs_pro && rhs_con) || (rhs_pro && lhs_con)) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t agree = ((lhs_con || rhs_pro) && (rhs_con || lhs_pro)) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (kind == SHELL_EXPR_AND) { return shell_truth_join_get((lhs_pro && rhs_pro) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, (lhs_con || rhs_con) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (kind == SHELL_EXPR_OR) { return shell_truth_join_get((lhs_pro || rhs_pro) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, (lhs_con && rhs_con) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (kind == SHELL_EXPR_IMPL) { return shell_truth_join_get((lhs_con || rhs_pro) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, (lhs_pro && rhs_con) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE); }

    if (kind == SHELL_EXPR_IFF) { return shell_truth_join_get(agree, differ); }

    return shell_truth_join_get(differ, agree);
}

uint32_t ldg_shell_out_cb_set(ldg_shell_t *shell, ldg_shell_out_cb_t cb, void *user)
{
    if (LDG_UNLIKELY(!shell)) { return LDG_ERR_FUNC_ARG_NULL; }

    shell->out_cb = cb;
    shell->out_user = user;

    return LDG_ERR_AOK;
}

static void shell_addr_capture(ldg_shell_t *shell, const char *text, uint64_t len)
{
    char *slot = 0x0;
    uint64_t idx = 0;
    uint64_t end = 0;
    uint64_t take = 0;

    for (idx = 0; idx + SHELL_ADDR_LEAD_LEN < len && text[idx] != LDG_STR_TERM; idx++)
    {
        if (text[idx] != SHELL_ADDR_LEAD_FIRST || text[idx + SHELL_PREV_OFF] != SHELL_ADDR_LEAD_LAST) { continue; }

        if (idx > 0 && shell_name_ch_is(text[idx - SHELL_PREV_OFF]) == LDG_TRUTH_TRUE) { continue; }

        end = idx + SHELL_ADDR_LEAD_LEN;
        while (end < len && ldg_char_hex_is((uint8_t)text[end]) == LDG_TRUTH_TRUE) { end++; }

        take = end - idx;

        if (take >= SHELL_ADDR_LEAD_LEN + LDG_SHELL_ADDR_DIGITS_MIN && take < LDG_SHELL_ADDR_CAP && (end == len || shell_name_ch_is(text[end]) != LDG_TRUTH_TRUE))
        {
            slot = shell->addr[shell->addr_next];

            for (take = 0; take < end - idx; take++) { slot[take] = text[idx + take]; }

            slot[take] = LDG_STR_TERM;
            shell->addr_next = (shell->addr_next + SHELL_PREV_OFF) % LDG_SHELL_ADDR_MAX;

            if (shell->addr_cunt < LDG_SHELL_ADDR_MAX) { shell->addr_cunt++; }
        }

        idx = end;
    }
}

uint32_t ldg_shell_out_seq_get(ldg_shell_t *shell, uint64_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!shell || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = ldg_mut_lock(&shell->out_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = shell->out_seq;

    return ldg_mut_unlock(&shell->out_mut);
}

uint32_t ldg_shell_out_get(ldg_shell_t *shell, uint64_t seq, char *out, uint64_t cap, uint64_t *out_seq)
{
    uint64_t done = 0;
    uint64_t oldest = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!shell || !out || !out_seq)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_seq = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    ret = ldg_mut_lock(&shell->out_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    done = (shell->out_open == LDG_TRUTH_TRUE) ? (uint64_t)shell->out_cunt - SHELL_STACK_TOP_OFF : (uint64_t)shell->out_cunt;
    oldest = shell->out_seq - done;

    if (seq < oldest) { seq = oldest; }

    if (seq >= shell->out_seq) { ret = LDG_ERR_EMPTY; }
    else
    {
        slot = (uint32_t)(((uint64_t)shell->out_hd + (seq - oldest)) % (uint64_t)shell->conf.out_line_max);

        ret = ldg_str_rbrcpy(out, shell->out + (uint64_t)slot * shell->conf.out_line_cap, cap);
        if (ret == LDG_ERR_STR_TRUNC) { ret = LDG_ERR_AOK; }

        *out_seq = seq;
    }

    if (LDG_UNLIKELY(ldg_mut_unlock(&shell->out_mut) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    return ret;
}

uint32_t ldg_shell_out_push(ldg_shell_t *shell, const char *text)
{
    char *row = 0x0;
    uint64_t len = 0;
    uint64_t idx = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!shell || !text)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_str_len_get(text, UINT32_MAX, &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mut_lock(&shell->out_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    shell_addr_capture(shell, text, len);

    for (idx = 0; idx < len && text[idx] != LDG_STR_TERM; idx++)
    {
        if (shell->out_open != LDG_TRUTH_TRUE)
        {
            if (shell->out_cunt < shell->conf.out_line_max) { shell->out_cunt++; }
            else { shell->out_hd = (shell->out_hd + SHELL_STACK_TOP_OFF) % shell->conf.out_line_max; }

            shell->out_open = LDG_TRUTH_TRUE;
            shell->out_open_len = 0;
        }

        slot = (shell->out_hd + shell->out_cunt - SHELL_STACK_TOP_OFF) % shell->conf.out_line_max;
        row = shell->out + (uint64_t)slot * shell->conf.out_line_cap;

        if (text[idx] == LDG_ASCII_LF)
        {
            row[shell->out_open_len] = LDG_STR_TERM;
            shell->out_open = LDG_TRUTH_FALSE;
            shell->out_seq++;
            continue;
        }

        if (shell->out_open_len + LDG_STR_TERM_SIZE >= shell->conf.out_line_cap) { continue; }

        row[shell->out_open_len] = text[idx];
        shell->out_open_len++;
        row[shell->out_open_len] = LDG_STR_TERM;
    }

    if (LDG_UNLIKELY(ldg_mut_unlock(&shell->out_mut) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    if (shell->out_cb) { return shell->out_cb(shell->out_user, text, len); }

    return LDG_ERR_AOK;
}

typedef struct shell_candidate_sink
{
    char (*out)[LDG_SHELL_CANDIDATE_CAP];
    const char *word;
    uint32_t cap;
    uint32_t cunt;
    uint32_t word_len;
    uint8_t more;
    uint8_t pudding[3];
} shell_candidate_sink_t;

static uint32_t shell_candidate_put(shell_candidate_sink_t *sink, const char *text, const char *label)
{
    char *slot = 0x0;
    uint32_t len = 0;
    uint32_t idx = 0;
    uint32_t order = 0;

    for (idx = 0; idx < sink->word_len; idx++) { if (text[idx] == LDG_STR_TERM || text[idx] != sink->word[idx]) { return LDG_ERR_AOK; } }

    if (sink->cunt >= sink->cap)
    {
        sink->more = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    slot = sink->out[sink->cunt];

    while (len + LDG_STR_TERM_SIZE < LDG_SHELL_CANDIDATE_CAP && text[len] != LDG_STR_TERM)
    {
        slot[len] = text[len];
        len++;
    }

    if (text[len] != LDG_STR_TERM) { return LDG_ERR_AOK; }

    slot[len] = LDG_STR_TERM;

    for (idx = 0; idx < sink->cunt; idx++)
    {
        if (ldg_str_cmp(sink->out[idx], slot, len, &order) != LDG_ERR_AOK) { return LDG_ERR_INVALID; }

        if (order == LDG_ARITH_ORD_EQ && (sink->out[idx][len] == LDG_STR_TERM || sink->out[idx][len] == LDG_SHELL_CANDIDATE_LABEL_SEP)) { return LDG_ERR_AOK; }
    }

    if (label && label[0] != LDG_STR_TERM && len + LDG_STR_TERM_SIZE + LDG_STR_TERM_SIZE < LDG_SHELL_CANDIDATE_CAP)
    {
        slot[len] = LDG_SHELL_CANDIDATE_LABEL_SEP;
        len++;

        for (idx = 0; len + LDG_STR_TERM_SIZE < LDG_SHELL_CANDIDATE_CAP && label[idx] != LDG_STR_TERM; idx++)
        {
            slot[len] = (label[idx] >= SHELL_CANDIDATE_CH_MIN && label[idx] <= SHELL_CANDIDATE_CH_MAX) ? label[idx] : LDG_ASCII_SPACE;
            len++;
        }

        slot[len] = LDG_STR_TERM;
    }

    sink->cunt++;

    return LDG_ERR_AOK;
}

static uint32_t shell_candidate_vars_put(ldg_shell_t *shell, shell_candidate_sink_t *sink, const char *fmt)
{
    char text[LDG_SHELL_CANDIDATE_CAP] = LDG_ARR_ZERO_INIT;
    const char *name = 0x0;
    uint64_t text_len = 0;
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    for (idx = 0; idx < shell->conf.var_max && ret == LDG_ERR_AOK; idx++)
    {
        name = shell->var_name + (uint64_t)idx * LDG_SHELL_NAME_CAP;

        if (name[0] == LDG_STR_TERM) { continue; }

        ret = ldg_str_fmt(text, LDG_SHELL_CANDIDATE_CAP, &text_len, fmt, name);
        if (ret == LDG_ERR_AOK) { ret = shell_candidate_put(sink, text, shell->var_text + (uint64_t)idx * shell->conf.line_cap); }
    }

    return ret;
}

static uint32_t shell_candidate_addrs_put(ldg_shell_t *shell, shell_candidate_sink_t *sink)
{
    const char *text = 0x0;
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    ret = ldg_mut_lock(&shell->out_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < shell->addr_cunt && ret == LDG_ERR_AOK; idx++) { ret = shell_candidate_put(sink, shell->addr[idx], SHELL_ADDR_LABEL); }

    if (LDG_UNLIKELY(ldg_mut_unlock(&shell->out_mut) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    for (idx = 0; idx < shell->conf.var_max && ret == LDG_ERR_AOK; idx++)
    {
        text = shell->var_text + (uint64_t)idx * shell->conf.line_cap;

        if (shell->var_name[(uint64_t)idx * LDG_SHELL_NAME_CAP] == LDG_STR_TERM || text[0] != SHELL_ADDR_LEAD_FIRST || text[SHELL_PREV_OFF] != SHELL_ADDR_LEAD_LAST) { continue; }

        ret = shell_candidate_put(sink, text, shell->var_name + (uint64_t)idx * LDG_SHELL_NAME_CAP);
    }

    return ret;
}

static uint32_t shell_candidates_sort(shell_candidate_sink_t *sink)
{
    char held[LDG_SHELL_CANDIDATE_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t prior = 0;
    uint32_t order = 0;
    uint32_t ret = LDG_ERR_AOK;

    for (idx = SHELL_PREV_OFF; idx < sink->cunt && ret == LDG_ERR_AOK; idx++)
    {
        ret = ldg_mem_cpy(held, sink->out[idx], LDG_SHELL_CANDIDATE_CAP);
        prior = idx;

        while (ret == LDG_ERR_AOK && prior > 0)
        {
            ret = ldg_str_cmp(sink->out[prior - SHELL_PREV_OFF], held, LDG_SHELL_CANDIDATE_CAP, &order);
            if (ret != LDG_ERR_AOK || order != LDG_ARITH_ORD_GT) { break; }

            ret = ldg_mem_cpy(sink->out[prior], sink->out[prior - SHELL_PREV_OFF], LDG_SHELL_CANDIDATE_CAP);
            prior--;
        }

        if (ret == LDG_ERR_AOK) { ret = ldg_mem_cpy(sink->out[prior], held, LDG_SHELL_CANDIDATE_CAP); }
    }

    return ret;
}

uint32_t ldg_shell_candidate_get(ldg_shell_t *shell, const char *text, uint32_t len, char (*out)[LDG_SHELL_CANDIDATE_CAP], uint32_t cap, uint32_t *out_cunt, uint32_t *out_at, uint8_t *out_more)
{
    shell_candidate_sink_t sink = LDG_STRUCT_ZERO_INIT;
    uint32_t at = len;
    uint32_t verb_len = 0;
    uint32_t arg_idx = 0;
    uint32_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!shell || !text || !out || !out_cunt || !out_at || !out_more)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    *out_at = 0;
    *out_more = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (at > 0 && shell_space_is(text[at - SHELL_PREV_OFF]) != LDG_TRUTH_TRUE && text[at - SHELL_PREV_OFF] != SHELL_CH_LIST_SEP) { at--; }

    while (verb_len < len && shell_space_is(text[verb_len]) != LDG_TRUTH_TRUE) { verb_len++; }

    for (idx = 0; idx < at; idx++) { if (shell_space_is(text[idx]) != LDG_TRUTH_TRUE && (idx == 0 || shell_space_is(text[idx - SHELL_PREV_OFF]) == LDG_TRUTH_TRUE)) { arg_idx++; } }

    sink.out = out;
    sink.cap = cap;
    sink.word = text + at;
    sink.word_len = len - at;

    if (sink.word_len > 0 && sink.word[0] == SHELL_CH_VAR) { ret = shell_candidate_vars_put(shell, &sink, (sink.word_len > SHELL_VAR_BRACE_AT && sink.word[SHELL_VAR_BRACE_AT] == SHELL_CH_BRACE_OPEN) ? SHELL_CANDIDATE_VAR_BRACED_FMT : SHELL_CANDIDATE_VAR_FMT); }
    else if (sink.word_len >= SHELL_ADDR_LEAD_LEN && sink.word[0] == SHELL_ADDR_LEAD_FIRST && sink.word[SHELL_PREV_OFF] == SHELL_ADDR_LEAD_LAST) { ret = shell_candidate_addrs_put(shell, &sink); }
    else if (arg_idx == 0)
    {
        for (idx = 0; idx < shell->cmd_cunt && ret == LDG_ERR_AOK; idx++) { ret = shell_candidate_put(&sink, shell->cmd[idx].name, shell->cmd[idx].help); }

        for (idx = 0; idx < shell->conf.alias_max && ret == LDG_ERR_AOK; idx++) { if (shell->alias_name[(uint64_t)idx * LDG_SHELL_NAME_CAP] != LDG_STR_TERM) { ret = shell_candidate_put(&sink, shell->alias_name + (uint64_t)idx * LDG_SHELL_NAME_CAP, shell->alias_text + (uint64_t)idx * shell->conf.line_cap); } }
    }
    else if (arg_idx == SHELL_CMD_NAME_CUNT && shell_span_eq_is(SHELL_CMD_SET, text, verb_len) == LDG_TRUTH_TRUE) { ret = shell_candidate_vars_put(shell, &sink, SHELL_CANDIDATE_SET_FMT); }
    else if (arg_idx == SHELL_CMD_NAME_CUNT && shell_span_eq_is(SHELL_CMD_UNSET, text, verb_len) == LDG_TRUTH_TRUE) { ret = shell_candidate_vars_put(shell, &sink, SHELL_CANDIDATE_NAME_FMT); }

    if (ret == LDG_ERR_AOK) { ret = shell_candidates_sort(&sink); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_cunt = sink.cunt;
    *out_at = at;
    *out_more = sink.more;

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_var_set(ldg_shell_t *shell, const char *name, const char *text)
{
    if (LDG_UNLIKELY(!shell || !name || !text)) { return LDG_ERR_FUNC_ARG_NULL; }

    return shell_pair_set(shell->var_name, shell->var_text, shell->conf.var_max, shell->conf.line_cap, name, text);
}

uint32_t ldg_shell_var_get(const ldg_shell_t *shell, const char *name, const char **out)
{
    uint32_t slot = 0;

    if (LDG_UNLIKELY(!shell || !name || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    slot = shell_slot_find(shell->var_name, shell->conf.var_max, name, shell_len_get(name, LDG_SHELL_NAME_CAP));
    if (slot == LDG_SHELL_SEL_NONE) { return LDG_ERR_NOT_FOUND; }

    *out = shell->var_text + (uint64_t)slot * shell->conf.line_cap;

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_var_unset(ldg_shell_t *shell, const char *name)
{
    if (LDG_UNLIKELY(!shell || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    return shell_pair_unset(shell->var_name, shell->conf.var_max, name);
}

uint32_t ldg_shell_alias_set(ldg_shell_t *shell, const char *name, const char *text)
{
    if (LDG_UNLIKELY(!shell || !name || !text)) { return LDG_ERR_FUNC_ARG_NULL; }

    return shell_pair_set(shell->alias_name, shell->alias_text, shell->conf.alias_max, shell->conf.line_cap, name, text);
}

uint32_t ldg_shell_alias_get(const ldg_shell_t *shell, const char *name, const char **out)
{
    uint32_t slot = 0;

    if (LDG_UNLIKELY(!shell || !name || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    slot = shell_slot_find(shell->alias_name, shell->conf.alias_max, name, shell_len_get(name, LDG_SHELL_NAME_CAP));
    if (slot == LDG_SHELL_SEL_NONE) { return LDG_ERR_NOT_FOUND; }

    *out = shell->alias_text + (uint64_t)slot * shell->conf.line_cap;

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_alias_unset(ldg_shell_t *shell, const char *name)
{
    if (LDG_UNLIKELY(!shell || !name)) { return LDG_ERR_FUNC_ARG_NULL; }

    return shell_pair_unset(shell->alias_name, shell->conf.alias_max, name);
}

static uint32_t shell_row_put(ldg_shell_t *shell, const char *lhs, const char *sep, const char *rhs)
{
    uint32_t ret = 0;

    ret = ldg_shell_out_push(shell, lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_shell_out_push(shell, sep);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_shell_out_push(shell, rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_shell_out_push(shell, SHELL_MSG_LF);
}

static uint32_t shell_pairs_put(ldg_shell_t *shell, const char *names, const char *texts, uint32_t max)
{
    uint32_t slot = 0;
    uint32_t ret = 0;

    for (slot = 0; slot < max; slot++)
    {
        if (names[(uint64_t)slot * LDG_SHELL_NAME_CAP] == LDG_STR_TERM) { continue; }

        ret = shell_row_put(shell, names + (uint64_t)slot * LDG_SHELL_NAME_CAP, SHELL_SEP_PAIR, texts + (uint64_t)slot * shell->conf.line_cap);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t shell_help_cb(void *user, const char *const *arg, uint32_t arg_cunt, uint8_t *out_truth)
{
    ldg_shell_t *shell = (ldg_shell_t *)user;
    const ldg_shell_cmd_t *cmd = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!shell || !arg || !out_truth)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (arg_cunt > SHELL_CMD_NAME_CUNT)
    {
        ret = ldg_shell_cmd_find(shell, arg[1], &cmd);
        if (ret != LDG_ERR_AOK) { return ret; }

        return shell_row_put(shell, cmd->name, SHELL_SEP_HELP, cmd->help);
    }

    for (idx = 0; idx < shell->cmd_cunt; idx++)
    {
        ret = shell_row_put(shell, shell->cmd[idx].name, SHELL_SEP_HELP, shell->cmd[idx].help);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t shell_set_cb(void *user, const char *const *arg, uint32_t arg_cunt, uint8_t *out_truth)
{
    char name[LDG_SHELL_NAME_CAP] = LDG_ARR_ZERO_INIT;
    const char *text = 0x0;
    uint32_t name_len = 0;
    uint32_t pos = 0;

    if (LDG_UNLIKELY(!user || !arg || !out_truth)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(arg_cunt <= SHELL_CMD_NAME_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    text = arg[1];

    while (name_len + LDG_STR_TERM_SIZE < LDG_SHELL_NAME_CAP && shell_name_ch_is(text[name_len]) == LDG_TRUTH_TRUE)
    {
        name[name_len] = text[name_len];
        name_len++;
    }

    pos = name_len;
    while (shell_space_is(text[pos]) == LDG_TRUTH_TRUE) { pos++; }

    if (text[pos] == SHELL_CH_ASSIGN)
    {
        pos++;
        while (shell_space_is(text[pos]) == LDG_TRUTH_TRUE) { pos++; }
    }
    else if (LDG_UNLIKELY(pos == name_len)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ldg_shell_var_set((ldg_shell_t *)user, name, text + pos);
}

static uint32_t shell_unset_cb(void *user, const char *const *arg, uint32_t arg_cunt, uint8_t *out_truth)
{
    if (LDG_UNLIKELY(!user || !arg || !out_truth)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(arg_cunt <= SHELL_CMD_NAME_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ldg_shell_var_unset((ldg_shell_t *)user, arg[1]);
}

static uint32_t shell_vars_cb(void *user, const char *const *arg, uint32_t arg_cunt, uint8_t *out_truth)
{
    ldg_shell_t *shell = (ldg_shell_t *)user;

    if (LDG_UNLIKELY(!shell || !arg || !out_truth)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(arg_cunt != SHELL_CMD_NAME_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return shell_pairs_put(shell, shell->var_name, shell->var_text, shell->conf.var_max);
}

static uint32_t shell_aliases_cb(void *user, const char *const *arg, uint32_t arg_cunt, uint8_t *out_truth)
{
    ldg_shell_t *shell = (ldg_shell_t *)user;

    if (LDG_UNLIKELY(!shell || !arg || !out_truth)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(arg_cunt != SHELL_CMD_NAME_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return shell_pairs_put(shell, shell->alias_name, shell->alias_text, shell->conf.alias_max);
}

static uint32_t shell_builtin_reg(ldg_shell_t *shell, const char *name, const char *help, ldg_shell_cmd_cb_t cb, uint32_t arg_min, uint32_t arg_max, uint8_t remainder)
{
    ldg_shell_cmd_t cmd = LDG_STRUCT_ZERO_INIT;

    cmd.name = name;
    cmd.help = help;
    cmd.cb = cb;
    cmd.user = shell;
    cmd.arg_min = arg_min;
    cmd.arg_max = arg_max;
    cmd.kind = LDG_SHELL_CMD_PLAIN;
    cmd.remainder = remainder;

    return ldg_shell_cmd_reg(shell, &cmd);
}

static uint32_t shell_builtins_reg(ldg_shell_t *shell)
{
    uint32_t ret = 0;

    ret = shell_builtin_reg(shell, SHELL_CMD_HELP, SHELL_HELP_HELP, shell_help_cb, SHELL_ARG_CUNT_NONE, SHELL_NAME_ARG_CUNT, LDG_TRUTH_FALSE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = shell_builtin_reg(shell, SHELL_CMD_SET, SHELL_HELP_SET, shell_set_cb, SHELL_NAME_ARG_CUNT, SHELL_NAME_ARG_CUNT, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = shell_builtin_reg(shell, SHELL_CMD_UNSET, SHELL_HELP_UNSET, shell_unset_cb, SHELL_NAME_ARG_CUNT, SHELL_NAME_ARG_CUNT, LDG_TRUTH_FALSE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = shell_builtin_reg(shell, SHELL_CMD_VARS, SHELL_HELP_VARS, shell_vars_cb, SHELL_ARG_CUNT_NONE, SHELL_ARG_CUNT_NONE, LDG_TRUTH_FALSE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return shell_builtin_reg(shell, SHELL_CMD_ALIASES, SHELL_HELP_ALIASES, shell_aliases_cb, SHELL_ARG_CUNT_NONE, SHELL_ARG_CUNT_NONE, LDG_TRUTH_FALSE);
}

uint32_t ldg_shell_conf_default_get(ldg_shell_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_zero(out, sizeof(ldg_shell_conf_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    out->line_cap = LDG_SHELL_LINE_CAP_DEFAULT;
    out->hist_max = LDG_SHELL_HIST_MAX_DEFAULT;
    out->cmd_max = LDG_SHELL_CMD_MAX_DEFAULT;
    out->var_max = LDG_SHELL_VAR_MAX_DEFAULT;
    out->alias_max = LDG_SHELL_ALIAS_MAX_DEFAULT;
    out->out_line_max = LDG_SHELL_OUT_LINE_MAX_DEFAULT;
    out->out_line_cap = LDG_SHELL_OUT_LINE_CAP_DEFAULT;

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_pool_size_get(const ldg_shell_conf_t *conf, uint64_t *out)
{
    uint64_t total = 0;

    if (LDG_UNLIKELY(!conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(conf->line_cap < LDG_SHELL_LINE_CAP_MIN || conf->hist_max == 0 || conf->cmd_max == 0 || conf->var_max == 0 || conf->alias_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->out_line_max == 0 || conf->out_line_cap < LDG_SHELL_LINE_CAP_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    total = (uint64_t)sizeof(ldg_shell_t);
    total += (uint64_t)SHELL_POOL_ALLOC_CUNT * (uint64_t)LDG_MEM_POOL_VAR_ALIGN;
    total += (uint64_t)conf->line_cap * (uint64_t)SHELL_EXEC_BUFF_CUNT;
    total += (uint64_t)conf->hist_max * (uint64_t)conf->line_cap * (uint64_t)SHELL_HIST_BUFF_CUNT;
    total += (uint64_t)conf->cmd_max * (uint64_t)sizeof(ldg_shell_cmd_t);
    total += (uint64_t)(conf->var_max + conf->alias_max) * (uint64_t)(LDG_SHELL_NAME_CAP + conf->line_cap);
    total += (uint64_t)conf->out_line_max * (uint64_t)conf->out_line_cap;
    total += (uint64_t)conf->hist_max * (uint64_t)LDG_SHELL_HIST_ROW_EXTRA;
    total += (uint64_t)conf->alias_max * (uint64_t)(LDG_SHELL_NAME_CAP + conf->line_cap + LDG_SHELL_HIST_ROW_EXTRA);

    *out = total;

    return LDG_ERR_AOK;
}

static uint32_t shell_hist_buffs_take(ldg_mem_pool_t *pool, ldg_shell_t *shell)
{
    uint64_t cap = (uint64_t)shell->conf.line_cap;
    uint64_t blob_cap = (cap + (uint64_t)LDG_SHELL_HIST_ROW_EXTRA) * (uint64_t)shell->conf.hist_max + (uint64_t)shell->conf.alias_max * ((uint64_t)LDG_SHELL_NAME_CAP + cap + (uint64_t)LDG_SHELL_HIST_ROW_EXTRA);
    uint32_t ret = 0;

    ret = shell_buff_take(pool, cap * (uint64_t)shell->conf.hist_max, (void **)&shell->hist);
    if (ret == LDG_ERR_AOK) { ret = shell_buff_take(pool, blob_cap, (void **)&shell->hist_blob); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    shell->hist_max = shell->conf.hist_max;
    shell->hist_blob_cap = blob_cap;

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_create(ldg_mem_pool_t *pool, const ldg_shell_conf_t *conf, ldg_shell_t **out)
{
    ldg_shell_t *shell = 0x0;
    uint64_t need = 0;
    uint64_t cap = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    ret = ldg_shell_pool_size_get(conf, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = shell_buff_take(pool, sizeof(ldg_shell_t), (void **)&shell);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_cpy(&shell->conf, conf, sizeof(ldg_shell_conf_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cap = (uint64_t)conf->line_cap;

    ret = shell_hist_buffs_take(pool, shell);
    if (ret == LDG_ERR_AOK) { ret = shell_buff_take(pool, (uint64_t)conf->cmd_max * (uint64_t)sizeof(ldg_shell_cmd_t), (void **)&shell->cmd); }

    if (ret == LDG_ERR_AOK) { ret = shell_buff_take(pool, (uint64_t)conf->var_max * (uint64_t)LDG_SHELL_NAME_CAP, (void **)&shell->var_name); }

    if (ret == LDG_ERR_AOK) { ret = shell_buff_take(pool, (uint64_t)conf->var_max * cap, (void **)&shell->var_text); }

    if (ret == LDG_ERR_AOK) { ret = shell_buff_take(pool, (uint64_t)conf->alias_max * (uint64_t)LDG_SHELL_NAME_CAP, (void **)&shell->alias_name); }

    if (ret == LDG_ERR_AOK) { ret = shell_buff_take(pool, (uint64_t)conf->alias_max * cap, (void **)&shell->alias_text); }

    if (ret == LDG_ERR_AOK) { ret = shell_buff_take(pool, (uint64_t)conf->out_line_max * (uint64_t)conf->out_line_cap, (void **)&shell->out); }

    if (ret == LDG_ERR_AOK) { ret = shell_buff_take(pool, cap, (void **)&shell->exec_line); }

    if (ret == LDG_ERR_AOK) { ret = shell_buff_take(pool, cap * (uint64_t)SHELL_ARG_BUFF_FACTOR, (void **)&shell->arg_buff); }

    if (ret == LDG_ERR_AOK) { ret = shell_builtins_reg(shell); }

    if (ret == LDG_ERR_AOK) { ret = ldg_mut_init(&shell->out_mut, LDG_TRUTH_FALSE); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    shell->truth = LDG_TRUTH_INDETERMINATE;
    *out = shell;

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_uncreate(ldg_shell_t **shell)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!shell || !*shell)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mut_destroy(&(*shell)->out_mut);
    *shell = 0x0;

    return ret;
}

uint32_t ldg_shell_rst(ldg_shell_t *shell)
{
    if (LDG_UNLIKELY(!shell)) { return LDG_ERR_FUNC_ARG_NULL; }

    shell->status = LDG_ERR_AOK;
    shell->truth = LDG_TRUTH_INDETERMINATE;
    shell->truth_has = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_status_get(const ldg_shell_t *shell, uint32_t *out_status, uint8_t *out_truth, uint8_t *out_truth_has)
{
    if (LDG_UNLIKELY(!shell || !out_status || !out_truth || !out_truth_has)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_status = shell->status;
    *out_truth = shell->truth;
    *out_truth_has = shell->truth_has;

    return LDG_ERR_AOK;
}

static uint32_t shell_var_ref_take(ldg_shell_t *shell, const char *src, uint32_t len, uint32_t *pos, uint32_t *buff_len, uint32_t cap)
{
    const char *val = 0x0;
    uint32_t at = *pos;
    uint32_t name_start = 0;
    uint32_t slot = 0;
    uint32_t val_len = 0;
    uint32_t ret = 0;
    uint8_t braced = LDG_TRUTH_FALSE;

    at++;

    if (at < len && src[at] == SHELL_CH_VAR)
    {
        if (LDG_UNLIKELY(*buff_len + LDG_STR_TERM_SIZE >= cap)) { return LDG_ERR_OVERFLOW; }

        shell->arg_buff[*buff_len] = SHELL_CH_VAR;
        (*buff_len)++;
        at++;
        *pos = at;

        return LDG_ERR_AOK;
    }

    if (at < len && src[at] == SHELL_CH_BRACE_OPEN)
    {
        braced = LDG_TRUTH_TRUE;
        at++;
    }

    name_start = at;
    while (at < len && shell_name_ch_is(src[at]) == LDG_TRUTH_TRUE) { at++; }

    if (LDG_UNLIKELY(shell_name_valid_is(src + name_start, at - name_start) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (braced && LDG_UNLIKELY(at >= len || src[at] != SHELL_CH_BRACE_CLOSE)) { return LDG_ERR_STR_INVALID; }

    slot = shell_slot_find(shell->var_name, shell->conf.var_max, src + name_start, at - name_start);
    if (slot == LDG_SHELL_SEL_NONE) { return LDG_ERR_NOT_FOUND; }

    val = shell->var_text + (uint64_t)slot * shell->conf.line_cap;
    val_len = shell_len_get(val, shell->conf.line_cap);

    if (LDG_UNLIKELY(*buff_len + val_len + LDG_STR_TERM_SIZE >= cap)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_cpy(shell->arg_buff + *buff_len, val, val_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *buff_len += val_len;

    if (braced) { at++; }

    *pos = at;

    return LDG_ERR_AOK;
}

static uint32_t shell_words_take(ldg_shell_t *shell, const char *src, uint32_t len, uint32_t *pos, uint32_t *buff_len, uint32_t word_max)
{
    uint32_t cap = shell->conf.line_cap * SHELL_ARG_BUFF_FACTOR;
    uint32_t taken = 0;
    uint32_t ret = 0;
    uint8_t quote = SHELL_QUOTE_NONE;
    uint8_t quote_next = SHELL_QUOTE_NONE;

    while (taken < word_max)
    {
        while (*pos < len && shell_space_is(src[*pos]) == LDG_TRUTH_TRUE) { (*pos)++; }

        if (*pos >= len) { break; }

        if (LDG_UNLIKELY(shell->arg_cunt >= LDG_SHELL_ARG_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

        shell->arg[shell->arg_cunt] = shell->arg_buff + *buff_len;

        while (*pos < len)
        {
            if (quote == SHELL_QUOTE_NONE && shell_space_is(src[*pos]) == LDG_TRUTH_TRUE) { break; }

            quote_next = shell_quote_next_get(quote, src[*pos]);
            if (quote_next != quote)
            {
                quote = quote_next;
                (*pos)++;
                continue;
            }

            if (src[*pos] == SHELL_CH_VAR && quote == SHELL_QUOTE_NONE)
            {
                ret = shell_var_ref_take(shell, src, len, pos, buff_len, cap);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                continue;
            }

            if (LDG_UNLIKELY(*buff_len + LDG_STR_TERM_SIZE >= cap)) { return LDG_ERR_OVERFLOW; }

            shell->arg_buff[*buff_len] = src[*pos];
            (*buff_len)++;
            (*pos)++;
        }

        if (LDG_UNLIKELY(quote != SHELL_QUOTE_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(*buff_len >= cap)) { return LDG_ERR_OVERFLOW; }

        shell->arg_buff[*buff_len] = LDG_STR_TERM;
        (*buff_len)++;
        shell->arg_cunt++;
        taken++;
    }

    return LDG_ERR_AOK;
}

static uint32_t shell_leaf_split(ldg_shell_t *shell, uint32_t item)
{
    const char *src = shell->exec_line + shell->expr_start[item];
    const char *alias = 0x0;
    uint32_t len = shell->expr_len[item];
    uint32_t cap = shell->conf.line_cap * SHELL_ARG_BUFF_FACTOR;
    uint32_t pos = 0;
    uint32_t alias_pos = 0;
    uint32_t buff_len = 0;
    uint32_t end = 0;
    uint32_t slot = 0;
    uint32_t cmd_idx = 0;
    uint32_t ret = 0;

    shell->arg_cunt = 0;

    while (pos < len && shell_space_is(src[pos]) == LDG_TRUTH_TRUE) { pos++; }

    end = shell_word_end_get(src, len, pos);
    slot = shell_slot_find(shell->alias_name, shell->conf.alias_max, src + pos, end - pos);

    if (slot != LDG_SHELL_SEL_NONE)
    {
        alias = shell->alias_text + (uint64_t)slot * shell->conf.line_cap;
        ret = shell_words_take(shell, alias, shell_len_get(alias, shell->conf.line_cap), &alias_pos, &buff_len, UINT32_MAX);
        pos = end;
    }
    else { ret = shell_words_take(shell, src, len, &pos, &buff_len, SHELL_WORD_FIRST_CUNT); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(shell->arg_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cmd_idx = shell_cmd_span_find(shell, shell->arg[0], shell_len_get(shell->arg[0], cap));
    if (cmd_idx == LDG_SHELL_SEL_NONE || shell->cmd[cmd_idx].remainder != LDG_TRUTH_TRUE) { return shell_words_take(shell, src, len, &pos, &buff_len, UINT32_MAX); }

    while (pos < len && shell_space_is(src[pos]) == LDG_TRUTH_TRUE) { pos++; }

    if (pos >= len) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(shell->arg_cunt >= LDG_SHELL_ARG_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(buff_len + (len - pos) + LDG_STR_TERM_SIZE >= cap)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_cpy(shell->arg_buff + buff_len, src + pos, len - pos);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    shell->arg[shell->arg_cunt] = shell->arg_buff + buff_len;
    shell->arg_buff[buff_len + (len - pos)] = LDG_STR_TERM;
    shell->arg_cunt++;

    return LDG_ERR_AOK;
}

static uint32_t shell_leaf_cmd_get(ldg_shell_t *shell, uint32_t item, uint32_t *out_cmd)
{
    uint32_t cmd_idx = 0;
    uint32_t given = 0;
    uint32_t ret = 0;

    ret = shell_leaf_split(shell, item);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cmd_idx = shell_cmd_span_find(shell, shell->arg[0], shell_len_get(shell->arg[0], shell->conf.line_cap * SHELL_ARG_BUFF_FACTOR));
    if (cmd_idx == LDG_SHELL_SEL_NONE)
    {
        ret = shell_row_put(shell, SHELL_MSG_UNKNOWN, shell->arg[0], SHELL_MSG_NONE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return LDG_ERR_NOT_FOUND;
    }

    given = shell->arg_cunt - SHELL_CMD_NAME_CUNT;
    if (given < shell->cmd[cmd_idx].arg_min || given > shell->cmd[cmd_idx].arg_max) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out_cmd = cmd_idx;

    return LDG_ERR_AOK;
}

static uint32_t shell_leaf_run(ldg_shell_t *shell, uint32_t item, uint8_t *out_truth, uint8_t *out_kind)
{
    uint32_t cmd_idx = 0;
    uint32_t ret = 0;
    uint8_t truth = LDG_TRUTH_INDETERMINATE;

    ret = shell_leaf_cmd_get(shell, item, &cmd_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_kind = shell->cmd[cmd_idx].kind;
    ret = shell->cmd[cmd_idx].cb(shell->cmd[cmd_idx].user, shell->arg, shell->arg_cunt, &truth);
    *out_truth = truth;

    return ret;
}

static uint32_t shell_expr_item_put(ldg_shell_t *shell, uint32_t kind, uint32_t start, uint32_t len)
{
    if (LDG_UNLIKELY(shell->expr_cunt >= LDG_SHELL_EXPR_ITEM_MAX)) { return LDG_ERR_OVERFLOW; }

    shell->expr_kind[shell->expr_cunt] = kind;
    shell->expr_start[shell->expr_cunt] = start;
    shell->expr_len[shell->expr_cunt] = len;
    shell->expr_cunt++;

    return LDG_ERR_AOK;
}

static uint32_t shell_expr_leaf_end_get(const ldg_shell_t *shell, const char *text, uint32_t len, uint32_t pos, uint32_t end)
{
    uint32_t cmd_idx = shell_cmd_span_find(shell, text + pos, end - pos);
    uint32_t probe = 0;
    uint32_t probe_end = 0;

    if (cmd_idx != LDG_SHELL_SEL_NONE && shell->cmd[cmd_idx].remainder == LDG_TRUTH_TRUE) { return len; }

    for (;;)
    {
        probe = end;
        while (probe < len && shell_space_is(text[probe]) == LDG_TRUTH_TRUE) { probe++; }

        if (probe >= len || text[probe] == SHELL_CH_PAREN_OPEN || text[probe] == SHELL_CH_PAREN_CLOSE) { break; }

        probe_end = shell_word_end_get(text, len, probe);
        if (shell_word_kind_get(text + probe, probe_end - probe) != SHELL_EXPR_LEAF) { break; }

        end = probe_end;
    }

    return end;
}

static uint32_t shell_expr_close(ldg_shell_t *shell, uint32_t *depth, uint32_t aok_base)
{
    uint32_t popped = 0;
    uint32_t ret = 0;

    while (*depth > 0 && shell->op_stack[*depth - SHELL_STACK_TOP_OFF] < SHELL_EXPR_PAREN)
    {
        (*depth)--;
        ret = shell_expr_item_put(shell, shell->op_stack[*depth], SHELL_SPAN_NONE, SHELL_SPAN_NONE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        popped++;
    }

    if (LDG_UNLIKELY(*depth == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    (*depth)--;

    if (shell->op_stack[*depth] != SHELL_EXPR_AOK_OPEN) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(popped != 0 || shell->expr_cunt - aok_base != SHELL_AOK_LEAF_CUNT || shell->expr_kind[aok_base] != SHELL_EXPR_LEAF)) { return LDG_ERR_FUNC_ARG_INVALID; }

    shell->expr_kind[aok_base] = SHELL_EXPR_LEAF_AOK;

    return LDG_ERR_AOK;
}

static uint32_t shell_expr_op_push(ldg_shell_t *shell, uint32_t *depth, uint32_t kind)
{
    uint32_t top = 0;
    uint32_t ret = 0;

    while (kind > SHELL_EXPR_NOT && kind < SHELL_EXPR_PAREN && *depth > 0)
    {
        top = shell->op_stack[*depth - SHELL_STACK_TOP_OFF];
        if (top >= SHELL_EXPR_PAREN || top > kind || (top == kind && kind == SHELL_EXPR_IMPL)) { break; }

        (*depth)--;
        ret = shell_expr_item_put(shell, top, SHELL_SPAN_NONE, SHELL_SPAN_NONE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (LDG_UNLIKELY(*depth >= LDG_SHELL_EXPR_DEPTH_MAX)) { return LDG_ERR_OVERFLOW; }

    shell->op_stack[*depth] = (uint8_t)kind;
    (*depth)++;

    return LDG_ERR_AOK;
}

static uint32_t shell_expr_parse(ldg_shell_t *shell, uint32_t start, uint32_t len)
{
    const char *text = shell->exec_line + start;
    uint32_t pos = 0;
    uint32_t end = 0;
    uint32_t kind = 0;
    uint32_t depth = 0;
    uint32_t aok_base = 0;
    uint32_t ret = LDG_ERR_AOK;
    uint8_t want = LDG_TRUTH_TRUE;

    shell->expr_cunt = 0;

    while (pos < len && ret == LDG_ERR_AOK)
    {
        if (shell_space_is(text[pos]) == LDG_TRUTH_TRUE)
        {
            pos++;
            continue;
        }

        if (text[pos] == SHELL_CH_PAREN_CLOSE)
        {
            if (LDG_UNLIKELY(want)) { return LDG_ERR_FUNC_ARG_INVALID; }

            ret = shell_expr_close(shell, &depth, aok_base);
            pos++;
            continue;
        }

        if (text[pos] == SHELL_CH_PAREN_OPEN)
        {
            if (LDG_UNLIKELY(!want)) { return LDG_ERR_FUNC_ARG_INVALID; }

            ret = shell_expr_op_push(shell, &depth, SHELL_EXPR_PAREN);
            pos++;
            continue;
        }

        end = shell_word_end_get(text, len, pos);
        if (LDG_UNLIKELY(end == pos)) { return LDG_ERR_FUNC_ARG_INVALID; }

        kind = shell_word_kind_get(text + pos, end - pos);

        if (kind == SHELL_EXPR_LEAF)
        {
            if (LDG_UNLIKELY(!want)) { return LDG_ERR_FUNC_ARG_INVALID; }

            end = shell_expr_leaf_end_get(shell, text, len, pos, end);
            ret = shell_expr_item_put(shell, SHELL_EXPR_LEAF, start + pos, end - pos);
            want = LDG_TRUTH_FALSE;
            pos = end;
            continue;
        }

        pos = end;

        if (kind == SHELL_EXPR_AOK_OPEN)
        {
            while (pos < len && shell_space_is(text[pos]) == LDG_TRUTH_TRUE) { pos++; }

            if (LDG_UNLIKELY(!want || pos >= len || text[pos] != SHELL_CH_PAREN_OPEN)) { return LDG_ERR_FUNC_ARG_INVALID; }

            aok_base = shell->expr_cunt;
            pos++;
        }
        else if (LDG_UNLIKELY((kind == SHELL_EXPR_NOT) != (want == LDG_TRUTH_TRUE))) { return LDG_ERR_FUNC_ARG_INVALID; }

        ret = shell_expr_op_push(shell, &depth, kind);
        want = LDG_TRUTH_TRUE;
    }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(want)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (depth > 0)
    {
        depth--;
        if (LDG_UNLIKELY(shell->op_stack[depth] >= SHELL_EXPR_PAREN)) { return LDG_ERR_FUNC_ARG_INVALID; }

        ret = shell_expr_item_put(shell, shell->op_stack[depth], SHELL_SPAN_NONE, SHELL_SPAN_NONE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t shell_expr_ck(ldg_shell_t *shell)
{
    uint32_t idx = 0;
    uint32_t cmd_idx = 0;
    uint32_t ret = 0;

    for (idx = 0; idx < shell->expr_cunt; idx++)
    {
        if (shell->expr_kind[idx] > SHELL_EXPR_LEAF_AOK) { continue; }

        ret = shell_leaf_cmd_get(shell, idx, &cmd_idx);
        if (ret == LDG_ERR_NOT_FOUND) { return ret; }

        if (ret == LDG_ERR_AOK && shell->expr_cunt != SHELL_PLAIN_ITEM_CUNT && shell->expr_kind[idx] == SHELL_EXPR_LEAF && shell->cmd[cmd_idx].kind != LDG_SHELL_CMD_PREDICATE) { ret = LDG_ERR_FUNC_ARG_INVALID; }

        if (ret == LDG_ERR_AOK) { continue; }

        if (LDG_UNLIKELY(ldg_shell_out_push(shell, SHELL_MSG_REFUSED) != LDG_ERR_AOK)) { return LDG_ERR_IO_WR; }

        return ret;
    }

    return LDG_ERR_AOK;
}

static uint32_t shell_expr_eval(ldg_shell_t *shell)
{
    uint32_t idx = 0;
    uint32_t depth = 0;
    uint32_t ret = 0;
    uint8_t truth = LDG_TRUTH_INDETERMINATE;
    uint8_t kind = LDG_SHELL_CMD_PLAIN;

    for (idx = 0; idx < shell->expr_cunt; idx++)
    {
        if (shell->expr_kind[idx] == SHELL_EXPR_NOT)
        {
            if (LDG_UNLIKELY(depth < SHELL_STACK_TOP_OFF)) { return LDG_ERR_INVALID; }

            truth = shell->truth_stack[depth - SHELL_STACK_TOP_OFF];
            shell->truth_stack[depth - SHELL_STACK_TOP_OFF] = shell_truth_join_get(shell_truth_con_is(truth), shell_truth_pro_is(truth));
            continue;
        }

        if (shell->expr_kind[idx] > SHELL_EXPR_NOT)
        {
            if (LDG_UNLIKELY(depth <= SHELL_STACK_TOP_OFF)) { return LDG_ERR_INVALID; }

            depth--;
            shell->truth_stack[depth - SHELL_STACK_TOP_OFF] = shell_truth_op_get(shell->expr_kind[idx], shell->truth_stack[depth - SHELL_STACK_TOP_OFF], shell->truth_stack[depth]);
            continue;
        }

        ret = shell_leaf_run(shell, idx, &truth, &kind);

        if (shell->expr_kind[idx] == SHELL_EXPR_LEAF_AOK) { truth = (ret == LDG_ERR_AOK) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }
        else if (ret != LDG_ERR_AOK) { return ret; }

        if (LDG_UNLIKELY(truth > LDG_TRUTH_INDETERMINATE)) { return LDG_ERR_INVALID; }

        if (LDG_UNLIKELY(depth >= LDG_SHELL_EXPR_DEPTH_MAX)) { return LDG_ERR_OVERFLOW; }

        shell->truth_stack[depth] = truth;
        depth++;
    }

    shell->truth = shell->truth_stack[0];
    shell->truth_has = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t shell_alias_stmt_take(ldg_shell_t *shell, uint32_t start, uint32_t len, uint8_t *out_taken)
{
    char *text = shell->exec_line + start;
    uint32_t name_start = 0;
    uint32_t name_end = 0;
    uint32_t val_start = 0;
    uint32_t ret = 0;
    char held = text[len];

    *out_taken = LDG_TRUTH_FALSE;

    while (name_start < len && shell_space_is(text[name_start]) == LDG_TRUTH_TRUE) { name_start++; }

    name_end = name_start;
    while (name_end < len && shell_name_ch_is(text[name_end]) == LDG_TRUTH_TRUE) { name_end++; }

    if (name_end >= len || text[name_end] != SHELL_CH_ASSIGN || shell_name_valid_is(text + name_start, name_end - name_start) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    val_start = name_end;
    val_start++;

    if (val_start < len && text[val_start] == SHELL_CH_ASSIGN) { return LDG_ERR_AOK; }

    *out_taken = LDG_TRUTH_TRUE;

    if (shell_cmd_span_find(shell, text + name_start, name_end - name_start) != LDG_SHELL_SEL_NONE) { return LDG_ERR_DENIED; }

    while (val_start < len && shell_space_is(text[val_start]) == LDG_TRUTH_TRUE) { val_start++; }

    text[name_end] = LDG_STR_TERM;
    text[len] = LDG_STR_TERM;

    ret = (val_start == len) ? ldg_shell_alias_unset(shell, text + name_start) : ldg_shell_alias_set(shell, text + name_start, text + val_start);

    text[name_end] = SHELL_CH_ASSIGN;
    text[len] = held;

    return ret;
}

static uint32_t shell_stmt_exec(ldg_shell_t *shell, uint32_t start, uint32_t len)
{
    uint32_t ret = 0;
    uint8_t truth = LDG_TRUTH_INDETERMINATE;
    uint8_t kind = LDG_SHELL_CMD_PLAIN;
    uint8_t taken = LDG_TRUTH_FALSE;

    shell->truth = LDG_TRUTH_INDETERMINATE;
    shell->truth_has = LDG_TRUTH_FALSE;

    ret = shell_alias_stmt_take(shell, start, len, &taken);
    if (taken) { return ret; }

    ret = shell_expr_parse(shell, start, len);
    if (ret != LDG_ERR_AOK)
    {
        if (LDG_UNLIKELY(ldg_shell_out_push(shell, SHELL_MSG_REFUSED) != LDG_ERR_AOK)) { return LDG_ERR_IO_WR; }

        return ret;
    }

    ret = shell_expr_ck(shell);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (shell->expr_cunt != SHELL_PLAIN_ITEM_CUNT) { return shell_expr_eval(shell); }

    ret = shell_leaf_run(shell, SHELL_ITEM_FIRST, &truth, &kind);
    if (ret != LDG_ERR_AOK || kind != LDG_SHELL_CMD_PREDICATE) { return ret; }

    if (LDG_UNLIKELY(truth > LDG_TRUTH_INDETERMINATE)) { return LDG_ERR_INVALID; }

    shell->truth = truth;
    shell->truth_has = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t shell_status_put(ldg_shell_t *shell)
{
    char msg[SHELL_MSG_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t msg_len = 0;
    uint32_t ret = 0;

    if (shell->out_open == LDG_TRUTH_TRUE)
    {
        ret = ldg_shell_out_push(shell, SHELL_MSG_LF);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (shell->status != LDG_ERR_AOK)
    {
        ret = ldg_str_fmt(msg, SHELL_MSG_CAP, &msg_len, SHELL_MSG_ERR_FMT, shell->status);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return ldg_shell_out_push(shell, msg);
    }

    if (shell->truth_has != LDG_TRUTH_TRUE) { return ldg_shell_out_push(shell, SHELL_MSG_OK); }

    if (shell->truth == LDG_TRUTH_TRUE) { return ldg_shell_out_push(shell, SHELL_MSG_TRUE); }

    if (shell->truth == LDG_TRUTH_FALSE) { return ldg_shell_out_push(shell, SHELL_MSG_FALSE); }

    if (shell->truth == LDG_TRUTH_MAYBE) { return ldg_shell_out_push(shell, SHELL_MSG_MAYBE); }

    return ldg_shell_out_push(shell, SHELL_MSG_INDETERMINATE);
}

static uint32_t shell_span_exec(ldg_shell_t *shell, const char *text, uint32_t len)
{
    uint32_t pos = 0;
    uint32_t end = 0;
    uint32_t cmd_idx = 0;
    uint32_t ret = 0;
    uint8_t quote = SHELL_QUOTE_NONE;

    if (LDG_UNLIKELY(len >= shell->conf.line_cap))
    {
        shell->status = LDG_ERR_OVERFLOW;
        shell->truth_has = LDG_TRUTH_FALSE;

        ret = ldg_shell_out_push(shell, SHELL_MSG_REFUSED);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return shell_status_put(shell);
    }

    ret = ldg_mem_cpy(shell->exec_line, text, len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    shell->exec_line[len] = LDG_STR_TERM;

    while (pos < len)
    {
        if (shell_space_is(shell->exec_line[pos]) == LDG_TRUTH_TRUE || shell->exec_line[pos] == SHELL_CH_STMT_END)
        {
            pos++;
            continue;
        }

        end = shell_word_end_get(shell->exec_line, len, pos);
        cmd_idx = shell_cmd_span_find(shell, shell->exec_line + pos, end - pos);

        if (cmd_idx != LDG_SHELL_SEL_NONE && shell->cmd[cmd_idx].remainder == LDG_TRUTH_TRUE) { end = len; }
        else
        {
            quote = SHELL_QUOTE_NONE;
            for (end = pos; end < len; end++)
            {
                if (quote == SHELL_QUOTE_NONE && shell->exec_line[end] == SHELL_CH_STMT_END) { break; }

                quote = shell_quote_next_get(quote, shell->exec_line[end]);
            }
        }

        shell->status = shell_stmt_exec(shell, pos, end - pos);

        ret = shell_status_put(shell);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (shell->status != LDG_ERR_AOK) { break; }

        pos = end;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_line_exec(ldg_shell_t *shell, const char *line)
{
    if (LDG_UNLIKELY(!shell || !line)) { return LDG_ERR_FUNC_ARG_NULL; }

    shell->status = LDG_ERR_AOK;
    shell->truth = LDG_TRUTH_INDETERMINATE;
    shell->truth_has = LDG_TRUTH_FALSE;

    return shell_span_exec(shell, line, shell_len_get(line, shell->conf.line_cap));
}

uint32_t ldg_shell_file_exec(ldg_shell_t *shell, const char *path, const char *src, uint64_t len)
{
    uint64_t path_len = 0;
    uint64_t ext_len = sizeof(LDG_SHELL_SCRIPT_EXT) - LDG_STR_TERM_SIZE;
    uint64_t line_start = 0;
    uint64_t line_end = 0;
    uint64_t idx = 0;
    uint32_t order = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!shell || !path || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_str_len_get(path, UINT32_MAX, &path_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(path_len <= ext_len)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_cmp(path + path_len - ext_len, LDG_SHELL_SCRIPT_EXT, ext_len, &order);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(order != LDG_MEM_ORD_EQ)) { return LDG_ERR_FUNC_ARG_INVALID; }

    shell->status = LDG_ERR_AOK;
    shell->truth = LDG_TRUTH_INDETERMINATE;
    shell->truth_has = LDG_TRUTH_FALSE;

    for (idx = 0; idx <= len && shell->status == LDG_ERR_AOK; idx++)
    {
        if (idx < len && src[idx] != LDG_ASCII_LF) { continue; }

        line_end = idx;
        if (line_end > line_start && src[line_end - LDG_STR_TERM_SIZE] == LDG_ASCII_CR) { line_end--; }

        ret = shell_span_exec(shell, src + line_start, (line_end - line_start < UINT32_MAX) ? (uint32_t)(line_end - line_start) : UINT32_MAX);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        line_start = idx;
        line_start++;
    }

    return LDG_ERR_AOK;
}
