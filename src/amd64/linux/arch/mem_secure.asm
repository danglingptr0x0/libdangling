; mem_secure.asm - secure memory operations
; System V AMD64 ABI: args in rdi, rsi, rdx, rcx, r8, r9; return in rax

section .data
    align 64

section .bss
    align 64

section .text

global ldg_mem_secure_zero
ldg_mem_secure_zero:
    test    rdi, rdi
    jz      .done
    test    rsi, rsi
    jz      .done

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


global ldg_mem_secure_copy
ldg_mem_secure_copy:
    test    rdi, rdi
    jz      .copy_done
    test    rsi, rsi
    jz      .copy_done
    test    rdx, rdx
    jz      .copy_done

    push    rsi
    push    rdx
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
    jz      .copy_clear_src

.copy_byte_loop:
    mov     al, byte [rsi]
    mov     byte [rdi], al
    inc     rdi
    inc     rsi
    dec     rcx
    jnz     .copy_byte_loop

.copy_clear_src:
    pop     rdx
    pop     rsi
    mov     rdi, rsi
    mov     rcx, rdx
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


global ldg_mem_secure_cmp
ldg_mem_secure_cmp:
    test    rdi, rdi
    jz      .cmp_fail
    test    rsi, rsi
    jz      .cmp_fail
    test    rdx, rdx
    jz      .cmp_equal

    xor     eax, eax
    mov     rcx, rdx

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
    xor     esi, esi
    xor     edi, edi
    xor     r8d, r8d
    movzx   eax, al
    ret

.cmp_fail:
    mov     eax, -1
    ret

.cmp_equal:
    xor     eax, eax
    ret
