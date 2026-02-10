#ifndef LDG_MEM_ALLOC_H
#define LDG_MEM_ALLOC_H

#include <stdint.h>
#include <stddef.h>
#include <dangling/core/macros.h>

typedef struct ldg_mem_stats
{
    size_t bytes_allocated;
    size_t bytes_peak;
    size_t alloc_cunt;
    size_t dealloc_cunt;
    size_t pool_cunt;
    size_t active_alloc_cunt;
    uint8_t pudding[16];
} ldg_mem_stats_t;

typedef struct ldg_mem_pool ldg_mem_pool_t;

LDG_EXPORT uint32_t ldg_mem_init(void);
LDG_EXPORT void ldg_mem_shutdown(void);
LDG_EXPORT uint32_t ldg_mem_lock(void);
LDG_EXPORT uint32_t ldg_mem_locked_is(void);

LDG_EXPORT void* ldg_mem_alloc(size_t size);
LDG_EXPORT void* ldg_mem_realloc(void *ptr, size_t size);
LDG_EXPORT void ldg_mem_dealloc(void *ptr);

LDG_EXPORT ldg_mem_pool_t* ldg_mem_pool_create(size_t item_size, size_t capacity);
LDG_EXPORT void* ldg_mem_pool_alloc(ldg_mem_pool_t *pool, uint64_t size);
LDG_EXPORT void ldg_mem_pool_dealloc(ldg_mem_pool_t *pool, void *ptr);
LDG_EXPORT void ldg_mem_pool_destroy(ldg_mem_pool_t *pool);
LDG_EXPORT void ldg_mem_pool_reset(ldg_mem_pool_t *pool);
LDG_EXPORT uint64_t ldg_mem_pool_remaining_get(const ldg_mem_pool_t *pool);
LDG_EXPORT uint64_t ldg_mem_pool_used_get(const ldg_mem_pool_t *pool);
LDG_EXPORT uint64_t ldg_mem_pool_capacity_get(const ldg_mem_pool_t *pool);
LDG_EXPORT uint32_t ldg_mem_pool_var_is(const ldg_mem_pool_t *pool);

LDG_EXPORT void ldg_mem_stats_get(ldg_mem_stats_t *stats);
LDG_EXPORT void ldg_mem_leaks_dump(void);

LDG_EXPORT uint32_t ldg_mem_valid_is(const void *ptr);
LDG_EXPORT size_t ldg_mem_size_get(const void *ptr);

#endif
