#include <stdint.h>

#include <dangling/term/term.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_TERM_STEP 1
#define LDG_TERM_TAB_WIDTH 8
#define LDG_TERM_BLANK_CH 0x20
#define LDG_TERM_RED_IDX 0
#define LDG_TERM_GREEN_IDX 1
#define LDG_TERM_BLUE_IDX 2
#define LDG_TERM_DEFAULT_FG 0xC0
#define LDG_TERM_DEFAULT_BG 0x00
#define LDG_TERM_ROW_FIRST 0
#define LDG_TERM_COL_FIRST 0

static ldg_term_cell_t* term_grid_get(const ldg_term_t *term)
{
    if (term->alt_active == LDG_TRUTH_TRUE) { return term->alt_cell; }

    return term->cell;
}

static void term_dirty_mark(ldg_term_t *term, uint32_t row)
{
    if (row < term->dirty_top) { term->dirty_top = row; }

    if (row > term->dirty_bottom) { term->dirty_bottom = row; }
}

static void term_dirty_span_mark(ldg_term_t *term, uint32_t top, uint32_t bottom)
{
    term_dirty_mark(term, top);
    term_dirty_mark(term, bottom);
}

static void term_cell_blank(const ldg_term_t *term, ldg_term_cell_t *cell)
{
    cell->ch = LDG_TERM_BLANK_CH;
    cell->attr = LDG_TERM_ATTR_NONE;
    cell->fg[LDG_TERM_RED_IDX] = term->default_fg[LDG_TERM_RED_IDX];
    cell->fg[LDG_TERM_GREEN_IDX] = term->default_fg[LDG_TERM_GREEN_IDX];
    cell->fg[LDG_TERM_BLUE_IDX] = term->default_fg[LDG_TERM_BLUE_IDX];
    cell->bg[LDG_TERM_RED_IDX] = term->cur_bg[LDG_TERM_RED_IDX];
    cell->bg[LDG_TERM_GREEN_IDX] = term->cur_bg[LDG_TERM_GREEN_IDX];
    cell->bg[LDG_TERM_BLUE_IDX] = term->cur_bg[LDG_TERM_BLUE_IDX];
}

static void term_row_blank(ldg_term_t *term, ldg_term_cell_t *grid, uint32_t row)
{
    uint32_t col = 0;

    for (col = 0; col < term->col_cunt; col++) { term_cell_blank(term, &grid[(row * term->col_cunt) + col]); }
}

static void term_scrollback_push(ldg_term_t *term, const ldg_term_cell_t *row)
{
    uint32_t col = 0;
    uint64_t base = 0;

    if (term->scrollback_cap == 0) { return; }

    base = (uint64_t)term->scrollback_hd * (uint64_t)term->col_cunt;

    for (col = 0; col < term->col_cunt; col++) { term->scrollback[base + col] = row[col]; }

    term->scrollback_hd++;
    if (term->scrollback_hd >= term->scrollback_cap) { term->scrollback_hd = 0; }

    if (term->scrollback_used < term->scrollback_cap) { term->scrollback_used++; }
}

uint32_t ldg_term_conf_default_get(ldg_term_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->col_cunt = LDG_TERM_COL_DEFAULT;
    out->row_cunt = LDG_TERM_ROW_DEFAULT;
    out->scrollback_cunt = LDG_TERM_SCROLLBACK_DEFAULT;
    out->img_cap = LDG_TERM_IMG_CAP_DEFAULT;
    out->img_max = LDG_TERM_IMG_MAX_DEFAULT;
    out->placement_max = LDG_TERM_PLACEMENT_MAX_DEFAULT;
    out->buff_cap = LDG_TERM_BUFF_CAP_DEFAULT;
    out->fg[LDG_TERM_RED_IDX] = LDG_TERM_DEFAULT_FG;
    out->fg[LDG_TERM_GREEN_IDX] = LDG_TERM_DEFAULT_FG;
    out->fg[LDG_TERM_BLUE_IDX] = LDG_TERM_DEFAULT_FG;
    out->bg[LDG_TERM_RED_IDX] = LDG_TERM_DEFAULT_BG;
    out->bg[LDG_TERM_GREEN_IDX] = LDG_TERM_DEFAULT_BG;
    out->bg[LDG_TERM_BLUE_IDX] = LDG_TERM_DEFAULT_BG;
    out->pudding[0] = 0;
    out->pudding[1] = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_pool_size_get(const ldg_term_conf_t *conf, uint64_t *out)
{
    uint64_t cells = 0;
    uint64_t total = 0;

    if (LDG_UNLIKELY(!conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(conf->col_cunt == 0 || conf->row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cells = (uint64_t)conf->col_cunt * (uint64_t)conf->row_cunt;

    total = (uint64_t)sizeof(ldg_term_t);
    total += cells * (uint64_t)sizeof(ldg_term_cell_t) * (uint64_t)LDG_TERM_RGB_CUNT;
    total += (uint64_t)conf->scrollback_cunt * (uint64_t)conf->col_cunt * (uint64_t)sizeof(ldg_term_cell_t);
    total += (uint64_t)conf->col_cunt;
    total += (uint64_t)conf->row_cunt;
    total += (uint64_t)conf->img_cap;
    total += (uint64_t)conf->buff_cap;
    total += (uint64_t)conf->img_max * (uint64_t)sizeof(ldg_term_img_t);
    total += (uint64_t)conf->placement_max * (uint64_t)sizeof(ldg_term_placement_t);

    *out = total;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_create(ldg_mem_pool_t *pool, const ldg_term_conf_t *conf, ldg_term_t **out)
{
    ldg_term_t *term = 0x0;
    void *tmp = 0x0;
    uint64_t cells = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(conf->col_cunt == 0 || conf->col_cunt > LDG_TERM_COL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->row_cunt == 0 || conf->row_cunt > LDG_TERM_ROW_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->scrollback_cunt > LDG_TERM_SCROLLBACK_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cells = (uint64_t)conf->col_cunt * (uint64_t)conf->row_cunt;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)sizeof(ldg_term_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    term = (ldg_term_t *)tmp;

    term->pool = pool;
    term->col_cunt = conf->col_cunt;
    term->row_cunt = conf->row_cunt;
    term->col_cap = conf->col_cunt;
    term->row_cap = conf->row_cunt;
    term->scrollback_cap = conf->scrollback_cunt;
    term->scrollback = 0x0;

    term->default_fg[LDG_TERM_RED_IDX] = conf->fg[LDG_TERM_RED_IDX];
    term->default_fg[LDG_TERM_GREEN_IDX] = conf->fg[LDG_TERM_GREEN_IDX];
    term->default_fg[LDG_TERM_BLUE_IDX] = conf->fg[LDG_TERM_BLUE_IDX];
    term->default_bg[LDG_TERM_RED_IDX] = conf->bg[LDG_TERM_RED_IDX];
    term->default_bg[LDG_TERM_GREEN_IDX] = conf->bg[LDG_TERM_GREEN_IDX];
    term->default_bg[LDG_TERM_BLUE_IDX] = conf->bg[LDG_TERM_BLUE_IDX];

    ret = ldg_mem_pool_alloc(pool, cells * (uint64_t)sizeof(ldg_term_cell_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    term->cell = (ldg_term_cell_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, cells * (uint64_t)sizeof(ldg_term_cell_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    term->alt_cell = (ldg_term_cell_t *)tmp;

    if (conf->scrollback_cunt > 0)
    {
        ret = ldg_mem_pool_alloc(pool, (uint64_t)conf->scrollback_cunt * (uint64_t)conf->col_cunt * (uint64_t)sizeof(ldg_term_cell_t), &tmp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        term->scrollback = (ldg_term_cell_t *)tmp;
    }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)conf->col_cunt, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    term->tab_stop = (uint8_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)conf->row_cunt, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    term->line_attr = (uint8_t *)tmp;

    term->img_cap = conf->img_cap;
    term->img_max = conf->img_max;
    term->placement_max = conf->placement_max;
    term->img_used = 0;

    if (conf->img_max > 0)
    {
        ret = ldg_mem_pool_alloc(pool, (uint64_t)conf->img_max * (uint64_t)sizeof(ldg_term_img_t), &tmp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        term->img = (ldg_term_img_t *)tmp;
    }

    if (conf->placement_max > 0)
    {
        ret = ldg_mem_pool_alloc(pool, (uint64_t)conf->placement_max * (uint64_t)sizeof(ldg_term_placement_t), &tmp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        term->placement = (ldg_term_placement_t *)tmp;
    }

    if (conf->img_cap > 0)
    {
        ret = ldg_mem_pool_alloc(pool, (uint64_t)conf->img_cap, &tmp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        term->img_pixel = (uint8_t *)tmp;
    }

    term->buff_cap = conf->buff_cap;

    if (conf->buff_cap > 0)
    {
        ret = ldg_mem_pool_alloc(pool, (uint64_t)conf->buff_cap, &tmp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        term->buff = (uint8_t *)tmp;
    }

    ret = ldg_term_rst(term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = term;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_destroy(ldg_term_t **term)
{
    if (LDG_UNLIKELY(!term || !*term)) { return LDG_ERR_FUNC_ARG_NULL; }

    *term = 0x0;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_rst(ldg_term_t *term)
{
    uint32_t row = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->cursor_col = 0;
    term->cursor_row = 0;
    term->saved_cursor_col = 0;
    term->saved_cursor_row = 0;
    term->margin_top = 0;
    term->margin_bottom = term->row_cunt - LDG_TERM_STEP;
    term->margin_lhs = 0;
    term->margin_rhs = term->col_cunt - LDG_TERM_STEP;
    term->margin_side = LDG_TRUTH_FALSE;
    term->scrollback_hd = 0;
    term->scrollback_used = 0;
    term->alt_active = LDG_TRUTH_FALSE;
    term->cursor_visible = LDG_TRUTH_TRUE;
    term->wrap_pending = LDG_TRUTH_FALSE;
    term->orig_mode = LDG_TRUTH_FALSE;
    term->autowrap = LDG_TRUTH_TRUE;
    term->insert_mode = LDG_TRUTH_FALSE;
    term->newline_mode = LDG_TRUTH_FALSE;
    term->reverse_screen = LDG_TRUTH_FALSE;
    term->charset_g0 = LDG_TERM_CHARSET_ASCII;
    term->charset_g1 = LDG_TERM_CHARSET_ASCII;
    term->charset_shift = LDG_TRUTH_FALSE;
    term->sync_output = LDG_TRUTH_FALSE;
    term->link_cunt = 0;
    term->link_active = LDG_TERM_LINK_NONE;
    term->cwd_len = 0;
    term->mark_kind = 0;
    term->cur_attr = LDG_TERM_ATTR_NONE;
    term->saved_attr = LDG_TERM_ATTR_NONE;

    ldg_term_palette_rst(term);

    if (term->line_attr) { for (row = 0; row < term->row_cunt; row++) { term->line_attr[row] = LDG_TERM_LINE_SINGLE; } }

    if (term->tab_stop) { for (row = 0; row < term->col_cunt; row++) { term->tab_stop[row] = ((row % LDG_TERM_TAB_DEFAULT_SPAN) == 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; } }

    for (row = 0; row < LDG_TERM_RGB_CUNT; row++)
    {
        term->cur_fg[row] = term->default_fg[row];
        term->cur_bg[row] = term->default_bg[row];
        term->saved_fg[row] = term->default_fg[row];
        term->saved_bg[row] = term->default_bg[row];
    }

    for (row = 0; row < term->row_cunt; row++)
    {
        term_row_blank(term, term->cell, row);
        term_row_blank(term, term->alt_cell, row);
    }

    term->dirty_top = 0;
    term->dirty_bottom = term->row_cunt - LDG_TERM_STEP;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_scroll_up(ldg_term_t *term, uint32_t cunt)
{
    ldg_term_cell_t *grid = 0x0;
    uint32_t step = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t span = 0;
    uint32_t lhs = 0;
    uint32_t rhs = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cunt == 0) { return LDG_ERR_AOK; }

    grid = term_grid_get(term);
    span = term->margin_bottom - term->margin_top + LDG_TERM_STEP;

    if (cunt > span) { cunt = span; }

    lhs = (term->margin_side == LDG_TRUTH_TRUE) ? term->margin_lhs : LDG_TERM_COL_FIRST;
    rhs = (term->margin_side == LDG_TRUTH_TRUE) ? term->margin_rhs : (term->col_cunt - LDG_TERM_STEP);

    for (step = 0; step < cunt; step++)
    {
        if (term->alt_active != LDG_TRUTH_TRUE && term->margin_top == 0) { term_scrollback_push(term, &grid[term->margin_top * term->col_cunt]); }

        for (row = term->margin_top; row < term->margin_bottom; row++) { for (col = lhs; col <= rhs; col++) { grid[(row * term->col_cunt) + col] = grid[((row + LDG_TERM_STEP) * term->col_cunt) + col]; } }

        for (col = lhs; col <= rhs; col++) { term_cell_blank(term, &grid[(term->margin_bottom * term->col_cunt) + col]); }
    }

    term_dirty_span_mark(term, term->margin_top, term->margin_bottom);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_scroll_down(ldg_term_t *term, uint32_t cunt)
{
    ldg_term_cell_t *grid = 0x0;
    uint32_t step = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t span = 0;
    uint32_t lhs = 0;
    uint32_t rhs = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cunt == 0) { return LDG_ERR_AOK; }

    grid = term_grid_get(term);
    span = term->margin_bottom - term->margin_top + LDG_TERM_STEP;

    if (cunt > span) { cunt = span; }

    lhs = (term->margin_side == LDG_TRUTH_TRUE) ? term->margin_lhs : LDG_TERM_COL_FIRST;
    rhs = (term->margin_side == LDG_TRUTH_TRUE) ? term->margin_rhs : (term->col_cunt - LDG_TERM_STEP);

    for (step = 0; step < cunt; step++)
    {
        for (row = term->margin_bottom; row > term->margin_top; row--) { for (col = lhs; col <= rhs; col++) { grid[(row * term->col_cunt) + col] = grid[((row - LDG_TERM_STEP) * term->col_cunt) + col]; } }

        for (col = lhs; col <= rhs; col++) { term_cell_blank(term, &grid[(term->margin_top * term->col_cunt) + col]); }
    }

    term_dirty_span_mark(term, term->margin_top, term->margin_bottom);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_line_feed(ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->wrap_pending = LDG_TRUTH_FALSE;

    if (term->newline_mode == LDG_TRUTH_TRUE) { term->cursor_col = 0; }

    if (term->cursor_row == term->margin_bottom) { return ldg_term_scroll_up(term, LDG_TERM_STEP); }

    if (term->cursor_row + LDG_TERM_STEP >= term->row_cunt) { return LDG_ERR_AOK; }

    term->cursor_row++;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_carriage_ret(ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->cursor_col = (term->margin_side == LDG_TRUTH_TRUE) ? term->margin_lhs : LDG_TERM_COL_FIRST;
    term->wrap_pending = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_backspace(ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->wrap_pending = LDG_TRUTH_FALSE;

    if (term->cursor_col > 0) { term->cursor_col--; }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_tab_advance(ldg_term_t *term)
{
    uint32_t next = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    next = term->cursor_col;

    if (term->tab_stop) { while (next + LDG_TERM_STEP < term->col_cunt)
        {
            next++;
            if (term->tab_stop[next] == LDG_TRUTH_TRUE) { break; }
        }
    }
    else
    {
        next -= next % LDG_TERM_TAB_WIDTH;
        next += LDG_TERM_TAB_WIDTH;
    }

    if (next >= term->col_cunt) { next = term->col_cunt - LDG_TERM_STEP; }

    term->cursor_col = next;
    term->wrap_pending = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

#define LDG_TERM_GRAPHIC_LOW 0x5F
#define LDG_TERM_GRAPHIC_HIGH 0x7E
#define LDG_TERM_UK_POUND 0x23
#define LDG_TERM_ALIGN_CH 0x45

static uint8_t term_graphic_fold(uint8_t ch)
{
    if (ch == 'j' || ch == 'k' || ch == 'l' || ch == 'm' || ch == 'n' || ch == 't' || ch == 'u' || ch == 'v' || ch == 'w') { return '+'; }

    if (ch == 'q' || ch == 'o' || ch == 'p' || ch == 'r' || ch == 's') { return '-'; }

    if (ch == 'x') { return '|'; }

    if (ch == '`') { return '*'; }

    if (ch == 'a') { return ':'; }

    if (ch == 'f') { return '\''; }

    if (ch == 'g') { return '#'; }

    if (ch == 'y' || ch == 'z') { return '<'; }

    if (ch == '~') { return '.'; }

    if (ch == '_') { return ' '; }

    return ch;
}

static uint8_t term_charset_apply(const ldg_term_t *term, uint8_t ch)
{
    uint8_t active = 0;

    active = term->charset_g0;

    if (term->charset_shift == LDG_TRUTH_TRUE) { active = term->charset_g1; }

    if (active == LDG_TERM_CHARSET_GRAPHIC && ch >= LDG_TERM_GRAPHIC_LOW && ch <= LDG_TERM_GRAPHIC_HIGH) { return term_graphic_fold(ch); }

    if (active == LDG_TERM_CHARSET_UK && ch == LDG_TERM_UK_POUND) { return '$'; }

    return ch;
}

uint32_t ldg_term_ch_put(ldg_term_t *term, uint8_t ch)
{
    ldg_term_cell_t *grid = 0x0;
    ldg_term_cell_t *cell = 0x0;
    uint32_t ret = 0;
    uint32_t edge = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (term->wrap_pending == LDG_TRUTH_TRUE && term->autowrap == LDG_TRUTH_TRUE)
    {
        ret = ldg_term_carriage_ret(term);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_term_line_feed(term);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (term->insert_mode == LDG_TRUTH_TRUE) { ldg_term_ch_insert(term, LDG_TERM_STEP); }

    grid = term_grid_get(term);
    cell = &grid[(term->cursor_row * term->col_cunt) + term->cursor_col];

    cell->ch = term_charset_apply(term, ch);
    cell->attr = term->cur_attr;
    cell->link = (uint8_t)term->link_active;
    cell->fg[LDG_TERM_RED_IDX] = term->cur_fg[LDG_TERM_RED_IDX];
    cell->fg[LDG_TERM_GREEN_IDX] = term->cur_fg[LDG_TERM_GREEN_IDX];
    cell->fg[LDG_TERM_BLUE_IDX] = term->cur_fg[LDG_TERM_BLUE_IDX];
    cell->bg[LDG_TERM_RED_IDX] = term->cur_bg[LDG_TERM_RED_IDX];
    cell->bg[LDG_TERM_GREEN_IDX] = term->cur_bg[LDG_TERM_GREEN_IDX];
    cell->bg[LDG_TERM_BLUE_IDX] = term->cur_bg[LDG_TERM_BLUE_IDX];

    term_dirty_mark(term, term->cursor_row);

    edge = (term->margin_side == LDG_TRUTH_TRUE) ? term->margin_rhs : (term->col_cunt - LDG_TERM_STEP);

    if (term->cursor_col >= edge) { term->wrap_pending = LDG_TRUTH_TRUE; }
    else { term->cursor_col++; }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_cursor_set(ldg_term_t *term, uint32_t col, uint32_t row)
{
    uint32_t row_low = 0;
    uint32_t row_high = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    row_low = LDG_TERM_ROW_FIRST;
    row_high = term->row_cunt - LDG_TERM_STEP;

    if (term->orig_mode == LDG_TRUTH_TRUE)
    {
        row_low = term->margin_top;
        row_high = term->margin_bottom;
        row += term->margin_top;
    }

    if (term->orig_mode == LDG_TRUTH_TRUE && term->margin_side == LDG_TRUTH_TRUE)
    {
        col += term->margin_lhs;

        if (col > term->margin_rhs) { col = term->margin_rhs; }
    }

    if (col >= term->col_cunt) { col = term->col_cunt - LDG_TERM_STEP; }

    if (row < row_low) { row = row_low; }

    if (row > row_high) { row = row_high; }

    term->cursor_col = col;
    term->cursor_row = row;
    term->wrap_pending = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_cursor_get(const ldg_term_t *term, uint32_t *out_col, uint32_t *out_row)
{
    if (LDG_UNLIKELY(!term || !out_col || !out_row)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_col = term->cursor_col;
    *out_row = term->cursor_row;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_cursor_col_mov(ldg_term_t *term, int32_t delta)
{
    int64_t target = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    target = (int64_t)term->cursor_col + (int64_t)delta;

    if (target < 0) { target = 0; }

    if (target >= (int64_t)term->col_cunt) { target = (int64_t)term->col_cunt - LDG_TERM_STEP; }

    term->cursor_col = (uint32_t)target;
    term->wrap_pending = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_cursor_row_mov(ldg_term_t *term, int32_t delta)
{
    int64_t target = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    target = (int64_t)term->cursor_row + (int64_t)delta;

    if (target < 0) { target = 0; }

    if (target >= (int64_t)term->row_cunt) { target = (int64_t)term->row_cunt - LDG_TERM_STEP; }

    term->cursor_row = (uint32_t)target;
    term->wrap_pending = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_cursor_save(ldg_term_t *term)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->saved_cursor_col = term->cursor_col;
    term->saved_cursor_row = term->cursor_row;
    term->saved_attr = term->cur_attr;

    for (idx = 0; idx < LDG_TERM_RGB_CUNT; idx++)
    {
        term->saved_fg[idx] = term->cur_fg[idx];
        term->saved_bg[idx] = term->cur_bg[idx];
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_cursor_restore(ldg_term_t *term)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->cursor_col = term->saved_cursor_col;
    term->cursor_row = term->saved_cursor_row;
    term->cur_attr = term->saved_attr;

    for (idx = 0; idx < LDG_TERM_RGB_CUNT; idx++)
    {
        term->cur_fg[idx] = term->saved_fg[idx];
        term->cur_bg[idx] = term->saved_bg[idx];
    }

    if (term->cursor_col >= term->col_cunt) { term->cursor_col = term->col_cunt - LDG_TERM_STEP; }

    if (term->cursor_row >= term->row_cunt) { term->cursor_row = term->row_cunt - LDG_TERM_STEP; }

    term->wrap_pending = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_cursor_visible_set(ldg_term_t *term, uint8_t visible)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(visible > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    term->cursor_visible = visible;

    return LDG_ERR_AOK;
}

uint8_t ldg_term_cursor_visible_is(const ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    return term->cursor_visible;
}

#define LDG_TERM_CUBE_BASE 16
#define LDG_TERM_CUBE_SIDE 6
#define LDG_TERM_CUBE_STEP 40
#define LDG_TERM_CUBE_LIFT 55
#define LDG_TERM_GRAY_BASE 232
#define LDG_TERM_GRAY_STEP 10
#define LDG_TERM_GRAY_LIFT 8
#define LDG_TERM_ANSI_CUNT 16
#define LDG_TERM_ANSI_DIM 128
#define LDG_TERM_ANSI_FULL 255
#define LDG_TERM_COLOR_OFF 0
#define LDG_TERM_PAIR_STEP 2
#define LDG_TERM_BIT_RED 0x1
#define LDG_TERM_BIT_GREEN 0x2
#define LDG_TERM_BIT_BLUE 0x4

static void term_palette_ansi_get(uint32_t idx, uint8_t *out)
{
    uint8_t level = 0;

    level = (idx >= (LDG_TERM_ANSI_CUNT / LDG_TERM_PAIR_STEP)) ? LDG_TERM_ANSI_FULL : LDG_TERM_ANSI_DIM;

    out[LDG_TERM_RED_IDX] = (idx & LDG_TERM_BIT_RED) ? level : LDG_TERM_COLOR_OFF;
    out[LDG_TERM_GREEN_IDX] = (idx & LDG_TERM_BIT_GREEN) ? level : LDG_TERM_COLOR_OFF;
    out[LDG_TERM_BLUE_IDX] = (idx & LDG_TERM_BIT_BLUE) ? level : LDG_TERM_COLOR_OFF;
}

uint32_t ldg_term_palette_rst(ldg_term_t *term)
{
    uint32_t idx = 0;
    uint32_t cube = 0;
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint8_t gray = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < LDG_TERM_ANSI_CUNT; idx++) { term_palette_ansi_get(idx, term->palette[idx]); }

    for (idx = LDG_TERM_CUBE_BASE; idx < LDG_TERM_GRAY_BASE; idx++)
    {
        cube = idx - LDG_TERM_CUBE_BASE;
        red = cube / (LDG_TERM_CUBE_SIDE * LDG_TERM_CUBE_SIDE);
        green = (cube / LDG_TERM_CUBE_SIDE) % LDG_TERM_CUBE_SIDE;
        blue = cube % LDG_TERM_CUBE_SIDE;

        term->palette[idx][LDG_TERM_RED_IDX] = (red > 0) ? (uint8_t)(LDG_TERM_CUBE_LIFT + (red * LDG_TERM_CUBE_STEP)) : LDG_TERM_COLOR_OFF;
        term->palette[idx][LDG_TERM_GREEN_IDX] = (green > 0) ? (uint8_t)(LDG_TERM_CUBE_LIFT + (green * LDG_TERM_CUBE_STEP)) : LDG_TERM_COLOR_OFF;
        term->palette[idx][LDG_TERM_BLUE_IDX] = (blue > 0) ? (uint8_t)(LDG_TERM_CUBE_LIFT + (blue * LDG_TERM_CUBE_STEP)) : LDG_TERM_COLOR_OFF;
    }

    for (idx = LDG_TERM_GRAY_BASE; idx < LDG_TERM_PALETTE_CUNT; idx++)
    {
        gray = (uint8_t)(LDG_TERM_GRAY_LIFT + ((idx - LDG_TERM_GRAY_BASE) * LDG_TERM_GRAY_STEP));

        term->palette[idx][LDG_TERM_RED_IDX] = gray;
        term->palette[idx][LDG_TERM_GREEN_IDX] = gray;
        term->palette[idx][LDG_TERM_BLUE_IDX] = gray;
    }

    for (idx = 0; idx < LDG_TERM_RGB_CUNT; idx++)
    {
        term->role_color[LDG_TERM_COLOR_FG][idx] = term->default_fg[idx];
        term->role_color[LDG_TERM_COLOR_BG][idx] = term->default_bg[idx];
        term->role_color[LDG_TERM_COLOR_CURSOR][idx] = term->default_fg[idx];
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_palette_set(ldg_term_t *term, uint32_t idx, uint8_t red, uint8_t green, uint8_t blue)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(idx >= LDG_TERM_PALETTE_CUNT)) { return LDG_ERR_BOUNDS; }

    term->palette[idx][LDG_TERM_RED_IDX] = red;
    term->palette[idx][LDG_TERM_GREEN_IDX] = green;
    term->palette[idx][LDG_TERM_BLUE_IDX] = blue;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_palette_get(const ldg_term_t *term, uint32_t idx, uint8_t *out)
{
    uint32_t comp = 0;

    if (LDG_UNLIKELY(!term || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(idx >= LDG_TERM_PALETTE_CUNT)) { return LDG_ERR_BOUNDS; }

    for (comp = 0; comp < LDG_TERM_RGB_CUNT; comp++) { out[comp] = term->palette[idx][comp]; }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_role_color_set(ldg_term_t *term, uint32_t role, uint8_t red, uint8_t green, uint8_t blue)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(role >= LDG_TERM_COLOR_ROLE_CUNT)) { return LDG_ERR_BOUNDS; }

    term->role_color[role][LDG_TERM_RED_IDX] = red;
    term->role_color[role][LDG_TERM_GREEN_IDX] = green;
    term->role_color[role][LDG_TERM_BLUE_IDX] = blue;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_role_color_get(const ldg_term_t *term, uint32_t role, uint8_t *out)
{
    uint32_t comp = 0;

    if (LDG_UNLIKELY(!term || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(role >= LDG_TERM_COLOR_ROLE_CUNT)) { return LDG_ERR_BOUNDS; }

    for (comp = 0; comp < LDG_TERM_RGB_CUNT; comp++) { out[comp] = term->role_color[role][comp]; }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_link_open(ldg_term_t *term, const char *uri, uint64_t len)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!term || !uri)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (term->link_cunt + LDG_TERM_STEP >= LDG_TERM_LINK_MAX) { return LDG_ERR_FULL; }

    if (len >= LDG_TERM_LINK_URI_MAX) { len = LDG_TERM_LINK_URI_MAX - LDG_TERM_STEP; }

    term->link_cunt++;

    for (idx = 0; idx < len; idx++) { term->link_uri[term->link_cunt][idx] = uri[idx]; }

    term->link_uri[term->link_cunt][len] = LDG_STR_TERM;
    term->link_active = term->link_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_link_close(ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->link_active = LDG_TERM_LINK_NONE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_link_uri_get(const ldg_term_t *term, uint32_t id, char *out, uint64_t cap)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!term || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    if (LDG_UNLIKELY(id == LDG_TERM_LINK_NONE || id >= LDG_TERM_LINK_MAX)) { return LDG_ERR_NOT_FOUND; }

    while (idx + LDG_TERM_STEP < cap && term->link_uri[id][idx] != LDG_STR_TERM)
    {
        out[idx] = term->link_uri[id][idx];
        idx++;
    }

    out[idx] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_cwd_set(ldg_term_t *term, const char *cwd, uint64_t len)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!term || !cwd)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len >= LDG_TERM_CWD_MAX) { len = LDG_TERM_CWD_MAX - LDG_TERM_STEP; }

    for (idx = 0; idx < len; idx++) { term->cwd[idx] = cwd[idx]; }

    term->cwd[len] = LDG_STR_TERM;
    term->cwd_len = (uint32_t)len;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_cwd_get(const ldg_term_t *term, char *out, uint64_t cap)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!term || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    if (term->cwd_len == 0) { return LDG_ERR_EMPTY; }

    while (idx + LDG_TERM_STEP < cap && idx < term->cwd_len)
    {
        out[idx] = term->cwd[idx];
        idx++;
    }

    out[idx] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_mark_set(ldg_term_t *term, uint32_t kind)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->mark_kind = kind;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_mark_get(const ldg_term_t *term, uint32_t *out)
{
    if (LDG_UNLIKELY(!term || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = term->mark_kind;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_sync_set(ldg_term_t *term, uint8_t en)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->sync_output = en;

    return LDG_ERR_AOK;
}

uint8_t ldg_term_sync_is(const ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    return term->sync_output;
}

uint32_t ldg_term_charset_set(ldg_term_t *term, uint8_t slot, uint8_t charset)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (slot == LDG_TRUTH_FALSE) { term->charset_g0 = charset; }
    else { term->charset_g1 = charset; }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_charset_shift(ldg_term_t *term, uint8_t slot)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->charset_shift = slot;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_tab_set(ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!term->tab_stop)) { return LDG_ERR_NOT_INIT; }

    term->tab_stop[term->cursor_col] = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_tab_clear(ldg_term_t *term, uint8_t all)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!term->tab_stop)) { return LDG_ERR_NOT_INIT; }

    if (all == LDG_TRUTH_TRUE)
    {
        for (idx = 0; idx < term->col_cunt; idx++) { term->tab_stop[idx] = LDG_TRUTH_FALSE; }

        return LDG_ERR_AOK;
    }

    term->tab_stop[term->cursor_col] = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_tab_back(ldg_term_t *term, uint32_t cunt)
{
    uint32_t step = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!term->tab_stop)) { return LDG_ERR_NOT_INIT; }

    for (step = 0; step < cunt; step++) { while (term->cursor_col > 0)
        {
            term->cursor_col--;
            if (term->tab_stop[term->cursor_col] == LDG_TRUTH_TRUE) { break; }
        }
    }

    term->wrap_pending = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_tab_advance_cunt(ldg_term_t *term, uint32_t cunt)
{
    uint32_t step = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cunt > term->col_cunt) { cunt = term->col_cunt; }

    for (step = 0; step < cunt; step++)
    {
        ret = ldg_term_tab_advance(term);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_insert_mode_set(ldg_term_t *term, uint8_t en)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->insert_mode = en;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_newline_mode_set(ldg_term_t *term, uint8_t en)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->newline_mode = en;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_reverse_set(ldg_term_t *term, uint8_t en)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->reverse_screen = en;

    term_dirty_span_mark(term, LDG_TERM_ROW_FIRST, term->row_cunt - LDG_TERM_STEP);

    return LDG_ERR_AOK;
}

uint8_t ldg_term_reverse_is(const ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    return term->reverse_screen;
}

uint32_t ldg_term_line_attr_set(ldg_term_t *term, uint8_t attr)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!term->line_attr)) { return LDG_ERR_NOT_INIT; }

    term->line_attr[term->cursor_row] = attr;

    term_dirty_mark(term, term->cursor_row);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_line_attr_get(const ldg_term_t *term, uint32_t row, uint8_t *out)
{
    if (LDG_UNLIKELY(!term || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_TERM_LINE_SINGLE;

    if (LDG_UNLIKELY(row >= term->row_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(!term->line_attr)) { return LDG_ERR_NOT_INIT; }

    *out = term->line_attr[row];

    return LDG_ERR_AOK;
}

uint32_t ldg_term_align_fill(ldg_term_t *term)
{
    ldg_term_cell_t *grid = 0x0;
    uint32_t col = 0;
    uint32_t row = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    grid = term_grid_get(term);

    for (row = 0; row < term->row_cunt; row++) { for (col = 0; col < term->col_cunt; col++)
        {
            grid[(row * term->col_cunt) + col].ch = LDG_TERM_ALIGN_CH;
            grid[(row * term->col_cunt) + col].attr = LDG_TERM_ATTR_NONE;
        }
    }

    term_dirty_span_mark(term, LDG_TERM_ROW_FIRST, term->row_cunt - LDG_TERM_STEP);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_soft_rst(ldg_term_t *term)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->orig_mode = LDG_TRUTH_FALSE;
    term->autowrap = LDG_TRUTH_TRUE;
    term->insert_mode = LDG_TRUTH_FALSE;
    term->newline_mode = LDG_TRUTH_FALSE;
    term->reverse_screen = LDG_TRUTH_FALSE;
    term->cursor_visible = LDG_TRUTH_TRUE;
    term->charset_g0 = LDG_TERM_CHARSET_ASCII;
    term->charset_g1 = LDG_TERM_CHARSET_ASCII;
    term->charset_shift = LDG_TRUTH_FALSE;
    term->margin_top = 0;
    term->margin_bottom = term->row_cunt - LDG_TERM_STEP;
    term->margin_lhs = 0;
    term->margin_rhs = term->col_cunt - LDG_TERM_STEP;
    term->margin_side = LDG_TRUTH_FALSE;
    term->saved_cursor_col = 0;
    term->saved_cursor_row = 0;

    if (term->line_attr) { for (idx = 0; idx < term->row_cunt; idx++) { term->line_attr[idx] = LDG_TERM_LINE_SINGLE; } }

    ret = ldg_term_attr_rst(term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_term_cursor_set(term, LDG_TERM_COL_FIRST, LDG_TERM_ROW_FIRST);
}

static uint32_t term_img_slot_find(const ldg_term_t *term, uint32_t id, uint32_t *out)
{
    uint32_t idx = 0;

    for (idx = 0; idx < term->img_max; idx++) { if (term->img[idx].alive == LDG_TRUTH_TRUE && term->img[idx].id == id)
        {
            *out = idx;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_term_img_st(ldg_term_t *term, uint32_t id, uint32_t width, uint32_t height, uint8_t fmt, const uint8_t *pixel, uint32_t size)
{
    uint32_t idx = 0;
    uint32_t slot = UINT32_MAX;
    uint32_t byte = 0;

    if (LDG_UNLIKELY(!term || !pixel)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(size == 0 || width == 0 || height == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(fmt != LDG_TERM_IMG_FMT_RGB && fmt != LDG_TERM_IMG_FMT_RGBA)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(!term->img || !term->placement || !term->img_pixel)) { return LDG_ERR_NOT_INIT; }

    if (term_img_slot_find(term, id, &slot) == LDG_ERR_AOK) { term->img[slot].alive = LDG_TRUTH_FALSE; }

    slot = UINT32_MAX;

    for (idx = 0; idx < term->img_max; idx++) { if (term->img[idx].alive != LDG_TRUTH_TRUE)
        {
            slot = idx;
            break;
        }
    }

    if (LDG_UNLIKELY(slot == UINT32_MAX)) { return LDG_ERR_FULL; }

    if (LDG_UNLIKELY(size > term->img_cap - term->img_used)) { return LDG_ERR_FULL; }

    for (byte = 0; byte < size; byte++) { term->img_pixel[term->img_used + byte] = pixel[byte]; }

    term->img[slot].id = id;
    term->img[slot].width = width;
    term->img[slot].height = height;
    term->img[slot].fmt = fmt;
    term->img[slot].offset = term->img_used;
    term->img[slot].size = size;
    term->img[slot].alive = LDG_TRUTH_TRUE;

    term->img_used += size;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_img_drop(ldg_term_t *term, uint32_t id)
{
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = term_img_slot_find(term, id, &slot);
    if (ret != LDG_ERR_AOK) { return ret; }

    term->img[slot].alive = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_img_get(const ldg_term_t *term, uint32_t id, ldg_term_img_t *out)
{
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!term || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = term_img_slot_find(term, id, &slot);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out = term->img[slot];

    return LDG_ERR_AOK;
}

uint32_t ldg_term_img_pixel_get(const ldg_term_t *term, uint32_t id, const uint8_t **out, uint32_t *out_size)
{
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!term || !out || !out_size)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    *out_size = 0;

    ret = term_img_slot_find(term, id, &slot);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out = &term->img_pixel[term->img[slot].offset];
    *out_size = term->img[slot].size;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_placement_add(ldg_term_t *term, const ldg_term_placement_t *placement)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!term || !placement)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!term->placement)) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < term->placement_max; idx++) { if (term->placement[idx].alive != LDG_TRUTH_TRUE)
        {
            term->placement[idx] = *placement;
            term->placement[idx].alive = LDG_TRUTH_TRUE;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_FULL;
}

uint32_t ldg_term_placement_cunt_get(const ldg_term_t *term, uint32_t *out)
{
    uint32_t idx = 0;
    uint32_t live = 0;

    if (LDG_UNLIKELY(!term || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (!term->placement) { return LDG_ERR_AOK; }

    for (idx = 0; idx < term->placement_max; idx++) { if (term->placement[idx].alive == LDG_TRUTH_TRUE) { live++; } }

    *out = live;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_placement_get(const ldg_term_t *term, uint32_t idx, ldg_term_placement_t *out)
{
    uint32_t slot = 0;
    uint32_t live = 0;

    if (LDG_UNLIKELY(!term || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!term->placement)) { return LDG_ERR_NOT_INIT; }

    for (slot = 0; slot < term->placement_max; slot++)
    {
        if (term->placement[slot].alive != LDG_TRUTH_TRUE) { continue; }

        if (live == idx)
        {
            *out = term->placement[slot];
            return LDG_ERR_AOK;
        }

        live++;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_term_placement_clear(ldg_term_t *term)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!term->placement)) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < term->placement_max; idx++) { term->placement[idx].alive = LDG_TRUTH_FALSE; }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_orig_set(ldg_term_t *term, uint8_t en)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(en > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    term->orig_mode = en;

    return ldg_term_cursor_set(term, LDG_TERM_COL_FIRST, LDG_TERM_ROW_FIRST);
}

uint8_t ldg_term_orig_is(const ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    return term->orig_mode;
}

uint32_t ldg_term_autowrap_set(ldg_term_t *term, uint8_t en)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(en > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    term->autowrap = en;

    return LDG_ERR_AOK;
}

uint8_t ldg_term_autowrap_is(const ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    return term->autowrap;
}

uint32_t ldg_term_attr_set(ldg_term_t *term, uint8_t attr)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->cur_attr |= attr;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_attr_clear(ldg_term_t *term, uint8_t attr)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->cur_attr &= (uint8_t)(~attr);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_attr_rst(ldg_term_t *term)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->cur_attr = LDG_TERM_ATTR_NONE;

    for (idx = 0; idx < LDG_TERM_RGB_CUNT; idx++)
    {
        term->cur_fg[idx] = term->default_fg[idx];
        term->cur_bg[idx] = term->default_bg[idx];
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_fg_set(ldg_term_t *term, uint8_t red, uint8_t green, uint8_t blue)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->cur_fg[LDG_TERM_RED_IDX] = red;
    term->cur_fg[LDG_TERM_GREEN_IDX] = green;
    term->cur_fg[LDG_TERM_BLUE_IDX] = blue;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_bg_set(ldg_term_t *term, uint8_t red, uint8_t green, uint8_t blue)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->cur_bg[LDG_TERM_RED_IDX] = red;
    term->cur_bg[LDG_TERM_GREEN_IDX] = green;
    term->cur_bg[LDG_TERM_BLUE_IDX] = blue;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_display_erase(ldg_term_t *term, ldg_term_erase_t mode)
{
    ldg_term_cell_t *grid = 0x0;
    uint32_t row = 0;
    uint32_t col = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    grid = term_grid_get(term);

    if (mode == LDG_TERM_ERASE_SCROLLBACK)
    {
        term->scrollback_hd = 0;
        term->scrollback_used = 0;

        return LDG_ERR_AOK;
    }

    if (mode == LDG_TERM_ERASE_ALL)
    {
        for (row = 0; row < term->row_cunt; row++) { term_row_blank(term, grid, row); }

        term_dirty_span_mark(term, LDG_TERM_ROW_FIRST, term->row_cunt - LDG_TERM_STEP);

        return LDG_ERR_AOK;
    }

    if (mode == LDG_TERM_ERASE_TO_END)
    {
        for (col = term->cursor_col; col < term->col_cunt; col++) { term_cell_blank(term, &grid[(term->cursor_row * term->col_cunt) + col]); }

        for (row = term->cursor_row + LDG_TERM_STEP; row < term->row_cunt; row++) { term_row_blank(term, grid, row); }

        term_dirty_span_mark(term, term->cursor_row, term->row_cunt - LDG_TERM_STEP);

        return LDG_ERR_AOK;
    }

    if (mode == LDG_TERM_ERASE_TO_START)
    {
        for (row = 0; row < term->cursor_row; row++) { term_row_blank(term, grid, row); }

        for (col = 0; col <= term->cursor_col; col++) { term_cell_blank(term, &grid[(term->cursor_row * term->col_cunt) + col]); }

        term_dirty_span_mark(term, LDG_TERM_ROW_FIRST, term->cursor_row);

        return LDG_ERR_AOK;
    }

    return LDG_ERR_FUNC_ARG_INVALID;
}

uint32_t ldg_term_line_erase(ldg_term_t *term, ldg_term_erase_t mode)
{
    ldg_term_cell_t *grid = 0x0;
    uint32_t col = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    grid = term_grid_get(term);

    if (mode == LDG_TERM_ERASE_ALL) { term_row_blank(term, grid, term->cursor_row); }
    else if (mode == LDG_TERM_ERASE_TO_END) { for (col = term->cursor_col; col < term->col_cunt; col++) { term_cell_blank(term, &grid[(term->cursor_row * term->col_cunt) + col]); } }
    else if (mode == LDG_TERM_ERASE_TO_START) { for (col = 0; col <= term->cursor_col; col++) { term_cell_blank(term, &grid[(term->cursor_row * term->col_cunt) + col]); } }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    term_dirty_mark(term, term->cursor_row);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_line_insert(ldg_term_t *term, uint32_t cunt)
{
    ldg_term_cell_t *grid = 0x0;
    uint32_t step = 0;
    uint32_t row = 0;
    uint32_t col = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (term->cursor_row < term->margin_top || term->cursor_row > term->margin_bottom) { return LDG_ERR_AOK; }

    if (cunt > term->row_cunt) { cunt = term->row_cunt; }

    grid = term_grid_get(term);

    for (step = 0; step < cunt; step++)
    {
        for (row = term->margin_bottom; row > term->cursor_row; row--) { for (col = 0; col < term->col_cunt; col++) { grid[(row * term->col_cunt) + col] = grid[((row - LDG_TERM_STEP) * term->col_cunt) + col]; } }

        term_row_blank(term, grid, term->cursor_row);
    }

    term_dirty_span_mark(term, term->cursor_row, term->margin_bottom);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_line_delete(ldg_term_t *term, uint32_t cunt)
{
    ldg_term_cell_t *grid = 0x0;
    uint32_t step = 0;
    uint32_t row = 0;
    uint32_t col = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (term->cursor_row < term->margin_top || term->cursor_row > term->margin_bottom) { return LDG_ERR_AOK; }

    if (cunt > term->row_cunt) { cunt = term->row_cunt; }

    grid = term_grid_get(term);

    for (step = 0; step < cunt; step++)
    {
        for (row = term->cursor_row; row < term->margin_bottom; row++) { for (col = 0; col < term->col_cunt; col++) { grid[(row * term->col_cunt) + col] = grid[((row + LDG_TERM_STEP) * term->col_cunt) + col]; } }

        term_row_blank(term, grid, term->margin_bottom);
    }

    term_dirty_span_mark(term, term->cursor_row, term->margin_bottom);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_ch_delete(ldg_term_t *term, uint32_t cunt)
{
    ldg_term_cell_t *grid = 0x0;
    uint32_t col = 0;
    uint32_t src = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cunt > term->col_cunt) { cunt = term->col_cunt; }

    grid = term_grid_get(term);

    for (col = term->cursor_col; col < term->col_cunt; col++)
    {
        src = col + cunt;

        if (src < term->col_cunt) { grid[(term->cursor_row * term->col_cunt) + col] = grid[(term->cursor_row * term->col_cunt) + src]; }
        else { term_cell_blank(term, &grid[(term->cursor_row * term->col_cunt) + col]); }
    }

    term_dirty_mark(term, term->cursor_row);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_ch_insert(ldg_term_t *term, uint32_t cunt)
{
    ldg_term_cell_t *grid = 0x0;
    uint64_t row_base = 0;
    uint32_t col = 0;
    uint32_t src = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (cunt > term->col_cunt) { cunt = term->col_cunt; }

    grid = term_grid_get(term);
    row_base = (uint64_t)term->cursor_row * term->col_cunt;
    col = term->col_cunt;

    while (col > term->cursor_col)
    {
        col--;

        if (col >= term->cursor_col + cunt)
        {
            src = col - cunt;
            grid[row_base + col] = grid[row_base + src];
        }
        else { term_cell_blank(term, &grid[row_base + col]); }
    }

    term_dirty_mark(term, term->cursor_row);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_ch_erase(ldg_term_t *term, uint32_t cunt)
{
    ldg_term_cell_t *grid = 0x0;
    uint64_t row_base = 0;
    uint32_t col = 0;
    uint32_t last = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    grid = term_grid_get(term);
    row_base = (uint64_t)term->cursor_row * term->col_cunt;
    last = term->cursor_col + cunt;

    if (last > term->col_cunt) { last = term->col_cunt; }

    for (col = term->cursor_col; col < last; col++) { term_cell_blank(term, &grid[row_base + col]); }

    term_dirty_mark(term, term->cursor_row);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_resize(ldg_term_t *term, uint32_t col_cunt, uint32_t row_cunt)
{
    ldg_term_cell_t *grid = 0x0;
    uint32_t last_col = 0;
    uint32_t last_row = 0;
    uint32_t row = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(col_cunt == 0 || row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(col_cunt > term->col_cap || row_cunt > term->row_cap)) { return LDG_ERR_BOUNDS; }

    term->col_cunt = col_cunt;
    term->row_cunt = row_cunt;

    last_col = col_cunt;
    last_col--;
    last_row = row_cunt;
    last_row--;

    if (term->cursor_col > last_col) { term->cursor_col = last_col; }

    if (term->cursor_row > last_row) { term->cursor_row = last_row; }

    term->margin_top = LDG_TERM_ROW_FIRST;
    term->margin_bottom = last_row;
    term->margin_lhs = LDG_TERM_COL_FIRST;
    term->margin_rhs = last_col;
    term->margin_side = LDG_TRUTH_FALSE;
    term->wrap_pending = LDG_TRUTH_FALSE;

    grid = term_grid_get(term);

    for (row = LDG_TERM_ROW_FIRST; row < row_cunt; row++) { term_row_blank(term, grid, row); }

    term_dirty_span_mark(term, LDG_TERM_ROW_FIRST, last_row);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_margin_set(ldg_term_t *term, uint32_t top, uint32_t bottom)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bottom >= term->row_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(top >= bottom)) { return LDG_ERR_FUNC_ARG_INVALID; }

    term->margin_top = top;
    term->margin_bottom = bottom;

    return ldg_term_cursor_set(term, LDG_TERM_COL_FIRST, LDG_TERM_ROW_FIRST);
}

uint32_t ldg_term_margin_side_set(ldg_term_t *term, uint32_t lhs, uint32_t rhs)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(rhs >= term->col_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(lhs >= rhs)) { return LDG_ERR_FUNC_ARG_INVALID; }

    term->margin_lhs = lhs;
    term->margin_rhs = rhs;

    return ldg_term_cursor_set(term, LDG_TERM_COL_FIRST, LDG_TERM_ROW_FIRST);
}

uint32_t ldg_term_margin_side_en(ldg_term_t *term, uint8_t en)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->margin_side = en;

    if (en != LDG_TRUTH_TRUE)
    {
        term->margin_lhs = 0;
        term->margin_rhs = term->col_cunt - LDG_TERM_STEP;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_margin_rst(ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->margin_top = 0;
    term->margin_bottom = term->row_cunt - LDG_TERM_STEP;
    term->margin_lhs = 0;
    term->margin_rhs = term->col_cunt - LDG_TERM_STEP;
    term->margin_side = LDG_TRUTH_FALSE;

    return ldg_term_cursor_set(term, LDG_TERM_COL_FIRST, LDG_TERM_ROW_FIRST);
}

uint32_t ldg_term_alt_enter(ldg_term_t *term)
{
    uint32_t row = 0;

    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (term->alt_active == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    term->alt_active = LDG_TRUTH_TRUE;

    for (row = 0; row < term->row_cunt; row++) { term_row_blank(term, term->alt_cell, row); }

    term->cursor_col = 0;
    term->cursor_row = 0;
    term->wrap_pending = LDG_TRUTH_FALSE;

    term_dirty_span_mark(term, LDG_TERM_ROW_FIRST, term->row_cunt - LDG_TERM_STEP);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_alt_exit(ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (term->alt_active != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    term->alt_active = LDG_TRUTH_FALSE;
    term->wrap_pending = LDG_TRUTH_FALSE;

    term_dirty_span_mark(term, LDG_TERM_ROW_FIRST, term->row_cunt - LDG_TERM_STEP);

    return LDG_ERR_AOK;
}

uint8_t ldg_term_alt_active_is(const ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    return term->alt_active;
}

uint32_t ldg_term_cell_get(const ldg_term_t *term, uint32_t col, uint32_t row, ldg_term_cell_t *out)
{
    const ldg_term_cell_t *grid = 0x0;

    if (LDG_UNLIKELY(!term || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(col >= term->col_cunt || row >= term->row_cunt)) { return LDG_ERR_BOUNDS; }

    grid = term_grid_get(term);

    *out = grid[(row * term->col_cunt) + col];

    return LDG_ERR_AOK;
}

uint32_t ldg_term_row_get(const ldg_term_t *term, uint32_t row, ldg_term_cell_t *out, uint32_t cap)
{
    const ldg_term_cell_t *grid = 0x0;
    uint32_t col = 0;

    if (LDG_UNLIKELY(!term || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(row >= term->row_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(cap < term->col_cunt)) { return LDG_ERR_FULL; }

    grid = term_grid_get(term);

    for (col = 0; col < term->col_cunt; col++) { out[col] = grid[(row * term->col_cunt) + col]; }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_scrollback_row_get(const ldg_term_t *term, uint32_t back, ldg_term_cell_t *out, uint32_t cap)
{
    uint32_t slot = 0;
    uint32_t col = 0;
    uint64_t base = 0;

    if (LDG_UNLIKELY(!term || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap < term->col_cunt)) { return LDG_ERR_FULL; }

    if (LDG_UNLIKELY(back >= term->scrollback_used)) { return LDG_ERR_BOUNDS; }

    slot = term->scrollback_hd + term->scrollback_cap;
    slot -= LDG_TERM_STEP;
    slot -= back;
    slot = slot % term->scrollback_cap;

    base = (uint64_t)slot * (uint64_t)term->col_cunt;

    for (col = 0; col < term->col_cunt; col++) { out[col] = term->scrollback[base + col]; }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_scrollback_cunt_get(const ldg_term_t *term, uint32_t *out)
{
    if (LDG_UNLIKELY(!term || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = term->scrollback_used;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_dirty_get(const ldg_term_t *term, uint32_t *out_top, uint32_t *out_bottom)
{
    if (LDG_UNLIKELY(!term || !out_top || !out_bottom)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_top = 0;
    *out_bottom = 0;

    if (term->dirty_top > term->dirty_bottom) { return LDG_ERR_EMPTY; }

    *out_top = term->dirty_top;
    *out_bottom = term->dirty_bottom;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_dirty_clear(ldg_term_t *term)
{
    if (LDG_UNLIKELY(!term)) { return LDG_ERR_FUNC_ARG_NULL; }

    term->dirty_top = term->row_cunt;
    term->dirty_bottom = 0;

    return LDG_ERR_AOK;
}
