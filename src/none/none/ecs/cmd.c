#include <stdint.h>
#include <string.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/thread/sync.h>
#include <dangling/ecs/cmd.h>

LDG_EXPORT uint32_t ldg_ecs_cmd_buff_init(ldg_ecs_cmd_buff_t *buff, uint32_t cmd_cap, uint64_t payload_cap)
{
    void *cmd_mem = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cmd_cap == 0 || payload_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    memset(buff, 0, sizeof(*buff));
    ret = ldg_mem_alloc((uint64_t)cmd_cap * sizeof(ldg_ecs_cmd_t), &cmd_mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    memset(cmd_mem, 0, (uint64_t)cmd_cap * sizeof(ldg_ecs_cmd_t));
    ret = ldg_mem_pool_create(0, payload_cap, &buff->payload_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(cmd_mem);
        return ret;
    }

    ret = ldg_mut_init(&buff->lock, 0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_pool_destroy(&buff->payload_pool);
        ldg_mem_dealloc(cmd_mem);
        return ret;
    }

    buff->cmd = (ldg_ecs_cmd_t *)cmd_mem;
    buff->cap = cmd_cap;
    buff->cmd_cunt = 0;
    buff->is_init = 1;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_ecs_cmd_buff_shutdown(ldg_ecs_cmd_buff_t *buff)
{
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!buff->is_init)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mem_pool_destroy(&buff->payload_pool);
    if (buff->cmd) { ldg_mem_dealloc(buff->cmd); }

    ldg_mut_destroy(&buff->lock);

    buff->cmd = 0x0;
    buff->cap = 0;
    buff->cmd_cunt = 0;
    buff->is_init = 0;
    return ret;
}

LDG_EXPORT uint32_t ldg_ecs_cmd_entity_create(ldg_ecs_cmd_buff_t *buff)
{
    ldg_ecs_cmd_t *rec = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!buff->is_init)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mut_lock(&buff->lock);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (buff->cmd_cunt >= buff->cap)
    {
        ldg_mut_unlock(&buff->lock);
        return LDG_ERR_ECS_CMD_OVERFLOW;
    }

    rec = &buff->cmd[buff->cmd_cunt];
    rec->entity = LDG_ECS_ENTITY_NULL;
    rec->payload = 0x0;
    rec->comp_id = 0;
    rec->op = LDG_ECS_CMD_OP_ENTITY_CREATE;
    buff->cmd_cunt++;

    return ldg_mut_unlock(&buff->lock);
}

LDG_EXPORT uint32_t ldg_ecs_cmd_entity_destroy(ldg_ecs_cmd_buff_t *buff, uint64_t entity)
{
    ldg_ecs_cmd_t *rec = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!buff->is_init)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mut_lock(&buff->lock);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (buff->cmd_cunt >= buff->cap)
    {
        ldg_mut_unlock(&buff->lock);
        return LDG_ERR_ECS_CMD_OVERFLOW;
    }

    rec = &buff->cmd[buff->cmd_cunt];
    rec->entity = entity;
    rec->payload = 0x0;
    rec->comp_id = 0;
    rec->op = LDG_ECS_CMD_OP_ENTITY_DESTROY;
    buff->cmd_cunt++;

    return ldg_mut_unlock(&buff->lock);
}

LDG_EXPORT uint32_t ldg_ecs_cmd_comp_add(ldg_ecs_cmd_buff_t *buff, uint64_t entity, uint32_t comp_id, const void *src, uint64_t src_size)
{
    ldg_ecs_cmd_t *rec = 0x0;
    void *payload = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!buff || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!buff->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(src_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(src_size > UINT32_MAX)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mut_lock(&buff->lock);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (buff->cmd_cunt >= buff->cap)
    {
        ldg_mut_unlock(&buff->lock);
        return LDG_ERR_ECS_CMD_OVERFLOW;
    }

    ret = ldg_mem_pool_alloc(buff->payload_pool, src_size, &payload);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mut_unlock(&buff->lock);
        return LDG_ERR_ECS_CMD_OVERFLOW;
    }

    memcpy(payload, src, (uint32_t)src_size);
    rec = &buff->cmd[buff->cmd_cunt];
    rec->entity = entity;
    rec->payload = payload;
    rec->comp_id = comp_id;
    rec->op = LDG_ECS_CMD_OP_COMP_ADD;
    buff->cmd_cunt++;

    return ldg_mut_unlock(&buff->lock);
}

LDG_EXPORT uint32_t ldg_ecs_cmd_comp_remove(ldg_ecs_cmd_buff_t *buff, uint64_t entity, uint32_t comp_id)
{
    ldg_ecs_cmd_t *rec = 0x0;
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!buff->is_init)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mut_lock(&buff->lock);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (buff->cmd_cunt >= buff->cap)
    {
        ldg_mut_unlock(&buff->lock);
        return LDG_ERR_ECS_CMD_OVERFLOW;
    }

    rec = &buff->cmd[buff->cmd_cunt];
    rec->entity = entity;
    rec->payload = 0x0;
    rec->comp_id = comp_id;
    rec->op = LDG_ECS_CMD_OP_COMP_REMOVE;
    buff->cmd_cunt++;

    return ldg_mut_unlock(&buff->lock);
}

LDG_EXPORT uint32_t ldg_ecs_cmd_buff_rst(ldg_ecs_cmd_buff_t *buff)
{
    uint32_t ret = 0x0;

    if (LDG_UNLIKELY(!buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!buff->is_init)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mem_pool_rst(buff->payload_pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    buff->cmd_cunt = 0;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_ecs_cmd_buff_cunt_get(ldg_ecs_cmd_buff_t *buff)
{
    if (LDG_UNLIKELY(!buff)) { return UINT64_MAX; }

    return (uint64_t)buff->cmd_cunt;
}
