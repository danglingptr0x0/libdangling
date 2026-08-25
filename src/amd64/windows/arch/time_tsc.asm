section .data
    align 64

section .bss
    align 64

section .text

global ldg_time_tsc_sample_get
ldg_time_tsc_sample_get:
    mov r8, rcx

    rdtscp
    shl rdx, 32
    or rax, rdx

    test r8, r8
    jz .done
    mov [r8], ecx

.done:
    ret

global ldg_time_tsc_serialize
ldg_time_tsc_serialize:
    push rbx
    xor eax, eax
    cpuid
    pop rbx
    ret

global ldg_time_tsc_serialized_sample_get
ldg_time_tsc_serialized_sample_get:
    mov r8, rcx

    lfence
    rdtscp
    shl rdx, 32
    or rax, rdx
    lfence

    test r8, r8
    jz .skip_store
    mov [r8], ecx

.skip_store:
    ret

global ldg_time_tsc_delta_get
ldg_time_tsc_delta_get:
    mov rax, rdx
    sub rax, rcx
    ret
