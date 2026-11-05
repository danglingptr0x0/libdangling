#include <stdint.h>

#include <dangling/math/rational.h>
#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_MATH_RATIONAL_LIMB_HI_IDX 1
#define LDG_MATH_RATIONAL_SIGN_STEP 1

typedef struct ldg_math_rational_wide
{
    ldg_uint128_t mag;
    ldg_uint128_t den;
    uint8_t neg;
    uint8_t pudding[7];
} ldg_math_rational_wide_t;

static uint8_t rational_uint128_zero_is(const ldg_uint128_t *val)
{
    if (val->limb[0] == 0 && val->limb[LDG_MATH_RATIONAL_LIMB_HI_IDX] == 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint64_t rational_mag_get(int64_t num)
{
    if (num >= 0) { return (uint64_t)num; }

    return (uint64_t)(-(num + LDG_MATH_RATIONAL_SIGN_STEP)) + LDG_MATH_RATIONAL_SIGN_STEP;
}

static uint8_t rational_neg_is(int64_t num)
{
    if (num < 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t rational_valid_ck(const ldg_math_rational_t *val)
{
    if (LDG_UNLIKELY(!val)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(val->den == 0)) { return LDG_ERR_MATH_VAL_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t rational_uint128_mul(uint64_t lhs, uint64_t rhs, ldg_uint128_t *out)
{
    ldg_uint128_t lhs_wide = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t rhs_wide = LDG_STRUCT_ZERO_INIT;

    lhs_wide.limb[0] = lhs;
    rhs_wide.limb[0] = rhs;

    return ldg_arith_uint128_mul(&lhs_wide, &rhs_wide, out);
}

static uint32_t rational_gcd_get(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, ldg_uint128_t *out)
{
    ldg_uint128_t curr = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t next = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t quot = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t rem = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    curr = *lhs;
    next = *rhs;

    while (rational_uint128_zero_is(&next) != LDG_TRUTH_TRUE)
    {
        ret = ldg_arith_uint128_div_compute(&curr, &next, &quot, &rem);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        curr = next;
        next = rem;
    }

    *out = curr;

    return LDG_ERR_AOK;
}

static uint32_t rational_finish(const ldg_math_rational_wide_t *wide, ldg_math_rational_t *out)
{
    ldg_uint128_t gcd = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t mag = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t den = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t rem = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (rational_uint128_zero_is(&wide->mag) == LDG_TRUTH_TRUE)
    {
        out->num = (int64_t)wide->mag.limb[0];
        out->den = LDG_MATH_RATIONAL_WHOLE_DEN;

        return LDG_ERR_AOK;
    }

    ret = rational_gcd_get(&wide->mag, &wide->den, &gcd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_arith_uint128_div_compute(&wide->mag, &gcd, &mag, &rem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_arith_uint128_div_compute(&wide->den, &gcd, &den, &rem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(mag.limb[LDG_MATH_RATIONAL_LIMB_HI_IDX] != 0 || den.limb[LDG_MATH_RATIONAL_LIMB_HI_IDX] != 0)) { return LDG_ERR_RANGE; }

    if (wide->neg != LDG_TRUTH_TRUE)
    {
        if (LDG_UNLIKELY(mag.limb[0] > (uint64_t)INT64_MAX)) { return LDG_ERR_RANGE; }

        out->num = (int64_t)mag.limb[0];
        out->den = den.limb[0];

        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY(mag.limb[0] > (uint64_t)INT64_MAX + LDG_MATH_RATIONAL_SIGN_STEP)) { return LDG_ERR_RANGE; }

    out->num = -(int64_t)(mag.limb[0] - LDG_MATH_RATIONAL_SIGN_STEP) - LDG_MATH_RATIONAL_SIGN_STEP;
    out->den = den.limb[0];

    return LDG_ERR_AOK;
}

static uint32_t rational_sum(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, uint8_t rhs_flip, ldg_math_rational_t *out)
{
    ldg_math_rational_wide_t wide = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t lhs_cross = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t rhs_cross = LDG_STRUCT_ZERO_INIT;
    uint32_t ord = 0;
    uint32_t ret = 0;
    uint8_t lhs_neg = LDG_TRUTH_FALSE;
    uint8_t rhs_neg = LDG_TRUTH_FALSE;

    lhs_neg = rational_neg_is(lhs->num);
    rhs_neg = rational_neg_is(rhs->num);

    if (rhs_flip == LDG_TRUTH_TRUE && rhs->num != 0) { rhs_neg = (rhs_neg == LDG_TRUTH_TRUE) ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE; }

    ret = rational_uint128_mul(rational_mag_get(lhs->num), rhs->den, &lhs_cross);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = rational_uint128_mul(rational_mag_get(rhs->num), lhs->den, &rhs_cross);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = rational_uint128_mul(lhs->den, rhs->den, &wide.den);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (lhs_neg == rhs_neg)
    {
        ret = ldg_arith_uint128_add(&lhs_cross, &rhs_cross, &wide.mag);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        wide.neg = lhs_neg;

        return rational_finish(&wide, out);
    }

    ret = ldg_arith_uint128_cmp(&lhs_cross, &rhs_cross, &ord);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ord != LDG_ARITH_ORD_LT)
    {
        ret = ldg_arith_uint128_sub(&lhs_cross, &rhs_cross, &wide.mag);
        wide.neg = lhs_neg;
    }
    else
    {
        ret = ldg_arith_uint128_sub(&rhs_cross, &lhs_cross, &wide.mag);
        wide.neg = rhs_neg;
    }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return rational_finish(&wide, out);
}

uint32_t ldg_math_rational_set(int64_t num, uint64_t den, ldg_math_rational_t *out)
{
    ldg_math_rational_wide_t wide = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(den == 0)) { return LDG_ERR_MATH_VAL_INVALID; }

    wide.mag.limb[0] = rational_mag_get(num);
    wide.den.limb[0] = den;
    wide.neg = rational_neg_is(num);

    return rational_finish(&wide, out);
}

uint32_t ldg_math_rational_add(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, ldg_math_rational_t *out)
{
    ldg_math_rational_t lhs_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_rational_t rhs_val = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = rational_valid_ck(lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = rational_valid_ck(rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_val = *lhs;
    rhs_val = *rhs;

    return rational_sum(&lhs_val, &rhs_val, LDG_TRUTH_FALSE, out);
}

uint32_t ldg_math_rational_sub(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, ldg_math_rational_t *out)
{
    ldg_math_rational_t lhs_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_rational_t rhs_val = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = rational_valid_ck(lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = rational_valid_ck(rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_val = *lhs;
    rhs_val = *rhs;

    return rational_sum(&lhs_val, &rhs_val, LDG_TRUTH_TRUE, out);
}

uint32_t ldg_math_rational_mul(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, ldg_math_rational_t *out)
{
    ldg_math_rational_wide_t wide = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = rational_valid_ck(lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = rational_valid_ck(rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = rational_uint128_mul(rational_mag_get(lhs->num), rational_mag_get(rhs->num), &wide.mag);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = rational_uint128_mul(lhs->den, rhs->den, &wide.den);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    wide.neg = (rational_neg_is(lhs->num) != rational_neg_is(rhs->num)) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return rational_finish(&wide, out);
}

uint32_t ldg_math_rational_div(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, ldg_math_rational_t *out)
{
    ldg_math_rational_wide_t wide = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = rational_valid_ck(lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = rational_valid_ck(rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(rhs->num == 0)) { return LDG_ERR_MATH_DIV_ZERO; }

    ret = rational_uint128_mul(rational_mag_get(lhs->num), rhs->den, &wide.mag);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = rational_uint128_mul(lhs->den, rational_mag_get(rhs->num), &wide.den);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    wide.neg = (rational_neg_is(lhs->num) != rational_neg_is(rhs->num)) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return rational_finish(&wide, out);
}

uint32_t ldg_math_rational_neg(const ldg_math_rational_t *val, ldg_math_rational_t *out)
{
    ldg_math_rational_wide_t wide = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = rational_valid_ck(val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    wide.mag.limb[0] = rational_mag_get(val->num);
    wide.den.limb[0] = val->den;
    wide.neg = (val->num > 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return rational_finish(&wide, out);
}

uint32_t ldg_math_rational_abs(const ldg_math_rational_t *val, ldg_math_rational_t *out)
{
    ldg_math_rational_wide_t wide = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = rational_valid_ck(val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    wide.mag.limb[0] = rational_mag_get(val->num);
    wide.den.limb[0] = val->den;
    wide.neg = LDG_TRUTH_FALSE;

    return rational_finish(&wide, out);
}

uint32_t ldg_math_rational_cmp(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, uint32_t *out)
{
    ldg_uint128_t lhs_cross = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t rhs_cross = LDG_STRUCT_ZERO_INIT;
    uint32_t ord = 0;
    uint32_t ret = 0;
    uint8_t lhs_neg = LDG_TRUTH_FALSE;
    uint8_t rhs_neg = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_ARITH_ORD_EQ;

    ret = rational_valid_ck(lhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = rational_valid_ck(rhs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    lhs_neg = rational_neg_is(lhs->num);
    rhs_neg = rational_neg_is(rhs->num);

    if (lhs_neg == LDG_TRUTH_TRUE && rhs_neg != LDG_TRUTH_TRUE)
    {
        *out = LDG_ARITH_ORD_LT;
        return LDG_ERR_AOK;
    }

    if (lhs_neg != LDG_TRUTH_TRUE && rhs_neg == LDG_TRUTH_TRUE)
    {
        *out = LDG_ARITH_ORD_GT;
        return LDG_ERR_AOK;
    }

    ret = rational_uint128_mul(rational_mag_get(lhs->num), rhs->den, &lhs_cross);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = rational_uint128_mul(rational_mag_get(rhs->num), lhs->den, &rhs_cross);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_arith_uint128_cmp(&lhs_cross, &rhs_cross, &ord);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (lhs_neg == LDG_TRUTH_TRUE && ord == LDG_ARITH_ORD_LT) { ord = LDG_ARITH_ORD_GT; }
    else if (lhs_neg == LDG_TRUTH_TRUE && ord == LDG_ARITH_ORD_GT) { ord = LDG_ARITH_ORD_LT; }

    *out = ord;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_rational_min(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, ldg_math_rational_t *out)
{
    uint32_t ord = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_rational_cmp(lhs, rhs, &ord);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ord == LDG_ARITH_ORD_GT) { return ldg_math_rational_set(rhs->num, rhs->den, out); }

    return ldg_math_rational_set(lhs->num, lhs->den, out);
}

uint32_t ldg_math_rational_max(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, ldg_math_rational_t *out)
{
    uint32_t ord = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_rational_cmp(lhs, rhs, &ord);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ord == LDG_ARITH_ORD_LT) { return ldg_math_rational_set(rhs->num, rhs->den, out); }

    return ldg_math_rational_set(lhs->num, lhs->den, out);
}

uint32_t ldg_math_rational_uint64_mul(const ldg_math_rational_t *val, uint64_t factor, ldg_math_rational_t *out)
{
    ldg_math_rational_wide_t wide = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = rational_valid_ck(val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = rational_uint128_mul(rational_mag_get(val->num), factor, &wide.mag);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    wide.den.limb[0] = val->den;
    wide.neg = rational_neg_is(val->num);

    return rational_finish(&wide, out);
}

uint32_t ldg_math_rational_floor_get(const ldg_math_rational_t *val, int64_t *out)
{
    uint64_t mag = 0;
    uint64_t quot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = rational_valid_ck(val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mag = rational_mag_get(val->num);
    quot = mag / val->den;

    if (rational_neg_is(val->num) != LDG_TRUTH_TRUE)
    {
        *out = (int64_t)quot;
        return LDG_ERR_AOK;
    }

    if (mag % val->den != 0) { quot += LDG_MATH_RATIONAL_SIGN_STEP; }

    *out = -(int64_t)(quot - LDG_MATH_RATIONAL_SIGN_STEP) - LDG_MATH_RATIONAL_SIGN_STEP;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_rational_ceil_get(const ldg_math_rational_t *val, int64_t *out)
{
    uint64_t mag = 0;
    uint64_t quot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = rational_valid_ck(val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mag = rational_mag_get(val->num);
    quot = mag / val->den;

    if (rational_neg_is(val->num) == LDG_TRUTH_TRUE)
    {
        if (quot == 0)
        {
            *out = (int64_t)quot;
            return LDG_ERR_AOK;
        }

        *out = -(int64_t)(quot - LDG_MATH_RATIONAL_SIGN_STEP) - LDG_MATH_RATIONAL_SIGN_STEP;
        return LDG_ERR_AOK;
    }

    if (mag % val->den != 0) { quot += LDG_MATH_RATIONAL_SIGN_STEP; }

    *out = (int64_t)quot;

    return LDG_ERR_AOK;
}
