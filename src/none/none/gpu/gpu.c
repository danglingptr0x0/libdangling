#ifdef LDG_GPU_VULKAN

#include <vulkan/vulkan.h>

#include "state.h"
#include <dangling/gpu/gpu.h>
#include <dangling/core/err.h>
#include <dangling/mem/secure.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/thread/sync.h>
#include <dangling/io/file.h>
#include <dangling/str/str.h>

static VkBool32 VKAPI_PTR gpu_debug_msg_cb(VkDebugUtilsMessageSeverityFlagBitsEXT severity, __attribute__((unused)) VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT *data, __attribute__((unused)) void *user_data)
{
    if (!data || !data->pMessage) { return VK_FALSE; }

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) { LDG_ERRLOG_ERR(data->pMessage); }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) { LDG_ERRLOG_WARN(data->pMessage); }
    else { LDG_ERRLOG_INFO(data->pMessage); }

    return VK_FALSE;
}

uint32_t gpu_fmt_to_vk(uint32_t fmt)
{
    if (fmt == LDG_GPU_FMT_R8G8B8A8_UNORM) { return (uint32_t)VK_FORMAT_R8G8B8A8_UNORM; }

    if (fmt == LDG_GPU_FMT_B8G8R8A8_UNORM) { return (uint32_t)VK_FORMAT_B8G8R8A8_UNORM; }

    if (fmt == LDG_GPU_FMT_B8G8R8A8_SRGB) { return (uint32_t)VK_FORMAT_B8G8R8A8_SRGB; }

    if (fmt == LDG_GPU_FMT_R32G32_SFLOAT) { return (uint32_t)VK_FORMAT_R32G32_SFLOAT; }

    if (fmt == LDG_GPU_FMT_R32G32B32_SFLOAT) { return (uint32_t)VK_FORMAT_R32G32B32_SFLOAT; }

    if (fmt == LDG_GPU_FMT_R32G32B32A32_SFLOAT) { return (uint32_t)VK_FORMAT_R32G32B32A32_SFLOAT; }

    if (fmt == LDG_GPU_FMT_D24_UNORM_S8_UINT) { return (uint32_t)VK_FORMAT_D24_UNORM_S8_UINT; }

    if (fmt == LDG_GPU_FMT_D32_SFLOAT) { return (uint32_t)VK_FORMAT_D32_SFLOAT; }

    if (fmt == LDG_GPU_FMT_D32_SFLOAT_S8_UINT) { return (uint32_t)VK_FORMAT_D32_SFLOAT_S8_UINT; }

    return (uint32_t)VK_FORMAT_UNDEFINED;
}

uint32_t gpu_vk_to_fmt(uint32_t vk_fmt)
{
    if (vk_fmt == (uint32_t)VK_FORMAT_R8G8B8A8_UNORM) { return LDG_GPU_FMT_R8G8B8A8_UNORM; }

    if (vk_fmt == (uint32_t)VK_FORMAT_B8G8R8A8_UNORM) { return LDG_GPU_FMT_B8G8R8A8_UNORM; }

    if (vk_fmt == (uint32_t)VK_FORMAT_B8G8R8A8_SRGB) { return LDG_GPU_FMT_B8G8R8A8_SRGB; }

    if (vk_fmt == (uint32_t)VK_FORMAT_R32G32_SFLOAT) { return LDG_GPU_FMT_R32G32_SFLOAT; }

    if (vk_fmt == (uint32_t)VK_FORMAT_R32G32B32_SFLOAT) { return LDG_GPU_FMT_R32G32B32_SFLOAT; }

    if (vk_fmt == (uint32_t)VK_FORMAT_R32G32B32A32_SFLOAT) { return LDG_GPU_FMT_R32G32B32A32_SFLOAT; }

    if (vk_fmt == (uint32_t)VK_FORMAT_D24_UNORM_S8_UINT) { return LDG_GPU_FMT_D24_UNORM_S8_UINT; }

    if (vk_fmt == (uint32_t)VK_FORMAT_D32_SFLOAT) { return LDG_GPU_FMT_D32_SFLOAT; }

    if (vk_fmt == (uint32_t)VK_FORMAT_D32_SFLOAT_S8_UINT) { return LDG_GPU_FMT_D32_SFLOAT_S8_UINT; }

    return LDG_GPU_FMT_UNDEFINED;
}

static uint32_t gpu_shutdown_internal(ldg_gpu_ctx_t *ctx)
{
    VkDevice dev = (VkDevice)ctx->dev;
    VkInstance instance = (VkInstance)ctx->instance;
    uint32_t i = 0;
    uint32_t j = 0;

    if (dev) { vkDeviceWaitIdle(dev); }

    for (i = 0; i < LDG_GPU_FRAME_IN_FLIGHT; i++)
    {
        if (!ctx->frames[i].in_use) { continue; }

        ctx->frames[i] = (ldg_gpu_frame_entry_t)LDG_STRUCT_ZERO_INIT;
    }

    for (i = 0; i < LDG_GPU_SWAPCHAIN_MAX; i++)
    {
        if (!ctx->swapchains[i].in_use) { continue; }

        for (j = 0; j < LDG_GPU_FRAME_IN_FLIGHT; j++)
        {
            if (ctx->swapchains[i].frame_sync[j].in_flight_fence) { vkDestroyFence(dev, (VkFence)ctx->swapchains[i].frame_sync[j].in_flight_fence, 0x0); }

            if (ctx->swapchains[i].frame_sync[j].img_available_sem) { vkDestroySemaphore(dev, (VkSemaphore)ctx->swapchains[i].frame_sync[j].img_available_sem, 0x0); }

            if (ctx->swapchains[i].frame_sync[j].render_finished_sem) { vkDestroySemaphore(dev, (VkSemaphore)ctx->swapchains[i].frame_sync[j].render_finished_sem, 0x0); }

            if (ctx->swapchains[i].frame_sync[j].cmd_buff && ctx->cmd_pool) { vkFreeCommandBuffers(dev, (VkCommandPool)ctx->cmd_pool, 1, (VkCommandBuffer *)&ctx->swapchains[i].frame_sync[j].cmd_buff); }
        }

        for (j = 0; j < ctx->swapchains[i].img_cunt && j < LDG_GPU_SWAPCHAIN_IMG_MAX; j++)
        {
            if (ctx->swapchains[i].imgs[j].fbo) { vkDestroyFramebuffer(dev, (VkFramebuffer)ctx->swapchains[i].imgs[j].fbo, 0x0); }

            if (ctx->swapchains[i].imgs[j].img_view) { vkDestroyImageView(dev, (VkImageView)ctx->swapchains[i].imgs[j].img_view, 0x0); }
        }

        if (ctx->swapchains[i].depth_img_view) { vkDestroyImageView(dev, (VkImageView)ctx->swapchains[i].depth_img_view, 0x0); }

        if (ctx->swapchains[i].depth_img) { vkDestroyImage(dev, (VkImage)ctx->swapchains[i].depth_img, 0x0); }

        if (ctx->swapchains[i].depth_mem) { vkFreeMemory(dev, (VkDeviceMemory)ctx->swapchains[i].depth_mem, 0x0); }

        if (ctx->swapchains[i].vk_swapchain) { vkDestroySwapchainKHR(dev, (VkSwapchainKHR)ctx->swapchains[i].vk_swapchain, 0x0); }
    }

    for (i = 0; i < LDG_GPU_RENDERPASS_MAX; i++)
    {
        if (!ctx->renderpasses[i].in_use) { continue; }

        if (ctx->renderpasses[i].renderpass) { vkDestroyRenderPass(dev, (VkRenderPass)ctx->renderpasses[i].renderpass, 0x0); }
    }

    for (i = 0; i < LDG_GPU_SURFACE_MAX; i++)
    {
        if (!ctx->surfaces[i].in_use) { continue; }

        if (ctx->surfaces[i].vk_surface && instance) { vkDestroySurfaceKHR(instance, (VkSurfaceKHR)ctx->surfaces[i].vk_surface, 0x0); }
    }

    for (i = 0; i < LDG_GPU_FENCE_MAX; i++)
    {
        if (!ctx->fences[i].in_use) { continue; }

        if (ctx->fences[i].fence) { vkDestroyFence(dev, (VkFence)ctx->fences[i].fence, 0x0); }

        if (ctx->fences[i].cmd_buff) { vkFreeCommandBuffers(dev, (VkCommandPool)ctx->cmd_pool, 1, (VkCommandBuffer *)&ctx->fences[i].cmd_buff); }

        if (ctx->fences[i].desc_set && ctx->desc_pool) { vkFreeDescriptorSets(dev, (VkDescriptorPool)ctx->desc_pool, 1, (VkDescriptorSet *)&ctx->fences[i].desc_set); }
    }

    for (i = 0; i < LDG_GPU_PIPELINE_POOL_MAX; i++)
    {
        if (!ctx->pipelines[i].in_use) { continue; }

        if (ctx->pipelines[i].pipeline) { vkDestroyPipeline(dev, (VkPipeline)ctx->pipelines[i].pipeline, 0x0); }

        if (ctx->pipelines[i].kind == LDG_GPU_PIPELINE_COMPUTE && ctx->pipelines[i].layout) { vkDestroyPipelineLayout(dev, (VkPipelineLayout)ctx->pipelines[i].layout, 0x0); }

        if (ctx->pipelines[i].vert_module) { vkDestroyShaderModule(dev, (VkShaderModule)ctx->pipelines[i].vert_module, 0x0); }

        if (ctx->pipelines[i].frag_module) { vkDestroyShaderModule(dev, (VkShaderModule)ctx->pipelines[i].frag_module, 0x0); }
    }

    for (i = 0; i < LDG_GPU_BUFF_MAX; i++)
    {
        if (!ctx->buffs[i].in_use) { continue; }

        if (ctx->buffs[i].vk_buff) { vkDestroyBuffer(dev, (VkBuffer)ctx->buffs[i].vk_buff, 0x0); }
    }

    if (ctx->staging_map && ctx->staging_mem) { vkUnmapMemory(dev, (VkDeviceMemory)ctx->staging_mem); }

    if (ctx->staging_buff) { vkDestroyBuffer(dev, (VkBuffer)ctx->staging_buff, 0x0); }

    if (ctx->staging_mem) { vkFreeMemory(dev, (VkDeviceMemory)ctx->staging_mem, 0x0); }

    for (i = 0; i < LDG_GPU_MEM_SLAB_MAX; i++)
    {
        if (!ctx->slabs[i].in_use) { continue; }

        if (ctx->slabs[i].mem) { vkFreeMemory(dev, (VkDeviceMemory)ctx->slabs[i].mem, 0x0); }
    }

    if (ctx->gfx_pipeline_layout) { vkDestroyPipelineLayout(dev, (VkPipelineLayout)ctx->gfx_pipeline_layout, 0x0); }

    if (ctx->desc_set_layout) { vkDestroyDescriptorSetLayout(dev, (VkDescriptorSetLayout)ctx->desc_set_layout, 0x0); }

    if (ctx->desc_pool) { vkDestroyDescriptorPool(dev, (VkDescriptorPool)ctx->desc_pool, 0x0); }

    if (ctx->cmd_pool) { vkDestroyCommandPool(dev, (VkCommandPool)ctx->cmd_pool, 0x0); }

    if (ctx->debug_messenger && instance)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroy_fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroy_fn) { destroy_fn(instance, (VkDebugUtilsMessengerEXT)ctx->debug_messenger, 0x0); }
    }

    if (dev) { vkDestroyDevice(dev, 0x0); }

    if (instance) { vkDestroyInstance(instance, 0x0); }

    if (ctx->mut_init) { ldg_mut_destroy(&ctx->mut); }

    *ctx = (ldg_gpu_ctx_t)LDG_STRUCT_ZERO_INIT;

    return LDG_ERR_AOK;
}

uint32_t gpu_slab_alloc(ldg_gpu_ctx_t *ctx, uint8_t want_dev_local, uint64_t size, uint64_t alignment, uint32_t mem_type_bits, uint32_t *out_slab_idx, uint64_t *out_offset)
{
    uint32_t i = 0;
    uint64_t aligned_off = 0;

    if (LDG_UNLIKELY(!out_slab_idx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_offset)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_slab_idx = UINT32_MAX;
    *out_offset = 0;

    for (i = 0; i < LDG_GPU_MEM_SLAB_MAX; i++)
    {
        if (!ctx->slabs[i].in_use) { continue; }

        if (want_dev_local && !ctx->slabs[i].is_dev_local) { continue; }

        if (!want_dev_local && !ctx->slabs[i].is_host_visible) { continue; }

        if (!((1u << ctx->slabs[i].mem_type_idx) & mem_type_bits)) { continue; }

        aligned_off = LDG_GPU_ALIGN_UP(ctx->slabs[i].offset, alignment);
        if (aligned_off + size <= ctx->slabs[i].size)
        {
            *out_slab_idx = i;
            *out_offset = aligned_off;
            ctx->slabs[i].offset = aligned_off + size;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_GPU_MEM_ALLOC;
}

uint32_t gpu_spill_slab_create(ldg_gpu_ctx_t *ctx, uint64_t min_size, uint32_t mem_type_bits)
{
    VkMemoryAllocateInfo alloc_info = { 0 };
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkDevice dev = (VkDevice)ctx->dev;
    uint64_t slab_size = 0;
    uint32_t i = 0;

    if (ctx->slab_cunt >= LDG_GPU_MEM_SLAB_MAX) { return LDG_ERR_GPU_MEM_ALLOC; }

    if (!((1u << ctx->host_visible_type_idx) & mem_type_bits)) { return LDG_ERR_GPU_MEM_ALLOC; }

    for (i = 0; i < LDG_GPU_MEM_SLAB_MAX; i++) { if (!ctx->slabs[i].in_use) { break; } }
    if (i >= LDG_GPU_MEM_SLAB_MAX) { return LDG_ERR_GPU_MEM_ALLOC; }

    slab_size = min_size < LDG_GPU_DEFAULT_SLAB_SIZE ? LDG_GPU_DEFAULT_SLAB_SIZE : min_size;

    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = (VkDeviceSize)slab_size;
    alloc_info.memoryTypeIndex = ctx->host_visible_type_idx;

    if (LDG_UNLIKELY(vkAllocateMemory(dev, &alloc_info, 0x0, &mem) != VK_SUCCESS)) { return LDG_ERR_GPU_MEM_ALLOC; }

    ctx->slabs[i].mem = (void *)mem;
    ctx->slabs[i].size = slab_size;
    ctx->slabs[i].offset = 0;
    ctx->slabs[i].mem_type_idx = ctx->host_visible_type_idx;
    ctx->slabs[i].is_dev_local = 0;
    ctx->slabs[i].is_host_visible = 1;
    ctx->slabs[i].in_use = 1;
    ctx->slab_cunt++;

    return LDG_ERR_AOK;
}

uint32_t gpu_cmd_begin_oneshot(ldg_gpu_ctx_t *ctx, void *out_cmd)
{
    VkCommandBufferAllocateInfo alloc_info = { 0 };
    VkCommandBufferBeginInfo begin_info = { 0 };
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkCommandBuffer *out = (VkCommandBuffer *)out_cmd;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = VK_NULL_HANDLE;

    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = (VkCommandPool)ctx->cmd_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    if (LDG_UNLIKELY(vkAllocateCommandBuffers((VkDevice)ctx->dev, &alloc_info, &cmd) != VK_SUCCESS)) { return LDG_ERR_GPU_CMD_RECORD; }

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (LDG_UNLIKELY(vkBeginCommandBuffer(cmd, &begin_info) != VK_SUCCESS))
    {
        vkFreeCommandBuffers((VkDevice)ctx->dev, (VkCommandPool)ctx->cmd_pool, 1, &cmd);
        return LDG_ERR_GPU_CMD_RECORD;
    }

    *out = cmd;
    return LDG_ERR_AOK;
}

uint32_t gpu_cmd_submit_wait(ldg_gpu_ctx_t *ctx, void *cmd_handle)
{
    VkSubmitInfo submit_info = { 0 };
    VkFenceCreateInfo fence_info = { 0 };
    VkFence fence = VK_NULL_HANDLE;
    VkDevice dev = (VkDevice)ctx->dev;
    VkCommandBuffer cmd = (VkCommandBuffer)cmd_handle;
    VkResult res = VK_SUCCESS;

    if (LDG_UNLIKELY(vkEndCommandBuffer(cmd) != VK_SUCCESS)) { return LDG_ERR_GPU_CMD_RECORD; }

    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if (LDG_UNLIKELY(vkCreateFence(dev, &fence_info, 0x0, &fence) != VK_SUCCESS)) { return LDG_ERR_GPU_FENCE_CREATE; }

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    res = vkQueueSubmit((VkQueue)ctx->queue, 1, &submit_info, fence);
    if (LDG_UNLIKELY(res != VK_SUCCESS))
    {
        vkDestroyFence(dev, fence, 0x0);
        return LDG_ERR_GPU_SUBMIT;
    }

    res = vkWaitForFences(dev, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(dev, fence, 0x0);

    if (LDG_UNLIKELY(res != VK_SUCCESS)) { return LDG_ERR_GPU_FENCE_TIMEOUT; }

    return LDG_ERR_AOK;
}

uint32_t gpu_staging_transfer(ldg_gpu_ctx_t *ctx, uint32_t buff_idx, void *host_data, uint64_t size, uint64_t buff_offset, uint8_t to_dev)
{
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkBufferCopy region = { 0 };
    VkBuffer vk_buff = VK_NULL_HANDLE;
    VkBuffer staging = (VkBuffer)ctx->staging_buff;
    uint64_t remaining = size;
    uint64_t data_off = 0;
    uint64_t chunk = 0;
    uint32_t err = 0;

    vk_buff = (VkBuffer)ctx->buffs[buff_idx].vk_buff;

    while (remaining > 0)
    {
        chunk = remaining > ctx->staging_size ? ctx->staging_size : remaining;

        if (to_dev) { if (LDG_UNLIKELY(ldg_mem_secure_copy(ctx->staging_map, (const uint8_t *)host_data + data_off, (uint64_t)chunk) != LDG_ERR_AOK)) { return LDG_ERR_GPU_TRANSFER; } }

        err = gpu_cmd_begin_oneshot(ctx, &cmd);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        region.size = (VkDeviceSize)chunk;
        if (to_dev)
        {
            region.srcOffset = 0;
            region.dstOffset = (VkDeviceSize)(buff_offset + data_off);
            vkCmdCopyBuffer(cmd, staging, vk_buff, 1, &region);
        }
        else
        {
            region.srcOffset = (VkDeviceSize)(buff_offset + data_off);
            region.dstOffset = 0;
            vkCmdCopyBuffer(cmd, vk_buff, staging, 1, &region);
        }

        err = gpu_cmd_submit_wait(ctx, (void *)cmd);
        vkFreeCommandBuffers((VkDevice)ctx->dev, (VkCommandPool)ctx->cmd_pool, 1, &cmd);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (!to_dev) { if (LDG_UNLIKELY(ldg_mem_secure_copy((uint8_t *)host_data + data_off, ctx->staging_map, (uint64_t)chunk) != LDG_ERR_AOK)) { return LDG_ERR_GPU_TRANSFER; } }

        remaining -= chunk;
        data_off += chunk;
    }

    return LDG_ERR_AOK;
}

static uint32_t gpu_dispatch_prepare(ldg_gpu_ctx_t *ctx, const ldg_gpu_dispatch_desc_t *desc, VkDescriptorSet *out_ds, VkCommandBuffer *out_cmd)
{
    VkDescriptorSetAllocateInfo ds_alloc = { 0 };
    VkDescriptorBufferInfo buff_infos[LDG_GPU_BIND_MAX] = { { 0 } };
    VkWriteDescriptorSet wrs[LDG_GPU_BIND_MAX] = { { 0 } };
    VkCommandBufferAllocateInfo cmd_alloc = { 0 };
    VkCommandBufferBeginInfo cmd_begin = { 0 };
    VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
    VkDescriptorSet ds = VK_NULL_HANDLE;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkDevice dev = (VkDevice)ctx->dev;
    uint32_t pid = 0;
    uint32_t i = 0;

    pid = desc->pipeline_id;
    if (LDG_UNLIKELY(pid >= LDG_GPU_PIPELINE_POOL_MAX || !ctx->pipelines[pid].in_use)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->pipelines[pid].kind != LDG_GPU_PIPELINE_COMPUTE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (i = 0; i < desc->buff_cunt; i++) { if (LDG_UNLIKELY(desc->buff_ids[i] >= LDG_GPU_BUFF_MAX || !ctx->buffs[desc->buff_ids[i]].in_use)) { return LDG_ERR_FUNC_ARG_INVALID; } }

    dsl = (VkDescriptorSetLayout)ctx->desc_set_layout;
    ds_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ds_alloc.descriptorPool = (VkDescriptorPool)ctx->desc_pool;
    ds_alloc.descriptorSetCount = 1;
    ds_alloc.pSetLayouts = &dsl;

    if (LDG_UNLIKELY(vkAllocateDescriptorSets(dev, &ds_alloc, &ds) != VK_SUCCESS)) { return LDG_ERR_GPU_DESC_ALLOC; }

    for (i = 0; i < desc->buff_cunt; i++)
    {
        buff_infos[i].buffer = (VkBuffer)ctx->buffs[desc->buff_ids[i]].vk_buff;
        buff_infos[i].offset = 0;
        buff_infos[i].range = VK_WHOLE_SIZE;

        wrs[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wrs[i].dstSet = ds;
        wrs[i].dstBinding = i;
        wrs[i].descriptorCount = 1;
        wrs[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        wrs[i].pBufferInfo = &buff_infos[i];
    }

    vkUpdateDescriptorSets(dev, desc->buff_cunt, wrs, 0, 0x0);

    cmd_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc.commandPool = (VkCommandPool)ctx->cmd_pool;
    cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc.commandBufferCount = 1;

    if (LDG_UNLIKELY(vkAllocateCommandBuffers(dev, &cmd_alloc, &cmd) != VK_SUCCESS))
    {
        vkFreeDescriptorSets(dev, (VkDescriptorPool)ctx->desc_pool, 1, &ds);
        return LDG_ERR_GPU_CMD_RECORD;
    }

    cmd_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (LDG_UNLIKELY(vkBeginCommandBuffer(cmd, &cmd_begin) != VK_SUCCESS))
    {
        vkFreeCommandBuffers(dev, (VkCommandPool)ctx->cmd_pool, 1, &cmd);
        vkFreeDescriptorSets(dev, (VkDescriptorPool)ctx->desc_pool, 1, &ds);
        return LDG_ERR_GPU_CMD_RECORD;
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipeline)ctx->pipelines[pid].pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipelineLayout)ctx->pipelines[pid].layout, 0, 1, &ds, 0, 0x0);
    vkCmdDispatch(cmd, desc->group_cunt_x, desc->group_cunt_y, desc->group_cunt_z);

    if (LDG_UNLIKELY(vkEndCommandBuffer(cmd) != VK_SUCCESS))
    {
        vkFreeCommandBuffers(dev, (VkCommandPool)ctx->cmd_pool, 1, &cmd);
        vkFreeDescriptorSets(dev, (VkDescriptorPool)ctx->desc_pool, 1, &ds);
        return LDG_ERR_GPU_CMD_RECORD;
    }

    *out_ds = ds;
    *out_cmd = cmd;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_dev_list(ldg_gpu_dev_info_t **devs, uint32_t *cunt)
{
    VkInstance tmp_instance = VK_NULL_HANDLE;
    VkApplicationInfo app_info = { 0 };
    VkInstanceCreateInfo create_info = { 0 };
    VkPhysicalDevice phys_devs[LDG_GPU_DEV_ENUM_MAX] = { 0 };
    VkPhysicalDeviceProperties *props = 0x0;
    VkPhysicalDeviceMemoryProperties *mem_props = 0x0;
    ldg_mem_pool_t *scratch = 0x0;
    ldg_gpu_dev_info_t *out = 0x0;
    uint32_t dev_cunt = 0;
    uint32_t err = 0;
    uint32_t i = 0;
    uint32_t h = 0;

    if (LDG_UNLIKELY(!devs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *devs = 0x0;
    *cunt = 0;

    err = ldg_mem_pool_create(0, 2048, &scratch);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkPhysicalDeviceProperties), (void **)&props);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ldg_mem_pool_destroy(&scratch);
        return err;
    }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkPhysicalDeviceMemoryProperties), (void **)&mem_props);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ldg_mem_pool_destroy(&scratch);
        return err;
    }

    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.apiVersion = VK_API_VERSION_1_2;

    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    if (LDG_UNLIKELY(vkCreateInstance(&create_info, 0x0, &tmp_instance) != VK_SUCCESS))
    {
        ldg_mem_pool_destroy(&scratch);
        return LDG_ERR_GPU_INIT;
    }

    dev_cunt = LDG_GPU_DEV_ENUM_MAX;
    if (LDG_UNLIKELY(vkEnumeratePhysicalDevices(tmp_instance, &dev_cunt, phys_devs) < 0))
    {
        vkDestroyInstance(tmp_instance, 0x0);
        ldg_mem_pool_destroy(&scratch);
        return LDG_ERR_GPU_DEV_NOT_FOUND;
    }

    if (dev_cunt == 0)
    {
        vkDestroyInstance(tmp_instance, 0x0);
        ldg_mem_pool_destroy(&scratch);
        return LDG_ERR_GPU_DEV_NOT_FOUND;
    }

    err = ldg_mem_alloc((uint64_t)(dev_cunt * sizeof(ldg_gpu_dev_info_t)), (void **)&out);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        vkDestroyInstance(tmp_instance, 0x0);
        ldg_mem_pool_destroy(&scratch);
        return err;
    }

    for (i = 0; i < dev_cunt; i++) { out[i] = (ldg_gpu_dev_info_t)LDG_STRUCT_ZERO_INIT; }

    for (i = 0; i < dev_cunt; i++)
    {
        *props = (VkPhysicalDeviceProperties)LDG_STRUCT_ZERO_INIT;
        *mem_props = (VkPhysicalDeviceMemoryProperties)LDG_STRUCT_ZERO_INIT;

        vkGetPhysicalDeviceProperties(phys_devs[i], props);
        vkGetPhysicalDeviceMemoryProperties(phys_devs[i], mem_props);

        out[i].dev_id = i;
        out[i].vendor_id = props->vendorID;
        out[i].api_ver = props->apiVersion;
        out[i].driver_ver = props->driverVersion;
        out[i].is_discrete = (props->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? 1 : 0;

        ldg_strrbrcpy(out[i].name, props->deviceName, LDG_GPU_NAME_MAX);

        for (h = 0; h < mem_props->memoryHeapCount; h++) { if (mem_props->memoryHeaps[h].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) { out[i].vram_size += mem_props->memoryHeaps[h].size; }
            else { out[i].host_visible_size += mem_props->memoryHeaps[h].size; } }
    }

    *devs = out;
    *cunt = dev_cunt;

    vkDestroyInstance(tmp_instance, 0x0);
    ldg_mem_pool_destroy(&scratch);
    return LDG_ERR_AOK;
}

LDG_EXPORT void ldg_gpu_dev_free(ldg_gpu_dev_info_t *devs)
{
    if (!devs) { return; }

    ldg_mem_dealloc(devs);
}

LDG_EXPORT uint32_t ldg_gpu_init(const ldg_gpu_init_desc_t *desc, void **out)
{
    ldg_gpu_ctx_t *ctx = 0x0;
    VkApplicationInfo app_info = { 0 };
    VkInstanceCreateInfo inst_info = { 0 };
    VkDebugUtilsMessengerCreateInfoEXT dbg_info = { 0 };
    VkDeviceQueueCreateInfo queue_info = { 0 };
    VkDeviceCreateInfo dev_info = { 0 };
    VkCommandPoolCreateInfo pool_info = { 0 };
    VkDescriptorBindingFlags binding_flags[LDG_GPU_BIND_MAX] = { 0 };
    VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info = { 0 };
    VkDescriptorSetLayoutCreateInfo dsl_info = { 0 };
    VkDescriptorPoolSize dp_size = { 0 };
    VkDescriptorPoolCreateInfo dp_info = { 0 };
    VkMemoryAllocateInfo slab_alloc = { 0 };
    VkBufferCreateInfo staging_buff_info = { 0 };
    VkMemoryRequirements staging_reqs = { 0 };
    VkMemoryAllocateInfo staging_alloc = { 0 };
    VkPipelineLayoutCreateInfo gfx_layout_info = { 0 };
    VkPushConstantRange push_range = { 0 };
    VkPhysicalDevice *phys_devs = 0x0;
    VkPhysicalDeviceProperties *dev_props = 0x0;
    VkPhysicalDeviceMemoryProperties *mem_props = 0x0;
    VkQueueFamilyProperties *queue_fam_props = 0x0;
    VkPhysicalDeviceVulkan12Features *vk12_feat = 0x0;
    VkDescriptorSetLayoutBinding *bindings = 0x0;
    ldg_mem_pool_t *scratch = 0x0;
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice dev = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
    VkDescriptorPool dp = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT dbg_messenger = VK_NULL_HANDLE;
    VkBuffer staging_buff = VK_NULL_HANDLE;
    VkDeviceMemory staging_mem = VK_NULL_HANDLE;
    VkDeviceMemory slab_mem = VK_NULL_HANDLE;
    VkPipelineLayout gfx_layout = VK_NULL_HANDLE;
    void *staging_map = 0x0;
    float queue_priority = 1.0f;
    const char *validation_layer = "VK_LAYER_KHRONOS_validation";
    const char *debug_ext = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    const char *swapchain_ext = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    const char **all_inst_exts = 0x0;
    uint32_t all_inst_ext_cunt = 0;
    uint32_t dev_cunt = 0;
    uint32_t queue_fam_cunt = 0;
    uint32_t slab_cunt = 0;
    uint64_t slab_size = 0;
    uint32_t dev_idx = 0;
    uint32_t selected_idx = UINT32_MAX;
    uint32_t compute_fam_idx = UINT32_MAX;
    uint32_t gfx_fam_idx = UINT32_MAX;
    uint8_t enable_validation = 0;
    uint8_t dev_local_found = 0;
    uint8_t host_visible_found = 0;
    uint32_t i = 0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    err = ldg_mem_alloc(sizeof(ldg_gpu_ctx_t), (void **)&ctx);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    *ctx = (ldg_gpu_ctx_t)LDG_STRUCT_ZERO_INIT;

    ctx->gfx_queue_family_idx = UINT32_MAX;

    err = ldg_mem_pool_create(0, 4096, &scratch);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkPhysicalDevice) * LDG_GPU_DEV_ENUM_MAX, (void **)&phys_devs);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ldg_mem_pool_destroy(&scratch);
        return err;
    }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkPhysicalDeviceProperties), (void **)&dev_props);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ldg_mem_pool_destroy(&scratch);
        return err;
    }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkPhysicalDeviceMemoryProperties), (void **)&mem_props);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ldg_mem_pool_destroy(&scratch);
        return err;
    }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkQueueFamilyProperties) * 32, (void **)&queue_fam_props);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ldg_mem_pool_destroy(&scratch);
        return err;
    }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkPhysicalDeviceVulkan12Features), (void **)&vk12_feat);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ldg_mem_pool_destroy(&scratch);
        return err;
    }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkDescriptorSetLayoutBinding) * LDG_GPU_BIND_MAX, (void **)&bindings);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ldg_mem_pool_destroy(&scratch);
        return err;
    }

    for (i = 0; i < LDG_GPU_DEV_ENUM_MAX; i++) { phys_devs[i] = VK_NULL_HANDLE; }

    *vk12_feat = (VkPhysicalDeviceVulkan12Features)LDG_STRUCT_ZERO_INIT;

    for (i = 0; i < LDG_GPU_BIND_MAX; i++) { bindings[i] = (VkDescriptorSetLayoutBinding)LDG_STRUCT_ZERO_INIT; }

    ctx->flags = desc->flags;
    slab_cunt = desc->slab_cunt > 0 ? desc->slab_cunt : LDG_GPU_DEFAULT_SLAB_CUNT;
    slab_size = desc->slab_size > 0 ? desc->slab_size : LDG_GPU_DEFAULT_SLAB_SIZE;
    if (slab_cunt > LDG_GPU_MEM_SLAB_MAX) { slab_cunt = LDG_GPU_MEM_SLAB_MAX; }

#ifdef STD_DEBUG
    enable_validation = 1;
#endif
    if (desc->flags & LDG_GPU_FLAG_VALIDATION) { enable_validation = 1; }

    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "ldg_gpu";
    app_info.apiVersion = VK_API_VERSION_1_2;

    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pApplicationInfo = &app_info;

    all_inst_ext_cunt = desc->instance_extension_cunt + (enable_validation ? 1 : 0);
    if (all_inst_ext_cunt > 0)
    {
        err = ldg_mem_pool_alloc(scratch, sizeof(const char *) * (all_inst_ext_cunt + 1), (void **)&all_inst_exts);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            ldg_mem_pool_destroy(&scratch);
            return err;
        }

        for (i = 0; i < desc->instance_extension_cunt; i++) { all_inst_exts[i] = desc->instance_extensions[i]; }

        if (enable_validation) { all_inst_exts[desc->instance_extension_cunt] = debug_ext; }

        inst_info.enabledExtensionCount = all_inst_ext_cunt;
        inst_info.ppEnabledExtensionNames = all_inst_exts;
    }

    if (enable_validation)
    {
        inst_info.enabledLayerCount = 1;
        inst_info.ppEnabledLayerNames = &validation_layer;
    }

    if (vkCreateInstance(&inst_info, 0x0, &instance) != VK_SUCCESS)
    {
        if (enable_validation)
        {
            enable_validation = 0;
            inst_info.enabledLayerCount = 0;
            inst_info.ppEnabledLayerNames = 0x0;

            if (all_inst_ext_cunt > 1)
            {
                all_inst_ext_cunt--;
                inst_info.enabledExtensionCount = all_inst_ext_cunt;
            }
            else if (all_inst_ext_cunt == 1 && desc->instance_extension_cunt == 0)
            {
                all_inst_ext_cunt = 0;
                inst_info.enabledExtensionCount = 0;
                inst_info.ppEnabledExtensionNames = 0x0;
            }

            if (LDG_UNLIKELY(vkCreateInstance(&inst_info, 0x0, &instance) != VK_SUCCESS))
            {
                ldg_mem_pool_destroy(&scratch);
                return LDG_ERR_GPU_INIT;
            }

            LDG_ERRLOG_WARN("gpu: validation layer unavailable; continuing without");
        }
        else
        {
            ldg_mem_pool_destroy(&scratch);
            return LDG_ERR_GPU_INIT;
        }
    }

    ctx->instance = (void *)instance;

    if (enable_validation)
    {
        PFN_vkCreateDebugUtilsMessengerEXT create_dbg_fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (create_dbg_fn)
        {
            dbg_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            dbg_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
            dbg_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            dbg_info.pfnUserCallback = gpu_debug_msg_cb;
            if (create_dbg_fn(instance, &dbg_info, 0x0, &dbg_messenger) == VK_SUCCESS) { ctx->debug_messenger = (void *)dbg_messenger; }
        }
    }

    dev_cunt = LDG_GPU_DEV_ENUM_MAX;
    if (LDG_UNLIKELY(vkEnumeratePhysicalDevices(instance, &dev_cunt, phys_devs) < 0 || dev_cunt == 0))
    {
        ldg_mem_pool_destroy(&scratch);
        gpu_shutdown_internal(ctx);
        return LDG_ERR_GPU_DEV_NOT_FOUND;
    }

    dev_idx = desc->dev_idx;
    if (dev_idx == UINT32_MAX)
    {
        for (i = 0; i < dev_cunt; i++)
        {
            *dev_props = (VkPhysicalDeviceProperties)LDG_STRUCT_ZERO_INIT;

            vkGetPhysicalDeviceProperties(phys_devs[i], dev_props);
            if (dev_props->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                selected_idx = i;
                break;
            }
        }
        if (selected_idx == UINT32_MAX) { selected_idx = 0; }
    }
    else
    {
        if (LDG_UNLIKELY(dev_idx >= dev_cunt))
        {
            ldg_mem_pool_destroy(&scratch);
            gpu_shutdown_internal(ctx);
            return LDG_ERR_GPU_DEV_NOT_FOUND;
        }

        selected_idx = dev_idx;
    }

    ctx->phys_dev = (void *)phys_devs[selected_idx];

    *dev_props = (VkPhysicalDeviceProperties)LDG_STRUCT_ZERO_INIT;

    vkGetPhysicalDeviceProperties(phys_devs[selected_idx], dev_props);
    ctx->min_storage_buff_offset_align = (uint64_t)dev_props->limits.minStorageBufferOffsetAlignment;
    if (ctx->min_storage_buff_offset_align == 0) { ctx->min_storage_buff_offset_align = 256; }

    *mem_props = (VkPhysicalDeviceMemoryProperties)LDG_STRUCT_ZERO_INIT;

    vkGetPhysicalDeviceMemoryProperties(phys_devs[selected_idx], mem_props);

    for (i = 0; i < mem_props->memoryTypeCount; i++)
    {
        VkMemoryPropertyFlags mflags = mem_props->memoryTypes[i].propertyFlags;

        if ((mflags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !dev_local_found)
        {
            ctx->dev_local_type_idx = i;
            dev_local_found = 1;
        }

        if ((mflags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && !host_visible_found)
        {
            ctx->host_visible_type_idx = i;
            host_visible_found = 1;
        }
    }

    if (LDG_UNLIKELY(!dev_local_found || !host_visible_found))
    {
        ldg_mem_pool_destroy(&scratch);
        gpu_shutdown_internal(ctx);
        return LDG_ERR_GPU_MEM_ALLOC;
    }

    {
        VkMemoryPropertyFlags dl_flags = mem_props->memoryTypes[ctx->dev_local_type_idx].propertyFlags;
        if (dl_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) { ctx->has_unified_mem = 1; }
    }

    for (i = 0; i < mem_props->memoryHeapCount; i++) { if (mem_props->memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) { ctx->vram_total += mem_props->memoryHeaps[i].size; }
        else { ctx->host_total += mem_props->memoryHeaps[i].size; } }

    queue_fam_cunt = 32;
    vkGetPhysicalDeviceQueueFamilyProperties(phys_devs[selected_idx], &queue_fam_cunt, queue_fam_props);

    for (i = 0; i < queue_fam_cunt; i++) { if ((queue_fam_props[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) == (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))
        {
            gfx_fam_idx = i;
            compute_fam_idx = i;
            break;
        }
    }

    if (compute_fam_idx == UINT32_MAX) { for (i = 0; i < queue_fam_cunt; i++) { if (queue_fam_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                compute_fam_idx = i;
                break;
            }
        } }

    if (LDG_UNLIKELY(compute_fam_idx == UINT32_MAX))
    {
        ldg_mem_pool_destroy(&scratch);
        gpu_shutdown_internal(ctx);
        return LDG_ERR_GPU_QUEUE_NOT_FOUND;
    }

    ctx->compute_queue_family_idx = compute_fam_idx;
    ctx->gfx_queue_family_idx = gfx_fam_idx;
    ctx->has_gfx = (gfx_fam_idx != UINT32_MAX) ? 1 : 0;

    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = compute_fam_idx;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    vk12_feat->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vk12_feat->descriptorBindingPartiallyBound = VK_TRUE;

    dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_info.pNext = vk12_feat;
    dev_info.queueCreateInfoCount = 1;
    dev_info.pQueueCreateInfos = &queue_info;

    if (ctx->has_gfx)
    {
        dev_info.enabledExtensionCount = 1;
        dev_info.ppEnabledExtensionNames = &swapchain_ext;
    }

    if (LDG_UNLIKELY(vkCreateDevice(phys_devs[selected_idx], &dev_info, 0x0, &dev) != VK_SUCCESS))
    {
        if (ctx->has_gfx)
        {
            dev_info.enabledExtensionCount = 0;
            dev_info.ppEnabledExtensionNames = 0x0;
            ctx->has_gfx = 0;
            ctx->has_swapchain_ext = 0;
            ctx->gfx_queue_family_idx = UINT32_MAX;

            if (LDG_UNLIKELY(vkCreateDevice(phys_devs[selected_idx], &dev_info, 0x0, &dev) != VK_SUCCESS))
            {
                ldg_mem_pool_destroy(&scratch);
                gpu_shutdown_internal(ctx);
                return LDG_ERR_GPU_INIT;
            }

            LDG_ERRLOG_WARN("gpu: VK_KHR_swapchain unavailable; gfx disabled");
        }
        else
        {
            ldg_mem_pool_destroy(&scratch);
            gpu_shutdown_internal(ctx);
            return LDG_ERR_GPU_INIT;
        }
    }
    else { if (ctx->has_gfx) { ctx->has_swapchain_ext = 1; } }

    ctx->dev = (void *)dev;

    vkGetDeviceQueue(dev, compute_fam_idx, 0, &queue);
    ctx->queue = (void *)queue;

    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = compute_fam_idx;

    if (LDG_UNLIKELY(vkCreateCommandPool(dev, &pool_info, 0x0, &cmd_pool) != VK_SUCCESS))
    {
        ldg_mem_pool_destroy(&scratch);
        gpu_shutdown_internal(ctx);
        return LDG_ERR_GPU_INIT;
    }

    ctx->cmd_pool = (void *)cmd_pool;

    for (i = 0; i < LDG_GPU_BIND_MAX; i++)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
        binding_flags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    }

    binding_flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    binding_flags_info.bindingCount = LDG_GPU_BIND_MAX;
    binding_flags_info.pBindingFlags = binding_flags;

    dsl_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_info.pNext = &binding_flags_info;
    dsl_info.bindingCount = LDG_GPU_BIND_MAX;
    dsl_info.pBindings = bindings;

    if (LDG_UNLIKELY(vkCreateDescriptorSetLayout(dev, &dsl_info, 0x0, &dsl) != VK_SUCCESS))
    {
        ldg_mem_pool_destroy(&scratch);
        gpu_shutdown_internal(ctx);
        return LDG_ERR_GPU_INIT;
    }

    ldg_mem_pool_destroy(&scratch);

    ctx->desc_set_layout = (void *)dsl;

    dp_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    dp_size.descriptorCount = (uint32_t)(LDG_GPU_BIND_MAX * (LDG_GPU_FENCE_MAX + LDG_GPU_FRAME_IN_FLIGHT + 1));

    dp_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dp_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    dp_info.maxSets = LDG_GPU_FENCE_MAX + LDG_GPU_FRAME_IN_FLIGHT + 1;
    dp_info.poolSizeCount = 1;
    dp_info.pPoolSizes = &dp_size;

    if (LDG_UNLIKELY(vkCreateDescriptorPool(dev, &dp_info, 0x0, &dp) != VK_SUCCESS))
    {
        gpu_shutdown_internal(ctx);
        return LDG_ERR_GPU_INIT;
    }

    ctx->desc_pool = (void *)dp;

    if (ctx->has_gfx)
    {
        push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        push_range.offset = 0;
        push_range.size = LDG_GPU_PUSH_CONST_SIZE;

        gfx_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        gfx_layout_info.setLayoutCount = 1;
        gfx_layout_info.pSetLayouts = &dsl;
        gfx_layout_info.pushConstantRangeCount = 1;
        gfx_layout_info.pPushConstantRanges = &push_range;

        if (LDG_UNLIKELY(vkCreatePipelineLayout(dev, &gfx_layout_info, 0x0, &gfx_layout) != VK_SUCCESS))
        {
            gpu_shutdown_internal(ctx);
            return LDG_ERR_GPU_INIT;
        }

        ctx->gfx_pipeline_layout = (void *)gfx_layout;
    }

    for (i = 0; i < slab_cunt; i++)
    {
        slab_mem = VK_NULL_HANDLE;
        slab_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        slab_alloc.allocationSize = (VkDeviceSize)slab_size;
        slab_alloc.memoryTypeIndex = ctx->dev_local_type_idx;

        if (LDG_UNLIKELY(vkAllocateMemory(dev, &slab_alloc, 0x0, &slab_mem) != VK_SUCCESS))
        {
            gpu_shutdown_internal(ctx);
            return LDG_ERR_GPU_MEM_ALLOC;
        }

        ctx->slabs[i].mem = (void *)slab_mem;
        ctx->slabs[i].size = slab_size;
        ctx->slabs[i].offset = 0;
        ctx->slabs[i].mem_type_idx = ctx->dev_local_type_idx;
        ctx->slabs[i].is_dev_local = 1;
        ctx->slabs[i].is_host_visible = ctx->has_unified_mem;
        ctx->slabs[i].in_use = 1;
        ctx->slab_cunt++;
        ctx->vram_used += slab_size;
    }

    staging_buff_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    staging_buff_info.size = (VkDeviceSize)LDG_GPU_STAGING_SIZE;
    staging_buff_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    staging_buff_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (LDG_UNLIKELY(vkCreateBuffer(dev, &staging_buff_info, 0x0, &staging_buff) != VK_SUCCESS))
    {
        gpu_shutdown_internal(ctx);
        return LDG_ERR_GPU_INIT;
    }

    ctx->staging_buff = (void *)staging_buff;

    vkGetBufferMemoryRequirements(dev, staging_buff, &staging_reqs);

    staging_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    staging_alloc.allocationSize = staging_reqs.size;
    staging_alloc.memoryTypeIndex = ctx->host_visible_type_idx;

    if (LDG_UNLIKELY(vkAllocateMemory(dev, &staging_alloc, 0x0, &staging_mem) != VK_SUCCESS))
    {
        gpu_shutdown_internal(ctx);
        return LDG_ERR_GPU_MEM_ALLOC;
    }

    ctx->staging_mem = (void *)staging_mem;

    if (LDG_UNLIKELY(vkBindBufferMemory(dev, staging_buff, staging_mem, 0) != VK_SUCCESS))
    {
        gpu_shutdown_internal(ctx);
        return LDG_ERR_GPU_MEM_BIND;
    }

    if (LDG_UNLIKELY(vkMapMemory(dev, staging_mem, 0, staging_reqs.size, 0, &staging_map) != VK_SUCCESS))
    {
        gpu_shutdown_internal(ctx);
        return LDG_ERR_GPU_BUFF_MAP;
    }

    ctx->staging_map = staging_map;
    ctx->staging_size = LDG_GPU_STAGING_SIZE;

    err = ldg_mut_init(&ctx->mut, 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        gpu_shutdown_internal(ctx);
        return LDG_ERR_GPU_INIT;
    }

    ctx->mut_init = 1;

    ctx->is_init = 1;
    *out = ctx;
    return LDG_ERR_AOK;
}

LDG_EXPORT void ldg_gpu_shutdown(void *vk)
{
    ldg_gpu_ctx_t *ctx = vk;

    if (!ctx) { return; }

    if (!ctx->is_init) { return; }

    gpu_shutdown_internal(ctx);
    ldg_mem_dealloc(ctx);
}

LDG_EXPORT uint32_t ldg_gpu_instance_get(void *vk, void **out)
{
    ldg_gpu_ctx_t *ctx = vk;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    *out = ctx->instance;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_buff_create(void *vk, const ldg_gpu_buff_desc_t *desc, ldg_gpu_buff_t *out)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkBufferCreateInfo buff_info = { 0 };
    VkMemoryRequirements mem_reqs = { 0 };
    VkBuffer vk_buff = VK_NULL_HANDLE;
    VkDevice dev = VK_NULL_HANDLE;
    uint32_t slot = UINT32_MAX;
    uint32_t slab_idx = UINT32_MAX;
    uint64_t slab_off = 0;
    uint8_t want_dev_local = 1;
    uint8_t spilled = 0;
    uint32_t err = 0;
    uint32_t ret = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = (ldg_gpu_buff_t)LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(desc->size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dev = (VkDevice)ctx->dev;
    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (i = 0; i < LDG_GPU_BUFF_MAX; i++) { if (!ctx->buffs[i].in_use)
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

    buff_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buff_info.size = (VkDeviceSize)desc->size;
    buff_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    buff_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (LDG_UNLIKELY(vkCreateBuffer(dev, &buff_info, 0x0, &vk_buff) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_BUFF_CREATE;
    }

    vkGetBufferMemoryRequirements(dev, vk_buff, &mem_reqs);

    want_dev_local = (desc->mem_flags & LDG_GPU_MEM_HOST_VISIBLE) ? 0 : 1;

    err = gpu_slab_alloc(ctx, want_dev_local, mem_reqs.size, mem_reqs.alignment, (uint32_t)mem_reqs.memoryTypeBits, &slab_idx, &slab_off);

    if (err != LDG_ERR_AOK && want_dev_local && (ctx->flags & LDG_GPU_FLAG_SPILL_ENABLE))
    {
        LDG_ERRLOG_WARN("gpu: device-local exhausted; spilling to host-visible");
        err = gpu_slab_alloc(ctx, 0, mem_reqs.size, mem_reqs.alignment, (uint32_t)mem_reqs.memoryTypeBits, &slab_idx, &slab_off);
        if (err != LDG_ERR_AOK)
        {
            err = gpu_spill_slab_create(ctx, mem_reqs.size, (uint32_t)mem_reqs.memoryTypeBits);
            if (err == LDG_ERR_AOK) { err = gpu_slab_alloc(ctx, 0, mem_reqs.size, mem_reqs.alignment, (uint32_t)mem_reqs.memoryTypeBits, &slab_idx, &slab_off); }
        }

        if (err == LDG_ERR_AOK)
        {
            spilled = 1;
            ctx->spill_cunt++;
        }
    }

    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        vkDestroyBuffer(dev, vk_buff, 0x0);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return err;
    }

    if (LDG_UNLIKELY(vkBindBufferMemory(dev, vk_buff, (VkDeviceMemory)ctx->slabs[slab_idx].mem, (VkDeviceSize)slab_off) != VK_SUCCESS))
    {
        vkDestroyBuffer(dev, vk_buff, 0x0);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_MEM_BIND;
    }

    ctx->buffs[slot].vk_buff = (void *)vk_buff;
    ctx->buffs[slot].size = desc->size;
    ctx->buffs[slot].mem_offset = slab_off;
    ctx->buffs[slot].slab_idx = slab_idx;
    ctx->buffs[slot].mem_flags = desc->mem_flags;
    ctx->buffs[slot].in_use = 1;
    ctx->buffs[slot].spilled = spilled;
    ctx->buffs[slot].is_host_visible = ctx->slabs[slab_idx].is_host_visible;

    out->id = slot;
    out->mem_flags = desc->mem_flags;
    out->size = desc->size;
    out->spilled = spilled;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_buff_destroy(void *vk, uint32_t buff_id)
{
    ldg_gpu_ctx_t *ctx = vk;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(buff_id >= LDG_GPU_BUFF_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!ctx->buffs[buff_id].in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_NOT_FOUND;
    }

    if (ctx->buffs[buff_id].vk_buff) { vkDestroyBuffer((VkDevice)ctx->dev, (VkBuffer)ctx->buffs[buff_id].vk_buff, 0x0); }

    ctx->buffs[buff_id] = (ldg_gpu_buff_entry_t)LDG_STRUCT_ZERO_INIT;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_buff_wr(void *vk, uint32_t id, const void *data, uint64_t size, uint64_t offset)
{
    ldg_gpu_ctx_t *ctx = vk;
    void *mapped = 0x0;
    uint32_t err = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!data)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(id >= LDG_GPU_BUFF_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!ctx->buffs[id].in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_NOT_FOUND;
    }

    if (LDG_UNLIKELY(offset + size > ctx->buffs[id].size))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_BOUNDS;
    }

    if (ctx->buffs[id].is_host_visible)
    {
        if (LDG_UNLIKELY(vkMapMemory((VkDevice)ctx->dev, (VkDeviceMemory)ctx->slabs[ctx->buffs[id].slab_idx].mem, (VkDeviceSize)(ctx->buffs[id].mem_offset + offset), (VkDeviceSize)size, 0, &mapped) != VK_SUCCESS))
        {
            LDG_GPU_UNLOCK_OR_WARN(ctx);
            return LDG_ERR_GPU_BUFF_MAP;
        }

        if (LDG_UNLIKELY(ldg_mem_secure_copy(mapped, data, (uint64_t)size) != LDG_ERR_AOK))
        {
            vkUnmapMemory((VkDevice)ctx->dev, (VkDeviceMemory)ctx->slabs[ctx->buffs[id].slab_idx].mem);
            LDG_GPU_UNLOCK_OR_WARN(ctx);
            return LDG_ERR_GPU_TRANSFER;
        }

        vkUnmapMemory((VkDevice)ctx->dev, (VkDeviceMemory)ctx->slabs[ctx->buffs[id].slab_idx].mem);
    }
    else
    {
        err = gpu_staging_transfer(ctx, id, (void *)data, size, offset, 1);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            LDG_GPU_UNLOCK_OR_WARN(ctx);
            return err;
        }
    }

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_buff_rd(void *vk, uint32_t id, void *data, uint64_t size, uint64_t offset)
{
    ldg_gpu_ctx_t *ctx = vk;
    void *mapped = 0x0;
    uint32_t err = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!data)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(id >= LDG_GPU_BUFF_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!ctx->buffs[id].in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_NOT_FOUND;
    }

    if (LDG_UNLIKELY(offset + size > ctx->buffs[id].size))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_BOUNDS;
    }

    if (ctx->buffs[id].is_host_visible)
    {
        if (LDG_UNLIKELY(vkMapMemory((VkDevice)ctx->dev, (VkDeviceMemory)ctx->slabs[ctx->buffs[id].slab_idx].mem, (VkDeviceSize)(ctx->buffs[id].mem_offset + offset), (VkDeviceSize)size, 0, &mapped) != VK_SUCCESS))
        {
            LDG_GPU_UNLOCK_OR_WARN(ctx);
            return LDG_ERR_GPU_BUFF_MAP;
        }

        if (LDG_UNLIKELY(ldg_mem_secure_copy(data, mapped, (uint64_t)size) != LDG_ERR_AOK))
        {
            vkUnmapMemory((VkDevice)ctx->dev, (VkDeviceMemory)ctx->slabs[ctx->buffs[id].slab_idx].mem);
            LDG_GPU_UNLOCK_OR_WARN(ctx);
            return LDG_ERR_GPU_TRANSFER;
        }

        vkUnmapMemory((VkDevice)ctx->dev, (VkDeviceMemory)ctx->slabs[ctx->buffs[id].slab_idx].mem);
    }
    else
    {
        err = gpu_staging_transfer(ctx, id, data, size, offset, 0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            LDG_GPU_UNLOCK_OR_WARN(ctx);
            return err;
        }
    }

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_buff_fill(void *vk, uint32_t id, uint32_t val, uint64_t size, uint64_t offset)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    uint32_t err = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(id >= LDG_GPU_BUFF_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!ctx->buffs[id].in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_NOT_FOUND;
    }

    if (LDG_UNLIKELY(offset + size > ctx->buffs[id].size))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_BOUNDS;
    }

    if (LDG_UNLIKELY(offset % 4 != 0 || size % 4 != 0))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    err = gpu_cmd_begin_oneshot(ctx, &cmd);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return err;
    }

    vkCmdFillBuffer(cmd, (VkBuffer)ctx->buffs[id].vk_buff, (VkDeviceSize)offset, (VkDeviceSize)size, val);

    err = gpu_cmd_submit_wait(ctx, (void *)cmd);
    vkFreeCommandBuffers((VkDevice)ctx->dev, (VkCommandPool)ctx->cmd_pool, 1, &cmd);

    LDG_GPU_UNLOCK_OR_WARN(ctx);
    return err;
}

LDG_EXPORT uint32_t ldg_gpu_pipeline_create(void *vk, const ldg_gpu_spirv_desc_t *spirv, uint32_t *id)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkShaderModuleCreateInfo sm_info = { 0 };
    VkPipelineLayoutCreateInfo pl_info = { 0 };
    VkComputePipelineCreateInfo cp_info = { 0 };
    VkShaderModule shader_module = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
    VkDevice dev = VK_NULL_HANDLE;
    uint32_t slot = UINT32_MAX;
    uint32_t ret = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!spirv)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!id)) { return LDG_ERR_FUNC_ARG_NULL; }

    *id = UINT32_MAX;
    if (LDG_UNLIKELY(!spirv->code || spirv->code_size == 0)) { return LDG_ERR_GPU_SPIRV_INVALID; }

    if (LDG_UNLIKELY(!spirv->entry_name)) { return LDG_ERR_GPU_SPIRV_INVALID; }

    if (LDG_UNLIKELY(spirv->code_size % 4 != 0)) { return LDG_ERR_GPU_SPIRV_INVALID; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    dev = (VkDevice)ctx->dev;
    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (i = 0; i < LDG_GPU_PIPELINE_POOL_MAX; i++) { if (!ctx->pipelines[i].in_use)
        {
            slot = i;
            break;
        }
    }

    if (LDG_UNLIKELY(slot == UINT32_MAX))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_PIPELINE_FULL;
    }

    sm_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sm_info.codeSize = (uint64_t)spirv->code_size;
    sm_info.pCode = spirv->code;

    if (LDG_UNLIKELY(vkCreateShaderModule(dev, &sm_info, 0x0, &shader_module) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_PIPELINE_CREATE;
    }

    dsl = (VkDescriptorSetLayout)ctx->desc_set_layout;
    pl_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_info.setLayoutCount = 1;
    pl_info.pSetLayouts = &dsl;

    if (LDG_UNLIKELY(vkCreatePipelineLayout(dev, &pl_info, 0x0, &layout) != VK_SUCCESS))
    {
        vkDestroyShaderModule(dev, shader_module, 0x0);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_PIPELINE_CREATE;
    }

    cp_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cp_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    cp_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    cp_info.stage.module = shader_module;
    cp_info.stage.pName = spirv->entry_name;
    cp_info.layout = layout;

    if (LDG_UNLIKELY(vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &cp_info, 0x0, &pipeline) != VK_SUCCESS))
    {
        vkDestroyPipelineLayout(dev, layout, 0x0);
        vkDestroyShaderModule(dev, shader_module, 0x0);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_PIPELINE_CREATE;
    }

    ctx->pipelines[slot].vert_module = (void *)shader_module;
    ctx->pipelines[slot].frag_module = 0x0;
    ctx->pipelines[slot].layout = (void *)layout;
    ctx->pipelines[slot].pipeline = (void *)pipeline;
    ctx->pipelines[slot].renderpass = 0x0;
    ctx->pipelines[slot].kind = LDG_GPU_PIPELINE_COMPUTE;
    ctx->pipelines[slot].in_use = 1;

    *id = slot;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_pipeline_destroy(void *vk, uint32_t id)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkDevice dev = VK_NULL_HANDLE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(id >= LDG_GPU_PIPELINE_POOL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dev = (VkDevice)ctx->dev;
    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!ctx->pipelines[id].in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_NOT_FOUND;
    }

    if (ctx->pipelines[id].pipeline) { vkDestroyPipeline(dev, (VkPipeline)ctx->pipelines[id].pipeline, 0x0); }

    if (ctx->pipelines[id].kind == LDG_GPU_PIPELINE_COMPUTE && ctx->pipelines[id].layout) { vkDestroyPipelineLayout(dev, (VkPipelineLayout)ctx->pipelines[id].layout, 0x0); }

    if (ctx->pipelines[id].vert_module) { vkDestroyShaderModule(dev, (VkShaderModule)ctx->pipelines[id].vert_module, 0x0); }

    if (ctx->pipelines[id].frag_module) { vkDestroyShaderModule(dev, (VkShaderModule)ctx->pipelines[id].frag_module, 0x0); }

    ctx->pipelines[id] = (ldg_gpu_pipeline_entry_t)LDG_STRUCT_ZERO_INIT;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_dispatch(void *vk, const ldg_gpu_dispatch_desc_t *desc)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkSubmitInfo submit_info = { 0 };
    VkFenceCreateInfo fence_info = { 0 };
    VkDescriptorSet ds = VK_NULL_HANDLE;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkDevice dev = VK_NULL_HANDLE;
    VkResult res = VK_SUCCESS;
    uint32_t err = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    dev = (VkDevice)ctx->dev;
    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    err = gpu_dispatch_prepare(ctx, desc, &ds, &cmd);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return err;
    }

    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (LDG_UNLIKELY(vkCreateFence(dev, &fence_info, 0x0, &fence) != VK_SUCCESS))
    {
        vkFreeCommandBuffers(dev, (VkCommandPool)ctx->cmd_pool, 1, &cmd);
        vkFreeDescriptorSets(dev, (VkDescriptorPool)ctx->desc_pool, 1, &ds);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FENCE_CREATE;
    }

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    res = vkQueueSubmit((VkQueue)ctx->queue, 1, &submit_info, fence);
    if (LDG_UNLIKELY(res != VK_SUCCESS))
    {
        vkDestroyFence(dev, fence, 0x0);
        vkFreeCommandBuffers(dev, (VkCommandPool)ctx->cmd_pool, 1, &cmd);
        vkFreeDescriptorSets(dev, (VkDescriptorPool)ctx->desc_pool, 1, &ds);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SUBMIT;
    }

    res = vkWaitForFences(dev, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(dev, fence, 0x0);
    vkFreeCommandBuffers(dev, (VkCommandPool)ctx->cmd_pool, 1, &cmd);
    vkFreeDescriptorSets(dev, (VkDescriptorPool)ctx->desc_pool, 1, &ds);

    LDG_GPU_UNLOCK_OR_WARN(ctx);
    return (res == VK_SUCCESS) ? LDG_ERR_AOK : LDG_ERR_GPU_DISPATCH;
}

LDG_EXPORT uint32_t ldg_gpu_dispatch_async(void *vk, const ldg_gpu_dispatch_desc_t *desc, ldg_gpu_fence_t *fence_out)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkSubmitInfo submit_info = { 0 };
    VkFenceCreateInfo fence_info = { 0 };
    VkDescriptorSet ds = VK_NULL_HANDLE;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkDevice dev = VK_NULL_HANDLE;
    uint32_t slot = UINT32_MAX;
    uint32_t err = 0;
    uint32_t ret = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!fence_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *fence_out = (ldg_gpu_fence_t)LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    dev = (VkDevice)ctx->dev;
    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (i = 0; i < LDG_GPU_FENCE_MAX; i++) { if (!ctx->fences[i].in_use)
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

    err = gpu_dispatch_prepare(ctx, desc, &ds, &cmd);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return err;
    }

    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (LDG_UNLIKELY(vkCreateFence(dev, &fence_info, 0x0, &fence) != VK_SUCCESS))
    {
        vkFreeCommandBuffers(dev, (VkCommandPool)ctx->cmd_pool, 1, &cmd);
        vkFreeDescriptorSets(dev, (VkDescriptorPool)ctx->desc_pool, 1, &ds);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_FENCE_CREATE;
    }

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    if (LDG_UNLIKELY(vkQueueSubmit((VkQueue)ctx->queue, 1, &submit_info, fence) != VK_SUCCESS))
    {
        vkDestroyFence(dev, fence, 0x0);
        vkFreeCommandBuffers(dev, (VkCommandPool)ctx->cmd_pool, 1, &cmd);
        vkFreeDescriptorSets(dev, (VkDescriptorPool)ctx->desc_pool, 1, &ds);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_SUBMIT;
    }

    ctx->fences[slot].fence = (void *)fence;
    ctx->fences[slot].cmd_buff = (void *)cmd;
    ctx->fences[slot].desc_set = (void *)ds;
    ctx->fences[slot].in_use = 1;

    fence_out->id = slot;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_fence_wait(void *vk, ldg_gpu_fence_t *fence, uint64_t timeout_ms)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkFence vk_fence = VK_NULL_HANDLE;
    VkResult res = VK_SUCCESS;
    uint64_t timeout_ns = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!fence)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(fence->id >= LDG_GPU_FENCE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!ctx->fences[fence->id].in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_NOT_FOUND;
    }

    vk_fence = (VkFence)ctx->fences[fence->id].fence;
    timeout_ns = (timeout_ms == UINT64_MAX) ? UINT64_MAX : timeout_ms * LDG_NS_PER_MS;

    res = vkWaitForFences((VkDevice)ctx->dev, 1, &vk_fence, VK_TRUE, timeout_ns);

    LDG_GPU_UNLOCK_OR_WARN(ctx);
    return (res == VK_SUCCESS) ? LDG_ERR_AOK : LDG_ERR_GPU_FENCE_TIMEOUT;
}

LDG_EXPORT uint32_t ldg_gpu_fence_poll(void *vk, ldg_gpu_fence_t *fence, uint8_t *ready)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkFence vk_fence = VK_NULL_HANDLE;
    VkResult res = VK_SUCCESS;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!fence)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ready)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ready = 0;
    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(fence->id >= LDG_GPU_FENCE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!ctx->fences[fence->id].in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_NOT_FOUND;
    }

    vk_fence = (VkFence)ctx->fences[fence->id].fence;
    res = vkGetFenceStatus((VkDevice)ctx->dev, vk_fence);
    *ready = (res == VK_SUCCESS) ? 1 : 0;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_fence_destroy(void *vk, ldg_gpu_fence_t *fence)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkDevice dev = VK_NULL_HANDLE;
    VkFence vk_fence = VK_NULL_HANDLE;
    uint32_t fid = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!fence)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(fence->id >= LDG_GPU_FENCE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dev = (VkDevice)ctx->dev;
    fid = fence->id;
    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!ctx->fences[fid].in_use))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_NOT_FOUND;
    }

    vk_fence = (VkFence)ctx->fences[fid].fence;
    if (vk_fence) { vkWaitForFences(dev, 1, &vk_fence, VK_TRUE, UINT64_MAX); }

    if (ctx->fences[fid].cmd_buff) { vkFreeCommandBuffers(dev, (VkCommandPool)ctx->cmd_pool, 1, (VkCommandBuffer *)&ctx->fences[fid].cmd_buff); }

    if (ctx->fences[fid].desc_set) { vkFreeDescriptorSets(dev, (VkDescriptorPool)ctx->desc_pool, 1, (VkDescriptorSet *)&ctx->fences[fid].desc_set); }

    if (vk_fence) { vkDestroyFence(dev, vk_fence, 0x0); }

    ctx->fences[fid] = (ldg_gpu_fence_entry_t)LDG_STRUCT_ZERO_INIT;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_mem_stats_get(void *vk, ldg_gpu_mem_stats_t *stats)
{
    ldg_gpu_ctx_t *ctx = vk;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_FUNC_ARG_NULL; }

    *stats = (ldg_gpu_mem_stats_t)LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    stats->vram_total = ctx->vram_total;
    stats->vram_used = ctx->vram_used;
    stats->host_total = ctx->host_total;
    stats->host_used = ctx->host_used;
    stats->slab_cunt = ctx->slab_cunt;
    stats->spill_cunt = ctx->spill_cunt;

    return ldg_mut_unlock(&ctx->mut);
}

LDG_EXPORT uint32_t ldg_gpu_spirv_file_load(const char *path, uint32_t **code, uint64_t *size)
{
    ldg_io_file_t *f = 0x0;
    ldg_io_stat_t st = { 0 };
    uint32_t *buff = 0x0;
    uint64_t file_size = 0;
    uint64_t bytes_rd = 0;
    uint32_t close_err = 0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!code)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!size)) { return LDG_ERR_FUNC_ARG_NULL; }

    *code = 0x0;
    *size = 0;

    err = ldg_io_file_stat(path, &st);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_GPU_SPIRV_RD; }

    file_size = st.size;
    if (LDG_UNLIKELY(file_size == 0 || file_size % 4 != 0)) { return LDG_ERR_GPU_SPIRV_INVALID; }

    err = ldg_mem_alloc(file_size, (void **)&buff);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_io_file_open(path, LDG_IO_RDONLY, 0, &f);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(buff);
        return LDG_ERR_GPU_SPIRV_RD;
    }

    err = ldg_io_file_rd(f, buff, file_size, &bytes_rd);
    close_err = ldg_io_file_close(f);

    if (LDG_UNLIKELY(err != LDG_ERR_AOK || bytes_rd != file_size || close_err != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(buff);
        return LDG_ERR_GPU_SPIRV_RD;
    }

    *code = buff;
    *size = file_size;
    return LDG_ERR_AOK;
}

LDG_EXPORT void ldg_gpu_spirv_file_free(uint32_t *code)
{
    if (!code) { return; }

    ldg_mem_dealloc(code);
}

#endif
