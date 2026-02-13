#include <string.h>
#include <windows.h>

#include <dangling/thread/mpmc.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/macros.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/arch/amd64/fence.h>

#define MPMC_MAX_SPIN 1024
#define MPMC_WAIT_MAX_ITER 4096

static uint64_t mpmc_monotonic_ms_get(void)
{
    return (uint64_t)GetTickCount64();
}

// slot

static uint32_t mpmc_slot_get(ldg_mpmc_queue_t *q, uint64_t idx, ldg_mpmc_slot_t **out)
{
    if (LDG_UNLIKELY(!q || !q->buff || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(idx >= q->capacity)) { return LDG_ERR_BOUNDS; }

    *out = (ldg_mpmc_slot_t *)(q->buff + (idx * q->slot_size));

    return LDG_ERR_AOK;
}

static uint8_t mpmc_capacity_pow2_is(uint64_t capacity)
{
    return (capacity > 0) && ((capacity & (capacity - 1)) == 0);
}

uint32_t ldg_mpmc_init(ldg_mpmc_queue_t *q, uint64_t item_size, uint64_t capacity)
{
    uint64_t i = 0;
    uint64_t slot_data_offset = 0;
    ldg_mpmc_slot_t *slot = 0x0;
    uint32_t ret = 0;
    void *buff_tmp = 0x0;

    if (LDG_UNLIKELY(!q)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(item_size == 0 || capacity == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!mpmc_capacity_pow2_is(capacity))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(q, 0, sizeof(ldg_mpmc_queue_t)) != q)) { return LDG_ERR_MEM_BAD; }

    slot_data_offset = LDG_AMD64_CACHE_LINE_WIDTH;
    if (LDG_UNLIKELY(item_size > UINT64_MAX - slot_data_offset)) { return LDG_ERR_OVERFLOW; }

    q->slot_size = LDG_ALIGNED_UP(slot_data_offset + item_size);
    q->item_size = item_size;
    q->capacity = capacity;
    q->mask = capacity - 1;

    if (LDG_UNLIKELY(capacity != 0 && q->slot_size > UINT64_MAX / capacity)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_alloc(q->slot_size * capacity, &buff_tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    q->buff = (uint8_t *)buff_tmp;

    if (LDG_UNLIKELY(memset(q->buff, 0, q->slot_size * capacity) != q->buff)) { ldg_mem_dealloc(q->buff); q->buff = 0x0; return LDG_ERR_MEM_BAD; }

    for (i = 0; i < capacity; i++)
    {
        if (LDG_UNLIKELY(mpmc_slot_get(q, i, &slot) != LDG_ERR_AOK)) { ldg_mem_dealloc(q->buff); q->buff = 0x0; return LDG_ERR_MEM_BAD; }

        LDG_STORE_RELEASE(slot->seq, i);
    }

    q->head = 0;
    q->tail = 0;

    if (LDG_UNLIKELY(ldg_mut_init(&q->wait_mut, 0) != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(q->buff);
        q->buff = 0x0;
        return LDG_ERR_ALLOC_NULL;
    }

    if (LDG_UNLIKELY(ldg_cond_init(&q->wait_cond, 0) != LDG_ERR_AOK))
    {
        ldg_mut_destroy(&q->wait_mut);
        ldg_mem_dealloc(q->buff);
        q->buff = 0x0;
        return LDG_ERR_ALLOC_NULL;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_mpmc_shutdown(ldg_mpmc_queue_t *q)
{
    uint32_t first_err = LDG_ERR_AOK;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!q)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_cond_bcast(&q->wait_cond);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && first_err == LDG_ERR_AOK)) { first_err = ret; }

    ret = ldg_cond_destroy(&q->wait_cond);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && first_err == LDG_ERR_AOK)) { first_err = ret; }

    ret = ldg_mut_destroy(&q->wait_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK && first_err == LDG_ERR_AOK)) { first_err = ret; }

    if (q->buff)
    {
        ldg_mem_dealloc(q->buff);
        q->buff = 0x0;
    }

    q->slot_size = 0;
    q->item_size = 0;
    q->capacity = 0;
    q->mask = 0;
    q->head = 0;
    q->tail = 0;

    return first_err;
}

uint32_t ldg_mpmc_push(ldg_mpmc_queue_t *q, const void *item)
{
    uint64_t pos = 0;
    uint64_t seq = 0;
    uint32_t spin = 0;
    ldg_mpmc_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(!q || !item)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (spin = 0; spin < MPMC_MAX_SPIN; spin++)
    {
        pos = LDG_LOAD_ACQUIRE(q->head);
        if (LDG_UNLIKELY(pos > UINT64_MAX - q->capacity)) { return LDG_ERR_OVERFLOW; }

        if (LDG_UNLIKELY(mpmc_slot_get(q, pos & q->mask, &slot) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

        seq = LDG_LOAD_ACQUIRE(slot->seq);

        if (seq == pos) { if (LDG_CAS(&q->head, &pos, pos + 1)) { break; } }
        else if (seq < pos) { return LDG_ERR_FULL; }
        else { LDG_PAUSE; }
    }

    if (LDG_UNLIKELY(spin >= MPMC_MAX_SPIN)) { return LDG_ERR_AGAIN; }

    memcpy(slot->data, item, q->item_size);

    LDG_STORE_RELEASE(slot->seq, pos + 1);

    // signal outside lock; re-check-before-wait in mpmc_wait mitigates lost wakeup
    ldg_cond_sig(&q->wait_cond);

    return LDG_ERR_AOK;
}

uint32_t ldg_mpmc_pop(ldg_mpmc_queue_t *q, void *item_out)
{
    uint64_t pos = 0;
    uint64_t seq = 0;
    uint32_t spin = 0;
    ldg_mpmc_slot_t *slot = 0x0;

    if (LDG_UNLIKELY(!q || !item_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (spin = 0; spin < MPMC_MAX_SPIN; spin++)
    {
        pos = LDG_LOAD_ACQUIRE(q->tail);
        if (LDG_UNLIKELY(pos > UINT64_MAX - q->capacity)) { return LDG_ERR_OVERFLOW; }

        if (LDG_UNLIKELY(mpmc_slot_get(q, pos & q->mask, &slot) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

        seq = LDG_LOAD_ACQUIRE(slot->seq);

        if (seq == pos + 1) { if (LDG_CAS(&q->tail, &pos, pos + 1)) { break; } }
        else if (seq < pos + 1) { return LDG_ERR_EMPTY; }
        else { LDG_PAUSE; }
    }

    if (LDG_UNLIKELY(spin >= MPMC_MAX_SPIN)) { return LDG_ERR_AGAIN; }

    memcpy(item_out, slot->data, q->item_size);

    LDG_STORE_RELEASE(slot->seq, pos + q->capacity);

    return LDG_ERR_AOK;
}

uint32_t ldg_mpmc_wait(ldg_mpmc_queue_t *q, void *item_out, uint64_t timeout_ms)
{
    uint32_t ret = 0;
    uint32_t iter = 0;
    uint64_t deadline_ms = 0;
    uint64_t now_ms = 0;
    uint64_t remaining = 0;

    if (LDG_UNLIKELY(!q || !item_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mpmc_pop(q, item_out);
    if (ret == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(ret != LDG_ERR_EMPTY)) { return ret; }

    now_ms = mpmc_monotonic_ms_get();
    if (LDG_UNLIKELY(timeout_ms > UINT64_MAX - now_ms)) { deadline_ms = UINT64_MAX; }
    else { deadline_ms = now_ms + timeout_ms; }

    ret = ldg_mut_lock(&q->wait_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_BUSY; }

    for (iter = 0; iter < MPMC_WAIT_MAX_ITER; iter++)
    {
        ret = ldg_mpmc_pop(q, item_out);
        if (ret == LDG_ERR_AOK) { break; }

        if (LDG_UNLIKELY(ret != LDG_ERR_EMPTY)) { ldg_mut_unlock(&q->wait_mut); return ret; }

        now_ms = mpmc_monotonic_ms_get();
        if (now_ms >= deadline_ms) { ldg_mut_unlock(&q->wait_mut); return LDG_ERR_TIMEOUT; }

        remaining = deadline_ms - now_ms;

        if (ldg_cond_timedwait(&q->wait_cond, &q->wait_mut, remaining) != LDG_ERR_AOK)
        {
            ldg_mut_unlock(&q->wait_mut);
            return LDG_ERR_TIMEOUT;
        }
    }

    if (LDG_UNLIKELY(iter >= MPMC_WAIT_MAX_ITER)) { ldg_mut_unlock(&q->wait_mut); return LDG_ERR_TIMEOUT; }

    // pop succeeded; unlock failure does not revoke the committed item
    if (LDG_UNLIKELY(ldg_mut_unlock(&q->wait_mut) != LDG_ERR_AOK)) { return LDG_ERR_AOK; }

    return LDG_ERR_AOK;
}

uint64_t ldg_mpmc_cunt_get(const ldg_mpmc_queue_t *q)
{
    uint64_t head = 0;
    uint64_t tail = 0;

    if (LDG_UNLIKELY(!q)) { return UINT64_MAX; }

    head = LDG_LOAD_ACQUIRE(q->head);
    tail = LDG_LOAD_ACQUIRE(q->tail);

    if (head >= tail) { return head - tail; }

    return 0;
}

uint8_t ldg_mpmc_empty_is(const ldg_mpmc_queue_t *q)
{
    if (LDG_UNLIKELY(!q)) { return LDG_TRUTH_TRUE; }

    return LDG_LOAD_ACQUIRE(q->head) == LDG_LOAD_ACQUIRE(q->tail);
}

uint8_t ldg_mpmc_full_is(const ldg_mpmc_queue_t *q)
{
    uint64_t head = 0;
    uint64_t tail = 0;

    if (LDG_UNLIKELY(!q)) { return LDG_TRUTH_TRUE; }

    head = LDG_LOAD_ACQUIRE(q->head);
    tail = LDG_LOAD_ACQUIRE(q->tail);

    if (head < tail) { return LDG_TRUTH_FALSE; }

    return (head - tail) >= q->capacity;
}
