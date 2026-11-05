#ifndef LDG_MATH_STARK_H
#define LDG_MATH_STARK_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/math/air.h>
#include <dangling/math/field.h>
#include <dangling/math/fri.h>
#include <dangling/math/merkle.h>
#include <dangling/math/ntt.h>
#include <dangling/math/transcript.h>

#define LDG_MATH_STARK_ROOT_SIZE LDG_MATH_MERKLE_NODE_SIZE
#define LDG_MATH_STARK_QUERY_MAX 128
#define LDG_MATH_STARK_QUERY_MIN 1
#define LDG_MATH_STARK_LAYER_MAX 32
#define LDG_MATH_STARK_LOG_REMAINDER_MIN 2

typedef struct ldg_math_stark_conf
{
    ldg_math_air_conf_t air;
    uint32_t query_cunt;
    uint32_t log_remainder;
    uint32_t grind_bit_cunt;
    uint8_t pudding[4];
} ldg_math_stark_conf_t;

LDG_EXPORT uint32_t ldg_math_stark_conf_default_get(ldg_math_stark_conf_t *out);
LDG_EXPORT uint32_t ldg_math_stark_conf_ck(const ldg_math_stark_conf_t *conf);
LDG_EXPORT uint32_t ldg_math_stark_prove_buff_cunt_get(const ldg_math_stark_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_stark_verify_buff_cunt_get(const ldg_math_stark_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_stark_node_cap_get(const ldg_math_stark_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_stark_proof_size_get(const ldg_math_stark_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_stark_prove(const ldg_math_stark_conf_t *conf, ldg_math_air_eval_cb_t air_cb, ldg_math_air_aux_fill_cb_t aux_cb, void *air_ctx, const ldg_math_field_elem_t *pub, const ldg_math_field_elem_t *trace, const ldg_math_field_elem_t *rand, const ldg_math_field_elem_t *periodic, ldg_math_field_elem_t *buff, uint64_t buff_cunt, uint8_t *node, uint64_t node_cap, uint8_t *proof, uint64_t proof_cap, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_math_stark_verify(const ldg_math_stark_conf_t *conf, ldg_math_air_eval_cb_t air_cb, void *air_ctx, const ldg_math_field_elem_t *pub, const ldg_math_field_elem_t *periodic, const uint8_t *proof, uint64_t proof_len, ldg_math_field_elem_t *buff, uint64_t buff_cunt);

#endif
