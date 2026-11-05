#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_GRU_WZ 0
#define LAYER_GRU_UZ 1
#define LAYER_GRU_BZ 2
#define LAYER_GRU_WR 3
#define LAYER_GRU_UR 4
#define LAYER_GRU_BR 5
#define LAYER_GRU_WH 6
#define LAYER_GRU_UH 7
#define LAYER_GRU_BH 8
#define LAYER_GRU_PARAM_CUNT 9
#define LAYER_GRU_GATE_CUNT 3
#define LAYER_GRU_STATE_SLOT_CUNT 1
#define LAYER_GRU_NODE_PER_STEP 28
#define LAYER_GRU_ELEM_PER_ROW 24
#define LAYER_GRU_CUNTR_LEN 1

uint32_t ldg_math_stat_layer_gru_shape_set(ldg_math_stat_layer_t *layer)
{
    if (LDG_UNLIKELY(layer->desc.dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    layer->out_dim = layer->desc.dim;
    if (layer->desc.concat_is == LDG_TRUTH_TRUE) { layer->out_dim = layer->in_dim + layer->desc.dim; }

    layer->param_cunt = LAYER_GRU_PARAM_CUNT;
    layer->state_len = layer->desc.dim;
    if (layer->desc.stride > LDG_MATH_STAT_LAYER_STRIDE_EVERY_STEP) { layer->state_len = layer->state_len + LAYER_GRU_CUNTR_LEN; }

    layer->node_per_step = LAYER_GRU_NODE_PER_STEP;
    layer->elem_per_row = ((uintptr_t)LAYER_GRU_ELEM_PER_ROW * (uintptr_t)layer->desc.dim) + (uintptr_t)layer->in_dim + (uintptr_t)LAYER_GRU_CUNTR_LEN;
    layer->work_per_row = LAYER_GRU_CUNTR_LEN;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_gru_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t gate = 0;
    uint32_t ret = 0;

    while (gate < LAYER_GRU_PARAM_CUNT)
    {
        ret = ldg_math_stat_layer_param_weight_push(ctx, layer->in_dim, layer->desc.dim, &param[gate]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_stat_layer_param_weight_push(ctx, layer->desc.dim, layer->desc.dim, &param[gate + LAYER_GRU_UZ]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_stat_layer_param_vec_push(ctx, layer->desc.dim, LDG_MATH_STAT_LAYER_BIAS_INIT, &param[gate + LAYER_GRU_BZ]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        gate = gate + LAYER_GRU_GATE_CUNT;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_gru_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t state = 0;
    uint32_t update_pre = 0;
    uint32_t update = 0;
    uint32_t rst_pre = 0;
    uint32_t rst = 0;
    uint32_t gated = 0;
    uint32_t cand_pre = 0;
    uint32_t cand = 0;
    uint32_t diff = 0;
    uint32_t delta = 0;
    uint32_t next = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_stat_layer_param_state_ck(ctx, layer, row_cunt, LAYER_GRU_STATE_SLOT_CUNT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state = layer->node_state_in[LDG_MATH_STAT_LAYER_STATE_H];

    ret = ldg_math_stat_layer_param_gate_push(ctx, in_node, state, param[LAYER_GRU_WZ], param[LAYER_GRU_UZ], param[LAYER_GRU_BZ], &update_pre);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_sigmoid_push(ctx->graph, update_pre, &update);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_gate_push(ctx, in_node, state, param[LAYER_GRU_WR], param[LAYER_GRU_UR], param[LAYER_GRU_BR], &rst_pre);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_sigmoid_push(ctx->graph, rst_pre, &rst);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mul_push(ctx->graph, rst, state, &gated);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_gate_push(ctx, in_node, gated, param[LAYER_GRU_WH], param[LAYER_GRU_UH], param[LAYER_GRU_BH], &cand_pre);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_tanh_push(ctx->graph, cand_pre, &cand);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_sub_push(ctx->graph, cand, state, &diff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mul_push(ctx->graph, update, diff, &delta);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_add_push(ctx->graph, state, delta, &next);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_stride_apply(ctx, layer, row_cunt, LDG_MATH_STAT_LAYER_STATE_H, next);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_stride_advance(layer, row_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = layer->node_state_out[LDG_MATH_STAT_LAYER_STATE_H];

    if (layer->desc.concat_is != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return ldg_math_stat_autograd_col_concat_push(ctx->graph, in_node, layer->node_state_out[LDG_MATH_STAT_LAYER_STATE_H], out);
}
