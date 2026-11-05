#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_SYNAPSE_SRC 0
#define LAYER_SYNAPSE_DST 1
#define LAYER_SYNAPSE_BASELINE_OFF 0.0
#define LAYER_SYNAPSE_DECAY_REST 1.0

uint32_t ldg_math_stat_layer_synapse_src_width_get(const ldg_math_stat_layer_t *layer)
{
    return layer->in_dim + (layer->desc.delay_max * layer->desc.dim);
}

uint32_t ldg_math_stat_layer_synapse_ck(const ldg_math_stat_layer_t *layer, const ldg_math_stat_synapse_desc_t *synapse)
{
    if (LDG_UNLIKELY(synapse->dst >= layer->desc.dim)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(synapse->src >= layer->in_dim + layer->desc.dim)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(synapse->plastic_is > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (synapse->src < layer->in_dim)
    {
        if (LDG_UNLIKELY(synapse->delay != LDG_MATH_STAT_LAYER_DELAY_INPUT)) { return LDG_ERR_FUNC_ARG_INVALID; }

        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY(synapse->delay < LDG_MATH_STAT_LAYER_DELAY_MIN || synapse->delay > layer->desc.delay_max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_synapse_col_get(const ldg_math_stat_layer_t *layer, const ldg_math_stat_synapse_desc_t *synapse)
{
    if (synapse->src < layer->in_dim) { return synapse->src; }

    return layer->in_dim + ((synapse->delay - LDG_MATH_STAT_LAYER_DELAY_MIN) * layer->desc.dim) + (synapse->src - layer->in_dim);
}

uint32_t ldg_math_stat_layer_synapse_table_fill(const ldg_math_stat_layer_ctx_t *ctx, const ldg_math_stat_layer_t *layer, double *dst)
{
    const ldg_math_stat_synapse_desc_t *synapse = ctx->synapse + layer->synapse_off;
    uint32_t idx = 0;

    while (idx < layer->synapse_cunt)
    {
        dst[((uintptr_t)idx * (uintptr_t)LDG_MATH_STAT_SYNAPSE_IDX_CUNT) + LAYER_SYNAPSE_SRC] = (double)ldg_math_stat_layer_synapse_col_get(layer, &synapse[idx]);
        dst[((uintptr_t)idx * (uintptr_t)LDG_MATH_STAT_SYNAPSE_IDX_CUNT) + LAYER_SYNAPSE_DST] = (double)synapse[idx].dst;
        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_synapse_trace_update(const ldg_math_stat_layer_ctx_t *ctx, const ldg_math_stat_layer_t *layer, const double *pre_now, const double *post_now, double *pre_trace, double *post_trace, double *elig)
{
    const ldg_math_stat_synapse_desc_t *synapse = ctx->synapse + layer->synapse_off;
    double decay = layer->desc.trace_decay;
    double pos = layer->desc.plastic_pos;
    double neg = layer->desc.plastic_neg;
    uint32_t src_width = ldg_math_stat_layer_synapse_src_width_get(layer);
    uint32_t col = 0;
    uint32_t dst = 0;
    uint32_t idx = 0;

    while (idx < layer->synapse_cunt)
    {
        if (synapse[idx].plastic_is == LDG_TRUTH_TRUE)
        {
            col = ldg_math_stat_layer_synapse_col_get(layer, &synapse[idx]);
            dst = synapse[idx].dst;
            elig[idx] = (elig[idx] * decay) + (pos * pre_trace[col] * post_now[dst]) - (neg * post_trace[dst] * pre_now[col]);
        }

        idx++;
    }

    idx = 0;
    while (idx < src_width)
    {
        pre_trace[idx] = (pre_trace[idx] * decay) + pre_now[idx];
        idx++;
    }

    idx = 0;
    while (idx < layer->desc.dim)
    {
        post_trace[idx] = (post_trace[idx] * decay) + post_now[idx];
        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_synapse_weight_apply(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, double reward, uint32_t row_cunt)
{
    const ldg_math_stat_synapse_desc_t *synapse = ctx->synapse + layer->synapse_off;
    const double *elig = 0x0;
    double *weight = ctx->graph->val + ctx->graph->node[ctx->param_node[layer->param_off + LDG_MATH_STAT_LAYER_NEURON_WEIGHT]].val_off;
    double modulator = reward;
    double step = 0.0;
    double cum = 0.0;
    uintptr_t state_len = layer->state_len;
    uintptr_t elig_off = ldg_math_stat_layer_neuron_elig_off_get(layer);
    uint32_t row = 0;
    uint32_t idx = 0;

    if (layer->desc.reward_decay > LAYER_SYNAPSE_BASELINE_OFF)
    {
        modulator = reward - layer->reward_mean;
        layer->reward_mean = (layer->desc.reward_decay * layer->reward_mean) + ((LAYER_SYNAPSE_DECAY_REST - layer->desc.reward_decay) * reward);
    }

    step = layer->desc.plastic_lr * modulator;

    while (idx < layer->synapse_cunt)
    {
        if (synapse[idx].plastic_is == LDG_TRUTH_TRUE)
        {
            cum = 0.0;
            row = 0;
            while (row < row_cunt)
            {
                elig = layer->host + ldg_math_stat_layer_neuron_state_host_off_get(layer) + ((uintptr_t)row * state_len) + elig_off;
                cum = cum + elig[idx];
                row++;
            }

            weight[idx] = weight[idx] + (step * cum);
            if (weight[idx] < layer->desc.weight_min) { weight[idx] = layer->desc.weight_min; }

            if (weight[idx] > layer->desc.weight_max) { weight[idx] = layer->desc.weight_max; }
        }

        idx++;
    }

    return LDG_ERR_AOK;
}
