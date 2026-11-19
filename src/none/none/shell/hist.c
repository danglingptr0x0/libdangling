#include <stdint.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/cpy.h>
#include <dangling/str/str.h>
#include <dangling/shell/shell.h>
#include <dangling/shell/hist.h>

#define SHELL_HIST_NEWEST_OFF 1
#define SHELL_HIST_RING_STEP 1

uint32_t ldg_shell_hist_cunt_get(const ldg_shell_t *shell, uint32_t *out)
{
    if (LDG_UNLIKELY(!shell || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = shell->hist_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_hist_get(const ldg_shell_t *shell, uint32_t idx, const char **out)
{
    uint32_t slot = 0;

    if (LDG_UNLIKELY(!shell || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(idx >= shell->hist_cunt)) { return LDG_ERR_BOUNDS; }

    slot = (shell->hist_hd + shell->hist_cunt - SHELL_HIST_NEWEST_OFF - idx) % shell->hist_max;
    *out = shell->hist + (uint64_t)slot * shell->conf.line_cap;

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_hist_push(ldg_shell_t *shell, const char *text)
{
    const char *newest = 0x0;
    uint64_t len = 0;
    uint32_t slot = 0;
    uint32_t order = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!shell || !text)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_str_len_get(text, shell->conf.line_cap, &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(len >= shell->conf.line_cap)) { return LDG_ERR_OVERFLOW; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (shell->hist_cunt > 0)
    {
        slot = (shell->hist_hd + shell->hist_cunt - SHELL_HIST_NEWEST_OFF) % shell->hist_max;
        newest = shell->hist + (uint64_t)slot * shell->conf.line_cap;

        if (ldg_str_cmp(newest, text, shell->conf.line_cap, &order) == LDG_ERR_AOK && order == LDG_ARITH_ORD_EQ) { return LDG_ERR_AOK; }
    }

    if (shell->hist_cunt < shell->hist_max)
    {
        slot = (shell->hist_hd + shell->hist_cunt) % shell->hist_max;
        shell->hist_cunt++;
    }
    else
    {
        slot = shell->hist_hd;
        shell->hist_hd = (shell->hist_hd + SHELL_HIST_RING_STEP) % shell->hist_max;
    }

    return ldg_mem_cpy(shell->hist + (uint64_t)slot * shell->conf.line_cap, text, len + LDG_STR_TERM_SIZE);
}

uint32_t ldg_shell_hist_io_set(ldg_shell_t *shell, const ldg_shell_hist_io_t *io)
{
    if (LDG_UNLIKELY(!shell || !io)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_mem_cpy(&shell->hist_io, io, sizeof(ldg_shell_hist_io_t));
}

uint32_t ldg_shell_hist_ld(ldg_shell_t *shell)
{
    uint64_t len = 0;
    uint64_t start = 0;
    uint64_t end = 0;
    uint64_t body = 0;
    uint64_t sep_at = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;
    char kind = LDG_SHELL_HIST_KIND_CMD;

    if (LDG_UNLIKELY(!shell)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!shell->hist_io.ld_cb)) { return LDG_ERR_NOT_INIT; }

    ret = shell->hist_io.ld_cb(shell->hist_io.user, shell->hist_blob, shell->hist_blob_cap - LDG_STR_TERM_SIZE, &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(len >= shell->hist_blob_cap)) { return LDG_ERR_OVERFLOW; }

    for (idx = 0; idx <= len; idx++)
    {
        if (idx < len && shell->hist_blob[idx] != LDG_ASCII_LF) { continue; }

        end = idx;
        if (end > start && shell->hist_blob[end - LDG_STR_TERM_SIZE] == LDG_ASCII_CR) { end--; }

        shell->hist_blob[end] = LDG_STR_TERM;
        body = start;
        kind = LDG_SHELL_HIST_KIND_CMD;

        if (end - start > LDG_SHELL_HIST_LEAD_LEN && shell->hist_blob[start + LDG_STR_TERM_SIZE] == LDG_SHELL_HIST_KIND_SEP)
        {
            kind = shell->hist_blob[start];
            body = start + LDG_SHELL_HIST_LEAD_LEN;
        }

        if (kind == LDG_SHELL_HIST_KIND_CMD && end - body < shell->conf.line_cap)
        {
            ret = ldg_shell_hist_push(shell, shell->hist_blob + body);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else if (kind == LDG_SHELL_HIST_KIND_ALIAS && ldg_str_char_find(shell->hist_blob + body, LDG_SHELL_HIST_ALIAS_SEP, end - body, &sep_at) == LDG_ERR_AOK)
        {
            shell->hist_blob[body + sep_at] = LDG_STR_TERM;

            ret = ldg_shell_alias_set(shell, shell->hist_blob + body, shell->hist_blob + body + sep_at + LDG_STR_TERM_SIZE);
            if (ret != LDG_ERR_AOK) { ret = LDG_ERR_AOK; }
        }

        start = idx;
        start++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_shell_hist_st(ldg_shell_t *shell)
{
    const char *text = 0x0;
    const char *name = 0x0;
    uint64_t len = 0;
    uint64_t text_len = 0;
    uint64_t name_len = 0;
    uint32_t remaining = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!shell)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!shell->hist_io.st_cb)) { return LDG_ERR_NOT_INIT; }

    for (remaining = shell->conf.alias_max; remaining > 0; remaining--)
    {
        name = shell->alias_name + (uint64_t)(remaining - SHELL_HIST_NEWEST_OFF) * LDG_SHELL_NAME_CAP;
        text = shell->alias_text + (uint64_t)(remaining - SHELL_HIST_NEWEST_OFF) * shell->conf.line_cap;

        if (name[0] == LDG_STR_TERM) { continue; }

        ret = ldg_str_len_get(name, LDG_SHELL_NAME_CAP, &name_len);
        if (ret == LDG_ERR_AOK) { ret = ldg_str_len_get(text, shell->conf.line_cap, &text_len); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(len + name_len + text_len + LDG_SHELL_HIST_ROW_EXTRA > shell->hist_blob_cap)) { return LDG_ERR_OVERFLOW; }

        shell->hist_blob[len] = LDG_SHELL_HIST_KIND_ALIAS;
        len++;
        shell->hist_blob[len] = LDG_SHELL_HIST_KIND_SEP;
        len++;

        ret = ldg_mem_cpy(shell->hist_blob + len, name, name_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        len += name_len;
        shell->hist_blob[len] = LDG_SHELL_HIST_ALIAS_SEP;
        len++;

        ret = ldg_mem_cpy(shell->hist_blob + len, text, text_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        len += text_len;
        shell->hist_blob[len] = LDG_ASCII_LF;
        len++;
    }

    for (remaining = shell->hist_cunt; remaining > 0; remaining--)
    {
        ret = ldg_shell_hist_get(shell, remaining - SHELL_HIST_NEWEST_OFF, &text);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_str_len_get(text, shell->conf.line_cap, &text_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(len + text_len + LDG_SHELL_HIST_ROW_EXTRA > shell->hist_blob_cap)) { return LDG_ERR_OVERFLOW; }

        shell->hist_blob[len] = LDG_SHELL_HIST_KIND_CMD;
        len++;
        shell->hist_blob[len] = LDG_SHELL_HIST_KIND_SEP;
        len++;

        ret = ldg_mem_cpy(shell->hist_blob + len, text, text_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        len += text_len;
        shell->hist_blob[len] = LDG_ASCII_LF;
        len++;
    }

    return shell->hist_io.st_cb(shell->hist_io.user, shell->hist_blob, len);
}
