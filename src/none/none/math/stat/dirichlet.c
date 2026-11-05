#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>
#include <dangling/mem/cpy.h>

#define DIRICHLET_NONE 0.0

static uint32_t dirichlet_init_ck(const ldg_math_stat_dirichlet_t *dirichlet)
{
    if (LDG_UNLIKELY(!dirichlet)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(dirichlet->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

static uint32_t dirichlet_prior_ck(const double *prior, uint32_t category_cunt)
{
    uint32_t category = 0;

    while (category < category_cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(prior[category]) != LDG_TRUTH_TRUE || prior[category] <= DIRICHLET_NONE)) { return LDG_ERR_MATH_STAT_PRIOR_INVALID; }

        category++;
    }

    return LDG_ERR_AOK;
}

static double dirichlet_mass_get(const ldg_math_stat_dirichlet_t *dirichlet, uint32_t category)
{
    return dirichlet->prior[category] + (double)dirichlet->obs_cunt[category];
}

static double dirichlet_mass_total_get(const ldg_math_stat_dirichlet_t *dirichlet)
{
    double total = (double)dirichlet->total_obs;
    uint32_t category = 0;

    while (category < dirichlet->category_cunt)
    {
        total += dirichlet->prior[category];
        category++;
    }

    return total;
}

uint32_t ldg_math_stat_dirichlet_init(ldg_math_stat_dirichlet_t *dirichlet, uint32_t category_cunt, const double *prior)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!dirichlet || !prior)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(category_cunt == 0 || category_cunt > LDG_MATH_STAT_DIRICHLET_CATEGORY_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = dirichlet_prior_ck(prior, category_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(dirichlet, sizeof(*dirichlet));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    dirichlet->prior = prior;
    dirichlet->category_cunt = category_cunt;
    dirichlet->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_dirichlet_shutdown(ldg_math_stat_dirichlet_t *dirichlet)
{
    uint32_t ret = dirichlet_init_ck(dirichlet);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(dirichlet, sizeof(*dirichlet));
}

uint32_t ldg_math_stat_dirichlet_observe(ldg_math_stat_dirichlet_t *dirichlet, uint32_t category)
{
    uint32_t ret = dirichlet_init_ck(dirichlet);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(category >= dirichlet->category_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(dirichlet->total_obs == UINT64_MAX)) { return LDG_ERR_OVERFLOW; }

    dirichlet->obs_cunt[category]++;
    dirichlet->total_obs++;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_dirichlet_mean_get(const ldg_math_stat_dirichlet_t *dirichlet, double *dst, uint32_t cap)
{
    double total = DIRICHLET_NONE;
    uint32_t category = 0;
    uint32_t ret = dirichlet_init_ck(dirichlet);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap < dirichlet->category_cunt)) { return LDG_ERR_OVERFLOW; }

    ret = dirichlet_prior_ck(dirichlet->prior, dirichlet->category_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    total = dirichlet_mass_total_get(dirichlet);

    while (category < dirichlet->category_cunt)
    {
        dst[category] = dirichlet_mass_get(dirichlet, category) / total;
        category++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_dirichlet_predictive_get(const ldg_math_stat_dirichlet_t *dirichlet, uint32_t category, double *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!dirichlet || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = DIRICHLET_NONE;

    ret = dirichlet_init_ck(dirichlet);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(category >= dirichlet->category_cunt)) { return LDG_ERR_BOUNDS; }

    ret = dirichlet_prior_ck(dirichlet->prior, dirichlet->category_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = dirichlet_mass_get(dirichlet, category) / dirichlet_mass_total_get(dirichlet);

    return LDG_ERR_AOK;
}

uint64_t ldg_math_stat_dirichlet_obs_cunt_get(const ldg_math_stat_dirichlet_t *dirichlet, uint32_t category)
{
    if (LDG_UNLIKELY(dirichlet_init_ck(dirichlet) != LDG_ERR_AOK)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(category >= dirichlet->category_cunt)) { return UINT64_MAX; }

    return dirichlet->obs_cunt[category];
}

uint32_t ldg_math_stat_dirichlet_rst(ldg_math_stat_dirichlet_t *dirichlet)
{
    uint32_t category = 0;
    uint32_t ret = dirichlet_init_ck(dirichlet);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (category < dirichlet->category_cunt)
    {
        dirichlet->obs_cunt[category] = 0;
        category++;
    }

    dirichlet->total_obs = 0;

    return LDG_ERR_AOK;
}
