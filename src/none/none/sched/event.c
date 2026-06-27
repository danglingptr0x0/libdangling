#include <string.h>

#include <dangling/sched/event.h>
#include <dangling/arch/amd64/fence.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

static uint8_t event_desc_invalid_is(ldg_sched_event_desc_t desc)
{
    return (desc.idx == UINT32_MAX) ? 1 : 0;
}

static void sched_event_pool_lock(ldg_sched_event_pool_t *pool)
{
    uint32_t expected = 0;

    for (;;)
    {
        expected = 0;
        if (LDG_CAS(&pool->lock, &expected, 1)) { return; }

        LDG_PAUSE;
    }
}

LDG_EXPORT uint32_t ldg_sched_event_pool_init(ldg_sched_event_pool_t *pool)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(pool, 0, sizeof(ldg_sched_event_pool_t)) != pool)) { return LDG_ERR_INVALID; }

    for (idx = 0; idx < LDG_SCHED_EVENT_MAX; idx++)
    {
        pool->event_free_list[idx] = idx;
        pool->events[idx].desc.idx = idx;
        pool->events[idx].desc.generation = 0;
    }

    for (idx = 0; idx < LDG_SCHED_SUB_MAX; idx++) { pool->sub_free_list[idx] = idx; }

    pool->is_init = 1;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_event_pool_shutdown(ldg_sched_event_pool_t *pool)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pool->is_init)) { return LDG_ERR_AOK; }

    for (idx = 0; idx < LDG_SCHED_EVENT_MAX; idx++) { if (pool->events[idx].is_active && pool->events[idx].on_destroy) { pool->events[idx].on_destroy(pool->events[idx].desc, pool->events[idx].user_data); } }

    pool->is_init = 0;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_event_create(ldg_sched_event_pool_t *pool, uint64_t filter_mask, ldg_sched_event_desc_t *out_desc)
{
    uint64_t hd = 0;
    uint32_t idx = 0;
    ldg_sched_event_t *event = 0x0;

    if (LDG_UNLIKELY(!pool || !out_desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_desc = LDG_SCHED_EVENT_DESC_INVALID;
    if (LDG_UNLIKELY(!pool->is_init)) { return LDG_ERR_NOT_INIT; }

    sched_event_pool_lock(pool);

    hd = pool->event_free_hd;

    if (LDG_UNLIKELY(hd >= LDG_SCHED_EVENT_MAX))
    {
        LDG_STORE_RELEASE(pool->lock, 0);
        return LDG_ERR_FULL;
    }

    idx = pool->event_free_list[hd];
    pool->event_free_hd = hd + 1;
    pool->event_cunt++;

    LDG_STORE_RELEASE(pool->lock, 0);

    event = &pool->events[idx];
    event->desc.idx = idx;
    event->filter_mask = filter_mask;
    event->ctx = 0x0;
    event->ctx_size = 0;
    event->on_destroy = 0x0;
    event->user_data = 0x0;
    event->is_active = 1;

    LDG_SMP_WMB();

    *out_desc = event->desc;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_event_destroy(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc)
{
    ldg_sched_event_t *event = 0x0;
    uint64_t hd = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pool->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(desc.idx >= LDG_SCHED_EVENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    event = &pool->events[desc.idx];

    sched_event_pool_lock(pool);

    if (LDG_UNLIKELY(!event->is_active))
    {
        LDG_STORE_RELEASE(pool->lock, 0);
        return LDG_ERR_NOT_FOUND;
    }

    if (LDG_UNLIKELY(event->desc.generation != desc.generation))
    {
        LDG_STORE_RELEASE(pool->lock, 0);
        return LDG_ERR_NOT_FOUND;
    }

    event->is_active = 0;
    event->desc.generation++;

    LDG_STORE_RELEASE(pool->lock, 0);

    if (event->on_destroy) { event->on_destroy(desc, event->user_data); }

    event->ctx = 0x0;
    event->ctx_size = 0;
    event->filter_mask = 0;
    event->on_destroy = 0x0;
    event->user_data = 0x0;

    sched_event_pool_lock(pool);

    hd = pool->event_free_hd;

    if (hd > 0)
    {
        pool->event_free_hd = hd - 1;
        pool->event_free_list[hd - 1] = desc.idx;
    }

    pool->event_cunt--;

    LDG_STORE_RELEASE(pool->lock, 0);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint8_t ldg_sched_event_valid_is(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc)
{
    ldg_sched_event_t *event = 0x0;
    uint8_t res = 0;

    if (LDG_UNLIKELY(!pool || !pool->is_init)) { return res; }

    if (LDG_UNLIKELY(event_desc_invalid_is(desc))) { return res; }

    if (LDG_UNLIKELY(desc.idx >= LDG_SCHED_EVENT_MAX)) { return res; }

    event = &pool->events[desc.idx];

    if (!event->is_active) { return res; }

    if (event->desc.generation != desc.generation) { return res; }

    res = 1;

    return res;
}

LDG_EXPORT uint32_t ldg_sched_event_ctx_set(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, void *ctx, uint64_t ctx_size)
{
    ldg_sched_event_t *event = 0x0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pool->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(desc.idx >= LDG_SCHED_EVENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    event = &pool->events[desc.idx];

    if (LDG_UNLIKELY(!event->is_active)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(event->desc.generation != desc.generation)) { return LDG_ERR_NOT_FOUND; }

    event->ctx = ctx;
    event->ctx_size = ctx_size;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_event_ctx_get(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, void **out_ctx)
{
    ldg_sched_event_t *event = 0x0;

    if (LDG_UNLIKELY(!pool || !out_ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_ctx = 0x0;
    if (LDG_UNLIKELY(!pool->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(desc.idx >= LDG_SCHED_EVENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    event = &pool->events[desc.idx];

    if (LDG_UNLIKELY(!event->is_active)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(event->desc.generation != desc.generation)) { return LDG_ERR_NOT_FOUND; }

    *out_ctx = event->ctx;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_sched_event_ctx_size_get(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc)
{
    ldg_sched_event_t *event = 0x0;

    if (LDG_UNLIKELY(!pool || !pool->is_init)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(desc.idx >= LDG_SCHED_EVENT_MAX)) { return UINT64_MAX; }

    event = &pool->events[desc.idx];

    if (LDG_UNLIKELY(!event->is_active)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(event->desc.generation != desc.generation)) { return UINT64_MAX; }

    return event->ctx_size;
}

LDG_EXPORT uint32_t ldg_sched_event_sub(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, ldg_sched_event_handler_cb_t handler, uint64_t filter_mask, uint8_t prio, void *user_data, uint32_t *out_sub_id)
{
    uint64_t hd = 0;
    uint32_t idx = 0;
    ldg_sched_sub_t *sub = 0x0;

    if (LDG_UNLIKELY(!pool || !handler || !out_sub_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sub_id = 0;
    if (LDG_UNLIKELY(!pool->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(!ldg_sched_event_valid_is(pool, desc))) { return LDG_ERR_NOT_FOUND; }

    sched_event_pool_lock(pool);

    hd = pool->sub_free_hd;

    if (LDG_UNLIKELY(hd >= LDG_SCHED_SUB_MAX))
    {
        LDG_STORE_RELEASE(pool->lock, 0);
        return LDG_ERR_FULL;
    }

    idx = pool->sub_free_list[hd];
    pool->sub_free_hd = hd + 1;
    pool->sub_cunt++;

    LDG_STORE_RELEASE(pool->lock, 0);

    sub = &pool->subs[idx];
    sub->id = idx + 1;
    sub->event = desc;
    sub->handler = handler;
    sub->filter_mask = filter_mask;
    sub->prio = prio;
    sub->user_data = user_data;
    sub->on_destroy = 0x0;
    sub->on_owner_change = 0x0;
    sub->is_active = 1;

    LDG_SMP_WMB();

    *out_sub_id = sub->id;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_event_unsub(ldg_sched_event_pool_t *pool, uint32_t sub_id)
{
    ldg_sched_sub_t *sub = 0x0;
    uint64_t hd = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pool->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(sub_id == 0 || sub_id > LDG_SCHED_SUB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = sub_id - 1;
    sub = &pool->subs[idx];

    sched_event_pool_lock(pool);

    if (LDG_UNLIKELY(!sub->is_active))
    {
        LDG_STORE_RELEASE(pool->lock, 0);
        return LDG_ERR_NOT_FOUND;
    }

    sub->is_active = 0;

    LDG_STORE_RELEASE(pool->lock, 0);

    if (sub->on_destroy) { sub->on_destroy(sub->event, sub->user_data); }

    sub->handler = 0x0;
    sub->filter_mask = 0;
    sub->prio = 0;
    sub->user_data = 0x0;

    sched_event_pool_lock(pool);

    hd = pool->sub_free_hd;

    if (hd > 0)
    {
        pool->sub_free_hd = hd - 1;
        pool->sub_free_list[hd - 1] = idx;
    }

    pool->sub_cunt--;

    LDG_STORE_RELEASE(pool->lock, 0);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_event_emit(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc)
{
    ldg_sched_event_t *event = 0x0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pool->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(desc.idx >= LDG_SCHED_EVENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    event = &pool->events[desc.idx];

    return ldg_sched_event_ctx_emit(pool, desc, event->ctx, event->ctx_size);
}

LDG_EXPORT uint32_t ldg_sched_event_ctx_emit(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, void *ctx, uint64_t ctx_size)
{
    ldg_sched_event_t *event = 0x0;
    ldg_sched_sub_t *sorted[LDG_SCHED_EVENT_HANDLER_MAX] = LDG_ARR_ZERO_INIT;
    ldg_sched_sub_t *cand = 0x0;
    uint32_t sorted_cunt = 0;
    uint32_t idx = 0;
    uint32_t pos = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pool->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(!ldg_sched_event_valid_is(pool, desc))) { return LDG_ERR_NOT_FOUND; }

    event = &pool->events[desc.idx];

    sched_event_pool_lock(pool);

    for (idx = 0; idx < LDG_SCHED_SUB_MAX; idx++)
    {
        cand = &pool->subs[idx];

        if (!cand->is_active) { continue; }

        if (cand->event.idx != desc.idx) { continue; }

        if (cand->event.generation != desc.generation) { continue; }

        if ((cand->filter_mask & event->filter_mask) == 0 && cand->filter_mask != 0) { continue; }

        if (sorted_cunt < LDG_SCHED_EVENT_HANDLER_MAX)
        {
            pos = sorted_cunt;
            sorted_cunt++;
        }
        else
        {
            if (cand->prio >= sorted[LDG_SCHED_EVENT_HANDLER_MAX - 1]->prio) { continue; }

            pos = LDG_SCHED_EVENT_HANDLER_MAX - 1;
        }

        while (pos > 0 && sorted[pos - 1]->prio > cand->prio)
        {
            sorted[pos] = sorted[pos - 1];
            pos--;
        }

        sorted[pos] = cand;
    }

    LDG_STORE_RELEASE(pool->lock, 0);

    for (idx = 0; idx < sorted_cunt; idx++)
    {
        if (!sorted[idx]->is_active) { continue; }

        if (sorted[idx]->handler) { sorted[idx]->handler(desc, ctx, ctx_size, sorted[idx]->user_data); }
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_event_on_destroy_set(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, ldg_sched_event_on_destroy_cb_t cb, void *user_data)
{
    ldg_sched_event_t *event = 0x0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pool->is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(desc.idx >= LDG_SCHED_EVENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    event = &pool->events[desc.idx];

    if (LDG_UNLIKELY(!event->is_active)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(event->desc.generation != desc.generation)) { return LDG_ERR_NOT_FOUND; }

    event->on_destroy = cb;
    event->user_data = user_data;

    return LDG_ERR_AOK;
}
