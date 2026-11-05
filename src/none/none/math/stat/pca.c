#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/complex.h>
#include <dangling/math/eigen.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>

#define PCA_DIM_DEFAULT 8
#define PCA_COMP_CUNT_DEFAULT 2
#define PCA_NONE 0.0
#define PCA_ROW_MIN 2
#define PCA_BESSEL_OFF 1
#define PCA_ADJACENT_OFF 1
#define PCA_STATE_HDR_CUNT 1

static uint32_t pca_alloc(uint64_t size, void **out)
{
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc(size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

static void pca_free(void *ptr)
{
    if (ptr) { ldg_mem_dealloc(ptr); }
}

static void pca_buff_free(ldg_math_stat_pca_t *pca)
{
    if (pca->eigen) { ldg_math_eigen_ctx_destroy(&pca->eigen); }

    pca_free(pca->order);
    pca_free(pca->var);
    pca_free(pca->basis);
    pca_free(pca->mean);
    pca_free(pca->val);
    pca_free(pca->vec);
    pca_free(pca->mat);
}

static uint32_t pca_init_ck(const ldg_math_stat_pca_t *pca)
{
    if (LDG_UNLIKELY(!pca)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(pca->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

static uint32_t pca_fit_ck(const ldg_math_stat_pca_t *pca)
{
    uint32_t ret = pca_init_ck(pca);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(pca->is_fit != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_STATE; }

    return LDG_ERR_AOK;
}

static uint32_t pca_elem_ck(const double *elem, uintptr_t cunt)
{
    uintptr_t idx = 0;

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(elem[idx]) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        idx++;
    }

    return LDG_ERR_AOK;
}

static uintptr_t pca_state_cunt_get(const ldg_math_stat_pca_t *pca)
{
    uintptr_t dim = (uintptr_t)pca->conf.dim;
    uintptr_t comp_cunt = (uintptr_t)pca->conf.comp_cunt;

    return (uintptr_t)PCA_STATE_HDR_CUNT + dim + comp_cunt + (comp_cunt * dim);
}

static void pca_mean_fill(ldg_math_stat_pca_t *pca, const double *point, uint32_t row_cunt)
{
    const double *row_ptr = point;
    uint32_t dim = pca->conf.dim;
    uint32_t idx = 0;
    uint32_t row = 0;

    while (idx < dim)
    {
        pca->mean[idx] = PCA_NONE;
        idx++;
    }

    while (row < row_cunt)
    {
        idx = 0;

        while (idx < dim)
        {
            pca->mean[idx] += row_ptr[idx];
            idx++;
        }

        row_ptr += dim;
        row++;
    }

    idx = 0;

    while (idx < dim)
    {
        pca->mean[idx] /= (double)row_cunt;
        idx++;
    }
}

static void pca_cov_row_cum(ldg_math_stat_pca_t *pca, const double *row_ptr)
{
    ldg_math_complex_t *mat = pca->mat;
    double lhs = PCA_NONE;
    uint32_t dim = pca->conf.dim;
    uint32_t lhs_idx = 0;
    uint32_t rhs_idx = 0;

    while (lhs_idx < dim)
    {
        lhs = row_ptr[lhs_idx] - pca->mean[lhs_idx];
        rhs_idx = 0;

        while (rhs_idx < dim)
        {
            mat[rhs_idx].re += lhs * (row_ptr[rhs_idx] - pca->mean[rhs_idx]);
            rhs_idx++;
        }

        mat += dim;
        lhs_idx++;
    }
}

static void pca_cov_fill(ldg_math_stat_pca_t *pca, const double *point, uint32_t row_cunt)
{
    const double *row_ptr = point;
    double divisor = (double)(row_cunt - PCA_BESSEL_OFF);
    uintptr_t elem_cunt = (uintptr_t)pca->conf.dim * (uintptr_t)pca->conf.dim;
    uintptr_t elem = 0;
    uint32_t row = 0;

    while (elem < elem_cunt)
    {
        pca->mat[elem].re = PCA_NONE;
        pca->mat[elem].im = PCA_NONE;
        elem++;
    }

    while (row < row_cunt)
    {
        pca_cov_row_cum(pca, row_ptr);
        row_ptr += pca->conf.dim;
        row++;
    }

    elem = 0;

    while (elem < elem_cunt)
    {
        pca->mat[elem].re /= divisor;
        elem++;
    }
}

static void pca_order_sort(ldg_math_stat_pca_t *pca)
{
    uint32_t idx = 0;
    uint32_t slot = 0;

    while (idx < pca->conf.dim)
    {
        slot = idx;

        while (slot > 0 && pca->val[pca->order[slot - PCA_ADJACENT_OFF]] < pca->val[idx])
        {
            pca->order[slot] = pca->order[slot - PCA_ADJACENT_OFF];
            slot--;
        }

        pca->order[slot] = idx;
        idx++;
    }
}

static void pca_comp_fill(ldg_math_stat_pca_t *pca, uint32_t comp)
{
    const ldg_math_complex_t *vec = pca->vec + pca->order[comp];
    double *basis = pca->basis + ((uintptr_t)comp * (uintptr_t)pca->conf.dim);
    double peak = PCA_NONE;
    double peak_abs = PCA_NONE;
    uint32_t dim = pca->conf.dim;
    uint32_t idx = 0;

    while (idx < dim)
    {
        basis[idx] = vec->re;

        if (ldg_math_abs_get(basis[idx]) > peak_abs)
        {
            peak = basis[idx];
            peak_abs = ldg_math_abs_get(basis[idx]);
        }

        vec += dim;
        idx++;
    }

    idx = 0;

    while (peak < PCA_NONE && idx < dim)
    {
        basis[idx] = -basis[idx];
        idx++;
    }
}

static void pca_basis_fill(ldg_math_stat_pca_t *pca)
{
    double val = PCA_NONE;
    uint32_t comp = 0;
    uint32_t idx = 0;

    pca->var_total = PCA_NONE;

    while (idx < pca->conf.dim)
    {
        if (pca->val[idx] > PCA_NONE) { pca->var_total += pca->val[idx]; }

        idx++;
    }

    while (comp < pca->conf.comp_cunt)
    {
        val = pca->val[pca->order[comp]];
        pca->var[comp] = (val > PCA_NONE) ? val : PCA_NONE;
        pca_comp_fill(pca, comp);
        comp++;
    }
}

uint32_t ldg_math_stat_pca_conf_default_get(ldg_math_stat_pca_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->dim = PCA_DIM_DEFAULT;
    out->comp_cunt = PCA_COMP_CUNT_DEFAULT;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_pca_init(ldg_math_stat_pca_t *pca, const ldg_math_stat_pca_conf_t *conf)
{
    void *ptr = 0x0;
    uint64_t dim = 0;
    uint64_t comp_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pca || !conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->dim == 0 || conf->comp_cunt == 0 || conf->comp_cunt > conf->dim)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_zero(pca, sizeof(*pca));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pca->conf = *conf;
    dim = (uint64_t)conf->dim;
    comp_cunt = (uint64_t)conf->comp_cunt;

    ret = pca_alloc(dim * dim * (uint64_t)sizeof(ldg_math_complex_t), &ptr);
    pca->mat = (ldg_math_complex_t *)ptr;

    if (ret == LDG_ERR_AOK) { ret = pca_alloc(dim * dim * (uint64_t)sizeof(ldg_math_complex_t), &ptr); }

    pca->vec = (ret == LDG_ERR_AOK) ? (ldg_math_complex_t *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = pca_alloc(dim * (uint64_t)sizeof(double), &ptr); }

    pca->val = (ret == LDG_ERR_AOK) ? (double *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = pca_alloc(dim * (uint64_t)sizeof(double), &ptr); }

    pca->mean = (ret == LDG_ERR_AOK) ? (double *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = pca_alloc(comp_cunt * dim * (uint64_t)sizeof(double), &ptr); }

    pca->basis = (ret == LDG_ERR_AOK) ? (double *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = pca_alloc(comp_cunt * (uint64_t)sizeof(double), &ptr); }

    pca->var = (ret == LDG_ERR_AOK) ? (double *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = pca_alloc(dim * (uint64_t)sizeof(uint32_t), &ptr); }

    pca->order = (ret == LDG_ERR_AOK) ? (uint32_t *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = ldg_math_eigen_ctx_create(dim, &pca->eigen); }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        pca_buff_free(pca);
        return ret;
    }

    pca->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_pca_shutdown(ldg_math_stat_pca_t *pca)
{
    uint32_t ret = pca_init_ck(pca);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pca_buff_free(pca);

    return ldg_mem_zero(pca, sizeof(*pca));
}

uint32_t ldg_math_stat_pca_rst(ldg_math_stat_pca_t *pca)
{
    uint32_t ret = pca_init_ck(pca);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pca->var_total = PCA_NONE;
    pca->is_fit = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_pca_fit(ldg_math_stat_pca_t *pca, const double *point, uint32_t row_cunt)
{
    uint32_t ret = pca_init_ck(pca);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!point)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(row_cunt < PCA_ROW_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = pca_elem_ck(point, (uintptr_t)row_cunt * (uintptr_t)pca->conf.dim);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pca->is_fit = LDG_TRUTH_FALSE;
    pca_mean_fill(pca, point, row_cunt);
    pca_cov_fill(pca, point, row_cunt);

    ret = ldg_math_eigen_hermitian_compute(pca->eigen, pca->mat, pca->val, pca->vec);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pca_order_sort(pca);
    pca_basis_fill(pca);
    pca->is_fit = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_pca_point_proj(const ldg_math_stat_pca_t *pca, const double *src, double *dst, uint32_t cap)
{
    const double *basis = 0x0;
    double cum = PCA_NONE;
    uint32_t comp = 0;
    uint32_t idx = 0;
    uint32_t ret = pca_fit_ck(pca);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!src || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap < pca->conf.comp_cunt)) { return LDG_ERR_OVERFLOW; }

    ret = pca_elem_ck(src, (uintptr_t)pca->conf.dim);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    basis = pca->basis;

    while (comp < pca->conf.comp_cunt)
    {
        cum = PCA_NONE;
        idx = 0;

        while (idx < pca->conf.dim)
        {
            cum += (src[idx] - pca->mean[idx]) * basis[idx];
            idx++;
        }

        dst[comp] = cum;
        basis += pca->conf.dim;
        comp++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_pca_point_reconstruct(const ldg_math_stat_pca_t *pca, const double *src, double *dst, uint32_t cap)
{
    const double *basis = 0x0;
    uint32_t comp = 0;
    uint32_t idx = 0;
    uint32_t ret = pca_fit_ck(pca);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!src || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap < pca->conf.dim)) { return LDG_ERR_OVERFLOW; }

    ret = pca_elem_ck(src, (uintptr_t)pca->conf.comp_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (idx < pca->conf.dim)
    {
        dst[idx] = pca->mean[idx];
        idx++;
    }

    basis = pca->basis;

    while (comp < pca->conf.comp_cunt)
    {
        idx = 0;

        while (idx < pca->conf.dim)
        {
            dst[idx] += src[comp] * basis[idx];
            idx++;
        }

        basis += pca->conf.dim;
        comp++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_pca_comp_get(const ldg_math_stat_pca_t *pca, uint32_t comp, double *dst, uint32_t cap)
{
    const double *basis = 0x0;
    uint32_t idx = 0;
    uint32_t ret = pca_fit_ck(pca);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(comp >= pca->conf.comp_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(cap < pca->conf.dim)) { return LDG_ERR_OVERFLOW; }

    basis = pca->basis + ((uintptr_t)comp * (uintptr_t)pca->conf.dim);

    while (idx < pca->conf.dim)
    {
        dst[idx] = basis[idx];
        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_pca_mean_get(const ldg_math_stat_pca_t *pca, double *dst, uint32_t cap)
{
    uint32_t idx = 0;
    uint32_t ret = pca_fit_ck(pca);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap < pca->conf.dim)) { return LDG_ERR_OVERFLOW; }

    while (idx < pca->conf.dim)
    {
        dst[idx] = pca->mean[idx];
        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_pca_var_get(const ldg_math_stat_pca_t *pca, uint32_t comp, double *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pca || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = PCA_NONE;

    ret = pca_fit_ck(pca);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(comp >= pca->conf.comp_cunt)) { return LDG_ERR_BOUNDS; }

    *out = pca->var[comp];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_pca_var_total_get(const ldg_math_stat_pca_t *pca, double *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pca || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = PCA_NONE;

    ret = pca_fit_ck(pca);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = pca->var_total;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_pca_state_cunt_get(const ldg_math_stat_pca_t *pca, uintptr_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pca || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = pca_init_ck(pca);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = pca_state_cunt_get(pca);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_pca_state_get(const ldg_math_stat_pca_t *pca, double *dst, uintptr_t cap, uintptr_t *out_cunt)
{
    double *lane = dst;
    uintptr_t basis_cunt = 0;
    uintptr_t elem = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pca || !dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = pca_fit_ck(pca);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < pca_state_cunt_get(pca))) { return LDG_ERR_OVERFLOW; }

    *lane = pca->var_total;
    lane += PCA_STATE_HDR_CUNT;

    while (idx < pca->conf.dim)
    {
        lane[idx] = pca->mean[idx];
        idx++;
    }

    lane += pca->conf.dim;
    idx = 0;

    while (idx < pca->conf.comp_cunt)
    {
        lane[idx] = pca->var[idx];
        idx++;
    }

    lane += pca->conf.comp_cunt;
    basis_cunt = (uintptr_t)pca->conf.comp_cunt * (uintptr_t)pca->conf.dim;

    while (elem < basis_cunt)
    {
        lane[elem] = pca->basis[elem];
        elem++;
    }

    *out_cunt = pca_state_cunt_get(pca);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_pca_state_set(ldg_math_stat_pca_t *pca, const double *src, uintptr_t cunt)
{
    const double *lane = src;
    uintptr_t basis_cunt = 0;
    uintptr_t elem = 0;
    uint32_t idx = 0;
    uint32_t ret = pca_init_ck(pca);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt != pca_state_cunt_get(pca))) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = pca_elem_ck(src, cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pca->var_total = *lane;
    lane += PCA_STATE_HDR_CUNT;

    while (idx < pca->conf.dim)
    {
        pca->mean[idx] = lane[idx];
        idx++;
    }

    lane += pca->conf.dim;
    idx = 0;

    while (idx < pca->conf.comp_cunt)
    {
        pca->var[idx] = lane[idx];
        idx++;
    }

    lane += pca->conf.comp_cunt;
    basis_cunt = (uintptr_t)pca->conf.comp_cunt * (uintptr_t)pca->conf.dim;

    while (elem < basis_cunt)
    {
        pca->basis[elem] = lane[elem];
        elem++;
    }

    pca->is_fit = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}
