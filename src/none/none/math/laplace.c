#include <string.h>

#include <dangling/math/laplace.h>
#include <dangling/math/krylov.h>
#include <dangling/math/sparse.h>
#include <dangling/math/vec.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

static uint32_t laplace_alloc_doubles(uint64_t cunt, double **out)
{
    void *tmp = 0x0;
    uint32_t ret = 0;

    ret = ldg_mem_alloc(cunt * (uint64_t)sizeof(double), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_MATH_ALLOC; }

    *out = (double *)tmp;

    return LDG_MATH_OK;
}

static uint32_t laplace_alloc_u64(uint64_t cunt, uint64_t **out)
{
    void *tmp = 0x0;
    uint32_t ret = 0;

    ret = ldg_mem_alloc(cunt * (uint64_t)sizeof(uint64_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_MATH_ALLOC; }

    *out = (uint64_t *)tmp;

    return LDG_MATH_OK;
}

static void laplace_ctx_destroy_internal(ldg_math_laplace_ctx_t *ctx)
{
    if (ctx->op) { ldg_math_sparse_destroy(&ctx->op); }

    if (ctx->krylov) { ldg_math_krylov_ctx_destroy(&ctx->krylov); }

    if (ctx->full_to_free) { ldg_mem_dealloc(ctx->full_to_free); }

    if (ctx->free_to_full) { ldg_mem_dealloc(ctx->free_to_full); }

    if (ctx->pin_val) { ldg_mem_dealloc(ctx->pin_val); }

    if (ctx->rhs) { ldg_mem_dealloc(ctx->rhs); }

    if (ctx->sol) { ldg_mem_dealloc(ctx->sol); }

    if (ctx->cg_resid) { ldg_mem_dealloc(ctx->cg_resid); }

    if (ctx->cg_dir) { ldg_mem_dealloc(ctx->cg_dir); }

    if (ctx->cg_a_dir) { ldg_mem_dealloc(ctx->cg_a_dir); }

    if (ctx->full_src) { ldg_mem_dealloc(ctx->full_src); }

    if (ctx->full_pot) { ldg_mem_dealloc(ctx->full_pot); }

    ldg_mem_dealloc(ctx);
}

static uint32_t laplace_pins_register(ldg_math_laplace_ctx_t *ctx, uint64_t vert_cunt, const ldg_math_laplace_pin_t *pin, uint64_t pin_cunt)
{
    uint64_t idx = 0;
    uint64_t vert = 0;

    for (idx = 0; idx < vert_cunt; idx++) { ctx->full_to_free[idx] = 0; }

    for (idx = 0; idx < pin_cunt; idx++)
    {
        vert = pin[idx].vert;
        if (LDG_UNLIKELY(vert >= vert_cunt)) { return LDG_MATH_BOUNDS; }

        if (LDG_UNLIKELY(ctx->full_to_free[vert] == UINT64_MAX)) { return LDG_MATH_ARG_INVALID; }

        ctx->full_to_free[vert] = UINT64_MAX;
        ctx->pin_val[vert] = pin[idx].potential;
    }

    return LDG_MATH_OK;
}

static uint64_t laplace_free_map_build(ldg_math_laplace_ctx_t *ctx, uint64_t vert_cunt)
{
    uint64_t vert = 0;
    uint64_t free_cunt = 0;

    for (vert = 0; vert < vert_cunt; vert++)
    {
        if (ctx->full_to_free[vert] == UINT64_MAX) { continue; }

        ctx->full_to_free[vert] = free_cunt;
        ctx->free_to_full[free_cunt] = vert;
        free_cunt++;
    }

    return free_cunt;
}

static uint32_t laplace_edges_validate(uint64_t vert_cunt, const ldg_math_laplace_edge_t *edge, uint64_t edge_cunt)
{
    uint64_t idx = 0;

    for (idx = 0; idx < edge_cunt; idx++)
    {
        if (LDG_UNLIKELY(edge[idx].vert_a >= vert_cunt || edge[idx].vert_b >= vert_cunt)) { return LDG_MATH_BOUNDS; }

        if (LDG_UNLIKELY(edge[idx].vert_a == edge[idx].vert_b)) { return LDG_MATH_ARG_INVALID; }

        if (LDG_UNLIKELY(edge[idx].weight <= 0.0)) { return LDG_MATH_ARG_INVALID; }
    }

    return LDG_MATH_OK;
}

static uint32_t laplace_assemble(ldg_math_laplace_ctx_t *ctx, uint64_t free_cunt, const ldg_math_laplace_edge_t *edge, uint64_t edge_cunt, const double *src, double gmin)
{
    uint64_t idx = 0;
    uint64_t vert = 0;
    uint64_t free_a = 0;
    uint64_t free_b = 0;
    double weight = 0.0;
    uint32_t ret = 0;

    ctx->op->rows = free_cunt;
    ctx->op->cols = free_cunt;
    ctx->op->finalized = 0;
    ctx->op->coo_cunt = 0;
    ctx->op->nnz = 0;

    for (idx = 0; idx < free_cunt; idx++)
    {
        vert = ctx->free_to_full[idx];
        ret = ldg_math_sparse_entry_add(ctx->op, idx, idx, gmin);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ctx->rhs[idx] = (src != 0x0) ? src[vert] : 0.0;
    }

    for (idx = 0; idx < edge_cunt; idx++)
    {
        free_a = ctx->full_to_free[edge[idx].vert_a];
        free_b = ctx->full_to_free[edge[idx].vert_b];
        weight = edge[idx].weight;

        if (free_a != UINT64_MAX && free_b != UINT64_MAX)
        {
            ret = ldg_math_sparse_entry_add(ctx->op, free_a, free_a, weight);
            if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

            ret = ldg_math_sparse_entry_add(ctx->op, free_b, free_b, weight);
            if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

            ret = ldg_math_sparse_entry_add(ctx->op, free_a, free_b, -weight);
            if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

            ret = ldg_math_sparse_entry_add(ctx->op, free_b, free_a, -weight);
            if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }
        }
        else if (free_a != UINT64_MAX)
        {
            ret = ldg_math_sparse_entry_add(ctx->op, free_a, free_a, weight);
            if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

            ctx->rhs[free_a] += weight * ctx->pin_val[edge[idx].vert_b];
        }
        else if (free_b != UINT64_MAX)
        {
            ret = ldg_math_sparse_entry_add(ctx->op, free_b, free_b, weight);
            if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

            ctx->rhs[free_b] += weight * ctx->pin_val[edge[idx].vert_a];
        }
    }

    return ldg_math_sparse_finalize(ctx->op);
}

static void laplace_scatter(ldg_math_laplace_ctx_t *ctx, uint64_t vert_cunt, double *potential_out)
{
    uint64_t vert = 0;

    for (vert = 0; vert < vert_cunt; vert++) { if (ctx->full_to_free[vert] == UINT64_MAX) { potential_out[vert] = ctx->pin_val[vert]; }
        else { potential_out[vert] = ctx->sol[ctx->full_to_free[vert]]; } }
}

static uint32_t laplace_apply_anchor(ldg_math_laplace_ctx_t *ctx, uint64_t dim, double anchor_term, const double *vec, double *out)
{
    uint64_t idx = 0;
    double mean_sum = 0.0;
    uint32_t ret = 0;

    ret = ldg_math_sparse_mv(ctx->op, vec, out);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    if (anchor_term > 0.0)
    {
        for (idx = 0; idx < dim; idx++) { mean_sum += vec[idx]; }
        mean_sum *= anchor_term;
        for (idx = 0; idx < dim; idx++) { out[idx] += mean_sum; }
    }

    return LDG_MATH_OK;
}

static uint32_t laplace_cg_anchor(ldg_math_laplace_ctx_t *ctx, uint64_t dim, double anchor_term, double tol, uint64_t max_iters, ldg_math_laplace_res_t *res)
{
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

    res->iters = 0;
    res->rel_resid = 0.0;
    res->converged = 0;

    ret = ldg_math_vec_norm2_get(ctx->rhs, dim, &rhs_norm);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    denom = (rhs_norm > 0.0) ? rhs_norm : 1.0;

    ret = laplace_apply_anchor(ctx, dim, anchor_term, ctx->sol, ctx->cg_a_dir);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = ldg_math_vec_sub(ctx->cg_resid, ctx->rhs, ctx->cg_a_dir, dim);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = ldg_math_vec_norm2_get(ctx->cg_resid, dim, &resid_norm);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    rel = resid_norm / denom;
    res->rel_resid = rel;
    if (rel <= tol)
    {
        res->converged = 1;
        return LDG_MATH_OK;
    }

    ret = ldg_math_vec_cpy(ctx->cg_dir, ctx->cg_resid, dim);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = ldg_math_vec_dot_get(ctx->cg_resid, ctx->cg_resid, dim, &rz_prev);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    for (iter = 1; iter <= max_iters; iter++)
    {
        ret = laplace_apply_anchor(ctx, dim, anchor_term, ctx->cg_dir, ctx->cg_a_dir);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_vec_dot_get(ctx->cg_dir, ctx->cg_a_dir, dim, &dir_a_dir);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        if (LDG_UNLIKELY(dir_a_dir <= 0.0)) { return LDG_MATH_NOT_SPD; }

        alpha = rz_prev / dir_a_dir;

        ret = ldg_math_vec_axpy(ctx->sol, ctx->cg_dir, alpha, dim);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_vec_axpy(ctx->cg_resid, ctx->cg_a_dir, -alpha, dim);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_vec_norm2_get(ctx->cg_resid, dim, &resid_norm);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        rel = resid_norm / denom;
        res->iters = iter;
        res->rel_resid = rel;
        if (rel <= tol)
        {
            res->converged = 1;
            return LDG_MATH_OK;
        }

        ret = ldg_math_vec_dot_get(ctx->cg_resid, ctx->cg_resid, dim, &rz_curr);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        if (LDG_UNLIKELY(rz_prev <= 0.0)) { return LDG_MATH_NOT_SPD; }

        beta = rz_curr / rz_prev;

        ret = ldg_math_vec_scale(ctx->cg_dir, beta, dim);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_vec_axpy(ctx->cg_dir, ctx->cg_resid, 1.0, dim);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        rz_prev = rz_curr;
    }

    return LDG_MATH_NOT_CONVERGED;
}

LDG_EXPORT uint32_t ldg_math_laplace_ctx_create(uint64_t max_verts, uint64_t max_edges, ldg_math_krylov_precond_t precond_kind, ldg_math_laplace_ctx_t **out)
{
    ldg_math_laplace_ctx_t *ctx = 0x0;
    void *tmp = 0x0;
    uint64_t max_nnz = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_MATH_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(max_verts == 0)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(max_edges > (UINT64_MAX - max_verts) / 4)) { return LDG_MATH_ARG_INVALID; }

    max_nnz = 4 * max_edges + max_verts;

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_math_laplace_ctx_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_MATH_ALLOC; }

    ctx = (ldg_math_laplace_ctx_t *)tmp;
    if (LDG_UNLIKELY(memset(ctx, 0, sizeof(ldg_math_laplace_ctx_t)) != ctx))
    {
        ldg_mem_dealloc(ctx);
        return LDG_MATH_ALLOC;
    }

    ctx->max_verts = max_verts;
    ctx->max_edges = max_edges;
    ctx->precond_kind = (uint32_t)precond_kind;

    ret = ldg_math_sparse_create(max_verts, max_verts, max_nnz, &ctx->op);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        laplace_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = ldg_math_krylov_ctx_create(max_verts, max_nnz, precond_kind, &ctx->krylov);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        laplace_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = laplace_alloc_u64(max_verts, &ctx->full_to_free);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        laplace_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = laplace_alloc_u64(max_verts, &ctx->free_to_full);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        laplace_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = laplace_alloc_doubles(max_verts, &ctx->pin_val);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        laplace_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = laplace_alloc_doubles(max_verts, &ctx->rhs);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        laplace_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = laplace_alloc_doubles(max_verts, &ctx->sol);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        laplace_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = laplace_alloc_doubles(max_verts, &ctx->cg_resid);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        laplace_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = laplace_alloc_doubles(max_verts, &ctx->cg_dir);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        laplace_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = laplace_alloc_doubles(max_verts, &ctx->cg_a_dir);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        laplace_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = laplace_alloc_doubles(max_verts, &ctx->full_src);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        laplace_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = laplace_alloc_doubles(max_verts, &ctx->full_pot);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK))
    {
        laplace_ctx_destroy_internal(ctx);
        return ret;
    }

    *out = ctx;

    return LDG_MATH_OK;
}

LDG_EXPORT uint32_t ldg_math_laplace_ctx_destroy(ldg_math_laplace_ctx_t **ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(!*ctx)) { return LDG_MATH_OK; }

    laplace_ctx_destroy_internal(*ctx);
    *ctx = 0x0;

    return LDG_MATH_OK;
}

LDG_EXPORT uint32_t ldg_math_laplace_solve(ldg_math_laplace_ctx_t *ctx, uint64_t vert_cunt, const ldg_math_laplace_edge_t *edge, uint64_t edge_cunt, const ldg_math_laplace_pin_t *pin, uint64_t pin_cunt, const double *src, double gmin, double tol, uint64_t max_iters, double *potential_out, ldg_math_laplace_res_t *res)
{
    ldg_math_krylov_res_t kres = LDG_STRUCT_ZERO_INIT;
    uint64_t free_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !potential_out || !res)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(edge_cunt > 0 && !edge)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(pin_cunt > 0 && !pin)) { return LDG_MATH_ARG_NULL; }

    res->iters = 0;
    res->free_cunt = 0;
    res->rel_resid = 0.0;
    res->converged = 0;

    if (LDG_UNLIKELY(vert_cunt == 0 || vert_cunt > ctx->max_verts)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(edge_cunt > ctx->max_edges)) { return LDG_MATH_BOUNDS; }

    if (LDG_UNLIKELY(gmin < 0.0)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(tol <= 0.0 || max_iters == 0)) { return LDG_MATH_ARG_INVALID; }

    ret = laplace_edges_validate(vert_cunt, edge, edge_cunt);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = laplace_pins_register(ctx, vert_cunt, pin, pin_cunt);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    free_cunt = laplace_free_map_build(ctx, vert_cunt);
    res->free_cunt = free_cunt;

    if (free_cunt == 0)
    {
        laplace_scatter(ctx, vert_cunt, potential_out);
        res->converged = 1;
        return LDG_MATH_OK;
    }

    if (LDG_UNLIKELY(pin_cunt == 0 && gmin <= 0.0)) { return LDG_MATH_ARG_INVALID; }

    ret = laplace_assemble(ctx, free_cunt, edge, edge_cunt, src, gmin);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = ldg_math_vec_zero(ctx->sol, free_cunt);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    if (ctx->precond_kind == LDG_MATH_KRYLOV_PRECOND_NONE) { ret = ldg_math_krylov_cg_solve(ctx->krylov, ctx->op, ctx->rhs, ctx->sol, tol, max_iters, &kres); }
    else
    {
        ret = ldg_math_krylov_precond_build(ctx->krylov, ctx->op);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        ret = ldg_math_krylov_pcg_solve(ctx->krylov, ctx->op, ctx->rhs, ctx->sol, tol, max_iters, &kres);
    }

    res->iters = kres.iters;
    res->rel_resid = kres.rel_resid;
    res->converged = kres.converged;

    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    laplace_scatter(ctx, vert_cunt, potential_out);

    return LDG_MATH_OK;
}

LDG_EXPORT uint32_t ldg_math_laplace_anchor_solve(ldg_math_laplace_ctx_t *ctx, uint64_t vert_cunt, const ldg_math_laplace_edge_t *edge, uint64_t edge_cunt, const ldg_math_laplace_pin_t *pin, uint64_t pin_cunt, const double *src, double anchor_beta, double tol, uint64_t max_iters, double *potential_out, ldg_math_laplace_res_t *res)
{
    uint64_t free_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !potential_out || !res)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(edge_cunt > 0 && !edge)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(pin_cunt > 0 && !pin)) { return LDG_MATH_ARG_NULL; }

    res->iters = 0;
    res->free_cunt = 0;
    res->rel_resid = 0.0;
    res->converged = 0;

    if (LDG_UNLIKELY(vert_cunt == 0 || vert_cunt > ctx->max_verts)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(edge_cunt > ctx->max_edges)) { return LDG_MATH_BOUNDS; }

    if (LDG_UNLIKELY(anchor_beta < 0.0)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(tol <= 0.0 || max_iters == 0)) { return LDG_MATH_ARG_INVALID; }

    ret = laplace_edges_validate(vert_cunt, edge, edge_cunt);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = laplace_pins_register(ctx, vert_cunt, pin, pin_cunt);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    free_cunt = laplace_free_map_build(ctx, vert_cunt);
    res->free_cunt = free_cunt;

    if (free_cunt == 0)
    {
        laplace_scatter(ctx, vert_cunt, potential_out);
        res->converged = 1;
        return LDG_MATH_OK;
    }

    if (LDG_UNLIKELY(pin_cunt == 0 && anchor_beta <= 0.0)) { return LDG_MATH_ARG_INVALID; }

    ret = laplace_assemble(ctx, free_cunt, edge, edge_cunt, src, 0.0);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = ldg_math_vec_zero(ctx->sol, free_cunt);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    ret = laplace_cg_anchor(ctx, free_cunt, anchor_beta / (double)free_cunt, tol, max_iters, res);
    res->free_cunt = free_cunt;
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    laplace_scatter(ctx, vert_cunt, potential_out);

    return LDG_MATH_OK;
}

LDG_EXPORT uint32_t ldg_math_laplace_resistance_get(ldg_math_laplace_ctx_t *ctx, uint64_t vert_cunt, const ldg_math_laplace_edge_t *edge, uint64_t edge_cunt, uint64_t vert_i, uint64_t vert_j, double gmin, double tol, uint64_t max_iters, double *out)
{
    ldg_math_laplace_res_t res = LDG_STRUCT_ZERO_INIT;
    ldg_math_laplace_pin_t pin = LDG_STRUCT_ZERO_INIT;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_MATH_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(edge_cunt > 0 && !edge)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(vert_cunt == 0 || vert_cunt > ctx->max_verts)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(vert_i >= vert_cunt || vert_j >= vert_cunt)) { return LDG_MATH_BOUNDS; }

    if (vert_i == vert_j) { return LDG_MATH_OK; }

    for (idx = 0; idx < vert_cunt; idx++) { ctx->full_src[idx] = 0.0; }
    ctx->full_src[vert_i] = 1.0;

    pin.vert = vert_j;
    pin.potential = 0.0;

    ret = ldg_math_laplace_solve(ctx, vert_cunt, edge, edge_cunt, &pin, 1, ctx->full_src, gmin, tol, max_iters, ctx->full_pot, &res);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    *out = ctx->full_pot[vert_i];

    return LDG_MATH_OK;
}

LDG_EXPORT uint32_t ldg_math_laplace_tree_cunt_get(ldg_math_laplace_ctx_t *ctx, uint64_t vert_cunt, const ldg_math_laplace_edge_t *edge, uint64_t edge_cunt, double *dense_scratch, double *out)
{
    uint64_t dim = 0;
    uint64_t row = 0;
    uint64_t col = 0;
    uint64_t inner = 0;
    uint64_t idx = 0;
    uint64_t vert_a = 0;
    uint64_t vert_b = 0;
    double weight = 0.0;
    double sum = 0.0;
    double pivot = 0.0;
    double det = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_MATH_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(edge_cunt > 0 && !edge)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(vert_cunt == 0 || vert_cunt > ctx->max_verts)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(edge_cunt > ctx->max_edges)) { return LDG_MATH_BOUNDS; }

    ret = laplace_edges_validate(vert_cunt, edge, edge_cunt);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    if (vert_cunt == 1)
    {
        *out = 1.0;
        return LDG_MATH_OK;
    }

    if (LDG_UNLIKELY(!dense_scratch)) { return LDG_MATH_ARG_NULL; }

    dim = vert_cunt - 1;

    for (idx = 0; idx < dim * dim; idx++) { dense_scratch[idx] = 0.0; }

    for (idx = 0; idx < edge_cunt; idx++)
    {
        vert_a = edge[idx].vert_a;
        vert_b = edge[idx].vert_b;
        weight = edge[idx].weight;

        if (vert_a >= 1) { dense_scratch[(vert_a - 1) * dim + (vert_a - 1)] += weight; }

        if (vert_b >= 1) { dense_scratch[(vert_b - 1) * dim + (vert_b - 1)] += weight; }

        if (vert_a >= 1 && vert_b >= 1)
        {
            dense_scratch[(vert_a - 1) * dim + (vert_b - 1)] -= weight;
            dense_scratch[(vert_b - 1) * dim + (vert_a - 1)] -= weight;
        }
    }

    det = 1.0;
    for (row = 0; row < dim; row++)
    {
        for (col = 0; col <= row; col++)
        {
            sum = dense_scratch[row * dim + col];
            for (inner = 0; inner < col; inner++) { sum -= dense_scratch[row * dim + inner] * dense_scratch[col * dim + inner]; }

            if (row == col)
            {
                if (sum <= 0.0)
                {
                    *out = 0.0;
                    return LDG_MATH_OK;
                }

                pivot = __builtin_sqrt(sum);
                dense_scratch[row * dim + row] = pivot;
                det *= pivot;
            }
            else{ dense_scratch[row * dim + col] = sum / dense_scratch[col * dim + col]; }
        }
    }

    *out = det * det;

    return LDG_MATH_OK;
}

LDG_EXPORT uint32_t ldg_math_laplace_kron_reduce(ldg_math_laplace_ctx_t *ctx, uint64_t vert_cunt, const ldg_math_laplace_edge_t *edge, uint64_t edge_cunt, const uint64_t *retain, uint64_t retain_cunt, double tol, uint64_t max_iters, double *reduced_out)
{
    ldg_math_krylov_res_t kres = LDG_STRUCT_ZERO_INIT;
    uint64_t *inner_idx = 0x0;
    uint64_t *retain_idx = 0x0;
    uint64_t inner_cunt = 0;
    uint64_t idx = 0;
    uint64_t col = 0;
    uint64_t vert = 0;
    uint64_t vert_a = 0;
    uint64_t vert_b = 0;
    double weight = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !reduced_out)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(edge_cunt > 0 && !edge)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(retain_cunt > 0 && !retain)) { return LDG_MATH_ARG_NULL; }

    if (LDG_UNLIKELY(vert_cunt == 0 || vert_cunt > ctx->max_verts)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(edge_cunt > ctx->max_edges)) { return LDG_MATH_BOUNDS; }

    if (LDG_UNLIKELY(retain_cunt == 0 || retain_cunt > vert_cunt)) { return LDG_MATH_ARG_INVALID; }

    if (LDG_UNLIKELY(tol <= 0.0 || max_iters == 0)) { return LDG_MATH_ARG_INVALID; }

    ret = laplace_edges_validate(vert_cunt, edge, edge_cunt);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    inner_idx = ctx->full_to_free;
    retain_idx = ctx->free_to_full;

    for (idx = 0; idx < vert_cunt; idx++)
    {
        inner_idx[idx] = UINT64_MAX;
        retain_idx[idx] = UINT64_MAX;
    }

    for (idx = 0; idx < retain_cunt; idx++)
    {
        vert = retain[idx];
        if (LDG_UNLIKELY(vert >= vert_cunt)) { return LDG_MATH_BOUNDS; }

        if (LDG_UNLIKELY(retain_idx[vert] != UINT64_MAX)) { return LDG_MATH_ARG_INVALID; }

        retain_idx[vert] = idx;
    }

    for (vert = 0; vert < vert_cunt; vert++)
    {
        if (retain_idx[vert] != UINT64_MAX) { continue; }

        inner_idx[vert] = inner_cunt;
        inner_cunt++;
    }

    for (idx = 0; idx < retain_cunt * retain_cunt; idx++) { reduced_out[idx] = 0.0; }

    for (idx = 0; idx < edge_cunt; idx++)
    {
        vert_a = edge[idx].vert_a;
        vert_b = edge[idx].vert_b;
        weight = edge[idx].weight;

        if (retain_idx[vert_a] != UINT64_MAX) { reduced_out[retain_idx[vert_a] * retain_cunt + retain_idx[vert_a]] += weight; }

        if (retain_idx[vert_b] != UINT64_MAX) { reduced_out[retain_idx[vert_b] * retain_cunt + retain_idx[vert_b]] += weight; }

        if (retain_idx[vert_a] != UINT64_MAX && retain_idx[vert_b] != UINT64_MAX)
        {
            reduced_out[retain_idx[vert_a] * retain_cunt + retain_idx[vert_b]] -= weight;
            reduced_out[retain_idx[vert_b] * retain_cunt + retain_idx[vert_a]] -= weight;
        }
    }

    if (inner_cunt == 0) { return LDG_MATH_OK; }

    ctx->op->rows = inner_cunt;
    ctx->op->cols = inner_cunt;
    ctx->op->finalized = 0;
    ctx->op->coo_cunt = 0;
    ctx->op->nnz = 0;

    for (idx = 0; idx < edge_cunt; idx++)
    {
        vert_a = edge[idx].vert_a;
        vert_b = edge[idx].vert_b;
        weight = edge[idx].weight;

        if (inner_idx[vert_a] != UINT64_MAX)
        {
            ret = ldg_math_sparse_entry_add(ctx->op, inner_idx[vert_a], inner_idx[vert_a], weight);
            if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }
        }

        if (inner_idx[vert_b] != UINT64_MAX)
        {
            ret = ldg_math_sparse_entry_add(ctx->op, inner_idx[vert_b], inner_idx[vert_b], weight);
            if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }
        }

        if (inner_idx[vert_a] != UINT64_MAX && inner_idx[vert_b] != UINT64_MAX)
        {
            ret = ldg_math_sparse_entry_add(ctx->op, inner_idx[vert_a], inner_idx[vert_b], -weight);
            if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

            ret = ldg_math_sparse_entry_add(ctx->op, inner_idx[vert_b], inner_idx[vert_a], -weight);
            if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }
        }
    }

    ret = ldg_math_sparse_finalize(ctx->op);
    if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

    if (ctx->precond_kind != LDG_MATH_KRYLOV_PRECOND_NONE)
    {
        ret = ldg_math_krylov_precond_build(ctx->krylov, ctx->op);
        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }
    }

    for (col = 0; col < retain_cunt; col++)
    {
        vert = retain[col];

        for (idx = 0; idx < inner_cunt; idx++) { ctx->full_src[idx] = 0.0; }

        for (idx = 0; idx < edge_cunt; idx++)
        {
            vert_a = edge[idx].vert_a;
            vert_b = edge[idx].vert_b;
            weight = edge[idx].weight;

            if (vert_a == vert && inner_idx[vert_b] != UINT64_MAX) { ctx->full_src[inner_idx[vert_b]] -= weight; }

            if (vert_b == vert && inner_idx[vert_a] != UINT64_MAX) { ctx->full_src[inner_idx[vert_a]] -= weight; }
        }

        for (idx = 0; idx < inner_cunt; idx++) { ctx->full_pot[idx] = 0.0; }

        if (ctx->precond_kind != LDG_MATH_KRYLOV_PRECOND_NONE) { ret = ldg_math_krylov_pcg_solve(ctx->krylov, ctx->op, ctx->full_src, ctx->full_pot, tol, max_iters, &kres); }
        else { ret = ldg_math_krylov_cg_solve(ctx->krylov, ctx->op, ctx->full_src, ctx->full_pot, tol, max_iters, &kres); }

        if (LDG_UNLIKELY(ret != LDG_MATH_OK)) { return ret; }

        for (idx = 0; idx < edge_cunt; idx++)
        {
            vert_a = edge[idx].vert_a;
            vert_b = edge[idx].vert_b;
            weight = edge[idx].weight;

            if (retain_idx[vert_a] != UINT64_MAX && inner_idx[vert_b] != UINT64_MAX) { reduced_out[retain_idx[vert_a] * retain_cunt + col] += weight * ctx->full_pot[inner_idx[vert_b]]; }

            if (retain_idx[vert_b] != UINT64_MAX && inner_idx[vert_a] != UINT64_MAX) { reduced_out[retain_idx[vert_b] * retain_cunt + col] += weight * ctx->full_pot[inner_idx[vert_a]]; }
        }
    }

    return LDG_MATH_OK;
}
