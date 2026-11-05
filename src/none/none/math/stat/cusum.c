#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>
#include <dangling/mem/cpy.h>

#define CUSUM_NONE 0.0

static uint32_t cusum_init_ck(const ldg_math_stat_cusum_t *cusum)
{
    if (LDG_UNLIKELY(!cusum)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cusum->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

static void cusum_side_step(double *level, uint64_t *run, double gain)
{
    *level += gain;

    if (*level > CUSUM_NONE)
    {
        (*run)++;
        return;
    }

    *level = CUSUM_NONE;
    *run = 0;
}

uint32_t ldg_math_stat_cusum_init(ldg_math_stat_cusum_t *cusum, double tgt, double slack, double threshold)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!cusum)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_math_finite_is(tgt) != LDG_TRUTH_TRUE || ldg_math_finite_is(slack) != LDG_TRUTH_TRUE || ldg_math_finite_is(threshold) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(slack < CUSUM_NONE || threshold <= CUSUM_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_zero(cusum, sizeof(*cusum));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cusum->tgt = tgt;
    cusum->slack = slack;
    cusum->threshold = threshold;
    cusum->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_cusum_observe(ldg_math_stat_cusum_t *cusum, double val, uint32_t *out_side)
{
    uint32_t side = LDG_MATH_STAT_CUSUM_SIDE_NONE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!cusum || !out_side)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_side = LDG_MATH_STAT_CUSUM_SIDE_NONE;

    ret = cusum_init_ck(cusum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ldg_math_finite_is(val) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cusum_side_step(&cusum->upper, &cusum->upper_run, (val - cusum->tgt) - cusum->slack);
    cusum_side_step(&cusum->lower, &cusum->lower_run, (cusum->tgt - val) - cusum->slack);
    cusum->obs_cunt++;

    if (cusum->upper > cusum->threshold) { side |= LDG_MATH_STAT_CUSUM_SIDE_UPPER; }

    if (cusum->lower > cusum->threshold) { side |= LDG_MATH_STAT_CUSUM_SIDE_LOWER; }

    *out_side = side;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_cusum_level_get(const ldg_math_stat_cusum_t *cusum, uint32_t side, double *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!cusum || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = CUSUM_NONE;

    ret = cusum_init_ck(cusum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(side != LDG_MATH_STAT_CUSUM_SIDE_UPPER && side != LDG_MATH_STAT_CUSUM_SIDE_LOWER)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = (side == LDG_MATH_STAT_CUSUM_SIDE_UPPER) ? cusum->upper : cusum->lower;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_cusum_run_get(const ldg_math_stat_cusum_t *cusum, uint32_t side, uint64_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!cusum || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = cusum_init_ck(cusum);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(side != LDG_MATH_STAT_CUSUM_SIDE_UPPER && side != LDG_MATH_STAT_CUSUM_SIDE_LOWER)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = (side == LDG_MATH_STAT_CUSUM_SIDE_UPPER) ? cusum->upper_run : cusum->lower_run;

    return LDG_ERR_AOK;
}

uint64_t ldg_math_stat_cusum_obs_cunt_get(const ldg_math_stat_cusum_t *cusum)
{
    if (LDG_UNLIKELY(cusum_init_ck(cusum) != LDG_ERR_AOK)) { return UINT64_MAX; }

    return cusum->obs_cunt;
}

uint32_t ldg_math_stat_cusum_rst(ldg_math_stat_cusum_t *cusum)
{
    uint32_t ret = cusum_init_ck(cusum);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    cusum->upper = CUSUM_NONE;
    cusum->lower = CUSUM_NONE;
    cusum->upper_run = 0;
    cusum->lower_run = 0;
    cusum->obs_cunt = 0;

    return LDG_ERR_AOK;
}
