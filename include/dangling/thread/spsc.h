#ifndef LDG_THREAD_SPSC_H
#define LDG_THREAD_SPSC_H

#include <stdint.h>
#include <stddef.h>
#include <dangling/core/macros.h>

typedef struct ldg_spsc_queue
{
    uint8_t *buff;
    size_t buff_size;
    size_t item_size;
    size_t capacity;
    size_t head;
    uint8_t pudding0[LDG_CACHE_LINE_WIDTH - sizeof(size_t)];
    size_t tail;
    uint8_t pudding1[LDG_CACHE_LINE_WIDTH - sizeof(size_t)];
} LDG_ALIGNED ldg_spsc_queue_t;

LDG_EXPORT int32_t ldg_spsc_init(ldg_spsc_queue_t *q, size_t item_size, size_t capacity);
LDG_EXPORT void ldg_spsc_shutdown(ldg_spsc_queue_t *q);
LDG_EXPORT int32_t ldg_spsc_push(ldg_spsc_queue_t *q, const void *item);
LDG_EXPORT int32_t ldg_spsc_pop(ldg_spsc_queue_t *q, void *item_out);
LDG_EXPORT int32_t ldg_spsc_peek(const ldg_spsc_queue_t *q, void *item_out);
LDG_EXPORT size_t ldg_spsc_cunt(const ldg_spsc_queue_t *q);
LDG_EXPORT int32_t ldg_spsc_empty(const ldg_spsc_queue_t *q);
LDG_EXPORT int32_t ldg_spsc_full(const ldg_spsc_queue_t *q);

#endif
