
#include <dangling/math/cholesky.h>
#include <dangling/math/complex.h>
#include <dangling/math/math.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

LDG_EXPORT uint32_t ldg_math_cholesky_ctx_create(uint64_t dim, ldg_math_cholesky_ctx_t **out)
{
    ldg_math_cholesky_ctx_t *ctx = 0x0;
    void *blk = 0x0;
    uint64_t square = 0;
    uint32_t ret = 0;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    if (LDG_UNLIKELY(dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(dim > UINT64_MAX / dim)) { return LDG_ERR_OVERFLOW; }

    square = dim * dim;
    if (LDG_UNLIKELY(square > UINT64_MAX / (uint64_t)sizeof(ldg_math_complex_t))) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_math_cholesky_ctx_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    ctx = (ldg_math_cholesky_ctx_t *)blk;
    if (LDG_UNLIKELY(ldg_mem_zero(ctx, sizeof(ldg_math_cholesky_ctx_t)) != LDG_ERR_AOK))
    {
        ret = ldg_mem_dealloc(ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->dim = dim;
    ret = ldg_mem_alloc(square * (uint64_t)sizeof(ldg_math_complex_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_mem_dealloc(ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->factor = (ldg_math_complex_t *)blk;
    *out = ctx;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_cholesky_ctx_destroy(ldg_math_cholesky_ctx_t **ctx)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !*ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if ((*ctx)->factor != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->factor);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    err = ldg_mem_dealloc(*ctx);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    *ctx = 0x0;
    return ret;
}

LDG_EXPORT uint32_t ldg_math_cholesky_factor_compute(ldg_math_cholesky_ctx_t *ctx, const ldg_math_complex_t *mat)
{
    ldg_math_complex_t *pivot_col = 0x0;
    ldg_math_complex_t *work_row = 0x0;
    double pivot_val = 0.0;
    double root = 0.0;
    uint64_t step = 0;
    uint64_t row = 0;
    uint64_t col = 0;
    if (LDG_UNLIKELY(!ctx || !mat)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->ready = LDG_TRUTH_FALSE;
    if (LDG_UNLIKELY(ldg_mem_cpy(ctx->factor, mat, ctx->dim * ctx->dim * sizeof(ldg_math_complex_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    for (row = 0; row < ctx->dim; row++) { ctx->factor[row * ctx->dim + row].im = 0.0; }
    for (step = 0; step < ctx->dim; step++)
    {
        pivot_val = ctx->factor[step * ctx->dim + step].re;
        if (LDG_UNLIKELY(!(pivot_val > 0.0))) { return LDG_ERR_MATH_NOT_SPD; }

        root = ldg_math_sqrt_get(pivot_val);
        ctx->factor[step * ctx->dim + step].re = root;
        ctx->factor[step * ctx->dim + step].im = 0.0;
        for (row = step + 1; row < ctx->dim; row++)
        {
            ctx->factor[row * ctx->dim + step].re /= root;
            ctx->factor[row * ctx->dim + step].im /= root;
        }
        for (row = step + 1; row < ctx->dim; row++)
        {
            work_row = &ctx->factor[row * ctx->dim];
            pivot_col = &ctx->factor[row * ctx->dim + step];
            for (col = step + 1; col <= row; col++)
            {
                work_row[col].re -= pivot_col->re * ctx->factor[col * ctx->dim + step].re + pivot_col->im * ctx->factor[col * ctx->dim + step].im;
                work_row[col].im -= pivot_col->im * ctx->factor[col * ctx->dim + step].re - pivot_col->re * ctx->factor[col * ctx->dim + step].im;
            }
        }
    }
    ctx->ready = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_cholesky_solve_compute(const ldg_math_cholesky_ctx_t *ctx, const ldg_math_complex_t *rhs, uint64_t rhs_cunt, ldg_math_complex_t *out)
{
    const ldg_math_complex_t *entry = 0x0;
    ldg_math_complex_t *col_vec = 0x0;
    double diag_val = 0.0;
    uint64_t idx = 0;
    uint64_t row = 0;
    uint64_t col = 0;
    uint64_t back = 0;
    if (LDG_UNLIKELY(!ctx || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(rhs_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->ready != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_STATE; }

    if (LDG_UNLIKELY(ldg_mem_cpy(out, rhs, rhs_cunt * ctx->dim * sizeof(ldg_math_complex_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    for (idx = 0; idx < rhs_cunt; idx++)
    {
        col_vec = &out[idx * ctx->dim];
        for (row = 0; row < ctx->dim; row++)
        {
            for (col = 0; col < row; col++)
            {
                entry = &ctx->factor[row * ctx->dim + col];
                col_vec[row].re -= entry->re * col_vec[col].re - entry->im * col_vec[col].im;
                col_vec[row].im -= entry->re * col_vec[col].im + entry->im * col_vec[col].re;
            }
            diag_val = ctx->factor[row * ctx->dim + row].re;
            col_vec[row].re /= diag_val;
            col_vec[row].im /= diag_val;
        }
        for (back = 0; back < ctx->dim; back++)
        {
            row = ctx->dim - 1 - back;
            for (col = row + 1; col < ctx->dim; col++)
            {
                entry = &ctx->factor[col * ctx->dim + row];
                col_vec[row].re -= entry->re * col_vec[col].re + entry->im * col_vec[col].im;
                col_vec[row].im -= entry->re * col_vec[col].im - entry->im * col_vec[col].re;
            }
            diag_val = ctx->factor[row * ctx->dim + row].re;
            col_vec[row].re /= diag_val;
            col_vec[row].im /= diag_val;
        }
    }
    return LDG_ERR_AOK;
}
