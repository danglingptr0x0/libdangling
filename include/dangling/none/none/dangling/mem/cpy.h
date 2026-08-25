#ifndef LDG_MEM_CPY_H
#define LDG_MEM_CPY_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_MEM_ORD_LT 0
#define LDG_MEM_ORD_EQ 1
#define LDG_MEM_ORD_GT 2

LDG_EXPORT uint32_t ldg_mem_cpy(void *dst, const void *src, uint64_t len);
LDG_EXPORT uint32_t ldg_mem_mov(void *dst, const void *src, uint64_t len);
LDG_EXPORT uint32_t ldg_mem_zero(void *ptr, uint64_t len);
LDG_EXPORT uint32_t ldg_mem_fill(void *ptr, uint8_t val, uint64_t len);
LDG_EXPORT uint32_t ldg_mem_cmp(const void *lhs, const void *rhs, uint64_t len, uint32_t *out);

#endif
