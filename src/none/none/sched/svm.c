#include <string.h>

#include <dangling/sched/svm.h>
#include <dangling/math/vec.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

LDG_EXPORT uint32_t ldg_sched_svm_init(ldg_sched_svm_t *svm, double cost, double epsilon)
{
    if (LDG_UNLIKELY(!svm)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cost <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(epsilon < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(svm, 0, sizeof(ldg_sched_svm_t)) != svm)) { return LDG_ERR_INVALID; }

    svm->cost = cost;
    svm->epsilon = epsilon;
    svm->is_init = 1;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_svm_shutdown(ldg_sched_svm_t *svm)
{
    if (LDG_UNLIKELY(!svm)) { return LDG_ERR_FUNC_ARG_NULL; }

    svm->is_init = 0;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_svm_predict(const ldg_sched_svm_t *svm, const ldg_sched_svm_feat_t *feat, double *out)
{
    double dot = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!svm || !feat || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!svm->is_init)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_math_vec_dot_get(svm->weights, feat->val, LDG_SCHED_SVM_FEAT_CUNT, &dot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = dot + svm->bias;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_svm_observe(ldg_sched_svm_t *svm, const ldg_sched_svm_feat_t *feat, double actual)
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

    if (LDG_UNLIKELY(!svm->is_init)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_sched_svm_predict(svm, feat, &predicted);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    err = actual - predicted;
    loss = __builtin_fabs(err) - svm->epsilon;

    if (loss <= 0.0)
    {
        svm->sample_cunt++;
        return LDG_ERR_AOK;
    }

    norm_sq = 1.0;
    for (idx = 0; idx < LDG_SCHED_SVM_FEAT_CUNT; idx++) { norm_sq += feat->val[idx] * feat->val[idx]; }

    denom = norm_sq + 1.0 / (2.0 * svm->cost);
    tau = loss / denom;

    sign = (err > 0.0) ? 1.0 : -1.0;

    for (idx = 0; idx < LDG_SCHED_SVM_FEAT_CUNT; idx++) { svm->weights[idx] += tau * sign * feat->val[idx]; }

    svm->bias += tau * sign;

    svm->sample_cunt++;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_svm_weights_rst(ldg_sched_svm_t *svm)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!svm)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!svm->is_init)) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < LDG_SCHED_SVM_FEAT_CUNT; idx++) { svm->weights[idx] = 0.0; }

    svm->bias = 0.0;
    svm->sample_cunt = 0;

    return LDG_ERR_AOK;
}
