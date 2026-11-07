#include <stdint.h>
#include <setjmp.h>
#include <windows.h>

#include <dangling/sys/guard.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>

#define LDG_SYS_GUARD_VEH_FIRST 1

typedef struct ldg_sys_guard_state
{
    ldg_sys_guard_t *live;
    void *handle;
} ldg_sys_guard_state_t;

static ldg_sys_guard_state_t g_sys_guard = { .live = 0x0 };

static uint8_t sys_guard_range_hit_is(const ldg_sys_guard_t *guard, uint64_t at, uint32_t *out)
{
    uint32_t idx = 0;

    *out = LDG_SYS_GUARD_SLOT_NONE;

    for (idx = 0; idx < LDG_SYS_GUARD_RANGE_MAX; idx++)
    {
        if (guard->range[idx].live != LDG_TRUTH_TRUE) { continue; }

        if (at < guard->range[idx].base || at - guard->range[idx].base >= guard->range[idx].span) { continue; }

        *out = guard->range[idx].slot;

        return LDG_TRUTH_TRUE;
    }

    return LDG_TRUTH_FALSE;
}

static void sys_guard_land(void)
{
    longjmp(g_sys_guard.live->bail, LDG_SYS_GUARD_BAIL);
}

static LONG CALLBACK sys_guard_veh_cb(PEXCEPTION_POINTERS info)
{
    ldg_sys_guard_t *guard = g_sys_guard.live;
    uint64_t at = 0;
    uint32_t slot = 0;

    if (!guard || guard->armed != LDG_TRUTH_TRUE) { return EXCEPTION_CONTINUE_SEARCH; }

    if (!info || !info->ContextRecord || !info->ExceptionRecord) { return EXCEPTION_CONTINUE_SEARCH; }

    at = (uint64_t)info->ContextRecord->Rip;

    if (sys_guard_range_hit_is(guard, at, &slot) != LDG_TRUTH_TRUE) { return EXCEPTION_CONTINUE_SEARCH; }

    guard->fault_addr = at;
    guard->fault_slot = slot;
    guard->fault_cunt++;
    guard->armed = LDG_TRUTH_FALSE;

    info->ContextRecord->Rip = (DWORD64)(uintptr_t)sys_guard_land;

    return EXCEPTION_CONTINUE_EXECUTION;
}

uint32_t ldg_sys_guard_init(ldg_sys_guard_t *guard)
{
    if (LDG_UNLIKELY(!guard)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_sys_guard.live != 0x0)) { return LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(guard, sizeof(ldg_sys_guard_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    guard->fault_slot = LDG_SYS_GUARD_SLOT_NONE;
    g_sys_guard.live = guard;

    g_sys_guard.handle = AddVectoredExceptionHandler(LDG_SYS_GUARD_VEH_FIRST, sys_guard_veh_cb);
    if (LDG_UNLIKELY(!g_sys_guard.handle))
    {
        g_sys_guard.live = 0x0;
        return LDG_ERR_DENIED;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_guard_shutdown(ldg_sys_guard_t *guard)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!guard)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_sys_guard.live != guard)) { return LDG_ERR_NOT_INIT; }

    guard->armed = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(RemoveVectoredExceptionHandler(g_sys_guard.handle) == 0)) { ret = LDG_ERR_DENIED; }

    g_sys_guard.handle = 0x0;
    g_sys_guard.live = 0x0;

    return ret;
}

uint32_t ldg_sys_guard_range_add(ldg_sys_guard_t *guard, uint64_t base, uint64_t span, uint32_t slot)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!guard)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(span == 0 || slot == LDG_SYS_GUARD_SLOT_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < LDG_SYS_GUARD_RANGE_MAX; idx++) { if (guard->range[idx].live == LDG_TRUTH_TRUE && guard->range[idx].slot == slot) { return LDG_ERR_EXISTS; } }

    for (idx = 0; idx < LDG_SYS_GUARD_RANGE_MAX; idx++)
    {
        if (guard->range[idx].live == LDG_TRUTH_TRUE) { continue; }

        guard->range[idx].base = base;
        guard->range[idx].span = span;
        guard->range[idx].slot = slot;
        guard->range[idx].live = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_FULL;
}

uint32_t ldg_sys_guard_range_drop(ldg_sys_guard_t *guard, uint32_t slot)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!guard)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < LDG_SYS_GUARD_RANGE_MAX; idx++)
    {
        if (guard->range[idx].live != LDG_TRUTH_TRUE || guard->range[idx].slot != slot) { continue; }

        guard->range[idx].live = LDG_TRUTH_FALSE;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_sys_guard_fault_get(const ldg_sys_guard_t *guard, uint32_t *out_slot, uint64_t *out_addr)
{
    if (LDG_UNLIKELY(!guard || !out_slot || !out_addr)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_slot = guard->fault_slot;
    *out_addr = guard->fault_addr;

    if (guard->fault_cunt == 0) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}
