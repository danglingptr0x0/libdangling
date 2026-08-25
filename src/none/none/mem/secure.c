#include <stdint.h>

#include <dangling/mem/secure.h>
#include <dangling/core/err.h>

#define LDG_MEM_BARRIER __asm__ __volatile__ ("" ::: "memory")

uint32_t ldg_mem_secure_zero(void *ptr, uint64_t len)
{
    volatile uint8_t *ptr_byte = 0x0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!ptr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ptr_byte = (volatile uint8_t *)ptr;

    LDG_MEM_BARRIER;

#if defined(__x86_64__) && defined(__GNUC__)
    __asm__ __volatile__ (
        "rep stosb"
        : "+D" (ptr_byte), "+c" (len)
        : "a" (0)
        : "memory"
        );
#else
    for (idx = 0; idx < len; idx++) { ptr_byte[idx] = 0; }
#endif

    LDG_MEM_BARRIER;

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_secure_cpy(void *dst, const void *src, uint64_t len)
{
    volatile uint8_t *dst_byte = 0x0;
    volatile const uint8_t *src_byte = 0x0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dst_byte = (volatile uint8_t *)dst;
    src_byte = (volatile const uint8_t *)src;

    for (idx = 0; idx < len; idx++) { dst_byte[idx] = src_byte[idx]; }

    LDG_MEM_BARRIER;

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_secure_cmp(const void *lhs, const void *rhs, uint64_t len, uint32_t *out)
{
    volatile const uint8_t *lhs_byte = 0x0;
    volatile const uint8_t *rhs_byte = 0x0;
    volatile uint8_t cum = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (len == 0) { return LDG_ERR_AOK; }

    lhs_byte = (volatile const uint8_t *)lhs;
    rhs_byte = (volatile const uint8_t *)rhs;

    LDG_MEM_BARRIER;

    for (idx = 0; idx < len; idx++) { cum |= lhs_byte[idx] ^ rhs_byte[idx]; }

    LDG_MEM_BARRIER;

    cum |= cum >> 4;
    cum |= cum >> 2;
    cum |= cum >> 1;

    *out = (uint32_t)(cum & 1);

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_secure_cmov(void *dst, const void *src, uint64_t len, uint8_t cond)
{
    volatile uint8_t *dst_byte = 0x0;
    volatile const uint8_t *src_byte = 0x0;
    uint8_t mask = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dst_byte = (volatile uint8_t *)dst;
    src_byte = (volatile const uint8_t *)src;

    LDG_MEM_BARRIER;

    mask = (uint8_t)(0 - (cond & 1));

    for (idx = 0; idx < len; idx++) { dst_byte[idx] ^= mask & (dst_byte[idx] ^ src_byte[idx]); }

    LDG_MEM_BARRIER;

    return LDG_ERR_AOK;
}

uint8_t ldg_mem_secure_eq_is(const void *lhs, const void *rhs, uint64_t len)
{
    volatile const uint8_t *lhs_byte = 0x0;
    volatile const uint8_t *rhs_byte = 0x0;
    volatile uint8_t cum = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!lhs || !rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_TRUTH_TRUE; }

    lhs_byte = (volatile const uint8_t *)lhs;
    rhs_byte = (volatile const uint8_t *)rhs;

    LDG_MEM_BARRIER;

    for (idx = 0; idx < len; idx++) { cum |= lhs_byte[idx] ^ rhs_byte[idx]; }

    LDG_MEM_BARRIER;

    cum |= cum >> 4;
    cum |= cum >> 2;
    cum |= cum >> 1;

    return (cum & 1) ? LDG_TRUTH_FALSE : LDG_TRUTH_TRUE;
}
