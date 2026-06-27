#include <string.h>

#include <dangling/sched/bayes.h>
#include <dangling/arch/amd64/fence.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

static void sched_bayes_lock(ldg_sched_bayes_t *bayes)
{
    uint32_t expected = 0;

    for (;;)
    {
        expected = 0;
        if (LDG_CAS(&bayes->lock, &expected, 1)) { return; }

        LDG_PAUSE;
    }
}

static void sched_bayes_predictive(const ldg_sched_bayes_t *bayes, const ldg_sched_bayes_class_t *cls, double *out_loc, double *out_var)
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

    obs = (double)cls->obs_cunt;
    xbar = cls->mean;
    ssd = cls->m2;

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

static void sched_bayes_moments(ldg_sched_bayes_t *bayes, uint16_t job_class, double *out_loc, double *out_var)
{
    ldg_sched_bayes_class_t snap = LDG_STRUCT_ZERO_INIT;

    sched_bayes_lock(bayes);
    snap = bayes->classes[job_class];
    LDG_STORE_RELEASE(bayes->lock, 0);

    sched_bayes_predictive(bayes, &snap, out_loc, out_var);
}

LDG_EXPORT uint32_t ldg_sched_bayes_init(ldg_sched_bayes_t *bayes, double prior_m, double prior_kappa, double prior_alpha, double prior_beta, double risk_k)
{
    if (LDG_UNLIKELY(!bayes)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(prior_kappa <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(prior_alpha <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(prior_beta <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(bayes, 0, sizeof(ldg_sched_bayes_t)) != bayes)) { return LDG_ERR_INVALID; }

    bayes->prior_m = prior_m;
    bayes->prior_kappa = prior_kappa;
    bayes->prior_alpha = prior_alpha;
    bayes->prior_beta = prior_beta;
    bayes->risk_k = risk_k;
    bayes->lock = 0;
    bayes->is_init = 1;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_bayes_shutdown(ldg_sched_bayes_t *bayes)
{
    if (LDG_UNLIKELY(!bayes)) { return LDG_ERR_FUNC_ARG_NULL; }

    bayes->is_init = 0;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_bayes_observe(ldg_sched_bayes_t *bayes, uint16_t job_class, double duration)
{
    ldg_sched_bayes_class_t *cls = 0x0;
    double delta = 0.0;
    double delta2 = 0.0;

    if (LDG_UNLIKELY(!bayes)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!bayes->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(job_class >= LDG_SCHED_BAYES_CLASS_MAX)) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY(duration < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    sched_bayes_lock(bayes);

    cls = &bayes->classes[job_class];

    cls->obs_cunt++;
    delta = duration - cls->mean;
    cls->mean += delta / (double)cls->obs_cunt;
    delta2 = duration - cls->mean;
    cls->m2 += delta * delta2;
    cls->last = duration;

    bayes->total_obs++;

    LDG_STORE_RELEASE(bayes->lock, 0);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_bayes_proj_k(ldg_sched_bayes_t *bayes, uint16_t job_class, double risk_k, double *out)
{
    double loc = 0.0;
    double var = 0.0;
    double sigma = 0.0;
    double res = 0.0;

    if (LDG_UNLIKELY(!bayes || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!bayes->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(job_class >= LDG_SCHED_BAYES_CLASS_MAX)) { return LDG_ERR_RANGE; }

    sched_bayes_moments(bayes, job_class, &loc, &var);

    sigma = __builtin_sqrt(var);

    res = loc + risk_k * sigma;
    if (res < 0.0) { res = 0.0; }

    *out = res;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_bayes_proj(ldg_sched_bayes_t *bayes, uint16_t job_class, double *out)
{
    double risk_k = 0.0;

    if (LDG_UNLIKELY(!bayes || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!bayes->is_init)) { return LDG_ERR_NOT_INIT; }

    risk_k = bayes->risk_k;

    return ldg_sched_bayes_proj_k(bayes, job_class, risk_k, out);
}

LDG_EXPORT uint32_t ldg_sched_bayes_mean_get(ldg_sched_bayes_t *bayes, uint16_t job_class, double *out)
{
    double loc = 0.0;
    double var = 0.0;

    if (LDG_UNLIKELY(!bayes || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!bayes->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(job_class >= LDG_SCHED_BAYES_CLASS_MAX)) { return LDG_ERR_RANGE; }

    sched_bayes_moments(bayes, job_class, &loc, &var);

    *out = loc;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_bayes_var_get(ldg_sched_bayes_t *bayes, uint16_t job_class, double *out)
{
    double loc = 0.0;
    double var = 0.0;

    if (LDG_UNLIKELY(!bayes || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!bayes->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(job_class >= LDG_SCHED_BAYES_CLASS_MAX)) { return LDG_ERR_RANGE; }

    sched_bayes_moments(bayes, job_class, &loc, &var);

    *out = var;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_bayes_sigma_get(ldg_sched_bayes_t *bayes, uint16_t job_class, double *out)
{
    double loc = 0.0;
    double var = 0.0;

    if (LDG_UNLIKELY(!bayes || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!bayes->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(job_class >= LDG_SCHED_BAYES_CLASS_MAX)) { return LDG_ERR_RANGE; }

    sched_bayes_moments(bayes, job_class, &loc, &var);

    *out = __builtin_sqrt(var);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_sched_bayes_obs_cunt_get(ldg_sched_bayes_t *bayes, uint16_t job_class)
{
    uint64_t res = 0;

    if (LDG_UNLIKELY(!bayes || !bayes->is_init))
    {
        res = UINT64_MAX;
        return res;
    }

    if (LDG_UNLIKELY(job_class >= LDG_SCHED_BAYES_CLASS_MAX))
    {
        res = UINT64_MAX;
        return res;
    }

    sched_bayes_lock(bayes);
    res = bayes->classes[job_class].obs_cunt;
    LDG_STORE_RELEASE(bayes->lock, 0);

    return res;
}

LDG_EXPORT uint32_t ldg_sched_bayes_risk_k_set(ldg_sched_bayes_t *bayes, double risk_k)
{
    if (LDG_UNLIKELY(!bayes)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!bayes->is_init)) { return LDG_ERR_NOT_INIT; }

    sched_bayes_lock(bayes);
    bayes->risk_k = risk_k;
    LDG_STORE_RELEASE(bayes->lock, 0);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_bayes_class_rst(ldg_sched_bayes_t *bayes, uint16_t job_class)
{
    ldg_sched_bayes_class_t *cls = 0x0;

    if (LDG_UNLIKELY(!bayes)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!bayes->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(job_class >= LDG_SCHED_BAYES_CLASS_MAX)) { return LDG_ERR_RANGE; }

    sched_bayes_lock(bayes);
    cls = &bayes->classes[job_class];
    cls->obs_cunt = 0;
    cls->mean = 0.0;
    cls->m2 = 0.0;
    cls->last = 0.0;
    LDG_STORE_RELEASE(bayes->lock, 0);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_bayes_rst(ldg_sched_bayes_t *bayes)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!bayes)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!bayes->is_init)) { return LDG_ERR_NOT_INIT; }

    sched_bayes_lock(bayes);
    for (idx = 0; idx < LDG_SCHED_BAYES_CLASS_MAX; idx++)
    {
        bayes->classes[idx].obs_cunt = 0;
        bayes->classes[idx].mean = 0.0;
        bayes->classes[idx].m2 = 0.0;
        bayes->classes[idx].last = 0.0;
    }
    bayes->total_obs = 0;
    LDG_STORE_RELEASE(bayes->lock, 0);

    return LDG_ERR_AOK;
}
