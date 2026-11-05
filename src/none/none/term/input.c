#include <stdint.h>

#include <dangling/term/input.h>
#include <dangling/term/key.h>
#include <dangling/term/vt.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/console.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/mem.h>
#include <dangling/str/str.h>
#include <dangling/sys/tty.h>
#include <dangling/time/time.h>

#define LDG_TERM_INPUT_POOL_ITEM_SIZE_VAR 0
#define LDG_TERM_INPUT_POOL_ALLOC_CUNT 2
#define LDG_TERM_INPUT_PEND_CAP 256
#define LDG_TERM_INPUT_STEP 1
#define LDG_TERM_INPUT_KITTY_FLAGS_NONE 0
#define LDG_TERM_INPUT_ESC 0x1B
#define LDG_TERM_INPUT_CSI_LEAD '['
#define LDG_TERM_INPUT_QUERY_LEAD '?'
#define LDG_TERM_INPUT_KITTY_FINAL 'u'
#define LDG_TERM_INPUT_REPLY_DIGIT_OFF 3
#define LDG_TERM_INPUT_WIN32_FINAL '_'

struct ldg_term_input
{
    ldg_mem_pool_t *pool;
    ldg_term_key_event_t *event;
    uint64_t dropped_cunt;
    uint64_t pend_at;
    uint32_t event_cap;
    uint32_t event_hd;
    uint32_t event_len;
    uint32_t pend_len;
    uint32_t esc_wait;
    uint8_t pend[LDG_TERM_INPUT_PEND_CAP];
    uint8_t key_down[LDG_TERM_KEY_CUNT];
    uint8_t codepoint_down[LDG_TERM_INPUT_CODEPOINT_TRACK_MAX];
    uint8_t kitty_en;
    uint8_t reply_seen;
    uint8_t release_report;
    uint8_t other_keys_en;
    uint8_t pudding[3];
};

static uint32_t input_teardown(ldg_term_input_t *input, uint32_t ret)
{
    ldg_mem_pool_t *pool = input->pool;

    if (input->other_keys_en == LDG_TRUTH_TRUE && LDG_UNLIKELY(ldg_sys_tty_other_keys_en_set(LDG_TRUTH_FALSE) != LDG_ERR_AOK) && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_WR; }

    if (input->kitty_en == LDG_TRUTH_TRUE && LDG_UNLIKELY(ldg_sys_tty_kb_report_en_set(LDG_TERM_INPUT_KITTY_FLAGS_NONE, LDG_TRUTH_FALSE) != LDG_ERR_AOK) && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_WR; }

    if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK) && ret == LDG_ERR_AOK) { ret = LDG_ERR_MEM_POOL_INVALID; }

    return ret;
}

static void input_state_apply(ldg_term_input_t *input, const ldg_term_key_event_t *event)
{
    uint8_t down = LDG_TRUTH_TRUE;

    if (event->kind == LDG_TERM_KEY_KIND_RELEASE || event->key == LDG_TERM_KEY_PRINT) { down = LDG_TRUTH_FALSE; }

    if (event->key != LDG_TERM_KEY_NONE && event->key < LDG_TERM_KEY_CUNT)
    {
        input->key_down[event->key] = down;
        return;
    }

    if (event->codepoint < LDG_TERM_INPUT_CODEPOINT_TRACK_MAX) { input->codepoint_down[event->codepoint] = down; }
}

static void input_event_push(ldg_term_input_t *input, const ldg_term_key_event_t *event)
{
    uint32_t slot = 0;

    if (input->event_len == input->event_cap)
    {
        input->event_hd = (input->event_hd + LDG_TERM_INPUT_STEP) % input->event_cap;
        input->event_len--;
        input->dropped_cunt++;
    }

    slot = (input->event_hd + input->event_len) % input->event_cap;
    input->event[slot] = *event;
    input->event_len++;
}

static void input_kitty_reply_take(ldg_term_input_t *input, const uint8_t *seq, uint64_t len)
{
    uint64_t pos = LDG_TERM_INPUT_REPLY_DIGIT_OFF;
    uint32_t flags = 0;

    if (len <= LDG_TERM_INPUT_REPLY_DIGIT_OFF) { return; }

    if (seq[0] != LDG_TERM_INPUT_ESC || seq[LDG_TERM_INPUT_STEP] != LDG_TERM_INPUT_CSI_LEAD || seq[LDG_TERM_INPUT_REPLY_DIGIT_OFF - LDG_TERM_INPUT_STEP] != LDG_TERM_INPUT_QUERY_LEAD) { return; }

    if (seq[len - LDG_TERM_INPUT_STEP] != LDG_TERM_INPUT_KITTY_FINAL) { return; }

    while (pos < len - LDG_TERM_INPUT_STEP)
    {
        if (ldg_char_digit_is(seq[pos]) != LDG_TRUTH_TRUE) { return; }

        flags = (flags * LDG_BASE_DEC) + (uint32_t)(seq[pos] - LDG_ASCII_DIGIT_ZERO);
        pos++;
    }

    input->reply_seen = LDG_TRUTH_TRUE;
    input->release_report = ((flags & LDG_TERM_VT_KBD_EVENT_TYPE) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static void input_win32_seen_take(ldg_term_input_t *input, const uint8_t *seq, uint64_t len)
{
    if (len <= LDG_TERM_INPUT_STEP || seq[0] != LDG_TERM_INPUT_ESC || seq[len - LDG_TERM_INPUT_STEP] != LDG_TERM_INPUT_WIN32_FINAL) { return; }

    input->reply_seen = LDG_TRUTH_TRUE;
    input->release_report = LDG_TRUTH_TRUE;
}

static void input_repeat_mark(const ldg_term_input_t *input, ldg_term_key_event_t *event)
{
    uint8_t down = LDG_TRUTH_FALSE;

    if (input->release_report != LDG_TRUTH_TRUE || event->kind != LDG_TERM_KEY_KIND_PRESS) { return; }

    if (event->key != LDG_TERM_KEY_NONE && event->key < LDG_TERM_KEY_CUNT) { down = input->key_down[event->key]; }
    else if (event->codepoint < LDG_TERM_INPUT_CODEPOINT_TRACK_MAX) { down = input->codepoint_down[event->codepoint]; }

    if (down == LDG_TRUTH_TRUE) { event->kind = LDG_TERM_KEY_KIND_REPEAT; }
}

uint32_t ldg_term_input_conf_default_get(ldg_term_input_conf_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, (uint64_t)sizeof(*out));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->event_cap = LDG_TERM_INPUT_EVENT_CAP_DEFAULT;
    out->kitty_flags = LDG_TERM_INPUT_KITTY_FLAGS_DEFAULT;
    out->esc_wait = LDG_TERM_INPUT_ESC_WAIT_DEFAULT;
    out->kitty_en = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_input_create(const ldg_term_input_conf_t *conf, ldg_term_input_t **out)
{
    ldg_mem_pool_t *pool = 0x0;
    ldg_term_input_t *input = 0x0;
    void *mem = 0x0;
    uint64_t pool_cap = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(conf->event_cap == 0 || conf->event_cap > LDG_TERM_INPUT_EVENT_CAP_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->kitty_en != LDG_TRUTH_TRUE && conf->kitty_en != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    pool_cap = (uint64_t)sizeof(struct ldg_term_input) + ((uint64_t)conf->event_cap * (uint64_t)sizeof(ldg_term_key_event_t)) + ((uint64_t)LDG_TERM_INPUT_POOL_ALLOC_CUNT * LDG_MEM_POOL_VAR_ALIGN);

    ret = ldg_mem_pool_create(LDG_TERM_INPUT_POOL_ITEM_SIZE_VAR, pool_cap, &pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)sizeof(struct ldg_term_input), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK)) { return LDG_ERR_MEM_POOL_INVALID; }

        return ret;
    }

    input = (ldg_term_input_t *)mem;

    ret = ldg_mem_zero(input, (uint64_t)sizeof(struct ldg_term_input));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK)) { return LDG_ERR_MEM_POOL_INVALID; }

        return ret;
    }

    input->pool = pool;
    input->event_cap = conf->event_cap;
    input->esc_wait = conf->esc_wait;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)conf->event_cap * (uint64_t)sizeof(ldg_term_key_event_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return input_teardown(input, ret); }

    input->event = (ldg_term_key_event_t *)mem;

    ret = ldg_time_monotonic_get(&input->pend_at);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return input_teardown(input, ret); }

    ret = ldg_sys_tty_other_keys_en_set(LDG_TRUTH_TRUE);

    if (ret == LDG_ERR_AOK) { input->other_keys_en = LDG_TRUTH_TRUE; }
    else if (LDG_UNLIKELY(ret != LDG_ERR_UNSUPPORTED)) { return input_teardown(input, ret); }

    if (conf->kitty_en == LDG_TRUTH_TRUE)
    {
        ret = ldg_sys_tty_kb_report_en_set(conf->kitty_flags, LDG_TRUTH_TRUE);

        if (ret == LDG_ERR_AOK) { input->kitty_en = LDG_TRUTH_TRUE; }
        else if (LDG_UNLIKELY(ret != LDG_ERR_UNSUPPORTED)) { return input_teardown(input, ret); }
    }

    *out = input;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_input_destroy(ldg_term_input_t *input)
{
    if (LDG_UNLIKELY(!input)) { return LDG_ERR_FUNC_ARG_NULL; }

    return input_teardown(input, LDG_ERR_AOK);
}

uint32_t ldg_term_input_poll(ldg_term_input_t *input)
{
    ldg_term_key_event_t event = LDG_STRUCT_ZERO_INIT;
    uint64_t now = 0;
    uint64_t got = 0;
    uint64_t off = 0;
    uint64_t used = 0;
    uint32_t ret = 0;
    uint8_t final_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!input)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_io_console_rd(input->pend + input->pend_len, (uint64_t)(LDG_TERM_INPUT_PEND_CAP - input->pend_len), &got);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_time_monotonic_get(&now);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (got > 0)
    {
        input->pend_len += (uint32_t)got;
        input->pend_at = now;
    }

    if (input->pend_len == LDG_TERM_INPUT_PEND_CAP || now - input->pend_at >= (uint64_t)input->esc_wait) { final_is = LDG_TRUTH_TRUE; }

    while (off < input->pend_len)
    {
        ret = ldg_term_key_decode(input->pend + off, (uint64_t)input->pend_len - off, final_is, &event, &used);

        if (ret == LDG_ERR_AGAIN || ret == LDG_ERR_EMPTY) { break; }

        if (used == 0) { used = LDG_TERM_INPUT_STEP; }

        if (ret == LDG_ERR_AOK)
        {
            input_win32_seen_take(input, input->pend + off, used);
            input_repeat_mark(input, &event);
            input_state_apply(input, &event);
            input_event_push(input, &event);
        }
        else if (ret == LDG_ERR_UNSUPPORTED) { input_kitty_reply_take(input, input->pend + off, used); }
        else { return ret; }

        off += used;
    }

    if (off == 0) { return LDG_ERR_AOK; }

    if (off < input->pend_len)
    {
        ret = ldg_mem_mov(input->pend, input->pend + off, (uint64_t)input->pend_len - off);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    input->pend_len -= (uint32_t)off;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_input_event_next(ldg_term_input_t *input, ldg_term_key_event_t *out)
{
    if (LDG_UNLIKELY(!input || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (input->event_len == 0) { return LDG_ERR_EMPTY; }

    *out = input->event[input->event_hd];
    input->event_hd = (input->event_hd + LDG_TERM_INPUT_STEP) % input->event_cap;
    input->event_len--;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_input_dropped_cunt_get(const ldg_term_input_t *input, uint64_t *out)
{
    if (LDG_UNLIKELY(!input || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = input->dropped_cunt;

    return LDG_ERR_AOK;
}

uint8_t ldg_term_input_key_down_is(const ldg_term_input_t *input, uint8_t key)
{
    if (LDG_UNLIKELY(!input)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(key == LDG_TERM_KEY_NONE || key >= LDG_TERM_KEY_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (input->key_down[key] != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (input->release_report == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_MAYBE;
}

uint8_t ldg_term_input_codepoint_down_is(const ldg_term_input_t *input, uint32_t codepoint)
{
    if (LDG_UNLIKELY(!input)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (codepoint >= LDG_TERM_INPUT_CODEPOINT_TRACK_MAX) { return LDG_TRUTH_INDETERMINATE; }

    if (input->codepoint_down[codepoint] != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (input->release_report == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_MAYBE;
}

uint8_t ldg_term_input_release_report_is(const ldg_term_input_t *input)
{
    if (LDG_UNLIKELY(!input)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (input->kitty_en != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (input->reply_seen != LDG_TRUTH_TRUE) { return LDG_TRUTH_INDETERMINATE; }

    return input->release_report;
}
