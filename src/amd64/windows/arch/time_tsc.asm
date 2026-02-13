; time_tsc.asm - RDTSCP-based high-resolution timing
; Microsoft x64 ABI: args in rcx, rdx, r8, r9; return in rax
; Callee-saved: rbx, rbp, rdi, rsi, r12-r15

section .data
    align 64

section .bss
    align 64

section .text

; uint64_t ldg_tsc_sample(uint32_t *core_id)
; rcx=core_id
global ldg_tsc_sample
ldg_tsc_sample:
    rdtscp
    shl     rdx, 32
    or      rax, rdx

    test    rcx, rcx
    jz      .done
    mov     [rcx], ecx

.done:
    ret


; void ldg_tsc_serialize(void)
global ldg_tsc_serialize
ldg_tsc_serialize:
    push    rbx
    xor     eax, eax
    cpuid
    pop     rbx
    ret


; uint64_t ldg_tsc_serialized_sample(uint32_t *core_id)
; rcx=core_id
global ldg_tsc_serialized_sample
ldg_tsc_serialized_sample:
    push    rbx
    push    rcx

    xor     eax, eax
    cpuid

    rdtscp
    shl     rdx, 32
    or      rax, rdx
    mov     r8, rax
    mov     r9d, ecx

    lfence

    pop     rcx
    test    rcx, rcx
    jz      .skip_store
    mov     [rcx], r9d

.skip_store:
    mov     rax, r8
    pop     rbx
    ret


; uint64_t ldg_tsc_delta(uint64_t start, uint64_t end)
; rcx=start, rdx=end
global ldg_tsc_delta
ldg_tsc_delta:
    mov     rax, rdx
    sub     rax, rcx
    ret
