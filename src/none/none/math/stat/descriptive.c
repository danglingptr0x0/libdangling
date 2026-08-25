
#include <dangling/core/err.h>
#include <dangling/math/math.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>

static void ldg_math_stat_sift_down(double *arr, uint64_t start, uint64_t end)
{
    uint64_t root = 0;
    uint64_t child = 0;
    double tmp = 0.0;

    root = start;
    while (2 * root + 1 < end)
    {
        child = 2 * root + 1;
        if (child + 1 < end && arr[child] < arr[child + 1]) { child++; }

        if (arr[root] >= arr[child]) { return; }

        tmp = arr[root];
        arr[root] = arr[child];
        arr[child] = tmp;
        root = child;
    }
}

static void ldg_math_stat_sort(double *arr, uint64_t cunt)
{
    uint64_t idx = 0;
    uint64_t end = 0;
    double tmp = 0.0;

    if (cunt < 2) { return; }

    for (idx = cunt / 2; idx > 0; idx--) { ldg_math_stat_sift_down(arr, idx - 1, cunt); }
    for (end = cunt; end > 1; end--)
    {
        tmp = arr[0];
        arr[0] = arr[end - 1];
        arr[end - 1] = tmp;
        ldg_math_stat_sift_down(arr, 0, end - 1);
    }
}

static uint32_t ldg_math_stat_welford_cum(const double *val, uint64_t cunt, ldg_math_stat_welford_t *acc)
{
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!val)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_EMPTY; }

    ret = ldg_math_stat_welford_init(acc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < cunt; idx++)
    {
        ret = ldg_math_stat_welford_observe(acc, val[idx]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_mean_get(const double *val, uint64_t cunt, double *out)
{
    ldg_math_stat_welford_t acc = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    ret = ldg_math_stat_welford_cum(val, cunt, &acc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = acc.mean;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_var_get(const double *val, uint64_t cunt, double *out)
{
    ldg_math_stat_welford_t acc = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    ret = ldg_math_stat_welford_cum(val, cunt, &acc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(acc.cunt < 2)) { return LDG_ERR_EMPTY; }

    *out = acc.m2 / (double)(acc.cunt - 1);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_sigma_get(const double *val, uint64_t cunt, double *out)
{
    ldg_math_stat_welford_t acc = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    ret = ldg_math_stat_welford_cum(val, cunt, &acc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(acc.cunt < 2)) { return LDG_ERR_EMPTY; }

    *out = ldg_math_sqrt_get(acc.m2 / (double)(acc.cunt - 1));
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_min_get(const double *val, uint64_t cunt, double *out)
{
    ldg_math_stat_welford_t acc = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    ret = ldg_math_stat_welford_cum(val, cunt, &acc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = acc.min;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_max_get(const double *val, uint64_t cunt, double *out)
{
    ldg_math_stat_welford_t acc = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    ret = ldg_math_stat_welford_cum(val, cunt, &acc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = acc.max;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_skew_get(const double *val, uint64_t cunt, double *out)
{
    ldg_math_stat_welford_t acc = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    ret = ldg_math_stat_welford_cum(val, cunt, &acc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(acc.m2 <= 0.0)) { return LDG_ERR_AOK; }

    *out = ldg_math_sqrt_get((double)acc.cunt) * acc.m3 / (acc.m2 * ldg_math_sqrt_get(acc.m2));
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_kurt_get(const double *val, uint64_t cunt, double *out)
{
    ldg_math_stat_welford_t acc = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    ret = ldg_math_stat_welford_cum(val, cunt, &acc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(acc.m2 <= 0.0)) { return LDG_ERR_AOK; }

    *out = (double)acc.cunt * acc.m4 / (acc.m2 * acc.m2) - 3.0;
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_stat_quantile_pick(const double *val, uint64_t cunt, double level, double *scratch, double *out)
{
    uint64_t lo = 0;
    uint64_t hi = 0;
    double pos = 0.0;
    double frac = 0.0;
    double lo_val = 0.0;
    double hi_val = 0.0;

    if (LDG_UNLIKELY(ldg_mem_cpy(scratch, val, cunt * sizeof(double)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    ldg_math_stat_sort(scratch, cunt);

    pos = level * (double)(cunt - 1);
    lo = (uint64_t)pos;
    hi = lo + 1;
    frac = pos - (double)lo;
    lo_val = scratch[lo];
    hi_val = (hi < cunt) ? scratch[hi] : scratch[lo];
    *out = lo_val + frac * (hi_val - lo_val);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_quantile_get(const double *val, uint64_t cunt, double level, double *scratch, uint64_t scratch_cunt, double *out)
{
    if (LDG_UNLIKELY(!val || !scratch || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(scratch_cunt < cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(level < 0.0 || level > 1.0)) { return LDG_ERR_RANGE; }

    return ldg_math_stat_quantile_pick(val, cunt, level, scratch, out);
}

LDG_EXPORT uint32_t ldg_math_stat_median_get(const double *val, uint64_t cunt, double *scratch, uint64_t scratch_cunt, double *out)
{
    if (LDG_UNLIKELY(!val || !scratch || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(scratch_cunt < cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ldg_math_stat_quantile_pick(val, cunt, 0.5, scratch, out);
}

static uint32_t ldg_math_stat_cov_cum(const double *arr_a, const double *arr_b, uint64_t cunt, double *mean_a, double *mean_b, double *m2_a, double *m2_b, double *co_m2)
{
    uint64_t idx = 0;
    uint64_t obs_cunt = 0;
    double delta_a = 0.0;
    double delta_b = 0.0;

    if (LDG_UNLIKELY(!arr_a || !arr_b)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_EMPTY; }

    *mean_a = 0.0;
    *mean_b = 0.0;
    *m2_a = 0.0;
    *m2_b = 0.0;
    *co_m2 = 0.0;
    for (idx = 0; idx < cunt; idx++)
    {
        obs_cunt = idx + 1;
        delta_a = arr_a[idx] - *mean_a;
        delta_b = arr_b[idx] - *mean_b;
        *mean_a += delta_a / (double)obs_cunt;
        *mean_b += delta_b / (double)obs_cunt;
        *m2_a += delta_a * (arr_a[idx] - *mean_a);
        *m2_b += delta_b * (arr_b[idx] - *mean_b);
        *co_m2 += delta_a * (arr_b[idx] - *mean_b);
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_cov_get(const double *arr_a, const double *arr_b, uint64_t cunt, double *out)
{
    double mean_a = 0.0;
    double mean_b = 0.0;
    double m2_a = 0.0;
    double m2_b = 0.0;
    double co_m2 = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    ret = ldg_math_stat_cov_cum(arr_a, arr_b, cunt, &mean_a, &mean_b, &m2_a, &m2_b, &co_m2);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cunt < 2)) { return LDG_ERR_EMPTY; }

    *out = co_m2 / (double)(cunt - 1);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_corr_get(const double *arr_a, const double *arr_b, uint64_t cunt, double *out)
{
    double mean_a = 0.0;
    double mean_b = 0.0;
    double m2_a = 0.0;
    double m2_b = 0.0;
    double co_m2 = 0.0;
    double sigma_a = 0.0;
    double sigma_b = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    ret = ldg_math_stat_cov_cum(arr_a, arr_b, cunt, &mean_a, &mean_b, &m2_a, &m2_b, &co_m2);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cunt < 2)) { return LDG_ERR_EMPTY; }

    if (LDG_UNLIKELY(m2_a <= 0.0 || m2_b <= 0.0)) { return LDG_ERR_MATH_STAT_INVALID_DIST; }

    sigma_a = ldg_math_sqrt_get(m2_a / (double)(cunt - 1));
    sigma_b = ldg_math_sqrt_get(m2_b / (double)(cunt - 1));
    *out = (co_m2 / (double)(cunt - 1)) / (sigma_a * sigma_b);
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_hist_init(ldg_math_stat_hist_t *hist, double lower, double upper, uint32_t bkt_cunt)
{
    if (LDG_UNLIKELY(!hist)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bkt_cunt == 0 || bkt_cunt > LDG_MATH_STAT_HIST_BKT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(lower >= upper)) { return LDG_ERR_MATH_STAT_INVALID_DIST; }

    if (LDG_UNLIKELY(ldg_mem_zero(hist, sizeof(ldg_math_stat_hist_t)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    hist->lower = lower;
    hist->upper = upper;
    hist->bkt_cunt = bkt_cunt;
    hist->is_init = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_hist_observe(ldg_math_stat_hist_t *hist, double val)
{
    double span = 0.0;
    double pos = 0.0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!hist)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(hist->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_STAT_HIST_FULL; }

    if (LDG_UNLIKELY(val < hist->lower || val >= hist->upper)) { return LDG_ERR_RANGE; }

    span = hist->upper - hist->lower;
    pos = (val - hist->lower) / span * (double)hist->bkt_cunt;
    idx = (uint32_t)pos;
    if (idx >= hist->bkt_cunt) { idx = hist->bkt_cunt - 1; }

    hist->bkt[idx]++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_math_stat_hist_bkt_get(const ldg_math_stat_hist_t *hist, uint32_t idx)
{
    if (LDG_UNLIKELY(!hist || idx >= hist->bkt_cunt)) { return UINT64_MAX; }

    return hist->bkt[idx];
}

LDG_EXPORT uint32_t ldg_math_stat_hist_rst(ldg_math_stat_hist_t *hist)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!hist)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(hist->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    for (idx = 0; idx < hist->bkt_cunt; idx++) { hist->bkt[idx] = 0; }
    return LDG_ERR_AOK;
}
