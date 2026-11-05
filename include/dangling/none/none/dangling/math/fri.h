#ifndef LDG_MATH_FRI_H
#define LDG_MATH_FRI_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/math/ext.h>
#include <dangling/math/field.h>
#include <dangling/math/ntt.h>

#define LDG_MATH_FRI_FOLD_RADIX 2

LDG_EXPORT uint32_t ldg_math_fri_ext_fold(const ldg_math_ext_t *src, uint64_t cunt, ldg_math_field_elem_t shift, ldg_math_ext_t alpha, ldg_math_ext_t *out, ldg_math_field_elem_t *buff, uint64_t cap);
LDG_EXPORT uint32_t ldg_math_fri_ext_fold_pair_get(ldg_math_ext_t lhs_val, ldg_math_ext_t rhs_val, ldg_math_field_elem_t point, ldg_math_ext_t alpha, ldg_math_ext_t *out);
LDG_EXPORT uint32_t ldg_math_fri_ext_remainder_ck(const ldg_math_ext_t *src, uint64_t cunt, ldg_math_field_elem_t shift, uint64_t degree_bound, const ldg_math_field_elem_t *twiddle_inverse, ldg_math_field_elem_t *buff, uint64_t cap);

LDG_EXPORT uint32_t ldg_math_fri_fold_buff_cunt_get(uint64_t cunt, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_fri_fold(const ldg_math_field_elem_t *src, uint64_t cunt, ldg_math_field_elem_t shift, ldg_math_field_elem_t alpha, ldg_math_field_elem_t *out, ldg_math_field_elem_t *buff, uint64_t cap);
LDG_EXPORT uint32_t ldg_math_fri_fold_pair_get(ldg_math_field_elem_t lhs_val, ldg_math_field_elem_t rhs_val, ldg_math_field_elem_t point, ldg_math_field_elem_t alpha, ldg_math_field_elem_t *out);
LDG_EXPORT uint32_t ldg_math_fri_shift_next_get(ldg_math_field_elem_t shift, ldg_math_field_elem_t *out);
LDG_EXPORT uint32_t ldg_math_fri_layer_cunt_get(uint32_t log_domain, uint32_t log_remainder, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_fri_remainder_ck(const ldg_math_field_elem_t *src, uint64_t cunt, ldg_math_field_elem_t shift, uint64_t degree_bound, const ldg_math_field_elem_t *twiddle_inverse, ldg_math_field_elem_t *buff, uint64_t cap);

#endif
