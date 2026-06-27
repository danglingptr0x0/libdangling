#include <string.h>

#include <dangling/sched/trace.h>
#include <dangling/mem/alloc.h>
#include <dangling/arch/amd64/fence.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/arch/amd64/syscall.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

static void sched_trace_ring_lock(ldg_sched_trace_ring_t *ring)
{
    uint32_t expected = 0;

    for (;;)
    {
        expected = 0;
        if (LDG_CAS(&ring->lock, &expected, 1)) { return; }

        LDG_PAUSE;
    }
}

LDG_EXPORT uint32_t ldg_sched_trace_init(ldg_sched_trace_ring_t *ring)
{
    void *tmp = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ring)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(ring, 0, sizeof(ldg_sched_trace_ring_t)) != ring)) { return LDG_ERR_INVALID; }

    ret = ldg_mem_alloc(LDG_SCHED_TRACE_RING_SIZE, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(memset(tmp, 0, LDG_SCHED_TRACE_RING_SIZE) != tmp))
    {
        ldg_mem_dealloc(tmp);
        return LDG_ERR_INVALID;
    }

    ring->entries = (ldg_sched_trace_entry_t *)tmp;
    ring->cap = LDG_SCHED_TRACE_ENTRY_CUNT;
    ring->hd = 0;
    ring->is_init = 1;

    LDG_SMP_WMB();

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_trace_shutdown(ldg_sched_trace_ring_t *ring)
{
    if (LDG_UNLIKELY(!ring)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ring->is_init)) { return LDG_ERR_AOK; }

    ring->is_init = 0;

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
    ldg_sched_trace_entry_t *entry = 0x0;

    if (LDG_UNLIKELY(!ring)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ring->is_init || !ring->entries)) { return LDG_ERR_NOT_INIT; }

    sched_trace_ring_lock(ring);

    idx = ring->hd;
    entry = &ring->entries[idx % ring->cap];

    entry->tsc = ldg_rdtsc();
    entry->type = (uint32_t)type;
    entry->data0 = data0;
    entry->data1 = data1;

    ring->hd = idx + 1;

    LDG_STORE_RELEASE(ring->lock, 0);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_sched_trace_pop(ldg_sched_trace_ring_t *ring, ldg_sched_trace_entry_t *entry)
{
    uint64_t hd = 0;
    uint64_t tail = 0;
    uint64_t cunt = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!ring || !entry)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ring->is_init)) { return LDG_ERR_NOT_INIT; }

    sched_trace_ring_lock(ring);

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
        LDG_STORE_RELEASE(ring->lock, 0);
        return LDG_ERR_EMPTY;
    }

    idx = tail % ring->cap;

    entry->tsc = ring->entries[idx].tsc;
    entry->type = ring->entries[idx].type;
    entry->data0 = ring->entries[idx].data0;
    entry->data1 = ring->entries[idx].data1;

    LDG_STORE_RELEASE(ring->lock, 0);

    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_sched_trace_cunt_get(ldg_sched_trace_ring_t *ring)
{
    uint64_t hd = 0;

    if (LDG_UNLIKELY(!ring || !ring->is_init)) { return UINT64_MAX; }

    hd = LDG_RD_ONCE(ring->hd);

    if (hd >= ring->cap) { return ring->cap; }

    return hd;
}

LDG_EXPORT uint32_t ldg_sched_trace_clear(ldg_sched_trace_ring_t *ring)
{
    if (LDG_UNLIKELY(!ring)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ring->is_init)) { return LDG_ERR_NOT_INIT; }

    sched_trace_ring_lock(ring);

    ring->hd = 0;

    LDG_STORE_RELEASE(ring->lock, 0);

    return LDG_ERR_AOK;
}
