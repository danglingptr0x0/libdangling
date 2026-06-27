#ifndef LDG_ECS_TAXON_H
#define LDG_ECS_TAXON_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/ecs/entity.h>

#define LDG_ECS_TAXON_MAX 128
#define LDG_ECS_TAXON_CAP_DEFAULT 32
#define LDG_ECS_TAXON_ID_INVALID 0
#define LDG_ECS_TAXON_COMP_MASK_WORDS 4

typedef struct ldg_ecs_taxon_desc
{
    uint64_t required_mask[LDG_ECS_TAXON_COMP_MASK_WORDS];
    uint64_t optional_mask[LDG_ECS_TAXON_COMP_MASK_WORDS];
    uint64_t forbidden_mask[LDG_ECS_TAXON_COMP_MASK_WORDS];
} ldg_ecs_taxon_desc_t;

typedef struct ldg_ecs_taxon
{
    uint64_t required_mask[LDG_ECS_TAXON_COMP_MASK_WORDS];
    uint64_t optional_mask[LDG_ECS_TAXON_COMP_MASK_WORDS];
    uint64_t forbidden_mask[LDG_ECS_TAXON_COMP_MASK_WORDS];
    uint32_t id;
    uint8_t is_reg;
    uint8_t pudding[3];
} ldg_ecs_taxon_t;

LDG_EXPORT uint32_t ldg_ecs_taxon_reg(ldg_ecs_taxon_t *taxon, uint32_t id, const ldg_ecs_taxon_desc_t *desc);
LDG_EXPORT uint32_t ldg_ecs_taxon_unreg(ldg_ecs_taxon_t *taxon);
LDG_EXPORT uint8_t ldg_ecs_taxon_comp_required_is(ldg_ecs_taxon_t *taxon, uint32_t comp_id);
LDG_EXPORT uint8_t ldg_ecs_taxon_comp_forbidden_is(ldg_ecs_taxon_t *taxon, uint32_t comp_id);

#endif
