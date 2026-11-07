#include <stdint.h>
#include <errno.h>
#include <sys/mman.h>

#include <dangling/mem/exec.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_MEM_EXEC_ANON_FD (-1)
#define LDG_MEM_EXEC_ANON_OFFSET 0

static uint32_t mem_exec_errno_translate(void)
{
    if (errno == ENOMEM) { return LDG_ERR_ALLOC_NULL; }

    if (errno == EACCES || errno == EPERM) { return LDG_ERR_DENIED; }

    return LDG_ERR_FUNC_ARG_INVALID;
}

uint32_t ldg_mem_exec_map(uint64_t size, void **out)
{
    void *addr = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    addr = mmap(0x0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, LDG_MEM_EXEC_ANON_FD, LDG_MEM_EXEC_ANON_OFFSET);
    if (LDG_UNLIKELY(addr == MAP_FAILED)) { return mem_exec_errno_translate(); }

    *out = addr;

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_exec_prot_set(void *addr, uint64_t size, ldg_mem_prot_t prot)
{
    int32_t flags = PROT_READ;

    if (LDG_UNLIKELY(!addr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(size == 0 || prot >= LDG_MEM_PROT_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (prot == LDG_MEM_PROT_RW) { flags = PROT_READ | PROT_WRITE; }
    else if (prot == LDG_MEM_PROT_RX) { flags = PROT_READ | PROT_EXEC; }

    if (LDG_UNLIKELY(mprotect(addr, size, flags) != 0)) { return mem_exec_errno_translate(); }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_exec_unmap(void *addr, uint64_t size)
{
    if (LDG_UNLIKELY(!addr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(munmap(addr, size) != 0)) { return mem_exec_errno_translate(); }

    return LDG_ERR_AOK;
}
