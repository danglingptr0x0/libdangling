
#include <dangling/math/lu.h>
#include <dangling/math/complex.h>
#include <dangling/math/math.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

static void lu_permute_apply(const ldg_math_lu_ctx_t *ctx, ldg_math_complex_t *vec, uint64_t stride)
{
    ldg_math_complex_t held = LDG_STRUCT_ZERO_INIT;
    uint64_t step = 0;

    for (step = 0; step < ctx->dim; step++) { if (ctx->pivot[step] != step)
        {
            held = vec[step * stride];
            vec[step * stride] = vec[ctx->pivot[step] * stride];
            vec[ctx->pivot[step] * stride] = held;
        }
    }
}

static void lu_substitute_run(const ldg_math_lu_ctx_t *ctx, ldg_math_complex_t *vec, uint64_t stride)
{
    const ldg_math_complex_t *entry = 0x0;
    ldg_math_complex_t inv = LDG_STRUCT_ZERO_INIT;
    ldg_math_complex_t held = LDG_STRUCT_ZERO_INIT;
    double denom = 0.0;
    uint64_t col = 0;
    uint64_t row = 0;
    uint64_t back = 0;

    for (col = 0; col < ctx->dim; col++) { for (row = col + 1; row < ctx->dim; row++)
        {
            entry = &ctx->factor[row * ctx->dim + col];
            vec[row * stride].re -= entry->re * vec[col * stride].re - entry->im * vec[col * stride].im;
            vec[row * stride].im -= entry->re * vec[col * stride].im + entry->im * vec[col * stride].re;
        }
    }
    for (back = 0; back < ctx->dim; back++)
    {
        col = ctx->dim - 1 - back;
        entry = &ctx->factor[col * ctx->dim + col];
        denom = entry->re * entry->re + entry->im * entry->im;
        inv.re = entry->re / denom;
        inv.im = (0.0 - entry->im) / denom;
        held = vec[col * stride];
        vec[col * stride].re = held.re * inv.re - held.im * inv.im;
        vec[col * stride].im = held.re * inv.im + held.im * inv.re;
        for (row = 0; row < col; row++)
        {
            entry = &ctx->factor[row * ctx->dim + col];
            vec[row * stride].re -= entry->re * vec[col * stride].re - entry->im * vec[col * stride].im;
            vec[row * stride].im -= entry->re * vec[col * stride].im + entry->im * vec[col * stride].re;
        }
    }
}

LDG_EXPORT uint32_t ldg_math_lu_ctx_create(uint64_t dim, ldg_math_lu_ctx_t **out)
{
    ldg_math_lu_ctx_t *ctx = 0x0;
    void *blk = 0x0;
    uint64_t square = 0;
    uint32_t ret = 0;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    if (LDG_UNLIKELY(dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(dim > UINT64_MAX / dim)) { return LDG_ERR_OVERFLOW; }

    square = dim * dim;
    if (LDG_UNLIKELY(square > UINT64_MAX / (uint64_t)sizeof(ldg_math_complex_t))) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_math_lu_ctx_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    ctx = (ldg_math_lu_ctx_t *)blk;
    if (LDG_UNLIKELY(ldg_mem_zero(ctx, sizeof(ldg_math_lu_ctx_t)) != LDG_ERR_AOK))
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
    ret = ldg_mem_alloc(dim * (uint64_t)sizeof(uint64_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_mem_dealloc(ctx->factor);
        ret = ldg_mem_dealloc(ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->pivot = (uint64_t *)blk;
    *out = ctx;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_lu_ctx_destroy(ldg_math_lu_ctx_t **ctx)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !*ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if ((*ctx)->factor != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->factor);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->pivot != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->pivot);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    err = ldg_mem_dealloc(*ctx);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    *ctx = 0x0;
    return ret;
}

LDG_EXPORT uint32_t ldg_math_lu_factor_compute(ldg_math_lu_ctx_t *ctx, const ldg_math_complex_t *mat)
{
    ldg_math_complex_t *pivot_row = 0x0;
    ldg_math_complex_t *swap_row = 0x0;
    ldg_math_complex_t *work_row = 0x0;
    ldg_math_complex_t inv = LDG_STRUCT_ZERO_INIT;
    ldg_math_complex_t mult = LDG_STRUCT_ZERO_INIT;
    ldg_math_complex_t held = LDG_STRUCT_ZERO_INIT;
    double best_val = 0.0;
    double cand_val = 0.0;
    double denom = 0.0;
    uint64_t step = 0;
    uint64_t row = 0;
    uint64_t col = 0;
    uint64_t best_idx = 0;
    if (LDG_UNLIKELY(!ctx || !mat)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->ready = LDG_TRUTH_FALSE;
    ctx->swap_cunt = 0;
    if (LDG_UNLIKELY(ldg_mem_cpy(ctx->factor, mat, ctx->dim * ctx->dim * sizeof(ldg_math_complex_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    for (step = 0; step < ctx->dim; step++)
    {
        best_idx = step;
        best_val = ctx->factor[step * ctx->dim + step].re * ctx->factor[step * ctx->dim + step].re + ctx->factor[step * ctx->dim + step].im * ctx->factor[step * ctx->dim + step].im;
        for (row = step + 1; row < ctx->dim; row++)
        {
            cand_val = ctx->factor[row * ctx->dim + step].re * ctx->factor[row * ctx->dim + step].re + ctx->factor[row * ctx->dim + step].im * ctx->factor[row * ctx->dim + step].im;
            if (cand_val > best_val)
            {
                best_val = cand_val;
                best_idx = row;
            }
        }
        ctx->pivot[step] = best_idx;
        if (LDG_UNLIKELY(!(best_val > 0.0))) { return LDG_ERR_MATH_PIVOT; }

        if (best_idx != step)
        {
            pivot_row = &ctx->factor[step * ctx->dim];
            swap_row = &ctx->factor[best_idx * ctx->dim];
            for (col = 0; col < ctx->dim; col++)
            {
                held = pivot_row[col];
                pivot_row[col] = swap_row[col];
                swap_row[col] = held;
            }
            ctx->swap_cunt += 1;
        }

        pivot_row = &ctx->factor[step * ctx->dim];
        denom = pivot_row[step].re * pivot_row[step].re + pivot_row[step].im * pivot_row[step].im;
        inv.re = pivot_row[step].re / denom;
        inv.im = (0.0 - pivot_row[step].im) / denom;
        for (row = step + 1; row < ctx->dim; row++)
        {
            work_row = &ctx->factor[row * ctx->dim];
            mult.re = work_row[step].re * inv.re - work_row[step].im * inv.im;
            mult.im = work_row[step].re * inv.im + work_row[step].im * inv.re;
            work_row[step] = mult;
            for (col = step + 1; col < ctx->dim; col++)
            {
                work_row[col].re -= mult.re * pivot_row[col].re - mult.im * pivot_row[col].im;
                work_row[col].im -= mult.re * pivot_row[col].im + mult.im * pivot_row[col].re;
            }
        }
    }
    ctx->ready = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_lu_solve_compute(const ldg_math_lu_ctx_t *ctx, const ldg_math_complex_t *rhs, uint64_t rhs_cunt, ldg_math_complex_t *out)
{
    ldg_math_complex_t *col_vec = 0x0;
    uint64_t idx = 0;
    if (LDG_UNLIKELY(!ctx || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(rhs_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->ready != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_STATE; }

    if (LDG_UNLIKELY(ldg_mem_cpy(out, rhs, rhs_cunt * ctx->dim * sizeof(ldg_math_complex_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    for (idx = 0; idx < rhs_cunt; idx++)
    {
        col_vec = &out[idx * ctx->dim];
        lu_permute_apply(ctx, col_vec, 1);
        lu_substitute_run(ctx, col_vec, 1);
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_lu_invert_compute(const ldg_math_lu_ctx_t *ctx, ldg_math_complex_t *out)
{
    uint64_t col = 0;
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->ready != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_STATE; }

    if (LDG_UNLIKELY(ldg_mem_zero(out, ctx->dim * ctx->dim * sizeof(ldg_math_complex_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    for (col = 0; col < ctx->dim; col++)
    {
        out[col * ctx->dim + col].re = 1.0;
        lu_permute_apply(ctx, &out[col], ctx->dim);
        lu_substitute_run(ctx, &out[col], ctx->dim);
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_lu_det_get(const ldg_math_lu_ctx_t *ctx, ldg_math_complex_t *out)
{
    ldg_math_complex_t prod = LDG_STRUCT_ZERO_INIT;
    ldg_math_complex_t held = LDG_STRUCT_ZERO_INIT;
    uint64_t step = 0;
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->re = 0.0;
    out->im = 0.0;
    if (LDG_UNLIKELY(ctx->ready != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_STATE; }

    prod.re = 1.0;
    for (step = 0; step < ctx->dim; step++)
    {
        held = prod;
        prod.re = held.re * ctx->factor[step * ctx->dim + step].re - held.im * ctx->factor[step * ctx->dim + step].im;
        prod.im = held.re * ctx->factor[step * ctx->dim + step].im + held.im * ctx->factor[step * ctx->dim + step].re;
    }
    if (ctx->swap_cunt % 2 == 1)
    {
        prod.re = 0.0 - prod.re;
        prod.im = 0.0 - prod.im;
    }

    *out = prod;
    return LDG_ERR_AOK;
}
