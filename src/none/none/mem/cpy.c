#include <stdint.h>

#include <dangling/mem/cpy.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

uint32_t ldg_mem_cpy(void *dst, const void *src, uint64_t len)
{
    uint8_t *dst_byte = 0x0;
    const uint8_t *src_byte = 0x0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    dst_byte = (uint8_t *)dst;
    src_byte = (const uint8_t *)src;

    for (idx = 0; idx < len; idx++) { dst_byte[idx] = src_byte[idx]; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_mov(void *dst, const void *src, uint64_t len)
{
    uint8_t *dst_byte = 0x0;
    const uint8_t *src_byte = 0x0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    dst_byte = (uint8_t *)dst;
    src_byte = (const uint8_t *)src;

    if (dst_byte < src_byte)
    {
        for (idx = 0; idx < len; idx++) { dst_byte[idx] = src_byte[idx]; }

        return LDG_ERR_AOK;
    }

    idx = len;
    while (idx > 0)
    {
        idx--;
        dst_byte[idx] = src_byte[idx];
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_zero(void *ptr, uint64_t len)
{
    uint8_t *byte = 0x0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    byte = (uint8_t *)ptr;

    for (idx = 0; idx < len; idx++) { byte[idx] = LDG_TRUTH_FALSE; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_fill(void *ptr, uint8_t val, uint64_t len)
{
    uint8_t *byte = 0x0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    byte = (uint8_t *)ptr;

    for (idx = 0; idx < len; idx++) { byte[idx] = val; }

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
