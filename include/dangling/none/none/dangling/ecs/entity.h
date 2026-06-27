#ifndef LDG_ECS_ENTITY_H
#define LDG_ECS_ENTITY_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_ECS_ENTITY_NULL 0ULL
#define LDG_ECS_ENTITY_IDX_RESERVED 0
#define LDG_ECS_ENTITY_CAP_DEFAULT 1024
#define LDG_ECS_DENSE_NULL 0xFFFFFFFFU

typedef struct ldg_ecs_entity_slot
{
    uint32_t dense_pos;
    uint32_t generation;
    uint32_t category;
    uint8_t alive;
    uint8_t pudding[3];
} ldg_ecs_entity_slot_t;

typedef struct ldg_ecs_entity_store
{
    ldg_ecs_entity_slot_t *slot;
    uint64_t *dense;
    uint32_t cap;
    uint32_t alive_cunt;
    uint32_t high_water;
    uint8_t is_init;
    uint8_t pudding[3];
} ldg_ecs_entity_store_t;

LDG_EXPORT uint32_t ldg_ecs_entity_store_init(ldg_ecs_entity_store_t *store, uint32_t cap);
LDG_EXPORT uint32_t ldg_ecs_entity_store_shutdown(ldg_ecs_entity_store_t *store);
LDG_EXPORT uint32_t ldg_ecs_entity_create(ldg_ecs_entity_store_t *store, uint64_t *out);
LDG_EXPORT uint32_t ldg_ecs_entity_destroy(ldg_ecs_entity_store_t *store, uint64_t entity);
LDG_EXPORT uint32_t ldg_ecs_entity_recycle_bulk(ldg_ecs_entity_store_t *store, uint32_t cunt, uint64_t *out, uint64_t max_cunt, uint64_t *out_cunt);
LDG_EXPORT uint8_t ldg_ecs_entity_alive_is(ldg_ecs_entity_store_t *store, uint64_t entity);
LDG_EXPORT uint64_t ldg_ecs_entity_alive_cunt_get(ldg_ecs_entity_store_t *store);
LDG_EXPORT uint64_t ldg_ecs_entity_idx_get(uint64_t entity);
LDG_EXPORT uint64_t ldg_ecs_entity_generation_get(uint64_t entity);
LDG_EXPORT uint32_t ldg_ecs_entity_category_set(ldg_ecs_entity_store_t *store, uint64_t entity, uint32_t category);
LDG_EXPORT uint64_t ldg_ecs_entity_category_get(ldg_ecs_entity_store_t *store, uint64_t entity);

#endif
