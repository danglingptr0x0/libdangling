#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

#include <dangling/thread/trap.h>
#include <dangling/arch/amd64/syscall.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>

#include "uc.h"

#define LDG_THREAD_TRAP_BREAK_MAX 64
#define LDG_THREAD_TRAP_SIG_CUNT 2
#define LDG_THREAD_TRAP_SIG_STEP_IDX 0
#define LDG_THREAD_TRAP_SIG_ILLEGAL_IDX 1
#define LDG_THREAD_TRAP_CODE_STEP 2
#define LDG_THREAD_TRAP_FD_NONE (-1)
#define LDG_THREAD_TRAP_CPU_ANY (-1)
#define LDG_THREAD_TRAP_GROUP_NONE (-1)
#define LDG_THREAD_TRAP_EVERY_HIT 1
#define LDG_THREAD_TRAP_EXEC_LEN 8

typedef struct ldg_thread_trap_break
{
    uint64_t id;
    uint64_t addr;
    int32_t fd;
    uint32_t slot;
    uint8_t live;
    uint8_t pudding[7];
} ldg_thread_trap_break_t;

typedef struct ldg_thread_trap_state
{
    ldg_thread_trap_cb_t cb;
    void *user;
    struct sigaction prev[LDG_THREAD_TRAP_SIG_CUNT];
    ldg_thread_trap_break_t armed[LDG_THREAD_TRAP_BREAK_MAX];
} ldg_thread_trap_state_t;

static ldg_thread_trap_state_t g_thread_trap = { .cb = 0x0 };

static int32_t thread_trap_sig_get(uint32_t idx)
{
    const int32_t sig[LDG_THREAD_TRAP_SIG_CUNT] = { SIGTRAP, SIGILL };

    return sig[idx];
}

static uint32_t thread_trap_slot_find(uint64_t id, uint64_t addr)
{
    uint32_t idx = 0;

    for (idx = 0; idx < LDG_THREAD_TRAP_BREAK_MAX; idx++) { if (g_thread_trap.armed[idx].live == LDG_TRUTH_TRUE && g_thread_trap.armed[idx].id == id && g_thread_trap.armed[idx].addr == addr) { return g_thread_trap.armed[idx].slot; } }

    return LDG_THREAD_TRAP_SLOT_NONE;
}

static void thread_trap_prev_run(uint32_t sig_idx, int32_t sig, siginfo_t *info, void *uc)
{
    const struct sigaction *prev = &g_thread_trap.prev[sig_idx];

    if ((prev->sa_flags & SA_SIGINFO) != 0 && prev->sa_sigaction)
    {
        prev->sa_sigaction(sig, info, uc);
        return;
    }

    if (prev->sa_handler == SIG_IGN) { return; }

    if (prev->sa_handler != SIG_DFL)
    {
        prev->sa_handler(sig);
        return;
    }

    if (sigaction(sig, prev, 0x0) != 0) { _exit(sig); }
}

static void thread_trap_sig_cb(int32_t sig, siginfo_t *info, void *uc)
{
    ldg_thread_regs_t regs = LDG_STRUCT_ZERO_INIT;
    ldg_thread_trap_cb_t cb = g_thread_trap.cb;
    ldg_thread_trap_kind_t kind = LDG_THREAD_TRAP_ILLEGAL;
    uint64_t self = 0;
    uint32_t sig_idx = LDG_THREAD_TRAP_SIG_ILLEGAL_IDX;
    uint32_t slot = LDG_THREAD_TRAP_SLOT_NONE;

    if (!info || !uc) { return; }

    self = (uint64_t)ldg_syscall0(LDG_SYS_GETTID);
    thread_uc_regs_ld(uc, &regs);

    if (sig == SIGTRAP)
    {
        sig_idx = LDG_THREAD_TRAP_SIG_STEP_IDX;
        kind = LDG_THREAD_TRAP_STEP;
        slot = thread_trap_slot_find(self, regs.rip);

        if (slot != LDG_THREAD_TRAP_SLOT_NONE) { kind = LDG_THREAD_TRAP_BREAK; }
        else if (info->si_code != LDG_THREAD_TRAP_CODE_STEP) { cb = 0x0; }
    }

    if (!cb || cb(kind, self, slot, &regs, g_thread_trap.user) != LDG_ERR_AOK)
    {
        thread_trap_prev_run(sig_idx, sig, info, uc);
        return;
    }

    if (kind == LDG_THREAD_TRAP_BREAK) { regs.rflags |= LDG_THREAD_TRAP_RFLAGS_RESUME; }

    thread_uc_regs_st(uc, &regs);
}

uint32_t ldg_thread_trap_init(ldg_thread_trap_cb_t cb, void *user)
{
    struct sigaction act = LDG_STRUCT_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t back = 0;

    if (LDG_UNLIKELY(!cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_thread_trap.cb != 0x0)) { return LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(g_thread_trap.armed, sizeof(g_thread_trap.armed)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    for (idx = 0; idx < LDG_THREAD_TRAP_BREAK_MAX; idx++) { g_thread_trap.armed[idx].fd = LDG_THREAD_TRAP_FD_NONE; }

    act.sa_sigaction = thread_trap_sig_cb;
    act.sa_flags = SA_SIGINFO | SA_NODEFER;
    if (LDG_UNLIKELY(sigemptyset(&act.sa_mask) != 0)) { return LDG_ERR_DENIED; }

    g_thread_trap.user = user;
    g_thread_trap.cb = cb;

    for (idx = 0; idx < LDG_THREAD_TRAP_SIG_CUNT; idx++)
    {
        if (sigaction(thread_trap_sig_get(idx), &act, &g_thread_trap.prev[idx]) == 0) { continue; }

        for (back = 0; back < idx; back++) { if (sigaction(thread_trap_sig_get(back), &g_thread_trap.prev[back], 0x0) != 0) { break; } }

        g_thread_trap.cb = 0x0;

        return LDG_ERR_DENIED;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_trap_shutdown(void)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(g_thread_trap.cb == 0x0)) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < LDG_THREAD_TRAP_BREAK_MAX; idx++)
    {
        if (g_thread_trap.armed[idx].live != LDG_TRUTH_TRUE) { continue; }

        g_thread_trap.armed[idx].live = LDG_TRUTH_FALSE;
        if (close(g_thread_trap.armed[idx].fd) != 0) { ret = LDG_ERR_IO_CLOSE; }
    }

    for (idx = 0; idx < LDG_THREAD_TRAP_SIG_CUNT; idx++) { if (sigaction(thread_trap_sig_get(idx), &g_thread_trap.prev[idx], 0x0) != 0) { ret = LDG_ERR_DENIED; } }

    g_thread_trap.cb = 0x0;
    g_thread_trap.user = 0x0;

    return ret;
}

uint32_t ldg_thread_trap_break_arm(ldg_thread_ref_t *thread, uint32_t slot, uint64_t addr)
{
    struct perf_event_attr attr = LDG_STRUCT_ZERO_INIT;
    ldg_thread_trap_break_t *entry = 0x0;
    int64_t fd = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!thread)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(slot >= LDG_THREAD_TRAP_SLOT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(thread->outside == LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(g_thread_trap.cb == 0x0 || thread->live != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < LDG_THREAD_TRAP_BREAK_MAX; idx++)
    {
        if (g_thread_trap.armed[idx].live == LDG_TRUTH_TRUE && g_thread_trap.armed[idx].id == thread->id && g_thread_trap.armed[idx].slot == slot) { return LDG_ERR_EXISTS; }

        if (!entry && g_thread_trap.armed[idx].live != LDG_TRUTH_TRUE) { entry = &g_thread_trap.armed[idx]; }
    }

    if (LDG_UNLIKELY(!entry)) { return LDG_ERR_FULL; }

    attr.type = PERF_TYPE_BREAKPOINT;
    attr.size = sizeof(struct perf_event_attr);
    attr.bp_type = HW_BREAKPOINT_X;
    attr.bp_addr = addr;
    attr.bp_len = LDG_THREAD_TRAP_EXEC_LEN;
    attr.sample_period = LDG_THREAD_TRAP_EVERY_HIT;
    attr.exclude_kernel = LDG_TRUTH_TRUE;
    attr.exclude_hv = LDG_TRUTH_TRUE;
    attr.remove_on_exec = LDG_TRUTH_TRUE;
    attr.sigtrap = LDG_TRUTH_TRUE;

    fd = ldg_syscall5(LDG_SYS_PERF_EVENT_OPEN, (int64_t)(uintptr_t)&attr, (int64_t)thread->id, LDG_THREAD_TRAP_CPU_ANY, LDG_THREAD_TRAP_GROUP_NONE, PERF_FLAG_FD_CLOEXEC);
    if (fd < 0)
    {
        if (fd + (int64_t)EACCES == 0 || fd + (int64_t)EPERM == 0) { return LDG_ERR_DENIED; }

        if (fd + (int64_t)ENOSPC == 0 || fd + (int64_t)EBUSY == 0) { return LDG_ERR_FULL; }

        if (fd + (int64_t)ESRCH == 0) { return LDG_ERR_NOT_FOUND; }

        return LDG_ERR_UNSUPPORTED;
    }

    entry->id = thread->id;
    entry->addr = addr;
    entry->fd = (int32_t)fd;
    entry->slot = slot;
    entry->live = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_trap_break_disarm(ldg_thread_ref_t *thread, uint32_t slot)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!thread)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(slot >= LDG_THREAD_TRAP_SLOT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < LDG_THREAD_TRAP_BREAK_MAX; idx++)
    {
        if (g_thread_trap.armed[idx].live != LDG_TRUTH_TRUE || g_thread_trap.armed[idx].id != thread->id || g_thread_trap.armed[idx].slot != slot) { continue; }

        g_thread_trap.armed[idx].live = LDG_TRUTH_FALSE;

        if (LDG_UNLIKELY(close(g_thread_trap.armed[idx].fd) != 0)) { return LDG_ERR_IO_CLOSE; }

        g_thread_trap.armed[idx].fd = LDG_THREAD_TRAP_FD_NONE;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}
