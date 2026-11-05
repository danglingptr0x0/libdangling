#ifndef LDG_FMT_SVG_H
#define LDG_FMT_SVG_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

#define LDG_FMT_SVG_FIXED_SHIFT 16U
#define LDG_FMT_SVG_STOP_MAX 16U
#define LDG_FMT_SVG_PLANE_CUNT 2U

typedef enum ldg_fmt_svg_fill_rule
{
    LDG_FMT_SVG_FILL_RULE_NONZERO = 0,
    LDG_FMT_SVG_FILL_RULE_EVEN_ODD
} ldg_fmt_svg_fill_rule_t;

typedef enum ldg_fmt_svg_paint_kind
{
    LDG_FMT_SVG_PAINT_SOLID = 0,
    LDG_FMT_SVG_PAINT_RADIAL
} ldg_fmt_svg_paint_kind_t;

typedef struct ldg_fmt_svg_stop
{
    int32_t off;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t pudding[1];
} ldg_fmt_svg_stop_t;

typedef struct ldg_fmt_svg_paint
{
    int32_t center[LDG_FMT_SVG_PLANE_CUNT];
    int32_t radius;
    uint32_t kind;
    uint32_t stop_cunt;
    ldg_fmt_svg_stop_t stop[LDG_FMT_SVG_STOP_MAX];
} ldg_fmt_svg_paint_t;

typedef struct ldg_fmt_svg_shape
{
    ldg_fmt_svg_paint_t paint;
    uint32_t contour_first;
    uint32_t contour_cunt;
    uint32_t pt_first;
    uint32_t rule;
} ldg_fmt_svg_shape_t;

typedef struct ldg_fmt_svg
{
    int32_t view_pos[LDG_FMT_SVG_PLANE_CUNT];
    int32_t view_extent[LDG_FMT_SVG_PLANE_CUNT];
    ldg_fmt_svg_shape_t *shape;
    int32_t *pt;
    uint32_t *contour_len;
    uint32_t shape_cunt;
    uint32_t contour_cunt;
    uint32_t pt_cunt;
    uint8_t pudding[4];
} ldg_fmt_svg_t;

LDG_EXPORT uint32_t ldg_fmt_svg_pool_size_get(const uint8_t *src, uint64_t len, uint64_t *out);
LDG_EXPORT uint32_t ldg_fmt_svg_parse(ldg_mem_pool_t *pool, const uint8_t *src, uint64_t len, ldg_fmt_svg_t **out);

#endif
