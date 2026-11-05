#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_BABE_PARAM_CUNT 0
#define LAYER_BABE_NODE_PER_STEP 2
#define LAYER_BABE_ROW 1
#define LAYER_BABE_HOST_NONE 0
#define LAYER_BABE_FIXED_SCALE 65536.0
#define LAYER_BABE_RATE_MIN 0.0
#define LAYER_BABE_RATE_MAX 1.0

uintptr_t ldg_math_stat_layer_babe_host_len_get(const ldg_math_stat_layer_t *layer)
{
    if (layer->desc.kind != LDG_MATH_STAT_LAYER_BABE || !layer->desc.babe) { return LAYER_BABE_HOST_NONE; }

    return (uintptr_t)layer->desc.babe->in_cunt + (uintptr_t)layer->desc.babe->readout_cunt;
}

uint32_t ldg_math_stat_layer_babe_shape_set(ldg_math_stat_layer_t *layer)
{
    const ldg_math_stat_babe_t *babe = layer->desc.babe;

    if (LDG_UNLIKELY(!babe)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(layer->in_dim != babe->in_cunt || layer->desc.dim != babe->readout_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    layer->out_dim = babe->readout_cunt;
    layer->param_cunt = LAYER_BABE_PARAM_CUNT;
    layer->state_len = babe->readout_cunt;
    layer->node_per_step = LAYER_BABE_NODE_PER_STEP;
    layer->elem_per_row = babe->readout_cunt;
    layer->work_per_row = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_babe_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    if (LDG_UNLIKELY(!ctx->graph || !layer->host || !layer->desc.babe)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_babe_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    ldg_math_stat_babe_t *babe = layer->desc.babe;
    ldg_math_stat_babe_fixed_t *src = (ldg_math_stat_babe_fixed_t *)layer->host;
    ldg_math_stat_babe_fixed_t *readout = src + babe->in_cunt;
    const double *lane = 0x0;
    double *dst = 0x0;
    double rate = 0.0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->live_is != LDG_TRUTH_TRUE)
    {
        if (LDG_UNLIKELY(layer->node_state_in[LDG_MATH_STAT_LAYER_STATE_H] == LDG_MATH_STAT_AUTOGRAD_NODE_NONE)) { return LDG_ERR_NOT_INIT; }

        ret = ldg_math_stat_layer_param_state_ck(ctx, layer, row_cunt, LAYER_BABE_ROW);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        layer->node_state_out[LDG_MATH_STAT_LAYER_STATE_H] = layer->node_state_in[LDG_MATH_STAT_LAYER_STATE_H];
        *out = layer->node_state_in[LDG_MATH_STAT_LAYER_STATE_H];

        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY(row_cunt != LAYER_BABE_ROW)) { return LDG_ERR_FUNC_ARG_INVALID; }

    lane = ctx->graph->val + ctx->graph->node[in_node].val_off;

    while (idx < babe->in_cunt)
    {
        rate = lane[idx];
        if (rate < LAYER_BABE_RATE_MIN) { rate = LAYER_BABE_RATE_MIN; }

        if (rate > LAYER_BABE_RATE_MAX) { rate = LAYER_BABE_RATE_MAX; }

        src[idx] = (ldg_math_stat_babe_fixed_t)(rate * LAYER_BABE_FIXED_SCALE);
        idx++;
    }

    ret = ldg_math_stat_babe_step(babe, src, readout, babe->readout_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_tensor_push(ctx->graph, LAYER_BABE_ROW, babe->readout_cunt, LDG_TRUTH_FALSE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dst = ctx->graph->val + ctx->graph->node[*out].val_off;

    idx = 0;
    while (idx < babe->readout_cunt)
    {
        dst[idx] = (double)readout[idx] / (double)LAYER_BABE_FIXED_SCALE;
        idx++;
    }

    layer->node_state_out[LDG_MATH_STAT_LAYER_STATE_H] = *out;

    return LDG_ERR_AOK;
}
