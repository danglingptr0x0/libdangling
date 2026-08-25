#include <stdint.h>
#include <stdarg.h>

#include <dangling/str/fmt.h>
#include <dangling/str/str.h>
#include <dangling/math/math.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_STR_FMT_WIDTH_FROM_ARG '*'
#define LDG_STR_FMT_PRECISION_MARK '.'
#define LDG_STR_FMT_FLAG_LEFT '-'
#define LDG_STR_FMT_FLAG_ZERO '0'
#define LDG_STR_FMT_WIDTH_32 "32"
#define LDG_STR_FMT_WIDTH_64 "64"
#define LDG_STR_FMT_WIDTH_TAG_LEN 2
#define LDG_STR_FMT_SIGN_LEN 1
#define LDG_STR_FMT_PTR_PREFIX "0x"
#define LDG_STR_FMT_PTR_PREFIX_LEN 2
#define LDG_STR_FMT_FRACTION_CAP 17
#define LDG_STR_FMT_ROUND_BIAS 0.5
#define LDG_STR_FMT_NO_PREFIX 0
#define LDG_STR_FMT_EMPTY_PREFIX ""
#define LDG_STR_FMT_MARKER_LEN 3
#define LDG_STR_FMT_UINT64_LIMIT 18446744073709551616.0

typedef struct ldg_str_fmt_sink
{
    char *dst;
    uint64_t cap;
    uint64_t len;
} ldg_str_fmt_sink_t;

typedef struct ldg_str_fmt_field
{
    uint32_t width;
    uint32_t precision;
    uint8_t left_align;
    uint8_t zero_pad;
    uint8_t precision_given;
    uint8_t pudding[1];
} ldg_str_fmt_field_t;

static void fmt_byte_put(ldg_str_fmt_sink_t *sink, char byte)
{
    if (sink->len < sink->cap) { sink->dst[sink->len] = byte; }

    sink->len++;
}

static void fmt_pad_put(ldg_str_fmt_sink_t *sink, char byte, uint32_t cunt)
{
    uint32_t idx = 0;

    for (idx = 0; idx < cunt; idx++) { fmt_byte_put(sink, byte); }
}

static uint32_t fmt_uint_digits_get(uint64_t val, uint32_t base, uint8_t upper, char *buff)
{
    const char *lower_set = "0123456789abcdef";
    const char *upper_set = "0123456789ABCDEF";
    const char *digit_set = (upper == LDG_TRUTH_TRUE) ? upper_set : lower_set;
    uint32_t cunt = 0;

    if (val == 0)
    {
        buff[cunt] = digit_set[0];
        cunt++;

        return cunt;
    }

    while (val > 0)
    {
        buff[cunt] = digit_set[val % base];
        cunt++;
        val /= base;
    }

    return cunt;
}

static void fmt_body_emit(ldg_str_fmt_sink_t *sink, const char *body, uint32_t body_len, const char *prefix, uint32_t prefix_len, const ldg_str_fmt_field_t *field)
{
    uint32_t total = prefix_len + body_len;
    uint32_t pad = 0;
    uint32_t idx = 0;

    if (field->width > total) { pad = field->width - total; }

    if (field->left_align != LDG_TRUTH_TRUE && field->zero_pad != LDG_TRUTH_TRUE) { fmt_pad_put(sink, ' ', pad); }

    for (idx = 0; idx < prefix_len; idx++) { fmt_byte_put(sink, prefix[idx]); }

    if (field->left_align != LDG_TRUTH_TRUE && field->zero_pad == LDG_TRUTH_TRUE) { fmt_pad_put(sink, LDG_STR_FMT_FLAG_ZERO, pad); }

    for (idx = 0; idx < body_len; idx++) { fmt_byte_put(sink, body[idx]); }

    if (field->left_align == LDG_TRUTH_TRUE) { fmt_pad_put(sink, ' ', pad); }
}

static void fmt_uint_emit(ldg_str_fmt_sink_t *sink, uint64_t val, uint32_t base, uint8_t upper, const char *prefix, uint32_t prefix_len, const ldg_str_fmt_field_t *field)
{
    char digits[LDG_STR_FMT_DIGIT_CAP] = LDG_ARR_ZERO_INIT;
    char body[LDG_STR_FMT_DIGIT_CAP] = LDG_ARR_ZERO_INIT;
    uint32_t digit_cunt = 0;
    uint32_t idx = 0;

    digit_cunt = fmt_uint_digits_get(val, base, upper, digits);

    for (idx = 0; idx < digit_cunt; idx++) { body[idx] = digits[digit_cunt - idx - LDG_STR_FMT_SIGN_LEN]; }

    fmt_body_emit(sink, body, digit_cunt, prefix, prefix_len, field);
}

static void fmt_int_emit(ldg_str_fmt_sink_t *sink, int64_t val, const ldg_str_fmt_field_t *field)
{
    const char *sign = "-";
    uint64_t magnitude = 0;

    if (val < 0)
    {
        magnitude = (uint64_t)(-(val + LDG_STR_FMT_SIGN_LEN)) + LDG_STR_FMT_SIGN_LEN;

        fmt_uint_emit(sink, magnitude, LDG_BASE_DEC, LDG_TRUTH_FALSE, sign, LDG_STR_FMT_SIGN_LEN, field);

        return;
    }

    fmt_uint_emit(sink, (uint64_t)val, LDG_BASE_DEC, LDG_TRUTH_FALSE, sign, LDG_STR_FMT_NO_PREFIX, field);
}

static void fmt_text_emit(ldg_str_fmt_sink_t *sink, const char *text, const ldg_str_fmt_field_t *field)
{
    const char *body = (text) ? text : "(null)";
    uint32_t body_len = 0;

    while (body[body_len] != LDG_STR_TERM) { body_len++; }

    if (field->precision_given == LDG_TRUTH_TRUE && field->precision < body_len) { body_len = field->precision; }

    fmt_body_emit(sink, body, body_len, LDG_STR_FMT_EMPTY_PREFIX, LDG_STR_FMT_NO_PREFIX, field);
}

static void fmt_fp64_emit(ldg_str_fmt_sink_t *sink, double val, const ldg_str_fmt_field_t *field)
{
    char body[LDG_STR_FMT_DIGIT_CAP] = LDG_ARR_ZERO_INIT;
    char digits[LDG_STR_FMT_DIGIT_CAP] = LDG_ARR_ZERO_INIT;
    const char *sign = "-";
    uint32_t sign_len = 0;
    uint32_t body_len = 0;
    uint32_t digit_cunt = 0;
    uint32_t idx = 0;
    uint32_t precision = LDG_STR_FMT_PRECISION_DEFAULT;
    uint32_t tail_zero = 0;
    uint64_t whole = 0;
    double magnitude = 0.0;
    double fraction = 0.0;
    double scale = 0.0;

    if (field->precision_given == LDG_TRUTH_TRUE) { precision = field->precision; }

    if (precision >= LDG_STR_FMT_FRACTION_CAP) { precision = LDG_STR_FMT_FRACTION_CAP - LDG_STR_FMT_SIGN_LEN; }

    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE)
    {
        fmt_body_emit(sink, "nan", LDG_STR_FMT_MARKER_LEN, LDG_STR_FMT_EMPTY_PREFIX, LDG_STR_FMT_NO_PREFIX, field);

        return;
    }

    magnitude = ldg_math_abs_get(val);

    if (val < 0.0) { sign_len = LDG_STR_FMT_SIGN_LEN; }

    if (ldg_math_inf_is(val) == LDG_TRUTH_TRUE)
    {
        fmt_body_emit(sink, "inf", LDG_STR_FMT_MARKER_LEN, sign, sign_len, field);

        return;
    }

    while (magnitude >= LDG_STR_FMT_UINT64_LIMIT)
    {
        magnitude /= (double)LDG_BASE_DEC;
        tail_zero++;
    }

    whole = (uint64_t)ldg_math_trunc_get(magnitude);
    fraction = magnitude - (double)whole;

    digit_cunt = fmt_uint_digits_get(whole, LDG_BASE_DEC, LDG_TRUTH_FALSE, digits);

    for (idx = 0; idx < digit_cunt; idx++) { body[body_len + idx] = digits[digit_cunt - idx - LDG_STR_FMT_SIGN_LEN]; }

    body_len += digit_cunt;

    for (idx = 0; idx < tail_zero; idx++)
    {
        body[body_len] = LDG_STR_FMT_FLAG_ZERO;
        body_len++;
    }

    if (precision > 0)
    {
        body[body_len] = LDG_STR_FMT_PRECISION_MARK;
        body_len++;

        scale = 1.0;
        for (idx = 0; idx < precision; idx++) { scale *= (double)LDG_BASE_DEC; }

        whole = (uint64_t)(fraction * scale + LDG_STR_FMT_ROUND_BIAS);

        digit_cunt = fmt_uint_digits_get(whole, LDG_BASE_DEC, LDG_TRUTH_FALSE, digits);

        for (idx = digit_cunt; idx < precision; idx++)
        {
            body[body_len] = LDG_STR_FMT_FLAG_ZERO;
            body_len++;
        }

        for (idx = 0; idx < digit_cunt; idx++) { body[body_len + idx] = digits[digit_cunt - idx - LDG_STR_FMT_SIGN_LEN]; }

        body_len += digit_cunt;
    }

    fmt_body_emit(sink, body, body_len, sign, sign_len, field);
}

static uint32_t fmt_number_parse(const char *fmt, uint64_t *pos)
{
    uint32_t val = 0;

    while (ldg_char_digit_is((uint8_t)fmt[*pos]) == LDG_TRUTH_TRUE)
    {
        val = (val * LDG_BASE_DEC) + (uint32_t)(fmt[*pos] - LDG_STR_FMT_FLAG_ZERO);
        (*pos)++;
    }

    return val;
}

static uint8_t fmt_width_tag_match_is(const char *fmt, uint64_t pos, const char *tag)
{
    if (fmt[pos] != tag[0]) { return LDG_TRUTH_FALSE; }

    if (fmt[pos + LDG_STR_FMT_SIGN_LEN] != tag[LDG_STR_FMT_SIGN_LEN]) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_str_variable_fmt(char *dst, uint64_t cap, uint64_t *out_len, const char *fmt, va_list args)
{
    ldg_str_fmt_sink_t sink = LDG_STRUCT_ZERO_INIT;
    ldg_str_fmt_field_t field = LDG_STRUCT_ZERO_INIT;
    uint64_t pos = 0;
    char spec = 0;

    if (LDG_UNLIKELY(!out_len || !fmt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(!dst && cap > 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    sink.dst = dst;
    sink.cap = cap;

    while (fmt[pos] != LDG_STR_TERM)
    {
        if (fmt[pos] != LDG_STR_FMT_ESCAPE)
        {
            fmt_byte_put(&sink, fmt[pos]);
            pos++;
            continue;
        }

        pos++;

        field.width = 0;
        field.precision = 0;
        field.left_align = LDG_TRUTH_FALSE;
        field.zero_pad = LDG_TRUTH_FALSE;
        field.precision_given = LDG_TRUTH_FALSE;

        while (fmt[pos] == LDG_STR_FMT_FLAG_LEFT || fmt[pos] == LDG_STR_FMT_FLAG_ZERO)
        {
            if (fmt[pos] == LDG_STR_FMT_FLAG_LEFT) { field.left_align = LDG_TRUTH_TRUE; }
            else { field.zero_pad = LDG_TRUTH_TRUE; }

            pos++;
        }

        if (fmt[pos] == LDG_STR_FMT_WIDTH_FROM_ARG)
        {
            field.width = va_arg(args, uint32_t);
            pos++;
        }
        else { field.width = fmt_number_parse(fmt, &pos); }

        if (fmt[pos] == LDG_STR_FMT_PRECISION_MARK)
        {
            pos++;
            field.precision_given = LDG_TRUTH_TRUE;

            if (fmt[pos] == LDG_STR_FMT_WIDTH_FROM_ARG)
            {
                field.precision = va_arg(args, uint32_t);
                pos++;
            }
            else { field.precision = fmt_number_parse(fmt, &pos); }
        }

        spec = fmt[pos];

        if (spec == LDG_STR_FMT_ESCAPE)
        {
            fmt_byte_put(&sink, LDG_STR_FMT_ESCAPE);
            pos++;
            continue;
        }

        if (spec == 's')
        {
            fmt_text_emit(&sink, va_arg(args, const char *), &field);
            pos++;
            continue;
        }

        if (spec == 'c')
        {
            fmt_byte_put(&sink, (char)va_arg(args, uint32_t));
            pos++;
            continue;
        }

        if (spec == 'f')
        {
            fmt_fp64_emit(&sink, va_arg(args, double), &field);
            pos++;
            continue;
        }

        if (spec == 'p')
        {
            fmt_uint_emit(&sink, (uint64_t)(uintptr_t)va_arg(args, void *), LDG_BASE_HEX, LDG_TRUTH_FALSE, LDG_STR_FMT_PTR_PREFIX, LDG_STR_FMT_PTR_PREFIX_LEN, &field);
            pos++;
            continue;
        }

        pos++;

        if (spec == 'u' || spec == 'x' || spec == 'X' || spec == 'i')
        {
            if (fmt_width_tag_match_is(fmt, pos, LDG_STR_FMT_WIDTH_64) == LDG_TRUTH_TRUE)
            {
                pos += LDG_STR_FMT_WIDTH_TAG_LEN;

                if (spec == 'i') { fmt_int_emit(&sink, va_arg(args, int64_t), &field); }
                else if (spec == 'u') { fmt_uint_emit(&sink, va_arg(args, uint64_t), LDG_BASE_DEC, LDG_TRUTH_FALSE, LDG_STR_FMT_EMPTY_PREFIX, LDG_STR_FMT_NO_PREFIX, &field); }
                else { fmt_uint_emit(&sink, va_arg(args, uint64_t), LDG_BASE_HEX, (spec == 'X') ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, LDG_STR_FMT_EMPTY_PREFIX, LDG_STR_FMT_NO_PREFIX, &field); }

                continue;
            }

            if (fmt_width_tag_match_is(fmt, pos, LDG_STR_FMT_WIDTH_32) == LDG_TRUTH_TRUE)
            {
                pos += LDG_STR_FMT_WIDTH_TAG_LEN;

                if (spec == 'i') { fmt_int_emit(&sink, (int64_t)va_arg(args, int32_t), &field); }
                else if (spec == 'u') { fmt_uint_emit(&sink, (uint64_t)va_arg(args, uint32_t), LDG_BASE_DEC, LDG_TRUTH_FALSE, LDG_STR_FMT_EMPTY_PREFIX, LDG_STR_FMT_NO_PREFIX, &field); }
                else { fmt_uint_emit(&sink, (uint64_t)va_arg(args, uint32_t), LDG_BASE_HEX, (spec == 'X') ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE, LDG_STR_FMT_EMPTY_PREFIX, LDG_STR_FMT_NO_PREFIX, &field); }

                continue;
            }

            return LDG_ERR_STR_FMT;
        }

        return LDG_ERR_STR_FMT;
    }

    if (sink.len < sink.cap) { sink.dst[sink.len] = LDG_STR_TERM; }
    else if (cap > 0) { sink.dst[cap - LDG_STR_FMT_SIGN_LEN] = LDG_STR_TERM; }

    *out_len = sink.len;

    if (sink.len >= cap) { return LDG_ERR_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint32_t ldg_str_fmt(char *dst, uint64_t cap, uint64_t *out_len, const char *fmt, ...)
{
    va_list args = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out_len || !fmt)) { return LDG_ERR_FUNC_ARG_NULL; }

    va_start(args, fmt);
    ret = ldg_str_variable_fmt(dst, cap, out_len, fmt, args);
    va_end(args);

    return ret;
}
