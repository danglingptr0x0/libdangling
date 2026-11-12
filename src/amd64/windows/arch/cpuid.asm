%include "dangling/core/err.inc"

section .text

global ldg_cpuid
ldg_cpuid:
    test r8, r8
    jz .cpuid_null_err
    push rbx
    mov eax, ecx
    mov ecx, edx
    mov r9, r8

    cpuid

    mov [r9], eax
    mov [r9 + 4], ebx
    mov [r9 + 8], ecx
    mov [r9 + 12], edx

    xor eax, eax
    pop rbx
    ret

.cpuid_null_err:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_cpuid_vendor_get
ldg_cpuid_vendor_get:
    test rcx, rcx
    jz .vendor_null_err
    push rbx
    mov r8, rcx

    xor eax, eax
    cpuid

    mov [r8], ebx
    mov [r8 + 4], edx
    mov [r8 + 8], ecx
    mov byte [r8 + 12], 0

    xor eax, eax
    pop rbx
    ret

.vendor_null_err:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_cpuid_brand_get
ldg_cpuid_brand_get:
    test rcx, rcx
    jz .brand_null_err
    push rbx
    push r12
    mov r12, rcx

    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000004
    jb .no_brand

    mov eax, 0x80000002
    cpuid
    mov [r12], eax
    mov [r12 + 4], ebx
    mov [r12 + 8], ecx
    mov [r12 + 12], edx

    mov eax, 0x80000003
    cpuid
    mov [r12 + 16], eax
    mov [r12 + 20], ebx
    mov [r12 + 24], ecx
    mov [r12 + 28], edx

    mov eax, 0x80000004
    cpuid
    mov [r12 + 32], eax
    mov [r12 + 36], ebx
    mov [r12 + 40], ecx
    mov [r12 + 44], edx

    mov byte [r12 + 48], 0
    jmp .done

.no_brand:
    mov byte [r12], 0

.done:
    xor eax, eax
    pop r12
    pop rbx
    ret

.brand_null_err:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

%define FEAT_PACK_SHIFT 10

global ldg_cpuid_feat_get
ldg_cpuid_feat_get:
    test rcx, rcx
    jz .feat_null_err
    push rbx
    push r12
    mov r12, rcx

    xor eax, eax
    mov [r12], eax

    mov eax, 1
    cpuid

    xor r8d, r8d

    bt edx, 25
    rcr r8d, 1
    bt edx, 26
    rcr r8d, 1
    bt ecx, 0
    rcr r8d, 1
    bt ecx, 9
    rcr r8d, 1
    bt ecx, 19
    rcr r8d, 1
    bt ecx, 20
    rcr r8d, 1
    bt ecx, 28
    rcr r8d, 1

    mov r9d, ecx
    mov r10d, edx

    xor eax, eax
    cpuid
    xor ebx, ebx
    cmp eax, 7
    jb .feat_ext_skip
    mov eax, 7
    xor ecx, ecx
    cpuid
.feat_ext_skip:

    bt ebx, 5
    rcr r8d, 1
    bt ebx, 16
    rcr r8d, 1

    bt r9d, 25
    rcr r8d, 1
    bt r9d, 1
    rcr r8d, 1
    bt r9d, 30
    rcr r8d, 1
    bt ebx, 18
    rcr r8d, 1
    bt ebx, 3
    rcr r8d, 1
    bt ebx, 8
    rcr r8d, 1
    bt r9d, 23
    rcr r8d, 1
    bt r9d, 12
    rcr r8d, 1
    bt r9d, 29
    rcr r8d, 1
    bt r10d, 8
    rcr r8d, 1
    bt r9d, 13
    rcr r8d, 1
    bt r10d, 28
    rcr r8d, 1

    mov eax, 0x80000000
    cpuid
    xor edx, edx
    cmp eax, 0x80000007
    jb .feat_pwr_skip
    mov eax, 0x80000007
    cpuid
.feat_pwr_skip:
    bt edx, 8
    rcr r8d, 1

    shr r8d, FEAT_PACK_SHIFT
    mov [r12], r8d

    xor eax, eax
    pop r12
    pop rbx
    ret

.feat_null_err:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_cpuid_max_leaf_get
ldg_cpuid_max_leaf_get:
    push rbx
    xor eax, eax
    cpuid
    pop rbx
    ret

global ldg_cpuid_max_ext_leaf_get
ldg_cpuid_max_ext_leaf_get:
    push rbx
    mov eax, 0x80000000
    cpuid
    pop rbx
    ret

global ldg_cpu_core_id_get
ldg_cpu_core_id_get:
    rdtscp
    mov eax, ecx
    ret

global ldg_cpu_relax
ldg_cpu_relax:
    pause
    ret

%define TOPOLOGY_LVL_MAX 8
%define TOPOLOGY_QWORD_CUNT 19
%define TOPOLOGY_LVL_KIND 0
%define TOPOLOGY_LVL_SHIFT 4
%define TOPOLOGY_LVL_LOGICAL_CUNT 8
%define TOPOLOGY_LVL_CUNT 128
%define TOPOLOGY_APIC_ID 132
%define TOPOLOGY_SMT_ID 136
%define TOPOLOGY_CORE_ID 140
%define TOPOLOGY_PACKAGE_ID 144
%define TOPOLOGY_LEAF 148
%define TOPOLOGY_KIND_SMT 1
%define TOPOLOGY_KIND_CORE 2
%define TOPOLOGY_LEAF_V1 0xB
%define TOPOLOGY_LEAF_V2 0x1F
%define FREQ_TSC_HZ 0
%define FREQ_BASE_MHZ 8
%define FREQ_MAX_MHZ 12
%define FREQ_BUS_MHZ 16
%define FREQ_CRYSTAL_HZ 20
%define FREQ_TSC_NUMERATOR 24
%define FREQ_TSC_DENOMINATOR 28
%define FREQ_LEAF_TSC 0x15
%define FREQ_LEAF_BASE 0x16

global ldg_cpuid_topology_get
ldg_cpuid_topology_get:
    test rcx, rcx
    jz .topology_null_err
    push rbx
    push r12
    push r13
    push r14
    mov r12, rcx
    xor eax, eax
    mov ecx, TOPOLOGY_QWORD_CUNT
.topology_zero:
    mov [r12 + rcx * 8 - 8], rax
    dec ecx
    jnz .topology_zero
    cpuid
    mov r13d, eax
    mov r14d, TOPOLOGY_LEAF_V2
    cmp r13d, TOPOLOGY_LEAF_V2
    jb .topology_v1
    mov eax, TOPOLOGY_LEAF_V2
    xor ecx, ecx
    cpuid
    test ebx, ebx
    jnz .topology_leaf_set
.topology_v1:
    mov r14d, TOPOLOGY_LEAF_V1
    cmp r13d, TOPOLOGY_LEAF_V1
    jb .topology_unsupported
    mov eax, TOPOLOGY_LEAF_V1
    xor ecx, ecx
    cpuid
    test ebx, ebx
    jz .topology_unsupported
.topology_leaf_set:
    mov [r12 + TOPOLOGY_LEAF], r14d
    xor r13d, r13d
.topology_lvl:
    cmp r13d, TOPOLOGY_LVL_MAX
    jae .topology_lvl_done
    mov eax, r14d
    mov ecx, r13d
    cpuid
    mov r8d, ecx
    shr r8d, 8
    and r8d, 0xFF
    jz .topology_lvl_done
    lea r9, [r13 + r13]
    lea r9, [r12 + r9 * 8]
    mov [r9 + TOPOLOGY_LVL_KIND], r8d
    and eax, 0x1F
    mov [r9 + TOPOLOGY_LVL_SHIFT], eax
    and ebx, 0xFFFF
    mov [r9 + TOPOLOGY_LVL_LOGICAL_CUNT], ebx
    mov [r12 + TOPOLOGY_APIC_ID], edx
    inc r13d
    jmp .topology_lvl
.topology_lvl_done:
    mov [r12 + TOPOLOGY_LVL_CUNT], r13d
    test r13d, r13d
    jz .topology_unsupported
    mov edx, [r12 + TOPOLOGY_APIC_ID]
    xor r8d, r8d
    xor r9d, r9d
    xor r10d, r10d
    xor r11d, r11d
.topology_derive:
    cmp r11d, r13d
    jae .topology_derived
    lea rax, [r11 + r11]
    lea rax, [r12 + rax * 8]
    mov ebx, [rax + TOPOLOGY_LVL_SHIFT]
    mov r10d, ebx
    cmp dword [rax + TOPOLOGY_LVL_KIND], TOPOLOGY_KIND_SMT
    jne .topology_not_smt
    mov r8d, ebx
.topology_not_smt:
    cmp dword [rax + TOPOLOGY_LVL_KIND], TOPOLOGY_KIND_CORE
    jne .topology_not_core
    mov r9d, ebx
.topology_not_core:
    inc r11d
    jmp .topology_derive
.topology_derived:
    cmp r9d, r8d
    jae .topology_core_set
    mov r9d, r10d
.topology_core_set:
    mov eax, 1
    mov ecx, r8d
    shl eax, cl
    dec eax
    and eax, edx
    mov [r12 + TOPOLOGY_SMT_ID], eax
    mov eax, edx
    shr eax, cl
    mov ebx, 1
    mov ecx, r9d
    sub ecx, r8d
    shl ebx, cl
    dec ebx
    and eax, ebx
    mov [r12 + TOPOLOGY_CORE_ID], eax
    mov eax, edx
    mov ecx, r10d
    shr eax, cl
    mov [r12 + TOPOLOGY_PACKAGE_ID], eax
    xor eax, eax
.topology_exit:
    pop r14
    pop r13
    pop r12
    pop rbx
    ret
.topology_unsupported:
    mov eax, LDG_ERR_UNSUPPORTED
    jmp .topology_exit
.topology_null_err:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret

global ldg_cpuid_freq_get
ldg_cpuid_freq_get:
    test rcx, rcx
    jz .freq_null_err
    push rbx
    push r12
    push r13
    mov r12, rcx
    xor eax, eax
    mov [r12], rax
    mov [r12 + 8], rax
    mov [r12 + 16], rax
    mov [r12 + 24], rax
    cpuid
    mov r13d, eax
    xor r8d, r8d
    cmp r13d, FREQ_LEAF_TSC
    jb .freq_done
    mov eax, FREQ_LEAF_TSC
    xor ecx, ecx
    cpuid
    mov [r12 + FREQ_TSC_DENOMINATOR], eax
    mov [r12 + FREQ_TSC_NUMERATOR], ebx
    mov [r12 + FREQ_CRYSTAL_HZ], ecx
    test eax, eax
    jz .freq_base
    test ebx, ebx
    jz .freq_base
    mov r8d, 1
    test ecx, ecx
    jz .freq_base
    mov r9d, eax
    mov eax, ecx
    mul rbx
    div r9
    mov [r12 + FREQ_TSC_HZ], rax
.freq_base:
    cmp r13d, FREQ_LEAF_BASE
    jb .freq_done
    mov eax, FREQ_LEAF_BASE
    xor ecx, ecx
    cpuid
    and eax, 0xFFFF
    and ebx, 0xFFFF
    and ecx, 0xFFFF
    mov [r12 + FREQ_BASE_MHZ], eax
    mov [r12 + FREQ_MAX_MHZ], ebx
    mov [r12 + FREQ_BUS_MHZ], ecx
    test eax, eax
    jz .freq_done
    mov r8d, 1
.freq_done:
    xor eax, eax
    test r8d, r8d
    jnz .freq_exit
    mov eax, LDG_ERR_UNSUPPORTED
.freq_exit:
    pop r13
    pop r12
    pop rbx
    ret
.freq_null_err:
    mov eax, LDG_ERR_FUNC_ARG_NULL
    ret
