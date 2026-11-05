#include <stdint.h>

#include <dangling/mem/cpy.h>
#include <dangling/core/err.h>
#include <dangling/core/bits.h>
#include <dangling/core/macros.h>

static uint32_t mem_pattern_get(uint8_t val)
{
    uint32_t pattern = 0;
    uint32_t pos = 0;

    for (pos = 0; pos < LDG_WORD_BYTES; pos++) { pattern = (pattern << LDG_BYTE_BITS) | val; }

    return pattern;
}

static uint8_t* mem_word_end_get(uint8_t *byte, const uint8_t *end)
{
    return byte + ((uintptr_t)(end - byte) & ~(uintptr_t)LDG_WORD_MASK);
}

uint32_t ldg_mem_cpy(void *dst, const void *src, uint64_t len)
{
    uint8_t *dst_byte = 0x0;
    const uint8_t *src_byte = 0x0;
    const uint8_t *end = 0x0;
    const uint8_t *word_end = 0x0;

    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    dst_byte = (uint8_t *)dst;
    src_byte = (const uint8_t *)src;
    end = src_byte + len;

    if ((((uintptr_t)dst_byte ^ (uintptr_t)src_byte) & LDG_WORD_MASK) == 0)
    {
        while (src_byte < end && ((uintptr_t)src_byte & LDG_WORD_MASK) != 0)
        {
            *dst_byte = *src_byte;
            dst_byte++;
            src_byte++;
        }

        word_end = mem_word_end_get((uint8_t *)src_byte, end);

        while (src_byte < word_end)
        {
            *(uint32_t *)dst_byte = *(const uint32_t *)src_byte;
            dst_byte += LDG_WORD_BYTES;
            src_byte += LDG_WORD_BYTES;
        }
    }

    while (src_byte < end)
    {
        *dst_byte = *src_byte;
        dst_byte++;
        src_byte++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_mov(void *dst, const void *src, uint64_t len)
{
    uint8_t *dst_byte = 0x0;
    const uint8_t *src_byte = 0x0;

    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    dst_byte = (uint8_t *)dst;
    src_byte = (const uint8_t *)src;

    if (dst_byte < src_byte) { return ldg_mem_cpy(dst, src, len); }

    dst_byte += len;
    src_byte += len;

    while (src_byte > (const uint8_t *)src)
    {
        dst_byte--;
        src_byte--;
        *dst_byte = *src_byte;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_zero(void *ptr, uint64_t len)
{
    uint8_t *byte = 0x0;
    const uint8_t *end = 0x0;
    const uint8_t *word_end = 0x0;

    if (LDG_UNLIKELY(!ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    byte = (uint8_t *)ptr;
    end = byte + len;

    while (byte < end && ((uintptr_t)byte & LDG_WORD_MASK) != 0)
    {
        *byte = LDG_TRUTH_FALSE;
        byte++;
    }

    word_end = mem_word_end_get(byte, end);

    while (byte < word_end)
    {
        *(uint32_t *)byte = LDG_TRUTH_FALSE;
        byte += LDG_WORD_BYTES;
    }

    while (byte < end)
    {
        *byte = LDG_TRUTH_FALSE;
        byte++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_fill(void *ptr, uint8_t val, uint64_t len)
{
    uint8_t *byte = 0x0;
    const uint8_t *end = 0x0;
    const uint8_t *word_end = 0x0;
    uint32_t pattern = 0;

    if (LDG_UNLIKELY(!ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    byte = (uint8_t *)ptr;
    end = byte + len;

    while (byte < end && ((uintptr_t)byte & LDG_WORD_MASK) != 0)
    {
        *byte = val;
        byte++;
    }

    pattern = mem_pattern_get(val);
    word_end = mem_word_end_get(byte, end);

    while (byte < word_end)
    {
        *(uint32_t *)byte = pattern;
        byte += LDG_WORD_BYTES;
    }

    while (byte < end)
    {
        *byte = val;
        byte++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_cmp(const void *lhs, const void *rhs, uint64_t len, uint32_t *out)
{
    const uint8_t *lhs_byte = 0x0;
    const uint8_t *rhs_byte = 0x0;
    const uint8_t *end = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MEM_ORD_EQ;

    if (LDG_UNLIKELY(!lhs || !rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    lhs_byte = (const uint8_t *)lhs;
    rhs_byte = (const uint8_t *)rhs;
    end = lhs_byte + len;

    while (lhs_byte < end)
    {
        if (*lhs_byte < *rhs_byte)
        {
            *out = LDG_MEM_ORD_LT;

            return LDG_ERR_AOK;
        }

        if (*lhs_byte > *rhs_byte)
        {
            *out = LDG_MEM_ORD_GT;

            return LDG_ERR_AOK;
        }

        lhs_byte++;
        rhs_byte++;
    }

    return LDG_ERR_AOK;
}
