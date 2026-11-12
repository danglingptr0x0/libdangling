#ifndef LDG_THREAD_UC_H
#define LDG_THREAD_UC_H

#include <stdint.h>
#include <signal.h>
#include <ucontext.h>

#include <dangling/thread/ref.h>

#define LDG_THREAD_UC_R8 0
#define LDG_THREAD_UC_R9 1
#define LDG_THREAD_UC_R10 2
#define LDG_THREAD_UC_R11 3
#define LDG_THREAD_UC_R12 4
#define LDG_THREAD_UC_R13 5
#define LDG_THREAD_UC_R14 6
#define LDG_THREAD_UC_R15 7
#define LDG_THREAD_UC_RDI 8
#define LDG_THREAD_UC_RSI 9
#define LDG_THREAD_UC_RBP 10
#define LDG_THREAD_UC_RBX 11
#define LDG_THREAD_UC_RDX 12
#define LDG_THREAD_UC_RAX 13
#define LDG_THREAD_UC_RCX 14
#define LDG_THREAD_UC_RSP 15
#define LDG_THREAD_UC_RIP 16
#define LDG_THREAD_UC_RFLAGS 17

static inline void thread_uc_regs_ld(const void *uc, ldg_thread_regs_t *out)
{
    const ucontext_t *state = (const ucontext_t *)uc;
    const uint64_t *raw = (const uint64_t *)(const void *)&state->uc_mcontext;

    out->gpr[LDG_THREAD_REG_RAX] = raw[LDG_THREAD_UC_RAX];
    out->gpr[LDG_THREAD_REG_RCX] = raw[LDG_THREAD_UC_RCX];
    out->gpr[LDG_THREAD_REG_RDX] = raw[LDG_THREAD_UC_RDX];
    out->gpr[LDG_THREAD_REG_RBX] = raw[LDG_THREAD_UC_RBX];
    out->gpr[LDG_THREAD_REG_RSP] = raw[LDG_THREAD_UC_RSP];
    out->gpr[LDG_THREAD_REG_RBP] = raw[LDG_THREAD_UC_RBP];
    out->gpr[LDG_THREAD_REG_RSI] = raw[LDG_THREAD_UC_RSI];
    out->gpr[LDG_THREAD_REG_RDI] = raw[LDG_THREAD_UC_RDI];
    out->gpr[LDG_THREAD_REG_R8] = raw[LDG_THREAD_UC_R8];
    out->gpr[LDG_THREAD_REG_R9] = raw[LDG_THREAD_UC_R9];
    out->gpr[LDG_THREAD_REG_R10] = raw[LDG_THREAD_UC_R10];
    out->gpr[LDG_THREAD_REG_R11] = raw[LDG_THREAD_UC_R11];
    out->gpr[LDG_THREAD_REG_R12] = raw[LDG_THREAD_UC_R12];
    out->gpr[LDG_THREAD_REG_R13] = raw[LDG_THREAD_UC_R13];
    out->gpr[LDG_THREAD_REG_R14] = raw[LDG_THREAD_UC_R14];
    out->gpr[LDG_THREAD_REG_R15] = raw[LDG_THREAD_UC_R15];
    out->rip = raw[LDG_THREAD_UC_RIP];
    out->rflags = raw[LDG_THREAD_UC_RFLAGS];
}

static inline void thread_uc_regs_st(void *uc, const ldg_thread_regs_t *regs)
{
    ucontext_t *state = (ucontext_t *)uc;
    uint64_t *raw = (uint64_t *)(void *)&state->uc_mcontext;

    raw[LDG_THREAD_UC_RAX] = regs->gpr[LDG_THREAD_REG_RAX];
    raw[LDG_THREAD_UC_RCX] = regs->gpr[LDG_THREAD_REG_RCX];
    raw[LDG_THREAD_UC_RDX] = regs->gpr[LDG_THREAD_REG_RDX];
    raw[LDG_THREAD_UC_RBX] = regs->gpr[LDG_THREAD_REG_RBX];
    raw[LDG_THREAD_UC_RSP] = regs->gpr[LDG_THREAD_REG_RSP];
    raw[LDG_THREAD_UC_RBP] = regs->gpr[LDG_THREAD_REG_RBP];
    raw[LDG_THREAD_UC_RSI] = regs->gpr[LDG_THREAD_REG_RSI];
    raw[LDG_THREAD_UC_RDI] = regs->gpr[LDG_THREAD_REG_RDI];
    raw[LDG_THREAD_UC_R8] = regs->gpr[LDG_THREAD_REG_R8];
    raw[LDG_THREAD_UC_R9] = regs->gpr[LDG_THREAD_REG_R9];
    raw[LDG_THREAD_UC_R10] = regs->gpr[LDG_THREAD_REG_R10];
    raw[LDG_THREAD_UC_R11] = regs->gpr[LDG_THREAD_REG_R11];
    raw[LDG_THREAD_UC_R12] = regs->gpr[LDG_THREAD_REG_R12];
    raw[LDG_THREAD_UC_R13] = regs->gpr[LDG_THREAD_REG_R13];
    raw[LDG_THREAD_UC_R14] = regs->gpr[LDG_THREAD_REG_R14];
    raw[LDG_THREAD_UC_R15] = regs->gpr[LDG_THREAD_REG_R15];
    raw[LDG_THREAD_UC_RIP] = regs->rip;
    raw[LDG_THREAD_UC_RFLAGS] = regs->rflags;
}

#endif
