#include <stdlib.h>
#include <string.h>

#include <dangling/thread/mpmc.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/arch/x86_64/atomic.h>
#include <dangling/arch/x86_64/fence.h>

// slot

static ldg_mpmc_slot_t* slot_get(ldg_mpmc_queue_t *q, size_t idx)
{
    return (ldg_mpmc_slot_t *)(q->buff + (idx * q->slot_size));
}

static uint32_t capacity_pow2_is(size_t capacity)
{
    return (capacity > 0) && ((capacity & (capacity - 1)) == 0);
}

uint32_t ldg_mpmc_init(ldg_mpmc_queue_t *q, size_t item_size, size_t capacity)
{
    size_t i = 0;
    size_t slot_data_offset = 0;
    ldg_mpmc_slot_t *slot = NULL;

    if (LDG_UNLIKELY(!q || item_size == 0 || capacity == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!capacity_pow2_is(capacity))) { return LDG_ERR_FUNC_ARG_INVALID; }

    (void)memset(q, 0, sizeof(ldg_mpmc_queue_t));

    slot_data_offset = LDG_CACHE_LINE_WIDTH;
    q->slot_size = LDG_ALIGNED_UP(slot_data_offset + item_size);
    q->item_size = item_size;
    q->capacity = capacity;
    q->mask = capacity - 1;

    if (LDG_UNLIKELY(posix_memalign((void **)&q->buff, LDG_CACHE_LINE_WIDTH, q->slot_size * capacity) != 0)) { return LDG_ERR_ALLOC_NULL; }

    (void)memset(q->buff, 0, q->slot_size * capacity);

    for (i = 0; i < capacity; i++)
    {
        slot = slot_get(q, i);
        LDG_STORE_RELEASE(slot->seq, i);
    }

    q->head = 0;
    q->tail = 0;

    if (LDG_UNLIKELY(ldg_mut_init(&q->wait_mut, 0) != LDG_ERR_AOK))
    {
        free(q->buff);
        q->buff = NULL;
        return LDG_ERR_ALLOC_NULL;
    }

    if (LDG_UNLIKELY(ldg_cond_init(&q->wait_cond, 0) != LDG_ERR_AOK))
    {
        ldg_mut_destroy(&q->wait_mut);
        free(q->buff);
        q->buff = NULL;
        return LDG_ERR_ALLOC_NULL;
    }

    return LDG_ERR_AOK;
}

void ldg_mpmc_shutdown(ldg_mpmc_queue_t *q)
{
    if (LDG_UNLIKELY(!q)) { return; }

    ldg_cond_destroy(&q->wait_cond);
    ldg_mut_destroy(&q->wait_mut);

    if (q->buff)
    {
        free(q->buff);
        q->buff = NULL;
    }

    q->slot_size = 0;
    q->item_size = 0;
    q->capacity = 0;
    q->mask = 0;
    q->head = 0;
    q->tail = 0;
}

uint32_t ldg_mpmc_push(ldg_mpmc_queue_t *q, const void *item)
{
    size_t pos = 0;
    size_t seq = 0;
    int64_t diff = 0;
    ldg_mpmc_slot_t *slot = NULL;

    if (LDG_UNLIKELY(!q || !item)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (;;)
    {
        pos = LDG_LOAD_ACQUIRE(q->head);
        slot = slot_get(q, pos & q->mask);
        seq = LDG_LOAD_ACQUIRE(slot->seq);
        diff = (int64_t)seq - (int64_t)pos;

        if (diff == 0) { if (LDG_CAS(&q->head, &pos, pos + 1)) { break; } }
        else if (diff < 0) { return LDG_ERR_FULL; }
        else{ LDG_PAUSE; }
    }

    (void)memcpy(slot->data, item, q->item_size);
    LDG_STORE_RELEASE(slot->seq, pos + 1);

    ldg_cond_signal(&q->wait_cond);

    return LDG_ERR_AOK;
}

uint32_t ldg_mpmc_pop(ldg_mpmc_queue_t *q, void *item_out)
{
    size_t pos = 0;
    size_t seq = 0;
    int64_t diff = 0;
    ldg_mpmc_slot_t *slot = NULL;

    if (LDG_UNLIKELY(!q || !item_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (;;)
    {
        pos = LDG_LOAD_ACQUIRE(q->tail);
        slot = slot_get(q, pos & q->mask);
        seq = LDG_LOAD_ACQUIRE(slot->seq);
        diff = (int64_t)seq - (int64_t)(pos + 1);

        if (diff == 0) { if (LDG_CAS(&q->tail, &pos, pos + 1)) { break; } }
        else if (diff < 0) { return LDG_ERR_EMPTY; }
        else{ LDG_PAUSE; }
    }

    (void)memcpy(item_out, slot->data, q->item_size);
    LDG_STORE_RELEASE(slot->seq, pos + q->capacity);

    return LDG_ERR_AOK;
}

uint32_t ldg_mpmc_wait(ldg_mpmc_queue_t *q, void *item_out, uint64_t timeout_ms)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!q || !item_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ldg_mpmc_pop(q, item_out) == LDG_ERR_AOK) { return LDG_ERR_AOK; }

    ldg_mut_lock(&q->wait_mut);

    while ((ret = ldg_mpmc_pop(q, item_out)) != LDG_ERR_AOK) { if (ldg_cond_timedwait(&q->wait_cond, &q->wait_mut, timeout_ms) != 0)
        {
            ldg_mut_unlock(&q->wait_mut);
            return LDG_ERR_TIMEOUT;
        }
    }

    ldg_mut_unlock(&q->wait_mut);

    return LDG_ERR_AOK;
}

size_t ldg_mpmc_cunt_get(const ldg_mpmc_queue_t *q)
{
    size_t head = 0;
    size_t tail = 0;

    if (LDG_UNLIKELY(!q)) { return 0; }

    head = LDG_LOAD_ACQUIRE(q->head);
    tail = LDG_LOAD_ACQUIRE(q->tail);

    if (head >= tail) { return head - tail; }

    return 0;
}

uint32_t ldg_mpmc_empty_is(const ldg_mpmc_queue_t *q)
{
    if (LDG_UNLIKELY(!q)) { return 1; }

    return LDG_LOAD_ACQUIRE(q->head) == LDG_LOAD_ACQUIRE(q->tail);
}

uint32_t ldg_mpmc_full_is(const ldg_mpmc_queue_t *q)
{
    size_t head = 0;
    size_t tail = 0;

    if (LDG_UNLIKELY(!q)) { return 0; }

    head = LDG_LOAD_ACQUIRE(q->head);
    tail = LDG_LOAD_ACQUIRE(q->tail);

    return (head - tail) >= q->capacity;
}
