#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_EUCLID_PROTO 0
#define LAYER_EUCLID_TEMP 1
#define LAYER_EUCLID_PARAM_CUNT 2
#define LAYER_EUCLID_NODE_PER_STEP 4
#define LAYER_EUCLID_ELEM_PER_ROW 4
#define LAYER_EUCLID_TEMP_INIT 1.0

uint32_t ldg_math_stat_layer_euclid_shape_set(ldg_math_stat_layer_t *layer)
{
    if (LDG_UNLIKELY(layer->desc.dim == 0 || layer->in_dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    layer->out_dim = layer->desc.dim;
    layer->param_cunt = LAYER_EUCLID_PARAM_CUNT;
    layer->state_len = 0;
    layer->node_per_step = LAYER_EUCLID_NODE_PER_STEP;
    layer->elem_per_row = (uintptr_t)LAYER_EUCLID_ELEM_PER_ROW * (uintptr_t)layer->desc.dim;
    layer->work_per_row = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_euclid_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t ret = 0;

    ret = ldg_math_stat_layer_param_weight_push(ctx, layer->desc.dim, layer->in_dim, &param[LAYER_EUCLID_PROTO]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_layer_param_vec_push(ctx, LDG_MATH_STAT_LAYER_SCALAR_COL, LAYER_EUCLID_TEMP_INIT, &param[LAYER_EUCLID_TEMP]);
}

uint32_t ldg_math_stat_layer_euclid_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t sim = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_stat_autograd_euclid_push(ctx->graph, in_node, param[LAYER_EUCLID_PROTO], &sim);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_scalar_mul_push(ctx->graph, sim, param[LAYER_EUCLID_TEMP], out);
}
