; mem_secure.asm - secure memory operations
; Microsoft x64 ABI: args in rcx, rdx, r8, r9; return in rax
; Callee-saved: rbx, rbp, rdi, rsi, r12-r15

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
    jz      .done
    test    rdx, rdx
    jz      .done

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


; void ldg_mem_secure_copy(void *dst, const void *src, uint64_t len)
; rcx=dst, rdx=src, r8=len
global ldg_mem_secure_copy
ldg_mem_secure_copy:
    test    rcx, rcx
    jz      .copy_done
    test    rdx, rdx
    jz      .copy_done
    test    r8, r8
    jz      .copy_done

    push    rdi
    push    rsi
    mov     rdi, rcx
    mov     rsi, rdx
    mov     rcx, r8

    push    rsi
    push    rcx

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
    jz      .copy_clear_src

.copy_byte_loop:
    mov     al, byte [rsi]
    mov     byte [rdi], al
    inc     rdi
    inc     rsi
    dec     rcx
    jnz     .copy_byte_loop

.copy_clear_src:
    pop     rcx
    pop     rsi
    mov     rdi, rsi
    xor     eax, eax

    mov     r8, rcx
    shr     rcx, 3
    test    rcx, rcx
    jz      .clear_src_byte_setup

.clear_src_qword_loop:
    mov     qword [rdi], rax
    add     rdi, 8
    dec     rcx
    jnz     .clear_src_qword_loop

.clear_src_byte_setup:
    mov     rcx, r8
    and     rcx, 7
    test    rcx, rcx
    jz      .copy_clear_regs

.clear_src_byte_loop:
    mov     byte [rdi], al
    inc     rdi
    dec     rcx
    jnz     .clear_src_byte_loop

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


; uint32_t ldg_mem_secure_cmp(const void *a, const void *b, uint64_t len)
; rcx=a, rdx=b, r8=len
global ldg_mem_secure_cmp
ldg_mem_secure_cmp:
    test    rcx, rcx
    jz      .cmp_fail
    test    rdx, rdx
    jz      .cmp_fail
    test    r8, r8
    jz      .cmp_equal

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

    xor     ecx, ecx
    xor     edx, edx
    xor     r8d, r8d
    movzx   eax, al

    pop     rsi
    pop     rdi
    ret

.cmp_fail:
    mov     eax, -1
    ret

.cmp_equal:
    xor     eax, eax
    ret
