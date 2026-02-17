#ifdef LDG_GPU_VULKAN

#include <string.h>
#include <vulkan/vulkan.h>

#include <dangling/gpu/gpu.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/thread/sync.h>
#include <dangling/io/file.h>
#include <dangling/str/str.h>

#define LDG_GPU_DEV_ENUM_MAX 16
#define LDG_GPU_BUFF_MAX 256
#define LDG_GPU_FENCE_MAX 64
#define LDG_GPU_STAGING_SIZE (4 * LDG_MIB)
#define LDG_GPU_DEFAULT_SLAB_CUNT 4
#define LDG_GPU_DEFAULT_SLAB_SIZE (64 * LDG_MIB)

#define GPU_ALIGN_UP(x, a) (((uint64_t)(x) + ((uint64_t)(a) - 1)) & ~((uint64_t)(a) - 1))

typedef struct ldg_gpu_slab
{
    void *mem;
    uint64_t size;
    uint64_t offset;
    uint32_t mem_type_idx;
    uint8_t is_device_local;
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
    void *shader_module;
    void *layout;
    void *pipeline;
    uint8_t in_use;
    uint8_t pudding[7];
} ldg_gpu_pipeline_entry_t;

typedef struct ldg_gpu_fence_entry
{
    void *fence;
    void *cmd_buff;
    void *desc_set;
    uint8_t in_use;
    uint8_t pudding[7];
} ldg_gpu_fence_entry_t;

typedef struct ldg_gpu_ctx
{
    void *instance;
    void *phys_dev;
    void *device;
    void *queue;
    void *cmd_pool;
    void *desc_pool;
    void *desc_set_layout;
    void *debug_messenger;
    void *staging_buff;
    void *staging_mem;
    void *staging_map;
    uint64_t staging_size;
    uint32_t device_local_type_idx;
    uint32_t host_visible_type_idx;
    uint64_t min_storage_buff_offset_align;
    uint64_t vram_total;
    uint64_t vram_used;
    uint64_t host_total;
    uint64_t host_used;
    uint32_t spill_cunt;
    uint32_t compute_queue_family_idx;
    uint32_t flags;
    uint32_t slab_cunt;
    uint8_t has_unified_mem;
    uint8_t mut_init;
    uint8_t pudding_ctx[6];
    ldg_gpu_slab_t slabs[LDG_GPU_MEM_SLAB_MAX];
    ldg_gpu_buff_entry_t buffs[LDG_GPU_BUFF_MAX];
    ldg_gpu_pipeline_entry_t pipelines[LDG_GPU_PIPELINE_POOL_MAX];
    ldg_gpu_fence_entry_t fences[LDG_GPU_FENCE_MAX];
    ldg_mut_t mut;
    volatile uint8_t is_init;
    uint8_t pudding[7];
} LDG_ALIGNED ldg_gpu_ctx_t;

// singleton gpu backend; file-scope static required for Vulkan debug messenger callback
static ldg_gpu_ctx_t g_gpu_ctx = { 0 };

static VkBool32 VKAPI_PTR gpu_debug_msg_cb(VkDebugUtilsMessageSeverityFlagBitsEXT severity, __attribute__((unused)) VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT *data, __attribute__((unused)) void *user_data)
{
    if (!data || !data->pMessage) { return VK_FALSE; }

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) { LDG_ERRLOG_ERR(data->pMessage); }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) { LDG_ERRLOG_WARN(data->pMessage); }
    else { LDG_ERRLOG_INFO(data->pMessage); }

    return VK_FALSE;
}

static void gpu_shutdown_internal(void)
{
    VkDevice device = (VkDevice)g_gpu_ctx.device;
    VkInstance instance = (VkInstance)g_gpu_ctx.instance;
    uint32_t i = 0;

    if (device) { vkDeviceWaitIdle(device); }

    for (i = 0; i < LDG_GPU_FENCE_MAX; i++)
    {
        if (!g_gpu_ctx.fences[i].in_use) { continue; }

        if (g_gpu_ctx.fences[i].fence) { vkDestroyFence(device, (VkFence)g_gpu_ctx.fences[i].fence, 0x0); }

        if (g_gpu_ctx.fences[i].cmd_buff) { vkFreeCommandBuffers(device, (VkCommandPool)g_gpu_ctx.cmd_pool, 1, (VkCommandBuffer *)&g_gpu_ctx.fences[i].cmd_buff); }

        if (g_gpu_ctx.fences[i].desc_set && g_gpu_ctx.desc_pool) { vkFreeDescriptorSets(device, (VkDescriptorPool)g_gpu_ctx.desc_pool, 1, (VkDescriptorSet *)&g_gpu_ctx.fences[i].desc_set); }
    }

    for (i = 0; i < LDG_GPU_PIPELINE_POOL_MAX; i++)
    {
        if (!g_gpu_ctx.pipelines[i].in_use) { continue; }

        if (g_gpu_ctx.pipelines[i].pipeline) { vkDestroyPipeline(device, (VkPipeline)g_gpu_ctx.pipelines[i].pipeline, 0x0); }

        if (g_gpu_ctx.pipelines[i].layout) { vkDestroyPipelineLayout(device, (VkPipelineLayout)g_gpu_ctx.pipelines[i].layout, 0x0); }

        if (g_gpu_ctx.pipelines[i].shader_module) { vkDestroyShaderModule(device, (VkShaderModule)g_gpu_ctx.pipelines[i].shader_module, 0x0); }
    }

    for (i = 0; i < LDG_GPU_BUFF_MAX; i++)
    {
        if (!g_gpu_ctx.buffs[i].in_use) { continue; }

        if (g_gpu_ctx.buffs[i].vk_buff) { vkDestroyBuffer(device, (VkBuffer)g_gpu_ctx.buffs[i].vk_buff, 0x0); }
    }

    if (g_gpu_ctx.staging_map && g_gpu_ctx.staging_mem) { vkUnmapMemory(device, (VkDeviceMemory)g_gpu_ctx.staging_mem); }

    if (g_gpu_ctx.staging_buff) { vkDestroyBuffer(device, (VkBuffer)g_gpu_ctx.staging_buff, 0x0); }

    if (g_gpu_ctx.staging_mem) { vkFreeMemory(device, (VkDeviceMemory)g_gpu_ctx.staging_mem, 0x0); }

    for (i = 0; i < LDG_GPU_MEM_SLAB_MAX; i++)
    {
        if (!g_gpu_ctx.slabs[i].in_use) { continue; }

        if (g_gpu_ctx.slabs[i].mem) { vkFreeMemory(device, (VkDeviceMemory)g_gpu_ctx.slabs[i].mem, 0x0); }
    }

    if (g_gpu_ctx.desc_set_layout) { vkDestroyDescriptorSetLayout(device, (VkDescriptorSetLayout)g_gpu_ctx.desc_set_layout, 0x0); }

    if (g_gpu_ctx.desc_pool) { vkDestroyDescriptorPool(device, (VkDescriptorPool)g_gpu_ctx.desc_pool, 0x0); }

    if (g_gpu_ctx.cmd_pool) { vkDestroyCommandPool(device, (VkCommandPool)g_gpu_ctx.cmd_pool, 0x0); }

    if (g_gpu_ctx.debug_messenger && instance)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroy_fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroy_fn) { destroy_fn(instance, (VkDebugUtilsMessengerEXT)g_gpu_ctx.debug_messenger, 0x0); }
    }

    if (device) { vkDestroyDevice(device, 0x0); }

    if (instance) { vkDestroyInstance(instance, 0x0); }

    if (g_gpu_ctx.mut_init) { ldg_mut_destroy(&g_gpu_ctx.mut); }

    if (LDG_UNLIKELY(memset(&g_gpu_ctx, 0, sizeof(ldg_gpu_ctx_t)) != &g_gpu_ctx)) { return; }
}

static uint32_t gpu_slab_alloc(uint8_t want_device_local, uint64_t size, uint64_t alignment, uint32_t mem_type_bits, uint32_t *out_slab_idx, uint64_t *out_offset)
{
    uint32_t i = 0;
    uint64_t aligned_off = 0;

    if (LDG_UNLIKELY(!out_slab_idx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_offset)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_slab_idx = UINT32_MAX;
    *out_offset = 0;

    for (i = 0; i < LDG_GPU_MEM_SLAB_MAX; i++)
    {
        if (!g_gpu_ctx.slabs[i].in_use) { continue; }

        if (want_device_local && !g_gpu_ctx.slabs[i].is_device_local) { continue; }

        if (!want_device_local && !g_gpu_ctx.slabs[i].is_host_visible) { continue; }

        if (!((1u << g_gpu_ctx.slabs[i].mem_type_idx) & mem_type_bits)) { continue; }

        aligned_off = GPU_ALIGN_UP(g_gpu_ctx.slabs[i].offset, alignment);
        if (aligned_off + size <= g_gpu_ctx.slabs[i].size)
        {
            *out_slab_idx = i;
            *out_offset = aligned_off;
            g_gpu_ctx.slabs[i].offset = aligned_off + size;
            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_GPU_MEM_ALLOC;
}

static uint32_t gpu_spill_slab_create(uint64_t min_size, uint32_t mem_type_bits)
{
    VkMemoryAllocateInfo alloc_info = { 0 };
    VkDeviceMemory mem = VK_NULL_HANDLE;
    VkDevice device = (VkDevice)g_gpu_ctx.device;
    uint64_t slab_size = 0;
    uint32_t i = 0;

    if (g_gpu_ctx.slab_cunt >= LDG_GPU_MEM_SLAB_MAX) { return LDG_ERR_GPU_MEM_ALLOC; }

    if (!((1u << g_gpu_ctx.host_visible_type_idx) & mem_type_bits)) { return LDG_ERR_GPU_MEM_ALLOC; }

    for (i = 0; i < LDG_GPU_MEM_SLAB_MAX; i++) { if (!g_gpu_ctx.slabs[i].in_use) { break; } }
    if (i >= LDG_GPU_MEM_SLAB_MAX) { return LDG_ERR_GPU_MEM_ALLOC; }

    slab_size = min_size < LDG_GPU_DEFAULT_SLAB_SIZE ? LDG_GPU_DEFAULT_SLAB_SIZE : min_size;

    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = (VkDeviceSize)slab_size;
    alloc_info.memoryTypeIndex = g_gpu_ctx.host_visible_type_idx;

    if (LDG_UNLIKELY(vkAllocateMemory(device, &alloc_info, 0x0, &mem) != VK_SUCCESS)) { return LDG_ERR_GPU_MEM_ALLOC; }

    g_gpu_ctx.slabs[i].mem = (void *)mem;
    g_gpu_ctx.slabs[i].size = slab_size;
    g_gpu_ctx.slabs[i].offset = 0;
    g_gpu_ctx.slabs[i].mem_type_idx = g_gpu_ctx.host_visible_type_idx;
    g_gpu_ctx.slabs[i].is_device_local = 0;
    g_gpu_ctx.slabs[i].is_host_visible = 1;
    g_gpu_ctx.slabs[i].in_use = 1;
    g_gpu_ctx.slab_cunt++;

    return LDG_ERR_AOK;
}

static uint32_t gpu_cmd_begin_oneshot(VkCommandBuffer *out)
{
    VkCommandBufferAllocateInfo alloc_info = { 0 };
    VkCommandBufferBeginInfo begin_info = { 0 };
    VkCommandBuffer cmd = VK_NULL_HANDLE;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = VK_NULL_HANDLE;

    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = (VkCommandPool)g_gpu_ctx.cmd_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    if (LDG_UNLIKELY(vkAllocateCommandBuffers((VkDevice)g_gpu_ctx.device, &alloc_info, &cmd) != VK_SUCCESS)) { return LDG_ERR_GPU_CMD_RECORD; }

    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (LDG_UNLIKELY(vkBeginCommandBuffer(cmd, &begin_info) != VK_SUCCESS))
    {
        vkFreeCommandBuffers((VkDevice)g_gpu_ctx.device, (VkCommandPool)g_gpu_ctx.cmd_pool, 1, &cmd);
        return LDG_ERR_GPU_CMD_RECORD;
    }

    *out = cmd;
    return LDG_ERR_AOK;
}

static uint32_t gpu_cmd_submit_wait(VkCommandBuffer cmd)
{
    VkSubmitInfo submit_info = { 0 };
    VkFenceCreateInfo fence_info = { 0 };
    VkFence fence = VK_NULL_HANDLE;
    VkDevice device = (VkDevice)g_gpu_ctx.device;
    VkResult res = VK_SUCCESS;

    if (LDG_UNLIKELY(vkEndCommandBuffer(cmd) != VK_SUCCESS)) { return LDG_ERR_GPU_CMD_RECORD; }

    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if (LDG_UNLIKELY(vkCreateFence(device, &fence_info, 0x0, &fence) != VK_SUCCESS)) { return LDG_ERR_GPU_FENCE_CREATE; }

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    res = vkQueueSubmit((VkQueue)g_gpu_ctx.queue, 1, &submit_info, fence);
    if (LDG_UNLIKELY(res != VK_SUCCESS))
    {
        vkDestroyFence(device, fence, 0x0);
        return LDG_ERR_GPU_SUBMIT;
    }

    res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device, fence, 0x0);

    if (LDG_UNLIKELY(res != VK_SUCCESS)) { return LDG_ERR_GPU_FENCE_TIMEOUT; }

    return LDG_ERR_AOK;
}

static uint32_t gpu_staging_transfer(uint32_t buff_idx, void *host_data, uint64_t size, uint64_t buff_offset, uint8_t to_device)
{
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkBufferCopy region = { 0 };
    VkBuffer vk_buff = VK_NULL_HANDLE;
    VkBuffer staging = (VkBuffer)g_gpu_ctx.staging_buff;
    uint64_t remaining = size;
    uint64_t data_off = 0;
    uint64_t chunk = 0;
    uint32_t err = 0;

    vk_buff = (VkBuffer)g_gpu_ctx.buffs[buff_idx].vk_buff;

    while (remaining > 0)
    {
        chunk = remaining > g_gpu_ctx.staging_size ? g_gpu_ctx.staging_size : remaining;

        if (to_device) { if (LDG_UNLIKELY(memcpy(g_gpu_ctx.staging_map, (const uint8_t *)host_data + data_off, (uint64_t)chunk) != g_gpu_ctx.staging_map)) { return LDG_ERR_GPU_TRANSFER; } }

        err = gpu_cmd_begin_oneshot(&cmd);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        region.size = (VkDeviceSize)chunk;
        if (to_device)
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

        err = gpu_cmd_submit_wait(cmd);
        vkFreeCommandBuffers((VkDevice)g_gpu_ctx.device, (VkCommandPool)g_gpu_ctx.cmd_pool, 1, &cmd);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (!to_device) { if (LDG_UNLIKELY(memcpy((uint8_t *)host_data + data_off, g_gpu_ctx.staging_map, (uint64_t)chunk) != (uint8_t *)host_data + data_off)) { return LDG_ERR_GPU_TRANSFER; } }

        remaining -= chunk;
        data_off += chunk;
    }

    return LDG_ERR_AOK;
}

static uint32_t gpu_dispatch_prepare(const ldg_gpu_dispatch_desc_t *desc, VkDescriptorSet *out_ds, VkCommandBuffer *out_cmd)
{
    VkDescriptorSetAllocateInfo ds_alloc = { 0 };
    VkDescriptorBufferInfo buff_infos[LDG_GPU_BIND_MAX] = { { 0 } };
    VkWriteDescriptorSet writes[LDG_GPU_BIND_MAX] = { { 0 } };
    VkCommandBufferAllocateInfo cmd_alloc = { 0 };
    VkCommandBufferBeginInfo cmd_begin = { 0 };
    VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
    VkDescriptorSet ds = VK_NULL_HANDLE;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkDevice device = (VkDevice)g_gpu_ctx.device;
    uint32_t pid = 0;
    uint32_t i = 0;

    pid = desc->pipeline_id;
    if (LDG_UNLIKELY(pid >= LDG_GPU_PIPELINE_POOL_MAX || !g_gpu_ctx.pipelines[pid].in_use)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (i = 0; i < desc->buff_cunt; i++) { if (LDG_UNLIKELY(desc->buff_ids[i] >= LDG_GPU_BUFF_MAX || !g_gpu_ctx.buffs[desc->buff_ids[i]].in_use)) { return LDG_ERR_FUNC_ARG_INVALID; } }

    dsl = (VkDescriptorSetLayout)g_gpu_ctx.desc_set_layout;
    ds_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ds_alloc.descriptorPool = (VkDescriptorPool)g_gpu_ctx.desc_pool;
    ds_alloc.descriptorSetCount = 1;
    ds_alloc.pSetLayouts = &dsl;

    if (LDG_UNLIKELY(vkAllocateDescriptorSets(device, &ds_alloc, &ds) != VK_SUCCESS)) { return LDG_ERR_GPU_DESC_ALLOC; }

    for (i = 0; i < desc->buff_cunt; i++)
    {
        buff_infos[i].buffer = (VkBuffer)g_gpu_ctx.buffs[desc->buff_ids[i]].vk_buff;
        buff_infos[i].offset = 0;
        buff_infos[i].range = VK_WHOLE_SIZE;

        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = ds;
        writes[i].dstBinding = i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &buff_infos[i];
    }

    vkUpdateDescriptorSets(device, desc->buff_cunt, writes, 0, 0x0);

    cmd_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc.commandPool = (VkCommandPool)g_gpu_ctx.cmd_pool;
    cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc.commandBufferCount = 1;

    if (LDG_UNLIKELY(vkAllocateCommandBuffers(device, &cmd_alloc, &cmd) != VK_SUCCESS))
    {
        vkFreeDescriptorSets(device, (VkDescriptorPool)g_gpu_ctx.desc_pool, 1, &ds);
        return LDG_ERR_GPU_CMD_RECORD;
    }

    cmd_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (LDG_UNLIKELY(vkBeginCommandBuffer(cmd, &cmd_begin) != VK_SUCCESS))
    {
        vkFreeCommandBuffers(device, (VkCommandPool)g_gpu_ctx.cmd_pool, 1, &cmd);
        vkFreeDescriptorSets(device, (VkDescriptorPool)g_gpu_ctx.desc_pool, 1, &ds);
        return LDG_ERR_GPU_CMD_RECORD;
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipeline)g_gpu_ctx.pipelines[pid].pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipelineLayout)g_gpu_ctx.pipelines[pid].layout, 0, 1, &ds, 0, 0x0);
    vkCmdDispatch(cmd, desc->group_cunt_x, desc->group_cunt_y, desc->group_cunt_z);

    if (LDG_UNLIKELY(vkEndCommandBuffer(cmd) != VK_SUCCESS))
    {
        vkFreeCommandBuffers(device, (VkCommandPool)g_gpu_ctx.cmd_pool, 1, &cmd);
        vkFreeDescriptorSets(device, (VkDescriptorPool)g_gpu_ctx.desc_pool, 1, &ds);
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
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return err; }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkPhysicalDeviceMemoryProperties), (void **)&mem_props);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return err; }

    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.apiVersion = VK_API_VERSION_1_2;

    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    if (LDG_UNLIKELY(vkCreateInstance(&create_info, 0x0, &tmp_instance) != VK_SUCCESS)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_GPU_INIT; }

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

    if (LDG_UNLIKELY(memset(out, 0, dev_cunt * sizeof(ldg_gpu_dev_info_t)) != out))
    {
        ldg_mem_dealloc(out);
        vkDestroyInstance(tmp_instance, 0x0);
        ldg_mem_pool_destroy(&scratch);
        return LDG_ERR_MEM_BAD;
    }

    for (i = 0; i < dev_cunt; i++)
    {
        if (LDG_UNLIKELY(memset(props, 0, sizeof(VkPhysicalDeviceProperties)) != props)) { continue; }

        if (LDG_UNLIKELY(memset(mem_props, 0, sizeof(VkPhysicalDeviceMemoryProperties)) != mem_props)) { continue; }

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

LDG_EXPORT uint32_t ldg_gpu_init(const ldg_gpu_init_desc_t *desc)
{
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
    VkPhysicalDevice *phys_devs = 0x0;
    VkPhysicalDeviceProperties *dev_props = 0x0;
    VkPhysicalDeviceMemoryProperties *mem_props = 0x0;
    VkQueueFamilyProperties *queue_fam_props = 0x0;
    VkPhysicalDeviceVulkan12Features *vk12_feat = 0x0;
    VkDescriptorSetLayoutBinding *bindings = 0x0;
    ldg_mem_pool_t *scratch = 0x0;
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
    VkDescriptorPool dp = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT dbg_messenger = VK_NULL_HANDLE;
    VkBuffer staging_buff = VK_NULL_HANDLE;
    VkDeviceMemory staging_mem = VK_NULL_HANDLE;
    VkDeviceMemory slab_mem = VK_NULL_HANDLE;
    void *staging_map = 0x0;
    float queue_priority = 1.0f;
    const char *validation_layer = "VK_LAYER_KHRONOS_validation";
    const char *debug_ext = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    uint32_t dev_cunt = 0;
    uint32_t queue_fam_cunt = 0;
    uint32_t slab_cunt = 0;
    uint64_t slab_size = 0;
    uint32_t dev_idx = 0;
    uint32_t selected_idx = UINT32_MAX;
    uint32_t compute_fam_idx = UINT32_MAX;
    uint8_t enable_validation = 0;
    uint8_t device_local_found = 0;
    uint8_t host_visible_found = 0;
    uint32_t i = 0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (g_gpu_ctx.is_init) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(memset(&g_gpu_ctx, 0, sizeof(ldg_gpu_ctx_t)) != &g_gpu_ctx)) { return LDG_ERR_MEM_BAD; }

    err = ldg_mem_pool_create(0, 4096, &scratch);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkPhysicalDevice) * LDG_GPU_DEV_ENUM_MAX, (void **)&phys_devs);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return err; }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkPhysicalDeviceProperties), (void **)&dev_props);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return err; }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkPhysicalDeviceMemoryProperties), (void **)&mem_props);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return err; }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkQueueFamilyProperties) * 32, (void **)&queue_fam_props);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return err; }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkPhysicalDeviceVulkan12Features), (void **)&vk12_feat);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return err; }

    err = ldg_mem_pool_alloc(scratch, sizeof(VkDescriptorSetLayoutBinding) * LDG_GPU_BIND_MAX, (void **)&bindings);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ldg_mem_pool_destroy(&scratch); return err; }

    if (LDG_UNLIKELY(memset(phys_devs, 0, sizeof(VkPhysicalDevice) * LDG_GPU_DEV_ENUM_MAX) != phys_devs)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(memset(vk12_feat, 0, sizeof(VkPhysicalDeviceVulkan12Features)) != vk12_feat)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(memset(bindings, 0, sizeof(VkDescriptorSetLayoutBinding) * LDG_GPU_BIND_MAX) != bindings)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_MEM_BAD; }

    g_gpu_ctx.flags = desc->flags;
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

    if (enable_validation)
    {
        inst_info.enabledLayerCount = 1;
        inst_info.ppEnabledLayerNames = &validation_layer;
        inst_info.enabledExtensionCount = 1;
        inst_info.ppEnabledExtensionNames = &debug_ext;
    }

    if (vkCreateInstance(&inst_info, 0x0, &instance) != VK_SUCCESS)
    {
        if (enable_validation)
        {
            enable_validation = 0;
            inst_info.enabledLayerCount = 0;
            inst_info.ppEnabledLayerNames = 0x0;
            inst_info.enabledExtensionCount = 0;
            inst_info.ppEnabledExtensionNames = 0x0;
            if (LDG_UNLIKELY(vkCreateInstance(&inst_info, 0x0, &instance) != VK_SUCCESS)) { ldg_mem_pool_destroy(&scratch); return LDG_ERR_GPU_INIT; }

            LDG_ERRLOG_WARN("gpu: validation layer unavailable; continuing without");
        }
        else { ldg_mem_pool_destroy(&scratch); return LDG_ERR_GPU_INIT; }
    }

    g_gpu_ctx.instance = (void *)instance;

    if (enable_validation)
    {
        PFN_vkCreateDebugUtilsMessengerEXT create_dbg_fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (create_dbg_fn)
        {
            dbg_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            dbg_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
            dbg_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            dbg_info.pfnUserCallback = gpu_debug_msg_cb;
            if (create_dbg_fn(instance, &dbg_info, 0x0, &dbg_messenger) == VK_SUCCESS) { g_gpu_ctx.debug_messenger = (void *)dbg_messenger; }
        }
    }

    dev_cunt = LDG_GPU_DEV_ENUM_MAX;
    if (LDG_UNLIKELY(vkEnumeratePhysicalDevices(instance, &dev_cunt, phys_devs) < 0 || dev_cunt == 0))
    {
        ldg_mem_pool_destroy(&scratch);
        gpu_shutdown_internal();
        return LDG_ERR_GPU_DEV_NOT_FOUND;
    }

    dev_idx = desc->dev_idx;
    if (dev_idx == UINT32_MAX)
    {
        for (i = 0; i < dev_cunt; i++)
        {
            if (LDG_UNLIKELY(memset(dev_props, 0, sizeof(VkPhysicalDeviceProperties)) != dev_props)) { continue; }

            vkGetPhysicalDeviceProperties(phys_devs[i], dev_props);
            if (dev_props->deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { selected_idx = i; break; }
        }
        if (selected_idx == UINT32_MAX) { selected_idx = 0; }
    }
    else
    {
        if (LDG_UNLIKELY(dev_idx >= dev_cunt)) { ldg_mem_pool_destroy(&scratch); gpu_shutdown_internal(); return LDG_ERR_GPU_DEV_NOT_FOUND; }

        selected_idx = dev_idx;
    }

    g_gpu_ctx.phys_dev = (void *)phys_devs[selected_idx];

    if (LDG_UNLIKELY(memset(dev_props, 0, sizeof(VkPhysicalDeviceProperties)) != dev_props)) { ldg_mem_pool_destroy(&scratch); gpu_shutdown_internal(); return LDG_ERR_MEM_BAD; }

    vkGetPhysicalDeviceProperties(phys_devs[selected_idx], dev_props);
    g_gpu_ctx.min_storage_buff_offset_align = (uint64_t)dev_props->limits.minStorageBufferOffsetAlignment;
    if (g_gpu_ctx.min_storage_buff_offset_align == 0) { g_gpu_ctx.min_storage_buff_offset_align = 256; }

    if (LDG_UNLIKELY(memset(mem_props, 0, sizeof(VkPhysicalDeviceMemoryProperties)) != mem_props)) { ldg_mem_pool_destroy(&scratch); gpu_shutdown_internal(); return LDG_ERR_MEM_BAD; }

    vkGetPhysicalDeviceMemoryProperties(phys_devs[selected_idx], mem_props);

    for (i = 0; i < mem_props->memoryTypeCount; i++)
    {
        VkMemoryPropertyFlags mflags = mem_props->memoryTypes[i].propertyFlags;

        if ((mflags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && !device_local_found)
        {
            g_gpu_ctx.device_local_type_idx = i;
            device_local_found = 1;
        }

        if ((mflags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && !host_visible_found)
        {
            g_gpu_ctx.host_visible_type_idx = i;
            host_visible_found = 1;
        }
    }

    if (LDG_UNLIKELY(!device_local_found || !host_visible_found)) { ldg_mem_pool_destroy(&scratch); gpu_shutdown_internal(); return LDG_ERR_GPU_MEM_ALLOC; }

    {
        VkMemoryPropertyFlags dl_flags = mem_props->memoryTypes[g_gpu_ctx.device_local_type_idx].propertyFlags;
        if (dl_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) { g_gpu_ctx.has_unified_mem = 1; }
    }

    for (i = 0; i < mem_props->memoryHeapCount; i++) { if (mem_props->memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) { g_gpu_ctx.vram_total += mem_props->memoryHeaps[i].size; }
        else { g_gpu_ctx.host_total += mem_props->memoryHeaps[i].size; } }

    queue_fam_cunt = 32;
    vkGetPhysicalDeviceQueueFamilyProperties(phys_devs[selected_idx], &queue_fam_cunt, queue_fam_props);

    for (i = 0; i < queue_fam_cunt; i++) { if (queue_fam_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) { compute_fam_idx = i; break; } }
    if (LDG_UNLIKELY(compute_fam_idx == UINT32_MAX)) { ldg_mem_pool_destroy(&scratch); gpu_shutdown_internal(); return LDG_ERR_GPU_QUEUE_NOT_FOUND; }

    g_gpu_ctx.compute_queue_family_idx = compute_fam_idx;

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

    if (LDG_UNLIKELY(vkCreateDevice(phys_devs[selected_idx], &dev_info, 0x0, &device) != VK_SUCCESS)) { ldg_mem_pool_destroy(&scratch); gpu_shutdown_internal(); return LDG_ERR_GPU_INIT; }

    g_gpu_ctx.device = (void *)device;

    vkGetDeviceQueue(device, compute_fam_idx, 0, &queue);
    g_gpu_ctx.queue = (void *)queue;

    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = compute_fam_idx;

    if (LDG_UNLIKELY(vkCreateCommandPool(device, &pool_info, 0x0, &cmd_pool) != VK_SUCCESS)) { ldg_mem_pool_destroy(&scratch); gpu_shutdown_internal(); return LDG_ERR_GPU_INIT; }

    g_gpu_ctx.cmd_pool = (void *)cmd_pool;

    for (i = 0; i < LDG_GPU_BIND_MAX; i++)
    {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        binding_flags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    }

    binding_flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    binding_flags_info.bindingCount = LDG_GPU_BIND_MAX;
    binding_flags_info.pBindingFlags = binding_flags;

    dsl_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_info.pNext = &binding_flags_info;
    dsl_info.bindingCount = LDG_GPU_BIND_MAX;
    dsl_info.pBindings = bindings;

    if (LDG_UNLIKELY(vkCreateDescriptorSetLayout(device, &dsl_info, 0x0, &dsl) != VK_SUCCESS)) { ldg_mem_pool_destroy(&scratch); gpu_shutdown_internal(); return LDG_ERR_GPU_INIT; }

    ldg_mem_pool_destroy(&scratch);

    g_gpu_ctx.desc_set_layout = (void *)dsl;

    dp_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    dp_size.descriptorCount = (uint32_t)(LDG_GPU_BIND_MAX * (LDG_GPU_FENCE_MAX + 1));

    dp_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dp_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    dp_info.maxSets = LDG_GPU_FENCE_MAX + 1;
    dp_info.poolSizeCount = 1;
    dp_info.pPoolSizes = &dp_size;

    if (LDG_UNLIKELY(vkCreateDescriptorPool(device, &dp_info, 0x0, &dp) != VK_SUCCESS)) { gpu_shutdown_internal(); return LDG_ERR_GPU_INIT; }

    g_gpu_ctx.desc_pool = (void *)dp;

    for (i = 0; i < slab_cunt; i++)
    {
        slab_mem = VK_NULL_HANDLE;
        slab_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        slab_alloc.allocationSize = (VkDeviceSize)slab_size;
        slab_alloc.memoryTypeIndex = g_gpu_ctx.device_local_type_idx;

        if (LDG_UNLIKELY(vkAllocateMemory(device, &slab_alloc, 0x0, &slab_mem) != VK_SUCCESS)) { gpu_shutdown_internal(); return LDG_ERR_GPU_MEM_ALLOC; }

        g_gpu_ctx.slabs[i].mem = (void *)slab_mem;
        g_gpu_ctx.slabs[i].size = slab_size;
        g_gpu_ctx.slabs[i].offset = 0;
        g_gpu_ctx.slabs[i].mem_type_idx = g_gpu_ctx.device_local_type_idx;
        g_gpu_ctx.slabs[i].is_device_local = 1;
        g_gpu_ctx.slabs[i].is_host_visible = g_gpu_ctx.has_unified_mem;
        g_gpu_ctx.slabs[i].in_use = 1;
        g_gpu_ctx.slab_cunt++;
        g_gpu_ctx.vram_used += slab_size;
    }

    staging_buff_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    staging_buff_info.size = (VkDeviceSize)LDG_GPU_STAGING_SIZE;
    staging_buff_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    staging_buff_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (LDG_UNLIKELY(vkCreateBuffer(device, &staging_buff_info, 0x0, &staging_buff) != VK_SUCCESS)) { gpu_shutdown_internal(); return LDG_ERR_GPU_INIT; }

    g_gpu_ctx.staging_buff = (void *)staging_buff;

    vkGetBufferMemoryRequirements(device, staging_buff, &staging_reqs);

    staging_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    staging_alloc.allocationSize = staging_reqs.size;
    staging_alloc.memoryTypeIndex = g_gpu_ctx.host_visible_type_idx;

    if (LDG_UNLIKELY(vkAllocateMemory(device, &staging_alloc, 0x0, &staging_mem) != VK_SUCCESS)) { gpu_shutdown_internal(); return LDG_ERR_GPU_MEM_ALLOC; }

    g_gpu_ctx.staging_mem = (void *)staging_mem;

    if (LDG_UNLIKELY(vkBindBufferMemory(device, staging_buff, staging_mem, 0) != VK_SUCCESS)) { gpu_shutdown_internal(); return LDG_ERR_GPU_MEM_BIND; }

    if (LDG_UNLIKELY(vkMapMemory(device, staging_mem, 0, staging_reqs.size, 0, &staging_map) != VK_SUCCESS)) { gpu_shutdown_internal(); return LDG_ERR_GPU_BUFF_MAP; }

    g_gpu_ctx.staging_map = staging_map;
    g_gpu_ctx.staging_size = LDG_GPU_STAGING_SIZE;

    err = ldg_mut_init(&g_gpu_ctx.mut, 0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { gpu_shutdown_internal(); return LDG_ERR_GPU_INIT; }

    g_gpu_ctx.mut_init = 1;

    g_gpu_ctx.is_init = 1;
    return LDG_ERR_AOK;
}

LDG_EXPORT void ldg_gpu_shutdown(void)
{
    if (!g_gpu_ctx.is_init) { return; }

    gpu_shutdown_internal();
}

LDG_EXPORT uint32_t ldg_gpu_buff_create(const ldg_gpu_buff_desc_t *desc, ldg_gpu_buff_t *out)
{
    VkBufferCreateInfo buff_info = { 0 };
    VkMemoryRequirements mem_reqs = { 0 };
    VkBuffer vk_buff = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t slot = UINT32_MAX;
    uint32_t slab_idx = UINT32_MAX;
    uint64_t slab_off = 0;
    uint8_t want_device_local = 1;
    uint8_t spilled = 0;
    uint32_t err = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(out, 0, sizeof(ldg_gpu_buff_t)) != out)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(!g_gpu_ctx.is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(desc->size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    device = (VkDevice)g_gpu_ctx.device;
    ldg_mut_lock(&g_gpu_ctx.mut);

    for (i = 0; i < LDG_GPU_BUFF_MAX; i++) { if (!g_gpu_ctx.buffs[i].in_use) { slot = i; break; } }
    if (LDG_UNLIKELY(slot == UINT32_MAX)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_FULL; }

    buff_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buff_info.size = (VkDeviceSize)desc->size;
    buff_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buff_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (LDG_UNLIKELY(vkCreateBuffer(device, &buff_info, 0x0, &vk_buff) != VK_SUCCESS)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_GPU_BUFF_CREATE; }

    vkGetBufferMemoryRequirements(device, vk_buff, &mem_reqs);

    want_device_local = (desc->mem_flags & LDG_GPU_MEM_HOST_VISIBLE) ? 0 : 1;

    err = gpu_slab_alloc(want_device_local, mem_reqs.size, mem_reqs.alignment, (uint32_t)mem_reqs.memoryTypeBits, &slab_idx, &slab_off);

    if (err != LDG_ERR_AOK && want_device_local && (g_gpu_ctx.flags & LDG_GPU_FLAG_SPILL_ENABLE))
    {
        LDG_ERRLOG_WARN("gpu: device-local exhausted; spilling to host-visible");
        err = gpu_slab_alloc(0, mem_reqs.size, mem_reqs.alignment, (uint32_t)mem_reqs.memoryTypeBits, &slab_idx, &slab_off);
        if (err != LDG_ERR_AOK)
        {
            err = gpu_spill_slab_create(mem_reqs.size, (uint32_t)mem_reqs.memoryTypeBits);
            if (err == LDG_ERR_AOK) { err = gpu_slab_alloc(0, mem_reqs.size, mem_reqs.alignment, (uint32_t)mem_reqs.memoryTypeBits, &slab_idx, &slab_off); }
        }

        if (err == LDG_ERR_AOK)
        {
            spilled = 1;
            g_gpu_ctx.spill_cunt++;
        }
    }

    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        vkDestroyBuffer(device, vk_buff, 0x0);
        ldg_mut_unlock(&g_gpu_ctx.mut);
        return err;
    }

    if (LDG_UNLIKELY(vkBindBufferMemory(device, vk_buff, (VkDeviceMemory)g_gpu_ctx.slabs[slab_idx].mem, (VkDeviceSize)slab_off) != VK_SUCCESS))
    {
        vkDestroyBuffer(device, vk_buff, 0x0);
        ldg_mut_unlock(&g_gpu_ctx.mut);
        return LDG_ERR_GPU_MEM_BIND;
    }

    g_gpu_ctx.buffs[slot].vk_buff = (void *)vk_buff;
    g_gpu_ctx.buffs[slot].size = desc->size;
    g_gpu_ctx.buffs[slot].mem_offset = slab_off;
    g_gpu_ctx.buffs[slot].slab_idx = slab_idx;
    g_gpu_ctx.buffs[slot].mem_flags = desc->mem_flags;
    g_gpu_ctx.buffs[slot].in_use = 1;
    g_gpu_ctx.buffs[slot].spilled = spilled;
    g_gpu_ctx.buffs[slot].is_host_visible = g_gpu_ctx.slabs[slab_idx].is_host_visible;

    out->id = slot;
    out->mem_flags = desc->mem_flags;
    out->size = desc->size;
    out->spilled = spilled;

    ldg_mut_unlock(&g_gpu_ctx.mut);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_buff_destroy(uint32_t buff_id)
{
    if (LDG_UNLIKELY(!g_gpu_ctx.is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(buff_id >= LDG_GPU_BUFF_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ldg_mut_lock(&g_gpu_ctx.mut);

    if (LDG_UNLIKELY(!g_gpu_ctx.buffs[buff_id].in_use)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_NOT_FOUND; }

    if (g_gpu_ctx.buffs[buff_id].vk_buff) { vkDestroyBuffer((VkDevice)g_gpu_ctx.device, (VkBuffer)g_gpu_ctx.buffs[buff_id].vk_buff, 0x0); }

    if (LDG_UNLIKELY(memset(&g_gpu_ctx.buffs[buff_id], 0, sizeof(ldg_gpu_buff_entry_t)) != &g_gpu_ctx.buffs[buff_id])) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_MEM_BAD; }

    ldg_mut_unlock(&g_gpu_ctx.mut);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_buff_wr(uint32_t id, const void *data, uint64_t size, uint64_t offset)
{
    void *mapped = 0x0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!data)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_gpu_ctx.is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(id >= LDG_GPU_BUFF_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ldg_mut_lock(&g_gpu_ctx.mut);

    if (LDG_UNLIKELY(!g_gpu_ctx.buffs[id].in_use)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(offset + size > g_gpu_ctx.buffs[id].size)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_BOUNDS; }

    if (g_gpu_ctx.buffs[id].is_host_visible)
    {
        if (LDG_UNLIKELY(vkMapMemory((VkDevice)g_gpu_ctx.device, (VkDeviceMemory)g_gpu_ctx.slabs[g_gpu_ctx.buffs[id].slab_idx].mem, (VkDeviceSize)(g_gpu_ctx.buffs[id].mem_offset + offset), (VkDeviceSize)size, 0, &mapped) != VK_SUCCESS))
        {
            ldg_mut_unlock(&g_gpu_ctx.mut);
            return LDG_ERR_GPU_BUFF_MAP;
        }

        if (LDG_UNLIKELY(memcpy(mapped, data, (uint64_t)size) != mapped))
        {
            vkUnmapMemory((VkDevice)g_gpu_ctx.device, (VkDeviceMemory)g_gpu_ctx.slabs[g_gpu_ctx.buffs[id].slab_idx].mem);
            ldg_mut_unlock(&g_gpu_ctx.mut);
            return LDG_ERR_GPU_TRANSFER;
        }

        vkUnmapMemory((VkDevice)g_gpu_ctx.device, (VkDeviceMemory)g_gpu_ctx.slabs[g_gpu_ctx.buffs[id].slab_idx].mem);
    }
    else
    {
        err = gpu_staging_transfer(id, (void *)data, size, offset, 1);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ldg_mut_unlock(&g_gpu_ctx.mut); return err; }
    }

    ldg_mut_unlock(&g_gpu_ctx.mut);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_buff_rd(uint32_t id, void *data, uint64_t size, uint64_t offset)
{
    void *mapped = 0x0;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!data)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_gpu_ctx.is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(id >= LDG_GPU_BUFF_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ldg_mut_lock(&g_gpu_ctx.mut);

    if (LDG_UNLIKELY(!g_gpu_ctx.buffs[id].in_use)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(offset + size > g_gpu_ctx.buffs[id].size)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_BOUNDS; }

    if (g_gpu_ctx.buffs[id].is_host_visible)
    {
        if (LDG_UNLIKELY(vkMapMemory((VkDevice)g_gpu_ctx.device, (VkDeviceMemory)g_gpu_ctx.slabs[g_gpu_ctx.buffs[id].slab_idx].mem, (VkDeviceSize)(g_gpu_ctx.buffs[id].mem_offset + offset), (VkDeviceSize)size, 0, &mapped) != VK_SUCCESS))
        {
            ldg_mut_unlock(&g_gpu_ctx.mut);
            return LDG_ERR_GPU_BUFF_MAP;
        }

        if (LDG_UNLIKELY(memcpy(data, mapped, (uint64_t)size) != data))
        {
            vkUnmapMemory((VkDevice)g_gpu_ctx.device, (VkDeviceMemory)g_gpu_ctx.slabs[g_gpu_ctx.buffs[id].slab_idx].mem);
            ldg_mut_unlock(&g_gpu_ctx.mut);
            return LDG_ERR_GPU_TRANSFER;
        }

        vkUnmapMemory((VkDevice)g_gpu_ctx.device, (VkDeviceMemory)g_gpu_ctx.slabs[g_gpu_ctx.buffs[id].slab_idx].mem);
    }
    else
    {
        err = gpu_staging_transfer(id, data, size, offset, 0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ldg_mut_unlock(&g_gpu_ctx.mut); return err; }
    }

    ldg_mut_unlock(&g_gpu_ctx.mut);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_buff_fill(uint32_t id, uint32_t val, uint64_t size, uint64_t offset)
{
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!g_gpu_ctx.is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(id >= LDG_GPU_BUFF_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ldg_mut_lock(&g_gpu_ctx.mut);

    if (LDG_UNLIKELY(!g_gpu_ctx.buffs[id].in_use)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(offset + size > g_gpu_ctx.buffs[id].size)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(offset % 4 != 0 || size % 4 != 0)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_FUNC_ARG_INVALID; }

    err = gpu_cmd_begin_oneshot(&cmd);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ldg_mut_unlock(&g_gpu_ctx.mut); return err; }

    vkCmdFillBuffer(cmd, (VkBuffer)g_gpu_ctx.buffs[id].vk_buff, (VkDeviceSize)offset, (VkDeviceSize)size, val);

    err = gpu_cmd_submit_wait(cmd);
    vkFreeCommandBuffers((VkDevice)g_gpu_ctx.device, (VkCommandPool)g_gpu_ctx.cmd_pool, 1, &cmd);

    ldg_mut_unlock(&g_gpu_ctx.mut);
    return err;
}

LDG_EXPORT uint32_t ldg_gpu_pipeline_create(const ldg_gpu_spirv_desc_t *spirv, uint32_t *id)
{
    VkShaderModuleCreateInfo sm_info = { 0 };
    VkPipelineLayoutCreateInfo pl_info = { 0 };
    VkComputePipelineCreateInfo cp_info = { 0 };
    VkShaderModule shader_module = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout dsl = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t slot = UINT32_MAX;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!spirv)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!id)) { return LDG_ERR_FUNC_ARG_NULL; }

    *id = UINT32_MAX;
    if (LDG_UNLIKELY(!spirv->code || spirv->code_size == 0)) { return LDG_ERR_GPU_SPIRV_INVALID; }

    if (LDG_UNLIKELY(!spirv->entry_name)) { return LDG_ERR_GPU_SPIRV_INVALID; }

    if (LDG_UNLIKELY(spirv->code_size % 4 != 0)) { return LDG_ERR_GPU_SPIRV_INVALID; }

    if (LDG_UNLIKELY(!g_gpu_ctx.is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    device = (VkDevice)g_gpu_ctx.device;
    ldg_mut_lock(&g_gpu_ctx.mut);

    for (i = 0; i < LDG_GPU_PIPELINE_POOL_MAX; i++) { if (!g_gpu_ctx.pipelines[i].in_use) { slot = i; break; } }
    if (LDG_UNLIKELY(slot == UINT32_MAX)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_GPU_PIPELINE_FULL; }

    sm_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sm_info.codeSize = (uint64_t)spirv->code_size;
    sm_info.pCode = spirv->code;

    if (LDG_UNLIKELY(vkCreateShaderModule(device, &sm_info, 0x0, &shader_module) != VK_SUCCESS)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_GPU_PIPELINE_CREATE; }

    dsl = (VkDescriptorSetLayout)g_gpu_ctx.desc_set_layout;
    pl_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_info.setLayoutCount = 1;
    pl_info.pSetLayouts = &dsl;

    if (LDG_UNLIKELY(vkCreatePipelineLayout(device, &pl_info, 0x0, &layout) != VK_SUCCESS))
    {
        vkDestroyShaderModule(device, shader_module, 0x0);
        ldg_mut_unlock(&g_gpu_ctx.mut);
        return LDG_ERR_GPU_PIPELINE_CREATE;
    }

    cp_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cp_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    cp_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    cp_info.stage.module = shader_module;
    cp_info.stage.pName = spirv->entry_name;
    cp_info.layout = layout;

    if (LDG_UNLIKELY(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &cp_info, 0x0, &pipeline) != VK_SUCCESS))
    {
        vkDestroyPipelineLayout(device, layout, 0x0);
        vkDestroyShaderModule(device, shader_module, 0x0);
        ldg_mut_unlock(&g_gpu_ctx.mut);
        return LDG_ERR_GPU_PIPELINE_CREATE;
    }

    g_gpu_ctx.pipelines[slot].shader_module = (void *)shader_module;
    g_gpu_ctx.pipelines[slot].layout = (void *)layout;
    g_gpu_ctx.pipelines[slot].pipeline = (void *)pipeline;
    g_gpu_ctx.pipelines[slot].in_use = 1;

    *id = slot;

    ldg_mut_unlock(&g_gpu_ctx.mut);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_pipeline_destroy(uint32_t id)
{
    VkDevice device = VK_NULL_HANDLE;

    if (LDG_UNLIKELY(!g_gpu_ctx.is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(id >= LDG_GPU_PIPELINE_POOL_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    device = (VkDevice)g_gpu_ctx.device;
    ldg_mut_lock(&g_gpu_ctx.mut);

    if (LDG_UNLIKELY(!g_gpu_ctx.pipelines[id].in_use)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_NOT_FOUND; }

    if (g_gpu_ctx.pipelines[id].pipeline) { vkDestroyPipeline(device, (VkPipeline)g_gpu_ctx.pipelines[id].pipeline, 0x0); }

    if (g_gpu_ctx.pipelines[id].layout) { vkDestroyPipelineLayout(device, (VkPipelineLayout)g_gpu_ctx.pipelines[id].layout, 0x0); }

    if (g_gpu_ctx.pipelines[id].shader_module) { vkDestroyShaderModule(device, (VkShaderModule)g_gpu_ctx.pipelines[id].shader_module, 0x0); }

    if (LDG_UNLIKELY(memset(&g_gpu_ctx.pipelines[id], 0, sizeof(ldg_gpu_pipeline_entry_t)) != &g_gpu_ctx.pipelines[id])) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_MEM_BAD; }

    ldg_mut_unlock(&g_gpu_ctx.mut);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_dispatch(const ldg_gpu_dispatch_desc_t *desc)
{
    VkSubmitInfo submit_info = { 0 };
    VkFenceCreateInfo fence_info = { 0 };
    VkDescriptorSet ds = VK_NULL_HANDLE;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkResult res = VK_SUCCESS;
    uint32_t err = 0;

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_gpu_ctx.is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    device = (VkDevice)g_gpu_ctx.device;
    ldg_mut_lock(&g_gpu_ctx.mut);

    err = gpu_dispatch_prepare(desc, &ds, &cmd);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ldg_mut_unlock(&g_gpu_ctx.mut); return err; }

    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (LDG_UNLIKELY(vkCreateFence(device, &fence_info, 0x0, &fence) != VK_SUCCESS))
    {
        vkFreeCommandBuffers(device, (VkCommandPool)g_gpu_ctx.cmd_pool, 1, &cmd);
        vkFreeDescriptorSets(device, (VkDescriptorPool)g_gpu_ctx.desc_pool, 1, &ds);
        ldg_mut_unlock(&g_gpu_ctx.mut);
        return LDG_ERR_GPU_FENCE_CREATE;
    }

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    res = vkQueueSubmit((VkQueue)g_gpu_ctx.queue, 1, &submit_info, fence);
    if (LDG_UNLIKELY(res != VK_SUCCESS))
    {
        vkDestroyFence(device, fence, 0x0);
        vkFreeCommandBuffers(device, (VkCommandPool)g_gpu_ctx.cmd_pool, 1, &cmd);
        vkFreeDescriptorSets(device, (VkDescriptorPool)g_gpu_ctx.desc_pool, 1, &ds);
        ldg_mut_unlock(&g_gpu_ctx.mut);
        return LDG_ERR_GPU_SUBMIT;
    }

    res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(device, fence, 0x0);
    vkFreeCommandBuffers(device, (VkCommandPool)g_gpu_ctx.cmd_pool, 1, &cmd);
    vkFreeDescriptorSets(device, (VkDescriptorPool)g_gpu_ctx.desc_pool, 1, &ds);

    ldg_mut_unlock(&g_gpu_ctx.mut);
    return (res == VK_SUCCESS) ? LDG_ERR_AOK : LDG_ERR_GPU_DISPATCH;
}

LDG_EXPORT uint32_t ldg_gpu_dispatch_async(const ldg_gpu_dispatch_desc_t *desc, ldg_gpu_fence_t *fence_out)
{
    VkSubmitInfo submit_info = { 0 };
    VkFenceCreateInfo fence_info = { 0 };
    VkDescriptorSet ds = VK_NULL_HANDLE;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t slot = UINT32_MAX;
    uint32_t err = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!fence_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(fence_out, 0, sizeof(ldg_gpu_fence_t)) != fence_out)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(!g_gpu_ctx.is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    device = (VkDevice)g_gpu_ctx.device;
    ldg_mut_lock(&g_gpu_ctx.mut);

    for (i = 0; i < LDG_GPU_FENCE_MAX; i++) { if (!g_gpu_ctx.fences[i].in_use) { slot = i; break; } }
    if (LDG_UNLIKELY(slot == UINT32_MAX)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_FULL; }

    err = gpu_dispatch_prepare(desc, &ds, &cmd);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ldg_mut_unlock(&g_gpu_ctx.mut); return err; }

    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (LDG_UNLIKELY(vkCreateFence(device, &fence_info, 0x0, &fence) != VK_SUCCESS))
    {
        vkFreeCommandBuffers(device, (VkCommandPool)g_gpu_ctx.cmd_pool, 1, &cmd);
        vkFreeDescriptorSets(device, (VkDescriptorPool)g_gpu_ctx.desc_pool, 1, &ds);
        ldg_mut_unlock(&g_gpu_ctx.mut);
        return LDG_ERR_GPU_FENCE_CREATE;
    }

    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    if (LDG_UNLIKELY(vkQueueSubmit((VkQueue)g_gpu_ctx.queue, 1, &submit_info, fence) != VK_SUCCESS))
    {
        vkDestroyFence(device, fence, 0x0);
        vkFreeCommandBuffers(device, (VkCommandPool)g_gpu_ctx.cmd_pool, 1, &cmd);
        vkFreeDescriptorSets(device, (VkDescriptorPool)g_gpu_ctx.desc_pool, 1, &ds);
        ldg_mut_unlock(&g_gpu_ctx.mut);
        return LDG_ERR_GPU_SUBMIT;
    }

    g_gpu_ctx.fences[slot].fence = (void *)fence;
    g_gpu_ctx.fences[slot].cmd_buff = (void *)cmd;
    g_gpu_ctx.fences[slot].desc_set = (void *)ds;
    g_gpu_ctx.fences[slot].in_use = 1;

    fence_out->id = slot;

    ldg_mut_unlock(&g_gpu_ctx.mut);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_fence_wait(ldg_gpu_fence_t *fence, uint64_t timeout_ms)
{
    VkFence vk_fence = VK_NULL_HANDLE;
    VkResult res = VK_SUCCESS;
    uint64_t timeout_ns = 0;

    if (LDG_UNLIKELY(!fence)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_gpu_ctx.is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(fence->id >= LDG_GPU_FENCE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ldg_mut_lock(&g_gpu_ctx.mut);

    if (LDG_UNLIKELY(!g_gpu_ctx.fences[fence->id].in_use)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_NOT_FOUND; }

    vk_fence = (VkFence)g_gpu_ctx.fences[fence->id].fence;
    timeout_ns = (timeout_ms == UINT64_MAX) ? UINT64_MAX : timeout_ms * LDG_NS_PER_MS;

    res = vkWaitForFences((VkDevice)g_gpu_ctx.device, 1, &vk_fence, VK_TRUE, timeout_ns);

    ldg_mut_unlock(&g_gpu_ctx.mut);
    return (res == VK_SUCCESS) ? LDG_ERR_AOK : LDG_ERR_GPU_FENCE_TIMEOUT;
}

LDG_EXPORT uint32_t ldg_gpu_fence_poll(ldg_gpu_fence_t *fence, uint8_t *ready)
{
    VkFence vk_fence = VK_NULL_HANDLE;
    VkResult res = VK_SUCCESS;

    if (LDG_UNLIKELY(!fence)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ready)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ready = 0;
    if (LDG_UNLIKELY(!g_gpu_ctx.is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(fence->id >= LDG_GPU_FENCE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ldg_mut_lock(&g_gpu_ctx.mut);

    if (LDG_UNLIKELY(!g_gpu_ctx.fences[fence->id].in_use)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_NOT_FOUND; }

    vk_fence = (VkFence)g_gpu_ctx.fences[fence->id].fence;
    res = vkGetFenceStatus((VkDevice)g_gpu_ctx.device, vk_fence);
    *ready = (res == VK_SUCCESS) ? 1 : 0;

    ldg_mut_unlock(&g_gpu_ctx.mut);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_fence_destroy(ldg_gpu_fence_t *fence)
{
    VkDevice device = VK_NULL_HANDLE;
    VkFence vk_fence = VK_NULL_HANDLE;
    uint32_t fid = 0;

    if (LDG_UNLIKELY(!fence)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!g_gpu_ctx.is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(fence->id >= LDG_GPU_FENCE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    device = (VkDevice)g_gpu_ctx.device;
    fid = fence->id;
    ldg_mut_lock(&g_gpu_ctx.mut);

    if (LDG_UNLIKELY(!g_gpu_ctx.fences[fid].in_use)) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_NOT_FOUND; }

    vk_fence = (VkFence)g_gpu_ctx.fences[fid].fence;
    if (vk_fence) { vkWaitForFences(device, 1, &vk_fence, VK_TRUE, UINT64_MAX); }

    if (g_gpu_ctx.fences[fid].cmd_buff) { vkFreeCommandBuffers(device, (VkCommandPool)g_gpu_ctx.cmd_pool, 1, (VkCommandBuffer *)&g_gpu_ctx.fences[fid].cmd_buff); }

    if (g_gpu_ctx.fences[fid].desc_set) { vkFreeDescriptorSets(device, (VkDescriptorPool)g_gpu_ctx.desc_pool, 1, (VkDescriptorSet *)&g_gpu_ctx.fences[fid].desc_set); }

    if (vk_fence) { vkDestroyFence(device, vk_fence, 0x0); }

    if (LDG_UNLIKELY(memset(&g_gpu_ctx.fences[fid], 0, sizeof(ldg_gpu_fence_entry_t)) != &g_gpu_ctx.fences[fid])) { ldg_mut_unlock(&g_gpu_ctx.mut); return LDG_ERR_MEM_BAD; }

    ldg_mut_unlock(&g_gpu_ctx.mut);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_gpu_mem_stats_get(ldg_gpu_mem_stats_t *stats)
{
    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(memset(stats, 0, sizeof(ldg_gpu_mem_stats_t)) != stats)) { return LDG_ERR_MEM_BAD; }

    if (LDG_UNLIKELY(!g_gpu_ctx.is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    ldg_mut_lock(&g_gpu_ctx.mut);

    stats->vram_total = g_gpu_ctx.vram_total;
    stats->vram_used = g_gpu_ctx.vram_used;
    stats->host_total = g_gpu_ctx.host_total;
    stats->host_used = g_gpu_ctx.host_used;
    stats->slab_cunt = g_gpu_ctx.slab_cunt;
    stats->spill_cunt = g_gpu_ctx.spill_cunt;

    ldg_mut_unlock(&g_gpu_ctx.mut);
    return LDG_ERR_AOK;
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
