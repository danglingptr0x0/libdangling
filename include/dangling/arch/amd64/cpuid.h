#ifndef LDG_ARCH_AMD64_CPUID_H
#define LDG_ARCH_AMD64_CPUID_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_CPUID_VENDOR_LEN 12
#define LDG_CPUID_BRAND_LEN 48

typedef struct ldg_cpuid_regs
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
} ldg_cpuid_regs_t;

typedef struct ldg_cpuid_features
{
    uint32_t sse : 1;
    uint32_t sse2 : 1;
    uint32_t sse3 : 1;
    uint32_t ssse3 : 1;
    uint32_t sse41 : 1;
    uint32_t sse42 : 1;
    uint32_t avx : 1;
    uint32_t avx2 : 1;
    uint32_t avx512f : 1;
    uint32_t aes : 1;
    uint32_t pclmul : 1;
    uint32_t rdrand : 1;
    uint32_t rdseed : 1;
    uint32_t bmi1 : 1;
    uint32_t bmi2 : 1;
    uint32_t popcnt : 1;
    uint32_t fma : 1;
    uint32_t f16c : 1;
    uint32_t cx8 : 1;
    uint32_t cx16 : 1;
    uint32_t htt : 1;
    uint32_t invariant_tsc : 1;
    uint32_t pudding : 10;
} ldg_cpuid_features_t;

LDG_EXPORT void ldg_cpuid(uint32_t leaf, uint32_t subleaf, ldg_cpuid_regs_t *regs);
LDG_EXPORT void ldg_cpuid_vendor_get(char out[LDG_CPUID_VENDOR_LEN + 1]);
LDG_EXPORT void ldg_cpuid_brand_get(char out[LDG_CPUID_BRAND_LEN + 1]);
LDG_EXPORT void ldg_cpuid_features_get(ldg_cpuid_features_t *features);
LDG_EXPORT uint32_t ldg_cpuid_max_leaf_get(void);
LDG_EXPORT uint32_t ldg_cpuid_max_ext_leaf_get(void);

LDG_EXPORT uint32_t ldg_cpu_core_id_get(void);
LDG_EXPORT void ldg_cpu_relax(void);

#endif
