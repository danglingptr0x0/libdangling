%include "dangling/core/err.inc"

%macro ARGS_LD 0
    mov r9, rdx
    mov r10, rdi
    mov r11, rsi
%endmacro

%macro ARGS_LD4 0
    mov r10, rdi
    mov r11, rsi
    mov r9, rdx
    mov r8, rcx
%endmacro

%macro ARGS_LD_D1 0
    mov r10, rdi
%endmacro

section .text

global ldg_arith_uint32_add
ldg_arith_uint32_add:
    test rdx, rdx
    jz .null
    mov dword [rdx], 0
    mov eax, edi
    add eax, esi
    jc .ovf
    mov dword [rdx], eax
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
    test rdx, rdx
    jz .null
    mov dword [rdx], 0
    mov eax, edi
    sub eax, esi
    jc .ovf
    mov dword [rdx], eax
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
    test rdx, rdx
    jz .null
    mov r8, rdx
    mov dword [r8], 0
    mov eax, edi
    mul esi
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
    test rdx, rdx
    jz .null
    mov r8, rdx
    mov dword [r8], 0
    test esi, esi
    jz .divzero
    mov eax, edi
    xor edx, edx
    div esi
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
    test rdx, rdx
    jz .null
    mov qword [rdx], 0
    mov rax, rdi
    add rax, rsi
    jc .ovf
    mov qword [rdx], rax
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
    test rdx, rdx
    jz .null
    mov qword [rdx], 0
    mov rax, rdi
    sub rax, rsi
    jc .ovf
    mov qword [rdx], rax
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
    test rdx, rdx
    jz .null
    mov r8, rdx
    mov qword [r8], 0
    mov rax, rdi
    mul rsi
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
    test rdx, rdx
    jz .null
    mov r8, rdx
    mov qword [r8], 0
    test rsi, rsi
    jz .divzero
    mov rax, rdi
    xor edx, edx
    div rsi
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
