#ifndef LDG_MATH_AIR_H
#define LDG_MATH_AIR_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/math/ext.h>
#include <dangling/math/field.h>
#include <dangling/math/ntt.h>

#define LDG_MATH_AIR_COL_MAX 64
#define LDG_MATH_AIR_AUX_COL_MAX 64
#define LDG_MATH_AIR_CONSTRAINT_MAX 64
#define LDG_MATH_AIR_PERIODIC_MAX 64
#define LDG_MATH_AIR_PUB_MAX 64
#define LDG_MATH_AIR_BLIND_MAX 64
#define LDG_MATH_AIR_CHALLENGE_MAX 16
#define LDG_MATH_AIR_LOG_BLOWUP_MIN 1
#define LDG_MATH_AIR_LOG_BLOWUP_MAX 5
#define LDG_MATH_AIR_LOG_TRACE_MIN 1
#define LDG_MATH_AIR_LOG_PERIOD_MIN 1
#define LDG_MATH_AIR_CONSTRAINT_DEGREE_MIN 1

typedef struct ldg_math_air_row
{
    const ldg_math_ext_t *main_curr;
    const ldg_math_ext_t *main_next;
    const ldg_math_ext_t *aux_curr;
    const ldg_math_ext_t *aux_next;
    const ldg_math_ext_t *periodic;
    const ldg_math_ext_t *pub;
    const ldg_math_ext_t *challenge;
    ldg_math_ext_t point;
} ldg_math_air_row_t;

typedef uint32_t (*ldg_math_air_eval_cb_t)(void *ctx, const ldg_math_air_row_t *row, ldg_math_ext_t *out_eval, uint32_t eval_cap);
typedef uint32_t (*ldg_math_air_aux_fill_cb_t)(void *ctx, const ldg_math_field_elem_t *pub, const ldg_math_ext_t *challenge, const ldg_math_field_elem_t *trace, uint64_t row_cunt, uint32_t col_cunt, ldg_math_field_elem_t *aux_out, uint32_t aux_col_cunt);

typedef struct ldg_math_air_conf
{
    uint32_t log_trace;
    uint32_t col_cunt;
    uint32_t aux_col_cunt;
    uint32_t log_blowup;
    uint32_t constraint_cunt;
    uint32_t periodic_cunt;
    uint32_t pub_cunt;
    uint32_t challenge_cunt;
    uint32_t blind_cunt;
    uint32_t log_period;
    uint32_t constraint_degree;
    uint8_t pudding[4];
    ldg_math_field_elem_t coset_shift;
} ldg_math_air_conf_t;

LDG_EXPORT uint32_t ldg_math_air_conf_ck(const ldg_math_air_conf_t *conf);
LDG_EXPORT uint32_t ldg_math_air_domain_cunt_get(const ldg_math_air_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_air_deep_degree_bound_get(const ldg_math_air_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_air_col_coeff_cunt_get(const ldg_math_air_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_air_coeff_cunt_get(const ldg_math_air_conf_t *conf, uint32_t col_cunt, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_air_rand_cunt_get(const ldg_math_air_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_air_composition_buff_cunt_get(const ldg_math_air_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_air_composition_ext_buff_cunt_get(const ldg_math_air_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_air_segment_extend(const ldg_math_air_conf_t *conf, const ldg_math_field_elem_t *trace, uint32_t col_cunt, const ldg_math_field_elem_t *rand, uint32_t blind_cunt, const ldg_math_field_elem_t *twiddle_trace_inverse, const ldg_math_field_elem_t *twiddle_domain, ldg_math_field_elem_t *coeff_out, ldg_math_field_elem_t *lde_out, ldg_math_field_elem_t *buff, uint64_t cap);
LDG_EXPORT uint32_t ldg_math_air_composition_build(const ldg_math_air_conf_t *conf, ldg_math_air_eval_cb_t air_cb, void *air_ctx, const ldg_math_field_elem_t *pub, const ldg_math_ext_t *challenge, const ldg_math_field_elem_t *lde, const ldg_math_field_elem_t *aux_lde, const ldg_math_field_elem_t *periodic, ldg_math_ext_t beta, ldg_math_ext_t *comp_out, ldg_math_ext_t *ext_buff, uint64_t ext_cap, ldg_math_field_elem_t *buff, uint64_t cap);
LDG_EXPORT uint32_t ldg_math_air_periodic_buff_cunt_get(const ldg_math_air_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_air_periodic_extend(const ldg_math_air_conf_t *conf, const ldg_math_field_elem_t *periodic, ldg_math_field_elem_t *lde_out, ldg_math_field_elem_t *buff, uint64_t cap);
LDG_EXPORT uint32_t ldg_math_air_periodic_evaluate(const ldg_math_air_conf_t *conf, const ldg_math_field_elem_t *periodic, ldg_math_ext_t point, ldg_math_ext_t *out, ldg_math_field_elem_t *buff, uint64_t cap);
LDG_EXPORT uint32_t ldg_math_air_col_gather(const ldg_math_field_elem_t *lde, uint64_t row_cunt, uint32_t col_cunt, uint32_t col_idx, ldg_math_field_elem_t *out);

#endif
