#ifndef LDG_ECS_ECS_H
#define LDG_ECS_ECS_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/ecs/entity.h>
#include <dangling/ecs/comp.h>
#include <dangling/ecs/taxon.h>
#include <dangling/ecs/cmd.h>
#include <dangling/ecs/query.h>

#define LDG_ECS_LOGIC_COMP_ID 1
#define LDG_ECS_SYS_CAP_DEFAULT 32
#define LDG_ECS_RULE_CAP_DEFAULT 32
#define LDG_ECS_WORKER_CUNT_DEFAULT 1
#define LDG_ECS_COMP_ARENA_CAP_DEFAULT (256 * LDG_KIB)

typedef uint32_t (*ldg_ecs_logic_cb_t)(void *world, uint64_t entity);
typedef uint32_t (*ldg_ecs_sys_cb_t)(void *world);
typedef uint32_t (*ldg_ecs_rule_cb_t)(void *world, uint64_t entity, uint8_t *out);

typedef struct ldg_ecs_sys_desc
{
    ldg_ecs_sys_cb_t func;
    uint64_t rd_mask[LDG_ECS_TAXON_COMP_MASK_WORDS];
    uint64_t wr_mask[LDG_ECS_TAXON_COMP_MASK_WORDS];
    uint32_t id;
    uint8_t pudding[4];
} ldg_ecs_sys_desc_t;

typedef struct ldg_ecs_conf
{
    uint64_t comp_arena_cap;
    uint64_t cmd_payload_cap;
    uint32_t entity_cap;
    uint32_t comp_cap;
    uint32_t taxon_cap;
    uint32_t sys_cap;
    uint32_t rule_cap;
    uint32_t cmd_cap;
    uint32_t worker_cunt;
    uint8_t pudding[4];
} ldg_ecs_conf_t;

typedef struct ldg_ecs_ctx
{
    ldg_ecs_cmd_buff_t cmd_buff;
    ldg_ecs_entity_store_t store;
    ldg_ecs_comp_t *comp;
    ldg_ecs_taxon_t *taxon;
    ldg_ecs_sys_desc_t *sys;
    ldg_ecs_rule_cb_t *glob_rule;
    ldg_ecs_cmd_buff_t *worker_cmd_buff;
    void *sched;
    ldg_mem_pool_t *comp_arena;
    uint32_t *entity_taxon;
    ldg_ecs_conf_t conf;
    uint32_t comp_cunt;
    uint32_t taxon_cunt;
    uint32_t sys_cunt;
    uint32_t glob_rule_cunt;
    uint8_t is_init;
    uint8_t pudding[31];
} LDG_ALIGNED ldg_ecs_ctx_t;

LDG_EXPORT uint32_t ldg_ecs_conf_default(ldg_ecs_conf_t *conf);
LDG_EXPORT uint32_t ldg_ecs_ctx_init(ldg_ecs_ctx_t *ctx, const ldg_ecs_conf_t *conf);
LDG_EXPORT uint32_t ldg_ecs_ctx_shutdown(ldg_ecs_ctx_t *ctx);

LDG_EXPORT uint32_t ldg_ecs_ctx_comp_reg(ldg_ecs_ctx_t *ctx, uint32_t comp_id, const ldg_ecs_comp_desc_t *desc);
LDG_EXPORT uint32_t ldg_ecs_ctx_taxon_reg(ldg_ecs_ctx_t *ctx, uint32_t taxon_id, const ldg_ecs_taxon_desc_t *desc);
LDG_EXPORT uint32_t ldg_ecs_ctx_inst_reg(ldg_ecs_ctx_t *ctx, uint32_t taxon_id, ldg_ecs_logic_cb_t logic, uint64_t *out);
LDG_EXPORT uint32_t ldg_ecs_ctx_taxon_inst_reg(ldg_ecs_ctx_t *ctx, uint32_t taxon_id, const ldg_ecs_taxon_desc_t *desc, ldg_ecs_logic_cb_t logic, uint64_t *out);

LDG_EXPORT uint32_t ldg_ecs_ctx_comp_add(ldg_ecs_ctx_t *ctx, uint64_t entity, uint32_t comp_id, const void *src);
LDG_EXPORT uint32_t ldg_ecs_ctx_comp_remove(ldg_ecs_ctx_t *ctx, uint64_t entity, uint32_t comp_id);
LDG_EXPORT uint32_t ldg_ecs_ctx_comp_get(ldg_ecs_ctx_t *ctx, uint64_t entity, uint32_t comp_id, void **out);
LDG_EXPORT uint8_t ldg_ecs_ctx_comp_has(ldg_ecs_ctx_t *ctx, uint64_t entity, uint32_t comp_id);

LDG_EXPORT uint32_t ldg_ecs_ctx_glob_rule_add(ldg_ecs_ctx_t *ctx, ldg_ecs_rule_cb_t rule);
LDG_EXPORT uint32_t ldg_ecs_ctx_entity_validate(ldg_ecs_ctx_t *ctx, uint64_t entity, uint32_t taxon_id, uint8_t *out);
LDG_EXPORT uint32_t ldg_ecs_ctx_entity_category_set(ldg_ecs_ctx_t *ctx, uint64_t entity, uint32_t category);
LDG_EXPORT uint64_t ldg_ecs_ctx_entity_category_get(ldg_ecs_ctx_t *ctx, uint64_t entity);
LDG_EXPORT uint32_t ldg_ecs_ctx_entity_destroy(ldg_ecs_ctx_t *ctx, uint64_t entity);

LDG_EXPORT uint32_t ldg_ecs_ctx_sys_reg(ldg_ecs_ctx_t *ctx, const ldg_ecs_sys_desc_t *desc);
LDG_EXPORT uint32_t ldg_ecs_ctx_tick(ldg_ecs_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_ecs_ctx_sys_all_seq_run(ldg_ecs_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_ecs_ctx_sys_all_sched_run(ldg_ecs_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_ecs_sched_init(ldg_ecs_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_ecs_sched_shutdown(ldg_ecs_ctx_t *ctx);

LDG_EXPORT uint32_t ldg_ecs_ctx_cmd_flush(ldg_ecs_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_ecs_ctx_worker_cmd_flush(ldg_ecs_ctx_t *ctx);

LDG_EXPORT uint32_t ldg_ecs_ctx_query_begin(ldg_ecs_ctx_t *ctx, const ldg_ecs_query_t *query, ldg_ecs_query_iter_t *out);
LDG_EXPORT uint8_t ldg_ecs_ctx_query_next(ldg_ecs_ctx_t *ctx, const ldg_ecs_query_t *query, ldg_ecs_query_iter_t *iter);

#endif
