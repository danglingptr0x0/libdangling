#ifndef LDG_MATH_PHYS_COMPUTE_H
#define LDG_MATH_PHYS_COMPUTE_H

#include <stdint.h>

#include <dangling/math/phys.h>

#include "field.h"

#define LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT 255
#define LDG_MATH_PHYS_COMPUTE_SERIES_OFFSET 8
#define LDG_MATH_PHYS_COMPUTE_TAP_OFFSET (8 + LDG_MATH_PHYS_EM_DRIVE_CAP * LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT * 8)
#define LDG_MATH_PHYS_COMPUTE_PORT_TAP_OFFSET (8 + (LDG_MATH_PHYS_EM_DRIVE_CAP + LDG_MATH_PHYS_EM_PROBE_CAP) * LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT * 8)
#define LDG_MATH_PHYS_COMPUTE_PORT_SLOT_CUNT (LDG_MATH_PHYS_EM_PORT_CAP * LDG_MATH_PHYS_EM_PORT_MODE_CAP)
#define LDG_MATH_PHYS_COMPUTE_BATCH_BLK_SIZE (LDG_MATH_PHYS_COMPUTE_PORT_TAP_OFFSET + LDG_MATH_PHYS_COMPUTE_PORT_SLOT_CUNT * 2 * LDG_MATH_PHYS_COMPUTE_BATCH_TICK_CUNT * 8)
#define LDG_MATH_PHYS_COMPUTE_TAP_PARAM_SIZE 416
#define LDG_MATH_PHYS_COMPUTE_PORT_PARAM_SIZE 400

typedef struct ldg_math_phys_em_drive
{
    ldg_math_phys_sig_t sig;
    uint64_t cell;
    uint64_t tick_start;
    uint32_t comp;
    uint8_t pudding[4];
} ldg_math_phys_em_drive_t;

typedef struct ldg_math_phys_compute_region_info
{
    double *electric;
    double *magnetic;
    const uint16_t *mtl;
    const double *coeff;
    const double *stretch;
    ldg_math_phys_em_drive_t *drive;
    double spacing;
    double dt_sub;
    uint64_t nx;
    uint64_t ny;
    uint64_t nz;
    uint64_t cell_cunt;
    uint64_t mtl_cunt;
    uint64_t pml_depth;
    uint64_t drive_cunt;
    uint64_t tick_cuntr;
    uint64_t subcycle_cunt;
    uint64_t face_cunt;
    uint8_t plane_armed;
    uint8_t pudding[7];
} ldg_math_phys_compute_region_info_t;

typedef struct ldg_math_phys_acoustic_compute_info
{
    double *pressure;
    double *velocity;
    double *drive;
    double *psi;
    const uint16_t *mtl;
    const double *coeff;
    const double *stretch;
    double spacing;
    double dt_sub;
    uint64_t nx;
    uint64_t ny;
    uint64_t nz;
    uint64_t cell_cunt;
    uint64_t mtl_cunt;
    uint64_t pml_depth;
    uint64_t tick_cuntr;
    uint64_t subcycle_cunt;
    uint64_t face_cunt;
    uint64_t wall_cunt;
    uint64_t mono_cunt;
    uint64_t mono_body[LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX];
    uint64_t mono_tick_start[LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX];
    uint64_t mono_series_cunt[LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX];
    const double *mono_series[LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX];
    double mono_amplitude[LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX];
    double mono_freq[LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX];
    uint8_t plane_armed;
    uint8_t pudding[7];
} ldg_math_phys_acoustic_compute_info_t;

typedef struct ldg_math_phys_compute_hdr
{
    void *dev;
} ldg_math_phys_compute_hdr_t;

void* ldg_math_phys_compute_dev_get(const ldg_math_phys_ctx_t *ctx);
uint32_t ldg_math_phys_compute_region_kind_get(const ldg_math_phys_ctx_t *ctx, uint32_t region);
uint8_t ldg_math_phys_region_step_skip_is(const ldg_math_phys_ctx_t *ctx, uint32_t region);
uint32_t ldg_math_phys_region_tick_advance(ldg_math_phys_ctx_t *ctx, uint32_t region, double dt, uint32_t frame_idx);
uint32_t ldg_math_phys_dt_tick_get(const ldg_math_phys_ctx_t *ctx, double *out);
uint32_t ldg_math_phys_region_probe_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, double *out);
uint32_t ldg_math_phys_em_port_capture(ldg_math_phys_ctx_t *ctx, uint32_t region);
uint32_t ldg_math_phys_em_port_gpu_pack(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t *out_word, double *out_val, const double **out_pattern, uint64_t *out_pattern_cunt, uint32_t *out_pm_cunt);
uint32_t ldg_math_phys_em_port_batch_push(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *span, uint64_t span_ticks);
uint32_t ldg_math_phys_atmosphere_compute_solve(ldg_math_phys_ctx_t *ctx);
uint32_t ldg_math_phys_atmosphere_vol_compute_get(const ldg_math_phys_ctx_t *ctx, const ldg_math_phys_atmosphere_vol_desc_t *desc, double *out_transmittance, double *out_inscatter);
uint32_t ldg_math_phys_atmosphere_cpu_solve(ldg_math_phys_ctx_t *ctx);
uint32_t ldg_math_phys_atmosphere_vol_cpu_get(const ldg_math_phys_ctx_t *ctx, const ldg_math_phys_atmosphere_vol_desc_t *desc, double *out_transmittance, double *out_inscatter);

uint32_t ldg_math_phys_compute_cpu_set(ldg_math_phys_ctx_t *ctx, void *compute);
uint32_t ldg_math_phys_compute_cpu_teardown(ldg_math_phys_ctx_t *ctx);
uint32_t ldg_math_phys_compute_cpu_region_arm(ldg_math_phys_ctx_t *ctx, uint32_t region);
uint32_t ldg_math_phys_compute_cpu_burst_advance(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt, double *out_tap);
uint32_t ldg_math_phys_compute_cpu_burst_begin(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt);
uint32_t ldg_math_phys_compute_cpu_burst_poll(ldg_math_phys_ctx_t *ctx, uint32_t region, uint8_t *out);
uint32_t ldg_math_phys_compute_cpu_burst_await(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_tap);
uint32_t ldg_math_phys_compute_cpu_sync(ldg_math_phys_ctx_t *ctx, uint32_t region);
uint32_t ldg_math_phys_compute_cpu_pack_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t vol_stride, uint32_t slice_z);
uint32_t ldg_math_phys_compute_cpu_pack_get(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_vol, double *out_slice);

#ifdef LDG_GPU_VULKAN
uint32_t ldg_math_phys_compute_vk_set(ldg_math_phys_ctx_t *ctx, void *compute);
uint32_t ldg_math_phys_compute_vk_teardown(ldg_math_phys_ctx_t *ctx);
uint32_t ldg_math_phys_compute_vk_clear(ldg_math_phys_ctx_t *ctx);
uint32_t ldg_math_phys_compute_vk_region_arm(ldg_math_phys_ctx_t *ctx, uint32_t region);
uint32_t ldg_math_phys_compute_vk_burst_advance(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt, double *out_tap);
uint32_t ldg_math_phys_compute_vk_burst_begin(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt);
uint32_t ldg_math_phys_compute_vk_burst_poll(ldg_math_phys_ctx_t *ctx, uint32_t region, uint8_t *out);
uint32_t ldg_math_phys_compute_vk_burst_await(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_tap);
uint32_t ldg_math_phys_compute_vk_sync(ldg_math_phys_ctx_t *ctx, uint32_t region);
uint32_t ldg_math_phys_compute_vk_pack_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t vol_stride, uint32_t slice_z);
uint32_t ldg_math_phys_compute_vk_pack_get(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_vol, double *out_slice);
uint32_t ldg_math_phys_atmosphere_vk_solve(ldg_math_phys_ctx_t *ctx);
uint32_t ldg_math_phys_atmosphere_vol_vk_get(const ldg_math_phys_ctx_t *ctx, const ldg_math_phys_atmosphere_vol_desc_t *desc, double *out_transmittance, double *out_inscatter);
#endif
uint32_t ldg_math_phys_compute_region_info_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, ldg_math_phys_compute_region_info_t *out);
uint32_t ldg_math_phys_acoustic_compute_info_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, ldg_math_phys_acoustic_compute_info_t *out);
uint32_t ldg_math_phys_acoustic_body_pos_get(const ldg_math_phys_ctx_t *ctx, uint64_t body, double *out_pos, uint32_t *out_region);
uint32_t ldg_math_phys_acoustic_compute_commit(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt);
uint32_t ldg_math_phys_compute_commit(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt);
uint32_t ldg_math_phys_compute_teardown(ldg_math_phys_ctx_t *ctx);

#endif
