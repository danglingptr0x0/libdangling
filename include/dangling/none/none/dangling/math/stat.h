#ifndef LDG_MATH_STAT_H
#define LDG_MATH_STAT_H

#include <stdint.h>

#include <dangling/core/macros.h>
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
    uint8_t pudding[31];
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
    uint8_t pudding[3];
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
    uint32_t state_max;
    uint32_t action_max;
    uint32_t strat;
    double alpha;
    double gamma;
    double lambda;
    double epsilon;
    double q_init;
    uint8_t last_greedy;
    uint8_t is_init;
    uint8_t pudding[6];
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
    uint8_t pudding[3];
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

#endif
