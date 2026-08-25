#include <string.h>
#include <windows.h>

#include <dangling/time/perf.h>
#include <dangling/time/time.h>
#include <dangling/arch/amd64/tsc.h>
#include <dangling/mem/secure.h>
#include <dangling/core/err.h>

#define LDG_TSC_CALIBRATION_MS 100
#define LDG_TSC_CORE_MASK_BIT ((DWORD_PTR)1)

uint32_t ldg_time_init(ldg_time_ctx_t *ctx)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(ctx, 0, sizeof(ldg_time_ctx_t)) != ctx)) { return LDG_ERR_MEM_BAD; }

    err = ldg_time_monotonic_get(&ctx->time);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ctx->time_prev = ctx->time;
    ctx->frame_time = 0.0;
    ctx->frame_time_smoothed = 0.016;
    ctx->frame_cunt = 0;
    ctx->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

void ldg_time_tick(ldg_time_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || ctx->is_init != LDG_TRUTH_TRUE)) { return; }

    ctx->time_prev = ctx->time;
    ldg_time_monotonic_get(&ctx->time);
    ctx->frame_time = ctx->time - ctx->time_prev;

    ctx->frame_time_smoothed = LDG_TIME_SMOOTH_ALPHA * ctx->frame_time + (1.0 - LDG_TIME_SMOOTH_ALPHA) * ctx->frame_time_smoothed;

    ctx->frame_cunt++;
}

uint32_t ldg_time_get(ldg_time_ctx_t *ctx, double *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = ctx->time;

    return LDG_ERR_AOK;
}

uint32_t ldg_time_dt_get(ldg_time_ctx_t *ctx, double *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = ctx->frame_time;

    return LDG_ERR_AOK;
}

uint32_t ldg_time_dt_smoothed_get(ldg_time_ctx_t *ctx, double *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = ctx->frame_time_smoothed;

    return LDG_ERR_AOK;
}

uint32_t ldg_time_fps_get(ldg_time_ctx_t *ctx, double *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = (ctx->frame_time_smoothed > 0.0) ? (1.0 / ctx->frame_time_smoothed) : 0.0;

    return LDG_ERR_AOK;
}

uint64_t ldg_time_frame_cunt_get(ldg_time_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || ctx->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    return ctx->frame_cunt;
}

uint32_t ldg_time_tsc_calibrate(ldg_time_tsc_ctx_t *ctx)
{
    LARGE_INTEGER qpc_freq = LDG_STRUCT_ZERO_INIT;
    LARGE_INTEGER qpc_start = LDG_STRUCT_ZERO_INIT;
    LARGE_INTEGER qpc_end = LDG_STRUCT_ZERO_INIT;
    DWORD_PTR affinity_prev = 0;
    DWORD_PTR affinity_rst = 0;
    uint64_t tsc_start = 0;
    uint64_t tsc_end = 0;
    uint32_t core_start = 0;
    uint32_t core_end = 0;
    double elapsed = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(ctx, sizeof(ldg_time_tsc_ctx_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!QueryPerformanceFrequency(&qpc_freq))) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    if (LDG_UNLIKELY(qpc_freq.QuadPart <= 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    affinity_prev = SetThreadAffinityMask(GetCurrentThread(), LDG_TSC_CORE_MASK_BIT << GetCurrentProcessorNumber());
    if (LDG_UNLIKELY(affinity_prev == 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    tsc_start = ldg_time_tsc_serialized_sample_get(&core_start);
    if (LDG_UNLIKELY(!QueryPerformanceCounter(&qpc_start)))
    {
        affinity_rst = SetThreadAffinityMask(GetCurrentThread(), affinity_prev);
        if (LDG_UNLIKELY(affinity_rst == 0)) { LDG_ERRLOG_WARN("time: thread affinity not restored"); }

        return LDG_ERR_TIME_NOT_CALIBRATED;
    }

    Sleep(LDG_TSC_CALIBRATION_MS);

    tsc_end = ldg_time_tsc_serialized_sample_get(&core_end);
    if (LDG_UNLIKELY(!QueryPerformanceCounter(&qpc_end)))
    {
        affinity_rst = SetThreadAffinityMask(GetCurrentThread(), affinity_prev);
        if (LDG_UNLIKELY(affinity_rst == 0)) { LDG_ERRLOG_WARN("time: thread affinity not restored"); }

        return LDG_ERR_TIME_NOT_CALIBRATED;
    }

    affinity_rst = SetThreadAffinityMask(GetCurrentThread(), affinity_prev);
    if (LDG_UNLIKELY(affinity_rst == 0)) { LDG_ERRLOG_WARN("time: thread affinity not restored"); }

    if (LDG_UNLIKELY(core_start != core_end)) { return LDG_ERR_TIME_CORE_MIGRATED; }

    if (LDG_UNLIKELY(qpc_end.QuadPart <= qpc_start.QuadPart)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    if (LDG_UNLIKELY(tsc_end <= tsc_start)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    elapsed = (double)(qpc_end.QuadPart - qpc_start.QuadPart) / (double)qpc_freq.QuadPart;

    ctx->freq = (uint64_t)((double)(tsc_end - tsc_start) / elapsed);
    ctx->freq_inv = 1.0 / (double)ctx->freq;
    ctx->base = tsc_end;
    ctx->core_id = core_end;
    ctx->is_calibrated = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_time_tsc_to_sec_convert(ldg_time_tsc_ctx_t *ctx, uint64_t cycles, double *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_calibrated != LDG_TRUTH_TRUE)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    *out = (double)cycles * ctx->freq_inv;

    return LDG_ERR_AOK;
}
