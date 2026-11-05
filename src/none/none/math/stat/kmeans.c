#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>

#define KMEANS_ROW_CAP_DEFAULT 1024
#define KMEANS_DIM_DEFAULT 2
#define KMEANS_CLUSTER_CUNT_DEFAULT 8
#define KMEANS_PASS_MAX_DEFAULT 100
#define KMEANS_UNIT_SHIFT 11
#define KMEANS_UNIT_SCALE 9007199254740992.0
#define KMEANS_NONE 0.0
#define KMEANS_LAST_OFF 1

static uint32_t kmeans_alloc(uint64_t size, void **out)
{
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc(size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

static void kmeans_free(void *ptr)
{
    if (ptr) { ldg_mem_dealloc(ptr); }
}

static uint32_t kmeans_init_ck(const ldg_math_stat_kmeans_t *kmeans)
{
    if (LDG_UNLIKELY(!kmeans)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(kmeans->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

static uint32_t kmeans_fit_ck(const ldg_math_stat_kmeans_t *kmeans)
{
    uint32_t ret = kmeans_init_ck(kmeans);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(kmeans->is_fit != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_STATE; }

    return LDG_ERR_AOK;
}

static uint32_t kmeans_elem_ck(const double *elem, uintptr_t cunt)
{
    uintptr_t idx = 0;

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(elem[idx]) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        idx++;
    }

    return LDG_ERR_AOK;
}

static uintptr_t kmeans_elem_cunt_get(const ldg_math_stat_kmeans_t *kmeans)
{
    return (uintptr_t)kmeans->conf.cluster_cunt * (uintptr_t)kmeans->conf.dim;
}

static double kmeans_unit_draw(const ldg_math_stat_kmeans_t *kmeans)
{
    return (double)(kmeans->rng_cb(kmeans->rng_ctx) >> KMEANS_UNIT_SHIFT) / KMEANS_UNIT_SCALE;
}

static uint32_t kmeans_row_draw(const ldg_math_stat_kmeans_t *kmeans, uint32_t row_cunt)
{
    uint32_t row = (uint32_t)(kmeans_unit_draw(kmeans) * (double)row_cunt);

    if (row >= row_cunt) { return row_cunt - KMEANS_LAST_OFF; }

    return row;
}

static double kmeans_dist_square_get(const double *lhs, const double *rhs, uint32_t dim)
{
    double cum = KMEANS_NONE;
    double diff = KMEANS_NONE;
    uint32_t idx = 0;

    while (idx < dim)
    {
        diff = lhs[idx] - rhs[idx];
        cum += diff * diff;
        idx++;
    }

    return cum;
}

static void kmeans_row_cpy(double *dst, const double *src, uint32_t dim)
{
    uint32_t idx = 0;

    while (idx < dim)
    {
        dst[idx] = src[idx];
        idx++;
    }
}

static double kmeans_dist_total_get(const ldg_math_stat_kmeans_t *kmeans, uint32_t row_cunt)
{
    double total = KMEANS_NONE;
    uint32_t row = 0;

    while (row < row_cunt)
    {
        total += kmeans->dist[row];
        row++;
    }

    return total;
}

static uint32_t kmeans_weighted_draw(const ldg_math_stat_kmeans_t *kmeans, uint32_t row_cunt, double total)
{
    double tgt = kmeans_unit_draw(kmeans) * total;
    double cum = KMEANS_NONE;
    uint32_t pick = 0;
    uint32_t row = 0;

    while (row < row_cunt && cum <= tgt)
    {
        if (kmeans->dist[row] > KMEANS_NONE) { pick = row; }

        cum += kmeans->dist[row];
        row++;
    }

    return pick;
}

static void kmeans_dist_shrink(ldg_math_stat_kmeans_t *kmeans, const double *point, uint32_t row_cunt, const double *centroid)
{
    const double *row_ptr = point;
    double dist = KMEANS_NONE;
    uint32_t dim = kmeans->conf.dim;
    uint32_t row = 0;

    while (row < row_cunt)
    {
        dist = kmeans_dist_square_get(row_ptr, centroid, dim);

        if (dist < kmeans->dist[row]) { kmeans->dist[row] = dist; }

        row_ptr += dim;
        row++;
    }
}

static void kmeans_seed(ldg_math_stat_kmeans_t *kmeans, const double *point, uint32_t row_cunt)
{
    double *centroid = kmeans->centroid;
    double total = KMEANS_NONE;
    uint32_t dim = kmeans->conf.dim;
    uint32_t cluster = 0;
    uint32_t pick = 0;
    uint32_t row = 0;

    while (row < row_cunt)
    {
        kmeans->dist[row] = LDG_MATH_INFINITY;
        row++;
    }

    while (cluster < kmeans->conf.cluster_cunt)
    {
        total = (cluster == 0) ? KMEANS_NONE : kmeans_dist_total_get(kmeans, row_cunt);
        pick = (total > KMEANS_NONE) ? kmeans_weighted_draw(kmeans, row_cunt, total) : kmeans_row_draw(kmeans, row_cunt);

        kmeans_row_cpy(centroid, point + ((uintptr_t)pick * (uintptr_t)dim), dim);
        kmeans_dist_shrink(kmeans, point, row_cunt, centroid);

        centroid += dim;
        cluster++;
    }
}

static uint32_t kmeans_nearest_get(const ldg_math_stat_kmeans_t *kmeans, const double *row_ptr, double *out_dist)
{
    const double *centroid = kmeans->centroid;
    double best_dist = LDG_MATH_INFINITY;
    double dist = KMEANS_NONE;
    uint32_t dim = kmeans->conf.dim;
    uint32_t best = 0;
    uint32_t cluster = 0;

    while (cluster < kmeans->conf.cluster_cunt)
    {
        dist = kmeans_dist_square_get(row_ptr, centroid, dim);

        if (dist < best_dist)
        {
            best = cluster;
            best_dist = dist;
        }

        centroid += dim;
        cluster++;
    }

    *out_dist = best_dist;

    return best;
}

static uint32_t kmeans_assign(ldg_math_stat_kmeans_t *kmeans, const double *point, uint32_t row_cunt, uint32_t *label)
{
    const double *row_ptr = point;
    double dist = KMEANS_NONE;
    uint32_t changed_cunt = 0;
    uint32_t best = 0;
    uint32_t row = 0;

    while (row < row_cunt)
    {
        best = kmeans_nearest_get(kmeans, row_ptr, &dist);
        kmeans->dist[row] = dist;

        if (label[row] != best) { changed_cunt++; }

        label[row] = best;
        row_ptr += kmeans->conf.dim;
        row++;
    }

    return changed_cunt;
}

static void kmeans_member_tally(ldg_math_stat_kmeans_t *kmeans, uint32_t row_cunt, const uint32_t *label)
{
    uint32_t cluster = 0;
    uint32_t row = 0;

    while (cluster < kmeans->conf.cluster_cunt)
    {
        kmeans->member_cunt[cluster] = 0;
        cluster++;
    }

    while (row < row_cunt)
    {
        kmeans->member_cunt[label[row]]++;
        row++;
    }
}

static void kmeans_cum_fill(ldg_math_stat_kmeans_t *kmeans, const double *point, uint32_t row_cunt, const uint32_t *label)
{
    const double *row_ptr = point;
    double *cum = 0x0;
    uintptr_t elem_cunt = kmeans_elem_cunt_get(kmeans);
    uintptr_t elem = 0;
    uint32_t dim = kmeans->conf.dim;
    uint32_t idx = 0;
    uint32_t row = 0;

    while (elem < elem_cunt)
    {
        kmeans->cum[elem] = KMEANS_NONE;
        elem++;
    }

    while (row < row_cunt)
    {
        cum = kmeans->cum + ((uintptr_t)label[row] * (uintptr_t)dim);
        idx = 0;

        while (idx < dim)
        {
            cum[idx] += row_ptr[idx];
            idx++;
        }

        row_ptr += dim;
        row++;
    }
}

static uint32_t kmeans_farthest_get(const ldg_math_stat_kmeans_t *kmeans, uint32_t row_cunt)
{
    double best_dist = KMEANS_NONE;
    uint32_t best = 0;
    uint32_t row = 0;

    while (row < row_cunt)
    {
        if (kmeans->dist[row] > best_dist)
        {
            best = row;
            best_dist = kmeans->dist[row];
        }

        row++;
    }

    return best;
}

static uint32_t kmeans_centroid_update(ldg_math_stat_kmeans_t *kmeans, const double *point, uint32_t row_cunt, const uint32_t *label)
{
    double *centroid = kmeans->centroid;
    const double *cum = kmeans->cum;
    double member = KMEANS_NONE;
    uint32_t dim = kmeans->conf.dim;
    uint32_t reseed_cunt = 0;
    uint32_t cluster = 0;
    uint32_t pick = 0;
    uint32_t idx = 0;

    kmeans_member_tally(kmeans, row_cunt, label);
    kmeans_cum_fill(kmeans, point, row_cunt, label);

    while (cluster < kmeans->conf.cluster_cunt)
    {
        if (kmeans->member_cunt[cluster] == 0)
        {
            pick = kmeans_farthest_get(kmeans, row_cunt);
            kmeans_row_cpy(centroid, point + ((uintptr_t)pick * (uintptr_t)dim), dim);
            kmeans->dist[pick] = KMEANS_NONE;
            reseed_cunt++;
        }
        else
        {
            member = (double)kmeans->member_cunt[cluster];
            idx = 0;

            while (idx < dim)
            {
                centroid[idx] = cum[idx] / member;
                idx++;
            }
        }

        centroid += dim;
        cum += dim;
        cluster++;
    }

    return reseed_cunt;
}

uint32_t ldg_math_stat_kmeans_conf_default_get(ldg_math_stat_kmeans_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->row_cap = KMEANS_ROW_CAP_DEFAULT;
    out->dim = KMEANS_DIM_DEFAULT;
    out->cluster_cunt = KMEANS_CLUSTER_CUNT_DEFAULT;
    out->pass_max = KMEANS_PASS_MAX_DEFAULT;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_kmeans_init(ldg_math_stat_kmeans_t *kmeans, const ldg_math_stat_kmeans_conf_t *conf, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx)
{
    void *ptr = 0x0;
    uint64_t elem_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!kmeans || !conf || !rng_cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->row_cap == 0 || conf->dim == 0 || conf->cluster_cunt == 0 || conf->pass_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->cluster_cunt > conf->row_cap || conf->cluster_cunt == LDG_MATH_STAT_KMEANS_LABEL_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_zero(kmeans, sizeof(*kmeans));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    kmeans->rng_cb = rng_cb;
    kmeans->rng_ctx = rng_ctx;
    kmeans->conf = *conf;
    elem_cunt = (uint64_t)conf->cluster_cunt * (uint64_t)conf->dim;

    ret = kmeans_alloc(elem_cunt * (uint64_t)sizeof(double), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    kmeans->centroid = (double *)ptr;

    ret = kmeans_alloc(elem_cunt * (uint64_t)sizeof(double), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        kmeans_free(kmeans->centroid);
        return ret;
    }

    kmeans->cum = (double *)ptr;

    ret = kmeans_alloc((uint64_t)conf->row_cap * (uint64_t)sizeof(double), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        kmeans_free(kmeans->cum);
        kmeans_free(kmeans->centroid);
        return ret;
    }

    kmeans->dist = (double *)ptr;

    ret = kmeans_alloc((uint64_t)conf->cluster_cunt * (uint64_t)sizeof(uint32_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        kmeans_free(kmeans->dist);
        kmeans_free(kmeans->cum);
        kmeans_free(kmeans->centroid);
        return ret;
    }

    kmeans->member_cunt = (uint32_t *)ptr;

    kmeans->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_kmeans_shutdown(ldg_math_stat_kmeans_t *kmeans)
{
    uint32_t ret = kmeans_init_ck(kmeans);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    kmeans_free(kmeans->member_cunt);
    kmeans_free(kmeans->dist);
    kmeans_free(kmeans->cum);
    kmeans_free(kmeans->centroid);

    return ldg_mem_zero(kmeans, sizeof(*kmeans));
}

uint32_t ldg_math_stat_kmeans_rst(ldg_math_stat_kmeans_t *kmeans)
{
    uint64_t elem_cunt = 0;
    uint32_t ret = kmeans_init_ck(kmeans);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    elem_cunt = (uint64_t)kmeans->conf.cluster_cunt * (uint64_t)kmeans->conf.dim;
    kmeans->is_fit = LDG_TRUTH_FALSE;

    ret = ldg_mem_zero(kmeans->centroid, elem_cunt * (uint64_t)sizeof(double));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(kmeans->member_cunt, (uint64_t)kmeans->conf.cluster_cunt * (uint64_t)sizeof(uint32_t));
}

uint32_t ldg_math_stat_kmeans_fit(ldg_math_stat_kmeans_t *kmeans, const double *point, uint32_t row_cunt, uint32_t *label, ldg_math_stat_kmeans_report_t *out)
{
    uint32_t reseed_cunt = 0;
    uint32_t pass_cunt = 0;
    uint32_t row = 0;
    uint32_t ret = kmeans_init_ck(kmeans);
    uint8_t converged_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!point || !label || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, sizeof(*out));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(row_cunt < kmeans->conf.cluster_cunt || row_cunt > kmeans->conf.row_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = kmeans_elem_ck(point, (uintptr_t)row_cunt * (uintptr_t)kmeans->conf.dim);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    kmeans->is_fit = LDG_TRUTH_FALSE;

    while (row < row_cunt)
    {
        label[row] = LDG_MATH_STAT_KMEANS_LABEL_NONE;
        row++;
    }

    kmeans_seed(kmeans, point, row_cunt);

    while (pass_cunt < kmeans->conf.pass_max && !converged_is)
    {
        pass_cunt++;

        if (kmeans_assign(kmeans, point, row_cunt, label) == 0) { converged_is = LDG_TRUTH_TRUE; }
        else { reseed_cunt += kmeans_centroid_update(kmeans, point, row_cunt, label); }
    }

    if (!converged_is) { if (kmeans_assign(kmeans, point, row_cunt, label) == 0) { converged_is = LDG_TRUTH_TRUE; } }

    kmeans_member_tally(kmeans, row_cunt, label);

    out->inertia = kmeans_dist_total_get(kmeans, row_cunt);
    out->pass_cunt = pass_cunt;
    out->reseed_cunt = reseed_cunt;
    out->converged_is = converged_is;
    kmeans->is_fit = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_kmeans_label_get(const ldg_math_stat_kmeans_t *kmeans, const double *point, uint32_t *out)
{
    double dist = KMEANS_NONE;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!kmeans || !point || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_STAT_KMEANS_LABEL_NONE;

    ret = kmeans_fit_ck(kmeans);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = kmeans_elem_ck(point, (uintptr_t)kmeans->conf.dim);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = kmeans_nearest_get(kmeans, point, &dist);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_kmeans_centroid_get(const ldg_math_stat_kmeans_t *kmeans, uint32_t cluster, double *dst, uint32_t cap)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!kmeans || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = kmeans_fit_ck(kmeans);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cluster >= kmeans->conf.cluster_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(cap < kmeans->conf.dim)) { return LDG_ERR_OVERFLOW; }

    kmeans_row_cpy(dst, kmeans->centroid + ((uintptr_t)cluster * (uintptr_t)kmeans->conf.dim), kmeans->conf.dim);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_kmeans_member_cunt_get(const ldg_math_stat_kmeans_t *kmeans, uint32_t cluster, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!kmeans || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = kmeans_fit_ck(kmeans);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cluster >= kmeans->conf.cluster_cunt)) { return LDG_ERR_BOUNDS; }

    *out = kmeans->member_cunt[cluster];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_kmeans_state_cunt_get(const ldg_math_stat_kmeans_t *kmeans, uintptr_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!kmeans || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = kmeans_init_ck(kmeans);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = kmeans_elem_cunt_get(kmeans);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_kmeans_state_get(const ldg_math_stat_kmeans_t *kmeans, double *dst, uintptr_t cap, uintptr_t *out_cunt)
{
    uintptr_t elem_cunt = 0;
    uintptr_t elem = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!kmeans || !dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = kmeans_fit_ck(kmeans);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    elem_cunt = kmeans_elem_cunt_get(kmeans);
    if (LDG_UNLIKELY(cap < elem_cunt)) { return LDG_ERR_OVERFLOW; }

    while (elem < elem_cunt)
    {
        dst[elem] = kmeans->centroid[elem];
        elem++;
    }

    *out_cunt = elem_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_kmeans_state_set(ldg_math_stat_kmeans_t *kmeans, const double *src, uintptr_t cunt)
{
    uintptr_t elem = 0;
    uint32_t cluster = 0;
    uint32_t ret = kmeans_init_ck(kmeans);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt != kmeans_elem_cunt_get(kmeans))) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = kmeans_elem_ck(src, cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (elem < cunt)
    {
        kmeans->centroid[elem] = src[elem];
        elem++;
    }

    while (cluster < kmeans->conf.cluster_cunt)
    {
        kmeans->member_cunt[cluster] = 0;
        cluster++;
    }

    kmeans->is_fit = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}
