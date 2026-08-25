
#include <dangling/time/timer.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

static uint32_t timer_bkt_idx_calc(ldg_time_timer_wheel_t *wheel, uint64_t fire_time_ns)
{
    uint64_t fire_tick = 0;
    uint32_t bkt_idx = 0;

    if (LDG_UNLIKELY(wheel->tick_interval_ns == 0)) { return bkt_idx; }

    fire_tick = fire_time_ns / wheel->tick_interval_ns;
    bkt_idx = (uint32_t)(fire_tick % LDG_TIME_TIMER_WHEEL_SIZE);

    return bkt_idx;
}

static void timer_bkt_insert(ldg_time_timer_wheel_t *wheel, uint32_t timer_idx, uint32_t bkt_idx)
{
    ldg_time_timer_t *timer = 0x0;
    ldg_time_timer_bkt_t *bkt = 0x0;

    timer = &wheel->timers[timer_idx];
    bkt = &wheel->bkts[bkt_idx];

    timer->bkt_idx = bkt_idx;
    timer->next_in_bkt = bkt->hd;
    bkt->hd = timer_idx + 1;
    bkt->cunt++;
}

static void timer_bkt_remove(ldg_time_timer_wheel_t *wheel, uint32_t timer_idx, uint32_t bkt_idx)
{
    ldg_time_timer_bkt_t *bkt = 0x0;
    uint32_t curr = 0;
    uint32_t prev = 0;

    bkt = &wheel->bkts[bkt_idx];

    if (bkt->hd == 0) { return; }

    if (bkt->hd == timer_idx + 1)
    {
        bkt->hd = wheel->timers[timer_idx].next_in_bkt;
        bkt->cunt--;
        return;
    }

    prev = bkt->hd - 1;
    curr = wheel->timers[prev].next_in_bkt;

    while (curr != 0)
    {
        if (curr == timer_idx + 1)
        {
            wheel->timers[prev].next_in_bkt = wheel->timers[timer_idx].next_in_bkt;
            bkt->cunt--;
            return;
        }

        prev = curr - 1;
        curr = wheel->timers[prev].next_in_bkt;
    }
}

LDG_EXPORT uint32_t ldg_time_timer_wheel_init(ldg_time_timer_wheel_t *wheel, uint64_t tick_interval_ns)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!wheel)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(tick_interval_ns == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ldg_mem_zero(wheel, sizeof(ldg_time_timer_wheel_t)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    wheel->is_init = UINT8_MAX;

    for (idx = 0; idx < LDG_TIME_TIMER_MAX; idx++)
    {
        wheel->free_list[idx] = idx;
        wheel->timers[idx].id = idx + 1;
        wheel->timers[idx].is_active = LDG_TRUTH_FALSE;
    }

    wheel->tick_interval_ns = tick_interval_ns;
    wheel->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_time_timer_wheel_shutdown(ldg_time_timer_wheel_t *wheel)
{
    if (LDG_UNLIKELY(!wheel)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(wheel->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AOK; }

    wheel->is_init = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_time_timer_create(ldg_time_timer_wheel_t *wheel, uint64_t interval_ns, uint8_t repeat, ldg_time_timer_cb_t func, void *arg, ldg_time_timer_id_t *out_id)
{
    uint64_t hd = 0;
    uint32_t idx = 0;
    uint32_t bkt_idx = 0;
    ldg_time_timer_t *timer = 0x0;

    if (LDG_UNLIKELY(!wheel || !func || !out_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_id = LDG_TIME_TIMER_ID_INVALID;
    if (LDG_UNLIKELY(wheel->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(interval_ns == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    hd = wheel->free_hd;

    if (LDG_UNLIKELY(hd >= LDG_TIME_TIMER_MAX)) { return LDG_ERR_FULL; }

    idx = wheel->free_list[hd];
    timer = &wheel->timers[idx];

    wheel->free_hd = hd + 1;

    timer->id = idx + 1;
    timer->interval_ns = interval_ns;
    timer->next_fire_ns = wheel->current_time_ns + interval_ns;
    timer->func = func;
    timer->arg = arg;
    timer->repeat = (repeat == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    timer->is_active = LDG_TRUTH_TRUE;
    timer->next_in_bkt = 0;

    bkt_idx = timer_bkt_idx_calc(wheel, timer->next_fire_ns);
    timer_bkt_insert(wheel, idx, bkt_idx);

    wheel->timer_cunt++;

    *out_id = timer->id;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_time_timer_destroy(ldg_time_timer_wheel_t *wheel, ldg_time_timer_id_t id)
{
    ldg_time_timer_t *timer = 0x0;
    uint64_t hd = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!wheel)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(wheel->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(id == LDG_TIME_TIMER_ID_INVALID || id > LDG_TIME_TIMER_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    timer = &wheel->timers[idx];

    if (LDG_UNLIKELY(timer->is_active != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    timer_bkt_remove(wheel, idx, timer->bkt_idx);

    timer->is_active = LDG_TRUTH_FALSE;
    timer->func = 0x0;
    timer->arg = 0x0;

    hd = wheel->free_hd;

    if (hd > 0)
    {
        wheel->free_hd = hd - 1;
        wheel->free_list[hd - 1] = idx;
    }

    wheel->timer_cunt--;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint8_t ldg_time_timer_active_is(ldg_time_timer_wheel_t *wheel, ldg_time_timer_id_t id)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!wheel)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(wheel->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(id == LDG_TIME_TIMER_ID_INVALID || id > LDG_TIME_TIMER_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;

    if (wheel->timers[idx].is_active == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

LDG_EXPORT uint32_t ldg_time_timer_rst(ldg_time_timer_wheel_t *wheel, ldg_time_timer_id_t id)
{
    ldg_time_timer_t *timer = 0x0;
    uint32_t idx = 0;
    uint32_t new_bkt = 0;

    if (LDG_UNLIKELY(!wheel)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(wheel->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(id == LDG_TIME_TIMER_ID_INVALID || id > LDG_TIME_TIMER_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    timer = &wheel->timers[idx];

    if (LDG_UNLIKELY(timer->is_active != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    timer_bkt_remove(wheel, idx, timer->bkt_idx);

    timer->next_fire_ns = wheel->current_time_ns + timer->interval_ns;
    new_bkt = timer_bkt_idx_calc(wheel, timer->next_fire_ns);

    timer_bkt_insert(wheel, idx, new_bkt);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_time_timer_interval_set(ldg_time_timer_wheel_t *wheel, ldg_time_timer_id_t id, uint64_t interval_ns)
{
    ldg_time_timer_t *timer = 0x0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!wheel)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(wheel->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(id == LDG_TIME_TIMER_ID_INVALID || id > LDG_TIME_TIMER_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(interval_ns == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = id - 1;
    timer = &wheel->timers[idx];

    if (LDG_UNLIKELY(timer->is_active != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    timer->interval_ns = interval_ns;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_time_timer_wheel_tick(ldg_time_timer_wheel_t *wheel, uint64_t current_time_ns)
{
    ldg_time_timer_bkt_t *bkt = 0x0;
    ldg_time_timer_t *timer = 0x0;
    uint64_t now_tick = 0;
    uint64_t hd = 0;
    uint32_t bkt_idx = 0;
    uint32_t curr = 0;
    uint32_t next = 0;
    uint32_t new_bkt = 0;
    uint32_t timer_idx = 0;

    if (LDG_UNLIKELY(!wheel)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(wheel->is_init != LDG_TRUTH_TRUE || wheel->tick_interval_ns == 0)) { return LDG_ERR_NOT_INIT; }

    wheel->current_time_ns = current_time_ns;
    now_tick = current_time_ns / wheel->tick_interval_ns;

    while (wheel->current_tick < now_tick)
    {
        wheel->current_tick++;
        bkt_idx = (uint32_t)(wheel->current_tick % LDG_TIME_TIMER_WHEEL_SIZE);
        bkt = &wheel->bkts[bkt_idx];
        curr = bkt->hd;

        while (curr != 0)
        {
            timer_idx = curr - 1;
            timer = &wheel->timers[timer_idx];
            next = timer->next_in_bkt;

            if (timer->is_active == LDG_TRUTH_TRUE && timer->next_fire_ns <= current_time_ns)
            {
                if (timer->func) { timer->func(timer->id, timer->arg); }

                timer_bkt_remove(wheel, timer_idx, bkt_idx);

                if (timer->repeat == LDG_TRUTH_TRUE && timer->is_active == LDG_TRUTH_TRUE)
                {
                    timer->next_fire_ns = current_time_ns + timer->interval_ns;
                    new_bkt = timer_bkt_idx_calc(wheel, timer->next_fire_ns);
                    timer_bkt_insert(wheel, timer_idx, new_bkt);
                }
                else
                {
                    timer->is_active = LDG_TRUTH_FALSE;
                    timer->func = 0x0;
                    timer->arg = 0x0;

                    hd = wheel->free_hd;

                    if (hd > 0)
                    {
                        wheel->free_hd = hd - 1;
                        wheel->free_list[hd - 1] = timer_idx;
                    }

                    wheel->timer_cunt--;
                }
            }

            curr = next;
        }
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_time_timer_remaining_get(ldg_time_timer_wheel_t *wheel, ldg_time_timer_id_t id)
{
    ldg_time_timer_t *timer = 0x0;
    uint64_t remaining = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!wheel || wheel->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(id == LDG_TIME_TIMER_ID_INVALID || id > LDG_TIME_TIMER_MAX)) { return UINT64_MAX; }

    idx = id - 1;
    timer = &wheel->timers[idx];

    if (LDG_UNLIKELY(timer->is_active != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    if (timer->next_fire_ns > wheel->current_time_ns) { remaining = timer->next_fire_ns - wheel->current_time_ns; }

    return remaining;
}

LDG_EXPORT uint64_t ldg_time_timer_cunt_get(ldg_time_timer_wheel_t *wheel)
{
    if (LDG_UNLIKELY(!wheel || wheel->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    return wheel->timer_cunt;
}
