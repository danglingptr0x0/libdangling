#include <string.h>
#include <time.h>

#include <dangling/time/perf.h>
#include <dangling/time/time.h>
#include <dangling/arch/amd64/tsc.h>
#include <dangling/mem/secure.h>
#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_TSC_CALIBRATION_MS 100
#define LDG_TSC_SLEEP_RELATIVE 0
#define LDG_TSC_LIMB_HI_IDX 1
uint32_t ldg_time_init(ldg_time_ctx_t *ctx)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(ctx, sizeof(ldg_time_ctx_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_time_monotonic_get(&ctx->time);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->time_prev = ctx->time;
    ctx->frame_time_smoothed = LDG_TIME_FRAME_TIME_INITIAL;
    ctx->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

void ldg_time_tick(ldg_time_ctx_t *ctx)
{
    uint64_t now = 0;

    if (LDG_UNLIKELY(!ctx || ctx->is_init != LDG_TRUTH_TRUE)) { return; }

    if (LDG_UNLIKELY(ldg_time_monotonic_get(&now) != LDG_ERR_AOK)) { return; }

    ctx->time_prev = ctx->time;
    ctx->time = now;
    ctx->frame_time = ctx->time - ctx->time_prev;

    if (ctx->frame_time >= ctx->frame_time_smoothed) { ctx->frame_time_smoothed += (ctx->frame_time - ctx->frame_time_smoothed) / LDG_TIME_SMOOTH_DIV; }
    else { ctx->frame_time_smoothed -= (ctx->frame_time_smoothed - ctx->frame_time) / LDG_TIME_SMOOTH_DIV; }

    ctx->frame_cunt++;
}

uint32_t ldg_time_get(ldg_time_ctx_t *ctx, uint64_t *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = ctx->time;

    return LDG_ERR_AOK;
}

uint32_t ldg_time_dt_get(ldg_time_ctx_t *ctx, uint64_t *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = ctx->frame_time;

    return LDG_ERR_AOK;
}

uint32_t ldg_time_dt_smoothed_get(ldg_time_ctx_t *ctx, uint64_t *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = ctx->frame_time_smoothed;

    return LDG_ERR_AOK;
}

uint32_t ldg_time_fps_get(ldg_time_ctx_t *ctx, uint64_t *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (ctx->frame_time_smoothed == 0) { return LDG_ERR_AOK; }

    *out = LDG_MS_PER_SEC / ctx->frame_time_smoothed;

    return LDG_ERR_AOK;
}

uint64_t ldg_time_frame_cunt_get(ldg_time_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || ctx->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    return ctx->frame_cunt;
}

uint32_t ldg_time_tsc_calibrate(ldg_time_tsc_ctx_t *ctx)
{
    struct timespec sleep_req = LDG_STRUCT_ZERO_INIT;
    struct timespec wall_start = LDG_STRUCT_ZERO_INIT;
    struct timespec wall_end = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t lhs = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t rhs = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t product = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t quot = LDG_STRUCT_ZERO_INIT;
    uint64_t tsc_start = 0;
    uint64_t tsc_end = 0;
    uint64_t wall_start_ns = 0;
    uint64_t wall_end_ns = 0;
    uint32_t core_start = 0;
    uint32_t core_end = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(ctx, sizeof(ldg_time_tsc_ctx_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sleep_req.tv_sec = 0;
    sleep_req.tv_nsec = LDG_TSC_CALIBRATION_MS * LDG_NS_PER_MS;

    if (LDG_UNLIKELY(clock_gettime(CLOCK_MONOTONIC, &wall_start) != 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    tsc_start = ldg_time_tsc_serialized_sample_get(&core_start);

    if (clock_nanosleep(CLOCK_MONOTONIC, LDG_TSC_SLEEP_RELATIVE, &sleep_req, 0x0) != 0) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    tsc_end = ldg_time_tsc_serialized_sample_get(&core_end);

    if (LDG_UNLIKELY(clock_gettime(CLOCK_MONOTONIC, &wall_end) != 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    if (core_start != core_end) { return LDG_ERR_TIME_CORE_MIGRATED; }

    if (LDG_UNLIKELY(tsc_end <= tsc_start)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    wall_start_ns = ((uint64_t)wall_start.tv_sec * LDG_NS_PER_SEC) + (uint64_t)wall_start.tv_nsec;
    wall_end_ns = ((uint64_t)wall_end.tv_sec * LDG_NS_PER_SEC) + (uint64_t)wall_end.tv_nsec;

    if (LDG_UNLIKELY(wall_end_ns <= wall_start_ns)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    lhs.limb[0] = tsc_end - tsc_start;
    rhs.limb[0] = LDG_NS_PER_SEC;
    ret = ldg_arith_uint128_mul(&lhs, &rhs, &product);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    rhs.limb[0] = wall_end_ns - wall_start_ns;
    ret = ldg_arith_uint128_div(&product, &rhs, &quot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(quot.limb[LDG_TSC_LIMB_HI_IDX] != 0 || quot.limb[0] == 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    ctx->freq = quot.limb[0];
    ctx->base = tsc_end;
    ctx->core_id = core_end;
    ctx->is_calibrated = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_time_tsc_to_ns_convert(ldg_time_tsc_ctx_t *ctx, uint64_t cycles, uint64_t *out)
{
    ldg_uint128_t lhs = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t rhs = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t product = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t quot = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(ctx->is_calibrated != LDG_TRUTH_TRUE || ctx->freq == 0)) { return LDG_ERR_TIME_NOT_CALIBRATED; }

    lhs.limb[0] = cycles;
    rhs.limb[0] = LDG_NS_PER_SEC;
    ret = ldg_arith_uint128_mul(&lhs, &rhs, &product);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    rhs.limb[0] = ctx->freq;
    ret = ldg_arith_uint128_div(&product, &rhs, &quot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(quot.limb[LDG_TSC_LIMB_HI_IDX] != 0)) { return LDG_ERR_OVERFLOW; }

    *out = quot.limb[0];

    return LDG_ERR_AOK;
}
