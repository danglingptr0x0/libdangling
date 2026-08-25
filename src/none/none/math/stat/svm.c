
#include <dangling/math/vec.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

#include "svm.h"

LDG_EXPORT uint32_t ldg_math_stat_svm_init(ldg_math_stat_svm_t *svm, uint32_t feat_cunt, double cost, double epsilon)
{
    if (LDG_UNLIKELY(!svm)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(feat_cunt == 0 || feat_cunt > LDG_MATH_STAT_SVM_FEAT_CUNT_MAX)) { return LDG_ERR_MATH_STAT_SVM_FEAT_INVALID; }

    if (LDG_UNLIKELY(cost <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(epsilon < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ldg_mem_zero(svm, sizeof(ldg_math_stat_svm_t)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    svm->feat_cunt = feat_cunt;
    svm->cost = cost;
    svm->epsilon = epsilon;
    svm->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_svm_shutdown(ldg_math_stat_svm_t *svm)
{
    if (LDG_UNLIKELY(!svm)) { return LDG_ERR_FUNC_ARG_NULL; }

    svm->is_init = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_svm_predict(const ldg_math_stat_svm_t *svm, const double *feat, double *out)
{
    double dot = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!svm || !feat || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(svm->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_math_vec_dot_get(svm->weights, feat, svm->feat_cunt, &dot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = dot + svm->bias;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_svm_observe(ldg_math_stat_svm_t *svm, const double *feat, double actual)
{
    double predicted = 0.0;
    double err = 0.0;
    double loss = 0.0;
    double norm_sq = 0.0;
    double denom = 0.0;
    double tau = 0.0;
    double sign = 0.0;
    uint32_t ret = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!svm || !feat)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(svm->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_math_stat_svm_predict(svm, feat, &predicted);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    err = actual - predicted;
    loss = __builtin_fabs(err) - svm->epsilon;

    if (loss <= 0.0)
    {
        svm->sample_cunt++;
        return LDG_ERR_AOK;
    }

    norm_sq = 1.0;
    for (idx = 0; idx < svm->feat_cunt; idx++) { norm_sq += feat[idx] * feat[idx]; }

    denom = norm_sq + 1.0 / (2.0 * svm->cost);
    tau = loss / denom;

    sign = (err > 0.0) ? 1.0 : -1.0;

    for (idx = 0; idx < svm->feat_cunt; idx++) { svm->weights[idx] += tau * sign * feat[idx]; }

    svm->bias += tau * sign;

    svm->sample_cunt++;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_svm_weights_rst(ldg_math_stat_svm_t *svm)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!svm)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(svm->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < svm->feat_cunt; idx++) { svm->weights[idx] = 0.0; }

    svm->bias = 0.0;
    svm->sample_cunt = 0;

    return LDG_ERR_AOK;
}
