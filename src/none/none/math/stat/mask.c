#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

#include "rl.h"

#define MASK_RATE_DEFAULT 0.15
#define MASK_REPLACE_FRAC_DEFAULT 0.8
#define MASK_RANDOM_FRAC_DEFAULT 0.1
#define MASK_TOK_DEFAULT 0
#define MASK_VOCAB_CUNT_DEFAULT 0
#define MASK_FRAC_MIN 0.0
#define MASK_FRAC_MAX 1.0
#define MASK_SEL_ON 1.0
#define MASK_SEL_OFF 0.0

uint32_t ldg_math_stat_mask_conf_default_get(ldg_math_stat_mask_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->rate = MASK_RATE_DEFAULT;
    out->replace_frac = MASK_REPLACE_FRAC_DEFAULT;
    out->random_frac = MASK_RANDOM_FRAC_DEFAULT;
    out->mask_tok = MASK_TOK_DEFAULT;
    out->vocab_cunt = MASK_VOCAB_CUNT_DEFAULT;

    return LDG_ERR_AOK;
}

static uint32_t mask_conf_ck(const ldg_math_stat_mask_conf_t *conf)
{
    if (LDG_UNLIKELY(conf->vocab_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->rate < MASK_FRAC_MIN || conf->rate > MASK_FRAC_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->replace_frac < MASK_FRAC_MIN || conf->random_frac < MASK_FRAC_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->replace_frac + conf->random_frac > MASK_FRAC_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_mask_tok_corrupt(const ldg_math_stat_mask_conf_t *conf, const double *src, uint32_t len, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, double *dst_in, double *dst_tgt, double *dst_mask, uint32_t *out_cunt)
{
    double sel = 0.0;
    double kind = 0.0;
    double orig = 0.0;
    uint32_t cunt = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!conf || !src || !rng_cb || !dst_in || !dst_tgt || !dst_mask || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = mask_conf_ck(conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (idx < len)
    {
        orig = src[idx];
        dst_in[idx] = orig;
        dst_tgt[idx] = orig;
        dst_mask[idx] = MASK_SEL_OFF;

        ret = ldg_math_stat_rng_uniform_sample(rng_cb, rng_ctx, &sel);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (sel < conf->rate)
        {
            dst_mask[idx] = MASK_SEL_ON;
            cunt++;

            ret = ldg_math_stat_rng_uniform_sample(rng_cb, rng_ctx, &kind);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (kind < conf->replace_frac) { dst_in[idx] = (double)conf->mask_tok; }
            else if (kind < conf->replace_frac + conf->random_frac) { dst_in[idx] = (double)(rng_cb(rng_ctx) % (uint64_t)conf->vocab_cunt); }
        }

        idx++;
    }

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}
