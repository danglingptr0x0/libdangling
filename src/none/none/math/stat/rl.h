#ifndef LDG_MATH_STAT_RL_H
#define LDG_MATH_STAT_RL_H

#include <dangling/math/stat.h>

uint32_t ldg_math_stat_rng_uniform_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double *out);
uint32_t ldg_math_stat_rng_normal_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double *out);
uint32_t ldg_math_stat_rng_gamma_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double shape, double *out);
uint32_t ldg_math_stat_rng_beta_sample(ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double alpha, double beta, double *out);

#endif
