#include <stdint.h>
#include <windows.h>

#include <dangling/mem/region.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>

#define LDG_MEM_REGION_PROT_RD (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)
#define LDG_MEM_REGION_PROT_WR (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)
#define LDG_MEM_REGION_PROT_EXEC (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

static uint32_t mem_region_query(const ldg_sys_proc_t *proc, uint64_t addr, ldg_mem_region_t *out)
{
    MEMORY_BASIC_INFORMATION info = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, sizeof(ldg_mem_region_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (VirtualQueryEx((HANDLE)(uintptr_t)proc->handle, (const void *)(uintptr_t)addr, &info, sizeof(MEMORY_BASIC_INFORMATION)) == 0) { return (GetLastError() == ERROR_ACCESS_DENIED) ? LDG_ERR_DENIED : LDG_ERR_EOF; }

    out->base = (uint64_t)(uintptr_t)info.BaseAddress;
    out->size = (uint64_t)info.RegionSize;
    out->alloc_base = (uint64_t)(uintptr_t)info.AllocationBase;

    if (info.State != MEM_COMMIT) { return LDG_ERR_AOK; }

    if (info.Type == MEM_IMAGE) { out->kind = LDG_MEM_REGION_KIND_IMG; }
    else if (info.Type == MEM_MAPPED) { out->kind = LDG_MEM_REGION_KIND_MAPPED; }
    else { out->kind = LDG_MEM_REGION_KIND_PRIVATE; }

    out->committed = LDG_TRUTH_TRUE;
    out->readable = ((info.Protect & LDG_MEM_REGION_PROT_RD) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    out->writable = ((info.Protect & LDG_MEM_REGION_PROT_WR) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    out->executable = ((info.Protect & LDG_MEM_REGION_PROT_EXEC) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    out->guard = ((info.Protect & PAGE_GUARD) != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_region_get(const ldg_sys_proc_t *proc, uint64_t addr, ldg_mem_region_t *out)
{
    if (LDG_UNLIKELY(!proc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    return mem_region_query(proc, addr, out);
}

uint32_t ldg_mem_region_list(const ldg_sys_proc_t *proc, uint64_t addr, ldg_mem_region_t *out, uint64_t cap, uint64_t *out_cunt)
{
    uint64_t cursor = addr;
    uint64_t next = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!proc || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (*out_cunt < cap)
    {
        ret = mem_region_query(proc, cursor, &out[*out_cunt]);
        if (ret == LDG_ERR_EOF) { break; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        next = out[*out_cunt].base + out[*out_cunt].size;
        if (out[*out_cunt].committed == LDG_TRUTH_TRUE) { (*out_cunt)++; }

        if (next <= cursor) { break; }

        cursor = next;
    }

    return LDG_ERR_AOK;
}
