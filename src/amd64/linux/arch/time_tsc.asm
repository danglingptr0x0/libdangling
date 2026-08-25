section .data
    align 64

section .bss
    align 64

section .text

global ldg_time_tsc_sample_get
ldg_time_tsc_sample_get:
    rdtscp
    shl rdx, 32
    or rax, rdx

    test rdi, rdi
    jz .done
    mov [rdi], ecx

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
    lfence
    rdtscp
    shl rdx, 32
    or rax, rdx
    lfence

    test rdi, rdi
    jz .skip_store
    mov [rdi], ecx

.skip_store:
    ret

global ldg_time_tsc_delta_get
ldg_time_tsc_delta_get:
    mov rax, rsi
    sub rax, rdi
    ret
