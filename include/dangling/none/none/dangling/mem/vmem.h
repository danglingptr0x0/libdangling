#ifndef LDG_MEM_VMEM_H
#define LDG_MEM_VMEM_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/sys/proc.h>

LDG_EXPORT uint32_t ldg_mem_vmem_rd(const ldg_sys_proc_t *proc, uint64_t addr, void *dst, uint64_t len, uint64_t *out_got);
LDG_EXPORT uint32_t ldg_mem_vmem_wr(const ldg_sys_proc_t *proc, uint64_t addr, const void *src, uint64_t len, uint64_t *out_put);
LDG_EXPORT uint32_t ldg_mem_vmem_flush(const ldg_sys_proc_t *proc, uint64_t addr, uint64_t len);

#endif
