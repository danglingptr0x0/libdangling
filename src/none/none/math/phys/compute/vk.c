
#include <dangling/math/phys.h>
#include <dangling/math/math.h>
#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/secure.h>
#include <dangling/gpu/gpu.h>
#include <dangling/compute/compute.h>

#include "compute.h"
#include "field.h"
#include "atmosphere.h"

extern const uint32_t ldg_math_phys_spirv_em_electric_rigid[];
extern const uint64_t ldg_math_phys_spirv_em_electric_rigid_size;
extern const uint32_t ldg_math_phys_spirv_em_magnetic_rigid[];
extern const uint64_t ldg_math_phys_spirv_em_magnetic_rigid_size;
extern const uint32_t ldg_math_phys_spirv_em_electric_cpml[];
extern const uint64_t ldg_math_phys_spirv_em_electric_cpml_size;
extern const uint32_t ldg_math_phys_spirv_em_magnetic_cpml[];
extern const uint64_t ldg_math_phys_spirv_em_magnetic_cpml_size;
extern const uint32_t ldg_math_phys_spirv_acoustic_prime[];
extern const uint64_t ldg_math_phys_spirv_acoustic_prime_size;
extern const uint32_t ldg_math_phys_spirv_vol_pack[];
extern const uint64_t ldg_math_phys_spirv_vol_pack_size;
extern const uint32_t ldg_math_phys_spirv_slice_pack[];
extern const uint64_t ldg_math_phys_spirv_slice_pack_size;
extern const uint32_t ldg_math_phys_spirv_acoustic_velocity[];
extern const uint64_t ldg_math_phys_spirv_acoustic_velocity_size;
extern const uint32_t ldg_math_phys_spirv_acoustic_pressure[];
extern const uint64_t ldg_math_phys_spirv_acoustic_pressure_size;
extern const uint32_t ldg_math_phys_spirv_acoustic_drive_clear[];
extern const uint64_t ldg_math_phys_spirv_acoustic_drive_clear_size;
extern const uint32_t ldg_math_phys_spirv_acoustic_src_deposit[];
extern const uint64_t ldg_math_phys_spirv_acoustic_src_deposit_size;
extern const uint32_t ldg_math_phys_spirv_acoustic_tap[];
extern const uint64_t ldg_math_phys_spirv_acoustic_tap_size;
extern const uint32_t ldg_math_phys_spirv_acoustic_vol_pack[];
extern const uint64_t ldg_math_phys_spirv_acoustic_vol_pack_size;
extern const uint32_t ldg_math_phys_spirv_acoustic_slice_pack[];
extern const uint64_t ldg_math_phys_spirv_acoustic_slice_pack_size;
extern const uint32_t ldg_math_phys_spirv_acoustic_velocity_rigid[];
extern const uint64_t ldg_math_phys_spirv_acoustic_velocity_rigid_size;
extern const uint32_t ldg_math_phys_spirv_acoustic_pressure_rigid[];
extern const uint64_t ldg_math_phys_spirv_acoustic_pressure_rigid_size;
extern const uint32_t ldg_math_phys_spirv_em_port_tap[];
extern const uint64_t ldg_math_phys_spirv_em_port_tap_size;

#define LDG_MATH_PHYS_COMPUTE_PIPELINE_CUNT 17

typedef struct ldg_math_phys_compute_slot
{
    ldg_gpu_buff_t electric;
    ldg_gpu_buff_t magnetic;
    ldg_gpu_buff_t param;
    ldg_gpu_buff_t cell_coeff;
    ldg_gpu_buff_t batch_blk;
    ldg_gpu_buff_t tap_param;
    ldg_gpu_buff_t mask;
    ldg_gpu_buff_t psi;
    ldg_gpu_buff_t stretch;
    ldg_gpu_buff_t pack;
    ldg_gpu_buff_t pack_param;
    ldg_gpu_buff_t pressure;
    ldg_gpu_buff_t velocity;
    ldg_gpu_buff_t drive;
    ldg_gpu_buff_t footprint_cell;
    ldg_gpu_buff_t footprint_weight;
    ldg_gpu_buff_t footprint_param;
    ldg_gpu_buff_t port_param;
    ldg_gpu_buff_t port_pattern;
    ldg_gpu_fence_t fence;
    double *series_span;
    double *tap_span;
    double *port_span;
    uint32_t *footprint_cell_host;
    double *footprint_weight_host;
    uint64_t pending_tick;
    uint64_t vol_dim[3];
    uint64_t pack_plane;
    uint64_t burst_total;
    uint64_t burst_done;
    uint64_t burst_span;
    uint32_t vol_stride;
    uint32_t slice_z;
    uint32_t group_x;
    uint32_t group_y;
    uint32_t group_z;
    uint32_t cell_group;
    uint32_t tick_desc_cunt;
    uint32_t probe_cunt;
    uint32_t footprint_cunt;
    uint32_t port_pm_cunt;
    uint8_t armed;
    uint8_t open;
    uint8_t pack_armed;
    uint8_t in_flight;
    uint8_t kind;
    uint8_t pudding[3];
} ldg_math_phys_compute_slot_t;

typedef struct ldg_math_phys_compute_bridge
{
    void *dev;
    ldg_gpu_dispatch_desc_t *desc;
    ldg_math_phys_compute_slot_t *slot;
    uint32_t pipeline_electric;
    uint32_t pipeline_magnetic;
    uint32_t pipeline_electric_cpml;
    uint32_t pipeline_magnetic_cpml;
    uint32_t pipeline_prime;
    uint32_t pipeline_vol_pack;
    uint32_t pipeline_slice_pack;
    uint32_t pipeline_acoustic_velocity;
    uint32_t pipeline_acoustic_pressure;
    uint32_t pipeline_acoustic_velocity_rigid;
    uint32_t pipeline_acoustic_pressure_rigid;
    uint32_t pipeline_acoustic_drive_clear;
    uint32_t pipeline_acoustic_deposit;
    uint32_t pipeline_acoustic_tap;
    uint32_t pipeline_acoustic_vol_pack;
    uint32_t pipeline_acoustic_slice_pack;
    uint32_t pipeline_em_port_tap;
    uint32_t pipeline_cunt;
} ldg_math_phys_compute_bridge_t;


static uint32_t compute_slot_buff_release(void *dev, ldg_gpu_buff_t *buff)
{
    uint32_t ret = 0;

    if (buff->id == LDG_GPU_BUFF_ID_NONE || buff->size == 0) { return LDG_ERR_AOK; }

    ret = ldg_gpu_buff_destroy(dev, buff->id);

    *buff = (ldg_gpu_buff_t)LDG_STRUCT_ZERO_INIT;
    buff->id = LDG_GPU_BUFF_ID_NONE;

    return ret;
}

static uint32_t ldg_math_phys_compute_slot_release(ldg_math_phys_compute_bridge_t *bridge, ldg_math_phys_compute_slot_t *slot)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t err = 0;
    if (!slot->armed) { return LDG_ERR_AOK; }

    if (slot->in_flight)
    {
        err = ldg_gpu_fence_wait(bridge->dev, &slot->fence, UINT64_MAX);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

        err = ldg_gpu_fence_destroy(bridge->dev, &slot->fence);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

        slot->in_flight = 0;
    }

    if (slot->kind == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC)
    {
        err = compute_slot_buff_release(bridge->dev, &slot->pressure);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

        err = compute_slot_buff_release(bridge->dev, &slot->velocity);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

        err = compute_slot_buff_release(bridge->dev, &slot->drive);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

        err = compute_slot_buff_release(bridge->dev, &slot->psi);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

        err = compute_slot_buff_release(bridge->dev, &slot->stretch);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

        err = compute_slot_buff_release(bridge->dev, &slot->footprint_cell);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

        err = compute_slot_buff_release(bridge->dev, &slot->footprint_weight);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

        err = compute_slot_buff_release(bridge->dev, &slot->footprint_param);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }
    else
    {
        err = compute_slot_buff_release(bridge->dev, &slot->electric);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

        err = compute_slot_buff_release(bridge->dev, &slot->magnetic);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

        if (slot->open)
        {
            err = compute_slot_buff_release(bridge->dev, &slot->psi);
            if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

            err = compute_slot_buff_release(bridge->dev, &slot->stretch);
            if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
        }

        if (slot->port_pm_cunt > 0)
        {
            err = compute_slot_buff_release(bridge->dev, &slot->port_param);
            if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

            err = compute_slot_buff_release(bridge->dev, &slot->port_pattern);
            if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

            slot->port_pm_cunt = 0;
        }
    }

    err = compute_slot_buff_release(bridge->dev, &slot->param);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    err = compute_slot_buff_release(bridge->dev, &slot->cell_coeff);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    err = compute_slot_buff_release(bridge->dev, &slot->batch_blk);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    err = compute_slot_buff_release(bridge->dev, &slot->tap_param);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    err = compute_slot_buff_release(bridge->dev, &slot->mask);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    if (slot->pack_armed)
    {
        err = compute_slot_buff_release(bridge->dev, &slot->pack);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

        err = compute_slot_buff_release(bridge->dev, &slot->pack_param);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

        slot->pack_armed = 0;
    }

    if (slot->series_span != 0x0)
    {
        err = ldg_mem_dealloc(slot->series_span);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if (slot->tap_span != 0x0)
    {
        err = ldg_mem_dealloc(slot->tap_span);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if (slot->port_span != 0x0)
    {
        err = ldg_mem_dealloc(slot->port_span);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if (slot->footprint_cell_host != 0x0)
    {
        err = ldg_mem_dealloc(slot->footprint_cell_host);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if (slot->footprint_weight_host != 0x0)
    {
        err = ldg_mem_dealloc(slot->footprint_weight_host);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    slot->series_span = 0x0;
    slot->tap_span = 0x0;
    slot->port_span = 0x0;
    slot->footprint_cell_host = 0x0;
    slot->footprint_weight_host = 0x0;
    slot->armed = 0;
    return ret;
}

uint32_t ldg_math_phys_compute_vk_teardown(ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    uint32_t ret = LDG_ERR_AOK;
    uint32_t err = 0;
    uint32_t idx = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->compute == 0x0) { return LDG_ERR_AOK; }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    for (idx = 0; idx < ctx->conf.region_cap; idx++)
    {
        err = ldg_math_phys_compute_slot_release(bridge, &bridge->slot[idx]);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }
    if (bridge->pipeline_cunt >= 1) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_electric); }

    if (bridge->pipeline_cunt >= 2) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_magnetic); }

    if (bridge->pipeline_cunt >= 3) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_electric_cpml); }

    if (bridge->pipeline_cunt >= 4) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_magnetic_cpml); }

    if (bridge->pipeline_cunt >= 5) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_prime); }

    if (bridge->pipeline_cunt >= 6) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_vol_pack); }

    if (bridge->pipeline_cunt >= 7) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_slice_pack); }

    if (bridge->pipeline_cunt >= 8) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_acoustic_velocity); }

    if (bridge->pipeline_cunt >= 9) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_acoustic_pressure); }

    if (bridge->pipeline_cunt >= 10) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_acoustic_drive_clear); }

    if (bridge->pipeline_cunt >= 11) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_acoustic_deposit); }

    if (bridge->pipeline_cunt >= 12) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_acoustic_tap); }

    if (bridge->pipeline_cunt >= 13) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_acoustic_vol_pack); }

    if (bridge->pipeline_cunt >= 14) { err = ldg_gpu_pipeline_destroy(bridge->dev, bridge->pipeline_acoustic_slice_pack); }

    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    err = ldg_mem_dealloc(bridge->desc);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    err = ldg_mem_dealloc(bridge->slot);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    err = ldg_mem_dealloc(bridge);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    ctx->compute = 0x0;
    return ret;
}

uint32_t ldg_math_phys_compute_vk_set(ldg_math_phys_ctx_t *ctx, void *compute)
{
    ldg_gpu_spirv_desc_t spirv_desc = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    void *blk = 0x0;
    void *dev = 0x0;
    uint32_t err = 0;
    uint32_t ret = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!compute)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->compute != 0x0)) { return LDG_ERR_EXISTS; }

    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_DENIED; }

    err = ldg_compute_dev_get(compute, &dev);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_alloc(sizeof(ldg_math_phys_compute_bridge_t), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bridge = (ldg_math_phys_compute_bridge_t *)blk;
    err = ldg_mem_secure_zero(bridge, sizeof(ldg_math_phys_compute_bridge_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_mem_dealloc(blk);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->dev = dev;
    err = ldg_mem_alloc(LDG_GPU_BATCH_MAX * sizeof(ldg_gpu_dispatch_desc_t), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_mem_dealloc(bridge);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->desc = (ldg_gpu_dispatch_desc_t *)blk;
    err = ldg_mem_alloc((uint64_t)ctx->conf.region_cap * sizeof(ldg_math_phys_compute_slot_t), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_mem_dealloc(bridge->desc);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        ret = ldg_mem_dealloc(bridge);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->slot = (ldg_math_phys_compute_slot_t *)blk;
    err = ldg_mem_secure_zero(bridge->slot, (uint64_t)ctx->conf.region_cap * sizeof(ldg_math_phys_compute_slot_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ctx->compute = bridge;
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    ctx->compute = bridge;
    spirv_desc.entry_name = "main";
    spirv_desc.code = ldg_math_phys_spirv_em_electric_rigid;
    spirv_desc.code_size = ldg_math_phys_spirv_em_electric_rigid_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_electric);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 1;
    spirv_desc.code = ldg_math_phys_spirv_em_magnetic_rigid;
    spirv_desc.code_size = ldg_math_phys_spirv_em_magnetic_rigid_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_magnetic);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 2;
    spirv_desc.code = ldg_math_phys_spirv_em_electric_cpml;
    spirv_desc.code_size = ldg_math_phys_spirv_em_electric_cpml_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_electric_cpml);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 3;
    spirv_desc.code = ldg_math_phys_spirv_em_magnetic_cpml;
    spirv_desc.code_size = ldg_math_phys_spirv_em_magnetic_cpml_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_magnetic_cpml);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 4;
    spirv_desc.code = ldg_math_phys_spirv_acoustic_prime;
    spirv_desc.code_size = ldg_math_phys_spirv_acoustic_prime_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_prime);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 5;
    spirv_desc.code = ldg_math_phys_spirv_vol_pack;
    spirv_desc.code_size = ldg_math_phys_spirv_vol_pack_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_vol_pack);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 6;
    spirv_desc.code = ldg_math_phys_spirv_slice_pack;
    spirv_desc.code_size = ldg_math_phys_spirv_slice_pack_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_slice_pack);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 7;
    spirv_desc.code = ldg_math_phys_spirv_acoustic_velocity;
    spirv_desc.code_size = ldg_math_phys_spirv_acoustic_velocity_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_acoustic_velocity);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 8;
    spirv_desc.code = ldg_math_phys_spirv_acoustic_pressure;
    spirv_desc.code_size = ldg_math_phys_spirv_acoustic_pressure_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_acoustic_pressure);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 9;
    spirv_desc.code = ldg_math_phys_spirv_acoustic_drive_clear;
    spirv_desc.code_size = ldg_math_phys_spirv_acoustic_drive_clear_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_acoustic_drive_clear);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 10;
    spirv_desc.code = ldg_math_phys_spirv_acoustic_src_deposit;
    spirv_desc.code_size = ldg_math_phys_spirv_acoustic_src_deposit_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_acoustic_deposit);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 11;
    spirv_desc.code = ldg_math_phys_spirv_acoustic_tap;
    spirv_desc.code_size = ldg_math_phys_spirv_acoustic_tap_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_acoustic_tap);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 12;
    spirv_desc.code = ldg_math_phys_spirv_acoustic_vol_pack;
    spirv_desc.code_size = ldg_math_phys_spirv_acoustic_vol_pack_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_acoustic_vol_pack);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 13;
    spirv_desc.code = ldg_math_phys_spirv_acoustic_slice_pack;
    spirv_desc.code_size = ldg_math_phys_spirv_acoustic_slice_pack_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_acoustic_slice_pack);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 14;
    spirv_desc.code = ldg_math_phys_spirv_acoustic_velocity_rigid;
    spirv_desc.code_size = ldg_math_phys_spirv_acoustic_velocity_rigid_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_acoustic_velocity_rigid);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 15;
    spirv_desc.code = ldg_math_phys_spirv_acoustic_pressure_rigid;
    spirv_desc.code_size = ldg_math_phys_spirv_acoustic_pressure_rigid_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_acoustic_pressure_rigid);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 16;
    spirv_desc.code = ldg_math_phys_spirv_em_port_tap;
    spirv_desc.code_size = ldg_math_phys_spirv_em_port_tap_size;
    err = ldg_gpu_pipeline_create(bridge->dev, &spirv_desc, &bridge->pipeline_em_port_tap);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_vk_teardown(ctx);
        if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

        return err;
    }

    bridge->pipeline_cunt = 17;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_vk_clear(ldg_math_phys_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    return ldg_math_phys_compute_vk_teardown(ctx);
}

static uint32_t ldg_math_phys_compute_selftest(ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    ldg_math_phys_compute_region_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_gpu_dispatch_desc_t impulse_desc = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_compute_bridge_t *bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    const double *row = 0x0;
    double impulse_val = 1.0;
    double probe_val = 0.0;
    double expect_val = 0.0;
    uint64_t test_cell = 0;
    uint64_t interior_lo = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t found = 0;
    uint32_t err = 0;
    err = ldg_math_phys_compute_region_info_get(ctx, region, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    slot = &bridge->slot[region];
    err = ldg_gpu_buff_fill(bridge->dev, slot->tap_param.id, 0, LDG_MATH_PHYS_COMPUTE_TAP_PARAM_SIZE, 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    interior_lo = info.pml_depth + 1;
    for (idx_z = interior_lo; idx_z + interior_lo < info.nz && found == 0; idx_z++) { for (idx_y = interior_lo; idx_y + interior_lo < info.ny && found == 0; idx_y++) { for (idx_x = interior_lo; idx_x + interior_lo < info.nx && found == 0; idx_x++)
            {
                test_cell = (idx_z * info.ny + idx_y) * info.nx + idx_x;
                row = info.coeff + (uint64_t)info.mtl[test_cell] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                if (info.mtl[test_cell] == 0 && info.mtl[test_cell - info.nx] == 0 && info.mtl[test_cell - info.nx * info.ny] == 0) { found = 1; }
            }
        } }
    if (LDG_UNLIKELY(found == 0)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    expect_val = row[LDG_MATH_PHYS_EM_COEFF_CB] / info.spacing;
    if (LDG_UNLIKELY(!ldg_math_finite_is(expect_val) || expect_val <= 0.0)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(info.dt_sub) || info.dt_sub <= 0.0)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    err = ldg_gpu_buff_wr(bridge->dev, slot->magnetic.id, &impulse_val, sizeof(double), (2 * info.cell_cunt + test_cell) * sizeof(double));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    impulse_desc.pipeline_id = bridge->pipeline_electric;
    impulse_desc.group_cunt_x = slot->group_x;
    impulse_desc.group_cunt_y = slot->group_y;
    impulse_desc.group_cunt_z = slot->group_z;
    impulse_desc.buff_ids[0] = slot->electric.id;
    impulse_desc.buff_ids[1] = slot->magnetic.id;
    impulse_desc.buff_ids[2] = slot->param.id;
    impulse_desc.buff_ids[3] = slot->batch_blk.id;
    impulse_desc.buff_ids[4] = slot->tap_param.id;
    impulse_desc.buff_ids[5] = slot->mask.id;
    impulse_desc.buff_ids[6] = slot->cell_coeff.id;
    impulse_desc.buff_cunt = 7;
    err = ldg_gpu_batch_dispatch(bridge->dev, &impulse_desc, 1);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_rd(bridge->dev, slot->electric.id, &probe_val, sizeof(double), test_cell * sizeof(double));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(probe_val) || ldg_math_abs_get(probe_val - expect_val) > 1.0e-9 * ldg_math_abs_get(expect_val))) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    impulse_val = 0.0;
    err = ldg_gpu_buff_wr(bridge->dev, slot->magnetic.id, &impulse_val, sizeof(double), (2 * info.cell_cunt + test_cell) * sizeof(double));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_fill(bridge->dev, slot->electric.id, 0, 3 * info.cell_cunt * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    impulse_val = 1.0;
    err = ldg_gpu_buff_wr(bridge->dev, slot->electric.id, &impulse_val, sizeof(double), (info.cell_cunt + test_cell) * sizeof(double));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    impulse_desc.pipeline_id = bridge->pipeline_magnetic;
    impulse_desc.buff_ids[0] = slot->magnetic.id;
    impulse_desc.buff_ids[1] = slot->electric.id;
    impulse_desc.buff_cunt = 5;
    err = ldg_gpu_batch_dispatch(bridge->dev, &impulse_desc, 1);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_rd(bridge->dev, slot->magnetic.id, &probe_val, sizeof(double), (2 * info.cell_cunt + test_cell) * sizeof(double));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    expect_val = row[LDG_MATH_PHYS_EM_COEFF_DB] / info.spacing;
    if (LDG_UNLIKELY(!ldg_math_finite_is(probe_val) || ldg_math_abs_get(probe_val - expect_val) > 1.0e-9 * ldg_math_abs_get(expect_val))) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    err = ldg_gpu_buff_fill(bridge->dev, slot->electric.id, 0, 3 * info.cell_cunt * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_fill(bridge->dev, slot->magnetic.id, 0, 3 * info.cell_cunt * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_fill(bridge->dev, slot->batch_blk.id, 0, LDG_MATH_PHYS_COMPUTE_BATCH_BLK_SIZE, 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    return LDG_ERR_AOK;
}

#define LDG_MATH_PHYS_ACOUSTIC_GPU_FOOTPRINT_MAX 1372
#define LDG_MATH_PHYS_ACOUSTIC_GPU_PARAM_SIZE 24

static uint32_t ldg_math_phys_acoustic_region_arm(ldg_math_phys_ctx_t *ctx, uint32_t region, ldg_math_phys_compute_bridge_t *bridge, ldg_math_phys_compute_slot_t *slot)
{
    ldg_math_phys_acoustic_compute_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_gpu_buff_desc_t buff_desc = LDG_STRUCT_ZERO_INIT;
    uint8_t param_raw[LDG_MATH_PHYS_ACOUSTIC_GPU_PARAM_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t param_word[4] = LDG_ARR_ZERO_INIT;
    void *blk = 0x0;
    uint64_t mtl_word_cunt = 0;
    uint32_t err = 0;
    uint32_t ret = 0;
    err = ldg_math_phys_acoustic_compute_info_get(ctx, region, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(info.nz == 1)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(info.subcycle_cunt != 1)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(info.plane_armed)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(info.face_cunt != 0 || info.wall_cunt != 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(info.mono_cunt > LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(info.mtl_cunt == 0)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(info.cell_cunt > UINT32_MAX)) { return LDG_ERR_UNSUPPORTED; }

    mtl_word_cunt = (info.cell_cunt + 1) / 2;
    buff_desc.mem_flags = LDG_GPU_MEM_DEV_LOCAL;
    buff_desc.size = info.cell_cunt * sizeof(double);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->pressure);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    slot->kind = LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC;
    slot->armed = 1;
    slot->open = info.pml_depth > 0;
    buff_desc.size = 3 * info.cell_cunt * sizeof(double);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->velocity);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    buff_desc.size = LDG_MATH_PHYS_ACOUSTIC_PSI_CUNT * info.cell_cunt * sizeof(double);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->psi);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    buff_desc.size = info.cell_cunt * sizeof(double);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->drive);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    buff_desc.size = LDG_MATH_PHYS_ACOUSTIC_STRETCH_STRIDE * (info.nx + info.ny + info.nz) * sizeof(double);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->stretch);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    buff_desc.size = mtl_word_cunt * sizeof(uint32_t);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->mask);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    buff_desc.size = info.mtl_cunt * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE * sizeof(double);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->cell_coeff);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    buff_desc.size = LDG_MATH_PHYS_ACOUSTIC_GPU_PARAM_SIZE;
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->param);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    buff_desc.size = LDG_MATH_PHYS_COMPUTE_BATCH_BLK_SIZE;
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->batch_blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    buff_desc.size = LDG_MATH_PHYS_COMPUTE_TAP_PARAM_SIZE;
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->tap_param);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    buff_desc.size = LDG_MATH_PHYS_ACOUSTIC_GPU_FOOTPRINT_MAX * sizeof(uint32_t);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->footprint_cell);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    buff_desc.size = LDG_MATH_PHYS_ACOUSTIC_GPU_FOOTPRINT_MAX * LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX * sizeof(double);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->footprint_weight);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    buff_desc.size = 4 * sizeof(uint32_t);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->footprint_param);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    err = ldg_gpu_buff_wr(bridge->dev, slot->pressure.id, info.pressure, info.cell_cunt * sizeof(double), 0);
    if (err == LDG_ERR_AOK) { err = ldg_gpu_buff_wr(bridge->dev, slot->velocity.id, info.velocity, 3 * info.cell_cunt * sizeof(double), 0); }

    if (err == LDG_ERR_AOK) { err = ldg_gpu_buff_fill(bridge->dev, slot->psi.id, 0, LDG_MATH_PHYS_ACOUSTIC_PSI_CUNT * info.cell_cunt * sizeof(double), 0); }

    if (err == LDG_ERR_AOK) { err = ldg_gpu_buff_fill(bridge->dev, slot->drive.id, 0, info.cell_cunt * sizeof(double), 0); }

    if (err == LDG_ERR_AOK) { err = ldg_gpu_buff_wr(bridge->dev, slot->stretch.id, info.stretch, LDG_MATH_PHYS_ACOUSTIC_STRETCH_STRIDE * (info.nx + info.ny + info.nz) * sizeof(double), 0); }

    if (err == LDG_ERR_AOK) { err = ldg_gpu_buff_wr(bridge->dev, slot->mask.id, info.mtl, info.cell_cunt * sizeof(uint16_t), 0); }

    if (err == LDG_ERR_AOK) { err = ldg_gpu_buff_wr(bridge->dev, slot->cell_coeff.id, info.coeff, info.mtl_cunt * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE * sizeof(double), 0); }

    if (err == LDG_ERR_AOK) { err = ldg_gpu_buff_fill(bridge->dev, slot->batch_blk.id, 0, LDG_MATH_PHYS_COMPUTE_BATCH_BLK_SIZE, 0); }

    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    ldg_mem_cpy(param_raw, &info.spacing, sizeof(double));
    param_word[0] = (uint32_t)info.nx;
    param_word[1] = (uint32_t)info.ny;
    param_word[2] = (uint32_t)info.nz;
    param_word[3] = 0;
    ldg_mem_cpy(param_raw + sizeof(double), param_word, 4 * sizeof(uint32_t));
    err = ldg_gpu_buff_wr(bridge->dev, slot->param.id, param_raw, LDG_MATH_PHYS_ACOUSTIC_GPU_PARAM_SIZE, 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    err = ldg_mem_alloc(LDG_MATH_PHYS_EM_DRIVE_CAP * LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT * sizeof(double), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    slot->series_span = (double *)blk;
    err = ldg_mem_alloc(LDG_MATH_PHYS_EM_PROBE_CAP * LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT * sizeof(double), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    slot->tap_span = (double *)blk;
    err = ldg_mem_alloc(LDG_MATH_PHYS_ACOUSTIC_GPU_FOOTPRINT_MAX * sizeof(uint32_t), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    slot->footprint_cell_host = (uint32_t *)blk;
    err = ldg_mem_alloc(LDG_MATH_PHYS_ACOUSTIC_GPU_FOOTPRINT_MAX * LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX * sizeof(double), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    slot->footprint_weight_host = (double *)blk;
    slot->group_x = (uint32_t)((info.nx + 7) / 8);
    slot->group_y = (uint32_t)((info.ny + 7) / 8);
    slot->group_z = (uint32_t)((info.nz + 3) / 4);
    slot->cell_group = (uint32_t)((info.cell_cunt + 63) / 64);
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_vk_region_arm(ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    ldg_math_phys_compute_region_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_gpu_buff_desc_t buff_desc = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    const double *row = 0x0;
    const double *pattern_src = 0x0;
    double param_blk[8] = LDG_ARR_ZERO_INIT;
    double port_val[16] = LDG_ARR_ZERO_INIT;
    double *coeff_scratch = 0x0;
    uint32_t *mask_scratch = 0x0;
    void *blk = 0x0;
    uint64_t mask_word_cunt = 0;
    uint64_t cell = 0;
    uint64_t mtl_idx = 0;
    uint64_t pattern_cunt = 0;
    uint32_t param_word[4] = LDG_ARR_ZERO_INIT;
    uint32_t port_word[68] = LDG_ARR_ZERO_INIT;
    uint32_t kind = 0;
    uint32_t err = 0;
    uint32_t ret = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_DENIED; }

    kind = ldg_math_phys_compute_region_kind_get(ctx, region);
    if (kind == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC)
    {
        bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
        slot = &bridge->slot[region];
        if (LDG_UNLIKELY(slot->armed)) { return LDG_ERR_EXISTS; }

        return ldg_math_phys_acoustic_region_arm(ctx, region, bridge, slot);
    }

    err = ldg_math_phys_compute_region_info_get(ctx, region, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(info.nz == 1)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(info.subcycle_cunt != 1)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(info.face_cunt != 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(info.plane_armed)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(info.mtl_cunt == 0)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(info.cell_cunt > UINT32_MAX)) { return LDG_ERR_UNSUPPORTED; }

    for (mtl_idx = 0; mtl_idx < info.mtl_cunt; mtl_idx++)
    {
        row = info.coeff + mtl_idx * LDG_MATH_PHYS_EM_COEFF_STRIDE;
        if (LDG_UNLIKELY(ldg_math_abs_get(row[LDG_MATH_PHYS_EM_COEFF_MU] - LDG_MATH_PHYS_VACUUM_PERMEABILITY) > 0.0)) { return LDG_ERR_UNSUPPORTED; }
    }
    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(slot->armed)) { return LDG_ERR_EXISTS; }

    mask_word_cunt = (info.cell_cunt + 31) / 32;
    buff_desc.mem_flags = LDG_GPU_MEM_DEV_LOCAL;
    buff_desc.size = 3 * info.cell_cunt * sizeof(double);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->electric);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    slot->armed = 1;
    slot->open = info.pml_depth > 0;
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->magnetic);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { slot->open = 0; ret = ldg_math_phys_compute_slot_release(bridge, slot); return (ret != LDG_ERR_AOK) ? ret : err; }

    buff_desc.size = 8 * sizeof(double);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->param);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { slot->open = 0; ret = ldg_math_phys_compute_slot_release(bridge, slot); return (ret != LDG_ERR_AOK) ? ret : err; }

    buff_desc.size = 2 * info.cell_cunt * sizeof(double);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->cell_coeff);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { slot->open = 0; ret = ldg_math_phys_compute_slot_release(bridge, slot); return (ret != LDG_ERR_AOK) ? ret : err; }

    buff_desc.size = LDG_MATH_PHYS_COMPUTE_BATCH_BLK_SIZE;
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->batch_blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { slot->open = 0; ret = ldg_math_phys_compute_slot_release(bridge, slot); return (ret != LDG_ERR_AOK) ? ret : err; }

    buff_desc.size = LDG_MATH_PHYS_COMPUTE_TAP_PARAM_SIZE;
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->tap_param);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { slot->open = 0; ret = ldg_math_phys_compute_slot_release(bridge, slot); return (ret != LDG_ERR_AOK) ? ret : err; }

    buff_desc.size = mask_word_cunt * sizeof(uint32_t);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->mask);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { slot->open = 0; ret = ldg_math_phys_compute_slot_release(bridge, slot); return (ret != LDG_ERR_AOK) ? ret : err; }

    if (slot->open)
    {
        buff_desc.size = 12 * info.cell_cunt * sizeof(double);
        err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->psi);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { slot->open = 0; ret = ldg_math_phys_compute_slot_release(bridge, slot); return (ret != LDG_ERR_AOK) ? ret : err; }

        buff_desc.size = 6 * (info.nx + info.ny + info.nz) * sizeof(double);
        err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->stretch);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            ret = compute_slot_buff_release(bridge->dev, &slot->psi);
            slot->open = 0;
            ret = ldg_math_phys_compute_slot_release(bridge, slot);
            return (ret != LDG_ERR_AOK) ? ret : err;
        }

        err = ldg_gpu_buff_fill(bridge->dev, slot->psi.id, 0, 12 * info.cell_cunt * sizeof(double), 0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            ret = ldg_math_phys_compute_slot_release(bridge, slot);
            if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

            return err;
        }

        err = ldg_gpu_buff_wr(bridge->dev, slot->stretch.id, info.stretch, 6 * (info.nx + info.ny + info.nz) * sizeof(double), 0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            ret = ldg_math_phys_compute_slot_release(bridge, slot);
            if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

            return err;
        }
    }

    err = ldg_mem_alloc(2 * info.cell_cunt * sizeof(double), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        if (ret != LDG_ERR_AOK) { LDG_ERRLOG_WARN("phys: compute slot rollback failed"); }

        return err;
    }

    coeff_scratch = (double *)blk;
    err = ldg_mem_alloc(mask_word_cunt * sizeof(uint32_t), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_mem_dealloc(coeff_scratch);
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        return (ret != LDG_ERR_AOK) ? ret : err;
    }

    mask_scratch = (uint32_t *)blk;
    ldg_mem_zero(mask_scratch, mask_word_cunt * sizeof(uint32_t));
    for (cell = 0; cell < info.cell_cunt; cell++)
    {
        row = info.coeff + (uint64_t)info.mtl[cell] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
        coeff_scratch[cell] = row[LDG_MATH_PHYS_EM_COEFF_CA];
        coeff_scratch[info.cell_cunt + cell] = row[LDG_MATH_PHYS_EM_COEFF_CB];
        if (ldg_math_inf_is(row[LDG_MATH_PHYS_EM_COEFF_SIGMA])) { mask_scratch[cell >> 5] |= 1u << (cell & 31u); }
    }
    err = ldg_gpu_buff_wr(bridge->dev, slot->cell_coeff.id, coeff_scratch, 2 * info.cell_cunt * sizeof(double), 0);
    if (err == LDG_ERR_AOK) { err = ldg_gpu_buff_wr(bridge->dev, slot->mask.id, mask_scratch, mask_word_cunt * sizeof(uint32_t), 0); }

    if (err == LDG_ERR_AOK) { err = ldg_gpu_buff_wr(bridge->dev, slot->electric.id, info.electric, 3 * info.cell_cunt * sizeof(double), 0); }

    if (err == LDG_ERR_AOK) { err = ldg_gpu_buff_wr(bridge->dev, slot->magnetic.id, info.magnetic, 3 * info.cell_cunt * sizeof(double), 0); }

    if (err == LDG_ERR_AOK) { err = ldg_gpu_buff_fill(bridge->dev, slot->batch_blk.id, 0, LDG_MATH_PHYS_COMPUTE_BATCH_BLK_SIZE, 0); }

    ret = ldg_mem_dealloc(coeff_scratch);
    if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

    ret = ldg_mem_dealloc(mask_scratch);
    if (ret != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = ret; }

    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        return (ret != LDG_ERR_AOK) ? ret : err;
    }

    param_blk[0] = 1.0 / info.spacing;
    param_blk[1] = 1.0;
    param_blk[2] = info.dt_sub / LDG_MATH_PHYS_VACUUM_PERMITTIVITY;
    param_blk[3] = info.dt_sub / LDG_MATH_PHYS_VACUUM_PERMEABILITY;
    param_word[0] = (uint32_t)info.nx;
    param_word[1] = (uint32_t)info.ny;
    param_word[2] = (uint32_t)info.nz;
    param_word[3] = (uint32_t)info.pml_depth;
    ldg_mem_cpy(&param_blk[4], param_word, 4 * sizeof(uint32_t));
    param_word[0] = 1;
    param_word[1] = 0;
    param_word[2] = 0;
    param_word[3] = 0;
    ldg_mem_cpy(&param_blk[6], param_word, 4 * sizeof(uint32_t));
    err = ldg_gpu_buff_wr(bridge->dev, slot->param.id, param_blk, 8 * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        return (ret != LDG_ERR_AOK) ? ret : err;
    }

    err = ldg_mem_alloc(LDG_MATH_PHYS_EM_DRIVE_CAP * LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT * sizeof(double), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        return (ret != LDG_ERR_AOK) ? ret : err;
    }

    slot->series_span = (double *)blk;
    err = ldg_mem_alloc(LDG_MATH_PHYS_EM_PROBE_CAP * LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT * sizeof(double), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        return (ret != LDG_ERR_AOK) ? ret : err;
    }

    slot->tap_span = (double *)blk;
    slot->group_x = (uint32_t)((info.nx + 7) / 8);
    slot->group_y = (uint32_t)((info.ny + 7) / 8);
    slot->group_z = (uint32_t)((info.nz + 3) / 4);
    slot->tick_desc_cunt = slot->open ? 4 : 2;
    slot->port_pm_cunt = 0;
    err = ldg_math_phys_em_port_gpu_pack(ctx, region, port_word, port_val, &pattern_src, &pattern_cunt, &slot->port_pm_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        return (ret != LDG_ERR_AOK) ? ret : err;
    }

    if (slot->port_pm_cunt > 0)
    {
        buff_desc.size = LDG_MATH_PHYS_COMPUTE_PORT_PARAM_SIZE;
        err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->port_param);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { slot->port_pm_cunt = 0; ret = ldg_math_phys_compute_slot_release(bridge, slot); return (ret != LDG_ERR_AOK) ? ret : err; }

        buff_desc.size = pattern_cunt * sizeof(double);
        err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->port_pattern);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            ret = compute_slot_buff_release(bridge->dev, &slot->port_param);
            slot->port_pm_cunt = 0;
            ret = ldg_math_phys_compute_slot_release(bridge, slot);
            return (ret != LDG_ERR_AOK) ? ret : err;
        }

        err = ldg_gpu_buff_wr(bridge->dev, slot->port_param.id, port_word, 272, 0);
        if (err == LDG_ERR_AOK) { err = ldg_gpu_buff_wr(bridge->dev, slot->port_param.id, port_val, 128, 272); }

        if (err == LDG_ERR_AOK) { err = ldg_gpu_buff_wr(bridge->dev, slot->port_pattern.id, pattern_src, pattern_cunt * sizeof(double), 0); }

        if (err == LDG_ERR_AOK) { err = ldg_mem_alloc(LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT * LDG_MATH_PHYS_COMPUTE_PORT_SLOT_CUNT * 2 * sizeof(double), &blk); }

        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            ret = ldg_math_phys_compute_slot_release(bridge, slot);
            return (ret != LDG_ERR_AOK) ? ret : err;
        }

        slot->port_span = (double *)blk;
        slot->tick_desc_cunt += 1;
    }

    err = ldg_math_phys_compute_selftest(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = ldg_math_phys_compute_slot_release(bridge, slot);
        return (ret != LDG_ERR_AOK) ? ret : err;
    }

    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_acoustic_pack_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t vol_stride, uint32_t slice_z)
{
    ldg_math_phys_acoustic_compute_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_gpu_buff_desc_t buff_desc = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    uint64_t vol_cell = 0;
    uint32_t pack_word[8] = LDG_ARR_ZERO_INIT;
    uint32_t err = 0;
    uint32_t ret = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_DENIED; }

    err = ldg_math_phys_acoustic_compute_info_get(ctx, region, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(!slot->armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(slot->pack_armed)) { return LDG_ERR_EXISTS; }

    if (LDG_UNLIKELY(slice_z >= info.nz)) { return LDG_ERR_FUNC_ARG_INVALID; }

    slot->vol_stride = vol_stride;
    slot->slice_z = slice_z;
    slot->pack_plane = info.nx * info.ny;
    vol_cell = 0;
    if (vol_stride > 0)
    {
        slot->vol_dim[0] = (info.nx + vol_stride - 1) / vol_stride;
        slot->vol_dim[1] = (info.ny + vol_stride - 1) / vol_stride;
        slot->vol_dim[2] = (info.nz + vol_stride - 1) / vol_stride;
        vol_cell = slot->vol_dim[0] * slot->vol_dim[1] * slot->vol_dim[2];
    }
    else
    {
        slot->vol_dim[0] = 0;
        slot->vol_dim[1] = 0;
        slot->vol_dim[2] = 0;
    }

    buff_desc.mem_flags = LDG_GPU_MEM_HOST_VISIBLE;
    buff_desc.size = (4 * vol_cell + 4 * slot->pack_plane) * sizeof(double);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->pack);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    buff_desc.mem_flags = LDG_GPU_MEM_DEV_LOCAL;
    buff_desc.size = 8 * sizeof(uint32_t);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->pack_param);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = compute_slot_buff_release(bridge->dev, &slot->pack);
        return (ret != LDG_ERR_AOK) ? ret : err;
    }

    pack_word[0] = (uint32_t)info.nx;
    pack_word[1] = (uint32_t)info.ny;
    pack_word[2] = (uint32_t)info.nz;
    pack_word[3] = vol_stride;
    for (axis = 0; axis < 3; axis++) { pack_word[4 + axis] = (uint32_t)slot->vol_dim[axis]; }

    pack_word[7] = slice_z;
    err = ldg_gpu_buff_wr(bridge->dev, slot->pack_param.id, pack_word, 8 * sizeof(uint32_t), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = compute_slot_buff_release(bridge->dev, &slot->pack);
        if (ret == LDG_ERR_AOK) { ret = compute_slot_buff_release(bridge->dev, &slot->pack_param); }

        return (ret != LDG_ERR_AOK) ? ret : err;
    }

    slot->pack_armed = 1;
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_acoustic_pack_get(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_vol, double *out_slice)
{
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    uint64_t vol_cell = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!out_vol && !out_slice)) { return LDG_ERR_FUNC_ARG_NULL; }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    if (LDG_UNLIKELY(region >= ctx->conf.region_cap)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(!slot->pack_armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(slot->in_flight)) { return LDG_ERR_BUSY; }

    vol_cell = slot->vol_dim[0] * slot->vol_dim[1] * slot->vol_dim[2];
    if (out_vol != 0x0)
    {
        if (LDG_UNLIKELY(slot->vol_stride == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        err = ldg_gpu_buff_rd(bridge->dev, slot->pack.id, out_vol, 4 * vol_cell * sizeof(double), 0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    if (out_slice != 0x0)
    {
        err = ldg_gpu_buff_rd(bridge->dev, slot->pack.id, out_slice, 4 * slot->pack_plane * sizeof(double), 4 * vol_cell * sizeof(double));
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_vk_pack_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t vol_stride, uint32_t slice_z)
{
    ldg_math_phys_compute_region_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_gpu_buff_desc_t buff_desc = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    uint64_t vol_cell = 0;
    uint32_t pack_word[8] = LDG_ARR_ZERO_INIT;
    uint32_t err = 0;
    uint32_t ret = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (ldg_math_phys_compute_region_kind_get(ctx, region) == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC) { return ldg_math_phys_acoustic_pack_set(ctx, region, vol_stride, slice_z); }

    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_DENIED; }

    err = ldg_math_phys_compute_region_info_get(ctx, region, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(!slot->armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(slot->pack_armed)) { return LDG_ERR_EXISTS; }

    if (LDG_UNLIKELY(slice_z >= info.nz)) { return LDG_ERR_FUNC_ARG_INVALID; }

    slot->vol_stride = vol_stride;
    slot->slice_z = slice_z;
    slot->pack_plane = info.nx * info.ny;
    vol_cell = 1;
    if (vol_stride > 0)
    {
        slot->vol_dim[0] = (info.nx + vol_stride - 1) / vol_stride;
        slot->vol_dim[1] = (info.ny + vol_stride - 1) / vol_stride;
        slot->vol_dim[2] = (info.nz + vol_stride - 1) / vol_stride;
        vol_cell = slot->vol_dim[0] * slot->vol_dim[1] * slot->vol_dim[2];
    }
    else
    {
        slot->vol_dim[0] = 0;
        slot->vol_dim[1] = 0;
        slot->vol_dim[2] = 0;
        vol_cell = 0;
    }

    buff_desc.mem_flags = LDG_GPU_MEM_HOST_VISIBLE;
    buff_desc.size = (6 * vol_cell + 6 * slot->pack_plane) * sizeof(double);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->pack);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    buff_desc.mem_flags = LDG_GPU_MEM_DEV_LOCAL;
    buff_desc.size = 8 * sizeof(uint32_t);
    err = ldg_gpu_buff_create(bridge->dev, &buff_desc, &slot->pack_param);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = compute_slot_buff_release(bridge->dev, &slot->pack);
        return (ret != LDG_ERR_AOK) ? ret : err;
    }

    pack_word[0] = (uint32_t)info.nx;
    pack_word[1] = (uint32_t)info.ny;
    pack_word[2] = (uint32_t)info.nz;
    pack_word[3] = vol_stride;
    for (axis = 0; axis < 3; axis++) { pack_word[4 + axis] = (uint32_t)slot->vol_dim[axis]; }
    pack_word[7] = slice_z;
    err = ldg_gpu_buff_wr(bridge->dev, slot->pack_param.id, pack_word, 8 * sizeof(uint32_t), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ret = compute_slot_buff_release(bridge->dev, &slot->pack);
        if (ret == LDG_ERR_AOK) { ret = compute_slot_buff_release(bridge->dev, &slot->pack_param); }

        return (ret != LDG_ERR_AOK) ? ret : err;
    }

    slot->pack_armed = 1;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_vk_pack_get(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_vol, double *out_slice)
{
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    uint64_t vol_cell = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_vol && !out_slice)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (ldg_math_phys_compute_region_kind_get(ctx, region) == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC) { return ldg_math_phys_acoustic_pack_get(ctx, region, out_vol, out_slice); }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    if (LDG_UNLIKELY(region >= ctx->conf.region_cap)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(!slot->pack_armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(slot->in_flight)) { return LDG_ERR_BUSY; }

    vol_cell = slot->vol_dim[0] * slot->vol_dim[1] * slot->vol_dim[2];
    if (out_vol != 0x0)
    {
        if (LDG_UNLIKELY(slot->vol_stride == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        err = ldg_gpu_buff_rd(bridge->dev, slot->pack.id, out_vol, 6 * vol_cell * sizeof(double), 0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    if (out_slice != 0x0)
    {
        err = ldg_gpu_buff_rd(bridge->dev, slot->pack.id, out_slice, 6 * slot->pack_plane * sizeof(double), 6 * vol_cell * sizeof(double));
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_compute_tap_param_wr(ldg_math_phys_compute_bridge_t *bridge, ldg_math_phys_compute_slot_t *slot, const ldg_math_phys_compute_region_info_t *info, const double *probe_pos, uint32_t probe_cunt)
{
    uint8_t tap_param_raw[LDG_MATH_PHYS_COMPUTE_TAP_PARAM_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t tap_param_word[40] = LDG_ARR_ZERO_INIT;
    double frac[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double weight_one = 1.0;
    double coord = 0.0;
    uint64_t probe_base[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t probe_corner = 0;
    uint64_t slot_idx = 0;
    uint32_t probe_idx = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(probe_cunt > LDG_MATH_PHYS_EM_PROBE_CAP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(probe_cunt > 0 && probe_pos == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (probe_idx = 0; probe_idx < probe_cunt; probe_idx++)
    {
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            coord = probe_pos[probe_idx * LDG_MATH_PHYS_DIM + axis];
            if (LDG_UNLIKELY(!ldg_math_finite_is(coord) || coord < 0.0)) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

            coord /= info->spacing;
            probe_base[axis] = (uint64_t)coord;
            frac[axis] = coord - (double)probe_base[axis];
        }
        if (LDG_UNLIKELY(probe_base[0] + 1 >= info->nx || probe_base[1] + 1 >= info->ny || probe_base[2] + 1 >= info->nz)) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

        probe_corner = (probe_base[2] * info->ny + probe_base[1]) * info->nx + probe_base[0];
        ldg_mem_cpy(tap_param_raw + probe_idx * sizeof(double), &frac[0], sizeof(double));
        ldg_mem_cpy(tap_param_raw + (LDG_MATH_PHYS_EM_PROBE_CAP + probe_idx) * sizeof(double), &frac[1], sizeof(double));
        ldg_mem_cpy(tap_param_raw + (2 * LDG_MATH_PHYS_EM_PROBE_CAP + probe_idx) * sizeof(double), &frac[2], sizeof(double));
        tap_param_word[probe_idx] = (uint32_t)probe_corner;
    }
    for (slot_idx = 0; slot_idx < LDG_MATH_PHYS_EM_PROBE_CAP; slot_idx++) { ldg_mem_cpy(tap_param_raw + (3 * LDG_MATH_PHYS_EM_PROBE_CAP + slot_idx) * sizeof(double), &weight_one, sizeof(double)); }
    tap_param_word[8] = (uint32_t)info->nx;
    tap_param_word[9] = (uint32_t)(info->nx * info->ny);
    tap_param_word[10] = LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT;
    tap_param_word[11] = probe_cunt;
    for (slot_idx = 0; slot_idx < info->drive_cunt; slot_idx++)
    {
        tap_param_word[12 + slot_idx] = (uint32_t)info->drive[slot_idx].cell;
        tap_param_word[20 + slot_idx] = info->drive[slot_idx].comp;
        tap_param_word[28 + slot_idx] = (uint32_t)slot_idx;
    }
    tap_param_word[36] = (uint32_t)info->drive_cunt;
    ldg_mem_cpy(tap_param_raw + 4 * LDG_MATH_PHYS_EM_PROBE_CAP * sizeof(double), tap_param_word, 40 * sizeof(uint32_t));
    return ldg_gpu_buff_wr(bridge->dev, slot->tap_param.id, tap_param_raw, LDG_MATH_PHYS_COMPUTE_TAP_PARAM_SIZE, 0);
}

static uint32_t ldg_math_phys_compute_series_stage(ldg_math_phys_compute_bridge_t *bridge, ldg_math_phys_compute_slot_t *slot, const ldg_math_phys_compute_region_info_t *info, uint64_t tick_base, uint64_t span)
{
    double sample = 0.0;
    uint64_t slot_idx = 0;
    uint64_t batch_tick = 0;
    uint64_t global_tick = 0;
    uint32_t err = 0;
    if (info->drive_cunt == 0) { return LDG_ERR_AOK; }

    ldg_mem_zero(slot->series_span, LDG_MATH_PHYS_EM_DRIVE_CAP * LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT * sizeof(double));
    for (slot_idx = 0; slot_idx < info->drive_cunt; slot_idx++) { for (batch_tick = 0; batch_tick < span; batch_tick++)
        {
            global_tick = tick_base + batch_tick;
            if (global_tick < info->drive[slot_idx].tick_start) { continue; }

            err = ldg_math_phys_sig_sample_get(&info->drive[slot_idx].sig, global_tick - info->drive[slot_idx].tick_start, info->dt_sub, &sample);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            slot->series_span[slot_idx * LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT + batch_tick] = sample;
        }
    }
    return ldg_gpu_buff_wr(bridge->dev, slot->batch_blk.id, slot->series_span, info->drive_cunt * LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT * sizeof(double), LDG_MATH_PHYS_COMPUTE_SERIES_OFFSET);
}

static uint32_t ldg_math_phys_compute_batch_build(ldg_math_phys_compute_bridge_t *bridge, ldg_math_phys_compute_slot_t *slot, uint64_t span, uint8_t final_batch)
{
    ldg_gpu_dispatch_desc_t *tick_desc = 0x0;
    uint64_t batch_tick = 0;
    uint32_t dispatch_cunt = 0;
    ldg_mem_zero(&bridge->desc[0], sizeof(ldg_gpu_dispatch_desc_t));
    bridge->desc[0].pipeline_id = bridge->pipeline_prime;
    bridge->desc[0].group_cunt_x = 1;
    bridge->desc[0].group_cunt_y = 1;
    bridge->desc[0].group_cunt_z = 1;
    bridge->desc[0].buff_ids[0] = slot->batch_blk.id;
    bridge->desc[0].buff_cunt = 1;
    for (batch_tick = 0; batch_tick < span; batch_tick++)
    {
        tick_desc = &bridge->desc[1 + (uint64_t)slot->tick_desc_cunt * batch_tick];
        ldg_mem_zero(tick_desc, (uint64_t)slot->tick_desc_cunt * sizeof(ldg_gpu_dispatch_desc_t));
        tick_desc[0].pipeline_id = bridge->pipeline_electric;
        tick_desc[0].group_cunt_x = slot->group_x;
        tick_desc[0].group_cunt_y = slot->group_y;
        tick_desc[0].group_cunt_z = slot->group_z;
        tick_desc[0].buff_ids[0] = slot->electric.id;
        tick_desc[0].buff_ids[1] = slot->magnetic.id;
        tick_desc[0].buff_ids[2] = slot->param.id;
        tick_desc[0].buff_ids[3] = slot->batch_blk.id;
        tick_desc[0].buff_ids[4] = slot->tap_param.id;
        tick_desc[0].buff_ids[5] = slot->mask.id;
        tick_desc[0].buff_ids[6] = slot->cell_coeff.id;
        tick_desc[0].buff_cunt = 7;
        if (slot->open)
        {
            tick_desc[1] = tick_desc[0];
            tick_desc[1].pipeline_id = bridge->pipeline_electric_cpml;
            tick_desc[1].buff_ids[6] = slot->psi.id;
            tick_desc[1].buff_ids[7] = slot->stretch.id;
            tick_desc[1].buff_ids[8] = slot->cell_coeff.id;
            tick_desc[1].buff_cunt = 9;
            tick_desc[1].concurrent = 1;
        }

        tick_desc[slot->tick_desc_cunt / 2] = tick_desc[0];
        tick_desc[slot->tick_desc_cunt / 2].pipeline_id = bridge->pipeline_magnetic;
        tick_desc[slot->tick_desc_cunt / 2].buff_ids[0] = slot->magnetic.id;
        tick_desc[slot->tick_desc_cunt / 2].buff_ids[1] = slot->electric.id;
        tick_desc[slot->tick_desc_cunt / 2].buff_cunt = 5;
        if (slot->open)
        {
            tick_desc[3].pipeline_id = bridge->pipeline_magnetic_cpml;
            tick_desc[3].group_cunt_x = slot->group_x;
            tick_desc[3].group_cunt_y = slot->group_y;
            tick_desc[3].group_cunt_z = slot->group_z;
            tick_desc[3].buff_ids[0] = slot->magnetic.id;
            tick_desc[3].buff_ids[1] = slot->electric.id;
            tick_desc[3].buff_ids[2] = slot->param.id;
            tick_desc[3].buff_ids[3] = slot->psi.id;
            tick_desc[3].buff_ids[4] = slot->stretch.id;
            tick_desc[3].buff_cunt = 5;
            tick_desc[3].concurrent = 1;
        }

        if (slot->port_pm_cunt > 0)
        {
            tick_desc[slot->tick_desc_cunt - 1].pipeline_id = bridge->pipeline_em_port_tap;
            tick_desc[slot->tick_desc_cunt - 1].group_cunt_x = slot->port_pm_cunt;
            tick_desc[slot->tick_desc_cunt - 1].group_cunt_y = 1;
            tick_desc[slot->tick_desc_cunt - 1].group_cunt_z = 1;
            tick_desc[slot->tick_desc_cunt - 1].buff_ids[0] = slot->electric.id;
            tick_desc[slot->tick_desc_cunt - 1].buff_ids[1] = slot->magnetic.id;
            tick_desc[slot->tick_desc_cunt - 1].buff_ids[2] = slot->batch_blk.id;
            tick_desc[slot->tick_desc_cunt - 1].buff_ids[3] = slot->port_param.id;
            tick_desc[slot->tick_desc_cunt - 1].buff_ids[4] = slot->port_pattern.id;
            tick_desc[slot->tick_desc_cunt - 1].buff_cunt = 5;
        }
    }
    dispatch_cunt = 1 + slot->tick_desc_cunt * (uint32_t)span;
    if (slot->pack_armed && final_batch)
    {
        if (slot->vol_stride > 0)
        {
            ldg_mem_zero(&bridge->desc[dispatch_cunt], sizeof(ldg_gpu_dispatch_desc_t));
            bridge->desc[dispatch_cunt].pipeline_id = bridge->pipeline_vol_pack;
            bridge->desc[dispatch_cunt].group_cunt_x = (uint32_t)((slot->vol_dim[0] + 3) / 4);
            bridge->desc[dispatch_cunt].group_cunt_y = (uint32_t)((slot->vol_dim[1] + 3) / 4);
            bridge->desc[dispatch_cunt].group_cunt_z = (uint32_t)((slot->vol_dim[2] + 3) / 4);
            bridge->desc[dispatch_cunt].buff_ids[0] = slot->electric.id;
            bridge->desc[dispatch_cunt].buff_ids[1] = slot->magnetic.id;
            bridge->desc[dispatch_cunt].buff_ids[2] = slot->pack.id;
            bridge->desc[dispatch_cunt].buff_ids[3] = slot->pack_param.id;
            bridge->desc[dispatch_cunt].buff_cunt = 4;
            dispatch_cunt++;
        }

        ldg_mem_zero(&bridge->desc[dispatch_cunt], sizeof(ldg_gpu_dispatch_desc_t));
        bridge->desc[dispatch_cunt].pipeline_id = bridge->pipeline_slice_pack;
        bridge->desc[dispatch_cunt].group_cunt_x = slot->group_x;
        bridge->desc[dispatch_cunt].group_cunt_y = slot->group_y;
        bridge->desc[dispatch_cunt].group_cunt_z = 1;
        bridge->desc[dispatch_cunt].buff_ids[0] = slot->electric.id;
        bridge->desc[dispatch_cunt].buff_ids[1] = slot->magnetic.id;
        bridge->desc[dispatch_cunt].buff_ids[2] = slot->pack.id;
        bridge->desc[dispatch_cunt].buff_ids[3] = slot->pack_param.id;
        bridge->desc[dispatch_cunt].buff_cunt = 4;
        dispatch_cunt++;
    }

    return dispatch_cunt;
}

LDG_NOINLINE static uint32_t ldg_math_phys_acoustic_footprint_upload(ldg_math_phys_ctx_t *ctx, ldg_math_phys_compute_bridge_t *bridge, ldg_math_phys_compute_slot_t *slot, const ldg_math_phys_acoustic_compute_info_t *info, uint32_t region)
{
    double body_pos[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double grid_pos[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double radius_cell = LDG_MATH_PHYS_ACOUSTIC_MONOPOLE_RADIUS_CELL;
    double alpha = LDG_MATH_PHYS_ACOUSTIC_MONOPOLE_ALPHA;
    double dist_sq = 0.0;
    double weight = 0.0;
    double weight_sum = 0.0;
    double delta = 0.0;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t lo[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t hi[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t cell = 0;
    uint32_t param_word[4] = LDG_ARR_ZERO_INIT;
    uint32_t footprint_cunt = 0;
    uint32_t src = 0;
    uint32_t body_region = 0;
    uint32_t axis = 0;
    uint32_t found = 0;
    uint32_t entry = 0;
    uint32_t weight_idx = 0;
    uint32_t err = 0;
    dim[0] = info->nx;
    dim[1] = info->ny;
    dim[2] = info->nz;
    for (src = 0; src < (uint32_t)info->mono_cunt && src < LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX; src++)
    {
        err = ldg_math_phys_acoustic_body_pos_get(ctx, info->mono_body[src], body_pos, &body_region);
        if (err != LDG_ERR_AOK || body_region != region) { continue; }

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            grid_pos[axis] = body_pos[axis] / info->spacing;
            delta = grid_pos[axis] - radius_cell;
            lo[axis] = (delta > 0.0) ? (uint64_t)delta : 0;
            delta = grid_pos[axis] + radius_cell;
            hi[axis] = (delta < 0.0) ? 0 : (uint64_t)delta + 1;
            if (hi[axis] > dim[axis] - 1) { hi[axis] = dim[axis] - 1; }
        }
        weight_sum = 0.0;
        for (idx_z = lo[2]; idx_z <= hi[2]; idx_z++) { for (idx_y = lo[1]; idx_y <= hi[1]; idx_y++) { for (idx_x = lo[0]; idx_x <= hi[0]; idx_x++)
                {
                    dist_sq = ((double)idx_x - grid_pos[0]) * ((double)idx_x - grid_pos[0]) + ((double)idx_y - grid_pos[1]) * ((double)idx_y - grid_pos[1]) + ((double)idx_z - grid_pos[2]) * ((double)idx_z - grid_pos[2]);
                    if (dist_sq > radius_cell * radius_cell) { continue; }

                    weight_sum += ldg_math_exp_get((0.0 - dist_sq) / (alpha * alpha));
                }
            } }
        if (weight_sum <= 0.0) { continue; }

        for (idx_z = lo[2]; idx_z <= hi[2]; idx_z++)
        {
            for (idx_y = lo[1]; idx_y <= hi[1]; idx_y++)
            {
                for (idx_x = lo[0]; idx_x <= hi[0]; idx_x++)
                {
                    dist_sq = ((double)idx_x - grid_pos[0]) * ((double)idx_x - grid_pos[0]) + ((double)idx_y - grid_pos[1]) * ((double)idx_y - grid_pos[1]) + ((double)idx_z - grid_pos[2]) * ((double)idx_z - grid_pos[2]);
                    if (dist_sq > radius_cell * radius_cell) { continue; }

                    weight = ldg_math_exp_get((0.0 - dist_sq) / (alpha * alpha)) / weight_sum;
                    cell = (idx_z * info->ny + idx_y) * info->nx + idx_x;
                    found = UINT32_MAX;
                    for (entry = 0; entry < footprint_cunt; entry++) { if (slot->footprint_cell_host[entry] == (uint32_t)cell) { found = entry; break; } }

                    if (found == UINT32_MAX)
                    {
                        if (LDG_UNLIKELY(footprint_cunt >= LDG_MATH_PHYS_ACOUSTIC_GPU_FOOTPRINT_MAX)) { return LDG_ERR_FULL; }

                        found = footprint_cunt;
                        footprint_cunt++;
                        slot->footprint_cell_host[found] = (uint32_t)cell;
                        for (weight_idx = 0; weight_idx < LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX; weight_idx++) { slot->footprint_weight_host[found * LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX + weight_idx] = 0.0; }
                    }

                    slot->footprint_weight_host[found * LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX + src] += weight;
                }
            }
        }
    }
    slot->footprint_cunt = footprint_cunt;
    if (footprint_cunt > 0)
    {
        err = ldg_gpu_buff_wr(bridge->dev, slot->footprint_cell.id, slot->footprint_cell_host, footprint_cunt * sizeof(uint32_t), 0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_gpu_buff_wr(bridge->dev, slot->footprint_weight.id, slot->footprint_weight_host, footprint_cunt * LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX * sizeof(double), 0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    param_word[0] = footprint_cunt;
    return ldg_gpu_buff_wr(bridge->dev, slot->footprint_param.id, param_word, 4 * sizeof(uint32_t), 0);
}

LDG_NOINLINE static uint32_t ldg_math_phys_acoustic_tap_param_wr(ldg_math_phys_compute_bridge_t *bridge, ldg_math_phys_compute_slot_t *slot, const ldg_math_phys_acoustic_compute_info_t *info, const double *probe_pos, uint32_t probe_cunt)
{
    uint8_t raw[LDG_MATH_PHYS_COMPUTE_TAP_PARAM_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t word[40] = LDG_ARR_ZERO_INIT;
    double frac[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double weight_one = 1.0;
    double coord = 0.0;
    uint64_t base[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t corner = 0;
    uint64_t slot_idx = 0;
    uint32_t probe_idx = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(probe_cunt > LDG_MATH_PHYS_EM_PROBE_CAP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(probe_cunt > 0 && probe_pos == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (probe_idx = 0; probe_idx < probe_cunt; probe_idx++)
    {
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            coord = probe_pos[probe_idx * LDG_MATH_PHYS_DIM + axis];
            if (LDG_UNLIKELY(!ldg_math_finite_is(coord) || coord < 0.0)) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

            coord /= info->spacing;
            base[axis] = (uint64_t)coord;
            frac[axis] = coord - (double)base[axis];
        }
        if (LDG_UNLIKELY(base[0] + 1 >= info->nx || base[1] + 1 >= info->ny || base[2] + 1 >= info->nz)) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

        corner = (base[2] * info->ny + base[1]) * info->nx + base[0];
        ldg_mem_cpy(raw + probe_idx * sizeof(double), &frac[0], sizeof(double));
        ldg_mem_cpy(raw + (LDG_MATH_PHYS_EM_PROBE_CAP + probe_idx) * sizeof(double), &frac[1], sizeof(double));
        ldg_mem_cpy(raw + (2 * LDG_MATH_PHYS_EM_PROBE_CAP + probe_idx) * sizeof(double), &frac[2], sizeof(double));
        word[probe_idx] = (uint32_t)corner;
    }
    for (slot_idx = 0; slot_idx < LDG_MATH_PHYS_EM_PROBE_CAP; slot_idx++) { ldg_mem_cpy(raw + (3 * LDG_MATH_PHYS_EM_PROBE_CAP + slot_idx) * sizeof(double), &weight_one, sizeof(double)); }

    word[8] = (uint32_t)info->nx;
    word[9] = (uint32_t)(info->nx * info->ny);
    word[10] = LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT;
    word[11] = probe_cunt;
    ldg_mem_cpy(raw + 4 * LDG_MATH_PHYS_EM_PROBE_CAP * sizeof(double), word, 40 * sizeof(uint32_t));
    return ldg_gpu_buff_wr(bridge->dev, slot->tap_param.id, raw, LDG_MATH_PHYS_COMPUTE_TAP_PARAM_SIZE, 0);
}

LDG_NOINLINE static uint32_t ldg_math_phys_acoustic_series_stage(ldg_math_phys_compute_bridge_t *bridge, ldg_math_phys_compute_slot_t *slot, const ldg_math_phys_acoustic_compute_info_t *info, uint64_t tick_base, uint64_t span)
{
    double dt_sub = info->dt_sub;
    double t_rel = 0.0;
    double cycle = 0.0;
    double ramp = 0.0;
    double sample = 0.0;
    uint64_t src = 0;
    uint64_t batch_tick = 0;
    uint64_t global_tick = 0;
    uint64_t sample_idx = 0;
    ldg_mem_zero(slot->series_span, LDG_MATH_PHYS_EM_DRIVE_CAP * LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT * sizeof(double));
    for (src = 0; src < info->mono_cunt && src < LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX; src++)
    {
        for (batch_tick = 0; batch_tick < span; batch_tick++)
        {
            global_tick = tick_base + batch_tick;
            if (global_tick < info->mono_tick_start[src]) { continue; }

            sample_idx = global_tick - info->mono_tick_start[src];
            if (info->mono_series[src] != 0x0)
            {
                if (sample_idx >= info->mono_series_cunt[src]) { continue; }

                sample = info->mono_amplitude[src] * info->mono_series[src][sample_idx];
            }
            else
            {
                t_rel = (double)sample_idx * dt_sub;
                cycle = info->mono_freq[src] * t_rel;
                ramp = 1.0;
                if (cycle < LDG_MATH_PHYS_ACOUSTIC_RAMP_PERIOD_CUNT) { ramp = 0.5 * (1.0 - ldg_math_cos_get(LDG_MATH_PI * cycle / LDG_MATH_PHYS_ACOUSTIC_RAMP_PERIOD_CUNT)); }

                sample = info->mono_amplitude[src] * ramp* ldg_math_sin_get(LDG_MATH_TAU *cycle);
            }

            slot->series_span[src * LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT + batch_tick] = sample;
        }
    }
    return ldg_gpu_buff_wr(bridge->dev, slot->batch_blk.id, slot->series_span, LDG_MATH_PHYS_EM_DRIVE_CAP * LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT * sizeof(double), LDG_MATH_PHYS_COMPUTE_SERIES_OFFSET);
}

LDG_NOINLINE static uint32_t ldg_math_phys_acoustic_batch_build(ldg_math_phys_compute_bridge_t *bridge, ldg_math_phys_compute_slot_t *slot, uint64_t span, uint8_t final_batch)
{
    uint64_t batch_tick = 0;
    uint32_t dispatch_cunt = 0;
    uint32_t deposit_group = (uint32_t)((slot->footprint_cunt + 63) / 64);
    ldg_mem_zero(&bridge->desc[dispatch_cunt], sizeof(ldg_gpu_dispatch_desc_t));
    bridge->desc[dispatch_cunt].pipeline_id = bridge->pipeline_prime;
    bridge->desc[dispatch_cunt].group_cunt_x = 1;
    bridge->desc[dispatch_cunt].group_cunt_y = 1;
    bridge->desc[dispatch_cunt].group_cunt_z = 1;
    bridge->desc[dispatch_cunt].buff_ids[0] = slot->batch_blk.id;
    bridge->desc[dispatch_cunt].buff_cunt = 1;
    dispatch_cunt++;
    ldg_mem_zero(&bridge->desc[dispatch_cunt], sizeof(ldg_gpu_dispatch_desc_t));
    bridge->desc[dispatch_cunt].pipeline_id = bridge->pipeline_acoustic_drive_clear;
    bridge->desc[dispatch_cunt].group_cunt_x = slot->cell_group;
    bridge->desc[dispatch_cunt].group_cunt_y = 1;
    bridge->desc[dispatch_cunt].group_cunt_z = 1;
    bridge->desc[dispatch_cunt].buff_ids[0] = slot->drive.id;
    bridge->desc[dispatch_cunt].buff_ids[1] = slot->param.id;
    bridge->desc[dispatch_cunt].buff_cunt = 2;
    dispatch_cunt++;
    for (batch_tick = 0; batch_tick < span; batch_tick++)
    {
        if (slot->footprint_cunt > 0)
        {
            ldg_mem_zero(&bridge->desc[dispatch_cunt], sizeof(ldg_gpu_dispatch_desc_t));
            bridge->desc[dispatch_cunt].pipeline_id = bridge->pipeline_acoustic_deposit;
            bridge->desc[dispatch_cunt].group_cunt_x = deposit_group;
            bridge->desc[dispatch_cunt].group_cunt_y = 1;
            bridge->desc[dispatch_cunt].group_cunt_z = 1;
            bridge->desc[dispatch_cunt].buff_ids[0] = slot->batch_blk.id;
            bridge->desc[dispatch_cunt].buff_ids[1] = slot->drive.id;
            bridge->desc[dispatch_cunt].buff_ids[2] = slot->footprint_cell.id;
            bridge->desc[dispatch_cunt].buff_ids[3] = slot->footprint_weight.id;
            bridge->desc[dispatch_cunt].buff_ids[4] = slot->footprint_param.id;
            bridge->desc[dispatch_cunt].buff_cunt = 5;
            dispatch_cunt++;
        }

        ldg_mem_zero(&bridge->desc[dispatch_cunt], sizeof(ldg_gpu_dispatch_desc_t));
        bridge->desc[dispatch_cunt].group_cunt_x = slot->group_x;
        bridge->desc[dispatch_cunt].group_cunt_y = slot->group_y;
        bridge->desc[dispatch_cunt].group_cunt_z = slot->group_z;
        if (slot->open)
        {
            bridge->desc[dispatch_cunt].pipeline_id = bridge->pipeline_acoustic_velocity;
            bridge->desc[dispatch_cunt].buff_ids[0] = slot->velocity.id;
            bridge->desc[dispatch_cunt].buff_ids[1] = slot->pressure.id;
            bridge->desc[dispatch_cunt].buff_ids[2] = slot->psi.id;
            bridge->desc[dispatch_cunt].buff_ids[3] = slot->mask.id;
            bridge->desc[dispatch_cunt].buff_ids[4] = slot->cell_coeff.id;
            bridge->desc[dispatch_cunt].buff_ids[5] = slot->stretch.id;
            bridge->desc[dispatch_cunt].buff_ids[6] = slot->param.id;
            bridge->desc[dispatch_cunt].buff_ids[7] = slot->batch_blk.id;
            bridge->desc[dispatch_cunt].buff_ids[8] = slot->tap_param.id;
            bridge->desc[dispatch_cunt].buff_cunt = 9;
        }
        else
        {
            bridge->desc[dispatch_cunt].pipeline_id = bridge->pipeline_acoustic_velocity_rigid;
            bridge->desc[dispatch_cunt].buff_ids[0] = slot->velocity.id;
            bridge->desc[dispatch_cunt].buff_ids[1] = slot->pressure.id;
            bridge->desc[dispatch_cunt].buff_ids[2] = slot->mask.id;
            bridge->desc[dispatch_cunt].buff_ids[3] = slot->cell_coeff.id;
            bridge->desc[dispatch_cunt].buff_ids[4] = slot->param.id;
            bridge->desc[dispatch_cunt].buff_ids[5] = slot->batch_blk.id;
            bridge->desc[dispatch_cunt].buff_ids[6] = slot->tap_param.id;
            bridge->desc[dispatch_cunt].buff_cunt = 7;
        }

        dispatch_cunt++;
        ldg_mem_zero(&bridge->desc[dispatch_cunt], sizeof(ldg_gpu_dispatch_desc_t));
        bridge->desc[dispatch_cunt].group_cunt_x = slot->group_x;
        bridge->desc[dispatch_cunt].group_cunt_y = slot->group_y;
        bridge->desc[dispatch_cunt].group_cunt_z = slot->group_z;
        if (slot->open)
        {
            bridge->desc[dispatch_cunt].pipeline_id = bridge->pipeline_acoustic_pressure;
            bridge->desc[dispatch_cunt].buff_ids[0] = slot->velocity.id;
            bridge->desc[dispatch_cunt].buff_ids[1] = slot->pressure.id;
            bridge->desc[dispatch_cunt].buff_ids[2] = slot->psi.id;
            bridge->desc[dispatch_cunt].buff_ids[3] = slot->mask.id;
            bridge->desc[dispatch_cunt].buff_ids[4] = slot->cell_coeff.id;
            bridge->desc[dispatch_cunt].buff_ids[5] = slot->stretch.id;
            bridge->desc[dispatch_cunt].buff_ids[6] = slot->param.id;
            bridge->desc[dispatch_cunt].buff_ids[7] = slot->drive.id;
            bridge->desc[dispatch_cunt].buff_cunt = 8;
        }
        else
        {
            bridge->desc[dispatch_cunt].pipeline_id = bridge->pipeline_acoustic_pressure_rigid;
            bridge->desc[dispatch_cunt].buff_ids[0] = slot->velocity.id;
            bridge->desc[dispatch_cunt].buff_ids[1] = slot->pressure.id;
            bridge->desc[dispatch_cunt].buff_ids[2] = slot->mask.id;
            bridge->desc[dispatch_cunt].buff_ids[3] = slot->cell_coeff.id;
            bridge->desc[dispatch_cunt].buff_ids[4] = slot->param.id;
            bridge->desc[dispatch_cunt].buff_ids[5] = slot->drive.id;
            bridge->desc[dispatch_cunt].buff_cunt = 6;
        }

        dispatch_cunt++;
    }
    ldg_mem_zero(&bridge->desc[dispatch_cunt], sizeof(ldg_gpu_dispatch_desc_t));
    bridge->desc[dispatch_cunt].pipeline_id = bridge->pipeline_acoustic_tap;
    bridge->desc[dispatch_cunt].group_cunt_x = 1;
    bridge->desc[dispatch_cunt].group_cunt_y = 1;
    bridge->desc[dispatch_cunt].group_cunt_z = 1;
    bridge->desc[dispatch_cunt].buff_ids[0] = slot->pressure.id;
    bridge->desc[dispatch_cunt].buff_ids[1] = slot->batch_blk.id;
    bridge->desc[dispatch_cunt].buff_ids[2] = slot->tap_param.id;
    bridge->desc[dispatch_cunt].buff_cunt = 3;
    dispatch_cunt++;
    if (slot->pack_armed && final_batch)
    {
        if (slot->vol_stride > 0)
        {
            ldg_mem_zero(&bridge->desc[dispatch_cunt], sizeof(ldg_gpu_dispatch_desc_t));
            bridge->desc[dispatch_cunt].pipeline_id = bridge->pipeline_acoustic_vol_pack;
            bridge->desc[dispatch_cunt].group_cunt_x = (uint32_t)((slot->vol_dim[0] + 3) / 4);
            bridge->desc[dispatch_cunt].group_cunt_y = (uint32_t)((slot->vol_dim[1] + 3) / 4);
            bridge->desc[dispatch_cunt].group_cunt_z = (uint32_t)((slot->vol_dim[2] + 3) / 4);
            bridge->desc[dispatch_cunt].buff_ids[0] = slot->pressure.id;
            bridge->desc[dispatch_cunt].buff_ids[1] = slot->velocity.id;
            bridge->desc[dispatch_cunt].buff_ids[2] = slot->pack.id;
            bridge->desc[dispatch_cunt].buff_ids[3] = slot->pack_param.id;
            bridge->desc[dispatch_cunt].buff_cunt = 4;
            dispatch_cunt++;
        }

        ldg_mem_zero(&bridge->desc[dispatch_cunt], sizeof(ldg_gpu_dispatch_desc_t));
        bridge->desc[dispatch_cunt].pipeline_id = bridge->pipeline_acoustic_slice_pack;
        bridge->desc[dispatch_cunt].group_cunt_x = slot->group_x;
        bridge->desc[dispatch_cunt].group_cunt_y = slot->group_y;
        bridge->desc[dispatch_cunt].group_cunt_z = 1;
        bridge->desc[dispatch_cunt].buff_ids[0] = slot->pressure.id;
        bridge->desc[dispatch_cunt].buff_ids[1] = slot->velocity.id;
        bridge->desc[dispatch_cunt].buff_ids[2] = slot->pack.id;
        bridge->desc[dispatch_cunt].buff_ids[3] = slot->pack_param.id;
        bridge->desc[dispatch_cunt].buff_cunt = 4;
        dispatch_cunt++;
    }

    return dispatch_cunt;
}

static uint32_t ldg_math_phys_acoustic_burst_advance(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt, double *out_tap)
{
    ldg_math_phys_acoustic_compute_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    uint64_t tick_base = 0;
    uint64_t done = 0;
    uint64_t batch_span = 0;
    uint64_t batch_tick = 0;
    uint64_t probe_idx = 0;
    uint32_t dispatch_cunt = 0;
    uint32_t err = 0;
    err = ldg_math_phys_acoustic_compute_info_get(ctx, region, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(!slot->armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(slot->in_flight)) { return LDG_ERR_BUSY; }

    err = ldg_math_phys_acoustic_tap_param_wr(bridge, slot, &info, probe_pos, probe_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_acoustic_footprint_upload(ctx, bridge, slot, &info, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    tick_base = info.tick_cuntr + slot->pending_tick;
    while (done < tick_cunt)
    {
        batch_span = tick_cunt - done;
        if (batch_span > LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT) { batch_span = LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT; }

        err = ldg_math_phys_acoustic_series_stage(bridge, slot, &info, tick_base + done, batch_span);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        dispatch_cunt = ldg_math_phys_acoustic_batch_build(bridge, slot, batch_span, done + batch_span >= tick_cunt);
        err = ldg_gpu_batch_dispatch(bridge->dev, bridge->desc, dispatch_cunt);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (probe_cunt > 0)
        {
            err = ldg_gpu_buff_rd(bridge->dev, slot->batch_blk.id, slot->tap_span, batch_span * LDG_MATH_PHYS_EM_PROBE_CAP * sizeof(double), LDG_MATH_PHYS_COMPUTE_TAP_OFFSET);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            if (LDG_UNLIKELY(ldg_math_nan_is(slot->tap_span[0]))) { return LDG_ERR_MATH_PHYS_DOMAIN; }

            if (out_tap != 0x0) { for (batch_tick = 0; batch_tick < batch_span; batch_tick++) { for (probe_idx = 0; probe_idx < probe_cunt; probe_idx++) { out_tap[probe_idx * tick_cunt + done + batch_tick] = slot->tap_span[batch_tick * LDG_MATH_PHYS_EM_PROBE_CAP + probe_idx]; } } }
        }

        done += batch_span;
    }
    slot->pending_tick += tick_cunt;
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_acoustic_burst_begin(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt)
{
    ldg_math_phys_acoustic_compute_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    uint64_t batch_span = 0;
    uint32_t dispatch_cunt = 0;
    uint32_t err = 0;
    err = ldg_math_phys_acoustic_compute_info_get(ctx, region, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(!slot->armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(slot->in_flight)) { return LDG_ERR_BUSY; }

    err = ldg_math_phys_acoustic_tap_param_wr(bridge, slot, &info, probe_pos, probe_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_acoustic_footprint_upload(ctx, bridge, slot, &info, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    slot->burst_total = tick_cunt;
    slot->burst_done = 0;
    slot->probe_cunt = probe_cunt;
    batch_span = tick_cunt;
    if (batch_span > LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT) { batch_span = LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT; }

    err = ldg_math_phys_acoustic_series_stage(bridge, slot, &info, info.tick_cuntr + slot->pending_tick, batch_span);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    dispatch_cunt = ldg_math_phys_acoustic_batch_build(bridge, slot, batch_span, batch_span >= slot->burst_total);
    err = ldg_gpu_batch_async_dispatch(bridge->dev, bridge->desc, dispatch_cunt, &slot->fence);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    slot->burst_span = batch_span;
    slot->in_flight = 1;
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_acoustic_burst_await(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_tap)
{
    ldg_math_phys_acoustic_compute_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    uint64_t batch_span = 0;
    uint64_t batch_tick = 0;
    uint64_t probe_idx = 0;
    uint32_t dispatch_cunt = 0;
    uint32_t err = 0;
    uint32_t ret = 0;
    err = ldg_math_phys_acoustic_compute_info_get(ctx, region, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(!slot->in_flight)) { return LDG_ERR_NOT_INIT; }

    err = ldg_gpu_fence_wait(bridge->dev, &slot->fence, UINT64_MAX);
    ret = ldg_gpu_fence_destroy(bridge->dev, &slot->fence);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK || ret != LDG_ERR_AOK))
    {
        slot->in_flight = 0;
        return (err != LDG_ERR_AOK) ? err : ret;
    }

    if (slot->probe_cunt > 0)
    {
        err = ldg_gpu_buff_rd(bridge->dev, slot->batch_blk.id, slot->tap_span, slot->burst_span * LDG_MATH_PHYS_EM_PROBE_CAP * sizeof(double), LDG_MATH_PHYS_COMPUTE_TAP_OFFSET);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            slot->in_flight = 0;
            return err;
        }

        if (LDG_UNLIKELY(ldg_math_nan_is(slot->tap_span[0])))
        {
            slot->in_flight = 0;
            return LDG_ERR_MATH_PHYS_DOMAIN;
        }

        if (out_tap != 0x0) { for (batch_tick = 0; batch_tick < slot->burst_span; batch_tick++) { for (probe_idx = 0; probe_idx < slot->probe_cunt; probe_idx++) { out_tap[probe_idx * slot->burst_total + slot->burst_done + batch_tick] = slot->tap_span[batch_tick * LDG_MATH_PHYS_EM_PROBE_CAP + probe_idx]; } } }
    }

    slot->burst_done += slot->burst_span;
    slot->pending_tick += slot->burst_span;
    if (slot->burst_done >= slot->burst_total)
    {
        slot->in_flight = 0;
        return LDG_ERR_AOK;
    }

    batch_span = slot->burst_total - slot->burst_done;
    if (batch_span > LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT) { batch_span = LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT; }

    err = ldg_math_phys_acoustic_series_stage(bridge, slot, &info, info.tick_cuntr + slot->pending_tick, batch_span);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        slot->in_flight = 0;
        return err;
    }

    dispatch_cunt = ldg_math_phys_acoustic_batch_build(bridge, slot, batch_span, slot->burst_done + batch_span >= slot->burst_total);
    err = ldg_gpu_batch_async_dispatch(bridge->dev, bridge->desc, dispatch_cunt, &slot->fence);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        slot->in_flight = 0;
        return err;
    }

    slot->burst_span = batch_span;
    return LDG_ERR_AGAIN;
}

uint32_t ldg_math_phys_compute_vk_burst_advance(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt, double *out_tap)
{
    ldg_math_phys_compute_region_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    uint64_t tick_base = 0;
    uint64_t done = 0;
    uint64_t batch_span = 0;
    uint64_t batch_tick = 0;
    uint64_t probe_idx = 0;
    uint32_t dispatch_cunt = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(tick_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ldg_math_phys_compute_region_kind_get(ctx, region) == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC) { return ldg_math_phys_acoustic_burst_advance(ctx, region, tick_cunt, probe_pos, probe_cunt, out_tap); }

    err = ldg_math_phys_compute_region_info_get(ctx, region, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(!slot->armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(slot->in_flight)) { return LDG_ERR_BUSY; }

    err = ldg_math_phys_compute_tap_param_wr(bridge, slot, &info, probe_pos, probe_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    tick_base = info.tick_cuntr + slot->pending_tick;
    while (done < tick_cunt)
    {
        batch_span = tick_cunt - done;
        if (batch_span > LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT) { batch_span = LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT; }

        err = ldg_math_phys_compute_series_stage(bridge, slot, &info, tick_base + done, batch_span);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        dispatch_cunt = ldg_math_phys_compute_batch_build(bridge, slot, batch_span, done + batch_span >= tick_cunt);
        err = ldg_gpu_batch_dispatch(bridge->dev, bridge->desc, dispatch_cunt);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (probe_cunt > 0)
        {
            err = ldg_gpu_buff_rd(bridge->dev, slot->batch_blk.id, slot->tap_span, batch_span * LDG_MATH_PHYS_EM_PROBE_CAP * sizeof(double), LDG_MATH_PHYS_COMPUTE_TAP_OFFSET);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            if (LDG_UNLIKELY(ldg_math_nan_is(slot->tap_span[0]))) { return LDG_ERR_MATH_PHYS_DOMAIN; }

            if (out_tap != 0x0) { for (batch_tick = 0; batch_tick < batch_span; batch_tick++) { for (probe_idx = 0; probe_idx < probe_cunt; probe_idx++) { out_tap[probe_idx * tick_cunt + done + batch_tick] = slot->tap_span[batch_tick * LDG_MATH_PHYS_EM_PROBE_CAP + probe_idx]; } } }
        }

        if (slot->port_pm_cunt > 0)
        {
            err = ldg_gpu_buff_rd(bridge->dev, slot->batch_blk.id, slot->port_span, batch_span * LDG_MATH_PHYS_COMPUTE_PORT_SLOT_CUNT * 2 * sizeof(double), LDG_MATH_PHYS_COMPUTE_PORT_TAP_OFFSET);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_math_phys_em_port_batch_push(ctx, region, slot->port_span, batch_span);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
        }

        done += batch_span;
    }
    slot->pending_tick += tick_cunt;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_vk_burst_begin(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt)
{
    ldg_math_phys_compute_region_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    uint64_t batch_span = 0;
    uint32_t dispatch_cunt = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(tick_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ldg_math_phys_compute_region_kind_get(ctx, region) == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC) { return ldg_math_phys_acoustic_burst_begin(ctx, region, tick_cunt, probe_pos, probe_cunt); }

    err = ldg_math_phys_compute_region_info_get(ctx, region, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(!slot->armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(slot->in_flight)) { return LDG_ERR_BUSY; }

    err = ldg_math_phys_compute_tap_param_wr(bridge, slot, &info, probe_pos, probe_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    slot->burst_total = tick_cunt;
    slot->burst_done = 0;
    slot->probe_cunt = probe_cunt;
    batch_span = tick_cunt;
    if (batch_span > LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT) { batch_span = LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT; }

    err = ldg_math_phys_compute_series_stage(bridge, slot, &info, info.tick_cuntr + slot->pending_tick, batch_span);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    dispatch_cunt = ldg_math_phys_compute_batch_build(bridge, slot, batch_span, batch_span >= slot->burst_total);
    err = ldg_gpu_batch_async_dispatch(bridge->dev, bridge->desc, dispatch_cunt, &slot->fence);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    slot->burst_span = batch_span;
    slot->in_flight = 1;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_vk_burst_poll(ldg_math_phys_ctx_t *ctx, uint32_t region, uint8_t *out)
{
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    if (LDG_UNLIKELY(region >= ctx->conf.region_cap)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(!slot->in_flight)) { return LDG_ERR_NOT_INIT; }

    return ldg_gpu_fence_poll(bridge->dev, &slot->fence, out);
}

uint32_t ldg_math_phys_compute_vk_burst_await(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_tap)
{
    ldg_math_phys_compute_region_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    uint64_t batch_span = 0;
    uint64_t batch_tick = 0;
    uint64_t probe_idx = 0;
    uint32_t dispatch_cunt = 0;
    uint32_t err = 0;
    uint32_t ret = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (ldg_math_phys_compute_region_kind_get(ctx, region) == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC) { return ldg_math_phys_acoustic_burst_await(ctx, region, out_tap); }

    err = ldg_math_phys_compute_region_info_get(ctx, region, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(!slot->in_flight)) { return LDG_ERR_NOT_INIT; }

    err = ldg_gpu_fence_wait(bridge->dev, &slot->fence, UINT64_MAX);
    ret = ldg_gpu_fence_destroy(bridge->dev, &slot->fence);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK || ret != LDG_ERR_AOK))
    {
        slot->in_flight = 0;
        return (err != LDG_ERR_AOK) ? err : ret;
    }

    if (slot->probe_cunt > 0)
    {
        err = ldg_gpu_buff_rd(bridge->dev, slot->batch_blk.id, slot->tap_span, slot->burst_span * LDG_MATH_PHYS_EM_PROBE_CAP * sizeof(double), LDG_MATH_PHYS_COMPUTE_TAP_OFFSET);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            slot->in_flight = 0;
            return err;
        }

        if (LDG_UNLIKELY(ldg_math_nan_is(slot->tap_span[0])))
        {
            slot->in_flight = 0;
            return LDG_ERR_MATH_PHYS_DOMAIN;
        }

        if (out_tap != 0x0) { for (batch_tick = 0; batch_tick < slot->burst_span; batch_tick++) { for (probe_idx = 0; probe_idx < slot->probe_cunt; probe_idx++) { out_tap[probe_idx * slot->burst_total + slot->burst_done + batch_tick] = slot->tap_span[batch_tick * LDG_MATH_PHYS_EM_PROBE_CAP + probe_idx]; } } }
    }

    if (slot->port_pm_cunt > 0)
    {
        err = ldg_gpu_buff_rd(bridge->dev, slot->batch_blk.id, slot->port_span, slot->burst_span * LDG_MATH_PHYS_COMPUTE_PORT_SLOT_CUNT * 2 * sizeof(double), LDG_MATH_PHYS_COMPUTE_PORT_TAP_OFFSET);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            slot->in_flight = 0;
            return err;
        }

        err = ldg_math_phys_em_port_batch_push(ctx, region, slot->port_span, slot->burst_span);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            slot->in_flight = 0;
            return err;
        }
    }

    slot->burst_done += slot->burst_span;
    slot->pending_tick += slot->burst_span;
    if (slot->burst_done >= slot->burst_total)
    {
        slot->in_flight = 0;
        return LDG_ERR_AOK;
    }

    batch_span = slot->burst_total - slot->burst_done;
    if (batch_span > LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT) { batch_span = LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT; }

    err = ldg_math_phys_compute_series_stage(bridge, slot, &info, info.tick_cuntr + slot->pending_tick, batch_span);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        slot->in_flight = 0;
        return err;
    }

    dispatch_cunt = ldg_math_phys_compute_batch_build(bridge, slot, batch_span, slot->burst_done + batch_span >= slot->burst_total);
    err = ldg_gpu_batch_async_dispatch(bridge->dev, bridge->desc, dispatch_cunt, &slot->fence);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        slot->in_flight = 0;
        return err;
    }

    slot->burst_span = batch_span;
    return LDG_ERR_AGAIN;
}

static uint32_t ldg_math_phys_acoustic_sync(ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    ldg_math_phys_acoustic_compute_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    uint64_t pending = 0;
    uint32_t err = 0;
    err = ldg_math_phys_acoustic_compute_info_get(ctx, region, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(!slot->armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(slot->in_flight)) { return LDG_ERR_BUSY; }

    if (slot->pending_tick == 0) { return LDG_ERR_AOK; }

    err = ldg_gpu_buff_rd(bridge->dev, slot->pressure.id, info.pressure, info.cell_cunt * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_rd(bridge->dev, slot->velocity.id, info.velocity, 3 * info.cell_cunt * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(info.pressure[info.cell_cunt / 2]))) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    pending = slot->pending_tick;
    if (pending > UINT32_MAX) { pending = UINT32_MAX; }

    err = ldg_math_phys_acoustic_compute_commit(ctx, region, (uint32_t)pending);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    slot->pending_tick = 0;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_vk_sync(ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    ldg_math_phys_compute_region_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_compute_bridge_t *bridge = 0x0;
    ldg_math_phys_compute_slot_t *slot = 0x0;
    uint64_t pending = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->compute == 0x0)) { return LDG_ERR_NOT_INIT; }

    if (ldg_math_phys_compute_region_kind_get(ctx, region) == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC) { return ldg_math_phys_acoustic_sync(ctx, region); }

    err = ldg_math_phys_compute_region_info_get(ctx, region, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bridge = (ldg_math_phys_compute_bridge_t *)ctx->compute;
    slot = &bridge->slot[region];
    if (LDG_UNLIKELY(!slot->armed)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(slot->in_flight)) { return LDG_ERR_BUSY; }

    if (slot->pending_tick == 0) { return LDG_ERR_AOK; }

    err = ldg_gpu_buff_rd(bridge->dev, slot->electric.id, info.electric, 3 * info.cell_cunt * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_rd(bridge->dev, slot->magnetic.id, info.magnetic, 3 * info.cell_cunt * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(info.electric[info.cell_cunt / 2]) || !ldg_math_finite_is(info.magnetic[info.cell_cunt / 2]))) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    pending = slot->pending_tick;
    if (pending > UINT32_MAX) { pending = UINT32_MAX; }

    err = ldg_math_phys_compute_commit(ctx, region, (uint32_t)pending);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    slot->pending_tick = 0;
    return LDG_ERR_AOK;
}

extern const uint32_t ldg_math_phys_spirv_atmosphere_transmittance[];
extern const uint64_t ldg_math_phys_spirv_atmosphere_transmittance_size;
extern const uint32_t ldg_math_phys_spirv_atmosphere_single[];
extern const uint64_t ldg_math_phys_spirv_atmosphere_single_size;
extern const uint32_t ldg_math_phys_spirv_atmosphere_order[];
extern const uint64_t ldg_math_phys_spirv_atmosphere_order_size;
extern const uint32_t ldg_math_phys_spirv_atmosphere_gather[];
extern const uint64_t ldg_math_phys_spirv_atmosphere_gather_size;
extern const uint32_t ldg_math_phys_spirv_atmosphere_irradiance[];
extern const uint64_t ldg_math_phys_spirv_atmosphere_irradiance_size;
extern const uint32_t ldg_math_phys_spirv_atmosphere_cum[];
extern const uint64_t ldg_math_phys_spirv_atmosphere_cum_size;
extern const uint32_t ldg_math_phys_spirv_atmosphere_vol[];
extern const uint64_t ldg_math_phys_spirv_atmosphere_vol_size;

#define ATMOSPHERE_GPU_PIPE_CUNT 6
#define ATMOSPHERE_GPU_PIPE_TRANS 0
#define ATMOSPHERE_GPU_PIPE_SINGLE 1
#define ATMOSPHERE_GPU_PIPE_ORDER 2
#define ATMOSPHERE_GPU_PIPE_GATHER 3
#define ATMOSPHERE_GPU_PIPE_IRRAD 4
#define ATMOSPHERE_GPU_PIPE_CUM 5

#define ATMOSPHERE_GPU_BUFF_CUNT 17
#define ATMOSPHERE_GPU_B_OZONE 16
#define ATMOSPHERE_GPU_B_PARAM 0
#define ATMOSPHERE_GPU_B_CUM_PARAM 1
#define ATMOSPHERE_GPU_B_RAYLEIGH 2
#define ATMOSPHERE_GPU_B_SUN 3
#define ATMOSPHERE_GPU_B_ALBEDO 4
#define ATMOSPHERE_GPU_B_DIRS 5
#define ATMOSPHERE_GPU_B_TRANS 6
#define ATMOSPHERE_GPU_B_DR 7
#define ATMOSPHERE_GPU_B_DM 8
#define ATMOSPHERE_GPU_B_DF 9
#define ATMOSPHERE_GPU_B_DI 10
#define ATMOSPHERE_GPU_B_GATHER 11
#define ATMOSPHERE_GPU_B_MULTI 12
#define ATMOSPHERE_GPU_B_GROUND 13
#define ATMOSPHERE_GPU_B_PING 14
#define ATMOSPHERE_GPU_B_PONG 15

#define ATMOSPHERE_GPU_PARAM_SIZE 132
#define ATMOSPHERE_GPU_FLAG_OFFSET 128

#define ATMOSPHERE_VOL_GPU_BUFF_CUNT 9
#define ATMOSPHERE_VOL_GPU_B_PARAM 0
#define ATMOSPHERE_VOL_GPU_B_RAYLEIGH 1
#define ATMOSPHERE_VOL_GPU_B_DR 2
#define ATMOSPHERE_VOL_GPU_B_DM 3
#define ATMOSPHERE_VOL_GPU_B_DF 4
#define ATMOSPHERE_VOL_GPU_B_MULTI 5
#define ATMOSPHERE_VOL_GPU_B_OUT_TRANS 6
#define ATMOSPHERE_VOL_GPU_B_OUT_INSCATTER 7
#define ATMOSPHERE_VOL_GPU_B_OZONE 8
#define ATMOSPHERE_VOL_GPU_PARAM_SIZE 240
#define ATMOSPHERE_GPU_DIRS_ELEM 64
#define ATMOSPHERE_GPU_GATHER_POLAR 8
#define ATMOSPHERE_GPU_GATHER_AZIM 16
#define ATMOSPHERE_GPU_HEMI_POLAR 8

static uint32_t atmosphere_gpu_ceil_div(uint64_t num, uint64_t den)
{
    return (uint32_t)((num + den - 1) / den);
}

LDG_NOINLINE static uint32_t atmosphere_gpu_teardown(void *dev, ldg_gpu_buff_t *buff, uint32_t buff_made, uint32_t *pipe, uint32_t pipe_made)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t err = 0;
    uint32_t idx = 0;
    for (idx = 0; idx < buff_made; idx++)
    {
        err = ldg_gpu_buff_destroy(dev, buff[idx].id);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }
    for (idx = 0; idx < pipe_made; idx++)
    {
        err = ldg_gpu_pipeline_destroy(dev, pipe[idx]);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }
    return ret;
}

LDG_NOINLINE static uint32_t atmosphere_gpu_run(void *dev, uint32_t pipeline, uint32_t gx, uint32_t gy, uint32_t gz, const uint32_t *ids, uint32_t cunt)
{
    ldg_gpu_dispatch_desc_t desc = LDG_STRUCT_ZERO_INIT;
    uint32_t idx = 0;
    desc.pipeline_id = pipeline;
    desc.group_cunt_x = gx;
    desc.group_cunt_y = gy;
    desc.group_cunt_z = gz;
    for (idx = 0; idx < cunt; idx++) { desc.buff_ids[idx] = ids[idx]; }

    desc.buff_cunt = cunt;
    return ldg_gpu_dispatch(dev, &desc);
}

LDG_NOINLINE static uint32_t atmosphere_gpu_flag_set(void *dev, uint32_t param_id, uint32_t flag)
{
    return ldg_gpu_buff_wr(dev, param_id, &flag, sizeof(uint32_t), ATMOSPHERE_GPU_FLAG_OFFSET);
}

LDG_NOINLINE static uint32_t atmosphere_gpu_cum(void *dev, uint32_t *pipe, ldg_gpu_buff_t *buff, uint32_t dst_id, uint32_t src_id, uint64_t elem_cunt)
{
    uint32_t ids[3] = LDG_ARR_ZERO_INIT;
    uint32_t elem = (uint32_t)elem_cunt;
    uint32_t err = 0;
    err = ldg_gpu_buff_wr(dev, buff[ATMOSPHERE_GPU_B_CUM_PARAM].id, &elem, sizeof(uint32_t), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ids[0] = dst_id;
    ids[1] = src_id;
    ids[2] = buff[ATMOSPHERE_GPU_B_CUM_PARAM].id;
    return atmosphere_gpu_run(dev, pipe[ATMOSPHERE_GPU_PIPE_CUM], atmosphere_gpu_ceil_div(elem_cunt, 64), 1, 1, ids, 3);
}

LDG_NOINLINE static uint32_t atmosphere_gpu_param_upload(void *dev, uint32_t param_id, const ldg_math_phys_atmosphere_hdr_t *hdr)
{
    uint8_t raw[ATMOSPHERE_GPU_PARAM_SIZE] = LDG_ARR_ZERO_INIT;
    double dvals[13] = LDG_ARR_ZERO_INIT;
    uint32_t uvals[7] = LDG_ARR_ZERO_INIT;
    dvals[0] = hdr->planet_radius;
    dvals[1] = hdr->thickness;
    dvals[2] = hdr->rayleigh_scale_height;
    dvals[3] = hdr->mie_scale_height;
    dvals[4] = hdr->mie_scatter;
    dvals[5] = hdr->mie_absorb;
    dvals[6] = hdr->mie_anisotropy;
    dvals[7] = hdr->fog_scale_height;
    dvals[8] = hdr->fog_scatter;
    dvals[9] = hdr->fog_absorb;
    dvals[10] = hdr->fog_anisotropy;
    dvals[11] = hdr->ozone_center_altitude;
    dvals[12] = hdr->ozone_half_width;
    uvals[0] = (uint32_t)hdr->bin_cunt;
    uvals[1] = (uint32_t)hdr->order_cunt;
    uvals[2] = (uint32_t)hdr->altitude_cunt;
    uvals[3] = (uint32_t)hdr->view_cunt;
    uvals[4] = (uint32_t)hdr->sun_cunt;
    uvals[5] = (uint32_t)hdr->view_sun_cunt;
    uvals[6] = 0;
    ldg_mem_cpy(raw, dvals, 13 * sizeof(double));
    ldg_mem_cpy(raw + 13 * sizeof(double), uvals, 7 * sizeof(uint32_t));
    return ldg_gpu_buff_wr(dev, param_id, raw, ATMOSPHERE_GPU_PARAM_SIZE, 0);
}

LDG_NOINLINE static uint32_t atmosphere_gpu_dirs_upload(void *dev, uint32_t dirs_id)
{
    double dirs[ATMOSPHERE_GPU_DIRS_ELEM] = LDG_ARR_ZERO_INIT;
    double polar = 0.0;
    double azim = 0.0;
    double gather_step = LDG_MATH_PI / (double)ATMOSPHERE_GPU_GATHER_POLAR;
    double azim_step = LDG_MATH_TAU / (double)ATMOSPHERE_GPU_GATHER_AZIM;
    double hemi_step = (LDG_MATH_PI * 0.5) / (double)ATMOSPHERE_GPU_HEMI_POLAR;
    uint32_t idx = 0;
    for (idx = 0; idx < ATMOSPHERE_GPU_GATHER_POLAR; idx++)
    {
        polar = ((double)idx + 0.5) * gather_step;
        dirs[idx] = ldg_math_cos_get(polar);
        dirs[8 + idx] = ldg_math_sin_get(polar);
    }
    for (idx = 0; idx < ATMOSPHERE_GPU_GATHER_AZIM; idx++)
    {
        azim = ((double)idx + 0.5) * azim_step;
        dirs[16 + idx] = ldg_math_cos_get(azim);
        dirs[32 + idx] = ldg_math_sin_get(azim);
    }
    for (idx = 0; idx < ATMOSPHERE_GPU_HEMI_POLAR; idx++)
    {
        polar = ((double)idx + 0.5) * hemi_step;
        dirs[48 + idx] = ldg_math_cos_get(polar);
        dirs[56 + idx] = ldg_math_sin_get(polar);
    }
    return ldg_gpu_buff_wr(dev, dirs_id, dirs, ATMOSPHERE_GPU_DIRS_ELEM * sizeof(double), 0);
}

LDG_NOINLINE static uint32_t atmosphere_gpu_buffs_make(void *dev, ldg_gpu_buff_t *buff, const ldg_math_phys_atmosphere_compute_info_t *info, uint32_t *pipe, uint32_t pipe_made)
{
    ldg_gpu_buff_desc_t bdesc = LDG_STRUCT_ZERO_INIT;
    uint64_t bin = info->hdr->bin_cunt;
    uint64_t bsize[ATMOSPHERE_GPU_BUFF_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t err = 0;
    uint32_t idx = 0;
    bsize[ATMOSPHERE_GPU_B_PARAM] = ATMOSPHERE_GPU_PARAM_SIZE;
    bsize[ATMOSPHERE_GPU_B_CUM_PARAM] = 4 * sizeof(uint32_t);
    bsize[ATMOSPHERE_GPU_B_RAYLEIGH] = bin * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_SUN] = bin * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_ALBEDO] = bin * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_DIRS] = ATMOSPHERE_GPU_DIRS_ELEM * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_TRANS] = info->trans_total * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_DR] = info->grid_total * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_DM] = info->grid_total * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_DF] = info->grid_total * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_DI] = info->ground_total * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_GATHER] = info->grid_total * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_MULTI] = info->grid_total * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_GROUND] = info->ground_total * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_PING] = info->grid_total * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_PONG] = info->grid_total * sizeof(double);
    bsize[ATMOSPHERE_GPU_B_OZONE] = bin * sizeof(double);
    for (idx = 0; idx < ATMOSPHERE_GPU_BUFF_CUNT; idx++)
    {
        bdesc.mem_flags = LDG_GPU_MEM_DEV_LOCAL;
        bdesc.size = bsize[idx];
        err = ldg_gpu_buff_create(dev, &bdesc, &buff[idx]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            atmosphere_gpu_teardown(dev, buff, idx, pipe, pipe_made);
            return err;
        }
    }
    return LDG_ERR_AOK;
}

LDG_NOINLINE static uint32_t atmosphere_gpu_pipes_make(void *dev, uint32_t *pipe)
{
    ldg_gpu_spirv_desc_t spirv = LDG_STRUCT_ZERO_INIT;
    const uint32_t *code[ATMOSPHERE_GPU_PIPE_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t code_size[ATMOSPHERE_GPU_PIPE_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t err = 0;
    uint32_t idx = 0;
    code[ATMOSPHERE_GPU_PIPE_TRANS] = ldg_math_phys_spirv_atmosphere_transmittance;
    code[ATMOSPHERE_GPU_PIPE_SINGLE] = ldg_math_phys_spirv_atmosphere_single;
    code[ATMOSPHERE_GPU_PIPE_ORDER] = ldg_math_phys_spirv_atmosphere_order;
    code[ATMOSPHERE_GPU_PIPE_GATHER] = ldg_math_phys_spirv_atmosphere_gather;
    code[ATMOSPHERE_GPU_PIPE_IRRAD] = ldg_math_phys_spirv_atmosphere_irradiance;
    code[ATMOSPHERE_GPU_PIPE_CUM] = ldg_math_phys_spirv_atmosphere_cum;
    code_size[ATMOSPHERE_GPU_PIPE_TRANS] = ldg_math_phys_spirv_atmosphere_transmittance_size;
    code_size[ATMOSPHERE_GPU_PIPE_SINGLE] = ldg_math_phys_spirv_atmosphere_single_size;
    code_size[ATMOSPHERE_GPU_PIPE_ORDER] = ldg_math_phys_spirv_atmosphere_order_size;
    code_size[ATMOSPHERE_GPU_PIPE_GATHER] = ldg_math_phys_spirv_atmosphere_gather_size;
    code_size[ATMOSPHERE_GPU_PIPE_IRRAD] = ldg_math_phys_spirv_atmosphere_irradiance_size;
    code_size[ATMOSPHERE_GPU_PIPE_CUM] = ldg_math_phys_spirv_atmosphere_cum_size;
    for (idx = 0; idx < ATMOSPHERE_GPU_PIPE_CUNT; idx++)
    {
        spirv.code = code[idx];
        spirv.code_size = code_size[idx];
        spirv.entry_name = "main";
        err = ldg_gpu_pipeline_create(dev, &spirv, &pipe[idx]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            atmosphere_gpu_teardown(dev, 0x0, 0, pipe, idx);
            return err;
        }
    }
    return LDG_ERR_AOK;
}

LDG_NOINLINE static uint32_t atmosphere_gpu_inputs_upload(void *dev, ldg_gpu_buff_t *buff, const ldg_math_phys_atmosphere_compute_info_t *info)
{
    uint64_t bin = info->hdr->bin_cunt;
    uint32_t err = 0;
    err = atmosphere_gpu_param_upload(dev, buff[ATMOSPHERE_GPU_B_PARAM].id, info->hdr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = atmosphere_gpu_dirs_upload(dev, buff[ATMOSPHERE_GPU_B_DIRS].id);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_wr(dev, buff[ATMOSPHERE_GPU_B_RAYLEIGH].id, info->rayleigh, bin * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_wr(dev, buff[ATMOSPHERE_GPU_B_OZONE].id, info->ozone, bin * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_wr(dev, buff[ATMOSPHERE_GPU_B_SUN].id, info->sun_irradiance, bin * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_wr(dev, buff[ATMOSPHERE_GPU_B_ALBEDO].id, info->albedo, bin * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_fill(dev, buff[ATMOSPHERE_GPU_B_MULTI].id, 0, info->grid_total * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    return ldg_gpu_buff_fill(dev, buff[ATMOSPHERE_GPU_B_GROUND].id, 0, info->ground_total * sizeof(double), 0);
}

LDG_NOINLINE static uint32_t atmosphere_gpu_readback(void *dev, ldg_gpu_buff_t *buff, const ldg_math_phys_atmosphere_compute_info_t *info)
{
    uint32_t err = 0;
    err = ldg_gpu_buff_rd(dev, buff[ATMOSPHERE_GPU_B_TRANS].id, info->transmittance, info->trans_total * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_rd(dev, buff[ATMOSPHERE_GPU_B_DR].id, info->delta_rayleigh, info->grid_total * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_rd(dev, buff[ATMOSPHERE_GPU_B_DM].id, info->delta_mie, info->grid_total * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_rd(dev, buff[ATMOSPHERE_GPU_B_DF].id, info->delta_fog, info->grid_total * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_gpu_buff_rd(dev, buff[ATMOSPHERE_GPU_B_MULTI].id, info->multi, info->grid_total * sizeof(double), 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    return ldg_gpu_buff_rd(dev, buff[ATMOSPHERE_GPU_B_GROUND].id, info->ground, info->ground_total * sizeof(double), 0);
}

LDG_NOINLINE static uint32_t atmosphere_gpu_chain(void *dev, uint32_t *pipe, ldg_gpu_buff_t *buff, const ldg_math_phys_atmosphere_compute_info_t *info)
{
    uint32_t ids[11] = LDG_ARR_ZERO_INIT;
    uint64_t alt = info->hdr->altitude_cunt;
    uint64_t view = info->hdr->view_cunt;
    uint64_t sv = info->hdr->sun_cunt * info->hdr->view_sun_cunt;
    uint32_t gx4 = atmosphere_gpu_ceil_div(alt, 4);
    uint32_t gy4 = atmosphere_gpu_ceil_div(view, 4);
    uint32_t gz4 = atmosphere_gpu_ceil_div(sv, 4);
    uint32_t gsun = atmosphere_gpu_ceil_div(info->hdr->sun_cunt, 64);
    uint32_t prev_id = UINT32_MAX;
    uint32_t cur_id = 0;
    uint32_t use_ping = 1;
    uint32_t prevbuf = 0;
    uint32_t gflag = 0;
    uint32_t iflag = 0;
    uint32_t err = 0;
    uint64_t order = 0;
    ids[0] = buff[ATMOSPHERE_GPU_B_PARAM].id;
    ids[1] = buff[ATMOSPHERE_GPU_B_RAYLEIGH].id;
    ids[2] = buff[ATMOSPHERE_GPU_B_TRANS].id;
    ids[3] = buff[ATMOSPHERE_GPU_B_OZONE].id;
    err = atmosphere_gpu_run(dev, pipe[ATMOSPHERE_GPU_PIPE_TRANS], atmosphere_gpu_ceil_div(alt, 8), atmosphere_gpu_ceil_div(view, 8), 1, ids, 4);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ids[0] = buff[ATMOSPHERE_GPU_B_PARAM].id;
    ids[1] = buff[ATMOSPHERE_GPU_B_RAYLEIGH].id;
    ids[2] = buff[ATMOSPHERE_GPU_B_SUN].id;
    ids[3] = buff[ATMOSPHERE_GPU_B_TRANS].id;
    ids[4] = buff[ATMOSPHERE_GPU_B_DR].id;
    ids[5] = buff[ATMOSPHERE_GPU_B_DM].id;
    ids[6] = buff[ATMOSPHERE_GPU_B_DF].id;
    ids[7] = buff[ATMOSPHERE_GPU_B_OZONE].id;
    err = atmosphere_gpu_run(dev, pipe[ATMOSPHERE_GPU_PIPE_SINGLE], gx4, gy4, gz4, ids, 8);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = atmosphere_gpu_flag_set(dev, buff[ATMOSPHERE_GPU_B_PARAM].id, 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ids[0] = buff[ATMOSPHERE_GPU_B_PARAM].id;
    ids[1] = buff[ATMOSPHERE_GPU_B_SUN].id;
    ids[2] = buff[ATMOSPHERE_GPU_B_TRANS].id;
    ids[3] = buff[ATMOSPHERE_GPU_B_DR].id;
    ids[4] = buff[ATMOSPHERE_GPU_B_DM].id;
    ids[5] = buff[ATMOSPHERE_GPU_B_DF].id;
    ids[6] = buff[ATMOSPHERE_GPU_B_DR].id;
    ids[7] = buff[ATMOSPHERE_GPU_B_DI].id;
    ids[8] = buff[ATMOSPHERE_GPU_B_DIRS].id;
    err = atmosphere_gpu_run(dev, pipe[ATMOSPHERE_GPU_PIPE_IRRAD], gsun, 1, 1, ids, 9);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (order = 2; order <= info->hdr->order_cunt; order++)
    {
        prevbuf = (prev_id == UINT32_MAX) ? buff[ATMOSPHERE_GPU_B_DR].id : prev_id;
        gflag = (prev_id == UINT32_MAX) ? 0 : 1;
        iflag = (prev_id == UINT32_MAX) ? 1 : 2;
        err = atmosphere_gpu_flag_set(dev, buff[ATMOSPHERE_GPU_B_PARAM].id, gflag);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        ids[0] = buff[ATMOSPHERE_GPU_B_PARAM].id;
        ids[1] = buff[ATMOSPHERE_GPU_B_RAYLEIGH].id;
        ids[2] = buff[ATMOSPHERE_GPU_B_ALBEDO].id;
        ids[3] = buff[ATMOSPHERE_GPU_B_TRANS].id;
        ids[4] = buff[ATMOSPHERE_GPU_B_DR].id;
        ids[5] = buff[ATMOSPHERE_GPU_B_DM].id;
        ids[6] = buff[ATMOSPHERE_GPU_B_DF].id;
        ids[7] = prevbuf;
        ids[8] = buff[ATMOSPHERE_GPU_B_DI].id;
        ids[9] = buff[ATMOSPHERE_GPU_B_GATHER].id;
        ids[10] = buff[ATMOSPHERE_GPU_B_DIRS].id;
        err = atmosphere_gpu_run(dev, pipe[ATMOSPHERE_GPU_PIPE_GATHER], gx4, gy4, gz4, ids, 11);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = atmosphere_gpu_flag_set(dev, buff[ATMOSPHERE_GPU_B_PARAM].id, iflag);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        ids[0] = buff[ATMOSPHERE_GPU_B_PARAM].id;
        ids[1] = buff[ATMOSPHERE_GPU_B_SUN].id;
        ids[2] = buff[ATMOSPHERE_GPU_B_TRANS].id;
        ids[3] = buff[ATMOSPHERE_GPU_B_DR].id;
        ids[4] = buff[ATMOSPHERE_GPU_B_DM].id;
        ids[5] = buff[ATMOSPHERE_GPU_B_DF].id;
        ids[6] = prevbuf;
        ids[7] = buff[ATMOSPHERE_GPU_B_DI].id;
        ids[8] = buff[ATMOSPHERE_GPU_B_DIRS].id;
        err = atmosphere_gpu_run(dev, pipe[ATMOSPHERE_GPU_PIPE_IRRAD], gsun, 1, 1, ids, 9);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = atmosphere_gpu_cum(dev, pipe, buff, buff[ATMOSPHERE_GPU_B_GROUND].id, buff[ATMOSPHERE_GPU_B_DI].id, info->ground_total);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        cur_id = (use_ping != 0) ? buff[ATMOSPHERE_GPU_B_PING].id : buff[ATMOSPHERE_GPU_B_PONG].id;
        ids[0] = buff[ATMOSPHERE_GPU_B_PARAM].id;
        ids[1] = buff[ATMOSPHERE_GPU_B_RAYLEIGH].id;
        ids[2] = buff[ATMOSPHERE_GPU_B_GATHER].id;
        ids[3] = cur_id;
        ids[4] = buff[ATMOSPHERE_GPU_B_OZONE].id;
        err = atmosphere_gpu_run(dev, pipe[ATMOSPHERE_GPU_PIPE_ORDER], gx4, gy4, gz4, ids, 5);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = atmosphere_gpu_cum(dev, pipe, buff, buff[ATMOSPHERE_GPU_B_MULTI].id, cur_id, info->grid_total);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        prev_id = cur_id;
        use_ping = (use_ping != 0) ? 0 : 1;
    }
    prevbuf = (prev_id == UINT32_MAX) ? buff[ATMOSPHERE_GPU_B_DR].id : prev_id;
    iflag = (prev_id == UINT32_MAX) ? 1 : 2;
    err = atmosphere_gpu_flag_set(dev, buff[ATMOSPHERE_GPU_B_PARAM].id, iflag);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ids[0] = buff[ATMOSPHERE_GPU_B_PARAM].id;
    ids[1] = buff[ATMOSPHERE_GPU_B_SUN].id;
    ids[2] = buff[ATMOSPHERE_GPU_B_TRANS].id;
    ids[3] = buff[ATMOSPHERE_GPU_B_DR].id;
    ids[4] = buff[ATMOSPHERE_GPU_B_DM].id;
    ids[5] = buff[ATMOSPHERE_GPU_B_DF].id;
    ids[6] = prevbuf;
    ids[7] = buff[ATMOSPHERE_GPU_B_DI].id;
    ids[8] = buff[ATMOSPHERE_GPU_B_DIRS].id;
    err = atmosphere_gpu_run(dev, pipe[ATMOSPHERE_GPU_PIPE_IRRAD], gsun, 1, 1, ids, 9);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    return atmosphere_gpu_cum(dev, pipe, buff, buff[ATMOSPHERE_GPU_B_GROUND].id, buff[ATMOSPHERE_GPU_B_DI].id, info->ground_total);
}

uint32_t ldg_math_phys_atmosphere_vk_solve(ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_compute_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_gpu_buff_t buff[ATMOSPHERE_GPU_BUFF_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t pipe[ATMOSPHERE_GPU_PIPE_CUNT] = LDG_ARR_ZERO_INIT;
    void *dev = 0x0;
    uint32_t err = 0;
    uint32_t chain_err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    dev = ldg_math_phys_compute_dev_get(ctx);
    if (dev == 0x0) { return LDG_ERR_UNSUPPORTED; }

    err = ldg_math_phys_atmosphere_compute_info_get(ctx, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(info.grid_total > UINT32_MAX)) { return LDG_ERR_UNSUPPORTED; }

    err = atmosphere_gpu_pipes_make(dev, pipe);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = atmosphere_gpu_buffs_make(dev, buff, &info, pipe, ATMOSPHERE_GPU_PIPE_CUNT);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = atmosphere_gpu_inputs_upload(dev, buff, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        atmosphere_gpu_teardown(dev, buff, ATMOSPHERE_GPU_BUFF_CUNT, pipe, ATMOSPHERE_GPU_PIPE_CUNT);
        return err;
    }

    chain_err = atmosphere_gpu_chain(dev, pipe, buff, &info);
    if (chain_err == LDG_ERR_AOK) { chain_err = atmosphere_gpu_readback(dev, buff, &info); }

    err = atmosphere_gpu_teardown(dev, buff, ATMOSPHERE_GPU_BUFF_CUNT, pipe, ATMOSPHERE_GPU_PIPE_CUNT);
    if (chain_err != LDG_ERR_AOK) { return chain_err; }

    return err;
}

LDG_NOINLINE static uint32_t atmosphere_gpu_vol_param_build(const ldg_math_phys_atmosphere_compute_info_t *info, const ldg_math_phys_atmosphere_vol_desc_t *desc, uint8_t *raw)
{
    double dvals[26] = LDG_ARR_ZERO_INIT;
    uint32_t uvals[8] = LDG_ARR_ZERO_INIT;
    double sun_norm = 0.0;
    double up_norm = 0.0;
    double cam_altitude = 0.0;
    double mu_s = 0.0;
    uint32_t axis = 0;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        sun_norm += desc->sun[axis] * desc->sun[axis];
        up_norm += desc->up[axis] * desc->up[axis];
    }
    if (LDG_UNLIKELY(sun_norm <= 0.0 || up_norm <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    sun_norm = ldg_math_sqrt_get(sun_norm);
    up_norm = ldg_math_sqrt_get(up_norm);
    cam_altitude = desc->ref_altitude;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        cam_altitude += desc->cam[axis] * (desc->up[axis] / up_norm);
        mu_s += (desc->sun[axis] / sun_norm) * (desc->up[axis] / up_norm);
    }
    if (cam_altitude < 0.0) { cam_altitude = 0.0; }

    if (cam_altitude > info->hdr->thickness) { cam_altitude = info->hdr->thickness; }

    if (mu_s > 1.0) { mu_s = 1.0; }

    if (mu_s < 0.0 - 1.0) { mu_s = 0.0 - 1.0; }

    dvals[0] = info->hdr->thickness;
    dvals[1] = info->hdr->mie_anisotropy;
    dvals[2] = info->hdr->fog_anisotropy;
    dvals[3] = desc->ref_altitude;
    dvals[4] = desc->spacing;
    dvals[5] = cam_altitude;
    dvals[6] = mu_s;
    dvals[7] = info->hdr->rayleigh_scale_height;
    dvals[8] = info->hdr->mie_scale_height;
    dvals[9] = info->hdr->fog_scale_height;
    dvals[10] = info->hdr->mie_scatter + info->hdr->mie_absorb;
    dvals[11] = info->hdr->fog_scatter + info->hdr->fog_absorb;
    dvals[12] = info->hdr->ozone_center_altitude;
    dvals[13] = info->hdr->ozone_half_width;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        dvals[14 + axis] = desc->orig[axis];
        dvals[17 + axis] = desc->cam[axis];
        dvals[20 + axis] = desc->sun[axis] / sun_norm;
        dvals[23 + axis] = desc->up[axis] / up_norm;
    }
    uvals[0] = (uint32_t)info->hdr->bin_cunt;
    uvals[1] = (uint32_t)info->hdr->altitude_cunt;
    uvals[2] = (uint32_t)info->hdr->view_cunt;
    uvals[3] = (uint32_t)info->hdr->sun_cunt;
    uvals[4] = (uint32_t)info->hdr->view_sun_cunt;
    uvals[5] = (uint32_t)desc->cunt_x;
    uvals[6] = (uint32_t)desc->cunt_y;
    uvals[7] = (uint32_t)desc->cunt_z;
    ldg_mem_cpy(raw, dvals, 26 * sizeof(double));
    ldg_mem_cpy(raw + 26 * sizeof(double), uvals, 8 * sizeof(uint32_t));
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_atmosphere_vol_vk_get(const ldg_math_phys_ctx_t *ctx, const ldg_math_phys_atmosphere_vol_desc_t *desc, double *out_transmittance, double *out_inscatter)
{
    ldg_math_phys_atmosphere_compute_info_t info = LDG_STRUCT_ZERO_INIT;
    ldg_gpu_buff_t buff[ATMOSPHERE_VOL_GPU_BUFF_CUNT] = LDG_ARR_ZERO_INIT;
    ldg_gpu_buff_desc_t bdesc = LDG_STRUCT_ZERO_INIT;
    ldg_gpu_spirv_desc_t spirv = LDG_STRUCT_ZERO_INIT;
    uint8_t raw[ATMOSPHERE_VOL_GPU_PARAM_SIZE] = LDG_ARR_ZERO_INIT;
    uint64_t bsize[ATMOSPHERE_VOL_GPU_BUFF_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t ids[ATMOSPHERE_VOL_GPU_BUFF_CUNT] = LDG_ARR_ZERO_INIT;
    void *dev = 0x0;
    uint64_t vol_total = 0;
    uint32_t pipe = 0;
    uint32_t err = 0;
    uint32_t run_err = 0;
    uint32_t idx = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_transmittance)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_inscatter)) { return LDG_ERR_FUNC_ARG_NULL; }

    dev = ldg_math_phys_compute_dev_get(ctx);
    if (dev == 0x0) { return LDG_ERR_UNSUPPORTED; }

    err = ldg_math_phys_atmosphere_compute_info_get(ctx, &info);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(desc->cunt_x, desc->cunt_y, &vol_total);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(vol_total, desc->cunt_z, &vol_total);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(vol_total, info.hdr->bin_cunt, &vol_total);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(vol_total > UINT32_MAX)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(info.grid_total > UINT32_MAX || info.trans_total > UINT32_MAX)) { return LDG_ERR_UNSUPPORTED; }

    err = atmosphere_gpu_vol_param_build(&info, desc, raw);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    spirv.code = ldg_math_phys_spirv_atmosphere_vol;
    spirv.code_size = ldg_math_phys_spirv_atmosphere_vol_size;
    spirv.entry_name = "main";
    err = ldg_gpu_pipeline_create(dev, &spirv, &pipe);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    bsize[ATMOSPHERE_VOL_GPU_B_PARAM] = ATMOSPHERE_VOL_GPU_PARAM_SIZE;
    bsize[ATMOSPHERE_VOL_GPU_B_RAYLEIGH] = info.hdr->bin_cunt * sizeof(double);
    bsize[ATMOSPHERE_VOL_GPU_B_DR] = info.grid_total * sizeof(double);
    bsize[ATMOSPHERE_VOL_GPU_B_DM] = info.grid_total * sizeof(double);
    bsize[ATMOSPHERE_VOL_GPU_B_DF] = info.grid_total * sizeof(double);
    bsize[ATMOSPHERE_VOL_GPU_B_MULTI] = info.grid_total * sizeof(double);
    bsize[ATMOSPHERE_VOL_GPU_B_OUT_TRANS] = vol_total * sizeof(double);
    bsize[ATMOSPHERE_VOL_GPU_B_OUT_INSCATTER] = vol_total * sizeof(double);
    bsize[ATMOSPHERE_VOL_GPU_B_OZONE] = info.hdr->bin_cunt * sizeof(double);
    for (idx = 0; idx < ATMOSPHERE_VOL_GPU_BUFF_CUNT; idx++)
    {
        bdesc.mem_flags = LDG_GPU_MEM_DEV_LOCAL;
        bdesc.size = bsize[idx];
        err = ldg_gpu_buff_create(dev, &bdesc, &buff[idx]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            atmosphere_gpu_teardown(dev, buff, idx, &pipe, 1);
            return err;
        }
    }
    run_err = ldg_gpu_buff_wr(dev, buff[ATMOSPHERE_VOL_GPU_B_PARAM].id, raw, ATMOSPHERE_VOL_GPU_PARAM_SIZE, 0);
    if (run_err == LDG_ERR_AOK) { run_err = ldg_gpu_buff_wr(dev, buff[ATMOSPHERE_VOL_GPU_B_RAYLEIGH].id, info.rayleigh, info.hdr->bin_cunt * sizeof(double), 0); }

    if (run_err == LDG_ERR_AOK) { run_err = ldg_gpu_buff_wr(dev, buff[ATMOSPHERE_VOL_GPU_B_OZONE].id, info.ozone, info.hdr->bin_cunt * sizeof(double), 0); }

    if (run_err == LDG_ERR_AOK) { run_err = ldg_gpu_buff_wr(dev, buff[ATMOSPHERE_VOL_GPU_B_DR].id, info.delta_rayleigh, info.grid_total * sizeof(double), 0); }

    if (run_err == LDG_ERR_AOK) { run_err = ldg_gpu_buff_wr(dev, buff[ATMOSPHERE_VOL_GPU_B_DM].id, info.delta_mie, info.grid_total * sizeof(double), 0); }

    if (run_err == LDG_ERR_AOK) { run_err = ldg_gpu_buff_wr(dev, buff[ATMOSPHERE_VOL_GPU_B_DF].id, info.delta_fog, info.grid_total * sizeof(double), 0); }

    if (run_err == LDG_ERR_AOK) { run_err = ldg_gpu_buff_wr(dev, buff[ATMOSPHERE_VOL_GPU_B_MULTI].id, info.multi, info.grid_total * sizeof(double), 0); }

    if (run_err == LDG_ERR_AOK)
    {
        for (idx = 0; idx < ATMOSPHERE_VOL_GPU_BUFF_CUNT; idx++) { ids[idx] = buff[idx].id; }
        run_err = atmosphere_gpu_run(dev, pipe, atmosphere_gpu_ceil_div(desc->cunt_x, 4), atmosphere_gpu_ceil_div(desc->cunt_y, 4), atmosphere_gpu_ceil_div(desc->cunt_z, 4), ids, ATMOSPHERE_VOL_GPU_BUFF_CUNT);
    }

    if (run_err == LDG_ERR_AOK) { run_err = ldg_gpu_buff_rd(dev, buff[ATMOSPHERE_VOL_GPU_B_OUT_TRANS].id, out_transmittance, vol_total * sizeof(double), 0); }

    if (run_err == LDG_ERR_AOK) { run_err = ldg_gpu_buff_rd(dev, buff[ATMOSPHERE_VOL_GPU_B_OUT_INSCATTER].id, out_inscatter, vol_total * sizeof(double), 0); }

    err = atmosphere_gpu_teardown(dev, buff, ATMOSPHERE_VOL_GPU_BUFF_CUNT, &pipe, 1);
    if (run_err != LDG_ERR_AOK) { return run_err; }

    return err;
}
