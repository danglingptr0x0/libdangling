#ifndef LDG_SCHED_EVENT_H
#define LDG_SCHED_EVENT_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_SCHED_EVENT_MAX 1024
#define LDG_SCHED_SUB_MAX 4096
#define LDG_SCHED_EVENT_HANDLER_MAX 64

typedef struct ldg_sched_event_desc
{
    uint32_t idx;
    uint32_t generation;
} ldg_sched_event_desc_t;

#define LDG_SCHED_EVENT_DESC_INVALID ((ldg_sched_event_desc_t) { .idx = UINT32_MAX, .generation = UINT32_MAX })

typedef void (*ldg_sched_event_handler_cb_t)(ldg_sched_event_desc_t desc, void *ctx, uint64_t ctx_size, void *user_data);
typedef void (*ldg_sched_event_on_destroy_cb_t)(ldg_sched_event_desc_t desc, void *user_data);
typedef void (*ldg_sched_event_on_owner_change_cb_t)(ldg_sched_event_desc_t desc, void *old_owner, void *new_owner, void *user_data);

typedef struct ldg_sched_event
{
    ldg_sched_event_desc_t desc;
    uint64_t filter_mask;
    void *ctx;
    uint64_t ctx_size;
    ldg_sched_event_on_destroy_cb_t on_destroy;
    void *user_data;
    uint8_t is_active;
    uint8_t pudding[15];
} LDG_ALIGNED ldg_sched_event_t;

typedef struct ldg_sched_sub
{
    ldg_sched_event_handler_cb_t handler;
    ldg_sched_event_on_destroy_cb_t on_destroy;
    ldg_sched_event_on_owner_change_cb_t on_owner_change;
    uint64_t filter_mask;
    void *user_data;
    ldg_sched_event_desc_t event;
    uint32_t id;
    uint8_t prio;
    uint8_t is_active;
    uint8_t pudding[10];
} LDG_ALIGNED ldg_sched_sub_t;

typedef struct ldg_sched_event_pool
{
    ldg_sched_event_t events[LDG_SCHED_EVENT_MAX];
    ldg_sched_sub_t subs[LDG_SCHED_SUB_MAX];
    uint32_t event_free_list[LDG_SCHED_EVENT_MAX];
    uint32_t sub_free_list[LDG_SCHED_SUB_MAX];
    volatile uint64_t event_free_hd;
    volatile uint64_t sub_free_hd;
    uint32_t event_cunt;
    uint32_t sub_cunt;
    volatile uint32_t lock;
    uint8_t is_init;
    uint8_t pudding[35];
} LDG_ALIGNED ldg_sched_event_pool_t;

LDG_EXPORT uint32_t ldg_sched_event_pool_init(ldg_sched_event_pool_t *pool);
LDG_EXPORT uint32_t ldg_sched_event_pool_shutdown(ldg_sched_event_pool_t *pool);
LDG_EXPORT uint32_t ldg_sched_event_create(ldg_sched_event_pool_t *pool, uint64_t filter_mask, ldg_sched_event_desc_t *out_desc);
LDG_EXPORT uint32_t ldg_sched_event_destroy(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc);
LDG_EXPORT uint8_t ldg_sched_event_valid_is(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc);
LDG_EXPORT uint32_t ldg_sched_event_ctx_set(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, void *ctx, uint64_t ctx_size);
LDG_EXPORT uint32_t ldg_sched_event_ctx_get(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, void **out_ctx);
LDG_EXPORT uint64_t ldg_sched_event_ctx_size_get(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc);
LDG_EXPORT uint32_t ldg_sched_event_sub(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, ldg_sched_event_handler_cb_t handler, uint64_t filter_mask, uint8_t prio, void *user_data, uint32_t *out_sub_id);
LDG_EXPORT uint32_t ldg_sched_event_unsub(ldg_sched_event_pool_t *pool, uint32_t sub_id);
LDG_EXPORT uint32_t ldg_sched_event_emit(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc);
LDG_EXPORT uint32_t ldg_sched_event_ctx_emit(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, void *ctx, uint64_t ctx_size);
LDG_EXPORT uint32_t ldg_sched_event_on_destroy_set(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, ldg_sched_event_on_destroy_cb_t cb, void *user_data);

#endif
