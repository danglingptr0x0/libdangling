; time_tsc.asm - RDTSCP-based high-resolution timing
; System V AMD64 ABI: args in rdi, rsi, rdx, rcx, r8, r9; return in rax

section .data
    align 64

section .bss
    align 64

section .text

global ldg_tsc_sample
ldg_tsc_sample:
    rdtscp
    shl     rdx, 32
    or      rax, rdx

    test    rdi, rdi
    jz      .done
    mov     [rdi], ecx

.done:
    ret


global ldg_tsc_serialize
ldg_tsc_serialize:
    xor     eax, eax
    cpuid
    ret


global ldg_tsc_serialized_sample
ldg_tsc_serialized_sample:
    push    rbx
    push    rdi

    xor     eax, eax
    cpuid

    rdtscp
    shl     rdx, 32
    or      rax, rdx
    mov     r8, rax
    mov     r9d, ecx

    lfence

    pop     rdi
    test    rdi, rdi
    jz      .skip_store
    mov     [rdi], r9d

.skip_store:
    mov     rax, r8
    pop     rbx
    ret


global ldg_tsc_delta
ldg_tsc_delta:
    mov     rax, rsi
    sub     rax, rdi
    ret
