#ifndef LDG_MEM_RT_H
#define LDG_MEM_RT_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT void* ldg_mem_rt_cpy(void *dst, const void *src, uint64_t len);
LDG_EXPORT void* ldg_mem_rt_mov(void *dst, const void *src, uint64_t len);
LDG_EXPORT void* ldg_mem_rt_fill(void *dst, int32_t val, uint64_t len);
LDG_EXPORT int32_t ldg_mem_rt_cmp(const void *lhs, const void *rhs, uint64_t len);

#endif
