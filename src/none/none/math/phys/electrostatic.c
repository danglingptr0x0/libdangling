
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>

#include "electrostatic.h"
#include "field.h"

typedef struct ldg_math_phys_electrostatic_medium
{
    const uint16_t *conductor;
    const uint16_t *mtl;
    const double *coeff;
    uint32_t coeff_sel;
    uint8_t vacuum_is;
    uint8_t pudding[3];
} ldg_math_phys_electrostatic_medium_t;

static double ldg_math_phys_em_electrostatic_eps_get(const ldg_math_phys_electrostatic_medium_t *medium, uint64_t cell)
{
    if (medium->vacuum_is) { return LDG_MATH_PHYS_VACUUM_PERMITTIVITY; }

    return medium->coeff[(uint64_t)medium->mtl[cell] * LDG_MATH_PHYS_EM_COEFF_STRIDE + medium->coeff_sel];
}

static double ldg_math_phys_em_electrostatic_face_eps_get(const ldg_math_phys_electrostatic_medium_t *medium, uint64_t cell_a, uint64_t cell_b)
{
    double eps_a = ldg_math_phys_em_electrostatic_eps_get(medium, cell_a);
    double eps_b = ldg_math_phys_em_electrostatic_eps_get(medium, cell_b);
    if (medium->conductor[cell_b] != LDG_MATH_PHYS_ELECTROSTATIC_FREE) { return eps_a; }

    if (medium->conductor[cell_a] != LDG_MATH_PHYS_ELECTROSTATIC_FREE) { return eps_b; }

    if (eps_a + eps_b <= 0.0) { return 0.0; }

    return 2.0 * eps_a * eps_b / (eps_a + eps_b);
}

static double ldg_math_phys_em_electrostatic_op_face_get(const double *in_vec, const ldg_math_phys_electrostatic_medium_t *medium, uint64_t center, uint64_t near_cell)
{
    double face_eps = ldg_math_phys_em_electrostatic_face_eps_get(medium, center, near_cell);
    double near_val = (medium->conductor[near_cell] != LDG_MATH_PHYS_ELECTROSTATIC_FREE) ? 0.0 : in_vec[near_cell];
    return face_eps * (in_vec[center] - near_val);
}

static double ldg_math_phys_em_electrostatic_fixed_face_get(const double *potential, const ldg_math_phys_electrostatic_medium_t *medium, uint64_t center, uint64_t near_cell)
{
    if (medium->conductor[near_cell] == LDG_MATH_PHYS_ELECTROSTATIC_FREE) { return 0.0; }

    return ldg_math_phys_em_electrostatic_face_eps_get(medium, center, near_cell) * potential[near_cell];
}

static double ldg_math_phys_em_electrostatic_charge_face_get(const double *potential, const ldg_math_phys_electrostatic_medium_t *medium, uint64_t center, uint64_t near_cell, uint32_t tgt)
{
    if (medium->conductor[near_cell] != tgt) { return 0.0; }

    return ldg_math_phys_em_electrostatic_face_eps_get(medium, center, near_cell) * (potential[near_cell] - potential[center]);
}

static void ldg_math_phys_em_electrostatic_op_apply(double *out_vec, const double *in_vec, const ldg_math_phys_electrostatic_medium_t *medium, uint64_t nx, uint64_t ny, uint64_t nz)
{
    double acc = 0.0;
    uint64_t plane = nx * ny;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                if (medium->conductor[center] != LDG_MATH_PHYS_ELECTROSTATIC_FREE)
                {
                    out_vec[center] = 0.0;
                    continue;
                }

                acc = 0.0;
                if (idx_x > 0) { acc += ldg_math_phys_em_electrostatic_op_face_get(in_vec, medium, center, center - 1); }

                if (idx_x + 1 < nx) { acc += ldg_math_phys_em_electrostatic_op_face_get(in_vec, medium, center, center + 1); }

                if (idx_y > 0) { acc += ldg_math_phys_em_electrostatic_op_face_get(in_vec, medium, center, center - nx); }

                if (idx_y + 1 < ny) { acc += ldg_math_phys_em_electrostatic_op_face_get(in_vec, medium, center, center + nx); }

                if (idx_z > 0) { acc += ldg_math_phys_em_electrostatic_op_face_get(in_vec, medium, center, center - plane); }

                if (idx_z + 1 < nz) { acc += ldg_math_phys_em_electrostatic_op_face_get(in_vec, medium, center, center + plane); }

                out_vec[center] = acc;
            }
        }
    }
}

uint32_t ldg_math_phys_em_electrostatic_solve(double *potential, double *resid, double *dir, double *a_dir, const uint16_t *conductor, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint32_t excited, uint32_t coeff_sel, uint8_t vacuum_is, double tol, uint64_t iter_max)
{
    ldg_math_phys_electrostatic_medium_t medium = LDG_STRUCT_ZERO_INIT;
    double resid_dot = 0.0;
    double resid_dot_next = 0.0;
    double resid_dot_first = 0.0;
    double dir_dot = 0.0;
    double alpha = 0.0;
    double beta = 0.0;
    uint64_t plane = 0;
    uint64_t cell_cunt = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t iter = 0;
    if (LDG_UNLIKELY(!potential)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!resid)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!dir)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!a_dir)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!conductor)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 3 || ny < 3 || nz == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(tol) || tol <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(iter_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    medium.conductor = conductor;
    medium.mtl = mtl;
    medium.coeff = coeff;
    medium.coeff_sel = coeff_sel;
    medium.vacuum_is = vacuum_is;
    plane = nx * ny;
    cell_cunt = plane * nz;
    for (center = 0; center < cell_cunt; center++)
    {
        if (conductor[center] != LDG_MATH_PHYS_ELECTROSTATIC_FREE) { potential[center] = (conductor[center] == excited) ? 1.0 : 0.0; }
        else { potential[center] = 0.0; }

        resid[center] = 0.0;
        dir[center] = 0.0;
        a_dir[center] = 0.0;
    }
    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                if (conductor[center] != LDG_MATH_PHYS_ELECTROSTATIC_FREE) { continue; }

                if (idx_x > 0) { resid[center] += ldg_math_phys_em_electrostatic_fixed_face_get(potential, &medium, center, center - 1); }

                if (idx_x + 1 < nx) { resid[center] += ldg_math_phys_em_electrostatic_fixed_face_get(potential, &medium, center, center + 1); }

                if (idx_y > 0) { resid[center] += ldg_math_phys_em_electrostatic_fixed_face_get(potential, &medium, center, center - nx); }

                if (idx_y + 1 < ny) { resid[center] += ldg_math_phys_em_electrostatic_fixed_face_get(potential, &medium, center, center + nx); }

                if (idx_z > 0) { resid[center] += ldg_math_phys_em_electrostatic_fixed_face_get(potential, &medium, center, center - plane); }

                if (idx_z + 1 < nz) { resid[center] += ldg_math_phys_em_electrostatic_fixed_face_get(potential, &medium, center, center + plane); }
            }
        }
    }
    resid_dot = 0.0;
    for (center = 0; center < cell_cunt; center++)
    {
        dir[center] = resid[center];
        resid_dot += resid[center] * resid[center];
    }
    resid_dot_first = resid_dot;
    if (!(resid_dot_first > 0.0)) { return LDG_ERR_AOK; }

    for (iter = 0; iter < iter_max; iter++)
    {
        ldg_math_phys_em_electrostatic_op_apply(a_dir, dir, &medium, nx, ny, nz);
        dir_dot = 0.0;
        for (center = 0; center < cell_cunt; center++) { dir_dot += dir[center] * a_dir[center]; }
        if (!(dir_dot > 0.0)) { return LDG_ERR_AOK; }

        alpha = resid_dot / dir_dot;
        for (center = 0; center < cell_cunt; center++)
        {
            if (conductor[center] != LDG_MATH_PHYS_ELECTROSTATIC_FREE) { continue; }

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

uint32_t ldg_math_phys_em_electrostatic_charge_get(const double *potential, const uint16_t *conductor, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint32_t tgt, uint32_t coeff_sel, uint8_t vacuum_is, double *out)
{
    ldg_math_phys_electrostatic_medium_t medium = LDG_STRUCT_ZERO_INIT;
    double face_factor = 0.0;
    double charge_sum = 0.0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    if (LDG_UNLIKELY(!potential)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!conductor)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(nx < 3 || ny < 3 || nz == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    medium.conductor = conductor;
    medium.mtl = mtl;
    medium.coeff = coeff;
    medium.coeff_sel = coeff_sel;
    medium.vacuum_is = vacuum_is;
    plane = nx * ny;
    face_factor = (nz == 1) ? 1.0 : spacing;
    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                if (conductor[center] != LDG_MATH_PHYS_ELECTROSTATIC_FREE) { continue; }

                if (idx_x > 0) { charge_sum += ldg_math_phys_em_electrostatic_charge_face_get(potential, &medium, center, center - 1, tgt); }

                if (idx_x + 1 < nx) { charge_sum += ldg_math_phys_em_electrostatic_charge_face_get(potential, &medium, center, center + 1, tgt); }

                if (idx_y > 0) { charge_sum += ldg_math_phys_em_electrostatic_charge_face_get(potential, &medium, center, center - nx, tgt); }

                if (idx_y + 1 < ny) { charge_sum += ldg_math_phys_em_electrostatic_charge_face_get(potential, &medium, center, center + nx, tgt); }

                if (idx_z > 0) { charge_sum += ldg_math_phys_em_electrostatic_charge_face_get(potential, &medium, center, center - plane, tgt); }

                if (idx_z + 1 < nz) { charge_sum += ldg_math_phys_em_electrostatic_charge_face_get(potential, &medium, center, center + plane, tgt); }
            }
        }
    }
    *out = charge_sum * face_factor;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_em_electrostatic_perimeter_get(const uint16_t *conductor, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint32_t tgt, double *out_perimeter, uint64_t *out_cell_cunt)
{
    double face_factor = 0.0;
    uint64_t plane = 0;
    uint64_t face_cunt = 0;
    uint64_t hit_cunt = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    if (LDG_UNLIKELY(!conductor)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_perimeter)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_cell_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_perimeter = 0.0;
    *out_cell_cunt = 0;
    if (LDG_UNLIKELY(nx < 3 || ny < 3 || nz == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    plane = nx * ny;
    face_factor = (nz == 1) ? spacing : spacing * spacing;
    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                if (conductor[center] != tgt) { continue; }

                hit_cunt++;
                if (idx_x > 0 && conductor[center - 1] == LDG_MATH_PHYS_ELECTROSTATIC_FREE) { face_cunt++; }

                if (idx_x + 1 < nx && conductor[center + 1] == LDG_MATH_PHYS_ELECTROSTATIC_FREE) { face_cunt++; }

                if (idx_y > 0 && conductor[center - nx] == LDG_MATH_PHYS_ELECTROSTATIC_FREE) { face_cunt++; }

                if (idx_y + 1 < ny && conductor[center + nx] == LDG_MATH_PHYS_ELECTROSTATIC_FREE) { face_cunt++; }

                if (idx_z > 0 && conductor[center - plane] == LDG_MATH_PHYS_ELECTROSTATIC_FREE) { face_cunt++; }

                if (idx_z + 1 < nz && conductor[center + plane] == LDG_MATH_PHYS_ELECTROSTATIC_FREE) { face_cunt++; }
            }
        }
    }
    *out_perimeter = (double)face_cunt * face_factor;
    *out_cell_cunt = hit_cunt;
    return LDG_ERR_AOK;
}
