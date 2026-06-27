#ifndef LDG_ECS_CMD_H
#define LDG_ECS_CMD_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/ecs/entity.h>
#include <dangling/thread/sync.h>

#define LDG_ECS_CMD_CAP_DEFAULT 256
#define LDG_ECS_CMD_PAYLOAD_CAP_DEFAULT (128 * LDG_KIB)

typedef enum ldg_ecs_cmd_op
{
    LDG_ECS_CMD_OP_ENTITY_CREATE = 0,
    LDG_ECS_CMD_OP_ENTITY_DESTROY,
    LDG_ECS_CMD_OP_COMP_ADD,
    LDG_ECS_CMD_OP_COMP_REMOVE,
    LDG_ECS_CMD_OP_MAX
} ldg_ecs_cmd_op_t;

typedef struct ldg_ecs_cmd
{
    uint64_t entity;
    void *payload;
    uint32_t comp_id;
    uint32_t op;
    uint8_t pudding[8];
} ldg_ecs_cmd_t;

typedef struct ldg_ecs_cmd_buff
{
    ldg_ecs_cmd_t *cmd;
    ldg_mem_pool_t *payload_pool;
    ldg_mut_t lock;
    uint32_t cap;
    uint32_t cmd_cunt;
    uint8_t is_init;
    uint8_t pudding[47];
} LDG_ALIGNED ldg_ecs_cmd_buff_t;

LDG_EXPORT uint32_t ldg_ecs_cmd_buff_init(ldg_ecs_cmd_buff_t *buff, uint32_t cmd_cap, uint64_t payload_cap);
LDG_EXPORT uint32_t ldg_ecs_cmd_buff_shutdown(ldg_ecs_cmd_buff_t *buff);
LDG_EXPORT uint32_t ldg_ecs_cmd_entity_create(ldg_ecs_cmd_buff_t *buff);
LDG_EXPORT uint32_t ldg_ecs_cmd_entity_destroy(ldg_ecs_cmd_buff_t *buff, uint64_t entity);
LDG_EXPORT uint32_t ldg_ecs_cmd_comp_add(ldg_ecs_cmd_buff_t *buff, uint64_t entity, uint32_t comp_id, const void *src, uint64_t src_size);
LDG_EXPORT uint32_t ldg_ecs_cmd_comp_remove(ldg_ecs_cmd_buff_t *buff, uint64_t entity, uint32_t comp_id);
LDG_EXPORT uint32_t ldg_ecs_cmd_buff_rst(ldg_ecs_cmd_buff_t *buff);
LDG_EXPORT uint64_t ldg_ecs_cmd_buff_cunt_get(ldg_ecs_cmd_buff_t *buff);

#endif
