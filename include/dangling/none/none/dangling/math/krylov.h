#ifndef LDG_MATH_KRYLOV_H
#define LDG_MATH_KRYLOV_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/sparse.h>

typedef enum ldg_math_krylov_precond
{
    LDG_MATH_KRYLOV_PRECOND_NONE = 0,
    LDG_MATH_KRYLOV_PRECOND_JACOBI = 1,
    LDG_MATH_KRYLOV_PRECOND_IC0 = 2
} ldg_math_krylov_precond_t;

typedef struct ldg_math_krylov_res
{
    uint64_t iters;
    double rel_resid;
    uint8_t converged;
    uint8_t pudding[7];
} ldg_math_krylov_res_t;

typedef struct ldg_math_krylov_ctx
{
    double *resid;
    double *dir;
    double *a_dir;
    double *precond_resid;
    double *inv_diag;
    uint64_t *l_row_ptr;
    uint64_t *l_col_idx;
    double *l_vals;
    double *l_diag;
    uint64_t max_dim;
    uint64_t max_nnz;
    uint64_t dim;
    uint64_t l_nnz;
    uint32_t precond_kind;
    uint8_t precond_built;
    uint8_t pudding[3];
} ldg_math_krylov_ctx_t;

LDG_EXPORT uint32_t ldg_math_krylov_ctx_create(uint64_t max_dim, uint64_t max_nnz, ldg_math_krylov_precond_t precond_kind, ldg_math_krylov_ctx_t **out);
LDG_EXPORT uint32_t ldg_math_krylov_ctx_destroy(ldg_math_krylov_ctx_t **ctx);
LDG_EXPORT uint32_t ldg_math_krylov_precond_build(ldg_math_krylov_ctx_t *ctx, const ldg_math_sparse_t *mat);
LDG_EXPORT uint32_t ldg_math_krylov_cg_solve(ldg_math_krylov_ctx_t *ctx, const ldg_math_sparse_t *mat, const double *rhs, double *sol, double tol, uint64_t max_iters, ldg_math_krylov_res_t *res);
LDG_EXPORT uint32_t ldg_math_krylov_pcg_solve(ldg_math_krylov_ctx_t *ctx, const ldg_math_sparse_t *mat, const double *rhs, double *sol, double tol, uint64_t max_iters, ldg_math_krylov_res_t *res);

#endif
