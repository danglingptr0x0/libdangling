#include <dangling/mem/secure.h>
#include <dangling/core/err.h>

#define LDG_MEM_BARRIER __asm__ __volatile__ ("" ::: "memory")

void ldg_mem_secure_zero(void *ptr, size_t len)
{
    volatile uint8_t *p = 0x0;
    size_t i = 0;

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

void ldg_mem_secure_copy(void *dst, const void *src, size_t len)
{
    volatile uint8_t *d = 0x0;
    volatile const uint8_t *s = 0x0;
    size_t i = 0;

    if (!dst || !src || len == 0) { return; }

    d = (volatile uint8_t *)dst;
    s = (volatile const uint8_t *)src;

    for (i = 0; i < len; i++) { d[i] = s[i]; }

    LDG_MEM_BARRIER;
}

uint32_t ldg_mem_secure_cmp(const void *a, const void *b, size_t len)
{
    volatile const uint8_t *pa = 0x0;
    volatile const uint8_t *pb = 0x0;
    volatile uint8_t acc = 0;
    size_t i = 0;

    if (LDG_UNLIKELY(!a || !b)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return 0; }

    pa = (volatile const uint8_t *)a;
    pb = (volatile const uint8_t *)b;

    LDG_MEM_BARRIER;

    for (i = 0; i < len; i++) { acc |= pa[i] ^ pb[i]; }

    LDG_MEM_BARRIER;

    acc |= acc >> 4;
    acc |= acc >> 2;
    acc |= acc >> 1;

    return (uint32_t)(acc & 1);
}

void ldg_mem_secure_cmov(void *dst, const void *src, size_t len, int cond)
{
    volatile uint8_t *d = 0x0;
    volatile const uint8_t *s = 0x0;
    uint8_t mask = 0;
    size_t i = 0;

    if (!dst || !src || len == 0) { return; }

    d = (volatile uint8_t *)dst;
    s = (volatile const uint8_t *)src;

    LDG_MEM_BARRIER;

    mask = (uint8_t)(-(int8_t)(cond & 1));

    for (i = 0; i < len; i++) { d[i] ^= mask & (d[i] ^ s[i]); }

    LDG_MEM_BARRIER;
}

uint32_t ldg_mem_secure_neq_is(const void *a, const void *b, size_t len)
{
    volatile const uint8_t *pa = 0x0;
    volatile const uint8_t *pb = 0x0;
    volatile uint8_t acc = 0;
    size_t i = 0;

    if (!a || !b) { return (uint32_t) !0; }

    if (len == 0) { return 0; }

    pa = (volatile const uint8_t *)a;
    pb = (volatile const uint8_t *)b;

    LDG_MEM_BARRIER;

    for (i = 0; i < len; i++) { acc |= pa[i] ^ pb[i]; }

    LDG_MEM_BARRIER;

    return (acc != 0) ? 1 : 0;
}
