
#include <dangling/sched/trace.h>
#include <dangling/mem/alloc.h>
#include <dangling/arch/fence.h>
#include <dangling/arch/atomic.h>
#include <dangling/time/time.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

static uint32_t sched_trace_ring_lock(ldg_sched_trace_ring_t *ring)
{
    uint32_t expected = 0;
    uint32_t spin = 0;

    for (spin = 0; spin < LDG_SCHED_TRACE_LOCK_SPIN_MAX; spin++)
    {
        expected = LDG_SCHED_TRACE_LOCK_FREE;
        if (LDG_CAS(&ring->lock, &expected, LDG_SCHED_TRACE_LOCK_HELD)) { return LDG_ERR_AOK; }

        LDG_PAUSE;
    }

    return LDG_ERR_BUSY;
}

LDG_EXPORT uint32_t ldg_sched_trace_init(ldg_sched_trace_ring_t *ring)
{
    void *tmp = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ring)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_zero(ring, sizeof(ldg_sched_trace_ring_t)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    ret = ldg_mem_alloc(LDG_SCHED_TRACE_RING_SIZE, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ldg_mem_zero(tmp, LDG_SCHED_TRACE_RING_SIZE) != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(tmp);
        return LDG_ERR_INVALID;
    }

    ring->entries = (ldg_sched_trace_entry_t *)tmp;
    ring->cap = LDG_SCHED_TRACE_ENTRY_CUNT;
    ring->hd = 0;
    ring->is_init = LDG_TRUTH_TRUE;

    LDG_SMP_WMB();

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_trace_shutdown(ldg_sched_trace_ring_t *ring)
{
    if (LDG_UNLIKELY(!ring)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ring->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_AOK; }

    ring->is_init = LDG_TRUTH_FALSE;

    LDG_SMP_MB();

    if (ring->entries)
    {
        ldg_mem_dealloc(ring->entries);
        ring->entries = 0x0;
    }

    ring->hd = 0;
    ring->cap = 0;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_trace_push(ldg_sched_trace_ring_t *ring, ldg_sched_trace_type_t type, uint32_t data0, uint64_t data1)
{
    uint64_t idx = 0;
    uint32_t ret = 0;
    ldg_sched_trace_entry_t *entry = 0x0;

    if (LDG_UNLIKELY(!ring)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ring->is_init != LDG_TRUTH_TRUE || !ring->entries)) { return LDG_ERR_NOT_INIT; }

    ret = sched_trace_ring_lock(ring);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    idx = ring->hd;
    entry = &ring->entries[idx % ring->cap];

    entry->tsc = ldg_time_cycle_get();
    entry->type = (uint32_t)type;
    entry->data0 = data0;
    entry->data1 = data1;

    ring->hd = idx + LDG_SCHED_TRACE_STEP;

    LDG_ST_RELEASE(ring->lock, LDG_SCHED_TRACE_LOCK_FREE);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_trace_pop(ldg_sched_trace_ring_t *ring, ldg_sched_trace_entry_t *entry)
{
    uint64_t hd = 0;
    uint64_t tail = 0;
    uint64_t cunt = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ring || !entry)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ring->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = sched_trace_ring_lock(ring);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    hd = ring->hd;

    if (hd >= ring->cap)
    {
        cunt = ring->cap;
        tail = hd - ring->cap;
    }
    else
    {
        cunt = hd;
        tail = 0;
    }

    if (cunt == 0)
    {
        LDG_ST_RELEASE(ring->lock, LDG_SCHED_TRACE_LOCK_FREE);
        return LDG_ERR_EMPTY;
    }

    idx = tail % ring->cap;

    entry->tsc = ring->entries[idx].tsc;
    entry->type = ring->entries[idx].type;
    entry->data0 = ring->entries[idx].data0;
    entry->data1 = ring->entries[idx].data1;

    LDG_ST_RELEASE(ring->lock, LDG_SCHED_TRACE_LOCK_FREE);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_sched_trace_cunt_get(ldg_sched_trace_ring_t *ring)
{
    uint64_t hd = 0;

    if (LDG_UNLIKELY(!ring || ring->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    hd = LDG_RD_ONCE(ring->hd);

    if (hd >= ring->cap) { return ring->cap; }

    return hd;
}

LDG_EXPORT uint32_t ldg_sched_trace_clear(ldg_sched_trace_ring_t *ring)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ring)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ring->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = sched_trace_ring_lock(ring);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ring->hd = 0;

    LDG_ST_RELEASE(ring->lock, LDG_SCHED_TRACE_LOCK_FREE);

    return LDG_ERR_AOK;
}
