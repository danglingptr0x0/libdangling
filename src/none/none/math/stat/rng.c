#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>

#include "rl.h"

#define RNG_NONE 0.0
#define RNG_SHARE_TOTAL 1.0

static uint32_t rng_weight_total_get(const double *weight, uint32_t cunt, double *out)
{
    double total = RNG_NONE;
    uint32_t idx = 0;

    *out = RNG_NONE;

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(weight[idx]) != LDG_TRUTH_TRUE || weight[idx] < RNG_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        total += weight[idx];
        idx++;
    }

    if (LDG_UNLIKELY(ldg_math_finite_is(total) != LDG_TRUTH_TRUE || total <= RNG_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = total;

    return LDG_ERR_AOK;
}

static uint32_t rng_weight_last_get(const double *weight, uint32_t cunt)
{
    uint32_t last = 0;
    uint32_t idx = 0;

    while (idx < cunt)
    {
        if (weight[idx] > RNG_NONE) { last = idx; }

        idx++;
    }

    return last;
}

uint32_t ldg_math_stat_rng_categorical_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, const double *weight, uint32_t cunt, uint32_t *out)
{
    double total = RNG_NONE;
    double unit = RNG_NONE;
    double tgt = RNG_NONE;
    double cum = RNG_NONE;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!rng_cb || !weight || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = rng_weight_total_get(weight, cunt, &total);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_rng_uniform_sample(rng_cb, rng_ctx, &unit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    tgt = unit * total;

    while (idx < cunt)
    {
        cum += weight[idx];

        if (weight[idx] > RNG_NONE && tgt < cum)
        {
            *out = idx;
            return LDG_ERR_AOK;
        }

        idx++;
    }

    *out = rng_weight_last_get(weight, cunt);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_rng_dirichlet_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, const double *alpha, uint32_t cunt, double *dst)
{
    double total = RNG_NONE;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!rng_cb || !alpha || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (idx < cunt)
    {
        dst[idx] = RNG_NONE;
        idx++;
    }

    idx = 0;

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(alpha[idx]) != LDG_TRUTH_TRUE || alpha[idx] <= RNG_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        idx++;
    }

    idx = 0;

    while (idx < cunt)
    {
        ret = ldg_math_stat_rng_gamma_sample(rng_cb, rng_ctx, alpha[idx], dst + idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        total += dst[idx];
        idx++;
    }

    idx = 0;

    while (idx < cunt)
    {
        dst[idx] = (total > RNG_NONE) ? (dst[idx] / total) : (RNG_SHARE_TOTAL / (double)cunt);
        idx++;
    }

    return LDG_ERR_AOK;
}
