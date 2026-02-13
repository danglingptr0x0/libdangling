#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <dangling/mem/alloc.h>
#include <dangling/mem/mem.h>
#include <dangling/core/err.h>
#include <dangling/core/arith.h>
#include <dangling/thread/sync.h>
#include <dangling/core/macros.h>

typedef struct ldg_mem_hdr
{
    uint32_t sentinel_front;
    uint8_t pudding_inner[4];
    struct ldg_mem_hdr *next;
    struct ldg_mem_hdr *prev;
    uint64_t size;
    uint8_t pudding[24];
} LDG_ALIGNED ldg_mem_hdr_t;

struct ldg_mem_pool
{
    uint8_t *buff;
    uint8_t *free_list;
    uint64_t item_size;
    uint64_t capacity;
    uint64_t alloc_cunt;
    uint64_t buff_size;
    union
    {
        uint64_t user_item_size;
        uint64_t bump_offset;
    }
    u;
    ldg_mut_t mut;
    uint8_t is_var;
    uint8_t is_destroying;
    uint8_t pudding[14];
};

typedef struct ldg_mem_state
{
    ldg_mem_hdr_t *alloc_list;
    ldg_mem_stats_t stats;
    uint8_t is_init;
    uint8_t is_locked;
    uint8_t pudding[54];
} LDG_ALIGNED ldg_mem_state_t;

// singleton allocator; file-scope statics required for process-wide state
static ldg_mem_state_t g_mem = LDG_STRUCT_ZERO_INIT;
static ldg_mut_t g_mem_mut = LDG_STRUCT_ZERO_INIT;

static uint32_t ldg_mem_leaks_dump_unlocked(void)
{
    ldg_mem_hdr_t *hdr = 0x0;
    uint64_t leak_cunt = 0;
    uint64_t max_walk = 0;

    max_walk = g_mem.stats.active_alloc_cunt;
    hdr = g_mem.alloc_list;
    while (hdr)
    {
        leak_cunt++;
        if (LDG_UNLIKELY(leak_cunt > max_walk)) { return LDG_ERR_MEM_CORRUPTION; }

        hdr = hdr->next;
    }


    return LDG_ERR_AOK;
}

static uint32_t ldg_mem_sentinel_wr(uint8_t *user_ptr, uint64_t size)
{
    uint32_t sentinel = LDG_MEM_SENTINEL;

    if (LDG_UNLIKELY(!user_ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memcpy(user_ptr + size, &sentinel, (uint64_t)sizeof(uint32_t)) != user_ptr + size)) { return LDG_ERR_MEM_BAD; }

    return LDG_ERR_AOK;
}

static uint32_t ldg_mem_sentinel_back_check(const ldg_mem_hdr_t *hdr)
{
    uint8_t *user_ptr = 0x0;
    uint32_t sentinel = 0;

    if (LDG_UNLIKELY(!hdr)) { return LDG_ERR_FUNC_ARG_NULL; }

    user_ptr = (uint8_t *)(uintptr_t)hdr + (uint64_t)sizeof(ldg_mem_hdr_t);
    if (LDG_UNLIKELY(memcpy(&sentinel, user_ptr + hdr->size, (uint64_t)sizeof(uint32_t)) != &sentinel)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(sentinel != LDG_MEM_SENTINEL)) { return LDG_ERR_MEM_BAD; }

    return LDG_ERR_AOK;
}

static uint32_t ldg_mem_hdr_find(const void *ptr, ldg_mem_hdr_t **out)
{
    ldg_mem_hdr_t *hdr = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY((uintptr_t)ptr < (uint64_t)sizeof(ldg_mem_hdr_t))) { return LDG_ERR_FUNC_ARG_INVALID; }

    hdr = (ldg_mem_hdr_t *)(void *)((uint8_t *)(uintptr_t)ptr - (uint64_t)sizeof(ldg_mem_hdr_t));

    if (LDG_UNLIKELY(hdr->sentinel_front != LDG_MEM_SENTINEL)) { return LDG_ERR_MEM_CORRUPTION; }

    *out = hdr;

    return LDG_ERR_AOK;
}

static uint32_t ldg_mem_alloc_unlocked(uint64_t size, void **out)
{
    ldg_mem_hdr_t *hdr = 0x0;
    uint8_t *raw = 0x0;
    uint8_t *user_ptr = 0x0;
    void *raw_tmp = 0x0;
    uint64_t total_size = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!g_mem.is_init)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(g_mem.is_locked)) { return LDG_ERR_DENIED; }

    ret = ldg_arith_64_add((uint64_t)sizeof(ldg_mem_hdr_t), size, &total_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_arith_64_add(total_size, (uint64_t)sizeof(uint32_t), &total_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    ret = ldg_arith_64_add(total_size, (uint64_t)(LDG_AMD64_CACHE_LINE_WIDTH - 1), &total_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_OVERFLOW; }

    total_size &= ~((uint64_t)LDG_AMD64_CACHE_LINE_WIDTH - 1);

    if (LDG_UNLIKELY(posix_memalign(&raw_tmp, LDG_AMD64_CACHE_LINE_WIDTH, total_size) != 0)) { return LDG_ERR_ALLOC_NULL; }

    raw = (uint8_t *)raw_tmp;

    if (LDG_UNLIKELY(memset(raw, 0, total_size) != raw))
    {
        free(raw);
        return LDG_ERR_MEM_BAD;
    }

    hdr = (ldg_mem_hdr_t *)(void *)raw;
    user_ptr = raw + (uint64_t)sizeof(ldg_mem_hdr_t);

    hdr->sentinel_front = LDG_MEM_SENTINEL;
    hdr->size = size;
    hdr->next = g_mem.alloc_list;
    hdr->prev = 0x0;

    ret = ldg_mem_sentinel_wr(user_ptr, size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        free(raw);
        return ret;
    }

    {
        uint64_t new_bytes = 0;
        ret = ldg_arith_64_add(g_mem.stats.bytes_alloc, size, &new_bytes);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            free(raw);
            return LDG_ERR_OVERFLOW;
        }

        if (g_mem.alloc_list) { g_mem.alloc_list->prev = hdr; }

        g_mem.alloc_list = hdr;
        g_mem.stats.bytes_alloc = new_bytes;
    }
    g_mem.stats.alloc_cunt++;
    g_mem.stats.active_alloc_cunt++;

    if (g_mem.stats.bytes_alloc > g_mem.stats.bytes_peak) { g_mem.stats.bytes_peak = g_mem.stats.bytes_alloc; }

    *out = user_ptr;

    return LDG_ERR_AOK;
}

static uint32_t ldg_mem_dealloc_unlocked(void *ptr)
{
    ldg_mem_hdr_t *hdr = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_mem.is_init)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mem_hdr_find(ptr, &hdr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_MEM_CORRUPTION; }

    ret = ldg_mem_sentinel_back_check(hdr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_MEM_CORRUPTION; }

    if (LDG_UNLIKELY(hdr->size > g_mem.stats.bytes_alloc)) { return LDG_ERR_MEM_CORRUPTION; }

    if (LDG_UNLIKELY(g_mem.stats.active_alloc_cunt == 0)) { return LDG_ERR_MEM_CORRUPTION; }

    if (hdr->prev) { hdr->prev->next = hdr->next; }
    else { g_mem.alloc_list = hdr->next; }

    if (hdr->next) { hdr->next->prev = hdr->prev; }

    g_mem.stats.bytes_alloc -= hdr->size;
    g_mem.stats.dealloc_cunt++;
    g_mem.stats.active_alloc_cunt--;

    if (LDG_UNLIKELY(memset(ptr, LDG_MEM_POISON_BYTE, hdr->size) != ptr)) { return LDG_ERR_MEM_BAD; }

    hdr->sentinel_front = 0;

    free(hdr);

    return LDG_ERR_AOK;
}

// lifecycle: init/shutdown shall be called from a single thread
uint32_t ldg_mem_init(void)
{
    uint32_t ret = 0;

    if (!g_mem_mut.is_init)
    {
        ret = ldg_mut_init(&g_mem_mut, 0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_mut_lock(&g_mem_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (g_mem.is_init) { ldg_mut_unlock(&g_mem_mut); return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(memset(&g_mem, 0, (uint64_t)sizeof(ldg_mem_state_t)) != &g_mem))
    {
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_MEM_BAD;
    }

    g_mem.is_init = 1;

    ldg_mut_unlock(&g_mem_mut);

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_shutdown(void)
{
    uint32_t ret = 0;

    ret = ldg_mut_lock(&g_mem_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!g_mem.is_init))
    {
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_NOT_INIT;
    }

    if (LDG_UNLIKELY(g_mem.stats.active_alloc_cunt > 0))
    {
        ldg_mem_leaks_dump_unlocked();

        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_BUSY;
    }

    if (LDG_UNLIKELY(memset(&g_mem, 0, (uint64_t)sizeof(ldg_mem_state_t)) != &g_mem))
    {
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_MEM_BAD;
    }

    ldg_mut_unlock(&g_mem_mut);

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_lock(void)
{
    uint32_t ret = 0;

    ret = ldg_mut_lock(&g_mem_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!g_mem.is_init))
    {
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_NOT_INIT;
    }

    g_mem.is_locked = 1;

    ldg_mut_unlock(&g_mem_mut);

    return LDG_ERR_AOK;
}

uint8_t ldg_mem_locked_is(void)
{
    uint8_t locked = 0;

    if (LDG_UNLIKELY(ldg_mut_lock(&g_mem_mut) != LDG_ERR_AOK)) { return 0; }

    if (LDG_UNLIKELY(!g_mem.is_init)) { ldg_mut_unlock(&g_mem_mut); return 0; }

    locked = g_mem.is_locked;
    ldg_mut_unlock(&g_mem_mut);

    return locked;
}

uint32_t ldg_mem_alloc(uint64_t size, void **out)
{
    uint32_t ret = 0;

    ret = ldg_mut_lock(&g_mem_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_alloc_unlocked(size, out);

    ldg_mut_unlock(&g_mem_mut);

    return ret;
}

uint32_t ldg_mem_realloc(void *ptr, uint64_t size, void **out)
{
    ldg_mem_hdr_t *hdr = 0x0;
    void *new_ptr = 0x0;
    uint64_t copy_size = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ptr)) { return ldg_mem_alloc(size, out); }

    ret = ldg_mut_lock(&g_mem_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!g_mem.is_init))
    {
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_NOT_INIT;
    }

    if (LDG_UNLIKELY(g_mem.is_locked))
    {
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_DENIED;
    }

    ret = ldg_mem_hdr_find(ptr, &hdr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_MEM_CORRUPTION;
    }

    ret = ldg_mem_sentinel_back_check(hdr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_MEM_CORRUPTION;
    }

    ret = ldg_mem_alloc_unlocked(size, &new_ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mut_unlock(&g_mem_mut);
        return ret;
    }

    copy_size = (hdr->size < size) ? hdr->size : size;
    if (LDG_UNLIKELY(memcpy(new_ptr, ptr, copy_size) != new_ptr))
    {
        ldg_mem_dealloc_unlocked(new_ptr);
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_MEM_BAD;
    }

    ret = ldg_mem_dealloc_unlocked(ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        *out = new_ptr;
        ldg_mut_unlock(&g_mem_mut);
        return ret;
    }

    *out = new_ptr;

    ldg_mut_unlock(&g_mem_mut);

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_dealloc(void *ptr)
{
    uint32_t ret = 0;

    ret = ldg_mut_lock(&g_mem_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_dealloc_unlocked(ptr);

    ldg_mut_unlock(&g_mem_mut);

    return ret;
}

uint32_t ldg_mem_pool_create(uint64_t item_size, uint64_t capacity, ldg_mem_pool_t **out)
{
    ldg_mem_pool_t *pool = 0x0;
    void *pool_tmp = 0x0;
    void *buff = 0x0;
    uint64_t aligned_item_size = 0;
    uint64_t buff_size = 0;
    uint64_t i = 0;
    uint8_t *item = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(capacity == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&g_mem_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!g_mem.is_init)) { ldg_mut_unlock(&g_mem_mut); return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(g_mem.is_locked)) { ldg_mut_unlock(&g_mem_mut); return LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(g_mem.stats.pool_cunt >= LDG_MEM_POOL_MAX))
    {
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_FULL;
    }

    ret = ldg_mem_alloc_unlocked((uint64_t)sizeof(ldg_mem_pool_t), &pool_tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mut_unlock(&g_mem_mut);
        return ret;
    }

    pool = (ldg_mem_pool_t *)pool_tmp;

    if (item_size == 0)
    {
        ret = ldg_mem_alloc_unlocked(capacity, &buff);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ldg_mem_dealloc_unlocked(pool);
            ldg_mut_unlock(&g_mem_mut);
            return LDG_ERR_ALLOC_NULL;
        }

        pool->buff = buff;
        pool->free_list = 0x0;
        pool->item_size = 0;
        pool->capacity = capacity;
        pool->alloc_cunt = 0;
        pool->buff_size = capacity;
        pool->u.bump_offset = 0;
        pool->is_var = 1;
        pool->is_destroying = 0;

        ret = ldg_mut_init(&pool->mut, 0);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            ldg_mem_dealloc_unlocked(buff);
            ldg_mem_dealloc_unlocked(pool);
            ldg_mut_unlock(&g_mem_mut);
            return ret;
        }

        g_mem.stats.pool_cunt++;
        ldg_mut_unlock(&g_mem_mut);

        *out = pool;

        return LDG_ERR_AOK;
    }

    ret = ldg_arith_64_add(item_size, (uint64_t)sizeof(void *), &aligned_item_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc_unlocked(pool);
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_OVERFLOW;
    }

    ret = ldg_arith_64_add(aligned_item_size, (uint64_t)(LDG_AMD64_CACHE_LINE_WIDTH - 1), &aligned_item_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc_unlocked(pool);
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_OVERFLOW;
    }

    aligned_item_size &= ~((uint64_t)LDG_AMD64_CACHE_LINE_WIDTH - 1);

    if (LDG_UNLIKELY(ldg_arith_64_mul(aligned_item_size, capacity, &buff_size) != LDG_ERR_AOK))
    {
        ldg_mem_dealloc_unlocked(pool);
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_OVERFLOW;
    }

    ret = ldg_mem_alloc_unlocked(buff_size, &buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc_unlocked(pool);
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_ALLOC_NULL;
    }

    pool->buff = buff;
    pool->item_size = aligned_item_size;
    pool->capacity = capacity;
    pool->alloc_cunt = 0;
    pool->buff_size = buff_size;
    pool->u.user_item_size = item_size;
    pool->is_var = 0;
    pool->is_destroying = 0;

    pool->free_list = pool->buff;
    for (i = 0; i < capacity - 1; i++)
    {
        item = pool->buff + (i * aligned_item_size);
        *(uint8_t **)(void *)(item) = pool->buff + ((i + 1) * aligned_item_size);
    }
    item = pool->buff + ((capacity - 1) * aligned_item_size);
    *(uint8_t **)(void *)(item) = 0x0;

    ret = ldg_mut_init(&pool->mut, 0);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_dealloc_unlocked(buff);
        ldg_mem_dealloc_unlocked(pool);
        ldg_mut_unlock(&g_mem_mut);
        return ret;
    }

    g_mem.stats.pool_cunt++;
    ldg_mut_unlock(&g_mem_mut);

    *out = pool;

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_pool_alloc(ldg_mem_pool_t *pool, uint64_t size, void **out)
{
    uint8_t *item = 0x0;
    uint64_t aligned = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mut_lock(&pool->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(pool->is_destroying)) { ldg_mut_unlock(&pool->mut); return LDG_ERR_INVALID; }

    if (pool->is_var)
    {
        if (LDG_UNLIKELY(size == 0)) { ldg_mut_unlock(&pool->mut); return LDG_ERR_FUNC_ARG_INVALID; }

        aligned = (pool->u.bump_offset + LDG_MEM_POOL_VAR_ALIGN - 1) & ~((uint64_t)LDG_MEM_POOL_VAR_ALIGN - 1);
        if (LDG_UNLIKELY(aligned < pool->u.bump_offset)) { ldg_mut_unlock(&pool->mut); return LDG_ERR_OVERFLOW; }

        if (LDG_UNLIKELY(aligned + size < aligned)) { ldg_mut_unlock(&pool->mut); return LDG_ERR_OVERFLOW; }

        if (LDG_UNLIKELY(aligned + size > pool->capacity)) { ldg_mut_unlock(&pool->mut); return LDG_ERR_MEM_POOL_FULL; }

        item = pool->buff + aligned;
        if (LDG_UNLIKELY(memset(item, 0, size) != item)) { ldg_mut_unlock(&pool->mut); return LDG_ERR_MEM_BAD; }

        pool->u.bump_offset = aligned + size;
        pool->alloc_cunt++;

        *out = item;

        ldg_mut_unlock(&pool->mut);

        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY(size != pool->u.user_item_size)) { ldg_mut_unlock(&pool->mut); return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!pool->free_list)) { ldg_mut_unlock(&pool->mut); return LDG_ERR_MEM_POOL_FULL; }

    item = pool->free_list;
    pool->free_list = *(uint8_t **)(void *)(item);
    pool->alloc_cunt++;

    if (LDG_UNLIKELY(memset(item, 0, pool->u.user_item_size) != item))
    {
        *(uint8_t **)(void *)(item) = pool->free_list;
        pool->free_list = item;
        pool->alloc_cunt--;
        ldg_mut_unlock(&pool->mut);
        return LDG_ERR_MEM_BAD;
    }

    *out = item;

    ldg_mut_unlock(&pool->mut);

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_pool_dealloc(ldg_mem_pool_t *pool, void *ptr)
{
    uint8_t *item = 0x0;
    uint8_t *walk = 0x0;
    uint64_t walk_cunt = 0;
    uint64_t max_free = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    item = (uint8_t *)ptr;

    ret = ldg_mut_lock(&pool->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(pool->is_destroying)) { ldg_mut_unlock(&pool->mut); return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(pool->is_var))
    {
        ldg_mut_unlock(&pool->mut);
        return LDG_ERR_UNSUPPORTED;
    }

    if (LDG_UNLIKELY(item < pool->buff || item >= pool->buff + pool->buff_size))
    {
        ldg_mut_unlock(&pool->mut);
        return LDG_ERR_BOUNDS;
    }

    if (LDG_UNLIKELY((uintptr_t)(item - pool->buff) % pool->item_size != 0))
    {
        ldg_mut_unlock(&pool->mut);
        return LDG_ERR_MEM_ALIGNMENT;
    }

    if (LDG_UNLIKELY(pool->alloc_cunt == 0))
    {
        ldg_mut_unlock(&pool->mut);
        return LDG_ERR_MEM_DOUBLE_FREE;
    }

    max_free = pool->capacity - pool->alloc_cunt;
    walk = pool->free_list;
    while (walk)
    {
        if (LDG_UNLIKELY(walk == item))
        {
            ldg_mut_unlock(&pool->mut);
            return LDG_ERR_MEM_DOUBLE_FREE;
        }

        walk_cunt++;
        if (LDG_UNLIKELY(walk_cunt > max_free))
        {
            ldg_mut_unlock(&pool->mut);
            return LDG_ERR_MEM_CORRUPTION;
        }

        walk = *(uint8_t **)(void *)(walk);
    }

    if (LDG_UNLIKELY(memset(item, LDG_MEM_POISON_BYTE, pool->item_size) != item))
    {
        ldg_mut_unlock(&pool->mut);
        return LDG_ERR_MEM_BAD;
    }

    *(uint8_t **)(void *)(item) = pool->free_list;
    pool->free_list = item;
    pool->alloc_cunt--;

    ldg_mut_unlock(&pool->mut);

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_pool_destroy(ldg_mem_pool_t **pool)
{
    void *buff = 0x0;
    uint32_t ret = 0;
    uint32_t first_err = 0;

    if (LDG_UNLIKELY(!pool || !*pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mut_lock(&(*pool)->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    (*pool)->is_destroying = 1;
    buff = (*pool)->buff;

    ldg_mut_unlock(&(*pool)->mut);

    ret = ldg_mem_dealloc(buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { if (first_err == 0) { first_err = ret; } }

    ret = ldg_mut_destroy(&(*pool)->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { if (first_err == 0) { first_err = ret; } }

    ret = ldg_mem_dealloc(*pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { if (first_err == 0) { first_err = ret; } }

    *pool = 0x0;

    ret = ldg_mut_lock(&g_mem_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (first_err != 0) ? first_err : ret; }

    if (LDG_UNLIKELY(g_mem.stats.pool_cunt == 0))
    {
        ldg_mut_unlock(&g_mem_mut);
        return (first_err != 0) ? first_err : LDG_ERR_MEM_CORRUPTION;
    }

    g_mem.stats.pool_cunt--;
    ldg_mut_unlock(&g_mem_mut);

    return first_err;
}

uint32_t ldg_mem_pool_rst(ldg_mem_pool_t *pool)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mut_lock(&pool->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(pool->is_destroying)) { ldg_mut_unlock(&pool->mut); return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(!pool->is_var))
    {
        ldg_mut_unlock(&pool->mut);
        return LDG_ERR_UNSUPPORTED;
    }

    if (LDG_UNLIKELY(memset(pool->buff, 0, pool->capacity) != pool->buff))
    {
        ldg_mut_unlock(&pool->mut);
        return LDG_ERR_MEM_BAD;
    }

    pool->u.bump_offset = 0;
    pool->alloc_cunt = 0;

    ldg_mut_unlock(&pool->mut);

    return LDG_ERR_AOK;
}

uint64_t ldg_mem_pool_remaining_get(ldg_mem_pool_t *pool)
{
    uint64_t aligned = 0;
    uint64_t remaining = 0;

    if (LDG_UNLIKELY(!pool)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(ldg_mut_lock(&pool->mut) != LDG_ERR_AOK)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(pool->is_destroying || !pool->is_var)) { ldg_mut_unlock(&pool->mut); return UINT64_MAX; }

    aligned = (pool->u.bump_offset + LDG_MEM_POOL_VAR_ALIGN - 1) & ~((uint64_t)LDG_MEM_POOL_VAR_ALIGN - 1);
    if (LDG_UNLIKELY(aligned < pool->u.bump_offset)) { ldg_mut_unlock(&pool->mut); return UINT64_MAX; }

    if (LDG_UNLIKELY(aligned >= pool->capacity)) { ldg_mut_unlock(&pool->mut); return 0; }

    remaining = pool->capacity - aligned;

    ldg_mut_unlock(&pool->mut);

    return remaining;
}

uint64_t ldg_mem_pool_used_get(ldg_mem_pool_t *pool)
{
    uint64_t used = 0;

    if (LDG_UNLIKELY(!pool)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(ldg_mut_lock(&pool->mut) != LDG_ERR_AOK)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(pool->is_destroying || !pool->is_var)) { ldg_mut_unlock(&pool->mut); return UINT64_MAX; }

    used = pool->u.bump_offset;

    ldg_mut_unlock(&pool->mut);

    return used;
}

uint64_t ldg_mem_pool_capacity_get(ldg_mem_pool_t *pool)
{
    uint64_t cap = 0;

    if (LDG_UNLIKELY(!pool)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(ldg_mut_lock(&pool->mut) != LDG_ERR_AOK)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(pool->is_destroying)) { ldg_mut_unlock(&pool->mut); return UINT64_MAX; }

    cap = pool->capacity;

    ldg_mut_unlock(&pool->mut);

    return cap;
}

uint8_t ldg_mem_pool_var_is(ldg_mem_pool_t *pool)
{
    uint8_t var = 0;

    if (LDG_UNLIKELY(!pool)) { return 0; }

    if (LDG_UNLIKELY(ldg_mut_lock(&pool->mut) != LDG_ERR_AOK)) { return 0; }

    if (LDG_UNLIKELY(pool->is_destroying)) { ldg_mut_unlock(&pool->mut); return 0; }

    var = pool->is_var;

    ldg_mut_unlock(&pool->mut);

    return var;
}

uint32_t ldg_mem_stats_get(ldg_mem_stats_t *stats)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mut_lock(&g_mem_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *stats = g_mem.stats;
    ldg_mut_unlock(&g_mem_mut);

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_leaks_dump(void)
{
    uint32_t ret = 0;

    ret = ldg_mut_lock(&g_mem_mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!g_mem.is_init))
    {
        ldg_mut_unlock(&g_mem_mut);
        return LDG_ERR_NOT_INIT;
    }

    ret = ldg_mem_leaks_dump_unlocked();

    ldg_mut_unlock(&g_mem_mut);

    return ret;
}

uint8_t ldg_mem_valid_is(const void *ptr)
{
    ldg_mem_hdr_t *hdr = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ldg_mut_lock(&g_mem_mut) != LDG_ERR_AOK)) { return 0; }

    ret = ldg_mem_hdr_find(ptr, &hdr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mut_unlock(&g_mem_mut);
        return 0;
    }

    ret = ldg_mem_sentinel_back_check(hdr);

    ldg_mut_unlock(&g_mem_mut);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return 0; }

    return 1;
}

uint64_t ldg_mem_size_get(const void *ptr)
{
    ldg_mem_hdr_t *hdr = 0x0;
    uint64_t size = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(ldg_mut_lock(&g_mem_mut) != LDG_ERR_AOK)) { return UINT64_MAX; }

    ret = ldg_mem_hdr_find(ptr, &hdr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mut_unlock(&g_mem_mut);
        return UINT64_MAX;
    }

    size = hdr->size;

    ldg_mut_unlock(&g_mem_mut);

    return size;
}
