#ifndef LDG_MATH_PROOF_H
#define LDG_MATH_PROOF_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/math/ext.h>
#include <dangling/math/field.h>
#include <dangling/math/ntt.h>

#define LDG_MATH_PROOF_LOG_CUNT_MAX LDG_MATH_NTT_LOG_CUNT_MAX

LDG_EXPORT uint32_t ldg_math_proof_ext_poly_evaluate_get(const ldg_math_field_elem_t *coeff, uint64_t cunt, ldg_math_ext_t point, ldg_math_ext_t *out);
LDG_EXPORT uint32_t ldg_math_proof_ext_coeff_evaluate_get(const ldg_math_ext_t *coeff, uint64_t cunt, ldg_math_ext_t point, ldg_math_ext_t *out);
LDG_EXPORT uint32_t ldg_math_proof_ext_poly_cum(ldg_math_ext_t *acc, const ldg_math_ext_t *src, uint64_t cunt, ldg_math_ext_t factor);
LDG_EXPORT uint32_t ldg_math_proof_ext_vanish_evaluate_get(uint32_t log_trace, ldg_math_ext_t point, ldg_math_ext_t *out);
LDG_EXPORT uint32_t ldg_math_proof_ext_vanish_div(ldg_math_ext_t *data, uint32_t log_domain, ldg_math_field_elem_t shift, uint32_t log_trace, ldg_math_field_elem_t *buff, uint64_t cap);
LDG_EXPORT uint32_t ldg_math_proof_ext_deep_div(ldg_math_ext_t *data, uint32_t log_domain, ldg_math_field_elem_t shift, ldg_math_ext_t point, ldg_math_ext_t val, ldg_math_ext_t *buff, uint64_t cap);
LDG_EXPORT uint32_t ldg_math_proof_lift_deep_div(const ldg_math_field_elem_t *src, uint32_t log_domain, ldg_math_field_elem_t shift, ldg_math_ext_t point, ldg_math_ext_t val, ldg_math_ext_t *out, ldg_math_ext_t *buff, uint64_t cap);

LDG_EXPORT uint32_t ldg_math_proof_poly_evaluate_get(const ldg_math_field_elem_t *coeff, uint64_t cunt, ldg_math_field_elem_t point, ldg_math_field_elem_t *out);
LDG_EXPORT uint32_t ldg_math_proof_poly_scale(ldg_math_field_elem_t *data, uint64_t cunt, ldg_math_field_elem_t factor);
LDG_EXPORT uint32_t ldg_math_proof_poly_add(const ldg_math_field_elem_t *lhs, const ldg_math_field_elem_t *rhs, uint64_t cunt, ldg_math_field_elem_t *out);
LDG_EXPORT uint32_t ldg_math_proof_poly_cum(ldg_math_field_elem_t *acc, const ldg_math_field_elem_t *src, uint64_t cunt, ldg_math_field_elem_t factor);
LDG_EXPORT uint32_t ldg_math_proof_domain_point_get(uint32_t log_domain, ldg_math_field_elem_t shift, uint64_t idx, ldg_math_field_elem_t *out);
LDG_EXPORT uint32_t ldg_math_proof_vanish_evaluate_get(uint32_t log_trace, ldg_math_field_elem_t point, ldg_math_field_elem_t *out);
LDG_EXPORT uint32_t ldg_math_proof_coset_vanish_evaluate_get(uint32_t log_trace, ldg_math_field_elem_t shift, ldg_math_field_elem_t point, ldg_math_field_elem_t *out);
LDG_EXPORT uint32_t ldg_math_proof_vanish_buff_cunt_get(uint32_t log_domain, uint32_t log_trace, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_proof_vanish_div(ldg_math_field_elem_t *data, uint32_t log_domain, ldg_math_field_elem_t shift, uint32_t log_trace, ldg_math_field_elem_t *buff, uint64_t cap);
LDG_EXPORT uint32_t ldg_math_proof_deep_buff_cunt_get(uint32_t log_domain, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_proof_deep_div(ldg_math_field_elem_t *data, uint32_t log_domain, ldg_math_field_elem_t shift, ldg_math_field_elem_t point, ldg_math_field_elem_t val, ldg_math_field_elem_t *buff, uint64_t cap);

#endif
