
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>

#include "magnetostatic.h"
#include "field.h"

static double ldg_math_phys_em_magnetostatic_reluctivity_get(const uint16_t *mtl, const double *coeff, uint64_t cell)
{
    double mu_val = coeff[(uint64_t)mtl[cell] * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_MU];
    if (mu_val <= 0.0) { return 0.0; }

    return 1.0 / mu_val;
}

static double ldg_math_phys_em_magnetostatic_face_reluctivity_get(const uint16_t *mtl, const double *coeff, uint64_t cell_a, uint64_t cell_b)
{
    double nu_a = ldg_math_phys_em_magnetostatic_reluctivity_get(mtl, coeff, cell_a);
    double nu_b = ldg_math_phys_em_magnetostatic_reluctivity_get(mtl, coeff, cell_b);
    if (nu_a + nu_b <= 0.0) { return 0.0; }

    return 2.0 * nu_a * nu_b / (nu_a + nu_b);
}

static void ldg_math_phys_em_magnetostatic_op_apply(double *out_vec, const double *in_vec, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny)
{
    double acc = 0.0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    for (idx_y = 0; idx_y < ny; idx_y++)
    {
        for (idx_x = 0; idx_x < nx; idx_x++)
        {
            center = idx_y * nx + idx_x;
            acc = 0.0;
            if (idx_x > 0) { acc += ldg_math_phys_em_magnetostatic_face_reluctivity_get(mtl, coeff, center, center - 1) * (in_vec[center] - in_vec[center - 1]); }
            else { acc += ldg_math_phys_em_magnetostatic_reluctivity_get(mtl, coeff, center) * in_vec[center]; }

            if (idx_x + 1 < nx) { acc += ldg_math_phys_em_magnetostatic_face_reluctivity_get(mtl, coeff, center, center + 1) * (in_vec[center] - in_vec[center + 1]); }
            else { acc += ldg_math_phys_em_magnetostatic_reluctivity_get(mtl, coeff, center) * in_vec[center]; }

            if (idx_y > 0) { acc += ldg_math_phys_em_magnetostatic_face_reluctivity_get(mtl, coeff, center, center - nx) * (in_vec[center] - in_vec[center - nx]); }
            else { acc += ldg_math_phys_em_magnetostatic_reluctivity_get(mtl, coeff, center) * in_vec[center]; }

            if (idx_y + 1 < ny) { acc += ldg_math_phys_em_magnetostatic_face_reluctivity_get(mtl, coeff, center, center + nx) * (in_vec[center] - in_vec[center + nx]); }
            else { acc += ldg_math_phys_em_magnetostatic_reluctivity_get(mtl, coeff, center) * in_vec[center]; }

            out_vec[center] = acc;
        }
    }
}

uint32_t ldg_math_phys_em_magnetostatic_solve(double *potential, double *resid, double *dir, double *a_dir, const uint16_t *conductor, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, double spacing, uint32_t excited, double tol, uint64_t iter_max)
{
    double resid_dot = 0.0;
    double resid_dot_next = 0.0;
    double resid_dot_first = 0.0;
    double dir_dot = 0.0;
    double alpha = 0.0;
    double beta = 0.0;
    double src_val = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t excited_cunt = 0;
    uint64_t center = 0;
    uint64_t iter = 0;
    if (LDG_UNLIKELY(!potential)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!resid)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!dir)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!a_dir)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!conductor)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 3 || ny < 3)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(tol) || tol <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(iter_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny;
    for (center = 0; center < cell_cunt; center++) { if (conductor[center] == excited) { excited_cunt++; } }
    if (LDG_UNLIKELY(excited_cunt == 0)) { return LDG_ERR_EMPTY; }

    src_val = 1.0 / (double)excited_cunt;
    for (center = 0; center < cell_cunt; center++)
    {
        potential[center] = 0.0;
        resid[center] = (conductor[center] == excited) ? src_val : 0.0;
        dir[center] = resid[center];
        a_dir[center] = 0.0;
    }
    resid_dot = 0.0;
    for (center = 0; center < cell_cunt; center++) { resid_dot += resid[center] * resid[center]; }
    resid_dot_first = resid_dot;
    if (!(resid_dot_first > 0.0)) { return LDG_ERR_EMPTY; }

    for (iter = 0; iter < iter_max; iter++)
    {
        ldg_math_phys_em_magnetostatic_op_apply(a_dir, dir, mtl, coeff, nx, ny);
        dir_dot = 0.0;
        for (center = 0; center < cell_cunt; center++) { dir_dot += dir[center] * a_dir[center]; }
        if (!(dir_dot > 0.0)) { return LDG_ERR_AOK; }

        alpha = resid_dot / dir_dot;
        for (center = 0; center < cell_cunt; center++)
        {
            potential[center] += alpha * dir[center];
            resid[center] -= alpha * a_dir[center];
        }
        resid_dot_next = 0.0;
        for (center = 0; center < cell_cunt; center++) { resid_dot_next += resid[center] * resid[center]; }
        if (ldg_math_sqrt_get(resid_dot_next) <= tol * ldg_math_sqrt_get(resid_dot_first)) { return LDG_ERR_AOK; }

        beta = resid_dot_next / resid_dot;
        for (center = 0; center < cell_cunt; center++) { dir[center] = resid[center] + beta * dir[center]; }
        resid_dot = resid_dot_next;
    }
    return LDG_ERR_TIMEOUT;
}

uint32_t ldg_math_phys_em_magnetostatic_link_get(const double *potential, const uint16_t *conductor, uint64_t cell_cunt, uint32_t tgt, double *out)
{
    double link_sum = 0.0;
    uint64_t tgt_cunt = 0;
    uint64_t center = 0;
    if (LDG_UNLIKELY(!potential)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!conductor)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(cell_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (center = 0; center < cell_cunt; center++)
    {
        if (conductor[center] != tgt) { continue; }

        link_sum += potential[center];
        tgt_cunt++;
    }
    if (LDG_UNLIKELY(tgt_cunt == 0)) { return LDG_ERR_EMPTY; }

    *out = link_sum / (double)tgt_cunt;
    return LDG_ERR_AOK;
}
