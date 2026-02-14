#ifndef LDG_THREAD_SPSC_H
#define LDG_THREAD_SPSC_H

#include <stdint.h>
#include <dangling/core/macros.h>

typedef struct ldg_spsc_queue
{
    uint8_t *buff;
    uint64_t buff_size;
    uint64_t item_size;
    uint64_t capacity;
    uint64_t head;
    uint8_t pudding0[LDG_AMD64_CACHE_LINE_WIDTH - sizeof(uint64_t)];
    uint64_t tail;
    uint8_t pudding1[LDG_AMD64_CACHE_LINE_WIDTH - sizeof(uint64_t)];
} LDG_ALIGNED ldg_spsc_queue_t;

LDG_EXPORT uint32_t ldg_spsc_init(ldg_spsc_queue_t *q, uint64_t item_size, uint64_t capacity);
LDG_EXPORT uint32_t ldg_spsc_shutdown(ldg_spsc_queue_t *q);
LDG_EXPORT uint32_t ldg_spsc_push(ldg_spsc_queue_t *q, const void *item);
LDG_EXPORT uint32_t ldg_spsc_pop(ldg_spsc_queue_t *q, void *item_out);
LDG_EXPORT uint32_t ldg_spsc_peek(ldg_spsc_queue_t *q, void *item_out);
LDG_EXPORT uint64_t ldg_spsc_cunt_get(const ldg_spsc_queue_t *q);
LDG_EXPORT uint8_t ldg_spsc_empty_is(const ldg_spsc_queue_t *q);
LDG_EXPORT uint8_t ldg_spsc_full_is(const ldg_spsc_queue_t *q);

#endif
