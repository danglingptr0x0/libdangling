#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_RMS_GAIN 0
#define LAYER_RMS_PARAM_CUNT 1
#define LAYER_RMS_NODE_PER_STEP 2
#define LAYER_RMS_ELEM_PER_ROW 2
#define LAYER_RMS_EPS 0.000001

uint32_t ldg_math_stat_layer_rms_shape_set(ldg_math_stat_layer_t *layer)
{
    layer->out_dim = layer->in_dim;
    layer->param_cunt = LAYER_RMS_PARAM_CUNT;
    layer->state_len = 0;
    layer->node_per_step = LAYER_RMS_NODE_PER_STEP;
    layer->elem_per_row = (uintptr_t)LAYER_RMS_ELEM_PER_ROW * (uintptr_t)layer->in_dim;
    layer->work_per_row = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_rms_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    uint32_t *param = ctx->param_node + layer->param_off;

    return ldg_math_stat_layer_param_vec_push(ctx, layer->in_dim, LDG_MATH_STAT_LAYER_GAIN_INIT, &param[LAYER_RMS_GAIN]);
}

uint32_t ldg_math_stat_layer_rms_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t scaled = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_stat_autograd_row_rms_push(ctx->graph, in_node, LAYER_RMS_EPS, &scaled);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_row_mul_push(ctx->graph, scaled, param[LAYER_RMS_GAIN], out);
}
