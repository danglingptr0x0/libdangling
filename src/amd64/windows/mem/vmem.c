#include <stdint.h>
#include <windows.h>

#include <dangling/mem/vmem.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

static uint32_t mem_vmem_fail_translate(void)
{
    DWORD code = GetLastError();

    if (code == ERROR_ACCESS_DENIED) { return LDG_ERR_DENIED; }

    return LDG_ERR_MEM_BAD;
}

uint32_t ldg_mem_vmem_rd(const ldg_sys_proc_t *proc, uint64_t addr, void *dst, uint64_t len, uint64_t *out_got)
{
    SIZE_T got = 0;

    if (LDG_UNLIKELY(!proc || !dst || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (!ReadProcessMemory((HANDLE)(uintptr_t)proc->handle, (const void *)(uintptr_t)addr, dst, (SIZE_T)len, &got))
    {
        *out_got = (uint64_t)got;
        return mem_vmem_fail_translate();
    }

    *out_got = (uint64_t)got;

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_vmem_wr(const ldg_sys_proc_t *proc, uint64_t addr, const void *src, uint64_t len, uint64_t *out_put)
{
    SIZE_T put = 0;

    if (LDG_UNLIKELY(!proc || !src || !out_put)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_put = 0;

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (!WriteProcessMemory((HANDLE)(uintptr_t)proc->handle, (void *)(uintptr_t)addr, src, (SIZE_T)len, &put))
    {
        *out_put = (uint64_t)put;
        return mem_vmem_fail_translate();
    }

    *out_put = (uint64_t)put;

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_vmem_flush(const ldg_sys_proc_t *proc, uint64_t addr, uint64_t len)
{
    if (LDG_UNLIKELY(!proc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(!FlushInstructionCache((HANDLE)(uintptr_t)proc->handle, (const void *)(uintptr_t)addr, (SIZE_T)len))) { return LDG_ERR_MEM_BAD; }

    return LDG_ERR_AOK;
}
