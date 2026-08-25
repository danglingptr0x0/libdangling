
#include <dangling/arch/fence.h>
#include <dangling/arch/atomic.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

#include "bayes.h"

#define LDG_MATH_STAT_BAYES_LOCK_SPIN_MAX 65536
#define LDG_MATH_STAT_BAYES_LOCK_FREE 0
#define LDG_MATH_STAT_BAYES_LOCK_HELD 1

static uint32_t ldg_math_stat_bayes_lock(ldg_math_stat_bayes_t *bayes)
{
    uint32_t expected = 0;
    uint32_t spin = 0;

    for (spin = 0; spin < LDG_MATH_STAT_BAYES_LOCK_SPIN_MAX; spin++)
    {
        expected = LDG_MATH_STAT_BAYES_LOCK_FREE;
        if (LDG_CAS(&bayes->lock, &expected, LDG_MATH_STAT_BAYES_LOCK_HELD)) { return LDG_ERR_AOK; }

        LDG_PAUSE;
    }

    return LDG_ERR_BUSY;
}

static void ldg_math_stat_bayes_predictive(const ldg_math_stat_bayes_t *bayes, const ldg_math_stat_bayes_kind_t *kind, double *out_loc, double *out_var)
{
    double obs = 0.0;
    double xbar = 0.0;
    double ssd = 0.0;
    double kappa_n = 0.0;
    double m_n = 0.0;
    double alpha_n = 0.0;
    double beta_n = 0.0;
    double dof = 0.0;
    double scale_sq = 0.0;
    double var = 0.0;
    double diff = 0.0;

    obs = (double)kind->obs_cunt;
    xbar = kind->mean;
    ssd = kind->m2;

    kappa_n = bayes->prior_kappa + obs;
    m_n = (bayes->prior_kappa * bayes->prior_m + obs * xbar) / kappa_n;
    alpha_n = bayes->prior_alpha + obs / 2.0;
    diff = xbar - bayes->prior_m;
    beta_n = bayes->prior_beta + 0.5 * ssd + (bayes->prior_kappa * obs * diff * diff) / (2.0 * kappa_n);

    dof = 2.0 * alpha_n;
    scale_sq = beta_n * (kappa_n + 1.0) / (alpha_n * kappa_n);

    if (dof > 2.0) { var = scale_sq * dof / (dof - 2.0); }

    else { var = scale_sq; }

    if (var < 0.0) { var = 0.0; }

    *out_loc = m_n;
    *out_var = var;
}

static uint32_t ldg_math_stat_bayes_moments(ldg_math_stat_bayes_t *bayes, uint16_t kind, double *out_loc, double *out_var)
{
    ldg_math_stat_bayes_kind_t snap = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    ret = ldg_math_stat_bayes_lock(bayes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    snap = bayes->kind[kind];

    LDG_ST_RELEASE(bayes->lock, LDG_MATH_STAT_BAYES_LOCK_FREE);

    ldg_math_stat_bayes_predictive(bayes, &snap, out_loc, out_var);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_bayes_init(ldg_math_stat_bayes_t *bayes, double prior_m, double prior_kappa, double prior_alpha, double prior_beta, double risk_k)
{
    if (LDG_UNLIKELY(!bayes)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(prior_kappa <= 0.0)) { return LDG_ERR_MATH_STAT_PRIOR_INVALID; }

    if (LDG_UNLIKELY(prior_alpha <= 0.0)) { return LDG_ERR_MATH_STAT_PRIOR_INVALID; }

    if (LDG_UNLIKELY(prior_beta <= 0.0)) { return LDG_ERR_MATH_STAT_PRIOR_INVALID; }

    if (LDG_UNLIKELY(ldg_mem_zero(bayes, sizeof(ldg_math_stat_bayes_t)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    bayes->prior_m = prior_m;
    bayes->prior_kappa = prior_kappa;
    bayes->prior_alpha = prior_alpha;
    bayes->prior_beta = prior_beta;
    bayes->risk_k = risk_k;
    bayes->lock = 0;
    bayes->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_bayes_shutdown(ldg_math_stat_bayes_t *bayes)
{
    if (LDG_UNLIKELY(!bayes)) { return LDG_ERR_FUNC_ARG_NULL; }

    bayes->is_init = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_bayes_observe(ldg_math_stat_bayes_t *bayes, uint16_t kind, double duration)
{
    ldg_math_stat_bayes_kind_t *kind_entry = 0x0;
    double delta = 0.0;
    double delta2 = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bayes)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bayes->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(kind >= LDG_MATH_STAT_BAYES_KIND_MAX)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY(duration < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_stat_bayes_lock(bayes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    kind_entry = &bayes->kind[kind];

    kind_entry->obs_cunt++;
    delta = duration - kind_entry->mean;
    kind_entry->mean += delta / (double)kind_entry->obs_cunt;
    delta2 = duration - kind_entry->mean;
    kind_entry->m2 += delta * delta2;
    kind_entry->last = duration;

    bayes->total_obs++;

    LDG_ST_RELEASE(bayes->lock, LDG_MATH_STAT_BAYES_LOCK_FREE);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_bayes_proj_k(ldg_math_stat_bayes_t *bayes, uint16_t kind, double risk_k, double *out)
{
    double loc = 0.0;
    double var = 0.0;
    double sigma = 0.0;
    double res = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bayes || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(bayes->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(kind >= LDG_MATH_STAT_BAYES_KIND_MAX)) { return LDG_ERR_RANGE; }

    ret = ldg_math_stat_bayes_moments(bayes, kind, &loc, &var);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sigma = __builtin_sqrt(var);

    res = loc + risk_k * sigma;
    if (res < 0.0) { res = 0.0; }

    *out = res;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_bayes_proj(ldg_math_stat_bayes_t *bayes, uint16_t kind, double *out)
{
    double risk_k = 0.0;

    if (LDG_UNLIKELY(!bayes || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    risk_k = bayes->risk_k;

    return ldg_math_stat_bayes_proj_k(bayes, kind, risk_k, out);
}

LDG_EXPORT uint32_t ldg_math_stat_bayes_mean_get(ldg_math_stat_bayes_t *bayes, uint16_t kind, double *out)
{
    double loc = 0.0;
    double var = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bayes || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(bayes->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(kind >= LDG_MATH_STAT_BAYES_KIND_MAX)) { return LDG_ERR_RANGE; }

    ret = ldg_math_stat_bayes_moments(bayes, kind, &loc, &var);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = loc;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_bayes_var_get(ldg_math_stat_bayes_t *bayes, uint16_t kind, double *out)
{
    double loc = 0.0;
    double var = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bayes || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(bayes->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(kind >= LDG_MATH_STAT_BAYES_KIND_MAX)) { return LDG_ERR_RANGE; }

    ret = ldg_math_stat_bayes_moments(bayes, kind, &loc, &var);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = var;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_bayes_sigma_get(ldg_math_stat_bayes_t *bayes, uint16_t kind, double *out)
{
    double loc = 0.0;
    double var = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bayes || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(bayes->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(kind >= LDG_MATH_STAT_BAYES_KIND_MAX)) { return LDG_ERR_RANGE; }

    ret = ldg_math_stat_bayes_moments(bayes, kind, &loc, &var);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = __builtin_sqrt(var);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_math_stat_bayes_obs_cunt_get(ldg_math_stat_bayes_t *bayes, uint16_t kind)
{
    uint64_t res = 0;

    if (LDG_UNLIKELY(!bayes || bayes->is_init != LDG_TRUTH_TRUE))
    {
        res = UINT64_MAX;
        return res;
    }

    if (LDG_UNLIKELY(kind >= LDG_MATH_STAT_BAYES_KIND_MAX))
    {
        res = UINT64_MAX;
        return res;
    }

    if (LDG_UNLIKELY(ldg_math_stat_bayes_lock(bayes) != LDG_ERR_AOK))
    {
        res = UINT64_MAX;
        return res;
    }

    res = bayes->kind[kind].obs_cunt;

    LDG_ST_RELEASE(bayes->lock, LDG_MATH_STAT_BAYES_LOCK_FREE);

    return res;
}

LDG_EXPORT uint32_t ldg_math_stat_bayes_risk_k_set(ldg_math_stat_bayes_t *bayes, double risk_k)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bayes)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bayes->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_math_stat_bayes_lock(bayes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    bayes->risk_k = risk_k;
    LDG_ST_RELEASE(bayes->lock, LDG_MATH_STAT_BAYES_LOCK_FREE);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_bayes_kind_rst(ldg_math_stat_bayes_t *bayes, uint16_t kind)
{
    ldg_math_stat_bayes_kind_t *kind_entry = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bayes)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bayes->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(kind >= LDG_MATH_STAT_BAYES_KIND_MAX)) { return LDG_ERR_RANGE; }

    ret = ldg_math_stat_bayes_lock(bayes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    kind_entry = &bayes->kind[kind];
    kind_entry->obs_cunt = 0;
    kind_entry->mean = 0.0;
    kind_entry->m2 = 0.0;
    kind_entry->last = 0.0;
    LDG_ST_RELEASE(bayes->lock, LDG_MATH_STAT_BAYES_LOCK_FREE);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_bayes_rst(ldg_math_stat_bayes_t *bayes)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!bayes)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bayes->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_math_stat_bayes_lock(bayes);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < LDG_MATH_STAT_BAYES_KIND_MAX; idx++)
    {
        bayes->kind[idx].obs_cunt = 0;
        bayes->kind[idx].mean = 0.0;
        bayes->kind[idx].m2 = 0.0;
        bayes->kind[idx].last = 0.0;
    }
    bayes->total_obs = 0;
    LDG_ST_RELEASE(bayes->lock, LDG_MATH_STAT_BAYES_LOCK_FREE);

    return LDG_ERR_AOK;
}
