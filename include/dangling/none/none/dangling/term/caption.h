#ifndef LDG_TERM_CAPTION_H
#define LDG_TERM_CAPTION_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/term/render.h>
#include <dangling/term/layout.h>

#define LDG_TERM_CAPTION_CUE_MAX 512U
#define LDG_TERM_CAPTION_TEXT_CAP 512U
#define LDG_TERM_CAPTION_SPKR_CAP 32U
#define LDG_TERM_CAPTION_ACTIVE_MAX 8U
#define LDG_TERM_CAPTION_LINE_CAP 16U
#define LDG_TERM_CAPTION_AXIS_CUNT 3U
#define LDG_TERM_CAPTION_ID_NONE 0U

#define LDG_TERM_CAPTION_MASK_SPEECH 0x1U
#define LDG_TERM_CAPTION_MASK_SOUND 0x2U
#define LDG_TERM_CAPTION_MASK_MUSIC 0x4U
#define LDG_TERM_CAPTION_MASK_ALL 0x7U

typedef enum ldg_term_caption_kind
{
    LDG_TERM_CAPTION_KIND_SPEECH = 0,
    LDG_TERM_CAPTION_KIND_SOUND,
    LDG_TERM_CAPTION_KIND_MUSIC,
    LDG_TERM_CAPTION_KIND_CUNT
} ldg_term_caption_kind_t;

typedef enum ldg_term_caption_anchor
{
    LDG_TERM_CAPTION_ANCHOR_BOTTOM = 0,
    LDG_TERM_CAPTION_ANCHOR_TOP
} ldg_term_caption_anchor_t;

typedef struct ldg_term_caption_style
{
    ldg_term_render_color_t text_color;
    ldg_term_render_color_t spkr_color;
    ldg_term_render_color_t sound_color;
    ldg_term_render_color_t music_color;
    uint32_t line_frac[LDG_TERM_CAPTION_KIND_CUNT];
    uint32_t line_shown[LDG_TERM_CAPTION_KIND_CUNT];
    uint32_t margin_frac;
    uint32_t fade;
    uint8_t anchor;
    uint8_t spkr_en;
    uint8_t bearing_en;
    uint8_t pudding[1];
} ldg_term_caption_style_t;

typedef struct ldg_term_caption_cue_desc
{
    const char *text;
    const char *spkr;
    uint64_t text_len;
    uint64_t spkr_len;
    uint64_t start;
    uint64_t end;
    ldg_term_render_fixed_t pos[LDG_TERM_CAPTION_AXIS_CUNT];
    ldg_term_render_color_t color;
    uint32_t kind;
    uint8_t pos_en;
    uint8_t color_en;
    uint8_t pudding[2];
} ldg_term_caption_cue_desc_t;

typedef struct ldg_term_caption ldg_term_caption_t;

LDG_EXPORT uint32_t ldg_term_caption_style_default_get(ldg_term_caption_style_t *out);
LDG_EXPORT uint32_t ldg_term_caption_create(ldg_term_caption_t **out);
LDG_EXPORT uint32_t ldg_term_caption_destroy(ldg_term_caption_t *caption);
LDG_EXPORT uint32_t ldg_term_caption_style_set(ldg_term_caption_t *caption, const ldg_term_caption_style_t *style);
LDG_EXPORT uint32_t ldg_term_caption_style_get(const ldg_term_caption_t *caption, ldg_term_caption_style_t *out);
LDG_EXPORT uint32_t ldg_term_caption_mask_set(ldg_term_caption_t *caption, uint32_t mask);
LDG_EXPORT uint32_t ldg_term_caption_clock_set(ldg_term_caption_t *caption, uint64_t time);
LDG_EXPORT uint32_t ldg_term_caption_listener_set(ldg_term_caption_t *caption, const ldg_term_render_fixed_t *pos, const ldg_term_render_fixed_t *fwd);
LDG_EXPORT uint32_t ldg_term_caption_listener_clear(ldg_term_caption_t *caption);
LDG_EXPORT uint32_t ldg_term_caption_cue_add(ldg_term_caption_t *caption, const ldg_term_caption_cue_desc_t *desc, uint64_t *out);
LDG_EXPORT uint32_t ldg_term_caption_cue_remove(ldg_term_caption_t *caption, uint64_t id);
LDG_EXPORT uint32_t ldg_term_caption_clear(ldg_term_caption_t *caption);
LDG_EXPORT uint32_t ldg_term_caption_cue_cunt_get(const ldg_term_caption_t *caption, uint32_t *out);
LDG_EXPORT uint32_t ldg_term_caption_active_cunt_get(const ldg_term_caption_t *caption, uint32_t *out);
LDG_EXPORT uint32_t ldg_term_caption_draw(const ldg_term_caption_t *caption, ldg_term_render_t *render, const ldg_term_rect_t *rect);

#endif
