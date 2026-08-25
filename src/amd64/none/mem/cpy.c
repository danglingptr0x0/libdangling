#include <stdint.h>
#include <string.h>

#include <dangling/mem/cpy.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

static uint8_t mem_overlap_is(const void *dst, const void *src, uint64_t len)
{
    const uint8_t *dst_byte = (const uint8_t *)dst;
    const uint8_t *src_byte = (const uint8_t *)src;

    if (dst_byte < src_byte && (dst_byte + len) > src_byte) { return LDG_TRUTH_TRUE; }

    if (src_byte < dst_byte && (src_byte + len) > dst_byte) { return LDG_TRUTH_TRUE; }

    if (dst_byte == src_byte) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_mem_cpy(void *dst, const void *src, uint64_t len)
{
    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(len > UINT64_MAX - (uint64_t)(uintptr_t)dst)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(len > UINT64_MAX - (uint64_t)(uintptr_t)src)) { return LDG_ERR_OVERFLOW; }

    if (mem_overlap_is(dst, src, len) == LDG_TRUTH_TRUE)
    {
        if (LDG_UNLIKELY(memmove(dst, src, len) != dst)) { return LDG_ERR_MEM_BAD; }

        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY(memcpy(dst, src, len) != dst)) { return LDG_ERR_MEM_BAD; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_mov(void *dst, const void *src, uint64_t len)
{
    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(len > UINT64_MAX - (uint64_t)(uintptr_t)dst)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(len > UINT64_MAX - (uint64_t)(uintptr_t)src)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(memmove(dst, src, len) != dst)) { return LDG_ERR_MEM_BAD; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_zero(void *ptr, uint64_t len)
{
    if (LDG_UNLIKELY(!ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(len > UINT64_MAX - (uint64_t)(uintptr_t)ptr)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(memset(ptr, LDG_TRUTH_FALSE, len) != ptr)) { return LDG_ERR_MEM_BAD; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_fill(void *ptr, uint8_t val, uint64_t len)
{
    if (LDG_UNLIKELY(!ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(len > UINT64_MAX - (uint64_t)(uintptr_t)ptr)) { return LDG_ERR_OVERFLOW; }

    if (LDG_UNLIKELY(memset(ptr, val, len) != ptr)) { return LDG_ERR_MEM_BAD; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_cmp(const void *lhs, const void *rhs, uint64_t len, uint32_t *out)
{
    const uint8_t *lhs_byte = 0x0;
    const uint8_t *rhs_byte = 0x0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MEM_ORD_EQ;

    if (LDG_UNLIKELY(!lhs || !rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (memcmp(lhs, rhs, len) == 0) { return LDG_ERR_AOK; }

    lhs_byte = (const uint8_t *)lhs;
    rhs_byte = (const uint8_t *)rhs;

    for (idx = 0; idx < len; idx++)
    {
        if (lhs_byte[idx] < rhs_byte[idx])
        {
            *out = LDG_MEM_ORD_LT;

            return LDG_ERR_AOK;
        }

        if (lhs_byte[idx] > rhs_byte[idx])
        {
            *out = LDG_MEM_ORD_GT;

            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_AOK;
}
