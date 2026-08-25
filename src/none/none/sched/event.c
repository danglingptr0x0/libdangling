
#include <dangling/sched/event.h>
#include <dangling/arch/fence.h>
#include <dangling/arch/atomic.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

static uint8_t event_desc_valid_is(ldg_sched_event_desc_t desc)
{
    if (desc.idx == UINT32_MAX) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t sched_event_pool_lock(ldg_sched_event_pool_t *pool)
{
    uint32_t expected = 0;
    uint32_t spin = 0;

    for (spin = 0; spin < LDG_SCHED_EVENT_LOCK_SPIN_MAX; spin++)
    {
        expected = LDG_SCHED_EVENT_LOCK_FREE;
        if (LDG_CAS(&pool->lock, &expected, LDG_SCHED_EVENT_LOCK_HELD)) { return LDG_ERR_AOK; }

        LDG_PAUSE;
    }

    return LDG_ERR_BUSY;
}

LDG_EXPORT uint32_t ldg_sched_event_pool_init(ldg_sched_event_pool_t *pool)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_zero(pool, sizeof(ldg_sched_event_pool_t)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    pool->is_init = UINT8_MAX;

    for (idx = 0; idx < LDG_SCHED_EVENT_MAX; idx++)
    {
        pool->event_free_list[idx] = idx;
        pool->events[idx].desc.idx = idx;
        pool->events[idx].desc.generation = 0;
        pool->events[idx].is_active = LDG_TRUTH_FALSE;
    }

    for (idx = 0; idx < LDG_SCHED_SUB_MAX; idx++)
    {
        pool->sub_free_list[idx] = idx;
        pool->subs[idx].is_active = LDG_TRUTH_FALSE;
    }

    pool->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_event_pool_shutdown(ldg_sched_event_pool_t *pool)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AOK; }

    for (idx = 0; idx < LDG_SCHED_EVENT_MAX; idx++) { if (pool->events[idx].is_active == LDG_TRUTH_TRUE && pool->events[idx].on_destroy) { pool->events[idx].on_destroy(pool->events[idx].desc, pool->events[idx].user_data); } }

    pool->is_init = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_event_create(ldg_sched_event_pool_t *pool, uint64_t filter_mask, ldg_sched_event_desc_t *out_desc)
{
    uint64_t hd = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    ldg_sched_event_t *event = 0x0;

    if (LDG_UNLIKELY(!pool || !out_desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_desc = LDG_SCHED_EVENT_DESC_INVALID;
    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = sched_event_pool_lock(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    hd = pool->event_free_hd;

    if (LDG_UNLIKELY(hd >= LDG_SCHED_EVENT_MAX))
    {
        LDG_ST_RELEASE(pool->lock, LDG_SCHED_EVENT_LOCK_FREE);
        return LDG_ERR_FULL;
    }

    idx = pool->event_free_list[hd];
    pool->event_free_hd = hd + 1;
    pool->event_cunt++;

    LDG_ST_RELEASE(pool->lock, LDG_SCHED_EVENT_LOCK_FREE);

    event = &pool->events[idx];
    event->desc.idx = idx;
    event->filter_mask = filter_mask;
    event->ctx = 0x0;
    event->ctx_size = 0;
    event->on_destroy = 0x0;
    event->user_data = 0x0;
    event->is_active = LDG_TRUTH_TRUE;

    LDG_SMP_WMB();

    *out_desc = event->desc;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_event_destroy(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc)
{
    ldg_sched_event_t *event = 0x0;
    uint64_t hd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(desc.idx >= LDG_SCHED_EVENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    event = &pool->events[desc.idx];

    ret = sched_event_pool_lock(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(event->is_active != LDG_TRUTH_TRUE))
    {
        LDG_ST_RELEASE(pool->lock, LDG_SCHED_EVENT_LOCK_FREE);
        return LDG_ERR_NOT_FOUND;
    }

    if (LDG_UNLIKELY(event->desc.generation != desc.generation))
    {
        LDG_ST_RELEASE(pool->lock, LDG_SCHED_EVENT_LOCK_FREE);
        return LDG_ERR_NOT_FOUND;
    }

    event->is_active = LDG_TRUTH_FALSE;
    event->desc.generation++;

    LDG_ST_RELEASE(pool->lock, LDG_SCHED_EVENT_LOCK_FREE);

    if (event->on_destroy) { event->on_destroy(desc, event->user_data); }

    event->ctx = 0x0;
    event->ctx_size = 0;
    event->filter_mask = 0;
    event->on_destroy = 0x0;
    event->user_data = 0x0;

    ret = sched_event_pool_lock(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    hd = pool->event_free_hd;

    if (hd > 0)
    {
        pool->event_free_hd = hd - LDG_SCHED_EVENT_STEP;
        pool->event_free_list[hd - LDG_SCHED_EVENT_STEP] = desc.idx;
    }

    pool->event_cunt--;

    LDG_ST_RELEASE(pool->lock, LDG_SCHED_EVENT_LOCK_FREE);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint8_t ldg_sched_event_valid_is(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc)
{
    ldg_sched_event_t *event = 0x0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (event_desc_valid_is(desc) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (desc.idx >= LDG_SCHED_EVENT_MAX) { return LDG_TRUTH_FALSE; }

    event = &pool->events[desc.idx];

    if (event->is_active != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (event->desc.generation != desc.generation) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

LDG_EXPORT uint32_t ldg_sched_event_ctx_set(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, void *ctx, uint64_t ctx_size)
{
    ldg_sched_event_t *event = 0x0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(desc.idx >= LDG_SCHED_EVENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    event = &pool->events[desc.idx];

    if (LDG_UNLIKELY(event->is_active != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

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
    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(desc.idx >= LDG_SCHED_EVENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    event = &pool->events[desc.idx];

    if (LDG_UNLIKELY(event->is_active != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(event->desc.generation != desc.generation)) { return LDG_ERR_NOT_FOUND; }

    *out_ctx = event->ctx;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_sched_event_ctx_size_get(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc)
{
    ldg_sched_event_t *event = 0x0;

    if (LDG_UNLIKELY(!pool || pool->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(desc.idx >= LDG_SCHED_EVENT_MAX)) { return UINT64_MAX; }

    event = &pool->events[desc.idx];

    if (LDG_UNLIKELY(event->is_active != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(event->desc.generation != desc.generation)) { return UINT64_MAX; }

    return event->ctx_size;
}

LDG_EXPORT uint32_t ldg_sched_event_sub(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, ldg_sched_event_handler_cb_t handler, uint64_t filter_mask, uint8_t prio, void *user_data, uint32_t *out_sub_id)
{
    uint64_t hd = 0;
    uint32_t idx = 0;
    uint32_t scan = 0;
    uint32_t event_sub_cunt = 0;
    uint32_t ret = 0;
    ldg_sched_sub_t *sub = 0x0;

    if (LDG_UNLIKELY(!pool || !handler || !out_sub_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sub_id = 0;
    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_sched_event_valid_is(pool, desc) != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    ret = sched_event_pool_lock(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (scan = 0; scan < LDG_SCHED_SUB_MAX; scan++)
    {
        if (pool->subs[scan].is_active != LDG_TRUTH_TRUE) { continue; }

        if (pool->subs[scan].event.idx != desc.idx) { continue; }

        if (pool->subs[scan].event.generation != desc.generation) { continue; }

        event_sub_cunt++;
    }

    if (LDG_UNLIKELY(event_sub_cunt >= LDG_SCHED_EVENT_HANDLER_MAX))
    {
        LDG_ST_RELEASE(pool->lock, LDG_SCHED_EVENT_LOCK_FREE);
        return LDG_ERR_FULL;
    }

    hd = pool->sub_free_hd;

    if (LDG_UNLIKELY(hd >= LDG_SCHED_SUB_MAX))
    {
        LDG_ST_RELEASE(pool->lock, LDG_SCHED_EVENT_LOCK_FREE);
        return LDG_ERR_FULL;
    }

    idx = pool->sub_free_list[hd];
    pool->sub_free_hd = hd + LDG_SCHED_EVENT_STEP;
    pool->sub_cunt++;

    sub = &pool->subs[idx];
    sub->id = idx + LDG_SCHED_EVENT_SUB_ID_BASE;
    sub->event = desc;
    sub->handler = handler;
    sub->filter_mask = filter_mask;
    sub->prio = prio;
    sub->user_data = user_data;
    sub->on_destroy = 0x0;
    sub->on_owner_change = 0x0;
    sub->in_flight = 0;

    LDG_ST_RELEASE(sub->is_active, LDG_TRUTH_TRUE);

    *out_sub_id = sub->id;

    LDG_ST_RELEASE(pool->lock, LDG_SCHED_EVENT_LOCK_FREE);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_event_unsub(ldg_sched_event_pool_t *pool, uint32_t sub_id)
{
    ldg_sched_sub_t *sub = 0x0;
    uint64_t hd = 0;
    uint32_t idx = 0;
    uint32_t spin = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(sub_id == 0 || sub_id > LDG_SCHED_SUB_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = sub_id - LDG_SCHED_EVENT_SUB_ID_BASE;
    sub = &pool->subs[idx];

    ret = sched_event_pool_lock(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(sub->is_active != LDG_TRUTH_TRUE))
    {
        LDG_ST_RELEASE(pool->lock, LDG_SCHED_EVENT_LOCK_FREE);
        return LDG_ERR_NOT_FOUND;
    }

    sub->generation++;

    LDG_ST_RELEASE(sub->is_active, LDG_TRUTH_FALSE);

    LDG_ST_RELEASE(pool->lock, LDG_SCHED_EVENT_LOCK_FREE);

    for (spin = 0; spin < LDG_SCHED_EVENT_DRAIN_SPIN_MAX; spin++)
    {
        if (LDG_LD_ACQUIRE(sub->in_flight) == 0) { break; }

        LDG_PAUSE;
    }

    if (LDG_UNLIKELY(spin >= LDG_SCHED_EVENT_DRAIN_SPIN_MAX)) { return LDG_ERR_TIMEOUT; }

    if (sub->on_destroy) { sub->on_destroy(sub->event, sub->user_data); }

    sub->handler = 0x0;
    sub->filter_mask = 0;
    sub->prio = 0;
    sub->user_data = 0x0;

    ret = sched_event_pool_lock(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    hd = pool->sub_free_hd;

    if (hd > 0)
    {
        pool->sub_free_hd = hd - LDG_SCHED_EVENT_STEP;
        pool->sub_free_list[hd - LDG_SCHED_EVENT_STEP] = idx;
    }

    pool->sub_cunt--;

    LDG_ST_RELEASE(pool->lock, LDG_SCHED_EVENT_LOCK_FREE);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_event_emit(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc)
{
    ldg_sched_event_t *event = 0x0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(desc.idx >= LDG_SCHED_EVENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    event = &pool->events[desc.idx];

    return ldg_sched_event_ctx_emit(pool, desc, event->ctx, event->ctx_size);
}

LDG_EXPORT uint32_t ldg_sched_event_ctx_emit(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, void *ctx, uint64_t ctx_size)
{
    ldg_sched_event_t *event = 0x0;
    uint32_t sorted_idx[LDG_SCHED_EVENT_HANDLER_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t sorted_generation[LDG_SCHED_EVENT_HANDLER_MAX] = LDG_ARR_ZERO_INIT;
    uint8_t sorted_prio[LDG_SCHED_EVENT_HANDLER_MAX] = LDG_ARR_ZERO_INIT;
    ldg_sched_sub_t *cand = 0x0;
    ldg_sched_event_handler_cb_t handler = 0x0;
    void *user_data = 0x0;
    uint32_t sorted_cunt = 0;
    uint32_t idx = 0;
    uint32_t pos = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_sched_event_valid_is(pool, desc) != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    event = &pool->events[desc.idx];

    ret = sched_event_pool_lock(pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < LDG_SCHED_SUB_MAX; idx++)
    {
        cand = &pool->subs[idx];

        if (cand->is_active != LDG_TRUTH_TRUE) { continue; }

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
            if (cand->prio >= sorted_prio[LDG_SCHED_EVENT_HANDLER_MAX - LDG_SCHED_EVENT_STEP]) { continue; }

            pos = LDG_SCHED_EVENT_HANDLER_MAX - LDG_SCHED_EVENT_STEP;
            LDG_FETCH_SUB(pool->subs[sorted_idx[pos]].in_flight, LDG_SCHED_EVENT_STEP);
        }

        while (pos > 0 && sorted_prio[pos - LDG_SCHED_EVENT_STEP] > cand->prio)
        {
            sorted_idx[pos] = sorted_idx[pos - LDG_SCHED_EVENT_STEP];
            sorted_generation[pos] = sorted_generation[pos - LDG_SCHED_EVENT_STEP];
            sorted_prio[pos] = sorted_prio[pos - LDG_SCHED_EVENT_STEP];
            pos--;
        }

        sorted_idx[pos] = idx;
        sorted_generation[pos] = cand->generation;
        sorted_prio[pos] = cand->prio;
        LDG_FETCH_ADD(cand->in_flight, LDG_SCHED_EVENT_STEP);
    }

    LDG_ST_RELEASE(pool->lock, LDG_SCHED_EVENT_LOCK_FREE);

    for (idx = 0; idx < sorted_cunt; idx++)
    {
        cand = &pool->subs[sorted_idx[idx]];

        handler = cand->handler;
        user_data = cand->user_data;

        if (LDG_LD_ACQUIRE(cand->is_active) == LDG_TRUTH_TRUE && cand->generation == sorted_generation[idx] && handler) { handler(desc, ctx, ctx_size, user_data); }

        LDG_FETCH_SUB(cand->in_flight, LDG_SCHED_EVENT_STEP);
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_event_on_destroy_set(ldg_sched_event_pool_t *pool, ldg_sched_event_desc_t desc, ldg_sched_event_on_destroy_cb_t cb, void *user_data)
{
    ldg_sched_event_t *event = 0x0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pool->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(desc.idx >= LDG_SCHED_EVENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    event = &pool->events[desc.idx];

    if (LDG_UNLIKELY(event->is_active != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(event->desc.generation != desc.generation)) { return LDG_ERR_NOT_FOUND; }

    event->on_destroy = cb;
    event->user_data = user_data;

    return LDG_ERR_AOK;
}
