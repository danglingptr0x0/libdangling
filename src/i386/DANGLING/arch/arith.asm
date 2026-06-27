%include "dangling/core/err.inc"

section .text

global ldg_arith_32_add
ldg_arith_32_add:
    mov ecx, [esp+12]
    test ecx, ecx
    jz .null
    mov dword [ecx], 0
    mov eax, [esp+4]
    add eax, [esp+8]
    jc .ovf
    mov [ecx], eax
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
    mov ecx, [esp+12]
    test ecx, ecx
    jz .null
    mov dword [ecx], 0
    mov eax, [esp+4]
    sub eax, [esp+8]
    jc .ovf
    mov [ecx], eax
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
    mov ecx, [esp+12]
    test ecx, ecx
    jz .null
    mov dword [ecx], 0
    mov eax, [esp+4]
    mul dword [esp+8]
    jc .ovf
    mov [ecx], eax
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
    mov ecx, [esp+12]
    test ecx, ecx
    jz .null
    mov dword [ecx], 0
    cmp dword [esp+8], 0
    je .divzero
    mov eax, [esp+4]
    xor edx, edx
    div dword [esp+8]
    mov [ecx], eax
    xor eax, eax
    ret
.divzero:
    mov eax, LDG_ERR_FUNC_ARG_INVALID
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret
