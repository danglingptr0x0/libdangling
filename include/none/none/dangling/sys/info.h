#ifndef LDG_SYS_INFO_H
#define LDG_SYS_INFO_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT uint32_t ldg_sys_hostname_get(char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_sys_cpu_cunt_get(uint32_t *cunt);
LDG_EXPORT uint32_t ldg_sys_page_size_get(uint64_t *size);
LDG_EXPORT uint32_t ldg_sys_env_get(const char *name, char *buff, uint64_t buff_size);
LDG_EXPORT uint32_t ldg_sys_pid_get(uint64_t *pid);

#endif
