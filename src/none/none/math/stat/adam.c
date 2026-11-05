#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>
#include <dangling/math/math.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>

#define ADAM_LR_DEFAULT 0.001
#define ADAM_BETA_MEAN_DEFAULT 0.9
#define ADAM_BETA_VAR_DEFAULT 0.999
#define ADAM_EPS_DEFAULT 0.00000001
#define ADAM_GRAD_NORM_DEFAULT 1.0
#define ADAM_WEIGHT_DECAY_DEFAULT 0.0
#define ADAM_SCHEDULE_HALF 0.5
#define ADAM_WARMUP_NEXT 1
#define ADAM_UNIT 1.0
#define ADAM_NO_CLIP 0.0
#define ADAM_CUM_SEED 0.0

static uint32_t adam_alloc(uint64_t size, void **out)
{
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc(size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

static void adam_free(void *ptr)
{
    if (ptr) { ldg_mem_dealloc(ptr); }
}

static uint32_t adam_init_ck(const ldg_math_stat_adam_t *adam)
{
    if (LDG_UNLIKELY(!adam)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(adam->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

static double adam_grad_norm_get(const ldg_math_stat_autograd_t *graph, uintptr_t param_cunt)
{
    const double *grad = graph->grad;
    const double *end = graph->grad + param_cunt;
    double cum = ADAM_CUM_SEED;

    while (grad < end)
    {
        cum = cum + (*grad * *grad);
        grad++;
    }

    return ldg_math_sqrt_get(cum);
}

uint32_t ldg_math_stat_adam_conf_default_get(ldg_math_stat_adam_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->lr = ADAM_LR_DEFAULT;
    out->beta_mean = ADAM_BETA_MEAN_DEFAULT;
    out->beta_var = ADAM_BETA_VAR_DEFAULT;
    out->eps = ADAM_EPS_DEFAULT;
    out->grad_norm_max = ADAM_GRAD_NORM_DEFAULT;
    out->weight_decay = ADAM_WEIGHT_DECAY_DEFAULT;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_adam_init(ldg_math_stat_adam_t *adam, const ldg_math_stat_adam_conf_t *conf, const ldg_math_stat_autograd_t *graph)
{
    void *ptr = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!adam || !conf || !graph)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(graph->elem_mark == 0)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(conf->lr <= ADAM_NO_CLIP || conf->beta_mean <= ADAM_NO_CLIP || conf->beta_mean >= ADAM_UNIT || conf->beta_var <= ADAM_NO_CLIP || conf->beta_var >= ADAM_UNIT || conf->eps <= ADAM_NO_CLIP || conf->grad_norm_max < ADAM_NO_CLIP || conf->weight_decay < ADAM_NO_CLIP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_zero(adam, sizeof(*adam));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    adam->conf = *conf;
    adam->param_cunt = graph->elem_mark;

    ret = adam_alloc((uint64_t)adam->param_cunt * (uint64_t)sizeof(double), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    adam->mean = (double *)ptr;

    ret = adam_alloc((uint64_t)adam->param_cunt * (uint64_t)sizeof(double), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        adam_free(adam->mean);
        return ret;
    }

    adam->var = (double *)ptr;

    ret = adam_alloc((uint64_t)adam->param_cunt, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        adam_free(adam->var);
        adam_free(adam->mean);
        return ret;
    }

    adam->frozen = (uint8_t *)ptr;

    adam->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_adam_shutdown(ldg_math_stat_adam_t *adam)
{
    uint32_t ret = adam_init_ck(adam);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    adam_free(adam->frozen);
    adam_free(adam->var);
    adam_free(adam->mean);

    return ldg_mem_zero(adam, sizeof(*adam));
}

uint32_t ldg_math_stat_adam_rst(ldg_math_stat_adam_t *adam)
{
    uint32_t ret = adam_init_ck(adam);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(adam->mean, (uint64_t)adam->param_cunt * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(adam->var, (uint64_t)adam->param_cunt * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    adam->step = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_adam_frozen_set(ldg_math_stat_adam_t *adam, const ldg_math_stat_model_t *model)
{
    uint32_t cunt = 0;
    uint32_t ret = adam_init_ck(adam);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!model)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_stat_model_frozen_mask_set(model, adam->frozen, adam->param_cunt, &cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    adam->frozen_cunt = adam->frozen_cunt + cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_adam_frozen_clear(ldg_math_stat_adam_t *adam)
{
    uint32_t ret = adam_init_ck(adam);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    adam->frozen_cunt = 0;

    return ldg_mem_zero(adam->frozen, (uint64_t)adam->param_cunt);
}

uint32_t ldg_math_stat_adam_lr_set(ldg_math_stat_adam_t *adam, double lr)
{
    uint32_t ret = adam_init_ck(adam);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(lr <= ADAM_NO_CLIP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    adam->conf.lr = lr;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_adam_step(ldg_math_stat_adam_t *adam, ldg_math_stat_autograd_t *graph, double *out_grad_norm)
{
    double *val = 0x0;
    const double *grad = 0x0;
    const double *end = 0x0;
    const uint8_t *frozen = 0x0;
    double *mean = 0x0;
    double *var = 0x0;
    double norm = ADAM_CUM_SEED;
    double scale = (double)ADAM_UNIT;
    double step = ADAM_CUM_SEED;
    double bias_mean = ADAM_CUM_SEED;
    double bias_var = ADAM_CUM_SEED;
    double beta_mean = ADAM_CUM_SEED;
    double beta_var = ADAM_CUM_SEED;
    double beta_mean_rest = ADAM_CUM_SEED;
    double beta_var_rest = ADAM_CUM_SEED;
    double lr = ADAM_CUM_SEED;
    double eps = ADAM_CUM_SEED;
    double decay = ADAM_CUM_SEED;
    uint32_t ret = adam_init_ck(adam);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!graph)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(graph->elem_mark != adam->param_cunt)) { return LDG_ERR_INVALID; }

    if (out_grad_norm) { *out_grad_norm = ADAM_CUM_SEED; }

    norm = adam_grad_norm_get(graph, adam->param_cunt);
    if (adam->conf.grad_norm_max > ADAM_NO_CLIP && norm > (double)adam->conf.grad_norm_max) { scale = (double)adam->conf.grad_norm_max / norm; }

    adam->step++;

    beta_mean = (double)adam->conf.beta_mean;
    beta_var = (double)adam->conf.beta_var;
    beta_mean_rest = (double)(ADAM_UNIT - adam->conf.beta_mean);
    beta_var_rest = (double)(ADAM_UNIT - adam->conf.beta_var);
    lr = (double)adam->conf.lr;
    eps = (double)adam->conf.eps;
    decay = (double)adam->conf.weight_decay;
    bias_mean = (double)(ADAM_UNIT - ldg_math_pwr_get(adam->conf.beta_mean, (double)adam->step));
    bias_var = (double)(ADAM_UNIT - ldg_math_pwr_get(adam->conf.beta_var, (double)adam->step));

    val = graph->val;
    grad = graph->grad;
    end = graph->val + adam->param_cunt;
    frozen = adam->frozen;
    mean = adam->mean;
    var = adam->var;

    while (val < end)
    {
        if (adam->frozen_cunt == 0 || *frozen != LDG_TRUTH_TRUE)
        {
            step = *grad * scale;
            *mean = (beta_mean * *mean) + (beta_mean_rest * step);
            *var = (beta_var * *var) + (beta_var_rest * step * step);
            *val = *val - (lr * (((*mean / bias_mean) / (ldg_math_sqrt_get(*var / bias_var) + eps)) + (decay * *val)));
        }

        val++;
        grad++;
        frozen++;
        mean++;
        var++;
    }

    if (out_grad_norm) { *out_grad_norm = norm; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_adam_schedule_get(double lr_max, double final_frac, uint32_t warmup_cunt, uint32_t total_cunt, uint32_t step, double *out)
{
    double ratio = ADAM_CUM_SEED;
    double coeff = ADAM_CUM_SEED;
    double lr_min = ADAM_CUM_SEED;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ADAM_CUM_SEED;

    if (LDG_UNLIKELY(lr_max <= ADAM_NO_CLIP || final_frac < ADAM_NO_CLIP || final_frac > ADAM_UNIT || total_cunt == 0 || warmup_cunt >= total_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (step < warmup_cunt)
    {
        *out = lr_max * ((double)(step + ADAM_WARMUP_NEXT) / (double)warmup_cunt);
        return LDG_ERR_AOK;
    }

    if (step >= total_cunt) { step = total_cunt; }

    ratio = (double)(step - warmup_cunt) / (double)(total_cunt - warmup_cunt);
    coeff = ADAM_SCHEDULE_HALF * (ADAM_UNIT + ldg_math_cos_get(LDG_MATH_PI * ratio));
    lr_min = lr_max * final_frac;
    *out = lr_min + (coeff * (lr_max - lr_min));

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_adam_get(const ldg_math_stat_adam_t *adam, double *dst_mean, double *dst_var, uintptr_t cap, uintptr_t *out_cunt, uint32_t *out_step)
{
    uint32_t ret = adam_init_ck(adam);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!dst_mean || !dst_var || !out_cunt || !out_step)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    *out_step = 0;

    if (LDG_UNLIKELY(cap < adam->param_cunt)) { return LDG_ERR_BOUNDS; }

    ret = ldg_mem_cpy(dst_mean, adam->mean, (uint64_t)adam->param_cunt * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_cpy(dst_var, adam->var, (uint64_t)adam->param_cunt * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_cunt = adam->param_cunt;
    *out_step = adam->step;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_adam_set(ldg_math_stat_adam_t *adam, const double *src_mean, const double *src_var, uintptr_t cunt, uint32_t step)
{
    uint32_t ret = adam_init_ck(adam);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!src_mean || !src_var)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt != adam->param_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_cpy(adam->mean, src_mean, (uint64_t)cunt * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_cpy(adam->var, src_var, (uint64_t)cunt * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    adam->step = step;

    return LDG_ERR_AOK;
}
