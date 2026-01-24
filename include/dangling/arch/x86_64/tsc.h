#ifndef LDG_ARCH_X86_64_TSC_H
#define LDG_ARCH_X86_64_TSC_H

#include <stdint.h>
#include <dangling/core/macros.h>

typedef struct ldg_tsc_ctx
{
    uint64_t freq;
    double freq_inv;
    uint64_t base;
    uint32_t core_id;
    uint8_t is_calibrated;
    uint8_t pudding[3];
} LDG_ALIGNED ldg_tsc_ctx_t;

LDG_EXPORT uint64_t ldg_tsc_sample(uint32_t *core_id);
LDG_EXPORT void ldg_tsc_serialize(void);
LDG_EXPORT uint64_t ldg_tsc_serialized_sample(uint32_t *core_id);
LDG_EXPORT uint64_t ldg_tsc_delta(uint64_t start, uint64_t end);

LDG_EXPORT uint32_t ldg_tsc_calibrate(ldg_tsc_ctx_t *ctx);
LDG_EXPORT double ldg_tsc_to_sec(ldg_tsc_ctx_t *ctx, uint64_t cycles);

#endif
