#ifndef LDG_MATH_RATIONAL_H
#define LDG_MATH_RATIONAL_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_MATH_RATIONAL_WHOLE_DEN 1U

typedef struct ldg_math_rational
{
    int64_t num;
    uint64_t den;
} ldg_math_rational_t;

LDG_EXPORT uint32_t ldg_math_rational_set(int64_t num, uint64_t den, ldg_math_rational_t *out);
LDG_EXPORT uint32_t ldg_math_rational_add(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, ldg_math_rational_t *out);
LDG_EXPORT uint32_t ldg_math_rational_sub(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, ldg_math_rational_t *out);
LDG_EXPORT uint32_t ldg_math_rational_mul(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, ldg_math_rational_t *out);
LDG_EXPORT uint32_t ldg_math_rational_div(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, ldg_math_rational_t *out);
LDG_EXPORT uint32_t ldg_math_rational_neg(const ldg_math_rational_t *val, ldg_math_rational_t *out);
LDG_EXPORT uint32_t ldg_math_rational_abs(const ldg_math_rational_t *val, ldg_math_rational_t *out);
LDG_EXPORT uint32_t ldg_math_rational_cmp(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_rational_min(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, ldg_math_rational_t *out);
LDG_EXPORT uint32_t ldg_math_rational_max(const ldg_math_rational_t *lhs, const ldg_math_rational_t *rhs, ldg_math_rational_t *out);
LDG_EXPORT uint32_t ldg_math_rational_uint64_mul(const ldg_math_rational_t *val, uint64_t factor, ldg_math_rational_t *out);
LDG_EXPORT uint32_t ldg_math_rational_floor_get(const ldg_math_rational_t *val, int64_t *out);
LDG_EXPORT uint32_t ldg_math_rational_ceil_get(const ldg_math_rational_t *val, int64_t *out);

#endif
