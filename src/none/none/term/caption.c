#include <stdint.h>

#include <dangling/term/caption.h>
#include <dangling/term/render.h>
#include <dangling/term/layout.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/mem.h>
#include <dangling/str/str.h>

#define LDG_TERM_CAPTION_STEP 1
#define LDG_TERM_CAPTION_PAIR 2
#define LDG_TERM_CAPTION_POOL_ITEM_SIZE_VAR 0
#define LDG_TERM_CAPTION_POOL_ALLOC_CUNT 2
#define LDG_TERM_CAPTION_PER_MILLE 1000U
#define LDG_TERM_CAPTION_MARGIN_FRAC_MAX 500U
#define LDG_TERM_CAPTION_LINE_TOTAL_MAX (LDG_TERM_CAPTION_ACTIVE_MAX * LDG_TERM_CAPTION_LINE_CAP)
#define LDG_TERM_CAPTION_LINE_W_MIN 8U
#define LDG_TERM_CAPTION_ARROW_SPAN 2U
#define LDG_TERM_CAPTION_ARROW_NONE 0
#define LDG_TERM_CAPTION_ARROW_AHEAD '^'
#define LDG_TERM_CAPTION_ARROW_RHS '>'
#define LDG_TERM_CAPTION_ARROW_BEHIND 'v'
#define LDG_TERM_CAPTION_ARROW_LHS '<'
#define LDG_TERM_CAPTION_SPACE ' '
#define LDG_TERM_CAPTION_X_IDX 0
#define LDG_TERM_CAPTION_Z_IDX 2
#define LDG_TERM_CAPTION_DIR_SCALE 256
#define LDG_TERM_CAPTION_ID_SLOT_MASK 0xFFFFFFFFULL
#define LDG_TERM_CAPTION_ID_GEN_SHIFT 32
#define LDG_TERM_CAPTION_GEN_FIRST 1U
#define LDG_TERM_CAPTION_LINE_FRAC_SPEECH_DEFAULT 300U
#define LDG_TERM_CAPTION_LINE_FRAC_SOUND_DEFAULT 880U
#define LDG_TERM_CAPTION_LINE_FRAC_MUSIC_DEFAULT 880U
#define LDG_TERM_CAPTION_LINE_SHOWN_SPEECH_DEFAULT 4U
#define LDG_TERM_CAPTION_LINE_SHOWN_SOUND_DEFAULT 2U
#define LDG_TERM_CAPTION_LINE_SHOWN_MUSIC_DEFAULT 2U
#define LDG_TERM_CAPTION_MARGIN_FRAC_DEFAULT 60U
#define LDG_TERM_CAPTION_FADE_DEFAULT 180U
#define LDG_TERM_CAPTION_TEXT_RED 0xF0U
#define LDG_TERM_CAPTION_TEXT_GREEN 0xF0U
#define LDG_TERM_CAPTION_TEXT_BLUE 0xF0U
#define LDG_TERM_CAPTION_SPKR_RED 0xBFU
#define LDG_TERM_CAPTION_SPKR_GREEN 0xFFU
#define LDG_TERM_CAPTION_SPKR_BLUE 0x00U
#define LDG_TERM_CAPTION_SOUND_RED 0xC8U
#define LDG_TERM_CAPTION_SOUND_GREEN 0xD8U
#define LDG_TERM_CAPTION_SOUND_BLUE 0xB0U
#define LDG_TERM_CAPTION_MUSIC_RED 0xB0U
#define LDG_TERM_CAPTION_MUSIC_GREEN 0xC8U
#define LDG_TERM_CAPTION_MUSIC_BLUE 0xD8U

typedef struct caption_cue
{
    char text[LDG_TERM_CAPTION_TEXT_CAP];
    char spkr[LDG_TERM_CAPTION_SPKR_CAP];
    uint64_t start;
    uint64_t end;
    ldg_term_render_fixed_t pos[LDG_TERM_CAPTION_AXIS_CUNT];
    ldg_term_render_color_t color;
    uint32_t text_len;
    uint32_t spkr_len;
    uint32_t gen;
    uint32_t kind;
    uint8_t pos_en;
    uint8_t color_en;
    uint8_t used;
    uint8_t pudding[5];
} caption_cue_t;

struct ldg_term_caption
{
    ldg_mem_pool_t *pool;
    caption_cue_t *cue;
    uint64_t clock;
    ldg_term_caption_style_t style;
    ldg_term_render_fixed_t listener_pos[LDG_TERM_CAPTION_AXIS_CUNT];
    ldg_term_render_fixed_t listener_fwd[LDG_TERM_CAPTION_AXIS_CUNT];
    uint32_t active[LDG_TERM_CAPTION_ACTIVE_MAX];
    uint32_t fade[LDG_TERM_CAPTION_ACTIVE_MAX];
    uint32_t mask;
    uint32_t cue_cunt;
    uint32_t active_cunt;
    uint8_t arrow[LDG_TERM_CAPTION_ACTIVE_MAX];
    uint8_t listener_en;
    uint8_t pudding[7];
};

typedef struct caption_layout
{
    int64_t box_row[LDG_TERM_CAPTION_KIND_CUNT];
    uint32_t width[LDG_TERM_CAPTION_KIND_CUNT];
    uint32_t first[LDG_TERM_CAPTION_KIND_CUNT];
    uint32_t shown[LDG_TERM_CAPTION_KIND_CUNT];
    uint32_t line_cunt[LDG_TERM_CAPTION_ACTIVE_MAX];
    uint32_t margin_col;
    uint32_t margin_row;
    uint8_t pudding[4];
} caption_layout_t;

static uint32_t caption_kind_mask_get(uint32_t kind)
{
    if (kind == LDG_TERM_CAPTION_KIND_SOUND) { return LDG_TERM_CAPTION_MASK_SOUND; }

    if (kind == LDG_TERM_CAPTION_KIND_MUSIC) { return LDG_TERM_CAPTION_MASK_MUSIC; }

    return LDG_TERM_CAPTION_MASK_SPEECH;
}

static ldg_term_render_color_t caption_kind_color_get(const ldg_term_caption_style_t *style, uint32_t kind)
{
    if (kind == LDG_TERM_CAPTION_KIND_SOUND) { return style->sound_color; }

    if (kind == LDG_TERM_CAPTION_KIND_MUSIC) { return style->music_color; }

    return style->text_color;
}

static uint32_t caption_fade_get(const ldg_term_caption_t *caption, const caption_cue_t *cue)
{
    uint64_t elapsed = caption->clock - cue->start;
    uint64_t remain = cue->end - caption->clock;
    uint64_t fade = LDG_TERM_CAPTION_PER_MILLE;
    uint64_t ramp = 0;

    if (caption->style.fade == 0) { return LDG_TERM_CAPTION_PER_MILLE; }

    if (elapsed < caption->style.fade) { fade = (elapsed * LDG_TERM_CAPTION_PER_MILLE) / caption->style.fade; }

    if (remain < caption->style.fade)
    {
        ramp = (remain * LDG_TERM_CAPTION_PER_MILLE) / caption->style.fade;
        if (ramp < fade) { fade = ramp; }
    }

    return (uint32_t)fade;
}

static int64_t caption_abs_get(int64_t val)
{
    return (val < 0) ? -val : val;
}

static uint8_t caption_arrow_get(const ldg_term_caption_t *caption, const caption_cue_t *cue)
{
    int64_t to_x = 0;
    int64_t to_z = 0;
    int64_t fwd_x = 0;
    int64_t fwd_z = 0;
    int64_t dot = 0;
    int64_t cross = 0;

    if (caption->listener_en != LDG_TRUTH_TRUE || cue->pos_en != LDG_TRUTH_TRUE) { return LDG_TERM_CAPTION_ARROW_NONE; }

    to_x = ((int64_t)cue->pos[LDG_TERM_CAPTION_X_IDX] - (int64_t)caption->listener_pos[LDG_TERM_CAPTION_X_IDX]) / LDG_TERM_CAPTION_DIR_SCALE;
    to_z = ((int64_t)cue->pos[LDG_TERM_CAPTION_Z_IDX] - (int64_t)caption->listener_pos[LDG_TERM_CAPTION_Z_IDX]) / LDG_TERM_CAPTION_DIR_SCALE;
    fwd_x = (int64_t)caption->listener_fwd[LDG_TERM_CAPTION_X_IDX] / LDG_TERM_CAPTION_DIR_SCALE;
    fwd_z = (int64_t)caption->listener_fwd[LDG_TERM_CAPTION_Z_IDX] / LDG_TERM_CAPTION_DIR_SCALE;

    if ((to_x == 0 && to_z == 0) || (fwd_x == 0 && fwd_z == 0)) { return LDG_TERM_CAPTION_ARROW_NONE; }

    dot = (fwd_x * to_x) + (fwd_z * to_z);
    cross = (fwd_x * to_z) - (fwd_z * to_x);

    if (caption_abs_get(cross) <= caption_abs_get(dot)) { return (dot > 0) ? (uint8_t)LDG_TERM_CAPTION_ARROW_AHEAD : (uint8_t)LDG_TERM_CAPTION_ARROW_BEHIND; }

    return (cross > 0) ? (uint8_t)LDG_TERM_CAPTION_ARROW_RHS : (uint8_t)LDG_TERM_CAPTION_ARROW_LHS;
}

static void caption_resolve(ldg_term_caption_t *caption)
{
    const caption_cue_t *cue = 0x0;
    uint32_t slot = 0;
    uint32_t idx = 0;
    uint32_t shift = 0;

    caption->active_cunt = 0;

    for (slot = 0; slot < LDG_TERM_CAPTION_CUE_MAX; slot++)
    {
        cue = &caption->cue[slot];

        if (cue->used != LDG_TRUTH_TRUE) { continue; }

        if ((caption->mask & caption_kind_mask_get(cue->kind)) == 0) { continue; }

        if (caption->clock < cue->start || caption->clock > cue->end) { continue; }

        idx = caption->active_cunt;

        while (idx > 0 && caption->cue[caption->active[idx - LDG_TERM_CAPTION_STEP]].start > cue->start) { idx--; }

        if (caption->active_cunt >= LDG_TERM_CAPTION_ACTIVE_MAX)
        {
            if (idx == 0) { continue; }

            for (shift = 0; shift + LDG_TERM_CAPTION_STEP < idx; shift++) { caption->active[shift] = caption->active[shift + LDG_TERM_CAPTION_STEP]; }

            caption->active[idx - LDG_TERM_CAPTION_STEP] = slot;
            continue;
        }

        for (shift = caption->active_cunt; shift > idx; shift--) { caption->active[shift] = caption->active[shift - LDG_TERM_CAPTION_STEP]; }

        caption->active[idx] = slot;
        caption->active_cunt++;
    }

    for (idx = 0; idx < caption->active_cunt; idx++)
    {
        cue = &caption->cue[caption->active[idx]];
        caption->fade[idx] = caption_fade_get(caption, cue);
        caption->arrow[idx] = caption_arrow_get(caption, cue);
    }
}

static uint8_t caption_line_next(const char *text, uint32_t len, uint32_t width, uint32_t *start, uint32_t *out_off, uint32_t *out_len)
{
    uint32_t brk = 0;

    while (*start < len && text[*start] == LDG_TERM_CAPTION_SPACE) { (*start)++; }

    if (*start >= len || width == 0) { return LDG_TRUTH_FALSE; }

    brk = len - *start;

    if (brk > width)
    {
        brk = width;

        while (brk > 0 && text[*start + brk] != LDG_TERM_CAPTION_SPACE) { brk--; }

        if (brk == 0) { brk = width; }
    }

    *out_off = *start;
    *out_len = brk;
    *start += brk;

    return LDG_TRUTH_TRUE;
}

static uint32_t caption_text_line_cunt_get(const char *text, uint32_t len, uint32_t width)
{
    uint32_t start = 0;
    uint32_t off = 0;
    uint32_t line_len = 0;
    uint32_t cunt = 0;

    while (cunt < LDG_TERM_CAPTION_LINE_CAP && caption_line_next(text, len, width, &start, &off, &line_len) == LDG_TRUTH_TRUE) { cunt++; }

    return cunt;
}

static uint32_t caption_cue_line_cunt_get(const ldg_term_caption_t *caption, const caption_cue_t *cue, uint32_t width)
{
    uint32_t cunt = 0;

    if (caption->style.spkr_en == LDG_TRUTH_TRUE && cue->spkr_len > 0) { cunt = caption_text_line_cunt_get(cue->spkr, cue->spkr_len, width); }

    cunt += caption_text_line_cunt_get(cue->text, cue->text_len, width);

    if (cunt > LDG_TERM_CAPTION_LINE_CAP) { cunt = LDG_TERM_CAPTION_LINE_CAP; }

    return cunt;
}

static ldg_term_render_color_t caption_color_fade_get(ldg_term_render_color_t color, uint32_t fade)
{
    color.red = (uint8_t)(((uint32_t)color.red * fade) / LDG_TERM_CAPTION_PER_MILLE);
    color.green = (uint8_t)(((uint32_t)color.green * fade) / LDG_TERM_CAPTION_PER_MILLE);
    color.blue = (uint8_t)(((uint32_t)color.blue * fade) / LDG_TERM_CAPTION_PER_MILLE);

    return color;
}

static uint32_t caption_width_get(const ldg_term_caption_t *caption, const ldg_term_rect_t *rect, uint32_t margin_col, uint32_t kind)
{
    uint32_t safe = rect->col_cunt;
    uint32_t width = 0;

    if (rect->col_cunt > margin_col * LDG_TERM_CAPTION_PAIR) { safe = rect->col_cunt - (margin_col * LDG_TERM_CAPTION_PAIR); }

    width = (uint32_t)(((uint64_t)rect->col_cunt * caption->style.line_frac[kind]) / LDG_TERM_CAPTION_PER_MILLE);

    if (width > safe) { width = safe; }

    if (width < LDG_TERM_CAPTION_LINE_W_MIN) { width = LDG_TERM_CAPTION_LINE_W_MIN; }

    if (kind != LDG_TERM_CAPTION_KIND_SPEECH && caption->style.bearing_en == LDG_TRUTH_TRUE && width > LDG_TERM_CAPTION_ARROW_SPAN) { width -= LDG_TERM_CAPTION_ARROW_SPAN; }

    if (width > rect->col_cunt) { width = rect->col_cunt; }

    return width;
}

static void caption_window_get(const ldg_term_caption_t *caption, const caption_layout_t *layout, uint32_t kind, uint32_t total, uint32_t *out_first)
{
    uint32_t span = total;
    uint32_t first = 0;
    uint32_t mark = 0;
    uint32_t pick = UINT32_MAX;
    uint32_t prior = 0;
    uint32_t idx = 0;

    if (span > caption->style.line_shown[kind]) { span = caption->style.line_shown[kind]; }

    first = total - span;

    for (idx = 0; idx < caption->active_cunt; idx++)
    {
        if (caption->cue[caption->active[idx]].kind != kind) { continue; }

        if (mark >= first && mark < total && pick == UINT32_MAX) { pick = mark; }

        if (mark <= first) { prior = mark; }

        mark += layout->line_cunt[idx];
    }

    *out_first = (pick == UINT32_MAX) ? prior : pick;
}

static void caption_layout_build(const ldg_term_caption_t *caption, const ldg_term_rect_t *rect, caption_layout_t *out)
{
    int64_t cursor = 0;
    uint32_t total = 0;
    uint32_t kind = 0;
    uint32_t idx = 0;

    out->margin_col = (uint32_t)(((uint64_t)rect->col_cunt * caption->style.margin_frac) / LDG_TERM_CAPTION_PER_MILLE);
    out->margin_row = (uint32_t)(((uint64_t)rect->row_cunt * caption->style.margin_frac) / LDG_TERM_CAPTION_PER_MILLE);

    for (kind = 0; kind < LDG_TERM_CAPTION_KIND_CUNT; kind++)
    {
        out->width[kind] = caption_width_get(caption, rect, out->margin_col, kind);
        total = 0;

        for (idx = 0; idx < caption->active_cunt; idx++)
        {
            if (caption->cue[caption->active[idx]].kind != kind) { continue; }

            out->line_cunt[idx] = caption_cue_line_cunt_get(caption, &caption->cue[caption->active[idx]], out->width[kind]);
            total += out->line_cunt[idx];
        }

        caption_window_get(caption, out, kind, total, &out->first[kind]);
        out->shown[kind] = total - out->first[kind];
    }

    cursor = (caption->style.anchor == LDG_TERM_CAPTION_ANCHOR_TOP) ? (int64_t)out->margin_row : (int64_t)rect->row_cunt - (int64_t)out->margin_row;

    for (kind = 0; kind < LDG_TERM_CAPTION_KIND_CUNT; kind++)
    {
        if (caption->style.anchor == LDG_TERM_CAPTION_ANCHOR_TOP)
        {
            out->box_row[kind] = cursor;
            cursor += out->shown[kind];
        }
        else
        {
            out->box_row[kind] = cursor - (int64_t)out->shown[kind];
            cursor = out->box_row[kind];
        }

        if (out->box_row[kind] < 0) { out->box_row[kind] = 0; }
    }
}

static uint32_t caption_line_draw(ldg_term_render_t *render, const ldg_term_rect_t *rect, int64_t row, uint32_t col, const char *text, uint32_t len, const ldg_term_render_color_t *fg)
{
    ldg_term_render_color_t bg = LDG_STRUCT_ZERO_INIT;

    if (row < 0 || row >= (int64_t)rect->row_cunt || col >= rect->col_cunt) { return LDG_ERR_AOK; }

    if (len > rect->col_cunt - col) { len = rect->col_cunt - col; }

    return ldg_term_render_text_draw(render, rect->col + col, rect->row + (uint32_t)row, text, len, fg, &bg);
}

static uint32_t caption_cue_draw(const ldg_term_caption_t *caption, ldg_term_render_t *render, const ldg_term_rect_t *rect, const caption_layout_t *layout, uint32_t idx, uint32_t base)
{
    const caption_cue_t *cue = &caption->cue[caption->active[idx]];
    ldg_term_render_color_t text_color = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_color_t spkr_color = LDG_STRUCT_ZERO_INIT;
    const char *src = 0x0;
    uint32_t kind = cue->kind;
    uint32_t spkr_start = 0;
    uint32_t text_start = 0;
    uint32_t off = 0;
    uint32_t len = 0;
    uint32_t line = 0;
    uint32_t global = 0;
    uint32_t col = 0;
    uint32_t ret = 0;
    uint8_t spkr_line = LDG_TRUTH_FALSE;
    char arrow = LDG_TERM_CAPTION_ARROW_NONE;

    text_color = caption_color_fade_get((cue->color_en == LDG_TRUTH_TRUE) ? cue->color : caption_kind_color_get(&caption->style, kind), caption->fade[idx]);
    spkr_color = caption_color_fade_get(caption->style.spkr_color, caption->fade[idx]);

    for (line = 0; line < layout->line_cunt[idx]; line++)
    {
        spkr_line = LDG_TRUTH_FALSE;
        src = cue->text;

        if (caption->style.spkr_en == LDG_TRUTH_TRUE && caption_line_next(cue->spkr, cue->spkr_len, layout->width[kind], &spkr_start, &off, &len) == LDG_TRUTH_TRUE)
        {
            spkr_line = LDG_TRUTH_TRUE;
            src = cue->spkr;
        }
        else if (caption_line_next(cue->text, cue->text_len, layout->width[kind], &text_start, &off, &len) != LDG_TRUTH_TRUE) { break; }

        global = base + line;

        if (global < layout->first[kind] || global >= layout->first[kind] + layout->shown[kind]) { continue; }

        col = (kind == LDG_TERM_CAPTION_KIND_SPEECH) ? (rect->col_cunt - len) / LDG_TERM_CAPTION_PAIR : layout->margin_col;

        if (kind != LDG_TERM_CAPTION_KIND_SPEECH && line == 0 && caption->style.bearing_en == LDG_TRUTH_TRUE && caption->arrow[idx] != LDG_TERM_CAPTION_ARROW_NONE)
        {
            arrow = (char)caption->arrow[idx];

            ret = caption_line_draw(render, rect, layout->box_row[kind] + (int64_t)(global - layout->first[kind]), col, &arrow, LDG_TERM_CAPTION_STEP, &text_color);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            col += LDG_TERM_CAPTION_ARROW_SPAN;
        }

        ret = caption_line_draw(render, rect, layout->box_row[kind] + (int64_t)(global - layout->first[kind]), col, src + off, len, (spkr_line == LDG_TRUTH_TRUE) ? &spkr_color : &text_color);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_style_default_get(ldg_term_caption_style_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, (uint64_t)sizeof(*out));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->text_color.red = LDG_TERM_CAPTION_TEXT_RED;
    out->text_color.green = LDG_TERM_CAPTION_TEXT_GREEN;
    out->text_color.blue = LDG_TERM_CAPTION_TEXT_BLUE;
    out->spkr_color.red = LDG_TERM_CAPTION_SPKR_RED;
    out->spkr_color.green = LDG_TERM_CAPTION_SPKR_GREEN;
    out->spkr_color.blue = LDG_TERM_CAPTION_SPKR_BLUE;
    out->sound_color.red = LDG_TERM_CAPTION_SOUND_RED;
    out->sound_color.green = LDG_TERM_CAPTION_SOUND_GREEN;
    out->sound_color.blue = LDG_TERM_CAPTION_SOUND_BLUE;
    out->music_color.red = LDG_TERM_CAPTION_MUSIC_RED;
    out->music_color.green = LDG_TERM_CAPTION_MUSIC_GREEN;
    out->music_color.blue = LDG_TERM_CAPTION_MUSIC_BLUE;
    out->line_frac[LDG_TERM_CAPTION_KIND_SPEECH] = LDG_TERM_CAPTION_LINE_FRAC_SPEECH_DEFAULT;
    out->line_frac[LDG_TERM_CAPTION_KIND_SOUND] = LDG_TERM_CAPTION_LINE_FRAC_SOUND_DEFAULT;
    out->line_frac[LDG_TERM_CAPTION_KIND_MUSIC] = LDG_TERM_CAPTION_LINE_FRAC_MUSIC_DEFAULT;
    out->line_shown[LDG_TERM_CAPTION_KIND_SPEECH] = LDG_TERM_CAPTION_LINE_SHOWN_SPEECH_DEFAULT;
    out->line_shown[LDG_TERM_CAPTION_KIND_SOUND] = LDG_TERM_CAPTION_LINE_SHOWN_SOUND_DEFAULT;
    out->line_shown[LDG_TERM_CAPTION_KIND_MUSIC] = LDG_TERM_CAPTION_LINE_SHOWN_MUSIC_DEFAULT;
    out->margin_frac = LDG_TERM_CAPTION_MARGIN_FRAC_DEFAULT;
    out->fade = LDG_TERM_CAPTION_FADE_DEFAULT;
    out->anchor = LDG_TERM_CAPTION_ANCHOR_BOTTOM;
    out->spkr_en = LDG_TRUTH_TRUE;
    out->bearing_en = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_create(ldg_term_caption_t **out)
{
    ldg_mem_pool_t *pool = 0x0;
    ldg_term_caption_t *caption = 0x0;
    void *mem = 0x0;
    uint64_t cue_size = 0;
    uint64_t pool_cap = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    cue_size = (uint64_t)LDG_TERM_CAPTION_CUE_MAX * (uint64_t)sizeof(caption_cue_t);
    pool_cap = (uint64_t)sizeof(struct ldg_term_caption) + cue_size + ((uint64_t)LDG_TERM_CAPTION_POOL_ALLOC_CUNT * LDG_MEM_POOL_VAR_ALIGN);

    ret = ldg_mem_pool_create(LDG_TERM_CAPTION_POOL_ITEM_SIZE_VAR, pool_cap, &pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)sizeof(struct ldg_term_caption), &mem);
    if (ret == LDG_ERR_AOK) { ret = ldg_mem_zero(mem, (uint64_t)sizeof(struct ldg_term_caption)); }

    caption = (ldg_term_caption_t *)mem;

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_pool_alloc(pool, cue_size, &mem); }

    if (ret == LDG_ERR_AOK) { ret = ldg_mem_zero(mem, cue_size); }

    if (ret == LDG_ERR_AOK) { ret = ldg_term_caption_style_default_get(&caption->style); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK)) { return LDG_ERR_MEM_POOL_INVALID; }

        return ret;
    }

    caption->pool = pool;
    caption->cue = (caption_cue_t *)mem;
    caption->mask = LDG_TERM_CAPTION_MASK_ALL;
    caption->listener_en = LDG_TRUTH_FALSE;

    *out = caption;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_destroy(ldg_term_caption_t *caption)
{
    ldg_mem_pool_t *pool = 0x0;

    if (LDG_UNLIKELY(!caption)) { return LDG_ERR_FUNC_ARG_NULL; }

    pool = caption->pool;

    return ldg_mem_pool_destroy(&pool);
}

uint32_t ldg_term_caption_style_set(ldg_term_caption_t *caption, const ldg_term_caption_style_t *style)
{
    uint32_t kind = 0;

    if (LDG_UNLIKELY(!caption || !style)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(style->margin_frac >= LDG_TERM_CAPTION_MARGIN_FRAC_MAX)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY(style->anchor != LDG_TERM_CAPTION_ANCHOR_BOTTOM && style->anchor != LDG_TERM_CAPTION_ANCHOR_TOP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (kind = 0; kind < LDG_TERM_CAPTION_KIND_CUNT; kind++)
    {
        if (LDG_UNLIKELY(style->line_shown[kind] == 0 || style->line_shown[kind] > LDG_TERM_CAPTION_LINE_TOTAL_MAX)) { return LDG_ERR_RANGE; }

        if (LDG_UNLIKELY(style->line_frac[kind] == 0 || style->line_frac[kind] > LDG_TERM_CAPTION_PER_MILLE)) { return LDG_ERR_RANGE; }
    }

    caption->style = *style;
    caption_resolve(caption);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_style_get(const ldg_term_caption_t *caption, ldg_term_caption_style_t *out)
{
    if (LDG_UNLIKELY(!caption || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = caption->style;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_mask_set(ldg_term_caption_t *caption, uint32_t mask)
{
    if (LDG_UNLIKELY(!caption)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY((mask & ~LDG_TERM_CAPTION_MASK_ALL) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    caption->mask = mask;
    caption_resolve(caption);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_clock_set(ldg_term_caption_t *caption, uint64_t time)
{
    if (LDG_UNLIKELY(!caption)) { return LDG_ERR_FUNC_ARG_NULL; }

    caption->clock = time;
    caption_resolve(caption);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_listener_set(ldg_term_caption_t *caption, const ldg_term_render_fixed_t *pos, const ldg_term_render_fixed_t *fwd)
{
    uint32_t axis = 0;

    if (LDG_UNLIKELY(!caption || !pos || !fwd)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (axis = 0; axis < LDG_TERM_CAPTION_AXIS_CUNT; axis++)
    {
        caption->listener_pos[axis] = pos[axis];
        caption->listener_fwd[axis] = fwd[axis];
    }

    caption->listener_en = LDG_TRUTH_TRUE;
    caption_resolve(caption);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_listener_clear(ldg_term_caption_t *caption)
{
    if (LDG_UNLIKELY(!caption)) { return LDG_ERR_FUNC_ARG_NULL; }

    caption->listener_en = LDG_TRUTH_FALSE;
    caption_resolve(caption);

    return LDG_ERR_AOK;
}

static uint32_t caption_slot_free_get(const ldg_term_caption_t *caption)
{
    uint32_t slot = 0;

    for (slot = 0; slot < LDG_TERM_CAPTION_CUE_MAX; slot++) { if (caption->cue[slot].used != LDG_TRUTH_TRUE) { return slot; } }

    return UINT32_MAX;
}

static uint32_t caption_str_cpy(char *dst, const char *src, uint64_t len)
{
    uint32_t ret = 0;

    if (len > 0)
    {
        ret = ldg_mem_cpy(dst, src, len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    dst[len] = LDG_STR_TERM;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_cue_add(ldg_term_caption_t *caption, const ldg_term_caption_cue_desc_t *desc, uint64_t *out)
{
    caption_cue_t *cue = 0x0;
    uint32_t slot = 0;
    uint32_t axis = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!caption || !desc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_TERM_CAPTION_ID_NONE;

    if (LDG_UNLIKELY((!desc->text && desc->text_len != 0) || (!desc->spkr && desc->spkr_len != 0))) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(desc->kind >= LDG_TERM_CAPTION_KIND_CUNT || desc->end < desc->start)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->text_len >= LDG_TERM_CAPTION_TEXT_CAP || desc->spkr_len >= LDG_TERM_CAPTION_SPKR_CAP)) { return LDG_ERR_STR_TRUNC; }

    slot = caption_slot_free_get(caption);
    if (LDG_UNLIKELY(slot == UINT32_MAX)) { return LDG_ERR_FULL; }

    cue = &caption->cue[slot];

    ret = caption_str_cpy(cue->text, desc->text, desc->text_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = caption_str_cpy(cue->spkr, desc->spkr, desc->spkr_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (axis = 0; axis < LDG_TERM_CAPTION_AXIS_CUNT; axis++) { cue->pos[axis] = desc->pos[axis]; }

    cue->text_len = (uint32_t)desc->text_len;
    cue->spkr_len = (uint32_t)desc->spkr_len;
    cue->start = desc->start;
    cue->end = desc->end;
    cue->color = desc->color;
    cue->kind = desc->kind;
    cue->pos_en = (desc->pos_en == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    cue->color_en = (desc->color_en == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    cue->gen++;

    if (cue->gen == 0) { cue->gen = LDG_TERM_CAPTION_GEN_FIRST; }

    cue->used = LDG_TRUTH_TRUE;
    caption->cue_cunt++;

    *out = ((uint64_t)cue->gen << LDG_TERM_CAPTION_ID_GEN_SHIFT) | (uint64_t)slot;
    caption_resolve(caption);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_cue_remove(ldg_term_caption_t *caption, uint64_t id)
{
    caption_cue_t *cue = 0x0;
    uint64_t slot = id & LDG_TERM_CAPTION_ID_SLOT_MASK;

    if (LDG_UNLIKELY(!caption)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(slot >= LDG_TERM_CAPTION_CUE_MAX)) { return LDG_ERR_NOT_FOUND; }

    cue = &caption->cue[slot];

    if (LDG_UNLIKELY(cue->used != LDG_TRUTH_TRUE || (uint64_t)cue->gen != (id >> LDG_TERM_CAPTION_ID_GEN_SHIFT))) { return LDG_ERR_NOT_FOUND; }

    cue->used = LDG_TRUTH_FALSE;
    caption->cue_cunt--;
    caption_resolve(caption);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_clear(ldg_term_caption_t *caption)
{
    uint32_t slot = 0;

    if (LDG_UNLIKELY(!caption)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (slot = 0; slot < LDG_TERM_CAPTION_CUE_MAX; slot++) { caption->cue[slot].used = LDG_TRUTH_FALSE; }

    caption->cue_cunt = 0;
    caption->active_cunt = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_cue_cunt_get(const ldg_term_caption_t *caption, uint32_t *out)
{
    if (LDG_UNLIKELY(!caption || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = caption->cue_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_active_cunt_get(const ldg_term_caption_t *caption, uint32_t *out)
{
    if (LDG_UNLIKELY(!caption || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = caption->active_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_caption_draw(const ldg_term_caption_t *caption, ldg_term_render_t *render, const ldg_term_rect_t *rect)
{
    caption_layout_t layout = LDG_STRUCT_ZERO_INIT;
    uint32_t base[LDG_TERM_CAPTION_KIND_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t kind = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!caption || !render || !rect)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (caption->active_cunt == 0 || rect->col_cunt == 0 || rect->row_cunt == 0) { return LDG_ERR_AOK; }

    caption_layout_build(caption, rect, &layout);

    for (idx = 0; idx < caption->active_cunt; idx++)
    {
        kind = caption->cue[caption->active[idx]].kind;

        ret = caption_cue_draw(caption, render, rect, &layout, idx, base[kind]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        base[kind] += layout.line_cunt[idx];
    }

    return LDG_ERR_AOK;
}
