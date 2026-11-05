#include <stdint.h>

#include <dangling/term/render.h>
#include <dangling/fmt/svg.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_TERM_SVG_PAIR 2
#define LDG_TERM_SVG_X_IDX 0
#define LDG_TERM_SVG_Y_IDX 1
#define LDG_TERM_SVG_FIXED_ONE (1LL << LDG_TERM_RENDER_FIXED_SHIFT)
#define LDG_TERM_SVG_AFFINE2_SCALE_X_IDX 0
#define LDG_TERM_SVG_AFFINE2_SCALE_Y_IDX 3
#define LDG_TERM_SVG_AFFINE2_OFF_X_IDX 4

static void svg_paint_convert(const ldg_fmt_svg_paint_t *src, uint32_t bright, ldg_term_render_paint_t *out)
{
    uint32_t idx = 0;

    out->kind = (src->kind == LDG_FMT_SVG_PAINT_RADIAL) ? LDG_TERM_RENDER_PAINT_RADIAL : LDG_TERM_RENDER_PAINT_SOLID;
    out->bright = bright;
    out->center[LDG_TERM_SVG_X_IDX] = src->center[LDG_TERM_SVG_X_IDX];
    out->center[LDG_TERM_SVG_Y_IDX] = src->center[LDG_TERM_SVG_Y_IDX];
    out->radius = src->radius;
    out->stop_cunt = src->stop_cunt;
    out->color.red = src->stop[0].red;
    out->color.green = src->stop[0].green;
    out->color.blue = src->stop[0].blue;
    out->color.pudding[0] = 0;

    for (idx = 0; idx < src->stop_cunt && idx < LDG_TERM_RENDER_GRAD_STOP_MAX; idx++)
    {
        out->stop[idx].off = src->stop[idx].off;
        out->stop[idx].color.red = src->stop[idx].red;
        out->stop[idx].color.green = src->stop[idx].green;
        out->stop[idx].color.blue = src->stop[idx].blue;
        out->stop[idx].color.pudding[0] = 0;
    }
}

static int64_t svg_fit_scale_get(const ldg_fmt_svg_t *svg, const ldg_term_render_fixed_t *extent)
{
    int64_t scale_x = 0;
    int64_t scale_y = 0;

    scale_x = ((int64_t)extent[LDG_TERM_SVG_X_IDX] * LDG_TERM_SVG_FIXED_ONE) / (int64_t)svg->view_extent[LDG_TERM_SVG_X_IDX];
    scale_y = ((int64_t)extent[LDG_TERM_SVG_Y_IDX] * LDG_TERM_SVG_FIXED_ONE) / (int64_t)svg->view_extent[LDG_TERM_SVG_Y_IDX];

    return (scale_x < scale_y) ? scale_x : scale_y;
}

static uint32_t svg_fit_affine_get(const ldg_fmt_svg_t *svg, const ldg_term_render_fixed_t *pos, const ldg_term_render_fixed_t *extent, ldg_term_render_affine2_t *out)
{
    int64_t scale = 0;
    int64_t fitted = 0;
    int64_t off = 0;
    uint32_t axis = 0;

    scale = svg_fit_scale_get(svg, extent);

    if (LDG_UNLIKELY(scale <= 0 || scale > INT32_MAX)) { return LDG_ERR_RANGE; }

    out->elem[LDG_TERM_SVG_AFFINE2_SCALE_X_IDX] = (ldg_term_render_fixed_t)scale;
    out->elem[LDG_TERM_SVG_AFFINE2_SCALE_Y_IDX] = (ldg_term_render_fixed_t)scale;

    for (axis = 0; axis < LDG_TERM_SVG_PAIR; axis++)
    {
        fitted = ((int64_t)svg->view_extent[axis] * scale) / LDG_TERM_SVG_FIXED_ONE;
        off = (int64_t)pos[axis] + (((int64_t)extent[axis] - fitted) / LDG_TERM_SVG_PAIR) - (((int64_t)svg->view_pos[axis] * scale) / LDG_TERM_SVG_FIXED_ONE);

        if (LDG_UNLIKELY(off > INT32_MAX || off < INT32_MIN)) { return LDG_ERR_RANGE; }

        out->elem[LDG_TERM_SVG_AFFINE2_OFF_X_IDX + axis] = (ldg_term_render_fixed_t)off;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_svg_draw(ldg_term_render_t *render, const ldg_fmt_svg_t *svg, const ldg_term_render_fixed_t *pos, const ldg_term_render_fixed_t *extent, uint32_t bright)
{
    ldg_term_render_affine2_t xf = LDG_STRUCT_ZERO_INIT;
    ldg_term_render_paint_t paint = LDG_STRUCT_ZERO_INIT;
    const ldg_fmt_svg_shape_t *shape = 0x0;
    uint32_t rule = LDG_TERM_RENDER_FILL_RULE_NONZERO;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!render || !svg || !pos || !extent)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(extent[LDG_TERM_SVG_X_IDX] <= 0 || extent[LDG_TERM_SVG_Y_IDX] <= 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(svg->view_extent[LDG_TERM_SVG_X_IDX] <= 0 || svg->view_extent[LDG_TERM_SVG_Y_IDX] <= 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = svg_fit_affine_get(svg, pos, extent, &xf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < svg->shape_cunt; idx++)
    {
        shape = &svg->shape[idx];

        if (shape->contour_cunt == 0 || shape->paint.stop_cunt == 0) { continue; }

        svg_paint_convert(&shape->paint, bright, &paint);
        rule = (shape->rule == LDG_FMT_SVG_FILL_RULE_EVEN_ODD) ? LDG_TERM_RENDER_FILL_RULE_EVEN_ODD : LDG_TERM_RENDER_FILL_RULE_NONZERO;

        ret = ldg_term_render_poly_fill(render, &svg->pt[(uint64_t)shape->pt_first * LDG_TERM_SVG_PAIR], &svg->contour_len[shape->contour_first], shape->contour_cunt, rule, &xf, &paint);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}
