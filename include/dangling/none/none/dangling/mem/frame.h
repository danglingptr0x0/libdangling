#ifndef LDG_MEM_FRAME_H
#define LDG_MEM_FRAME_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_MEM_FRAME_WIDTH 0x1000
#define LDG_MEM_EXTENT_MAX 16

typedef enum ldg_mem_extent_kind
{
    LDG_MEM_EXTENT_KIND_NONE,
    LDG_MEM_EXTENT_KIND_SYSMEM,
    LDG_MEM_EXTENT_KIND_RESERVED
} ldg_mem_extent_kind_t;

typedef struct ldg_mem_extent
{
    uint64_t base;
    uint64_t width;
    uint32_t kind;
    uint8_t pudding[4];
} ldg_mem_extent_t;

LDG_EXPORT uint32_t ldg_mem_frame_init(const ldg_mem_extent_t *extent, uint32_t cunt);
LDG_EXPORT uint32_t ldg_mem_frame_shutdown(void);

LDG_EXPORT uint32_t ldg_mem_frame_alloc(uint64_t cunt, uint64_t *out);
LDG_EXPORT uint32_t ldg_mem_frame_dealloc(uint64_t addr);

LDG_EXPORT uint64_t ldg_mem_frame_run_cunt_get(uint64_t addr);
LDG_EXPORT uint64_t ldg_mem_frame_free_cunt_get(void);
LDG_EXPORT uint64_t ldg_mem_frame_total_cunt_get(void);

#endif
