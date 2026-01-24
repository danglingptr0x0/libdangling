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
    uint8_t pudding[24];
};

typedef struct ldg_mem_state
{
    ldg_mem_hdr_t *alloc_list;
    ldg_mem_stats_t stats;
    uint8_t is_init;
    uint8_t pudding[55];
} LDG_ALIGNED ldg_mem_state_t;

static ldg_mem_state_t g_mem = { 0 };


int32_t ldg_mem_init(void)
{
    if (g_mem.is_init) { return LDG_ERR_AOK; }

    (void)memset(&g_mem, 0, sizeof(ldg_mem_state_t));
    g_mem.is_init = 1;

    return LDG_ERR_AOK;
}

void ldg_mem_shutdown(void)
{
    if (!g_mem.is_init) { return; }

    ldg_mem_leaks_dump();
    (void)memset(&g_mem, 0, sizeof(ldg_mem_state_t));
}

void* ldg_mem_alloc(size_t size)
{
    ldg_mem_hdr_t *hdr = NULL;
    uint8_t *raw = NULL;
    uint8_t *user_ptr = NULL;
    uint32_t *sentinel_back = NULL;
    size_t total_size = 0;

    if (LDG_UNLIKELY(size == 0)) { return NULL; }

    if (LDG_UNLIKELY(!g_mem.is_init)) { return NULL; }

    total_size = sizeof(ldg_mem_hdr_t) + size + sizeof(uint32_t);
    total_size = (total_size + LDG_CACHE_LINE_WIDTH - 1) & ~((size_t)LDG_CACHE_LINE_WIDTH - 1);

    raw = aligned_alloc(LDG_CACHE_LINE_WIDTH, total_size);
    if (LDG_UNLIKELY(!raw)) { return NULL; }

    hdr = (ldg_mem_hdr_t *)(void *)raw;
    user_ptr = raw + sizeof(ldg_mem_hdr_t);
    sentinel_back = (uint32_t *)(void *)(user_ptr + size);

    (void)memset(user_ptr, 0, size);

    hdr->sentinel_front = LDG_MEM_SENTINEL;
    hdr->size = (uint32_t)size;
    hdr->next = g_mem.alloc_list;
    hdr->prev = NULL;
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
    ldg_mem_hdr_t *hdr = NULL;

    if (LDG_UNLIKELY(!ptr)) { return NULL; }

    hdr = (ldg_mem_hdr_t *)(void *)((uint8_t *)(uintptr_t)ptr - sizeof(ldg_mem_hdr_t));

    if (hdr->sentinel_front != LDG_MEM_SENTINEL)
    {
        LDG_LOG_ERROR("ldg_mem: front sentinel corrupted; ptr: %p", ptr);
        return NULL;
    }

    return hdr;
}

static int32_t mem_sentinel_back_check(ldg_mem_hdr_t *hdr)
{
    uint8_t *user_ptr = (uint8_t *)(void *)hdr + sizeof(ldg_mem_hdr_t);
    uint32_t *sentinel_back = (uint32_t *)(void *)(user_ptr + hdr->size);

    if (*sentinel_back != LDG_MEM_SENTINEL)
    {
        LDG_LOG_ERROR("ldg_mem: back sentinel corrupted; ptr: %p; size: %u", user_ptr, hdr->size);
        return LDG_ERR_MEM_BAD;
    }

    return LDG_ERR_AOK;
}

void* ldg_mem_realloc(void *ptr, size_t size)
{
    ldg_mem_hdr_t *hdr = NULL;
    void *new_ptr = NULL;
    size_t copy_size = 0;

    if (!ptr) { return ldg_mem_alloc(size); }

    if (size == 0)
    {
        ldg_mem_dealloc(ptr);
        return NULL;
    }

    hdr = mem_hdr_find(ptr);
    if (LDG_UNLIKELY(!hdr)) { return NULL; }

    if (mem_sentinel_back_check(hdr) != LDG_ERR_AOK) { return NULL; }

    new_ptr = ldg_mem_alloc(size);
    if (LDG_UNLIKELY(!new_ptr)) { return NULL; }

    copy_size = (hdr->size < size) ? hdr->size : size;
    (void)memcpy(new_ptr, ptr, copy_size);

    ldg_mem_dealloc(ptr);

    return new_ptr;
}

void ldg_mem_dealloc(void *ptr)
{
    ldg_mem_hdr_t *hdr = NULL;
    uint64_t *poison_ptr = NULL;
    size_t poison_cunt = 0;
    size_t i = 0;

    if (LDG_UNLIKELY(!ptr)) { return; }

    if (LDG_UNLIKELY(!g_mem.is_init)) { return; }

    hdr = mem_hdr_find(ptr);
    if (LDG_UNLIKELY(!hdr)) { return; }

    if (mem_sentinel_back_check(hdr) != LDG_ERR_AOK) { return; }

    if (hdr->prev) { hdr->prev->next = hdr->next; }
    else{ g_mem.alloc_list = hdr->next; }

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
    ldg_mem_pool_t *pool = NULL;
    size_t aligned_item_size = 0;
    size_t i = 0;
    uint8_t *item = NULL;

    if (LDG_UNLIKELY(item_size == 0 || capacity == 0)) { return NULL; }

    aligned_item_size = (item_size + sizeof(void *) + LDG_CACHE_LINE_WIDTH - 1) & ~((size_t)LDG_CACHE_LINE_WIDTH - 1);

    pool = ldg_mem_alloc(sizeof(ldg_mem_pool_t));
    if (LDG_UNLIKELY(!pool)) { return NULL; }

    pool->buff = ldg_mem_alloc(aligned_item_size * capacity);
    if (LDG_UNLIKELY(!pool->buff))
    {
        ldg_mem_dealloc(pool);
        return NULL;
    }

    pool->item_size = aligned_item_size;
    pool->capacity = capacity;
    pool->alloc_cunt = 0;

    pool->free_list = pool->buff;
    for (i = 0; i < capacity - 1; i++)
    {
        item = pool->buff + (i * aligned_item_size);
        *(uint8_t **)(void *)(item) = pool->buff + ((i + 1) * aligned_item_size);
    }
    item = pool->buff + ((capacity - 1) * aligned_item_size);
    *(uint8_t **)(void *)(item) = NULL;

    g_mem.stats.pool_cunt++;

    return pool;
}

void* ldg_mem_pool_alloc(ldg_mem_pool_t *pool)
{
    uint8_t *item = NULL;

    if (LDG_UNLIKELY(!pool)) { return NULL; }

    if (LDG_UNLIKELY(!pool->free_list)) { return NULL; }

    item = pool->free_list;
    pool->free_list = *(uint8_t **)(void *)(item);
    pool->alloc_cunt++;

    (void)memset(item, 0, pool->item_size);

    return item;
}

void ldg_mem_pool_dealloc(ldg_mem_pool_t *pool, void *ptr)
{
    uint8_t *item = NULL;

    if (LDG_UNLIKELY(!pool || !ptr)) { return; }

    item = (uint8_t *)ptr;

    if (item < pool->buff || item >= pool->buff + (pool->item_size * pool->capacity))
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

    if (pool->alloc_cunt > 0) { LDG_LOG_WARNING("ldg_mem_pool: destroying pool with %zu active allocs", pool->alloc_cunt); }

    ldg_mem_dealloc(pool->buff);
    ldg_mem_dealloc(pool);

    g_mem.stats.pool_cunt--;
}

void ldg_mem_stats_get(ldg_mem_stats_t *stats)
{
    if (LDG_UNLIKELY(!stats)) { return; }

    *stats = g_mem.stats;
}

void ldg_mem_leaks_dump(void)
{
    ldg_mem_hdr_t *hdr = NULL;
    uint32_t leak_cunt = 0;

    if (!g_mem.is_init) { return; }

    hdr = g_mem.alloc_list;
    while (hdr)
    {
        LDG_LOG_WARNING("ldg_mem: leak; ptr: %p; size: %u", (uint8_t *)hdr + sizeof(ldg_mem_hdr_t), hdr->size);
        leak_cunt++;
        hdr = hdr->next;
    }

    if (leak_cunt > 0) { LDG_LOG_WARNING("ldg_mem: total leaks: %u; bytes: %zu", leak_cunt, g_mem.stats.bytes_allocated); }
}

int32_t ldg_mem_valid_is(const void *ptr)
{
    ldg_mem_hdr_t *hdr = NULL;

    hdr = mem_hdr_find(ptr);
    if (!hdr) { return 0; }

    if (mem_sentinel_back_check(hdr) != LDG_ERR_AOK) { return 0; }

    return 1;
}

size_t ldg_mem_size_get(const void *ptr)
{
    ldg_mem_hdr_t *hdr = NULL;

    hdr = mem_hdr_find(ptr);
    if (!hdr) { return 0; }

    return hdr->size;
}
