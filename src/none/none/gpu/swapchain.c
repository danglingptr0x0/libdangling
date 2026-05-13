#ifdef LDG_GPU_VULKAN

#include <string.h>
#include <vulkan/vulkan.h>

#include "state.h"

static uint32_t swapchain_depth_create(ldg_gpu_ctx_t *ctx, ldg_gpu_swapchain_entry_t *sc, VkDevice device)
{
    VkImageCreateInfo img_info = { 0 };
    VkMemoryRequirements mem_reqs = { 0 };
    VkMemoryAllocateInfo mem_alloc = { 0 };
    VkImageViewCreateInfo view_info = { 0 };
    VkImage depth_image = VK_NULL_HANDLE;
    VkDeviceMemory depth_mem = VK_NULL_HANDLE;
    VkImageView depth_view = VK_NULL_HANDLE;
    VkFormat depth_vk = VK_FORMAT_UNDEFINED;

    depth_vk = (VkFormat)gpu_fmt_to_vk(sc->depth_fmt);
    if (LDG_UNLIKELY(depth_vk == VK_FORMAT_UNDEFINED)) { return LDG_ERR_GPU_FMT_UNSUPPORTED; }

    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.format = depth_vk;
    img_info.extent.width = sc->w;
    img_info.extent.height = sc->h;
    img_info.extent.depth = 1;
    img_info.mipLevels = 1;
    img_info.arrayLayers = 1;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (LDG_UNLIKELY(vkCreateImage(device, &img_info, 0x0, &depth_image) != VK_SUCCESS)) { return LDG_ERR_GPU_DEPTH_CREATE; }

    vkGetImageMemoryRequirements(device, depth_image, &mem_reqs);

    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.allocationSize = mem_reqs.size;
    mem_alloc.memoryTypeIndex = ctx->device_local_type_idx;

    if (LDG_UNLIKELY(vkAllocateMemory(device, &mem_alloc, 0x0, &depth_mem) != VK_SUCCESS))
    {
        vkDestroyImage(device, depth_image, 0x0);
        return LDG_ERR_GPU_DEPTH_CREATE;
    }

    if (LDG_UNLIKELY(vkBindImageMemory(device, depth_image, depth_mem, 0) != VK_SUCCESS))
    {
        vkFreeMemory(device, depth_mem, 0x0);
        vkDestroyImage(device, depth_image, 0x0);
        return LDG_ERR_GPU_DEPTH_CREATE;
    }

    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = depth_image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = depth_vk;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    if (LDG_UNLIKELY(vkCreateImageView(device, &view_info, 0x0, &depth_view) != VK_SUCCESS))
    {
        vkFreeMemory(device, depth_mem, 0x0);
        vkDestroyImage(device, depth_image, 0x0);
        return LDG_ERR_GPU_DEPTH_CREATE;
    }

    sc->depth_image = (void *)depth_image;
    sc->depth_mem = (void *)depth_mem;
    sc->depth_image_view = (void *)depth_view;
    return LDG_ERR_AOK;
}

static uint32_t swapchain_depth_destroy(ldg_gpu_swapchain_entry_t *sc, VkDevice device)
{
    if (LDG_UNLIKELY(!sc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sc->depth_image_view)
    {
        vkDestroyImageView(device, (VkImageView)sc->depth_image_view, 0x0);
        sc->depth_image_view = 0x0;
    }

    if (sc->depth_image)
    {
        vkDestroyImage(device, (VkImage)sc->depth_image, 0x0);
        sc->depth_image = 0x0;
    }

    if (sc->depth_mem)
    {
        vkFreeMemory(device, (VkDeviceMemory)sc->depth_mem, 0x0);
        sc->depth_mem = 0x0;
    }

    return LDG_ERR_AOK;
}

static uint32_t swapchain_framebuffers_destroy(ldg_gpu_swapchain_entry_t *sc, VkDevice device)
{
    uint32_t i = 0;

    if (LDG_UNLIKELY(!sc)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (i = 0; i < sc->image_cunt && i < LDG_GPU_SWAPCHAIN_IMAGE_MAX; i++) { if (sc->images[i].framebuffer)
        {
            vkDestroyFramebuffer(device, (VkFramebuffer)sc->images[i].framebuffer, 0x0);
            sc->images[i].framebuffer = 0x0;
        }
    }
    sc->cached_renderpass_id = UINT32_MAX;
    return LDG_ERR_AOK;
}

static uint32_t swapchain_image_views_destroy(ldg_gpu_swapchain_entry_t *sc, VkDevice device)
{
    uint32_t i = 0;

    if (LDG_UNLIKELY(!sc)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (i = 0; i < sc->image_cunt && i < LDG_GPU_SWAPCHAIN_IMAGE_MAX; i++) { if (sc->images[i].image_view)
        {
            vkDestroyImageView(device, (VkImageView)sc->images[i].image_view, 0x0);
            sc->images[i].image_view = 0x0;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t swapchain_image_views_create(ldg_gpu_swapchain_entry_t *sc, VkDevice device, VkSwapchainKHR vk_sc)
{
    VkImage images[LDG_GPU_SWAPCHAIN_IMAGE_MAX] = { 0 };
    VkImageViewCreateInfo view_info = { 0 };
    uint32_t image_cunt = LDG_GPU_SWAPCHAIN_IMAGE_MAX;
    VkFormat color_vk = VK_FORMAT_UNDEFINED;
    uint32_t i = 0;

    if (LDG_UNLIKELY(vkGetSwapchainImagesKHR(device, vk_sc, &image_cunt, images) != VK_SUCCESS)) { return LDG_ERR_GPU_SWAPCHAIN_CREATE; }

    if (image_cunt > LDG_GPU_SWAPCHAIN_IMAGE_MAX) { image_cunt = LDG_GPU_SWAPCHAIN_IMAGE_MAX; }

    sc->image_cunt = image_cunt;
    color_vk = (VkFormat)gpu_fmt_to_vk(sc->color_fmt);

    for (i = 0; i < image_cunt; i++)
    {
        view_info = (VkImageViewCreateInfo)LDG_STRUCT_ZERO_INIT;

        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = color_vk;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        if (LDG_UNLIKELY(vkCreateImageView(device, &view_info, 0x0, (VkImageView *)&sc->images[i].image_view) != VK_SUCCESS)) { return LDG_ERR_GPU_SWAPCHAIN_CREATE; }
    }

    return LDG_ERR_AOK;
}

static uint32_t swapchain_sync_create(ldg_gpu_ctx_t *ctx, ldg_gpu_swapchain_entry_t *sc, VkDevice device)
{
    VkSemaphoreCreateInfo sem_info = { 0 };
    VkFenceCreateInfo fence_info = { 0 };
    VkCommandBufferAllocateInfo cmd_alloc = { 0 };
    uint32_t i = 0;

    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    cmd_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc.commandPool = (VkCommandPool)ctx->cmd_pool;
    cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc.commandBufferCount = 1;

    for (i = 0; i < LDG_GPU_FRAME_IN_FLIGHT; i++)
    {
        if (LDG_UNLIKELY(vkCreateSemaphore(device, &sem_info, 0x0, (VkSemaphore *)&sc->frame_sync[i].image_available_sem) != VK_SUCCESS)) { return LDG_ERR_GPU_SWAPCHAIN_CREATE; }

        if (LDG_UNLIKELY(vkCreateSemaphore(device, &sem_info, 0x0, (VkSemaphore *)&sc->frame_sync[i].render_finished_sem) != VK_SUCCESS)) { return LDG_ERR_GPU_SWAPCHAIN_CREATE; }

        if (LDG_UNLIKELY(vkCreateFence(device, &fence_info, 0x0, (VkFence *)&sc->frame_sync[i].in_flight_fence) != VK_SUCCESS)) { return LDG_ERR_GPU_SWAPCHAIN_CREATE; }

        if (LDG_UNLIKELY(vkAllocateCommandBuffers(device, &cmd_alloc, (VkCommandBuffer *)&sc->frame_sync[i].cmd_buff) != VK_SUCCESS)) { return LDG_ERR_GPU_SWAPCHAIN_CREATE; }
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_swapchain_create(void *vk, const ldg_gpu_swapchain_desc_t *desc, ldg_gpu_swapchain_t *out)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkSurfaceCapabilitiesKHR caps = { 0 };
    VkSwapchainCreateInfoKHR sc_info = { 0 };
    VkSwapchainKHR vk_sc = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSurfaceFormatKHR surf_fmts[16] = { { 0 } };
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice phys = VK_NULL_HANDLE;
    VkBool32 present_supported = VK_FALSE;
    uint32_t fmt_cunt = 16;
    uint32_t slot = UINT32_MAX;
    uint32_t chosen_fmt = LDG_GPU_FMT_UNDEFINED;
    VkFormat chosen_vk_fmt = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR chosen_color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    uint32_t image_cunt = 0;
    uint32_t ret = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (ldg_gpu_swapchain_t)LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(!ctx->has_swapchain_ext)) { return LDG_ERR_GPU_QUEUE_NOT_FOUND; }

    if (LDG_UNLIKELY(desc->surface_id >= LDG_GPU_SURFACE_MAX || !ctx->surfaces[desc->surface_id].in_use)) { return LDG_ERR_GPU_SURFACE_NOT_FOUND; }

    device = (VkDevice)ctx->device;
    phys = (VkPhysicalDevice)ctx->phys_dev;
    surface = (VkSurfaceKHR)ctx->surfaces[desc->surface_id].vk_surface;

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(vkGetPhysicalDeviceSurfaceSupportKHR(phys, ctx->gfx_queue_family_idx, surface, &present_supported) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_CREATE;
    }

    if (LDG_UNLIKELY(!present_supported))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_CREATE;
    }

    for (i = 0; i < LDG_GPU_SWAPCHAIN_MAX; i++) { if (!ctx->swapchains[i].in_use)
        {
            slot = i;
            break;
        }
    }

    if (LDG_UNLIKELY(slot == UINT32_MAX))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_FULL;
    }

    if (LDG_UNLIKELY(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_CREATE;
    }

    if (LDG_UNLIKELY(vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &fmt_cunt, surf_fmts) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_CREATE;
    }

    chosen_vk_fmt = surf_fmts[0].format;
    chosen_color_space = surf_fmts[0].colorSpace;
    for (i = 0; i < fmt_cunt; i++) { if (surf_fmts[i].format == VK_FORMAT_B8G8R8A8_SRGB && surf_fmts[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            chosen_vk_fmt = surf_fmts[i].format;
            chosen_color_space = surf_fmts[i].colorSpace;
            break;
        }
    }
    chosen_fmt = gpu_vk_to_fmt((uint32_t)chosen_vk_fmt);

    image_cunt = desc->preferred_image_cunt;
    if (image_cunt < caps.minImageCount) { image_cunt = caps.minImageCount; }

    if (caps.maxImageCount > 0 && image_cunt > caps.maxImageCount) { image_cunt = caps.maxImageCount; }

    if (image_cunt > LDG_GPU_SWAPCHAIN_IMAGE_MAX) { image_cunt = LDG_GPU_SWAPCHAIN_IMAGE_MAX; }

    sc_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sc_info.surface = surface;
    sc_info.minImageCount = image_cunt;
    sc_info.imageFormat = chosen_vk_fmt;
    sc_info.imageColorSpace = chosen_color_space;
    sc_info.imageExtent.width = desc->w;
    sc_info.imageExtent.height = desc->h;
    sc_info.imageArrayLayers = 1;
    sc_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sc_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sc_info.preTransform = caps.currentTransform;
    sc_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sc_info.presentMode = (desc->present_mode == LDG_GPU_PRESENT_MAILBOX) ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
    sc_info.clipped = VK_TRUE;

    if (LDG_UNLIKELY(vkCreateSwapchainKHR(device, &sc_info, 0x0, &vk_sc) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_CREATE;
    }

    ctx->swapchains[slot] = (ldg_gpu_swapchain_entry_t)LDG_STRUCT_ZERO_INIT;
    ctx->swapchains[slot].vk_swapchain = (void *)vk_sc;
    ctx->swapchains[slot].surface_id = desc->surface_id;
    ctx->swapchains[slot].w = desc->w;
    ctx->swapchains[slot].h = desc->h;
    ctx->swapchains[slot].color_fmt = chosen_fmt;
    ctx->swapchains[slot].depth_fmt = desc->depth_fmt;
    ctx->swapchains[slot].present_mode = desc->present_mode;
    ctx->swapchains[slot].cached_renderpass_id = UINT32_MAX;
    ctx->swapchains[slot].current_frame_idx = 0;
    ctx->swapchains[slot].in_use = 1;

    ret = swapchain_image_views_create(&ctx->swapchains[slot], device, vk_sc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        vkDestroySwapchainKHR(device, vk_sc, 0x0);
        ctx->swapchains[slot] = (ldg_gpu_swapchain_entry_t)LDG_STRUCT_ZERO_INIT;

        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return ret;
    }

    if (desc->depth_fmt != LDG_GPU_FMT_UNDEFINED)
    {
        ret = swapchain_depth_create(ctx, &ctx->swapchains[slot], device);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            if (LDG_UNLIKELY(swapchain_image_views_destroy(&ctx->swapchains[slot], device) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("image views destroy failed"); }

            vkDestroySwapchainKHR(device, vk_sc, 0x0);
            ctx->swapchains[slot] = (ldg_gpu_swapchain_entry_t)LDG_STRUCT_ZERO_INIT;

            LDG_GPU_UNLOCK_OR_WARN(ctx);
            return ret;
        }
    }

    ret = swapchain_sync_create(ctx, &ctx->swapchains[slot], device);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(swapchain_depth_destroy(&ctx->swapchains[slot], device) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("depth destroy failed"); }

        if (LDG_UNLIKELY(swapchain_image_views_destroy(&ctx->swapchains[slot], device) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("image views destroy failed"); }

        vkDestroySwapchainKHR(device, vk_sc, 0x0);
        ctx->swapchains[slot] = (ldg_gpu_swapchain_entry_t)LDG_STRUCT_ZERO_INIT;

        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return ret;
    }

    out->id = slot;
    out->image_cunt = ctx->swapchains[slot].image_cunt;
    out->w = desc->w;
    out->h = desc->h;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_swapchain_destroy(void *vk, uint32_t swapchain_id)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkDevice device = VK_NULL_HANDLE;
    ldg_gpu_swapchain_entry_t *sc = 0x0;
    uint32_t ret = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(swapchain_id >= LDG_GPU_SWAPCHAIN_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    device = (VkDevice)ctx->device;
    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sc = &ctx->swapchains[swapchain_id];
    if (LDG_UNLIKELY(!sc->in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_NOT_FOUND;
    }

    if (LDG_UNLIKELY(vkDeviceWaitIdle(device) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_NOT_FOUND;
    }

    for (i = 0; i < LDG_GPU_FRAME_IN_FLIGHT; i++)
    {
        if (sc->frame_sync[i].in_flight_fence) { vkDestroyFence(device, (VkFence)sc->frame_sync[i].in_flight_fence, 0x0); }

        if (sc->frame_sync[i].image_available_sem) { vkDestroySemaphore(device, (VkSemaphore)sc->frame_sync[i].image_available_sem, 0x0); }

        if (sc->frame_sync[i].render_finished_sem) { vkDestroySemaphore(device, (VkSemaphore)sc->frame_sync[i].render_finished_sem, 0x0); }

        if (sc->frame_sync[i].cmd_buff) { vkFreeCommandBuffers(device, (VkCommandPool)ctx->cmd_pool, 1, (VkCommandBuffer *)&sc->frame_sync[i].cmd_buff); }
    }

    if (LDG_UNLIKELY(swapchain_framebuffers_destroy(sc, device) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("framebuffers destroy failed"); }

    if (LDG_UNLIKELY(swapchain_depth_destroy(sc, device) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("depth destroy failed"); }

    if (LDG_UNLIKELY(swapchain_image_views_destroy(sc, device) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("image views destroy failed"); }

    if (sc->vk_swapchain) { vkDestroySwapchainKHR(device, (VkSwapchainKHR)sc->vk_swapchain, 0x0); }

    *sc = (ldg_gpu_swapchain_entry_t)LDG_STRUCT_ZERO_INIT;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_swapchain_recreate(void *vk, uint32_t swapchain_id, uint32_t new_w, uint32_t new_h)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkSurfaceCapabilitiesKHR caps = { 0 };
    VkSwapchainCreateInfoKHR sc_info = { 0 };
    VkSwapchainKHR old_sc = VK_NULL_HANDLE;
    VkSwapchainKHR new_sc = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice phys = VK_NULL_HANDLE;
    ldg_gpu_swapchain_entry_t *sc = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(swapchain_id >= LDG_GPU_SWAPCHAIN_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(new_w == 0 || new_h == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    device = (VkDevice)ctx->device;
    phys = (VkPhysicalDevice)ctx->phys_dev;
    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sc = &ctx->swapchains[swapchain_id];
    if (LDG_UNLIKELY(!sc->in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_NOT_FOUND;
    }

    if (LDG_UNLIKELY(vkDeviceWaitIdle(device) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_NOT_FOUND;
    }

    if (LDG_UNLIKELY(swapchain_framebuffers_destroy(sc, device) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("framebuffers destroy failed"); }

    if (LDG_UNLIKELY(swapchain_depth_destroy(sc, device) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("depth destroy failed"); }

    if (LDG_UNLIKELY(swapchain_image_views_destroy(sc, device) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("image views destroy failed"); }

    surface = (VkSurfaceKHR)ctx->surfaces[sc->surface_id].vk_surface;
    old_sc = (VkSwapchainKHR)sc->vk_swapchain;

    if (LDG_UNLIKELY(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_CREATE;
    }

    sc_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sc_info.surface = surface;
    sc_info.minImageCount = sc->image_cunt;
    sc_info.imageFormat = (VkFormat)gpu_fmt_to_vk(sc->color_fmt);
    sc_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sc_info.imageExtent.width = new_w;
    sc_info.imageExtent.height = new_h;
    sc_info.imageArrayLayers = 1;
    sc_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sc_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sc_info.preTransform = caps.currentTransform;
    sc_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sc_info.presentMode = (sc->present_mode == LDG_GPU_PRESENT_MAILBOX) ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
    sc_info.clipped = VK_TRUE;
    sc_info.oldSwapchain = old_sc;

    if (LDG_UNLIKELY(vkCreateSwapchainKHR(device, &sc_info, 0x0, &new_sc) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_CREATE;
    }

    vkDestroySwapchainKHR(device, old_sc, 0x0);

    sc->vk_swapchain = (void *)new_sc;
    sc->w = new_w;
    sc->h = new_h;

    ret = swapchain_image_views_create(sc, device, new_sc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return ret;
    }

    if (sc->depth_fmt != LDG_GPU_FMT_UNDEFINED)
    {
        ret = swapchain_depth_create(ctx, sc, device);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
        {
            LDG_GPU_UNLOCK_OR_WARN(ctx);
            return ret;
        }
    }

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_swapchain_image_acquire(void *vk, uint32_t swapchain_id, uint32_t *image_idx)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkDevice device = VK_NULL_HANDLE;
    VkFence in_flight = VK_NULL_HANDLE;
    VkSemaphore img_avail = VK_NULL_HANDLE;
    ldg_gpu_swapchain_entry_t *sc = 0x0;
    uint32_t fidx = UINT32_MAX;
    uint32_t idx = UINT32_MAX;
    uint32_t ret = 0;
    VkResult res = VK_SUCCESS;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!image_idx)) { return LDG_ERR_FUNC_ARG_NULL; }

    *image_idx = UINT32_MAX;

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(swapchain_id >= LDG_GPU_SWAPCHAIN_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    device = (VkDevice)ctx->device;
    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sc = &ctx->swapchains[swapchain_id];
    if (LDG_UNLIKELY(!sc->in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_NOT_FOUND;
    }

    fidx = sc->current_frame_idx;
    in_flight = (VkFence)sc->frame_sync[fidx].in_flight_fence;
    img_avail = (VkSemaphore)sc->frame_sync[fidx].image_available_sem;

    if (LDG_UNLIKELY(vkWaitForFences(device, 1, &in_flight, VK_TRUE, UINT64_MAX) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FENCE_TIMEOUT;
    }

    res = vkAcquireNextImageKHR(device, (VkSwapchainKHR)sc->vk_swapchain, UINT64_MAX, img_avail, VK_NULL_HANDLE, &idx);
    if (LDG_UNLIKELY(res == VK_ERROR_OUT_OF_DATE_KHR))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_OUT_OF_DATE;
    }

    if (LDG_UNLIKELY(vkResetFences(device, 1, &in_flight) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_ACQUIRE;
    }

    sc->acquired_image_idx = idx;
    *image_idx = idx;

    LDG_GPU_UNLOCK_OR_WARN(ctx);

    if (res == VK_SUBOPTIMAL_KHR) { return LDG_ERR_GPU_SWAPCHAIN_SUBOPTIMAL; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_swapchain_present(void *vk, uint32_t swapchain_id, uint32_t image_idx)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkPresentInfoKHR present_info = { 0 };
    VkSwapchainKHR vk_sc = VK_NULL_HANDLE;
    VkSemaphore render_finished = VK_NULL_HANDLE;
    ldg_gpu_swapchain_entry_t *sc = 0x0;
    uint32_t fidx = UINT32_MAX;
    uint32_t ret = 0;
    VkResult res = VK_SUCCESS;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(swapchain_id >= LDG_GPU_SWAPCHAIN_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sc = &ctx->swapchains[swapchain_id];
    if (LDG_UNLIKELY(!sc->in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SWAPCHAIN_NOT_FOUND;
    }

    fidx = sc->current_frame_idx;
    vk_sc = (VkSwapchainKHR)sc->vk_swapchain;
    render_finished = (VkSemaphore)sc->frame_sync[fidx].render_finished_sem;

    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_finished;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &vk_sc;
    present_info.pImageIndices = &image_idx;

    res = vkQueuePresentKHR((VkQueue)ctx->queue, &present_info);

    sc->current_frame_idx = (fidx + 1) % LDG_GPU_FRAME_IN_FLIGHT;

    LDG_GPU_UNLOCK_OR_WARN(ctx);

    if (res == VK_ERROR_OUT_OF_DATE_KHR) { return LDG_ERR_GPU_SWAPCHAIN_OUT_OF_DATE; }

    if (res == VK_SUBOPTIMAL_KHR) { return LDG_ERR_GPU_SWAPCHAIN_SUBOPTIMAL; }

    if (LDG_UNLIKELY(res != VK_SUCCESS)) { return LDG_ERR_GPU_FRAME_PRESENT; }

    return LDG_ERR_AOK;
}

#endif
