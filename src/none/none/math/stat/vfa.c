#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "rl.h"

#define LDG_MATH_STAT_VFA_STEP 1

static uint32_t ldg_math_stat_vfa_q_pick(const ldg_math_stat_vfa_t *vfa, const double *state, uint32_t action, double *out)
{
    uint32_t active_idx[LDG_MATH_STAT_TILE_TILING_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t tiling = 0;
    uint32_t ret = 0;
    double sum = 0.0;

    ret = ldg_math_stat_tile_feature_compute(&vfa->tile, state, action, active_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { *out = 0.0; return ret; }

    for (tiling = 0; tiling < vfa->tile.tiling_cunt; tiling++) { sum += vfa->weight[active_idx[tiling]]; }
    *out = sum;
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_stat_vfa_q_max_pick(const ldg_math_stat_vfa_t *vfa, const double *state, double *out)
{
    uint32_t action = 0;
    uint32_t ret = 0;
    double best = 0.0;
    double val = 0.0;

    ret = ldg_math_stat_vfa_q_pick(vfa, state, 0, &best);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { *out = 0.0; return ret; }

    for (action = 1; action < vfa->tile.action_max; action++)
    {
        ret = ldg_math_stat_vfa_q_pick(vfa, state, action, &val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { *out = 0.0; return ret; }

        if (val > best) { best = val; }
    }
    *out = best;
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_stat_vfa_action_best_pick(const ldg_math_stat_vfa_t *vfa, const double *state, uint32_t *out)
{
    uint32_t action = 0;
    uint32_t best = 0;
    uint32_t ret = 0;
    double best_val = 0.0;
    double val = 0.0;

    best = 0;
    ret = ldg_math_stat_vfa_q_pick(vfa, state, 0, &best_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { *out = 0; return ret; }

    for (action = 1; action < vfa->tile.action_max; action++)
    {
        ret = ldg_math_stat_vfa_q_pick(vfa, state, action, &val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { *out = 0; return ret; }

        if (val > best_val) { best = action; best_val = val; }
    }
    *out = best;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_vfa_init(ldg_math_stat_vfa_t *vfa, const ldg_math_stat_tile_t *tile, uint32_t strat, double alpha, double gamma, double lambda, double epsilon, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx)
{
    uint64_t bytes = 0;
    uint64_t idx = 0;
    void *weight_ptr = 0x0;
    void *trace_ptr = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!vfa || !tile)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(tile->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(strat >= LDG_MATH_STAT_VFA_STRAT_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(alpha <= 0.0 || alpha > 1.0)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY(gamma < 0.0 || gamma > 1.0)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY(lambda < 0.0 || lambda > 1.0)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY(epsilon < 0.0 || epsilon > 1.0)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY(!rng_cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    bytes = (uint64_t)tile->weight_cap * (uint64_t)sizeof(double);
    ret = ldg_mem_alloc(bytes, &weight_ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_alloc(bytes, &trace_ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_mem_dealloc(weight_ptr); return ret; }

    vfa->tile = *tile;
    vfa->weight = (double *)weight_ptr;
    vfa->trace = (double *)trace_ptr;
    for (idx = 0; idx < tile->weight_cap; idx++)
    {
        vfa->weight[idx] = 0.0;
        vfa->trace[idx] = 0.0;
    }
    vfa->weight_cap = tile->weight_cap;
    vfa->strat = strat;
    vfa->alpha = alpha;
    vfa->gamma = gamma;
    vfa->lambda = lambda;
    vfa->epsilon = epsilon;
    vfa->rng_cb = rng_cb;
    vfa->rng_ctx = rng_ctx;
    vfa->last_greedy = 0;
    vfa->is_init = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_vfa_shutdown(ldg_math_stat_vfa_t *vfa)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!vfa)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(vfa->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mem_dealloc(vfa->weight);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_dealloc(vfa->trace);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    vfa->weight = 0x0;
    vfa->trace = 0x0;
    vfa->is_init = LDG_TRUTH_FALSE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_vfa_action_sel(ldg_math_stat_vfa_t *vfa, const double *state, uint32_t *out)
{
    double draw = 0.0;
    double pick = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!vfa || !state || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(vfa->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(!vfa->rng_cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_stat_rng_uniform_sample(vfa->rng_cb, vfa->rng_ctx, &draw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (draw < vfa->epsilon)
    {
        ret = ldg_math_stat_rng_uniform_sample(vfa->rng_cb, vfa->rng_ctx, &pick);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *out = (uint32_t)(pick * (double)vfa->tile.action_max);
        if (*out >= vfa->tile.action_max) { *out = vfa->tile.action_max - LDG_MATH_STAT_VFA_STEP; }

        vfa->last_greedy = LDG_TRUTH_FALSE;
        return LDG_ERR_AOK;
    }

    ret = ldg_math_stat_vfa_action_best_pick(vfa, state, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    vfa->last_greedy = 1;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_vfa_observe(ldg_math_stat_vfa_t *vfa, const double *state, uint32_t action, double reward, const double *next_state, uint32_t next_action, uint8_t terminal)
{
    uint32_t active_idx[LDG_MATH_STAT_TILE_TILING_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t tiling = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;
    double q_sa = 0.0;
    double tgt = 0.0;
    double delta = 0.0;
    double q_max_next = 0.0;
    double q_next = 0.0;

    if (LDG_UNLIKELY(!vfa || !state)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(vfa->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(action >= vfa->tile.action_max)) { return LDG_ERR_MATH_STAT_VFA_FEAT_INVALID; }

    if (LDG_UNLIKELY(!terminal && !next_state)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!terminal && next_action >= vfa->tile.action_max)) { return LDG_ERR_MATH_STAT_VFA_FEAT_INVALID; }

    ret = ldg_math_stat_vfa_q_pick(vfa, state, action, &q_sa);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (terminal) { tgt = reward; }
    else if (vfa->strat == LDG_MATH_STAT_VFA_STRAT_Q_LEARNING || vfa->strat == LDG_MATH_STAT_VFA_STRAT_Q_LAMBDA)
    {
        ret = ldg_math_stat_vfa_q_max_pick(vfa, next_state, &q_max_next);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        tgt = reward + vfa->gamma * q_max_next;
    }
    else
    {
        ret = ldg_math_stat_vfa_q_pick(vfa, next_state, next_action, &q_next);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        tgt = reward + vfa->gamma * q_next;
    }

    delta = tgt - q_sa;

    ret = ldg_math_stat_tile_feature_compute(&vfa->tile, state, action, active_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (vfa->strat == LDG_MATH_STAT_VFA_STRAT_Q_LAMBDA || vfa->strat == LDG_MATH_STAT_VFA_STRAT_SARSA_LAMBDA)
    {
        for (tiling = 0; tiling < vfa->tile.tiling_cunt; tiling++) { vfa->trace[active_idx[tiling]] += 1.0; }
        for (idx = 0; idx < vfa->weight_cap; idx++) { vfa->weight[idx] += vfa->alpha * delta * vfa->trace[idx]; }
        if (terminal) { for (idx = 0; idx < vfa->weight_cap; idx++) { vfa->trace[idx] = 0.0; } }
        else if (vfa->strat == LDG_MATH_STAT_VFA_STRAT_Q_LAMBDA) { if (vfa->last_greedy) { for (idx = 0; idx < vfa->weight_cap; idx++) { vfa->trace[idx] *= vfa->gamma * vfa->lambda; } }
            else { for (idx = 0; idx < vfa->weight_cap; idx++) { vfa->trace[idx] = 0.0; } } }
        else { for (idx = 0; idx < vfa->weight_cap; idx++) { vfa->trace[idx] *= vfa->gamma * vfa->lambda; } }
    }
    else { for (tiling = 0; tiling < vfa->tile.tiling_cunt; tiling++) { vfa->weight[active_idx[tiling]] += vfa->alpha * delta; } }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_vfa_q_get(const ldg_math_stat_vfa_t *vfa, const double *state, uint32_t action, double *out)
{
    if (LDG_UNLIKELY(!vfa || !state || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(vfa->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(action >= vfa->tile.action_max)) { return LDG_ERR_MATH_STAT_VFA_FEAT_INVALID; }

    return ldg_math_stat_vfa_q_pick(vfa, state, action, out);
}

LDG_EXPORT uint32_t ldg_math_stat_vfa_action_best_get(const ldg_math_stat_vfa_t *vfa, const double *state, uint32_t *out)
{
    if (LDG_UNLIKELY(!vfa || !state || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(vfa->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_math_stat_vfa_action_best_pick(vfa, state, out);
}

LDG_EXPORT uint32_t ldg_math_stat_vfa_q_max_get(const ldg_math_stat_vfa_t *vfa, const double *state, double *out)
{
    if (LDG_UNLIKELY(!vfa || !state || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(vfa->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_math_stat_vfa_q_max_pick(vfa, state, out);
}

LDG_EXPORT uint32_t ldg_math_stat_vfa_rst(ldg_math_stat_vfa_t *vfa)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!vfa)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(vfa->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < vfa->weight_cap; idx++)
    {
        vfa->weight[idx] = 0.0;
        vfa->trace[idx] = 0.0;
    }
    vfa->last_greedy = 0;
    return LDG_ERR_AOK;
}
