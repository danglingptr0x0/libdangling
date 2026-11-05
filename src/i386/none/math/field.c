#include <stdint.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/field.h>

#define LDG_MATH_FIELD_INVERT_EXP 0xFFFFFFFEFFFFFFFFULL
#define LDG_MATH_FIELD_BIT_ADVANCE 1
#define LDG_MATH_FIELD_PREV_OFF 1

static ldg_math_field_elem_t field_wide_reduce(uint64_t hi, uint64_t lo)
{
    uint64_t hi_hi = 0;
    uint64_t hi_lo = 0;
    uint64_t fold_lo = 0;
    uint64_t fold_hi = 0;

    hi_hi = hi >> LDG_MATH_FIELD_LIMB_SHIFT;
    hi_lo = hi & LDG_MATH_FIELD_LIMB_MASK;

    fold_lo = lo - hi_hi;

    if (hi_hi > lo) { fold_lo -= LDG_MATH_FIELD_OVERFLOW_FOLD; }

    fold_hi = hi_lo * LDG_MATH_FIELD_OVERFLOW_FOLD;

    return ldg_math_field_add(ldg_math_field_reduce(fold_lo), ldg_math_field_reduce(fold_hi));
}

ldg_math_field_elem_t ldg_math_field_mul(ldg_math_field_elem_t lhs, ldg_math_field_elem_t rhs)
{
    uint64_t lhs_lo = 0;
    uint64_t lhs_hi = 0;
    uint64_t rhs_lo = 0;
    uint64_t rhs_hi = 0;
    uint64_t prod_ll = 0;
    uint64_t prod_lh = 0;
    uint64_t prod_hl = 0;
    uint64_t prod_hh = 0;
    uint64_t mid = 0;
    uint64_t wide_lo = 0;
    uint64_t wide_hi = 0;

    lhs_lo = lhs & LDG_MATH_FIELD_LIMB_MASK;
    lhs_hi = lhs >> LDG_MATH_FIELD_LIMB_SHIFT;
    rhs_lo = rhs & LDG_MATH_FIELD_LIMB_MASK;
    rhs_hi = rhs >> LDG_MATH_FIELD_LIMB_SHIFT;

    prod_ll = lhs_lo * rhs_lo;
    prod_lh = lhs_lo * rhs_hi;
    prod_hl = lhs_hi * rhs_lo;
    prod_hh = lhs_hi * rhs_hi;

    mid = (prod_ll >> LDG_MATH_FIELD_LIMB_SHIFT) + (prod_lh & LDG_MATH_FIELD_LIMB_MASK) + (prod_hl & LDG_MATH_FIELD_LIMB_MASK);

    wide_lo = (prod_ll & LDG_MATH_FIELD_LIMB_MASK) | (mid << LDG_MATH_FIELD_LIMB_SHIFT);
    wide_hi = prod_hh + (prod_lh >> LDG_MATH_FIELD_LIMB_SHIFT) + (prod_hl >> LDG_MATH_FIELD_LIMB_SHIFT) + (mid >> LDG_MATH_FIELD_LIMB_SHIFT);

    return field_wide_reduce(wide_hi, wide_lo);
}

ldg_math_field_elem_t ldg_math_field_pwr_get(ldg_math_field_elem_t base, uint64_t exp)
{
    ldg_math_field_elem_t acc = LDG_MATH_FIELD_MUL_IDENT;
    ldg_math_field_elem_t curr = 0;
    uint64_t bit_walk = 0;

    curr = ldg_math_field_reduce(base);
    bit_walk = exp;

    while (bit_walk != 0)
    {
        if ((bit_walk & LDG_MATH_FIELD_LSB_MASK) != 0) { acc = ldg_math_field_mul(acc, curr); }

        curr = ldg_math_field_mul(curr, curr);
        bit_walk >>= LDG_MATH_FIELD_BIT_ADVANCE;
    }

    return acc;
}

uint32_t ldg_math_field_invert_get(ldg_math_field_elem_t src, ldg_math_field_elem_t *out)
{
    ldg_math_field_elem_t canonical = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    canonical = ldg_math_field_reduce(src);

    if (LDG_UNLIKELY(canonical == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = ldg_math_field_pwr_get(canonical, LDG_MATH_FIELD_INVERT_EXP);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_field_batch_invert(const ldg_math_field_elem_t *src, uint64_t cunt, ldg_math_field_elem_t *buff, ldg_math_field_elem_t *out)
{
    uint64_t idx = 0;
    uint32_t ret = LDG_ERR_AOK;
    ldg_math_field_elem_t running = 0;
    ldg_math_field_elem_t inverted = 0;

    if (LDG_UNLIKELY(!src || !buff || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < cunt; idx++) { if (LDG_UNLIKELY(ldg_math_field_reduce(src[idx]) == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; } }

    running = ldg_math_field_reduce(src[0]);
    buff[0] = running;

    for (idx = 1; idx < cunt; idx++)
    {
        running = ldg_math_field_mul(running, src[idx]);
        buff[idx] = running;
    }

    ret = ldg_math_field_invert_get(running, &inverted);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    idx = cunt - LDG_MATH_FIELD_PREV_OFF;

    while (idx > 0)
    {
        out[idx] = ldg_math_field_mul(inverted, buff[idx - LDG_MATH_FIELD_PREV_OFF]);
        inverted = ldg_math_field_mul(inverted, src[idx]);
        idx--;
    }

    out[0] = inverted;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_field_root_get(uint32_t log_cunt, ldg_math_field_elem_t *out)
{
    uint64_t exp = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(log_cunt > LDG_MATH_FIELD_TWO_ADICITY)) { return LDG_ERR_FUNC_ARG_INVALID; }

    exp = LDG_MATH_FIELD_ODD_ORDER << (LDG_MATH_FIELD_TWO_ADICITY - log_cunt);
    *out = ldg_math_field_pwr_get(LDG_MATH_FIELD_GENERATOR, exp);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_field_root_order_ck(ldg_math_field_elem_t root, uint32_t log_cunt)
{
    ldg_math_field_elem_t acc = 0;
    ldg_math_field_elem_t prev = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(log_cunt > LDG_MATH_FIELD_TWO_ADICITY)) { return LDG_ERR_FUNC_ARG_INVALID; }

    acc = ldg_math_field_reduce(root);

    if (LDG_UNLIKELY(acc == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_INVALID; }

    for (idx = 0; idx < log_cunt; idx++)
    {
        prev = acc;
        acc = ldg_math_field_mul(acc, acc);
    }

    if (LDG_UNLIKELY(acc != LDG_MATH_FIELD_MUL_IDENT)) { return LDG_ERR_INVALID; }

    if (log_cunt > 0) { if (LDG_UNLIKELY(prev != ldg_math_field_neg(LDG_MATH_FIELD_MUL_IDENT))) { return LDG_ERR_INVALID; } }

    return LDG_ERR_AOK;
}

uint8_t ldg_math_field_canonical_is(uint64_t val)
{
    if (val < LDG_MATH_FIELD_MODULUS) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}
