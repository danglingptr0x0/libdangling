#include <stdint.h>
#include <windows.h>
#include <tlhelp32.h>

#include <dangling/thread/ref.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>

#include "ctx.h"

#define LDG_THREAD_REF_ACCESS (THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION | SYNCHRONIZE)
#define LDG_THREAD_REF_SUSPEND_FAIL ((DWORD)-1)
#define LDG_THREAD_REF_SNAP_ALL 0
#define LDG_THREAD_REF_WAIT_NONE 0

static uint32_t thread_ref_ctx_get(const ldg_thread_ref_t *ref, CONTEXT *ctx)
{
    if (LDG_UNLIKELY(ref->live != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ref->suspended != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ctx->ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;

    if (LDG_UNLIKELY(!GetThreadContext((HANDLE)(uintptr_t)ref->handle, ctx))) { return LDG_ERR_DENIED; }

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_id_list(const ldg_sys_proc_t *proc, uint64_t *out, uint64_t cap, uint64_t *out_cunt)
{
    THREADENTRY32 entry = LDG_STRUCT_ZERO_INIT;
    HANDLE snap = INVALID_HANDLE_VALUE;
    DWORD owner = 0;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!proc || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    owner = (DWORD)proc->pid;

    snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, LDG_THREAD_REF_SNAP_ALL);
    if (LDG_UNLIKELY(snap == INVALID_HANDLE_VALUE)) { return LDG_ERR_DENIED; }

    entry.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(snap, &entry))
    {
        do
        {
            if (*out_cunt >= cap) { break; }

            if (entry.th32OwnerProcessID != owner) { continue; }

            out[*out_cunt] = (uint64_t)entry.th32ThreadID;
            (*out_cunt)++;
        }
        while (Thread32Next(snap, &entry));
    }

    if (LDG_UNLIKELY(!CloseHandle(snap))) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

uint32_t ldg_thread_id_self_get(uint64_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (uint64_t)GetCurrentThreadId();

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_ref_open(const ldg_sys_proc_t *proc, uint64_t id, ldg_thread_ref_t *out)
{
    HANDLE thread = 0x0;

    if (LDG_UNLIKELY(!proc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, sizeof(ldg_thread_ref_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    out->handle = LDG_THREAD_REF_HANDLE_NONE;

    if (LDG_UNLIKELY(id == 0 || id > (uint64_t)MAXDWORD)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    thread = OpenThread(LDG_THREAD_REF_ACCESS, FALSE, (DWORD)id);
    if (!thread) { return (GetLastError() == ERROR_INVALID_PARAMETER) ? LDG_ERR_NOT_FOUND : LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(GetProcessIdOfThread(thread) != (DWORD)proc->pid))
    {
        if (!CloseHandle(thread)) { return LDG_ERR_IO_CLOSE; }

        return LDG_ERR_NOT_FOUND;
    }

    out->pid = proc->pid;
    out->outside = (proc->pid != (uint32_t)GetCurrentProcessId()) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    out->id = id;
    out->handle = (uint64_t)(uintptr_t)thread;
    out->live = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint8_t ldg_thread_ref_exited_is(const ldg_thread_ref_t *ref)
{
    if (LDG_UNLIKELY(!ref)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ref->live != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (WaitForSingleObject((HANDLE)(uintptr_t)ref->handle, LDG_THREAD_REF_WAIT_NONE) == WAIT_OBJECT_0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_thread_ref_suspend(ldg_thread_ref_t *ref)
{
    CONTEXT ctx = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ref)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ref->live != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ref->suspended == LDG_TRUTH_TRUE)) { return LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(ref->id == (uint64_t)GetCurrentThreadId())) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(SuspendThread((HANDLE)(uintptr_t)ref->handle) == LDG_THREAD_REF_SUSPEND_FAIL)) { return LDG_ERR_DENIED; }

    ref->suspended = LDG_TRUTH_TRUE;

    return thread_ref_ctx_get(ref, &ctx);
}

uint32_t ldg_thread_ref_resume(ldg_thread_ref_t *ref)
{
    if (LDG_UNLIKELY(!ref)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ref->live != LDG_TRUTH_TRUE || ref->suspended != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ResumeThread((HANDLE)(uintptr_t)ref->handle) == LDG_THREAD_REF_SUSPEND_FAIL)) { return LDG_ERR_DENIED; }

    ref->suspended = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_ref_regs_get(ldg_thread_ref_t *ref, ldg_thread_regs_t *out)
{
    CONTEXT ctx = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ref || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = thread_ref_ctx_get(ref, &ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    thread_ctx_regs_ld(&ctx, out);

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_ref_regs_set(ldg_thread_ref_t *ref, const ldg_thread_regs_t *regs)
{
    CONTEXT ctx = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ref || !regs)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = thread_ref_ctx_get(ref, &ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    thread_ctx_regs_st(&ctx, regs);

    if (LDG_UNLIKELY(!SetThreadContext((HANDLE)(uintptr_t)ref->handle, &ctx))) { return LDG_ERR_DENIED; }

    return LDG_ERR_AOK;
}

uint32_t ldg_thread_ref_close(ldg_thread_ref_t *ref)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!ref)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ref->handle == LDG_THREAD_REF_HANDLE_NONE) { return LDG_ERR_AOK; }

    if (ref->suspended == LDG_TRUTH_TRUE) { ret = ldg_thread_ref_resume(ref); }

    if (LDG_UNLIKELY(!CloseHandle((HANDLE)(uintptr_t)ref->handle)) && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    ref->handle = LDG_THREAD_REF_HANDLE_NONE;
    ref->live = LDG_TRUTH_FALSE;

    return ret;
}
