; mem_secure.asm - secure memory operations
; Microsoft x64 ABI: args in rcx, rdx, r8, r9; return in rax
; Callee-saved: rbx, rbp, rdi, rsi, r12-r15

%include "dangling/core/err.inc"

section .data
    align 64

section .bss
    align 64

section .text

; void ldg_mem_secure_zero(void *ptr, uint64_t len)
; rcx=ptr, rdx=len
global ldg_mem_secure_zero
ldg_mem_secure_zero:
    test    rcx, rcx
    jz      .null_err
    test    rdx, rdx
    jz      .invalid_err

    push    rdi
    mov     rdi, rcx
    mov     rcx, rdx
    xor     eax, eax

    mov     rdx, rcx
    shr     rcx, 3
    test    rcx, rcx
    jz      .byte_loop_setup

.qword_loop:
    mov     qword [rdi], rax
    add     rdi, 8
    dec     rcx
    jnz     .qword_loop

.byte_loop_setup:
    mov     rcx, rdx
    and     rcx, 7
    test    rcx, rcx
    jz      .clear_regs

.byte_loop:
    mov     byte [rdi], al
    inc     rdi
    dec     rcx
    jnz     .byte_loop

.clear_regs:
    mfence

    xor     eax, eax
    xor     ecx, ecx
    xor     edx, edx
    xor     r8d, r8d
    xor     r9d, r9d
    xor     r10d, r10d
    xor     r11d, r11d

    pxor    xmm0, xmm0
    pxor    xmm1, xmm1
    pxor    xmm2, xmm2
    pxor    xmm3, xmm3
    pxor    xmm4, xmm4
    pxor    xmm5, xmm5
    pxor    xmm6, xmm6
    pxor    xmm7, xmm7

    pop     rdi

.done:
    ret

.null_err:
    mov     eax, LDG_ERR_FUNC_ARG_NULL
    ret

.invalid_err:
    mov     eax, LDG_ERR_FUNC_ARG_INVALID
    ret


; uint32_t ldg_mem_secure_copy(void *dst, const void *src, uint64_t len)
; rcx=dst, rdx=src, r8=len
global ldg_mem_secure_copy
ldg_mem_secure_copy:
    test    rcx, rcx
    jz      .copy_null_err
    test    rdx, rdx
    jz      .copy_null_err
    test    r8, r8
    jz      .copy_invalid_err

    push    rdi
    push    rsi
    mov     rdi, rcx
    mov     rsi, rdx
    mov     rcx, r8

    mov     r8, rcx
    shr     rcx, 3
    test    rcx, rcx
    jz      .copy_byte_setup

.copy_qword_loop:
    mov     rax, qword [rsi]
    mov     qword [rdi], rax
    add     rdi, 8
    add     rsi, 8
    dec     rcx
    jnz     .copy_qword_loop

.copy_byte_setup:
    mov     rcx, r8
    and     rcx, 7
    test    rcx, rcx
    jz      .copy_clear_regs

.copy_byte_loop:
    mov     al, byte [rsi]
    mov     byte [rdi], al
    inc     rdi
    inc     rsi
    dec     rcx
    jnz     .copy_byte_loop

.copy_clear_regs:
    mfence

    xor     eax, eax
    xor     ecx, ecx
    xor     edx, edx
    xor     r8d, r8d
    xor     r9d, r9d
    xor     r10d, r10d
    xor     r11d, r11d

    pxor    xmm0, xmm0
    pxor    xmm1, xmm1
    pxor    xmm2, xmm2
    pxor    xmm3, xmm3
    pxor    xmm4, xmm4
    pxor    xmm5, xmm5
    pxor    xmm6, xmm6
    pxor    xmm7, xmm7

    pop     rsi
    pop     rdi

.copy_done:
    ret

.copy_null_err:
    mov     eax, LDG_ERR_FUNC_ARG_NULL
    ret

.copy_invalid_err:
    mov     eax, LDG_ERR_FUNC_ARG_INVALID
    ret


; uint32_t ldg_mem_secure_cmp(const void *a, const void *b, uint64_t len, uint32_t *result)
; rcx=a, rdx=b, r8=len, r9=result
global ldg_mem_secure_cmp
ldg_mem_secure_cmp:
    test    rcx, rcx
    jz      .cmp_null_err
    test    rdx, rdx
    jz      .cmp_null_err
    test    r9, r9
    jz      .cmp_null_err

    mov     dword [r9], 0
    test    r8, r8
    jz      .cmp_done

    push    rdi
    push    rsi
    mov     rdi, rcx
    mov     rsi, rdx
    mov     rcx, r8

    xor     eax, eax

.cmp_loop:
    mov     r8b, byte [rdi]
    xor     r8b, byte [rsi]
    or      al, r8b
    inc     rdi
    inc     rsi
    dec     rcx
    jnz     .cmp_loop

    mov     r8b, al
    shr     r8b, 4
    or      al, r8b
    mov     r8b, al
    shr     r8b, 2
    or      al, r8b
    mov     r8b, al
    shr     r8b, 1
    or      al, r8b
    and     eax, 1

    mov     dword [r9], eax

    xor     eax, eax
    xor     ecx, ecx
    xor     edx, edx
    xor     r8d, r8d

    pop     rsi
    pop     rdi

.cmp_done:
    xor     eax, eax
    ret

.cmp_null_err:
    mov     eax, LDG_ERR_FUNC_ARG_NULL
    ret


; uint32_t ldg_mem_secure_cmov(void *dst, const void *src, uint64_t len, uint8_t cond)
; rcx=dst, rdx=src, r8=len, r9=cond (r9b)
global ldg_mem_secure_cmov
ldg_mem_secure_cmov:
    test    rcx, rcx
    jz      .cmov_null_err
    test    rdx, rdx
    jz      .cmov_null_err
    test    r8, r8
    jz      .cmov_invalid_err

    push    rdi
    push    rsi
    mov     rdi, rcx
    mov     rsi, rdx

    and     r9d, 1
    neg     r9b
    mov     rcx, r8
    xor     eax, eax

.cmov_loop:
    mov     al, byte [rdi]
    xor     al, byte [rsi]
    and     al, r9b
    xor     byte [rdi], al
    inc     rdi
    inc     rsi
    dec     rcx
    jnz     .cmov_loop

    mfence

    xor     eax, eax
    xor     ecx, ecx
    xor     edx, edx
    xor     r8d, r8d
    xor     r9d, r9d

    pop     rsi
    pop     rdi

    ret

.cmov_null_err:
    mov     eax, LDG_ERR_FUNC_ARG_NULL
    ret

.cmov_invalid_err:
    mov     eax, LDG_ERR_FUNC_ARG_INVALID
    ret


; uint8_t ldg_mem_secure_neq_is(const void *a, const void *b, uint64_t len)
; rcx=a, rdx=b, r8=len; returns 0 or 1
global ldg_mem_secure_neq_is
ldg_mem_secure_neq_is:
    test    rcx, rcx
    jz      .neq_null
    test    rdx, rdx
    jz      .neq_null
    test    r8, r8
    jz      .neq_zero_len

    push    rdi
    push    rsi
    mov     rdi, rcx
    mov     rsi, rdx
    mov     rcx, r8
    xor     eax, eax

.neq_loop:
    mov     r8b, byte [rdi]
    xor     r8b, byte [rsi]
    or      al, r8b
    inc     rdi
    inc     rsi
    dec     rcx
    jnz     .neq_loop

    mov     r8b, al
    shr     r8b, 4
    or      al, r8b
    mov     r8b, al
    shr     r8b, 2
    or      al, r8b
    mov     r8b, al
    shr     r8b, 1
    or      al, r8b
    and     eax, 1

    xor     ecx, ecx
    xor     edx, edx
    xor     r8d, r8d

    pop     rsi
    pop     rdi

    ret

.neq_null:
    mov     eax, 1
    ret

.neq_zero_len:
    xor     eax, eax
    ret
