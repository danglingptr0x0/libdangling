#ifndef LDG_SYS_MOD_H
#define LDG_SYS_MOD_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/sys/proc.h>

#define LDG_SYS_MOD_NAME_MAX 64
#define LDG_SYS_MOD_PATH_MAX 512

typedef struct ldg_sys_mod
{
    uint64_t base;
    uint64_t size;
    char name[LDG_SYS_MOD_NAME_MAX];
    char path[LDG_SYS_MOD_PATH_MAX];
} ldg_sys_mod_t;

LDG_EXPORT uint32_t ldg_sys_mod_list(const ldg_sys_proc_t *proc, ldg_sys_mod_t *out, uint64_t cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_sys_mod_find(const ldg_sys_proc_t *proc, const char *name, ldg_sys_mod_t *out);
LDG_EXPORT uint32_t ldg_sys_mod_addr_find(const ldg_sys_proc_t *proc, uint64_t addr, ldg_sys_mod_t *out);
LDG_EXPORT uint32_t ldg_sys_mod_sym_get(const ldg_sys_proc_t *proc, const ldg_sys_mod_t *mod, const char *name, uint64_t *out_addr);
LDG_EXPORT uint32_t ldg_sys_mod_pin(const ldg_sys_proc_t *proc, const ldg_sys_mod_t *mod);

#endif
