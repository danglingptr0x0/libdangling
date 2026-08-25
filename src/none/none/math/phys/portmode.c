
#include <dangling/math/phys.h>
#include <dangling/math/math.h>
#include <dangling/math/complex.h>
#include <dangling/math/eigen.h>
#include <dangling/math/cholesky.h>
#include <dangling/math/lu.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

#include "portmode.h"

#define LDG_MATH_PHYS_PORTMODE_IM_TOL 1e-6

typedef struct portmode_mesh
{
    double *bary_grad;
    double *tri_area;
    uint64_t *tri_node;
    uint64_t *tri_edge;
    uint64_t *edge_dof;
    uint64_t *node_dof;
    uint64_t *edge_use;
    uint8_t *tri_in;
    uint64_t grid_a;
    uint64_t grid_b;
    uint64_t edge_cunt;
    uint64_t node_cunt;
    uint64_t tri_cunt;
    uint64_t dof_edge_cunt;
    uint64_t dof_cunt;
    double spacing;
} portmode_mesh_t;

LDG_NOINLINE static void portmode_mesh_fill(portmode_mesh_t *mesh, const uint8_t *pec)
{
    double spacing = mesh->spacing;
    double base_x = 0.0;
    double base_y = 0.0;
    double vert_x[3];
    double vert_y[3];
    double area_sig = 0.0;
    uint64_t ga = mesh->grid_a;
    uint64_t gb = mesh->grid_b;
    uint64_t na = ga + 1;
    uint64_t cell_a = 0;
    uint64_t cell_b = 0;
    uint64_t tri = 0;
    uint64_t half = 0;
    uint64_t corner = 0;

    for (cell_b = 0; cell_b < gb; cell_b++)
    {
        for (cell_a = 0; cell_a < ga; cell_a++)
        {
            base_x = (double)cell_a * spacing;
            base_y = (double)cell_b * spacing;
            for (half = 0; half < 2; half++)
            {
                tri = (cell_b * ga + cell_a) * 2 + half;
                mesh->tri_in[tri] = (pec[cell_b * ga + cell_a] == 0) ? 1 : 0;
                if (half == 0)
                {
                    mesh->tri_node[tri * 3 + 0] = cell_b * na + cell_a;
                    mesh->tri_node[tri * 3 + 1] = cell_b * na + cell_a + 1;
                    mesh->tri_node[tri * 3 + 2] = (cell_b + 1) * na + cell_a;
                    mesh->tri_edge[tri * 6 + 0] = cell_b * ga + cell_a;
                    mesh->tri_edge[tri * 6 + 1] = 1;
                    mesh->tri_edge[tri * 6 + 2] = ga * (gb + 1) + na * gb + cell_b * ga + cell_a;
                    mesh->tri_edge[tri * 6 + 3] = 1 * 3 + 2;
                    mesh->tri_edge[tri * 6 + 4] = ga * (gb + 1) + cell_b * na + cell_a;
                    mesh->tri_edge[tri * 6 + 5] = 0 * 3 + 2;
                    vert_x[0] = base_x;
                    vert_y[0] = base_y;
                    vert_x[1] = base_x + spacing;
                    vert_y[1] = base_y;
                    vert_x[2] = base_x;
                    vert_y[2] = base_y + spacing;
                }
                else
                {
                    mesh->tri_node[tri * 3 + 0] = cell_b * na + cell_a + 1;
                    mesh->tri_node[tri * 3 + 1] = (cell_b + 1) * na + cell_a + 1;
                    mesh->tri_node[tri * 3 + 2] = (cell_b + 1) * na + cell_a;
                    mesh->tri_edge[tri * 6 + 0] = ga * (gb + 1) + cell_b * na + cell_a + 1;
                    mesh->tri_edge[tri * 6 + 1] = 1;
                    mesh->tri_edge[tri * 6 + 2] = (cell_b + 1) * ga + cell_a;
                    mesh->tri_edge[tri * 6 + 3] = 2 * 3 + 1;
                    mesh->tri_edge[tri * 6 + 4] = ga * (gb + 1) + na * gb + cell_b * ga + cell_a;
                    mesh->tri_edge[tri * 6 + 5] = 0 * 3 + 2;
                    vert_x[0] = base_x + spacing;
                    vert_y[0] = base_y;
                    vert_x[1] = base_x + spacing;
                    vert_y[1] = base_y + spacing;
                    vert_x[2] = base_x;
                    vert_y[2] = base_y + spacing;
                }

                area_sig = 0.5 * ((vert_x[1] - vert_x[0]) * (vert_y[2] - vert_y[0]) - (vert_x[2] - vert_x[0]) * (vert_y[1] - vert_y[0]));
                mesh->tri_area[tri] = ldg_math_abs_get(area_sig);
                for (corner = 0; corner < 3; corner++)
                {
                    mesh->bary_grad[tri * 6 + corner * 2 + 0] = (vert_y[(corner + 1) % 3] - vert_y[(corner + 2) % 3]) / (2.0 * area_sig);
                    mesh->bary_grad[tri * 6 + corner * 2 + 1] = (vert_x[(corner + 2) % 3] - vert_x[(corner + 1) % 3]) / (2.0 * area_sig);
                }
            }
        }
    }
}

LDG_NOINLINE static void portmode_dof_build(portmode_mesh_t *mesh)
{
    uint64_t tri = 0;
    uint64_t side = 0;
    uint64_t edge = 0;
    uint64_t node = 0;
    uint64_t local_pair = 0;
    uint64_t dof = 0;

    for (edge = 0; edge < mesh->edge_cunt; edge++)
    {
        mesh->edge_use[edge] = 0;
        mesh->edge_dof[edge] = UINT64_MAX;
    }
    for (node = 0; node < mesh->node_cunt; node++) { mesh->node_dof[node] = UINT64_MAX; }
    for (tri = 0; tri < mesh->tri_cunt; tri++)
    {
        if (!mesh->tri_in[tri]) { continue; }

        for (side = 0; side < 3; side++) { mesh->edge_use[mesh->tri_edge[tri * 6 + side * 2]] += 1; }
    }
    for (tri = 0; tri < mesh->tri_cunt; tri++)
    {
        if (!mesh->tri_in[tri]) { continue; }

        for (side = 0; side < 3; side++)
        {
            edge = mesh->tri_edge[tri * 6 + side * 2];
            if (mesh->edge_use[edge] == 2) { mesh->edge_dof[edge] = 0; }
        }
    }
    for (tri = 0; tri < mesh->tri_cunt; tri++)
    {
        if (!mesh->tri_in[tri]) { continue; }

        for (side = 0; side < 3; side++)
        {
            edge = mesh->tri_edge[tri * 6 + side * 2];
            local_pair = mesh->tri_edge[tri * 6 + side * 2 + 1];
            if (mesh->edge_use[edge] == 2) { continue; }

            mesh->node_dof[mesh->tri_node[tri * 3 + local_pair / 3]] = UINT64_MAX - 1;
            mesh->node_dof[mesh->tri_node[tri * 3 + local_pair % 3]] = UINT64_MAX - 1;
        }
    }
    dof = 0;
    for (edge = 0; edge < mesh->edge_cunt; edge++) { if (mesh->edge_dof[edge] == 0)
        {
            mesh->edge_dof[edge] = dof;
            dof++;
        }
        else { mesh->edge_dof[edge] = UINT64_MAX; } }
    mesh->dof_edge_cunt = dof;
    for (tri = 0; tri < mesh->tri_cunt; tri++)
    {
        if (!mesh->tri_in[tri]) { continue; }

        for (side = 0; side < 3; side++)
        {
            node = mesh->tri_node[tri * 3 + side];
            if (mesh->node_dof[node] == UINT64_MAX) { mesh->node_dof[node] = UINT64_MAX - 2; }
        }
    }
    for (node = 0; node < mesh->node_cunt; node++) { if (mesh->node_dof[node] == UINT64_MAX - 2)
        {
            mesh->node_dof[node] = dof;
            dof++;
        }
        else { mesh->node_dof[node] = UINT64_MAX; } }
    mesh->dof_cunt = dof;
}

LDG_NOINLINE static void portmode_blk_assemble(const portmode_mesh_t *mesh, const double *eps_r, const double *mu_r, double k0, double *a_ee, double *t_mu, double *g_mu, double *g_eps, double *tz_eps)
{
    const double *grad = 0x0;
    double area = 0.0;
    double eps_val = 0.0;
    double inv_mu = 0.0;
    double curl_val[3];
    double dot_gg = 0.0;
    double mass_val = 0.0;
    double couple_val = 0.0;
    uint64_t n_edge = mesh->dof_edge_cunt;
    uint64_t n_node = mesh->dof_cunt - mesh->dof_edge_cunt;
    uint64_t tri = 0;
    uint64_t cell = 0;
    uint64_t side_a = 0;
    uint64_t side_b = 0;
    uint64_t pair_a = 0;
    uint64_t pair_b = 0;
    uint64_t dof_a = 0;
    uint64_t dof_b = 0;
    uint64_t p_a = 0;
    uint64_t q_a = 0;
    uint64_t p_b = 0;
    uint64_t q_b = 0;

    for (tri = 0; tri < mesh->tri_cunt; tri++)
    {
        if (!mesh->tri_in[tri]) { continue; }

        cell = tri / 2;
        area = mesh->tri_area[tri];
        eps_val = eps_r[cell];
        inv_mu = 1.0 / mu_r[cell];
        grad = &mesh->bary_grad[tri * 6];
        for (side_a = 0; side_a < 3; side_a++)
        {
            pair_a = mesh->tri_edge[tri * 6 + side_a * 2 + 1];
            p_a = pair_a / 3;
            q_a = pair_a % 3;
            curl_val[side_a] = 2.0 * (grad[p_a * 2 + 0] * grad[q_a * 2 + 1] - grad[p_a * 2 + 1] * grad[q_a * 2 + 0]);
        }
        for (side_a = 0; side_a < 3; side_a++)
        {
            dof_a = mesh->edge_dof[mesh->tri_edge[tri * 6 + side_a * 2]];
            if (dof_a == UINT64_MAX) { continue; }

            pair_a = mesh->tri_edge[tri * 6 + side_a * 2 + 1];
            p_a = pair_a / 3;
            q_a = pair_a % 3;
            for (side_b = 0; side_b < 3; side_b++)
            {
                dof_b = mesh->edge_dof[mesh->tri_edge[tri * 6 + side_b * 2]];
                if (dof_b == UINT64_MAX) { continue; }

                pair_b = mesh->tri_edge[tri * 6 + side_b * 2 + 1];
                p_b = pair_b / 3;
                q_b = pair_b % 3;
                mass_val = 0.0;
                dot_gg = grad[q_a * 2 + 0] * grad[q_b * 2 + 0] + grad[q_a * 2 + 1] * grad[q_b * 2 + 1];
                mass_val += dot_gg * area / 12.0 * ((p_a == p_b) ? 2.0 : 1.0);
                dot_gg = grad[q_a * 2 + 0] * grad[p_b * 2 + 0] + grad[q_a * 2 + 1] * grad[p_b * 2 + 1];
                mass_val -= dot_gg * area / 12.0 * ((p_a == q_b) ? 2.0 : 1.0);
                dot_gg = grad[p_a * 2 + 0] * grad[q_b * 2 + 0] + grad[p_a * 2 + 1] * grad[q_b * 2 + 1];
                mass_val -= dot_gg * area / 12.0 * ((q_a == p_b) ? 2.0 : 1.0);
                dot_gg = grad[p_a * 2 + 0] * grad[p_b * 2 + 0] + grad[p_a * 2 + 1] * grad[p_b * 2 + 1];
                mass_val += dot_gg * area / 12.0 * ((q_a == q_b) ? 2.0 : 1.0);
                a_ee[dof_a * n_edge + dof_b] += inv_mu * curl_val[side_a] * curl_val[side_b] * area / (k0 * k0) - eps_val * mass_val;
                t_mu[dof_a * n_edge + dof_b] += inv_mu * mass_val / (k0 * k0);
            }
            for (side_b = 0; side_b < 3; side_b++)
            {
                dof_b = mesh->node_dof[mesh->tri_node[tri * 3 + side_b]];
                if (dof_b == UINT64_MAX) { continue; }

                couple_val = (grad[q_a * 2 + 0] * grad[side_b * 2 + 0] + grad[q_a * 2 + 1] * grad[side_b * 2 + 1]) - (grad[p_a * 2 + 0] * grad[side_b * 2 + 0] + grad[p_a * 2 + 1] * grad[side_b * 2 + 1]);
                couple_val *= area / 3.0;
                g_mu[dof_a * n_node + (dof_b - n_edge)] += inv_mu * couple_val;
                g_eps[dof_a * n_node + (dof_b - n_edge)] += eps_val * couple_val;
            }
        }
        for (side_a = 0; side_a < 3; side_a++)
        {
            dof_a = mesh->node_dof[mesh->tri_node[tri * 3 + side_a]];
            if (dof_a == UINT64_MAX) { continue; }

            for (side_b = 0; side_b < 3; side_b++)
            {
                dof_b = mesh->node_dof[mesh->tri_node[tri * 3 + side_b]];
                if (dof_b == UINT64_MAX) { continue; }

                tz_eps[(dof_a - n_edge) * n_node + (dof_b - n_edge)] += eps_val * area / 12.0 * ((side_a == side_b) ? 2.0 : 1.0);
            }
        }
    }
}

LDG_NOINLINE static uint32_t portmode_spd_solve_cols(ldg_math_cholesky_ctx_t *chol, ldg_math_complex_t *stage, const double *mat_spd, const double *rhs_cols, uint64_t dim, uint64_t col_cunt, double *out_cols)
{
    uint64_t row = 0;
    uint64_t col = 0;
    uint32_t err = 0;

    for (row = 0; row < dim; row++) { for (col = 0; col < dim; col++)
        {
            stage[row * dim + col].re = mat_spd[row * dim + col];
            stage[row * dim + col].im = 0.0;
        }
    }
    err = ldg_math_cholesky_factor_compute(chol, stage);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (col = 0; col < col_cunt; col++)
    {
        for (row = 0; row < dim; row++)
        {
            stage[row].re = rhs_cols[row * col_cunt + col];
            stage[row].im = 0.0;
        }
        err = ldg_math_cholesky_solve_compute(chol, stage, 1, &stage[dim]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        for (row = 0; row < dim; row++) { out_cols[row * col_cunt + col] = stage[dim + row].re; }
    }
    return LDG_ERR_AOK;
}

LDG_NOINLINE static void portmode_field_sample(const portmode_mesh_t *mesh, const double *mu_r, const double *e_dof, const double *z_dof, double beta, double k0, double *out_e, double *out_h)
{
    const double *grad = 0x0;
    double et_a = 0.0;
    double et_b = 0.0;
    double gz_a = 0.0;
    double gz_b = 0.0;
    double bary[3];
    double coef = 0.0;
    double cross_sum = 0.0;
    double inv_mu = 0.0;
    uint64_t face_cunt = mesh->grid_a * mesh->grid_b;
    uint64_t tri = 0;
    uint64_t cell = 0;
    uint64_t half = 0;
    uint64_t side = 0;
    uint64_t pair = 0;
    uint64_t dof = 0;
    uint64_t idx = 0;

    for (idx = 0; idx < 2 * face_cunt; idx++)
    {
        out_e[idx] = 0.0;
        out_h[idx] = 0.0;
    }
    for (cell = 0; cell < face_cunt; cell++)
    {
        if (!mesh->tri_in[cell * 2]) { continue; }

        inv_mu = 1.0 / mu_r[cell];
        et_a = 0.0;
        et_b = 0.0;
        gz_a = 0.0;
        gz_b = 0.0;
        for (half = 0; half < 2; half++)
        {
            tri = cell * 2 + half;
            grad = &mesh->bary_grad[tri * 6];
            bary[0] = (half == 0) ? 0.0 : 0.5;
            bary[1] = (half == 0) ? 0.5 : 0.0;
            bary[2] = 0.5;
            for (side = 0; side < 3; side++)
            {
                dof = mesh->edge_dof[mesh->tri_edge[tri * 6 + side * 2]];
                if (dof == UINT64_MAX) { continue; }

                pair = mesh->tri_edge[tri * 6 + side * 2 + 1];
                coef = e_dof[dof];
                et_a += coef * (bary[pair / 3] * grad[(pair % 3) * 2 + 0] - bary[pair % 3] * grad[(pair / 3) * 2 + 0]) * 0.5;
                et_b += coef * (bary[pair / 3] * grad[(pair % 3) * 2 + 1] - bary[pair % 3] * grad[(pair / 3) * 2 + 1]) * 0.5;
            }
            for (side = 0; side < 3; side++)
            {
                dof = mesh->node_dof[mesh->tri_node[tri * 3 + side]];
                if (dof == UINT64_MAX) { continue; }

                gz_a += z_dof[dof - mesh->dof_edge_cunt] * grad[side * 2 + 0] * 0.5;
                gz_b += z_dof[dof - mesh->dof_edge_cunt] * grad[side * 2 + 1] * 0.5;
            }
        }
        out_e[cell] = et_a;
        out_e[face_cunt + cell] = et_b;
        out_h[cell] = (beta / (k0 * mu_r[cell])) * (0.0 - et_b) - inv_mu / k0 * gz_b;
        out_h[face_cunt + cell] = (beta / (k0 * mu_r[cell])) * et_a + inv_mu / k0 * gz_a;
    }
    cross_sum = 0.0;
    for (cell = 0; cell < face_cunt; cell++) { cross_sum += out_e[cell] * out_h[face_cunt + cell] - out_e[face_cunt + cell] * out_h[cell]; }
    if (cross_sum < 0.0) { for (idx = 0; idx < 2 * face_cunt; idx++) { out_h[idx] = 0.0 - out_h[idx]; } }
}

uint32_t ldg_math_phys_em_portmode_solve(const double *eps_r, const double *mu_r, const uint8_t *pec, uint64_t grid_a, uint64_t grid_b, double spacing, double k0, uint32_t mode_idx, double *out_e, double *out_h, double *out_beta)
{
    portmode_mesh_t mesh = LDG_STRUCT_ZERO_INIT;
    ldg_math_eigen_schur_ctx_t *schur = 0x0;
    ldg_math_cholesky_ctx_t *chol_z = 0x0;
    ldg_math_cholesky_ctx_t *chol_e = 0x0;
    ldg_math_lu_ctx_t *lu = 0x0;
    ldg_math_complex_t *stage = 0x0;
    ldg_math_complex_t *lam = 0x0;
    ldg_math_complex_t *vec_c = 0x0;
    ldg_math_complex_t rot = LDG_STRUCT_ZERO_INIT;
    double *a_ee = 0x0;
    double *t_mu = 0x0;
    double *g_mu = 0x0;
    double *g_eps = 0x0;
    double *tz_eps = 0x0;
    double *wz_map = 0x0;
    double *m_std = 0x0;
    double *e_dof = 0x0;
    double *z_dof = 0x0;
    void *blk = 0x0;
    double beta_sq_norm = 0.0;
    double best_mag = 0.0;
    double lam_scale = 0.0;
    uint64_t n_edge = 0;
    uint64_t n_node = 0;
    uint64_t face_cunt = 0;
    uint64_t row = 0;
    uint64_t col = 0;
    uint64_t mid = 0;
    uint64_t idx = 0;
    uint64_t pick = 0;
    uint64_t rank = 0;
    uint64_t best_idx = 0;
    uint32_t err = 0;
    uint32_t ret = 0;
    if (LDG_UNLIKELY(!eps_r || !mu_r || !pec || !out_e || !out_h || !out_beta)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_beta = 0.0;
    if (LDG_UNLIKELY(grid_a < 2 || grid_b < 2 || !(spacing > 0.0) || !(k0 > 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    face_cunt = grid_a * grid_b;
    mesh.grid_a = grid_a;
    mesh.grid_b = grid_b;
    mesh.spacing = spacing;
    mesh.edge_cunt = grid_a * (grid_b + 1) + (grid_a + 1) * grid_b + face_cunt;
    mesh.node_cunt = (grid_a + 1) * (grid_b + 1);
    mesh.tri_cunt = face_cunt * 2;
    err = ldg_mem_alloc(mesh.tri_cunt * 6 * sizeof(double), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    mesh.bary_grad = (double *)blk;
    err = ldg_mem_alloc(mesh.tri_cunt * sizeof(double), &blk);
    if (err == LDG_ERR_AOK) { mesh.tri_area = (double *)blk; }

    if (err == LDG_ERR_AOK) { err = ldg_mem_alloc(mesh.tri_cunt * 3 * sizeof(uint64_t), &blk); }

    if (err == LDG_ERR_AOK)
    {
        mesh.tri_node = (uint64_t *)blk;
        err = ldg_mem_alloc(mesh.tri_cunt * 6 * sizeof(uint64_t), &blk);
    }

    if (err == LDG_ERR_AOK)
    {
        mesh.tri_edge = (uint64_t *)blk;
        err = ldg_mem_alloc(mesh.edge_cunt * sizeof(uint64_t), &blk);
    }

    if (err == LDG_ERR_AOK)
    {
        mesh.edge_dof = (uint64_t *)blk;
        err = ldg_mem_alloc(mesh.node_cunt * sizeof(uint64_t), &blk);
    }

    if (err == LDG_ERR_AOK)
    {
        mesh.node_dof = (uint64_t *)blk;
        err = ldg_mem_alloc(mesh.edge_cunt * sizeof(uint64_t), &blk);
    }

    if (err == LDG_ERR_AOK)
    {
        mesh.edge_use = (uint64_t *)blk;
        err = ldg_mem_alloc(mesh.tri_cunt * sizeof(uint8_t), &blk);
    }

    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ret = LDG_ERR_ALLOC_NULL; }
    else
    {
        mesh.tri_in = (uint8_t *)blk;
        portmode_mesh_fill(&mesh, pec);
        portmode_dof_build(&mesh);
        n_edge = mesh.dof_edge_cunt;
        n_node = mesh.dof_cunt - n_edge;
        if (n_edge < 2 || n_node < 1) { ret = LDG_ERR_EMPTY; }
        else
        {
            err = ldg_mem_alloc(n_edge * n_edge * sizeof(double), &blk);
            if (err == LDG_ERR_AOK)
            {
                a_ee = (double *)blk;
                err = ldg_mem_alloc(n_edge * n_edge * sizeof(double), &blk);
            }

            if (err == LDG_ERR_AOK)
            {
                t_mu = (double *)blk;
                err = ldg_mem_alloc(n_edge * n_node * sizeof(double), &blk);
            }

            if (err == LDG_ERR_AOK)
            {
                g_mu = (double *)blk;
                err = ldg_mem_alloc(n_edge * n_node * sizeof(double), &blk);
            }

            if (err == LDG_ERR_AOK)
            {
                g_eps = (double *)blk;
                err = ldg_mem_alloc(n_node * n_node * sizeof(double), &blk);
            }

            if (err == LDG_ERR_AOK)
            {
                tz_eps = (double *)blk;
                err = ldg_mem_alloc(n_node * n_edge * sizeof(double), &blk);
            }

            if (err == LDG_ERR_AOK)
            {
                wz_map = (double *)blk;
                err = ldg_mem_alloc(n_edge * n_edge * sizeof(double), &blk);
            }

            if (err == LDG_ERR_AOK)
            {
                m_std = (double *)blk;
                err = ldg_mem_alloc((n_edge * n_edge + 2 * n_edge) * sizeof(ldg_math_complex_t), &blk);
            }

            if (err == LDG_ERR_AOK)
            {
                stage = (ldg_math_complex_t *)blk;
                err = ldg_mem_alloc(n_edge * sizeof(ldg_math_complex_t), &blk);
            }

            if (err == LDG_ERR_AOK)
            {
                lam = (ldg_math_complex_t *)blk;
                err = ldg_mem_alloc(n_edge * sizeof(ldg_math_complex_t), &blk);
            }

            if (err == LDG_ERR_AOK)
            {
                vec_c = (ldg_math_complex_t *)blk;
                err = ldg_mem_alloc(n_edge * sizeof(double), &blk);
            }

            if (err == LDG_ERR_AOK)
            {
                e_dof = (double *)blk;
                err = ldg_mem_alloc(n_node * sizeof(double), &blk);
            }

            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ret = LDG_ERR_ALLOC_NULL; }
            else
            {
                z_dof = (double *)blk;
                ldg_mem_zero(a_ee, n_edge * n_edge * sizeof(double));
                ldg_mem_zero(t_mu, n_edge * n_edge * sizeof(double));
                ldg_mem_zero(g_mu, n_edge * n_node * sizeof(double));
                ldg_mem_zero(g_eps, n_edge * n_node * sizeof(double));
                ldg_mem_zero(tz_eps, n_node * n_node * sizeof(double));
                portmode_blk_assemble(&mesh, eps_r, mu_r, k0, a_ee, t_mu, g_mu, g_eps, tz_eps);
                err = ldg_math_cholesky_ctx_create(n_node, &chol_z);
                if (err == LDG_ERR_AOK) { err = ldg_math_cholesky_ctx_create(n_edge, &chol_e); }

                if (err == LDG_ERR_AOK) { err = ldg_math_eigen_schur_ctx_create(n_edge, &schur); }

                if (err == LDG_ERR_AOK) { err = ldg_math_lu_ctx_create(n_edge, &lu); }

                if (err == LDG_ERR_AOK)
                {
                    for (row = 0; row < n_node; row++) { for (col = 0; col < n_edge; col++) { wz_map[row * n_edge + col] = g_eps[col * n_node + row]; } }
                    err = portmode_spd_solve_cols(chol_z, stage, tz_eps, wz_map, n_node, n_edge, wz_map);
                }

                if (err == LDG_ERR_AOK)
                {
                    for (row = 0; row < n_node * n_edge; row++) { wz_map[row] /= k0 * k0; }
                    for (row = 0; row < n_edge; row++) { for (col = 0; col < n_edge; col++)
                        {
                            m_std[row * n_edge + col] = a_ee[row * n_edge + col];
                            for (mid = 0; mid < n_node; mid++) { m_std[row * n_edge + col] += g_mu[row * n_node + mid] * wz_map[mid * n_edge + col]; }
                        }
                    }
                    err = portmode_spd_solve_cols(chol_e, stage, t_mu, m_std, n_edge, n_edge, m_std);
                }

                if (err == LDG_ERR_AOK)
                {
                    for (row = 0; row < n_edge * n_edge; row++) { m_std[row] = 0.0 - m_std[row]; }
                    err = ldg_math_eigen_schur_compute(schur, m_std, lam);
                }

                if (err != LDG_ERR_AOK) { ret = err; }
                else
                {
                    lam_scale = 0.0;
                    for (idx = 0; idx < n_edge; idx++) { if (ldg_math_abs_get(lam[idx].re) > lam_scale) { lam_scale = ldg_math_abs_get(lam[idx].re); } }
                    pick = UINT64_MAX;
                    rank = 0;
                    for (idx = 0; idx < n_edge; idx++)
                    {
                        row = n_edge - 1 - idx;
                        if (lam[row].re > 0.0 && ldg_math_abs_get(lam[row].im) <= LDG_MATH_PHYS_PORTMODE_IM_TOL * lam_scale)
                        {
                            if (rank == (uint64_t)mode_idx)
                            {
                                pick = row;
                                break;
                            }

                            rank++;
                        }
                    }
                    if (pick == UINT64_MAX) { ret = LDG_ERR_NOT_FOUND; }
                    else
                    {
                        beta_sq_norm = lam[pick].re;
                        *out_beta = ldg_math_sqrt_get(beta_sq_norm);
                        err = ldg_math_eigen_schur_vec_compute(schur, lu, pick, vec_c);
                        if (err != LDG_ERR_AOK) { ret = err; }
                        else
                        {
                            best_mag = 0.0;
                            best_idx = 0;
                            for (row = 0; row < n_edge; row++) { if (vec_c[row].re * vec_c[row].re + vec_c[row].im * vec_c[row].im > best_mag)
                                {
                                    best_mag = vec_c[row].re * vec_c[row].re + vec_c[row].im * vec_c[row].im;
                                    best_idx = row;
                                }
                            }
                            best_mag = ldg_math_sqrt_get(best_mag);
                            rot.re = vec_c[best_idx].re / best_mag;
                            rot.im = vec_c[best_idx].im / best_mag;
                            for (row = 0; row < n_edge; row++) { e_dof[row] = vec_c[row].re * rot.re + vec_c[row].im * rot.im; }
                            for (row = 0; row < n_node; row++)
                            {
                                z_dof[row] = 0.0;
                                for (col = 0; col < n_edge; col++) { z_dof[row] += wz_map[row * n_edge + col] * e_dof[col]; }
                            }
                            portmode_field_sample(&mesh, mu_r, e_dof, z_dof, *out_beta, k0, out_e, out_h);
                        }
                    }
                }
            }
        }
    }

    if (lu != 0x0) { err = ldg_math_lu_ctx_destroy(&lu); }

    if (schur != 0x0) { err = ldg_math_eigen_schur_ctx_destroy(&schur); }

    if (chol_e != 0x0) { err = ldg_math_cholesky_ctx_destroy(&chol_e); }

    if (chol_z != 0x0) { err = ldg_math_cholesky_ctx_destroy(&chol_z); }

    if (z_dof != 0x0) { err = ldg_mem_dealloc(z_dof); }

    if (e_dof != 0x0) { err = ldg_mem_dealloc(e_dof); }

    if (vec_c != 0x0) { err = ldg_mem_dealloc(vec_c); }

    if (lam != 0x0) { err = ldg_mem_dealloc(lam); }

    if (stage != 0x0) { err = ldg_mem_dealloc(stage); }

    if (m_std != 0x0) { err = ldg_mem_dealloc(m_std); }

    if (wz_map != 0x0) { err = ldg_mem_dealloc(wz_map); }

    if (tz_eps != 0x0) { err = ldg_mem_dealloc(tz_eps); }

    if (g_eps != 0x0) { err = ldg_mem_dealloc(g_eps); }

    if (g_mu != 0x0) { err = ldg_mem_dealloc(g_mu); }

    if (t_mu != 0x0) { err = ldg_mem_dealloc(t_mu); }

    if (a_ee != 0x0) { err = ldg_mem_dealloc(a_ee); }

    if (mesh.tri_in != 0x0) { err = ldg_mem_dealloc(mesh.tri_in); }

    if (mesh.edge_use != 0x0) { err = ldg_mem_dealloc(mesh.edge_use); }

    if (mesh.node_dof != 0x0) { err = ldg_mem_dealloc(mesh.node_dof); }

    if (mesh.edge_dof != 0x0) { err = ldg_mem_dealloc(mesh.edge_dof); }

    if (mesh.tri_edge != 0x0) { err = ldg_mem_dealloc(mesh.tri_edge); }

    if (mesh.tri_node != 0x0) { err = ldg_mem_dealloc(mesh.tri_node); }

    if (mesh.tri_area != 0x0) { err = ldg_mem_dealloc(mesh.tri_area); }

    err = ldg_mem_dealloc(mesh.bary_grad);
    return ret;
}
