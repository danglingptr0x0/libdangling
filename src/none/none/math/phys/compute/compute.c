#include <dangling/math/phys.h>
#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/compute/compute.h>

#include "compute.h"

void* ldg_math_phys_compute_dev_get(const ldg_math_phys_ctx_t *ctx)
{
    if (ctx == 0x0 || ctx->compute == 0x0) { return 0x0; }

    return ((ldg_math_phys_compute_hdr_t *)ctx->compute)->dev;
}

uint32_t ldg_math_phys_compute_teardown(ldg_math_phys_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->compute == 0x0) { return LDG_ERR_AOK; }

#ifdef LDG_GPU_VULKAN
    if (ldg_math_phys_compute_dev_get(ctx) != 0x0) { return ldg_math_phys_compute_vk_teardown(ctx); }

#endif
    return ldg_math_phys_compute_cpu_teardown(ctx);
}

uint32_t ldg_math_phys_atmosphere_compute_solve(ldg_math_phys_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

#ifdef LDG_GPU_VULKAN
    if (ldg_math_phys_compute_dev_get(ctx) != 0x0)
    {
        uint32_t err = 0;
        err = ldg_math_phys_atmosphere_vk_solve(ctx);
        if (err != LDG_ERR_UNSUPPORTED) { return err; }
    }

#endif
    return ldg_math_phys_atmosphere_cpu_solve(ctx);
}

uint32_t ldg_math_phys_atmosphere_vol_compute_get(const ldg_math_phys_ctx_t *ctx, const ldg_math_phys_atmosphere_vol_desc_t *desc, double *out_transmittance, double *out_inscatter)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_transmittance)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_inscatter)) { return LDG_ERR_FUNC_ARG_NULL; }

#ifdef LDG_GPU_VULKAN
    if (ldg_math_phys_compute_dev_get(ctx) != 0x0)
    {
        uint32_t err = 0;
        err = ldg_math_phys_atmosphere_vol_vk_get(ctx, desc, out_transmittance, out_inscatter);
        if (err != LDG_ERR_UNSUPPORTED) { return err; }
    }

#endif
    return ldg_math_phys_atmosphere_vol_cpu_get(ctx, desc, out_transmittance, out_inscatter);
}

LDG_EXPORT uint32_t ldg_math_phys_compute_set(ldg_math_phys_ctx_t *ctx, void *compute)
{
    void *dev = 0x0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!compute)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->compute != 0x0)) { return LDG_ERR_EXISTS; }

    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_DENIED; }

    err = ldg_compute_dev_get(compute, &dev);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (dev != 0x0)
    {
#ifdef LDG_GPU_VULKAN
        return ldg_math_phys_compute_vk_set(ctx, compute);
#else
        return LDG_ERR_UNSUPPORTED;
#endif
    }

    return ldg_math_phys_compute_cpu_set(ctx, compute);
}

LDG_EXPORT uint32_t ldg_math_phys_compute_clear(ldg_math_phys_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    return ldg_math_phys_compute_teardown(ctx);
}

LDG_EXPORT uint32_t ldg_math_phys_compute_region_arm(ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_math_phys_region_step_skip_is(ctx, region) == LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

#ifdef LDG_GPU_VULKAN
    if (ldg_math_phys_compute_dev_get(ctx) != 0x0) { return ldg_math_phys_compute_vk_region_arm(ctx, region); }

#endif
    return ldg_math_phys_compute_cpu_region_arm(ctx, region);
}

LDG_EXPORT uint32_t ldg_math_phys_compute_burst_advance(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt, double *out_tap)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

#ifdef LDG_GPU_VULKAN
    if (ldg_math_phys_compute_dev_get(ctx) != 0x0) { return ldg_math_phys_compute_vk_burst_advance(ctx, region, tick_cunt, probe_pos, probe_cunt, out_tap); }

#endif
    return ldg_math_phys_compute_cpu_burst_advance(ctx, region, tick_cunt, probe_pos, probe_cunt, out_tap);
}

LDG_EXPORT uint32_t ldg_math_phys_compute_burst_begin(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

#ifdef LDG_GPU_VULKAN
    if (ldg_math_phys_compute_dev_get(ctx) != 0x0) { return ldg_math_phys_compute_vk_burst_begin(ctx, region, tick_cunt, probe_pos, probe_cunt); }

#endif
    return ldg_math_phys_compute_cpu_burst_begin(ctx, region, tick_cunt, probe_pos, probe_cunt);
}

LDG_EXPORT uint32_t ldg_math_phys_compute_burst_poll(ldg_math_phys_ctx_t *ctx, uint32_t region, uint8_t *out)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

#ifdef LDG_GPU_VULKAN
    if (ldg_math_phys_compute_dev_get(ctx) != 0x0) { return ldg_math_phys_compute_vk_burst_poll(ctx, region, out); }

#endif
    return ldg_math_phys_compute_cpu_burst_poll(ctx, region, out);
}

LDG_EXPORT uint32_t ldg_math_phys_compute_burst_await(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_tap)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

#ifdef LDG_GPU_VULKAN
    if (ldg_math_phys_compute_dev_get(ctx) != 0x0) { return ldg_math_phys_compute_vk_burst_await(ctx, region, out_tap); }

#endif
    return ldg_math_phys_compute_cpu_burst_await(ctx, region, out_tap);
}

LDG_EXPORT uint32_t ldg_math_phys_compute_sync(ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

#ifdef LDG_GPU_VULKAN
    if (ldg_math_phys_compute_dev_get(ctx) != 0x0) { return ldg_math_phys_compute_vk_sync(ctx, region); }

#endif
    return ldg_math_phys_compute_cpu_sync(ctx, region);
}

LDG_EXPORT uint32_t ldg_math_phys_compute_pack_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t vol_stride, uint32_t slice_z)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

#ifdef LDG_GPU_VULKAN
    if (ldg_math_phys_compute_dev_get(ctx) != 0x0) { return ldg_math_phys_compute_vk_pack_set(ctx, region, vol_stride, slice_z); }

#endif
    return ldg_math_phys_compute_cpu_pack_set(ctx, region, vol_stride, slice_z);
}

LDG_EXPORT uint32_t ldg_math_phys_compute_pack_get(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_vol, double *out_slice)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

#ifdef LDG_GPU_VULKAN
    if (ldg_math_phys_compute_dev_get(ctx) != 0x0) { return ldg_math_phys_compute_vk_pack_get(ctx, region, out_vol, out_slice); }

#endif
    return ldg_math_phys_compute_cpu_pack_get(ctx, region, out_vol, out_slice);
}
