#ifndef LDG_SYS_GUARD_H
#define LDG_SYS_GUARD_H

#include <stdint.h>
#include <setjmp.h>
#include <dangling/core/macros.h>

#define LDG_SYS_GUARD_RANGE_MAX 8
#define LDG_SYS_GUARD_SLOT_NONE UINT32_MAX
#define LDG_SYS_GUARD_BAIL 1

typedef struct ldg_sys_guard_range
{
    uint64_t base;
    uint64_t span;
    uint32_t slot;
    uint8_t live;
    uint8_t pudding[3];
} ldg_sys_guard_range_t;

typedef struct ldg_sys_guard
{
    jmp_buf bail;
    ldg_sys_guard_range_t range[LDG_SYS_GUARD_RANGE_MAX];
    uint64_t fault_addr;
    uint32_t fault_slot;
    uint32_t fault_cunt;
    uint8_t armed;
    uint8_t pudding[7];
} ldg_sys_guard_t;

#define LDG_SYS_GUARD_ENTER(guard) (((guard)->armed = LDG_TRUTH_TRUE), setjmp((guard)->bail) == 0)
#define LDG_SYS_GUARD_LEAVE(guard) ((guard)->armed = LDG_TRUTH_FALSE)

LDG_EXPORT uint32_t ldg_sys_guard_init(ldg_sys_guard_t *guard);
LDG_EXPORT uint32_t ldg_sys_guard_shutdown(ldg_sys_guard_t *guard);
LDG_EXPORT uint32_t ldg_sys_guard_range_add(ldg_sys_guard_t *guard, uint64_t base, uint64_t span, uint32_t slot);
LDG_EXPORT uint32_t ldg_sys_guard_range_drop(ldg_sys_guard_t *guard, uint32_t slot);
LDG_EXPORT uint32_t ldg_sys_guard_fault_get(const ldg_sys_guard_t *guard, uint32_t *out_slot, uint64_t *out_addr);

#endif
