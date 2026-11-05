#include <stdint.h>
#include <dangling/core/arith.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/air.h>
#include <dangling/math/ext.h>
#include <dangling/math/field.h>
#include <dangling/math/ntt.h>
#include <dangling/math/proof.h>

#define LDG_MATH_AIR_UNIT_DOMAIN 1
#define LDG_MATH_AIR_PREV_OFF 1
#define LDG_MATH_AIR_BUFF_LANE_CUNT 3
#define LDG_MATH_AIR_WINDOW_CUNT 2

static uint32_t air_deep_degree_bound_compute(const ldg_math_air_conf_t *conf, uint64_t trace_cunt, uint64_t *out)
{
    uint64_t trace_bound = 0;
    uint64_t comp_bound = 0;
    uint32_t ret = 0;

    *out = 0;
    trace_bound = trace_cunt + conf->blind_cunt;

    ret = ldg_arith_uint64_mul(trace_bound, (uint64_t)conf->constraint_degree, &comp_bound);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    comp_bound -= trace_cunt;
    *out = (comp_bound > trace_bound) ? comp_bound : trace_bound;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_air_conf_ck(const ldg_math_air_conf_t *conf)
{
    uint64_t trace_cunt = 0;
    uint64_t deep_bound = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->log_trace < LDG_MATH_AIR_LOG_TRACE_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->log_blowup < LDG_MATH_AIR_LOG_BLOWUP_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->log_blowup > LDG_MATH_AIR_LOG_BLOWUP_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY((conf->log_trace + conf->log_blowup) > LDG_MATH_NTT_LOG_CUNT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->col_cunt == 0 || conf->col_cunt > LDG_MATH_AIR_COL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->aux_col_cunt > LDG_MATH_AIR_AUX_COL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->constraint_cunt == 0 || conf->constraint_cunt > LDG_MATH_AIR_CONSTRAINT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->periodic_cunt > LDG_MATH_AIR_PERIODIC_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->pub_cunt > LDG_MATH_AIR_PUB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (conf->periodic_cunt > 0) { if (LDG_UNLIKELY(conf->log_period < LDG_MATH_AIR_LOG_PERIOD_MIN || conf->log_period > conf->log_trace)) { return LDG_ERR_FUNC_ARG_INVALID; } }

    if (LDG_UNLIKELY(conf->challenge_cunt > LDG_MATH_AIR_CHALLENGE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->blind_cunt > LDG_MATH_AIR_BLIND_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->aux_col_cunt == 0 && conf->challenge_cunt > 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->aux_col_cunt > 0 && conf->challenge_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    trace_cunt = (uint64_t)LDG_MATH_AIR_UNIT_DOMAIN << conf->log_trace;

    if (LDG_UNLIKELY((uint64_t)conf->blind_cunt > trace_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->constraint_degree < LDG_MATH_AIR_CONSTRAINT_DEGREE_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = air_deep_degree_bound_compute(conf, trace_cunt, &deep_bound);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(deep_bound >= (trace_cunt << conf->log_blowup))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ldg_math_field_reduce(conf->coset_shift) == LDG_MATH_FIELD_ADD_IDENT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_air_deep_degree_bound_get(const ldg_math_air_conf_t *conf, uint64_t *out)
{
    uint64_t trace_cunt = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    ret = ldg_math_air_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_domain_cunt_get(conf->log_trace, &trace_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return air_deep_degree_bound_compute(conf, trace_cunt, out);
}

uint32_t ldg_math_air_domain_cunt_get(const ldg_math_air_conf_t *conf, uint64_t *out)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    ret = ldg_math_air_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_ntt_domain_cunt_get(conf->log_trace + conf->log_blowup, out);
}

uint32_t ldg_math_air_col_coeff_cunt_get(const ldg_math_air_conf_t *conf, uint64_t *out)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    ret = ldg_math_air_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_ntt_domain_cunt_get(conf->log_trace + LDG_MATH_AIR_PREV_OFF, out);
}

uint32_t ldg_math_air_coeff_cunt_get(const ldg_math_air_conf_t *conf, uint32_t col_cunt, uint64_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t col_coeff = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    ret = ldg_math_air_col_coeff_cunt_get(conf, &col_coeff);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = col_coeff * col_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_air_rand_cunt_get(const ldg_math_air_conf_t *conf, uint64_t *out)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    ret = ldg_math_air_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = (uint64_t)conf->blind_cunt * (conf->col_cunt + conf->aux_col_cunt);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_air_composition_buff_cunt_get(const ldg_math_air_conf_t *conf, uint64_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t blowup = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    ret = ldg_math_air_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_domain_cunt_get(conf->log_blowup, &blowup);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = blowup * LDG_MATH_AIR_BUFF_LANE_CUNT;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_air_composition_ext_buff_cunt_get(const ldg_math_air_conf_t *conf, uint64_t *out)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    ret = ldg_math_air_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = (uint64_t)conf->constraint_cunt + ((uint64_t)(conf->col_cunt + conf->aux_col_cunt) * LDG_MATH_AIR_WINDOW_CUNT) + conf->periodic_cunt + conf->pub_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_air_col_gather(const ldg_math_field_elem_t *lde, uint64_t row_cunt, uint32_t col_cunt, uint32_t col_idx, ldg_math_field_elem_t *out)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!lde || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(col_cunt == 0 || col_idx >= col_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < row_cunt; idx++) { out[idx] = lde[(idx * col_cunt) + col_idx]; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_air_segment_extend(const ldg_math_air_conf_t *conf, const ldg_math_field_elem_t *trace, uint32_t col_cunt, const ldg_math_field_elem_t *rand, uint32_t blind_cunt, const ldg_math_field_elem_t *twiddle_trace_inverse, const ldg_math_field_elem_t *twiddle_domain, ldg_math_field_elem_t *coeff_out, ldg_math_field_elem_t *lde_out, ldg_math_field_elem_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t col = 0;
    uint32_t bdx = 0;
    uint64_t trace_cunt = 0;
    uint64_t col_coeff = 0;
    uint64_t domain_cunt = 0;
    uint64_t idx = 0;
    ldg_math_field_elem_t *dst = 0x0;
    ldg_math_field_elem_t mask = 0;

    if (LDG_UNLIKELY(!trace || !twiddle_trace_inverse || !twiddle_domain || !coeff_out || !lde_out || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_air_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(col_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(blind_cunt > 0 && !rand)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_ntt_domain_cunt_get(conf->log_trace, &trace_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY((uint64_t)blind_cunt > trace_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_air_col_coeff_cunt_get(conf, &col_coeff);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_air_domain_cunt_get(conf, &domain_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < domain_cunt)) { return LDG_ERR_BOUNDS; }

    for (col = 0; col < col_cunt; col++)
    {
        for (idx = 0; idx < trace_cunt; idx++) { buff[idx] = ldg_math_field_reduce(trace[(idx * col_cunt) + col]); }

        ret = ldg_math_ntt_interpolate(buff, conf->log_trace, twiddle_trace_inverse);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        dst = coeff_out + ((uint64_t)col * col_coeff);

        for (idx = 0; idx < trace_cunt; idx++) { dst[idx] = buff[idx]; }

        for (idx = trace_cunt; idx < col_coeff; idx++) { dst[idx] = LDG_MATH_FIELD_ADD_IDENT; }

        for (bdx = 0; bdx < blind_cunt; bdx++)
        {
            mask = ldg_math_field_reduce(rand[((uint64_t)col * blind_cunt) + bdx]);
            dst[bdx] = ldg_math_field_sub(dst[bdx], mask);
            dst[trace_cunt + bdx] = ldg_math_field_add(dst[trace_cunt + bdx], mask);
        }

        ret = ldg_math_ntt_extend(dst, conf->log_trace + LDG_MATH_AIR_PREV_OFF, buff, conf->log_trace + conf->log_blowup, conf->coset_shift, twiddle_domain);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (idx = 0; idx < domain_cunt; idx++) { lde_out[(idx * col_cunt) + col] = buff[idx]; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_air_composition_build(const ldg_math_air_conf_t *conf, ldg_math_air_eval_cb_t air_cb, void *air_ctx, const ldg_math_field_elem_t *pub, const ldg_math_ext_t *challenge, const ldg_math_field_elem_t *lde, const ldg_math_field_elem_t *aux_lde, const ldg_math_field_elem_t *periodic, ldg_math_ext_t beta, ldg_math_ext_t *comp_out, ldg_math_ext_t *ext_buff, uint64_t ext_cap, ldg_math_field_elem_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t kdx = 0;
    uint32_t col = 0;
    uint64_t need = 0;
    uint64_t ext_need = 0;
    uint64_t domain_cunt = 0;
    uint64_t blowup = 0;
    uint64_t idx = 0;
    uint64_t next_idx = 0;
    ldg_math_air_row_t row = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t *eval_buff = 0x0;
    ldg_math_ext_t *main_curr = 0x0;
    ldg_math_ext_t *main_next = 0x0;
    ldg_math_ext_t *aux_curr = 0x0;
    ldg_math_ext_t *aux_next = 0x0;
    ldg_math_ext_t *periodic_row = 0x0;
    ldg_math_ext_t *pub_ext = 0x0;
    ldg_math_ext_t acc = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t pwr = LDG_STRUCT_ZERO_INIT;
    ldg_math_field_elem_t domain_root = 0;
    ldg_math_field_elem_t point = 0;

    if (LDG_UNLIKELY(!air_cb || !lde || !comp_out || !ext_buff || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_air_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(conf->periodic_cunt > 0 && !periodic)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->pub_cunt > 0 && !pub)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->aux_col_cunt > 0 && (!aux_lde || !challenge))) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_air_composition_buff_cunt_get(conf, &need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_air_composition_ext_buff_cunt_get(conf, &ext_need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ext_cap < ext_need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_air_domain_cunt_get(conf, &domain_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_domain_cunt_get(conf->log_blowup, &blowup);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_field_root_get(conf->log_trace + conf->log_blowup, &domain_root);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    eval_buff = ext_buff;
    main_curr = eval_buff + conf->constraint_cunt;
    main_next = main_curr + conf->col_cunt;
    aux_curr = main_next + conf->col_cunt;
    aux_next = aux_curr + conf->aux_col_cunt;
    periodic_row = aux_next + conf->aux_col_cunt;
    pub_ext = periodic_row + conf->periodic_cunt;

    for (col = 0; col < conf->pub_cunt; col++) { pub_ext[col] = ldg_math_ext_base_embed(pub[col]); }

    point = ldg_math_field_reduce(conf->coset_shift);
    row.pub = pub_ext;
    row.challenge = challenge;
    row.main_curr = main_curr;
    row.main_next = main_next;

    if (conf->aux_col_cunt > 0)
    {
        row.aux_curr = aux_curr;
        row.aux_next = aux_next;
    }

    if (conf->periodic_cunt > 0) { row.periodic = periodic_row; }

    for (idx = 0; idx < domain_cunt; idx++)
    {
        next_idx = (idx + blowup) & (domain_cunt - LDG_MATH_AIR_PREV_OFF);
        row.point = ldg_math_ext_base_embed(point);

        for (col = 0; col < conf->col_cunt; col++)
        {
            main_curr[col] = ldg_math_ext_base_embed(lde[(idx * conf->col_cunt) + col]);
            main_next[col] = ldg_math_ext_base_embed(lde[(next_idx * conf->col_cunt) + col]);
        }

        for (col = 0; col < conf->aux_col_cunt; col++)
        {
            aux_curr[col] = ldg_math_ext_base_embed(aux_lde[(idx * conf->aux_col_cunt) + col]);
            aux_next[col] = ldg_math_ext_base_embed(aux_lde[(next_idx * conf->aux_col_cunt) + col]);
        }

        for (col = 0; col < conf->periodic_cunt; col++) { periodic_row[col] = ldg_math_ext_base_embed(periodic[(idx * conf->periodic_cunt) + col]); }

        ret = air_cb(air_ctx, &row, eval_buff, conf->constraint_cunt);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        acc = ldg_math_ext_base_embed(LDG_MATH_FIELD_ADD_IDENT);
        pwr = ldg_math_ext_base_embed(LDG_MATH_FIELD_MUL_IDENT);

        for (kdx = 0; kdx < conf->constraint_cunt; kdx++)
        {
            acc = ldg_math_ext_add(acc, ldg_math_ext_mul(eval_buff[kdx], pwr));
            pwr = ldg_math_ext_mul(pwr, beta);
        }

        comp_out[idx] = acc;
        point = ldg_math_field_mul(point, domain_root);
    }

    return ldg_math_proof_ext_vanish_div(comp_out, conf->log_trace + conf->log_blowup, conf->coset_shift, conf->log_trace, buff, cap);
}

uint32_t ldg_math_air_periodic_buff_cunt_get(const ldg_math_air_conf_t *conf, uint64_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t period_cunt = 0;
    uint64_t ext_cunt = 0;
    uint64_t twiddle_period_cunt = 0;
    uint64_t twiddle_ext_cunt = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    ret = ldg_math_air_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (conf->periodic_cunt == 0) { return LDG_ERR_AOK; }

    ret = ldg_math_ntt_domain_cunt_get(conf->log_period, &period_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_domain_cunt_get(conf->log_period + conf->log_blowup, &ext_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_twiddle_cunt_get(conf->log_period, &twiddle_period_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_twiddle_cunt_get(conf->log_period + conf->log_blowup, &twiddle_ext_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = period_cunt + ext_cunt + twiddle_period_cunt + twiddle_ext_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_air_periodic_extend(const ldg_math_air_conf_t *conf, const ldg_math_field_elem_t *periodic, ldg_math_field_elem_t *lde_out, ldg_math_field_elem_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t col = 0;
    uint64_t idx = 0;
    uint64_t need = 0;
    uint64_t period_cunt = 0;
    uint64_t ext_cunt = 0;
    uint64_t domain_cunt = 0;
    uint64_t stride = 0;
    uint64_t twiddle_period_cunt = 0;
    uint64_t twiddle_ext_cunt = 0;
    ldg_math_field_elem_t shift_pwr = 0;
    ldg_math_field_elem_t *work = 0x0;
    ldg_math_field_elem_t *eval = 0x0;
    ldg_math_field_elem_t *twiddle_period_inverse = 0x0;
    ldg_math_field_elem_t *twiddle_ext = 0x0;

    if (LDG_UNLIKELY(!periodic || !lde_out || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_air_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(conf->periodic_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_air_periodic_buff_cunt_get(conf, &need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_ntt_domain_cunt_get(conf->log_period, &period_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_domain_cunt_get(conf->log_period + conf->log_blowup, &ext_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_air_domain_cunt_get(conf, &domain_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_domain_cunt_get(conf->log_trace - conf->log_period, &stride);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_twiddle_cunt_get(conf->log_period, &twiddle_period_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_twiddle_cunt_get(conf->log_period + conf->log_blowup, &twiddle_ext_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    work = buff;
    eval = work + period_cunt;
    twiddle_period_inverse = eval + ext_cunt;
    twiddle_ext = twiddle_period_inverse + twiddle_period_cunt;

    ret = ldg_math_ntt_twiddle_fill(conf->log_period, LDG_TRUTH_TRUE, twiddle_period_inverse, twiddle_period_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_twiddle_fill(conf->log_period + conf->log_blowup, LDG_TRUTH_FALSE, twiddle_ext, twiddle_ext_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    shift_pwr = ldg_math_field_pwr_get(conf->coset_shift, stride);

    for (col = 0; col < conf->periodic_cunt; col++)
    {
        for (idx = 0; idx < period_cunt; idx++) { work[idx] = ldg_math_field_reduce(periodic[(idx * conf->periodic_cunt) + col]); }

        ret = ldg_math_ntt_interpolate(work, conf->log_period, twiddle_period_inverse);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_ntt_extend(work, conf->log_period, eval, conf->log_period + conf->log_blowup, shift_pwr, twiddle_ext);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (idx = 0; idx < domain_cunt; idx++) { lde_out[(idx * conf->periodic_cunt) + col] = eval[idx & (ext_cunt - LDG_MATH_AIR_PREV_OFF)]; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_air_periodic_evaluate(const ldg_math_air_conf_t *conf, const ldg_math_field_elem_t *periodic, ldg_math_ext_t point, ldg_math_ext_t *out, ldg_math_field_elem_t *buff, uint64_t cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t col = 0;
    uint64_t idx = 0;
    uint64_t need = 0;
    uint64_t period_cunt = 0;
    uint64_t stride = 0;
    uint64_t twiddle_period_cunt = 0;
    ldg_math_ext_t lifted = LDG_STRUCT_ZERO_INIT;
    ldg_math_field_elem_t *work = 0x0;
    ldg_math_field_elem_t *twiddle_period_inverse = 0x0;

    if (LDG_UNLIKELY(!periodic || !out || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_air_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(conf->periodic_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_air_periodic_buff_cunt_get(conf, &need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_ntt_domain_cunt_get(conf->log_period, &period_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_domain_cunt_get(conf->log_trace - conf->log_period, &stride);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_twiddle_cunt_get(conf->log_period, &twiddle_period_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    work = buff;
    twiddle_period_inverse = work + period_cunt;

    ret = ldg_math_ntt_twiddle_fill(conf->log_period, LDG_TRUTH_TRUE, twiddle_period_inverse, twiddle_period_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ext_pwr_get(point, stride, &lifted);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (col = 0; col < conf->periodic_cunt; col++)
    {
        for (idx = 0; idx < period_cunt; idx++) { work[idx] = ldg_math_field_reduce(periodic[(idx * conf->periodic_cunt) + col]); }

        ret = ldg_math_ntt_interpolate(work, conf->log_period, twiddle_period_inverse);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_proof_ext_poly_evaluate_get(work, period_cunt, lifted, &out[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}
