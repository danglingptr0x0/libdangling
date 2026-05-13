#ifdef LDG_GPU_VULKAN

#include <string.h>
#include <vulkan/vulkan.h>

#include "state.h"

static uint32_t frame_framebuffers_ensure(ldg_gpu_ctx_t *ctx, ldg_gpu_swapchain_entry_t *sc, uint32_t renderpass_id, VkDevice device)
{
    VkFramebufferCreateInfo fb_info = { 0 };
    VkImageView attachments[2] = { 0 };
    uint32_t attach_cunt = 0;
    uint32_t i = 0;

    if (sc->cached_renderpass_id == renderpass_id) { return LDG_ERR_AOK; }

    for (i = 0; i < sc->image_cunt && i < LDG_GPU_SWAPCHAIN_IMAGE_MAX; i++) { if (sc->images[i].framebuffer)
        {
            vkDestroyFramebuffer(device, (VkFramebuffer)sc->images[i].framebuffer, 0x0);
            sc->images[i].framebuffer = 0x0;
        }
    }

    for (i = 0; i < sc->image_cunt && i < LDG_GPU_SWAPCHAIN_IMAGE_MAX; i++)
    {
        attachments[0] = (VkImageView)sc->images[i].image_view;
        attach_cunt = 1;

        if (sc->depth_image_view)
        {
            attachments[1] = (VkImageView)sc->depth_image_view;
            attach_cunt = 2;
        }

        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.renderPass = (VkRenderPass)ctx->renderpasses[renderpass_id].renderpass;
        fb_info.attachmentCount = attach_cunt;
        fb_info.pAttachments = attachments;
        fb_info.width = sc->w;
        fb_info.height = sc->h;
        fb_info.layers = 1;

        if (LDG_UNLIKELY(vkCreateFramebuffer(device, &fb_info, 0x0, (VkFramebuffer *)&sc->images[i].framebuffer) != VK_SUCCESS)) { return LDG_ERR_GPU_FRAME_BEGIN; }
    }

    sc->cached_renderpass_id = renderpass_id;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_frame_begin(void *vk, uint32_t swapchain_id, ldg_gpu_frame_t *frame)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkCommandBufferBeginInfo begin_info = { 0 };
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    ldg_gpu_swapchain_entry_t *sc = 0x0;
    uint32_t fidx = UINT32_MAX;
    uint32_t slot = UINT32_MAX;
    uint32_t ret = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!frame)) { return LDG_ERR_FUNC_ARG_NULL; }

    *frame = (ldg_gpu_frame_t)LDG_STRUCT_ZERO_INIT;

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

    for (i = 0; i < LDG_GPU_FRAME_IN_FLIGHT; i++) { if (!ctx->frames[i].in_use)
        {
            slot = i;
            break;
        }
    }

    if (LDG_UNLIKELY(slot == UINT32_MAX))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_FULL;
    }

    fidx = sc->current_frame_idx;
    cmd = (VkCommandBuffer)sc->frame_sync[fidx].cmd_buff;

    if (LDG_UNLIKELY(vkResetCommandBuffer(cmd, 0) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_BEGIN;
    }

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (LDG_UNLIKELY(vkBeginCommandBuffer(cmd, &begin_info) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_BEGIN;
    }

    ctx->frames[slot].cmd_buff = (void *)cmd;
    ctx->frames[slot].swapchain_id = swapchain_id;
    ctx->frames[slot].image_idx = sc->acquired_image_idx;
    ctx->frames[slot].frame_sync_idx = fidx;
    ctx->frames[slot].recording = 1;
    ctx->frames[slot].in_renderpass = 0;
    ctx->frames[slot].in_use = 1;

    frame->id = slot;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_frame_renderpass_begin(void *vk, ldg_gpu_frame_t *frame, uint32_t renderpass_id, double clear_color[4], double clear_depth)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkRenderPassBeginInfo rp_begin = { 0 };
    VkClearValue clear_vals[2] = { { { { 0 } } } };
    VkViewport viewport = { 0 };
    VkRect2D scissor = { { 0 }, { 0 } };
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    ldg_gpu_swapchain_entry_t *sc = 0x0;
    ldg_gpu_frame_entry_t *frame_entry = 0x0;
    uint32_t clear_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!frame)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(frame->id >= LDG_GPU_FRAME_IN_FLIGHT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(renderpass_id >= LDG_GPU_RENDERPASS_MAX || !ctx->renderpasses[renderpass_id].in_use)) { return LDG_ERR_GPU_RENDERPASS_NOT_FOUND; }

    device = (VkDevice)ctx->device;
    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    frame_entry = &ctx->frames[frame->id];
    if (LDG_UNLIKELY(!frame_entry->in_use || !frame_entry->recording))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_BEGIN;
    }

    sc = &ctx->swapchains[frame_entry->swapchain_id];

    ret = frame_framebuffers_ensure(ctx, sc, renderpass_id, device);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return ret;
    }

    cmd = (VkCommandBuffer)frame_entry->cmd_buff;

    if (clear_color)
    {
        clear_vals[0].color.float32[0] = (float)clear_color[0];
        clear_vals[0].color.float32[1] = (float)clear_color[1];
        clear_vals[0].color.float32[2] = (float)clear_color[2];
        clear_vals[0].color.float32[3] = (float)clear_color[3];
    }

    clear_cunt = 1;

    if (ctx->renderpasses[renderpass_id].depth_fmt != LDG_GPU_FMT_UNDEFINED)
    {
        clear_vals[1].depthStencil.depth = (float)clear_depth;
        clear_vals[1].depthStencil.stencil = 0;
        clear_cunt = 2;
    }

    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.renderPass = (VkRenderPass)ctx->renderpasses[renderpass_id].renderpass;
    rp_begin.framebuffer = (VkFramebuffer)sc->images[frame_entry->image_idx].framebuffer;
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = sc->w;
    rp_begin.renderArea.extent.height = sc->h;
    rp_begin.clearValueCount = clear_cunt;
    rp_begin.pClearValues = clear_vals;

    vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)sc->w;
    viewport.height = (float)sc->h;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    scissor.extent.width = sc->w;
    scissor.extent.height = sc->h;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    frame_entry->in_renderpass = 1;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_frame_pipeline_bind(void *vk, ldg_gpu_frame_t *frame, uint32_t pipeline_id)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    ldg_gpu_frame_entry_t *frame_entry = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!frame)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(frame->id >= LDG_GPU_FRAME_IN_FLIGHT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(pipeline_id >= LDG_GPU_PIPELINE_POOL_MAX || !ctx->pipelines[pipeline_id].in_use)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    frame_entry = &ctx->frames[frame->id];
    if (LDG_UNLIKELY(!frame_entry->in_use || !frame_entry->recording))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_BEGIN;
    }

    cmd = (VkCommandBuffer)frame_entry->cmd_buff;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)ctx->pipelines[pipeline_id].pipeline);

    if (ctx->pipelines[pipeline_id].kind == LDG_GPU_PIPELINE_GRAPHICS)
    {
        VkDescriptorSetLayout dsl = (VkDescriptorSetLayout)ctx->desc_set_layout;
        VkDescriptorSetAllocateInfo ds_alloc = { 0 };
        VkDescriptorSet ds = VK_NULL_HANDLE;

        ds_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ds_alloc.descriptorPool = (VkDescriptorPool)ctx->desc_pool;
        ds_alloc.descriptorSetCount = 1;
        ds_alloc.pSetLayouts = &dsl;

        if (LDG_UNLIKELY(vkAllocateDescriptorSets((VkDevice)ctx->device, &ds_alloc, &ds) != VK_SUCCESS))
        {
            LDG_GPU_UNLOCK_OR_WARN(ctx);
            return LDG_ERR_GPU_DESC_ALLOC;
        }

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipelineLayout)ctx->gfx_pipeline_layout, 0, 1, &ds, 0, 0x0);
        frame_entry->desc_set = (void *)ds;
    }

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_frame_vertex_buff_bind(void *vk, ldg_gpu_frame_t *frame, uint32_t buff_id)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkBuffer vk_buff = VK_NULL_HANDLE;
    VkDeviceSize offset = 0;
    ldg_gpu_frame_entry_t *frame_entry = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!frame)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(frame->id >= LDG_GPU_FRAME_IN_FLIGHT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(buff_id >= LDG_GPU_BUFF_MAX || !ctx->buffs[buff_id].in_use)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    frame_entry = &ctx->frames[frame->id];
    if (LDG_UNLIKELY(!frame_entry->in_use || !frame_entry->recording))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_BEGIN;
    }

    cmd = (VkCommandBuffer)frame_entry->cmd_buff;
    vk_buff = (VkBuffer)ctx->buffs[buff_id].vk_buff;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vk_buff, &offset);

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_frame_idx_buff_bind(void *vk, ldg_gpu_frame_t *frame, uint32_t buff_id)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    ldg_gpu_frame_entry_t *frame_entry = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!frame)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(frame->id >= LDG_GPU_FRAME_IN_FLIGHT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(buff_id >= LDG_GPU_BUFF_MAX || !ctx->buffs[buff_id].in_use)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    frame_entry = &ctx->frames[frame->id];
    if (LDG_UNLIKELY(!frame_entry->in_use || !frame_entry->recording))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_BEGIN;
    }

    cmd = (VkCommandBuffer)frame_entry->cmd_buff;
    vkCmdBindIndexBuffer(cmd, (VkBuffer)ctx->buffs[buff_id].vk_buff, 0, VK_INDEX_TYPE_UINT32);

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_frame_buff_bind(void *vk, ldg_gpu_frame_t *frame, uint32_t slot, uint32_t buff_id)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkDescriptorBufferInfo buff_info = { 0 };
    VkWriteDescriptorSet write = { 0 };
    VkDescriptorSet ds = VK_NULL_HANDLE;
    ldg_gpu_frame_entry_t *frame_entry = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!frame)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(frame->id >= LDG_GPU_FRAME_IN_FLIGHT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(slot >= LDG_GPU_BIND_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(buff_id >= LDG_GPU_BUFF_MAX || !ctx->buffs[buff_id].in_use)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    frame_entry = &ctx->frames[frame->id];
    if (LDG_UNLIKELY(!frame_entry->in_use || !frame_entry->recording))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_BEGIN;
    }

    if (LDG_UNLIKELY(!frame_entry->desc_set))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_DESC_ALLOC;
    }

    ds = (VkDescriptorSet)frame_entry->desc_set;

    buff_info.buffer = (VkBuffer)ctx->buffs[buff_id].vk_buff;
    buff_info.offset = 0;
    buff_info.range = VK_WHOLE_SIZE;

    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = ds;
    write.dstBinding = slot;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &buff_info;

    vkUpdateDescriptorSets((VkDevice)ctx->device, 1, &write, 0, 0x0);

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_frame_push_const(void *vk, ldg_gpu_frame_t *frame, uint32_t offset, uint32_t size, const void *data)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    ldg_gpu_frame_entry_t *frame_entry = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!frame)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!data)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(frame->id >= LDG_GPU_FRAME_IN_FLIGHT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(offset + size > LDG_GPU_PUSH_CONST_SIZE)) { return LDG_ERR_BOUNDS; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    frame_entry = &ctx->frames[frame->id];
    if (LDG_UNLIKELY(!frame_entry->in_use || !frame_entry->recording))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_BEGIN;
    }

    cmd = (VkCommandBuffer)frame_entry->cmd_buff;
    vkCmdPushConstants(cmd, (VkPipelineLayout)ctx->gfx_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, offset, size, data);

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_frame_draw(void *vk, ldg_gpu_frame_t *frame, uint32_t vertex_cunt, uint32_t instance_cunt)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    ldg_gpu_frame_entry_t *frame_entry = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!frame)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(frame->id >= LDG_GPU_FRAME_IN_FLIGHT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    frame_entry = &ctx->frames[frame->id];
    if (LDG_UNLIKELY(!frame_entry->in_use || !frame_entry->recording || !frame_entry->in_renderpass))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_BEGIN;
    }

    cmd = (VkCommandBuffer)frame_entry->cmd_buff;
    vkCmdDraw(cmd, vertex_cunt, instance_cunt, 0, 0);

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_frame_draw_idxd(void *vk, ldg_gpu_frame_t *frame, uint32_t idx_cunt, uint32_t instance_cunt)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    ldg_gpu_frame_entry_t *frame_entry = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!frame)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(frame->id >= LDG_GPU_FRAME_IN_FLIGHT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    frame_entry = &ctx->frames[frame->id];
    if (LDG_UNLIKELY(!frame_entry->in_use || !frame_entry->recording || !frame_entry->in_renderpass))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_BEGIN;
    }

    cmd = (VkCommandBuffer)frame_entry->cmd_buff;
    vkCmdDrawIndexed(cmd, idx_cunt, instance_cunt, 0, 0, 0);

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_frame_renderpass_end(void *vk, ldg_gpu_frame_t *frame)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    ldg_gpu_frame_entry_t *frame_entry = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!frame)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(frame->id >= LDG_GPU_FRAME_IN_FLIGHT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    frame_entry = &ctx->frames[frame->id];
    if (LDG_UNLIKELY(!frame_entry->in_use || !frame_entry->recording || !frame_entry->in_renderpass))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_BEGIN;
    }

    cmd = (VkCommandBuffer)frame_entry->cmd_buff;
    vkCmdEndRenderPass(cmd);
    frame_entry->in_renderpass = 0;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_frame_end(void *vk, ldg_gpu_frame_t *frame)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkSubmitInfo submit_info = { 0 };
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkSemaphore wait_sem = VK_NULL_HANDLE;
    VkSemaphore sig_sem = VK_NULL_HANDLE;
    VkFence in_flight = VK_NULL_HANDLE;
    ldg_gpu_frame_entry_t *frame_entry = 0x0;
    ldg_gpu_swapchain_entry_t *sc = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!frame)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(frame->id >= LDG_GPU_FRAME_IN_FLIGHT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    frame_entry = &ctx->frames[frame->id];
    if (LDG_UNLIKELY(!frame_entry->in_use || !frame_entry->recording))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_END;
    }

    cmd = (VkCommandBuffer)frame_entry->cmd_buff;

    if (LDG_UNLIKELY(vkEndCommandBuffer(cmd) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FRAME_END;
    }

    sc = &ctx->swapchains[frame_entry->swapchain_id];
    wait_sem = (VkSemaphore)sc->frame_sync[frame_entry->frame_sync_idx].image_available_sem;
    sig_sem = (VkSemaphore)sc->frame_sync[frame_entry->frame_sync_idx].render_finished_sem;
    in_flight = (VkFence)sc->frame_sync[frame_entry->frame_sync_idx].in_flight_fence;

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &wait_sem;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &sig_sem;

    if (LDG_UNLIKELY(vkQueueSubmit((VkQueue)ctx->queue, 1, &submit_info, in_flight) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SUBMIT;
    }

    frame_entry->recording = 0;
    frame_entry->in_use = 0;

    return ldg_mut_unlock(&ctx->mut);
}

#endif
