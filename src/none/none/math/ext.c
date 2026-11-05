#include <stdint.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/ext.h>
#include <dangling/math/field.h>

#define LDG_MATH_EXT_BIT_ADVANCE 1
#define LDG_MATH_EXT_PREV_OFF 1
#define LDG_MATH_EXT_LEGENDRE_EXP 0x7FFFFFFF80000000ULL

uint32_t ldg_math_ext_norm_get(ldg_math_ext_t src, ldg_math_field_elem_t *out)
{
    ldg_math_field_elem_t lo_reduced = 0;
    ldg_math_field_elem_t hi_reduced = 0;
    ldg_math_field_elem_t lo_square = 0;
    ldg_math_field_elem_t hi_square = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    lo_reduced = ldg_math_field_reduce(src.lo);
    hi_reduced = ldg_math_field_reduce(src.hi);
    lo_square = ldg_math_field_mul(lo_reduced, lo_reduced);
    hi_square = ldg_math_field_mul(LDG_MATH_EXT_NON_RESIDUE, ldg_math_field_mul(hi_reduced, hi_reduced));
    *out = ldg_math_field_sub(lo_square, hi_square);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_ext_invert_get(ldg_math_ext_t src, ldg_math_ext_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    ldg_math_field_elem_t norm = 0;
    ldg_math_field_elem_t norm_inverse = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->lo = LDG_MATH_FIELD_ADD_IDENT;
    out->hi = LDG_MATH_FIELD_ADD_IDENT;

    if (LDG_UNLIKELY(ldg_math_ext_zero_is(src) == LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_ext_norm_get(src, &norm);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_field_invert_get(norm, &norm_inverse);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->lo = ldg_math_field_mul(ldg_math_field_reduce(src.lo), norm_inverse);
    out->hi = ldg_math_field_mul(ldg_math_field_neg(ldg_math_field_reduce(src.hi)), norm_inverse);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_ext_pwr_get(ldg_math_ext_t base, uint64_t exp, ldg_math_ext_t *out)
{
    ldg_math_ext_t acc = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t curr = LDG_STRUCT_ZERO_INIT;
    uint64_t bit_walk = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    acc = ldg_math_ext_base_embed(LDG_MATH_FIELD_MUL_IDENT);
    curr.lo = ldg_math_field_reduce(base.lo);
    curr.hi = ldg_math_field_reduce(base.hi);
    bit_walk = exp;

    while (bit_walk != 0)
    {
        if ((bit_walk & LDG_MATH_FIELD_LSB_MASK) != 0) { acc = ldg_math_ext_mul(acc, curr); }

        curr = ldg_math_ext_square(curr);
        bit_walk >>= LDG_MATH_EXT_BIT_ADVANCE;
    }

    *out = acc;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_ext_batch_invert(const ldg_math_ext_t *src, uint64_t cunt, ldg_math_ext_t *buff, ldg_math_ext_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t idx = 0;
    ldg_math_ext_t running = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t inverted = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!src || !buff || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < cunt; idx++) { if (LDG_UNLIKELY(ldg_math_ext_zero_is(src[idx]) == LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; } }

    running = src[0];
    buff[0] = running;

    for (idx = 1; idx < cunt; idx++)
    {
        running = ldg_math_ext_mul(running, src[idx]);
        buff[idx] = running;
    }

    ret = ldg_math_ext_invert_get(running, &inverted);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    idx = cunt - LDG_MATH_EXT_PREV_OFF;

    while (idx > 0)
    {
        out[idx] = ldg_math_ext_mul(inverted, buff[idx - LDG_MATH_EXT_PREV_OFF]);
        inverted = ldg_math_ext_mul(inverted, src[idx]);
        idx--;
    }

    out[0] = inverted;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_ext_non_residue_ck(void)
{
    ldg_math_field_elem_t legendre = 0;

    legendre = ldg_math_field_pwr_get(LDG_MATH_EXT_NON_RESIDUE, LDG_MATH_EXT_LEGENDRE_EXP);

    if (legendre != ldg_math_field_neg(LDG_MATH_FIELD_MUL_IDENT)) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}
