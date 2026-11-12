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

typedef struct ldg_cpuid_feat
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
} ldg_cpuid_feat_t;

#define LDG_CPUID_TOPOLOGY_LVL_MAX 8

typedef enum ldg_cpuid_topology_kind
{
    LDG_CPUID_TOPOLOGY_KIND_NONE = 0,
    LDG_CPUID_TOPOLOGY_KIND_SMT,
    LDG_CPUID_TOPOLOGY_KIND_CORE,
    LDG_CPUID_TOPOLOGY_KIND_MODULE,
    LDG_CPUID_TOPOLOGY_KIND_TILE,
    LDG_CPUID_TOPOLOGY_KIND_DIE,
    LDG_CPUID_TOPOLOGY_KIND_DIE_GROUP,
    LDG_CPUID_TOPOLOGY_KIND_CUNT
} ldg_cpuid_topology_kind_t;

typedef struct ldg_cpuid_topology_lvl
{
    uint32_t kind;
    uint32_t shift;
    uint32_t logical_cunt;
    uint8_t pudding[4];
} ldg_cpuid_topology_lvl_t;

typedef struct ldg_cpuid_topology
{
    ldg_cpuid_topology_lvl_t lvl[LDG_CPUID_TOPOLOGY_LVL_MAX];
    uint32_t lvl_cunt;
    uint32_t apic_id;
    uint32_t smt_id;
    uint32_t core_id;
    uint32_t package_id;
    uint32_t leaf;
} ldg_cpuid_topology_t;

typedef struct ldg_cpuid_freq
{
    uint64_t tsc_hz;
    uint32_t base_mhz;
    uint32_t max_mhz;
    uint32_t bus_mhz;
    uint32_t crystal_hz;
    uint32_t tsc_numerator;
    uint32_t tsc_denominator;
} ldg_cpuid_freq_t;

LDG_EXPORT uint32_t ldg_cpuid_topology_get(ldg_cpuid_topology_t *out);
LDG_EXPORT uint32_t ldg_cpuid_freq_get(ldg_cpuid_freq_t *out);
LDG_EXPORT uint32_t ldg_cpuid(uint32_t leaf, uint32_t subleaf, ldg_cpuid_regs_t *regs);
LDG_EXPORT uint32_t ldg_cpuid_vendor_get(char out[LDG_CPUID_VENDOR_LEN + 1]);
LDG_EXPORT uint32_t ldg_cpuid_brand_get(char out[LDG_CPUID_BRAND_LEN + 1]);
LDG_EXPORT uint32_t ldg_cpuid_feat_get(ldg_cpuid_feat_t *feat);
LDG_EXPORT uint32_t ldg_cpuid_max_leaf_get(void);
LDG_EXPORT uint32_t ldg_cpuid_max_ext_leaf_get(void);

LDG_EXPORT uint32_t ldg_cpu_core_id_get(void);
LDG_EXPORT void ldg_cpu_relax(void);

#endif
