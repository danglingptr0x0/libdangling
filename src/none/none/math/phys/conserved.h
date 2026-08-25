#ifndef LDG_MATH_PHYS_CONSERVED_H
#define LDG_MATH_PHYS_CONSERVED_H

#include <stdint.h>

#include <dangling/math/phys.h>

uint32_t ldg_math_phys_conserved_add(const ldg_math_phys_conserved_t *lhs, const ldg_math_phys_conserved_t *rhs, ldg_math_phys_conserved_t *out);
uint32_t ldg_math_phys_conserved_sub(const ldg_math_phys_conserved_t *lhs, const ldg_math_phys_conserved_t *rhs, ldg_math_phys_conserved_t *out);
uint32_t ldg_math_phys_conserved_neg(const ldg_math_phys_conserved_t *val, ldg_math_phys_conserved_t *out);
uint32_t ldg_math_phys_conserved_cmp(const ldg_math_phys_conserved_t *lhs, const ldg_math_phys_conserved_t *rhs, uint32_t *out);
uint32_t ldg_math_phys_conserved_rescale(const ldg_math_phys_conserved_t *val, int64_t delta, ldg_math_phys_conserved_t *out);
uint32_t ldg_math_phys_double_to_conserved_convert(double val, int64_t exponent, ldg_math_phys_conserved_t *out);
uint32_t ldg_math_phys_conserved_to_double_convert(const ldg_math_phys_conserved_t *val, int64_t exponent, double *out);

#endif
