#include <stdint.h>
#include <math.h>

#include <dangling/math/math.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_MATH_FP64_MANT_BITS 52
#define LDG_MATH_FP64_EXP_BIAS 1023
#define LDG_MATH_FP64_EXP_MASK 0x7FFULL
#define LDG_MATH_FP64_MANT_MASK 0xFFFFFFFFFFFFFULL
#define LDG_MATH_FP64_ABS_MASK 0x7FFFFFFFFFFFFFFFULL
#define LDG_MATH_FP64_SIGN_MASK 0x8000000000000000ULL
#define LDG_MATH_FP64_EXP_MAX 2047
#define LDG_MATH_FP64_NAN_BITS 0x7FF8000000000000ULL
#define LDG_MATH_FRACTION_BIAS 1
#define LDG_MATH_SUBNORMAL_SCALE_BITS 54
#define LDG_MATH_SUBNORMAL_SCALE 18014398509481984.0

typedef union ldg_math_fp64_bits
{
    double val;
    uint64_t bits;
} ldg_math_fp64_bits_t;

static double fp64_nan_get(void)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    split.bits = LDG_MATH_FP64_NAN_BITS;

    return split.val;
}

uint8_t ldg_math_nan_is(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    split.val = val;

    if (((split.bits >> LDG_MATH_FP64_MANT_BITS) & LDG_MATH_FP64_EXP_MASK) != LDG_MATH_FP64_EXP_MAX) { return LDG_TRUTH_FALSE; }

    if ((split.bits & LDG_MATH_FP64_MANT_MASK) == 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint8_t ldg_math_inf_is(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    split.val = val;

    if (((split.bits >> LDG_MATH_FP64_MANT_BITS) & LDG_MATH_FP64_EXP_MASK) != LDG_MATH_FP64_EXP_MAX) { return LDG_TRUTH_FALSE; }

    if ((split.bits & LDG_MATH_FP64_MANT_MASK) != 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint8_t ldg_math_finite_is(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    split.val = val;

    if (((split.bits >> LDG_MATH_FP64_MANT_BITS) & LDG_MATH_FP64_EXP_MASK) == LDG_MATH_FP64_EXP_MAX) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

double ldg_math_abs_get(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    split.val = val;
    split.bits &= LDG_MATH_FP64_ABS_MASK;

    return split.val;
}

double ldg_math_floor_get(double val)
{
    if (ldg_math_finite_is(val) != LDG_TRUTH_TRUE) { return val; }

    return floor(val);
}

double ldg_math_ceil_get(double val)
{
    if (ldg_math_finite_is(val) != LDG_TRUTH_TRUE) { return val; }

    return ceil(val);
}

double ldg_math_trunc_get(double val)
{
    if (ldg_math_finite_is(val) != LDG_TRUTH_TRUE) { return val; }

    return trunc(val);
}

double ldg_math_min_get(double lhs, double rhs)
{
    if (ldg_math_nan_is(lhs) == LDG_TRUTH_TRUE) { return rhs; }

    if (ldg_math_nan_is(rhs) == LDG_TRUTH_TRUE) { return lhs; }

    if (lhs < rhs) { return lhs; }

    return rhs;
}

double ldg_math_max_get(double lhs, double rhs)
{
    if (ldg_math_nan_is(lhs) == LDG_TRUTH_TRUE) { return rhs; }

    if (ldg_math_nan_is(rhs) == LDG_TRUTH_TRUE) { return lhs; }

    if (lhs > rhs) { return lhs; }

    return rhs;
}

double ldg_math_sqrt_get(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    split.val = val;

    if ((split.bits & LDG_MATH_FP64_ABS_MASK) == 0) { return val; }

    if ((split.bits & LDG_MATH_FP64_SIGN_MASK) != 0) { return fp64_nan_get(); }

    if (ldg_math_inf_is(val) == LDG_TRUTH_TRUE) { return val; }

    return sqrt(val);
}

double ldg_math_pwr2_scale_get(double val, int32_t exponent)
{
    if (ldg_math_finite_is(val) != LDG_TRUTH_TRUE) { return val; }

    return scalbn(val, exponent);
}

double ldg_math_next_toward_get(double val, double toward)
{
    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    if (ldg_math_nan_is(toward) == LDG_TRUTH_TRUE) { return toward; }

    return nextafter(val, toward);
}

double ldg_math_hypotenuse_get(double lhs, double rhs)
{
    if (ldg_math_inf_is(lhs) == LDG_TRUTH_TRUE || ldg_math_inf_is(rhs) == LDG_TRUTH_TRUE) { return ldg_math_abs_get(lhs) + ldg_math_abs_get(rhs); }

    if (ldg_math_nan_is(lhs) == LDG_TRUTH_TRUE) { return lhs; }

    if (ldg_math_nan_is(rhs) == LDG_TRUTH_TRUE) { return rhs; }

    return hypot(lhs, rhs);
}

double ldg_math_exp_get(double val)
{
    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    if (val > LDG_MATH_EXP_ARG_MAX) { return LDG_MATH_INFINITY; }

    if (val < LDG_MATH_EXP_ARG_MIN) { return 0.0; }

    return exp(val);
}

double ldg_math_log_get(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    split.val = val;

    if ((split.bits & LDG_MATH_FP64_SIGN_MASK) != 0 || (split.bits & LDG_MATH_FP64_ABS_MASK) == 0) { return fp64_nan_get(); }

    if (ldg_math_inf_is(val) == LDG_TRUTH_TRUE) { return val; }

    return log(val);
}

double ldg_math_pwr_get(double base, double exponent)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;
    double whole = 0.0;

    if (ldg_math_nan_is(base) == LDG_TRUTH_TRUE) { return base; }

    if (ldg_math_nan_is(exponent) == LDG_TRUTH_TRUE) { return exponent; }

    if (ldg_math_abs_get(exponent) < LDG_MATH_EPSILON) { return 1.0; }

    split.val = base;

    if ((split.bits & LDG_MATH_FP64_ABS_MASK) == 0) { return 0.0; }

    if ((split.bits & LDG_MATH_FP64_SIGN_MASK) != 0)
    {
        whole = trunc(exponent);

        if (ldg_math_abs_get(whole - exponent) > LDG_MATH_EPSILON) { return fp64_nan_get(); }
    }

    return pow(base, exponent);
}

double ldg_math_sin_get(double val)
{
    if (ldg_math_finite_is(val) != LDG_TRUTH_TRUE) { return val; }

    return sin(val);
}

double ldg_math_cos_get(double val)
{
    if (ldg_math_finite_is(val) != LDG_TRUTH_TRUE) { return val; }

    return cos(val);
}

double ldg_math_arctan_get(double val)
{
    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    if (ldg_math_inf_is(val) == LDG_TRUTH_TRUE)
    {
        if (val < 0.0) { return LDG_MATH_NEG_HALF_PI; }

        return LDG_MATH_HALF_PI;
    }

    return atan(val);
}

double ldg_math_arctan2_get(double num, double den)
{
    if (ldg_math_nan_is(num) == LDG_TRUTH_TRUE) { return num; }

    if (ldg_math_nan_is(den) == LDG_TRUTH_TRUE) { return den; }

    if (ldg_math_abs_get(den) < LDG_MATH_EPSILON && ldg_math_abs_get(num) < LDG_MATH_EPSILON) { return 0.0; }

    return atan2(num, den);
}

double ldg_math_arcsin_get(double val)
{
    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    if (ldg_math_abs_get(val) > 1.0) { return fp64_nan_get(); }

    return asin(val);
}

double ldg_math_arccos_get(double val)
{
    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    if (ldg_math_abs_get(val) > 1.0) { return fp64_nan_get(); }

    return acos(val);
}

uint32_t ldg_math_int64_abs_get(int64_t val, int64_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(val == INT64_MIN)) { return LDG_ERR_OVERFLOW; }

    *out = (val < 0) ? -val : val;

    return LDG_ERR_AOK;
}

double ldg_math_tan_get(double val)
{
    if (ldg_math_finite_is(val) != LDG_TRUTH_TRUE) { return val; }

    if (ldg_math_abs_get(ldg_math_cos_get(val)) < LDG_MATH_EPSILON) { return fp64_nan_get(); }

    return tan(val);
}

double ldg_math_log10_get(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    split.val = val;

    if ((split.bits & LDG_MATH_FP64_SIGN_MASK) != 0 || (split.bits & LDG_MATH_FP64_ABS_MASK) == 0) { return fp64_nan_get(); }

    if (ldg_math_inf_is(val) == LDG_TRUTH_TRUE) { return val; }

    return log10(val);
}

double ldg_math_remainder_get(double numer, double denom)
{
    if (ldg_math_nan_is(numer) == LDG_TRUTH_TRUE) { return numer; }

    if (ldg_math_nan_is(denom) == LDG_TRUTH_TRUE) { return denom; }

    if (ldg_math_abs_get(denom) < LDG_MATH_EPSILON || ldg_math_inf_is(numer) == LDG_TRUTH_TRUE) { return fp64_nan_get(); }

    if (ldg_math_inf_is(denom) == LDG_TRUTH_TRUE) { return numer; }

    return fmod(numer, denom);
}

uint32_t ldg_math_fraction_split(double val, double *out_fraction, int32_t *out_exponent)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;
    int32_t biased = 0;

    if (LDG_UNLIKELY(!out_fraction || !out_exponent)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_fraction = val;
    *out_exponent = 0;

    if (ldg_math_finite_is(val) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    split.val = val;

    if ((split.bits & LDG_MATH_FP64_ABS_MASK) == 0) { return LDG_ERR_AOK; }

    biased = (int32_t)((split.bits >> LDG_MATH_FP64_MANT_BITS) & LDG_MATH_FP64_EXP_MASK);

    if (biased == 0)
    {
        split.val = val * LDG_MATH_SUBNORMAL_SCALE;
        biased = (int32_t)((split.bits >> LDG_MATH_FP64_MANT_BITS) & LDG_MATH_FP64_EXP_MASK);
        *out_exponent -= LDG_MATH_SUBNORMAL_SCALE_BITS;
    }

    *out_exponent += biased - LDG_MATH_FP64_EXP_BIAS + LDG_MATH_FRACTION_BIAS;

    split.bits &= ~(LDG_MATH_FP64_EXP_MASK << LDG_MATH_FP64_MANT_BITS);
    split.bits |= (uint64_t)(LDG_MATH_FP64_EXP_BIAS - LDG_MATH_FRACTION_BIAS) << LDG_MATH_FP64_MANT_BITS;

    *out_fraction = split.val;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_whole_split(double val, double *out_whole, double *out_fraction)
{
    if (LDG_UNLIKELY(!out_whole || !out_fraction)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_whole = 0.0;
    *out_fraction = 0.0;

    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE)
    {
        *out_whole = val;
        *out_fraction = val;

        return LDG_ERR_AOK;
    }

    if (ldg_math_inf_is(val) == LDG_TRUTH_TRUE)
    {
        *out_whole = val;

        return LDG_ERR_AOK;
    }

    *out_fraction = modf(val, out_whole);

    return LDG_ERR_AOK;
}
