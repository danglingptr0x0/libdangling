#ifndef LDG_TERM_RENDER_H
#define LDG_TERM_RENDER_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/term/term.h>
#include <dangling/term/layout.h>
#include <dangling/fmt/svg.h>

#define LDG_TERM_RENDER_POOL_MAX (128ULL * LDG_MIB)
#define LDG_TERM_RENDER_GLYPH_FIRST 0x20U
#define LDG_TERM_RENDER_GLYPH_CUNT 95U
#define LDG_TERM_RENDER_GLYPH_COL_CUNT 8U
#define LDG_TERM_RENDER_GLYPH_ROW_CUNT 16U
#define LDG_TERM_RENDER_SAMPLE_COL_DEFAULT 8U
#define LDG_TERM_RENDER_SAMPLE_ROW_DEFAULT 16U
#define LDG_TERM_RENDER_CELL_ASPECT_NUM_DEFAULT 1U
#define LDG_TERM_RENDER_CELL_ASPECT_DEN_DEFAULT 2U
#define LDG_TERM_RENDER_VERT_MAX_DEFAULT 4096U
#define LDG_TERM_RENDER_CANDIDATE_CUNT 8U
#define LDG_TERM_RENDER_RAMP_MAX 32U
#define LDG_TERM_RENDER_BIT_WORD_CUNT 2U
#define LDG_TERM_RENDER_PER_MILLE 1000U
#define LDG_TERM_RENDER_FIT_GAIN_MIN_DEFAULT 500U
#define LDG_TERM_RENDER_FIT_HOLD_DEFAULT 200U
#define LDG_TERM_RENDER_FIT_CONTRAST_MIN_DEFAULT 12U
#define LDG_TERM_RENDER_FIT_CONTRAST_MAX 255U
#define LDG_TERM_RENDER_AXIS_CUNT 3U
#define LDG_TERM_RENDER_HOMOGENEOUS_CUNT 4U
#define LDG_TERM_RENDER_PLANE_CUNT 2U

#define LDG_TERM_RENDER_FIXED_SHIFT 16U
#define LDG_TERM_RENDER_FIXED_ONE (1 << LDG_TERM_RENDER_FIXED_SHIFT)
#define LDG_TERM_RENDER_FIXED_HALF (1 << (LDG_TERM_RENDER_FIXED_SHIFT - 1U))
#define LDG_TERM_RENDER_ANGLE_QUARTER 0x40000000U
#define LDG_TERM_RENDER_ANGLE_HALF 0x80000000U
#define LDG_TERM_RENDER_DEPTH_FAR UINT32_MAX
#define LDG_TERM_RENDER_DEPTH_NEAR 0U
#define LDG_TERM_RENDER_CROSS_MAX_DEFAULT 1024U
#define LDG_TERM_RENDER_GRAD_STOP_MAX 16U
#define LDG_TERM_RENDER_AFFINE2_CUNT 6U

typedef int32_t ldg_term_render_fixed_t;
typedef uint32_t ldg_term_render_angle_t;

typedef enum ldg_term_render_fill_rule
{
    LDG_TERM_RENDER_FILL_RULE_NONZERO = 0,
    LDG_TERM_RENDER_FILL_RULE_EVEN_ODD
} ldg_term_render_fill_rule_t;

typedef enum ldg_term_render_paint_kind
{
    LDG_TERM_RENDER_PAINT_SOLID = 0,
    LDG_TERM_RENDER_PAINT_RADIAL
} ldg_term_render_paint_kind_t;

typedef struct ldg_term_render_color
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t pudding[1];
} ldg_term_render_color_t;

#define LDG_TERM_RENDER_FRAME_EDGE_NONE 0x00U
#define LDG_TERM_RENDER_FRAME_EDGE_TOP 0x01U
#define LDG_TERM_RENDER_FRAME_EDGE_RHS 0x02U
#define LDG_TERM_RENDER_FRAME_EDGE_BOTTOM 0x04U
#define LDG_TERM_RENDER_FRAME_EDGE_LHS 0x08U
#define LDG_TERM_RENDER_FRAME_EDGE_MASK 0x0FU
#define LDG_TERM_RENDER_FRAME_EDGE_TOP_LHS (LDG_TERM_RENDER_FRAME_EDGE_TOP | LDG_TERM_RENDER_FRAME_EDGE_LHS)
#define LDG_TERM_RENDER_FRAME_EDGE_BOTTOM_RHS (LDG_TERM_RENDER_FRAME_EDGE_BOTTOM | LDG_TERM_RENDER_FRAME_EDGE_RHS)

typedef struct ldg_term_render_frame_style
{
    ldg_term_render_color_t border;
    ldg_term_render_color_t accent;
    ldg_term_render_color_t bg;
    ldg_term_render_color_t title_fg;
    ldg_term_render_color_t title_bg;
    uint8_t accent_edge;
    uint8_t pudding[3];
} ldg_term_render_frame_style_t;

typedef struct ldg_term_render_affine2
{
    ldg_term_render_fixed_t elem[LDG_TERM_RENDER_AFFINE2_CUNT];
} ldg_term_render_affine2_t;

typedef struct ldg_term_render_grad_stop
{
    ldg_term_render_fixed_t off;
    ldg_term_render_color_t color;
} ldg_term_render_grad_stop_t;

typedef struct ldg_term_render_paint
{
    ldg_term_render_color_t color;
    ldg_term_render_fixed_t center[LDG_TERM_RENDER_PLANE_CUNT];
    ldg_term_render_fixed_t radius;
    uint32_t kind;
    uint32_t bright;
    uint32_t stop_cunt;
    ldg_term_render_grad_stop_t stop[LDG_TERM_RENDER_GRAD_STOP_MAX];
} ldg_term_render_paint_t;

typedef struct ldg_term_render_vert2
{
    ldg_term_render_fixed_t pos[LDG_TERM_RENDER_PLANE_CUNT];
    ldg_term_render_color_t color;
} ldg_term_render_vert2_t;

typedef struct ldg_term_render_vert3
{
    ldg_term_render_fixed_t pos[LDG_TERM_RENDER_AXIS_CUNT];
    ldg_term_render_fixed_t normal[LDG_TERM_RENDER_AXIS_CUNT];
    ldg_term_render_color_t color;
} ldg_term_render_vert3_t;

typedef struct ldg_term_render_mat4
{
    ldg_term_render_fixed_t elem[LDG_TERM_RENDER_HOMOGENEOUS_CUNT * LDG_TERM_RENDER_HOMOGENEOUS_CUNT];
} ldg_term_render_mat4_t;

typedef struct ldg_term_render_light
{
    ldg_term_render_fixed_t dir[LDG_TERM_RENDER_AXIS_CUNT];
    ldg_term_render_color_t ambient;
    ldg_term_render_color_t diffuse;
} ldg_term_render_light_t;

typedef struct ldg_term_render_conf
{
    uint32_t col_cunt;
    uint32_t row_cunt;
    uint32_t sample_col_cunt;
    uint32_t sample_row_cunt;
    uint32_t cell_aspect_num;
    uint32_t cell_aspect_den;
    uint32_t vert_max;
    uint32_t cross_max;
    uint32_t fit_gain_min;
    uint32_t fit_hold;
    uint32_t fit_contrast_min;
    uint8_t fg_only;
    uint8_t bg[LDG_TERM_RGB_CUNT];
} ldg_term_render_conf_t;

typedef struct ldg_term_render_clip_vert
{
    int64_t pos[LDG_TERM_RENDER_HOMOGENEOUS_CUNT];
    int64_t color[LDG_TERM_RENDER_AXIS_CUNT];
    int64_t pudding;
} ldg_term_render_clip_vert_t;

typedef struct ldg_term_render
{
    ldg_mem_pool_t *pool;
    uint8_t *sample_color;
    uint8_t *sample_prev;
    uint32_t *sample_depth;
    ldg_term_cell_t *cell;
    ldg_term_cell_t *text_cell;
    uint8_t *text_mask;
    uint8_t *text_prev;
    uint8_t *cell_ready;
    uint8_t *glyph_mask;
    uint16_t *glyph_coverage;
    uint8_t *glyph_by_coverage;
    uint64_t *glyph_bits;
    uint8_t *ramp_glyph;
    uint32_t *cell_sample_off;
    ldg_term_render_clip_vert_t *vert_buff;
    int64_t *cross_buff;
    ldg_term_render_mat4_t transform;
    ldg_term_render_light_t light;
    uint32_t col_cunt;
    uint32_t row_cunt;
    uint32_t sample_col_cunt;
    uint32_t sample_row_cunt;
    uint32_t width;
    uint32_t height;
    uint32_t cell_aspect_num;
    uint32_t cell_aspect_den;
    uint32_t vert_max;
    uint32_t cross_max;
    uint32_t fit_gain_min;
    uint32_t fit_hold;
    uint32_t fit_contrast_min;
    uint32_t ramp_cunt;
    ldg_term_rect_t clip;
    uint32_t scroll_col;
    uint32_t scroll_row;
    uint32_t clip_x_lo;
    uint32_t clip_y_lo;
    uint32_t clip_x_hi;
    uint32_t clip_y_hi;
    uint8_t depth_test;
    uint8_t light_en;
    uint8_t fg_only;
    uint8_t bg[LDG_TERM_RGB_CUNT];
    uint8_t pudding[6];
} ldg_term_render_t;

LDG_EXPORT extern const uint8_t LDG_TERM_RENDER_GLYPH_MASK[LDG_TERM_RENDER_GLYPH_CUNT][LDG_TERM_RENDER_GLYPH_ROW_CUNT];

LDG_EXPORT uint32_t ldg_term_render_conf_default_get(ldg_term_render_conf_t *out);
LDG_EXPORT uint32_t ldg_term_render_pool_size_get(const ldg_term_render_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_term_render_create(ldg_mem_pool_t *pool, const ldg_term_render_conf_t *conf, ldg_term_render_t **out);
LDG_EXPORT uint32_t ldg_term_render_destroy(ldg_term_render_t **render);

LDG_EXPORT uint32_t ldg_term_render_clear(ldg_term_render_t *render, const ldg_term_render_color_t *color);
LDG_EXPORT uint32_t ldg_term_render_depth_test_set(ldg_term_render_t *render, uint8_t en);
LDG_EXPORT uint32_t ldg_term_render_transform_set(ldg_term_render_t *render, const ldg_term_render_mat4_t *transform);
LDG_EXPORT uint32_t ldg_term_render_light_set(ldg_term_render_t *render, const ldg_term_render_light_t *light);
LDG_EXPORT uint32_t ldg_term_render_light_en_set(ldg_term_render_t *render, uint8_t en);
LDG_EXPORT uint32_t ldg_term_render_clip_set(ldg_term_render_t *render, const ldg_term_rect_t *clip, uint32_t scroll_col, uint32_t scroll_row);
LDG_EXPORT uint32_t ldg_term_render_clip_rst(ldg_term_render_t *render);
LDG_EXPORT uint32_t ldg_term_render_clip_get(const ldg_term_render_t *render, ldg_term_rect_t *out);

LDG_EXPORT uint32_t ldg_term_render_point_draw(ldg_term_render_t *render, const ldg_term_render_vert2_t *pt);
LDG_EXPORT uint32_t ldg_term_render_line_draw(ldg_term_render_t *render, const ldg_term_render_vert2_t *lhs, const ldg_term_render_vert2_t *rhs);
LDG_EXPORT uint32_t ldg_term_render_tri_draw(ldg_term_render_t *render, const ldg_term_render_vert2_t *vert_a, const ldg_term_render_vert2_t *vert_b, const ldg_term_render_vert2_t *vert_c);
LDG_EXPORT uint32_t ldg_term_render_rect_fill(ldg_term_render_t *render, const ldg_term_render_fixed_t *pos, const ldg_term_render_fixed_t *extent, const ldg_term_render_color_t *color);
LDG_EXPORT uint32_t ldg_term_render_text_draw(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *text, uint64_t len, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg);
LDG_EXPORT uint32_t ldg_term_render_poly_fill(ldg_term_render_t *render, const ldg_term_render_fixed_t *pt, const uint32_t *contour_len, uint32_t contour_cunt, uint32_t rule, const ldg_term_render_affine2_t *xf, const ldg_term_render_paint_t *paint);
LDG_EXPORT uint32_t ldg_term_render_svg_draw(ldg_term_render_t *render, const ldg_fmt_svg_t *svg, const ldg_term_render_fixed_t *pos, const ldg_term_render_fixed_t *extent, uint32_t bright);
LDG_EXPORT uint32_t ldg_term_render_frame_draw(ldg_term_render_t *render, const char *title, uint64_t len, const ldg_term_render_frame_style_t *style);

LDG_EXPORT uint32_t ldg_term_render_tri3_draw(ldg_term_render_t *render, const ldg_term_render_vert3_t *vert_a, const ldg_term_render_vert3_t *vert_b, const ldg_term_render_vert3_t *vert_c);
LDG_EXPORT uint32_t ldg_term_render_mesh_draw(ldg_term_render_t *render, const ldg_term_render_vert3_t *vert, uint32_t vert_cunt, const uint32_t *idx, uint32_t idx_cunt);

LDG_EXPORT uint32_t ldg_term_render_resolve(ldg_term_render_t *render);
LDG_EXPORT uint32_t ldg_term_render_cell_get(const ldg_term_render_t *render, uint32_t col, uint32_t row, ldg_term_cell_t *out);
LDG_EXPORT uint32_t ldg_term_render_cell_arr_get(const ldg_term_render_t *render, const ldg_term_cell_t **out, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_term_render_place(const ldg_term_render_t *render, ldg_term_t *term, uint32_t col, uint32_t row);
LDG_EXPORT uint32_t ldg_term_render_aspect_get(const ldg_term_render_t *render, ldg_term_render_fixed_t *out);

LDG_EXPORT uint32_t ldg_term_render_fixed_mul(ldg_term_render_fixed_t lhs, ldg_term_render_fixed_t rhs, ldg_term_render_fixed_t *out);
LDG_EXPORT uint32_t ldg_term_render_fixed_div(ldg_term_render_fixed_t lhs, ldg_term_render_fixed_t rhs, ldg_term_render_fixed_t *out);
LDG_EXPORT uint32_t ldg_term_render_fixed_square_root_get(ldg_term_render_fixed_t val, ldg_term_render_fixed_t *out);
LDG_EXPORT uint32_t ldg_term_render_fixed_sin_get(ldg_term_render_angle_t angle, ldg_term_render_fixed_t *out);
LDG_EXPORT uint32_t ldg_term_render_fixed_cos_get(ldg_term_render_angle_t angle, ldg_term_render_fixed_t *out);

LDG_EXPORT uint32_t ldg_term_render_mat4_ident_set(ldg_term_render_mat4_t *out);
LDG_EXPORT uint32_t ldg_term_render_mat4_mul(const ldg_term_render_mat4_t *lhs, const ldg_term_render_mat4_t *rhs, ldg_term_render_mat4_t *out);
LDG_EXPORT uint32_t ldg_term_render_mat4_translation_set(const ldg_term_render_fixed_t *pos, ldg_term_render_mat4_t *out);
LDG_EXPORT uint32_t ldg_term_render_mat4_scale_set(const ldg_term_render_fixed_t *scale, ldg_term_render_mat4_t *out);
LDG_EXPORT uint32_t ldg_term_render_mat4_rotation_x_set(ldg_term_render_angle_t angle, ldg_term_render_mat4_t *out);
LDG_EXPORT uint32_t ldg_term_render_mat4_rotation_y_set(ldg_term_render_angle_t angle, ldg_term_render_mat4_t *out);
LDG_EXPORT uint32_t ldg_term_render_mat4_rotation_z_set(ldg_term_render_angle_t angle, ldg_term_render_mat4_t *out);
LDG_EXPORT uint32_t ldg_term_render_mat4_perspective_set(ldg_term_render_angle_t fov_y, ldg_term_render_fixed_t aspect, ldg_term_render_fixed_t near, ldg_term_render_fixed_t far, ldg_term_render_mat4_t *out);
LDG_EXPORT uint32_t ldg_term_render_mat4_cam_set(const ldg_term_render_fixed_t *eye, const ldg_term_render_fixed_t *tgt, const ldg_term_render_fixed_t *up, ldg_term_render_mat4_t *out);

#endif
