#ifndef LDG_MATH_COMPLEX_H
#define LDG_MATH_COMPLEX_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/math.h>

typedef struct ldg_math_complex
{
    double re;
    double im;
} ldg_math_complex_t;

LDG_EXPORT uint32_t ldg_math_complex_add(const ldg_math_complex_t *lhs, const ldg_math_complex_t *rhs, ldg_math_complex_t *out);
LDG_EXPORT uint32_t ldg_math_complex_sub(const ldg_math_complex_t *lhs, const ldg_math_complex_t *rhs, ldg_math_complex_t *out);
LDG_EXPORT uint32_t ldg_math_complex_mul(const ldg_math_complex_t *lhs, const ldg_math_complex_t *rhs, ldg_math_complex_t *out);
LDG_EXPORT uint32_t ldg_math_complex_div(const ldg_math_complex_t *lhs, const ldg_math_complex_t *rhs, ldg_math_complex_t *out);
LDG_EXPORT uint32_t ldg_math_complex_scale(const ldg_math_complex_t *val, double factor, ldg_math_complex_t *out);
LDG_EXPORT uint32_t ldg_math_complex_conjugate(const ldg_math_complex_t *val, ldg_math_complex_t *out);
LDG_EXPORT uint32_t ldg_math_complex_abs_get(const ldg_math_complex_t *val, double *out);
LDG_EXPORT uint32_t ldg_math_complex_arg_get(const ldg_math_complex_t *val, double *out);
LDG_EXPORT uint32_t ldg_math_complex_exp_compute(const ldg_math_complex_t *val, ldg_math_complex_t *out);
LDG_EXPORT uint32_t ldg_math_complex_from_polar_compute(double mag, double arg, ldg_math_complex_t *out);

#endif
