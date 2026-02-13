#ifndef LDG_MEM_SECURE_H
#define LDG_MEM_SECURE_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT void ldg_mem_secure_zero(void *ptr, uint64_t len);
LDG_EXPORT void ldg_mem_secure_copy(void *dst, const void *src, uint64_t len);
LDG_EXPORT uint32_t ldg_mem_secure_cmp(const void *a, const void *b, uint64_t len, uint32_t *result);
LDG_EXPORT void ldg_mem_secure_cmov(void *dst, const void *src, uint64_t len, uint8_t cond);
LDG_EXPORT uint8_t ldg_mem_secure_neq_is(const void *a, const void *b, uint64_t len);

#endif
