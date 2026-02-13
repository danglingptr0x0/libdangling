#include <stdint.h>

#include <dangling/mem/secure.h>
#include <dangling/core/err.h>

#define LDG_MEM_BARRIER __asm__ __volatile__ ("" ::: "memory")

void ldg_mem_secure_zero(void *ptr, uint64_t len)
{
    volatile uint8_t *p = 0x0;
    uint64_t i = 0;

    if (!ptr || len == 0) { return; }

    p = (volatile uint8_t *)ptr;

    LDG_MEM_BARRIER;

#if defined(__x86_64__) && defined(__GNUC__)
    __asm__ __volatile__ (
        "rep stosb"
        : "+D" (p), "+c" (len)
        : "a" (0)
        : "memory"
        );
#else
    for (i = 0; i < len; i++) { p[i] = 0; }
#endif

    LDG_MEM_BARRIER;
}

void ldg_mem_secure_copy(void *dst, const void *src, uint64_t len)
{
    volatile uint8_t *d = 0x0;
    volatile const uint8_t *s = 0x0;
    uint64_t i = 0;

    if (!dst || !src || len == 0) { return; }

    d = (volatile uint8_t *)dst;
    s = (volatile const uint8_t *)src;

    for (i = 0; i < len; i++) { d[i] = s[i]; }

    LDG_MEM_BARRIER;
}

uint32_t ldg_mem_secure_cmp(const void *a, const void *b, uint64_t len, uint32_t *result)
{
    volatile const uint8_t *pa = 0x0;
    volatile const uint8_t *pb = 0x0;
    volatile uint8_t acc = 0;
    uint64_t i = 0;

    if (LDG_UNLIKELY(!a || !b || !result)) { return LDG_ERR_FUNC_ARG_NULL; }

    *result = 0;

    if (len == 0) { return LDG_ERR_AOK; }

    pa = (volatile const uint8_t *)a;
    pb = (volatile const uint8_t *)b;

    LDG_MEM_BARRIER;

    for (i = 0; i < len; i++) { acc |= pa[i] ^ pb[i]; }

    LDG_MEM_BARRIER;

    acc |= acc >> 4;
    acc |= acc >> 2;
    acc |= acc >> 1;

    *result = (uint32_t)(acc & 1);

    return LDG_ERR_AOK;
}

void ldg_mem_secure_cmov(void *dst, const void *src, uint64_t len, uint8_t cond)
{
    volatile uint8_t *d = 0x0;
    volatile const uint8_t *s = 0x0;
    uint8_t mask = 0;
    uint64_t i = 0;

    if (!dst || !src || len == 0) { return; }

    d = (volatile uint8_t *)dst;
    s = (volatile const uint8_t *)src;

    LDG_MEM_BARRIER;

    mask = (uint8_t)(0 - (cond & 1));

    for (i = 0; i < len; i++) { d[i] ^= mask & (d[i] ^ s[i]); }

    LDG_MEM_BARRIER;
}

uint8_t ldg_mem_secure_neq_is(const void *a, const void *b, uint64_t len)
{
    volatile const uint8_t *pa = 0x0;
    volatile const uint8_t *pb = 0x0;
    volatile uint8_t acc = 0;
    uint64_t i = 0;

    if (!a || !b) { return LDG_TRUTH_TRUE; }

    if (len == 0) { return 0; }

    pa = (volatile const uint8_t *)a;
    pb = (volatile const uint8_t *)b;

    LDG_MEM_BARRIER;

    for (i = 0; i < len; i++) { acc |= pa[i] ^ pb[i]; }

    LDG_MEM_BARRIER;

    acc |= acc >> 4;
    acc |= acc >> 2;
    acc |= acc >> 1;

    return (uint8_t)(acc & 1);
}
