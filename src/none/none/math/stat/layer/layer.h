#ifndef LDG_MATH_STAT_LAYER_H
#define LDG_MATH_STAT_LAYER_H

#include <dangling/math/stat.h>

#define LDG_MATH_STAT_LAYER_SCALAR_ROW 1
#define LDG_MATH_STAT_LAYER_SCALAR_COL 1
#define LDG_MATH_STAT_LAYER_STATE_H 0
#define LDG_MATH_STAT_LAYER_STATE_C 1
#define LDG_MATH_STAT_LAYER_CUNTR_START 0.0
#define LDG_MATH_STAT_LAYER_CUNTR_STEP 1.0
#define LDG_MATH_STAT_LAYER_MASK_ON 1.0
#define LDG_MATH_STAT_LAYER_MASK_OFF 0.0
#define LDG_MATH_STAT_LAYER_BIAS_INIT 0.0
#define LDG_MATH_STAT_LAYER_GAIN_INIT 1.0
#define LDG_MATH_STAT_LAYER_NEURON_WEIGHT 0
#define LDG_MATH_STAT_LAYER_NEURON_BIAS 1
#define LDG_MATH_STAT_LAYER_DELAY_INPUT 0
#define LDG_MATH_STAT_LAYER_DELAY_MIN 1

typedef struct ldg_math_stat_layer_ctx
{
    ldg_math_stat_autograd_t *graph;
    uint32_t *param_node;
    double *work;
    const ldg_math_stat_neuron_desc_t *neuron;
    ldg_math_stat_synapse_desc_t *synapse;
    ldg_math_stat_rng_cb_t rng_cb;
    void *rng_ctx;
    const uint8_t *out_mask;
    uint32_t row_max;
    uint8_t train_is;
    uint8_t live_is;
    uint8_t pudding[2];
} ldg_math_stat_layer_ctx_t;

uint32_t ldg_math_stat_layer_param_weight_push(ldg_math_stat_layer_ctx_t *ctx, uint32_t fan_in, uint32_t fan_out, uint32_t *out);
uint32_t ldg_math_stat_layer_param_vec_push(ldg_math_stat_layer_ctx_t *ctx, uint32_t dim, double init_val, uint32_t *out);
uint32_t ldg_math_stat_layer_param_leaf_push(ldg_math_stat_layer_ctx_t *ctx, const double *src, uint32_t row_cunt, uint32_t col_cunt, uint32_t *out);
uint32_t ldg_math_stat_layer_param_gate_push(ldg_math_stat_layer_ctx_t *ctx, uint32_t in_node, uint32_t state_node, uint32_t weight, uint32_t recur, uint32_t bias, uint32_t *out);
uint32_t ldg_math_stat_layer_param_stride_apply(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t row_cunt, uint32_t slot, uint32_t cand);
uint32_t ldg_math_stat_layer_param_stride_advance(ldg_math_stat_layer_t *layer, uint32_t row_cunt);
uint32_t ldg_math_stat_layer_param_state_ck(const ldg_math_stat_layer_ctx_t *ctx, const ldg_math_stat_layer_t *layer, uint32_t row_cunt, uint32_t slot_cunt);

uint32_t ldg_math_stat_layer_dense_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_dense_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_dense_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);

uint32_t ldg_math_stat_layer_norm_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_norm_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_norm_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);

uint32_t ldg_math_stat_layer_rms_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_rms_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_rms_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);

uint32_t ldg_math_stat_layer_gru_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_gru_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_gru_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);

uint32_t ldg_math_stat_layer_lstm_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_lstm_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_lstm_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);

uint32_t ldg_math_stat_layer_attn_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_attn_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_attn_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);

uint32_t ldg_math_stat_layer_synapse_src_width_get(const ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_synapse_ck(const ldg_math_stat_layer_t *layer, const ldg_math_stat_synapse_desc_t *synapse);
uint32_t ldg_math_stat_layer_synapse_col_get(const ldg_math_stat_layer_t *layer, const ldg_math_stat_synapse_desc_t *synapse);
uint32_t ldg_math_stat_layer_synapse_table_fill(const ldg_math_stat_layer_ctx_t *ctx, const ldg_math_stat_layer_t *layer, double *dst);
uint32_t ldg_math_stat_layer_synapse_trace_update(const ldg_math_stat_layer_ctx_t *ctx, const ldg_math_stat_layer_t *layer, const double *pre_now, const double *post_now, double *pre_trace, double *post_trace, double *elig);
uint32_t ldg_math_stat_layer_synapse_weight_apply(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, double reward, uint32_t row_cunt);

uint32_t ldg_math_stat_layer_neuron_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_neuron_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_neuron_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);
uintptr_t ldg_math_stat_layer_neuron_host_len_get(const ldg_math_stat_layer_t *layer, uint32_t row_max);
uintptr_t ldg_math_stat_layer_neuron_state_host_off_get(const ldg_math_stat_layer_t *layer);
uintptr_t ldg_math_stat_layer_neuron_elig_off_get(const ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_neuron_host_init(const ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);

uint32_t ldg_math_stat_layer_conv_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_conv_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_conv_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);
uintptr_t ldg_math_stat_layer_conv_host_len_get(const ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_conv_host_init(ldg_math_stat_layer_t *layer);

uint32_t ldg_math_stat_layer_linreg_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_linreg_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_linreg_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);
uintptr_t ldg_math_stat_layer_linreg_host_len_get(const ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_linreg_rst(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_linreg_observe(ldg_math_stat_layer_t *layer, const double *src, const double *tgt, uint32_t row_cunt);
uint32_t ldg_math_stat_layer_linreg_refit(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, double ridge);

uint32_t ldg_math_stat_layer_svm_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_svm_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_svm_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);
uintptr_t ldg_math_stat_layer_svm_host_len_get(const ldg_math_stat_layer_t *layer, uint32_t row_max);
uint32_t ldg_math_stat_layer_svm_rst(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_svm_fit(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, const double *src, const uint32_t *label, uint32_t row_cunt, double cost, uint32_t pass_cunt);

uint32_t ldg_math_stat_layer_cosine_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_cosine_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_cosine_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);

uint32_t ldg_math_stat_layer_euclid_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_euclid_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_euclid_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);

uint32_t ldg_math_stat_layer_pool_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_pool_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_pool_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);
uintptr_t ldg_math_stat_layer_pool_host_len_get(const ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_pool_host_init(ldg_math_stat_layer_t *layer);

uint32_t ldg_math_stat_layer_embedding_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_embedding_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_embedding_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);

uint32_t ldg_math_stat_layer_dropout_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_dropout_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_dropout_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);

uint32_t ldg_math_stat_layer_transformer_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_transformer_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_transformer_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);

uint32_t ldg_math_stat_layer_sequence_pool_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_sequence_pool_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_sequence_pool_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);

uint32_t ldg_math_stat_layer_sequence_hd_shape_set(ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_sequence_hd_build(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer);
uint32_t ldg_math_stat_layer_sequence_hd_forward(ldg_math_stat_layer_ctx_t *ctx, ldg_math_stat_layer_t *layer, uint32_t in_node, uint32_t row_cunt, uint32_t *out);

#endif
