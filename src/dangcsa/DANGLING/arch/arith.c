#include <stdint.h>

#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_ARITH_LIMB_BITS 64
#define LDG_ARITH_HALF_BITS 32
#define LDG_ARITH_HALF_MASK 0xFFFFFFFFULL
#define LDG_ARITH_LIMB_CUNT_MAX 4
#define LDG_ARITH_UINT128_LIMB_CUNT 2
#define LDG_ARITH_UINT256_LIMB_CUNT 4
#define LDG_ARITH_UINT128_BIT_CUNT 128
#define LDG_ARITH_STEP 1

#define LDG_ARITH_FP64_MANT_BITS 52
#define LDG_ARITH_FP64_EXP_BIAS 1023
#define LDG_ARITH_FP64_EXP_MASK 0x7FFULL
#define LDG_ARITH_FP64_MANT_MASK 0xFFFFFFFFFFFFFULL
#define LDG_ARITH_FP64_HIDDEN 0x10000000000000ULL
#define LDG_ARITH_FP64_SIGN_SHIFT 63

typedef union ldg_arith_fp64_bits
{
    double val;
    uint64_t bits;
} ldg_arith_fp64_bits_t;

static uint64_t limb_carry_add(uint64_t lhs, uint64_t rhs, uint64_t carry_in, uint64_t *carry_out)
{
    uint64_t sum = 0;

    sum = lhs + rhs;
    *carry_out = (sum < lhs) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    sum += carry_in;
    if (sum < carry_in) { *carry_out = LDG_TRUTH_TRUE; }

    return sum;
}

static uint64_t limb_borrow_sub(uint64_t lhs, uint64_t rhs, uint64_t borrow_in, uint64_t *borrow_out)
{
    uint64_t diff = 0;

    diff = lhs - rhs;
    *borrow_out = (lhs < rhs) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (diff < borrow_in) { *borrow_out = LDG_TRUTH_TRUE; }

    diff -= borrow_in;

    return diff;
}

static void limb_wide_mul(uint64_t lhs, uint64_t rhs, uint64_t *lo, uint64_t *hi)
{
    uint64_t lhs_lo = lhs & LDG_ARITH_HALF_MASK;
    uint64_t lhs_hi = lhs >> LDG_ARITH_HALF_BITS;
    uint64_t rhs_lo = rhs & LDG_ARITH_HALF_MASK;
    uint64_t rhs_hi = rhs >> LDG_ARITH_HALF_BITS;
    uint64_t part_ll = lhs_lo * rhs_lo;
    uint64_t part_lh = lhs_lo * rhs_hi;
    uint64_t part_hl = lhs_hi * rhs_lo;
    uint64_t part_hh = lhs_hi * rhs_hi;
    uint64_t mid = 0;
    uint64_t mid_carry = 0;

    mid = (part_ll >> LDG_ARITH_HALF_BITS) + (part_lh & LDG_ARITH_HALF_MASK) + (part_hl & LDG_ARITH_HALF_MASK);
    mid_carry = mid >> LDG_ARITH_HALF_BITS;

    *lo = (part_ll & LDG_ARITH_HALF_MASK) | (mid << LDG_ARITH_HALF_BITS);
    *hi = part_hh + (part_lh >> LDG_ARITH_HALF_BITS) + (part_hl >> LDG_ARITH_HALF_BITS) + mid_carry;
}

static uint64_t wide_add(const uint64_t *lhs, const uint64_t *rhs, uint64_t *out, uint32_t cunt)
{
    uint64_t carry = 0;
    uint64_t carry_out = 0;
    uint32_t idx = 0;

    for (idx = 0; idx < cunt; idx++)
    {
        out[idx] = limb_carry_add(lhs[idx], rhs[idx], carry, &carry_out);
        carry = carry_out;
    }

    return carry;
}

static uint64_t wide_sub(const uint64_t *lhs, const uint64_t *rhs, uint64_t *out, uint32_t cunt)
{
    uint64_t borrow = 0;
    uint64_t borrow_out = 0;
    uint32_t idx = 0;

    for (idx = 0; idx < cunt; idx++)
    {
        out[idx] = limb_borrow_sub(lhs[idx], rhs[idx], borrow, &borrow_out);
        borrow = borrow_out;
    }

    return borrow;
}

static uint32_t wide_cmp(const uint64_t *lhs, const uint64_t *rhs, uint32_t cunt)
{
    uint32_t idx = 0;

    idx = cunt;
    while (idx > 0)
    {
        idx--;

        if (lhs[idx] < rhs[idx]) { return LDG_ARITH_ORD_LT; }

        if (lhs[idx] > rhs[idx]) { return LDG_ARITH_ORD_GT; }
    }

    return LDG_ARITH_ORD_EQ;
}

static void wide_zero(uint64_t *val, uint32_t cunt)
{
    uint32_t idx = 0;

    for (idx = 0; idx < cunt; idx++) { val[idx] = 0; }
}

static void wide_copy(const uint64_t *src, uint64_t *dst, uint32_t cunt)
{
    uint32_t idx = 0;

    for (idx = 0; idx < cunt; idx++) { dst[idx] = src[idx]; }
}

static uint8_t wide_zero_is(const uint64_t *val, uint32_t cunt)
{
    uint32_t idx = 0;

    for (idx = 0; idx < cunt; idx++) { if (val[idx] != 0) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static uint8_t wide_bit_get(const uint64_t *val, uint32_t bit)
{
    uint32_t limb = bit / LDG_ARITH_LIMB_BITS;
    uint32_t off = bit % LDG_ARITH_LIMB_BITS;

    return (uint8_t)((val[limb] >> off) & LDG_ARITH_STEP);
}

static void wide_bit_set(uint64_t *val, uint32_t bit)
{
    uint32_t limb = bit / LDG_ARITH_LIMB_BITS;
    uint32_t off = bit % LDG_ARITH_LIMB_BITS;

    val[limb] |= ((uint64_t)LDG_ARITH_STEP << off);
}

static void wide_shl(const uint64_t *val, uint32_t amt, uint64_t *out, uint32_t cunt)
{
    uint32_t limb_shift = 0;
    uint32_t bit_shift = 0;
    uint32_t idx = 0;
    uint32_t src = 0;

    if (amt >= cunt * LDG_ARITH_LIMB_BITS)
    {
        wide_zero(out, cunt);
        return;
    }

    limb_shift = amt / LDG_ARITH_LIMB_BITS;
    bit_shift = amt % LDG_ARITH_LIMB_BITS;

    idx = cunt;
    while (idx > 0)
    {
        idx--;

        if (idx < limb_shift)
        {
            out[idx] = 0;
            continue;
        }

        src = idx - limb_shift;
        out[idx] = val[src] << bit_shift;

        if (bit_shift > 0 && src > 0) { out[idx] |= val[src - LDG_ARITH_STEP] >> (LDG_ARITH_LIMB_BITS - bit_shift); }
    }
}

static void wide_shr(const uint64_t *val, uint32_t amt, uint64_t *out, uint32_t cunt)
{
    uint32_t limb_shift = 0;
    uint32_t bit_shift = 0;
    uint32_t idx = 0;
    uint32_t src = 0;

    if (amt >= cunt * LDG_ARITH_LIMB_BITS)
    {
        wide_zero(out, cunt);
        return;
    }

    limb_shift = amt / LDG_ARITH_LIMB_BITS;
    bit_shift = amt % LDG_ARITH_LIMB_BITS;

    for (idx = 0; idx < cunt; idx++)
    {
        src = idx + limb_shift;

        if (src >= cunt)
        {
            out[idx] = 0;
            continue;
        }

        out[idx] = val[src] >> bit_shift;

        if (bit_shift > 0 && (src + LDG_ARITH_STEP) < cunt) { out[idx] |= val[src + LDG_ARITH_STEP] << (LDG_ARITH_LIMB_BITS - bit_shift); }
    }
}

static uint8_t wide_mul(const uint64_t *lhs, const uint64_t *rhs, uint64_t *out, uint32_t cunt)
{
    uint64_t acc[LDG_ARITH_LIMB_CUNT_MAX * 2] = LDG_ARR_ZERO_INIT;
    uint64_t lo = 0;
    uint64_t hi = 0;
    uint64_t carry = 0;
    uint64_t carry_out = 0;
    uint32_t outer = 0;
    uint32_t inner = 0;
    uint32_t idx = 0;

    for (outer = 0; outer < cunt; outer++)
    {
        carry = LDG_TRUTH_FALSE;

        for (inner = 0; inner < cunt; inner++)
        {
            limb_wide_mul(lhs[outer], rhs[inner], &lo, &hi);

            acc[outer + inner] = limb_carry_add(acc[outer + inner], lo, carry, &carry_out);
            carry = hi + carry_out;
        }

        idx = outer + cunt;
        while (carry != 0 && idx < (cunt * 2))
        {
            acc[idx] = limb_carry_add(acc[idx], carry, LDG_TRUTH_FALSE, &carry_out);
            carry = carry_out;
            idx++;
        }
    }

    wide_copy(acc, out, cunt);

    for (idx = cunt; idx < (cunt * 2); idx++) { if (acc[idx] != 0) { return LDG_TRUTH_TRUE; } }

    return LDG_TRUTH_FALSE;
}

static uint32_t wide_div_compute(const uint64_t *num, const uint64_t *den, uint64_t *quot, uint64_t *rem, uint32_t cunt)
{
    uint64_t work[LDG_ARITH_LIMB_CUNT_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t shifted[LDG_ARITH_LIMB_CUNT_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t bit = 0;

    if (LDG_UNLIKELY(wide_zero_is(den, cunt) == LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    wide_zero(quot, cunt);
    wide_zero(work, cunt);

    bit = cunt * LDG_ARITH_LIMB_BITS;
    while (bit > 0)
    {
        bit--;

        wide_shl(work, LDG_ARITH_STEP, shifted, cunt);
        wide_copy(shifted, work, cunt);

        if (wide_bit_get(num, bit) == LDG_TRUTH_TRUE) { work[0] |= (uint64_t)LDG_ARITH_STEP; }

        if (wide_cmp(work, den, cunt) != LDG_ARITH_ORD_LT)
        {
            wide_sub(work, den, shifted, cunt);
            wide_copy(shifted, work, cunt);
            wide_bit_set(quot, bit);
        }
    }

    wide_copy(work, rem, cunt);

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint32_add(uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(lhs > UINT32_MAX - rhs)) { return LDG_ERR_OVERFLOW; }

    *out = lhs + rhs;

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint32_sub(uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(lhs < rhs)) { return LDG_ERR_OVERFLOW; }

    *out = lhs - rhs;

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint32_mul(uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (rhs != 0 && LDG_UNLIKELY(lhs > UINT32_MAX / rhs)) { return LDG_ERR_OVERFLOW; }

    *out = lhs * rhs;

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint32_div(uint32_t lhs, uint32_t rhs, uint32_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(rhs == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = lhs / rhs;

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint64_add(uint64_t lhs, uint64_t rhs, uint64_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(lhs > UINT64_MAX - rhs)) { return LDG_ERR_OVERFLOW; }

    *out = lhs + rhs;

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint64_sub(uint64_t lhs, uint64_t rhs, uint64_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(lhs < rhs)) { return LDG_ERR_OVERFLOW; }

    *out = lhs - rhs;

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint64_mul(uint64_t lhs, uint64_t rhs, uint64_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (rhs != 0 && LDG_UNLIKELY(lhs > UINT64_MAX / rhs)) { return LDG_ERR_OVERFLOW; }

    *out = lhs * rhs;

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint64_div(uint64_t lhs, uint64_t rhs, uint64_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(rhs == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = lhs / rhs;

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint128_add(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, ldg_uint128_t *out)
{
    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(wide_add(lhs->limb, rhs->limb, out->limb, LDG_ARITH_UINT128_LIMB_CUNT) != 0)) { return LDG_ERR_OVERFLOW; }

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint128_sub(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, ldg_uint128_t *out)
{
    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(wide_sub(lhs->limb, rhs->limb, out->limb, LDG_ARITH_UINT128_LIMB_CUNT) != 0)) { return LDG_ERR_OVERFLOW; }

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint128_mul(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, ldg_uint128_t *out)
{
    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(wide_mul(lhs->limb, rhs->limb, out->limb, LDG_ARITH_UINT128_LIMB_CUNT) == LDG_TRUTH_TRUE)) { return LDG_ERR_OVERFLOW; }

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint128_div(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, ldg_uint128_t *out)
{
    ldg_uint128_t rem = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    return wide_div_compute(lhs->limb, rhs->limb, out->limb, rem.limb, LDG_ARITH_UINT128_LIMB_CUNT);
}

uint32_t ldg_arith_uint256_add(const ldg_uint256_t *lhs, const ldg_uint256_t *rhs, ldg_uint256_t *out)
{
    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(wide_add(lhs->limb, rhs->limb, out->limb, LDG_ARITH_UINT256_LIMB_CUNT) != 0)) { return LDG_ERR_OVERFLOW; }

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint256_sub(const ldg_uint256_t *lhs, const ldg_uint256_t *rhs, ldg_uint256_t *out)
{
    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(wide_sub(lhs->limb, rhs->limb, out->limb, LDG_ARITH_UINT256_LIMB_CUNT) != 0)) { return LDG_ERR_OVERFLOW; }

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint256_mul(const ldg_uint256_t *lhs, const ldg_uint256_t *rhs, ldg_uint256_t *out)
{
    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(wide_mul(lhs->limb, rhs->limb, out->limb, LDG_ARITH_UINT256_LIMB_CUNT) == LDG_TRUTH_TRUE)) { return LDG_ERR_OVERFLOW; }

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint256_div(const ldg_uint256_t *lhs, const ldg_uint256_t *rhs, ldg_uint256_t *out)
{
    ldg_uint256_t rem = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    return wide_div_compute(lhs->limb, rhs->limb, out->limb, rem.limb, LDG_ARITH_UINT256_LIMB_CUNT);
}

uint32_t ldg_arith_uint128_cmp(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, uint32_t *out)
{
    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = wide_cmp(lhs->limb, rhs->limb, LDG_ARITH_UINT128_LIMB_CUNT);

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint256_cmp(const ldg_uint256_t *lhs, const ldg_uint256_t *rhs, uint32_t *out)
{
    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = wide_cmp(lhs->limb, rhs->limb, LDG_ARITH_UINT256_LIMB_CUNT);

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint128_to_uint256_mul(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, ldg_uint256_t *out)
{
    ldg_uint256_t wide_lhs = LDG_STRUCT_ZERO_INIT;
    ldg_uint256_t wide_rhs = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    wide_copy(lhs->limb, wide_lhs.limb, LDG_ARITH_UINT128_LIMB_CUNT);
    wide_copy(rhs->limb, wide_rhs.limb, LDG_ARITH_UINT128_LIMB_CUNT);

    wide_mul(wide_lhs.limb, wide_rhs.limb, out->limb, LDG_ARITH_UINT256_LIMB_CUNT);

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint128_div_compute(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, ldg_uint128_t *quot, ldg_uint128_t *rem)
{
    if (LDG_UNLIKELY(!lhs || !rhs || !quot || !rem)) { return LDG_ERR_FUNC_ARG_NULL; }

    return wide_div_compute(lhs->limb, rhs->limb, quot->limb, rem->limb, LDG_ARITH_UINT128_LIMB_CUNT);
}

uint32_t ldg_arith_uint256_div_compute(const ldg_uint256_t *lhs, const ldg_uint256_t *rhs, ldg_uint256_t *quot, ldg_uint256_t *rem)
{
    if (LDG_UNLIKELY(!lhs || !rhs || !quot || !rem)) { return LDG_ERR_FUNC_ARG_NULL; }

    return wide_div_compute(lhs->limb, rhs->limb, quot->limb, rem->limb, LDG_ARITH_UINT256_LIMB_CUNT);
}

uint32_t ldg_arith_uint128_shl(const ldg_uint128_t *val, uint32_t amt, ldg_uint128_t *out)
{
    if (LDG_UNLIKELY(!val || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    wide_shl(val->limb, amt, out->limb, LDG_ARITH_UINT128_LIMB_CUNT);

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint128_shr(const ldg_uint128_t *val, uint32_t amt, ldg_uint128_t *out)
{
    if (LDG_UNLIKELY(!val || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    wide_shr(val->limb, amt, out->limb, LDG_ARITH_UINT128_LIMB_CUNT);

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint256_shl(const ldg_uint256_t *val, uint32_t amt, ldg_uint256_t *out)
{
    if (LDG_UNLIKELY(!val || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    wide_shl(val->limb, amt, out->limb, LDG_ARITH_UINT256_LIMB_CUNT);

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint256_shr(const ldg_uint256_t *val, uint32_t amt, ldg_uint256_t *out)
{
    if (LDG_UNLIKELY(!val || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    wide_shr(val->limb, amt, out->limb, LDG_ARITH_UINT256_LIMB_CUNT);

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_uint128_to_double_convert(const ldg_uint128_t *val, double *out)
{
    uint64_t top = 0;
    uint64_t sticky = 0;
    uint32_t bit_len = 0;
    uint32_t shift = 0;
    uint32_t idx = 0;
    double scaled = 0.0;

    if (LDG_UNLIKELY(!val || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    bit_len = LDG_ARITH_UINT128_BIT_CUNT;
    while (bit_len > 0)
    {
        if (wide_bit_get(val->limb, bit_len - LDG_ARITH_STEP) == LDG_TRUTH_TRUE) { break; }

        bit_len--;
    }

    if (bit_len == 0) { return LDG_ERR_AOK; }

    if (bit_len <= LDG_ARITH_LIMB_BITS)
    {
        *out = (double)val->limb[0];

        return LDG_ERR_AOK;
    }

    shift = bit_len - LDG_ARITH_LIMB_BITS;

    for (idx = 0; idx < shift; idx++) { if (wide_bit_get(val->limb, idx) == LDG_TRUTH_TRUE)
        {
            sticky = LDG_ARITH_STEP;
            break;
        }
    }

    top = (val->limb[1] << (LDG_ARITH_LIMB_BITS - shift)) | (val->limb[0] >> shift);
    top |= sticky;

    scaled = (double)top;
    for (idx = 0; idx < shift; idx++) { scaled *= 2.0; }

    *out = scaled;

    return LDG_ERR_AOK;
}

uint32_t ldg_arith_double_to_uint128_convert(double val, ldg_uint128_t *out)
{
    ldg_arith_fp64_bits_t split = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t mant = LDG_STRUCT_ZERO_INIT;
    uint64_t raw = 0;
    uint64_t frac = 0;
    int32_t exponent = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->limb[0] = 0;
    out->limb[1] = 0;

    split.val = val;
    raw = split.bits;

    if (LDG_UNLIKELY((raw >> LDG_ARITH_FP64_SIGN_SHIFT) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    exponent = (int32_t)((raw >> LDG_ARITH_FP64_MANT_BITS) & LDG_ARITH_FP64_EXP_MASK);
    frac = raw & LDG_ARITH_FP64_MANT_MASK;

    if (LDG_UNLIKELY(exponent == (int32_t)LDG_ARITH_FP64_EXP_MASK)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (exponent == 0) { return LDG_ERR_AOK; }

    exponent -= LDG_ARITH_FP64_EXP_BIAS;

    if (exponent < 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(exponent >= (int32_t)LDG_ARITH_UINT128_BIT_CUNT)) { return LDG_ERR_OVERFLOW; }

    mant.limb[0] = frac | LDG_ARITH_FP64_HIDDEN;

    if (exponent >= (int32_t)LDG_ARITH_FP64_MANT_BITS) { wide_shl(mant.limb, (uint32_t)(exponent - (int32_t)LDG_ARITH_FP64_MANT_BITS), out->limb, LDG_ARITH_UINT128_LIMB_CUNT); }
    else { wide_shr(mant.limb, (uint32_t)((int32_t)LDG_ARITH_FP64_MANT_BITS - exponent), out->limb, LDG_ARITH_UINT128_LIMB_CUNT); }

    return LDG_ERR_AOK;
}
