#ifndef LDG_THREAD_MPMC_H
#define LDG_THREAD_MPMC_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/thread/sync.h>

typedef struct ldg_mpmc_slot
{
    uint64_t seq;
    uint8_t pudding0[LDG_AMD64_CACHE_LINE_WIDTH - sizeof(uint64_t)];
    uint8_t data[];
} ldg_mpmc_slot_t;

typedef struct ldg_mpmc_queue
{
    uint8_t *buff;
    uint64_t slot_size;
    uint64_t item_size;
    uint64_t capacity;
    uint64_t mask;
    uint64_t head;
    uint8_t pudding0[LDG_AMD64_CACHE_LINE_WIDTH - sizeof(uint64_t)];
    uint64_t tail;
    uint8_t pudding1[LDG_AMD64_CACHE_LINE_WIDTH - sizeof(uint64_t)];
    ldg_mut_t wait_mut;
    ldg_cond_t wait_cond;
} LDG_ALIGNED ldg_mpmc_queue_t;

LDG_EXPORT uint32_t ldg_mpmc_init(ldg_mpmc_queue_t *q, uint64_t item_size, uint64_t capacity);
LDG_EXPORT uint32_t ldg_mpmc_shutdown(ldg_mpmc_queue_t *q);
LDG_EXPORT uint32_t ldg_mpmc_push(ldg_mpmc_queue_t *q, const void *item);
LDG_EXPORT uint32_t ldg_mpmc_pop(ldg_mpmc_queue_t *q, void *item_out);
LDG_EXPORT uint32_t ldg_mpmc_wait(ldg_mpmc_queue_t *q, void *item_out, uint64_t timeout_ms);
LDG_EXPORT uint64_t ldg_mpmc_cunt_get(const ldg_mpmc_queue_t *q);
LDG_EXPORT uint8_t ldg_mpmc_empty_is(const ldg_mpmc_queue_t *q);
LDG_EXPORT uint8_t ldg_mpmc_full_is(const ldg_mpmc_queue_t *q);

#endif
