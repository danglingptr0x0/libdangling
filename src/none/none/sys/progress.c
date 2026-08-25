
#include <dangling/sys/progress.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

static uint32_t progress_char_append(uint8_t *out, uint64_t cap, uint64_t *pos, uint8_t ch)
{
    if (LDG_UNLIKELY(*pos >= cap)) { return LDG_ERR_STR_TRUNC; }

    out[*pos] = ch;
    (*pos)++;

    return LDG_ERR_AOK;
}

static uint32_t progress_str_append(uint8_t *out, uint64_t cap, uint64_t *pos, const char *str, uint64_t len)
{
    uint64_t idx = 0;
    uint32_t ret = 0;

    for (idx = 0; idx < len; idx++)
    {
        ret = progress_char_append(out, cap, pos, (uint8_t)str[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t progress_cstr_append(uint8_t *out, uint64_t cap, uint64_t *pos, const char *str)
{
    uint32_t ret = 0;

    while (*str != LDG_STR_TERM)
    {
        ret = progress_char_append(out, cap, pos, (uint8_t)*str);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        str++;
    }

    return LDG_ERR_AOK;
}

static uint32_t progress_utf8_append(uint8_t *out, uint64_t cap, uint64_t *pos, uint32_t cp)
{
    uint32_t ret = 0;

    ret = progress_char_append(out, cap, pos, (uint8_t)(0xE0 | (cp >> 12)));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = progress_char_append(out, cap, pos, (uint8_t)(0x80 | ((cp >> 6) & 0x3F)));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return progress_char_append(out, cap, pos, (uint8_t)(0x80 | (cp & 0x3F)));
}

static uint32_t progress_uint_append(uint8_t *out, uint64_t cap, uint64_t *pos, uint64_t val)
{
    uint8_t tmp[20] = LDG_ARR_ZERO_INIT;
    uint32_t len = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (val == 0) { return progress_char_append(out, cap, pos, (uint8_t)'0'); }

    while (val > 0)
    {
        tmp[len] = (uint8_t)('0' + (val % LDG_BASE_DEC));
        val /= LDG_BASE_DEC;
        len++;
    }

    for (idx = 0; idx < len; idx++)
    {
        ret = progress_char_append(out, cap, pos, tmp[len - 1 - idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t progress_pad2_append(uint8_t *out, uint64_t cap, uint64_t *pos, uint64_t val)
{
    uint32_t ret = 0;

    ret = progress_char_append(out, cap, pos, (uint8_t)('0' + (val / LDG_BASE_DEC) % LDG_BASE_DEC));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return progress_char_append(out, cap, pos, (uint8_t)('0' + val % LDG_BASE_DEC));
}

static uint32_t progress_num_append(uint8_t *out, uint64_t cap, uint64_t *pos, uint64_t val, uint32_t unit)
{
    const char *const si_suffix[6] = { "", "k", "M", "G", "T", "P" };
    const char *const iec_suffix[6] = { "", "Ki", "Mi", "Gi", "Ti", "Pi" };
    const char *const *suffix = 0x0;
    uint64_t div = 0;
    uint64_t scaled = 0;
    uint64_t rem = 0;
    uint32_t idx = 0;
    uint32_t dec = 0;
    uint32_t ret = 0;

    if (unit == LDG_SYS_PROGRESS_UNIT_RAW) { return progress_uint_append(out, cap, pos, val); }

    if (unit == LDG_SYS_PROGRESS_UNIT_IEC)
    {
        div = LDG_KIB;
        suffix = iec_suffix;
    }
    else
    {
        div = LDG_MS_PER_SEC;
        suffix = si_suffix;
    }

    scaled = val;

    while (scaled >= div && idx < 5)
    {
        rem = scaled % div;
        scaled /= div;
        idx++;
    }

    ret = progress_uint_append(out, cap, pos, scaled);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (idx > 0)
    {
        dec = (uint32_t)((rem * LDG_BASE_DEC) / div);

        if (dec > 0)
        {
            ret = progress_char_append(out, cap, pos, (uint8_t)'.');
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = progress_uint_append(out, cap, pos, dec);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    return progress_cstr_append(out, cap, pos, suffix[idx]);
}

static uint32_t progress_time_append(uint8_t *out, uint64_t cap, uint64_t *pos, uint64_t secs)
{
    uint64_t hour = 0;
    uint64_t min = 0;
    uint64_t sec = 0;
    uint32_t ret = 0;

    if (secs == UINT64_MAX) { return progress_cstr_append(out, cap, pos, "--:--"); }

    hour = secs / LDG_SECS_PER_HOUR;
    min = (secs % LDG_SECS_PER_HOUR) / LDG_SECS_PER_MIN;
    sec = secs % LDG_SECS_PER_MIN;

    if (hour > 0)
    {
        ret = progress_uint_append(out, cap, pos, hour);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = progress_char_append(out, cap, pos, (uint8_t)':');
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = progress_pad2_append(out, cap, pos, min);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = progress_char_append(out, cap, pos, (uint8_t)':');
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return progress_pad2_append(out, cap, pos, sec);
}

static uint64_t progress_pct_bp(const ldg_sys_progress_t *bar)
{
    uint64_t bp = 0;

    if (bar->done == LDG_TRUTH_TRUE) { return LDG_SYS_PROGRESS_PCT_SCALE; }

    if (bar->conf.total == 0) { return 0; }

    bp = (bar->cur * (uint64_t)LDG_SYS_PROGRESS_PCT_SCALE) / bar->conf.total;

    if (bp > LDG_SYS_PROGRESS_PCT_SCALE) { bp = LDG_SYS_PROGRESS_PCT_SCALE; }

    return bp;
}

static uint32_t progress_bar_append(ldg_sys_progress_t *bar, uint8_t *out, uint64_t cap, uint64_t *pos)
{
    uint64_t total_eighth = 0;
    uint64_t filled = 0;
    uint32_t width = 0;
    uint32_t ascii = 0;
    uint32_t color = 0;
    uint32_t full = 0;
    uint32_t part = 0;
    uint32_t drawn = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    width = bar->conf.width;
    ascii = (bar->conf.flags & LDG_SYS_PROGRESS_FLAG_ASCII) ? 1 : 0;
    color = (bar->conf.flags & LDG_SYS_PROGRESS_FLAG_COLOR) ? 1 : 0;

    total_eighth = (uint64_t)width * 8;
    filled = (progress_pct_bp(bar) * total_eighth) / LDG_SYS_PROGRESS_PCT_SCALE;
    full = (uint32_t)(filled / 8);
    part = (uint32_t)(filled % 8);

    ret = progress_char_append(out, cap, pos, (uint8_t)'[');
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (color)
    {
        ret = progress_cstr_append(out, cap, pos, "\x1b[32m");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < full && drawn < width; idx++)
    {
        if (ascii) { ret = progress_char_append(out, cap, pos, (uint8_t)'#'); }
        else { ret = progress_utf8_append(out, cap, pos, 0x2588); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        drawn++;
    }

    if (part > 0 && drawn < width)
    {
        if (ascii) { if (part >= 4)
            {
                ret = progress_char_append(out, cap, pos, (uint8_t)'#');
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                drawn++;
            }
        }
        else
        {
            ret = progress_utf8_append(out, cap, pos, 0x2590 - part);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            drawn++;
        }
    }

    if (color)
    {
        ret = progress_cstr_append(out, cap, pos, "\x1b[0m");
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (; drawn < width; drawn++)
    {
        ret = progress_char_append(out, cap, pos, (uint8_t)' ');
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return progress_char_append(out, cap, pos, (uint8_t)']');
}

static uint32_t progress_spinner_append(ldg_sys_progress_t *bar, uint8_t *out, uint64_t cap, uint64_t *pos)
{
    const char ascii_frame[4] = { '|', '/', '-', '\\' };
    const uint32_t braille_frame[10] = { 0x280B, 0x2819, 0x2839, 0x2838, 0x283C, 0x2834, 0x2826, 0x2827, 0x2807, 0x280F };

    if (bar->conf.flags & LDG_SYS_PROGRESS_FLAG_ASCII) { return progress_char_append(out, cap, pos, (uint8_t)ascii_frame[bar->spinner_phase % 4]); }

    return progress_utf8_append(out, cap, pos, braille_frame[bar->spinner_phase % 10]);
}

static void progress_rate_update(ldg_sys_progress_t *bar, uint64_t now)
{
    uint64_t dt_tick = 0;
    uint64_t d_cur = 0;
    double dt_sec = 0.0;
    double tau_sec = 0.0;
    double inst = 0.0;
    double alpha = 0.0;

    if (bar->conf.time_freq == 0) { return; }

    if (now <= bar->last_time)
    {
        bar->last_cur = bar->cur;
        return;
    }

    dt_tick = now - bar->last_time;
    d_cur = (bar->cur > bar->last_cur) ? (bar->cur - bar->last_cur) : 0;

    dt_sec = (double)dt_tick / (double)bar->conf.time_freq;
    inst = (double)d_cur / dt_sec;

    if (bar->rate_valid != LDG_TRUTH_TRUE || bar->conf.rate_tau == 0)
    {
        bar->rate = inst;
        bar->rate_valid = LDG_TRUTH_TRUE;
    }
    else
    {
        tau_sec = (double)bar->conf.rate_tau / (double)bar->conf.time_freq;
        alpha = dt_sec / (tau_sec + dt_sec);
        bar->rate = bar->rate + alpha * (inst - bar->rate);
    }

    bar->last_time = now;
    bar->last_cur = bar->cur;
}

LDG_EXPORT uint32_t ldg_sys_progress_conf_default_get(ldg_sys_progress_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_zero(out, sizeof(ldg_sys_progress_conf_t)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    out->label = 0x0;
    out->total = 0;
    out->time_freq = LDG_NS_PER_SEC;
    out->rate_tau = LDG_NS_PER_SEC * 3;
    out->label_len = 0;
    out->width = 40;
    out->mode = LDG_SYS_PROGRESS_MODE_BAR;
    out->unit = LDG_SYS_PROGRESS_UNIT_RAW;
    out->flags = LDG_SYS_PROGRESS_FLAG_PCT | LDG_SYS_PROGRESS_FLAG_CUNT | LDG_SYS_PROGRESS_FLAG_RATE | LDG_SYS_PROGRESS_FLAG_ETA;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sys_progress_init(ldg_sys_progress_t *bar, const ldg_sys_progress_conf_t *conf, uint64_t now)
{
    if (LDG_UNLIKELY(!bar || !conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->mode > LDG_SYS_PROGRESS_MODE_SPINNER)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->unit > LDG_SYS_PROGRESS_UNIT_IEC)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->mode == LDG_SYS_PROGRESS_MODE_BAR && conf->width == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ldg_mem_zero(bar, sizeof(ldg_sys_progress_t)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    bar->is_init = UINT8_MAX;

    bar->conf = *conf;
    bar->cur = 0;
    bar->start_time = now;
    bar->last_time = now;
    bar->last_cur = 0;
    bar->rate = 0.0;
    bar->spinner_phase = 0;
    bar->rate_valid = LDG_TRUTH_FALSE;
    bar->done = LDG_TRUTH_FALSE;
    bar->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sys_progress_rst(ldg_sys_progress_t *bar, uint64_t now)
{
    if (LDG_UNLIKELY(!bar)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bar->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    bar->cur = 0;
    bar->start_time = now;
    bar->last_time = now;
    bar->last_cur = 0;
    bar->rate = 0.0;
    bar->spinner_phase = 0;
    bar->rate_valid = LDG_TRUTH_FALSE;
    bar->done = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sys_progress_advance(ldg_sys_progress_t *bar, uint64_t delta, uint64_t now)
{
    if (LDG_UNLIKELY(!bar)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bar->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (bar->cur > UINT64_MAX - delta) { bar->cur = UINT64_MAX; }
    else { bar->cur += delta; }

    progress_rate_update(bar, now);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sys_progress_cur_set(ldg_sys_progress_t *bar, uint64_t cur, uint64_t now)
{
    if (LDG_UNLIKELY(!bar)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bar->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    bar->cur = cur;

    progress_rate_update(bar, now);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sys_progress_total_set(ldg_sys_progress_t *bar, uint64_t total)
{
    if (LDG_UNLIKELY(!bar)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bar->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    bar->conf.total = total;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sys_progress_finish(ldg_sys_progress_t *bar, uint64_t now)
{
    if (LDG_UNLIKELY(!bar)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bar->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (bar->conf.mode == LDG_SYS_PROGRESS_MODE_BAR && bar->conf.total > 0) { bar->cur = bar->conf.total; }

    progress_rate_update(bar, now);

    bar->done = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sys_progress_render(ldg_sys_progress_t *bar, uint8_t *out, uint64_t cap, uint64_t *len)
{
    uint64_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bar || !out || !len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *len = 0;

    if (LDG_UNLIKELY(bar->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_STR_TRUNC; }

    if (bar->conf.label && bar->conf.label_len > 0)
    {
        ret = progress_str_append(out, cap, &pos, bar->conf.label, bar->conf.label_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = progress_char_append(out, cap, &pos, (uint8_t)' ');
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (bar->conf.mode == LDG_SYS_PROGRESS_MODE_SPINNER)
    {
        ret = progress_spinner_append(bar, out, cap, &pos);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = progress_char_append(out, cap, &pos, (uint8_t)' ');
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = progress_num_append(out, cap, &pos, bar->cur, bar->conf.unit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = progress_char_append(out, cap, &pos, (uint8_t)' ');
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (bar->conf.flags & LDG_SYS_PROGRESS_FLAG_RATE)
        {
            ret = progress_num_append(out, cap, &pos, ldg_sys_progress_rate_get(bar), bar->conf.unit);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = progress_cstr_append(out, cap, &pos, "/s ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        bar->spinner_phase++;
    }
    else
    {
        if (bar->conf.flags & LDG_SYS_PROGRESS_FLAG_PCT)
        {
            ret = progress_uint_append(out, cap, &pos, progress_pct_bp(bar) / 100);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = progress_cstr_append(out, cap, &pos, "% ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = progress_bar_append(bar, out, cap, &pos);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = progress_char_append(out, cap, &pos, (uint8_t)' ');
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (bar->conf.flags & LDG_SYS_PROGRESS_FLAG_CUNT)
        {
            ret = progress_num_append(out, cap, &pos, bar->cur, bar->conf.unit);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = progress_char_append(out, cap, &pos, (uint8_t)'/');
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = progress_num_append(out, cap, &pos, bar->conf.total, bar->conf.unit);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = progress_char_append(out, cap, &pos, (uint8_t)' ');
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (bar->conf.flags & LDG_SYS_PROGRESS_FLAG_RATE)
        {
            ret = progress_num_append(out, cap, &pos, ldg_sys_progress_rate_get(bar), bar->conf.unit);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = progress_cstr_append(out, cap, &pos, "/s ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (bar->conf.flags & LDG_SYS_PROGRESS_FLAG_ETA)
        {
            ret = progress_cstr_append(out, cap, &pos, "eta ");
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = progress_time_append(out, cap, &pos, ldg_sys_progress_eta_get(bar));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = progress_char_append(out, cap, &pos, (uint8_t)' ');
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    if (pos > 0 && out[pos - 1] == (uint8_t)' ') { pos--; }

    *len = pos;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_sys_progress_rate_get(const ldg_sys_progress_t *bar)
{
    if (LDG_UNLIKELY(!bar || bar->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    if (bar->rate_valid != LDG_TRUTH_TRUE || bar->rate < 0.0) { return 0; }

    return (uint64_t)(bar->rate + 0.5);
}

LDG_EXPORT uint64_t ldg_sys_progress_eta_get(const ldg_sys_progress_t *bar)
{
    uint64_t remaining = 0;

    if (LDG_UNLIKELY(!bar || bar->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    if (bar->rate_valid != LDG_TRUTH_TRUE || bar->rate <= 0.0 || bar->conf.total == 0) { return UINT64_MAX; }

    if (bar->cur >= bar->conf.total) { return 0; }

    remaining = bar->conf.total - bar->cur;

    return (uint64_t)((double)remaining / bar->rate + 0.5);
}

LDG_EXPORT uint32_t ldg_sys_progress_pct_get(const ldg_sys_progress_t *bar)
{
    if (LDG_UNLIKELY(!bar || bar->is_init != LDG_TRUTH_TRUE)) { return UINT32_MAX; }

    return (uint32_t)progress_pct_bp(bar);
}

LDG_EXPORT uint64_t ldg_sys_progress_cur_get(const ldg_sys_progress_t *bar)
{
    if (LDG_UNLIKELY(!bar || bar->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    return bar->cur;
}

LDG_EXPORT uint8_t ldg_sys_progress_done_is(const ldg_sys_progress_t *bar)
{
    if (LDG_UNLIKELY(!bar)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bar->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (bar->done) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}
