#include <stdint.h>
#include <string.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/ecs/comp.h>

LDG_EXPORT uint32_t ldg_ecs_comp_reg(ldg_ecs_comp_t *comp, uint32_t id, const ldg_ecs_comp_desc_t *desc, ldg_mem_pool_t *pool, uint32_t entity_cap)
{
    uint64_t align = 0x0;
    uint64_t stride = 0x0;
    uint64_t dense_size = 0x0;
    uint64_t sparse_size = 0x0;
    uint64_t back_size = 0x0;
    uint64_t slot = 0x0;
    uint32_t ret = 0x0;
    void *tmp = 0x0;

    if (LDG_UNLIKELY(!comp || !desc || !pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(comp->is_reg)) { return LDG_ERR_EXISTS; }

    if (LDG_UNLIKELY(id == LDG_ECS_COMP_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->item_size == 0 || desc->cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->item_size > UINT32_MAX)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(entity_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    align = desc->align;
    if (align == 0) { align = 1; }

    if (LDG_UNLIKELY((align & (align - 1)) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    stride = (desc->item_size + (align - 1)) & ~(align - 1);
    if (LDG_UNLIKELY((uint64_t)desc->cap > UINT64_MAX / stride)) { return LDG_ERR_OVERFLOW; }

    dense_size = stride * (uint64_t)desc->cap;
    sparse_size = (uint64_t)entity_cap * (uint64_t)sizeof(uint32_t);
    back_size = (uint64_t)desc->cap * (uint64_t)sizeof(uint64_t);

    ret = ldg_mem_pool_alloc(pool, dense_size, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    comp->dense = (uint8_t *)tmp;
    tmp = 0x0;

    ret = ldg_mem_pool_alloc(pool, sparse_size, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        comp->dense = 0x0;
        return ret;
    }

    comp->sparse = (uint32_t *)tmp;
    tmp = 0x0;

    ret = ldg_mem_pool_alloc(pool, back_size, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        comp->dense = 0x0;
        comp->sparse = 0x0;
        return ret;
    }

    comp->back = (uint64_t *)tmp;

    for (slot = 0; slot < entity_cap; slot++) { comp->sparse[slot] = LDG_ECS_DENSE_NULL; }

    comp->item_size = desc->item_size;
    comp->stride = stride;
    comp->align = align;
    comp->id = id;
    comp->dense_cunt = 0;
    comp->cap = desc->cap;
    comp->entity_cap = entity_cap;
    comp->is_reg = 1;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_comp_unreg(ldg_ecs_comp_t *comp)
{
    if (LDG_UNLIKELY(!comp)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!comp->is_reg)) { return LDG_ERR_NOT_INIT; }

    comp->dense = 0x0;
    comp->sparse = 0x0;
    comp->back = 0x0;
    comp->item_size = 0;
    comp->stride = 0;
    comp->align = 0;
    comp->id = 0;
    comp->dense_cunt = 0;
    comp->cap = 0;
    comp->entity_cap = 0;
    comp->is_reg = 0;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_comp_add(ldg_ecs_comp_t *comp, uint64_t entity, const void *src)
{
    uint32_t idx = 0x0;
    uint32_t pos = 0x0;

    if (LDG_UNLIKELY(!comp)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!comp->is_reg)) { return LDG_ERR_NOT_INIT; }

    idx = (uint32_t)(entity & 0xFFFFFFFFULL);
    if (LDG_UNLIKELY(idx == LDG_ECS_ENTITY_IDX_RESERVED || idx >= comp->entity_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(comp->sparse[idx] != LDG_ECS_DENSE_NULL)) { return LDG_ERR_EXISTS; }

    if (LDG_UNLIKELY(comp->dense_cunt >= comp->cap)) { return LDG_ERR_FULL; }

    pos = comp->dense_cunt;
    if (src != 0x0) { memcpy(comp->dense + (uint64_t)pos * comp->stride, src, (uint32_t)comp->item_size); }
    else{ memset(comp->dense + (uint64_t)pos * comp->stride, 0, (uint32_t)comp->item_size); }

    comp->back[pos] = entity;
    comp->sparse[idx] = pos;
    comp->dense_cunt++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_comp_remove(ldg_ecs_comp_t *comp, uint64_t entity)
{
    uint32_t idx = 0x0;
    uint32_t pos = 0x0;
    uint32_t last_pos = 0x0;
    uint64_t last_entity = 0x0;
    uint32_t last_idx = 0x0;

    if (LDG_UNLIKELY(!comp)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!comp->is_reg)) { return LDG_ERR_NOT_INIT; }

    idx = (uint32_t)(entity & 0xFFFFFFFFULL);
    if (LDG_UNLIKELY(idx == LDG_ECS_ENTITY_IDX_RESERVED || idx >= comp->entity_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(comp->sparse[idx] == LDG_ECS_DENSE_NULL)) { return LDG_ERR_ECS_COMP_ABSENT; }

    pos = comp->sparse[idx];
    last_pos = comp->dense_cunt - 1;
    if (pos != last_pos)
    {
        last_entity = comp->back[last_pos];
        last_idx = (uint32_t)(last_entity & 0xFFFFFFFFULL);
        memcpy(comp->dense + (uint64_t)pos * comp->stride, comp->dense + (uint64_t)last_pos * comp->stride, (uint32_t)comp->item_size);
        comp->back[pos] = last_entity;
        comp->sparse[last_idx] = pos;
    }

    comp->sparse[idx] = LDG_ECS_DENSE_NULL;
    comp->dense_cunt--;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_comp_get(ldg_ecs_comp_t *comp, uint64_t entity, void **out)
{
    uint32_t idx = 0x0;
    uint32_t pos = 0x0;

    if (LDG_UNLIKELY(!comp || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    if (LDG_UNLIKELY(!comp->is_reg)) { return LDG_ERR_NOT_INIT; }

    idx = (uint32_t)(entity & 0xFFFFFFFFULL);
    if (LDG_UNLIKELY(idx == LDG_ECS_ENTITY_IDX_RESERVED || idx >= comp->entity_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(comp->sparse[idx] == LDG_ECS_DENSE_NULL)) { return LDG_ERR_ECS_COMP_ABSENT; }

    pos = comp->sparse[idx];
    *out = comp->dense + (uint64_t)pos * comp->stride;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint8_t ldg_ecs_comp_has(ldg_ecs_comp_t *comp, uint64_t entity)
{
    uint32_t idx = 0x0;

    if (LDG_UNLIKELY(!comp)) { return 0; }

    if (LDG_UNLIKELY(!comp->is_reg)) { return 0; }

    idx = (uint32_t)(entity & 0xFFFFFFFFULL);
    if (LDG_UNLIKELY(idx == LDG_ECS_ENTITY_IDX_RESERVED || idx >= comp->entity_cap)) { return 0; }

    return (uint8_t)(comp->sparse[idx] != LDG_ECS_DENSE_NULL);
}

LDG_EXPORT uint64_t ldg_ecs_comp_cunt_get(ldg_ecs_comp_t *comp)
{
    if (LDG_UNLIKELY(!comp)) { return UINT64_MAX; }

    return (uint64_t)comp->dense_cunt;
}

LDG_EXPORT uint32_t ldg_ecs_comp_back_get(ldg_ecs_comp_t *comp, uint32_t dense_pos, uint64_t *out)
{
    if (LDG_UNLIKELY(!comp || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_ECS_ENTITY_NULL;
    if (LDG_UNLIKELY(!comp->is_reg)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(dense_pos >= comp->dense_cunt)) { return LDG_ERR_BOUNDS; }

    *out = comp->back[dense_pos];
    return LDG_ERR_AOK;
}
