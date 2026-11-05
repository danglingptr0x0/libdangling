#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>

#include "layer/layer.h"

#define MODEL_ROW_MAX_DEFAULT 1
#define MODEL_STEP_MAX_DEFAULT 1
#define MODEL_LAYER_CAP_DEFAULT 8
#define MODEL_PARAM_CAP_DEFAULT 4
#define MODEL_EXTRA_NODE_CAP_DEFAULT 256
#define MODEL_EXTRA_ELEM_CAP_DEFAULT 4096
#define MODEL_NEURON_CAP_DEFAULT 0
#define MODEL_SYNAPSE_CAP_DEFAULT 0
#define MODEL_HD_CUNT_DEFAULT 1
#define MODEL_TICK_CUNT_DEFAULT 1
#define MODEL_DELAY_MAX_DEFAULT 1
#define MODEL_TRACE_DECAY_DEFAULT 0.9
#define MODEL_REWARD_DECAY_DEFAULT 0.99
#define MODEL_PLASTIC_LR_DEFAULT 0.001
#define MODEL_PLASTIC_POS_DEFAULT 1.0
#define MODEL_PLASTIC_NEG_DEFAULT 1.0
#define MODEL_WEIGHT_MIN_DEFAULT (-1.0)
#define MODEL_WEIGHT_MAX_DEFAULT 1.0
#define MODEL_NEURON_BIAS_DEFAULT 0.0
#define MODEL_NEURON_LEAK_DEFAULT 0.9
#define MODEL_NEURON_THRESHOLD_DEFAULT 1.0
#define MODEL_NEURON_RST_DEFAULT 0.0
#define MODEL_NEURON_REFRACTORY_DEFAULT 1
#define MODEL_MEMBER_CUNT_DEFAULT 1
#define MODEL_SAMPLE_CAP_DEFAULT 256
#define MODEL_KEEP_RATE_DEFAULT 0.5
#define MODEL_REWARD_MEAN_SEED 0.0
#define MODEL_NODE_SLACK 64
#define MODEL_ELEM_SLACK 256
#define MODEL_NODE_PER_STEP_BASE 2
#define MODEL_OUT_ELEM_PER_LAYER 3
#define MODEL_NODE_PER_LAYER_STEP 6
#define MODEL_LAYER_FIRST 0
#define MODEL_GRU_SLOT_CUNT 1
#define MODEL_LSTM_SLOT_CUNT 2
#define MODEL_SLOT_NONE 0
#define MODEL_HOST_NONE 0
#define MODEL_SEQUENCE_LEN_DEFAULT 1
#define MODEL_MASK_SUM_SEED 0.0
#define MODEL_LAST_OFF 1

static uint8_t model_layer_recur_is(const ldg_math_stat_layer_t *layer)
{
    if (layer->desc.kind == LDG_MATH_STAT_LAYER_GRU || layer->desc.kind == LDG_MATH_STAT_LAYER_LSTM) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t model_layer_slot_cunt_get(const ldg_math_stat_layer_t *layer)
{
    if (layer->desc.kind == LDG_MATH_STAT_LAYER_GRU) { return MODEL_GRU_SLOT_CUNT; }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_LSTM) { return MODEL_LSTM_SLOT_CUNT; }

    return MODEL_SLOT_NONE;
}

static uint8_t model_layer_cuntr_is(const ldg_math_stat_layer_t *layer)
{
    if (model_layer_recur_is(layer) == LDG_TRUTH_TRUE && layer->desc.stride > LDG_MATH_STAT_LAYER_STRIDE_EVERY_STEP) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uintptr_t model_layer_fan_max_get(const ldg_math_stat_layer_t *layer)
{
    uintptr_t fan_max = layer->in_dim;

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_TRANSFORMER || layer->desc.kind == LDG_MATH_STAT_LAYER_SEQUENCE_HD || layer->desc.kind == LDG_MATH_STAT_LAYER_SEQUENCE_POOL)
    {
        fan_max = layer->desc.dim;
        if (fan_max < (uintptr_t)layer->desc.expand_dim) { fan_max = layer->desc.expand_dim; }

        if (fan_max < (uintptr_t)layer->desc.vocab_cunt) { fan_max = layer->desc.vocab_cunt; }

        if (fan_max < (uintptr_t)layer->desc.sequence_len) { fan_max = layer->desc.sequence_len; }

        return fan_max;
    }

    if (fan_max < (uintptr_t)layer->desc.dim) { fan_max = layer->desc.dim; }

    return fan_max;
}

static uint32_t model_layer_shape_set(ldg_math_stat_layer_t *layer)
{
    if (layer->desc.kind == LDG_MATH_STAT_LAYER_DENSE) { return ldg_math_stat_layer_dense_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_NORM) { return ldg_math_stat_layer_norm_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_RMS_NORM) { return ldg_math_stat_layer_rms_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_GRU) { return ldg_math_stat_layer_gru_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_LSTM) { return ldg_math_stat_layer_lstm_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_ATTN) { return ldg_math_stat_layer_attn_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_NEURON) { return ldg_math_stat_layer_neuron_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_CONV) { return ldg_math_stat_layer_conv_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_LINREG) { return ldg_math_stat_layer_linreg_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_SVM) { return ldg_math_stat_layer_svm_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_COSINE) { return ldg_math_stat_layer_cosine_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_EUCLID) { return ldg_math_stat_layer_euclid_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_POOL) { return ldg_math_stat_layer_pool_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_EMBEDDING) { return ldg_math_stat_layer_embedding_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_DROPOUT) { return ldg_math_stat_layer_dropout_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_TRANSFORMER) { return ldg_math_stat_layer_transformer_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_SEQUENCE_POOL) { return ldg_math_stat_layer_sequence_pool_shape_set(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_SEQUENCE_HD) { return ldg_math_stat_layer_sequence_hd_shape_set(layer); }

    return LDG_ERR_UNSUPPORTED;
}

static uint32_t model_layer_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    if (layer->desc.kind == LDG_MATH_STAT_LAYER_DENSE) { return ldg_math_stat_layer_dense_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_NORM) { return ldg_math_stat_layer_norm_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_RMS_NORM) { return ldg_math_stat_layer_rms_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_GRU) { return ldg_math_stat_layer_gru_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_LSTM) { return ldg_math_stat_layer_lstm_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_ATTN) { return ldg_math_stat_layer_attn_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_NEURON) { return ldg_math_stat_layer_neuron_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_CONV) { return ldg_math_stat_layer_conv_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_LINREG) { return ldg_math_stat_layer_linreg_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_SVM) { return ldg_math_stat_layer_svm_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_COSINE) { return ldg_math_stat_layer_cosine_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_EUCLID) { return ldg_math_stat_layer_euclid_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_POOL) { return ldg_math_stat_layer_pool_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_EMBEDDING) { return ldg_math_stat_layer_embedding_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_DROPOUT) { return ldg_math_stat_layer_dropout_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_TRANSFORMER) { return ldg_math_stat_layer_transformer_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_SEQUENCE_POOL) { return ldg_math_stat_layer_sequence_pool_build(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_SEQUENCE_HD) { return ldg_math_stat_layer_sequence_hd_build(ctx, layer); }

    return LDG_ERR_UNSUPPORTED;
}

static uint32_t model_layer_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    if (layer->desc.kind == LDG_MATH_STAT_LAYER_DENSE) { return ldg_math_stat_layer_dense_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_NORM) { return ldg_math_stat_layer_norm_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_RMS_NORM) { return ldg_math_stat_layer_rms_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_GRU) { return ldg_math_stat_layer_gru_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_LSTM) { return ldg_math_stat_layer_lstm_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_ATTN) { return ldg_math_stat_layer_attn_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_NEURON) { return ldg_math_stat_layer_neuron_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_CONV) { return ldg_math_stat_layer_conv_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_LINREG) { return ldg_math_stat_layer_linreg_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_SVM) { return ldg_math_stat_layer_svm_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_COSINE) { return ldg_math_stat_layer_cosine_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_EUCLID) { return ldg_math_stat_layer_euclid_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_POOL) { return ldg_math_stat_layer_pool_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_EMBEDDING) { return ldg_math_stat_layer_embedding_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_DROPOUT) { return ldg_math_stat_layer_dropout_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_TRANSFORMER) { return ldg_math_stat_layer_transformer_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_SEQUENCE_POOL) { return ldg_math_stat_layer_sequence_pool_forward(ctx, layer, in_node, row_cunt, out); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_SEQUENCE_HD) { return ldg_math_stat_layer_sequence_hd_forward(ctx, layer, in_node, row_cunt, out); }

    return LDG_ERR_UNSUPPORTED;
}

static uint8_t model_layer_neuron_is(const ldg_math_stat_layer_t *layer)
{
    if (layer->desc.kind == LDG_MATH_STAT_LAYER_NEURON) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uintptr_t model_layer_host_len_get(const ldg_math_stat_layer_t *layer, uint32_t row_max)
{
    if (layer->desc.kind == LDG_MATH_STAT_LAYER_NEURON) { return ldg_math_stat_layer_neuron_host_len_get(layer, row_max); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_CONV) { return ldg_math_stat_layer_conv_host_len_get(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_LINREG) { return ldg_math_stat_layer_linreg_host_len_get(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_SVM) { return ldg_math_stat_layer_svm_host_len_get(layer, row_max); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_POOL) { return ldg_math_stat_layer_pool_host_len_get(layer); }

    return MODEL_HOST_NONE;
}

static uint32_t model_layer_host_init(const ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    if (layer->desc.kind == LDG_MATH_STAT_LAYER_NEURON) { return ldg_math_stat_layer_neuron_host_init(ctx, layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_CONV) { return ldg_math_stat_layer_conv_host_init(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_SVM) { return ldg_math_stat_layer_svm_rst(layer); }

    if (layer->desc.kind == LDG_MATH_STAT_LAYER_POOL) { return ldg_math_stat_layer_pool_host_init(layer); }

    return LDG_ERR_AOK;
}

static uint32_t model_built_ck(const ldg_math_stat_model_t *model)
{
    if (LDG_UNLIKELY(model->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(model->is_built != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

static uint32_t model_layer_kind_ck(const ldg_math_stat_model_t *model, uint32_t layer, uint32_t kind, ldg_math_stat_layer_t **out)
{
    uint32_t ret = 0;

    *out = 0x0;

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(layer >= model->layer_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(model->layer[layer].desc.kind != kind)) { return LDG_ERR_UNSUPPORTED; }

    *out = &model->layer[layer];

    return LDG_ERR_AOK;
}

static uint32_t model_out_mask_ck(const ldg_math_stat_model_t *model)
{
    const ldg_math_stat_layer_t *tail = 0x0;

    if (!model->out_mask) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(model->layer_cunt == 0)) { return LDG_ERR_UNSUPPORTED; }

    tail = &model->layer[model->layer_cunt - MODEL_LAST_OFF];

    if (tail->desc.act == LDG_MATH_STAT_LAYER_ACT_SOFTMAX || tail->desc.kind == LDG_MATH_STAT_LAYER_SEQUENCE_HD) { return LDG_ERR_AOK; }

    return LDG_ERR_UNSUPPORTED;
}

static uint32_t model_act_push(ldg_math_stat_autograd_t *graph, uint32_t act, uint32_t in_node, const uint8_t *mask, uint32_t *out)
{
    uint32_t logp = 0;
    uint32_t ret = 0;

    if (act == LDG_MATH_STAT_LAYER_ACT_TANH) { return ldg_math_stat_autograd_tanh_push(graph, in_node, out); }

    if (act == LDG_MATH_STAT_LAYER_ACT_SIGMOID) { return ldg_math_stat_autograd_sigmoid_push(graph, in_node, out); }

    if (act == LDG_MATH_STAT_LAYER_ACT_RELU) { return ldg_math_stat_autograd_relu_push(graph, in_node, out); }

    if (act == LDG_MATH_STAT_LAYER_ACT_GELU) { return ldg_math_stat_autograd_gelu_push(graph, in_node, out); }

    if (act == LDG_MATH_STAT_LAYER_ACT_SOFTMAX)
    {
        ret = ldg_math_stat_autograd_log_softmax_push(graph, in_node, mask, &logp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return ldg_math_stat_autograd_exp_push(graph, logp, out);
    }

    *out = in_node;

    return LDG_ERR_AOK;
}

static void model_ctx_get(ldg_math_stat_model_t *model, ldg_math_stat_layer_ctx_t *out)
{
    out->graph = model->graph;
    out->param_node = model->param_node;
    out->work = model->work;
    out->neuron = model->neuron;
    out->synapse = model->synapse;
    out->rng_cb = model->rng_cb;
    out->rng_ctx = model->rng_ctx;
    out->out_mask = 0x0;
    out->row_max = model->conf.row_max;
    out->train_is = model->train_is;
    out->live_is = model->live_is;
}

static uint32_t model_alloc(uint64_t size, void **out)
{
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc(size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

static void model_free(void *ptr)
{
    if (ptr) { ldg_mem_dealloc(ptr); }
}

uint32_t ldg_math_stat_model_conf_default_get(ldg_math_stat_model_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->extra_elem_cap = MODEL_EXTRA_ELEM_CAP_DEFAULT;
    out->in_dim = 0;
    out->row_max = MODEL_ROW_MAX_DEFAULT;
    out->step_max = MODEL_STEP_MAX_DEFAULT;
    out->layer_cap = MODEL_LAYER_CAP_DEFAULT;
    out->param_cap = MODEL_PARAM_CAP_DEFAULT;
    out->extra_node_cap = MODEL_EXTRA_NODE_CAP_DEFAULT;
    out->neuron_cap = MODEL_NEURON_CAP_DEFAULT;
    out->synapse_cap = MODEL_SYNAPSE_CAP_DEFAULT;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_neuron_desc_default_get(uint32_t kind, ldg_math_stat_neuron_desc_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, (uint64_t)sizeof(ldg_math_stat_neuron_desc_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(kind >= LDG_MATH_STAT_NEURON_KIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out->kind = kind;
    out->bias = MODEL_NEURON_BIAS_DEFAULT;
    out->act = LDG_MATH_STAT_LAYER_ACT_TANH;
    out->leak = MODEL_NEURON_LEAK_DEFAULT;
    out->threshold = MODEL_NEURON_THRESHOLD_DEFAULT;
    out->rst_val = MODEL_NEURON_RST_DEFAULT;
    out->refractory = MODEL_NEURON_REFRACTORY_DEFAULT;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_layer_desc_default_get(uint32_t kind, uint32_t dim, ldg_math_stat_layer_desc_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, (uint64_t)sizeof(ldg_math_stat_layer_desc_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(kind >= LDG_MATH_STAT_LAYER_KIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out->kind = kind;
    out->dim = dim;
    out->act = LDG_MATH_STAT_LAYER_ACT_NONE;
    out->stride = LDG_MATH_STAT_LAYER_STRIDE_EVERY_STEP;
    out->hd_cunt = MODEL_HD_CUNT_DEFAULT;
    out->tick_cunt = MODEL_TICK_CUNT_DEFAULT;
    out->delay_max = MODEL_DELAY_MAX_DEFAULT;
    out->trace_decay = MODEL_TRACE_DECAY_DEFAULT;
    out->reward_decay = MODEL_REWARD_DECAY_DEFAULT;
    out->plastic_lr = MODEL_PLASTIC_LR_DEFAULT;
    out->plastic_pos = MODEL_PLASTIC_POS_DEFAULT;
    out->plastic_neg = MODEL_PLASTIC_NEG_DEFAULT;
    out->weight_min = MODEL_WEIGHT_MIN_DEFAULT;
    out->weight_max = MODEL_WEIGHT_MAX_DEFAULT;
    out->member_cunt = MODEL_MEMBER_CUNT_DEFAULT;
    out->sample_cap = MODEL_SAMPLE_CAP_DEFAULT;
    out->keep_rate = MODEL_KEEP_RATE_DEFAULT;
    out->concat_is = LDG_TRUTH_FALSE;
    out->qk_norm_is = LDG_TRUTH_FALSE;
    out->residual_is = LDG_TRUTH_FALSE;
    out->pool_max_is = LDG_TRUTH_FALSE;
    out->frozen_is = LDG_TRUTH_FALSE;
    out->pos_is = LDG_TRUTH_FALSE;
    out->sequence_len = MODEL_SEQUENCE_LEN_DEFAULT;
    out->expand_dim = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_init(ldg_math_stat_model_t *model, const ldg_math_stat_model_conf_t *conf, ldg_math_stat_autograd_t *graph, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx)
{
    void *ptr = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !conf || !graph || !rng_cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->in_dim == 0 || conf->row_max == 0 || conf->step_max == 0 || conf->layer_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_zero(model, (uint64_t)sizeof(ldg_math_stat_model_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    model->conf = *conf;
    model->graph = graph;
    model->rng_cb = rng_cb;
    model->rng_ctx = rng_ctx;
    model->node_in = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;

    ret = model_alloc((uint64_t)conf->layer_cap * (uint64_t)sizeof(ldg_math_stat_layer_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    model->layer = (ldg_math_stat_layer_t *)ptr;

    if (conf->param_cap > 0)
    {
        ret = model_alloc((uint64_t)conf->param_cap * (uint64_t)sizeof(ldg_math_stat_model_param_t), &ptr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        model->param = (ldg_math_stat_model_param_t *)ptr;
    }

    if (conf->neuron_cap > 0)
    {
        ret = model_alloc((uint64_t)conf->neuron_cap * (uint64_t)sizeof(ldg_math_stat_neuron_desc_t), &ptr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        model->neuron = (ldg_math_stat_neuron_desc_t *)ptr;
    }

    if (conf->synapse_cap > 0)
    {
        ret = model_alloc((uint64_t)conf->synapse_cap * (uint64_t)sizeof(ldg_math_stat_synapse_desc_t), &ptr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        model->synapse = (ldg_math_stat_synapse_desc_t *)ptr;
    }

    model->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_shutdown(ldg_math_stat_model_t *model)
{
    if (LDG_UNLIKELY(!model)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (model->is_init != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    model_free(model->host);
    model_free(model->cuntr);
    model_free(model->work);
    model_free(model->param_node);
    model_free(model->synapse);
    model_free(model->neuron);
    model_free(model->param);
    model_free(model->layer);

    return ldg_mem_zero(model, (uint64_t)sizeof(ldg_math_stat_model_t));
}

uint32_t ldg_math_stat_model_layer_add(ldg_math_stat_model_t *model, const ldg_math_stat_layer_desc_t *desc, uint32_t *out)
{
    ldg_math_stat_layer_t *layer = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !desc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (LDG_UNLIKELY(model->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(model->is_built == LDG_TRUTH_TRUE)) { return LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(desc->kind >= LDG_MATH_STAT_LAYER_KIND_CUNT || desc->act >= LDG_MATH_STAT_LAYER_ACT_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->concat_is > LDG_TRUTH_TRUE || desc->qk_norm_is > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->residual_is > LDG_TRUTH_TRUE || desc->pool_max_is > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(model->layer_cunt >= model->conf.layer_cap)) { return LDG_ERR_FULL; }

    layer = &model->layer[model->layer_cunt];

    ret = ldg_mem_zero(layer, (uint64_t)sizeof(ldg_math_stat_layer_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    layer->desc = *desc;
    if (layer->desc.stride == 0) { layer->desc.stride = LDG_MATH_STAT_LAYER_STRIDE_EVERY_STEP; }

    layer->node_out = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;
    layer->node_aux = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;
    layer->node_state_in[LDG_MATH_STAT_LAYER_STATE_H] = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;
    layer->node_state_in[LDG_MATH_STAT_LAYER_STATE_C] = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;
    layer->node_state_out[LDG_MATH_STAT_LAYER_STATE_H] = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;
    layer->node_state_out[LDG_MATH_STAT_LAYER_STATE_C] = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;

    if (model_layer_neuron_is(layer) == LDG_TRUTH_TRUE)
    {
        if (LDG_UNLIKELY(desc->dim == 0 || desc->synapse_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(desc->dim > model->conf.neuron_cap - model->neuron_cunt)) { return LDG_ERR_FULL; }

        if (LDG_UNLIKELY(desc->synapse_cap > model->conf.synapse_cap - model->synapse_cunt)) { return LDG_ERR_FULL; }

        layer->neuron_off = model->neuron_cunt;
        layer->synapse_off = model->synapse_cunt;
        layer->synapse_cunt = 0;
        model->neuron_cunt = model->neuron_cunt + desc->dim;
        model->synapse_cunt = model->synapse_cunt + desc->synapse_cap;

        idx = 0;
        while (idx < desc->dim)
        {
            ret = ldg_math_stat_model_neuron_desc_default_get(LDG_MATH_STAT_NEURON_RATE, &model->neuron[layer->neuron_off + idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            idx++;
        }
    }

    *out = model->layer_cunt;
    model->layer_cunt++;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_neuron_set(ldg_math_stat_model_t *model, uint32_t layer, uint32_t idx, const ldg_math_stat_neuron_desc_t *desc)
{
    ldg_math_stat_layer_t *slot = 0x0;

    if (LDG_UNLIKELY(!model || !desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(model->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(model->is_built == LDG_TRUTH_TRUE)) { return LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(layer >= model->layer_cunt)) { return LDG_ERR_BOUNDS; }

    slot = &model->layer[layer];

    if (LDG_UNLIKELY(model_layer_neuron_is(slot) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(idx >= slot->desc.dim)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(desc->kind >= LDG_MATH_STAT_NEURON_KIND_CUNT || desc->act >= LDG_MATH_STAT_LAYER_ACT_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    model->neuron[slot->neuron_off + idx] = *desc;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_synapse_add(ldg_math_stat_model_t *model, uint32_t layer, const ldg_math_stat_synapse_desc_t *desc, uint32_t *out)
{
    ldg_math_stat_layer_t *slot = 0x0;

    if (LDG_UNLIKELY(!model || !desc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (LDG_UNLIKELY(model->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(model->is_built == LDG_TRUTH_TRUE)) { return LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(layer >= model->layer_cunt)) { return LDG_ERR_BOUNDS; }

    slot = &model->layer[layer];

    if (LDG_UNLIKELY(model_layer_neuron_is(slot) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(desc->dst >= slot->desc.dim || desc->plastic_is > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(slot->synapse_cunt >= slot->desc.synapse_cap)) { return LDG_ERR_FULL; }

    model->synapse[slot->synapse_off + slot->synapse_cunt] = *desc;

    *out = slot->synapse_cunt;
    slot->synapse_cunt++;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_param_add(ldg_math_stat_model_t *model, uint32_t row_cunt, uint32_t col_cunt, double init_val, uint32_t *out)
{
    ldg_math_stat_model_param_t *param = 0x0;

    if (LDG_UNLIKELY(!model || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (LDG_UNLIKELY(model->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(model->is_built == LDG_TRUTH_TRUE)) { return LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(row_cunt == 0 || col_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(model->param_cunt >= model->conf.param_cap)) { return LDG_ERR_FULL; }

    param = &model->param[model->param_cunt];
    param->init_val = init_val;
    param->row_cunt = row_cunt;
    param->col_cunt = col_cunt;
    param->node = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;

    *out = model->param_cunt;
    model->param_cunt++;

    return LDG_ERR_AOK;
}

static uint32_t model_shape_build(ldg_math_stat_model_t *model, uintptr_t *out_param_elem, uintptr_t *out_elem_per_row, uintptr_t *out_work_per_row, uint32_t *out_node_per_step)
{
    ldg_math_stat_layer_t *layer = 0x0;
    uintptr_t fan_max = 0;
    uint32_t in_dim = model->conf.in_dim;
    uint32_t idx = 0;
    uint32_t ret = 0;

    *out_param_elem = 0;
    *out_elem_per_row = 0;
    *out_work_per_row = 0;
    *out_node_per_step = MODEL_NODE_PER_STEP_BASE;

    model->param_node_cunt = 0;
    model->state_len = 0;

    while (idx < model->layer_cunt)
    {
        layer = &model->layer[idx];
        layer->in_dim = in_dim;

        if (LDG_UNLIKELY(layer->desc.kind == LDG_MATH_STAT_LAYER_ATTN && idx != MODEL_LAYER_FIRST)) { return LDG_ERR_UNSUPPORTED; }

        ret = model_layer_shape_set(layer);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(layer->desc.residual_is == LDG_TRUTH_TRUE && layer->out_dim != layer->in_dim)) { return LDG_ERR_FUNC_ARG_INVALID; }

        layer->param_off = model->param_node_cunt;
        layer->state_off = model->state_len;

        model->param_node_cunt = model->param_node_cunt + layer->param_cunt;
        model->state_len = model->state_len + layer->state_len;

        fan_max = model_layer_fan_max_get(layer);

        *out_param_elem = *out_param_elem + ((uintptr_t)layer->param_cunt * (fan_max + (uintptr_t)LDG_MATH_STAT_LAYER_SCALAR_ROW) * (fan_max + (uintptr_t)LDG_MATH_STAT_LAYER_SCALAR_COL)) + (uintptr_t)layer->synapse_cunt;
        *out_elem_per_row = *out_elem_per_row + layer->elem_per_row + ((uintptr_t)MODEL_OUT_ELEM_PER_LAYER * (uintptr_t)layer->out_dim) + (uintptr_t)layer->state_len;
        *out_node_per_step = *out_node_per_step + layer->node_per_step + MODEL_NODE_PER_LAYER_STEP;

        if (*out_work_per_row < layer->work_per_row) { *out_work_per_row = layer->work_per_row; }

        in_dim = layer->out_dim;
        idx++;
    }

    model->out_dim = in_dim;

    idx = 0;
    while (idx < model->param_cunt)
    {
        *out_param_elem = *out_param_elem + ((uintptr_t)model->param[idx].row_cunt * (uintptr_t)model->param[idx].col_cunt);
        idx++;
    }

    return LDG_ERR_AOK;
}

static uint32_t model_param_build(ldg_math_stat_model_t *model, ldg_math_stat_layer_ctx_t *ctx)
{
    ldg_math_stat_model_param_t *param = 0x0;
    double *ptr = 0x0;
    const double *end = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    while (idx < model->layer_cunt)
    {
        ret = model_layer_build(ctx, &model->layer[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        idx++;
    }

    idx = 0;
    while (idx < model->param_cunt)
    {
        param = &model->param[idx];

        ret = ldg_math_stat_autograd_tensor_push(model->graph, param->row_cunt, param->col_cunt, LDG_TRUTH_TRUE, &param->node);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ptr = model->graph->val + model->graph->node[param->node].val_off;
        end = ptr + ((uintptr_t)param->row_cunt * (uintptr_t)param->col_cunt);
        while (ptr < end)
        {
            *ptr = param->init_val;
            ptr++;
        }

        idx++;
    }

    return LDG_ERR_AOK;
}

static uint32_t model_pending_ck(const ldg_math_stat_model_t *model)
{
    if (LDG_UNLIKELY(model->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(model->is_built == LDG_TRUTH_TRUE)) { return LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(model->layer_cunt == 0)) { return LDG_ERR_EMPTY; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_cap_get(ldg_math_stat_model_t *model, uint32_t *out_node_cap, uintptr_t *out_elem_cap)
{
    uintptr_t param_elem = 0;
    uintptr_t elem_per_row = 0;
    uintptr_t work_per_row = 0;
    uintptr_t rows = 0;
    uint32_t node_per_step = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !out_node_cap || !out_elem_cap)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_node_cap = 0;
    *out_elem_cap = 0;

    ret = model_pending_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = model_shape_build(model, &param_elem, &elem_per_row, &work_per_row, &node_per_step);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    rows = (uintptr_t)model->conf.step_max * (uintptr_t)model->conf.row_max;
    if (model->conf.row_step_max > 0) { rows = model->conf.row_step_max; }

    *out_elem_cap = param_elem + (rows * (elem_per_row + (uintptr_t)model->conf.in_dim)) + model->conf.extra_elem_cap + (uintptr_t)MODEL_ELEM_SLACK;
    *out_node_cap = MODEL_NODE_SLACK + model->conf.extra_node_cap + model->param_node_cunt + model->param_cunt + (model->conf.step_max * node_per_step);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_build(ldg_math_stat_model_t *model)
{
    ldg_math_stat_layer_ctx_t ctx = LDG_STRUCT_ZERO_INIT;
    void *ptr = 0x0;
    uintptr_t param_elem = 0;
    uintptr_t elem_per_row = 0;
    uintptr_t work_per_row = 0;
    uintptr_t rows = 0;
    uintptr_t host_len = 0;
    uint32_t node_per_step = 0;
    uint32_t cuntr_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = model_pending_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(model->graph->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = model_shape_build(model, &param_elem, &elem_per_row, &work_per_row, &node_per_step);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    rows = model->conf.row_max;

    if (model->param_node_cunt > 0)
    {
        ret = model_alloc((uint64_t)model->param_node_cunt * (uint64_t)sizeof(uint32_t), &ptr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        model->param_node = (uint32_t *)ptr;
    }

    if (work_per_row > 0)
    {
        ret = model_alloc((uint64_t)(rows * work_per_row) * (uint64_t)sizeof(double), &ptr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        model->work = (double *)ptr;
    }

    while (idx < model->layer_cunt)
    {
        if (model_layer_cuntr_is(&model->layer[idx]) == LDG_TRUTH_TRUE) { cuntr_cunt++; }

        idx++;
    }

    if (cuntr_cunt > 0)
    {
        ret = model_alloc((uint64_t)cuntr_cunt * (uint64_t)rows * (uint64_t)sizeof(double), &ptr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        model->cuntr = (double *)ptr;

        cuntr_cunt = 0;
        idx = 0;
        while (idx < model->layer_cunt)
        {
            if (model_layer_cuntr_is(&model->layer[idx]) == LDG_TRUTH_TRUE)
            {
                model->layer[idx].cuntr = model->cuntr + ((uintptr_t)cuntr_cunt * rows);
                cuntr_cunt++;
            }

            idx++;
        }
    }

    host_len = 0;
    idx = 0;
    while (idx < model->layer_cunt)
    {
        host_len = host_len + model_layer_host_len_get(&model->layer[idx], (uint32_t)rows);
        idx++;
    }

    if (host_len > 0)
    {
        ret = model_alloc((uint64_t)host_len * (uint64_t)sizeof(double), &ptr);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        model->host = (double *)ptr;
    }

    model->is_built = LDG_TRUTH_TRUE;
    model->param_off = model->graph->elem_cunt;

    model_ctx_get(model, &ctx);

    host_len = 0;
    idx = 0;
    while (idx < model->layer_cunt)
    {
        if (model_layer_host_len_get(&model->layer[idx], (uint32_t)rows) > MODEL_HOST_NONE)
        {
            model->layer[idx].host = model->host + host_len;
            host_len = host_len + model_layer_host_len_get(&model->layer[idx], (uint32_t)rows);

            ret = model_layer_host_init(&ctx, &model->layer[idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        idx++;
    }

    ret = model_param_build(model, &ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    model->param_elem_cunt = model->graph->elem_cunt - model->param_off;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_param_redraw(ldg_math_stat_model_t *model)
{
    ldg_math_stat_layer_ctx_t ctx = LDG_STRUCT_ZERO_INIT;
    uintptr_t host_len = 0;
    uintptr_t elem_before = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    elem_before = model->param_elem_cunt;
    model->param_off = model->graph->elem_cunt;

    model_ctx_get(model, &ctx);

    while (idx < model->layer_cunt)
    {
        model->layer[idx].reward_mean = MODEL_REWARD_MEAN_SEED;
        host_len = model_layer_host_len_get(&model->layer[idx], model->conf.row_max);

        if (host_len > MODEL_HOST_NONE)
        {
            ret = ldg_mem_zero(model->layer[idx].host, (uint64_t)host_len * (uint64_t)sizeof(double));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = model_layer_host_init(&ctx, &model->layer[idx]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        idx++;
    }

    ret = model_param_build(model, &ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    model->param_elem_cunt = model->graph->elem_cunt - model->param_off;

    if (LDG_UNLIKELY(model->param_elem_cunt != elem_before)) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_reward_apply(ldg_math_stat_model_t *model, double reward, uint32_t row_cunt)
{
    ldg_math_stat_layer_ctx_t ctx = LDG_STRUCT_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(row_cunt == 0 || row_cunt > model->conf.row_max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    model_ctx_get(model, &ctx);

    while (idx < model->layer_cunt)
    {
        if (model_layer_neuron_is(&model->layer[idx]) == LDG_TRUTH_TRUE)
        {
            ret = ldg_math_stat_layer_synapse_weight_apply(&ctx, &model->layer[idx], reward, row_cunt);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_forward(ldg_math_stat_model_t *model, const double *src, uint32_t row_cunt, uint32_t *out)
{
    ldg_math_stat_layer_ctx_t ctx = LDG_STRUCT_ZERO_INIT;
    ldg_math_stat_layer_t *layer = 0x0;
    const uint8_t *mask = 0x0;
    uint32_t curr = 0;
    uint32_t raw = 0;
    uint32_t slot = 0;
    uint32_t slot_cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !src || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(row_cunt == 0 || row_cunt > model->conf.row_max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = model_out_mask_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    model_ctx_get(model, &ctx);

    ret = ldg_math_stat_layer_param_leaf_push(&ctx, src, row_cunt, model->conf.in_dim, &model->node_in);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    curr = model->node_in;

    while (idx < model->layer_cunt)
    {
        layer = &model->layer[idx];

        mask = 0x0;
        if (idx + MODEL_LAST_OFF == model->layer_cunt) { mask = model->out_mask; }

        ctx.out_mask = mask;

        ret = model_layer_forward(&ctx, layer, curr, row_cunt, &raw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = model_act_push(model->graph, layer->desc.act, raw, mask, &layer->node_out);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (layer->desc.residual_is == LDG_TRUTH_TRUE)
        {
            ret = ldg_math_stat_autograd_add_push(model->graph, curr, layer->node_out, &raw);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            layer->node_out = raw;
        }

        slot_cunt = model_layer_slot_cunt_get(layer);
        slot = 0;
        while (slot < slot_cunt)
        {
            layer->node_state_in[slot] = layer->node_state_out[slot];
            slot++;
        }

        curr = layer->node_out;
        idx++;
    }

    *out = curr;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_state_push(ldg_math_stat_model_t *model, const double *src, uint32_t row_cunt)
{
    ldg_math_stat_layer_t *layer = 0x0;
    double *dst = 0x0;
    const double *src_row = 0x0;
    uintptr_t state_len = 0;
    uint32_t dim = 0;
    uint32_t slot = 0;
    uint32_t slot_cunt = 0;
    uint32_t row = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(row_cunt == 0 || row_cunt > model->conf.row_max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    state_len = model->state_len;

    while (idx < model->layer_cunt)
    {
        layer = &model->layer[idx];
        slot_cunt = model_layer_slot_cunt_get(layer);
        dim = layer->desc.dim;

        slot = 0;
        while (slot < slot_cunt)
        {
            ret = ldg_math_stat_autograd_tensor_push(model->graph, row_cunt, dim, LDG_TRUTH_FALSE, &layer->node_state_in[slot]);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            dst = model->graph->val + model->graph->node[layer->node_state_in[slot]].val_off;
            src_row = src + layer->state_off + ((uintptr_t)slot * (uintptr_t)dim);

            row = 0;
            while (row < row_cunt)
            {
                ret = ldg_mem_cpy(dst, src_row, (uint64_t)dim * (uint64_t)sizeof(double));
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                dst = dst + dim;
                src_row = src_row + state_len;
                row++;
            }

            layer->node_state_out[slot] = layer->node_state_in[slot];
            slot++;
        }

        if (model_layer_cuntr_is(layer) == LDG_TRUTH_TRUE)
        {
            src_row = src + layer->state_off + ((uintptr_t)slot_cunt * (uintptr_t)dim);

            row = 0;
            while (row < row_cunt)
            {
                layer->cuntr[row] = *src_row;
                src_row = src_row + state_len;
                row++;
            }
        }

        if (model_layer_neuron_is(layer) == LDG_TRUTH_TRUE)
        {
            src_row = src + layer->state_off;
            dst = layer->host + ldg_math_stat_layer_neuron_state_host_off_get(layer);

            row = 0;
            while (row < row_cunt)
            {
                ret = ldg_mem_cpy(dst, src_row, (uint64_t)layer->state_len * (uint64_t)sizeof(double));
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                dst = dst + layer->state_len;
                src_row = src_row + state_len;
                row++;
            }
        }

        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_state_get(const ldg_math_stat_model_t *model, double *dst, uint32_t row_cunt, uintptr_t cap)
{
    const ldg_math_stat_layer_t *layer = 0x0;
    const ldg_math_stat_autograd_node_t *node = 0x0;
    const double *src = 0x0;
    double *dst_row = 0x0;
    uintptr_t state_len = 0;
    uint32_t dim = 0;
    uint32_t slot = 0;
    uint32_t slot_cunt = 0;
    uint32_t row = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(row_cunt == 0 || row_cunt > model->conf.row_max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    state_len = model->state_len;

    if (LDG_UNLIKELY(cap < (uintptr_t)row_cunt * state_len)) { return LDG_ERR_BOUNDS; }

    while (idx < model->layer_cunt)
    {
        layer = &model->layer[idx];
        slot_cunt = model_layer_slot_cunt_get(layer);
        dim = layer->desc.dim;

        slot = 0;
        while (slot < slot_cunt)
        {
            if (LDG_UNLIKELY(layer->node_state_out[slot] >= model->graph->node_cunt)) { return LDG_ERR_NOT_INIT; }

            node = &model->graph->node[layer->node_state_out[slot]];
            if (LDG_UNLIKELY(node->row_cunt != row_cunt || node->col_cunt != dim)) { return LDG_ERR_FUNC_ARG_INVALID; }

            src = model->graph->val + node->val_off;
            dst_row = dst + layer->state_off + ((uintptr_t)slot * (uintptr_t)dim);

            row = 0;
            while (row < row_cunt)
            {
                ret = ldg_mem_cpy(dst_row, src, (uint64_t)dim * (uint64_t)sizeof(double));
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                src = src + dim;
                dst_row = dst_row + state_len;
                row++;
            }

            slot++;
        }

        if (model_layer_cuntr_is(layer) == LDG_TRUTH_TRUE)
        {
            dst_row = dst + layer->state_off + ((uintptr_t)slot_cunt * (uintptr_t)dim);

            row = 0;
            while (row < row_cunt)
            {
                *dst_row = layer->cuntr[row];
                dst_row = dst_row + state_len;
                row++;
            }
        }

        if (model_layer_neuron_is(layer) == LDG_TRUTH_TRUE)
        {
            src = layer->host + ldg_math_stat_layer_neuron_state_host_off_get(layer);
            dst_row = dst + layer->state_off;

            row = 0;
            while (row < row_cunt)
            {
                ret = ldg_mem_cpy(dst_row, src, (uint64_t)layer->state_len * (uint64_t)sizeof(double));
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                src = src + layer->state_len;
                dst_row = dst_row + state_len;
                row++;
            }
        }

        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_synapse_get(const ldg_math_stat_model_t *model, uint32_t layer, uint32_t idx, ldg_math_stat_synapse_desc_t *out)
{
    const ldg_math_stat_layer_t *slot = 0x0;
    uint32_t node = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(layer >= model->layer_cunt)) { return LDG_ERR_BOUNDS; }

    slot = &model->layer[layer];

    if (LDG_UNLIKELY(model_layer_neuron_is(slot) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(idx >= slot->synapse_cunt)) { return LDG_ERR_BOUNDS; }

    *out = model->synapse[slot->synapse_off + idx];
    node = model->param_node[slot->param_off + LDG_MATH_STAT_LAYER_NEURON_WEIGHT];
    out->weight = model->graph->val[model->graph->node[node].val_off + (uintptr_t)idx];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_tensor_get(const ldg_math_stat_model_t *model, uint32_t layer, uint32_t tensor, uintptr_t *out_off, uint32_t *out_row_cunt, uint32_t *out_col_cunt)
{
    const ldg_math_stat_layer_t *slot = 0x0;
    uint32_t node = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !out_off || !out_row_cunt || !out_col_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_off = 0;
    *out_row_cunt = 0;
    *out_col_cunt = 0;

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(layer >= model->layer_cunt)) { return LDG_ERR_BOUNDS; }

    slot = &model->layer[layer];

    if (LDG_UNLIKELY(tensor >= slot->param_cunt)) { return LDG_ERR_BOUNDS; }

    node = model->param_node[slot->param_off + tensor];

    *out_off = model->graph->node[node].val_off;
    *out_row_cunt = model->graph->node[node].row_cunt;
    *out_col_cunt = model->graph->node[node].col_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_layer_cunt_get(const ldg_math_stat_model_t *model, uint32_t *out)
{
    if (LDG_UNLIKELY(!model || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(model->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = model->layer_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_layer_get(const ldg_math_stat_model_t *model, uint32_t layer, const ldg_math_stat_layer_t **out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(layer >= model->layer_cunt)) { return LDG_ERR_BOUNDS; }

    *out = &model->layer[layer];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_param_node_get(const ldg_math_stat_model_t *model, uint32_t param, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(param >= model->param_cunt)) { return LDG_ERR_BOUNDS; }

    *out = model->param[param].node;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_param_cunt_get(const ldg_math_stat_model_t *model, uintptr_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = model->param_elem_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_params_get(const ldg_math_stat_model_t *model, double *dst, uintptr_t cap, uintptr_t *out_cunt)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < model->param_elem_cunt)) { return LDG_ERR_BOUNDS; }

    ret = ldg_mem_cpy(dst, model->graph->val + model->param_off, (uint64_t)model->param_elem_cunt * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_cunt = model->param_elem_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_params_set(ldg_math_stat_model_t *model, const double *src, uintptr_t cunt)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cunt != model->param_elem_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ldg_mem_cpy(model->graph->val + model->param_off, src, (uint64_t)cunt * (uint64_t)sizeof(double));
}

uint32_t ldg_math_stat_model_frozen_set(ldg_math_stat_model_t *model, uint32_t layer, uint8_t frozen_is)
{
    if (LDG_UNLIKELY(!model)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(model->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(layer >= model->layer_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(frozen_is != LDG_TRUTH_TRUE && frozen_is != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    model->layer[layer].desc.frozen_is = frozen_is;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_frozen_mask_set(const ldg_math_stat_model_t *model, uint8_t *mask, uintptr_t cap, uint32_t *out_cunt)
{
    const ldg_math_stat_layer_t *layer = 0x0;
    const ldg_math_stat_autograd_node_t *node = 0x0;
    uintptr_t off = 0;
    uintptr_t end = 0;
    uint32_t tensor = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !mask || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < model->param_off + model->param_elem_cunt)) { return LDG_ERR_BOUNDS; }

    while (idx < model->layer_cunt)
    {
        layer = &model->layer[idx];
        tensor = 0;
        while (tensor < layer->param_cunt)
        {
            node = &model->graph->node[model->param_node[layer->param_off + tensor]];
            off = node->val_off;
            end = off + ((uintptr_t)node->row_cunt * (uintptr_t)node->col_cunt);
            while (off < end)
            {
                mask[off] = layer->desc.frozen_is;
                off++;
            }

            tensor++;
        }

        if (layer->desc.frozen_is == LDG_TRUTH_TRUE) { *out_cunt = *out_cunt + layer->param_cunt; }

        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_sequence_loss_push(ldg_math_stat_model_t *model, uint32_t logp_node, const double *tgt, const double *mask, uint32_t row_cunt, uint32_t sequence_len, uint32_t vocab_cunt, uint32_t *out)
{
    ldg_math_stat_layer_ctx_t ctx = LDG_STRUCT_ZERO_INIT;
    uintptr_t tok_cunt = (uintptr_t)row_cunt * (uintptr_t)sequence_len;
    uintptr_t idx = 0;
    double mask_sum = MODEL_MASK_SUM_SEED;
    uint32_t tok = 0;
    uint32_t tgt_node = 0;
    uint32_t picked = 0;
    uint32_t mask_node = 0;
    uint32_t kept = 0;
    uint32_t mean = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !tgt || !mask || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(row_cunt == 0 || sequence_len == 0 || vocab_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (idx < tok_cunt)
    {
        mask_sum = mask_sum + mask[idx];
        idx++;
    }

    if (LDG_UNLIKELY(mask_sum <= MODEL_MASK_SUM_SEED)) { return LDG_ERR_EMPTY; }

    model_ctx_get(model, &ctx);

    ret = ldg_math_stat_autograd_reshape_push(model->graph, logp_node, row_cunt * sequence_len, vocab_cunt, &tok);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_leaf_push(&ctx, tgt, row_cunt * sequence_len, LDG_MATH_STAT_LAYER_SCALAR_COL, &tgt_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_gather_push(model->graph, tok, tgt_node, &picked);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_leaf_push(&ctx, mask, row_cunt * sequence_len, LDG_MATH_STAT_LAYER_SCALAR_COL, &mask_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_col_mul_push(model->graph, picked, mask_node, &kept);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mean_push(model->graph, kept, &mean);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_scale_push(model->graph, mean, -(double)tok_cunt / mask_sum, out);
}

uint32_t ldg_math_stat_model_out_dim_get(const ldg_math_stat_model_t *model, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = model->out_dim;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_state_len_get(const ldg_math_stat_model_t *model, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = model_built_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = model->state_len;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_linreg_observe(ldg_math_stat_model_t *model, uint32_t layer, const double *src, const double *tgt, uint32_t row_cunt)
{
    ldg_math_stat_layer_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !src || !tgt)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = model_layer_kind_ck(model, layer, LDG_MATH_STAT_LAYER_LINREG, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ldg_math_stat_layer_linreg_observe(slot, src, tgt, row_cunt);
}

uint32_t ldg_math_stat_model_linreg_refit(ldg_math_stat_model_t *model, uint32_t layer, double ridge)
{
    ldg_math_stat_layer_ctx_t ctx = LDG_STRUCT_ZERO_INIT;
    ldg_math_stat_layer_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = model_layer_kind_ck(model, layer, LDG_MATH_STAT_LAYER_LINREG, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    model_ctx_get(model, &ctx);

    return ldg_math_stat_layer_linreg_refit(&ctx, slot, ridge);
}

uint32_t ldg_math_stat_model_linreg_rst(ldg_math_stat_model_t *model, uint32_t layer)
{
    ldg_math_stat_layer_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = model_layer_kind_ck(model, layer, LDG_MATH_STAT_LAYER_LINREG, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_layer_linreg_rst(slot);
}

uint32_t ldg_math_stat_model_svm_fit(ldg_math_stat_model_t *model, uint32_t layer, const double *src, const uint32_t *label, uint32_t row_cunt, double cost, uint32_t pass_cunt)
{
    ldg_math_stat_layer_ctx_t ctx = LDG_STRUCT_ZERO_INIT;
    ldg_math_stat_layer_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !src || !label)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = model_layer_kind_ck(model, layer, LDG_MATH_STAT_LAYER_SVM, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    model_ctx_get(model, &ctx);

    return ldg_math_stat_layer_svm_fit(&ctx, slot, src, label, row_cunt, cost, pass_cunt);
}

uint32_t ldg_math_stat_model_train_set(ldg_math_stat_model_t *model, uint8_t train_is)
{
    if (LDG_UNLIKELY(!model)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(model->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(train_is > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    model->train_is = train_is;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_live_set(ldg_math_stat_model_t *model, uint8_t live_is)
{
    if (LDG_UNLIKELY(!model)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(model->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(live_is > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    model->live_is = live_is;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_out_mask_set(ldg_math_stat_model_t *model, const uint8_t *mask)
{
    if (LDG_UNLIKELY(!model)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(model->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    model->out_mask = mask;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_model_svm_rst(ldg_math_stat_model_t *model, uint32_t layer)
{
    ldg_math_stat_layer_t *slot = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = model_layer_kind_ck(model, layer, LDG_MATH_STAT_LAYER_SVM, &slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_layer_svm_rst(slot);
}
