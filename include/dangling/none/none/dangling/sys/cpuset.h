#ifndef LDG_SYS_CPUSET_H
#define LDG_SYS_CPUSET_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_SYS_CPUSET_MAX 64

typedef struct ldg_sys_cpuset
{
    uint32_t id;
    uint16_t group;
    uint8_t logical_proc_idx;
    uint8_t core_idx;
    uint8_t cache_idx;
    uint8_t numa_idx;
    uint8_t efficiency_class;
    uint8_t pudding[5];
} ldg_sys_cpuset_t;

LDG_EXPORT uint32_t ldg_sys_cpuset_list(ldg_sys_cpuset_t *out, uint64_t cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_sys_cpuset_proc_default_set(const uint32_t *id, uint32_t cunt);
LDG_EXPORT uint32_t ldg_sys_cpuset_thread_self_set(const uint32_t *id, uint32_t cunt);

#endif
