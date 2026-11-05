#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>

#include "../rl.h"
#include "layer.h"

#define LAYER_PARAM_XAVIER_NUM 6.0
#define LAYER_PARAM_RANGE_SPAN 2.0
#define LAYER_PARAM_UNIT_POS 1.0

uint32_t ldg_math_stat_layer_param_weight_push(ldg_math_stat_layer_ctx_t *ctx, uint32_t fan_in, uint32_t fan_out, uint32_t *out)
{
    double *ptr = 0x0;
    const double *end = 0x0;
    double limit = 0.0;
    double unit = 0.0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_tensor_push(ctx->graph, fan_in, fan_out, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    limit = (double)ldg_math_sqrt_get(LAYER_PARAM_XAVIER_NUM / (double)(fan_in + fan_out));

    ptr = ctx->graph->val + ctx->graph->node[*out].val_off;
    end = ptr + ((uintptr_t)fan_in * (uintptr_t)fan_out);

    while (ptr < end)
    {
        ret = ldg_math_stat_rng_uniform_sample(ctx->rng_cb, ctx->rng_ctx, &unit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *ptr = (double)((unit * LAYER_PARAM_RANGE_SPAN) - LAYER_PARAM_UNIT_POS) * limit;
        ptr++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_param_vec_push(ldg_math_stat_layer_ctx_t *ctx, uint32_t dim, double init_val, uint32_t *out)
{
    double *ptr = 0x0;
    const double *end = 0x0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_tensor_push(ctx->graph, LDG_MATH_STAT_LAYER_SCALAR_ROW, dim, LDG_TRUTH_TRUE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ptr = ctx->graph->val + ctx->graph->node[*out].val_off;
    end = ptr + dim;

    while (ptr < end)
    {
        *ptr = init_val;
        ptr++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_param_leaf_push(ldg_math_stat_layer_ctx_t *ctx, const double *src, uint32_t row_cunt, uint32_t col_cunt, uint32_t *out)
{
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_tensor_push(ctx->graph, row_cunt, col_cunt, LDG_TRUTH_FALSE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_val_set(ctx->graph, *out, src, (uintptr_t)row_cunt * (uintptr_t)col_cunt);
}

uint32_t ldg_math_stat_layer_param_gate_push(ldg_math_stat_layer_ctx_t *ctx, uint32_t in_node, uint32_t state_node, uint32_t weight, uint32_t recur, uint32_t bias, uint32_t *out)
{
    uint32_t in_term = 0;
    uint32_t state_term = 0;
    uint32_t sum = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, in_node, weight, &in_term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, state_node, recur, &state_term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_add_push(ctx->graph, in_term, state_term, &sum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_bias_add_push(ctx->graph, sum, bias, out);
}

uint32_t ldg_math_stat_layer_param_stride_apply(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t row_cunt, uint32_t slot, uint32_t cand)
{
    double *mask = ctx->work;
    uint32_t mask_node = 0;
    uint32_t diff = 0;
    uint32_t gated = 0;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (layer->desc.stride <= LDG_MATH_STAT_LAYER_STRIDE_EVERY_STEP)
    {
        layer->node_state_out[slot] = cand;

        return LDG_ERR_AOK;
    }

    while (row < row_cunt)
    {
        mask[row] = LDG_MATH_STAT_LAYER_MASK_OFF;
        if (layer->cuntr[row] <= LDG_MATH_STAT_LAYER_CUNTR_START) { mask[row] = LDG_MATH_STAT_LAYER_MASK_ON; }

        row++;
    }

    ret = ldg_math_stat_layer_param_leaf_push(ctx, mask, row_cunt, LDG_MATH_STAT_LAYER_SCALAR_COL, &mask_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_sub_push(ctx->graph, cand, layer->node_state_in[slot], &diff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_col_mul_push(ctx->graph, diff, mask_node, &gated);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_add_push(ctx->graph, layer->node_state_in[slot], gated, &layer->node_state_out[slot]);
}

uint32_t ldg_math_stat_layer_param_stride_advance(ldg_math_stat_layer_t *layer, uint32_t row_cunt)
{
    double limit = (double)layer->desc.stride;
    uint32_t row = 0;

    if (layer->desc.stride <= LDG_MATH_STAT_LAYER_STRIDE_EVERY_STEP) { return LDG_ERR_AOK; }

    while (row < row_cunt)
    {
        layer->cuntr[row] = layer->cuntr[row] + LDG_MATH_STAT_LAYER_CUNTR_STEP;
        if (layer->cuntr[row] >= limit) { layer->cuntr[row] = LDG_MATH_STAT_LAYER_CUNTR_START; }

        row++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_param_state_ck(const ldg_math_stat_layer_ctx_t *ctx, const ldg_math_stat_layer_t *layer, uint32_t row_cunt, uint32_t slot_cunt)
{
    const ldg_math_stat_autograd_node_t *node = 0x0;
    uint32_t slot = 0;

    while (slot < slot_cunt)
    {
        if (LDG_UNLIKELY(layer->node_state_in[slot] >= ctx->graph->node_cunt)) { return LDG_ERR_NOT_INIT; }

        node = &ctx->graph->node[layer->node_state_in[slot]];
        if (LDG_UNLIKELY(node->row_cunt != row_cunt || node->col_cunt != layer->desc.dim)) { return LDG_ERR_FUNC_ARG_INVALID; }

        slot++;
    }

    return LDG_ERR_AOK;
}
