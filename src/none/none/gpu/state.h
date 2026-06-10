#ifndef LDG_GPU_STATE_H
#define LDG_GPU_STATE_H

#include <dangling/gpu/gpu.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/thread/sync.h>

#define LDG_GPU_DEV_ENUM_MAX 16
#define LDG_GPU_BUFF_MAX 256
#define LDG_GPU_FENCE_MAX 64
#define LDG_GPU_STAGING_SIZE (4 * LDG_MIB)
#define LDG_GPU_DEFAULT_SLAB_CUNT 4
#define LDG_GPU_DEFAULT_SLAB_SIZE (64 * LDG_MIB)
#define LDG_GPU_PUSH_CONST_SIZE 128

#define LDG_GPU_ALIGN_UP(x, a) (((uint64_t)(x) + ((uint64_t)(a) - 1)) & ~((uint64_t)(a) - 1))

#define LDG_GPU_UNLOCK_OR_WARN(ctx) \
        do { \
            if (LDG_UNLIKELY(ldg_mut_unlock(&(ctx)->mut) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("gpu mut unlock failed"); } \
        } while (0)

typedef struct ldg_gpu_slab
{
    void *mem;
    uint64_t size;
    uint64_t offset;
    uint32_t mem_type_idx;
    uint8_t is_dev_local;
    uint8_t is_host_visible;
    uint8_t in_use;
    uint8_t pudding[1];
} ldg_gpu_slab_t;

typedef struct ldg_gpu_buff_entry
{
    void *vk_buff;
    uint64_t size;
    uint64_t mem_offset;
    uint32_t slab_idx;
    uint32_t mem_flags;
    uint8_t in_use;
    uint8_t spilled;
    uint8_t is_host_visible;
    uint8_t pudding[5];
} ldg_gpu_buff_entry_t;

typedef struct ldg_gpu_pipeline_entry
{
    void *vert_module;
    void *frag_module;
    void *layout;
    void *pipeline;
    void *renderpass;
    uint8_t kind;
    uint8_t in_use;
    uint8_t pudding[6];
} ldg_gpu_pipeline_entry_t;

typedef struct ldg_gpu_fence_entry
{
    void *fence;
    void *cmd_buff;
    void *desc_set;
    uint8_t in_use;
    uint8_t pudding[7];
} ldg_gpu_fence_entry_t;

typedef struct ldg_gpu_surface_entry
{
    void *vk_surface;
    uint8_t in_use;
    uint8_t pudding[7];
} ldg_gpu_surface_entry_t;

typedef struct ldg_gpu_swapchain_img
{
    void *img_view;
    void *fbo;
} ldg_gpu_swapchain_img_t;

typedef struct ldg_gpu_frame_sync
{
    void *img_available_sem;
    void *render_finished_sem;
    void *in_flight_fence;
    void *cmd_buff;
} ldg_gpu_frame_sync_t;

typedef struct ldg_gpu_swapchain_entry
{
    void *vk_swapchain;
    void *depth_img;
    void *depth_img_view;
    void *depth_mem;
    uint32_t surface_id;
    uint32_t img_cunt;
    uint32_t w;
    uint32_t h;
    uint32_t color_fmt;
    uint32_t depth_fmt;
    uint32_t present_mode;
    uint32_t cached_renderpass_id;
    uint32_t current_frame_idx;
    uint32_t acquired_img_idx;
    ldg_gpu_swapchain_img_t imgs[LDG_GPU_SWAPCHAIN_IMG_MAX];
    ldg_gpu_frame_sync_t frame_sync[LDG_GPU_FRAME_IN_FLIGHT];
    uint8_t in_use;
    uint8_t pudding[7];
} ldg_gpu_swapchain_entry_t;

typedef struct ldg_gpu_renderpass_entry
{
    void *renderpass;
    uint32_t color_fmt;
    uint32_t depth_fmt;
    uint8_t load_clear;
    uint8_t in_use;
    uint8_t pudding[6];
} ldg_gpu_renderpass_entry_t;

typedef struct ldg_gpu_frame_entry
{
    void *cmd_buff;
    void *desc_set;
    uint32_t swapchain_id;
    uint32_t img_idx;
    uint32_t frame_sync_idx;
    uint8_t recording;
    uint8_t in_renderpass;
    uint8_t in_use;
    uint8_t pudding[5];
} ldg_gpu_frame_entry_t;

typedef struct ldg_gpu_ctx
{
    void *instance;
    void *phys_dev;
    void *dev;
    void *queue;
    void *cmd_pool;
    void *desc_pool;
    void *desc_set_layout;
    void *debug_messenger;
    void *staging_buff;
    void *staging_mem;
    void *staging_map;
    void *gfx_pipeline_layout;
    uint64_t staging_size;
    uint32_t dev_local_type_idx;
    uint32_t host_visible_type_idx;
    uint64_t min_storage_buff_offset_align;
    uint64_t vram_total;
    uint64_t vram_used;
    uint64_t host_total;
    uint64_t host_used;
    uint32_t spill_cunt;
    uint32_t compute_queue_family_idx;
    uint32_t gfx_queue_family_idx;
    uint32_t flags;
    uint32_t slab_cunt;
    uint8_t has_unified_mem;
    uint8_t has_gfx;
    uint8_t has_swapchain_ext;
    uint8_t mut_init;
    uint8_t pudding_ctx[4];
    ldg_gpu_slab_t slabs[LDG_GPU_MEM_SLAB_MAX];
    ldg_gpu_buff_entry_t buffs[LDG_GPU_BUFF_MAX];
    ldg_gpu_pipeline_entry_t pipelines[LDG_GPU_PIPELINE_POOL_MAX];
    ldg_gpu_fence_entry_t fences[LDG_GPU_FENCE_MAX];
    ldg_gpu_surface_entry_t surfaces[LDG_GPU_SURFACE_MAX];
    ldg_gpu_swapchain_entry_t swapchains[LDG_GPU_SWAPCHAIN_MAX];
    ldg_gpu_renderpass_entry_t renderpasses[LDG_GPU_RENDERPASS_MAX];
    ldg_gpu_frame_entry_t frames[LDG_GPU_FRAME_IN_FLIGHT];
    ldg_mut_t mut;
    volatile uint8_t is_init;
    uint8_t pudding[7];
} LDG_ALIGNED ldg_gpu_ctx_t;

uint32_t gpu_slab_alloc(ldg_gpu_ctx_t *ctx, uint8_t want_dev_local, uint64_t size, uint64_t alignment, uint32_t mem_type_bits, uint32_t *out_slab_idx, uint64_t *out_offset);
uint32_t gpu_spill_slab_create(ldg_gpu_ctx_t *ctx, uint64_t min_size, uint32_t mem_type_bits);
uint32_t gpu_cmd_begin_oneshot(ldg_gpu_ctx_t *ctx, void *out_cmd);
uint32_t gpu_cmd_submit_wait(ldg_gpu_ctx_t *ctx, void *cmd);
uint32_t gpu_staging_transfer(ldg_gpu_ctx_t *ctx, uint32_t buff_idx, void *host_data, uint64_t size, uint64_t buff_offset, uint8_t to_dev);

uint32_t gpu_fmt_to_vk(uint32_t fmt);
uint32_t gpu_vk_to_fmt(uint32_t vk_fmt);

#endif
