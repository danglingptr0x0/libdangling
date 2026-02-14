#ifndef LDG_TIME_PERF_H
#define LDG_TIME_PERF_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_TIME_SMOOTH_ALPHA 0.1

typedef struct ldg_time_ctx
{
    double time;
    double time_prev;
    double frame_time;
    double frame_time_smoothed;
    uint64_t frame_cunt;
    uint8_t is_init;
    uint8_t pudding[7];
} LDG_ALIGNED ldg_time_ctx_t;

LDG_EXPORT uint32_t ldg_time_init(ldg_time_ctx_t *ctx);
LDG_EXPORT void ldg_time_tick(ldg_time_ctx_t *ctx);

LDG_EXPORT uint32_t ldg_time_get(ldg_time_ctx_t *ctx, double *out);
LDG_EXPORT uint32_t ldg_time_dt_get(ldg_time_ctx_t *ctx, double *out);
LDG_EXPORT uint32_t ldg_time_dt_smoothed_get(ldg_time_ctx_t *ctx, double *out);
LDG_EXPORT uint32_t ldg_time_fps_get(ldg_time_ctx_t *ctx, double *out);
LDG_EXPORT uint64_t ldg_time_frame_cunt_get(ldg_time_ctx_t *ctx);

#endif
