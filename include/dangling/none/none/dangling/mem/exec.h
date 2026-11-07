#ifndef LDG_MEM_EXEC_H
#define LDG_MEM_EXEC_H

#include <stdint.h>
#include <dangling/core/macros.h>

typedef enum ldg_mem_prot
{
    LDG_MEM_PROT_R = 0,
    LDG_MEM_PROT_RW,
    LDG_MEM_PROT_RX,
    LDG_MEM_PROT_CUNT
} ldg_mem_prot_t;

LDG_EXPORT uint32_t ldg_mem_exec_map(uint64_t size, void **out);
LDG_EXPORT uint32_t ldg_mem_exec_prot_set(void *addr, uint64_t size, ldg_mem_prot_t prot);
LDG_EXPORT uint32_t ldg_mem_exec_unmap(void *addr, uint64_t size);

#endif
