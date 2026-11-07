#include <stdint.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/ext.h>
#include <dangling/math/field.h>
#include <dangling/math/ntt.h>
#include <dangling/math/proof.h>

#define LDG_MATH_PROOF_BUFF_LANE_CUNT 3
#define LDG_MATH_PROOF_UNIT_DOMAIN 1

static ldg_math_field_elem_t proof_pwr_two_get(ldg_math_field_elem_t base, uint32_t log_exp)
{
    ldg_math_field_elem_t acc = 0;
    uint32_t idx = 0;

    acc = ldg_math_field_reduce(base);

    for (idx = 0; idx < log_exp; idx++) { acc = ldg_math_field_mul(acc, acc); }

    return acc;
}

static uint32_t proof_vanish_inverse_fill(uint32_t log_domain, ldg_math_field_elem_t shift, uint32_t log_trace, uint64_t blowup, ldg_math_field_elem_t *buff, ldg_math_field_elem_t **out_inverted)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t idx = 0;
    ldg_math_field_elem_t root = 0;
    ldg_math_field_elem_t zeta = 0;
    ldg_math_field_elem_t shift_pwr = 0;
    ldg_math_field_elem_t curr = LDG_MATH_FIELD_MUL_IDENT;
    ldg_math_field_elem_t *denom = 0x0;
    ldg_math_field_elem_t *mid = 0x0;
    ldg_math_field_elem_t *inverted = 0x0;

    if (LDG_UNLIKELY(blowup == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_field_root_get(log_domain, &root);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    denom = buff;
    mid = buff + blowup;
    inverted = mid + blowup;

    zeta = proof_pwr_two_get(root, log_trace);
    shift_pwr = proof_pwr_two_get(shift, log_trace);

    for (idx = 0; idx < blowup; idx++)
    {
        denom[idx] = ldg_math_field_sub(ldg_math_field_mul(shift_pwr, curr), LDG_MATH_FIELD_MUL_IDENT);

        if (LDG_UNLIKELY(denom[idx] == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

        curr = ldg_math_field_mul(curr, zeta);
    }

    ret = ldg_math_field_batch_invert(denom, blowup, mid, inverted);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_inverted = inverted;

    return LDG_ERR_AOK;
}

static uint32_t proof_ext_denom_invert(uint32_t log_domain, ldg_math_field_elem_t shift, ldg_math_ext_t point, uint64_t cunt, ldg_math_ext_t *buff, ldg_math_ext_t **out_inverted)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t idx = 0;
    ldg_math_field_elem_t root = 0;
    ldg_math_field_elem_t curr = 0;
    ldg_math_ext_t *denom = 0x0;
    ldg_math_ext_t *mid = 0x0;
    ldg_math_ext_t *inverted = 0x0;

    ret = ldg_math_field_root_get(log_domain, &root);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    denom = buff;
    mid = buff + cunt;
    inverted = mid + cunt;
    curr = ldg_math_field_reduce(shift);

    for (idx = 0; idx < cunt; idx++)
    {
        denom[idx] = ldg_math_ext_sub(ldg_math_ext_base_embed(curr), point);

        if (LDG_UNLIKELY(ldg_math_ext_zero_is(denom[idx]) == LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        curr = ldg_math_field_mul(curr, root);
    }

    ret = ldg_math_ext_batch_invert(denom, cunt, mid, inverted);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_inverted = inverted;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_poly_evaluate_get(const ldg_math_field_elem_t *coeff, uint64_t cunt, ldg_math_field_elem_t point, ldg_math_field_elem_t *out)
{
    ldg_math_field_elem_t acc = LDG_MATH_FIELD_ADD_IDENT;
    ldg_math_field_elem_t canonical = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!coeff || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    canonical = ldg_math_field_reduce(point);
    idx = cunt;

    while (idx > 0)
    {
        idx--;
        acc = ldg_math_field_add(ldg_math_field_mul(acc, canonical), ldg_math_field_reduce(coeff[idx]));
    }

    *out = acc;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_poly_scale(ldg_math_field_elem_t *data, uint64_t cunt, ldg_math_field_elem_t factor)
{
    ldg_math_field_elem_t canonical = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!data)) { return LDG_ERR_FUNC_ARG_NULL; }

    canonical = ldg_math_field_reduce(factor);

    for (idx = 0; idx < cunt; idx++) { data[idx] = ldg_math_field_mul(data[idx], canonical); }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_poly_add(const ldg_math_field_elem_t *lhs, const ldg_math_field_elem_t *rhs, uint64_t cunt, ldg_math_field_elem_t *out)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < cunt; idx++) { out[idx] = ldg_math_field_add(ldg_math_field_reduce(lhs[idx]), ldg_math_field_reduce(rhs[idx])); }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_poly_cum(ldg_math_field_elem_t *acc, const ldg_math_field_elem_t *src, uint64_t cunt, ldg_math_field_elem_t factor)
{
    ldg_math_field_elem_t canonical = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!acc || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    canonical = ldg_math_field_reduce(factor);

    for (idx = 0; idx < cunt; idx++) { acc[idx] = ldg_math_field_add(acc[idx], ldg_math_field_mul(ldg_math_field_reduce(src[idx]), canonical)); }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_domain_point_get(uint32_t log_domain, ldg_math_field_elem_t shift, uint64_t idx, ldg_math_field_elem_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    ldg_math_field_elem_t root = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = ldg_math_field_root_get(log_domain, &root);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = ldg_math_field_mul(ldg_math_field_reduce(shift), ldg_math_field_pwr_get(root, idx));

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_vanish_evaluate_get(uint32_t log_trace, ldg_math_field_elem_t point, ldg_math_field_elem_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(log_trace > LDG_MATH_PROOF_LOG_CUNT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = ldg_math_field_sub(proof_pwr_two_get(point, log_trace), LDG_MATH_FIELD_MUL_IDENT);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_coset_vanish_evaluate_get(uint32_t log_trace, ldg_math_field_elem_t shift, ldg_math_field_elem_t point, ldg_math_field_elem_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(log_trace > LDG_MATH_PROOF_LOG_CUNT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = ldg_math_field_sub(proof_pwr_two_get(point, log_trace), proof_pwr_two_get(shift, log_trace));

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_vanish_buff_cunt_get(uint32_t log_domain, uint32_t log_trace, uint64_t *out)
{
    uint64_t blowup = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(log_domain > LDG_MATH_PROOF_LOG_CUNT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(log_trace > log_domain)) { return LDG_ERR_FUNC_ARG_INVALID; }

    blowup = (uint64_t)LDG_MATH_PROOF_UNIT_DOMAIN << (log_domain - log_trace);
    *out = blowup * LDG_MATH_PROOF_BUFF_LANE_CUNT;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_vanish_div(ldg_math_field_elem_t *data, uint32_t log_domain, ldg_math_field_elem_t shift, uint32_t log_trace, ldg_math_field_elem_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t need = 0;
    uint64_t blowup = 0;
    uint64_t cunt = 0;
    uint64_t idx = 0;
    ldg_math_field_elem_t *inverted = 0x0;

    if (LDG_UNLIKELY(!data || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_math_field_reduce(shift) == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_proof_vanish_buff_cunt_get(log_domain, log_trace, &need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_ntt_domain_cunt_get(log_domain, &cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    blowup = need / LDG_MATH_PROOF_BUFF_LANE_CUNT;

    ret = proof_vanish_inverse_fill(log_domain, shift, log_trace, blowup, buff, &inverted);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < cunt; idx++) { data[idx] = ldg_math_field_mul(data[idx], inverted[idx % blowup]); }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_deep_buff_cunt_get(uint32_t log_domain, uint64_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t cunt = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = ldg_math_ntt_domain_cunt_get(log_domain, &cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = cunt * LDG_MATH_PROOF_BUFF_LANE_CUNT;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_deep_div(ldg_math_field_elem_t *data, uint32_t log_domain, ldg_math_field_elem_t shift, ldg_math_field_elem_t point, ldg_math_field_elem_t val, ldg_math_field_elem_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t need = 0;
    uint64_t cunt = 0;
    uint64_t idx = 0;
    ldg_math_field_elem_t root = 0;
    ldg_math_field_elem_t canonical = 0;
    ldg_math_field_elem_t curr = 0;
    ldg_math_field_elem_t *denom = 0x0;
    ldg_math_field_elem_t *mid = 0x0;
    ldg_math_field_elem_t *inverted = 0x0;

    if (LDG_UNLIKELY(!data || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_math_field_reduce(shift) == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_proof_deep_buff_cunt_get(log_domain, &need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_ntt_domain_cunt_get(log_domain, &cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_field_root_get(log_domain, &root);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    denom = buff;
    mid = buff + cunt;
    inverted = mid + cunt;

    canonical = ldg_math_field_reduce(point);
    curr = ldg_math_field_reduce(shift);

    for (idx = 0; idx < cunt; idx++)
    {
        denom[idx] = ldg_math_field_sub(curr, canonical);

        if (LDG_UNLIKELY(denom[idx] == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

        curr = ldg_math_field_mul(curr, root);
    }

    ret = ldg_math_field_batch_invert(denom, cunt, mid, inverted);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    canonical = ldg_math_field_reduce(val);

    for (idx = 0; idx < cunt; idx++) { data[idx] = ldg_math_field_mul(ldg_math_field_sub(data[idx], canonical), inverted[idx]); }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_ext_poly_evaluate_get(const ldg_math_field_elem_t *coeff, uint64_t cunt, ldg_math_ext_t point, ldg_math_ext_t *out)
{
    ldg_math_ext_t acc = LDG_STRUCT_ZERO_INIT;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!coeff || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    acc = ldg_math_ext_base_embed(LDG_MATH_FIELD_ADD_IDENT);
    *out = acc;

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = cunt;

    while (idx > 0)
    {
        idx--;
        acc = ldg_math_ext_add(ldg_math_ext_mul(acc, point), ldg_math_ext_base_embed(coeff[idx]));
    }

    *out = acc;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_ext_coeff_evaluate_get(const ldg_math_ext_t *coeff, uint64_t cunt, ldg_math_ext_t point, ldg_math_ext_t *out)
{
    ldg_math_ext_t acc = LDG_STRUCT_ZERO_INIT;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!coeff || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    acc = ldg_math_ext_base_embed(LDG_MATH_FIELD_ADD_IDENT);
    *out = acc;

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = cunt;

    while (idx > 0)
    {
        idx--;
        acc = ldg_math_ext_add(ldg_math_ext_mul(acc, point), coeff[idx]);
    }

    *out = acc;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_ext_poly_cum(ldg_math_ext_t *acc, const ldg_math_ext_t *src, uint64_t cunt, ldg_math_ext_t factor)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!acc || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < cunt; idx++) { acc[idx] = ldg_math_ext_add(acc[idx], ldg_math_ext_mul(src[idx], factor)); }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_ext_vanish_evaluate_get(uint32_t log_trace, ldg_math_ext_t point, ldg_math_ext_t *out)
{
    ldg_math_ext_t acc = LDG_STRUCT_ZERO_INIT;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    acc = ldg_math_ext_base_embed(LDG_MATH_FIELD_ADD_IDENT);
    *out = acc;

    if (LDG_UNLIKELY(log_trace > LDG_MATH_PROOF_LOG_CUNT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    acc = point;

    for (idx = 0; idx < log_trace; idx++) { acc = ldg_math_ext_square(acc); }

    *out = ldg_math_ext_sub(acc, ldg_math_ext_base_embed(LDG_MATH_FIELD_MUL_IDENT));

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_ext_vanish_div(ldg_math_ext_t *data, uint32_t log_domain, ldg_math_field_elem_t shift, uint32_t log_trace, ldg_math_field_elem_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t need = 0;
    uint64_t blowup = 0;
    uint64_t cunt = 0;
    uint64_t idx = 0;
    ldg_math_field_elem_t *inverted = 0x0;

    if (LDG_UNLIKELY(!data || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_math_field_reduce(shift) == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_proof_vanish_buff_cunt_get(log_domain, log_trace, &need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_ntt_domain_cunt_get(log_domain, &cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    blowup = need / LDG_MATH_PROOF_BUFF_LANE_CUNT;

    ret = proof_vanish_inverse_fill(log_domain, shift, log_trace, blowup, buff, &inverted);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < cunt; idx++) { data[idx] = ldg_math_ext_scale(data[idx], inverted[idx % blowup]); }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_ext_deep_div(ldg_math_ext_t *data, uint32_t log_domain, ldg_math_field_elem_t shift, ldg_math_ext_t point, ldg_math_ext_t val, ldg_math_ext_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t need = 0;
    uint64_t cunt = 0;
    uint64_t idx = 0;
    ldg_math_ext_t *inverted = 0x0;

    if (LDG_UNLIKELY(!data || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_math_field_reduce(shift) == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_proof_deep_buff_cunt_get(log_domain, &need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_ntt_domain_cunt_get(log_domain, &cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = proof_ext_denom_invert(log_domain, shift, point, cunt, buff, &inverted);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < cunt; idx++) { data[idx] = ldg_math_ext_mul(ldg_math_ext_sub(data[idx], val), inverted[idx]); }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_proof_lift_deep_div(const ldg_math_field_elem_t *src, uint32_t log_domain, ldg_math_field_elem_t shift, ldg_math_ext_t point, ldg_math_ext_t val, ldg_math_ext_t *out, ldg_math_ext_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t need = 0;
    uint64_t cunt = 0;
    uint64_t idx = 0;
    ldg_math_ext_t *inverted = 0x0;

    if (LDG_UNLIKELY(!src || !out || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_math_field_reduce(shift) == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_proof_deep_buff_cunt_get(log_domain, &need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_ntt_domain_cunt_get(log_domain, &cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = proof_ext_denom_invert(log_domain, shift, point, cunt, buff, &inverted);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < cunt; idx++) { out[idx] = ldg_math_ext_mul(ldg_math_ext_sub(ldg_math_ext_base_embed(src[idx]), val), inverted[idx]); }

    return LDG_ERR_AOK;
}
