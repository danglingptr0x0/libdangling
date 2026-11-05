#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>
#include <dangling/mem/cpy.h>

#include "layer.h"

#define LAYER_ATTN_W_SELF 0
#define LAYER_ATTN_B_SELF 1
#define LAYER_ATTN_W_SELF_REC 2
#define LAYER_ATTN_W_REC 3
#define LAYER_ATTN_B_REC 4
#define LAYER_ATTN_W_Q 5
#define LAYER_ATTN_W_K 6
#define LAYER_ATTN_W_V 7
#define LAYER_ATTN_W_O 8
#define LAYER_ATTN_B_O 9
#define LAYER_ATTN_TEMP 10
#define LAYER_ATTN_PARAM_CUNT 10
#define LAYER_ATTN_PARAM_QK_NORM_CUNT 11
#define LAYER_ATTN_OUT_PART_CUNT 2
#define LAYER_ATTN_NODE_PER_STEP 36
#define LAYER_ATTN_SELF_ELEM_PER_ROW 7
#define LAYER_ATTN_EMB_ELEM_PER_ROW 14
#define LAYER_ATTN_SCORE_ELEM_PER_ROW 5
#define LAYER_ATTN_MASK_LEN 1
#define LAYER_ATTN_QK_NORM_EPS 0.00001
#define LAYER_ATTN_TEMP_INIT 1.0

static uint32_t layer_attn_entity_len_get(const ldg_math_stat_layer_t *layer)
{
    return layer->desc.entity_cap * (layer->desc.entity_dim + LAYER_ATTN_MASK_LEN);
}

static uint32_t layer_attn_self_dim_get(const ldg_math_stat_layer_t *layer)
{
    return layer->in_dim - layer_attn_entity_len_get(layer);
}

uint32_t ldg_math_stat_layer_attn_shape_set(ldg_math_stat_layer_t *layer)
{
    uintptr_t dim = layer->desc.dim;
    uintptr_t cap = layer->desc.entity_cap;
    uintptr_t hd_cunt = layer->desc.hd_cunt;

    if (LDG_UNLIKELY(layer->desc.dim == 0 || layer->desc.entity_cap == 0 || layer->desc.entity_dim == 0 || layer->desc.hd_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(layer->desc.dim % layer->desc.hd_cunt != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(layer->in_dim <= layer_attn_entity_len_get(layer))) { return LDG_ERR_FUNC_ARG_INVALID; }

    layer->out_dim = LAYER_ATTN_OUT_PART_CUNT * layer->desc.dim;
    layer->param_cunt = LAYER_ATTN_PARAM_CUNT;
    if (layer->desc.qk_norm_is == LDG_TRUTH_TRUE) { layer->param_cunt = LAYER_ATTN_PARAM_QK_NORM_CUNT; }

    layer->state_len = 0;
    layer->node_per_step = LAYER_ATTN_NODE_PER_STEP;
    layer->elem_per_row = (uintptr_t)layer->in_dim + ((uintptr_t)LAYER_ATTN_SELF_ELEM_PER_ROW * dim) + ((uintptr_t)LAYER_ATTN_EMB_ELEM_PER_ROW * cap * dim) + ((uintptr_t)LAYER_ATTN_SCORE_ELEM_PER_ROW * hd_cunt * cap * cap);
    layer->work_per_row = layer->in_dim;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_attn_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t self_dim = layer_attn_self_dim_get(layer);
    uint32_t dim = layer->desc.dim;
    uint32_t ret = 0;

    ret = ldg_math_stat_layer_param_weight_push(ctx, self_dim, dim, &param[LAYER_ATTN_W_SELF]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_vec_push(ctx, dim, LDG_MATH_STAT_LAYER_BIAS_INIT, &param[LAYER_ATTN_B_SELF]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_weight_push(ctx, self_dim, dim, &param[LAYER_ATTN_W_SELF_REC]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_weight_push(ctx, layer->desc.entity_dim, dim, &param[LAYER_ATTN_W_REC]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_vec_push(ctx, dim, LDG_MATH_STAT_LAYER_BIAS_INIT, &param[LAYER_ATTN_B_REC]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_weight_push(ctx, dim, dim, &param[LAYER_ATTN_W_Q]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_weight_push(ctx, dim, dim, &param[LAYER_ATTN_W_K]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_weight_push(ctx, dim, dim, &param[LAYER_ATTN_W_V]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_weight_push(ctx, dim, dim, &param[LAYER_ATTN_W_O]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_vec_push(ctx, dim, LDG_MATH_STAT_LAYER_BIAS_INIT, &param[LAYER_ATTN_B_O]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (layer->desc.qk_norm_is != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return ldg_math_stat_layer_param_vec_push(ctx, LDG_MATH_STAT_LAYER_SCALAR_COL, LAYER_ATTN_TEMP_INIT, &param[LAYER_ATTN_TEMP]);
}

static uint32_t layer_attn_stage(ldg_math_stat_layer_ctx_t *ctx, const ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out_self, uint32_t *out_rec, uint32_t *out_mask)
{
    const double *src_row = ctx->graph->val + ctx->graph->node[in_node].val_off;
    uintptr_t cap = layer->desc.entity_cap;
    uintptr_t entity_dim = layer->desc.entity_dim;
    uintptr_t self_dim = layer_attn_self_dim_get(layer);
    uintptr_t rows = row_cunt;
    double *self_dst = ctx->work;
    double *rec_dst = self_dst + (rows * self_dim);
    double *mask_dst = rec_dst + (rows * cap * entity_dim);
    uint32_t row = 0;
    uint32_t ret = 0;

    while (row < row_cunt)
    {
        ret = ldg_mem_cpy(self_dst + ((uintptr_t)row * self_dim), src_row, (uint64_t)self_dim * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_mem_cpy(rec_dst + ((uintptr_t)row * cap * entity_dim), src_row + self_dim, (uint64_t)cap * (uint64_t)entity_dim * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_mem_cpy(mask_dst + ((uintptr_t)row * cap), src_row + self_dim + (cap * entity_dim), (uint64_t)cap * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        src_row = src_row + layer->in_dim;
        row++;
    }

    ret = ldg_math_stat_layer_param_leaf_push(ctx, self_dst, row_cunt, (uint32_t)self_dim, out_self);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_layer_param_leaf_push(ctx, rec_dst, row_cunt * layer->desc.entity_cap, layer->desc.entity_dim, out_rec);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_layer_param_leaf_push(ctx, mask_dst, row_cunt * layer->desc.entity_cap, LDG_MATH_STAT_LAYER_SCALAR_COL, out_mask);
}

static uint32_t layer_attn_emb_push(ldg_math_stat_layer_ctx_t *ctx, const ldg_math_stat_layer_t *layer, uint32_t self_node, uint32_t rec_node, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t self_rec = 0;
    uint32_t rep = 0;
    uint32_t rec_pre = 0;
    uint32_t rec_sum = 0;
    uint32_t rec_bias = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, self_node, param[LAYER_ATTN_W_SELF_REC], &self_rec);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_row_repeat_push(ctx->graph, self_rec, layer->desc.entity_cap, &rep);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, rec_node, param[LAYER_ATTN_W_REC], &rec_pre);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_add_push(ctx->graph, rec_pre, rep, &rec_sum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_bias_add_push(ctx->graph, rec_sum, param[LAYER_ATTN_B_REC], &rec_bias);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_tanh_push(ctx->graph, rec_bias, out);
}

static uint32_t layer_attn_score_push(ldg_math_stat_layer_ctx_t *ctx, const ldg_math_stat_layer_t *layer, uint32_t emb, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t query = 0;
    uint32_t key = 0;
    uint32_t query_norm = 0;
    uint32_t key_norm = 0;
    uint32_t score = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, emb, param[LAYER_ATTN_W_Q], &query);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, emb, param[LAYER_ATTN_W_K], &key);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (layer->desc.qk_norm_is != LDG_TRUTH_TRUE) { return ldg_math_stat_autograd_attn_score_push(ctx->graph, query, key, layer->desc.entity_cap, layer->desc.hd_cunt, out); }

    ret = ldg_math_stat_autograd_row_norm_push(ctx->graph, query, LAYER_ATTN_QK_NORM_EPS, &query_norm);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_row_norm_push(ctx->graph, key, LAYER_ATTN_QK_NORM_EPS, &key_norm);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_attn_score_push(ctx->graph, query_norm, key_norm, layer->desc.entity_cap, layer->desc.hd_cunt, &score);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_scalar_mul_push(ctx->graph, score, param[LAYER_ATTN_TEMP], out);
}

uint32_t ldg_math_stat_layer_attn_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t self_node = 0;
    uint32_t rec_node = 0;
    uint32_t mask_node = 0;
    uint32_t self_pre = 0;
    uint32_t self_bias = 0;
    uint32_t self_emb = 0;
    uint32_t emb = 0;
    uint32_t val = 0;
    uint32_t score = 0;
    uint32_t masked = 0;
    uint32_t logp = 0;
    uint32_t prob = 0;
    uint32_t attended = 0;
    uint32_t proj = 0;
    uint32_t proj_bias = 0;
    uint32_t resid = 0;
    uint32_t pool = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = layer_attn_stage(ctx, layer, in_node, row_cunt, &self_node, &rec_node, &mask_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, self_node, param[LAYER_ATTN_W_SELF], &self_pre);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_bias_add_push(ctx->graph, self_pre, param[LAYER_ATTN_B_SELF], &self_bias);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_tanh_push(ctx->graph, self_bias, &self_emb);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = layer_attn_emb_push(ctx, layer, self_node, rec_node, &emb);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, emb, param[LAYER_ATTN_W_V], &val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = layer_attn_score_push(ctx, layer, emb, &score);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_attn_mask_push(ctx->graph, score, mask_node, layer->desc.entity_cap, layer->desc.hd_cunt, &masked);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_log_softmax_push(ctx->graph, masked, 0x0, &logp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_exp_push(ctx->graph, logp, &prob);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    layer->node_aux = prob;

    ret = ldg_math_stat_autograd_attn_apply_push(ctx->graph, prob, val, layer->desc.entity_cap, layer->desc.hd_cunt, &attended);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, attended, param[LAYER_ATTN_W_O], &proj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_bias_add_push(ctx->graph, proj, param[LAYER_ATTN_B_O], &proj_bias);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_add_push(ctx->graph, emb, proj_bias, &resid);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_row_pool_push(ctx->graph, resid, mask_node, layer->desc.entity_cap, &pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_col_concat_push(ctx->graph, self_emb, pool, out);
}
