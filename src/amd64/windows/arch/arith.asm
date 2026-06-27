%include "dangling/core/err.inc"

section .text

global ldg_arith_32_add
ldg_arith_32_add:
    test r8, r8
    jz .null
    mov dword [r8], 0
    mov eax, ecx
    add eax, edx
    jc .ovf
    mov dword [r8], eax
    xor eax, eax
    ret
.ovf:
    mov eax, LDG_ERR_OVERFLOW
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_32_sub
ldg_arith_32_sub:
    test r8, r8
    jz .null
    mov dword [r8], 0
    mov eax, ecx
    sub eax, edx
    jc .ovf
    mov dword [r8], eax
    xor eax, eax
    ret
.ovf:
    mov eax, LDG_ERR_OVERFLOW
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_32_mul
ldg_arith_32_mul:
    test r8, r8
    jz .null
    mov dword [r8], 0
    mov eax, ecx
    mul edx
    jc .ovf
    mov dword [r8], eax
    xor eax, eax
    ret
.ovf:
    mov eax, LDG_ERR_OVERFLOW
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_32_div
ldg_arith_32_div:
    test r8, r8
    jz .null
    mov dword [r8], 0
    test edx, edx
    jz .divzero
    mov r9d, edx
    mov eax, ecx
    xor edx, edx
    div r9d
    mov dword [r8], eax
    xor eax, eax
    ret
.divzero:
    mov eax, LDG_ERR_FUNC_ARG_INVALID
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_64_add
ldg_arith_64_add:
    test r8, r8
    jz .null
    mov qword [r8], 0
    mov rax, rcx
    add rax, rdx
    jc .ovf
    mov qword [r8], rax
    xor eax, eax
    ret
.ovf:
    mov eax, LDG_ERR_OVERFLOW
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_64_sub
ldg_arith_64_sub:
    test r8, r8
    jz .null
    mov qword [r8], 0
    mov rax, rcx
    sub rax, rdx
    jc .ovf
    mov qword [r8], rax
    xor eax, eax
    ret
.ovf:
    mov eax, LDG_ERR_OVERFLOW
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_64_mul
ldg_arith_64_mul:
    test r8, r8
    jz .null
    mov qword [r8], 0
    mov rax, rcx
    mul rdx
    jc .ovf
    mov qword [r8], rax
    xor eax, eax
    ret
.ovf:
    mov eax, LDG_ERR_OVERFLOW
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_64_div
ldg_arith_64_div:
    test r8, r8
    jz .null
    mov qword [r8], 0
    test rdx, rdx
    jz .divzero
    mov r9, rdx
    mov rax, rcx
    xor edx, edx
    div r9
    mov qword [r8], rax
    xor eax, eax
    ret
.divzero:
    mov eax, LDG_ERR_FUNC_ARG_INVALID
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret
