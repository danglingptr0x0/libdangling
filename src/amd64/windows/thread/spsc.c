#include <string.h>

#include <dangling/thread/spsc.h>
#include <dangling/core/err.h>
#include <dangling/mem/secure.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/macros.h>
#include <dangling/core/arith.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/arch/amd64/fence.h>

uint32_t ldg_spsc_init(ldg_spsc_queue_t *q, uint64_t item_size, uint64_t cap)
{
    void *buff_tmp = 0x0;
    uint64_t buff_size = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!q)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(item_size == 0 || cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(q, 0, sizeof(ldg_spsc_queue_t)) != q)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(ldg_arith_64_mul(item_size, cap, &buff_size) != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    q->buff_size = buff_size;
    q->item_size = item_size;
    q->cap = cap;

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
    q->cap = 0;
    q->hd = 0;
    q->tail = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_spsc_push(ldg_spsc_queue_t *q, const void *item)
{
    uint64_t hd = 0;
    uint64_t next_hd = 0;
    void *dst = 0x0;

    if (LDG_UNLIKELY(!q || !item)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(q->cap == 0 || !q->buff)) { return LDG_ERR_NOT_INIT; }

    hd = LDG_LOAD_ACQUIRE(q->hd);
    next_hd = (hd + 1) % q->cap;

    if (next_hd == LDG_LOAD_ACQUIRE(q->tail)) { return LDG_ERR_FULL; }

    if (LDG_UNLIKELY(hd >= q->cap || hd * q->item_size + q->item_size > q->buff_size)) { return LDG_ERR_BOUNDS; }

    dst = q->buff + (hd * q->item_size);
    if (LDG_UNLIKELY(ldg_mem_secure_copy(dst, item, q->item_size) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    LDG_STORE_RELEASE(q->hd, next_hd);

    return LDG_ERR_AOK;
}

uint32_t ldg_spsc_pop(ldg_spsc_queue_t *q, void *item_out)
{
    uint64_t tail = 0;
    void *src = 0x0;

    if (LDG_UNLIKELY(!q || !item_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(q->cap == 0 || !q->buff)) { return LDG_ERR_NOT_INIT; }

    tail = LDG_LOAD_ACQUIRE(q->tail);

    if (tail == LDG_LOAD_ACQUIRE(q->hd)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(tail >= q->cap || tail * q->item_size + q->item_size > q->buff_size)) { return LDG_ERR_BOUNDS; }

    src = q->buff + (tail * q->item_size);
    if (LDG_UNLIKELY(ldg_mem_secure_copy(item_out, src, q->item_size) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    LDG_STORE_RELEASE(q->tail, (tail + 1) % q->cap);

    return LDG_ERR_AOK;
}

uint32_t ldg_spsc_peek(ldg_spsc_queue_t *q, void *item_out)
{
    uint64_t tail = 0;
    void *src = 0x0;

    if (LDG_UNLIKELY(!q || !item_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(q->cap == 0 || !q->buff)) { return LDG_ERR_NOT_INIT; }

    tail = LDG_LOAD_ACQUIRE(q->tail);

    if (tail == LDG_LOAD_ACQUIRE(q->hd)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(tail >= q->cap || tail * q->item_size + q->item_size > q->buff_size)) { return LDG_ERR_BOUNDS; }

    src = (void *)(q->buff + (tail * q->item_size));
    if (LDG_UNLIKELY(ldg_mem_secure_copy(item_out, src, q->item_size) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    return LDG_ERR_AOK;
}

uint64_t ldg_spsc_cunt_get(const ldg_spsc_queue_t *q)
{
    uint64_t hd = 0;
    uint64_t tail = 0;

    if (LDG_UNLIKELY(!q)) { return UINT64_MAX; }

    hd = LDG_LOAD_ACQUIRE(q->hd);
    tail = LDG_LOAD_ACQUIRE(q->tail);

    if (hd >= tail) { return hd - tail; }

    return q->cap - tail + hd;
}

uint8_t ldg_spsc_empty_is(const ldg_spsc_queue_t *q)
{
    if (LDG_UNLIKELY(!q)) { return LDG_TRUTH_TRUE; }

    return LDG_LOAD_ACQUIRE(q->hd) == LDG_LOAD_ACQUIRE(q->tail);
}

uint8_t ldg_spsc_full_is(const ldg_spsc_queue_t *q)
{
    uint64_t next_hd = 0;

    if (LDG_UNLIKELY(!q || q->cap == 0)) { return LDG_TRUTH_TRUE; }

    next_hd = (LDG_LOAD_ACQUIRE(q->hd) + 1) % q->cap;

    return next_hd == LDG_LOAD_ACQUIRE(q->tail);
}
