
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>

#include "rl.h"

#define LDG_MATH_STAT_RNG_ATTEMPT_MAX 64
#define LDG_MATH_STAT_RNG_STEP 1
#define LDG_MATH_STAT_RNG_MANTISSA_SHIFT 11
#define LDG_MATH_STAT_RNG_MANTISSA_BITS 53
#define LDG_MATH_STAT_RNG_MANTISSA_SCALE ((double)(1ULL << LDG_MATH_STAT_RNG_MANTISSA_BITS))
#define LDG_MATH_STAT_RNG_GAMMA_SQUEEZE 0.0331
#define LDG_MATH_STAT_RNG_GAMMA_SHAPE_OFF (1.0 / 3.0)
#define LDG_MATH_STAT_RNG_GAMMA_SCALE_DENOM 9.0
#define LDG_MATH_STAT_RNG_BETA_DEGENERATE 0.5

uint32_t ldg_math_stat_rng_uniform_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double *out)
{
    uint64_t raw = 0;

    if (LDG_UNLIKELY(!rng_cb || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    raw = rng_cb(rng_ctx);
    *out = (double)(raw >> LDG_MATH_STAT_RNG_MANTISSA_SHIFT) / LDG_MATH_STAT_RNG_MANTISSA_SCALE;

    return LDG_ERR_AOK;
}

static uint32_t rng_uniform_pos_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double *out)
{
    uint32_t attempt = 0;
    uint32_t ret = 0;

    for (attempt = 0; attempt < LDG_MATH_STAT_RNG_ATTEMPT_MAX; attempt++)
    {
        ret = ldg_math_stat_rng_uniform_sample(rng_cb, rng_ctx, out);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (*out > 0.0) { return LDG_ERR_AOK; }
    }

    return LDG_ERR_TIMEOUT;
}

uint32_t ldg_math_stat_rng_normal_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double *out)
{
    double u1 = 0.0;
    double u2 = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!rng_cb || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    ret = rng_uniform_pos_sample(rng_cb, rng_ctx, &u1);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_rng_uniform_sample(rng_cb, rng_ctx, &u2);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = ldg_math_sqrt_get(0.0 - 2.0 * ldg_math_log_get(u1)) * ldg_math_cos_get(LDG_MATH_TAU * u2);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_rng_gamma_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double shape, double *out)
{
    double shape_adj = 0.0;
    double scale = 0.0;
    double norm = 0.0;
    double cand = 0.0;
    double unif = 0.0;
    double boost_unif = 0.0;
    double boost = 1.0;
    uint32_t attempt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!rng_cb || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    if (LDG_UNLIKELY(!(shape > 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (shape < 1.0)
    {
        ret = rng_uniform_pos_sample(rng_cb, rng_ctx, &boost_unif);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        boost = ldg_math_pwr_get(boost_unif, 1.0 / shape);
        shape = shape + 1.0;
    }

    shape_adj = shape - LDG_MATH_STAT_RNG_GAMMA_SHAPE_OFF;
    scale = 1.0 / ldg_math_sqrt_get(LDG_MATH_STAT_RNG_GAMMA_SCALE_DENOM * shape_adj);

    for (attempt = 0; attempt < LDG_MATH_STAT_RNG_ATTEMPT_MAX; attempt++)
    {
        ret = ldg_math_stat_rng_normal_sample(rng_cb, rng_ctx, &norm);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        cand = 1.0 + scale * norm;
        if (cand <= 0.0) { continue; }

        cand = cand * cand * cand;

        ret = rng_uniform_pos_sample(rng_cb, rng_ctx, &unif);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (unif < 1.0 - LDG_MATH_STAT_RNG_GAMMA_SQUEEZE * norm * norm * norm * norm)
        {
            *out = boost * shape_adj * cand;
            return LDG_ERR_AOK;
        }

        if (ldg_math_log_get(unif) < 0.5 * norm * norm + shape_adj * (1.0 - cand + ldg_math_log_get(cand)))
        {
            *out = boost * shape_adj * cand;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_TIMEOUT;
}

uint32_t ldg_math_stat_rng_beta_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double alpha, double beta, double *out)
{
    double ga = 0.0;
    double gb = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!rng_cb || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    ret = ldg_math_stat_rng_gamma_sample(rng_cb, rng_ctx, alpha, &ga);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_rng_gamma_sample(rng_cb, rng_ctx, beta, &gb);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ga + gb <= 0.0))
    {
        *out = LDG_MATH_STAT_RNG_BETA_DEGENERATE;
        return LDG_ERR_AOK;
    }

    *out = ga / (ga + gb);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_rl_bandit_init(ldg_math_stat_rl_bandit_t *bandit, uint32_t arm_cunt, uint32_t strat, double epsilon, double c_const, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx)
{
    uint32_t arm = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bandit)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(arm_cunt == 0 || arm_cunt > LDG_MATH_STAT_RL_BANDIT_ARM_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(strat >= LDG_MATH_STAT_RL_BANDIT_STRAT_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(epsilon < 0.0 || epsilon > 1.0)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY((strat == LDG_MATH_STAT_RL_BANDIT_STRAT_EPSILON_GREEDY || strat == LDG_MATH_STAT_RL_BANDIT_STRAT_THOMPSON_BERNOULLI || strat == LDG_MATH_STAT_RL_BANDIT_STRAT_THOMPSON_GAUSSIAN) && !rng_cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_zero(bandit, sizeof(ldg_math_stat_rl_bandit_t)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    for (arm = 0; arm < arm_cunt; arm++)
    {
        ret = ldg_math_stat_welford_init(&bandit->arm[arm].welford);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        bandit->arm[arm].alpha = 1.0;
        bandit->arm[arm].beta = 1.0;
    }
    bandit->arm_cunt = arm_cunt;
    bandit->strat = strat;
    bandit->epsilon = epsilon;
    bandit->c_const = c_const;
    bandit->rng_cb = rng_cb;
    bandit->rng_ctx = rng_ctx;
    bandit->is_init = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_rl_bandit_sel(ldg_math_stat_rl_bandit_t *bandit, uint32_t *out)
{
    uint32_t arm = 0;
    uint32_t best = 0;
    double best_val = 0.0;
    double val = 0.0;
    double mean = 0.0;
    double sigma = 0.0;
    double draw = 0.0;
    double pick = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bandit || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bandit->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY((bandit->strat == LDG_MATH_STAT_RL_BANDIT_STRAT_EPSILON_GREEDY || bandit->strat == LDG_MATH_STAT_RL_BANDIT_STRAT_THOMPSON_BERNOULLI || bandit->strat == LDG_MATH_STAT_RL_BANDIT_STRAT_THOMPSON_GAUSSIAN) && !bandit->rng_cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    bandit->step_cunt++;

    for (arm = 0; arm < bandit->arm_cunt; arm++) { if (bandit->arm[arm].pull_cunt == 0) { *out = arm; return LDG_ERR_AOK; } }

    if (bandit->strat == LDG_MATH_STAT_RL_BANDIT_STRAT_EPSILON_GREEDY)
    {
        ret = ldg_math_stat_rng_uniform_sample(bandit->rng_cb, bandit->rng_ctx, &draw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (draw < bandit->epsilon)
        {
            ret = ldg_math_stat_rng_uniform_sample(bandit->rng_cb, bandit->rng_ctx, &pick);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            *out = (uint32_t)(pick * (double)bandit->arm_cunt);
            if (*out >= bandit->arm_cunt) { *out = bandit->arm_cunt - LDG_MATH_STAT_RNG_STEP; }

            return LDG_ERR_AOK;
        }
    }

    for (arm = 0; arm < bandit->arm_cunt; arm++)
    {
        if (bandit->strat == LDG_MATH_STAT_RL_BANDIT_STRAT_UCB1)
        {
            mean = bandit->arm[arm].welford.mean;
            val = mean + bandit->c_const * ldg_math_sqrt_get(ldg_math_log_get((double)bandit->step_cunt) / (double)bandit->arm[arm].pull_cunt);
        }
        else if (bandit->strat == LDG_MATH_STAT_RL_BANDIT_STRAT_THOMPSON_BERNOULLI)
        {
            ret = ldg_math_stat_rng_beta_sample(bandit->rng_cb, bandit->rng_ctx, bandit->arm[arm].alpha + bandit->arm[arm].succ_cunt, bandit->arm[arm].beta + (double)bandit->arm[arm].pull_cunt - bandit->arm[arm].succ_cunt, &val);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
        else if (bandit->strat == LDG_MATH_STAT_RL_BANDIT_STRAT_THOMPSON_GAUSSIAN)
        {
            mean = bandit->arm[arm].welford.mean;
            if (bandit->arm[arm].pull_cunt < 2) { sigma = 1.0; }
            else { sigma = ldg_math_sqrt_get(bandit->arm[arm].welford.m2 / (double)(bandit->arm[arm].pull_cunt - LDG_MATH_STAT_RNG_STEP)); }

            ret = ldg_math_stat_rng_normal_sample(bandit->rng_cb, bandit->rng_ctx, &draw);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            val = mean + sigma * draw;
        }
        else { val = bandit->arm[arm].welford.mean; }

        if (arm == 0 || val > best_val) { best = arm; best_val = val; }
    }

    *out = best;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_rl_bandit_observe(ldg_math_stat_rl_bandit_t *bandit, uint32_t arm, double reward)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bandit)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bandit->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(arm >= bandit->arm_cunt)) { return LDG_ERR_MATH_STAT_RL_ARM_INVALID; }

    ret = ldg_math_stat_welford_observe(&bandit->arm[arm].welford, reward);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    bandit->arm[arm].pull_cunt++;
    if (bandit->strat == LDG_MATH_STAT_RL_BANDIT_STRAT_THOMPSON_BERNOULLI) { bandit->arm[arm].succ_cunt += reward; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_rl_bandit_arm_mean_get(const ldg_math_stat_rl_bandit_t *bandit, uint32_t arm, double *out)
{
    if (LDG_UNLIKELY(!bandit || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(bandit->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(arm >= bandit->arm_cunt)) { return LDG_ERR_MATH_STAT_RL_ARM_INVALID; }

    *out = bandit->arm[arm].welford.mean;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_math_stat_rl_bandit_arm_pull_get(const ldg_math_stat_rl_bandit_t *bandit, uint32_t arm)
{
    if (LDG_UNLIKELY(!bandit || bandit->is_init != LDG_TRUTH_TRUE || arm >= bandit->arm_cunt)) { return UINT64_MAX; }

    return bandit->arm[arm].pull_cunt;
}

LDG_EXPORT uint32_t ldg_math_stat_rl_bandit_rst(ldg_math_stat_rl_bandit_t *bandit)
{
    uint32_t arm = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bandit)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bandit->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    for (arm = 0; arm < bandit->arm_cunt; arm++)
    {
        ret = ldg_math_stat_welford_rst(&bandit->arm[arm].welford);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        bandit->arm[arm].pull_cunt = 0;
        bandit->arm[arm].succ_cunt = 0.0;
        bandit->arm[arm].alpha = 1.0;
        bandit->arm[arm].beta = 1.0;
    }
    bandit->step_cunt = 0;
    return LDG_ERR_AOK;
}
