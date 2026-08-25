#ifndef LDG_SCHED_SCHED_H
#define LDG_SCHED_SCHED_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/astar.h>
#include <dangling/math/stat.h>
#include <dangling/time/perf.h>
#include <dangling/time/timer.h>
#include <dangling/thread/pool.h>
#include <dangling/sched/event.h>
#include <dangling/sched/job.h>
#include <dangling/sched/trace.h>

#define LDG_SCHED_WORKER_MAX 64
#define LDG_SCHED_FIXED_DT 0.016666666666666666
#define LDG_SCHED_SPIN_DT_MAX 0.25

#define LDG_SCHED_WORKER_CUNT_DEFAULT 4
#define LDG_SCHED_DRAIN_WAVE_MAX 4096
#define LDG_SCHED_DRAIN_SPIN_MAX 16777216
#define LDG_SCHED_MULTI_WORKER_MIN 2
#define LDG_SCHED_SVM_COST_DEFAULT 1.0
#define LDG_SCHED_SVM_EPSILON_DEFAULT 0.1

#define LDG_SCHED_EVENT_FILTER_ALL 0xFFFFFFFFFFFFFFFFULL
#define LDG_SCHED_EVENT_FILTER_CORE 0x0000000000000001ULL

typedef enum ldg_sched_phase
{
    LDG_SCHED_PHASE_START = 0,
    LDG_SCHED_PHASE_SPIN_BEGIN,
    LDG_SCHED_PHASE_FIXED_STEP,
    LDG_SCHED_PHASE_STEP,
    LDG_SCHED_PHASE_LATE_STEP,
    LDG_SCHED_PHASE_SPIN_END,
    LDG_SCHED_PHASE_STOP,
    LDG_SCHED_PHASE_MAX
} ldg_sched_phase_t;

typedef enum ldg_sched_est_strat
{
    LDG_SCHED_EST_STRAT_MANUAL = 0,
    LDG_SCHED_EST_STRAT_SVM,
    LDG_SCHED_EST_STRAT_BAYES,
    LDG_SCHED_EST_STRAT_MAX
} ldg_sched_est_strat_t;

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

typedef struct ldg_sched_conf
{
    double fixed_dt;
    double spin_budget_ms;
    double plot_weight_crit;
    double plot_weight_remaining;
    double plot_weight_deadline;
    double svm_cost;
    double svm_epsilon;
    double bayes_prior_m;
    double bayes_prior_kappa;
    double bayes_prior_alpha;
    double bayes_prior_beta;
    double bayes_risk_k;
    uint64_t timer_tick_ns;
    ldg_sched_prio_cb_t prio_cb;
    uint32_t est_strat;
    uint32_t worker_cunt;
    uint8_t enable_tracing;
    uint8_t pudding[7];
} ldg_sched_conf_t;

typedef struct ldg_sched_step_ctx
{
    double dt;
    double time;
    uint64_t spin;
} ldg_sched_step_ctx_t;

typedef struct ldg_sched_ctx
{
    ldg_sched_event_pool_t event_pool;
    ldg_time_timer_wheel_t timer_wheel;
    ldg_sched_job_pool_t job_pool;
    ldg_math_stat_svm_t svm;
    ldg_math_stat_bayes_t bayes;
    ldg_sched_trace_ring_t trace;
    ldg_sched_worker_deque_t worker_deques[LDG_SCHED_WORKER_MAX];
    ldg_thread_pool_t pool;
    ldg_time_ctx_t time_ctx;
    ldg_math_astar_ctx_t *astar;
    double fixed_accumulator;
    ldg_sched_event_desc_t phase_events[LDG_SCHED_PHASE_MAX];
    ldg_sched_job_id_t job_order[LDG_SCHED_JOB_MAX];
    ldg_sched_conf_t conf;
    volatile uint32_t should_stop;
    volatile uint32_t active_jobs;
    volatile uint32_t exec_stop;
    uint8_t is_init;
    uint8_t pudding[51];
} LDG_ALIGNED ldg_sched_ctx_t;

LDG_EXPORT uint32_t ldg_sched_conf_default_get(ldg_sched_conf_t *conf);
LDG_EXPORT uint32_t ldg_sched_ctx_init(ldg_sched_ctx_t *ctx, const ldg_sched_conf_t *conf);
LDG_EXPORT uint32_t ldg_sched_ctx_shutdown(ldg_sched_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_sched_ctx_spin(ldg_sched_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_sched_ctx_stop(ldg_sched_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_sched_ctx_drain(ldg_sched_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_sched_ctx_phase_event_get(ldg_sched_ctx_t *ctx, ldg_sched_phase_t phase, ldg_sched_event_desc_t *out_desc);
LDG_EXPORT uint32_t ldg_sched_ctx_event_create(ldg_sched_ctx_t *ctx, uint64_t filter_mask, ldg_sched_event_desc_t *out_desc);
LDG_EXPORT uint32_t ldg_sched_ctx_event_destroy(ldg_sched_ctx_t *ctx, ldg_sched_event_desc_t desc);
LDG_EXPORT uint32_t ldg_sched_ctx_event_sub(ldg_sched_ctx_t *ctx, ldg_sched_event_desc_t desc, ldg_sched_event_handler_cb_t handler, uint64_t filter_mask, uint8_t prio, void *user_data, uint32_t *out_sub_id);
LDG_EXPORT uint32_t ldg_sched_ctx_event_unsub(ldg_sched_ctx_t *ctx, uint32_t sub_id);
LDG_EXPORT uint32_t ldg_sched_ctx_event_emit(ldg_sched_ctx_t *ctx, ldg_sched_event_desc_t desc);
LDG_EXPORT uint32_t ldg_sched_ctx_event_ctx_emit(ldg_sched_ctx_t *ctx, ldg_sched_event_desc_t desc, void *event_ctx, uint64_t ctx_size);
LDG_EXPORT uint32_t ldg_sched_ctx_timer_create(ldg_sched_ctx_t *ctx, uint64_t interval_ns, uint8_t repeat, ldg_time_timer_cb_t func, void *arg, ldg_time_timer_id_t *out_id);
LDG_EXPORT uint32_t ldg_sched_ctx_timer_destroy(ldg_sched_ctx_t *ctx, ldg_time_timer_id_t id);
LDG_EXPORT uint32_t ldg_sched_ctx_job_create(ldg_sched_ctx_t *ctx, ldg_sched_job_func_cb_t func, void *arg, uint8_t prio, uint8_t proc_affinity, ldg_sched_job_id_t *out_id);
LDG_EXPORT uint32_t ldg_sched_ctx_job_submit(ldg_sched_ctx_t *ctx, ldg_sched_job_id_t id);
LDG_EXPORT uint32_t ldg_sched_ctx_job_dep_add(ldg_sched_ctx_t *ctx, ldg_sched_job_id_t job, ldg_sched_job_id_t dep);
LDG_EXPORT uint32_t ldg_sched_ctx_job_class_set(ldg_sched_ctx_t *ctx, ldg_sched_job_id_t id, uint16_t job_class);
LDG_EXPORT uint32_t ldg_sched_ctx_est_strat_set(ldg_sched_ctx_t *ctx, uint32_t est_strat);
LDG_EXPORT uint32_t ldg_sched_ctx_est_strat_get(ldg_sched_ctx_t *ctx, uint32_t *out);
LDG_EXPORT uint32_t ldg_sched_ctx_job_deadline_set(ldg_sched_ctx_t *ctx, ldg_sched_job_id_t id, double deadline);
LDG_EXPORT uint32_t ldg_sched_ctx_trace_push(ldg_sched_ctx_t *ctx, ldg_sched_trace_type_t type, uint32_t data0, uint64_t data1);
LDG_EXPORT uint32_t ldg_sched_ctx_dt_get(ldg_sched_ctx_t *ctx, double *out);
LDG_EXPORT uint32_t ldg_sched_ctx_time_get(ldg_sched_ctx_t *ctx, double *out);
LDG_EXPORT uint64_t ldg_sched_ctx_spin_get(ldg_sched_ctx_t *ctx);

#endif
