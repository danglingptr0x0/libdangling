#ifdef LDG_GPU_VULKAN

#include <string.h>
#include <vulkan/vulkan.h>

#include "state.h"

LDG_EXPORT uint32_t ldg_gpu_renderpass_create(void *vk, const ldg_gpu_renderpass_desc_t *desc, uint32_t *id)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkAttachmentDescription attachments[2] = { { 0 } };
    VkAttachmentReference color_ref = { 0 };
    VkAttachmentReference depth_ref = { 0 };
    VkSubpassDescription subpass = { 0 };
    VkSubpassDependency dep = { 0 };
    VkRenderPassCreateInfo rp_info = { 0 };
    VkRenderPass renderpass = VK_NULL_HANDLE;
    VkDevice dev = VK_NULL_HANDLE;
    VkFormat color_vk = VK_FORMAT_UNDEFINED;
    VkFormat depth_vk = VK_FORMAT_UNDEFINED;
    uint32_t attach_cunt = 0;
    uint32_t slot = UINT32_MAX;
    uint32_t ret = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!id)) { return LDG_ERR_FUNC_ARG_NULL; }

    *id = UINT32_MAX;

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(!ctx->has_gfx)) { return LDG_ERR_GPU_QUEUE_NOT_FOUND; }

    color_vk = (VkFormat)gpu_fmt_to_vk(desc->color_fmt);
    if (LDG_UNLIKELY(color_vk == VK_FORMAT_UNDEFINED)) { return LDG_ERR_GPU_FMT_UNSUPPORTED; }

    dev = (VkDevice)ctx->dev;
    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (i = 0; i < LDG_GPU_RENDERPASS_MAX; i++) { if (!ctx->renderpasses[i].in_use)
        {
            slot = i;
            break;
        }
    }

    if (LDG_UNLIKELY(slot == UINT32_MAX))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_RENDERPASS_FULL;
    }

    attachments[0].format = color_vk;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = desc->load_clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = desc->load_clear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attach_cunt = 1;

    color_ref.attachment = 0;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;

    if (desc->depth_fmt != LDG_GPU_FMT_UNDEFINED)
    {
        depth_vk = (VkFormat)gpu_fmt_to_vk(desc->depth_fmt);
        if (LDG_UNLIKELY(depth_vk == VK_FORMAT_UNDEFINED))
        {
            LDG_GPU_UNLOCK_OR_WARN(ctx);
            return LDG_ERR_GPU_FMT_UNSUPPORTED;
        }

        attachments[1].format = depth_vk;
        attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp = desc->load_clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout = desc->load_clear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attach_cunt = 2;

        depth_ref.attachment = 1;
        depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpass.pDepthStencilAttachment = &depth_ref;
    }

    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = 0;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.attachmentCount = attach_cunt;
    rp_info.pAttachments = attachments;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    rp_info.dependencyCount = 1;
    rp_info.pDependencies = &dep;

    if (LDG_UNLIKELY(vkCreateRenderPass(dev, &rp_info, 0x0, &renderpass) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_RENDERPASS_CREATE;
    }

    ctx->renderpasses[slot].renderpass = (void *)renderpass;
    ctx->renderpasses[slot].color_fmt = desc->color_fmt;
    ctx->renderpasses[slot].depth_fmt = desc->depth_fmt;
    ctx->renderpasses[slot].load_clear = desc->load_clear;
    ctx->renderpasses[slot].in_use = 1;

    *id = slot;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_renderpass_destroy(void *vk, uint32_t id)
{
    ldg_gpu_ctx_t *ctx = vk;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(id >= LDG_GPU_RENDERPASS_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!ctx->renderpasses[id].in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_RENDERPASS_NOT_FOUND;
    }

    if (ctx->renderpasses[id].renderpass) { vkDestroyRenderPass((VkDevice)ctx->dev, (VkRenderPass)ctx->renderpasses[id].renderpass, 0x0); }

    ctx->renderpasses[id] = (ldg_gpu_renderpass_entry_t)LDG_STRUCT_ZERO_INIT;

    return ldg_mut_unlock(&ctx->mut);
}

#endif
