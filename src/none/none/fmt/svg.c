#include <stdint.h>

#include <dangling/fmt/svg.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/mem.h>
#include <dangling/str/str.h>

#define LDG_FMT_SVG_STEP 1
#define LDG_FMT_SVG_PAIR 2
#define LDG_FMT_SVG_X_IDX 0
#define LDG_FMT_SVG_Y_IDX 1
#define LDG_FMT_SVG_FIXED_ONE (1LL << LDG_FMT_SVG_FIXED_SHIFT)
#define LDG_FMT_SVG_CSS_RULE_MAX 128
#define LDG_FMT_SVG_GRAD_MAX 64
#define LDG_FMT_SVG_HREF_DEPTH_MAX 8
#define LDG_FMT_SVG_SEGMENT_MAX 64
#define LDG_FMT_SVG_SEGMENT_LEN (2LL * LDG_FMT_SVG_FIXED_ONE)
#define LDG_FMT_SVG_DIGIT_MAX 12
#define LDG_FMT_SVG_POW10_MAX 18
#define LDG_FMT_SVG_POOL_ALLOC_CUNT 8
#define LDG_FMT_SVG_ALLOC_MIN 1
#define LDG_FMT_SVG_PI 205887LL
#define LDG_FMT_SVG_TAU (LDG_FMT_SVG_PI * LDG_FMT_SVG_PAIR)
#define LDG_FMT_SVG_HALF_PI (LDG_FMT_SVG_PI / LDG_FMT_SVG_PAIR)
#define LDG_FMT_SVG_QUARTER_PI 51472LL
#define LDG_FMT_SVG_ATAN_C0 16037LL
#define LDG_FMT_SVG_ATAN_C1 4345LL
#define LDG_FMT_SVG_SIN_DIV_3 6
#define LDG_FMT_SVG_SIN_DIV_5 20
#define LDG_FMT_SVG_SIN_DIV_7 42
#define LDG_FMT_SVG_SIN_DIV_9 72
#define LDG_FMT_SVG_DEG_HALF_TURN 180
#define LDG_FMT_SVG_PERCENT 100
#define LDG_FMT_SVG_CUBIC_COEF 3
#define LDG_FMT_SVG_COLOR_SHORT_CUNT 3
#define LDG_FMT_SVG_COLOR_LONG_CUNT 6
#define LDG_FMT_SVG_NIBBLE_SHIFT 4
#define LDG_FMT_SVG_NIBBLE_REPEAT 17
#define LDG_FMT_SVG_FLAG_ON '1'
#define LDG_FMT_SVG_FLAG_OFF '0'
#define LDG_FMT_SVG_SQRT_BIT_START (1ULL << 62)
#define LDG_FMT_SVG_SQRT_BIT_STEP 2
#define LDG_FMT_SVG_HYPOT_MAG_MAX 0x7FFFFFFFULL
#define LDG_FMT_SVG_COORD_MAX ((int64_t)INT32_MAX)
#define LDG_FMT_SVG_UNIT_MAX (1LL << 46)
#define LDG_FMT_SVG_NONE UINT32_MAX
#define LDG_FMT_SVG_CMD_NONE 0
#define LDG_FMT_SVG_FIXED_NONE 0
#define LDG_FMT_SVG_EXP_MAX 1000
#define LDG_FMT_SVG_ID_LEAD '#'
#define LDG_FMT_SVG_CLASS_LEAD '.'
#define LDG_FMT_SVG_URL_CLOSE ')'
#define LDG_FMT_SVG_PERCENT_SIGN '%'
#define LDG_FMT_SVG_DECL_SEP ';'
#define LDG_FMT_SVG_DECL_ASSIGN ':'
#define LDG_FMT_SVG_SEL_SEP ','
#define LDG_FMT_SVG_BLK_OPEN '{'
#define LDG_FMT_SVG_BLK_CLOSE '}'
#define LDG_FMT_SVG_TAG_OPEN '<'
#define LDG_FMT_SVG_TAG_CLOSE '>'
#define LDG_FMT_SVG_TAG_SLASH '/'
#define LDG_FMT_SVG_ATTR_ASSIGN '='
#define LDG_FMT_SVG_QUOTE_DOUBLE '"'
#define LDG_FMT_SVG_QUOTE_SINGLE '\''
#define LDG_FMT_SVG_ARG_SEP ','
#define LDG_FMT_SVG_SIGN_POS '+'
#define LDG_FMT_SVG_SIGN_NEG '-'
#define LDG_FMT_SVG_POINT '.'
#define LDG_FMT_SVG_EXP_LOWER 'e'
#define LDG_FMT_SVG_EXP_UPPER 'E'
#define LDG_FMT_SVG_PAREN_OPEN '('
#define LDG_FMT_SVG_CHANNEL_CUNT 3
#define LDG_FMT_SVG_RED_IDX 0
#define LDG_FMT_SVG_GREEN_IDX 1
#define LDG_FMT_SVG_BLUE_IDX 2
#define LDG_FMT_SVG_CMD_MOV 'M'
#define LDG_FMT_SVG_CMD_LINE 'L'
#define LDG_FMT_SVG_CMD_HORIZONTAL 'H'
#define LDG_FMT_SVG_CMD_VERTICAL 'V'
#define LDG_FMT_SVG_CMD_CUBIC 'C'
#define LDG_FMT_SVG_CMD_CUBIC_SMOOTH 'S'
#define LDG_FMT_SVG_CMD_QUAD 'Q'
#define LDG_FMT_SVG_CMD_ARC 'A'
#define LDG_FMT_SVG_CMD_CLOSE 'Z'
#define LDG_FMT_SVG_ARG_MAX 6
#define LDG_FMT_SVG_ARG_PT_CUNT 2
#define LDG_FMT_SVG_ARG_CUBIC_CUNT 6
#define LDG_FMT_SVG_ARG_QUAD_CUNT 4
#define LDG_FMT_SVG_ARG_SINGLE_CUNT 1
#define LDG_FMT_SVG_ARG_A_IDX 0
#define LDG_FMT_SVG_ARG_B_IDX 2
#define LDG_FMT_SVG_ARG_C_IDX 4
#define LDG_FMT_SVG_CUBIC_WEIGHT_CUNT 4
#define LDG_FMT_SVG_WEIGHT_START_IDX 0
#define LDG_FMT_SVG_WEIGHT_CONTROL_A_IDX 1
#define LDG_FMT_SVG_WEIGHT_CONTROL_B_IDX 2
#define LDG_FMT_SVG_WEIGHT_END_IDX 3
#define LDG_FMT_SVG_ARC_RADIUS_X_IDX 0
#define LDG_FMT_SVG_ARC_RADIUS_Y_IDX 1
#define LDG_FMT_SVG_ARC_PHI_IDX 2
#define LDG_FMT_SVG_ARC_END_X_IDX 3
#define LDG_FMT_SVG_ARC_END_Y_IDX 4
#define LDG_FMT_SVG_ARC_LEAD_CUNT 3
#define LDG_FMT_SVG_COMMENT_OPEN_LEN 4
#define LDG_FMT_SVG_COMMENT_CLOSE_LEN 3
#define LDG_FMT_SVG_CDATA_OPEN_LEN 9
#define LDG_FMT_SVG_CDATA_CLOSE_LEN 3
#define LDG_FMT_SVG_CSS_COMMENT_LEN 2
#define LDG_FMT_SVG_PROLOG_MARK '?'
#define LDG_FMT_SVG_DECL_MARK '!'
#define LDG_FMT_SVG_TRANSLATE_ARG_MAX 2
#define LDG_FMT_SVG_COMMENT_OPEN "<!--"
#define LDG_FMT_SVG_COMMENT_CLOSE "-->"
#define LDG_FMT_SVG_CDATA_OPEN "<![CDATA["
#define LDG_FMT_SVG_CDATA_CLOSE "]]>"
#define LDG_FMT_SVG_CSS_COMMENT_OPEN "/*"
#define LDG_FMT_SVG_CSS_COMMENT_CLOSE "*/"
#define LDG_FMT_SVG_URL_OPEN "url("
#define LDG_FMT_SVG_VIEW_CUNT 4
#define LDG_FMT_SVG_VIEW_EXTENT_X_IDX 2
#define LDG_FMT_SVG_VIEW_EXTENT_Y_IDX 3

typedef enum svg_fill_kind
{
    SVG_FILL_KIND_UNSET = 0,
    SVG_FILL_KIND_NONE,
    SVG_FILL_KIND_COLOR,
    SVG_FILL_KIND_URL
} svg_fill_kind_t;

typedef struct svg_slice
{
    uint64_t off;
    uint64_t len;
} svg_slice_t;

typedef struct svg_css_rule
{
    uint64_t name_hash;
    svg_slice_t decl;
} svg_css_rule_t;

typedef struct svg_grad
{
    ldg_fmt_svg_stop_t stop[LDG_FMT_SVG_STOP_MAX];
    int64_t center_x;
    int64_t center_y;
    int64_t radius;
    uint64_t id_hash;
    uint64_t href_hash;
    uint32_t stop_cunt;
    uint8_t center_x_set;
    uint8_t center_y_set;
    uint8_t radius_set;
    uint8_t space_set;
    uint8_t user_space;
    uint8_t href_set;
    uint8_t pudding[6];
} svg_grad_t;

typedef struct svg_shape_src
{
    svg_slice_t sel;
    svg_slice_t style;
    svg_slice_t fill;
    svg_slice_t rule;
    int64_t shift_x;
    int64_t shift_y;
} svg_shape_src_t;

typedef struct svg_state
{
    svg_css_rule_t css_rule[LDG_FMT_SVG_CSS_RULE_MAX];
    svg_grad_t grad[LDG_FMT_SVG_GRAD_MAX];
    uint32_t css_rule_cunt;
    uint32_t grad_cunt;
    uint32_t grad_open;
    uint8_t pudding[4];
} svg_state_t;

typedef struct svg_sink
{
    const uint8_t *src;
    ldg_fmt_svg_t *svg;
    svg_state_t *state;
    svg_shape_src_t *shape_src;
    uint64_t len;
    uint64_t shape_cunt;
    uint64_t contour_cunt;
    uint64_t pt_cunt;
    uint64_t shape_cap;
    uint64_t contour_cap;
    uint64_t pt_cap;
    uint8_t contour_open;
    uint8_t view_set;
    uint8_t style_open;
    uint8_t pudding[5];
} svg_sink_t;

typedef struct svg_pen
{
    int64_t curr[LDG_FMT_SVG_PAIR];
    int64_t start[LDG_FMT_SVG_PAIR];
    int64_t control[LDG_FMT_SVG_PAIR];
    int64_t shift[LDG_FMT_SVG_PAIR];
    uint8_t control_cubic;
    uint8_t pudding[7];
} svg_pen_t;

typedef struct svg_fill
{
    uint64_t url_hash;
    uint32_t kind;
    uint32_t rule;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t pudding[5];
} svg_fill_t;

static int64_t svg_fixed_mul(int64_t lhs, int64_t rhs)
{
    return (lhs * rhs) / LDG_FMT_SVG_FIXED_ONE;
}

static int64_t svg_fixed_div(int64_t lhs, int64_t rhs)
{
    return (lhs * LDG_FMT_SVG_FIXED_ONE) / rhs;
}

static int64_t svg_abs_get(int64_t val)
{
    return (val < 0) ? -val : val;
}

static uint64_t svg_isqrt_get(uint64_t val)
{
    uint64_t res = 0;
    uint64_t bit = LDG_FMT_SVG_SQRT_BIT_START;

    while (bit > val) { bit >>= LDG_FMT_SVG_SQRT_BIT_STEP; }

    while (bit != 0)
    {
        if (val >= res + bit)
        {
            val -= res + bit;
            res = (res >> LDG_FMT_SVG_STEP) + bit;
        }
        else { res >>= LDG_FMT_SVG_STEP; }

        bit >>= LDG_FMT_SVG_SQRT_BIT_STEP;
    }

    return res;
}

static int64_t svg_fixed_sqrt_get(int64_t val)
{
    if (val <= 0) { return LDG_FMT_SVG_FIXED_NONE; }

    return (int64_t)svg_isqrt_get((uint64_t)val * (uint64_t)LDG_FMT_SVG_FIXED_ONE);
}

static int64_t svg_hypot_get(int64_t delta_x, int64_t delta_y)
{
    uint64_t mag_x = (uint64_t)svg_abs_get(delta_x);
    uint64_t mag_y = (uint64_t)svg_abs_get(delta_y);
    uint32_t shift = 0;

    while (mag_x > LDG_FMT_SVG_HYPOT_MAG_MAX || mag_y > LDG_FMT_SVG_HYPOT_MAG_MAX)
    {
        mag_x >>= LDG_FMT_SVG_STEP;
        mag_y >>= LDG_FMT_SVG_STEP;
        shift++;
    }

    return (int64_t)(svg_isqrt_get((mag_x * mag_x) + (mag_y * mag_y)) << shift);
}

static uint8_t svg_coord_valid_is(int64_t val)
{
    if (val > LDG_FMT_SVG_COORD_MAX || val + LDG_FMT_SVG_COORD_MAX < 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static int64_t svg_sin_get(int64_t angle)
{
    int64_t square = 0;
    int64_t term = 0;
    int64_t sum = 0;

    angle %= LDG_FMT_SVG_TAU;

    if (angle > LDG_FMT_SVG_PI) { angle -= LDG_FMT_SVG_TAU; }

    if (angle + LDG_FMT_SVG_PI < 0) { angle += LDG_FMT_SVG_TAU; }

    if (angle > LDG_FMT_SVG_HALF_PI) { angle = LDG_FMT_SVG_PI - angle; }

    if (angle + LDG_FMT_SVG_HALF_PI < 0) { angle = -(angle + LDG_FMT_SVG_PI); }

    square = svg_fixed_mul(angle, angle);
    term = angle;
    sum = angle;
    term = -svg_fixed_mul(term, square) / LDG_FMT_SVG_SIN_DIV_3;
    sum += term;
    term = -svg_fixed_mul(term, square) / LDG_FMT_SVG_SIN_DIV_5;
    sum += term;
    term = -svg_fixed_mul(term, square) / LDG_FMT_SVG_SIN_DIV_7;
    sum += term;
    term = -svg_fixed_mul(term, square) / LDG_FMT_SVG_SIN_DIV_9;
    sum += term;

    return sum;
}

static int64_t svg_atan_unit_get(int64_t ratio)
{
    int64_t mag = svg_abs_get(ratio);

    return svg_fixed_mul(LDG_FMT_SVG_QUARTER_PI, ratio) - svg_fixed_mul(svg_fixed_mul(ratio, mag - LDG_FMT_SVG_FIXED_ONE), LDG_FMT_SVG_ATAN_C0 + svg_fixed_mul(LDG_FMT_SVG_ATAN_C1, mag));
}

static int64_t svg_atan2_get(int64_t pos_y, int64_t pos_x)
{
    int64_t angle = 0;

    if (pos_x == 0 && pos_y == 0) { return angle; }

    if (svg_abs_get(pos_x) >= svg_abs_get(pos_y))
    {
        angle = svg_atan_unit_get(svg_fixed_div(pos_y, pos_x));

        if (pos_x < 0) { angle += (pos_y >= 0) ? LDG_FMT_SVG_PI : -LDG_FMT_SVG_PI; }

        return angle;
    }

    angle = (pos_y > 0) ? LDG_FMT_SVG_HALF_PI : -LDG_FMT_SVG_HALF_PI;

    return angle - svg_atan_unit_get(svg_fixed_div(pos_x, pos_y));
}

static uint8_t svg_sep_is(uint8_t ch)
{
    if (ldg_char_space_is(ch) == LDG_TRUTH_TRUE || ch == LDG_FMT_SVG_ARG_SEP) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t svg_slice_eq_is(const svg_sink_t *sink, const svg_slice_t *slice, const char *lit)
{
    uint64_t idx = 0;

    for (idx = 0; idx < slice->len; idx++) { if (lit[idx] == LDG_STR_TERM || (uint8_t)lit[idx] != sink->src[slice->off + idx]) { return LDG_TRUTH_FALSE; } }

    return (lit[slice->len] == LDG_STR_TERM) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t svg_prefix_is(const svg_sink_t *sink, uint64_t pos, uint64_t end, const char *lit)
{
    uint64_t idx = 0;

    for (idx = 0; lit[idx] != LDG_STR_TERM; idx++) { if (pos + idx >= end || sink->src[pos + idx] != (uint8_t)lit[idx]) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static uint64_t svg_find(const svg_sink_t *sink, uint64_t pos, uint64_t end, const char *lit)
{
    while (pos < end)
    {
        if (svg_prefix_is(sink, pos, end, lit) == LDG_TRUTH_TRUE) { return pos; }

        pos++;
    }

    return end;
}

static void svg_slice_trim(const svg_sink_t *sink, svg_slice_t *slice)
{
    while (slice->len > 0 && ldg_char_space_is(sink->src[slice->off]) == LDG_TRUTH_TRUE)
    {
        slice->off++;
        slice->len--;
    }

    while (slice->len > 0 && ldg_char_space_is(sink->src[slice->off + slice->len - LDG_FMT_SVG_STEP]) == LDG_TRUTH_TRUE) { slice->len--; }
}

static uint32_t svg_number_fixed_get(uint64_t mantissa, int64_t scale, int64_t *out)
{
    int64_t val = 0;
    int64_t divisor = LDG_FMT_SVG_STEP;
    int64_t step = 0;

    val = (int64_t)mantissa * LDG_FMT_SVG_FIXED_ONE;

    for (step = 0; step < scale; step++)
    {
        if (LDG_UNLIKELY(val > INT64_MAX / LDG_BASE_DEC)) { return LDG_ERR_RANGE; }

        val *= LDG_BASE_DEC;
    }

    if (scale + LDG_FMT_SVG_POW10_MAX < 0)
    {
        *out = LDG_FMT_SVG_FIXED_NONE;
        return LDG_ERR_AOK;
    }

    for (step = 0; step < -scale; step++) { divisor *= LDG_BASE_DEC; }

    val = (val + (divisor / LDG_FMT_SVG_PAIR)) / divisor;

    if (LDG_UNLIKELY(svg_coord_valid_is(val) != LDG_TRUTH_TRUE)) { return LDG_ERR_RANGE; }

    *out = val;

    return LDG_ERR_AOK;
}

static uint64_t svg_exp_parse(const uint8_t *src, uint64_t end, uint64_t at, int64_t *out)
{
    uint64_t look = at + LDG_FMT_SVG_STEP;
    int64_t exp_val = 0;
    uint8_t neg = LDG_TRUTH_FALSE;

    *out = 0;

    if (look < end && (src[look] == LDG_FMT_SVG_SIGN_POS || src[look] == LDG_FMT_SVG_SIGN_NEG))
    {
        neg = (src[look] == LDG_FMT_SVG_SIGN_NEG) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        look++;
    }

    if (look >= end || ldg_char_digit_is(src[look]) != LDG_TRUTH_TRUE) { return at; }

    while (look < end && ldg_char_digit_is(src[look]) == LDG_TRUTH_TRUE)
    {
        if (exp_val < LDG_FMT_SVG_EXP_MAX) { exp_val = (exp_val * LDG_BASE_DEC) + (int64_t)(src[look] - LDG_ASCII_DIGIT_ZERO); }

        look++;
    }

    *out = (neg == LDG_TRUTH_TRUE) ? -exp_val : exp_val;

    return look;
}

static uint32_t svg_number_parse(const uint8_t *src, uint64_t end, uint64_t *pos, int64_t *out)
{
    uint64_t mantissa = 0;
    uint64_t at = *pos;
    int64_t scale = 0;
    int64_t exp_val = 0;
    uint32_t digit_cunt = 0;
    uint32_t ret = 0;
    uint8_t neg = LDG_TRUTH_FALSE;
    uint8_t seen = LDG_TRUTH_FALSE;

    *out = 0;

    if (at < end && (src[at] == LDG_FMT_SVG_SIGN_POS || src[at] == LDG_FMT_SVG_SIGN_NEG))
    {
        neg = (src[at] == LDG_FMT_SVG_SIGN_NEG) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        at++;
    }

    while (at < end && ldg_char_digit_is(src[at]) == LDG_TRUTH_TRUE)
    {
        if (digit_cunt < LDG_FMT_SVG_DIGIT_MAX)
        {
            mantissa = (mantissa * LDG_BASE_DEC) + (uint64_t)(src[at] - LDG_ASCII_DIGIT_ZERO);
            digit_cunt++;
        }
        else { scale++; }

        seen = LDG_TRUTH_TRUE;
        at++;
    }

    if (at < end && src[at] == LDG_FMT_SVG_POINT)
    {
        at++;

        while (at < end && ldg_char_digit_is(src[at]) == LDG_TRUTH_TRUE)
        {
            if (digit_cunt < LDG_FMT_SVG_DIGIT_MAX)
            {
                mantissa = (mantissa * LDG_BASE_DEC) + (uint64_t)(src[at] - LDG_ASCII_DIGIT_ZERO);
                digit_cunt++;
                scale--;
            }

            seen = LDG_TRUTH_TRUE;
            at++;
        }
    }

    if (seen != LDG_TRUTH_TRUE) { return LDG_ERR_NOT_FOUND; }

    if (at < end && (src[at] == LDG_FMT_SVG_EXP_LOWER || src[at] == LDG_FMT_SVG_EXP_UPPER))
    {
        at = svg_exp_parse(src, end, at, &exp_val);
        scale += exp_val;
    }

    ret = svg_number_fixed_get(mantissa, scale, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (neg == LDG_TRUTH_TRUE) { *out = -*out; }

    *pos = at;

    return LDG_ERR_AOK;
}

static uint32_t svg_arg_next(const uint8_t *src, uint64_t end, uint64_t *pos, int64_t *out)
{
    while (*pos < end && svg_sep_is(src[*pos]) == LDG_TRUTH_TRUE) { (*pos)++; }

    return svg_number_parse(src, end, pos, out);
}

static uint32_t svg_flag_next(const uint8_t *src, uint64_t end, uint64_t *pos, uint8_t *out)
{
    *out = LDG_TRUTH_FALSE;

    while (*pos < end && svg_sep_is(src[*pos]) == LDG_TRUTH_TRUE) { (*pos)++; }

    if (LDG_UNLIKELY(*pos >= end)) { return LDG_ERR_INVALID; }

    if (src[*pos] == LDG_FMT_SVG_FLAG_ON) { *out = LDG_TRUTH_TRUE; }
    else if (LDG_UNLIKELY(src[*pos] != LDG_FMT_SVG_FLAG_OFF)) { return LDG_ERR_INVALID; }

    (*pos)++;

    return LDG_ERR_AOK;
}

static uint32_t svg_color_parse(const svg_sink_t *sink, const svg_slice_t *val, svg_fill_t *out)
{
    byte_t nibble[LDG_FMT_SVG_COLOR_LONG_CUNT] = LDG_ARR_ZERO_INIT;
    uint8_t channel[LDG_FMT_SVG_CHANNEL_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t digit_cunt = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    digit_cunt = val->len - LDG_FMT_SVG_STEP;

    if (LDG_UNLIKELY(val->len == 0 || sink->src[val->off] != LDG_FMT_SVG_ID_LEAD)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(digit_cunt != LDG_FMT_SVG_COLOR_SHORT_CUNT && digit_cunt != LDG_FMT_SVG_COLOR_LONG_CUNT)) { return LDG_ERR_UNSUPPORTED; }

    for (idx = 0; idx < digit_cunt; idx++)
    {
        ret = ldg_hex_to_nipple((char)sink->src[val->off + LDG_FMT_SVG_STEP + idx], &nibble[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }
    }

    for (idx = 0; idx < LDG_FMT_SVG_CHANNEL_CUNT; idx++) { if (digit_cunt == LDG_FMT_SVG_COLOR_SHORT_CUNT) { channel[idx] = (uint8_t)(nibble[idx] * LDG_FMT_SVG_NIBBLE_REPEAT); }
        else { channel[idx] = (uint8_t)((nibble[idx * LDG_FMT_SVG_PAIR] << LDG_FMT_SVG_NIBBLE_SHIFT) | nibble[(idx * LDG_FMT_SVG_PAIR) + LDG_FMT_SVG_STEP]); } }

    out->kind = SVG_FILL_KIND_COLOR;
    out->red = channel[LDG_FMT_SVG_RED_IDX];
    out->green = channel[LDG_FMT_SVG_GREEN_IDX];
    out->blue = channel[LDG_FMT_SVG_BLUE_IDX];

    return LDG_ERR_AOK;
}

static uint32_t svg_pt_emit(svg_sink_t *sink, const svg_pen_t *pen, int64_t pos_x, int64_t pos_y)
{
    int64_t out_x = pos_x + pen->shift[LDG_FMT_SVG_X_IDX];
    int64_t out_y = pos_y + pen->shift[LDG_FMT_SVG_Y_IDX];

    if (LDG_UNLIKELY(out_x > INT32_MAX || out_x < INT32_MIN || out_y > INT32_MAX || out_y < INT32_MIN)) { return LDG_ERR_RANGE; }

    if (sink->svg)
    {
        if (LDG_UNLIKELY(sink->pt_cunt >= sink->pt_cap || sink->contour_cunt == 0 || sink->contour_cunt > sink->contour_cap)) { return LDG_ERR_MEM_CORRUPTION; }

        sink->svg->pt[(sink->pt_cunt * LDG_FMT_SVG_PAIR) + LDG_FMT_SVG_X_IDX] = (int32_t)out_x;
        sink->svg->pt[(sink->pt_cunt * LDG_FMT_SVG_PAIR) + LDG_FMT_SVG_Y_IDX] = (int32_t)out_y;
        sink->svg->contour_len[sink->contour_cunt - LDG_FMT_SVG_STEP]++;
    }

    sink->pt_cunt++;

    if (LDG_UNLIKELY(sink->pt_cunt > UINT32_MAX)) { return LDG_ERR_RANGE; }

    return LDG_ERR_AOK;
}

static uint32_t svg_contour_begin(svg_sink_t *sink, const svg_pen_t *pen)
{
    if (sink->svg)
    {
        if (LDG_UNLIKELY(sink->contour_cunt >= sink->contour_cap)) { return LDG_ERR_MEM_CORRUPTION; }

        sink->svg->contour_len[sink->contour_cunt] = 0;
    }

    sink->contour_cunt++;
    sink->contour_open = LDG_TRUTH_TRUE;

    if (LDG_UNLIKELY(sink->contour_cunt > UINT32_MAX)) { return LDG_ERR_RANGE; }

    return svg_pt_emit(sink, pen, pen->curr[LDG_FMT_SVG_X_IDX], pen->curr[LDG_FMT_SVG_Y_IDX]);
}

static uint32_t svg_line_to(svg_sink_t *sink, svg_pen_t *pen, int64_t pos_x, int64_t pos_y)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(svg_coord_valid_is(pos_x) != LDG_TRUTH_TRUE || svg_coord_valid_is(pos_y) != LDG_TRUTH_TRUE)) { return LDG_ERR_RANGE; }

    if (sink->contour_open != LDG_TRUTH_TRUE)
    {
        ret = svg_contour_begin(sink, pen);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = svg_pt_emit(sink, pen, pos_x, pos_y);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pen->curr[LDG_FMT_SVG_X_IDX] = pos_x;
    pen->curr[LDG_FMT_SVG_Y_IDX] = pos_y;

    return LDG_ERR_AOK;
}

static int64_t svg_segment_cunt_get(int64_t len)
{
    int64_t cunt = (len / LDG_FMT_SVG_SEGMENT_LEN) + LDG_FMT_SVG_STEP;

    if (cunt > LDG_FMT_SVG_SEGMENT_MAX) { cunt = LDG_FMT_SVG_SEGMENT_MAX; }

    return cunt;
}

static uint32_t svg_cubic_to(svg_sink_t *sink, svg_pen_t *pen, const int64_t *control_a, const int64_t *control_b, const int64_t *end_pt)
{
    int64_t orig_pt[LDG_FMT_SVG_PAIR] = LDG_ARR_ZERO_INIT;
    int64_t seg_cunt = 0;
    int64_t cube = 0;
    int64_t step = 0;
    int64_t rest = 0;
    int64_t weight[LDG_FMT_SVG_CUBIC_WEIGHT_CUNT] = LDG_ARR_ZERO_INIT;
    int64_t pos[LDG_FMT_SVG_PAIR] = LDG_ARR_ZERO_INIT;
    uint32_t axis = 0;
    uint32_t ret = 0;

    orig_pt[LDG_FMT_SVG_X_IDX] = pen->curr[LDG_FMT_SVG_X_IDX];
    orig_pt[LDG_FMT_SVG_Y_IDX] = pen->curr[LDG_FMT_SVG_Y_IDX];

    seg_cunt = svg_segment_cunt_get(svg_hypot_get(control_a[LDG_FMT_SVG_X_IDX] - orig_pt[LDG_FMT_SVG_X_IDX], control_a[LDG_FMT_SVG_Y_IDX] - orig_pt[LDG_FMT_SVG_Y_IDX]) + svg_hypot_get(control_b[LDG_FMT_SVG_X_IDX] - control_a[LDG_FMT_SVG_X_IDX], control_b[LDG_FMT_SVG_Y_IDX] - control_a[LDG_FMT_SVG_Y_IDX]) + svg_hypot_get(end_pt[LDG_FMT_SVG_X_IDX] - control_b[LDG_FMT_SVG_X_IDX], end_pt[LDG_FMT_SVG_Y_IDX] - control_b[LDG_FMT_SVG_Y_IDX]));
    cube = seg_cunt * seg_cunt * seg_cunt;

    for (step = LDG_FMT_SVG_STEP; step <= seg_cunt; step++)
    {
        rest = seg_cunt - step;
        weight[LDG_FMT_SVG_WEIGHT_START_IDX] = rest * rest * rest;
        weight[LDG_FMT_SVG_WEIGHT_CONTROL_A_IDX] = LDG_FMT_SVG_CUBIC_COEF * rest * rest * step;
        weight[LDG_FMT_SVG_WEIGHT_CONTROL_B_IDX] = LDG_FMT_SVG_CUBIC_COEF * rest * step * step;
        weight[LDG_FMT_SVG_WEIGHT_END_IDX] = step * step * step;

        for (axis = 0; axis < LDG_FMT_SVG_PAIR; axis++) { pos[axis] = ((orig_pt[axis] * weight[LDG_FMT_SVG_WEIGHT_START_IDX]) + (control_a[axis] * weight[LDG_FMT_SVG_WEIGHT_CONTROL_A_IDX]) + (control_b[axis] * weight[LDG_FMT_SVG_WEIGHT_CONTROL_B_IDX]) + (end_pt[axis] * weight[LDG_FMT_SVG_WEIGHT_END_IDX])) / cube; }

        ret = svg_line_to(sink, pen, pos[LDG_FMT_SVG_X_IDX], pos[LDG_FMT_SVG_Y_IDX]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    pen->control[LDG_FMT_SVG_X_IDX] = control_b[LDG_FMT_SVG_X_IDX];
    pen->control[LDG_FMT_SVG_Y_IDX] = control_b[LDG_FMT_SVG_Y_IDX];

    return LDG_ERR_AOK;
}

static uint32_t svg_quad_to(svg_sink_t *sink, svg_pen_t *pen, const int64_t *control, const int64_t *end_pt)
{
    int64_t orig_pt[LDG_FMT_SVG_PAIR] = LDG_ARR_ZERO_INIT;
    int64_t pos[LDG_FMT_SVG_PAIR] = LDG_ARR_ZERO_INIT;
    int64_t seg_cunt = 0;
    int64_t square = 0;
    int64_t step = 0;
    int64_t rest = 0;
    uint32_t axis = 0;
    uint32_t ret = 0;

    orig_pt[LDG_FMT_SVG_X_IDX] = pen->curr[LDG_FMT_SVG_X_IDX];
    orig_pt[LDG_FMT_SVG_Y_IDX] = pen->curr[LDG_FMT_SVG_Y_IDX];

    seg_cunt = svg_segment_cunt_get(svg_hypot_get(control[LDG_FMT_SVG_X_IDX] - orig_pt[LDG_FMT_SVG_X_IDX], control[LDG_FMT_SVG_Y_IDX] - orig_pt[LDG_FMT_SVG_Y_IDX]) + svg_hypot_get(end_pt[LDG_FMT_SVG_X_IDX] - control[LDG_FMT_SVG_X_IDX], end_pt[LDG_FMT_SVG_Y_IDX] - control[LDG_FMT_SVG_Y_IDX]));
    square = seg_cunt * seg_cunt;

    for (step = LDG_FMT_SVG_STEP; step <= seg_cunt; step++)
    {
        rest = seg_cunt - step;

        for (axis = 0; axis < LDG_FMT_SVG_PAIR; axis++) { pos[axis] = ((orig_pt[axis] * rest * rest) + (control[axis] * LDG_FMT_SVG_PAIR * rest * step) + (end_pt[axis] * step * step)) / square; }

        ret = svg_line_to(sink, pen, pos[LDG_FMT_SVG_X_IDX], pos[LDG_FMT_SVG_Y_IDX]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    pen->control[LDG_FMT_SVG_X_IDX] = control[LDG_FMT_SVG_X_IDX];
    pen->control[LDG_FMT_SVG_Y_IDX] = control[LDG_FMT_SVG_Y_IDX];

    return LDG_ERR_AOK;
}

static uint32_t svg_arc_to(svg_sink_t *sink, svg_pen_t *pen, const int64_t *arg, uint8_t large, uint8_t sweep)
{
    int64_t start_x = pen->curr[LDG_FMT_SVG_X_IDX];
    int64_t start_y = pen->curr[LDG_FMT_SVG_Y_IDX];
    int64_t end_x = arg[LDG_FMT_SVG_ARC_END_X_IDX];
    int64_t end_y = arg[LDG_FMT_SVG_ARC_END_Y_IDX];
    int64_t radius_x = svg_abs_get(arg[LDG_FMT_SVG_ARC_RADIUS_X_IDX]);
    int64_t radius_y = svg_abs_get(arg[LDG_FMT_SVG_ARC_RADIUS_Y_IDX]);
    int64_t phi = 0;
    int64_t cos_phi = 0;
    int64_t sin_phi = 0;
    int64_t half_x = 0;
    int64_t half_y = 0;
    int64_t unit_x = 0;
    int64_t unit_y = 0;
    int64_t lam = 0;
    int64_t scale = 0;
    int64_t norm = 0;
    int64_t coef = 0;
    int64_t center_x = 0;
    int64_t center_y = 0;
    int64_t angle_from = 0;
    int64_t angle_span = 0;
    int64_t angle = 0;
    int64_t ellipse_x = 0;
    int64_t ellipse_y = 0;
    int64_t seg_cunt = 0;
    int64_t step = 0;
    uint32_t ret = 0;

    if (start_x == end_x && start_y == end_y) { return LDG_ERR_AOK; }

    if (radius_x == 0 || radius_y == 0) { return svg_line_to(sink, pen, end_x, end_y); }

    phi = (arg[LDG_FMT_SVG_ARC_PHI_IDX] * LDG_FMT_SVG_PI) / (LDG_FMT_SVG_DEG_HALF_TURN * LDG_FMT_SVG_FIXED_ONE);
    cos_phi = svg_sin_get(phi + LDG_FMT_SVG_HALF_PI);
    sin_phi = svg_sin_get(phi);
    half_x = (start_x - end_x) / LDG_FMT_SVG_PAIR;
    half_y = (start_y - end_y) / LDG_FMT_SVG_PAIR;
    unit_x = svg_fixed_div(svg_fixed_mul(cos_phi, half_x) + svg_fixed_mul(sin_phi, half_y), radius_x);
    unit_y = svg_fixed_div(svg_fixed_mul(cos_phi, half_y) - svg_fixed_mul(sin_phi, half_x), radius_y);
    scale = svg_hypot_get(unit_x, unit_y);

    if (scale <= LDG_FMT_SVG_FIXED_ONE) { lam = svg_fixed_mul(unit_x, unit_x) + svg_fixed_mul(unit_y, unit_y); }
    else
    {
        if (LDG_UNLIKELY(radius_x > (LDG_FMT_SVG_COORD_MAX * LDG_FMT_SVG_FIXED_ONE) / scale || radius_y > (LDG_FMT_SVG_COORD_MAX * LDG_FMT_SVG_FIXED_ONE) / scale)) { return LDG_ERR_RANGE; }

        radius_x = svg_fixed_mul(radius_x, scale);
        radius_y = svg_fixed_mul(radius_y, scale);
        norm = scale;

        while (svg_abs_get(unit_x) > LDG_FMT_SVG_UNIT_MAX || svg_abs_get(unit_y) > LDG_FMT_SVG_UNIT_MAX)
        {
            unit_x /= LDG_FMT_SVG_PAIR;
            unit_y /= LDG_FMT_SVG_PAIR;
            norm /= LDG_FMT_SVG_PAIR;
        }

        unit_x = svg_fixed_div(unit_x, norm);
        unit_y = svg_fixed_div(unit_y, norm);
        lam = svg_fixed_mul(unit_x, unit_x) + svg_fixed_mul(unit_y, unit_y);
    }

    if (lam <= 0) { return svg_line_to(sink, pen, end_x, end_y); }

    coef = (lam >= LDG_FMT_SVG_FIXED_ONE) ? LDG_FMT_SVG_FIXED_NONE : svg_fixed_sqrt_get(svg_fixed_div(LDG_FMT_SVG_FIXED_ONE - lam, lam));

    if (large == sweep) { coef = -coef; }

    ellipse_x = svg_fixed_mul(svg_fixed_mul(coef, unit_y), radius_x);
    ellipse_y = -svg_fixed_mul(svg_fixed_mul(coef, unit_x), radius_y);
    center_x = svg_fixed_mul(cos_phi, ellipse_x) - svg_fixed_mul(sin_phi, ellipse_y) + ((start_x + end_x) / LDG_FMT_SVG_PAIR);
    center_y = svg_fixed_mul(sin_phi, ellipse_x) + svg_fixed_mul(cos_phi, ellipse_y) + ((start_y + end_y) / LDG_FMT_SVG_PAIR);

    angle_from = svg_atan2_get(unit_y + svg_fixed_mul(coef, unit_x), unit_x - svg_fixed_mul(coef, unit_y));
    angle_span = svg_atan2_get(svg_fixed_mul(coef, unit_x) - unit_y, -unit_x - svg_fixed_mul(coef, unit_y)) - angle_from;

    if (sweep != LDG_TRUTH_TRUE && angle_span > 0) { angle_span -= LDG_FMT_SVG_TAU; }

    if (sweep == LDG_TRUTH_TRUE && angle_span < 0) { angle_span += LDG_FMT_SVG_TAU; }

    seg_cunt = svg_segment_cunt_get(svg_fixed_mul(svg_abs_get(angle_span), (radius_x > radius_y) ? radius_x : radius_y));

    for (step = LDG_FMT_SVG_STEP; step < seg_cunt; step++)
    {
        angle = angle_from + ((angle_span * step) / seg_cunt);
        ellipse_x = svg_fixed_mul(svg_sin_get(angle + LDG_FMT_SVG_HALF_PI), radius_x);
        ellipse_y = svg_fixed_mul(svg_sin_get(angle), radius_y);

        ret = svg_line_to(sink, pen, svg_fixed_mul(cos_phi, ellipse_x) - svg_fixed_mul(sin_phi, ellipse_y) + center_x, svg_fixed_mul(sin_phi, ellipse_x) + svg_fixed_mul(cos_phi, ellipse_y) + center_y);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return svg_line_to(sink, pen, end_x, end_y);
}

static uint32_t svg_args_get(const uint8_t *src, uint64_t end, uint64_t *pos, int64_t *arg, uint32_t cunt)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    for (idx = 0; idx < cunt; idx++)
    {
        ret = svg_arg_next(src, end, pos, &arg[idx]);
        if (LDG_UNLIKELY(ret == LDG_ERR_NOT_FOUND)) { return LDG_ERR_INVALID; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t svg_args_shift(int64_t *arg, uint32_t cunt, const svg_pen_t *pen)
{
    uint32_t idx = 0;

    for (idx = 0; idx < cunt; idx++)
    {
        arg[idx] += pen->curr[idx % LDG_FMT_SVG_PAIR];

        if (LDG_UNLIKELY(svg_coord_valid_is(arg[idx]) != LDG_TRUTH_TRUE)) { return LDG_ERR_RANGE; }
    }

    return LDG_ERR_AOK;
}

static uint32_t svg_arc_cmd_exec(svg_sink_t *sink, svg_pen_t *pen, uint8_t rel, const uint8_t *src, uint64_t end, uint64_t *pos)
{
    int64_t arg[LDG_FMT_SVG_ARG_MAX] = LDG_ARR_ZERO_INIT;
    uint8_t large = LDG_TRUTH_FALSE;
    uint8_t sweep = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    ret = svg_args_get(src, end, pos, arg, LDG_FMT_SVG_ARC_LEAD_CUNT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = svg_flag_next(src, end, pos, &large);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = svg_flag_next(src, end, pos, &sweep);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = svg_args_get(src, end, pos, &arg[LDG_FMT_SVG_ARC_END_X_IDX], LDG_FMT_SVG_ARG_PT_CUNT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (rel == LDG_TRUTH_TRUE)
    {
        ret = svg_args_shift(&arg[LDG_FMT_SVG_ARC_END_X_IDX], LDG_FMT_SVG_ARG_PT_CUNT, pen);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return svg_arc_to(sink, pen, arg, large, sweep);
}

static uint32_t svg_curve_cmd_exec(svg_sink_t *sink, svg_pen_t *pen, uint8_t kind, uint8_t rel, const uint8_t *src, uint64_t end, uint64_t *pos)
{
    int64_t arg[LDG_FMT_SVG_ARG_MAX] = LDG_ARR_ZERO_INIT;
    int64_t reflect[LDG_FMT_SVG_PAIR] = LDG_ARR_ZERO_INIT;
    uint32_t cunt = LDG_FMT_SVG_ARG_QUAD_CUNT;
    uint32_t ret = 0;

    if (kind == LDG_FMT_SVG_CMD_CUBIC) { cunt = LDG_FMT_SVG_ARG_CUBIC_CUNT; }

    ret = svg_args_get(src, end, pos, arg, cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (rel == LDG_TRUTH_TRUE)
    {
        ret = svg_args_shift(arg, cunt, pen);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (kind == LDG_FMT_SVG_CMD_CUBIC) { return svg_cubic_to(sink, pen, &arg[LDG_FMT_SVG_ARG_A_IDX], &arg[LDG_FMT_SVG_ARG_B_IDX], &arg[LDG_FMT_SVG_ARG_C_IDX]); }

    if (kind == LDG_FMT_SVG_CMD_QUAD) { return svg_quad_to(sink, pen, &arg[LDG_FMT_SVG_ARG_A_IDX], &arg[LDG_FMT_SVG_ARG_B_IDX]); }

    reflect[LDG_FMT_SVG_X_IDX] = pen->curr[LDG_FMT_SVG_X_IDX];
    reflect[LDG_FMT_SVG_Y_IDX] = pen->curr[LDG_FMT_SVG_Y_IDX];

    if (pen->control_cubic == LDG_TRUTH_TRUE)
    {
        reflect[LDG_FMT_SVG_X_IDX] = (pen->curr[LDG_FMT_SVG_X_IDX] * LDG_FMT_SVG_PAIR) - pen->control[LDG_FMT_SVG_X_IDX];
        reflect[LDG_FMT_SVG_Y_IDX] = (pen->curr[LDG_FMT_SVG_Y_IDX] * LDG_FMT_SVG_PAIR) - pen->control[LDG_FMT_SVG_Y_IDX];
    }

    return svg_cubic_to(sink, pen, reflect, &arg[LDG_FMT_SVG_ARG_A_IDX], &arg[LDG_FMT_SVG_ARG_B_IDX]);
}

static uint32_t svg_cmd_exec(svg_sink_t *sink, svg_pen_t *pen, uint8_t cmd, const uint8_t *src, uint64_t end, uint64_t *pos)
{
    int64_t arg[LDG_FMT_SVG_ARG_MAX] = LDG_ARR_ZERO_INIT;
    uint8_t rel = ldg_char_lower_is(cmd);
    uint8_t kind = (uint8_t)ldg_char_upper_get((char)cmd);
    uint8_t cubic = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    if (kind == LDG_FMT_SVG_CMD_MOV || kind == LDG_FMT_SVG_CMD_LINE)
    {
        ret = svg_args_get(src, end, pos, arg, LDG_FMT_SVG_ARG_PT_CUNT);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (rel == LDG_TRUTH_TRUE)
        {
            ret = svg_args_shift(arg, LDG_FMT_SVG_ARG_PT_CUNT, pen);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        if (kind == LDG_FMT_SVG_CMD_LINE) { ret = svg_line_to(sink, pen, arg[LDG_FMT_SVG_X_IDX], arg[LDG_FMT_SVG_Y_IDX]); }
        else
        {
            pen->curr[LDG_FMT_SVG_X_IDX] = arg[LDG_FMT_SVG_X_IDX];
            pen->curr[LDG_FMT_SVG_Y_IDX] = arg[LDG_FMT_SVG_Y_IDX];
            pen->start[LDG_FMT_SVG_X_IDX] = arg[LDG_FMT_SVG_X_IDX];
            pen->start[LDG_FMT_SVG_Y_IDX] = arg[LDG_FMT_SVG_Y_IDX];
            sink->contour_open = LDG_TRUTH_FALSE;
            ret = svg_contour_begin(sink, pen);
        }
    }
    else if (kind == LDG_FMT_SVG_CMD_HORIZONTAL || kind == LDG_FMT_SVG_CMD_VERTICAL)
    {
        ret = svg_args_get(src, end, pos, arg, LDG_FMT_SVG_ARG_SINGLE_CUNT);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (kind == LDG_FMT_SVG_CMD_HORIZONTAL)
        {
            if (rel == LDG_TRUTH_TRUE) { arg[0] += pen->curr[LDG_FMT_SVG_X_IDX]; }

            ret = svg_line_to(sink, pen, arg[0], pen->curr[LDG_FMT_SVG_Y_IDX]);
        }
        else
        {
            if (rel == LDG_TRUTH_TRUE) { arg[0] += pen->curr[LDG_FMT_SVG_Y_IDX]; }

            ret = svg_line_to(sink, pen, pen->curr[LDG_FMT_SVG_X_IDX], arg[0]);
        }
    }
    else if (kind == LDG_FMT_SVG_CMD_CUBIC || kind == LDG_FMT_SVG_CMD_CUBIC_SMOOTH || kind == LDG_FMT_SVG_CMD_QUAD)
    {
        ret = svg_curve_cmd_exec(sink, pen, kind, rel, src, end, pos);
        cubic = (kind != LDG_FMT_SVG_CMD_QUAD) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    }
    else if (kind == LDG_FMT_SVG_CMD_ARC) { ret = svg_arc_cmd_exec(sink, pen, rel, src, end, pos); }
    else { return LDG_ERR_UNSUPPORTED; }

    pen->control_cubic = cubic;

    return ret;
}

static uint32_t svg_path_parse(svg_sink_t *sink, svg_pen_t *pen, const svg_slice_t *data)
{
    uint64_t pos = data->off;
    uint64_t end = data->off + data->len;
    uint8_t cmd = LDG_FMT_SVG_CMD_NONE;
    uint8_t ch = 0;
    uint32_t ret = 0;

    sink->contour_open = LDG_TRUTH_FALSE;

    while (pos < end)
    {
        while (pos < end && svg_sep_is(sink->src[pos]) == LDG_TRUTH_TRUE) { pos++; }

        if (pos >= end) { break; }

        ch = sink->src[pos];

        if (ldg_char_alpha_is(ch) == LDG_TRUTH_TRUE)
        {
            cmd = ch;
            pos++;

            if ((uint8_t)ldg_char_upper_get((char)cmd) == LDG_FMT_SVG_CMD_CLOSE)
            {
                pen->curr[LDG_FMT_SVG_X_IDX] = pen->start[LDG_FMT_SVG_X_IDX];
                pen->curr[LDG_FMT_SVG_Y_IDX] = pen->start[LDG_FMT_SVG_Y_IDX];
                pen->control_cubic = LDG_TRUTH_FALSE;
                sink->contour_open = LDG_TRUTH_FALSE;
                continue;
            }
        }
        else if (LDG_UNLIKELY(cmd == LDG_FMT_SVG_CMD_NONE || (uint8_t)ldg_char_upper_get((char)cmd) == LDG_FMT_SVG_CMD_CLOSE)) { return LDG_ERR_INVALID; }

        ret = svg_cmd_exec(sink, pen, cmd, sink->src, end, &pos);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if ((uint8_t)ldg_char_upper_get((char)cmd) == LDG_FMT_SVG_CMD_MOV) { cmd = (ldg_char_lower_is(cmd) == LDG_TRUTH_TRUE) ? (uint8_t)ldg_char_lower_get((char)LDG_FMT_SVG_CMD_LINE) : LDG_FMT_SVG_CMD_LINE; }
    }

    sink->contour_open = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint64_t svg_tag_end_get(const svg_sink_t *sink, uint64_t pos)
{
    uint8_t quote = 0;

    while (pos < sink->len)
    {
        if (quote != 0) { if (sink->src[pos] == quote) { quote = 0; } }
        else if (sink->src[pos] == LDG_FMT_SVG_QUOTE_DOUBLE || sink->src[pos] == LDG_FMT_SVG_QUOTE_SINGLE) { quote = sink->src[pos]; }
        else if (sink->src[pos] == LDG_FMT_SVG_TAG_CLOSE) { return pos; }

        pos++;
    }

    return sink->len;
}

static uint8_t svg_name_end_is(uint8_t ch)
{
    if (ldg_char_space_is(ch) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    if (ch == LDG_FMT_SVG_TAG_SLASH || ch == LDG_FMT_SVG_TAG_CLOSE || ch == LDG_FMT_SVG_ATTR_ASSIGN) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static void svg_tag_name_get(const svg_sink_t *sink, uint64_t pos, uint64_t hi, svg_slice_t *out)
{
    out->off = pos;

    while (pos < hi && svg_name_end_is(sink->src[pos]) != LDG_TRUTH_TRUE) { pos++; }

    out->len = pos - out->off;
}

static uint32_t svg_attr_get(const svg_sink_t *sink, uint64_t lo, uint64_t hi, const char *name, svg_slice_t *out)
{
    svg_slice_t key = LDG_STRUCT_ZERO_INIT;
    svg_slice_t val = LDG_STRUCT_ZERO_INIT;
    uint64_t pos = lo + LDG_FMT_SVG_STEP;
    uint8_t quote = 0;

    out->off = 0;
    out->len = 0;

    while (pos < hi && svg_name_end_is(sink->src[pos]) != LDG_TRUTH_TRUE) { pos++; }

    while (pos < hi)
    {
        while (pos < hi && ldg_char_space_is(sink->src[pos]) == LDG_TRUTH_TRUE) { pos++; }

        if (pos >= hi || sink->src[pos] == LDG_FMT_SVG_TAG_SLASH) { break; }

        svg_tag_name_get(sink, pos, hi, &key);
        pos = key.off + key.len;
        val.off = pos;
        val.len = 0;

        while (pos < hi && ldg_char_space_is(sink->src[pos]) == LDG_TRUTH_TRUE) { pos++; }

        if (pos < hi && sink->src[pos] == LDG_FMT_SVG_ATTR_ASSIGN)
        {
            pos++;

            while (pos < hi && ldg_char_space_is(sink->src[pos]) == LDG_TRUTH_TRUE) { pos++; }

            if (LDG_UNLIKELY(pos >= hi || (sink->src[pos] != LDG_FMT_SVG_QUOTE_DOUBLE && sink->src[pos] != LDG_FMT_SVG_QUOTE_SINGLE))) { return LDG_ERR_INVALID; }

            quote = sink->src[pos];
            pos++;
            val.off = pos;

            while (pos < hi && sink->src[pos] != quote) { pos++; }

            val.len = pos - val.off;
            pos++;
        }

        if (key.len == 0) { pos++; }

        if (svg_slice_eq_is(sink, &key, name) == LDG_TRUTH_TRUE)
        {
            *out = val;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

static uint64_t svg_byte_find(const svg_sink_t *sink, uint64_t pos, uint64_t end, uint8_t ch)
{
    while (pos < end && sink->src[pos] != ch) { pos++; }

    return pos;
}

static uint32_t svg_decl_get(const svg_sink_t *sink, const svg_slice_t *decl, const char *name, svg_slice_t *out)
{
    svg_slice_t key = LDG_STRUCT_ZERO_INIT;
    svg_slice_t val = LDG_STRUCT_ZERO_INIT;
    uint64_t pos = decl->off;
    uint64_t end = decl->off + decl->len;
    uint64_t item_hi = 0;
    uint64_t colon = 0;
    uint32_t ret = LDG_ERR_NOT_FOUND;

    while (pos < end)
    {
        item_hi = svg_byte_find(sink, pos, end, LDG_FMT_SVG_DECL_SEP);
        colon = svg_byte_find(sink, pos, item_hi, LDG_FMT_SVG_DECL_ASSIGN);

        if (colon < item_hi)
        {
            key.off = pos;
            key.len = colon - pos;
            val.off = colon + LDG_FMT_SVG_STEP;
            val.len = item_hi - val.off;
            svg_slice_trim(sink, &key);
            svg_slice_trim(sink, &val);

            if (svg_slice_eq_is(sink, &key, name) == LDG_TRUTH_TRUE)
            {
                *out = val;
                ret = LDG_ERR_AOK;
            }
        }

        pos = item_hi + LDG_FMT_SVG_STEP;
    }

    return ret;
}

static uint32_t svg_fill_val_apply(const svg_sink_t *sink, const svg_slice_t *raw, svg_fill_t *fill)
{
    svg_slice_t val = *raw;
    svg_slice_t ref = LDG_STRUCT_ZERO_INIT;
    uint64_t lead = 0;
    uint64_t ref_hi = 0;

    svg_slice_trim(sink, &val);

    if (svg_slice_eq_is(sink, &val, "none") == LDG_TRUTH_TRUE)
    {
        fill->kind = SVG_FILL_KIND_NONE;
        return LDG_ERR_AOK;
    }

    if (svg_prefix_is(sink, val.off, val.off + val.len, LDG_FMT_SVG_URL_OPEN) == LDG_TRUTH_TRUE)
    {
        lead = svg_byte_find(sink, val.off, val.off + val.len, LDG_FMT_SVG_ID_LEAD);
        ref_hi = svg_byte_find(sink, lead, val.off + val.len, LDG_FMT_SVG_URL_CLOSE);

        if (LDG_UNLIKELY(lead >= ref_hi)) { return LDG_ERR_INVALID; }

        ref.off = lead + LDG_FMT_SVG_STEP;
        ref.len = ref_hi - ref.off;
        fill->kind = SVG_FILL_KIND_URL;

        return ldg_hash_fnv1a_64_compute(sink->src + ref.off, ref.len, &fill->url_hash);
    }

    return svg_color_parse(sink, &val, fill);
}

static uint32_t svg_rule_val_apply(const svg_sink_t *sink, const svg_slice_t *raw, svg_fill_t *fill)
{
    svg_slice_t val = *raw;

    svg_slice_trim(sink, &val);

    if (svg_slice_eq_is(sink, &val, "evenodd") == LDG_TRUTH_TRUE)
    {
        fill->rule = LDG_FMT_SVG_FILL_RULE_EVEN_ODD;
        return LDG_ERR_AOK;
    }

    if (svg_slice_eq_is(sink, &val, "nonzero") == LDG_TRUTH_TRUE)
    {
        fill->rule = LDG_FMT_SVG_FILL_RULE_NONZERO;
        return LDG_ERR_AOK;
    }

    return LDG_ERR_INVALID;
}

static uint32_t svg_decl_apply(const svg_sink_t *sink, const svg_slice_t *decl, svg_fill_t *fill)
{
    svg_slice_t val = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (svg_decl_get(sink, decl, "fill", &val) == LDG_ERR_AOK)
    {
        ret = svg_fill_val_apply(sink, &val, fill);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (svg_decl_get(sink, decl, "fill-rule", &val) == LDG_ERR_AOK) { return svg_rule_val_apply(sink, &val, fill); }

    return LDG_ERR_AOK;
}

static uint32_t svg_view_parse(svg_sink_t *sink, uint64_t lo, uint64_t hi)
{
    svg_slice_t val = LDG_STRUCT_ZERO_INIT;
    int64_t view[LDG_FMT_SVG_VIEW_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t pos = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (svg_attr_get(sink, lo, hi, "viewBox", &val) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    pos = val.off;

    ret = svg_args_get(sink->src, val.off + val.len, &pos, view, LDG_FMT_SVG_VIEW_CUNT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < LDG_FMT_SVG_VIEW_CUNT; idx++) { if (LDG_UNLIKELY(view[idx] > INT32_MAX || view[idx] < INT32_MIN)) { return LDG_ERR_RANGE; } }

    if (LDG_UNLIKELY(view[LDG_FMT_SVG_VIEW_EXTENT_X_IDX] <= 0 || view[LDG_FMT_SVG_VIEW_EXTENT_Y_IDX] <= 0)) { return LDG_ERR_INVALID; }

    if (sink->svg)
    {
        sink->svg->view_pos[LDG_FMT_SVG_X_IDX] = (int32_t)view[LDG_FMT_SVG_X_IDX];
        sink->svg->view_pos[LDG_FMT_SVG_Y_IDX] = (int32_t)view[LDG_FMT_SVG_Y_IDX];
        sink->svg->view_extent[LDG_FMT_SVG_X_IDX] = (int32_t)view[LDG_FMT_SVG_VIEW_EXTENT_X_IDX];
        sink->svg->view_extent[LDG_FMT_SVG_Y_IDX] = (int32_t)view[LDG_FMT_SVG_VIEW_EXTENT_Y_IDX];
    }

    sink->view_set = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t svg_translate_parse(const svg_sink_t *sink, const svg_slice_t *raw, svg_pen_t *pen)
{
    svg_slice_t val = *raw;
    int64_t arg[LDG_FMT_SVG_TRANSLATE_ARG_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t pos = 0;
    uint64_t paren_hi = 0;
    uint32_t ret = 0;

    svg_slice_trim(sink, &val);

    if (val.len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(svg_prefix_is(sink, val.off, val.off + val.len, "translate") != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    pos = svg_byte_find(sink, val.off, val.off + val.len, LDG_FMT_SVG_PAREN_OPEN) + LDG_FMT_SVG_STEP;
    paren_hi = svg_byte_find(sink, pos, val.off + val.len, LDG_FMT_SVG_URL_CLOSE);

    if (LDG_UNLIKELY(paren_hi >= val.off + val.len)) { return LDG_ERR_INVALID; }

    ret = svg_arg_next(sink->src, paren_hi, &pos, &arg[LDG_FMT_SVG_X_IDX]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    ret = svg_arg_next(sink->src, paren_hi, &pos, &arg[LDG_FMT_SVG_Y_IDX]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_NOT_FOUND)) { return ret; }

    pen->shift[LDG_FMT_SVG_X_IDX] = arg[LDG_FMT_SVG_X_IDX];
    pen->shift[LDG_FMT_SVG_Y_IDX] = arg[LDG_FMT_SVG_Y_IDX];

    return LDG_ERR_AOK;
}

static uint32_t svg_shape_parse(svg_sink_t *sink, uint64_t lo, uint64_t hi)
{
    svg_pen_t pen = LDG_STRUCT_ZERO_INIT;
    svg_slice_t data = LDG_STRUCT_ZERO_INIT;
    svg_slice_t transform = LDG_STRUCT_ZERO_INIT;
    svg_shape_src_t *shape_src = 0x0;
    ldg_fmt_svg_shape_t *shape = 0x0;
    uint64_t contour_first = 0;
    uint32_t ret = 0;

    if (svg_attr_get(sink, lo, hi, "d", &data) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (svg_attr_get(sink, lo, hi, "transform", &transform) == LDG_ERR_AOK)
    {
        ret = svg_translate_parse(sink, &transform, &pen);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (sink->svg)
    {
        if (LDG_UNLIKELY(sink->shape_cunt >= sink->shape_cap)) { return LDG_ERR_MEM_CORRUPTION; }

        shape = &sink->svg->shape[sink->shape_cunt];
        shape->contour_first = (uint32_t)sink->contour_cunt;
        shape->pt_first = (uint32_t)sink->pt_cunt;
    }

    contour_first = sink->contour_cunt;

    ret = svg_path_parse(sink, &pen, &data);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (sink->svg)
    {
        shape->contour_cunt = (uint32_t)(sink->contour_cunt - contour_first);
        shape_src = &sink->shape_src[sink->shape_cunt];
        shape_src->shift_x = pen.shift[LDG_FMT_SVG_X_IDX];
        shape_src->shift_y = pen.shift[LDG_FMT_SVG_Y_IDX];

        if (svg_attr_get(sink, lo, hi, "class", &shape_src->sel) != LDG_ERR_AOK) { shape_src->sel.len = 0; }

        if (svg_attr_get(sink, lo, hi, "style", &shape_src->style) != LDG_ERR_AOK) { shape_src->style.len = 0; }

        if (svg_attr_get(sink, lo, hi, "fill", &shape_src->fill) != LDG_ERR_AOK) { shape_src->fill.len = 0; }

        if (svg_attr_get(sink, lo, hi, "fill-rule", &shape_src->rule) != LDG_ERR_AOK) { shape_src->rule.len = 0; }
    }

    sink->shape_cunt++;

    if (LDG_UNLIKELY(sink->shape_cunt > UINT32_MAX)) { return LDG_ERR_RANGE; }

    return LDG_ERR_AOK;
}

static uint32_t svg_length_get(const svg_sink_t *sink, const svg_slice_t *raw, int64_t extent, int64_t *out)
{
    svg_slice_t val = *raw;
    uint64_t pos = 0;
    uint64_t end = 0;
    int64_t num = 0;
    uint32_t ret = 0;

    *out = 0;

    svg_slice_trim(sink, &val);
    pos = val.off;
    end = val.off + val.len;

    ret = svg_number_parse(sink->src, end, &pos, &num);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    if (pos < end && sink->src[pos] == LDG_FMT_SVG_PERCENT_SIGN)
    {
        *out = svg_fixed_mul(num, extent) / LDG_FMT_SVG_PERCENT;
        return LDG_ERR_AOK;
    }

    *out = num;

    return LDG_ERR_AOK;
}

static int64_t svg_view_diag_get(const ldg_fmt_svg_t *svg)
{
    int64_t extent_x = svg->view_extent[LDG_FMT_SVG_X_IDX];
    int64_t extent_y = svg->view_extent[LDG_FMT_SVG_Y_IDX];

    return svg_fixed_sqrt_get((svg_fixed_mul(extent_x, extent_x) + svg_fixed_mul(extent_y, extent_y)) / LDG_FMT_SVG_PAIR);
}

static uint32_t svg_href_parse(const svg_sink_t *sink, uint64_t lo, uint64_t hi, svg_grad_t *grad)
{
    svg_slice_t val = LDG_STRUCT_ZERO_INIT;

    if (svg_attr_get(sink, lo, hi, "xlink:href", &val) != LDG_ERR_AOK && svg_attr_get(sink, lo, hi, "href", &val) != LDG_ERR_AOK) { return LDG_ERR_AOK; }

    svg_slice_trim(sink, &val);

    if (LDG_UNLIKELY(val.len < LDG_FMT_SVG_PAIR || sink->src[val.off] != LDG_FMT_SVG_ID_LEAD)) { return LDG_ERR_UNSUPPORTED; }

    grad->href_set = LDG_TRUTH_TRUE;

    return ldg_hash_fnv1a_64_compute(sink->src + val.off + LDG_FMT_SVG_STEP, val.len - LDG_FMT_SVG_STEP, &grad->href_hash);
}

static uint32_t svg_grad_parse(svg_sink_t *sink, uint64_t lo, uint64_t hi, uint8_t self_close)
{
    svg_state_t *state = sink->state;
    svg_grad_t *grad = 0x0;
    svg_slice_t val = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (!state) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(svg_attr_get(sink, lo, hi, "gradientTransform", &val) == LDG_ERR_AOK)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(state->grad_cunt >= LDG_FMT_SVG_GRAD_MAX)) { return LDG_ERR_FULL; }

    grad = &state->grad[state->grad_cunt];

    ret = ldg_mem_zero(grad, (uint64_t)sizeof(svg_grad_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (svg_attr_get(sink, lo, hi, "id", &val) == LDG_ERR_AOK)
    {
        ret = ldg_hash_fnv1a_64_compute(sink->src + val.off, val.len, &grad->id_hash);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (svg_attr_get(sink, lo, hi, "cx", &val) == LDG_ERR_AOK)
    {
        ret = svg_length_get(sink, &val, sink->svg->view_extent[LDG_FMT_SVG_X_IDX], &grad->center_x);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        grad->center_x_set = LDG_TRUTH_TRUE;
    }

    if (svg_attr_get(sink, lo, hi, "cy", &val) == LDG_ERR_AOK)
    {
        ret = svg_length_get(sink, &val, sink->svg->view_extent[LDG_FMT_SVG_Y_IDX], &grad->center_y);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        grad->center_y_set = LDG_TRUTH_TRUE;
    }

    if (svg_attr_get(sink, lo, hi, "r", &val) == LDG_ERR_AOK)
    {
        ret = svg_length_get(sink, &val, svg_view_diag_get(sink->svg), &grad->radius);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        grad->radius_set = LDG_TRUTH_TRUE;
    }

    if (svg_attr_get(sink, lo, hi, "gradientUnits", &val) == LDG_ERR_AOK)
    {
        grad->space_set = LDG_TRUTH_TRUE;
        grad->user_space = svg_slice_eq_is(sink, &val, "userSpaceOnUse");
    }

    ret = svg_href_parse(sink, lo, hi, grad);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->grad_open = (self_close == LDG_TRUTH_TRUE) ? LDG_FMT_SVG_NONE : state->grad_cunt;
    state->grad_cunt++;

    return LDG_ERR_AOK;
}

static uint32_t svg_stop_color_get(const svg_sink_t *sink, uint64_t lo, uint64_t hi, svg_fill_t *out)
{
    svg_slice_t val = LDG_STRUCT_ZERO_INIT;
    svg_slice_t style = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (svg_attr_get(sink, lo, hi, "stop-color", &val) == LDG_ERR_AOK)
    {
        svg_slice_trim(sink, &val);

        ret = svg_color_parse(sink, &val, out);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (svg_attr_get(sink, lo, hi, "style", &style) == LDG_ERR_AOK && svg_decl_get(sink, &style, "stop-color", &val) == LDG_ERR_AOK) { return svg_color_parse(sink, &val, out); }

    return LDG_ERR_AOK;
}

static uint32_t svg_stop_parse(svg_sink_t *sink, uint64_t lo, uint64_t hi)
{
    svg_state_t *state = sink->state;
    svg_grad_t *grad = 0x0;
    ldg_fmt_svg_stop_t *stop = 0x0;
    svg_fill_t color = LDG_STRUCT_ZERO_INIT;
    svg_slice_t val = LDG_STRUCT_ZERO_INIT;
    int64_t off = 0;
    uint32_t ret = 0;

    if (!state || state->grad_open == LDG_FMT_SVG_NONE) { return LDG_ERR_AOK; }

    grad = &state->grad[state->grad_open];

    if (LDG_UNLIKELY(grad->stop_cunt >= LDG_FMT_SVG_STOP_MAX)) { return LDG_ERR_FULL; }

    if (svg_attr_get(sink, lo, hi, "offset", &val) == LDG_ERR_AOK)
    {
        ret = svg_length_get(sink, &val, LDG_FMT_SVG_FIXED_ONE, &off);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (off < 0) { off = 0; }

    if (off > LDG_FMT_SVG_FIXED_ONE) { off = LDG_FMT_SVG_FIXED_ONE; }

    if (grad->stop_cunt > 0 && off < grad->stop[grad->stop_cunt - LDG_FMT_SVG_STEP].off) { off = grad->stop[grad->stop_cunt - LDG_FMT_SVG_STEP].off; }

    ret = svg_stop_color_get(sink, lo, hi, &color);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    stop = &grad->stop[grad->stop_cunt];
    stop->off = (int32_t)off;
    stop->red = color.red;
    stop->green = color.green;
    stop->blue = color.blue;
    stop->pudding[0] = 0;
    grad->stop_cunt++;

    return LDG_ERR_AOK;
}

static uint32_t svg_css_rule_add(svg_sink_t *sink, const svg_slice_t *sel, const svg_slice_t *decl)
{
    svg_state_t *state = sink->state;
    svg_css_rule_t *rule = 0x0;

    if (sel->len < LDG_FMT_SVG_PAIR || sink->src[sel->off] != LDG_FMT_SVG_CLASS_LEAD) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(state->css_rule_cunt >= LDG_FMT_SVG_CSS_RULE_MAX)) { return LDG_ERR_FULL; }

    rule = &state->css_rule[state->css_rule_cunt];
    rule->decl = *decl;
    state->css_rule_cunt++;

    return ldg_hash_fnv1a_64_compute(sink->src + sel->off + LDG_FMT_SVG_STEP, sel->len - LDG_FMT_SVG_STEP, &rule->name_hash);
}

static uint32_t svg_css_parse(svg_sink_t *sink, uint64_t pos, uint64_t end)
{
    svg_slice_t sel = LDG_STRUCT_ZERO_INIT;
    svg_slice_t decl = LDG_STRUCT_ZERO_INIT;
    uint64_t blk_lo = 0;
    uint64_t blk_hi = 0;
    uint64_t sel_pos = 0;
    uint64_t sel_hi = 0;
    uint32_t ret = 0;

    if (!sink->state) { return LDG_ERR_AOK; }

    while (pos < end)
    {
        while (pos < end && ldg_char_space_is(sink->src[pos]) == LDG_TRUTH_TRUE) { pos++; }

        if (pos >= end) { break; }

        if (svg_prefix_is(sink, pos, end, LDG_FMT_SVG_CSS_COMMENT_OPEN) == LDG_TRUTH_TRUE)
        {
            pos = svg_find(sink, pos, end, LDG_FMT_SVG_CSS_COMMENT_CLOSE) + LDG_FMT_SVG_CSS_COMMENT_LEN;
            continue;
        }

        blk_lo = svg_byte_find(sink, pos, end, LDG_FMT_SVG_BLK_OPEN);
        blk_hi = svg_byte_find(sink, blk_lo, end, LDG_FMT_SVG_BLK_CLOSE);

        if (LDG_UNLIKELY(blk_lo >= end || blk_hi >= end)) { return LDG_ERR_INVALID; }

        decl.off = blk_lo + LDG_FMT_SVG_STEP;
        decl.len = blk_hi - decl.off;
        sel_pos = pos;

        while (sel_pos < blk_lo)
        {
            sel_hi = svg_byte_find(sink, sel_pos, blk_lo, LDG_FMT_SVG_SEL_SEP);
            sel.off = sel_pos;
            sel.len = sel_hi - sel_pos;
            svg_slice_trim(sink, &sel);

            ret = svg_css_rule_add(sink, &sel, &decl);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            sel_pos = sel_hi + LDG_FMT_SVG_STEP;
        }

        pos = blk_hi + LDG_FMT_SVG_STEP;
    }

    return LDG_ERR_AOK;
}

static uint32_t svg_elem_handle(svg_sink_t *sink, uint64_t lo, uint64_t hi, uint8_t self_close)
{
    svg_slice_t name = LDG_STRUCT_ZERO_INIT;
    svg_slice_t val = LDG_STRUCT_ZERO_INIT;

    svg_tag_name_get(sink, lo + LDG_FMT_SVG_STEP, hi, &name);

    if (svg_slice_eq_is(sink, &name, "svg") == LDG_TRUTH_TRUE) { return svg_view_parse(sink, lo, hi); }

    if (svg_slice_eq_is(sink, &name, "style") == LDG_TRUTH_TRUE)
    {
        if (self_close != LDG_TRUTH_TRUE) { sink->style_open = LDG_TRUTH_TRUE; }

        return LDG_ERR_AOK;
    }

    if (svg_slice_eq_is(sink, &name, "radialGradient") == LDG_TRUTH_TRUE) { return svg_grad_parse(sink, lo, hi, self_close); }

    if (svg_slice_eq_is(sink, &name, "stop") == LDG_TRUTH_TRUE) { return svg_stop_parse(sink, lo, hi); }

    if (svg_slice_eq_is(sink, &name, "path") == LDG_TRUTH_TRUE) { return svg_shape_parse(sink, lo, hi); }

    if (svg_slice_eq_is(sink, &name, "g") == LDG_TRUTH_TRUE && LDG_UNLIKELY(svg_attr_get(sink, lo, hi, "transform", &val) == LDG_ERR_AOK)) { return LDG_ERR_UNSUPPORTED; }

    return LDG_ERR_AOK;
}

static void svg_elem_close(svg_sink_t *sink, uint64_t lo, uint64_t hi)
{
    svg_slice_t name = LDG_STRUCT_ZERO_INIT;

    svg_tag_name_get(sink, lo + LDG_FMT_SVG_PAIR, hi, &name);

    if (svg_slice_eq_is(sink, &name, "style") == LDG_TRUTH_TRUE) { sink->style_open = LDG_TRUTH_FALSE; }
    else if (svg_slice_eq_is(sink, &name, "radialGradient") == LDG_TRUTH_TRUE && sink->state) { sink->state->grad_open = LDG_FMT_SVG_NONE; }
}

static uint32_t svg_doc_walk(svg_sink_t *sink)
{
    uint64_t pos = 0;
    uint64_t hi = 0;
    uint8_t mark = 0;
    uint8_t self_close = LDG_TRUTH_FALSE;
    uint32_t ret = 0;

    while (pos < sink->len)
    {
        if (sink->src[pos] != LDG_FMT_SVG_TAG_OPEN)
        {
            hi = svg_byte_find(sink, pos, sink->len, LDG_FMT_SVG_TAG_OPEN);

            if (sink->style_open == LDG_TRUTH_TRUE)
            {
                ret = svg_css_parse(sink, pos, hi);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            pos = hi;
            continue;
        }

        if (svg_prefix_is(sink, pos, sink->len, LDG_FMT_SVG_COMMENT_OPEN) == LDG_TRUTH_TRUE)
        {
            pos = svg_find(sink, pos + LDG_FMT_SVG_COMMENT_OPEN_LEN, sink->len, LDG_FMT_SVG_COMMENT_CLOSE) + LDG_FMT_SVG_COMMENT_CLOSE_LEN;
            continue;
        }

        if (svg_prefix_is(sink, pos, sink->len, LDG_FMT_SVG_CDATA_OPEN) == LDG_TRUTH_TRUE)
        {
            hi = svg_find(sink, pos + LDG_FMT_SVG_CDATA_OPEN_LEN, sink->len, LDG_FMT_SVG_CDATA_CLOSE);

            if (sink->style_open == LDG_TRUTH_TRUE)
            {
                ret = svg_css_parse(sink, pos + LDG_FMT_SVG_CDATA_OPEN_LEN, hi);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }

            pos = hi + LDG_FMT_SVG_CDATA_CLOSE_LEN;
            continue;
        }

        hi = svg_tag_end_get(sink, pos);
        if (LDG_UNLIKELY(hi >= sink->len || pos + LDG_FMT_SVG_STEP >= hi)) { return LDG_ERR_INVALID; }

        mark = sink->src[pos + LDG_FMT_SVG_STEP];

        if (mark == LDG_FMT_SVG_TAG_SLASH) { svg_elem_close(sink, pos, hi); }
        else if (mark != LDG_FMT_SVG_PROLOG_MARK && mark != LDG_FMT_SVG_DECL_MARK)
        {
            self_close = (sink->src[hi - LDG_FMT_SVG_STEP] == LDG_FMT_SVG_TAG_SLASH) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

            ret = svg_elem_handle(sink, pos, hi, self_close);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        pos = hi + LDG_FMT_SVG_STEP;
    }

    return LDG_ERR_AOK;
}

static uint8_t svg_sel_has(const svg_sink_t *sink, const svg_slice_t *sel, uint64_t name_hash)
{
    uint64_t pos = sel->off;
    uint64_t end = sel->off + sel->len;
    uint64_t tok_off = 0;
    uint64_t hash = 0;
    uint32_t ret = 0;

    while (pos < end)
    {
        while (pos < end && ldg_char_space_is(sink->src[pos]) == LDG_TRUTH_TRUE) { pos++; }

        tok_off = pos;

        while (pos < end && ldg_char_space_is(sink->src[pos]) != LDG_TRUTH_TRUE) { pos++; }

        if (pos == tok_off) { continue; }

        ret = ldg_hash_fnv1a_64_compute(sink->src + tok_off, pos - tok_off, &hash);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

        if (hash == name_hash) { return LDG_TRUTH_TRUE; }
    }

    return LDG_TRUTH_FALSE;
}

static uint32_t svg_grad_find(const svg_state_t *state, uint64_t hash, uint32_t *out)
{
    uint32_t idx = 0;

    *out = LDG_FMT_SVG_NONE;

    for (idx = 0; idx < state->grad_cunt; idx++) { if (state->grad[idx].id_hash == hash)
        {
            *out = idx;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t svg_grad_resolve(const svg_sink_t *sink, uint64_t hash, const svg_shape_src_t *src, ldg_fmt_svg_paint_t *out)
{
    const svg_grad_t *grad = 0x0;
    const svg_grad_t *stop_src = 0x0;
    int64_t center_x = 0;
    int64_t center_y = 0;
    int64_t radius = 0;
    uint32_t idx = 0;
    uint32_t depth = 0;
    uint32_t ret = 0;
    uint8_t center_x_found = LDG_TRUTH_FALSE;
    uint8_t center_y_found = LDG_TRUTH_FALSE;
    uint8_t radius_found = LDG_TRUTH_FALSE;
    uint8_t space_found = LDG_TRUTH_FALSE;
    uint8_t user_space = LDG_TRUTH_FALSE;

    center_x = (int64_t)sink->svg->view_pos[LDG_FMT_SVG_X_IDX] + ((int64_t)sink->svg->view_extent[LDG_FMT_SVG_X_IDX] / LDG_FMT_SVG_PAIR);
    center_y = (int64_t)sink->svg->view_pos[LDG_FMT_SVG_Y_IDX] + ((int64_t)sink->svg->view_extent[LDG_FMT_SVG_Y_IDX] / LDG_FMT_SVG_PAIR);
    radius = svg_view_diag_get(sink->svg) / LDG_FMT_SVG_PAIR;

    ret = svg_grad_find(sink->state, hash, &idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (depth = 0; depth < LDG_FMT_SVG_HREF_DEPTH_MAX; depth++)
    {
        grad = &sink->state->grad[idx];

        if (center_x_found != LDG_TRUTH_TRUE && grad->center_x_set == LDG_TRUTH_TRUE)
        {
            center_x = grad->center_x;
            center_x_found = LDG_TRUTH_TRUE;
        }

        if (center_y_found != LDG_TRUTH_TRUE && grad->center_y_set == LDG_TRUTH_TRUE)
        {
            center_y = grad->center_y;
            center_y_found = LDG_TRUTH_TRUE;
        }

        if (radius_found != LDG_TRUTH_TRUE && grad->radius_set == LDG_TRUTH_TRUE)
        {
            radius = grad->radius;
            radius_found = LDG_TRUTH_TRUE;
        }

        if (space_found != LDG_TRUTH_TRUE && grad->space_set == LDG_TRUTH_TRUE)
        {
            user_space = grad->user_space;
            space_found = LDG_TRUTH_TRUE;
        }

        if (!stop_src && grad->stop_cunt > 0) { stop_src = grad; }

        if (grad->href_set != LDG_TRUTH_TRUE || svg_grad_find(sink->state, grad->href_hash, &idx) != LDG_ERR_AOK) { break; }
    }

    if (LDG_UNLIKELY(space_found != LDG_TRUTH_TRUE || user_space != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (!stop_src) { return LDG_ERR_EMPTY; }

    center_x += src->shift_x;
    center_y += src->shift_y;

    if (LDG_UNLIKELY(center_x > INT32_MAX || center_x < INT32_MIN || center_y > INT32_MAX || center_y < INT32_MIN || radius > INT32_MAX || radius <= 0)) { return LDG_ERR_RANGE; }

    out->kind = LDG_FMT_SVG_PAINT_RADIAL;
    out->center[LDG_FMT_SVG_X_IDX] = (int32_t)center_x;
    out->center[LDG_FMT_SVG_Y_IDX] = (int32_t)center_y;
    out->radius = (int32_t)radius;
    out->stop_cunt = stop_src->stop_cunt;

    for (idx = 0; idx < stop_src->stop_cunt; idx++) { out->stop[idx] = stop_src->stop[idx]; }

    return LDG_ERR_AOK;
}

static uint32_t svg_shape_fill_get(const svg_sink_t *sink, const svg_shape_src_t *src, svg_fill_t *fill)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    fill->kind = SVG_FILL_KIND_COLOR;
    fill->rule = LDG_FMT_SVG_FILL_RULE_NONZERO;

    if (src->fill.len > 0)
    {
        ret = svg_fill_val_apply(sink, &src->fill, fill);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (src->rule.len > 0)
    {
        ret = svg_rule_val_apply(sink, &src->rule, fill);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < sink->state->css_rule_cunt && src->sel.len > 0; idx++)
    {
        if (svg_sel_has(sink, &src->sel, sink->state->css_rule[idx].name_hash) != LDG_TRUTH_TRUE) { continue; }

        ret = svg_decl_apply(sink, &sink->state->css_rule[idx].decl, fill);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (src->style.len > 0) { return svg_decl_apply(sink, &src->style, fill); }

    return LDG_ERR_AOK;
}

static uint32_t svg_shape_resolve(svg_sink_t *sink, uint64_t idx)
{
    svg_fill_t fill = LDG_STRUCT_ZERO_INIT;
    ldg_fmt_svg_shape_t *shape = &sink->svg->shape[idx];
    uint32_t ret = 0;

    ret = svg_shape_fill_get(sink, &sink->shape_src[idx], &fill);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    shape->rule = fill.rule;

    if (fill.kind == SVG_FILL_KIND_NONE)
    {
        shape->contour_cunt = 0;
        return LDG_ERR_AOK;
    }

    if (fill.kind == SVG_FILL_KIND_URL)
    {
        ret = svg_grad_resolve(sink, fill.url_hash, &sink->shape_src[idx], &shape->paint);
        if (ret == LDG_ERR_EMPTY) { shape->contour_cunt = 0; }

        return (ret == LDG_ERR_EMPTY) ? LDG_ERR_AOK : ret;
    }

    shape->paint.kind = LDG_FMT_SVG_PAINT_SOLID;
    shape->paint.stop_cunt = LDG_FMT_SVG_STEP;
    shape->paint.stop[0].off = LDG_FMT_SVG_FIXED_NONE;
    shape->paint.stop[0].red = fill.red;
    shape->paint.stop[0].green = fill.green;
    shape->paint.stop[0].blue = fill.blue;

    return LDG_ERR_AOK;
}

static uint64_t svg_alloc_cunt_get(uint64_t cunt)
{
    if (cunt == 0) { return LDG_FMT_SVG_ALLOC_MIN; }

    return cunt;
}

static uint32_t svg_tally(const uint8_t *src, uint64_t len, svg_sink_t *out)
{
    uint32_t ret = 0;

    ret = ldg_mem_zero(out, (uint64_t)sizeof(svg_sink_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->src = src;
    out->len = len;

    return svg_doc_walk(out);
}

static uint64_t svg_pool_need_get(const svg_sink_t *tally)
{
    uint64_t total = 0;

    total = (uint64_t)sizeof(ldg_fmt_svg_t);
    total += svg_alloc_cunt_get(tally->shape_cunt) * (uint64_t)sizeof(ldg_fmt_svg_shape_t);
    total += svg_alloc_cunt_get(tally->contour_cunt) * (uint64_t)sizeof(uint32_t);
    total += svg_alloc_cunt_get(tally->pt_cunt) * LDG_FMT_SVG_PAIR * (uint64_t)sizeof(int32_t);
    total += (uint64_t)sizeof(svg_state_t);
    total += svg_alloc_cunt_get(tally->shape_cunt) * (uint64_t)sizeof(svg_shape_src_t);
    total += (uint64_t)LDG_FMT_SVG_POOL_ALLOC_CUNT * (uint64_t)LDG_MEM_POOL_VAR_ALIGN;

    return total;
}

static uint32_t svg_pool_take(ldg_mem_pool_t *pool, uint64_t size, void **out)
{
    uint32_t ret = 0;

    ret = ldg_mem_pool_alloc(pool, size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

uint32_t ldg_fmt_svg_pool_size_get(const uint8_t *src, uint64_t len, uint64_t *out)
{
    svg_sink_t tally = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!src || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = svg_tally(src, len, &tally);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = svg_pool_need_get(&tally);

    return LDG_ERR_AOK;
}

static uint32_t svg_storage_take(ldg_mem_pool_t *pool, const svg_sink_t *tally, svg_sink_t *sink)
{
    void *mem = 0x0;
    uint32_t ret = 0;

    ret = svg_pool_take(pool, (uint64_t)sizeof(ldg_fmt_svg_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sink->svg = (ldg_fmt_svg_t *)mem;

    ret = svg_pool_take(pool, svg_alloc_cunt_get(tally->shape_cunt) * (uint64_t)sizeof(ldg_fmt_svg_shape_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sink->svg->shape = (ldg_fmt_svg_shape_t *)mem;

    ret = svg_pool_take(pool, svg_alloc_cunt_get(tally->contour_cunt) * (uint64_t)sizeof(uint32_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sink->svg->contour_len = (uint32_t *)mem;

    ret = svg_pool_take(pool, svg_alloc_cunt_get(tally->pt_cunt) * LDG_FMT_SVG_PAIR * (uint64_t)sizeof(int32_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sink->svg->pt = (int32_t *)mem;

    ret = svg_pool_take(pool, (uint64_t)sizeof(svg_state_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sink->state = (svg_state_t *)mem;
    sink->state->grad_open = LDG_FMT_SVG_NONE;

    ret = svg_pool_take(pool, svg_alloc_cunt_get(tally->shape_cunt) * (uint64_t)sizeof(svg_shape_src_t), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sink->shape_src = (svg_shape_src_t *)mem;
    sink->shape_cap = tally->shape_cunt;
    sink->contour_cap = tally->contour_cunt;
    sink->pt_cap = tally->pt_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_fmt_svg_parse(ldg_mem_pool_t *pool, const uint8_t *src, uint64_t len, ldg_fmt_svg_t **out)
{
    svg_sink_t tally = LDG_STRUCT_ZERO_INIT;
    svg_sink_t sink = LDG_STRUCT_ZERO_INIT;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !src || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    ret = svg_tally(src, len, &tally);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(tally.view_set != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    sink.src = src;
    sink.len = len;

    ret = svg_storage_take(pool, &tally, &sink);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = svg_doc_walk(&sink);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(sink.shape_cunt != tally.shape_cunt || sink.contour_cunt != tally.contour_cunt || sink.pt_cunt != tally.pt_cunt)) { return LDG_ERR_MEM_CORRUPTION; }

    sink.svg->shape_cunt = (uint32_t)sink.shape_cunt;
    sink.svg->contour_cunt = (uint32_t)sink.contour_cunt;
    sink.svg->pt_cunt = (uint32_t)sink.pt_cunt;

    for (idx = 0; idx < sink.shape_cunt; idx++)
    {
        ret = svg_shape_resolve(&sink, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    *out = sink.svg;

    return LDG_ERR_AOK;
}
