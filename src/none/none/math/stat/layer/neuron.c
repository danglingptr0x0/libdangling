#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>
#include <dangling/mem/cpy.h>

#include "layer.h"

#define LAYER_NEURON_PARAM_CUNT 2
#define LAYER_NEURON_ACTIVE_DIV 32
#define LAYER_NEURON_ACTIVE_MIN 1
#define LAYER_NEURON_NODE_PER_STEP 12
#define LAYER_NEURON_OP_ELEM_PER_ROW 4
#define LAYER_NEURON_STATE_SLOT_CUNT 2
#define LAYER_NEURON_RING_SLOT_FIRST 0
#define LAYER_NEURON_DERIV_SEED 0.0
#define LAYER_NEURON_TABLE_KIND 0
#define LAYER_NEURON_TABLE_ACT 1
#define LAYER_NEURON_TABLE_LEAK 2
#define LAYER_NEURON_TABLE_THRESHOLD 3
#define LAYER_NEURON_TABLE_RST 4
#define LAYER_NEURON_TABLE_REFRACTORY 5

static uint32_t layer_neuron_ring_len_get(const ldg_math_stat_layer_t *layer)
{
    return layer->desc.delay_max * layer->desc.dim;
}

static uintptr_t layer_neuron_pre_off_get(const ldg_math_stat_layer_t *layer)
{
    return (uintptr_t)layer_neuron_ring_len_get(layer) + ((uintptr_t)LAYER_NEURON_STATE_SLOT_CUNT * (uintptr_t)layer->desc.dim);
}

static uintptr_t layer_neuron_post_off_get(const ldg_math_stat_layer_t *layer)
{
    return layer_neuron_pre_off_get(layer) + (uintptr_t)ldg_math_stat_layer_synapse_src_width_get(layer);
}

uintptr_t ldg_math_stat_layer_neuron_elig_off_get(const ldg_math_stat_layer_t *layer)
{
    return layer_neuron_post_off_get(layer) + (uintptr_t)layer->desc.dim;
}

static uintptr_t layer_neuron_table_len_get(const ldg_math_stat_layer_t *layer)
{
    return (uintptr_t)layer->desc.dim * (uintptr_t)LDG_MATH_STAT_NEURON_PARAM_CUNT;
}

static uintptr_t layer_neuron_idx_len_get(const ldg_math_stat_layer_t *layer)
{
    return (uintptr_t)layer->synapse_cunt * (uintptr_t)LDG_MATH_STAT_SYNAPSE_IDX_CUNT;
}

uintptr_t ldg_math_stat_layer_neuron_state_host_off_get(const ldg_math_stat_layer_t *layer)
{
    return layer_neuron_table_len_get(layer) + layer_neuron_idx_len_get(layer);
}

uintptr_t ldg_math_stat_layer_neuron_host_len_get(const ldg_math_stat_layer_t *layer, uint32_t row_max)
{
    return ldg_math_stat_layer_neuron_state_host_off_get(layer) + ((uintptr_t)row_max * (uintptr_t)layer->state_len);
}

static double* layer_neuron_state_row_get(const ldg_math_stat_layer_t *layer, uint32_t row)
{
    return layer->host + ldg_math_stat_layer_neuron_state_host_off_get(layer) + ((uintptr_t)row * (uintptr_t)layer->state_len);
}

uint32_t ldg_math_stat_layer_neuron_shape_set(ldg_math_stat_layer_t *layer)
{
    uintptr_t dim = layer->desc.dim;
    uintptr_t src_width = 0;

    if (LDG_UNLIKELY(layer->desc.dim == 0 || layer->synapse_cunt == 0 || layer->desc.tick_cunt == 0 || layer->desc.delay_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(layer->desc.weight_min > layer->desc.weight_max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (layer->desc.active_cunt == 0)
    {
        layer->desc.active_cunt = layer->desc.dim / LAYER_NEURON_ACTIVE_DIV;
        if (layer->desc.active_cunt < LAYER_NEURON_ACTIVE_MIN) { layer->desc.active_cunt = LAYER_NEURON_ACTIVE_MIN; }
    }

    src_width = ldg_math_stat_layer_synapse_src_width_get(layer);

    layer->out_dim = layer->desc.dim;
    layer->param_cunt = LAYER_NEURON_PARAM_CUNT;
    layer->state_len = (uint32_t)(ldg_math_stat_layer_neuron_elig_off_get(layer) + (uintptr_t)layer->synapse_cunt);
    layer->node_per_step = LAYER_NEURON_NODE_PER_STEP;
    layer->elem_per_row = src_width + (uintptr_t)layer_neuron_ring_len_get(layer) + ((uintptr_t)LDG_MATH_STAT_NEURON_STATE_CUNT * dim) + ((uintptr_t)LAYER_NEURON_OP_ELEM_PER_ROW * dim) + layer_neuron_table_len_get(layer) + layer_neuron_idx_len_get(layer);
    layer->work_per_row = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_neuron_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    uint32_t *param = ctx->param_node + layer->param_off;
    const ldg_math_stat_synapse_desc_t *synapse = ctx->synapse + layer->synapse_off;
    double *weight = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    while (idx < layer->synapse_cunt)
    {
        ret = ldg_math_stat_layer_synapse_ck(layer, &synapse[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        idx++;
    }

    ret = ldg_math_stat_autograd_tensor_push(ctx->graph, layer->synapse_cunt, LDG_MATH_STAT_LAYER_SCALAR_COL, LDG_TRUTH_TRUE, &param[LDG_MATH_STAT_LAYER_NEURON_WEIGHT]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    weight = ctx->graph->val + ctx->graph->node[param[LDG_MATH_STAT_LAYER_NEURON_WEIGHT]].val_off;

    idx = 0;
    while (idx < layer->synapse_cunt)
    {
        weight[idx] = synapse[idx].weight;
        idx++;
    }

    ret = ldg_math_stat_autograd_tensor_push(ctx->graph, LDG_MATH_STAT_LAYER_SCALAR_ROW, layer->desc.dim, LDG_TRUTH_TRUE, &param[LDG_MATH_STAT_LAYER_NEURON_BIAS]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    weight = ctx->graph->val + ctx->graph->node[param[LDG_MATH_STAT_LAYER_NEURON_BIAS]].val_off;

    idx = 0;
    while (idx < layer->desc.dim)
    {
        weight[idx] = ctx->neuron[layer->neuron_off + idx].bias;
        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_neuron_host_init(const ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    const ldg_math_stat_neuron_desc_t *neuron = ctx->neuron + layer->neuron_off;
    double *table = layer->host;
    uint32_t idx = 0;

    while (idx < layer->desc.dim)
    {
        table[LAYER_NEURON_TABLE_KIND] = (double)neuron[idx].kind;
        table[LAYER_NEURON_TABLE_ACT] = (double)neuron[idx].act;
        table[LAYER_NEURON_TABLE_LEAK] = neuron[idx].leak;
        table[LAYER_NEURON_TABLE_THRESHOLD] = neuron[idx].threshold;
        table[LAYER_NEURON_TABLE_RST] = neuron[idx].rst_val;
        table[LAYER_NEURON_TABLE_REFRACTORY] = (double)neuron[idx].refractory;
        table = table + LDG_MATH_STAT_NEURON_PARAM_CUNT;
        idx++;
    }

    return ldg_math_stat_layer_synapse_table_fill(ctx, layer, layer->host + layer_neuron_table_len_get(layer));
}

static uint32_t layer_neuron_ring_push(ldg_math_stat_layer_ctx_t *ctx, const ldg_math_stat_layer_t *layer, uint32_t row_cunt, uint32_t *out)
{
    double *dst = 0x0;
    uint32_t ring_len = layer_neuron_ring_len_get(layer);
    uint32_t row = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_tensor_push(ctx->graph, row_cunt, ring_len, LDG_TRUTH_FALSE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dst = ctx->graph->val + ctx->graph->node[*out].val_off;

    while (row < row_cunt)
    {
        ret = ldg_mem_cpy(dst, layer_neuron_state_row_get(layer, row), (uint64_t)ring_len * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        dst = dst + ring_len;
        row++;
    }

    return LDG_ERR_AOK;
}

static uint32_t layer_neuron_state_push(ldg_math_stat_layer_ctx_t *ctx, const ldg_math_stat_layer_t *layer, uint32_t row_cunt, uint32_t *out)
{
    double *dst = 0x0;
    uintptr_t dim = layer->desc.dim;
    uintptr_t slot_len = (uintptr_t)LAYER_NEURON_STATE_SLOT_CUNT * dim;
    uint32_t row = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_tensor_push(ctx->graph, row_cunt, (uint32_t)((uintptr_t)LDG_MATH_STAT_NEURON_STATE_CUNT * dim), LDG_TRUTH_FALSE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dst = ctx->graph->val + ctx->graph->node[*out].val_off;

    while (row < row_cunt)
    {
        ret = ldg_mem_cpy(dst, layer_neuron_state_row_get(layer, row) + layer_neuron_ring_len_get(layer), (uint64_t)slot_len * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        idx = 0;
        while (idx < dim)
        {
            dst[slot_len + idx] = LAYER_NEURON_DERIV_SEED;
            idx++;
        }

        dst = dst + slot_len + dim;
        row++;
    }

    return LDG_ERR_AOK;
}

static uint32_t layer_neuron_state_pull(ldg_math_stat_layer_ctx_t *ctx, const ldg_math_stat_layer_t *layer, uint32_t state_node, uint32_t row_cunt)
{
    const double *src = ctx->graph->val + ctx->graph->node[state_node].val_off;
    uintptr_t dim = layer->desc.dim;
    uintptr_t slot_len = (uintptr_t)LAYER_NEURON_STATE_SLOT_CUNT * dim;
    uint32_t row = 0;
    uint32_t ret = 0;

    while (row < row_cunt)
    {
        ret = ldg_mem_cpy(layer_neuron_state_row_get(layer, row) + layer_neuron_ring_len_get(layer), src, (uint64_t)slot_len * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        src = src + slot_len + dim;
        row++;
    }

    return LDG_ERR_AOK;
}

static uint32_t layer_neuron_trace_run(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t src_node, uint32_t out_node, uint32_t row_cunt)
{
    const double *pre_now = ctx->graph->val + ctx->graph->node[src_node].val_off;
    const double *post_now = ctx->graph->val + ctx->graph->node[out_node].val_off;
    double *state = 0x0;
    uintptr_t src_width = ldg_math_stat_layer_synapse_src_width_get(layer);
    uintptr_t dim = layer->desc.dim;
    uintptr_t ring_len = layer_neuron_ring_len_get(layer);
    uint32_t row = 0;
    uint32_t ret = 0;

    while (row < row_cunt)
    {
        state = layer_neuron_state_row_get(layer, row);

        ret = ldg_math_stat_layer_synapse_trace_update(ctx, layer, pre_now, post_now, state + layer_neuron_pre_off_get(layer), state + layer_neuron_post_off_get(layer), state + ldg_math_stat_layer_neuron_elig_off_get(layer));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ring_len > dim)
        {
            ret = ldg_mem_mov(state + dim, state, (uint64_t)(ring_len - dim) * (uint64_t)sizeof(double));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = ldg_mem_cpy(state + LAYER_NEURON_RING_SLOT_FIRST, post_now, (uint64_t)dim * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        pre_now = pre_now + src_width;
        post_now = post_now + dim;
        row++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_neuron_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t ring_node = 0;
    uint32_t src_node = 0;
    uint32_t idx_node = 0;
    uint32_t table_node = 0;
    uint32_t state_node = 0;
    uint32_t current = 0;
    uint32_t biased = 0;
    uint32_t raw = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0 || row_cunt > ctx->row_max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = layer_neuron_ring_push(ctx, layer, row_cunt, &ring_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_col_concat_push(ctx->graph, in_node, ring_node, &src_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_leaf_push(ctx, layer->host + layer_neuron_table_len_get(layer), layer->synapse_cunt, LDG_MATH_STAT_SYNAPSE_IDX_CUNT, &idx_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_synapse_push(ctx->graph, src_node, param[LDG_MATH_STAT_LAYER_NEURON_WEIGHT], idx_node, layer->desc.dim, &current);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_bias_add_push(ctx->graph, current, param[LDG_MATH_STAT_LAYER_NEURON_BIAS], &biased);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_leaf_push(ctx, layer->host, layer->desc.dim, LDG_MATH_STAT_NEURON_PARAM_CUNT, &table_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = layer_neuron_state_push(ctx, layer, row_cunt, &state_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_neuron_push(ctx->graph, biased, table_node, state_node, layer->desc.tick_cunt, &raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    layer->node_aux = raw;

    ret = ldg_math_stat_autograd_kwin_push(ctx->graph, raw, layer->desc.active_cunt, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = layer_neuron_state_pull(ctx, layer, state_node, row_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return layer_neuron_trace_run(ctx, layer, src_node, *out, row_cunt);
}
