
#include <dangling/sched/job.h>
#include <dangling/math/astar.h>
#include <dangling/math/math.h>
#include <dangling/arch/fence.h>
#include <dangling/arch/atomic.h>
#include <dangling/time/time.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

#define LDG_SCHED_PLOT_DEADLINE_BIAS 1
#define LDG_SCHED_RECLAIM_REF_NONE 0
#define LDG_SCHED_RECLAIM_REF_HELD 1

static uint32_t sched_job_pool_lock(ldg_sched_job_pool_t *pool)
{
    uint32_t expected = 0;
    uint32_t spin = 0;

    for (spin = 0; spin < LDG_SCHED_JOB_LOCK_SPIN_MAX; spin++)
    {
        expected = LDG_SCHED_LOCK_FREE;
        if (LDG_CAS(&pool->lock, &expected, LDG_SCHED_LOCK_HELD)) { return LDG_ERR_AOK; }

        LDG_PAUSE;
    }

    return LDG_ERR_BUSY;
}

static void sched_job_ready_promote(ldg_sched_job_t *job, uint8_t from)
{
    uint8_t expected = from;

    if (LDG_CAS(&job->state, &expected, (uint8_t)LDG_SCHED_JOB_STATE_READY)) { return; }
}

LDG_EXPORT uint32_t ldg_sched_job_pool_init(ldg_sched_job_pool_t *pool)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_zero(pool, sizeof(ldg_sched_job_pool_t)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    pool->is_init = UINT8_MAX;

    for (idx = 0; idx < LDG_SCHED_JOB_MAX; idx++)
    {
        pool->free_list[idx] = idx;
        pool->jobs[idx].id = idx + 1;
        pool->jobs[idx].state = LDG_SCHED_JOB_STATE_FREE;
        pool->jobs[idx].is_static = LDG_TRUTH_FALSE;
    }

    pool->free_hd = 0;
    pool->job_cunt = 0;
    if (LDG_UNLIKELY(ldg_math_rational_set(LDG_SCHED_FRAME_DEADLINE_NUM_DEFAULT, LDG_SCHED_FRAME_DEADLINE_DEN_DEFAULT, &pool->frame_deadline) != LDG_ERR_AOK)) { return LDG_ERR_MATH_VAL_INVALID; }

    if (LDG_UNLIKELY(ldg_math_rational_set(LDG_SCHED_PLOT_WEIGHT_CRIT_NUM_DEFAULT, LDG_SCHED_PLOT_WEIGHT_CRIT_DEN_DEFAULT, &pool->plot_weight_crit) != LDG_ERR_AOK)) { return LDG_ERR_MATH_VAL_INVALID; }

    if (LDG_UNLIKELY(ldg_math_rational_set(LDG_SCHED_PLOT_WEIGHT_REMAINING_NUM_DEFAULT, LDG_SCHED_PLOT_WEIGHT_REMAINING_DEN_DEFAULT, &pool->plot_weight_remaining) != LDG_ERR_AOK)) { return LDG_ERR_MATH_VAL_INVALID; }

    if (LDG_UNLIKELY(ldg_math_rational_set(LDG_SCHED_PLOT_WEIGHT_DEADLINE_NUM_DEFAULT, LDG_SCHED_PLOT_WEIGHT_DEADLINE_DEN_DEFAULT, &pool->plot_weight_deadline) != LDG_ERR_AOK)) { return LDG_ERR_MATH_VAL_INVALID; }

    pool->prio_cb = 0x0;
    pool->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_pool_shutdown(ldg_sched_job_pool_t *pool)
{
    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    pool->is_init = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_create(ldg_sched_job_pool_t *pool, ldg_sched_job_func_cb_t func, void *arg, uint8_t prio, uint8_t proc_affinity, ldg_sched_job_id_t *out_id)
{
    ldg_sched_job_t *job = 0x0;
    uint64_t hd = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !func || !out_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out_id = LDG_SCHED_JOB_ID_INVALID;

    if (LDG_UNLIKELY(prio >= LDG_SCHED_PRIO_LEVELS)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = sched_job_pool_lock(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    hd = pool->free_hd;
    if (LDG_UNLIKELY(hd >= LDG_SCHED_JOB_MAX))
    {
        LDG_ST_RELEASE(pool->lock, LDG_SCHED_LOCK_FREE);
        return LDG_ERR_FULL;
    }

    idx = pool->free_list[hd];
    pool->free_hd = hd + LDG_SCHED_STEP;
    pool->job_cunt++;

    job = &pool->jobs[idx];
    job->func = func;
    job->arg = arg;
    job->yield_ctx = 0x0;
    job->est_duration = LDG_SCHED_DURATION_NONE;
    job->actual_duration = LDG_SCHED_DURATION_NONE;
    job->submit_time = 0;
    job->start_time = 0;
    job->end_time = 0;
    job->id = idx + LDG_SCHED_JOB_ID_BASE;
    job->cont = LDG_SCHED_JOB_ID_INVALID;
    job->dep_cunt = 0;
    job->deps_remaining = 0;
    job->prio = prio;
    job->proc_affinity = proc_affinity;
    job->is_static = LDG_TRUTH_FALSE;
    job->job_class = 0;
    job->deadline = LDG_SCHED_DURATION_NONE;

    if (LDG_UNLIKELY(ldg_mem_fill(job->deps, LDG_SCHED_JOB_ID_INVALID, LDG_SCHED_DEP_MAX * sizeof(ldg_sched_job_id_t)) != LDG_ERR_AOK))
    {
        LDG_ST_RELEASE(pool->lock, LDG_SCHED_LOCK_FREE);
        return LDG_ERR_INVALID;
    }

    LDG_ST_RELEASE(job->state, LDG_SCHED_JOB_STATE_CREATED);

    *out_id = job->id;

    LDG_ST_RELEASE(pool->lock, LDG_SCHED_LOCK_FREE);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_destroy(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id)
{
    ldg_sched_job_t *job = 0x0;
    uint64_t hd = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - LDG_SCHED_JOB_ID_BASE;
    job = &pool->jobs[idx];

    ret = sched_job_pool_lock(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(job->state == LDG_SCHED_JOB_STATE_FREE))
    {
        LDG_ST_RELEASE(pool->lock, LDG_SCHED_LOCK_FREE);
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    job->state = LDG_SCHED_JOB_STATE_FREE;
    job->func = 0x0;
    job->arg = 0x0;

    hd = pool->free_hd;

    if (hd > 0)
    {
        pool->free_hd = hd - 1;
        pool->free_list[hd - 1] = idx;
    }

    pool->job_cunt--;

    LDG_ST_RELEASE(pool->lock, LDG_SCHED_LOCK_FREE);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_done_reclaim(ldg_sched_job_pool_t *pool, uint64_t *out_cunt)
{
    ldg_sched_job_t *job = 0x0;
    ldg_sched_job_id_t dep_id = 0;
    uint64_t hd = 0;
    uint64_t freed = 0;
    uint32_t idx = 0;
    uint32_t dep_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = sched_job_pool_lock(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < LDG_SCHED_JOB_MAX; idx++) { pool->plot_crit_state[idx] = LDG_SCHED_RECLAIM_REF_NONE; }

    for (idx = 0; idx < LDG_SCHED_JOB_MAX; idx++)
    {
        job = &pool->jobs[idx];

        if (job->state == LDG_SCHED_JOB_STATE_FREE || job->state == LDG_SCHED_JOB_STATE_DONE) { continue; }

        for (dep_idx = 0; dep_idx < job->dep_cunt && dep_idx < LDG_SCHED_DEP_MAX; dep_idx++)
        {
            dep_id = job->deps[dep_idx];

            if (dep_id == LDG_SCHED_JOB_ID_INVALID || dep_id > LDG_SCHED_JOB_MAX) { continue; }

            pool->plot_crit_state[dep_id - LDG_SCHED_JOB_ID_BASE] = LDG_SCHED_RECLAIM_REF_HELD;
        }
    }

    for (idx = 0; idx < LDG_SCHED_JOB_MAX; idx++)
    {
        job = &pool->jobs[idx];

        if (job->state != LDG_SCHED_JOB_STATE_DONE || job->is_static == LDG_TRUTH_TRUE) { continue; }

        if (pool->plot_crit_state[idx] == LDG_SCHED_RECLAIM_REF_HELD) { continue; }

        job->state = LDG_SCHED_JOB_STATE_FREE;
        job->func = 0x0;
        job->arg = 0x0;

        hd = pool->free_hd;

        if (hd > 0)
        {
            pool->free_hd = hd - LDG_SCHED_STEP;
            pool->free_list[hd - LDG_SCHED_STEP] = idx;
        }

        pool->job_cunt--;
        freed++;
    }

    LDG_ST_RELEASE(pool->lock, LDG_SCHED_LOCK_FREE);

    *out_cunt = freed;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_dep_add(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t job_id, ldg_sched_job_id_t dep_id)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;
    uint32_t dep_idx = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(job_id == LDG_SCHED_JOB_ID_INVALID || job_id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(dep_id == LDG_SCHED_JOB_ID_INVALID || dep_id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = job_id - 1;
    job = &pool->jobs[idx];

    if (LDG_UNLIKELY(job->state == LDG_SCHED_JOB_STATE_FREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (dep_idx = 0; dep_idx < job->dep_cunt; dep_idx++) { if (job->deps[dep_idx] == dep_id) { return LDG_ERR_AOK; } }

    if (LDG_UNLIKELY(job->dep_cunt >= LDG_SCHED_DEP_MAX)) { return LDG_ERR_FULL; }

    if (LDG_UNLIKELY(LDG_LD_ACQUIRE(pool->jobs[dep_id - LDG_SCHED_STEP].state) == LDG_SCHED_JOB_STATE_FREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    job->deps[job->dep_cunt] = dep_id;
    job->dep_cunt++;

    if (LDG_LD_ACQUIRE(pool->jobs[dep_id - LDG_SCHED_STEP].state) == LDG_SCHED_JOB_STATE_DONE) { return LDG_ERR_AOK; }

    LDG_FETCH_ADD(job->deps_remaining, (uint8_t)LDG_SCHED_STEP);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_dep_sig(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t completed_id)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t job_idx = 0;
    uint32_t dep_idx = 0;
    uint32_t attempt = 0;
    uint32_t ret = 0;
    uint8_t old = 0;
    uint8_t state = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(completed_id == LDG_SCHED_JOB_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (job_idx = 0; job_idx < LDG_SCHED_JOB_MAX; job_idx++)
    {
        job = &pool->jobs[job_idx];
        state = LDG_LD_ACQUIRE(job->state);

        if (state != LDG_SCHED_JOB_STATE_PENDING && state != LDG_SCHED_JOB_STATE_CREATED) { continue; }

        for (dep_idx = 0; dep_idx < job->dep_cunt; dep_idx++)
        {
            if (job->deps[dep_idx] != completed_id) { continue; }

            for (attempt = 0; attempt < LDG_SCHED_JOB_CAS_RETRY_MAX; attempt++)
            {
                old = LDG_RD_ONCE(job->deps_remaining);

                if (old == 0) { break; }

                if (LDG_CAS(&job->deps_remaining, &old, (uint8_t)(old - LDG_SCHED_STEP)))
                {
                    if (old == LDG_SCHED_STEP) { sched_job_ready_promote(job, LDG_SCHED_JOB_STATE_PENDING); }

                    break;
                }
            }

            if (LDG_UNLIKELY(attempt >= LDG_SCHED_JOB_CAS_RETRY_MAX)) { ret = LDG_ERR_BUSY; }

            break;
        }
    }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_cont_set(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t job_id, ldg_sched_job_id_t cont)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(job_id == LDG_SCHED_JOB_ID_INVALID || job_id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = job_id - 1;
    job = &pool->jobs[idx];

    if (LDG_UNLIKELY(job->state == LDG_SCHED_JOB_STATE_FREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    job->cont = cont;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_submit(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - LDG_SCHED_JOB_ID_BASE;
    job = &pool->jobs[idx];

    if (LDG_UNLIKELY(LDG_LD_ACQUIRE(job->state) != LDG_SCHED_JOB_STATE_CREATED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_time_monotonic_ns_get(&job->submit_time);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    LDG_ST_RELEASE(job->state, LDG_SCHED_JOB_STATE_PENDING);

    if (LDG_RD_ONCE(job->deps_remaining) == 0) { sched_job_ready_promote(job, LDG_SCHED_JOB_STATE_PENDING); }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_run(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, ldg_sched_job_res_t *out_res)
{
    ldg_sched_job_t *job = 0x0;
    ldg_sched_job_res_t res = LDG_SCHED_JOB_RES_DONE;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t prev = 0;

    if (LDG_UNLIKELY(!pool || !out_res)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out_res = LDG_SCHED_JOB_RES_DONE;

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];

    prev = LDG_SCHED_JOB_STATE_READY;

    if (!LDG_CAS(&job->state, &prev, (uint8_t)LDG_SCHED_JOB_STATE_RUNNING))
    {
        prev = LDG_SCHED_JOB_STATE_YIELDED;

        if (!LDG_CAS(&job->state, &prev, (uint8_t)LDG_SCHED_JOB_STATE_RUNNING)) { return LDG_ERR_BUSY; }
    }

    ret = ldg_time_monotonic_ns_get(&job->start_time);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        LDG_ST_RELEASE(job->state, prev);
        return ret;
    }

    if (job->func) { res = job->func(job->id, job->arg, job->yield_ctx); }

    if (LDG_UNLIKELY(ldg_time_monotonic_ns_get(&job->end_time) != LDG_ERR_AOK || job->end_time < job->start_time)) { job->end_time = job->start_time; }

    if (res == LDG_SCHED_JOB_RES_YIELD)
    {
        job->state = LDG_SCHED_JOB_STATE_YIELDED;
        *out_res = res;
        return LDG_ERR_AOK;
    }

    job->state = LDG_SCHED_JOB_STATE_DONE;
    job->actual_duration = job->end_time - job->start_time;

    ret = ldg_sched_job_dep_sig(pool, id);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (job->cont != LDG_SCHED_JOB_ID_INVALID)
    {
        ret = ldg_sched_job_submit(pool, job->cont);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    *out_res = res;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_yield_ctx_set(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, void *ctx)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];
    job->yield_ctx = ctx;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_est_duration_set(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, uint64_t est)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];
    job->est_duration = est;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_actual_duration_get(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, uint64_t *out)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = 0;

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];
    *out = job->actual_duration;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_class_set(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, uint16_t job_class)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];
    job->job_class = job_class;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_class_get(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, uint16_t *out)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = 0;

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];
    *out = job->job_class;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_deadline_set(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, uint64_t deadline)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];
    job->deadline = deadline;

    return LDG_ERR_AOK;
}

static uint32_t sched_job_rational_uint64_set(uint64_t val, ldg_math_rational_t *out)
{
    if (LDG_UNLIKELY(val > (uint64_t)INT64_MAX)) { return LDG_ERR_RANGE; }

    return ldg_math_rational_set((int64_t)val, LDG_MATH_RATIONAL_WHOLE_DEN, out);
}

LDG_EXPORT uint32_t ldg_sched_prio_deadline_cb(const ldg_sched_job_t *job, ldg_math_rational_t *out)
{
    uint64_t deadline = 0;

    if (LDG_UNLIKELY(!job || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    deadline = job->deadline;
    if (deadline == LDG_SCHED_DURATION_NONE || deadline > LDG_SCHED_PRIO_DEADLINE_NONE) { deadline = LDG_SCHED_PRIO_DEADLINE_NONE; }

    return sched_job_rational_uint64_set(deadline, out);
}

LDG_EXPORT uint32_t ldg_sched_prio_fastest_cb(const ldg_sched_job_t *job, ldg_math_rational_t *out)
{
    if (LDG_UNLIKELY(!job || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    return sched_job_rational_uint64_set(job->est_duration, out);
}

LDG_EXPORT uint32_t ldg_sched_job_plot_weight_set(ldg_sched_job_pool_t *pool, const ldg_math_rational_t *weight_crit, const ldg_math_rational_t *weight_remaining, const ldg_math_rational_t *weight_deadline)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !weight_crit || !weight_remaining || !weight_deadline)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(weight_crit->num < 0 || weight_remaining->num < 0 || weight_deadline->num < 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(weight_crit->den == 0 || weight_remaining->den == 0 || weight_deadline->den == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_rational_set(weight_crit->num, weight_crit->den, &pool->plot_weight_crit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_rational_set(weight_remaining->num, weight_remaining->den, &pool->plot_weight_remaining);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_rational_set(weight_deadline->num, weight_deadline->den, &pool->plot_weight_deadline);
}

LDG_EXPORT uint32_t ldg_sched_job_frame_deadline_set(ldg_sched_job_pool_t *pool, const ldg_math_rational_t *deadline)
{
    if (LDG_UNLIKELY(!pool || !deadline)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(deadline->num <= 0 || deadline->den == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ldg_math_rational_set(deadline->num, deadline->den, &pool->frame_deadline);
}

static uint32_t sched_job_crit_fill(ldg_sched_job_pool_t *pool)
{
    ldg_sched_job_t *job = 0x0;
    ldg_sched_job_id_t dep_id = 0;
    uint64_t visits = 0;
    uint32_t root = 0;
    uint32_t sp = 0;
    uint32_t curr = 0;
    uint32_t dep_idx = 0;

    for (root = 0; root < LDG_SCHED_JOB_MAX; root++)
    {
        pool->plot_crit[root] = LDG_SCHED_DURATION_NONE;
        pool->plot_crit_state[root] = LDG_SCHED_PLOT_CRIT_UNVISITED;
        pool->plot_crit_cursor[root] = 0;
    }

    for (root = 0; root < LDG_SCHED_JOB_MAX; root++)
    {
        if (pool->plot_crit_state[root] != LDG_SCHED_PLOT_CRIT_UNVISITED) { continue; }

        pool->plot_crit_state[root] = LDG_SCHED_PLOT_CRIT_ACTIVE;
        pool->plot_stack[0] = root;
        sp = LDG_SCHED_STEP;

        while (sp > 0)
        {
            visits++;
            if (LDG_UNLIKELY(visits > LDG_SCHED_PLOT_VISIT_MAX)) { return LDG_ERR_MEM_CORRUPTION; }

            curr = pool->plot_stack[sp - LDG_SCHED_STEP];
            job = &pool->jobs[curr];

            if (job->state == LDG_SCHED_JOB_STATE_FREE || job->state == LDG_SCHED_JOB_STATE_DONE)
            {
                pool->plot_crit[curr] = LDG_SCHED_DURATION_NONE;
                pool->plot_crit_state[curr] = LDG_SCHED_PLOT_CRIT_SETTLED;
                sp--;
                continue;
            }

            if (pool->plot_crit_cursor[curr] >= job->dep_cunt)
            {
                pool->plot_crit[curr] += job->est_duration;
                pool->plot_crit_state[curr] = LDG_SCHED_PLOT_CRIT_SETTLED;
                sp--;
                continue;
            }

            dep_id = job->deps[pool->plot_crit_cursor[curr]];
            pool->plot_crit_cursor[curr]++;

            if (dep_id == LDG_SCHED_JOB_ID_INVALID || dep_id > LDG_SCHED_JOB_MAX) { continue; }

            dep_idx = dep_id - LDG_SCHED_JOB_ID_BASE;

            if (pool->plot_crit_state[dep_idx] == LDG_SCHED_PLOT_CRIT_SETTLED)
            {
                if (pool->plot_crit[dep_idx] > pool->plot_crit[curr]) { pool->plot_crit[curr] = pool->plot_crit[dep_idx]; }

                continue;
            }

            if (pool->plot_crit_state[dep_idx] == LDG_SCHED_PLOT_CRIT_ACTIVE) { continue; }

            if (LDG_UNLIKELY(sp >= LDG_SCHED_JOB_MAX)) { return LDG_ERR_FULL; }

            pool->plot_crit_state[dep_idx] = LDG_SCHED_PLOT_CRIT_ACTIVE;
            pool->plot_stack[sp] = dep_idx;
            sp++;
        }
    }

    return LDG_ERR_AOK;
}

static uint64_t sched_job_total_remaining_get(const ldg_sched_job_pool_t *pool)
{
    uint64_t total = 0;
    uint32_t idx = 0;

    for (idx = 0; idx < LDG_SCHED_JOB_MAX; idx++) { if (pool->jobs[idx].state == LDG_SCHED_JOB_STATE_READY || pool->jobs[idx].state == LDG_SCHED_JOB_STATE_PENDING) { total += pool->jobs[idx].est_duration; } }

    return total;
}

static uint32_t sched_job_plot_heur(uint64_t node, uint64_t goal, void *user_ctx, ldg_math_rational_t *out)
{
    ldg_sched_job_pool_t *pool = 0x0;
    const ldg_sched_job_t *job = 0x0;
    ldg_math_rational_t term = LDG_STRUCT_ZERO_INIT;
    ldg_math_rational_t deadline_bias = LDG_STRUCT_ZERO_INIT;
    ldg_math_rational_t deadline_dist = LDG_STRUCT_ZERO_INIT;
    ldg_math_rational_t deadline_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_rational_t res = LDG_STRUCT_ZERO_INIT;
    uint64_t remaining = 0;
    uint32_t ret = 0;

    pool = (ldg_sched_job_pool_t *)user_ctx;

    if (node == goal || node >= LDG_SCHED_JOB_MAX) { return ldg_math_rational_set(LDG_SCHED_DURATION_NONE, LDG_MATH_RATIONAL_WHOLE_DEN, out); }

    job = &pool->jobs[node];

    if (pool->prio_cb)
    {
        ret = pool->prio_cb(job, out);
        if (ret == LDG_ERR_AOK) { return LDG_ERR_AOK; }
    }

    ret = ldg_math_rational_uint64_mul(&pool->plot_weight_crit, pool->plot_crit[node], &res);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    remaining = LDG_SCHED_DURATION_NONE;
    if (pool->plot_total_remaining > job->est_duration) { remaining = pool->plot_total_remaining - job->est_duration; }

    ret = ldg_math_rational_uint64_mul(&pool->plot_weight_remaining, remaining, &term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_rational_add(&res, &term, &res);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_rational_set(LDG_SCHED_PLOT_DEADLINE_BIAS, LDG_MATH_RATIONAL_WHOLE_DEN, &deadline_bias);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_rational_add(&deadline_bias, &pool->frame_deadline, &deadline_dist);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    deadline_val = pool->frame_deadline;
    if (job->deadline != LDG_SCHED_DURATION_NONE)
    {
        ret = sched_job_rational_uint64_set(job->deadline, &deadline_val);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_math_rational_mul(&pool->plot_weight_deadline, &deadline_val, &term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_rational_div(&term, &deadline_dist, &term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_rational_add(&res, &term, &res);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_rational_set((int64_t)(LDG_SCHED_PRIO_LEVELS - job->prio), LDG_MATH_RATIONAL_WHOLE_DEN, &term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_rational_sub(&res, &term, out);
}

static uint32_t sched_job_plot_succ(uint64_t node, uint64_t *nbr, ldg_math_rational_t *cost, uint64_t max_nbr, uint64_t *nbr_cunt, void *user_ctx)
{
    ldg_sched_job_pool_t *pool = 0x0;
    ldg_sched_job_t *cand = 0x0;
    ldg_math_rational_t edge_cost = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;
    uint64_t out_cunt = 0;
    ldg_sched_job_id_t curr_id = 0;
    ldg_sched_job_id_t dep_id = 0;
    uint32_t idx = 0;
    uint32_t dep_idx = 0;
    uint8_t depends = UINT8_MAX;
    uint8_t dep_pending = UINT8_MAX;

    pool = (ldg_sched_job_pool_t *)user_ctx;

    *nbr_cunt = 0;

    if (node == LDG_SCHED_JOB_MAX)
    {
        for (idx = 0; idx < LDG_SCHED_JOB_MAX; idx++) { if (pool->jobs[idx].state == LDG_SCHED_JOB_STATE_READY || pool->jobs[idx].state == LDG_SCHED_JOB_STATE_YIELDED)
            {
                if (out_cunt >= max_nbr) { return LDG_ERR_BOUNDS; }

                nbr[out_cunt] = (uint64_t)idx;
                ret = ldg_math_rational_set(LDG_SCHED_DURATION_NONE, LDG_MATH_RATIONAL_WHOLE_DEN, &cost[out_cunt]);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                out_cunt++;
            }
        }

        *nbr_cunt = out_cunt;

        return LDG_ERR_AOK;
    }

    if (node > LDG_SCHED_JOB_MAX) { return LDG_ERR_AOK; }

    curr_id = (ldg_sched_job_id_t)(node + LDG_SCHED_JOB_ID_BASE);
    ret = sched_job_rational_uint64_set(pool->jobs[node].est_duration, &edge_cost);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < LDG_SCHED_JOB_MAX; idx++)
    {
        cand = &pool->jobs[idx];

        if (cand->state != LDG_SCHED_JOB_STATE_PENDING) { continue; }

        depends = LDG_TRUTH_FALSE;
        for (dep_idx = 0; dep_idx < cand->dep_cunt; dep_idx++) { if (cand->deps[dep_idx] == curr_id)
            {
                depends = LDG_TRUTH_TRUE;
                break;
            }
        }

        if (depends != LDG_TRUTH_TRUE) { continue; }

        dep_pending = LDG_TRUTH_FALSE;
        for (dep_idx = 0; dep_idx < cand->dep_cunt; dep_idx++)
        {
            dep_id = cand->deps[dep_idx];

            if (dep_id == LDG_SCHED_JOB_ID_INVALID || dep_id > LDG_SCHED_JOB_MAX || dep_id == curr_id) { continue; }

            if (pool->jobs[dep_id - LDG_SCHED_JOB_ID_BASE].state == LDG_SCHED_JOB_STATE_PENDING || pool->jobs[dep_id - LDG_SCHED_JOB_ID_BASE].state == LDG_SCHED_JOB_STATE_CREATED)
            {
                dep_pending = LDG_TRUTH_TRUE;
                break;
            }
        }

        if (dep_pending == LDG_TRUTH_TRUE) { continue; }

        if (out_cunt >= max_nbr) { return LDG_ERR_BOUNDS; }

        nbr[out_cunt] = (uint64_t)idx;
        cost[out_cunt] = edge_cost;
        out_cunt++;
    }

    *nbr_cunt = out_cunt;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_plot(ldg_sched_job_pool_t *pool, ldg_math_astar_ctx_t *astar, ldg_sched_job_id_t *out_order, uint64_t max_cunt, uint64_t *out_cunt)
{
    uint64_t order_cunt = 0;
    uint64_t order_idx = 0;
    uint64_t copy_cunt = 0;
    uint64_t node = 0;
    ldg_math_rational_t total_cost = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !astar || !out_order || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(max_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = sched_job_pool_lock(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sched_job_crit_fill(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        LDG_ST_RELEASE(pool->lock, LDG_SCHED_LOCK_FREE);
        return ret;
    }

    pool->plot_total_remaining = sched_job_total_remaining_get(pool);

    ret = ldg_math_astar_traverse(astar, LDG_SCHED_PLOT_ROOT_NODE, sched_job_plot_succ, sched_job_plot_heur, pool, pool->plot_order, (uint64_t)LDG_SCHED_PLOT_NODE_CUNT, &order_cunt, &total_cost);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        LDG_ST_RELEASE(pool->lock, LDG_SCHED_LOCK_FREE);
        return ret;
    }

    for (order_idx = 0; order_idx < order_cunt; order_idx++)
    {
        node = pool->plot_order[order_idx];

        if (node >= LDG_SCHED_JOB_MAX) { continue; }

        if (copy_cunt >= max_cunt) { break; }

        out_order[copy_cunt] = (ldg_sched_job_id_t)(node + LDG_SCHED_JOB_ID_BASE);
        copy_cunt++;
    }

    *out_cunt = copy_cunt;

    LDG_ST_RELEASE(pool->lock, LDG_SCHED_LOCK_FREE);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_replot(ldg_sched_job_pool_t *pool, ldg_math_astar_ctx_t *astar, ldg_sched_job_id_t failed_job, ldg_sched_job_id_t *out_order, uint64_t max_cunt, uint64_t *out_cunt)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool || !astar || !out_order || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(failed_job == LDG_SCHED_JOB_ID_INVALID || failed_job > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = failed_job - 1;
    job = &pool->jobs[idx];

    if (job->state == LDG_SCHED_JOB_STATE_YIELDED) { job->state = LDG_SCHED_JOB_STATE_READY; }

    return ldg_sched_job_plot(pool, astar, out_order, max_cunt, out_cunt);
}

LDG_EXPORT uint32_t ldg_sched_worker_deque_init(ldg_sched_worker_deque_t *deque, uint32_t core_id, uint32_t owner_id)
{
    if (LDG_UNLIKELY(!deque)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_zero(deque, sizeof(ldg_sched_worker_deque_t)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    deque->top = 0;
    deque->bottom = 0;
    deque->core_id = core_id;
    deque->owner_id = owner_id;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_worker_deque_shutdown(ldg_sched_worker_deque_t *deque)
{
    if (LDG_UNLIKELY(!deque)) { return LDG_ERR_FUNC_ARG_NULL; }

    deque->top = 0;
    deque->bottom = 0;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_worker_deque_push(ldg_sched_worker_deque_t *deque, ldg_sched_job_id_t job)
{
    uint64_t bottom = 0;
    uint64_t top = 0;
    uint64_t size = 0;

    if (LDG_UNLIKELY(!deque)) { return LDG_ERR_FUNC_ARG_NULL; }

    bottom = LDG_RD_ONCE(deque->bottom);
    top = LDG_LD_ACQUIRE(deque->top);
    size = bottom - top;

    if (LDG_UNLIKELY(size >= LDG_SCHED_WORKER_DEQUE_SIZE - 1)) { return LDG_ERR_FULL; }

    LDG_WR_ONCE(deque->jobs[bottom % LDG_SCHED_WORKER_DEQUE_SIZE], job);

    LDG_ST_RELEASE(deque->bottom, bottom + 1);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_worker_deque_pop(ldg_sched_worker_deque_t *deque, ldg_sched_job_id_t *out_job)
{
    uint64_t bottom = 0;
    uint64_t top = 0;
    uint64_t expected = 0;
    ldg_sched_job_id_t job = 0;

    if (LDG_UNLIKELY(!deque || !out_job)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_job = LDG_SCHED_JOB_ID_INVALID;

    bottom = LDG_RD_ONCE(deque->bottom);
    if (bottom == 0) { return LDG_ERR_EMPTY; }

    bottom = bottom - 1;
    LDG_WR_ONCE(deque->bottom, bottom);
    LDG_SMP_MB();
    top = LDG_RD_ONCE(deque->top);

    if (top > bottom)
    {
        LDG_WR_ONCE(deque->bottom, top);
        return LDG_ERR_EMPTY;
    }

    job = LDG_RD_ONCE(deque->jobs[bottom % LDG_SCHED_WORKER_DEQUE_SIZE]);

    if (top == bottom)
    {
        expected = top;
        if (!LDG_CAS(&deque->top, &expected, top + 1)) { job = LDG_SCHED_JOB_ID_INVALID; }

        LDG_WR_ONCE(deque->bottom, top + 1);
    }

    if (job == LDG_SCHED_JOB_ID_INVALID) { return LDG_ERR_EMPTY; }

    *out_job = job;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_worker_deque_steal(ldg_sched_worker_deque_t *deque, ldg_sched_job_id_t *out_job)
{
    uint64_t top = 0;
    uint64_t bottom = 0;
    uint64_t expected = 0;
    ldg_sched_job_id_t job = 0;

    if (LDG_UNLIKELY(!deque || !out_job)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_job = LDG_SCHED_JOB_ID_INVALID;

    top = LDG_LD_ACQUIRE(deque->top);
    bottom = LDG_LD_ACQUIRE(deque->bottom);

    if (top >= bottom) { return LDG_ERR_EMPTY; }

    job = LDG_RD_ONCE(deque->jobs[top % LDG_SCHED_WORKER_DEQUE_SIZE]);

    expected = top;
    if (!LDG_CAS(&deque->top, &expected, top + 1)) { return LDG_ERR_EMPTY; }

    *out_job = job;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_sched_worker_deque_cunt_get(ldg_sched_worker_deque_t *deque)
{
    uint64_t top = 0;
    uint64_t bottom = 0;
    uint64_t res = 0;

    if (LDG_UNLIKELY(!deque))
    {
        res = UINT64_MAX;
        return res;
    }

    top = LDG_LD_ACQUIRE(deque->top);
    bottom = LDG_LD_ACQUIRE(deque->bottom);

    if (bottom <= top) { return res; }

    res = bottom - top;

    return res;
}

LDG_EXPORT uint8_t ldg_sched_job_ready_is(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];

    if (job->state == LDG_SCHED_JOB_STATE_READY) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

LDG_EXPORT uint8_t ldg_sched_job_state_get(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;
    uint8_t res = 0;

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE))
    {
        res = (uint8_t)UINT8_MAX;
        return res;
    }

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX))
    {
        res = (uint8_t)UINT8_MAX;
        return res;
    }

    idx = id - 1;
    job = &pool->jobs[idx];
    res = job->state;

    return res;
}

LDG_EXPORT uint64_t ldg_sched_job_cunt_get(ldg_sched_job_pool_t *pool)
{
    uint64_t res = 0;

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE))
    {
        res = UINT64_MAX;
        return res;
    }

    res = pool->job_cunt;

    return res;
}

LDG_EXPORT uint32_t ldg_sched_job_ready_collect(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t *out, uint64_t max, uint64_t *out_cunt)
{
    uint64_t cunt = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out_cunt = 0;

    for (idx = 0; idx < LDG_SCHED_JOB_MAX; idx++)
    {
        if (cunt >= max) { break; }

        if (pool->jobs[idx].state == LDG_SCHED_JOB_STATE_READY || pool->jobs[idx].state == LDG_SCHED_JOB_STATE_YIELDED)
        {
            out[cunt] = pool->jobs[idx].id;
            cunt++;
        }
    }

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}
