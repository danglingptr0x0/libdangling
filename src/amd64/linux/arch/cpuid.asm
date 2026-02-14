; cpuid.asm - CPUID utilities
; System V AMD64 ABI: args in rdi, rsi, rdx, rcx, r8, r9; return in rax

%include "dangling/core/err.inc"

section .text

; uint32_t ldg_cpuid(uint32_t leaf, uint32_t subleaf, ldg_cpuid_regs_t *regs)
global ldg_cpuid
ldg_cpuid:
    test    rdx, rdx
    jz      .cpuid_null_err
    push    rbx
    mov     eax, edi
    mov     ecx, esi
    mov     r8, rdx

    cpuid

    mov     [r8], eax
    mov     [r8 + 4], ebx
    mov     [r8 + 8], ecx
    mov     [r8 + 12], edx

    xor     eax, eax
    pop     rbx
    ret

.cpuid_null_err:
    mov     eax, LDG_ERR_FUNC_ARG_NULL
    ret


; uint32_t ldg_cpuid_vendor_get(char out[13])
global ldg_cpuid_vendor_get
ldg_cpuid_vendor_get:
    test    rdi, rdi
    jz      .vendor_null_err
    push    rbx
    mov     r8, rdi

    xor     eax, eax
    cpuid

    mov     [r8], ebx
    mov     [r8 + 4], edx
    mov     [r8 + 8], ecx
    mov     byte [r8 + 12], 0

    xor     eax, eax
    pop     rbx
    ret

.vendor_null_err:
    mov     eax, LDG_ERR_FUNC_ARG_NULL
    ret


; uint32_t ldg_cpuid_brand_get(char out[49])
global ldg_cpuid_brand_get
ldg_cpuid_brand_get:
    test    rdi, rdi
    jz      .brand_null_err
    push    rbx
    push    r12
    mov     r12, rdi

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
    xor     eax, eax
    pop     r12
    pop     rbx
    ret

.brand_null_err:
    mov     eax, LDG_ERR_FUNC_ARG_NULL
    ret


; uint32_t ldg_cpuid_feat_get(ldg_cpuid_feat_t *feat)
global ldg_cpuid_feat_get
ldg_cpuid_feat_get:
    test    rdi, rdi
    jz      .feat_null_err
    push    rbx
    push    r12
    mov     r12, rdi

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

    xor     eax, eax
    pop     r12
    pop     rbx
    ret

.feat_null_err:
    mov     eax, LDG_ERR_FUNC_ARG_NULL
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
