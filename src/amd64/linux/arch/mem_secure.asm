; mem_secure.asm - secure memory operations
; System V AMD64 ABI: args in rdi, rsi, rdx, rcx, r8, r9; return in rax

%include "dangling/core/err.inc"

section .data
    align 64

section .bss
    align 64

section .text

global ldg_mem_secure_zero
ldg_mem_secure_zero:
    test    rdi, rdi
    jz      .null_err
    test    rsi, rsi
    jz      .invalid_err

    mov     rcx, rsi
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
    xor     esi, esi
    xor     edi, edi
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

.done:
    ret

.null_err:
    mov     eax, LDG_ERR_FUNC_ARG_NULL
    ret

.invalid_err:
    mov     eax, LDG_ERR_FUNC_ARG_INVALID
    ret


global ldg_mem_secure_copy
ldg_mem_secure_copy:
    test    rdi, rdi
    jz      .copy_null_err
    test    rsi, rsi
    jz      .copy_null_err
    test    rdx, rdx
    jz      .copy_invalid_err

    mov     rcx, rdx

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
    xor     esi, esi
    xor     edi, edi
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

.copy_done:
    ret

.copy_null_err:
    mov     eax, LDG_ERR_FUNC_ARG_NULL
    ret

.copy_invalid_err:
    mov     eax, LDG_ERR_FUNC_ARG_INVALID
    ret


; uint32_t ldg_mem_secure_cmp(const void *a, const void *b, uint64_t len, uint32_t *result)
; rdi=a, rsi=b, rdx=len, rcx=result
global ldg_mem_secure_cmp
ldg_mem_secure_cmp:
    test    rdi, rdi
    jz      .cmp_null_err
    test    rsi, rsi
    jz      .cmp_null_err
    test    rcx, rcx
    jz      .cmp_null_err

    mov     dword [rcx], 0
    test    rdx, rdx
    jz      .cmp_done

    push    rcx
    mov     rcx, rdx
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

    pop     rcx
    mov     dword [rcx], eax

    xor     eax, eax
    xor     ecx, ecx
    xor     edx, edx
    xor     esi, esi
    xor     edi, edi
    xor     r8d, r8d

.cmp_done:
    xor     eax, eax
    ret

.cmp_null_err:
    mov     eax, LDG_ERR_FUNC_ARG_NULL
    ret


; uint32_t ldg_mem_secure_cmov(void *dst, const void *src, uint64_t len, uint8_t cond)
; rdi=dst, rsi=src, rdx=len, rcx=cond (cl)
global ldg_mem_secure_cmov
ldg_mem_secure_cmov:
    test    rdi, rdi
    jz      .cmov_null_err
    test    rsi, rsi
    jz      .cmov_null_err
    test    rdx, rdx
    jz      .cmov_invalid_err

    and     ecx, 1
    neg     cl
    mov     r8, rdx
    xor     eax, eax

.cmov_loop:
    mov     al, byte [rdi]
    xor     al, byte [rsi]
    and     al, cl
    xor     byte [rdi], al
    inc     rdi
    inc     rsi
    dec     r8
    jnz     .cmov_loop

    mfence

    xor     eax, eax
    xor     ecx, ecx
    xor     edx, edx
    xor     esi, esi
    xor     edi, edi
    xor     r8d, r8d

    ret

.cmov_null_err:
    mov     eax, LDG_ERR_FUNC_ARG_NULL
    ret

.cmov_invalid_err:
    mov     eax, LDG_ERR_FUNC_ARG_INVALID
    ret


; uint8_t ldg_mem_secure_neq_is(const void *a, const void *b, uint64_t len)
; rdi=a, rsi=b, rdx=len; returns 0 or 1
global ldg_mem_secure_neq_is
ldg_mem_secure_neq_is:
    test    rdi, rdi
    jz      .neq_null
    test    rsi, rsi
    jz      .neq_null
    test    rdx, rdx
    jz      .neq_zero_len

    mov     rcx, rdx
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
    xor     esi, esi
    xor     edi, edi
    xor     r8d, r8d

    ret

.neq_null:
    mov     eax, 1
    ret

.neq_zero_len:
    xor     eax, eax
    ret
