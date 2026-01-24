#ifndef LDG_THREAD_MPMC_H
#define LDG_THREAD_MPMC_H

#include <stdint.h>
#include <stddef.h>
#include <dangling/core/macros.h>
#include <dangling/thread/sync.h>

typedef struct ldg_mpmc_slot
{
    size_t seq;
    uint8_t pudding0[LDG_CACHE_LINE_WIDTH - sizeof(size_t)];
    uint8_t data[];
} ldg_mpmc_slot_t;

typedef struct ldg_mpmc_queue
{
    uint8_t *buff;
    size_t slot_size;
    size_t item_size;
    size_t capacity;
    size_t mask;
    size_t head;
    uint8_t pudding0[LDG_CACHE_LINE_WIDTH - sizeof(size_t)];
    size_t tail;
    uint8_t pudding1[LDG_CACHE_LINE_WIDTH - sizeof(size_t)];
    ldg_mut_t wait_mut;
    ldg_cond_t wait_cond;
} LDG_ALIGNED ldg_mpmc_queue_t;

LDG_EXPORT int32_t ldg_mpmc_init(ldg_mpmc_queue_t *q, size_t item_size, size_t capacity);
LDG_EXPORT void ldg_mpmc_shutdown(ldg_mpmc_queue_t *q);
LDG_EXPORT int32_t ldg_mpmc_push(ldg_mpmc_queue_t *q, const void *item);
LDG_EXPORT int32_t ldg_mpmc_pop(ldg_mpmc_queue_t *q, void *item_out);
LDG_EXPORT int32_t ldg_mpmc_wait(ldg_mpmc_queue_t *q, void *item_out, uint64_t timeout_ms);
LDG_EXPORT size_t ldg_mpmc_cunt_get(const ldg_mpmc_queue_t *q);
LDG_EXPORT int32_t ldg_mpmc_empty_is(const ldg_mpmc_queue_t *q);
LDG_EXPORT int32_t ldg_mpmc_full_is(const ldg_mpmc_queue_t *q);

#endif
