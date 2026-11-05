#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_LSTM_WI 0
#define LAYER_LSTM_UI 1
#define LAYER_LSTM_BI 2
#define LAYER_LSTM_WF 3
#define LAYER_LSTM_UF 4
#define LAYER_LSTM_BF 5
#define LAYER_LSTM_WO 6
#define LAYER_LSTM_UO 7
#define LAYER_LSTM_BO 8
#define LAYER_LSTM_WG 9
#define LAYER_LSTM_UG 10
#define LAYER_LSTM_BG 11
#define LAYER_LSTM_PARAM_CUNT 12
#define LAYER_LSTM_GATE_CUNT 3
#define LAYER_LSTM_STATE_SLOT_CUNT 2
#define LAYER_LSTM_NODE_PER_STEP 40
#define LAYER_LSTM_ELEM_PER_ROW 32
#define LAYER_LSTM_CUNTR_LEN 1
#define LAYER_LSTM_FORGET_BIAS_INIT 1.0

uint32_t ldg_math_stat_layer_lstm_shape_set(ldg_math_stat_layer_t *layer)
{
    if (LDG_UNLIKELY(layer->desc.dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    layer->out_dim = layer->desc.dim;
    if (layer->desc.concat_is == LDG_TRUTH_TRUE) { layer->out_dim = layer->in_dim + layer->desc.dim; }

    layer->param_cunt = LAYER_LSTM_PARAM_CUNT;
    layer->state_len = LAYER_LSTM_STATE_SLOT_CUNT * layer->desc.dim;
    if (layer->desc.stride > LDG_MATH_STAT_LAYER_STRIDE_EVERY_STEP) { layer->state_len = layer->state_len + LAYER_LSTM_CUNTR_LEN; }

    layer->node_per_step = LAYER_LSTM_NODE_PER_STEP;
    layer->elem_per_row = ((uintptr_t)LAYER_LSTM_ELEM_PER_ROW * (uintptr_t)layer->desc.dim) + (uintptr_t)layer->in_dim + (uintptr_t)LAYER_LSTM_STATE_SLOT_CUNT;
    layer->work_per_row = LAYER_LSTM_CUNTR_LEN;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_lstm_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    uint32_t *param = ctx->param_node + layer->param_off;
    double bias_init = LDG_MATH_STAT_LAYER_BIAS_INIT;
    uint32_t gate = 0;
    uint32_t ret = 0;

    while (gate < LAYER_LSTM_PARAM_CUNT)
    {
        bias_init = LDG_MATH_STAT_LAYER_BIAS_INIT;
        if (gate == LAYER_LSTM_WF) { bias_init = LAYER_LSTM_FORGET_BIAS_INIT; }

        ret = ldg_math_stat_layer_param_weight_push(ctx, layer->in_dim, layer->desc.dim, &param[gate]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_stat_layer_param_weight_push(ctx, layer->desc.dim, layer->desc.dim, &param[gate + LAYER_LSTM_UI]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_stat_layer_param_vec_push(ctx, layer->desc.dim, bias_init, &param[gate + LAYER_LSTM_BI]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        gate = gate + LAYER_LSTM_GATE_CUNT;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_lstm_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t hidden = 0;
    uint32_t cell = 0;
    uint32_t in_pre = 0;
    uint32_t in_gate = 0;
    uint32_t forget_pre = 0;
    uint32_t forget = 0;
    uint32_t out_pre = 0;
    uint32_t out_gate = 0;
    uint32_t cand_pre = 0;
    uint32_t cand = 0;
    uint32_t kept = 0;
    uint32_t written = 0;
    uint32_t cell_next = 0;
    uint32_t cell_act = 0;
    uint32_t hidden_next = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_stat_layer_param_state_ck(ctx, layer, row_cunt, LAYER_LSTM_STATE_SLOT_CUNT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    hidden = layer->node_state_in[LDG_MATH_STAT_LAYER_STATE_H];
    cell = layer->node_state_in[LDG_MATH_STAT_LAYER_STATE_C];

    ret = ldg_math_stat_layer_param_gate_push(ctx, in_node, hidden, param[LAYER_LSTM_WI], param[LAYER_LSTM_UI], param[LAYER_LSTM_BI], &in_pre);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_sigmoid_push(ctx->graph, in_pre, &in_gate);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_gate_push(ctx, in_node, hidden, param[LAYER_LSTM_WF], param[LAYER_LSTM_UF], param[LAYER_LSTM_BF], &forget_pre);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_sigmoid_push(ctx->graph, forget_pre, &forget);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_gate_push(ctx, in_node, hidden, param[LAYER_LSTM_WO], param[LAYER_LSTM_UO], param[LAYER_LSTM_BO], &out_pre);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_sigmoid_push(ctx->graph, out_pre, &out_gate);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_gate_push(ctx, in_node, hidden, param[LAYER_LSTM_WG], param[LAYER_LSTM_UG], param[LAYER_LSTM_BG], &cand_pre);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_tanh_push(ctx->graph, cand_pre, &cand);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mul_push(ctx->graph, forget, cell, &kept);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mul_push(ctx->graph, in_gate, cand, &written);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_add_push(ctx->graph, kept, written, &cell_next);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_stride_apply(ctx, layer, row_cunt, LDG_MATH_STAT_LAYER_STATE_C, cell_next);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_tanh_push(ctx->graph, layer->node_state_out[LDG_MATH_STAT_LAYER_STATE_C], &cell_act);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mul_push(ctx->graph, out_gate, cell_act, &hidden_next);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_stride_apply(ctx, layer, row_cunt, LDG_MATH_STAT_LAYER_STATE_H, hidden_next);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_stride_advance(layer, row_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = layer->node_state_out[LDG_MATH_STAT_LAYER_STATE_H];

    if (layer->desc.concat_is != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return ldg_math_stat_autograd_col_concat_push(ctx->graph, in_node, layer->node_state_out[LDG_MATH_STAT_LAYER_STATE_H], out);
}
