#ifndef LDG_MEM_ALLOC_H
#define LDG_MEM_ALLOC_H

#include <stdint.h>
#include <dangling/core/macros.h>

typedef struct ldg_mem_stats
{
    uint64_t bytes_alloc;
    uint64_t bytes_peak;
    uint64_t alloc_cunt;
    uint64_t dealloc_cunt;
    uint64_t pool_cunt;
    uint64_t active_alloc_cunt;
    uint8_t pudding[16];
} ldg_mem_stats_t;

typedef struct ldg_mem_pool ldg_mem_pool_t;

LDG_EXPORT uint32_t ldg_mem_init(void);
LDG_EXPORT uint32_t ldg_mem_shutdown(void);
LDG_EXPORT uint32_t ldg_mem_lock(void);
LDG_EXPORT uint8_t ldg_mem_locked_is(void);

LDG_EXPORT uint32_t ldg_mem_alloc(uint64_t size, void **out);
LDG_EXPORT uint32_t ldg_mem_realloc(void *ptr, uint64_t size, void **out);
LDG_EXPORT uint32_t ldg_mem_dealloc(void *ptr);

LDG_EXPORT uint32_t ldg_mem_pool_create(uint64_t item_size, uint64_t capacity, ldg_mem_pool_t **out);
LDG_EXPORT uint32_t ldg_mem_pool_alloc(ldg_mem_pool_t *pool, uint64_t size, void **out);
LDG_EXPORT uint32_t ldg_mem_pool_dealloc(ldg_mem_pool_t *pool, void *ptr);
LDG_EXPORT uint32_t ldg_mem_pool_destroy(ldg_mem_pool_t **pool);
LDG_EXPORT uint32_t ldg_mem_pool_rst(ldg_mem_pool_t *pool);
LDG_EXPORT uint64_t ldg_mem_pool_remaining_get(ldg_mem_pool_t *pool);
LDG_EXPORT uint64_t ldg_mem_pool_used_get(ldg_mem_pool_t *pool);
LDG_EXPORT uint64_t ldg_mem_pool_capacity_get(ldg_mem_pool_t *pool);
LDG_EXPORT uint8_t ldg_mem_pool_var_is(ldg_mem_pool_t *pool);

LDG_EXPORT uint32_t ldg_mem_stats_get(ldg_mem_stats_t *stats);
LDG_EXPORT uint32_t ldg_mem_leaks_dump(void);

LDG_EXPORT uint8_t ldg_mem_valid_is(const void *ptr);
LDG_EXPORT uint64_t ldg_mem_size_get(const void *ptr);

#endif
