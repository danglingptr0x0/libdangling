#ifndef LDG_MATH_CHOLESKY_H
#define LDG_MATH_CHOLESKY_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/complex.h>

typedef struct ldg_math_cholesky_ctx
{
    ldg_math_complex_t *factor;
    uint64_t dim;
    uint8_t ready;
    uint8_t pudding[7];
} ldg_math_cholesky_ctx_t;

LDG_EXPORT uint32_t ldg_math_cholesky_ctx_create(uint64_t dim, ldg_math_cholesky_ctx_t **out);
LDG_EXPORT uint32_t ldg_math_cholesky_ctx_destroy(ldg_math_cholesky_ctx_t **ctx);
LDG_EXPORT uint32_t ldg_math_cholesky_factor_compute(ldg_math_cholesky_ctx_t *ctx, const ldg_math_complex_t *mat);
LDG_EXPORT uint32_t ldg_math_cholesky_solve_compute(const ldg_math_cholesky_ctx_t *ctx, const ldg_math_complex_t *rhs, uint64_t rhs_cunt, ldg_math_complex_t *out);

#endif
