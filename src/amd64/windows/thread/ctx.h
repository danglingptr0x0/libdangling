#ifndef LDG_THREAD_CTX_H
#define LDG_THREAD_CTX_H

#include <stdint.h>
#include <windows.h>

#include <dangling/thread/ref.h>

static inline void thread_ctx_regs_ld(const CONTEXT *ctx, ldg_thread_regs_t *out)
{
    out->gpr[LDG_THREAD_REG_RAX] = (uint64_t)ctx->Rax;
    out->gpr[LDG_THREAD_REG_RCX] = (uint64_t)ctx->Rcx;
    out->gpr[LDG_THREAD_REG_RDX] = (uint64_t)ctx->Rdx;
    out->gpr[LDG_THREAD_REG_RBX] = (uint64_t)ctx->Rbx;
    out->gpr[LDG_THREAD_REG_RSP] = (uint64_t)ctx->Rsp;
    out->gpr[LDG_THREAD_REG_RBP] = (uint64_t)ctx->Rbp;
    out->gpr[LDG_THREAD_REG_RSI] = (uint64_t)ctx->Rsi;
    out->gpr[LDG_THREAD_REG_RDI] = (uint64_t)ctx->Rdi;
    out->gpr[LDG_THREAD_REG_R8] = (uint64_t)ctx->R8;
    out->gpr[LDG_THREAD_REG_R9] = (uint64_t)ctx->R9;
    out->gpr[LDG_THREAD_REG_R10] = (uint64_t)ctx->R10;
    out->gpr[LDG_THREAD_REG_R11] = (uint64_t)ctx->R11;
    out->gpr[LDG_THREAD_REG_R12] = (uint64_t)ctx->R12;
    out->gpr[LDG_THREAD_REG_R13] = (uint64_t)ctx->R13;
    out->gpr[LDG_THREAD_REG_R14] = (uint64_t)ctx->R14;
    out->gpr[LDG_THREAD_REG_R15] = (uint64_t)ctx->R15;
    out->rip = (uint64_t)ctx->Rip;
    out->rflags = (uint64_t)ctx->EFlags;
}

static inline void thread_ctx_regs_st(CONTEXT *ctx, const ldg_thread_regs_t *regs)
{
    ctx->Rax = (DWORD64)regs->gpr[LDG_THREAD_REG_RAX];
    ctx->Rcx = (DWORD64)regs->gpr[LDG_THREAD_REG_RCX];
    ctx->Rdx = (DWORD64)regs->gpr[LDG_THREAD_REG_RDX];
    ctx->Rbx = (DWORD64)regs->gpr[LDG_THREAD_REG_RBX];
    ctx->Rsp = (DWORD64)regs->gpr[LDG_THREAD_REG_RSP];
    ctx->Rbp = (DWORD64)regs->gpr[LDG_THREAD_REG_RBP];
    ctx->Rsi = (DWORD64)regs->gpr[LDG_THREAD_REG_RSI];
    ctx->Rdi = (DWORD64)regs->gpr[LDG_THREAD_REG_RDI];
    ctx->R8 = (DWORD64)regs->gpr[LDG_THREAD_REG_R8];
    ctx->R9 = (DWORD64)regs->gpr[LDG_THREAD_REG_R9];
    ctx->R10 = (DWORD64)regs->gpr[LDG_THREAD_REG_R10];
    ctx->R11 = (DWORD64)regs->gpr[LDG_THREAD_REG_R11];
    ctx->R12 = (DWORD64)regs->gpr[LDG_THREAD_REG_R12];
    ctx->R13 = (DWORD64)regs->gpr[LDG_THREAD_REG_R13];
    ctx->R14 = (DWORD64)regs->gpr[LDG_THREAD_REG_R14];
    ctx->R15 = (DWORD64)regs->gpr[LDG_THREAD_REG_R15];
    ctx->Rip = (DWORD64)regs->rip;
    ctx->EFlags = (DWORD)regs->rflags;
}

#endif
