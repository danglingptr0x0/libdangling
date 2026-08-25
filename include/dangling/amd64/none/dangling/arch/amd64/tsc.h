#ifndef LDG_ARCH_AMD64_TSC_H
#define LDG_ARCH_AMD64_TSC_H

#include <stdint.h>
#include <dangling/core/macros.h>

typedef struct ldg_time_tsc_ctx
{
    uint64_t freq;
    double freq_inv;
    uint64_t base;
    uint32_t core_id;
    uint8_t is_calibrated;
    uint8_t pudding[3];
} LDG_ALIGNED ldg_time_tsc_ctx_t;

LDG_EXPORT uint64_t ldg_time_tsc_sample_get(uint32_t *core_id);
LDG_EXPORT void ldg_time_tsc_serialize(void);
LDG_EXPORT uint64_t ldg_time_tsc_serialized_sample_get(uint32_t *core_id);
LDG_EXPORT uint64_t ldg_time_tsc_delta_get(uint64_t start, uint64_t end);

LDG_EXPORT uint32_t ldg_time_tsc_calibrate(ldg_time_tsc_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_time_tsc_to_sec_convert(ldg_time_tsc_ctx_t *ctx, uint64_t cycles, double *out);

#endif
