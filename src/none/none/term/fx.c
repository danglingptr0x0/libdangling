#include <stdint.h>

#include <dangling/term/fx.h>
#include <dangling/term/render.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/mem.h>
#include <dangling/sys/entropy.h>

#define LDG_TERM_FX_STEP 1
#define LDG_TERM_FX_PAIR 2
#define LDG_TERM_FX_RGB_CUNT 3
#define LDG_TERM_FX_RED_IDX 0
#define LDG_TERM_FX_GREEN_IDX 1
#define LDG_TERM_FX_BLUE_IDX 2
#define LDG_TERM_FX_POOL_ITEM_SIZE_VAR 0
#define LDG_TERM_FX_POOL_ALLOC_CUNT 12
#define LDG_TERM_FX_ONE (1LL << LDG_TERM_RENDER_FIXED_SHIFT)
#define LDG_TERM_FX_HALF (LDG_TERM_FX_ONE / LDG_TERM_FX_PAIR)
#define LDG_TERM_FX_COLOR_MAX 255
#define LDG_TERM_FX_LUMA_RED 19595LL
#define LDG_TERM_FX_LUMA_GREEN 38470LL
#define LDG_TERM_FX_LUMA_BLUE 7471LL
#define LDG_TERM_FX_MS_PER_SEC 1000LL
#define LDG_TERM_FX_TIME_WRAP_MS 1000000000ULL
#define LDG_TERM_FX_GAIN_FIELD_CUNT 24
#define LDG_TERM_FX_RATE_FIELD_CUNT 8
#define LDG_TERM_FX_EDGE_LUT_CUNT 1024
#define LDG_TERM_FX_VEIL_GRID 6
#define LDG_TERM_FX_VEIL_FLOOR 5243LL
#define LDG_TERM_FX_BLOOM_DIV 4
#define LDG_TERM_FX_BLOOM_TAP 8
#define LDG_TERM_FX_BLOOM_SIGMA 229376LL
#define LDG_TERM_FX_HALO_NEAR 2
#define LDG_TERM_FX_HALO_FAR 7
#define LDG_TERM_FX_HALO_DIV 12
#define LDG_TERM_FX_HALO_FAR_DIV 2
#define LDG_TERM_FX_NONE UINT32_MAX
#define LDG_TERM_FX_HASH_MUL_A 0xBF58476D1CE4E5B9ULL
#define LDG_TERM_FX_HASH_MUL_B 0x94D049BB133111EBULL
#define LDG_TERM_FX_HASH_GOLDEN 0x9E3779B97F4A7C15ULL
#define LDG_TERM_FX_HASH_SHIFT_A 30
#define LDG_TERM_FX_HASH_SHIFT_B 27
#define LDG_TERM_FX_HASH_SHIFT_C 31
#define LDG_TERM_FX_HASH_FRAC_SHIFT 48
#define LDG_TERM_FX_HASH_ROW_SALT 7
#define LDG_TERM_FX_LOG2_ITER 16
#define LDG_TERM_FX_EXP2_C1 45426LL
#define LDG_TERM_FX_EXP2_C2 15743LL
#define LDG_TERM_FX_EXP2_C3 3638LL
#define LDG_TERM_FX_EXP2_C4 629LL
#define LDG_TERM_FX_LOG2_E 94548LL
#define LDG_TERM_FX_EDGE_SCALE 4
#define LDG_TERM_FX_RIPPLE_HARMONIC 34734LL
#define LDG_TERM_FX_RIPPLE_SCALE 16384LL
#define LDG_TERM_FX_RIPPLE_HW 104858LL
#define LDG_TERM_FX_HSYNC_RATE 120
#define LDG_TERM_FX_HSYNC_AMP 52LL
#define LDG_TERM_FX_HSYNC_HW 26LL
#define LDG_TERM_FX_VWOB_RATE 7300LL
#define LDG_TERM_FX_VWOB_AMP 20LL
#define LDG_TERM_FX_VWOB_HW 13LL
#define LDG_TERM_FX_LIFT_RATE 30
#define LDG_TERM_FX_LIFT_AMP 1311LL
#define LDG_TERM_FX_LIFT_HW 1966LL
#define LDG_TERM_FX_WARM_RATE 3129LL
#define LDG_TERM_FX_NONE_FIXED 0LL
#define LDG_TERM_FX_WARM_HW 49152LL
#define LDG_TERM_FX_WARM_RED 32768LL
#define LDG_TERM_FX_WARM_BLUE 19661LL
#define LDG_TERM_FX_HUM_SLOPE 88474LL
#define LDG_TERM_FX_HUM_RATE 354LL
#define LDG_TERM_FX_GRILLE_BASE 55706LL
#define LDG_TERM_FX_GRILLE_AMP 9830LL
#define LDG_TERM_FX_THIRD 21845LL
#define LDG_TERM_FX_SMOOTH_FAR 32768LL
#define LDG_TERM_FX_SMOOTH_NEAR 7864LL
#define LDG_TERM_FX_HOT_LO 19661LL
#define LDG_TERM_FX_HOT_HI 72090LL
#define LDG_TERM_FX_BEAM_GLOW_SPAN 6
#define LDG_TERM_FX_BEAM_DIM_AHEAD 2
#define LDG_TERM_FX_BEAM_DIM_BEHIND 4
#define LDG_TERM_FX_BEAM_DIM_FLOOR 60293LL
#define LDG_TERM_FX_BEAM_GLOW_GAIN 5243LL
#define LDG_TERM_FX_CUBIC_COEF 3
#define LDG_TERM_FX_CELL_CENTER_W 4
#define LDG_TERM_FX_CELL_EDGE_W 2
#define LDG_TERM_FX_CELL_DIV 16
#define LDG_TERM_FX_CELL_TAP_DIV 3
#define LDG_TERM_FX_BLEED_DIV 4
#define LDG_TERM_FX_DIAG_DIV 8
#define LDG_TERM_FX_APERTURE_EDGE 10486LL
#define LDG_TERM_FX_CELL_LIT_LO 131072LL
#define LDG_TERM_FX_CELL_LIT_HI 393216LL
#define LDG_TERM_FX_BAND_ROWS 128
#define LDG_TERM_FX_BAND_RAMP 42598LL
#define LDG_TERM_FX_BAND_HASH 22938LL
#define LDG_TERM_FX_SUSTAIN_HARMONIC 4
#define LDG_TERM_FX_ADDR_DT_MAX 250
#define LDG_TERM_FX_AXIS_X 0
#define LDG_TERM_FX_AXIS_Y 1
#define LDG_TERM_FX_BYTE_CUNT 256
#define LDG_TERM_FX_SIN_LUT_CUNT 1024
#define LDG_TERM_FX_SIN_LUT_MASK (LDG_TERM_FX_SIN_LUT_CUNT - 1)
#define LDG_TERM_FX_SIN_LUT_SHIFT 6
#define LDG_TERM_FX_SIN_PART_ONE (1LL << LDG_TERM_FX_SIN_LUT_SHIFT)
#define LDG_TERM_FX_SIN_PART_MASK ((uint64_t)LDG_TERM_FX_SIN_PART_ONE - 1U)
#define LDG_TERM_FX_SIN_ANGLE_STEP (LDG_TERM_RENDER_ANGLE_QUARTER / (LDG_TERM_FX_SIN_LUT_CUNT / 4U))
#define LDG_TERM_FX_FRAC_MASK ((uint64_t)LDG_TERM_FX_ONE - 1U)
#define LDG_TERM_FX_QUARTER_CYCLE (LDG_TERM_FX_ONE / 4)
#define LDG_TERM_FX_HALO_TAP_CUNT 5
#define LDG_TERM_FX_HALO_CENTER 2
#define LDG_TERM_FX_HALO_NORM ((int64_t)LDG_TERM_FX_COLOR_MAX * LDG_TERM_FX_HALO_FAR_DIV * LDG_TERM_FX_HALO_DIV)
#define LDG_TERM_FX_VEIL_LAST (LDG_TERM_FX_VEIL_GRID - LDG_TERM_FX_STEP)
#define LDG_TERM_FX_HOT_SPAN (LDG_TERM_FX_HOT_HI - LDG_TERM_FX_HOT_LO)
#define LDG_TERM_FX_WAVE_HUM LDG_TERM_FX_RGB_CUNT
#define LDG_TERM_FX_WAVE_CUNT (LDG_TERM_FX_RGB_CUNT + 1)

#define LDG_TERM_FX_CRT_TINT_RED 0x3CU
#define LDG_TERM_FX_CRT_TINT_GREEN 0xFFU
#define LDG_TERM_FX_CRT_TINT_BLUE 0xA5U
#define LDG_TERM_FX_CRT_HOT_RED 0x8CU
#define LDG_TERM_FX_CRT_HOT_GREEN 0xFFU
#define LDG_TERM_FX_CRT_HOT_BLUE 0xB9U
#define LDG_TERM_FX_VGA_TINT_LEVEL 0xFFU
#define LDG_TERM_FX_CGA_TINT_RED 0xFFU
#define LDG_TERM_FX_CGA_TINT_GREEN 0x55U
#define LDG_TERM_FX_CGA_TINT_BLUE 0x22U
#define LDG_TERM_FX_CGA_HOT_RED 0xFFU
#define LDG_TERM_FX_CGA_HOT_GREEN 0xD2U
#define LDG_TERM_FX_CGA_HOT_BLUE 0xA0U
#define LDG_TERM_FX_SCOPE_TINT_RED 0x8CU
#define LDG_TERM_FX_SCOPE_TINT_GREEN 0xFFU
#define LDG_TERM_FX_SCOPE_TINT_BLUE 0xB4U
#define LDG_TERM_FX_SCOPE_HOT_RED 0xDCU
#define LDG_TERM_FX_SCOPE_HOT_GREEN 0xFFU
#define LDG_TERM_FX_SCOPE_HOT_BLUE 0xEBU

#define LDG_TERM_FX_CRT_BRIGHT 1100U
#define LDG_TERM_FX_CRT_NOISE 20U
#define LDG_TERM_FX_CRT_PHOSPHOR 180U
#define LDG_TERM_FX_CRT_BLOOM 350U
#define LDG_TERM_FX_CRT_EDGE_FADE 250U
#define LDG_TERM_FX_CRT_CURVE 55U
#define LDG_TERM_FX_CRT_OVERSCAN 20U
#define LDG_TERM_FX_CRT_ZOOM 30U
#define LDG_TERM_FX_CRT_SCANLINE_GAP 350U
#define LDG_TERM_FX_CRT_SCROLL 20U
#define LDG_TERM_FX_CRT_LINE_CUNT 240U
#define LDG_TERM_FX_MASK_PITCH 3000U
#define LDG_TERM_FX_CRT_BEAM_RATE 600U
#define LDG_TERM_FX_CRT_BEAM_INTENSITY 500U
#define LDG_TERM_FX_CRT_BEAM_WIDTH 4000U
#define LDG_TERM_FX_CRT_WARMTH 20U
#define LDG_TERM_FX_CRT_RIPPLE 50U
#define LDG_TERM_FX_MAINS_RATE 50000U
#define LDG_TERM_FX_PANEL_CELL_CUNT_X 640U
#define LDG_TERM_FX_PANEL_CELL_CUNT_Y 400U
#define LDG_TERM_FX_CRT_HAZE 250U
#define LDG_TERM_FX_VGA_CURVE 45U
#define LDG_TERM_FX_VGA_SCANLINE 350U
#define LDG_TERM_FX_VGA_SCANLINE_GAP 500U
#define LDG_TERM_FX_VGA_LINE_CUNT 480U
#define LDG_TERM_FX_VGA_PHOSPHOR 220U
#define LDG_TERM_FX_VGA_BLOOM 280U
#define LDG_TERM_FX_VGA_NOISE 12U
#define LDG_TERM_FX_VGA_MASK 140U
#define LDG_TERM_FX_VGA_CONVERGE 600U
#define LDG_TERM_FX_VGA_BRIGHT 1050U
#define LDG_TERM_FX_VGA_WARMTH 15U
#define LDG_TERM_FX_VGA_RIPPLE 30U
#define LDG_TERM_FX_VGA_HAZE 140U
#define LDG_TERM_FX_CGA_PHOSPHOR 80U
#define LDG_TERM_FX_CGA_BLOOM 550U
#define LDG_TERM_FX_CGA_NOISE 22U
#define LDG_TERM_FX_CGA_EDGE_FADE 100U
#define LDG_TERM_FX_CGA_BRIGHT 1150U
#define LDG_TERM_FX_CGA_RIPPLE 35U
#define LDG_TERM_FX_CGA_CELL 450U
#define LDG_TERM_FX_CGA_BLEED 180U
#define LDG_TERM_FX_CGA_SUSTAIN 35U
#define LDG_TERM_FX_CGA_PEDESTAL 85U
#define LDG_TERM_FX_CGA_HAZE 550U
#define LDG_TERM_FX_CGA_BAND 100U
#define LDG_TERM_FX_CGA_ADDR_RATE 8000U
#define LDG_TERM_FX_SCOPE_PHOSPHOR 920U
#define LDG_TERM_FX_SCOPE_BLOOM 550U
#define LDG_TERM_FX_SCOPE_SCANLINE 120U
#define LDG_TERM_FX_SCOPE_HAZE 220U
#define LDG_TERM_FX_BLOOM_THRESHOLD 350U
#define LDG_TERM_FX_BLOOM_GAIN 550U
#define LDG_TERM_FX_BLOOM_RADIUS 44000U

typedef struct fx_row
{
    int64_t center_v;
    int64_t square_v;
    int64_t beam_mul;
    int64_t beam_add[LDG_TERM_FX_RGB_CUNT];
} fx_row_t;

struct ldg_term_fx
{
    ldg_mem_pool_t *pool;
    uint8_t *src;
    uint8_t *persist;
    uint8_t *face;
    uint16_t *bloom_a;
    uint16_t *bloom_b;
    int64_t *col_center;
    int64_t *col_square;
    fx_row_t *row;
    uint64_t sample_cap;
    uint64_t bloom_cap;
    uint64_t col_cap;
    uint64_t row_cap;
    uint64_t key;
    uint64_t frame;
    uint64_t time_prev;
    int64_t gauss[LDG_TERM_FX_BLOOM_TAP + LDG_TERM_FX_STEP];
    int32_t byte_fixed[LDG_TERM_FX_BYTE_CUNT];
    int32_t sin_lut[LDG_TERM_FX_SIN_LUT_CUNT];
    uint32_t edge_lut[LDG_TERM_FX_EDGE_LUT_CUNT];
    uint32_t veil[LDG_TERM_FX_VEIL_GRID * LDG_TERM_FX_VEIL_GRID];
    uint32_t edge_lut_fade;
    uint8_t persist_en;
    uint8_t face_valid;
    uint8_t time_valid;
    uint8_t edge_lut_valid;
};

typedef struct fx_frame
{
    const ldg_term_fx_style_t *style;
    int64_t time;
    int64_t hw;
    int64_t ac;
    int64_t hsync;
    int64_t vwob;
    int64_t lift;
    int64_t warm;
    int64_t beam_pos;
    int64_t beam_span;
    int64_t beam_lit;
    int64_t triad_lit;
    int64_t sustain;
    int64_t zoom;
    int64_t curve;
    int64_t overscan_inv;
    int64_t converge;
    int64_t bloom;
    int64_t line;
    int64_t scroll;
    int64_t scanline;
    int64_t gap;
    int64_t mask_w;
    int64_t pitch_scale;
    int64_t bright;
    int64_t pedestal;
    int64_t haze;
    int64_t noise;
    int64_t ripple;
    int64_t hum_t;
    int64_t phosphor;
    int64_t warm_red;
    int64_t warm_blue;
    int64_t tint[LDG_TERM_FX_RGB_CUNT];
    int64_t hot[LDG_TERM_FX_RGB_CUNT];
    uint64_t grain_seed;
    uint32_t x_lo;
    uint32_t y_lo;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint8_t mask_en;
    uint8_t mono;
    uint8_t pudding[2];
} fx_frame_t;

static int64_t fx_mul(int64_t lhs, int64_t rhs)
{
    return (lhs * rhs) / LDG_TERM_FX_ONE;
}

static int64_t fx_div(int64_t lhs, int64_t rhs)
{
    if (rhs == 0) { return LDG_TERM_FX_ONE; }

    return (lhs * LDG_TERM_FX_ONE) / rhs;
}

static int64_t fx_clamp(int64_t val, int64_t lo, int64_t hi)
{
    if (val < lo) { return lo; }

    if (val > hi) { return hi; }

    return val;
}

static int64_t fx_mille_get(uint32_t val)
{
    return ((int64_t)val * LDG_TERM_FX_ONE) / LDG_TERM_FX_PER_MILLE;
}

static int64_t fx_frac_get(int64_t val)
{
    int64_t frac = val % LDG_TERM_FX_ONE;

    if (frac < 0) { frac += LDG_TERM_FX_ONE; }

    return frac;
}

static int64_t fx_floor_get(int64_t val)
{
    return val - fx_frac_get(val);
}

static int64_t fx_abs_get(int64_t val)
{
    return (val < 0) ? -val : val;
}

static int64_t fx_mix(int64_t lhs, int64_t rhs, int64_t weight)
{
    return lhs + fx_mul(rhs - lhs, weight);
}

static int64_t fx_smoothstep(int64_t edge_lo, int64_t edge_hi, int64_t val)
{
    int64_t ramp = 0;

    ramp = fx_clamp(fx_div(val - edge_lo, edge_hi - edge_lo), LDG_TERM_FX_NONE_FIXED, LDG_TERM_FX_ONE);

    return fx_mul(fx_mul(ramp, ramp), (LDG_TERM_FX_CUBIC_COEF * LDG_TERM_FX_ONE) - (LDG_TERM_FX_PAIR * ramp));
}

static int64_t fx_sin_cycles_get(int64_t cycles)
{
    ldg_term_render_fixed_t out = 0;
    uint64_t angle = (uint64_t)cycles * (uint64_t)LDG_TERM_FX_ONE;

    if (ldg_term_render_fixed_sin_get((ldg_term_render_angle_t)angle, &out) != LDG_ERR_AOK) { return LDG_TERM_FX_NONE_FIXED; }

    return out;
}

static uint64_t fx_hash_get(uint64_t val)
{
    val += LDG_TERM_FX_HASH_GOLDEN;
    val = (val ^ (val >> LDG_TERM_FX_HASH_SHIFT_A)) * LDG_TERM_FX_HASH_MUL_A;
    val = (val ^ (val >> LDG_TERM_FX_HASH_SHIFT_B)) * LDG_TERM_FX_HASH_MUL_B;

    return val ^ (val >> LDG_TERM_FX_HASH_SHIFT_C);
}

static int64_t fx_hash_unit_get(uint64_t val)
{
    return (int64_t)(fx_hash_get(val) >> LDG_TERM_FX_HASH_FRAC_SHIFT);
}

static int64_t fx_vnoise_get(uint64_t key, int64_t pos)
{
    int64_t cell = fx_floor_get(pos) / LDG_TERM_FX_ONE;
    int64_t frac = fx_frac_get(pos);
    int64_t lo = fx_hash_unit_get(key ^ (uint64_t)cell);
    int64_t hi = fx_hash_unit_get(key ^ (uint64_t)(cell + LDG_TERM_FX_STEP));
    int64_t ease = fx_mul(fx_mul(frac, frac), (LDG_TERM_FX_CUBIC_COEF * LDG_TERM_FX_ONE) - (LDG_TERM_FX_PAIR * frac));

    return fx_mix(lo, hi, ease);
}

static int64_t fx_log2_get(int64_t val)
{
    int64_t res = 0;
    uint32_t iter = 0;

    if (val <= 0) { return INT32_MIN; }

    while (val < LDG_TERM_FX_ONE)
    {
        val *= LDG_TERM_FX_PAIR;
        res -= LDG_TERM_FX_ONE;
    }

    while (val >= LDG_TERM_FX_PAIR * LDG_TERM_FX_ONE)
    {
        val /= LDG_TERM_FX_PAIR;
        res += LDG_TERM_FX_ONE;
    }

    for (iter = 0; iter < LDG_TERM_FX_LOG2_ITER; iter++)
    {
        val = fx_mul(val, val);

        if (val >= LDG_TERM_FX_PAIR * LDG_TERM_FX_ONE)
        {
            val /= LDG_TERM_FX_PAIR;
            res += LDG_TERM_FX_ONE >> (iter + LDG_TERM_FX_STEP);
        }
    }

    return res;
}

static int64_t fx_exp2_get(int64_t val)
{
    int64_t whole = fx_floor_get(val) / LDG_TERM_FX_ONE;
    int64_t frac = fx_frac_get(val);
    int64_t res = 0;

    res = LDG_TERM_FX_ONE + fx_mul(frac, LDG_TERM_FX_EXP2_C1 + fx_mul(frac, LDG_TERM_FX_EXP2_C2 + fx_mul(frac, LDG_TERM_FX_EXP2_C3 + fx_mul(frac, LDG_TERM_FX_EXP2_C4))));

    while (whole < 0 && res > 0)
    {
        res /= LDG_TERM_FX_PAIR;
        whole++;
    }

    while (whole > 0)
    {
        res *= LDG_TERM_FX_PAIR;
        whole--;
    }

    return res;
}

static int64_t fx_pow_get(int64_t base, int64_t power)
{
    if (base <= 0) { return LDG_TERM_FX_NONE_FIXED; }

    if (power == 0) { return LDG_TERM_FX_ONE; }

    return fx_exp2_get(fx_mul(fx_log2_get(base), power));
}

static void fx_color_set(ldg_term_render_color_t *out, uint8_t red, uint8_t green, uint8_t blue)
{
    out->red = red;
    out->green = green;
    out->blue = blue;
    out->pudding[0] = 0;
}

static void fx_style_crt_set(ldg_term_fx_style_t *out)
{
    fx_color_set(&out->tint, LDG_TERM_FX_CRT_TINT_RED, LDG_TERM_FX_CRT_TINT_GREEN, LDG_TERM_FX_CRT_TINT_BLUE);
    fx_color_set(&out->hot, LDG_TERM_FX_CRT_HOT_RED, LDG_TERM_FX_CRT_HOT_GREEN, LDG_TERM_FX_CRT_HOT_BLUE);
    out->beam_color = out->tint;
    out->bright = LDG_TERM_FX_CRT_BRIGHT;
    out->noise = LDG_TERM_FX_CRT_NOISE;
    out->phosphor = LDG_TERM_FX_CRT_PHOSPHOR;
    out->bloom = LDG_TERM_FX_CRT_BLOOM;
    out->edge_fade = LDG_TERM_FX_CRT_EDGE_FADE;
    out->curve = LDG_TERM_FX_CRT_CURVE;
    out->overscan = LDG_TERM_FX_CRT_OVERSCAN;
    out->zoom = LDG_TERM_FX_CRT_ZOOM;
    out->scanline_gap = LDG_TERM_FX_CRT_SCANLINE_GAP;
    out->scanline_scroll = LDG_TERM_FX_CRT_SCROLL;
    out->line_cunt = LDG_TERM_FX_CRT_LINE_CUNT;
    out->mask_pitch = LDG_TERM_FX_MASK_PITCH;
    out->beam_rate = LDG_TERM_FX_CRT_BEAM_RATE;
    out->beam_intensity = LDG_TERM_FX_CRT_BEAM_INTENSITY;
    out->beam_width = LDG_TERM_FX_CRT_BEAM_WIDTH;
    out->warmth = LDG_TERM_FX_CRT_WARMTH;
    out->ripple = LDG_TERM_FX_CRT_RIPPLE;
    out->mains_rate = LDG_TERM_FX_MAINS_RATE;
    out->cell_cunt_x = LDG_TERM_FX_PANEL_CELL_CUNT_X;
    out->cell_cunt_y = LDG_TERM_FX_PANEL_CELL_CUNT_Y;
    out->haze = LDG_TERM_FX_CRT_HAZE;
    out->bloom_threshold = LDG_TERM_FX_BLOOM_THRESHOLD;
    out->bloom_gain = LDG_TERM_FX_BLOOM_GAIN;
    out->bloom_radius = LDG_TERM_FX_BLOOM_RADIUS;
    out->beam_en = LDG_TRUTH_TRUE;
    out->mono = LDG_TRUTH_TRUE;
}

static void fx_style_vga_set(ldg_term_fx_style_t *out)
{
    fx_color_set(&out->tint, LDG_TERM_FX_VGA_TINT_LEVEL, LDG_TERM_FX_VGA_TINT_LEVEL, LDG_TERM_FX_VGA_TINT_LEVEL);
    out->hot = out->tint;
    out->beam_color = out->tint;
    out->curve = LDG_TERM_FX_VGA_CURVE;
    out->scanline = LDG_TERM_FX_VGA_SCANLINE;
    out->scanline_gap = LDG_TERM_FX_VGA_SCANLINE_GAP;
    out->scanline_scroll = 0;
    out->line_cunt = LDG_TERM_FX_VGA_LINE_CUNT;
    out->phosphor = LDG_TERM_FX_VGA_PHOSPHOR;
    out->bloom = LDG_TERM_FX_VGA_BLOOM;
    out->noise = LDG_TERM_FX_VGA_NOISE;
    out->mask = LDG_TERM_FX_VGA_MASK;
    out->converge = LDG_TERM_FX_VGA_CONVERGE;
    out->bright = LDG_TERM_FX_VGA_BRIGHT;
    out->warmth = LDG_TERM_FX_VGA_WARMTH;
    out->ripple = LDG_TERM_FX_VGA_RIPPLE;
    out->haze = LDG_TERM_FX_VGA_HAZE;
    out->mono = LDG_TRUTH_FALSE;
}

static void fx_style_cga_set(ldg_term_fx_style_t *out)
{
    fx_color_set(&out->tint, LDG_TERM_FX_CGA_TINT_RED, LDG_TERM_FX_CGA_TINT_GREEN, LDG_TERM_FX_CGA_TINT_BLUE);
    fx_color_set(&out->hot, LDG_TERM_FX_CGA_HOT_RED, LDG_TERM_FX_CGA_HOT_GREEN, LDG_TERM_FX_CGA_HOT_BLUE);
    out->beam_color = out->tint;
    out->curve = 0;
    out->overscan = 0;
    out->zoom = 0;
    out->scanline = 0;
    out->scanline_gap = 0;
    out->scanline_scroll = 0;
    out->mask = 0;
    out->converge = 0;
    out->warmth = 0;
    out->phosphor = LDG_TERM_FX_CGA_PHOSPHOR;
    out->bloom = LDG_TERM_FX_CGA_BLOOM;
    out->noise = LDG_TERM_FX_CGA_NOISE;
    out->edge_fade = LDG_TERM_FX_CGA_EDGE_FADE;
    out->bright = LDG_TERM_FX_CGA_BRIGHT;
    out->ripple = LDG_TERM_FX_CGA_RIPPLE;
    out->cell = LDG_TERM_FX_CGA_CELL;
    out->bleed = LDG_TERM_FX_CGA_BLEED;
    out->sustain = LDG_TERM_FX_CGA_SUSTAIN;
    out->pedestal = LDG_TERM_FX_CGA_PEDESTAL;
    out->haze = LDG_TERM_FX_CGA_HAZE;
    out->band = LDG_TERM_FX_CGA_BAND;
    out->addr_rate = LDG_TERM_FX_CGA_ADDR_RATE;
    out->beam_en = LDG_TRUTH_FALSE;
}

static void fx_style_scope_set(ldg_term_fx_style_t *out)
{
    fx_color_set(&out->tint, LDG_TERM_FX_SCOPE_TINT_RED, LDG_TERM_FX_SCOPE_TINT_GREEN, LDG_TERM_FX_SCOPE_TINT_BLUE);
    fx_color_set(&out->hot, LDG_TERM_FX_SCOPE_HOT_RED, LDG_TERM_FX_SCOPE_HOT_GREEN, LDG_TERM_FX_SCOPE_HOT_BLUE);
    out->beam_color = out->tint;
    out->phosphor = LDG_TERM_FX_SCOPE_PHOSPHOR;
    out->bloom = LDG_TERM_FX_SCOPE_BLOOM;
    out->scanline = LDG_TERM_FX_SCOPE_SCANLINE;
    out->line_cunt = LDG_TERM_FX_VGA_LINE_CUNT;
    out->haze = LDG_TERM_FX_SCOPE_HAZE;
    out->mask = 0;
    out->beam_en = LDG_TRUTH_FALSE;
}

uint32_t ldg_term_fx_style_default_get(uint32_t kind, ldg_term_fx_style_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(kind >= LDG_TERM_FX_KIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_zero(out, (uint64_t)sizeof(*out));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fx_style_crt_set(out);
    out->kind = kind;

    if (kind == LDG_TERM_FX_KIND_VGA) { fx_style_vga_set(out); }
    else if (kind == LDG_TERM_FX_KIND_CGA) { fx_style_cga_set(out); }
    else if (kind == LDG_TERM_FX_KIND_SCOPE) { fx_style_scope_set(out); }

    return LDG_ERR_AOK;
}

static uint32_t fx_lerp_get(uint32_t cold, uint32_t peak, uint32_t mix)
{
    int64_t span = (int64_t)peak - (int64_t)cold;

    return (uint32_t)((int64_t)cold + ((span * (int64_t)mix) / LDG_TERM_FX_PER_MILLE));
}

uint32_t ldg_term_fx_style_lerp(const ldg_term_fx_style_t *cold, const ldg_term_fx_style_t *peak, const ldg_term_fx_mix_t *mix, ldg_term_fx_style_t *out)
{
    if (LDG_UNLIKELY(!cold || !peak || !mix || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(mix->bright > LDG_TERM_FX_PER_MILLE || mix->beam > LDG_TERM_FX_PER_MILLE || mix->converge > LDG_TERM_FX_PER_MILLE || mix->bloom > LDG_TERM_FX_PER_MILLE)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY(mix->haze > LDG_TERM_FX_PER_MILLE || mix->noise > LDG_TERM_FX_PER_MILLE || mix->warmth > LDG_TERM_FX_PER_MILLE)) { return LDG_ERR_RANGE; }

    *out = *peak;
    out->bright = fx_lerp_get(cold->bright, peak->bright, mix->bright);
    out->beam_intensity = fx_lerp_get(cold->beam_intensity, peak->beam_intensity, mix->beam);
    out->converge = fx_lerp_get(cold->converge, peak->converge, mix->converge);
    out->bloom = fx_lerp_get(cold->bloom, peak->bloom, mix->bloom);
    out->bloom_gain = fx_lerp_get(cold->bloom_gain, peak->bloom_gain, mix->bloom);
    out->haze = fx_lerp_get(cold->haze, peak->haze, mix->haze);
    out->noise = fx_lerp_get(cold->noise, peak->noise, mix->noise);
    out->warmth = fx_lerp_get(cold->warmth, peak->warmth, mix->warmth);

    return LDG_ERR_AOK;
}

uint32_t ldg_term_fx_conf_default_get(ldg_term_fx_conf_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, (uint64_t)sizeof(*out));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->col_max = LDG_TERM_FX_COL_MAX_DEFAULT;
    out->row_max = LDG_TERM_FX_ROW_MAX_DEFAULT;
    out->sample_col_cunt = LDG_TERM_RENDER_SAMPLE_COL_DEFAULT;
    out->sample_row_cunt = LDG_TERM_RENDER_SAMPLE_ROW_DEFAULT;
    out->persist_en = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t fx_buff_take(ldg_mem_pool_t *pool, uint64_t size, void **out)
{
    uint32_t ret = 0;

    ret = ldg_mem_pool_alloc(pool, size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

static uint32_t fx_storage_take(ldg_term_fx_t *fx)
{
    void *mem = 0x0;
    uint32_t ret = 0;

    ret = fx_buff_take(fx->pool, fx->sample_cap * LDG_TERM_FX_RGB_CUNT, &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fx->src = (uint8_t *)mem;

    ret = fx_buff_take(fx->pool, fx->bloom_cap * LDG_TERM_FX_RGB_CUNT * (uint64_t)sizeof(uint16_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fx->bloom_a = (uint16_t *)mem;

    ret = fx_buff_take(fx->pool, fx->bloom_cap * LDG_TERM_FX_RGB_CUNT * (uint64_t)sizeof(uint16_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fx->bloom_b = (uint16_t *)mem;

    ret = fx_buff_take(fx->pool, fx->col_cap * (uint64_t)sizeof(int64_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fx->col_center = (int64_t *)mem;

    ret = fx_buff_take(fx->pool, fx->col_cap * (uint64_t)sizeof(int64_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fx->col_square = (int64_t *)mem;

    ret = fx_buff_take(fx->pool, fx->row_cap * (uint64_t)sizeof(fx_row_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fx->row = (fx_row_t *)mem;

    if (fx->persist_en != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = fx_buff_take(fx->pool, fx->sample_cap * LDG_TERM_FX_RGB_CUNT, &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fx->persist = (uint8_t *)mem;

    ret = fx_buff_take(fx->pool, fx->sample_cap * LDG_TERM_FX_RGB_CUNT, &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fx->face = (uint8_t *)mem;

    return LDG_ERR_AOK;
}

static uint32_t fx_lut_build(ldg_term_fx_t *fx)
{
    ldg_term_render_fixed_t val = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    for (idx = 0; idx < LDG_TERM_FX_BYTE_CUNT; idx++) { fx->byte_fixed[idx] = (int32_t)(((int64_t)idx * LDG_TERM_FX_ONE) / LDG_TERM_FX_COLOR_MAX); }

    for (idx = 0; idx < LDG_TERM_FX_SIN_LUT_CUNT; idx++)
    {
        ret = ldg_term_render_fixed_sin_get((ldg_term_render_angle_t)(idx * LDG_TERM_FX_SIN_ANGLE_STEP), &val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        fx->sin_lut[idx] = (int32_t)val;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_fx_create(const ldg_term_fx_conf_t *conf, ldg_term_fx_t **out)
{
    ldg_mem_pool_t *pool = 0x0;
    ldg_term_fx_t *fx = 0x0;
    void *mem = 0x0;
    uint64_t sample_cap = 0;
    uint64_t bloom_cap = 0;
    uint64_t col_cap = 0;
    uint64_t row_cap = 0;
    uint64_t pool_cap = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(conf->col_max == 0 || conf->row_max == 0 || conf->sample_col_cunt == 0 || conf->sample_row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    col_cap = (uint64_t)conf->col_max * conf->sample_col_cunt;
    row_cap = (uint64_t)conf->row_max * conf->sample_row_cunt;

    if (LDG_UNLIKELY(col_cap > LDG_TERM_FX_EXTENT_MAX || row_cap > LDG_TERM_FX_EXTENT_MAX)) { return LDG_ERR_RANGE; }

    sample_cap = col_cap * row_cap;
    bloom_cap = (sample_cap / (LDG_TERM_FX_BLOOM_DIV * LDG_TERM_FX_BLOOM_DIV)) + LDG_TERM_FX_STEP;
    pool_cap = (uint64_t)sizeof(struct ldg_term_fx) + (sample_cap * LDG_TERM_FX_RGB_CUNT) + (LDG_TERM_FX_PAIR * bloom_cap * LDG_TERM_FX_RGB_CUNT * (uint64_t)sizeof(uint16_t)) + ((uint64_t)LDG_TERM_FX_POOL_ALLOC_CUNT * LDG_MEM_POOL_VAR_ALIGN);
    pool_cap += (LDG_TERM_FX_PAIR * col_cap * (uint64_t)sizeof(int64_t)) + (row_cap * (uint64_t)sizeof(fx_row_t));

    if (conf->persist_en == LDG_TRUTH_TRUE) { pool_cap += LDG_TERM_FX_PAIR * sample_cap * LDG_TERM_FX_RGB_CUNT; }

    ret = ldg_mem_pool_create(LDG_TERM_FX_POOL_ITEM_SIZE_VAR, pool_cap, &pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = fx_buff_take(pool, (uint64_t)sizeof(struct ldg_term_fx), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK)) { return LDG_ERR_MEM_POOL_INVALID; }

        return ret;
    }

    fx = (ldg_term_fx_t *)mem;
    fx->pool = pool;
    fx->sample_cap = sample_cap;
    fx->bloom_cap = bloom_cap;
    fx->col_cap = col_cap;
    fx->row_cap = row_cap;
    fx->persist_en = (conf->persist_en == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    fx->edge_lut_fade = LDG_TERM_FX_NONE;

    ret = fx_storage_take(fx);
    if (ret == LDG_ERR_AOK) { ret = fx_lut_build(fx); }

    if (ret == LDG_ERR_AOK) { ret = ldg_sys_entropy_get((uint8_t *)&fx->key, (uint32_t)sizeof(fx->key)); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK)) { return LDG_ERR_MEM_POOL_INVALID; }

        return ret;
    }

    *out = fx;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_fx_destroy(ldg_term_fx_t *fx)
{
    ldg_mem_pool_t *pool = 0x0;

    if (LDG_UNLIKELY(!fx)) { return LDG_ERR_FUNC_ARG_NULL; }

    pool = fx->pool;

    return ldg_mem_pool_destroy(&pool);
}

uint32_t ldg_term_fx_rst(ldg_term_fx_t *fx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!fx)) { return LDG_ERR_FUNC_ARG_NULL; }

    fx->face_valid = LDG_TRUTH_FALSE;
    fx->time_valid = LDG_TRUTH_FALSE;
    fx->frame = 0;

    if (fx->persist_en != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_mem_zero(fx->persist, fx->sample_cap * LDG_TERM_FX_RGB_CUNT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(fx->face, fx->sample_cap * LDG_TERM_FX_RGB_CUNT);
}

static int64_t fx_channel_fixed_get(uint8_t val)
{
    return ((int64_t)val * LDG_TERM_FX_ONE) / LDG_TERM_FX_COLOR_MAX;
}

static uint8_t fx_channel_byte_get(int64_t val)
{
    return (uint8_t)fx_clamp((val * LDG_TERM_FX_COLOR_MAX) / LDG_TERM_FX_ONE, LDG_TERM_FX_NONE_FIXED, LDG_TERM_FX_COLOR_MAX);
}

static int64_t fx_luma_get(const int64_t *col)
{
    return ((col[LDG_TERM_FX_RED_IDX] * LDG_TERM_FX_LUMA_RED) + (col[LDG_TERM_FX_GREEN_IDX] * LDG_TERM_FX_LUMA_GREEN) + (col[LDG_TERM_FX_BLUE_IDX] * LDG_TERM_FX_LUMA_BLUE)) / LDG_TERM_FX_ONE;
}

static void fx_color_fixed_get(const ldg_term_render_color_t *color, int64_t *out)
{
    out[LDG_TERM_FX_RED_IDX] = fx_channel_fixed_get(color->red);
    out[LDG_TERM_FX_GREEN_IDX] = fx_channel_fixed_get(color->green);
    out[LDG_TERM_FX_BLUE_IDX] = fx_channel_fixed_get(color->blue);
}

static uint64_t fx_local_idx_get(const fx_frame_t *frame, int64_t pos_u, int64_t pos_v)
{
    int64_t col = fx_clamp((pos_u * (int64_t)frame->width) / LDG_TERM_FX_ONE, LDG_TERM_FX_NONE_FIXED, (int64_t)frame->width - LDG_TERM_FX_STEP);
    int64_t row = fx_clamp((pos_v * (int64_t)frame->height) / LDG_TERM_FX_ONE, LDG_TERM_FX_NONE_FIXED, (int64_t)frame->height - LDG_TERM_FX_STEP);

    return (((uint64_t)row * frame->width) + (uint64_t)col) * LDG_TERM_FX_RGB_CUNT;
}

static void fx_fetch(const uint8_t *src, const fx_frame_t *frame, int64_t pos_u, int64_t pos_v, int64_t *out)
{
    uint64_t idx = fx_local_idx_get(frame, pos_u, pos_v);

    out[LDG_TERM_FX_RED_IDX] = fx_channel_fixed_get(src[idx + LDG_TERM_FX_RED_IDX]);
    out[LDG_TERM_FX_GREEN_IDX] = fx_channel_fixed_get(src[idx + LDG_TERM_FX_GREEN_IDX]);
    out[LDG_TERM_FX_BLUE_IDX] = fx_channel_fixed_get(src[idx + LDG_TERM_FX_BLUE_IDX]);
}

static int64_t fx_fetch_luma_get(const uint8_t *src, const fx_frame_t *frame, int64_t pos_u, int64_t pos_v)
{
    int64_t col[LDG_TERM_FX_RGB_CUNT] = LDG_ARR_ZERO_INIT;

    fx_fetch(src, frame, pos_u, pos_v, col);

    return fx_luma_get(col);
}

static void fx_edge_lut_build(ldg_term_fx_t *fx, uint32_t edge_fade)
{
    int64_t base = 0;
    int64_t power = fx_mille_get(edge_fade);
    uint32_t idx = 0;

    if (fx->edge_lut_valid == LDG_TRUTH_TRUE && fx->edge_lut_fade == edge_fade) { return; }

    for (idx = 0; idx < LDG_TERM_FX_EDGE_LUT_CUNT; idx++)
    {
        base = ((int64_t)idx * LDG_TERM_FX_ONE) / (LDG_TERM_FX_EDGE_LUT_CUNT - LDG_TERM_FX_STEP);
        fx->edge_lut[idx] = (uint32_t)fx_clamp(fx_pow_get(base, power), LDG_TERM_FX_NONE_FIXED, LDG_TERM_FX_ONE);
    }

    fx->edge_lut_fade = edge_fade;
    fx->edge_lut_valid = LDG_TRUTH_TRUE;
}

static int64_t fx_edge_mask_get(const ldg_term_fx_t *fx, int64_t pos_u, int64_t pos_v)
{
    int64_t span_u = fx_mul(LDG_TERM_FX_EDGE_SCALE * pos_u, LDG_TERM_FX_ONE - pos_u);
    int64_t span_v = fx_mul(LDG_TERM_FX_EDGE_SCALE * pos_v, LDG_TERM_FX_ONE - pos_v);
    int64_t prod = fx_clamp(fx_mul(span_u, span_v), LDG_TERM_FX_NONE_FIXED, LDG_TERM_FX_ONE);

    return fx->edge_lut[(prod * (LDG_TERM_FX_EDGE_LUT_CUNT - LDG_TERM_FX_STEP)) / LDG_TERM_FX_ONE];
}

static void fx_veil_build(ldg_term_fx_t *fx, const uint8_t *src, const fx_frame_t *frame)
{
    uint64_t sum[LDG_TERM_FX_VEIL_GRID * LDG_TERM_FX_VEIL_GRID] = LDG_ARR_ZERO_INIT;
    uint64_t cunt[LDG_TERM_FX_VEIL_GRID * LDG_TERM_FX_VEIL_GRID] = LDG_ARR_ZERO_INIT;
    uint32_t col_cell[LDG_TERM_FX_VEIL_GRID + LDG_TERM_FX_STEP] = LDG_ARR_ZERO_INIT;
    int64_t col[LDG_TERM_FX_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t idx = 0;
    uint32_t sample_x = 0;
    uint32_t sample_y = 0;
    uint32_t row_cell = 0;
    uint32_t grid = 0;
    uint32_t cell = 0;

    for (grid = 0; grid <= LDG_TERM_FX_VEIL_GRID; grid++) { col_cell[grid] = ((grid * frame->width) + LDG_TERM_FX_VEIL_LAST) / LDG_TERM_FX_VEIL_GRID; }

    for (sample_y = 0; sample_y < frame->height; sample_y++)
    {
        row_cell = ((sample_y * LDG_TERM_FX_VEIL_GRID) / frame->height) * LDG_TERM_FX_VEIL_GRID;

        for (grid = 0; grid < LDG_TERM_FX_VEIL_GRID; grid++)
        {
            cell = row_cell + grid;

            for (sample_x = col_cell[grid]; sample_x < col_cell[grid + LDG_TERM_FX_STEP]; sample_x++)
            {
                idx = (((uint64_t)sample_y * frame->width) + sample_x) * LDG_TERM_FX_RGB_CUNT;
                col[LDG_TERM_FX_RED_IDX] = fx->byte_fixed[src[idx + LDG_TERM_FX_RED_IDX]];
                col[LDG_TERM_FX_GREEN_IDX] = fx->byte_fixed[src[idx + LDG_TERM_FX_GREEN_IDX]];
                col[LDG_TERM_FX_BLUE_IDX] = fx->byte_fixed[src[idx + LDG_TERM_FX_BLUE_IDX]];
                sum[cell] += (uint64_t)(((col[LDG_TERM_FX_RED_IDX] * LDG_TERM_FX_LUMA_RED) + (col[LDG_TERM_FX_GREEN_IDX] * LDG_TERM_FX_LUMA_GREEN) + (col[LDG_TERM_FX_BLUE_IDX] * LDG_TERM_FX_LUMA_BLUE)) / LDG_TERM_FX_ONE);
                cunt[cell]++;
            }
        }
    }

    for (cell = 0; cell < LDG_TERM_FX_VEIL_GRID * LDG_TERM_FX_VEIL_GRID; cell++) { fx->veil[cell] = (cunt[cell] == 0) ? (uint32_t)LDG_TERM_FX_NONE_FIXED : (uint32_t)(sum[cell] / cunt[cell]); }
}

static int64_t fx_veil_get(const ldg_term_fx_t *fx, int64_t pos_u, int64_t pos_v)
{
    int64_t grid_u = fx_clamp((pos_u * LDG_TERM_FX_VEIL_GRID) - LDG_TERM_FX_HALF, LDG_TERM_FX_NONE_FIXED, (LDG_TERM_FX_VEIL_GRID - LDG_TERM_FX_STEP) * LDG_TERM_FX_ONE);
    int64_t grid_v = fx_clamp((pos_v * LDG_TERM_FX_VEIL_GRID) - LDG_TERM_FX_HALF, LDG_TERM_FX_NONE_FIXED, (LDG_TERM_FX_VEIL_GRID - LDG_TERM_FX_STEP) * LDG_TERM_FX_ONE);
    int64_t cell_u = grid_u / LDG_TERM_FX_ONE;
    int64_t cell_v = grid_v / LDG_TERM_FX_ONE;
    int64_t next_u = (cell_u + LDG_TERM_FX_STEP < LDG_TERM_FX_VEIL_GRID) ? cell_u + LDG_TERM_FX_STEP : cell_u;
    int64_t next_v = (cell_v + LDG_TERM_FX_STEP < LDG_TERM_FX_VEIL_GRID) ? cell_v + LDG_TERM_FX_STEP : cell_v;
    int64_t top = fx_mix(fx->veil[(cell_v * LDG_TERM_FX_VEIL_GRID) + cell_u], fx->veil[(cell_v * LDG_TERM_FX_VEIL_GRID) + next_u], fx_frac_get(grid_u));
    int64_t bottom = fx_mix(fx->veil[(next_v * LDG_TERM_FX_VEIL_GRID) + cell_u], fx->veil[(next_v * LDG_TERM_FX_VEIL_GRID) + next_u], fx_frac_get(grid_u));

    return fx_mix(top, bottom, fx_frac_get(grid_v));
}

static void fx_barrel(int64_t curve, int64_t *pos_u, int64_t *pos_v)
{
    int64_t center_u = (LDG_TERM_FX_PAIR * *pos_u) - LDG_TERM_FX_ONE;
    int64_t center_v = (LDG_TERM_FX_PAIR * *pos_v) - LDG_TERM_FX_ONE;
    int64_t factor = LDG_TERM_FX_ONE + fx_mul(curve, fx_mul(center_u, center_u) + fx_mul(center_v, center_v));

    *pos_u = (fx_mul(center_u, factor) / LDG_TERM_FX_PAIR) + LDG_TERM_FX_HALF;
    *pos_v = (fx_mul(center_v, factor) / LDG_TERM_FX_PAIR) + LDG_TERM_FX_HALF;
}

static uint8_t fx_unit_inside_is(int64_t pos_u, int64_t pos_v)
{
    if (pos_u < 0 || pos_u > LDG_TERM_FX_ONE || pos_v < 0 || pos_v > LDG_TERM_FX_ONE) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static int64_t fx_hum_get(const fx_frame_t *frame, int64_t pos_v)
{
    const ldg_term_fx_style_t *style = frame->style;
    int64_t phase = fx_mul(pos_v, LDG_TERM_FX_HUM_SLOPE) - fx_mul(fx_mul(frame->time, fx_mille_get(style->mains_rate)), LDG_TERM_FX_HUM_RATE);

    return fx_mul(fx_sin_cycles_get(phase), fx_mille_get(style->ripple));
}

static void fx_persist_apply(const ldg_term_fx_t *fx, const fx_frame_t *frame, uint64_t idx, int64_t *col)
{
    int64_t phosphor = fx_mille_get(frame->style->phosphor);
    int64_t prev = 0;
    uint32_t channel = 0;

    if (!fx->persist) { return; }

    for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++)
    {
        prev = fx_mul(fx_channel_fixed_get(fx->persist[idx + channel]), phosphor);

        if (prev > col[channel]) { col[channel] = prev; }
    }
}

static void fx_warm_apply(const fx_frame_t *frame, int64_t *col)
{
    col[LDG_TERM_FX_RED_IDX] = fx_mul(col[LDG_TERM_FX_RED_IDX], LDG_TERM_FX_ONE + fx_mul(frame->warm, LDG_TERM_FX_WARM_RED));
    col[LDG_TERM_FX_BLUE_IDX] = fx_mul(col[LDG_TERM_FX_BLUE_IDX], LDG_TERM_FX_ONE - fx_mul(frame->warm, LDG_TERM_FX_WARM_BLUE));
}

static void fx_tone_apply(const fx_frame_t *frame, int64_t drive, const int64_t *rgb, int64_t *col)
{
    const ldg_term_fx_style_t *style = frame->style;
    int64_t tint[LDG_TERM_FX_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t hot[LDG_TERM_FX_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t heat = fx_smoothstep(LDG_TERM_FX_HOT_LO, LDG_TERM_FX_HOT_HI, drive);
    uint32_t channel = 0;

    fx_color_fixed_get(&style->tint, tint);
    fx_color_fixed_get(&style->hot, hot);

    for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { col[channel] = (style->mono == LDG_TRUTH_TRUE || !rgb) ? fx_mul(fx_mix(tint[channel], hot[channel], heat), drive) : rgb[channel]; }
}

static int64_t fx_cell_drive_get(const uint8_t *src, const fx_frame_t *frame, int64_t center_u, int64_t center_v, int64_t span_u, int64_t span_v)
{
    const ldg_term_fx_style_t *style = frame->style;
    int64_t tap_u = span_u / LDG_TERM_FX_CELL_TAP_DIV;
    int64_t tap_v = span_v / LDG_TERM_FX_CELL_TAP_DIV;
    int64_t core = 0;
    int64_t near = 0;
    int64_t diag = 0;

    core = fx_fetch_luma_get(src, frame, center_u, center_v) * LDG_TERM_FX_CELL_CENTER_W;
    core += (fx_fetch_luma_get(src, frame, center_u + tap_u, center_v) + fx_fetch_luma_get(src, frame, center_u - tap_u, center_v)) * LDG_TERM_FX_CELL_EDGE_W;
    core += (fx_fetch_luma_get(src, frame, center_u, center_v + tap_v) + fx_fetch_luma_get(src, frame, center_u, center_v - tap_v)) * LDG_TERM_FX_CELL_EDGE_W;
    core += fx_fetch_luma_get(src, frame, center_u + tap_u, center_v + tap_v) + fx_fetch_luma_get(src, frame, center_u - tap_u, center_v - tap_v);
    core += fx_fetch_luma_get(src, frame, center_u + tap_u, center_v - tap_v) + fx_fetch_luma_get(src, frame, center_u - tap_u, center_v + tap_v);
    core /= LDG_TERM_FX_CELL_DIV;

    near = fx_fetch_luma_get(src, frame, center_u + span_u, center_v) + fx_fetch_luma_get(src, frame, center_u - span_u, center_v);
    near += fx_fetch_luma_get(src, frame, center_u, center_v + span_v) + fx_fetch_luma_get(src, frame, center_u, center_v - span_v);
    diag = fx_fetch_luma_get(src, frame, center_u + span_u, center_v + span_v) + fx_fetch_luma_get(src, frame, center_u - span_u, center_v - span_v);
    diag += fx_fetch_luma_get(src, frame, center_u + span_u, center_v - span_v) + fx_fetch_luma_get(src, frame, center_u - span_u, center_v + span_v);

    return core + (fx_mul(near, fx_mille_get(style->bleed)) / LDG_TERM_FX_BLEED_DIV) + (fx_mul(diag, fx_mille_get(style->bloom)) / LDG_TERM_FX_DIAG_DIV);
}

static int64_t fx_grid_get(uint32_t cunt, uint32_t extent)
{
    if (cunt == 0) { return LDG_TERM_FX_ONE; }

    if (cunt > extent) { return (int64_t)extent * LDG_TERM_FX_ONE; }

    return (int64_t)cunt * LDG_TERM_FX_ONE;
}

static void fx_plasma_sample(const ldg_term_fx_t *fx, const uint8_t *src, const fx_frame_t *frame, uint32_t sample_x, uint32_t sample_y, int64_t *out)
{
    const ldg_term_fx_style_t *style = frame->style;
    int64_t pos_u = ((int64_t)sample_x * LDG_TERM_FX_ONE + LDG_TERM_FX_HALF) / (int64_t)frame->width;
    int64_t pos_v = ((int64_t)sample_y * LDG_TERM_FX_ONE + LDG_TERM_FX_HALF) / (int64_t)frame->height;
    int64_t grid_u = fx_grid_get(style->cell_cunt_x, frame->width);
    int64_t grid_v = fx_grid_get(style->cell_cunt_y, frame->height);
    int64_t span_u = 0;
    int64_t span_v = 0;
    int64_t cell_u = 0;
    int64_t cell_v = 0;
    int64_t drive = 0;
    int64_t frac_u = 0;
    int64_t frac_v = 0;
    int64_t aperture = 0;
    int64_t cell_px = 0;
    int64_t lit = 0;
    int64_t row = 0;
    int64_t pulse = 0;
    uint64_t cell_id = 0;

    out[LDG_TERM_FX_RED_IDX] = LDG_TERM_FX_NONE_FIXED;
    out[LDG_TERM_FX_GREEN_IDX] = LDG_TERM_FX_NONE_FIXED;
    out[LDG_TERM_FX_BLUE_IDX] = LDG_TERM_FX_NONE_FIXED;

    fx_barrel(fx_mille_get(style->curve), &pos_u, &pos_v);

    if (fx_unit_inside_is(pos_u, pos_v) != LDG_TRUTH_TRUE) { return; }

    span_u = fx_div(LDG_TERM_FX_ONE, grid_u);
    span_v = fx_div(LDG_TERM_FX_ONE, grid_v);
    cell_u = fx_floor_get(fx_mul(pos_u, grid_u));
    cell_v = fx_floor_get(fx_mul(pos_v, grid_v));
    drive = fx_cell_drive_get(src, frame, fx_mul(cell_u + LDG_TERM_FX_HALF, span_u), fx_mul(cell_v + LDG_TERM_FX_HALF, span_v), span_u, span_v);

    frac_u = fx_frac_get(fx_mul(pos_u, grid_u));
    frac_v = fx_frac_get(fx_mul(pos_v, grid_v));
    aperture = fx_mul(fx_mul(fx_smoothstep(LDG_TERM_FX_NONE_FIXED, LDG_TERM_FX_APERTURE_EDGE, frac_u), fx_smoothstep(LDG_TERM_FX_NONE_FIXED, LDG_TERM_FX_APERTURE_EDGE, LDG_TERM_FX_ONE - frac_u)), fx_mul(fx_smoothstep(LDG_TERM_FX_NONE_FIXED, LDG_TERM_FX_APERTURE_EDGE, frac_v), fx_smoothstep(LDG_TERM_FX_NONE_FIXED, LDG_TERM_FX_APERTURE_EDGE, LDG_TERM_FX_ONE - frac_v)));
    cell_px = fx_mul((int64_t)frame->width * LDG_TERM_FX_ONE, span_u);

    if (fx_mul((int64_t)frame->height * LDG_TERM_FX_ONE, span_v) < cell_px) { cell_px = fx_mul((int64_t)frame->height * LDG_TERM_FX_ONE, span_v); }

    lit = fx_smoothstep(LDG_TERM_FX_CELL_LIT_LO, LDG_TERM_FX_CELL_LIT_HI, cell_px);
    drive = fx_mul(drive, fx_mix(LDG_TERM_FX_ONE, fx_mix(LDG_TERM_FX_ONE - fx_mille_get(style->cell), LDG_TERM_FX_ONE, aperture), lit));

    row = fx_mul(pos_v, grid_v) / LDG_TERM_FX_BAND_ROWS;
    drive = fx_mul(drive, LDG_TERM_FX_ONE - fx_mul(fx_mille_get(style->band), fx_mul(fx_frac_get(row), LDG_TERM_FX_BAND_RAMP) + fx_mul(fx_hash_unit_get(fx->key ^ (uint64_t)(fx_floor_get(row) + LDG_TERM_FX_HASH_ROW_SALT)), LDG_TERM_FX_BAND_HASH)));

    pulse = fx_mul(fx_mille_get(style->bright), LDG_TERM_FX_ONE + frame->ac + fx_hum_get(frame, pos_v) + frame->sustain) + fx_mul(frame->hw, LDG_TERM_FX_LIFT_HW);
    drive = fx_mul(drive, pulse) + fx_mille_get(style->pedestal) + fx_mul(fx_veil_get(fx, pos_u, pos_v), fx_mille_get(style->haze));
    drive = fx_mul(drive, fx_edge_mask_get(fx, pos_u, pos_v));
    cell_id = ((uint64_t)(cell_v / LDG_TERM_FX_ONE) * (uint64_t)style->cell_cunt_x) + (uint64_t)(cell_u / LDG_TERM_FX_ONE);
    drive += fx_mul(fx_hash_unit_get(frame->grain_seed ^ cell_id) - LDG_TERM_FX_HALF, fx_mille_get(style->noise));

    fx_tone_apply(frame, drive, 0x0, out);
}

static void fx_face_refresh(ldg_term_fx_t *fx, const fx_frame_t *frame, uint64_t elapsed)
{
    int64_t rate = 0;
    uint64_t total = (uint64_t)frame->width * frame->height * LDG_TERM_FX_RGB_CUNT;
    uint64_t idx = 0;

    if (elapsed > LDG_TERM_FX_ADDR_DT_MAX) { elapsed = LDG_TERM_FX_ADDR_DT_MAX; }

    rate = (fx_mille_get(frame->style->addr_rate) * (int64_t)elapsed) / LDG_TERM_FX_MS_PER_SEC;

    if (rate <= 0 || rate > LDG_TERM_FX_ONE || fx->face_valid != LDG_TRUTH_TRUE) { rate = LDG_TERM_FX_ONE; }

    for (idx = 0; idx < total; idx++) { fx->face[idx] = fx_channel_byte_get(fx_mix(fx_channel_fixed_get(fx->face[idx]), fx_channel_fixed_get(fx->src[idx]), rate)); }

    fx->face_valid = LDG_TRUTH_TRUE;
}

static void fx_gauss_build(ldg_term_fx_t *fx)
{
    int64_t sigma_square = fx_mul(LDG_TERM_FX_BLOOM_SIGMA, LDG_TERM_FX_BLOOM_SIGMA);
    int64_t expo = 0;
    uint32_t tap = 0;

    for (tap = 0; tap <= LDG_TERM_FX_BLOOM_TAP; tap++)
    {
        expo = fx_div((int64_t)tap * (int64_t)tap * LDG_TERM_FX_ONE, sigma_square * LDG_TERM_FX_PAIR);
        fx->gauss[tap] = fx_exp2_get(-fx_mul(expo, LDG_TERM_FX_LOG2_E));
    }
}

static void fx_bloom_cut(ldg_term_fx_t *fx, const ldg_term_render_t *render, const fx_frame_t *frame, uint32_t bloom_w, uint32_t bloom_h)
{
    const ldg_term_fx_style_t *style = frame->style;
    int64_t col[LDG_TERM_FX_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t luma = 0;
    int64_t keep = 0;
    uint64_t src_idx = 0;
    uint64_t dst_idx = 0;
    uint32_t cell_x = 0;
    uint32_t cell_y = 0;
    uint32_t sub_x = 0;
    uint32_t sub_y = 0;
    uint32_t channel = 0;

    for (cell_y = 0; cell_y < bloom_h; cell_y++)
    {
        for (cell_x = 0; cell_x < bloom_w; cell_x++)
        {
            col[LDG_TERM_FX_RED_IDX] = LDG_TERM_FX_NONE_FIXED;
            col[LDG_TERM_FX_GREEN_IDX] = LDG_TERM_FX_NONE_FIXED;
            col[LDG_TERM_FX_BLUE_IDX] = LDG_TERM_FX_NONE_FIXED;

            for (sub_y = 0; sub_y < LDG_TERM_FX_BLOOM_DIV; sub_y++) { for (sub_x = 0; sub_x < LDG_TERM_FX_BLOOM_DIV; sub_x++)
                {
                    src_idx = ((((uint64_t)frame->y_lo + (cell_y * LDG_TERM_FX_BLOOM_DIV) + sub_y) * frame->stride) + frame->x_lo + (cell_x * LDG_TERM_FX_BLOOM_DIV) + sub_x) * LDG_TERM_FX_RGB_CUNT;
                    for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { col[channel] += fx->byte_fixed[render->sample_color[src_idx + channel]]; }
                }
            }

            luma = fx_luma_get(col) / (LDG_TERM_FX_BLOOM_DIV * LDG_TERM_FX_BLOOM_DIV);
            keep = (luma > LDG_TERM_FX_NONE_FIXED) ? fx_div(fx_clamp(luma - fx_mille_get(style->bloom_threshold), LDG_TERM_FX_NONE_FIXED, INT64_MAX), luma) : LDG_TERM_FX_NONE_FIXED;
            dst_idx = (((uint64_t)cell_y * bloom_w) + cell_x) * LDG_TERM_FX_RGB_CUNT;

            for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { fx->bloom_a[dst_idx + channel] = (uint16_t)fx_clamp((fx_mul(fx_mul(col[channel] / (LDG_TERM_FX_BLOOM_DIV * LDG_TERM_FX_BLOOM_DIV), keep), fx_mille_get(style->bloom_gain)) * LDG_TERM_FX_COLOR_MAX) / LDG_TERM_FX_ONE, LDG_TERM_FX_NONE_FIXED, UINT16_MAX); }
        }
    }
}

static void fx_bloom_blur(const ldg_term_fx_t *fx, const uint16_t *src, uint16_t *dst, uint32_t bloom_w, uint32_t bloom_h, int64_t spacing, uint32_t axis)
{
    int64_t acc[LDG_TERM_FX_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t off[LDG_TERM_FX_BLOOM_TAP + LDG_TERM_FX_STEP] = LDG_ARR_ZERO_INIT;
    int64_t wsum = 0;
    int64_t at_x = 0;
    int64_t at_y = 0;
    int64_t sign = 0;
    int64_t last_x = (int64_t)bloom_w - LDG_TERM_FX_STEP;
    int64_t last_y = (int64_t)bloom_h - LDG_TERM_FX_STEP;
    int64_t val = 0;
    uint64_t idx = 0;
    uint32_t pos_x = 0;
    uint32_t pos_y = 0;
    uint32_t tap = 0;
    uint32_t side = 0;
    uint32_t channel = 0;

    wsum = fx->gauss[0];

    for (tap = LDG_TERM_FX_STEP; tap <= LDG_TERM_FX_BLOOM_TAP; tap++)
    {
        off[tap] = fx_mul((int64_t)tap * LDG_TERM_FX_ONE, spacing) / LDG_TERM_FX_ONE;
        wsum += LDG_TERM_FX_PAIR * fx->gauss[tap];
    }

    for (pos_y = 0; pos_y < bloom_h; pos_y++)
    {
        for (pos_x = 0; pos_x < bloom_w; pos_x++)
        {
            idx = (((uint64_t)pos_y * bloom_w) + pos_x) * LDG_TERM_FX_RGB_CUNT;

            for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { acc[channel] = (int64_t)src[idx + channel] * fx->gauss[0]; }

            for (tap = LDG_TERM_FX_STEP; tap <= LDG_TERM_FX_BLOOM_TAP; tap++)
            {
                for (side = 0; side < LDG_TERM_FX_PAIR; side++)
                {
                    sign = (int64_t)(side * LDG_TERM_FX_PAIR) - LDG_TERM_FX_STEP;
                    at_x = (int64_t)pos_x;
                    at_y = (int64_t)pos_y;

                    if (axis == LDG_TERM_FX_AXIS_X) { at_x += sign * off[tap]; }
                    else { at_y += sign * off[tap]; }

                    if (at_x < 0) { at_x = LDG_TERM_FX_NONE_FIXED; }

                    if (at_x > last_x) { at_x = last_x; }

                    if (at_y < 0) { at_y = LDG_TERM_FX_NONE_FIXED; }

                    if (at_y > last_y) { at_y = last_y; }

                    for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { acc[channel] += (int64_t)src[((((uint64_t)at_y * bloom_w) + (uint64_t)at_x) * LDG_TERM_FX_RGB_CUNT) + channel] * fx->gauss[tap]; }
                }
            }

            for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++)
            {
                val = acc[channel] / wsum;

                if (val < 0) { val = LDG_TERM_FX_NONE_FIXED; }

                if (val > UINT16_MAX) { val = UINT16_MAX; }

                dst[idx + channel] = (uint16_t)val;
            }
        }
    }
}

static void fx_bloom_apply(ldg_term_fx_t *fx, ldg_term_render_t *render, const fx_frame_t *frame)
{
    const ldg_term_fx_style_t *style = frame->style;
    uint64_t dst_idx = 0;
    uint64_t src_idx = 0;
    uint32_t bloom_w = frame->width / LDG_TERM_FX_BLOOM_DIV;
    uint32_t bloom_h = frame->height / LDG_TERM_FX_BLOOM_DIV;
    uint32_t sample_x = 0;
    uint32_t sample_y = 0;
    uint32_t cell_x = 0;
    uint32_t cell_y = 0;
    uint32_t channel = 0;
    int64_t spacing = 0;
    int64_t val = 0;

    if (style->bloom_gain == 0 || bloom_w == 0 || bloom_h == 0) { return; }

    spacing = fx_mille_get(style->bloom_radius) / (LDG_TERM_FX_BLOOM_DIV * LDG_TERM_FX_BLOOM_TAP);

    fx_bloom_cut(fx, render, frame, bloom_w, bloom_h);
    fx_bloom_blur(fx, fx->bloom_a, fx->bloom_b, bloom_w, bloom_h, spacing, LDG_TERM_FX_AXIS_X);
    fx_bloom_blur(fx, fx->bloom_b, fx->bloom_a, bloom_w, bloom_h, spacing, LDG_TERM_FX_AXIS_Y);

    for (sample_y = 0; sample_y < frame->height; sample_y++)
    {
        cell_y = sample_y / LDG_TERM_FX_BLOOM_DIV;

        if (cell_y >= bloom_h) { cell_y = bloom_h - LDG_TERM_FX_STEP; }

        for (sample_x = 0; sample_x < frame->width; sample_x++)
        {
            cell_x = sample_x / LDG_TERM_FX_BLOOM_DIV;

            if (cell_x >= bloom_w) { cell_x = bloom_w - LDG_TERM_FX_STEP; }

            dst_idx = ((((uint64_t)frame->y_lo + sample_y) * frame->stride) + frame->x_lo + sample_x) * LDG_TERM_FX_RGB_CUNT;
            src_idx = (((uint64_t)cell_y * bloom_w) + cell_x) * LDG_TERM_FX_RGB_CUNT;

            for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++)
            {
                val = (int64_t)render->sample_color[dst_idx + channel] + (int64_t)fx->bloom_a[src_idx + channel];

                if (val > LDG_TERM_FX_COLOR_MAX) { val = LDG_TERM_FX_COLOR_MAX; }

                render->sample_color[dst_idx + channel] = (uint8_t)val;
            }
        }
    }
}

static void fx_text_channel_apply(const fx_frame_t *frame, uint8_t *rgb, int64_t lift)
{
    const ldg_term_fx_style_t *style = frame->style;
    int64_t col[LDG_TERM_FX_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t tint[LDG_TERM_FX_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t luma = 0;
    uint32_t channel = 0;

    for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { col[channel] = fx_channel_fixed_get(rgb[channel]); }

    if (style->mono == LDG_TRUTH_TRUE)
    {
        fx_color_fixed_get(&style->tint, tint);
        luma = fx_luma_get(col);

        for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { col[channel] = fx_mul(tint[channel], luma); }
    }

    for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { col[channel] = fx_mul(col[channel], lift); }

    fx_warm_apply(frame, col);

    for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { rgb[channel] = fx_channel_byte_get(col[channel]); }
}

static void fx_text_apply(ldg_term_render_t *render, const fx_frame_t *frame)
{
    ldg_term_cell_t *cell = 0x0;
    int64_t lift = fx_mul(fx_mille_get(frame->style->bright), LDG_TERM_FX_ONE + frame->ac) + frame->lift;
    uint64_t idx = 0;
    uint32_t col = 0;
    uint32_t row = 0;

    for (row = render->clip.row; row < render->clip.row + render->clip.row_cunt; row++)
    {
        for (col = render->clip.col; col < render->clip.col + render->clip.col_cunt; col++)
        {
            idx = ((uint64_t)row * render->col_cunt) + col;

            if (render->text_mask[idx] != LDG_TRUTH_TRUE) { continue; }

            cell = &render->text_cell[idx];
            fx_text_channel_apply(frame, cell->fg, lift);
            fx_text_channel_apply(frame, cell->bg, lift);
        }
    }
}

static uint32_t fx_frame_build(ldg_term_fx_t *fx, const ldg_term_render_t *render, const ldg_term_fx_style_t *style, uint64_t time, fx_frame_t *out)
{
    int64_t cycles = 0;
    int64_t ripple = fx_mille_get(style->ripple);
    int64_t line_cunt = (int64_t)style->line_cunt * LDG_TERM_FX_ONE;
    uint32_t ret = 0;

    out->style = style;
    out->x_lo = render->clip_x_lo;
    out->y_lo = render->clip_y_lo;
    out->width = render->clip_x_hi - render->clip_x_lo;
    out->height = render->clip_y_hi - render->clip_y_lo;
    out->stride = render->width;

    if (LDG_UNLIKELY((uint64_t)out->width > fx->col_cap || (uint64_t)out->height > fx->row_cap)) { return LDG_ERR_RANGE; }

    out->time = ((int64_t)(time % LDG_TERM_FX_TIME_WRAP_MS) * LDG_TERM_FX_ONE) / LDG_TERM_FX_MS_PER_SEC;
    out->hw = fx_clamp(fx_mille_get(style->hw_noise), LDG_TERM_FX_NONE_FIXED, LDG_TERM_FX_ONE);
    cycles = fx_mul(out->time, fx_mille_get(style->mains_rate));

    if (style->kind == LDG_TERM_FX_KIND_CGA) { out->ac = fx_mul(fx_sin_cycles_get(cycles), fx_mul(ripple, LDG_TERM_FX_RIPPLE_SCALE)); }
    else { out->ac = fx_mul(fx_sin_cycles_get(cycles) + fx_mul(fx_sin_cycles_get(cycles * LDG_TERM_FX_PAIR), LDG_TERM_FX_RIPPLE_HARMONIC), fx_mul(ripple, LDG_TERM_FX_RIPPLE_SCALE)); }

    out->ac += fx_mul(fx_mul(out->hw, ripple), LDG_TERM_FX_RIPPLE_HW);
    out->sustain = fx_mul(fx_sin_cycles_get(cycles * LDG_TERM_FX_SUSTAIN_HARMONIC), fx_mille_get(style->sustain));
    out->hsync = fx_mul(fx_vnoise_get(fx->key, out->time * LDG_TERM_FX_HSYNC_RATE) - LDG_TERM_FX_HALF, LDG_TERM_FX_HSYNC_AMP) + fx_mul(out->hw, LDG_TERM_FX_HSYNC_HW);
    out->vwob = fx_mul(fx_sin_cycles_get(fx_mul(out->time, LDG_TERM_FX_VWOB_RATE)), LDG_TERM_FX_VWOB_AMP) + fx_mul(out->hw, LDG_TERM_FX_VWOB_HW);
    out->lift = fx_mul(fx_vnoise_get(fx->key, out->time * LDG_TERM_FX_LIFT_RATE) - LDG_TERM_FX_HALF, LDG_TERM_FX_LIFT_AMP) + fx_mul(out->hw, LDG_TERM_FX_LIFT_HW);
    out->warm = fx_mul(fx_sin_cycles_get(fx_mul(out->time, LDG_TERM_FX_WARM_RATE)), fx_mille_get(style->warmth)) + fx_mul(fx_mul(out->hw, fx_mille_get(style->warmth)), LDG_TERM_FX_WARM_HW);
    out->beam_pos = fx_frac_get(fx_mul(out->time, fx_mille_get(style->beam_rate)));
    out->beam_span = (line_cunt > 0) ? fx_div(fx_mille_get(style->beam_width), line_cunt) : LDG_TERM_FX_NONE_FIXED;
    out->beam_lit = (out->height > 0) ? fx_smoothstep(LDG_TERM_FX_SMOOTH_FAR, LDG_TERM_FX_SMOOTH_NEAR, line_cunt / (int64_t)out->height) : LDG_TERM_FX_NONE_FIXED;
    out->triad_lit = (style->mask_pitch > 0) ? fx_smoothstep(LDG_TERM_FX_SMOOTH_FAR, LDG_TERM_FX_SMOOTH_NEAR, fx_div(LDG_TERM_FX_ONE, fx_mille_get(style->mask_pitch))) : LDG_TERM_FX_NONE_FIXED;
    out->zoom = LDG_TERM_FX_ONE + fx_mille_get(style->zoom);
    out->curve = fx_mille_get(style->curve);
    out->overscan_inv = fx_div(LDG_TERM_FX_ONE, LDG_TERM_FX_ONE + fx_mille_get(style->overscan));
    out->converge = (out->width > 0) ? fx_mille_get(style->converge) / (int64_t)out->width : LDG_TERM_FX_NONE_FIXED;
    out->bloom = fx_mille_get(style->bloom);
    out->line = line_cunt;
    out->scroll = fx_mul(out->time, fx_mille_get(style->scanline_scroll));
    out->scanline = fx_mille_get(style->scanline);
    out->gap = fx_mille_get(style->scanline_gap);
    out->mask_en = (style->mask != 0 && style->mask_pitch != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    out->mask_w = fx_mul(fx_mille_get(style->mask), out->triad_lit);
    out->pitch_scale = (out->mask_en == LDG_TRUTH_TRUE) ? fx_div((int64_t)out->width * LDG_TERM_FX_ONE, fx_mille_get(style->mask_pitch)) : LDG_TERM_FX_NONE_FIXED;
    out->bright = fx_mille_get(style->bright);
    out->pedestal = fx_mille_get(style->pedestal);
    out->haze = fx_mille_get(style->haze);
    out->noise = fx_mille_get(style->noise);
    out->ripple = ripple;
    out->hum_t = fx_mul(fx_mul(out->time, fx_mille_get(style->mains_rate)), LDG_TERM_FX_HUM_RATE);
    out->phosphor = fx_mille_get(style->phosphor);
    out->warm_red = LDG_TERM_FX_ONE + fx_mul(out->warm, LDG_TERM_FX_WARM_RED);
    out->warm_blue = LDG_TERM_FX_ONE - fx_mul(out->warm, LDG_TERM_FX_WARM_BLUE);
    out->mono = (style->mono == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    fx_color_fixed_get(&style->tint, out->tint);
    fx_color_fixed_get(&style->hot, out->hot);

    ret = ldg_sys_entropy_get((uint8_t *)&out->grain_seed, (uint32_t)sizeof(out->grain_seed));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->grain_seed ^= fx_hash_get(fx->frame);

    return LDG_ERR_AOK;
}

static uint32_t fx_src_capture(ldg_term_fx_t *fx, const ldg_term_render_t *render, const fx_frame_t *frame)
{
    uint64_t src_idx = 0;
    uint64_t dst_idx = 0;
    uint64_t row_size = (uint64_t)frame->width * LDG_TERM_FX_RGB_CUNT;
    uint32_t sample_y = 0;
    uint32_t ret = 0;

    for (sample_y = 0; sample_y < frame->height; sample_y++)
    {
        src_idx = ((((uint64_t)frame->y_lo + sample_y) * frame->stride) + frame->x_lo) * LDG_TERM_FX_RGB_CUNT;
        dst_idx = (uint64_t)sample_y * row_size;

        ret = ldg_mem_cpy(fx->src + dst_idx, render->sample_color + src_idx, row_size);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static void fx_tube_tables_build(ldg_term_fx_t *fx, const fx_frame_t *frame)
{
    const ldg_term_fx_style_t *style = frame->style;
    fx_row_t *row = 0x0;
    int64_t beam_color[LDG_TERM_FX_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t pos = 0;
    int64_t pre = 0;
    int64_t glow = 0;
    int64_t dim = 0;
    uint32_t idx = 0;
    uint32_t channel = 0;

    for (idx = 0; idx < frame->width; idx++)
    {
        pos = (((int64_t)idx * LDG_TERM_FX_ONE) + LDG_TERM_FX_HALF) / (int64_t)frame->width;
        pre = fx_mul(pos + frame->hsync - LDG_TERM_FX_HALF, frame->zoom) + LDG_TERM_FX_HALF;
        fx->col_center[idx] = (LDG_TERM_FX_PAIR * pre) - LDG_TERM_FX_ONE;
        fx->col_square[idx] = fx_mul(fx->col_center[idx], fx->col_center[idx]);
    }

    fx_color_fixed_get(&style->beam_color, beam_color);

    for (idx = 0; idx < frame->height; idx++)
    {
        row = &fx->row[idx];
        pos = (((int64_t)idx * LDG_TERM_FX_ONE) + LDG_TERM_FX_HALF) / (int64_t)frame->height;
        pre = fx_mul(pos + frame->vwob - LDG_TERM_FX_HALF, frame->zoom) + LDG_TERM_FX_HALF;
        row->center_v = (LDG_TERM_FX_PAIR * pre) - LDG_TERM_FX_ONE;
        row->square_v = fx_mul(row->center_v, row->center_v);
        row->beam_mul = LDG_TERM_FX_ONE;

        for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { row->beam_add[channel] = LDG_TERM_FX_NONE_FIXED; }

        if (style->beam_en != LDG_TRUTH_TRUE) { continue; }

        glow = fx_mul(fx_smoothstep(frame->beam_span * LDG_TERM_FX_BEAM_GLOW_SPAN, LDG_TERM_FX_NONE_FIXED, fx_abs_get(pos - frame->beam_pos)), fx_mille_get(style->beam_intensity));
        dim = fx_smoothstep(frame->beam_pos + (frame->beam_span * LDG_TERM_FX_BEAM_DIM_AHEAD), frame->beam_pos - (frame->beam_span * LDG_TERM_FX_BEAM_DIM_BEHIND), pos);
        row->beam_mul = fx_mix(LDG_TERM_FX_ONE, LDG_TERM_FX_BEAM_DIM_FLOOR, dim);

        for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { row->beam_add[channel] = fx_mul(fx_mul(beam_color[channel], glow), LDG_TERM_FX_BEAM_GLOW_GAIN); }
    }
}

static void fx_tube_shade(const ldg_term_fx_t *fx, const uint8_t *src, const fx_frame_t *frame, uint32_t sample_x, uint32_t sample_y, int64_t *col)
{
    const fx_row_t *row = &fx->row[sample_y];
    int64_t near_sum[LDG_TERM_FX_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t far_sum[LDG_TERM_FX_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t hx[LDG_TERM_FX_HALO_TAP_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t hy[LDG_TERM_FX_HALO_TAP_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t phase[LDG_TERM_FX_WAVE_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t wave[LDG_TERM_FX_WAVE_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t width = (int64_t)frame->width;
    int64_t height = (int64_t)frame->height;
    int64_t factor = 0;
    int64_t warp_u = 0;
    int64_t warp_v = 0;
    int64_t scan_u = 0;
    int64_t scan_v = 0;
    int64_t sx = 0;
    int64_t sy = 0;
    int64_t sxr = 0;
    int64_t sxb = 0;
    uint32_t near_u = 0;
    uint32_t near_v = 0;
    int64_t dist = 0;
    int64_t shade = 0;
    int64_t profile = 0;
    int64_t grille = 0;
    int64_t lift = 0;
    int64_t grid_u = 0;
    int64_t grid_v = 0;
    int64_t cell_u = 0;
    int64_t cell_v = 0;
    int64_t next_u = 0;
    int64_t next_v = 0;
    int64_t frac_u = 0;
    int64_t frac_v = 0;
    int64_t top = 0;
    int64_t bottom = 0;
    int64_t glow = 0;
    int64_t span_u = 0;
    int64_t span_v = 0;
    int64_t prod = 0;
    int64_t edge = 0;
    int64_t drive = 0;
    int64_t ramp = 0;
    int64_t heat = 0;
    int64_t grain = 0;
    int64_t lo = 0;
    int64_t hi = 0;
    uint64_t near_idx = 0;
    uint64_t far_idx = 0;
    uint64_t hash = 0;
    uint64_t part = 0;
    uint32_t lut_idx = 0;
    uint32_t channel = 0;
    uint32_t tap = 0;

    col[LDG_TERM_FX_RED_IDX] = LDG_TERM_FX_NONE_FIXED;
    col[LDG_TERM_FX_GREEN_IDX] = LDG_TERM_FX_NONE_FIXED;
    col[LDG_TERM_FX_BLUE_IDX] = LDG_TERM_FX_NONE_FIXED;

    factor = LDG_TERM_FX_ONE + ((frame->curve * (fx->col_square[sample_x] + row->square_v)) / LDG_TERM_FX_ONE);
    warp_u = (((fx->col_center[sample_x] * factor) / LDG_TERM_FX_ONE) / LDG_TERM_FX_PAIR) + LDG_TERM_FX_HALF;
    warp_v = (((row->center_v * factor) / LDG_TERM_FX_ONE) / LDG_TERM_FX_PAIR) + LDG_TERM_FX_HALF;
    scan_u = (((warp_u - LDG_TERM_FX_HALF) * frame->overscan_inv) / LDG_TERM_FX_ONE) + LDG_TERM_FX_HALF;
    scan_v = (((warp_v - LDG_TERM_FX_HALF) * frame->overscan_inv) / LDG_TERM_FX_ONE) + LDG_TERM_FX_HALF;

    if (scan_u < 0 || scan_u > LDG_TERM_FX_ONE || scan_v < 0 || scan_v > LDG_TERM_FX_ONE) { return; }

    sx = (scan_u * width) / LDG_TERM_FX_ONE;
    sy = (scan_v * height) / LDG_TERM_FX_ONE;
    sxr = ((scan_u + frame->converge) * width) / LDG_TERM_FX_ONE;
    sxb = ((scan_u - frame->converge) * width) / LDG_TERM_FX_ONE;

    if (sx >= width) { sx = width - LDG_TERM_FX_STEP; }

    if (sy >= height) { sy = height - LDG_TERM_FX_STEP; }

    if (sxr >= width) { sxr = width - LDG_TERM_FX_STEP; }

    if (sxb >= width) { sxb = width - LDG_TERM_FX_STEP; }

    if (sxb < 0) { sxb = LDG_TERM_FX_NONE_FIXED; }

    col[LDG_TERM_FX_RED_IDX] = fx->byte_fixed[src[((((uint64_t)sy * frame->width) + (uint64_t)sxr) * LDG_TERM_FX_RGB_CUNT) + LDG_TERM_FX_RED_IDX]];
    col[LDG_TERM_FX_GREEN_IDX] = fx->byte_fixed[src[((((uint64_t)sy * frame->width) + (uint64_t)sx) * LDG_TERM_FX_RGB_CUNT) + LDG_TERM_FX_GREEN_IDX]];
    col[LDG_TERM_FX_BLUE_IDX] = fx->byte_fixed[src[((((uint64_t)sy * frame->width) + (uint64_t)sxb) * LDG_TERM_FX_RGB_CUNT) + LDG_TERM_FX_BLUE_IDX]];

    if (frame->bloom != 0)
    {
        hx[0] = sx - LDG_TERM_FX_HALO_FAR;
        hx[LDG_TERM_FX_HALO_CENTER - LDG_TERM_FX_STEP] = sx - LDG_TERM_FX_HALO_NEAR;
        hx[LDG_TERM_FX_HALO_CENTER] = sx;
        hx[LDG_TERM_FX_HALO_CENTER + LDG_TERM_FX_STEP] = sx + LDG_TERM_FX_HALO_NEAR;
        hx[LDG_TERM_FX_HALO_TAP_CUNT - LDG_TERM_FX_STEP] = sx + LDG_TERM_FX_HALO_FAR;
        hy[0] = sy - LDG_TERM_FX_HALO_FAR;
        hy[LDG_TERM_FX_HALO_CENTER - LDG_TERM_FX_STEP] = sy - LDG_TERM_FX_HALO_NEAR;
        hy[LDG_TERM_FX_HALO_CENTER] = sy;
        hy[LDG_TERM_FX_HALO_CENTER + LDG_TERM_FX_STEP] = sy + LDG_TERM_FX_HALO_NEAR;
        hy[LDG_TERM_FX_HALO_TAP_CUNT - LDG_TERM_FX_STEP] = sy + LDG_TERM_FX_HALO_FAR;

        for (tap = 0; tap < LDG_TERM_FX_HALO_TAP_CUNT; tap++)
        {
            if (hx[tap] < 0) { hx[tap] = LDG_TERM_FX_NONE_FIXED; }

            if (hx[tap] >= width) { hx[tap] = width - LDG_TERM_FX_STEP; }

            if (hy[tap] < 0) { hy[tap] = LDG_TERM_FX_NONE_FIXED; }

            if (hy[tap] >= height) { hy[tap] = height - LDG_TERM_FX_STEP; }
        }

        for (near_v = LDG_TERM_FX_HALO_CENTER - LDG_TERM_FX_STEP; near_v <= LDG_TERM_FX_HALO_CENTER + LDG_TERM_FX_STEP; near_v++)
        {
            for (near_u = LDG_TERM_FX_HALO_CENTER - LDG_TERM_FX_STEP; near_u <= LDG_TERM_FX_HALO_CENTER + LDG_TERM_FX_STEP; near_u++)
            {
                if (near_u == LDG_TERM_FX_HALO_CENTER && near_v == LDG_TERM_FX_HALO_CENTER) { continue; }

                near_idx = (((uint64_t)hy[near_v] * frame->width) + (uint64_t)hx[near_u]) * LDG_TERM_FX_RGB_CUNT;
                far_idx = (((uint64_t)hy[(near_v * LDG_TERM_FX_PAIR) - LDG_TERM_FX_HALO_CENTER] * frame->width) + (uint64_t)hx[(near_u * LDG_TERM_FX_PAIR) - LDG_TERM_FX_HALO_CENTER]) * LDG_TERM_FX_RGB_CUNT;

                for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++)
                {
                    near_sum[channel] += (int64_t)src[near_idx + channel];
                    far_sum[channel] += (int64_t)src[far_idx + channel];
                }
            }
        }

        for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { col[channel] += (((((near_sum[channel] * LDG_TERM_FX_HALO_FAR_DIV) + far_sum[channel]) * LDG_TERM_FX_ONE) / LDG_TERM_FX_HALO_NORM) * frame->bloom) / LDG_TERM_FX_ONE; }
    }

    dist = (int64_t)((uint64_t)(((warp_v * frame->line) / LDG_TERM_FX_ONE) + frame->scroll) & LDG_TERM_FX_FRAC_MASK) - LDG_TERM_FX_HALF;
    dist = ((dist < 0) ? -dist : dist) * LDG_TERM_FX_PAIR;
    shade = LDG_TERM_FX_ONE - ((((dist * dist) / LDG_TERM_FX_ONE) * frame->scanline) / LDG_TERM_FX_ONE);

    if (shade < frame->gap) { shade = frame->gap; }

    profile = LDG_TERM_FX_ONE + (((shade - LDG_TERM_FX_ONE) * frame->beam_lit) / LDG_TERM_FX_ONE);

    for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++)
    {
        col[channel] = (col[channel] * profile) / LDG_TERM_FX_ONE;
        phase[channel] = ((warp_u * frame->pitch_scale) / LDG_TERM_FX_ONE) - ((int64_t)channel * LDG_TERM_FX_THIRD) + LDG_TERM_FX_QUARTER_CYCLE;
    }

    phase[LDG_TERM_FX_WAVE_HUM] = ((warp_v * LDG_TERM_FX_HUM_SLOPE) / LDG_TERM_FX_ONE) - frame->hum_t;

    for (tap = 0; tap < LDG_TERM_FX_WAVE_CUNT; tap++)
    {
        part = (uint64_t)phase[tap] & LDG_TERM_FX_FRAC_MASK;
        lut_idx = (uint32_t)(part >> LDG_TERM_FX_SIN_LUT_SHIFT);
        lo = (int64_t)fx->sin_lut[lut_idx];
        hi = (int64_t)fx->sin_lut[(lut_idx + LDG_TERM_FX_STEP) & LDG_TERM_FX_SIN_LUT_MASK];
        wave[tap] = lo + (((hi - lo) * (int64_t)(part & LDG_TERM_FX_SIN_PART_MASK)) / LDG_TERM_FX_SIN_PART_ONE);
    }

    if (frame->mask_en == LDG_TRUTH_TRUE) { for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++)
        {
            grille = LDG_TERM_FX_GRILLE_BASE + ((LDG_TERM_FX_GRILLE_AMP * wave[channel]) / LDG_TERM_FX_ONE);
            col[channel] = (col[channel] * (LDG_TERM_FX_ONE + (((grille - LDG_TERM_FX_ONE) * frame->mask_w) / LDG_TERM_FX_ONE))) / LDG_TERM_FX_ONE;
        }
    }

    lift = ((frame->bright * (LDG_TERM_FX_ONE + frame->ac + ((wave[LDG_TERM_FX_WAVE_HUM] * frame->ripple) / LDG_TERM_FX_ONE))) / LDG_TERM_FX_ONE) + frame->lift;

    grid_u = (scan_u * LDG_TERM_FX_VEIL_GRID) - LDG_TERM_FX_HALF;
    grid_v = (scan_v * LDG_TERM_FX_VEIL_GRID) - LDG_TERM_FX_HALF;

    if (grid_u < 0) { grid_u = LDG_TERM_FX_NONE_FIXED; }

    if (grid_u > LDG_TERM_FX_VEIL_LAST * LDG_TERM_FX_ONE) { grid_u = LDG_TERM_FX_VEIL_LAST * LDG_TERM_FX_ONE; }

    if (grid_v < 0) { grid_v = LDG_TERM_FX_NONE_FIXED; }

    if (grid_v > LDG_TERM_FX_VEIL_LAST * LDG_TERM_FX_ONE) { grid_v = LDG_TERM_FX_VEIL_LAST * LDG_TERM_FX_ONE; }

    cell_u = grid_u / LDG_TERM_FX_ONE;
    cell_v = grid_v / LDG_TERM_FX_ONE;
    next_u = (cell_u < LDG_TERM_FX_VEIL_LAST) ? cell_u + LDG_TERM_FX_STEP : cell_u;
    next_v = (cell_v < LDG_TERM_FX_VEIL_LAST) ? cell_v + LDG_TERM_FX_STEP : cell_v;
    frac_u = grid_u - (cell_u * LDG_TERM_FX_ONE);
    frac_v = grid_v - (cell_v * LDG_TERM_FX_ONE);
    lo = (int64_t)fx->veil[(cell_v * LDG_TERM_FX_VEIL_GRID) + cell_u];
    top = lo + ((((int64_t)fx->veil[(cell_v * LDG_TERM_FX_VEIL_GRID) + next_u] - lo) * frac_u) / LDG_TERM_FX_ONE);
    lo = (int64_t)fx->veil[(next_v * LDG_TERM_FX_VEIL_GRID) + cell_u];
    bottom = lo + ((((int64_t)fx->veil[(next_v * LDG_TERM_FX_VEIL_GRID) + next_u] - lo) * frac_u) / LDG_TERM_FX_ONE);
    glow = top + (((bottom - top) * frac_v) / LDG_TERM_FX_ONE) - LDG_TERM_FX_VEIL_FLOOR;

    if (glow < 0) { glow = LDG_TERM_FX_NONE_FIXED; }

    glow = frame->pedestal + ((glow * frame->haze) / LDG_TERM_FX_ONE);

    span_u = ((LDG_TERM_FX_EDGE_SCALE * warp_u) * (LDG_TERM_FX_ONE - warp_u)) / LDG_TERM_FX_ONE;
    span_v = ((LDG_TERM_FX_EDGE_SCALE * warp_v) * (LDG_TERM_FX_ONE - warp_v)) / LDG_TERM_FX_ONE;
    prod = (span_u * span_v) / LDG_TERM_FX_ONE;

    if (prod < 0) { prod = LDG_TERM_FX_NONE_FIXED; }

    if (prod > LDG_TERM_FX_ONE) { prod = LDG_TERM_FX_ONE; }

    edge = (int64_t)fx->edge_lut[(prod * (LDG_TERM_FX_EDGE_LUT_CUNT - LDG_TERM_FX_STEP)) / LDG_TERM_FX_ONE];

    if (frame->mono == LDG_TRUTH_TRUE)
    {
        drive = ((col[LDG_TERM_FX_RED_IDX] * LDG_TERM_FX_LUMA_RED) + (col[LDG_TERM_FX_GREEN_IDX] * LDG_TERM_FX_LUMA_GREEN) + (col[LDG_TERM_FX_BLUE_IDX] * LDG_TERM_FX_LUMA_BLUE)) / LDG_TERM_FX_ONE;
        drive = ((((drive * lift) / LDG_TERM_FX_ONE) + glow) * edge) / LDG_TERM_FX_ONE;
        ramp = ((drive - LDG_TERM_FX_HOT_LO) * LDG_TERM_FX_ONE) / LDG_TERM_FX_HOT_SPAN;

        if (ramp < 0) { ramp = LDG_TERM_FX_NONE_FIXED; }

        if (ramp > LDG_TERM_FX_ONE) { ramp = LDG_TERM_FX_ONE; }

        heat = (((ramp * ramp) / LDG_TERM_FX_ONE) * ((LDG_TERM_FX_CUBIC_COEF * LDG_TERM_FX_ONE) - (LDG_TERM_FX_PAIR * ramp))) / LDG_TERM_FX_ONE;

        for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { col[channel] = ((frame->tint[channel] + (((frame->hot[channel] - frame->tint[channel]) * heat) / LDG_TERM_FX_ONE)) * drive) / LDG_TERM_FX_ONE; }
    }
    else { for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { col[channel] = ((((((col[channel] * lift) / LDG_TERM_FX_ONE) + glow) * frame->tint[channel]) / LDG_TERM_FX_ONE) * edge) / LDG_TERM_FX_ONE; } }

    col[LDG_TERM_FX_RED_IDX] = (col[LDG_TERM_FX_RED_IDX] * frame->warm_red) / LDG_TERM_FX_ONE;
    col[LDG_TERM_FX_BLUE_IDX] = (col[LDG_TERM_FX_BLUE_IDX] * frame->warm_blue) / LDG_TERM_FX_ONE;

    hash = (frame->grain_seed ^ (((uint64_t)sample_y * frame->width) + sample_x)) + LDG_TERM_FX_HASH_GOLDEN;
    hash = (hash ^ (hash >> LDG_TERM_FX_HASH_SHIFT_A)) * LDG_TERM_FX_HASH_MUL_A;
    hash = (hash ^ (hash >> LDG_TERM_FX_HASH_SHIFT_B)) * LDG_TERM_FX_HASH_MUL_B;
    hash ^= hash >> LDG_TERM_FX_HASH_SHIFT_C;
    grain = (((int64_t)(hash >> LDG_TERM_FX_HASH_FRAC_SHIFT) - LDG_TERM_FX_HALF) * frame->noise) / LDG_TERM_FX_ONE;

    for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++) { col[channel] = (((col[channel] + grain) * row->beam_mul) / LDG_TERM_FX_ONE) + row->beam_add[channel]; }
}

static void fx_tube_render(ldg_term_fx_t *fx, ldg_term_render_t *render, const fx_frame_t *frame, const uint8_t *src)
{
    int64_t col[LDG_TERM_FX_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t prev = 0;
    int64_t val = 0;
    uint64_t local_idx = 0;
    uint64_t dst_idx = 0;
    uint32_t sample_x = 0;
    uint32_t sample_y = 0;
    uint32_t channel = 0;

    fx_tube_tables_build(fx, frame);

    for (sample_y = 0; sample_y < frame->height; sample_y++)
    {
        local_idx = (uint64_t)sample_y * frame->width * LDG_TERM_FX_RGB_CUNT;
        dst_idx = ((((uint64_t)frame->y_lo + sample_y) * frame->stride) + frame->x_lo) * LDG_TERM_FX_RGB_CUNT;

        for (sample_x = 0; sample_x < frame->width; sample_x++)
        {
            fx_tube_shade(fx, src, frame, sample_x, sample_y, col);

            for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++)
            {
                if (fx->persist)
                {
                    prev = ((int64_t)fx->byte_fixed[fx->persist[local_idx + channel]] * frame->phosphor) / LDG_TERM_FX_ONE;

                    if (prev > col[channel]) { col[channel] = prev; }
                }

                val = (col[channel] * LDG_TERM_FX_COLOR_MAX) / LDG_TERM_FX_ONE;

                if (val < 0) { val = LDG_TERM_FX_NONE_FIXED; }

                if (val > LDG_TERM_FX_COLOR_MAX) { val = LDG_TERM_FX_COLOR_MAX; }

                render->sample_color[dst_idx + channel] = (uint8_t)val;

                if (fx->persist) { fx->persist[local_idx + channel] = (uint8_t)val; }
            }

            local_idx += LDG_TERM_FX_RGB_CUNT;
            dst_idx += LDG_TERM_FX_RGB_CUNT;
        }
    }
}

static void fx_region_render(ldg_term_fx_t *fx, ldg_term_render_t *render, const fx_frame_t *frame, const uint8_t *src)
{
    int64_t col[LDG_TERM_FX_RGB_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t local_idx = 0;
    uint64_t dst_idx = 0;
    uint32_t sample_x = 0;
    uint32_t sample_y = 0;
    uint32_t channel = 0;

    for (sample_y = 0; sample_y < frame->height; sample_y++)
    {
        for (sample_x = 0; sample_x < frame->width; sample_x++)
        {
            fx_plasma_sample(fx, src, frame, sample_x, sample_y, col);

            local_idx = (((uint64_t)sample_y * frame->width) + sample_x) * LDG_TERM_FX_RGB_CUNT;
            fx_persist_apply(fx, frame, local_idx, col);
            dst_idx = ((((uint64_t)frame->y_lo + sample_y) * frame->stride) + frame->x_lo + sample_x) * LDG_TERM_FX_RGB_CUNT;

            for (channel = 0; channel < LDG_TERM_FX_RGB_CUNT; channel++)
            {
                render->sample_color[dst_idx + channel] = fx_channel_byte_get(col[channel]);

                if (fx->persist) { fx->persist[local_idx + channel] = render->sample_color[dst_idx + channel]; }
            }
        }
    }
}

static uint8_t fx_style_valid_is(const ldg_term_fx_style_t *style)
{
    const uint32_t gain[LDG_TERM_FX_GAIN_FIELD_CUNT] = { style->bright, style->noise, style->hw_noise, style->phosphor, style->bloom, style->edge_fade, style->curve, style->overscan, style->zoom, style->scanline, style->scanline_gap, style->mask, style->converge, style->beam_intensity, style->warmth, style->ripple, style->cell, style->bleed, style->sustain, style->pedestal, style->haze, style->band, style->bloom_threshold, style->bloom_gain };
    const uint32_t rate[LDG_TERM_FX_RATE_FIELD_CUNT] = { style->line_cunt, style->mask_pitch, style->beam_rate, style->beam_width, style->mains_rate, style->scanline_scroll, style->addr_rate, style->bloom_radius };
    uint32_t idx = 0;

    for (idx = 0; idx < LDG_TERM_FX_GAIN_FIELD_CUNT; idx++) { if (gain[idx] > LDG_TERM_FX_GAIN_MAX) { return LDG_TRUTH_FALSE; } }

    for (idx = 0; idx < LDG_TERM_FX_RATE_FIELD_CUNT; idx++) { if (rate[idx] > LDG_TERM_FX_RATE_MAX) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_term_fx_apply(ldg_term_fx_t *fx, ldg_term_render_t *render, const ldg_term_fx_style_t *style, uint64_t time)
{
    fx_frame_t frame = LDG_STRUCT_ZERO_INIT;
    const uint8_t *src = 0x0;
    uint64_t elapsed = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!fx || !render || !style)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(style->kind >= LDG_TERM_FX_KIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(fx_style_valid_is(style) != LDG_TRUTH_TRUE)) { return LDG_ERR_RANGE; }

    ret = fx_frame_build(fx, render, style, time, &frame);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (frame.width == 0 || frame.height == 0) { return LDG_ERR_AOK; }

    fx_edge_lut_build(fx, style->edge_fade);
    fx_gauss_build(fx);
    ret = fx_src_capture(fx, render, &frame);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    src = fx->src;

    if (style->kind == LDG_TERM_FX_KIND_CGA && style->addr_rate > 0 && fx->face)
    {
        elapsed = (fx->time_valid == LDG_TRUTH_TRUE && time > fx->time_prev) ? time - fx->time_prev : LDG_TERM_FX_ADDR_DT_MAX;
        fx_face_refresh(fx, &frame, elapsed);
        src = fx->face;
    }

    fx_veil_build(fx, src, &frame);

    if (style->kind == LDG_TERM_FX_KIND_CGA) { fx_region_render(fx, render, &frame, src); }
    else { fx_tube_render(fx, render, &frame, src); }

    fx_bloom_apply(fx, render, &frame);
    fx_text_apply(render, &frame);

    fx->time_prev = time;
    fx->time_valid = LDG_TRUTH_TRUE;
    fx->frame++;

    return LDG_ERR_AOK;
}
