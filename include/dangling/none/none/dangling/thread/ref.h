#ifndef LDG_THREAD_REF_H
#define LDG_THREAD_REF_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/sys/proc.h>

#define LDG_THREAD_REF_HANDLE_NONE UINT64_MAX

typedef enum ldg_thread_reg
{
    LDG_THREAD_REG_RAX = 0,
    LDG_THREAD_REG_RCX,
    LDG_THREAD_REG_RDX,
    LDG_THREAD_REG_RBX,
    LDG_THREAD_REG_RSP,
    LDG_THREAD_REG_RBP,
    LDG_THREAD_REG_RSI,
    LDG_THREAD_REG_RDI,
    LDG_THREAD_REG_R8,
    LDG_THREAD_REG_R9,
    LDG_THREAD_REG_R10,
    LDG_THREAD_REG_R11,
    LDG_THREAD_REG_R12,
    LDG_THREAD_REG_R13,
    LDG_THREAD_REG_R14,
    LDG_THREAD_REG_R15,
    LDG_THREAD_REG_CUNT
} ldg_thread_reg_t;

typedef struct ldg_thread_regs
{
    uint64_t gpr[LDG_THREAD_REG_CUNT];
    uint64_t rip;
    uint64_t rflags;
} ldg_thread_regs_t;

typedef struct ldg_thread_ref
{
    uint64_t id;
    uint64_t handle;
    uint32_t pid;
    uint8_t live;
    uint8_t suspended;
    uint8_t outside;
    uint8_t pudding[1];
} ldg_thread_ref_t;

LDG_EXPORT uint32_t ldg_thread_id_list(const ldg_sys_proc_t *proc, uint64_t *out, uint64_t cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_thread_id_self_get(uint64_t *out);
LDG_EXPORT uint32_t ldg_thread_ref_open(const ldg_sys_proc_t *proc, uint64_t id, ldg_thread_ref_t *out);
LDG_EXPORT uint8_t ldg_thread_ref_exited_is(const ldg_thread_ref_t *ref);
LDG_EXPORT uint32_t ldg_thread_ref_suspend(ldg_thread_ref_t *ref);
LDG_EXPORT uint32_t ldg_thread_ref_resume(ldg_thread_ref_t *ref);
LDG_EXPORT uint32_t ldg_thread_ref_regs_get(ldg_thread_ref_t *ref, ldg_thread_regs_t *out);
LDG_EXPORT uint32_t ldg_thread_ref_regs_set(ldg_thread_ref_t *ref, const ldg_thread_regs_t *regs);
LDG_EXPORT uint32_t ldg_thread_ref_close(ldg_thread_ref_t *ref);

#endif
