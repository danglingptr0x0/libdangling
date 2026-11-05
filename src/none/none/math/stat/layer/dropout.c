#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "../rl.h"
#include "layer.h"

#define LAYER_DROPOUT_PARAM_CUNT 0
#define LAYER_DROPOUT_NODE_PER_STEP 3
#define LAYER_DROPOUT_ELEM_PER_ROW 2
#define LAYER_DROPOUT_KEEP_MAX 1.0
#define LAYER_DROPOUT_KEEP_MIN 0.0
#define LAYER_DROPOUT_DROP 0.0

uint32_t ldg_math_stat_layer_dropout_shape_set(ldg_math_stat_layer_t *layer)
{
    if (LDG_UNLIKELY(layer->in_dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(layer->desc.keep_rate <= LAYER_DROPOUT_KEEP_MIN || layer->desc.keep_rate > LAYER_DROPOUT_KEEP_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    layer->out_dim = layer->in_dim;
    layer->param_cunt = LAYER_DROPOUT_PARAM_CUNT;
    layer->state_len = 0;
    layer->node_per_step = LAYER_DROPOUT_NODE_PER_STEP;
    layer->elem_per_row = (uintptr_t)LAYER_DROPOUT_ELEM_PER_ROW * (uintptr_t)layer->in_dim;
    layer->work_per_row = layer->in_dim;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_dropout_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    if (LDG_UNLIKELY(!ctx->graph || !ctx->work || layer->in_dim == 0)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_dropout_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    double *mask = ctx->work;
    const double *end = 0x0;
    double scale = LAYER_DROPOUT_KEEP_MAX / layer->desc.keep_rate;
    double unit = 0.0;
    uint32_t mask_node = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0 || row_cunt > ctx->row_max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->train_is != LDG_TRUTH_TRUE)
    {
        *out = in_node;

        return LDG_ERR_AOK;
    }

    end = mask + ((uintptr_t)row_cunt * (uintptr_t)layer->in_dim);

    while (mask < end)
    {
        ret = ldg_math_stat_rng_uniform_sample(ctx->rng_cb, ctx->rng_ctx, &unit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *mask = LAYER_DROPOUT_DROP;
        if ((double)unit < layer->desc.keep_rate) { *mask = scale; }

        mask++;
    }

    ret = ldg_math_stat_layer_param_leaf_push(ctx, ctx->work, row_cunt, layer->in_dim, &mask_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_mul_push(ctx->graph, in_node, mask_node, out);
}
