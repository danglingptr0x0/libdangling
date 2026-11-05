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

global ldg_rdtscp
ldg_rdtscp:
    mov r8, rcx
    rdtscp
    shl rdx, 32
    or rax, rdx
    test r8, r8
    jz .done
    mov [r8], ecx
.done:
    ret
