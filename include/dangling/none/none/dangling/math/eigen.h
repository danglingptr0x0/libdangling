#ifndef LDG_MATH_EIGEN_H
#define LDG_MATH_EIGEN_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/complex.h>
#include <dangling/math/sparse.h>
#include <dangling/math/cholesky.h>
#include <dangling/math/lu.h>

typedef struct ldg_math_eigen_ctx
{
    ldg_math_complex_t *work;
    ldg_math_complex_t *house;
    ldg_math_complex_t *scratch;
    ldg_math_complex_t *phase;
    double *diag;
    double *offdiag;
    double *rot;
    uint64_t dim;
} ldg_math_eigen_ctx_t;

typedef struct ldg_math_eigen_lanczos_ctx
{
    double *basis;
    double *diag;
    double *offdiag;
    double *omega;
    double *omega_prev;
    double *ritz_vec;
    double *ritz_val;
    uint64_t dim;
    uint64_t step_cap;
} ldg_math_eigen_lanczos_ctx_t;

LDG_EXPORT uint32_t ldg_math_eigen_ctx_create(uint64_t dim, ldg_math_eigen_ctx_t **out);
LDG_EXPORT uint32_t ldg_math_eigen_ctx_destroy(ldg_math_eigen_ctx_t **ctx);
LDG_EXPORT uint32_t ldg_math_eigen_hermitian_compute(ldg_math_eigen_ctx_t *ctx, const ldg_math_complex_t *mat, double *out_val, ldg_math_complex_t *out_vec);
LDG_EXPORT uint32_t ldg_math_eigen_generalized_compute(ldg_math_eigen_ctx_t *ctx, ldg_math_cholesky_ctx_t *chol, const ldg_math_complex_t *mat_a, const ldg_math_complex_t *mat_b, double *out_val, ldg_math_complex_t *out_vec);
LDG_EXPORT uint32_t ldg_math_eigen_lanczos_ctx_create(uint64_t dim, uint64_t step_cap, ldg_math_eigen_lanczos_ctx_t **out);
LDG_EXPORT uint32_t ldg_math_eigen_lanczos_ctx_destroy(ldg_math_eigen_lanczos_ctx_t **ctx);
LDG_EXPORT uint32_t ldg_math_eigen_lanczos_compute(ldg_math_eigen_lanczos_ctx_t *ctx, const ldg_math_sparse_t *mat, double tol, double *out_val, double *out_vec, uint64_t *out_cunt);

typedef struct ldg_math_eigen_pencil_ctx
{
    ldg_math_complex_t *basis;
    ldg_math_complex_t *shifted;
    ldg_math_complex_t *apply;
    ldg_math_complex_t *bprod;
    double *diag;
    double *offdiag;
    double *ritz_vec;
    double *ritz_val;
    uint64_t dim;
    uint64_t step_cap;
} ldg_math_eigen_pencil_ctx_t;

LDG_EXPORT uint32_t ldg_math_eigen_pencil_ctx_create(uint64_t dim, uint64_t step_cap, ldg_math_eigen_pencil_ctx_t **out);
LDG_EXPORT uint32_t ldg_math_eigen_pencil_ctx_destroy(ldg_math_eigen_pencil_ctx_t **ctx);
LDG_EXPORT uint32_t ldg_math_eigen_pencil_shift_invert_compute(ldg_math_eigen_pencil_ctx_t *ctx, ldg_math_lu_ctx_t *lu, const ldg_math_complex_t *mat_a, const ldg_math_complex_t *mat_b, double sigma, double tol, double *out_val, ldg_math_complex_t *out_vec, uint64_t *out_cunt);

typedef struct ldg_math_eigen_schur_ctx
{
    double *work;
    double *basis;
    double *house;
    double *scale;
    ldg_math_complex_t *val;
    ldg_math_complex_t *stage;
    uint64_t dim;
    uint8_t ready;
    uint8_t pudding[7];
} ldg_math_eigen_schur_ctx_t;

LDG_EXPORT uint32_t ldg_math_eigen_schur_ctx_create(uint64_t dim, ldg_math_eigen_schur_ctx_t **out);
LDG_EXPORT uint32_t ldg_math_eigen_schur_ctx_destroy(ldg_math_eigen_schur_ctx_t **ctx);
LDG_EXPORT uint32_t ldg_math_eigen_schur_compute(ldg_math_eigen_schur_ctx_t *ctx, const double *mat, ldg_math_complex_t *out_val);
LDG_EXPORT uint32_t ldg_math_eigen_schur_vec_compute(ldg_math_eigen_schur_ctx_t *ctx, ldg_math_lu_ctx_t *lu, uint64_t which, ldg_math_complex_t *out_vec);

#endif
