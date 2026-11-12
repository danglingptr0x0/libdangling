#include <stdint.h>
#include <windows.h>

#include <dangling/thread/trap.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#include "ctx.h"

#define LDG_THREAD_TRAP_VEH_FIRST 1
#define LDG_THREAD_TRAP_DR7_EN_STRIDE 2
#define LDG_THREAD_TRAP_DR7_COND_BASE 16
#define LDG_THREAD_TRAP_DR7_COND_STRIDE 4
#define LDG_THREAD_TRAP_DR7_COND_MASK 0xFULL
#define LDG_THREAD_TRAP_DR6_CLEAR 0
#define LDG_THREAD_TRAP_ADDR_NONE 0

typedef enum ldg_thread_trap_dr
{
    LDG_THREAD_TRAP_DR0 = 0,
    LDG_THREAD_TRAP_DR1,
    LDG_THREAD_TRAP_DR2,
    LDG_THREAD_TRAP_DR3,
    LDG_THREAD_TRAP_DR_CUNT
} ldg_thread_trap_dr_t;

typedef struct ldg_thread_trap_state
{
    ldg_thread_trap_cb_t cb;
    void *user;
    void *veh;
} ldg_thread_trap_state_t;

static ldg_thread_trap_state_t g_thread_trap = { .cb = 0x0 };

static uint64_t thread_trap_en_bit_get(uint32_t slot)
{
    return (uint64_t)LDG_BIT(slot * LDG_THREAD_TRAP_DR7_EN_STRIDE);
}

static void thread_trap_dr_set(CONTEXT *ctx, uint32_t slot, uint64_t addr)
{
    if (slot == LDG_THREAD_TRAP_DR0) { ctx->Dr0 = (DWORD64)addr; }
    else if (slot == LDG_THREAD_TRAP_DR1) { ctx->Dr1 = (DWORD64)addr; }
    else if (slot == LDG_THREAD_TRAP_DR2) { ctx->Dr2 = (DWORD64)addr; }
    else { ctx->Dr3 = (DWORD64)addr; }
}

static LONG CALLBACK thread_trap_veh_cb(EXCEPTION_POINTERS *info)
{
    ldg_thread_regs_t regs = LDG_STRUCT_ZERO_INIT;
    ldg_thread_trap_cb_t cb = g_thread_trap.cb;
    ldg_thread_trap_kind_t kind = LDG_THREAD_TRAP_ILLEGAL;
    CONTEXT *ctx = 0x0;
    DWORD code = 0;
    uint32_t slot = LDG_THREAD_TRAP_SLOT_NONE;
    uint32_t idx = 0;

    if (!cb || !info || !info->ExceptionRecord || !info->ContextRecord) { return EXCEPTION_CONTINUE_SEARCH; }

    ctx = info->ContextRecord;
    code = info->ExceptionRecord->ExceptionCode;

    if (code != EXCEPTION_ILLEGAL_INSTRUCTION && code != EXCEPTION_SINGLE_STEP) { return EXCEPTION_CONTINUE_SEARCH; }

    if (code == EXCEPTION_SINGLE_STEP)
    {
        kind = LDG_THREAD_TRAP_STEP;

        for (idx = 0; idx < LDG_THREAD_TRAP_SLOT_MAX; idx++)
        {
            if ((ctx->Dr6 & (DWORD64)LDG_BIT(idx)) == 0 || (ctx->Dr7 & (DWORD64)thread_trap_en_bit_get(idx)) == 0) { continue; }

            kind = LDG_THREAD_TRAP_BREAK;
            slot = idx;
            break;
        }
    }

    thread_ctx_regs_ld(ctx, &regs);

    if (cb(kind, (uint64_t)GetCurrentThreadId(), slot, &regs, g_thread_trap.user) != LDG_ERR_AOK) { return EXCEPTION_CONTINUE_SEARCH; }

    if (kind == LDG_THREAD_TRAP_BREAK) { regs.rflags |= LDG_THREAD_TRAP_RFLAGS_RESUME; }

    thread_ctx_regs_st(ctx, &regs);
    ctx->Dr6 = LDG_THREAD_TRAP_DR6_CLEAR;

    return EXCEPTION_CONTINUE_EXECUTION;
}

static uint32_t thread_trap_break_apply(ldg_thread_ref_t *thread, uint32_t slot, uint64_t addr, uint8_t en)
{
    CONTEXT ctx = LDG_STRUCT_ZERO_INIT;
    uint64_t en_bit = 0;
    uint64_t cond = 0;

    if (LDG_UNLIKELY(thread->outside == LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(g_thread_trap.cb == 0x0 || thread->live != LDG_TRUTH_TRUE || thread->suspended != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (LDG_UNLIKELY(!GetThreadContext((HANDLE)(uintptr_t)thread->handle, &ctx))) { return LDG_ERR_DENIED; }

    en_bit = thread_trap_en_bit_get(slot);
    cond = LDG_THREAD_TRAP_DR7_COND_MASK << (LDG_THREAD_TRAP_DR7_COND_BASE + slot * LDG_THREAD_TRAP_DR7_COND_STRIDE);

    if (en == LDG_TRUTH_TRUE && (ctx.Dr7 & (DWORD64)en_bit) != 0) { return LDG_ERR_EXISTS; }

    if (en != LDG_TRUTH_TRUE && (ctx.Dr7 & (DWORD64)en_bit) == 0) { return LDG_ERR_NOT_FOUND; }

    thread_trap_dr_set(&ctx, slot, addr);
    ctx.Dr7 &= ~(DWORD64)cond;

    if (en == LDG_TRUTH_TRUE) { ctx.Dr7 |= (DWORD64)en_bit; }
    else { ctx.Dr7 &= ~(DWORD64)en_bit; }

    if (LDG_UNLIKELY(!SetThreadContext((HANDLE)(uintptr_t)thread->handle, &ctx))) { return LDG_ERR_DENIED; }

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_trap_init(ldg_thread_trap_cb_t cb, void *user)
{
    if (LDG_UNLIKELY(!cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(g_thread_trap.cb != 0x0)) { return LDG_ERR_BUSY; }

    g_thread_trap.user = user;
    g_thread_trap.cb = cb;

    g_thread_trap.veh = AddVectoredExceptionHandler(LDG_THREAD_TRAP_VEH_FIRST, thread_trap_veh_cb);
    if (LDG_UNLIKELY(!g_thread_trap.veh))
    {
        g_thread_trap.cb = 0x0;
        return LDG_ERR_DENIED;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_trap_shutdown(void)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(g_thread_trap.cb == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(RemoveVectoredExceptionHandler(g_thread_trap.veh) == 0)) { ret = LDG_ERR_DENIED; }

    g_thread_trap.cb = 0x0;
    g_thread_trap.user = 0x0;
    g_thread_trap.veh = 0x0;

    return ret;
}

uint32_t ldg_thread_trap_break_arm(ldg_thread_ref_t *thread, uint32_t slot, uint64_t addr)
{
    if (LDG_UNLIKELY(!thread)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(slot >= LDG_THREAD_TRAP_SLOT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return thread_trap_break_apply(thread, slot, addr, LDG_TRUTH_TRUE);
}

uint32_t ldg_thread_trap_break_disarm(ldg_thread_ref_t *thread, uint32_t slot)
{
    if (LDG_UNLIKELY(!thread)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(slot >= LDG_THREAD_TRAP_SLOT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return thread_trap_break_apply(thread, slot, LDG_THREAD_TRAP_ADDR_NONE, LDG_TRUTH_FALSE);
}
