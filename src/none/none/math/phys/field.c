
#include <dangling/core/err.h>
#include <dangling/math/math.h>

#include "field.h"

uint32_t ldg_math_phys_field_energy_get(const double *cur, const double *prev, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double wave_speed, double dt, double *out)
{
    double kinetic_sum = 0.0;
    double grad_sum = 0.0;
    double rate = 0.0;
    double diff = 0.0;
    double cell_vol = 0.0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t center = 0;
    if (LDG_UNLIKELY(!cur)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!prev)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(nx == 0 || ny == 0 || nz == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(wave_speed) || wave_speed < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(dt) || dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                rate = (cur[center] - prev[center]) / dt;
                kinetic_sum += 0.5 * rate * rate;
                if (idx_x + 1 < nx)
                {
                    diff = (cur[center + 1] - cur[center]) / spacing;
                    grad_sum += 0.5 * diff * diff;
                }

                if (idx_y + 1 < ny)
                {
                    diff = (cur[center + nx] - cur[center]) / spacing;
                    grad_sum += 0.5 * diff * diff;
                }

                if (idx_z + 1 < nz)
                {
                    diff = (cur[center + ny * nx] - cur[center]) / spacing;
                    grad_sum += 0.5 * diff * diff;
                }
            }
        }
    }
    cell_vol = (nz == 1) ? (spacing * spacing) : (spacing * spacing * spacing);
    *out = (kinetic_sum + wave_speed * wave_speed * grad_sum) * cell_vol;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_em_electric_advance(double *electric, const double *magnetic, const double *drive, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint64_t slab_lo, uint64_t slab_hi)
{
    const double *row = 0x0;
    const double *magnetic_x = 0x0;
    const double *magnetic_y = 0x0;
    const double *magnetic_z = 0x0;
    const double *stretch_x = 0x0;
    const double *stretch_y = 0x0;
    const double *stretch_z = 0x0;
    double *electric_x = 0x0;
    double *electric_y = 0x0;
    double *electric_z = 0x0;
    double *psi_a = 0x0;
    double *psi_b = 0x0;
    double diff_a = 0.0;
    double diff_b = 0.0;
    double term_a = 0.0;
    double term_b = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t z_lo = 0;
    uint64_t z_hi = 0;
    if (LDG_UNLIKELY(slab_lo >= slab_hi || slab_hi > nz)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!electric)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!magnetic)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!drive)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!psi)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!stretch)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 2 || ny < 2 || nz < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny * nz;
    plane = nx * ny;
    electric_x = electric;
    electric_y = electric + cell_cunt;
    electric_z = electric + 2 * cell_cunt;
    magnetic_x = magnetic;
    magnetic_y = magnetic + cell_cunt;
    magnetic_z = magnetic + 2 * cell_cunt;
    stretch_x = stretch;
    stretch_y = stretch + LDG_MATH_PHYS_EM_STRETCH_STRIDE * nx;
    stretch_z = stretch + LDG_MATH_PHYS_EM_STRETCH_STRIDE * (nx + ny);
    psi_a = psi + LDG_MATH_PHYS_EM_PSI_EXY * cell_cunt;
    psi_b = psi + LDG_MATH_PHYS_EM_PSI_EXZ * cell_cunt;
    z_lo = (slab_lo < 1) ? 1 : slab_lo;
    z_hi = slab_hi;
    for (idx_z = z_lo; idx_z < z_hi; idx_z++)
    {
        for (idx_y = 1; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                diff_a = (magnetic_z[center] - magnetic_z[center - nx]) / spacing;
                psi_a[center] = stretch_y[LDG_MATH_PHYS_EM_STRETCH_B_INT * ny + idx_y] * psi_a[center] + stretch_y[LDG_MATH_PHYS_EM_STRETCH_C_INT * ny + idx_y] * diff_a;
                term_a = diff_a / stretch_y[LDG_MATH_PHYS_EM_STRETCH_KAPPA_INT * ny + idx_y] + psi_a[center];
                diff_b = (magnetic_y[center] - magnetic_y[center - plane]) / spacing;
                psi_b[center] = stretch_z[LDG_MATH_PHYS_EM_STRETCH_B_INT * nz + idx_z] * psi_b[center] + stretch_z[LDG_MATH_PHYS_EM_STRETCH_C_INT * nz + idx_z] * diff_b;
                term_b = diff_b / stretch_z[LDG_MATH_PHYS_EM_STRETCH_KAPPA_INT * nz + idx_z] + psi_b[center];
                electric_x[center] = row[LDG_MATH_PHYS_EM_COEFF_CA] * electric_x[center] + row[LDG_MATH_PHYS_EM_COEFF_CB] * (term_a - term_b - drive[center]);
            }
        }
    }
    psi_a = psi + LDG_MATH_PHYS_EM_PSI_EYZ * cell_cunt;
    psi_b = psi + LDG_MATH_PHYS_EM_PSI_EYX * cell_cunt;
    for (idx_z = z_lo; idx_z < z_hi; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 1; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                diff_a = (magnetic_x[center] - magnetic_x[center - plane]) / spacing;
                psi_a[center] = stretch_z[LDG_MATH_PHYS_EM_STRETCH_B_INT * nz + idx_z] * psi_a[center] + stretch_z[LDG_MATH_PHYS_EM_STRETCH_C_INT * nz + idx_z] * diff_a;
                term_a = diff_a / stretch_z[LDG_MATH_PHYS_EM_STRETCH_KAPPA_INT * nz + idx_z] + psi_a[center];
                diff_b = (magnetic_z[center] - magnetic_z[center - 1]) / spacing;
                psi_b[center] = stretch_x[LDG_MATH_PHYS_EM_STRETCH_B_INT * nx + idx_x] * psi_b[center] + stretch_x[LDG_MATH_PHYS_EM_STRETCH_C_INT * nx + idx_x] * diff_b;
                term_b = diff_b / stretch_x[LDG_MATH_PHYS_EM_STRETCH_KAPPA_INT * nx + idx_x] + psi_b[center];
                electric_y[center] = row[LDG_MATH_PHYS_EM_COEFF_CA] * electric_y[center] + row[LDG_MATH_PHYS_EM_COEFF_CB] * (term_a - term_b - drive[cell_cunt + center]);
            }
        }
    }
    psi_a = psi + LDG_MATH_PHYS_EM_PSI_EZX * cell_cunt;
    psi_b = psi + LDG_MATH_PHYS_EM_PSI_EZY * cell_cunt;
    for (idx_z = slab_lo; idx_z < slab_hi; idx_z++)
    {
        for (idx_y = 1; idx_y < ny; idx_y++)
        {
            for (idx_x = 1; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                diff_a = (magnetic_y[center] - magnetic_y[center - 1]) / spacing;
                psi_a[center] = stretch_x[LDG_MATH_PHYS_EM_STRETCH_B_INT * nx + idx_x] * psi_a[center] + stretch_x[LDG_MATH_PHYS_EM_STRETCH_C_INT * nx + idx_x] * diff_a;
                term_a = diff_a / stretch_x[LDG_MATH_PHYS_EM_STRETCH_KAPPA_INT * nx + idx_x] + psi_a[center];
                diff_b = (magnetic_x[center] - magnetic_x[center - nx]) / spacing;
                psi_b[center] = stretch_y[LDG_MATH_PHYS_EM_STRETCH_B_INT * ny + idx_y] * psi_b[center] + stretch_y[LDG_MATH_PHYS_EM_STRETCH_C_INT * ny + idx_y] * diff_b;
                term_b = diff_b / stretch_y[LDG_MATH_PHYS_EM_STRETCH_KAPPA_INT * ny + idx_y] + psi_b[center];
                electric_z[center] = row[LDG_MATH_PHYS_EM_COEFF_CA] * electric_z[center] + row[LDG_MATH_PHYS_EM_COEFF_CB] * (term_a - term_b - drive[2 * cell_cunt + center]);
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_em_magnetic_advance(double *magnetic, const double *electric, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint64_t slab_lo, uint64_t slab_hi)
{
    const double *row = 0x0;
    const double *electric_x = 0x0;
    const double *electric_y = 0x0;
    const double *electric_z = 0x0;
    const double *stretch_x = 0x0;
    const double *stretch_y = 0x0;
    const double *stretch_z = 0x0;
    double *magnetic_x = 0x0;
    double *magnetic_y = 0x0;
    double *magnetic_z = 0x0;
    double *psi_a = 0x0;
    double *psi_b = 0x0;
    double diff_a = 0.0;
    double diff_b = 0.0;
    double term_a = 0.0;
    double term_b = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t z_lo = 0;
    uint64_t z_hi = 0;
    if (LDG_UNLIKELY(slab_lo >= slab_hi || slab_hi > nz)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!magnetic)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!electric)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!psi)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!stretch)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 2 || ny < 2 || nz < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny * nz;
    plane = nx * ny;
    magnetic_x = magnetic;
    magnetic_y = magnetic + cell_cunt;
    magnetic_z = magnetic + 2 * cell_cunt;
    electric_x = electric;
    electric_y = electric + cell_cunt;
    electric_z = electric + 2 * cell_cunt;
    stretch_x = stretch;
    stretch_y = stretch + LDG_MATH_PHYS_EM_STRETCH_STRIDE * nx;
    stretch_z = stretch + LDG_MATH_PHYS_EM_STRETCH_STRIDE * (nx + ny);
    psi_a = psi + LDG_MATH_PHYS_EM_PSI_HXY * cell_cunt;
    psi_b = psi + LDG_MATH_PHYS_EM_PSI_HXZ * cell_cunt;
    z_lo = slab_lo;
    z_hi = (slab_hi > nz - 1) ? nz - 1 : slab_hi;
    for (idx_z = z_lo; idx_z < z_hi; idx_z++)
    {
        for (idx_y = 0; idx_y + 1 < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                diff_a = (electric_z[center + nx] - electric_z[center]) / spacing;
                psi_a[center] = stretch_y[LDG_MATH_PHYS_EM_STRETCH_B_HALF * ny + idx_y] * psi_a[center] + stretch_y[LDG_MATH_PHYS_EM_STRETCH_C_HALF * ny + idx_y] * diff_a;
                term_a = diff_a / stretch_y[LDG_MATH_PHYS_EM_STRETCH_KAPPA_HALF * ny + idx_y] + psi_a[center];
                diff_b = (electric_y[center + plane] - electric_y[center]) / spacing;
                psi_b[center] = stretch_z[LDG_MATH_PHYS_EM_STRETCH_B_HALF * nz + idx_z] * psi_b[center] + stretch_z[LDG_MATH_PHYS_EM_STRETCH_C_HALF * nz + idx_z] * diff_b;
                term_b = diff_b / stretch_z[LDG_MATH_PHYS_EM_STRETCH_KAPPA_HALF * nz + idx_z] + psi_b[center];
                magnetic_x[center] -= row[LDG_MATH_PHYS_EM_COEFF_DB] * (term_a - term_b);
            }
        }
    }
    psi_a = psi + LDG_MATH_PHYS_EM_PSI_HYZ * cell_cunt;
    psi_b = psi + LDG_MATH_PHYS_EM_PSI_HYX * cell_cunt;
    for (idx_z = z_lo; idx_z < z_hi; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x + 1 < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                diff_a = (electric_x[center + plane] - electric_x[center]) / spacing;
                psi_a[center] = stretch_z[LDG_MATH_PHYS_EM_STRETCH_B_HALF * nz + idx_z] * psi_a[center] + stretch_z[LDG_MATH_PHYS_EM_STRETCH_C_HALF * nz + idx_z] * diff_a;
                term_a = diff_a / stretch_z[LDG_MATH_PHYS_EM_STRETCH_KAPPA_HALF * nz + idx_z] + psi_a[center];
                diff_b = (electric_z[center + 1] - electric_z[center]) / spacing;
                psi_b[center] = stretch_x[LDG_MATH_PHYS_EM_STRETCH_B_HALF * nx + idx_x] * psi_b[center] + stretch_x[LDG_MATH_PHYS_EM_STRETCH_C_HALF * nx + idx_x] * diff_b;
                term_b = diff_b / stretch_x[LDG_MATH_PHYS_EM_STRETCH_KAPPA_HALF * nx + idx_x] + psi_b[center];
                magnetic_y[center] -= row[LDG_MATH_PHYS_EM_COEFF_DB] * (term_a - term_b);
            }
        }
    }
    psi_a = psi + LDG_MATH_PHYS_EM_PSI_HZX * cell_cunt;
    psi_b = psi + LDG_MATH_PHYS_EM_PSI_HZY * cell_cunt;
    for (idx_z = slab_lo; idx_z < slab_hi; idx_z++)
    {
        for (idx_y = 0; idx_y + 1 < ny; idx_y++)
        {
            for (idx_x = 0; idx_x + 1 < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                diff_a = (electric_y[center + 1] - electric_y[center]) / spacing;
                psi_a[center] = stretch_x[LDG_MATH_PHYS_EM_STRETCH_B_HALF * nx + idx_x] * psi_a[center] + stretch_x[LDG_MATH_PHYS_EM_STRETCH_C_HALF * nx + idx_x] * diff_a;
                term_a = diff_a / stretch_x[LDG_MATH_PHYS_EM_STRETCH_KAPPA_HALF * nx + idx_x] + psi_a[center];
                diff_b = (electric_x[center + nx] - electric_x[center]) / spacing;
                psi_b[center] = stretch_y[LDG_MATH_PHYS_EM_STRETCH_B_HALF * ny + idx_y] * psi_b[center] + stretch_y[LDG_MATH_PHYS_EM_STRETCH_C_HALF * ny + idx_y] * diff_b;
                term_b = diff_b / stretch_y[LDG_MATH_PHYS_EM_STRETCH_KAPPA_HALF * ny + idx_y] + psi_b[center];
                magnetic_z[center] -= row[LDG_MATH_PHYS_EM_COEFF_DB] * (term_a - term_b);
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_em_electric_2d_advance(double *electric, const double *magnetic, const double *drive, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, double spacing)
{
    const double *row = 0x0;
    const double *magnetic_x = 0x0;
    const double *magnetic_y = 0x0;
    const double *magnetic_z = 0x0;
    const double *stretch_x = 0x0;
    const double *stretch_y = 0x0;
    double *electric_x = 0x0;
    double *electric_y = 0x0;
    double *electric_z = 0x0;
    double *psi_a = 0x0;
    double *psi_b = 0x0;
    double diff_a = 0.0;
    double diff_b = 0.0;
    double term_a = 0.0;
    double term_b = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    if (LDG_UNLIKELY(!electric)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!magnetic)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!drive)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!psi)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!stretch)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 2 || ny < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny;
    electric_x = electric;
    electric_y = electric + cell_cunt;
    electric_z = electric + 2 * cell_cunt;
    magnetic_x = magnetic;
    magnetic_y = magnetic + cell_cunt;
    magnetic_z = magnetic + 2 * cell_cunt;
    stretch_x = stretch;
    stretch_y = stretch + LDG_MATH_PHYS_EM_STRETCH_STRIDE * nx;
    psi_a = psi + LDG_MATH_PHYS_EM_PSI_EXY * cell_cunt;
    for (idx_y = 1; idx_y < ny; idx_y++) { for (idx_x = 0; idx_x < nx; idx_x++)
        {
            center = idx_y * nx + idx_x;
            row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
            diff_a = (magnetic_z[center] - magnetic_z[center - nx]) / spacing;
            psi_a[center] = stretch_y[LDG_MATH_PHYS_EM_STRETCH_B_INT * ny + idx_y] * psi_a[center] + stretch_y[LDG_MATH_PHYS_EM_STRETCH_C_INT * ny + idx_y] * diff_a;
            term_a = diff_a / stretch_y[LDG_MATH_PHYS_EM_STRETCH_KAPPA_INT * ny + idx_y] + psi_a[center];
            electric_x[center] = row[LDG_MATH_PHYS_EM_COEFF_CA] * electric_x[center] + row[LDG_MATH_PHYS_EM_COEFF_CB] * (term_a - drive[center]);
        }
    }
    psi_b = psi + LDG_MATH_PHYS_EM_PSI_EYX * cell_cunt;
    for (idx_y = 0; idx_y < ny; idx_y++) { for (idx_x = 1; idx_x < nx; idx_x++)
        {
            center = idx_y * nx + idx_x;
            row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
            diff_b = (magnetic_z[center] - magnetic_z[center - 1]) / spacing;
            psi_b[center] = stretch_x[LDG_MATH_PHYS_EM_STRETCH_B_INT * nx + idx_x] * psi_b[center] + stretch_x[LDG_MATH_PHYS_EM_STRETCH_C_INT * nx + idx_x] * diff_b;
            term_b = diff_b / stretch_x[LDG_MATH_PHYS_EM_STRETCH_KAPPA_INT * nx + idx_x] + psi_b[center];
            electric_y[center] = row[LDG_MATH_PHYS_EM_COEFF_CA] * electric_y[center] + row[LDG_MATH_PHYS_EM_COEFF_CB] * ((0.0 - term_b) - drive[cell_cunt + center]);
        }
    }
    psi_a = psi + LDG_MATH_PHYS_EM_PSI_EZX * cell_cunt;
    psi_b = psi + LDG_MATH_PHYS_EM_PSI_EZY * cell_cunt;
    for (idx_y = 1; idx_y < ny; idx_y++)
    {
        for (idx_x = 1; idx_x < nx; idx_x++)
        {
            center = idx_y * nx + idx_x;
            row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
            diff_a = (magnetic_y[center] - magnetic_y[center - 1]) / spacing;
            psi_a[center] = stretch_x[LDG_MATH_PHYS_EM_STRETCH_B_INT * nx + idx_x] * psi_a[center] + stretch_x[LDG_MATH_PHYS_EM_STRETCH_C_INT * nx + idx_x] * diff_a;
            term_a = diff_a / stretch_x[LDG_MATH_PHYS_EM_STRETCH_KAPPA_INT * nx + idx_x] + psi_a[center];
            diff_b = (magnetic_x[center] - magnetic_x[center - nx]) / spacing;
            psi_b[center] = stretch_y[LDG_MATH_PHYS_EM_STRETCH_B_INT * ny + idx_y] * psi_b[center] + stretch_y[LDG_MATH_PHYS_EM_STRETCH_C_INT * ny + idx_y] * diff_b;
            term_b = diff_b / stretch_y[LDG_MATH_PHYS_EM_STRETCH_KAPPA_INT * ny + idx_y] + psi_b[center];
            electric_z[center] = row[LDG_MATH_PHYS_EM_COEFF_CA] * electric_z[center] + row[LDG_MATH_PHYS_EM_COEFF_CB] * (term_a - term_b - drive[2 * cell_cunt + center]);
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_em_magnetic_2d_advance(double *magnetic, const double *electric, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, double spacing)
{
    const double *row = 0x0;
    const double *electric_x = 0x0;
    const double *electric_y = 0x0;
    const double *electric_z = 0x0;
    const double *stretch_x = 0x0;
    const double *stretch_y = 0x0;
    double *magnetic_x = 0x0;
    double *magnetic_y = 0x0;
    double *magnetic_z = 0x0;
    double *psi_a = 0x0;
    double *psi_b = 0x0;
    double diff_a = 0.0;
    double diff_b = 0.0;
    double term_a = 0.0;
    double term_b = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    if (LDG_UNLIKELY(!magnetic)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!electric)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!psi)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!stretch)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 2 || ny < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny;
    magnetic_x = magnetic;
    magnetic_y = magnetic + cell_cunt;
    magnetic_z = magnetic + 2 * cell_cunt;
    electric_x = electric;
    electric_y = electric + cell_cunt;
    electric_z = electric + 2 * cell_cunt;
    stretch_x = stretch;
    stretch_y = stretch + LDG_MATH_PHYS_EM_STRETCH_STRIDE * nx;
    psi_a = psi + LDG_MATH_PHYS_EM_PSI_HXY * cell_cunt;
    for (idx_y = 0; idx_y + 1 < ny; idx_y++) { for (idx_x = 0; idx_x < nx; idx_x++)
        {
            center = idx_y * nx + idx_x;
            row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
            diff_a = (electric_z[center + nx] - electric_z[center]) / spacing;
            psi_a[center] = stretch_y[LDG_MATH_PHYS_EM_STRETCH_B_HALF * ny + idx_y] * psi_a[center] + stretch_y[LDG_MATH_PHYS_EM_STRETCH_C_HALF * ny + idx_y] * diff_a;
            term_a = diff_a / stretch_y[LDG_MATH_PHYS_EM_STRETCH_KAPPA_HALF * ny + idx_y] + psi_a[center];
            magnetic_x[center] -= row[LDG_MATH_PHYS_EM_COEFF_DB] * term_a;
        }
    }
    psi_b = psi + LDG_MATH_PHYS_EM_PSI_HYX * cell_cunt;
    for (idx_y = 0; idx_y < ny; idx_y++) { for (idx_x = 0; idx_x + 1 < nx; idx_x++)
        {
            center = idx_y * nx + idx_x;
            row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
            diff_b = (electric_z[center + 1] - electric_z[center]) / spacing;
            psi_b[center] = stretch_x[LDG_MATH_PHYS_EM_STRETCH_B_HALF * nx + idx_x] * psi_b[center] + stretch_x[LDG_MATH_PHYS_EM_STRETCH_C_HALF * nx + idx_x] * diff_b;
            term_b = diff_b / stretch_x[LDG_MATH_PHYS_EM_STRETCH_KAPPA_HALF * nx + idx_x] + psi_b[center];
            magnetic_y[center] -= row[LDG_MATH_PHYS_EM_COEFF_DB] * (0.0 - term_b);
        }
    }
    psi_a = psi + LDG_MATH_PHYS_EM_PSI_HZX * cell_cunt;
    psi_b = psi + LDG_MATH_PHYS_EM_PSI_HZY * cell_cunt;
    for (idx_y = 0; idx_y + 1 < ny; idx_y++)
    {
        for (idx_x = 0; idx_x + 1 < nx; idx_x++)
        {
            center = idx_y * nx + idx_x;
            row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
            diff_a = (electric_y[center + 1] - electric_y[center]) / spacing;
            psi_a[center] = stretch_x[LDG_MATH_PHYS_EM_STRETCH_B_HALF * nx + idx_x] * psi_a[center] + stretch_x[LDG_MATH_PHYS_EM_STRETCH_C_HALF * nx + idx_x] * diff_a;
            term_a = diff_a / stretch_x[LDG_MATH_PHYS_EM_STRETCH_KAPPA_HALF * nx + idx_x] + psi_a[center];
            diff_b = (electric_x[center + nx] - electric_x[center]) / spacing;
            psi_b[center] = stretch_y[LDG_MATH_PHYS_EM_STRETCH_B_HALF * ny + idx_y] * psi_b[center] + stretch_y[LDG_MATH_PHYS_EM_STRETCH_C_HALF * ny + idx_y] * diff_b;
            term_b = diff_b / stretch_y[LDG_MATH_PHYS_EM_STRETCH_KAPPA_HALF * ny + idx_y] + psi_b[center];
            magnetic_z[center] -= row[LDG_MATH_PHYS_EM_COEFF_DB] * (term_a - term_b);
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_em_energy_get(const double *electric, const double *magnetic, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double *out)
{
    const double *row = 0x0;
    double electric_sum = 0.0;
    double magnetic_sum = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t center = 0;
    if (LDG_UNLIKELY(!electric)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!magnetic)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(nx == 0 || ny == 0 || nz == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny * nz;
    for (center = 0; center < cell_cunt; center++)
    {
        row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
        electric_sum += 0.5 * row[LDG_MATH_PHYS_EM_COEFF_EPSILON] * (electric[center] * electric[center] + electric[cell_cunt + center] * electric[cell_cunt + center] + electric[2 * cell_cunt + center] * electric[2 * cell_cunt + center]);
        magnetic_sum += 0.5 * row[LDG_MATH_PHYS_EM_COEFF_MU] * (magnetic[center] * magnetic[center] + magnetic[cell_cunt + center] * magnetic[cell_cunt + center] + magnetic[2 * cell_cunt + center] * magnetic[2 * cell_cunt + center]);
    }
    *out = (electric_sum + magnetic_sum) * ((nz == 1) ? (spacing * spacing) : (spacing * spacing * spacing));
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_acoustic_velocity_advance(double *velocity, const double *pressure, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint64_t slab_lo, uint64_t slab_hi)
{
    const double *row_lo = 0x0;
    const double *row_hi = 0x0;
    const double *stretch_x = 0x0;
    const double *stretch_y = 0x0;
    const double *stretch_z = 0x0;
    double *velocity_x = 0x0;
    double *velocity_y = 0x0;
    double *velocity_z = 0x0;
    double *psi_plane = 0x0;
    double diff_val = 0.0;
    double term_val = 0.0;
    double cv_face = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t z_hi = 0;
    if (LDG_UNLIKELY(slab_lo >= slab_hi || slab_hi > nz)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!velocity)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pressure)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!psi)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!stretch)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 2 || ny < 2 || nz < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny * nz;
    plane = nx * ny;
    velocity_x = velocity;
    velocity_y = velocity + cell_cunt;
    velocity_z = velocity + 2 * cell_cunt;
    stretch_x = stretch;
    stretch_y = stretch + LDG_MATH_PHYS_ACOUSTIC_STRETCH_STRIDE * nx;
    stretch_z = stretch + LDG_MATH_PHYS_ACOUSTIC_STRETCH_STRIDE * (nx + ny);
    psi_plane = psi + LDG_MATH_PHYS_ACOUSTIC_PSI_VX * cell_cunt;
    for (idx_z = slab_lo; idx_z < slab_hi; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x + 1 < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                row_lo = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
                row_hi = coeff + (uint64_t)mtl[center + 1] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
                cv_face = 0.5 * (row_lo[LDG_MATH_PHYS_ACOUSTIC_COEFF_CV] + row_hi[LDG_MATH_PHYS_ACOUSTIC_COEFF_CV]);
                diff_val = (pressure[center + 1] - pressure[center]) / spacing;
                psi_plane[center] = stretch_x[LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_HALF * nx + idx_x] * psi_plane[center] + stretch_x[LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_HALF * nx + idx_x] * diff_val;
                term_val = diff_val / stretch_x[LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_HALF * nx + idx_x] + psi_plane[center];
                velocity_x[center] -= cv_face * term_val;
            }
        }
    }
    psi_plane = psi + LDG_MATH_PHYS_ACOUSTIC_PSI_VY * cell_cunt;
    for (idx_z = slab_lo; idx_z < slab_hi; idx_z++)
    {
        for (idx_y = 0; idx_y + 1 < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                row_lo = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
                row_hi = coeff + (uint64_t)mtl[center + nx] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
                cv_face = 0.5 * (row_lo[LDG_MATH_PHYS_ACOUSTIC_COEFF_CV] + row_hi[LDG_MATH_PHYS_ACOUSTIC_COEFF_CV]);
                diff_val = (pressure[center + nx] - pressure[center]) / spacing;
                psi_plane[center] = stretch_y[LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_HALF * ny + idx_y] * psi_plane[center] + stretch_y[LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_HALF * ny + idx_y] * diff_val;
                term_val = diff_val / stretch_y[LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_HALF * ny + idx_y] + psi_plane[center];
                velocity_y[center] -= cv_face * term_val;
            }
        }
    }
    psi_plane = psi + LDG_MATH_PHYS_ACOUSTIC_PSI_VZ * cell_cunt;
    z_hi = (slab_hi > nz - 1) ? nz - 1 : slab_hi;
    for (idx_z = slab_lo; idx_z < z_hi; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                row_lo = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
                row_hi = coeff + (uint64_t)mtl[center + plane] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
                cv_face = 0.5 * (row_lo[LDG_MATH_PHYS_ACOUSTIC_COEFF_CV] + row_hi[LDG_MATH_PHYS_ACOUSTIC_COEFF_CV]);
                diff_val = (pressure[center + plane] - pressure[center]) / spacing;
                psi_plane[center] = stretch_z[LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_HALF * nz + idx_z] * psi_plane[center] + stretch_z[LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_HALF * nz + idx_z] * diff_val;
                term_val = diff_val / stretch_z[LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_HALF * nz + idx_z] + psi_plane[center];
                velocity_z[center] -= cv_face * term_val;
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_acoustic_pressure_advance(double *pressure, const double *velocity, const double *drive, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint64_t slab_lo, uint64_t slab_hi)
{
    const double *row = 0x0;
    const double *velocity_x = 0x0;
    const double *velocity_y = 0x0;
    const double *velocity_z = 0x0;
    const double *stretch_x = 0x0;
    const double *stretch_y = 0x0;
    const double *stretch_z = 0x0;
    double *psi_x = 0x0;
    double *psi_y = 0x0;
    double *psi_z = 0x0;
    double diff_val = 0.0;
    double div_sum = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t z_lo = 0;
    uint64_t z_hi = 0;
    if (LDG_UNLIKELY(slab_lo >= slab_hi || slab_hi > nz)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!pressure)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!velocity)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!drive)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!psi)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!stretch)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 2 || ny < 2 || nz < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny * nz;
    plane = nx * ny;
    velocity_x = velocity;
    velocity_y = velocity + cell_cunt;
    velocity_z = velocity + 2 * cell_cunt;
    stretch_x = stretch;
    stretch_y = stretch + LDG_MATH_PHYS_ACOUSTIC_STRETCH_STRIDE * nx;
    stretch_z = stretch + LDG_MATH_PHYS_ACOUSTIC_STRETCH_STRIDE * (nx + ny);
    psi_x = psi + LDG_MATH_PHYS_ACOUSTIC_PSI_PX * cell_cunt;
    psi_y = psi + LDG_MATH_PHYS_ACOUSTIC_PSI_PY * cell_cunt;
    psi_z = psi + LDG_MATH_PHYS_ACOUSTIC_PSI_PZ * cell_cunt;
    z_lo = (slab_lo < 1) ? 1 : slab_lo;
    z_hi = (slab_hi > nz - 1) ? nz - 1 : slab_hi;
    for (idx_z = z_lo; idx_z < z_hi; idx_z++)
    {
        for (idx_y = 1; idx_y + 1 < ny; idx_y++)
        {
            for (idx_x = 1; idx_x + 1 < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
                diff_val = (velocity_x[center] - velocity_x[center - 1]) / spacing;
                psi_x[center] = stretch_x[LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_INT * nx + idx_x] * psi_x[center] + stretch_x[LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_INT * nx + idx_x] * diff_val;
                div_sum = diff_val / stretch_x[LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_INT * nx + idx_x] + psi_x[center];
                diff_val = (velocity_y[center] - velocity_y[center - nx]) / spacing;
                psi_y[center] = stretch_y[LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_INT * ny + idx_y] * psi_y[center] + stretch_y[LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_INT * ny + idx_y] * diff_val;
                div_sum += diff_val / stretch_y[LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_INT * ny + idx_y] + psi_y[center];
                diff_val = (velocity_z[center] - velocity_z[center - plane]) / spacing;
                psi_z[center] = stretch_z[LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_INT * nz + idx_z] * psi_z[center] + stretch_z[LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_INT * nz + idx_z] * diff_val;
                div_sum += diff_val / stretch_z[LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_INT * nz + idx_z] + psi_z[center];
                pressure[center] = row[LDG_MATH_PHYS_ACOUSTIC_COEFF_CA] * pressure[center] + row[LDG_MATH_PHYS_ACOUSTIC_COEFF_CB] * (drive[center] - div_sum);
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_acoustic_velocity_2d_advance(double *velocity, const double *pressure, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, double spacing)
{
    const double *row_lo = 0x0;
    const double *row_hi = 0x0;
    const double *stretch_x = 0x0;
    const double *stretch_y = 0x0;
    double *velocity_x = 0x0;
    double *velocity_y = 0x0;
    double *psi_plane = 0x0;
    double diff_val = 0.0;
    double term_val = 0.0;
    double cv_face = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    if (LDG_UNLIKELY(!velocity)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pressure)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!psi)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!stretch)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 2 || ny < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny;
    velocity_x = velocity;
    velocity_y = velocity + cell_cunt;
    stretch_x = stretch;
    stretch_y = stretch + LDG_MATH_PHYS_ACOUSTIC_STRETCH_STRIDE * nx;
    psi_plane = psi + LDG_MATH_PHYS_ACOUSTIC_PSI_VX * cell_cunt;
    for (idx_y = 0; idx_y < ny; idx_y++)
    {
        for (idx_x = 0; idx_x + 1 < nx; idx_x++)
        {
            center = idx_y * nx + idx_x;
            row_lo = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
            row_hi = coeff + (uint64_t)mtl[center + 1] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
            cv_face = 0.5 * (row_lo[LDG_MATH_PHYS_ACOUSTIC_COEFF_CV] + row_hi[LDG_MATH_PHYS_ACOUSTIC_COEFF_CV]);
            diff_val = (pressure[center + 1] - pressure[center]) / spacing;
            psi_plane[center] = stretch_x[LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_HALF * nx + idx_x] * psi_plane[center] + stretch_x[LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_HALF * nx + idx_x] * diff_val;
            term_val = diff_val / stretch_x[LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_HALF * nx + idx_x] + psi_plane[center];
            velocity_x[center] -= cv_face * term_val;
        }
    }
    psi_plane = psi + LDG_MATH_PHYS_ACOUSTIC_PSI_VY * cell_cunt;
    for (idx_y = 0; idx_y + 1 < ny; idx_y++)
    {
        for (idx_x = 0; idx_x < nx; idx_x++)
        {
            center = idx_y * nx + idx_x;
            row_lo = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
            row_hi = coeff + (uint64_t)mtl[center + nx] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
            cv_face = 0.5 * (row_lo[LDG_MATH_PHYS_ACOUSTIC_COEFF_CV] + row_hi[LDG_MATH_PHYS_ACOUSTIC_COEFF_CV]);
            diff_val = (pressure[center + nx] - pressure[center]) / spacing;
            psi_plane[center] = stretch_y[LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_HALF * ny + idx_y] * psi_plane[center] + stretch_y[LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_HALF * ny + idx_y] * diff_val;
            term_val = diff_val / stretch_y[LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_HALF * ny + idx_y] + psi_plane[center];
            velocity_y[center] -= cv_face * term_val;
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_acoustic_pressure_2d_advance(double *pressure, const double *velocity, const double *drive, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, double spacing)
{
    const double *row = 0x0;
    const double *velocity_x = 0x0;
    const double *velocity_y = 0x0;
    const double *stretch_x = 0x0;
    const double *stretch_y = 0x0;
    double *psi_x = 0x0;
    double *psi_y = 0x0;
    double diff_val = 0.0;
    double div_sum = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    if (LDG_UNLIKELY(!pressure)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!velocity)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!drive)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!psi)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!stretch)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 2 || ny < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny;
    velocity_x = velocity;
    velocity_y = velocity + cell_cunt;
    stretch_x = stretch;
    stretch_y = stretch + LDG_MATH_PHYS_ACOUSTIC_STRETCH_STRIDE * nx;
    psi_x = psi + LDG_MATH_PHYS_ACOUSTIC_PSI_PX * cell_cunt;
    psi_y = psi + LDG_MATH_PHYS_ACOUSTIC_PSI_PY * cell_cunt;
    for (idx_y = 1; idx_y + 1 < ny; idx_y++)
    {
        for (idx_x = 1; idx_x + 1 < nx; idx_x++)
        {
            center = idx_y * nx + idx_x;
            row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
            diff_val = (velocity_x[center] - velocity_x[center - 1]) / spacing;
            psi_x[center] = stretch_x[LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_INT * nx + idx_x] * psi_x[center] + stretch_x[LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_INT * nx + idx_x] * diff_val;
            div_sum = diff_val / stretch_x[LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_INT * nx + idx_x] + psi_x[center];
            diff_val = (velocity_y[center] - velocity_y[center - nx]) / spacing;
            psi_y[center] = stretch_y[LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_INT * ny + idx_y] * psi_y[center] + stretch_y[LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_INT * ny + idx_y] * diff_val;
            div_sum += diff_val / stretch_y[LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_INT * ny + idx_y] + psi_y[center];
            pressure[center] = row[LDG_MATH_PHYS_ACOUSTIC_COEFF_CA] * pressure[center] + row[LDG_MATH_PHYS_ACOUSTIC_COEFF_CB] * (drive[center] - div_sum);
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_acoustic_energy_get(const double *pressure, const double *velocity, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double *out)
{
    const double *row = 0x0;
    double pressure_sum = 0.0;
    double velocity_sum = 0.0;
    double speed_val = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t center = 0;
    if (LDG_UNLIKELY(!pressure)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!velocity)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(nx == 0 || ny == 0 || nz == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny * nz;
    for (center = 0; center < cell_cunt; center++)
    {
        row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
        speed_val = row[LDG_MATH_PHYS_ACOUSTIC_COEFF_SPEED];
        pressure_sum += 0.5 * pressure[center] * pressure[center] / (row[LDG_MATH_PHYS_ACOUSTIC_COEFF_DENSITY] * speed_val * speed_val);
        velocity_sum += 0.5 * row[LDG_MATH_PHYS_ACOUSTIC_COEFF_DENSITY] * (velocity[center] * velocity[center] + velocity[cell_cunt + center] * velocity[cell_cunt + center] + velocity[2 * cell_cunt + center] * velocity[2 * cell_cunt + center]);
    }
    *out = (pressure_sum + velocity_sum) * ((nz == 1) ? (spacing * spacing) : (spacing * spacing * spacing));
    return LDG_ERR_AOK;
}

static void ldg_math_phys_field_refract_get(const uint16_t *mtl, const double *coeff, uint64_t center, double vacuum_norm, double *out)
{
    const double *row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
    *out = ldg_math_sqrt_get(row[LDG_MATH_PHYS_EM_COEFF_EPSILON] * row[LDG_MATH_PHYS_EM_COEFF_MU] / vacuum_norm);
}

static void ldg_math_phys_field_face_cum(const double *intensity, const double *phase, uint64_t center, uint64_t nb, double spacing, double refract_center, double refract_nb, double *influx, double *denom, uint8_t *has_upwind)
{
    double refract_face = 0.5 * (refract_center + refract_nb);
    double mismatch = (refract_nb - refract_center) / (refract_nb + refract_center);
    double slope = 0.0;
    if (phase[nb] >= LDG_MATH_PHYS_EM_PHASE_FAR) { return; }

    slope = (phase[nb] - phase[center]) / (spacing * refract_face);
    if (slope > 1.0) { slope = 1.0; }

    if (slope < 0.0 - 1.0) { slope = 0.0 - 1.0; }

    if (phase[nb] < phase[center])
    {
        *influx += (1.0 - mismatch * mismatch) * intensity[nb] * (0.0 - slope);
        *has_upwind = LDG_TRUTH_TRUE;
    }
    else { *denom += slope; }
}

static void ldg_math_phys_field_reflect_cum(const double *intensity, const double *phase, uint64_t center, uint64_t nb, double spacing, double refract_center, double refract_nb, double face_area, double *reflect)
{
    double refract_face = 0.5 * (refract_center + refract_nb);
    double mismatch = (refract_nb - refract_center) / (refract_nb + refract_center);
    double slope = 0.0;
    if (phase[nb] >= LDG_MATH_PHYS_EM_PHASE_FAR) { return; }

    if (phase[nb] >= phase[center]) { return; }

    slope = (phase[center] - phase[nb]) / (spacing * refract_face);
    if (slope > 1.0) { slope = 1.0; }

    reflect[nb] += mismatch * mismatch * intensity[nb] * slope * face_area;
}

static void ldg_math_phys_field_edge_cum(const double *phase, uint64_t center, uint64_t opp, double spacing, double refract_center, double *denom)
{
    double slope = (phase[center] - phase[opp]) / (spacing * refract_center);
    if (slope > 1.0) { slope = 1.0; }

    if (slope < 0.0) { slope = 0.0; }

    *denom += slope;
}

uint32_t ldg_math_phys_field_em_phase_solve(double *phase, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double tol, uint64_t pass_max, uint64_t *out)
{
    const double *row = 0x0;
    double vacuum_norm = 0.0;
    double refract = 0.0;
    double path_cost = 0.0;
    double val_x = 0.0;
    double val_y = 0.0;
    double val_z = 0.0;
    double low = 0.0;
    double mid = 0.0;
    double high = 0.0;
    double swap_val = 0.0;
    double candidate = 0.0;
    double sum_three = 0.0;
    double disc = 0.0;
    double diff = 0.0;
    double delta_max = 0.0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t pos_x = 0;
    uint64_t pos_y = 0;
    uint64_t pos_z = 0;
    uint64_t rev_x = 0;
    uint64_t rev_y = 0;
    uint64_t rev_z = 0;
    uint64_t sweep_idx = 0;
    uint64_t pass_idx = 0;
    if (LDG_UNLIKELY(!phase)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(nx < 2 || ny < 2 || nz < 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(tol) || tol < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(pass_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    vacuum_norm = LDG_MATH_PHYS_VACUUM_PERMITTIVITY * LDG_MATH_PHYS_VACUUM_PERMEABILITY;
    plane = nx * ny;
    for (pass_idx = 0; pass_idx < pass_max; pass_idx++)
    {
        delta_max = 0.0;
        for (sweep_idx = 0; sweep_idx < 8; sweep_idx++)
        {
            rev_x = sweep_idx & 1;
            rev_y = (sweep_idx >> 1) & 1;
            rev_z = (sweep_idx >> 2) & 1;
            for (idx_z = 0; idx_z < nz; idx_z++)
            {
                pos_z = (rev_z == 1) ? (nz - 1 - idx_z) : idx_z;
                for (idx_y = 0; idx_y < ny; idx_y++)
                {
                    pos_y = (rev_y == 1) ? (ny - 1 - idx_y) : idx_y;
                    for (idx_x = 0; idx_x < nx; idx_x++)
                    {
                        pos_x = (rev_x == 1) ? (nx - 1 - idx_x) : idx_x;
                        center = (pos_z * ny + pos_y) * nx + pos_x;
                        val_x = LDG_MATH_PHYS_EM_PHASE_FAR;
                        if (pos_x > 0) { val_x = phase[center - 1]; }

                        if (pos_x + 1 < nx) { val_x = ldg_math_min_get(val_x, phase[center + 1]); }

                        val_y = LDG_MATH_PHYS_EM_PHASE_FAR;
                        if (pos_y > 0) { val_y = phase[center - nx]; }

                        if (pos_y + 1 < ny) { val_y = ldg_math_min_get(val_y, phase[center + nx]); }

                        val_z = LDG_MATH_PHYS_EM_PHASE_FAR;
                        if (pos_z > 0) { val_z = phase[center - plane]; }

                        if (pos_z + 1 < nz) { val_z = ldg_math_min_get(val_z, phase[center + plane]); }

                        low = val_x;
                        mid = val_y;
                        high = val_z;
                        if (low > mid)
                        {
                            swap_val = low;
                            low = mid;
                            mid = swap_val;
                        }

                        if (mid > high)
                        {
                            swap_val = mid;
                            mid = high;
                            high = swap_val;
                        }

                        if (low > mid)
                        {
                            swap_val = low;
                            low = mid;
                            mid = swap_val;
                        }

                        if (low >= LDG_MATH_PHYS_EM_PHASE_FAR) { continue; }

                        row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                        refract = ldg_math_sqrt_get(row[LDG_MATH_PHYS_EM_COEFF_EPSILON] * row[LDG_MATH_PHYS_EM_COEFF_MU] / vacuum_norm);
                        path_cost = refract * spacing;
                        candidate = low + path_cost;
                        if (candidate > mid)
                        {
                            diff = low - mid;
                            disc = 2.0 * path_cost * path_cost - diff * diff;
                            if (disc > 0.0) { candidate = 0.5 * (low + mid + ldg_math_sqrt_get(disc)); }
                        }

                        if (candidate > high)
                        {
                            sum_three = low + mid + high;
                            disc = sum_three * sum_three - 3.0 * (low * low + mid * mid + high * high - path_cost * path_cost);
                            if (disc > 0.0) { candidate = (sum_three + ldg_math_sqrt_get(disc)) / 3.0; }
                        }

                        if (candidate < phase[center])
                        {
                            diff = phase[center] - candidate;
                            delta_max = ldg_math_max_get(delta_max, diff);
                            phase[center] = candidate;
                        }
                    }
                }
            }
        }
        *out = pass_idx + 1;
        if (delta_max <= tol) { return LDG_ERR_AOK; }
    }
    return LDG_ERR_TIMEOUT;
}

uint32_t ldg_math_phys_field_em_intensity_solve(double *intensity, double *reflect, const double *phase, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double tol, uint64_t pass_max, uint64_t *out)
{
    const double *row = 0x0;
    double vacuum_norm = 0.0;
    double refract_center = 0.0;
    double refract_nb = 0.0;
    double alpha = 0.0;
    double influx = 0.0;
    double denom = 0.0;
    double candidate = 0.0;
    double diff = 0.0;
    double delta_max = 0.0;
    double face_area = 0.0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t pos_x = 0;
    uint64_t pos_y = 0;
    uint64_t pos_z = 0;
    uint64_t rev_x = 0;
    uint64_t rev_y = 0;
    uint64_t rev_z = 0;
    uint64_t sweep_idx = 0;
    uint64_t pass_idx = 0;
    uint8_t has_upwind = UINT8_MAX;
    uint8_t converged = LDG_TRUTH_FALSE;
    if (LDG_UNLIKELY(!intensity)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!reflect)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!phase)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(nx < 2 || ny < 2 || nz < 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(tol) || tol < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(pass_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    vacuum_norm = LDG_MATH_PHYS_VACUUM_PERMITTIVITY * LDG_MATH_PHYS_VACUUM_PERMEABILITY;
    plane = nx * ny;
    face_area = (nz == 1) ? spacing : (spacing * spacing);
    for (center = 0; center < nx * ny * nz; center++) { reflect[center] = 0.0; }
    for (pass_idx = 0; pass_idx < pass_max; pass_idx++)
    {
        delta_max = 0.0;
        for (sweep_idx = 0; sweep_idx < 8; sweep_idx++)
        {
            rev_x = sweep_idx & 1;
            rev_y = (sweep_idx >> 1) & 1;
            rev_z = (sweep_idx >> 2) & 1;
            for (idx_z = 0; idx_z < nz; idx_z++)
            {
                pos_z = (rev_z == 1) ? (nz - 1 - idx_z) : idx_z;
                for (idx_y = 0; idx_y < ny; idx_y++)
                {
                    pos_y = (rev_y == 1) ? (ny - 1 - idx_y) : idx_y;
                    for (idx_x = 0; idx_x < nx; idx_x++)
                    {
                        pos_x = (rev_x == 1) ? (nx - 1 - idx_x) : idx_x;
                        center = (pos_z * ny + pos_y) * nx + pos_x;
                        if (phase[center] >= LDG_MATH_PHYS_EM_PHASE_FAR)
                        {
                            intensity[center] = 0.0;
                            continue;
                        }

                        ldg_math_phys_field_refract_get(mtl, coeff, center, vacuum_norm, &refract_center);
                        row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                        alpha = row[LDG_MATH_PHYS_EM_COEFF_SIGMA] * ldg_math_sqrt_get(row[LDG_MATH_PHYS_EM_COEFF_MU] / row[LDG_MATH_PHYS_EM_COEFF_EPSILON]);
                        influx = 0.0;
                        denom = 0.0;
                        has_upwind = LDG_TRUTH_FALSE;
                        if (pos_x > 0)
                        {
                            ldg_math_phys_field_refract_get(mtl, coeff, center - 1, vacuum_norm, &refract_nb);
                            ldg_math_phys_field_face_cum(intensity, phase, center, center - 1, spacing, refract_center, refract_nb, &influx, &denom, &has_upwind);
                        }
                        else { ldg_math_phys_field_edge_cum(phase, center, center + 1, spacing, refract_center, &denom); }

                        if (pos_x + 1 < nx)
                        {
                            ldg_math_phys_field_refract_get(mtl, coeff, center + 1, vacuum_norm, &refract_nb);
                            ldg_math_phys_field_face_cum(intensity, phase, center, center + 1, spacing, refract_center, refract_nb, &influx, &denom, &has_upwind);
                        }
                        else { ldg_math_phys_field_edge_cum(phase, center, center - 1, spacing, refract_center, &denom); }

                        if (pos_y > 0)
                        {
                            ldg_math_phys_field_refract_get(mtl, coeff, center - nx, vacuum_norm, &refract_nb);
                            ldg_math_phys_field_face_cum(intensity, phase, center, center - nx, spacing, refract_center, refract_nb, &influx, &denom, &has_upwind);
                        }
                        else { ldg_math_phys_field_edge_cum(phase, center, center + nx, spacing, refract_center, &denom); }

                        if (pos_y + 1 < ny)
                        {
                            ldg_math_phys_field_refract_get(mtl, coeff, center + nx, vacuum_norm, &refract_nb);
                            ldg_math_phys_field_face_cum(intensity, phase, center, center + nx, spacing, refract_center, refract_nb, &influx, &denom, &has_upwind);
                        }
                        else { ldg_math_phys_field_edge_cum(phase, center, center - nx, spacing, refract_center, &denom); }

                        if (nz > 1)
                        {
                            if (pos_z > 0)
                            {
                                ldg_math_phys_field_refract_get(mtl, coeff, center - plane, vacuum_norm, &refract_nb);
                                ldg_math_phys_field_face_cum(intensity, phase, center, center - plane, spacing, refract_center, refract_nb, &influx, &denom, &has_upwind);
                            }
                            else { ldg_math_phys_field_edge_cum(phase, center, center + plane, spacing, refract_center, &denom); }

                            if (pos_z + 1 < nz)
                            {
                                ldg_math_phys_field_refract_get(mtl, coeff, center + plane, vacuum_norm, &refract_nb);
                                ldg_math_phys_field_face_cum(intensity, phase, center, center + plane, spacing, refract_center, refract_nb, &influx, &denom, &has_upwind);
                            }
                            else { ldg_math_phys_field_edge_cum(phase, center, center - plane, spacing, refract_center, &denom); }
                        }

                        if (has_upwind != LDG_TRUTH_TRUE) { continue; }

                        denom += alpha * spacing;
                        if (denom <= 0.0) { continue; }

                        candidate = influx / denom;
                        diff = ldg_math_abs_get(intensity[center] - candidate);
                        delta_max = ldg_math_max_get(delta_max, diff);
                        intensity[center] = candidate;
                    }
                }
            }
        }
        *out = pass_idx + 1;
        if (delta_max <= tol)
        {
            converged = LDG_TRUTH_TRUE;
            break;
        }
    }
    if (converged != LDG_TRUTH_TRUE) { return LDG_ERR_TIMEOUT; }

    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                if (phase[center] >= LDG_MATH_PHYS_EM_PHASE_FAR) { continue; }

                ldg_math_phys_field_refract_get(mtl, coeff, center, vacuum_norm, &refract_center);
                if (idx_x > 0)
                {
                    ldg_math_phys_field_refract_get(mtl, coeff, center - 1, vacuum_norm, &refract_nb);
                    ldg_math_phys_field_reflect_cum(intensity, phase, center, center - 1, spacing, refract_center, refract_nb, face_area, reflect);
                }

                if (idx_x + 1 < nx)
                {
                    ldg_math_phys_field_refract_get(mtl, coeff, center + 1, vacuum_norm, &refract_nb);
                    ldg_math_phys_field_reflect_cum(intensity, phase, center, center + 1, spacing, refract_center, refract_nb, face_area, reflect);
                }

                if (idx_y > 0)
                {
                    ldg_math_phys_field_refract_get(mtl, coeff, center - nx, vacuum_norm, &refract_nb);
                    ldg_math_phys_field_reflect_cum(intensity, phase, center, center - nx, spacing, refract_center, refract_nb, face_area, reflect);
                }

                if (idx_y + 1 < ny)
                {
                    ldg_math_phys_field_refract_get(mtl, coeff, center + nx, vacuum_norm, &refract_nb);
                    ldg_math_phys_field_reflect_cum(intensity, phase, center, center + nx, spacing, refract_center, refract_nb, face_area, reflect);
                }

                if (nz > 1)
                {
                    if (idx_z > 0)
                    {
                        ldg_math_phys_field_refract_get(mtl, coeff, center - plane, vacuum_norm, &refract_nb);
                        ldg_math_phys_field_reflect_cum(intensity, phase, center, center - plane, spacing, refract_center, refract_nb, face_area, reflect);
                    }

                    if (idx_z + 1 < nz)
                    {
                        ldg_math_phys_field_refract_get(mtl, coeff, center + plane, vacuum_norm, &refract_nb);
                        ldg_math_phys_field_reflect_cum(intensity, phase, center, center + plane, spacing, refract_center, refract_nb, face_area, reflect);
                    }
                }
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_em_radiance_solve(double *radiance, double *flux, const double *emission, const uint16_t *mtl, const double *coeff, const double *scatter, const double *ordinate, uint64_t ordinate_cunt, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double tol, uint64_t pass_max, uint64_t *out)
{
    const double *row = 0x0;
    double *slab = 0x0;
    double dir_x = 0.0;
    double dir_y = 0.0;
    double dir_z = 0.0;
    double abs_x = 0.0;
    double abs_y = 0.0;
    double abs_z = 0.0;
    double alpha = 0.0;
    double beta = 0.0;
    double numer = 0.0;
    double denom = 0.0;
    double flux_val = 0.0;
    double diff = 0.0;
    double delta_max = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t pos_x = 0;
    uint64_t pos_y = 0;
    uint64_t pos_z = 0;
    uint64_t ord_idx = 0;
    uint64_t pass_idx = 0;
    if (LDG_UNLIKELY(!radiance)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!flux)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!emission)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!scatter)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ordinate)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(ordinate_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(nx < 2 || ny < 2 || nz < 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(tol) || tol < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(pass_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny * nz;
    plane = nx * ny;
    for (center = 0; center < cell_cunt; center++) { flux[center] = 0.0; }
    for (pass_idx = 0; pass_idx < pass_max; pass_idx++)
    {
        for (ord_idx = 0; ord_idx < ordinate_cunt; ord_idx++)
        {
            slab = radiance + ord_idx * cell_cunt;
            dir_x = ordinate[ord_idx * 4];
            dir_y = ordinate[ord_idx * 4 + 1];
            dir_z = ordinate[ord_idx * 4 + 2];
            abs_x = ldg_math_abs_get(dir_x) / spacing;
            abs_y = ldg_math_abs_get(dir_y) / spacing;
            abs_z = ldg_math_abs_get(dir_z) / spacing;
            for (idx_z = 0; idx_z < nz; idx_z++)
            {
                pos_z = (dir_z < 0.0) ? (nz - 1 - idx_z) : idx_z;
                for (idx_y = 0; idx_y < ny; idx_y++)
                {
                    pos_y = (dir_y < 0.0) ? (ny - 1 - idx_y) : idx_y;
                    for (idx_x = 0; idx_x < nx; idx_x++)
                    {
                        pos_x = (dir_x < 0.0) ? (nx - 1 - idx_x) : idx_x;
                        center = (pos_z * ny + pos_y) * nx + pos_x;
                        row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                        alpha = row[LDG_MATH_PHYS_EM_COEFF_SIGMA] * ldg_math_sqrt_get(row[LDG_MATH_PHYS_EM_COEFF_MU] / row[LDG_MATH_PHYS_EM_COEFF_EPSILON]);
                        beta = scatter[mtl[center]];
                        numer = emission[center] + beta * flux[center] / (4.0 * LDG_MATH_PI);
                        denom = alpha + beta;
                        if (abs_x > 0.0)
                        {
                            denom += abs_x;
                            if (dir_x > 0.0 && pos_x > 0) { numer += abs_x * slab[center - 1]; }

                            if (dir_x < 0.0 && pos_x + 1 < nx) { numer += abs_x * slab[center + 1]; }
                        }

                        if (abs_y > 0.0)
                        {
                            denom += abs_y;
                            if (dir_y > 0.0 && pos_y > 0) { numer += abs_y * slab[center - nx]; }

                            if (dir_y < 0.0 && pos_y + 1 < ny) { numer += abs_y * slab[center + nx]; }
                        }

                        if (abs_z > 0.0)
                        {
                            denom += abs_z;
                            if (dir_z > 0.0 && pos_z > 0) { numer += abs_z * slab[center - plane]; }

                            if (dir_z < 0.0 && pos_z + 1 < nz) { numer += abs_z * slab[center + plane]; }
                        }

                        if (denom <= 0.0) { continue; }

                        slab[center] = numer / denom;
                    }
                }
            }
        }
        delta_max = 0.0;
        for (center = 0; center < cell_cunt; center++)
        {
            flux_val = 0.0;
            for (ord_idx = 0; ord_idx < ordinate_cunt; ord_idx++) { flux_val += ordinate[ord_idx * 4 + 3] * radiance[ord_idx * cell_cunt + center]; }
            diff = ldg_math_abs_get(flux_val - flux[center]);
            delta_max = ldg_math_max_get(delta_max, diff);
            flux[center] = flux_val;
        }
        *out = pass_idx + 1;
        if (delta_max <= tol) { return LDG_ERR_AOK; }
    }
    return LDG_ERR_TIMEOUT;
}

uint32_t ldg_math_phys_field_em_radiance_conserved_get(const double *radiance, const double *emission, const uint16_t *mtl, const double *coeff, const double *ordinate, uint64_t ordinate_cunt, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double *out_absorbed, double *out_escaped, double *out_emitted)
{
    const double *row = 0x0;
    double alpha = 0.0;
    double flux_val = 0.0;
    double dir_val = 0.0;
    double edge_sum = 0.0;
    double cell_vol = 0.0;
    double face_area = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t ord_idx = 0;
    if (LDG_UNLIKELY(!radiance)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!emission)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ordinate)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_absorbed)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_escaped)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_emitted)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_absorbed = 0.0;
    *out_escaped = 0.0;
    *out_emitted = 0.0;
    if (LDG_UNLIKELY(ordinate_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(nx < 2 || ny < 2 || nz < 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny * nz;
    cell_vol = (nz == 1) ? (spacing * spacing) : (spacing * spacing * spacing);
    face_area = (nz == 1) ? spacing : (spacing * spacing);
    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                alpha = row[LDG_MATH_PHYS_EM_COEFF_SIGMA] * ldg_math_sqrt_get(row[LDG_MATH_PHYS_EM_COEFF_MU] / row[LDG_MATH_PHYS_EM_COEFF_EPSILON]);
                flux_val = 0.0;
                for (ord_idx = 0; ord_idx < ordinate_cunt; ord_idx++) { flux_val += ordinate[ord_idx * 4 + 3] * radiance[ord_idx * cell_cunt + center]; }
                *out_absorbed += alpha * flux_val * cell_vol;
                *out_emitted += emission[center] * 4.0 * LDG_MATH_PI * cell_vol;
                edge_sum = 0.0;
                for (ord_idx = 0; ord_idx < ordinate_cunt; ord_idx++)
                {
                    dir_val = ordinate[ord_idx * 4];
                    if (idx_x == 0 && dir_val < 0.0) { edge_sum += ordinate[ord_idx * 4 + 3] * radiance[ord_idx * cell_cunt + center] * (0.0 - dir_val); }

                    if (idx_x + 1 == nx && dir_val > 0.0) { edge_sum += ordinate[ord_idx * 4 + 3] * radiance[ord_idx * cell_cunt + center] * dir_val; }

                    dir_val = ordinate[ord_idx * 4 + 1];
                    if (idx_y == 0 && dir_val < 0.0) { edge_sum += ordinate[ord_idx * 4 + 3] * radiance[ord_idx * cell_cunt + center] * (0.0 - dir_val); }

                    if (idx_y + 1 == ny && dir_val > 0.0) { edge_sum += ordinate[ord_idx * 4 + 3] * radiance[ord_idx * cell_cunt + center] * dir_val; }

                    if (nz > 1)
                    {
                        dir_val = ordinate[ord_idx * 4 + 2];
                        if (idx_z == 0 && dir_val < 0.0) { edge_sum += ordinate[ord_idx * 4 + 3] * radiance[ord_idx * cell_cunt + center] * (0.0 - dir_val); }

                        if (idx_z + 1 == nz && dir_val > 0.0) { edge_sum += ordinate[ord_idx * 4 + 3] * radiance[ord_idx * cell_cunt + center] * dir_val; }
                    }
                }
                *out_escaped += edge_sum * face_area;
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_em_intensity_conserved_get(const double *intensity, const double *reflect, const double *phase, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double *out_absorbed, double *out_escaped, double *out_reflected)
{
    const double *row = 0x0;
    double vacuum_norm = 0.0;
    double refract_center = 0.0;
    double alpha = 0.0;
    double edge_sum = 0.0;
    double cell_vol = 0.0;
    double face_area = 0.0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    if (LDG_UNLIKELY(!intensity)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!reflect)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!phase)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_absorbed)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_escaped)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_reflected)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_absorbed = 0.0;
    *out_escaped = 0.0;
    *out_reflected = 0.0;
    if (LDG_UNLIKELY(nx < 2 || ny < 2 || nz < 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    vacuum_norm = LDG_MATH_PHYS_VACUUM_PERMITTIVITY * LDG_MATH_PHYS_VACUUM_PERMEABILITY;
    plane = nx * ny;
    cell_vol = (nz == 1) ? (spacing * spacing) : (spacing * spacing * spacing);
    face_area = (nz == 1) ? spacing : (spacing * spacing);
    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                if (phase[center] >= LDG_MATH_PHYS_EM_PHASE_FAR) { continue; }

                *out_reflected += reflect[center];
                ldg_math_phys_field_refract_get(mtl, coeff, center, vacuum_norm, &refract_center);
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                alpha = row[LDG_MATH_PHYS_EM_COEFF_SIGMA] * ldg_math_sqrt_get(row[LDG_MATH_PHYS_EM_COEFF_MU] / row[LDG_MATH_PHYS_EM_COEFF_EPSILON]);
                *out_absorbed += alpha * intensity[center] * cell_vol;
                edge_sum = 0.0;
                if (idx_x == 0) { ldg_math_phys_field_edge_cum(phase, center, center + 1, spacing, refract_center, &edge_sum); }

                if (idx_x + 1 == nx) { ldg_math_phys_field_edge_cum(phase, center, center - 1, spacing, refract_center, &edge_sum); }

                if (idx_y == 0) { ldg_math_phys_field_edge_cum(phase, center, center + nx, spacing, refract_center, &edge_sum); }

                if (idx_y + 1 == ny) { ldg_math_phys_field_edge_cum(phase, center, center - nx, spacing, refract_center, &edge_sum); }

                if (nz > 1)
                {
                    if (idx_z == 0) { ldg_math_phys_field_edge_cum(phase, center, center + plane, spacing, refract_center, &edge_sum); }

                    if (idx_z + 1 == nz) { ldg_math_phys_field_edge_cum(phase, center, center - plane, spacing, refract_center, &edge_sum); }
                }

                *out_escaped += intensity[center] * edge_sum * face_area;
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_em_bounce_solve(double *phase_next, double *intensity_next, double *reflect_next, const double *reflect, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double tol, uint64_t pass_max, uint64_t *out)
{
    double vacuum_norm = 0.0;
    double refract_center = 0.0;
    double refract_nb = 0.0;
    double refract_face = 0.0;
    double slope = 0.0;
    double weight_sum = 0.0;
    double face_area = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t face_idx = 0;
    uint64_t nb = 0;
    uint64_t src_cunt = 0;
    uint32_t err = 0;
    uint8_t nb_valid = 0;
    if (LDG_UNLIKELY(!phase_next)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!intensity_next)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!reflect_next)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!reflect)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(nx < 2 || ny < 2 || nz < 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny * nz;
    plane = nx * ny;
    face_area = (nz == 1) ? spacing : (spacing * spacing);
    vacuum_norm = LDG_MATH_PHYS_VACUUM_PERMITTIVITY * LDG_MATH_PHYS_VACUUM_PERMEABILITY;
    for (center = 0; center < cell_cunt; center++)
    {
        intensity_next[center] = 0.0;
        if (reflect[center] > 0.0)
        {
            phase_next[center] = 0.0;
            src_cunt++;
        }
        else { phase_next[center] = LDG_MATH_PHYS_EM_PHASE_FAR; }
    }
    if (src_cunt == 0) { return LDG_ERR_EMPTY; }

    err = ldg_math_phys_field_em_phase_solve(phase_next, mtl, coeff, nx, ny, nz, spacing, tol, pass_max, out);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                if (reflect[center] <= 0.0) { continue; }

                ldg_math_phys_field_refract_get(mtl, coeff, center, vacuum_norm, &refract_center);
                weight_sum = 0.0;
                for (face_idx = 0; face_idx < 6; face_idx++)
                {
                    nb_valid = 0;
                    if (face_idx == 0 && idx_x > 0)
                    {
                        nb = center - 1;
                        nb_valid = 1;
                    }

                    if (face_idx == 1 && idx_x + 1 < nx)
                    {
                        nb = center + 1;
                        nb_valid = 1;
                    }

                    if (face_idx == 2 && idx_y > 0)
                    {
                        nb = center - nx;
                        nb_valid = 1;
                    }

                    if (face_idx == 3 && idx_y + 1 < ny)
                    {
                        nb = center + nx;
                        nb_valid = 1;
                    }

                    if (face_idx == 4 && nz > 1 && idx_z > 0)
                    {
                        nb = center - plane;
                        nb_valid = 1;
                    }

                    if (face_idx == 5 && nz > 1 && idx_z + 1 < nz)
                    {
                        nb = center + plane;
                        nb_valid = 1;
                    }

                    if (nb_valid == 0) { continue; }

                    if (phase_next[nb] <= phase_next[center]) { continue; }

                    ldg_math_phys_field_refract_get(mtl, coeff, nb, vacuum_norm, &refract_nb);
                    refract_face = 0.5 * (refract_center + refract_nb);
                    slope = (phase_next[nb] - phase_next[center]) / (spacing * refract_face);
                    if (slope > 1.0) { slope = 1.0; }

                    weight_sum += slope;
                }
                if (weight_sum > 0.0) { intensity_next[center] = reflect[center] / (weight_sum * face_area); }
            }
        }
    }
    err = ldg_math_phys_field_em_intensity_solve(intensity_next, reflect_next, phase_next, mtl, coeff, nx, ny, nz, spacing, tol, pass_max, out);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (center = 0; center < cell_cunt; center++) { if (reflect[center] > 0.0 && intensity_next[center] < LDG_MATH_EPSILON) { reflect_next[center] += reflect[center]; } }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_em_eval_sel(double spacing, double lambda, double wave_err_max, double ray_err_max, uint32_t *out)
{
    double wave_err = 0.0;
    double ray_err = 0.0;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_PHYS_EM_EVAL_WAVE;
    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(lambda) || lambda <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(wave_err_max) || wave_err_max <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(ray_err_max) || ray_err_max <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    wave_err = (LDG_MATH_PI * LDG_MATH_PI / 6.0) * (spacing / lambda) * (spacing / lambda);
    ray_err = lambda / (LDG_MATH_TAU * spacing);
    if (wave_err <= wave_err_max)
    {
        *out = LDG_MATH_PHYS_EM_EVAL_WAVE;
        return LDG_ERR_AOK;
    }

    if (ray_err <= ray_err_max)
    {
        *out = LDG_MATH_PHYS_EM_EVAL_RAY;
        return LDG_ERR_AOK;
    }

    return LDG_ERR_UNSUPPORTED;
}
