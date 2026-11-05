#ifndef LDG_MATH_NTT_H
#define LDG_MATH_NTT_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/math/field.h>

#define LDG_MATH_NTT_LOG_CUNT_MAX LDG_MATH_FIELD_TWO_ADICITY

LDG_EXPORT uint32_t ldg_math_ntt_domain_cunt_get(uint32_t log_cunt, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_ntt_twiddle_cunt_get(uint32_t log_cunt, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_ntt_twiddle_fill(uint32_t log_cunt, uint8_t inverse_is, ldg_math_field_elem_t *buff, uint64_t cap);
LDG_EXPORT uint32_t ldg_math_ntt_bit_reverse(ldg_math_field_elem_t *data, uint32_t log_cunt);
LDG_EXPORT uint32_t ldg_math_ntt_evaluate(ldg_math_field_elem_t *data, uint32_t log_cunt, const ldg_math_field_elem_t *twiddle);
LDG_EXPORT uint32_t ldg_math_ntt_interpolate(ldg_math_field_elem_t *data, uint32_t log_cunt, const ldg_math_field_elem_t *twiddle);
LDG_EXPORT uint32_t ldg_math_ntt_coset_evaluate(ldg_math_field_elem_t *data, uint32_t log_cunt, ldg_math_field_elem_t shift, const ldg_math_field_elem_t *twiddle);
LDG_EXPORT uint32_t ldg_math_ntt_coset_interpolate(ldg_math_field_elem_t *data, uint32_t log_cunt, ldg_math_field_elem_t shift, const ldg_math_field_elem_t *twiddle);
LDG_EXPORT uint32_t ldg_math_ntt_extend(const ldg_math_field_elem_t *src, uint32_t log_src, ldg_math_field_elem_t *dst, uint32_t log_dst, ldg_math_field_elem_t shift, const ldg_math_field_elem_t *twiddle);

#endif
