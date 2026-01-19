; syscall.asm - direct syscall wrappers and TSC access
; System V AMD64 ABI: args in rdi, rsi, rdx, rcx, r8, r9; return in rax

section .data
    align 64

section .bss
    align 64

section .text

global ldg_syscall0
ldg_syscall0:
    mov     rax, rdi
    syscall
    ret


global ldg_syscall1
ldg_syscall1:
    mov     rax, rdi
    mov     rdi, rsi
    syscall
    ret


global ldg_syscall2
ldg_syscall2:
    mov     rax, rdi
    mov     rdi, rsi
    mov     rsi, rdx
    syscall
    ret


global ldg_syscall3
ldg_syscall3:
    mov     rax, rdi
    mov     rdi, rsi
    mov     rsi, rdx
    mov     r10, rcx
    syscall
    ret


global ldg_syscall4
ldg_syscall4:
    mov     rax, rdi
    mov     rdi, rsi
    mov     rsi, rdx
    mov     r10, rcx
    syscall
    ret


global ldg_rdtsc
ldg_rdtsc:
    rdtsc
    shl     rdx, 32
    or      rax, rdx
    ret


global ldg_rdtscp
ldg_rdtscp:
    rdtscp
    shl     rdx, 32
    or      rax, rdx
    test    rdi, rdi
    jz      .done
    mov     [rdi], ecx
.done:
    ret
