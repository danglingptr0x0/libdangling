#ifndef LDG_MEM_SECURE_H
#define LDG_MEM_SECURE_H

#include <stdint.h>
#include <stddef.h>
#include <dangling/core/macros.h>

LDG_EXPORT void ldg_mem_secure_zero(void *ptr, size_t len);
LDG_EXPORT void ldg_mem_secure_copy(void *dst, const void *src, size_t len);
LDG_EXPORT int32_t ldg_mem_secure_cmp(const void *a, const void *b, size_t len);
LDG_EXPORT void ldg_mem_secure_cmov(void *dst, const void *src, size_t len, int cond);
LDG_EXPORT int32_t ldg_mem_secure_neq(const void *a, const void *b, size_t len);

#endif
