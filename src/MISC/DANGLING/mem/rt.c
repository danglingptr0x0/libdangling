#include <stdint.h>

#include <dangling/mem/rt.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/err.h>
#include <dangling/core/bits.h>
#include <dangling/core/macros.h>

#define LDG_MEM_RT_ORD_BELOW (-1)
#define LDG_MEM_RT_ORD_SAME 0
#define LDG_MEM_RT_ORD_ABOVE 1

void* ldg_mem_rt_cpy(void *dst, const void *src, uint32_t len)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!dst || !src)) { return 0x0; }

    ret = ldg_mem_cpy(dst, src, (uint64_t)len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return 0x0; }

    return dst;
}

void* ldg_mem_rt_mov(void *dst, const void *src, uint32_t len)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!dst || !src)) { return 0x0; }

    ret = ldg_mem_mov(dst, src, (uint64_t)len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return 0x0; }

    return dst;
}

void* ldg_mem_rt_fill(void *dst, int32_t val, uint32_t len)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!dst)) { return 0x0; }

    ret = ldg_mem_fill(dst, (uint8_t)((uint32_t)val & LDG_BYTE_MASK), (uint64_t)len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return 0x0; }

    return dst;
}

int32_t ldg_mem_rt_cmp(const void *lhs, const void *rhs, uint32_t len)
{
    uint32_t ord = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!lhs || !rhs)) { return LDG_MEM_RT_ORD_SAME; }

    ret = ldg_mem_cmp(lhs, rhs, (uint64_t)len, &ord);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_MEM_RT_ORD_SAME; }

    if (ord == LDG_MEM_ORD_LT) { return LDG_MEM_RT_ORD_BELOW; }

    if (ord == LDG_MEM_ORD_GT) { return LDG_MEM_RT_ORD_ABOVE; }

    return LDG_MEM_RT_ORD_SAME;
}
