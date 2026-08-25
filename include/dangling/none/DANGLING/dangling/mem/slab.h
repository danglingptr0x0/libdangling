#ifndef LDG_MEM_SLAB_H
#define LDG_MEM_SLAB_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_MEM_SLAB_MIN_WIDTH 16
#define LDG_MEM_SLAB_MAX_WIDTH 2048

LDG_EXPORT uint32_t ldg_mem_slab_alloc(uint64_t size, void **out);
LDG_EXPORT uint32_t ldg_mem_slab_dealloc(void *ptr);
LDG_EXPORT uint8_t ldg_mem_slab_owned_is(const void *ptr);
LDG_EXPORT uint8_t ldg_mem_slab_serves_is(uint64_t size);
LDG_EXPORT uint64_t ldg_mem_slab_size_get(const void *ptr);
LDG_EXPORT uint64_t ldg_mem_slab_used_cunt_get(void);

#endif
