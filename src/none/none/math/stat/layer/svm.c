#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>

#include "../rl.h"
#include "layer.h"

#define LAYER_SVM_PARAM_CUNT 0
#define LAYER_SVM_NODE_PER_STEP 1
#define LAYER_SVM_HOST_NONE 0
#define LAYER_SVM_AUG_CUNT 1
#define LAYER_SVM_IDX_LAST 1
#define LAYER_SVM_CLASS_FIRST 0
#define LAYER_SVM_OUT_MEAN 0
#define LAYER_SVM_OUT_SPREAD 1
#define LAYER_SVM_WEIGHT_INIT 0.0
#define LAYER_SVM_CUM_INIT 0.0
#define LAYER_SVM_LOSS_NONE 0.0
#define LAYER_SVM_CONST_LANE 1.0
#define LAYER_SVM_MARGIN_TGT 1.0
#define LAYER_SVM_LABEL_POS 1.0
#define LAYER_SVM_LABEL_NEG (-1.0)

static uintptr_t layer_svm_aug_get(const ldg_math_stat_layer_t *layer)
{
    return (uintptr_t)layer->in_dim + (uintptr_t)LAYER_SVM_AUG_CUNT;
}

static uintptr_t layer_svm_weight_len_get(const ldg_math_stat_layer_t *layer)
{
    return (uintptr_t)layer->desc.member_cunt * (uintptr_t)layer->desc.dim * layer_svm_aug_get(layer);
}

static double* layer_svm_weight_get(const ldg_math_stat_layer_t *layer, uint32_t member, uint32_t class_idx)
{
    return layer->host + ((((uintptr_t)member * (uintptr_t)layer->desc.dim) + (uintptr_t)class_idx) * layer_svm_aug_get(layer));
}

static uint32_t* layer_svm_idx_get(const ldg_math_stat_layer_t *layer)
{
    return (uint32_t *)(layer->host + layer_svm_weight_len_get(layer));
}

static double* layer_svm_out_get(const ldg_math_stat_layer_t *layer)
{
    return layer->host + layer_svm_weight_len_get(layer) + (uintptr_t)layer->desc.sample_cap;
}

uintptr_t ldg_math_stat_layer_svm_host_len_get(const ldg_math_stat_layer_t *layer, uint32_t row_max)
{
    if (layer->desc.kind != LDG_MATH_STAT_LAYER_SVM) { return LAYER_SVM_HOST_NONE; }

    return layer_svm_weight_len_get(layer) + (uintptr_t)layer->desc.sample_cap + ((uintptr_t)row_max * (uintptr_t)layer->out_dim);
}

uint32_t ldg_math_stat_layer_svm_shape_set(ldg_math_stat_layer_t *layer)
{
    if (LDG_UNLIKELY(layer->desc.dim == 0 || layer->in_dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(layer->desc.member_cunt == 0 || layer->desc.sample_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    layer->out_dim = LDG_MATH_STAT_SVM_OUT_PER_CLASS * layer->desc.dim;
    layer->param_cunt = LAYER_SVM_PARAM_CUNT;
    layer->state_len = 0;
    layer->node_per_step = LAYER_SVM_NODE_PER_STEP;
    layer->elem_per_row = layer->out_dim;
    layer->work_per_row = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_svm_rst(ldg_math_stat_layer_t *layer)
{
    double *ptr = layer->host;
    const double *end = layer->host + layer_svm_weight_len_get(layer);

    while (ptr < end)
    {
        *ptr = LAYER_SVM_WEIGHT_INIT;
        ptr++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_svm_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    if (LDG_UNLIKELY(!ctx->graph || !layer->host)) { return LDG_ERR_NOT_INIT; }

    return ldg_math_stat_layer_svm_rst(layer);
}

static double layer_svm_margin_get(const ldg_math_stat_layer_t *layer, uint32_t member, uint32_t class_idx, const double *src)
{
    const double *weight = layer_svm_weight_get(layer, member, class_idx);
    double cum = LAYER_SVM_CUM_INIT;
    uint32_t lane = 0;

    while (lane < layer->in_dim)
    {
        cum = cum + (weight[lane] * src[lane]);
        lane++;
    }

    return cum + weight[layer->in_dim];
}

static void layer_svm_class_stat_get(const ldg_math_stat_layer_t *layer, uint32_t class_idx, const double *src, double *out_mean, double *out_spread)
{
    double mean = LAYER_SVM_CUM_INIT;
    double var = LAYER_SVM_CUM_INIT;
    double diff = 0.0;
    uint32_t member = 0;

    while (member < layer->desc.member_cunt)
    {
        mean = mean + layer_svm_margin_get(layer, member, class_idx, src);
        member++;
    }

    mean = mean / (double)layer->desc.member_cunt;

    member = 0;
    while (member < layer->desc.member_cunt)
    {
        diff = layer_svm_margin_get(layer, member, class_idx, src) - mean;
        var = var + (diff * diff);
        member++;
    }

    *out_mean = mean;
    *out_spread = ldg_math_sqrt_get(var / (double)layer->desc.member_cunt);
}

uint32_t ldg_math_stat_layer_svm_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const double *src = 0x0;
    double *dst = layer_svm_out_get(layer);
    uint32_t row = 0;
    uint32_t class_idx = 0;

    if (LDG_UNLIKELY(row_cunt == 0 || row_cunt > ctx->row_max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(in_node >= ctx->graph->node_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(ctx->graph->node[in_node].col_cunt != layer->in_dim)) { return LDG_ERR_FUNC_ARG_INVALID; }

    src = ctx->graph->val + ctx->graph->node[in_node].val_off;

    while (row < row_cunt)
    {
        class_idx = 0;
        while (class_idx < layer->desc.dim)
        {
            layer_svm_class_stat_get(layer, class_idx, src + ((uintptr_t)row * (uintptr_t)layer->in_dim), &dst[((uintptr_t)row * (uintptr_t)layer->out_dim) + ((uintptr_t)class_idx * LDG_MATH_STAT_SVM_OUT_PER_CLASS) + LAYER_SVM_OUT_MEAN], &dst[((uintptr_t)row * (uintptr_t)layer->out_dim) + ((uintptr_t)class_idx * LDG_MATH_STAT_SVM_OUT_PER_CLASS) + LAYER_SVM_OUT_SPREAD]);
            class_idx++;
        }

        row++;
    }

    return ldg_math_stat_layer_param_leaf_push(ctx, dst, row_cunt, layer->out_dim, out);
}

static uint32_t layer_svm_sample_draw(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t row_cunt)
{
    uint32_t *idx = layer_svm_idx_get(layer);
    double unit = 0.0;
    uint32_t pick = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;

    while (slot < row_cunt)
    {
        ret = ldg_math_stat_rng_uniform_sample(ctx->rng_cb, ctx->rng_ctx, &unit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        pick = (uint32_t)(unit * (double)row_cunt);
        if (pick >= row_cunt) { pick = row_cunt - LAYER_SVM_IDX_LAST; }

        idx[slot] = pick;
        slot++;
    }

    return LDG_ERR_AOK;
}

static void layer_svm_member_zero(ldg_math_stat_layer_t *layer, uint32_t member)
{
    double *ptr = layer_svm_weight_get(layer, member, LAYER_SVM_CLASS_FIRST);
    const double *end = ptr + ((uintptr_t)layer->desc.dim * layer_svm_aug_get(layer));

    while (ptr < end)
    {
        *ptr = LAYER_SVM_WEIGHT_INIT;
        ptr++;
    }
}

static void layer_svm_row_update(ldg_math_stat_layer_t *layer, uint32_t member, const double *src, uint32_t label, double cost)
{
    double *weight = 0x0;
    double norm_sq = LAYER_SVM_CONST_LANE;
    double sign = 0.0;
    double loss = 0.0;
    double tau = 0.0;
    uint32_t lane = 0;
    uint32_t class_idx = 0;

    while (lane < layer->in_dim)
    {
        norm_sq = norm_sq + (src[lane] * src[lane]);
        lane++;
    }

    while (class_idx < layer->desc.dim)
    {
        sign = LAYER_SVM_LABEL_NEG;
        if (class_idx == label) { sign = LAYER_SVM_LABEL_POS; }

        loss = LAYER_SVM_MARGIN_TGT - (sign * layer_svm_margin_get(layer, member, class_idx, src));
        if (loss > LAYER_SVM_LOSS_NONE)
        {
            tau = loss / norm_sq;
            if (tau > cost) { tau = cost; }

            weight = layer_svm_weight_get(layer, member, class_idx);

            lane = 0;
            while (lane < layer->in_dim)
            {
                weight[lane] = weight[lane] + (tau * sign * src[lane]);
                lane++;
            }

            weight[layer->in_dim] = weight[layer->in_dim] + (tau * sign);
        }

        class_idx++;
    }
}

uint32_t ldg_math_stat_layer_svm_fit(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, const double *src, const uint32_t *label, uint32_t row_cunt, double cost, uint32_t pass_cunt)
{
    const uint32_t *idx = layer_svm_idx_get(layer);
    uint32_t member = 0;
    uint32_t pass = 0;
    uint32_t slot = 0;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0 || row_cunt > layer->desc.sample_cap || pass_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(cost <= LAYER_SVM_LOSS_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (row < row_cunt)
    {
        if (LDG_UNLIKELY(label[row] >= layer->desc.dim)) { return LDG_ERR_BOUNDS; }

        row++;
    }

    while (member < layer->desc.member_cunt)
    {
        ret = layer_svm_sample_draw(ctx, layer, row_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        layer_svm_member_zero(layer, member);

        pass = 0;
        while (pass < pass_cunt)
        {
            slot = 0;
            while (slot < row_cunt)
            {
                row = idx[slot];
                layer_svm_row_update(layer, member, src + ((uintptr_t)row * (uintptr_t)layer->in_dim), label[row], cost);
                slot++;
            }

            pass++;
        }

        member++;
    }

    return LDG_ERR_AOK;
}
