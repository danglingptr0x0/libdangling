#include <stdint.h>

#include <dangling/math/sparse_lu.h>
#include <dangling/math/sparse.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/err.h>
#include <dangling/math/math.h>
#include <dangling/core/macros.h>

#define LDG_MATH_SPARSE_LU_STEP 1
#define LDG_MATH_SPARSE_LU_SLOT_NONE UINT32_MAX
#define LDG_MATH_SPARSE_LU_FILL_EST 8

typedef struct sparse_lu_upd
{
    uint32_t dst;
    uint32_t lhs;
    uint32_t rhs;
} sparse_lu_upd_t;

typedef struct sparse_lu_axpy
{
    uint32_t slot;
    uint32_t src;
} sparse_lu_axpy_t;

typedef struct sparse_lu_step
{
    uint32_t pivot;
    uint32_t mul_start;
    uint32_t mul_cunt;
    uint32_t upd_start;
    uint32_t upd_cunt;
    uint32_t fwd_start;
    uint32_t fwd_cunt;
    uint32_t bck_start;
    uint32_t bck_cunt;
    uint32_t pudding[3];
} sparse_lu_step_t;

static uint32_t sparse_lu_ld(ldg_math_sparse_lu_ctx_t *ctx, const ldg_math_sparse_t *mat)
{
    uint64_t total = 0;
    uint64_t row = 0;
    uint64_t next = 0;
    uint64_t idx = 0;
    uint64_t col = 0;
    uint64_t cell = 0;
    double mag = 0.0;
    double peak = 0.0;

    total = ctx->dim * ctx->dim;

    for (idx = 0; idx < total; idx++)
    {
        ctx->work[idx] = 0.0;
        ctx->occ[idx] = LDG_TRUTH_FALSE;
        ctx->slot[idx] = LDG_MATH_SPARSE_LU_SLOT_NONE;
    }

    for (row = 0; row < ctx->dim; row++)
    {
        ctx->row_cunt[row] = 0;
        ctx->col_cunt[row] = 0;
        ctx->row_active[row] = LDG_TRUTH_TRUE;
        ctx->col_active[row] = LDG_TRUTH_TRUE;
        ctx->row_perm[row] = 0;
        ctx->col_perm[row] = 0;
        ctx->col_ord[row] = 0;
    }

    for (row = 0; row < ctx->dim; row++)
    {
        next = row;
        next++;

        for (idx = mat->row_ptr[row]; idx < mat->row_ptr[next]; idx++)
        {
            col = mat->col_idx[idx];
            cell = (row * ctx->dim) + col;

            ctx->work[cell] = mat->vals[idx];
            ctx->occ[cell] = LDG_TRUTH_TRUE;
            ctx->slot[cell] = (uint32_t)idx;

            ctx->row_cunt[row]++;
            ctx->col_cunt[col]++;

            mag = ldg_math_abs_get(mat->vals[idx]);
            if (mag > peak) { peak = mag; }
        }
    }

    ctx->src_max = peak;
    ctx->factor_nnz = mat->nnz;

    return LDG_ERR_AOK;
}

static uint32_t sparse_lu_pivot_pick(ldg_math_sparse_lu_ctx_t *ctx, uint64_t *out_row, uint64_t *out_col)
{
    uint64_t row = 0;
    uint64_t col = 0;
    uint64_t cell = 0;
    uint64_t mark = 0;
    uint64_t best_mark = UINT64_MAX;
    double col_max = 0.0;
    double mag = 0.0;
    uint8_t found = LDG_TRUTH_FALSE;

    for (col = 0; col < ctx->dim; col++)
    {
        if (ctx->col_active[col] != LDG_TRUTH_TRUE) { continue; }

        col_max = 0.0;

        for (row = 0; row < ctx->dim; row++)
        {
            if (ctx->row_active[row] != LDG_TRUTH_TRUE) { continue; }

            cell = (row * ctx->dim) + col;
            if (ctx->occ[cell] != LDG_TRUTH_TRUE) { continue; }

            mag = ldg_math_abs_get(ctx->work[cell]);
            if (mag > col_max) { col_max = mag; }
        }

        if (col_max <= 0.0) { continue; }

        for (row = 0; row < ctx->dim; row++)
        {
            if (ctx->row_active[row] != LDG_TRUTH_TRUE) { continue; }

            cell = (row * ctx->dim) + col;
            if (ctx->occ[cell] != LDG_TRUTH_TRUE) { continue; }

            mag = ldg_math_abs_get(ctx->work[cell]);
            if (mag < ctx->pivot_thresh * col_max) { continue; }

            mark = (ctx->row_cunt[row] - LDG_MATH_SPARSE_LU_STEP) * (ctx->col_cunt[col] - LDG_MATH_SPARSE_LU_STEP);

            if (found != LDG_TRUTH_TRUE || mark < best_mark)
            {
                best_mark = mark;
                *out_row = row;
                *out_col = col;
                found = LDG_TRUTH_TRUE;
            }
        }
    }

    if (found != LDG_TRUTH_TRUE) { return LDG_ERR_MATH_PIVOT; }

    return LDG_ERR_AOK;
}

static uint32_t sparse_lu_slot_reserve(ldg_math_sparse_lu_ctx_t *ctx, uint64_t cell, uint32_t *out)
{
    if (ctx->slot[cell] != LDG_MATH_SPARSE_LU_SLOT_NONE)
    {
        *out = ctx->slot[cell];
        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY(ctx->factor_nnz >= ctx->factor_cap)) { return LDG_ERR_FULL; }

    ctx->slot[cell] = (uint32_t)ctx->factor_nnz;
    *out = ctx->slot[cell];
    ctx->factor_nnz++;

    return LDG_ERR_AOK;
}

static uint32_t sparse_lu_eliminate(ldg_math_sparse_lu_ctx_t *ctx, uint8_t record)
{
    sparse_lu_step_t *steps = (sparse_lu_step_t *)ctx->step;
    uint32_t *muls = (uint32_t *)ctx->mul;
    sparse_lu_upd_t *upds = (sparse_lu_upd_t *)ctx->upd;
    uint64_t step = 0;
    uint64_t row = 0;
    uint64_t col = 0;
    uint64_t pivot_row = 0;
    uint64_t pivot_col = 0;
    uint64_t pivot_cell = 0;
    uint64_t row_cell = 0;
    uint64_t src_cell = 0;
    uint64_t dst_cell = 0;
    uint32_t pivot_slot = 0;
    uint32_t dst_slot = 0;
    uint32_t lhs_slot = 0;
    uint32_t rhs_slot = 0;
    uint32_t ret = 0;
    double pivot_val = 0.0;
    double recip = 0.0;
    double mult = 0.0;
    double mag = 0.0;
    double worst = 0.0;

    ctx->mul_cunt = 0;
    ctx->upd_cunt = 0;
    ctx->step_cunt = 0;

    worst = ctx->src_max;

    for (step = 0; step < ctx->dim; step++)
    {
        ret = sparse_lu_pivot_pick(ctx, &pivot_row, &pivot_col);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        pivot_cell = (pivot_row * ctx->dim) + pivot_col;
        pivot_val = ctx->work[pivot_cell];

        mag = ldg_math_abs_get(pivot_val);
        if (mag < worst) { worst = mag; }

        ret = sparse_lu_slot_reserve(ctx, pivot_cell, &pivot_slot);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->row_perm[step] = pivot_row;
        ctx->col_perm[step] = pivot_col;
        ctx->col_ord[pivot_col] = step;

        ctx->row_active[pivot_row] = LDG_TRUTH_FALSE;
        ctx->col_active[pivot_col] = LDG_TRUTH_FALSE;

        for (col = 0; col < ctx->dim; col++)
        {
            if (ctx->col_active[col] != LDG_TRUTH_TRUE) { continue; }

            if (ctx->occ[(pivot_row * ctx->dim) + col] == LDG_TRUTH_TRUE) { ctx->col_cunt[col]--; }
        }

        for (row = 0; row < ctx->dim; row++)
        {
            if (ctx->row_active[row] != LDG_TRUTH_TRUE) { continue; }

            if (ctx->occ[(row * ctx->dim) + pivot_col] == LDG_TRUTH_TRUE) { ctx->row_cunt[row]--; }
        }

        if (record == LDG_TRUTH_TRUE)
        {
            steps[step].pivot = pivot_slot;
            steps[step].mul_start = (uint32_t)ctx->mul_cunt;
            steps[step].upd_start = (uint32_t)ctx->upd_cunt;
            steps[step].mul_cunt = 0;
            steps[step].upd_cunt = 0;
        }

        recip = 1.0 / pivot_val;

        for (row = 0; row < ctx->dim; row++)
        {
            if (ctx->row_active[row] != LDG_TRUTH_TRUE) { continue; }

            row_cell = (row * ctx->dim) + pivot_col;
            if (ctx->occ[row_cell] != LDG_TRUTH_TRUE) { continue; }

            ret = sparse_lu_slot_reserve(ctx, row_cell, &lhs_slot);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            mult = ctx->work[row_cell] * recip;
            ctx->work[row_cell] = mult;

            if (record == LDG_TRUTH_TRUE)
            {
                if (LDG_UNLIKELY(ctx->mul_cunt >= ctx->mul_cap)) { return LDG_ERR_FULL; }

                muls[ctx->mul_cunt] = lhs_slot;
                steps[step].mul_cunt++;
            }

            ctx->mul_cunt++;

            for (col = 0; col < ctx->dim; col++)
            {
                if (ctx->col_active[col] != LDG_TRUTH_TRUE) { continue; }

                src_cell = (pivot_row * ctx->dim) + col;
                if (ctx->occ[src_cell] != LDG_TRUTH_TRUE) { continue; }

                dst_cell = (row * ctx->dim) + col;

                if (ctx->occ[dst_cell] != LDG_TRUTH_TRUE)
                {
                    ctx->occ[dst_cell] = LDG_TRUTH_TRUE;
                    ctx->work[dst_cell] = 0.0;
                    ctx->row_cunt[row]++;
                    ctx->col_cunt[col]++;
                }

                ret = sparse_lu_slot_reserve(ctx, dst_cell, &dst_slot);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ret = sparse_lu_slot_reserve(ctx, src_cell, &rhs_slot);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

                ctx->work[dst_cell] -= mult * ctx->work[src_cell];

                if (record == LDG_TRUTH_TRUE)
                {
                    if (LDG_UNLIKELY(ctx->upd_cunt >= ctx->upd_cap)) { return LDG_ERR_FULL; }

                    upds[ctx->upd_cunt].dst = dst_slot;
                    upds[ctx->upd_cunt].lhs = lhs_slot;
                    upds[ctx->upd_cunt].rhs = rhs_slot;
                    steps[step].upd_cunt++;
                }

                ctx->upd_cunt++;
            }
        }

        ctx->step_cunt++;
    }

    if (ctx->src_max > 0.0) { ctx->growth = worst / ctx->src_max; }
    else { ctx->growth = 0.0; }

    return LDG_ERR_AOK;
}

static uint32_t sparse_lu_solve_record(ldg_math_sparse_lu_ctx_t *ctx)
{
    sparse_lu_step_t *steps = (sparse_lu_step_t *)ctx->step;
    sparse_lu_axpy_t *fwds = (sparse_lu_axpy_t *)ctx->fwd;
    sparse_lu_axpy_t *bcks = (sparse_lu_axpy_t *)ctx->bck;
    uint64_t step = 0;
    uint64_t col = 0;
    uint64_t cell = 0;
    uint64_t ord = 0;

    ctx->fwd_cunt = 0;
    ctx->bck_cunt = 0;

    for (step = 0; step < ctx->dim; step++)
    {
        steps[step].fwd_start = (uint32_t)ctx->fwd_cunt;
        steps[step].fwd_cunt = 0;

        for (col = 0; col < ctx->dim; col++)
        {
            cell = (ctx->row_perm[step] * ctx->dim) + col;
            if (ctx->occ[cell] != LDG_TRUTH_TRUE) { continue; }

            ord = ctx->col_ord[col];
            if (ord >= step) { continue; }

            if (LDG_UNLIKELY(ctx->fwd_cunt >= ctx->fwd_cap)) { return LDG_ERR_FULL; }

            fwds[ctx->fwd_cunt].slot = ctx->slot[cell];
            fwds[ctx->fwd_cunt].src = (uint32_t)ord;
            ctx->fwd_cunt++;
            steps[step].fwd_cunt++;
        }
    }

    for (step = 0; step < ctx->dim; step++)
    {
        steps[step].bck_start = (uint32_t)ctx->bck_cunt;
        steps[step].bck_cunt = 0;

        for (col = 0; col < ctx->dim; col++)
        {
            cell = (ctx->row_perm[step] * ctx->dim) + col;
            if (ctx->occ[cell] != LDG_TRUTH_TRUE) { continue; }

            ord = ctx->col_ord[col];
            if (ord <= step) { continue; }

            if (LDG_UNLIKELY(ctx->bck_cunt >= ctx->bck_cap)) { return LDG_ERR_FULL; }

            bcks[ctx->bck_cunt].slot = ctx->slot[cell];
            bcks[ctx->bck_cunt].src = (uint32_t)ord;
            ctx->bck_cunt++;
            steps[step].bck_cunt++;
        }
    }

    return LDG_ERR_AOK;
}

static uint32_t sparse_lu_val_ld(ldg_math_sparse_lu_ctx_t *ctx, const ldg_math_sparse_t *mat)
{
    uint64_t idx = 0;
    double mag = 0.0;
    double peak = 0.0;

    for (idx = 0; idx < mat->nnz; idx++)
    {
        ctx->val[idx] = mat->vals[idx];

        mag = ldg_math_abs_get(mat->vals[idx]);
        if (mag > peak) { peak = mag; }
    }

    for (idx = mat->nnz; idx < ctx->factor_nnz; idx++) { ctx->val[idx] = 0.0; }

    ctx->src_max = peak;

    return LDG_ERR_AOK;
}

static uint32_t sparse_lu_pattern_ck(const ldg_math_sparse_lu_ctx_t *ctx, const ldg_math_sparse_t *mat)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!mat->finalized)) { return LDG_ERR_MATH_STATE; }

    if (LDG_UNLIKELY(mat->rows != ctx->dim || mat->cols != ctx->dim)) { return LDG_ERR_MATH_DIM_MISMATCH; }

    if (LDG_UNLIKELY(mat->nnz != ctx->src_nnz)) { return LDG_ERR_MATH_PATTERN_MISMATCH; }

    if (mat == ctx->src) { return LDG_ERR_AOK; }

    for (idx = 0; idx < mat->nnz; idx++) { if (LDG_UNLIKELY(mat->col_idx[idx] != ctx->src->col_idx[idx])) { return LDG_ERR_MATH_PATTERN_MISMATCH; } }

    for (idx = 0; idx <= ctx->dim; idx++) { if (LDG_UNLIKELY(mat->row_ptr[idx] != ctx->src->row_ptr[idx])) { return LDG_ERR_MATH_PATTERN_MISMATCH; } }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_sparse_lu_pool_size_get(uint64_t dim, uint64_t nnz, uint64_t *out)
{
    uint64_t cells = 0;
    uint64_t fill = 0;
    uint64_t total = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(dim == 0 || nnz == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cells = dim * dim;
    fill = nnz * LDG_MATH_SPARSE_LU_FILL_EST;

    total = (uint64_t)sizeof(ldg_math_sparse_lu_ctx_t);
    total += cells * (uint64_t)sizeof(double);
    total += cells;
    total += cells * (uint64_t)sizeof(uint32_t);
    total += dim * (uint64_t)sizeof(double);
    total += dim * (uint64_t)sizeof(uint64_t) * (uint64_t)LDG_MATH_SPARSE_LU_SLACK_MUL * (uint64_t)LDG_MATH_SPARSE_LU_SLACK_MUL;
    total += dim * (uint64_t)sizeof(sparse_lu_step_t);
    total += fill * (uint64_t)sizeof(double);
    total += fill * (uint64_t)sizeof(uint32_t) * (uint64_t)LDG_MATH_SPARSE_LU_SLACK_MUL;
    total += fill * (uint64_t)sizeof(sparse_lu_upd_t) * (uint64_t)LDG_MATH_SPARSE_LU_SLACK_MUL;
    total += fill * (uint64_t)sizeof(sparse_lu_axpy_t) * (uint64_t)LDG_MATH_SPARSE_LU_SLACK_MUL;

    *out = total;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_sparse_lu_ctx_create(ldg_mem_pool_t *pool, const ldg_math_sparse_t *mat, double pivot_thresh, ldg_math_sparse_lu_ctx_t **out)
{
    ldg_math_sparse_lu_ctx_t *ctx = 0x0;
    void *tmp = 0x0;
    uint64_t cells = 0;
    uint64_t dim = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!pool || !mat || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(!mat->finalized)) { return LDG_ERR_MATH_STATE; }

    if (LDG_UNLIKELY(mat->rows != mat->cols || mat->rows == 0)) { return LDG_ERR_MATH_DIM_MISMATCH; }

    if (LDG_UNLIKELY(mat->nnz == 0)) { return LDG_ERR_MATH_PIVOT; }

    if (LDG_UNLIKELY(pivot_thresh <= 0.0 || pivot_thresh > LDG_MATH_SPARSE_LU_PIVOT_THRESH_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dim = mat->rows;
    cells = dim * dim;

    ret = ldg_mem_pool_alloc(pool, (uint64_t)sizeof(ldg_math_sparse_lu_ctx_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx = (ldg_math_sparse_lu_ctx_t *)tmp;

    ctx->pool = pool;
    ctx->src = mat;
    ctx->dim = dim;
    ctx->src_nnz = mat->nnz;
    ctx->pivot_thresh = pivot_thresh;
    ctx->ready = LDG_TRUTH_FALSE;
    ctx->growth = 0.0;
    ctx->src_max = 0.0;
    ctx->factor_nnz = 0;
    ctx->step_cunt = 0;
    ctx->mul_cunt = 0;
    ctx->upd_cunt = 0;
    ctx->fwd_cunt = 0;
    ctx->bck_cunt = 0;

    ret = ldg_mem_pool_alloc(pool, cells * (uint64_t)sizeof(double), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->work = (double *)tmp;

    ret = ldg_mem_pool_alloc(pool, cells, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->occ = (uint8_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, cells * (uint64_t)sizeof(uint32_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->slot = (uint32_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, dim * (uint64_t)sizeof(double), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->buff = (double *)tmp;

    ret = ldg_mem_pool_alloc(pool, dim * (uint64_t)sizeof(uint64_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->row_perm = (uint64_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, dim * (uint64_t)sizeof(uint64_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->col_perm = (uint64_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, dim * (uint64_t)sizeof(uint64_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->col_ord = (uint64_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, dim * (uint64_t)sizeof(uint64_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->row_cunt = (uint64_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, dim * (uint64_t)sizeof(uint64_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->col_cunt = (uint64_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, dim, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->row_active = (uint8_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, dim, &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->col_active = (uint8_t *)tmp;

    ret = ldg_mem_pool_alloc(pool, dim * (uint64_t)sizeof(sparse_lu_step_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->step = tmp;

    ctx->factor_cap = cells;
    ctx->mul_cap = 0;
    ctx->upd_cap = 0;
    ctx->fwd_cap = 0;
    ctx->bck_cap = 0;
    ctx->mul = 0x0;
    ctx->upd = 0x0;
    ctx->fwd = 0x0;
    ctx->bck = 0x0;

    ret = sparse_lu_ld(ctx, mat);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sparse_lu_eliminate(ctx, LDG_TRUTH_FALSE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->factor_cap = ctx->factor_nnz * (uint64_t)LDG_MATH_SPARSE_LU_SLACK_MUL;
    ctx->mul_cap = ctx->mul_cunt * (uint64_t)LDG_MATH_SPARSE_LU_SLACK_MUL;
    ctx->upd_cap = ctx->upd_cunt * (uint64_t)LDG_MATH_SPARSE_LU_SLACK_MUL;
    ctx->fwd_cap = ctx->factor_cap;
    ctx->bck_cap = ctx->factor_cap;

    if (ctx->mul_cap == 0) { ctx->mul_cap = dim; }

    if (ctx->upd_cap == 0) { ctx->upd_cap = dim; }

    ret = ldg_mem_pool_alloc(pool, ctx->factor_cap * (uint64_t)sizeof(double), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->val = (double *)tmp;

    ret = ldg_mem_pool_alloc(pool, ctx->mul_cap * (uint64_t)sizeof(uint32_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->mul = tmp;

    ret = ldg_mem_pool_alloc(pool, ctx->upd_cap * (uint64_t)sizeof(sparse_lu_upd_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->upd = tmp;

    ret = ldg_mem_pool_alloc(pool, ctx->fwd_cap * (uint64_t)sizeof(sparse_lu_axpy_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->fwd = tmp;

    ret = ldg_mem_pool_alloc(pool, ctx->bck_cap * (uint64_t)sizeof(sparse_lu_axpy_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->bck = tmp;

    ret = ldg_math_sparse_lu_factor_compute(ctx, mat);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = ctx;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_sparse_lu_ctx_destroy(ldg_math_sparse_lu_ctx_t **ctx)
{
    if (LDG_UNLIKELY(!ctx || !*ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    (*ctx)->ready = LDG_TRUTH_FALSE;
    *ctx = 0x0;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_sparse_lu_factor_compute(ldg_math_sparse_lu_ctx_t *ctx, const ldg_math_sparse_t *mat)
{
    uint64_t row = 0;
    uint64_t col = 0;
    uint64_t cell = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !mat)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = sparse_lu_pattern_ck(ctx, mat);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->ready = LDG_TRUTH_FALSE;

    ret = sparse_lu_ld(ctx, mat);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sparse_lu_eliminate(ctx, LDG_TRUTH_TRUE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sparse_lu_solve_record(ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ctx->factor_nnz; idx++) { ctx->val[idx] = 0.0; }

    for (row = 0; row < ctx->dim; row++) { for (col = 0; col < ctx->dim; col++)
        {
            cell = (row * ctx->dim) + col;
            if (ctx->occ[cell] != LDG_TRUTH_TRUE) { continue; }

            ctx->val[ctx->slot[cell]] = ctx->work[cell];
        }
    }

    ctx->ready = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_sparse_lu_refactor_compute(ldg_math_sparse_lu_ctx_t *ctx, const ldg_math_sparse_t *mat)
{
    const sparse_lu_step_t *steps = 0x0;
    const uint32_t *muls = 0x0;
    const sparse_lu_upd_t *upds = 0x0;
    uint64_t step = 0;
    uint64_t idx = 0;
    uint64_t stop = 0;
    uint32_t ret = 0;
    double recip = 0.0;
    double pivot_val = 0.0;
    double mag = 0.0;
    double worst = 0.0;

    if (LDG_UNLIKELY(!ctx || !mat)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->ready != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = sparse_lu_pattern_ck(ctx, mat);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sparse_lu_val_ld(ctx, mat);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    steps = (const sparse_lu_step_t *)ctx->step;
    muls = (const uint32_t *)ctx->mul;
    upds = (const sparse_lu_upd_t *)ctx->upd;

    worst = ctx->src_max;

    for (step = 0; step < ctx->step_cunt; step++)
    {
        pivot_val = ctx->val[steps[step].pivot];

        mag = ldg_math_abs_get(pivot_val);
        if (mag < worst) { worst = mag; }

        if (LDG_UNLIKELY(mag <= 0.0)) { return LDG_ERR_MATH_PIVOT; }

        recip = 1.0 / pivot_val;

        stop = (uint64_t)steps[step].mul_start + (uint64_t)steps[step].mul_cunt;
        for (idx = steps[step].mul_start; idx < stop; idx++) { ctx->val[muls[idx]] *= recip; }

        stop = (uint64_t)steps[step].upd_start + (uint64_t)steps[step].upd_cunt;
        for (idx = steps[step].upd_start; idx < stop; idx++) { ctx->val[upds[idx].dst] -= ctx->val[upds[idx].lhs] * ctx->val[upds[idx].rhs]; }
    }

    if (ctx->src_max > 0.0) { ctx->growth = worst / ctx->src_max; }
    else { ctx->growth = 0.0; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_sparse_lu_solve_compute(const ldg_math_sparse_lu_ctx_t *ctx, const double *rhs, double *out)
{
    const sparse_lu_step_t *steps = 0x0;
    const sparse_lu_axpy_t *fwds = 0x0;
    const sparse_lu_axpy_t *bcks = 0x0;
    uint64_t step = 0;
    uint64_t idx = 0;
    uint64_t stop = 0;
    uint64_t back = 0;
    double acc = 0.0;

    if (LDG_UNLIKELY(!ctx || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->ready != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    steps = (const sparse_lu_step_t *)ctx->step;
    fwds = (const sparse_lu_axpy_t *)ctx->fwd;
    bcks = (const sparse_lu_axpy_t *)ctx->bck;

    for (step = 0; step < ctx->dim; step++)
    {
        acc = rhs[ctx->row_perm[step]];

        stop = (uint64_t)steps[step].fwd_start + (uint64_t)steps[step].fwd_cunt;
        for (idx = steps[step].fwd_start; idx < stop; idx++) { acc -= ctx->val[fwds[idx].slot] * ctx->buff[fwds[idx].src]; }

        ctx->buff[step] = acc;
    }

    for (step = 0; step < ctx->dim; step++)
    {
        back = ctx->dim - LDG_MATH_SPARSE_LU_STEP - step;

        acc = ctx->buff[back];

        stop = (uint64_t)steps[back].bck_start + (uint64_t)steps[back].bck_cunt;
        for (idx = steps[back].bck_start; idx < stop; idx++) { acc -= ctx->val[bcks[idx].slot] * ctx->buff[bcks[idx].src]; }

        ctx->buff[back] = acc / ctx->val[steps[back].pivot];
    }

    for (step = 0; step < ctx->dim; step++) { out[ctx->col_perm[step]] = ctx->buff[step]; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_sparse_lu_growth_get(const ldg_math_sparse_lu_ctx_t *ctx, double *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    if (LDG_UNLIKELY(ctx->ready != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = ctx->growth;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_math_sparse_lu_factor_nnz_get(const ldg_math_sparse_lu_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return UINT64_MAX; }

    return ctx->factor_nnz;
}
