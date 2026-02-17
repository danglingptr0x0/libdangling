#ifndef LDG_GPU_GPU_H
#define LDG_GPU_GPU_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_GPU_NAME_MAX 256
#define LDG_GPU_PIPELINE_POOL_MAX 32
#define LDG_GPU_MEM_SLAB_MAX 16
#define LDG_GPU_BIND_MAX 8

#define LDG_GPU_FLAG_SPILL_ENABLE 0x01
#define LDG_GPU_FLAG_VALIDATION 0x02

#define LDG_GPU_MEM_DEVICE_LOCAL 0x01
#define LDG_GPU_MEM_HOST_VISIBLE 0x02

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
    uint8_t pudding[8];
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
LDG_EXPORT uint32_t ldg_gpu_init(const ldg_gpu_init_desc_t *desc);
LDG_EXPORT void ldg_gpu_shutdown(void);

LDG_EXPORT uint32_t ldg_gpu_buff_create(const ldg_gpu_buff_desc_t *desc, ldg_gpu_buff_t *out);
LDG_EXPORT uint32_t ldg_gpu_buff_destroy(uint32_t buff_id);
LDG_EXPORT uint32_t ldg_gpu_buff_wr(uint32_t id, const void *data, uint64_t size, uint64_t offset);
LDG_EXPORT uint32_t ldg_gpu_buff_rd(uint32_t id, void *data, uint64_t size, uint64_t offset);
LDG_EXPORT uint32_t ldg_gpu_buff_fill(uint32_t id, uint32_t val, uint64_t size, uint64_t offset);

LDG_EXPORT uint32_t ldg_gpu_pipeline_create(const ldg_gpu_spirv_desc_t *spirv, uint32_t *id);
LDG_EXPORT uint32_t ldg_gpu_pipeline_destroy(uint32_t id);

LDG_EXPORT uint32_t ldg_gpu_dispatch(const ldg_gpu_dispatch_desc_t *desc);
LDG_EXPORT uint32_t ldg_gpu_dispatch_async(const ldg_gpu_dispatch_desc_t *desc, ldg_gpu_fence_t *fence);
LDG_EXPORT uint32_t ldg_gpu_fence_wait(ldg_gpu_fence_t *fence, uint64_t timeout_ms);
LDG_EXPORT uint32_t ldg_gpu_fence_poll(ldg_gpu_fence_t *fence, uint8_t *ready);
LDG_EXPORT uint32_t ldg_gpu_fence_destroy(ldg_gpu_fence_t *fence);

LDG_EXPORT uint32_t ldg_gpu_mem_stats_get(ldg_gpu_mem_stats_t *stats);
LDG_EXPORT uint32_t ldg_gpu_spirv_file_load(const char *path, uint32_t **code, uint64_t *size);
LDG_EXPORT void ldg_gpu_spirv_file_free(uint32_t *code);

#endif
