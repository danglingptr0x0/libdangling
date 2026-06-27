#ifndef LDG_SCHED_SVM_H
#define LDG_SCHED_SVM_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_SCHED_SVM_FEAT_CUNT 16

typedef enum ldg_sched_svm_feat_idx
{
    LDG_SCHED_SVM_FEAT_JOB_TYPE = 0,
    LDG_SCHED_SVM_FEAT_INPUT_SIZE,
    LDG_SCHED_SVM_FEAT_HIST_MEAN,
    LDG_SCHED_SVM_FEAT_HIST_VAR,
    LDG_SCHED_SVM_FEAT_HIST_MIN,
    LDG_SCHED_SVM_FEAT_HIST_MAX,
    LDG_SCHED_SVM_FEAT_CPU_LOAD,
    LDG_SCHED_SVM_FEAT_MEM_PRESSURE,
    LDG_SCHED_SVM_FEAT_CACHE_MISS_RATE,
    LDG_SCHED_SVM_FEAT_LAST_DURATION,
    LDG_SCHED_SVM_FEAT_LAST_ERR,
    LDG_SCHED_SVM_FEAT_FRAME_BUDGET_REMAINING,
    LDG_SCHED_SVM_FEAT_PENDING_JOB_CUNT,
    LDG_SCHED_SVM_FEAT_ACTIVE_WORKER_CUNT,
    LDG_SCHED_SVM_FEAT_RESERVED0,
    LDG_SCHED_SVM_FEAT_RESERVED1
} ldg_sched_svm_feat_idx_t;

typedef struct ldg_sched_svm_feat
{
    double val[LDG_SCHED_SVM_FEAT_CUNT];
} ldg_sched_svm_feat_t;

typedef struct ldg_sched_svm
{
    double weights[LDG_SCHED_SVM_FEAT_CUNT];
    double bias;
    double cost;
    double epsilon;
    uint64_t sample_cunt;
    uint8_t is_init;
    uint8_t pudding[31];
} LDG_ALIGNED ldg_sched_svm_t;

LDG_EXPORT uint32_t ldg_sched_svm_init(ldg_sched_svm_t *svm, double cost, double epsilon);
LDG_EXPORT uint32_t ldg_sched_svm_shutdown(ldg_sched_svm_t *svm);
LDG_EXPORT uint32_t ldg_sched_svm_predict(const ldg_sched_svm_t *svm, const ldg_sched_svm_feat_t *feat, double *out);
LDG_EXPORT uint32_t ldg_sched_svm_observe(ldg_sched_svm_t *svm, const ldg_sched_svm_feat_t *feat, double actual);
LDG_EXPORT uint32_t ldg_sched_svm_weights_rst(ldg_sched_svm_t *svm);

#endif
