#ifdef LDG_GPU_VULKAN

#include <string.h>
#include <vulkan/vulkan.h>

#include "state.h"

LDG_EXPORT uint32_t ldg_gpu_gfx_pipeline_create(void *vk, const ldg_gpu_gfx_pipeline_desc_t *desc, uint32_t *id)
{
    ldg_gpu_ctx_t *ctx = vk;
    VkShaderModuleCreateInfo vert_sm_info = { 0 };
    VkShaderModuleCreateInfo frag_sm_info = { 0 };
    VkPipelineShaderStageCreateInfo stages[2] = { { 0 } };
    VkPipelineVertexInputStateCreateInfo vertex_input = { 0 };
    VkVertexInputBindingDescription binding = { 0 };
    VkVertexInputAttributeDescription attrs[LDG_GPU_VERTEX_ATTR_MAX] = { { 0 } };
    VkPipelineInputAssemblyStateCreateInfo input_assembly = { 0 };
    VkPipelineViewportStateCreateInfo viewport_state = { 0 };
    VkPipelineRasterizationStateCreateInfo raster = { 0 };
    VkPipelineMultisampleStateCreateInfo multisample = { 0 };
    VkPipelineDepthStencilStateCreateInfo depth_stencil = { 0 };
    VkPipelineColorBlendAttachmentState blend_attach = { 0 };
    VkPipelineColorBlendStateCreateInfo blend = { 0 };
    VkPipelineDynamicStateCreateInfo dynamic = { 0 };
    VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkGraphicsPipelineCreateInfo gp_info = { 0 };
    VkShaderModule vert_module = VK_NULL_HANDLE;
    VkShaderModule frag_module = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
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

    device = (VkDevice)ctx->device;
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

    vert_sm_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_sm_info.codeSize = (uint64_t)desc->vert.code_size;
    vert_sm_info.pCode = desc->vert.code;

    if (LDG_UNLIKELY(vkCreateShaderModule(device, &vert_sm_info, 0x0, &vert_module) != VK_SUCCESS))
    {
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_GFX_PIPELINE_CREATE;
    }

    frag_sm_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    frag_sm_info.codeSize = (uint64_t)desc->frag.code_size;
    frag_sm_info.pCode = desc->frag.code;

    if (LDG_UNLIKELY(vkCreateShaderModule(device, &frag_sm_info, 0x0, &frag_module) != VK_SUCCESS))
    {
        vkDestroyShaderModule(device, vert_module, 0x0);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_GFX_PIPELINE_CREATE;
    }

    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert_module;
    stages[0].pName = desc->vert.entry_name;

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag_module;
    stages[1].pName = desc->frag.entry_name;

    binding.binding = 0;
    binding.stride = desc->vertex_stride;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    for (i = 0; i < desc->vertex_attr_cunt && i < LDG_GPU_VERTEX_ATTR_MAX; i++)
    {
        attrs[i].location = desc->vertex_attrs[i].location;
        attrs[i].binding = 0;
        attrs[i].format = (VkFormat)gpu_fmt_to_vk(desc->vertex_attrs[i].format);
        attrs[i].offset = desc->vertex_attrs[i].offset;
    }

    vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = desc->vertex_attr_cunt;
    vertex_input.pVertexAttributeDescriptions = attrs;

    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    if (desc->topology == LDG_GPU_TOPOLOGY_TRI_STRIP) { input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; }
    else if (desc->topology == LDG_GPU_TOPOLOGY_LINE_LIST) { input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; }
    else if (desc->topology == LDG_GPU_TOPOLOGY_LINE_STRIP) { input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; }
    else { input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; }

    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.lineWidth = 1.0f;
    if (desc->cull_mode == LDG_GPU_CULL_BACK) { raster.cullMode = VK_CULL_MODE_BACK_BIT; }
    else if (desc->cull_mode == LDG_GPU_CULL_FRONT) { raster.cullMode = VK_CULL_MODE_FRONT_BIT; }
    else { raster.cullMode = VK_CULL_MODE_NONE; }

    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = desc->depth_test ? VK_TRUE : VK_FALSE;
    depth_stencil.depthWriteEnable = desc->depth_write ? VK_TRUE : VK_FALSE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;

    blend_attach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if (desc->blend_enable)
    {
        blend_attach.blendEnable = VK_TRUE;
        blend_attach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend_attach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend_attach.colorBlendOp = VK_BLEND_OP_ADD;
        blend_attach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend_attach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blend_attach.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blend_attach;

    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamic_states;

    gp_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gp_info.stageCount = 2;
    gp_info.pStages = stages;
    gp_info.pVertexInputState = &vertex_input;
    gp_info.pInputAssemblyState = &input_assembly;
    gp_info.pViewportState = &viewport_state;
    gp_info.pRasterizationState = &raster;
    gp_info.pMultisampleState = &multisample;
    gp_info.pDepthStencilState = &depth_stencil;
    gp_info.pColorBlendState = &blend;
    gp_info.pDynamicState = &dynamic;
    gp_info.layout = (VkPipelineLayout)ctx->gfx_pipeline_layout;
    gp_info.renderPass = (VkRenderPass)ctx->renderpasses[rp_id].renderpass;
    gp_info.subpass = 0;

    if (LDG_UNLIKELY(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gp_info, 0x0, &pipeline) != VK_SUCCESS))
    {
        vkDestroyShaderModule(device, frag_module, 0x0);
        vkDestroyShaderModule(device, vert_module, 0x0);
        LDG_GPU_UNLOCK_OR_WARN(ctx);
        return LDG_ERR_GPU_GFX_PIPELINE_CREATE;
    }

    ctx->pipelines[slot].vert_module = (void *)vert_module;
    ctx->pipelines[slot].frag_module = (void *)frag_module;
    ctx->pipelines[slot].layout = ctx->gfx_pipeline_layout;
    ctx->pipelines[slot].pipeline = (void *)pipeline;
    ctx->pipelines[slot].renderpass = ctx->renderpasses[rp_id].renderpass;
    ctx->pipelines[slot].kind = LDG_GPU_PIPELINE_GRAPHICS;
    ctx->pipelines[slot].in_use = 1;

    *id = slot;

    return ldg_mut_unlock(&ctx->mut);
}

#endif
