#ifdef LDG_GPU_VULKAN

#include <string.h>
#include <vulkan/vulkan.h>

#include "state.h"

LDG_EXPORT uint32_t ldg_gpu_surface_create(void *vk, void *native_surface, ldg_gpu_surface_t *out)
{
    ldg_gpu_ctx_t *ctx = vk;
    uint32_t slot = UINT32_MAX;
    uint32_t ret = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!native_surface)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (ldg_gpu_surface_t)LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(!ctx->has_gfx)) { return LDG_ERR_GPU_QUEUE_NOT_FOUND; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (i = 0; i < LDG_GPU_SURFACE_MAX; i++) { if (!ctx->surfaces[i].in_use)
        {
            slot = i;
            break;
        }
    }

    if (LDG_UNLIKELY(slot == UINT32_MAX))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SURFACE_FULL;
    }

    ctx->surfaces[slot].vk_surface = native_surface;
    ctx->surfaces[slot].in_use = 1;

    out->id = slot;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_surface_destroy(void *vk, uint32_t surface_id)
{
    ldg_gpu_ctx_t *ctx = vk;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(surface_id >= LDG_GPU_SURFACE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!ctx->surfaces[surface_id].in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SURFACE_NOT_FOUND;
    }

    if (ctx->surfaces[surface_id].vk_surface) { vkDestroySurfaceKHR((VkInstance)ctx->instance, (VkSurfaceKHR)ctx->surfaces[surface_id].vk_surface, 0x0); }

    ctx->surfaces[surface_id] = (ldg_gpu_surface_entry_t)LDG_STRUCT_ZERO_INIT;

    return ldg_mut_unlock(&ctx->mut);
}

#endif
