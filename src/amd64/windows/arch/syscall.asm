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
    rdtscp
    shl     rdx, 32
    or      rax, rdx
    test    rcx, rcx
    jz      .done
    mov     [rcx], ecx
.done:
    ret
