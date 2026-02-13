; cpuid.asm - CPUID utilities
; Microsoft x64 ABI: args in rcx, rdx, r8, r9; return in rax
; Callee-saved: rbx, rbp, rdi, rsi, r12-r15

section .text

; void ldg_cpuid(uint32_t leaf, uint32_t subleaf, ldg_cpuid_regs_t *regs)
; rcx=leaf, edx=subleaf, r8=regs
global ldg_cpuid
ldg_cpuid:
    push    rbx
    mov     eax, ecx
    mov     ecx, edx
    mov     r9, r8

    cpuid

    mov     [r9], eax
    mov     [r9 + 4], ebx
    mov     [r9 + 8], ecx
    mov     [r9 + 12], edx

    pop     rbx
    ret


; void ldg_cpuid_vendor_get(char out[13])
; rcx=out
global ldg_cpuid_vendor_get
ldg_cpuid_vendor_get:
    push    rbx
    mov     r8, rcx

    xor     eax, eax
    cpuid

    mov     [r8], ebx
    mov     [r8 + 4], edx
    mov     [r8 + 8], ecx
    mov     byte [r8 + 12], 0

    pop     rbx
    ret


; void ldg_cpuid_brand_get(char out[49])
; rcx=out
global ldg_cpuid_brand_get
ldg_cpuid_brand_get:
    push    rbx
    push    r12
    mov     r12, rcx

    mov     eax, 0x80000000
    cpuid
    cmp     eax, 0x80000004
    jb      .no_brand

    mov     eax, 0x80000002
    cpuid
    mov     [r12], eax
    mov     [r12 + 4], ebx
    mov     [r12 + 8], ecx
    mov     [r12 + 12], edx

    mov     eax, 0x80000003
    cpuid
    mov     [r12 + 16], eax
    mov     [r12 + 20], ebx
    mov     [r12 + 24], ecx
    mov     [r12 + 28], edx

    mov     eax, 0x80000004
    cpuid
    mov     [r12 + 32], eax
    mov     [r12 + 36], ebx
    mov     [r12 + 40], ecx
    mov     [r12 + 44], edx

    mov     byte [r12 + 48], 0
    jmp     .done

.no_brand:
    mov     byte [r12], 0

.done:
    pop     r12
    pop     rbx
    ret


; void ldg_cpuid_features_get(ldg_cpuid_features_t *features)
; rcx=features
global ldg_cpuid_features_get
ldg_cpuid_features_get:
    push    rbx
    push    r12
    mov     r12, rcx

    xor     eax, eax
    mov     [r12], eax

    mov     eax, 1
    cpuid

    xor     r8d, r8d

    bt      edx, 25
    adc     r8d, r8d
    bt      edx, 26
    adc     r8d, r8d
    bt      ecx, 0
    adc     r8d, r8d
    bt      ecx, 9
    adc     r8d, r8d
    bt      ecx, 19
    adc     r8d, r8d
    bt      ecx, 20
    adc     r8d, r8d
    bt      ecx, 28
    adc     r8d, r8d

    mov     r9d, ecx
    mov     r10d, edx

    mov     eax, 7
    xor     ecx, ecx
    cpuid

    bt      ebx, 5
    adc     r8d, r8d
    bt      ebx, 16
    adc     r8d, r8d

    bt      r9d, 25
    adc     r8d, r8d
    bt      r9d, 1
    adc     r8d, r8d
    bt      r9d, 30
    adc     r8d, r8d
    bt      ebx, 18
    adc     r8d, r8d
    bt      ebx, 3
    adc     r8d, r8d
    bt      ebx, 8
    adc     r8d, r8d
    bt      r9d, 23
    adc     r8d, r8d
    bt      r9d, 12
    adc     r8d, r8d
    bt      r9d, 29
    adc     r8d, r8d
    bt      r10d, 8
    adc     r8d, r8d
    bt      r9d, 13
    adc     r8d, r8d
    bt      r10d, 28
    adc     r8d, r8d

    mov     eax, 0x80000007
    cpuid
    bt      edx, 8
    adc     r8d, r8d

    mov     [r12], r8d

    pop     r12
    pop     rbx
    ret


; uint32_t ldg_cpuid_max_leaf_get(void)
global ldg_cpuid_max_leaf_get
ldg_cpuid_max_leaf_get:
    push    rbx
    xor     eax, eax
    cpuid
    pop     rbx
    ret


; uint32_t ldg_cpuid_max_ext_leaf_get(void)
global ldg_cpuid_max_ext_leaf_get
ldg_cpuid_max_ext_leaf_get:
    push    rbx
    mov     eax, 0x80000000
    cpuid
    pop     rbx
    ret


; uint32_t ldg_cpu_core_id_get(void)
global ldg_cpu_core_id_get
ldg_cpu_core_id_get:
    rdtscp
    mov     eax, ecx
    ret


; void ldg_cpu_relax(void)
global ldg_cpu_relax
ldg_cpu_relax:
    pause
    ret
