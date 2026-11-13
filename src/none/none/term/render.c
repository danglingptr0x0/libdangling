#include <stdint.h>

#include <dangling/term/render.h>
#include <dangling/term/term.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>
#include <dangling/mem/cpy.h>
#include <dangling/str/str.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_TERM_RENDER_STEP 1
#define LDG_TERM_RENDER_RED_IDX 0
#define LDG_TERM_RENDER_GREEN_IDX 1
#define LDG_TERM_RENDER_BLUE_IDX 2
#define LDG_TERM_RENDER_X_IDX 0
#define LDG_TERM_RENDER_Y_IDX 1
#define LDG_TERM_RENDER_Z_IDX 2
#define LDG_TERM_RENDER_W_IDX 3
#define LDG_TERM_RENDER_RGB_CUNT 3
#define LDG_TERM_RENDER_COLOR_MIN 0
#define LDG_TERM_RENDER_COLOR_MAX 255
#define LDG_TERM_RENDER_LUMA_RED 299
#define LDG_TERM_RENDER_LUMA_GREEN 587
#define LDG_TERM_RENDER_LUMA_BLUE 114
#define LDG_TERM_RENDER_LUMA_SCALE 1000
#define LDG_TERM_RENDER_ALLOC_CUNT 17
#define LDG_TERM_RENDER_WORD_SHIFT 6
#define LDG_TERM_RENDER_WORD_MASK 63
#define LDG_TERM_RENDER_POPCOUNT_M1 0x5555555555555555ULL
#define LDG_TERM_RENDER_POPCOUNT_M2 0x3333333333333333ULL
#define LDG_TERM_RENDER_POPCOUNT_M4 0x0F0F0F0F0F0F0F0FULL
#define LDG_TERM_RENDER_POPCOUNT_H01 0x0101010101010101ULL
#define LDG_TERM_RENDER_POPCOUNT_SHIFT_1 1
#define LDG_TERM_RENDER_POPCOUNT_SHIFT_2 2
#define LDG_TERM_RENDER_POPCOUNT_SHIFT_4 4
#define LDG_TERM_RENDER_POPCOUNT_SHIFT_OUT 56
#define LDG_TERM_RENDER_RAMP_SET " .'\x60,:;-~=+*<>!/\\|()[]{}#%&@$"
#define LDG_TERM_RENDER_LUMA_MAX (LDG_TERM_RENDER_COLOR_MAX * LDG_TERM_RENDER_LUMA_SCALE)
#define LDG_TERM_RENDER_TRI_VERT_CUNT 3
#define LDG_TERM_RENDER_CLIP_VERT_MAX 4
#define LDG_TERM_RENDER_SUBPIXEL_SHIFT 4
#define LDG_TERM_RENDER_SUBPIXEL_ONE 16
#define LDG_TERM_RENDER_SUBPIXEL_HALF 8
#define LDG_TERM_RENDER_FIXED_TO_SUBPIXEL_SHIFT 12
#define LDG_TERM_RENDER_NDC_TO_SUBPIXEL_SHIFT 13
#define LDG_TERM_RENDER_NDC_GUARD (4096 * LDG_TERM_RENDER_FIXED_ONE)
#define LDG_TERM_RENDER_NDC_GUARD_LOW (-LDG_TERM_RENDER_NDC_GUARD)
#define LDG_TERM_RENDER_NDC_NEAR (-LDG_TERM_RENDER_FIXED_ONE)
#define LDG_TERM_RENDER_NDC_FAR LDG_TERM_RENDER_FIXED_ONE
#define LDG_TERM_RENDER_PERSPECTIVE_W_Z (-LDG_TERM_RENDER_FIXED_ONE)
#define LDG_TERM_RENDER_CLIP_COORD_MAX (1LL << 40)
#define LDG_TERM_RENDER_CLIP_COORD_MIN (-LDG_TERM_RENDER_CLIP_COORD_MAX)
#define LDG_TERM_RENDER_DEPTH_SCALE_SHIFT 15
#define LDG_TERM_RENDER_INV_AREA_SHIFT 62
#define LDG_TERM_RENDER_INV_AREA_ONE (1LL << LDG_TERM_RENDER_INV_AREA_SHIFT)
#define LDG_TERM_RENDER_BARY_ONE_SHIFT 30
#define LDG_TERM_RENDER_BARY_SHIFT (LDG_TERM_RENDER_INV_AREA_SHIFT - LDG_TERM_RENDER_BARY_ONE_SHIFT)
#define LDG_TERM_RENDER_FIT_FLAT_DEVIATION 4
#define LDG_TERM_RENDER_FLAT_ON_CUNT 0
#define LDG_TERM_RENDER_MASK_ON_DIV 4
#define LDG_TERM_RENDER_QUADRANT_SHIFT 30
#define LDG_TERM_RENDER_QUADRANT_MASK 0x3FFFFFFFU
#define LDG_TERM_RENDER_QUADRANT_ONE 0x40000000LL
#define LDG_TERM_RENDER_Q30_TO_Q16_SHIFT 14
#define LDG_TERM_RENDER_SIN_C1 1686629713LL
#define LDG_TERM_RENDER_SIN_C3 693598671LL
#define LDG_TERM_RENDER_SIN_C5 85569278LL
#define LDG_TERM_RENDER_SIN_C7 5026937LL
#define LDG_TERM_RENDER_SIN_C9 172271LL
#define LDG_TERM_RENDER_SQRT_BIT_START (1ULL << 62)
#define LDG_TERM_RENDER_SQRT_BIT_STEP 2
#define LDG_TERM_RENDER_MASK_MSB 0x80U
#define LDG_TERM_RENDER_MAT4_DIM 4
#define LDG_TERM_RENDER_ELEM(row, col) ((row) * LDG_TERM_RENDER_MAT4_DIM + (col))
#define LDG_TERM_RENDER_FRAME_CORNER '+'
#define LDG_TERM_RENDER_FRAME_HORIZONTAL '-'
#define LDG_TERM_RENDER_FRAME_VERTICAL '|'
#define LDG_TERM_RENDER_FRAME_EDGE_CUNT 2
#define LDG_TERM_RENDER_SCROLL_NONE 0
#define LDG_TERM_RENDER_CROSS_PAIR 2
#define LDG_TERM_RENDER_CROSS_DIR_UP 1
#define LDG_TERM_RENDER_CROSS_DIR_DOWN 0
#define LDG_TERM_RENDER_AFFINE2_A_IDX 0
#define LDG_TERM_RENDER_AFFINE2_B_IDX 1
#define LDG_TERM_RENDER_AFFINE2_C_IDX 2
#define LDG_TERM_RENDER_AFFINE2_D_IDX 3
#define LDG_TERM_RENDER_AFFINE2_E_IDX 4
#define LDG_TERM_RENDER_AFFINE2_F_IDX 5
#define LDG_TERM_RENDER_CONTOUR_EDGE_MIN 2
#define LDG_TERM_RENDER_WINDING_PARITY 2
#define LDG_TERM_RENDER_GRAD_LERP_ONE 256
#define LDG_TERM_RENDER_SAMPLE_FIRST 0

typedef struct render_screen_vert
{
    int64_t pos[LDG_TERM_RENDER_PLANE_CUNT];
    int64_t color[LDG_TERM_RENDER_RGB_CUNT];
    uint32_t depth;
    uint32_t pudding;
} render_screen_vert_t;

const uint8_t LDG_TERM_RENDER_GLYPH_MASK[LDG_TERM_RENDER_GLYPH_CUNT][LDG_TERM_RENDER_GLYPH_ROW_CUNT] =
{
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x18, 0x3C, 0x3C, 0x3C, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x66, 0x66, 0x66, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x6C, 0x6C, 0xFE, 0x6C, 0x6C, 0x6C, 0xFE, 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00 },
    { 0x18, 0x18, 0x7C, 0xC6, 0xC2, 0xC0, 0x7C, 0x06, 0x06, 0x86, 0xC6, 0x7C, 0x18, 0x18, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0xC2, 0xC6, 0x0C, 0x18, 0x30, 0x60, 0xC6, 0x86, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x38, 0x6C, 0x6C, 0x38, 0x76, 0xDC, 0xCC, 0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x30, 0x30, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x30, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x02, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xCE, 0xDE, 0xF6, 0xE6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x18, 0x38, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x7C, 0xC6, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0xC6, 0xFE, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x7C, 0xC6, 0x06, 0x06, 0x3C, 0x06, 0x06, 0x06, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x0C, 0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x0C, 0x0C, 0x1E, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xFE, 0xC0, 0xC0, 0xC0, 0xFC, 0x06, 0x06, 0x06, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x38, 0x60, 0xC0, 0xC0, 0xFC, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xFE, 0xC6, 0x06, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x7E, 0x06, 0x06, 0x06, 0x0C, 0x78, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0x0C, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xDE, 0xDE, 0xDE, 0xDC, 0xC0, 0xC0, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x10, 0x38, 0x6C, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xFC, 0x66, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x66, 0x66, 0xFC, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x3C, 0x66, 0xC2, 0xC0, 0xC0, 0xC0, 0xC0, 0xC2, 0x66, 0x3C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xF8, 0x6C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x6C, 0xF8, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xFE, 0x66, 0x62, 0x68, 0x78, 0x68, 0x60, 0x62, 0x66, 0xFE, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xFE, 0x66, 0x62, 0x68, 0x78, 0x68, 0x60, 0x60, 0x60, 0xF0, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x3C, 0x66, 0xC2, 0xC0, 0xC0, 0xDE, 0xC6, 0xC6, 0x66, 0x3A, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xFE, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0xCC, 0x78, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xE6, 0x66, 0x66, 0x6C, 0x78, 0x78, 0x6C, 0x66, 0x66, 0xE6, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xF0, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x62, 0x66, 0xFE, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xC6, 0xEE, 0xFE, 0xFE, 0xD6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xC6, 0xE6, 0xF6, 0xFE, 0xDE, 0xCE, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xFC, 0x66, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x60, 0xF0, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xD6, 0xDE, 0x7C, 0x0C, 0x0E, 0x00, 0x00 },
    { 0x00, 0x00, 0xFC, 0x66, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x66, 0x66, 0xE6, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0x60, 0x38, 0x0C, 0x06, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x7E, 0x7E, 0x5A, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xD6, 0xD6, 0xD6, 0xFE, 0xEE, 0x6C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xC6, 0xC6, 0x6C, 0x7C, 0x38, 0x38, 0x7C, 0x6C, 0xC6, 0xC6, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xFE, 0xC6, 0x86, 0x0C, 0x18, 0x30, 0x60, 0xC2, 0xC6, 0xFE, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x06, 0x02, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00, 0x00, 0x00, 0x00 },
    { 0x10, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00 },
    { 0x30, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x0C, 0x7C, 0xCC, 0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0xE0, 0x60, 0x60, 0x78, 0x6C, 0x66, 0x66, 0x66, 0x66, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC0, 0xC0, 0xC0, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x1C, 0x0C, 0x0C, 0x3C, 0x6C, 0xCC, 0xCC, 0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xFE, 0xC0, 0xC0, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x38, 0x6C, 0x64, 0x60, 0xF0, 0x60, 0x60, 0x60, 0x60, 0xF0, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x7C, 0x0C, 0xCC, 0x78, 0x00 },
    { 0x00, 0x00, 0xE0, 0x60, 0x60, 0x6C, 0x76, 0x66, 0x66, 0x66, 0x66, 0xE6, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x18, 0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x06, 0x06, 0x00, 0x0E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x66, 0x66, 0x3C, 0x00 },
    { 0x00, 0x00, 0xE0, 0x60, 0x60, 0x66, 0x6C, 0x78, 0x78, 0x6C, 0x66, 0xE6, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0xEC, 0xFE, 0xD6, 0xD6, 0xD6, 0xD6, 0xC6, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7C, 0x60, 0x60, 0xF0, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x7C, 0x0C, 0x0C, 0x1E, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x76, 0x66, 0x60, 0x60, 0x60, 0xF0, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0xC6, 0x60, 0x38, 0x0C, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x10, 0x30, 0x30, 0xFC, 0x30, 0x30, 0x30, 0x30, 0x36, 0x1C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0xC6, 0xC6, 0xD6, 0xD6, 0xD6, 0xFE, 0x6C, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0xC6, 0x6C, 0x38, 0x38, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x7E, 0x06, 0x0C, 0xF8, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xCC, 0x18, 0x30, 0x60, 0xC6, 0xFE, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x0E, 0x18, 0x18, 0x18, 0x70, 0x18, 0x18, 0x18, 0x18, 0x0E, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x70, 0x18, 0x18, 0x18, 0x0E, 0x18, 0x18, 0x18, 0x18, 0x70, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x76, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static uint8_t render_conf_valid_is(const ldg_term_render_conf_t *conf)
{
    if (conf->col_cunt == 0 || conf->col_cunt > LDG_TERM_COL_MAX) { return LDG_TRUTH_FALSE; }

    if (conf->row_cunt == 0 || conf->row_cunt > LDG_TERM_ROW_MAX) { return LDG_TRUTH_FALSE; }

    if (conf->sample_col_cunt == 0 || conf->sample_col_cunt > LDG_TERM_RENDER_GLYPH_COL_CUNT) { return LDG_TRUTH_FALSE; }

    if (conf->sample_row_cunt == 0 || conf->sample_row_cunt > LDG_TERM_RENDER_GLYPH_ROW_CUNT) { return LDG_TRUTH_FALSE; }

    if ((LDG_TERM_RENDER_GLYPH_COL_CUNT % conf->sample_col_cunt) != 0) { return LDG_TRUTH_FALSE; }

    if ((LDG_TERM_RENDER_GLYPH_ROW_CUNT % conf->sample_row_cunt) != 0) { return LDG_TRUTH_FALSE; }

    if (conf->cell_aspect_num == 0 || conf->cell_aspect_den == 0) { return LDG_TRUTH_FALSE; }

    if (conf->vert_max < LDG_TERM_RENDER_TRI_VERT_CUNT) { return LDG_TRUTH_FALSE; }

    if (conf->cross_max < LDG_TERM_RENDER_CROSS_PAIR) { return LDG_TRUTH_FALSE; }

    if (conf->fit_gain_min > LDG_TERM_RENDER_PER_MILLE || conf->fit_hold > LDG_TERM_RENDER_PER_MILLE) { return LDG_TRUTH_FALSE; }

    if (conf->fit_contrast_min > LDG_TERM_RENDER_FIT_CONTRAST_MAX) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint64_t render_pool_need_get(const ldg_term_render_conf_t *conf)
{
    uint64_t sample_cunt = 0;
    uint64_t cell_cunt = 0;
    uint64_t cell_sample_cunt = 0;
    uint64_t total = 0;

    cell_cunt = (uint64_t)conf->col_cunt * (uint64_t)conf->row_cunt;
    cell_sample_cunt = (uint64_t)conf->sample_col_cunt * (uint64_t)conf->sample_row_cunt;
    sample_cunt = cell_cunt * cell_sample_cunt;

    total = (uint64_t)sizeof(ldg_term_render_t);
    total += sample_cunt * LDG_TERM_RENDER_RGB_CUNT;
    total += sample_cunt * LDG_TERM_RENDER_RGB_CUNT;
    total += sample_cunt * (uint64_t)sizeof(uint32_t);
    total += cell_cunt * (uint64_t)sizeof(ldg_term_cell_t);
    total += cell_cunt * (uint64_t)sizeof(ldg_term_cell_t);
    total += cell_cunt;
    total += cell_cunt;
    total += cell_cunt;
    total += (uint64_t)LDG_TERM_RENDER_GLYPH_CUNT * cell_sample_cunt;
    total += (uint64_t)LDG_TERM_RENDER_GLYPH_CUNT * (uint64_t)sizeof(uint16_t);
    total += (uint64_t)LDG_TERM_RENDER_GLYPH_CUNT;
    total += (uint64_t)LDG_TERM_RENDER_GLYPH_CUNT * LDG_TERM_RENDER_BIT_WORD_CUNT * (uint64_t)sizeof(uint64_t);
    total += (uint64_t)LDG_TERM_RENDER_RAMP_MAX;
    total += cell_sample_cunt * (uint64_t)sizeof(uint32_t);
    total += (uint64_t)conf->vert_max * (uint64_t)sizeof(ldg_term_render_clip_vert_t);
    total += (uint64_t)conf->cross_max * (uint64_t)sizeof(int64_t);
    total += (uint64_t)LDG_TERM_RENDER_ALLOC_CUNT * (uint64_t)LDG_MEM_POOL_VAR_ALIGN;

    return total;
}

static void render_glyph_table_build(ldg_term_render_t *render)
{
    uint32_t glyph = 0;
    uint32_t sample_col = 0;
    uint32_t sample_row = 0;
    uint32_t mask_col = 0;
    uint32_t mask_row = 0;
    uint32_t col_span = 0;
    uint32_t row_span = 0;
    uint32_t on_cunt = 0;
    uint32_t span_cunt = 0;
    uint32_t coverage = 0;
    uint32_t cell_sample_cunt = 0;
    uint32_t sample_idx = 0;
    uint32_t sorted = 0;
    uint32_t probe = 0;
    uint8_t swap = 0;

    col_span = LDG_TERM_RENDER_GLYPH_COL_CUNT / render->sample_col_cunt;
    row_span = LDG_TERM_RENDER_GLYPH_ROW_CUNT / render->sample_row_cunt;
    span_cunt = col_span * row_span;
    cell_sample_cunt = render->sample_col_cunt * render->sample_row_cunt;

    for (glyph = 0; glyph < LDG_TERM_RENDER_GLYPH_CUNT; glyph++)
    {
        coverage = 0;

        for (sample_row = 0; sample_row < render->sample_row_cunt; sample_row++)
        {
            for (sample_col = 0; sample_col < render->sample_col_cunt; sample_col++)
            {
                on_cunt = 0;

                for (mask_row = sample_row * row_span; mask_row < (sample_row + LDG_TERM_RENDER_STEP) * row_span; mask_row++) { for (mask_col = sample_col * col_span; mask_col < (sample_col + LDG_TERM_RENDER_STEP) * col_span; mask_col++) { if ((LDG_TERM_RENDER_GLYPH_MASK[glyph][mask_row] & (LDG_TERM_RENDER_MASK_MSB >> mask_col)) != 0) { on_cunt++; } } }

                if (on_cunt * LDG_TERM_RENDER_MASK_ON_DIV >= span_cunt && on_cunt > 0)
                {
                    sample_idx = (sample_row * render->sample_col_cunt) + sample_col;
                    render->glyph_mask[(glyph * cell_sample_cunt) + coverage] = (uint8_t)sample_idx;
                    render->glyph_bits[(glyph * LDG_TERM_RENDER_BIT_WORD_CUNT) + (sample_idx >> LDG_TERM_RENDER_WORD_SHIFT)] |= (uint64_t)LDG_TERM_RENDER_STEP << (sample_idx & LDG_TERM_RENDER_WORD_MASK);
                    coverage++;
                }
            }
        }

        render->glyph_coverage[glyph] = (uint16_t)coverage;
        render->glyph_by_coverage[glyph] = (uint8_t)glyph;
    }

    for (sorted = LDG_TERM_RENDER_STEP; sorted < LDG_TERM_RENDER_GLYPH_CUNT; sorted++)
    {
        probe = sorted;

        while (probe > 0 && render->glyph_coverage[render->glyph_by_coverage[probe - LDG_TERM_RENDER_STEP]] > render->glyph_coverage[render->glyph_by_coverage[probe]])
        {
            swap = render->glyph_by_coverage[probe];
            render->glyph_by_coverage[probe] = render->glyph_by_coverage[probe - LDG_TERM_RENDER_STEP];
            render->glyph_by_coverage[probe - LDG_TERM_RENDER_STEP] = swap;
            probe--;
        }
    }
}

static void render_ramp_build(ldg_term_render_t *render)
{
    const char *set = LDG_TERM_RENDER_RAMP_SET;
    uint32_t glyph = 0;
    uint32_t coverage = 0;
    uint32_t pos = 0;
    uint32_t probe = 0;
    uint8_t dup = LDG_TRUTH_FALSE;

    render->ramp_cunt = 0;

    for (pos = 0; set[pos] != LDG_STR_TERM && render->ramp_cunt < LDG_TERM_RENDER_RAMP_MAX; pos++)
    {
        glyph = (uint32_t)(uint8_t)set[pos] - LDG_TERM_RENDER_GLYPH_FIRST;
        coverage = render->glyph_coverage[glyph];
        dup = LDG_TRUTH_FALSE;

        for (probe = 0; probe < render->ramp_cunt; probe++) { if (render->glyph_coverage[render->ramp_glyph[probe]] == coverage) { dup = LDG_TRUTH_TRUE; } }

        if (dup == LDG_TRUTH_TRUE) { continue; }

        probe = render->ramp_cunt;

        while (probe > 0 && render->glyph_coverage[render->ramp_glyph[probe - LDG_TERM_RENDER_STEP]] > coverage)
        {
            render->ramp_glyph[probe] = render->ramp_glyph[probe - LDG_TERM_RENDER_STEP];
            probe--;
        }

        render->ramp_glyph[probe] = (uint8_t)glyph;
        render->ramp_cunt++;
    }
}

static void render_cell_sample_off_build(ldg_term_render_t *render)
{
    uint32_t sample_col = 0;
    uint32_t sample_row = 0;

    for (sample_row = 0; sample_row < render->sample_row_cunt; sample_row++) { for (sample_col = 0; sample_col < render->sample_col_cunt; sample_col++) { render->cell_sample_off[(sample_row * render->sample_col_cunt) + sample_col] = (sample_row * render->width) + sample_col; } }
}

uint32_t ldg_term_render_conf_default_get(ldg_term_render_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->col_cunt = LDG_TERM_COL_DEFAULT;
    out->row_cunt = LDG_TERM_ROW_DEFAULT;
    out->sample_col_cunt = LDG_TERM_RENDER_SAMPLE_COL_DEFAULT;
    out->sample_row_cunt = LDG_TERM_RENDER_SAMPLE_ROW_DEFAULT;
    out->cell_aspect_num = LDG_TERM_RENDER_CELL_ASPECT_NUM_DEFAULT;
    out->cell_aspect_den = LDG_TERM_RENDER_CELL_ASPECT_DEN_DEFAULT;
    out->vert_max = LDG_TERM_RENDER_VERT_MAX_DEFAULT;
    out->cross_max = LDG_TERM_RENDER_CROSS_MAX_DEFAULT;
    out->fit_gain_min = LDG_TERM_RENDER_FIT_GAIN_MIN_DEFAULT;
    out->fit_hold = LDG_TERM_RENDER_FIT_HOLD_DEFAULT;
    out->fit_contrast_min = LDG_TERM_RENDER_FIT_CONTRAST_MIN_DEFAULT;
    out->fg_only = LDG_TRUTH_FALSE;
    out->bg[LDG_TERM_RENDER_RED_IDX] = LDG_TERM_RENDER_COLOR_MIN;
    out->bg[LDG_TERM_RENDER_GREEN_IDX] = LDG_TERM_RENDER_COLOR_MIN;
    out->bg[LDG_TERM_RENDER_BLUE_IDX] = LDG_TERM_RENDER_COLOR_MIN;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_pool_size_get(const ldg_term_render_conf_t *conf, uint64_t *out)
{
    if (LDG_UNLIKELY(!conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(render_conf_valid_is(conf) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = render_pool_need_get(conf);

    return LDG_ERR_AOK;
}

static void render_clip_apply(ldg_term_render_t *render, const ldg_term_rect_t *clip, uint32_t scroll_col, uint32_t scroll_row)
{
    render->clip = *clip;
    render->scroll_col = scroll_col;
    render->scroll_row = scroll_row;
    render->clip_x_lo = clip->col * render->sample_col_cunt;
    render->clip_y_lo = clip->row * render->sample_row_cunt;
    render->clip_x_hi = (clip->col + clip->col_cunt) * render->sample_col_cunt;
    render->clip_y_hi = (clip->row + clip->row_cunt) * render->sample_row_cunt;
}

static void render_clip_full_apply(ldg_term_render_t *render)
{
    ldg_term_rect_t full = LDG_STRUCT_ZERO_INIT;

    full.col_cunt = render->col_cunt;
    full.row_cunt = render->row_cunt;

    render_clip_apply(render, &full, LDG_TERM_RENDER_SCROLL_NONE, LDG_TERM_RENDER_SCROLL_NONE);
}

static void render_orig_get(const ldg_term_render_t *render, int64_t *sample_x, int64_t *sample_y)
{
    *sample_x = ((int64_t)render->clip.col - (int64_t)render->scroll_col) * (int64_t)render->sample_col_cunt;
    *sample_y = ((int64_t)render->clip.row - (int64_t)render->scroll_row) * (int64_t)render->sample_row_cunt;
}

static uint8_t render_clip_full_is(const ldg_term_render_t *render)
{
    if (render->clip_x_lo != 0 || render->clip_y_lo != 0) { return LDG_TRUTH_FALSE; }

    if (render->clip_x_hi != render->width || render->clip_y_hi != render->height) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static void render_text_cell_put(ldg_term_render_t *render, uint32_t col, uint32_t row, uint8_t ch, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg)
{
    ldg_term_cell_t *cell = 0x0;
    uint64_t cell_idx = 0;

    if (col < render->clip.col || col >= render->clip.col + render->clip.col_cunt) { return; }

    if (row < render->clip.row || row >= render->clip.row + render->clip.row_cunt) { return; }

    if (ch < LDG_ASCII_PRINTABLE_MIN || ch > LDG_ASCII_PRINTABLE_MAX) { ch = LDG_ASCII_NONPRINTABLE_CHAR; }

    cell_idx = ((uint64_t)row * render->col_cunt) + col;
    cell = &render->text_cell[cell_idx];
    cell->ch = ch;
    cell->attr = LDG_TERM_ATTR_NONE;
    cell->link = LDG_TERM_LINK_NONE;
    cell->fg[LDG_TERM_RENDER_RED_IDX] = fg->red;
    cell->fg[LDG_TERM_RENDER_GREEN_IDX] = fg->green;
    cell->fg[LDG_TERM_RENDER_BLUE_IDX] = fg->blue;
    cell->bg[LDG_TERM_RENDER_RED_IDX] = bg->red;
    cell->bg[LDG_TERM_RENDER_GREEN_IDX] = bg->green;
    cell->bg[LDG_TERM_RENDER_BLUE_IDX] = bg->blue;
    render->text_mask[cell_idx] = LDG_TRUTH_TRUE;
}

uint32_t ldg_term_render_create(ldg_mem_pool_t *pool, const ldg_term_render_conf_t *conf, ldg_term_render_t **out)
{
    ldg_term_render_t *render = 0x0;
    ldg_term_render_color_t black = LDG_STRUCT_ZERO_INIT;
    void *tmp = 0x0;
    uint64_t sample_cunt = 0;
    uint64_t cell_cunt = 0;
    uint64_t cell_sample_cunt = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(render_conf_valid_is(conf) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(render_pool_need_get(conf) > LDG_TERM_RENDER_POOL_MAX)) { return LDG_ERR_RANGE; }

    cell_cunt = (uint64_t)conf->col_cunt * (uint64_t)conf->row_cunt;
    cell_sample_cunt = (uint64_t)conf->sample_col_cunt * (uint64_t)conf->sample_row_cunt;
    sample_cunt = cell_cunt * cell_sample_cunt;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)sizeof(ldg_term_render_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render = (ldg_term_render_t *)tmp;
    render->pool = pool;
    render->col_cunt = conf->col_cunt;
    render->row_cunt = conf->row_cunt;
    render->sample_col_cunt = conf->sample_col_cunt;
    render->sample_row_cunt = conf->sample_row_cunt;
    render->width = conf->col_cunt * conf->sample_col_cunt;
    render->height = conf->row_cunt * conf->sample_row_cunt;
    render->cell_aspect_num = conf->cell_aspect_num;
    render->cell_aspect_den = conf->cell_aspect_den;
    render->vert_max = conf->vert_max;
    render->cross_max = conf->cross_max;
    render->depth_test = LDG_TRUTH_TRUE;
    render->light_en = LDG_TRUTH_FALSE;
    render->fg_only = (conf->fg_only != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    render->fit_gain_min = conf->fit_gain_min;
    render->fit_hold = conf->fit_hold;
    render->fit_contrast_min = conf->fit_contrast_min;
    render->ramp_cunt = 0;
    render->pudding[0] = 0;
    render->pudding[LDG_TERM_RENDER_STEP] = 0;
    render->bg[LDG_TERM_RENDER_RED_IDX] = conf->bg[LDG_TERM_RENDER_RED_IDX];
    render->bg[LDG_TERM_RENDER_GREEN_IDX] = conf->bg[LDG_TERM_RENDER_GREEN_IDX];
    render->bg[LDG_TERM_RENDER_BLUE_IDX] = conf->bg[LDG_TERM_RENDER_BLUE_IDX];

    ret = ldg_mem_pool_alloc(pool, sample_cunt * LDG_TERM_RENDER_RGB_CUNT, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->sample_color = (uint8_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, sample_cunt * LDG_TERM_RENDER_RGB_CUNT, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->sample_prev = (uint8_t *)tmp;

    ret = ldg_mem_zero(render->sample_prev, sample_cunt * LDG_TERM_RENDER_RGB_CUNT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(pool, sample_cunt * (uint64_t)sizeof(uint32_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->sample_depth = (uint32_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, cell_cunt * (uint64_t)sizeof(ldg_term_cell_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->cell = (ldg_term_cell_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, cell_cunt * (uint64_t)sizeof(ldg_term_cell_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->text_cell = (ldg_term_cell_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, cell_cunt, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->text_mask = (uint8_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, cell_cunt, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->text_prev = (uint8_t *)tmp;

    ret = ldg_mem_zero(render->text_prev, cell_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(pool, cell_cunt, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->cell_ready = (uint8_t *)tmp;

    ret = ldg_mem_zero(render->cell_ready, cell_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)LDG_TERM_RENDER_GLYPH_CUNT * cell_sample_cunt, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->glyph_mask = (uint8_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)LDG_TERM_RENDER_GLYPH_CUNT * (uint64_t)sizeof(uint16_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->glyph_coverage = (uint16_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)LDG_TERM_RENDER_GLYPH_CUNT, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->glyph_by_coverage = (uint8_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)LDG_TERM_RENDER_GLYPH_CUNT * LDG_TERM_RENDER_BIT_WORD_CUNT * (uint64_t)sizeof(uint64_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->glyph_bits = (uint64_t *)tmp;

    ret = ldg_mem_zero(render->glyph_bits, (uint64_t)LDG_TERM_RENDER_GLYPH_CUNT * LDG_TERM_RENDER_BIT_WORD_CUNT * (uint64_t)sizeof(uint64_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)LDG_TERM_RENDER_RAMP_MAX, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->ramp_glyph = (uint8_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, cell_sample_cunt * (uint64_t)sizeof(uint32_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->cell_sample_off = (uint32_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)conf->vert_max * (uint64_t)sizeof(ldg_term_render_clip_vert_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->vert_buff = (ldg_term_render_clip_vert_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)conf->cross_max * (uint64_t)sizeof(int64_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->cross_buff = (int64_t *)tmp;

    render_glyph_table_build(render);
    render_ramp_build(render);
    render_cell_sample_off_build(render);

    for (idx = 0; idx < cell_cunt; idx++)
    {
        render->cell[idx].ch = (uint8_t)LDG_TERM_RENDER_GLYPH_FIRST;
        render->cell[idx].attr = LDG_TERM_ATTR_NONE;
        render->cell[idx].link = LDG_TERM_LINK_NONE;
        render->cell[idx].fg[LDG_TERM_RENDER_RED_IDX] = LDG_TERM_RENDER_COLOR_MIN;
        render->cell[idx].fg[LDG_TERM_RENDER_GREEN_IDX] = LDG_TERM_RENDER_COLOR_MIN;
        render->cell[idx].fg[LDG_TERM_RENDER_BLUE_IDX] = LDG_TERM_RENDER_COLOR_MIN;
        render->cell[idx].bg[LDG_TERM_RENDER_RED_IDX] = LDG_TERM_RENDER_COLOR_MIN;
        render->cell[idx].bg[LDG_TERM_RENDER_GREEN_IDX] = LDG_TERM_RENDER_COLOR_MIN;
        render->cell[idx].bg[LDG_TERM_RENDER_BLUE_IDX] = LDG_TERM_RENDER_COLOR_MIN;
    }

    ret = ldg_term_render_mat4_ident_set(&render->transform);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render->light.dir[LDG_TERM_RENDER_X_IDX] = 0;
    render->light.dir[LDG_TERM_RENDER_Y_IDX] = 0;
    render->light.dir[LDG_TERM_RENDER_Z_IDX] = LDG_TERM_RENDER_FIXED_ONE;
    render->light.ambient.red = LDG_TERM_RENDER_COLOR_MAX;
    render->light.ambient.green = LDG_TERM_RENDER_COLOR_MAX;
    render->light.ambient.blue = LDG_TERM_RENDER_COLOR_MAX;
    render->light.ambient.pudding[0] = 0;
    render->light.diffuse.red = 0;
    render->light.diffuse.green = 0;
    render->light.diffuse.blue = 0;
    render->light.diffuse.pudding[0] = 0;

    render_clip_full_apply(render);

    ret = ldg_term_render_clear(render, &black);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = render;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_destroy(ldg_term_render_t **render)
{
    if (LDG_UNLIKELY(!render || !*render)) { return LDG_ERR_FUNC_ARG_NULL; }

    *render = 0x0;

    return LDG_ERR_AOK;
}

static uint32_t render_pattern_extend(uint8_t *base, uint64_t seed_size, uint64_t total_size)
{
    uint64_t filled = seed_size;
    uint64_t chunk = 0;
    uint32_t ret = 0;

    while (filled < total_size)
    {
        chunk = (filled < total_size - filled) ? filled : total_size - filled;

        ret = ldg_mem_cpy(base + filled, base, chunk);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        filled += chunk;
    }

    return LDG_ERR_AOK;
}

static uint32_t render_clip_clear(ldg_term_render_t *render, const ldg_term_render_color_t *color)
{
    uint8_t *dst = 0x0;
    uint64_t idx = 0;
    uint32_t sample_x = 0;
    uint32_t sample_y = 0;
    uint32_t col = 0;
    uint32_t row = 0;

    for (sample_y = render->clip_y_lo; sample_y < render->clip_y_hi; sample_y++) { for (sample_x = render->clip_x_lo; sample_x < render->clip_x_hi; sample_x++)
        {
            idx = ((uint64_t)sample_y * render->width) + sample_x;
            dst = render->sample_color + (idx * LDG_TERM_RENDER_RGB_CUNT);
            dst[LDG_TERM_RENDER_RED_IDX] = color->red;
            dst[LDG_TERM_RENDER_GREEN_IDX] = color->green;
            dst[LDG_TERM_RENDER_BLUE_IDX] = color->blue;
            render->sample_depth[idx] = LDG_TERM_RENDER_DEPTH_FAR;
        }
    }

    for (row = render->clip.row; row < render->clip.row + render->clip.row_cunt; row++) { for (col = render->clip.col; col < render->clip.col + render->clip.col_cunt; col++) { render->text_mask[((uint64_t)row * render->col_cunt) + col] = LDG_TRUTH_FALSE; } }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_clear(ldg_term_render_t *render, const ldg_term_render_color_t *color)
{
    uint64_t sample_cunt = 0;
    uint64_t cell_cunt = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!render || !color)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (render_clip_full_is(render) != LDG_TRUTH_TRUE) { return render_clip_clear(render, color); }

    sample_cunt = (uint64_t)render->width * (uint64_t)render->height;
    cell_cunt = (uint64_t)render->col_cunt * (uint64_t)render->row_cunt;

    for (idx = 0; idx < render->width; idx++)
    {
        render->sample_color[(idx * LDG_TERM_RENDER_RGB_CUNT) + LDG_TERM_RENDER_RED_IDX] = color->red;
        render->sample_color[(idx * LDG_TERM_RENDER_RGB_CUNT) + LDG_TERM_RENDER_GREEN_IDX] = color->green;
        render->sample_color[(idx * LDG_TERM_RENDER_RGB_CUNT) + LDG_TERM_RENDER_BLUE_IDX] = color->blue;
        render->sample_depth[idx] = LDG_TERM_RENDER_DEPTH_FAR;
    }

    ret = render_pattern_extend(render->sample_color, (uint64_t)render->width * LDG_TERM_RENDER_RGB_CUNT, sample_cunt * LDG_TERM_RENDER_RGB_CUNT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = render_pattern_extend((uint8_t *)render->sample_depth, (uint64_t)render->width * (uint64_t)sizeof(uint32_t), sample_cunt * (uint64_t)sizeof(uint32_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(render->text_mask, cell_cunt);
}

uint32_t ldg_term_render_depth_test_set(ldg_term_render_t *render, uint8_t en)
{
    if (LDG_UNLIKELY(!render)) { return LDG_ERR_FUNC_ARG_NULL; }

    render->depth_test = (en != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_transform_set(ldg_term_render_t *render, const ldg_term_render_mat4_t *transform)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!render || !transform)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < LDG_TERM_RENDER_HOMOGENEOUS_CUNT * LDG_TERM_RENDER_HOMOGENEOUS_CUNT; idx++) { render->transform.elem[idx] = transform->elem[idx]; }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_light_set(ldg_term_render_t *render, const ldg_term_render_light_t *light)
{
    ldg_term_render_fixed_t len_square = 0;
    ldg_term_render_fixed_t len = 0;
    ldg_term_render_fixed_t term = 0;
    uint32_t axis = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!render || !light)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (axis = 0; axis < LDG_TERM_RENDER_AXIS_CUNT; axis++)
    {
        ret = ldg_term_render_fixed_mul(light->dir[axis], light->dir[axis], &term);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        len_square += term;
    }

    if (LDG_UNLIKELY(len_square == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_term_render_fixed_square_root_get(len_square, &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (axis = 0; axis < LDG_TERM_RENDER_AXIS_CUNT; axis++)
    {
        ret = ldg_term_render_fixed_div(light->dir[axis], len, &render->light.dir[axis]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    render->light.ambient = light->ambient;
    render->light.diffuse = light->diffuse;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_light_en_set(ldg_term_render_t *render, uint8_t en)
{
    if (LDG_UNLIKELY(!render)) { return LDG_ERR_FUNC_ARG_NULL; }

    render->light_en = (en != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_clip_set(ldg_term_render_t *render, const ldg_term_rect_t *clip, uint32_t scroll_col, uint32_t scroll_row)
{
    if (LDG_UNLIKELY(!render || !clip)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY((uint64_t)clip->col + clip->col_cunt > render->col_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY((uint64_t)clip->row + clip->row_cunt > render->row_cunt)) { return LDG_ERR_BOUNDS; }

    render_clip_apply(render, clip, scroll_col, scroll_row);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_clip_rst(ldg_term_render_t *render)
{
    if (LDG_UNLIKELY(!render)) { return LDG_ERR_FUNC_ARG_NULL; }

    render_clip_full_apply(render);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_clip_get(const ldg_term_render_t *render, ldg_term_rect_t *out)
{
    if (LDG_UNLIKELY(!render || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = render->clip;

    return LDG_ERR_AOK;
}

static void render_sample_put(ldg_term_render_t *render, int64_t sample_x, int64_t sample_y, uint32_t depth, int64_t red, int64_t green, int64_t blue)
{
    uint64_t idx = 0;

    if (sample_x < (int64_t)render->clip_x_lo || sample_y < (int64_t)render->clip_y_lo) { return; }

    if (sample_x >= (int64_t)render->clip_x_hi || sample_y >= (int64_t)render->clip_y_hi) { return; }

    idx = ((uint64_t)sample_y * render->width) + (uint64_t)sample_x;

    if (render->depth_test == LDG_TRUTH_TRUE && depth > render->sample_depth[idx]) { return; }

    if (red < 0) { red = 0; }

    if (green < 0) { green = 0; }

    if (blue < 0) { blue = 0; }

    if (red > LDG_TERM_RENDER_COLOR_MAX) { red = LDG_TERM_RENDER_COLOR_MAX; }

    if (green > LDG_TERM_RENDER_COLOR_MAX) { green = LDG_TERM_RENDER_COLOR_MAX; }

    if (blue > LDG_TERM_RENDER_COLOR_MAX) { blue = LDG_TERM_RENDER_COLOR_MAX; }

    render->sample_color[(idx * LDG_TERM_RENDER_RGB_CUNT) + LDG_TERM_RENDER_RED_IDX] = (uint8_t)red;
    render->sample_color[(idx * LDG_TERM_RENDER_RGB_CUNT) + LDG_TERM_RENDER_GREEN_IDX] = (uint8_t)green;
    render->sample_color[(idx * LDG_TERM_RENDER_RGB_CUNT) + LDG_TERM_RENDER_BLUE_IDX] = (uint8_t)blue;
    render->sample_depth[idx] = depth;
}

static int64_t render_fixed_to_sample_get(ldg_term_render_fixed_t val)
{
    return ((int64_t)val + LDG_TERM_RENDER_FIXED_HALF) >> LDG_TERM_RENDER_FIXED_SHIFT;
}

static int64_t render_absolute_get(int64_t val)
{
    return (val < 0) ? -val : val;
}

static void render_sample_st(ldg_term_render_t *render, int64_t sample_x, int64_t sample_y, uint32_t depth, const ldg_term_render_color_t *color)
{
    uint64_t idx = 0;
    uint8_t *dst = 0x0;

    if (sample_x < (int64_t)render->clip_x_lo || sample_y < (int64_t)render->clip_y_lo) { return; }

    if (sample_x >= (int64_t)render->clip_x_hi || sample_y >= (int64_t)render->clip_y_hi) { return; }

    idx = ((uint64_t)sample_y * render->width) + (uint64_t)sample_x;

    if (render->depth_test == LDG_TRUTH_TRUE && depth > render->sample_depth[idx]) { return; }

    dst = render->sample_color + (idx * LDG_TERM_RENDER_RGB_CUNT);
    dst[LDG_TERM_RENDER_RED_IDX] = color->red;
    dst[LDG_TERM_RENDER_GREEN_IDX] = color->green;
    dst[LDG_TERM_RENDER_BLUE_IDX] = color->blue;
    render->sample_depth[idx] = depth;
}

uint32_t ldg_term_render_point_draw(ldg_term_render_t *render, const ldg_term_render_vert2_t *pt)
{
    int64_t orig_x = 0;
    int64_t orig_y = 0;

    if (LDG_UNLIKELY(!render || !pt)) { return LDG_ERR_FUNC_ARG_NULL; }

    render_orig_get(render, &orig_x, &orig_y);
    render_sample_st(render, orig_x + render_fixed_to_sample_get(pt->pos[LDG_TERM_RENDER_X_IDX]), orig_y + render_fixed_to_sample_get(pt->pos[LDG_TERM_RENDER_Y_IDX]), LDG_TERM_RENDER_DEPTH_NEAR, &pt->color);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_line_draw(ldg_term_render_t *render, const ldg_term_render_vert2_t *lhs, const ldg_term_render_vert2_t *rhs)
{
    int64_t x0 = 0;
    int64_t y0 = 0;
    int64_t x1 = 0;
    int64_t y1 = 0;
    int64_t delta_x = 0;
    int64_t delta_y = 0;
    int64_t step_x = 0;
    int64_t step_y = 0;
    int64_t deviation = 0;
    int64_t deviation_double = 0;
    int64_t step_cunt = 0;
    int64_t step = 0;
    int64_t red = 0;
    int64_t green = 0;
    int64_t blue = 0;
    int64_t orig_x = 0;
    int64_t orig_y = 0;

    if (LDG_UNLIKELY(!render || !lhs || !rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    render_orig_get(render, &orig_x, &orig_y);
    x0 = orig_x + render_fixed_to_sample_get(lhs->pos[LDG_TERM_RENDER_X_IDX]);
    y0 = orig_y + render_fixed_to_sample_get(lhs->pos[LDG_TERM_RENDER_Y_IDX]);
    x1 = orig_x + render_fixed_to_sample_get(rhs->pos[LDG_TERM_RENDER_X_IDX]);
    y1 = orig_y + render_fixed_to_sample_get(rhs->pos[LDG_TERM_RENDER_Y_IDX]);

    delta_x = render_absolute_get(x1 - x0);
    delta_y = render_absolute_get(y1 - y0);
    step_x = (x0 < x1) ? LDG_TERM_RENDER_STEP : -LDG_TERM_RENDER_STEP;
    step_y = (y0 < y1) ? LDG_TERM_RENDER_STEP : -LDG_TERM_RENDER_STEP;
    deviation = delta_x - delta_y;
    step_cunt = (delta_x > delta_y) ? delta_x : delta_y;

    for (step = 0; step <= step_cunt; step++)
    {
        if (step_cunt == 0)
        {
            red = lhs->color.red;
            green = lhs->color.green;
            blue = lhs->color.blue;
        }
        else
        {
            red = lhs->color.red + (((int64_t)rhs->color.red - (int64_t)lhs->color.red) * step) / step_cunt;
            green = lhs->color.green + (((int64_t)rhs->color.green - (int64_t)lhs->color.green) * step) / step_cunt;
            blue = lhs->color.blue + (((int64_t)rhs->color.blue - (int64_t)lhs->color.blue) * step) / step_cunt;
        }

        render_sample_put(render, x0, y0, LDG_TERM_RENDER_DEPTH_NEAR, red, green, blue);

        if (x0 == x1 && y0 == y1) { break; }

        deviation_double = deviation * LDG_TERM_RENDER_PLANE_CUNT;

        if (deviation_double > -delta_y)
        {
            deviation -= delta_y;
            x0 += step_x;
        }

        if (deviation_double < delta_x)
        {
            deviation += delta_x;
            y0 += step_y;
        }
    }

    return LDG_ERR_AOK;
}

static int64_t render_edge_get(const int64_t *vert_a, const int64_t *vert_b, int64_t px, int64_t py)
{
    return ((vert_b[LDG_TERM_RENDER_X_IDX] - vert_a[LDG_TERM_RENDER_X_IDX]) * (py - vert_a[LDG_TERM_RENDER_Y_IDX])) - ((vert_b[LDG_TERM_RENDER_Y_IDX] - vert_a[LDG_TERM_RENDER_Y_IDX]) * (px - vert_a[LDG_TERM_RENDER_X_IDX]));
}

static int64_t render_min3_get(int64_t val_a, int64_t val_b, int64_t val_c)
{
    int64_t res = val_a;

    if (val_b < res) { res = val_b; }

    if (val_c < res) { res = val_c; }

    return res;
}

static int64_t render_max3_get(int64_t val_a, int64_t val_b, int64_t val_c)
{
    int64_t res = val_a;

    if (val_b > res) { res = val_b; }

    if (val_c > res) { res = val_c; }

    return res;
}

static uint8_t render_span_narrow(int64_t edge_at_min, int64_t step, int64_t min_x, int64_t *span_lo, int64_t *span_hi)
{
    int64_t bound = 0;

    if (step == 0) { return (edge_at_min >= 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

    if (step > 0)
    {
        if (edge_at_min >= 0) { return LDG_TRUTH_TRUE; }

        bound = min_x + ((-edge_at_min + step - LDG_TERM_RENDER_STEP) / step);

        if (bound > *span_lo) { *span_lo = bound; }

        return LDG_TRUTH_TRUE;
    }

    if (edge_at_min < 0) { return LDG_TRUTH_FALSE; }

    bound = min_x + (edge_at_min / -step);

    if (bound < *span_hi) { *span_hi = bound; }

    return LDG_TRUTH_TRUE;
}

static void render_tri_raster(ldg_term_render_t *render, const render_screen_vert_t *vert_a, const render_screen_vert_t *vert_b, const render_screen_vert_t *vert_c)
{
    const render_screen_vert_t *tmp = 0x0;
    int64_t area = 0;
    int64_t min_x = 0;
    int64_t min_y = 0;
    int64_t max_x = 0;
    int64_t max_y = 0;
    int64_t px = 0;
    int64_t py = 0;
    int64_t sample_x = 0;
    int64_t sample_y = 0;
    int64_t weight_a = 0;
    int64_t weight_b = 0;
    int64_t weight_c = 0;
    int64_t row_a = 0;
    int64_t row_b = 0;
    int64_t row_c = 0;
    int64_t step_a = 0;
    int64_t step_b = 0;
    int64_t step_c = 0;
    int64_t bary_a = 0;
    int64_t bary_b = 0;
    int64_t bary_c = 0;
    int64_t inv_area = 0;
    int64_t span_lo = 0;
    int64_t span_hi = 0;
    int64_t red = 0;
    int64_t green = 0;
    int64_t blue = 0;
    int64_t depth = 0;

    area = render_edge_get(vert_a->pos, vert_b->pos, vert_c->pos[LDG_TERM_RENDER_X_IDX], vert_c->pos[LDG_TERM_RENDER_Y_IDX]);

    if (area == 0) { return; }

    if (area < 0)
    {
        tmp = vert_b;
        vert_b = vert_c;
        vert_c = tmp;
        area = -area;
    }

    inv_area = LDG_TERM_RENDER_INV_AREA_ONE / area;
    step_a = -(vert_c->pos[LDG_TERM_RENDER_Y_IDX] - vert_b->pos[LDG_TERM_RENDER_Y_IDX]) * LDG_TERM_RENDER_SUBPIXEL_ONE;
    step_b = -(vert_a->pos[LDG_TERM_RENDER_Y_IDX] - vert_c->pos[LDG_TERM_RENDER_Y_IDX]) * LDG_TERM_RENDER_SUBPIXEL_ONE;
    step_c = -(vert_b->pos[LDG_TERM_RENDER_Y_IDX] - vert_a->pos[LDG_TERM_RENDER_Y_IDX]) * LDG_TERM_RENDER_SUBPIXEL_ONE;

    min_x = render_min3_get(vert_a->pos[LDG_TERM_RENDER_X_IDX], vert_b->pos[LDG_TERM_RENDER_X_IDX], vert_c->pos[LDG_TERM_RENDER_X_IDX]) >> LDG_TERM_RENDER_SUBPIXEL_SHIFT;
    min_y = render_min3_get(vert_a->pos[LDG_TERM_RENDER_Y_IDX], vert_b->pos[LDG_TERM_RENDER_Y_IDX], vert_c->pos[LDG_TERM_RENDER_Y_IDX]) >> LDG_TERM_RENDER_SUBPIXEL_SHIFT;
    max_x = render_max3_get(vert_a->pos[LDG_TERM_RENDER_X_IDX], vert_b->pos[LDG_TERM_RENDER_X_IDX], vert_c->pos[LDG_TERM_RENDER_X_IDX]) >> LDG_TERM_RENDER_SUBPIXEL_SHIFT;
    max_y = render_max3_get(vert_a->pos[LDG_TERM_RENDER_Y_IDX], vert_b->pos[LDG_TERM_RENDER_Y_IDX], vert_c->pos[LDG_TERM_RENDER_Y_IDX]) >> LDG_TERM_RENDER_SUBPIXEL_SHIFT;

    if (min_x < (int64_t)render->clip_x_lo) { min_x = (int64_t)render->clip_x_lo; }

    if (min_y < (int64_t)render->clip_y_lo) { min_y = (int64_t)render->clip_y_lo; }

    if (max_x >= (int64_t)render->clip_x_hi) { max_x = (int64_t)render->clip_x_hi - LDG_TERM_RENDER_STEP; }

    if (max_y >= (int64_t)render->clip_y_hi) { max_y = (int64_t)render->clip_y_hi - LDG_TERM_RENDER_STEP; }

    px = (min_x << LDG_TERM_RENDER_SUBPIXEL_SHIFT) + LDG_TERM_RENDER_SUBPIXEL_HALF;

    for (sample_y = min_y; sample_y <= max_y; sample_y++)
    {
        py = (sample_y << LDG_TERM_RENDER_SUBPIXEL_SHIFT) + LDG_TERM_RENDER_SUBPIXEL_HALF;

        row_a = render_edge_get(vert_b->pos, vert_c->pos, px, py);
        row_b = render_edge_get(vert_c->pos, vert_a->pos, px, py);
        row_c = render_edge_get(vert_a->pos, vert_b->pos, px, py);

        span_lo = min_x;
        span_hi = max_x;

        if (render_span_narrow(row_a, step_a, min_x, &span_lo, &span_hi) != LDG_TRUTH_TRUE) { continue; }

        if (render_span_narrow(row_b, step_b, min_x, &span_lo, &span_hi) != LDG_TRUTH_TRUE) { continue; }

        if (render_span_narrow(row_c, step_c, min_x, &span_lo, &span_hi) != LDG_TRUTH_TRUE) { continue; }

        if (span_lo > span_hi) { continue; }

        weight_a = row_a + ((span_lo - min_x) * step_a);
        weight_b = row_b + ((span_lo - min_x) * step_b);
        weight_c = row_c + ((span_lo - min_x) * step_c);

        for (sample_x = span_lo; sample_x <= span_hi; sample_x++, weight_a += step_a, weight_b += step_b, weight_c += step_c)
        {
            bary_a = (weight_a * inv_area) >> LDG_TERM_RENDER_BARY_SHIFT;
            bary_b = (weight_b * inv_area) >> LDG_TERM_RENDER_BARY_SHIFT;
            bary_c = (weight_c * inv_area) >> LDG_TERM_RENDER_BARY_SHIFT;

            red = ((bary_a * vert_a->color[LDG_TERM_RENDER_RED_IDX]) + (bary_b * vert_b->color[LDG_TERM_RENDER_RED_IDX]) + (bary_c * vert_c->color[LDG_TERM_RENDER_RED_IDX])) >> LDG_TERM_RENDER_BARY_ONE_SHIFT;
            green = ((bary_a * vert_a->color[LDG_TERM_RENDER_GREEN_IDX]) + (bary_b * vert_b->color[LDG_TERM_RENDER_GREEN_IDX]) + (bary_c * vert_c->color[LDG_TERM_RENDER_GREEN_IDX])) >> LDG_TERM_RENDER_BARY_ONE_SHIFT;
            blue = ((bary_a * vert_a->color[LDG_TERM_RENDER_BLUE_IDX]) + (bary_b * vert_b->color[LDG_TERM_RENDER_BLUE_IDX]) + (bary_c * vert_c->color[LDG_TERM_RENDER_BLUE_IDX])) >> LDG_TERM_RENDER_BARY_ONE_SHIFT;
            depth = ((bary_a * (int64_t)vert_a->depth) + (bary_b * (int64_t)vert_b->depth) + (bary_c * (int64_t)vert_c->depth)) >> LDG_TERM_RENDER_BARY_ONE_SHIFT;

            if (depth > (int64_t)LDG_TERM_RENDER_DEPTH_FAR) { depth = LDG_TERM_RENDER_DEPTH_FAR; }

            render_sample_put(render, sample_x, sample_y, (uint32_t)depth, red, green, blue);
        }
    }
}

static void render_screen_vert_from_vert2_set(const ldg_term_render_t *render, const ldg_term_render_vert2_t *src, render_screen_vert_t *out)
{
    int64_t orig_x = 0;
    int64_t orig_y = 0;

    render_orig_get(render, &orig_x, &orig_y);
    out->pos[LDG_TERM_RENDER_X_IDX] = (orig_x * LDG_TERM_RENDER_SUBPIXEL_ONE) + ((int64_t)src->pos[LDG_TERM_RENDER_X_IDX] >> LDG_TERM_RENDER_FIXED_TO_SUBPIXEL_SHIFT);
    out->pos[LDG_TERM_RENDER_Y_IDX] = (orig_y * LDG_TERM_RENDER_SUBPIXEL_ONE) + ((int64_t)src->pos[LDG_TERM_RENDER_Y_IDX] >> LDG_TERM_RENDER_FIXED_TO_SUBPIXEL_SHIFT);
    out->color[LDG_TERM_RENDER_RED_IDX] = src->color.red;
    out->color[LDG_TERM_RENDER_GREEN_IDX] = src->color.green;
    out->color[LDG_TERM_RENDER_BLUE_IDX] = src->color.blue;
    out->depth = LDG_TERM_RENDER_DEPTH_NEAR;
    out->pudding = 0;
}

uint32_t ldg_term_render_tri_draw(ldg_term_render_t *render, const ldg_term_render_vert2_t *vert_a, const ldg_term_render_vert2_t *vert_b, const ldg_term_render_vert2_t *vert_c)
{
    render_screen_vert_t screen_a = LDG_STRUCT_ZERO_INIT;
    render_screen_vert_t screen_b = LDG_STRUCT_ZERO_INIT;
    render_screen_vert_t screen_c = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!render || !vert_a || !vert_b || !vert_c)) { return LDG_ERR_FUNC_ARG_NULL; }

    render_screen_vert_from_vert2_set(render, vert_a, &screen_a);
    render_screen_vert_from_vert2_set(render, vert_b, &screen_b);
    render_screen_vert_from_vert2_set(render, vert_c, &screen_c);

    render_tri_raster(render, &screen_a, &screen_b, &screen_c);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_rect_fill(ldg_term_render_t *render, const ldg_term_render_fixed_t *pos, const ldg_term_render_fixed_t *extent, const ldg_term_render_color_t *color)
{
    int64_t x0 = 0;
    int64_t y0 = 0;
    int64_t x1 = 0;
    int64_t y1 = 0;
    int64_t sample_x = 0;
    int64_t sample_y = 0;
    int64_t orig_x = 0;
    int64_t orig_y = 0;

    if (LDG_UNLIKELY(!render || !pos || !extent || !color)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(extent[LDG_TERM_RENDER_X_IDX] < 0 || extent[LDG_TERM_RENDER_Y_IDX] < 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    render_orig_get(render, &orig_x, &orig_y);
    x0 = orig_x + render_fixed_to_sample_get(pos[LDG_TERM_RENDER_X_IDX]);
    y0 = orig_y + render_fixed_to_sample_get(pos[LDG_TERM_RENDER_Y_IDX]);
    x1 = orig_x + render_fixed_to_sample_get(pos[LDG_TERM_RENDER_X_IDX] + extent[LDG_TERM_RENDER_X_IDX]);
    y1 = orig_y + render_fixed_to_sample_get(pos[LDG_TERM_RENDER_Y_IDX] + extent[LDG_TERM_RENDER_Y_IDX]);

    if (x0 < (int64_t)render->clip_x_lo) { x0 = (int64_t)render->clip_x_lo; }

    if (y0 < (int64_t)render->clip_y_lo) { y0 = (int64_t)render->clip_y_lo; }

    if (x1 > (int64_t)render->clip_x_hi) { x1 = (int64_t)render->clip_x_hi; }

    if (y1 > (int64_t)render->clip_y_hi) { y1 = (int64_t)render->clip_y_hi; }

    for (sample_y = y0; sample_y < y1; sample_y++) { for (sample_x = x0; sample_x < x1; sample_x++) { render_sample_st(render, sample_x, sample_y, LDG_TERM_RENDER_DEPTH_NEAR, color); } }

    return LDG_ERR_AOK;
}

static void render_affine2_apply(const ldg_term_render_affine2_t *xf, int64_t pos_x, int64_t pos_y, int64_t *out_x, int64_t *out_y)
{
    *out_x = ((((int64_t)xf->elem[LDG_TERM_RENDER_AFFINE2_A_IDX] * pos_x) + ((int64_t)xf->elem[LDG_TERM_RENDER_AFFINE2_C_IDX] * pos_y)) / LDG_TERM_RENDER_FIXED_ONE) + xf->elem[LDG_TERM_RENDER_AFFINE2_E_IDX];
    *out_y = ((((int64_t)xf->elem[LDG_TERM_RENDER_AFFINE2_B_IDX] * pos_x) + ((int64_t)xf->elem[LDG_TERM_RENDER_AFFINE2_D_IDX] * pos_y)) / LDG_TERM_RENDER_FIXED_ONE) + xf->elem[LDG_TERM_RENDER_AFFINE2_F_IDX];
}

static void render_affine2_inv_apply(const int64_t *inv, int64_t pos_x, int64_t pos_y, int64_t *out_x, int64_t *out_y)
{
    *out_x = (((inv[LDG_TERM_RENDER_AFFINE2_A_IDX] * pos_x) + (inv[LDG_TERM_RENDER_AFFINE2_C_IDX] * pos_y)) / LDG_TERM_RENDER_FIXED_ONE) + inv[LDG_TERM_RENDER_AFFINE2_E_IDX];
    *out_y = (((inv[LDG_TERM_RENDER_AFFINE2_B_IDX] * pos_x) + (inv[LDG_TERM_RENDER_AFFINE2_D_IDX] * pos_y)) / LDG_TERM_RENDER_FIXED_ONE) + inv[LDG_TERM_RENDER_AFFINE2_F_IDX];
}

static uint8_t render_affine2_inv_get(const ldg_term_render_affine2_t *xf, int64_t *out)
{
    int64_t elem_a = xf->elem[LDG_TERM_RENDER_AFFINE2_A_IDX];
    int64_t elem_b = xf->elem[LDG_TERM_RENDER_AFFINE2_B_IDX];
    int64_t elem_c = xf->elem[LDG_TERM_RENDER_AFFINE2_C_IDX];
    int64_t elem_d = xf->elem[LDG_TERM_RENDER_AFFINE2_D_IDX];
    int64_t elem_e = xf->elem[LDG_TERM_RENDER_AFFINE2_E_IDX];
    int64_t elem_f = xf->elem[LDG_TERM_RENDER_AFFINE2_F_IDX];
    int64_t det = 0;

    det = ((elem_a * elem_d) - (elem_b * elem_c)) / LDG_TERM_RENDER_FIXED_ONE;

    if (det == 0) { return LDG_TRUTH_FALSE; }

    out[LDG_TERM_RENDER_AFFINE2_A_IDX] = (elem_d * LDG_TERM_RENDER_FIXED_ONE) / det;
    out[LDG_TERM_RENDER_AFFINE2_B_IDX] = -(elem_b * LDG_TERM_RENDER_FIXED_ONE) / det;
    out[LDG_TERM_RENDER_AFFINE2_C_IDX] = -(elem_c * LDG_TERM_RENDER_FIXED_ONE) / det;
    out[LDG_TERM_RENDER_AFFINE2_D_IDX] = (elem_a * LDG_TERM_RENDER_FIXED_ONE) / det;
    out[LDG_TERM_RENDER_AFFINE2_E_IDX] = -(((out[LDG_TERM_RENDER_AFFINE2_A_IDX] * elem_e) + (out[LDG_TERM_RENDER_AFFINE2_C_IDX] * elem_f)) / LDG_TERM_RENDER_FIXED_ONE);
    out[LDG_TERM_RENDER_AFFINE2_F_IDX] = -(((out[LDG_TERM_RENDER_AFFINE2_B_IDX] * elem_e) + (out[LDG_TERM_RENDER_AFFINE2_D_IDX] * elem_f)) / LDG_TERM_RENDER_FIXED_ONE);

    return LDG_TRUTH_TRUE;
}

static uint64_t render_isqrt_get(uint64_t val)
{
    uint64_t res = 0;
    uint64_t bit = LDG_TERM_RENDER_SQRT_BIT_START;

    while (bit > val) { bit >>= LDG_TERM_RENDER_SQRT_BIT_STEP; }

    while (bit != 0)
    {
        if (val >= res + bit)
        {
            val -= res + bit;
            res = (res >> LDG_TERM_RENDER_STEP) + bit;
        }
        else { res >>= LDG_TERM_RENDER_STEP; }

        bit >>= LDG_TERM_RENDER_SQRT_BIT_STEP;
    }

    return res;
}

static uint8_t render_channel_lerp_get(uint8_t lo, uint8_t hi, int64_t weight)
{
    return (uint8_t)((int64_t)lo + ((((int64_t)hi - (int64_t)lo) * weight) / LDG_TERM_RENDER_GRAD_LERP_ONE));
}

static void render_grad_color_get(const ldg_term_render_paint_t *paint, int64_t off, ldg_term_render_color_t *out)
{
    const ldg_term_render_grad_stop_t *lo = 0x0;
    const ldg_term_render_grad_stop_t *hi = 0x0;
    int64_t span = 0;
    int64_t weight = 0;
    uint32_t idx = 0;

    if (off <= paint->stop[0].off)
    {
        *out = paint->stop[0].color;
        return;
    }

    for (idx = LDG_TERM_RENDER_STEP; idx < paint->stop_cunt; idx++)
    {
        if (off > paint->stop[idx].off) { continue; }

        lo = &paint->stop[idx - LDG_TERM_RENDER_STEP];
        hi = &paint->stop[idx];
        span = (int64_t)hi->off - (int64_t)lo->off;
        weight = (span <= 0) ? LDG_TERM_RENDER_GRAD_LERP_ONE : ((off - (int64_t)lo->off) * LDG_TERM_RENDER_GRAD_LERP_ONE) / span;

        out->red = render_channel_lerp_get(lo->color.red, hi->color.red, weight);
        out->green = render_channel_lerp_get(lo->color.green, hi->color.green, weight);
        out->blue = render_channel_lerp_get(lo->color.blue, hi->color.blue, weight);
        out->pudding[0] = 0;
        return;
    }

    *out = paint->stop[paint->stop_cunt - LDG_TERM_RENDER_STEP].color;
}

static uint8_t render_channel_bright_get(uint8_t val, uint32_t bright)
{
    uint64_t scaled = ((uint64_t)val * bright) / LDG_TERM_RENDER_PER_MILLE;

    if (scaled > LDG_TERM_RENDER_COLOR_MAX) { scaled = LDG_TERM_RENDER_COLOR_MAX; }

    return (uint8_t)scaled;
}

static void render_paint_color_get(const ldg_term_render_paint_t *paint, const int64_t *inv, int64_t pos_x, int64_t pos_y, ldg_term_render_color_t *out)
{
    int64_t paint_x = 0;
    int64_t paint_y = 0;
    int64_t delta_x = 0;
    int64_t delta_y = 0;
    int64_t dist = 0;

    *out = paint->color;

    if (paint->kind == LDG_TERM_RENDER_PAINT_RADIAL)
    {
        render_affine2_inv_apply(inv, pos_x, pos_y, &paint_x, &paint_y);
        delta_x = paint_x - paint->center[LDG_TERM_RENDER_X_IDX];
        delta_y = paint_y - paint->center[LDG_TERM_RENDER_Y_IDX];
        dist = (int64_t)render_isqrt_get((uint64_t)((delta_x * delta_x) + (delta_y * delta_y)));
        render_grad_color_get(paint, (dist * LDG_TERM_RENDER_FIXED_ONE) / paint->radius, out);
    }

    out->red = render_channel_bright_get(out->red, paint->bright);
    out->green = render_channel_bright_get(out->green, paint->bright);
    out->blue = render_channel_bright_get(out->blue, paint->bright);
}

static uint32_t render_poly_row_cross(ldg_term_render_t *render, const ldg_term_render_fixed_t *pt, const uint32_t *contour_len, uint32_t contour_cunt, const ldg_term_render_affine2_t *xf, int64_t orig_x, int64_t orig_y, int64_t pos_y, uint32_t *out_cunt)
{
    int64_t x0 = 0;
    int64_t y0 = 0;
    int64_t x1 = 0;
    int64_t y1 = 0;
    int64_t cross_x = 0;
    int64_t cross_hi = 0;
    int64_t dir = 0;
    uint64_t base = 0;
    uint64_t curr = 0;
    uint64_t next = 0;
    uint32_t contour = 0;
    uint32_t idx = 0;
    uint32_t cunt = 0;
    uint32_t probe = 0;

    *out_cunt = 0;
    cross_hi = ((int64_t)render->clip_x_hi + LDG_TERM_RENDER_STEP) * LDG_TERM_RENDER_FIXED_ONE;

    for (contour = 0; contour < contour_cunt; contour++)
    {
        if (contour_len[contour] >= LDG_TERM_RENDER_CONTOUR_EDGE_MIN)
        {
            for (idx = 0; idx < contour_len[contour]; idx++)
            {
                curr = (base + idx) * LDG_TERM_RENDER_PLANE_CUNT;
                next = (base + ((idx + LDG_TERM_RENDER_STEP) % contour_len[contour])) * LDG_TERM_RENDER_PLANE_CUNT;

                render_affine2_apply(xf, pt[curr + LDG_TERM_RENDER_X_IDX], pt[curr + LDG_TERM_RENDER_Y_IDX], &x0, &y0);
                render_affine2_apply(xf, pt[next + LDG_TERM_RENDER_X_IDX], pt[next + LDG_TERM_RENDER_Y_IDX], &x1, &y1);
                x0 += orig_x;
                y0 += orig_y;
                x1 += orig_x;
                y1 += orig_y;

                if (y0 <= pos_y && pos_y < y1) { dir = LDG_TERM_RENDER_CROSS_DIR_UP; }
                else if (y1 <= pos_y && pos_y < y0) { dir = LDG_TERM_RENDER_CROSS_DIR_DOWN; }
                else { continue; }

                if (LDG_UNLIKELY(cunt >= render->cross_max)) { return LDG_ERR_FULL; }

                cross_x = x0 + (((pos_y - y0) * (x1 - x0)) / (y1 - y0));

                if (cross_x < 0) { cross_x = 0; }

                if (cross_x > cross_hi) { cross_x = cross_hi; }

                render->cross_buff[cunt] = (cross_x * LDG_TERM_RENDER_CROSS_PAIR) + dir;
                probe = cunt;

                while (probe > 0 && render->cross_buff[probe - LDG_TERM_RENDER_STEP] > render->cross_buff[probe])
                {
                    cross_x = render->cross_buff[probe];
                    render->cross_buff[probe] = render->cross_buff[probe - LDG_TERM_RENDER_STEP];
                    render->cross_buff[probe - LDG_TERM_RENDER_STEP] = cross_x;
                    probe--;
                }

                cunt++;
            }
        }

        base += contour_len[contour];
    }

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}

static int64_t render_span_edge_get(int64_t cross_x)
{
    if (cross_x <= LDG_TERM_RENDER_FIXED_HALF) { return LDG_TERM_RENDER_SAMPLE_FIRST; }

    return (cross_x - LDG_TERM_RENDER_FIXED_HALF + LDG_TERM_RENDER_FIXED_ONE - LDG_TERM_RENDER_STEP) / LDG_TERM_RENDER_FIXED_ONE;
}

static void render_poly_row_span(ldg_term_render_t *render, uint32_t cross_cunt, uint32_t rule, const ldg_term_render_paint_t *paint, const int64_t *inv, int64_t orig_x, int64_t orig_y, int64_t sample_y)
{
    ldg_term_render_color_t color = LDG_STRUCT_ZERO_INIT;
    int64_t winding = 0;
    int64_t span_lo = 0;
    int64_t span_hi = 0;
    int64_t sample_x = 0;
    int64_t pos_y = 0;
    uint32_t idx = 0;
    uint8_t inside = LDG_TRUTH_FALSE;

    pos_y = (sample_y * LDG_TERM_RENDER_FIXED_ONE) + LDG_TERM_RENDER_FIXED_HALF;

    for (idx = 0; idx + LDG_TERM_RENDER_STEP < cross_cunt; idx++)
    {
        winding += ((render->cross_buff[idx] % LDG_TERM_RENDER_CROSS_PAIR) == LDG_TERM_RENDER_CROSS_DIR_UP) ? LDG_TERM_RENDER_STEP : -LDG_TERM_RENDER_STEP;

        if (rule == LDG_TERM_RENDER_FILL_RULE_NONZERO) { inside = (winding != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }
        else { inside = ((winding % LDG_TERM_RENDER_WINDING_PARITY) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

        if (inside != LDG_TRUTH_TRUE) { continue; }

        span_lo = render_span_edge_get(render->cross_buff[idx] / LDG_TERM_RENDER_CROSS_PAIR);
        span_hi = render_span_edge_get(render->cross_buff[idx + LDG_TERM_RENDER_STEP] / LDG_TERM_RENDER_CROSS_PAIR);

        if (span_lo < (int64_t)render->clip_x_lo) { span_lo = (int64_t)render->clip_x_lo; }

        if (span_hi > (int64_t)render->clip_x_hi) { span_hi = (int64_t)render->clip_x_hi; }

        for (sample_x = span_lo; sample_x < span_hi; sample_x++)
        {
            render_paint_color_get(paint, inv, (sample_x * LDG_TERM_RENDER_FIXED_ONE) + LDG_TERM_RENDER_FIXED_HALF - orig_x, pos_y - orig_y, &color);
            render_sample_st(render, sample_x, sample_y, LDG_TERM_RENDER_DEPTH_NEAR, &color);
        }
    }
}

static uint8_t render_paint_valid_is(const ldg_term_render_paint_t *paint)
{
    if (paint->kind == LDG_TERM_RENDER_PAINT_SOLID) { return LDG_TRUTH_TRUE; }

    if (paint->kind != LDG_TERM_RENDER_PAINT_RADIAL) { return LDG_TRUTH_FALSE; }

    if (paint->radius <= 0) { return LDG_TRUTH_FALSE; }

    if (paint->stop_cunt == 0 || paint->stop_cunt > LDG_TERM_RENDER_GRAD_STOP_MAX) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static int64_t render_row_get(int64_t pos_y)
{
    if (pos_y < 0) { return LDG_TERM_RENDER_SAMPLE_FIRST; }

    return pos_y / LDG_TERM_RENDER_FIXED_ONE;
}

uint32_t ldg_term_render_poly_fill(ldg_term_render_t *render, const ldg_term_render_fixed_t *pt, const uint32_t *contour_len, uint32_t contour_cunt, uint32_t rule, const ldg_term_render_affine2_t *xf, const ldg_term_render_paint_t *paint)
{
    ldg_term_render_affine2_t ident = LDG_STRUCT_ZERO_INIT;
    const ldg_term_render_affine2_t *map = xf;
    int64_t inv[LDG_TERM_RENDER_AFFINE2_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t orig_x = 0;
    int64_t orig_y = 0;
    int64_t pos_x = 0;
    int64_t pos_y = 0;
    int64_t y_min = INT64_MAX;
    int64_t y_max = INT64_MIN;
    int64_t row_lo = 0;
    int64_t row_hi = 0;
    int64_t sample_y = 0;
    uint64_t pt_cunt = 0;
    uint64_t idx = 0;
    uint32_t contour = 0;
    uint32_t cross_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!render || !contour_len || !paint)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pt && contour_cunt != 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(rule != LDG_TERM_RENDER_FILL_RULE_NONZERO && rule != LDG_TERM_RENDER_FILL_RULE_EVEN_ODD)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(render_paint_valid_is(paint) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (!map)
    {
        ident.elem[LDG_TERM_RENDER_AFFINE2_A_IDX] = LDG_TERM_RENDER_FIXED_ONE;
        ident.elem[LDG_TERM_RENDER_AFFINE2_D_IDX] = LDG_TERM_RENDER_FIXED_ONE;
        map = &ident;
    }

    if (paint->kind == LDG_TERM_RENDER_PAINT_RADIAL && LDG_UNLIKELY(render_affine2_inv_get(map, inv) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    render_orig_get(render, &orig_x, &orig_y);
    orig_x *= LDG_TERM_RENDER_FIXED_ONE;
    orig_y *= LDG_TERM_RENDER_FIXED_ONE;

    for (contour = 0; contour < contour_cunt; contour++) { pt_cunt += contour_len[contour]; }

    if (pt_cunt == 0) { return LDG_ERR_AOK; }

    for (idx = 0; idx < pt_cunt; idx++)
    {
        render_affine2_apply(map, pt[(idx * LDG_TERM_RENDER_PLANE_CUNT) + LDG_TERM_RENDER_X_IDX], pt[(idx * LDG_TERM_RENDER_PLANE_CUNT) + LDG_TERM_RENDER_Y_IDX], &pos_x, &pos_y);
        pos_y += orig_y;

        if (pos_y < y_min) { y_min = pos_y; }

        if (pos_y > y_max) { y_max = pos_y; }
    }

    row_lo = render_row_get(y_min);
    row_hi = render_row_get(y_max) + LDG_TERM_RENDER_STEP;

    if (row_lo < (int64_t)render->clip_y_lo) { row_lo = (int64_t)render->clip_y_lo; }

    if (row_hi > (int64_t)render->clip_y_hi) { row_hi = (int64_t)render->clip_y_hi; }

    for (sample_y = row_lo; sample_y < row_hi; sample_y++)
    {
        ret = render_poly_row_cross(render, pt, contour_len, contour_cunt, map, orig_x, orig_y, (sample_y * LDG_TERM_RENDER_FIXED_ONE) + LDG_TERM_RENDER_FIXED_HALF, &cross_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        render_poly_row_span(render, cross_cunt, rule, paint, inv, orig_x, orig_y, sample_y);
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_text_draw(ldg_term_render_t *render, uint32_t col, uint32_t row, const char *text, uint64_t len, const ldg_term_render_color_t *fg, const ldg_term_render_color_t *bg)
{
    uint64_t idx = 0;
    uint64_t local_col = 0;
    uint64_t canvas_col = 0;
    uint64_t canvas_row = 0;

    if (LDG_UNLIKELY(!render || !text || !fg || !bg)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (row < render->scroll_row) { return LDG_ERR_AOK; }

    canvas_row = (uint64_t)render->clip.row + row - render->scroll_row;

    if (canvas_row >= (uint64_t)render->clip.row + render->clip.row_cunt) { return LDG_ERR_AOK; }

    for (idx = 0; idx < len; idx++)
    {
        local_col = (uint64_t)col + idx;

        if (local_col < render->scroll_col) { continue; }

        canvas_col = (uint64_t)render->clip.col + local_col - render->scroll_col;

        if (canvas_col >= (uint64_t)render->clip.col + render->clip.col_cunt) { break; }

        render_text_cell_put(render, (uint32_t)canvas_col, (uint32_t)canvas_row, (uint8_t)text[idx], fg, bg);
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_frame_draw(ldg_term_render_t *render, const char *title, uint64_t len, const ldg_term_render_frame_style_t *style)
{
    const ldg_term_render_color_t *top = 0x0;
    const ldg_term_render_color_t *bottom = 0x0;
    const ldg_term_render_color_t *lhs = 0x0;
    const ldg_term_render_color_t *rhs = 0x0;
    uint64_t idx = 0;
    uint32_t col_lo = 0;
    uint32_t col_hi = 0;
    uint32_t row_lo = 0;
    uint32_t row_hi = 0;
    uint32_t col = 0;
    uint32_t row = 0;

    if (LDG_UNLIKELY(!render || !style)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!title && len != 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY((style->accent_edge & ~LDG_TERM_RENDER_FRAME_EDGE_MASK) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(render->clip.col_cunt < LDG_TERM_RENDER_FRAME_EDGE_CUNT || render->clip.row_cunt < LDG_TERM_RENDER_FRAME_EDGE_CUNT)) { return LDG_ERR_BOUNDS; }

    top = ((style->accent_edge & LDG_TERM_RENDER_FRAME_EDGE_TOP) != 0) ? &style->accent : &style->border;
    bottom = ((style->accent_edge & LDG_TERM_RENDER_FRAME_EDGE_BOTTOM) != 0) ? &style->accent : &style->border;
    lhs = ((style->accent_edge & LDG_TERM_RENDER_FRAME_EDGE_LHS) != 0) ? &style->accent : &style->border;
    rhs = ((style->accent_edge & LDG_TERM_RENDER_FRAME_EDGE_RHS) != 0) ? &style->accent : &style->border;

    col_lo = render->clip.col;
    row_lo = render->clip.row;
    col_hi = col_lo + render->clip.col_cunt - LDG_TERM_RENDER_STEP;
    row_hi = row_lo + render->clip.row_cunt - LDG_TERM_RENDER_STEP;

    for (col = col_lo + LDG_TERM_RENDER_STEP; col < col_hi; col++)
    {
        render_text_cell_put(render, col, row_lo, LDG_TERM_RENDER_FRAME_HORIZONTAL, top, &style->bg);
        render_text_cell_put(render, col, row_hi, LDG_TERM_RENDER_FRAME_HORIZONTAL, bottom, &style->bg);
    }

    for (row = row_lo + LDG_TERM_RENDER_STEP; row < row_hi; row++)
    {
        render_text_cell_put(render, col_lo, row, LDG_TERM_RENDER_FRAME_VERTICAL, lhs, &style->bg);
        render_text_cell_put(render, col_hi, row, LDG_TERM_RENDER_FRAME_VERTICAL, rhs, &style->bg);
    }

    render_text_cell_put(render, col_lo, row_lo, LDG_TERM_RENDER_FRAME_CORNER, (top == &style->accent || lhs == &style->accent) ? &style->accent : &style->border, &style->bg);
    render_text_cell_put(render, col_hi, row_lo, LDG_TERM_RENDER_FRAME_CORNER, (top == &style->accent || rhs == &style->accent) ? &style->accent : &style->border, &style->bg);
    render_text_cell_put(render, col_lo, row_hi, LDG_TERM_RENDER_FRAME_CORNER, (bottom == &style->accent || lhs == &style->accent) ? &style->accent : &style->border, &style->bg);
    render_text_cell_put(render, col_hi, row_hi, LDG_TERM_RENDER_FRAME_CORNER, (bottom == &style->accent || rhs == &style->accent) ? &style->accent : &style->border, &style->bg);

    for (idx = 0; idx < len; idx++)
    {
        col = col_lo + LDG_TERM_RENDER_STEP + (uint32_t)idx;

        if (idx >= (uint64_t)render->clip.col_cunt - LDG_TERM_RENDER_FRAME_EDGE_CUNT) { break; }

        render_text_cell_put(render, col, row_lo, (uint8_t)title[idx], &style->title_fg, &style->title_bg);
    }

    return LDG_ERR_AOK;
}

static int64_t render_clamp_get(int64_t val, int64_t min, int64_t max)
{
    if (val < min) { return min; }

    if (val > max) { return max; }

    return val;
}

static int64_t render_channel_lit_get(int64_t base, uint8_t ambient, uint8_t diffuse, int64_t dot)
{
    int64_t intensity = 0;

    intensity = (int64_t)ambient + (((int64_t)diffuse * dot) >> LDG_TERM_RENDER_FIXED_SHIFT);

    return render_clamp_get((base * intensity) / LDG_TERM_RENDER_COLOR_MAX, LDG_TERM_RENDER_COLOR_MIN, LDG_TERM_RENDER_COLOR_MAX);
}

static void render_vert3_transform(const ldg_term_render_t *render, const ldg_term_render_vert3_t *src, ldg_term_render_clip_vert_t *out)
{
    int64_t in[LDG_TERM_RENDER_HOMOGENEOUS_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t cum = 0;
    int64_t dot = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t axis = 0;

    in[LDG_TERM_RENDER_X_IDX] = src->pos[LDG_TERM_RENDER_X_IDX];
    in[LDG_TERM_RENDER_Y_IDX] = src->pos[LDG_TERM_RENDER_Y_IDX];
    in[LDG_TERM_RENDER_Z_IDX] = src->pos[LDG_TERM_RENDER_Z_IDX];
    in[LDG_TERM_RENDER_W_IDX] = LDG_TERM_RENDER_FIXED_ONE;

    for (row = 0; row < LDG_TERM_RENDER_HOMOGENEOUS_CUNT; row++)
    {
        cum = 0;

        for (col = 0; col < LDG_TERM_RENDER_HOMOGENEOUS_CUNT; col++) { cum += (int64_t)render->transform.elem[LDG_TERM_RENDER_ELEM(row, col)] * in[col]; }

        out->pos[row] = render_clamp_get(cum >> LDG_TERM_RENDER_FIXED_SHIFT, LDG_TERM_RENDER_CLIP_COORD_MIN, LDG_TERM_RENDER_CLIP_COORD_MAX);
    }

    out->color[LDG_TERM_RENDER_RED_IDX] = src->color.red;
    out->color[LDG_TERM_RENDER_GREEN_IDX] = src->color.green;
    out->color[LDG_TERM_RENDER_BLUE_IDX] = src->color.blue;
    out->pudding = 0;

    if (render->light_en != LDG_TRUTH_TRUE) { return; }

    dot = 0;

    for (axis = 0; axis < LDG_TERM_RENDER_AXIS_CUNT; axis++) { dot += (int64_t)src->normal[axis] * (int64_t)render->light.dir[axis]; }

    dot >>= LDG_TERM_RENDER_FIXED_SHIFT;

    if (dot < 0) { dot = 0; }

    if (dot > LDG_TERM_RENDER_FIXED_ONE) { dot = LDG_TERM_RENDER_FIXED_ONE; }

    out->color[LDG_TERM_RENDER_RED_IDX] = render_channel_lit_get(out->color[LDG_TERM_RENDER_RED_IDX], render->light.ambient.red, render->light.diffuse.red, dot);
    out->color[LDG_TERM_RENDER_GREEN_IDX] = render_channel_lit_get(out->color[LDG_TERM_RENDER_GREEN_IDX], render->light.ambient.green, render->light.diffuse.green, dot);
    out->color[LDG_TERM_RENDER_BLUE_IDX] = render_channel_lit_get(out->color[LDG_TERM_RENDER_BLUE_IDX], render->light.ambient.blue, render->light.diffuse.blue, dot);
}

static void render_clip_vert_lerp(const ldg_term_render_clip_vert_t *lhs, const ldg_term_render_clip_vert_t *rhs, int64_t numerator, int64_t denominator, ldg_term_render_clip_vert_t *out)
{
    uint32_t idx = 0;

    for (idx = 0; idx < LDG_TERM_RENDER_HOMOGENEOUS_CUNT; idx++) { out->pos[idx] = lhs->pos[idx] + (((rhs->pos[idx] - lhs->pos[idx]) * numerator) / denominator); }

    for (idx = 0; idx < LDG_TERM_RENDER_RGB_CUNT; idx++) { out->color[idx] = lhs->color[idx] + (((rhs->color[idx] - lhs->color[idx]) * numerator) / denominator); }

    out->pudding = 0;
}

static void render_screen_vert_from_clip_set(const ldg_term_render_t *render, const ldg_term_render_clip_vert_t *src, render_screen_vert_t *out)
{
    int64_t ndc_x = 0;
    int64_t ndc_y = 0;
    int64_t ndc_z = 0;
    int64_t depth = 0;
    int64_t divisor = 0;

    divisor = src->pos[LDG_TERM_RENDER_W_IDX];

    if (divisor <= 0) { divisor = LDG_TERM_RENDER_STEP; }

    ndc_x = render_clamp_get((src->pos[LDG_TERM_RENDER_X_IDX] << LDG_TERM_RENDER_FIXED_SHIFT) / divisor, LDG_TERM_RENDER_NDC_GUARD_LOW, LDG_TERM_RENDER_NDC_GUARD);
    ndc_y = render_clamp_get((src->pos[LDG_TERM_RENDER_Y_IDX] << LDG_TERM_RENDER_FIXED_SHIFT) / divisor, LDG_TERM_RENDER_NDC_GUARD_LOW, LDG_TERM_RENDER_NDC_GUARD);
    ndc_z = render_clamp_get((src->pos[LDG_TERM_RENDER_Z_IDX] << LDG_TERM_RENDER_FIXED_SHIFT) / divisor, LDG_TERM_RENDER_NDC_NEAR, LDG_TERM_RENDER_NDC_FAR);

    out->pos[LDG_TERM_RENDER_X_IDX] = ((int64_t)render->clip_x_lo * LDG_TERM_RENDER_SUBPIXEL_ONE) + (((ndc_x + LDG_TERM_RENDER_FIXED_ONE) * ((int64_t)render->clip_x_hi - (int64_t)render->clip_x_lo)) >> LDG_TERM_RENDER_NDC_TO_SUBPIXEL_SHIFT);
    out->pos[LDG_TERM_RENDER_Y_IDX] = ((int64_t)render->clip_y_lo * LDG_TERM_RENDER_SUBPIXEL_ONE) + (((LDG_TERM_RENDER_FIXED_ONE - ndc_y) * ((int64_t)render->clip_y_hi - (int64_t)render->clip_y_lo)) >> LDG_TERM_RENDER_NDC_TO_SUBPIXEL_SHIFT);

    depth = (ndc_z + LDG_TERM_RENDER_FIXED_ONE) << LDG_TERM_RENDER_DEPTH_SCALE_SHIFT;
    depth = render_clamp_get(depth, LDG_TERM_RENDER_DEPTH_NEAR + LDG_TERM_RENDER_STEP, (int64_t)LDG_TERM_RENDER_DEPTH_FAR - LDG_TERM_RENDER_STEP);

    out->depth = (uint32_t)depth;
    out->color[LDG_TERM_RENDER_RED_IDX] = src->color[LDG_TERM_RENDER_RED_IDX];
    out->color[LDG_TERM_RENDER_GREEN_IDX] = src->color[LDG_TERM_RENDER_GREEN_IDX];
    out->color[LDG_TERM_RENDER_BLUE_IDX] = src->color[LDG_TERM_RENDER_BLUE_IDX];
    out->pudding = 0;
}

static void render_clip_tri_draw(ldg_term_render_t *render, const ldg_term_render_clip_vert_t *vert_a, const ldg_term_render_clip_vert_t *vert_b, const ldg_term_render_clip_vert_t *vert_c)
{
    const ldg_term_render_clip_vert_t *in[LDG_TERM_RENDER_TRI_VERT_CUNT] = LDG_ARR_ZERO_INIT;
    ldg_term_render_clip_vert_t out[LDG_TERM_RENDER_CLIP_VERT_MAX] = LDG_ARR_ZERO_INIT;
    render_screen_vert_t screen_a = LDG_STRUCT_ZERO_INIT;
    render_screen_vert_t screen_b = LDG_STRUCT_ZERO_INIT;
    render_screen_vert_t screen_c = LDG_STRUCT_ZERO_INIT;
    int64_t dist_curr = 0;
    int64_t dist_next = 0;
    uint32_t out_cunt = 0;
    uint32_t curr = 0;
    uint32_t next = 0;
    uint32_t fan = 0;

    in[0] = vert_a;
    in[LDG_TERM_RENDER_STEP] = vert_b;
    in[LDG_TERM_RENDER_STEP + LDG_TERM_RENDER_STEP] = vert_c;

    for (curr = 0; curr < LDG_TERM_RENDER_TRI_VERT_CUNT; curr++)
    {
        next = (curr + LDG_TERM_RENDER_STEP) % LDG_TERM_RENDER_TRI_VERT_CUNT;
        dist_curr = in[curr]->pos[LDG_TERM_RENDER_Z_IDX] + in[curr]->pos[LDG_TERM_RENDER_W_IDX];
        dist_next = in[next]->pos[LDG_TERM_RENDER_Z_IDX] + in[next]->pos[LDG_TERM_RENDER_W_IDX];

        if (dist_curr >= 0)
        {
            out[out_cunt] = *in[curr];
            out_cunt++;
        }

        if ((dist_curr >= 0) != (dist_next >= 0))
        {
            render_clip_vert_lerp(in[curr], in[next], dist_curr, dist_curr - dist_next, &out[out_cunt]);
            out_cunt++;
        }
    }

    if (out_cunt < LDG_TERM_RENDER_TRI_VERT_CUNT) { return; }

    render_screen_vert_from_clip_set(render, &out[0], &screen_a);

    for (fan = LDG_TERM_RENDER_STEP; fan + LDG_TERM_RENDER_STEP < out_cunt; fan++)
    {
        render_screen_vert_from_clip_set(render, &out[fan], &screen_b);
        render_screen_vert_from_clip_set(render, &out[fan + LDG_TERM_RENDER_STEP], &screen_c);
        render_tri_raster(render, &screen_a, &screen_b, &screen_c);
    }
}

uint32_t ldg_term_render_tri3_draw(ldg_term_render_t *render, const ldg_term_render_vert3_t *vert_a, const ldg_term_render_vert3_t *vert_b, const ldg_term_render_vert3_t *vert_c)
{
    if (LDG_UNLIKELY(!render || !vert_a || !vert_b || !vert_c)) { return LDG_ERR_FUNC_ARG_NULL; }

    render_vert3_transform(render, vert_a, &render->vert_buff[0]);
    render_vert3_transform(render, vert_b, &render->vert_buff[LDG_TERM_RENDER_STEP]);
    render_vert3_transform(render, vert_c, &render->vert_buff[LDG_TERM_RENDER_STEP + LDG_TERM_RENDER_STEP]);

    render_clip_tri_draw(render, &render->vert_buff[0], &render->vert_buff[LDG_TERM_RENDER_STEP], &render->vert_buff[LDG_TERM_RENDER_STEP + LDG_TERM_RENDER_STEP]);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_mesh_draw(ldg_term_render_t *render, const ldg_term_render_vert3_t *vert, uint32_t vert_cunt, const uint32_t *idx, uint32_t idx_cunt)
{
    uint32_t pos = 0;
    uint32_t tri = 0;

    if (LDG_UNLIKELY(!render || !vert || !idx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(vert_cunt > render->vert_max)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY((idx_cunt % LDG_TERM_RENDER_TRI_VERT_CUNT) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (pos = 0; pos < idx_cunt; pos++) { if (LDG_UNLIKELY(idx[pos] >= vert_cunt)) { return LDG_ERR_BOUNDS; } }

    for (pos = 0; pos < vert_cunt; pos++) { render_vert3_transform(render, &vert[pos], &render->vert_buff[pos]); }

    for (tri = 0; tri < idx_cunt; tri += LDG_TERM_RENDER_TRI_VERT_CUNT) { render_clip_tri_draw(render, &render->vert_buff[idx[tri]], &render->vert_buff[idx[tri + LDG_TERM_RENDER_STEP]], &render->vert_buff[idx[tri + LDG_TERM_RENDER_STEP + LDG_TERM_RENDER_STEP]]); }

    return LDG_ERR_AOK;
}

static int64_t render_luma_get(const uint8_t *sample)
{
    return ((int64_t)sample[LDG_TERM_RENDER_RED_IDX] * LDG_TERM_RENDER_LUMA_RED) + ((int64_t)sample[LDG_TERM_RENDER_GREEN_IDX] * LDG_TERM_RENDER_LUMA_GREEN) + ((int64_t)sample[LDG_TERM_RENDER_BLUE_IDX] * LDG_TERM_RENDER_LUMA_BLUE);
}

static int64_t render_channel_weight_get(uint32_t channel)
{
    if (channel == LDG_TERM_RENDER_RED_IDX) { return LDG_TERM_RENDER_LUMA_RED; }

    if (channel == LDG_TERM_RENDER_GREEN_IDX) { return LDG_TERM_RENDER_LUMA_GREEN; }

    return LDG_TERM_RENDER_LUMA_BLUE;
}

static int64_t render_split_residual_get(const int64_t *total, const int64_t *square_total, const int64_t *on_sum, int64_t on_cunt, int64_t off_cunt)
{
    int64_t residual = 0;
    int64_t channel_residual = 0;
    int64_t off_sum = 0;
    uint32_t channel = 0;

    for (channel = 0; channel < LDG_TERM_RENDER_RGB_CUNT; channel++)
    {
        channel_residual = square_total[channel];
        off_sum = total[channel] - on_sum[channel];

        if (on_cunt > 0) { channel_residual -= (on_sum[channel] * on_sum[channel]) / on_cunt; }

        if (off_cunt > 0) { channel_residual -= (off_sum * off_sum) / off_cunt; }

        residual += channel_residual * render_channel_weight_get(channel);
    }

    return residual;
}

static int64_t render_fixed_bg_residual_get(const ldg_term_render_t *render, const int64_t *total, const int64_t *square_total, const int64_t *on_sum, const int64_t *on_square, int64_t on_cunt, int64_t off_cunt)
{
    int64_t residual = 0;
    int64_t channel_residual = 0;
    int64_t off_sum = 0;
    int64_t off_square = 0;
    int64_t bg = 0;
    uint32_t channel = 0;

    for (channel = 0; channel < LDG_TERM_RENDER_RGB_CUNT; channel++)
    {
        bg = render->bg[channel];
        off_sum = total[channel] - on_sum[channel];
        off_square = square_total[channel] - on_square[channel];
        channel_residual = on_square[channel];

        if (on_cunt > 0) { channel_residual -= (on_sum[channel] * on_sum[channel]) / on_cunt; }

        channel_residual += off_square - (LDG_TERM_RENDER_PLANE_CUNT * bg * off_sum) + (off_cunt * bg * bg);

        residual += channel_residual * render_channel_weight_get(channel);
    }

    return residual;
}

static uint32_t render_popcount_get(uint64_t word)
{
    word = word - ((word >> LDG_TERM_RENDER_POPCOUNT_SHIFT_1) & LDG_TERM_RENDER_POPCOUNT_M1);
    word = (word & LDG_TERM_RENDER_POPCOUNT_M2) + ((word >> LDG_TERM_RENDER_POPCOUNT_SHIFT_2) & LDG_TERM_RENDER_POPCOUNT_M2);
    word = (word + (word >> LDG_TERM_RENDER_POPCOUNT_SHIFT_4)) & LDG_TERM_RENDER_POPCOUNT_M4;

    return (uint32_t)((word * LDG_TERM_RENDER_POPCOUNT_H01) >> LDG_TERM_RENDER_POPCOUNT_SHIFT_OUT);
}

static void render_candidate_eval(const ldg_term_render_t *render, const uint8_t *cell_base, uint32_t glyph, const int64_t *total, const int64_t *square_total, int64_t cell_sample_cunt, int64_t *best_residual, uint32_t *best_glyph, int64_t *best_on_sum)
{
    const uint8_t *on_list = 0x0;
    const uint8_t *sample = 0x0;
    int64_t on_sum[LDG_TERM_RENDER_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t on_square[LDG_TERM_RENDER_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t on_cunt = 0;
    int64_t residual = 0;
    uint32_t pos = 0;
    uint32_t channel = 0;

    on_cunt = render->glyph_coverage[glyph];
    on_list = &render->glyph_mask[glyph * (uint32_t)cell_sample_cunt];

    for (pos = 0; pos < (uint32_t)on_cunt; pos++)
    {
        sample = cell_base + ((uint64_t)render->cell_sample_off[on_list[pos]] * LDG_TERM_RENDER_RGB_CUNT);

        for (channel = 0; channel < LDG_TERM_RENDER_RGB_CUNT; channel++)
        {
            on_sum[channel] += sample[channel];
            on_square[channel] += (int64_t)sample[channel] * (int64_t)sample[channel];
        }
    }

    if (render->fg_only == LDG_TRUTH_TRUE) { residual = render_fixed_bg_residual_get(render, total, square_total, on_sum, on_square, on_cunt, cell_sample_cunt - on_cunt); }
    else { residual = render_split_residual_get(total, square_total, on_sum, on_cunt, cell_sample_cunt - on_cunt); }

    if (residual >= *best_residual) { return; }

    *best_residual = residual;
    *best_glyph = glyph;
    best_on_sum[LDG_TERM_RENDER_RED_IDX] = on_sum[LDG_TERM_RENDER_RED_IDX];
    best_on_sum[LDG_TERM_RENDER_GREEN_IDX] = on_sum[LDG_TERM_RENDER_GREEN_IDX];
    best_on_sum[LDG_TERM_RENDER_BLUE_IDX] = on_sum[LDG_TERM_RENDER_BLUE_IDX];
}

static void render_candidate_hamming_eval(const ldg_term_render_t *render, const uint8_t *cell_base, const uint64_t *cell_bits, const int64_t *total, const int64_t *square_total, int64_t cell_sample_cunt, int64_t *best_residual, uint32_t *best_glyph, int64_t *best_on_sum)
{
    uint32_t candidate[LDG_TERM_RENDER_CANDIDATE_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t candidate_dist[LDG_TERM_RENDER_CANDIDATE_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t candidate_cunt = 0;
    uint32_t glyph = 0;
    uint32_t dist = 0;
    uint32_t inverse = 0;
    uint32_t slot = 0;
    uint32_t probe = 0;
    const uint64_t *bits = 0x0;

    for (glyph = 0; glyph < LDG_TERM_RENDER_GLYPH_CUNT; glyph++)
    {
        bits = &render->glyph_bits[glyph * LDG_TERM_RENDER_BIT_WORD_CUNT];
        dist = render_popcount_get(bits[0] ^ cell_bits[0]) + render_popcount_get(bits[LDG_TERM_RENDER_STEP] ^ cell_bits[LDG_TERM_RENDER_STEP]);
        if ((int64_t)dist <= cell_sample_cunt)
        {
            inverse = (uint32_t)cell_sample_cunt - dist;
            if (inverse < dist) { dist = inverse; }
        }

        if (candidate_cunt == LDG_TERM_RENDER_CANDIDATE_CUNT && dist >= candidate_dist[candidate_cunt - LDG_TERM_RENDER_STEP]) { continue; }

        slot = (candidate_cunt < LDG_TERM_RENDER_CANDIDATE_CUNT) ? candidate_cunt : candidate_cunt - LDG_TERM_RENDER_STEP;

        while (slot > 0 && candidate_dist[slot - LDG_TERM_RENDER_STEP] > dist)
        {
            candidate[slot] = candidate[slot - LDG_TERM_RENDER_STEP];
            candidate_dist[slot] = candidate_dist[slot - LDG_TERM_RENDER_STEP];
            slot--;
        }

        candidate[slot] = glyph;
        candidate_dist[slot] = dist;

        if (candidate_cunt < LDG_TERM_RENDER_CANDIDATE_CUNT) { candidate_cunt++; }
    }

    for (probe = 0; probe < candidate_cunt; probe++) { render_candidate_eval(render, cell_base, candidate[probe], total, square_total, cell_sample_cunt, best_residual, best_glyph, best_on_sum); }
}

static uint32_t render_ramp_glyph_get(const ldg_term_render_t *render, int64_t luma_mean, int64_t cell_sample_cunt)
{
    int64_t luma_bg = 0;
    int64_t tgt = 0;
    int64_t dist = 0;
    int64_t best_dist = INT64_MAX;
    uint32_t best = 0;
    uint32_t pos = 0;

    luma_bg = render_luma_get(render->bg);

    if (luma_mean <= luma_bg || luma_bg >= LDG_TERM_RENDER_LUMA_MAX) { return render->ramp_glyph[0]; }

    tgt = (cell_sample_cunt * (luma_mean - luma_bg)) / (LDG_TERM_RENDER_LUMA_MAX - luma_bg);

    for (pos = 0; pos < render->ramp_cunt; pos++)
    {
        dist = render_absolute_get((int64_t)render->glyph_coverage[render->ramp_glyph[pos]] - tgt);

        if (dist < best_dist)
        {
            best_dist = dist;
            best = render->ramp_glyph[pos];
        }
    }

    return best;
}

static void render_cell_colors_set(const ldg_term_render_t *render, ldg_term_cell_t *out, uint32_t glyph, const int64_t *total, const int64_t *on_sum, int64_t cell_sample_cunt, uint8_t tone_scale)
{
    int64_t on_cunt = 0;
    int64_t off_cunt = 0;
    int64_t val = 0;
    uint32_t channel = 0;

    on_cunt = render->glyph_coverage[glyph];
    off_cunt = cell_sample_cunt - on_cunt;
    out->ch = (uint8_t)(LDG_TERM_RENDER_GLYPH_FIRST + glyph);

    for (channel = 0; channel < LDG_TERM_RENDER_RGB_CUNT; channel++)
    {
        if (render->fg_only == LDG_TRUTH_TRUE) { out->bg[channel] = render->bg[channel]; }
        else if (off_cunt > 0) { out->bg[channel] = (uint8_t)((total[channel] - on_sum[channel]) / off_cunt); }
        else { out->bg[channel] = (uint8_t)(on_sum[channel] / on_cunt); }

        if (on_cunt == 0) { val = out->bg[channel]; }
        else if (tone_scale == LDG_TRUTH_TRUE) { val = (total[channel] - (off_cunt * (int64_t)out->bg[channel])) / on_cunt; }
        else { val = on_sum[channel] / on_cunt; }

        out->fg[channel] = (uint8_t)render_clamp_get(val, LDG_TERM_RENDER_COLOR_MIN, LDG_TERM_RENDER_COLOR_MAX);
    }
}

static uint8_t render_gain_enough_is(int64_t candidate, int64_t baseline, uint32_t margin)
{
    if (candidate * (int64_t)LDG_TERM_RENDER_PER_MILLE < baseline * ((int64_t)LDG_TERM_RENDER_PER_MILLE - (int64_t)margin)) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static void render_cell_resolve(const ldg_term_render_t *render, uint32_t col, uint32_t row, uint8_t ready, ldg_term_cell_t *out)
{
    const uint8_t *cell_base = 0x0;
    const uint8_t *sample = 0x0;
    int64_t total[LDG_TERM_RENDER_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t square_total[LDG_TERM_RENDER_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t best_on_sum[LDG_TERM_RENDER_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t base_on_sum[LDG_TERM_RENDER_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t prev_on_sum[LDG_TERM_RENDER_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t flat_on_sum[LDG_TERM_RENDER_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t cell_bits[LDG_TERM_RENDER_BIT_WORD_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t luma_total = 0;
    int64_t luma_mean = 0;
    int64_t luma = 0;
    int64_t luma_min = INT64_MAX;
    int64_t luma_max = 0;
    int64_t luma_split = 0;
    int64_t cell_sample_cunt = 0;
    int64_t flat_residual = 0;
    int64_t flat_floor = 0;
    int64_t contrast_floor = 0;
    int64_t best_residual = INT64_MAX;
    int64_t base_residual = INT64_MAX;
    int64_t prev_residual = INT64_MAX;
    uint32_t best_glyph = 0;
    uint32_t base_glyph = 0;
    uint32_t prev_glyph = 0;
    uint32_t chosen_glyph = 0;
    uint32_t pos = 0;
    uint32_t channel = 0;
    uint32_t sample_row = 0;
    uint32_t sample_col = 0;
    uint8_t chosen_scale = LDG_TRUTH_FALSE;

    cell_sample_cunt = (int64_t)render->sample_col_cunt * (int64_t)render->sample_row_cunt;
    if (LDG_UNLIKELY(cell_sample_cunt == 0)) { return; }

    cell_base = render->sample_color + ((((uint64_t)row * render->sample_row_cunt * render->width) + ((uint64_t)col * render->sample_col_cunt)) * LDG_TERM_RENDER_RGB_CUNT);

    for (sample_row = 0; sample_row < render->sample_row_cunt; sample_row++)
    {
        sample = cell_base + ((uint64_t)sample_row * render->width * LDG_TERM_RENDER_RGB_CUNT);

        for (sample_col = 0; sample_col < render->sample_col_cunt; sample_col++)
        {
            total[LDG_TERM_RENDER_RED_IDX] += sample[LDG_TERM_RENDER_RED_IDX];
            total[LDG_TERM_RENDER_GREEN_IDX] += sample[LDG_TERM_RENDER_GREEN_IDX];
            total[LDG_TERM_RENDER_BLUE_IDX] += sample[LDG_TERM_RENDER_BLUE_IDX];
            square_total[LDG_TERM_RENDER_RED_IDX] += (int64_t)sample[LDG_TERM_RENDER_RED_IDX] * (int64_t)sample[LDG_TERM_RENDER_RED_IDX];
            square_total[LDG_TERM_RENDER_GREEN_IDX] += (int64_t)sample[LDG_TERM_RENDER_GREEN_IDX] * (int64_t)sample[LDG_TERM_RENDER_GREEN_IDX];
            square_total[LDG_TERM_RENDER_BLUE_IDX] += (int64_t)sample[LDG_TERM_RENDER_BLUE_IDX] * (int64_t)sample[LDG_TERM_RENDER_BLUE_IDX];
            luma = render_luma_get(sample);

            if (luma < luma_min) { luma_min = luma; }

            if (luma > luma_max) { luma_max = luma; }

            sample += LDG_TERM_RENDER_RGB_CUNT;
        }
    }

    luma_total = (total[LDG_TERM_RENDER_RED_IDX] * LDG_TERM_RENDER_LUMA_RED) + (total[LDG_TERM_RENDER_GREEN_IDX] * LDG_TERM_RENDER_LUMA_GREEN) + (total[LDG_TERM_RENDER_BLUE_IDX] * LDG_TERM_RENDER_LUMA_BLUE);
    luma_mean = luma_total / cell_sample_cunt;

    if (render->fg_only == LDG_TRUTH_TRUE)
    {
        base_glyph = render_ramp_glyph_get(render, luma_mean, cell_sample_cunt);
        chosen_scale = LDG_TRUTH_TRUE;
    }

    render_candidate_eval(render, cell_base, base_glyph, total, square_total, cell_sample_cunt, &base_residual, &base_glyph, base_on_sum);

    flat_residual = render_split_residual_get(total, square_total, flat_on_sum, LDG_TERM_RENDER_FLAT_ON_CUNT, cell_sample_cunt);
    flat_floor = (int64_t)LDG_TERM_RENDER_LUMA_SCALE * cell_sample_cunt * LDG_TERM_RENDER_FIT_FLAT_DEVIATION * LDG_TERM_RENDER_FIT_FLAT_DEVIATION;

    if (flat_residual > flat_floor)
    {
        luma_split = (luma_min + luma_max) / LDG_TERM_RENDER_PLANE_CUNT;

        for (pos = 0; pos < (uint32_t)cell_sample_cunt; pos++)
        {
            sample = cell_base + ((uint64_t)render->cell_sample_off[pos] * LDG_TERM_RENDER_RGB_CUNT);

            if (render_luma_get(sample) >= luma_split) { cell_bits[pos >> LDG_TERM_RENDER_WORD_SHIFT] |= (uint64_t)LDG_TERM_RENDER_STEP << (pos & LDG_TERM_RENDER_WORD_MASK); }
        }

        render_candidate_hamming_eval(render, cell_base, cell_bits, total, square_total, cell_sample_cunt, &best_residual, &best_glyph, best_on_sum);
    }

    chosen_glyph = base_glyph;
    contrast_floor = (int64_t)LDG_TERM_RENDER_LUMA_SCALE * cell_sample_cunt * (int64_t)render->fit_contrast_min * (int64_t)render->fit_contrast_min;

    if (render_gain_enough_is(best_residual, base_residual, render->fit_gain_min) == LDG_TRUTH_TRUE && base_residual - best_residual >= contrast_floor)
    {
        chosen_glyph = best_glyph;
        chosen_scale = LDG_TRUTH_FALSE;
        base_residual = best_residual;

        for (channel = 0; channel < LDG_TERM_RENDER_RGB_CUNT; channel++) { base_on_sum[channel] = best_on_sum[channel]; }
    }

    if (render->fit_hold > 0 && ready == LDG_TRUTH_TRUE && out->ch >= LDG_TERM_RENDER_GLYPH_FIRST && out->ch < LDG_TERM_RENDER_GLYPH_FIRST + LDG_TERM_RENDER_GLYPH_CUNT)
    {
        prev_glyph = (uint32_t)out->ch - LDG_TERM_RENDER_GLYPH_FIRST;

        if (prev_glyph != chosen_glyph)
        {
            render_candidate_eval(render, cell_base, prev_glyph, total, square_total, cell_sample_cunt, &prev_residual, &prev_glyph, prev_on_sum);

            if (render_gain_enough_is(base_residual, prev_residual, render->fit_hold) != LDG_TRUTH_TRUE)
            {
                chosen_glyph = prev_glyph;
                chosen_scale = (render->fg_only == LDG_TRUTH_TRUE && prev_glyph == render_ramp_glyph_get(render, luma_mean, cell_sample_cunt)) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

                for (channel = 0; channel < LDG_TERM_RENDER_RGB_CUNT; channel++) { base_on_sum[channel] = prev_on_sum[channel]; }
            }
        }
    }

    out->attr = LDG_TERM_ATTR_NONE;
    out->link = LDG_TERM_LINK_NONE;

    render_cell_colors_set(render, out, chosen_glyph, total, base_on_sum, cell_sample_cunt, chosen_scale);
}

static uint8_t render_span_differ_is(const uint8_t *lhs, const uint8_t *rhs, uint64_t len)
{
    uint32_t ord = LDG_MEM_ORD_EQ;

    if (ldg_mem_cmp(lhs, rhs, len, &ord) != LDG_ERR_AOK) { return LDG_TRUTH_TRUE; }

    return (ord != LDG_MEM_ORD_EQ) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t render_band_diff_get(const ldg_term_render_t *render, uint32_t row, uint8_t *row_diff)
{
    uint64_t row_size = (uint64_t)render->width * LDG_TERM_RENDER_RGB_CUNT;
    uint64_t off = (uint64_t)row * render->sample_row_cunt * row_size;
    uint32_t sample_row = 0;
    uint8_t any = LDG_TRUTH_FALSE;

    for (sample_row = 0; sample_row < render->sample_row_cunt; sample_row++)
    {
        row_diff[sample_row] = render_span_differ_is(render->sample_color + off, render->sample_prev + off, row_size);

        if (row_diff[sample_row] == LDG_TRUTH_TRUE) { any = LDG_TRUTH_TRUE; }

        off += row_size;
    }

    return any;
}

static uint8_t render_cell_changed_is(const ldg_term_render_t *render, uint32_t col, uint32_t row, const uint8_t *row_diff)
{
    uint64_t span = (uint64_t)render->sample_col_cunt * LDG_TERM_RENDER_RGB_CUNT;
    uint64_t row_size = (uint64_t)render->width * LDG_TERM_RENDER_RGB_CUNT;
    uint64_t off = ((uint64_t)row * render->sample_row_cunt * row_size) + ((uint64_t)col * span);
    uint32_t sample_row = 0;

    for (sample_row = 0; sample_row < render->sample_row_cunt; sample_row++)
    {
        if (row_diff[sample_row] == LDG_TRUTH_TRUE && render_span_differ_is(render->sample_color + off, render->sample_prev + off, span) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

        off += row_size;
    }

    return LDG_TRUTH_FALSE;
}

static uint32_t render_band_commit(ldg_term_render_t *render, uint32_t row, const uint8_t *row_diff)
{
    uint64_t row_size = (uint64_t)render->width * LDG_TERM_RENDER_RGB_CUNT;
    uint64_t off = (uint64_t)row * render->sample_row_cunt * row_size;
    uint32_t sample_row = 0;
    uint32_t ret = 0;

    for (sample_row = 0; sample_row < render->sample_row_cunt; sample_row++)
    {
        if (row_diff[sample_row] == LDG_TRUTH_TRUE)
        {
            ret = ldg_mem_cpy(render->sample_prev + off, render->sample_color + off, row_size);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        off += row_size;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_resolve(ldg_term_render_t *render)
{
    uint8_t row_diff[LDG_TERM_RENDER_GLYPH_ROW_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t col = 0;
    uint32_t row = 0;
    uint32_t ret = 0;
    uint64_t cell_idx = 0;
    uint8_t stale = LDG_TRUTH_FALSE;
    uint8_t band = LDG_TRUTH_FALSE;
    uint8_t changed = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!render)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (row = 0; row < render->row_cunt; row++)
    {
        band = render_band_diff_get(render, row, row_diff);

        for (col = 0; col < render->col_cunt; col++)
        {
            cell_idx = ((uint64_t)row * render->col_cunt) + col;

            if (render->text_mask[cell_idx] == LDG_TRUTH_TRUE)
            {
                render->cell[cell_idx] = render->text_cell[cell_idx];
                render->text_prev[cell_idx] = LDG_TRUTH_TRUE;
                continue;
            }

            stale = render->text_prev[cell_idx];
            render->text_prev[cell_idx] = LDG_TRUTH_FALSE;
            changed = (band == LDG_TRUTH_TRUE) ? render_cell_changed_is(render, col, row, row_diff) : LDG_TRUTH_FALSE;

            if (changed != LDG_TRUTH_TRUE && stale != LDG_TRUTH_TRUE) { continue; }

            render_cell_resolve(render, col, row, render->cell_ready[cell_idx], &render->cell[cell_idx]);
            render->cell_ready[cell_idx] = LDG_TRUTH_TRUE;
        }

        if (band != LDG_TRUTH_TRUE) { continue; }

        ret = render_band_commit(render, row, row_diff);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_cell_get(const ldg_term_render_t *render, uint32_t col, uint32_t row, ldg_term_cell_t *out)
{
    if (LDG_UNLIKELY(!render || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(col >= render->col_cunt || row >= render->row_cunt)) { return LDG_ERR_BOUNDS; }

    *out = render->cell[((uint64_t)row * render->col_cunt) + col];

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_cell_arr_get(const ldg_term_render_t *render, const ldg_term_cell_t **out, uint64_t *out_cunt)
{
    if (LDG_UNLIKELY(!render || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = render->cell;
    *out_cunt = (uint64_t)render->col_cunt * (uint64_t)render->row_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_place(const ldg_term_render_t *render, ldg_term_t *term, uint32_t col, uint32_t row)
{
    ldg_term_cell_t *grid = 0x0;
    uint32_t src_col = 0;
    uint32_t src_row = 0;
    uint32_t dst_col = 0;
    uint32_t dst_row = 0;
    uint32_t last_row = 0;

    if (LDG_UNLIKELY(!render || !term)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(col >= term->col_cunt || row >= term->row_cunt)) { return LDG_ERR_BOUNDS; }

    grid = (term->alt_active == LDG_TRUTH_TRUE) ? term->alt_cell : term->cell;
    last_row = row;

    for (src_row = 0; src_row < render->row_cunt; src_row++)
    {
        dst_row = row + src_row;

        if (dst_row >= term->row_cunt) { break; }

        last_row = dst_row;

        for (src_col = 0; src_col < render->col_cunt; src_col++)
        {
            dst_col = col + src_col;

            if (dst_col >= term->col_cunt) { break; }

            grid[((uint64_t)dst_row * term->col_cunt) + dst_col] = render->cell[((uint64_t)src_row * render->col_cunt) + src_col];
        }
    }

    if (row < term->dirty_top) { term->dirty_top = row; }

    if (last_row > term->dirty_bottom) { term->dirty_bottom = last_row; }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_aspect_get(const ldg_term_render_t *render, ldg_term_render_fixed_t *out)
{
    int64_t numerator = 0;
    int64_t denominator = 0;

    if (LDG_UNLIKELY(!render || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(render->clip.row_cunt == 0)) { return LDG_ERR_BOUNDS; }

    numerator = (int64_t)render->clip.col_cunt * (int64_t)render->cell_aspect_num;
    denominator = (int64_t)render->clip.row_cunt * (int64_t)render->cell_aspect_den;

    *out = (ldg_term_render_fixed_t)((numerator << LDG_TERM_RENDER_FIXED_SHIFT) / denominator);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_fixed_mul(ldg_term_render_fixed_t lhs, ldg_term_render_fixed_t rhs, ldg_term_render_fixed_t *out)
{
    int64_t product = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    product = ((int64_t)lhs * (int64_t)rhs) >> LDG_TERM_RENDER_FIXED_SHIFT;

    if (LDG_UNLIKELY(product > INT32_MAX || product < INT32_MIN)) { return LDG_ERR_OVERFLOW; }

    *out = (ldg_term_render_fixed_t)product;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_fixed_div(ldg_term_render_fixed_t lhs, ldg_term_render_fixed_t rhs, ldg_term_render_fixed_t *out)
{
    int64_t quotient = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(rhs == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    quotient = ((int64_t)lhs << LDG_TERM_RENDER_FIXED_SHIFT) / (int64_t)rhs;

    if (LDG_UNLIKELY(quotient > INT32_MAX || quotient < INT32_MIN)) { return LDG_ERR_OVERFLOW; }

    *out = (ldg_term_render_fixed_t)quotient;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_fixed_square_root_get(ldg_term_render_fixed_t val, ldg_term_render_fixed_t *out)
{
    uint64_t radicand = 0;
    uint64_t res = 0;
    uint64_t bit = LDG_TERM_RENDER_SQRT_BIT_START;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(val < 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    radicand = (uint64_t)val << LDG_TERM_RENDER_FIXED_SHIFT;

    while (bit > radicand) { bit >>= LDG_TERM_RENDER_SQRT_BIT_STEP; }

    while (bit != 0)
    {
        if (radicand >= res + bit)
        {
            radicand -= res + bit;
            res = (res >> LDG_TERM_RENDER_STEP) + bit;
        }
        else { res >>= LDG_TERM_RENDER_STEP; }

        bit >>= LDG_TERM_RENDER_SQRT_BIT_STEP;
    }

    *out = (ldg_term_render_fixed_t)res;

    return LDG_ERR_AOK;
}

static int64_t render_sin_quadrant_get(uint32_t frac)
{
    int64_t xq = frac;
    int64_t x2 = 0;
    int64_t poly = 0;

    x2 = (xq * xq) >> LDG_TERM_RENDER_QUADRANT_SHIFT;
    poly = LDG_TERM_RENDER_SIN_C9;
    poly = LDG_TERM_RENDER_SIN_C7 - ((poly * x2) >> LDG_TERM_RENDER_QUADRANT_SHIFT);
    poly = LDG_TERM_RENDER_SIN_C5 - ((poly * x2) >> LDG_TERM_RENDER_QUADRANT_SHIFT);
    poly = LDG_TERM_RENDER_SIN_C3 - ((poly * x2) >> LDG_TERM_RENDER_QUADRANT_SHIFT);
    poly = LDG_TERM_RENDER_SIN_C1 - ((poly * x2) >> LDG_TERM_RENDER_QUADRANT_SHIFT);

    return ((xq * poly) >> LDG_TERM_RENDER_QUADRANT_SHIFT) >> LDG_TERM_RENDER_Q30_TO_Q16_SHIFT;
}

uint32_t ldg_term_render_fixed_sin_get(ldg_term_render_angle_t angle, ldg_term_render_fixed_t *out)
{
    uint32_t quadrant = 0;
    uint32_t frac = 0;
    int64_t res = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    quadrant = angle >> LDG_TERM_RENDER_QUADRANT_SHIFT;
    frac = angle & LDG_TERM_RENDER_QUADRANT_MASK;

    if ((quadrant & LDG_TERM_RENDER_STEP) != 0) { frac = (uint32_t)(LDG_TERM_RENDER_QUADRANT_ONE - (int64_t)frac); }

    res = render_sin_quadrant_get(frac);

    if (res > LDG_TERM_RENDER_FIXED_ONE) { res = LDG_TERM_RENDER_FIXED_ONE; }

    if (quadrant >= LDG_TERM_RENDER_PLANE_CUNT) { res = -res; }

    *out = (ldg_term_render_fixed_t)res;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_fixed_cos_get(ldg_term_render_angle_t angle, ldg_term_render_fixed_t *out)
{
    ldg_term_render_angle_t quadrant = 0;
    ldg_term_render_angle_t frac = 0;
    int64_t res = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    angle += LDG_TERM_RENDER_ANGLE_QUARTER;
    quadrant = angle >> LDG_TERM_RENDER_QUADRANT_SHIFT;
    frac = angle & LDG_TERM_RENDER_QUADRANT_MASK;

    if ((quadrant & LDG_TERM_RENDER_STEP) != 0) { frac = (uint32_t)(LDG_TERM_RENDER_QUADRANT_ONE - (int64_t)frac); }

    res = render_sin_quadrant_get(frac);

    if (res > LDG_TERM_RENDER_FIXED_ONE) { res = LDG_TERM_RENDER_FIXED_ONE; }

    if (quadrant >= LDG_TERM_RENDER_PLANE_CUNT) { res = -res; }

    *out = (ldg_term_render_fixed_t)res;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_mat4_ident_set(ldg_term_render_mat4_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < LDG_TERM_RENDER_HOMOGENEOUS_CUNT * LDG_TERM_RENDER_HOMOGENEOUS_CUNT; idx++) { out->elem[idx] = 0; }

    for (idx = 0; idx < LDG_TERM_RENDER_HOMOGENEOUS_CUNT; idx++) { out->elem[LDG_TERM_RENDER_ELEM(idx, idx)] = LDG_TERM_RENDER_FIXED_ONE; }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_mat4_mul(const ldg_term_render_mat4_t *lhs, const ldg_term_render_mat4_t *rhs, ldg_term_render_mat4_t *out)
{
    ldg_term_render_mat4_t res = LDG_STRUCT_ZERO_INIT;
    int64_t cum = 0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t inner = 0;

    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (row = 0; row < LDG_TERM_RENDER_HOMOGENEOUS_CUNT; row++)
    {
        for (col = 0; col < LDG_TERM_RENDER_HOMOGENEOUS_CUNT; col++)
        {
            cum = 0;

            for (inner = 0; inner < LDG_TERM_RENDER_HOMOGENEOUS_CUNT; inner++) { cum += (int64_t)lhs->elem[LDG_TERM_RENDER_ELEM(row, inner)] * (int64_t)rhs->elem[LDG_TERM_RENDER_ELEM(inner, col)]; }

            cum >>= LDG_TERM_RENDER_FIXED_SHIFT;

            if (LDG_UNLIKELY(cum > INT32_MAX || cum < INT32_MIN)) { return LDG_ERR_OVERFLOW; }

            res.elem[LDG_TERM_RENDER_ELEM(row, col)] = (ldg_term_render_fixed_t)cum;
        }
    }

    *out = res;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_mat4_translation_set(const ldg_term_render_fixed_t *pos, ldg_term_render_mat4_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pos || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_term_render_mat4_ident_set(out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_X_IDX, LDG_TERM_RENDER_W_IDX)] = pos[LDG_TERM_RENDER_X_IDX];
    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_Y_IDX, LDG_TERM_RENDER_W_IDX)] = pos[LDG_TERM_RENDER_Y_IDX];
    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_Z_IDX, LDG_TERM_RENDER_W_IDX)] = pos[LDG_TERM_RENDER_Z_IDX];

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_mat4_scale_set(const ldg_term_render_fixed_t *scale, ldg_term_render_mat4_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!scale || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_term_render_mat4_ident_set(out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_X_IDX, LDG_TERM_RENDER_X_IDX)] = scale[LDG_TERM_RENDER_X_IDX];
    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_Y_IDX, LDG_TERM_RENDER_Y_IDX)] = scale[LDG_TERM_RENDER_Y_IDX];
    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_Z_IDX, LDG_TERM_RENDER_Z_IDX)] = scale[LDG_TERM_RENDER_Z_IDX];

    return LDG_ERR_AOK;
}

static uint32_t render_rotation_set(ldg_term_render_angle_t angle, uint32_t axis_a, uint32_t axis_b, ldg_term_render_mat4_t *out)
{
    ldg_term_render_fixed_t sin_val = 0;
    ldg_term_render_fixed_t cos_val = 0;
    uint32_t ret = 0;

    ret = ldg_term_render_mat4_ident_set(out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_render_fixed_sin_get(angle, &sin_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_render_fixed_cos_get(angle, &cos_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->elem[LDG_TERM_RENDER_ELEM(axis_a, axis_a)] = cos_val;
    out->elem[LDG_TERM_RENDER_ELEM(axis_a, axis_b)] = -sin_val;
    out->elem[LDG_TERM_RENDER_ELEM(axis_b, axis_a)] = sin_val;
    out->elem[LDG_TERM_RENDER_ELEM(axis_b, axis_b)] = cos_val;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_render_mat4_rotation_x_set(ldg_term_render_angle_t angle, ldg_term_render_mat4_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    return render_rotation_set(angle, LDG_TERM_RENDER_Y_IDX, LDG_TERM_RENDER_Z_IDX, out);
}

uint32_t ldg_term_render_mat4_rotation_y_set(ldg_term_render_angle_t angle, ldg_term_render_mat4_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    return render_rotation_set(angle, LDG_TERM_RENDER_Z_IDX, LDG_TERM_RENDER_X_IDX, out);
}

uint32_t ldg_term_render_mat4_rotation_z_set(ldg_term_render_angle_t angle, ldg_term_render_mat4_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    return render_rotation_set(angle, LDG_TERM_RENDER_X_IDX, LDG_TERM_RENDER_Y_IDX, out);
}

uint32_t ldg_term_render_mat4_perspective_set(ldg_term_render_angle_t fov_y, ldg_term_render_fixed_t aspect, ldg_term_render_fixed_t near, ldg_term_render_fixed_t far, ldg_term_render_mat4_t *out)
{
    ldg_term_render_fixed_t sin_half = 0;
    ldg_term_render_fixed_t cos_half = 0;
    ldg_term_render_fixed_t focal = 0;
    ldg_term_render_fixed_t focal_x = 0;
    ldg_term_render_fixed_t depth_scale = 0;
    ldg_term_render_fixed_t depth_offset = 0;
    ldg_term_render_fixed_t near_far = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(aspect <= 0 || near <= 0 || far <= near)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_term_render_fixed_sin_get(fov_y >> LDG_TERM_RENDER_STEP, &sin_half);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_render_fixed_cos_get(fov_y >> LDG_TERM_RENDER_STEP, &cos_half);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(sin_half <= 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_term_render_fixed_div(cos_half, sin_half, &focal);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_render_fixed_div(focal, aspect, &focal_x);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_render_fixed_div(far + near, near - far, &depth_scale);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_render_fixed_mul(near, far, &near_far);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_term_render_fixed_div(near_far * (ldg_term_render_fixed_t)LDG_TERM_RENDER_PLANE_CUNT, near - far, &depth_offset);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < LDG_TERM_RENDER_HOMOGENEOUS_CUNT * LDG_TERM_RENDER_HOMOGENEOUS_CUNT; idx++) { out->elem[idx] = 0; }

    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_X_IDX, LDG_TERM_RENDER_X_IDX)] = focal_x;
    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_Y_IDX, LDG_TERM_RENDER_Y_IDX)] = focal;
    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_Z_IDX, LDG_TERM_RENDER_Z_IDX)] = depth_scale;
    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_Z_IDX, LDG_TERM_RENDER_W_IDX)] = depth_offset;
    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_W_IDX, LDG_TERM_RENDER_Z_IDX)] = LDG_TERM_RENDER_PERSPECTIVE_W_Z;

    return LDG_ERR_AOK;
}

static uint32_t render_vec3_normalize(ldg_term_render_fixed_t *vec)
{
    ldg_term_render_fixed_t len_square = 0;
    ldg_term_render_fixed_t len = 0;
    ldg_term_render_fixed_t term = 0;
    uint32_t axis = 0;
    uint32_t ret = 0;

    for (axis = 0; axis < LDG_TERM_RENDER_AXIS_CUNT; axis++)
    {
        ret = ldg_term_render_fixed_mul(vec[axis], vec[axis], &term);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        len_square += term;
    }

    if (LDG_UNLIKELY(len_square == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_term_render_fixed_square_root_get(len_square, &len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (axis = 0; axis < LDG_TERM_RENDER_AXIS_CUNT; axis++)
    {
        ret = ldg_term_render_fixed_div(vec[axis], len, &vec[axis]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static void render_vec3_cross(const ldg_term_render_fixed_t *lhs, const ldg_term_render_fixed_t *rhs, ldg_term_render_fixed_t *out)
{
    out[LDG_TERM_RENDER_X_IDX] = (ldg_term_render_fixed_t)((((int64_t)lhs[LDG_TERM_RENDER_Y_IDX] * rhs[LDG_TERM_RENDER_Z_IDX]) - ((int64_t)lhs[LDG_TERM_RENDER_Z_IDX] * rhs[LDG_TERM_RENDER_Y_IDX])) >> LDG_TERM_RENDER_FIXED_SHIFT);
    out[LDG_TERM_RENDER_Y_IDX] = (ldg_term_render_fixed_t)((((int64_t)lhs[LDG_TERM_RENDER_Z_IDX] * rhs[LDG_TERM_RENDER_X_IDX]) - ((int64_t)lhs[LDG_TERM_RENDER_X_IDX] * rhs[LDG_TERM_RENDER_Z_IDX])) >> LDG_TERM_RENDER_FIXED_SHIFT);
    out[LDG_TERM_RENDER_Z_IDX] = (ldg_term_render_fixed_t)((((int64_t)lhs[LDG_TERM_RENDER_X_IDX] * rhs[LDG_TERM_RENDER_Y_IDX]) - ((int64_t)lhs[LDG_TERM_RENDER_Y_IDX] * rhs[LDG_TERM_RENDER_X_IDX])) >> LDG_TERM_RENDER_FIXED_SHIFT);
}

static ldg_term_render_fixed_t render_vec3_dot_get(const ldg_term_render_fixed_t *lhs, const ldg_term_render_fixed_t *rhs)
{
    int64_t cum = 0;
    uint32_t axis = 0;

    for (axis = 0; axis < LDG_TERM_RENDER_AXIS_CUNT; axis++) { cum += (int64_t)lhs[axis] * (int64_t)rhs[axis]; }

    return (ldg_term_render_fixed_t)(cum >> LDG_TERM_RENDER_FIXED_SHIFT);
}

uint32_t ldg_term_render_mat4_cam_set(const ldg_term_render_fixed_t *eye, const ldg_term_render_fixed_t *tgt, const ldg_term_render_fixed_t *up, ldg_term_render_mat4_t *out)
{
    ldg_term_render_fixed_t forward[LDG_TERM_RENDER_AXIS_CUNT] = LDG_ARR_ZERO_INIT;
    ldg_term_render_fixed_t side[LDG_TERM_RENDER_AXIS_CUNT] = LDG_ARR_ZERO_INIT;
    ldg_term_render_fixed_t true_up[LDG_TERM_RENDER_AXIS_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t axis = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!eye || !tgt || !up || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (axis = 0; axis < LDG_TERM_RENDER_AXIS_CUNT; axis++) { forward[axis] = tgt[axis] - eye[axis]; }

    ret = render_vec3_normalize(forward);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render_vec3_cross(forward, up, side);

    ret = render_vec3_normalize(side);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    render_vec3_cross(side, forward, true_up);

    ret = ldg_term_render_mat4_ident_set(out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (axis = 0; axis < LDG_TERM_RENDER_AXIS_CUNT; axis++)
    {
        out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_X_IDX, axis)] = side[axis];
        out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_Y_IDX, axis)] = true_up[axis];
        out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_Z_IDX, axis)] = -forward[axis];
    }

    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_X_IDX, LDG_TERM_RENDER_W_IDX)] = -render_vec3_dot_get(side, eye);
    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_Y_IDX, LDG_TERM_RENDER_W_IDX)] = -render_vec3_dot_get(true_up, eye);
    out->elem[LDG_TERM_RENDER_ELEM(LDG_TERM_RENDER_Z_IDX, LDG_TERM_RENDER_W_IDX)] = render_vec3_dot_get(forward, eye);

    return LDG_ERR_AOK;
}
