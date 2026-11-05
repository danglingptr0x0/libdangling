#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_NORM_GAIN 0
#define LAYER_NORM_BIAS 1
#define LAYER_NORM_PARAM_CUNT 2
#define LAYER_NORM_NODE_PER_STEP 4
#define LAYER_NORM_ELEM_PER_ROW 4
#define LAYER_NORM_EPS 0.00001

uint32_t ldg_math_stat_layer_norm_shape_set(ldg_math_stat_layer_t *layer)
{
    layer->out_dim = layer->in_dim;
    layer->param_cunt = LAYER_NORM_PARAM_CUNT;
    layer->state_len = 0;
    layer->node_per_step = LAYER_NORM_NODE_PER_STEP;
    layer->elem_per_row = (uintptr_t)LAYER_NORM_ELEM_PER_ROW * (uintptr_t)layer->in_dim;
    layer->work_per_row = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_norm_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t ret = 0;

    ret = ldg_math_stat_layer_param_vec_push(ctx, layer->in_dim, LDG_MATH_STAT_LAYER_GAIN_INIT, &param[LAYER_NORM_GAIN]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_layer_param_vec_push(ctx, layer->in_dim, LDG_MATH_STAT_LAYER_BIAS_INIT, &param[LAYER_NORM_BIAS]);
}

uint32_t ldg_math_stat_layer_norm_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t centered = 0;
    uint32_t scaled = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_stat_autograd_row_norm_push(ctx->graph, in_node, LAYER_NORM_EPS, &centered);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_row_mul_push(ctx->graph, centered, param[LAYER_NORM_GAIN], &scaled);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_bias_add_push(ctx->graph, scaled, param[LAYER_NORM_BIAS], out);
}
