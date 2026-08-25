
#include <dangling/core/err.h>
#include <dangling/math/math.h>
#include <dangling/core/macros.h>

#include "fluid.h"

static double ldg_math_phys_fluid_grid_sample(const double *grid, uint64_t dim_x, uint64_t dim_y, uint64_t dim_z, double coord_x, double coord_y, double coord_z)
{
    double frac_x = 0.0;
    double frac_y = 0.0;
    double frac_z = 0.0;
    double acc = 0.0;
    uint64_t base_x = 0;
    uint64_t base_y = 0;
    uint64_t base_z = 0;
    uint64_t corner_x = 0;
    uint64_t corner_y = 0;
    uint64_t corner_z = 0;
    uint64_t plane = dim_x * dim_y;
    if (coord_x < 0.0) { coord_x = 0.0; }

    if (coord_y < 0.0) { coord_y = 0.0; }

    if (coord_z < 0.0) { coord_z = 0.0; }

    if (coord_x > (double)(dim_x - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN) { coord_x = (double)(dim_x - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN; }

    if (coord_y > (double)(dim_y - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN) { coord_y = (double)(dim_y - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN; }

    if (coord_z > (double)(dim_z - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN) { coord_z = (double)(dim_z - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN; }

    base_x = (uint64_t)coord_x;
    base_y = (uint64_t)coord_y;
    base_z = (uint64_t)coord_z;
    frac_x = coord_x - (double)base_x;
    frac_y = coord_y - (double)base_y;
    frac_z = coord_z - (double)base_z;
    for (corner_z = 0; corner_z < 2; corner_z++) { for (corner_y = 0; corner_y < 2; corner_y++) { for (corner_x = 0; corner_x < 2; corner_x++) { acc += grid[(base_z + corner_z) * plane + (base_y + corner_y) * dim_x + (base_x + corner_x)] * ((corner_x == 1) ? frac_x : (1.0 - frac_x)) * ((corner_y == 1) ? frac_y : (1.0 - frac_y)) * ((corner_z == 1) ? frac_z : (1.0 - frac_z)); } } }
    return acc;
}

static void ldg_math_phys_fluid_grid_splat(double *grid, double *weight, uint64_t dim_x, uint64_t dim_y, uint64_t dim_z, double coord_x, double coord_y, double coord_z, double val)
{
    double frac_x = 0.0;
    double frac_y = 0.0;
    double frac_z = 0.0;
    double corner_weight = 0.0;
    uint64_t base_x = 0;
    uint64_t base_y = 0;
    uint64_t base_z = 0;
    uint64_t corner_x = 0;
    uint64_t corner_y = 0;
    uint64_t corner_z = 0;
    uint64_t slot = 0;
    uint64_t plane = dim_x * dim_y;
    if (coord_x < 0.0) { coord_x = 0.0; }

    if (coord_y < 0.0) { coord_y = 0.0; }

    if (coord_z < 0.0) { coord_z = 0.0; }

    if (coord_x > (double)(dim_x - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN) { coord_x = (double)(dim_x - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN; }

    if (coord_y > (double)(dim_y - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN) { coord_y = (double)(dim_y - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN; }

    if (coord_z > (double)(dim_z - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN) { coord_z = (double)(dim_z - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN; }

    base_x = (uint64_t)coord_x;
    base_y = (uint64_t)coord_y;
    base_z = (uint64_t)coord_z;
    frac_x = coord_x - (double)base_x;
    frac_y = coord_y - (double)base_y;
    frac_z = coord_z - (double)base_z;
    for (corner_z = 0; corner_z < 2; corner_z++) { for (corner_y = 0; corner_y < 2; corner_y++) { for (corner_x = 0; corner_x < 2; corner_x++)
            {
                corner_weight = ((corner_x == 1) ? frac_x : (1.0 - frac_x)) * ((corner_y == 1) ? frac_y : (1.0 - frac_y)) * ((corner_z == 1) ? frac_z : (1.0 - frac_z));
                slot = (base_z + corner_z) * plane + (base_y + corner_y) * dim_x + (base_x + corner_x);
                grid[slot] += val * corner_weight;
                weight[slot] += corner_weight;
            }
        } }
}

uint32_t ldg_math_phys_fluid_p2g_xfer(const double *particle, uint64_t particle_cunt, double *vel_x, double *vel_y, double *vel_z, double *weight_x, double *weight_y, double *weight_z, uint16_t *mask, uint64_t nx, uint64_t ny, uint64_t nz, double spacing)
{
    const double *row = 0x0;
    double inv_spacing = 0.0;
    double coord_x = 0.0;
    double coord_y = 0.0;
    double coord_z = 0.0;
    uint64_t face_x_cunt = 0;
    uint64_t face_y_cunt = 0;
    uint64_t face_z_cunt = 0;
    uint64_t cell_cunt = 0;
    uint64_t cell_x = 0;
    uint64_t cell_y = 0;
    uint64_t cell_z = 0;
    uint64_t cell = 0;
    uint64_t idx = 0;
    if (LDG_UNLIKELY(!particle)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!vel_x || !vel_y || !vel_z)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!weight_x || !weight_y || !weight_z)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mask)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < LDG_MATH_PHYS_FIELD_DIM_MIN || ny < LDG_MATH_PHYS_FIELD_DIM_MIN || nz < LDG_MATH_PHYS_FIELD_DIM_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv_spacing = 1.0 / spacing;
    face_x_cunt = (nx + 1) * ny * nz;
    face_y_cunt = nx * (ny + 1) * nz;
    face_z_cunt = nx * ny * (nz + 1);
    cell_cunt = nx * ny * nz;
    for (idx = 0; idx < face_x_cunt; idx++)
    {
        vel_x[idx] = 0.0;
        weight_x[idx] = 0.0;
    }
    for (idx = 0; idx < face_y_cunt; idx++)
    {
        vel_y[idx] = 0.0;
        weight_y[idx] = 0.0;
    }
    for (idx = 0; idx < face_z_cunt; idx++)
    {
        vel_z[idx] = 0.0;
        weight_z[idx] = 0.0;
    }
    for (cell = 0; cell < cell_cunt; cell++) { if (mask[cell] == LDG_MATH_PHYS_FLUID_CELL_WATER) { mask[cell] = LDG_MATH_PHYS_FLUID_CELL_AIR; } }
    for (idx = 0; idx < particle_cunt; idx++)
    {
        row = particle + idx * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE;
        if (LDG_UNLIKELY(!ldg_math_finite_is(row[0]) || !ldg_math_finite_is(row[1]) || !ldg_math_finite_is(row[2]))) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(row[3]) || !ldg_math_finite_is(row[4]) || !ldg_math_finite_is(row[5]))) { return LDG_ERR_FUNC_ARG_INVALID; }

        coord_x = row[0] * inv_spacing;
        coord_y = row[1] * inv_spacing;
        coord_z = row[2] * inv_spacing;
        if (coord_x < 0.0) { coord_x = 0.0; }

        if (coord_y < 0.0) { coord_y = 0.0; }

        if (coord_z < 0.0) { coord_z = 0.0; }

        if (coord_x > (double)(nx - 1)) { coord_x = (double)(nx - 1); }

        if (coord_y > (double)(ny - 1)) { coord_y = (double)(ny - 1); }

        if (coord_z > (double)(nz - 1)) { coord_z = (double)(nz - 1); }

        cell_x = (uint64_t)coord_x;
        cell_y = (uint64_t)coord_y;
        cell_z = (uint64_t)coord_z;
        cell = (cell_z * ny + cell_y) * nx + cell_x;
        if (mask[cell] == LDG_MATH_PHYS_FLUID_CELL_AIR) { mask[cell] = LDG_MATH_PHYS_FLUID_CELL_WATER; }

        ldg_math_phys_fluid_grid_splat(vel_x, weight_x, nx + 1, ny, nz, row[0] * inv_spacing, row[1] * inv_spacing - 0.5, row[2] * inv_spacing - 0.5, row[3]);
        ldg_math_phys_fluid_grid_splat(vel_y, weight_y, nx, ny + 1, nz, row[0] * inv_spacing - 0.5, row[1] * inv_spacing, row[2] * inv_spacing - 0.5, row[4]);
        ldg_math_phys_fluid_grid_splat(vel_z, weight_z, nx, ny, nz + 1, row[0] * inv_spacing - 0.5, row[1] * inv_spacing - 0.5, row[2] * inv_spacing, row[5]);
    }
    for (idx = 0; idx < face_x_cunt; idx++) { if (weight_x[idx] > 0.0) { vel_x[idx] /= weight_x[idx]; } }
    for (idx = 0; idx < face_y_cunt; idx++) { if (weight_y[idx] > 0.0) { vel_y[idx] /= weight_y[idx]; } }
    for (idx = 0; idx < face_z_cunt; idx++) { if (weight_z[idx] > 0.0) { vel_z[idx] /= weight_z[idx]; } }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_fluid_grid_advance(double *vel_x, double *vel_y, double *vel_z, double *pressure, double *divergence, const uint16_t *mask, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double dt, double density, double gravity, uint64_t iter_cunt)
{
    double scale = 0.0;
    double rhs_scale = 0.0;
    double nb_sum = 0.0;
    double nb_cunt = 0.0;
    uint64_t plane = 0;
    uint64_t face_x_row = 0;
    uint64_t face_y_plane = 0;
    uint64_t cell = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t iter_idx = 0;
    uint64_t face = 0;
    uint8_t red_black = 0;
    if (LDG_UNLIKELY(!vel_x || !vel_y || !vel_z)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pressure)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!divergence)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mask)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < LDG_MATH_PHYS_FIELD_DIM_MIN || ny < LDG_MATH_PHYS_FIELD_DIM_MIN || nz < LDG_MATH_PHYS_FIELD_DIM_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(dt) || dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(density) || density <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(gravity))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(iter_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    plane = nx * ny;
    face_x_row = nx + 1;
    face_y_plane = nx * (ny + 1);
    for (idx_z = 0; idx_z < nz + 1; idx_z++) { for (idx_y = 0; idx_y < ny; idx_y++) { for (idx_x = 0; idx_x < nx; idx_x++) { vel_z[(idx_z * ny + idx_y) * nx + idx_x] += gravity * dt; } } }
    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                cell = (idx_z * ny + idx_y) * nx + idx_x;
                if (mask[cell] != LDG_MATH_PHYS_FLUID_CELL_SOLID) { continue; }

                vel_x[(idx_z * ny + idx_y) * face_x_row + idx_x] = 0.0;
                vel_x[(idx_z * ny + idx_y) * face_x_row + idx_x + 1] = 0.0;
                vel_y[idx_z * face_y_plane + idx_y * nx + idx_x] = 0.0;
                vel_y[idx_z * face_y_plane + (idx_y + 1) * nx + idx_x] = 0.0;
                vel_z[(idx_z * ny + idx_y) * nx + idx_x] = 0.0;
                vel_z[((idx_z + 1) * ny + idx_y) * nx + idx_x] = 0.0;
            }
        }
    }
    rhs_scale = density * spacing / dt;
    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++) { for (idx_x = 0; idx_x < nx; idx_x++)
            {
                cell = (idx_z * ny + idx_y) * nx + idx_x;
                pressure[cell] = 0.0;
                divergence[cell] = 0.0;
                if (mask[cell] != LDG_MATH_PHYS_FLUID_CELL_WATER) { continue; }

                divergence[cell] = rhs_scale * (vel_x[(idx_z * ny + idx_y) * face_x_row + idx_x + 1] - vel_x[(idx_z * ny + idx_y) * face_x_row + idx_x] + vel_y[idx_z * face_y_plane + (idx_y + 1) * nx + idx_x] - vel_y[idx_z * face_y_plane + idx_y * nx + idx_x] + vel_z[((idx_z + 1) * ny + idx_y) * nx + idx_x] - vel_z[(idx_z * ny + idx_y) * nx + idx_x]);
            }
        }
    }
    for (iter_idx = 0; iter_idx < iter_cunt; iter_idx++)
    {
        for (red_black = 0; red_black < 2; red_black++)
        {
            for (idx_z = 1; idx_z + 1 < nz; idx_z++)
            {
                for (idx_y = 1; idx_y + 1 < ny; idx_y++)
                {
                    for (idx_x = 1; idx_x + 1 < nx; idx_x++)
                    {
                        if (((idx_x + idx_y + idx_z) & 1) != red_black) { continue; }

                        cell = (idx_z * ny + idx_y) * nx + idx_x;
                        if (mask[cell] != LDG_MATH_PHYS_FLUID_CELL_WATER) { continue; }

                        nb_sum = 0.0;
                        nb_cunt = 0.0;
                        if (mask[cell - 1] != LDG_MATH_PHYS_FLUID_CELL_SOLID)
                        {
                            nb_sum += pressure[cell - 1];
                            nb_cunt += 1.0;
                        }

                        if (mask[cell + 1] != LDG_MATH_PHYS_FLUID_CELL_SOLID)
                        {
                            nb_sum += pressure[cell + 1];
                            nb_cunt += 1.0;
                        }

                        if (mask[cell - nx] != LDG_MATH_PHYS_FLUID_CELL_SOLID)
                        {
                            nb_sum += pressure[cell - nx];
                            nb_cunt += 1.0;
                        }

                        if (mask[cell + nx] != LDG_MATH_PHYS_FLUID_CELL_SOLID)
                        {
                            nb_sum += pressure[cell + nx];
                            nb_cunt += 1.0;
                        }

                        if (mask[cell - plane] != LDG_MATH_PHYS_FLUID_CELL_SOLID)
                        {
                            nb_sum += pressure[cell - plane];
                            nb_cunt += 1.0;
                        }

                        if (mask[cell + plane] != LDG_MATH_PHYS_FLUID_CELL_SOLID)
                        {
                            nb_sum += pressure[cell + plane];
                            nb_cunt += 1.0;
                        }

                        if (nb_cunt > 0.0) { pressure[cell] = (nb_sum - divergence[cell]) / nb_cunt; }
                    }
                }
            }
        }
    }
    scale = dt / (density * spacing);
    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 1; idx_x < nx; idx_x++)
            {
                cell = (idx_z * ny + idx_y) * nx + idx_x;
                if (mask[cell] == LDG_MATH_PHYS_FLUID_CELL_SOLID || mask[cell - 1] == LDG_MATH_PHYS_FLUID_CELL_SOLID) { continue; }

                if (mask[cell] != LDG_MATH_PHYS_FLUID_CELL_WATER && mask[cell - 1] != LDG_MATH_PHYS_FLUID_CELL_WATER) { continue; }

                face = (idx_z * ny + idx_y) * face_x_row + idx_x;
                vel_x[face] -= scale * (pressure[cell] - pressure[cell - 1]);
            }
        }
    }
    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 1; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                cell = (idx_z * ny + idx_y) * nx + idx_x;
                if (mask[cell] == LDG_MATH_PHYS_FLUID_CELL_SOLID || mask[cell - nx] == LDG_MATH_PHYS_FLUID_CELL_SOLID) { continue; }

                if (mask[cell] != LDG_MATH_PHYS_FLUID_CELL_WATER && mask[cell - nx] != LDG_MATH_PHYS_FLUID_CELL_WATER) { continue; }

                face = idx_z * face_y_plane + idx_y * nx + idx_x;
                vel_y[face] -= scale * (pressure[cell] - pressure[cell - nx]);
            }
        }
    }
    for (idx_z = 1; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                cell = (idx_z * ny + idx_y) * nx + idx_x;
                if (mask[cell] == LDG_MATH_PHYS_FLUID_CELL_SOLID || mask[cell - plane] == LDG_MATH_PHYS_FLUID_CELL_SOLID) { continue; }

                if (mask[cell] != LDG_MATH_PHYS_FLUID_CELL_WATER && mask[cell - plane] != LDG_MATH_PHYS_FLUID_CELL_WATER) { continue; }

                face = (idx_z * ny + idx_y) * nx + idx_x;
                vel_z[face] -= scale * (pressure[cell] - pressure[cell - plane]);
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_fluid_g2p_advance(double *particle, uint64_t particle_cunt, const double *vel_x, const double *vel_y, const double *vel_z, const double *vel_x_old, const double *vel_y_old, const double *vel_z_old, const uint16_t *mask, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double dt, double flip_ratio, uint64_t particle_lo, uint64_t particle_hi)
{
    double *row = 0x0;
    double inv_spacing = 0.0;
    double sample_new[3] = LDG_ARR_ZERO_INIT;
    double sample_old[3] = LDG_ARR_ZERO_INIT;
    double mid_pos[3] = LDG_ARR_ZERO_INIT;
    double mid_vel[3] = LDG_ARR_ZERO_INIT;
    double lo_bound = 0.0;
    double vel_limit = 0.0;
    double hi_bound[3] = LDG_ARR_ZERO_INIT;
    uint64_t cell_idx[3] = LDG_ARR_ZERO_INIT;
    uint64_t cell = 0;
    uint64_t idx = 0;
    uint64_t axis = 0;
    if (LDG_UNLIKELY(!particle)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!vel_x || !vel_y || !vel_z)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!vel_x_old || !vel_y_old || !vel_z_old)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!mask)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < LDG_MATH_PHYS_FIELD_DIM_MIN || ny < LDG_MATH_PHYS_FIELD_DIM_MIN || nz < LDG_MATH_PHYS_FIELD_DIM_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(dt) || dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(flip_ratio) || flip_ratio < 0.0 || flip_ratio > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(particle_lo >= particle_hi || particle_hi > particle_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv_spacing = 1.0 / spacing;
    vel_limit = LDG_MATH_PHYS_FLUID_CFL_FACTOR * spacing / dt;
    lo_bound = LDG_MATH_PHYS_FLUID_BOUND_MARGIN * spacing;
    hi_bound[0] = ((double)nx - LDG_MATH_PHYS_FLUID_BOUND_MARGIN) * spacing;
    hi_bound[1] = ((double)ny - LDG_MATH_PHYS_FLUID_BOUND_MARGIN) * spacing;
    hi_bound[2] = ((double)nz - LDG_MATH_PHYS_FLUID_BOUND_MARGIN) * spacing;
    for (idx = particle_lo; idx < particle_hi; idx++)
    {
        row = particle + idx * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE;
        sample_new[0] = ldg_math_phys_fluid_grid_sample(vel_x, nx + 1, ny, nz, row[0] * inv_spacing, row[1] * inv_spacing - 0.5, row[2] * inv_spacing - 0.5);
        sample_new[1] = ldg_math_phys_fluid_grid_sample(vel_y, nx, ny + 1, nz, row[0] * inv_spacing - 0.5, row[1] * inv_spacing, row[2] * inv_spacing - 0.5);
        sample_new[2] = ldg_math_phys_fluid_grid_sample(vel_z, nx, ny, nz + 1, row[0] * inv_spacing - 0.5, row[1] * inv_spacing - 0.5, row[2] * inv_spacing);
        sample_old[0] = ldg_math_phys_fluid_grid_sample(vel_x_old, nx + 1, ny, nz, row[0] * inv_spacing, row[1] * inv_spacing - 0.5, row[2] * inv_spacing - 0.5);
        sample_old[1] = ldg_math_phys_fluid_grid_sample(vel_y_old, nx, ny + 1, nz, row[0] * inv_spacing - 0.5, row[1] * inv_spacing, row[2] * inv_spacing - 0.5);
        sample_old[2] = ldg_math_phys_fluid_grid_sample(vel_z_old, nx, ny, nz + 1, row[0] * inv_spacing - 0.5, row[1] * inv_spacing - 0.5, row[2] * inv_spacing);
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            row[3 + axis] = flip_ratio * (row[3 + axis] + sample_new[axis] - sample_old[axis]) + (1.0 - flip_ratio) * sample_new[axis];
            if (!(row[3 + axis] >= 0.0 - vel_limit)) { row[3 + axis] = 0.0 - vel_limit; }

            if (!(row[3 + axis] <= vel_limit)) { row[3 + axis] = vel_limit; }

            mid_pos[axis] = row[axis] + 0.5 * dt * sample_new[axis];
        }
        mid_vel[0] = ldg_math_phys_fluid_grid_sample(vel_x, nx + 1, ny, nz, mid_pos[0] * inv_spacing, mid_pos[1] * inv_spacing - 0.5, mid_pos[2] * inv_spacing - 0.5);
        mid_vel[1] = ldg_math_phys_fluid_grid_sample(vel_y, nx, ny + 1, nz, mid_pos[0] * inv_spacing - 0.5, mid_pos[1] * inv_spacing, mid_pos[2] * inv_spacing - 0.5);
        mid_vel[2] = ldg_math_phys_fluid_grid_sample(vel_z, nx, ny, nz + 1, mid_pos[0] * inv_spacing - 0.5, mid_pos[1] * inv_spacing - 0.5, mid_pos[2] * inv_spacing);
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            row[axis] += dt * mid_vel[axis];
            if (row[axis] < lo_bound) { row[axis] = lo_bound; }

            if (row[axis] > hi_bound[axis]) { row[axis] = hi_bound[axis]; }
        }
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { cell_idx[axis] = (uint64_t)(row[axis] * inv_spacing); }
        cell = (cell_idx[2] * ny + cell_idx[1]) * nx + cell_idx[0];
        if (mask[cell] == LDG_MATH_PHYS_FLUID_CELL_SOLID) { for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
            {
                row[axis] -= dt * mid_vel[axis];
                row[3 + axis] = 0.0;
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_fluid_conserved_get(const double *particle, uint64_t particle_cunt, uint16_t *cunt_scratch, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double density, double *out_mass, double *out_crowd_max)
{
    double inv_spacing = 0.0;
    double coord = 0.0;
    uint64_t cell_cunt = 0;
    uint64_t dim[3] = LDG_ARR_ZERO_INIT;
    uint64_t cell_idx[3] = LDG_ARR_ZERO_INIT;
    uint64_t cell = 0;
    uint64_t idx = 0;
    uint64_t axis = 0;
    uint64_t crowd_peak = 0;
    if (LDG_UNLIKELY(!particle)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!cunt_scratch)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_mass)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_crowd_max)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_mass = 0.0;
    *out_crowd_max = 0.0;
    if (LDG_UNLIKELY(nx < LDG_MATH_PHYS_FIELD_DIM_MIN || ny < LDG_MATH_PHYS_FIELD_DIM_MIN || nz < LDG_MATH_PHYS_FIELD_DIM_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(density) || density <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv_spacing = 1.0 / spacing;
    cell_cunt = nx * ny * nz;
    dim[0] = nx;
    dim[1] = ny;
    dim[2] = nz;
    for (cell = 0; cell < cell_cunt; cell++) { cunt_scratch[cell] = 0; }
    for (idx = 0; idx < particle_cunt; idx++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(particle[idx * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE]) || !ldg_math_finite_is(particle[idx * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE + 1]) || !ldg_math_finite_is(particle[idx * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE + 2]))) { return LDG_ERR_FUNC_ARG_INVALID; }

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            coord = particle[idx * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE + axis] * inv_spacing;
            if (coord < 0.0) { coord = 0.0; }

            if (coord > (double)(dim[axis] - 1)) { coord = (double)(dim[axis] - 1); }

            cell_idx[axis] = (uint64_t)coord;
        }
        cell = (cell_idx[2] * ny + cell_idx[1]) * nx + cell_idx[0];
        if (cunt_scratch[cell] < UINT16_MAX) { cunt_scratch[cell]++; }

        if (cunt_scratch[cell] > crowd_peak) { crowd_peak = cunt_scratch[cell]; }
    }
    *out_mass = (double)particle_cunt * density * spacing * spacing * spacing / (double)LDG_MATH_PHYS_FLUID_FRACTION_FULL;
    *out_crowd_max = (double)crowd_peak / (double)LDG_MATH_PHYS_FLUID_FRACTION_FULL;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_fluid_fraction_build(const double *particle, uint64_t particle_cunt, const uint16_t *mask, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double *out)
{
    const double *row = 0x0;
    double coord[3] = LDG_ARR_ZERO_INIT;
    double frac[3] = LDG_ARR_ZERO_INIT;
    double corner_weight = 0.0;
    uint64_t base[3] = LDG_ARR_ZERO_INIT;
    uint64_t corner_x = 0;
    uint64_t corner_y = 0;
    uint64_t corner_z = 0;
    uint64_t cell_cunt = 0;
    uint64_t cell = 0;
    uint64_t idx = 0;
    uint64_t axis = 0;
    double inv_spacing = 0.0;
    if (LDG_UNLIKELY(!mask)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!particle && particle_cunt > 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < LDG_MATH_PHYS_FIELD_DIM_MIN || ny < LDG_MATH_PHYS_FIELD_DIM_MIN || nz < LDG_MATH_PHYS_FIELD_DIM_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv_spacing = 1.0 / spacing;
    cell_cunt = nx * ny * nz;
    for (cell = 0; cell < cell_cunt; cell++) { out[cell] = 0.0; }
    for (idx = 0; idx < particle_cunt; idx++)
    {
        row = particle + idx * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            if (LDG_UNLIKELY(!ldg_math_finite_is(row[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; }

            coord[axis] = row[axis] * inv_spacing - 0.5;
            if (coord[axis] < 0.0) { coord[axis] = 0.0; }
        }
        if (coord[0] > (double)(nx - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN) { coord[0] = (double)(nx - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN; }

        if (coord[1] > (double)(ny - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN) { coord[1] = (double)(ny - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN; }

        if (coord[2] > (double)(nz - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN) { coord[2] = (double)(nz - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN; }

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            base[axis] = (uint64_t)coord[axis];
            frac[axis] = coord[axis] - (double)base[axis];
        }
        for (corner_z = 0; corner_z < 2; corner_z++) { for (corner_y = 0; corner_y < 2; corner_y++) { for (corner_x = 0; corner_x < 2; corner_x++)
                {
                    corner_weight = ((corner_x == 1) ? frac[0] : (1.0 - frac[0])) * ((corner_y == 1) ? frac[1] : (1.0 - frac[1])) * ((corner_z == 1) ? frac[2] : (1.0 - frac[2]));
                    out[((base[2] + corner_z) * ny + base[1] + corner_y) * nx + base[0] + corner_x] += corner_weight;
                }
            } }
    }
    for (cell = 0; cell < cell_cunt; cell++)
    {
        out[cell] /= (double)LDG_MATH_PHYS_FLUID_FRACTION_FULL;
        if (out[cell] > 1.0) { out[cell] = 1.0; }

        if (mask[cell] == LDG_MATH_PHYS_FLUID_CELL_SOLID) { out[cell] = 0.0; }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_fluid_caustic_build(const double *fraction, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, const double *light_dir, double refract, double *out, double *out_escaped)
{
    double dir[3] = LDG_ARR_ZERO_INIT;
    double normal[3] = LDG_ARR_ZERO_INIT;
    double refracted[3] = LDG_ARR_ZERO_INIT;
    double dir_len = 0.0;
    double normal_len = 0.0;
    double surface_z = 0.0;
    double cross_t = 0.0;
    double cos_in = 0.0;
    double sin2_out = 0.0;
    double cos_out = 0.0;
    double eta = 0.0;
    double travel = 0.0;
    double hit_x = 0.0;
    double hit_y = 0.0;
    double frac_x = 0.0;
    double frac_y = 0.0;
    double escaped = 0.0;
    double val_hi = 0.0;
    double val_lo = 0.0;
    uint64_t col_x = 0;
    uint64_t col_y = 0;
    uint64_t layer = 0;
    uint64_t base_x = 0;
    uint64_t base_y = 0;
    uint64_t floor_cunt = 0;
    uint64_t cell = 0;
    uint64_t axis = 0;
    uint8_t found = UINT8_MAX;
    if (LDG_UNLIKELY(!fraction)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!light_dir)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out_escaped) { *out_escaped = 0.0; }

    if (LDG_UNLIKELY(nx < LDG_MATH_PHYS_FIELD_DIM_MIN || ny < LDG_MATH_PHYS_FIELD_DIM_MIN || nz < LDG_MATH_PHYS_FIELD_DIM_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(spacing) || spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(refract) || refract < 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(light_dir[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; } }
    dir_len = ldg_math_sqrt_get(light_dir[0] * light_dir[0] + light_dir[1] * light_dir[1] + light_dir[2] * light_dir[2]);
    if (LDG_UNLIKELY(dir_len < LDG_MATH_EPSILON || light_dir[2] >= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { dir[axis] = light_dir[axis] / dir_len; }
    eta = 1.0 / refract;
    floor_cunt = nx * ny;
    for (cell = 0; cell < floor_cunt; cell++) { out[cell] = 0.0; }
    for (col_y = 0; col_y < ny; col_y++)
    {
        for (col_x = 0; col_x < nx; col_x++)
        {
            found = LDG_TRUTH_FALSE;
            for (layer = nz - 1; layer > 0; layer--)
            {
                val_hi = fraction[(layer * ny + col_y) * nx + col_x];
                val_lo = fraction[((layer - 1) * ny + col_y) * nx + col_x];
                if (val_lo >= LDG_MATH_PHYS_FLUID_ISO_LEVEL && val_hi < LDG_MATH_PHYS_FLUID_ISO_LEVEL)
                {
                    cross_t = (LDG_MATH_PHYS_FLUID_ISO_LEVEL - val_hi) / (val_lo - val_hi);
                    surface_z = (double)layer - cross_t;
                    found = LDG_TRUTH_TRUE;
                    break;
                }
            }
            if (found != LDG_TRUTH_TRUE && fraction[((nz - 1) * ny + col_y) * nx + col_x] >= LDG_MATH_PHYS_FLUID_ISO_LEVEL)
            {
                surface_z = (double)(nz - 1);
                found = LDG_TRUTH_TRUE;
            }

            if (found == LDG_TRUTH_TRUE)
            {
                normal[0] = ldg_math_phys_fluid_grid_sample(fraction, nx, ny, nz, (double)col_x - 1.0, (double)col_y, surface_z) - ldg_math_phys_fluid_grid_sample(fraction, nx, ny, nz, (double)col_x + 1.0, (double)col_y, surface_z);
                normal[1] = ldg_math_phys_fluid_grid_sample(fraction, nx, ny, nz, (double)col_x, (double)col_y - 1.0, surface_z) - ldg_math_phys_fluid_grid_sample(fraction, nx, ny, nz, (double)col_x, (double)col_y + 1.0, surface_z);
                normal[2] = ldg_math_phys_fluid_grid_sample(fraction, nx, ny, nz, (double)col_x, (double)col_y, surface_z - 1.0) - ldg_math_phys_fluid_grid_sample(fraction, nx, ny, nz, (double)col_x, (double)col_y, surface_z + 1.0);
                normal_len = ldg_math_sqrt_get(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
                if (normal_len < LDG_MATH_EPSILON || normal[2] <= 0.0)
                {
                    normal[0] = 0.0;
                    normal[1] = 0.0;
                    normal[2] = 1.0;
                    normal_len = 1.0;
                }

                for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { normal[axis] /= normal_len; }
                cos_in = 0.0 - (dir[0] * normal[0] + dir[1] * normal[1] + dir[2] * normal[2]);
                if (cos_in < 0.0) { cos_in = 0.0; }

                sin2_out = eta * eta * (1.0 - cos_in * cos_in);
                cos_out = ldg_math_sqrt_get(1.0 - sin2_out);
                for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { refracted[axis] = eta * dir[axis] + (eta * cos_in - cos_out) * normal[axis]; }
            }
            else
            {
                surface_z = (double)(nz - 1);
                for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { refracted[axis] = dir[axis]; }
            }

            if (refracted[2] >= 0.0 - LDG_MATH_EPSILON)
            {
                escaped += 1.0;
                continue;
            }

            travel = surface_z / (0.0 - refracted[2]);
            hit_x = (double)col_x + refracted[0] * travel;
            hit_y = (double)col_y + refracted[1] * travel;
            if (hit_x < 0.0 || hit_y < 0.0 || hit_x > (double)(nx - 1) || hit_y > (double)(ny - 1))
            {
                escaped += 1.0;
                continue;
            }

            if (hit_x > (double)(nx - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN) { hit_x = (double)(nx - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN; }

            if (hit_y > (double)(ny - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN) { hit_y = (double)(ny - 1) - LDG_MATH_PHYS_FLUID_CLAMP_MARGIN; }

            base_x = (uint64_t)hit_x;
            base_y = (uint64_t)hit_y;
            frac_x = hit_x - (double)base_x;
            frac_y = hit_y - (double)base_y;
            out[base_y * nx + base_x] += (1.0 - frac_x) * (1.0 - frac_y);
            out[base_y * nx + base_x + 1] += frac_x * (1.0 - frac_y);
            out[(base_y + 1) * nx + base_x] += (1.0 - frac_x) * frac_y;
            out[(base_y + 1) * nx + base_x + 1] += frac_x * frac_y;
        }
    }
    if (out_escaped) { *out_escaped = escaped; }

    return LDG_ERR_AOK;
}
