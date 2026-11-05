#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>

#define NEAREST_NONE 0.0
#define NEAREST_COSINE_MAX 1.0
#define NEAREST_COSINE_MIN (-1.0)
#define NEAREST_ADJACENT_OFF 1

static uint32_t nearest_elem_ck(const double *elem, uint32_t cunt)
{
    uint32_t idx = 0;

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(elem[idx]) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        idx++;
    }

    return LDG_ERR_AOK;
}

static double nearest_dot_get(const double *lhs, const double *rhs, uint32_t dim)
{
    double cum = NEAREST_NONE;
    uint32_t idx = 0;

    while (idx < dim)
    {
        cum += lhs[idx] * rhs[idx];
        idx++;
    }

    return cum;
}

static double nearest_cosine_get(const double *lhs, const double *rhs, uint32_t dim, double lhs_norm)
{
    double rhs_norm = ldg_math_sqrt_get(nearest_dot_get(rhs, rhs, dim));
    double cosine = NEAREST_NONE;

    if (lhs_norm <= NEAREST_NONE || rhs_norm <= NEAREST_NONE) { return NEAREST_NONE; }

    cosine = nearest_dot_get(lhs, rhs, dim) / (lhs_norm * rhs_norm);

    if (cosine > NEAREST_COSINE_MAX) { return NEAREST_COSINE_MAX; }

    if (cosine < NEAREST_COSINE_MIN) { return NEAREST_COSINE_MIN; }

    return cosine;
}

static double nearest_euclid_get(const double *lhs, const double *rhs, uint32_t dim)
{
    double cum = NEAREST_NONE;
    double diff = NEAREST_NONE;
    uint32_t idx = 0;

    while (idx < dim)
    {
        diff = lhs[idx] - rhs[idx];
        cum += diff * diff;
        idx++;
    }

    return ldg_math_sqrt_get(cum);
}

static uint32_t nearest_slot_put(uint32_t *idx_dst, double *dist_dst, uint32_t cunt, uint32_t cap, uint32_t row, double dist)
{
    uint32_t slot = cunt;

    if (cunt == cap)
    {
        slot = cap - NEAREST_ADJACENT_OFF;

        if (!(dist < dist_dst[slot])) { return cunt; }
    }

    while (slot > 0 && dist_dst[slot - NEAREST_ADJACENT_OFF] > dist)
    {
        dist_dst[slot] = dist_dst[slot - NEAREST_ADJACENT_OFF];
        idx_dst[slot] = idx_dst[slot - NEAREST_ADJACENT_OFF];
        slot--;
    }

    dist_dst[slot] = dist;
    idx_dst[slot] = row;

    return (cunt < cap) ? (cunt + NEAREST_ADJACENT_OFF) : cunt;
}

uint32_t ldg_math_stat_cosine_get(const double *lhs, const double *rhs, uint32_t dim, double *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = NEAREST_NONE;

    if (LDG_UNLIKELY(dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = nearest_elem_ck(lhs, dim);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = nearest_elem_ck(rhs, dim);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = nearest_cosine_get(lhs, rhs, dim, ldg_math_sqrt_get(nearest_dot_get(lhs, lhs, dim)));

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_nearest_get(const double *point, uint32_t row_cunt, uint32_t dim, const double *query, uint32_t metric, uint32_t *idx_dst, double *dist_dst, uint32_t cap, uint32_t *out_cunt)
{
    const double *row_ptr = point;
    double query_norm = NEAREST_NONE;
    double dist = NEAREST_NONE;
    uint32_t cunt = 0;
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!point || !query || !idx_dst || !dist_dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(row_cunt == 0 || dim == 0 || cap == 0 || metric >= LDG_MATH_STAT_METRIC_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = nearest_elem_ck(query, dim);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (metric == LDG_MATH_STAT_METRIC_COSINE) { query_norm = ldg_math_sqrt_get(nearest_dot_get(query, query, dim)); }

    while (row < row_cunt)
    {
        dist = (metric == LDG_MATH_STAT_METRIC_COSINE) ? (NEAREST_COSINE_MAX - nearest_cosine_get(query, row_ptr, dim, query_norm)) : nearest_euclid_get(query, row_ptr, dim);

        if (LDG_UNLIKELY(ldg_math_nan_is(dist) == LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        cunt = nearest_slot_put(idx_dst, dist_dst, cunt, cap, row, dist);
        row_ptr += dim;
        row++;
    }

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}
