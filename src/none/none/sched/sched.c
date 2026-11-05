
#include <dangling/sched/sched.h>
#include <dangling/math/math.h>
#include <dangling/time/time.h>
#include <dangling/arch/fence.h>
#include <dangling/arch/atomic.h>
#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/logging/logging.h>
#include <dangling/math/rational.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>
#include <dangling/thread/yield.h>

#define LDG_SCHED_STEP_DT_NONE 0
#define LDG_SCHED_FIXED_RATE_NONE 0
#define LDG_SCHED_DRAIN_WAIT_MS 2000U
#define LDG_SCHED_WAIT_SHARED LDG_TRUTH_FALSE
#define LDG_SCHED_YIELD_SLICE 0ULL
#define LDG_SCHED_SPIN_WAIT_MS_MIN 1ULL
#define LDG_SCHED_INIT_STAGE_NONE 0U
#define LDG_SCHED_INIT_STAGE_EVENT_POOL 1U
#define LDG_SCHED_INIT_STAGE_TIMER 2U
#define LDG_SCHED_INIT_STAGE_JOB_POOL 3U
#define LDG_SCHED_INIT_STAGE_SVM 4U
#define LDG_SCHED_INIT_STAGE_BAYES 5U
#define LDG_SCHED_INIT_STAGE_ASTAR 6U
#define LDG_SCHED_INIT_STAGE_TRACE 7U
#define LDG_SCHED_INIT_STAGE_WAIT 8U
#define LDG_SCHED_INIT_STAGE_POOL 9U

static uint32_t sched_worker_idle_wait(ldg_sched_ctx_t *ctx, uint32_t seen)
{
    uint32_t ret = 0;

    ret = ldg_mut_lock(&ctx->work_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (ret == LDG_ERR_AOK && !LDG_RD_ONCE(ctx->exec_stop) && LDG_LD_ACQUIRE(ctx->work_seq) == seen) { ret = ldg_cond_wait(&ctx->work_cond, &ctx->work_mut); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mut_unlock(&ctx->work_mut) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

        return ret;
    }

    return ldg_mut_unlock(&ctx->work_mut);
}

static uint32_t sched_cond_bcast(ldg_sched_ctx_t *ctx, ldg_cond_t *cond, uint8_t work_is)
{
    uint32_t ret = 0;

    ret = ldg_mut_lock(&ctx->work_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (work_is == LDG_TRUTH_TRUE) { LDG_ST_RELEASE(ctx->work_seq, ctx->work_seq + LDG_SCHED_STEP); }

    ret = ldg_cond_bcast(cond);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mut_unlock(&ctx->work_mut) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

        return ret;
    }

    return ldg_mut_unlock(&ctx->work_mut);
}

static uint32_t sched_drain_wait(ldg_sched_ctx_t *ctx)
{
    uint32_t ret = 0;

    ret = ldg_mut_lock(&ctx->work_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (ret == LDG_ERR_AOK && LDG_LD_ACQUIRE(ctx->active_jobs) != 0) { ret = ldg_cond_timedwait(&ctx->done_cond, &ctx->work_mut, LDG_SCHED_DRAIN_WAIT_MS); }

    ret = ldg_mut_unlock(&ctx->work_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_LD_ACQUIRE(ctx->active_jobs) != 0) { return LDG_ERR_TIMEOUT; }

    return LDG_ERR_AOK;
}

static uint64_t sched_spin_wait_ms_get(ldg_sched_ctx_t *ctx)
{
    uint64_t rate = (uint64_t)ctx->conf.fixed_rate;
    uint64_t gap = 0;
    uint64_t wait = LDG_SCHED_SPIN_WAIT_MS_MIN;
    uint64_t tick = ctx->conf.timer_tick_ns / LDG_NS_PER_MS;

    if (ctx->fixed_accumulator < LDG_MS_PER_SEC) { gap = LDG_MS_PER_SEC - ctx->fixed_accumulator; }

    if (rate > 0) { wait = (gap + rate - LDG_SCHED_STEP) / rate; }

    if (tick < LDG_SCHED_SPIN_WAIT_MS_MIN) { tick = LDG_SCHED_SPIN_WAIT_MS_MIN; }

    if (ldg_time_timer_cunt_get(&ctx->timer_wheel) > 0 && tick < wait) { wait = tick; }

    if (wait < LDG_SCHED_SPIN_WAIT_MS_MIN) { wait = LDG_SCHED_SPIN_WAIT_MS_MIN; }

    return wait;
}

static uint32_t sched_spin_idle_wait(ldg_sched_ctx_t *ctx, uint64_t wait_ms)
{
    uint32_t ret = 0;

    ret = ldg_mut_lock(&ctx->work_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_LD_ACQUIRE(ctx->spin_wake) != LDG_TRUTH_TRUE && !LDG_RD_ONCE(ctx->should_stop)) { ret = ldg_cond_timedwait(&ctx->spin_cond, &ctx->work_mut, wait_ms); }

    if (ret == LDG_ERR_TIMEOUT) { ret = LDG_ERR_AOK; }

    LDG_ST_RELEASE(ctx->spin_wake, LDG_TRUTH_FALSE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mut_unlock(&ctx->work_mut) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

        return ret;
    }

    return ldg_mut_unlock(&ctx->work_mut);
}

static uint32_t sched_spin_wake(ldg_sched_ctx_t *ctx)
{
    uint32_t ret = 0;

    ret = ldg_mut_lock(&ctx->work_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    LDG_ST_RELEASE(ctx->spin_wake, LDG_TRUTH_TRUE);

    ret = ldg_cond_bcast(&ctx->spin_cond);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mut_unlock(&ctx->work_mut) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

        return ret;
    }

    return ldg_mut_unlock(&ctx->work_mut);
}

static uint32_t sched_wait_destroy(ldg_sched_ctx_t *ctx)
{
    uint32_t ret = LDG_ERR_AOK;

    if (ctx->wait_init != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ldg_cond_destroy(&ctx->spin_cond) != LDG_ERR_AOK) { ret = LDG_ERR_INVALID; }

    if (ldg_cond_destroy(&ctx->done_cond) != LDG_ERR_AOK) { ret = LDG_ERR_INVALID; }

    if (ldg_cond_destroy(&ctx->work_cond) != LDG_ERR_AOK) { ret = LDG_ERR_INVALID; }

    if (ldg_mut_destroy(&ctx->work_mut) != LDG_ERR_AOK) { ret = LDG_ERR_INVALID; }

    ctx->wait_init = LDG_TRUTH_FALSE;

    return ret;
}

static uint32_t sched_wait_init(ldg_sched_ctx_t *ctx)
{
    uint32_t ret = 0;

    ret = ldg_mut_init(&ctx->work_mut, LDG_SCHED_WAIT_SHARED);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_cond_init(&ctx->work_cond, LDG_SCHED_WAIT_SHARED);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mut_destroy(&ctx->work_mut) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

        return ret;
    }

    ret = ldg_cond_init(&ctx->done_cond, LDG_SCHED_WAIT_SHARED);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_cond_destroy(&ctx->work_cond) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

        if (LDG_UNLIKELY(ldg_mut_destroy(&ctx->work_mut) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

        return ret;
    }

    ret = ldg_cond_init(&ctx->spin_cond, LDG_SCHED_WAIT_SHARED);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_cond_destroy(&ctx->done_cond) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

        if (LDG_UNLIKELY(ldg_cond_destroy(&ctx->work_cond) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

        if (LDG_UNLIKELY(ldg_mut_destroy(&ctx->work_mut) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

        return ret;
    }

    ctx->wait_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t sched_worker_loop(uint32_t worker_id, void *arg)
{
    ldg_sched_ctx_t *ctx = 0x0;
    ldg_sched_job_res_t res = LDG_SCHED_JOB_RES_DONE;
    ldg_sched_job_id_t job = 0;
    uint32_t scan = 0;
    uint32_t victim = 0;
    uint32_t seen = 0;
    uint32_t job_idx = 0;
    uint32_t ret = 0;
    uint8_t got = LDG_TRUTH_FALSE;

    ctx = (ldg_sched_ctx_t *)arg;

    while (!LDG_RD_ONCE(ctx->exec_stop))
    {
        got = LDG_TRUTH_FALSE;
        seen = LDG_LD_ACQUIRE(ctx->work_seq);

        for (scan = 0; scan < ctx->conf.worker_cunt; scan++)
        {
            victim = (worker_id + scan) % ctx->conf.worker_cunt;
            if (ldg_sched_worker_deque_steal(&ctx->worker_deques[victim], &job) == LDG_ERR_AOK)
            {
                got = LDG_TRUTH_TRUE;
                break;
            }
        }

        if (got != LDG_TRUTH_TRUE)
        {
            ret = sched_worker_idle_wait(ctx, seen);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            continue;
        }

        ret = ldg_sched_job_run(&ctx->job_pool, job, &res);

        if (ret == LDG_ERR_AOK && ctx->conf.est_strat == LDG_SCHED_EST_STRAT_BAYES && res == LDG_SCHED_JOB_RES_DONE)
        {
            job_idx = job - LDG_SCHED_STEP;
            ret = ldg_math_stat_bayes_observe(&ctx->bayes, ctx->job_pool.jobs[job_idx].job_class, (double)ctx->job_pool.jobs[job_idx].actual_duration);
        }

        if (LDG_SUB_FETCH(ctx->active_jobs, LDG_SCHED_STEP) == 0)
        {
            ret = sched_cond_bcast(ctx, &ctx->done_cond, LDG_TRUTH_FALSE);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_conf_default_get(ldg_sched_conf_t *conf)
{
    if (LDG_UNLIKELY(!conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_zero(conf, sizeof(ldg_sched_conf_t)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    conf->fixed_rate = LDG_SCHED_FIXED_RATE;
    if (LDG_UNLIKELY(ldg_math_rational_set(LDG_SCHED_FRAME_DEADLINE_NUM_DEFAULT, LDG_SCHED_FRAME_DEADLINE_DEN_DEFAULT, &conf->spin_budget) != LDG_ERR_AOK)) { return LDG_ERR_MATH_VAL_INVALID; }

    if (LDG_UNLIKELY(ldg_math_rational_set(LDG_SCHED_PLOT_WEIGHT_CRIT_NUM_DEFAULT, LDG_SCHED_PLOT_WEIGHT_CRIT_DEN_DEFAULT, &conf->plot_weight_crit) != LDG_ERR_AOK)) { return LDG_ERR_MATH_VAL_INVALID; }

    if (LDG_UNLIKELY(ldg_math_rational_set(LDG_SCHED_PLOT_WEIGHT_REMAINING_NUM_DEFAULT, LDG_SCHED_PLOT_WEIGHT_REMAINING_DEN_DEFAULT, &conf->plot_weight_remaining) != LDG_ERR_AOK)) { return LDG_ERR_MATH_VAL_INVALID; }

    if (LDG_UNLIKELY(ldg_math_rational_set(LDG_SCHED_PLOT_WEIGHT_DEADLINE_NUM_DEFAULT, LDG_SCHED_PLOT_WEIGHT_DEADLINE_DEN_DEFAULT, &conf->plot_weight_deadline) != LDG_ERR_AOK)) { return LDG_ERR_MATH_VAL_INVALID; }

    conf->svm_cost = LDG_SCHED_SVM_COST_DEFAULT;
    conf->svm_epsilon = LDG_SCHED_SVM_EPSILON_DEFAULT;
    conf->bayes_prior_m = LDG_MATH_STAT_BAYES_PRIOR_M_DEFAULT;
    conf->bayes_prior_kappa = LDG_MATH_STAT_BAYES_PRIOR_KAPPA_DEFAULT;
    conf->bayes_prior_alpha = LDG_MATH_STAT_BAYES_PRIOR_ALPHA_DEFAULT;
    conf->bayes_prior_beta = LDG_MATH_STAT_BAYES_PRIOR_BETA_DEFAULT;
    conf->bayes_risk_k = LDG_MATH_STAT_BAYES_RISK_K_DEFAULT;
    conf->timer_tick_ns = LDG_NS_PER_MS;
    conf->est_strat = LDG_SCHED_EST_STRAT_MANUAL;
    conf->prio_cb = 0x0;
    conf->worker_cunt = LDG_SCHED_WORKER_CUNT_DEFAULT;
    conf->tracing_en = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static void sched_unwind_log(uint32_t lvl, const char *step, uint32_t code)
{
    if (code == LDG_ERR_AOK) { return; }

    if (LDG_UNLIKELY(ldg_logging_line_wr(LDG_LOGGING_CON, lvl, "sched: init unwind failed; step: %s; code: %u32", step, code) != LDG_ERR_AOK)) { return; }
}

static uint32_t sched_init_unwind(ldg_sched_ctx_t *ctx, uint32_t stage, uint32_t deque_cunt, uint32_t fault)
{
    uint32_t idx = 0;
    uint32_t clean = 0;

    if (stage >= LDG_SCHED_INIT_STAGE_POOL)
    {
        LDG_WR_ONCE(ctx->exec_stop, LDG_TRUTH_TRUE);

        sched_unwind_log(LDG_LOGGING_LVL_WARN, "wake", sched_cond_bcast(ctx, &ctx->work_cond, LDG_TRUTH_TRUE));

        clean = ldg_thread_pool_shutdown(&ctx->pool);
        if (LDG_UNLIKELY(clean != LDG_ERR_AOK))
        {
            sched_unwind_log(LDG_LOGGING_LVL_ERR, "pool join", clean);
            return fault;
        }
    }

    if (stage >= LDG_SCHED_INIT_STAGE_WAIT) { sched_unwind_log(LDG_LOGGING_LVL_WARN, "wait destroy", sched_wait_destroy(ctx)); }

    if (stage >= LDG_SCHED_INIT_STAGE_TRACE && ctx->conf.tracing_en == LDG_TRUTH_TRUE) { sched_unwind_log(LDG_LOGGING_LVL_WARN, "trace shutdown", ldg_sched_trace_shutdown(&ctx->trace)); }

    if (stage >= LDG_SCHED_INIT_STAGE_ASTAR) { sched_unwind_log(LDG_LOGGING_LVL_WARN, "astar destroy", ldg_math_astar_ctx_destroy(&ctx->astar)); }

    for (idx = 0; idx < deque_cunt; idx++) { sched_unwind_log(LDG_LOGGING_LVL_WARN, "deque shutdown", ldg_sched_worker_deque_shutdown(&ctx->worker_deques[idx])); }

    if (stage >= LDG_SCHED_INIT_STAGE_BAYES) { sched_unwind_log(LDG_LOGGING_LVL_WARN, "bayes shutdown", ldg_math_stat_bayes_shutdown(&ctx->bayes)); }

    if (stage >= LDG_SCHED_INIT_STAGE_SVM) { sched_unwind_log(LDG_LOGGING_LVL_WARN, "svm shutdown", ldg_math_stat_svm_shutdown(&ctx->svm)); }

    if (stage >= LDG_SCHED_INIT_STAGE_JOB_POOL) { sched_unwind_log(LDG_LOGGING_LVL_WARN, "job pool shutdown", ldg_sched_job_pool_shutdown(&ctx->job_pool)); }

    if (stage >= LDG_SCHED_INIT_STAGE_TIMER) { sched_unwind_log(LDG_LOGGING_LVL_WARN, "timer wheel shutdown", ldg_time_timer_wheel_shutdown(&ctx->timer_wheel)); }

    if (stage >= LDG_SCHED_INIT_STAGE_EVENT_POOL) { sched_unwind_log(LDG_LOGGING_LVL_WARN, "event pool shutdown", ldg_sched_event_pool_shutdown(&ctx->event_pool)); }

    ctx->is_init = LDG_TRUTH_FALSE;

    return fault;
}

LDG_EXPORT uint32_t ldg_sched_ctx_init(ldg_sched_ctx_t *ctx, const ldg_sched_conf_t *conf)
{
    ldg_sched_conf_t default_conf = LDG_STRUCT_ZERO_INIT;
    ldg_sched_event_desc_t desc = LDG_SCHED_EVENT_DESC_INVALID;
    uint32_t idx = 0;
    uint32_t deque_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_zero(ctx, sizeof(ldg_sched_ctx_t)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    ctx->is_init = UINT8_MAX;

    if (!conf)
    {
        ret = ldg_sched_conf_default_get(&default_conf);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        conf = &default_conf;
    }

    ctx->conf = *conf;

    if (LDG_UNLIKELY(conf->fixed_rate == LDG_SCHED_FIXED_RATE_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_sched_event_pool_init(&ctx->event_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_NONE, deque_cunt, ret); }

    ret = ldg_time_timer_wheel_init(&ctx->timer_wheel, conf->timer_tick_ns);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_EVENT_POOL, deque_cunt, ret); }

    ret = ldg_sched_job_pool_init(&ctx->job_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_TIMER, deque_cunt, ret); }

    ret = ldg_sched_job_plot_weight_set(&ctx->job_pool, &conf->plot_weight_crit, &conf->plot_weight_remaining, &conf->plot_weight_deadline);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_JOB_POOL, deque_cunt, ret); }

    ret = ldg_sched_job_frame_deadline_set(&ctx->job_pool, &conf->spin_budget);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_JOB_POOL, deque_cunt, ret); }

    ctx->job_pool.prio_cb = conf->prio_cb;

    ret = ldg_math_stat_svm_init(&ctx->svm, LDG_SCHED_SVM_FEAT_CUNT, conf->svm_cost, conf->svm_epsilon);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_JOB_POOL, deque_cunt, ret); }

    ret = ldg_math_stat_bayes_init(&ctx->bayes, conf->bayes_prior_m, conf->bayes_prior_kappa, conf->bayes_prior_alpha, conf->bayes_prior_beta, conf->bayes_risk_k);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_SVM, deque_cunt, ret); }

    for (idx = 0; idx < conf->worker_cunt && idx < LDG_SCHED_WORKER_MAX; idx++)
    {
        ret = ldg_sched_worker_deque_init(&ctx->worker_deques[idx], idx, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_BAYES, deque_cunt, ret); }

        deque_cunt++;
    }

    ret = ldg_math_astar_ctx_create((uint64_t)LDG_SCHED_PLOT_NODE_CUNT, (uint64_t)LDG_SCHED_JOB_MAX, &ctx->astar);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_BAYES, deque_cunt, ret); }

    for (idx = 0; idx < LDG_SCHED_PHASE_MAX; idx++)
    {
        ret = ldg_sched_event_create(&ctx->event_pool, LDG_SCHED_EVENT_FILTER_CORE, &desc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_ASTAR, deque_cunt, ret); }

        ctx->phase_events[idx] = desc;
    }

    ret = ldg_time_init(&ctx->time_ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_ASTAR, deque_cunt, ret); }

    if (conf->tracing_en == LDG_TRUTH_TRUE)
    {
        ret = ldg_sched_trace_init(&ctx->trace);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_ASTAR, deque_cunt, ret); }
    }

    ret = sched_wait_init(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_TRACE, deque_cunt, ret); }

    if (ctx->conf.worker_cunt >= 2)
    {
        ret = ldg_thread_pool_init(&ctx->pool, ctx->conf.worker_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_WAIT, deque_cunt, ret); }

        ret = ldg_thread_pool_start(&ctx->pool, sched_worker_loop, ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sched_init_unwind(ctx, LDG_SCHED_INIT_STAGE_POOL, deque_cunt, ret); }
    }

    ctx->fixed_accumulator = 0;
    ctx->should_stop = 0;
    ctx->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t sched_first_fault_keep(uint32_t kept, uint32_t code)
{
    if (kept != LDG_ERR_AOK) { return kept; }

    return code;
}

LDG_EXPORT uint32_t ldg_sched_ctx_shutdown(ldg_sched_ctx_t *ctx)
{
    uint32_t idx = 0;
    uint32_t step = LDG_ERR_AOK;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (ctx->conf.worker_cunt >= 2)
    {
        LDG_WR_ONCE(ctx->exec_stop, LDG_TRUTH_TRUE);
        ret = sched_cond_bcast(ctx, &ctx->work_cond, LDG_TRUTH_TRUE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_thread_pool_shutdown(&ctx->pool);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = sched_wait_destroy(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ctx->conf.worker_cunt && idx < LDG_SCHED_WORKER_MAX; idx++) { step = sched_first_fault_keep(step, ldg_sched_worker_deque_shutdown(&ctx->worker_deques[idx])); }

    if (ctx->conf.tracing_en == LDG_TRUTH_TRUE) { step = sched_first_fault_keep(step, ldg_sched_trace_shutdown(&ctx->trace)); }

    step = sched_first_fault_keep(step, ldg_math_stat_svm_shutdown(&ctx->svm));
    step = sched_first_fault_keep(step, ldg_math_stat_bayes_shutdown(&ctx->bayes));
    step = sched_first_fault_keep(step, ldg_sched_job_pool_shutdown(&ctx->job_pool));
    step = sched_first_fault_keep(step, ldg_time_timer_wheel_shutdown(&ctx->timer_wheel));
    step = sched_first_fault_keep(step, ldg_sched_event_pool_shutdown(&ctx->event_pool));
    step = sched_first_fault_keep(step, ldg_math_astar_ctx_destroy(&ctx->astar));

    ctx->is_init = LDG_TRUTH_FALSE;

    return step;
}

LDG_EXPORT uint32_t ldg_sched_ctx_spin(ldg_sched_ctx_t *ctx)
{
    ldg_sched_step_ctx_t step_ctx = LDG_STRUCT_ZERO_INIT;
    uint64_t spin_dt = 0;
    uint64_t total_time = 0;
    uint64_t spin = 0;
    uint64_t reclaimed = 0;
    uint32_t ret = 0;
    uint8_t busy = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_sched_ctx_event_emit(ctx, ctx->phase_events[LDG_SCHED_PHASE_START]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (!LDG_RD_ONCE(ctx->should_stop))
    {
        ldg_time_tick(&ctx->time_ctx);

        ret = ldg_time_dt_get(&ctx->time_ctx, &spin_dt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (spin_dt > LDG_SCHED_SPIN_DT_MAX) { spin_dt = LDG_SCHED_SPIN_DT_MAX; }

        ret = ldg_time_get(&ctx->time_ctx, &total_time);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        spin = ldg_time_frame_cunt_get(&ctx->time_ctx);

        ret = ldg_sched_ctx_event_emit(ctx, ctx->phase_events[LDG_SCHED_PHASE_SPIN_BEGIN]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->fixed_accumulator += spin_dt * (uint64_t)ctx->conf.fixed_rate;

        busy = LDG_TRUTH_FALSE;

        while (ctx->fixed_accumulator >= LDG_MS_PER_SEC)
        {
            busy = LDG_TRUTH_TRUE;
            step_ctx.dt = LDG_SCHED_STEP_DT_NONE;
            step_ctx.time = total_time;
            step_ctx.spin = spin;
            step_ctx.fixed_rate = ctx->conf.fixed_rate;

            ret = ldg_sched_ctx_event_ctx_emit(ctx, ctx->phase_events[LDG_SCHED_PHASE_FIXED_STEP], &step_ctx, (uint64_t)sizeof(step_ctx));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ctx->fixed_accumulator -= LDG_MS_PER_SEC;
        }

        ret = ldg_sched_job_done_reclaim(&ctx->job_pool, &reclaimed);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ldg_sched_job_cunt_get(&ctx->job_pool) > 0)
        {
            busy = LDG_TRUTH_TRUE;
            ret = ldg_sched_ctx_drain(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = ldg_time_timer_wheel_tick(&ctx->timer_wheel, total_time * LDG_NS_PER_MS);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        step_ctx.dt = spin_dt;
        step_ctx.time = total_time;
        step_ctx.spin = spin;
        step_ctx.fixed_rate = LDG_SCHED_FIXED_RATE_NONE;

        ret = ldg_sched_ctx_event_ctx_emit(ctx, ctx->phase_events[LDG_SCHED_PHASE_STEP], &step_ctx, (uint64_t)sizeof(step_ctx));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_sched_ctx_event_ctx_emit(ctx, ctx->phase_events[LDG_SCHED_PHASE_LATE_STEP], &step_ctx, (uint64_t)sizeof(step_ctx));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_sched_ctx_event_emit(ctx, ctx->phase_events[LDG_SCHED_PHASE_SPIN_END]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (busy == LDG_TRUTH_TRUE) { ret = ldg_thread_yield(LDG_SCHED_YIELD_SLICE); }
        else { ret = sched_spin_idle_wait(ctx, sched_spin_wait_ms_get(ctx)); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_sched_ctx_event_emit(ctx, ctx->phase_events[LDG_SCHED_PHASE_STOP]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_stop(ldg_sched_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    LDG_WR_ONCE(ctx->should_stop, LDG_TRUTH_TRUE);

    return sched_spin_wake(ctx);
}

static uint32_t sched_ctx_est_apply(ldg_sched_ctx_t *ctx)
{
    double est = 0.0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t state = 0;

    if (ctx->conf.est_strat != LDG_SCHED_EST_STRAT_BAYES) { return LDG_ERR_AOK; }

    for (idx = 0; idx < LDG_SCHED_JOB_MAX; idx++)
    {
        state = ctx->job_pool.jobs[idx].state;
        if (state != LDG_SCHED_JOB_STATE_READY && state != LDG_SCHED_JOB_STATE_PENDING && state != LDG_SCHED_JOB_STATE_YIELDED) { continue; }

        if (ctx->job_pool.jobs[idx].job_class >= LDG_MATH_STAT_BAYES_KIND_MAX) { continue; }

        ret = ldg_math_stat_bayes_proj(&ctx->bayes, ctx->job_pool.jobs[idx].job_class, &est);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->job_pool.jobs[idx].est_duration = LDG_SCHED_DURATION_NONE;
        if (est > 0.0) { ctx->job_pool.jobs[idx].est_duration = (uint64_t)est; }
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_drain(ldg_sched_ctx_t *ctx)
{
    ldg_sched_job_res_t res = LDG_SCHED_JOB_RES_DONE;
    uint64_t order_cunt = 0;
    uint64_t wave_cunt = 0;
    uint64_t idx = 0;
    uint64_t max_wave = 0;
    uint64_t wave = 0;
    ldg_math_rational_t elapsed = LDG_STRUCT_ZERO_INIT;
    uint64_t now = 0;
    uint64_t start = 0;
    uint32_t ord = 0;
    uint32_t job_idx = 0;
    uint8_t state = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (ctx->conf.worker_cunt < LDG_SCHED_MULTI_WORKER_MIN)
    {
        ret = sched_ctx_est_apply(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_sched_job_plot(&ctx->job_pool, ctx->astar, ctx->job_order, (uint64_t)LDG_SCHED_JOB_MAX, &order_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        for (idx = 0; idx < order_cunt; idx++)
        {
            ret = ldg_sched_job_run(&ctx->job_pool, ctx->job_order[idx], &res);
            if (ret != LDG_ERR_AOK) { continue; }

            if (ctx->conf.est_strat == LDG_SCHED_EST_STRAT_BAYES && res == LDG_SCHED_JOB_RES_DONE)
            {
                job_idx = ctx->job_order[idx] - LDG_SCHED_STEP;
                ret = ldg_math_stat_bayes_observe(&ctx->bayes, ctx->job_pool.jobs[job_idx].job_class, (double)ctx->job_pool.jobs[job_idx].actual_duration);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_RANGE)) { return ret; }
            }
        }

        return LDG_ERR_AOK;
    }

    max_wave = (uint64_t)(LDG_SCHED_WORKER_DEQUE_SIZE - LDG_SCHED_STEP);

    ret = ldg_time_monotonic_get(&start);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (wave = 0; wave < LDG_SCHED_DRAIN_WAVE_MAX; wave++)
    {
        ret = ldg_time_monotonic_get(&now);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(now < start || now - start > (uint64_t)INT64_MAX)) { return LDG_ERR_TIMEOUT; }

        ret = ldg_math_rational_set((int64_t)(now - start), LDG_MATH_RATIONAL_WHOLE_DEN, &elapsed);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_rational_cmp(&elapsed, &ctx->job_pool.frame_deadline, &ord);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ord != LDG_ARITH_ORD_LT) { return LDG_ERR_TIMEOUT; }

        ret = sched_ctx_est_apply(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_sched_job_plot(&ctx->job_pool, ctx->astar, ctx->job_order, (uint64_t)LDG_SCHED_JOB_MAX, &order_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        wave_cunt = 0;
        for (idx = 0; idx < order_cunt; idx++)
        {
            if (wave_cunt >= max_wave) { break; }

            state = ldg_sched_job_state_get(&ctx->job_pool, ctx->job_order[idx]);
            if (state == LDG_SCHED_JOB_STATE_READY || state == LDG_SCHED_JOB_STATE_YIELDED)
            {
                ctx->job_order[wave_cunt] = ctx->job_order[idx];
                wave_cunt++;
            }
        }

        if (wave_cunt == 0) { break; }

        LDG_FETCH_ADD(ctx->active_jobs, (uint32_t)wave_cunt);

        for (idx = 0; idx < wave_cunt; idx++)
        {
            ret = ldg_sched_worker_deque_push(&ctx->worker_deques[0], ctx->job_order[idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
            {
                LDG_FETCH_SUB(ctx->active_jobs, LDG_SCHED_STEP);
                LDG_ERRLOG_ERR("sched: drain wave push failed; job dropped");
            }
        }

        ret = sched_cond_bcast(ctx, &ctx->work_cond, LDG_TRUTH_TRUE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = sched_drain_wait(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (LDG_UNLIKELY(wave >= LDG_SCHED_DRAIN_WAVE_MAX)) { return LDG_ERR_TIMEOUT; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_phase_event_get(ldg_sched_ctx_t *ctx, ldg_sched_phase_t phase, ldg_sched_event_desc_t *out_desc)
{
    if (LDG_UNLIKELY(!ctx || !out_desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out_desc = LDG_SCHED_EVENT_DESC_INVALID;

    if (LDG_UNLIKELY((uint32_t)phase >= (uint32_t)LDG_SCHED_PHASE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out_desc = ctx->phase_events[phase];

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_event_create(ldg_sched_ctx_t *ctx, uint64_t filter_mask, ldg_sched_event_desc_t *out_desc)
{
    if (LDG_UNLIKELY(!ctx || !out_desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_sched_event_create(&ctx->event_pool, filter_mask, out_desc);
}

LDG_EXPORT uint32_t ldg_sched_ctx_event_destroy(ldg_sched_ctx_t *ctx, ldg_sched_event_desc_t desc)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_sched_event_destroy(&ctx->event_pool, desc);
}

LDG_EXPORT uint32_t ldg_sched_ctx_event_sub(ldg_sched_ctx_t *ctx, ldg_sched_event_desc_t desc, ldg_sched_event_handler_cb_t handler, uint64_t filter_mask, uint8_t prio, void *user_data, uint32_t *out_sub_id)
{
    if (LDG_UNLIKELY(!ctx || !out_sub_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_sched_event_sub(&ctx->event_pool, desc, handler, filter_mask, prio, user_data, out_sub_id);
}

LDG_EXPORT uint32_t ldg_sched_ctx_event_unsub(ldg_sched_ctx_t *ctx, uint32_t sub_id)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_sched_event_unsub(&ctx->event_pool, sub_id);
}

LDG_EXPORT uint32_t ldg_sched_ctx_event_emit(ldg_sched_ctx_t *ctx, ldg_sched_event_desc_t desc)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_sched_event_emit(&ctx->event_pool, desc);
}

LDG_EXPORT uint32_t ldg_sched_ctx_event_ctx_emit(ldg_sched_ctx_t *ctx, ldg_sched_event_desc_t desc, void *event_ctx, uint64_t ctx_size)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_sched_event_ctx_emit(&ctx->event_pool, desc, event_ctx, ctx_size);
}

LDG_EXPORT uint32_t ldg_sched_ctx_timer_create(ldg_sched_ctx_t *ctx, uint64_t interval_ns, uint8_t repeat, ldg_time_timer_cb_t func, void *arg, ldg_time_timer_id_t *out_id)
{
    if (LDG_UNLIKELY(!ctx || !out_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_time_timer_create(&ctx->timer_wheel, interval_ns, repeat, func, arg, out_id);
}

LDG_EXPORT uint32_t ldg_sched_ctx_timer_destroy(ldg_sched_ctx_t *ctx, ldg_time_timer_id_t id)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_time_timer_destroy(&ctx->timer_wheel, id);
}

LDG_EXPORT uint32_t ldg_sched_ctx_job_create(ldg_sched_ctx_t *ctx, ldg_sched_job_func_cb_t func, void *arg, uint8_t prio, uint8_t proc_affinity, ldg_sched_job_id_t *out_id)
{
    if (LDG_UNLIKELY(!ctx || !out_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_sched_job_create(&ctx->job_pool, func, arg, prio, proc_affinity, out_id);
}

LDG_EXPORT uint32_t ldg_sched_ctx_job_submit(ldg_sched_ctx_t *ctx, ldg_sched_job_id_t id)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_sched_job_submit(&ctx->job_pool, id);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return sched_spin_wake(ctx);
}

LDG_EXPORT uint32_t ldg_sched_ctx_job_dep_add(ldg_sched_ctx_t *ctx, ldg_sched_job_id_t job, ldg_sched_job_id_t dep)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_sched_job_dep_add(&ctx->job_pool, job, dep);
}

LDG_EXPORT uint32_t ldg_sched_ctx_job_class_set(ldg_sched_ctx_t *ctx, ldg_sched_job_id_t id, uint16_t job_class)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(job_class >= LDG_MATH_STAT_BAYES_KIND_MAX)) { return LDG_ERR_RANGE; }

    return ldg_sched_job_class_set(&ctx->job_pool, id, job_class);
}

LDG_EXPORT uint32_t ldg_sched_ctx_job_deadline_set(ldg_sched_ctx_t *ctx, ldg_sched_job_id_t id, uint64_t deadline)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_sched_job_deadline_set(&ctx->job_pool, id, deadline);
}

LDG_EXPORT uint32_t ldg_sched_ctx_est_strat_set(ldg_sched_ctx_t *ctx, uint32_t est_strat)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(est_strat >= LDG_SCHED_EST_STRAT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->conf.est_strat = est_strat;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_est_strat_get(ldg_sched_ctx_t *ctx, uint32_t *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = 0;

    *out = ctx->conf.est_strat;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_trace_push(ldg_sched_ctx_t *ctx, ldg_sched_trace_type_t type, uint32_t data0, uint64_t data1)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->conf.tracing_en != LDG_TRUTH_TRUE)) { return LDG_ERR_AOK; }

    return ldg_sched_trace_push(&ctx->trace, type, data0, data1);
}

LDG_EXPORT uint32_t ldg_sched_ctx_dt_get(ldg_sched_ctx_t *ctx, uint64_t *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = 0;

    return ldg_time_dt_get(&ctx->time_ctx, out);
}

LDG_EXPORT uint32_t ldg_sched_ctx_time_get(ldg_sched_ctx_t *ctx, uint64_t *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = 0;

    return ldg_time_get(&ctx->time_ctx, out);
}

LDG_EXPORT uint64_t ldg_sched_ctx_spin_get(ldg_sched_ctx_t *ctx)
{
    uint64_t res = 0;

    if (LDG_UNLIKELY(!ctx || ctx->is_init != LDG_TRUTH_TRUE))
    {
        res = UINT64_MAX;
        return res;
    }

    res = ldg_time_frame_cunt_get(&ctx->time_ctx);

    return res;
}
