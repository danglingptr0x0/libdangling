; syscall.asm - TSC access (Windows; no kernel syscall wrappers)
; Microsoft x64 ABI: args in rcx, rdx, r8, r9; return in rax

section .data
    align 64

section .bss
    align 64

section .text

global ldg_rdtsc
ldg_rdtsc:
    rdtsc
    shl     rdx, 32
    or      rax, rdx
    ret


; uint64_t ldg_rdtscp(uint32_t *aux)
; rcx=aux
global ldg_rdtscp
ldg_rdtscp:
    rdtscp
    shl     rdx, 32
    or      rax, rdx
    test    rcx, rcx
    jz      .done
    mov     [rcx], ecx
.done:
    ret
