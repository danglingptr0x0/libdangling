#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_TRANSFORMER_NORM_IN_GAIN 0
#define LAYER_TRANSFORMER_NORM_IN_BIAS 1
#define LAYER_TRANSFORMER_W_Q 2
#define LAYER_TRANSFORMER_W_K 3
#define LAYER_TRANSFORMER_W_V 4
#define LAYER_TRANSFORMER_W_O 5
#define LAYER_TRANSFORMER_B_O 6
#define LAYER_TRANSFORMER_NORM_MID_GAIN 7
#define LAYER_TRANSFORMER_NORM_MID_BIAS 8
#define LAYER_TRANSFORMER_W_EXPAND 9
#define LAYER_TRANSFORMER_B_EXPAND 10
#define LAYER_TRANSFORMER_W_PROJ 11
#define LAYER_TRANSFORMER_B_PROJ 12
#define LAYER_TRANSFORMER_POS 13
#define LAYER_TRANSFORMER_PARAM_CUNT 13
#define LAYER_TRANSFORMER_PARAM_POS_CUNT 14
#define LAYER_TRANSFORMER_NODE_PER_STEP 30
#define LAYER_TRANSFORMER_EXPAND_FACTOR 4
#define LAYER_TRANSFORMER_TOKEN_ELEM 22
#define LAYER_TRANSFORMER_EXPAND_ELEM 3
#define LAYER_TRANSFORMER_SCORE_ELEM 3
#define LAYER_TRANSFORMER_NORM_EPS 0.00001

static uint32_t layer_transformer_norm_push(ldg_math_stat_layer_ctx_t *ctx, uint32_t in_node, uint32_t gain, uint32_t bias, uint32_t *out)
{
    uint32_t centered = 0;
    uint32_t scaled = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_row_norm_push(ctx->graph, in_node, LAYER_TRANSFORMER_NORM_EPS, &centered);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_row_mul_push(ctx->graph, centered, gain, &scaled);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_bias_add_push(ctx->graph, scaled, bias, out);
}

uint32_t ldg_math_stat_layer_transformer_shape_set(ldg_math_stat_layer_t *layer)
{
    uintptr_t dim = layer->desc.dim;
    uintptr_t seq = layer->desc.sequence_len;
    uintptr_t hd_cunt = layer->desc.hd_cunt;
    uintptr_t expand = layer->desc.expand_dim;

    if (LDG_UNLIKELY(dim == 0 || seq == 0 || hd_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(dim % hd_cunt != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY((uintptr_t)layer->in_dim != seq * dim)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (expand == 0)
    {
        expand = dim * LAYER_TRANSFORMER_EXPAND_FACTOR;
        layer->desc.expand_dim = (uint32_t)expand;
    }

    layer->out_dim = layer->in_dim;
    layer->param_cunt = LAYER_TRANSFORMER_PARAM_CUNT;
    if (layer->desc.pos_is == LDG_TRUTH_TRUE) { layer->param_cunt = LAYER_TRANSFORMER_PARAM_POS_CUNT; }

    layer->state_len = 0;
    layer->node_per_step = LAYER_TRANSFORMER_NODE_PER_STEP;
    layer->elem_per_row = (seq * (((uintptr_t)LAYER_TRANSFORMER_TOKEN_ELEM * dim) + ((uintptr_t)LAYER_TRANSFORMER_EXPAND_ELEM * expand))) + ((uintptr_t)LAYER_TRANSFORMER_SCORE_ELEM * hd_cunt * seq * seq);
    layer->work_per_row = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_transformer_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t dim = layer->desc.dim;
    uint32_t expand = layer->desc.expand_dim;
    uint32_t ret = 0;

    ret = ldg_math_stat_layer_param_vec_push(ctx, dim, LDG_MATH_STAT_LAYER_GAIN_INIT, &param[LAYER_TRANSFORMER_NORM_IN_GAIN]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_vec_push(ctx, dim, LDG_MATH_STAT_LAYER_BIAS_INIT, &param[LAYER_TRANSFORMER_NORM_IN_BIAS]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_weight_push(ctx, dim, dim, &param[LAYER_TRANSFORMER_W_Q]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_weight_push(ctx, dim, dim, &param[LAYER_TRANSFORMER_W_K]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_weight_push(ctx, dim, dim, &param[LAYER_TRANSFORMER_W_V]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_weight_push(ctx, dim, dim, &param[LAYER_TRANSFORMER_W_O]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_vec_push(ctx, dim, LDG_MATH_STAT_LAYER_BIAS_INIT, &param[LAYER_TRANSFORMER_B_O]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_vec_push(ctx, dim, LDG_MATH_STAT_LAYER_GAIN_INIT, &param[LAYER_TRANSFORMER_NORM_MID_GAIN]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_vec_push(ctx, dim, LDG_MATH_STAT_LAYER_BIAS_INIT, &param[LAYER_TRANSFORMER_NORM_MID_BIAS]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_weight_push(ctx, dim, expand, &param[LAYER_TRANSFORMER_W_EXPAND]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_vec_push(ctx, expand, LDG_MATH_STAT_LAYER_BIAS_INIT, &param[LAYER_TRANSFORMER_B_EXPAND]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_weight_push(ctx, expand, dim, &param[LAYER_TRANSFORMER_W_PROJ]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_vec_push(ctx, dim, LDG_MATH_STAT_LAYER_BIAS_INIT, &param[LAYER_TRANSFORMER_B_PROJ]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (layer->desc.pos_is != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return ldg_math_stat_layer_param_weight_push(ctx, layer->desc.sequence_len, dim, &param[LAYER_TRANSFORMER_POS]);
}

static uint32_t layer_transformer_attn_push(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t tok, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t seq = layer->desc.sequence_len;
    uint32_t hd_cunt = layer->desc.hd_cunt;
    uint32_t normed = 0;
    uint32_t query = 0;
    uint32_t key = 0;
    uint32_t val = 0;
    uint32_t score = 0;
    uint32_t logp = 0;
    uint32_t prob = 0;
    uint32_t attended = 0;
    uint32_t proj = 0;
    uint32_t proj_bias = 0;
    uint32_t ret = 0;

    ret = layer_transformer_norm_push(ctx, tok, param[LAYER_TRANSFORMER_NORM_IN_GAIN], param[LAYER_TRANSFORMER_NORM_IN_BIAS], &normed);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, normed, param[LAYER_TRANSFORMER_W_Q], &query);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, normed, param[LAYER_TRANSFORMER_W_K], &key);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, normed, param[LAYER_TRANSFORMER_W_V], &val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_attn_score_push(ctx->graph, query, key, seq, hd_cunt, &score);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_log_softmax_push(ctx->graph, score, 0x0, &logp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_exp_push(ctx->graph, logp, &prob);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    layer->node_aux = prob;

    ret = ldg_math_stat_autograd_attn_apply_push(ctx->graph, prob, val, seq, hd_cunt, &attended);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, attended, param[LAYER_TRANSFORMER_W_O], &proj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_bias_add_push(ctx->graph, proj, param[LAYER_TRANSFORMER_B_O], &proj_bias);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_add_push(ctx->graph, tok, proj_bias, out);
}

static uint32_t layer_transformer_expand_push(ldg_math_stat_layer_ctx_t *ctx, const ldg_math_stat_layer_t *layer, uint32_t tok, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t normed = 0;
    uint32_t wide = 0;
    uint32_t wide_bias = 0;
    uint32_t act = 0;
    uint32_t proj = 0;
    uint32_t proj_bias = 0;
    uint32_t ret = 0;

    ret = layer_transformer_norm_push(ctx, tok, param[LAYER_TRANSFORMER_NORM_MID_GAIN], param[LAYER_TRANSFORMER_NORM_MID_BIAS], &normed);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, normed, param[LAYER_TRANSFORMER_W_EXPAND], &wide);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_bias_add_push(ctx->graph, wide, param[LAYER_TRANSFORMER_B_EXPAND], &wide_bias);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_gelu_push(ctx->graph, wide_bias, &act);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, act, param[LAYER_TRANSFORMER_W_PROJ], &proj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_bias_add_push(ctx->graph, proj, param[LAYER_TRANSFORMER_B_PROJ], &proj_bias);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_add_push(ctx->graph, tok, proj_bias, out);
}

uint32_t ldg_math_stat_layer_transformer_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t seq = layer->desc.sequence_len;
    uint32_t dim = layer->desc.dim;
    uint32_t tok = 0;
    uint32_t pos = 0;
    uint32_t placed = 0;
    uint32_t mixed = 0;
    uint32_t expanded = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_stat_autograd_reshape_push(ctx->graph, in_node, row_cunt * seq, dim, &tok);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (layer->desc.pos_is == LDG_TRUTH_TRUE)
    {
        ret = ldg_math_stat_autograd_tile_push(ctx->graph, param[LAYER_TRANSFORMER_POS], row_cunt, &pos);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_stat_autograd_add_push(ctx->graph, tok, pos, &placed);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        tok = placed;
    }

    ret = layer_transformer_attn_push(ctx, layer, tok, &mixed);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = layer_transformer_expand_push(ctx, layer, mixed, &expanded);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_reshape_push(ctx->graph, expanded, row_cunt, layer->in_dim, out);
}
