#ifndef LDG_SCHED_BAYES_H
#define LDG_SCHED_BAYES_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_SCHED_BAYES_CLASS_MAX 64
#define LDG_SCHED_BAYES_CLASS_INVALID UINT16_MAX

#define LDG_SCHED_BAYES_PRIOR_M_DEFAULT 0.0
#define LDG_SCHED_BAYES_PRIOR_KAPPA_DEFAULT 1.0
#define LDG_SCHED_BAYES_PRIOR_ALPHA_DEFAULT 2.0
#define LDG_SCHED_BAYES_PRIOR_BETA_DEFAULT 1.0
#define LDG_SCHED_BAYES_RISK_K_DEFAULT 0.0

typedef struct ldg_sched_bayes_class
{
    uint64_t obs_cunt;
    double mean;
    double m2;
    double last;
} ldg_sched_bayes_class_t;

typedef struct ldg_sched_bayes
{
    ldg_sched_bayes_class_t classes[LDG_SCHED_BAYES_CLASS_MAX];
    double prior_m;
    double prior_kappa;
    double prior_alpha;
    double prior_beta;
    double risk_k;
    uint64_t total_obs;
    volatile uint32_t lock;
    uint8_t is_init;
    uint8_t pudding[11];
} LDG_ALIGNED ldg_sched_bayes_t;

LDG_EXPORT uint32_t ldg_sched_bayes_init(ldg_sched_bayes_t *bayes, double prior_m, double prior_kappa, double prior_alpha, double prior_beta, double risk_k);
LDG_EXPORT uint32_t ldg_sched_bayes_shutdown(ldg_sched_bayes_t *bayes);
LDG_EXPORT uint32_t ldg_sched_bayes_observe(ldg_sched_bayes_t *bayes, uint16_t job_class, double duration);
LDG_EXPORT uint32_t ldg_sched_bayes_proj(ldg_sched_bayes_t *bayes, uint16_t job_class, double *out);
LDG_EXPORT uint32_t ldg_sched_bayes_proj_k(ldg_sched_bayes_t *bayes, uint16_t job_class, double risk_k, double *out);
LDG_EXPORT uint32_t ldg_sched_bayes_mean_get(ldg_sched_bayes_t *bayes, uint16_t job_class, double *out);
LDG_EXPORT uint32_t ldg_sched_bayes_var_get(ldg_sched_bayes_t *bayes, uint16_t job_class, double *out);
LDG_EXPORT uint32_t ldg_sched_bayes_sigma_get(ldg_sched_bayes_t *bayes, uint16_t job_class, double *out);
LDG_EXPORT uint64_t ldg_sched_bayes_obs_cunt_get(ldg_sched_bayes_t *bayes, uint16_t job_class);
LDG_EXPORT uint32_t ldg_sched_bayes_risk_k_set(ldg_sched_bayes_t *bayes, double risk_k);
LDG_EXPORT uint32_t ldg_sched_bayes_class_rst(ldg_sched_bayes_t *bayes, uint16_t job_class);
LDG_EXPORT uint32_t ldg_sched_bayes_rst(ldg_sched_bayes_t *bayes);

#endif
