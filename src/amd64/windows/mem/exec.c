#include <stdint.h>
#include <windows.h>

#include <dangling/mem/exec.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_MEM_EXEC_RELEASE_SIZE 0

static uint32_t mem_exec_last_err_translate(void)
{
    DWORD code = GetLastError();

    if (code == ERROR_NOT_ENOUGH_MEMORY || code == ERROR_COMMITMENT_LIMIT) { return LDG_ERR_ALLOC_NULL; }

    if (code == ERROR_ACCESS_DENIED) { return LDG_ERR_DENIED; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

uint32_t ldg_mem_exec_map(uint64_t size, void **out)
{
    void *addr = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    addr = VirtualAlloc(0x0, (SIZE_T)size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (LDG_UNLIKELY(!addr)) { return mem_exec_last_err_translate(); }

    *out = addr;

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_exec_prot_set(void *addr, uint64_t size, ldg_mem_prot_t prot)
{
    DWORD flags = PAGE_READONLY;
    DWORD prev = 0;

    if (LDG_UNLIKELY(!addr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(size == 0 || prot >= LDG_MEM_PROT_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (prot == LDG_MEM_PROT_RW) { flags = PAGE_READWRITE; }
    else if (prot == LDG_MEM_PROT_RX) { flags = PAGE_EXECUTE_READ; }

    if (LDG_UNLIKELY(!VirtualProtect(addr, (SIZE_T)size, flags, &prev))) { return mem_exec_last_err_translate(); }

    if (prot == LDG_MEM_PROT_RX) { if (LDG_UNLIKELY(!FlushInstructionCache(GetCurrentProcess(), addr, (SIZE_T)size))) { return mem_exec_last_err_translate(); } }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_exec_unmap(void *addr, uint64_t size)
{
    if (LDG_UNLIKELY(!addr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!VirtualFree(addr, LDG_MEM_EXEC_RELEASE_SIZE, MEM_RELEASE))) { return mem_exec_last_err_translate(); }

    return LDG_ERR_AOK;
}
