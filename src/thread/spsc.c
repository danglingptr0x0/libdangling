#include <dangling/thread/spsc.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

uint32_t ldg_spsc_init(ldg_spsc_queue_t *q, size_t item_size, size_t capacity)
{
    if (LDG_UNLIKELY(!q || item_size == 0 || capacity == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    (void)memset(q, 0, sizeof(ldg_spsc_queue_t));

    q->buff_size = item_size * capacity;
    q->item_size = item_size;
    q->capacity = capacity;

    if (LDG_UNLIKELY(posix_memalign((void **)&q->buff, LDG_CACHE_LINE_WIDTH, q->buff_size) != 0)) { return LDG_ERR_ALLOC_NULL; }

    (void)memset(q->buff, 0, q->buff_size);

    return LDG_ERR_AOK;
}

void ldg_spsc_shutdown(ldg_spsc_queue_t *q)
{
    if (LDG_UNLIKELY(!q)) { return; }

    if (q->buff)
    {
        free(q->buff);
        q->buff = NULL;
    }

    q->buff_size = 0;
    q->item_size = 0;
    q->capacity = 0;
    q->head = 0;
    q->tail = 0;
}

uint32_t ldg_spsc_push(ldg_spsc_queue_t *q, const void *item)
{
    size_t head = 0;
    size_t next_head = 0;

    if (LDG_UNLIKELY(!q || !item)) { return LDG_ERR_FUNC_ARG_NULL; }

    head = q->head;
    next_head = (head + 1) % q->capacity;

    atomic_thread_fence(memory_order_acquire);
    if (next_head == q->tail) { return LDG_ERR_FULL; }

    (void)memcpy(q->buff + (head * q->item_size), item, q->item_size);

    atomic_thread_fence(memory_order_release);
    q->head = next_head;

    return LDG_ERR_AOK;
}

uint32_t ldg_spsc_pop(ldg_spsc_queue_t *q, void *item_out)
{
    size_t tail = 0;

    if (LDG_UNLIKELY(!q || !item_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    tail = q->tail;

    atomic_thread_fence(memory_order_acquire);
    if (tail == q->head) { return LDG_ERR_EMPTY; }

    (void)memcpy(item_out, q->buff + (tail * q->item_size), q->item_size);

    atomic_thread_fence(memory_order_release);
    q->tail = (tail + 1) % q->capacity;

    return LDG_ERR_AOK;
}

uint32_t ldg_spsc_peek(const ldg_spsc_queue_t *q, void *item_out)
{
    size_t tail = 0;

    if (LDG_UNLIKELY(!q || !item_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    tail = q->tail;

    atomic_thread_fence(memory_order_acquire);
    if (tail == q->head) { return LDG_ERR_EMPTY; }

    (void)memcpy(item_out, q->buff + (tail * q->item_size), q->item_size);

    return LDG_ERR_AOK;
}

size_t ldg_spsc_cunt_get(const ldg_spsc_queue_t *q)
{
    size_t head = 0;
    size_t tail = 0;

    if (LDG_UNLIKELY(!q)) { return 0; }

    atomic_thread_fence(memory_order_acquire);
    head = q->head;
    tail = q->tail;

    if (head >= tail) { return head - tail; }

    return q->capacity - tail + head;
}

uint32_t ldg_spsc_empty_is(const ldg_spsc_queue_t *q)
{
    if (LDG_UNLIKELY(!q)) { return 1; }

    atomic_thread_fence(memory_order_acquire);
    return q->head == q->tail;
}

uint32_t ldg_spsc_full_is(const ldg_spsc_queue_t *q)
{
    size_t next_head = 0;

    if (LDG_UNLIKELY(!q)) { return 0; }

    next_head = (q->head + 1) % q->capacity;

    atomic_thread_fence(memory_order_acquire);
    return next_head == q->tail;
}
