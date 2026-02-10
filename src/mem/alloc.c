#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>
#include <dangling/core/err.h>
#include <dangling/log/log.h>

#include <stdlib.h>
#include <string.h>

typedef struct ldg_mem_hdr
{
    uint32_t sentinel_front;
    uint32_t size;
    struct ldg_mem_hdr *next;
    struct ldg_mem_hdr *prev;
    uint8_t pudding[32];
} LDG_ALIGNED ldg_mem_hdr_t;

struct ldg_mem_pool
{
    uint8_t *buff;
    uint8_t *free_list;
    size_t item_size;
    size_t capacity;
    size_t alloc_cunt;
    uint64_t offset;
    uint8_t is_var;
    uint8_t pudding[15];
};

typedef struct ldg_mem_state
{
    ldg_mem_hdr_t *alloc_list;
    ldg_mem_stats_t stats;
    uint8_t is_init;
    uint8_t is_locked;
    uint8_t pudding[54];
} LDG_ALIGNED ldg_mem_state_t;

static ldg_mem_state_t g_mem = LDG_STRUCT_ZERO_INIT;


uint32_t ldg_mem_init(void)
{
    if (g_mem.is_init) { return LDG_ERR_AOK; }

    if (memset(&g_mem, 0, sizeof(ldg_mem_state_t)) != &g_mem) { return LDG_ERR_MEM_BAD; }

    g_mem.is_init = 1;

    return LDG_ERR_AOK;
}

void ldg_mem_shutdown(void)
{
    if (LDG_UNLIKELY(!g_mem.is_init)) { return; }

    ldg_mem_leaks_dump();
    if (memset(&g_mem, 0, sizeof(ldg_mem_state_t)) != &g_mem) { LDG_LOG_ERROR("ldg_mem: memset failed in shutdown; ptr: %p", (void *)&g_mem); }
}

uint32_t ldg_mem_lock(void)
{
    if (LDG_UNLIKELY(!g_mem.is_init)) { return LDG_ERR_NOT_INIT; }

    g_mem.is_locked = 1;

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_locked_is(void)
{
    return g_mem.is_locked;
}

void* ldg_mem_alloc(size_t size)
{
    ldg_mem_hdr_t *hdr = 0x0;
    uint8_t *raw = 0x0;
    uint8_t *user_ptr = 0x0;
    uint32_t *sentinel_back = 0x0;
    size_t total_size = 0;

    if (LDG_UNLIKELY(size == 0)) { return 0x0; }

    if (LDG_UNLIKELY(!g_mem.is_init)) { return 0x0; }

    if (LDG_UNLIKELY(g_mem.is_locked))
    {
        LDG_LOG_ERROR("ldg_mem: alloc after lock; size: %zu", size);
        return 0x0;
    }

    total_size = sizeof(ldg_mem_hdr_t) + size + sizeof(uint32_t);
    total_size = (total_size + LDG_AMD64_CACHE_LINE_WIDTH - 1) & ~((size_t)LDG_AMD64_CACHE_LINE_WIDTH - 1);

    if (LDG_UNLIKELY(posix_memalign((void **)&raw, LDG_AMD64_CACHE_LINE_WIDTH, total_size) != 0)) { return 0x0; }

    hdr = (ldg_mem_hdr_t *)(void *)raw;
    user_ptr = raw + sizeof(ldg_mem_hdr_t);
    sentinel_back = (uint32_t *)(void *)(user_ptr + size);

    if (memset(user_ptr, 0, size) != user_ptr) { free(raw); return 0x0; }

    hdr->sentinel_front = LDG_MEM_SENTINEL;
    hdr->size = (uint32_t)size;
    hdr->next = g_mem.alloc_list;
    hdr->prev = 0x0;
    *sentinel_back = LDG_MEM_SENTINEL;

    if (g_mem.alloc_list) { g_mem.alloc_list->prev = hdr; }

    g_mem.alloc_list = hdr;

    g_mem.stats.bytes_allocated += size;
    g_mem.stats.alloc_cunt++;
    g_mem.stats.active_alloc_cunt++;

    if (g_mem.stats.bytes_allocated > g_mem.stats.bytes_peak) { g_mem.stats.bytes_peak = g_mem.stats.bytes_allocated; }

    return user_ptr;
}

static ldg_mem_hdr_t* mem_hdr_find(const void *ptr)
{
    ldg_mem_hdr_t *hdr = 0x0;

    if (LDG_UNLIKELY(!ptr)) { return 0x0; }

    hdr = (ldg_mem_hdr_t *)(void *)((uint8_t *)(uintptr_t)ptr - sizeof(ldg_mem_hdr_t));

    if (hdr->sentinel_front != LDG_MEM_SENTINEL)
    {
        LDG_LOG_ERROR("ldg_mem: front sentinel corrupted; ptr: %p", ptr);
        return 0x0;
    }

    return hdr;
}

static uint32_t mem_sentinel_back_check(ldg_mem_hdr_t *hdr)
{
    uint8_t *user_ptr = 0x0;
    uint32_t *sentinel_back = 0x0;

    if (LDG_UNLIKELY(!hdr)) { return LDG_ERR_FUNC_ARG_NULL; }

    user_ptr = (uint8_t *)(void *)hdr + sizeof(ldg_mem_hdr_t);
    sentinel_back = (uint32_t *)(void *)(user_ptr + hdr->size);

    if (*sentinel_back != LDG_MEM_SENTINEL)
    {
        LDG_LOG_ERROR("ldg_mem: back sentinel corrupted; ptr: %p; size: %u", user_ptr, hdr->size);
        return LDG_ERR_MEM_BAD;
    }

    return LDG_ERR_AOK;
}

void* ldg_mem_realloc(void *ptr, size_t size)
{
    ldg_mem_hdr_t *hdr = 0x0;
    void *new_ptr = 0x0;
    size_t copy_size = 0;

    if (LDG_UNLIKELY(!ptr)) { return ldg_mem_alloc(size); }

    if (size == 0)
    {
        ldg_mem_dealloc(ptr);
        return 0x0;
    }

    hdr = mem_hdr_find(ptr);
    if (LDG_UNLIKELY(!hdr)) { return 0x0; }

    if (LDG_UNLIKELY(mem_sentinel_back_check(hdr) != LDG_ERR_AOK)) { return 0x0; }

    new_ptr = ldg_mem_alloc(size);
    if (LDG_UNLIKELY(!new_ptr)) { return 0x0; }

    copy_size = (hdr->size < size) ? hdr->size : size;
    if (memcpy(new_ptr, ptr, copy_size) != new_ptr) { ldg_mem_dealloc(new_ptr); return 0x0; }

    ldg_mem_dealloc(ptr);

    return new_ptr;
}

void ldg_mem_dealloc(void *ptr)
{
    ldg_mem_hdr_t *hdr = 0x0;
    uint64_t *poison_ptr = 0x0;
    size_t poison_cunt = 0;
    size_t i = 0;

    if (LDG_UNLIKELY(!ptr)) { return; }

    if (LDG_UNLIKELY(!g_mem.is_init)) { return; }

    hdr = mem_hdr_find(ptr);
    if (LDG_UNLIKELY(!hdr)) { return; }

    if (LDG_UNLIKELY(mem_sentinel_back_check(hdr) != LDG_ERR_AOK)) { return; }

    if (hdr->prev) { hdr->prev->next = hdr->next; }
    else { g_mem.alloc_list = hdr->next; }

    if (hdr->next) { hdr->next->prev = hdr->prev; }

    g_mem.stats.bytes_allocated -= hdr->size;
    g_mem.stats.dealloc_cunt++;
    g_mem.stats.active_alloc_cunt--;

    poison_ptr = (uint64_t *)ptr;
    poison_cunt = hdr->size / sizeof(uint64_t);
    for (i = 0; i < poison_cunt; i++) { poison_ptr[i] = LDG_MEM_POISON; }

    hdr->sentinel_front = 0;

    free(hdr);
}

ldg_mem_pool_t* ldg_mem_pool_create(size_t item_size, size_t capacity)
{
    ldg_mem_pool_t *pool = 0x0;
    size_t aligned_item_size = 0;
    size_t i = 0;
    uint8_t *item = 0x0;

    if (LDG_UNLIKELY(capacity == 0)) { return 0x0; }

    pool = ldg_mem_alloc(sizeof(ldg_mem_pool_t));
    if (LDG_UNLIKELY(!pool)) { return 0x0; }

    if (item_size == 0)
    {
        pool->buff = ldg_mem_alloc(capacity);
        if (LDG_UNLIKELY(!pool->buff))
        {
            ldg_mem_dealloc(pool);
            return 0x0;
        }

        pool->free_list = 0x0;
        pool->item_size = 0;
        pool->capacity = capacity;
        pool->alloc_cunt = 0;
        pool->offset = 0;
        pool->is_var = 1;

        g_mem.stats.pool_cunt++;

        return pool;
    }

    aligned_item_size = (item_size + sizeof(void *) + LDG_AMD64_CACHE_LINE_WIDTH - 1) & ~((size_t)LDG_AMD64_CACHE_LINE_WIDTH - 1);

    pool->buff = ldg_mem_alloc(aligned_item_size * capacity);
    if (LDG_UNLIKELY(!pool->buff))
    {
        ldg_mem_dealloc(pool);
        return 0x0;
    }

    pool->item_size = aligned_item_size;
    pool->capacity = capacity;
    pool->alloc_cunt = 0;
    pool->offset = item_size;
    pool->is_var = 0;

    pool->free_list = pool->buff;
    for (i = 0; i < capacity - 1; i++)
    {
        item = pool->buff + (i * aligned_item_size);
        *(uint8_t **)(void *)(item) = pool->buff + ((i + 1) * aligned_item_size);
    }
    item = pool->buff + ((capacity - 1) * aligned_item_size);
    *(uint8_t **)(void *)(item) = 0x0;

    g_mem.stats.pool_cunt++;

    return pool;
}

void* ldg_mem_pool_alloc(ldg_mem_pool_t *pool, uint64_t size)
{
    uint8_t *item = 0x0;
    uint64_t aligned = 0;

    if (LDG_UNLIKELY(!pool)) { return 0x0; }

    if (pool->is_var)
    {
        if (LDG_UNLIKELY(size == 0)) { return 0x0; }

        aligned = (pool->offset + LDG_MEM_POOL_VAR_ALIGN - 1) & ~((uint64_t)LDG_MEM_POOL_VAR_ALIGN - 1);
        if (LDG_UNLIKELY(aligned < pool->offset)) { return 0x0; }

        if (LDG_UNLIKELY(aligned + size < aligned)) { return 0x0; }

        if (LDG_UNLIKELY(aligned + size > pool->capacity)) { return 0x0; }

        item = pool->buff + aligned;
        if (memset(item, 0, size) != item) { return 0x0; }

        pool->offset = aligned + size;
        pool->alloc_cunt++;

        return item;
    }

    if (LDG_UNLIKELY(size != pool->offset)) { return 0x0; }

    if (LDG_UNLIKELY(!pool->free_list)) { return 0x0; }

    item = pool->free_list;
    pool->free_list = *(uint8_t **)(void *)(item);
    pool->alloc_cunt++;

    if (memset(item, 0, pool->item_size) != item) { return 0x0; }

    return item;
}

void ldg_mem_pool_dealloc(ldg_mem_pool_t *pool, void *ptr)
{
    uint8_t *item = 0x0;

    if (LDG_UNLIKELY(!pool || !ptr)) { return; }

    if (LDG_UNLIKELY(pool->is_var))
    {
        LDG_LOG_WARNING("ldg_mem_pool: dealloc on variable pool; use pool_reset; ptr: %p", ptr);
        return;
    }

    item = (uint8_t *)ptr;

    if (LDG_UNLIKELY(item < pool->buff || item >= pool->buff + (pool->item_size * pool->capacity)))
    {
        LDG_LOG_ERROR("ldg_mem_pool: ptr not in pool; ptr: %p", ptr);
        return;
    }

    *(uint8_t **)(void *)(item) = pool->free_list;
    pool->free_list = item;
    pool->alloc_cunt--;
}

void ldg_mem_pool_destroy(ldg_mem_pool_t *pool)
{
    if (LDG_UNLIKELY(!pool)) { return; }

    if (LDG_UNLIKELY(pool->alloc_cunt > 0 && !pool->is_var)) { LDG_LOG_WARNING("ldg_mem_pool: destroy with active allocs; cunt: %zu", pool->alloc_cunt); }

    ldg_mem_dealloc(pool->buff);
    ldg_mem_dealloc(pool);

    g_mem.stats.pool_cunt--;
}

void ldg_mem_pool_reset(ldg_mem_pool_t *pool)
{
    if (LDG_UNLIKELY(!pool)) { return; }

    if (LDG_UNLIKELY(!pool->is_var)) { return; }

    pool->offset = 0;
    pool->alloc_cunt = 0;
}

uint64_t ldg_mem_pool_remaining_get(const ldg_mem_pool_t *pool)
{
    uint64_t aligned = 0;

    if (LDG_UNLIKELY(!pool)) { return 0; }

    if (LDG_UNLIKELY(!pool->is_var)) { return 0; }

    aligned = (pool->offset + LDG_MEM_POOL_VAR_ALIGN - 1) & ~((uint64_t)LDG_MEM_POOL_VAR_ALIGN - 1);
    if (LDG_UNLIKELY(aligned < pool->offset)) { return 0; }

    if (LDG_UNLIKELY(aligned >= pool->capacity)) { return 0; }

    return pool->capacity - aligned;
}

uint64_t ldg_mem_pool_used_get(const ldg_mem_pool_t *pool)
{
    if (LDG_UNLIKELY(!pool)) { return 0; }

    if (LDG_UNLIKELY(!pool->is_var)) { return 0; }

    return pool->offset;
}

uint64_t ldg_mem_pool_capacity_get(const ldg_mem_pool_t *pool)
{
    if (LDG_UNLIKELY(!pool)) { return 0; }

    return pool->capacity;
}

uint32_t ldg_mem_pool_var_is(const ldg_mem_pool_t *pool)
{
    if (LDG_UNLIKELY(!pool)) { return 0; }

    return pool->is_var;
}

void ldg_mem_stats_get(ldg_mem_stats_t *stats)
{
    if (LDG_UNLIKELY(!stats)) { return; }

    *stats = g_mem.stats;
}

void ldg_mem_leaks_dump(void)
{
    ldg_mem_hdr_t *hdr = 0x0;
    uint32_t leak_cunt = 0;

    if (LDG_UNLIKELY(!g_mem.is_init)) { return; }

    hdr = g_mem.alloc_list;
    while (hdr)
    {
        LDG_LOG_WARNING("ldg_mem: leak; ptr: %p; size: %u", (uint8_t *)hdr + sizeof(ldg_mem_hdr_t), hdr->size);
        leak_cunt++;
        hdr = hdr->next;
    }

    if (LDG_UNLIKELY(leak_cunt > 0)) { LDG_LOG_WARNING("ldg_mem: total leaks: %u; bytes: %zu", leak_cunt, g_mem.stats.bytes_allocated); }
}

uint32_t ldg_mem_valid_is(const void *ptr)
{
    ldg_mem_hdr_t *hdr = 0x0;

    hdr = mem_hdr_find(ptr);
    if (LDG_UNLIKELY(!hdr)) { return 0; }

    if (LDG_UNLIKELY(mem_sentinel_back_check(hdr) != LDG_ERR_AOK)) { return 0; }

    return 1;
}

size_t ldg_mem_size_get(const void *ptr)
{
    ldg_mem_hdr_t *hdr = 0x0;

    hdr = mem_hdr_find(ptr);
    if (LDG_UNLIKELY(!hdr)) { return 0; }

    return hdr->size;
}
