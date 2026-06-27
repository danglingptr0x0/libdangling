#include <stdint.h>
#include <string.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/ecs/ecs.h>
#include <dangling/sched/sched.h>

typedef struct ldg_ecs_sys_job
{
    ldg_ecs_ctx_t *ecs;
    void *yield;
    uint32_t id;
    uint32_t sys_idx;
    uint32_t ret;
    uint8_t pudding[4];
} ldg_ecs_sys_job_t;

typedef struct ldg_ecs_sched
{
    ldg_sched_ctx_t sched_ctx;
    ldg_ecs_sys_job_t *job;
    ldg_sched_job_id_t *job_id;
    uint32_t job_cap;
    uint8_t pudding[4];
} ldg_ecs_sched_t;

static uint8_t ecs_sys_conflict_is(const ldg_ecs_sys_desc_t *later, const ldg_ecs_sys_desc_t *earlier)
{
    uint64_t conflict = 0x0;
    uint32_t word = 0x0;

    for (word = 0; word < LDG_ECS_TAXON_COMP_MASK_WORDS; word++)
    {
        conflict |= later->wr_mask[word] & (earlier->rd_mask[word] | earlier->wr_mask[word]);
        conflict |= earlier->wr_mask[word] & later->rd_mask[word];
    }

    return (uint8_t)(conflict != 0);
}

static ldg_sched_job_res_t ecs_sys_job_run(ldg_sched_job_id_t id, void *arg, void *yield_ctx)
{
    ldg_ecs_sys_job_t *sys_job = 0x0;

    if (LDG_UNLIKELY(!arg)) { return LDG_SCHED_JOB_RES_DONE; }

    sys_job = (ldg_ecs_sys_job_t *)arg;
    sys_job->id = id;
    sys_job->yield = yield_ctx;

    if (LDG_UNLIKELY(!sys_job->ecs->sys[sys_job->sys_idx].func))
    {
        sys_job->ret = LDG_ERR_AOK;
        return LDG_SCHED_JOB_RES_DONE;
    }

    sys_job->ret = sys_job->ecs->sys[sys_job->sys_idx].func(sys_job->ecs);

    return LDG_SCHED_JOB_RES_DONE;
}

uint32_t ldg_ecs_sched_init(ldg_ecs_ctx_t *ctx)
{
    ldg_sched_conf_t sched_conf = LDG_STRUCT_ZERO_INIT;
    ldg_ecs_sched_t *backend = 0x0;
    void *backend_mem = 0x0;
    void *job_mem = 0x0;
    void *job_id_mem = 0x0;
    uint32_t cap = 0;
    uint32_t ret = 0;

    ctx->sched = 0x0;

    cap = ctx->conf.sys_cap;
    if (cap == 0) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_ecs_sched_t), &backend_mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    backend = (ldg_ecs_sched_t *)backend_mem;
    if (LDG_UNLIKELY(memset(backend, 0, sizeof(ldg_ecs_sched_t)) != backend))
    {
        ldg_mem_dealloc(backend);
        return LDG_ERR_MEM_BAD;
    }

    ret = ldg_mem_alloc((uint64_t)cap * (uint64_t)sizeof(ldg_ecs_sys_job_t), &job_mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(backend);
        return ret;
    }

    backend->job = (ldg_ecs_sys_job_t *)job_mem;

    ret = ldg_mem_alloc((uint64_t)cap * (uint64_t)sizeof(ldg_sched_job_id_t), &job_id_mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(backend->job);
        ldg_mem_dealloc(backend);
        return ret;
    }

    backend->job_id = (ldg_sched_job_id_t *)job_id_mem;
    backend->job_cap = cap;

    ret = ldg_sched_conf_default(&sched_conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(backend->job_id);
        ldg_mem_dealloc(backend->job);
        ldg_mem_dealloc(backend);
        return ret;
    }

    sched_conf.worker_cunt = ctx->conf.worker_cunt;
    sched_conf.enable_tracing = 0;

    ret = ldg_sched_ctx_init(&backend->sched_ctx, &sched_conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(backend->job_id);
        ldg_mem_dealloc(backend->job);
        ldg_mem_dealloc(backend);
        return ret;
    }

    ctx->sched = backend;

    return LDG_ERR_AOK;
}

uint32_t ldg_ecs_sched_shutdown(ldg_ecs_ctx_t *ctx)
{
    ldg_ecs_sched_t *backend = 0x0;
    uint32_t ret = 0x0;

    if (!ctx->sched) { return LDG_ERR_AOK; }

    backend = (ldg_ecs_sched_t *)ctx->sched;

    ret = ldg_sched_ctx_shutdown(&backend->sched_ctx);

    if (backend->job_id) { ldg_mem_dealloc(backend->job_id); }

    if (backend->job) { ldg_mem_dealloc(backend->job); }

    ldg_mem_dealloc(backend);

    ctx->sched = 0x0;

    return ret;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_sys_all_sched_run(ldg_ecs_ctx_t *ctx)
{
    ldg_ecs_sched_t *backend = 0x0;
    ldg_sched_ctx_t *sched = 0x0;
    uint32_t first_err = 0x0;
    uint32_t sys_cunt = 0;
    uint32_t idx = 0;
    uint32_t prior_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init || !ctx->sched)) { return LDG_ERR_NOT_INIT; }

    backend = (ldg_ecs_sched_t *)ctx->sched;
    sched = &backend->sched_ctx;
    sys_cunt = ctx->sys_cunt;

    if (sys_cunt > 0)
    {
        if (LDG_UNLIKELY(sys_cunt > backend->job_cap)) { return LDG_ERR_BOUNDS; }

        for (idx = 0; idx < sys_cunt; idx++)
        {
            backend->job[idx].ecs = ctx;
            backend->job[idx].yield = 0x0;
            backend->job[idx].id = 0;
            backend->job[idx].sys_idx = idx;
            backend->job[idx].ret = LDG_ERR_AOK;

            ret = ldg_sched_ctx_job_create(sched, ecs_sys_job_run, &backend->job[idx], 0, 0, &backend->job_id[idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
            {
                for (prior_idx = 0; prior_idx < idx; prior_idx++) { ldg_sched_job_destroy(&sched->job_pool, backend->job_id[prior_idx]); }

                return ret;
            }
        }

        for (idx = 1; idx < sys_cunt; idx++) { for (prior_idx = 0; prior_idx < idx; prior_idx++) { if (ecs_sys_conflict_is(&ctx->sys[idx], &ctx->sys[prior_idx]))
                {
                    ret = ldg_sched_ctx_job_dep_add(sched, backend->job_id[idx], backend->job_id[prior_idx]);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK) && first_err == LDG_ERR_AOK) { first_err = ret; }
                }
            } }

        if (LDG_UNLIKELY(first_err != LDG_ERR_AOK))
        {
            for (idx = 0; idx < sys_cunt; idx++) { ldg_sched_job_destroy(&sched->job_pool, backend->job_id[idx]); }

            return first_err;
        }

        for (idx = 0; idx < sys_cunt; idx++)
        {
            ret = ldg_sched_ctx_job_submit(sched, backend->job_id[idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK) && first_err == LDG_ERR_AOK) { first_err = ret; }
        }

        ret = ldg_sched_ctx_drain(sched);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK) && first_err == LDG_ERR_AOK) { first_err = ret; }

        for (idx = 0; idx < sys_cunt; idx++)
        {
            if (backend->job[idx].ret != LDG_ERR_AOK && first_err == LDG_ERR_AOK) { first_err = backend->job[idx].ret; }

            ldg_sched_job_destroy(&sched->job_pool, backend->job_id[idx]);
        }
    }

    ret = ldg_ecs_ctx_worker_cmd_flush(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK) && first_err == LDG_ERR_AOK) { first_err = ret; }

    ret = ldg_ecs_ctx_cmd_flush(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK) && first_err == LDG_ERR_AOK) { first_err = ret; }

    return first_err;
}
