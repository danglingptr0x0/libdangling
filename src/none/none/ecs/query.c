#include <stdint.h>
#include <string.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/ecs/ecs.h>

LDG_EXPORT uint32_t ldg_ecs_ctx_query_begin(ldg_ecs_ctx_t *ctx, const ldg_ecs_query_t *query, ldg_ecs_query_iter_t *out)
{
    uint64_t best_cunt = 0x0;
    uint64_t comp_cunt = 0x0;
    uint32_t pivot = 0x0;
    uint32_t comp_id = 0x0;
    uint32_t query_idx = 0x0;

    if (LDG_UNLIKELY(!ctx || !query || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    memset(out, 0, sizeof(*out));
    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(query->comp_cunt == 0 || query->comp_cunt > LDG_ECS_QUERY_COMP_MAX)) { return LDG_ERR_ECS_QUERY_INVALID; }

    best_cunt = UINT64_MAX;
    pivot = LDG_ECS_COMP_ID_INVALID;
    for (query_idx = 0; query_idx < query->comp_cunt; query_idx++)
    {
        comp_id = query->comp_id[query_idx];
        if (LDG_UNLIKELY(comp_id == LDG_ECS_COMP_ID_INVALID || comp_id >= ctx->conf.comp_cap)) { return LDG_ERR_ECS_QUERY_INVALID; }

        if (LDG_UNLIKELY(!ctx->comp[comp_id].is_reg)) { return LDG_ERR_NOT_FOUND; }

        comp_cunt = ldg_ecs_comp_cunt_get(&ctx->comp[comp_id]);
        if (comp_cunt < best_cunt)
        {
            best_cunt = comp_cunt;
            pivot = comp_id;
        }
    }
    out->pivot_comp_id = pivot;
    out->pivot_cunt = (uint32_t)best_cunt;
    out->pivot_pos = 0;
    out->entity = LDG_ECS_ENTITY_NULL;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint8_t ldg_ecs_ctx_query_next(ldg_ecs_ctx_t *ctx, const ldg_ecs_query_t *query, ldg_ecs_query_iter_t *iter)
{
    uint64_t entity = 0x0;
    uint32_t comp_id = 0x0;
    uint32_t query_idx = 0x0;
    uint32_t ret = 0x0;
    uint8_t match = 0x0;

    if (LDG_UNLIKELY(!ctx || !query || !iter)) { return 0; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return 0; }

    if (LDG_UNLIKELY(iter->pivot_comp_id == LDG_ECS_COMP_ID_INVALID || iter->pivot_comp_id >= ctx->conf.comp_cap)) { return 0; }

    while (iter->pivot_pos < iter->pivot_cunt)
    {
        ret = ldg_ecs_comp_back_get(&ctx->comp[iter->pivot_comp_id], iter->pivot_pos, &entity);
        iter->pivot_pos++;
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { continue; }

        if (LDG_UNLIKELY(!ldg_ecs_entity_alive_is(&ctx->store, entity))) { continue; }

        match = 1;
        for (query_idx = 0; query_idx < query->comp_cunt; query_idx++)
        {
            comp_id = query->comp_id[query_idx];
            if (comp_id == iter->pivot_comp_id) { continue; }

            if (comp_id >= ctx->conf.comp_cap || !ldg_ecs_comp_has(&ctx->comp[comp_id], entity))
            {
                match = 0;
                break;
            }
        }
        if (match)
        {
            iter->entity = entity;
            return 1;
        }
    }
    iter->entity = LDG_ECS_ENTITY_NULL;
    return 0;
}
