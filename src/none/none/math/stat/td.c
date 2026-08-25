#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "rl.h"

#define LDG_MATH_STAT_TD_STEP 1

static double ldg_math_stat_td_q_max_pick(const ldg_math_stat_td_t *td, uint32_t state)
{
    uint32_t action = 0;
    double best = 0.0;
    double val = 0.0;

    best = td->q_arr[(uint64_t)state * td->action_max + 0];
    for (action = 1; action < td->action_max; action++)
    {
        val = td->q_arr[(uint64_t)state * td->action_max + action];
        if (val > best) { best = val; }
    }
    return best;
}

static uint32_t ldg_math_stat_td_action_best_pick(const ldg_math_stat_td_t *td, uint32_t state, uint32_t *out)
{
    uint32_t action = 0;
    uint32_t best = 0;
    double best_val = 0.0;
    double val = 0.0;

    best = 0;
    best_val = td->q_arr[(uint64_t)state * td->action_max + 0];
    for (action = 1; action < td->action_max; action++)
    {
        val = td->q_arr[(uint64_t)state * td->action_max + action];
        if (val > best_val) { best = action; best_val = val; }
    }
    *out = best;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_td_init(ldg_math_stat_td_t *td, uint32_t state_max, uint32_t action_max, uint32_t strat, double alpha, double gamma, double lambda, double epsilon, double q_init, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx)
{
    uint64_t cells = 0;
    uint64_t bytes = 0;
    uint64_t idx = 0;
    void *q_ptr = 0x0;
    void *trace_ptr = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!td)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(state_max == 0 || action_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cells = (uint64_t)state_max * (uint64_t)action_max;
    if (LDG_UNLIKELY(cells > LDG_MATH_STAT_TD_CELL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(strat >= LDG_MATH_STAT_TD_STRAT_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(alpha <= 0.0 || alpha > 1.0)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY(gamma < 0.0 || gamma > 1.0)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY(lambda < 0.0 || lambda > 1.0)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY(epsilon < 0.0 || epsilon > 1.0)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY(!rng_cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    bytes = cells * (uint64_t)sizeof(double);
    ret = ldg_mem_alloc(bytes, &q_ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_alloc(bytes, &trace_ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { ldg_mem_dealloc(q_ptr); return ret; }

    td->q_arr = (double *)q_ptr;
    td->trace = (double *)trace_ptr;
    for (idx = 0; idx < cells; idx++) { td->q_arr[idx] = q_init; td->trace[idx] = 0.0; }
    td->rng_cb = rng_cb;
    td->rng_ctx = rng_ctx;
    td->state_max = state_max;
    td->action_max = action_max;
    td->strat = strat;
    td->alpha = alpha;
    td->gamma = gamma;
    td->lambda = lambda;
    td->epsilon = epsilon;
    td->q_init = q_init;
    td->last_greedy = 0;
    td->is_init = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_td_shutdown(ldg_math_stat_td_t *td)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!td)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(td->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mem_dealloc(td->q_arr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_dealloc(td->trace);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    td->q_arr = 0x0;
    td->trace = 0x0;
    td->is_init = LDG_TRUTH_FALSE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_td_action_sel(ldg_math_stat_td_t *td, uint32_t state, uint32_t *out)
{
    double draw = 0.0;
    double pick = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!td || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(td->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(!td->rng_cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(state >= td->state_max)) { return LDG_ERR_MATH_STAT_TD_STATE_INVALID; }

    ret = ldg_math_stat_rng_uniform_sample(td->rng_cb, td->rng_ctx, &draw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (draw < td->epsilon)
    {
        ret = ldg_math_stat_rng_uniform_sample(td->rng_cb, td->rng_ctx, &pick);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *out = (uint32_t)(pick * (double)td->action_max);
        if (*out >= td->action_max) { *out = td->action_max - LDG_MATH_STAT_TD_STEP; }

        td->last_greedy = LDG_TRUTH_FALSE;
        return LDG_ERR_AOK;
    }

    ret = ldg_math_stat_td_action_best_pick(td, state, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    td->last_greedy = 1;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_td_observe(ldg_math_stat_td_t *td, uint32_t state, uint32_t action, double reward, uint32_t next_state, uint32_t next_action, uint8_t terminal)
{
    uint64_t cells = 0;
    uint64_t idx = 0;
    uint64_t next_idx = 0;
    uint64_t cell = 0;
    double q_sa = 0.0;
    double tgt = 0.0;
    double delta = 0.0;
    double q_max_next = 0.0;

    if (LDG_UNLIKELY(!td)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(td->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(state >= td->state_max)) { return LDG_ERR_MATH_STAT_TD_STATE_INVALID; }

    if (LDG_UNLIKELY(action >= td->action_max)) { return LDG_ERR_MATH_STAT_TD_ACTION_INVALID; }

    if (LDG_UNLIKELY(!terminal && next_state >= td->state_max)) { return LDG_ERR_MATH_STAT_TD_STATE_INVALID; }

    if (LDG_UNLIKELY(!terminal && next_action >= td->action_max)) { return LDG_ERR_MATH_STAT_TD_ACTION_INVALID; }

    cells = (uint64_t)td->state_max * td->action_max;
    idx = (uint64_t)state * td->action_max + action;
    q_sa = td->q_arr[idx];

    if (terminal) { tgt = reward; }
    else if (td->strat == LDG_MATH_STAT_TD_STRAT_Q_LEARNING || td->strat == LDG_MATH_STAT_TD_STRAT_Q_LAMBDA)
    {
        q_max_next = ldg_math_stat_td_q_max_pick(td, next_state);
        tgt = reward + td->gamma * q_max_next;
    }
    else
    {
        next_idx = (uint64_t)next_state * td->action_max + next_action;
        tgt = reward + td->gamma * td->q_arr[next_idx];
    }

    delta = tgt - q_sa;

    if (td->strat == LDG_MATH_STAT_TD_STRAT_Q_LAMBDA || td->strat == LDG_MATH_STAT_TD_STRAT_SARSA_LAMBDA)
    {
        td->trace[idx] = 1.0;
        for (cell = 0; cell < cells; cell++) { td->q_arr[cell] += td->alpha * delta * td->trace[cell]; }
        if (terminal) { for (cell = 0; cell < cells; cell++) { td->trace[cell] = 0.0; } }
        else if (td->strat == LDG_MATH_STAT_TD_STRAT_Q_LAMBDA) { if (td->last_greedy) { for (cell = 0; cell < cells; cell++) { td->trace[cell] *= td->gamma * td->lambda; } }
            else { for (cell = 0; cell < cells; cell++) { td->trace[cell] = 0.0; } } }
        else { for (cell = 0; cell < cells; cell++) { td->trace[cell] *= td->gamma * td->lambda; } }
    }
    else { td->q_arr[idx] += td->alpha * delta; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_td_q_get(const ldg_math_stat_td_t *td, uint32_t state, uint32_t action, double *out)
{
    if (LDG_UNLIKELY(!td || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(td->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(state >= td->state_max)) { return LDG_ERR_MATH_STAT_TD_STATE_INVALID; }

    if (LDG_UNLIKELY(action >= td->action_max)) { return LDG_ERR_MATH_STAT_TD_ACTION_INVALID; }

    *out = td->q_arr[(uint64_t)state * td->action_max + action];
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_td_action_best_get(const ldg_math_stat_td_t *td, uint32_t state, uint32_t *out)
{
    if (LDG_UNLIKELY(!td || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(td->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(state >= td->state_max)) { return LDG_ERR_MATH_STAT_TD_STATE_INVALID; }

    return ldg_math_stat_td_action_best_pick(td, state, out);
}

LDG_EXPORT uint32_t ldg_math_stat_td_q_max_get(const ldg_math_stat_td_t *td, uint32_t state, double *out)
{
    if (LDG_UNLIKELY(!td || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(td->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(state >= td->state_max)) { return LDG_ERR_MATH_STAT_TD_STATE_INVALID; }

    *out = ldg_math_stat_td_q_max_pick(td, state);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_td_alpha_set(ldg_math_stat_td_t *td, double alpha)
{
    if (LDG_UNLIKELY(!td)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(td->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(alpha <= 0.0 || alpha > 1.0)) { return LDG_ERR_RANGE; }

    td->alpha = alpha;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_td_epsilon_set(ldg_math_stat_td_t *td, double epsilon)
{
    if (LDG_UNLIKELY(!td)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(td->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(epsilon < 0.0 || epsilon > 1.0)) { return LDG_ERR_RANGE; }

    td->epsilon = epsilon;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_td_lambda_set(ldg_math_stat_td_t *td, double lambda)
{
    if (LDG_UNLIKELY(!td)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(td->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(lambda < 0.0 || lambda > 1.0)) { return LDG_ERR_RANGE; }

    td->lambda = lambda;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_td_rst(ldg_math_stat_td_t *td)
{
    uint64_t cells = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!td)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(td->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    cells = (uint64_t)td->state_max * td->action_max;
    for (idx = 0; idx < cells; idx++)
    {
        td->q_arr[idx] = td->q_init;
        td->trace[idx] = 0.0;
    }
    td->last_greedy = 0;
    return LDG_ERR_AOK;
}
