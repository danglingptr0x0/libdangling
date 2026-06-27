#ifndef LDG_ECS_COMP_H
#define LDG_ECS_COMP_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/ecs/entity.h>

#define LDG_ECS_COMP_MAX 256
#define LDG_ECS_COMP_ID_INVALID 0
#define LDG_ECS_COMP_CAP_DEFAULT 64

typedef struct ldg_ecs_comp_desc
{
    uint64_t item_size;
    uint64_t align;
    uint32_t cap;
    uint8_t pudding[4];
} ldg_ecs_comp_desc_t;

typedef struct ldg_ecs_comp
{
    uint8_t *dense;
    uint32_t *sparse;
    uint64_t *back;
    uint64_t item_size;
    uint64_t stride;
    uint64_t align;
    uint32_t id;
    uint32_t dense_cunt;
    uint32_t cap;
    uint32_t entity_cap;
    uint8_t is_reg;
    uint8_t pudding[7];
} ldg_ecs_comp_t;

LDG_EXPORT uint32_t ldg_ecs_comp_reg(ldg_ecs_comp_t *comp, uint32_t id, const ldg_ecs_comp_desc_t *desc, ldg_mem_pool_t *pool, uint32_t entity_cap);
LDG_EXPORT uint32_t ldg_ecs_comp_unreg(ldg_ecs_comp_t *comp);
LDG_EXPORT uint32_t ldg_ecs_comp_add(ldg_ecs_comp_t *comp, uint64_t entity, const void *src);
LDG_EXPORT uint32_t ldg_ecs_comp_remove(ldg_ecs_comp_t *comp, uint64_t entity);
LDG_EXPORT uint32_t ldg_ecs_comp_get(ldg_ecs_comp_t *comp, uint64_t entity, void **out);
LDG_EXPORT uint8_t ldg_ecs_comp_has(ldg_ecs_comp_t *comp, uint64_t entity);
LDG_EXPORT uint64_t ldg_ecs_comp_cunt_get(ldg_ecs_comp_t *comp);
LDG_EXPORT uint32_t ldg_ecs_comp_back_get(ldg_ecs_comp_t *comp, uint32_t dense_pos, uint64_t *out);

#endif
