#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/alloc.h>
#include <dangling/ecs/ecs.h>

static uint32_t ctx_entity_cascade_destroy(ldg_ecs_ctx_t *ctx, uint64_t entity)
{
    uint32_t comp_id = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(&ctx->store, entity) != LDG_TRUTH_TRUE)) { return LDG_ERR_ECS_ENTITY_DEAD; }

    for (comp_id = 1; comp_id < ctx->conf.comp_cap; comp_id++)
    {
        if (ctx->comp[comp_id].is_reg != LDG_TRUTH_TRUE) { continue; }

        if (ldg_ecs_comp_has(&ctx->comp[comp_id], entity) != LDG_TRUTH_TRUE) { continue; }

        if (LDG_UNLIKELY(ldg_ecs_comp_remove(&ctx->comp[comp_id], entity) != LDG_ERR_AOK && ret == LDG_ERR_AOK)) { ret = LDG_ERR_ECS_COMP_ABSENT; }
    }

    if (LDG_LIKELY(ldg_ecs_entity_destroy(&ctx->store, entity) == LDG_ERR_AOK)) { ctx->entity_taxon[(uint32_t)(entity & 0xFFFFFFFFULL)] = LDG_ECS_TAXON_ID_INVALID; }
    else if (ret == LDG_ERR_AOK) { ret = LDG_ERR_ECS_ENTITY_DEAD; }

    return ret;
}

static uint32_t cmd_buff_apply(ldg_ecs_ctx_t *ctx, ldg_ecs_cmd_buff_t *buff)
{
    ldg_ecs_cmd_t *rec = 0x0;
    uint64_t created = 0x0;
    uint32_t cunt = 0x0;
    uint32_t cmd_idx = 0x0;
    uint32_t step = 0x0;
    uint32_t ret = 0x0;

    cunt = buff->cmd_cunt;
    for (cmd_idx = 0; cmd_idx < cunt; cmd_idx++)
    {
        rec = &buff->cmd[cmd_idx];
        step = LDG_ERR_AOK;

        if (rec->op == LDG_ECS_CMD_OP_ENTITY_CREATE) { step = ldg_ecs_entity_create(&ctx->store, &created); }
        else if (rec->op == LDG_ECS_CMD_OP_ENTITY_DESTROY) { step = ctx_entity_cascade_destroy(ctx, rec->entity); }
        else if (rec->op == LDG_ECS_CMD_OP_COMP_ADD || rec->op == LDG_ECS_CMD_OP_COMP_REMOVE) { if (rec->comp_id >= ctx->conf.comp_cap || ctx->comp[rec->comp_id].is_reg != LDG_TRUTH_TRUE) { step = LDG_ERR_ECS_COMP_ABSENT; }
            else if (ldg_ecs_entity_alive_is(&ctx->store, rec->entity) != LDG_TRUTH_TRUE) { step = LDG_ERR_ECS_ENTITY_DEAD; }
            else if (rec->op == LDG_ECS_CMD_OP_COMP_ADD) { step = ldg_ecs_comp_add(&ctx->comp[rec->comp_id], rec->entity, rec->payload); }
            else { step = ldg_ecs_comp_remove(&ctx->comp[rec->comp_id], rec->entity); } }
        else { step = LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(step != LDG_ERR_AOK && ret == LDG_ERR_AOK)) { ret = step; }
    }

    step = ldg_ecs_cmd_buff_rst(buff);
    if (LDG_UNLIKELY(step != LDG_ERR_AOK && ret == LDG_ERR_AOK)) { ret = step; }

    return ret;
}

static void ctx_teardown(ldg_ecs_ctx_t *ctx)
{
    uint32_t worker = 0x0;
    uint32_t comp_idx = 0x0;

    ldg_ecs_sched_shutdown(ctx);

    if (ctx->worker_cmd_buff) { for (worker = 0; worker < ctx->conf.worker_cunt; worker++) { if (ctx->worker_cmd_buff[worker].is_init == LDG_TRUTH_TRUE) { ldg_ecs_cmd_buff_shutdown(&ctx->worker_cmd_buff[worker]); } } }

    if (ctx->cmd_buff.is_init == LDG_TRUTH_TRUE) { ldg_ecs_cmd_buff_shutdown(&ctx->cmd_buff); }

    if (ctx->comp) { for (comp_idx = 0; comp_idx < ctx->conf.comp_cap; comp_idx++) { if (ctx->comp[comp_idx].is_reg == LDG_TRUTH_TRUE) { ldg_ecs_comp_unreg(&ctx->comp[comp_idx]); } } }

    if (ctx->comp_arena) { ldg_mem_pool_destroy(&ctx->comp_arena); }

    if (ctx->store.is_init == LDG_TRUTH_TRUE) { ldg_ecs_entity_store_shutdown(&ctx->store); }

    if (ctx->worker_cmd_buff)
    {
        ldg_mem_dealloc(ctx->worker_cmd_buff);
        ctx->worker_cmd_buff = 0x0;
    }

    if (ctx->glob_rule)
    {
        ldg_mem_dealloc(ctx->glob_rule);
        ctx->glob_rule = 0x0;
    }

    if (ctx->entity_taxon)
    {
        ldg_mem_dealloc(ctx->entity_taxon);
        ctx->entity_taxon = 0x0;
    }

    if (ctx->sys)
    {
        ldg_mem_dealloc(ctx->sys);
        ctx->sys = 0x0;
    }

    if (ctx->taxon)
    {
        ldg_mem_dealloc(ctx->taxon);
        ctx->taxon = 0x0;
    }

    if (ctx->comp)
    {
        ldg_mem_dealloc(ctx->comp);
        ctx->comp = 0x0;
    }
}

static void inst_rollback(ldg_ecs_ctx_t *ctx, uint64_t entity, const ldg_ecs_taxon_t *taxon)
{
    uint32_t comp_id = 0x0;
    uint32_t word = 0x0;
    uint32_t bit = 0x0;

    if (ctx->comp[LDG_ECS_LOGIC_COMP_ID].is_reg == LDG_TRUTH_TRUE) { ldg_ecs_comp_remove(&ctx->comp[LDG_ECS_LOGIC_COMP_ID], entity); }

    for (comp_id = 1; comp_id < ctx->conf.comp_cap; comp_id++)
    {
        word = comp_id >> 6;
        bit = comp_id & 63;
        if (((taxon->required_mask[word] >> bit) & 1ULL) == 0) { continue; }

        if (ctx->comp[comp_id].is_reg == LDG_TRUTH_TRUE) { ldg_ecs_comp_remove(&ctx->comp[comp_id], entity); }
    }
    ldg_ecs_entity_destroy(&ctx->store, entity);
    ctx->entity_taxon[(uint32_t)(entity & 0xFFFFFFFFULL)] = LDG_ECS_TAXON_ID_INVALID;
}

LDG_EXPORT uint32_t ldg_ecs_conf_default_get(ldg_ecs_conf_t *conf)
{
    if (LDG_UNLIKELY(!conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    ldg_mem_zero(conf, sizeof(*conf));
    conf->comp_arena_cap = LDG_ECS_COMP_ARENA_CAP_DEFAULT;
    conf->cmd_payload_cap = LDG_ECS_CMD_PAYLOAD_CAP_DEFAULT;
    conf->entity_cap = LDG_ECS_ENTITY_CAP_DEFAULT;
    conf->comp_cap = LDG_ECS_COMP_CAP_DEFAULT;
    conf->taxon_cap = LDG_ECS_TAXON_CAP_DEFAULT;
    conf->sys_cap = LDG_ECS_SYS_CAP_DEFAULT;
    conf->rule_cap = LDG_ECS_RULE_CAP_DEFAULT;
    conf->cmd_cap = LDG_ECS_CMD_CAP_DEFAULT;
    conf->worker_cunt = LDG_ECS_WORKER_CUNT_DEFAULT;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_init(ldg_ecs_ctx_t *ctx, const ldg_ecs_conf_t *conf)
{
    ldg_ecs_comp_desc_t logic_desc = LDG_STRUCT_ZERO_INIT;
    void *comp_mem = 0x0;
    void *taxon_mem = 0x0;
    void *sys_mem = 0x0;
    void *rule_mem = 0x0;
    void *entity_taxon_mem = 0x0;
    void *worker_mem = 0x0;
    uint32_t worker = 0x0;
    uint32_t slot_idx = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!ctx || !conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->entity_cap == 0 || conf->comp_cap <= LDG_ECS_LOGIC_COMP_ID || conf->taxon_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->sys_cap == 0 || conf->rule_cap == 0 || conf->cmd_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->comp_arena_cap == 0 || conf->cmd_payload_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ldg_mem_zero(ctx, sizeof(*ctx));
    ctx->is_init = UINT8_MAX;
    ctx->conf = *conf;
    if (ctx->conf.comp_cap > LDG_ECS_COMP_MAX) { ctx->conf.comp_cap = LDG_ECS_COMP_MAX; }

    if (ctx->conf.taxon_cap > LDG_ECS_TAXON_MAX) { ctx->conf.taxon_cap = LDG_ECS_TAXON_MAX; }

    ret = ldg_mem_alloc((uint64_t)ctx->conf.comp_cap * sizeof(ldg_ecs_comp_t), &comp_mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ctx_teardown(ctx);
        return ret;
    }

    ldg_mem_zero(comp_mem, (uint64_t)ctx->conf.comp_cap * sizeof(ldg_ecs_comp_t));
    ctx->comp = (ldg_ecs_comp_t *)comp_mem;

    for (slot_idx = 0; slot_idx < ctx->conf.comp_cap; slot_idx++) { ctx->comp[slot_idx].is_reg = LDG_TRUTH_FALSE; }

    ret = ldg_mem_alloc((uint64_t)ctx->conf.taxon_cap * sizeof(ldg_ecs_taxon_t), &taxon_mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ctx_teardown(ctx);
        return ret;
    }

    ldg_mem_zero(taxon_mem, (uint64_t)ctx->conf.taxon_cap * sizeof(ldg_ecs_taxon_t));
    ctx->taxon = (ldg_ecs_taxon_t *)taxon_mem;

    for (slot_idx = 0; slot_idx < ctx->conf.taxon_cap; slot_idx++) { ctx->taxon[slot_idx].is_reg = LDG_TRUTH_FALSE; }

    ret = ldg_mem_alloc((uint64_t)ctx->conf.sys_cap * sizeof(ldg_ecs_sys_desc_t), &sys_mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ctx_teardown(ctx);
        return ret;
    }

    ldg_mem_zero(sys_mem, (uint64_t)ctx->conf.sys_cap * sizeof(ldg_ecs_sys_desc_t));
    ctx->sys = (ldg_ecs_sys_desc_t *)sys_mem;

    ret = ldg_mem_alloc((uint64_t)ctx->conf.rule_cap * sizeof(ldg_ecs_rule_cb_t), &rule_mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ctx_teardown(ctx);
        return ret;
    }

    ldg_mem_zero(rule_mem, (uint64_t)ctx->conf.rule_cap * sizeof(ldg_ecs_rule_cb_t));
    ctx->glob_rule = (ldg_ecs_rule_cb_t *)rule_mem;

    ret = ldg_mem_alloc((uint64_t)ctx->conf.entity_cap * sizeof(uint32_t), &entity_taxon_mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ctx_teardown(ctx);
        return ret;
    }

    ldg_mem_zero(entity_taxon_mem, (uint64_t)ctx->conf.entity_cap * sizeof(uint32_t));
    ctx->entity_taxon = (uint32_t *)entity_taxon_mem;

    if (ctx->conf.worker_cunt > 0)
    {
        ret = ldg_mem_alloc((uint64_t)ctx->conf.worker_cunt * sizeof(ldg_ecs_cmd_buff_t), &worker_mem);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ctx_teardown(ctx);
            return ret;
        }

        ldg_mem_zero(worker_mem, (uint64_t)ctx->conf.worker_cunt * sizeof(ldg_ecs_cmd_buff_t));
        ctx->worker_cmd_buff = (ldg_ecs_cmd_buff_t *)worker_mem;
    }

    ret = ldg_ecs_entity_store_init(&ctx->store, ctx->conf.entity_cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ctx_teardown(ctx);
        return ret;
    }

    ret = ldg_mem_pool_create(0, ctx->conf.comp_arena_cap, &ctx->comp_arena);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ctx_teardown(ctx);
        return ret;
    }

    ret = ldg_ecs_cmd_buff_init(&ctx->cmd_buff, ctx->conf.cmd_cap, ctx->conf.cmd_payload_cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ctx_teardown(ctx);
        return ret;
    }

    for (worker = 0; worker < ctx->conf.worker_cunt; worker++)
    {
        ret = ldg_ecs_cmd_buff_init(&ctx->worker_cmd_buff[worker], ctx->conf.cmd_cap, ctx->conf.cmd_payload_cap);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ctx_teardown(ctx);
            return ret;
        }
    }

    logic_desc.item_size = sizeof(ldg_ecs_logic_cb_t);
    logic_desc.align = sizeof(ldg_ecs_logic_cb_t);
    logic_desc.cap = ctx->conf.entity_cap;
    ret = ldg_ecs_comp_reg(&ctx->comp[LDG_ECS_LOGIC_COMP_ID], LDG_ECS_LOGIC_COMP_ID, &logic_desc, ctx->comp_arena, ctx->conf.entity_cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ctx_teardown(ctx);
        return ret;
    }

    ctx->comp_cunt = 1;

    ret = ldg_ecs_sched_init(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ctx_teardown(ctx);
        return ret;
    }

    ctx->is_init = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_shutdown(ldg_ecs_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ctx_teardown(ctx);
    ctx->comp_cunt = 0;
    ctx->taxon_cunt = 0;
    ctx->sys_cunt = 0;
    ctx->glob_rule_cunt = 0;
    ctx->is_init = LDG_TRUTH_FALSE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_comp_reg(ldg_ecs_ctx_t *ctx, uint32_t comp_id, const ldg_ecs_comp_desc_t *desc)
{
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!ctx || !desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_ECS_LOCKED; }

    if (LDG_UNLIKELY(comp_id == LDG_ECS_COMP_ID_INVALID || comp_id >= ctx->conf.comp_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_ecs_comp_reg(&ctx->comp[comp_id], comp_id, desc, ctx->comp_arena, ctx->conf.entity_cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->comp_cunt++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_taxon_reg(ldg_ecs_ctx_t *ctx, uint32_t taxon_id, const ldg_ecs_taxon_desc_t *desc)
{
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!ctx || !desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_ECS_LOCKED; }

    if (LDG_UNLIKELY(taxon_id == LDG_ECS_TAXON_ID_INVALID || taxon_id >= ctx->conf.taxon_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_ecs_taxon_reg(&ctx->taxon[taxon_id], taxon_id, desc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->taxon_cunt++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_inst_reg(ldg_ecs_ctx_t *ctx, uint32_t taxon_id, ldg_ecs_logic_cb_t logic, uint64_t *out)
{
    ldg_ecs_taxon_t *taxon = 0x0;
    uint64_t entity = 0x0;
    uint32_t comp_id = 0x0;
    uint32_t word = 0x0;
    uint32_t bit = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_ECS_ENTITY_NULL;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(taxon_id == LDG_ECS_TAXON_ID_INVALID || taxon_id >= ctx->conf.taxon_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    taxon = &ctx->taxon[taxon_id];
    if (LDG_UNLIKELY(taxon->is_reg != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    ret = ldg_ecs_entity_create(&ctx->store, &entity);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->entity_taxon[(uint32_t)(entity & 0xFFFFFFFFULL)] = taxon_id;

    for (comp_id = 1; comp_id < ctx->conf.comp_cap; comp_id++)
    {
        word = comp_id >> 6;
        bit = comp_id & 63;
        if (((taxon->required_mask[word] >> bit) & 1ULL) == 0) { continue; }

        if (comp_id == LDG_ECS_LOGIC_COMP_ID) { continue; }

        if (LDG_UNLIKELY(ctx->comp[comp_id].is_reg != LDG_TRUTH_TRUE))
        {
            inst_rollback(ctx, entity, taxon);
            return LDG_ERR_ECS_TAXON_UNSATISFIED;
        }

        ret = ldg_ecs_comp_add(&ctx->comp[comp_id], entity, 0x0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            inst_rollback(ctx, entity, taxon);
            return ret;
        }
    }

    if (logic != 0x0)
    {
        ret = ldg_ecs_comp_add(&ctx->comp[LDG_ECS_LOGIC_COMP_ID], entity, &logic);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            inst_rollback(ctx, entity, taxon);
            return ret;
        }
    }

    *out = entity;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_taxon_inst_reg(ldg_ecs_ctx_t *ctx, uint32_t taxon_id, const ldg_ecs_taxon_desc_t *desc, ldg_ecs_logic_cb_t logic, uint64_t *out)
{
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!ctx || !desc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_ECS_ENTITY_NULL;
    ret = ldg_ecs_ctx_taxon_reg(ctx, taxon_id, desc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_ecs_ctx_inst_reg(ctx, taxon_id, logic, out);
}

static void ctx_taxon_effective_rebuild(ldg_ecs_ctx_t *ctx)
{
    uint32_t taxon_id = 0x0;
    uint32_t parent_id = 0x0;
    uint32_t word = 0x0;
    uint32_t link_word = 0x0;
    uint32_t link_bit = 0x0;
    uint64_t merged = 0x0;
    uint8_t changed = UINT8_MAX;

    for (taxon_id = 1; taxon_id < ctx->conf.taxon_cap; taxon_id++)
    {
        if (ctx->taxon[taxon_id].is_reg != LDG_TRUTH_TRUE) { continue; }

        for (word = 0; word < LDG_ECS_TAXON_COMP_MASK_WORDS; word++)
        {
            ctx->taxon[taxon_id].required_mask[word] = ctx->taxon[taxon_id].own_required_mask[word];
            ctx->taxon[taxon_id].optional_mask[word] = ctx->taxon[taxon_id].own_optional_mask[word];
            ctx->taxon[taxon_id].forbidden_mask[word] = ctx->taxon[taxon_id].own_forbidden_mask[word];
        }
    }

    changed = LDG_TRUTH_TRUE;
    while (changed == LDG_TRUTH_TRUE)
    {
        changed = LDG_TRUTH_FALSE;
        for (taxon_id = 1; taxon_id < ctx->conf.taxon_cap; taxon_id++)
        {
            if (ctx->taxon[taxon_id].is_reg != LDG_TRUTH_TRUE) { continue; }

            for (parent_id = 1; parent_id < ctx->conf.taxon_cap; parent_id++)
            {
                link_word = parent_id >> 6;
                link_bit = parent_id & 63;
                if (((ctx->taxon[taxon_id].parent_mask[link_word] >> link_bit) & 1ULL) == 0) { continue; }

                if (ctx->taxon[parent_id].is_reg != LDG_TRUTH_TRUE) { continue; }

                for (word = 0; word < LDG_ECS_TAXON_COMP_MASK_WORDS; word++)
                {
                    merged = ctx->taxon[taxon_id].required_mask[word] | ctx->taxon[parent_id].required_mask[word];
                    if (merged != ctx->taxon[taxon_id].required_mask[word]) { ctx->taxon[taxon_id].required_mask[word] = merged; changed = LDG_TRUTH_TRUE; }

                    merged = ctx->taxon[taxon_id].forbidden_mask[word] | ctx->taxon[parent_id].forbidden_mask[word];
                    if (merged != ctx->taxon[taxon_id].forbidden_mask[word]) { ctx->taxon[taxon_id].forbidden_mask[word] = merged; changed = LDG_TRUTH_TRUE; }

                    merged = ctx->taxon[taxon_id].optional_mask[word] | ctx->taxon[parent_id].optional_mask[word];
                    if (merged != ctx->taxon[taxon_id].optional_mask[word]) { ctx->taxon[taxon_id].optional_mask[word] = merged; changed = LDG_TRUTH_TRUE; }
                }
            }
        }
    }

    for (taxon_id = 1; taxon_id < ctx->conf.taxon_cap; taxon_id++)
    {
        if (ctx->taxon[taxon_id].is_reg != LDG_TRUTH_TRUE) { continue; }

        for (word = 0; word < LDG_ECS_TAXON_COMP_MASK_WORDS; word++)
        {
            ctx->taxon[taxon_id].optional_mask[word] &= ~ctx->taxon[taxon_id].required_mask[word];
            ctx->taxon[taxon_id].optional_mask[word] &= ~ctx->taxon[taxon_id].forbidden_mask[word];
        }
    }
}

static uint8_t ctx_taxon_ancestor_is(ldg_ecs_ctx_t *ctx, uint32_t start, uint32_t target)
{
    uint64_t reach[LDG_ECS_TAXON_LINK_WORDS] = LDG_ARR_ZERO_INIT;
    uint32_t taxon_id = 0x0;
    uint32_t word = 0x0;
    uint32_t link_word = 0x0;
    uint32_t link_bit = 0x0;
    uint64_t merged = 0x0;
    uint8_t changed = UINT8_MAX;

    for (word = 0; word < LDG_ECS_TAXON_LINK_WORDS; word++) { reach[word] = ctx->taxon[start].parent_mask[word]; }

    changed = LDG_TRUTH_TRUE;
    while (changed == LDG_TRUTH_TRUE)
    {
        changed = LDG_TRUTH_FALSE;
        for (taxon_id = 1; taxon_id < ctx->conf.taxon_cap; taxon_id++)
        {
            link_word = taxon_id >> 6;
            link_bit = taxon_id & 63;
            if (((reach[link_word] >> link_bit) & 1ULL) == 0) { continue; }

            for (word = 0; word < LDG_ECS_TAXON_LINK_WORDS; word++)
            {
                merged = reach[word] | ctx->taxon[taxon_id].parent_mask[word];
                if (merged != reach[word]) { reach[word] = merged; changed = LDG_TRUTH_TRUE; }
            }
        }
    }

    link_word = target >> 6;
    link_bit = target & 63;

    if ((reach[link_word] >> link_bit) & 1ULL) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_subtaxon_add(ldg_ecs_ctx_t *ctx, uint32_t parent_id, uint32_t child_id)
{
    uint32_t taxon_id = 0x0;
    uint32_t word = 0x0;
    uint32_t link_word = 0x0;
    uint32_t link_bit = 0x0;
    uint8_t contradiction = UINT8_MAX;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_ECS_LOCKED; }

    if (LDG_UNLIKELY(parent_id == LDG_ECS_TAXON_ID_INVALID || parent_id >= ctx->conf.taxon_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(child_id == LDG_ECS_TAXON_ID_INVALID || child_id >= ctx->conf.taxon_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(parent_id == child_id)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->taxon[parent_id].is_reg != LDG_TRUTH_TRUE || ctx->taxon[child_id].is_reg != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(ctx_taxon_ancestor_is(ctx, parent_id, child_id) == LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    link_word = parent_id >> 6;
    link_bit = parent_id & 63;
    if (((ctx->taxon[child_id].parent_mask[link_word] >> link_bit) & 1ULL) != 0) { return LDG_ERR_AOK; }

    ctx->taxon[child_id].parent_mask[link_word] |= (1ULL << link_bit);
    ctx_taxon_effective_rebuild(ctx);

    contradiction = LDG_TRUTH_FALSE;
    for (taxon_id = 1; taxon_id < ctx->conf.taxon_cap; taxon_id++)
    {
        if (ctx->taxon[taxon_id].is_reg != LDG_TRUTH_TRUE) { continue; }

        for (word = 0; word < LDG_ECS_TAXON_COMP_MASK_WORDS; word++) { if ((ctx->taxon[taxon_id].required_mask[word] & ctx->taxon[taxon_id].forbidden_mask[word]) != 0) { contradiction = LDG_TRUTH_TRUE; } }
    }

    if (LDG_UNLIKELY(contradiction == LDG_TRUTH_TRUE))
    {
        ctx->taxon[child_id].parent_mask[link_word] &= ~(1ULL << link_bit);
        ctx_taxon_effective_rebuild(ctx);
        return LDG_ERR_ECS_TAXON_UNSATISFIED;
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_comp_add(ldg_ecs_ctx_t *ctx, uint64_t entity, uint32_t comp_id, const void *src)
{
    uint32_t idx = 0x0;
    uint32_t taxon_id = 0x0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(comp_id == LDG_ECS_COMP_ID_INVALID || comp_id >= ctx->conf.comp_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->comp[comp_id].is_reg != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(&ctx->store, entity) != LDG_TRUTH_TRUE)) { return LDG_ERR_ECS_ENTITY_DEAD; }

    idx = (uint32_t)(entity & 0xFFFFFFFFULL);
    taxon_id = ctx->entity_taxon[idx];
    if (taxon_id != LDG_ECS_TAXON_ID_INVALID && taxon_id < ctx->conf.taxon_cap) { if (LDG_UNLIKELY(ldg_ecs_taxon_comp_forbidden_is(&ctx->taxon[taxon_id], comp_id) == LDG_TRUTH_TRUE)) { return LDG_ERR_ECS_TAXON_UNSATISFIED; } }

    return ldg_ecs_comp_add(&ctx->comp[comp_id], entity, src);
}

LDG_EXPORT uint32_t ldg_ecs_ctx_comp_remove(ldg_ecs_ctx_t *ctx, uint64_t entity, uint32_t comp_id)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(comp_id == LDG_ECS_COMP_ID_INVALID || comp_id >= ctx->conf.comp_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->comp[comp_id].is_reg != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    return ldg_ecs_comp_remove(&ctx->comp[comp_id], entity);
}

LDG_EXPORT uint32_t ldg_ecs_ctx_comp_get(ldg_ecs_ctx_t *ctx, uint64_t entity, uint32_t comp_id, void **out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(comp_id == LDG_ECS_COMP_ID_INVALID || comp_id >= ctx->conf.comp_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->comp[comp_id].is_reg != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    return ldg_ecs_comp_get(&ctx->comp[comp_id], entity, out);
}

LDG_EXPORT uint8_t ldg_ecs_ctx_comp_has(ldg_ecs_ctx_t *ctx, uint64_t entity, uint32_t comp_id)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(comp_id == LDG_ECS_COMP_ID_INVALID || comp_id >= ctx->conf.comp_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->comp[comp_id].is_reg != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_ecs_comp_has(&ctx->comp[comp_id], entity);
}

LDG_EXPORT uint32_t ldg_ecs_ctx_glob_rule_add(ldg_ecs_ctx_t *ctx, ldg_ecs_rule_cb_t rule)
{
    if (LDG_UNLIKELY(!ctx || !rule)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_ECS_LOCKED; }

    if (LDG_UNLIKELY(ctx->glob_rule_cunt >= ctx->conf.rule_cap)) { return LDG_ERR_FULL; }

    ctx->glob_rule[ctx->glob_rule_cunt] = rule;
    ctx->glob_rule_cunt++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_entity_validate(ldg_ecs_ctx_t *ctx, uint64_t entity, uint32_t taxon_id, uint8_t *out)
{
    ldg_ecs_taxon_t *taxon = 0x0;
    uint64_t present[LDG_ECS_TAXON_COMP_MASK_WORDS] = LDG_ARR_ZERO_INIT;
    uint8_t verdict = UINT8_MAX;
    uint8_t rule_ok = UINT8_MAX;
    uint32_t comp_id = 0x0;
    uint32_t word = 0x0;
    uint32_t bit = 0x0;
    uint32_t rule_idx = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_TRUTH_FALSE;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(taxon_id == LDG_ECS_TAXON_ID_INVALID || taxon_id >= ctx->conf.taxon_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    taxon = &ctx->taxon[taxon_id];
    if (LDG_UNLIKELY(taxon->is_reg != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(&ctx->store, entity) != LDG_TRUTH_TRUE)) { return LDG_ERR_ECS_ENTITY_DEAD; }

    for (comp_id = 1; comp_id < ctx->conf.comp_cap; comp_id++)
    {
        if (ctx->comp[comp_id].is_reg != LDG_TRUTH_TRUE) { continue; }

        if (ldg_ecs_comp_has(&ctx->comp[comp_id], entity) == LDG_TRUTH_TRUE)
        {
            word = comp_id >> 6;
            bit = comp_id & 63;
            present[word] |= (1ULL << bit);
        }
    }

    verdict = LDG_TRUTH_TRUE;
    for (word = 0; word < LDG_ECS_TAXON_COMP_MASK_WORDS; word++)
    {
        if ((present[word] & taxon->required_mask[word]) != taxon->required_mask[word]) { verdict = LDG_TRUTH_FALSE; }

        if ((present[word] & taxon->forbidden_mask[word]) != 0) { verdict = LDG_TRUTH_FALSE; }
    }

    for (rule_idx = 0; rule_idx < ctx->glob_rule_cunt; rule_idx++)
    {
        rule_ok = LDG_TRUTH_FALSE;
        ret = ctx->glob_rule[rule_idx](ctx, entity, &rule_ok);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (rule_ok != LDG_TRUTH_TRUE) { verdict = LDG_TRUTH_FALSE; }
    }

    *out = verdict;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_entity_category_set(ldg_ecs_ctx_t *ctx, uint64_t entity, uint32_t category)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return ldg_ecs_entity_category_set(&ctx->store, entity, category);
}

LDG_EXPORT uint64_t ldg_ecs_ctx_entity_category_get(ldg_ecs_ctx_t *ctx, uint64_t entity)
{
    if (LDG_UNLIKELY(!ctx)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    return ldg_ecs_entity_category_get(&ctx->store, entity);
}

LDG_EXPORT uint32_t ldg_ecs_ctx_entity_destroy(ldg_ecs_ctx_t *ctx, uint64_t entity)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(&ctx->store, entity) != LDG_TRUTH_TRUE)) { return LDG_ERR_ECS_ENTITY_DEAD; }

    return ctx_entity_cascade_destroy(ctx, entity);
}

LDG_EXPORT uint32_t ldg_ecs_ctx_sys_reg(ldg_ecs_ctx_t *ctx, const ldg_ecs_sys_desc_t *desc)
{
    if (LDG_UNLIKELY(!ctx || !desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_ECS_LOCKED; }

    if (LDG_UNLIKELY(!desc->func)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->sys_cunt >= ctx->conf.sys_cap)) { return LDG_ERR_FULL; }

    ctx->sys[ctx->sys_cunt] = *desc;
    ctx->sys_cunt++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_tick(ldg_ecs_ctx_t *ctx)
{
    ldg_ecs_comp_t *logic = 0x0;
    ldg_ecs_logic_cb_t cb = 0x0;
    void *slot = 0x0;
    uint64_t entity = 0x0;
    uint32_t cunt = 0x0;
    uint32_t pos = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    logic = &ctx->comp[LDG_ECS_LOGIC_COMP_ID];
    cunt = logic->dense_cunt;
    for (pos = 0; pos < cunt; pos++)
    {
        if (LDG_UNLIKELY(ldg_ecs_comp_back_get(logic, pos, &entity) != LDG_ERR_AOK))
        {
            if (ret == LDG_ERR_AOK) { ret = LDG_ERR_ECS_COMP_ABSENT; }

            continue;
        }

        if (LDG_UNLIKELY(ldg_ecs_comp_get(logic, entity, &slot) != LDG_ERR_AOK))
        {
            if (ret == LDG_ERR_AOK) { ret = LDG_ERR_ECS_COMP_ABSENT; }

            continue;
        }

        ldg_mem_cpy(&cb, slot, sizeof(cb));
        if (cb == 0x0) { continue; }

        if (LDG_UNLIKELY(cb(ctx, entity) != LDG_ERR_AOK && ret == LDG_ERR_AOK)) { ret = LDG_ERR_INVALID; }
    }

    if (LDG_UNLIKELY(ldg_ecs_ctx_cmd_flush(ctx) != LDG_ERR_AOK && ret == LDG_ERR_AOK)) { ret = LDG_ERR_ECS_CMD_OVERFLOW; }

    return ret;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_sys_all_seq_run(ldg_ecs_ctx_t *ctx)
{
    uint32_t sys_idx = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    for (sys_idx = 0; sys_idx < ctx->sys_cunt; sys_idx++)
    {
        if (ctx->sys[sys_idx].func == 0x0) { continue; }

        if (LDG_UNLIKELY(ctx->sys[sys_idx].func(ctx) != LDG_ERR_AOK && ret == LDG_ERR_AOK)) { ret = LDG_ERR_INVALID; }
    }

    if (LDG_UNLIKELY(ldg_ecs_ctx_cmd_flush(ctx) != LDG_ERR_AOK && ret == LDG_ERR_AOK)) { ret = LDG_ERR_ECS_CMD_OVERFLOW; }

    return ret;
}

LDG_EXPORT uint32_t ldg_ecs_ctx_cmd_flush(ldg_ecs_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return cmd_buff_apply(ctx, &ctx->cmd_buff);
}

LDG_EXPORT uint32_t ldg_ecs_ctx_worker_cmd_flush(ldg_ecs_ctx_t *ctx)
{
    uint32_t worker = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    for (worker = 0; worker < ctx->conf.worker_cunt; worker++) { if (LDG_UNLIKELY(cmd_buff_apply(ctx, &ctx->worker_cmd_buff[worker]) != LDG_ERR_AOK && ret == LDG_ERR_AOK)) { ret = LDG_ERR_ECS_CMD_OVERFLOW; } }

    return ret;
}
