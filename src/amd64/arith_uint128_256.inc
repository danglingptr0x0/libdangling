section .text

%define LDG_ARITH_ORD_LT 0
%define LDG_ARITH_ORD_EQ 1
%define LDG_ARITH_ORD_GT 2

global ldg_arith_uint128_add
ldg_arith_uint128_add:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    mov rax, [r10]
    add rax, [r11]
    mov rcx, [r10+8]
    adc rcx, [r11+8]
    jc .ovf
    mov [r9], rax
    mov [r9+8], rcx
    xor eax, eax
    ret
.ovf:
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    mov eax, LDG_ERR_OVERFLOW
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint128_sub
ldg_arith_uint128_sub:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    mov rax, [r10]
    sub rax, [r11]
    mov rcx, [r10+8]
    sbb rcx, [r11+8]
    jc .ovf
    mov [r9], rax
    mov [r9+8], rcx
    xor eax, eax
    ret
.ovf:
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    mov eax, LDG_ERR_OVERFLOW
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint128_mul
ldg_arith_uint128_mul:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    push rbx
    push r12
    xor ecx, ecx
    xor r8d, r8d
    xor ebx, ebx
    xor r12d, r12d
    mov rax, [r10]
    mul qword [r11]
    add rcx, rax
    adc r8, rdx
    adc rbx, 0
    adc r12, 0
    mov rax, [r10]
    mul qword [r11+8]
    add r8, rax
    adc rbx, rdx
    adc r12, 0
    mov rax, [r10+8]
    mul qword [r11]
    add r8, rax
    adc rbx, rdx
    adc r12, 0
    mov rax, [r10+8]
    mul qword [r11+8]
    add rbx, rax
    adc r12, rdx
    mov rax, rbx
    or rax, r12
    jnz .ovf
    mov [r9], rcx
    mov [r9+8], r8
    pop r12
    pop rbx
    xor eax, eax
    ret
.ovf:
    pop r12
    pop rbx
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    mov eax, LDG_ERR_OVERFLOW
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint128_div
ldg_arith_uint128_div:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    mov rax, [r11]
    or rax, [r11+8]
    jz .divzero
    push rbx
    push r12
    push r13
    push r14
    mov rax, [r10]
    mov rcx, [r10+8]
    xor edx, edx
    xor r8d, r8d
    mov r14d, 128
.loop:
    add rax, rax
    adc rcx, rcx
    adc rdx, rdx
    adc r8, r8
    setc r13b
    movzx r13d, r13b
    mov rbx, rdx
    mov r12, r8
    sub rbx, [r11]
    sbb r12, [r11+8]
    sbb r13, 0
    jc .skip
    mov rdx, rbx
    mov r8, r12
    or rax, 1
.skip:
    dec r14d
    jnz .loop
    mov [r9], rax
    mov [r9+8], rcx
    pop r14
    pop r13
    pop r12
    pop rbx
    xor eax, eax
    ret
.divzero:
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    mov eax, LDG_ERR_FUNC_ARG_INVALID
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint256_add
ldg_arith_uint256_add:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    mov rax, [r10]
    add rax, [r11]
    mov rcx, [r10+8]
    adc rcx, [r11+8]
    mov rdx, [r10+16]
    adc rdx, [r11+16]
    mov r8, [r10+24]
    adc r8, [r11+24]
    jc .ovf
    mov [r9], rax
    mov [r9+8], rcx
    mov [r9+16], rdx
    mov [r9+24], r8
    xor eax, eax
    ret
.ovf:
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    mov [r9+16], rax
    mov [r9+24], rax
    mov eax, LDG_ERR_OVERFLOW
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint256_sub
ldg_arith_uint256_sub:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    mov rax, [r10]
    sub rax, [r11]
    mov rcx, [r10+8]
    sbb rcx, [r11+8]
    mov rdx, [r10+16]
    sbb rdx, [r11+16]
    mov r8, [r10+24]
    sbb r8, [r11+24]
    jc .ovf
    mov [r9], rax
    mov [r9+8], rcx
    mov [r9+16], rdx
    mov [r9+24], r8
    xor eax, eax
    ret
.ovf:
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    mov [r9+16], rax
    mov [r9+24], rax
    mov eax, LDG_ERR_OVERFLOW
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

%macro AR_MULADD 3
    mov rax, [r10 + %1]
    mul qword [r11 + %2]
    add qword [rsp + %3*8], rax
    adc qword [rsp + (%3+1)*8], rdx
    %assign _ci (%3)+2
    %rep 6-(%3)
        adc qword [rsp + _ci*8], 0
        %assign _ci _ci+1
    %endrep
%endmacro

global ldg_arith_uint256_mul
ldg_arith_uint256_mul:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    sub rsp, 64
    xor eax, eax
    mov [rsp], rax
    mov [rsp+8], rax
    mov [rsp+16], rax
    mov [rsp+24], rax
    mov [rsp+32], rax
    mov [rsp+40], rax
    mov [rsp+48], rax
    mov [rsp+56], rax
    AR_MULADD 0, 0, 0
    AR_MULADD 0, 8, 1
    AR_MULADD 0, 16, 2
    AR_MULADD 0, 24, 3
    AR_MULADD 8, 0, 1
    AR_MULADD 8, 8, 2
    AR_MULADD 8, 16, 3
    AR_MULADD 8, 24, 4
    AR_MULADD 16, 0, 2
    AR_MULADD 16, 8, 3
    AR_MULADD 16, 16, 4
    AR_MULADD 16, 24, 5
    AR_MULADD 24, 0, 3
    AR_MULADD 24, 8, 4
    AR_MULADD 24, 16, 5
    AR_MULADD 24, 24, 6
    mov rax, [rsp+32]
    or rax, [rsp+40]
    or rax, [rsp+48]
    or rax, [rsp+56]
    jnz .ovf
    mov rax, [rsp]
    mov [r9], rax
    mov rax, [rsp+8]
    mov [r9+8], rax
    mov rax, [rsp+16]
    mov [r9+16], rax
    mov rax, [rsp+24]
    mov [r9+24], rax
    add rsp, 64
    xor eax, eax
    ret
.ovf:
    add rsp, 64
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    mov [r9+16], rax
    mov [r9+24], rax
    mov eax, LDG_ERR_OVERFLOW
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint256_div
ldg_arith_uint256_div:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    mov rax, [r11]
    or rax, [r11+8]
    or rax, [r11+16]
    or rax, [r11+24]
    jz .divzero
    push rbx
    push r12
    sub rsp, 64
    mov rax, [r10]
    mov [rsp], rax
    mov rax, [r10+8]
    mov [rsp+8], rax
    mov rax, [r10+16]
    mov [rsp+16], rax
    mov rax, [r10+24]
    mov [rsp+24], rax
    xor eax, eax
    mov [rsp+32], rax
    mov [rsp+40], rax
    mov [rsp+48], rax
    mov [rsp+56], rax
    mov ecx, 256
.loop:
    shl qword [rsp], 1
    rcl qword [rsp+8], 1
    rcl qword [rsp+16], 1
    rcl qword [rsp+24], 1
    rcl qword [rsp+32], 1
    rcl qword [rsp+40], 1
    rcl qword [rsp+48], 1
    rcl qword [rsp+56], 1
    setc r12b
    movzx r12d, r12b
    mov rax, [rsp+32]
    mov rdx, [rsp+40]
    mov r8, [rsp+48]
    mov rbx, [rsp+56]
    sub rax, [r11]
    sbb rdx, [r11+8]
    sbb r8, [r11+16]
    sbb rbx, [r11+24]
    sbb r12, 0
    jc .skip
    mov [rsp+32], rax
    mov [rsp+40], rdx
    mov [rsp+48], r8
    mov [rsp+56], rbx
    or qword [rsp], 1
.skip:
    dec ecx
    jnz .loop
    mov rax, [rsp]
    mov [r9], rax
    mov rax, [rsp+8]
    mov [r9+8], rax
    mov rax, [rsp+16]
    mov [r9+16], rax
    mov rax, [rsp+24]
    mov [r9+24], rax
    add rsp, 64
    pop r12
    pop rbx
    xor eax, eax
    ret
.divzero:
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    mov [r9+16], rax
    mov [r9+24], rax
    mov eax, LDG_ERR_FUNC_ARG_INVALID
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint128_cmp
ldg_arith_uint128_cmp:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    mov rax, [r10+8]
    mov rcx, [r11+8]
    cmp rax, rcx
    ja .gt
    jb .lt
    mov rax, [r10]
    mov rcx, [r11]
    cmp rax, rcx
    ja .gt
    jb .lt
    mov dword [r9], LDG_ARITH_ORD_EQ
    xor eax, eax
    ret
.gt:
    mov dword [r9], LDG_ARITH_ORD_GT
    xor eax, eax
    ret
.lt:
    mov dword [r9], LDG_ARITH_ORD_LT
    xor eax, eax
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint256_cmp
ldg_arith_uint256_cmp:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    mov rax, [r10+24]
    mov rcx, [r11+24]
    cmp rax, rcx
    ja .gt
    jb .lt
    mov rax, [r10+16]
    mov rcx, [r11+16]
    cmp rax, rcx
    ja .gt
    jb .lt
    mov rax, [r10+8]
    mov rcx, [r11+8]
    cmp rax, rcx
    ja .gt
    jb .lt
    mov rax, [r10]
    mov rcx, [r11]
    cmp rax, rcx
    ja .gt
    jb .lt
    mov dword [r9], LDG_ARITH_ORD_EQ
    xor eax, eax
    ret
.gt:
    mov dword [r9], LDG_ARITH_ORD_GT
    xor eax, eax
    ret
.lt:
    mov dword [r9], LDG_ARITH_ORD_LT
    xor eax, eax
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint128_to_uint256_mul
ldg_arith_uint128_to_uint256_mul:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    push rbx
    push r12
    xor ecx, ecx
    xor r8d, r8d
    xor ebx, ebx
    xor r12d, r12d
    mov rax, [r10]
    mul qword [r11]
    add rcx, rax
    adc r8, rdx
    adc rbx, 0
    adc r12, 0
    mov rax, [r10]
    mul qword [r11+8]
    add r8, rax
    adc rbx, rdx
    adc r12, 0
    mov rax, [r10+8]
    mul qword [r11]
    add r8, rax
    adc rbx, rdx
    adc r12, 0
    mov rax, [r10+8]
    mul qword [r11+8]
    add rbx, rax
    adc r12, rdx
    mov [r9], rcx
    mov [r9+8], r8
    mov [r9+16], rbx
    mov [r9+24], r12
    pop r12
    pop rbx
    xor eax, eax
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint128_div_compute
ldg_arith_uint128_div_compute:
    ARGS_LD4
    test r9, r9
    jz .null
    test r8, r8
    jz .null
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    mov rax, [r11]
    or rax, [r11+8]
    jz .divzero
    push rbx
    push r12
    push r13
    push r14
    push r15
    mov r15, r8
    mov rax, [r10]
    mov rcx, [r10+8]
    xor edx, edx
    xor r8d, r8d
    mov r14d, 128
.loop:
    add rax, rax
    adc rcx, rcx
    adc rdx, rdx
    adc r8, r8
    setc r13b
    movzx r13d, r13b
    mov rbx, rdx
    mov r12, r8
    sub rbx, [r11]
    sbb r12, [r11+8]
    sbb r13, 0
    jc .skip
    mov rdx, rbx
    mov r8, r12
    or rax, 1
.skip:
    dec r14d
    jnz .loop
    mov [r9], rax
    mov [r9+8], rcx
    mov [r15], rdx
    mov [r15+8], r8
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    xor eax, eax
    ret
.divzero:
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    mov [r8], rax
    mov [r8+8], rax
    mov eax, LDG_ERR_FUNC_ARG_INVALID
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint256_div_compute
ldg_arith_uint256_div_compute:
    ARGS_LD4
    test r9, r9
    jz .null
    test r8, r8
    jz .null
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    mov rax, [r11]
    or rax, [r11+8]
    or rax, [r11+16]
    or rax, [r11+24]
    jz .divzero
    push rbx
    push r12
    push r13
    mov r13, r8
    sub rsp, 64
    mov rax, [r10]
    mov [rsp], rax
    mov rax, [r10+8]
    mov [rsp+8], rax
    mov rax, [r10+16]
    mov [rsp+16], rax
    mov rax, [r10+24]
    mov [rsp+24], rax
    xor eax, eax
    mov [rsp+32], rax
    mov [rsp+40], rax
    mov [rsp+48], rax
    mov [rsp+56], rax
    mov ecx, 256
.loop:
    shl qword [rsp], 1
    rcl qword [rsp+8], 1
    rcl qword [rsp+16], 1
    rcl qword [rsp+24], 1
    rcl qword [rsp+32], 1
    rcl qword [rsp+40], 1
    rcl qword [rsp+48], 1
    rcl qword [rsp+56], 1
    setc r12b
    movzx r12d, r12b
    mov rax, [rsp+32]
    mov rdx, [rsp+40]
    mov r8, [rsp+48]
    mov rbx, [rsp+56]
    sub rax, [r11]
    sbb rdx, [r11+8]
    sbb r8, [r11+16]
    sbb rbx, [r11+24]
    sbb r12, 0
    jc .skip
    mov [rsp+32], rax
    mov [rsp+40], rdx
    mov [rsp+48], r8
    mov [rsp+56], rbx
    or qword [rsp], 1
.skip:
    dec ecx
    jnz .loop
    mov rax, [rsp]
    mov [r9], rax
    mov rax, [rsp+8]
    mov [r9+8], rax
    mov rax, [rsp+16]
    mov [r9+16], rax
    mov rax, [rsp+24]
    mov [r9+24], rax
    mov rax, [rsp+32]
    mov [r13], rax
    mov rax, [rsp+40]
    mov [r13+8], rax
    mov rax, [rsp+48]
    mov [r13+16], rax
    mov rax, [rsp+56]
    mov [r13+24], rax
    add rsp, 64
    pop r13
    pop r12
    pop rbx
    xor eax, eax
    ret
.divzero:
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    mov [r9+16], rax
    mov [r9+24], rax
    mov [r8], rax
    mov [r8+8], rax
    mov [r8+16], rax
    mov [r8+24], rax
    mov eax, LDG_ERR_FUNC_ARG_INVALID
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint128_shl
ldg_arith_uint128_shl:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    cmp r11d, 128
    jae .zero
    mov rax, [r10]
    mov r8, [r10+8]
    mov ecx, r11d
    cmp cl, 64
    jae .big
    shld r8, rax, cl
    shl rax, cl
    mov [r9], rax
    mov [r9+8], r8
    xor eax, eax
    ret
.big:
    sub cl, 64
    shl rax, cl
    xor edx, edx
    mov [r9], rdx
    mov [r9+8], rax
    xor eax, eax
    ret
.zero:
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint128_shr
ldg_arith_uint128_shr:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    cmp r11d, 128
    jae .zero
    mov rax, [r10]
    mov r8, [r10+8]
    mov ecx, r11d
    cmp cl, 64
    jae .big
    shrd rax, r8, cl
    shr r8, cl
    mov [r9], rax
    mov [r9+8], r8
    xor eax, eax
    ret
.big:
    sub cl, 64
    shr r8, cl
    xor edx, edx
    mov [r9], r8
    mov [r9+8], rdx
    xor eax, eax
    ret
.zero:
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint256_shl
ldg_arith_uint256_shl:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    cmp r11d, 256
    jae .zero
    mov rax, [r10]
    mov rdx, [r10+8]
    mov r8, [r10+16]
    mov r10, [r10+24]
    mov ecx, r11d
    and ecx, 63
    shr r11d, 6
    cmp r11d, 0
    je .ls0
    cmp r11d, 1
    je .ls1
    cmp r11d, 2
    je .ls2
    jmp .ls3
.ls0:
    shld r10, r8, cl
    shld r8, rdx, cl
    shld rdx, rax, cl
    shl rax, cl
    mov [r9], rax
    mov [r9+8], rdx
    mov [r9+16], r8
    mov [r9+24], r10
    xor eax, eax
    ret
.ls1:
    shld r8, rdx, cl
    shld rdx, rax, cl
    shl rax, cl
    xor r11d, r11d
    mov [r9], r11
    mov [r9+8], rax
    mov [r9+16], rdx
    mov [r9+24], r8
    xor eax, eax
    ret
.ls2:
    shld rdx, rax, cl
    shl rax, cl
    xor r11d, r11d
    mov [r9], r11
    mov [r9+8], r11
    mov [r9+16], rax
    mov [r9+24], rdx
    xor eax, eax
    ret
.ls3:
    shl rax, cl
    xor r11d, r11d
    mov [r9], r11
    mov [r9+8], r11
    mov [r9+16], r11
    mov [r9+24], rax
    xor eax, eax
    ret
.zero:
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    mov [r9+16], rax
    mov [r9+24], rax
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint256_shr
ldg_arith_uint256_shr:
    ARGS_LD
    test r9, r9
    jz .null
    test r10, r10
    jz .null
    cmp r11d, 256
    jae .zero
    mov rax, [r10]
    mov rdx, [r10+8]
    mov r8, [r10+16]
    mov r10, [r10+24]
    mov ecx, r11d
    and ecx, 63
    shr r11d, 6
    cmp r11d, 0
    je .ls0
    cmp r11d, 1
    je .ls1
    cmp r11d, 2
    je .ls2
    jmp .ls3
.ls0:
    shrd rax, rdx, cl
    shrd rdx, r8, cl
    shrd r8, r10, cl
    shr r10, cl
    mov [r9], rax
    mov [r9+8], rdx
    mov [r9+16], r8
    mov [r9+24], r10
    xor eax, eax
    ret
.ls1:
    shrd rdx, r8, cl
    shrd r8, r10, cl
    shr r10, cl
    xor r11d, r11d
    mov [r9], rdx
    mov [r9+8], r8
    mov [r9+16], r10
    mov [r9+24], r11
    xor eax, eax
    ret
.ls2:
    shrd r8, r10, cl
    shr r10, cl
    xor r11d, r11d
    mov [r9], r8
    mov [r9+8], r10
    mov [r9+16], r11
    mov [r9+24], r11
    xor eax, eax
    ret
.ls3:
    shr r10, cl
    xor r11d, r11d
    mov [r9], r10
    mov [r9+8], r11
    mov [r9+16], r11
    mov [r9+24], r11
    xor eax, eax
    ret
.zero:
    xor eax, eax
    mov [r9], rax
    mov [r9+8], rax
    mov [r9+16], rax
    mov [r9+24], rax
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_uint128_to_double_convert
ldg_arith_uint128_to_double_convert:
    ARGS_LD
    test r10, r10
    jz .null
    test r11, r11
    jz .null
    mov rax, [r10]
    mov rdx, [r10+8]
    test rdx, rdx
    jnz .big
    test rax, rax
    js .lo_odd
    cvtsi2sd xmm0, rax
    movq [r11], xmm0
    xor eax, eax
    ret
.lo_odd:
    mov rcx, rax
    and ecx, 1
    shr rax, 1
    or rax, rcx
    cvtsi2sd xmm0, rax
    addsd xmm0, xmm0
    movq [r11], xmm0
    xor eax, eax
    ret
.big:
    bsr rcx, rdx
    lea r8, [rcx+1]
    cmp r8d, 64
    je .k64
    mov r9, rax
    mov ecx, 64
    sub ecx, r8d
    shl r9, cl
    mov ecx, r8d
    shrd rax, rdx, cl
    test r9, r9
    jz .conv
    or rax, 1
    jmp .conv
.k64:
    test rax, rax
    mov rax, rdx
    jz .conv
    or rax, 1
.conv:
    mov rcx, rax
    and ecx, 1
    shr rax, 1
    or rax, rcx
    cvtsi2sd xmm0, rax
    addsd xmm0, xmm0
    mov rcx, r8
    add rcx, 1023
    shl rcx, 52
    movq xmm1, rcx
    mulsd xmm0, xmm1
    movq [r11], xmm0
    xor eax, eax
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_arith_double_to_uint128_convert
ldg_arith_double_to_uint128_convert:
    ARGS_LD_D1
    test r10, r10
    jz .null
    ucomisd xmm0, xmm0
    jp .invalid
    xorpd xmm1, xmm1
    ucomisd xmm0, xmm1
    jb .invalid
    mov rcx, 1151
    shl rcx, 52
    movq xmm1, rcx
    ucomisd xmm0, xmm1
    jae .overflow
    movq rax, xmm0
    mov rcx, rax
    shr rcx, 52
    and ecx, 0x7FF
    test ecx, ecx
    jz .zero
    sub ecx, 1023
    js .zero
    mov rdx, rax
    mov r8, 0x000FFFFFFFFFFFFF
    and rdx, r8
    mov r8, 1
    shl r8, 52
    or rdx, r8
    sub ecx, 52
    js .shift_down
    cmp ecx, 64
    jae .shift_up_big
    mov rax, rdx
    xor r8d, r8d
    shld r8, rdx, cl
    shl rax, cl
    mov [r10], rax
    mov [r10+8], r8
    xor eax, eax
    ret
.shift_up_big:
    sub ecx, 64
    mov rax, rdx
    shl rax, cl
    xor edx, edx
    mov [r10], rdx
    mov [r10+8], rax
    xor eax, eax
    ret
.shift_down:
    neg ecx
    shr rdx, cl
    mov [r10], rdx
    xor eax, eax
    mov [r10+8], rax
    xor eax, eax
    ret
.zero:
    xor eax, eax
    mov [r10], rax
    mov [r10+8], rax
    ret
.overflow:
    xor eax, eax
    mov [r10], rax
    mov [r10+8], rax
    mov eax, LDG_ERR_OVERFLOW
    ret
.invalid:
    xor eax, eax
    mov [r10], rax
    mov [r10+8], rax
    mov eax, LDG_ERR_FUNC_ARG_INVALID
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret
