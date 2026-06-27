#ifndef LDG_CORE_ARITH_H
#define LDG_CORE_ARITH_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT uint32_t ldg_arith_32_add(uint32_t lhs, uint32_t rhs, uint32_t *ret);
LDG_EXPORT uint32_t ldg_arith_32_sub(uint32_t lhs, uint32_t rhs, uint32_t *ret);
LDG_EXPORT uint32_t ldg_arith_32_mul(uint32_t lhs, uint32_t rhs, uint32_t *ret);
LDG_EXPORT uint32_t ldg_arith_32_div(uint32_t lhs, uint32_t rhs, uint32_t *ret);
LDG_EXPORT uint32_t ldg_arith_64_add(uint64_t lhs, uint64_t rhs, uint64_t *ret);
LDG_EXPORT uint32_t ldg_arith_64_sub(uint64_t lhs, uint64_t rhs, uint64_t *ret);
LDG_EXPORT uint32_t ldg_arith_64_mul(uint64_t lhs, uint64_t rhs, uint64_t *ret);
LDG_EXPORT uint32_t ldg_arith_64_div(uint64_t lhs, uint64_t rhs, uint64_t *ret);

#endif
