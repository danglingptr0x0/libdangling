#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_COSINE_PROTO 0
#define LAYER_COSINE_TEMP 1
#define LAYER_COSINE_PARAM_CUNT 2
#define LAYER_COSINE_NODE_PER_STEP 4
#define LAYER_COSINE_ELEM_PER_ROW 4
#define LAYER_COSINE_TEMP_INIT 1.0
#define LAYER_COSINE_EPS 0.00001

uint32_t ldg_math_stat_layer_cosine_shape_set(ldg_math_stat_layer_t *layer)
{
    if (LDG_UNLIKELY(layer->desc.dim == 0 || layer->in_dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    layer->out_dim = layer->desc.dim;
    layer->param_cunt = LAYER_COSINE_PARAM_CUNT;
    layer->state_len = 0;
    layer->node_per_step = LAYER_COSINE_NODE_PER_STEP;
    layer->elem_per_row = (uintptr_t)LAYER_COSINE_ELEM_PER_ROW * (uintptr_t)layer->desc.dim;
    layer->work_per_row = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_cosine_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t ret = 0;

    ret = ldg_math_stat_layer_param_weight_push(ctx, layer->desc.dim, layer->in_dim, &param[LAYER_COSINE_PROTO]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_layer_param_vec_push(ctx, LDG_MATH_STAT_LAYER_SCALAR_COL, LAYER_COSINE_TEMP_INIT, &param[LAYER_COSINE_TEMP]);
}

uint32_t ldg_math_stat_layer_cosine_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t sim = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_stat_autograd_cosine_push(ctx->graph, in_node, param[LAYER_COSINE_PROTO], LAYER_COSINE_EPS, &sim);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_scalar_mul_push(ctx->graph, sim, param[LAYER_COSINE_TEMP], out);
}
