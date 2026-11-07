#include <stdint.h>
#include <signal.h>
#include <setjmp.h>
#include <ucontext.h>

#include <dangling/sys/guard.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>

#define LDG_SYS_GUARD_REG_RIP 16
#define LDG_SYS_GUARD_SIG_CUNT 4

typedef struct ldg_sys_guard_state
{
    ldg_sys_guard_t *live;
    struct sigaction prev[LDG_SYS_GUARD_SIG_CUNT];
} ldg_sys_guard_state_t;

static ldg_sys_guard_state_t g_sys_guard = { .live = 0x0 };

static int32_t sys_guard_sig_get(uint32_t idx)
{
    const int32_t sig[LDG_SYS_GUARD_SIG_CUNT] = { SIGSEGV, SIGILL, SIGFPE, SIGBUS };

    return sig[idx];
}

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

static void sys_guard_sig_cb(int32_t sig, siginfo_t *info, void *uc)
{
    ldg_sys_guard_t *guard = g_sys_guard.live;
    ucontext_t *state = (ucontext_t *)uc;
    const uint64_t *regs = 0x0;
    uint64_t at = 0;
    uint32_t slot = 0;

    if (!guard || guard->armed != LDG_TRUTH_TRUE || !state || !info || sig == 0) { return; }

    regs = (const uint64_t *)(const void *)&state->uc_mcontext;
    at = regs[LDG_SYS_GUARD_REG_RIP];

    if (sys_guard_range_hit_is(guard, at, &slot) != LDG_TRUTH_TRUE) { return; }

    guard->fault_addr = at;
    guard->fault_slot = slot;
    guard->fault_cunt++;
    guard->armed = LDG_TRUTH_FALSE;

    longjmp(guard->bail, LDG_SYS_GUARD_BAIL);
}

uint32_t ldg_sys_guard_init(ldg_sys_guard_t *guard)
{
    struct sigaction act = LDG_STRUCT_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t back = 0;

    if (LDG_UNLIKELY(!guard)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_sys_guard.live != 0x0)) { return LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(guard, sizeof(ldg_sys_guard_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    guard->fault_slot = LDG_SYS_GUARD_SLOT_NONE;

    act.sa_sigaction = sys_guard_sig_cb;
    act.sa_flags = SA_SIGINFO | SA_NODEFER;
    if (LDG_UNLIKELY(sigemptyset(&act.sa_mask) != 0)) { return LDG_ERR_DENIED; }

    g_sys_guard.live = guard;

    for (idx = 0; idx < LDG_SYS_GUARD_SIG_CUNT; idx++)
    {
        if (sigaction(sys_guard_sig_get(idx), &act, &g_sys_guard.prev[idx]) == 0) { continue; }

        for (back = 0; back < idx; back++) { if (sigaction(sys_guard_sig_get(back), &g_sys_guard.prev[back], 0x0) != 0) { break; } }

        g_sys_guard.live = 0x0;

        return LDG_ERR_DENIED;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_guard_shutdown(ldg_sys_guard_t *guard)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!guard)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_sys_guard.live != guard)) { return LDG_ERR_NOT_INIT; }

    guard->armed = LDG_TRUTH_FALSE;

    for (idx = 0; idx < LDG_SYS_GUARD_SIG_CUNT; idx++) { if (sigaction(sys_guard_sig_get(idx), &g_sys_guard.prev[idx], 0x0) != 0) { ret = LDG_ERR_DENIED; } }

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
