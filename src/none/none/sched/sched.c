#include <string.h>

#include <dangling/sched/sched.h>
#include <dangling/math/math.h>
#include <dangling/arch/amd64/fence.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

static uint32_t sched_worker_loop(void *arg)
{
    ldg_sched_ctx_t *ctx = 0x0;
    ldg_sched_job_res_t res = LDG_SCHED_JOB_RES_DONE;
    ldg_sched_job_id_t job = 0;
    uint32_t victim = 0;
    uint32_t got = 0;
    uint32_t job_idx = 0;
    uint32_t ret = 0;

    ctx = (ldg_sched_ctx_t *)arg;

    while (!LDG_RD_ONCE(ctx->exec_stop))
    {
        got = 0;

        for (victim = 0; victim < ctx->conf.worker_cunt; victim++) { if (ldg_sched_worker_deque_steal(&ctx->worker_deques[victim], &job) == LDG_ERR_AOK)
            {
                got = 1;
                break;
            }
        }

        if (got)
        {
            ret = ldg_sched_job_run(&ctx->job_pool, job, &res);

            if (ret == LDG_ERR_AOK && ctx->conf.est_strat == LDG_SCHED_EST_STRAT_BAYES && res == LDG_SCHED_JOB_RES_DONE)
            {
                job_idx = job - 1;
                ret = ldg_sched_bayes_observe(&ctx->bayes, ctx->job_pool.jobs[job_idx].job_class, ctx->job_pool.jobs[job_idx].actual_duration);
            }

            LDG_FETCH_SUB(ctx->active_jobs, 1);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_PAUSE; }
        }
        else{ LDG_PAUSE; }
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_conf_default(ldg_sched_conf_t *conf)
{
    if (LDG_UNLIKELY(!conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(conf, 0, sizeof(ldg_sched_conf_t)) != conf)) { return LDG_ERR_INVALID; }

    conf->fixed_dt = LDG_SCHED_FIXED_DT;
    conf->spin_budget_ms = LDG_SCHED_FRAME_DEADLINE_DEFAULT_MS;
    conf->plot_weight_crit = LDG_SCHED_PLOT_WEIGHT_CRIT_DEFAULT;
    conf->plot_weight_remaining = LDG_SCHED_PLOT_WEIGHT_REMAINING_DEFAULT;
    conf->plot_weight_deadline = LDG_SCHED_PLOT_WEIGHT_DEADLINE_DEFAULT;
    conf->svm_cost = LDG_SCHED_SVM_COST_DEFAULT;
    conf->svm_epsilon = LDG_SCHED_SVM_EPSILON_DEFAULT;
    conf->bayes_prior_m = LDG_SCHED_BAYES_PRIOR_M_DEFAULT;
    conf->bayes_prior_kappa = LDG_SCHED_BAYES_PRIOR_KAPPA_DEFAULT;
    conf->bayes_prior_alpha = LDG_SCHED_BAYES_PRIOR_ALPHA_DEFAULT;
    conf->bayes_prior_beta = LDG_SCHED_BAYES_PRIOR_BETA_DEFAULT;
    conf->bayes_risk_k = LDG_SCHED_BAYES_RISK_K_DEFAULT;
    conf->timer_tick_ns = LDG_NS_PER_MS;
    conf->est_strat = LDG_SCHED_EST_STRAT_MANUAL;
    conf->prio_cb = 0x0;
    conf->worker_cunt = LDG_SCHED_WORKER_CUNT_DEFAULT;
    conf->enable_tracing = 1;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_init(ldg_sched_ctx_t *ctx, const ldg_sched_conf_t *conf)
{
    ldg_sched_conf_t default_conf = LDG_STRUCT_ZERO_INIT;
    ldg_sched_event_desc_t desc = LDG_SCHED_EVENT_DESC_INVALID;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint32_t clean = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(ctx, 0, sizeof(ldg_sched_ctx_t)) != ctx)) { return LDG_ERR_INVALID; }

    if (!conf)
    {
        ret = ldg_sched_conf_default(&default_conf);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        conf = &default_conf;
    }

    ctx->conf = *conf;

    if (LDG_UNLIKELY(conf->fixed_dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_sched_event_pool_init(&ctx->event_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_time_timer_wheel_init(&ctx->timer_wheel, conf->timer_tick_ns);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_sched_job_pool_init(&ctx->job_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_sched_job_plot_weight_set(&ctx->job_pool, conf->plot_weight_crit, conf->plot_weight_remaining, conf->plot_weight_deadline);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_sched_job_frame_deadline_set(&ctx->job_pool, conf->spin_budget_ms);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->job_pool.prio_cb = conf->prio_cb;

    ret = ldg_sched_svm_init(&ctx->svm, conf->svm_cost, conf->svm_epsilon);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_sched_bayes_init(&ctx->bayes, conf->bayes_prior_m, conf->bayes_prior_kappa, conf->bayes_prior_alpha, conf->bayes_prior_beta, conf->bayes_risk_k);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < conf->worker_cunt && idx < LDG_SCHED_WORKER_MAX; idx++)
    {
        ret = ldg_sched_worker_deque_init(&ctx->worker_deques[idx], idx, idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_math_astar_ctx_create((uint64_t)LDG_SCHED_JOB_MAX + 1, (uint64_t)LDG_SCHED_JOB_MAX, &ctx->astar);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return LDG_ERR_ALLOC_NULL; }

    for (idx = 0; idx < LDG_SCHED_PHASE_MAX; idx++)
    {
        ret = ldg_sched_event_create(&ctx->event_pool, LDG_SCHED_EVENT_FILTER_CORE, &desc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            clean = ldg_math_astar_ctx_destroy(&ctx->astar);
            if (clean != LDG_ERR_AOK) { return clean; }

            return ret;
        }

        ctx->phase_events[idx] = desc;
    }

    ret = ldg_time_init(&ctx->time_ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        clean = ldg_math_astar_ctx_destroy(&ctx->astar);
        if (clean != LDG_ERR_AOK) { return clean; }

        return ret;
    }

    if (conf->enable_tracing)
    {
        ret = ldg_sched_trace_init(&ctx->trace);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            clean = ldg_math_astar_ctx_destroy(&ctx->astar);
            if (clean != LDG_ERR_AOK) { return clean; }

            return ret;
        }
    }

    if (ctx->conf.worker_cunt >= 2)
    {
        ret = ldg_thread_pool_init(&ctx->pool, ctx->conf.worker_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            if (conf->enable_tracing) { clean = ldg_sched_trace_shutdown(&ctx->trace); }

            clean = ldg_math_astar_ctx_destroy(&ctx->astar);
            if (clean != LDG_ERR_AOK) { return clean; }

            return ret;
        }

        ret = ldg_thread_pool_start(&ctx->pool, sched_worker_loop, ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            clean = ldg_thread_pool_shutdown(&ctx->pool);
            if (conf->enable_tracing) { clean = ldg_sched_trace_shutdown(&ctx->trace); }

            clean = ldg_math_astar_ctx_destroy(&ctx->astar);
            if (clean != LDG_ERR_AOK) { return clean; }

            return ret;
        }
    }

    ctx->fixed_accumulator = 0.0;
    ctx->should_stop = 0;
    ctx->is_init = 1;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_shutdown(ldg_sched_ctx_t *ctx)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->conf.worker_cunt >= 2)
    {
        LDG_WR_ONCE(ctx->exec_stop, 1);
        ret = ldg_thread_pool_shutdown(&ctx->pool);
    }

    for (idx = 0; idx < ctx->conf.worker_cunt && idx < LDG_SCHED_WORKER_MAX; idx++) { ret = ldg_sched_worker_deque_shutdown(&ctx->worker_deques[idx]); }

    if (ctx->conf.enable_tracing) { ret = ldg_sched_trace_shutdown(&ctx->trace); }

    ret = ldg_sched_svm_shutdown(&ctx->svm);
    ret = ldg_sched_bayes_shutdown(&ctx->bayes);
    ret = ldg_sched_job_pool_shutdown(&ctx->job_pool);
    ret = ldg_time_timer_wheel_shutdown(&ctx->timer_wheel);
    ret = ldg_sched_event_pool_shutdown(&ctx->event_pool);
    ret = ldg_math_astar_ctx_destroy(&ctx->astar);

    ctx->is_init = 0;

    return ret;
}

LDG_EXPORT uint32_t ldg_sched_ctx_spin(ldg_sched_ctx_t *ctx)
{
    ldg_sched_step_ctx_t step_ctx = LDG_STRUCT_ZERO_INIT;
    double spin_dt = 0.0;
    double total_time = 0.0;
    uint64_t spin = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

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

        ctx->fixed_accumulator += spin_dt;

        while (ctx->fixed_accumulator >= ctx->conf.fixed_dt)
        {
            step_ctx.dt = ctx->conf.fixed_dt;
            step_ctx.time = total_time;
            step_ctx.spin = spin;

            ret = ldg_sched_ctx_event_ctx_emit(ctx, ctx->phase_events[LDG_SCHED_PHASE_FIXED_STEP], &step_ctx, (uint64_t)sizeof(step_ctx));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ctx->fixed_accumulator -= ctx->conf.fixed_dt;
        }

        if (ldg_sched_job_cunt_get(&ctx->job_pool) > 0)
        {
            ret = ldg_sched_ctx_drain(ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = ldg_time_timer_wheel_tick(&ctx->timer_wheel, (uint64_t)(total_time * (double)LDG_NS_PER_SEC));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        step_ctx.dt = spin_dt;
        step_ctx.time = total_time;
        step_ctx.spin = spin;

        ret = ldg_sched_ctx_event_ctx_emit(ctx, ctx->phase_events[LDG_SCHED_PHASE_STEP], &step_ctx, (uint64_t)sizeof(step_ctx));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_sched_ctx_event_ctx_emit(ctx, ctx->phase_events[LDG_SCHED_PHASE_LATE_STEP], &step_ctx, (uint64_t)sizeof(step_ctx));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_sched_ctx_event_emit(ctx, ctx->phase_events[LDG_SCHED_PHASE_SPIN_END]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        LDG_PAUSE;
    }

    ret = ldg_sched_ctx_event_emit(ctx, ctx->phase_events[LDG_SCHED_PHASE_STOP]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_stop(ldg_sched_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    LDG_WR_ONCE(ctx->should_stop, 1);

    return LDG_ERR_AOK;
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

        if (ctx->job_pool.jobs[idx].job_class >= LDG_SCHED_BAYES_CLASS_MAX) { continue; }

        ret = ldg_sched_bayes_proj(&ctx->bayes, ctx->job_pool.jobs[idx].job_class, &est);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->job_pool.jobs[idx].est_duration = est;
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
    uint32_t job_idx = 0;
    uint8_t state = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->conf.worker_cunt < 2)
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
                job_idx = ctx->job_order[idx] - 1;
                ret = ldg_sched_bayes_observe(&ctx->bayes, ctx->job_pool.jobs[job_idx].job_class, ctx->job_pool.jobs[job_idx].actual_duration);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_RANGE)) { return ret; }
            }
        }

        return LDG_ERR_AOK;
    }

    max_wave = (uint64_t)(LDG_SCHED_WORKER_DEQUE_SIZE - 1);

    for (;;)
    {
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

        LDG_STORE_RELEASE(ctx->active_jobs, (uint32_t)wave_cunt);

        for (idx = 0; idx < wave_cunt; idx++)
        {
            ret = ldg_sched_worker_deque_push(&ctx->worker_deques[0], ctx->job_order[idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_FETCH_SUB(ctx->active_jobs, 1); }
        }

        while (LDG_LOAD_ACQUIRE(ctx->active_jobs) > 0) { LDG_PAUSE; }
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_phase_event_get(ldg_sched_ctx_t *ctx, ldg_sched_phase_t phase, ldg_sched_event_desc_t *out_desc)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !out_desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_desc = LDG_SCHED_EVENT_DESC_INVALID;

    if (LDG_UNLIKELY((uint32_t)phase >= (uint32_t)LDG_SCHED_PHASE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out_desc = ctx->phase_events[phase];

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_event_create(ldg_sched_ctx_t *ctx, uint64_t filter_mask, ldg_sched_event_desc_t *out_desc)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !out_desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_sched_event_create(&ctx->event_pool, filter_mask, out_desc);
}

LDG_EXPORT uint32_t ldg_sched_ctx_event_destroy(ldg_sched_ctx_t *ctx, ldg_sched_event_desc_t desc)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_sched_event_destroy(&ctx->event_pool, desc);
}

LDG_EXPORT uint32_t ldg_sched_ctx_event_sub(ldg_sched_ctx_t *ctx, ldg_sched_event_desc_t desc, ldg_sched_event_handler_cb_t handler, uint64_t filter_mask, uint8_t prio, void *user_data, uint32_t *out_sub_id)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !out_sub_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_sched_event_sub(&ctx->event_pool, desc, handler, filter_mask, prio, user_data, out_sub_id);
}

LDG_EXPORT uint32_t ldg_sched_ctx_event_unsub(ldg_sched_ctx_t *ctx, uint32_t sub_id)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_sched_event_unsub(&ctx->event_pool, sub_id);
}

LDG_EXPORT uint32_t ldg_sched_ctx_event_emit(ldg_sched_ctx_t *ctx, ldg_sched_event_desc_t desc)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_sched_event_emit(&ctx->event_pool, desc);
}

LDG_EXPORT uint32_t ldg_sched_ctx_event_ctx_emit(ldg_sched_ctx_t *ctx, ldg_sched_event_desc_t desc, void *event_ctx, uint64_t ctx_size)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_sched_event_ctx_emit(&ctx->event_pool, desc, event_ctx, ctx_size);
}

LDG_EXPORT uint32_t ldg_sched_ctx_timer_create(ldg_sched_ctx_t *ctx, uint64_t interval_ns, uint8_t repeat, ldg_time_timer_cb_t func, void *arg, ldg_time_timer_id_t *out_id)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !out_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_time_timer_create(&ctx->timer_wheel, interval_ns, repeat, func, arg, out_id);
}

LDG_EXPORT uint32_t ldg_sched_ctx_timer_destroy(ldg_sched_ctx_t *ctx, ldg_time_timer_id_t id)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_time_timer_destroy(&ctx->timer_wheel, id);
}

LDG_EXPORT uint32_t ldg_sched_ctx_job_create(ldg_sched_ctx_t *ctx, ldg_sched_job_func_cb_t func, void *arg, uint8_t prio, uint8_t proc_affinity, ldg_sched_job_id_t *out_id)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !out_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_sched_job_create(&ctx->job_pool, func, arg, prio, proc_affinity, out_id);
}

LDG_EXPORT uint32_t ldg_sched_ctx_job_submit(ldg_sched_ctx_t *ctx, ldg_sched_job_id_t id)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_sched_job_submit(&ctx->job_pool, id);
}

LDG_EXPORT uint32_t ldg_sched_ctx_job_dep_add(ldg_sched_ctx_t *ctx, ldg_sched_job_id_t job, ldg_sched_job_id_t dep)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_sched_job_dep_add(&ctx->job_pool, job, dep);
}

LDG_EXPORT uint32_t ldg_sched_ctx_job_class_set(ldg_sched_ctx_t *ctx, ldg_sched_job_id_t id, uint16_t job_class)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(job_class >= LDG_SCHED_BAYES_CLASS_MAX)) { return LDG_ERR_RANGE; }

    return ldg_sched_job_class_set(&ctx->job_pool, id, job_class);
}

LDG_EXPORT uint32_t ldg_sched_ctx_job_deadline_set(ldg_sched_ctx_t *ctx, ldg_sched_job_id_t id, double deadline)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_sched_job_deadline_set(&ctx->job_pool, id, deadline);
}

LDG_EXPORT uint32_t ldg_sched_ctx_est_strat_set(ldg_sched_ctx_t *ctx, uint32_t est_strat)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(est_strat >= LDG_SCHED_EST_STRAT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ctx->conf.est_strat = est_strat;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_est_strat_get(ldg_sched_ctx_t *ctx, uint32_t *out)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    *out = ctx->conf.est_strat;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_bayes_risk_k_set(ldg_sched_ctx_t *ctx, double risk_k)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_sched_bayes_risk_k_set(&ctx->bayes, risk_k);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->conf.bayes_risk_k = risk_k;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_ctx_bayes_observe(ldg_sched_ctx_t *ctx, uint16_t job_class, double duration)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_sched_bayes_observe(&ctx->bayes, job_class, duration);
}

LDG_EXPORT uint32_t ldg_sched_ctx_bayes_proj(ldg_sched_ctx_t *ctx, uint16_t job_class, double *out)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    return ldg_sched_bayes_proj(&ctx->bayes, job_class, out);
}

LDG_EXPORT uint32_t ldg_sched_ctx_trace_push(ldg_sched_ctx_t *ctx, ldg_sched_trace_type_t type, uint32_t data0, uint64_t data1)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->conf.enable_tracing)) { return LDG_ERR_AOK; }

    return ldg_sched_trace_push(&ctx->trace, type, data0, data1);
}

LDG_EXPORT uint32_t ldg_sched_ctx_dt_get(ldg_sched_ctx_t *ctx, double *out)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    return ldg_time_dt_get(&ctx->time_ctx, out);
}

LDG_EXPORT uint32_t ldg_sched_ctx_time_get(ldg_sched_ctx_t *ctx, double *out)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    return ldg_time_get(&ctx->time_ctx, out);
}

LDG_EXPORT uint64_t ldg_sched_ctx_spin_get(ldg_sched_ctx_t *ctx)
{
    uint64_t res = 0;

    if (LDG_UNLIKELY(!ctx || !ctx->is_init))
    {
        res = UINT64_MAX;
        return res;
    }

    res = ldg_time_frame_cunt_get(&ctx->time_ctx);

    return res;
}
