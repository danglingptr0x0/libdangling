#ifndef LDG_MATH_STAT_H
#define LDG_MATH_STAT_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/complex.h>
#include <dangling/math/eigen.h>
#include <dangling/math/math.h>
#include <dangling/mem/alloc.h>

#define LDG_MATH_STAT_WELFORD_INIT 0

typedef struct ldg_math_stat_welford
{
    uint64_t cunt;
    double mean;
    double m2;
    double m3;
    double m4;
    double min;
    double max;
} ldg_math_stat_welford_t;

LDG_EXPORT uint32_t ldg_math_stat_welford_init(ldg_math_stat_welford_t *acc);
LDG_EXPORT uint32_t ldg_math_stat_welford_observe(ldg_math_stat_welford_t *acc, double val);
LDG_EXPORT uint32_t ldg_math_stat_welford_mean_get(const ldg_math_stat_welford_t *acc, double *out);
LDG_EXPORT uint32_t ldg_math_stat_welford_var_get(const ldg_math_stat_welford_t *acc, double *out);
LDG_EXPORT uint32_t ldg_math_stat_welford_sigma_get(const ldg_math_stat_welford_t *acc, double *out);
LDG_EXPORT uint32_t ldg_math_stat_welford_skew_get(const ldg_math_stat_welford_t *acc, double *out);
LDG_EXPORT uint32_t ldg_math_stat_welford_kurt_get(const ldg_math_stat_welford_t *acc, double *out);
LDG_EXPORT uint32_t ldg_math_stat_welford_min_get(const ldg_math_stat_welford_t *acc, double *out);
LDG_EXPORT uint32_t ldg_math_stat_welford_max_get(const ldg_math_stat_welford_t *acc, double *out);
LDG_EXPORT uint64_t ldg_math_stat_welford_cunt_get(const ldg_math_stat_welford_t *acc);
LDG_EXPORT uint32_t ldg_math_stat_welford_rst(ldg_math_stat_welford_t *acc);

LDG_EXPORT uint32_t ldg_math_stat_mean_get(const double *val, uint64_t cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_var_get(const double *val, uint64_t cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_sigma_get(const double *val, uint64_t cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_min_get(const double *val, uint64_t cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_max_get(const double *val, uint64_t cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_median_get(const double *val, uint64_t cunt, double *scratch, uint64_t scratch_cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_quantile_get(const double *val, uint64_t cunt, double level, double *scratch, uint64_t scratch_cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_skew_get(const double *val, uint64_t cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_kurt_get(const double *val, uint64_t cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_cov_get(const double *arr_a, const double *arr_b, uint64_t cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_corr_get(const double *arr_a, const double *arr_b, uint64_t cunt, double *out);

#define LDG_MATH_STAT_HIST_BKT_MAX 256

typedef struct ldg_math_stat_hist
{
    uint64_t bkt[LDG_MATH_STAT_HIST_BKT_MAX];
    double lower;
    double upper;
    uint32_t bkt_cunt;
    uint8_t is_init;
    uint8_t pudding[3];
} ldg_math_stat_hist_t;

LDG_EXPORT uint32_t ldg_math_stat_hist_init(ldg_math_stat_hist_t *hist, double lower, double upper, uint32_t bkt_cunt);
LDG_EXPORT uint32_t ldg_math_stat_hist_observe(ldg_math_stat_hist_t *hist, double val);
LDG_EXPORT uint64_t ldg_math_stat_hist_bkt_get(const ldg_math_stat_hist_t *hist, uint32_t idx);
LDG_EXPORT uint32_t ldg_math_stat_hist_rst(ldg_math_stat_hist_t *hist);

#define LDG_MATH_STAT_BAYES_KIND_MAX 64
#define LDG_MATH_STAT_BAYES_KIND_INVALID UINT16_MAX
#define LDG_MATH_STAT_BAYES_PRIOR_M_DEFAULT 0.0
#define LDG_MATH_STAT_BAYES_PRIOR_KAPPA_DEFAULT 1.0
#define LDG_MATH_STAT_BAYES_PRIOR_ALPHA_DEFAULT 2.0
#define LDG_MATH_STAT_BAYES_PRIOR_BETA_DEFAULT 1.0
#define LDG_MATH_STAT_BAYES_RISK_K_DEFAULT 0.0

typedef struct ldg_math_stat_bayes_kind
{
    uint64_t obs_cunt;
    double mean;
    double m2;
    double last;
} ldg_math_stat_bayes_kind_t;

typedef struct ldg_math_stat_bayes
{
    ldg_math_stat_bayes_kind_t kind[LDG_MATH_STAT_BAYES_KIND_MAX];
    double prior_m;
    double prior_kappa;
    double prior_alpha;
    double prior_beta;
    double risk_k;
    uint64_t total_obs;
    volatile uint32_t lock;
    uint8_t is_init;
    uint8_t pudding[11];
} LDG_ALIGNED ldg_math_stat_bayes_t;

LDG_EXPORT uint32_t ldg_math_stat_bayes_init(ldg_math_stat_bayes_t *bayes, double prior_m, double prior_kappa, double prior_alpha, double prior_beta, double risk_k);
LDG_EXPORT uint32_t ldg_math_stat_bayes_shutdown(ldg_math_stat_bayes_t *bayes);
LDG_EXPORT uint32_t ldg_math_stat_bayes_observe(ldg_math_stat_bayes_t *bayes, uint16_t kind, double duration);
LDG_EXPORT uint32_t ldg_math_stat_bayes_proj(ldg_math_stat_bayes_t *bayes, uint16_t kind, double *out);
LDG_EXPORT uint32_t ldg_math_stat_bayes_proj_k(ldg_math_stat_bayes_t *bayes, uint16_t kind, double risk_k, double *out);
LDG_EXPORT uint32_t ldg_math_stat_bayes_mean_get(ldg_math_stat_bayes_t *bayes, uint16_t kind, double *out);
LDG_EXPORT uint32_t ldg_math_stat_bayes_var_get(ldg_math_stat_bayes_t *bayes, uint16_t kind, double *out);
LDG_EXPORT uint32_t ldg_math_stat_bayes_sigma_get(ldg_math_stat_bayes_t *bayes, uint16_t kind, double *out);
LDG_EXPORT uint64_t ldg_math_stat_bayes_obs_cunt_get(ldg_math_stat_bayes_t *bayes, uint16_t kind);
LDG_EXPORT uint32_t ldg_math_stat_bayes_risk_k_set(ldg_math_stat_bayes_t *bayes, double risk_k);
LDG_EXPORT uint32_t ldg_math_stat_bayes_kind_rst(ldg_math_stat_bayes_t *bayes, uint16_t kind);
LDG_EXPORT uint32_t ldg_math_stat_bayes_rst(ldg_math_stat_bayes_t *bayes);

#define LDG_MATH_STAT_SVM_FEAT_CUNT_MAX 64

typedef struct ldg_math_stat_svm
{
    double weights[LDG_MATH_STAT_SVM_FEAT_CUNT_MAX];
    double bias;
    double cost;
    double epsilon;
    uint64_t sample_cunt;
    uint32_t feat_cunt;
    uint8_t is_init;
    uint8_t pudding[27];
} LDG_ALIGNED ldg_math_stat_svm_t;

LDG_EXPORT uint32_t ldg_math_stat_svm_init(ldg_math_stat_svm_t *svm, uint32_t feat_cunt, double cost, double epsilon);
LDG_EXPORT uint32_t ldg_math_stat_svm_shutdown(ldg_math_stat_svm_t *svm);
LDG_EXPORT uint32_t ldg_math_stat_svm_predict(const ldg_math_stat_svm_t *svm, const double *feat, double *out);
LDG_EXPORT uint32_t ldg_math_stat_svm_observe(ldg_math_stat_svm_t *svm, const double *feat, double actual);
LDG_EXPORT uint32_t ldg_math_stat_svm_weights_rst(ldg_math_stat_svm_t *svm);

#define LDG_MATH_STAT_RL_BANDIT_ARM_MAX 64

typedef enum ldg_math_stat_rl_bandit_strat
{
    LDG_MATH_STAT_RL_BANDIT_STRAT_EPSILON_GREEDY = 0,
    LDG_MATH_STAT_RL_BANDIT_STRAT_UCB1,
    LDG_MATH_STAT_RL_BANDIT_STRAT_THOMPSON_BERNOULLI,
    LDG_MATH_STAT_RL_BANDIT_STRAT_THOMPSON_GAUSSIAN,
    LDG_MATH_STAT_RL_BANDIT_STRAT_CUNT
} ldg_math_stat_rl_bandit_strat_t;

typedef uint64_t (*ldg_math_stat_rng_cb_t)(void *ctx);

LDG_EXPORT uint32_t ldg_math_stat_rng_normal_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double *out);
LDG_EXPORT uint32_t ldg_math_stat_rng_gamma_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double shape, double *out);
LDG_EXPORT uint32_t ldg_math_stat_rng_beta_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double alpha, double beta, double *out);
LDG_EXPORT uint32_t ldg_math_stat_rng_dirichlet_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, const double *alpha, uint32_t cunt, double *dst);
LDG_EXPORT uint32_t ldg_math_stat_rng_categorical_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, const double *weight, uint32_t cunt, uint32_t *out);

typedef struct ldg_math_stat_rl_bandit_arm
{
    ldg_math_stat_welford_t welford;
    uint64_t pull_cunt;
    double succ_cunt;
    double alpha;
    double beta;
} ldg_math_stat_rl_bandit_arm_t;

typedef struct ldg_math_stat_rl_bandit
{
    ldg_math_stat_rl_bandit_arm_t arm[LDG_MATH_STAT_RL_BANDIT_ARM_MAX];
    ldg_math_stat_rng_cb_t rng_cb;
    void *rng_ctx;
    uint32_t arm_cunt;
    uint32_t strat;
    double epsilon;
    double c_const;
    uint64_t step_cunt;
    uint8_t is_init;
    uint8_t pudding[7];
} ldg_math_stat_rl_bandit_t;

LDG_EXPORT uint32_t ldg_math_stat_rl_bandit_init(ldg_math_stat_rl_bandit_t *bandit, uint32_t arm_cunt, uint32_t strat, double epsilon, double c_const, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx);
LDG_EXPORT uint32_t ldg_math_stat_rl_bandit_sel(ldg_math_stat_rl_bandit_t *bandit, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_rl_bandit_observe(ldg_math_stat_rl_bandit_t *bandit, uint32_t arm, double reward);
LDG_EXPORT uint32_t ldg_math_stat_rl_bandit_arm_mean_get(const ldg_math_stat_rl_bandit_t *bandit, uint32_t arm, double *out);
LDG_EXPORT uint64_t ldg_math_stat_rl_bandit_arm_pull_get(const ldg_math_stat_rl_bandit_t *bandit, uint32_t arm);
LDG_EXPORT uint32_t ldg_math_stat_rl_bandit_rst(ldg_math_stat_rl_bandit_t *bandit);

#define LDG_MATH_STAT_TD_CELL_MAX (1U << 20)

typedef enum ldg_math_stat_td_strat
{
    LDG_MATH_STAT_TD_STRAT_Q_LEARNING = 0,
    LDG_MATH_STAT_TD_STRAT_SARSA,
    LDG_MATH_STAT_TD_STRAT_Q_LAMBDA,
    LDG_MATH_STAT_TD_STRAT_SARSA_LAMBDA,
    LDG_MATH_STAT_TD_STRAT_CUNT
} ldg_math_stat_td_strat_t;

typedef struct ldg_math_stat_td
{
    double *q_arr;
    double *trace;
    ldg_math_stat_rng_cb_t rng_cb;
    void *rng_ctx;
    double alpha;
    double gamma;
    double lambda;
    double epsilon;
    double q_init;
    uint32_t state_max;
    uint32_t action_max;
    uint32_t strat;
    uint8_t last_greedy;
    uint8_t is_init;
    uint8_t pudding[2];
} ldg_math_stat_td_t;

LDG_EXPORT uint32_t ldg_math_stat_td_init(ldg_math_stat_td_t *td, uint32_t state_max, uint32_t action_max, uint32_t strat, double alpha, double gamma, double lambda, double epsilon, double q_init, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx);
LDG_EXPORT uint32_t ldg_math_stat_td_shutdown(ldg_math_stat_td_t *td);
LDG_EXPORT uint32_t ldg_math_stat_td_action_sel(ldg_math_stat_td_t *td, uint32_t state, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_td_observe(ldg_math_stat_td_t *td, uint32_t state, uint32_t action, double reward, uint32_t next_state, uint32_t next_action, uint8_t terminal);
LDG_EXPORT uint32_t ldg_math_stat_td_q_get(const ldg_math_stat_td_t *td, uint32_t state, uint32_t action, double *out);
LDG_EXPORT uint32_t ldg_math_stat_td_action_best_get(const ldg_math_stat_td_t *td, uint32_t state, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_td_q_max_get(const ldg_math_stat_td_t *td, uint32_t state, double *out);
LDG_EXPORT uint32_t ldg_math_stat_td_alpha_set(ldg_math_stat_td_t *td, double alpha);
LDG_EXPORT uint32_t ldg_math_stat_td_epsilon_set(ldg_math_stat_td_t *td, double epsilon);
LDG_EXPORT uint32_t ldg_math_stat_td_lambda_set(ldg_math_stat_td_t *td, double lambda);
LDG_EXPORT uint32_t ldg_math_stat_td_rst(ldg_math_stat_td_t *td);

#define LDG_MATH_STAT_TILE_DIMENSION_MAX 8
#define LDG_MATH_STAT_TILE_TILING_MAX 64

typedef struct ldg_math_stat_tile
{
    uint32_t tile_cunt[LDG_MATH_STAT_TILE_DIMENSION_MAX];
    double low[LDG_MATH_STAT_TILE_DIMENSION_MAX];
    double high[LDG_MATH_STAT_TILE_DIMENSION_MAX];
    uint32_t weight_cap;
    uint32_t tiling_cunt;
    uint32_t state_dimension;
    uint32_t action_max;
    uint8_t is_init;
    uint8_t pudding[7];
} ldg_math_stat_tile_t;

LDG_EXPORT uint32_t ldg_math_stat_tile_init(ldg_math_stat_tile_t *tile, uint32_t state_dimension, uint32_t tiling_cunt, const uint32_t *tile_cunt_arr, const double *low, const double *high, uint32_t weight_cap, uint32_t action_max);
LDG_EXPORT uint32_t ldg_math_stat_tile_feature_compute(const ldg_math_stat_tile_t *tile, const double *state, uint32_t action, uint32_t *idx_out);

typedef enum ldg_math_stat_vfa_strat
{
    LDG_MATH_STAT_VFA_STRAT_Q_LEARNING = 0,
    LDG_MATH_STAT_VFA_STRAT_SARSA,
    LDG_MATH_STAT_VFA_STRAT_Q_LAMBDA,
    LDG_MATH_STAT_VFA_STRAT_SARSA_LAMBDA,
    LDG_MATH_STAT_VFA_STRAT_CUNT
} ldg_math_stat_vfa_strat_t;

typedef struct ldg_math_stat_vfa
{
    ldg_math_stat_tile_t tile;
    double *weight;
    double *trace;
    ldg_math_stat_rng_cb_t rng_cb;
    void *rng_ctx;
    uint32_t weight_cap;
    uint32_t strat;
    double alpha;
    double gamma;
    double lambda;
    double epsilon;
    uint8_t last_greedy;
    uint8_t is_init;
    uint8_t pudding[6];
} ldg_math_stat_vfa_t;

LDG_EXPORT uint32_t ldg_math_stat_vfa_init(ldg_math_stat_vfa_t *vfa, const ldg_math_stat_tile_t *tile, uint32_t strat, double alpha, double gamma, double lambda, double epsilon, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx);
LDG_EXPORT uint32_t ldg_math_stat_vfa_shutdown(ldg_math_stat_vfa_t *vfa);
LDG_EXPORT uint32_t ldg_math_stat_vfa_action_sel(ldg_math_stat_vfa_t *vfa, const double *state, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_vfa_observe(ldg_math_stat_vfa_t *vfa, const double *state, uint32_t action, double reward, const double *next_state, uint32_t next_action, uint8_t terminal);
LDG_EXPORT uint32_t ldg_math_stat_vfa_q_get(const ldg_math_stat_vfa_t *vfa, const double *state, uint32_t action, double *out);
LDG_EXPORT uint32_t ldg_math_stat_vfa_action_best_get(const ldg_math_stat_vfa_t *vfa, const double *state, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_vfa_q_max_get(const ldg_math_stat_vfa_t *vfa, const double *state, double *out);
LDG_EXPORT uint32_t ldg_math_stat_vfa_rst(ldg_math_stat_vfa_t *vfa);

#define LDG_MATH_STAT_AUTOGRAD_NODE_NONE UINT32_MAX

typedef struct ldg_math_stat_autograd_node
{
    uintptr_t val_off;
    uintptr_t grad_off;
    uint32_t row_cunt;
    uint32_t col_cunt;
    uint8_t grad_is;
    uint8_t pudding[7];
} ldg_math_stat_autograd_node_t;

typedef struct ldg_math_stat_autograd_tape
{
    const void *ref;
    double scalar;
    uint32_t op;
    uint32_t out;
    uint32_t lhs;
    uint32_t rhs;
    uint32_t aux;
    uint8_t pudding[4];
} ldg_math_stat_autograd_tape_t;

typedef struct ldg_math_stat_autograd
{
    double *val;
    double *grad;
    ldg_math_stat_autograd_node_t *node;
    ldg_math_stat_autograd_tape_t *tape;
    uintptr_t elem_cap;
    uintptr_t elem_cunt;
    uintptr_t elem_mark;
    uint32_t node_cap;
    uint32_t node_cunt;
    uint32_t node_mark;
    uint32_t tape_cap;
    uint32_t tape_cunt;
    uint8_t is_init;
    uint8_t pudding[51];
} LDG_ALIGNED ldg_math_stat_autograd_t;

LDG_EXPORT uint32_t ldg_math_stat_autograd_init(ldg_math_stat_autograd_t *graph, uint32_t node_cap, uint32_t tape_cap, uintptr_t elem_cap);
LDG_EXPORT uint32_t ldg_math_stat_autograd_shutdown(ldg_math_stat_autograd_t *graph);
LDG_EXPORT uint32_t ldg_math_stat_autograd_param_mark_set(ldg_math_stat_autograd_t *graph);
LDG_EXPORT uint32_t ldg_math_stat_autograd_tape_rst(ldg_math_stat_autograd_t *graph);
LDG_EXPORT uint32_t ldg_math_stat_autograd_rst(ldg_math_stat_autograd_t *graph);
LDG_EXPORT uint32_t ldg_math_stat_autograd_grad_zero(ldg_math_stat_autograd_t *graph);
LDG_EXPORT uint32_t ldg_math_stat_autograd_tensor_push(ldg_math_stat_autograd_t *graph, uint32_t row_cunt, uint32_t col_cunt, uint8_t grad_is, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_val_set(ldg_math_stat_autograd_t *graph, uint32_t node, const double *src, uintptr_t cunt);
LDG_EXPORT uint32_t ldg_math_stat_autograd_val_get(const ldg_math_stat_autograd_t *graph, uint32_t node, double *out, uintptr_t cap);
LDG_EXPORT uint32_t ldg_math_stat_autograd_grad_get(const ldg_math_stat_autograd_t *graph, uint32_t node, double *out, uintptr_t cap);
LDG_EXPORT uint32_t ldg_math_stat_autograd_scalar_get(const ldg_math_stat_autograd_t *graph, uint32_t node, double *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_matmul_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_bias_add_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_row_mul_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_add_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_sub_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_mul_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_min_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_gather_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_scale_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, double scalar, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_shift_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, double scalar, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_clip_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, double eps, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_tanh_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_sigmoid_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_relu_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_exp_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_square_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_log_softmax_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, const uint8_t *mask, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_row_sum_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_mean_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_row_norm_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, double eps, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_row_rms_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, double eps, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_row_repeat_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rep_cunt, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_row_pool_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t group_len, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_attn_score_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t group_len, uint32_t hd_cunt, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_attn_mask_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t group_len, uint32_t hd_cunt, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_attn_apply_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t group_len, uint32_t hd_cunt, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_col_mul_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_col_concat_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_scalar_mul_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_synapse_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t idx, uint32_t out_dim, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_neuron_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t state, uint32_t tick_cunt, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_kwin_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t keep_cunt, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_conv_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t geom, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_group_bias_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_cosine_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, double eps, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_euclid_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_pool_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t geom, uint8_t max_is, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_embedding_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_gelu_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_reshape_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t row_cunt, uint32_t col_cunt, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_tile_push(ldg_math_stat_autograd_t *graph, uint32_t lhs, uint32_t rep_cunt, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_autograd_backward_run(ldg_math_stat_autograd_t *graph, uint32_t root);

#define LDG_MATH_STAT_BABE_FIXED_SHIFT 16U
#define LDG_MATH_STAT_BABE_FIXED_ONE (1 << LDG_MATH_STAT_BABE_FIXED_SHIFT)
#define LDG_MATH_STAT_BABE_CLASS_MAX 16
#define LDG_MATH_STAT_BABE_SUBSTANCE_MAX 16
#define LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT 2
#define LDG_MATH_STAT_BABE_UNIT_GRID 0
#define LDG_MATH_STAT_BABE_UNIT_INPUT 1
#define LDG_MATH_STAT_BABE_REGION_DIV 8
#define LDG_MATH_STAT_BABE_REGION_CUNT 64
#define LDG_MATH_STAT_BABE_DELAY_MAX 8
#define LDG_MATH_STAT_BABE_HIST_BITS 16
#define LDG_MATH_STAT_BABE_IN_CUNT_MAX 65535
#define LDG_MATH_STAT_BABE_READOUT_DIV 128
#define LDG_MATH_STAT_BABE_READOUT_STEP_CUNT 4
#define LDG_MATH_STAT_BABE_FIRE_NONE UINT32_MAX

typedef int32_t ldg_math_stat_babe_fixed_t;

typedef struct ldg_math_stat_babe_species
{
    ldg_math_stat_babe_fixed_t threshold;
    ldg_math_stat_babe_fixed_t rst_val;
    ldg_math_stat_babe_fixed_t adapt_step;
    ldg_math_stat_babe_fixed_t weight_init;
    ldg_math_stat_babe_fixed_t stdp_step;
    ldg_math_stat_babe_fixed_t lateral_inhibit_frac;
    ldg_math_stat_babe_fixed_t trace_floor;
    ldg_math_stat_babe_fixed_t receptor[LDG_MATH_STAT_BABE_CLASS_MAX][LDG_MATH_STAT_BABE_SUBSTANCE_MAX];
    ldg_math_stat_babe_fixed_t excite_threshold[LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT][LDG_MATH_STAT_BABE_SUBSTANCE_MAX];
    ldg_math_stat_babe_fixed_t excite_leak[LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT][LDG_MATH_STAT_BABE_SUBSTANCE_MAX];
    ldg_math_stat_babe_fixed_t cascade[LDG_MATH_STAT_BABE_SUBSTANCE_MAX][LDG_MATH_STAT_BABE_SUBSTANCE_MAX];
    ldg_math_stat_babe_fixed_t tolerance_mark[LDG_MATH_STAT_BABE_SUBSTANCE_MAX];
    uint32_t trace_shift[LDG_MATH_STAT_BABE_CLASS_MAX];
    uint32_t half_life_shift[LDG_MATH_STAT_BABE_SUBSTANCE_MAX];
    uint32_t class_cunt;
    uint32_t substance_cunt;
    uint32_t motor_class;
    uint32_t lateral_class;
    uint32_t leak_shift;
    uint32_t refrac_step_cunt;
    uint32_t adapt_shift;
    uint32_t stdp_window;
    uint32_t lr_shift;
    uint32_t delay_max;
    uint32_t lateral_radius;
    uint32_t diffuse_shift;
    uint32_t tolerance_down_shift;
    uint32_t tolerance_up_shift;
} ldg_math_stat_babe_species_t;

typedef struct ldg_math_stat_babe_conf
{
    const uint8_t *in_class;
    uint64_t seed;
    uint32_t grid_h;
    uint32_t grid_w;
    uint32_t in_cunt;
    uint32_t in_per_unit;
    uint32_t lateral_per_unit;
    uint8_t pudding[4];
} ldg_math_stat_babe_conf_t;

typedef struct ldg_math_stat_babe_synapse
{
    ldg_math_stat_babe_fixed_t weight;
    ldg_math_stat_babe_fixed_t trace;
    uint32_t dst;
    uint32_t delay;
    uint32_t kind;
    uint8_t inhibit_is;
    uint8_t pudding[3];
} ldg_math_stat_babe_synapse_t;

typedef struct ldg_math_stat_babe
{
    const ldg_math_stat_babe_species_t *species;
    ldg_math_stat_rng_cb_t rng_cb;
    void *rng_ctx;
    ldg_math_stat_babe_fixed_t *membrane;
    ldg_math_stat_babe_fixed_t *adapt;
    ldg_math_stat_babe_fixed_t *weight;
    ldg_math_stat_babe_fixed_t *trace;
    ldg_math_stat_babe_fixed_t *concentration;
    ldg_math_stat_babe_fixed_t *field_buff;
    ldg_math_stat_babe_fixed_t *fx;
    ldg_math_stat_babe_fixed_t *excite;
    ldg_math_stat_babe_fixed_t *readout;
    uint32_t *refrac;
    uint32_t *last_fire;
    uint16_t *in_lane;
    uint16_t *hist;
    uint16_t *stencil_pot;
    uint16_t *stencil_dep;
    int8_t *stencil_dx;
    int8_t *stencil_dy;
    uint8_t *stencil_delay;
    uint8_t *stencil_inhibit_is;
    uint8_t *fired_now;
    uint8_t *trace_is;
    uint8_t *region;
    uint8_t *in_class;
    uint8_t *spike_hist;
    ldg_math_stat_babe_fixed_t receptor[LDG_MATH_STAT_BABE_CLASS_MAX][LDG_MATH_STAT_BABE_SUBSTANCE_MAX];
    ldg_math_stat_babe_fixed_t excite_threshold[LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT][LDG_MATH_STAT_BABE_SUBSTANCE_MAX];
    ldg_math_stat_babe_fixed_t excite_leak[LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT][LDG_MATH_STAT_BABE_SUBSTANCE_MAX];
    uint64_t seed;
    uint32_t hash;
    uint32_t grid_h;
    uint32_t grid_w;
    uint32_t unit_cunt;
    uint32_t in_cunt;
    uint32_t synapse_cunt;
    uint32_t in_per_unit;
    uint32_t lateral_per_unit;
    uint32_t slot_cunt;
    uint32_t readout_cunt;
    uint32_t fired_cunt;
    uint32_t step_cunt;
    uint8_t is_init;
    uint8_t pudding[3];
} LDG_ALIGNED ldg_math_stat_babe_t;

typedef struct ldg_math_stat_babe_proto
{
    ldg_math_stat_babe_fixed_t *weight;
    ldg_math_stat_babe_fixed_t receptor[LDG_MATH_STAT_BABE_CLASS_MAX][LDG_MATH_STAT_BABE_SUBSTANCE_MAX];
    ldg_math_stat_babe_fixed_t excite_threshold[LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT][LDG_MATH_STAT_BABE_SUBSTANCE_MAX];
    ldg_math_stat_babe_fixed_t excite_leak[LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT][LDG_MATH_STAT_BABE_SUBSTANCE_MAX];
    uint32_t hash;
    uint32_t synapse_cunt;
    uint8_t is_init;
    uint8_t pudding[7];
} ldg_math_stat_babe_proto_t;

LDG_EXPORT uint32_t ldg_math_stat_babe_species_default_get(ldg_math_stat_babe_species_t *out);
LDG_EXPORT uint32_t ldg_math_stat_babe_conf_default_get(ldg_math_stat_babe_conf_t *out);
LDG_EXPORT uint32_t ldg_math_stat_babe_size_get(const ldg_math_stat_babe_species_t *species, const ldg_math_stat_babe_conf_t *conf, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_stat_babe_init(ldg_math_stat_babe_t *babe, const ldg_math_stat_babe_species_t *species, const ldg_math_stat_babe_conf_t *conf, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx);
LDG_EXPORT uint32_t ldg_math_stat_babe_shutdown(ldg_math_stat_babe_t *babe);
LDG_EXPORT uint32_t ldg_math_stat_babe_rst(ldg_math_stat_babe_t *babe);
LDG_EXPORT uint32_t ldg_math_stat_babe_hash_get(const ldg_math_stat_babe_t *babe, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_babe_step(ldg_math_stat_babe_t *babe, const ldg_math_stat_babe_fixed_t *src, ldg_math_stat_babe_fixed_t *dst, uint32_t dst_cap);
LDG_EXPORT uint32_t ldg_math_stat_babe_concentration_add(ldg_math_stat_babe_t *babe, uint32_t substance, uint32_t region, ldg_math_stat_babe_fixed_t amount);
LDG_EXPORT uint32_t ldg_math_stat_babe_concentration_get(const ldg_math_stat_babe_t *babe, uint32_t substance, uint32_t region, ldg_math_stat_babe_fixed_t *out);
LDG_EXPORT uint32_t ldg_math_stat_babe_receptor_get(const ldg_math_stat_babe_t *babe, uint32_t kind, uint32_t substance, ldg_math_stat_babe_fixed_t *out);
LDG_EXPORT uint32_t ldg_math_stat_babe_unit_get(const ldg_math_stat_babe_t *babe, uint32_t unit, ldg_math_stat_babe_fixed_t *out_membrane, ldg_math_stat_babe_fixed_t *out_adapt, uint32_t *out_refrac, uint32_t *out_last_fire);
LDG_EXPORT uint32_t ldg_math_stat_babe_unit_span_get(const ldg_math_stat_babe_t *babe, uint32_t unit, uint32_t *out_first, uint32_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_babe_synapse_get(const ldg_math_stat_babe_t *babe, uint32_t idx, ldg_math_stat_babe_synapse_t *out);
LDG_EXPORT uint32_t ldg_math_stat_babe_region_get(const ldg_math_stat_babe_t *babe, uint32_t unit, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_babe_state_cunt_get(const ldg_math_stat_babe_t *babe, uintptr_t *out);
LDG_EXPORT uint32_t ldg_math_stat_babe_state_get(const ldg_math_stat_babe_t *babe, ldg_math_stat_babe_fixed_t *dst, uintptr_t cap, uintptr_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_babe_state_set(ldg_math_stat_babe_t *babe, const ldg_math_stat_babe_fixed_t *src, uintptr_t cunt);
LDG_EXPORT uint32_t ldg_math_stat_babe_proto_init(ldg_math_stat_babe_proto_t *proto, const ldg_math_stat_babe_t *babe);
LDG_EXPORT uint32_t ldg_math_stat_babe_proto_shutdown(ldg_math_stat_babe_proto_t *proto);
LDG_EXPORT uint32_t ldg_math_stat_babe_proto_capture(ldg_math_stat_babe_proto_t *proto, const ldg_math_stat_babe_t *babe);
LDG_EXPORT uint32_t ldg_math_stat_babe_inherit(ldg_math_stat_babe_t *babe, const ldg_math_stat_babe_proto_t *proto, ldg_math_stat_babe_fixed_t variation);
LDG_EXPORT uint32_t ldg_math_stat_babe_delta_get(const ldg_math_stat_babe_t *babe, const ldg_math_stat_babe_proto_t *proto, uint32_t *idx_dst, ldg_math_stat_babe_fixed_t *val_dst, uint32_t cap, uint32_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_babe_delta_set(ldg_math_stat_babe_t *babe, const ldg_math_stat_babe_proto_t *proto, const uint32_t *idx_src, const ldg_math_stat_babe_fixed_t *val_src, uint32_t cunt);
LDG_EXPORT uint32_t ldg_math_stat_babe_proto_state_cunt_get(const ldg_math_stat_babe_proto_t *proto, uintptr_t *out);
LDG_EXPORT uint32_t ldg_math_stat_babe_proto_state_get(const ldg_math_stat_babe_proto_t *proto, ldg_math_stat_babe_fixed_t *dst, uintptr_t cap, uintptr_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_babe_proto_state_set(ldg_math_stat_babe_proto_t *proto, const ldg_math_stat_babe_fixed_t *src, uintptr_t cunt);

#define LDG_MATH_STAT_TRAIT_ALLELE_CUNT 2
#define LDG_MATH_STAT_TRAIT_ALLELE_A 0
#define LDG_MATH_STAT_TRAIT_ALLELE_B 1
#define LDG_MATH_STAT_TRAIT_ROW_NONE UINT32_MAX

typedef enum ldg_math_stat_trait_mix
{
    LDG_MATH_STAT_TRAIT_MIX_PICK = 0,
    LDG_MATH_STAT_TRAIT_MIX_MEAN,
    LDG_MATH_STAT_TRAIT_MIX_CROSSOVER,
    LDG_MATH_STAT_TRAIT_MIX_CUNT
} ldg_math_stat_trait_mix_t;

typedef enum ldg_math_stat_trait_dom
{
    LDG_MATH_STAT_TRAIT_DOM_MAX = 0,
    LDG_MATH_STAT_TRAIT_DOM_MIN,
    LDG_MATH_STAT_TRAIT_DOM_MEAN,
    LDG_MATH_STAT_TRAIT_DOM_CUNT
} ldg_math_stat_trait_dom_t;

typedef struct ldg_math_stat_trait_desc
{
    double min;
    double max;
    double mutate_scale;
    double mutate_rate;
    uint32_t mix;
    uint32_t dom;
} ldg_math_stat_trait_desc_t;

typedef struct ldg_math_stat_trait_conf
{
    uint32_t trait_cap;
    uint32_t row_cap;
} ldg_math_stat_trait_conf_t;

typedef struct ldg_math_stat_trait_table
{
    ldg_math_stat_trait_desc_t *desc;
    double *allele;
    uint32_t *free_next;
    uint8_t *alive;
    ldg_math_stat_rng_cb_t rng_cb;
    void *rng_ctx;
    uint32_t trait_cap;
    uint32_t trait_cunt;
    uint32_t row_cap;
    uint32_t row_cunt;
    uint32_t free_hd;
    uint8_t is_init;
    uint8_t pudding[3];
} ldg_math_stat_trait_table_t;

LDG_EXPORT uint32_t ldg_math_stat_trait_conf_default_get(ldg_math_stat_trait_conf_t *out);
LDG_EXPORT uint32_t ldg_math_stat_trait_desc_default_get(ldg_math_stat_trait_desc_t *out);
LDG_EXPORT uint32_t ldg_math_stat_trait_table_init(ldg_math_stat_trait_table_t *table, const ldg_math_stat_trait_conf_t *conf, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx);
LDG_EXPORT uint32_t ldg_math_stat_trait_table_shutdown(ldg_math_stat_trait_table_t *table);
LDG_EXPORT uint32_t ldg_math_stat_trait_add(ldg_math_stat_trait_table_t *table, const ldg_math_stat_trait_desc_t *desc, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_trait_desc_get(const ldg_math_stat_trait_table_t *table, uint32_t trait, ldg_math_stat_trait_desc_t *out);
LDG_EXPORT uint32_t ldg_math_stat_trait_cunt_get(const ldg_math_stat_trait_table_t *table, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_trait_row_alloc(ldg_math_stat_trait_table_t *table, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_trait_row_free(ldg_math_stat_trait_table_t *table, uint32_t row);
LDG_EXPORT uint8_t ldg_math_stat_trait_row_alive_is(const ldg_math_stat_trait_table_t *table, uint32_t row);
LDG_EXPORT uint32_t ldg_math_stat_trait_row_cunt_get(const ldg_math_stat_trait_table_t *table, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_trait_row_draw(ldg_math_stat_trait_table_t *table, uint32_t row);
LDG_EXPORT uint32_t ldg_math_stat_trait_row_cpy(ldg_math_stat_trait_table_t *table, uint32_t dst, uint32_t src);
LDG_EXPORT uint32_t ldg_math_stat_trait_row_mix(ldg_math_stat_trait_table_t *table, uint32_t child, uint32_t parent_a, uint32_t parent_b);
LDG_EXPORT uint32_t ldg_math_stat_trait_row_mutate(ldg_math_stat_trait_table_t *table, uint32_t row);
LDG_EXPORT uint32_t ldg_math_stat_trait_allele_get(const ldg_math_stat_trait_table_t *table, uint32_t row, uint32_t trait, uint32_t allele, double *out);
LDG_EXPORT uint32_t ldg_math_stat_trait_allele_set(ldg_math_stat_trait_table_t *table, uint32_t row, uint32_t trait, uint32_t allele, double val);
LDG_EXPORT uint32_t ldg_math_stat_trait_val_get(const ldg_math_stat_trait_table_t *table, uint32_t row, uint32_t trait, double *out);
LDG_EXPORT uint32_t ldg_math_stat_trait_lane_get(const ldg_math_stat_trait_table_t *table, uint32_t row, double *dst, uintptr_t cap, uintptr_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_trait_dist_get(const ldg_math_stat_trait_table_t *table, uint32_t row_a, uint32_t row_b, double *out);
LDG_EXPORT uint32_t ldg_math_stat_trait_stat_get(const ldg_math_stat_trait_table_t *table, uint32_t trait, double *out_mean, double *out_var);
LDG_EXPORT uint32_t ldg_math_stat_trait_state_cunt_get(const ldg_math_stat_trait_table_t *table, uintptr_t *out);
LDG_EXPORT uint32_t ldg_math_stat_trait_state_get(const ldg_math_stat_trait_table_t *table, double *dst, uintptr_t cap, uintptr_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_trait_state_set(ldg_math_stat_trait_table_t *table, const double *src, uintptr_t cunt);

#define LDG_MATH_STAT_LAYER_STATE_NODE_CUNT 2
#define LDG_MATH_STAT_LAYER_STRIDE_EVERY_STEP 1
#define LDG_MATH_STAT_NEURON_PARAM_CUNT 6
#define LDG_MATH_STAT_NEURON_STATE_CUNT 3
#define LDG_MATH_STAT_SYNAPSE_IDX_CUNT 2
#define LDG_MATH_STAT_CONV_GEOM_CUNT 8
#define LDG_MATH_STAT_POOL_GEOM_CUNT 6
#define LDG_MATH_STAT_SVM_OUT_PER_CLASS 2

typedef enum ldg_math_stat_layer_kind
{
    LDG_MATH_STAT_LAYER_DENSE = 0,
    LDG_MATH_STAT_LAYER_NORM,
    LDG_MATH_STAT_LAYER_GRU,
    LDG_MATH_STAT_LAYER_LSTM,
    LDG_MATH_STAT_LAYER_ATTN,
    LDG_MATH_STAT_LAYER_NEURON,
    LDG_MATH_STAT_LAYER_CONV,
    LDG_MATH_STAT_LAYER_LINREG,
    LDG_MATH_STAT_LAYER_SVM,
    LDG_MATH_STAT_LAYER_COSINE,
    LDG_MATH_STAT_LAYER_EUCLID,
    LDG_MATH_STAT_LAYER_POOL,
    LDG_MATH_STAT_LAYER_EMBEDDING,
    LDG_MATH_STAT_LAYER_DROPOUT,
    LDG_MATH_STAT_LAYER_TRANSFORMER,
    LDG_MATH_STAT_LAYER_SEQUENCE_POOL,
    LDG_MATH_STAT_LAYER_SEQUENCE_HD,
    LDG_MATH_STAT_LAYER_RMS_NORM,
    LDG_MATH_STAT_LAYER_KIND_CUNT
} ldg_math_stat_layer_kind_t;

typedef enum ldg_math_stat_neuron_kind
{
    LDG_MATH_STAT_NEURON_RATE = 0,
    LDG_MATH_STAT_NEURON_SPIKE,
    LDG_MATH_STAT_NEURON_KIND_CUNT
} ldg_math_stat_neuron_kind_t;

typedef struct ldg_math_stat_neuron_desc
{
    double bias;
    double leak;
    double threshold;
    double rst_val;
    uint32_t kind;
    uint32_t act;
    uint32_t refractory;
    uint8_t pudding[4];
} ldg_math_stat_neuron_desc_t;

typedef struct ldg_math_stat_synapse_desc
{
    double weight;
    uint32_t src;
    uint32_t dst;
    uint32_t delay;
    uint8_t plastic_is;
    uint8_t pudding[3];
} ldg_math_stat_synapse_desc_t;

typedef enum ldg_math_stat_layer_act
{
    LDG_MATH_STAT_LAYER_ACT_NONE = 0,
    LDG_MATH_STAT_LAYER_ACT_TANH,
    LDG_MATH_STAT_LAYER_ACT_SIGMOID,
    LDG_MATH_STAT_LAYER_ACT_RELU,
    LDG_MATH_STAT_LAYER_ACT_SOFTMAX,
    LDG_MATH_STAT_LAYER_ACT_GELU,
    LDG_MATH_STAT_LAYER_ACT_CUNT
} ldg_math_stat_layer_act_t;

typedef struct ldg_math_stat_layer_desc
{
    double trace_decay;
    double plastic_lr;
    double plastic_pos;
    double plastic_neg;
    double weight_min;
    double weight_max;
    double reward_decay;
    double keep_rate;
    uint32_t kind;
    uint32_t dim;
    uint32_t act;
    uint32_t stride;
    uint32_t entity_dim;
    uint32_t entity_cap;
    uint32_t hd_cunt;
    uint32_t synapse_cap;
    uint32_t tick_cunt;
    uint32_t active_cunt;
    uint32_t delay_max;
    uint32_t conv_h;
    uint32_t conv_w;
    uint32_t kernel_h;
    uint32_t kernel_w;
    uint32_t kernel_stride;
    uint32_t kernel_pad;
    uint32_t member_cunt;
    uint32_t sample_cap;
    uint32_t vocab_cunt;
    uint32_t sequence_len;
    uint32_t expand_dim;
    uint8_t concat_is;
    uint8_t qk_norm_is;
    uint8_t residual_is;
    uint8_t pool_max_is;
    uint8_t frozen_is;
    uint8_t pos_is;
    uint8_t pudding[2];
} ldg_math_stat_layer_desc_t;

typedef struct ldg_math_stat_layer
{
    ldg_math_stat_layer_desc_t desc;
    double *cuntr;
    double *host;
    uintptr_t elem_per_row;
    uintptr_t work_per_row;
    uint32_t in_dim;
    uint32_t out_dim;
    uint32_t param_off;
    uint32_t param_cunt;
    uint32_t state_off;
    uint32_t state_len;
    uint32_t node_per_step;
    uint32_t node_out;
    uint32_t node_aux;
    uint32_t node_state_in[LDG_MATH_STAT_LAYER_STATE_NODE_CUNT];
    uint32_t node_state_out[LDG_MATH_STAT_LAYER_STATE_NODE_CUNT];
    uint32_t neuron_off;
    uint32_t synapse_off;
    uint32_t synapse_cunt;
    double reward_mean;
} ldg_math_stat_layer_t;

typedef struct ldg_math_stat_model_conf
{
    uintptr_t extra_elem_cap;
    uintptr_t row_step_max;
    uint32_t in_dim;
    uint32_t row_max;
    uint32_t step_max;
    uint32_t layer_cap;
    uint32_t param_cap;
    uint32_t extra_node_cap;
    uint32_t neuron_cap;
    uint32_t synapse_cap;
} ldg_math_stat_model_conf_t;

typedef struct ldg_math_stat_model_param
{
    double init_val;
    uint32_t row_cunt;
    uint32_t col_cunt;
    uint32_t node;
    uint8_t pudding[4];
} ldg_math_stat_model_param_t;

typedef struct ldg_math_stat_model
{
    ldg_math_stat_autograd_t *graph;
    ldg_math_stat_rng_cb_t rng_cb;
    void *rng_ctx;
    ldg_math_stat_layer_t *layer;
    ldg_math_stat_model_param_t *param;
    ldg_math_stat_neuron_desc_t *neuron;
    ldg_math_stat_synapse_desc_t *synapse;
    uint32_t *param_node;
    double *work;
    double *cuntr;
    double *host;
    const uint8_t *out_mask;
    uintptr_t param_off;
    uintptr_t param_elem_cunt;
    uint32_t layer_cunt;
    uint32_t param_cunt;
    uint32_t param_node_cunt;
    uint32_t out_dim;
    uint32_t state_len;
    uint32_t node_in;
    uint32_t neuron_cunt;
    uint32_t synapse_cunt;
    ldg_math_stat_model_conf_t conf;
    uint8_t is_init;
    uint8_t is_built;
    uint8_t train_is;
    uint8_t live_is;
    uint8_t pudding[60];
} LDG_ALIGNED ldg_math_stat_model_t;

LDG_EXPORT uint32_t ldg_math_stat_model_conf_default_get(ldg_math_stat_model_conf_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_layer_desc_default_get(uint32_t kind, uint32_t dim, ldg_math_stat_layer_desc_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_init(ldg_math_stat_model_t *model, const ldg_math_stat_model_conf_t *conf, ldg_math_stat_autograd_t *graph, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx);
LDG_EXPORT uint32_t ldg_math_stat_model_shutdown(ldg_math_stat_model_t *model);
LDG_EXPORT uint32_t ldg_math_stat_model_layer_add(ldg_math_stat_model_t *model, const ldg_math_stat_layer_desc_t *desc, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_param_add(ldg_math_stat_model_t *model, uint32_t row_cunt, uint32_t col_cunt, double init_val, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_neuron_desc_default_get(uint32_t kind, ldg_math_stat_neuron_desc_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_neuron_set(ldg_math_stat_model_t *model, uint32_t layer, uint32_t idx, const ldg_math_stat_neuron_desc_t *desc);
LDG_EXPORT uint32_t ldg_math_stat_model_synapse_add(ldg_math_stat_model_t *model, uint32_t layer, const ldg_math_stat_synapse_desc_t *desc, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_synapse_get(const ldg_math_stat_model_t *model, uint32_t layer, uint32_t idx, ldg_math_stat_synapse_desc_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_tensor_get(const ldg_math_stat_model_t *model, uint32_t layer, uint32_t tensor, uintptr_t *out_off, uint32_t *out_row_cunt, uint32_t *out_col_cunt);
LDG_EXPORT uint32_t ldg_math_stat_model_reward_apply(ldg_math_stat_model_t *model, double reward, uint32_t row_cunt);
LDG_EXPORT uint32_t ldg_math_stat_model_cap_get(ldg_math_stat_model_t *model, uint32_t *out_node_cap, uintptr_t *out_elem_cap);
LDG_EXPORT uint32_t ldg_math_stat_model_build(ldg_math_stat_model_t *model);
LDG_EXPORT uint32_t ldg_math_stat_model_forward(ldg_math_stat_model_t *model, const double *src, uint32_t row_cunt, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_state_push(ldg_math_stat_model_t *model, const double *src, uint32_t row_cunt);
LDG_EXPORT uint32_t ldg_math_stat_model_state_get(const ldg_math_stat_model_t *model, double *dst, uint32_t row_cunt, uintptr_t cap);
LDG_EXPORT uint32_t ldg_math_stat_model_layer_cunt_get(const ldg_math_stat_model_t *model, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_layer_get(const ldg_math_stat_model_t *model, uint32_t layer, const ldg_math_stat_layer_t **out);
LDG_EXPORT uint32_t ldg_math_stat_model_param_node_get(const ldg_math_stat_model_t *model, uint32_t param, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_param_cunt_get(const ldg_math_stat_model_t *model, uintptr_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_params_get(const ldg_math_stat_model_t *model, double *dst, uintptr_t cap, uintptr_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_model_params_set(ldg_math_stat_model_t *model, const double *src, uintptr_t cunt);
LDG_EXPORT uint32_t ldg_math_stat_model_out_dim_get(const ldg_math_stat_model_t *model, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_state_len_get(const ldg_math_stat_model_t *model, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_linreg_observe(ldg_math_stat_model_t *model, uint32_t layer, const double *src, const double *tgt, uint32_t row_cunt);
LDG_EXPORT uint32_t ldg_math_stat_model_linreg_refit(ldg_math_stat_model_t *model, uint32_t layer, double ridge);
LDG_EXPORT uint32_t ldg_math_stat_model_linreg_rst(ldg_math_stat_model_t *model, uint32_t layer);
LDG_EXPORT uint32_t ldg_math_stat_model_svm_fit(ldg_math_stat_model_t *model, uint32_t layer, const double *src, const uint32_t *label, uint32_t row_cunt, double cost, uint32_t pass_cunt);
LDG_EXPORT uint32_t ldg_math_stat_model_svm_rst(ldg_math_stat_model_t *model, uint32_t layer);
LDG_EXPORT uint32_t ldg_math_stat_model_train_set(ldg_math_stat_model_t *model, uint8_t train_is);
LDG_EXPORT uint32_t ldg_math_stat_model_live_set(ldg_math_stat_model_t *model, uint8_t live_is);
LDG_EXPORT uint32_t ldg_math_stat_model_frozen_set(ldg_math_stat_model_t *model, uint32_t layer, uint8_t frozen_is);
LDG_EXPORT uint32_t ldg_math_stat_model_frozen_mask_set(const ldg_math_stat_model_t *model, uint8_t *mask, uintptr_t cap, uint32_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_model_sequence_loss_push(ldg_math_stat_model_t *model, uint32_t logp_node, const double *tgt, const double *mask, uint32_t row_cunt, uint32_t sequence_len, uint32_t vocab_cunt, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_model_out_mask_set(ldg_math_stat_model_t *model, const uint8_t *mask);

typedef struct ldg_math_stat_mask_conf
{
    double rate;
    double replace_frac;
    double random_frac;
    uint32_t mask_tok;
    uint32_t vocab_cunt;
} ldg_math_stat_mask_conf_t;

LDG_EXPORT uint32_t ldg_math_stat_mask_conf_default_get(ldg_math_stat_mask_conf_t *out);
LDG_EXPORT uint32_t ldg_math_stat_mask_tok_corrupt(const ldg_math_stat_mask_conf_t *conf, const double *src, uint32_t len, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double *dst_in, double *dst_tgt, double *dst_mask, uint32_t *out_cunt);

typedef struct ldg_math_stat_adam_conf
{
    double lr;
    double beta_mean;
    double beta_var;
    double eps;
    double grad_norm_max;
    double weight_decay;
} ldg_math_stat_adam_conf_t;

typedef struct ldg_math_stat_adam
{
    ldg_math_stat_adam_conf_t conf;
    double *mean;
    double *var;
    uint8_t *frozen;
    uintptr_t param_cunt;
    uint32_t step;
    uint32_t frozen_cunt;
    uint8_t is_init;
    uint8_t pudding[7];
} ldg_math_stat_adam_t;

LDG_EXPORT uint32_t ldg_math_stat_adam_conf_default_get(ldg_math_stat_adam_conf_t *out);
LDG_EXPORT uint32_t ldg_math_stat_adam_init(ldg_math_stat_adam_t *adam, const ldg_math_stat_adam_conf_t *conf, const ldg_math_stat_autograd_t *graph);
LDG_EXPORT uint32_t ldg_math_stat_adam_shutdown(ldg_math_stat_adam_t *adam);
LDG_EXPORT uint32_t ldg_math_stat_adam_rst(ldg_math_stat_adam_t *adam);
LDG_EXPORT uint32_t ldg_math_stat_adam_frozen_set(ldg_math_stat_adam_t *adam, const ldg_math_stat_model_t *model);
LDG_EXPORT uint32_t ldg_math_stat_adam_frozen_clear(ldg_math_stat_adam_t *adam);
LDG_EXPORT uint32_t ldg_math_stat_adam_lr_set(ldg_math_stat_adam_t *adam, double lr);
LDG_EXPORT uint32_t ldg_math_stat_adam_step(ldg_math_stat_adam_t *adam, ldg_math_stat_autograd_t *graph, double *out_grad_norm);
LDG_EXPORT uint32_t ldg_math_stat_adam_get(const ldg_math_stat_adam_t *adam, double *dst_mean, double *dst_var, uintptr_t cap, uintptr_t *out_cunt, uint32_t *out_step);
LDG_EXPORT uint32_t ldg_math_stat_adam_set(ldg_math_stat_adam_t *adam, const double *src_mean, const double *src_var, uintptr_t cunt, uint32_t step);
LDG_EXPORT uint32_t ldg_math_stat_adam_schedule_get(double lr_max, double final_frac, uint32_t warmup_cunt, uint32_t total_cunt, uint32_t step, double *out);
LDG_EXPORT uint32_t ldg_math_stat_model_param_redraw(ldg_math_stat_model_t *model);

#define LDG_MATH_STAT_PPO_LAYER_MAX 4
#define LDG_MATH_STAT_PPO_NET_ACTOR 0
#define LDG_MATH_STAT_PPO_NET_CRITIC 1
#define LDG_MATH_STAT_PPO_NET_CUNT 2
#define LDG_MATH_STAT_PPO_OPPONENT_MAX 4

typedef enum ldg_math_stat_ppo_opponent_strat
{
    LDG_MATH_STAT_PPO_OPPONENT_UNIFORM = 0,
    LDG_MATH_STAT_PPO_OPPONENT_LATEST,
    LDG_MATH_STAT_PPO_OPPONENT_QUALITY,
    LDG_MATH_STAT_PPO_OPPONENT_STRAT_CUNT
} ldg_math_stat_ppo_opponent_strat_t;

typedef enum ldg_math_stat_ppo_action_kind
{
    LDG_MATH_STAT_PPO_ACTION_DISCRETE = 0,
    LDG_MATH_STAT_PPO_ACTION_CONT,
    LDG_MATH_STAT_PPO_ACTION_CUNT
} ldg_math_stat_ppo_action_kind_t;

typedef struct ldg_math_stat_ppo_conf
{
    double gamma;
    double lambda;
    double clip;
    double lr;
    double val_coef;
    double entropy_coef;
    double grad_norm_max;
    double adam_beta_mean;
    double adam_beta_var;
    double adam_eps;
    double sil_threshold;
    double sil_weight;
    double norm_decay;
    double demo_weight;
    double opponent_lr;
    uint32_t obs_dim;
    uint32_t action_dim;
    uint32_t hidden_dim;
    uint32_t hidden_cunt;
    uint32_t step_cap;
    uint32_t batch_size;
    uint32_t epoch_cunt;
    uint32_t action_kind;
    uint32_t recur_dim;
    uint32_t seg_cap;
    uint32_t sil_seg_cap;
    uint32_t critic_obs_dim;
    uint32_t body_cap;
    uint32_t entity_dim;
    uint32_t entity_cap;
    uint32_t attn_dim;
    uint32_t hd_cunt;
    uint32_t demo_seg_cap;
    uint32_t neuron_cap;
    uint32_t synapse_cap;
    uint32_t snapshot_cap;
    uint32_t opponent_cap;
    uint32_t opponent_body_cap;
    uint8_t tgt_norm_is;
    uint8_t layer_norm_is;
    uint8_t act_keep_is;
    uint8_t pudding[1];
} ldg_math_stat_ppo_conf_t;

typedef struct ldg_math_stat_ppo_opponent
{
    ldg_math_stat_autograd_t graph;
    ldg_math_stat_model_t model;
    double *state;
    uint64_t tag;
    uint32_t snapshot;
    uint32_t log_std;
    uint8_t ld_is;
    uint8_t pudding[39];
} LDG_ALIGNED ldg_math_stat_ppo_opponent_t;

typedef struct ldg_math_stat_ppo_report
{
    double policy_loss;
    double val_loss;
    double entropy;
    double clip_frac;
    double kl;
    double grad_norm;
} ldg_math_stat_ppo_report_t;

typedef struct ldg_math_stat_ppo
{
    ldg_math_stat_autograd_t graph;
    ldg_math_stat_model_t actor;
    ldg_math_stat_model_t critic;
    ldg_math_stat_ppo_opponent_t opponent[LDG_MATH_STAT_PPO_OPPONENT_MAX];
    ldg_math_stat_rng_cb_t rng_cb;
    void *rng_ctx;
    double *snapshot;
    double *snapshot_quality;
    uint64_t *snapshot_tag;
    double *obs_buff;
    double *critic_obs_buff;
    double *action_buff;
    double *logp_buff;
    double *val_buff;
    double *reward_buff;
    double *adv_buff;
    double *tgt_buff;
    double *bootstrap_buff;
    double *adam_mean;
    double *adam_var;
    double *batch_buff;
    double *state_buff;
    double *state_curr;
    double *state_pend;
    double *body_carry;
    double *body_next;
    double *sil_obs;
    double *sil_critic_obs;
    double *sil_action;
    double *sil_tgt;
    double *sil_state;
    double *sil_prio;
    double *sil_step_buff;
    double *demo_obs;
    double *demo_critic_obs;
    double *demo_action;
    double *demo_weight;
    double *act_buff;
    uint32_t *demo_len;
    uint32_t *sil_len;
    uint32_t *idx_buff;
    uint32_t *seg_buff;
    uint32_t *seg_id_buff;
    uint32_t *seg_step_buff;
    uint32_t *body_buff;
    uint32_t *body_last;
    uint32_t *body_seg;
    uint8_t *terminal_buff;
    uint8_t *adam_frozen;
    uint8_t *action_mask_buff;
    uint8_t *batch_action_mask;
    uint8_t *sil_action_mask;
    uint8_t *demo_action_mask;
    double norm_mean;
    double norm_sq;
    uint64_t norm_cunt;
    uint64_t snapshot_seen;
    uintptr_t param_cunt;
    uint32_t snapshot_cunt;
    uint32_t adam_step;
    uint32_t log_std;
    uint32_t step_cunt;
    uint32_t sil_cunt;
    uint32_t demo_cunt;
    uint32_t demo_hd;
    uint32_t demo_open;
    uint32_t demo_open_len;
    uint32_t act_body;
    ldg_math_stat_ppo_conf_t conf;
    uint8_t is_init;
    uint8_t is_built;
    uint8_t act_is;
    uint8_t pudding[13];
} LDG_ALIGNED ldg_math_stat_ppo_t;

LDG_EXPORT uint32_t ldg_math_stat_ppo_conf_default_get(ldg_math_stat_ppo_conf_t *out);
LDG_EXPORT uint32_t ldg_math_stat_ppo_init(ldg_math_stat_ppo_t *ppo, const ldg_math_stat_ppo_conf_t *conf, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx);
LDG_EXPORT uint32_t ldg_math_stat_ppo_model_init(ldg_math_stat_ppo_t *ppo, const ldg_math_stat_ppo_conf_t *conf, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx);
LDG_EXPORT uint32_t ldg_math_stat_ppo_model_get(ldg_math_stat_ppo_t *ppo, uint32_t net, ldg_math_stat_model_t **out);
LDG_EXPORT uint32_t ldg_math_stat_ppo_build(ldg_math_stat_ppo_t *ppo);
LDG_EXPORT uint32_t ldg_math_stat_ppo_shutdown(ldg_math_stat_ppo_t *ppo);
LDG_EXPORT uint32_t ldg_math_stat_ppo_action_sel(ldg_math_stat_ppo_t *ppo, uint32_t body, const double *obs, const double *critic_obs, double *action_out, double *logp_out, double *val_out, const uint8_t *action_mask);
LDG_EXPORT uint32_t ldg_math_stat_ppo_observe(ldg_math_stat_ppo_t *ppo, uint32_t body, const double *obs, const double *critic_obs, const double *action, double logp, double val, double reward, uint8_t terminal, const uint8_t *action_mask);
LDG_EXPORT uint32_t ldg_math_stat_ppo_segment_end(ldg_math_stat_ppo_t *ppo, uint32_t body, double bootstrap_val);
LDG_EXPORT uint32_t ldg_math_stat_ppo_update(ldg_math_stat_ppo_t *ppo, double last_val, ldg_math_stat_ppo_report_t *out);
LDG_EXPORT uint32_t ldg_math_stat_ppo_val_get(ldg_math_stat_ppo_t *ppo, uint32_t body, const double *critic_obs, double *out);
LDG_EXPORT uint32_t ldg_math_stat_ppo_param_cunt_get(const ldg_math_stat_ppo_t *ppo, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_stat_ppo_params_get(const ldg_math_stat_ppo_t *ppo, double *dst, uint64_t dst_cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_params_set(ldg_math_stat_ppo_t *ppo, const double *src, uint64_t cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_adam_get(const ldg_math_stat_ppo_t *ppo, double *dst_mean, double *dst_var, uint64_t dst_cap, uint64_t *out_cunt, uint32_t *out_step);
LDG_EXPORT uint32_t ldg_math_stat_ppo_adam_set(ldg_math_stat_ppo_t *ppo, const double *src_mean, const double *src_var, uint64_t cunt, uint32_t step);
LDG_EXPORT uint32_t ldg_math_stat_ppo_step_cunt_get(const ldg_math_stat_ppo_t *ppo);
LDG_EXPORT uint32_t ldg_math_stat_ppo_rst(ldg_math_stat_ppo_t *ppo);
LDG_EXPORT uint32_t ldg_math_stat_ppo_reinit(ldg_math_stat_ppo_t *ppo);
LDG_EXPORT uint32_t ldg_math_stat_ppo_state_rst(ldg_math_stat_ppo_t *ppo, uint32_t body);
LDG_EXPORT uint32_t ldg_math_stat_ppo_state_get(const ldg_math_stat_ppo_t *ppo, uint32_t body, double *dst, uint64_t dst_cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_state_set(ldg_math_stat_ppo_t *ppo, uint32_t body, const double *src, uint64_t cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_act_layer_cunt_get(const ldg_math_stat_ppo_t *ppo, uint32_t *out_actor, uint32_t *out_critic);
LDG_EXPORT uint32_t ldg_math_stat_ppo_act_get(const ldg_math_stat_ppo_t *ppo, uint32_t body, uint32_t layer, double *dst, uint64_t dst_cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_attn_get(const ldg_math_stat_ppo_t *ppo, uint32_t body, uint32_t net, double *dst, uint64_t dst_cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_epoch_set(ldg_math_stat_ppo_t *ppo, uint32_t epoch_cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_entropy_set(ldg_math_stat_ppo_t *ppo, double entropy_coef);
LDG_EXPORT uint32_t ldg_math_stat_ppo_norm_get(const ldg_math_stat_ppo_t *ppo, double *out_mean, double *out_sq, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_norm_set(ldg_math_stat_ppo_t *ppo, double mean, double sq, uint64_t cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_sil_update(ldg_math_stat_ppo_t *ppo, uint32_t epoch_cunt, ldg_math_stat_ppo_report_t *out);
LDG_EXPORT uint32_t ldg_math_stat_ppo_sil_cunt_get(const ldg_math_stat_ppo_t *ppo, uint64_t *out_seg_cunt, uint64_t *out_seg_len);
LDG_EXPORT uint32_t ldg_math_stat_ppo_sil_get(const ldg_math_stat_ppo_t *ppo, double *dst, uint64_t dst_cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_sil_set(ldg_math_stat_ppo_t *ppo, const double *src, uint64_t cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_reward_add(ldg_math_stat_ppo_t *ppo, uint32_t body, double reward, uint32_t span);
LDG_EXPORT uint32_t ldg_math_stat_ppo_demo_add(ldg_math_stat_ppo_t *ppo, const double *obs, const double *critic_obs, const double *action, double weight, uint8_t terminal, const uint8_t *action_mask);
LDG_EXPORT uint32_t ldg_math_stat_ppo_demo_end(ldg_math_stat_ppo_t *ppo);
LDG_EXPORT uint32_t ldg_math_stat_ppo_demo_rst(ldg_math_stat_ppo_t *ppo);
LDG_EXPORT uint32_t ldg_math_stat_ppo_demo_update(ldg_math_stat_ppo_t *ppo, uint32_t epoch_cunt, ldg_math_stat_ppo_report_t *out);
LDG_EXPORT uint32_t ldg_math_stat_ppo_demo_cunt_get(const ldg_math_stat_ppo_t *ppo, uint64_t *out_seg_cunt, uint64_t *out_seg_len);
LDG_EXPORT uint32_t ldg_math_stat_ppo_demo_get(const ldg_math_stat_ppo_t *ppo, double *dst, uint64_t dst_cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_demo_set(ldg_math_stat_ppo_t *ppo, const double *src, uint64_t cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_snapshot_push(ldg_math_stat_ppo_t *ppo, uint32_t *out_slot);
LDG_EXPORT uint32_t ldg_math_stat_ppo_snapshot_cunt_get(const ldg_math_stat_ppo_t *ppo, uint64_t *out_cunt, uint64_t *out_len);
LDG_EXPORT uint32_t ldg_math_stat_ppo_snapshot_tag_get(const ldg_math_stat_ppo_t *ppo, uint32_t slot, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_stat_ppo_snapshot_get(const ldg_math_stat_ppo_t *ppo, double *dst, uint64_t dst_cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_snapshot_set(ldg_math_stat_ppo_t *ppo, const double *src, uint64_t cunt);
LDG_EXPORT uint32_t ldg_math_stat_ppo_snapshot_rst(ldg_math_stat_ppo_t *ppo);
LDG_EXPORT uint32_t ldg_math_stat_ppo_opponent_ld(ldg_math_stat_ppo_t *ppo, uint32_t opponent, uint32_t slot);
LDG_EXPORT uint32_t ldg_math_stat_ppo_opponent_sel(ldg_math_stat_ppo_t *ppo, uint32_t opponent, uint32_t strat, uint32_t *out_slot);
LDG_EXPORT uint32_t ldg_math_stat_ppo_opponent_tag_get(const ldg_math_stat_ppo_t *ppo, uint32_t opponent, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_stat_ppo_opponent_action_sel(ldg_math_stat_ppo_t *ppo, uint32_t opponent, uint32_t body, const double *obs, double *action_out, const uint8_t *action_mask);
LDG_EXPORT uint32_t ldg_math_stat_ppo_opponent_state_rst(ldg_math_stat_ppo_t *ppo, uint32_t opponent, uint32_t body);
LDG_EXPORT uint32_t ldg_math_stat_ppo_opponent_res_report(ldg_math_stat_ppo_t *ppo, uint32_t opponent, uint8_t win_is);
LDG_EXPORT uint32_t ldg_math_stat_ppo_snapshot_quality_get(const ldg_math_stat_ppo_t *ppo, uint32_t slot, double *out);

#define LDG_MATH_STAT_KMEANS_LABEL_NONE UINT32_MAX

typedef struct ldg_math_stat_kmeans_conf
{
    uint32_t row_cap;
    uint32_t dim;
    uint32_t cluster_cunt;
    uint32_t pass_max;
} ldg_math_stat_kmeans_conf_t;

typedef struct ldg_math_stat_kmeans_report
{
    double inertia;
    uint32_t pass_cunt;
    uint32_t reseed_cunt;
    uint8_t converged_is;
    uint8_t pudding[7];
} ldg_math_stat_kmeans_report_t;

typedef struct ldg_math_stat_kmeans
{
    double *centroid;
    double *cum;
    double *dist;
    uint32_t *member_cunt;
    ldg_math_stat_rng_cb_t rng_cb;
    void *rng_ctx;
    ldg_math_stat_kmeans_conf_t conf;
    uint8_t is_init;
    uint8_t is_fit;
    uint8_t pudding[6];
} ldg_math_stat_kmeans_t;

LDG_EXPORT uint32_t ldg_math_stat_kmeans_conf_default_get(ldg_math_stat_kmeans_conf_t *out);
LDG_EXPORT uint32_t ldg_math_stat_kmeans_init(ldg_math_stat_kmeans_t *kmeans, const ldg_math_stat_kmeans_conf_t *conf, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx);
LDG_EXPORT uint32_t ldg_math_stat_kmeans_shutdown(ldg_math_stat_kmeans_t *kmeans);
LDG_EXPORT uint32_t ldg_math_stat_kmeans_rst(ldg_math_stat_kmeans_t *kmeans);
LDG_EXPORT uint32_t ldg_math_stat_kmeans_fit(ldg_math_stat_kmeans_t *kmeans, const double *point, uint32_t row_cunt, uint32_t *label, ldg_math_stat_kmeans_report_t *out);
LDG_EXPORT uint32_t ldg_math_stat_kmeans_label_get(const ldg_math_stat_kmeans_t *kmeans, const double *point, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_kmeans_centroid_get(const ldg_math_stat_kmeans_t *kmeans, uint32_t cluster, double *dst, uint32_t cap);
LDG_EXPORT uint32_t ldg_math_stat_kmeans_member_cunt_get(const ldg_math_stat_kmeans_t *kmeans, uint32_t cluster, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_kmeans_state_cunt_get(const ldg_math_stat_kmeans_t *kmeans, uintptr_t *out);
LDG_EXPORT uint32_t ldg_math_stat_kmeans_state_get(const ldg_math_stat_kmeans_t *kmeans, double *dst, uintptr_t cap, uintptr_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_kmeans_state_set(ldg_math_stat_kmeans_t *kmeans, const double *src, uintptr_t cunt);

#define LDG_MATH_STAT_LINKAGE_ROW_MAX (1U << 30)
#define LDG_MATH_STAT_LINKAGE_NODE_NONE UINT32_MAX

typedef struct ldg_math_stat_linkage_conf
{
    uint32_t row_cap;
    uint32_t dim;
} ldg_math_stat_linkage_conf_t;

typedef struct ldg_math_stat_linkage_merge
{
    double height;
    uint32_t lhs;
    uint32_t rhs;
    uint32_t size;
    uint8_t pudding[4];
} ldg_math_stat_linkage_merge_t;

typedef struct ldg_math_stat_linkage
{
    ldg_math_stat_linkage_merge_t *merge;
    double *centroid;
    uint32_t *member_cunt;
    uint32_t *chain;
    uint32_t *parent;
    uint32_t *map;
    ldg_math_stat_linkage_conf_t conf;
    uint32_t row_cunt;
    uint32_t merge_cunt;
    uint8_t is_init;
    uint8_t is_built;
    uint8_t pudding[6];
} ldg_math_stat_linkage_t;

LDG_EXPORT uint32_t ldg_math_stat_linkage_conf_default_get(ldg_math_stat_linkage_conf_t *out);
LDG_EXPORT uint32_t ldg_math_stat_linkage_init(ldg_math_stat_linkage_t *linkage, const ldg_math_stat_linkage_conf_t *conf);
LDG_EXPORT uint32_t ldg_math_stat_linkage_shutdown(ldg_math_stat_linkage_t *linkage);
LDG_EXPORT uint32_t ldg_math_stat_linkage_rst(ldg_math_stat_linkage_t *linkage);
LDG_EXPORT uint32_t ldg_math_stat_linkage_build(ldg_math_stat_linkage_t *linkage, const double *point, uint32_t row_cunt);
LDG_EXPORT uint32_t ldg_math_stat_linkage_merge_cunt_get(const ldg_math_stat_linkage_t *linkage, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_linkage_merge_get(const ldg_math_stat_linkage_t *linkage, uint32_t idx, ldg_math_stat_linkage_merge_t *out);
LDG_EXPORT uint32_t ldg_math_stat_linkage_cunt_cut(ldg_math_stat_linkage_t *linkage, uint32_t cluster_cunt, uint32_t *label, uint32_t cap);
LDG_EXPORT uint32_t ldg_math_stat_linkage_height_cut(ldg_math_stat_linkage_t *linkage, double height, uint32_t *label, uint32_t cap, uint32_t *out_cunt);

typedef struct ldg_math_stat_pca_conf
{
    uint32_t dim;
    uint32_t comp_cunt;
} ldg_math_stat_pca_conf_t;

typedef struct ldg_math_stat_pca
{
    ldg_math_eigen_ctx_t *eigen;
    ldg_math_complex_t *mat;
    ldg_math_complex_t *vec;
    double *val;
    double *mean;
    double *basis;
    double *var;
    uint32_t *order;
    double var_total;
    ldg_math_stat_pca_conf_t conf;
    uint8_t is_init;
    uint8_t is_fit;
    uint8_t pudding[6];
} ldg_math_stat_pca_t;

LDG_EXPORT uint32_t ldg_math_stat_pca_conf_default_get(ldg_math_stat_pca_conf_t *out);
LDG_EXPORT uint32_t ldg_math_stat_pca_init(ldg_math_stat_pca_t *pca, const ldg_math_stat_pca_conf_t *conf);
LDG_EXPORT uint32_t ldg_math_stat_pca_shutdown(ldg_math_stat_pca_t *pca);
LDG_EXPORT uint32_t ldg_math_stat_pca_rst(ldg_math_stat_pca_t *pca);
LDG_EXPORT uint32_t ldg_math_stat_pca_fit(ldg_math_stat_pca_t *pca, const double *point, uint32_t row_cunt);
LDG_EXPORT uint32_t ldg_math_stat_pca_point_proj(const ldg_math_stat_pca_t *pca, const double *src, double *dst, uint32_t cap);
LDG_EXPORT uint32_t ldg_math_stat_pca_point_reconstruct(const ldg_math_stat_pca_t *pca, const double *src, double *dst, uint32_t cap);
LDG_EXPORT uint32_t ldg_math_stat_pca_comp_get(const ldg_math_stat_pca_t *pca, uint32_t comp, double *dst, uint32_t cap);
LDG_EXPORT uint32_t ldg_math_stat_pca_mean_get(const ldg_math_stat_pca_t *pca, double *dst, uint32_t cap);
LDG_EXPORT uint32_t ldg_math_stat_pca_var_get(const ldg_math_stat_pca_t *pca, uint32_t comp, double *out);
LDG_EXPORT uint32_t ldg_math_stat_pca_var_total_get(const ldg_math_stat_pca_t *pca, double *out);
LDG_EXPORT uint32_t ldg_math_stat_pca_state_cunt_get(const ldg_math_stat_pca_t *pca, uintptr_t *out);
LDG_EXPORT uint32_t ldg_math_stat_pca_state_get(const ldg_math_stat_pca_t *pca, double *dst, uintptr_t cap, uintptr_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_pca_state_set(ldg_math_stat_pca_t *pca, const double *src, uintptr_t cunt);

typedef struct ldg_math_stat_mds_conf
{
    uint32_t row_cunt;
    uint32_t out_dim;
} ldg_math_stat_mds_conf_t;

typedef struct ldg_math_stat_mds
{
    ldg_math_eigen_ctx_t *eigen;
    ldg_math_complex_t *mat;
    ldg_math_complex_t *vec;
    double *val;
    double *coord;
    double *proj;
    double *square_mean;
    double *eig;
    uint32_t *order;
    ldg_math_stat_mds_conf_t conf;
    uint32_t rank;
    uint8_t is_init;
    uint8_t is_fit;
    uint8_t pudding[2];
} ldg_math_stat_mds_t;

LDG_EXPORT uint32_t ldg_math_stat_mds_conf_default_get(ldg_math_stat_mds_conf_t *out);
LDG_EXPORT uint32_t ldg_math_stat_mds_init(ldg_math_stat_mds_t *mds, const ldg_math_stat_mds_conf_t *conf);
LDG_EXPORT uint32_t ldg_math_stat_mds_shutdown(ldg_math_stat_mds_t *mds);
LDG_EXPORT uint32_t ldg_math_stat_mds_rst(ldg_math_stat_mds_t *mds);
LDG_EXPORT uint32_t ldg_math_stat_mds_fit(ldg_math_stat_mds_t *mds, const double *dist);
LDG_EXPORT uint32_t ldg_math_stat_mds_coord_get(const ldg_math_stat_mds_t *mds, uint32_t row, double *dst, uint32_t cap);
LDG_EXPORT uint32_t ldg_math_stat_mds_val_get(const ldg_math_stat_mds_t *mds, uint32_t axis, double *out);
LDG_EXPORT uint32_t ldg_math_stat_mds_rank_get(const ldg_math_stat_mds_t *mds, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_mds_point_proj(const ldg_math_stat_mds_t *mds, const double *dist, double *dst, uint32_t cap);
LDG_EXPORT uint32_t ldg_math_stat_mds_state_cunt_get(const ldg_math_stat_mds_t *mds, uintptr_t *out);
LDG_EXPORT uint32_t ldg_math_stat_mds_state_get(const ldg_math_stat_mds_t *mds, double *dst, uintptr_t cap, uintptr_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_stat_mds_state_set(ldg_math_stat_mds_t *mds, const double *src, uintptr_t cunt);

typedef enum ldg_math_stat_metric
{
    LDG_MATH_STAT_METRIC_EUCLID = 0,
    LDG_MATH_STAT_METRIC_COSINE,
    LDG_MATH_STAT_METRIC_CUNT
} ldg_math_stat_metric_t;

LDG_EXPORT uint32_t ldg_math_stat_cosine_get(const double *lhs, const double *rhs, uint32_t dim, double *out);
#define LDG_MATH_STAT_DIRICHLET_CATEGORY_MAX 64

typedef struct ldg_math_stat_dirichlet
{
    uint64_t obs_cunt[LDG_MATH_STAT_DIRICHLET_CATEGORY_MAX];
    const double *prior;
    uint64_t total_obs;
    uint32_t category_cunt;
    uint8_t is_init;
    uint8_t pudding[3];
} ldg_math_stat_dirichlet_t;

LDG_EXPORT uint32_t ldg_math_stat_dirichlet_init(ldg_math_stat_dirichlet_t *dirichlet, uint32_t category_cunt, const double *prior);
LDG_EXPORT uint32_t ldg_math_stat_dirichlet_shutdown(ldg_math_stat_dirichlet_t *dirichlet);
LDG_EXPORT uint32_t ldg_math_stat_dirichlet_observe(ldg_math_stat_dirichlet_t *dirichlet, uint32_t category);
LDG_EXPORT uint32_t ldg_math_stat_dirichlet_mean_get(const ldg_math_stat_dirichlet_t *dirichlet, double *dst, uint32_t cap);
LDG_EXPORT uint32_t ldg_math_stat_dirichlet_predictive_get(const ldg_math_stat_dirichlet_t *dirichlet, uint32_t category, double *out);
LDG_EXPORT uint64_t ldg_math_stat_dirichlet_obs_cunt_get(const ldg_math_stat_dirichlet_t *dirichlet, uint32_t category);
LDG_EXPORT uint32_t ldg_math_stat_dirichlet_rst(ldg_math_stat_dirichlet_t *dirichlet);

LDG_EXPORT uint32_t ldg_math_stat_spearman_get(const double *arr_a, const double *arr_b, uint32_t cunt, double *buff, uintptr_t buff_cunt, uint32_t *order, uint32_t order_cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_kappa_get(const uint32_t *label_a, const uint32_t *label_b, uint32_t cunt, uint32_t class_cunt, uint32_t *buff, uint32_t buff_cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_entropy_get(const double *weight, uint32_t cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_divergence_get(const double *lhs, const double *rhs, uint32_t cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_mutual_info_get(const double *joint, uint32_t row_cunt, uint32_t col_cunt, double *out);

typedef struct ldg_math_stat_test
{
    double stat;
    double p_val;
    double shift;
    uint32_t cunt;
    uint8_t pudding[4];
} ldg_math_stat_test_t;

typedef uint32_t (*ldg_math_stat_estimate_cb_t)(const uint32_t *idx, uint32_t cunt, void *ctx, double *out);

LDG_EXPORT uint32_t ldg_math_stat_paired_t_test(const double *arr_a, const double *arr_b, uint32_t cunt, ldg_math_stat_test_t *out);
LDG_EXPORT uint32_t ldg_math_stat_signed_rank_test(const double *arr_a, const double *arr_b, uint32_t cunt, double *buff, uintptr_t buff_cunt, uint32_t *order, uint32_t order_cunt, ldg_math_stat_test_t *out);
LDG_EXPORT uint32_t ldg_math_stat_permutation_test(const double *arr_a, const double *arr_b, uint32_t cunt, uint32_t round_cunt, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, ldg_math_stat_test_t *out);
LDG_EXPORT uint32_t ldg_math_stat_bootstrap_interval_get(uint32_t cunt, uint32_t round_cunt, double level, ldg_math_stat_estimate_cb_t estimate_cb, void *estimate_ctx, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, uint32_t *idx_buff, uint32_t idx_cunt, double *rep_buff, uint32_t rep_cunt, double *out_lower, double *out_upper);

typedef struct ldg_math_stat_score
{
    double precision;
    double recall;
    double f1;
    uint64_t support;
} ldg_math_stat_score_t;

LDG_EXPORT uint32_t ldg_math_stat_confusion_cum(const uint32_t *truth, const uint32_t *predicted, uint32_t cunt, uint32_t class_cunt, uint64_t *mat);
LDG_EXPORT uint32_t ldg_math_stat_confusion_score_get(const uint64_t *mat, uint32_t class_cunt, uint32_t label, ldg_math_stat_score_t *out);
LDG_EXPORT uint32_t ldg_math_stat_confusion_accuracy_get(const uint64_t *mat, uint32_t class_cunt, double *out);
LDG_EXPORT uint32_t ldg_math_stat_roc_area_get(const double *score, const uint8_t *label, uint32_t cunt, uint32_t *order, uint32_t order_cunt, double *out);

typedef enum ldg_math_stat_cusum_side
{
    LDG_MATH_STAT_CUSUM_SIDE_NONE = 0,
    LDG_MATH_STAT_CUSUM_SIDE_UPPER,
    LDG_MATH_STAT_CUSUM_SIDE_LOWER,
    LDG_MATH_STAT_CUSUM_SIDE_BOTH
} ldg_math_stat_cusum_side_t;

typedef struct ldg_math_stat_cusum
{
    double tgt;
    double slack;
    double threshold;
    double upper;
    double lower;
    uint64_t upper_run;
    uint64_t lower_run;
    uint64_t obs_cunt;
    uint8_t is_init;
    uint8_t pudding[7];
} ldg_math_stat_cusum_t;

LDG_EXPORT uint32_t ldg_math_stat_cusum_init(ldg_math_stat_cusum_t *cusum, double tgt, double slack, double threshold);
LDG_EXPORT uint32_t ldg_math_stat_cusum_observe(ldg_math_stat_cusum_t *cusum, double val, uint32_t *out_side);
LDG_EXPORT uint32_t ldg_math_stat_cusum_level_get(const ldg_math_stat_cusum_t *cusum, uint32_t side, double *out);
LDG_EXPORT uint32_t ldg_math_stat_cusum_run_get(const ldg_math_stat_cusum_t *cusum, uint32_t side, uint64_t *out);
LDG_EXPORT uint64_t ldg_math_stat_cusum_obs_cunt_get(const ldg_math_stat_cusum_t *cusum);
LDG_EXPORT uint32_t ldg_math_stat_cusum_rst(ldg_math_stat_cusum_t *cusum);

typedef struct ldg_math_stat_platt
{
    double slope;
    double offset;
    double loss;
    uint32_t pass_cunt;
    uint8_t converged_is;
    uint8_t pudding[3];
} ldg_math_stat_platt_t;

typedef struct ldg_math_stat_temperature
{
    double temperature;
    double loss;
    uint32_t pass_cunt;
    uint8_t converged_is;
    uint8_t pudding[3];
} ldg_math_stat_temperature_t;

LDG_EXPORT uint32_t ldg_math_stat_platt_fit(const double *score, const uint8_t *label, uint32_t cunt, ldg_math_stat_platt_t *out);
LDG_EXPORT uint32_t ldg_math_stat_platt_apply(const ldg_math_stat_platt_t *fit, double score, double *out);
LDG_EXPORT uint32_t ldg_math_stat_temperature_fit(const double *logit, const uint32_t *label, uint32_t row_cunt, uint32_t class_cunt, ldg_math_stat_temperature_t *out);
LDG_EXPORT uint32_t ldg_math_stat_temperature_apply(const ldg_math_stat_temperature_t *fit, const double *logit, uint32_t class_cunt, double *dst, uint32_t cap);

#define LDG_MATH_STAT_SENTENCEPIECE_BYTE_CUNT 256

typedef struct ldg_math_stat_sentencepiece
{
    uint8_t *blob;
    uint32_t *piece_off;
    double *score;
    uint8_t *kind;
    uint32_t *slot;
    uint32_t piece_cunt;
    uint32_t slot_cunt;
    uint32_t byte_base;
    uint32_t byte_cunt;
    uint32_t unk_id;
    uint16_t user_first[LDG_MATH_STAT_SENTENCEPIECE_BYTE_CUNT];
    uint8_t byte_fallback_is;
    uint8_t is_init;
    uint8_t pudding[2];
} ldg_math_stat_sentencepiece_t;

LDG_EXPORT uint32_t ldg_math_stat_sentencepiece_init(ldg_math_stat_sentencepiece_t *model, const uint8_t *data, uint64_t len);
LDG_EXPORT uint32_t ldg_math_stat_sentencepiece_shutdown(ldg_math_stat_sentencepiece_t *model);
LDG_EXPORT uint32_t ldg_math_stat_sentencepiece_buff_cunt_get(uint32_t len, uintptr_t *out);
LDG_EXPORT uint32_t ldg_math_stat_sentencepiece_piece_cunt_get(const ldg_math_stat_sentencepiece_t *model, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_sentencepiece_piece_id_get(const ldg_math_stat_sentencepiece_t *model, const uint8_t *piece, uint32_t len, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_stat_sentencepiece_encode(const ldg_math_stat_sentencepiece_t *model, const uint8_t *text, uint32_t len, uint32_t *buff, uintptr_t buff_cunt, uint32_t *dst, uint32_t cap, uint32_t *out_cunt);

typedef struct ldg_math_stat_density_axis
{
    double lower;
    double upper;
    double bandwidth;
    uint32_t grid_cunt;
    uint8_t pudding[4];
} ldg_math_stat_density_axis_t;

LDG_EXPORT uint32_t ldg_math_stat_density_1d_fill(const double *val, uint32_t cunt, const ldg_math_stat_density_axis_t *axis, double *dst, uint32_t cap);
LDG_EXPORT uint32_t ldg_math_stat_density_2d_fill(const double *point, uint32_t cunt, const ldg_math_stat_density_axis_t *axis_x, const ldg_math_stat_density_axis_t *axis_y, double *buff, uint32_t buff_cunt, double *dst, uintptr_t cap);

LDG_EXPORT uint32_t ldg_math_stat_markov_buff_cunt_get(uint32_t state_cunt, uintptr_t *out);
LDG_EXPORT uint32_t ldg_math_stat_markov_cunt_cum(const uint32_t *sequence, uint32_t len, uint32_t state_cunt, uint64_t *cunt);
LDG_EXPORT uint32_t ldg_math_stat_markov_transition_fill(const uint64_t *cunt, uint32_t state_cunt, double pseudo, double *dst);
LDG_EXPORT uint32_t ldg_math_stat_markov_stationary_get(const double *transition, uint32_t state_cunt, double *buff, uintptr_t buff_cunt, double *dst);
LDG_EXPORT uint32_t ldg_math_stat_markov_hitting_get(const double *transition, uint32_t state_cunt, uint32_t tgt, double *buff, uintptr_t buff_cunt, double *dst);

LDG_EXPORT uint32_t ldg_math_stat_nearest_get(const double *point, uint32_t row_cunt, uint32_t dim, const double *query, uint32_t metric, uint32_t *idx_dst, double *dist_dst, uint32_t cap, uint32_t *out_cunt);

#endif
