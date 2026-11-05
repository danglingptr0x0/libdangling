#ifndef LDG_MATH_FIELD_H
#define LDG_MATH_FIELD_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_MATH_FIELD_MODULUS 0xFFFFFFFF00000001ULL
#define LDG_MATH_FIELD_OVERFLOW_FOLD 0x00000000FFFFFFFFULL
#define LDG_MATH_FIELD_ODD_ORDER 0x00000000FFFFFFFFULL
#define LDG_MATH_FIELD_TWO_ADICITY 32
#define LDG_MATH_FIELD_GENERATOR 0x0000000000000007ULL
#define LDG_MATH_FIELD_ADD_IDENT 0x0000000000000000ULL
#define LDG_MATH_FIELD_MUL_IDENT 0x0000000000000001ULL
#define LDG_MATH_FIELD_LIMB_MASK 0x00000000FFFFFFFFULL
#define LDG_MATH_FIELD_LIMB_SHIFT 32
#define LDG_MATH_FIELD_LSB_MASK 0x0000000000000001ULL

typedef uint64_t ldg_math_field_elem_t;

static inline ldg_math_field_elem_t ldg_math_field_reduce(uint64_t val)
{
    if (val >= LDG_MATH_FIELD_MODULUS) { return val - LDG_MATH_FIELD_MODULUS; }

    return val;
}

static inline ldg_math_field_elem_t ldg_math_field_add(ldg_math_field_elem_t lhs, ldg_math_field_elem_t rhs)
{
    uint64_t sum = 0;

    sum = lhs + rhs;

    if (sum < lhs) { sum += LDG_MATH_FIELD_OVERFLOW_FOLD; }

    return ldg_math_field_reduce(sum);
}

static inline ldg_math_field_elem_t ldg_math_field_sub(ldg_math_field_elem_t lhs, ldg_math_field_elem_t rhs)
{
    uint64_t diff = 0;

    diff = lhs - rhs;

    if (rhs > lhs) { diff -= LDG_MATH_FIELD_OVERFLOW_FOLD; }

    return diff;
}

static inline ldg_math_field_elem_t ldg_math_field_neg(ldg_math_field_elem_t src)
{
    if (src == LDG_MATH_FIELD_ADD_IDENT) { return LDG_MATH_FIELD_ADD_IDENT; }

    return LDG_MATH_FIELD_MODULUS - src;
}

LDG_EXPORT ldg_math_field_elem_t ldg_math_field_mul(ldg_math_field_elem_t lhs, ldg_math_field_elem_t rhs);
LDG_EXPORT ldg_math_field_elem_t ldg_math_field_pwr_get(ldg_math_field_elem_t base, uint64_t exp);
LDG_EXPORT uint32_t ldg_math_field_invert_get(ldg_math_field_elem_t src, ldg_math_field_elem_t *out);
LDG_EXPORT uint32_t ldg_math_field_batch_invert(const ldg_math_field_elem_t *src, uint64_t cunt, ldg_math_field_elem_t *buff, ldg_math_field_elem_t *out);
LDG_EXPORT uint32_t ldg_math_field_root_get(uint32_t log_cunt, ldg_math_field_elem_t *out);
LDG_EXPORT uint32_t ldg_math_field_root_order_ck(ldg_math_field_elem_t root, uint32_t log_cunt);
LDG_EXPORT uint8_t ldg_math_field_canonical_is(uint64_t val);

#endif
