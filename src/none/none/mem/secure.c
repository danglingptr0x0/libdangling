#include <stdint.h>

#include <dangling/mem/secure.h>
#include <dangling/core/err.h>
#include <dangling/core/bits.h>
#include <dangling/core/macros.h>

#define LDG_MEM_BARRIER __asm__ __volatile__ ("" ::: "memory")
#define LDG_MEM_SECURE_COND_MASK 0x1U
#define LDG_MEM_SECURE_SCRUB_BYTE 0

uint32_t ldg_mem_secure_zero(void *ptr, uint64_t len)
{
    volatile uint8_t *ptr_byte = 0x0;
    volatile uint8_t *end = 0x0;

    if (LDG_UNLIKELY(!ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ptr_byte = (volatile uint8_t *)ptr;

    LDG_MEM_BARRIER;

#if defined(__x86_64__) && defined(__GNUC__)
    __asm__ __volatile__ (
        "rep stosb"
        : "+D" (ptr_byte), "+c" (len)
        : "a" (LDG_MEM_SECURE_SCRUB_BYTE)
        : "memory"
        );
#else
    end = ptr_byte + len;

    while (ptr_byte < end)
    {
        *ptr_byte = LDG_MEM_SECURE_SCRUB_BYTE;
        ptr_byte++;
    }
#endif

    LDG_MEM_BARRIER;

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_secure_cpy(void *dst, const void *src, uint64_t len)
{
    volatile uint8_t *dst_byte = 0x0;
    volatile const uint8_t *src_byte = 0x0;
    volatile const uint8_t *end = 0x0;

    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dst_byte = (volatile uint8_t *)dst;
    src_byte = (volatile const uint8_t *)src;
    end = src_byte + len;

    while (src_byte < end)
    {
        *dst_byte = *src_byte;
        dst_byte++;
        src_byte++;
    }

    LDG_MEM_BARRIER;

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_secure_cmp(const void *lhs, const void *rhs, uint64_t len, uint32_t *out)
{
    volatile const uint8_t *lhs_byte = 0x0;
    volatile const uint8_t *rhs_byte = 0x0;
    volatile const uint8_t *end = 0x0;
    volatile uint32_t cum = 0;

    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_TRUTH_FALSE;

    if (len == 0) { return LDG_ERR_AOK; }

    lhs_byte = (volatile const uint8_t *)lhs;
    rhs_byte = (volatile const uint8_t *)rhs;
    end = lhs_byte + len;

    LDG_MEM_BARRIER;

    while (lhs_byte < end)
    {
        cum |= (uint32_t)(*lhs_byte ^ *rhs_byte);
        lhs_byte++;
        rhs_byte++;
    }

    LDG_MEM_BARRIER;

    *out = (cum + LDG_BYTE_MASK) >> LDG_BYTE_BITS;

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_secure_cmov(void *dst, const void *src, uint64_t len, uint8_t cond)
{
    volatile uint8_t *dst_byte = 0x0;
    volatile const uint8_t *src_byte = 0x0;
    volatile const uint8_t *end = 0x0;
    uint8_t bit = 0;
    uint8_t mask = 0;

    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dst_byte = (volatile uint8_t *)dst;
    src_byte = (volatile const uint8_t *)src;
    end = src_byte + len;

    LDG_MEM_BARRIER;

    bit = (uint8_t)(cond & LDG_MEM_SECURE_COND_MASK);
    mask = (uint8_t)(~(uint8_t)(bit - LDG_MEM_SECURE_COND_MASK));

    while (src_byte < end)
    {
        *dst_byte ^= mask & (uint8_t)(*dst_byte ^ *src_byte);
        dst_byte++;
        src_byte++;
    }

    LDG_MEM_BARRIER;

    return LDG_ERR_AOK;
}

uint8_t ldg_mem_secure_eq_is(const void *lhs, const void *rhs, uint64_t len)
{
    volatile const uint8_t *lhs_byte = 0x0;
    volatile const uint8_t *rhs_byte = 0x0;
    volatile const uint8_t *end = 0x0;
    volatile uint32_t cum = 0;

    if (LDG_UNLIKELY(!lhs || !rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_TRUTH_TRUE; }

    lhs_byte = (volatile const uint8_t *)lhs;
    rhs_byte = (volatile const uint8_t *)rhs;
    end = lhs_byte + len;

    LDG_MEM_BARRIER;

    while (lhs_byte < end)
    {
        cum |= (uint32_t)(*lhs_byte ^ *rhs_byte);
        lhs_byte++;
        rhs_byte++;
    }

    LDG_MEM_BARRIER;

    return (uint8_t)(LDG_TRUTH_TRUE - ((cum + LDG_BYTE_MASK) >> LDG_BYTE_BITS));
}
