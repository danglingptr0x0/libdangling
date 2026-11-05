#ifndef LDG_MATH_EXT_H
#define LDG_MATH_EXT_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/math/field.h>

#define LDG_MATH_EXT_DEGREE 2
#define LDG_MATH_EXT_NON_RESIDUE 0x0000000000000007ULL

typedef struct ldg_math_ext
{
    ldg_math_field_elem_t lo;
    ldg_math_field_elem_t hi;
} ldg_math_ext_t;

static inline ldg_math_ext_t ldg_math_ext_base_embed(ldg_math_field_elem_t src)
{
    ldg_math_ext_t out;

    out.lo = ldg_math_field_reduce(src);
    out.hi = LDG_MATH_FIELD_ADD_IDENT;

    return out;
}

static inline ldg_math_ext_t ldg_math_ext_add(ldg_math_ext_t lhs, ldg_math_ext_t rhs)
{
    ldg_math_ext_t out;

    out.lo = ldg_math_field_add(lhs.lo, rhs.lo);
    out.hi = ldg_math_field_add(lhs.hi, rhs.hi);

    return out;
}

static inline ldg_math_ext_t ldg_math_ext_sub(ldg_math_ext_t lhs, ldg_math_ext_t rhs)
{
    ldg_math_ext_t out;

    out.lo = ldg_math_field_sub(lhs.lo, rhs.lo);
    out.hi = ldg_math_field_sub(lhs.hi, rhs.hi);

    return out;
}

static inline ldg_math_ext_t ldg_math_ext_neg(ldg_math_ext_t src)
{
    ldg_math_ext_t out;

    out.lo = ldg_math_field_neg(src.lo);
    out.hi = ldg_math_field_neg(src.hi);

    return out;
}

static inline ldg_math_ext_t ldg_math_ext_mul(ldg_math_ext_t lhs, ldg_math_ext_t rhs)
{
    ldg_math_ext_t out;
    ldg_math_field_elem_t cross_lo = 0;
    ldg_math_field_elem_t cross_hi = 0;

    cross_lo = ldg_math_field_mul(lhs.hi, rhs.hi);
    cross_hi = ldg_math_field_add(ldg_math_field_mul(lhs.lo, rhs.hi), ldg_math_field_mul(lhs.hi, rhs.lo));

    out.lo = ldg_math_field_add(ldg_math_field_mul(lhs.lo, rhs.lo), ldg_math_field_mul(LDG_MATH_EXT_NON_RESIDUE, cross_lo));
    out.hi = cross_hi;

    return out;
}

static inline ldg_math_ext_t ldg_math_ext_scale(ldg_math_ext_t src, ldg_math_field_elem_t factor)
{
    ldg_math_ext_t out;
    ldg_math_field_elem_t canonical = 0;

    canonical = ldg_math_field_reduce(factor);
    out.lo = ldg_math_field_mul(src.lo, canonical);
    out.hi = ldg_math_field_mul(src.hi, canonical);

    return out;
}

static inline ldg_math_ext_t ldg_math_ext_square(ldg_math_ext_t src)
{
    return ldg_math_ext_mul(src, src);
}

static inline uint8_t ldg_math_ext_zero_is(ldg_math_ext_t src)
{
    if (src.lo == LDG_MATH_FIELD_ADD_IDENT && src.hi == LDG_MATH_FIELD_ADD_IDENT) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static inline uint8_t ldg_math_ext_eq_is(ldg_math_ext_t lhs, ldg_math_ext_t rhs)
{
    if (lhs.lo == rhs.lo && lhs.hi == rhs.hi) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

LDG_EXPORT uint32_t ldg_math_ext_norm_get(ldg_math_ext_t src, ldg_math_field_elem_t *out);
LDG_EXPORT uint32_t ldg_math_ext_invert_get(ldg_math_ext_t src, ldg_math_ext_t *out);
LDG_EXPORT uint32_t ldg_math_ext_pwr_get(ldg_math_ext_t base, uint64_t exp, ldg_math_ext_t *out);
LDG_EXPORT uint32_t ldg_math_ext_non_residue_ck(void);
LDG_EXPORT uint32_t ldg_math_ext_batch_invert(const ldg_math_ext_t *src, uint64_t cunt, ldg_math_ext_t *buff, ldg_math_ext_t *out);

#endif
