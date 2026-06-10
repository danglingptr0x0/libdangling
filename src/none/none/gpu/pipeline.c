#ifdef LDG_GPU_VULKAN

#include <string.h>
#include <vulkan/vulkan.h>

#include "state.h"
#include <dangling/mem/alloc.h>

typedef struct ldg_gpu_gfx_scratch
{
    VkShaderModuleCreateInfo vert_sm_info;
    VkShaderModuleCreateInfo frag_sm_info;
    VkPipelineShaderStageCreateInfo stages[2];
    VkPipelineVertexInputStateCreateInfo vert_input;
    VkVertexInputBindingDescription binding;
    VkVertexInputAttributeDescription attrs[LDG_GPU_VERT_ATTR_MAX];
    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineViewportStateCreateInfo viewport_state;
    VkPipelineRasterizationStateCreateInfo raster;
    VkPipelineMultisampleStateCreateInfo multisample;
    VkPipelineDepthStencilStateCreateInfo depth_stencil;
    VkPipelineColorBlendAttachmentState blend_attach;
    VkPipelineColorBlendStateCreateInfo blend;
    VkPipelineDynamicStateCreateInfo dynamic;
    VkDynamicState dynamic_states[2];
    VkGraphicsPipelineCreateInfo gp_info;
} ldg_gpu_gfx_scratch_t;

LDG_EXPORT uint32_t ldg_gpu_gfx_pipeline_create(void *vk, const ldg_gpu_gfx_pipeline_desc_t *desc, uint32_t *id)
{
    ldg_gpu_ctx_t *ctx = vk;
    ldg_mem_pool_t *scratch = 0x0;
    ldg_gpu_gfx_scratch_t *vk_info = 0x0;
    VkShaderModule vert_module = VK_NULL_HANDLE;
    VkShaderModule frag_module = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkDevice dev = VK_NULL_HANDLE;
    uint32_t slot = UINT32_MAX;
    uint32_t rp_id = 0;
    uint32_t ret = 0;
    uint32_t i = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!id)) { return LDG_ERR_FUNC_ARG_NULL; }

    *id = UINT32_MAX;

    if (LDG_UNLIKELY(!desc->vert.code || desc->vert.code_size == 0 || !desc->vert.entry_name)) { return LDG_ERR_GPU_SPIRV_INVALID; }

    if (LDG_UNLIKELY(!desc->frag.code || desc->frag.code_size == 0 || !desc->frag.entry_name)) { return LDG_ERR_GPU_SPIRV_INVALID; }

    if (LDG_UNLIKELY(desc->vert.code_size % 4 != 0 || desc->frag.code_size % 4 != 0)) { return LDG_ERR_GPU_SPIRV_INVALID; }

    if (LDG_UNLIKELY(!ctx->is_init)) { return LDG_ERR_GPU_NOT_INIT; }

    if (LDG_UNLIKELY(!ctx->has_gfx)) { return LDG_ERR_GPU_QUEUE_NOT_FOUND; }

    rp_id = desc->renderpass_id;
    if (LDG_UNLIKELY(rp_id >= LDG_GPU_RENDERPASS_MAX || !ctx->renderpasses[rp_id].in_use)) { return LDG_ERR_GPU_RENDERPASS_NOT_FOUND; }

    ret = ldg_mem_pool_create(0, 2048, &scratch);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(scratch, sizeof(ldg_gpu_gfx_scratch_t), (void **)&vk_info);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_pool_destroy(&scratch);
        return ret;
    }

    *vk_info = (ldg_gpu_gfx_scratch_t)LDG_STRUCT_ZERO_INIT;

    dev = (VkDevice)ctx->dev;
    ret = ldg_mut_lock(&ctx->mut);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ldg_mem_pool_destroy(&scratch);
        return ret;
    }

    for (i = 0; i < LDG_GPU_PIPELINE_POOL_MAX; i++) { if (!ctx->pipelines[i].in_use)
        {
            slot = i;
            break;
        }
    }

    if (LDG_UNLIKELY(slot == UINT32_MAX))
    {
        ldg_mem_pool_destroy(&scratch);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_PIPELINE_FULL;
    }

    vk_info->vert_sm_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vk_info->vert_sm_info.codeSize = (uint64_t)desc->vert.code_size;
    vk_info->vert_sm_info.pCode = desc->vert.code;

    if (LDG_UNLIKELY(vkCreateShaderModule(dev, &vk_info->vert_sm_info, 0x0, &vert_module) != VK_SUCCESS))
    {
        ldg_mem_pool_destroy(&scratch);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_GFX_PIPELINE_CREATE;
    }

    vk_info->frag_sm_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vk_info->frag_sm_info.codeSize = (uint64_t)desc->frag.code_size;
    vk_info->frag_sm_info.pCode = desc->frag.code;

    if (LDG_UNLIKELY(vkCreateShaderModule(dev, &vk_info->frag_sm_info, 0x0, &frag_module) != VK_SUCCESS))
    {
        vkDestroyShaderModule(dev, vert_module, 0x0);
        ldg_mem_pool_destroy(&scratch);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_GFX_PIPELINE_CREATE;
    }

    vk_info->stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vk_info->stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    vk_info->stages[0].module = vert_module;
    vk_info->stages[0].pName = desc->vert.entry_name;

    vk_info->stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vk_info->stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    vk_info->stages[1].module = frag_module;
    vk_info->stages[1].pName = desc->frag.entry_name;

    vk_info->binding.binding = 0;
    vk_info->binding.stride = desc->vert_stride;
    vk_info->binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    for (i = 0; i < desc->vert_attr_cunt && i < LDG_GPU_VERT_ATTR_MAX; i++)
    {
        vk_info->attrs[i].location = desc->vert_attrs[i].location;
        vk_info->attrs[i].binding = 0;
        vk_info->attrs[i].format = (VkFormat)gpu_fmt_to_vk(desc->vert_attrs[i].fmt);
        vk_info->attrs[i].offset = desc->vert_attrs[i].offset;
    }

    vk_info->vert_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vk_info->vert_input.vertexBindingDescriptionCount = 1;
    vk_info->vert_input.pVertexBindingDescriptions = &vk_info->binding;
    vk_info->vert_input.vertexAttributeDescriptionCount = desc->vert_attr_cunt;
    vk_info->vert_input.pVertexAttributeDescriptions = vk_info->attrs;

    vk_info->input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    if (desc->topology == LDG_GPU_TOPOLOGY_TRI_STRIP) { vk_info->input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; }
    else if (desc->topology == LDG_GPU_TOPOLOGY_LINE_LIST) { vk_info->input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; }
    else if (desc->topology == LDG_GPU_TOPOLOGY_LINE_STRIP) { vk_info->input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; }
    else { vk_info->input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; }

    vk_info->viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vk_info->viewport_state.viewportCount = 1;
    vk_info->viewport_state.scissorCount = 1;

    vk_info->raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    vk_info->raster.polygonMode = VK_POLYGON_MODE_FILL;
    vk_info->raster.lineWidth = 1.0f;
    if (desc->cull_mode == LDG_GPU_CULL_BACK) { vk_info->raster.cullMode = VK_CULL_MODE_BACK_BIT; }
    else if (desc->cull_mode == LDG_GPU_CULL_FRONT) { vk_info->raster.cullMode = VK_CULL_MODE_FRONT_BIT; }
    else { vk_info->raster.cullMode = VK_CULL_MODE_NONE; }

    vk_info->raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    vk_info->multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    vk_info->multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    vk_info->depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    vk_info->depth_stencil.depthTestEnable = desc->depth_test ? VK_TRUE : VK_FALSE;
    vk_info->depth_stencil.depthWriteEnable = desc->depth_wr ? VK_TRUE : VK_FALSE;
    vk_info->depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;

    vk_info->blend_attach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if (desc->blend_enable)
    {
        vk_info->blend_attach.blendEnable = VK_TRUE;
        vk_info->blend_attach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        vk_info->blend_attach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        vk_info->blend_attach.colorBlendOp = VK_BLEND_OP_ADD;
        vk_info->blend_attach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        vk_info->blend_attach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        vk_info->blend_attach.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    vk_info->blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    vk_info->blend.attachmentCount = 1;
    vk_info->blend.pAttachments = &vk_info->blend_attach;

    vk_info->dynamic_states[0] = VK_DYNAMIC_STATE_VIEWPORT;
    vk_info->dynamic_states[1] = VK_DYNAMIC_STATE_SCISSOR;
    vk_info->dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    vk_info->dynamic.dynamicStateCount = 2;
    vk_info->dynamic.pDynamicStates = vk_info->dynamic_states;

    vk_info->gp_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    vk_info->gp_info.stageCount = 2;
    vk_info->gp_info.pStages = vk_info->stages;
    vk_info->gp_info.pVertexInputState = &vk_info->vert_input;
    vk_info->gp_info.pInputAssemblyState = &vk_info->input_assembly;
    vk_info->gp_info.pViewportState = &vk_info->viewport_state;
    vk_info->gp_info.pRasterizationState = &vk_info->raster;
    vk_info->gp_info.pMultisampleState = &vk_info->multisample;
    vk_info->gp_info.pDepthStencilState = &vk_info->depth_stencil;
    vk_info->gp_info.pColorBlendState = &vk_info->blend;
    vk_info->gp_info.pDynamicState = &vk_info->dynamic;
    vk_info->gp_info.layout = (VkPipelineLayout)ctx->gfx_pipeline_layout;
    vk_info->gp_info.renderPass = (VkRenderPass)ctx->renderpasses[rp_id].renderpass;
    vk_info->gp_info.subpass = 0;

    if (LDG_UNLIKELY(vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &vk_info->gp_info, 0x0, &pipeline) != VK_SUCCESS))
    {
        vkDestroyShaderModule(dev, frag_module, 0x0);
        vkDestroyShaderModule(dev, vert_module, 0x0);
        ldg_mem_pool_destroy(&scratch);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_GFX_PIPELINE_CREATE;
    }

    ctx->pipelines[slot].vert_module = (void *)vert_module;
    ctx->pipelines[slot].frag_module = (void *)frag_module;
    ctx->pipelines[slot].layout = ctx->gfx_pipeline_layout;
    ctx->pipelines[slot].pipeline = (void *)pipeline;
    ctx->pipelines[slot].renderpass = ctx->renderpasses[rp_id].renderpass;
    ctx->pipelines[slot].kind = LDG_GPU_PIPELINE_GFX;
    ctx->pipelines[slot].in_use = 1;

    *id = slot;

    ldg_mem_pool_destroy(&scratch);

    return ldg_mut_unlock(&ctx->mut);
}

#endif
