
#include <dangling/sched/job.h>
#include <dangling/math/astar.h>
#include <dangling/math/math.h>
#include <dangling/arch/fence.h>
#include <dangling/arch/atomic.h>
#include <dangling/time/time.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

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
    pool->frame_deadline = LDG_SCHED_FRAME_DEADLINE_DEFAULT_MS;
    pool->plot_weight_crit = LDG_SCHED_PLOT_WEIGHT_CRIT_DEFAULT;
    pool->plot_weight_remaining = LDG_SCHED_PLOT_WEIGHT_REMAINING_DEFAULT;
    pool->plot_weight_deadline = LDG_SCHED_PLOT_WEIGHT_DEADLINE_DEFAULT;
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

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE || !func || !out_id)) { return LDG_ERR_FUNC_ARG_NULL; }

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
    job->est_duration = 0.0;
    job->actual_duration = 0.0;
    job->submit_tsc = 0;
    job->start_tsc = 0;
    job->end_tsc = 0;
    job->id = idx + LDG_SCHED_JOB_ID_BASE;
    job->cont = LDG_SCHED_JOB_ID_INVALID;
    job->dep_cunt = 0;
    job->deps_remaining = 0;
    job->prio = prio;
    job->proc_affinity = proc_affinity;
    job->is_static = LDG_TRUTH_FALSE;
    job->job_class = 0;
    job->deadline = 0.0;

    if (LDG_UNLIKELY(ldg_mem_fill(job->deps, LDG_SCHED_JOB_ID_INVALID, LDG_SCHED_DEP_MAX * sizeof(ldg_sched_job_id_t)) != LDG_ERR_AOK))
    {
        LDG_ST_RELEASE(pool->lock, LDG_SCHED_LOCK_FREE);
        return LDG_ERR_INVALID;
    }

    LDG_ST_RELEASE(job->state, LDG_SCHED_JOB_STATE_PENDING);

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

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

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

LDG_EXPORT uint32_t ldg_sched_job_dep_add(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t job_id, ldg_sched_job_id_t dep_id)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;
    uint32_t dep_idx = 0;

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(job_id == LDG_SCHED_JOB_ID_INVALID || job_id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(dep_id == LDG_SCHED_JOB_ID_INVALID || dep_id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = job_id - 1;
    job = &pool->jobs[idx];

    if (LDG_UNLIKELY(job->state == LDG_SCHED_JOB_STATE_FREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (dep_idx = 0; dep_idx < job->dep_cunt; dep_idx++) { if (job->deps[dep_idx] == dep_id) { return LDG_ERR_AOK; } }

    if (LDG_UNLIKELY(job->dep_cunt >= LDG_SCHED_DEP_MAX)) { return LDG_ERR_FULL; }

    job->deps[job->dep_cunt] = dep_id;
    job->dep_cunt++;
    job->deps_remaining++;

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

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(completed_id == LDG_SCHED_JOB_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (job_idx = 0; job_idx < LDG_SCHED_JOB_MAX; job_idx++)
    {
        job = &pool->jobs[job_idx];

        if (LDG_LD_ACQUIRE(job->state) != LDG_SCHED_JOB_STATE_PENDING) { continue; }

        for (dep_idx = 0; dep_idx < job->dep_cunt; dep_idx++)
        {
            if (job->deps[dep_idx] != completed_id) { continue; }

            for (attempt = 0; attempt < LDG_SCHED_JOB_CAS_RETRY_MAX; attempt++)
            {
                old = LDG_RD_ONCE(job->deps_remaining);

                if (old == 0) { break; }

                if (LDG_CAS(&job->deps_remaining, &old, (uint8_t)(old - LDG_SCHED_STEP)))
                {
                    if (old == LDG_SCHED_STEP) { LDG_ST_RELEASE(job->state, LDG_SCHED_JOB_STATE_READY); }

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

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

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

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];

    if (LDG_UNLIKELY(job->state != LDG_SCHED_JOB_STATE_PENDING)) { return LDG_ERR_FUNC_ARG_INVALID; }

    job->submit_tsc = ldg_time_cycle_get();

    if (job->deps_remaining == 0) { job->state = LDG_SCHED_JOB_STATE_READY; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_run(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, ldg_sched_job_res_t *out_res)
{
    ldg_sched_job_t *job = 0x0;
    ldg_sched_job_res_t res = LDG_SCHED_JOB_RES_DONE;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE || !out_res)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_res = LDG_SCHED_JOB_RES_DONE;

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];

    if (LDG_UNLIKELY(job->state != LDG_SCHED_JOB_STATE_READY && job->state != LDG_SCHED_JOB_STATE_YIELDED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    job->state = LDG_SCHED_JOB_STATE_RUNNING;
    job->start_tsc = ldg_time_cycle_get();

    if (job->func) { res = job->func(job->id, job->arg, job->yield_ctx); }

    job->end_tsc = ldg_time_cycle_get();

    if (res == LDG_SCHED_JOB_RES_YIELD)
    {
        job->state = LDG_SCHED_JOB_STATE_YIELDED;
        *out_res = res;
        return LDG_ERR_AOK;
    }

    job->state = LDG_SCHED_JOB_STATE_DONE;
    job->actual_duration = (double)(job->end_tsc - job->start_tsc);

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

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];
    job->yield_ctx = ctx;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_est_duration_set(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, double est)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];
    job->est_duration = est;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_actual_duration_get(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, double *out)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

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

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

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

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];
    *out = job->job_class;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_deadline_set(ldg_sched_job_pool_t *pool, ldg_sched_job_id_t id, double deadline)
{
    ldg_sched_job_t *job = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(id == LDG_SCHED_JOB_ID_INVALID || id > LDG_SCHED_JOB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    job = &pool->jobs[idx];
    job->deadline = deadline;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_prio_deadline_cb(const ldg_sched_job_t *job, double *out)
{
    if (LDG_UNLIKELY(!job || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (job->deadline > 0.0) ? job->deadline : LDG_SCHED_PRIO_DEADLINE_NONE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_prio_fastest_cb(const ldg_sched_job_t *job, double *out)
{
    if (LDG_UNLIKELY(!job || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = job->est_duration;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_plot_weight_set(ldg_sched_job_pool_t *pool, double weight_crit, double weight_remaining, double weight_deadline)
{
    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    pool->plot_weight_crit = weight_crit;
    pool->plot_weight_remaining = weight_remaining;
    pool->plot_weight_deadline = weight_deadline;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_job_frame_deadline_set(ldg_sched_job_pool_t *pool, double deadline)
{
    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!(deadline > 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    pool->frame_deadline = deadline;

    return LDG_ERR_AOK;
}

static uint32_t sched_job_crit_fill(ldg_sched_job_pool_t *pool)
{
    ldg_sched_job_t *job = 0x0;
    ldg_sched_job_id_t dep_id = 0;
    uint64_t visits = 0;
    uint32_t root = 0;
    uint32_t sp = 0;
    uint32_t cur = 0;
    uint32_t dep_idx = 0;

    for (root = 0; root < LDG_SCHED_JOB_MAX; root++)
    {
        pool->plot_crit[root] = 0.0;
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

            cur = pool->plot_stack[sp - LDG_SCHED_STEP];
            job = &pool->jobs[cur];

            if (job->state == LDG_SCHED_JOB_STATE_FREE || job->state == LDG_SCHED_JOB_STATE_DONE)
            {
                pool->plot_crit[cur] = 0.0;
                pool->plot_crit_state[cur] = LDG_SCHED_PLOT_CRIT_SETTLED;
                sp--;
                continue;
            }

            if (pool->plot_crit_cursor[cur] >= job->dep_cunt)
            {
                pool->plot_crit[cur] += job->est_duration;
                pool->plot_crit_state[cur] = LDG_SCHED_PLOT_CRIT_SETTLED;
                sp--;
                continue;
            }

            dep_id = job->deps[pool->plot_crit_cursor[cur]];
            pool->plot_crit_cursor[cur]++;

            if (dep_id == LDG_SCHED_JOB_ID_INVALID || dep_id > LDG_SCHED_JOB_MAX) { continue; }

            dep_idx = dep_id - LDG_SCHED_JOB_ID_BASE;

            if (pool->plot_crit_state[dep_idx] == LDG_SCHED_PLOT_CRIT_SETTLED)
            {
                if (pool->plot_crit[dep_idx] > pool->plot_crit[cur]) { pool->plot_crit[cur] = pool->plot_crit[dep_idx]; }

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

static double sched_job_total_remaining_get(const ldg_sched_job_pool_t *pool)
{
    double total = 0.0;
    uint32_t idx = 0;

    for (idx = 0; idx < LDG_SCHED_JOB_MAX; idx++) { if (pool->jobs[idx].state == LDG_SCHED_JOB_STATE_READY || pool->jobs[idx].state == LDG_SCHED_JOB_STATE_PENDING) { total += pool->jobs[idx].est_duration; } }

    return total;
}

static double sched_job_plot_heur(uint64_t node, uint64_t goal, void *user_ctx)
{
    ldg_sched_job_pool_t *pool = 0x0;
    const ldg_sched_job_t *job = 0x0;
    double crit_path = 0.0;
    double total_remaining = 0.0;
    double deadline_dist = 0.0;
    double prio_term = 0.0;
    double res = 0.0;
    uint32_t cb_ret = 0;

    pool = (ldg_sched_job_pool_t *)user_ctx;

    if (node == goal || node >= LDG_SCHED_JOB_MAX) { return res; }

    job = &pool->jobs[node];

    if (pool->prio_cb)
    {
        cb_ret = pool->prio_cb(job, &res);
        if (cb_ret == LDG_ERR_AOK) { return res; }
    }

    crit_path = pool->plot_crit[node];
    total_remaining = pool->plot_total_remaining;

    deadline_dist = pool->frame_deadline;
    if (deadline_dist < 0.0) { deadline_dist = __builtin_fabs(deadline_dist) * 2.0; }

    prio_term = (double)(LDG_SCHED_PRIO_LEVELS - job->prio);

    res = pool->plot_weight_crit * crit_path
        + pool->plot_weight_remaining * total_remaining
        + pool->plot_weight_deadline * (1.0 / (1.0 + deadline_dist))
        - prio_term;

    return res;
}

static uint32_t sched_job_plot_succ(uint64_t node, uint64_t *nbr, double *cost, uint64_t max_nbr, uint64_t *nbr_cunt, void *user_ctx)
{
    ldg_sched_job_pool_t *pool = 0x0;
    ldg_sched_job_t *cand = 0x0;
    double edge_cost = 0.0;
    uint64_t out_cunt = 0;
    ldg_sched_job_id_t cur_id = 0;
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
                cost[out_cunt] = 0.0;
                out_cunt++;
            }
        }

        *nbr_cunt = out_cunt;

        return LDG_ERR_AOK;
    }

    if (node > LDG_SCHED_JOB_MAX) { return LDG_ERR_AOK; }

    cur_id = (ldg_sched_job_id_t)(node + LDG_SCHED_JOB_ID_BASE);
    edge_cost = pool->jobs[node].est_duration;

    for (idx = 0; idx < LDG_SCHED_JOB_MAX; idx++)
    {
        cand = &pool->jobs[idx];

        if (cand->state != LDG_SCHED_JOB_STATE_PENDING) { continue; }

        depends = LDG_TRUTH_FALSE;
        for (dep_idx = 0; dep_idx < cand->dep_cunt; dep_idx++) { if (cand->deps[dep_idx] == cur_id)
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

            if (dep_id == LDG_SCHED_JOB_ID_INVALID || dep_id > LDG_SCHED_JOB_MAX || dep_id == cur_id) { continue; }

            if (pool->jobs[dep_id - LDG_SCHED_JOB_ID_BASE].state == LDG_SCHED_JOB_STATE_PENDING)
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
    double total_cost = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE || !astar || !out_order || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

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

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE || !astar || !out_order || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

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

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

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
