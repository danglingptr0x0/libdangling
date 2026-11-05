#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>

#include "layer.h"

#define LAYER_LINREG_WEIGHT 0
#define LAYER_LINREG_BIAS 1
#define LAYER_LINREG_PARAM_CUNT 2
#define LAYER_LINREG_NODE_PER_STEP 4
#define LAYER_LINREG_ELEM_PER_ROW 4
#define LAYER_LINREG_HOST_NONE 0
#define LAYER_LINREG_AUG_CUNT 1
#define LAYER_LINREG_BUFF_CUNT 2
#define LAYER_LINREG_PIVOT_FLOOR 0.0
#define LAYER_LINREG_UNIT 1.0

static uintptr_t layer_linreg_aug_get(const ldg_math_stat_layer_t *layer)
{
    return (uintptr_t)layer->in_dim + (uintptr_t)LAYER_LINREG_AUG_CUNT;
}

static double* layer_linreg_cross_get(const ldg_math_stat_layer_t *layer)
{
    return layer->host + (layer_linreg_aug_get(layer) * layer_linreg_aug_get(layer));
}

static double* layer_linreg_factor_get(const ldg_math_stat_layer_t *layer)
{
    return layer_linreg_cross_get(layer) + (layer_linreg_aug_get(layer) * (uintptr_t)layer->desc.dim);
}

static double* layer_linreg_solve_get(const ldg_math_stat_layer_t *layer)
{
    return layer_linreg_factor_get(layer) + (layer_linreg_aug_get(layer) * layer_linreg_aug_get(layer));
}

uintptr_t ldg_math_stat_layer_linreg_host_len_get(const ldg_math_stat_layer_t *layer)
{
    uintptr_t aug = 0;

    if (layer->desc.kind != LDG_MATH_STAT_LAYER_LINREG) { return LAYER_LINREG_HOST_NONE; }

    aug = layer_linreg_aug_get(layer);

    return ((uintptr_t)LAYER_LINREG_BUFF_CUNT * aug * aug) + ((uintptr_t)LAYER_LINREG_BUFF_CUNT * aug * (uintptr_t)layer->desc.dim);
}

uint32_t ldg_math_stat_layer_linreg_shape_set(ldg_math_stat_layer_t *layer)
{
    if (LDG_UNLIKELY(layer->desc.dim == 0 || layer->in_dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    layer->out_dim = layer->desc.dim;
    layer->param_cunt = LAYER_LINREG_PARAM_CUNT;
    layer->state_len = 0;
    layer->node_per_step = LAYER_LINREG_NODE_PER_STEP;
    layer->elem_per_row = (uintptr_t)LAYER_LINREG_ELEM_PER_ROW * (uintptr_t)layer->desc.dim;
    layer->work_per_row = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_linreg_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer)
{
    uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t ret = 0;

    ret = ldg_math_stat_layer_param_weight_push(ctx, layer->in_dim, layer->desc.dim, &param[LAYER_LINREG_WEIGHT]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_layer_param_vec_push(ctx, layer->desc.dim, LDG_MATH_STAT_LAYER_BIAS_INIT, &param[LAYER_LINREG_BIAS]);
}

uint32_t ldg_math_stat_layer_linreg_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    uint32_t pre = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(row_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_stat_autograd_matmul_push(ctx->graph, in_node, param[LAYER_LINREG_WEIGHT], &pre);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_bias_add_push(ctx->graph, pre, param[LAYER_LINREG_BIAS], out);
}

uint32_t ldg_math_stat_layer_linreg_rst(ldg_math_stat_layer_t *layer)
{
    double *ptr = layer->host;
    const double *end = layer->host + ldg_math_stat_layer_linreg_host_len_get(layer);

    while (ptr < end)
    {
        *ptr = LAYER_LINREG_PIVOT_FLOOR;
        ptr++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_layer_linreg_observe(ldg_math_stat_layer_t *layer, const double *src, const double *tgt, uint32_t row_cunt)
{
    double *gram = layer->host;
    double *cross = layer_linreg_cross_get(layer);
    uintptr_t aug = layer_linreg_aug_get(layer);
    uintptr_t in_dim = layer->in_dim;
    double lhs_val = 0.0;
    uint32_t row = 0;
    uint32_t lhs = 0;
    uint32_t rhs = 0;

    while (row < row_cunt)
    {
        lhs = 0;
        while (lhs < aug)
        {
            lhs_val = LAYER_LINREG_UNIT;
            if (lhs < in_dim) { lhs_val = src[((uintptr_t)row * in_dim) + (uintptr_t)lhs]; }

            rhs = 0;
            while (rhs < aug)
            {
                if (rhs < in_dim) { gram[((uintptr_t)lhs * aug) + (uintptr_t)rhs] = gram[((uintptr_t)lhs * aug) + (uintptr_t)rhs] + (lhs_val * src[((uintptr_t)row * in_dim) + (uintptr_t)rhs]); }
                else { gram[((uintptr_t)lhs * aug) + (uintptr_t)rhs] = gram[((uintptr_t)lhs * aug) + (uintptr_t)rhs] + lhs_val; }

                rhs++;
            }

            rhs = 0;
            while (rhs < layer->desc.dim)
            {
                cross[((uintptr_t)lhs * (uintptr_t)layer->desc.dim) + (uintptr_t)rhs] = cross[((uintptr_t)lhs * (uintptr_t)layer->desc.dim) + (uintptr_t)rhs] + (lhs_val * tgt[((uintptr_t)row * (uintptr_t)layer->desc.dim) + (uintptr_t)rhs]);
                rhs++;
            }

            lhs++;
        }

        row++;
    }

    return LDG_ERR_AOK;
}

static uint32_t layer_linreg_factor_compute(ldg_math_stat_layer_t *layer, double ridge)
{
    const double *gram = layer->host;
    double *factor = layer_linreg_factor_get(layer);
    uintptr_t aug = layer_linreg_aug_get(layer);
    double cum = 0.0;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t idx = 0;

    while (row < aug)
    {
        col = 0;
        while (col <= row)
        {
            cum = gram[((uintptr_t)row * aug) + (uintptr_t)col];
            if (row == col) { cum = cum + ridge; }

            idx = 0;
            while (idx < col)
            {
                cum = cum - (factor[((uintptr_t)row * aug) + (uintptr_t)idx] * factor[((uintptr_t)col * aug) + (uintptr_t)idx]);
                idx++;
            }

            if (row == col)
            {
                if (LDG_UNLIKELY(cum <= LAYER_LINREG_PIVOT_FLOOR)) { return LDG_ERR_INVALID; }

                factor[((uintptr_t)row * aug) + (uintptr_t)col] = ldg_math_sqrt_get(cum);
            }
            else { factor[((uintptr_t)row * aug) + (uintptr_t)col] = cum / factor[((uintptr_t)col * aug) + (uintptr_t)col]; }

            col++;
        }

        col = row + LAYER_LINREG_AUG_CUNT;
        while (col < aug)
        {
            factor[((uintptr_t)row * aug) + (uintptr_t)col] = LAYER_LINREG_PIVOT_FLOOR;
            col++;
        }

        row++;
    }

    return LDG_ERR_AOK;
}

static void layer_linreg_solve_compute(ldg_math_stat_layer_t *layer)
{
    const double *factor = layer_linreg_factor_get(layer);
    const double *cross = layer_linreg_cross_get(layer);
    double *solve = layer_linreg_solve_get(layer);
    uintptr_t aug = layer_linreg_aug_get(layer);
    uintptr_t width = layer->desc.dim;
    double cum = 0.0;
    uint32_t col = 0;
    uint32_t row = 0;
    uint32_t idx = 0;

    while (col < width)
    {
        row = 0;
        while (row < aug)
        {
            cum = cross[((uintptr_t)row * width) + (uintptr_t)col];

            idx = 0;
            while (idx < row)
            {
                cum = cum - (factor[((uintptr_t)row * aug) + (uintptr_t)idx] * solve[((uintptr_t)idx * width) + (uintptr_t)col]);
                idx++;
            }

            solve[((uintptr_t)row * width) + (uintptr_t)col] = cum / factor[((uintptr_t)row * aug) + (uintptr_t)row];
            row++;
        }

        row = (uint32_t)aug;
        while (row > 0)
        {
            row--;
            cum = solve[((uintptr_t)row * width) + (uintptr_t)col];

            idx = row + LAYER_LINREG_AUG_CUNT;
            while (idx < aug)
            {
                cum = cum - (factor[((uintptr_t)idx * aug) + (uintptr_t)row] * solve[((uintptr_t)idx * width) + (uintptr_t)col]);
                idx++;
            }

            solve[((uintptr_t)row * width) + (uintptr_t)col] = cum / factor[((uintptr_t)row * aug) + (uintptr_t)row];
        }

        col++;
    }
}

uint32_t ldg_math_stat_layer_linreg_refit(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, double ridge)
{
    const uint32_t *param = ctx->param_node + layer->param_off;
    const double *solve = 0x0;
    double *weight = 0x0;
    double *bias = 0x0;
    uintptr_t width = layer->desc.dim;
    uint32_t row = 0;
    uint32_t col = 0;
    uint32_t ret = 0;

    ret = layer_linreg_factor_compute(layer, ridge);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    layer_linreg_solve_compute(layer);

    solve = layer_linreg_solve_get(layer);
    weight = ctx->graph->val + ctx->graph->node[param[LAYER_LINREG_WEIGHT]].val_off;
    bias = ctx->graph->val + ctx->graph->node[param[LAYER_LINREG_BIAS]].val_off;

    while (row < layer->in_dim)
    {
        col = 0;
        while (col < width)
        {
            weight[((uintptr_t)row * width) + (uintptr_t)col] = solve[((uintptr_t)row * width) + (uintptr_t)col];
            col++;
        }

        row++;
    }

    col = 0;
    while (col < width)
    {
        bias[col] = solve[((uintptr_t)layer->in_dim * width) + (uintptr_t)col];
        col++;
    }

    return LDG_ERR_AOK;
}
