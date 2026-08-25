#include <dangling/math/phys.h>
#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/secure.h>

#include "compute.h"

typedef struct ldg_math_phys_compute_cpu_slot
{
    uint64_t vol_dim[LDG_MATH_PHYS_DIM];
    uint64_t pack_plane;
    uint64_t burst_total;
    uint64_t burst_done;
    double probe_pos[LDG_MATH_PHYS_EM_PROBE_CAP * LDG_MATH_PHYS_DIM];
    uint32_t vol_stride;
    uint32_t slice_z;
    uint32_t probe_cunt;
    uint8_t armed;
    uint8_t pack_armed;
    uint8_t in_flight;
    uint8_t pudding[6];
} ldg_math_phys_compute_cpu_slot_t;

typedef struct ldg_math_phys_compute_cpu
{
    void *dev;
    ldg_math_phys_compute_cpu_slot_t *slot;
} ldg_math_phys_compute_cpu_t;

uint32_t ldg_math_phys_compute_cpu_set(ldg_math_phys_ctx_t *ctx, void *compute)
{
    ldg_math_phys_compute_cpu_t *bridge = 0x0;
    void *blk = 0x0;
    uint32_t err = 0;
    uint32_t rollback_err = 0;
    if (LDG_UNLIKELY(!ctx || !compute)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_alloc(sizeof(ldg_math_phys_compute_cpu_t), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bridge = (ldg_math_phys_compute_cpu_t *)blk;
    err = ldg_mem_secure_zero(bridge, sizeof(ldg_math_phys_compute_cpu_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        rollback_err = ldg_mem_dealloc(blk);
        return (rollback_err != LDG_ERR_AOK) ? rollback_err : err;
    }

    err = ldg_mem_alloc((uint64_t)ctx->conf.region_cap * sizeof(ldg_math_phys_compute_cpu_slot_t), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        rollback_err = ldg_mem_dealloc(bridge);
        return (rollback_err != LDG_ERR_AOK) ? rollback_err : err;
    }

    bridge->slot = (ldg_math_phys_compute_cpu_slot_t *)blk;
    err = ldg_mem_secure_zero(bridge->slot, (uint64_t)ctx->conf.region_cap * sizeof(ldg_math_phys_compute_cpu_slot_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        rollback_err = ldg_mem_dealloc(bridge->slot);
        if (rollback_err == LDG_ERR_AOK) { rollback_err = ldg_mem_dealloc(bridge); }

        return (rollback_err != LDG_ERR_AOK) ? rollback_err : err;
    }

    bridge->dev = 0x0;
    ctx->compute = bridge;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_cpu_teardown(ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_compute_cpu_t *bridge = 0x0;
    uint32_t ret = LDG_ERR_AOK;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->compute == 0x0) { return LDG_ERR_AOK; }

    bridge = (ldg_math_phys_compute_cpu_t *)ctx->compute;
    if (bridge->slot != 0x0)
    {
        err = ldg_mem_dealloc(bridge->slot);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    err = ldg_mem_dealloc(bridge);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    ctx->compute = 0x0;
    return ret;
}

uint32_t ldg_math_phys_compute_cpu_region_arm(ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    ldg_math_phys_compute_cpu_t *bridge = 0x0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ldg_math_phys_compute_region_kind_get(ctx, region) == UINT32_MAX)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    bridge = (ldg_math_phys_compute_cpu_t *)ctx->compute;
    if (LDG_UNLIKELY(bridge->slot[region].armed)) { return LDG_ERR_EXISTS; }

    bridge->slot[region].armed = 1;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_cpu_burst_advance(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt, double *out_tap)
{
    ldg_math_phys_compute_cpu_t *bridge = 0x0;
    double dt = 0.0;
    double sample = 0.0;
    uint64_t tick = 0;
    uint64_t probe_idx = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(tick_cunt == 0 || probe_cunt > LDG_MATH_PHYS_EM_PROBE_CAP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(probe_cunt > 0 && probe_pos == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    bridge = (ldg_math_phys_compute_cpu_t *)ctx->compute;
    if (LDG_UNLIKELY(!bridge->slot[region].armed)) { return LDG_ERR_NOT_INIT; }

    err = ldg_math_phys_dt_tick_get(ctx, &dt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (tick = 0; tick < tick_cunt; tick++)
    {
        err = ldg_math_phys_region_tick_advance(ctx, region, dt, 0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (ctx->region[region].desc.field_kind == LDG_MATH_PHYS_FIELD_KIND_EM)
        {
            err = ldg_math_phys_em_port_capture(ctx, region);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
        }

        for (probe_idx = 0; probe_idx < probe_cunt; probe_idx++)
        {
            err = ldg_math_phys_region_probe_get(ctx, region, &probe_pos[probe_idx * LDG_MATH_PHYS_DIM], &sample);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            if (out_tap != 0x0) { out_tap[probe_idx * tick_cunt + tick] = sample; }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_cpu_burst_begin(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt)
{
    ldg_math_phys_compute_cpu_t *bridge = 0x0;
    uint64_t idx = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(tick_cunt == 0 || probe_cunt > LDG_MATH_PHYS_EM_PROBE_CAP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(probe_cunt > 0 && probe_pos == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    bridge = (ldg_math_phys_compute_cpu_t *)ctx->compute;
    if (LDG_UNLIKELY(!bridge->slot[region].armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(bridge->slot[region].in_flight)) { return LDG_ERR_BUSY; }

    for (idx = 0; idx < (uint64_t)probe_cunt * LDG_MATH_PHYS_DIM; idx++) { bridge->slot[region].probe_pos[idx] = probe_pos[idx]; }

    bridge->slot[region].burst_total = tick_cunt;
    bridge->slot[region].burst_done = 0;
    bridge->slot[region].probe_cunt = probe_cunt;
    bridge->slot[region].in_flight = 1;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_cpu_burst_poll(ldg_math_phys_ctx_t *ctx, uint32_t region, uint8_t *out)
{
    ldg_math_phys_compute_cpu_t *bridge = 0x0;
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    bridge = (ldg_math_phys_compute_cpu_t *)ctx->compute;
    if (LDG_UNLIKELY(!bridge->slot[region].in_flight)) { return LDG_ERR_NOT_INIT; }

    *out = 1;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_cpu_burst_await(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_tap)
{
    ldg_math_phys_compute_cpu_t *bridge = 0x0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    bridge = (ldg_math_phys_compute_cpu_t *)ctx->compute;
    if (LDG_UNLIKELY(!bridge->slot[region].in_flight)) { return LDG_ERR_NOT_INIT; }

    bridge->slot[region].in_flight = 0;
    err = ldg_math_phys_compute_cpu_burst_advance(ctx, region, (uint32_t)bridge->slot[region].burst_total, bridge->slot[region].probe_pos, bridge->slot[region].probe_cunt, out_tap);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bridge->slot[region].burst_done = bridge->slot[region].burst_total;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_cpu_sync(ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    ldg_math_phys_compute_cpu_t *bridge = 0x0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    bridge = (ldg_math_phys_compute_cpu_t *)ctx->compute;
    if (LDG_UNLIKELY(!bridge->slot[region].armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(bridge->slot[region].in_flight)) { return LDG_ERR_BUSY; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_cpu_pack_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t vol_stride, uint32_t slice_z)
{
    ldg_math_phys_compute_cpu_t *bridge = 0x0;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint32_t err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    bridge = (ldg_math_phys_compute_cpu_t *)ctx->compute;
    if (LDG_UNLIKELY(!bridge->slot[region].armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(bridge->slot[region].pack_armed)) { return LDG_ERR_EXISTS; }

    err = ldg_math_phys_region_grid_get(ctx, region, dim);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(slice_z >= dim[2])) { return LDG_ERR_FUNC_ARG_INVALID; }

    bridge->slot[region].vol_stride = vol_stride;
    bridge->slot[region].slice_z = slice_z;
    bridge->slot[region].pack_plane = dim[0] * dim[1];
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { bridge->slot[region].vol_dim[axis] = (vol_stride > 0) ? ((dim[axis] + vol_stride - 1) / vol_stride) : 0; }

    bridge->slot[region].pack_armed = 1;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_cpu_pack_get(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_vol, double *out_slice)
{
    ldg_math_phys_compute_cpu_t *bridge = 0x0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_vol && !out_slice)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    bridge = (ldg_math_phys_compute_cpu_t *)ctx->compute;
    if (LDG_UNLIKELY(!bridge->slot[region].pack_armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(bridge->slot[region].in_flight)) { return LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(ldg_math_phys_compute_region_kind_get(ctx, region) != LDG_MATH_PHYS_FIELD_KIND_EM)) { return LDG_ERR_UNSUPPORTED; }

    if (out_vol != 0x0)
    {
        if (LDG_UNLIKELY(bridge->slot[region].vol_stride == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        err = ldg_math_phys_field_em_vol_get(ctx, region, bridge->slot[region].vol_stride, out_vol);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    if (out_slice != 0x0) { return LDG_ERR_UNSUPPORTED; }

    return LDG_ERR_AOK;
}
