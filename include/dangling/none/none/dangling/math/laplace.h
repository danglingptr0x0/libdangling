#ifndef LDG_MATH_LAPLACE_H
#define LDG_MATH_LAPLACE_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/sparse.h>
#include <dangling/math/krylov.h>

typedef struct ldg_math_laplace_edge
{
    uint64_t vert_a;
    uint64_t vert_b;
    double weight;
} ldg_math_laplace_edge_t;

typedef struct ldg_math_laplace_pin
{
    uint64_t vert;
    double potential;
} ldg_math_laplace_pin_t;

typedef struct ldg_math_laplace_res
{
    uint64_t iters;
    uint64_t free_cunt;
    double rel_resid;
    uint8_t converged;
    uint8_t pudding[7];
} ldg_math_laplace_res_t;

typedef struct ldg_math_laplace_ctx
{
    ldg_math_sparse_t *op;
    ldg_math_krylov_ctx_t *krylov;
    uint64_t *full_to_free;
    uint64_t *free_to_full;
    double *pin_val;
    double *rhs;
    double *sol;
    double *cg_resid;
    double *cg_dir;
    double *cg_a_dir;
    double *full_src;
    double *full_pot;
    uint64_t max_verts;
    uint64_t max_edges;
    uint32_t precond_kind;
    uint8_t pudding[4];
} ldg_math_laplace_ctx_t;

LDG_EXPORT uint32_t ldg_math_laplace_ctx_create(uint64_t max_verts, uint64_t max_edges, ldg_math_krylov_precond_t precond_kind, ldg_math_laplace_ctx_t **out);
LDG_EXPORT uint32_t ldg_math_laplace_ctx_destroy(ldg_math_laplace_ctx_t **ctx);
LDG_EXPORT uint32_t ldg_math_laplace_solve(ldg_math_laplace_ctx_t *ctx, uint64_t vert_cunt, const ldg_math_laplace_edge_t *edge, uint64_t edge_cunt, const ldg_math_laplace_pin_t *pin, uint64_t pin_cunt, const double *src, double gmin, double tol, uint64_t max_iters, double *potential_out, ldg_math_laplace_res_t *res);
LDG_EXPORT uint32_t ldg_math_laplace_anchor_solve(ldg_math_laplace_ctx_t *ctx, uint64_t vert_cunt, const ldg_math_laplace_edge_t *edge, uint64_t edge_cunt, const ldg_math_laplace_pin_t *pin, uint64_t pin_cunt, const double *src, double anchor_beta, double tol, uint64_t max_iters, double *potential_out, ldg_math_laplace_res_t *res);
LDG_EXPORT uint32_t ldg_math_laplace_resistance_get(ldg_math_laplace_ctx_t *ctx, uint64_t vert_cunt, const ldg_math_laplace_edge_t *edge, uint64_t edge_cunt, uint64_t vert_i, uint64_t vert_j, double gmin, double tol, uint64_t max_iters, double *out);
LDG_EXPORT uint32_t ldg_math_laplace_tree_cunt_get(ldg_math_laplace_ctx_t *ctx, uint64_t vert_cunt, const ldg_math_laplace_edge_t *edge, uint64_t edge_cunt, double *dense_scratch, double *out);
LDG_EXPORT uint32_t ldg_math_laplace_kron_reduce(ldg_math_laplace_ctx_t *ctx, uint64_t vert_cunt, const ldg_math_laplace_edge_t *edge, uint64_t edge_cunt, const uint64_t *retain, uint64_t retain_cunt, double tol, uint64_t max_iters, double *reduced_out);

#endif
