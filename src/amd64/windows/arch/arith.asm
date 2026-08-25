%include "dangling/core/err.inc"

%macro ARGS_LD 0
    mov r9, r8
    mov r10, rcx
    mov r11, rdx
%endmacro

%macro ARGS_LD4 0
    mov r10, rcx
    mov r11, rdx
    xchg r8, r9
%endmacro

%macro ARGS_LD_D1 0
    mov r10, rdx
%endmacro

section .text

global ldg_arith_uint32_add
ldg_arith_uint32_add:
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

global ldg_arith_uint32_sub
ldg_arith_uint32_sub:
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

global ldg_arith_uint32_mul
ldg_arith_uint32_mul:
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

global ldg_arith_uint32_div
ldg_arith_uint32_div:
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

global ldg_arith_uint64_add
ldg_arith_uint64_add:
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

global ldg_arith_uint64_sub
ldg_arith_uint64_sub:
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

global ldg_arith_uint64_mul
ldg_arith_uint64_mul:
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

global ldg_arith_uint64_div
ldg_arith_uint64_div:
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

%include "arith_uint128_256.inc"
