#include <stdint.h>

#include <dangling/term/emit.h>
#include <dangling/term/term.h>
#include <dangling/io/console.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_TERM_EMIT_STEP 1
#define LDG_TERM_EMIT_RED_IDX 0
#define LDG_TERM_EMIT_GREEN_IDX 1
#define LDG_TERM_EMIT_BLUE_IDX 2
#define LDG_TERM_EMIT_RGB_CUNT 3
#define LDG_TERM_EMIT_ALLOC_CUNT 4
#define LDG_TERM_EMIT_ESC 0x1B
#define LDG_TERM_EMIT_CSI_OPEN '['
#define LDG_TERM_EMIT_PARAM_SEP ';'
#define LDG_TERM_EMIT_PRIVATE '?'
#define LDG_TERM_EMIT_CUP_FINAL 'H'
#define LDG_TERM_EMIT_SGR_FINAL 'm'
#define LDG_TERM_EMIT_MODE_SET_FINAL 'h'
#define LDG_TERM_EMIT_MODE_RST_FINAL 'l'
#define LDG_TERM_EMIT_ERASE_FINAL 'J'
#define LDG_TERM_EMIT_ERASE_ALL 2
#define LDG_TERM_EMIT_SGR_RST 0
#define LDG_TERM_EMIT_SGR_BOLD 1
#define LDG_TERM_EMIT_SGR_DIM 2
#define LDG_TERM_EMIT_SGR_ITALIC 3
#define LDG_TERM_EMIT_SGR_UNDERLINE 4
#define LDG_TERM_EMIT_SGR_BLINK 5
#define LDG_TERM_EMIT_SGR_REVERSE 7
#define LDG_TERM_EMIT_SGR_HIDDEN 8
#define LDG_TERM_EMIT_SGR_STRIKE 9
#define LDG_TERM_EMIT_CUBE_THRESHOLD_SECOND 115
#define LDG_TERM_EMIT_SGR_FG_EXT 38
#define LDG_TERM_EMIT_SGR_BG_EXT 48
#define LDG_TERM_EMIT_SGR_EXT_RGB 2
#define LDG_TERM_EMIT_SGR_EXT_PALETTE 5
#define LDG_TERM_EMIT_MODE_ALT_SCREEN 1049
#define LDG_TERM_EMIT_MODE_CURSOR 25
#define LDG_TERM_EMIT_MODE_SYNC 2026
#define LDG_TERM_EMIT_CURSOR_UNKNOWN UINT32_MAX
#define LDG_TERM_EMIT_PREV_CH_NONE UINT8_MAX
#define LDG_TERM_EMIT_DEC_DIGIT_MAX 10
#define LDG_TERM_EMIT_DIGIT_ZERO '0'
#define LDG_TERM_EMIT_SEQ_MAX 16
#define LDG_TERM_EMIT_CUBE_BASE 16
#define LDG_TERM_EMIT_CUBE_SIDE 6
#define LDG_TERM_EMIT_CUBE_LEVEL_FIRST 95
#define LDG_TERM_EMIT_CUBE_LEVEL_STEP 40
#define LDG_TERM_EMIT_GRAY_BASE 232
#define LDG_TERM_EMIT_GRAY_CUNT 24
#define LDG_TERM_EMIT_GRAY_FIRST 8
#define LDG_TERM_EMIT_GRAY_STEP 10
#define LDG_TERM_EMIT_GRAY_ROUND 3
#define LDG_TERM_EMIT_CUBE_THRESHOLD_FIRST 48
#define LDG_TERM_EMIT_LEVEL_DARKEST 0
#define LDG_TERM_EMIT_CHANNEL_MIN 0
#define LDG_TERM_EMIT_CHANNEL_MAX 255
#define LDG_TERM_EMIT_PLANE_CUNT 2
#define LDG_TERM_EMIT_PLANE_FG 0
#define LDG_TERM_EMIT_PLANE_BG 1
#define LDG_TERM_EMIT_DIFFUSE_ROW_CUNT 2
#define LDG_TERM_EMIT_DIFFUSE_GUARD 2
#define LDG_TERM_EMIT_DIFFUSE_RHS 7
#define LDG_TERM_EMIT_DIFFUSE_DOWN_LHS 3
#define LDG_TERM_EMIT_DIFFUSE_DOWN 5
#define LDG_TERM_EMIT_DIFFUSE_DOWN_RHS 1
#define LDG_TERM_EMIT_DIFFUSE_DENOMINATOR 16

typedef struct emit_sink
{
    ldg_byte_t *buff;
    uint64_t cap;
    uint64_t len;
    uint8_t console;
    uint8_t pudding[7];
} emit_sink_t;

static uint32_t emit_sink_flush(emit_sink_t *sink)
{
    uint32_t ret = 0;

    if (sink->console != LDG_TRUTH_TRUE) { return LDG_ERR_FULL; }

    if (sink->len == 0) { return LDG_ERR_AOK; }

    ret = ldg_io_console_wr(sink->buff, sink->len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sink->len = 0;

    return LDG_ERR_AOK;
}

static uint32_t emit_reserve(emit_sink_t *sink, uint64_t need)
{
    uint32_t ret = 0;

    if (sink->len + need <= sink->cap) { return LDG_ERR_AOK; }

    ret = emit_sink_flush(sink);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(need > sink->cap)) { return LDG_ERR_FULL; }

    return LDG_ERR_AOK;
}

static void emit_byte_st(emit_sink_t *sink, uint8_t byte)
{
    sink->buff[sink->len] = byte;
    sink->len++;
}

static void emit_dec_st(emit_sink_t *sink, uint32_t val)
{
    uint8_t digit[LDG_TERM_EMIT_DEC_DIGIT_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t cunt = 0;

    if (val < LDG_BASE_DEC)
    {
        emit_byte_st(sink, (uint8_t)(LDG_TERM_EMIT_DIGIT_ZERO + val));
        return;
    }

    if (val < LDG_BASE_DEC * LDG_BASE_DEC)
    {
        emit_byte_st(sink, (uint8_t)(LDG_TERM_EMIT_DIGIT_ZERO + (val / LDG_BASE_DEC)));
        emit_byte_st(sink, (uint8_t)(LDG_TERM_EMIT_DIGIT_ZERO + (val % LDG_BASE_DEC)));
        return;
    }

    if (val < LDG_BASE_DEC * LDG_BASE_DEC * LDG_BASE_DEC)
    {
        emit_byte_st(sink, (uint8_t)(LDG_TERM_EMIT_DIGIT_ZERO + (val / (LDG_BASE_DEC * LDG_BASE_DEC))));
        emit_byte_st(sink, (uint8_t)(LDG_TERM_EMIT_DIGIT_ZERO + ((val / LDG_BASE_DEC) % LDG_BASE_DEC)));
        emit_byte_st(sink, (uint8_t)(LDG_TERM_EMIT_DIGIT_ZERO + (val % LDG_BASE_DEC)));
        return;
    }

    while (val > 0)
    {
        digit[cunt] = (uint8_t)(LDG_TERM_EMIT_DIGIT_ZERO + (val % LDG_BASE_DEC));
        val /= LDG_BASE_DEC;
        cunt++;
    }

    while (cunt > 0)
    {
        cunt--;
        emit_byte_st(sink, digit[cunt]);
    }
}

static void emit_csi_st(emit_sink_t *sink)
{
    emit_byte_st(sink, LDG_TERM_EMIT_ESC);
    emit_byte_st(sink, LDG_TERM_EMIT_CSI_OPEN);
}

static void emit_private_mode_st(emit_sink_t *sink, uint32_t mode, uint8_t en)
{
    emit_csi_st(sink);
    emit_byte_st(sink, LDG_TERM_EMIT_PRIVATE);
    emit_dec_st(sink, mode);
    emit_byte_st(sink, (en == LDG_TRUTH_TRUE) ? LDG_TERM_EMIT_MODE_SET_FINAL : LDG_TERM_EMIT_MODE_RST_FINAL);
}

static void emit_sgr_rst_st(emit_sink_t *sink)
{
    emit_csi_st(sink);
    emit_dec_st(sink, LDG_TERM_EMIT_SGR_RST);
    emit_byte_st(sink, LDG_TERM_EMIT_SGR_FINAL);
}

static void emit_erase_st(emit_sink_t *sink)
{
    emit_csi_st(sink);
    emit_dec_st(sink, LDG_TERM_EMIT_ERASE_ALL);
    emit_byte_st(sink, LDG_TERM_EMIT_ERASE_FINAL);
}

static void emit_cup_st(emit_sink_t *sink, uint32_t col, uint32_t row)
{
    emit_csi_st(sink);
    emit_dec_st(sink, row + LDG_TERM_EMIT_STEP);
    emit_byte_st(sink, LDG_TERM_EMIT_PARAM_SEP);
    emit_dec_st(sink, col + LDG_TERM_EMIT_STEP);
    emit_byte_st(sink, LDG_TERM_EMIT_CUP_FINAL);
}

static uint8_t emit_cube_level_get(uint32_t step)
{
    uint32_t level = 0;

    if (step > 0) { level = LDG_TERM_EMIT_CUBE_LEVEL_FIRST + ((step - LDG_TERM_EMIT_STEP) * LDG_TERM_EMIT_CUBE_LEVEL_STEP); }

    return (uint8_t)level;
}

static void emit_palette_build(uint8_t *palette)
{
    uint32_t idx = 0;
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint32_t level = 0;
    uint8_t *entry = 0x0;

    for (idx = 0; idx < LDG_TERM_EMIT_CUBE_BASE; idx++)
    {
        entry = &palette[idx * LDG_TERM_EMIT_RGB_CUNT];
        entry[LDG_TERM_EMIT_RED_IDX] = 0;
        entry[LDG_TERM_EMIT_GREEN_IDX] = 0;
        entry[LDG_TERM_EMIT_BLUE_IDX] = 0;
    }

    for (red = 0; red < LDG_TERM_EMIT_CUBE_SIDE; red++) { for (green = 0; green < LDG_TERM_EMIT_CUBE_SIDE; green++) { for (blue = 0; blue < LDG_TERM_EMIT_CUBE_SIDE; blue++)
            {
                idx = LDG_TERM_EMIT_CUBE_BASE + (red * LDG_TERM_EMIT_CUBE_SIDE * LDG_TERM_EMIT_CUBE_SIDE) + (green * LDG_TERM_EMIT_CUBE_SIDE) + blue;
                entry = &palette[idx * LDG_TERM_EMIT_RGB_CUNT];
                entry[LDG_TERM_EMIT_RED_IDX] = emit_cube_level_get(red);
                entry[LDG_TERM_EMIT_GREEN_IDX] = emit_cube_level_get(green);
                entry[LDG_TERM_EMIT_BLUE_IDX] = emit_cube_level_get(blue);
            }
        } }

    for (idx = 0; idx < LDG_TERM_EMIT_GRAY_CUNT; idx++)
    {
        level = LDG_TERM_EMIT_GRAY_FIRST + (idx * LDG_TERM_EMIT_GRAY_STEP);
        entry = &palette[(LDG_TERM_EMIT_GRAY_BASE + idx) * LDG_TERM_EMIT_RGB_CUNT];
        entry[LDG_TERM_EMIT_RED_IDX] = (uint8_t)level;
        entry[LDG_TERM_EMIT_GREEN_IDX] = (uint8_t)level;
        entry[LDG_TERM_EMIT_BLUE_IDX] = (uint8_t)level;
    }
}

static uint32_t emit_cube_step_get(uint32_t val)
{
    uint32_t step = LDG_TERM_EMIT_LEVEL_DARKEST;

    if (val < LDG_TERM_EMIT_CUBE_THRESHOLD_FIRST) { return step; }

    if (val < LDG_TERM_EMIT_CUBE_THRESHOLD_SECOND) { return LDG_TERM_EMIT_STEP; }

    step = LDG_TERM_EMIT_STEP + LDG_TERM_EMIT_STEP + ((val - LDG_TERM_EMIT_CUBE_THRESHOLD_SECOND) / LDG_TERM_EMIT_CUBE_LEVEL_STEP);

    if (step >= LDG_TERM_EMIT_CUBE_SIDE) { step = LDG_TERM_EMIT_CUBE_SIDE - LDG_TERM_EMIT_STEP; }

    return step;
}

static int64_t emit_palette_dist_get(const uint8_t *palette, uint32_t idx, const uint8_t *rgb)
{
    const uint8_t *entry = &palette[idx * LDG_TERM_EMIT_RGB_CUNT];
    int64_t dist = 0;
    int64_t delta = 0;
    uint32_t channel = 0;

    for (channel = 0; channel < LDG_TERM_EMIT_RGB_CUNT; channel++)
    {
        delta = (int64_t)entry[channel] - (int64_t)rgb[channel];
        dist += delta * delta;
    }

    return dist;
}

static uint32_t emit_palette_nearest_get(const uint8_t *palette, const uint8_t *rgb)
{
    uint32_t cube_idx = 0;
    uint32_t gray_idx = LDG_TERM_EMIT_LEVEL_DARKEST;
    uint32_t mean = 0;

    cube_idx = LDG_TERM_EMIT_CUBE_BASE + (emit_cube_step_get(rgb[LDG_TERM_EMIT_RED_IDX]) * LDG_TERM_EMIT_CUBE_SIDE * LDG_TERM_EMIT_CUBE_SIDE) + (emit_cube_step_get(rgb[LDG_TERM_EMIT_GREEN_IDX]) * LDG_TERM_EMIT_CUBE_SIDE) + emit_cube_step_get(rgb[LDG_TERM_EMIT_BLUE_IDX]);

    mean = ((uint32_t)rgb[LDG_TERM_EMIT_RED_IDX] + rgb[LDG_TERM_EMIT_GREEN_IDX] + rgb[LDG_TERM_EMIT_BLUE_IDX]) / LDG_TERM_EMIT_RGB_CUNT;

    if (mean > LDG_TERM_EMIT_GRAY_ROUND) { gray_idx = (mean - LDG_TERM_EMIT_GRAY_ROUND) / LDG_TERM_EMIT_GRAY_STEP; }

    if (gray_idx >= LDG_TERM_EMIT_GRAY_CUNT) { gray_idx = LDG_TERM_EMIT_GRAY_CUNT - LDG_TERM_EMIT_STEP; }

    gray_idx += LDG_TERM_EMIT_GRAY_BASE;

    if (emit_palette_dist_get(palette, gray_idx, rgb) < emit_palette_dist_get(palette, cube_idx, rgb)) { return gray_idx; }

    return cube_idx;
}

static void emit_color_param_st(const ldg_term_emit_t *emit, emit_sink_t *sink, uint32_t sel, const uint8_t *rgb)
{
    uint32_t channel = 0;

    emit_dec_st(sink, sel);
    emit_byte_st(sink, LDG_TERM_EMIT_PARAM_SEP);

    if (emit->color_mode == LDG_TERM_EMIT_COLOR_PALETTE)
    {
        emit_dec_st(sink, LDG_TERM_EMIT_SGR_EXT_PALETTE);
        emit_byte_st(sink, LDG_TERM_EMIT_PARAM_SEP);
        emit_dec_st(sink, emit_palette_nearest_get(emit->palette, rgb));
        return;
    }

    emit_dec_st(sink, LDG_TERM_EMIT_SGR_EXT_RGB);

    for (channel = 0; channel < LDG_TERM_EMIT_RGB_CUNT; channel++)
    {
        emit_byte_st(sink, LDG_TERM_EMIT_PARAM_SEP);
        emit_dec_st(sink, rgb[channel]);
    }
}

static uint8_t emit_rgb_eq_is(const uint8_t *lhs, const uint8_t *rhs)
{
    uint32_t channel = 0;

    for (channel = 0; channel < LDG_TERM_EMIT_RGB_CUNT; channel++) { if (lhs[channel] != rhs[channel]) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static uint8_t emit_cell_eq_is(const ldg_term_cell_t *lhs, const ldg_term_cell_t *rhs)
{
    if (lhs->ch != rhs->ch || lhs->attr != rhs->attr) { return LDG_TRUTH_FALSE; }

    if (emit_rgb_eq_is(lhs->fg, rhs->fg) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (emit_rgb_eq_is(lhs->bg, rhs->bg) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static void emit_rgb_cpy(uint8_t *dst, const uint8_t *src)
{
    uint32_t channel = 0;

    for (channel = 0; channel < LDG_TERM_EMIT_RGB_CUNT; channel++) { dst[channel] = src[channel]; }
}

static void emit_attr_param_st(emit_sink_t *sink, uint8_t attr)
{
    const uint8_t bit[] = { LDG_TERM_ATTR_BOLD, LDG_TERM_ATTR_DIM, LDG_TERM_ATTR_ITALIC, LDG_TERM_ATTR_UNDERLINE, LDG_TERM_ATTR_BLINK, LDG_TERM_ATTR_REVERSE, LDG_TERM_ATTR_HIDDEN, LDG_TERM_ATTR_STRIKE };
    const uint8_t code[] = { LDG_TERM_EMIT_SGR_BOLD, LDG_TERM_EMIT_SGR_DIM, LDG_TERM_EMIT_SGR_ITALIC, LDG_TERM_EMIT_SGR_UNDERLINE, LDG_TERM_EMIT_SGR_BLINK, LDG_TERM_EMIT_SGR_REVERSE, LDG_TERM_EMIT_SGR_HIDDEN, LDG_TERM_EMIT_SGR_STRIKE };
    uint32_t idx = 0;

    emit_dec_st(sink, LDG_TERM_EMIT_SGR_RST);

    for (idx = 0; idx < sizeof(bit); idx++)
    {
        if ((attr & bit[idx]) == 0) { continue; }

        emit_byte_st(sink, LDG_TERM_EMIT_PARAM_SEP);
        emit_dec_st(sink, code[idx]);
    }
}

static void emit_sgr_st(ldg_term_emit_t *emit, emit_sink_t *sink, const ldg_term_cell_t *cell)
{
    uint8_t fg_same = LDG_TRUTH_FALSE;
    uint8_t bg_same = LDG_TRUTH_FALSE;
    uint8_t attr_same = LDG_TRUTH_FALSE;
    uint8_t sep = LDG_TRUTH_FALSE;

    if (emit->color_known == LDG_TRUTH_TRUE)
    {
        fg_same = emit_rgb_eq_is(emit->last_fg, cell->fg);
        bg_same = emit_rgb_eq_is(emit->last_bg, cell->bg);
        attr_same = (emit->last_attr == cell->attr) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    }

    if (attr_same != LDG_TRUTH_TRUE)
    {
        fg_same = LDG_TRUTH_FALSE;
        bg_same = LDG_TRUTH_FALSE;
    }

    if (emit->fg_only == LDG_TRUTH_TRUE) { bg_same = LDG_TRUTH_TRUE; }

    if (fg_same == LDG_TRUTH_TRUE && bg_same == LDG_TRUTH_TRUE && attr_same == LDG_TRUTH_TRUE) { return; }

    emit_csi_st(sink);

    if (attr_same != LDG_TRUTH_TRUE)
    {
        emit_attr_param_st(sink, cell->attr);
        sep = LDG_TRUTH_TRUE;
    }

    if (fg_same != LDG_TRUTH_TRUE)
    {
        if (sep == LDG_TRUTH_TRUE) { emit_byte_st(sink, LDG_TERM_EMIT_PARAM_SEP); }

        emit_color_param_st(emit, sink, LDG_TERM_EMIT_SGR_FG_EXT, cell->fg);
        sep = LDG_TRUTH_TRUE;
    }

    if (bg_same != LDG_TRUTH_TRUE)
    {
        if (sep == LDG_TRUTH_TRUE) { emit_byte_st(sink, LDG_TERM_EMIT_PARAM_SEP); }

        emit_color_param_st(emit, sink, LDG_TERM_EMIT_SGR_BG_EXT, cell->bg);
    }

    emit_byte_st(sink, LDG_TERM_EMIT_SGR_FINAL);

    emit_rgb_cpy(emit->last_fg, cell->fg);
    emit_rgb_cpy(emit->last_bg, cell->bg);
    emit->last_attr = cell->attr;
    emit->color_known = LDG_TRUTH_TRUE;
}

static uint64_t emit_diffuse_stride_get(const ldg_term_emit_t *emit)
{
    return ((uint64_t)emit->col_cunt + LDG_TERM_EMIT_DIFFUSE_GUARD) * LDG_TERM_EMIT_PLANE_CUNT * LDG_TERM_EMIT_RGB_CUNT;
}

static int32_t* emit_diffuse_get(const ldg_term_emit_t *emit, uint32_t row_sel, int64_t col, uint32_t plane)
{
    uint64_t base = 0;

    base = ((uint64_t)row_sel * emit_diffuse_stride_get(emit)) + ((((uint64_t)(col + LDG_TERM_EMIT_STEP)) * LDG_TERM_EMIT_PLANE_CUNT + plane) * LDG_TERM_EMIT_RGB_CUNT);

    return &emit->diffuse[base];
}

static void emit_diffuse_spread(int32_t *tgt, const int32_t *residual, int32_t weight)
{
    uint32_t channel = 0;

    for (channel = 0; channel < LDG_TERM_EMIT_RGB_CUNT; channel++) { tgt[channel] += (residual[channel] * weight) / LDG_TERM_EMIT_DIFFUSE_DENOMINATOR; }
}

static void emit_plane_quantize(ldg_term_emit_t *emit, uint32_t row_sel, uint32_t col, uint32_t plane, const uint8_t *src, uint8_t *dst)
{
    uint8_t tgt[LDG_TERM_EMIT_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int32_t residual[LDG_TERM_EMIT_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int32_t *carried = 0x0;
    const uint8_t *entry = 0x0;
    int32_t val = 0;
    uint32_t channel = 0;
    uint32_t next_row = (row_sel + LDG_TERM_EMIT_STEP) % LDG_TERM_EMIT_DIFFUSE_ROW_CUNT;

    carried = emit_diffuse_get(emit, row_sel, col, plane);

    for (channel = 0; channel < LDG_TERM_EMIT_RGB_CUNT; channel++)
    {
        val = (int32_t)src[channel] + carried[channel];

        if (val < LDG_TERM_EMIT_CHANNEL_MIN) { val = LDG_TERM_EMIT_CHANNEL_MIN; }

        if (val > LDG_TERM_EMIT_CHANNEL_MAX) { val = LDG_TERM_EMIT_CHANNEL_MAX; }

        tgt[channel] = (uint8_t)val;
    }

    entry = &emit->palette[emit_palette_nearest_get(emit->palette, tgt) * LDG_TERM_EMIT_RGB_CUNT];

    for (channel = 0; channel < LDG_TERM_EMIT_RGB_CUNT; channel++)
    {
        dst[channel] = entry[channel];
        residual[channel] = (int32_t)tgt[channel] - (int32_t)entry[channel];
    }

    emit_diffuse_spread(emit_diffuse_get(emit, row_sel, (int64_t)col + LDG_TERM_EMIT_STEP, plane), residual, LDG_TERM_EMIT_DIFFUSE_RHS);
    emit_diffuse_spread(emit_diffuse_get(emit, next_row, (int64_t)col - LDG_TERM_EMIT_STEP, plane), residual, LDG_TERM_EMIT_DIFFUSE_DOWN_LHS);
    emit_diffuse_spread(emit_diffuse_get(emit, next_row, (int64_t)col, plane), residual, LDG_TERM_EMIT_DIFFUSE_DOWN);
    emit_diffuse_spread(emit_diffuse_get(emit, next_row, (int64_t)col + LDG_TERM_EMIT_STEP, plane), residual, LDG_TERM_EMIT_DIFFUSE_DOWN_RHS);
}

static void emit_cell_arr_quantize(ldg_term_emit_t *emit, const ldg_term_cell_t *cell)
{
    uint32_t col = 0;
    uint32_t row = 0;
    uint32_t row_sel = 0;
    uint32_t next_row = 0;
    uint64_t idx = 0;
    uint64_t pos = 0;
    uint64_t stride = emit_diffuse_stride_get(emit);

    for (pos = 0; pos < stride * LDG_TERM_EMIT_DIFFUSE_ROW_CUNT; pos++) { emit->diffuse[pos] = 0; }

    for (row = 0; row < emit->row_cunt; row++)
    {
        row_sel = row % LDG_TERM_EMIT_DIFFUSE_ROW_CUNT;
        next_row = (row_sel + LDG_TERM_EMIT_STEP) % LDG_TERM_EMIT_DIFFUSE_ROW_CUNT;

        for (pos = 0; pos < stride; pos++) { emit->diffuse[(next_row * stride) + pos] = 0; }

        for (col = 0; col < emit->col_cunt; col++)
        {
            idx = ((uint64_t)row * emit->col_cunt) + col;
            emit->quant[idx].ch = cell[idx].ch;
            emit->quant[idx].attr = cell[idx].attr;
            emit->quant[idx].link = cell[idx].link;
            emit_plane_quantize(emit, row_sel, col, LDG_TERM_EMIT_PLANE_FG, cell[idx].fg, emit->quant[idx].fg);

            if (emit->fg_only == LDG_TRUTH_TRUE) { emit_rgb_cpy(emit->quant[idx].bg, cell[idx].bg); }
            else { emit_plane_quantize(emit, row_sel, col, LDG_TERM_EMIT_PLANE_BG, cell[idx].bg, emit->quant[idx].bg); }
        }
    }
}

static uint32_t emit_cell_arr_put(ldg_term_emit_t *emit, emit_sink_t *sink, const ldg_term_cell_t *cell)
{
    uint32_t col = 0;
    uint32_t row = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (emit->color_mode == LDG_TERM_EMIT_COLOR_PALETTE)
    {
        emit_cell_arr_quantize(emit, cell);
        cell = emit->quant;
    }

    ret = emit_reserve(sink, LDG_TERM_EMIT_SEQ_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (emit->sync == LDG_TRUTH_TRUE) { emit_private_mode_st(sink, LDG_TERM_EMIT_MODE_SYNC, LDG_TRUTH_TRUE); }

    if (emit->full_redraw == LDG_TRUTH_TRUE)
    {
        emit_sgr_rst_st(sink);
        emit->color_known = LDG_TRUTH_FALSE;
        emit->cursor_col = LDG_TERM_EMIT_CURSOR_UNKNOWN;
        emit->cursor_row = LDG_TERM_EMIT_CURSOR_UNKNOWN;
        emit->full_redraw = LDG_TRUTH_FALSE;
    }

    for (row = 0; row < emit->row_cunt; row++)
    {
        for (col = 0; col < emit->col_cunt; col++)
        {
            idx = ((uint64_t)row * emit->col_cunt) + col;

            if (emit_cell_eq_is(&emit->prev[idx], &cell[idx]) == LDG_TRUTH_TRUE) { continue; }

            ret = emit_reserve(sink, LDG_TERM_EMIT_CELL_ENCODED_MAX);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (emit->cursor_col != col || emit->cursor_row != row) { emit_cup_st(sink, col, row); }

            emit_sgr_st(emit, sink, &cell[idx]);
            emit_byte_st(sink, cell[idx].ch);

            emit->prev[idx] = cell[idx];
            emit->cursor_row = row;
            emit->cursor_col = col + LDG_TERM_EMIT_STEP;

            if (emit->cursor_col >= emit->col_cunt)
            {
                emit->cursor_col = LDG_TERM_EMIT_CURSOR_UNKNOWN;
                emit->cursor_row = LDG_TERM_EMIT_CURSOR_UNKNOWN;
            }
        }
    }

    if (emit->sync == LDG_TRUTH_TRUE)
    {
        ret = emit_reserve(sink, LDG_TERM_EMIT_SEQ_MAX);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        emit_private_mode_st(sink, LDG_TERM_EMIT_MODE_SYNC, LDG_TRUTH_FALSE);
    }

    return LDG_ERR_AOK;
}

static uint32_t emit_screen_enter_put(emit_sink_t *sink)
{
    uint32_t ret = 0;

    ret = emit_reserve(sink, LDG_TERM_EMIT_CELL_ENCODED_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    emit_private_mode_st(sink, LDG_TERM_EMIT_MODE_ALT_SCREEN, LDG_TRUTH_TRUE);
    emit_private_mode_st(sink, LDG_TERM_EMIT_MODE_CURSOR, LDG_TRUTH_FALSE);
    emit_sgr_rst_st(sink);
    emit_erase_st(sink);

    return LDG_ERR_AOK;
}

static uint32_t emit_screen_exit_put(emit_sink_t *sink)
{
    uint32_t ret = 0;

    ret = emit_reserve(sink, LDG_TERM_EMIT_CELL_ENCODED_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    emit_sgr_rst_st(sink);
    emit_private_mode_st(sink, LDG_TERM_EMIT_MODE_CURSOR, LDG_TRUTH_TRUE);
    emit_private_mode_st(sink, LDG_TERM_EMIT_MODE_ALT_SCREEN, LDG_TRUTH_FALSE);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_emit_conf_default_get(ldg_term_emit_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->col_cunt = LDG_TERM_COL_DEFAULT;
    out->row_cunt = LDG_TERM_ROW_DEFAULT;
    out->buff_cap = LDG_TERM_EMIT_BUFF_CAP_DEFAULT;
    out->color_mode = LDG_TERM_EMIT_COLOR_TRUE;
    out->sync = LDG_TRUTH_TRUE;
    out->fg_only = LDG_TRUTH_FALSE;
    out->pudding[0] = 0;

    return LDG_ERR_AOK;
}

static uint8_t emit_conf_valid_is(const ldg_term_emit_conf_t *conf)
{
    if (conf->col_cunt == 0 || conf->col_cunt > LDG_TERM_COL_MAX) { return LDG_TRUTH_FALSE; }

    if (conf->row_cunt == 0 || conf->row_cunt > LDG_TERM_ROW_MAX) { return LDG_TRUTH_FALSE; }

    if (conf->buff_cap < LDG_TERM_EMIT_CELL_ENCODED_MAX) { return LDG_TRUTH_FALSE; }

    if (conf->color_mode != LDG_TERM_EMIT_COLOR_TRUE && conf->color_mode != LDG_TERM_EMIT_COLOR_PALETTE) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_term_emit_pool_size_get(const ldg_term_emit_conf_t *conf, uint64_t *out)
{
    uint64_t total = 0;

    if (LDG_UNLIKELY(!conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(emit_conf_valid_is(conf) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    total = (uint64_t)sizeof(ldg_term_emit_t);
    total += (uint64_t)conf->col_cunt * (uint64_t)conf->row_cunt * (uint64_t)sizeof(ldg_term_cell_t);
    total += (uint64_t)conf->buff_cap;
    total += (uint64_t)LDG_TERM_EMIT_PALETTE_CUNT * LDG_TERM_EMIT_RGB_CUNT;
    total += (uint64_t)LDG_TERM_EMIT_ALLOC_CUNT * (uint64_t)LDG_MEM_POOL_VAR_ALIGN;

    if (conf->color_mode == LDG_TERM_EMIT_COLOR_PALETTE)
    {
        total += (uint64_t)conf->col_cunt * (uint64_t)conf->row_cunt * (uint64_t)sizeof(ldg_term_cell_t);
        total += ((uint64_t)conf->col_cunt + LDG_TERM_EMIT_DIFFUSE_GUARD) * LDG_TERM_EMIT_PLANE_CUNT * LDG_TERM_EMIT_RGB_CUNT * LDG_TERM_EMIT_DIFFUSE_ROW_CUNT * (uint64_t)sizeof(int32_t);
        total += (uint64_t)LDG_TERM_EMIT_PLANE_CUNT * (uint64_t)LDG_MEM_POOL_VAR_ALIGN;
    }

    *out = total;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_emit_create(ldg_mem_pool_t *pool, const ldg_term_emit_conf_t *conf, ldg_term_emit_t **out)
{
    ldg_term_emit_t *emit = 0x0;
    void *tmp = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(emit_conf_valid_is(conf) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)sizeof(ldg_term_emit_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    emit = (ldg_term_emit_t *)tmp;
    emit->pool = pool;
    emit->col_cunt = conf->col_cunt;
    emit->row_cunt = conf->row_cunt;
    emit->buff_cap = conf->buff_cap;
    emit->color_mode = conf->color_mode;
    emit->sync = (conf->sync != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    emit->fg_only = (conf->fg_only != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)conf->col_cunt * (uint64_t)conf->row_cunt * (uint64_t)sizeof(ldg_term_cell_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    emit->prev = (ldg_term_cell_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)conf->buff_cap, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    emit->buff = (uint8_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)LDG_TERM_EMIT_PALETTE_CUNT * LDG_TERM_EMIT_RGB_CUNT, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    emit->palette = (uint8_t *)tmp;
    emit->quant = 0x0;
    emit->diffuse = 0x0;

    if (conf->color_mode == LDG_TERM_EMIT_COLOR_PALETTE)
    {
        ret = ldg_mem_pool_alloc(pool, (uint64_t)conf->col_cunt * (uint64_t)conf->row_cunt * (uint64_t)sizeof(ldg_term_cell_t), &tmp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        emit->quant = (ldg_term_cell_t *)tmp;

        ret = ldg_mem_pool_alloc(pool, ((uint64_t)conf->col_cunt + LDG_TERM_EMIT_DIFFUSE_GUARD) * LDG_TERM_EMIT_PLANE_CUNT * LDG_TERM_EMIT_RGB_CUNT * LDG_TERM_EMIT_DIFFUSE_ROW_CUNT * (uint64_t)sizeof(int32_t), &tmp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        emit->diffuse = (int32_t *)tmp;
    }

    emit_palette_build(emit->palette);

    ret = ldg_term_emit_rst(emit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = emit;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_emit_destroy(ldg_term_emit_t **emit)
{
    if (LDG_UNLIKELY(!emit || !*emit)) { return LDG_ERR_FUNC_ARG_NULL; }

    *emit = 0x0;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_emit_rst(ldg_term_emit_t *emit)
{
    uint64_t cell_cunt = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!emit)) { return LDG_ERR_FUNC_ARG_NULL; }

    cell_cunt = (uint64_t)emit->col_cunt * (uint64_t)emit->row_cunt;

    for (idx = 0; idx < cell_cunt; idx++)
    {
        emit->prev[idx].ch = LDG_TERM_EMIT_PREV_CH_NONE;
        emit->prev[idx].attr = LDG_TERM_ATTR_NONE;
        emit->prev[idx].link = LDG_TERM_LINK_NONE;
        emit->prev[idx].fg[LDG_TERM_EMIT_RED_IDX] = 0;
        emit->prev[idx].fg[LDG_TERM_EMIT_GREEN_IDX] = 0;
        emit->prev[idx].fg[LDG_TERM_EMIT_BLUE_IDX] = 0;
        emit->prev[idx].bg[LDG_TERM_EMIT_RED_IDX] = 0;
        emit->prev[idx].bg[LDG_TERM_EMIT_GREEN_IDX] = 0;
        emit->prev[idx].bg[LDG_TERM_EMIT_BLUE_IDX] = 0;
    }

    emit->cursor_col = LDG_TERM_EMIT_CURSOR_UNKNOWN;
    emit->cursor_row = LDG_TERM_EMIT_CURSOR_UNKNOWN;
    emit->color_known = LDG_TRUTH_FALSE;
    emit->full_redraw = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_emit_screen_enter_encode(ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    emit_sink_t sink = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;
    sink.buff = out;
    sink.cap = cap;
    sink.console = LDG_TRUTH_FALSE;

    ret = emit_screen_enter_put(&sink);
    *out_len = sink.len;

    return ret;
}

uint32_t ldg_term_emit_screen_exit_encode(ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    emit_sink_t sink = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;
    sink.buff = out;
    sink.cap = cap;
    sink.console = LDG_TRUTH_FALSE;

    ret = emit_screen_exit_put(&sink);
    *out_len = sink.len;

    return ret;
}

uint32_t ldg_term_emit_cell_arr_encode(ldg_term_emit_t *emit, const ldg_term_cell_t *cell, ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    emit_sink_t sink = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!emit || !cell || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;
    sink.buff = out;
    sink.cap = cap;
    sink.console = LDG_TRUTH_FALSE;

    ret = emit_cell_arr_put(emit, &sink, cell);
    *out_len = sink.len;

    return ret;
}

uint32_t ldg_term_emit_cell_arr_wr(ldg_term_emit_t *emit, const ldg_term_cell_t *cell)
{
    emit_sink_t sink = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!emit || !cell)) { return LDG_ERR_FUNC_ARG_NULL; }

    sink.buff = emit->buff;
    sink.cap = emit->buff_cap;
    sink.console = LDG_TRUTH_TRUE;

    ret = emit_cell_arr_put(emit, &sink, cell);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return emit_sink_flush(&sink);
}

uint32_t ldg_term_emit_screen_enter_wr(void)
{
    ldg_byte_t buff[LDG_TERM_EMIT_CELL_ENCODED_MAX] = LDG_ARR_ZERO_INIT;
    emit_sink_t sink = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    sink.buff = buff;
    sink.cap = (uint64_t)sizeof(buff);
    sink.console = LDG_TRUTH_TRUE;

    ret = emit_screen_enter_put(&sink);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return emit_sink_flush(&sink);
}

uint32_t ldg_term_emit_screen_exit_wr(void)
{
    ldg_byte_t buff[LDG_TERM_EMIT_CELL_ENCODED_MAX] = LDG_ARR_ZERO_INIT;
    emit_sink_t sink = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    sink.buff = buff;
    sink.cap = (uint64_t)sizeof(buff);
    sink.console = LDG_TRUTH_TRUE;

    ret = emit_screen_exit_put(&sink);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return emit_sink_flush(&sink);
}
