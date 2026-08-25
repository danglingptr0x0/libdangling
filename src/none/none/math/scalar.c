#include <stdint.h>

#include <dangling/math/math.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_MATH_FP64_MANT_BITS 52
#define LDG_MATH_FP64_EXP_BIAS 1023
#define LDG_MATH_FP64_EXP_MASK 0x7FFULL
#define LDG_MATH_FP64_MANT_MASK 0xFFFFFFFFFFFFFULL
#define LDG_MATH_FP64_SIGN_MASK 0x8000000000000000ULL
#define LDG_MATH_FP64_ABS_MASK 0x7FFFFFFFFFFFFFFFULL
#define LDG_MATH_FP64_SIGN_SHIFT 63
#define LDG_MATH_FP64_EXP_MAX 2047
#define LDG_MATH_FP64_NAN_BITS 0x7FF8000000000000ULL
#define LDG_MATH_FP64_TINIEST 0x0000000000000001ULL
#define LDG_MATH_FRACTION_BIAS 1
#define LDG_MATH_SUBNORMAL_SCALE_BITS 54
#define LDG_MATH_SUBNORMAL_SCALE 18014398509481984.0
#define LDG_MATH_SQRT_SEED 1.5
#define LDG_MATH_SQRT_ITER_CUNT 8
#define LDG_MATH_NEWTON_MEAN 0.5
#define LDG_MATH_ROUND_NEAREST_BIAS 0.5
#define LDG_MATH_EXP_ODD_MASK 1

#define LDG_MATH_LN2_HI 0.693147180369123816490
#define LDG_MATH_LN2_LO 1.90821492927058770002e-10
#define LDG_MATH_EXP_TERM_CUNT 13

#define LDG_MATH_LOG_SERIES_CUNT 8
#define LDG_MATH_ODD_DENOM_STEP 2.0
#define LDG_MATH_ODD_DENOM_BASE 1.0

#define LDG_MATH_HALF_PI_HI 1.57079632673412561417
#define LDG_MATH_HALF_PI_MID 6.07710050650619224932e-11
#define LDG_MATH_HALF_PI_LO 2.02226624879595063154e-21
#define LDG_MATH_QUADRANT_MASK 3
#define LDG_MATH_QUADRANT_COS_POS 1
#define LDG_MATH_QUADRANT_SIN_NEG 2
#define LDG_MATH_QUADRANT_COS_NEG 3

#define LDG_MATH_SIN_COEF3 -0.16666666666666666667
#define LDG_MATH_SIN_COEF5 0.00833333333333333333
#define LDG_MATH_SIN_COEF7 -0.00019841269841269841
#define LDG_MATH_SIN_COEF9 2.75573192239858906526e-6
#define LDG_MATH_SIN_COEF11 -2.50521083854417187751e-8
#define LDG_MATH_SIN_COEF13 1.60590438368216145994e-10

#define LDG_MATH_COS_COEF2 -0.5
#define LDG_MATH_COS_COEF4 0.04166666666666666667
#define LDG_MATH_COS_COEF6 -0.00138888888888888889
#define LDG_MATH_COS_COEF8 2.48015873015873015873e-5
#define LDG_MATH_COS_COEF10 -2.75573192239858906526e-7
#define LDG_MATH_COS_COEF12 2.08767569878680989792e-9

#define LDG_MATH_ARCTAN_SERIES_CUNT 24
#define LDG_MATH_ARCTAN_REDUCE_LIMIT 0.26794919243112270647

typedef union ldg_math_fp64_bits
{
    double val;
    uint64_t bits;
} ldg_math_fp64_bits_t;

static uint32_t fp64_exp_get(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    split.val = val;

    return (uint32_t)((split.bits >> LDG_MATH_FP64_MANT_BITS) & LDG_MATH_FP64_EXP_MASK);
}

static uint64_t fp64_mant_get(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    split.val = val;

    return split.bits & LDG_MATH_FP64_MANT_MASK;
}

uint8_t ldg_math_nan_is(double val)
{
    if (fp64_exp_get(val) != LDG_MATH_FP64_EXP_MAX) { return LDG_TRUTH_FALSE; }

    if (fp64_mant_get(val) == 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint8_t ldg_math_inf_is(double val)
{
    if (fp64_exp_get(val) != LDG_MATH_FP64_EXP_MAX) { return LDG_TRUTH_FALSE; }

    if (fp64_mant_get(val) != 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint8_t ldg_math_finite_is(double val)
{
    if (fp64_exp_get(val) == LDG_MATH_FP64_EXP_MAX) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

double ldg_math_abs_get(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    split.val = val;
    split.bits &= LDG_MATH_FP64_ABS_MASK;

    return split.val;
}

double ldg_math_trunc_get(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;
    uint64_t mant_keep = 0;
    int32_t exponent = 0;

    if (ldg_math_finite_is(val) != LDG_TRUTH_TRUE) { return val; }

    exponent = (int32_t)fp64_exp_get(val) - LDG_MATH_FP64_EXP_BIAS;

    if (exponent < 0)
    {
        split.val = val;
        split.bits &= LDG_MATH_FP64_SIGN_MASK;

        return split.val;
    }

    if (exponent >= (int32_t)LDG_MATH_FP64_MANT_BITS) { return val; }

    split.val = val;
    mant_keep = LDG_MATH_FP64_MANT_MASK >> exponent;
    split.bits &= ~mant_keep;

    return split.val;
}

double ldg_math_floor_get(double val)
{
    double whole = 0.0;

    whole = ldg_math_trunc_get(val);

    if (whole > val) { return whole - 1.0; }

    return whole;
}

double ldg_math_ceil_get(double val)
{
    double whole = 0.0;

    whole = ldg_math_trunc_get(val);

    if (whole < val) { return whole + 1.0; }

    return whole;
}

static double fp64_pow2_get(int32_t exponent)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;
    uint64_t biased = 0;

    biased = (uint64_t)(exponent + LDG_MATH_FP64_EXP_BIAS);
    split.bits = biased << LDG_MATH_FP64_MANT_BITS;

    return split.val;
}

double ldg_math_pwr2_scale_get(double val, int32_t exponent)
{
    if (ldg_math_finite_is(val) != LDG_TRUTH_TRUE) { return val; }

    return val * fp64_pow2_get(exponent);
}

double ldg_math_next_toward_get(double val, double toward)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    if (ldg_math_nan_is(toward) == LDG_TRUTH_TRUE) { return toward; }

    if (ldg_math_abs_get(val - toward) < LDG_MATH_EPSILON) { return toward; }

    split.val = val;

    if ((split.bits & LDG_MATH_FP64_ABS_MASK) == 0)
    {
        split.bits = LDG_MATH_FP64_TINIEST;

        if (toward < val) { split.bits |= LDG_MATH_FP64_SIGN_MASK; }

        return split.val;
    }

    if ((val < toward) == ((split.bits & LDG_MATH_FP64_SIGN_MASK) == 0)) { split.bits++; }
    else { split.bits--; }

    return split.val;
}

double ldg_math_sqrt_get(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;
    double mant = 0.0;
    double root = 0.0;
    int32_t exponent = 0;
    uint32_t idx = 0;

    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    split.val = val;

    if ((split.bits & LDG_MATH_FP64_ABS_MASK) == 0) { return val; }

    if ((split.bits & LDG_MATH_FP64_SIGN_MASK) != 0)
    {
        split.bits = LDG_MATH_FP64_NAN_BITS;

        return split.val;
    }

    if (ldg_math_inf_is(val) == LDG_TRUTH_TRUE) { return val; }

    exponent = (int32_t)fp64_exp_get(val) - LDG_MATH_FP64_EXP_BIAS;

    split.bits &= ~(LDG_MATH_FP64_EXP_MASK << LDG_MATH_FP64_MANT_BITS);
    split.bits |= (uint64_t)LDG_MATH_FP64_EXP_BIAS << LDG_MATH_FP64_MANT_BITS;
    mant = split.val;

    if ((exponent & LDG_MATH_EXP_ODD_MASK) != 0)
    {
        mant *= 2.0;
        exponent--;
    }

    root = LDG_MATH_SQRT_SEED;
    for (idx = 0; idx < LDG_MATH_SQRT_ITER_CUNT; idx++) { root = LDG_MATH_NEWTON_MEAN * (root + (mant / root)); }

    return root * fp64_pow2_get(exponent / 2);
}

double ldg_math_hypotenuse_get(double lhs, double rhs)
{
    double big = 0.0;
    double small = 0.0;
    double ratio = 0.0;

    if (ldg_math_inf_is(lhs) == LDG_TRUTH_TRUE || ldg_math_inf_is(rhs) == LDG_TRUTH_TRUE) { return ldg_math_abs_get(lhs) + ldg_math_abs_get(rhs); }

    if (ldg_math_nan_is(lhs) == LDG_TRUTH_TRUE) { return lhs; }

    if (ldg_math_nan_is(rhs) == LDG_TRUTH_TRUE) { return rhs; }

    big = ldg_math_abs_get(lhs);
    small = ldg_math_abs_get(rhs);

    if (small > big)
    {
        ratio = big;
        big = small;
        small = ratio;
    }

    if (ldg_math_abs_get(big) < LDG_MATH_EPSILON) { return big; }

    ratio = small / big;

    return big * ldg_math_sqrt_get(1.0 + (ratio * ratio));
}

double ldg_math_exp_get(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;
    double rest = 0.0;
    double term = 0.0;
    double sum = 0.0;
    int32_t whole = 0;
    uint32_t idx = 0;

    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    if (val > LDG_MATH_EXP_ARG_MAX)
    {
        split.bits = (uint64_t)LDG_MATH_FP64_EXP_MAX << LDG_MATH_FP64_MANT_BITS;

        return split.val;
    }

    if (val < LDG_MATH_EXP_ARG_MIN) { return 0.0; }

    whole = (int32_t)ldg_math_floor_get((val * LDG_MATH_LOG2_E) + LDG_MATH_ROUND_NEAREST_BIAS);
    rest = val - ((double)whole * LDG_MATH_LN2_HI);
    rest -= (double)whole * LDG_MATH_LN2_LO;

    sum = 1.0;
    term = 1.0;
    for (idx = 1; idx <= LDG_MATH_EXP_TERM_CUNT; idx++)
    {
        term *= rest / (double)idx;
        sum += term;
    }

    return sum * fp64_pow2_get(whole);
}

double ldg_math_log_get(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;
    double mant = 0.0;
    double ratio = 0.0;
    double sq = 0.0;
    double sum = 0.0;
    double power = 0.0;
    double denom = 0.0;
    int32_t exponent = 0;
    uint32_t idx = 0;

    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    split.val = val;

    if ((split.bits & LDG_MATH_FP64_SIGN_MASK) != 0 || (split.bits & LDG_MATH_FP64_ABS_MASK) == 0)
    {
        split.bits = LDG_MATH_FP64_NAN_BITS;

        return split.val;
    }

    if (ldg_math_inf_is(val) == LDG_TRUTH_TRUE) { return val; }

    exponent = (int32_t)fp64_exp_get(val) - LDG_MATH_FP64_EXP_BIAS;

    split.bits &= ~(LDG_MATH_FP64_EXP_MASK << LDG_MATH_FP64_MANT_BITS);
    split.bits |= (uint64_t)LDG_MATH_FP64_EXP_BIAS << LDG_MATH_FP64_MANT_BITS;
    mant = split.val;

    if (mant > LDG_MATH_SQRT2)
    {
        mant *= LDG_MATH_NEWTON_MEAN;
        exponent++;
    }

    ratio = (mant - 1.0) / (mant + 1.0);
    sq = ratio * ratio;
    power = ratio;
    sum = ratio;
    denom = LDG_MATH_ODD_DENOM_BASE;

    for (idx = 0; idx < LDG_MATH_LOG_SERIES_CUNT; idx++)
    {
        power *= sq;
        denom += LDG_MATH_ODD_DENOM_STEP;
        sum += power / denom;
    }

    return (sum * 2.0) + ((double)exponent * (LDG_MATH_LN2_HI + LDG_MATH_LN2_LO));
}

double ldg_math_pwr_get(double base, double exponent)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;
    double whole = 0.0;
    double magnitude = 0.0;
    double parity = 0.0;

    if (ldg_math_nan_is(base) == LDG_TRUTH_TRUE) { return base; }

    if (ldg_math_nan_is(exponent) == LDG_TRUTH_TRUE) { return exponent; }

    if (ldg_math_abs_get(exponent) < LDG_MATH_EPSILON) { return 1.0; }

    split.val = base;

    if ((split.bits & LDG_MATH_FP64_ABS_MASK) == 0) { return 0.0; }

    if ((split.bits & LDG_MATH_FP64_SIGN_MASK) == 0) { return ldg_math_exp_get(exponent * ldg_math_log_get(base)); }

    whole = ldg_math_trunc_get(exponent);

    if (ldg_math_abs_get(whole - exponent) > LDG_MATH_EPSILON)
    {
        split.bits = LDG_MATH_FP64_NAN_BITS;

        return split.val;
    }

    magnitude = ldg_math_exp_get(exponent * ldg_math_log_get(-base));
    parity = whole * LDG_MATH_NEWTON_MEAN;

    if (ldg_math_abs_get(ldg_math_trunc_get(parity) - parity) > LDG_MATH_EPSILON) { return -magnitude; }

    return magnitude;
}

static double sin_poly_get(double val)
{
    double sq = val * val;
    double sum = LDG_MATH_SIN_COEF13;

    sum = (sum * sq) + LDG_MATH_SIN_COEF11;
    sum = (sum * sq) + LDG_MATH_SIN_COEF9;
    sum = (sum * sq) + LDG_MATH_SIN_COEF7;
    sum = (sum * sq) + LDG_MATH_SIN_COEF5;
    sum = (sum * sq) + LDG_MATH_SIN_COEF3;

    return val + (val * sq * sum);
}

static double cos_poly_get(double val)
{
    double sq = val * val;
    double sum = LDG_MATH_COS_COEF12;

    sum = (sum * sq) + LDG_MATH_COS_COEF10;
    sum = (sum * sq) + LDG_MATH_COS_COEF8;
    sum = (sum * sq) + LDG_MATH_COS_COEF6;
    sum = (sum * sq) + LDG_MATH_COS_COEF4;
    sum = (sum * sq) + LDG_MATH_COS_COEF2;

    return 1.0 + (sq * sum);
}

static double trig_reduce(double val, uint32_t *quadrant)
{
    double whole = 0.0;
    double rest = 0.0;

    whole = ldg_math_floor_get((val * LDG_MATH_TWO_OVER_PI) + LDG_MATH_ROUND_NEAREST_BIAS);

    rest = val - (whole * LDG_MATH_HALF_PI_HI);
    rest -= whole * LDG_MATH_HALF_PI_MID;
    rest -= whole * LDG_MATH_HALF_PI_LO;

    *quadrant = (uint32_t)((int32_t)whole & LDG_MATH_QUADRANT_MASK);

    return rest;
}

double ldg_math_sin_get(double val)
{
    double rest = 0.0;
    uint32_t quadrant = 0;

    if (ldg_math_finite_is(val) != LDG_TRUTH_TRUE) { return val; }

    rest = trig_reduce(val, &quadrant);

    if (quadrant == LDG_MATH_QUADRANT_COS_POS) { return cos_poly_get(rest); }

    if (quadrant == LDG_MATH_QUADRANT_SIN_NEG) { return -sin_poly_get(rest); }

    if (quadrant == LDG_MATH_QUADRANT_COS_NEG) { return -cos_poly_get(rest); }

    return sin_poly_get(rest);
}

double ldg_math_cos_get(double val)
{
    double rest = 0.0;
    uint32_t quadrant = 0;

    if (ldg_math_finite_is(val) != LDG_TRUTH_TRUE) { return val; }

    rest = trig_reduce(val, &quadrant);

    if (quadrant == LDG_MATH_QUADRANT_COS_POS) { return -sin_poly_get(rest); }

    if (quadrant == LDG_MATH_QUADRANT_SIN_NEG) { return -cos_poly_get(rest); }

    if (quadrant == LDG_MATH_QUADRANT_COS_NEG) { return sin_poly_get(rest); }

    return cos_poly_get(rest);
}

static double arctan_series_get(double val)
{
    double sq = val * val;
    double power = val;
    double sum = val;
    double denom = 0.0;
    uint32_t idx = 0;

    denom = LDG_MATH_ODD_DENOM_BASE;

    for (idx = 0; idx < LDG_MATH_ARCTAN_SERIES_CUNT; idx++)
    {
        power *= sq;
        denom += LDG_MATH_ODD_DENOM_STEP;

        if ((idx & LDG_MATH_EXP_ODD_MASK) == 0) { sum -= power / denom; }
        else { sum += power / denom; }
    }

    return sum;
}

double ldg_math_arctan_get(double val)
{
    double magnitude = 0.0;
    double reduced = 0.0;
    double res = 0.0;

    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    magnitude = ldg_math_abs_get(val);

    if (ldg_math_inf_is(val) == LDG_TRUTH_TRUE) { res = LDG_MATH_HALF_PI; }
    else if (magnitude > 1.0)
    {
        reduced = 1.0 / magnitude;

        if (reduced > LDG_MATH_ARCTAN_REDUCE_LIMIT)
        {
            reduced = ((LDG_MATH_SQRT3 * reduced) - 1.0) / (LDG_MATH_SQRT3 + reduced);
            res = LDG_MATH_HALF_PI - (LDG_MATH_SIXTH_PI + arctan_series_get(reduced));
        }
        else { res = LDG_MATH_HALF_PI - arctan_series_get(reduced); }
    }
    else if (magnitude > LDG_MATH_ARCTAN_REDUCE_LIMIT)
    {
        reduced = ((LDG_MATH_SQRT3 * magnitude) - 1.0) / (LDG_MATH_SQRT3 + magnitude);
        res = LDG_MATH_SIXTH_PI + arctan_series_get(reduced);
    }
    else { res = arctan_series_get(magnitude); }

    if (val < 0.0) { return -res; }

    return res;
}

double ldg_math_arctan2_get(double num, double den)
{
    double angle = 0.0;

    if (ldg_math_nan_is(num) == LDG_TRUTH_TRUE) { return num; }

    if (ldg_math_nan_is(den) == LDG_TRUTH_TRUE) { return den; }

    if (ldg_math_abs_get(den) < LDG_MATH_EPSILON)
    {
        if (ldg_math_abs_get(num) < LDG_MATH_EPSILON) { return 0.0; }

        if (num > 0.0) { return LDG_MATH_HALF_PI; }

        return LDG_MATH_NEG_HALF_PI;
    }

    angle = ldg_math_arctan_get(num / den);

    if (den > 0.0) { return angle; }

    if (num < 0.0) { return angle - LDG_MATH_PI; }

    return angle + LDG_MATH_PI;
}

double ldg_math_arcsin_get(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    if (ldg_math_abs_get(val) > 1.0)
    {
        split.bits = LDG_MATH_FP64_NAN_BITS;

        return split.val;
    }

    return ldg_math_arctan2_get(val, ldg_math_sqrt_get(1.0 - (val * val)));
}

double ldg_math_arccos_get(double val)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;

    if (ldg_math_nan_is(val) == LDG_TRUTH_TRUE) { return val; }

    if (ldg_math_abs_get(val) > 1.0)
    {
        split.bits = LDG_MATH_FP64_NAN_BITS;

        return split.val;
    }

    return ldg_math_arctan2_get(ldg_math_sqrt_get(1.0 - (val * val)), val);
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
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;
    double sine = 0.0;
    double cosine = 0.0;

    if (ldg_math_finite_is(val) != LDG_TRUTH_TRUE) { return val; }

    sine = ldg_math_sin_get(val);
    cosine = ldg_math_cos_get(val);

    if (ldg_math_abs_get(cosine) < LDG_MATH_EPSILON)
    {
        split.bits = LDG_MATH_FP64_NAN_BITS;

        return split.val;
    }

    return sine / cosine;
}

double ldg_math_log10_get(double val)
{
    return ldg_math_log_get(val) / LDG_MATH_LN10;
}

double ldg_math_remainder_get(double numer, double denom)
{
    ldg_math_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;
    double quotient = 0.0;

    if (ldg_math_nan_is(numer) == LDG_TRUTH_TRUE) { return numer; }

    if (ldg_math_nan_is(denom) == LDG_TRUTH_TRUE) { return denom; }

    if (ldg_math_abs_get(denom) < LDG_MATH_EPSILON || ldg_math_inf_is(numer) == LDG_TRUTH_TRUE)
    {
        split.bits = LDG_MATH_FP64_NAN_BITS;

        return split.val;
    }

    if (ldg_math_inf_is(denom) == LDG_TRUTH_TRUE) { return numer; }

    quotient = ldg_math_trunc_get(numer / denom);

    return numer - (quotient * denom);
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

    biased = (int32_t)fp64_exp_get(val);

    if (biased == 0)
    {
        split.val = val * LDG_MATH_SUBNORMAL_SCALE;
        biased = (int32_t)fp64_exp_get(split.val);
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

    *out_whole = ldg_math_trunc_get(val);
    *out_fraction = val - *out_whole;

    return LDG_ERR_AOK;
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
