#include <string.h>

#include <dangling/math/krylov.h>
#include <dangling/math/sparse.h>
#include <dangling/math/vec.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

static uint32_t krylov_alloc_doubles(uint64_t cunt, double **out)
{
    void *tmp = 0x0;
    uint32_t ret = 0;

    ret = ldg_mem_alloc(cunt * (uint64_t)sizeof(double), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_MATH_ALLOC; }

    *out = (double *)tmp;

    return LDG_MATH_OK;
}

static uint32_t krylov_alloc_u64(uint64_t cunt, uint64_t **out)
{
    void *tmp = 0x0;
    uint32_t ret = 0;

    ret = ldg_mem_alloc(cunt * (uint64_t)sizeof(uint64_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_MATH_ALLOC; }

    *out = (uint64_t *)tmp;

    return LDG_MATH_OK;
}

static void krylov_ctx_destroy_internal(ldg_math_krylov_ctx_t *ctx)
{
    if (ctx->resid) { ldg_mem_dealloc(ctx->resid); }

    if (ctx->dir) { ldg_mem_dealloc(ctx->dir); }

    if (ctx->a_dir) { ldg_mem_dealloc(ctx->a_dir); }

    if (ctx->precond_resid) { ldg_mem_dealloc(ctx->precond_resid); }

    if (ctx->inv_diag) { ldg_mem_dealloc(ctx->inv_diag); }

    if (ctx->l_row_ptr) { ldg_mem_dealloc(ctx->l_row_ptr); }

    if (ctx->l_col_idx) { ldg_mem_dealloc(ctx->l_col_idx); }

    if (ctx->l_vals) { ldg_mem_dealloc(ctx->l_vals); }

    if (ctx->l_diag) { ldg_mem_dealloc(ctx->l_diag); }

    ldg_mem_dealloc(ctx);
}

static uint32_t krylov_ic0_copy_lower(ldg_math_krylov_ctx_t *ctx, const ldg_math_sparse_t *mat)
{
    uint64_t row = 0;
    uint64_t nz = 0;
    uint64_t end = 0;
    uint64_t col = 0;
    uint64_t pos = 0;

    pos = 0;
    ctx->l_row_ptr[0] = 0;

    for (row = 0; row < ctx->dim; row++)
    {
        end = mat->row_ptr[row + 1];
        for (nz = mat->row_ptr[row]; nz < end; nz++)
        {
            col = mat->col_idx[nz];
            if (col > row) { break; }

            if (LDG_UNLIKELY(pos >= ctx->max_nnz)) { return LDG_MATH_BOUNDS; }

            ctx->l_col_idx[pos] = col;
            ctx->l_vals[pos] = mat->vals[nz];
            pos++;
        }
        ctx->l_row_ptr[row + 1] = pos;
    }

    ctx->l_nnz = pos;

    return LDG_MATH_OK;
}

static double krylov_ic0_row_dot(const ldg_math_krylov_ctx_t *ctx, uint64_t row_a, uint64_t row_b, uint64_t limit)
{
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    uint64_t end_a = 0;
    uint64_t end_b = 0;
    uint64_t col_a = 0;
    uint64_t col_b = 0;
    double acc = 0.0;

    idx_a = ctx->l_row_ptr[row_a];
    end_a = ctx->l_row_ptr[row_a + 1];
    idx_b = ctx->l_row_ptr[row_b];
    end_b = ctx->l_row_ptr[row_b + 1];

    while (idx_a < end_a && idx_b < end_b)
    {
        col_a = ctx->l_col_idx[idx_a];
        col_b = ctx->l_col_idx[idx_b];

        if (col_a >= limit || col_b >= limit) { break; }

        if (col_a == col_b)
        {
            acc += ctx->l_vals[idx_a] * ctx->l_vals[idx_b];
            idx_a++;
            idx_b++;
        }
        else if (col_a < col_b) { idx_a++; }
        else { idx_b++; }
    }

    return acc;
}

static uint32_t krylov_ic0_factor(ldg_math_krylov_ctx_t *ctx)
{
    uint64_t row = 0;
    uint64_t pos = 0;
    uint64_t end = 0;
    uint64_t col = 0;
    uint8_t diag_found = 0;
    double dot = 0.0;
    double radicand = 0.0;
    double pivot = 0.0;

    for (row = 0; row < ctx->dim; row++)
    {
        diag_found = 0;
        end = ctx->l_row_ptr[row + 1];

        for (pos = ctx->l_row_ptr[row]; pos < end; pos++)
        {
            col = ctx->l_col_idx[pos];

            if (col == row)
            {
                dot = krylov_ic0_row_dot(ctx, row, row, row);
                radicand = ctx->l_vals[pos] - dot;
                if (LDG_UNLIKELY(radicand <= 0.0)) { return LDG_MATH_PIVOT; }

                pivot = __builtin_sqrt(radicand);
                ctx->l_vals[pos] = pivot;
                ctx->l_diag[row] = pivot;
                diag_found = 1;
            }
            else
            {
                if (LDG_UNLIKELY(ctx->l_diag[col] <= 0.0)) { return LDG_MATH_PIVOT; }

                dot = krylov_ic0_row_dot(ctx, row, col, col);
                ctx->l_vals[pos] = (ctx->l_vals[pos] - dot) / ctx->l_diag[col];
            }
        }

        if (LDG_UNLIKELY(!diag_found)) { return LDG_MATH_PIVOT; }
    }

    return LDG_MATH_OK;
}

static uint32_t krylov_precond_apply(const ldg_math_krylov_ctx_t *ctx, const double *src, double *dst)
{
    uint64_t row = 0;
    uint64_t pos = 0;
    uint64_t end = 0;
    double acc = 0.0;
    uint32_t ret = 0;

    if (ctx->precond_kind == LDG_MATH_KRYLOV_PRECOND_NONE) { return ldg_math_vec_cpy(dst, src, ctx->dim); }

    if (ctx->precond_kind == LDG_MATH_KRYLOV_PRECOND_JACOBI) { return ldg_math_vec_mul_elem(dst, ctx->inv_diag, src, ctx->dim); }

    ret = ldg_math_vec_cpy(dst, src, ctx->dim);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    for (row = 0; row < ctx->dim; row++)
    {
        acc = dst[row];
        end = ctx->l_row_ptr[row + 1];
        for (pos = ctx->l_row_ptr[row]; pos < end; pos++) { if (ctx->l_col_idx[pos] < row) { acc -= ctx->l_vals[pos] * dst[ctx->l_col_idx[pos]]; } }
        dst[row] = acc / ctx->l_diag[row];
    }

    row = ctx->dim;
    while (row > 0)
    {
        row--;
        dst[row] = dst[row] / ctx->l_diag[row];
        end = ctx->l_row_ptr[row + 1];
        for (pos = ctx->l_row_ptr[row]; pos < end; pos++) { if (ctx->l_col_idx[pos] < row) { dst[ctx->l_col_idx[pos]] -= ctx->l_vals[pos] * dst[row]; } }
    }

    return LDG_MATH_OK;
}

LDG_EXPORT uint32_t ldg_math_krylov_ctx_create(uint64_t max_dim, uint64_t max_nnz, ldg_math_krylov_precond_t precond_kind, ldg_math_krylov_ctx_t **out)
{
    ldg_math_krylov_ctx_t *ctx = 0x0;
    void *tmp = 0x0;
    uint32_t ret = 0;
    uint32_t kind = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_MATH_ARG_NULL; }

    *out = 0x0;

    kind = (uint32_t)precond_kind;

    if (LDG_UNLIKELY(max_dim == 0)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(kind != LDG_MATH_KRYLOV_PRECOND_NONE && kind != LDG_MATH_KRYLOV_PRECOND_JACOBI && kind != LDG_MATH_KRYLOV_PRECOND_IC0)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(kind == LDG_MATH_KRYLOV_PRECOND_IC0 && max_nnz == 0)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(max_dim > UINT64_MAX / (uint64_t)sizeof(double) - 1)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(max_nnz > UINT64_MAX / (uint64_t)sizeof(double))) { return LDG_MATH_ARG_INVALID; }

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_math_krylov_ctx_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_MATH_ALLOC; }

    ctx = (ldg_math_krylov_ctx_t *)tmp;
    if (LDG_UNLIKELY(memset(ctx, 0, sizeof(ldg_math_krylov_ctx_t)) != ctx))
    {
        ldg_mem_dealloc(ctx);
        return LDG_MATH_ALLOC;
    }

    ctx->max_dim = max_dim;
    ctx->max_nnz = max_nnz;
    ctx->precond_kind = kind;

    ret = krylov_alloc_doubles(max_dim, &ctx->resid);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        krylov_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = krylov_alloc_doubles(max_dim, &ctx->dir);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        krylov_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = krylov_alloc_doubles(max_dim, &ctx->a_dir);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        krylov_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = krylov_alloc_doubles(max_dim, &ctx->precond_resid);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        krylov_ctx_destroy_internal(ctx);
        return ret;
    }

    if (kind == LDG_MATH_KRYLOV_PRECOND_JACOBI)
    {
        ret = krylov_alloc_doubles(max_dim, &ctx->inv_diag);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK))
        {
            krylov_ctx_destroy_internal(ctx);
            return ret;
        }
    }

    if (kind == LDG_MATH_KRYLOV_PRECOND_IC0)
    {
        ret = krylov_alloc_u64(max_dim + 1, &ctx->l_row_ptr);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK))
        {
            krylov_ctx_destroy_internal(ctx);
            return ret;
        }

        ret = krylov_alloc_u64(max_nnz, &ctx->l_col_idx);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK))
        {
            krylov_ctx_destroy_internal(ctx);
            return ret;
        }

        ret = krylov_alloc_doubles(max_nnz, &ctx->l_vals);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK))
        {
            krylov_ctx_destroy_internal(ctx);
            return ret;
        }

        ret = krylov_alloc_doubles(max_dim, &ctx->l_diag);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK))
        {
            krylov_ctx_destroy_internal(ctx);
            return ret;
        }
    }

    *out = ctx;

    return LDG_MATH_OK;
}

LDG_EXPORT uint32_t ldg_math_krylov_ctx_destroy(ldg_math_krylov_ctx_t **ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(!*ctx)) { return LDG_MATH_OK; }

    krylov_ctx_destroy_internal(*ctx);
    *ctx = 0x0;

    return LDG_MATH_OK;
}

LDG_EXPORT uint32_t ldg_math_krylov_precond_build(ldg_math_krylov_ctx_t *ctx, const ldg_math_sparse_t *mat)
{
    uint64_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !mat)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(!mat->finalized)) { return LDG_MATH_STATE; }

    if (LDG_UNLIKELY(mat->rows != mat->cols)) { return LDG_MATH_DIM_MISMATCH; }

    if (LDG_UNLIKELY(mat->rows > ctx->max_dim)) { return LDG_MATH_BOUNDS; }

    ctx->dim = mat->rows;
    ctx->precond_built = 0;

    if (ctx->precond_kind == LDG_MATH_KRYLOV_PRECOND_NONE)
    {
        ctx->precond_built = 1;
        return LDG_MATH_OK;
    }

    if (ctx->precond_kind == LDG_MATH_KRYLOV_PRECOND_JACOBI)
    {
        ret = ldg_math_sparse_diag_get(mat, ctx->inv_diag);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        for (row = 0; row < ctx->dim; row++)
        {
            if (LDG_UNLIKELY(ctx->inv_diag[row] <= 0.0)) { return LDG_MATH_PIVOT; }

            ctx->inv_diag[row] = 1.0 / ctx->inv_diag[row];
        }

        ctx->precond_built = 1;
        return LDG_MATH_OK;
    }

    ret = krylov_ic0_copy_lower(ctx, mat);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = krylov_ic0_factor(ctx);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ctx->precond_built = 1;

    return LDG_MATH_OK;
}

LDG_EXPORT uint32_t ldg_math_krylov_cg_solve(ldg_math_krylov_ctx_t *ctx, const ldg_math_sparse_t *mat, const double *rhs, double *sol, double tol, uint64_t max_iters, ldg_math_krylov_res_t *res)
{
    uint64_t dim = 0;
    uint64_t iter = 0;
    double denom = 0.0;
    double rhs_norm = 0.0;
    double resid_norm = 0.0;
    double rel = 0.0;
    double rz_prev = 0.0;
    double rz_curr = 0.0;
    double dir_a_dir = 0.0;
    double alpha = 0.0;
    double beta = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !mat || !rhs || !sol || !res)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(!mat->finalized)) { return LDG_MATH_STATE; }

    if (LDG_UNLIKELY(mat->rows != mat->cols)) { return LDG_MATH_DIM_MISMATCH; }

    if (LDG_UNLIKELY(mat->rows > ctx->max_dim)) { return LDG_MATH_BOUNDS; }

    if (LDG_UNLIKELY(max_iters == 0 || tol <= 0.0)) { return LDG_MATH_ARG_INVALID; }

    dim = mat->rows;

    res->iters = 0;
    res->rel_resid = 0.0;
    res->converged = 0;

    ret = ldg_math_vec_norm2_get(rhs, dim, &rhs_norm);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    denom = (rhs_norm > 0.0) ? rhs_norm : 1.0;

    ret = ldg_math_sparse_mv(mat, sol, ctx->a_dir);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = ldg_math_vec_sub(ctx->resid, rhs, ctx->a_dir, dim);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = ldg_math_vec_norm2_get(ctx->resid, dim, &resid_norm);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    rel = resid_norm / denom;
    res->rel_resid = rel;
    if (rel <= tol)
    {
        res->converged = 1;
        return LDG_MATH_OK;
    }

    ret = ldg_math_vec_cpy(ctx->dir, ctx->resid, dim);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = ldg_math_vec_dot_get(ctx->resid, ctx->resid, dim, &rz_prev);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    for (iter = 1; iter <= max_iters; iter++)
    {
        ret = ldg_math_sparse_mv(mat, ctx->dir, ctx->a_dir);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_vec_dot_get(ctx->dir, ctx->a_dir, dim, &dir_a_dir);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        if (LDG_UNLIKELY(dir_a_dir <= 0.0)) { return LDG_MATH_NOT_SPD; }

        alpha = rz_prev / dir_a_dir;

        ret = ldg_math_vec_axpy(sol, ctx->dir, alpha, dim);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_vec_axpy(ctx->resid, ctx->a_dir, -alpha, dim);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_vec_norm2_get(ctx->resid, dim, &resid_norm);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        rel = resid_norm / denom;
        res->iters = iter;
        res->rel_resid = rel;
        if (rel <= tol)
        {
            res->converged = 1;
            return LDG_MATH_OK;
        }

        ret = ldg_math_vec_dot_get(ctx->resid, ctx->resid, dim, &rz_curr);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        if (LDG_UNLIKELY(rz_prev <= 0.0)) { return LDG_MATH_NOT_SPD; }

        beta = rz_curr / rz_prev;

        ret = ldg_math_vec_scale(ctx->dir, beta, dim);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_vec_axpy(ctx->dir, ctx->resid, 1.0, dim);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        rz_prev = rz_curr;
    }

    return LDG_MATH_NOT_CONVERGED;
}

LDG_EXPORT uint32_t ldg_math_krylov_pcg_solve(ldg_math_krylov_ctx_t *ctx, const ldg_math_sparse_t *mat, const double *rhs, double *sol, double tol, uint64_t max_iters, ldg_math_krylov_res_t *res)
{
    uint64_t dim = 0;
    uint64_t iter = 0;
    double denom = 0.0;
    double rhs_norm = 0.0;
    double resid_norm = 0.0;
    double rel = 0.0;
    double rz_prev = 0.0;
    double rz_curr = 0.0;
    double dir_a_dir = 0.0;
    double alpha = 0.0;
    double beta = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !mat || !rhs || !sol || !res)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(!mat->finalized)) { return LDG_MATH_STATE; }

    if (LDG_UNLIKELY(mat->rows != mat->cols)) { return LDG_MATH_DIM_MISMATCH; }

    if (LDG_UNLIKELY(mat->rows > ctx->max_dim)) { return LDG_MATH_BOUNDS; }

    if (LDG_UNLIKELY(max_iters == 0 || tol <= 0.0)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(!ctx->precond_built)) { return LDG_MATH_STATE; }

    dim = mat->rows;
    if (LDG_UNLIKELY(ctx->dim != dim)) { return LDG_MATH_DIM_MISMATCH; }

    res->iters = 0;
    res->rel_resid = 0.0;
    res->converged = 0;

    ret = ldg_math_vec_norm2_get(rhs, dim, &rhs_norm);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    denom = (rhs_norm > 0.0) ? rhs_norm : 1.0;

    ret = ldg_math_sparse_mv(mat, sol, ctx->a_dir);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = ldg_math_vec_sub(ctx->resid, rhs, ctx->a_dir, dim);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = ldg_math_vec_norm2_get(ctx->resid, dim, &resid_norm);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    rel = resid_norm / denom;
    res->rel_resid = rel;
    if (rel <= tol)
    {
        res->converged = 1;
        return LDG_MATH_OK;
    }

    ret = krylov_precond_apply(ctx, ctx->resid, ctx->precond_resid);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = ldg_math_vec_cpy(ctx->dir, ctx->precond_resid, dim);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = ldg_math_vec_dot_get(ctx->resid, ctx->precond_resid, dim, &rz_prev);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    for (iter = 1; iter <= max_iters; iter++)
    {
        ret = ldg_math_sparse_mv(mat, ctx->dir, ctx->a_dir);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_vec_dot_get(ctx->dir, ctx->a_dir, dim, &dir_a_dir);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        if (LDG_UNLIKELY(dir_a_dir <= 0.0)) { return LDG_MATH_NOT_SPD; }

        alpha = rz_prev / dir_a_dir;

        ret = ldg_math_vec_axpy(sol, ctx->dir, alpha, dim);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_vec_axpy(ctx->resid, ctx->a_dir, -alpha, dim);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_vec_norm2_get(ctx->resid, dim, &resid_norm);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        rel = resid_norm / denom;
        res->iters = iter;
        res->rel_resid = rel;
        if (rel <= tol)
        {
            res->converged = 1;
            return LDG_MATH_OK;
        }

        ret = krylov_precond_apply(ctx, ctx->resid, ctx->precond_resid);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_vec_dot_get(ctx->resid, ctx->precond_resid, dim, &rz_curr);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        if (LDG_UNLIKELY(rz_prev <= 0.0)) { return LDG_MATH_NOT_SPD; }

        beta = rz_curr / rz_prev;

        ret = ldg_math_vec_scale(ctx->dir, beta, dim);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_vec_axpy(ctx->dir, ctx->precond_resid, 1.0, dim);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        rz_prev = rz_curr;
    }

    return LDG_MATH_NOT_CONVERGED;
}
