%include "dangling/core/err.inc"

%define FRAME_ADDR 0
%define FRAME_ARG 8
%define FRAME_RET 72
%define FRAME_FP_RET 80
%define FRAME_ARG_CUNT 88
%define FRAME_FP_MASK 92
%define ARG_MAX 8
%define INT_REG_CUNT 6
%define LOCAL_SIZE 112
%define LOCAL_FP 48
%define SPILL_SIZE 16
%define SAVED_SIZE 32

section .text

global ldg_call_frame_run
ldg_call_frame_run:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    test rdi, rdi
    jz .null
    mov rbx, rdi
    cmp qword [rbx + FRAME_ADDR], 0
    je .invalid
    mov r12d, [rbx + FRAME_ARG_CUNT]
    cmp r12d, ARG_MAX
    ja .invalid
    sub rsp, LOCAL_SIZE
    xor ecx, ecx
    xor r8d, r8d
    xor r9d, r9d
    xor r10d, r10d
    mov r11d, [rbx + FRAME_FP_MASK]
.classify:
    cmp ecx, r12d
    jae .classified
    mov rdx, [rbx + FRAME_ARG + rcx * 8]
    bt r11d, ecx
    jc .fp
    cmp r8d, INT_REG_CUNT
    jae .spill
    mov [rsp + r8 * 8], rdx
    inc r8d
    jmp .next
.spill:
    test r10d, r10d
    jnz .spill_second
    mov r13, rdx
    inc r10d
    jmp .next
.spill_second:
    mov r14, rdx
    inc r10d
    jmp .next
.fp:
    mov [rsp + LOCAL_FP + r9 * 8], rdx
    inc r9d
.next:
    inc ecx
    jmp .classify
.classified:
    mov r12, [rbx + FRAME_ADDR]
    mov eax, r9d
    movq xmm0, qword [rsp + LOCAL_FP]
    movq xmm1, qword [rsp + LOCAL_FP + 8]
    movq xmm2, qword [rsp + LOCAL_FP + 16]
    movq xmm3, qword [rsp + LOCAL_FP + 24]
    movq xmm4, qword [rsp + LOCAL_FP + 32]
    movq xmm5, qword [rsp + LOCAL_FP + 40]
    movq xmm6, qword [rsp + LOCAL_FP + 48]
    movq xmm7, qword [rsp + LOCAL_FP + 56]
    mov rdi, [rsp]
    mov rsi, [rsp + 8]
    mov rdx, [rsp + 16]
    mov rcx, [rsp + 24]
    mov r8, [rsp + 32]
    mov r9, [rsp + 40]
    test r10d, r10d
    jz .call
    sub rsp, SPILL_SIZE
    mov [rsp], r13
    mov [rsp + 8], r14
.call:
    call r12
    mov [rbx + FRAME_RET], rax
    movq qword [rbx + FRAME_FP_RET], xmm0
    xor eax, eax
.done:
    lea rsp, [rbp - SAVED_SIZE]
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    jmp .done
.invalid:
    mov eax, LDG_ERR_FUNC_ARG_INVALID
    jmp .done
