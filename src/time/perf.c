#include <dangling/time/perf.h>
#include <dangling/time/time.h>
#include <dangling/arch/x86_64/tsc.h>
#include <dangling/core/err.h>

#include <string.h>
#include <time.h>

#define LDG_TSC_CALIBRATION_MS 10

int32_t ldg_time_init(ldg_time_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    (void)memset(ctx, 0, sizeof(ldg_time_ctx_t));

    ctx->time = ldg_time_monotonic_get();
    ctx->time_prev = ctx->time;
    ctx->frame_time = 0.0;
    ctx->frame_time_smoothed = 0.016;
    ctx->frame_cunt = 0;
    ctx->is_init = 1;

    return LDG_ERR_AOK;
}

void ldg_time_tick(ldg_time_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return; }

    ctx->time_prev = ctx->time;
    ctx->time = ldg_time_monotonic_get();
    ctx->frame_time = ctx->time - ctx->time_prev;

    ctx->frame_time_smoothed = LDG_TIME_SMOOTH_ALPHA * ctx->frame_time + (1.0 - LDG_TIME_SMOOTH_ALPHA) * ctx->frame_time_smoothed;

    ctx->frame_cunt++;
}

double ldg_time_get(ldg_time_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return 0.0; }

    return ctx->time;
}

double ldg_time_dt_get(ldg_time_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return 0.0; }

    return ctx->frame_time;
}

double ldg_time_dt_smoothed_get(ldg_time_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return 0.0; }

    return ctx->frame_time_smoothed;
}

double ldg_time_fps_get(ldg_time_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return 0.0; }

    if (ctx->frame_time_smoothed <= 0.0) { return 0.0; }

    return 1.0 / ctx->frame_time_smoothed;
}

uint64_t ldg_time_frame_cunt_get(ldg_time_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_init)) { return 0; }

    return ctx->frame_cunt;
}

int32_t ldg_tsc_calibrate(ldg_tsc_ctx_t *ctx)
{
    struct timespec sleep_req = { 0 };
    uint64_t tsc_start = 0;
    uint64_t tsc_end = 0;
    uint32_t core_start = 0;
    uint32_t core_end = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    (void)memset(ctx, 0, sizeof(ldg_tsc_ctx_t));

    sleep_req.tv_sec = 0;
    sleep_req.tv_nsec = LDG_TSC_CALIBRATION_MS * 1000000L;

    tsc_start = ldg_tsc_serialized_sample(&core_start);
    (void)clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_req, NULL);
    tsc_end = ldg_tsc_serialized_sample(&core_end);

    if (core_start != core_end) { return LDG_ERR_TIME_CORE_MIGRATED; }

    ctx->freq = (tsc_end - tsc_start) * (1000 / LDG_TSC_CALIBRATION_MS);
    ctx->freq_inv = 1.0 / (double)ctx->freq;
    ctx->base = tsc_end;
    ctx->core_id = core_end;
    ctx->is_calibrated = 1;

    return LDG_ERR_AOK;
}

double ldg_tsc_to_sec(ldg_tsc_ctx_t *ctx, uint64_t cycles)
{
    if (LDG_UNLIKELY(!ctx || !ctx->is_calibrated)) { return 0.0; }

    return (double)cycles * ctx->freq_inv;
}
