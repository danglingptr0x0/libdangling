#include <stdint.h>

#include <dangling/term/sesh.h>
#include <dangling/term/render.h>
#include <dangling/term/emit.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/console.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/mem.h>
#include <dangling/str/str.h>
#include <dangling/sys/tty.h>
#include <dangling/thread/yield.h>
#include <dangling/time/time.h>

#define LDG_TERM_SESH_POOL_ITEM_SIZE_VAR 0
#define LDG_TERM_SESH_ESC 0x1B
#define LDG_TERM_SESH_OSC_OPEN ']'
#define LDG_TERM_SESH_OSC_TITLE '2'
#define LDG_TERM_SESH_PARAM_SEP ';'
#define LDG_TERM_SESH_BEL 0x07
#define LDG_TERM_SESH_TITLE_HDR_SIZE 4
#define LDG_TERM_SESH_TITLE_END_SIZE 1

struct ldg_term_sesh
{
    ldg_mem_pool_t *pool;
    ldg_mem_pool_t *grid_pool;
    ldg_term_render_t *render;
    ldg_term_emit_t *emit;
    uint64_t wr_period;
    uint64_t wr_last;
    uint64_t dt;
    ldg_sys_tty_raw_t raw;
    ldg_term_render_conf_t render_conf;
    ldg_term_emit_conf_t emit_conf;
    uint32_t col_max;
    uint32_t row_max;
    uint8_t screen_active;
    uint8_t pudding[7];
};

static uint32_t sesh_grid_build(ldg_term_sesh_t *sesh, uint32_t col_cunt, uint32_t row_cunt)
{
    uint32_t ret = 0;

    ret = ldg_mem_pool_rst(sesh->grid_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sesh->render = 0x0;
    sesh->emit = 0x0;
    sesh->render_conf.col_cunt = col_cunt;
    sesh->render_conf.row_cunt = row_cunt;
    sesh->emit_conf.col_cunt = col_cunt;
    sesh->emit_conf.row_cunt = row_cunt;

    ret = ldg_term_render_create(sesh->grid_pool, &sesh->render_conf, &sesh->render);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_term_emit_create(sesh->grid_pool, &sesh->emit_conf, &sesh->emit);
}

static uint32_t sesh_teardown(ldg_term_sesh_t *sesh, uint32_t ret)
{
    ldg_mem_pool_t *pool = sesh->pool;
    ldg_mem_pool_t *grid_pool = sesh->grid_pool;

    if (sesh->screen_active == LDG_TRUTH_TRUE && LDG_UNLIKELY(ldg_term_emit_screen_exit_wr() != LDG_ERR_AOK) && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_WR; }

    if (sesh->raw.is_active == LDG_TRUTH_TRUE && LDG_UNLIKELY(ldg_sys_tty_raw_en_set(&sesh->raw, LDG_TRUTH_FALSE) != LDG_ERR_AOK) && ret == LDG_ERR_AOK) { ret = LDG_ERR_UNSUPPORTED; }

    if (grid_pool && LDG_UNLIKELY(ldg_mem_pool_destroy(&grid_pool) != LDG_ERR_AOK) && ret == LDG_ERR_AOK) { ret = LDG_ERR_MEM_POOL_INVALID; }

    if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK) && ret == LDG_ERR_AOK) { ret = LDG_ERR_MEM_POOL_INVALID; }

    return ret;
}

uint32_t ldg_term_sesh_conf_default_get(ldg_term_sesh_conf_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, (uint64_t)sizeof(*out));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_render_conf_default_get(&out->render);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_emit_conf_default_get(&out->emit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->col_max = LDG_TERM_SESH_COL_MAX_DEFAULT;
    out->row_max = LDG_TERM_SESH_ROW_MAX_DEFAULT;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_sesh_create(const ldg_term_sesh_conf_t *conf, ldg_term_sesh_t **out)
{
    ldg_term_render_conf_t render_max = LDG_STRUCT_ZERO_INIT;
    ldg_term_emit_conf_t emit_max = LDG_STRUCT_ZERO_INIT;
    ldg_mem_pool_t *pool = 0x0;
    ldg_term_sesh_t *sesh = 0x0;
    void *mem = 0x0;
    uint64_t render_size = 0;
    uint64_t emit_size = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(conf->render.col_cunt > conf->col_max || conf->render.row_cunt > conf->row_max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    render_max = conf->render;
    render_max.col_cunt = conf->col_max;
    render_max.row_cunt = conf->row_max;
    emit_max = conf->emit;
    emit_max.col_cunt = conf->col_max;
    emit_max.row_cunt = conf->row_max;

    ret = ldg_term_render_pool_size_get(&render_max, &render_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(render_size > LDG_TERM_RENDER_POOL_MAX)) { return LDG_ERR_RANGE; }

    ret = ldg_term_emit_pool_size_get(&emit_max, &emit_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_create(LDG_TERM_SESH_POOL_ITEM_SIZE_VAR, (uint64_t)sizeof(struct ldg_term_sesh) + LDG_MEM_POOL_VAR_ALIGN, &pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)sizeof(struct ldg_term_sesh), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK)) { return LDG_ERR_MEM_POOL_INVALID; }

        return ret;
    }

    sesh = (ldg_term_sesh_t *)mem;

    ret = ldg_mem_zero(sesh, (uint64_t)sizeof(struct ldg_term_sesh));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK)) { return LDG_ERR_MEM_POOL_INVALID; }

        return ret;
    }

    sesh->pool = pool;
    sesh->wr_period = conf->wr_period;
    sesh->render_conf = conf->render;
    sesh->emit_conf = conf->emit;
    sesh->col_max = conf->col_max;
    sesh->row_max = conf->row_max;

    ret = ldg_mem_pool_create(LDG_TERM_SESH_POOL_ITEM_SIZE_VAR, render_size + emit_size, &sesh->grid_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sesh_teardown(sesh, ret); }

    ret = sesh_grid_build(sesh, conf->render.col_cunt, conf->render.row_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sesh_teardown(sesh, ret); }

    ret = ldg_sys_tty_vt_en_set(LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sesh_teardown(sesh, ret); }

    ret = ldg_sys_tty_raw_en_set(&sesh->raw, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sesh_teardown(sesh, ret); }

    ret = ldg_term_emit_screen_enter_wr();
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sesh_teardown(sesh, ret); }

    sesh->screen_active = LDG_TRUTH_TRUE;

    ret = ldg_time_monotonic_get(&sesh->wr_last);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sesh_teardown(sesh, ret); }

    *out = sesh;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_sesh_destroy(ldg_term_sesh_t *sesh)
{
    if (LDG_UNLIKELY(!sesh)) { return LDG_ERR_FUNC_ARG_NULL; }

    return sesh_teardown(sesh, LDG_ERR_AOK);
}

uint32_t ldg_term_sesh_render_get(const ldg_term_sesh_t *sesh, ldg_term_render_t **out)
{
    if (LDG_UNLIKELY(!sesh || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!sesh->render)) { return LDG_ERR_NOT_INIT; }

    *out = sesh->render;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_sesh_wr(ldg_term_sesh_t *sesh)
{
    const ldg_term_cell_t *cell = 0x0;
    uint64_t now = 0;
    uint64_t due = 0;
    uint64_t cell_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!sesh)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!sesh->render || !sesh->emit)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_term_render_resolve(sesh->render);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_render_cell_arr_get(sesh->render, &cell, &cell_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_time_monotonic_get(&now);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    due = sesh->wr_last + (sesh->wr_period / LDG_NS_PER_MS);
    if (now < due)
    {
        ret = ldg_thread_yield((due - now) * LDG_NS_PER_MS);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_time_monotonic_get(&now);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_term_emit_cell_arr_wr(sesh->emit, cell);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sesh->dt = now - sesh->wr_last;
    sesh->wr_last = now;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_sesh_resize(ldg_term_sesh_t *sesh, uint32_t col_cunt, uint32_t row_cunt)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!sesh)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(col_cunt == 0 || row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(col_cunt > sesh->col_max || row_cunt > sesh->row_max)) { return LDG_ERR_RANGE; }

    if (sesh->render)
    {
        ret = ldg_term_render_destroy(&sesh->render);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (sesh->emit)
    {
        ret = ldg_term_emit_destroy(&sesh->emit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return sesh_grid_build(sesh, col_cunt, row_cunt);
}

uint32_t ldg_term_sesh_title_set(ldg_term_sesh_t *sesh, const char *title, uint64_t len)
{
    uint8_t buff[LDG_TERM_SESH_TITLE_HDR_SIZE + LDG_TERM_SESH_TITLE_MAX + LDG_TERM_SESH_TITLE_END_SIZE] = LDG_ARR_ZERO_INIT;
    uint64_t pos = 0;
    uint64_t idx = 0;
    uint8_t ch = 0;

    if (LDG_UNLIKELY(!sesh || !title)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len > LDG_TERM_SESH_TITLE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    buff[pos] = LDG_TERM_SESH_ESC;
    pos++;
    buff[pos] = LDG_TERM_SESH_OSC_OPEN;
    pos++;
    buff[pos] = LDG_TERM_SESH_OSC_TITLE;
    pos++;
    buff[pos] = LDG_TERM_SESH_PARAM_SEP;
    pos++;

    for (idx = 0; idx < len; idx++)
    {
        ch = (uint8_t)title[idx];
        if (ch < LDG_ASCII_PRINTABLE_MIN || ch > LDG_ASCII_PRINTABLE_MAX) { ch = (uint8_t)LDG_ASCII_NONPRINTABLE_CHAR; }

        buff[pos] = ch;
        pos++;
    }

    buff[pos] = LDG_TERM_SESH_BEL;
    pos++;

    return ldg_io_console_wr(buff, pos);
}

uint32_t ldg_term_sesh_raw_wr(ldg_term_sesh_t *sesh, const uint8_t *src, uint64_t len)
{
    if (LDG_UNLIKELY(!sesh || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(sesh->screen_active != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_io_console_wr(src, len);
}

uint32_t ldg_term_sesh_dt_get(const ldg_term_sesh_t *sesh, uint64_t *out)
{
    if (LDG_UNLIKELY(!sesh || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = sesh->dt;

    return LDG_ERR_AOK;
}
