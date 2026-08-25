#ifndef LDG_CORE_ARITH_H
#define LDG_CORE_ARITH_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_ARITH_ORD_LT 0
#define LDG_ARITH_ORD_EQ 1
#define LDG_ARITH_ORD_GT 2

typedef struct ldg_uint128
{
    uint64_t limb[2];
} ldg_uint128_t;

typedef struct ldg_uint256
{
    uint64_t limb[4];
} ldg_uint256_t;

LDG_EXPORT uint32_t ldg_arith_uint32_add(uint32_t lhs, uint32_t rhs, uint32_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint32_sub(uint32_t lhs, uint32_t rhs, uint32_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint32_mul(uint32_t lhs, uint32_t rhs, uint32_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint32_div(uint32_t lhs, uint32_t rhs, uint32_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint64_add(uint64_t lhs, uint64_t rhs, uint64_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint64_sub(uint64_t lhs, uint64_t rhs, uint64_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint64_mul(uint64_t lhs, uint64_t rhs, uint64_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint64_div(uint64_t lhs, uint64_t rhs, uint64_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint128_add(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, ldg_uint128_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint128_sub(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, ldg_uint128_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint128_mul(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, ldg_uint128_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint128_div(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, ldg_uint128_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint256_add(const ldg_uint256_t *lhs, const ldg_uint256_t *rhs, ldg_uint256_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint256_sub(const ldg_uint256_t *lhs, const ldg_uint256_t *rhs, ldg_uint256_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint256_mul(const ldg_uint256_t *lhs, const ldg_uint256_t *rhs, ldg_uint256_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint256_div(const ldg_uint256_t *lhs, const ldg_uint256_t *rhs, ldg_uint256_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint128_cmp(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, uint32_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint256_cmp(const ldg_uint256_t *lhs, const ldg_uint256_t *rhs, uint32_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint128_to_uint256_mul(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, ldg_uint256_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint128_div_compute(const ldg_uint128_t *lhs, const ldg_uint128_t *rhs, ldg_uint128_t *quot, ldg_uint128_t *rem);
LDG_EXPORT uint32_t ldg_arith_uint256_div_compute(const ldg_uint256_t *lhs, const ldg_uint256_t *rhs, ldg_uint256_t *quot, ldg_uint256_t *rem);
LDG_EXPORT uint32_t ldg_arith_uint128_shl(const ldg_uint128_t *val, uint32_t amt, ldg_uint128_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint128_shr(const ldg_uint128_t *val, uint32_t amt, ldg_uint128_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint256_shl(const ldg_uint256_t *val, uint32_t amt, ldg_uint256_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint256_shr(const ldg_uint256_t *val, uint32_t amt, ldg_uint256_t *ret);
LDG_EXPORT uint32_t ldg_arith_uint128_to_double_convert(const ldg_uint128_t *val, double *ret);
LDG_EXPORT uint32_t ldg_arith_double_to_uint128_convert(double val, ldg_uint128_t *ret);

#endif
