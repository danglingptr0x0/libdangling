#ifndef LDG_MATH_PREDICATE_H
#define LDG_MATH_PREDICATE_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>

#define LDG_MATH_PREDICATE_POOL_CAP 655360u

typedef enum ldg_math_sign
{
    LDG_MATH_SIGN_ZERO,
    LDG_MATH_SIGN_NEG,
    LDG_MATH_SIGN_POS
} ldg_math_sign_t;

LDG_EXPORT uint32_t ldg_math_2d_orient(const double *point_a, const double *point_b, const double *point_c, ldg_mem_pool_t *pool, uint8_t *out);

LDG_EXPORT uint32_t ldg_math_2d_incircle(const double *point_a, const double *point_b, const double *point_c, const double *point_d, ldg_mem_pool_t *pool, uint8_t *out);

LDG_EXPORT uint32_t ldg_math_3d_orient(const double *point_a, const double *point_b, const double *point_c, const double *point_d, ldg_mem_pool_t *pool, uint8_t *out);

LDG_EXPORT uint32_t ldg_math_3d_insphere(const double *point_a, const double *point_b, const double *point_c, const double *point_d, const double *point_e, ldg_mem_pool_t *pool, uint8_t *out);

LDG_EXPORT uint32_t ldg_math_3d_orient_perturb(const double *vert, uint32_t idx_a, uint32_t idx_b, uint32_t idx_c, uint32_t idx_d, ldg_mem_pool_t *pool, uint8_t *out);

LDG_EXPORT uint32_t ldg_math_3d_insphere_perturb(const double *vert, uint32_t idx_a, uint32_t idx_b, uint32_t idx_c, uint32_t idx_d, uint32_t idx_e, ldg_mem_pool_t *pool, uint8_t *out);

#endif
