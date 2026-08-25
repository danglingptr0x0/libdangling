
#include <dangling/core/err.h>
#include <dangling/math/math.h>

#include "conserved.h"

static uint8_t ldg_math_phys_conserved_zero_is(const ldg_uint256_t *magnitude)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!magnitude)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < 4; idx++) { if (magnitude->limb[idx] != 0) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_math_phys_conserved_add(const ldg_math_phys_conserved_t *lhs, const ldg_math_phys_conserved_t *rhs, ldg_math_phys_conserved_t *out)
{
    ldg_math_phys_conserved_t zero_val = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;
    uint32_t ord = 0;
    if (LDG_UNLIKELY(!lhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = zero_val;
    if (lhs->sign == rhs->sign)
    {
        ret = ldg_arith_uint256_add(&lhs->magnitude, &rhs->magnitude, &out->magnitude);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ldg_math_phys_conserved_zero_is(&out->magnitude) != LDG_TRUTH_TRUE) { out->sign = lhs->sign; }

        return LDG_ERR_AOK;
    }

    ret = ldg_arith_uint256_cmp(&lhs->magnitude, &rhs->magnitude, &ord);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ord == LDG_ARITH_ORD_EQ) { return LDG_ERR_AOK; }

    if (ord == LDG_ARITH_ORD_GT)
    {
        ret = ldg_arith_uint256_sub(&lhs->magnitude, &rhs->magnitude, &out->magnitude);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        out->sign = lhs->sign;
        return LDG_ERR_AOK;
    }

    ret = ldg_arith_uint256_sub(&rhs->magnitude, &lhs->magnitude, &out->magnitude);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->sign = rhs->sign;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_conserved_sub(const ldg_math_phys_conserved_t *lhs, const ldg_math_phys_conserved_t *rhs, ldg_math_phys_conserved_t *out)
{
    ldg_math_phys_conserved_t neg_rhs = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;
    if (LDG_UNLIKELY(!lhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_phys_conserved_neg(rhs, &neg_rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_phys_conserved_add(lhs, &neg_rhs, out);
}

uint32_t ldg_math_phys_conserved_neg(const ldg_math_phys_conserved_t *val, ldg_math_phys_conserved_t *out)
{
    ldg_math_phys_conserved_t zero_val = LDG_STRUCT_ZERO_INIT;
    if (LDG_UNLIKELY(!val)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = zero_val;
    out->magnitude = val->magnitude;
    if (ldg_math_phys_conserved_zero_is(&val->magnitude) != LDG_TRUTH_TRUE) { out->sign = (uint8_t)(val->sign == 0); }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_conserved_cmp(const ldg_math_phys_conserved_t *lhs, const ldg_math_phys_conserved_t *rhs, uint32_t *out)
{
    uint32_t ret = 0;
    uint32_t ord = 0;
    if (LDG_UNLIKELY(!lhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (ldg_math_phys_conserved_zero_is(&lhs->magnitude) == LDG_TRUTH_TRUE && ldg_math_phys_conserved_zero_is(&rhs->magnitude) == LDG_TRUTH_TRUE)
    {
        *out = LDG_ARITH_ORD_EQ;
        return LDG_ERR_AOK;
    }

    if (lhs->sign != rhs->sign)
    {
        *out = lhs->sign ? LDG_ARITH_ORD_LT : LDG_ARITH_ORD_GT;
        return LDG_ERR_AOK;
    }

    ret = ldg_arith_uint256_cmp(&lhs->magnitude, &rhs->magnitude, &ord);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (lhs->sign) { if (ord == LDG_ARITH_ORD_LT) { ord = LDG_ARITH_ORD_GT; }
        else if (ord == LDG_ARITH_ORD_GT) { ord = LDG_ARITH_ORD_LT; } }

    *out = ord;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_conserved_rescale(const ldg_math_phys_conserved_t *val, int64_t delta, ldg_math_phys_conserved_t *out)
{
    ldg_math_phys_conserved_t zero_val = LDG_STRUCT_ZERO_INIT;
    ldg_uint256_t back = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;
    uint32_t ord = 0;
    uint32_t shift = 0;
    if (LDG_UNLIKELY(!val)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = zero_val;
    if (delta == 0)
    {
        *out = *val;
        return LDG_ERR_AOK;
    }

    if (delta > 0)
    {
        shift = (delta > 256) ? 257 : (uint32_t)delta;
        ret = ldg_arith_uint256_shr(&val->magnitude, shift, &out->magnitude);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_arith_uint256_shl(&out->magnitude, shift, &back);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_arith_uint256_cmp(&back, &val->magnitude, &ord);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ord != LDG_ARITH_ORD_EQ)
        {
            *out = zero_val;
            return LDG_ERR_MATH_PHYS_CONSERVATION;
        }
    }
    else
    {
        if ((delta + 256) < 0) { shift = 257; }
        else { shift = (uint32_t)(0 - delta); }

        ret = ldg_arith_uint256_shl(&val->magnitude, shift, &out->magnitude);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_arith_uint256_shr(&out->magnitude, shift, &back);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_arith_uint256_cmp(&back, &val->magnitude, &ord);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ord != LDG_ARITH_ORD_EQ)
        {
            *out = zero_val;
            return LDG_ERR_OVERFLOW;
        }
    }

    if (ldg_math_phys_conserved_zero_is(&out->magnitude) != LDG_TRUTH_TRUE) { out->sign = val->sign; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_double_to_conserved_convert(double val, int64_t exponent, ldg_math_phys_conserved_t *out)
{
    ldg_math_phys_conserved_t zero_val = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t hi = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t lo = LDG_STRUCT_ZERO_INIT;
    double magnitude_val = 0.0;
    double scaled = 0.0;
    double split_scale = 0.0;
    double hi_val = 0.0;
    double lo_val = 0.0;
    uint32_t ret = 0;
    uint8_t sign = 0;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = zero_val;

    split_scale = ldg_math_pwr2_scale_get(1.0, LDG_MATH_PHYS_CONSERVED_SPLIT_EXP);
    if (LDG_UNLIKELY(ldg_math_inf_is(split_scale) == LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(ldg_math_nan_is(val) == LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(exponent > LDG_MATH_PHYS_CONSERVED_EXPONENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(exponent < (0 - LDG_MATH_PHYS_CONSERVED_EXPONENT_MAX))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (val < 0.0)
    {
        sign = 1;
        magnitude_val = 0.0 - val;
    }
    else { magnitude_val = val; }

    scaled = ldg_math_pwr2_scale_get(magnitude_val, (int32_t)(0 - exponent));
    if (LDG_UNLIKELY(ldg_math_inf_is(scaled) == LDG_TRUTH_TRUE)) { return LDG_ERR_OVERFLOW; }

    if (scaled >= split_scale)
    {
        hi_val = ldg_math_trunc_get(ldg_math_pwr2_scale_get(scaled, LDG_MATH_PHYS_CONSERVED_SPLIT_EXP_NEG));
        ret = ldg_arith_double_to_uint128_convert(hi_val, &hi);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        lo_val = scaled - ldg_math_pwr2_scale_get(hi_val, LDG_MATH_PHYS_CONSERVED_SPLIT_EXP);
        ret = ldg_arith_double_to_uint128_convert(lo_val, &lo);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = ldg_arith_double_to_uint128_convert(scaled, &lo);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    out->magnitude.limb[0] = lo.limb[0];
    out->magnitude.limb[1] = lo.limb[1];
    out->magnitude.limb[2] = hi.limb[0];
    out->magnitude.limb[3] = hi.limb[1];
    if (ldg_math_phys_conserved_zero_is(&out->magnitude) != LDG_TRUTH_TRUE) { out->sign = sign; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_conserved_to_double_convert(const ldg_math_phys_conserved_t *val, int64_t exponent, double *out)
{
    ldg_uint128_t hi = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t lo = LDG_STRUCT_ZERO_INIT;
    double hi_val = 0.0;
    double lo_val = 0.0;
    double combined = 0.0;
    double split_scale = 0.0;
    uint32_t ret = 0;
    if (LDG_UNLIKELY(!val)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    split_scale = ldg_math_pwr2_scale_get(1.0, LDG_MATH_PHYS_CONSERVED_SPLIT_EXP);
    if (LDG_UNLIKELY(ldg_math_inf_is(split_scale) == LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(exponent > LDG_MATH_PHYS_CONSERVED_EXPONENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(exponent < (0 - LDG_MATH_PHYS_CONSERVED_EXPONENT_MAX))) { return LDG_ERR_FUNC_ARG_INVALID; }

    lo.limb[0] = val->magnitude.limb[0];
    lo.limb[1] = val->magnitude.limb[1];
    hi.limb[0] = val->magnitude.limb[2];
    hi.limb[1] = val->magnitude.limb[3];
    ret = ldg_arith_uint128_to_double_convert(&hi, &hi_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_arith_uint128_to_double_convert(&lo, &lo_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    combined = hi_val * split_scale + lo_val;
    combined = ldg_math_pwr2_scale_get(combined, (int32_t)exponent);
    if (LDG_UNLIKELY(ldg_math_inf_is(combined))) { return LDG_ERR_OVERFLOW; }

    if (val->sign) { combined = 0.0 - combined; }

    *out = combined;
    return LDG_ERR_AOK;
}
