#ifndef LDG_MEM_REGION_H
#define LDG_MEM_REGION_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/sys/proc.h>

typedef enum ldg_mem_region_kind
{
    LDG_MEM_REGION_KIND_NONE = 0,
    LDG_MEM_REGION_KIND_PRIVATE,
    LDG_MEM_REGION_KIND_MAPPED,
    LDG_MEM_REGION_KIND_IMG,
    LDG_MEM_REGION_KIND_CUNT
} ldg_mem_region_kind_t;

typedef struct ldg_mem_region
{
    uint64_t base;
    uint64_t size;
    uint64_t alloc_base;
    uint8_t committed;
    uint8_t readable;
    uint8_t writable;
    uint8_t executable;
    uint8_t guard;
    uint8_t kind;
    uint8_t pudding[2];
} ldg_mem_region_t;

LDG_EXPORT uint32_t ldg_mem_region_get(const ldg_sys_proc_t *proc, uint64_t addr, ldg_mem_region_t *out);
LDG_EXPORT uint32_t ldg_mem_region_list(const ldg_sys_proc_t *proc, uint64_t addr, ldg_mem_region_t *out, uint64_t cap, uint64_t *out_cunt);

#endif
