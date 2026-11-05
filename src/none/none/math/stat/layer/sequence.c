#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_SEQUENCE_POOL_PARAM_CUNT 0
#define LAYER_SEQUENCE_POOL_NODE_PER_STEP 4
#define LAYER_SEQUENCE_POOL_TOKEN_ELEM 2
#define LAYER_SEQUENCE_HD_W 0
#define LAYER_SEQUENCE_HD_B 1
#define LAYER_SEQUENCE_HD_PARAM_CUNT 2
#define LAYER_SEQUENCE_HD_NODE_PER_STEP 6
#define LAYER_SEQUENCE_HD_VOCAB_ELEM 4

uint32_t ldg_math_stat_layer_sequence_pool_shape_set(ldg_math_stat_layer_t *layer)
{
    uintptr_t dim = layer->desc.dim;
    uintptr_t seq = layer->desc.sequence_len;

    if (LDG_UNLIKELY(dim == 0 || seq == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY((uintptr_t)layer->in_dim != seq * dim)) { return LDG_ERR_FUNC_ARG_INVALID; }

    layer->out_dim = layer->desc.dim;
    layer->param_cunt = LAYER_SEQUENCE_POOL_PARAM_CUNT;
    layer->state_len = 0;
    layer->node_per_step = LAYER_SEQUENCE_POOL_NODE_PER_STEP;
    layer->elem_per_row = ((uintptr_t)LAYER_SEQUENCE_POOL_TOKEN_ELEM * (uintptr_t)layer->in_dim) + seq + dim;
    layer->work_per_row = seq;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_sequence_pool_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    if (LDG_UNLIKELY(!ctx->graph || layer->desc.sequence_len == 0)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_sequence_pool_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    uint32_t seq = layer->desc.sequence_len;
    uintptr_t tok_cunt = (uintptr_t)row_cunt * (uintptr_t)seq;
    uintptr_t idx = 0;
    uint32_t tok = 0;
    uint32_t mask = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_stat_autograd_reshape_push(ctx->graph, in_node, row_cunt * seq, layer->desc.dim, &tok);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (idx < tok_cunt)
    {
        ctx->work[idx] = LDG_MATH_STAT_LAYER_MASK_ON;
        idx++;
    }

    ret = ldg_math_stat_layer_param_leaf_push(ctx, ctx->work, row_cunt * seq, LDG_MATH_STAT_LAYER_SCALAR_COL, &mask);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_row_pool_push(ctx->graph, tok, mask, seq, out);
}

uint32_t ldg_math_stat_layer_sequence_hd_shape_set(ldg_math_stat_layer_t *layer)
{
    uintptr_t dim = layer->desc.dim;
    uintptr_t seq = layer->desc.sequence_len;
    uintptr_t vocab = layer->desc.vocab_cunt;

    if (LDG_UNLIKELY(dim == 0 || seq == 0 || vocab == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY((uintptr_t)layer->in_dim != seq * dim)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(layer->desc.act != LDG_MATH_STAT_LAYER_ACT_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    layer->out_dim = (uint32_t)(seq * vocab);
    layer->param_cunt = LAYER_SEQUENCE_HD_PARAM_CUNT;
    layer->state_len = 0;
    layer->node_per_step = LAYER_SEQUENCE_HD_NODE_PER_STEP;
    layer->elem_per_row = (uintptr_t)layer->in_dim + ((uintptr_t)LAYER_SEQUENCE_HD_VOCAB_ELEM * seq * vocab);
    layer->work_per_row = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_sequence_hd_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t ret = 0;

    ret = ldg_math_stat_layer_param_weight_push(ctx, layer->desc.dim, layer->desc.vocab_cunt, &param[LAYER_SEQUENCE_HD_W]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_layer_param_vec_push(ctx, layer->desc.vocab_cunt, LDG_MATH_STAT_LAYER_BIAS_INIT, &param[LAYER_SEQUENCE_HD_B]);
}

uint32_t ldg_math_stat_layer_sequence_hd_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t seq = layer->desc.sequence_len;
    uint32_t tok = 0;
    uint32_t logit = 0;
    uint32_t logit_bias = 0;
    uint32_t logp = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_stat_autograd_reshape_push(ctx->graph, in_node, row_cunt * seq, layer->desc.dim, &tok);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, tok, param[LAYER_SEQUENCE_HD_W], &logit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_bias_add_push(ctx->graph, logit, param[LAYER_SEQUENCE_HD_B], &logit_bias);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_log_softmax_push(ctx->graph, logit_bias, ctx->out_mask, &logp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_reshape_push(ctx->graph, logp, row_cunt, layer->out_dim, out);
}
