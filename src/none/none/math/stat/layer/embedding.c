#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_EMBEDDING_TABLE 0
#define LAYER_EMBEDDING_PARAM_CUNT 1
#define LAYER_EMBEDDING_NODE_PER_STEP 2

uint32_t ldg_math_stat_layer_embedding_shape_set(ldg_math_stat_layer_t *layer)
{
    if (LDG_UNLIKELY(layer->desc.dim == 0 || layer->desc.vocab_cunt == 0 || layer->in_dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    layer->out_dim = layer->in_dim * layer->desc.dim;
    layer->param_cunt = LAYER_EMBEDDING_PARAM_CUNT;
    layer->state_len = 0;
    layer->node_per_step = LAYER_EMBEDDING_NODE_PER_STEP;
    layer->elem_per_row = layer->out_dim;
    layer->work_per_row = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_embedding_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    uint32_t *param = ctx->param_node + layer->param_off;

    return ldg_math_stat_layer_param_weight_push(ctx, layer->desc.vocab_cunt, layer->desc.dim, &param[LAYER_EMBEDDING_TABLE]);
}

uint32_t ldg_math_stat_layer_embedding_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ldg_math_stat_autograd_embedding_push(ctx->graph, in_node, param[LAYER_EMBEDDING_TABLE], out);
}
