#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/alloc.h>
#include <dangling/ecs/entity.h>

LDG_EXPORT uint64_t ldg_ecs_entity_idx_get(uint64_t entity)
{
    if (LDG_UNLIKELY(entity == LDG_ECS_ENTITY_NULL)) { return UINT64_MAX; }

    return (uint64_t)(uint32_t)(entity & 0xFFFFFFFFULL);
}

LDG_EXPORT uint64_t ldg_ecs_entity_generation_get(uint64_t entity)
{
    if (LDG_UNLIKELY(entity == LDG_ECS_ENTITY_NULL)) { return UINT64_MAX; }

    return (uint64_t)(uint32_t)(entity >> 32);
}

LDG_EXPORT uint32_t ldg_ecs_entity_store_init(ldg_ecs_entity_store_t *store, uint32_t cap)
{
    void *slot_mem = 0x0;
    void *dense_mem = 0x0;
    uint32_t idx = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!store)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ldg_mem_zero(store, sizeof(*store));
    store->is_init = UINT8_MAX;
    ret = ldg_mem_alloc((uint64_t)cap * sizeof(ldg_ecs_entity_slot_t), &slot_mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_alloc((uint64_t)cap * sizeof(uint64_t), &dense_mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(slot_mem);
        return ret;
    }

    ldg_mem_zero(slot_mem, (uint64_t)cap * sizeof(ldg_ecs_entity_slot_t));
    ldg_mem_zero(dense_mem, (uint64_t)cap * sizeof(uint64_t));
    store->slot = (ldg_ecs_entity_slot_t *)slot_mem;
    store->dense = (uint64_t *)dense_mem;

    for (idx = 0; idx < cap; idx++) { store->slot[idx].alive = LDG_TRUTH_FALSE; }

    store->cap = cap;
    store->high_water = LDG_ECS_ENTITY_IDX_RESERVED + 1;
    store->is_init = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_entity_store_shutdown(ldg_ecs_entity_store_t *store)
{
    if (LDG_UNLIKELY(!store)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (store->slot) { ldg_mem_dealloc(store->slot); }

    if (store->dense) { ldg_mem_dealloc(store->dense); }

    store->slot = 0x0;
    store->dense = 0x0;
    store->cap = 0;
    store->is_init = LDG_TRUTH_FALSE;
    store->alive_cunt = 0;
    store->high_water = 0;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint8_t ldg_ecs_entity_alive_is(ldg_ecs_entity_store_t *store, uint64_t entity)
{
    uint32_t idx = 0x0;
    uint32_t generation = 0x0;

    if (LDG_UNLIKELY(!store)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (entity == LDG_ECS_ENTITY_NULL) { return LDG_TRUTH_FALSE; }

    idx = (uint32_t)(entity & 0xFFFFFFFFULL);
    generation = (uint32_t)(entity >> 32);
    if (idx == LDG_ECS_ENTITY_IDX_RESERVED) { return LDG_TRUTH_FALSE; }

    if (idx >= store->high_water) { return LDG_TRUTH_FALSE; }

    if (store->slot[idx].alive == LDG_TRUTH_TRUE && store->slot[idx].generation == generation) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

LDG_EXPORT uint32_t ldg_ecs_entity_create(ldg_ecs_entity_store_t *store, uint64_t *out)
{
    uint32_t total_dense = 0x0;
    uint32_t pos = 0x0;
    uint32_t idx = 0x0;
    uint64_t handle = 0x0;

    if (LDG_UNLIKELY(!store || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_ECS_ENTITY_NULL;
    if (LDG_UNLIKELY(store->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    total_dense = store->high_water - 1;
    if (store->alive_cunt < total_dense)
    {
        pos = store->alive_cunt;
        idx = (uint32_t)(store->dense[pos] & 0xFFFFFFFFULL);
        store->slot[idx].alive = LDG_TRUTH_TRUE;
        store->slot[idx].dense_pos = pos;
        handle = ((uint64_t)store->slot[idx].generation << 32) | (uint64_t)idx;
        store->dense[pos] = handle;
        store->alive_cunt++;
        *out = handle;
        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY(store->high_water >= store->cap)) { return LDG_ERR_FULL; }

    idx = store->high_water;
    pos = store->alive_cunt;
    store->slot[idx].generation = 0;
    store->slot[idx].alive = LDG_TRUTH_TRUE;
    store->slot[idx].dense_pos = pos;
    store->slot[idx].category = 0;
    handle = (uint64_t)idx;
    store->dense[pos] = handle;
    store->high_water++;
    store->alive_cunt++;
    *out = handle;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_entity_destroy(ldg_ecs_entity_store_t *store, uint64_t entity)
{
    uint32_t idx = 0x0;
    uint32_t pos = 0x0;
    uint32_t last_pos = 0x0;
    uint64_t last_handle = 0x0;
    uint32_t last_idx = 0x0;

    if (LDG_UNLIKELY(!store)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(store, entity) != LDG_TRUTH_TRUE)) { return LDG_ERR_ECS_ENTITY_DEAD; }

    idx = (uint32_t)(entity & 0xFFFFFFFFULL);
    pos = store->slot[idx].dense_pos;
    last_pos = store->alive_cunt - 1;
    last_handle = store->dense[last_pos];
    last_idx = (uint32_t)(last_handle & 0xFFFFFFFFULL);
    store->dense[pos] = last_handle;
    store->slot[last_idx].dense_pos = pos;
    store->slot[idx].generation++;
    store->slot[idx].alive = LDG_TRUTH_FALSE;
    store->slot[idx].dense_pos = last_pos;
    store->dense[last_pos] = ((uint64_t)store->slot[idx].generation << 32) | (uint64_t)idx;
    store->alive_cunt--;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_entity_recycle_bulk(ldg_ecs_entity_store_t *store, uint32_t cunt, uint64_t *out, uint64_t max_cunt, uint64_t *out_cunt)
{
    uint32_t total_dense = 0x0;
    uint32_t dead_cunt = 0x0;
    uint32_t revive_cunt = 0x0;
    uint32_t revived = 0x0;
    uint32_t pos = 0x0;
    uint32_t idx = 0x0;
    uint64_t handle = 0x0;

    if (LDG_UNLIKELY(!store || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    if (LDG_UNLIKELY(store->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    total_dense = store->high_water - 1;
    dead_cunt = total_dense - store->alive_cunt;
    revive_cunt = cunt;
    if (revive_cunt > dead_cunt) { revive_cunt = dead_cunt; }

    if ((uint64_t)revive_cunt > max_cunt) { revive_cunt = (uint32_t)max_cunt; }

    for (revived = 0; revived < revive_cunt; revived++)
    {
        pos = store->alive_cunt;
        idx = (uint32_t)(store->dense[pos] & 0xFFFFFFFFULL);
        store->slot[idx].alive = LDG_TRUTH_TRUE;
        store->slot[idx].dense_pos = pos;
        handle = ((uint64_t)store->slot[idx].generation << 32) | (uint64_t)idx;
        store->dense[pos] = handle;
        out[revived] = handle;
        store->alive_cunt++;
    }
    *out_cunt = (uint64_t)revive_cunt;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_ecs_entity_alive_cunt_get(ldg_ecs_entity_store_t *store)
{
    if (LDG_UNLIKELY(!store)) { return UINT64_MAX; }

    return (uint64_t)store->alive_cunt;
}

LDG_EXPORT uint32_t ldg_ecs_entity_category_set(ldg_ecs_entity_store_t *store, uint64_t entity, uint32_t category)
{
    uint32_t idx = 0x0;

    if (LDG_UNLIKELY(!store)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(store, entity) != LDG_TRUTH_TRUE)) { return LDG_ERR_ECS_ENTITY_DEAD; }

    idx = (uint32_t)(entity & 0xFFFFFFFFULL);
    store->slot[idx].category = category;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_ecs_entity_category_get(ldg_ecs_entity_store_t *store, uint64_t entity)
{
    uint32_t idx = 0x0;

    if (LDG_UNLIKELY(!store)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(store, entity) != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    idx = (uint32_t)(entity & 0xFFFFFFFFULL);
    return (uint64_t)store->slot[idx].category;
}
