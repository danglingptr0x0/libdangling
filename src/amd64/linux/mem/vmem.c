#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>

#include <dangling/mem/vmem.h>
#include <dangling/arch/amd64/syscall.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_MEM_VMEM_SPAN_CUNT 1
#define LDG_MEM_VMEM_NO_FLAG 0

static uint32_t mem_vmem_xfer(const ldg_sys_proc_t *proc, int64_t call, uint64_t addr, void *near, uint64_t len, uint64_t *out_done)
{
    struct iovec near_span = LDG_STRUCT_ZERO_INIT;
    struct iovec far_span = LDG_STRUCT_ZERO_INIT;
    int64_t done = 0;

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    near_span.iov_base = near;
    near_span.iov_len = len;
    far_span.iov_base = (void *)(uintptr_t)addr;
    far_span.iov_len = len;

    done = ldg_syscall6(call, (int64_t)proc->pid, (int64_t)(uintptr_t)&near_span, LDG_MEM_VMEM_SPAN_CUNT, (int64_t)(uintptr_t)&far_span, LDG_MEM_VMEM_SPAN_CUNT, LDG_MEM_VMEM_NO_FLAG);
    if (done + (int64_t)EPERM == 0) { return LDG_ERR_DENIED; }

    if (done + (int64_t)ESRCH == 0) { return LDG_ERR_NOT_FOUND; }

    if (done < 0) { return LDG_ERR_MEM_BAD; }

    *out_done = (uint64_t)done;

    if ((uint64_t)done != len) { return LDG_ERR_MEM_BAD; }

    return LDG_ERR_AOK;
}

uint32_t ldg_mem_vmem_rd(const ldg_sys_proc_t *proc, uint64_t addr, void *dst, uint64_t len, uint64_t *out_got)
{
    if (LDG_UNLIKELY(!proc || !dst || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (len == 0) { return LDG_ERR_AOK; }

    return mem_vmem_xfer(proc, LDG_SYS_PROC_VMEM_RD, addr, dst, len, out_got);
}

uint32_t ldg_mem_vmem_wr(const ldg_sys_proc_t *proc, uint64_t addr, const void *src, uint64_t len, uint64_t *out_put)
{
    void *near = 0x0;

    if (LDG_UNLIKELY(!proc || !src || !out_put)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_put = 0;

    if (len == 0) { return LDG_ERR_AOK; }

    near = (void *)(uintptr_t)src;

    return mem_vmem_xfer(proc, LDG_SYS_PROC_VMEM_WR, addr, near, len, out_put);
}

uint32_t ldg_mem_vmem_flush(const ldg_sys_proc_t *proc, uint64_t addr, uint64_t len)
{
    if (LDG_UNLIKELY(!proc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0 || addr + len < addr)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(proc->handle == LDG_SYS_PROC_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (proc->pid == (uint32_t)getpid()) { __builtin___clear_cache((char *)(uintptr_t)addr, (char *)(uintptr_t)(addr + len)); }

    return LDG_ERR_AOK;
}
