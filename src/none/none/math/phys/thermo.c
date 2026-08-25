
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>

#include "thermo.h"

static double ldg_math_phys_thermo_cell_heat_cap_get(const uint16_t *mtl, const double *coeff, uint64_t cell)
{
    const double *row = coeff + (uint64_t)mtl[cell] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE;
    return row[LDG_MATH_PHYS_THERMO_COEFF_RHO] * row[LDG_MATH_PHYS_THERMO_COEFF_CAP];
}

static double ldg_math_phys_thermo_face_k_get(const uint16_t *mtl, const double *coeff, uint64_t cell_a, uint64_t cell_b)
{
    double k_a = coeff[(uint64_t)mtl[cell_a] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE + LDG_MATH_PHYS_THERMO_COEFF_K];
    double k_b = coeff[(uint64_t)mtl[cell_b] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE + LDG_MATH_PHYS_THERMO_COEFF_K];
    if (k_a + k_b <= 0.0) { return 0.0; }

    return 2.0 * k_a * k_b / (k_a + k_b);
}

uint32_t ldg_math_phys_thermo_conduct_advance(double *temp_next, const double *temp, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double dt)
{
    const double *row = 0x0;
    double flux_sum = 0.0;
    double heat_cap = 0.0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    if (LDG_UNLIKELY(!temp_next)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!temp)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 3 || ny < 3 || nz < 3)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(dt) || dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    plane = nx * ny;
    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE;
                heat_cap = row[LDG_MATH_PHYS_THERMO_COEFF_RHO] * row[LDG_MATH_PHYS_THERMO_COEFF_CAP];
                if (heat_cap <= 0.0)
                {
                    temp_next[center] = temp[center];
                    continue;
                }

                flux_sum = 0.0;
                if (idx_x > 0) { flux_sum += ldg_math_phys_thermo_face_k_get(mtl, coeff, center, center - 1) * (temp[center - 1] - temp[center]); }

                if (idx_x + 1 < nx) { flux_sum += ldg_math_phys_thermo_face_k_get(mtl, coeff, center, center + 1) * (temp[center + 1] - temp[center]); }

                if (idx_y > 0) { flux_sum += ldg_math_phys_thermo_face_k_get(mtl, coeff, center, center - nx) * (temp[center - nx] - temp[center]); }

                if (idx_y + 1 < ny) { flux_sum += ldg_math_phys_thermo_face_k_get(mtl, coeff, center, center + nx) * (temp[center + nx] - temp[center]); }

                if (idx_z > 0) { flux_sum += ldg_math_phys_thermo_face_k_get(mtl, coeff, center, center - plane) * (temp[center - plane] - temp[center]); }

                if (idx_z + 1 < nz) { flux_sum += ldg_math_phys_thermo_face_k_get(mtl, coeff, center, center + plane) * (temp[center + plane] - temp[center]); }

                temp_next[center] = temp[center] + dt * flux_sum / (heat_cap * spacing * spacing);
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_thermo_convect_advance(double *temp_next, const double *temp, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double dt, double ambient, double *out_escaped)
{
    const double *row = 0x0;
    double heat_cap = 0.0;
    double convection = 0.0;
    double loss = 0.0;
    double escaped_sum = 0.0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t face_cunt = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    if (LDG_UNLIKELY(!temp_next)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!temp)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_escaped)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_escaped = 0.0;
    if (LDG_UNLIKELY(nx < 3 || ny < 3 || nz < 3)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(dt) || dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(ambient) || ambient < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    plane = nx * ny;
    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE;
                heat_cap = row[LDG_MATH_PHYS_THERMO_COEFF_RHO] * row[LDG_MATH_PHYS_THERMO_COEFF_CAP];
                if (heat_cap <= 0.0) { continue; }

                convection = row[LDG_MATH_PHYS_THERMO_COEFF_CONVECTION];
                if (convection <= 0.0) { continue; }

                face_cunt = 0;
                if (idx_x == 0 || ldg_math_phys_thermo_cell_heat_cap_get(mtl, coeff, center - 1) <= 0.0) { face_cunt++; }

                if (idx_x + 1 == nx || ldg_math_phys_thermo_cell_heat_cap_get(mtl, coeff, center + 1) <= 0.0) { face_cunt++; }

                if (idx_y == 0 || ldg_math_phys_thermo_cell_heat_cap_get(mtl, coeff, center - nx) <= 0.0) { face_cunt++; }

                if (idx_y + 1 == ny || ldg_math_phys_thermo_cell_heat_cap_get(mtl, coeff, center + nx) <= 0.0) { face_cunt++; }

                if (idx_z == 0 || ldg_math_phys_thermo_cell_heat_cap_get(mtl, coeff, center - plane) <= 0.0) { face_cunt++; }

                if (idx_z + 1 == nz || ldg_math_phys_thermo_cell_heat_cap_get(mtl, coeff, center + plane) <= 0.0) { face_cunt++; }

                if (face_cunt == 0) { continue; }

                loss = convection * (temp[center] - ambient) * spacing * spacing * (double)face_cunt * dt;
                temp_next[center] -= loss / (heat_cap * spacing * spacing * spacing);
                escaped_sum += loss;
            }
        }
    }
    *out_escaped = escaped_sum;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_thermo_pwr_advance(double *temp_next, const uint16_t *mtl, const double *coeff, const double *pwr, uint64_t cell_cunt, double spacing, double dt, double *out_injected)
{
    double heat_cap = 0.0;
    double cell_vol = 0.0;
    double injected_sum = 0.0;
    uint64_t cell = 0;
    if (LDG_UNLIKELY(!temp_next)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pwr)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_injected)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_injected = 0.0;
    if (LDG_UNLIKELY(cell_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(dt) || dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_vol = spacing * spacing * spacing;
    for (cell = 0; cell < cell_cunt; cell++)
    {
        if (!(ldg_math_abs_get(pwr[cell]) > 0.0)) { continue; }

        heat_cap = ldg_math_phys_thermo_cell_heat_cap_get(mtl, coeff, cell);
        if (heat_cap <= 0.0) { continue; }

        temp_next[cell] += pwr[cell] * dt / heat_cap;
        injected_sum += pwr[cell] * cell_vol * dt;
    }
    *out_injected = injected_sum;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_thermo_emission_build(double *emission, const double *temp, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing)
{
    const double *row = 0x0;
    double emit_val = 0.0;
    double temp_sq = 0.0;
    double exposed_cunt = 0.0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    if (LDG_UNLIKELY(!emission)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!temp)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 3 || ny < 3 || nz < 3)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    plane = nx * ny;
    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                center = (idx_z * ny + idx_y) * nx + idx_x;
                emission[center] = 0.0;
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE;
                if (row[LDG_MATH_PHYS_THERMO_COEFF_EMIT] <= 0.0) { continue; }

                exposed_cunt = 0.0;
                if (idx_x > 0 && coeff[(uint64_t)mtl[center - 1] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE + LDG_MATH_PHYS_THERMO_COEFF_ABSORB] <= 0.0) { exposed_cunt += 1.0; }

                if (idx_x + 1 < nx && coeff[(uint64_t)mtl[center + 1] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE + LDG_MATH_PHYS_THERMO_COEFF_ABSORB] <= 0.0) { exposed_cunt += 1.0; }

                if (idx_y > 0 && coeff[(uint64_t)mtl[center - nx] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE + LDG_MATH_PHYS_THERMO_COEFF_ABSORB] <= 0.0) { exposed_cunt += 1.0; }

                if (idx_y + 1 < ny && coeff[(uint64_t)mtl[center + nx] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE + LDG_MATH_PHYS_THERMO_COEFF_ABSORB] <= 0.0) { exposed_cunt += 1.0; }

                if (idx_z > 0 && coeff[(uint64_t)mtl[center - plane] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE + LDG_MATH_PHYS_THERMO_COEFF_ABSORB] <= 0.0) { exposed_cunt += 1.0; }

                if (idx_z + 1 < nz && coeff[(uint64_t)mtl[center + plane] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE + LDG_MATH_PHYS_THERMO_COEFF_ABSORB] <= 0.0) { exposed_cunt += 1.0; }

                if (exposed_cunt <= 0.0) { continue; }

                temp_sq = temp[center] * temp[center];
                emit_val = row[LDG_MATH_PHYS_THERMO_COEFF_EMIT] * LDG_MATH_PHYS_STEFAN_BOLTZMANN * temp_sq * temp_sq;
                emission[center] = emit_val * exposed_cunt / (4.0 * LDG_MATH_PI * spacing);
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_thermo_xchg_advance(double *temp, const double *flux, const double *emission, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double dt)
{
    const double *row = 0x0;
    double heat_cap = 0.0;
    double absorbed = 0.0;
    double emitted = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t center = 0;
    if (LDG_UNLIKELY(!temp)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!flux)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!emission)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mtl)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!coeff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 3 || ny < 3 || nz < 3)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(dt) || dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cell_cunt = nx * ny * nz;
    for (center = 0; center < cell_cunt; center++)
    {
        row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE;
        heat_cap = row[LDG_MATH_PHYS_THERMO_COEFF_RHO] * row[LDG_MATH_PHYS_THERMO_COEFF_CAP];
        if (heat_cap <= 0.0) { continue; }

        absorbed = row[LDG_MATH_PHYS_THERMO_COEFF_ABSORB] * flux[center];
        emitted = 4.0 * LDG_MATH_PI * emission[center];
        temp[center] += dt * (absorbed - emitted) / heat_cap;
        if (temp[center] < 0.0) { temp[center] = 0.0; }
    }
    return LDG_ERR_AOK;
}
