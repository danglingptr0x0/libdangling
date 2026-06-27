#ifndef LDG_SCHED_JOB_H
#define LDG_SCHED_JOB_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/astar.h>

#define LDG_SCHED_PRIO_LEVELS 16
#define LDG_SCHED_JOB_MAX 4096
#define LDG_SCHED_DEP_MAX 8
#define LDG_SCHED_WORKER_DEQUE_SIZE 256
#define LDG_SCHED_JOB_ID_INVALID 0

#define LDG_SCHED_FRAME_DEADLINE_DEFAULT_MS 16.666
#define LDG_SCHED_PLOT_WEIGHT_CRIT_DEFAULT 0.4
#define LDG_SCHED_PLOT_WEIGHT_REMAINING_DEFAULT 0.3
#define LDG_SCHED_PLOT_WEIGHT_DEADLINE_DEFAULT 0.3
#define LDG_SCHED_PRIO_DEADLINE_NONE 1.0e300

typedef uint32_t ldg_sched_job_id_t;

typedef enum ldg_sched_job_state
{
    LDG_SCHED_JOB_STATE_FREE = 0,
    LDG_SCHED_JOB_STATE_PENDING,
    LDG_SCHED_JOB_STATE_READY,
    LDG_SCHED_JOB_STATE_RUNNING,
    LDG_SCHED_JOB_STATE_YIELDED,
    LDG_SCHED_JOB_STATE_DONE
} ldg_sched_job_state_t;

typedef enum ldg_sched_job_res
{
    LDG_SCHED_JOB_RES_DONE = 0,
    LDG_SCHED_JOB_RES_YIELD,
    LDG_SCHED_JOB_RES_CONT
} ldg_sched_job_res_t;

typedef ldg_sched_job_res_t (*ldg_sched_job_func_cb_t)(ldg_sched_job_id_t id, void *arg, void *yield_ctx);

typedef struct ldg_sched_job
{
    ldg_sched_job_func_cb_t func;
    void *arg;
    void *yield_ctx;
    double est_duration;
    double actual_duration;
    uint64_t submit_tsc;
    uint64_t start_tsc;
    uint64_t end_tsc;
    ldg_sched_job_id_t id;
    ldg_sched_job_id_t cont;
    ldg_sched_job_id_t deps[LDG_SCHED_DEP_MAX];
    uint8_t dep_cunt;
    uint8_t deps_remaining;
    uint8_t prio;
    uint8_t state;
    uint8_t proc_affinity;
    uint8_t is_static;
    uint16_t job_class;
    double deadline;
    uint8_t pudding[8];
} LDG_ALIGNED ldg_sched_job_t;

typedef struct ldg_sched_worker_deque
{
    ldg_sched_job_id_t jobs[LDG_SCHED_WORKER_DEQUE_SIZE];
    volatile uint64_t top;
    uint8_t pudding0[56];
    volatile uint64_t bottom;
    uint8_t pudding1[56];
    uint32_t core_id;
    uint32_t owner_id;
    uint8_t pudding2[56];
} LDG_ALIGNED ldg_sched_worker_deque_t;

typedef uint32_t (*ldg_sched_prio_cb_t)(const ldg_sched_job_t *job, double *out);

typedef struct ldg_sched_job_pool
{
    ldg_sched_job_t jobs[LDG_SCHED_JOB_MAX];
    uint32_t free_list[LDG_SCHED_JOB_MAX];
    uint64_t plot_order[LDG_SCHED_JOB_MAX + 1];
    uint8_t plot_dep_remaining[LDG_SCHED_JOB_MAX];
    uint8_t plot_crit_busy[LDG_SCHED_JOB_MAX];
    volatile uint64_t free_hd;
    double frame_deadline;
    double plot_weight_crit;
    double plot_weight_remaining;
    double plot_weight_deadline;
    ldg_sched_prio_cb_t prio_cb;
    volatile uint32_t lock;
    uint32_t job_cunt;
    uint8_t is_init;
    uint8_t pudding[63];
} LDG_ALIGNED ldg_sched_job_pool_t;

LDG_EXPORT uint32_t ldg_sched_job_pool_init(ldg_sched_job_pool_t *pool);
LDG_EXPORT uint32_t ldg_sched_job_pool_shutdown(ldg_sched_job_pool_t *pool);
LDG_EXPORT uint32_t ldg_sched_job_create(ldg_sched_job_pool_t *pool, ldg_sched_job_func_cb_t func, void *arg, uint8_t prio, uint8_t proc_affinity, ldg_sched_job_id_t *out_id);
LDG_EXPORT uint32_t ldg_sched_job_destroy(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id);
LDG_EXPORT uint32_t ldg_sched_job_dep_add(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t job, ldg_sched_job_id_t dep);
LDG_EXPORT uint32_t ldg_sched_job_dep_sig(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t completed);
LDG_EXPORT uint32_t ldg_sched_job_cont_set(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t job, ldg_sched_job_id_t cont);
LDG_EXPORT uint32_t ldg_sched_job_submit(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id);
LDG_EXPORT uint32_t ldg_sched_job_run(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, ldg_sched_job_res_t *out_res);
LDG_EXPORT uint32_t ldg_sched_job_yield_ctx_set(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, void *ctx);
LDG_EXPORT uint32_t ldg_sched_job_est_duration_set(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, double est);
LDG_EXPORT uint32_t ldg_sched_job_actual_duration_get(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, double *out);
LDG_EXPORT uint32_t ldg_sched_job_class_set(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, uint16_t job_class);
LDG_EXPORT uint32_t ldg_sched_job_class_get(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, uint16_t *out);
LDG_EXPORT uint32_t ldg_sched_job_deadline_set(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, double deadline);
LDG_EXPORT uint32_t ldg_sched_prio_deadline_cb(const ldg_sched_job_t *job, double *out);
LDG_EXPORT uint32_t ldg_sched_prio_fastest_cb(const ldg_sched_job_t *job, double *out);
LDG_EXPORT uint32_t ldg_sched_job_plot_weight_set(ldg_sched_job_pool_t *pool, double weight_crit, double weight_remaining, double weight_deadline);
LDG_EXPORT uint32_t ldg_sched_job_frame_deadline_set(ldg_sched_job_pool_t *pool, double deadline);
LDG_EXPORT uint32_t ldg_sched_job_plot(ldg_sched_job_pool_t *pool, ldg_math_astar_ctx_t *astar, ldg_sched_job_id_t *out_order, uint64_t max_cunt, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_sched_job_replot(ldg_sched_job_pool_t *pool, ldg_math_astar_ctx_t *astar, ldg_sched_job_id_t failed_job, ldg_sched_job_id_t *out_order, uint64_t max_cunt, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_sched_worker_deque_init(ldg_sched_worker_deque_t *deque, uint32_t core_id, uint32_t owner_id);
LDG_EXPORT uint32_t ldg_sched_worker_deque_shutdown(ldg_sched_worker_deque_t *deque);
LDG_EXPORT uint32_t ldg_sched_worker_deque_push(ldg_sched_worker_deque_t *deque, ldg_sched_job_id_t job);
LDG_EXPORT uint32_t ldg_sched_worker_deque_pop(ldg_sched_worker_deque_t *deque, ldg_sched_job_id_t *out_job);
LDG_EXPORT uint32_t ldg_sched_worker_deque_steal(ldg_sched_worker_deque_t *deque, ldg_sched_job_id_t *out_job);
LDG_EXPORT uint64_t ldg_sched_worker_deque_cunt_get(ldg_sched_worker_deque_t *deque);
LDG_EXPORT uint8_t ldg_sched_job_ready_is(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id);
LDG_EXPORT uint8_t ldg_sched_job_state_get(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id);
LDG_EXPORT uint64_t ldg_sched_job_cunt_get(ldg_sched_job_pool_t *pool);
LDG_EXPORT uint32_t ldg_sched_job_ready_collect(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t *out, uint64_t max, uint64_t *out_cunt);

#endif
