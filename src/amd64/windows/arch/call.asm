%include "dangling/core/err.inc"

%define FRAME_ADDR 0
%define FRAME_ARG 8
%define FRAME_RET 72
%define FRAME_FP_RET 80
%define STACK_SIZE 0x50
%define STACK_ARG 0x20
%define UNWIND_REV 1
%define UNWIND_CODE_CUNT 2
%define UNWIND_NO_FRAME_REG 0
%define UNWIND_ALLOC_SMALL 0x92
%define UNWIND_PUSH_RBX 0x30

section .text

global ldg_call_frame_run
ldg_call_frame_run:
    push rbx
ldg_call_frame_run_pushed:
    sub rsp, STACK_SIZE
ldg_call_frame_run_body:
    test rcx, rcx
    jz .null
    mov rbx, rcx
    mov rax, [rbx + FRAME_ADDR]
    test rax, rax
    jz .invalid
    mov r10, [rbx + FRAME_ARG + 32]
    mov [rsp + STACK_ARG], r10
    mov r10, [rbx + FRAME_ARG + 40]
    mov [rsp + STACK_ARG + 8], r10
    mov r10, [rbx + FRAME_ARG + 48]
    mov [rsp + STACK_ARG + 16], r10
    mov r10, [rbx + FRAME_ARG + 56]
    mov [rsp + STACK_ARG + 24], r10
    mov rcx, [rbx + FRAME_ARG]
    movq xmm0, rcx
    mov rdx, [rbx + FRAME_ARG + 8]
    movq xmm1, rdx
    mov r8, [rbx + FRAME_ARG + 16]
    movq xmm2, r8
    mov r9, [rbx + FRAME_ARG + 24]
    movq xmm3, r9
    call rax
    mov [rbx + FRAME_RET], rax
    movq qword [rbx + FRAME_FP_RET], xmm0
    xor eax, eax
.done:
    add rsp, STACK_SIZE
    pop rbx
    ret
.null:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    jmp .done
.invalid:
    mov eax, LDG_ERR_FUNC_ARG_INVALID
    jmp .done
ldg_call_frame_run_end:

section .pdata rdata align=4
    dd ldg_call_frame_run wrt ..imagebase
    dd ldg_call_frame_run_end wrt ..imagebase
    dd ldg_call_frame_run_unwind wrt ..imagebase

section .xdata rdata align=8
ldg_call_frame_run_unwind:
    db UNWIND_REV
    db ldg_call_frame_run_body - ldg_call_frame_run
    db UNWIND_CODE_CUNT
    db UNWIND_NO_FRAME_REG
    db ldg_call_frame_run_body - ldg_call_frame_run
    db UNWIND_ALLOC_SMALL
    db ldg_call_frame_run_pushed - ldg_call_frame_run
    db UNWIND_PUSH_RBX
