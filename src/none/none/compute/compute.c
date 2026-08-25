#include <dangling/compute/compute.h>
#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/secure.h>

#ifdef LDG_GPU_VULKAN
#include <dangling/gpu/gpu.h>
#endif

#define LDG_COMPUTE_PIPELINE_POOL_DEFAULT 32
#define LDG_COMPUTE_SLAB_CUNT_DEFAULT 8
#define LDG_COMPUTE_SLAB_SIZE_DEFAULT (512ULL * LDG_MIB)

typedef struct ldg_compute
{
    void *dev;
    uint8_t owned;
    uint8_t pudding[7];
} ldg_compute_t;

#ifdef LDG_GPU_VULKAN
static uint32_t ldg_compute_vk_dev_resolve(uint32_t dev_idx, uint32_t *out)
{
    ldg_gpu_dev_info_t *info = 0x0;
    uint32_t cunt = 0;
    uint32_t idx = 0;
    uint32_t err = 0;
    uint8_t match = UINT8_MAX;
    *out = UINT32_MAX;
    err = ldg_gpu_dev_list(&info, &cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (idx = 0; idx < cunt; idx++)
    {
        match = (dev_idx == LDG_COMPUTE_DEV_VK_APU) ? info[idx].is_integrated : info[idx].is_cpu;
        if (match == LDG_TRUTH_TRUE)
        {
            *out = info[idx].dev_id;
            break;
        }
    }
    ldg_gpu_dev_free(info);
    if (*out == UINT32_MAX) { return LDG_ERR_GPU_DEV_NOT_FOUND; }

    return LDG_ERR_AOK;
}

#endif

static uint32_t ldg_compute_dev_acquire(const ldg_compute_desc_t *desc, void **out)
{
#ifdef LDG_GPU_VULKAN
    ldg_gpu_init_desc_t init_desc = LDG_STRUCT_ZERO_INIT;
    uint32_t resolved = 0;
    uint32_t err = 0;
    if (desc->dev_sel == LDG_COMPUTE_DEV_VK_DISCRETE) { init_desc.dev_idx = UINT32_MAX; }
    else if (desc->dev_sel == LDG_COMPUTE_DEV_VK_APU || desc->dev_sel == LDG_COMPUTE_DEV_VK_CPU)
    {
        err = ldg_compute_vk_dev_resolve(desc->dev_sel, &resolved);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        init_desc.dev_idx = resolved;
    }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    init_desc.pipeline_pool_cunt = (desc->pipeline_pool_cunt == 0) ? LDG_COMPUTE_PIPELINE_POOL_DEFAULT : desc->pipeline_pool_cunt;
    init_desc.slab_cunt = (desc->slab_cunt == 0) ? LDG_COMPUTE_SLAB_CUNT_DEFAULT : desc->slab_cunt;
    init_desc.slab_size = (desc->slab_size == 0) ? LDG_COMPUTE_SLAB_SIZE_DEFAULT : desc->slab_size;
    return ldg_gpu_init(&init_desc, out);
#else
    if (desc == 0x0 || out == 0x0) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    return LDG_ERR_UNSUPPORTED;
#endif
}

static uint32_t ldg_compute_dev_release(void *dev)
{
#ifdef LDG_GPU_VULKAN
    ldg_gpu_shutdown(dev);
    return LDG_ERR_AOK;
#else
    if (dev == 0x0) { return LDG_ERR_AOK; }

    return LDG_ERR_UNSUPPORTED;
#endif
}

LDG_EXPORT uint32_t ldg_compute_create(const ldg_compute_desc_t *desc, void **out)
{
    ldg_compute_t *compute = 0x0;
    void *blk = 0x0;
    void *dev = 0x0;
    uint8_t owned = 0;
    uint32_t err = 0;
    uint32_t rollback_err = 0;
    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    if (desc->dev != 0x0) { dev = desc->dev; }
    else if (desc->dev_sel != LDG_COMPUTE_DEV_CPU)
    {
        err = ldg_compute_dev_acquire(desc, &dev);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        owned = 1;
    }

    err = ldg_mem_alloc(sizeof(ldg_compute_t), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        if (owned) { rollback_err = ldg_compute_dev_release(dev); if (rollback_err != LDG_ERR_AOK) { return rollback_err; } }

        return err;
    }

    compute = (ldg_compute_t *)blk;
    rollback_err = ldg_mem_secure_zero(compute, sizeof(ldg_compute_t));
    if (LDG_UNLIKELY(rollback_err != LDG_ERR_AOK))
    {
        err = ldg_mem_dealloc(blk);
        if (err == LDG_ERR_AOK) { err = rollback_err; }

        if (owned) { rollback_err = ldg_compute_dev_release(dev); if (rollback_err != LDG_ERR_AOK) { return rollback_err; } }

        return err;
    }

    compute->dev = dev;
    compute->owned = owned;
    *out = compute;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_compute_destroy(void *compute)
{
    ldg_compute_t *ctx = (ldg_compute_t *)compute;
    uint32_t err = LDG_ERR_AOK;
    if (LDG_UNLIKELY(!compute)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->owned) { err = ldg_compute_dev_release(ctx->dev); }

    ctx->dev = 0x0;
    ctx->owned = 0;
    if (err != LDG_ERR_AOK) { return err; }

    return ldg_mem_dealloc(compute);
}

LDG_EXPORT uint32_t ldg_compute_dev_get(void *compute, void **out)
{
    ldg_compute_t *ctx = (ldg_compute_t *)compute;
    if (LDG_UNLIKELY(!compute)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ctx->dev;
    return LDG_ERR_AOK;
}
