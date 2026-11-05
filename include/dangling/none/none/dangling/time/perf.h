#ifndef LDG_TIME_PERF_H
#define LDG_TIME_PERF_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_TIME_SMOOTH_DIV 10
#define LDG_TIME_FRAME_TIME_INITIAL 16

typedef struct ldg_time_ctx
{
    uint64_t time;
    uint64_t time_prev;
    uint64_t frame_time;
    uint64_t frame_time_smoothed;
    uint64_t frame_cunt;
    uint8_t is_init;
    uint8_t pudding[23];
} LDG_ALIGNED ldg_time_ctx_t;

LDG_EXPORT uint32_t ldg_time_init(ldg_time_ctx_t *ctx);
LDG_EXPORT void ldg_time_tick(ldg_time_ctx_t *ctx);

LDG_EXPORT uint32_t ldg_time_get(ldg_time_ctx_t *ctx, uint64_t *out);
LDG_EXPORT uint32_t ldg_time_dt_get(ldg_time_ctx_t *ctx, uint64_t *out);
LDG_EXPORT uint32_t ldg_time_dt_smoothed_get(ldg_time_ctx_t *ctx, uint64_t *out);
LDG_EXPORT uint32_t ldg_time_fps_get(ldg_time_ctx_t *ctx, uint64_t *out);
LDG_EXPORT uint64_t ldg_time_frame_cunt_get(ldg_time_ctx_t *ctx);

#endif
