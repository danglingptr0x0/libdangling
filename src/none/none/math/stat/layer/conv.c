#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_CONV_WEIGHT 0
#define LAYER_CONV_BIAS 1
#define LAYER_CONV_PARAM_CUNT 2
#define LAYER_CONV_NODE_PER_STEP 6
#define LAYER_CONV_ELEM_PER_OUT 3
#define LAYER_CONV_WIN_ORIG 1
#define LAYER_CONV_PAD_SIDE_CUNT 2
#define LAYER_CONV_GEOM_IN_H 0
#define LAYER_CONV_GEOM_IN_W 1
#define LAYER_CONV_GEOM_IN_C 2
#define LAYER_CONV_GEOM_OUT_C 3
#define LAYER_CONV_GEOM_KERNEL_H 4
#define LAYER_CONV_GEOM_KERNEL_W 5
#define LAYER_CONV_GEOM_STRIDE 6
#define LAYER_CONV_GEOM_PAD 7
#define LAYER_CONV_HOST_NONE 0

static uint32_t layer_conv_chan_get(const ldg_math_stat_layer_t *layer)
{
    return layer->in_dim / (layer->desc.conv_h * layer->desc.conv_w);
}

static uint32_t layer_conv_span_get(uint32_t extent, uint32_t kernel, uint32_t stride, uint32_t pad, uint32_t *out)
{
    uint32_t span = extent + (LAYER_CONV_PAD_SIDE_CUNT * pad);

    *out = 0;

    if (LDG_UNLIKELY(span < kernel)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = ((span - kernel) / stride) + LAYER_CONV_WIN_ORIG;

    return LDG_ERR_AOK;
}

static uint32_t layer_conv_tap_get(const ldg_math_stat_layer_t *layer)
{
    return layer->desc.kernel_h * layer->desc.kernel_w * layer_conv_chan_get(layer);
}

uintptr_t ldg_math_stat_layer_conv_host_len_get(const ldg_math_stat_layer_t *layer)
{
    if (layer->desc.kind != LDG_MATH_STAT_LAYER_CONV) { return LAYER_CONV_HOST_NONE; }

    return LDG_MATH_STAT_CONV_GEOM_CUNT;
}

uint32_t ldg_math_stat_layer_conv_shape_set(ldg_math_stat_layer_t *layer)
{
    uint32_t chan = 0;
    uint32_t out_h = 0;
    uint32_t out_w = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(layer->desc.dim == 0 || layer->desc.conv_h == 0 || layer->desc.conv_w == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(layer->desc.kernel_h == 0 || layer->desc.kernel_w == 0 || layer->desc.kernel_stride == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(layer->in_dim % (layer->desc.conv_h * layer->desc.conv_w) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    chan = layer_conv_chan_get(layer);

    if (LDG_UNLIKELY(chan == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = layer_conv_span_get(layer->desc.conv_h, layer->desc.kernel_h, layer->desc.kernel_stride, layer->desc.kernel_pad, &out_h);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = layer_conv_span_get(layer->desc.conv_w, layer->desc.kernel_w, layer->desc.kernel_stride, layer->desc.kernel_pad, &out_w);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    layer->out_dim = out_h * out_w * layer->desc.dim;
    layer->param_cunt = LAYER_CONV_PARAM_CUNT;
    layer->state_len = 0;
    layer->node_per_step = LAYER_CONV_NODE_PER_STEP;
    layer->elem_per_row = (uintptr_t)layer->in_dim + ((uintptr_t)LAYER_CONV_ELEM_PER_OUT * (uintptr_t)layer->out_dim) + (uintptr_t)LDG_MATH_STAT_CONV_GEOM_CUNT;
    layer->work_per_row = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_conv_host_init(ldg_math_stat_layer_t *layer)
{
    double *geom = layer->host;

    geom[LAYER_CONV_GEOM_IN_H] = (double)layer->desc.conv_h;
    geom[LAYER_CONV_GEOM_IN_W] = (double)layer->desc.conv_w;
    geom[LAYER_CONV_GEOM_IN_C] = (double)layer_conv_chan_get(layer);
    geom[LAYER_CONV_GEOM_OUT_C] = (double)layer->desc.dim;
    geom[LAYER_CONV_GEOM_KERNEL_H] = (double)layer->desc.kernel_h;
    geom[LAYER_CONV_GEOM_KERNEL_W] = (double)layer->desc.kernel_w;
    geom[LAYER_CONV_GEOM_STRIDE] = (double)layer->desc.kernel_stride;
    geom[LAYER_CONV_GEOM_PAD] = (double)layer->desc.kernel_pad;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_conv_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t ret = 0;

    ret = ldg_math_stat_layer_param_weight_push(ctx, layer_conv_tap_get(layer), layer->desc.dim, &param[LAYER_CONV_WEIGHT]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_layer_param_vec_push(ctx, layer->desc.dim, LDG_MATH_STAT_LAYER_BIAS_INIT, &param[LAYER_CONV_BIAS]);
}

uint32_t ldg_math_stat_layer_conv_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t geom_node = 0;
    uint32_t tapped = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_stat_layer_param_leaf_push(ctx, layer->host, LDG_MATH_STAT_LAYER_SCALAR_ROW, LDG_MATH_STAT_CONV_GEOM_CUNT, &geom_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_conv_push(ctx->graph, in_node, param[LAYER_CONV_WEIGHT], geom_node, &tapped);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_group_bias_push(ctx->graph, tapped, param[LAYER_CONV_BIAS], out);
}
