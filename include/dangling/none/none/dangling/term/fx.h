#ifndef LDG_TERM_FX_H
#define LDG_TERM_FX_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/term/render.h>

#define LDG_TERM_FX_PER_MILLE 1000U
#define LDG_TERM_FX_COL_MAX_DEFAULT 160U
#define LDG_TERM_FX_ROW_MAX_DEFAULT 64U
#define LDG_TERM_FX_EXTENT_MAX 16384U
#define LDG_TERM_FX_GAIN_MAX 10000U
#define LDG_TERM_FX_RATE_MAX 100000U

typedef enum ldg_term_fx_kind
{
    LDG_TERM_FX_KIND_CRT = 0,
    LDG_TERM_FX_KIND_VGA,
    LDG_TERM_FX_KIND_CGA,
    LDG_TERM_FX_KIND_SCOPE,
    LDG_TERM_FX_KIND_CUNT
} ldg_term_fx_kind_t;

typedef struct ldg_term_fx_style
{
    ldg_term_render_color_t tint;
    ldg_term_render_color_t hot;
    ldg_term_render_color_t beam_color;
    uint32_t kind;
    uint32_t bright;
    uint32_t noise;
    uint32_t hw_noise;
    uint32_t phosphor;
    uint32_t bloom;
    uint32_t edge_fade;
    uint32_t curve;
    uint32_t overscan;
    uint32_t zoom;
    uint32_t scanline;
    uint32_t scanline_gap;
    uint32_t scanline_scroll;
    uint32_t line_cunt;
    uint32_t mask;
    uint32_t mask_pitch;
    uint32_t converge;
    uint32_t beam_rate;
    uint32_t beam_intensity;
    uint32_t beam_width;
    uint32_t warmth;
    uint32_t ripple;
    uint32_t mains_rate;
    uint32_t cell;
    uint32_t cell_cunt_x;
    uint32_t cell_cunt_y;
    uint32_t bleed;
    uint32_t sustain;
    uint32_t pedestal;
    uint32_t haze;
    uint32_t band;
    uint32_t addr_rate;
    uint32_t bloom_threshold;
    uint32_t bloom_gain;
    uint32_t bloom_radius;
    uint8_t beam_en;
    uint8_t mono;
    uint8_t pudding[2];
} ldg_term_fx_style_t;

typedef struct ldg_term_fx_mix
{
    uint32_t bright;
    uint32_t beam;
    uint32_t converge;
    uint32_t bloom;
    uint32_t haze;
    uint32_t noise;
    uint32_t warmth;
} ldg_term_fx_mix_t;

typedef struct ldg_term_fx_conf
{
    uint32_t col_max;
    uint32_t row_max;
    uint32_t sample_col_cunt;
    uint32_t sample_row_cunt;
    uint8_t persist_en;
    uint8_t pudding[3];
} ldg_term_fx_conf_t;

typedef struct ldg_term_fx ldg_term_fx_t;

LDG_EXPORT uint32_t ldg_term_fx_conf_default_get(ldg_term_fx_conf_t *out);
LDG_EXPORT uint32_t ldg_term_fx_style_default_get(uint32_t kind, ldg_term_fx_style_t *out);
LDG_EXPORT uint32_t ldg_term_fx_style_lerp(const ldg_term_fx_style_t *cold, const ldg_term_fx_style_t *peak, const ldg_term_fx_mix_t *mix, ldg_term_fx_style_t *out);
LDG_EXPORT uint32_t ldg_term_fx_create(const ldg_term_fx_conf_t *conf, ldg_term_fx_t **out);
LDG_EXPORT uint32_t ldg_term_fx_destroy(ldg_term_fx_t *fx);
LDG_EXPORT uint32_t ldg_term_fx_rst(ldg_term_fx_t *fx);
LDG_EXPORT uint32_t ldg_term_fx_apply(ldg_term_fx_t *fx, ldg_term_render_t *render, const ldg_term_fx_style_t *style, uint64_t time);

#endif
