#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/complex.h>
#include <dangling/math/eigen.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>

#define MDS_ROW_CUNT_DEFAULT 64
#define MDS_OUT_DIM_DEFAULT 2
#define MDS_NONE 0.0
#define MDS_GRAM_SCALE (-0.5)
#define MDS_SYMMETRIC_WEIGHT 0.5
#define MDS_ROW_MIN 2
#define MDS_ADJACENT_OFF 1
#define MDS_STATE_HDR_CUNT 1

static uint32_t mds_alloc(uint64_t size, void **out)
{
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc(size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

static void mds_free(void *ptr)
{
    if (ptr) { ldg_mem_dealloc(ptr); }
}

static void mds_buff_free(ldg_math_stat_mds_t *mds)
{
    if (mds->eigen) { ldg_math_eigen_ctx_destroy(&mds->eigen); }

    mds_free(mds->order);
    mds_free(mds->eig);
    mds_free(mds->square_mean);
    mds_free(mds->proj);
    mds_free(mds->coord);
    mds_free(mds->val);
    mds_free(mds->vec);
    mds_free(mds->mat);
}

static uint32_t mds_init_ck(const ldg_math_stat_mds_t *mds)
{
    if (LDG_UNLIKELY(!mds)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(mds->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

static uint32_t mds_fit_ck(const ldg_math_stat_mds_t *mds)
{
    uint32_t ret = mds_init_ck(mds);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(mds->is_fit != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_STATE; }

    return LDG_ERR_AOK;
}

static uint32_t mds_elem_ck(const double *elem, uintptr_t cunt)
{
    uintptr_t idx = 0;

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(elem[idx]) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        idx++;
    }

    return LDG_ERR_AOK;
}

static uint32_t mds_dist_ck(const double *dist, uintptr_t cunt)
{
    uintptr_t idx = 0;

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(dist[idx]) != LDG_TRUTH_TRUE || dist[idx] < MDS_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        idx++;
    }

    return LDG_ERR_AOK;
}

static uintptr_t mds_state_cunt_get(const ldg_math_stat_mds_t *mds)
{
    uintptr_t row_cunt = (uintptr_t)mds->conf.row_cunt;
    uintptr_t out_dim = (uintptr_t)mds->conf.out_dim;

    return (uintptr_t)MDS_STATE_HDR_CUNT + row_cunt + out_dim + (out_dim * row_cunt) + (row_cunt * out_dim);
}

static double mds_square_get(const ldg_math_stat_mds_t *mds, const double *dist, uint32_t row, uint32_t col)
{
    double upper = dist[((uintptr_t)row * (uintptr_t)mds->conf.row_cunt) + (uintptr_t)col];
    double lower = dist[((uintptr_t)col * (uintptr_t)mds->conf.row_cunt) + (uintptr_t)row];

    return MDS_SYMMETRIC_WEIGHT * ((upper * upper) + (lower * lower));
}

static double mds_square_mean_fill(ldg_math_stat_mds_t *mds, const double *dist)
{
    double total = MDS_NONE;
    double cum = MDS_NONE;
    uint32_t row_cunt = mds->conf.row_cunt;
    uint32_t row = 0;
    uint32_t col = 0;

    while (row < row_cunt)
    {
        cum = MDS_NONE;
        col = 0;

        while (col < row_cunt)
        {
            cum += mds_square_get(mds, dist, row, col);
            col++;
        }

        mds->square_mean[row] = cum / (double)row_cunt;
        total += mds->square_mean[row];
        row++;
    }

    return total / (double)row_cunt;
}

static void mds_gram_fill(ldg_math_stat_mds_t *mds, const double *dist)
{
    ldg_math_complex_t *mat = mds->mat;
    double grand = mds_square_mean_fill(mds, dist);
    uint32_t row_cunt = mds->conf.row_cunt;
    uint32_t row = 0;
    uint32_t col = 0;

    while (row < row_cunt)
    {
        col = 0;

        while (col < row_cunt)
        {
            mat->re = MDS_GRAM_SCALE * (mds_square_get(mds, dist, row, col) - mds->square_mean[row] - mds->square_mean[col] + grand);
            mat->im = MDS_NONE;
            mat++;
            col++;
        }

        row++;
    }
}

static void mds_order_sort(ldg_math_stat_mds_t *mds)
{
    uint32_t idx = 0;
    uint32_t slot = 0;

    while (idx < mds->conf.row_cunt)
    {
        slot = idx;

        while (slot > 0 && mds->val[mds->order[slot - MDS_ADJACENT_OFF]] < mds->val[idx])
        {
            mds->order[slot] = mds->order[slot - MDS_ADJACENT_OFF];
            slot--;
        }

        mds->order[slot] = idx;
        idx++;
    }
}

static double mds_axis_sign_get(const ldg_math_stat_mds_t *mds, uint32_t axis)
{
    const ldg_math_complex_t *vec = mds->vec + mds->order[axis];
    double peak = MDS_NONE;
    double peak_abs = MDS_NONE;
    uint32_t row = 0;

    while (row < mds->conf.row_cunt)
    {
        if (ldg_math_abs_get(vec->re) > peak_abs)
        {
            peak = vec->re;
            peak_abs = ldg_math_abs_get(vec->re);
        }

        vec += mds->conf.row_cunt;
        row++;
    }

    return (peak < MDS_NONE) ? -ldg_math_sqrt_get(mds->eig[axis]) : ldg_math_sqrt_get(mds->eig[axis]);
}

static void mds_axis_fill(ldg_math_stat_mds_t *mds, uint32_t axis, uint8_t kept_is)
{
    const ldg_math_complex_t *vec = mds->vec + mds->order[axis];
    double *coord = mds->coord + axis;
    double *proj = mds->proj + ((uintptr_t)axis * (uintptr_t)mds->conf.row_cunt);
    double scale = kept_is ? mds_axis_sign_get(mds, axis) : MDS_NONE;
    uint32_t row = 0;

    while (row < mds->conf.row_cunt)
    {
        *coord = kept_is ? (vec->re * scale) : MDS_NONE;
        proj[row] = kept_is ? (vec->re / scale) : MDS_NONE;
        coord += mds->conf.out_dim;
        vec += mds->conf.row_cunt;
        row++;
    }
}

static void mds_map_fill(ldg_math_stat_mds_t *mds)
{
    double eig_min = mds->val[mds->order[0]] * LDG_MATH_EPSILON;
    uint32_t axis = 0;
    uint8_t kept_is = LDG_TRUTH_FALSE;

    mds->rank = 0;

    while (axis < mds->conf.out_dim)
    {
        mds->eig[axis] = mds->val[mds->order[axis]];
        kept_is = (mds->eig[axis] > eig_min && mds->eig[axis] > MDS_NONE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

        if (kept_is) { mds->rank++; }

        mds_axis_fill(mds, axis, kept_is);
        axis++;
    }
}

uint32_t ldg_math_stat_mds_conf_default_get(ldg_math_stat_mds_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->row_cunt = MDS_ROW_CUNT_DEFAULT;
    out->out_dim = MDS_OUT_DIM_DEFAULT;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_mds_init(ldg_math_stat_mds_t *mds, const ldg_math_stat_mds_conf_t *conf)
{
    void *ptr = 0x0;
    uint64_t row_cunt = 0;
    uint64_t out_dim = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!mds || !conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->row_cunt < MDS_ROW_MIN || conf->out_dim == 0 || conf->out_dim >= conf->row_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_zero(mds, sizeof(*mds));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mds->conf = *conf;
    row_cunt = (uint64_t)conf->row_cunt;
    out_dim = (uint64_t)conf->out_dim;

    ret = mds_alloc(row_cunt * row_cunt * (uint64_t)sizeof(ldg_math_complex_t), &ptr);
    mds->mat = (ldg_math_complex_t *)ptr;

    if (ret == LDG_ERR_AOK) { ret = mds_alloc(row_cunt * row_cunt * (uint64_t)sizeof(ldg_math_complex_t), &ptr); }

    mds->vec = (ret == LDG_ERR_AOK) ? (ldg_math_complex_t *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = mds_alloc(row_cunt * (uint64_t)sizeof(double), &ptr); }

    mds->val = (ret == LDG_ERR_AOK) ? (double *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = mds_alloc(row_cunt * out_dim * (uint64_t)sizeof(double), &ptr); }

    mds->coord = (ret == LDG_ERR_AOK) ? (double *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = mds_alloc(out_dim * row_cunt * (uint64_t)sizeof(double), &ptr); }

    mds->proj = (ret == LDG_ERR_AOK) ? (double *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = mds_alloc(row_cunt * (uint64_t)sizeof(double), &ptr); }

    mds->square_mean = (ret == LDG_ERR_AOK) ? (double *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = mds_alloc(out_dim * (uint64_t)sizeof(double), &ptr); }

    mds->eig = (ret == LDG_ERR_AOK) ? (double *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = mds_alloc(row_cunt * (uint64_t)sizeof(uint32_t), &ptr); }

    mds->order = (ret == LDG_ERR_AOK) ? (uint32_t *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = ldg_math_eigen_ctx_create(row_cunt, &mds->eigen); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        mds_buff_free(mds);
        return ret;
    }

    mds->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_mds_shutdown(ldg_math_stat_mds_t *mds)
{
    uint32_t ret = mds_init_ck(mds);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mds_buff_free(mds);

    return ldg_mem_zero(mds, sizeof(*mds));
}

uint32_t ldg_math_stat_mds_rst(ldg_math_stat_mds_t *mds)
{
    uint32_t ret = mds_init_ck(mds);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mds->rank = 0;
    mds->is_fit = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_mds_fit(ldg_math_stat_mds_t *mds, const double *dist)
{
    uint32_t ret = mds_init_ck(mds);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!dist)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = mds_dist_ck(dist, (uintptr_t)mds->conf.row_cunt * (uintptr_t)mds->conf.row_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mds->is_fit = LDG_TRUTH_FALSE;
    mds_gram_fill(mds, dist);

    ret = ldg_math_eigen_hermitian_compute(mds->eigen, mds->mat, mds->val, mds->vec);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    mds_order_sort(mds);
    mds_map_fill(mds);
    mds->is_fit = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_mds_coord_get(const ldg_math_stat_mds_t *mds, uint32_t row, double *dst, uint32_t cap)
{
    const double *coord = 0x0;
    uint32_t axis = 0;
    uint32_t ret = mds_fit_ck(mds);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(row >= mds->conf.row_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(cap < mds->conf.out_dim)) { return LDG_ERR_OVERFLOW; }

    coord = mds->coord + ((uintptr_t)row * (uintptr_t)mds->conf.out_dim);

    while (axis < mds->conf.out_dim)
    {
        dst[axis] = coord[axis];
        axis++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_mds_val_get(const ldg_math_stat_mds_t *mds, uint32_t axis, double *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!mds || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = MDS_NONE;

    ret = mds_fit_ck(mds);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(axis >= mds->conf.out_dim)) { return LDG_ERR_BOUNDS; }

    *out = mds->eig[axis];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_mds_rank_get(const ldg_math_stat_mds_t *mds, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!mds || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = mds_fit_ck(mds);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = mds->rank;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_mds_point_proj(const ldg_math_stat_mds_t *mds, const double *dist, double *dst, uint32_t cap)
{
    const double *proj = 0x0;
    double cum = MDS_NONE;
    uint32_t axis = 0;
    uint32_t row = 0;
    uint32_t ret = mds_fit_ck(mds);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!dist || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap < mds->conf.out_dim)) { return LDG_ERR_OVERFLOW; }

    ret = mds_dist_ck(dist, (uintptr_t)mds->conf.row_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    proj = mds->proj;

    while (axis < mds->conf.out_dim)
    {
        cum = MDS_NONE;
        row = 0;

        while (row < mds->conf.row_cunt)
        {
            cum += proj[row] * ((dist[row] * dist[row]) - mds->square_mean[row]);
            row++;
        }

        dst[axis] = MDS_GRAM_SCALE * cum;
        proj += mds->conf.row_cunt;
        axis++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_mds_state_cunt_get(const ldg_math_stat_mds_t *mds, uintptr_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!mds || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = mds_init_ck(mds);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = mds_state_cunt_get(mds);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_mds_state_get(const ldg_math_stat_mds_t *mds, double *dst, uintptr_t cap, uintptr_t *out_cunt)
{
    double *lane = dst;
    uintptr_t map_cunt = 0;
    uintptr_t elem = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!mds || !dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = mds_fit_ck(mds);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < mds_state_cunt_get(mds))) { return LDG_ERR_OVERFLOW; }

    *lane = (double)mds->rank;
    lane += MDS_STATE_HDR_CUNT;

    while (idx < mds->conf.row_cunt)
    {
        lane[idx] = mds->square_mean[idx];
        idx++;
    }

    lane += mds->conf.row_cunt;
    idx = 0;

    while (idx < mds->conf.out_dim)
    {
        lane[idx] = mds->eig[idx];
        idx++;
    }

    lane += mds->conf.out_dim;
    map_cunt = (uintptr_t)mds->conf.out_dim * (uintptr_t)mds->conf.row_cunt;

    while (elem < map_cunt)
    {
        lane[elem] = mds->proj[elem];
        lane[map_cunt + elem] = mds->coord[elem];
        elem++;
    }

    *out_cunt = mds_state_cunt_get(mds);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_mds_state_set(ldg_math_stat_mds_t *mds, const double *src, uintptr_t cunt)
{
    const double *lane = src;
    uintptr_t map_cunt = 0;
    uintptr_t elem = 0;
    uint32_t idx = 0;
    uint32_t ret = mds_init_ck(mds);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt != mds_state_cunt_get(mds))) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = mds_elem_ck(src, cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(*lane < MDS_NONE || *lane > (double)mds->conf.out_dim)) { return LDG_ERR_FUNC_ARG_INVALID; }

    mds->rank = (uint32_t)*lane;
    lane += MDS_STATE_HDR_CUNT;

    while (idx < mds->conf.row_cunt)
    {
        mds->square_mean[idx] = lane[idx];
        idx++;
    }

    lane += mds->conf.row_cunt;
    idx = 0;

    while (idx < mds->conf.out_dim)
    {
        mds->eig[idx] = lane[idx];
        idx++;
    }

    lane += mds->conf.out_dim;
    map_cunt = (uintptr_t)mds->conf.out_dim * (uintptr_t)mds->conf.row_cunt;

    while (elem < map_cunt)
    {
        mds->proj[elem] = lane[elem];
        mds->coord[elem] = lane[map_cunt + elem];
        elem++;
    }

    mds->is_fit = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}
