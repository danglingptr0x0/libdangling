#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_POOL_PARAM_CUNT 0
#define LAYER_POOL_NODE_PER_STEP 2
#define LAYER_POOL_WIN_ORIG 1
#define LAYER_POOL_HOST_NONE 0
#define LAYER_POOL_GEOM_IN_H 0
#define LAYER_POOL_GEOM_IN_W 1
#define LAYER_POOL_GEOM_IN_C 2
#define LAYER_POOL_GEOM_KERNEL_H 3
#define LAYER_POOL_GEOM_KERNEL_W 4
#define LAYER_POOL_GEOM_STRIDE 5

static uint32_t layer_pool_chan_get(const ldg_math_stat_layer_t *layer)
{
    return layer->in_dim / (layer->desc.conv_h * layer->desc.conv_w);
}

static uint32_t layer_pool_span_get(uint32_t extent, uint32_t kernel, uint32_t stride)
{
    return ((extent - kernel) / stride) + LAYER_POOL_WIN_ORIG;
}

uintptr_t ldg_math_stat_layer_pool_host_len_get(const ldg_math_stat_layer_t *layer)
{
    if (layer->desc.kind != LDG_MATH_STAT_LAYER_POOL) { return LAYER_POOL_HOST_NONE; }

    return LDG_MATH_STAT_POOL_GEOM_CUNT;
}

uint32_t ldg_math_stat_layer_pool_shape_set(ldg_math_stat_layer_t *layer)
{
    uint32_t chan = 0;

    if (LDG_UNLIKELY(layer->desc.conv_h == 0 || layer->desc.conv_w == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(layer->desc.kernel_h == 0 || layer->desc.kernel_w == 0 || layer->desc.kernel_stride == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(layer->desc.conv_h < layer->desc.kernel_h || layer->desc.conv_w < layer->desc.kernel_w)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(layer->in_dim % (layer->desc.conv_h * layer->desc.conv_w) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    chan = layer_pool_chan_get(layer);

    if (LDG_UNLIKELY(chan == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    layer->out_dim = layer_pool_span_get(layer->desc.conv_h, layer->desc.kernel_h, layer->desc.kernel_stride) * layer_pool_span_get(layer->desc.conv_w, layer->desc.kernel_w, layer->desc.kernel_stride) * chan;
    layer->param_cunt = LAYER_POOL_PARAM_CUNT;
    layer->state_len = 0;
    layer->node_per_step = LAYER_POOL_NODE_PER_STEP;
    layer->elem_per_row = (uintptr_t)layer->out_dim + (uintptr_t)LDG_MATH_STAT_POOL_GEOM_CUNT;
    layer->work_per_row = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_pool_host_init(ldg_math_stat_layer_t *layer)
{
    double *geom = layer->host;

    geom[LAYER_POOL_GEOM_IN_H] = (double)layer->desc.conv_h;
    geom[LAYER_POOL_GEOM_IN_W] = (double)layer->desc.conv_w;
    geom[LAYER_POOL_GEOM_IN_C] = (double)layer_pool_chan_get(layer);
    geom[LAYER_POOL_GEOM_KERNEL_H] = (double)layer->desc.kernel_h;
    geom[LAYER_POOL_GEOM_KERNEL_W] = (double)layer->desc.kernel_w;
    geom[LAYER_POOL_GEOM_STRIDE] = (double)layer->desc.kernel_stride;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_pool_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    if (LDG_UNLIKELY(!ctx->graph || !layer->host)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_pool_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    uint32_t geom_node = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_stat_layer_param_leaf_push(ctx, layer->host, LDG_MATH_STAT_LAYER_SCALAR_ROW, LDG_MATH_STAT_POOL_GEOM_CUNT, &geom_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_pool_push(ctx->graph, in_node, geom_node, layer->desc.pool_max_is, out);
}
