#include <string.h>

#include <dangling/thread/spsc.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/macros.h>
#include <dangling/core/arith.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/arch/amd64/fence.h>

uint32_t ldg_spsc_init(ldg_spsc_queue_t *q, uint64_t item_size, uint64_t capacity)
{
    void *buff_tmp = 0x0;
    uint64_t buff_size = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!q)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(item_size == 0 || capacity == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(q, 0, sizeof(ldg_spsc_queue_t)) != q)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(ldg_arith_64_mul(item_size, capacity, &buff_size) != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    q->buff_size = buff_size;
    q->item_size = item_size;
    q->capacity = capacity;

    ret = ldg_mem_alloc(q->buff_size, &buff_tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    q->buff = (uint8_t *)buff_tmp;

    if (LDG_UNLIKELY(memset(q->buff, 0, q->buff_size) != q->buff)) { ldg_mem_dealloc(q->buff); q->buff = 0x0; return LDG_ERR_MEM_BAD; }

    return LDG_ERR_AOK;
}

uint32_t ldg_spsc_shutdown(ldg_spsc_queue_t *q)
{
    if (LDG_UNLIKELY(!q)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (q->buff)
    {
        ldg_mem_dealloc(q->buff);
        q->buff = 0x0;
    }

    q->buff_size = 0;
    q->item_size = 0;
    q->capacity = 0;
    q->head = 0;
    q->tail = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_spsc_push(ldg_spsc_queue_t *q, const void *item)
{
    uint64_t head = 0;
    uint64_t next_head = 0;
    void *dst = 0x0;

    if (LDG_UNLIKELY(!q || !item)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(q->capacity == 0 || !q->buff)) { return LDG_ERR_NOT_INIT; }

    head = LDG_LOAD_ACQUIRE(q->head);
    next_head = (head + 1) % q->capacity;

    if (next_head == LDG_LOAD_ACQUIRE(q->tail)) { return LDG_ERR_FULL; }

    if (LDG_UNLIKELY(head >= q->capacity || head * q->item_size + q->item_size > q->buff_size)) { return LDG_ERR_BOUNDS; }

    dst = q->buff + (head * q->item_size);
    memcpy(dst, item, q->item_size);

    LDG_STORE_RELEASE(q->head, next_head);

    return LDG_ERR_AOK;
}

uint32_t ldg_spsc_pop(ldg_spsc_queue_t *q, void *item_out)
{
    uint64_t tail = 0;
    void *src = 0x0;

    if (LDG_UNLIKELY(!q || !item_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(q->capacity == 0 || !q->buff)) { return LDG_ERR_NOT_INIT; }

    tail = LDG_LOAD_ACQUIRE(q->tail);

    if (tail == LDG_LOAD_ACQUIRE(q->head)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(tail >= q->capacity || tail * q->item_size + q->item_size > q->buff_size)) { return LDG_ERR_BOUNDS; }

    src = q->buff + (tail * q->item_size);
    memcpy(item_out, src, q->item_size);

    LDG_STORE_RELEASE(q->tail, (tail + 1) % q->capacity);

    return LDG_ERR_AOK;
}

uint32_t ldg_spsc_peek(ldg_spsc_queue_t *q, void *item_out)
{
    uint64_t tail = 0;
    void *src = 0x0;

    if (LDG_UNLIKELY(!q || !item_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(q->capacity == 0 || !q->buff)) { return LDG_ERR_NOT_INIT; }

    tail = LDG_LOAD_ACQUIRE(q->tail);

    if (tail == LDG_LOAD_ACQUIRE(q->head)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(tail >= q->capacity || tail * q->item_size + q->item_size > q->buff_size)) { return LDG_ERR_BOUNDS; }

    src = (void *)(q->buff + (tail * q->item_size));
    memcpy(item_out, src, q->item_size);

    return LDG_ERR_AOK;
}

uint64_t ldg_spsc_cunt_get(const ldg_spsc_queue_t *q)
{
    uint64_t head = 0;
    uint64_t tail = 0;

    if (LDG_UNLIKELY(!q)) { return UINT64_MAX; }

    head = LDG_LOAD_ACQUIRE(q->head);
    tail = LDG_LOAD_ACQUIRE(q->tail);

    if (head >= tail) { return head - tail; }

    return q->capacity - tail + head;
}

uint8_t ldg_spsc_empty_is(const ldg_spsc_queue_t *q)
{
    if (LDG_UNLIKELY(!q)) { return LDG_TRUTH_TRUE; }

    return LDG_LOAD_ACQUIRE(q->head) == LDG_LOAD_ACQUIRE(q->tail);
}

uint8_t ldg_spsc_full_is(const ldg_spsc_queue_t *q)
{
    uint64_t next_head = 0;

    if (LDG_UNLIKELY(!q || q->capacity == 0)) { return LDG_TRUTH_TRUE; }

    next_head = (LDG_LOAD_ACQUIRE(q->head) + 1) % q->capacity;

    return next_head == LDG_LOAD_ACQUIRE(q->tail);
}
