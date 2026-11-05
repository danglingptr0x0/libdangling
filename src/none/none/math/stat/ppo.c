#include <stdint.h>

#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>
#include <dangling/math/math.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>

#include "rl.h"

#define PPO_GAMMA_DEFAULT 0.99
#define PPO_LAMBDA_DEFAULT 0.95
#define PPO_CLIP_DEFAULT 0.2
#define PPO_LR_DEFAULT 0.0003
#define PPO_VAL_COEF_DEFAULT 0.5
#define PPO_ENTROPY_COEF_DEFAULT 0.01
#define PPO_GRAD_NORM_DEFAULT 0.5
#define PPO_ADAM_BETA_MEAN_DEFAULT 0.9
#define PPO_ADAM_BETA_VAR_DEFAULT 0.999
#define PPO_ADAM_EPS_DEFAULT 0.00000001
#define PPO_OBS_DIM_DEFAULT 4
#define PPO_ACTION_DIM_DEFAULT 2
#define PPO_HIDDEN_DIM_DEFAULT 64
#define PPO_HIDDEN_CUNT_DEFAULT 2
#define PPO_STEP_CAP_DEFAULT 2048
#define PPO_BATCH_DEFAULT 64
#define PPO_EPOCH_DEFAULT 10
#define PPO_RECUR_DIM_DEFAULT 0
#define PPO_SEG_CAP_DEFAULT 64
#define PPO_SIL_SEG_CAP_DEFAULT 0
#define PPO_SIL_THRESHOLD_DEFAULT 0.0
#define PPO_SIL_WEIGHT_DEFAULT 1.0
#define PPO_NORM_DECAY_DEFAULT 0.99999
#define PPO_CRITIC_OBS_DIM_DEFAULT 0
#define PPO_BODY_CAP_DEFAULT 1
#define PPO_ENTITY_DIM_DEFAULT 0
#define PPO_ENTITY_CAP_DEFAULT 0
#define PPO_ATTN_DIM_DEFAULT 64
#define PPO_HD_CUNT_DEFAULT 4
#define PPO_DEMO_SEG_CAP_DEFAULT 0
#define PPO_DEMO_WEIGHT_DEFAULT 1.0
#define PPO_NEURON_CAP_DEFAULT 0
#define PPO_SYNAPSE_CAP_DEFAULT 0
#define PPO_SNAPSHOT_CAP_DEFAULT 0
#define PPO_OPPONENT_CAP_DEFAULT 0
#define PPO_OPPONENT_BODY_CAP_DEFAULT 1
#define PPO_OPPONENT_ROW 1
#define PPO_SNAPSHOT_HDR_CUNT 2
#define PPO_SNAPSHOT_HDR_SEEN 0
#define PPO_SNAPSHOT_HDR_FILL 1
#define PPO_SNAPSHOT_ENTRY_TAG 0
#define PPO_SNAPSHOT_ENTRY_QUALITY 1
#define PPO_SNAPSHOT_ENTRY_HDR_CUNT 2
#define PPO_RESERVOIR_SELF 1
#define PPO_OPPONENT_LR_DEFAULT 0.01
#define PPO_QUALITY_SEED 0.0
#define PPO_DEMO_HDR_CUNT 1
#define PPO_SIL_VAL_HALF 0.5
#define PPO_SIL_HDR_CUNT 2
#define PPO_SIL_PRIO_FLOOR 0.0
#define PPO_ROW_FIRST 0
#define PPO_LAYER_FIRST 0
#define PPO_LEN_NONE 0
#define PPO_NORM_EPS 0.00000001
#define PPO_NORM_SIGMA_UNIT 1.0

#define PPO_GAUSS_QUAD_COEF (-0.5)
#define PPO_LOG_TWO_PI 1.8378770664093453
#define PPO_GAUSS_ENTROPY_CONST 1.4189385332046903
#define PPO_ADV_EPS 0.00000001
#define PPO_UNIT_NEG (-1.0)
#define PPO_UNIT_POS 1.0
#define PPO_ZERO_SEED 0.0
#define PPO_MASK_ON 1.0
#define PPO_MASK_OFF 0.0
#define PPO_SCALAR_ROW 1
#define PPO_SCALAR_COL 1
#define PPO_NODE_SLACK 384
#define PPO_NODE_PER_STEP 192
#define PPO_ELEM_SLACK 256
#define PPO_ELEM_PER_ROW 48
#define PPO_CELL_GATE_CUNT 3
#define PPO_TAPE_FACTOR 2
#define PPO_MODEL_LAYER_CAP 16
#define PPO_MODEL_PARAM_CAP 2
#define PPO_NET_CUNT LDG_MATH_STAT_PPO_NET_CUNT
#define PPO_NET_ACTOR LDG_MATH_STAT_PPO_NET_ACTOR
#define PPO_NET_CRITIC LDG_MATH_STAT_PPO_NET_CRITIC
#define PPO_SEG_NONE 0
#define PPO_BUFF_LANE_CUNT 4
#define PPO_LANE_OBS 0
#define PPO_LANE_CRITIC_OBS 1
#define PPO_LANE_ACTION 2
#define PPO_LANE_LOGP 3
#define PPO_LANE_ADV 4
#define PPO_LANE_TGT 5
#define PPO_LANE_MASK 6
#define PPO_LANE_STATE 7
#define PPO_LANE_SAMPLE 8
#define PPO_CELL_LAYER 1
#define PPO_CELL_HIDDEN_MIN (PPO_CELL_LAYER + 1)

static ldg_math_stat_model_t* ppo_model_get(ldg_math_stat_ppo_t *ppo, uint32_t net)
{
    if (net == PPO_NET_CRITIC) { return &ppo->critic; }

    return &ppo->actor;
}

static uint32_t ppo_model_state_len_get(const ldg_math_stat_ppo_t *ppo, uint32_t net)
{
    if (net == PPO_NET_CRITIC) { return ppo->critic.state_len; }

    return ppo->actor.state_len;
}

static uint8_t ppo_recur_is(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo->actor.state_len > 0 || ppo->critic.state_len > 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t ppo_model_neuron_is(const ldg_math_stat_model_t *model)
{
    uint32_t idx = 0;

    while (idx < model->layer_cunt)
    {
        if (model->layer[idx].desc.kind == LDG_MATH_STAT_LAYER_NEURON) { return LDG_TRUTH_TRUE; }

        idx++;
    }

    return LDG_TRUTH_FALSE;
}

static uint8_t ppo_sil_is(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo->conf.sil_seg_cap > 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t ppo_demo_is(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo->conf.demo_seg_cap > 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t ppo_seg_is(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo_recur_is(ppo) == LDG_TRUTH_TRUE || ppo_sil_is(ppo) == LDG_TRUTH_TRUE || ppo_demo_is(ppo) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t ppo_attn_is(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo->conf.entity_cap > 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t ppo_split_is(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo->conf.critic_obs_dim > 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t ppo_norm_is(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo->conf.tgt_norm_is == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t ppo_ln_is(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo->conf.layer_norm_is == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t ppo_critic_dim_get(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE) { return ppo->conf.critic_obs_dim; }

    return ppo->conf.obs_dim;
}

static uint32_t ppo_seg_per_batch_get(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo_seg_is(ppo) != LDG_TRUTH_TRUE) { return PPO_SEG_NONE; }

    return ppo->conf.batch_size / ppo->conf.seg_cap;
}

static uint32_t ppo_action_width_get(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo->conf.action_kind == LDG_MATH_STAT_PPO_ACTION_DISCRETE) { return PPO_SCALAR_COL; }

    return ppo->conf.action_dim;
}

static uint8_t ppo_mask_is(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo->conf.action_kind == LDG_MATH_STAT_PPO_ACTION_DISCRETE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t ppo_mask_width_get(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo_mask_is(ppo) == LDG_TRUTH_TRUE) { return ppo->conf.action_dim; }

    return PPO_LEN_NONE;
}

static uint32_t ppo_sample_len_get(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo->conf.action_dim > ppo->conf.batch_size) { return ppo->conf.action_dim; }

    return ppo->conf.batch_size;
}

static uint32_t ppo_net_out_dim_get(const ldg_math_stat_ppo_t *ppo, uint32_t net)
{
    if (net == PPO_NET_CRITIC) { return PPO_SCALAR_COL; }

    return ppo->conf.action_dim;
}

static uint32_t ppo_net_in_dim_get(const ldg_math_stat_ppo_t *ppo, uint32_t net)
{
    if (net == PPO_NET_CRITIC) { return ppo_critic_dim_get(ppo); }

    return ppo->conf.obs_dim;
}

static uintptr_t ppo_sil_seg_len_get(const ldg_math_stat_ppo_t *ppo)
{
    uintptr_t seg_cap = ppo->conf.seg_cap;
    uintptr_t width = ppo_action_width_get(ppo);
    uintptr_t state_len = (uintptr_t)ppo->actor.state_len + (uintptr_t)ppo->critic.state_len;
    uintptr_t critic_len = 0;

    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE) { critic_len = ppo->conf.critic_obs_dim; }

    return (uintptr_t)PPO_SIL_HDR_CUNT + state_len + (seg_cap * ((uintptr_t)ppo->conf.obs_dim + critic_len + width + (uintptr_t)PPO_SCALAR_COL + (uintptr_t)ppo_mask_width_get(ppo)));
}

static uintptr_t ppo_demo_seg_len_get(const ldg_math_stat_ppo_t *ppo)
{
    uintptr_t seg_cap = ppo->conf.seg_cap;
    uintptr_t width = ppo_action_width_get(ppo);
    uintptr_t critic_len = 0;

    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE) { critic_len = ppo->conf.critic_obs_dim; }

    return (uintptr_t)PPO_DEMO_HDR_CUNT + (seg_cap * ((uintptr_t)ppo->conf.obs_dim + critic_len + width + (uintptr_t)PPO_SCALAR_COL + (uintptr_t)ppo_mask_width_get(ppo)));
}

static uintptr_t ppo_state_lane_len_get(const ldg_math_stat_ppo_t *ppo)
{
    return (uintptr_t)ppo_seg_per_batch_get(ppo) * ((uintptr_t)ppo->actor.state_len + (uintptr_t)ppo->critic.state_len);
}

static uintptr_t ppo_critic_lane_len_get(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo_split_is(ppo) != LDG_TRUTH_TRUE) { return PPO_LEN_NONE; }

    return (uintptr_t)ppo->conf.batch_size * (uintptr_t)ppo->conf.critic_obs_dim;
}

static double* ppo_lane_get(const ldg_math_stat_ppo_t *ppo, uint32_t lane)
{
    uintptr_t batch = ppo->conf.batch_size;
    uintptr_t off = batch * (uintptr_t)ppo->conf.obs_dim;

    if (lane == PPO_LANE_OBS) { return ppo->batch_buff; }

    if (lane == PPO_LANE_CRITIC_OBS)
    {
        if (ppo_split_is(ppo) != LDG_TRUTH_TRUE) { return ppo->batch_buff; }

        return ppo->batch_buff + off;
    }

    off = off + ppo_critic_lane_len_get(ppo);

    if (lane == PPO_LANE_ACTION) { return ppo->batch_buff + off; }

    off = off + (batch * (uintptr_t)ppo_action_width_get(ppo));

    if (lane == PPO_LANE_LOGP) { return ppo->batch_buff + off; }

    if (lane == PPO_LANE_ADV) { return ppo->batch_buff + off + batch; }

    if (lane == PPO_LANE_TGT) { return ppo->batch_buff + off + (batch * (uintptr_t)PPO_NET_CUNT); }

    if (lane == PPO_LANE_MASK) { return ppo->batch_buff + off + (batch * (uintptr_t)PPO_CELL_GATE_CUNT); }

    off = off + (batch * (uintptr_t)PPO_BUFF_LANE_CUNT);

    if (lane == PPO_LANE_STATE) { return ppo->batch_buff + off; }

    return ppo->batch_buff + off + ppo_state_lane_len_get(ppo);
}

static uintptr_t ppo_state_len_get(const ldg_math_stat_ppo_t *ppo)
{
    return (uintptr_t)ppo->actor.state_len + (uintptr_t)ppo->critic.state_len;
}

static double* ppo_state_body_get(const ldg_math_stat_ppo_t *ppo, double *base, uint32_t body)
{
    return base + ((uintptr_t)body * ppo_state_len_get(ppo));
}

static double* ppo_state_slot_get(const ldg_math_stat_ppo_t *ppo, double *base, uint32_t net)
{
    if (net == PPO_NET_CRITIC) { return base + (uintptr_t)ppo->actor.state_len; }

    return base;
}

static double* ppo_state_step_get(const ldg_math_stat_ppo_t *ppo, uint32_t step)
{
    return ppo->state_buff + ((uintptr_t)step * ppo_state_len_get(ppo));
}

static uint32_t ppo_mask_row_ck(const uint8_t *mask, uint32_t dim)
{
    uint32_t allowed = 0;
    uint32_t idx = 0;

    while (idx < dim)
    {
        if (LDG_UNLIKELY(mask[idx] > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        allowed = allowed + mask[idx];
        idx++;
    }

    if (LDG_UNLIKELY(allowed == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t ppo_mask_arg_ck(const ldg_math_stat_ppo_t *ppo, const uint8_t *mask)
{
    if (!mask) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(ppo_mask_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ppo_mask_row_ck(mask, ppo->conf.action_dim);
}

static uint32_t ppo_mask_st(const ldg_math_stat_ppo_t *ppo, const uint8_t *src, uint8_t *base, uintptr_t row)
{
    uintptr_t dim = ppo_mask_width_get(ppo);

    if (dim == PPO_LEN_NONE) { return LDG_ERR_AOK; }

    if (!src) { return ldg_mem_fill(base + (row * dim), (uint8_t)LDG_TRUTH_TRUE, (uint64_t)dim); }

    return ldg_mem_cpy(base + (row * dim), src, (uint64_t)dim);
}

static uint32_t ppo_mask_row_cpy(const ldg_math_stat_ppo_t *ppo, uint8_t *dst_base, uintptr_t dst_row, const uint8_t *src_base, uintptr_t src_row)
{
    uintptr_t dim = ppo_mask_width_get(ppo);

    if (dim == PPO_LEN_NONE) { return LDG_ERR_AOK; }

    return ldg_mem_cpy(dst_base + (dst_row * dim), src_base + (src_row * dim), (uint64_t)dim);
}

static void ppo_mask_to_double_cpy(const uint8_t *src, double *dst, uintptr_t cunt)
{
    const uint8_t *end = src + cunt;

    while (src < end)
    {
        *dst = (double)*src;
        src++;
        dst++;
    }
}

static void ppo_double_to_mask_cpy(const double *src, uint8_t *dst, uintptr_t cunt)
{
    const double *end = src + cunt;

    while (src < end)
    {
        *dst = (uint8_t)*src;
        src++;
        dst++;
    }
}

static uint32_t ppo_mask_seg_ck(const ldg_math_stat_ppo_t *ppo, const double *seg, uintptr_t seg_len)
{
    uintptr_t dim = ppo_mask_width_get(ppo);
    const double *mask_row = 0x0;
    uintptr_t idx = 0;
    uint32_t len = 0;
    uint32_t allowed = 0;
    uint32_t step = 0;

    if (dim == PPO_LEN_NONE) { return LDG_ERR_AOK; }

    len = (uint32_t)seg[PPO_ROW_FIRST];
    mask_row = seg + seg_len - ((uintptr_t)ppo->conf.seg_cap * dim);

    while (step < len)
    {
        allowed = 0;
        idx = 0;
        while (idx < dim)
        {
            if (LDG_UNLIKELY(mask_row[idx] != PPO_MASK_ON && mask_row[idx] != PPO_MASK_OFF)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (mask_row[idx] == PPO_MASK_ON) { allowed++; }

            idx++;
        }

        if (LDG_UNLIKELY(allowed == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        mask_row = mask_row + dim;
        step++;
    }

    return LDG_ERR_AOK;
}

static uint32_t ppo_leaf_push(ldg_math_stat_ppo_t *ppo, const double *src, uint32_t row_cunt, uint32_t col_cunt, uint32_t *out)
{
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_tensor_push(&ppo->graph, row_cunt, col_cunt, LDG_TRUTH_FALSE, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_val_set(&ppo->graph, *out, src, (uintptr_t)row_cunt * (uintptr_t)col_cunt);
}

static uint32_t ppo_logp_discrete_push(ldg_math_stat_ppo_t *ppo, uint32_t logits, uint32_t act_node, const uint8_t *act_mask, uint32_t *out_logp, uint32_t *out_entropy)
{
    uint32_t logp_all = 0;
    uint32_t prob = 0;
    uint32_t prod = 0;
    uint32_t row = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_log_softmax_push(&ppo->graph, logits, act_mask, &logp_all);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_gather_push(&ppo->graph, logp_all, act_node, out_logp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_exp_push(&ppo->graph, logp_all, &prob);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mul_push(&ppo->graph, prob, logp_all, &prod);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_row_sum_push(&ppo->graph, prod, &row);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_scale_push(&ppo->graph, row, PPO_UNIT_NEG, out_entropy);
}

static uint32_t ppo_logp_cont_push(ldg_math_stat_ppo_t *ppo, uint32_t mu, uint32_t act_node, uint32_t *out_logp, uint32_t *out_entropy)
{
    uint32_t inv_scale = 0;
    uint32_t neg_log_std = 0;
    uint32_t diff = 0;
    uint32_t norm = 0;
    uint32_t sq = 0;
    uint32_t quad = 0;
    uint32_t scaled = 0;
    uint32_t log_std_sum = 0;
    uint32_t neg_sum = 0;
    uint32_t shifted = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, ppo->log_std, PPO_UNIT_NEG, &neg_log_std);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_exp_push(&ppo->graph, neg_log_std, &inv_scale);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_sub_push(&ppo->graph, act_node, mu, &diff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_row_mul_push(&ppo->graph, diff, inv_scale, &norm);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_square_push(&ppo->graph, norm, &sq);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_row_sum_push(&ppo->graph, sq, &quad);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, quad, PPO_GAUSS_QUAD_COEF, &scaled);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_row_sum_push(&ppo->graph, ppo->log_std, &log_std_sum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, log_std_sum, PPO_UNIT_NEG, &neg_sum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_bias_add_push(&ppo->graph, scaled, neg_sum, &shifted);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_shift_push(&ppo->graph, shifted, PPO_GAUSS_QUAD_COEF * (double)ppo->conf.action_dim * PPO_LOG_TWO_PI, out_logp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_shift_push(&ppo->graph, log_std_sum, PPO_GAUSS_ENTROPY_CONST * (double)ppo->conf.action_dim, out_entropy);
}

static uint32_t ppo_logp_push(ldg_math_stat_ppo_t *ppo, uint32_t hd, uint32_t act_node, const uint8_t *act_mask, uint32_t *out_logp, uint32_t *out_entropy)
{
    if (ppo->conf.action_kind == LDG_MATH_STAT_PPO_ACTION_DISCRETE) { return ppo_logp_discrete_push(ppo, hd, act_node, act_mask, out_logp, out_entropy); }

    return ppo_logp_cont_push(ppo, hd, act_node, out_logp, out_entropy);
}

uint32_t ldg_math_stat_ppo_conf_default_get(ldg_math_stat_ppo_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->gamma = PPO_GAMMA_DEFAULT;
    out->lambda = PPO_LAMBDA_DEFAULT;
    out->clip = PPO_CLIP_DEFAULT;
    out->lr = PPO_LR_DEFAULT;
    out->val_coef = PPO_VAL_COEF_DEFAULT;
    out->entropy_coef = PPO_ENTROPY_COEF_DEFAULT;
    out->grad_norm_max = PPO_GRAD_NORM_DEFAULT;
    out->adam_beta_mean = PPO_ADAM_BETA_MEAN_DEFAULT;
    out->adam_beta_var = PPO_ADAM_BETA_VAR_DEFAULT;
    out->adam_eps = PPO_ADAM_EPS_DEFAULT;
    out->sil_threshold = PPO_SIL_THRESHOLD_DEFAULT;
    out->sil_weight = PPO_SIL_WEIGHT_DEFAULT;
    out->norm_decay = PPO_NORM_DECAY_DEFAULT;
    out->obs_dim = PPO_OBS_DIM_DEFAULT;
    out->action_dim = PPO_ACTION_DIM_DEFAULT;
    out->hidden_dim = PPO_HIDDEN_DIM_DEFAULT;
    out->hidden_cunt = PPO_HIDDEN_CUNT_DEFAULT;
    out->step_cap = PPO_STEP_CAP_DEFAULT;
    out->batch_size = PPO_BATCH_DEFAULT;
    out->epoch_cunt = PPO_EPOCH_DEFAULT;
    out->action_kind = LDG_MATH_STAT_PPO_ACTION_DISCRETE;
    out->recur_dim = PPO_RECUR_DIM_DEFAULT;
    out->seg_cap = PPO_SEG_CAP_DEFAULT;
    out->sil_seg_cap = PPO_SIL_SEG_CAP_DEFAULT;
    out->critic_obs_dim = PPO_CRITIC_OBS_DIM_DEFAULT;
    out->body_cap = PPO_BODY_CAP_DEFAULT;
    out->entity_dim = PPO_ENTITY_DIM_DEFAULT;
    out->entity_cap = PPO_ENTITY_CAP_DEFAULT;
    out->attn_dim = PPO_ATTN_DIM_DEFAULT;
    out->hd_cunt = PPO_HD_CUNT_DEFAULT;
    out->demo_seg_cap = PPO_DEMO_SEG_CAP_DEFAULT;
    out->demo_weight = PPO_DEMO_WEIGHT_DEFAULT;
    out->neuron_cap = PPO_NEURON_CAP_DEFAULT;
    out->synapse_cap = PPO_SYNAPSE_CAP_DEFAULT;
    out->snapshot_cap = PPO_SNAPSHOT_CAP_DEFAULT;
    out->opponent_cap = PPO_OPPONENT_CAP_DEFAULT;
    out->opponent_body_cap = PPO_OPPONENT_BODY_CAP_DEFAULT;
    out->opponent_lr = PPO_OPPONENT_LR_DEFAULT;
    out->tgt_norm_is = LDG_TRUTH_FALSE;
    out->layer_norm_is = LDG_TRUTH_FALSE;
    out->act_keep_is = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint32_t ppo_conf_attn_ck(const ldg_math_stat_ppo_conf_t *conf)
{
    uint32_t entity_len = conf->entity_cap * (conf->entity_dim + PPO_SCALAR_COL);
    uint32_t critic_dim = conf->critic_obs_dim;

    if (critic_dim == 0) { critic_dim = conf->obs_dim; }

    if (conf->entity_cap == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(conf->entity_dim == 0 || conf->attn_dim == 0 || conf->hd_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->attn_dim % conf->hd_cunt != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->obs_dim <= entity_len || critic_dim <= entity_len)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t ppo_conf_ck(const ldg_math_stat_ppo_conf_t *conf)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(conf->obs_dim == 0 || conf->action_dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->hidden_cunt > LDG_MATH_STAT_PPO_LAYER_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->hidden_cunt > 0 && conf->hidden_dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->step_cap == 0 || conf->batch_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->batch_size > conf->step_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->epoch_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->action_kind >= LDG_MATH_STAT_PPO_ACTION_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->body_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->tgt_norm_is > LDG_TRUTH_TRUE || conf->layer_norm_is > LDG_TRUTH_TRUE || conf->act_keep_is > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->tgt_norm_is == LDG_TRUTH_TRUE && (conf->norm_decay <= PPO_ZERO_SEED || conf->norm_decay >= PPO_UNIT_POS))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->recur_dim > 0 && conf->hidden_cunt < PPO_CELL_HIDDEN_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->sil_seg_cap > 0 && conf->sil_weight <= PPO_SIL_PRIO_FLOOR)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->sil_seg_cap > conf->step_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->demo_seg_cap > 0 && conf->demo_weight <= PPO_SIL_PRIO_FLOOR)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->demo_seg_cap > conf->step_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->opponent_cap > LDG_MATH_STAT_PPO_OPPONENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->opponent_cap > 0 && (conf->snapshot_cap == 0 || conf->opponent_body_cap == 0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->snapshot_cap > 0 && conf->opponent_lr <= PPO_ZERO_SEED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ppo_conf_attn_ck(conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (conf->recur_dim == 0 && conf->sil_seg_cap == 0 && conf->demo_seg_cap == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(conf->seg_cap == 0 || conf->seg_cap > conf->batch_size)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->batch_size % conf->seg_cap != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t ppo_float_buff_alloc(uint64_t cunt, double **out)
{
    void *ptr = 0x0;
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc(cunt * (uint64_t)sizeof(double), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = (double *)ptr;

    return ldg_mem_zero(ptr, cunt * (uint64_t)sizeof(double));
}

static uint32_t ppo_idx_buff_alloc(uint64_t cunt, uint32_t **out)
{
    void *ptr = 0x0;
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc(cunt * (uint64_t)sizeof(uint32_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = (uint32_t *)ptr;

    return ldg_mem_zero(ptr, cunt * (uint64_t)sizeof(uint32_t));
}

static uint32_t ppo_mask_buff_alloc(uint64_t cunt, uint8_t **out)
{
    void *ptr = 0x0;
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc(cunt, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = (uint8_t *)ptr;

    return ldg_mem_fill(ptr, (uint8_t)LDG_TRUTH_TRUE, cunt);
}

static uint32_t ppo_state_buff_alloc(ldg_math_stat_ppo_t *ppo)
{
    uint64_t state_len = (uint64_t)ppo_state_len_get(ppo);
    uint64_t body_len = state_len * (uint64_t)ppo->conf.body_cap;
    uint32_t ret = 0;

    ret = ppo_float_buff_alloc((uint64_t)ppo->conf.step_cap * state_len, &ppo->state_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc(body_len, &ppo->state_curr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ppo_float_buff_alloc(body_len, &ppo->state_pend);
}

static uint32_t ppo_sil_buff_alloc(ldg_math_stat_ppo_t *ppo)
{
    uint64_t row_cunt = (uint64_t)ppo->conf.sil_seg_cap * (uint64_t)ppo->conf.seg_cap;
    uint32_t ret = 0;

    ret = ppo_float_buff_alloc(row_cunt * (uint64_t)ppo->conf.obs_dim, &ppo->sil_obs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ppo_float_buff_alloc(row_cunt * (uint64_t)ppo->conf.critic_obs_dim, &ppo->sil_critic_obs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ppo_float_buff_alloc(row_cunt * (uint64_t)ppo_action_width_get(ppo), &ppo->sil_action);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc(row_cunt, &ppo->sil_tgt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc((uint64_t)ppo->conf.sil_seg_cap, &ppo->sil_prio);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc((uint64_t)ppo->conf.step_cap, &ppo->sil_step_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_mask_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ppo_mask_buff_alloc(row_cunt * (uint64_t)ppo->conf.action_dim, &ppo->sil_action_mask);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ppo_idx_buff_alloc((uint64_t)ppo->conf.sil_seg_cap, &ppo->sil_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_recur_is(ppo) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return ppo_float_buff_alloc((uint64_t)ppo->conf.sil_seg_cap * (uint64_t)ppo_state_len_get(ppo), &ppo->sil_state);
}

static uint8_t ppo_act_is(const ldg_math_stat_ppo_t *ppo)
{
    if (ppo->conf.act_keep_is == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static const ldg_math_stat_model_t* ppo_model_view_get(const ldg_math_stat_ppo_t *ppo, uint32_t net)
{
    if (net == PPO_NET_CRITIC) { return &ppo->critic; }

    return &ppo->actor;
}

static uintptr_t ppo_act_net_len_get(const ldg_math_stat_ppo_t *ppo, uint32_t net)
{
    const ldg_math_stat_model_t *model = ppo_model_view_get(ppo, net);
    uintptr_t len = 0;
    uint32_t idx = 0;

    while (idx < model->layer_cunt)
    {
        len = len + (uintptr_t)model->layer[idx].out_dim;
        idx++;
    }

    return len;
}

static const ldg_math_stat_layer_t* ppo_attn_layer_get(const ldg_math_stat_ppo_t *ppo, uint32_t net)
{
    const ldg_math_stat_model_t *model = ppo_model_view_get(ppo, net);

    if (model->layer_cunt == 0) { return 0x0; }

    if (model->layer[PPO_LAYER_FIRST].desc.kind != LDG_MATH_STAT_LAYER_ATTN) { return 0x0; }

    return &model->layer[PPO_LAYER_FIRST];
}

static uintptr_t ppo_act_attn_len_get(const ldg_math_stat_ppo_t *ppo, uint32_t net)
{
    const ldg_math_stat_layer_t *layer = ppo_attn_layer_get(ppo, net);

    if (!layer) { return PPO_LEN_NONE; }

    return (uintptr_t)layer->desc.hd_cunt * (uintptr_t)layer->desc.entity_cap * (uintptr_t)layer->desc.entity_cap;
}

static double* ppo_act_net_get(const ldg_math_stat_ppo_t *ppo, uint32_t net)
{
    if (net == PPO_NET_CRITIC) { return ppo->act_buff + ppo_act_net_len_get(ppo, PPO_NET_ACTOR); }

    return ppo->act_buff;
}

static double* ppo_act_attn_get(const ldg_math_stat_ppo_t *ppo, uint32_t net)
{
    double *base = ppo->act_buff + ppo_act_net_len_get(ppo, PPO_NET_ACTOR) + ppo_act_net_len_get(ppo, PPO_NET_CRITIC);

    if (net == PPO_NET_CRITIC) { return base + ppo_act_attn_len_get(ppo, PPO_NET_ACTOR); }

    return base;
}

static uint32_t ppo_demo_buff_alloc(ldg_math_stat_ppo_t *ppo)
{
    uint64_t row_cunt = (uint64_t)ppo->conf.demo_seg_cap * (uint64_t)ppo->conf.seg_cap;
    uint32_t ret = 0;

    ret = ppo_float_buff_alloc(row_cunt * (uint64_t)ppo->conf.obs_dim, &ppo->demo_obs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ppo_float_buff_alloc(row_cunt * (uint64_t)ppo->conf.critic_obs_dim, &ppo->demo_critic_obs);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ppo_float_buff_alloc(row_cunt * (uint64_t)ppo_action_width_get(ppo), &ppo->demo_action);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc(row_cunt, &ppo->demo_weight);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_mask_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ppo_mask_buff_alloc(row_cunt * (uint64_t)ppo->conf.action_dim, &ppo->demo_action_mask);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return ppo_idx_buff_alloc((uint64_t)ppo->conf.demo_seg_cap, &ppo->demo_len);
}

static uint32_t ppo_buff_alloc(ldg_math_stat_ppo_t *ppo)
{
    void *ptr = 0x0;
    uint64_t step_cap = (uint64_t)ppo->conf.step_cap;
    uint64_t width = (uint64_t)ppo_action_width_get(ppo);
    uint64_t batch = (uint64_t)ppo->conf.batch_size;
    uint64_t batch_len = 0;
    uint32_t ret = 0;

    ret = ppo_float_buff_alloc(step_cap * (uint64_t)ppo->conf.obs_dim, &ppo->obs_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ppo_float_buff_alloc(step_cap * (uint64_t)ppo->conf.critic_obs_dim, &ppo->critic_obs_buff);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ppo_float_buff_alloc(step_cap * width, &ppo->action_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc(step_cap, &ppo->logp_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc(step_cap, &ppo->val_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc(step_cap, &ppo->reward_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc(step_cap, &ppo->adv_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc(step_cap, &ppo->tgt_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc(step_cap, &ppo->bootstrap_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc((uint64_t)ppo->conf.body_cap, &ppo->body_carry);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc((uint64_t)ppo->conf.body_cap, &ppo->body_next);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_idx_buff_alloc(step_cap, &ppo->idx_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_idx_buff_alloc(step_cap + (uint64_t)PPO_SCALAR_ROW, &ppo->seg_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_idx_buff_alloc(step_cap, &ppo->seg_id_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_idx_buff_alloc(step_cap, &ppo->seg_step_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_idx_buff_alloc(step_cap, &ppo->body_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_idx_buff_alloc((uint64_t)ppo->conf.body_cap, &ppo->body_last);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_idx_buff_alloc((uint64_t)ppo->conf.body_cap, &ppo->body_seg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_alloc(step_cap, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo->terminal_buff = (uint8_t *)ptr;

    if (ppo_mask_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ppo_mask_buff_alloc(step_cap * (uint64_t)ppo->conf.action_dim, &ppo->action_mask_buff);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ppo_mask_buff_alloc(batch * (uint64_t)ppo->conf.action_dim, &ppo->batch_action_mask);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    batch_len = (batch * ((uint64_t)ppo->conf.obs_dim + width + (uint64_t)PPO_BUFF_LANE_CUNT)) + (uint64_t)ppo_critic_lane_len_get(ppo) + (uint64_t)ppo_state_lane_len_get(ppo) + (uint64_t)ppo_sample_len_get(ppo);

    ret = ppo_float_buff_alloc(batch_len, &ppo->batch_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_recur_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ppo_state_buff_alloc(ppo);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ppo_demo_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ppo_demo_buff_alloc(ppo);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ppo_act_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ppo_float_buff_alloc((uint64_t)(ppo_act_net_len_get(ppo, PPO_NET_ACTOR) + ppo_act_net_len_get(ppo, PPO_NET_CRITIC) + ppo_act_attn_len_get(ppo, PPO_NET_ACTOR) + ppo_act_attn_len_get(ppo, PPO_NET_CRITIC)), &ppo->act_buff);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ppo_sil_is(ppo) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return ppo_sil_buff_alloc(ppo);
}

static uint32_t ppo_layer_push(ldg_math_stat_model_t *model, uint32_t kind, uint32_t dim, uint32_t act, ldg_math_stat_layer_desc_t *desc)
{
    uint32_t layer = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_model_layer_desc_default_get(kind, dim, desc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    desc->act = act;

    return ldg_math_stat_model_layer_add(model, desc, &layer);
}

static uint32_t ppo_default_layer_add(ldg_math_stat_ppo_t *ppo, uint32_t net)
{
    ldg_math_stat_model_t *model = ppo_model_get(ppo, net);
    ldg_math_stat_layer_desc_t desc = LDG_STRUCT_ZERO_INIT;
    uint32_t act = LDG_MATH_STAT_LAYER_ACT_TANH;
    uint32_t layer = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (ppo_ln_is(ppo) == LDG_TRUTH_TRUE) { act = LDG_MATH_STAT_LAYER_ACT_NONE; }

    if (ppo_attn_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ldg_math_stat_model_layer_desc_default_get(LDG_MATH_STAT_LAYER_ATTN, ppo->conf.attn_dim, &desc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        desc.entity_dim = ppo->conf.entity_dim;
        desc.entity_cap = ppo->conf.entity_cap;
        desc.hd_cunt = ppo->conf.hd_cunt;

        ret = ldg_math_stat_model_layer_add(model, &desc, &layer);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    while (idx < ppo->conf.hidden_cunt)
    {
        if (idx == PPO_CELL_LAYER && ppo->conf.recur_dim > 0)
        {
            ret = ldg_math_stat_model_layer_desc_default_get(LDG_MATH_STAT_LAYER_GRU, ppo->conf.recur_dim, &desc);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            desc.concat_is = LDG_TRUTH_TRUE;

            ret = ldg_math_stat_model_layer_add(model, &desc, &layer);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = ppo_layer_push(model, LDG_MATH_STAT_LAYER_DENSE, ppo->conf.hidden_dim, act, &desc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ppo_ln_is(ppo) == LDG_TRUTH_TRUE)
        {
            ret = ppo_layer_push(model, LDG_MATH_STAT_LAYER_NORM, ppo->conf.hidden_dim, LDG_MATH_STAT_LAYER_ACT_TANH, &desc);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        idx++;
    }

    return ppo_layer_push(model, LDG_MATH_STAT_LAYER_DENSE, ppo_net_out_dim_get(ppo, net), LDG_MATH_STAT_LAYER_ACT_NONE, &desc);
}

uint32_t ldg_math_stat_ppo_model_init(ldg_math_stat_ppo_t *ppo, const ldg_math_stat_ppo_conf_t *conf, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx)
{
    ldg_math_stat_model_conf_t model_conf = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !conf || !rng_cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_conf_ck(conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(ppo, (uint64_t)sizeof(ldg_math_stat_ppo_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo->conf = *conf;
    ppo->rng_cb = rng_cb;
    ppo->rng_ctx = rng_ctx;
    ppo->log_std = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;
    ppo->demo_open = UINT32_MAX;

    ret = ldg_math_stat_model_conf_default_get(&model_conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    model_conf.row_max = conf->batch_size;
    model_conf.row_step_max = conf->batch_size;
    model_conf.step_max = PPO_SCALAR_ROW;
    if (conf->seg_cap > 0) { model_conf.step_max = conf->seg_cap; }

    model_conf.layer_cap = PPO_MODEL_LAYER_CAP;
    model_conf.param_cap = PPO_MODEL_PARAM_CAP;
    model_conf.extra_node_cap = 0;
    model_conf.extra_elem_cap = 0;
    model_conf.neuron_cap = conf->neuron_cap;
    model_conf.synapse_cap = conf->synapse_cap;

    model_conf.in_dim = ppo_net_in_dim_get(ppo, PPO_NET_ACTOR);

    ret = ldg_math_stat_model_init(&ppo->actor, &model_conf, &ppo->graph, rng_cb, rng_ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    model_conf.in_dim = ppo_net_in_dim_get(ppo, PPO_NET_CRITIC);

    return ldg_math_stat_model_init(&ppo->critic, &model_conf, &ppo->graph, rng_cb, rng_ctx);
}

uint32_t ldg_math_stat_ppo_model_get(ldg_math_stat_ppo_t *ppo, uint32_t net, ldg_math_stat_model_t **out)
{
    if (LDG_UNLIKELY(!ppo || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(ppo->actor.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(net >= PPO_NET_CUNT)) { return LDG_ERR_BOUNDS; }

    *out = ppo_model_get(ppo, net);

    return LDG_ERR_AOK;
}

static uint32_t ppo_snapshot_alloc(ldg_math_stat_ppo_t *ppo)
{
    void *ptr = 0x0;
    uint64_t cap = ppo->conf.snapshot_cap;
    uint32_t ret = 0;

    if (cap == 0) { return LDG_ERR_AOK; }

    ret = ppo_float_buff_alloc(cap * (uint64_t)ppo->actor.param_elem_cunt, &ppo->snapshot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc(cap, &ppo->snapshot_quality);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_alloc(cap * (uint64_t)sizeof(uint64_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo->snapshot_tag = (uint64_t *)ptr;

    return ldg_mem_zero(ptr, cap * (uint64_t)sizeof(uint64_t));
}

static uint32_t ppo_opponent_layer_copy(ldg_math_stat_ppo_t *ppo, ldg_math_stat_ppo_opponent_t *opp)
{
    const ldg_math_stat_layer_t *layer = 0x0;
    uint32_t layer_idx = 0;
    uint32_t added = 0;
    uint32_t idx = 0;
    uint32_t sub = 0;
    uint32_t ret = 0;

    while (idx < ppo->actor.layer_cunt)
    {
        layer = &ppo->actor.layer[idx];

        ret = ldg_math_stat_model_layer_add(&opp->model, &layer->desc, &layer_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (layer->desc.kind == LDG_MATH_STAT_LAYER_NEURON)
        {
            sub = 0;
            while (sub < layer->desc.dim)
            {
                ret = ldg_math_stat_model_neuron_set(&opp->model, layer_idx, sub, &ppo->actor.neuron[layer->neuron_off + sub]);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                sub++;
            }

            sub = 0;
            while (sub < layer->synapse_cunt)
            {
                ret = ldg_math_stat_model_synapse_add(&opp->model, layer_idx, &ppo->actor.synapse[layer->synapse_off + sub], &added);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                sub++;
            }
        }

        idx++;
    }

    return LDG_ERR_AOK;
}

static uint32_t ppo_opponent_build(ldg_math_stat_ppo_t *ppo, uint32_t opponent)
{
    ldg_math_stat_ppo_opponent_t *opp = &ppo->opponent[opponent];
    ldg_math_stat_model_conf_t model_conf = ppo->actor.conf;
    uintptr_t elem_cap = 0;
    uint32_t node_cap = 0;
    uint32_t log_std_idx = UINT32_MAX;
    uint32_t added = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    model_conf.row_max = PPO_OPPONENT_ROW;
    model_conf.step_max = PPO_OPPONENT_ROW;

    opp->log_std = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;
    opp->ld_is = LDG_TRUTH_FALSE;

    ret = ldg_math_stat_model_init(&opp->model, &model_conf, &opp->graph, ppo->rng_cb, ppo->rng_ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_opponent_layer_copy(ppo, opp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (idx < ppo->actor.param_cunt)
    {
        ret = ldg_math_stat_model_param_add(&opp->model, ppo->actor.param[idx].row_cunt, ppo->actor.param[idx].col_cunt, ppo->actor.param[idx].init_val, &added);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ppo->actor.param[idx].node == ppo->log_std) { log_std_idx = added; }

        idx++;
    }

    ret = ldg_math_stat_model_cap_get(&opp->model, &node_cap, &elem_cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_init(&opp->graph, node_cap, node_cap * PPO_TAPE_FACTOR, elem_cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_build(&opp->model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (log_std_idx != UINT32_MAX)
    {
        ret = ldg_math_stat_model_param_node_get(&opp->model, log_std_idx, &opp->log_std);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_math_stat_autograd_param_mark_set(&opp->graph);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(opp->model.param_elem_cunt != ppo->actor.param_elem_cunt)) { return LDG_ERR_INVALID; }

    if (opp->model.state_len == 0) { return LDG_ERR_AOK; }

    return ppo_float_buff_alloc((uint64_t)ppo->conf.opponent_body_cap * (uint64_t)opp->model.state_len, &opp->state);
}

static uint32_t ppo_model_shape_ck(const ldg_math_stat_ppo_t *ppo)
{
    if (LDG_UNLIKELY(ppo->actor.out_dim != ppo_net_out_dim_get(ppo, PPO_NET_ACTOR))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ppo->critic.out_dim != ppo_net_out_dim_get(ppo, PPO_NET_CRITIC))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ppo_recur_is(ppo) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(ppo->conf.seg_cap == 0 || ppo->conf.seg_cap > ppo->conf.batch_size)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ppo->conf.batch_size % ppo->conf.seg_cap != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t ppo_frozen_alloc(ldg_math_stat_ppo_t *ppo)
{
    void *ptr = 0x0;
    uint32_t ret = 0;

    ret = ldg_mem_alloc((uint64_t)ppo->param_cunt, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo->adam_frozen = (uint8_t *)ptr;

    return ldg_mem_zero(ptr, (uint64_t)ppo->param_cunt);
}

uint32_t ldg_math_stat_ppo_build(ldg_math_stat_ppo_t *ppo)
{
    uintptr_t actor_elem = 0;
    uintptr_t critic_elem = 0;
    uintptr_t elem_cap = 0;
    uintptr_t step_max = PPO_SCALAR_ROW;
    uint32_t actor_node = 0;
    uint32_t critic_node = 0;
    uint32_t node_cap = 0;
    uint32_t log_std_idx = UINT32_MAX;
    uint32_t opponent = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->actor.is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo->is_built == LDG_TRUTH_TRUE)) { return LDG_ERR_BUSY; }

    if (ppo->conf.action_kind == LDG_MATH_STAT_PPO_ACTION_CONT)
    {
        ret = ldg_math_stat_model_param_add(&ppo->actor, PPO_SCALAR_ROW, ppo->conf.action_dim, PPO_ZERO_SEED, &log_std_idx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_math_stat_model_cap_get(&ppo->actor, &actor_node, &actor_elem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_cap_get(&ppo->critic, &critic_node, &critic_elem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_model_shape_ck(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo->conf.seg_cap > 0) { step_max = ppo->conf.seg_cap; }

    node_cap = actor_node + critic_node + PPO_NODE_SLACK + (PPO_NODE_PER_STEP * (uint32_t)step_max);
    elem_cap = actor_elem + critic_elem + ((uintptr_t)ppo->conf.batch_size * (uintptr_t)PPO_ELEM_PER_ROW * ((uintptr_t)ppo->conf.action_dim + (uintptr_t)PPO_SCALAR_COL)) + (uintptr_t)PPO_ELEM_SLACK;

    ret = ldg_math_stat_autograd_init(&ppo->graph, node_cap, node_cap * PPO_TAPE_FACTOR, elem_cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_build(&ppo->actor);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_build(&ppo->critic);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (log_std_idx != UINT32_MAX)
    {
        ret = ldg_math_stat_model_param_node_get(&ppo->actor, log_std_idx, &ppo->log_std);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_math_stat_autograd_param_mark_set(&ppo->graph);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo->param_cunt = ppo->graph.elem_mark;

    ret = ppo_snapshot_alloc(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (opponent < ppo->conf.opponent_cap)
    {
        ret = ppo_opponent_build(ppo, opponent);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        opponent++;
    }

    ret = ppo_float_buff_alloc((uint64_t)ppo->param_cunt, &ppo->adam_mean);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_float_buff_alloc((uint64_t)ppo->param_cunt, &ppo->adam_var);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_frozen_alloc(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_buff_alloc(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_fill(ppo->body_last, (uint8_t)LDG_BYTE_MASK, (uint64_t)ppo->conf.body_cap * (uint64_t)sizeof(uint32_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo->is_built = LDG_TRUTH_TRUE;
    ppo->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_init(ldg_math_stat_ppo_t *ppo, const ldg_math_stat_ppo_conf_t *conf, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx)
{
    uint32_t ret = 0;

    ret = ldg_math_stat_ppo_model_init(ppo, conf, rng_cb, rng_ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_default_layer_add(ppo, PPO_NET_ACTOR);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_default_layer_add(ppo, PPO_NET_CRITIC);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_ppo_build(ppo);
}

static void ppo_buff_free(void *ptr)
{
    if (ptr) { ldg_mem_dealloc(ptr); }
}

static uint32_t ppo_opponent_teardown(ldg_math_stat_ppo_t *ppo)
{
    ldg_math_stat_ppo_opponent_t *opp = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    while (idx < LDG_MATH_STAT_PPO_OPPONENT_MAX)
    {
        opp = &ppo->opponent[idx];

        ppo_buff_free(opp->state);
        opp->state = 0x0;

        ret = ldg_math_stat_model_shutdown(&opp->model);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (opp->graph.is_init == LDG_TRUTH_TRUE)
        {
            ret = ldg_math_stat_autograd_shutdown(&opp->graph);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        idx++;
    }

    ppo_buff_free(ppo->snapshot_tag);
    ppo_buff_free(ppo->snapshot_quality);
    ppo_buff_free(ppo->snapshot);
    ppo->snapshot_tag = 0x0;
    ppo->snapshot_quality = 0x0;
    ppo->snapshot = 0x0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_shutdown(ldg_math_stat_ppo_t *ppo)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_opponent_teardown(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo->is_init != LDG_TRUTH_TRUE)
    {
        ret = ldg_math_stat_model_shutdown(&ppo->critic);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_stat_model_shutdown(&ppo->actor);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        return ldg_mem_zero(ppo, (uint64_t)sizeof(ldg_math_stat_ppo_t));
    }

    ppo_buff_free(ppo->demo_action_mask);
    ppo_buff_free(ppo->sil_action_mask);
    ppo_buff_free(ppo->batch_action_mask);
    ppo_buff_free(ppo->action_mask_buff);
    ppo_buff_free(ppo->act_buff);
    ppo_buff_free(ppo->demo_len);
    ppo_buff_free(ppo->demo_weight);
    ppo_buff_free(ppo->demo_action);
    ppo_buff_free(ppo->demo_critic_obs);
    ppo_buff_free(ppo->demo_obs);
    ppo_buff_free(ppo->sil_state);
    ppo_buff_free(ppo->sil_len);
    ppo_buff_free(ppo->sil_step_buff);
    ppo_buff_free(ppo->sil_prio);
    ppo_buff_free(ppo->sil_tgt);
    ppo_buff_free(ppo->sil_action);
    ppo_buff_free(ppo->sil_critic_obs);
    ppo_buff_free(ppo->sil_obs);
    ppo_buff_free(ppo->state_pend);
    ppo_buff_free(ppo->state_curr);
    ppo_buff_free(ppo->state_buff);
    ppo_buff_free(ppo->batch_buff);
    ppo_buff_free(ppo->terminal_buff);
    ppo_buff_free(ppo->body_seg);
    ppo_buff_free(ppo->body_last);
    ppo_buff_free(ppo->body_buff);
    ppo_buff_free(ppo->seg_step_buff);
    ppo_buff_free(ppo->seg_id_buff);
    ppo_buff_free(ppo->seg_buff);
    ppo_buff_free(ppo->idx_buff);
    ppo_buff_free(ppo->body_next);
    ppo_buff_free(ppo->body_carry);
    ppo_buff_free(ppo->bootstrap_buff);
    ppo_buff_free(ppo->tgt_buff);
    ppo_buff_free(ppo->adv_buff);
    ppo_buff_free(ppo->reward_buff);
    ppo_buff_free(ppo->val_buff);
    ppo_buff_free(ppo->logp_buff);
    ppo_buff_free(ppo->action_buff);
    ppo_buff_free(ppo->critic_obs_buff);
    ppo_buff_free(ppo->obs_buff);
    ppo_buff_free(ppo->adam_frozen);
    ppo_buff_free(ppo->adam_var);
    ppo_buff_free(ppo->adam_mean);

    ret = ldg_math_stat_model_shutdown(&ppo->critic);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_shutdown(&ppo->actor);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_shutdown(&ppo->graph);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(ppo, (uint64_t)sizeof(ldg_math_stat_ppo_t));
}

static uint32_t ppo_body_ck(const ldg_math_stat_ppo_t *ppo, uint32_t body)
{
    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(body >= ppo->conf.body_cap)) { return LDG_ERR_BOUNDS; }

    return LDG_ERR_AOK;
}

static void ppo_float_to_double_cpy(const double *src, double *dst, uintptr_t cunt)
{
    const double *end = src + cunt;

    while (src < end)
    {
        *dst = (double)*src;
        src++;
        dst++;
    }
}

static void ppo_double_to_float_cpy(const double *src, double *dst, uintptr_t cunt)
{
    const double *end = src + cunt;

    while (src < end)
    {
        *dst = (double)*src;
        src++;
        dst++;
    }
}

uint32_t ldg_math_stat_ppo_state_rst(ldg_math_stat_ppo_t *ppo, uint32_t body)
{
    uint64_t state_size = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_body_ck(ppo, body);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_recur_is(ppo) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    state_size = (uint64_t)ppo_state_len_get(ppo) * (uint64_t)sizeof(double);

    ret = ldg_mem_zero(ppo_state_body_get(ppo, ppo->state_curr, body), state_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(ppo_state_body_get(ppo, ppo->state_pend, body), state_size);
}

uint32_t ldg_math_stat_ppo_state_get(const ldg_math_stat_ppo_t *ppo, uint32_t body, double *dst, uint64_t dst_cap, uint64_t *out_cunt)
{
    uintptr_t state_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = ppo_body_ck(ppo, body);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ppo_recur_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    state_len = ppo_state_len_get(ppo);

    if (LDG_UNLIKELY(dst_cap < (uint64_t)state_len)) { return LDG_ERR_BOUNDS; }

    ppo_float_to_double_cpy(ppo_state_body_get(ppo, ppo->state_curr, body), dst, state_len);

    *out_cunt = (uint64_t)state_len;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_state_set(ldg_math_stat_ppo_t *ppo, uint32_t body, const double *src, uint64_t cunt)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_body_ck(ppo, body);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ppo_recur_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(cunt != (uint64_t)ppo_state_len_get(ppo))) { return LDG_ERR_FUNC_ARG_INVALID; }

    ppo_double_to_float_cpy(src, ppo_state_body_get(ppo, ppo->state_curr, body), ppo_state_len_get(ppo));

    return LDG_ERR_AOK;
}

static uint32_t ppo_rollout_rst(ldg_math_stat_ppo_t *ppo)
{
    ppo->step_cunt = 0;

    return ldg_mem_fill(ppo->body_last, (uint8_t)LDG_BYTE_MASK, (uint64_t)ppo->conf.body_cap * (uint64_t)sizeof(uint32_t));
}

uint32_t ldg_math_stat_ppo_rst(ldg_math_stat_ppo_t *ppo)
{
    uint32_t body = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ppo_rollout_rst(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (body < ppo->conf.body_cap)
    {
        ret = ldg_math_stat_ppo_state_rst(ppo, body);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        body++;
    }

    return ldg_math_stat_autograd_tape_rst(&ppo->graph);
}

uint32_t ldg_math_stat_ppo_reinit(ldg_math_stat_ppo_t *ppo)
{
    uint32_t log_std_idx = UINT32_MAX;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    while (idx < ppo->actor.param_cunt)
    {
        if (ppo->actor.param[idx].node == ppo->log_std) { log_std_idx = idx; }

        idx++;
    }

    ret = ldg_math_stat_autograd_rst(&ppo->graph);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_param_redraw(&ppo->actor);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_param_redraw(&ppo->critic);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (log_std_idx != UINT32_MAX)
    {
        ret = ldg_math_stat_model_param_node_get(&ppo->actor, log_std_idx, &ppo->log_std);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_math_stat_autograd_param_mark_set(&ppo->graph);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ppo->graph.elem_mark != ppo->param_cunt)) { return LDG_ERR_INVALID; }

    ret = ldg_mem_zero(ppo->adam_mean, (uint64_t)ppo->param_cunt * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(ppo->adam_var, (uint64_t)ppo->param_cunt * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo->adam_step = 0;
    ppo->norm_mean = PPO_ZERO_SEED;
    ppo->norm_sq = PPO_ZERO_SEED;
    ppo->norm_cunt = 0;
    ppo->sil_cunt = 0;
    ppo->demo_cunt = 0;
    ppo->demo_hd = 0;
    ppo->demo_open = UINT32_MAX;
    ppo->demo_open_len = 0;
    ppo->act_is = LDG_TRUTH_FALSE;
    ppo->snapshot_cunt = 0;
    ppo->snapshot_seen = 0;

    idx = 0;
    while (idx < ppo->conf.opponent_cap)
    {
        ppo->opponent[idx].ld_is = LDG_TRUTH_FALSE;
        idx++;
    }

    return ldg_math_stat_ppo_rst(ppo);
}

uint32_t ldg_math_stat_ppo_step_cunt_get(const ldg_math_stat_ppo_t *ppo)
{
    if (LDG_UNLIKELY(!ppo)) { return UINT32_MAX; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return UINT32_MAX; }

    return ppo->step_cunt;
}

uint32_t ldg_math_stat_ppo_epoch_set(ldg_math_stat_ppo_t *ppo, uint32_t epoch_cunt)
{
    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(epoch_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ppo->conf.epoch_cunt = epoch_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_entropy_set(ldg_math_stat_ppo_t *ppo, double entropy_coef)
{
    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(entropy_coef < PPO_ZERO_SEED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ppo->conf.entropy_coef = entropy_coef;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_norm_get(const ldg_math_stat_ppo_t *ppo, double *out_mean, double *out_sq, uint64_t *out_cunt)
{
    if (LDG_UNLIKELY(!ppo || !out_mean || !out_sq || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_mean = 0.0;
    *out_sq = 0.0;
    *out_cunt = 0;

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo_norm_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    *out_mean = ppo->norm_mean;
    *out_sq = ppo->norm_sq;
    *out_cunt = ppo->norm_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_norm_set(ldg_math_stat_ppo_t *ppo, double mean, double sq, uint64_t cunt)
{
    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo_norm_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(sq < (mean * mean))) { return LDG_ERR_FUNC_ARG_INVALID; }

    ppo->norm_mean = mean;
    ppo->norm_sq = sq;
    ppo->norm_cunt = cunt;

    return LDG_ERR_AOK;
}

static void ppo_norm_stat_get(const ldg_math_stat_ppo_t *ppo, double *out_mean, double *out_sigma)
{
    double corr = 0.0;
    double mean = 0.0;
    double var = 0.0;

    *out_mean = PPO_ZERO_SEED;
    *out_sigma = PPO_NORM_SIGMA_UNIT;

    if (ppo_norm_is(ppo) != LDG_TRUTH_TRUE || ppo->norm_cunt == 0) { return; }

    corr = PPO_UNIT_POS - ldg_math_pwr_get(ppo->conf.norm_decay, (double)ppo->norm_cunt);
    mean = ppo->norm_mean / corr;
    var = (ppo->norm_sq / corr) - (mean * mean);
    if (var < PPO_NORM_EPS) { var = PPO_NORM_EPS; }

    *out_mean = (double)mean;
    *out_sigma = (double)ldg_math_sqrt_get(var);
}

static double ppo_tgt_norm_apply(const ldg_math_stat_ppo_t *ppo, double val)
{
    double mean = 0.0;
    double sigma = 0.0;

    if (ppo_norm_is(ppo) != LDG_TRUTH_TRUE) { return val; }

    ppo_norm_stat_get(ppo, &mean, &sigma);

    return (val - mean) / sigma;
}

static double ppo_val_denorm_apply(const ldg_math_stat_ppo_t *ppo, double val)
{
    double mean = 0.0;
    double sigma = 0.0;

    if (ppo_norm_is(ppo) != LDG_TRUTH_TRUE) { return val; }

    ppo_norm_stat_get(ppo, &mean, &sigma);

    return (val * sigma) + mean;
}

static void ppo_norm_update(ldg_math_stat_ppo_t *ppo)
{
    double decay = ppo->conf.norm_decay;
    double rest = PPO_UNIT_POS - decay;
    double sample = 0.0;
    uint32_t idx = 0;

    while (idx < ppo->step_cunt)
    {
        sample = ppo->tgt_buff[idx];
        ppo->norm_mean = (decay * ppo->norm_mean) + (rest * sample);
        ppo->norm_sq = (decay * ppo->norm_sq) + (rest * sample * sample);
        ppo->norm_cunt++;
        idx++;
    }
}

static void ppo_stage(const double *src, double *dst, uint32_t cunt)
{
    uint32_t idx = 0;

    while (idx < cunt)
    {
        dst[idx] = (double)src[idx];
        idx++;
    }
}

static uint32_t ppo_net_infer(ldg_math_stat_ppo_t *ppo, uint32_t net, uint32_t body, const double *src, uint8_t advance, uint32_t *out)
{
    ldg_math_stat_model_t *model = ppo_model_get(ppo, net);
    double *state = 0x0;
    uint32_t ret = 0;

    if (model->state_len > 0)
    {
        state = ppo_state_slot_get(ppo, ppo_state_body_get(ppo, ppo->state_curr, body), net);

        ret = ldg_math_stat_model_state_push(model, state, PPO_SCALAR_ROW);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    model->live_is = advance;
    ret = ldg_math_stat_model_forward(model, src, PPO_SCALAR_ROW, out);
    model->live_is = LDG_TRUTH_FALSE;
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (model->state_len == 0 || advance != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    return ldg_math_stat_model_state_get(model, state, PPO_SCALAR_ROW, (uintptr_t)model->state_len);
}

uint32_t ldg_math_stat_ppo_val_get(ldg_math_stat_ppo_t *ppo, uint32_t body, const double *critic_obs, double *out)
{
    double staged = 0.0;
    uint32_t val_node = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !critic_obs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_body_ck(ppo, body);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_tape_rst(&ppo->graph);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo_stage(critic_obs, ppo_lane_get(ppo, PPO_LANE_CRITIC_OBS), ppo_critic_dim_get(ppo));

    ret = ppo_net_infer(ppo, PPO_NET_CRITIC, body, ppo_lane_get(ppo, PPO_LANE_CRITIC_OBS), LDG_TRUTH_FALSE, &val_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_scalar_get(&ppo->graph, val_node, &staged);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = (double)ppo_val_denorm_apply(ppo, staged);

    return ldg_math_stat_autograd_tape_rst(&ppo->graph);
}

uint32_t ldg_math_stat_ppo_param_cunt_get(const ldg_math_stat_ppo_t *ppo, uint64_t *out)
{
    if (LDG_UNLIKELY(!ppo || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = (uint64_t)ppo->param_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_params_get(const ldg_math_stat_ppo_t *ppo, double *dst, uint64_t dst_cap, uint64_t *out_cunt)
{
    if (LDG_UNLIKELY(!ppo || !dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(dst_cap < (uint64_t)ppo->param_cunt)) { return LDG_ERR_BOUNDS; }

    ppo_float_to_double_cpy(ppo->graph.val, dst, ppo->param_cunt);

    *out_cunt = (uint64_t)ppo->param_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_params_set(ldg_math_stat_ppo_t *ppo, const double *src, uint64_t cunt)
{
    if (LDG_UNLIKELY(!ppo || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(cunt != (uint64_t)ppo->param_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ppo_double_to_float_cpy(src, ppo->graph.val, ppo->param_cunt);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_adam_get(const ldg_math_stat_ppo_t *ppo, double *dst_mean, double *dst_var, uint64_t dst_cap, uint64_t *out_cunt, uint32_t *out_step)
{
    if (LDG_UNLIKELY(!ppo || !dst_mean || !dst_var || !out_cunt || !out_step)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    *out_step = 0;

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(dst_cap < (uint64_t)ppo->param_cunt)) { return LDG_ERR_BOUNDS; }

    ppo_float_to_double_cpy(ppo->adam_mean, dst_mean, ppo->param_cunt);
    ppo_float_to_double_cpy(ppo->adam_var, dst_var, ppo->param_cunt);

    *out_cunt = (uint64_t)ppo->param_cunt;
    *out_step = ppo->adam_step;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_adam_set(ldg_math_stat_ppo_t *ppo, const double *src_mean, const double *src_var, uint64_t cunt, uint32_t step)
{
    if (LDG_UNLIKELY(!ppo || !src_mean || !src_var)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(cunt != (uint64_t)ppo->param_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ppo_double_to_float_cpy(src_mean, ppo->adam_mean, ppo->param_cunt);
    ppo_double_to_float_cpy(src_var, ppo->adam_var, ppo->param_cunt);

    ppo->adam_step = step;

    return LDG_ERR_AOK;
}

static uint32_t ppo_discrete_draw(ldg_math_stat_ppo_t *ppo, const ldg_math_stat_autograd_t *graph, uint32_t logp_all, double *action_out, double *logp_out)
{
    double *buff = ppo_lane_get(ppo, PPO_LANE_SAMPLE);
    double unit = 0.0;
    double cum = 0.0;
    double prob = 0.0;
    uint32_t last = ppo->conf.action_dim - PPO_SCALAR_ROW;
    uint32_t idx = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_val_get(graph, logp_all, buff, (uintptr_t)ppo_sample_len_get(ppo));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_rng_uniform_sample(ppo->rng_cb, ppo->rng_ctx, &unit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (idx < ppo->conf.action_dim)
    {
        prob = ldg_math_exp_get(buff[idx]);
        if (prob > PPO_ZERO_SEED) { last = idx; }

        cum = cum + prob;
        if (prob > PPO_ZERO_SEED && unit < cum)
        {
            *action_out = (double)idx;
            *logp_out = buff[idx];
            return LDG_ERR_AOK;
        }

        idx++;
    }

    *action_out = (double)last;
    *logp_out = buff[last];

    return LDG_ERR_AOK;
}

static uint32_t ppo_cont_draw(ldg_math_stat_ppo_t *ppo, const ldg_math_stat_autograd_t *graph, uint32_t mu, uint32_t log_std_node, double *action_out, double *logp_out)
{
    double *buff = ppo_lane_get(ppo, PPO_LANE_SAMPLE);
    const double *log_std = graph->val + graph->node[log_std_node].val_off;
    double normal = 0.0;
    double scale = 0.0;
    double norm = 0.0;
    double quad = 0.0;
    double spread = 0.0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_val_get(graph, mu, buff, (uintptr_t)ppo_sample_len_get(ppo));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (idx < ppo->conf.action_dim)
    {
        ret = ldg_math_stat_rng_normal_sample(ppo->rng_cb, ppo->rng_ctx, &normal);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        scale = ldg_math_exp_get(log_std[idx]);
        action_out[idx] = (double)buff[idx] + ((double)scale * normal);
        norm = (double)(action_out[idx] - (double)buff[idx]) / scale;
        quad = quad + (norm * norm);
        spread = spread + log_std[idx];
        idx++;
    }

    *logp_out = (PPO_GAUSS_QUAD_COEF * quad) - spread + (PPO_GAUSS_QUAD_COEF * (double)ppo->conf.action_dim * PPO_LOG_TWO_PI);

    return LDG_ERR_AOK;
}

static uint32_t ppo_snapshot_ck(const ldg_math_stat_ppo_t *ppo)
{
    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo->conf.snapshot_cap == 0)) { return LDG_ERR_UNSUPPORTED; }

    return LDG_ERR_AOK;
}

static uintptr_t ppo_snapshot_len_get(const ldg_math_stat_ppo_t *ppo)
{
    return ppo->actor.param_elem_cunt;
}

static double* ppo_snapshot_slot_get(const ldg_math_stat_ppo_t *ppo, uint32_t slot)
{
    return ppo->snapshot + ((uintptr_t)slot * ppo_snapshot_len_get(ppo));
}

static const double* ppo_actor_param_get(const ldg_math_stat_ppo_t *ppo)
{
    return ppo->graph.val + ppo->actor.param_off;
}

static double ppo_snapshot_quality_max_get(const ldg_math_stat_ppo_t *ppo)
{
    double best = PPO_QUALITY_SEED;
    uint32_t idx = 0;

    while (idx < ppo->snapshot_cunt)
    {
        if (idx == 0 || ppo->snapshot_quality[idx] > best) { best = ppo->snapshot_quality[idx]; }

        idx++;
    }

    return best;
}

static double ppo_snapshot_quality_prob_get(const ldg_math_stat_ppo_t *ppo, uint32_t slot)
{
    double best = ppo_snapshot_quality_max_get(ppo);
    double total = PPO_QUALITY_SEED;
    uint32_t idx = 0;

    while (idx < ppo->snapshot_cunt)
    {
        total = total + ldg_math_exp_get(ppo->snapshot_quality[idx] - best);
        idx++;
    }

    return ldg_math_exp_get(ppo->snapshot_quality[slot] - best) / total;
}

static uint32_t ppo_snapshot_quality_draw(ldg_math_stat_ppo_t *ppo, uint32_t *out_slot)
{
    double best = ppo_snapshot_quality_max_get(ppo);
    double total = PPO_QUALITY_SEED;
    double cum = PPO_QUALITY_SEED;
    double unit = 0.0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    while (idx < ppo->snapshot_cunt)
    {
        total = total + ldg_math_exp_get(ppo->snapshot_quality[idx] - best);
        idx++;
    }

    ret = ldg_math_stat_rng_uniform_sample(ppo->rng_cb, ppo->rng_ctx, &unit);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_slot = ppo->snapshot_cunt - PPO_SCALAR_ROW;

    idx = 0;
    while (idx < ppo->snapshot_cunt)
    {
        cum = cum + (ldg_math_exp_get(ppo->snapshot_quality[idx] - best) / total);
        if ((double)unit < cum)
        {
            *out_slot = idx;
            return LDG_ERR_AOK;
        }

        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_snapshot_push(ldg_math_stat_ppo_t *ppo, uint32_t *out_slot)
{
    double quality = PPO_QUALITY_SEED;
    uint64_t pick = 0;
    uint32_t slot = UINT32_MAX;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !out_slot)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_slot = UINT32_MAX;

    ret = ppo_snapshot_ck(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    quality = ppo_snapshot_quality_max_get(ppo);

    if (ppo->snapshot_cunt < ppo->conf.snapshot_cap)
    {
        slot = ppo->snapshot_cunt;
        ppo->snapshot_cunt++;
    }
    else
    {
        pick = ppo->rng_cb(ppo->rng_ctx) % (ppo->snapshot_seen + (uint64_t)PPO_RESERVOIR_SELF);
        if (pick < (uint64_t)ppo->conf.snapshot_cap) { slot = (uint32_t)pick; }
    }

    if (slot != UINT32_MAX)
    {
        ret = ldg_mem_cpy(ppo_snapshot_slot_get(ppo, slot), ppo_actor_param_get(ppo), (uint64_t)ppo_snapshot_len_get(ppo) * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ppo->snapshot_tag[slot] = ppo->snapshot_seen;
        ppo->snapshot_quality[slot] = quality;
    }

    ppo->snapshot_seen++;
    *out_slot = slot;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_snapshot_quality_get(const ldg_math_stat_ppo_t *ppo, uint32_t slot, double *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    ret = ppo_snapshot_ck(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(slot >= ppo->snapshot_cunt)) { return LDG_ERR_BOUNDS; }

    *out = (double)ppo->snapshot_quality[slot];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_snapshot_cunt_get(const ldg_math_stat_ppo_t *ppo, uint64_t *out_cunt, uint64_t *out_len)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !out_cunt || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    *out_len = 0;

    ret = ppo_snapshot_ck(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_cunt = ppo->snapshot_cunt;
    *out_len = (uint64_t)PPO_SNAPSHOT_ENTRY_HDR_CUNT + (uint64_t)ppo_snapshot_len_get(ppo);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_snapshot_tag_get(const ldg_math_stat_ppo_t *ppo, uint32_t slot, uint64_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = ppo_snapshot_ck(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(slot >= ppo->snapshot_cunt)) { return LDG_ERR_BOUNDS; }

    *out = ppo->snapshot_tag[slot];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_snapshot_get(const ldg_math_stat_ppo_t *ppo, double *dst, uint64_t dst_cap, uint64_t *out_cunt)
{
    uint64_t entry_len = 0;
    uint64_t total = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = ppo_snapshot_ck(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    entry_len = (uint64_t)PPO_SNAPSHOT_ENTRY_HDR_CUNT + (uint64_t)ppo_snapshot_len_get(ppo);
    total = (uint64_t)PPO_SNAPSHOT_HDR_CUNT + ((uint64_t)ppo->snapshot_cunt * entry_len);

    if (LDG_UNLIKELY(dst_cap < total)) { return LDG_ERR_BOUNDS; }

    dst[PPO_SNAPSHOT_HDR_SEEN] = (double)ppo->snapshot_seen;
    dst[PPO_SNAPSHOT_HDR_FILL] = (double)ppo->snapshot_cunt;
    dst = dst + PPO_SNAPSHOT_HDR_CUNT;

    while (slot < ppo->snapshot_cunt)
    {
        dst[PPO_SNAPSHOT_ENTRY_TAG] = (double)ppo->snapshot_tag[slot];
        dst[PPO_SNAPSHOT_ENTRY_QUALITY] = (double)ppo->snapshot_quality[slot];
        ppo_float_to_double_cpy(ppo_snapshot_slot_get(ppo, slot), dst + PPO_SNAPSHOT_ENTRY_HDR_CUNT, ppo_snapshot_len_get(ppo));
        dst = dst + entry_len;
        slot++;
    }

    *out_cunt = total;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_snapshot_set(ldg_math_stat_ppo_t *ppo, const double *src, uint64_t cunt)
{
    uint64_t entry_len = 0;
    uint64_t seen = 0;
    uint64_t fill = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_snapshot_ck(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    entry_len = (uint64_t)PPO_SNAPSHOT_ENTRY_HDR_CUNT + (uint64_t)ppo_snapshot_len_get(ppo);

    if (LDG_UNLIKELY(cunt < (uint64_t)PPO_SNAPSHOT_HDR_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY((cunt - (uint64_t)PPO_SNAPSHOT_HDR_CUNT) % entry_len != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(src[PPO_SNAPSHOT_HDR_SEEN] < PPO_ZERO_SEED || src[PPO_SNAPSHOT_HDR_FILL] < PPO_ZERO_SEED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    seen = (uint64_t)src[PPO_SNAPSHOT_HDR_SEEN];
    fill = (uint64_t)src[PPO_SNAPSHOT_HDR_FILL];

    if (LDG_UNLIKELY(fill != (cunt - (uint64_t)PPO_SNAPSHOT_HDR_CUNT) / entry_len)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(fill > (uint64_t)ppo->conf.snapshot_cap || fill > seen)) { return LDG_ERR_FUNC_ARG_INVALID; }

    src = src + PPO_SNAPSHOT_HDR_CUNT;

    while (slot < fill)
    {
        if (LDG_UNLIKELY(src[PPO_SNAPSHOT_ENTRY_TAG] < PPO_ZERO_SEED)) { return LDG_ERR_FUNC_ARG_INVALID; }

        ppo->snapshot_tag[slot] = (uint64_t)src[PPO_SNAPSHOT_ENTRY_TAG];
        ppo->snapshot_quality[slot] = (double)src[PPO_SNAPSHOT_ENTRY_QUALITY];
        ppo_double_to_float_cpy(src + PPO_SNAPSHOT_ENTRY_HDR_CUNT, ppo_snapshot_slot_get(ppo, slot), ppo_snapshot_len_get(ppo));
        src = src + entry_len;
        slot++;
    }

    ppo->snapshot_seen = seen;
    ppo->snapshot_cunt = (uint32_t)fill;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_snapshot_rst(ldg_math_stat_ppo_t *ppo)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_snapshot_ck(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo->snapshot_seen = 0;
    ppo->snapshot_cunt = 0;

    return LDG_ERR_AOK;
}

static uint32_t ppo_opponent_ck(const ldg_math_stat_ppo_t *ppo, uint32_t opponent)
{
    uint32_t ret = 0;

    ret = ppo_snapshot_ck(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(opponent >= ppo->conf.opponent_cap)) { return LDG_ERR_BOUNDS; }

    return LDG_ERR_AOK;
}

static double* ppo_opponent_state_get(const ldg_math_stat_ppo_opponent_t *opp, uint32_t body)
{
    return opp->state + ((uintptr_t)body * (uintptr_t)opp->model.state_len);
}

static uint32_t ppo_opponent_state_zero(ldg_math_stat_ppo_opponent_t *opp, uint32_t body_cap)
{
    if (opp->model.state_len == 0) { return LDG_ERR_AOK; }

    return ldg_mem_zero(opp->state, (uint64_t)body_cap * (uint64_t)opp->model.state_len * (uint64_t)sizeof(double));
}

uint32_t ldg_math_stat_ppo_opponent_ld(ldg_math_stat_ppo_t *ppo, uint32_t opponent, uint32_t slot)
{
    ldg_math_stat_ppo_opponent_t *opp = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_opponent_ck(ppo, opponent);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(slot >= ppo->snapshot_cunt)) { return LDG_ERR_BOUNDS; }

    opp = &ppo->opponent[opponent];

    ret = ldg_mem_cpy(opp->graph.val + opp->model.param_off, ppo_snapshot_slot_get(ppo, slot), (uint64_t)ppo_snapshot_len_get(ppo) * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_opponent_state_zero(opp, ppo->conf.opponent_body_cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    opp->snapshot = slot;
    opp->tag = ppo->snapshot_tag[slot];
    opp->ld_is = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_opponent_sel(ldg_math_stat_ppo_t *ppo, uint32_t opponent, uint32_t strat, uint32_t *out_slot)
{
    uint32_t slot = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !out_slot)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_slot = UINT32_MAX;

    ret = ppo_opponent_ck(ppo, opponent);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(strat >= LDG_MATH_STAT_PPO_OPPONENT_STRAT_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ppo->snapshot_cunt == 0)) { return LDG_ERR_EMPTY; }

    if (strat == LDG_MATH_STAT_PPO_OPPONENT_UNIFORM) { slot = (uint32_t)(ppo->rng_cb(ppo->rng_ctx) % (uint64_t)ppo->snapshot_cunt); }
    else if (strat == LDG_MATH_STAT_PPO_OPPONENT_QUALITY)
    {
        ret = ppo_snapshot_quality_draw(ppo, &slot);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else { while (idx < ppo->snapshot_cunt)
        {
            if (ppo->snapshot_tag[idx] > ppo->snapshot_tag[slot]) { slot = idx; }

            idx++;
        }
    }

    ret = ldg_math_stat_ppo_opponent_ld(ppo, opponent, slot);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_slot = slot;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_opponent_tag_get(const ldg_math_stat_ppo_t *ppo, uint32_t opponent, uint64_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = ppo_opponent_ck(ppo, opponent);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ppo->opponent[opponent].ld_is != LDG_TRUTH_TRUE)) { return LDG_ERR_EMPTY; }

    *out = ppo->opponent[opponent].tag;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_opponent_action_sel(ldg_math_stat_ppo_t *ppo, uint32_t opponent, uint32_t body, const double *obs, double *action_out, const uint8_t *action_mask)
{
    ldg_math_stat_ppo_opponent_t *opp = 0x0;
    double *state = 0x0;
    double logp = 0.0;
    uint32_t hd = 0;
    uint32_t logp_all = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !obs || !action_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_opponent_ck(ppo, opponent);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(body >= ppo->conf.opponent_body_cap)) { return LDG_ERR_BOUNDS; }

    opp = &ppo->opponent[opponent];

    if (LDG_UNLIKELY(opp->ld_is != LDG_TRUTH_TRUE)) { return LDG_ERR_EMPTY; }

    ret = ppo_mask_arg_ck(ppo, action_mask);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_tape_rst(&opp->graph);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo_stage(obs, ppo_lane_get(ppo, PPO_LANE_OBS), ppo->conf.obs_dim);

    if (opp->model.state_len > 0)
    {
        state = ppo_opponent_state_get(opp, body);

        ret = ldg_math_stat_model_state_push(&opp->model, state, PPO_OPPONENT_ROW);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_math_stat_model_forward(&opp->model, ppo_lane_get(ppo, PPO_LANE_OBS), PPO_OPPONENT_ROW, &hd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (state)
    {
        ret = ldg_math_stat_model_state_get(&opp->model, state, PPO_OPPONENT_ROW, (uintptr_t)opp->model.state_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    if (ppo->conf.action_kind == LDG_MATH_STAT_PPO_ACTION_DISCRETE)
    {
        ret = ldg_math_stat_autograd_log_softmax_push(&opp->graph, hd, action_mask, &logp_all);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ppo_discrete_draw(ppo, &opp->graph, logp_all, action_out, &logp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = ppo_cont_draw(ppo, &opp->graph, hd, opp->log_std, action_out, &logp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return ldg_math_stat_autograd_tape_rst(&opp->graph);
}

uint32_t ldg_math_stat_ppo_opponent_state_rst(ldg_math_stat_ppo_t *ppo, uint32_t opponent, uint32_t body)
{
    ldg_math_stat_ppo_opponent_t *opp = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_opponent_ck(ppo, opponent);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(body >= ppo->conf.opponent_body_cap)) { return LDG_ERR_BOUNDS; }

    opp = &ppo->opponent[opponent];

    if (opp->model.state_len == 0) { return LDG_ERR_AOK; }

    return ldg_mem_zero(ppo_opponent_state_get(opp, body), (uint64_t)opp->model.state_len * (uint64_t)sizeof(double));
}

uint32_t ldg_math_stat_ppo_opponent_res_report(ldg_math_stat_ppo_t *ppo, uint32_t opponent, uint8_t win_is)
{
    const ldg_math_stat_ppo_opponent_t *opp = 0x0;
    double prob = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_opponent_ck(ppo, opponent);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(win_is > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    opp = &ppo->opponent[opponent];

    if (LDG_UNLIKELY(opp->ld_is != LDG_TRUTH_TRUE)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(opp->snapshot >= ppo->snapshot_cunt || ppo->snapshot_tag[opp->snapshot] != opp->tag)) { return LDG_ERR_NOT_FOUND; }

    if (win_is != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    prob = ppo_snapshot_quality_prob_get(ppo, opp->snapshot);
    ppo->snapshot_quality[opp->snapshot] = ppo->snapshot_quality[opp->snapshot] - ((double)ppo->conf.opponent_lr / ((double)ppo->snapshot_cunt * prob));

    return LDG_ERR_AOK;
}

static uint32_t ppo_critic_arg_ck(const ldg_math_stat_ppo_t *ppo, const double *critic_obs)
{
    if (ppo_split_is(ppo) != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(!critic_obs)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_AOK;
}

static uint32_t ppo_act_node_st(ldg_math_stat_ppo_t *ppo, uint32_t node, double *dst, uintptr_t cunt)
{
    if (node == LDG_MATH_STAT_AUTOGRAD_NODE_NONE) { return LDG_ERR_AOK; }

    return ldg_math_stat_autograd_val_get(&ppo->graph, node, dst, cunt);
}

static uint32_t ppo_act_net_capture(ldg_math_stat_ppo_t *ppo, uint32_t net, uint32_t hd_node)
{
    const ldg_math_stat_model_t *model = ppo_model_view_get(ppo, net);
    const ldg_math_stat_layer_t *attn = ppo_attn_layer_get(ppo, net);
    double *dst = ppo_act_net_get(ppo, net);
    uint32_t node = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    while (idx < model->layer_cunt)
    {
        node = model->layer[idx].node_out;
        if (idx == model->layer_cunt - PPO_SCALAR_ROW && hd_node != LDG_MATH_STAT_AUTOGRAD_NODE_NONE) { node = hd_node; }

        ret = ppo_act_node_st(ppo, node, dst, (uintptr_t)model->layer[idx].out_dim);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        dst = dst + model->layer[idx].out_dim;
        idx++;
    }

    if (!attn) { return LDG_ERR_AOK; }

    return ppo_act_node_st(ppo, attn->node_aux, ppo_act_attn_get(ppo, net), ppo_act_attn_len_get(ppo, net));
}

uint32_t ldg_math_stat_ppo_action_sel(ldg_math_stat_ppo_t *ppo, uint32_t body, const double *obs, const double *critic_obs, double *action_out, double *logp_out, double *val_out, const uint8_t *action_mask)
{
    double staged = 0.0;
    uint32_t hd = 0;
    uint32_t logp_all = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;
    uint32_t val_node = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !obs || !action_out || !logp_out || !val_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_body_ck(ppo, body);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_critic_arg_ck(ppo, critic_obs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_mask_arg_ck(ppo, action_mask);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_tape_rst(&ppo->graph);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_recur_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ldg_mem_cpy(ppo_state_body_get(ppo, ppo->state_pend, body), ppo_state_body_get(ppo, ppo->state_curr, body), (uint64_t)ppo_state_len_get(ppo) * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ppo_stage(obs, ppo_lane_get(ppo, PPO_LANE_OBS), ppo->conf.obs_dim);
    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE) { ppo_stage(critic_obs, ppo_lane_get(ppo, PPO_LANE_CRITIC_OBS), ppo->conf.critic_obs_dim); }

    ret = ppo_net_infer(ppo, PPO_NET_ACTOR, body, ppo_lane_get(ppo, PPO_LANE_OBS), LDG_TRUTH_TRUE, &hd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo->conf.action_kind == LDG_MATH_STAT_PPO_ACTION_DISCRETE)
    {
        ret = ldg_math_stat_autograd_log_softmax_push(&ppo->graph, hd, action_mask, &logp_all);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ppo_discrete_draw(ppo, &ppo->graph, logp_all, action_out, logp_out);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else
    {
        ret = ppo_cont_draw(ppo, &ppo->graph, hd, ppo->log_std, action_out, logp_out);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ppo_net_infer(ppo, PPO_NET_CRITIC, body, ppo_lane_get(ppo, PPO_LANE_CRITIC_OBS), LDG_TRUTH_TRUE, &val_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_scalar_get(&ppo->graph, val_node, &staged);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *val_out = (double)ppo_val_denorm_apply(ppo, staged);

    if (ppo_act_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ppo_act_net_capture(ppo, PPO_NET_ACTOR, logp_all);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ppo_act_net_capture(ppo, PPO_NET_CRITIC, LDG_MATH_STAT_AUTOGRAD_NODE_NONE);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ppo->act_body = body;
        ppo->act_is = LDG_TRUTH_TRUE;
    }

    return ldg_math_stat_autograd_tape_rst(&ppo->graph);
}

static uint32_t ppo_act_ck(const ldg_math_stat_ppo_t *ppo, uint32_t body)
{
    uint32_t ret = 0;

    ret = ppo_body_ck(ppo, body);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ppo_act_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(ppo->act_is != LDG_TRUTH_TRUE)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(ppo->act_body != body)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_act_layer_cunt_get(const ldg_math_stat_ppo_t *ppo, uint32_t *out_actor, uint32_t *out_critic)
{
    if (LDG_UNLIKELY(!ppo || !out_actor || !out_critic)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_actor = 0;
    *out_critic = 0;

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out_actor = ppo->actor.layer_cunt;
    *out_critic = ppo->critic.layer_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_act_get(const ldg_math_stat_ppo_t *ppo, uint32_t body, uint32_t layer, double *dst, uint64_t dst_cap, uint64_t *out_cunt)
{
    const ldg_math_stat_model_t *model = 0x0;
    const double *src = 0x0;
    uintptr_t width = 0;
    uint32_t net = PPO_NET_ACTOR;
    uint32_t local = layer;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = ppo_act_ck(ppo, body);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(layer >= ppo->actor.layer_cunt + ppo->critic.layer_cunt)) { return LDG_ERR_BOUNDS; }

    if (layer >= ppo->actor.layer_cunt)
    {
        net = PPO_NET_CRITIC;
        local = layer - ppo->actor.layer_cunt;
    }

    model = ppo_model_view_get(ppo, net);
    src = ppo_act_net_get(ppo, net);

    while (idx < local)
    {
        src = src + model->layer[idx].out_dim;
        idx++;
    }

    width = model->layer[local].out_dim;

    if (LDG_UNLIKELY(dst_cap < (uint64_t)width)) { return LDG_ERR_BOUNDS; }

    ppo_float_to_double_cpy(src, dst, width);

    *out_cunt = (uint64_t)width;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_attn_get(const ldg_math_stat_ppo_t *ppo, uint32_t body, uint32_t net, double *dst, uint64_t dst_cap, uint64_t *out_cunt)
{
    uintptr_t width = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = ppo_act_ck(ppo, body);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(net >= PPO_NET_CUNT)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(!ppo_attn_layer_get(ppo, net))) { return LDG_ERR_UNSUPPORTED; }

    width = ppo_act_attn_len_get(ppo, net);

    if (LDG_UNLIKELY(dst_cap < (uint64_t)width)) { return LDG_ERR_BOUNDS; }

    ppo_float_to_double_cpy(ppo_act_attn_get(ppo, net), dst, width);

    *out_cunt = (uint64_t)width;

    return LDG_ERR_AOK;
}

static uint32_t ppo_reward_modulate(ldg_math_stat_ppo_t *ppo, uint32_t body, double reward)
{
    ldg_math_stat_model_t *model = 0x0;
    double *state = 0x0;
    uint32_t net = 0;
    uint32_t ret = 0;

    while (net < PPO_NET_CUNT)
    {
        model = ppo_model_get(ppo, net);

        if (ppo_model_neuron_is(model) == LDG_TRUTH_TRUE)
        {
            state = ppo_state_slot_get(ppo, ppo_state_body_get(ppo, ppo->state_curr, body), net);

            ret = ldg_math_stat_model_state_push(model, state, PPO_SCALAR_ROW);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = ldg_math_stat_model_reward_apply(model, reward, PPO_SCALAR_ROW);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        net++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_observe(ldg_math_stat_ppo_t *ppo, uint32_t body, const double *obs, const double *critic_obs, const double *action, double logp, double val, double reward, uint8_t terminal, const uint8_t *action_mask)
{
    uint64_t state_size = 0;
    uint32_t step = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !obs || !action)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_body_ck(ppo, body);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_critic_arg_ck(ppo, critic_obs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_mask_arg_ck(ppo, action_mask);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ppo->step_cunt >= ppo->conf.step_cap)) { return LDG_ERR_FULL; }

    step = ppo->step_cunt;

    ppo_stage(obs, ppo->obs_buff + ((uintptr_t)step * (uintptr_t)ppo->conf.obs_dim), ppo->conf.obs_dim);
    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE) { ppo_stage(critic_obs, ppo->critic_obs_buff + ((uintptr_t)step * (uintptr_t)ppo->conf.critic_obs_dim), ppo->conf.critic_obs_dim); }

    ppo_stage(action, ppo->action_buff + ((uintptr_t)step * (uintptr_t)ppo_action_width_get(ppo)), ppo_action_width_get(ppo));

    ret = ppo_mask_st(ppo, action_mask, ppo->action_mask_buff, (uintptr_t)step);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo->logp_buff[step] = (double)logp;
    ppo->val_buff[step] = (double)val;
    ppo->reward_buff[step] = (double)reward;
    ppo->terminal_buff[step] = terminal;
    ppo->bootstrap_buff[step] = PPO_ZERO_SEED;
    ppo->body_buff[step] = body;
    ppo->body_last[body] = step;

    ret = ppo_reward_modulate(ppo, body, (double)reward);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_recur_is(ppo) == LDG_TRUTH_TRUE)
    {
        state_size = (uint64_t)ppo_state_len_get(ppo) * (uint64_t)sizeof(double);

        ret = ldg_mem_cpy(ppo_state_step_get(ppo, step), ppo_state_body_get(ppo, ppo->state_pend, body), state_size);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (terminal == LDG_TRUTH_TRUE)
        {
            ret = ldg_mem_zero(ppo_state_body_get(ppo, ppo->state_curr, body), state_size);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    ppo->step_cunt++;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_segment_end(ldg_math_stat_ppo_t *ppo, uint32_t body, double bootstrap_val)
{
    uint32_t step = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_body_ck(ppo, body);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    step = ppo->body_last[body];

    if (LDG_UNLIKELY(step == UINT32_MAX)) { return LDG_ERR_EMPTY; }

    ppo->terminal_buff[step] = LDG_TRUTH_TRUE;
    ppo->bootstrap_buff[step] = (double)bootstrap_val;

    return LDG_ERR_AOK;
}

static void ppo_body_seed(ldg_math_stat_ppo_t *ppo, double last_val)
{
    uint32_t body = 0;

    while (body < ppo->conf.body_cap)
    {
        ppo->body_carry[body] = PPO_ZERO_SEED;
        ppo->body_next[body] = last_val;
        body++;
    }
}

static void ppo_gae_compute(ldg_math_stat_ppo_t *ppo, double last_val)
{
    double delta = 0.0;
    uint32_t body = 0;
    uint32_t idx = 0;

    ppo_body_seed(ppo, last_val);

    idx = ppo->step_cunt;
    while (idx > 0)
    {
        idx--;
        body = ppo->body_buff[idx];

        if (ppo->terminal_buff[idx] == LDG_TRUTH_TRUE)
        {
            ppo->body_carry[body] = PPO_ZERO_SEED;
            ppo->body_next[body] = ppo->bootstrap_buff[idx];
        }

        delta = ppo->reward_buff[idx] + (ppo->conf.gamma * ppo->body_next[body]) - ppo->val_buff[idx];
        ppo->body_carry[body] = delta + (ppo->conf.gamma * ppo->conf.lambda * ppo->body_carry[body]);

        ppo->adv_buff[idx] = ppo->body_carry[body];
        ppo->tgt_buff[idx] = ppo->body_carry[body] + ppo->val_buff[idx];
        ppo->body_next[body] = ppo->val_buff[idx];
    }
}

static void ppo_adv_normalize(ldg_math_stat_ppo_t *ppo)
{
    double cum = 0.0;
    double mean = 0.0;
    double var = 0.0;
    double spread = 0.0;
    uint32_t idx = 0;

    while (idx < ppo->step_cunt)
    {
        cum = cum + ppo->adv_buff[idx];
        idx++;
    }

    mean = cum / (double)ppo->step_cunt;

    idx = 0;
    while (idx < ppo->step_cunt)
    {
        var = var + ((ppo->adv_buff[idx] - mean) * (ppo->adv_buff[idx] - mean));
        idx++;
    }

    spread = ldg_math_sqrt_get(var / (double)ppo->step_cunt) + PPO_ADV_EPS;

    idx = 0;
    while (idx < ppo->step_cunt)
    {
        ppo->adv_buff[idx] = (ppo->adv_buff[idx] - mean) / spread;
        idx++;
    }
}

static void ppo_idx_shuffle(ldg_math_stat_ppo_t *ppo, uint32_t cunt)
{
    uint64_t pick = 0;
    uint32_t swap = 0;
    uint32_t idx = 0;

    while (idx < cunt)
    {
        ppo->idx_buff[idx] = idx;
        idx++;
    }

    idx = cunt;
    while (idx > 0)
    {
        idx--;

        pick = ppo->rng_cb(ppo->rng_ctx) % ((uint64_t)idx + (uint64_t)PPO_SCALAR_ROW);

        swap = ppo->idx_buff[idx];
        ppo->idx_buff[idx] = ppo->idx_buff[pick];
        ppo->idx_buff[pick] = swap;
    }
}

static double ppo_grad_norm_get(const ldg_math_stat_ppo_t *ppo)
{
    const double *ptr = ppo->graph.grad;
    const double *end = ppo->graph.grad + ppo->param_cunt;
    double cum = 0.0;

    while (ptr < end)
    {
        cum = cum + (*ptr * *ptr);
        ptr++;
    }

    return ldg_math_sqrt_get(cum);
}

static uint32_t ppo_frozen_refresh(ldg_math_stat_ppo_t *ppo, uint32_t *out_cunt)
{
    uint32_t actor_cunt = 0;
    uint32_t critic_cunt = 0;
    uint32_t ret = 0;

    *out_cunt = 0;

    ret = ldg_math_stat_model_frozen_mask_set(&ppo->actor, ppo->adam_frozen, ppo->param_cunt, &actor_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_frozen_mask_set(&ppo->critic, ppo->adam_frozen, ppo->param_cunt, &critic_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_cunt = actor_cunt + critic_cunt;

    return LDG_ERR_AOK;
}

static void ppo_adam_apply(ldg_math_stat_ppo_t *ppo, double scale, uint32_t frozen_cunt)
{
    double *val = ppo->graph.val;
    const double *grad = ppo->graph.grad;
    const uint8_t *frozen = ppo->adam_frozen;
    double *mean = ppo->adam_mean;
    double *var = ppo->adam_var;
    const double *end = ppo->graph.val + ppo->param_cunt;
    double bias_mean = 0.0;
    double bias_var = 0.0;
    double step = 0.0;
    double beta_mean = (double)ppo->conf.adam_beta_mean;
    double beta_var = (double)ppo->conf.adam_beta_var;
    double beta_mean_rest = (double)(PPO_UNIT_POS - ppo->conf.adam_beta_mean);
    double beta_var_rest = (double)(PPO_UNIT_POS - ppo->conf.adam_beta_var);
    double lr = (double)ppo->conf.lr;
    double eps = (double)ppo->conf.adam_eps;

    ppo->adam_step++;

    bias_mean = (double)(PPO_UNIT_POS - ldg_math_pwr_get(ppo->conf.adam_beta_mean, (double)ppo->adam_step));
    bias_var = (double)(PPO_UNIT_POS - ldg_math_pwr_get(ppo->conf.adam_beta_var, (double)ppo->adam_step));

    while (val < end)
    {
        if (frozen_cunt == 0 || *frozen != LDG_TRUTH_TRUE)
        {
            step = *grad * scale;

            *mean = (beta_mean * *mean) + (beta_mean_rest * step);
            *var = (beta_var * *var) + (beta_var_rest * step * step);

            *val = *val - (lr * (*mean / bias_mean) / (ldg_math_sqrt_get(*var / bias_var) + eps));
        }

        val++;
        grad++;
        frozen++;
        mean++;
        var++;
    }
}

static uint32_t ppo_row_gather(ldg_math_stat_ppo_t *ppo, uint32_t row, uint32_t pick)
{
    double *obs_lane = ppo_lane_get(ppo, PPO_LANE_OBS);
    double *critic_lane = ppo_lane_get(ppo, PPO_LANE_CRITIC_OBS);
    double *act_lane = ppo_lane_get(ppo, PPO_LANE_ACTION);
    double *logp_lane = ppo_lane_get(ppo, PPO_LANE_LOGP);
    double *adv_lane = ppo_lane_get(ppo, PPO_LANE_ADV);
    double *tgt_lane = ppo_lane_get(ppo, PPO_LANE_TGT);
    uintptr_t width = ppo_action_width_get(ppo);
    uintptr_t obs_dim = ppo->conf.obs_dim;
    uintptr_t critic_dim = ppo->conf.critic_obs_dim;
    uint32_t ret = 0;

    ret = ldg_mem_cpy(obs_lane + ((uintptr_t)row * obs_dim), ppo->obs_buff + ((uintptr_t)pick * obs_dim), (uint64_t)obs_dim * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ldg_mem_cpy(critic_lane + ((uintptr_t)row * critic_dim), ppo->critic_obs_buff + ((uintptr_t)pick * critic_dim), (uint64_t)critic_dim * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_mem_cpy(act_lane + ((uintptr_t)row * width), ppo->action_buff + ((uintptr_t)pick * width), (uint64_t)width * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_mask_row_cpy(ppo, ppo->batch_action_mask, (uintptr_t)row, ppo->action_mask_buff, (uintptr_t)pick);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    logp_lane[row] = ppo->logp_buff[pick];
    adv_lane[row] = ppo->adv_buff[pick];
    tgt_lane[row] = ppo_tgt_norm_apply(ppo, ppo->tgt_buff[pick]);

    return LDG_ERR_AOK;
}

static uint32_t ppo_row_blank(ldg_math_stat_ppo_t *ppo, uint32_t row)
{
    double *obs_lane = ppo_lane_get(ppo, PPO_LANE_OBS);
    double *critic_lane = ppo_lane_get(ppo, PPO_LANE_CRITIC_OBS);
    double *act_lane = ppo_lane_get(ppo, PPO_LANE_ACTION);
    double *logp_lane = ppo_lane_get(ppo, PPO_LANE_LOGP);
    double *adv_lane = ppo_lane_get(ppo, PPO_LANE_ADV);
    double *tgt_lane = ppo_lane_get(ppo, PPO_LANE_TGT);
    uintptr_t width = ppo_action_width_get(ppo);
    uintptr_t obs_dim = ppo->conf.obs_dim;
    uintptr_t critic_dim = ppo->conf.critic_obs_dim;
    uint32_t ret = 0;

    ret = ldg_mem_zero(obs_lane + ((uintptr_t)row * obs_dim), (uint64_t)obs_dim * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ldg_mem_zero(critic_lane + ((uintptr_t)row * critic_dim), (uint64_t)critic_dim * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_mem_zero(act_lane + ((uintptr_t)row * width), (uint64_t)width * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_mask_st(ppo, 0x0, ppo->batch_action_mask, (uintptr_t)row);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    logp_lane[row] = PPO_ZERO_SEED;
    adv_lane[row] = PPO_ZERO_SEED;
    tgt_lane[row] = PPO_ZERO_SEED;

    return LDG_ERR_AOK;
}

static uint32_t ppo_batch_gather(ldg_math_stat_ppo_t *ppo, uint32_t start, uint32_t cunt)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    while (idx < cunt)
    {
        ret = ppo_row_gather(ppo, idx, ppo->idx_buff[start + idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        idx++;
    }

    return LDG_ERR_AOK;
}

static uint32_t ppo_loss_push(ldg_math_stat_ppo_t *ppo, uint32_t obj, uint32_t val_loss, uint32_t entropy, uint32_t *out)
{
    uint32_t neg_obj = 0;
    uint32_t val_term = 0;
    uint32_t ent_term = 0;
    uint32_t sum_a = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, obj, PPO_UNIT_NEG, &neg_obj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, val_loss, ppo->conf.val_coef, &val_term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, entropy, PPO_UNIT_NEG * ppo->conf.entropy_coef, &ent_term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_add_push(&ppo->graph, neg_obj, val_term, &sum_a);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_add_push(&ppo->graph, sum_a, ent_term, out);
}

static uint32_t ppo_surr_push(ldg_math_stat_ppo_t *ppo, uint32_t logp, uint32_t old_logp_node, uint32_t adv_node, uint32_t *out_ratio, uint32_t *out_pick)
{
    uint32_t delta = 0;
    uint32_t surr_raw = 0;
    uint32_t surr_clip = 0;
    uint32_t clipped = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_sub_push(&ppo->graph, logp, old_logp_node, &delta);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_exp_push(&ppo->graph, delta, out_ratio);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mul_push(&ppo->graph, *out_ratio, adv_node, &surr_raw);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_clip_push(&ppo->graph, *out_ratio, ppo->conf.clip, &clipped);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mul_push(&ppo->graph, clipped, adv_node, &surr_clip);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_min_push(&ppo->graph, surr_raw, surr_clip, out_pick);
}

static uint32_t ppo_val_sq_push(ldg_math_stat_ppo_t *ppo, uint32_t val_node, uint32_t tgt_node, uint32_t *out)
{
    uint32_t val_diff = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_sub_push(&ppo->graph, val_node, tgt_node, &val_diff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_square_push(&ppo->graph, val_diff, out);
}

static uint32_t ppo_step_finish(ldg_math_stat_ppo_t *ppo, uint32_t total, uint32_t obj, uint32_t val_loss, uint32_t entropy, uint32_t ratio, uint32_t cunt, const double *mask, uint8_t diag_is, ldg_math_stat_ppo_report_t *acc)
{
    double *ratio_lane = ppo_lane_get(ppo, PPO_LANE_SAMPLE);
    double obj_val = 0.0;
    double val_loss_val = 0.0;
    double ent_val = 0.0;
    double norm = 0.0;
    double scale = PPO_UNIT_POS;
    double gap = 0.0;
    double kl_cum = 0.0;
    uint32_t clip_cunt = 0;
    uint32_t valid = 0;
    uint32_t idx = 0;
    uint32_t frozen_cunt = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_grad_zero(&ppo->graph);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_backward_run(&ppo->graph, total);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    norm = ppo_grad_norm_get(ppo);
    if (ppo->conf.grad_norm_max > PPO_ZERO_SEED && norm > ppo->conf.grad_norm_max) { scale = ppo->conf.grad_norm_max / norm; }

    ret = ppo_frozen_refresh(ppo, &frozen_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo_adam_apply(ppo, scale, frozen_cunt);

    ret = ldg_math_stat_autograd_scalar_get(&ppo->graph, obj, &obj_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_scalar_get(&ppo->graph, val_loss, &val_loss_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_scalar_get(&ppo->graph, entropy, &ent_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    acc->policy_loss = acc->policy_loss - obj_val;
    acc->val_loss = acc->val_loss + val_loss_val;
    acc->entropy = acc->entropy + ent_val;
    acc->grad_norm = acc->grad_norm + norm;

    if (diag_is != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ratio != LDG_MATH_STAT_AUTOGRAD_NODE_NONE)
    {
        ret = ldg_math_stat_autograd_val_get(&ppo->graph, ratio, ratio_lane, (uintptr_t)ppo_sample_len_get(ppo));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    while (idx < cunt)
    {
        if (!mask || mask[idx] == PPO_MASK_ON)
        {
            gap = ratio_lane[idx] - PPO_UNIT_POS;
            if (ldg_math_abs_get(gap) > ppo->conf.clip) { clip_cunt++; }

            kl_cum = kl_cum + (gap - ldg_math_log_get(ratio_lane[idx]));
            valid++;
        }

        idx++;
    }

    acc->clip_frac = acc->clip_frac + ((double)clip_cunt / (double)valid);
    acc->kl = acc->kl + (kl_cum / (double)valid);

    return LDG_ERR_AOK;
}

static uint32_t ppo_minibatch_run(ldg_math_stat_ppo_t *ppo, uint32_t cunt, ldg_math_stat_ppo_report_t *acc)
{
    uint32_t act_node = 0;
    uint32_t old_logp_node = 0;
    uint32_t adv_node = 0;
    uint32_t tgt_node = 0;
    uint32_t hd = 0;
    uint32_t logp = 0;
    uint32_t row_ent = 0;
    uint32_t entropy = 0;
    uint32_t ratio = 0;
    uint32_t pick = 0;
    uint32_t obj = 0;
    uint32_t val_node = 0;
    uint32_t val_sq = 0;
    uint32_t val_loss = 0;
    uint32_t total = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_tape_rst(&ppo->graph);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_leaf_push(ppo, ppo_lane_get(ppo, PPO_LANE_ACTION), cunt, ppo_action_width_get(ppo), &act_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_leaf_push(ppo, ppo_lane_get(ppo, PPO_LANE_LOGP), cunt, PPO_SCALAR_COL, &old_logp_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_leaf_push(ppo, ppo_lane_get(ppo, PPO_LANE_ADV), cunt, PPO_SCALAR_COL, &adv_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_leaf_push(ppo, ppo_lane_get(ppo, PPO_LANE_TGT), cunt, PPO_SCALAR_COL, &tgt_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_forward(&ppo->actor, ppo_lane_get(ppo, PPO_LANE_OBS), cunt, &hd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_logp_push(ppo, hd, act_node, ppo->batch_action_mask, &logp, &row_ent);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mean_push(&ppo->graph, row_ent, &entropy);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_surr_push(ppo, logp, old_logp_node, adv_node, &ratio, &pick);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mean_push(&ppo->graph, pick, &obj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_forward(&ppo->critic, ppo_lane_get(ppo, PPO_LANE_CRITIC_OBS), cunt, &val_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_val_sq_push(ppo, val_node, tgt_node, &val_sq);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mean_push(&ppo->graph, val_sq, &val_loss);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_loss_push(ppo, obj, val_loss, entropy, &total);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ppo_step_finish(ppo, total, obj, val_loss, entropy, ratio, cunt, 0x0, LDG_TRUTH_TRUE, acc);
}

static uint32_t ppo_seg_table_build(ldg_math_stat_ppo_t *ppo)
{
    uint32_t seg_cunt = 0;
    uint32_t body = 0;
    uint32_t seg = 0;
    uint32_t cum = 0;
    uint32_t len = 0;
    uint32_t idx = 0;

    body = 0;
    while (body < ppo->conf.body_cap)
    {
        ppo->body_seg[body] = UINT32_MAX;
        body++;
    }

    while (idx < ppo->step_cunt)
    {
        body = ppo->body_buff[idx];

        if (ppo->body_seg[body] == UINT32_MAX)
        {
            ppo->body_seg[body] = seg_cunt;
            ppo->seg_buff[seg_cunt] = 0;
            seg_cunt++;
        }

        seg = ppo->body_seg[body];
        ppo->seg_id_buff[idx] = seg;
        ppo->seg_buff[seg]++;

        if (ppo->terminal_buff[idx] == LDG_TRUTH_TRUE || ppo->seg_buff[seg] == ppo->conf.seg_cap) { ppo->body_seg[body] = UINT32_MAX; }

        idx++;
    }

    seg = 0;
    while (seg < seg_cunt)
    {
        len = ppo->seg_buff[seg];
        ppo->seg_buff[seg] = cum;
        ppo->idx_buff[seg] = cum;
        cum = cum + len;
        seg++;
    }

    ppo->seg_buff[seg_cunt] = ppo->step_cunt;

    idx = 0;
    while (idx < ppo->step_cunt)
    {
        seg = ppo->seg_id_buff[idx];
        ppo->seg_step_buff[ppo->idx_buff[seg]] = idx;
        ppo->idx_buff[seg]++;
        idx++;
    }

    return seg_cunt;
}

static uint32_t ppo_seg_step_get(const ldg_math_stat_ppo_t *ppo, uint32_t seg, uint32_t step)
{
    return ppo->seg_step_buff[ppo->seg_buff[seg] + step];
}

static uint32_t ppo_seg_len_get(const ldg_math_stat_ppo_t *ppo, uint32_t seg)
{
    return ppo->seg_buff[seg + PPO_SCALAR_ROW] - ppo->seg_buff[seg];
}

static double* ppo_state_lane_net_get(const ldg_math_stat_ppo_t *ppo, uint32_t net, uint32_t seg_in_batch)
{
    double *state_lane = ppo_lane_get(ppo, PPO_LANE_STATE);

    if (net == PPO_NET_CRITIC) { return state_lane + ((uintptr_t)seg_in_batch * (uintptr_t)ppo->actor.state_len); }

    return state_lane;
}

static uint32_t ppo_state_lane_fill(ldg_math_stat_ppo_t *ppo, uint32_t row, uint32_t seg_in_batch, const double *src)
{
    uintptr_t len = 0;
    uint32_t net = 0;
    uint32_t ret = 0;

    while (net < PPO_NET_CUNT)
    {
        len = ppo_model_state_len_get(ppo, net);

        if (len > 0)
        {
            ret = ldg_mem_cpy(ppo_state_lane_net_get(ppo, net, seg_in_batch) + ((uintptr_t)row * len), src, (uint64_t)len * (uint64_t)sizeof(double));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        src = src + len;
        net++;
    }

    return LDG_ERR_AOK;
}

static uint32_t ppo_seg_gather(ldg_math_stat_ppo_t *ppo, uint32_t seg_start_idx, uint32_t seg_in_batch)
{
    double *mask_lane = ppo_lane_get(ppo, PPO_LANE_MASK);
    uint32_t seg = 0;
    uint32_t seg_len = 0;
    uint32_t row = 0;
    uint32_t lane_row = 0;
    uint32_t step = 0;
    uint32_t ret = 0;

    while (row < seg_in_batch)
    {
        seg = ppo->idx_buff[seg_start_idx + row];
        seg_len = ppo_seg_len_get(ppo, seg);

        if (ppo_recur_is(ppo) == LDG_TRUTH_TRUE)
        {
            ret = ppo_state_lane_fill(ppo, row, seg_in_batch, ppo_state_step_get(ppo, ppo_seg_step_get(ppo, seg, PPO_ROW_FIRST)));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        step = 0;
        while (step < ppo->conf.seg_cap)
        {
            lane_row = (step * seg_in_batch) + row;

            if (step < seg_len)
            {
                ret = ppo_row_gather(ppo, lane_row, ppo_seg_step_get(ppo, seg, step));
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                mask_lane[lane_row] = PPO_MASK_ON;
            }
            else
            {
                ret = ppo_row_blank(ppo, lane_row);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                mask_lane[lane_row] = PPO_MASK_OFF;
            }

            step++;
        }

        row++;
    }

    return LDG_ERR_AOK;
}

static uint32_t ppo_masked_mean_push(ldg_math_stat_ppo_t *ppo, uint32_t node, uint32_t mask_node, uint32_t *out)
{
    uint32_t masked = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_mul_push(&ppo->graph, node, mask_node, &masked);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_mean_push(&ppo->graph, masked, out);
}

static uint32_t ppo_cum_push(ldg_math_stat_ppo_t *ppo, uint32_t cum, uint32_t term, uint32_t *out)
{
    if (cum == LDG_MATH_STAT_AUTOGRAD_NODE_NONE)
    {
        *out = term;
        return LDG_ERR_AOK;
    }

    return ldg_math_stat_autograd_add_push(&ppo->graph, cum, term, out);
}

static uint32_t ppo_ent_step_push(ldg_math_stat_ppo_t *ppo, uint32_t row_ent, uint32_t mask_node, uint32_t *ent_cum)
{
    uint32_t step_ent = row_ent;
    uint32_t ret = 0;

    if (ppo->conf.action_kind == LDG_MATH_STAT_PPO_ACTION_DISCRETE)
    {
        ret = ppo_masked_mean_push(ppo, row_ent, mask_node, &step_ent);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return ppo_cum_push(ppo, *ent_cum, step_ent, ent_cum);
}

static uint32_t ppo_ent_total_push(ldg_math_stat_ppo_t *ppo, uint32_t ent_cum, double row_scale, uint32_t *out)
{
    if (ppo->conf.action_kind == LDG_MATH_STAT_PPO_ACTION_DISCRETE) { return ldg_math_stat_autograd_scale_push(&ppo->graph, ent_cum, row_scale, out); }

    return ldg_math_stat_autograd_scale_push(&ppo->graph, ent_cum, PPO_UNIT_POS / (double)ppo->conf.seg_cap, out);
}

typedef struct ppo_step_leaf
{
    const double *obs;
    const double *critic_obs;
    const uint8_t *act_mask;
    uint32_t act_node;
    uint32_t logp_node;
    uint32_t adv_node;
    uint32_t tgt_node;
    uint32_t mask_node;
    uint32_t rows;
} ppo_step_leaf_t;

static uint32_t ppo_step_leaf_push(ldg_math_stat_ppo_t *ppo, uint32_t step, uint32_t seg_in_batch, ppo_step_leaf_t *out)
{
    uintptr_t row_off = (uintptr_t)step * (uintptr_t)seg_in_batch;
    uint32_t width = ppo_action_width_get(ppo);
    uint32_t ret = 0;

    out->rows = seg_in_batch;
    out->obs = ppo_lane_get(ppo, PPO_LANE_OBS) + (row_off * (uintptr_t)ppo->conf.obs_dim);
    out->critic_obs = ppo_lane_get(ppo, PPO_LANE_CRITIC_OBS) + (row_off * (uintptr_t)ppo_critic_dim_get(ppo));
    out->act_mask = 0x0;
    if (ppo->batch_action_mask) { out->act_mask = ppo->batch_action_mask + (row_off * (uintptr_t)ppo->conf.action_dim); }

    ret = ppo_leaf_push(ppo, ppo_lane_get(ppo, PPO_LANE_ACTION) + (row_off * (uintptr_t)width), seg_in_batch, width, &out->act_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_leaf_push(ppo, ppo_lane_get(ppo, PPO_LANE_LOGP) + row_off, seg_in_batch, PPO_SCALAR_COL, &out->logp_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_leaf_push(ppo, ppo_lane_get(ppo, PPO_LANE_ADV) + row_off, seg_in_batch, PPO_SCALAR_COL, &out->adv_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_leaf_push(ppo, ppo_lane_get(ppo, PPO_LANE_TGT) + row_off, seg_in_batch, PPO_SCALAR_COL, &out->tgt_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ppo_leaf_push(ppo, ppo_lane_get(ppo, PPO_LANE_MASK) + row_off, seg_in_batch, PPO_SCALAR_COL, &out->mask_node);
}

typedef struct ppo_cum
{
    uint32_t obj;
    uint32_t val;
    uint32_t ent;
} ppo_cum_t;

static uint32_t ppo_recur_step_push(ldg_math_stat_ppo_t *ppo, uint32_t step, uint32_t seg_in_batch, ppo_cum_t *cum, uint32_t *out_ratio)
{
    ppo_step_leaf_t leaf = LDG_STRUCT_ZERO_INIT;
    uint32_t hd = 0;
    uint32_t logp = 0;
    uint32_t row_ent = 0;
    uint32_t pick = 0;
    uint32_t val_node = 0;
    uint32_t val_sq = 0;
    uint32_t step_obj = 0;
    uint32_t step_val = 0;
    uint32_t ret = 0;

    ret = ppo_step_leaf_push(ppo, step, seg_in_batch, &leaf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_forward(&ppo->actor, leaf.obs, seg_in_batch, &hd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_logp_push(ppo, hd, leaf.act_node, leaf.act_mask, &logp, &row_ent);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_surr_push(ppo, logp, leaf.logp_node, leaf.adv_node, out_ratio, &pick);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_masked_mean_push(ppo, pick, leaf.mask_node, &step_obj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_cum_push(ppo, cum->obj, step_obj, &cum->obj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_ent_step_push(ppo, row_ent, leaf.mask_node, &cum->ent);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_forward(&ppo->critic, leaf.critic_obs, seg_in_batch, &val_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_val_sq_push(ppo, val_node, leaf.tgt_node, &val_sq);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_masked_mean_push(ppo, val_sq, leaf.mask_node, &step_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ppo_cum_push(ppo, cum->val, step_val, &cum->val);
}

static uint32_t ppo_recur_valid_get(const ldg_math_stat_ppo_t *ppo, uint32_t row_cunt)
{
    const double *mask_lane = ppo_lane_get(ppo, PPO_LANE_MASK);
    uint32_t valid = 0;
    uint32_t idx = 0;

    while (idx < row_cunt)
    {
        if (mask_lane[idx] == PPO_MASK_ON) { valid++; }

        idx++;
    }

    return valid;
}

static uint32_t ppo_cum_begin(ldg_math_stat_ppo_t *ppo, uint32_t seg_in_batch, ppo_cum_t *cum)
{
    uint32_t net = 0;
    uint32_t ret = 0;

    cum->obj = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;
    cum->val = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;
    cum->ent = LDG_MATH_STAT_AUTOGRAD_NODE_NONE;

    ret = ldg_math_stat_autograd_tape_rst(&ppo->graph);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (net < PPO_NET_CUNT)
    {
        if (ppo_model_state_len_get(ppo, net) > 0)
        {
            ret = ldg_math_stat_model_state_push(ppo_model_get(ppo, net), ppo_state_lane_net_get(ppo, net, seg_in_batch), seg_in_batch);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        net++;
    }

    return LDG_ERR_AOK;
}

static uint32_t ppo_recur_minibatch_run(ldg_math_stat_ppo_t *ppo, uint32_t seg_in_batch, ldg_math_stat_ppo_report_t *acc)
{
    ppo_cum_t cum = LDG_STRUCT_ZERO_INIT;
    double *ratio_lane = ppo_lane_get(ppo, PPO_LANE_SAMPLE);
    const double *mask_lane = ppo_lane_get(ppo, PPO_LANE_MASK);
    uint32_t row_cunt = seg_in_batch * ppo->conf.seg_cap;
    uint32_t valid = ppo_recur_valid_get(ppo, row_cunt);
    double row_scale = 0.0;
    uint32_t ratio = 0;
    uint32_t obj = 0;
    uint32_t val_loss = 0;
    uint32_t entropy = 0;
    uint32_t total = 0;
    uint32_t step = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(valid == 0)) { return LDG_ERR_EMPTY; }

    row_scale = (double)seg_in_batch / (double)valid;

    ret = ppo_cum_begin(ppo, seg_in_batch, &cum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (step < ppo->conf.seg_cap)
    {
        ret = ppo_recur_step_push(ppo, step, seg_in_batch, &cum, &ratio);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_stat_autograd_val_get(&ppo->graph, ratio, ratio_lane + ((uintptr_t)step * (uintptr_t)seg_in_batch), (uintptr_t)seg_in_batch);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        step++;
    }

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, cum.obj, row_scale, &obj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, cum.val, row_scale, &val_loss);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_ent_total_push(ppo, cum.ent, row_scale, &entropy);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_loss_push(ppo, obj, val_loss, entropy, &total);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ppo_step_finish(ppo, total, obj, val_loss, entropy, LDG_MATH_STAT_AUTOGRAD_NODE_NONE, row_cunt, mask_lane, LDG_TRUTH_TRUE, acc);
}

static uint32_t ppo_recur_epoch_run(ldg_math_stat_ppo_t *ppo, uint32_t seg_cunt, ldg_math_stat_ppo_report_t *acc, uint32_t *batch_cunt)
{
    uint32_t seg_per_batch = ppo_seg_per_batch_get(ppo);
    uint32_t start = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    ppo_idx_shuffle(ppo, seg_cunt);

    while (start < seg_cunt)
    {
        cunt = seg_per_batch;
        if (start + cunt > seg_cunt) { cunt = seg_cunt - start; }

        ret = ppo_seg_gather(ppo, start, cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ppo_recur_minibatch_run(ppo, cunt, acc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        (*batch_cunt)++;
        start = start + cunt;
    }

    return LDG_ERR_AOK;
}

static uint32_t ppo_epoch_run(ldg_math_stat_ppo_t *ppo, ldg_math_stat_ppo_report_t *acc, uint32_t *batch_cunt)
{
    uint32_t start = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    ppo_idx_shuffle(ppo, ppo->step_cunt);

    while (start < ppo->step_cunt)
    {
        cunt = ppo->conf.batch_size;
        if (start + cunt > ppo->step_cunt) { cunt = ppo->step_cunt - start; }

        ret = ppo_batch_gather(ppo, start, cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ppo_minibatch_run(ppo, cunt, acc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        (*batch_cunt)++;
        start = start + cunt;
    }

    return LDG_ERR_AOK;
}

static void ppo_sil_tgt_compute(ldg_math_stat_ppo_t *ppo, double last_val)
{
    uint32_t body = 0;
    uint32_t idx = ppo->step_cunt;

    ppo_body_seed(ppo, last_val);

    while (idx > 0)
    {
        idx--;
        body = ppo->body_buff[idx];

        if (ppo->terminal_buff[idx] == LDG_TRUTH_TRUE) { ppo->body_next[body] = ppo->bootstrap_buff[idx]; }

        ppo->body_next[body] = ppo->reward_buff[idx] + (ppo->conf.gamma * ppo->body_next[body]);
        ppo->sil_step_buff[idx] = ppo->body_next[body];
    }
}

static double ppo_sil_seg_prio_get(const ldg_math_stat_ppo_t *ppo, uint32_t seg)
{
    double cum = 0.0;
    double gap = 0.0;
    double mean = 0.0;
    double sigma = 0.0;
    uint32_t len = ppo_seg_len_get(ppo, seg);
    uint32_t pick = 0;
    uint32_t step = 0;

    ppo_norm_stat_get(ppo, &mean, &sigma);

    while (step < len)
    {
        pick = ppo_seg_step_get(ppo, seg, step);
        gap = (ppo->sil_step_buff[pick] - ppo->val_buff[pick]) / sigma;
        if (gap > PPO_SIL_PRIO_FLOOR) { cum = cum + gap; }

        step++;
    }

    return cum / (double)len;
}

static uint32_t ppo_sil_slot_get(const ldg_math_stat_ppo_t *ppo, double prio)
{
    uint32_t low = 0;
    uint32_t idx = 0;

    if (ppo->sil_cunt < ppo->conf.sil_seg_cap) { return ppo->sil_cunt; }

    while (idx < ppo->sil_cunt)
    {
        if (ppo->sil_prio[idx] < ppo->sil_prio[low]) { low = idx; }

        idx++;
    }

    if (ppo->sil_prio[low] >= prio) { return UINT32_MAX; }

    return low;
}

static double* ppo_sil_state_get(const ldg_math_stat_ppo_t *ppo, uint32_t slot)
{
    return ppo->sil_state + ((uintptr_t)slot * ppo_state_len_get(ppo));
}

static uint32_t ppo_sil_row_st(ldg_math_stat_ppo_t *ppo, uintptr_t dst_row, uint32_t pick)
{
    uintptr_t width = ppo_action_width_get(ppo);
    uintptr_t obs_dim = ppo->conf.obs_dim;
    uintptr_t critic_dim = ppo->conf.critic_obs_dim;
    uint32_t ret = 0;

    ret = ldg_mem_cpy(ppo->sil_obs + (dst_row * obs_dim), ppo->obs_buff + ((uintptr_t)pick * obs_dim), (uint64_t)obs_dim * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ldg_mem_cpy(ppo->sil_critic_obs + (dst_row * critic_dim), ppo->critic_obs_buff + ((uintptr_t)pick * critic_dim), (uint64_t)critic_dim * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_mem_cpy(ppo->sil_action + (dst_row * width), ppo->action_buff + ((uintptr_t)pick * width), (uint64_t)width * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_mask_row_cpy(ppo, ppo->sil_action_mask, dst_row, ppo->action_mask_buff, (uintptr_t)pick);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo->sil_tgt[dst_row] = ppo->sil_step_buff[pick];

    return LDG_ERR_AOK;
}

static uint32_t ppo_sil_seg_insert(ldg_math_stat_ppo_t *ppo, uint32_t slot, uint32_t seg, double prio)
{
    uintptr_t base = (uintptr_t)slot * (uintptr_t)ppo->conf.seg_cap;
    uint32_t len = ppo_seg_len_get(ppo, seg);
    uint32_t step = 0;
    uint32_t ret = 0;

    while (step < len)
    {
        ret = ppo_sil_row_st(ppo, base + (uintptr_t)step, ppo_seg_step_get(ppo, seg, step));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        step++;
    }

    if (ppo_recur_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ldg_mem_cpy(ppo_sil_state_get(ppo, slot), ppo_state_step_get(ppo, ppo_seg_step_get(ppo, seg, PPO_ROW_FIRST)), (uint64_t)ppo_state_len_get(ppo) * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ppo->sil_len[slot] = len;
    ppo->sil_prio[slot] = prio;

    if (slot == ppo->sil_cunt) { ppo->sil_cunt++; }

    return LDG_ERR_AOK;
}

static uint32_t ppo_sil_fill(ldg_math_stat_ppo_t *ppo, uint32_t seg_cunt)
{
    double threshold = (double)ppo->conf.sil_threshold;
    double prio = 0.0;
    uint32_t seg = 0;
    uint32_t slot = 0;
    uint32_t ret = 0;

    while (seg < seg_cunt)
    {
        prio = ppo_sil_seg_prio_get(ppo, seg);

        if (prio > threshold)
        {
            slot = ppo_sil_slot_get(ppo, prio);
            if (slot != UINT32_MAX)
            {
                ret = ppo_sil_seg_insert(ppo, slot, seg, prio);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
        }

        seg++;
    }

    return LDG_ERR_AOK;
}

static uint32_t ppo_sil_row_ld(ldg_math_stat_ppo_t *ppo, uint32_t lane_row, uintptr_t src_row)
{
    double *obs_lane = ppo_lane_get(ppo, PPO_LANE_OBS);
    double *critic_lane = ppo_lane_get(ppo, PPO_LANE_CRITIC_OBS);
    double *act_lane = ppo_lane_get(ppo, PPO_LANE_ACTION);
    double *tgt_lane = ppo_lane_get(ppo, PPO_LANE_TGT);
    uintptr_t width = ppo_action_width_get(ppo);
    uintptr_t obs_dim = ppo->conf.obs_dim;
    uintptr_t critic_dim = ppo->conf.critic_obs_dim;
    uint32_t ret = 0;

    ret = ldg_mem_cpy(obs_lane + ((uintptr_t)lane_row * obs_dim), ppo->sil_obs + (src_row * obs_dim), (uint64_t)obs_dim * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ldg_mem_cpy(critic_lane + ((uintptr_t)lane_row * critic_dim), ppo->sil_critic_obs + (src_row * critic_dim), (uint64_t)critic_dim * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_mem_cpy(act_lane + ((uintptr_t)lane_row * width), ppo->sil_action + (src_row * width), (uint64_t)width * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_mask_row_cpy(ppo, ppo->batch_action_mask, (uintptr_t)lane_row, ppo->sil_action_mask, src_row);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    tgt_lane[lane_row] = ppo_tgt_norm_apply(ppo, ppo->sil_tgt[src_row]);

    return LDG_ERR_AOK;
}

static uint32_t ppo_sil_gather(ldg_math_stat_ppo_t *ppo, uint32_t seg_start_idx, uint32_t seg_in_batch)
{
    double *mask_lane = ppo_lane_get(ppo, PPO_LANE_MASK);
    uintptr_t base = 0;
    uint32_t slot = 0;
    uint32_t len = 0;
    uint32_t row = 0;
    uint32_t lane_row = 0;
    uint32_t step = 0;
    uint32_t ret = 0;

    while (row < seg_in_batch)
    {
        slot = ppo->idx_buff[seg_start_idx + row];
        len = ppo->sil_len[slot];
        base = (uintptr_t)slot * (uintptr_t)ppo->conf.seg_cap;

        if (ppo_recur_is(ppo) == LDG_TRUTH_TRUE)
        {
            ret = ppo_state_lane_fill(ppo, row, seg_in_batch, ppo_sil_state_get(ppo, slot));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        step = 0;
        while (step < ppo->conf.seg_cap)
        {
            lane_row = (step * seg_in_batch) + row;

            if (step < len)
            {
                ret = ppo_sil_row_ld(ppo, lane_row, base + (uintptr_t)step);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                mask_lane[lane_row] = PPO_MASK_ON;
            }
            else
            {
                ret = ppo_row_blank(ppo, lane_row);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                mask_lane[lane_row] = PPO_MASK_OFF;
            }

            step++;
        }

        row++;
    }

    return LDG_ERR_AOK;
}

static uint32_t ppo_sil_weight_push(ldg_math_stat_ppo_t *ppo, uint32_t val_node, uintptr_t row_off, uint32_t row_cunt, uint32_t seg_in_batch, uint32_t *out)
{
    double *buff = ppo_lane_get(ppo, PPO_LANE_SAMPLE);
    double *cum_lane = ppo_lane_get(ppo, PPO_LANE_ADV);
    const double *tgt_lane = ppo_lane_get(ppo, PPO_LANE_TGT) + row_off;
    const double *mask_lane = ppo_lane_get(ppo, PPO_LANE_MASK) + row_off;
    double gap = 0.0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_val_get(&ppo->graph, val_node, buff, (uintptr_t)ppo_sample_len_get(ppo));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (idx < row_cunt)
    {
        gap = tgt_lane[idx] - buff[idx];
        buff[idx] = PPO_SIL_PRIO_FLOOR;
        if (mask_lane[idx] == PPO_MASK_ON && gap > PPO_SIL_PRIO_FLOOR) { buff[idx] = gap; }

        cum_lane[idx % seg_in_batch] = cum_lane[idx % seg_in_batch] + buff[idx];
        idx++;
    }

    return ppo_leaf_push(ppo, buff, row_cunt, PPO_SCALAR_COL, out);
}

static uint32_t ppo_sil_loss_push(ldg_math_stat_ppo_t *ppo, uint32_t logp, uint32_t weight, uint32_t val_node, uint32_t tgt_node, uint32_t mask_node, uint32_t *out_obj, uint32_t *out_val)
{
    uint32_t term = 0;
    uint32_t diff = 0;
    uint32_t pos = 0;
    uint32_t sq = 0;
    uint32_t half = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_mul_push(&ppo->graph, logp, weight, &term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mean_push(&ppo->graph, term, out_obj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_sub_push(&ppo->graph, tgt_node, val_node, &diff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_relu_push(&ppo->graph, diff, &pos);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_square_push(&ppo->graph, pos, &sq);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, sq, PPO_SIL_VAL_HALF, &half);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ppo_masked_mean_push(ppo, half, mask_node, out_val);
}

static uint32_t ppo_sil_total_push(ldg_math_stat_ppo_t *ppo, uint32_t obj, uint32_t val_loss, uint32_t *out)
{
    uint32_t neg_obj = 0;
    uint32_t val_term = 0;
    uint32_t sum = 0;
    uint32_t ret = 0;

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, obj, PPO_UNIT_NEG, &neg_obj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, val_loss, ppo->conf.val_coef, &val_term);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_add_push(&ppo->graph, neg_obj, val_term, &sum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_scale_push(&ppo->graph, sum, ppo->conf.sil_weight, out);
}

static uint32_t ppo_sil_step_push(ldg_math_stat_ppo_t *ppo, uint32_t step, uint32_t seg_in_batch, uint32_t row_cunt, ppo_cum_t *cum)
{
    ppo_step_leaf_t leaf = LDG_STRUCT_ZERO_INIT;
    uintptr_t row_off = (uintptr_t)step * (uintptr_t)seg_in_batch;
    uint32_t val_node = 0;
    uint32_t weight = 0;
    uint32_t hd = 0;
    uint32_t logp = 0;
    uint32_t row_ent = 0;
    uint32_t step_obj = 0;
    uint32_t step_val = 0;
    uint32_t ret = 0;

    ret = ppo_step_leaf_push(ppo, step, row_cunt, &leaf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_forward(&ppo->critic, leaf.critic_obs, row_cunt, &val_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_sil_weight_push(ppo, val_node, row_off, row_cunt, seg_in_batch, &weight);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_forward(&ppo->actor, leaf.obs, row_cunt, &hd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_logp_push(ppo, hd, leaf.act_node, leaf.act_mask, &logp, &row_ent);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_sil_loss_push(ppo, logp, weight, val_node, leaf.tgt_node, leaf.mask_node, &step_obj, &step_val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_cum_push(ppo, cum->obj, step_obj, &cum->obj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_cum_push(ppo, cum->val, step_val, &cum->val);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ppo_ent_step_push(ppo, row_ent, leaf.mask_node, &cum->ent);
}

static uint32_t ppo_sil_minibatch_run(ldg_math_stat_ppo_t *ppo, uint32_t seg_in_batch, ldg_math_stat_ppo_report_t *acc)
{
    ppo_cum_t cum = LDG_STRUCT_ZERO_INIT;
    const double *mask_lane = ppo_lane_get(ppo, PPO_LANE_MASK);
    uint32_t row_cunt = seg_in_batch * ppo->conf.seg_cap;
    uint32_t valid = ppo_recur_valid_get(ppo, row_cunt);
    uint32_t step_cunt = ppo->conf.seg_cap;
    uint32_t step_rows = seg_in_batch;
    double row_scale = 0.0;
    uint32_t obj = 0;
    uint32_t val_loss = 0;
    uint32_t entropy = 0;
    uint32_t total = 0;
    uint32_t step = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(valid == 0)) { return LDG_ERR_EMPTY; }

    if (ppo_recur_is(ppo) != LDG_TRUTH_TRUE)
    {
        step_cunt = PPO_SCALAR_ROW;
        step_rows = row_cunt;
    }

    row_scale = (double)step_rows / (double)valid;

    ret = ppo_cum_begin(ppo, seg_in_batch, &cum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (step < step_cunt)
    {
        ret = ppo_sil_step_push(ppo, step, seg_in_batch, step_rows, &cum);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        step++;
    }

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, cum.obj, row_scale, &obj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, cum.val, row_scale, &val_loss);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_recur_is(ppo) != LDG_TRUTH_TRUE && ppo->conf.action_kind != LDG_MATH_STAT_PPO_ACTION_DISCRETE) { row_scale = PPO_UNIT_POS; }

    if (ppo_recur_is(ppo) == LDG_TRUTH_TRUE) { ret = ppo_ent_total_push(ppo, cum.ent, row_scale, &entropy); }
    else { ret = ldg_math_stat_autograd_scale_push(&ppo->graph, cum.ent, row_scale, &entropy); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_sil_total_push(ppo, obj, val_loss, &total);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ppo_step_finish(ppo, total, obj, val_loss, entropy, LDG_MATH_STAT_AUTOGRAD_NODE_NONE, row_cunt, mask_lane, LDG_TRUTH_FALSE, acc);
}

static void ppo_sil_prio_refresh(ldg_math_stat_ppo_t *ppo, uint32_t seg_start_idx, uint32_t seg_in_batch)
{
    const double *cum_lane = ppo_lane_get(ppo, PPO_LANE_ADV);
    uint32_t slot = 0;
    uint32_t row = 0;

    while (row < seg_in_batch)
    {
        slot = ppo->idx_buff[seg_start_idx + row];
        ppo->sil_prio[slot] = cum_lane[row] / (double)ppo->sil_len[slot];
        row++;
    }
}

static uint32_t ppo_sil_epoch_run(ldg_math_stat_ppo_t *ppo, ldg_math_stat_ppo_report_t *acc, uint32_t *batch_cunt)
{
    uint32_t seg_per_batch = ppo_seg_per_batch_get(ppo);
    uint32_t start = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    ppo_idx_shuffle(ppo, ppo->sil_cunt);

    while (start < ppo->sil_cunt)
    {
        cunt = seg_per_batch;
        if (start + cunt > ppo->sil_cunt) { cunt = ppo->sil_cunt - start; }

        ret = ldg_mem_zero(ppo_lane_get(ppo, PPO_LANE_ADV), (uint64_t)cunt * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ppo_sil_gather(ppo, start, cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ppo_sil_minibatch_run(ppo, cunt, acc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ppo_sil_prio_refresh(ppo, start, cunt);

        (*batch_cunt)++;
        start = start + cunt;
    }

    return LDG_ERR_AOK;
}

static void ppo_report_fill(const ldg_math_stat_ppo_report_t *acc, uint32_t batch_cunt, ldg_math_stat_ppo_report_t *out)
{
    out->policy_loss = acc->policy_loss / (double)batch_cunt;
    out->val_loss = acc->val_loss / (double)batch_cunt;
    out->entropy = acc->entropy / (double)batch_cunt;
    out->grad_norm = acc->grad_norm / (double)batch_cunt;
    out->clip_frac = acc->clip_frac / (double)batch_cunt;
    out->kl = acc->kl / (double)batch_cunt;
}

uint32_t ldg_math_stat_ppo_sil_update(ldg_math_stat_ppo_t *ppo, uint32_t epoch_cunt, ldg_math_stat_ppo_report_t *out)
{
    ldg_math_stat_ppo_report_t acc = LDG_STRUCT_ZERO_INIT;
    uint32_t epoch = 0;
    uint32_t batch_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo_sil_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(epoch_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ppo->sil_cunt == 0)) { return LDG_ERR_EMPTY; }

    while (epoch < epoch_cunt)
    {
        ret = ppo_sil_epoch_run(ppo, &acc, &batch_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        epoch++;
    }

    ppo_report_fill(&acc, batch_cunt, out);

    return ldg_math_stat_autograd_tape_rst(&ppo->graph);
}

uint32_t ldg_math_stat_ppo_sil_cunt_get(const ldg_math_stat_ppo_t *ppo, uint64_t *out_seg_cunt, uint64_t *out_seg_len)
{
    if (LDG_UNLIKELY(!ppo || !out_seg_cunt || !out_seg_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_seg_cunt = 0;
    *out_seg_len = 0;

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo_sil_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    *out_seg_cunt = ppo->sil_cunt;
    *out_seg_len = (uint64_t)ppo_sil_seg_len_get(ppo);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_sil_get(const ldg_math_stat_ppo_t *ppo, double *dst, uint64_t dst_cap, uint64_t *out_cunt)
{
    uintptr_t seg_len = 0;
    uintptr_t seg_cap = 0;
    uintptr_t width = 0;
    uintptr_t state_len = 0;
    uintptr_t critic_dim = 0;
    uintptr_t base = 0;
    double *curr = dst;
    uint32_t slot = 0;

    if (LDG_UNLIKELY(!ppo || !dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo_sil_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    seg_len = ppo_sil_seg_len_get(ppo);
    seg_cap = ppo->conf.seg_cap;
    width = ppo_action_width_get(ppo);
    state_len = ppo_state_len_get(ppo);
    critic_dim = ppo->conf.critic_obs_dim;

    if (LDG_UNLIKELY(dst_cap < (uint64_t)seg_len * (uint64_t)ppo->sil_cunt)) { return LDG_ERR_BOUNDS; }

    while (slot < ppo->sil_cunt)
    {
        base = (uintptr_t)slot * seg_cap;

        *curr = (double)ppo->sil_len[slot];
        curr++;
        *curr = (double)ppo->sil_prio[slot];
        curr++;

        if (state_len > 0)
        {
            ppo_float_to_double_cpy(ppo_sil_state_get(ppo, slot), curr, state_len);
            curr = curr + state_len;
        }

        ppo_float_to_double_cpy(ppo->sil_obs + (base * (uintptr_t)ppo->conf.obs_dim), curr, seg_cap * (uintptr_t)ppo->conf.obs_dim);
        curr = curr + (seg_cap * (uintptr_t)ppo->conf.obs_dim);

        if (ppo_split_is(ppo) == LDG_TRUTH_TRUE)
        {
            ppo_float_to_double_cpy(ppo->sil_critic_obs + (base * critic_dim), curr, seg_cap * critic_dim);
            curr = curr + (seg_cap * critic_dim);
        }

        ppo_float_to_double_cpy(ppo->sil_action + (base * width), curr, seg_cap * width);
        curr = curr + (seg_cap * width);

        ppo_float_to_double_cpy(ppo->sil_tgt + base, curr, seg_cap);
        curr = curr + seg_cap;

        if (ppo_mask_is(ppo) == LDG_TRUTH_TRUE)
        {
            ppo_mask_to_double_cpy(ppo->sil_action_mask + (base * (uintptr_t)ppo->conf.action_dim), curr, seg_cap * (uintptr_t)ppo->conf.action_dim);
            curr = curr + (seg_cap * (uintptr_t)ppo->conf.action_dim);
        }

        slot++;
    }

    *out_cunt = (uint64_t)seg_len * (uint64_t)ppo->sil_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_sil_set(ldg_math_stat_ppo_t *ppo, const double *src, uint64_t cunt)
{
    uintptr_t seg_len = 0;
    uintptr_t seg_cap = 0;
    uintptr_t width = 0;
    uintptr_t state_len = 0;
    uintptr_t critic_dim = 0;
    uintptr_t base = 0;
    const double *curr = src;
    uint64_t seg_cunt = 0;
    uint32_t slot = 0;

    if (LDG_UNLIKELY(!ppo || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo_sil_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    seg_len = ppo_sil_seg_len_get(ppo);
    seg_cap = ppo->conf.seg_cap;
    width = ppo_action_width_get(ppo);
    state_len = ppo_state_len_get(ppo);
    critic_dim = ppo->conf.critic_obs_dim;

    if (LDG_UNLIKELY(cunt % (uint64_t)seg_len != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    seg_cunt = cunt / (uint64_t)seg_len;

    if (LDG_UNLIKELY(seg_cunt > (uint64_t)ppo->conf.sil_seg_cap)) { return LDG_ERR_BOUNDS; }

    while (slot < seg_cunt)
    {
        if (LDG_UNLIKELY(curr[0] < PPO_UNIT_POS || curr[0] > (double)seg_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(ppo_mask_seg_ck(ppo, curr, seg_len) != LDG_ERR_AOK)) { return LDG_ERR_FUNC_ARG_INVALID; }

        curr = curr + seg_len;
        slot++;
    }

    curr = src;
    slot = 0;
    while (slot < seg_cunt)
    {
        base = (uintptr_t)slot * seg_cap;

        ppo->sil_len[slot] = (uint32_t)*curr;
        curr++;
        ppo->sil_prio[slot] = (double)*curr;
        curr++;

        if (state_len > 0)
        {
            ppo_double_to_float_cpy(curr, ppo_sil_state_get(ppo, slot), state_len);
            curr = curr + state_len;
        }

        ppo_double_to_float_cpy(curr, ppo->sil_obs + (base * (uintptr_t)ppo->conf.obs_dim), seg_cap * (uintptr_t)ppo->conf.obs_dim);
        curr = curr + (seg_cap * (uintptr_t)ppo->conf.obs_dim);

        if (ppo_split_is(ppo) == LDG_TRUTH_TRUE)
        {
            ppo_double_to_float_cpy(curr, ppo->sil_critic_obs + (base * critic_dim), seg_cap * critic_dim);
            curr = curr + (seg_cap * critic_dim);
        }

        ppo_double_to_float_cpy(curr, ppo->sil_action + (base * width), seg_cap * width);
        curr = curr + (seg_cap * width);

        ppo_double_to_float_cpy(curr, ppo->sil_tgt + base, seg_cap);
        curr = curr + seg_cap;

        if (ppo_mask_is(ppo) == LDG_TRUTH_TRUE)
        {
            ppo_double_to_mask_cpy(curr, ppo->sil_action_mask + (base * (uintptr_t)ppo->conf.action_dim), seg_cap * (uintptr_t)ppo->conf.action_dim);
            curr = curr + (seg_cap * (uintptr_t)ppo->conf.action_dim);
        }

        slot++;
    }

    ppo->sil_cunt = (uint32_t)seg_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_reward_add(ldg_math_stat_ppo_t *ppo, uint32_t body, double reward, uint32_t span)
{
    uint32_t idx = 0;
    uint32_t done = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ppo_body_ck(ppo, body);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(span == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = ppo->body_last[body];

    if (LDG_UNLIKELY(idx == UINT32_MAX)) { return LDG_ERR_EMPTY; }

    idx++;
    while (idx > 0 && done < span)
    {
        idx--;

        if (ppo->body_buff[idx] == body)
        {
            ppo->reward_buff[idx] = ppo->reward_buff[idx] + (double)reward;
            done++;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t ppo_demo_close(ldg_math_stat_ppo_t *ppo)
{
    if (ppo->demo_open == UINT32_MAX) { return LDG_ERR_AOK; }

    ppo->demo_len[ppo->demo_open] = ppo->demo_open_len;
    ppo->demo_open = UINT32_MAX;
    ppo->demo_open_len = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_demo_add(ldg_math_stat_ppo_t *ppo, const double *obs, const double *critic_obs, const double *action, double weight, uint8_t terminal, const uint8_t *action_mask)
{
    uintptr_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !obs || !action)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo_demo_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    ret = ppo_critic_arg_ck(ppo, critic_obs);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_mask_arg_ck(ppo, action_mask);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo->demo_open == UINT32_MAX)
    {
        ppo->demo_open = ppo->demo_hd;
        ppo->demo_open_len = 0;
        ppo->demo_len[ppo->demo_open] = 0;
        ppo->demo_hd = (ppo->demo_hd + PPO_SCALAR_ROW) % ppo->conf.demo_seg_cap;
        if (ppo->demo_cunt < ppo->conf.demo_seg_cap) { ppo->demo_cunt++; }
    }

    row = ((uintptr_t)ppo->demo_open * (uintptr_t)ppo->conf.seg_cap) + (uintptr_t)ppo->demo_open_len;

    ppo_stage(obs, ppo->demo_obs + (row * (uintptr_t)ppo->conf.obs_dim), ppo->conf.obs_dim);
    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE) { ppo_stage(critic_obs, ppo->demo_critic_obs + (row * (uintptr_t)ppo->conf.critic_obs_dim), ppo->conf.critic_obs_dim); }

    ppo_stage(action, ppo->demo_action + (row * (uintptr_t)ppo_action_width_get(ppo)), ppo_action_width_get(ppo));
    ppo->demo_weight[row] = (double)weight;

    ret = ppo_mask_st(ppo, action_mask, ppo->demo_action_mask, row);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ppo->demo_open_len++;
    ppo->demo_len[ppo->demo_open] = ppo->demo_open_len;

    if (terminal == LDG_TRUTH_TRUE || ppo->demo_open_len == ppo->conf.seg_cap) { return ppo_demo_close(ppo); }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_demo_end(ldg_math_stat_ppo_t *ppo)
{
    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo_demo_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    return ppo_demo_close(ppo);
}

uint32_t ldg_math_stat_ppo_demo_rst(ldg_math_stat_ppo_t *ppo)
{
    if (LDG_UNLIKELY(!ppo)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo_demo_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    ppo->demo_cunt = 0;
    ppo->demo_hd = 0;
    ppo->demo_open = UINT32_MAX;
    ppo->demo_open_len = 0;

    return LDG_ERR_AOK;
}

static uint32_t ppo_demo_row_ld(ldg_math_stat_ppo_t *ppo, uint32_t lane_row, uintptr_t src_row)
{
    double *obs_lane = ppo_lane_get(ppo, PPO_LANE_OBS);
    double *critic_lane = ppo_lane_get(ppo, PPO_LANE_CRITIC_OBS);
    double *act_lane = ppo_lane_get(ppo, PPO_LANE_ACTION);
    uintptr_t width = ppo_action_width_get(ppo);
    uintptr_t obs_dim = ppo->conf.obs_dim;
    uintptr_t critic_dim = ppo->conf.critic_obs_dim;
    uint32_t ret = 0;

    ret = ldg_mem_cpy(obs_lane + ((uintptr_t)lane_row * obs_dim), ppo->demo_obs + (src_row * obs_dim), (uint64_t)obs_dim * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_split_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ldg_mem_cpy(critic_lane + ((uintptr_t)lane_row * critic_dim), ppo->demo_critic_obs + (src_row * critic_dim), (uint64_t)critic_dim * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ppo_lane_get(ppo, PPO_LANE_ADV)[lane_row] = ppo->demo_weight[src_row];

    ret = ldg_mem_cpy(act_lane + ((uintptr_t)lane_row * width), ppo->demo_action + (src_row * width), (uint64_t)width * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ppo_mask_row_cpy(ppo, ppo->batch_action_mask, (uintptr_t)lane_row, ppo->demo_action_mask, src_row);
}

static uint32_t ppo_demo_gather(ldg_math_stat_ppo_t *ppo, uint32_t seg_start_idx, uint32_t seg_in_batch)
{
    double *mask_lane = ppo_lane_get(ppo, PPO_LANE_MASK);
    uintptr_t base = 0;
    uint32_t slot = 0;
    uint32_t len = 0;
    uint32_t row = 0;
    uint32_t lane_row = 0;
    uint32_t step = 0;
    uint32_t ret = 0;

    if (ppo_recur_is(ppo) == LDG_TRUTH_TRUE)
    {
        ret = ldg_mem_zero(ppo_lane_get(ppo, PPO_LANE_STATE), (uint64_t)ppo_state_lane_len_get(ppo) * (uint64_t)sizeof(double));
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    while (row < seg_in_batch)
    {
        slot = ppo->idx_buff[seg_start_idx + row];
        len = ppo->demo_len[slot];
        base = (uintptr_t)slot * (uintptr_t)ppo->conf.seg_cap;

        step = 0;
        while (step < ppo->conf.seg_cap)
        {
            lane_row = (step * seg_in_batch) + row;

            if (step < len)
            {
                ret = ppo_demo_row_ld(ppo, lane_row, base + (uintptr_t)step);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                mask_lane[lane_row] = PPO_MASK_ON;
            }
            else
            {
                ret = ppo_row_blank(ppo, lane_row);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                mask_lane[lane_row] = PPO_MASK_OFF;
            }

            step++;
        }

        row++;
    }

    return LDG_ERR_AOK;
}

static uint32_t ppo_demo_step_push(ldg_math_stat_ppo_t *ppo, uint32_t step, uint32_t seg_in_batch, uint32_t row_cunt, ppo_cum_t *cum)
{
    ppo_step_leaf_t leaf = LDG_STRUCT_ZERO_INIT;
    uint32_t hd = 0;
    uint32_t logp = 0;
    uint32_t row_ent = 0;
    uint32_t weighted = 0;
    uint32_t step_obj = 0;
    uint32_t ret = 0;

    ret = ppo_step_leaf_push(ppo, step, seg_in_batch, &leaf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_model_forward(&ppo->actor, leaf.obs, row_cunt, &hd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_logp_push(ppo, hd, leaf.act_node, leaf.act_mask, &logp, &row_ent);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_mul_push(&ppo->graph, logp, leaf.adv_node, &weighted);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_masked_mean_push(ppo, weighted, leaf.mask_node, &step_obj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ppo_cum_push(ppo, cum->obj, step_obj, &cum->obj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ppo_ent_step_push(ppo, row_ent, leaf.mask_node, &cum->ent);
}

static uint32_t ppo_demo_minibatch_run(ldg_math_stat_ppo_t *ppo, uint32_t seg_in_batch, ldg_math_stat_ppo_report_t *acc)
{
    ppo_cum_t cum = LDG_STRUCT_ZERO_INIT;
    double *zero_src = ppo_lane_get(ppo, PPO_LANE_SAMPLE);
    const double *mask_lane = ppo_lane_get(ppo, PPO_LANE_MASK);
    uint32_t row_cunt = seg_in_batch * ppo->conf.seg_cap;
    uint32_t valid = ppo_recur_valid_get(ppo, row_cunt);
    uint32_t step_cunt = ppo->conf.seg_cap;
    uint32_t step_rows = seg_in_batch;
    uint32_t step_width = seg_in_batch;
    double row_scale = 0.0;
    uint32_t obj = 0;
    uint32_t entropy = 0;
    uint32_t zero_node = 0;
    uint32_t total = 0;
    uint32_t step = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(valid == 0)) { return LDG_ERR_EMPTY; }

    if (ppo_recur_is(ppo) != LDG_TRUTH_TRUE)
    {
        step_cunt = PPO_SCALAR_ROW;
        step_rows = row_cunt;
        step_width = row_cunt;
    }

    row_scale = (double)step_rows / (double)valid;

    ret = ppo_cum_begin(ppo, seg_in_batch, &cum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (step < step_cunt)
    {
        ret = ppo_demo_step_push(ppo, step, step_width, step_rows, &cum);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        step++;
    }

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, cum.obj, row_scale, &obj);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ppo_recur_is(ppo) != LDG_TRUTH_TRUE && ppo->conf.action_kind != LDG_MATH_STAT_PPO_ACTION_DISCRETE) { row_scale = PPO_UNIT_POS; }

    if (ppo_recur_is(ppo) == LDG_TRUTH_TRUE) { ret = ppo_ent_total_push(ppo, cum.ent, row_scale, &entropy); }
    else { ret = ldg_math_stat_autograd_scale_push(&ppo->graph, cum.ent, row_scale, &entropy); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    zero_src[0] = PPO_ZERO_SEED;

    ret = ppo_leaf_push(ppo, zero_src, PPO_SCALAR_ROW, PPO_SCALAR_COL, &zero_node);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stat_autograd_scale_push(&ppo->graph, obj, PPO_UNIT_NEG * (double)ppo->conf.demo_weight, &total);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ppo_step_finish(ppo, total, obj, zero_node, entropy, LDG_MATH_STAT_AUTOGRAD_NODE_NONE, row_cunt, mask_lane, LDG_TRUTH_FALSE, acc);
}

static uint32_t ppo_demo_epoch_run(ldg_math_stat_ppo_t *ppo, ldg_math_stat_ppo_report_t *acc, uint32_t *batch_cunt)
{
    uint32_t seg_per_batch = ppo_seg_per_batch_get(ppo);
    uint32_t start = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    ppo_idx_shuffle(ppo, ppo->demo_cunt);

    while (start < ppo->demo_cunt)
    {
        cunt = seg_per_batch;
        if (start + cunt > ppo->demo_cunt) { cunt = ppo->demo_cunt - start; }

        ret = ppo_demo_gather(ppo, start, cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ppo_demo_minibatch_run(ppo, cunt, acc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        (*batch_cunt)++;
        start = start + cunt;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_demo_update(ldg_math_stat_ppo_t *ppo, uint32_t epoch_cunt, ldg_math_stat_ppo_report_t *out)
{
    ldg_math_stat_ppo_report_t acc = LDG_STRUCT_ZERO_INIT;
    uint32_t epoch = 0;
    uint32_t batch_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo_demo_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(epoch_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ppo_demo_close(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ppo->demo_cunt == 0)) { return LDG_ERR_EMPTY; }

    while (epoch < epoch_cunt)
    {
        ret = ppo_demo_epoch_run(ppo, &acc, &batch_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        epoch++;
    }

    ppo_report_fill(&acc, batch_cunt, out);

    return ldg_math_stat_autograd_tape_rst(&ppo->graph);
}

uint32_t ldg_math_stat_ppo_demo_cunt_get(const ldg_math_stat_ppo_t *ppo, uint64_t *out_seg_cunt, uint64_t *out_seg_len)
{
    if (LDG_UNLIKELY(!ppo || !out_seg_cunt || !out_seg_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_seg_cunt = 0;
    *out_seg_len = 0;

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo_demo_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    *out_seg_cunt = ppo->demo_cunt;
    *out_seg_len = (uint64_t)ppo_demo_seg_len_get(ppo);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_demo_get(const ldg_math_stat_ppo_t *ppo, double *dst, uint64_t dst_cap, uint64_t *out_cunt)
{
    uintptr_t seg_len = 0;
    uintptr_t seg_cap = 0;
    uintptr_t width = 0;
    uintptr_t critic_dim = 0;
    uintptr_t base = 0;
    double *curr = dst;
    uint32_t slot = 0;

    if (LDG_UNLIKELY(!ppo || !dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo_demo_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    seg_len = ppo_demo_seg_len_get(ppo);
    seg_cap = ppo->conf.seg_cap;
    width = ppo_action_width_get(ppo);
    critic_dim = ppo->conf.critic_obs_dim;

    if (LDG_UNLIKELY(dst_cap < (uint64_t)seg_len * (uint64_t)ppo->demo_cunt)) { return LDG_ERR_BOUNDS; }

    while (slot < ppo->demo_cunt)
    {
        base = (uintptr_t)slot * seg_cap;

        *curr = (double)ppo->demo_len[slot];
        curr++;

        ppo_float_to_double_cpy(ppo->demo_obs + (base * (uintptr_t)ppo->conf.obs_dim), curr, seg_cap * (uintptr_t)ppo->conf.obs_dim);
        curr = curr + (seg_cap * (uintptr_t)ppo->conf.obs_dim);

        if (ppo_split_is(ppo) == LDG_TRUTH_TRUE)
        {
            ppo_float_to_double_cpy(ppo->demo_critic_obs + (base * critic_dim), curr, seg_cap * critic_dim);
            curr = curr + (seg_cap * critic_dim);
        }

        ppo_float_to_double_cpy(ppo->demo_action + (base * width), curr, seg_cap * width);
        curr = curr + (seg_cap * width);

        ppo_float_to_double_cpy(ppo->demo_weight + base, curr, seg_cap);
        curr = curr + seg_cap;

        if (ppo_mask_is(ppo) == LDG_TRUTH_TRUE)
        {
            ppo_mask_to_double_cpy(ppo->demo_action_mask + (base * (uintptr_t)ppo->conf.action_dim), curr, seg_cap * (uintptr_t)ppo->conf.action_dim);
            curr = curr + (seg_cap * (uintptr_t)ppo->conf.action_dim);
        }

        slot++;
    }

    *out_cunt = (uint64_t)seg_len * (uint64_t)ppo->demo_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_demo_set(ldg_math_stat_ppo_t *ppo, const double *src, uint64_t cunt)
{
    uintptr_t seg_len = 0;
    uintptr_t seg_cap = 0;
    uintptr_t width = 0;
    uintptr_t critic_dim = 0;
    uintptr_t base = 0;
    const double *curr = src;
    uint64_t seg_cunt = 0;
    uint32_t slot = 0;

    if (LDG_UNLIKELY(!ppo || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo_demo_is(ppo) != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    seg_len = ppo_demo_seg_len_get(ppo);
    seg_cap = ppo->conf.seg_cap;
    width = ppo_action_width_get(ppo);
    critic_dim = ppo->conf.critic_obs_dim;

    if (LDG_UNLIKELY(cunt % (uint64_t)seg_len != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    seg_cunt = cunt / (uint64_t)seg_len;

    if (LDG_UNLIKELY(seg_cunt > (uint64_t)ppo->conf.demo_seg_cap)) { return LDG_ERR_BOUNDS; }

    while (slot < seg_cunt)
    {
        if (LDG_UNLIKELY(curr[0] < PPO_UNIT_POS || curr[0] > (double)seg_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(ppo_mask_seg_ck(ppo, curr, seg_len) != LDG_ERR_AOK)) { return LDG_ERR_FUNC_ARG_INVALID; }

        curr = curr + seg_len;
        slot++;
    }

    curr = src;
    slot = 0;
    while (slot < seg_cunt)
    {
        base = (uintptr_t)slot * seg_cap;

        ppo->demo_len[slot] = (uint32_t)*curr;
        curr++;

        ppo_double_to_float_cpy(curr, ppo->demo_obs + (base * (uintptr_t)ppo->conf.obs_dim), seg_cap * (uintptr_t)ppo->conf.obs_dim);
        curr = curr + (seg_cap * (uintptr_t)ppo->conf.obs_dim);

        if (ppo_split_is(ppo) == LDG_TRUTH_TRUE)
        {
            ppo_double_to_float_cpy(curr, ppo->demo_critic_obs + (base * critic_dim), seg_cap * critic_dim);
            curr = curr + (seg_cap * critic_dim);
        }

        ppo_double_to_float_cpy(curr, ppo->demo_action + (base * width), seg_cap * width);
        curr = curr + (seg_cap * width);

        ppo_double_to_float_cpy(curr, ppo->demo_weight + base, seg_cap);
        curr = curr + seg_cap;

        if (ppo_mask_is(ppo) == LDG_TRUTH_TRUE)
        {
            ppo_double_to_mask_cpy(curr, ppo->demo_action_mask + (base * (uintptr_t)ppo->conf.action_dim), seg_cap * (uintptr_t)ppo->conf.action_dim);
            curr = curr + (seg_cap * (uintptr_t)ppo->conf.action_dim);
        }

        slot++;
    }

    ppo->demo_cunt = (uint32_t)seg_cunt;
    ppo->demo_hd = (uint32_t)(seg_cunt % (uint64_t)ppo->conf.demo_seg_cap);
    ppo->demo_open = UINT32_MAX;
    ppo->demo_open_len = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_ppo_update(ldg_math_stat_ppo_t *ppo, double last_val, ldg_math_stat_ppo_report_t *out)
{
    ldg_math_stat_ppo_report_t acc = LDG_STRUCT_ZERO_INIT;
    uint32_t epoch = 0;
    uint32_t seg_cunt = 0;
    uint32_t batch_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ppo || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ppo->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ppo->step_cunt == 0)) { return LDG_ERR_EMPTY; }

    ppo_gae_compute(ppo, (double)last_val);

    if (ppo_norm_is(ppo) == LDG_TRUTH_TRUE) { ppo_norm_update(ppo); }

    if (ppo_seg_is(ppo) == LDG_TRUTH_TRUE) { seg_cunt = ppo_seg_table_build(ppo); }

    if (ppo_sil_is(ppo) == LDG_TRUTH_TRUE)
    {
        ppo_sil_tgt_compute(ppo, (double)last_val);

        ret = ppo_sil_fill(ppo, seg_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ppo_adv_normalize(ppo);

    while (epoch < ppo->conf.epoch_cunt)
    {
        if (ppo_recur_is(ppo) == LDG_TRUTH_TRUE) { ret = ppo_recur_epoch_run(ppo, seg_cunt, &acc, &batch_cunt); }
        else { ret = ppo_epoch_run(ppo, &acc, &batch_cunt); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        epoch++;
    }

    ppo_report_fill(&acc, batch_cunt, out);

    ret = ppo_rollout_rst(ppo);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_stat_autograd_tape_rst(&ppo->graph);
}
