
#include <dangling/core/err.h>
#include <dangling/math/math.h>

#include "welford.h"

void ldg_math_stat_welford_m2_update(uint64_t *cunt, double *mean, double *m2, double val)
{
    uint64_t old_cunt = 0;
    double delta = 0.0;
    old_cunt = *cunt;
    delta = val - *mean;
    *mean += delta / (double)(old_cunt + 1);
    *m2 += delta * (val - *mean);
    *cunt = old_cunt + 1;
}

uint32_t ldg_math_stat_welford_init(ldg_math_stat_welford_t *acc)
{
    if (LDG_UNLIKELY(!acc)) { return LDG_ERR_FUNC_ARG_NULL; }

    acc->cunt = 0;
    acc->mean = 0.0;
    acc->m2 = 0.0;
    acc->m3 = 0.0;
    acc->m4 = 0.0;
    acc->min = LDG_MATH_INFINITY;
    acc->max = 0.0 - LDG_MATH_INFINITY;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_welford_observe(ldg_math_stat_welford_t *acc, double val)
{
    double old_cunt = 0.0;
    double new_cunt = 0.0;
    double delta = 0.0;
    if (LDG_UNLIKELY(!acc)) { return LDG_ERR_FUNC_ARG_NULL; }

    old_cunt = (double)acc->cunt;
    new_cunt = (double)(acc->cunt + 1);
    delta = val - acc->mean;
    acc->m4 += delta * delta * delta * delta * old_cunt * (old_cunt * old_cunt - old_cunt + 1.0) / (new_cunt * new_cunt * new_cunt) + 6.0 * delta * delta * acc->m2 * old_cunt / (new_cunt * new_cunt) - 4.0 * delta * acc->m3 / new_cunt;
    acc->m3 += delta * delta * delta * old_cunt * (old_cunt - 1.0) / (new_cunt * new_cunt) - 3.0 * delta * acc->m2 / new_cunt;
    ldg_math_stat_welford_m2_update(&acc->cunt, &acc->mean, &acc->m2, val);
    if (val < acc->min) { acc->min = val; }

    if (val > acc->max) { acc->max = val; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_welford_mean_get(const ldg_math_stat_welford_t *acc, double *out)
{
    if (LDG_UNLIKELY(!acc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = acc->mean;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_welford_var_get(const ldg_math_stat_welford_t *acc, double *out)
{
    if (LDG_UNLIKELY(!acc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(acc->cunt < 2)) { return LDG_ERR_EMPTY; }

    *out = acc->m2 / (double)(acc->cunt - 1);
    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_welford_sigma_get(const ldg_math_stat_welford_t *acc, double *out)
{
    if (LDG_UNLIKELY(!acc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(acc->cunt < 2)) { return LDG_ERR_EMPTY; }

    *out = ldg_math_sqrt_get(acc->m2 / (double)(acc->cunt - 1));
    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_welford_skew_get(const ldg_math_stat_welford_t *acc, double *out)
{
    double obs_cunt = 0.0;
    if (LDG_UNLIKELY(!acc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(acc->m2 <= 0.0)) { return LDG_ERR_AOK; }

    obs_cunt = (double)acc->cunt;
    *out = ldg_math_sqrt_get(obs_cunt) * acc->m3 / (acc->m2 * ldg_math_sqrt_get(acc->m2));
    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_welford_kurt_get(const ldg_math_stat_welford_t *acc, double *out)
{
    double obs_cunt = 0.0;
    if (LDG_UNLIKELY(!acc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(acc->m2 <= 0.0)) { return LDG_ERR_AOK; }

    obs_cunt = (double)acc->cunt;
    *out = obs_cunt * acc->m4 / (acc->m2 * acc->m2) - 3.0;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_welford_min_get(const ldg_math_stat_welford_t *acc, double *out)
{
    if (LDG_UNLIKELY(!acc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = acc->min;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_welford_max_get(const ldg_math_stat_welford_t *acc, double *out)
{
    if (LDG_UNLIKELY(!acc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = acc->max;
    return LDG_ERR_AOK;
}

uint64_t ldg_math_stat_welford_cunt_get(const ldg_math_stat_welford_t *acc)
{
    if (LDG_UNLIKELY(!acc)) { return UINT64_MAX; }

    return acc->cunt;
}

uint32_t ldg_math_stat_welford_rst(ldg_math_stat_welford_t *acc)
{
    if (LDG_UNLIKELY(!acc)) { return LDG_ERR_FUNC_ARG_NULL; }

    acc->cunt = 0;
    acc->mean = 0.0;
    acc->m2 = 0.0;
    acc->m3 = 0.0;
    acc->m4 = 0.0;
    acc->min = LDG_MATH_INFINITY;
    acc->max = 0.0 - LDG_MATH_INFINITY;
    return LDG_ERR_AOK;
}
