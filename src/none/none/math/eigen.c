#include <float.h>

#include <dangling/math/eigen.h>
#include <dangling/math/cholesky.h>
#include <dangling/math/complex.h>
#include <dangling/math/sparse.h>
#include <dangling/math/math.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

#define LDG_MATH_EIGEN_SWEEP_CAP 30U

static uint32_t eigen_tridiag_qr_run(double *diag, double *offdiag, uint64_t dim, double *rot, uint64_t rot_rows)
{
    double shift_gap = 0.0;
    double shift_off = 0.0;
    double shift_mu = 0.0;
    double shift_sgn = 0.0;
    double chase_x = 0.0;
    double chase_z = 0.0;
    double givens_r = 0.0;
    double givens_c = 0.0;
    double givens_s = 0.0;
    double diag_lead = 0.0;
    double diag_trail = 0.0;
    double off_val = 0.0;
    double held_a = 0.0;
    double held_b = 0.0;
    double min_val = 0.0;
    uint64_t hi = 0;
    uint64_t lo = 0;
    uint64_t chase = 0;
    uint64_t row = 0;
    uint64_t idx = 0;
    uint64_t scan = 0;
    uint64_t min_idx = 0;
    uint64_t sweep_cunt = 0;

    hi = dim - 1;
    while (hi > 0)
    {
        lo = hi;
        while (lo > 0)
        {
            if (ldg_math_abs_get(offdiag[lo - 1]) <= DBL_EPSILON * (ldg_math_abs_get(diag[lo - 1]) + ldg_math_abs_get(diag[lo])))
            {
                offdiag[lo - 1] = 0.0;
                break;
            }

            lo--;
        }
        if (lo == hi)
        {
            hi--;
            continue;
        }

        sweep_cunt++;
        if (LDG_UNLIKELY(sweep_cunt > (uint64_t)LDG_MATH_EIGEN_SWEEP_CAP * dim)) { return LDG_ERR_MATH_NOT_CONVERGED; }

        shift_gap = (diag[hi - 1] - diag[hi]) / 2.0;
        shift_off = offdiag[hi - 1];
        shift_sgn = (shift_gap >= 0.0) ? 1.0 : (0.0 - 1.0);
        shift_mu = diag[hi] - shift_off * shift_off / (shift_gap + shift_sgn * ldg_math_sqrt_get(shift_gap * shift_gap + shift_off * shift_off));
        chase_x = diag[lo] - shift_mu;
        chase_z = offdiag[lo];
        for (chase = lo; chase < hi; chase++)
        {
            givens_r = ldg_math_hypotenuse_get(chase_x, chase_z);
            if (givens_r > 0.0)
            {
                givens_c = chase_x / givens_r;
                givens_s = chase_z / givens_r;
            }
            else
            {
                givens_c = 1.0;
                givens_s = 0.0;
            }

            if (chase > lo) { offdiag[chase - 1] = givens_r; }

            diag_lead = diag[chase];
            off_val = offdiag[chase];
            diag_trail = diag[chase + 1];
            diag[chase] = givens_c * givens_c * diag_lead + 2.0 * givens_c * givens_s * off_val + givens_s * givens_s * diag_trail;
            diag[chase + 1] = givens_s * givens_s * diag_lead - 2.0 * givens_c * givens_s * off_val + givens_c * givens_c * diag_trail;
            offdiag[chase] = (givens_c * givens_c - givens_s * givens_s) * off_val + givens_c * givens_s * (diag_trail - diag_lead);
            if (chase < hi - 1)
            {
                chase_z = givens_s * offdiag[chase + 1];
                offdiag[chase + 1] = givens_c * offdiag[chase + 1];
            }

            chase_x = offdiag[chase];
            if (rot != 0x0) { for (row = 0; row < rot_rows; row++)
                {
                    held_a = rot[row * dim + chase];
                    held_b = rot[row * dim + chase + 1];
                    rot[row * dim + chase] = givens_c * held_a + givens_s * held_b;
                    rot[row * dim + chase + 1] = (0.0 - givens_s) * held_a + givens_c * held_b;
                }
            }
        }
    }
    for (idx = 0; idx + 1 < dim; idx++)
    {
        min_idx = idx;
        min_val = diag[idx];
        for (scan = idx + 1; scan < dim; scan++) { if (diag[scan] < min_val)
            {
                min_val = diag[scan];
                min_idx = scan;
            }
        }
        if (min_idx != idx)
        {
            diag[min_idx] = diag[idx];
            diag[idx] = min_val;
            if (rot != 0x0) { for (row = 0; row < rot_rows; row++)
                {
                    held_a = rot[row * dim + idx];
                    rot[row * dim + idx] = rot[row * dim + min_idx];
                    rot[row * dim + min_idx] = held_a;
                }
            }
        }
    }
    return LDG_ERR_AOK;
}

static void eigen_forward_col(const ldg_math_complex_t *factor, uint64_t dim, ldg_math_complex_t *vec, uint64_t stride)
{
    const ldg_math_complex_t *entry = 0x0;
    double diag_val = 0.0;
    uint64_t row = 0;
    uint64_t col = 0;

    for (row = 0; row < dim; row++)
    {
        for (col = 0; col < row; col++)
        {
            entry = &factor[row * dim + col];
            vec[row * stride].re -= entry->re * vec[col * stride].re - entry->im * vec[col * stride].im;
            vec[row * stride].im -= entry->re * vec[col * stride].im + entry->im * vec[col * stride].re;
        }
        diag_val = factor[row * dim + row].re;
        vec[row * stride].re /= diag_val;
        vec[row * stride].im /= diag_val;
    }
}

static void eigen_backward_col(const ldg_math_complex_t *factor, uint64_t dim, ldg_math_complex_t *vec, uint64_t stride)
{
    const ldg_math_complex_t *entry = 0x0;
    double diag_val = 0.0;
    uint64_t row = 0;
    uint64_t col = 0;
    uint64_t back = 0;

    for (back = 0; back < dim; back++)
    {
        row = dim - 1 - back;
        for (col = row + 1; col < dim; col++)
        {
            entry = &factor[col * dim + row];
            vec[row * stride].re -= entry->re * vec[col * stride].re + entry->im * vec[col * stride].im;
            vec[row * stride].im -= entry->re * vec[col * stride].im - entry->im * vec[col * stride].re;
        }
        diag_val = factor[row * dim + row].re;
        vec[row * stride].re /= diag_val;
        vec[row * stride].im /= diag_val;
    }
}

static void eigen_hermitian_fill(ldg_math_complex_t *work, uint64_t dim)
{
    uint64_t row = 0;
    uint64_t col = 0;

    for (row = 0; row < dim; row++)
    {
        work[row * dim + row].im = 0.0;
        for (col = row + 1; col < dim; col++)
        {
            work[row * dim + col].re = work[col * dim + row].re;
            work[row * dim + col].im = 0.0 - work[col * dim + row].im;
        }
    }
}

LDG_NOINLINE static void eigen_tridiag_reduce(ldg_math_eigen_ctx_t *ctx)
{
    ldg_math_complex_t *work = ctx->work;
    ldg_math_complex_t *house_y = ctx->house;
    ldg_math_complex_t *scratch_w = ctx->scratch;
    ldg_math_complex_t beta = LDG_STRUCT_ZERO_INIT;
    ldg_math_complex_t unit = LDG_STRUCT_ZERO_INIT;
    ldg_math_complex_t hd = LDG_STRUCT_ZERO_INIT;
    double xnorm_sq = 0.0;
    double hd_abs = 0.0;
    double norm_val = 0.0;
    double tau = 0.0;
    double beta_scale = 0.0;
    uint64_t dim = ctx->dim;
    uint64_t step = 0;
    uint64_t row = 0;
    uint64_t col = 0;

    for (step = 0; step + 2 < dim; step++)
    {
        xnorm_sq = 0.0;
        for (row = step + 1; row < dim; row++) { xnorm_sq += work[row * dim + step].re * work[row * dim + step].re + work[row * dim + step].im * work[row * dim + step].im; }
        hd = work[(step + 1) * dim + step];
        hd_abs = ldg_math_sqrt_get(hd.re * hd.re + hd.im * hd.im);
        if (!(xnorm_sq > 0.0))
        {
            ctx->phase[step].re = 0.0;
            ctx->phase[step].im = 0.0;
            for (row = step + 1; row < dim; row++)
            {
                work[row * dim + step].re = 0.0;
                work[row * dim + step].im = 0.0;
            }
            continue;
        }

        norm_val = ldg_math_sqrt_get(xnorm_sq);
        if (hd_abs > 0.0)
        {
            unit.re = hd.re / hd_abs;
            unit.im = hd.im / hd_abs;
        }
        else
        {
            unit.re = 1.0;
            unit.im = 0.0;
        }

        beta.re = (0.0 - unit.re) * norm_val;
        beta.im = (0.0 - unit.im) * norm_val;
        ctx->phase[step] = beta;
        work[(step + 1) * dim + step].re = hd.re - beta.re;
        work[(step + 1) * dim + step].im = hd.im - beta.im;
        tau = xnorm_sq + hd_abs * norm_val;
        for (row = step + 1; row < dim; row++)
        {
            house_y[row].re = 0.0;
            house_y[row].im = 0.0;
            for (col = step + 1; col < dim; col++)
            {
                house_y[row].re += work[row * dim + col].re * work[col * dim + step].re - work[row * dim + col].im * work[col * dim + step].im;
                house_y[row].im += work[row * dim + col].re * work[col * dim + step].im + work[row * dim + col].im * work[col * dim + step].re;
            }
        }
        beta_scale = 0.0;
        for (row = step + 1; row < dim; row++) { beta_scale += work[row * dim + step].re * house_y[row].re + work[row * dim + step].im * house_y[row].im; }
        beta_scale /= 2.0 * tau;
        for (row = step + 1; row < dim; row++)
        {
            scratch_w[row].re = (house_y[row].re - beta_scale * work[row * dim + step].re) / tau;
            scratch_w[row].im = (house_y[row].im - beta_scale * work[row * dim + step].im) / tau;
        }
        for (row = step + 1; row < dim; row++) { for (col = step + 1; col < dim; col++)
            {
                work[row * dim + col].re -= work[row * dim + step].re * scratch_w[col].re + work[row * dim + step].im * scratch_w[col].im;
                work[row * dim + col].im -= work[row * dim + step].im * scratch_w[col].re - work[row * dim + step].re * scratch_w[col].im;
                work[row * dim + col].re -= scratch_w[row].re * work[col * dim + step].re + scratch_w[row].im * work[col * dim + step].im;
                work[row * dim + col].im -= scratch_w[row].im * work[col * dim + step].re - scratch_w[row].re * work[col * dim + step].im;
            }
        }
    }
}

LDG_NOINLINE static void eigen_basis_form(ldg_math_eigen_ctx_t *ctx, ldg_math_complex_t *out_vec)
{
    ldg_math_complex_t *work = ctx->work;
    ldg_math_complex_t proj = LDG_STRUCT_ZERO_INIT;
    ldg_math_complex_t held = LDG_STRUCT_ZERO_INIT;
    double tau = 0.0;
    uint64_t dim = ctx->dim;
    uint64_t back = 0;
    uint64_t step = 0;
    uint64_t row = 0;
    uint64_t col = 0;
    uint64_t mid = 0;

    for (row = 0; row < dim; row++) { for (col = 0; col < dim; col++)
        {
            out_vec[row * dim + col].re = (row == col) ? 1.0 : 0.0;
            out_vec[row * dim + col].im = 0.0;
        }
    }
    if (dim >= 3)
    {
        for (back = 0; back <= dim - 3; back++)
        {
            step = dim - 3 - back;
            tau = 0.0;
            for (row = step + 1; row < dim; row++) { tau += work[row * dim + step].re * work[row * dim + step].re + work[row * dim + step].im * work[row * dim + step].im; }
            tau /= 2.0;
            if (!(tau > 0.0)) { continue; }

            for (col = 0; col < dim; col++)
            {
                proj.re = 0.0;
                proj.im = 0.0;
                for (row = step + 1; row < dim; row++)
                {
                    proj.re += work[row * dim + step].re * out_vec[row * dim + col].re + work[row * dim + step].im * out_vec[row * dim + col].im;
                    proj.im += work[row * dim + step].re * out_vec[row * dim + col].im - work[row * dim + step].im * out_vec[row * dim + col].re;
                }
                proj.re /= tau;
                proj.im /= tau;
                for (row = step + 1; row < dim; row++)
                {
                    out_vec[row * dim + col].re -= work[row * dim + step].re * proj.re - work[row * dim + step].im * proj.im;
                    out_vec[row * dim + col].im -= work[row * dim + step].re * proj.im + work[row * dim + step].im * proj.re;
                }
            }
        }
    }

    for (col = 0; col < dim; col++) { for (row = 0; row < dim; row++)
        {
            held = out_vec[row * dim + col];
            out_vec[row * dim + col].re = held.re * ctx->phase[col].re - held.im * ctx->phase[col].im;
            out_vec[row * dim + col].im = held.re * ctx->phase[col].im + held.im * ctx->phase[col].re;
        }
    }
    for (row = 0; row < dim; row++)
    {
        for (col = 0; col < dim; col++)
        {
            ctx->scratch[col].re = 0.0;
            ctx->scratch[col].im = 0.0;
            for (mid = 0; mid < dim; mid++)
            {
                ctx->scratch[col].re += out_vec[row * dim + mid].re * ctx->rot[mid * dim + col];
                ctx->scratch[col].im += out_vec[row * dim + mid].im * ctx->rot[mid * dim + col];
            }
        }
        for (col = 0; col < dim; col++) { out_vec[row * dim + col] = ctx->scratch[col]; }
    }
}

LDG_NOINLINE static uint32_t eigen_hermitian_run(ldg_math_eigen_ctx_t *ctx, double *out_val, ldg_math_complex_t *out_vec)
{
    ldg_math_complex_t sub = LDG_STRUCT_ZERO_INIT;
    ldg_math_complex_t d_cur = LDG_STRUCT_ZERO_INIT;
    ldg_math_complex_t d_next = LDG_STRUCT_ZERO_INIT;
    double sub_abs = 0.0;
    uint64_t dim = ctx->dim;
    uint64_t step = 0;
    uint64_t row = 0;
    uint64_t col = 0;
    uint32_t err = 0;

    eigen_hermitian_fill(ctx->work, dim);
    eigen_tridiag_reduce(ctx);
    if (dim >= 2) { ctx->phase[dim - 2] = ctx->work[(dim - 1) * dim + (dim - 2)]; }

    for (step = 0; step < dim; step++) { ctx->diag[step] = ctx->work[step * dim + step].re; }
    d_cur.re = 1.0;
    d_cur.im = 0.0;
    for (step = 0; step + 1 < dim; step++)
    {
        sub = ctx->phase[step];
        sub_abs = ldg_math_sqrt_get(sub.re * sub.re + sub.im * sub.im);
        ctx->offdiag[step] = sub_abs;
        ctx->phase[step] = d_cur;
        if (sub_abs > 0.0)
        {
            d_next.re = (d_cur.re * sub.re - d_cur.im * sub.im) / sub_abs;
            d_next.im = (d_cur.re * sub.im + d_cur.im * sub.re) / sub_abs;
            d_cur = d_next;
        }
    }
    ctx->phase[dim - 1] = d_cur;
    if (out_vec != 0x0) { for (row = 0; row < dim; row++) { for (col = 0; col < dim; col++) { ctx->rot[row * dim + col] = (row == col) ? 1.0 : 0.0; } } }

    err = eigen_tridiag_qr_run(ctx->diag, ctx->offdiag, dim, (out_vec != 0x0) ? ctx->rot : 0x0, dim);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (step = 0; step < dim; step++) { out_val[step] = ctx->diag[step]; }
    if (out_vec != 0x0) { eigen_basis_form(ctx, out_vec); }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_eigen_ctx_create(uint64_t dim, ldg_math_eigen_ctx_t **out)
{
    ldg_math_eigen_ctx_t *ctx = 0x0;
    void *blk = 0x0;
    uint64_t square = 0;
    uint32_t ret = 0;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    if (LDG_UNLIKELY(dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(dim > UINT64_MAX / dim)) { return LDG_ERR_OVERFLOW; }

    square = dim * dim;
    if (LDG_UNLIKELY(square > UINT64_MAX / (uint64_t)sizeof(ldg_math_complex_t))) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_math_eigen_ctx_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    ctx = (ldg_math_eigen_ctx_t *)blk;
    if (LDG_UNLIKELY(ldg_mem_zero(ctx, sizeof(ldg_math_eigen_ctx_t)) != LDG_ERR_AOK))
    {
        ret = ldg_mem_dealloc(ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->dim = dim;
    ret = ldg_mem_alloc(square * (uint64_t)sizeof(ldg_math_complex_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->work = (ldg_math_complex_t *)blk;
    ret = ldg_mem_alloc(dim * (uint64_t)sizeof(ldg_math_complex_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->house = (ldg_math_complex_t *)blk;
    ret = ldg_mem_alloc(dim * (uint64_t)sizeof(ldg_math_complex_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->scratch = (ldg_math_complex_t *)blk;
    ret = ldg_mem_alloc(dim * (uint64_t)sizeof(ldg_math_complex_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->phase = (ldg_math_complex_t *)blk;
    ret = ldg_mem_alloc(dim * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->diag = (double *)blk;
    ret = ldg_mem_alloc(dim * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->offdiag = (double *)blk;
    ret = ldg_mem_alloc(square * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->rot = (double *)blk;
    *out = ctx;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_eigen_ctx_destroy(ldg_math_eigen_ctx_t **ctx)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !*ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if ((*ctx)->work != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->work);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->house != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->house);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->scratch != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->scratch);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->phase != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->phase);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->diag != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->diag);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->offdiag != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->offdiag);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->rot != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->rot);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    err = ldg_mem_dealloc(*ctx);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    *ctx = 0x0;
    return ret;
}

LDG_EXPORT uint32_t ldg_math_eigen_hermitian_compute(ldg_math_eigen_ctx_t *ctx, const ldg_math_complex_t *mat, double *out_val, ldg_math_complex_t *out_vec)
{
    if (LDG_UNLIKELY(!ctx || !mat || !out_val)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_cpy(ctx->work, mat, ctx->dim * ctx->dim * sizeof(ldg_math_complex_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    return eigen_hermitian_run(ctx, out_val, out_vec);
}

LDG_EXPORT uint32_t ldg_math_eigen_generalized_compute(ldg_math_eigen_ctx_t *ctx, ldg_math_cholesky_ctx_t *chol, const ldg_math_complex_t *mat_a, const ldg_math_complex_t *mat_b, double *out_val, ldg_math_complex_t *out_vec)
{
    ldg_math_complex_t held = LDG_STRUCT_ZERO_INIT;
    uint64_t row = 0;
    uint64_t col = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !chol || !mat_a || !mat_b || !out_val)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->dim != chol->dim)) { return LDG_ERR_MATH_DIM_MISMATCH; }

    err = ldg_math_cholesky_factor_compute(chol, mat_b);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(ldg_mem_cpy(ctx->work, mat_a, ctx->dim * ctx->dim * sizeof(ldg_math_complex_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    eigen_hermitian_fill(ctx->work, ctx->dim);
    for (col = 0; col < ctx->dim; col++) { eigen_forward_col(chol->factor, ctx->dim, &ctx->work[col], ctx->dim); }
    for (row = 0; row < ctx->dim; row++)
    {
        ctx->work[row * ctx->dim + row].im = 0.0 - ctx->work[row * ctx->dim + row].im;
        for (col = row + 1; col < ctx->dim; col++)
        {
            held = ctx->work[row * ctx->dim + col];
            ctx->work[row * ctx->dim + col].re = ctx->work[col * ctx->dim + row].re;
            ctx->work[row * ctx->dim + col].im = 0.0 - ctx->work[col * ctx->dim + row].im;
            ctx->work[col * ctx->dim + row].re = held.re;
            ctx->work[col * ctx->dim + row].im = 0.0 - held.im;
        }
    }
    for (col = 0; col < ctx->dim; col++) { eigen_forward_col(chol->factor, ctx->dim, &ctx->work[col], ctx->dim); }
    err = eigen_hermitian_run(ctx, out_val, out_vec);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (out_vec != 0x0) { for (col = 0; col < ctx->dim; col++) { eigen_backward_col(chol->factor, ctx->dim, &out_vec[col], ctx->dim); } }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_eigen_lanczos_ctx_create(uint64_t dim, uint64_t step_cap, ldg_math_eigen_lanczos_ctx_t **out)
{
    ldg_math_eigen_lanczos_ctx_t *ctx = 0x0;
    void *blk = 0x0;
    uint64_t basis_cunt = 0;
    uint64_t step_sq = 0;
    uint32_t ret = 0;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    if (LDG_UNLIKELY(dim == 0 || step_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(step_cap > UINT64_MAX / step_cap)) { return LDG_ERR_OVERFLOW; }

    step_sq = step_cap * step_cap;
    if (LDG_UNLIKELY(dim > UINT64_MAX / (step_cap + 1))) { return LDG_ERR_OVERFLOW; }

    basis_cunt = dim * (step_cap + 1);
    if (LDG_UNLIKELY(basis_cunt > UINT64_MAX / (uint64_t)sizeof(double))) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_math_eigen_lanczos_ctx_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    ctx = (ldg_math_eigen_lanczos_ctx_t *)blk;
    if (LDG_UNLIKELY(ldg_mem_zero(ctx, sizeof(ldg_math_eigen_lanczos_ctx_t)) != LDG_ERR_AOK))
    {
        ret = ldg_mem_dealloc(ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->dim = dim;
    ctx->step_cap = step_cap;
    ret = ldg_mem_alloc(basis_cunt * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_lanczos_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->basis = (double *)blk;
    ret = ldg_mem_alloc(step_cap * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_lanczos_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->diag = (double *)blk;
    ret = ldg_mem_alloc(step_cap * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_lanczos_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->offdiag = (double *)blk;
    ret = ldg_mem_alloc((step_cap + 1) * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_lanczos_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->omega = (double *)blk;
    ret = ldg_mem_alloc((step_cap + 1) * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_lanczos_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->omega_prev = (double *)blk;
    if (LDG_UNLIKELY(step_sq > UINT64_MAX / (uint64_t)sizeof(double)))
    {
        ret = ldg_math_eigen_lanczos_ctx_destroy(&ctx);
        return LDG_ERR_OVERFLOW;
    }

    ret = ldg_mem_alloc(step_sq * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_lanczos_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->ritz_vec = (double *)blk;
    ret = ldg_mem_alloc(step_cap * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_lanczos_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->ritz_val = (double *)blk;
    *out = ctx;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_eigen_lanczos_ctx_destroy(ldg_math_eigen_lanczos_ctx_t **ctx)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !*ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if ((*ctx)->basis != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->basis);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->diag != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->diag);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->offdiag != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->offdiag);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->omega != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->omega);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->omega_prev != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->omega_prev);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->ritz_vec != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->ritz_vec);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->ritz_val != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->ritz_val);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    err = ldg_mem_dealloc(*ctx);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    *ctx = 0x0;
    return ret;
}

LDG_EXPORT uint32_t ldg_math_eigen_lanczos_compute(ldg_math_eigen_lanczos_ctx_t *ctx, const ldg_math_sparse_t *mat, double tol, double *out_val, double *out_vec, uint64_t *out_cunt)
{
    double *q_cur = 0x0;
    double *q_next = 0x0;
    double *om_cur = 0x0;
    double *om_new = 0x0;
    double *om_swap = 0x0;
    double alpha = 0.0;
    double beta_prev = 0.0;
    double beta_raw = 0.0;
    double beta_val = 0.0;
    double anorm = 0.0;
    double gersh = 0.0;
    double proj = 0.0;
    double accum = 0.0;
    double res_val = 0.0;
    double beta_last = 0.0;
    uint64_t dim = 0;
    uint64_t cap = 0;
    uint64_t steps = 0;
    uint64_t step = 0;
    uint64_t pos = 0;
    uint64_t row = 0;
    uint64_t idx = 0;
    uint64_t batch_lo = 0;
    uint64_t batch_hi = 0;
    uint64_t cunt = 0;
    uint8_t force_reorth = LDG_TRUTH_FALSE;
    uint8_t need_reorth = LDG_TRUTH_FALSE;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !mat || !out_val || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    if (LDG_UNLIKELY(!(tol >= 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    dim = ctx->dim;
    if (LDG_UNLIKELY(mat->rows != dim || mat->cols != dim)) { return LDG_ERR_MATH_DIM_MISMATCH; }

    cap = (ctx->step_cap < dim) ? ctx->step_cap : dim;
    om_cur = ctx->omega;
    om_new = ctx->omega_prev;
    accum = 0.0;
    for (row = 0; row < dim; row++)
    {
        ctx->basis[row] = 1.0 + 0.5 * ldg_math_cos_get(2.3 * (double)row);
        accum += ctx->basis[row] * ctx->basis[row];
    }
    accum = ldg_math_sqrt_get(accum);
    for (row = 0; row < dim; row++) { ctx->basis[row] /= accum; }
    om_cur[0] = 1.0;
    for (step = 0; step < cap; step++)
    {
        q_cur = &ctx->basis[step * dim];
        q_next = &ctx->basis[(step + 1) * dim];
        err = ldg_math_sparse_mv(mat, q_cur, q_next);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        alpha = 0.0;
        for (row = 0; row < dim; row++) { alpha += q_cur[row] * q_next[row]; }
        ctx->diag[step] = alpha;
        beta_prev = (step > 0) ? ctx->offdiag[step - 1] : 0.0;
        for (row = 0; row < dim; row++)
        {
            q_next[row] -= alpha * q_cur[row];
            if (step > 0) { q_next[row] -= beta_prev * ctx->basis[(step - 1) * dim + row]; }
        }
        beta_raw = 0.0;
        for (row = 0; row < dim; row++) { beta_raw += q_next[row] * q_next[row]; }
        beta_raw = ldg_math_sqrt_get(beta_raw);
        gersh = ldg_math_abs_get(alpha) + beta_prev + beta_raw;
        if (gersh > anorm) { anorm = gersh; }

        steps = step + 1;
        if (beta_raw <= anorm * DBL_EPSILON * (double)dim)
        {
            ctx->offdiag[step] = 0.0;
            break;
        }

        for (pos = 0; pos < step; pos++)
        {
            accum = ctx->offdiag[pos] * om_cur[pos + 1] + (ctx->diag[pos] - alpha) * om_cur[pos];
            if (pos > 0) { accum += ctx->offdiag[pos - 1] * om_cur[pos - 1]; }

            accum -= beta_prev * om_new[pos];
            om_new[pos] = (accum + DBL_EPSILON * anorm) / beta_raw;
        }
        om_new[step] = DBL_EPSILON;
        om_new[step + 1] = 1.0;
        need_reorth = force_reorth;
        if (need_reorth != LDG_TRUTH_TRUE) { for (pos = 0; pos < step; pos++) { if (ldg_math_abs_get(om_new[pos]) > ldg_math_sqrt_get(DBL_EPSILON))
                {
                    need_reorth = LDG_TRUTH_TRUE;
                    break;
                }
            } }

        if (need_reorth == LDG_TRUTH_TRUE && step > 0)
        {
            if (force_reorth != LDG_TRUTH_TRUE)
            {
                batch_lo = step;
                batch_hi = 0;
                for (pos = 0; pos < step; pos++) { if (ldg_math_abs_get(om_new[pos]) > ldg_math_sqrt_get(DBL_EPSILON))
                    {
                        if (pos < batch_lo) { batch_lo = pos; }

                        if (pos > batch_hi) { batch_hi = pos; }
                    }
                }
                while (batch_lo > 0 && ldg_math_abs_get(om_new[batch_lo - 1]) > ldg_math_pwr_get(DBL_EPSILON, 0.75)) { batch_lo--; }

                while (batch_hi + 1 < step && ldg_math_abs_get(om_new[batch_hi + 1]) > ldg_math_pwr_get(DBL_EPSILON, 0.75)) { batch_hi++; }

                force_reorth = LDG_TRUTH_TRUE;
            }
            else
            {
                if (batch_lo > 0) { batch_lo--; }

                if (batch_hi + 1 < step) { batch_hi++; }

                force_reorth = LDG_TRUTH_FALSE;
            }

            for (pos = batch_lo; pos <= batch_hi; pos++)
            {
                proj = 0.0;
                for (row = 0; row < dim; row++) { proj += ctx->basis[pos * dim + row] * q_next[row]; }
                for (row = 0; row < dim; row++) { q_next[row] -= proj * ctx->basis[pos * dim + row]; }
                om_new[pos] = DBL_EPSILON;
            }
            beta_raw = 0.0;
            for (row = 0; row < dim; row++) { beta_raw += q_next[row] * q_next[row]; }
            beta_raw = ldg_math_sqrt_get(beta_raw);
        }

        beta_val = beta_raw;
        if (beta_val <= anorm * DBL_EPSILON * (double)dim)
        {
            ctx->offdiag[step] = 0.0;
            break;
        }

        ctx->offdiag[step] = beta_val;
        for (row = 0; row < dim; row++) { q_next[row] /= beta_val; }
        om_swap = om_cur;
        om_cur = om_new;
        om_new = om_swap;
    }
    beta_last = ctx->offdiag[steps - 1];
    for (pos = 0; pos < steps; pos++) { ctx->ritz_val[pos] = ctx->diag[pos]; }
    for (row = 0; row < steps; row++) { for (pos = 0; pos < steps; pos++) { ctx->ritz_vec[row * steps + pos] = (row == pos) ? 1.0 : 0.0; } }
    err = eigen_tridiag_qr_run(ctx->ritz_val, ctx->offdiag, steps, ctx->ritz_vec, steps);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    cunt = 0;
    for (idx = 0; idx < steps; idx++)
    {
        res_val = beta_last * ldg_math_abs_get(ctx->ritz_vec[(steps - 1) * steps + idx]);
        if (res_val <= tol * ldg_math_abs_get(ctx->ritz_val[idx]))
        {
            out_val[cunt] = ctx->ritz_val[idx];
            if (out_vec != 0x0) { for (row = 0; row < dim; row++)
                {
                    accum = 0.0;
                    for (pos = 0; pos < steps; pos++) { accum += ctx->basis[pos * dim + row] * ctx->ritz_vec[pos * steps + idx]; }
                    out_vec[cunt * dim + row] = accum;
                }
            }

            cunt++;
        }
    }
    *out_cunt = cunt;
    return LDG_ERR_AOK;
}

LDG_NOINLINE static void eigen_schur_balance_run(double *work, double *scale, uint64_t dim)
{
    double row_max = 0.0;
    double col_max = 0.0;
    double factor = 0.0;
    uint64_t sweep = 0;
    uint64_t row = 0;
    uint64_t col = 0;
    uint8_t settled = LDG_TRUTH_FALSE;

    for (row = 0; row < dim; row++) { scale[row] = 1.0; }
    for (sweep = 0; sweep < 20 && settled != LDG_TRUTH_TRUE; sweep++)
    {
        settled = LDG_TRUTH_TRUE;
        for (row = 0; row < dim; row++)
        {
            row_max = 0.0;
            col_max = 0.0;
            for (col = 0; col < dim; col++)
            {
                if (col == row) { continue; }

                if (ldg_math_abs_get(work[row * dim + col]) > row_max) { row_max = ldg_math_abs_get(work[row * dim + col]); }

                if (ldg_math_abs_get(work[col * dim + row]) > col_max) { col_max = ldg_math_abs_get(work[col * dim + row]); }
            }
            if (!(row_max > 0.0) || !(col_max > 0.0)) { continue; }

            factor = ldg_math_sqrt_get(row_max / col_max);
            if (factor < 2.0 && factor > 0.5) { continue; }

            settled = LDG_TRUTH_FALSE;
            scale[row] *= factor;
            for (col = 0; col < dim; col++)
            {
                work[col * dim + row] *= factor;
                work[row * dim + col] /= factor;
            }
        }
    }
}

LDG_NOINLINE static void eigen_schur_hessenberg_run(double *work, double *basis, double *house, uint64_t dim)
{
    double norm_val = 0.0;
    double uu_val = 0.0;
    double proj = 0.0;
    uint64_t step = 0;
    uint64_t row = 0;
    uint64_t col = 0;

    for (row = 0; row < dim; row++) { for (col = 0; col < dim; col++) { basis[row * dim + col] = (row == col) ? 1.0 : 0.0; } }
    for (step = 0; step + 2 < dim; step++)
    {
        norm_val = 0.0;
        for (row = step + 1; row < dim; row++) { norm_val += work[row * dim + step] * work[row * dim + step]; }
        norm_val = ldg_math_sqrt_get(norm_val);
        if (!(norm_val > 0.0)) { continue; }

        for (row = step + 1; row < dim; row++) { house[row] = work[row * dim + step]; }
        house[step + 1] += (house[step + 1] >= 0.0) ? norm_val : (0.0 - norm_val);
        uu_val = 0.0;
        for (row = step + 1; row < dim; row++) { uu_val += house[row] * house[row]; }
        if (!(uu_val > 0.0)) { continue; }

        for (col = 0; col < dim; col++)
        {
            proj = 0.0;
            for (row = step + 1; row < dim; row++) { proj += house[row] * work[row * dim + col]; }
            proj = 2.0 * proj / uu_val;
            for (row = step + 1; row < dim; row++) { work[row * dim + col] -= proj * house[row]; }
        }
        for (row = 0; row < dim; row++)
        {
            proj = 0.0;
            for (col = step + 1; col < dim; col++) { proj += house[col] * work[row * dim + col]; }
            proj = 2.0 * proj / uu_val;
            for (col = step + 1; col < dim; col++) { work[row * dim + col] -= proj * house[col]; }
        }
        for (row = 0; row < dim; row++)
        {
            proj = 0.0;
            for (col = step + 1; col < dim; col++) { proj += house[col] * basis[row * dim + col]; }
            proj = 2.0 * proj / uu_val;
            for (col = step + 1; col < dim; col++) { basis[row * dim + col] -= proj * house[col]; }
        }
        for (row = step + 2; row < dim; row++) { work[row * dim + step] = 0.0; }
    }
}

LDG_NOINLINE static void eigen_schur_reflect_apply(double *work, double *basis, uint64_t dim, uint64_t lo, uint64_t hi, uint64_t at, double ref_a, double ref_b, double ref_c, uint8_t wide)
{
    double vec_u[3];
    double norm_val = 0.0;
    double uu_val = 0.0;
    double proj = 0.0;
    uint64_t span = (wide == LDG_TRUTH_TRUE) ? 3 : 2;
    uint64_t row = 0;
    uint64_t col = 0;
    uint64_t col_lo = 0;
    uint64_t row_hi = 0;
    uint64_t idx = 0;

    vec_u[0] = ref_a;
    vec_u[1] = ref_b;
    vec_u[2] = (wide == LDG_TRUTH_TRUE) ? ref_c : 0.0;
    norm_val = ldg_math_sqrt_get(vec_u[0] * vec_u[0] + vec_u[1] * vec_u[1] + vec_u[2] * vec_u[2]);
    if (!(norm_val > 0.0)) { return; }

    vec_u[0] += (vec_u[0] >= 0.0) ? norm_val : (0.0 - norm_val);
    uu_val = vec_u[0] * vec_u[0] + vec_u[1] * vec_u[1] + vec_u[2] * vec_u[2];
    if (!(uu_val > 0.0)) { return; }

    col_lo = (at > lo) ? at - 1 : lo;
    for (col = col_lo; col < dim; col++)
    {
        proj = 0.0;
        for (idx = 0; idx < span; idx++) { proj += vec_u[idx] * work[(at + idx) * dim + col]; }
        proj = 2.0 * proj / uu_val;
        for (idx = 0; idx < span; idx++) { work[(at + idx) * dim + col] -= proj * vec_u[idx]; }
    }
    row_hi = (at + span + 1 < hi + 1) ? at + span + 1 : hi + 1;
    for (row = 0; row < row_hi; row++)
    {
        proj = 0.0;
        for (idx = 0; idx < span; idx++) { proj += vec_u[idx] * work[row * dim + at + idx]; }
        proj = 2.0 * proj / uu_val;
        for (idx = 0; idx < span; idx++) { work[row * dim + at + idx] -= proj * vec_u[idx]; }
    }
    for (row = 0; row < dim; row++)
    {
        proj = 0.0;
        for (idx = 0; idx < span; idx++) { proj += vec_u[idx] * basis[row * dim + at + idx]; }
        proj = 2.0 * proj / uu_val;
        for (idx = 0; idx < span; idx++) { basis[row * dim + at + idx] -= proj * vec_u[idx]; }
    }
}

LDG_NOINLINE static uint32_t eigen_schur_francis_run(double *work, double *basis, ldg_math_complex_t *val, uint64_t dim)
{
    double trace_val = 0.0;
    double det_val = 0.0;
    double bulge_a = 0.0;
    double bulge_b = 0.0;
    double bulge_c = 0.0;
    double half_tr = 0.0;
    double disc = 0.0;
    double exc_sum = 0.0;
    double hnorm = 0.0;
    double tst = 0.0;
    uint64_t hi = 0;
    uint64_t lo = 0;
    uint64_t start = 0;
    uint64_t chase = 0;
    uint64_t iter_cunt = 0;
    uint64_t stall_cunt = 0;
    uint64_t prev_hi = 0;
    uint8_t wide = LDG_TRUTH_FALSE;

    hi = dim - 1;
    prev_hi = hi;
    hnorm = 0.0;
    for (chase = 0; chase < dim; chase++) { for (lo = (chase > 0) ? chase - 1 : 0; lo < dim; lo++) { if (ldg_math_abs_get(work[chase * dim + lo]) > hnorm) { hnorm = ldg_math_abs_get(work[chase * dim + lo]); } } }
    if (!(hnorm > 0.0)) { hnorm = 1.0; }

    while (1)
    {
        while (hi > 0)
        {
            tst = ldg_math_abs_get(work[(hi - 1) * dim + hi - 1]) + ldg_math_abs_get(work[hi * dim + hi]);
            if (!(tst > 0.0)) { tst = hnorm; }

            if (ldg_math_abs_get(work[hi * dim + hi - 1]) <= DBL_EPSILON * tst)
            {
                work[hi * dim + hi - 1] = 0.0;
                val[hi].re = work[hi * dim + hi];
                val[hi].im = 0.0;
                hi--;
                continue;
            }

            tst = (hi >= 2) ? ldg_math_abs_get(work[(hi - 2) * dim + hi - 2]) + ldg_math_abs_get(work[(hi - 1) * dim + hi - 1]) : hnorm;
            if (!(tst > 0.0)) { tst = hnorm; }

            if (hi >= 1 && (hi == 1 || ldg_math_abs_get(work[(hi - 1) * dim + hi - 2]) <= DBL_EPSILON * tst))
            {
                if (hi >= 2) { work[(hi - 1) * dim + hi - 2] = 0.0; }

                half_tr = (work[(hi - 1) * dim + hi - 1] + work[hi * dim + hi]) / 2.0;
                det_val = work[(hi - 1) * dim + hi - 1] * work[hi * dim + hi] - work[(hi - 1) * dim + hi] * work[hi * dim + hi - 1];
                disc = half_tr * half_tr - det_val;
                if (disc >= 0.0)
                {
                    val[hi - 1].re = half_tr + ((half_tr >= 0.0) ? ldg_math_sqrt_get(disc) : (0.0 - ldg_math_sqrt_get(disc)));
                    val[hi - 1].im = 0.0;
                    val[hi].re = (ldg_math_abs_get(val[hi - 1].re) > 0.0) ? det_val / val[hi - 1].re : half_tr;
                    val[hi].im = 0.0;
                }
                else
                {
                    val[hi - 1].re = half_tr;
                    val[hi - 1].im = ldg_math_sqrt_get(0.0 - disc);
                    val[hi].re = half_tr;
                    val[hi].im = 0.0 - ldg_math_sqrt_get(0.0 - disc);
                }

                if (hi == 1) { return LDG_ERR_AOK; }

                hi -= 2;
                continue;
            }

            break;
        }
        if (hi == 0)
        {
            val[0].re = work[0];
            val[0].im = 0.0;
            return LDG_ERR_AOK;
        }

        iter_cunt++;
        if (LDG_UNLIKELY(iter_cunt > 200 * dim)) { return LDG_ERR_MATH_NOT_CONVERGED; }

        if (hi == prev_hi) { stall_cunt++; }
        else
        {
            prev_hi = hi;
            stall_cunt = 0;
        }

        if (LDG_UNLIKELY(stall_cunt > 60)) { return LDG_ERR_MATH_NOT_CONVERGED; }

        lo = hi;
        while (lo > 0)
        {
            tst = ldg_math_abs_get(work[(lo - 1) * dim + lo - 1]) + ldg_math_abs_get(work[lo * dim + lo]);
            if (!(tst > 0.0)) { tst = hnorm; }

            if (ldg_math_abs_get(work[lo * dim + lo - 1]) <= DBL_EPSILON * tst) { break; }

            lo--;
        }
        if (lo > 0) { work[lo * dim + lo - 1] = 0.0; }

        if (stall_cunt > 0 && stall_cunt % 20 == 10)
        {
            exc_sum = ldg_math_abs_get(work[hi * dim + hi - 1]) + ((hi >= 2) ? ldg_math_abs_get(work[(hi - 1) * dim + hi - 2]) : 0.0);
            trace_val = 1.5 * exc_sum;
            det_val = (0.0 - 0.4375) * exc_sum * exc_sum;
        }
        else if (stall_cunt > 0 && stall_cunt % 20 == 0)
        {
            exc_sum = ldg_math_abs_get(work[hi * dim + hi - 1]) + ((hi >= 2) ? ldg_math_abs_get(work[(hi - 1) * dim + hi - 2]) : 0.0);
            trace_val = 2.0 * work[hi * dim + hi] + 1.5 * exc_sum;
            det_val = work[hi * dim + hi] * work[hi * dim + hi] + 1.5 * work[hi * dim + hi] * exc_sum - 0.4375 * exc_sum * exc_sum;
        }
        else
        {
            trace_val = work[(hi - 1) * dim + hi - 1] + work[hi * dim + hi];
            det_val = work[(hi - 1) * dim + hi - 1] * work[hi * dim + hi] - work[(hi - 1) * dim + hi] * work[hi * dim + hi - 1];
        }

        start = hi - 2;
        while (start > lo)
        {
            bulge_a = work[start * dim + start] * work[start * dim + start] + work[start * dim + start + 1] * work[(start + 1) * dim + start] - trace_val * work[start * dim + start] + det_val;
            bulge_b = work[(start + 1) * dim + start] * (work[start * dim + start] + work[(start + 1) * dim + start + 1] - trace_val);
            bulge_c = work[(start + 1) * dim + start] * work[(start + 2) * dim + start + 1];
            if (ldg_math_abs_get(work[start * dim + start - 1]) * (ldg_math_abs_get(bulge_b) + ldg_math_abs_get(bulge_c)) <= DBL_EPSILON * ldg_math_abs_get(bulge_a) * (ldg_math_abs_get(work[(start - 1) * dim + start - 1]) + ldg_math_abs_get(work[start * dim + start]) + ldg_math_abs_get(work[(start + 1) * dim + start + 1]))) { break; }

            start--;
        }
        bulge_a = work[start * dim + start] * work[start * dim + start] + work[start * dim + start + 1] * work[(start + 1) * dim + start] - trace_val * work[start * dim + start] + det_val;
        bulge_b = work[(start + 1) * dim + start] * (work[start * dim + start] + work[(start + 1) * dim + start + 1] - trace_val);
        bulge_c = (start + 2 <= hi) ? work[(start + 1) * dim + start] * work[(start + 2) * dim + start + 1] : 0.0;
        for (chase = start; chase + 1 <= hi; chase++)
        {
            wide = (chase + 2 <= hi) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
            eigen_schur_reflect_apply(work, basis, dim, lo, hi, chase, bulge_a, bulge_b, bulge_c, wide);
            if (chase > lo)
            {
                work[(chase + 1) * dim + chase - 1] = 0.0;
                if (wide == LDG_TRUTH_TRUE) { work[(chase + 2) * dim + chase - 1] = 0.0; }
            }

            if (chase + 1 <= hi)
            {
                bulge_a = work[(chase + 1) * dim + chase];
                bulge_b = (chase + 2 <= hi) ? work[(chase + 2) * dim + chase] : 0.0;
                bulge_c = (chase + 3 <= hi) ? work[(chase + 3) * dim + chase] : 0.0;
            }
        }
    }
}

static void eigen_pencil_bmul(const ldg_math_complex_t *mat_b, uint64_t dim, const ldg_math_complex_t *vec, ldg_math_complex_t *out)
{
    const ldg_math_complex_t *entry = 0x0;
    uint64_t row = 0;
    uint64_t col = 0;

    for (row = 0; row < dim; row++)
    {
        out[row].re = mat_b[row * dim + row].re * vec[row].re;
        out[row].im = mat_b[row * dim + row].re * vec[row].im;
        for (col = 0; col < row; col++)
        {
            entry = &mat_b[row * dim + col];
            out[row].re += entry->re * vec[col].re - entry->im * vec[col].im;
            out[row].im += entry->re * vec[col].im + entry->im * vec[col].re;
        }
        for (col = row + 1; col < dim; col++)
        {
            entry = &mat_b[col * dim + row];
            out[row].re += entry->re * vec[col].re + entry->im * vec[col].im;
            out[row].im += entry->re * vec[col].im - entry->im * vec[col].re;
        }
    }
}

LDG_EXPORT uint32_t ldg_math_eigen_schur_ctx_create(uint64_t dim, ldg_math_eigen_schur_ctx_t **out)
{
    ldg_math_eigen_schur_ctx_t *ctx = 0x0;
    void *blk = 0x0;
    uint64_t square = 0;
    uint32_t ret = 0;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    if (LDG_UNLIKELY(dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(dim > UINT64_MAX / dim)) { return LDG_ERR_OVERFLOW; }

    square = dim * dim;
    if (LDG_UNLIKELY(square > UINT64_MAX / (uint64_t)sizeof(ldg_math_complex_t))) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_math_eigen_schur_ctx_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    ctx = (ldg_math_eigen_schur_ctx_t *)blk;
    if (LDG_UNLIKELY(ldg_mem_zero(ctx, sizeof(ldg_math_eigen_schur_ctx_t)) != LDG_ERR_AOK))
    {
        ret = ldg_mem_dealloc(ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->dim = dim;
    ret = ldg_mem_alloc(square * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_schur_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->work = (double *)blk;
    ret = ldg_mem_alloc(square * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_schur_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->basis = (double *)blk;
    ret = ldg_mem_alloc(dim * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_schur_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->house = (double *)blk;
    ret = ldg_mem_alloc(dim * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_schur_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->scale = (double *)blk;
    ret = ldg_mem_alloc(dim * (uint64_t)sizeof(ldg_math_complex_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_schur_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->val = (ldg_math_complex_t *)blk;
    ret = ldg_mem_alloc((square + 2 * dim) * (uint64_t)sizeof(ldg_math_complex_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_schur_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->stage = (ldg_math_complex_t *)blk;
    *out = ctx;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_eigen_schur_ctx_destroy(ldg_math_eigen_schur_ctx_t **ctx)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !*ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if ((*ctx)->work != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->work);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->basis != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->basis);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->house != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->house);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->scale != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->scale);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->val != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->val);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->stage != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->stage);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    err = ldg_mem_dealloc(*ctx);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    *ctx = 0x0;
    return ret;
}

LDG_EXPORT uint32_t ldg_math_eigen_schur_compute(ldg_math_eigen_schur_ctx_t *ctx, const double *mat, ldg_math_complex_t *out_val)
{
    ldg_math_complex_t held = LDG_STRUCT_ZERO_INIT;
    uint64_t idx = 0;
    uint64_t scan = 0;
    uint64_t min_idx = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !mat || !out_val)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->ready = LDG_TRUTH_FALSE;
    if (LDG_UNLIKELY(ldg_mem_cpy(ctx->work, mat, ctx->dim * ctx->dim * sizeof(double)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    eigen_schur_balance_run(ctx->work, ctx->scale, ctx->dim);
    if (ctx->dim >= 3) { eigen_schur_hessenberg_run(ctx->work, ctx->basis, ctx->house, ctx->dim); }
    else
    {
        for (idx = 0; idx < ctx->dim * ctx->dim; idx++) { ctx->basis[idx] = 0.0; }
        for (idx = 0; idx < ctx->dim; idx++) { ctx->basis[idx * ctx->dim + idx] = 1.0; }
    }

    err = eigen_schur_francis_run(ctx->work, ctx->basis, ctx->val, ctx->dim);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (idx = 0; idx + 1 < ctx->dim; idx++)
    {
        min_idx = idx;
        for (scan = idx + 1; scan < ctx->dim; scan++) { if (ctx->val[scan].re < ctx->val[min_idx].re || (!(ctx->val[scan].re > ctx->val[min_idx].re) && ctx->val[scan].im < ctx->val[min_idx].im)) { min_idx = scan; } }
        if (min_idx != idx)
        {
            held = ctx->val[idx];
            ctx->val[idx] = ctx->val[min_idx];
            ctx->val[min_idx] = held;
        }
    }
    for (idx = 0; idx < ctx->dim; idx++) { out_val[idx] = ctx->val[idx]; }
    ctx->ready = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_eigen_schur_vec_compute(ldg_math_eigen_schur_ctx_t *ctx, ldg_math_lu_ctx_t *lu, uint64_t which, ldg_math_complex_t *out_vec)
{
    ldg_math_complex_t *shift_mat = 0x0;
    ldg_math_complex_t *rhs_vec = 0x0;
    ldg_math_complex_t *sol_vec = 0x0;
    ldg_math_complex_t lam = LDG_STRUCT_ZERO_INIT;
    double norm_val = 0.0;
    double nudge = 0.0;
    uint64_t dim = 0;
    uint64_t row = 0;
    uint64_t col = 0;
    uint64_t attempt = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !lu || !out_vec)) { return LDG_ERR_FUNC_ARG_NULL; }

    dim = ctx->dim;
    if (LDG_UNLIKELY(ctx->ready != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_STATE; }

    if (LDG_UNLIKELY(lu->dim != dim)) { return LDG_ERR_MATH_DIM_MISMATCH; }

    if (LDG_UNLIKELY(which >= dim)) { return LDG_ERR_FUNC_ARG_INVALID; }

    shift_mat = ctx->stage;
    rhs_vec = &ctx->stage[dim * dim];
    sol_vec = &ctx->stage[dim * dim + dim];
    lam = ctx->val[which];
    nudge = DBL_EPSILON * (1.0 + ldg_math_sqrt_get(lam.re * lam.re + lam.im * lam.im));
    for (attempt = 0; attempt < 3; attempt++)
    {
        for (row = 0; row < dim; row++)
        {
            for (col = 0; col < dim; col++)
            {
                shift_mat[row * dim + col].re = ctx->work[row * dim + col];
                shift_mat[row * dim + col].im = 0.0;
            }
            shift_mat[row * dim + row].re -= lam.re;
            shift_mat[row * dim + row].im = 0.0 - lam.im;
        }
        err = ldg_math_lu_factor_compute(lu, shift_mat);
        if (err == LDG_ERR_AOK) { break; }

        if (LDG_UNLIKELY(err != LDG_ERR_MATH_PIVOT)) { return err; }

        lam.re += nudge * 100.0;
        nudge *= 100.0;
    }
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (row = 0; row < dim; row++)
    {
        rhs_vec[row].re = 1.0 + 0.5 * ldg_math_cos_get(2.3 * (double)row);
        rhs_vec[row].im = 0.3 * ldg_math_sin_get(1.7 * (double)row);
    }
    for (attempt = 0; attempt < 3; attempt++)
    {
        err = ldg_math_lu_solve_compute(lu, rhs_vec, 1, sol_vec);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        norm_val = 0.0;
        for (row = 0; row < dim; row++) { norm_val += sol_vec[row].re * sol_vec[row].re + sol_vec[row].im * sol_vec[row].im; }
        norm_val = ldg_math_sqrt_get(norm_val);
        if (LDG_UNLIKELY(!(norm_val > 0.0))) { return LDG_ERR_MATH_STATE; }

        for (row = 0; row < dim; row++)
        {
            rhs_vec[row].re = sol_vec[row].re / norm_val;
            rhs_vec[row].im = sol_vec[row].im / norm_val;
        }
    }
    for (row = 0; row < dim; row++)
    {
        sol_vec[row].re = 0.0;
        sol_vec[row].im = 0.0;
        for (col = 0; col < dim; col++)
        {
            sol_vec[row].re += ctx->basis[row * dim + col] * rhs_vec[col].re;
            sol_vec[row].im += ctx->basis[row * dim + col] * rhs_vec[col].im;
        }
    }
    norm_val = 0.0;
    for (row = 0; row < dim; row++)
    {
        sol_vec[row].re *= ctx->scale[row];
        sol_vec[row].im *= ctx->scale[row];
        norm_val += sol_vec[row].re * sol_vec[row].re + sol_vec[row].im * sol_vec[row].im;
    }
    norm_val = ldg_math_sqrt_get(norm_val);
    if (LDG_UNLIKELY(!(norm_val > 0.0))) { return LDG_ERR_MATH_STATE; }

    for (row = 0; row < dim; row++)
    {
        out_vec[row].re = sol_vec[row].re / norm_val;
        out_vec[row].im = sol_vec[row].im / norm_val;
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_eigen_pencil_ctx_create(uint64_t dim, uint64_t step_cap, ldg_math_eigen_pencil_ctx_t **out)
{
    ldg_math_eigen_pencil_ctx_t *ctx = 0x0;
    void *blk = 0x0;
    uint64_t square = 0;
    uint64_t basis_cunt = 0;
    uint64_t step_sq = 0;
    uint32_t ret = 0;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;
    if (LDG_UNLIKELY(dim == 0 || step_cap < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(dim > UINT64_MAX / dim)) { return LDG_ERR_OVERFLOW; }

    square = dim * dim;
    if (LDG_UNLIKELY(step_cap > UINT64_MAX / (step_cap + 1))) { return LDG_ERR_OVERFLOW; }

    step_sq = (step_cap + 1) * (step_cap + 1);
    if (LDG_UNLIKELY(dim > UINT64_MAX / (step_cap + 2))) { return LDG_ERR_OVERFLOW; }

    basis_cunt = dim * (step_cap + 2);
    if (LDG_UNLIKELY(square > UINT64_MAX / (uint64_t)sizeof(ldg_math_complex_t) || basis_cunt > UINT64_MAX / (uint64_t)sizeof(ldg_math_complex_t))) { return LDG_ERR_OVERFLOW; }

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_math_eigen_pencil_ctx_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    ctx = (ldg_math_eigen_pencil_ctx_t *)blk;
    if (LDG_UNLIKELY(ldg_mem_zero(ctx, sizeof(ldg_math_eigen_pencil_ctx_t)) != LDG_ERR_AOK))
    {
        ret = ldg_mem_dealloc(ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->dim = dim;
    ctx->step_cap = step_cap;
    ret = ldg_mem_alloc(basis_cunt * (uint64_t)sizeof(ldg_math_complex_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_pencil_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->basis = (ldg_math_complex_t *)blk;
    ret = ldg_mem_alloc(square * (uint64_t)sizeof(ldg_math_complex_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_pencil_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->shifted = (ldg_math_complex_t *)blk;
    ret = ldg_mem_alloc(dim * (uint64_t)sizeof(ldg_math_complex_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_pencil_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->apply = (ldg_math_complex_t *)blk;
    ret = ldg_mem_alloc(dim * (uint64_t)sizeof(ldg_math_complex_t), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_pencil_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->bprod = (ldg_math_complex_t *)blk;
    ret = ldg_mem_alloc((step_cap + 1) * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_pencil_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->diag = (double *)blk;
    ret = ldg_mem_alloc((step_cap + 1) * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_pencil_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->offdiag = (double *)blk;
    ret = ldg_mem_alloc(step_sq * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_pencil_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->ritz_vec = (double *)blk;
    ret = ldg_mem_alloc((step_cap + 1) * (uint64_t)sizeof(double), &blk);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        ret = ldg_math_eigen_pencil_ctx_destroy(&ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->ritz_val = (double *)blk;
    *out = ctx;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_eigen_pencil_ctx_destroy(ldg_math_eigen_pencil_ctx_t **ctx)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !*ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if ((*ctx)->basis != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->basis);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->shifted != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->shifted);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->apply != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->apply);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->bprod != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->bprod);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->diag != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->diag);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->offdiag != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->offdiag);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->ritz_vec != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->ritz_vec);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    if ((*ctx)->ritz_val != 0x0)
    {
        err = ldg_mem_dealloc((*ctx)->ritz_val);
        if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }
    }

    err = ldg_mem_dealloc(*ctx);
    if (err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = err; }

    *ctx = 0x0;
    return ret;
}

LDG_EXPORT uint32_t ldg_math_eigen_pencil_shift_invert_compute(ldg_math_eigen_pencil_ctx_t *ctx, ldg_math_lu_ctx_t *lu, const ldg_math_complex_t *mat_a, const ldg_math_complex_t *mat_b, double sigma, double tol, double *out_val, ldg_math_complex_t *out_vec, uint64_t *out_cunt)
{
    ldg_math_complex_t *q_cur = 0x0;
    ldg_math_complex_t *q_next = 0x0;
    ldg_math_complex_t *f_vec = 0x0;
    ldg_math_complex_t proj = LDG_STRUCT_ZERO_INIT;
    double alpha = 0.0;
    double beta_val = 0.0;
    double beta_res = 0.0;
    double bnorm_sq = 0.0;
    double tnorm = 0.0;
    double givens_r = 0.0;
    double givens_c = 0.0;
    double givens_s = 0.0;
    double band_lead = 0.0;
    double band_sub = 0.0;
    double r_min = 0.0;
    double r_max = 0.0;
    double f_bnorm = 0.0;
    double theta = 0.0;
    double theta_max = 0.0;
    double res_val = 0.0;
    double z_last = 0.0;
    double lam = 0.0;
    double norm_val = 0.0;
    double held = 0.0;
    uint64_t dim = 0;
    uint64_t cap = 0;
    uint64_t steps = 0;
    uint64_t step = 0;
    uint64_t pos = 0;
    uint64_t row = 0;
    uint64_t idx = 0;
    uint64_t pass = 0;
    uint64_t ritz_dim = 0;
    uint64_t cunt = 0;
    uint64_t sort_idx = 0;
    uint8_t broke = LDG_TRUTH_FALSE;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !lu || !mat_a || !mat_b || !out_val || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    if (LDG_UNLIKELY(!(tol >= 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    dim = ctx->dim;
    if (LDG_UNLIKELY(lu->dim != dim)) { return LDG_ERR_MATH_DIM_MISMATCH; }

    for (row = 0; row < dim; row++)
    {
        ctx->shifted[row * dim + row].re = mat_a[row * dim + row].re - sigma * mat_b[row * dim + row].re;
        ctx->shifted[row * dim + row].im = 0.0;
        for (pos = 0; pos < row; pos++)
        {
            ctx->shifted[row * dim + pos].re = mat_a[row * dim + pos].re - sigma * mat_b[row * dim + pos].re;
            ctx->shifted[row * dim + pos].im = mat_a[row * dim + pos].im - sigma * mat_b[row * dim + pos].im;
            ctx->shifted[pos * dim + row].re = ctx->shifted[row * dim + pos].re;
            ctx->shifted[pos * dim + row].im = 0.0 - ctx->shifted[row * dim + pos].im;
        }
    }
    err = ldg_math_lu_factor_compute(lu, ctx->shifted);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    cap = (ctx->step_cap < dim) ? ctx->step_cap : dim;
    for (row = 0; row < dim; row++)
    {
        ctx->basis[row].re = 1.0 + 0.5 * ldg_math_cos_get(2.3 * (double)row);
        ctx->basis[row].im = 0.0;
    }
    eigen_pencil_bmul(mat_b, dim, ctx->basis, ctx->bprod);
    bnorm_sq = 0.0;
    for (row = 0; row < dim; row++) { bnorm_sq += ctx->basis[row].re * ctx->bprod[row].re + ctx->basis[row].im * ctx->bprod[row].im; }
    if (LDG_UNLIKELY(!(bnorm_sq > 0.0))) { return LDG_ERR_MATH_STATE; }

    norm_val = ldg_math_sqrt_get(bnorm_sq);
    for (row = 0; row < dim; row++)
    {
        ctx->basis[row].re /= norm_val;
        ctx->basis[row].im /= norm_val;
    }
    for (step = 0; step < cap; step++)
    {
        q_cur = &ctx->basis[step * dim];
        q_next = &ctx->basis[(step + 1) * dim];
        eigen_pencil_bmul(mat_b, dim, q_cur, ctx->bprod);
        err = ldg_math_lu_solve_compute(lu, ctx->bprod, 1, ctx->apply);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        alpha = 0.0;
        for (row = 0; row < dim; row++)
        {
            q_next[row] = ctx->apply[row];
            alpha += ctx->bprod[row].re * ctx->apply[row].re + ctx->bprod[row].im * ctx->apply[row].im;
        }
        ctx->diag[step] = alpha;
        for (row = 0; row < dim; row++)
        {
            q_next[row].re -= alpha * q_cur[row].re;
            q_next[row].im -= alpha * q_cur[row].im;
            if (step > 0)
            {
                q_next[row].re -= ctx->offdiag[step - 1] * ctx->basis[(step - 1) * dim + row].re;
                q_next[row].im -= ctx->offdiag[step - 1] * ctx->basis[(step - 1) * dim + row].im;
            }
        }
        for (pass = 0; pass < 2; pass++)
        {
            eigen_pencil_bmul(mat_b, dim, q_next, ctx->bprod);
            for (pos = 0; pos <= step; pos++)
            {
                proj.re = 0.0;
                proj.im = 0.0;
                for (row = 0; row < dim; row++)
                {
                    proj.re += ctx->basis[pos * dim + row].re * ctx->bprod[row].re + ctx->basis[pos * dim + row].im * ctx->bprod[row].im;
                    proj.im += ctx->basis[pos * dim + row].re * ctx->bprod[row].im - ctx->basis[pos * dim + row].im * ctx->bprod[row].re;
                }
                for (row = 0; row < dim; row++)
                {
                    q_next[row].re -= proj.re * ctx->basis[pos * dim + row].re - proj.im * ctx->basis[pos * dim + row].im;
                    q_next[row].im -= proj.re * ctx->basis[pos * dim + row].im + proj.im * ctx->basis[pos * dim + row].re;
                }
            }
        }
        eigen_pencil_bmul(mat_b, dim, q_next, ctx->bprod);
        bnorm_sq = 0.0;
        for (row = 0; row < dim; row++) { bnorm_sq += q_next[row].re * ctx->bprod[row].re + q_next[row].im * ctx->bprod[row].im; }
        beta_val = (bnorm_sq > 0.0) ? ldg_math_sqrt_get(bnorm_sq) : 0.0;
        held = ldg_math_abs_get(alpha) + beta_val + ((step > 0) ? ctx->offdiag[step - 1] : 0.0);
        if (held > tnorm) { tnorm = held; }

        steps = step + 1;
        ctx->offdiag[step] = beta_val;
        if (beta_val <= tnorm * DBL_EPSILON * (double)dim)
        {
            ctx->offdiag[step] = 0.0;
            for (row = 0; row < dim; row++)
            {
                q_next[row].re = 0.0;
                q_next[row].im = 0.0;
            }
            broke = LDG_TRUTH_TRUE;
            break;
        }

        for (row = 0; row < dim; row++)
        {
            q_next[row].re /= beta_val;
            q_next[row].im /= beta_val;
        }
    }
    beta_res = ctx->offdiag[steps - 1];
    if (steps >= 2 && broke != LDG_TRUTH_TRUE)
    {
        for (row = 0; row < steps; row++) { for (pos = 0; pos < steps; pos++) { ctx->ritz_vec[row * steps + pos] = (row == pos) ? 1.0 : 0.0; } }
        for (pos = 0; pos + 1 < steps; pos++) { ctx->ritz_val[pos] = 0.0; }
        r_min = 0.0;
        r_max = 0.0;
        band_lead = ctx->diag[0];
        band_sub = ctx->offdiag[0];
        for (pos = 0; pos + 1 < steps; pos++)
        {
            givens_r = ldg_math_hypotenuse_get(band_lead, band_sub);
            if (givens_r > 0.0)
            {
                givens_c = band_lead / givens_r;
                givens_s = band_sub / givens_r;
            }
            else
            {
                givens_c = 1.0;
                givens_s = 0.0;
            }

            if (pos == 0 || givens_r < r_min) { r_min = givens_r; }

            if (givens_r > r_max) { r_max = givens_r; }

            held = ctx->offdiag[pos];
            ctx->diag[pos] = givens_r;
            ctx->offdiag[pos] = givens_c * held + givens_s * ctx->diag[pos + 1];
            band_lead = (0.0 - givens_s) * held + givens_c * ctx->diag[pos + 1];
            if (pos + 2 < steps)
            {
                band_sub = ctx->offdiag[pos + 1];
                ctx->ritz_val[pos] = givens_s * band_sub;
                ctx->offdiag[pos + 1] = givens_c * band_sub;
            }

            for (row = 0; row < steps; row++)
            {
                held = ctx->ritz_vec[row * steps + pos];
                ctx->ritz_vec[row * steps + pos] = givens_c * held + givens_s * ctx->ritz_vec[row * steps + pos + 1];
                ctx->ritz_vec[row * steps + pos + 1] = (0.0 - givens_s) * held + givens_c * ctx->ritz_vec[row * steps + pos + 1];
            }
        }
        ctx->diag[steps - 1] = band_lead;
        if (ldg_math_abs_get(band_lead) < r_min) { r_min = ldg_math_abs_get(band_lead); }

        if (ldg_math_abs_get(band_lead) > r_max) { r_max = ldg_math_abs_get(band_lead); }

        if (LDG_UNLIKELY(r_min <= DBL_EPSILON * r_max)) { return LDG_ERR_MATH_STATE; }

        for (pos = 0; pos < steps; pos++)
        {
            held = ctx->diag[pos] * ctx->ritz_vec[pos * steps + pos];
            if (pos + 1 < steps) { held += ctx->offdiag[pos] * ctx->ritz_vec[(pos + 1) * steps + pos]; }

            if (pos + 2 < steps) { held += ctx->ritz_val[pos] * ctx->ritz_vec[(pos + 2) * steps + pos]; }

            ctx->apply[pos].re = held;
            if (pos + 1 < steps)
            {
                held = ctx->diag[pos + 1] * ctx->ritz_vec[(pos + 1) * steps + pos];
                if (pos + 2 < steps) { held += ctx->offdiag[pos + 1] * ctx->ritz_vec[(pos + 2) * steps + pos]; }

                if (pos + 3 < steps) { held += ctx->ritz_val[pos + 1] * ctx->ritz_vec[(pos + 3) * steps + pos]; }

                ctx->apply[pos].im = held;
            }
            else { ctx->apply[pos].im = 0.0; }
        }
        for (pos = 0; pos < steps; pos++)
        {
            ctx->diag[pos] = ctx->apply[pos].re;
            if (pos + 1 < steps) { ctx->offdiag[pos] = ctx->apply[pos].im; }
        }
        for (row = 0; row < dim; row++)
        {
            for (pos = 0; pos < steps; pos++)
            {
                ctx->apply[pos].re = 0.0;
                ctx->apply[pos].im = 0.0;
                for (idx = 0; idx < steps; idx++)
                {
                    ctx->apply[pos].re += ctx->basis[idx * dim + row].re * ctx->ritz_vec[idx * steps + pos];
                    ctx->apply[pos].im += ctx->basis[idx * dim + row].im * ctx->ritz_vec[idx * steps + pos];
                }
            }
            for (pos = 0; pos < steps; pos++) { ctx->basis[pos * dim + row] = ctx->apply[pos]; }
        }
        f_vec = &ctx->basis[steps * dim];
        for (row = 0; row < dim; row++)
        {
            f_vec[row].re = ctx->basis[(steps - 1) * dim + row].re * ctx->offdiag[steps - 2] + f_vec[row].re * beta_res * ctx->ritz_vec[(steps - 1) * steps + (steps - 2)];
            f_vec[row].im = ctx->basis[(steps - 1) * dim + row].im * ctx->offdiag[steps - 2] + f_vec[row].im * beta_res * ctx->ritz_vec[(steps - 1) * steps + (steps - 2)];
        }
        ritz_dim = steps - 1;
    }
    else
    {
        f_vec = &ctx->basis[steps * dim];
        for (row = 0; row < dim; row++)
        {
            f_vec[row].re *= beta_res;
            f_vec[row].im *= beta_res;
        }
        ritz_dim = steps;
    }

    for (row = 0; row < ritz_dim; row++) { for (pos = 0; pos < ritz_dim; pos++) { ctx->ritz_vec[row * ritz_dim + pos] = (row == pos) ? 1.0 : 0.0; } }
    err = eigen_tridiag_qr_run(ctx->diag, ctx->offdiag, ritz_dim, ctx->ritz_vec, ritz_dim);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    eigen_pencil_bmul(mat_b, dim, f_vec, ctx->bprod);
    bnorm_sq = 0.0;
    for (row = 0; row < dim; row++) { bnorm_sq += f_vec[row].re * ctx->bprod[row].re + f_vec[row].im * ctx->bprod[row].im; }
    f_bnorm = (bnorm_sq > 0.0) ? ldg_math_sqrt_get(bnorm_sq) : 0.0;
    theta_max = 0.0;
    for (idx = 0; idx < ritz_dim; idx++) { if (ldg_math_abs_get(ctx->diag[idx]) > theta_max) { theta_max = ldg_math_abs_get(ctx->diag[idx]); } }
    cunt = 0;
    for (idx = 0; idx < ritz_dim; idx++)
    {
        theta = ctx->diag[idx];
        if (ldg_math_abs_get(theta) <= DBL_EPSILON * theta_max) { continue; }

        z_last = ctx->ritz_vec[(ritz_dim - 1) * ritz_dim + idx];
        res_val = f_bnorm * ldg_math_abs_get(z_last);
        if (res_val > tol * ldg_math_abs_get(theta)) { continue; }

        lam = sigma + 1.0 / theta;
        out_val[cunt] = lam;
        if (out_vec != 0x0)
        {
            for (row = 0; row < dim; row++)
            {
                ctx->apply[row].re = f_vec[row].re * z_last;
                ctx->apply[row].im = f_vec[row].im * z_last;
                for (pos = 0; pos < ritz_dim; pos++)
                {
                    ctx->apply[row].re += theta * ctx->basis[pos * dim + row].re * ctx->ritz_vec[pos * ritz_dim + idx];
                    ctx->apply[row].im += theta * ctx->basis[pos * dim + row].im * ctx->ritz_vec[pos * ritz_dim + idx];
                }
            }
            if (broke == LDG_TRUTH_TRUE) { for (pass = 0; pass < 2; pass++)
                {
                    eigen_pencil_bmul(mat_b, dim, ctx->apply, ctx->bprod);
                    err = ldg_math_lu_solve_compute(lu, ctx->bprod, 1, ctx->apply);
                    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
                }
            }

            norm_val = 0.0;
            for (row = 0; row < dim; row++) { norm_val += ctx->apply[row].re * ctx->apply[row].re + ctx->apply[row].im * ctx->apply[row].im; }
            norm_val = ldg_math_sqrt_get(norm_val);
            if (norm_val > 0.0) { for (row = 0; row < dim; row++)
                {
                    out_vec[cunt * dim + row].re = ctx->apply[row].re / norm_val;
                    out_vec[cunt * dim + row].im = ctx->apply[row].im / norm_val;
                }
            }
        }

        cunt++;
    }
    for (idx = 0; idx + 1 < cunt; idx++)
    {
        sort_idx = idx;
        for (pos = idx + 1; pos < cunt; pos++) { if (out_val[pos] < out_val[sort_idx]) { sort_idx = pos; } }
        if (sort_idx != idx)
        {
            held = out_val[idx];
            out_val[idx] = out_val[sort_idx];
            out_val[sort_idx] = held;
            if (out_vec != 0x0) { for (row = 0; row < dim; row++)
                {
                    proj = out_vec[idx * dim + row];
                    out_vec[idx * dim + row] = out_vec[sort_idx * dim + row];
                    out_vec[sort_idx * dim + row] = proj;
                }
            }
        }
    }
    *out_cunt = cunt;
    return LDG_ERR_AOK;
}
