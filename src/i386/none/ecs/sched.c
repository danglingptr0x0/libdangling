#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/ecs/ecs.h>

uint32_t ldg_ecs_sched_init(ldg_ecs_ctx_t *ctx)
{
    ctx->sched = 0x0;

    return LDG_ERR_AOK;
}

uint32_t ldg_ecs_sched_shutdown(ldg_ecs_ctx_t *ctx)
{
    ctx->sched = 0x0;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_sys_all_sched_run(ldg_ecs_ctx_t *ctx)
{
    uint32_t first_err = 0x0;
    uint32_t sys_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_NOT_INIT; }

    sys_cunt = ctx->sys_cunt;

    for (idx = 0; idx < sys_cunt; idx++)
    {
        if (ctx->sys[idx].func == 0x0) { continue; }

        ret = ctx->sys[idx].func(ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK) && first_err == LDG_ERR_AOK) { first_err = ret; }
    }

    ret = ldg_ecs_ctx_worker_cmd_flush(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK) && first_err == LDG_ERR_AOK) { first_err = ret; }

    ret = ldg_ecs_ctx_cmd_flush(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK) && first_err == LDG_ERR_AOK) { first_err = ret; }

    return first_err;
}
