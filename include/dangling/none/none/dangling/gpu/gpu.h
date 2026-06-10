#ifndef LDG_GPU_GPU_H
#define LDG_GPU_GPU_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_GPU_NAME_MAX 256
#define LDG_GPU_PIPELINE_POOL_MAX 64
#define LDG_GPU_MEM_SLAB_MAX 16
#define LDG_GPU_BIND_MAX 8

#define LDG_GPU_FLAG_SPILL_ENABLE 0x01
#define LDG_GPU_FLAG_VALIDATION 0x02

#define LDG_GPU_MEM_DEV_LOCAL 0x01
#define LDG_GPU_MEM_HOST_VISIBLE 0x02

#define LDG_GPU_SURFACE_MAX 4
#define LDG_GPU_SWAPCHAIN_MAX 4
#define LDG_GPU_RENDERPASS_MAX 16
#define LDG_GPU_VERT_ATTR_MAX 8
#define LDG_GPU_FRAME_IN_FLIGHT 3
#define LDG_GPU_SWAPCHAIN_IMG_MAX 4

#define LDG_GPU_PRESENT_FIFO 0
#define LDG_GPU_PRESENT_MAILBOX 1

#define LDG_GPU_TOPOLOGY_TRI_LIST 0
#define LDG_GPU_TOPOLOGY_TRI_STRIP 1
#define LDG_GPU_TOPOLOGY_LINE_LIST 2
#define LDG_GPU_TOPOLOGY_LINE_STRIP 3

#define LDG_GPU_CULL_NONE 0
#define LDG_GPU_CULL_BACK 1
#define LDG_GPU_CULL_FRONT 2

#define LDG_GPU_FMT_UNDEFINED 0
#define LDG_GPU_FMT_R8G8B8A8_UNORM 1
#define LDG_GPU_FMT_B8G8R8A8_UNORM 2
#define LDG_GPU_FMT_B8G8R8A8_SRGB 3
#define LDG_GPU_FMT_R32G32_SFLOAT 4
#define LDG_GPU_FMT_R32G32B32_SFLOAT 5
#define LDG_GPU_FMT_R32G32B32A32_SFLOAT 6
#define LDG_GPU_FMT_D24_UNORM_S8_UINT 7
#define LDG_GPU_FMT_D32_SFLOAT 8
#define LDG_GPU_FMT_D32_SFLOAT_S8_UINT 9

#define LDG_GPU_PIPELINE_COMPUTE 0
#define LDG_GPU_PIPELINE_GFX 1

typedef struct ldg_gpu_dev_info
{
    uint32_t dev_id;
    uint32_t vendor_id;
    uint32_t api_ver;
    uint32_t driver_ver;
    uint64_t vram_size;
    uint64_t host_visible_size;
    char name[LDG_GPU_NAME_MAX];
    uint8_t is_discrete;
    uint8_t pudding[7];
} ldg_gpu_dev_info_t;

typedef struct ldg_gpu_init_desc
{
    uint32_t dev_idx;
    uint32_t flags;
    uint32_t pipeline_pool_cunt;
    uint32_t slab_cunt;
    uint64_t slab_size;
    const char **instance_extensions;
    uint32_t instance_extension_cunt;
    uint8_t pudding[4];
} ldg_gpu_init_desc_t;

typedef struct ldg_gpu_buff_desc
{
    uint64_t size;
    uint32_t mem_flags;
    uint8_t pudding[4];
} ldg_gpu_buff_desc_t;

typedef struct ldg_gpu_buff
{
    uint32_t id;
    uint32_t mem_flags;
    uint64_t size;
    uint8_t spilled;
    uint8_t pudding[7];
} ldg_gpu_buff_t;

typedef struct ldg_gpu_spirv_desc
{
    const uint32_t *code;
    uint64_t code_size;
    const char *entry_name;
} ldg_gpu_spirv_desc_t;

typedef struct ldg_gpu_dispatch_desc
{
    uint32_t pipeline_id;
    uint32_t group_cunt_x;
    uint32_t group_cunt_y;
    uint32_t group_cunt_z;
    uint32_t buff_ids[LDG_GPU_BIND_MAX];
    uint32_t buff_cunt;
    uint8_t pudding[4];
} ldg_gpu_dispatch_desc_t;

typedef struct ldg_gpu_fence
{
    uint32_t id;
    uint8_t pudding[4];
} ldg_gpu_fence_t;

typedef struct ldg_gpu_mem_stats
{
    uint64_t vram_total;
    uint64_t vram_used;
    uint64_t host_total;
    uint64_t host_used;
    uint32_t slab_cunt;
    uint32_t spill_cunt;
    uint8_t pudding[24];
} ldg_gpu_mem_stats_t;

LDG_EXPORT uint32_t ldg_gpu_dev_list(ldg_gpu_dev_info_t **devs, uint32_t *cunt);
LDG_EXPORT void ldg_gpu_dev_free(ldg_gpu_dev_info_t *devs);
LDG_EXPORT uint32_t ldg_gpu_init(const ldg_gpu_init_desc_t *desc, void **out);
LDG_EXPORT void ldg_gpu_shutdown(void *vk);

LDG_EXPORT uint32_t ldg_gpu_buff_create(void *vk, const ldg_gpu_buff_desc_t *desc, ldg_gpu_buff_t *out);
LDG_EXPORT uint32_t ldg_gpu_buff_destroy(void *vk, uint32_t buff_id);
LDG_EXPORT uint32_t ldg_gpu_buff_wr(void *vk, uint32_t id, const void *data, uint64_t size, uint64_t offset);
LDG_EXPORT uint32_t ldg_gpu_buff_rd(void *vk, uint32_t id, void *data, uint64_t size, uint64_t offset);
LDG_EXPORT uint32_t ldg_gpu_buff_fill(void *vk, uint32_t id, uint32_t val, uint64_t size, uint64_t offset);

LDG_EXPORT uint32_t ldg_gpu_pipeline_create(void *vk, const ldg_gpu_spirv_desc_t *spirv, uint32_t *id);
LDG_EXPORT uint32_t ldg_gpu_pipeline_destroy(void *vk, uint32_t id);

LDG_EXPORT uint32_t ldg_gpu_dispatch(void *vk, const ldg_gpu_dispatch_desc_t *desc);
LDG_EXPORT uint32_t ldg_gpu_dispatch_async(void *vk, const ldg_gpu_dispatch_desc_t *desc, ldg_gpu_fence_t *fence);
LDG_EXPORT uint32_t ldg_gpu_fence_wait(void *vk, ldg_gpu_fence_t *fence, uint64_t timeout_ms);
LDG_EXPORT uint32_t ldg_gpu_fence_poll(void *vk, ldg_gpu_fence_t *fence, uint8_t *ready);
LDG_EXPORT uint32_t ldg_gpu_fence_destroy(void *vk, ldg_gpu_fence_t *fence);

LDG_EXPORT uint32_t ldg_gpu_mem_stats_get(void *vk, ldg_gpu_mem_stats_t *stats);
LDG_EXPORT uint32_t ldg_gpu_spirv_file_load(const char *path, uint32_t **code, uint64_t *size);
LDG_EXPORT void ldg_gpu_spirv_file_free(uint32_t *code);

typedef struct ldg_gpu_surface
{
    uint32_t id;
    uint8_t pudding[4];
} ldg_gpu_surface_t;

typedef struct ldg_gpu_swapchain_desc
{
    uint32_t surface_id;
    uint32_t w;
    uint32_t h;
    uint32_t preferred_img_cunt;
    uint32_t present_mode;
    uint32_t depth_fmt;
    uint8_t pudding[8];
} ldg_gpu_swapchain_desc_t;

typedef struct ldg_gpu_swapchain
{
    uint32_t id;
    uint32_t img_cunt;
    uint32_t w;
    uint32_t h;
} ldg_gpu_swapchain_t;

typedef struct ldg_gpu_renderpass_desc
{
    uint32_t color_fmt;
    uint32_t depth_fmt;
    uint8_t load_clear;
    uint8_t pudding[3];
} ldg_gpu_renderpass_desc_t;

typedef struct ldg_gpu_vert_attr
{
    uint32_t location;
    uint32_t offset;
    uint32_t fmt;
} ldg_gpu_vert_attr_t;

typedef struct ldg_gpu_gfx_pipeline_desc
{
    ldg_gpu_spirv_desc_t vert;
    ldg_gpu_spirv_desc_t frag;
    uint32_t renderpass_id;
    uint32_t vert_stride;
    ldg_gpu_vert_attr_t vert_attrs[LDG_GPU_VERT_ATTR_MAX];
    uint32_t vert_attr_cunt;
    uint8_t topology;
    uint8_t cull_mode;
    uint8_t depth_test;
    uint8_t depth_wr;
    uint8_t blend_enable;
    uint8_t pudding[3];
} ldg_gpu_gfx_pipeline_desc_t;

typedef struct ldg_gpu_frame
{
    uint32_t id;
    uint8_t pudding[4];
} ldg_gpu_frame_t;

LDG_EXPORT uint32_t ldg_gpu_instance_get(void *vk, void **out);

LDG_EXPORT uint32_t ldg_gpu_surface_create(void *vk, void *native_surface, ldg_gpu_surface_t *out);
LDG_EXPORT uint32_t ldg_gpu_surface_destroy(void *vk, uint32_t surface_id);

LDG_EXPORT uint32_t ldg_gpu_swapchain_create(void *vk, const ldg_gpu_swapchain_desc_t *desc, ldg_gpu_swapchain_t *out);
LDG_EXPORT uint32_t ldg_gpu_swapchain_destroy(void *vk, uint32_t swapchain_id);
LDG_EXPORT uint32_t ldg_gpu_swapchain_recreate(void *vk, uint32_t swapchain_id, uint32_t new_w, uint32_t new_h);
LDG_EXPORT uint32_t ldg_gpu_swapchain_img_acquire(void *vk, uint32_t swapchain_id, uint32_t *img_idx);
LDG_EXPORT uint32_t ldg_gpu_swapchain_present(void *vk, uint32_t swapchain_id, uint32_t img_idx);

LDG_EXPORT uint32_t ldg_gpu_renderpass_create(void *vk, const ldg_gpu_renderpass_desc_t *desc, uint32_t *id);
LDG_EXPORT uint32_t ldg_gpu_renderpass_destroy(void *vk, uint32_t id);

LDG_EXPORT uint32_t ldg_gpu_gfx_pipeline_create(void *vk, const ldg_gpu_gfx_pipeline_desc_t *desc, uint32_t *id);

LDG_EXPORT uint32_t ldg_gpu_frame_begin(void *vk, uint32_t swapchain_id, ldg_gpu_frame_t *frame);
LDG_EXPORT uint32_t ldg_gpu_frame_renderpass_begin(void *vk, ldg_gpu_frame_t *frame, uint32_t renderpass_id, double clear_color[4], double clear_depth);
LDG_EXPORT uint32_t ldg_gpu_frame_pipeline_bind(void *vk, ldg_gpu_frame_t *frame, uint32_t pipeline_id);
LDG_EXPORT uint32_t ldg_gpu_frame_vert_buff_bind(void *vk, ldg_gpu_frame_t *frame, uint32_t buff_id);
LDG_EXPORT uint32_t ldg_gpu_frame_idx_buff_bind(void *vk, ldg_gpu_frame_t *frame, uint32_t buff_id);
LDG_EXPORT uint32_t ldg_gpu_frame_buff_bind(void *vk, ldg_gpu_frame_t *frame, uint32_t slot, uint32_t buff_id);
LDG_EXPORT uint32_t ldg_gpu_frame_push_const(void *vk, ldg_gpu_frame_t *frame, uint32_t offset, uint32_t size, const void *data);
LDG_EXPORT uint32_t ldg_gpu_frame_draw(void *vk, ldg_gpu_frame_t *frame, uint32_t vert_cunt, uint32_t instance_cunt);
LDG_EXPORT uint32_t ldg_gpu_frame_draw_idxd(void *vk, ldg_gpu_frame_t *frame, uint32_t idx_cunt, uint32_t instance_cunt);
LDG_EXPORT uint32_t ldg_gpu_frame_renderpass_end(void *vk, ldg_gpu_frame_t *frame);
LDG_EXPORT uint32_t ldg_gpu_frame_end(void *vk, ldg_gpu_frame_t *frame);

#endif
