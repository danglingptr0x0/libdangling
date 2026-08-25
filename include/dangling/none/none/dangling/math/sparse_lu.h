#ifndef LDG_MATH_SPARSE_LU_H
#define LDG_MATH_SPARSE_LU_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/sparse.h>
#include <dangling/mem/alloc.h>

#define LDG_MATH_SPARSE_LU_PIVOT_THRESH_DEFAULT 0.001
#define LDG_MATH_SPARSE_LU_PIVOT_THRESH_MAX 1.0
#define LDG_MATH_SPARSE_LU_SLACK_MUL 2

typedef struct ldg_math_sparse_lu_ctx
{
    void *pool;
    const ldg_math_sparse_t *src;
    double *val;
    double *work;
    double *buff;
    uint8_t *occ;
    uint32_t *slot;
    uint64_t *row_perm;
    uint64_t *col_perm;
    uint64_t *col_ord;
    uint64_t *row_cunt;
    uint64_t *col_cunt;
    uint8_t *row_active;
    uint8_t *col_active;
    void *step;
    void *mul;
    void *upd;
    void *fwd;
    void *bck;
    uint64_t dim;
    uint64_t src_nnz;
    uint64_t factor_nnz;
    uint64_t factor_cap;
    uint64_t step_cunt;
    uint64_t mul_cunt;
    uint64_t mul_cap;
    uint64_t upd_cunt;
    uint64_t upd_cap;
    uint64_t fwd_cunt;
    uint64_t fwd_cap;
    uint64_t bck_cunt;
    uint64_t bck_cap;
    double pivot_thresh;
    double src_max;
    double growth;
    uint8_t ready;
    uint8_t pudding[7];
} ldg_math_sparse_lu_ctx_t;

LDG_EXPORT uint32_t ldg_math_sparse_lu_ctx_create(ldg_mem_pool_t *pool, const ldg_math_sparse_t *mat, double pivot_thresh, ldg_math_sparse_lu_ctx_t **out);
LDG_EXPORT uint32_t ldg_math_sparse_lu_ctx_destroy(ldg_math_sparse_lu_ctx_t **ctx);
LDG_EXPORT uint32_t ldg_math_sparse_lu_factor_compute(ldg_math_sparse_lu_ctx_t *ctx, const ldg_math_sparse_t *mat);
LDG_EXPORT uint32_t ldg_math_sparse_lu_refactor_compute(ldg_math_sparse_lu_ctx_t *ctx, const ldg_math_sparse_t *mat);
LDG_EXPORT uint32_t ldg_math_sparse_lu_solve_compute(const ldg_math_sparse_lu_ctx_t *ctx, const double *rhs, double *out);
LDG_EXPORT uint32_t ldg_math_sparse_lu_growth_get(const ldg_math_sparse_lu_ctx_t *ctx, double *out);
LDG_EXPORT uint32_t ldg_math_sparse_lu_pool_size_get(uint64_t dim, uint64_t nnz, uint64_t *out);
LDG_EXPORT uint64_t ldg_math_sparse_lu_factor_nnz_get(const ldg_math_sparse_lu_ctx_t *ctx);

#endif
