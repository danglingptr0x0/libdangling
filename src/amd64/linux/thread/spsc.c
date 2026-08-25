#include <string.h>

#include <dangling/thread/spsc.h>
#include <dangling/core/err.h>
#include <dangling/mem/secure.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/macros.h>
#include <dangling/core/arith.h>
#include <dangling/arch/amd64/atomic.h>
#include <dangling/arch/amd64/fence.h>

uint32_t ldg_spsc_init(ldg_spsc_queue_t *queue, uint64_t item_size, uint64_t cap)
{
    void *buff_tmp = 0x0;
    uint64_t buff_size = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!queue)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(item_size == 0 || cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(memset(queue, 0, sizeof(ldg_spsc_queue_t)) != queue)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(ldg_arith_uint64_mul(item_size, cap, &buff_size) != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    queue->buff_size = buff_size;
    queue->item_size = item_size;
    queue->cap = cap;

    ret = ldg_mem_alloc(queue->buff_size, &buff_tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    queue->buff = (uint8_t *)buff_tmp;

    if (LDG_UNLIKELY(memset(queue->buff, 0, queue->buff_size) != queue->buff)) { ldg_mem_dealloc(queue->buff); queue->buff = 0x0; return LDG_ERR_MEM_BAD; }

    return LDG_ERR_AOK;
}

uint32_t ldg_spsc_shutdown(ldg_spsc_queue_t *queue)
{
    if (LDG_UNLIKELY(!queue)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (queue->buff)
    {
        ldg_mem_dealloc(queue->buff);
        queue->buff = 0x0;
    }

    queue->buff_size = 0;
    queue->item_size = 0;
    queue->cap = 0;
    queue->hd = 0;
    queue->tail = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_spsc_push(ldg_spsc_queue_t *queue, const void *item)
{
    uint64_t hd = 0;
    uint64_t next_hd = 0;
    void *dst = 0x0;

    if (LDG_UNLIKELY(!queue || !item)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(queue->cap == 0 || !queue->buff)) { return LDG_ERR_NOT_INIT; }

    hd = LDG_LD_ACQUIRE(queue->hd);
    next_hd = (hd + 1) % queue->cap;

    if (next_hd == LDG_LD_ACQUIRE(queue->tail)) { return LDG_ERR_FULL; }

    if (LDG_UNLIKELY(hd >= queue->cap || hd * queue->item_size + queue->item_size > queue->buff_size)) { return LDG_ERR_BOUNDS; }

    dst = queue->buff + (hd * queue->item_size);
    if (LDG_UNLIKELY(ldg_mem_secure_cpy(dst, item, queue->item_size) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    LDG_ST_RELEASE(queue->hd, next_hd);

    return LDG_ERR_AOK;
}

uint32_t ldg_spsc_pop(ldg_spsc_queue_t *queue, void *item_out)
{
    uint64_t tail = 0;
    void *src = 0x0;

    if (LDG_UNLIKELY(!queue || !item_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(queue->cap == 0 || !queue->buff)) { return LDG_ERR_NOT_INIT; }

    tail = LDG_LD_ACQUIRE(queue->tail);

    if (tail == LDG_LD_ACQUIRE(queue->hd)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(tail >= queue->cap || tail * queue->item_size + queue->item_size > queue->buff_size)) { return LDG_ERR_BOUNDS; }

    src = queue->buff + (tail * queue->item_size);
    if (LDG_UNLIKELY(ldg_mem_secure_cpy(item_out, src, queue->item_size) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    LDG_ST_RELEASE(queue->tail, (tail + 1) % queue->cap);

    return LDG_ERR_AOK;
}

uint32_t ldg_spsc_peek(ldg_spsc_queue_t *queue, void *item_out)
{
    uint64_t tail = 0;
    void *src = 0x0;

    if (LDG_UNLIKELY(!queue || !item_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(queue->cap == 0 || !queue->buff)) { return LDG_ERR_NOT_INIT; }

    tail = LDG_LD_ACQUIRE(queue->tail);

    if (tail == LDG_LD_ACQUIRE(queue->hd)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(tail >= queue->cap || tail * queue->item_size + queue->item_size > queue->buff_size)) { return LDG_ERR_BOUNDS; }

    src = (void *)(queue->buff + (tail * queue->item_size));
    if (LDG_UNLIKELY(ldg_mem_secure_cpy(item_out, src, queue->item_size) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    return LDG_ERR_AOK;
}

uint64_t ldg_spsc_cunt_get(const ldg_spsc_queue_t *queue)
{
    uint64_t hd = 0;
    uint64_t tail = 0;

    if (LDG_UNLIKELY(!queue)) { return UINT64_MAX; }

    hd = LDG_LD_ACQUIRE(queue->hd);
    tail = LDG_LD_ACQUIRE(queue->tail);

    if (hd >= tail) { return hd - tail; }

    return queue->cap - tail + hd;
}

uint8_t ldg_spsc_empty_is(const ldg_spsc_queue_t *queue)
{
    if (LDG_UNLIKELY(!queue)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_LD_ACQUIRE(queue->hd) == LDG_LD_ACQUIRE(queue->tail)) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint8_t ldg_spsc_full_is(const ldg_spsc_queue_t *queue)
{
    uint64_t next_hd = 0;

    if (LDG_UNLIKELY(!queue)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(queue->cap == 0)) { return LDG_ERR_NOT_INIT; }

    next_hd = (LDG_LD_ACQUIRE(queue->hd) + 1) % queue->cap;

    if (next_hd == LDG_LD_ACQUIRE(queue->tail)) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}
