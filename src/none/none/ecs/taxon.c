#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/ecs/taxon.h>

LDG_EXPORT uint32_t ldg_ecs_taxon_reg(ldg_ecs_taxon_t *taxon, uint32_t id, const ldg_ecs_taxon_desc_t *desc)
{
    uint32_t word = 0x0;

    if (LDG_UNLIKELY(!taxon || !desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(taxon->is_reg == LDG_TRUTH_TRUE)) { return LDG_ERR_EXISTS; }

    if (LDG_UNLIKELY(id == LDG_ECS_TAXON_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (word = 0; word < LDG_ECS_TAXON_COMP_MASK_WORDS; word++)
    {
        if (LDG_UNLIKELY((desc->required_mask[word] & desc->forbidden_mask[word]) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY((desc->optional_mask[word] & desc->forbidden_mask[word]) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }
    }
    for (word = 0; word < LDG_ECS_TAXON_COMP_MASK_WORDS; word++)
    {
        taxon->required_mask[word] = desc->required_mask[word];
        taxon->optional_mask[word] = desc->optional_mask[word];
        taxon->forbidden_mask[word] = desc->forbidden_mask[word];
        taxon->own_required_mask[word] = desc->required_mask[word];
        taxon->own_optional_mask[word] = desc->optional_mask[word];
        taxon->own_forbidden_mask[word] = desc->forbidden_mask[word];
    }
    for (word = 0; word < LDG_ECS_TAXON_LINK_WORDS; word++) { taxon->parent_mask[word] = 0; }
    taxon->id = id;
    taxon->is_reg = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_taxon_unreg(ldg_ecs_taxon_t *taxon)
{
    uint32_t word = 0x0;

    if (LDG_UNLIKELY(!taxon)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(taxon->is_reg != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    for (word = 0; word < LDG_ECS_TAXON_COMP_MASK_WORDS; word++)
    {
        taxon->required_mask[word] = 0;
        taxon->optional_mask[word] = 0;
        taxon->forbidden_mask[word] = 0;
        taxon->own_required_mask[word] = 0;
        taxon->own_optional_mask[word] = 0;
        taxon->own_forbidden_mask[word] = 0;
    }
    for (word = 0; word < LDG_ECS_TAXON_LINK_WORDS; word++) { taxon->parent_mask[word] = 0; }
    taxon->id = 0;
    taxon->is_reg = LDG_TRUTH_FALSE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint8_t ldg_ecs_taxon_comp_required_is(ldg_ecs_taxon_t *taxon, uint32_t comp_id)
{
    uint32_t word = 0x0;
    uint32_t bit = 0x0;

    if (LDG_UNLIKELY(!taxon)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(taxon->is_reg != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(comp_id >= (uint32_t)(LDG_ECS_TAXON_COMP_MASK_WORDS * 64))) { return LDG_ERR_FUNC_ARG_INVALID; }

    word = comp_id >> 6;
    bit = comp_id & 63;

    if ((taxon->required_mask[word] >> bit) & 1ULL) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

LDG_EXPORT uint8_t ldg_ecs_taxon_comp_forbidden_is(ldg_ecs_taxon_t *taxon, uint32_t comp_id)
{
    uint32_t word = 0x0;
    uint32_t bit = 0x0;

    if (LDG_UNLIKELY(!taxon)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(taxon->is_reg != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(comp_id >= (uint32_t)(LDG_ECS_TAXON_COMP_MASK_WORDS * 64))) { return LDG_ERR_FUNC_ARG_INVALID; }

    word = comp_id >> 6;
    bit = comp_id & 63;

    if ((taxon->forbidden_mask[word] >> bit) & 1ULL) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}
