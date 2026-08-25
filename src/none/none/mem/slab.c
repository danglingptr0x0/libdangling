#include <stdint.h>

#include <dangling/mem/slab.h>
#include <dangling/mem/frame.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/err.h>
#include <dangling/core/bits.h>
#include <dangling/core/macros.h>

#define LDG_MEM_SLAB_MAGIC 0x424C5321
#define LDG_MEM_SLAB_CLASS_CUNT 8
#define LDG_MEM_SLAB_ALIGN 16
#define LDG_MEM_SLAB_ALIGN_BIAS (LDG_MEM_SLAB_ALIGN - 1)
#define LDG_MEM_SLAB_FRAME_BIAS ((uint64_t)LDG_MEM_FRAME_WIDTH - 1)
#define LDG_MEM_SLAB_FRAME_CUNT 1
#define LDG_MEM_SLAB_CLASS_SHIFT 1

typedef struct ldg_mem_slab_obj_hdr
{
    uint32_t size;
    uint32_t pudding_link;
    uint8_t in_use;
    uint8_t pudding[7];
} ldg_mem_slab_obj_hdr_t;

typedef struct ldg_mem_slab_hdr
{
    uint32_t magic;
    uint32_t class_idx;
    struct ldg_mem_slab_hdr *next;
    uint8_t *free_list;
    uint64_t obj_cunt;
    uint64_t used_cunt;
    uint64_t stride;
} ldg_mem_slab_hdr_t;

typedef struct ldg_mem_slab_state
{
    ldg_mem_slab_hdr_t *partial[LDG_MEM_SLAB_CLASS_CUNT];
    uint64_t used_cunt;
} ldg_mem_slab_state_t;

static ldg_mem_slab_state_t* slab_state_get(void)
{
    static ldg_mem_slab_state_t state = LDG_STRUCT_ZERO_INIT;

    return &state;
}

static uint64_t slab_class_width_get(uint32_t class_idx)
{
    return (uint64_t)LDG_MEM_SLAB_MIN_WIDTH << class_idx;
}

static uint32_t slab_class_idx_get(uint64_t size, uint32_t *out)
{
    uint64_t width = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(size == 0 || size > LDG_MEM_SLAB_MAX_WIDTH)) { return LDG_ERR_FUNC_ARG_INVALID; }

    width = LDG_MEM_SLAB_MIN_WIDTH;

    for (idx = 0; idx < LDG_MEM_SLAB_CLASS_CUNT; idx++)
    {
        if (size <= width)
        {
            *out = idx;

            return LDG_ERR_AOK;
        }

        width = width << LDG_MEM_SLAB_CLASS_SHIFT;
    }

    return LDG_ERR_FUNC_ARG_INVALID;
}

static ldg_mem_slab_hdr_t* slab_hdr_of(const void *ptr)
{
    uint64_t base = 0;

    base = (uint64_t)(uintptr_t)ptr & ~LDG_MEM_SLAB_FRAME_BIAS;

    return (ldg_mem_slab_hdr_t *)(uintptr_t)base;
}

static uint64_t slab_obj_base_get(void)
{
    return ((uint64_t)sizeof(ldg_mem_slab_hdr_t) + LDG_MEM_SLAB_ALIGN_BIAS) & ~(uint64_t)LDG_MEM_SLAB_ALIGN_BIAS;
}

static uint32_t slab_carve(uint32_t class_idx, ldg_mem_slab_hdr_t **out)
{
    ldg_mem_slab_hdr_t *slab = 0x0;
    uint8_t *area = 0x0;
    uint8_t *item = 0x0;
    uint8_t *prev = 0x0;
    uint64_t stride = 0;
    uint64_t room = 0;
    uint64_t cunt = 0;
    uint64_t idx = 0;
    uint64_t addr = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    ret = ldg_mem_frame_alloc(LDG_MEM_SLAB_FRAME_CUNT, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero((void *)(uintptr_t)addr, (uint64_t)LDG_MEM_FRAME_WIDTH);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_frame_dealloc(addr);
        return ret;
    }

    stride = (uint64_t)sizeof(ldg_mem_slab_obj_hdr_t) + slab_class_width_get(class_idx);
    stride = (stride + LDG_MEM_SLAB_ALIGN_BIAS) & ~(uint64_t)LDG_MEM_SLAB_ALIGN_BIAS;

    room = (uint64_t)LDG_MEM_FRAME_WIDTH - slab_obj_base_get();
    cunt = room / stride;

    if (LDG_UNLIKELY(cunt == 0))
    {
        ldg_mem_frame_dealloc(addr);
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    slab = (ldg_mem_slab_hdr_t *)(uintptr_t)addr;
    area = (uint8_t *)(uintptr_t)addr + slab_obj_base_get();

    prev = 0x0;
    for (idx = 0; idx < cunt; idx++)
    {
        item = area + (idx * stride);

        if (prev) { *(uint8_t **)(void *)prev = item; }

        prev = item;
    }

    if (prev) { *(uint8_t **)(void *)prev = 0x0; }

    slab->magic = LDG_MEM_SLAB_MAGIC;
    slab->class_idx = class_idx;
    slab->next = 0x0;
    slab->free_list = area;
    slab->obj_cunt = cunt;
    slab->used_cunt = 0;
    slab->stride = stride;

    *out = slab;

    return LDG_ERR_AOK;
}

static uint32_t slab_partial_unlink(ldg_mem_slab_state_t *state, ldg_mem_slab_hdr_t *slab)
{
    ldg_mem_slab_hdr_t *walk = 0x0;
    ldg_mem_slab_hdr_t *prev = 0x0;

    if (LDG_UNLIKELY(!state || !slab)) { return LDG_ERR_FUNC_ARG_NULL; }

    walk = state->partial[slab->class_idx];
    while (walk)
    {
        if (walk == slab)
        {
            if (prev) { prev->next = walk->next; }
            else { state->partial[slab->class_idx] = walk->next; }

            slab->next = 0x0;

            return LDG_ERR_AOK;
        }

        prev = walk;
        walk = walk->next;
    }

    return LDG_ERR_NOT_FOUND;
}

uint8_t ldg_mem_slab_serves_is(uint64_t size)
{
    if (size == 0) { return LDG_TRUTH_FALSE; }

    if (size > LDG_MEM_SLAB_MAX_WIDTH) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint8_t ldg_mem_slab_owned_is(const void *ptr)
{
    ldg_mem_slab_hdr_t *slab = 0x0;

    if (LDG_UNLIKELY(!ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    slab = slab_hdr_of(ptr);

    if (slab->magic != LDG_MEM_SLAB_MAGIC) { return LDG_TRUTH_FALSE; }

    if (LDG_UNLIKELY(slab->class_idx >= LDG_MEM_SLAB_CLASS_CUNT)) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint64_t ldg_mem_slab_size_get(const void *ptr)
{
    const ldg_mem_slab_obj_hdr_t *obj = 0x0;

    if (LDG_UNLIKELY(!ptr)) { return UINT64_MAX; }

    if (ldg_mem_slab_owned_is(ptr) != LDG_TRUTH_TRUE) { return UINT64_MAX; }

    obj = (const ldg_mem_slab_obj_hdr_t *)(const void *)((const uint8_t *)ptr - (uint64_t)sizeof(ldg_mem_slab_obj_hdr_t));

    return (uint64_t)obj->size;
}

uint64_t ldg_mem_slab_used_cunt_get(void)
{
    return slab_state_get()->used_cunt;
}

uint32_t ldg_mem_slab_alloc(uint64_t size, void **out)
{
    ldg_mem_slab_state_t *state = 0x0;
    ldg_mem_slab_hdr_t *slab = 0x0;
    ldg_mem_slab_obj_hdr_t *obj = 0x0;
    uint8_t *item = 0x0;
    uint32_t class_idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    ret = slab_class_idx_get(size, &class_idx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state = slab_state_get();

    slab = state->partial[class_idx];

    if (!slab)
    {
        ret = slab_carve(class_idx, &slab);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        slab->next = state->partial[class_idx];
        state->partial[class_idx] = slab;
    }

    if (LDG_UNLIKELY(!slab->free_list)) { return LDG_ERR_MEM_CORRUPTION; }

    item = slab->free_list;
    slab->free_list = *(uint8_t **)(void *)item;
    slab->used_cunt++;
    state->used_cunt++;

    if (!slab->free_list)
    {
        ret = slab_partial_unlink(state, slab);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    obj = (ldg_mem_slab_obj_hdr_t *)(void *)item;
    obj->size = (uint32_t)size;
    obj->pudding_link = 0;
    obj->in_use = LDG_TRUTH_TRUE;

    *out = item + (uint64_t)sizeof(ldg_mem_slab_obj_hdr_t);

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_slab_dealloc(void *ptr)
{
    ldg_mem_slab_state_t *state = 0x0;
    ldg_mem_slab_hdr_t *slab = 0x0;
    ldg_mem_slab_obj_hdr_t *obj = 0x0;
    uint8_t *item = 0x0;
    uint64_t area = 0;
    uint64_t item_addr = 0;
    uint64_t addr = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_slab_owned_is(ptr) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    slab = slab_hdr_of(ptr);
    item = (uint8_t *)ptr - (uint64_t)sizeof(ldg_mem_slab_obj_hdr_t);
    obj = (ldg_mem_slab_obj_hdr_t *)(void *)item;

    area = (uint64_t)(uintptr_t)slab + slab_obj_base_get();
    item_addr = (uint64_t)(uintptr_t)item;

    if (LDG_UNLIKELY(item_addr < area)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(item_addr >= area + (slab->obj_cunt * slab->stride))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(((item_addr - area) % slab->stride) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(obj->in_use != LDG_TRUTH_TRUE)) { return LDG_ERR_MEM_DOUBLE_FREE; }

    if (LDG_UNLIKELY((uint64_t)obj->size > slab_class_width_get(slab->class_idx))) { return LDG_ERR_MEM_CORRUPTION; }

    if (LDG_UNLIKELY(slab->used_cunt == 0)) { return LDG_ERR_MEM_DOUBLE_FREE; }

    obj->in_use = LDG_TRUTH_FALSE;

    state = slab_state_get();

    if (!slab->free_list)
    {
        slab->next = state->partial[slab->class_idx];
        state->partial[slab->class_idx] = slab;
    }

    *(uint8_t **)(void *)item = slab->free_list;
    slab->free_list = item;
    slab->used_cunt--;

    if (LDG_UNLIKELY(state->used_cunt == 0)) { return LDG_ERR_MEM_CORRUPTION; }

    state->used_cunt--;

    if (slab->used_cunt == 0)
    {
        ret = slab_partial_unlink(state, slab);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        addr = (uint64_t)(uintptr_t)slab;
        slab->magic = 0;

        return ldg_mem_frame_dealloc(addr);
    }

    return LDG_ERR_AOK;
}
