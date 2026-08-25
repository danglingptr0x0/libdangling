
#include <dangling/math/phys.h>
#include <dangling/math/math.h>
#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/secure.h>
#include <dangling/time/time.h>
#include <dangling/thread/pool.h>
#include <dangling/arch/atomic.h>
#include <dangling/arch/fence.h>
#include "phys/kernel.h"
#include "phys/conserved.h"
#include "phys/particle.h"
#include "phys/field.h"
#include "phys/thermo.h"
#include "phys/electrostatic.h"
#include "phys/magnetostatic.h"
#include "phys/portmode.h"
#include "phys/fluid.h"
#include "phys/atmosphere.h"
#include "phys/compute/compute.h"

uint32_t ldg_math_phys_integrate(double *pos, double *prev, const double *accel, const double *inv_mass, uint64_t cunt, double dt, double damping)
{
    uint64_t idx = 0;
    uint64_t base = 0;
    uint64_t axis = 0;
    double dt2 = 0.0;
    double retain = 0.0;
    double cur = 0.0;
    double nxt = 0.0;

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!prev)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!accel)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!inv_mass)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(damping < 0.0 || damping > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dt2 = dt * dt;
    retain = 1.0 - damping;

    for (idx = 0; idx < cunt; idx++)
    {
        if (inv_mass[idx] <= 0.0) { continue; }

        base = idx * LDG_MATH_PHYS_DIM;

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            cur = pos[base + axis];
            nxt = cur + retain * (cur - prev[base + axis]) + accel[base + axis] * dt2;
            prev[base + axis] = cur;
            pos[base + axis] = nxt;
        }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_force_cum(double *accel, uint64_t cunt, const double *gravity, const double *wind)
{
    uint64_t idx = 0;
    uint64_t base = 0;
    uint64_t axis = 0;
    double uniform[LDG_MATH_PHYS_DIM] = { 0 };

    if (LDG_UNLIKELY(!accel)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!gravity)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!wind)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { uniform[axis] = gravity[axis] + wind[axis]; }

    for (idx = 0; idx < cunt; idx++)
    {
        base = idx * LDG_MATH_PHYS_DIM;

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { accel[base + axis] += uniform[axis]; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_plane_collide(double *pos, double *prev, const double *inv_mass, uint64_t cunt, const double *normal, double offset, double restitution)
{
    uint64_t idx = 0;
    uint64_t base = 0;
    uint64_t axis = 0;
    double dist = 0.0;
    double vn = 0.0;
    double vn_new = 0.0;
    double prev_shift = 0.0;

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!prev)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!inv_mass)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!normal)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(restitution < 0.0 || restitution > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < cunt; idx++)
    {
        if (inv_mass[idx] <= 0.0) { continue; }

        base = idx * LDG_MATH_PHYS_DIM;

        dist = -offset;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { dist += normal[axis] * pos[base + axis]; }

        if (dist >= 0.0) { continue; }

        vn = 0.0;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { vn += normal[axis] * (pos[base + axis] - prev[base + axis]); }

        vn_new = vn;
        if (vn < 0.0) { vn_new = -restitution * vn; }

        prev_shift = -dist + (vn - vn_new);

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            pos[base + axis] -= dist * normal[axis];
            prev[base + axis] += prev_shift * normal[axis];
        }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_sphere_collide(double *pos, double *prev, const double *inv_mass, uint64_t cunt, const double *center, double radius, double restitution)
{
    uint64_t idx = 0;
    uint64_t base = 0;
    uint64_t axis = 0;
    double nrm[LDG_MATH_PHYS_DIM] = { 0 };
    double rel[LDG_MATH_PHYS_DIM] = { 0 };
    double d_len = 0.0;
    double dist = 0.0;
    double vn = 0.0;
    double vn_new = 0.0;
    double prev_shift = 0.0;

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!prev)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!inv_mass)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!center)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(radius < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(restitution < 0.0 || restitution > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < cunt; idx++)
    {
        if (inv_mass[idx] <= 0.0) { continue; }

        base = idx * LDG_MATH_PHYS_DIM;

        d_len = 0.0;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            rel[axis] = pos[base + axis] - center[axis];
            d_len += rel[axis] * rel[axis];
        }
        d_len = ldg_math_sqrt_get(d_len);

        if (d_len <= 0.0)
        {
            nrm[0] = 0.0;
            nrm[1] = 0.0;
            nrm[2] = 1.0;
            dist = -radius;
        }
        else
        {
            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { nrm[axis] = rel[axis] / d_len; }
            dist = d_len - radius;
        }

        if (dist >= 0.0) { continue; }

        vn = 0.0;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { vn += nrm[axis] * (pos[base + axis] - prev[base + axis]); }

        vn_new = vn;
        if (vn < 0.0) { vn_new = -restitution * vn; }

        prev_shift = -dist + (vn - vn_new);

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            pos[base + axis] -= dist * nrm[axis];
            prev[base + axis] += prev_shift * nrm[axis];
        }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_box_collide(double *pos, double *prev, const double *inv_mass, uint64_t cunt, const double *center, const double *half, const double *axes, double restitution)
{
    uint64_t idx = 0;
    uint64_t base = 0;
    uint64_t axis = 0;
    uint64_t comp = 0;
    uint64_t min_axis = 0;
    double rel[LDG_MATH_PHYS_DIM] = { 0 };
    double local[LDG_MATH_PHYS_DIM] = { 0 };
    double nrm[LDG_MATH_PHYS_DIM] = { 0 };
    double pen = 0.0;
    double min_pen = 0.0;
    double dist = 0.0;
    double vn = 0.0;
    double vn_new = 0.0;
    double prev_shift = 0.0;
    uint8_t inside = 0;

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!prev)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!inv_mass)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!center)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!half)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!axes)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(restitution < 0.0 || restitution > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < cunt; idx++)
    {
        if (inv_mass[idx] <= 0.0) { continue; }

        base = idx * LDG_MATH_PHYS_DIM;

        for (comp = 0; comp < LDG_MATH_PHYS_DIM; comp++) { rel[comp] = pos[base + comp] - center[comp]; }

        inside = 1;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            local[axis] = 0.0;
            for (comp = 0; comp < LDG_MATH_PHYS_DIM; comp++) { local[axis] += axes[axis * LDG_MATH_PHYS_DIM + comp] * rel[comp]; }
            if (ldg_math_abs_get(local[axis]) >= half[axis]) { inside = 0; }
        }

        if (!inside) { continue; }

        min_axis = 0;
        min_pen = half[0] - ldg_math_abs_get(local[0]);
        for (axis = 1; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            pen = half[axis] - ldg_math_abs_get(local[axis]);
            if (pen < min_pen)
            {
                min_pen = pen;
                min_axis = axis;
            }
        }

        for (comp = 0; comp < LDG_MATH_PHYS_DIM; comp++) { nrm[comp] = axes[min_axis * LDG_MATH_PHYS_DIM + comp]; }
        if (local[min_axis] < 0.0) { for (comp = 0; comp < LDG_MATH_PHYS_DIM; comp++) { nrm[comp] = -nrm[comp]; } }

        dist = -min_pen;

        vn = 0.0;
        for (comp = 0; comp < LDG_MATH_PHYS_DIM; comp++) { vn += nrm[comp] * (pos[base + comp] - prev[base + comp]); }

        vn_new = vn;
        if (vn < 0.0) { vn_new = -restitution * vn; }

        prev_shift = -dist + (vn - vn_new);

        for (comp = 0; comp < LDG_MATH_PHYS_DIM; comp++)
        {
            pos[base + comp] -= dist * nrm[comp];
            prev[base + comp] += prev_shift * nrm[comp];
        }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_capsule_collide(double *pos, double *prev, const double *inv_mass, uint64_t cunt, const double *point_a, const double *point_b, double radius, double restitution)
{
    uint64_t idx = 0;
    uint64_t base = 0;
    uint64_t axis = 0;
    double edge[LDG_MATH_PHYS_DIM] = { 0 };
    double rel[LDG_MATH_PHYS_DIM] = { 0 };
    double closest[LDG_MATH_PHYS_DIM] = { 0 };
    double delta[LDG_MATH_PHYS_DIM] = { 0 };
    double nrm[LDG_MATH_PHYS_DIM] = { 0 };
    double edge_len2 = 0.0;
    double param = 0.0;
    double d_len = 0.0;
    double dist = 0.0;
    double vn = 0.0;
    double vn_new = 0.0;
    double prev_shift = 0.0;

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!prev)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!inv_mass)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!point_a)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!point_b)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(radius < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(restitution < 0.0 || restitution > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    edge_len2 = 0.0;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        edge[axis] = point_b[axis] - point_a[axis];
        edge_len2 += edge[axis] * edge[axis];
    }

    for (idx = 0; idx < cunt; idx++)
    {
        if (inv_mass[idx] <= 0.0) { continue; }

        base = idx * LDG_MATH_PHYS_DIM;

        param = 0.0;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            rel[axis] = pos[base + axis] - point_a[axis];
            param += rel[axis] * edge[axis];
        }

        if (edge_len2 > 0.0)
        {
            param = param / edge_len2;
            if (param < 0.0) { param = 0.0; }

            if (param > 1.0) { param = 1.0; }
        }
        else { param = 0.0; }

        d_len = 0.0;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            closest[axis] = point_a[axis] + param * edge[axis];
            delta[axis] = pos[base + axis] - closest[axis];
            d_len += delta[axis] * delta[axis];
        }
        d_len = ldg_math_sqrt_get(d_len);

        if (d_len <= 0.0)
        {
            nrm[0] = 0.0;
            nrm[1] = 0.0;
            nrm[2] = 1.0;
            dist = -radius;
        }
        else
        {
            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { nrm[axis] = delta[axis] / d_len; }
            dist = d_len - radius;
        }

        if (dist >= 0.0) { continue; }

        vn = 0.0;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { vn += nrm[axis] * (pos[base + axis] - prev[base + axis]); }

        vn_new = vn;
        if (vn < 0.0) { vn_new = -restitution * vn; }

        prev_shift = -dist + (vn - vn_new);

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            pos[base + axis] -= dist * nrm[axis];
            prev[base + axis] += prev_shift * nrm[axis];
        }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_xpbd_dist_solve(double *pos, const double *inv_mass, uint64_t cunt, const ldg_math_phys_dist_constraint_t *constraint, double *lambda, uint64_t constraint_cunt, double dt, uint32_t iter)
{
    uint64_t sweep = 0;
    uint64_t idx = 0;
    uint64_t axis = 0;
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    uint64_t base_a = 0;
    uint64_t base_b = 0;
    double wa = 0.0;
    double wb = 0.0;
    double w_sum = 0.0;
    double diff[LDG_MATH_PHYS_DIM] = { 0 };
    double len = 0.0;
    double inv_len = 0.0;
    double c_val = 0.0;
    double reg_compliance = 0.0;
    double denom = 0.0;
    double dlambda = 0.0;
    double corr = 0.0;
    double dt2 = 0.0;

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!inv_mass)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!constraint)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!lambda)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dt2 = dt * dt;

    for (idx = 0; idx < constraint_cunt; idx++) { lambda[idx] = 0.0; }

    for (sweep = 0; sweep < iter; sweep++)
    {
        for (idx = 0; idx < constraint_cunt; idx++)
        {
            idx_a = constraint[idx].idx_a;
            idx_b = constraint[idx].idx_b;
            if (idx_a >= cunt || idx_b >= cunt) { continue; }

            wa = inv_mass[idx_a];
            wb = inv_mass[idx_b];
            w_sum = wa + wb;
            if (w_sum <= 0.0) { continue; }

            base_a = idx_a * LDG_MATH_PHYS_DIM;
            base_b = idx_b * LDG_MATH_PHYS_DIM;

            len = 0.0;
            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
            {
                diff[axis] = pos[base_a + axis] - pos[base_b + axis];
                len += diff[axis] * diff[axis];
            }
            len = ldg_math_sqrt_get(len);
            if (len <= 0.0) { continue; }

            inv_len = 1.0 / len;
            c_val = len - constraint[idx].rest_len;
            reg_compliance = constraint[idx].compliance / dt2;
            denom = w_sum + reg_compliance;
            if (denom <= 0.0) { continue; }

            dlambda = (-c_val - reg_compliance * lambda[idx]) / denom;
            lambda[idx] += dlambda;

            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
            {
                corr = dlambda * diff[axis] * inv_len;
                pos[base_a + axis] += wa * corr;
                pos[base_b + axis] -= wb * corr;
            }
        }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_2d_advance(const double *cur, const double *prev, double *next, uint64_t nx, uint64_t ny, double spacing, double wave_speed, double dt, double damping, uint32_t boundary)
{
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t center = 0;
    uint64_t low_x = 0;
    uint64_t high_x = 0;
    uint64_t low_y = 0;
    uint64_t high_y = 0;
    double retain = 0.0;
    double accel_factor = 0.0;
    double mur_factor = 0.0;
    double neighbour_sum = 0.0;
    double cur_center = 0.0;

    if (LDG_UNLIKELY(!cur)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!prev)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!next)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < LDG_MATH_PHYS_FIELD_DIM_MIN || ny < LDG_MATH_PHYS_FIELD_DIM_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(wave_speed < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(damping < 0.0 || damping > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(boundary > LDG_MATH_PHYS_FIELD_BOUNDARY_ABSORBING)) { return LDG_ERR_FUNC_ARG_INVALID; }

    retain = 1.0 - damping;
    accel_factor = wave_speed * wave_speed * dt * dt / (spacing * spacing);
    mur_factor = (wave_speed * dt - spacing) / (wave_speed * dt + spacing);

    for (idx_y = 0; idx_y < ny; idx_y++)
    {
        for (idx_x = 0; idx_x < nx; idx_x++)
        {
            if (boundary == LDG_MATH_PHYS_FIELD_BOUNDARY_ABSORBING) { if (idx_x == 0 || idx_x == nx - 1 || idx_y == 0 || idx_y == ny - 1) { continue; } }

            center = idx_y * nx + idx_x;
            low_x = (idx_x == 0) ? (idx_x + 1) : (idx_x - 1);
            high_x = (idx_x == nx - 1) ? (idx_x - 1) : (idx_x + 1);
            low_y = (idx_y == 0) ? (idx_y + 1) : (idx_y - 1);
            high_y = (idx_y == ny - 1) ? (idx_y - 1) : (idx_y + 1);

            cur_center = cur[center];
            neighbour_sum = cur[idx_y * nx + low_x] + cur[idx_y * nx + high_x] + cur[low_y * nx + idx_x] + cur[high_y * nx + idx_x] - 4.0 * cur_center;
            next[center] = cur_center + retain * (cur_center - prev[center]) + accel_factor * neighbour_sum;
        }
    }

    if (boundary == LDG_MATH_PHYS_FIELD_BOUNDARY_ABSORBING)
    {
        for (idx_y = 1; idx_y < ny - 1; idx_y++)
        {
            center = idx_y * nx;
            next[center] = cur[idx_y * nx + 1] + mur_factor * (next[idx_y * nx + 1] - cur[center]);
            center = idx_y * nx + (nx - 1);
            next[center] = cur[idx_y * nx + (nx - 2)] + mur_factor * (next[idx_y * nx + (nx - 2)] - cur[center]);
        }

        for (idx_x = 1; idx_x < nx - 1; idx_x++)
        {
            center = idx_x;
            next[center] = cur[nx + idx_x] + mur_factor * (next[nx + idx_x] - cur[center]);
            center = (ny - 1) * nx + idx_x;
            next[center] = cur[(ny - 2) * nx + idx_x] + mur_factor * (next[(ny - 2) * nx + idx_x] - cur[center]);
        }

        next[0] = 0.5 * (next[1] + next[nx]);
        next[nx - 1] = 0.5 * (next[nx - 2] + next[nx + (nx - 1)]);
        next[(ny - 1) * nx] = 0.5 * (next[(ny - 1) * nx + 1] + next[(ny - 2) * nx]);
        next[(ny - 1) * nx + (nx - 1)] = 0.5 * (next[(ny - 1) * nx + (nx - 2)] + next[(ny - 2) * nx + (nx - 1)]);
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_2d_sample_get(const double *field, uint64_t nx, uint64_t ny, double spacing, double pos_x, double pos_y, double *out)
{
    uint64_t idx_x0 = 0;
    uint64_t idx_x1 = 0;
    uint64_t idx_y0 = 0;
    uint64_t idx_y1 = 0;
    double grid_x = 0.0;
    double grid_y = 0.0;
    double fraction_x = 0.0;
    double fraction_y = 0.0;
    double row_low = 0.0;
    double row_high = 0.0;

    if (LDG_UNLIKELY(!field)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 1 || ny < 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = 0.0;

    grid_x = pos_x / spacing;
    grid_y = pos_y / spacing;
    if (grid_x < 0.0) { grid_x = 0.0; }

    if (grid_y < 0.0) { grid_y = 0.0; }

    if (grid_x > (double)(nx - 1)) { grid_x = (double)(nx - 1); }

    if (grid_y > (double)(ny - 1)) { grid_y = (double)(ny - 1); }

    idx_x0 = (uint64_t)ldg_math_floor_get(grid_x);
    idx_y0 = (uint64_t)ldg_math_floor_get(grid_y);
    idx_x1 = idx_x0 + 1;
    idx_y1 = idx_y0 + 1;
    if (idx_x1 >= nx) { idx_x1 = nx - 1; }

    if (idx_y1 >= ny) { idx_y1 = ny - 1; }

    fraction_x = grid_x - (double)idx_x0;
    fraction_y = grid_y - (double)idx_y0;

    row_low = field[idx_y0 * nx + idx_x0] + fraction_x * (field[idx_y0 * nx + idx_x1] - field[idx_y0 * nx + idx_x0]);
    row_high = field[idx_y1 * nx + idx_x0] + fraction_x * (field[idx_y1 * nx + idx_x1] - field[idx_y1 * nx + idx_x0]);
    *out = row_low + fraction_y * (row_high - row_low);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_2d_src_add(double *field, uint64_t nx, uint64_t ny, double spacing, double pos_x, double pos_y, double amount)
{
    uint64_t idx_x0 = 0;
    uint64_t idx_x1 = 0;
    uint64_t idx_y0 = 0;
    uint64_t idx_y1 = 0;
    double grid_x = 0.0;
    double grid_y = 0.0;
    double fraction_x = 0.0;
    double fraction_y = 0.0;
    double weight_x_low = 0.0;
    double weight_x_high = 0.0;
    double weight_y_low = 0.0;
    double weight_y_high = 0.0;

    if (LDG_UNLIKELY(!field)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 1 || ny < 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    grid_x = pos_x / spacing;
    grid_y = pos_y / spacing;
    if (grid_x < 0.0) { grid_x = 0.0; }

    if (grid_y < 0.0) { grid_y = 0.0; }

    if (grid_x > (double)(nx - 1)) { grid_x = (double)(nx - 1); }

    if (grid_y > (double)(ny - 1)) { grid_y = (double)(ny - 1); }

    idx_x0 = (uint64_t)ldg_math_floor_get(grid_x);
    idx_y0 = (uint64_t)ldg_math_floor_get(grid_y);
    idx_x1 = idx_x0 + 1;
    idx_y1 = idx_y0 + 1;
    if (idx_x1 >= nx) { idx_x1 = nx - 1; }

    if (idx_y1 >= ny) { idx_y1 = ny - 1; }

    fraction_x = grid_x - (double)idx_x0;
    fraction_y = grid_y - (double)idx_y0;
    weight_x_high = fraction_x;
    weight_x_low = 1.0 - fraction_x;
    weight_y_high = fraction_y;
    weight_y_low = 1.0 - fraction_y;

    field[idx_y0 * nx + idx_x0] += weight_x_low * weight_y_low * amount;
    field[idx_y0 * nx + idx_x1] += weight_x_high * weight_y_low * amount;
    field[idx_y1 * nx + idx_x0] += weight_x_low * weight_y_high * amount;
    field[idx_y1 * nx + idx_x1] += weight_x_high * weight_y_high * amount;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_3d_advance(const double *cur, const double *prev, double *next, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double wave_speed, double dt, double damping, uint32_t boundary)
{
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t plane = 0;
    uint64_t center = 0;
    uint64_t low_x = 0;
    uint64_t high_x = 0;
    uint64_t low_y = 0;
    uint64_t high_y = 0;
    uint64_t low_z = 0;
    uint64_t high_z = 0;
    double retain = 0.0;
    double accel_factor = 0.0;
    double mur_factor = 0.0;
    double neighbour_sum = 0.0;
    double cur_center = 0.0;

    if (LDG_UNLIKELY(!cur)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!prev)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!next)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < LDG_MATH_PHYS_FIELD_DIM_MIN || ny < LDG_MATH_PHYS_FIELD_DIM_MIN || nz < LDG_MATH_PHYS_FIELD_DIM_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(wave_speed < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(damping < 0.0 || damping > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(boundary > LDG_MATH_PHYS_FIELD_BOUNDARY_ABSORBING)) { return LDG_ERR_FUNC_ARG_INVALID; }

    plane = nx * ny;
    retain = 1.0 - damping;
    accel_factor = wave_speed * wave_speed * dt * dt / (spacing * spacing);
    mur_factor = (wave_speed * dt - spacing) / (wave_speed * dt + spacing);

    for (idx_z = 0; idx_z < nz; idx_z++)
    {
        for (idx_y = 0; idx_y < ny; idx_y++)
        {
            for (idx_x = 0; idx_x < nx; idx_x++)
            {
                if (boundary == LDG_MATH_PHYS_FIELD_BOUNDARY_ABSORBING) { if (idx_x == 0 || idx_x == nx - 1 || idx_y == 0 || idx_y == ny - 1 || idx_z == 0 || idx_z == nz - 1) { continue; } }

                center = idx_z * plane + idx_y * nx + idx_x;
                low_x = (idx_x == 0) ? (idx_x + 1) : (idx_x - 1);
                high_x = (idx_x == nx - 1) ? (idx_x - 1) : (idx_x + 1);
                low_y = (idx_y == 0) ? (idx_y + 1) : (idx_y - 1);
                high_y = (idx_y == ny - 1) ? (idx_y - 1) : (idx_y + 1);
                low_z = (idx_z == 0) ? (idx_z + 1) : (idx_z - 1);
                high_z = (idx_z == nz - 1) ? (idx_z - 1) : (idx_z + 1);

                cur_center = cur[center];
                neighbour_sum = cur[idx_z * plane + idx_y * nx + low_x] + cur[idx_z * plane + idx_y * nx + high_x] + cur[idx_z * plane + low_y * nx + idx_x] + cur[idx_z * plane + high_y * nx + idx_x] + cur[low_z * plane + idx_y * nx + idx_x] + cur[high_z * plane + idx_y * nx + idx_x] - 6.0 * cur_center;
                next[center] = cur_center + retain * (cur_center - prev[center]) + accel_factor * neighbour_sum;
            }
        }
    }

    if (boundary == LDG_MATH_PHYS_FIELD_BOUNDARY_ABSORBING)
    {
        for (idx_z = 1; idx_z < nz - 1; idx_z++) { for (idx_y = 1; idx_y < ny - 1; idx_y++)
            {
                center = idx_z * plane + idx_y * nx;
                next[center] = cur[idx_z * plane + idx_y * nx + 1] + mur_factor * (next[idx_z * plane + idx_y * nx + 1] - cur[center]);
                center = idx_z * plane + idx_y * nx + (nx - 1);
                next[center] = cur[idx_z * plane + idx_y * nx + (nx - 2)] + mur_factor * (next[idx_z * plane + idx_y * nx + (nx - 2)] - cur[center]);
            }
        }

        for (idx_z = 1; idx_z < nz - 1; idx_z++) { for (idx_x = 1; idx_x < nx - 1; idx_x++)
            {
                center = idx_z * plane + idx_x;
                next[center] = cur[idx_z * plane + nx + idx_x] + mur_factor * (next[idx_z * plane + nx + idx_x] - cur[center]);
                center = idx_z * plane + (ny - 1) * nx + idx_x;
                next[center] = cur[idx_z * plane + (ny - 2) * nx + idx_x] + mur_factor * (next[idx_z * plane + (ny - 2) * nx + idx_x] - cur[center]);
            }
        }

        for (idx_y = 1; idx_y < ny - 1; idx_y++) { for (idx_x = 1; idx_x < nx - 1; idx_x++)
            {
                center = idx_y * nx + idx_x;
                next[center] = cur[plane + idx_y * nx + idx_x] + mur_factor * (next[plane + idx_y * nx + idx_x] - cur[center]);
                center = (nz - 1) * plane + idx_y * nx + idx_x;
                next[center] = cur[(nz - 2) * plane + idx_y * nx + idx_x] + mur_factor * (next[(nz - 2) * plane + idx_y * nx + idx_x] - cur[center]);
            }
        }

        for (idx_z = 1; idx_z < nz - 1; idx_z++)
        {
            next[idx_z * plane] = 0.5 * (next[idx_z * plane + 1] + next[idx_z * plane + nx]);
            next[idx_z * plane + (nx - 1)] = 0.5 * (next[idx_z * plane + (nx - 2)] + next[idx_z * plane + nx + (nx - 1)]);
            next[idx_z * plane + (ny - 1) * nx] = 0.5 * (next[idx_z * plane + (ny - 1) * nx + 1] + next[idx_z * plane + (ny - 2) * nx]);
            next[idx_z * plane + (ny - 1) * nx + (nx - 1)] = 0.5 * (next[idx_z * plane + (ny - 1) * nx + (nx - 2)] + next[idx_z * plane + (ny - 2) * nx + (nx - 1)]);
        }

        for (idx_y = 1; idx_y < ny - 1; idx_y++)
        {
            next[idx_y * nx] = 0.5 * (next[idx_y * nx + 1] + next[plane + idx_y * nx]);
            next[idx_y * nx + (nx - 1)] = 0.5 * (next[idx_y * nx + (nx - 2)] + next[plane + idx_y * nx + (nx - 1)]);
            next[(nz - 1) * plane + idx_y * nx] = 0.5 * (next[(nz - 1) * plane + idx_y * nx + 1] + next[(nz - 2) * plane + idx_y * nx]);
            next[(nz - 1) * plane + idx_y * nx + (nx - 1)] = 0.5 * (next[(nz - 1) * plane + idx_y * nx + (nx - 2)] + next[(nz - 2) * plane + idx_y * nx + (nx - 1)]);
        }

        for (idx_x = 1; idx_x < nx - 1; idx_x++)
        {
            next[idx_x] = 0.5 * (next[nx + idx_x] + next[plane + idx_x]);
            next[(ny - 1) * nx + idx_x] = 0.5 * (next[(ny - 2) * nx + idx_x] + next[plane + (ny - 1) * nx + idx_x]);
            next[(nz - 1) * plane + idx_x] = 0.5 * (next[(nz - 1) * plane + nx + idx_x] + next[(nz - 2) * plane + idx_x]);
            next[(nz - 1) * plane + (ny - 1) * nx + idx_x] = 0.5 * (next[(nz - 1) * plane + (ny - 2) * nx + idx_x] + next[(nz - 2) * plane + (ny - 1) * nx + idx_x]);
        }

        next[0] = (next[1] + next[nx] + next[plane]) / 3.0;
        next[nx - 1] = (next[nx - 2] + next[nx + (nx - 1)] + next[plane + (nx - 1)]) / 3.0;
        next[(ny - 1) * nx] = (next[(ny - 1) * nx + 1] + next[(ny - 2) * nx] + next[plane + (ny - 1) * nx]) / 3.0;
        next[(ny - 1) * nx + (nx - 1)] = (next[(ny - 1) * nx + (nx - 2)] + next[(ny - 2) * nx + (nx - 1)] + next[plane + (ny - 1) * nx + (nx - 1)]) / 3.0;
        next[(nz - 1) * plane] = (next[(nz - 1) * plane + 1] + next[(nz - 1) * plane + nx] + next[(nz - 2) * plane]) / 3.0;
        next[(nz - 1) * plane + (nx - 1)] = (next[(nz - 1) * plane + (nx - 2)] + next[(nz - 1) * plane + nx + (nx - 1)] + next[(nz - 2) * plane + (nx - 1)]) / 3.0;
        next[(nz - 1) * plane + (ny - 1) * nx] = (next[(nz - 1) * plane + (ny - 1) * nx + 1] + next[(nz - 1) * plane + (ny - 2) * nx] + next[(nz - 2) * plane + (ny - 1) * nx]) / 3.0;
        next[(nz - 1) * plane + (ny - 1) * nx + (nx - 1)] = (next[(nz - 1) * plane + (ny - 1) * nx + (nx - 2)] + next[(nz - 1) * plane + (ny - 2) * nx + (nx - 1)] + next[(nz - 2) * plane + (ny - 1) * nx + (nx - 1)]) / 3.0;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_3d_sample_get(const double *field, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double pos_x, double pos_y, double pos_z, double *out)
{
    uint64_t idx_x0 = 0;
    uint64_t idx_x1 = 0;
    uint64_t idx_y0 = 0;
    uint64_t idx_y1 = 0;
    uint64_t idx_z0 = 0;
    uint64_t idx_z1 = 0;
    uint64_t plane = 0;
    double grid_x = 0.0;
    double grid_y = 0.0;
    double grid_z = 0.0;
    double fraction_x = 0.0;
    double fraction_y = 0.0;
    double fraction_z = 0.0;
    double row_low_z0 = 0.0;
    double row_high_z0 = 0.0;
    double row_low_z1 = 0.0;
    double row_high_z1 = 0.0;
    double face_z0 = 0.0;
    double face_z1 = 0.0;

    if (LDG_UNLIKELY(!field)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 1 || ny < 1 || nz < 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = 0.0;
    plane = nx * ny;

    grid_x = pos_x / spacing;
    grid_y = pos_y / spacing;
    grid_z = pos_z / spacing;
    if (grid_x < 0.0) { grid_x = 0.0; }

    if (grid_y < 0.0) { grid_y = 0.0; }

    if (grid_z < 0.0) { grid_z = 0.0; }

    if (grid_x > (double)(nx - 1)) { grid_x = (double)(nx - 1); }

    if (grid_y > (double)(ny - 1)) { grid_y = (double)(ny - 1); }

    if (grid_z > (double)(nz - 1)) { grid_z = (double)(nz - 1); }

    idx_x0 = (uint64_t)ldg_math_floor_get(grid_x);
    idx_y0 = (uint64_t)ldg_math_floor_get(grid_y);
    idx_z0 = (uint64_t)ldg_math_floor_get(grid_z);
    idx_x1 = idx_x0 + 1;
    idx_y1 = idx_y0 + 1;
    idx_z1 = idx_z0 + 1;
    if (idx_x1 >= nx) { idx_x1 = nx - 1; }

    if (idx_y1 >= ny) { idx_y1 = ny - 1; }

    if (idx_z1 >= nz) { idx_z1 = nz - 1; }

    fraction_x = grid_x - (double)idx_x0;
    fraction_y = grid_y - (double)idx_y0;
    fraction_z = grid_z - (double)idx_z0;

    row_low_z0 = field[idx_z0 * plane + idx_y0 * nx + idx_x0] + fraction_x * (field[idx_z0 * plane + idx_y0 * nx + idx_x1] - field[idx_z0 * plane + idx_y0 * nx + idx_x0]);
    row_high_z0 = field[idx_z0 * plane + idx_y1 * nx + idx_x0] + fraction_x * (field[idx_z0 * plane + idx_y1 * nx + idx_x1] - field[idx_z0 * plane + idx_y1 * nx + idx_x0]);
    row_low_z1 = field[idx_z1 * plane + idx_y0 * nx + idx_x0] + fraction_x * (field[idx_z1 * plane + idx_y0 * nx + idx_x1] - field[idx_z1 * plane + idx_y0 * nx + idx_x0]);
    row_high_z1 = field[idx_z1 * plane + idx_y1 * nx + idx_x0] + fraction_x * (field[idx_z1 * plane + idx_y1 * nx + idx_x1] - field[idx_z1 * plane + idx_y1 * nx + idx_x0]);
    face_z0 = row_low_z0 + fraction_y * (row_high_z0 - row_low_z0);
    face_z1 = row_low_z1 + fraction_y * (row_high_z1 - row_low_z1);
    *out = face_z0 + fraction_z * (face_z1 - face_z0);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_field_3d_src_add(double *field, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double pos_x, double pos_y, double pos_z, double amount)
{
    uint64_t idx_x0 = 0;
    uint64_t idx_x1 = 0;
    uint64_t idx_y0 = 0;
    uint64_t idx_y1 = 0;
    uint64_t idx_z0 = 0;
    uint64_t idx_z1 = 0;
    uint64_t plane = 0;
    double grid_x = 0.0;
    double grid_y = 0.0;
    double grid_z = 0.0;
    double fraction_x = 0.0;
    double fraction_y = 0.0;
    double fraction_z = 0.0;
    double weight_x_low = 0.0;
    double weight_x_high = 0.0;
    double weight_y_low = 0.0;
    double weight_y_high = 0.0;
    double weight_z_low = 0.0;
    double weight_z_high = 0.0;

    if (LDG_UNLIKELY(!field)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(nx < 1 || ny < 1 || nz < 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    plane = nx * ny;

    grid_x = pos_x / spacing;
    grid_y = pos_y / spacing;
    grid_z = pos_z / spacing;
    if (grid_x < 0.0) { grid_x = 0.0; }

    if (grid_y < 0.0) { grid_y = 0.0; }

    if (grid_z < 0.0) { grid_z = 0.0; }

    if (grid_x > (double)(nx - 1)) { grid_x = (double)(nx - 1); }

    if (grid_y > (double)(ny - 1)) { grid_y = (double)(ny - 1); }

    if (grid_z > (double)(nz - 1)) { grid_z = (double)(nz - 1); }

    idx_x0 = (uint64_t)ldg_math_floor_get(grid_x);
    idx_y0 = (uint64_t)ldg_math_floor_get(grid_y);
    idx_z0 = (uint64_t)ldg_math_floor_get(grid_z);
    idx_x1 = idx_x0 + 1;
    idx_y1 = idx_y0 + 1;
    idx_z1 = idx_z0 + 1;
    if (idx_x1 >= nx) { idx_x1 = nx - 1; }

    if (idx_y1 >= ny) { idx_y1 = ny - 1; }

    if (idx_z1 >= nz) { idx_z1 = nz - 1; }

    fraction_x = grid_x - (double)idx_x0;
    fraction_y = grid_y - (double)idx_y0;
    fraction_z = grid_z - (double)idx_z0;
    weight_x_high = fraction_x;
    weight_x_low = 1.0 - fraction_x;
    weight_y_high = fraction_y;
    weight_y_low = 1.0 - fraction_y;
    weight_z_high = fraction_z;
    weight_z_low = 1.0 - fraction_z;

    field[idx_z0 * plane + idx_y0 * nx + idx_x0] += weight_x_low * weight_y_low * weight_z_low * amount;
    field[idx_z0 * plane + idx_y0 * nx + idx_x1] += weight_x_high * weight_y_low * weight_z_low * amount;
    field[idx_z0 * plane + idx_y1 * nx + idx_x0] += weight_x_low * weight_y_high * weight_z_low * amount;
    field[idx_z0 * plane + idx_y1 * nx + idx_x1] += weight_x_high * weight_y_high * weight_z_low * amount;
    field[idx_z1 * plane + idx_y0 * nx + idx_x0] += weight_x_low * weight_y_low * weight_z_high * amount;
    field[idx_z1 * plane + idx_y0 * nx + idx_x1] += weight_x_high * weight_y_low * weight_z_high * amount;
    field[idx_z1 * plane + idx_y1 * nx + idx_x0] += weight_x_low * weight_y_high * weight_z_high * amount;
    field[idx_z1 * plane + idx_y1 * nx + idx_x1] += weight_x_high * weight_y_high * weight_z_high * amount;

    return LDG_ERR_AOK;
}

static void ldg_math_phys_mask_bit_set(uint64_t *mask, uint32_t comp_id)
{
    mask[comp_id >> 6] |= (UINT64_C(1) << (comp_id & 63));
}

static void ldg_math_phys_name_copy(char *dst, const char *src)
{
    uint32_t idx = 0;
    while (src[idx] != '\0' && idx < (LDG_MATH_PHYS_PARTICLE_NAME_MAX - 1))
    {
        dst[idx] = src[idx];
        idx++;
    }
    dst[idx] = '\0';
}

static uint32_t ldg_math_phys_int64_to_conserved(int64_t quanta, ldg_math_phys_conserved_t *out)
{
    ldg_math_phys_conserved_t zero_val = LDG_STRUCT_ZERO_INIT;
    *out = zero_val;
    if (quanta >= 0)
    {
        out->magnitude.limb[0] = (uint64_t)quanta;
        return LDG_ERR_AOK;
    }

    out->magnitude.limb[0] = (uint64_t)(0 - (quanta + 1)) + 1;
    out->sign = 1;
    return LDG_ERR_AOK;
}

static void ldg_math_phys_particle_tag_get(uint32_t kind, int64_t *charge, int64_t *lepton, int64_t *baryon)
{
    *charge = 0;
    *lepton = 0;
    *baryon = 0;
    switch (kind)
    {
        case LDG_MATH_PHYS_PARTICLE_UP:
        case LDG_MATH_PHYS_PARTICLE_CHARM:
        case LDG_MATH_PHYS_PARTICLE_TOP:
            *charge = 2;
            *baryon = 1;
            break;
        case LDG_MATH_PHYS_PARTICLE_DOWN:
        case LDG_MATH_PHYS_PARTICLE_STRANGE:
        case LDG_MATH_PHYS_PARTICLE_BOTTOM:
            *charge = 0 - 1;
            *baryon = 1;
            break;
        case LDG_MATH_PHYS_PARTICLE_ELECTRON:
        case LDG_MATH_PHYS_PARTICLE_MUON:
        case LDG_MATH_PHYS_PARTICLE_TAU:
            *charge = 0 - 3;
            *lepton = 1;
            break;
        case LDG_MATH_PHYS_PARTICLE_ELECTRON_NEUTRINO:
        case LDG_MATH_PHYS_PARTICLE_MUON_NEUTRINO:
        case LDG_MATH_PHYS_PARTICLE_TAU_NEUTRINO:
            *lepton = 1;
            break;
        case LDG_MATH_PHYS_PARTICLE_W:
            *charge = 3;
            break;
        default:
            break;
    }
}

static int64_t ldg_math_phys_region_exponent_get(const ldg_math_phys_region_t *region, uint32_t quantity)
{
    switch (quantity)
    {
        case LDG_MATH_PHYS_QUANTITY_MASS:
            return region->desc.mass_scale_exponent - LDG_MATH_PHYS_MASS_FRACTION;
        case LDG_MATH_PHYS_QUANTITY_ENERGY:
            return region->desc.energy_scale_exponent - LDG_MATH_PHYS_ENERGY_FRACTION;
        case LDG_MATH_PHYS_QUANTITY_MOMENTUM_X:
        case LDG_MATH_PHYS_QUANTITY_MOMENTUM_Y:
        case LDG_MATH_PHYS_QUANTITY_MOMENTUM_Z:
            return region->desc.momentum_scale_exponent - LDG_MATH_PHYS_MOMENTUM_FRACTION;
        case LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_X:
        case LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_Y:
        case LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_Z:
            return region->desc.angular_momentum_scale_exponent - LDG_MATH_PHYS_ANGULAR_MOMENTUM_FRACTION;
        default:
            return 0;
    }
}

static uint32_t ldg_math_phys_init_undo(ldg_math_phys_ctx_t *ctx, uint32_t comp_reg_cunt, uint32_t taxon_reg_cunt)
{
    uint32_t undo_err = 0;
    uint32_t step_err = 0;
    uint32_t idx = 0;
    for (idx = 0; idx < taxon_reg_cunt; idx++)
    {
        step_err = ldg_ecs_taxon_unreg(&ctx->world->taxon[ctx->conf.taxon_id_base + idx]);
        if (step_err != LDG_ERR_AOK && undo_err == LDG_ERR_AOK) { undo_err = step_err; }
    }
    for (idx = 0; idx < comp_reg_cunt; idx++)
    {
        step_err = ldg_ecs_comp_unreg(&ctx->world->comp[ctx->conf.comp_id_base + idx]);
        if (step_err != LDG_ERR_AOK && undo_err == LDG_ERR_AOK) { undo_err = step_err; }
    }
    if (ctx->region != 0x0)
    {
        for (idx = 0; idx < ctx->conf.region_cap; idx++) { if (ctx->region[idx].used && ctx->region[idx].field != 0x0)
            {
                step_err = ldg_mem_dealloc(ctx->region[idx].field);
                if (step_err != LDG_ERR_AOK && undo_err == LDG_ERR_AOK) { undo_err = step_err; }
            }
        }
        step_err = ldg_mem_dealloc(ctx->region);
        if (step_err != LDG_ERR_AOK && undo_err == LDG_ERR_AOK) { undo_err = step_err; }
    }

    if (ctx->aggregate != 0x0)
    {
        step_err = ldg_mem_dealloc(ctx->aggregate);
        if (step_err != LDG_ERR_AOK && undo_err == LDG_ERR_AOK) { undo_err = step_err; }
    }

    if (ctx->constraint != 0x0)
    {
        step_err = ldg_mem_dealloc(ctx->constraint);
        if (step_err != LDG_ERR_AOK && undo_err == LDG_ERR_AOK) { undo_err = step_err; }
    }

    if (ctx->collider != 0x0)
    {
        step_err = ldg_mem_dealloc(ctx->collider);
        if (step_err != LDG_ERR_AOK && undo_err == LDG_ERR_AOK) { undo_err = step_err; }
    }

    if (ctx->history != 0x0)
    {
        step_err = ldg_mem_dealloc(ctx->history);
        if (step_err != LDG_ERR_AOK && undo_err == LDG_ERR_AOK) { undo_err = step_err; }
    }

    if (ctx->atmosphere != 0x0)
    {
        step_err = ldg_mem_dealloc(ctx->atmosphere);
        if (step_err != LDG_ERR_AOK && undo_err == LDG_ERR_AOK) { undo_err = step_err; }
    }

    return undo_err;
}

static void ldg_math_phys_particle_seed_set(ldg_math_phys_particle_t *particle, uint32_t idx)
{
    particle->mass = 0.0;
    particle->charge = 0.0;
    particle->spin = 0.5;
    switch (idx)
    {
        case LDG_MATH_PHYS_PARTICLE_PHOTON:
            ldg_math_phys_name_copy(particle->name, "photon");
            particle->spin = 1.0;
            break;
        case LDG_MATH_PHYS_PARTICLE_GLUON:
            ldg_math_phys_name_copy(particle->name, "gluon");
            particle->spin = 1.0;
            break;
        case LDG_MATH_PHYS_PARTICLE_W:
            ldg_math_phys_name_copy(particle->name, "w");
            particle->mass = LDG_MATH_PHYS_PARTICLE_MASS_W;
            particle->charge = 1.0;
            particle->spin = 1.0;
            break;
        case LDG_MATH_PHYS_PARTICLE_Z:
            ldg_math_phys_name_copy(particle->name, "z");
            particle->mass = LDG_MATH_PHYS_PARTICLE_MASS_Z;
            particle->spin = 1.0;
            break;
        case LDG_MATH_PHYS_PARTICLE_HIGGS:
            ldg_math_phys_name_copy(particle->name, "higgs");
            particle->mass = LDG_MATH_PHYS_PARTICLE_MASS_HIGGS;
            particle->spin = 0.0;
            break;
        case LDG_MATH_PHYS_PARTICLE_UP:
            ldg_math_phys_name_copy(particle->name, "up");
            particle->mass = LDG_MATH_PHYS_PARTICLE_MASS_UP;
            particle->charge = 2.0 / 3.0;
            break;
        case LDG_MATH_PHYS_PARTICLE_CHARM:
            ldg_math_phys_name_copy(particle->name, "charm");
            particle->mass = LDG_MATH_PHYS_PARTICLE_MASS_CHARM;
            particle->charge = 2.0 / 3.0;
            break;
        case LDG_MATH_PHYS_PARTICLE_TOP:
            ldg_math_phys_name_copy(particle->name, "top");
            particle->mass = LDG_MATH_PHYS_PARTICLE_MASS_TOP;
            particle->charge = 2.0 / 3.0;
            break;
        case LDG_MATH_PHYS_PARTICLE_DOWN:
            ldg_math_phys_name_copy(particle->name, "down");
            particle->mass = LDG_MATH_PHYS_PARTICLE_MASS_DOWN;
            particle->charge = 0.0 - (1.0 / 3.0);
            break;
        case LDG_MATH_PHYS_PARTICLE_STRANGE:
            ldg_math_phys_name_copy(particle->name, "strange");
            particle->mass = LDG_MATH_PHYS_PARTICLE_MASS_STRANGE;
            particle->charge = 0.0 - (1.0 / 3.0);
            break;
        case LDG_MATH_PHYS_PARTICLE_BOTTOM:
            ldg_math_phys_name_copy(particle->name, "bottom");
            particle->mass = LDG_MATH_PHYS_PARTICLE_MASS_BOTTOM;
            particle->charge = 0.0 - (1.0 / 3.0);
            break;
        case LDG_MATH_PHYS_PARTICLE_ELECTRON:
            ldg_math_phys_name_copy(particle->name, "electron");
            particle->mass = LDG_MATH_PHYS_PARTICLE_MASS_ELECTRON;
            particle->charge = 0.0 - 1.0;
            break;
        case LDG_MATH_PHYS_PARTICLE_MUON:
            ldg_math_phys_name_copy(particle->name, "muon");
            particle->mass = LDG_MATH_PHYS_PARTICLE_MASS_MUON;
            particle->charge = 0.0 - 1.0;
            break;
        case LDG_MATH_PHYS_PARTICLE_TAU:
            ldg_math_phys_name_copy(particle->name, "tau");
            particle->mass = LDG_MATH_PHYS_PARTICLE_MASS_TAU;
            particle->charge = 0.0 - 1.0;
            break;
        case LDG_MATH_PHYS_PARTICLE_ELECTRON_NEUTRINO:
            ldg_math_phys_name_copy(particle->name, "electron_neutrino");
            break;
        case LDG_MATH_PHYS_PARTICLE_MUON_NEUTRINO:
            ldg_math_phys_name_copy(particle->name, "muon_neutrino");
            break;
        case LDG_MATH_PHYS_PARTICLE_TAU_NEUTRINO:
            ldg_math_phys_name_copy(particle->name, "tau_neutrino");
            break;
        default:
            particle->name[0] = '\0';
            break;
    }
}

static void ldg_math_phys_comp_desc_set(ldg_ecs_comp_desc_t *desc, uint32_t idx, uint32_t cap)
{
    desc->item_size = sizeof(uint32_t);
    desc->align = 4;
    desc->cap = cap;
    switch (idx)
    {
        case LDG_MATH_PHYS_COMP_MASS:
        case LDG_MATH_PHYS_COMP_ENERGY:
        case LDG_MATH_PHYS_COMP_CHARGE:
            desc->item_size = sizeof(ldg_math_phys_conserved_t);
            desc->align = 8;
            break;
        case LDG_MATH_PHYS_COMP_MOMENTUM:
        case LDG_MATH_PHYS_COMP_ANGULAR_MOMENTUM:
            desc->item_size = sizeof(ldg_math_phys_conserved_t) * LDG_MATH_PHYS_DIM;
            desc->align = 8;
            break;
        case LDG_MATH_PHYS_COMP_POS:
            desc->item_size = sizeof(ldg_uint128_t) * LDG_MATH_PHYS_DIM;
            desc->align = 8;
            break;
        case LDG_MATH_PHYS_COMP_ORIENTATION:
            desc->item_size = sizeof(double) * LDG_MATH_QUATERNION_DIM;
            desc->align = 8;
            break;
        case LDG_MATH_PHYS_COMP_SPIN:
            desc->item_size = sizeof(double) * LDG_MATH_PHYS_DIM;
            desc->align = 8;
            break;
        case LDG_MATH_PHYS_COMP_INERTIA:
            desc->item_size = sizeof(double);
            desc->align = 8;
            break;
        case LDG_MATH_PHYS_COMP_TAG:
            desc->item_size = sizeof(ldg_math_phys_tag_t);
            desc->align = 8;
            break;
        default:
            break;
    }
}

typedef struct ldg_math_phys_body_val
{
    ldg_math_phys_conserved_t mass_c;
    ldg_math_phys_conserved_t energy_c;
    ldg_math_phys_conserved_t momentum_c[LDG_MATH_PHYS_DIM];
    ldg_math_phys_conserved_t angular_c[LDG_MATH_PHYS_DIM];
    ldg_math_phys_conserved_t charge_c;
    ldg_uint128_t pos_fixed[LDG_MATH_PHYS_DIM];
    double orientation[LDG_MATH_QUATERNION_DIM];
    double spin[LDG_MATH_PHYS_DIM];
    double inertia;
} ldg_math_phys_body_val_t;

static uint32_t ldg_math_phys_body_convert(const ldg_math_phys_region_t *region_ptr, const ldg_math_phys_body_desc_t *desc, double mass_val, int64_t charge_quanta, ldg_math_phys_body_val_t *out)
{
    double momentum_d[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double angular_d[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double energy_d = 0.0;
    uint32_t err = 0;
    uint32_t axis = 0;
    err = ldg_mem_secure_zero(out, sizeof(ldg_math_phys_body_val_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(mass_val) || mass_val < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->pos[axis]) || desc->pos[axis] < 0.0 || desc->pos[axis] >= region_ptr->desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->velocity[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->spin[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; }
    }
    for (axis = 0; axis < LDG_MATH_QUATERNION_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(desc->orientation[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; } }
    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->inertia) || desc->inertia < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->inertia > 0.0 && !(mass_val > 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ldg_math_abs_get(desc->spin[0]) + ldg_math_abs_get(desc->spin[1]) + ldg_math_abs_get(desc->spin[2]) > 0.0 && !(desc->inertia > 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ldg_math_abs_get(desc->orientation[0]) + ldg_math_abs_get(desc->orientation[1]) + ldg_math_abs_get(desc->orientation[2]) + ldg_math_abs_get(desc->orientation[3]) > 0.0)
    {
        err = ldg_math_quaternion_normalize(desc->orientation, out->orientation);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_FUNC_ARG_INVALID; }
    }
    else { out->orientation[LDG_MATH_QUATERNION_DIM - 1] = 1.0; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { out->spin[axis] = desc->spin[axis]; }
    out->inertia = desc->inertia;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { momentum_d[axis] = mass_val * desc->velocity[axis]; }
    energy_d = 0.5 * mass_val * (desc->velocity[0] * desc->velocity[0] + desc->velocity[1] * desc->velocity[1] + desc->velocity[2] * desc->velocity[2]) + 0.5 * desc->inertia * (desc->spin[0] * desc->spin[0] + desc->spin[1] * desc->spin[1] + desc->spin[2] * desc->spin[2]);
    angular_d[0] = desc->pos[1] * momentum_d[2] - desc->pos[2] * momentum_d[1] + desc->inertia * desc->spin[0];
    angular_d[1] = desc->pos[2] * momentum_d[0] - desc->pos[0] * momentum_d[2] + desc->inertia * desc->spin[1];
    angular_d[2] = desc->pos[0] * momentum_d[1] - desc->pos[1] * momentum_d[0] + desc->inertia * desc->spin[2];
    err = ldg_math_phys_double_to_conserved_convert(mass_val, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_MASS), &out->mass_c);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_double_to_conserved_convert(energy_d, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY), &out->energy_c);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_math_phys_double_to_conserved_convert(momentum_d[axis], ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_MOMENTUM_X), &out->momentum_c[axis]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_double_to_conserved_convert(angular_d[axis], ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_X), &out->angular_c[axis]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_arith_double_to_uint128_convert(ldg_math_pwr2_scale_get(desc->pos[axis], LDG_MATH_PHYS_POS_FRACTION), &out->pos_fixed[axis]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    return ldg_math_phys_int64_to_conserved(charge_quanta, &out->charge_c);
}

static uint32_t ldg_math_phys_region_total_add(ldg_math_phys_region_t *region_ptr, const ldg_math_phys_body_val_t *val, uint8_t neg)
{
    ldg_math_phys_conserved_t new_total[LDG_MATH_PHYS_QUANTITY_CUNT] = LDG_ARR_ZERO_INIT;
    ldg_math_phys_conserved_t term_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t sum_val = LDG_STRUCT_ZERO_INIT;
    const ldg_math_phys_conserved_t *src = 0x0;
    uint32_t err = 0;
    uint32_t idx = 0;
    for (idx = 0; idx < LDG_MATH_PHYS_QUANTITY_CUNT; idx++)
    {
        switch (idx)
        {
            case LDG_MATH_PHYS_QUANTITY_MASS:
                src = &val->mass_c;
                break;
            case LDG_MATH_PHYS_QUANTITY_ENERGY:
                src = &val->energy_c;
                break;
            case LDG_MATH_PHYS_QUANTITY_MOMENTUM_X:
            case LDG_MATH_PHYS_QUANTITY_MOMENTUM_Y:
            case LDG_MATH_PHYS_QUANTITY_MOMENTUM_Z:
                src = &val->momentum_c[idx - LDG_MATH_PHYS_QUANTITY_MOMENTUM_X];
                break;
            case LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_X:
            case LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_Y:
            case LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_Z:
                src = &val->angular_c[idx - LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_X];
                break;
            default:
                src = &val->charge_c;
                break;
        }
        if (neg)
        {
            err = ldg_math_phys_conserved_neg(src, &term_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
        }
        else { term_val = *src; }

        err = ldg_math_phys_conserved_add(&region_ptr->total[idx], &term_val, &sum_val);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        new_total[idx] = sum_val;
    }
    for (idx = 0; idx < LDG_MATH_PHYS_QUANTITY_CUNT; idx++) { region_ptr->total[idx] = new_total[idx]; }
    return LDG_ERR_AOK;
}

typedef struct ldg_math_phys_history_hdr
{
    ldg_uint128_t clock_anchor;
    ldg_math_phys_body_desc_t fragment_desc[LDG_MATH_PHYS_FRAGMENT_MAX];
    uint64_t fragment_child[LDG_MATH_PHYS_FRAGMENT_MAX];
    uint32_t fragment_kind[LDG_MATH_PHYS_FRAGMENT_MAX];
    double wall_anchor;
    uint64_t hd;
    uint64_t cunt;
    uint8_t anchor_set;
    uint8_t pudding[7];
} ldg_math_phys_history_hdr_t;

typedef struct ldg_math_phys_history_meta
{
    ldg_uint128_t stamp;
} ldg_math_phys_history_meta_t;

typedef struct ldg_math_phys_history_slot
{
    double pos[LDG_MATH_PHYS_DIM];
    double velocity[LDG_MATH_PHYS_DIM];
    double orientation[LDG_MATH_QUATERNION_DIM];
    uint64_t generation;
    uint8_t used;
    uint8_t pudding[7];
} ldg_math_phys_history_slot_t;

static uint64_t ldg_math_phys_history_size_get(uint32_t depth, uint32_t entity_cap)
{
    return sizeof(ldg_math_phys_history_hdr_t) + (uint64_t)depth * sizeof(ldg_math_phys_history_meta_t) + (uint64_t)depth * entity_cap * sizeof(ldg_math_phys_history_slot_t);
}

static ldg_math_phys_history_hdr_t* ldg_math_phys_history_hdr_get(const ldg_math_phys_ctx_t *ctx)
{
    return (ldg_math_phys_history_hdr_t *)ctx->history;
}

static ldg_math_phys_history_meta_t* ldg_math_phys_history_meta_get(const ldg_math_phys_ctx_t *ctx, uint32_t frame_idx)
{
    uint8_t *base = (uint8_t *)ctx->history;
    return (ldg_math_phys_history_meta_t *)(base + sizeof(ldg_math_phys_history_hdr_t)) + frame_idx;
}

static ldg_math_phys_history_slot_t* ldg_math_phys_history_slot_get(const ldg_math_phys_ctx_t *ctx, uint32_t frame_idx)
{
    uint8_t *base = (uint8_t *)ctx->history;
    uint64_t offset = sizeof(ldg_math_phys_history_hdr_t) + (uint64_t)ctx->conf.history_depth * sizeof(ldg_math_phys_history_meta_t);
    return (ldg_math_phys_history_slot_t *)(base + offset) + (uint64_t)frame_idx * ctx->world->conf.entity_cap;
}

static uint32_t ldg_math_phys_dt_get(const ldg_math_phys_ctx_t *ctx, double *out)
{
    double raw = 0.0;
    uint32_t err = 0;
    err = ldg_arith_uint128_to_double_convert(&ctx->conf.tick_budget, &raw);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    *out = ldg_math_pwr2_scale_get(raw, 0 - LDG_MATH_PHYS_TIME_FRACTION);
    return LDG_ERR_AOK;
}

typedef struct ldg_math_phys_body_motion
{
    double pos[LDG_MATH_PHYS_DIM];
    double velocity[LDG_MATH_PHYS_DIM];
    double momentum[LDG_MATH_PHYS_DIM];
    double mass;
    uint32_t region;
    uint8_t massive;
    uint8_t pudding[3];
} ldg_math_phys_body_motion_t;

static uint32_t ldg_math_phys_body_motion_get(const ldg_math_phys_ctx_t *ctx, uint64_t entity, ldg_math_phys_body_motion_t *out)
{
    const ldg_math_phys_region_t *region_ptr = 0x0;
    ldg_math_phys_conserved_t *conserved_ptr = 0x0;
    ldg_uint128_t *pos_ptr = 0x0;
    void *comp_ptr = 0x0;
    double raw = 0.0;
    uint32_t err = 0;
    uint32_t axis = 0;
    err = ldg_mem_secure_zero(out, sizeof(ldg_math_phys_body_motion_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    out->region = *(uint32_t *)comp_ptr;
    if (LDG_UNLIKELY(out->region >= ctx->conf.region_cap || !ctx->region[out->region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    region_ptr = &ctx->region[out->region];
    err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MASS, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_conserved_to_double_convert((ldg_math_phys_conserved_t *)comp_ptr, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_MASS), &out->mass);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MOMENTUM, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_math_phys_conserved_to_double_convert(&conserved_ptr[axis], ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_MOMENTUM_X), &out->momentum[axis]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_POS, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    pos_ptr = (ldg_uint128_t *)comp_ptr;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_arith_uint128_to_double_convert(&pos_ptr[axis], &raw);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        out->pos[axis] = ldg_math_pwr2_scale_get(raw, 0 - LDG_MATH_PHYS_POS_FRACTION);
    }
    if (out->mass > 0.0)
    {
        out->massive = 1;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { out->velocity[axis] = out->momentum[axis] / out->mass; }
    }

    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_momentum_quantize(const ldg_math_phys_region_t *region_ptr, const double *impulse, ldg_math_phys_conserved_t *out)
{
    uint32_t err = 0;
    uint32_t axis = 0;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_math_phys_double_to_conserved_convert(impulse[axis], ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_MOMENTUM_X), &out[axis]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_momentum_delta_apply(const ldg_math_phys_ctx_t *ctx, uint64_t entity, const ldg_math_phys_conserved_t *delta, uint8_t neg)
{
    ldg_math_phys_conserved_t term_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t sum_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t *momentum_ptr = 0x0;
    void *comp_ptr = 0x0;
    uint32_t err = 0;
    uint32_t axis = 0;
    err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MOMENTUM, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    momentum_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (neg)
        {
            err = ldg_math_phys_conserved_neg(&delta[axis], &term_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
        }
        else { term_val = delta[axis]; }

        err = ldg_math_phys_conserved_add(&momentum_ptr[axis], &term_val, &sum_val);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        momentum_ptr[axis] = sum_val;
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_region_total_cum(ldg_math_phys_region_t *region_ptr, uint32_t quantity, const ldg_math_phys_conserved_t *term)
{
    ldg_math_phys_conserved_t sum_val = LDG_STRUCT_ZERO_INIT;
    uint32_t err = 0;
    err = ldg_math_phys_conserved_add(&region_ptr->total[quantity], term, &sum_val);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_ptr->total[quantity] = sum_val;
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_region_momentum_inject(ldg_math_phys_region_t *region_ptr, const ldg_math_phys_conserved_t *delta)
{
    uint32_t err = 0;
    uint32_t axis = 0;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_math_phys_region_total_cum(region_ptr, LDG_MATH_PHYS_QUANTITY_MOMENTUM_X + axis, &delta[axis]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_step_accel_apply(ldg_math_phys_ctx_t *ctx, double dt)
{
    ldg_math_phys_body_motion_t motion = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t delta[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    ldg_math_phys_region_t *region_ptr = 0x0;
    double impulse[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t entity = 0;
    uint32_t err = 0;
    uint32_t dense_idx = 0;
    uint32_t axis = 0;
    for (dense_idx = 0; dense_idx < ctx->world->store.alive_cunt; dense_idx++)
    {
        entity = ctx->world->store.dense[dense_idx];
        if (ldg_ecs_ctx_comp_has(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION) != LDG_TRUTH_TRUE) { continue; }

        err = ldg_math_phys_body_motion_get(ctx, entity, &motion);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (!motion.massive) { continue; }

        region_ptr = &ctx->region[motion.region];
        if (region_ptr->desc.tier != LDG_MATH_PHYS_TIER_PARTICLE) { continue; }

        if (!(ldg_math_abs_get(region_ptr->accel[0]) + ldg_math_abs_get(region_ptr->accel[1]) + ldg_math_abs_get(region_ptr->accel[2]) > 0.0)) { continue; }

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { impulse[axis] = motion.mass * region_ptr->accel[axis] * dt; }
        err = ldg_math_phys_momentum_quantize(region_ptr, impulse, delta);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_momentum_delta_apply(ctx, entity, delta, 0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_region_momentum_inject(region_ptr, delta);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_step_constraint_apply(ldg_math_phys_ctx_t *ctx, double dt)
{
    ldg_math_phys_body_motion_t motion_a = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_body_motion_t motion_b = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t delta[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    ldg_math_phys_region_t *region_ptr = 0x0;
    double impulse[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint32_t err = 0;
    uint32_t idx = 0;
    for (idx = 0; idx < ctx->conf.constraint_cap; idx++)
    {
        if (!ctx->constraint[idx].used) { continue; }

        if (ldg_ecs_entity_alive_is(&ctx->world->store, ctx->constraint[idx].body_a) != LDG_TRUTH_TRUE) { continue; }

        if (ldg_ecs_entity_alive_is(&ctx->world->store, ctx->constraint[idx].body_b) != LDG_TRUTH_TRUE) { continue; }

        err = ldg_math_phys_body_motion_get(ctx, ctx->constraint[idx].body_a, &motion_a);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_body_motion_get(ctx, ctx->constraint[idx].body_b, &motion_b);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (!motion_a.massive || !motion_b.massive) { continue; }

        region_ptr = &ctx->region[motion_a.region];
        if (region_ptr->desc.tier != LDG_MATH_PHYS_TIER_PARTICLE) { continue; }

        err = ldg_math_phys_particle_spring_impulse_get(motion_a.pos, motion_b.pos, motion_a.velocity, motion_b.velocity, ctx->constraint[idx].rest_len, ctx->constraint[idx].compliance, ctx->constraint[idx].damping, dt, impulse);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_momentum_quantize(region_ptr, impulse, delta);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_momentum_delta_apply(ctx, ctx->constraint[idx].body_a, delta, 0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_momentum_delta_apply(ctx, ctx->constraint[idx].body_b, delta, 1);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_step_body_advance(ldg_math_phys_ctx_t *ctx, double dt, uint32_t frame_idx)
{
    ldg_math_phys_body_motion_t motion = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_history_slot_t *slot_base = 0x0;
    ldg_math_phys_conserved_t delta[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    ldg_math_phys_conserved_t quantized = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t *conserved_ptr = 0x0;
    ldg_math_phys_region_t *region_ptr = 0x0;
    ldg_uint128_t *pos_ptr = 0x0;
    void *comp_ptr = 0x0;
    double impulse[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double shift[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double angular[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double *orientation_ptr = 0x0;
    double *spin_ptr = 0x0;
    double inertia_val = 0.0;
    double energy_val = 0.0;
    double bound = 0.0;
    uint64_t entity = 0;
    uint64_t slot_idx = 0;
    uint32_t err = 0;
    uint32_t dense_idx = 0;
    uint32_t axis = 0;
    uint32_t coll_idx = 0;
    uint32_t coll_cap = 0;
    slot_base = ldg_math_phys_history_slot_get(ctx, frame_idx);
    for (dense_idx = 0; dense_idx < ctx->world->store.alive_cunt; dense_idx++)
    {
        entity = ctx->world->store.dense[dense_idx];
        if (ldg_ecs_ctx_comp_has(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION) != LDG_TRUTH_TRUE) { continue; }

        err = ldg_math_phys_body_motion_get(ctx, entity, &motion);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        region_ptr = &ctx->region[motion.region];
        coll_cap = (region_ptr->desc.tier == LDG_MATH_PHYS_TIER_PARTICLE) ? ctx->conf.collider_cap : 0;
        err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_ORIENTATION, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        orientation_ptr = (double *)comp_ptr;
        err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_SPIN, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        spin_ptr = (double *)comp_ptr;
        err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_INERTIA, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        inertia_val = *(double *)comp_ptr;
        if (motion.massive)
        {
            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { motion.pos[axis] += motion.velocity[axis] * dt; }
            for (coll_idx = 0; coll_idx < coll_cap; coll_idx++)
            {
                if (!ctx->collider[coll_idx].used) { continue; }

                if (ctx->collider[coll_idx].desc.shape == LDG_MATH_PHYS_COLLIDER_PLANE) { err = ldg_math_phys_particle_plane_contact_get(motion.pos, motion.velocity, motion.mass, &ctx->collider[coll_idx].desc, impulse, shift); }
                else { err = ldg_math_phys_particle_sphere_contact_get(motion.pos, motion.velocity, motion.mass, &ctx->collider[coll_idx].desc, impulse, shift); }

                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                if (ldg_math_abs_get(impulse[0]) + ldg_math_abs_get(impulse[1]) + ldg_math_abs_get(impulse[2]) > 0.0)
                {
                    err = ldg_math_phys_momentum_quantize(region_ptr, impulse, delta);
                    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                    err = ldg_math_phys_momentum_delta_apply(ctx, entity, delta, 0);
                    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                    err = ldg_math_phys_region_momentum_inject(region_ptr, delta);
                    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
                    {
                        motion.momentum[axis] += impulse[axis];
                        motion.velocity[axis] = motion.momentum[axis] / motion.mass;
                    }
                }

                for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { motion.pos[axis] += shift[axis]; }
            }
            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
            {
                bound = ldg_math_next_toward_get(region_ptr->desc.extent[axis], 0.0);
                if (motion.pos[axis] < 0.0) { motion.pos[axis] = 0.0; }

                if (motion.pos[axis] > bound) { motion.pos[axis] = bound; }
            }
            err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_POS, &comp_ptr);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            pos_ptr = (ldg_uint128_t *)comp_ptr;
            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
            {
                err = ldg_arith_double_to_uint128_convert(ldg_math_pwr2_scale_get(motion.pos[axis], LDG_MATH_PHYS_POS_FRACTION), &pos_ptr[axis]);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
            }
            err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MOMENTUM, &comp_ptr);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
            {
                err = ldg_math_phys_conserved_to_double_convert(&conserved_ptr[axis], ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_MOMENTUM_X), &motion.momentum[axis]);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                motion.velocity[axis] = motion.momentum[axis] / motion.mass;
            }
            if (ldg_math_abs_get(spin_ptr[0]) + ldg_math_abs_get(spin_ptr[1]) + ldg_math_abs_get(spin_ptr[2]) > 0.0)
            {
                err = ldg_math_quaternion_integrate(orientation_ptr, spin_ptr, dt);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }
            }

            energy_val = (motion.momentum[0] * motion.momentum[0] + motion.momentum[1] * motion.momentum[1] + motion.momentum[2] * motion.momentum[2]) / (2.0 * motion.mass) + 0.5 * inertia_val * (spin_ptr[0] * spin_ptr[0] + spin_ptr[1] * spin_ptr[1] + spin_ptr[2] * spin_ptr[2]);
            err = ldg_math_phys_double_to_conserved_convert(energy_val, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY), &quantized);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_ENERGY, &comp_ptr);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            *(ldg_math_phys_conserved_t *)comp_ptr = quantized;
            err = ldg_math_phys_region_total_cum(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY, &quantized);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            angular[0] = motion.pos[1] * motion.momentum[2] - motion.pos[2] * motion.momentum[1] + inertia_val * spin_ptr[0];
            angular[1] = motion.pos[2] * motion.momentum[0] - motion.pos[0] * motion.momentum[2] + inertia_val * spin_ptr[1];
            angular[2] = motion.pos[0] * motion.momentum[1] - motion.pos[1] * motion.momentum[0] + inertia_val * spin_ptr[2];
            err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_ANGULAR_MOMENTUM, &comp_ptr);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
            {
                err = ldg_math_phys_double_to_conserved_convert(angular[axis], ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_X), &quantized);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                conserved_ptr[axis] = quantized;
                err = ldg_math_phys_region_total_cum(region_ptr, LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_X + axis, &quantized);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
            }
        }
        else
        {
            err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_ENERGY, &comp_ptr);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_math_phys_region_total_cum(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY, (ldg_math_phys_conserved_t *)comp_ptr);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
        }

        slot_idx = ldg_ecs_entity_idx_get(entity);
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            slot_base[slot_idx].pos[axis] = motion.pos[axis];
            slot_base[slot_idx].velocity[axis] = motion.velocity[axis];
        }
        for (axis = 0; axis < LDG_MATH_QUATERNION_DIM; axis++) { slot_base[slot_idx].orientation[axis] = orientation_ptr[axis]; }
        slot_base[slot_idx].generation = ldg_ecs_entity_generation_get(entity);
        slot_base[slot_idx].used = 1;
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_step_constraint_energy_cum(ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_body_motion_t motion_a = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_body_motion_t motion_b = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t quantized = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_region_t *region_ptr = 0x0;
    double energy_val = 0.0;
    uint32_t err = 0;
    uint32_t idx = 0;
    for (idx = 0; idx < ctx->conf.constraint_cap; idx++)
    {
        if (!ctx->constraint[idx].used) { continue; }

        if (ldg_ecs_entity_alive_is(&ctx->world->store, ctx->constraint[idx].body_a) != LDG_TRUTH_TRUE) { continue; }

        if (ldg_ecs_entity_alive_is(&ctx->world->store, ctx->constraint[idx].body_b) != LDG_TRUTH_TRUE) { continue; }

        err = ldg_math_phys_body_motion_get(ctx, ctx->constraint[idx].body_a, &motion_a);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_body_motion_get(ctx, ctx->constraint[idx].body_b, &motion_b);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (!motion_a.massive || !motion_b.massive) { continue; }

        region_ptr = &ctx->region[motion_a.region];
        if (region_ptr->desc.tier != LDG_MATH_PHYS_TIER_PARTICLE) { continue; }

        err = ldg_math_phys_particle_spring_energy_get(motion_a.pos, motion_b.pos, ctx->constraint[idx].rest_len, ctx->constraint[idx].compliance, &energy_val);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_double_to_conserved_convert(energy_val, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY), &quantized);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_region_total_cum(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY, &quantized);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    return LDG_ERR_AOK;
}

typedef struct ldg_math_phys_field_hdr
{
    ldg_math_phys_conserved_t momentum_account[LDG_MATH_PHYS_DIM];
    double emit;
    double absorb;
    uint64_t nx;
    uint64_t ny;
    uint64_t nz;
    uint64_t cell_cunt;
    uint64_t cur_idx;
    uint64_t prev_idx;
} ldg_math_phys_field_hdr_t;

static ldg_math_phys_field_hdr_t* ldg_math_phys_field_hdr_get(const ldg_math_phys_region_t *region_ptr)
{
    return (ldg_math_phys_field_hdr_t *)region_ptr->field;
}

static double* ldg_math_phys_field_buff_get(const ldg_math_phys_region_t *region_ptr, uint64_t buff_idx)
{
    uint8_t *base = (uint8_t *)region_ptr->field;
    ldg_math_phys_field_hdr_t *hdr = (ldg_math_phys_field_hdr_t *)region_ptr->field;
    return (double *)(base + sizeof(ldg_math_phys_field_hdr_t)) + buff_idx * hdr->cell_cunt;
}

static double* ldg_math_phys_field_snapshot_get(const ldg_math_phys_region_t *region_ptr, uint32_t frame_idx)
{
    uint8_t *base = (uint8_t *)region_ptr->field;
    ldg_math_phys_field_hdr_t *hdr = (ldg_math_phys_field_hdr_t *)region_ptr->field;
    return (double *)(base + sizeof(ldg_math_phys_field_hdr_t)) + (3 + (uint64_t)frame_idx) * hdr->cell_cunt;
}

#define LDG_MATH_PHYS_PASS_ACOUSTIC_VELOCITY 0
#define LDG_MATH_PHYS_PASS_ACOUSTIC_PRESSURE 1
#define LDG_MATH_PHYS_PASS_EM_ELECTRIC 2
#define LDG_MATH_PHYS_PASS_EM_MAGNETIC 3
#define LDG_MATH_PHYS_SLAB_MIN 4

typedef struct ldg_math_phys_slab_job
{
    double *field_a;
    const double *field_b;
    const double *drive;
    double *psi;
    const uint16_t *mtl;
    const double *coeff;
    const double *stretch;
    uint32_t *done_cuntr;
    double spacing;
    uint64_t nx;
    uint64_t ny;
    uint64_t nz;
    uint64_t slab_lo;
    uint64_t slab_hi;
    uint32_t pass;
    uint32_t worker_id;
    uint32_t ret;
    uint8_t pudding[4];
} ldg_math_phys_slab_job_t;

typedef struct ldg_math_phys_em_port
{
    double z_ref;
    double face_area;
    uint64_t base_cell;
    uint64_t stride_a;
    uint64_t stride_b;
    uint64_t grid_a;
    uint64_t grid_b;
    uint64_t mode_cunt;
    uint64_t mode_cap;
    uint32_t axis_a;
    uint32_t axis_b;
    uint8_t is_forward;
    uint8_t used;
    uint8_t pudding[6];
} ldg_math_phys_em_port_t;

typedef struct ldg_math_phys_em_hdr
{
    ldg_math_phys_conserved_t momentum_account[LDG_MATH_PHYS_DIM];
    double plane_sign;
    double plane_pol_sign;
    double plane_hpol_sign;
    double drive_cur;
    double drive_prev;
    double dt_sub;
    uint64_t plane_axis;
    uint64_t plane_pol_axis;
    uint64_t plane_hpol_axis;
    uint64_t tf_lo[LDG_MATH_PHYS_DIM];
    uint64_t tf_hi[LDG_MATH_PHYS_DIM];
    uint64_t aux_cunt;
    uint64_t nx;
    uint64_t ny;
    uint64_t nz;
    uint64_t cell_cunt;
    uint64_t mtl_cap;
    uint64_t mtl_cunt;
    uint64_t subcycle_cunt;
    uint64_t snapshot_depth;
    uint64_t snapshot_hd;
    uint64_t snapshot_cunt;
    uint64_t snapshot_stride;
    uint64_t energy_stride;
    uint64_t tick_cuntr;
    uint64_t pml_depth;
    uint64_t sheet_cunt;
    uint64_t face_cunt;
    uint64_t face_cap;
    double optic_wave_err_max;
    double optic_ray_err_max;
    uint64_t optic_bin_cunt;
    uint64_t optic_ord_cunt;
    uint64_t optic_src_cunt;
    uint64_t optic_lambda_cunt;
    ldg_math_phys_em_drive_t drive_slot[LDG_MATH_PHYS_EM_DRIVE_CAP];
    uint64_t drive_cunt;
    uint64_t conductor_offset;
    uint64_t conductor_cap;
    uint64_t conductor_cunt;
    ldg_math_phys_em_port_t port_slot[LDG_MATH_PHYS_EM_PORT_CAP];
    uint64_t port_cunt;
    uint64_t port_cap;
    uint64_t port_pattern_offset;
    uint64_t port_tap_offset;
    uint64_t port_tap_depth;
    uint64_t port_tap_hd;
    uint64_t port_tap_cunt;
    uint64_t face_max;
    uint8_t plane_armed;
    uint8_t pudding[7];
} ldg_math_phys_em_hdr_t;

#define LDG_MATH_PHYS_OPTIC_SRC_CAP 32
#define LDG_MATH_PHYS_OPTIC_BIN_MAX 64
#define LDG_MATH_PHYS_OPTIC_LEDGER_STRIDE 4
#define LDG_MATH_PHYS_ORDINATE_STRIDE 4
#define LDG_MATH_PHYS_ACOUSTIC_SNAPSHOT_STRIDE 4
#define LDG_MATH_PHYS_ACOUSTIC_CELL_GRID_CUNT 5
#define LDG_MATH_PHYS_OPTIC_MTL_STRIDE 3
#define LDG_MATH_PHYS_THERMO_ORD_CUNT 8
#define LDG_MATH_PHYS_THERMO_RADIANCE_TOL 1.0e-10
#define LDG_MATH_PHYS_THERMO_RADIANCE_PASS_MAX 256
#define LDG_MATH_PHYS_THERMO_CELL_GRID_CUNT 6
#define LDG_MATH_PHYS_WATER_PARTICLE_PER_CELL 8
#define LDG_MATH_PHYS_WATER_GRAVITY (0.0 - 9.81)
#define LDG_MATH_PHYS_WATER_DENSITY 1000.0
#define LDG_MATH_PHYS_WATER_FACE_GRID_CUNT 9
#define LDG_MATH_PHYS_WATER_CELL_GRID_CUNT 2
#define LDG_MATH_PHYS_WATER_ITER_DEFAULT 80
#define LDG_MATH_PHYS_WATER_FLIP_DEFAULT 0.9

typedef struct ldg_math_phys_water_hdr
{
    double flip;
    double refract;
    double absorb[LDG_MATH_PHYS_WATER_OPTIC_BIN_MAX];
    uint64_t optic_bin_cunt;
    uint64_t nx;
    uint64_t ny;
    uint64_t nz;
    uint64_t cell_cunt;
    uint64_t face_max;
    uint64_t particle_cap;
    uint64_t particle_cunt;
    uint64_t iter_cunt;
} ldg_math_phys_water_hdr_t;

static ldg_math_phys_water_hdr_t* ldg_math_phys_water_hdr_get(const ldg_math_phys_region_t *region_ptr)
{
    return (ldg_math_phys_water_hdr_t *)region_ptr->field;
}

static double* ldg_math_phys_water_particle_get(const ldg_math_phys_region_t *region_ptr)
{
    return (double *)((uint8_t *)region_ptr->field + sizeof(ldg_math_phys_water_hdr_t));
}

static double* ldg_math_phys_water_vel_get(const ldg_math_phys_region_t *region_ptr, uint64_t slot)
{
    ldg_math_phys_water_hdr_t *hdr = ldg_math_phys_water_hdr_get(region_ptr);
    return ldg_math_phys_water_particle_get(region_ptr) + hdr->particle_cap * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE + slot * hdr->face_max;
}

static double* ldg_math_phys_water_pressure_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_water_hdr_t *hdr = ldg_math_phys_water_hdr_get(region_ptr);
    return ldg_math_phys_water_particle_get(region_ptr) + hdr->particle_cap * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE + LDG_MATH_PHYS_WATER_FACE_GRID_CUNT * hdr->face_max;
}

static double* ldg_math_phys_water_divergence_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_water_hdr_t *hdr = ldg_math_phys_water_hdr_get(region_ptr);
    return ldg_math_phys_water_pressure_get(region_ptr) + hdr->cell_cunt;
}

static uint16_t* ldg_math_phys_water_mask_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_water_hdr_t *hdr = ldg_math_phys_water_hdr_get(region_ptr);
    return (uint16_t *)(ldg_math_phys_water_divergence_get(region_ptr) + hdr->cell_cunt);
}

static uint16_t* ldg_math_phys_water_scratch_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_water_hdr_t *hdr = ldg_math_phys_water_hdr_get(region_ptr);
    return (uint16_t *)((uint8_t *)ldg_math_phys_water_mask_get(region_ptr) + LDG_ALIGNED_UP(hdr->cell_cunt * sizeof(uint16_t)));
}

static uint32_t ldg_math_phys_water_size_get(uint64_t cell_cunt, uint64_t face_max, uint64_t *out)
{
    uint64_t double_cunt = 0;
    uint64_t term = 0;
    uint32_t err = 0;
    *out = 0;
    err = ldg_arith_uint64_mul(cell_cunt, LDG_MATH_PHYS_WATER_PARTICLE_PER_CELL * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE + LDG_MATH_PHYS_WATER_CELL_GRID_CUNT, &double_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(face_max, LDG_MATH_PHYS_WATER_FACE_GRID_CUNT, &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(double_cunt, term, &double_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(double_cunt, sizeof(double), &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(term, sizeof(ldg_math_phys_water_hdr_t) + 2 * LDG_ALIGNED_UP(cell_cunt * sizeof(uint16_t)), out);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    return LDG_ERR_AOK;
}

typedef struct ldg_math_phys_thermo_hdr
{
    double ambient;
    double escaped_cum;
    double injected_cum;
    uint64_t nx;
    uint64_t ny;
    uint64_t nz;
    uint64_t cell_cunt;
    uint64_t mtl_cap;
    uint64_t mtl_cunt;
} ldg_math_phys_thermo_hdr_t;

static ldg_math_phys_thermo_hdr_t* ldg_math_phys_thermo_hdr_get(const ldg_math_phys_region_t *region_ptr)
{
    return (ldg_math_phys_thermo_hdr_t *)region_ptr->field;
}

static double* ldg_math_phys_thermo_coeff_get(const ldg_math_phys_region_t *region_ptr)
{
    return (double *)((uint8_t *)region_ptr->field + sizeof(ldg_math_phys_thermo_hdr_t));
}

static double* ldg_math_phys_thermo_em_coeff_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_thermo_hdr_t *hdr = ldg_math_phys_thermo_hdr_get(region_ptr);
    return ldg_math_phys_thermo_coeff_get(region_ptr) + hdr->mtl_cap * LDG_MATH_PHYS_THERMO_COEFF_STRIDE;
}

static double* ldg_math_phys_thermo_scatter_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_thermo_hdr_t *hdr = ldg_math_phys_thermo_hdr_get(region_ptr);
    return ldg_math_phys_thermo_em_coeff_get(region_ptr) + hdr->mtl_cap * LDG_MATH_PHYS_EM_COEFF_STRIDE;
}

static double* ldg_math_phys_thermo_ordinate_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_thermo_hdr_t *hdr = ldg_math_phys_thermo_hdr_get(region_ptr);
    return ldg_math_phys_thermo_scatter_get(region_ptr) + hdr->mtl_cap;
}

static double* ldg_math_phys_thermo_hold_get(const ldg_math_phys_region_t *region_ptr)
{
    return ldg_math_phys_thermo_ordinate_get(region_ptr) + LDG_MATH_PHYS_THERMO_ORD_CUNT * LDG_MATH_PHYS_ORDINATE_STRIDE;
}

static double* ldg_math_phys_thermo_temp_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_thermo_hdr_t *hdr = ldg_math_phys_thermo_hdr_get(region_ptr);
    return ldg_math_phys_thermo_hold_get(region_ptr) + hdr->cell_cunt;
}

static double* ldg_math_phys_thermo_temp_next_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_thermo_hdr_t *hdr = ldg_math_phys_thermo_hdr_get(region_ptr);
    return ldg_math_phys_thermo_temp_get(region_ptr) + hdr->cell_cunt;
}

static double* ldg_math_phys_thermo_emission_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_thermo_hdr_t *hdr = ldg_math_phys_thermo_hdr_get(region_ptr);
    return ldg_math_phys_thermo_temp_next_get(region_ptr) + hdr->cell_cunt;
}

static double* ldg_math_phys_thermo_flux_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_thermo_hdr_t *hdr = ldg_math_phys_thermo_hdr_get(region_ptr);
    return ldg_math_phys_thermo_emission_get(region_ptr) + hdr->cell_cunt;
}

static double* ldg_math_phys_thermo_radiance_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_thermo_hdr_t *hdr = ldg_math_phys_thermo_hdr_get(region_ptr);
    return ldg_math_phys_thermo_flux_get(region_ptr) + hdr->cell_cunt;
}

static double* ldg_math_phys_thermo_pwr_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_thermo_hdr_t *hdr = ldg_math_phys_thermo_hdr_get(region_ptr);
    return ldg_math_phys_thermo_radiance_get(region_ptr) + LDG_MATH_PHYS_THERMO_ORD_CUNT * hdr->cell_cunt;
}

static uint16_t* ldg_math_phys_thermo_mtl_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_thermo_hdr_t *hdr = ldg_math_phys_thermo_hdr_get(region_ptr);
    return (uint16_t *)(ldg_math_phys_thermo_pwr_get(region_ptr) + hdr->cell_cunt);
}

static uint32_t ldg_math_phys_thermo_size_get(uint64_t cell_cunt, uint64_t mtl_cap, uint64_t *out)
{
    uint64_t double_cunt = 0;
    uint64_t term = 0;
    uint32_t err = 0;
    *out = 0;
    err = ldg_arith_uint64_mul(mtl_cap, LDG_MATH_PHYS_THERMO_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_STRIDE + 1, &double_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(double_cunt, LDG_MATH_PHYS_THERMO_ORD_CUNT * LDG_MATH_PHYS_ORDINATE_STRIDE, &double_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(cell_cunt, LDG_MATH_PHYS_THERMO_CELL_GRID_CUNT + LDG_MATH_PHYS_THERMO_ORD_CUNT, &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(double_cunt, term, &double_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(double_cunt, sizeof(double), &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(term, sizeof(ldg_math_phys_thermo_hdr_t), &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(term, LDG_ALIGNED_UP(cell_cunt * sizeof(uint16_t)), out);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    return LDG_ERR_AOK;
}

typedef struct ldg_math_phys_em_sheet
{
    double sigma;
    double thickness;
    double mu_val;
    double residue[LDG_MATH_PHYS_SHEET_POLE_MAX];
    double pole[LDG_MATH_PHYS_SHEET_POLE_MAX];
    double decay[LDG_MATH_PHYS_SHEET_POLE_MAX];
    double gain[LDG_MATH_PHYS_SHEET_POLE_MAX];
    double inst;
    uint64_t pole_cunt;
    uint8_t used;
    uint8_t pudding[7];
} ldg_math_phys_em_sheet_t;

typedef struct ldg_math_phys_em_face
{
    double psi[2][LDG_MATH_PHYS_SHEET_POLE_MAX];
    double ca_mod;
    double cb_mod;
    double e_old[2];
    double flow[2];
    uint64_t cell;
    uint32_t sheet;
    uint32_t normal_axis;
} ldg_math_phys_em_face_t;

static ldg_math_phys_em_hdr_t* ldg_math_phys_em_hdr_get(const ldg_math_phys_region_t *region_ptr)
{
    return (ldg_math_phys_em_hdr_t *)region_ptr->field;
}

static double* ldg_math_phys_em_coeff_get(const ldg_math_phys_region_t *region_ptr)
{
    return (double *)((uint8_t *)region_ptr->field + sizeof(ldg_math_phys_em_hdr_t));
}

static ldg_uint128_t* ldg_math_phys_em_stamp_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return (ldg_uint128_t *)(ldg_math_phys_em_coeff_get(region_ptr) + hdr->mtl_cap * LDG_MATH_PHYS_EM_COEFF_STRIDE);
}

static double* ldg_math_phys_em_electric_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return (double *)(ldg_math_phys_em_stamp_get(region_ptr) + hdr->snapshot_depth);
}

static double* ldg_math_phys_em_magnetic_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_electric_get(region_ptr) + 3 * hdr->cell_cunt;
}

static double* ldg_math_phys_em_drive_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_magnetic_get(region_ptr) + 3 * hdr->cell_cunt;
}

static uint16_t* ldg_math_phys_em_mtl_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return (uint16_t *)(ldg_math_phys_em_drive_get(region_ptr) + 3 * hdr->cell_cunt);
}

static double* ldg_math_phys_em_snapshot_get(const ldg_math_phys_region_t *region_ptr, uint64_t slot)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    uint64_t mtl_bytes = LDG_ALIGNED_UP(hdr->cell_cunt * sizeof(uint16_t));
    return (double *)((uint8_t *)ldg_math_phys_em_mtl_get(region_ptr) + mtl_bytes) + slot * 6 * hdr->cell_cunt;
}

static double* ldg_math_phys_em_stretch_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_snapshot_get(region_ptr, hdr->snapshot_depth);
}

static double* ldg_math_phys_em_psi_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_stretch_get(region_ptr) + LDG_MATH_PHYS_EM_STRETCH_STRIDE * (hdr->nx + hdr->ny + hdr->nz);
}

static void ldg_math_phys_em_stretch_set(double *stretch, uint64_t nx, uint64_t ny, uint64_t nz, uint64_t pml_depth, double dt_sub, double spacing)
{
    double *axis_base = 0x0;
    double sigma_max = 0.0;
    double imp = 0.0;
    double pos = 0.0;
    double lo_dist = 0.0;
    double hi_dist = 0.0;
    double grade_frac = 0.0;
    double graded = 0.0;
    double sigma_val = 0.0;
    double kappa_val = 0.0;
    double alpha_val = 0.0;
    double b_val = 0.0;
    double c_val = 0.0;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t axis_offset = 0;
    uint64_t idx = 0;
    uint32_t axis = 0;
    uint32_t half = 0;
    imp = ldg_math_sqrt_get(LDG_MATH_PHYS_VACUUM_PERMEABILITY / LDG_MATH_PHYS_VACUUM_PERMITTIVITY);
    sigma_max = LDG_MATH_PHYS_PML_SIGMA_SCALE * (LDG_MATH_PHYS_PML_GRADE + 1.0) / (imp * spacing);
    dim[0] = nx;
    dim[1] = ny;
    dim[2] = nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        axis_base = stretch + LDG_MATH_PHYS_EM_STRETCH_STRIDE * axis_offset;
        for (idx = 0; idx < dim[axis]; idx++)
        {
            for (half = 0; half < 2; half++)
            {
                pos = (double)idx + ((half == 1) ? 0.5 : 0.0);
                kappa_val = 1.0;
                b_val = 1.0;
                c_val = 0.0;
                if (pml_depth > 0)
                {
                    lo_dist = (double)pml_depth - pos;
                    hi_dist = pos - ((double)dim[axis] - 1.0 - (double)pml_depth);
                    grade_frac = (lo_dist > hi_dist) ? lo_dist : hi_dist;
                    grade_frac /= (double)pml_depth;
                    if (grade_frac > 1.0) { grade_frac = 1.0; }

                    if (grade_frac > 0.0)
                    {
                        graded = ldg_math_pwr_get(grade_frac, LDG_MATH_PHYS_PML_GRADE);
                        sigma_val = sigma_max * graded;
                        kappa_val = 1.0 + (LDG_MATH_PHYS_PML_KAPPA_MAX - 1.0) * graded;
                        alpha_val = LDG_MATH_PHYS_PML_ALPHA_MAX * (1.0 - grade_frac);
                        b_val = ldg_math_exp_get((0.0 - (sigma_val / kappa_val + alpha_val)) * dt_sub / LDG_MATH_PHYS_VACUUM_PERMITTIVITY);
                        c_val = 0.0;
                        if (sigma_val + kappa_val * alpha_val > 0.0) { c_val = sigma_val * (b_val - 1.0) / ((sigma_val + kappa_val * alpha_val) * kappa_val); }
                    }
                }

                axis_base[((half == 1) ? LDG_MATH_PHYS_EM_STRETCH_KAPPA_HALF : LDG_MATH_PHYS_EM_STRETCH_KAPPA_INT) * dim[axis] + idx] = kappa_val;
                axis_base[((half == 1) ? LDG_MATH_PHYS_EM_STRETCH_B_HALF : LDG_MATH_PHYS_EM_STRETCH_B_INT) * dim[axis] + idx] = b_val;
                axis_base[((half == 1) ? LDG_MATH_PHYS_EM_STRETCH_C_HALF : LDG_MATH_PHYS_EM_STRETCH_C_INT) * dim[axis] + idx] = c_val;
            }
        }
        axis_offset += dim[axis];
    }
}

static double* ldg_math_phys_em_aux_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_psi_get(region_ptr) + LDG_MATH_PHYS_EM_PSI_CUNT * hdr->cell_cunt;
}

static ldg_math_phys_em_sheet_t* ldg_math_phys_em_sheet_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return (ldg_math_phys_em_sheet_t *)(ldg_math_phys_em_aux_get(region_ptr) + 2 * hdr->aux_cunt);
}

static ldg_math_phys_em_face_t* ldg_math_phys_em_face_get(const ldg_math_phys_region_t *region_ptr)
{
    return (ldg_math_phys_em_face_t *)(ldg_math_phys_em_sheet_get(region_ptr) + LDG_MATH_PHYS_SHEET_CAP);
}

static ldg_math_phys_slab_job_t* ldg_math_phys_em_job_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return (ldg_math_phys_slab_job_t *)(ldg_math_phys_em_face_get(region_ptr) + hdr->face_cap);
}

static double* ldg_math_phys_em_optic_lambda_get(const ldg_math_phys_region_t *region_ptr)
{
    return (double *)(ldg_math_phys_em_job_get(region_ptr) + LDG_THREAD_POOL_MAX_WORKERS);
}

static double* ldg_math_phys_em_optic_mtl_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_optic_lambda_get(region_ptr) + hdr->optic_bin_cunt;
}

static double* ldg_math_phys_em_optic_scratch_coeff_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_optic_mtl_get(region_ptr) + hdr->optic_bin_cunt * hdr->mtl_cap * LDG_MATH_PHYS_OPTIC_MTL_STRIDE;
}

static double* ldg_math_phys_em_optic_scratch_scatter_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_optic_scratch_coeff_get(region_ptr) + hdr->mtl_cap * LDG_MATH_PHYS_EM_COEFF_STRIDE;
}

static double* ldg_math_phys_em_optic_ordinate_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_optic_scratch_scatter_get(region_ptr) + hdr->mtl_cap;
}

static double* ldg_math_phys_em_optic_src_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_optic_ordinate_get(region_ptr) + hdr->optic_ord_cunt * LDG_MATH_PHYS_ORDINATE_STRIDE;
}

static double* ldg_math_phys_em_optic_flux_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_optic_src_get(region_ptr) + LDG_MATH_PHYS_OPTIC_SRC_CAP * (3 + hdr->optic_bin_cunt);
}

static double* ldg_math_phys_em_optic_ledger_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_optic_flux_get(region_ptr) + hdr->optic_bin_cunt * hdr->cell_cunt;
}

static double* ldg_math_phys_em_optic_work_get(const ldg_math_phys_region_t *region_ptr, uint64_t slot)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_optic_ledger_get(region_ptr) + hdr->optic_bin_cunt * LDG_MATH_PHYS_OPTIC_LEDGER_STRIDE + slot * hdr->cell_cunt;
}

static double* ldg_math_phys_em_optic_radiance_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_optic_ledger_get(region_ptr) + hdr->optic_bin_cunt * LDG_MATH_PHYS_OPTIC_LEDGER_STRIDE + 6 * hdr->cell_cunt;
}

static double* ldg_math_phys_em_optic_emission_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_optic_radiance_get(region_ptr) + hdr->optic_ord_cunt * hdr->cell_cunt;
}

static uint32_t ldg_math_phys_em_optic_size_get(uint64_t cell_cunt, uint64_t mtl_cap, uint64_t bin_cunt, uint64_t ord_cunt, uint64_t *out)
{
    uint64_t double_cunt = 0;
    uint64_t term = 0;
    uint32_t err = 0;
    *out = 0;
    if (bin_cunt == 0) { return LDG_ERR_AOK; }

    double_cunt = bin_cunt + mtl_cap * (bin_cunt * LDG_MATH_PHYS_OPTIC_MTL_STRIDE + LDG_MATH_PHYS_EM_COEFF_STRIDE + 1) + ord_cunt * LDG_MATH_PHYS_ORDINATE_STRIDE + LDG_MATH_PHYS_OPTIC_SRC_CAP * (LDG_MATH_PHYS_DIM + bin_cunt) + bin_cunt * LDG_MATH_PHYS_OPTIC_LEDGER_STRIDE;
    err = ldg_arith_uint64_mul(bin_cunt, cell_cunt, &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(double_cunt, term, &double_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(ord_cunt, cell_cunt, &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(double_cunt, term, &double_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(cell_cunt, 7, &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(double_cunt, term, &double_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(double_cunt, sizeof(double), out);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_em_size_get(uint64_t cell_cunt, uint64_t axis_sum, uint64_t axis_max, uint64_t mtl_cap, uint64_t depth, uint64_t face_cap, uint64_t bin_cunt, uint64_t ord_cunt, uint64_t conductor_cap, uint64_t port_cap, uint64_t tap_depth, uint64_t face_max, uint64_t *out_conductor_offset, uint64_t *out_port_pattern_offset, uint64_t *out_port_tap_offset, uint64_t *out)
{
    uint64_t field_bytes = 0;
    uint64_t snapshot_bytes = 0;
    uint64_t term = 0;
    uint64_t total = 0;
    uint32_t err = 0;
    *out_conductor_offset = 0;
    *out_port_pattern_offset = 0;
    *out_port_tap_offset = 0;
    *out = 0;
    err = ldg_arith_uint64_mul(cell_cunt, (9 + LDG_MATH_PHYS_EM_PSI_CUNT) * sizeof(double), &field_bytes);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(cell_cunt, 6 * sizeof(double), &snapshot_bytes);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(snapshot_bytes, depth, &snapshot_bytes);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    total = sizeof(ldg_math_phys_em_hdr_t) + mtl_cap * LDG_MATH_PHYS_EM_COEFF_STRIDE * sizeof(double) + depth * sizeof(ldg_uint128_t) + axis_sum * LDG_MATH_PHYS_EM_STRETCH_STRIDE * sizeof(double) + 2 * (axis_max + 6) * sizeof(double) + LDG_MATH_PHYS_SHEET_CAP * sizeof(ldg_math_phys_em_sheet_t) + LDG_THREAD_POOL_MAX_WORKERS * sizeof(ldg_math_phys_slab_job_t);
    err = ldg_arith_uint64_add(total, field_bytes, &total);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(face_cap, sizeof(ldg_math_phys_em_face_t), &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(total, term, &total);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(cell_cunt, sizeof(uint16_t), &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(total, LDG_ALIGNED_UP(term), &total);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(total, snapshot_bytes, &total);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_em_optic_size_get(cell_cunt, mtl_cap, bin_cunt, ord_cunt, &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(total, term, &total);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    *out_conductor_offset = total;
    if (conductor_cap > 0)
    {
        err = ldg_arith_uint64_mul(cell_cunt, sizeof(uint16_t), &term);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_arith_uint64_add(total, LDG_ALIGNED_UP(term), &total);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_arith_uint64_mul(cell_cunt, 4 * sizeof(double), &term);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_arith_uint64_add(total, term, &total);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    *out_port_pattern_offset = total;
    *out_port_tap_offset = total;
    if (port_cap > 0)
    {
        err = ldg_arith_uint64_mul(LDG_MATH_PHYS_EM_PORT_CAP * LDG_MATH_PHYS_EM_PORT_MODE_CAP * 4 * sizeof(double), face_max, &term);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_arith_uint64_add(total, term, &total);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        *out_port_tap_offset = total;
        err = ldg_arith_uint64_mul(LDG_MATH_PHYS_EM_PORT_CAP * LDG_MATH_PHYS_EM_PORT_MODE_CAP * 2 * sizeof(double), tap_depth, &term);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_arith_uint64_add(total, term, &total);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    *out = total;
    return LDG_ERR_AOK;
}

static double* ldg_math_phys_em_port_pattern_get(const ldg_math_phys_region_t *region_ptr, uint64_t port, uint64_t mode, uint64_t comp)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return (double *)((uint8_t *)region_ptr->field + hdr->port_pattern_offset) + ((port * LDG_MATH_PHYS_EM_PORT_MODE_CAP + mode) * 4 + comp) * hdr->face_max;
}

static double* ldg_math_phys_em_port_tap_ring_get(const ldg_math_phys_region_t *region_ptr, uint64_t port, uint64_t mode, uint64_t which)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return (double *)((uint8_t *)region_ptr->field + hdr->port_tap_offset) + ((port * LDG_MATH_PHYS_EM_PORT_MODE_CAP + mode) * 2 + which) * hdr->port_tap_depth;
}

static uint16_t* ldg_math_phys_em_conductor_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return (uint16_t *)((uint8_t *)region_ptr->field + hdr->conductor_offset);
}

static double* ldg_math_phys_em_potential_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return (double *)((uint8_t *)region_ptr->field + hdr->conductor_offset + LDG_ALIGNED_UP(hdr->cell_cunt * sizeof(uint16_t)));
}

static double* ldg_math_phys_em_electrostatic_resid_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_potential_get(region_ptr) + hdr->cell_cunt;
}

static double* ldg_math_phys_em_electrostatic_dir_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_electrostatic_resid_get(region_ptr) + hdr->cell_cunt;
}

static double* ldg_math_phys_em_electrostatic_a_dir_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    return ldg_math_phys_em_electrostatic_dir_get(region_ptr) + hdr->cell_cunt;
}

static void ldg_math_phys_em_mtl_coeff_set(double *row, const ldg_math_phys_mtl_desc_t *desc, double dt_sub)
{
    double epsilon_val = desc->epsilon_r * LDG_MATH_PHYS_VACUUM_PERMITTIVITY;
    double mu_val = desc->mu_r * LDG_MATH_PHYS_VACUUM_PERMEABILITY;
    double loss = 0.0;
    row[LDG_MATH_PHYS_EM_COEFF_EPSILON] = epsilon_val;
    row[LDG_MATH_PHYS_EM_COEFF_MU] = mu_val;
    row[LDG_MATH_PHYS_EM_COEFF_SIGMA] = desc->sigma;
    row[LDG_MATH_PHYS_EM_COEFF_DB] = dt_sub / mu_val;
    if (ldg_math_inf_is(desc->sigma))
    {
        row[LDG_MATH_PHYS_EM_COEFF_CA] = 0.0;
        row[LDG_MATH_PHYS_EM_COEFF_CB] = 0.0;
        return;
    }

    loss = desc->sigma * dt_sub / (2.0 * epsilon_val);
    row[LDG_MATH_PHYS_EM_COEFF_CA] = (1.0 - loss) / (1.0 + loss);
    row[LDG_MATH_PHYS_EM_COEFF_CB] = (dt_sub / epsilon_val) / (1.0 + loss);
}

typedef struct ldg_math_phys_acoustic_hdr
{
    double dt_sub;
    double pml_freq;
    double drive_cur;
    double drive_prev;
    double plane_sign;
    uint64_t plane_axis;
    uint64_t tf_lo[LDG_MATH_PHYS_DIM];
    uint64_t tf_hi[LDG_MATH_PHYS_DIM];
    uint64_t aux_cunt;
    uint64_t monopole_cunt;
    uint64_t nx;
    uint64_t ny;
    uint64_t nz;
    uint64_t cell_cunt;
    uint64_t mtl_cap;
    uint64_t mtl_cunt;
    uint64_t subcycle_cunt;
    uint64_t snapshot_depth;
    uint64_t snapshot_hd;
    uint64_t snapshot_cunt;
    uint64_t snapshot_stride;
    uint64_t energy_stride;
    uint64_t tick_cuntr;
    uint64_t pml_depth;
    uint64_t wall_cunt;
    uint64_t face_cunt;
    uint64_t face_cap;
    uint8_t plane_armed;
    uint8_t pudding[7];
} ldg_math_phys_acoustic_hdr_t;

typedef struct ldg_math_phys_acoustic_monopole
{
    double amplitude;
    double freq;
    const double *series;
    uint64_t series_cunt;
    uint64_t body;
    uint64_t tick_start;
} ldg_math_phys_acoustic_monopole_t;

typedef struct ldg_math_phys_acoustic_wall
{
    double z_inf;
    double residue[LDG_MATH_PHYS_WALL_POLE_MAX];
    double pole[LDG_MATH_PHYS_WALL_POLE_MAX];
    double decay[LDG_MATH_PHYS_WALL_POLE_MAX];
    double gain[LDG_MATH_PHYS_WALL_POLE_MAX];
    double denom;
    uint64_t pole_cunt;
    uint8_t used;
    uint8_t pudding[7];
} ldg_math_phys_acoustic_wall_t;

typedef struct ldg_math_phys_acoustic_face
{
    double psi[2][LDG_MATH_PHYS_WALL_POLE_MAX];
    uint64_t cell;
    uint32_t wall;
    uint32_t normal_axis;
} ldg_math_phys_acoustic_face_t;

static void ldg_math_phys_buff_copy(double *dst, const double *src, uint64_t cunt)
{
    uint64_t idx = 0;
    for (idx = 0; idx < cunt; idx++) { dst[idx] = src[idx]; }
}

static void ldg_math_phys_buff_zero(double *buff, uint64_t cunt)
{
    uint64_t idx = 0;
    for (idx = 0; idx < cunt; idx++) { buff[idx] = 0.0; }
}

static uint32_t ldg_math_phys_slab_run(uint32_t worker_id, void *arg)
{
    ldg_math_phys_slab_job_t *job = (ldg_math_phys_slab_job_t *)arg;
    if (LDG_UNLIKELY(!job)) { return LDG_ERR_FUNC_ARG_NULL; }

    job->worker_id = worker_id;
    if (job->pass == LDG_MATH_PHYS_PASS_ACOUSTIC_VELOCITY) { job->ret = ldg_math_phys_field_acoustic_velocity_advance(job->field_a, job->field_b, job->psi, job->mtl, job->coeff, job->stretch, job->nx, job->ny, job->nz, job->spacing, job->slab_lo, job->slab_hi); }
    else if (job->pass == LDG_MATH_PHYS_PASS_ACOUSTIC_PRESSURE) { job->ret = ldg_math_phys_field_acoustic_pressure_advance(job->field_a, job->field_b, job->drive, job->psi, job->mtl, job->coeff, job->stretch, job->nx, job->ny, job->nz, job->spacing, job->slab_lo, job->slab_hi); }
    else if (job->pass == LDG_MATH_PHYS_PASS_EM_ELECTRIC) { job->ret = ldg_math_phys_field_em_electric_advance(job->field_a, job->field_b, job->drive, job->psi, job->mtl, job->coeff, job->stretch, job->nx, job->ny, job->nz, job->spacing, job->slab_lo, job->slab_hi); }
    else { job->ret = ldg_math_phys_field_em_magnetic_advance(job->field_a, job->field_b, job->psi, job->mtl, job->coeff, job->stretch, job->nx, job->ny, job->nz, job->spacing, job->slab_lo, job->slab_hi); }

    LDG_FETCH_ADD(*job->done_cuntr, 1);
    return job->ret;
}

static uint32_t ldg_math_phys_pass_dispatch(const ldg_math_phys_ctx_t *ctx, ldg_math_phys_slab_job_t *job)
{
    ldg_thread_pool_t *pool = (ldg_thread_pool_t *)ctx->conf.pool;
    uint32_t done_cuntr = 0;
    uint64_t job_cunt = 0;
    uint64_t chunk = 0;
    uint64_t nz = job[0].nz;
    uint64_t idx = 0;
    uint32_t err = 0;
    job[0].slab_lo = 0;
    job[0].slab_hi = nz;
    job[0].done_cuntr = &done_cuntr;
    if (pool) { job_cunt = ldg_thread_pool_worker_cunt_get(pool); }

    if (job_cunt > LDG_THREAD_POOL_MAX_WORKERS) { job_cunt = LDG_THREAD_POOL_MAX_WORKERS; }

    if (job_cunt > nz / LDG_MATH_PHYS_SLAB_MIN) { job_cunt = nz / LDG_MATH_PHYS_SLAB_MIN; }

    if (job_cunt < 2)
    {
        err = ldg_math_phys_slab_run(0, &job[0]);
        return err;
    }

    chunk = (nz + job_cunt - 1) / job_cunt;
    for (idx = 0; idx < job_cunt; idx++)
    {
        if (idx * chunk >= nz)
        {
            job_cunt = idx;
            break;
        }

        job[idx] = job[0];
        job[idx].slab_lo = idx * chunk;
        job[idx].slab_hi = ((idx + 1) * chunk > nz) ? nz : (idx + 1) * chunk;
    }
    for (idx = 1; idx < job_cunt; idx++)
    {
        err = ldg_thread_pool_submit(pool, ldg_math_phys_slab_run, &job[idx]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { err = ldg_math_phys_slab_run(0, &job[idx]); }
    }
    err = ldg_math_phys_slab_run(0, &job[0]);
    while (LDG_LD_ACQUIRE(done_cuntr) < job_cunt) { LDG_PAUSE; }
    for (idx = 0; idx < job_cunt; idx++) { if (LDG_UNLIKELY(job[idx].ret != LDG_ERR_AOK)) { return job[idx].ret; } }
    return LDG_ERR_AOK;
}

static ldg_math_phys_acoustic_hdr_t* ldg_math_phys_acoustic_hdr_get(const ldg_math_phys_region_t *region_ptr)
{
    return (ldg_math_phys_acoustic_hdr_t *)region_ptr->field;
}

static double* ldg_math_phys_acoustic_coeff_get(const ldg_math_phys_region_t *region_ptr)
{
    return (double *)((uint8_t *)region_ptr->field + sizeof(ldg_math_phys_acoustic_hdr_t));
}

static ldg_uint128_t* ldg_math_phys_acoustic_stamp_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    return (ldg_uint128_t *)(ldg_math_phys_acoustic_coeff_get(region_ptr) + hdr->mtl_cap * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE);
}

static double* ldg_math_phys_acoustic_pressure_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    return (double *)(ldg_math_phys_acoustic_stamp_get(region_ptr) + hdr->snapshot_depth);
}

static double* ldg_math_phys_acoustic_velocity_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    return ldg_math_phys_acoustic_pressure_get(region_ptr) + hdr->cell_cunt;
}

static double* ldg_math_phys_acoustic_drive_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    return ldg_math_phys_acoustic_velocity_get(region_ptr) + 3 * hdr->cell_cunt;
}

static uint16_t* ldg_math_phys_acoustic_mtl_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    return (uint16_t *)(ldg_math_phys_acoustic_drive_get(region_ptr) + hdr->cell_cunt);
}

static double* ldg_math_phys_acoustic_snapshot_get(const ldg_math_phys_region_t *region_ptr, uint64_t slot)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    uint64_t mtl_bytes = LDG_ALIGNED_UP(hdr->cell_cunt * sizeof(uint16_t));
    return (double *)((uint8_t *)ldg_math_phys_acoustic_mtl_get(region_ptr) + mtl_bytes) + slot * LDG_MATH_PHYS_ACOUSTIC_SNAPSHOT_STRIDE * hdr->cell_cunt;
}

static double* ldg_math_phys_acoustic_stretch_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    return ldg_math_phys_acoustic_snapshot_get(region_ptr, hdr->snapshot_depth);
}

static double* ldg_math_phys_acoustic_psi_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    return ldg_math_phys_acoustic_stretch_get(region_ptr) + LDG_MATH_PHYS_ACOUSTIC_STRETCH_STRIDE * (hdr->nx + hdr->ny + hdr->nz);
}

static ldg_math_phys_acoustic_wall_t* ldg_math_phys_acoustic_wall_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    return (ldg_math_phys_acoustic_wall_t *)(ldg_math_phys_acoustic_psi_get(region_ptr) + LDG_MATH_PHYS_ACOUSTIC_PSI_CUNT * hdr->cell_cunt);
}

static ldg_math_phys_acoustic_face_t* ldg_math_phys_acoustic_face_get(const ldg_math_phys_region_t *region_ptr)
{
    return (ldg_math_phys_acoustic_face_t *)(ldg_math_phys_acoustic_wall_get(region_ptr) + LDG_MATH_PHYS_WALL_CAP);
}

static ldg_math_phys_slab_job_t* ldg_math_phys_acoustic_job_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    return (ldg_math_phys_slab_job_t *)(ldg_math_phys_acoustic_face_get(region_ptr) + hdr->face_cap);
}

static double* ldg_math_phys_acoustic_aux_get(const ldg_math_phys_region_t *region_ptr)
{
    return (double *)(ldg_math_phys_acoustic_job_get(region_ptr) + LDG_THREAD_POOL_MAX_WORKERS);
}

static ldg_math_phys_acoustic_monopole_t* ldg_math_phys_acoustic_monopole_get(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    return (ldg_math_phys_acoustic_monopole_t *)(ldg_math_phys_acoustic_aux_get(region_ptr) + 2 * hdr->aux_cunt);
}

static uint32_t ldg_math_phys_acoustic_size_get(uint64_t cell_cunt, uint64_t axis_sum, uint64_t mtl_cap, uint64_t depth, uint64_t face_cap, uint64_t aux_cunt, uint64_t *out)
{
    uint64_t field_bytes = 0;
    uint64_t snapshot_bytes = 0;
    uint64_t term = 0;
    uint64_t total = 0;
    uint32_t err = 0;
    *out = 0;
    err = ldg_arith_uint64_mul(cell_cunt, (LDG_MATH_PHYS_ACOUSTIC_CELL_GRID_CUNT + LDG_MATH_PHYS_ACOUSTIC_PSI_CUNT) * sizeof(double), &field_bytes);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(cell_cunt, LDG_MATH_PHYS_ACOUSTIC_SNAPSHOT_STRIDE * sizeof(double), &snapshot_bytes);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(snapshot_bytes, depth, &snapshot_bytes);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    total = sizeof(ldg_math_phys_acoustic_hdr_t) + mtl_cap * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE * sizeof(double) + depth * sizeof(ldg_uint128_t) + axis_sum * LDG_MATH_PHYS_ACOUSTIC_STRETCH_STRIDE * sizeof(double) + LDG_MATH_PHYS_WALL_CAP * sizeof(ldg_math_phys_acoustic_wall_t) + LDG_THREAD_POOL_MAX_WORKERS * sizeof(ldg_math_phys_slab_job_t) + 2 * aux_cunt * sizeof(double) + LDG_MATH_PHYS_ACOUSTIC_MONOPOLE_CAP * sizeof(ldg_math_phys_acoustic_monopole_t);
    err = ldg_arith_uint64_add(total, field_bytes, &total);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(face_cap, sizeof(ldg_math_phys_acoustic_face_t), &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(total, term, &total);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(cell_cunt, sizeof(uint16_t), &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(total, LDG_ALIGNED_UP(term), &total);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(total, snapshot_bytes, &total);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    *out = total;
    return LDG_ERR_AOK;
}

static void ldg_math_phys_acoustic_stretch_set(double *stretch, uint64_t nx, uint64_t ny, uint64_t nz, uint64_t pml_depth, double dt_sub, double spacing, double speed, double pml_freq)
{
    double *axis_base = 0x0;
    double damp_max = 0.0;
    double alpha_max = 0.0;
    double pos = 0.0;
    double lo_dist = 0.0;
    double hi_dist = 0.0;
    double grade_frac = 0.0;
    double graded = 0.0;
    double damp_val = 0.0;
    double kappa_val = 0.0;
    double alpha_val = 0.0;
    double b_val = 0.0;
    double c_val = 0.0;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t axis_offset = 0;
    uint64_t idx = 0;
    uint32_t axis = 0;
    uint32_t half = 0;
    if (pml_depth > 0)
    {
        damp_max = (0.0 - (LDG_MATH_PHYS_ACOUSTIC_PML_GRADE + 1.0) * speed * ldg_math_log_get(LDG_MATH_PHYS_ACOUSTIC_PML_RCOEF)) / (2.0 * (double)pml_depth * spacing);
        alpha_max = ldg_math_arccos_get(0.0 - 1.0) * pml_freq;
    }

    dim[0] = nx;
    dim[1] = ny;
    dim[2] = nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        axis_base = stretch + LDG_MATH_PHYS_ACOUSTIC_STRETCH_STRIDE * axis_offset;
        for (idx = 0; idx < dim[axis]; idx++)
        {
            for (half = 0; half < 2; half++)
            {
                pos = (double)idx + ((half == 1) ? 0.5 : 0.0);
                kappa_val = 1.0;
                b_val = 1.0;
                c_val = 0.0;
                if (pml_depth > 0)
                {
                    lo_dist = (double)pml_depth - pos;
                    hi_dist = pos - ((double)dim[axis] - 1.0 - (double)pml_depth);
                    grade_frac = (lo_dist > hi_dist) ? lo_dist : hi_dist;
                    grade_frac /= (double)pml_depth;
                    if (grade_frac > 1.0) { grade_frac = 1.0; }

                    if (grade_frac > 0.0)
                    {
                        graded = ldg_math_pwr_get(grade_frac, LDG_MATH_PHYS_ACOUSTIC_PML_GRADE);
                        damp_val = damp_max * graded;
                        kappa_val = 1.0 + (LDG_MATH_PHYS_ACOUSTIC_PML_KAPPA_MAX - 1.0) * graded;
                        alpha_val = alpha_max * (1.0 - grade_frac);
                        b_val = ldg_math_exp_get((0.0 - (damp_val / kappa_val + alpha_val)) * dt_sub);
                        c_val = 0.0;
                        if (damp_val + kappa_val * alpha_val > 0.0) { c_val = damp_val * (b_val - 1.0) / ((damp_val + kappa_val * alpha_val) * kappa_val); }
                    }
                }

                axis_base[((half == 1) ? LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_HALF : LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_INT) * dim[axis] + idx] = kappa_val;
                axis_base[((half == 1) ? LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_HALF : LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_INT) * dim[axis] + idx] = b_val;
                axis_base[((half == 1) ? LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_HALF : LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_INT) * dim[axis] + idx] = c_val;
            }
        }
        axis_offset += dim[axis];
    }
}

static void ldg_math_phys_acoustic_mtl_coeff_set(double *row, const ldg_math_phys_acoustic_mtl_desc_t *desc, double dt_sub)
{
    double loss_term = desc->loss * dt_sub / 2.0;
    row[LDG_MATH_PHYS_ACOUSTIC_COEFF_DENSITY] = desc->density;
    row[LDG_MATH_PHYS_ACOUSTIC_COEFF_SPEED] = desc->speed;
    row[LDG_MATH_PHYS_ACOUSTIC_COEFF_LOSS] = desc->loss;
    row[LDG_MATH_PHYS_ACOUSTIC_COEFF_CA] = (1.0 - loss_term) / (1.0 + loss_term);
    row[LDG_MATH_PHYS_ACOUSTIC_COEFF_CB] = desc->density * desc->speed * desc->speed * dt_sub / (1.0 + loss_term);
    row[LDG_MATH_PHYS_ACOUSTIC_COEFF_CV] = dt_sub / desc->density;
}

static double ldg_math_phys_em_cycle_sign_get(uint64_t lhs, uint64_t rhs)
{
    return (rhs == (lhs + 1) % LDG_MATH_PHYS_DIM) ? 1.0 : (0.0 - 1.0);
}

static uint64_t ldg_math_phys_em_plane_e_idx_get(const ldg_math_phys_em_hdr_t *hdr, uint64_t pos)
{
    if (hdr->plane_sign > 0.0) { return 2 + pos - hdr->tf_lo[hdr->plane_axis]; }

    return 2 + hdr->tf_hi[hdr->plane_axis] - pos;
}

static uint64_t ldg_math_phys_em_plane_h_idx_get(const ldg_math_phys_em_hdr_t *hdr, uint64_t pos)
{
    if (hdr->plane_sign > 0.0) { return 2 + pos - hdr->tf_lo[hdr->plane_axis]; }

    return 1 + hdr->tf_hi[hdr->plane_axis] - pos;
}

static void ldg_math_phys_em_plane_aux_h_advance(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    double *e_aux = ldg_math_phys_em_aux_get(region_ptr);
    double *h_aux = e_aux + hdr->aux_cunt;
    double coef = hdr->dt_sub / (LDG_MATH_PHYS_VACUUM_PERMEABILITY * region_ptr->desc.spacing);
    uint64_t idx = 0;
    for (idx = 0; idx + 1 < hdr->aux_cunt; idx++) { h_aux[idx] -= coef * (e_aux[idx + 1] - e_aux[idx]); }
}

static void ldg_math_phys_em_plane_aux_e_advance(const ldg_math_phys_region_t *region_ptr, double drive_val)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    double *e_aux = ldg_math_phys_em_aux_get(region_ptr);
    double *h_aux = e_aux + hdr->aux_cunt;
    double coef = hdr->dt_sub / (LDG_MATH_PHYS_VACUUM_PERMITTIVITY * region_ptr->desc.spacing);
    double light_speed = 1.0 / ldg_math_sqrt_get(LDG_MATH_PHYS_VACUUM_PERMITTIVITY * LDG_MATH_PHYS_VACUUM_PERMEABILITY);
    double mur = (light_speed * hdr->dt_sub - region_ptr->desc.spacing) / (light_speed * hdr->dt_sub + region_ptr->desc.spacing);
    double hd_old = 0.0;
    double hd_next_old = 0.0;
    double tail_old = 0.0;
    double tail_prev_old = 0.0;
    uint64_t last = hdr->aux_cunt - 1;
    uint64_t idx = 0;
    hd_old = e_aux[0];
    hd_next_old = e_aux[1];
    tail_old = e_aux[last];
    tail_prev_old = e_aux[last - 1];
    for (idx = 1; idx < last; idx++) { e_aux[idx] -= coef * (h_aux[idx] - h_aux[idx - 1]); }
    e_aux[1] += drive_val;
    e_aux[last] = tail_prev_old + mur * (e_aux[last - 1] - tail_old);
    e_aux[0] = hd_next_old + mur * (e_aux[1] - hd_old);
}

static void ldg_math_phys_em_plane_electric_fix(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    const double *coeff = ldg_math_phys_em_coeff_get(region_ptr);
    const double *row = 0x0;
    const double *h_aux = 0x0;
    const uint16_t *mtl = ldg_math_phys_em_mtl_get(region_ptr);
    double *electric = ldg_math_phys_em_electric_get(region_ptr);
    double hinc = 0.0;
    double face_sign = 0.0;
    double sign_pu = 0.0;
    double sign_up = 0.0;
    uint64_t stride[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t axis_u = 0;
    uint64_t axis_p = 0;
    uint64_t axis_q = 0;
    uint64_t center = 0;
    uint64_t face = 0;
    uint64_t idx_u = 0;
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    h_aux = ldg_math_phys_em_aux_get(region_ptr) + hdr->aux_cunt;
    axis_u = hdr->plane_axis;
    axis_p = hdr->plane_pol_axis;
    axis_q = hdr->plane_hpol_axis;
    sign_pu = ldg_math_phys_em_cycle_sign_get(axis_p, axis_u);
    sign_up = ldg_math_phys_em_cycle_sign_get(axis_u, axis_p);
    stride[0] = 1;
    stride[1] = hdr->nx;
    stride[2] = hdr->nx * hdr->ny;
    for (face = 0; face < 2; face++)
    {
        idx_u = (face == 1) ? hdr->tf_hi[axis_u] : hdr->tf_lo[axis_u];
        face_sign = (face == 1) ? 1.0 : (0.0 - 1.0);
        hinc = hdr->plane_hpol_sign * h_aux[ldg_math_phys_em_plane_h_idx_get(hdr, (face == 1) ? idx_u : idx_u - 1)];
        for (idx_a = hdr->tf_lo[axis_p]; idx_a < hdr->tf_hi[axis_p]; idx_a++) { for (idx_b = hdr->tf_lo[axis_q]; idx_b <= hdr->tf_hi[axis_q]; idx_b++)
            {
                center = idx_u * stride[axis_u] + idx_a * stride[axis_p] + idx_b * stride[axis_q];
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                electric[axis_p * hdr->cell_cunt + center] += face_sign * row[LDG_MATH_PHYS_EM_COEFF_CB] * sign_pu * hinc / region_ptr->desc.spacing;
            }
        }
    }
    for (face = 0; face < 2; face++)
    {
        idx_a = (face == 1) ? hdr->tf_hi[axis_p] : hdr->tf_lo[axis_p];
        face_sign = (face == 1) ? 1.0 : (0.0 - 1.0);
        for (idx_u = hdr->tf_lo[axis_u]; idx_u < hdr->tf_hi[axis_u]; idx_u++)
        {
            hinc = hdr->plane_hpol_sign * h_aux[ldg_math_phys_em_plane_h_idx_get(hdr, idx_u)];
            for (idx_b = hdr->tf_lo[axis_q]; idx_b <= hdr->tf_hi[axis_q]; idx_b++)
            {
                center = idx_u * stride[axis_u] + idx_a * stride[axis_p] + idx_b * stride[axis_q];
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                electric[axis_u * hdr->cell_cunt + center] += face_sign * row[LDG_MATH_PHYS_EM_COEFF_CB] * sign_up * hinc / region_ptr->desc.spacing;
            }
        }
    }
}

static void ldg_math_phys_em_plane_magnetic_fix(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    const double *coeff = ldg_math_phys_em_coeff_get(region_ptr);
    const double *row = 0x0;
    const double *e_aux = 0x0;
    const uint16_t *mtl = ldg_math_phys_em_mtl_get(region_ptr);
    double *magnetic = ldg_math_phys_em_magnetic_get(region_ptr);
    double einc = 0.0;
    double face_sign = 0.0;
    double sign_qu = 0.0;
    double sign_uq = 0.0;
    uint64_t stride[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t axis_u = 0;
    uint64_t axis_p = 0;
    uint64_t axis_q = 0;
    uint64_t center = 0;
    uint64_t face = 0;
    uint64_t idx_u = 0;
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    e_aux = ldg_math_phys_em_aux_get(region_ptr);
    axis_u = hdr->plane_axis;
    axis_p = hdr->plane_pol_axis;
    axis_q = hdr->plane_hpol_axis;
    sign_qu = ldg_math_phys_em_cycle_sign_get(axis_q, axis_u);
    sign_uq = ldg_math_phys_em_cycle_sign_get(axis_u, axis_q);
    stride[0] = 1;
    stride[1] = hdr->nx;
    stride[2] = hdr->nx * hdr->ny;
    for (face = 0; face < 2; face++)
    {
        idx_u = (face == 1) ? hdr->tf_hi[axis_u] : hdr->tf_lo[axis_u] - 1;
        face_sign = (face == 1) ? (0.0 - 1.0) : 1.0;
        einc = hdr->plane_pol_sign * e_aux[ldg_math_phys_em_plane_e_idx_get(hdr, (face == 1) ? idx_u : idx_u + 1)];
        for (idx_a = hdr->tf_lo[axis_p]; idx_a < hdr->tf_hi[axis_p]; idx_a++) { for (idx_b = hdr->tf_lo[axis_q]; idx_b <= hdr->tf_hi[axis_q]; idx_b++)
            {
                center = idx_u * stride[axis_u] + idx_a * stride[axis_p] + idx_b * stride[axis_q];
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                magnetic[axis_q * hdr->cell_cunt + center] += face_sign * row[LDG_MATH_PHYS_EM_COEFF_DB] * sign_qu * einc / region_ptr->desc.spacing;
            }
        }
    }
    for (face = 0; face < 2; face++)
    {
        idx_b = (face == 1) ? hdr->tf_hi[axis_q] : hdr->tf_lo[axis_q] - 1;
        face_sign = (face == 1) ? (0.0 - 1.0) : 1.0;
        for (idx_u = hdr->tf_lo[axis_u]; idx_u <= hdr->tf_hi[axis_u]; idx_u++)
        {
            einc = hdr->plane_pol_sign * e_aux[ldg_math_phys_em_plane_e_idx_get(hdr, idx_u)];
            for (idx_a = hdr->tf_lo[axis_p]; idx_a < hdr->tf_hi[axis_p]; idx_a++)
            {
                center = idx_u * stride[axis_u] + idx_a * stride[axis_p] + idx_b * stride[axis_q];
                row = coeff + (uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
                magnetic[axis_u * hdr->cell_cunt + center] += face_sign * row[LDG_MATH_PHYS_EM_COEFF_DB] * sign_uq * einc / region_ptr->desc.spacing;
            }
        }
    }
}

static void ldg_math_phys_em_sheet_fix_prep(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    ldg_math_phys_em_sheet_t *sheet_ptr = 0x0;
    ldg_math_phys_em_face_t *face = ldg_math_phys_em_face_get(region_ptr);
    const double *electric = ldg_math_phys_em_electric_get(region_ptr);
    const double *magnetic = ldg_math_phys_em_magnetic_get(region_ptr);
    double htan_val = 0.0;
    double flow_sum = 0.0;
    double sign_val = 0.0;
    uint64_t stride[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t face_idx = 0;
    uint64_t pole_idx = 0;
    uint32_t tan_idx = 0;
    uint32_t axis_n = 0;
    uint32_t axis_t = 0;
    uint32_t axis_o = 0;
    stride[0] = 1;
    stride[1] = hdr->nx;
    stride[2] = hdr->nx * hdr->ny;
    for (face_idx = 0; face_idx < hdr->face_cunt; face_idx++)
    {
        sheet_ptr = ldg_math_phys_em_sheet_get(region_ptr) + face[face_idx].sheet;
        axis_n = face[face_idx].normal_axis;
        for (tan_idx = 0; tan_idx < 2; tan_idx++)
        {
            axis_t = (axis_n + 1 + tan_idx) % LDG_MATH_PHYS_DIM;
            axis_o = 3 - axis_n - axis_t;
            sign_val = ldg_math_phys_em_cycle_sign_get(axis_t, axis_o);
            htan_val = sign_val * 0.5 * (magnetic[axis_o * hdr->cell_cunt + face[face_idx].cell] + magnetic[axis_o * hdr->cell_cunt + face[face_idx].cell - stride[axis_n]]);
            flow_sum = sheet_ptr->inst * htan_val;
            for (pole_idx = 0; pole_idx < sheet_ptr->pole_cunt; pole_idx++)
            {
                face[face_idx].psi[tan_idx][pole_idx] = sheet_ptr->decay[pole_idx] * face[face_idx].psi[tan_idx][pole_idx] + sheet_ptr->gain[pole_idx] * htan_val;
                flow_sum += face[face_idx].psi[tan_idx][pole_idx];
            }
            face[face_idx].e_old[tan_idx] = electric[axis_t * hdr->cell_cunt + face[face_idx].cell];
            face[face_idx].flow[tan_idx] = flow_sum;
        }
    }
}

static void ldg_math_phys_em_sheet_fix_commit(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    ldg_math_phys_em_face_t *face = ldg_math_phys_em_face_get(region_ptr);
    const double *coeff = ldg_math_phys_em_coeff_get(region_ptr);
    const double *row = 0x0;
    const uint16_t *mtl = ldg_math_phys_em_mtl_get(region_ptr);
    double *electric = ldg_math_phys_em_electric_get(region_ptr);
    double std_val = 0.0;
    double curl_val = 0.0;
    uint64_t face_idx = 0;
    uint32_t tan_idx = 0;
    uint32_t axis_t = 0;
    for (face_idx = 0; face_idx < hdr->face_cunt; face_idx++)
    {
        row = coeff + (uint64_t)mtl[face[face_idx].cell] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
        for (tan_idx = 0; tan_idx < 2; tan_idx++)
        {
            axis_t = (face[face_idx].normal_axis + 1 + tan_idx) % LDG_MATH_PHYS_DIM;
            std_val = electric[axis_t * hdr->cell_cunt + face[face_idx].cell];
            curl_val = (std_val - row[LDG_MATH_PHYS_EM_COEFF_CA] * face[face_idx].e_old[tan_idx]) / row[LDG_MATH_PHYS_EM_COEFF_CB];
            electric[axis_t * hdr->cell_cunt + face[face_idx].cell] = face[face_idx].ca_mod * face[face_idx].e_old[tan_idx] + face[face_idx].cb_mod * (curl_val - face[face_idx].flow[tan_idx] / region_ptr->desc.spacing);
        }
    }
}

static void ldg_math_phys_acoustic_wall_velocity_fix(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    ldg_math_phys_acoustic_face_t *face = ldg_math_phys_acoustic_face_get(region_ptr);
    double *velocity = ldg_math_phys_acoustic_velocity_get(region_ptr);
    uint64_t face_idx = 0;
    for (face_idx = 0; face_idx < hdr->face_cunt; face_idx++) { velocity[(uint64_t)face[face_idx].normal_axis * hdr->cell_cunt + face[face_idx].cell] = 0.0; }
}

static void ldg_math_phys_acoustic_wall_pressure_fix(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    ldg_math_phys_acoustic_wall_t *wall_ptr = 0x0;
    ldg_math_phys_acoustic_face_t *face = ldg_math_phys_acoustic_face_get(region_ptr);
    const double *coeff = ldg_math_phys_acoustic_coeff_get(region_ptr);
    const double *row = 0x0;
    const uint16_t *mtl = ldg_math_phys_acoustic_mtl_get(region_ptr);
    double *pressure = ldg_math_phys_acoustic_pressure_get(region_ptr);
    double hist_sum = 0.0;
    double flow_val = 0.0;
    uint64_t stride[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t face_idx = 0;
    uint64_t pole_idx = 0;
    uint64_t cell_side = 0;
    uint32_t side = 0;
    stride[0] = 1;
    stride[1] = hdr->nx;
    stride[2] = hdr->nx * hdr->ny;
    for (face_idx = 0; face_idx < hdr->face_cunt; face_idx++)
    {
        wall_ptr = ldg_math_phys_acoustic_wall_get(region_ptr) + face[face_idx].wall;
        for (side = 0; side < 2; side++)
        {
            cell_side = face[face_idx].cell + ((side == 1) ? stride[face[face_idx].normal_axis] : 0);
            row = coeff + (uint64_t)mtl[cell_side] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
            hist_sum = 0.0;
            for (pole_idx = 0; pole_idx < wall_ptr->pole_cunt; pole_idx++) { hist_sum += wall_ptr->residue[pole_idx] * wall_ptr->decay[pole_idx] * face[face_idx].psi[side][pole_idx]; }
            flow_val = (pressure[cell_side] - hist_sum) / wall_ptr->denom;
            pressure[cell_side] -= row[LDG_MATH_PHYS_ACOUSTIC_COEFF_CB] * flow_val / region_ptr->desc.spacing;
            for (pole_idx = 0; pole_idx < wall_ptr->pole_cunt; pole_idx++) { face[face_idx].psi[side][pole_idx] = wall_ptr->decay[pole_idx] * face[face_idx].psi[side][pole_idx] + wall_ptr->gain[pole_idx] * flow_val; }
        }
    }
}

static uint64_t ldg_math_phys_acoustic_plane_p_idx_get(const ldg_math_phys_acoustic_hdr_t *hdr, uint64_t pos)
{
    if (hdr->plane_sign > 0.0) { return 2 + pos - hdr->tf_lo[hdr->plane_axis]; }

    return 2 + hdr->tf_hi[hdr->plane_axis] - pos;
}

static uint64_t ldg_math_phys_acoustic_plane_v_idx_get(const ldg_math_phys_acoustic_hdr_t *hdr, uint64_t pos)
{
    if (hdr->plane_sign > 0.0) { return 2 + pos - hdr->tf_lo[hdr->plane_axis]; }

    return 1 + hdr->tf_hi[hdr->plane_axis] - pos;
}

static void ldg_math_phys_acoustic_plane_aux_v_advance(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    const double *row = ldg_math_phys_acoustic_coeff_get(region_ptr);
    double *p_aux = ldg_math_phys_acoustic_aux_get(region_ptr);
    double *v_aux = p_aux + hdr->aux_cunt;
    double coef = row[LDG_MATH_PHYS_ACOUSTIC_COEFF_CV] / region_ptr->desc.spacing;
    uint64_t idx = 0;
    for (idx = 0; idx + 1 < hdr->aux_cunt; idx++) { v_aux[idx] -= coef * (p_aux[idx + 1] - p_aux[idx]); }
}

static void ldg_math_phys_acoustic_plane_aux_p_advance(const ldg_math_phys_region_t *region_ptr, double drive_val)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    const double *row = ldg_math_phys_acoustic_coeff_get(region_ptr);
    double *p_aux = ldg_math_phys_acoustic_aux_get(region_ptr);
    double *v_aux = p_aux + hdr->aux_cunt;
    double coef = row[LDG_MATH_PHYS_ACOUSTIC_COEFF_CB] / region_ptr->desc.spacing;
    double retain = row[LDG_MATH_PHYS_ACOUSTIC_COEFF_CA];
    double mur = (row[LDG_MATH_PHYS_ACOUSTIC_COEFF_SPEED] * hdr->dt_sub - region_ptr->desc.spacing) / (row[LDG_MATH_PHYS_ACOUSTIC_COEFF_SPEED] * hdr->dt_sub + region_ptr->desc.spacing);
    double hd_old = 0.0;
    double hd_next_old = 0.0;
    double tail_old = 0.0;
    double tail_prev_old = 0.0;
    uint64_t last = hdr->aux_cunt - 1;
    uint64_t idx = 0;
    hd_old = p_aux[0];
    hd_next_old = p_aux[1];
    tail_old = p_aux[last];
    tail_prev_old = p_aux[last - 1];
    for (idx = 1; idx < last; idx++) { p_aux[idx] = retain * p_aux[idx] - coef * (v_aux[idx] - v_aux[idx - 1]); }
    p_aux[1] += drive_val;
    p_aux[last] = tail_prev_old + mur * (p_aux[last - 1] - tail_old);
    p_aux[0] = hd_next_old + mur * (p_aux[1] - hd_old);
}

static void ldg_math_phys_acoustic_plane_velocity_fix(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    const double *coeff = ldg_math_phys_acoustic_coeff_get(region_ptr);
    const double *p_aux = ldg_math_phys_acoustic_aux_get(region_ptr);
    const uint16_t *mtl = ldg_math_phys_acoustic_mtl_get(region_ptr);
    double *velocity = ldg_math_phys_acoustic_velocity_get(region_ptr);
    double p_inc = 0.0;
    double cv_face = 0.0;
    double face_sign = 0.0;
    uint64_t stride[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t axis_u = hdr->plane_axis;
    uint64_t axis_n = 0;
    uint64_t axis_a = 0;
    uint64_t axis_b = 0;
    uint64_t node = 0;
    uint64_t center = 0;
    uint64_t face = 0;
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    uint64_t u_pos = 0;
    stride[0] = 1;
    stride[1] = hdr->nx;
    stride[2] = hdr->nx * hdr->ny;
    for (axis_n = 0; axis_n < LDG_MATH_PHYS_DIM; axis_n++)
    {
        axis_a = (axis_n + 1) % LDG_MATH_PHYS_DIM;
        axis_b = (axis_n + 2) % LDG_MATH_PHYS_DIM;
        for (face = 0; face < 2; face++)
        {
            node = (face == 1) ? hdr->tf_hi[axis_n] : hdr->tf_lo[axis_n] - 1;
            face_sign = (face == 1) ? (0.0 - 1.0) : 1.0;
            for (idx_a = hdr->tf_lo[axis_a]; idx_a <= hdr->tf_hi[axis_a]; idx_a++)
            {
                for (idx_b = hdr->tf_lo[axis_b]; idx_b <= hdr->tf_hi[axis_b]; idx_b++)
                {
                    center = node * stride[axis_n] + idx_a * stride[axis_a] + idx_b * stride[axis_b];
                    if (axis_n == axis_u) { u_pos = (face == 1) ? hdr->tf_hi[axis_u] : hdr->tf_lo[axis_u]; }
                    else if (axis_a == axis_u) { u_pos = idx_a; }
                    else { u_pos = idx_b; }

                    p_inc = p_aux[ldg_math_phys_acoustic_plane_p_idx_get(hdr, u_pos)];
                    cv_face = 0.5 * (coeff[(uint64_t)mtl[center] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE + LDG_MATH_PHYS_ACOUSTIC_COEFF_CV] + coeff[(uint64_t)mtl[center + stride[axis_n]] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE + LDG_MATH_PHYS_ACOUSTIC_COEFF_CV]);
                    velocity[axis_n * hdr->cell_cunt + center] += face_sign * cv_face * p_inc / region_ptr->desc.spacing;
                }
            }
        }
    }
}

static void ldg_math_phys_acoustic_plane_pressure_fix(const ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    const double *coeff = ldg_math_phys_acoustic_coeff_get(region_ptr);
    const double *v_aux = ldg_math_phys_acoustic_aux_get(region_ptr) + hdr->aux_cunt;
    const uint16_t *mtl = ldg_math_phys_acoustic_mtl_get(region_ptr);
    double *pressure = ldg_math_phys_acoustic_pressure_get(region_ptr);
    double v_inc = 0.0;
    double face_sign = 0.0;
    uint64_t stride[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t axis_u = hdr->plane_axis;
    uint64_t axis_a = (hdr->plane_axis + 1) % LDG_MATH_PHYS_DIM;
    uint64_t axis_b = (hdr->plane_axis + 2) % LDG_MATH_PHYS_DIM;
    uint64_t cell_u = 0;
    uint64_t center = 0;
    uint64_t face = 0;
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    stride[0] = 1;
    stride[1] = hdr->nx;
    stride[2] = hdr->nx * hdr->ny;
    for (face = 0; face < 2; face++)
    {
        cell_u = (face == 1) ? hdr->tf_hi[axis_u] : hdr->tf_lo[axis_u];
        face_sign = (face == 1) ? (0.0 - 1.0) : 1.0;
        v_inc = hdr->plane_sign * v_aux[ldg_math_phys_acoustic_plane_v_idx_get(hdr, (face == 1) ? cell_u : cell_u - 1)];
        for (idx_a = hdr->tf_lo[axis_a]; idx_a <= hdr->tf_hi[axis_a]; idx_a++) { for (idx_b = hdr->tf_lo[axis_b]; idx_b <= hdr->tf_hi[axis_b]; idx_b++)
            {
                center = cell_u * stride[axis_u] + idx_a * stride[axis_a] + idx_b * stride[axis_b];
                pressure[center] += face_sign * coeff[(uint64_t)mtl[center] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE + LDG_MATH_PHYS_ACOUSTIC_COEFF_CB] * v_inc / region_ptr->desc.spacing;
            }
        }
    }
}

static uint32_t ldg_math_phys_acoustic_monopole_deposit(ldg_math_phys_ctx_t *ctx, ldg_math_phys_region_t *region_ptr, uint32_t region_idx)
{
    ldg_math_phys_acoustic_hdr_t *hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    ldg_math_phys_acoustic_monopole_t *monopole = ldg_math_phys_acoustic_monopole_get(region_ptr);
    ldg_math_phys_body_motion_t motion = LDG_STRUCT_ZERO_INIT;
    double *drive = ldg_math_phys_acoustic_drive_get(region_ptr);
    double dt = 0.0;
    double t_rel = 0.0;
    double cycle = 0.0;
    double ramp = 0.0;
    double sample = 0.0;
    double grid_pos[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double delta = 0.0;
    double dist_sq = 0.0;
    double weight = 0.0;
    double weight_sum = 0.0;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t lo[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t hi[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t slot = 0;
    uint64_t sample_idx = 0;
    uint32_t err = 0;
    uint32_t axis = 0;
    uint32_t pass = 0;
    err = ldg_math_phys_dt_get(ctx, &dt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (slot = 0; slot < hdr->monopole_cunt; slot++)
    {
        err = ldg_math_phys_body_motion_get(ctx, monopole[slot].body, &motion);
        if (err != LDG_ERR_AOK) { continue; }

        if (motion.region != region_idx) { continue; }

        if (hdr->tick_cuntr < monopole[slot].tick_start) { continue; }

        sample_idx = hdr->tick_cuntr - monopole[slot].tick_start;
        if (monopole[slot].series != 0x0)
        {
            if (sample_idx >= monopole[slot].series_cunt) { continue; }

            sample = monopole[slot].amplitude * monopole[slot].series[sample_idx];
        }
        else
        {
            t_rel = (double)sample_idx * dt;
            cycle = monopole[slot].freq * t_rel;
            ramp = 1.0;
            if (cycle < LDG_MATH_PHYS_ACOUSTIC_RAMP_PERIOD_CUNT) { ramp = 0.5 * (1.0 - ldg_math_cos_get(LDG_MATH_PI * cycle / LDG_MATH_PHYS_ACOUSTIC_RAMP_PERIOD_CUNT)); }

            sample = monopole[slot].amplitude * ramp * ldg_math_sin_get(LDG_MATH_TAU * cycle);
        }

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            grid_pos[axis] = motion.pos[axis] / region_ptr->desc.spacing;
            delta = grid_pos[axis] - LDG_MATH_PHYS_ACOUSTIC_MONOPOLE_RADIUS_CELL;
            lo[axis] = (delta > 0.0) ? (uint64_t)delta : 0;
            delta = grid_pos[axis] + LDG_MATH_PHYS_ACOUSTIC_MONOPOLE_RADIUS_CELL;
            hi[axis] = (delta < 0.0) ? 0 : (uint64_t)delta + 1;
            if (hi[axis] > dim[axis] - 1) { hi[axis] = dim[axis] - 1; }
        }
        weight_sum = 0.0;
        for (pass = 0; pass < 2; pass++)
        {
            for (idx_z = lo[2]; idx_z <= hi[2]; idx_z++)
            {
                for (idx_y = lo[1]; idx_y <= hi[1]; idx_y++)
                {
                    for (idx_x = lo[0]; idx_x <= hi[0]; idx_x++)
                    {
                        dist_sq = ((double)idx_x - grid_pos[0]) * ((double)idx_x - grid_pos[0]) + ((double)idx_y - grid_pos[1]) * ((double)idx_y - grid_pos[1]) + ((double)idx_z - grid_pos[2]) * ((double)idx_z - grid_pos[2]);
                        if (dist_sq > LDG_MATH_PHYS_ACOUSTIC_MONOPOLE_RADIUS_CELL * LDG_MATH_PHYS_ACOUSTIC_MONOPOLE_RADIUS_CELL) { continue; }

                        weight = ldg_math_exp_get((0.0 - dist_sq) / (LDG_MATH_PHYS_ACOUSTIC_MONOPOLE_ALPHA * LDG_MATH_PHYS_ACOUSTIC_MONOPOLE_ALPHA));
                        if (pass == 0)
                        {
                            weight_sum += weight;
                            continue;
                        }

                        drive[(idx_z * hdr->ny + idx_y) * hdr->nx + idx_x] += sample * weight / weight_sum;
                    }
                }
            }
            if (weight_sum <= 0.0) { break; }
        }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_step_acoustic_advance(ldg_math_phys_ctx_t *ctx, ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_acoustic_hdr_t *hdr = 0x0;
    ldg_math_phys_conserved_t quantized = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_slab_job_t *job = 0x0;
    ldg_uint128_t stamp = LDG_STRUCT_ZERO_INIT;
    double *pressure = 0x0;
    double *velocity = 0x0;
    double *drive = 0x0;
    double *coeff = 0x0;
    double *snapshot = 0x0;
    double *stretch = 0x0;
    double *psi = 0x0;
    uint16_t *mtl = 0x0;
    double energy_val = 0.0;
    double drive_val = 0.0;
    uint64_t sub_idx = 0;
    uint64_t slot = 0;
    uint32_t err = 0;
    hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    if (LDG_UNLIKELY(hdr->mtl_cunt == 0)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (hdr->monopole_cunt)
    {
        err = ldg_math_phys_acoustic_monopole_deposit(ctx, region_ptr, (uint32_t)(region_ptr - ctx->region));
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    pressure = ldg_math_phys_acoustic_pressure_get(region_ptr);
    velocity = ldg_math_phys_acoustic_velocity_get(region_ptr);
    drive = ldg_math_phys_acoustic_drive_get(region_ptr);
    coeff = ldg_math_phys_acoustic_coeff_get(region_ptr);
    mtl = ldg_math_phys_acoustic_mtl_get(region_ptr);
    stretch = ldg_math_phys_acoustic_stretch_get(region_ptr);
    psi = ldg_math_phys_acoustic_psi_get(region_ptr);
    job = ldg_math_phys_acoustic_job_get(region_ptr);
    job[0].psi = psi;
    job[0].mtl = mtl;
    job[0].coeff = coeff;
    job[0].stretch = stretch;
    job[0].nx = hdr->nx;
    job[0].ny = hdr->ny;
    job[0].nz = hdr->nz;
    job[0].spacing = region_ptr->desc.spacing;
    for (sub_idx = 0; sub_idx < hdr->subcycle_cunt; sub_idx++)
    {
        if (hdr->nz == 1)
        {
            err = ldg_math_phys_field_acoustic_velocity_2d_advance(velocity, pressure, psi, mtl, coeff, stretch, hdr->nx, hdr->ny, region_ptr->desc.spacing);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

            err = ldg_math_phys_field_acoustic_pressure_2d_advance(pressure, velocity, drive, psi, mtl, coeff, stretch, hdr->nx, hdr->ny, region_ptr->desc.spacing);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

            continue;
        }

        job[0].pass = LDG_MATH_PHYS_PASS_ACOUSTIC_VELOCITY;
        job[0].field_a = velocity;
        job[0].field_b = pressure;
        job[0].drive = 0x0;
        err = ldg_math_phys_pass_dispatch(ctx, job);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

        if (hdr->face_cunt) { ldg_math_phys_acoustic_wall_velocity_fix(region_ptr); }

        if (hdr->plane_armed)
        {
            ldg_math_phys_acoustic_plane_velocity_fix(region_ptr);
            ldg_math_phys_acoustic_plane_aux_v_advance(region_ptr);
        }

        job[0].pass = LDG_MATH_PHYS_PASS_ACOUSTIC_PRESSURE;
        job[0].field_a = pressure;
        job[0].field_b = velocity;
        job[0].drive = drive;
        err = ldg_math_phys_pass_dispatch(ctx, job);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

        if (hdr->face_cunt) { ldg_math_phys_acoustic_wall_pressure_fix(region_ptr); }

        if (hdr->plane_armed)
        {
            ldg_math_phys_acoustic_plane_pressure_fix(region_ptr);
            drive_val = hdr->drive_prev + (hdr->drive_cur - hdr->drive_prev) * ((double)(sub_idx + 1) / (double)hdr->subcycle_cunt);
            ldg_math_phys_acoustic_plane_aux_p_advance(region_ptr, drive_val);
        }
    }
    hdr->drive_prev = hdr->drive_cur;
    hdr->tick_cuntr++;
    if (hdr->tick_cuntr % hdr->energy_stride == 0)
    {
        err = ldg_math_phys_field_acoustic_energy_get(pressure, velocity, mtl, coeff, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, &energy_val);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

        err = ldg_math_phys_double_to_conserved_convert(energy_val, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY), &quantized);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_region_total_cum(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY, &quantized);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    ldg_math_phys_buff_zero(drive, hdr->cell_cunt);
    if (hdr->tick_cuntr % hdr->snapshot_stride != 0) { return LDG_ERR_AOK; }

    slot = hdr->snapshot_hd;
    snapshot = ldg_math_phys_acoustic_snapshot_get(region_ptr, slot);
    ldg_math_phys_buff_copy(snapshot, pressure, hdr->cell_cunt);
    ldg_math_phys_buff_copy(snapshot + hdr->cell_cunt, velocity, 3 * hdr->cell_cunt);
    err = ldg_arith_uint128_add(&ctx->clock, &ctx->conf.tick_budget, &stamp);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ldg_math_phys_acoustic_stamp_get(region_ptr)[slot] = stamp;
    if (hdr->snapshot_cunt < hdr->snapshot_depth) { hdr->snapshot_cunt++; }

    LDG_ST_RELEASE(hdr->snapshot_hd, (slot + 1) % hdr->snapshot_depth);
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_step_em_advance(ldg_math_phys_ctx_t *ctx, ldg_math_phys_region_t *region_ptr)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_math_phys_conserved_t quantized = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_slab_job_t *job = 0x0;
    ldg_uint128_t stamp = LDG_STRUCT_ZERO_INIT;
    double *electric = 0x0;
    double *magnetic = 0x0;
    double *drive = 0x0;
    double *coeff = 0x0;
    double *snapshot = 0x0;
    double *stretch = 0x0;
    double *psi = 0x0;
    uint16_t *mtl = 0x0;
    double drive_val = 0.0;
    double energy_val = 0.0;
    uint64_t sub_idx = 0;
    uint64_t slot = 0;
    uint32_t err = 0;
    hdr = ldg_math_phys_em_hdr_get(region_ptr);
    electric = ldg_math_phys_em_electric_get(region_ptr);
    magnetic = ldg_math_phys_em_magnetic_get(region_ptr);
    drive = ldg_math_phys_em_drive_get(region_ptr);
    coeff = ldg_math_phys_em_coeff_get(region_ptr);
    mtl = ldg_math_phys_em_mtl_get(region_ptr);
    stretch = ldg_math_phys_em_stretch_get(region_ptr);
    psi = ldg_math_phys_em_psi_get(region_ptr);
    job = ldg_math_phys_em_job_get(region_ptr);
    job[0].psi = psi;
    job[0].mtl = mtl;
    job[0].coeff = coeff;
    job[0].stretch = stretch;
    job[0].nx = hdr->nx;
    job[0].ny = hdr->ny;
    job[0].nz = hdr->nz;
    job[0].spacing = region_ptr->desc.spacing;
    for (slot = 0; slot < hdr->drive_cunt; slot++)
    {
        if (hdr->tick_cuntr < hdr->drive_slot[slot].tick_start) { continue; }

        err = ldg_math_phys_sig_sample_get(&hdr->drive_slot[slot].sig, hdr->tick_cuntr - hdr->drive_slot[slot].tick_start, hdr->dt_sub * (double)hdr->subcycle_cunt, &drive_val);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        drive[(uint64_t)hdr->drive_slot[slot].comp * hdr->cell_cunt + hdr->drive_slot[slot].cell] += drive_val;
    }
    for (sub_idx = 0; sub_idx < hdr->subcycle_cunt; sub_idx++)
    {
        if (hdr->nz == 1)
        {
            err = ldg_math_phys_field_em_electric_2d_advance(electric, magnetic, drive, psi, mtl, coeff, stretch, hdr->nx, hdr->ny, region_ptr->desc.spacing);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

            err = ldg_math_phys_field_em_magnetic_2d_advance(magnetic, electric, psi, mtl, coeff, stretch, hdr->nx, hdr->ny, region_ptr->desc.spacing);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

            continue;
        }

        if (hdr->face_cunt) { ldg_math_phys_em_sheet_fix_prep(region_ptr); }

        job[0].pass = LDG_MATH_PHYS_PASS_EM_ELECTRIC;
        job[0].field_a = electric;
        job[0].field_b = magnetic;
        job[0].drive = drive;
        err = ldg_math_phys_pass_dispatch(ctx, job);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

        if (hdr->face_cunt) { ldg_math_phys_em_sheet_fix_commit(region_ptr); }

        if (hdr->plane_armed)
        {
            ldg_math_phys_em_plane_electric_fix(region_ptr);
            drive_val = hdr->drive_prev + (hdr->drive_cur - hdr->drive_prev) * ((double)(sub_idx + 1) / (double)hdr->subcycle_cunt);
            ldg_math_phys_em_plane_aux_e_advance(region_ptr, drive_val);
        }

        job[0].pass = LDG_MATH_PHYS_PASS_EM_MAGNETIC;
        job[0].field_a = magnetic;
        job[0].field_b = electric;
        job[0].drive = 0x0;
        err = ldg_math_phys_pass_dispatch(ctx, job);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

        if (hdr->plane_armed)
        {
            ldg_math_phys_em_plane_magnetic_fix(region_ptr);
            ldg_math_phys_em_plane_aux_h_advance(region_ptr);
        }
    }
    hdr->drive_prev = hdr->drive_cur;
    hdr->tick_cuntr++;
    if (hdr->tick_cuntr % hdr->energy_stride == 0)
    {
        err = ldg_math_phys_field_em_energy_get(electric, magnetic, mtl, coeff, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, &energy_val);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

        err = ldg_math_phys_double_to_conserved_convert(energy_val, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY), &quantized);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_region_total_cum(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY, &quantized);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    ldg_math_phys_buff_zero(drive, 3 * hdr->cell_cunt);
    if (hdr->tick_cuntr % hdr->snapshot_stride != 0) { return LDG_ERR_AOK; }

    slot = hdr->snapshot_hd;
    snapshot = ldg_math_phys_em_snapshot_get(region_ptr, slot);
    ldg_math_phys_buff_copy(snapshot, electric, 3 * hdr->cell_cunt);
    ldg_math_phys_buff_copy(snapshot + 3 * hdr->cell_cunt, magnetic, 3 * hdr->cell_cunt);
    err = ldg_arith_uint128_add(&ctx->clock, &ctx->conf.tick_budget, &stamp);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ldg_math_phys_em_stamp_get(region_ptr)[slot] = stamp;
    if (hdr->snapshot_cunt < hdr->snapshot_depth) { hdr->snapshot_cunt++; }

    LDG_ST_RELEASE(hdr->snapshot_hd, (slot + 1) % hdr->snapshot_depth);
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_dt_tick_get(const ldg_math_phys_ctx_t *ctx, double *out)
{
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    return ldg_math_phys_dt_get(ctx, out);
}

uint32_t ldg_math_phys_region_probe_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, double *out)
{
    ldg_math_phys_slice_desc_t desc = LDG_STRUCT_ZERO_INIT;
    double electric[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint32_t kind = 0;
    uint32_t err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx || !pos || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    kind = ctx->region[region].desc.field_kind;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { desc.orig[axis] = pos[axis]; }

    desc.axis_a = 0;
    desc.axis_b = 1;
    desc.step_a = ctx->region[region].desc.spacing;
    desc.step_b = ctx->region[region].desc.spacing;
    desc.cunt_a = 1;
    desc.cunt_b = 1;
    if (kind == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC) { return ldg_math_phys_field_acoustic_slice_get(ctx, region, &desc, 0x0, out, 0x0); }

    if (kind == LDG_MATH_PHYS_FIELD_KIND_EM)
    {
        err = ldg_math_phys_field_em_slice_get(ctx, region, &desc, 0x0, electric, 0x0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        *out = electric[2];
        return LDG_ERR_AOK;
    }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_math_phys_region_tick_advance(ldg_math_phys_ctx_t *ctx, uint32_t region, double dt, uint32_t frame_idx)
{
    ldg_math_phys_field_hdr_t *hdr = 0x0;
    ldg_math_phys_region_t *region_ptr = 0x0;
    ldg_math_phys_conserved_t quantized = LDG_STRUCT_ZERO_INIT;
    double *cur_buff = 0x0;
    double *prev_buff = 0x0;
    double *next_buff = 0x0;
    double *snapshot = 0x0;
    double energy_val = 0.0;
    uint64_t next_idx = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    region_ptr = &ctx->region[region];
    if (LDG_UNLIKELY(region_ptr->desc.tier != LDG_MATH_PHYS_TIER_FIELD || region_ptr->field == 0x0)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (region_ptr->desc.field_kind == LDG_MATH_PHYS_FIELD_KIND_EM) { return ldg_math_phys_step_em_advance(ctx, region_ptr); }

    if (region_ptr->desc.field_kind == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC) { return ldg_math_phys_step_acoustic_advance(ctx, region_ptr); }

    hdr = ldg_math_phys_field_hdr_get(region_ptr);
    next_idx = 3 - hdr->cur_idx - hdr->prev_idx;
    cur_buff = ldg_math_phys_field_buff_get(region_ptr, hdr->cur_idx);
    prev_buff = ldg_math_phys_field_buff_get(region_ptr, hdr->prev_idx);
    next_buff = ldg_math_phys_field_buff_get(region_ptr, next_idx);
    if (hdr->nz == 1) { err = ldg_math_phys_field_2d_advance(cur_buff, prev_buff, next_buff, hdr->nx, hdr->ny, region_ptr->desc.spacing, region_ptr->desc.wave_speed, dt, region_ptr->desc.damping, region_ptr->desc.boundary); }
    else { err = ldg_math_phys_field_3d_advance(cur_buff, prev_buff, next_buff, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, region_ptr->desc.wave_speed, dt, region_ptr->desc.damping, region_ptr->desc.boundary); }

    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    hdr->prev_idx = hdr->cur_idx;
    hdr->cur_idx = next_idx;
    err = ldg_math_phys_field_energy_get(next_buff, cur_buff, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, region_ptr->desc.wave_speed, dt, &energy_val);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_double_to_conserved_convert(energy_val, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY), &quantized);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_region_total_cum(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY, &quantized);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    snapshot = ldg_math_phys_field_snapshot_get(region_ptr, frame_idx);
    return ldg_mem_secure_cpy(snapshot, next_buff, hdr->cell_cunt * sizeof(double));
}

static uint32_t ldg_math_phys_step_field_advance(ldg_math_phys_ctx_t *ctx, double dt, uint32_t frame_idx)
{
    ldg_math_phys_region_t *region_ptr = 0x0;
    uint32_t err = 0;
    uint32_t idx = 0;
    for (idx = 0; idx < ctx->conf.region_cap; idx++)
    {
        region_ptr = &ctx->region[idx];
        if (!region_ptr->used) { continue; }

        if (region_ptr->desc.tier != LDG_MATH_PHYS_TIER_FIELD) { continue; }

        if (region_ptr->field == 0x0) { continue; }

        if (region_ptr->desc.step_skip) { continue; }

        err = ldg_math_phys_region_tick_advance(ctx, idx, dt, frame_idx);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_field_grad_get(const ldg_math_phys_region_t *region_ptr, const double *pos, double *out_grad)
{
    ldg_math_phys_field_hdr_t *hdr = 0x0;
    double *cur_buff = 0x0;
    double probe[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double bound = 0.0;
    double lo_pos = 0.0;
    double hi_pos = 0.0;
    double lo_val = 0.0;
    double hi_val = 0.0;
    double half = 0.0;
    uint32_t err = 0;
    uint32_t axis = 0;
    uint32_t axis_cunt = 0;
    hdr = ldg_math_phys_field_hdr_get(region_ptr);
    cur_buff = ldg_math_phys_field_buff_get(region_ptr, hdr->cur_idx);
    half = region_ptr->desc.spacing * 0.5;
    axis_cunt = (hdr->nz == 1) ? 2 : 3;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        out_grad[axis] = 0.0;
        probe[axis] = pos[axis];
    }
    for (axis = 0; axis < axis_cunt; axis++)
    {
        bound = ldg_math_next_toward_get(region_ptr->desc.extent[axis], 0.0);
        lo_pos = pos[axis] - half;
        if (lo_pos < 0.0) { lo_pos = 0.0; }

        hi_pos = pos[axis] + half;
        if (hi_pos > bound) { hi_pos = bound; }

        if (!(hi_pos > lo_pos)) { continue; }

        probe[axis] = lo_pos;
        if (hdr->nz == 1) { err = ldg_math_phys_field_2d_sample_get(cur_buff, hdr->nx, hdr->ny, region_ptr->desc.spacing, probe[0], probe[1], &lo_val); }
        else { err = ldg_math_phys_field_3d_sample_get(cur_buff, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, probe[0], probe[1], probe[2], &lo_val); }

        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

        probe[axis] = hi_pos;
        if (hdr->nz == 1) { err = ldg_math_phys_field_2d_sample_get(cur_buff, hdr->nx, hdr->ny, region_ptr->desc.spacing, probe[0], probe[1], &hi_val); }
        else { err = ldg_math_phys_field_3d_sample_get(cur_buff, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, probe[0], probe[1], probe[2], &hi_val); }

        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

        out_grad[axis] = (hi_val - lo_val) / (hi_pos - lo_pos);
        probe[axis] = pos[axis];
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_step_couple_apply(ldg_math_phys_ctx_t *ctx, double dt)
{
    ldg_math_phys_body_motion_t motion = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_field_hdr_t *hdr = 0x0;
    ldg_math_phys_region_t *region_ptr = 0x0;
    ldg_math_phys_conserved_t delta[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    ldg_math_phys_conserved_t neg_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t sum_val = LDG_STRUCT_ZERO_INIT;
    double *cur_buff = 0x0;
    double grad[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double impulse[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t entity = 0;
    uint32_t err = 0;
    uint32_t dense_idx = 0;
    uint32_t axis = 0;
    for (dense_idx = 0; dense_idx < ctx->world->store.alive_cunt; dense_idx++)
    {
        entity = ctx->world->store.dense[dense_idx];
        if (ldg_ecs_ctx_comp_has(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION) != LDG_TRUTH_TRUE) { continue; }

        err = ldg_math_phys_body_motion_get(ctx, entity, &motion);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (!motion.massive) { continue; }

        region_ptr = &ctx->region[motion.region];
        if (region_ptr->desc.tier != LDG_MATH_PHYS_TIER_FIELD) { continue; }

        if (region_ptr->desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_SCALAR) { continue; }

        if (region_ptr->field == 0x0) { continue; }

        hdr = ldg_math_phys_field_hdr_get(region_ptr);
        if (!(ldg_math_abs_get(hdr->emit) + ldg_math_abs_get(hdr->absorb) > 0.0)) { continue; }

        if (ldg_math_abs_get(hdr->absorb) > 0.0)
        {
            err = ldg_math_phys_field_grad_get(region_ptr, motion.pos, grad);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { impulse[axis] = (0.0 - hdr->absorb) * grad[axis] * dt; }
            err = ldg_math_phys_momentum_quantize(region_ptr, impulse, delta);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_math_phys_momentum_delta_apply(ctx, entity, delta, 0);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
            {
                err = ldg_math_phys_conserved_neg(&delta[axis], &neg_val);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                err = ldg_math_phys_conserved_add(&hdr->momentum_account[axis], &neg_val, &sum_val);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                hdr->momentum_account[axis] = sum_val;
            }
        }

        if (ldg_math_abs_get(hdr->emit) > 0.0)
        {
            cur_buff = ldg_math_phys_field_buff_get(region_ptr, hdr->cur_idx);
            if (hdr->nz == 1) { err = ldg_math_phys_field_2d_src_add(cur_buff, hdr->nx, hdr->ny, region_ptr->desc.spacing, motion.pos[0], motion.pos[1], hdr->emit * dt * dt); }
            else { err = ldg_math_phys_field_3d_src_add(cur_buff, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, motion.pos[0], motion.pos[1], motion.pos[2], hdr->emit * dt * dt); }

            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }
        }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_body_wr(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t kind, const ldg_math_phys_body_val_t *val, const ldg_math_phys_tag_t *tag, uint64_t *out)
{
    ldg_math_phys_conserved_t *conserved_ptr = 0x0;
    ldg_uint128_t *pos_ptr = 0x0;
    void *comp_ptr = 0x0;
    uint64_t entity = 0;
    uint32_t err = 0;
    uint32_t rollback_err = 0;
    uint32_t comp_idx = 0;
    uint32_t axis = 0;
    err = ldg_ecs_ctx_inst_reg(ctx->world, ctx->conf.taxon_id_base + ctx->region[region].desc.tier, 0x0, &entity);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (comp_idx = 0; comp_idx < LDG_MATH_PHYS_COMP_TAG; comp_idx++)
    {
        err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + comp_idx, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { break; }

        switch (comp_idx)
        {
            case LDG_MATH_PHYS_COMP_MASS:
                *(ldg_math_phys_conserved_t *)comp_ptr = val->mass_c;
                break;
            case LDG_MATH_PHYS_COMP_ENERGY:
                *(ldg_math_phys_conserved_t *)comp_ptr = val->energy_c;
                break;
            case LDG_MATH_PHYS_COMP_MOMENTUM:
                conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
                for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { conserved_ptr[axis] = val->momentum_c[axis]; }
                break;
            case LDG_MATH_PHYS_COMP_ANGULAR_MOMENTUM:
                conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
                for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { conserved_ptr[axis] = val->angular_c[axis]; }
                break;
            case LDG_MATH_PHYS_COMP_CHARGE:
                *(ldg_math_phys_conserved_t *)comp_ptr = val->charge_c;
                break;
            case LDG_MATH_PHYS_COMP_POS:
                pos_ptr = (ldg_uint128_t *)comp_ptr;
                for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { pos_ptr[axis] = val->pos_fixed[axis]; }
                break;
            case LDG_MATH_PHYS_COMP_ORIENTATION:
                for (axis = 0; axis < LDG_MATH_QUATERNION_DIM; axis++) { ((double *)comp_ptr)[axis] = val->orientation[axis]; }
                break;
            case LDG_MATH_PHYS_COMP_SPIN:
                for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { ((double *)comp_ptr)[axis] = val->spin[axis]; }
                break;
            case LDG_MATH_PHYS_COMP_INERTIA:
                *(double *)comp_ptr = val->inertia;
                break;
            case LDG_MATH_PHYS_COMP_KIND:
                *(uint32_t *)comp_ptr = kind;
                break;
            default:
                *(uint32_t *)comp_ptr = region;
                break;
        }
    }
    if (err == LDG_ERR_AOK && (kind & LDG_MATH_PHYS_AGGREGATE_ID_BASE) != 0) { err = ldg_ecs_ctx_comp_add(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_TAG, tag); }

    if (err == LDG_ERR_AOK) { err = ldg_math_phys_region_total_add(&ctx->region[region], val, 0); }

    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        rollback_err = ldg_ecs_ctx_entity_destroy(ctx->world, entity);
        if (rollback_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = rollback_err; }

        return err;
    }

    ctx->region[region].body_cunt++;
    ctx->body_cunt++;
    *out = entity;
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_split_sum_get(const ldg_math_phys_ctx_t *ctx, const ldg_math_phys_region_t *region_ptr, const ldg_math_phys_body_desc_t *desc, uint64_t cunt, ldg_math_phys_conserved_t *sum)
{
    ldg_math_phys_body_val_t child_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t sum_val = LDG_STRUCT_ZERO_INIT;
    uint64_t child_idx = 0;
    uint32_t err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (child_idx = 0; child_idx < cunt; child_idx++)
    {
        err = ldg_math_phys_body_convert(region_ptr, &desc[child_idx], desc[child_idx].mass, desc[child_idx].charge, &child_val);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_conserved_add(&sum[0], &child_val.mass_c, &sum_val);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        sum[0] = sum_val;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            err = ldg_math_phys_conserved_add(&sum[1 + axis], &child_val.momentum_c[axis], &sum_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            sum[1 + axis] = sum_val;
        }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_split_child_spawn(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t child_kind, const ldg_math_phys_body_desc_t *child_desc, const ldg_math_phys_conserved_t *term, uint64_t *out)
{
    ldg_math_phys_body_val_t child_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_tag_t tag_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t sum_val = LDG_STRUCT_ZERO_INIT;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    double mass_val = 0.0;
    double momentum_val[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double energy_val = 0.0;
    double angular_val[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint32_t err = 0;
    uint32_t axis = 0;
    region_ptr = &ctx->region[region];
    err = ldg_math_phys_body_convert(region_ptr, child_desc, child_desc->mass, child_desc->charge, &child_val);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_conserved_add(&child_val.mass_c, &term[0], &sum_val);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(sum_val.sign != 0)) { return LDG_ERR_MATH_PHYS_CONSERVATION; }

    child_val.mass_c = sum_val;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_math_phys_conserved_add(&child_val.momentum_c[axis], &term[1 + axis], &sum_val);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        child_val.momentum_c[axis] = sum_val;
    }
    err = ldg_math_phys_conserved_to_double_convert(&child_val.mass_c, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_MASS), &mass_val);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_math_phys_conserved_to_double_convert(&child_val.momentum_c[axis], ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_MOMENTUM_X), &momentum_val[axis]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    if (mass_val > 0.0) { energy_val = (momentum_val[0] * momentum_val[0] + momentum_val[1] * momentum_val[1] + momentum_val[2] * momentum_val[2]) / (2.0 * mass_val); }

    err = ldg_math_phys_double_to_conserved_convert(energy_val, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY), &child_val.energy_c);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    angular_val[0] = child_desc->pos[1] * momentum_val[2] - child_desc->pos[2] * momentum_val[1];
    angular_val[1] = child_desc->pos[2] * momentum_val[0] - child_desc->pos[0] * momentum_val[2];
    angular_val[2] = child_desc->pos[0] * momentum_val[1] - child_desc->pos[1] * momentum_val[0];
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_math_phys_double_to_conserved_convert(angular_val[axis], ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_X), &child_val.angular_c[axis]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    tag_val.lepton = child_desc->lepton;
    tag_val.baryon = child_desc->baryon;
    return ldg_math_phys_body_wr(ctx, region, child_kind, &child_val, &tag_val, out);
}

static uint32_t ldg_math_phys_merge_sum_get(const ldg_math_phys_ctx_t *ctx, const uint64_t *body, uint64_t cunt, ldg_math_phys_body_val_t *val, ldg_math_phys_tag_t *tag, uint32_t *region_out, double *com, double *pos_avg, double *mass_total, double *angular_sum, double *inertia_total)
{
    ldg_math_phys_conserved_t sum_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t *conserved_ptr = 0x0;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    ldg_uint128_t *pos_ptr = 0x0;
    void *comp_ptr = 0x0;
    double momentum_d[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double pos_d[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double spin_d[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double mass_val = 0.0;
    double inertia_val = 0.0;
    double raw = 0.0;
    int64_t charge_quanta = 0;
    int64_t lepton_val = 0;
    int64_t baryon_val = 0;
    uint64_t src_idx = 0;
    uint32_t err = 0;
    uint32_t region = 0;
    uint32_t src_kind = 0;
    uint32_t axis = 0;
    for (src_idx = 0; src_idx < cunt; src_idx++)
    {
        if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(&ctx->world->store, body[src_idx]) != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

        if (LDG_UNLIKELY((ldg_ecs_ctx_entity_category_get(ctx->world, body[src_idx]) & LDG_MATH_PHYS_PIN_CATEGORY) != 0)) { return LDG_ERR_MATH_PHYS_DEMOTE_DENIED; }

        err = ldg_ecs_ctx_comp_get(ctx->world, body[src_idx], ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        region = *(uint32_t *)comp_ptr;
        if (src_idx == 0)
        {
            if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

            *region_out = region;
        }
        else if (LDG_UNLIKELY(region != *region_out)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

        region_ptr = &ctx->region[*region_out];
        err = ldg_ecs_ctx_comp_get(ctx->world, body[src_idx], ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MASS, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_conserved_add(&val->mass_c, (ldg_math_phys_conserved_t *)comp_ptr, &sum_val);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        val->mass_c = sum_val;
        err = ldg_math_phys_conserved_to_double_convert((ldg_math_phys_conserved_t *)comp_ptr, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_MASS), &mass_val);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_ecs_ctx_comp_get(ctx->world, body[src_idx], ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MOMENTUM, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            err = ldg_math_phys_conserved_add(&val->momentum_c[axis], &conserved_ptr[axis], &sum_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            val->momentum_c[axis] = sum_val;
            err = ldg_math_phys_conserved_to_double_convert(&conserved_ptr[axis], ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_MOMENTUM_X), &momentum_d[axis]);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
        }
        err = ldg_ecs_ctx_comp_get(ctx->world, body[src_idx], ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_CHARGE, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_conserved_add(&val->charge_c, (ldg_math_phys_conserved_t *)comp_ptr, &sum_val);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        val->charge_c = sum_val;
        err = ldg_ecs_ctx_comp_get(ctx->world, body[src_idx], ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_POS, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        pos_ptr = (ldg_uint128_t *)comp_ptr;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            err = ldg_arith_uint128_to_double_convert(&pos_ptr[axis], &raw);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            pos_d[axis] = ldg_math_pwr2_scale_get(raw, 0 - LDG_MATH_PHYS_POS_FRACTION);
            com[axis] += mass_val * pos_d[axis];
            pos_avg[axis] += pos_d[axis];
        }
        *mass_total += mass_val;
        err = ldg_ecs_ctx_comp_get(ctx->world, body[src_idx], ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_SPIN, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { spin_d[axis] = ((double *)comp_ptr)[axis]; }
        err = ldg_ecs_ctx_comp_get(ctx->world, body[src_idx], ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_INERTIA, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        inertia_val = *(double *)comp_ptr;
        *inertia_total += inertia_val;
        angular_sum[0] += pos_d[1] * momentum_d[2] - pos_d[2] * momentum_d[1] + inertia_val * spin_d[0];
        angular_sum[1] += pos_d[2] * momentum_d[0] - pos_d[0] * momentum_d[2] + inertia_val * spin_d[1];
        angular_sum[2] += pos_d[0] * momentum_d[1] - pos_d[1] * momentum_d[0] + inertia_val * spin_d[2];
        err = ldg_ecs_ctx_comp_get(ctx->world, body[src_idx], ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_KIND, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        src_kind = *(uint32_t *)comp_ptr;
        if (ldg_ecs_ctx_comp_has(ctx->world, body[src_idx], ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_TAG) == LDG_TRUTH_TRUE)
        {
            err = ldg_ecs_ctx_comp_get(ctx->world, body[src_idx], ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_TAG, &comp_ptr);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            tag->lepton += ((ldg_math_phys_tag_t *)comp_ptr)->lepton;
            tag->baryon += ((ldg_math_phys_tag_t *)comp_ptr)->baryon;
        }
        else
        {
            ldg_math_phys_particle_tag_get(src_kind, &charge_quanta, &lepton_val, &baryon_val);
            tag->lepton += lepton_val;
            tag->baryon += baryon_val;
        }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_field_point_energy_xfer(const ldg_math_phys_region_t *region_ptr, const double *pos, double energy, double dt, uint8_t withdraw)
{
    ldg_math_phys_field_hdr_t *hdr = 0x0;
    double *cur_buff = 0x0;
    double *prev_buff = 0x0;
    double cell_vol = 0.0;
    double grad_coef = 0.0;
    double quad_a = 0.0;
    double quad_b = 0.0;
    double disc = 0.0;
    double amplitude = 0.0;
    double spacing = 0.0;
    uint64_t cell[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t center = 0;
    uint64_t stride = 0;
    uint32_t axis = 0;
    uint32_t axis_cunt = 0;
    hdr = ldg_math_phys_field_hdr_get(region_ptr);
    cur_buff = ldg_math_phys_field_buff_get(region_ptr, hdr->cur_idx);
    prev_buff = ldg_math_phys_field_buff_get(region_ptr, hdr->prev_idx);
    spacing = region_ptr->desc.spacing;
    axis_cunt = (hdr->nz == 1) ? 2 : 3;
    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (axis = 0; axis < axis_cunt && axis < LDG_MATH_PHYS_DIM; axis++)
    {
        cell[axis] = (uint64_t)(pos[axis] / spacing + 0.5);
        if (cell[axis] >= dim[axis]) { cell[axis] = dim[axis] - 1; }
    }
    center = (cell[2] * hdr->ny + cell[1]) * hdr->nx + cell[0];
    cell_vol = (hdr->nz == 1) ? (spacing * spacing) : (spacing * spacing * spacing);
    grad_coef = cell_vol * region_ptr->desc.wave_speed * region_ptr->desc.wave_speed / (2.0 * spacing * spacing);
    quad_a = cell_vol / (2.0 * dt * dt);
    quad_b = cell_vol * (cur_buff[center] - prev_buff[center]) / (dt * dt);
    for (axis = 0; axis < axis_cunt && axis < LDG_MATH_PHYS_DIM; axis++)
    {
        stride = (axis == 0) ? 1 : ((axis == 1) ? hdr->nx : hdr->nx * hdr->ny);
        if (cell[axis] + 1 < dim[axis])
        {
            quad_a += grad_coef;
            quad_b += 2.0 * grad_coef * (cur_buff[center] - cur_buff[center + stride]);
        }

        if (cell[axis] > 0)
        {
            quad_a += grad_coef;
            quad_b += 2.0 * grad_coef * (cur_buff[center] - cur_buff[center - stride]);
        }
    }
    if (withdraw)
    {
        disc = quad_b * quad_b - 4.0 * quad_a * energy;
        if (disc < 0.0) { return LDG_ERR_MATH_PHYS_CONSERVATION_ENERGY; }
    }
    else { disc = quad_b * quad_b + 4.0 * quad_a * energy; }

    if (quad_b >= 0.0) { amplitude = ((0.0 - quad_b) + ldg_math_sqrt_get(disc)) / (2.0 * quad_a); }
    else { amplitude = ((0.0 - quad_b) - ldg_math_sqrt_get(disc)) / (2.0 * quad_a); }

    cur_buff[center] += amplitude;
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_history_frame_find(const ldg_math_phys_ctx_t *ctx, const ldg_uint128_t *time, uint32_t *out_frame_a, uint32_t *out_frame_b, double *out_ratio)
{
    ldg_math_phys_history_hdr_t *hdr = 0x0;
    ldg_uint128_t span = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t into = LDG_STRUCT_ZERO_INIT;
    double span_val = 0.0;
    double into_val = 0.0;
    uint64_t depth = 0;
    uint32_t err = 0;
    uint32_t ord = 0;
    uint32_t scan_idx = 0;
    uint32_t idx = 0;
    uint32_t prev_idx = 0;
    uint32_t oldest_idx = 0;
    uint32_t newest_idx = 0;
    *out_frame_a = 0;
    *out_frame_b = 0;
    *out_ratio = 0.0;
    hdr = ldg_math_phys_history_hdr_get(ctx);
    if (hdr->cunt == 0) { return LDG_ERR_MATH_PHYS_SAMPLE_FUTURE; }

    depth = ctx->conf.history_depth;
    newest_idx = (uint32_t)((hdr->hd + depth - 1) % depth);
    oldest_idx = (uint32_t)((hdr->hd + depth - hdr->cunt) % depth);
    err = ldg_arith_uint128_cmp(time, &ldg_math_phys_history_meta_get(ctx, newest_idx)->stamp, &ord);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (ord == LDG_ARITH_ORD_GT) { return LDG_ERR_MATH_PHYS_SAMPLE_FUTURE; }

    err = ldg_arith_uint128_cmp(time, &ldg_math_phys_history_meta_get(ctx, oldest_idx)->stamp, &ord);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (ord == LDG_ARITH_ORD_LT) { return LDG_ERR_MATH_PHYS_SAMPLE_EXPIRED; }

    prev_idx = oldest_idx;
    for (scan_idx = 0; scan_idx < hdr->cunt; scan_idx++)
    {
        idx = (uint32_t)((oldest_idx + scan_idx) % depth);
        err = ldg_arith_uint128_cmp(time, &ldg_math_phys_history_meta_get(ctx, idx)->stamp, &ord);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (ord == LDG_ARITH_ORD_EQ)
        {
            *out_frame_a = idx;
            *out_frame_b = idx;
            return LDG_ERR_AOK;
        }

        if (ord == LDG_ARITH_ORD_LT)
        {
            *out_frame_a = prev_idx;
            *out_frame_b = idx;
            err = ldg_arith_uint128_sub(&ldg_math_phys_history_meta_get(ctx, idx)->stamp, &ldg_math_phys_history_meta_get(ctx, prev_idx)->stamp, &span);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_arith_uint128_sub(time, &ldg_math_phys_history_meta_get(ctx, prev_idx)->stamp, &into);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_arith_uint128_to_double_convert(&span, &span_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_arith_uint128_to_double_convert(&into, &into_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            *out_ratio = into_val / span_val;
            return LDG_ERR_AOK;
        }

        prev_idx = idx;
    }
    return LDG_ERR_MATH_PHYS_SAMPLE_FUTURE;
}

LDG_EXPORT uint32_t ldg_math_phys_conf_default_get(ldg_math_phys_conf_t *out)
{
    ldg_math_phys_conf_t zero_conf = LDG_STRUCT_ZERO_INIT;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = zero_conf;
    out->region_cap = LDG_MATH_PHYS_REGION_CAP_DEFAULT;
    out->constraint_cap = LDG_MATH_PHYS_CONSTRAINT_CAP_DEFAULT;
    out->collider_cap = LDG_MATH_PHYS_COLLIDER_CAP_DEFAULT;
    out->aggregate_cap = LDG_MATH_PHYS_AGGREGATE_CAP_DEFAULT;
    out->history_depth = LDG_MATH_PHYS_HISTORY_DEPTH_DEFAULT;
    out->comp_id_base = LDG_MATH_PHYS_COMP_ID_BASE_DEFAULT;
    out->taxon_id_base = LDG_MATH_PHYS_TAXON_ID_BASE_DEFAULT;
    out->pacing = 0.0;
    out->tick_budget.limb[0] = UINT64_C(1) << LDG_MATH_PHYS_TICK_BUDGET_DEFAULT_SHIFT;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_init(ldg_math_phys_ctx_t *ctx, ldg_ecs_ctx_t *world, const ldg_math_phys_conf_t *conf)
{
    ldg_ecs_comp_desc_t comp_desc = LDG_STRUCT_ZERO_INIT;
    ldg_ecs_taxon_desc_t taxon_desc = LDG_STRUCT_ZERO_INIT;
    void *blk = 0x0;
    uint32_t err = 0;
    uint32_t undo_err = 0;
    uint32_t idx = 0;
    uint32_t comp_reg_cunt = 0;
    uint32_t taxon_reg_cunt = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!world)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init == LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_INIT; }

    if (LDG_UNLIKELY(world->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(conf->region_cap == 0)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(conf->aggregate_cap == 0)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(conf->constraint_cap == 0)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(conf->collider_cap == 0)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(conf->history_depth == 0)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(conf->aggregate_cap >= LDG_MATH_PHYS_AGGREGATE_ID_BASE)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(conf->comp_id_base < 2)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(conf->comp_id_base + LDG_MATH_PHYS_COMP_CUNT > world->conf.comp_cap)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(conf->taxon_id_base == LDG_ECS_TAXON_ID_INVALID)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(conf->taxon_id_base + LDG_MATH_PHYS_TIER_CUNT > world->conf.taxon_cap)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(conf->pacing) || conf->pacing < 0.0)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(conf->tick_budget.limb[0] == 0 && conf->tick_budget.limb[1] == 0)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    err = ldg_mem_secure_zero(ctx, sizeof(ldg_math_phys_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ctx->conf = *conf;
    ctx->world = world;
    ctx->pacing = conf->pacing;
    for (idx = 0; idx < LDG_MATH_PHYS_PARTICLE_CUNT; idx++) { ldg_math_phys_particle_seed_set(&ctx->particle[idx], idx); }
    err = ldg_mem_alloc(sizeof(ldg_math_phys_region_t) * conf->region_cap, &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ctx->region = (ldg_math_phys_region_t *)blk;
    err = ldg_mem_alloc(sizeof(ldg_math_phys_aggregate_t) * conf->aggregate_cap, &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        undo_err = ldg_math_phys_init_undo(ctx, 0, 0);
        if (undo_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = undo_err; }

        undo_err = ldg_mem_secure_zero(ctx, sizeof(ldg_math_phys_ctx_t));
        if (undo_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = undo_err; }

        return err;
    }

    ctx->aggregate = (ldg_math_phys_aggregate_t *)blk;
    err = ldg_mem_alloc(sizeof(ldg_math_phys_constraint_t) * conf->constraint_cap, &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        undo_err = ldg_math_phys_init_undo(ctx, 0, 0);
        if (undo_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = undo_err; }

        undo_err = ldg_mem_secure_zero(ctx, sizeof(ldg_math_phys_ctx_t));
        if (undo_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = undo_err; }

        return err;
    }

    ctx->constraint = (ldg_math_phys_constraint_t *)blk;
    err = ldg_mem_alloc(sizeof(ldg_math_phys_collider_t) * conf->collider_cap, &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        undo_err = ldg_math_phys_init_undo(ctx, 0, 0);
        if (undo_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = undo_err; }

        undo_err = ldg_mem_secure_zero(ctx, sizeof(ldg_math_phys_ctx_t));
        if (undo_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = undo_err; }

        return err;
    }

    ctx->collider = (ldg_math_phys_collider_t *)blk;
    err = ldg_mem_alloc(ldg_math_phys_history_size_get(conf->history_depth, world->conf.entity_cap), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        undo_err = ldg_math_phys_init_undo(ctx, 0, 0);
        if (undo_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = undo_err; }

        undo_err = ldg_mem_secure_zero(ctx, sizeof(ldg_math_phys_ctx_t));
        if (undo_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = undo_err; }

        return err;
    }

    ctx->history = blk;
    err = ldg_mem_secure_zero(ctx->history, ldg_math_phys_history_size_get(conf->history_depth, world->conf.entity_cap));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_secure_zero(ctx->region, sizeof(ldg_math_phys_region_t) * conf->region_cap);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_secure_zero(ctx->aggregate, sizeof(ldg_math_phys_aggregate_t) * conf->aggregate_cap);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_secure_zero(ctx->constraint, sizeof(ldg_math_phys_constraint_t) * conf->constraint_cap);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_secure_zero(ctx->collider, sizeof(ldg_math_phys_collider_t) * conf->collider_cap);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (idx = 0; idx < LDG_MATH_PHYS_COMP_CUNT; idx++)
    {
        ldg_math_phys_comp_desc_set(&comp_desc, idx, world->conf.entity_cap);
        err = ldg_ecs_ctx_comp_reg(world, conf->comp_id_base + idx, &comp_desc);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            undo_err = ldg_math_phys_init_undo(ctx, comp_reg_cunt, 0);
            if (undo_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = undo_err; }

            undo_err = ldg_mem_secure_zero(ctx, sizeof(ldg_math_phys_ctx_t));
            if (undo_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = undo_err; }

            return err;
        }

        comp_reg_cunt++;
    }
    for (idx = 0; idx < LDG_MATH_PHYS_COMP_CUNT; idx++) { if (idx == LDG_MATH_PHYS_COMP_TAG) { ldg_math_phys_mask_bit_set(taxon_desc.optional_mask, conf->comp_id_base + idx); }
        else { ldg_math_phys_mask_bit_set(taxon_desc.required_mask, conf->comp_id_base + idx); } }
    for (idx = 0; idx < LDG_MATH_PHYS_TIER_CUNT; idx++)
    {
        err = ldg_ecs_ctx_taxon_reg(world, conf->taxon_id_base + idx, &taxon_desc);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            undo_err = ldg_math_phys_init_undo(ctx, comp_reg_cunt, taxon_reg_cunt);
            if (undo_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = undo_err; }

            undo_err = ldg_mem_secure_zero(ctx, sizeof(ldg_math_phys_ctx_t));
            if (undo_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = undo_err; }

            return err;
        }

        taxon_reg_cunt++;
    }
    ctx->is_init = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_shutdown(ldg_math_phys_ctx_t *ctx)
{
    uint32_t ret = 0;
    uint32_t zero_err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    ret = ldg_math_phys_compute_teardown(ctx);
    zero_err = ldg_math_phys_init_undo(ctx, LDG_MATH_PHYS_COMP_CUNT, LDG_MATH_PHYS_TIER_CUNT);
    if (zero_err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = zero_err; }

    zero_err = ldg_mem_secure_zero(ctx, sizeof(ldg_math_phys_ctx_t));
    if (zero_err != LDG_ERR_AOK && ret == LDG_ERR_AOK) { ret = zero_err; }

    return ret;
}

LDG_EXPORT uint32_t ldg_math_phys_particle_get(const ldg_math_phys_ctx_t *ctx, uint32_t particle_id, ldg_math_phys_particle_t *out)
{
    ldg_math_phys_particle_t zero_particle = LDG_STRUCT_ZERO_INIT;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = zero_particle;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(particle_id >= LDG_MATH_PHYS_PARTICLE_CUNT)) { return LDG_ERR_NOT_FOUND; }

    *out = ctx->particle[particle_id];
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_aggregate_reg(ldg_math_phys_ctx_t *ctx, const ldg_math_phys_aggregate_desc_t *desc, uint32_t *out)
{
    uint32_t idx = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(desc->name[0] == '\0')) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < ctx->conf.aggregate_cap; idx++) { if (!ctx->aggregate[idx].used)
        {
            ldg_math_phys_name_copy(ctx->aggregate[idx].name, desc->name);
            ctx->aggregate[idx].used = 1;
            *out = LDG_MATH_PHYS_AGGREGATE_ID_BASE | idx;
            return LDG_ERR_AOK;
        }
    }
    return LDG_ERR_MATH_PHYS_AGGREGATE_FULL;
}

static void ldg_math_phys_optic_ordinate_fill(double *ordinate, uint64_t polar_cunt, uint64_t azim_cunt)
{
    double node = 0.0;
    double node_prev = 0.0;
    double legendre_cur = 0.0;
    double legendre_prev = 0.0;
    double legendre_next = 0.0;
    double slope = 0.0;
    double weight = 0.0;
    double sin_polar = 0.0;
    double azim = 0.0;
    uint64_t node_cunt = polar_cunt * 2;
    uint64_t node_idx = 0;
    uint64_t azim_idx = 0;
    uint64_t iter_idx = 0;
    uint64_t degree = 0;
    uint64_t row = 0;
    for (node_idx = 0; node_idx < node_cunt; node_idx++)
    {
        node = ldg_math_cos_get(LDG_MATH_PI * ((double)node_idx + 0.75) / ((double)node_cunt + 0.5));
        for (iter_idx = 0; iter_idx < 16; iter_idx++)
        {
            legendre_cur = node;
            legendre_prev = 1.0;
            for (degree = 2; degree <= node_cunt; degree++)
            {
                legendre_next = ((2.0 * (double)degree - 1.0) * node * legendre_cur - ((double)degree - 1.0) * legendre_prev) / (double)degree;
                legendre_prev = legendre_cur;
                legendre_cur = legendre_next;
            }
            slope = (double)node_cunt * (node * legendre_cur - legendre_prev) / (node * node - 1.0);
            node_prev = node;
            node = node - legendre_cur / slope;
            if (ldg_math_abs_get(node - node_prev) < 1.0e-15) { break; }
        }
        weight = 2.0 / ((1.0 - node * node) * slope * slope);
        sin_polar = ldg_math_sqrt_get(1.0 - node * node);
        for (azim_idx = 0; azim_idx < azim_cunt; azim_idx++)
        {
            azim = ((double)azim_idx + 0.5) * LDG_MATH_TAU / (double)azim_cunt;
            row = (node_idx * azim_cunt + azim_idx) * LDG_MATH_PHYS_ORDINATE_STRIDE;
            ordinate[row] = sin_polar * ldg_math_cos_get(azim);
            ordinate[row + 1] = sin_polar * ldg_math_sin_get(azim);
            ordinate[row + 2] = node;
            ordinate[row + 3] = weight * LDG_MATH_TAU / (double)azim_cunt;
        }
    }
}

LDG_EXPORT uint32_t ldg_math_phys_region_add(ldg_math_phys_ctx_t *ctx, const ldg_math_phys_region_desc_t *desc, uint32_t *out)
{
    ldg_math_phys_field_hdr_t *hdr = 0x0;
    ldg_math_phys_em_hdr_t *em_hdr = 0x0;
    ldg_math_phys_acoustic_hdr_t *acoustic_hdr = 0x0;
    ldg_math_phys_thermo_hdr_t *thermo_hdr = 0x0;
    ldg_math_phys_water_hdr_t *water_hdr = 0x0;
    ldg_math_phys_mtl_desc_t vacuum_desc = LDG_STRUCT_ZERO_INIT;
    void *field_blk = 0x0;
    double dt = 0.0;
    double dt_sub = 0.0;
    double light_speed = 0.0;
    uint64_t cell_x = 0;
    uint64_t cell_y = 0;
    uint64_t cell_z = 1;
    uint64_t cell_cunt = 0;
    uint64_t cell = 0;
    uint64_t size = 0;
    uint64_t total_bytes = 0;
    uint64_t face_max = 0;
    uint64_t conductor_offset = 0;
    uint64_t port_pattern_offset = 0;
    uint64_t port_tap_offset = 0;
    uint64_t port_tap_depth_res = 0;
    uint64_t subcycle_res = 0;
    uint64_t mtl_cap_res = 0;
    uint64_t depth_res = 0;
    uint64_t pml_min = 0;
    uint64_t axis_max = 0;
    uint64_t optic_ord_res = 0;
    uint64_t cell_x_pos = 0;
    uint64_t cell_y_pos = 0;
    uint64_t cell_z_pos = 0;
    uint32_t optic_polar_res = 0;
    uint32_t optic_azim_res = 0;
    uint32_t err = 0;
    uint32_t rollback_err = 0;
    uint32_t idx = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(desc->tier >= LDG_MATH_PHYS_TIER_CUNT)) { return LDG_ERR_MATH_PHYS_TIER_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(desc->extent[axis]) || desc->extent[axis] <= 0.0 || desc->extent[axis] >= 0x1p64)) { return LDG_ERR_FUNC_ARG_INVALID; } }
    if (LDG_UNLIKELY(desc->mass_scale_exponent > LDG_MATH_PHYS_CONSERVED_EXPONENT_MAX || desc->mass_scale_exponent < (0 - LDG_MATH_PHYS_CONSERVED_EXPONENT_MAX))) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(desc->energy_scale_exponent > LDG_MATH_PHYS_CONSERVED_EXPONENT_MAX || desc->energy_scale_exponent < (0 - LDG_MATH_PHYS_CONSERVED_EXPONENT_MAX))) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(desc->momentum_scale_exponent > LDG_MATH_PHYS_CONSERVED_EXPONENT_MAX || desc->momentum_scale_exponent < (0 - LDG_MATH_PHYS_CONSERVED_EXPONENT_MAX))) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(desc->angular_momentum_scale_exponent > LDG_MATH_PHYS_CONSERVED_EXPONENT_MAX || desc->angular_momentum_scale_exponent < (0 - LDG_MATH_PHYS_CONSERVED_EXPONENT_MAX))) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(desc->field_kind >= LDG_MATH_PHYS_FIELD_KIND_CUNT)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (desc->tier == LDG_MATH_PHYS_TIER_FIELD)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->spacing) || desc->spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(desc->extent[0] <= desc->spacing || desc->extent[1] <= desc->spacing)) { return LDG_ERR_FUNC_ARG_INVALID; }

        err = ldg_math_phys_dt_get(ctx, &dt);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        cell_x = (uint64_t)ldg_math_ceil_get(desc->extent[0] / desc->spacing) + 1;
        cell_y = (uint64_t)ldg_math_ceil_get(desc->extent[1] / desc->spacing) + 1;
        if (desc->extent[2] > desc->spacing) { cell_z = (uint64_t)ldg_math_ceil_get(desc->extent[2] / desc->spacing) + 1; }

        err = ldg_arith_uint64_mul(cell_x, cell_y, &cell_cunt);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_arith_uint64_mul(cell_cunt, cell_z, &cell_cunt);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (desc->field_kind == LDG_MATH_PHYS_FIELD_KIND_SCALAR)
        {
            if (LDG_UNLIKELY(!ldg_math_finite_is(desc->wave_speed) || desc->wave_speed < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(desc->boundary > LDG_MATH_PHYS_FIELD_BOUNDARY_ABSORBING)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(!ldg_math_finite_is(desc->damping) || desc->damping < 0.0 || desc->damping > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(!desc->step_skip && desc->wave_speed * dt / desc->spacing > 1.0 / ldg_math_sqrt_get((cell_z == 1) ? 2.0 : 3.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

            err = ldg_arith_uint64_mul(cell_cunt, sizeof(double), &size);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_arith_uint64_mul(size, 3 + (uint64_t)ctx->conf.history_depth, &total_bytes);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_arith_uint64_add(total_bytes, sizeof(ldg_math_phys_field_hdr_t), &total_bytes);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
        }
        else if (desc->field_kind == LDG_MATH_PHYS_FIELD_KIND_EM)
        {
            if (LDG_UNLIKELY(ldg_math_abs_get(desc->wave_speed) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(ldg_math_abs_get(desc->damping) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(ldg_math_abs_get(desc->pml_freq) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(desc->boundary == LDG_MATH_PHYS_FIELD_BOUNDARY_REFLECTIVE && desc->pml_depth != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(desc->boundary == LDG_MATH_PHYS_FIELD_BOUNDARY_ABSORBING && desc->pml_depth == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(desc->boundary > LDG_MATH_PHYS_FIELD_BOUNDARY_ABSORBING)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (desc->pml_depth != 0)
            {
                pml_min = (cell_x < cell_y) ? cell_x : cell_y;
                if (cell_z > 1 && cell_z < pml_min) { pml_min = cell_z; }

                if (LDG_UNLIKELY(2 * (uint64_t)desc->pml_depth + LDG_MATH_PHYS_FIELD_DIM_MIN > pml_min)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }
            }

            subcycle_res = (desc->subcycle_cunt == 0) ? 1 : desc->subcycle_cunt;
            if (LDG_UNLIKELY(subcycle_res > LDG_MATH_PHYS_SUBCYCLE_MAX)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

            mtl_cap_res = (desc->mtl_cap == 0) ? LDG_MATH_PHYS_MTL_CAP_DEFAULT : desc->mtl_cap;
            if (LDG_UNLIKELY(mtl_cap_res > UINT16_MAX + 1)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

            depth_res = (desc->em_history_depth == 0) ? ctx->conf.history_depth : desc->em_history_depth;
            dt_sub = dt / (double)subcycle_res;
            light_speed = 1.0 / ldg_math_sqrt_get(LDG_MATH_PHYS_VACUUM_PERMITTIVITY * LDG_MATH_PHYS_VACUUM_PERMEABILITY);
            if (LDG_UNLIKELY(!desc->step_skip && light_speed * dt_sub / desc->spacing > 1.0 / ldg_math_sqrt_get((cell_z == 1) ? 2.0 : 3.0))) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

            axis_max = (cell_x > cell_y) ? cell_x : cell_y;
            if (cell_z > axis_max) { axis_max = cell_z; }

            if (LDG_UNLIKELY(desc->optic_bin_cunt > LDG_MATH_PHYS_OPTIC_BIN_MAX)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

            if (desc->optic_bin_cunt == 0)
            {
                if (LDG_UNLIKELY(desc->optic_ord_polar_cunt != 0 || desc->optic_ord_azim_cunt != 0)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

                if (LDG_UNLIKELY(ldg_math_abs_get(desc->optic_wave_err_max) > 0.0 || ldg_math_abs_get(desc->optic_ray_err_max) > 0.0)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

                optic_ord_res = 0;
            }
            else
            {
                if (LDG_UNLIKELY(!ldg_math_finite_is(desc->optic_wave_err_max) || desc->optic_wave_err_max <= 0.0)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

                if (LDG_UNLIKELY(!ldg_math_finite_is(desc->optic_ray_err_max) || desc->optic_ray_err_max <= 0.0)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

                optic_polar_res = (desc->optic_ord_polar_cunt == 0) ? 2 : desc->optic_ord_polar_cunt;
                optic_azim_res = (desc->optic_ord_azim_cunt == 0) ? 4 : desc->optic_ord_azim_cunt;
                if (LDG_UNLIKELY(optic_polar_res > 32 || optic_azim_res > 64)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

                optic_ord_res = (uint64_t)optic_polar_res * 2 * (uint64_t)optic_azim_res;
            }

            if (LDG_UNLIKELY(desc->conductor_cap > LDG_MATH_PHYS_EM_CONDUCTOR_CAP)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

            if (LDG_UNLIKELY(desc->port_cap > LDG_MATH_PHYS_EM_PORT_CAP)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

            port_tap_depth_res = (desc->port_tap_depth == 0) ? LDG_MATH_PHYS_EM_PORT_TAP_DEPTH_DEFAULT : desc->port_tap_depth;
            face_max = cell_x * cell_y;
            if (cell_y * cell_z > face_max) { face_max = cell_y * cell_z; }

            if (cell_x * cell_z > face_max) { face_max = cell_x * cell_z; }

            err = ldg_math_phys_em_size_get(cell_cunt, cell_x + cell_y + cell_z, axis_max, mtl_cap_res, depth_res, desc->sheet_face_cap, desc->optic_bin_cunt, optic_ord_res, desc->conductor_cap, desc->port_cap, port_tap_depth_res, face_max, &conductor_offset, &port_pattern_offset, &port_tap_offset, &total_bytes);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
        }
        else if (desc->field_kind == LDG_MATH_PHYS_FIELD_KIND_THERMO)
        {
            if (LDG_UNLIKELY(ldg_math_abs_get(desc->wave_speed) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(ldg_math_abs_get(desc->damping) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(desc->pml_depth != 0 || ldg_math_abs_get(desc->pml_freq) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(!ldg_math_finite_is(desc->thermo_ambient) || desc->thermo_ambient < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            mtl_cap_res = (desc->mtl_cap == 0) ? LDG_MATH_PHYS_MTL_CAP_DEFAULT : desc->mtl_cap;
            if (LDG_UNLIKELY(mtl_cap_res > UINT16_MAX + 1)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

            err = ldg_math_phys_thermo_size_get(cell_cunt, mtl_cap_res, &total_bytes);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
        }
        else if (desc->field_kind == LDG_MATH_PHYS_FIELD_KIND_WATER)
        {
            if (LDG_UNLIKELY(ldg_math_abs_get(desc->wave_speed) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(ldg_math_abs_get(desc->damping) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(desc->pml_depth != 0 || ldg_math_abs_get(desc->pml_freq) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(!ldg_math_finite_is(desc->water_flip) || desc->water_flip < 0.0 || desc->water_flip > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(cell_x < LDG_MATH_PHYS_FIELD_DIM_MIN || cell_y < LDG_MATH_PHYS_FIELD_DIM_MIN || cell_z < LDG_MATH_PHYS_FIELD_DIM_MIN)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

            err = ldg_arith_uint64_mul(cell_x + 1, cell_y + 1, &face_max);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_arith_uint64_mul(face_max, cell_z + 1, &face_max);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_math_phys_water_size_get(cell_cunt, face_max, &total_bytes);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
        }
        else
        {
            if (LDG_UNLIKELY(ldg_math_abs_get(desc->wave_speed) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(ldg_math_abs_get(desc->damping) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(desc->boundary == LDG_MATH_PHYS_FIELD_BOUNDARY_REFLECTIVE && desc->pml_depth != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(desc->boundary == LDG_MATH_PHYS_FIELD_BOUNDARY_ABSORBING && desc->pml_depth == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(desc->boundary > LDG_MATH_PHYS_FIELD_BOUNDARY_ABSORBING)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(!ldg_math_finite_is(desc->pml_freq) || desc->pml_freq < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(desc->pml_depth != 0 && desc->pml_freq <= 0.0)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

            if (LDG_UNLIKELY(desc->pml_depth == 0 && desc->pml_freq > 0.0)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

            if (desc->pml_depth != 0)
            {
                pml_min = (cell_x < cell_y) ? cell_x : cell_y;
                if (cell_z > 1 && cell_z < pml_min) { pml_min = cell_z; }

                if (LDG_UNLIKELY(2 * (uint64_t)desc->pml_depth + LDG_MATH_PHYS_FIELD_DIM_MIN > pml_min)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }
            }

            subcycle_res = (desc->subcycle_cunt == 0) ? 1 : desc->subcycle_cunt;
            if (LDG_UNLIKELY(subcycle_res > LDG_MATH_PHYS_SUBCYCLE_MAX)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

            mtl_cap_res = (desc->mtl_cap == 0) ? LDG_MATH_PHYS_MTL_CAP_DEFAULT : desc->mtl_cap;
            if (LDG_UNLIKELY(mtl_cap_res > UINT16_MAX + 1)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

            depth_res = (desc->acoustic_history_depth == 0) ? ctx->conf.history_depth : desc->acoustic_history_depth;
            dt_sub = dt / (double)subcycle_res;
            axis_max = (cell_x > cell_y) ? cell_x : cell_y;
            if (cell_z > axis_max) { axis_max = cell_z; }

            err = ldg_math_phys_acoustic_size_get(cell_cunt, cell_x + cell_y + cell_z, mtl_cap_res, depth_res, desc->wall_face_cap, axis_max + 6, &total_bytes);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
        }
    }

    for (idx = 0; idx < ctx->conf.region_cap; idx++)
    {
        if (!ctx->region[idx].used)
        {
            if (desc->tier == LDG_MATH_PHYS_TIER_FIELD)
            {
                err = ldg_mem_alloc(total_bytes, &field_blk);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                err = ldg_mem_secure_zero(field_blk, total_bytes);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK))
                {
                    rollback_err = ldg_mem_dealloc(field_blk);
                    if (rollback_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = rollback_err; }

                    return err;
                }

                if (desc->field_kind == LDG_MATH_PHYS_FIELD_KIND_SCALAR)
                {
                    hdr = (ldg_math_phys_field_hdr_t *)field_blk;
                    hdr->nx = cell_x;
                    hdr->ny = cell_y;
                    hdr->nz = cell_z;
                    hdr->cell_cunt = cell_cunt;
                    hdr->cur_idx = 0;
                    hdr->prev_idx = 1;
                }
                else if (desc->field_kind == LDG_MATH_PHYS_FIELD_KIND_EM)
                {
                    em_hdr = (ldg_math_phys_em_hdr_t *)field_blk;
                    em_hdr->nx = cell_x;
                    em_hdr->ny = cell_y;
                    em_hdr->nz = cell_z;
                    em_hdr->cell_cunt = cell_cunt;
                    em_hdr->mtl_cap = mtl_cap_res;
                    em_hdr->mtl_cunt = 1;
                    em_hdr->subcycle_cunt = subcycle_res;
                    em_hdr->snapshot_depth = depth_res;
                    em_hdr->snapshot_stride = (desc->snapshot_stride == 0) ? 1 : desc->snapshot_stride;
                    em_hdr->energy_stride = (desc->energy_stride == 0) ? 1 : desc->energy_stride;
                    em_hdr->dt_sub = dt_sub;
                    em_hdr->pml_depth = desc->pml_depth;
                    em_hdr->aux_cunt = axis_max + 6;
                    em_hdr->face_cap = desc->sheet_face_cap;
                    em_hdr->optic_bin_cunt = desc->optic_bin_cunt;
                    em_hdr->optic_ord_cunt = optic_ord_res;
                    em_hdr->optic_wave_err_max = desc->optic_wave_err_max;
                    em_hdr->optic_ray_err_max = desc->optic_ray_err_max;
                    em_hdr->optic_src_cunt = 0;
                    em_hdr->optic_lambda_cunt = 0;
                    em_hdr->conductor_offset = conductor_offset;
                    em_hdr->conductor_cap = desc->conductor_cap;
                    em_hdr->conductor_cunt = 0;
                    em_hdr->port_cunt = 0;
                    em_hdr->port_cap = desc->port_cap;
                    em_hdr->port_pattern_offset = port_pattern_offset;
                    em_hdr->port_tap_offset = port_tap_offset;
                    em_hdr->port_tap_depth = port_tap_depth_res;
                    em_hdr->port_tap_hd = 0;
                    em_hdr->port_tap_cunt = 0;
                    em_hdr->face_max = face_max;
                    vacuum_desc.epsilon_r = 1.0;
                    vacuum_desc.mu_r = 1.0;
                    ldg_math_phys_em_mtl_coeff_set((double *)((uint8_t *)field_blk + sizeof(ldg_math_phys_em_hdr_t)), &vacuum_desc, dt_sub);
                    ctx->region[idx].field = field_blk;
                    ldg_math_phys_em_stretch_set(ldg_math_phys_em_stretch_get(&ctx->region[idx]), cell_x, cell_y, cell_z, desc->pml_depth, dt_sub, desc->spacing);
                    if (desc->conductor_cap > 0) { for (cell = 0; cell < cell_cunt; cell++) { ldg_math_phys_em_conductor_get(&ctx->region[idx])[cell] = UINT16_MAX; } }

                    if (desc->optic_bin_cunt != 0)
                    {
                        for (cell = 0; cell < cell_cunt; cell++)
                        {
                            ldg_math_phys_em_optic_work_get(&ctx->region[idx], 0)[cell] = LDG_MATH_PHYS_EM_PHASE_FAR;
                            ldg_math_phys_em_optic_work_get(&ctx->region[idx], 3)[cell] = LDG_MATH_PHYS_EM_PHASE_FAR;
                        }
                        ldg_math_phys_optic_ordinate_fill(ldg_math_phys_em_optic_ordinate_get(&ctx->region[idx]), optic_polar_res, optic_azim_res);
                    }
                }
                else if (desc->field_kind == LDG_MATH_PHYS_FIELD_KIND_WATER)
                {
                    water_hdr = (ldg_math_phys_water_hdr_t *)field_blk;
                    water_hdr->nx = cell_x;
                    water_hdr->ny = cell_y;
                    water_hdr->nz = cell_z;
                    water_hdr->cell_cunt = cell_cunt;
                    water_hdr->face_max = face_max;
                    water_hdr->particle_cap = LDG_MATH_PHYS_WATER_PARTICLE_PER_CELL * cell_cunt;
                    water_hdr->particle_cunt = 0;
                    water_hdr->iter_cunt = (desc->water_iter_cunt == 0) ? LDG_MATH_PHYS_WATER_ITER_DEFAULT : desc->water_iter_cunt;
                    water_hdr->flip = (ldg_math_abs_get(desc->water_flip) > 0.0) ? desc->water_flip : LDG_MATH_PHYS_WATER_FLIP_DEFAULT;
                    ctx->region[idx].field = field_blk;
                    for (cell = 0; cell < cell_cunt; cell++)
                    {
                        cell_x_pos = cell % cell_x;
                        cell_y_pos = (cell / cell_x) % cell_y;
                        cell_z_pos = cell / (cell_x * cell_y);
                        if (cell_x_pos == 0 || cell_y_pos == 0 || cell_z_pos == 0 || cell_x_pos + 1 == cell_x || cell_y_pos + 1 == cell_y || cell_z_pos + 1 == cell_z) { ldg_math_phys_water_mask_get(&ctx->region[idx])[cell] = LDG_MATH_PHYS_FLUID_CELL_SOLID; }
                    }
                }
                else if (desc->field_kind == LDG_MATH_PHYS_FIELD_KIND_THERMO)
                {
                    thermo_hdr = (ldg_math_phys_thermo_hdr_t *)field_blk;
                    thermo_hdr->nx = cell_x;
                    thermo_hdr->ny = cell_y;
                    thermo_hdr->nz = cell_z;
                    thermo_hdr->cell_cunt = cell_cunt;
                    thermo_hdr->mtl_cap = mtl_cap_res;
                    thermo_hdr->mtl_cunt = 1;
                    thermo_hdr->ambient = desc->thermo_ambient;
                    thermo_hdr->escaped_cum = 0.0;
                    thermo_hdr->injected_cum = 0.0;
                    ctx->region[idx].field = field_blk;
                    ldg_math_phys_thermo_em_coeff_get(&ctx->region[idx])[LDG_MATH_PHYS_EM_COEFF_EPSILON] = LDG_MATH_PHYS_VACUUM_PERMITTIVITY;
                    ldg_math_phys_thermo_em_coeff_get(&ctx->region[idx])[LDG_MATH_PHYS_EM_COEFF_MU] = LDG_MATH_PHYS_VACUUM_PERMEABILITY;
                    ldg_math_phys_optic_ordinate_fill(ldg_math_phys_thermo_ordinate_get(&ctx->region[idx]), 2, 2);
                    for (cell = 0; cell < cell_cunt; cell++)
                    {
                        ldg_math_phys_thermo_temp_get(&ctx->region[idx])[cell] = desc->thermo_ambient;
                        ldg_math_phys_thermo_hold_get(&ctx->region[idx])[cell] = 0.0 - 1.0;
                    }
                }
                else
                {
                    acoustic_hdr = (ldg_math_phys_acoustic_hdr_t *)field_blk;
                    acoustic_hdr->nx = cell_x;
                    acoustic_hdr->ny = cell_y;
                    acoustic_hdr->nz = cell_z;
                    acoustic_hdr->cell_cunt = cell_cunt;
                    acoustic_hdr->mtl_cap = mtl_cap_res;
                    acoustic_hdr->mtl_cunt = 0;
                    acoustic_hdr->subcycle_cunt = subcycle_res;
                    acoustic_hdr->snapshot_depth = depth_res;
                    acoustic_hdr->snapshot_stride = (desc->snapshot_stride == 0) ? 1 : desc->snapshot_stride;
                    acoustic_hdr->energy_stride = (desc->energy_stride == 0) ? 1 : desc->energy_stride;
                    acoustic_hdr->dt_sub = dt_sub;
                    acoustic_hdr->pml_depth = desc->pml_depth;
                    acoustic_hdr->pml_freq = desc->pml_freq;
                    acoustic_hdr->face_cap = desc->wall_face_cap;
                    acoustic_hdr->aux_cunt = axis_max + 6;
                    ctx->region[idx].field = field_blk;
                }

                if (desc->field_kind == LDG_MATH_PHYS_FIELD_KIND_SCALAR) { ctx->region[idx].field = field_blk; }
            }

            ctx->region[idx].desc = *desc;
            ctx->region[idx].used = 1;
            *out = idx;
            return LDG_ERR_AOK;
        }
    }
    return LDG_ERR_MATH_PHYS_REGION_FULL;
}

LDG_EXPORT uint32_t ldg_math_phys_optic_lambda_set(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lambda, uint32_t cunt)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    double *lambda_tbl = 0x0;
    uint64_t bin_idx = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!lambda)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_EM)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->optic_bin_cunt == 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(cunt != hdr->optic_bin_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (bin_idx = 0; bin_idx < cunt; bin_idx++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(lambda[bin_idx]) || lambda[bin_idx] <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(bin_idx > 0 && lambda[bin_idx] <= lambda[bin_idx - 1])) { return LDG_ERR_FUNC_ARG_INVALID; }
    }
    lambda_tbl = ldg_math_phys_em_optic_lambda_get(&ctx->region[region]);
    for (bin_idx = 0; bin_idx < cunt; bin_idx++) { lambda_tbl[bin_idx] = lambda[bin_idx]; }
    hdr->optic_lambda_cunt = cunt;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_optic_mtl_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t mtl, const double *absorb, const double *scatter_val, const double *refract)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    double *mtl_tbl = 0x0;
    uint64_t bin_idx = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!absorb)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!scatter_val)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!refract)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_EM)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->optic_bin_cunt == 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(mtl >= hdr->mtl_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (bin_idx = 0; bin_idx < hdr->optic_bin_cunt; bin_idx++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(absorb[bin_idx]) || absorb[bin_idx] < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(scatter_val[bin_idx]) || scatter_val[bin_idx] < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(refract[bin_idx]) || refract[bin_idx] < 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }
    }
    mtl_tbl = ldg_math_phys_em_optic_mtl_get(&ctx->region[region]);
    for (bin_idx = 0; bin_idx < hdr->optic_bin_cunt; bin_idx++)
    {
        mtl_tbl[(bin_idx * hdr->mtl_cap + mtl) * 3] = absorb[bin_idx];
        mtl_tbl[(bin_idx * hdr->mtl_cap + mtl) * 3 + 1] = scatter_val[bin_idx];
        mtl_tbl[(bin_idx * hdr->mtl_cap + mtl) * 3 + 2] = refract[bin_idx];
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_optic_src_add(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, const double *power)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    double *src_row = 0x0;
    double *emission = 0x0;
    double cell_vol = 0.0;
    uint64_t cell_idx[3] = LDG_ARR_ZERO_INIT;
    uint64_t dim_cunt[3] = LDG_ARR_ZERO_INIT;
    uint64_t center = 0;
    uint64_t bin_idx = 0;
    uint64_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!power)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_EM)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->optic_bin_cunt == 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(hdr->optic_src_cunt >= LDG_MATH_PHYS_OPTIC_SRC_CAP)) { return LDG_ERR_FULL; }

    dim_cunt[0] = hdr->nx;
    dim_cunt[1] = hdr->ny;
    dim_cunt[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(pos[axis]) || pos[axis] < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        cell_idx[axis] = (uint64_t)(pos[axis] / ctx->region[region].desc.spacing);
        if (LDG_UNLIKELY(cell_idx[axis] >= dim_cunt[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
    }
    for (bin_idx = 0; bin_idx < hdr->optic_bin_cunt; bin_idx++) { if (LDG_UNLIKELY(!ldg_math_finite_is(power[bin_idx]) || power[bin_idx] < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; } }
    center = (cell_idx[2] * hdr->ny + cell_idx[1]) * hdr->nx + cell_idx[0];
    cell_vol = (hdr->nz == 1) ? (ctx->region[region].desc.spacing * ctx->region[region].desc.spacing) : (ctx->region[region].desc.spacing * ctx->region[region].desc.spacing * ctx->region[region].desc.spacing);
    src_row = ldg_math_phys_em_optic_src_get(&ctx->region[region]) + hdr->optic_src_cunt * (3 + hdr->optic_bin_cunt);
    emission = ldg_math_phys_em_optic_emission_get(&ctx->region[region]);
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { src_row[axis] = pos[axis]; }
    for (bin_idx = 0; bin_idx < hdr->optic_bin_cunt; bin_idx++)
    {
        src_row[3 + bin_idx] = power[bin_idx];
        emission[center] += power[bin_idx] / (4.0 * LDG_MATH_PI * cell_vol);
    }
    hdr->optic_src_cunt++;
    return LDG_ERR_AOK;
}

static void ldg_math_phys_optic_scratch_fill(const ldg_math_phys_region_t *region_ptr, uint64_t bin_idx)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    const double *mtl_tbl = ldg_math_phys_em_optic_mtl_get(region_ptr);
    double *coeff = ldg_math_phys_em_optic_scratch_coeff_get(region_ptr);
    double *scatter_tbl = ldg_math_phys_em_optic_scratch_scatter_get(region_ptr);
    double imp_vacuum = ldg_math_sqrt_get(LDG_MATH_PHYS_VACUUM_PERMEABILITY / LDG_MATH_PHYS_VACUUM_PERMITTIVITY);
    double refract_val = 0.0;
    uint64_t mtl_idx = 0;
    for (mtl_idx = 0; mtl_idx < hdr->mtl_cunt; mtl_idx++)
    {
        refract_val = mtl_tbl[(bin_idx * hdr->mtl_cap + mtl_idx) * 3 + 2];
        if (refract_val < 1.0) { refract_val = 1.0; }

        coeff[mtl_idx * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_EPSILON] = refract_val * refract_val * LDG_MATH_PHYS_VACUUM_PERMITTIVITY;
        coeff[mtl_idx * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_MU] = LDG_MATH_PHYS_VACUUM_PERMEABILITY;
        coeff[mtl_idx * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_SIGMA] = mtl_tbl[(bin_idx * hdr->mtl_cap + mtl_idx) * 3] * refract_val / imp_vacuum;
        coeff[mtl_idx * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_CA] = 0.0;
        coeff[mtl_idx * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_CB] = 0.0;
        coeff[mtl_idx * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_DB] = 0.0;
        scatter_tbl[mtl_idx] = mtl_tbl[(bin_idx * hdr->mtl_cap + mtl_idx) * 3 + 1];
    }
}

static uint64_t ldg_math_phys_optic_src_cell_get(const ldg_math_phys_em_hdr_t *hdr, const double *src_row, double spacing)
{
    uint64_t cell_x = (uint64_t)(src_row[0] / spacing);
    uint64_t cell_y = (uint64_t)(src_row[1] / spacing);
    uint64_t cell_z = (uint64_t)(src_row[2] / spacing);
    return (cell_z * hdr->ny + cell_y) * hdr->nx + cell_x;
}

static double ldg_math_phys_optic_inject_weight_get(const ldg_math_phys_em_hdr_t *hdr, const double *phase, const double *coeff, const uint16_t *mtl, uint64_t center, double spacing)
{
    double refract_center = ldg_math_sqrt_get(coeff[(uint64_t)mtl[center] * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_EPSILON] / LDG_MATH_PHYS_VACUUM_PERMITTIVITY);
    double weight_sum = 0.0;
    double slope = 0.0;
    double refract_nb = 0.0;
    uint64_t plane = hdr->nx * hdr->ny;
    uint64_t nb = 0;
    uint64_t face_idx = 0;
    uint64_t pos_x = center % hdr->nx;
    uint64_t pos_y = (center / hdr->nx) % hdr->ny;
    uint64_t pos_z = center / plane;
    uint8_t nb_valid = 0;
    for (face_idx = 0; face_idx < 6; face_idx++)
    {
        nb_valid = 0;
        if (face_idx == 0 && pos_x > 0)
        {
            nb = center - 1;
            nb_valid = 1;
        }

        if (face_idx == 1 && pos_x + 1 < hdr->nx)
        {
            nb = center + 1;
            nb_valid = 1;
        }

        if (face_idx == 2 && pos_y > 0)
        {
            nb = center - hdr->nx;
            nb_valid = 1;
        }

        if (face_idx == 3 && pos_y + 1 < hdr->ny)
        {
            nb = center + hdr->nx;
            nb_valid = 1;
        }

        if (face_idx == 4 && hdr->nz > 1 && pos_z > 0)
        {
            nb = center - plane;
            nb_valid = 1;
        }

        if (face_idx == 5 && hdr->nz > 1 && pos_z + 1 < hdr->nz)
        {
            nb = center + plane;
            nb_valid = 1;
        }

        if (nb_valid == 0) { continue; }

        if (phase[nb] <= phase[center]) { continue; }

        refract_nb = ldg_math_sqrt_get(coeff[(uint64_t)mtl[nb] * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_EPSILON] / LDG_MATH_PHYS_VACUUM_PERMITTIVITY);
        slope = (phase[nb] - phase[center]) / (spacing * 0.5 * (refract_center + refract_nb));
        if (slope > 1.0) { slope = 1.0; }

        weight_sum += slope;
    }
    return weight_sum;
}

LDG_EXPORT uint32_t ldg_math_phys_optic_solve(ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    const uint16_t *mtl = 0x0;
    const double *lambda_tbl = 0x0;
    const double *src_tbl = 0x0;
    const double *src_row = 0x0;
    double *scratch_coeff = 0x0;
    double *scratch_scatter = 0x0;
    double *flux_bin = 0x0;
    double *ledger = 0x0;
    double *phase = 0x0;
    double *intensity = 0x0;
    double *reflect = 0x0;
    double *phase_next = 0x0;
    double *intensity_next = 0x0;
    double *reflect_next = 0x0;
    double *emission = 0x0;
    double spacing = 0.0;
    double cell_vol = 0.0;
    double face_area = 0.0;
    double weight_sum = 0.0;
    double injected = 0.0;
    double absorbed = 0.0;
    double escaped = 0.0;
    double reflected = 0.0;
    double stage_absorbed = 0.0;
    double stage_escaped = 0.0;
    double stage_reflected = 0.0;
    uint64_t bin_idx = 0;
    uint64_t src_idx = 0;
    uint64_t center = 0;
    uint64_t cell_idx = 0;
    uint64_t bounce_idx = 0;
    uint64_t pass_used = 0;
    uint32_t err = 0;
    uint32_t eval_kind = 0;
    uint8_t scatter_present = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_EM)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->optic_bin_cunt == 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(hdr->optic_lambda_cunt != hdr->optic_bin_cunt)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(hdr->optic_src_cunt == 0)) { return LDG_ERR_EMPTY; }

    spacing = ctx->region[region].desc.spacing;
    cell_vol = (hdr->nz == 1) ? (spacing * spacing) : (spacing * spacing * spacing);
    face_area = (hdr->nz == 1) ? spacing : (spacing * spacing);
    mtl = ldg_math_phys_em_mtl_get(&ctx->region[region]);
    lambda_tbl = ldg_math_phys_em_optic_lambda_get(&ctx->region[region]);
    src_tbl = ldg_math_phys_em_optic_src_get(&ctx->region[region]);
    scratch_coeff = ldg_math_phys_em_optic_scratch_coeff_get(&ctx->region[region]);
    scratch_scatter = ldg_math_phys_em_optic_scratch_scatter_get(&ctx->region[region]);
    ledger = ldg_math_phys_em_optic_ledger_get(&ctx->region[region]);
    phase = ldg_math_phys_em_optic_work_get(&ctx->region[region], 0);
    intensity = ldg_math_phys_em_optic_work_get(&ctx->region[region], 1);
    reflect = ldg_math_phys_em_optic_work_get(&ctx->region[region], 2);
    phase_next = ldg_math_phys_em_optic_work_get(&ctx->region[region], 3);
    intensity_next = ldg_math_phys_em_optic_work_get(&ctx->region[region], 4);
    reflect_next = ldg_math_phys_em_optic_work_get(&ctx->region[region], 5);
    emission = ldg_math_phys_em_optic_emission_get(&ctx->region[region]);
    for (bin_idx = 0; bin_idx < hdr->optic_bin_cunt; bin_idx++)
    {
        err = ldg_math_phys_field_em_eval_sel(spacing, lambda_tbl[bin_idx], hdr->optic_wave_err_max, hdr->optic_ray_err_max, &eval_kind);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (LDG_UNLIKELY(eval_kind != LDG_MATH_PHYS_EM_EVAL_RAY)) { return LDG_ERR_UNSUPPORTED; }

        ldg_math_phys_optic_scratch_fill(&ctx->region[region], bin_idx);
        scatter_present = 0;
        for (cell_idx = 0; cell_idx < hdr->mtl_cunt; cell_idx++) { if (scratch_scatter[cell_idx] > 0.0) { scatter_present = 1; } }
        flux_bin = ldg_math_phys_em_optic_flux_get(&ctx->region[region]) + bin_idx * hdr->cell_cunt;
        injected = 0.0;
        absorbed = 0.0;
        escaped = 0.0;
        reflected = 0.0;
        for (src_idx = 0; src_idx < hdr->optic_src_cunt; src_idx++) { injected += src_tbl[src_idx * (3 + hdr->optic_bin_cunt) + 3 + bin_idx]; }
        if (scatter_present == 1)
        {
            for (center = 0; center < hdr->cell_cunt; center++) { emission[center] = 0.0; }
            for (src_idx = 0; src_idx < hdr->optic_src_cunt; src_idx++)
            {
                src_row = src_tbl + src_idx * (3 + hdr->optic_bin_cunt);
                center = ldg_math_phys_optic_src_cell_get(hdr, src_row, spacing);
                emission[center] += src_row[3 + bin_idx] / (4.0 * LDG_MATH_PI * cell_vol);
            }
            err = ldg_math_phys_field_em_radiance_solve(ldg_math_phys_em_optic_radiance_get(&ctx->region[region]), flux_bin, emission, mtl, scratch_coeff, scratch_scatter, ldg_math_phys_em_optic_ordinate_get(&ctx->region[region]), hdr->optic_ord_cunt, hdr->nx, hdr->ny, hdr->nz, spacing, 1.0e-12, 1024, &pass_used);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_math_phys_field_em_radiance_conserved_get(ldg_math_phys_em_optic_radiance_get(&ctx->region[region]), emission, mtl, scratch_coeff, ldg_math_phys_em_optic_ordinate_get(&ctx->region[region]), hdr->optic_ord_cunt, hdr->nx, hdr->ny, hdr->nz, spacing, &absorbed, &escaped, &injected);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
        }
        else
        {
            for (center = 0; center < hdr->cell_cunt; center++)
            {
                phase[center] = LDG_MATH_PHYS_EM_PHASE_FAR;
                intensity[center] = 0.0;
            }
            for (src_idx = 0; src_idx < hdr->optic_src_cunt; src_idx++)
            {
                src_row = src_tbl + src_idx * (3 + hdr->optic_bin_cunt);
                phase[ldg_math_phys_optic_src_cell_get(hdr, src_row, spacing)] = 0.0;
            }
            err = ldg_math_phys_field_em_phase_solve(phase, mtl, scratch_coeff, hdr->nx, hdr->ny, hdr->nz, spacing, 1.0e-12, 64, &pass_used);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            for (src_idx = 0; src_idx < hdr->optic_src_cunt; src_idx++)
            {
                src_row = src_tbl + src_idx * (3 + hdr->optic_bin_cunt);
                center = ldg_math_phys_optic_src_cell_get(hdr, src_row, spacing);
                weight_sum = ldg_math_phys_optic_inject_weight_get(hdr, phase, scratch_coeff, mtl, center, spacing);
                if (weight_sum > 0.0) { intensity[center] += src_row[3 + bin_idx] / (weight_sum * face_area); }
            }
            err = ldg_math_phys_field_em_intensity_solve(intensity, reflect, phase, mtl, scratch_coeff, hdr->nx, hdr->ny, hdr->nz, spacing, 1.0e-12, 64, &pass_used);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_math_phys_field_em_intensity_conserved_get(intensity, reflect, phase, mtl, scratch_coeff, hdr->nx, hdr->ny, hdr->nz, spacing, &absorbed, &escaped, &reflected);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            for (center = 0; center < hdr->cell_cunt; center++) { flux_bin[center] = intensity[center]; }
            for (bounce_idx = 0; bounce_idx < 8; bounce_idx++)
            {
                err = ldg_math_phys_field_em_bounce_solve(phase_next, intensity_next, reflect_next, reflect, mtl, scratch_coeff, hdr->nx, hdr->ny, hdr->nz, spacing, 1.0e-12, 64, &pass_used);
                if (err == LDG_ERR_EMPTY)
                {
                    reflected = 0.0;
                    break;
                }

                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                err = ldg_math_phys_field_em_intensity_conserved_get(intensity_next, reflect_next, phase_next, mtl, scratch_coeff, hdr->nx, hdr->ny, hdr->nz, spacing, &stage_absorbed, &stage_escaped, &stage_reflected);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                absorbed += stage_absorbed;
                escaped += stage_escaped;
                reflected = stage_reflected;
                for (center = 0; center < hdr->cell_cunt; center++)
                {
                    flux_bin[center] += intensity_next[center];
                    reflect[center] = reflect_next[center];
                }
            }
        }

        ledger[bin_idx * LDG_MATH_PHYS_OPTIC_LEDGER_STRIDE] = injected;
        ledger[bin_idx * LDG_MATH_PHYS_OPTIC_LEDGER_STRIDE + 1] = absorbed;
        ledger[bin_idx * LDG_MATH_PHYS_OPTIC_LEDGER_STRIDE + 2] = escaped;
        ledger[bin_idx * LDG_MATH_PHYS_OPTIC_LEDGER_STRIDE + 3] = reflected;
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_optic_conserved_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t bin, double *out_injected, double *out_absorbed, double *out_escaped, double *out_residual)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    const double *ledger = 0x0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_injected)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_absorbed)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_escaped)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_residual)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_injected = 0.0;
    *out_absorbed = 0.0;
    *out_escaped = 0.0;
    *out_residual = 0.0;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_EM)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->optic_bin_cunt == 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(bin >= hdr->optic_bin_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ledger = ldg_math_phys_em_optic_ledger_get(&ctx->region[region]);
    *out_injected = ledger[(uint64_t)bin * LDG_MATH_PHYS_OPTIC_LEDGER_STRIDE];
    *out_absorbed = ledger[(uint64_t)bin * LDG_MATH_PHYS_OPTIC_LEDGER_STRIDE + 1];
    *out_escaped = ledger[(uint64_t)bin * LDG_MATH_PHYS_OPTIC_LEDGER_STRIDE + 2];
    *out_residual = ledger[(uint64_t)bin * LDG_MATH_PHYS_OPTIC_LEDGER_STRIDE + 3];
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_optic_flux_sample_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t bin, const double *pos, double *out)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    const double *flux_bin = 0x0;
    double spacing = 0.0;
    uint64_t cell_idx[3] = LDG_ARR_ZERO_INIT;
    uint64_t dim_cunt[3] = LDG_ARR_ZERO_INIT;
    uint64_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_EM)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->optic_bin_cunt == 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(bin >= hdr->optic_bin_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    spacing = ctx->region[region].desc.spacing;
    dim_cunt[0] = hdr->nx;
    dim_cunt[1] = hdr->ny;
    dim_cunt[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(pos[axis]) || pos[axis] < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        cell_idx[axis] = (uint64_t)(pos[axis] / spacing);
        if (LDG_UNLIKELY(cell_idx[axis] >= dim_cunt[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
    }
    flux_bin = ldg_math_phys_em_optic_flux_get(&ctx->region[region]) + (uint64_t)bin * hdr->cell_cunt;
    *out = flux_bin[(cell_idx[2] * hdr->ny + cell_idx[1]) * hdr->nx + cell_idx[0]];
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_optic_flux_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t bin, double *out)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    const double *flux_bin = 0x0;
    uint64_t cell = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_EM)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->optic_bin_cunt == 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(bin >= hdr->optic_bin_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    flux_bin = ldg_math_phys_em_optic_flux_get(&ctx->region[region]) + (uint64_t)bin * hdr->cell_cunt;
    for (cell = 0; cell < hdr->cell_cunt; cell++) { out[cell] = flux_bin[cell]; }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_thermo_region_ck(const ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_THERMO)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_thermo_mtl_reg(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_thermo_mtl_desc_t *desc, uint32_t *out)
{
    ldg_math_phys_thermo_hdr_t *hdr = 0x0;
    double *row = 0x0;
    double *em_row = 0x0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    err = ldg_math_phys_thermo_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->conductivity) || desc->conductivity < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->density) || desc->density <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->heat_cap) || desc->heat_cap <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->emissivity) || desc->emissivity < 0.0 || desc->emissivity > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->absorb) || desc->absorb < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->molar_mass) || desc->molar_mass <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->convection) || desc->convection < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    hdr = ldg_math_phys_thermo_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->mtl_cunt >= hdr->mtl_cap)) { return LDG_ERR_FULL; }

    row = ldg_math_phys_thermo_coeff_get(&ctx->region[region]) + hdr->mtl_cunt * LDG_MATH_PHYS_THERMO_COEFF_STRIDE;
    row[LDG_MATH_PHYS_THERMO_COEFF_K] = desc->conductivity;
    row[LDG_MATH_PHYS_THERMO_COEFF_RHO] = desc->density;
    row[LDG_MATH_PHYS_THERMO_COEFF_CAP] = desc->heat_cap;
    row[LDG_MATH_PHYS_THERMO_COEFF_EMIT] = desc->emissivity;
    row[LDG_MATH_PHYS_THERMO_COEFF_ABSORB] = desc->absorb;
    row[LDG_MATH_PHYS_THERMO_COEFF_MOLAR] = desc->molar_mass;
    row[LDG_MATH_PHYS_THERMO_COEFF_CONVECTION] = desc->convection;
    em_row = ldg_math_phys_thermo_em_coeff_get(&ctx->region[region]) + hdr->mtl_cunt * LDG_MATH_PHYS_EM_COEFF_STRIDE;
    em_row[LDG_MATH_PHYS_EM_COEFF_EPSILON] = LDG_MATH_PHYS_VACUUM_PERMITTIVITY;
    em_row[LDG_MATH_PHYS_EM_COEFF_MU] = LDG_MATH_PHYS_VACUUM_PERMEABILITY;
    em_row[LDG_MATH_PHYS_EM_COEFF_SIGMA] = desc->absorb / ldg_math_sqrt_get(LDG_MATH_PHYS_VACUUM_PERMEABILITY / LDG_MATH_PHYS_VACUUM_PERMITTIVITY);
    ldg_math_phys_thermo_scatter_get(&ctx->region[region])[hdr->mtl_cunt] = 0.0;
    *out = (uint32_t)hdr->mtl_cunt;
    hdr->mtl_cunt++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_thermo_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi, uint32_t mtl, double temp_k, uint32_t hold)
{
    ldg_math_phys_thermo_hdr_t *hdr = 0x0;
    double *temp = 0x0;
    double *hold_grid = 0x0;
    uint16_t *mtl_grid = 0x0;
    double spacing = 0.0;
    uint64_t box_lo[3] = LDG_ARR_ZERO_INIT;
    uint64_t box_hi[3] = LDG_ARR_ZERO_INIT;
    uint64_t dim[3] = LDG_ARR_ZERO_INIT;
    uint64_t pos_x = 0;
    uint64_t pos_y = 0;
    uint64_t pos_z = 0;
    uint64_t cell = 0;
    uint32_t axis = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!lo || !hi)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_thermo_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(temp_k) || temp_k < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(hold > 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    hdr = ldg_math_phys_thermo_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(mtl >= hdr->mtl_cunt)) { return LDG_ERR_NOT_FOUND; }

    spacing = ctx->region[region].desc.spacing;
    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(lo[axis]) || !ldg_math_finite_is(hi[axis]) || lo[axis] > hi[axis])) { return LDG_ERR_FUNC_ARG_INVALID; }

        box_lo[axis] = (uint64_t)((lo[axis] < 0.0) ? 0.0 : (lo[axis] / spacing + 0.5));
        box_hi[axis] = (uint64_t)(hi[axis] / spacing + 0.5);
        if (box_hi[axis] >= dim[axis]) { box_hi[axis] = dim[axis] - 1; }

        if (LDG_UNLIKELY(box_lo[axis] > box_hi[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
    }
    temp = ldg_math_phys_thermo_temp_get(&ctx->region[region]);
    hold_grid = ldg_math_phys_thermo_hold_get(&ctx->region[region]);
    mtl_grid = ldg_math_phys_thermo_mtl_get(&ctx->region[region]);
    for (pos_z = box_lo[2]; pos_z <= box_hi[2]; pos_z++) { for (pos_y = box_lo[1]; pos_y <= box_hi[1]; pos_y++) { for (pos_x = box_lo[0]; pos_x <= box_hi[0]; pos_x++)
            {
                cell = (pos_z * hdr->ny + pos_y) * hdr->nx + pos_x;
                mtl_grid[cell] = (uint16_t)mtl;
                temp[cell] = temp_k;
                hold_grid[cell] = (hold == 1) ? temp_k : (0.0 - 1.0);
            }
        } }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_thermo_pwr_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi, double pwr)
{
    ldg_math_phys_thermo_hdr_t *hdr = 0x0;
    double *pwr_grid = 0x0;
    double spacing = 0.0;
    uint64_t box_lo[3] = LDG_ARR_ZERO_INIT;
    uint64_t box_hi[3] = LDG_ARR_ZERO_INIT;
    uint64_t dim[3] = LDG_ARR_ZERO_INIT;
    uint64_t pos_x = 0;
    uint64_t pos_y = 0;
    uint64_t pos_z = 0;
    uint64_t cell = 0;
    uint32_t axis = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!lo || !hi)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_thermo_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(pwr))) { return LDG_ERR_FUNC_ARG_INVALID; }

    hdr = ldg_math_phys_thermo_hdr_get(&ctx->region[region]);
    spacing = ctx->region[region].desc.spacing;
    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(lo[axis]) || !ldg_math_finite_is(hi[axis]) || lo[axis] > hi[axis])) { return LDG_ERR_FUNC_ARG_INVALID; }

        box_lo[axis] = (uint64_t)((lo[axis] < 0.0) ? 0.0 : (lo[axis] / spacing + 0.5));
        box_hi[axis] = (uint64_t)(hi[axis] / spacing + 0.5);
        if (box_hi[axis] >= dim[axis]) { box_hi[axis] = dim[axis] - 1; }

        if (LDG_UNLIKELY(box_lo[axis] > box_hi[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
    }
    pwr_grid = ldg_math_phys_thermo_pwr_get(&ctx->region[region]);
    for (pos_z = box_lo[2]; pos_z <= box_hi[2]; pos_z++) { for (pos_y = box_lo[1]; pos_y <= box_hi[1]; pos_y++) { for (pos_x = box_lo[0]; pos_x <= box_hi[0]; pos_x++)
            {
                cell = (pos_z * hdr->ny + pos_y) * hdr->nx + pos_x;
                pwr_grid[cell] = pwr;
            }
        } }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_thermo_step_advance(ldg_math_phys_ctx_t *ctx, uint32_t region, double dt, uint32_t step_cunt)
{
    ldg_math_phys_thermo_hdr_t *hdr = 0x0;
    double *temp = 0x0;
    double *temp_next = 0x0;
    double *emission = 0x0;
    double *flux = 0x0;
    double *radiance = 0x0;
    double *hold_grid = 0x0;
    const uint16_t *mtl_grid = 0x0;
    const double *coeff = 0x0;
    double spacing = 0.0;
    double cell_vol = 0.0;
    double heat_cap = 0.0;
    double absorbed = 0.0;
    double escaped = 0.0;
    double emitted = 0.0;
    double convect_escaped = 0.0;
    double pwr_injected = 0.0;
    uint64_t pass_used = 0;
    uint64_t cell = 0;
    uint32_t step_idx = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_thermo_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(dt) || dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(step_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    hdr = ldg_math_phys_thermo_hdr_get(&ctx->region[region]);
    spacing = ctx->region[region].desc.spacing;
    cell_vol = spacing * spacing * spacing;
    temp = ldg_math_phys_thermo_temp_get(&ctx->region[region]);
    temp_next = ldg_math_phys_thermo_temp_next_get(&ctx->region[region]);
    emission = ldg_math_phys_thermo_emission_get(&ctx->region[region]);
    flux = ldg_math_phys_thermo_flux_get(&ctx->region[region]);
    radiance = ldg_math_phys_thermo_radiance_get(&ctx->region[region]);
    hold_grid = ldg_math_phys_thermo_hold_get(&ctx->region[region]);
    mtl_grid = ldg_math_phys_thermo_mtl_get(&ctx->region[region]);
    coeff = ldg_math_phys_thermo_coeff_get(&ctx->region[region]);
    for (step_idx = 0; step_idx < step_cunt; step_idx++)
    {
        err = ldg_math_phys_thermo_emission_build(emission, temp, mtl_grid, ldg_math_phys_thermo_coeff_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, spacing);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_field_em_radiance_solve(radiance, flux, emission, mtl_grid, ldg_math_phys_thermo_em_coeff_get(&ctx->region[region]), ldg_math_phys_thermo_scatter_get(&ctx->region[region]), ldg_math_phys_thermo_ordinate_get(&ctx->region[region]), LDG_MATH_PHYS_THERMO_ORD_CUNT, hdr->nx, hdr->ny, hdr->nz, spacing, LDG_MATH_PHYS_THERMO_RADIANCE_TOL, LDG_MATH_PHYS_THERMO_RADIANCE_PASS_MAX, &pass_used);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_field_em_radiance_conserved_get(radiance, emission, mtl_grid, ldg_math_phys_thermo_em_coeff_get(&ctx->region[region]), ldg_math_phys_thermo_ordinate_get(&ctx->region[region]), LDG_MATH_PHYS_THERMO_ORD_CUNT, hdr->nx, hdr->ny, hdr->nz, spacing, &absorbed, &escaped, &emitted);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        hdr->escaped_cum += escaped * dt;
        err = ldg_math_phys_thermo_xchg_advance(temp, flux, emission, mtl_grid, ldg_math_phys_thermo_coeff_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, spacing, dt);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_thermo_conduct_advance(temp_next, temp, mtl_grid, ldg_math_phys_thermo_coeff_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, spacing, dt);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        convect_escaped = 0.0;
        err = ldg_math_phys_thermo_convect_advance(temp_next, temp, mtl_grid, coeff, hdr->nx, hdr->ny, hdr->nz, spacing, dt, hdr->ambient, &convect_escaped);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        hdr->escaped_cum += convect_escaped;
        pwr_injected = 0.0;
        err = ldg_math_phys_thermo_pwr_advance(temp_next, mtl_grid, coeff, ldg_math_phys_thermo_pwr_get(&ctx->region[region]), hdr->cell_cunt, spacing, dt, &pwr_injected);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        hdr->injected_cum += pwr_injected;
        for (cell = 0; cell < hdr->cell_cunt; cell++) { if (hold_grid[cell] >= 0.0)
            {
                heat_cap = coeff[(uint64_t)mtl_grid[cell] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE + LDG_MATH_PHYS_THERMO_COEFF_RHO] * coeff[(uint64_t)mtl_grid[cell] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE + LDG_MATH_PHYS_THERMO_COEFF_CAP];
                hdr->injected_cum += heat_cap * (hold_grid[cell] - temp_next[cell]) * cell_vol;
                temp[cell] = hold_grid[cell];
            }
            else { temp[cell] = temp_next[cell]; } }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_thermo_sample_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, double *out_temp, double *out_flux)
{
    ldg_math_phys_thermo_hdr_t *hdr = 0x0;
    double spacing = 0.0;
    uint64_t cell_idx[3] = LDG_ARR_ZERO_INIT;
    uint64_t dim[3] = LDG_ARR_ZERO_INIT;
    uint64_t cell = 0;
    uint32_t axis = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_temp && !out_flux)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out_temp) { *out_temp = 0.0; }

    if (out_flux) { *out_flux = 0.0; }

    err = ldg_math_phys_thermo_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_thermo_hdr_get(&ctx->region[region]);
    spacing = ctx->region[region].desc.spacing;
    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(pos[axis]) || pos[axis] < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        cell_idx[axis] = (uint64_t)(pos[axis] / spacing);
        if (LDG_UNLIKELY(cell_idx[axis] >= dim[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
    }
    cell = (cell_idx[2] * hdr->ny + cell_idx[1]) * hdr->nx + cell_idx[0];
    if (out_temp) { *out_temp = ldg_math_phys_thermo_temp_get(&ctx->region[region])[cell]; }

    if (out_flux) { *out_flux = ldg_math_phys_thermo_flux_get(&ctx->region[region])[cell]; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_thermo_vol_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t vol_stride, double *out_temp, double *out_flux)
{
    ldg_math_phys_thermo_hdr_t *hdr = 0x0;
    const double *temp = 0x0;
    const double *flux = 0x0;
    uint64_t vol_dim[3] = LDG_ARR_ZERO_INIT;
    uint64_t cell = 0;
    uint64_t vcell = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_temp && !out_flux)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(vol_stride == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_thermo_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_thermo_hdr_get(&ctx->region[region]);
    temp = ldg_math_phys_thermo_temp_get(&ctx->region[region]);
    flux = ldg_math_phys_thermo_flux_get(&ctx->region[region]);
    vol_dim[0] = (hdr->nx + vol_stride - 1) / vol_stride;
    vol_dim[1] = (hdr->ny + vol_stride - 1) / vol_stride;
    vol_dim[2] = (hdr->nz + vol_stride - 1) / vol_stride;
    for (idx_z = 0; idx_z < vol_dim[2]; idx_z++) { for (idx_y = 0; idx_y < vol_dim[1]; idx_y++) { for (idx_x = 0; idx_x < vol_dim[0]; idx_x++)
            {
                cell = (idx_z * vol_stride * hdr->ny + idx_y * vol_stride) * hdr->nx + idx_x * vol_stride;
                vcell = (idx_z * vol_dim[1] + idx_y) * vol_dim[0] + idx_x;
                if (out_temp) { out_temp[vcell] = temp[cell]; }

                if (out_flux) { out_flux[vcell] = flux[cell]; }
            }
        } }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_thermo_slice_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_slice_desc_t *desc, double *out_temp, double *out_flux)
{
    ldg_math_phys_thermo_hdr_t *hdr = 0x0;
    const double *temp = 0x0;
    const double *flux = 0x0;
    double spacing = 0.0;
    double pos[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double lo_pos = 0.0;
    double hi_pos = 0.0;
    uint64_t cell_idx[3] = LDG_ARR_ZERO_INIT;
    uint64_t dim[3] = LDG_ARR_ZERO_INIT;
    uint64_t cell = 0;
    uint64_t span_cunt = 0;
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    uint32_t err = 0;
    uint32_t axis = 0;
    uint32_t pass = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_temp && !out_flux)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_thermo_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_thermo_hdr_get(&ctx->region[region]);
    spacing = ctx->region[region].desc.spacing;
    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    if (LDG_UNLIKELY(desc->axis_a >= LDG_MATH_PHYS_DIM || desc->axis_b >= LDG_MATH_PHYS_DIM || desc->axis_a == desc->axis_b)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->step_a) || desc->step_a <= 0.0 || !ldg_math_finite_is(desc->step_b) || desc->step_b <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->cunt_a == 0 || desc->cunt_b == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(desc->orig[axis]) || desc->orig[axis] < 0.0 || desc->orig[axis] >= (double)dim[axis] * spacing)) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; } }
    for (pass = 0; pass < 2; pass++)
    {
        axis = (pass == 0) ? desc->axis_a : desc->axis_b;
        span_cunt = (pass == 0) ? desc->cunt_a : desc->cunt_b;
        lo_pos = desc->orig[axis];
        hi_pos = lo_pos + (double)(span_cunt - 1) * ((pass == 0) ? desc->step_a : desc->step_b);
        if (LDG_UNLIKELY(!ldg_math_finite_is(hi_pos) || hi_pos >= (double)dim[axis] * spacing)) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
    }
    temp = ldg_math_phys_thermo_temp_get(&ctx->region[region]);
    flux = ldg_math_phys_thermo_flux_get(&ctx->region[region]);
    for (idx_b = 0; idx_b < desc->cunt_b; idx_b++)
    {
        for (idx_a = 0; idx_a < desc->cunt_a; idx_a++)
        {
            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { pos[axis] = desc->orig[axis]; }
            pos[desc->axis_a] = desc->orig[desc->axis_a] + (double)idx_a * desc->step_a;
            pos[desc->axis_b] = desc->orig[desc->axis_b] + (double)idx_b * desc->step_b;
            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { cell_idx[axis] = (uint64_t)(pos[axis] / spacing); }
            cell = (cell_idx[2] * hdr->ny + cell_idx[1]) * hdr->nx + cell_idx[0];
            if (out_temp) { out_temp[idx_b * desc->cunt_a + idx_a] = temp[cell]; }

            if (out_flux) { out_flux[idx_b * desc->cunt_a + idx_a] = flux[cell]; }
        }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_thermo_conserved_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_heat, double *out_escaped_cum, double *out_injected_cum)
{
    ldg_math_phys_thermo_hdr_t *hdr = 0x0;
    const double *temp = 0x0;
    const double *coeff = 0x0;
    const uint16_t *mtl_grid = 0x0;
    double spacing = 0.0;
    double heat_cap = 0.0;
    uint64_t cell = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_heat)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_escaped_cum)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_heat = 0.0;
    *out_escaped_cum = 0.0;
    if (out_injected_cum) { *out_injected_cum = 0.0; }

    err = ldg_math_phys_thermo_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_thermo_hdr_get(&ctx->region[region]);
    spacing = ctx->region[region].desc.spacing;
    temp = ldg_math_phys_thermo_temp_get(&ctx->region[region]);
    coeff = ldg_math_phys_thermo_coeff_get(&ctx->region[region]);
    mtl_grid = ldg_math_phys_thermo_mtl_get(&ctx->region[region]);
    for (cell = 0; cell < hdr->cell_cunt; cell++)
    {
        heat_cap = coeff[(uint64_t)mtl_grid[cell] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE + LDG_MATH_PHYS_THERMO_COEFF_RHO] * coeff[(uint64_t)mtl_grid[cell] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE + LDG_MATH_PHYS_THERMO_COEFF_CAP];
        *out_heat += heat_cap * temp[cell] * spacing * spacing * spacing;
    }
    *out_escaped_cum = hdr->escaped_cum;
    if (out_injected_cum) { *out_injected_cum = hdr->injected_cum; }

    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_water_region_ck(const ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_WATER)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_water_box_clip(const ldg_math_phys_water_hdr_t *hdr, double spacing, const double *lo, const double *hi, uint64_t *box_lo, uint64_t *box_hi)
{
    uint64_t dim[3] = LDG_ARR_ZERO_INIT;
    uint32_t axis = 0;
    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(lo[axis]) || !ldg_math_finite_is(hi[axis]) || lo[axis] > hi[axis])) { return LDG_ERR_FUNC_ARG_INVALID; }

        box_lo[axis] = (uint64_t)((lo[axis] < 0.0) ? 0.0 : (lo[axis] / spacing + 0.5));
        box_hi[axis] = (uint64_t)(hi[axis] / spacing + 0.5);
        if (box_hi[axis] >= dim[axis]) { box_hi[axis] = dim[axis] - 1; }

        if (LDG_UNLIKELY(box_lo[axis] > box_hi[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_water_solid_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi)
{
    ldg_math_phys_water_hdr_t *hdr = 0x0;
    uint16_t *mask = 0x0;
    uint64_t box_lo[3] = LDG_ARR_ZERO_INIT;
    uint64_t box_hi[3] = LDG_ARR_ZERO_INIT;
    uint64_t pos_x = 0;
    uint64_t pos_y = 0;
    uint64_t pos_z = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!lo || !hi)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_water_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_water_hdr_get(&ctx->region[region]);
    err = ldg_math_phys_water_box_clip(hdr, ctx->region[region].desc.spacing, lo, hi, box_lo, box_hi);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    mask = ldg_math_phys_water_mask_get(&ctx->region[region]);
    for (pos_z = box_lo[2]; pos_z <= box_hi[2]; pos_z++) { for (pos_y = box_lo[1]; pos_y <= box_hi[1]; pos_y++) { for (pos_x = box_lo[0]; pos_x <= box_hi[0]; pos_x++) { mask[(pos_z * hdr->ny + pos_y) * hdr->nx + pos_x] = LDG_MATH_PHYS_FLUID_CELL_SOLID; } } }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_water_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi)
{
    ldg_math_phys_water_hdr_t *hdr = 0x0;
    double *particle = 0x0;
    const uint16_t *mask = 0x0;
    double spacing = 0.0;
    uint64_t box_lo[3] = LDG_ARR_ZERO_INIT;
    uint64_t box_hi[3] = LDG_ARR_ZERO_INIT;
    uint64_t pos_x = 0;
    uint64_t pos_y = 0;
    uint64_t pos_z = 0;
    uint64_t sub = 0;
    uint64_t jitter_seed = 0;
    uint64_t cell = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!lo || !hi)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_water_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_water_hdr_get(&ctx->region[region]);
    spacing = ctx->region[region].desc.spacing;
    err = ldg_math_phys_water_box_clip(hdr, spacing, lo, hi, box_lo, box_hi);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    particle = ldg_math_phys_water_particle_get(&ctx->region[region]);
    mask = ldg_math_phys_water_mask_get(&ctx->region[region]);
    for (pos_z = box_lo[2]; pos_z <= box_hi[2]; pos_z++)
    {
        for (pos_y = box_lo[1]; pos_y <= box_hi[1]; pos_y++)
        {
            for (pos_x = box_lo[0]; pos_x <= box_hi[0]; pos_x++)
            {
                cell = (pos_z * hdr->ny + pos_y) * hdr->nx + pos_x;
                if (mask[cell] == LDG_MATH_PHYS_FLUID_CELL_SOLID) { continue; }

                for (sub = 0; sub < LDG_MATH_PHYS_WATER_PARTICLE_PER_CELL; sub++)
                {
                    if (LDG_UNLIKELY(hdr->particle_cunt >= hdr->particle_cap)) { return LDG_ERR_FULL; }

                    jitter_seed = (hdr->particle_cunt * 2654435761ULL + sub * 40503ULL) & 0xFFFFFFFFULL;
                    particle[hdr->particle_cunt * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE] = ((double)pos_x + 0.25 + 0.5 * (double)(sub & 1) + 0.2 * ((double)((jitter_seed * 48271ULL) & 1023ULL) / 1023.0 - 0.5)) * spacing;
                    particle[hdr->particle_cunt * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE + 1] = ((double)pos_y + 0.25 + 0.5 * (double)((sub >> 1) & 1) + 0.2 * ((double)((jitter_seed * 16807ULL) & 1023ULL) / 1023.0 - 0.5)) * spacing;
                    particle[hdr->particle_cunt * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE + 2] = ((double)pos_z + 0.25 + 0.5 * (double)((sub >> 2) & 1) + 0.2 * ((double)((jitter_seed * 69621ULL) & 1023ULL) / 1023.0 - 0.5)) * spacing;
                    particle[hdr->particle_cunt * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE + 3] = 0.0;
                    particle[hdr->particle_cunt * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE + 4] = 0.0;
                    particle[hdr->particle_cunt * LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE + 5] = 0.0;
                    hdr->particle_cunt++;
                }
            }
        }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_water_step_advance(ldg_math_phys_ctx_t *ctx, uint32_t region, double dt, uint32_t step_cunt)
{
    ldg_math_phys_water_hdr_t *hdr = 0x0;
    double *vel_x = 0x0;
    double *vel_y = 0x0;
    double *vel_z = 0x0;
    double *vel_x_old = 0x0;
    double *vel_y_old = 0x0;
    double *vel_z_old = 0x0;
    double spacing = 0.0;
    uint64_t face_x_cunt = 0;
    uint64_t face_y_cunt = 0;
    uint64_t face_z_cunt = 0;
    uint32_t step_idx = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_water_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(dt) || dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(step_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    hdr = ldg_math_phys_water_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->particle_cunt == 0)) { return LDG_ERR_EMPTY; }

    spacing = ctx->region[region].desc.spacing;
    face_x_cunt = (hdr->nx + 1) * hdr->ny * hdr->nz;
    face_y_cunt = hdr->nx * (hdr->ny + 1) * hdr->nz;
    face_z_cunt = hdr->nx * hdr->ny * (hdr->nz + 1);
    vel_x = ldg_math_phys_water_vel_get(&ctx->region[region], 0);
    vel_y = ldg_math_phys_water_vel_get(&ctx->region[region], 1);
    vel_z = ldg_math_phys_water_vel_get(&ctx->region[region], 2);
    vel_x_old = ldg_math_phys_water_vel_get(&ctx->region[region], 3);
    vel_y_old = ldg_math_phys_water_vel_get(&ctx->region[region], 4);
    vel_z_old = ldg_math_phys_water_vel_get(&ctx->region[region], 5);
    for (step_idx = 0; step_idx < step_cunt; step_idx++)
    {
        err = ldg_math_phys_fluid_p2g_xfer(ldg_math_phys_water_particle_get(&ctx->region[region]), hdr->particle_cunt, vel_x, vel_y, vel_z, ldg_math_phys_water_vel_get(&ctx->region[region], 6), ldg_math_phys_water_vel_get(&ctx->region[region], 7), ldg_math_phys_water_vel_get(&ctx->region[region], 8), ldg_math_phys_water_mask_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, spacing);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        ldg_mem_cpy(vel_x_old, vel_x, face_x_cunt * sizeof(double));
        ldg_mem_cpy(vel_y_old, vel_y, face_y_cunt * sizeof(double));
        ldg_mem_cpy(vel_z_old, vel_z, face_z_cunt * sizeof(double));
        err = ldg_math_phys_fluid_grid_advance(vel_x, vel_y, vel_z, ldg_math_phys_water_pressure_get(&ctx->region[region]), ldg_math_phys_water_divergence_get(&ctx->region[region]), ldg_math_phys_water_mask_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, spacing, dt, LDG_MATH_PHYS_WATER_DENSITY, LDG_MATH_PHYS_WATER_GRAVITY, hdr->iter_cunt);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_fluid_g2p_advance(ldg_math_phys_water_particle_get(&ctx->region[region]), hdr->particle_cunt, vel_x, vel_y, vel_z, vel_x_old, vel_y_old, vel_z_old, ldg_math_phys_water_mask_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, spacing, dt, hdr->flip, 0, hdr->particle_cunt);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_water_sample_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, uint32_t *out_state, double *out_vel)
{
    ldg_math_phys_water_hdr_t *hdr = 0x0;
    double spacing = 0.0;
    uint64_t cell_idx[3] = LDG_ARR_ZERO_INIT;
    uint64_t dim[3] = LDG_ARR_ZERO_INIT;
    uint64_t cell = 0;
    uint64_t face_x_row = 0;
    uint32_t axis = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_state && !out_vel)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out_state) { *out_state = 0; }

    if (out_vel)
    {
        out_vel[0] = 0.0;
        out_vel[1] = 0.0;
        out_vel[2] = 0.0;
    }

    err = ldg_math_phys_water_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_water_hdr_get(&ctx->region[region]);
    spacing = ctx->region[region].desc.spacing;
    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(pos[axis]) || pos[axis] < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        cell_idx[axis] = (uint64_t)(pos[axis] / spacing);
        if (LDG_UNLIKELY(cell_idx[axis] >= dim[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
    }
    cell = (cell_idx[2] * hdr->ny + cell_idx[1]) * hdr->nx + cell_idx[0];
    if (out_state) { *out_state = ldg_math_phys_water_mask_get(&ctx->region[region])[cell]; }

    if (out_vel)
    {
        face_x_row = hdr->nx + 1;
        out_vel[0] = 0.5 * (ldg_math_phys_water_vel_get(&ctx->region[region], 0)[(cell_idx[2] * hdr->ny + cell_idx[1]) * face_x_row + cell_idx[0]] + ldg_math_phys_water_vel_get(&ctx->region[region], 0)[(cell_idx[2] * hdr->ny + cell_idx[1]) * face_x_row + cell_idx[0] + 1]);
        out_vel[1] = 0.5 * (ldg_math_phys_water_vel_get(&ctx->region[region], 1)[cell_idx[2] * hdr->nx * (hdr->ny + 1) + cell_idx[1] * hdr->nx + cell_idx[0]] + ldg_math_phys_water_vel_get(&ctx->region[region], 1)[cell_idx[2] * hdr->nx * (hdr->ny + 1) + (cell_idx[1] + 1) * hdr->nx + cell_idx[0]]);
        out_vel[2] = 0.5 * (ldg_math_phys_water_vel_get(&ctx->region[region], 2)[(cell_idx[2] * hdr->ny + cell_idx[1]) * hdr->nx + cell_idx[0]] + ldg_math_phys_water_vel_get(&ctx->region[region], 2)[((cell_idx[2] + 1) * hdr->ny + cell_idx[1]) * hdr->nx + cell_idx[0]]);
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_water_conserved_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_mass, double *out_crowd_max)
{
    ldg_math_phys_water_hdr_t *hdr = 0x0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_mass)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_crowd_max)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_mass = 0.0;
    *out_crowd_max = 0.0;
    err = ldg_math_phys_water_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_water_hdr_get(&ctx->region[region]);
    if (hdr->particle_cunt == 0) { return LDG_ERR_AOK; }

    return ldg_math_phys_fluid_conserved_get(ldg_math_phys_water_particle_get(&ctx->region[region]), hdr->particle_cunt, ldg_math_phys_water_scratch_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, ctx->region[region].desc.spacing, LDG_MATH_PHYS_WATER_DENSITY, out_mass, out_crowd_max);
}

LDG_EXPORT uint32_t ldg_math_phys_region_grid_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint64_t *out)
{
    const ldg_math_phys_region_t *region_ptr = 0x0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { out[axis] = 0; }
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    region_ptr = &ctx->region[region];
    if (LDG_UNLIKELY(region_ptr->desc.tier != LDG_MATH_PHYS_TIER_FIELD)) { return LDG_ERR_MATH_PHYS_TIER_INVALID; }

    if (LDG_UNLIKELY(region_ptr->field == 0x0)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (region_ptr->desc.field_kind == LDG_MATH_PHYS_FIELD_KIND_EM)
    {
        out[0] = ldg_math_phys_em_hdr_get(region_ptr)->nx;
        out[1] = ldg_math_phys_em_hdr_get(region_ptr)->ny;
        out[2] = ldg_math_phys_em_hdr_get(region_ptr)->nz;
        return LDG_ERR_AOK;
    }

    if (region_ptr->desc.field_kind == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC)
    {
        out[0] = ldg_math_phys_acoustic_hdr_get(region_ptr)->nx;
        out[1] = ldg_math_phys_acoustic_hdr_get(region_ptr)->ny;
        out[2] = ldg_math_phys_acoustic_hdr_get(region_ptr)->nz;
        return LDG_ERR_AOK;
    }

    if (region_ptr->desc.field_kind == LDG_MATH_PHYS_FIELD_KIND_THERMO)
    {
        out[0] = ldg_math_phys_thermo_hdr_get(region_ptr)->nx;
        out[1] = ldg_math_phys_thermo_hdr_get(region_ptr)->ny;
        out[2] = ldg_math_phys_thermo_hdr_get(region_ptr)->nz;
        return LDG_ERR_AOK;
    }

    if (region_ptr->desc.field_kind == LDG_MATH_PHYS_FIELD_KIND_WATER)
    {
        out[0] = ldg_math_phys_water_hdr_get(region_ptr)->nx;
        out[1] = ldg_math_phys_water_hdr_get(region_ptr)->ny;
        out[2] = ldg_math_phys_water_hdr_get(region_ptr)->nz;
        return LDG_ERR_AOK;
    }

    out[0] = ldg_math_phys_field_hdr_get(region_ptr)->nx;
    out[1] = ldg_math_phys_field_hdr_get(region_ptr)->ny;
    out[2] = ldg_math_phys_field_hdr_get(region_ptr)->nz;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_water_fraction_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, double *out)
{
    ldg_math_phys_water_hdr_t *hdr = 0x0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_water_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_water_hdr_get(&ctx->region[region]);
    return ldg_math_phys_fluid_fraction_build(ldg_math_phys_water_particle_get(&ctx->region[region]), hdr->particle_cunt, ldg_math_phys_water_mask_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, ctx->region[region].desc.spacing, out);
}

LDG_EXPORT uint32_t ldg_math_phys_water_optic_set(ldg_math_phys_ctx_t *ctx, uint32_t region, double refract, const double *absorb, uint32_t bin_cunt)
{
    ldg_math_phys_water_hdr_t *hdr = 0x0;
    uint32_t bin = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!absorb)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_water_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(refract) || refract < 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(bin_cunt == 0 || bin_cunt > LDG_MATH_PHYS_WATER_OPTIC_BIN_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (bin = 0; bin < bin_cunt; bin++) { if (LDG_UNLIKELY(!ldg_math_finite_is(absorb[bin]) || absorb[bin] < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; } }
    hdr = ldg_math_phys_water_hdr_get(&ctx->region[region]);
    hdr->refract = refract;
    hdr->optic_bin_cunt = bin_cunt;
    for (bin = 0; bin < bin_cunt; bin++) { hdr->absorb[bin] = absorb[bin]; }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_water_optic_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_refract, double *out_absorb, uint32_t *out_bin_cunt)
{
    ldg_math_phys_water_hdr_t *hdr = 0x0;
    uint64_t bin = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_refract && !out_absorb && !out_bin_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out_refract) { *out_refract = 0.0; }

    if (out_bin_cunt) { *out_bin_cunt = 0; }

    err = ldg_math_phys_water_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_water_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->optic_bin_cunt == 0)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (out_refract) { *out_refract = hdr->refract; }

    if (out_bin_cunt) { *out_bin_cunt = (uint32_t)hdr->optic_bin_cunt; }

    if (out_absorb) { for (bin = 0; bin < hdr->optic_bin_cunt; bin++) { out_absorb[bin] = hdr->absorb[bin]; } }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_water_caustic_get(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *light_dir, double *out, double *out_escaped)
{
    ldg_math_phys_water_hdr_t *hdr = 0x0;
    double *fraction = 0x0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!light_dir)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out_escaped) { *out_escaped = 0.0; }

    err = ldg_math_phys_water_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_water_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->optic_bin_cunt == 0)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    fraction = ldg_math_phys_water_divergence_get(&ctx->region[region]);
    err = ldg_math_phys_fluid_fraction_build(ldg_math_phys_water_particle_get(&ctx->region[region]), hdr->particle_cunt, ldg_math_phys_water_mask_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, ctx->region[region].desc.spacing, fraction);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    return ldg_math_phys_fluid_caustic_build(fraction, hdr->nx, hdr->ny, hdr->nz, ctx->region[region].desc.spacing, light_dir, hdr->refract, out, out_escaped);
}

LDG_EXPORT uint32_t ldg_math_phys_region_accel_set(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *accel)
{
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!accel)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(accel[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; } }
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { ctx->region[region].accel[axis] = accel[axis]; }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_pacing_set(ldg_math_phys_ctx_t *ctx, double ratio)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(ratio) || ratio < 0.0)) { return LDG_ERR_MATH_PHYS_PACING_INVALID; }

    ctx->pacing = ratio;
    if (ctx->history != 0x0) { ldg_math_phys_history_hdr_get(ctx)->anchor_set = 0; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_time_get(const ldg_math_phys_ctx_t *ctx, ldg_uint128_t *out)
{
    ldg_uint128_t zero_stamp = LDG_STRUCT_ZERO_INIT;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = zero_stamp;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    *out = ctx->clock;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_body_create(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t kind, const ldg_math_phys_body_desc_t *desc, uint64_t *out)
{
    ldg_math_phys_body_val_t body_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_tag_t tag_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_region_t *region_ptr = 0x0;
    double mass_val = 0.0;
    int64_t charge_quanta = 0;
    int64_t lepton_val = 0;
    int64_t baryon_val = 0;
    uint32_t err = 0;
    uint32_t aggregate_idx = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    region_ptr = &ctx->region[region];
    if (kind < LDG_MATH_PHYS_PARTICLE_CUNT)
    {
        mass_val = ctx->particle[kind].mass;
        ldg_math_phys_particle_tag_get(kind, &charge_quanta, &lepton_val, &baryon_val);
    }
    else if ((kind & LDG_MATH_PHYS_AGGREGATE_ID_BASE) != 0)
    {
        aggregate_idx = kind & ~LDG_MATH_PHYS_AGGREGATE_ID_BASE;
        if (LDG_UNLIKELY(aggregate_idx >= ctx->conf.aggregate_cap || !ctx->aggregate[aggregate_idx].used)) { return LDG_ERR_MATH_PHYS_KIND_INVALID; }

        mass_val = desc->mass;
        charge_quanta = desc->charge;
        lepton_val = desc->lepton;
        baryon_val = desc->baryon;
    }
    else { return LDG_ERR_MATH_PHYS_KIND_INVALID; }

    err = ldg_math_phys_body_convert(region_ptr, desc, mass_val, charge_quanta, &body_val);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    tag_val.lepton = lepton_val;
    tag_val.baryon = baryon_val;
    return ldg_math_phys_body_wr(ctx, region, kind, &body_val, &tag_val, out);
}

LDG_EXPORT uint32_t ldg_math_phys_body_destroy(ldg_math_phys_ctx_t *ctx, uint64_t body)
{
    ldg_math_phys_body_val_t body_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_region_t *region_ptr = 0x0;
    ldg_math_phys_conserved_t *conserved_ptr = 0x0;
    void *comp_ptr = 0x0;
    uint32_t region = 0;
    uint32_t err = 0;
    uint32_t rollback_err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(&ctx->world->store, body) != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region = *(uint32_t *)comp_ptr;
    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    region_ptr = &ctx->region[region];
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MASS, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    body_val.mass_c = *(ldg_math_phys_conserved_t *)comp_ptr;
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_ENERGY, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    body_val.energy_c = *(ldg_math_phys_conserved_t *)comp_ptr;
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MOMENTUM, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { body_val.momentum_c[axis] = conserved_ptr[axis]; }
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_ANGULAR_MOMENTUM, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { body_val.angular_c[axis] = conserved_ptr[axis]; }
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_CHARGE, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    body_val.charge_c = *(ldg_math_phys_conserved_t *)comp_ptr;
    err = ldg_math_phys_region_total_add(region_ptr, &body_val, 1);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_ecs_ctx_entity_destroy(ctx->world, body);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        rollback_err = ldg_math_phys_region_total_add(region_ptr, &body_val, 0);
        if (rollback_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = rollback_err; }

        return err;
    }

    if (region_ptr->body_cunt > 0) { region_ptr->body_cunt--; }

    if (ctx->body_cunt > 0) { ctx->body_cunt--; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint64_t ldg_math_phys_body_cunt_get(const ldg_math_phys_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return UINT64_MAX; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return UINT64_MAX; }

    return ctx->body_cunt;
}

LDG_EXPORT uint32_t ldg_math_phys_conserved_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t quantity, ldg_uint256_t *magnitude, uint8_t *sign)
{
    ldg_math_phys_conserved_t sum_total = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t sum_val = LDG_STRUCT_ZERO_INIT;
    ldg_uint256_t zero_magnitude = LDG_STRUCT_ZERO_INIT;
    int64_t shared_exponent = 0;
    uint32_t err = 0;
    uint32_t idx = 0;
    uint8_t first_found = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!magnitude)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!sign)) { return LDG_ERR_FUNC_ARG_NULL; }

    *magnitude = zero_magnitude;
    *sign = 0;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(quantity >= LDG_MATH_PHYS_QUANTITY_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (region == LDG_MATH_PHYS_REGION_WORLD)
    {
        for (idx = 0; idx < ctx->conf.region_cap; idx++)
        {
            if (!ctx->region[idx].used) { continue; }

            if (!first_found)
            {
                shared_exponent = ldg_math_phys_region_exponent_get(&ctx->region[idx], quantity);
                first_found = 1;
            }

            if (LDG_UNLIKELY(ldg_math_phys_region_exponent_get(&ctx->region[idx], quantity) != shared_exponent)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

            err = ldg_math_phys_conserved_add(&sum_total, &ctx->region[idx].total[quantity], &sum_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            sum_total = sum_val;
        }
        *magnitude = sum_total.magnitude;
        *sign = sum_total.sign;
        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    *magnitude = ctx->region[region].total[quantity].magnitude;
    *sign = ctx->region[region].total[quantity].sign;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_constraint_add(ldg_math_phys_ctx_t *ctx, uint64_t body_a, uint64_t body_b, double rest_len, double compliance, double damping, uint64_t *out)
{
    void *comp_ptr = 0x0;
    double mass_val = 0.0;
    uint32_t region_a = 0;
    uint32_t region_b = 0;
    uint32_t err = 0;
    uint32_t idx = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT64_MAX;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(body_a == body_b)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(rest_len) || rest_len < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(compliance) || compliance <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(damping) || damping < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(&ctx->world->store, body_a) != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(&ctx->world->store, body_b) != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    err = ldg_ecs_ctx_comp_get(ctx->world, body_a, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_a = *(uint32_t *)comp_ptr;
    err = ldg_ecs_ctx_comp_get(ctx->world, body_b, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_b = *(uint32_t *)comp_ptr;
    if (LDG_UNLIKELY(region_a != region_b)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    if (LDG_UNLIKELY(region_a >= ctx->conf.region_cap || !ctx->region[region_a].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region_a].desc.tier != LDG_MATH_PHYS_TIER_PARTICLE)) { return LDG_ERR_MATH_PHYS_TIER_INVALID; }

    err = ldg_ecs_ctx_comp_get(ctx->world, body_a, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MASS, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_conserved_to_double_convert((ldg_math_phys_conserved_t *)comp_ptr, ldg_math_phys_region_exponent_get(&ctx->region[region_a], LDG_MATH_PHYS_QUANTITY_MASS), &mass_val);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(mass_val <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_ecs_ctx_comp_get(ctx->world, body_b, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MASS, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_conserved_to_double_convert((ldg_math_phys_conserved_t *)comp_ptr, ldg_math_phys_region_exponent_get(&ctx->region[region_a], LDG_MATH_PHYS_QUANTITY_MASS), &mass_val);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(mass_val <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < ctx->conf.constraint_cap; idx++)
    {
        if (!ctx->constraint[idx].used)
        {
            ctx->constraint[idx].body_a = body_a;
            ctx->constraint[idx].body_b = body_b;
            ctx->constraint[idx].rest_len = rest_len;
            ctx->constraint[idx].compliance = compliance;
            ctx->constraint[idx].damping = damping;
            ctx->constraint[idx].used = 1;
            *out = ((uint64_t)ctx->constraint[idx].generation << 32) | idx;
            return LDG_ERR_AOK;
        }
    }
    return LDG_ERR_FULL;
}

LDG_EXPORT uint32_t ldg_math_phys_constraint_remove(ldg_math_phys_ctx_t *ctx, uint64_t constraint)
{
    uint32_t idx = 0;
    uint32_t generation = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    idx = (uint32_t)(constraint & 0xFFFFFFFFU);
    generation = (uint32_t)(constraint >> 32);
    if (LDG_UNLIKELY(idx >= ctx->conf.constraint_cap)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(!ctx->constraint[idx].used || ctx->constraint[idx].generation != generation)) { return LDG_ERR_NOT_FOUND; }

    ctx->constraint[idx].used = 0;
    ctx->constraint[idx].generation++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_collider_add(ldg_math_phys_ctx_t *ctx, const ldg_math_phys_collider_desc_t *desc, uint64_t *out)
{
    double norm_len = 0.0;
    uint32_t idx = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT64_MAX;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->restitution) || desc->restitution < 0.0 || desc->restitution > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (desc->shape == LDG_MATH_PHYS_COLLIDER_PLANE)
    {
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(desc->normal[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; } }
        norm_len = desc->normal[0] * desc->normal[0] + desc->normal[1] * desc->normal[1] + desc->normal[2] * desc->normal[2];
        if (LDG_UNLIKELY(norm_len <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->offset))) { return LDG_ERR_FUNC_ARG_INVALID; }
    }
    else if (desc->shape == LDG_MATH_PHYS_COLLIDER_SPHERE)
    {
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(desc->center[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; } }
        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->radius) || desc->radius <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }
    }
    else if (desc->shape == LDG_MATH_PHYS_COLLIDER_BOX || desc->shape == LDG_MATH_PHYS_COLLIDER_CAPSULE) { return LDG_ERR_UNSUPPORTED; }
    else { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < ctx->conf.collider_cap; idx++) { if (!ctx->collider[idx].used)
        {
            ctx->collider[idx].desc = *desc;
            ctx->collider[idx].used = 1;
            *out = ((uint64_t)ctx->collider[idx].generation << 32) | idx;
            return LDG_ERR_AOK;
        }
    }
    return LDG_ERR_FULL;
}

LDG_EXPORT uint32_t ldg_math_phys_collider_remove(ldg_math_phys_ctx_t *ctx, uint64_t collider)
{
    uint32_t idx = 0;
    uint32_t generation = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    idx = (uint32_t)(collider & 0xFFFFFFFFU);
    generation = (uint32_t)(collider >> 32);
    if (LDG_UNLIKELY(idx >= ctx->conf.collider_cap)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(!ctx->collider[idx].used || ctx->collider[idx].generation != generation)) { return LDG_ERR_NOT_FOUND; }

    ctx->collider[idx].used = 0;
    ctx->collider[idx].generation++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_body_sample_get(const ldg_math_phys_ctx_t *ctx, uint64_t body, const ldg_uint128_t *time, ldg_math_phys_body_state_t *out)
{
    ldg_math_phys_body_state_t zero_state = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_history_slot_t *slot_a = 0x0;
    ldg_math_phys_history_slot_t *slot_b = 0x0;
    ldg_math_phys_conserved_t *conserved_ptr = 0x0;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    void *comp_ptr = 0x0;
    double ratio = 0.0;
    int64_t charge_val = 0;
    uint64_t slot_idx = 0;
    uint64_t generation = 0;
    uint32_t err = 0;
    uint32_t frame_a = 0;
    uint32_t frame_b = 0;
    uint32_t region = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!time)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = zero_state;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(&ctx->world->store, body) != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    err = ldg_math_phys_history_frame_find(ctx, time, &frame_a, &frame_b, &ratio);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    slot_idx = ldg_ecs_entity_idx_get(body);
    generation = ldg_ecs_entity_generation_get(body);
    slot_a = ldg_math_phys_history_slot_get(ctx, frame_a) + slot_idx;
    slot_b = ldg_math_phys_history_slot_get(ctx, frame_b) + slot_idx;
    if (LDG_UNLIKELY(!slot_a->used || slot_a->generation != generation)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(!slot_b->used || slot_b->generation != generation)) { return LDG_ERR_NOT_FOUND; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        out->pos[axis] = slot_a->pos[axis] + (slot_b->pos[axis] - slot_a->pos[axis]) * ratio;
        out->velocity[axis] = slot_a->velocity[axis] + (slot_b->velocity[axis] - slot_a->velocity[axis]) * ratio;
    }
    err = ldg_math_quaternion_nlerp_compute(slot_a->orientation, slot_b->orientation, ratio, out->orientation);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_SPIN, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { out->spin[axis] = ((double *)comp_ptr)[axis]; }
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_INERTIA, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    out->inertia = *(double *)comp_ptr;
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region = *(uint32_t *)comp_ptr;
    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    region_ptr = &ctx->region[region];
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MASS, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_conserved_to_double_convert((ldg_math_phys_conserved_t *)comp_ptr, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_MASS), &out->mass);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { out->momentum[axis] = out->mass * out->velocity[axis]; }
    out->energy = 0.5 * out->mass * (out->velocity[0] * out->velocity[0] + out->velocity[1] * out->velocity[1] + out->velocity[2] * out->velocity[2]) + 0.5 * out->inertia * (out->spin[0] * out->spin[0] + out->spin[1] * out->spin[1] + out->spin[2] * out->spin[2]);
    out->angular_momentum[0] = out->pos[1] * out->momentum[2] - out->pos[2] * out->momentum[1] + out->inertia * out->spin[0];
    out->angular_momentum[1] = out->pos[2] * out->momentum[0] - out->pos[0] * out->momentum[2] + out->inertia * out->spin[1];
    out->angular_momentum[2] = out->pos[0] * out->momentum[1] - out->pos[1] * out->momentum[0] + out->inertia * out->spin[2];
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_CHARGE, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    charge_val = (int64_t)conserved_ptr->magnitude.limb[0];
    if (conserved_ptr->sign) { charge_val = 0 - charge_val; }

    out->charge = charge_val;
    out->tier = region_ptr->desc.tier;
    out->region = region;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_body_bulk_sample_get(const ldg_math_phys_ctx_t *ctx, const uint64_t *body, uint64_t cunt, const ldg_uint128_t *time, ldg_math_phys_body_state_t *out)
{
    uint64_t idx = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!body)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!time)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    for (idx = 0; idx < cunt; idx++)
    {
        err = ldg_math_phys_body_sample_get(ctx, body[idx], time, &out[idx]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_src_add(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, double amount)
{
    ldg_math_phys_field_hdr_t *hdr = 0x0;
    ldg_math_phys_acoustic_hdr_t *acoustic_hdr = 0x0;
    ldg_math_phys_thermo_hdr_t *thermo_hdr = 0x0;
    ldg_math_phys_region_t *region_ptr = 0x0;
    double *cur_buff = 0x0;
    double *temp_grid = 0x0;
    const double *hold_grid = 0x0;
    const double *coeff_row = 0x0;
    const uint16_t *mtl_grid = 0x0;
    double spacing = 0.0;
    double cell_heat_cap = 0.0;
    double temp_new = 0.0;
    uint64_t cell_idx[3] = LDG_ARR_ZERO_INIT;
    uint64_t dim[3] = LDG_ARR_ZERO_INIT;
    uint64_t cell = 0;
    uint32_t err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    region_ptr = &ctx->region[region];
    if (LDG_UNLIKELY(region_ptr->desc.tier != LDG_MATH_PHYS_TIER_FIELD)) { return LDG_ERR_MATH_PHYS_TIER_INVALID; }

    if (LDG_UNLIKELY(region_ptr->desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_SCALAR && region_ptr->desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC && region_ptr->desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_THERMO)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(region_ptr->field == 0x0)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(pos[axis]) || pos[axis] < 0.0 || pos[axis] >= region_ptr->desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; } }
    if (LDG_UNLIKELY(!ldg_math_finite_is(amount))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (region_ptr->desc.field_kind == LDG_MATH_PHYS_FIELD_KIND_THERMO)
    {
        thermo_hdr = ldg_math_phys_thermo_hdr_get(region_ptr);
        spacing = region_ptr->desc.spacing;
        dim[0] = thermo_hdr->nx;
        dim[1] = thermo_hdr->ny;
        dim[2] = thermo_hdr->nz;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            cell_idx[axis] = (uint64_t)(pos[axis] / spacing);
            if (LDG_UNLIKELY(cell_idx[axis] >= dim[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
        }
        cell = (cell_idx[2] * thermo_hdr->ny + cell_idx[1]) * thermo_hdr->nx + cell_idx[0];
        mtl_grid = ldg_math_phys_thermo_mtl_get(region_ptr);
        if (LDG_UNLIKELY(mtl_grid[cell] == 0)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

        hold_grid = ldg_math_phys_thermo_hold_get(region_ptr);
        if (LDG_UNLIKELY(hold_grid[cell] >= 0.0)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

        coeff_row = ldg_math_phys_thermo_coeff_get(region_ptr) + (uint64_t)mtl_grid[cell] * LDG_MATH_PHYS_THERMO_COEFF_STRIDE;
        cell_heat_cap = coeff_row[LDG_MATH_PHYS_THERMO_COEFF_RHO] * coeff_row[LDG_MATH_PHYS_THERMO_COEFF_CAP] * spacing * spacing * spacing;
        temp_grid = ldg_math_phys_thermo_temp_get(region_ptr);
        temp_new = temp_grid[cell] + amount / cell_heat_cap;
        if (LDG_UNLIKELY(temp_new < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        temp_grid[cell] = temp_new;
        thermo_hdr->injected_cum += amount;
        return LDG_ERR_AOK;
    }

    if (region_ptr->desc.field_kind == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC)
    {
        acoustic_hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
        if (LDG_UNLIKELY(acoustic_hdr->mtl_cunt == 0)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

        cur_buff = ldg_math_phys_acoustic_drive_get(region_ptr);
        if (acoustic_hdr->nz == 1) { err = ldg_math_phys_field_2d_src_add(cur_buff, acoustic_hdr->nx, acoustic_hdr->ny, region_ptr->desc.spacing, pos[0], pos[1], amount); }
        else { err = ldg_math_phys_field_3d_src_add(cur_buff, acoustic_hdr->nx, acoustic_hdr->ny, acoustic_hdr->nz, region_ptr->desc.spacing, pos[0], pos[1], pos[2], amount); }

        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

        return LDG_ERR_AOK;
    }

    hdr = ldg_math_phys_field_hdr_get(region_ptr);
    cur_buff = ldg_math_phys_field_buff_get(region_ptr, hdr->cur_idx);
    if (hdr->nz == 1) { err = ldg_math_phys_field_2d_src_add(cur_buff, hdr->nx, hdr->ny, region_ptr->desc.spacing, pos[0], pos[1], amount); }
    else { err = ldg_math_phys_field_3d_src_add(cur_buff, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, pos[0], pos[1], pos[2], amount); }

    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_em_region_ck(const ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.tier != LDG_MATH_PHYS_TIER_FIELD)) { return LDG_ERR_MATH_PHYS_TIER_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_EM)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(ctx->region[region].field == 0x0)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_em_ring_find(const ldg_math_phys_region_t *region_ptr, const ldg_uint128_t *time, uint64_t *out_slot_a, uint64_t *out_slot_b, double *out_ratio)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_uint128_t *stamp = 0x0;
    ldg_uint128_t span = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t into = LDG_STRUCT_ZERO_INIT;
    double span_val = 0.0;
    double into_val = 0.0;
    uint64_t depth = 0;
    uint64_t oldest = 0;
    uint64_t newest = 0;
    uint64_t slot = 0;
    uint64_t prev_slot = 0;
    uint64_t scan_idx = 0;
    uint64_t hd_local = 0;
    uint64_t cunt_local = 0;
    uint32_t err = 0;
    uint32_t ord = 0;
    *out_slot_a = 0;
    *out_slot_b = 0;
    *out_ratio = 0.0;
    hdr = ldg_math_phys_em_hdr_get(region_ptr);
    stamp = ldg_math_phys_em_stamp_get(region_ptr);
    hd_local = LDG_LD_ACQUIRE(hdr->snapshot_hd);
    cunt_local = LDG_RD_ONCE(hdr->snapshot_cunt);
    depth = hdr->snapshot_depth;
    if (cunt_local > depth) { cunt_local = depth; }

    if (cunt_local == 0) { return LDG_ERR_MATH_PHYS_SAMPLE_FUTURE; }

    newest = (hd_local + depth - 1) % depth;
    oldest = (hd_local + depth - cunt_local) % depth;
    err = ldg_arith_uint128_cmp(time, &stamp[newest], &ord);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (ord == LDG_ARITH_ORD_GT) { return LDG_ERR_MATH_PHYS_SAMPLE_FUTURE; }

    err = ldg_arith_uint128_cmp(time, &stamp[oldest], &ord);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (ord == LDG_ARITH_ORD_LT) { return LDG_ERR_MATH_PHYS_SAMPLE_EXPIRED; }

    prev_slot = oldest;
    for (scan_idx = 0; scan_idx < cunt_local; scan_idx++)
    {
        slot = (oldest + scan_idx) % depth;
        err = ldg_arith_uint128_cmp(time, &stamp[slot], &ord);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (ord == LDG_ARITH_ORD_EQ)
        {
            *out_slot_a = slot;
            *out_slot_b = slot;
            return LDG_ERR_AOK;
        }

        if (ord == LDG_ARITH_ORD_LT)
        {
            *out_slot_a = prev_slot;
            *out_slot_b = slot;
            err = ldg_arith_uint128_sub(&stamp[slot], &stamp[prev_slot], &span);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_arith_uint128_sub(time, &stamp[prev_slot], &into);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_arith_uint128_to_double_convert(&span, &span_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_arith_uint128_to_double_convert(&into, &into_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            *out_ratio = into_val / span_val;
            return LDG_ERR_AOK;
        }

        prev_slot = slot;
    }
    return LDG_ERR_MATH_PHYS_SAMPLE_FUTURE;
}

static uint32_t ldg_math_phys_acoustic_region_ck(const ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.tier != LDG_MATH_PHYS_TIER_FIELD)) { return LDG_ERR_MATH_PHYS_TIER_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(ctx->region[region].field == 0x0)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_acoustic_ring_find(const ldg_math_phys_region_t *region_ptr, const ldg_uint128_t *time, uint64_t *out_slot_a, uint64_t *out_slot_b, double *out_ratio)
{
    ldg_math_phys_acoustic_hdr_t *hdr = 0x0;
    ldg_uint128_t *stamp = 0x0;
    ldg_uint128_t span = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t into = LDG_STRUCT_ZERO_INIT;
    double span_val = 0.0;
    double into_val = 0.0;
    uint64_t depth = 0;
    uint64_t oldest = 0;
    uint64_t newest = 0;
    uint64_t slot = 0;
    uint64_t prev_slot = 0;
    uint64_t scan_idx = 0;
    uint64_t hd_local = 0;
    uint64_t cunt_local = 0;
    uint32_t err = 0;
    uint32_t ord = 0;
    *out_slot_a = 0;
    *out_slot_b = 0;
    *out_ratio = 0.0;
    hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    stamp = ldg_math_phys_acoustic_stamp_get(region_ptr);
    hd_local = LDG_LD_ACQUIRE(hdr->snapshot_hd);
    cunt_local = LDG_RD_ONCE(hdr->snapshot_cunt);
    depth = hdr->snapshot_depth;
    if (cunt_local > depth) { cunt_local = depth; }

    if (cunt_local == 0) { return LDG_ERR_MATH_PHYS_SAMPLE_FUTURE; }

    newest = (hd_local + depth - 1) % depth;
    oldest = (hd_local + depth - cunt_local) % depth;
    err = ldg_arith_uint128_cmp(time, &stamp[newest], &ord);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (ord == LDG_ARITH_ORD_GT) { return LDG_ERR_MATH_PHYS_SAMPLE_FUTURE; }

    err = ldg_arith_uint128_cmp(time, &stamp[oldest], &ord);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (ord == LDG_ARITH_ORD_LT) { return LDG_ERR_MATH_PHYS_SAMPLE_EXPIRED; }

    prev_slot = oldest;
    for (scan_idx = 0; scan_idx < cunt_local; scan_idx++)
    {
        slot = (oldest + scan_idx) % depth;
        err = ldg_arith_uint128_cmp(time, &stamp[slot], &ord);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (ord == LDG_ARITH_ORD_EQ)
        {
            *out_slot_a = slot;
            *out_slot_b = slot;
            return LDG_ERR_AOK;
        }

        if (ord == LDG_ARITH_ORD_LT)
        {
            *out_slot_a = prev_slot;
            *out_slot_b = slot;
            err = ldg_arith_uint128_sub(&stamp[slot], &stamp[prev_slot], &span);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_arith_uint128_sub(time, &stamp[prev_slot], &into);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_arith_uint128_to_double_convert(&span, &span_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_arith_uint128_to_double_convert(&into, &into_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            *out_ratio = into_val / span_val;
            return LDG_ERR_AOK;
        }

        prev_slot = slot;
    }
    return LDG_ERR_MATH_PHYS_SAMPLE_FUTURE;
}

static double ldg_math_phys_lattice_sample_get(const double *buff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, const double *pos, const double *offset)
{
    double coord[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double frac[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double lo_val = 0.0;
    double hi_val = 0.0;
    double front = 0.0;
    double back = 0.0;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t base[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t plane = 0;
    uint64_t corner = 0;
    uint32_t axis = 0;
    dim[0] = nx;
    dim[1] = ny;
    dim[2] = nz;
    plane = nx * ny;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (dim[axis] < 2) { continue; }

        coord[axis] = pos[axis] / spacing - offset[axis];
        if (coord[axis] < 0.0) { coord[axis] = 0.0; }

        if (coord[axis] > (double)(dim[axis] - 1)) { coord[axis] = (double)(dim[axis] - 1); }

        base[axis] = (uint64_t)coord[axis];
        if (base[axis] > dim[axis] - 2) { base[axis] = dim[axis] - 2; }

        frac[axis] = coord[axis] - (double)base[axis];
    }
    corner = (base[2] * ny + base[1]) * nx + base[0];
    lo_val = buff[corner] + (buff[corner + 1] - buff[corner]) * frac[0];
    hi_val = buff[corner + nx] + (buff[corner + nx + 1] - buff[corner + nx]) * frac[0];
    front = lo_val + (hi_val - lo_val) * frac[1];
    if (nz < 2) { return front; }

    lo_val = buff[corner + plane] + (buff[corner + plane + 1] - buff[corner + plane]) * frac[0];
    hi_val = buff[corner + plane + nx] + (buff[corner + plane + nx + 1] - buff[corner + plane + nx]) * frac[0];
    back = lo_val + (hi_val - lo_val) * frac[1];
    return front + (back - front) * frac[2];
}

LDG_EXPORT uint32_t ldg_math_phys_mtl_reg(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_mtl_desc_t *desc, uint32_t *out)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    double *row = 0x0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;
    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->epsilon_r) || desc->epsilon_r <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->mu_r) || desc->mu_r <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ldg_math_nan_is(desc->sigma) || desc->sigma < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->epsilon_r * desc->mu_r < 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->mtl_cunt >= hdr->mtl_cap)) { return LDG_ERR_FULL; }

    row = ldg_math_phys_em_coeff_get(&ctx->region[region]) + hdr->mtl_cunt * LDG_MATH_PHYS_EM_COEFF_STRIDE;
    ldg_math_phys_em_mtl_coeff_set(row, desc, hdr->dt_sub);
    *out = (uint32_t)hdr->mtl_cunt;
    hdr->mtl_cunt++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_mtl_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi, uint32_t mtl)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    uint16_t *mtl_map = 0x0;
    uint64_t lo_cell[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t hi_cell[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint32_t err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!lo)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!hi)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(mtl >= hdr->mtl_cunt)) { return LDG_ERR_NOT_FOUND; }

    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(lo[axis]) || !ldg_math_finite_is(hi[axis]) || lo[axis] < 0.0 || hi[axis] < lo[axis] || hi[axis] >= ctx->region[region].desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

        lo_cell[axis] = (uint64_t)ldg_math_floor_get(lo[axis] / ctx->region[region].desc.spacing);
        hi_cell[axis] = (uint64_t)ldg_math_ceil_get(hi[axis] / ctx->region[region].desc.spacing);
        if (hi_cell[axis] > dim[axis] - 1) { hi_cell[axis] = dim[axis] - 1; }
    }
    mtl_map = ldg_math_phys_em_mtl_get(&ctx->region[region]);
    for (idx_z = lo_cell[2]; idx_z <= hi_cell[2]; idx_z++) { for (idx_y = lo_cell[1]; idx_y <= hi_cell[1]; idx_y++) { for (idx_x = lo_cell[0]; idx_x <= hi_cell[0]; idx_x++) { mtl_map[(idx_z * hdr->ny + idx_y) * hdr->nx + idx_x] = (uint16_t)mtl; } } }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_sheet_reg(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_sheet_desc_t *desc, uint32_t *out)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_math_phys_em_sheet_t *entry = 0x0;
    double pair_sum = 0.0;
    double pi = 0.0;
    double harmonic = 0.0;
    uint64_t pole_idx = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;
    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->nz == 1)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->sigma) || desc->sigma <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->thickness) || desc->thickness <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->mu_r) || desc->mu_r <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->thickness >= ctx->region[region].desc.spacing / 5.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->pole_cunt > LDG_MATH_PHYS_SHEET_POLE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(hdr->sheet_cunt >= LDG_MATH_PHYS_SHEET_CAP)) { return LDG_ERR_FULL; }

    entry = ldg_math_phys_em_sheet_get(&ctx->region[region]) + hdr->sheet_cunt;
    err = ldg_mem_secure_zero(entry, sizeof(ldg_math_phys_em_sheet_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    entry->sigma = desc->sigma;
    entry->thickness = desc->thickness;
    entry->mu_val = desc->mu_r * LDG_MATH_PHYS_VACUUM_PERMEABILITY;
    entry->pole_cunt = desc->pole_cunt;
    for (pole_idx = 0; pole_idx < desc->pole_cunt; pole_idx++) { pair_sum += ldg_math_abs_get(desc->residue[pole_idx]) + ldg_math_abs_get(desc->pole[pole_idx]); }
    pi = ldg_math_arccos_get(0.0 - 1.0);
    for (pole_idx = 0; pole_idx < desc->pole_cunt; pole_idx++)
    {
        if (pair_sum > 0.0)
        {
            if (LDG_UNLIKELY(!ldg_math_finite_is(desc->pole[pole_idx]) || desc->pole[pole_idx] <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (LDG_UNLIKELY(!ldg_math_finite_is(desc->residue[pole_idx]))) { return LDG_ERR_FUNC_ARG_INVALID; }

            entry->pole[pole_idx] = desc->pole[pole_idx];
            entry->residue[pole_idx] = desc->residue[pole_idx];
        }
        else
        {
            harmonic = (double)(pole_idx + 1);
            entry->pole[pole_idx] = harmonic * harmonic * pi * pi / (entry->mu_val * entry->sigma * entry->thickness * entry->thickness);
            entry->residue[pole_idx] = (0.0 - 2.0) * entry->pole[pole_idx] / (entry->sigma * entry->thickness);
            entry->inst += 2.0 / (entry->sigma * entry->thickness);
        }

        entry->decay[pole_idx] = ldg_math_exp_get((0.0 - entry->pole[pole_idx]) * hdr->dt_sub);
        entry->gain[pole_idx] = (entry->residue[pole_idx] / entry->pole[pole_idx]) * (1.0 - entry->decay[pole_idx]);
    }
    entry->used = 1;
    *out = (uint32_t)hdr->sheet_cunt;
    hdr->sheet_cunt++;
    return LDG_ERR_AOK;
}

static uint8_t ldg_math_phys_polygon_inside_is(const double *vert, uint64_t vert_cunt, double pos_a, double pos_b)
{
    double edge_a1 = 0.0;
    double edge_b1 = 0.0;
    double edge_a2 = 0.0;
    double edge_b2 = 0.0;
    uint64_t edge_idx = 0;
    uint64_t prev_idx = 0;
    uint8_t crossing = 0;

    if (LDG_UNLIKELY(!vert)) { return LDG_ERR_FUNC_ARG_NULL; }

    prev_idx = vert_cunt - 1;
    for (edge_idx = 0; edge_idx < vert_cunt; edge_idx++)
    {
        edge_a1 = vert[2 * edge_idx];
        edge_b1 = vert[2 * edge_idx + 1];
        edge_a2 = vert[2 * prev_idx];
        edge_b2 = vert[2 * prev_idx + 1];
        if ((edge_b1 > pos_b) != (edge_b2 > pos_b)) { if (pos_a < edge_a1 + (pos_b - edge_b1) * (edge_a2 - edge_a1) / (edge_b2 - edge_b1)) { crossing = (uint8_t)(1 - crossing); } }

        prev_idx = edge_idx;
    }

    if (crossing != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

LDG_EXPORT uint32_t ldg_math_phys_sheet_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t normal_axis, double plane_pos, const double *vert, uint64_t vert_cunt, uint32_t sheet)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_math_phys_em_sheet_t *sheet_ptr = 0x0;
    ldg_math_phys_em_face_t *face = 0x0;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    const double *coeff = 0x0;
    const uint16_t *mtl = 0x0;
    double eps_eff = 0.0;
    double sigma_eff = 0.0;
    double loss = 0.0;
    double spacing = 0.0;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t stride[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t plane_idx = 0;
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    uint64_t hit_cunt = 0;
    uint64_t vert_idx = 0;
    uint64_t cell = 0;
    uint32_t err = 0;
    uint32_t axis_a = 0;
    uint32_t axis_b = 0;
    uint32_t pass = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!vert)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_ptr = &ctx->region[region];
    hdr = ldg_math_phys_em_hdr_get(region_ptr);
    if (LDG_UNLIKELY(hdr->nz == 1)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(normal_axis >= LDG_MATH_PHYS_DIM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(sheet >= hdr->sheet_cunt)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(vert_cunt < 3)) { return LDG_ERR_FUNC_ARG_INVALID; }

    spacing = region_ptr->desc.spacing;
    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    stride[0] = 1;
    stride[1] = hdr->nx;
    stride[2] = hdr->nx * hdr->ny;
    if (LDG_UNLIKELY(!ldg_math_finite_is(plane_pos) || plane_pos < 0.0 || plane_pos >= region_ptr->desc.extent[normal_axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

    plane_idx = (uint64_t)(plane_pos / spacing + 0.5);
    if (LDG_UNLIKELY(plane_idx < 1 || plane_idx > dim[normal_axis] - 2)) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

    axis_a = (normal_axis == 0) ? 1 : 0;
    axis_b = (normal_axis == 2) ? 1 : 2;
    for (vert_idx = 0; vert_idx < vert_cunt; vert_idx++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(vert[2 * vert_idx]) || vert[2 * vert_idx] < 0.0 || vert[2 * vert_idx] >= region_ptr->desc.extent[axis_a])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(vert[2 * vert_idx + 1]) || vert[2 * vert_idx + 1] < 0.0 || vert[2 * vert_idx + 1] >= region_ptr->desc.extent[axis_b])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
    }
    sheet_ptr = ldg_math_phys_em_sheet_get(region_ptr) + sheet;
    coeff = ldg_math_phys_em_coeff_get(region_ptr);
    mtl = ldg_math_phys_em_mtl_get(region_ptr);
    face = ldg_math_phys_em_face_get(region_ptr);
    sigma_eff = sheet_ptr->sigma * sheet_ptr->thickness / spacing;
    for (pass = 0; pass < 2; pass++)
    {
        hit_cunt = 0;
        for (idx_b = 1; idx_b + 1 < dim[axis_b]; idx_b++)
        {
            for (idx_a = 1; idx_a + 1 < dim[axis_a]; idx_a++)
            {
                if (ldg_math_phys_polygon_inside_is(vert, vert_cunt, (double)idx_a * spacing, (double)idx_b * spacing) != LDG_TRUTH_TRUE) { continue; }

                if (pass == 1)
                {
                    cell = plane_idx * stride[normal_axis] + idx_a * stride[axis_a] + idx_b * stride[axis_b];
                    err = ldg_mem_secure_zero(&face[hdr->face_cunt], sizeof(ldg_math_phys_em_face_t));
                    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                    eps_eff = coeff[(uint64_t)mtl[cell] * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_EPSILON];
                    loss = sigma_eff * hdr->dt_sub / (2.0 * eps_eff);
                    face[hdr->face_cunt].ca_mod = (1.0 - loss) / (1.0 + loss);
                    face[hdr->face_cunt].cb_mod = (hdr->dt_sub / eps_eff) / (1.0 + loss);
                    face[hdr->face_cunt].cell = cell;
                    face[hdr->face_cunt].sheet = sheet;
                    face[hdr->face_cunt].normal_axis = normal_axis;
                    hdr->face_cunt++;
                }

                hit_cunt++;
            }
        }
        if (pass == 0 && LDG_UNLIKELY(hdr->face_cunt + hit_cunt > hdr->face_cap)) { return LDG_ERR_FULL; }
    }
    if (LDG_UNLIKELY(hit_cunt == 0)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_current_add(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, const double *current)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    double *drive = 0x0;
    uint64_t cell[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t center = 0;
    uint32_t err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!current)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(pos[axis]) || pos[axis] < 0.0 || pos[axis] >= ctx->region[region].desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(current[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; }

        cell[axis] = (uint64_t)(pos[axis] / ctx->region[region].desc.spacing + 0.5);
        if (cell[axis] > dim[axis] - 1) { cell[axis] = dim[axis] - 1; }
    }
    drive = ldg_math_phys_em_drive_get(&ctx->region[region]);
    center = (cell[2] * hdr->ny + cell[1]) * hdr->nx + cell[0];
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { drive[axis * hdr->cell_cunt + center] += current[axis]; }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_acoustic_plane_wave_set(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_plane_wave_desc_t *desc)
{
    ldg_math_phys_acoustic_hdr_t *hdr = 0x0;
    double *p_aux = 0x0;
    double dir_val = 0.0;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t margin = 0;
    uint64_t axis_u = LDG_MATH_PHYS_DIM;
    uint32_t err = 0;
    uint32_t axis = 0;
    err = ldg_math_phys_acoustic_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_acoustic_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->nz == 1)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(hdr->mtl_cunt == 0)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->direction[axis]) || !ldg_math_finite_is(desc->polarization[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(ldg_math_abs_get(desc->polarization[axis]) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (ldg_math_abs_get(desc->direction[axis]) > 0.0)
        {
            if (LDG_UNLIKELY(axis_u != LDG_MATH_PHYS_DIM)) { return LDG_ERR_UNSUPPORTED; }

            axis_u = axis;
            dir_val = desc->direction[axis];
        }
    }
    if (LDG_UNLIKELY(axis_u == LDG_MATH_PHYS_DIM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    margin = hdr->pml_depth + 3;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(dim[axis] < 2 * margin + 3)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

        hdr->tf_lo[axis] = margin;
        hdr->tf_hi[axis] = dim[axis] - 1 - margin;
    }
    hdr->plane_axis = axis_u;
    hdr->plane_sign = (dir_val > 0.0) ? 1.0 : (0.0 - 1.0);
    hdr->drive_cur = 0.0;
    hdr->drive_prev = 0.0;
    p_aux = ldg_math_phys_acoustic_aux_get(&ctx->region[region]);
    err = ldg_mem_secure_zero(p_aux, 2 * hdr->aux_cunt * sizeof(double));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr->plane_armed = 1;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_plane_wave_set(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_plane_wave_desc_t *desc)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    double *e_aux = 0x0;
    double dir_val = 0.0;
    double pol_val = 0.0;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t margin = 0;
    uint64_t axis_u = LDG_MATH_PHYS_DIM;
    uint64_t axis_p = LDG_MATH_PHYS_DIM;
    uint32_t err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->is_init == LDG_TRUTH_TRUE && region < ctx->conf.region_cap && ctx->region[region].used && ctx->region[region].desc.field_kind == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC) { return ldg_math_phys_acoustic_plane_wave_set(ctx, region, desc); }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->nz == 1)) { return LDG_ERR_UNSUPPORTED; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->direction[axis]) || !ldg_math_finite_is(desc->polarization[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (ldg_math_abs_get(desc->direction[axis]) > 0.0)
        {
            if (LDG_UNLIKELY(axis_u != LDG_MATH_PHYS_DIM)) { return LDG_ERR_UNSUPPORTED; }

            axis_u = axis;
            dir_val = desc->direction[axis];
        }

        if (ldg_math_abs_get(desc->polarization[axis]) > 0.0)
        {
            if (LDG_UNLIKELY(axis_p != LDG_MATH_PHYS_DIM)) { return LDG_ERR_UNSUPPORTED; }

            axis_p = axis;
            pol_val = desc->polarization[axis];
        }
    }
    if (LDG_UNLIKELY(axis_u == LDG_MATH_PHYS_DIM || axis_p == LDG_MATH_PHYS_DIM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(axis_u == axis_p)) { return LDG_ERR_FUNC_ARG_INVALID; }

    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    margin = hdr->pml_depth + 3;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(dim[axis] < 2 * margin + 3)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

        hdr->tf_lo[axis] = margin;
        hdr->tf_hi[axis] = dim[axis] - 1 - margin;
    }
    hdr->plane_axis = axis_u;
    hdr->plane_pol_axis = axis_p;
    hdr->plane_hpol_axis = 3 - axis_u - axis_p;
    hdr->plane_sign = (dir_val > 0.0) ? 1.0 : (0.0 - 1.0);
    hdr->plane_pol_sign = (pol_val > 0.0) ? 1.0 : (0.0 - 1.0);
    hdr->plane_hpol_sign = hdr->plane_sign * hdr->plane_pol_sign * ldg_math_phys_em_cycle_sign_get(axis_u, axis_p);
    hdr->drive_cur = 0.0;
    hdr->drive_prev = 0.0;
    e_aux = ldg_math_phys_em_aux_get(&ctx->region[region]);
    err = ldg_mem_secure_zero(e_aux, 2 * hdr->aux_cunt * sizeof(double));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr->plane_armed = 1;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_plane_wave_drive(ldg_math_phys_ctx_t *ctx, uint32_t region, double amplitude)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_math_phys_acoustic_hdr_t *acoustic_hdr = 0x0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->is_init == LDG_TRUTH_TRUE && region < ctx->conf.region_cap && ctx->region[region].used && ctx->region[region].desc.field_kind == LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC)
    {
        err = ldg_math_phys_acoustic_region_ck(ctx, region);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(amplitude))) { return LDG_ERR_FUNC_ARG_INVALID; }

        acoustic_hdr = ldg_math_phys_acoustic_hdr_get(&ctx->region[region]);
        if (LDG_UNLIKELY(!acoustic_hdr->plane_armed)) { return LDG_ERR_NOT_INIT; }

        acoustic_hdr->drive_cur = amplitude;
        return LDG_ERR_AOK;
    }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(amplitude))) { return LDG_ERR_FUNC_ARG_INVALID; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(!hdr->plane_armed)) { return LDG_ERR_NOT_INIT; }

    hdr->drive_cur = amplitude;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_acoustic_monopole_add(ldg_math_phys_ctx_t *ctx, uint32_t region, uint64_t body, double amplitude, double freq, uint64_t *out)
{
    ldg_math_phys_acoustic_hdr_t *hdr = 0x0;
    ldg_math_phys_acoustic_monopole_t *monopole = 0x0;
    ldg_math_phys_body_motion_t motion = LDG_STRUCT_ZERO_INIT;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT64_MAX;
    err = ldg_math_phys_acoustic_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(amplitude))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(freq) || freq <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_body_motion_get(ctx, body, &motion);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(motion.region != region)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    hdr = ldg_math_phys_acoustic_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->monopole_cunt >= LDG_MATH_PHYS_ACOUSTIC_MONOPOLE_CAP)) { return LDG_ERR_FULL; }

    monopole = ldg_math_phys_acoustic_monopole_get(&ctx->region[region]);
    monopole[hdr->monopole_cunt].body = body;
    monopole[hdr->monopole_cunt].amplitude = amplitude;
    monopole[hdr->monopole_cunt].freq = freq;
    monopole[hdr->monopole_cunt].tick_start = hdr->tick_cuntr;
    *out = hdr->monopole_cunt;
    hdr->monopole_cunt++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_acoustic_monopole_series_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint64_t monopole, const double *series, uint64_t cunt)
{
    ldg_math_phys_acoustic_hdr_t *hdr = 0x0;
    ldg_math_phys_acoustic_monopole_t *slot = 0x0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(series != 0x0 && cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_acoustic_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_acoustic_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(monopole >= hdr->monopole_cunt)) { return LDG_ERR_MATH_PHYS_KIND_INVALID; }

    slot = ldg_math_phys_acoustic_monopole_get(&ctx->region[region]);
    slot[monopole].series = series;
    slot[monopole].series_cunt = (series != 0x0) ? cunt : 0;
    slot[monopole].tick_start = hdr->tick_cuntr;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_field_em_sample_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, const ldg_uint128_t *time, double *out_electric, double *out_magnetic)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    double *snapshot_a = 0x0;
    double *snapshot_b = 0x0;
    double offset[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double sample_a = 0.0;
    double sample_b = 0.0;
    double sample_val = 0.0;
    double ratio = 0.0;
    uint64_t slot_a = 0;
    uint64_t slot_b = 0;
    uint32_t err = 0;
    uint32_t comp_idx = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!time)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_electric && !out_magnetic)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (out_electric) { out_electric[axis] = 0.0; }

        if (out_magnetic) { out_magnetic[axis] = 0.0; }
    }
    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_ptr = &ctx->region[region];
    hdr = ldg_math_phys_em_hdr_get(region_ptr);
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(pos[axis]) || pos[axis] < 0.0 || pos[axis] >= region_ptr->desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; } }
    err = ldg_math_phys_em_ring_find(region_ptr, time, &slot_a, &slot_b, &ratio);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    snapshot_a = ldg_math_phys_em_snapshot_get(region_ptr, slot_a);
    snapshot_b = ldg_math_phys_em_snapshot_get(region_ptr, slot_b);
    for (comp_idx = 0; comp_idx < 6; comp_idx++)
    {
        if (comp_idx < LDG_MATH_PHYS_DIM && !out_electric) { continue; }

        if (comp_idx >= 3 && !out_magnetic) { continue; }

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { offset[axis] = (comp_idx < LDG_MATH_PHYS_DIM) ? ((axis == comp_idx) ? 0.5 : 0.0) : ((axis == comp_idx - LDG_MATH_PHYS_DIM) ? 0.0 : 0.5); }
        sample_a = ldg_math_phys_lattice_sample_get(snapshot_a + comp_idx * hdr->cell_cunt, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, pos, offset);
        sample_b = ldg_math_phys_lattice_sample_get(snapshot_b + comp_idx * hdr->cell_cunt, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, pos, offset);
        sample_val = sample_a + (sample_b - sample_a) * ratio;
        if (comp_idx < LDG_MATH_PHYS_DIM) { out_electric[comp_idx] = sample_val; }
        else { out_magnetic[comp_idx - LDG_MATH_PHYS_DIM] = sample_val; }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_field_em_slice_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_slice_desc_t *desc, const ldg_uint128_t *time, double *out_electric, double *out_magnetic)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    double *snapshot_a = 0x0;
    double *snapshot_b = 0x0;
    double *out_plane = 0x0;
    double offset[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double pos[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double sample_a = 0.0;
    double sample_b = 0.0;
    double ratio = 0.0;
    double lo_pos = 0.0;
    double hi_pos = 0.0;
    uint64_t slot_a = 0;
    uint64_t slot_b = 0;
    uint64_t hd_local = 0;
    uint64_t cunt_local = 0;
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    uint64_t span_cunt = 0;
    uint32_t err = 0;
    uint32_t comp_idx = 0;
    uint32_t axis = 0;
    uint32_t pass = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_electric && !out_magnetic)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_ptr = &ctx->region[region];
    hdr = ldg_math_phys_em_hdr_get(region_ptr);
    if (LDG_UNLIKELY(desc->axis_a >= LDG_MATH_PHYS_DIM || desc->axis_b >= LDG_MATH_PHYS_DIM || desc->axis_a == desc->axis_b)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->step_a) || desc->step_a <= 0.0 || !ldg_math_finite_is(desc->step_b) || desc->step_b <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->cunt_a == 0 || desc->cunt_b == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(desc->orig[axis]) || desc->orig[axis] < 0.0 || desc->orig[axis] >= region_ptr->desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; } }
    for (pass = 0; pass < 2; pass++)
    {
        axis = (pass == 0) ? desc->axis_a : desc->axis_b;
        span_cunt = (pass == 0) ? desc->cunt_a : desc->cunt_b;
        lo_pos = desc->orig[axis];
        hi_pos = lo_pos + (double)(span_cunt - 1) * ((pass == 0) ? desc->step_a : desc->step_b);
        if (LDG_UNLIKELY(!ldg_math_finite_is(hi_pos) || hi_pos >= region_ptr->desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
    }
    if (time)
    {
        err = ldg_math_phys_em_ring_find(region_ptr, time, &slot_a, &slot_b, &ratio);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    else
    {
        hd_local = LDG_LD_ACQUIRE(hdr->snapshot_hd);
        cunt_local = LDG_RD_ONCE(hdr->snapshot_cunt);
        if (cunt_local > hdr->snapshot_depth) { cunt_local = hdr->snapshot_depth; }

        if (LDG_UNLIKELY(cunt_local == 0)) { return LDG_ERR_MATH_PHYS_SAMPLE_FUTURE; }

        slot_a = (hd_local + hdr->snapshot_depth - 1) % hdr->snapshot_depth;
        slot_b = slot_a;
        ratio = 0.0;
    }

    snapshot_a = ldg_math_phys_em_snapshot_get(region_ptr, slot_a);
    snapshot_b = ldg_math_phys_em_snapshot_get(region_ptr, slot_b);
    for (comp_idx = 0; comp_idx < 6; comp_idx++)
    {
        if (comp_idx < LDG_MATH_PHYS_DIM && !out_electric) { continue; }

        if (comp_idx >= 3 && !out_magnetic) { continue; }

        out_plane = (comp_idx < LDG_MATH_PHYS_DIM) ? out_electric : out_magnetic;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { offset[axis] = (comp_idx < LDG_MATH_PHYS_DIM) ? ((axis == comp_idx) ? 0.5 : 0.0) : ((axis == comp_idx - LDG_MATH_PHYS_DIM) ? 0.0 : 0.5); }
        for (idx_b = 0; idx_b < desc->cunt_b; idx_b++) { for (idx_a = 0; idx_a < desc->cunt_a; idx_a++)
            {
                for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { pos[axis] = desc->orig[axis]; }
                pos[desc->axis_a] = desc->orig[desc->axis_a] + (double)idx_a * desc->step_a;
                pos[desc->axis_b] = desc->orig[desc->axis_b] + (double)idx_b * desc->step_b;
                sample_a = ldg_math_phys_lattice_sample_get(snapshot_a + comp_idx * hdr->cell_cunt, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, pos, offset);
                sample_b = ldg_math_phys_lattice_sample_get(snapshot_b + comp_idx * hdr->cell_cunt, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, pos, offset);
                out_plane[(idx_b * desc->cunt_a + idx_a) * LDG_MATH_PHYS_DIM + (comp_idx % 3)] = sample_a + (sample_b - sample_a) * ratio;
            }
        }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_field_em_vol_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t vol_stride, double *out)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    const double *snapshot = 0x0;
    const double *comp_base = 0x0;
    uint64_t hd_local = 0;
    uint64_t cunt_local = 0;
    uint64_t slot = 0;
    uint64_t vol_dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t vol_cell = 0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint32_t err = 0;
    uint32_t comp_idx = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(vol_stride == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_ptr = &ctx->region[region];
    hdr = ldg_math_phys_em_hdr_get(region_ptr);
    hd_local = LDG_LD_ACQUIRE(hdr->snapshot_hd);
    cunt_local = LDG_RD_ONCE(hdr->snapshot_cunt);
    if (cunt_local > hdr->snapshot_depth) { cunt_local = hdr->snapshot_depth; }

    if (LDG_UNLIKELY(cunt_local == 0)) { return LDG_ERR_MATH_PHYS_SAMPLE_FUTURE; }

    slot = (hd_local + hdr->snapshot_depth - 1) % hdr->snapshot_depth;
    snapshot = ldg_math_phys_em_snapshot_get(region_ptr, slot);
    vol_dim[0] = (hdr->nx + vol_stride - 1) / vol_stride;
    vol_dim[1] = (hdr->ny + vol_stride - 1) / vol_stride;
    vol_dim[2] = (hdr->nz + vol_stride - 1) / vol_stride;
    vol_cell = vol_dim[0] * vol_dim[1] * vol_dim[2];
    for (comp_idx = 0; comp_idx < 6; comp_idx++)
    {
        comp_base = snapshot + comp_idx * hdr->cell_cunt;
        for (idx_z = 0; idx_z < vol_dim[2]; idx_z++) { for (idx_y = 0; idx_y < vol_dim[1]; idx_y++) { for (idx_x = 0; idx_x < vol_dim[0]; idx_x++) { out[comp_idx * vol_cell + (idx_z * vol_dim[1] + idx_y) * vol_dim[0] + idx_x] = comp_base[(idx_z * vol_stride * hdr->ny + idx_y * vol_stride) * hdr->nx + idx_x * vol_stride]; } } }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_acoustic_mtl_reg(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_acoustic_mtl_desc_t *desc, uint32_t *out)
{
    ldg_math_phys_acoustic_hdr_t *hdr = 0x0;
    double *row = 0x0;
    double courant_max = 0.0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;
    err = ldg_math_phys_acoustic_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->density) || desc->density <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->speed) || desc->speed <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->loss) || desc->loss < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    hdr = ldg_math_phys_acoustic_hdr_get(&ctx->region[region]);
    courant_max = 1.0 / ldg_math_sqrt_get((hdr->nz == 1) ? 2.0 : 3.0);
    if (LDG_UNLIKELY(desc->speed * hdr->dt_sub / ctx->region[region].desc.spacing > courant_max)) { return LDG_ERR_MATH_PHYS_CONF_INVALID; }

    if (LDG_UNLIKELY(hdr->mtl_cunt >= hdr->mtl_cap)) { return LDG_ERR_FULL; }

    row = ldg_math_phys_acoustic_coeff_get(&ctx->region[region]) + hdr->mtl_cunt * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
    ldg_math_phys_acoustic_mtl_coeff_set(row, desc, hdr->dt_sub);
    if (hdr->mtl_cunt == 0) { ldg_math_phys_acoustic_stretch_set(ldg_math_phys_acoustic_stretch_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, hdr->pml_depth, hdr->dt_sub, ctx->region[region].desc.spacing, desc->speed, hdr->pml_freq); }

    *out = (uint32_t)hdr->mtl_cunt;
    hdr->mtl_cunt++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_acoustic_mtl_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi, uint32_t mtl)
{
    ldg_math_phys_acoustic_hdr_t *hdr = 0x0;
    uint16_t *mtl_map = 0x0;
    uint64_t lo_cell[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t hi_cell[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint32_t err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!lo)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!hi)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_acoustic_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_acoustic_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(mtl >= hdr->mtl_cunt)) { return LDG_ERR_NOT_FOUND; }

    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(lo[axis]) || !ldg_math_finite_is(hi[axis]) || lo[axis] < 0.0 || hi[axis] < lo[axis] || hi[axis] >= ctx->region[region].desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

        lo_cell[axis] = (uint64_t)ldg_math_floor_get(lo[axis] / ctx->region[region].desc.spacing);
        hi_cell[axis] = (uint64_t)ldg_math_ceil_get(hi[axis] / ctx->region[region].desc.spacing);
        if (hi_cell[axis] > dim[axis] - 1) { hi_cell[axis] = dim[axis] - 1; }
    }
    mtl_map = ldg_math_phys_acoustic_mtl_get(&ctx->region[region]);
    for (idx_z = lo_cell[2]; idx_z <= hi_cell[2]; idx_z++) { for (idx_y = lo_cell[1]; idx_y <= hi_cell[1]; idx_y++) { for (idx_x = lo_cell[0]; idx_x <= hi_cell[0]; idx_x++) { mtl_map[(idx_z * hdr->ny + idx_y) * hdr->nx + idx_x] = (uint16_t)mtl; } } }
    return LDG_ERR_AOK;
}

static uint8_t ldg_math_phys_wall_admissible_is(const double *residue, const double *pole, uint64_t pole_cunt, double z_inf, double z_local)
{
    double base[LDG_MATH_PHYS_WALL_POLE_MAX + 1] = LDG_ARR_ZERO_INIT;
    double part[LDG_MATH_PHYS_WALL_POLE_MAX + 1] = LDG_ARR_ZERO_INIT;
    double next[LDG_MATH_PHYS_WALL_POLE_MAX + 1] = LDG_ARR_ZERO_INIT;
    double total[LDG_MATH_PHYS_WALL_POLE_MAX + 1] = LDG_ARR_ZERO_INIT;
    uint64_t deg = 0;
    uint64_t pole_idx = 0;
    uint64_t skip_idx = 0;
    uint64_t coeff_idx = 0;
    base[0] = 1.0;
    for (pole_idx = 0; pole_idx < pole_cunt; pole_idx++)
    {
        for (coeff_idx = 0; coeff_idx <= LDG_MATH_PHYS_WALL_POLE_MAX; coeff_idx++) { next[coeff_idx] = 0.0; }
        for (coeff_idx = 0; coeff_idx <= pole_idx; coeff_idx++)
        {
            next[coeff_idx + 1] += base[coeff_idx];
            next[coeff_idx] += pole[pole_idx] * base[coeff_idx];
        }
        for (coeff_idx = 0; coeff_idx <= LDG_MATH_PHYS_WALL_POLE_MAX; coeff_idx++) { base[coeff_idx] = next[coeff_idx]; }
    }
    deg = pole_cunt;
    for (coeff_idx = 0; coeff_idx <= deg; coeff_idx++) { total[coeff_idx] = (z_inf + z_local) * base[coeff_idx]; }
    for (skip_idx = 0; skip_idx < pole_cunt; skip_idx++)
    {
        part[0] = 1.0;
        for (coeff_idx = 1; coeff_idx <= LDG_MATH_PHYS_WALL_POLE_MAX; coeff_idx++) { part[coeff_idx] = 0.0; }
        deg = 0;
        for (pole_idx = 0; pole_idx < pole_cunt; pole_idx++)
        {
            if (pole_idx == skip_idx) { continue; }

            for (coeff_idx = 0; coeff_idx <= LDG_MATH_PHYS_WALL_POLE_MAX; coeff_idx++) { next[coeff_idx] = 0.0; }
            for (coeff_idx = 0; coeff_idx <= deg; coeff_idx++)
            {
                next[coeff_idx + 1] += part[coeff_idx];
                next[coeff_idx] += pole[pole_idx] * part[coeff_idx];
            }
            for (coeff_idx = 0; coeff_idx <= LDG_MATH_PHYS_WALL_POLE_MAX; coeff_idx++) { part[coeff_idx] = next[coeff_idx]; }
            deg++;
        }
        for (coeff_idx = 0; coeff_idx <= deg; coeff_idx++) { total[coeff_idx] += residue[skip_idx] * part[coeff_idx]; }
    }
    deg = pole_cunt;
    for (coeff_idx = 0; coeff_idx <= deg; coeff_idx++) { if (!(total[coeff_idx] > 0.0)) { return LDG_TRUTH_FALSE; } }
    if (deg == 3) { if (!(total[2] * total[1] > total[3] * total[0])) { return LDG_TRUTH_FALSE; } }

    if (deg == 4)
    {
        if (!(total[3] * total[2] > total[4] * total[1])) { return LDG_TRUTH_FALSE; }

        if (!((total[3] * total[2] - total[4] * total[1]) * total[1] > total[3] * total[3] * total[0])) { return LDG_TRUTH_FALSE; }
    }

    return LDG_TRUTH_TRUE;
}

LDG_EXPORT uint32_t ldg_math_phys_wall_reg(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_wall_desc_t *desc, uint32_t *out)
{
    ldg_math_phys_acoustic_hdr_t *hdr = 0x0;
    ldg_math_phys_acoustic_wall_t *entry = 0x0;
    const double *row = 0x0;
    double z_local = 0.0;
    uint64_t pole_idx = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;
    err = ldg_math_phys_acoustic_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_acoustic_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->nz == 1)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(hdr->mtl_cunt == 0)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->z_inf) || desc->z_inf < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->pole_cunt > LDG_MATH_PHYS_WALL_POLE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (pole_idx = 0; pole_idx < desc->pole_cunt; pole_idx++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->pole[pole_idx]) || desc->pole[pole_idx] <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->residue[pole_idx]))) { return LDG_ERR_FUNC_ARG_INVALID; }
    }
    if (LDG_UNLIKELY(hdr->wall_cunt >= LDG_MATH_PHYS_WALL_CAP)) { return LDG_ERR_FULL; }

    row = ldg_math_phys_acoustic_coeff_get(&ctx->region[region]);
    z_local = row[LDG_MATH_PHYS_ACOUSTIC_COEFF_DENSITY] * row[LDG_MATH_PHYS_ACOUSTIC_COEFF_SPEED];
    if (LDG_UNLIKELY(ldg_math_phys_wall_admissible_is(desc->residue, desc->pole, desc->pole_cunt, desc->z_inf, z_local) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    entry = ldg_math_phys_acoustic_wall_get(&ctx->region[region]) + hdr->wall_cunt;
    err = ldg_mem_secure_zero(entry, sizeof(ldg_math_phys_acoustic_wall_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    entry->z_inf = desc->z_inf;
    entry->pole_cunt = desc->pole_cunt;
    entry->denom = desc->z_inf;
    for (pole_idx = 0; pole_idx < desc->pole_cunt; pole_idx++)
    {
        entry->pole[pole_idx] = desc->pole[pole_idx];
        entry->residue[pole_idx] = desc->residue[pole_idx];
        entry->decay[pole_idx] = ldg_math_exp_get((0.0 - desc->pole[pole_idx]) * hdr->dt_sub);
        entry->gain[pole_idx] = (1.0 - entry->decay[pole_idx]) / desc->pole[pole_idx];
        entry->denom += entry->residue[pole_idx] * entry->gain[pole_idx];
    }
    if (LDG_UNLIKELY(!(entry->denom > 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    entry->used = 1;
    *out = (uint32_t)hdr->wall_cunt;
    hdr->wall_cunt++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_wall_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t normal_axis, double plane_pos, const double *vert, uint64_t vert_cunt, uint32_t wall)
{
    ldg_math_phys_acoustic_hdr_t *hdr = 0x0;
    ldg_math_phys_acoustic_wall_t *wall_ptr = 0x0;
    ldg_math_phys_acoustic_face_t *face = 0x0;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    const double *coeff = 0x0;
    const double *row = 0x0;
    const uint16_t *mtl = 0x0;
    double spacing = 0.0;
    double z_local = 0.0;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t stride[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t plane_idx = 0;
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    uint64_t hit_cunt = 0;
    uint64_t vert_idx = 0;
    uint64_t cell = 0;
    uint64_t cell_side = 0;
    uint32_t err = 0;
    uint32_t axis_a = 0;
    uint32_t axis_b = 0;
    uint32_t pass = 0;
    uint32_t side = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!vert)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_acoustic_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_ptr = &ctx->region[region];
    hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    if (LDG_UNLIKELY(hdr->nz == 1)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(normal_axis >= LDG_MATH_PHYS_DIM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(wall >= hdr->wall_cunt)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(vert_cunt < 3)) { return LDG_ERR_FUNC_ARG_INVALID; }

    spacing = region_ptr->desc.spacing;
    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    stride[0] = 1;
    stride[1] = hdr->nx;
    stride[2] = hdr->nx * hdr->ny;
    if (LDG_UNLIKELY(!ldg_math_finite_is(plane_pos) || plane_pos < 0.0 || plane_pos >= region_ptr->desc.extent[normal_axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

    plane_idx = (uint64_t)(plane_pos / spacing + 0.5);
    if (LDG_UNLIKELY(plane_idx < 1 || plane_idx > dim[normal_axis] - 2)) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

    axis_a = (normal_axis == 0) ? 1 : 0;
    axis_b = (normal_axis == 2) ? 1 : 2;
    for (vert_idx = 0; vert_idx < vert_cunt; vert_idx++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(vert[2 * vert_idx]) || vert[2 * vert_idx] < 0.0 || vert[2 * vert_idx] >= region_ptr->desc.extent[axis_a])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(vert[2 * vert_idx + 1]) || vert[2 * vert_idx + 1] < 0.0 || vert[2 * vert_idx + 1] >= region_ptr->desc.extent[axis_b])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
    }
    wall_ptr = ldg_math_phys_acoustic_wall_get(region_ptr) + wall;
    coeff = ldg_math_phys_acoustic_coeff_get(region_ptr);
    mtl = ldg_math_phys_acoustic_mtl_get(region_ptr);
    face = ldg_math_phys_acoustic_face_get(region_ptr);
    for (pass = 0; pass < 2; pass++)
    {
        hit_cunt = 0;
        for (idx_b = 1; idx_b + 1 < dim[axis_b]; idx_b++)
        {
            for (idx_a = 1; idx_a + 1 < dim[axis_a]; idx_a++)
            {
                if (ldg_math_phys_polygon_inside_is(vert, vert_cunt, (double)idx_a * spacing, (double)idx_b * spacing) != LDG_TRUTH_TRUE) { continue; }

                cell = plane_idx * stride[normal_axis] + idx_a * stride[axis_a] + idx_b * stride[axis_b];
                if (pass == 0) { for (side = 0; side < 2; side++)
                    {
                        cell_side = cell + ((side == 1) ? stride[normal_axis] : 0);
                        row = coeff + (uint64_t)mtl[cell_side] * LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE;
                        z_local = row[LDG_MATH_PHYS_ACOUSTIC_COEFF_DENSITY] * row[LDG_MATH_PHYS_ACOUSTIC_COEFF_SPEED];
                        if (LDG_UNLIKELY(ldg_math_phys_wall_admissible_is(wall_ptr->residue, wall_ptr->pole, wall_ptr->pole_cunt, wall_ptr->z_inf, z_local) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }
                    }
                }

                if (pass == 1)
                {
                    err = ldg_mem_secure_zero(&face[hdr->face_cunt], sizeof(ldg_math_phys_acoustic_face_t));
                    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                    face[hdr->face_cunt].cell = cell;
                    face[hdr->face_cunt].wall = wall;
                    face[hdr->face_cunt].normal_axis = normal_axis;
                    hdr->face_cunt++;
                }

                hit_cunt++;
            }
        }
        if (pass == 0 && LDG_UNLIKELY(hdr->face_cunt + hit_cunt > hdr->face_cap)) { return LDG_ERR_FULL; }
    }
    if (LDG_UNLIKELY(hit_cunt == 0)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_field_acoustic_sample_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, const ldg_uint128_t *time, double *out_pressure, double *out_velocity)
{
    ldg_math_phys_acoustic_hdr_t *hdr = 0x0;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    double *snapshot_a = 0x0;
    double *snapshot_b = 0x0;
    double offset[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double sample_a = 0.0;
    double sample_b = 0.0;
    double sample_val = 0.0;
    double ratio = 0.0;
    uint64_t slot_a = 0;
    uint64_t slot_b = 0;
    uint32_t err = 0;
    uint32_t comp_idx = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!time)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_pressure && !out_velocity)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (out_pressure) { *out_pressure = 0.0; }

    if (out_velocity) { for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { out_velocity[axis] = 0.0; } }

    err = ldg_math_phys_acoustic_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_ptr = &ctx->region[region];
    hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(pos[axis]) || pos[axis] < 0.0 || pos[axis] >= region_ptr->desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; } }
    err = ldg_math_phys_acoustic_ring_find(region_ptr, time, &slot_a, &slot_b, &ratio);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    snapshot_a = ldg_math_phys_acoustic_snapshot_get(region_ptr, slot_a);
    snapshot_b = ldg_math_phys_acoustic_snapshot_get(region_ptr, slot_b);
    for (comp_idx = 0; comp_idx < 4; comp_idx++)
    {
        if (comp_idx == 0 && !out_pressure) { continue; }

        if (comp_idx > 0 && !out_velocity) { continue; }

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { offset[axis] = (comp_idx > 0 && axis == comp_idx - 1) ? 0.5 : 0.0; }
        sample_a = ldg_math_phys_lattice_sample_get(snapshot_a + comp_idx * hdr->cell_cunt, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, pos, offset);
        sample_b = ldg_math_phys_lattice_sample_get(snapshot_b + comp_idx * hdr->cell_cunt, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, pos, offset);
        sample_val = sample_a + (sample_b - sample_a) * ratio;
        if (comp_idx == 0) { *out_pressure = sample_val; }
        else { out_velocity[comp_idx - 1] = sample_val; }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_field_acoustic_slice_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_slice_desc_t *desc, const ldg_uint128_t *time, double *out_pressure, double *out_velocity)
{
    ldg_math_phys_acoustic_hdr_t *hdr = 0x0;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    double *snapshot_a = 0x0;
    double *snapshot_b = 0x0;
    double offset[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double pos[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double sample_a = 0.0;
    double sample_b = 0.0;
    double sample_val = 0.0;
    double ratio = 0.0;
    double lo_pos = 0.0;
    double hi_pos = 0.0;
    uint64_t slot_a = 0;
    uint64_t slot_b = 0;
    uint64_t hd_local = 0;
    uint64_t cunt_local = 0;
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    uint64_t span_cunt = 0;
    uint32_t err = 0;
    uint32_t comp_idx = 0;
    uint32_t axis = 0;
    uint32_t pass = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_pressure && !out_velocity)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_acoustic_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_ptr = &ctx->region[region];
    hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    if (LDG_UNLIKELY(desc->axis_a >= LDG_MATH_PHYS_DIM || desc->axis_b >= LDG_MATH_PHYS_DIM || desc->axis_a == desc->axis_b)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->step_a) || desc->step_a <= 0.0 || !ldg_math_finite_is(desc->step_b) || desc->step_b <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->cunt_a == 0 || desc->cunt_b == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(desc->orig[axis]) || desc->orig[axis] < 0.0 || desc->orig[axis] >= region_ptr->desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; } }
    for (pass = 0; pass < 2; pass++)
    {
        axis = (pass == 0) ? desc->axis_a : desc->axis_b;
        span_cunt = (pass == 0) ? desc->cunt_a : desc->cunt_b;
        lo_pos = desc->orig[axis];
        hi_pos = lo_pos + (double)(span_cunt - 1) * ((pass == 0) ? desc->step_a : desc->step_b);
        if (LDG_UNLIKELY(!ldg_math_finite_is(hi_pos) || hi_pos >= region_ptr->desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
    }
    if (time)
    {
        err = ldg_math_phys_acoustic_ring_find(region_ptr, time, &slot_a, &slot_b, &ratio);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    else
    {
        hd_local = LDG_LD_ACQUIRE(hdr->snapshot_hd);
        cunt_local = LDG_RD_ONCE(hdr->snapshot_cunt);
        if (cunt_local > hdr->snapshot_depth) { cunt_local = hdr->snapshot_depth; }

        if (LDG_UNLIKELY(cunt_local == 0)) { return LDG_ERR_MATH_PHYS_SAMPLE_FUTURE; }

        slot_a = (hd_local + hdr->snapshot_depth - 1) % hdr->snapshot_depth;
        slot_b = slot_a;
        ratio = 0.0;
    }

    snapshot_a = ldg_math_phys_acoustic_snapshot_get(region_ptr, slot_a);
    snapshot_b = ldg_math_phys_acoustic_snapshot_get(region_ptr, slot_b);
    for (comp_idx = 0; comp_idx < 4; comp_idx++)
    {
        if (comp_idx == 0 && !out_pressure) { continue; }

        if (comp_idx > 0 && !out_velocity) { continue; }

        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { offset[axis] = (comp_idx > 0 && axis == comp_idx - 1) ? 0.5 : 0.0; }
        for (idx_b = 0; idx_b < desc->cunt_b; idx_b++)
        {
            for (idx_a = 0; idx_a < desc->cunt_a; idx_a++)
            {
                for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { pos[axis] = desc->orig[axis]; }
                pos[desc->axis_a] = desc->orig[desc->axis_a] + (double)idx_a * desc->step_a;
                pos[desc->axis_b] = desc->orig[desc->axis_b] + (double)idx_b * desc->step_b;
                sample_a = ldg_math_phys_lattice_sample_get(snapshot_a + comp_idx * hdr->cell_cunt, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, pos, offset);
                sample_b = ldg_math_phys_lattice_sample_get(snapshot_b + comp_idx * hdr->cell_cunt, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, pos, offset);
                sample_val = sample_a + (sample_b - sample_a) * ratio;
                if (comp_idx == 0) { out_pressure[idx_b * desc->cunt_a + idx_a] = sample_val; }
                else { out_velocity[(idx_b * desc->cunt_a + idx_a) * LDG_MATH_PHYS_DIM + (comp_idx - 1)] = sample_val; }
            }
        }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_field_sample_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, const ldg_uint128_t *time, double *out)
{
    ldg_math_phys_field_hdr_t *hdr = 0x0;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    double *snapshot = 0x0;
    double val_a = 0.0;
    double val_b = 0.0;
    double ratio = 0.0;
    uint32_t err = 0;
    uint32_t frame_a = 0;
    uint32_t frame_b = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!time)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    region_ptr = &ctx->region[region];
    if (LDG_UNLIKELY(region_ptr->desc.tier != LDG_MATH_PHYS_TIER_FIELD)) { return LDG_ERR_MATH_PHYS_TIER_INVALID; }

    if (LDG_UNLIKELY(region_ptr->desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_SCALAR)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(region_ptr->field == 0x0)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(pos[axis]) || pos[axis] < 0.0 || pos[axis] >= region_ptr->desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; } }
    err = ldg_math_phys_history_frame_find(ctx, time, &frame_a, &frame_b, &ratio);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_field_hdr_get(region_ptr);
    snapshot = ldg_math_phys_field_snapshot_get(region_ptr, frame_a);
    if (hdr->nz == 1) { err = ldg_math_phys_field_2d_sample_get(snapshot, hdr->nx, hdr->ny, region_ptr->desc.spacing, pos[0], pos[1], &val_a); }
    else { err = ldg_math_phys_field_3d_sample_get(snapshot, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, pos[0], pos[1], pos[2], &val_a); }

    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    snapshot = ldg_math_phys_field_snapshot_get(region_ptr, frame_b);
    if (hdr->nz == 1) { err = ldg_math_phys_field_2d_sample_get(snapshot, hdr->nx, hdr->ny, region_ptr->desc.spacing, pos[0], pos[1], &val_b); }
    else { err = ldg_math_phys_field_3d_sample_get(snapshot, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, pos[0], pos[1], pos[2], &val_b); }

    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    *out = val_a + (val_b - val_a) * ratio;
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_split_plan_get(ldg_math_phys_ctx_t *ctx, uint64_t body, const uint32_t *kind, const ldg_math_phys_body_desc_t *desc, uint64_t cunt, uint32_t *region_out, ldg_math_phys_conserved_t *share, uint64_t *extra, uint8_t *deficit_sign)
{
    ldg_math_phys_conserved_t deficit[4] = LDG_ARR_ZERO_INIT;
    ldg_math_phys_conserved_t diff_val = LDG_STRUCT_ZERO_INIT;
    ldg_uint256_t cunt_val = LDG_STRUCT_ZERO_INIT;
    ldg_uint256_t quot = LDG_STRUCT_ZERO_INIT;
    ldg_uint256_t rem = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t *conserved_ptr = 0x0;
    void *comp_ptr = 0x0;
    int64_t charge_sum = 0;
    int64_t lepton_sum = 0;
    int64_t baryon_sum = 0;
    int64_t parent_charge = 0;
    int64_t parent_lepton = 0;
    int64_t parent_baryon = 0;
    uint64_t child_idx = 0;
    uint32_t err = 0;
    uint32_t region = 0;
    uint32_t parent_kind = 0;
    uint32_t aggregate_idx = 0;
    uint32_t quantity_idx = 0;
    uint32_t axis = 0;
    for (child_idx = 0; child_idx < cunt; child_idx++)
    {
        if (LDG_UNLIKELY((kind[child_idx] & LDG_MATH_PHYS_AGGREGATE_ID_BASE) == 0)) { return LDG_ERR_MATH_PHYS_KIND_INVALID; }

        aggregate_idx = kind[child_idx] & ~LDG_MATH_PHYS_AGGREGATE_ID_BASE;
        if (LDG_UNLIKELY(aggregate_idx >= ctx->conf.aggregate_cap || !ctx->aggregate[aggregate_idx].used)) { return LDG_ERR_MATH_PHYS_KIND_INVALID; }
    }
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region = *(uint32_t *)comp_ptr;
    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    *region_out = region;
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_KIND, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    parent_kind = *(uint32_t *)comp_ptr;
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_CHARGE, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    parent_charge = (int64_t)conserved_ptr->magnitude.limb[0];
    if (conserved_ptr->sign) { parent_charge = 0 - parent_charge; }

    if (ldg_ecs_ctx_comp_has(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_TAG) == LDG_TRUTH_TRUE)
    {
        err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_TAG, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        parent_lepton = ((ldg_math_phys_tag_t *)comp_ptr)->lepton;
        parent_baryon = ((ldg_math_phys_tag_t *)comp_ptr)->baryon;
    }
    else { ldg_math_phys_particle_tag_get(parent_kind, &parent_charge, &parent_lepton, &parent_baryon); }

    for (child_idx = 0; child_idx < cunt; child_idx++)
    {
        charge_sum += desc[child_idx].charge;
        lepton_sum += desc[child_idx].lepton;
        baryon_sum += desc[child_idx].baryon;
    }
    if (LDG_UNLIKELY(charge_sum != parent_charge)) { return LDG_ERR_MATH_PHYS_CONSERVATION_CHARGE; }

    if (LDG_UNLIKELY(lepton_sum != parent_lepton || baryon_sum != parent_baryon)) { return LDG_ERR_MATH_PHYS_CONSERVATION_TAG; }

    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MASS, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    deficit[0] = *(ldg_math_phys_conserved_t *)comp_ptr;
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MOMENTUM, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { deficit[1 + axis] = conserved_ptr[axis]; }
    err = ldg_math_phys_split_sum_get(ctx, &ctx->region[region], desc, cunt, share);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (quantity_idx = 0; quantity_idx < 4; quantity_idx++)
    {
        err = ldg_math_phys_conserved_sub(&deficit[quantity_idx], &share[quantity_idx], &diff_val);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        deficit[quantity_idx] = diff_val;
    }
    cunt_val.limb[0] = cunt;
    for (quantity_idx = 0; quantity_idx < 4; quantity_idx++)
    {
        err = ldg_arith_uint256_div_compute(&deficit[quantity_idx].magnitude, &cunt_val, &quot, &rem);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        share[quantity_idx].magnitude = quot;
        share[quantity_idx].sign = 0;
        if ((quot.limb[0] | quot.limb[1] | quot.limb[2] | quot.limb[3]) != 0 && deficit[quantity_idx].sign) { share[quantity_idx].sign = 1; }

        extra[quantity_idx] = rem.limb[0];
        deficit_sign[quantity_idx] = deficit[quantity_idx].sign;
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_body_split(ldg_math_phys_ctx_t *ctx, uint64_t body, const uint32_t *kind, const ldg_math_phys_body_desc_t *desc, uint64_t cunt, uint64_t *out)
{
    ldg_math_phys_conserved_t share[4] = LDG_ARR_ZERO_INIT;
    ldg_math_phys_conserved_t term[4] = LDG_ARR_ZERO_INIT;
    ldg_math_phys_conserved_t quantum = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t adjusted = LDG_STRUCT_ZERO_INIT;
    uint64_t extra[4] = LDG_ARR_ZERO_INIT;
    uint8_t deficit_sign[4] = LDG_ARR_ZERO_INIT;
    uint64_t child_idx = 0;
    uint64_t rollback_idx = 0;
    uint32_t err = 0;
    uint32_t rollback_err = 0;
    uint32_t region = 0;
    uint32_t quantity_idx = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!kind)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (child_idx = 0; child_idx < cunt; child_idx++) { out[child_idx] = 0; }
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(&ctx->world->store, body) != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY((ldg_ecs_ctx_entity_category_get(ctx->world, body) & LDG_MATH_PHYS_PIN_CATEGORY) != 0)) { return LDG_ERR_MATH_PHYS_PROMOTE_DENIED; }

    err = ldg_math_phys_split_plan_get(ctx, body, kind, desc, cunt, &region, share, extra, deficit_sign);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (child_idx = 0; child_idx < cunt; child_idx++)
    {
        for (quantity_idx = 0; quantity_idx < 4; quantity_idx++)
        {
            term[quantity_idx] = share[quantity_idx];
            if (child_idx < extra[quantity_idx])
            {
                quantum.magnitude.limb[0] = 1;
                quantum.sign = deficit_sign[quantity_idx];
                err = ldg_math_phys_conserved_add(&share[quantity_idx], &quantum, &adjusted);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                term[quantity_idx] = adjusted;
            }
        }
        err = ldg_math_phys_split_child_spawn(ctx, region, kind[child_idx], &desc[child_idx], term, &out[child_idx]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK))
        {
            for (rollback_idx = 0; rollback_idx < child_idx; rollback_idx++)
            {
                rollback_err = ldg_math_phys_body_destroy(ctx, out[rollback_idx]);
                if (rollback_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = rollback_err; }

                out[rollback_idx] = 0;
            }
            out[child_idx] = 0;
            return err;
        }
    }
    err = ldg_math_phys_body_destroy(ctx, body);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        for (rollback_idx = 0; rollback_idx < cunt; rollback_idx++)
        {
            rollback_err = ldg_math_phys_body_destroy(ctx, out[rollback_idx]);
            if (rollback_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = rollback_err; }

            out[rollback_idx] = 0;
        }
        return err;
    }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_body_merge(ldg_math_phys_ctx_t *ctx, const uint64_t *body, uint64_t cunt, uint32_t kind, uint64_t *out)
{
    ldg_math_phys_body_val_t merged_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_tag_t tag_val = LDG_STRUCT_ZERO_INIT;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    double com[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double pos_avg[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double momentum_val[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double angular_sum[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double inertia_total = 0.0;
    double mass_total = 0.0;
    double mass_val = 0.0;
    double energy_val = 0.0;
    double angular_val = 0.0;
    double bound = 0.0;
    uint64_t merged_body = 0;
    uint64_t src_idx = 0;
    uint32_t err = 0;
    uint32_t region = 0;
    uint32_t aggregate_idx = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!body)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(cunt < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY((kind & LDG_MATH_PHYS_AGGREGATE_ID_BASE) == 0)) { return LDG_ERR_MATH_PHYS_KIND_INVALID; }

    aggregate_idx = kind & ~LDG_MATH_PHYS_AGGREGATE_ID_BASE;
    if (LDG_UNLIKELY(aggregate_idx >= ctx->conf.aggregate_cap || !ctx->aggregate[aggregate_idx].used)) { return LDG_ERR_MATH_PHYS_KIND_INVALID; }

    err = ldg_math_phys_merge_sum_get(ctx, body, cunt, &merged_val, &tag_val, &region, com, pos_avg, &mass_total, angular_sum, &inertia_total);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_ptr = &ctx->region[region];
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (mass_total > 0.0) { com[axis] /= mass_total; }
        else { com[axis] = pos_avg[axis] / (double)cunt; }

        bound = ldg_math_next_toward_get(region_ptr->desc.extent[axis], 0.0);
        if (com[axis] < 0.0) { com[axis] = 0.0; }

        if (com[axis] > bound) { com[axis] = bound; }

        err = ldg_arith_double_to_uint128_convert(ldg_math_pwr2_scale_get(com[axis], LDG_MATH_PHYS_POS_FRACTION), &merged_val.pos_fixed[axis]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    err = ldg_math_phys_conserved_to_double_convert(&merged_val.mass_c, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_MASS), &mass_val);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_math_phys_conserved_to_double_convert(&merged_val.momentum_c[axis], ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_MOMENTUM_X), &momentum_val[axis]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    merged_val.orientation[LDG_MATH_QUATERNION_DIM - 1] = 1.0;
    merged_val.inertia = inertia_total;
    if (inertia_total > 0.0) { for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { merged_val.spin[axis] = (angular_sum[axis] - (com[(axis + 1) % LDG_MATH_PHYS_DIM] * momentum_val[(axis + 2) % LDG_MATH_PHYS_DIM] - com[(axis + 2) % LDG_MATH_PHYS_DIM] * momentum_val[(axis + 1) % LDG_MATH_PHYS_DIM])) / inertia_total; } }

    if (mass_val > 0.0) { energy_val = (momentum_val[0] * momentum_val[0] + momentum_val[1] * momentum_val[1] + momentum_val[2] * momentum_val[2]) / (2.0 * mass_val) + 0.5 * inertia_total * (merged_val.spin[0] * merged_val.spin[0] + merged_val.spin[1] * merged_val.spin[1] + merged_val.spin[2] * merged_val.spin[2]); }

    err = ldg_math_phys_double_to_conserved_convert(energy_val, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY), &merged_val.energy_c);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        angular_val = com[(axis + 1) % LDG_MATH_PHYS_DIM] * momentum_val[(axis + 2) % LDG_MATH_PHYS_DIM] - com[(axis + 2) % LDG_MATH_PHYS_DIM] * momentum_val[(axis + 1) % LDG_MATH_PHYS_DIM] + inertia_total * merged_val.spin[axis];
        err = ldg_math_phys_double_to_conserved_convert(angular_val, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_X), &merged_val.angular_c[axis]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    err = ldg_math_phys_body_wr(ctx, region, kind, &merged_val, &tag_val, &merged_body);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    *out = merged_body;
    for (src_idx = 0; src_idx < cunt; src_idx++)
    {
        err = ldg_math_phys_body_destroy(ctx, body[src_idx]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_region_couple_set(ldg_math_phys_ctx_t *ctx, uint32_t region, double emit, double absorb)
{
    ldg_math_phys_field_hdr_t *hdr = 0x0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.tier != LDG_MATH_PHYS_TIER_FIELD)) { return LDG_ERR_MATH_PHYS_TIER_INVALID; }

    if (LDG_UNLIKELY(ctx->region[region].desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_SCALAR)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(ctx->region[region].field == 0x0)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(emit) || !ldg_math_finite_is(absorb))) { return LDG_ERR_FUNC_ARG_INVALID; }

    hdr = ldg_math_phys_field_hdr_get(&ctx->region[region]);
    hdr->emit = emit;
    hdr->absorb = absorb;
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_merge_pair_find(const ldg_math_phys_ctx_t *ctx, uint32_t region, double merge_dist, uint64_t *out_a, uint64_t *out_b, uint8_t *found)
{
    ldg_math_phys_body_motion_t motion_a = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_body_motion_t motion_b = LDG_STRUCT_ZERO_INIT;
    void *comp_ptr = 0x0;
    double best_dist_sq = 0.0;
    double dist_sq = 0.0;
    double diff = 0.0;
    uint64_t entity_a = 0;
    uint64_t entity_b = 0;
    uint32_t err = 0;
    uint32_t dense_a = 0;
    uint32_t dense_b = 0;
    uint32_t kind_a = 0;
    uint32_t kind_b = 0;
    uint32_t axis = 0;
    *out_a = 0;
    *out_b = 0;
    *found = 0;
    best_dist_sq = merge_dist * merge_dist;
    for (dense_a = 0; dense_a < ctx->world->store.alive_cunt; dense_a++)
    {
        entity_a = ctx->world->store.dense[dense_a];
        if (ldg_ecs_ctx_comp_has(ctx->world, entity_a, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION) != LDG_TRUTH_TRUE) { continue; }

        err = ldg_ecs_ctx_comp_get(ctx->world, entity_a, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (*(uint32_t *)comp_ptr != region) { continue; }

        err = ldg_ecs_ctx_comp_get(ctx->world, entity_a, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_KIND, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        kind_a = *(uint32_t *)comp_ptr;
        if ((kind_a & LDG_MATH_PHYS_AGGREGATE_ID_BASE) == 0) { continue; }

        if ((ldg_ecs_ctx_entity_category_get(ctx->world, entity_a) & LDG_MATH_PHYS_PIN_CATEGORY) != 0) { continue; }

        err = ldg_math_phys_body_motion_get(ctx, entity_a, &motion_a);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        for (dense_b = dense_a + 1; dense_b < ctx->world->store.alive_cunt; dense_b++)
        {
            entity_b = ctx->world->store.dense[dense_b];
            if (ldg_ecs_ctx_comp_has(ctx->world, entity_b, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION) != LDG_TRUTH_TRUE) { continue; }

            err = ldg_ecs_ctx_comp_get(ctx->world, entity_b, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            if (*(uint32_t *)comp_ptr != region) { continue; }

            err = ldg_ecs_ctx_comp_get(ctx->world, entity_b, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_KIND, &comp_ptr);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            kind_b = *(uint32_t *)comp_ptr;
            if (kind_b != kind_a) { continue; }

            if ((ldg_ecs_ctx_entity_category_get(ctx->world, entity_b) & LDG_MATH_PHYS_PIN_CATEGORY) != 0) { continue; }

            err = ldg_math_phys_body_motion_get(ctx, entity_b, &motion_b);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            dist_sq = 0.0;
            for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
            {
                diff = motion_b.pos[axis] - motion_a.pos[axis];
                dist_sq += diff * diff;
            }
            if (dist_sq <= best_dist_sq)
            {
                best_dist_sq = dist_sq;
                *out_a = entity_a;
                *out_b = entity_b;
                *found = 1;
            }
        }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_body_fragment(ldg_math_phys_ctx_t *ctx, uint32_t region, uint64_t body, const ldg_math_phys_body_motion_t *motion, uint32_t kind, int64_t charge, int64_t lepton, int64_t baryon)
{
    ldg_math_phys_history_hdr_t *hdr = 0x0;
    ldg_math_phys_body_desc_t *child_desc = 0x0;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    void *comp_ptr = 0x0;
    double parent_orientation[LDG_MATH_QUATERNION_DIM] = LDG_ARR_ZERO_INIT;
    double parent_spin[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double parent_inertia = 0.0;
    double golden = 0.0;
    double height = 0.0;
    double rho = 0.0;
    double theta = 0.0;
    double bound = 0.0;
    int64_t base = 0;
    int64_t rem = 0;
    int64_t unit = 0;
    uint64_t rem_cunt = 0;
    uint64_t frag_cunt = 0;
    uint64_t child_idx = 0;
    uint32_t err = 0;
    uint32_t axis = 0;
    region_ptr = &ctx->region[region];
    frag_cunt = region_ptr->policy.fragment_cunt;
    if (LDG_UNLIKELY(frag_cunt < 2 || frag_cunt > LDG_MATH_PHYS_FRAGMENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_ORIENTATION, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_QUATERNION_DIM; axis++) { parent_orientation[axis] = ((double *)comp_ptr)[axis]; }
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_SPIN, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { parent_spin[axis] = ((double *)comp_ptr)[axis]; }
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_INERTIA, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    parent_inertia = *(double *)comp_ptr;
    hdr = ldg_math_phys_history_hdr_get(ctx);
    child_desc = hdr->fragment_desc;
    golden = ldg_math_arccos_get(0.0 - 1.0) * (3.0 - ldg_math_sqrt_get(5.0));
    for (child_idx = 0; child_idx < frag_cunt; child_idx++)
    {
        hdr->fragment_kind[child_idx] = kind;
        height = 1.0 - 2.0 * ((double)child_idx + 0.5) / (double)frag_cunt;
        rho = ldg_math_sqrt_get(1.0 - height * height);
        theta = golden * (double)child_idx;
        child_desc[child_idx].pos[0] = motion->pos[0] + region_ptr->policy.fragment_radius * rho * ldg_math_cos_get(theta);
        child_desc[child_idx].pos[1] = motion->pos[1] + region_ptr->policy.fragment_radius * rho * ldg_math_sin_get(theta);
        child_desc[child_idx].pos[2] = motion->pos[2] + region_ptr->policy.fragment_radius * height;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            bound = ldg_math_next_toward_get(region_ptr->desc.extent[axis], 0.0);
            if (child_desc[child_idx].pos[axis] < 0.0) { child_desc[child_idx].pos[axis] = 0.0; }

            if (child_desc[child_idx].pos[axis] > bound) { child_desc[child_idx].pos[axis] = bound; }

            child_desc[child_idx].velocity[axis] = motion->velocity[axis];
            child_desc[child_idx].spin[axis] = parent_spin[axis];
        }
        for (axis = 0; axis < LDG_MATH_QUATERNION_DIM; axis++) { child_desc[child_idx].orientation[axis] = parent_orientation[axis]; }
        child_desc[child_idx].mass = motion->mass / (double)frag_cunt;
        child_desc[child_idx].inertia = parent_inertia / (double)frag_cunt;
    }
    base = charge / (int64_t)frag_cunt;
    rem = charge - base * (int64_t)frag_cunt;
    unit = (rem < 0) ? (0 - 1) : 1;
    rem_cunt = (uint64_t)((rem < 0) ? (0 - rem) : rem);
    for (child_idx = 0; child_idx < frag_cunt; child_idx++) { child_desc[child_idx].charge = base + ((child_idx < rem_cunt) ? unit : 0); }
    base = lepton / (int64_t)frag_cunt;
    rem = lepton - base * (int64_t)frag_cunt;
    unit = (rem < 0) ? (0 - 1) : 1;
    rem_cunt = (uint64_t)((rem < 0) ? (0 - rem) : rem);
    for (child_idx = 0; child_idx < frag_cunt; child_idx++) { child_desc[child_idx].lepton = base + ((child_idx < rem_cunt) ? unit : 0); }
    base = baryon / (int64_t)frag_cunt;
    rem = baryon - base * (int64_t)frag_cunt;
    unit = (rem < 0) ? (0 - 1) : 1;
    rem_cunt = (uint64_t)((rem < 0) ? (0 - rem) : rem);
    for (child_idx = 0; child_idx < frag_cunt; child_idx++) { child_desc[child_idx].baryon = base + ((child_idx < rem_cunt) ? unit : 0); }
    return ldg_math_phys_body_split(ctx, body, hdr->fragment_kind, child_desc, frag_cunt, hdr->fragment_child);
}

static uint32_t ldg_math_phys_step_tier_apply(ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_body_motion_t motion = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_region_t *region_ptr = 0x0;
    void *comp_ptr = 0x0;
    uint64_t pair[2] = LDG_ARR_ZERO_INIT;
    uint64_t merged = 0;
    uint64_t entity = 0;
    uint64_t promote_body = 0;
    int64_t charge_val = 0;
    int64_t lepton_val = 0;
    int64_t baryon_val = 0;
    uint32_t err = 0;
    uint32_t idx = 0;
    uint32_t dense_idx = 0;
    uint32_t kind_val = 0;
    uint8_t found = 0;
    for (idx = 0; idx < ctx->conf.region_cap; idx++)
    {
        region_ptr = &ctx->region[idx];
        if (!region_ptr->used) { continue; }

        if (region_ptr->policy.high_water > 0 && region_ptr->policy.merge_dist > 0.0 && region_ptr->body_cunt > region_ptr->policy.high_water)
        {
            while (region_ptr->body_cunt > region_ptr->policy.low_water)
            {
                err = ldg_math_phys_merge_pair_find(ctx, idx, region_ptr->policy.merge_dist, &pair[0], &pair[1], &found);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                if (!found) { break; }

                err = ldg_ecs_ctx_comp_get(ctx->world, pair[0], ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_KIND, &comp_ptr);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                kind_val = *(uint32_t *)comp_ptr;
                err = ldg_math_phys_body_merge(ctx, pair, 2, kind_val, &merged);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
            }
        }

        if (region_ptr->policy.promote_mass > 0.0 && region_ptr->policy.fragment_cunt >= 2 && region_ptr->policy.fragment_radius > 0.0)
        {
            for (;;)
            {
                if (region_ptr->body_cunt + region_ptr->policy.fragment_cunt - 1 > region_ptr->policy.low_water) { break; }

                promote_body = 0;
                for (dense_idx = 0; dense_idx < ctx->world->store.alive_cunt; dense_idx++)
                {
                    entity = ctx->world->store.dense[dense_idx];
                    if (ldg_ecs_ctx_comp_has(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION) != LDG_TRUTH_TRUE) { continue; }

                    err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
                    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                    if (*(uint32_t *)comp_ptr != idx) { continue; }

                    err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_KIND, &comp_ptr);
                    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                    kind_val = *(uint32_t *)comp_ptr;
                    if ((kind_val & LDG_MATH_PHYS_AGGREGATE_ID_BASE) == 0) { continue; }

                    if ((ldg_ecs_ctx_entity_category_get(ctx->world, entity) & LDG_MATH_PHYS_PIN_CATEGORY) != 0) { continue; }

                    err = ldg_math_phys_body_motion_get(ctx, entity, &motion);
                    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                    if (motion.mass > region_ptr->policy.promote_mass)
                    {
                        promote_body = entity;
                        break;
                    }
                }
                if (promote_body == 0) { break; }

                err = ldg_ecs_ctx_comp_get(ctx->world, promote_body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_CHARGE, &comp_ptr);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                charge_val = (int64_t)((ldg_math_phys_conserved_t *)comp_ptr)->magnitude.limb[0];
                if (((ldg_math_phys_conserved_t *)comp_ptr)->sign) { charge_val = 0 - charge_val; }

                err = ldg_ecs_ctx_comp_get(ctx->world, promote_body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_TAG, &comp_ptr);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

                lepton_val = ((ldg_math_phys_tag_t *)comp_ptr)->lepton;
                baryon_val = ((ldg_math_phys_tag_t *)comp_ptr)->baryon;
                err = ldg_math_phys_body_fragment(ctx, idx, promote_body, &motion, kind_val, charge_val, lepton_val, baryon_val);
                if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
            }
        }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_em_body_charge_get(const ldg_math_phys_ctx_t *ctx, uint64_t entity, int64_t *out)
{
    ldg_math_phys_conserved_t *conserved_ptr = 0x0;
    void *comp_ptr = 0x0;
    int64_t quanta = 0;
    uint32_t err = 0;
    *out = 0;
    err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_CHARGE, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    quanta = (int64_t)conserved_ptr->magnitude.limb[0];
    if (conserved_ptr->sign) { quanta = 0 - quanta; }

    *out = quanta;
    return LDG_ERR_AOK;
}

static void ldg_math_phys_em_field_gather(const ldg_math_phys_region_t *region_ptr, const double *pos, double *out_electric, double *out_magnetic_flux)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(region_ptr);
    const double *coeff = ldg_math_phys_em_coeff_get(region_ptr);
    const double *electric = ldg_math_phys_em_electric_get(region_ptr);
    const double *magnetic = ldg_math_phys_em_magnetic_get(region_ptr);
    const uint16_t *mtl = ldg_math_phys_em_mtl_get(region_ptr);
    double offset[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double mu_val = 0.0;
    uint64_t cell[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint32_t comp_idx = 0;
    uint32_t axis = 0;
    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        cell[axis] = (uint64_t)(pos[axis] / region_ptr->desc.spacing + 0.5);
        if (cell[axis] > dim[axis] - 1) { cell[axis] = dim[axis] - 1; }
    }
    mu_val = coeff[(uint64_t)mtl[(cell[2] * hdr->ny + cell[1]) * hdr->nx + cell[0]] * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_MU];
    for (comp_idx = 0; comp_idx < 6; comp_idx++)
    {
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { offset[axis] = (comp_idx < LDG_MATH_PHYS_DIM) ? ((axis == comp_idx) ? 0.5 : 0.0) : ((axis == comp_idx - LDG_MATH_PHYS_DIM) ? 0.0 : 0.5); }
        if (comp_idx < LDG_MATH_PHYS_DIM) { out_electric[comp_idx] = ldg_math_phys_lattice_sample_get(electric + comp_idx * hdr->cell_cunt, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, pos, offset); }
        else { out_magnetic_flux[comp_idx - LDG_MATH_PHYS_DIM] = mu_val * ldg_math_phys_lattice_sample_get(magnetic + (comp_idx - LDG_MATH_PHYS_DIM) * hdr->cell_cunt, hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, pos, offset); }
    }
}

static uint32_t ldg_math_phys_step_em_couple_apply(ldg_math_phys_ctx_t *ctx, double dt)
{
    ldg_math_phys_body_motion_t motion = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t delta[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    ldg_math_phys_conserved_t neg_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t sum_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_math_phys_region_t *region_ptr = 0x0;
    double electric_val[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double flux_val[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double vel_minus[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double vel_prime[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double vel_plus[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double rot[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double rot_scale[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double impulse[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double half_qm = 0.0;
    double rot_sq = 0.0;
    double charge_val = 0.0;
    int64_t quanta = 0;
    uint64_t entity = 0;
    uint32_t err = 0;
    uint32_t dense_idx = 0;
    uint32_t axis = 0;
    for (dense_idx = 0; dense_idx < ctx->world->store.alive_cunt; dense_idx++)
    {
        entity = ctx->world->store.dense[dense_idx];
        if (ldg_ecs_ctx_comp_has(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION) != LDG_TRUTH_TRUE) { continue; }

        err = ldg_math_phys_body_motion_get(ctx, entity, &motion);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (!motion.massive) { continue; }

        region_ptr = &ctx->region[motion.region];
        if (region_ptr->desc.tier != LDG_MATH_PHYS_TIER_FIELD) { continue; }

        if (region_ptr->desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_EM) { continue; }

        if (region_ptr->field == 0x0) { continue; }

        err = ldg_math_phys_em_body_charge_get(ctx, entity, &quanta);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (quanta == 0) { continue; }

        charge_val = (double)quanta * LDG_MATH_PHYS_ELEM_CHARGE;
        ldg_math_phys_em_field_gather(region_ptr, motion.pos, electric_val, flux_val);
        half_qm = charge_val * dt / (2.0 * motion.mass);
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            vel_minus[axis] = motion.velocity[axis] + half_qm * electric_val[axis];
            rot[axis] = half_qm * flux_val[axis];
        }
        rot_sq = rot[0] * rot[0] + rot[1] * rot[1] + rot[2] * rot[2];
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { rot_scale[axis] = 2.0 * rot[axis] / (1.0 + rot_sq); }
        vel_prime[0] = vel_minus[0] + vel_minus[1] * rot[2] - vel_minus[2] * rot[1];
        vel_prime[1] = vel_minus[1] + vel_minus[2] * rot[0] - vel_minus[0] * rot[2];
        vel_prime[2] = vel_minus[2] + vel_minus[0] * rot[1] - vel_minus[1] * rot[0];
        vel_plus[0] = vel_minus[0] + vel_prime[1] * rot_scale[2] - vel_prime[2] * rot_scale[1];
        vel_plus[1] = vel_minus[1] + vel_prime[2] * rot_scale[0] - vel_prime[0] * rot_scale[2];
        vel_plus[2] = vel_minus[2] + vel_prime[0] * rot_scale[1] - vel_prime[1] * rot_scale[0];
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { impulse[axis] = motion.mass * (vel_plus[axis] + half_qm * electric_val[axis] - motion.velocity[axis]); }
        err = ldg_math_phys_momentum_quantize(region_ptr, impulse, delta);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_momentum_delta_apply(ctx, entity, delta, 0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        hdr = ldg_math_phys_em_hdr_get(region_ptr);
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            err = ldg_math_phys_conserved_neg(&delta[axis], &neg_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_math_phys_conserved_add(&hdr->momentum_account[axis], &neg_val, &sum_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            hdr->momentum_account[axis] = sum_val;
        }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_step_em_deposit_apply(ldg_math_phys_ctx_t *ctx, uint32_t frame_idx, double dt)
{
    ldg_math_phys_history_slot_t *slot_new = 0x0;
    ldg_math_phys_history_slot_t *slot_old = 0x0;
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_math_phys_region_t *region_ptr = 0x0;
    void *comp_ptr = 0x0;
    double *drive = 0x0;
    double shape_old[LDG_MATH_PHYS_DIM][4] = LDG_ARR_ZERO_INIT;
    double shape_new[LDG_MATH_PHYS_DIM][4] = LDG_ARR_ZERO_INIT;
    double shape_diff[LDG_MATH_PHYS_DIM][4] = LDG_ARR_ZERO_INIT;
    double face_cum = 0.0;
    double weight_val = 0.0;
    double coord_old = 0.0;
    double coord_new = 0.0;
    double node_val = 0.0;
    double charge_flow = 0.0;
    int64_t quanta = 0;
    int64_t base[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    int64_t node[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t entity = 0;
    uint64_t generation = 0;
    uint64_t slot_idx = 0;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint32_t prev_frame = 0;
    uint32_t err = 0;
    uint32_t dense_idx = 0;
    uint32_t axis = 0;
    uint32_t axis_b = 0;
    uint32_t axis_c = 0;
    uint32_t node_a = 0;
    uint32_t node_b = 0;
    uint32_t node_c = 0;
    if (ldg_math_phys_history_hdr_get(ctx)->cunt == 0) { return LDG_ERR_AOK; }

    prev_frame = (uint32_t)((frame_idx + ctx->conf.history_depth - 1) % ctx->conf.history_depth);
    for (dense_idx = 0; dense_idx < ctx->world->store.alive_cunt; dense_idx++)
    {
        entity = ctx->world->store.dense[dense_idx];
        if (ldg_ecs_ctx_comp_has(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION) != LDG_TRUTH_TRUE) { continue; }

        err = ldg_ecs_ctx_comp_get(ctx->world, entity, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (*(uint32_t *)comp_ptr >= ctx->conf.region_cap) { continue; }

        region_ptr = &ctx->region[*(uint32_t *)comp_ptr];
        if (!region_ptr->used || region_ptr->desc.tier != LDG_MATH_PHYS_TIER_FIELD) { continue; }

        if (region_ptr->desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_EM) { continue; }

        if (region_ptr->field == 0x0) { continue; }

        err = ldg_math_phys_em_body_charge_get(ctx, entity, &quanta);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (quanta == 0) { continue; }

        slot_idx = ldg_ecs_entity_idx_get(entity);
        generation = ldg_ecs_entity_generation_get(entity);
        slot_new = ldg_math_phys_history_slot_get(ctx, frame_idx) + slot_idx;
        slot_old = ldg_math_phys_history_slot_get(ctx, prev_frame) + slot_idx;
        if (!slot_new->used || slot_new->generation != generation) { continue; }

        if (!slot_old->used || slot_old->generation != generation) { continue; }

        hdr = ldg_math_phys_em_hdr_get(region_ptr);
        dim[0] = hdr->nx;
        dim[1] = hdr->ny;
        dim[2] = hdr->nz;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            coord_old = slot_old->pos[axis] / region_ptr->desc.spacing;
            coord_new = slot_new->pos[axis] / region_ptr->desc.spacing;
            if (LDG_UNLIKELY(ldg_math_abs_get(coord_new - coord_old) > 1.0)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

            base[axis] = (int64_t)ldg_math_floor_get(coord_old) - 1;
            for (node_a = 0; node_a < 4; node_a++)
            {
                node_val = (double)(base[axis] + (int64_t)node_a);
                shape_old[axis][node_a] = 1.0 - ldg_math_abs_get(coord_old - node_val);
                if (shape_old[axis][node_a] < 0.0) { shape_old[axis][node_a] = 0.0; }

                shape_new[axis][node_a] = 1.0 - ldg_math_abs_get(coord_new - node_val);
                if (shape_new[axis][node_a] < 0.0) { shape_new[axis][node_a] = 0.0; }

                shape_diff[axis][node_a] = shape_new[axis][node_a] - shape_old[axis][node_a];
            }
        }
        charge_flow = (double)quanta * LDG_MATH_PHYS_ELEM_CHARGE / (dt * region_ptr->desc.spacing * region_ptr->desc.spacing);
        drive = ldg_math_phys_em_drive_get(region_ptr);
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
        {
            axis_b = (axis + 1) % LDG_MATH_PHYS_DIM;
            axis_c = (axis + 2) % LDG_MATH_PHYS_DIM;
            for (node_b = 0; node_b < 4; node_b++)
            {
                for (node_c = 0; node_c < 4; node_c++)
                {
                    face_cum = 0.0;
                    for (node_a = 0; node_a < 4; node_a++)
                    {
                        weight_val = shape_diff[axis][node_a] * (shape_old[axis_b][node_b] * shape_old[axis_c][node_c] + 0.5 * shape_diff[axis_b][node_b] * shape_old[axis_c][node_c] + 0.5 * shape_old[axis_b][node_b] * shape_diff[axis_c][node_c] + shape_diff[axis_b][node_b] * shape_diff[axis_c][node_c] / 3.0);
                        face_cum -= charge_flow * weight_val;
                        node[axis] = base[axis] + (int64_t)node_a;
                        node[axis_b] = base[axis_b] + (int64_t)node_b;
                        node[axis_c] = base[axis_c] + (int64_t)node_c;
                        if (node[0] < 0 || node[1] < 0 || node[2] < 0) { continue; }

                        if ((uint64_t)node[0] > dim[0] - 1 || (uint64_t)node[1] > dim[1] - 1 || (uint64_t)node[2] > dim[2] - 1) { continue; }

                        drive[(uint64_t)axis * hdr->cell_cunt + ((uint64_t)node[2] * hdr->ny + (uint64_t)node[1]) * hdr->nx + (uint64_t)node[0]] += face_cum;
                    }
                }
            }
        }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_step(ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_history_hdr_t *hdr = 0x0;
    ldg_math_phys_conserved_t zero_val = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t next_clock = LDG_STRUCT_ZERO_INIT;
    double dt = 0.0;
    uint64_t frame_idx = 0;
    uint32_t err = 0;
    uint32_t idx = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    err = ldg_math_phys_dt_get(ctx, &dt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_step_tier_apply(ctx);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_history_hdr_get(ctx);
    frame_idx = hdr->hd;
    err = ldg_mem_secure_zero(ldg_math_phys_history_slot_get(ctx, (uint32_t)frame_idx), (uint64_t)ctx->world->conf.entity_cap * sizeof(ldg_math_phys_history_slot_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (idx = 0; idx < ctx->conf.region_cap; idx++)
    {
        if (!ctx->region[idx].used) { continue; }

        ctx->region[idx].total[LDG_MATH_PHYS_QUANTITY_ENERGY] = zero_val;
        for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { ctx->region[idx].total[LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_X + axis] = zero_val; }
    }
    err = ldg_math_phys_step_accel_apply(ctx, dt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_step_couple_apply(ctx, dt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_step_em_couple_apply(ctx, dt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_step_constraint_apply(ctx, dt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_step_body_advance(ctx, dt, (uint32_t)frame_idx);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_step_em_deposit_apply(ctx, (uint32_t)frame_idx, dt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_step_constraint_energy_cum(ctx);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_step_field_advance(ctx, dt, (uint32_t)frame_idx);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint128_add(&ctx->clock, &ctx->conf.tick_budget, &next_clock);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ctx->clock = next_clock;
    ldg_math_phys_history_meta_get(ctx, (uint32_t)frame_idx)->stamp = ctx->clock;
    hdr->hd = (hdr->hd + 1) % ctx->conf.history_depth;
    if (hdr->cunt < ctx->conf.history_depth) { hdr->cunt++; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_advance(ldg_math_phys_ctx_t *ctx, const ldg_uint128_t *until)
{
    ldg_math_phys_history_hdr_t *hdr = 0x0;
    ldg_uint128_t sim_span = LDG_STRUCT_ZERO_INIT;
    double wall_now = 0.0;
    double budget = 0.0;
    double sim_sec = 0.0;
    double raw = 0.0;
    uint32_t err = 0;
    uint32_t ord = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!until)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    hdr = ldg_math_phys_history_hdr_get(ctx);
    if (ctx->pacing > 0.0 && !hdr->anchor_set)
    {
        err = ldg_time_monotonic_get(&hdr->wall_anchor);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        hdr->clock_anchor = ctx->clock;
        hdr->anchor_set = 1;
    }

    for (;;)
    {
        err = ldg_arith_uint128_cmp(&ctx->clock, until, &ord);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (ord != LDG_ARITH_ORD_LT) { return LDG_ERR_AOK; }

        if (ctx->pacing > 0.0)
        {
            err = ldg_time_monotonic_get(&wall_now);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            budget = ctx->pacing * (wall_now - hdr->wall_anchor);
            err = ldg_arith_uint128_sub(&ctx->clock, &hdr->clock_anchor, &sim_span);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_arith_uint128_to_double_convert(&sim_span, &raw);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            sim_sec = ldg_math_pwr2_scale_get(raw, 0 - LDG_MATH_PHYS_TIME_FRACTION);
            if (sim_sec >= budget) { return LDG_ERR_AOK; }
        }

        err = ldg_math_phys_step(ctx);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
}

LDG_EXPORT uint32_t ldg_math_phys_region_policy_set(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_region_policy_t *desc)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->merge_dist) || desc->merge_dist < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->promote_mass) || desc->promote_mass < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->fragment_radius) || desc->fragment_radius < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->high_water < desc->low_water)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->fragment_cunt > LDG_MATH_PHYS_FRAGMENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->high_water > 0 && !(desc->merge_dist > 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (desc->promote_mass > 0.0)
    {
        if (LDG_UNLIKELY(desc->fragment_cunt < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!(desc->fragment_radius > 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(desc->low_water < desc->fragment_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }
    }

    ctx->region[region].policy = *desc;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_tier_pin_set(ldg_math_phys_ctx_t *ctx, uint64_t body, uint32_t tier)
{
    void *comp_ptr = 0x0;
    uint64_t category = 0;
    uint32_t err = 0;
    uint32_t region = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(tier > LDG_MATH_PHYS_TIER_CUNT)) { return LDG_ERR_MATH_PHYS_TIER_INVALID; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(&ctx->world->store, body) != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    if (tier == LDG_MATH_PHYS_TIER_CUNT)
    {
        category = ldg_ecs_ctx_entity_category_get(ctx->world, body);
        return ldg_ecs_ctx_entity_category_set(ctx->world, body, (uint32_t)category & ~LDG_MATH_PHYS_PIN_CATEGORY);
    }

    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region = *(uint32_t *)comp_ptr;
    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(tier != ctx->region[region].desc.tier)) { return LDG_ERR_MATH_PHYS_TIER_UNSATISFIED; }

    category = ldg_ecs_ctx_entity_category_get(ctx->world, body);
    return ldg_ecs_ctx_entity_category_set(ctx->world, body, (uint32_t)category | LDG_MATH_PHYS_PIN_CATEGORY);
}

LDG_EXPORT uint32_t ldg_math_phys_body_region_set(ldg_math_phys_ctx_t *ctx, uint64_t body, uint32_t region, const double *pos, uint64_t *out)
{
    ldg_math_phys_body_val_t body_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_tag_t tag_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t rescaled = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_conserved_t *conserved_ptr = 0x0;
    const ldg_math_phys_region_t *src_ptr = 0x0;
    const ldg_math_phys_region_t *tgt_ptr = 0x0;
    void *comp_ptr = 0x0;
    uint64_t category = 0;
    uint64_t moved = 0;
    int64_t delta = 0;
    uint32_t src_region = 0;
    uint32_t kind = 0;
    uint32_t err = 0;
    uint32_t rollback_err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(&ctx->world->store, body) != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    tgt_ptr = &ctx->region[region];
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    src_region = *(uint32_t *)comp_ptr;
    if (LDG_UNLIKELY(src_region >= ctx->conf.region_cap || !ctx->region[src_region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    src_ptr = &ctx->region[src_region];
    category = ldg_ecs_ctx_entity_category_get(ctx->world, body);
    if (LDG_UNLIKELY((category & LDG_MATH_PHYS_PIN_CATEGORY) != 0 && tgt_ptr->desc.tier != src_ptr->desc.tier)) { return LDG_ERR_MATH_PHYS_TIER_UNSATISFIED; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(pos[axis]) || pos[axis] < 0.0 || pos[axis] >= tgt_ptr->desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; } }
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_KIND, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    kind = *(uint32_t *)comp_ptr;
    if (ldg_ecs_ctx_comp_has(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_TAG) == LDG_TRUTH_TRUE)
    {
        err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_TAG, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        tag_val = *(ldg_math_phys_tag_t *)comp_ptr;
    }

    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MASS, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    delta = ldg_math_phys_region_exponent_get(tgt_ptr, LDG_MATH_PHYS_QUANTITY_MASS) - ldg_math_phys_region_exponent_get(src_ptr, LDG_MATH_PHYS_QUANTITY_MASS);
    err = ldg_math_phys_conserved_rescale((ldg_math_phys_conserved_t *)comp_ptr, delta, &body_val.mass_c);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_ENERGY, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    delta = ldg_math_phys_region_exponent_get(tgt_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY) - ldg_math_phys_region_exponent_get(src_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY);
    err = ldg_math_phys_conserved_rescale((ldg_math_phys_conserved_t *)comp_ptr, delta, &body_val.energy_c);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MOMENTUM, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    delta = ldg_math_phys_region_exponent_get(tgt_ptr, LDG_MATH_PHYS_QUANTITY_MOMENTUM_X) - ldg_math_phys_region_exponent_get(src_ptr, LDG_MATH_PHYS_QUANTITY_MOMENTUM_X);
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_math_phys_conserved_rescale(&conserved_ptr[axis], delta, &rescaled);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        body_val.momentum_c[axis] = rescaled;
    }
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_ANGULAR_MOMENTUM, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    delta = ldg_math_phys_region_exponent_get(tgt_ptr, LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_X) - ldg_math_phys_region_exponent_get(src_ptr, LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_X);
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_math_phys_conserved_rescale(&conserved_ptr[axis], delta, &rescaled);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        body_val.angular_c[axis] = rescaled;
    }
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_CHARGE, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    body_val.charge_c = *(ldg_math_phys_conserved_t *)comp_ptr;
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_ORIENTATION, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_QUATERNION_DIM; axis++) { body_val.orientation[axis] = ((double *)comp_ptr)[axis]; }
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_SPIN, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { body_val.spin[axis] = ((double *)comp_ptr)[axis]; }
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_INERTIA, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    body_val.inertia = *(double *)comp_ptr;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_arith_double_to_uint128_convert(ldg_math_pwr2_scale_get(pos[axis], LDG_MATH_PHYS_POS_FRACTION), &body_val.pos_fixed[axis]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    err = ldg_math_phys_body_wr(ctx, region, kind, &body_val, &tag_val, &moved);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_ecs_ctx_entity_category_set(ctx->world, moved, (uint32_t)category);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        rollback_err = ldg_math_phys_body_destroy(ctx, moved);
        if (rollback_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = rollback_err; }

        return err;
    }

    err = ldg_math_phys_body_destroy(ctx, body);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        rollback_err = ldg_math_phys_body_destroy(ctx, moved);
        if (rollback_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = rollback_err; }

        return err;
    }

    *out = moved;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_body_dissolve(ldg_math_phys_ctx_t *ctx, uint64_t body)
{
    ldg_math_phys_conserved_t *conserved_ptr = 0x0;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    ldg_uint128_t *pos_ptr = 0x0;
    void *comp_ptr = 0x0;
    double pos[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double energy_val = 0.0;
    double dt = 0.0;
    double raw = 0.0;
    uint32_t region = 0;
    uint32_t err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ldg_ecs_entity_alive_is(&ctx->world->store, body) != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY((ldg_ecs_ctx_entity_category_get(ctx->world, body) & LDG_MATH_PHYS_PIN_CATEGORY) != 0)) { return LDG_ERR_MATH_PHYS_DEMOTE_DENIED; }

    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_REGION, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region = *(uint32_t *)comp_ptr;
    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    region_ptr = &ctx->region[region];
    if (LDG_UNLIKELY(region_ptr->desc.tier != LDG_MATH_PHYS_TIER_FIELD)) { return LDG_ERR_MATH_PHYS_TIER_INVALID; }

    if (LDG_UNLIKELY(region_ptr->desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_SCALAR)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(region_ptr->field == 0x0)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MASS, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    if (LDG_UNLIKELY((conserved_ptr->magnitude.limb[0] | conserved_ptr->magnitude.limb[1] | conserved_ptr->magnitude.limb[2] | conserved_ptr->magnitude.limb[3]) != 0)) { return LDG_ERR_MATH_PHYS_CONSERVATION; }

    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_CHARGE, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    if (LDG_UNLIKELY((conserved_ptr->magnitude.limb[0] | conserved_ptr->magnitude.limb[1] | conserved_ptr->magnitude.limb[2] | conserved_ptr->magnitude.limb[3]) != 0)) { return LDG_ERR_MATH_PHYS_CONSERVATION_CHARGE; }

    if (ldg_ecs_ctx_comp_has(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_TAG) == LDG_TRUTH_TRUE)
    {
        err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_TAG, &comp_ptr);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (LDG_UNLIKELY(((ldg_math_phys_tag_t *)comp_ptr)->lepton != 0 || ((ldg_math_phys_tag_t *)comp_ptr)->baryon != 0)) { return LDG_ERR_MATH_PHYS_CONSERVATION_TAG; }
    }

    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_MOMENTUM, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY((conserved_ptr[axis].magnitude.limb[0] | conserved_ptr[axis].magnitude.limb[1] | conserved_ptr[axis].magnitude.limb[2] | conserved_ptr[axis].magnitude.limb[3]) != 0)) { return LDG_ERR_MATH_PHYS_CONSERVATION_MOMENTUM; } }
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_ANGULAR_MOMENTUM, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    conserved_ptr = (ldg_math_phys_conserved_t *)comp_ptr;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY((conserved_ptr[axis].magnitude.limb[0] | conserved_ptr[axis].magnitude.limb[1] | conserved_ptr[axis].magnitude.limb[2] | conserved_ptr[axis].magnitude.limb[3]) != 0)) { return LDG_ERR_MATH_PHYS_CONSERVATION_ANGULAR_MOMENTUM; } }
    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_ENERGY, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_conserved_to_double_convert((ldg_math_phys_conserved_t *)comp_ptr, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY), &energy_val);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_ecs_ctx_comp_get(ctx->world, body, ctx->conf.comp_id_base + LDG_MATH_PHYS_COMP_POS, &comp_ptr);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    pos_ptr = (ldg_uint128_t *)comp_ptr;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_arith_uint128_to_double_convert(&pos_ptr[axis], &raw);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        pos[axis] = ldg_math_pwr2_scale_get(raw, 0 - LDG_MATH_PHYS_POS_FRACTION);
    }
    err = ldg_math_phys_dt_get(ctx, &dt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (energy_val > 0.0)
    {
        err = ldg_math_phys_field_point_energy_xfer(region_ptr, pos, energy_val, dt, 0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    return ldg_math_phys_body_destroy(ctx, body);
}

LDG_EXPORT uint32_t ldg_math_phys_body_condense(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, double energy, uint64_t *out)
{
    ldg_math_phys_body_val_t body_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_tag_t tag_val = LDG_STRUCT_ZERO_INIT;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    double dt = 0.0;
    uint32_t err = 0;
    uint32_t rollback_err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap || !ctx->region[region].used)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    region_ptr = &ctx->region[region];
    if (LDG_UNLIKELY(region_ptr->desc.tier != LDG_MATH_PHYS_TIER_FIELD)) { return LDG_ERR_MATH_PHYS_TIER_INVALID; }

    if (LDG_UNLIKELY(region_ptr->desc.field_kind != LDG_MATH_PHYS_FIELD_KIND_SCALAR)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(region_ptr->field == 0x0)) { return LDG_ERR_MATH_PHYS_REGION_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(energy) || energy < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(pos[axis]) || pos[axis] < 0.0 || pos[axis] >= region_ptr->desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; } }
    err = ldg_math_phys_dt_get(ctx, &dt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (energy > 0.0)
    {
        err = ldg_math_phys_field_point_energy_xfer(region_ptr, pos, energy, dt, 1);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    err = ldg_math_phys_double_to_conserved_convert(energy, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY), &body_val.energy_c);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    body_val.orientation[LDG_MATH_QUATERNION_DIM - 1] = 1.0;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        err = ldg_arith_double_to_uint128_convert(ldg_math_pwr2_scale_get(pos[axis], LDG_MATH_PHYS_POS_FRACTION), &body_val.pos_fixed[axis]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    err = ldg_math_phys_body_wr(ctx, region, LDG_MATH_PHYS_PARTICLE_PHOTON, &body_val, &tag_val, out);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK))
    {
        if (energy > 0.0)
        {
            rollback_err = ldg_math_phys_field_point_energy_xfer(region_ptr, pos, energy, dt, 0);
            if (rollback_err != LDG_ERR_AOK && err == LDG_ERR_AOK) { err = rollback_err; }
        }

        return err;
    }

    return LDG_ERR_AOK;
}

static ldg_math_phys_atmosphere_hdr_t* ldg_math_phys_atmosphere_hdr_get(const ldg_math_phys_ctx_t *ctx)
{
    return (ldg_math_phys_atmosphere_hdr_t *)ctx->atmosphere;
}

static uint64_t ldg_math_phys_atmosphere_grid_cunt_get(const ldg_math_phys_atmosphere_hdr_t *hdr)
{
    return hdr->altitude_cunt * hdr->view_cunt * hdr->sun_cunt * hdr->view_sun_cunt;
}

static double* ldg_math_phys_atmosphere_table_rayleigh_get(const ldg_math_phys_ctx_t *ctx)
{
    return (double *)((uint8_t *)ctx->atmosphere + sizeof(ldg_math_phys_atmosphere_hdr_t));
}

static double* ldg_math_phys_atmosphere_table_ozone_get(const ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    return ldg_math_phys_atmosphere_table_rayleigh_get(ctx) + hdr->bin_cunt;
}

static double* ldg_math_phys_atmosphere_table_sun_irradiance_get(const ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    return ldg_math_phys_atmosphere_table_ozone_get(ctx) + hdr->bin_cunt;
}

static double* ldg_math_phys_atmosphere_table_albedo_get(const ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    return ldg_math_phys_atmosphere_table_sun_irradiance_get(ctx) + hdr->bin_cunt;
}

static double* ldg_math_phys_atmosphere_table_transmittance_get(const ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    return ldg_math_phys_atmosphere_table_albedo_get(ctx) + hdr->bin_cunt;
}

static double* ldg_math_phys_atmosphere_table_delta_rayleigh_get(const ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    return ldg_math_phys_atmosphere_table_transmittance_get(ctx) + hdr->altitude_cunt * hdr->view_cunt * hdr->bin_cunt;
}

static double* ldg_math_phys_atmosphere_table_delta_mie_get(const ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    return ldg_math_phys_atmosphere_table_delta_rayleigh_get(ctx) + ldg_math_phys_atmosphere_grid_cunt_get(hdr) * hdr->bin_cunt;
}

static double* ldg_math_phys_atmosphere_table_delta_fog_get(const ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    return ldg_math_phys_atmosphere_table_delta_mie_get(ctx) + ldg_math_phys_atmosphere_grid_cunt_get(hdr) * hdr->bin_cunt;
}

static double* ldg_math_phys_atmosphere_table_multi_get(const ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    return ldg_math_phys_atmosphere_table_delta_fog_get(ctx) + ldg_math_phys_atmosphere_grid_cunt_get(hdr) * hdr->bin_cunt;
}

static double* ldg_math_phys_atmosphere_table_ground_irradiance_get(const ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    return ldg_math_phys_atmosphere_table_multi_get(ctx) + ldg_math_phys_atmosphere_grid_cunt_get(hdr) * hdr->bin_cunt;
}

static double* ldg_math_phys_atmosphere_table_delta_irradiance_get(const ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    return ldg_math_phys_atmosphere_table_ground_irradiance_get(ctx) + hdr->sun_cunt * hdr->bin_cunt;
}

static double* ldg_math_phys_atmosphere_table_gather_get(const ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    return ldg_math_phys_atmosphere_table_delta_irradiance_get(ctx) + hdr->sun_cunt * hdr->bin_cunt;
}

static double* ldg_math_phys_atmosphere_table_ping_get(const ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    return ldg_math_phys_atmosphere_table_gather_get(ctx) + ldg_math_phys_atmosphere_grid_cunt_get(hdr) * hdr->bin_cunt;
}

static double* ldg_math_phys_atmosphere_table_pong_get(const ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    return ldg_math_phys_atmosphere_table_ping_get(ctx) + ldg_math_phys_atmosphere_grid_cunt_get(hdr) * hdr->bin_cunt;
}

static void ldg_math_phys_atmosphere_table_set(const ldg_math_phys_ctx_t *ctx, ldg_math_phys_atmosphere_table_t *table)
{
    table->hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    table->rayleigh = ldg_math_phys_atmosphere_table_rayleigh_get(ctx);
    table->ozone = ldg_math_phys_atmosphere_table_ozone_get(ctx);
    table->sun_irradiance = ldg_math_phys_atmosphere_table_sun_irradiance_get(ctx);
    table->albedo = ldg_math_phys_atmosphere_table_albedo_get(ctx);
    table->transmittance = ldg_math_phys_atmosphere_table_transmittance_get(ctx);
    table->delta_rayleigh = ldg_math_phys_atmosphere_table_delta_rayleigh_get(ctx);
    table->delta_mie = ldg_math_phys_atmosphere_table_delta_mie_get(ctx);
    table->delta_fog = ldg_math_phys_atmosphere_table_delta_fog_get(ctx);
    table->multi = ldg_math_phys_atmosphere_table_multi_get(ctx);
    table->ground_irradiance = ldg_math_phys_atmosphere_table_ground_irradiance_get(ctx);
}

static uint32_t ldg_math_phys_atmosphere_size_get(const ldg_math_phys_atmosphere_desc_t *desc, uint64_t *out)
{
    uint64_t grid = 0;
    uint64_t term = 0;
    uint64_t double_cunt = 0;
    uint64_t bytes = 0;
    uint32_t err = 0;
    *out = 0;
    err = ldg_arith_uint64_mul((uint64_t)desc->altitude_cunt, (uint64_t)desc->view_cunt, &grid);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(grid, (uint64_t)desc->sun_cunt, &grid);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(grid, (uint64_t)desc->view_sun_cunt, &grid);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    double_cunt = 4 * (uint64_t)desc->bin_cunt + 2 * (uint64_t)desc->sun_cunt * (uint64_t)desc->bin_cunt;
    err = ldg_arith_uint64_mul((uint64_t)desc->altitude_cunt * (uint64_t)desc->view_cunt, (uint64_t)desc->bin_cunt, &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(double_cunt, term, &double_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(grid, (uint64_t)desc->bin_cunt, &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(term, 7, &term);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(double_cunt, term, &double_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_mul(double_cunt, sizeof(double), &bytes);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint64_add(bytes, sizeof(ldg_math_phys_atmosphere_hdr_t), &bytes);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    *out = bytes;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_atmosphere_set(ldg_math_phys_ctx_t *ctx, const ldg_math_phys_atmosphere_desc_t *desc)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = 0x0;
    void *blk = 0x0;
    double *dst = 0x0;
    uint64_t size = 0;
    uint64_t bin = 0;
    uint32_t err = 0;
    uint8_t reuse = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(!desc->rayleigh_scatter || !desc->sun_irradiance || !desc->ground_albedo)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->planet_radius) || desc->planet_radius <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->thickness) || desc->thickness <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->rayleigh_scale_height) || desc->rayleigh_scale_height <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->mie_scale_height) || desc->mie_scale_height <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->mie_scatter) || desc->mie_scatter < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->mie_absorb) || desc->mie_absorb < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->mie_anisotropy) || desc->mie_anisotropy <= (0.0 - 1.0) || desc->mie_anisotropy >= 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->fog_scatter) || desc->fog_scatter < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->fog_absorb) || desc->fog_absorb < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (desc->fog_scatter > 0.0 || desc->fog_absorb > 0.0)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->fog_scale_height) || desc->fog_scale_height <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->fog_anisotropy) || desc->fog_anisotropy <= (0.0 - 1.0) || desc->fog_anisotropy >= 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }
    }
    else { if (LDG_UNLIKELY(ldg_math_abs_get(desc->fog_scale_height) > 0.0 || ldg_math_abs_get(desc->fog_anisotropy) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; } }

    if (desc->ozone_absorb != 0x0)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->ozone_center_altitude) || desc->ozone_center_altitude < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->ozone_half_width) || desc->ozone_half_width <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }
    }
    else { if (LDG_UNLIKELY(ldg_math_abs_get(desc->ozone_center_altitude) > 0.0 || ldg_math_abs_get(desc->ozone_half_width) > 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; } }

    if (LDG_UNLIKELY(desc->bin_cunt == 0 || desc->bin_cunt > LDG_MATH_PHYS_ATMOSPHERE_BIN_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->order_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->altitude_cunt < 2 || desc->view_cunt < 2 || desc->sun_cunt < 2 || desc->view_sun_cunt < 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (bin = 0; bin < desc->bin_cunt; bin++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->rayleigh_scatter[bin]) || desc->rayleigh_scatter[bin] < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->sun_irradiance[bin]) || desc->sun_irradiance[bin] < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->ground_albedo[bin]) || desc->ground_albedo[bin] < 0.0 || desc->ground_albedo[bin] > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(desc->ozone_absorb != 0x0 && (!ldg_math_finite_is(desc->ozone_absorb[bin]) || desc->ozone_absorb[bin] < 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }
    }
    err = ldg_math_phys_atmosphere_size_get(desc, &size);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (ctx->atmosphere != 0x0)
    {
        hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
        reuse = hdr->bin_cunt == desc->bin_cunt && hdr->altitude_cunt == desc->altitude_cunt && hdr->view_cunt == desc->view_cunt && hdr->sun_cunt == desc->sun_cunt && hdr->view_sun_cunt == desc->view_sun_cunt;
        if (!reuse)
        {
            if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_DENIED; }

            err = ldg_mem_dealloc(ctx->atmosphere);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            ctx->atmosphere = 0x0;
        }
    }

    if (ctx->atmosphere == 0x0)
    {
        if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_DENIED; }

        err = ldg_mem_alloc(size, &blk);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        ctx->atmosphere = blk;
        err = ldg_mem_secure_zero(ctx->atmosphere, size);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }

    hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    hdr->planet_radius = desc->planet_radius;
    hdr->thickness = desc->thickness;
    hdr->rayleigh_scale_height = desc->rayleigh_scale_height;
    hdr->mie_scale_height = desc->mie_scale_height;
    hdr->mie_scatter = desc->mie_scatter;
    hdr->mie_absorb = desc->mie_absorb;
    hdr->mie_anisotropy = desc->mie_anisotropy;
    hdr->fog_scale_height = desc->fog_scale_height;
    hdr->fog_scatter = desc->fog_scatter;
    hdr->fog_absorb = desc->fog_absorb;
    hdr->fog_anisotropy = desc->fog_anisotropy;
    hdr->ozone_center_altitude = desc->ozone_center_altitude;
    hdr->ozone_half_width = desc->ozone_half_width;
    hdr->bin_cunt = desc->bin_cunt;
    hdr->order_cunt = desc->order_cunt;
    hdr->altitude_cunt = desc->altitude_cunt;
    hdr->view_cunt = desc->view_cunt;
    hdr->sun_cunt = desc->sun_cunt;
    hdr->view_sun_cunt = desc->view_sun_cunt;
    hdr->solved = 0;
    dst = ldg_math_phys_atmosphere_table_rayleigh_get(ctx);
    for (bin = 0; bin < hdr->bin_cunt; bin++) { dst[bin] = desc->rayleigh_scatter[bin]; }
    dst = ldg_math_phys_atmosphere_table_ozone_get(ctx);
    for (bin = 0; bin < hdr->bin_cunt; bin++) { dst[bin] = (desc->ozone_absorb != 0x0) ? desc->ozone_absorb[bin] : 0.0; }
    dst = ldg_math_phys_atmosphere_table_sun_irradiance_get(ctx);
    for (bin = 0; bin < hdr->bin_cunt; bin++) { dst[bin] = desc->sun_irradiance[bin]; }
    dst = ldg_math_phys_atmosphere_table_albedo_get(ctx);
    for (bin = 0; bin < hdr->bin_cunt; bin++) { dst[bin] = desc->ground_albedo[bin]; }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_acoustic_compute_info_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, ldg_math_phys_acoustic_compute_info_t *out)
{
    ldg_math_phys_acoustic_hdr_t *hdr = 0x0;
    const ldg_math_phys_region_t *region_ptr = 0x0;
    ldg_math_phys_acoustic_monopole_t *monopole = 0x0;
    uint64_t idx = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_acoustic_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_ptr = &ctx->region[region];
    hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    out->pressure = ldg_math_phys_acoustic_pressure_get(region_ptr);
    out->velocity = ldg_math_phys_acoustic_velocity_get(region_ptr);
    out->drive = ldg_math_phys_acoustic_drive_get(region_ptr);
    out->psi = ldg_math_phys_acoustic_psi_get(region_ptr);
    out->mtl = ldg_math_phys_acoustic_mtl_get(region_ptr);
    out->coeff = ldg_math_phys_acoustic_coeff_get(region_ptr);
    out->stretch = ldg_math_phys_acoustic_stretch_get(region_ptr);
    out->spacing = region_ptr->desc.spacing;
    out->dt_sub = hdr->dt_sub;
    out->nx = hdr->nx;
    out->ny = hdr->ny;
    out->nz = hdr->nz;
    out->cell_cunt = hdr->cell_cunt;
    out->mtl_cunt = hdr->mtl_cunt;
    out->pml_depth = hdr->pml_depth;
    out->tick_cuntr = hdr->tick_cuntr;
    out->subcycle_cunt = hdr->subcycle_cunt;
    out->face_cunt = hdr->face_cunt;
    out->wall_cunt = hdr->wall_cunt;
    out->plane_armed = hdr->plane_armed;
    monopole = ldg_math_phys_acoustic_monopole_get(region_ptr);
    out->mono_cunt = hdr->monopole_cunt;
    for (idx = 0; idx < hdr->monopole_cunt && idx < LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX; idx++)
    {
        out->mono_body[idx] = monopole[idx].body;
        out->mono_tick_start[idx] = monopole[idx].tick_start;
        out->mono_series[idx] = monopole[idx].series;
        out->mono_series_cunt[idx] = monopole[idx].series_cunt;
        out->mono_amplitude[idx] = monopole[idx].amplitude;
        out->mono_freq[idx] = monopole[idx].freq;
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_region_kind_get(const ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    if (LDG_UNLIKELY(!ctx || ctx->is_init != LDG_TRUTH_TRUE)) { return UINT32_MAX; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap)) { return UINT32_MAX; }

    return ctx->region[region].desc.field_kind;
}

uint8_t ldg_math_phys_region_step_skip_is(const ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(region >= ctx->conf.region_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (ctx->region[region].desc.step_skip) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_math_phys_acoustic_body_pos_get(const ldg_math_phys_ctx_t *ctx, uint64_t body, double *out_pos, uint32_t *out_region)
{
    ldg_math_phys_body_motion_t motion = LDG_STRUCT_ZERO_INIT;
    uint32_t err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx || !out_pos || !out_region)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_region = UINT32_MAX;
    err = ldg_math_phys_body_motion_get(ctx, body, &motion);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { out_pos[axis] = motion.pos[axis]; }

    *out_region = motion.region;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_atmosphere_compute_info_get(const ldg_math_phys_ctx_t *ctx, ldg_math_phys_atmosphere_compute_info_t *out)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = 0x0;
    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->atmosphere == 0x0)) { return LDG_ERR_NOT_INIT; }

    hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    out->hdr = hdr;
    out->rayleigh = ldg_math_phys_atmosphere_table_rayleigh_get(ctx);
    out->ozone = ldg_math_phys_atmosphere_table_ozone_get(ctx);
    out->sun_irradiance = ldg_math_phys_atmosphere_table_sun_irradiance_get(ctx);
    out->albedo = ldg_math_phys_atmosphere_table_albedo_get(ctx);
    out->transmittance = ldg_math_phys_atmosphere_table_transmittance_get(ctx);
    out->delta_rayleigh = ldg_math_phys_atmosphere_table_delta_rayleigh_get(ctx);
    out->delta_mie = ldg_math_phys_atmosphere_table_delta_mie_get(ctx);
    out->delta_fog = ldg_math_phys_atmosphere_table_delta_fog_get(ctx);
    out->multi = ldg_math_phys_atmosphere_table_multi_get(ctx);
    out->ground = ldg_math_phys_atmosphere_table_ground_irradiance_get(ctx);
    out->delta_irradiance = ldg_math_phys_atmosphere_table_delta_irradiance_get(ctx);
    out->gather = ldg_math_phys_atmosphere_table_gather_get(ctx);
    out->ping = ldg_math_phys_atmosphere_table_ping_get(ctx);
    out->pong = ldg_math_phys_atmosphere_table_pong_get(ctx);
    out->grid_total = ldg_math_phys_atmosphere_grid_cunt_get(hdr) * hdr->bin_cunt;
    out->trans_total = hdr->altitude_cunt * hdr->view_cunt * hdr->bin_cunt;
    out->ground_total = hdr->sun_cunt * hdr->bin_cunt;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_atmosphere_cpu_solve(ldg_math_phys_ctx_t *ctx)
{
    ldg_math_phys_atmosphere_table_t table = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_atmosphere_hdr_t *hdr = 0x0;
    double *multi = 0x0;
    double *ground = 0x0;
    double *delta_irradiance = 0x0;
    double *gather = 0x0;
    double *ping = 0x0;
    double *pong = 0x0;
    double *prev = 0x0;
    double *cur = 0x0;
    uint64_t grid_total = 0;
    uint64_t ground_total = 0;
    uint64_t idx = 0;
    uint64_t order = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->atmosphere == 0x0)) { return LDG_ERR_NOT_INIT; }

    hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    ldg_math_phys_atmosphere_table_set(ctx, &table);
    multi = ldg_math_phys_atmosphere_table_multi_get(ctx);
    ground = ldg_math_phys_atmosphere_table_ground_irradiance_get(ctx);
    delta_irradiance = ldg_math_phys_atmosphere_table_delta_irradiance_get(ctx);
    gather = ldg_math_phys_atmosphere_table_gather_get(ctx);
    ping = ldg_math_phys_atmosphere_table_ping_get(ctx);
    pong = ldg_math_phys_atmosphere_table_pong_get(ctx);
    grid_total = ldg_math_phys_atmosphere_grid_cunt_get(hdr) * hdr->bin_cunt;
    ground_total = hdr->sun_cunt * hdr->bin_cunt;
    err = ldg_math_phys_atmosphere_transmittance_fill(hdr, table.rayleigh, table.ozone, ldg_math_phys_atmosphere_table_transmittance_get(ctx));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_atmosphere_single_fill(hdr, table.rayleigh, table.ozone, table.sun_irradiance, table.transmittance, ldg_math_phys_atmosphere_table_delta_rayleigh_get(ctx), ldg_math_phys_atmosphere_table_delta_mie_get(ctx), ldg_math_phys_atmosphere_table_delta_fog_get(ctx));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (idx = 0; idx < grid_total; idx++) { multi[idx] = 0.0; }
    for (idx = 0; idx < ground_total; idx++) { ground[idx] = 0.0; }
    err = ldg_math_phys_atmosphere_irradiance_fill(&table, 0x0, 0, delta_irradiance);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    cur = ping;
    for (order = 2; order <= hdr->order_cunt; order++)
    {
        err = ldg_math_phys_atmosphere_gather_fill(&table, prev, delta_irradiance, gather);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_phys_atmosphere_irradiance_fill(&table, prev, order - 1, delta_irradiance);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        for (idx = 0; idx < ground_total; idx++) { ground[idx] += delta_irradiance[idx]; }
        err = ldg_math_phys_atmosphere_order_fill(hdr, table.rayleigh, table.ozone, gather, cur);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        for (idx = 0; idx < grid_total; idx++) { multi[idx] += cur[idx]; }
        prev = cur;
        cur = prev == ping ? pong : ping;
    }
    err = ldg_math_phys_atmosphere_irradiance_fill(&table, prev, hdr->order_cunt, delta_irradiance);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (idx = 0; idx < ground_total; idx++) { ground[idx] += delta_irradiance[idx]; }
    hdr->solved = 1;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_atmosphere_solve(ldg_math_phys_ctx_t *ctx)
{
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->atmosphere == 0x0)) { return LDG_ERR_NOT_INIT; }

    err = ldg_math_phys_atmosphere_compute_solve(ctx);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ldg_math_phys_atmosphere_hdr_get(ctx)->solved = 1;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_atmosphere_transmittance_get(const ldg_math_phys_ctx_t *ctx, double altitude, double zenith_cos, double *out)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = 0x0;
    uint64_t bin = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->atmosphere == 0x0)) { return LDG_ERR_NOT_INIT; }

    hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    for (bin = 0; bin < hdr->bin_cunt; bin++) { out[bin] = 0.0; }
    if (LDG_UNLIKELY(!hdr->solved)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(altitude) || altitude < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(zenith_cos) || zenith_cos > 1.0 || zenith_cos < (0.0 - 1.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (altitude > hdr->thickness) { altitude = hdr->thickness; }

    return ldg_math_phys_atmosphere_transmittance_sample_get(hdr, ldg_math_phys_atmosphere_table_transmittance_get(ctx), altitude, zenith_cos, out);
}

LDG_EXPORT uint32_t ldg_math_phys_atmosphere_radiance_get(const ldg_math_phys_ctx_t *ctx, double altitude, double view_cos, double sun_cos, double view_sun_cos, double *out)
{
    ldg_math_phys_atmosphere_table_t table = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_atmosphere_hdr_t *hdr = 0x0;
    uint64_t bin = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->atmosphere == 0x0)) { return LDG_ERR_NOT_INIT; }

    hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    for (bin = 0; bin < hdr->bin_cunt; bin++) { out[bin] = 0.0; }
    if (LDG_UNLIKELY(!hdr->solved)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(altitude) || altitude < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(view_cos) || view_cos > 1.0 || view_cos < (0.0 - 1.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(sun_cos) || sun_cos > 1.0 || sun_cos < (0.0 - 1.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(view_sun_cos) || view_sun_cos > 1.0 || view_sun_cos < (0.0 - 1.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (altitude > hdr->thickness) { altitude = hdr->thickness; }

    ldg_math_phys_atmosphere_table_set(ctx, &table);
    return ldg_math_phys_atmosphere_radiance_sample_get(&table, altitude, view_cos, sun_cos, view_sun_cos, out);
}

LDG_EXPORT uint32_t ldg_math_phys_atmosphere_irradiance_get(const ldg_math_phys_ctx_t *ctx, double altitude, double sun_cos, double normal_cos, double normal_sun_cos, double *out)
{
    ldg_math_phys_atmosphere_table_t table = LDG_STRUCT_ZERO_INIT;
    ldg_math_phys_atmosphere_hdr_t *hdr = 0x0;
    uint64_t bin = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->atmosphere == 0x0)) { return LDG_ERR_NOT_INIT; }

    hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    for (bin = 0; bin < hdr->bin_cunt; bin++) { out[bin] = 0.0; }
    if (LDG_UNLIKELY(!hdr->solved)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(altitude) || altitude < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(sun_cos) || sun_cos > 1.0 || sun_cos < (0.0 - 1.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(normal_cos) || normal_cos > 1.0 || normal_cos < (0.0 - 1.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(normal_sun_cos) || normal_sun_cos > 1.0 || normal_sun_cos < (0.0 - 1.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (altitude > hdr->thickness) { altitude = hdr->thickness; }

    ldg_math_phys_atmosphere_table_set(ctx, &table);
    return ldg_math_phys_atmosphere_irradiance_sample_get(&table, altitude, sun_cos, normal_cos, normal_sun_cos, out);
}

LDG_EXPORT uint32_t ldg_math_phys_atmosphere_extinction_get(const ldg_math_phys_ctx_t *ctx, double altitude, double *out)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = 0x0;
    uint64_t bin = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->atmosphere == 0x0)) { return LDG_ERR_NOT_INIT; }

    hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    for (bin = 0; bin < hdr->bin_cunt; bin++) { out[bin] = 0.0; }
    if (LDG_UNLIKELY(!ldg_math_finite_is(altitude) || altitude < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (altitude > hdr->thickness) { altitude = hdr->thickness; }

    return ldg_math_phys_atmosphere_extinction_sample_get(hdr, ldg_math_phys_atmosphere_table_rayleigh_get(ctx), ldg_math_phys_atmosphere_table_ozone_get(ctx), altitude, out);
}

LDG_EXPORT uint32_t ldg_math_phys_atmosphere_medium_get(const ldg_math_phys_ctx_t *ctx, uint32_t medium, double altitude, double *out_scatter, double *out_absorb, double *out_anisotropy)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = 0x0;
    uint64_t bin = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_scatter)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_absorb)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_anisotropy)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->atmosphere == 0x0)) { return LDG_ERR_NOT_INIT; }

    hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    *out_anisotropy = 0.0;
    for (bin = 0; bin < hdr->bin_cunt; bin++)
    {
        out_scatter[bin] = 0.0;
        out_absorb[bin] = 0.0;
    }
    if (LDG_UNLIKELY(medium >= LDG_MATH_PHYS_ATMOSPHERE_MEDIUM_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(altitude) || altitude < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (altitude > hdr->thickness) { altitude = hdr->thickness; }

    return ldg_math_phys_atmosphere_medium_sample_get(hdr, ldg_math_phys_atmosphere_table_rayleigh_get(ctx), ldg_math_phys_atmosphere_table_ozone_get(ctx), medium, altitude, out_scatter, out_absorb, out_anisotropy);
}

LDG_EXPORT uint32_t ldg_math_phys_atmosphere_vol_get(const ldg_math_phys_ctx_t *ctx, const ldg_math_phys_atmosphere_vol_desc_t *desc, double *out_transmittance, double *out_inscatter)
{
    ldg_math_phys_atmosphere_hdr_t *hdr = 0x0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_transmittance)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_inscatter)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    if (LDG_UNLIKELY(ctx->atmosphere == 0x0)) { return LDG_ERR_NOT_INIT; }

    hdr = ldg_math_phys_atmosphere_hdr_get(ctx);
    if (LDG_UNLIKELY(!hdr->solved)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(desc->cunt_x == 0 || desc->cunt_y == 0 || desc->cunt_z == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->spacing) || desc->spacing <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->ref_altitude))) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->orig[axis]) || !ldg_math_finite_is(desc->cam[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(desc->sun[axis]) || !ldg_math_finite_is(desc->up[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; }
    }
    return ldg_math_phys_atmosphere_vol_compute_get(ctx, desc, out_transmittance, out_inscatter);
}

uint32_t ldg_math_phys_atmosphere_vol_cpu_get(const ldg_math_phys_ctx_t *ctx, const ldg_math_phys_atmosphere_vol_desc_t *desc, double *out_transmittance, double *out_inscatter)
{
    ldg_math_phys_atmosphere_table_t table = LDG_STRUCT_ZERO_INIT;
    if (LDG_UNLIKELY(!ctx || !desc || !out_transmittance || !out_inscatter)) { return LDG_ERR_FUNC_ARG_NULL; }

    ldg_math_phys_atmosphere_table_set(ctx, &table);
    return ldg_math_phys_atmosphere_vol_sample_get(&table, desc, out_transmittance, out_inscatter);
}

LDG_EXPORT uint32_t ldg_math_phys_em_drive_add(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, uint32_t comp, double amplitude, double freq, uint32_t mode, uint32_t *out)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    uint64_t cell_idx[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint32_t err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;
    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(comp >= LDG_MATH_PHYS_DIM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(amplitude))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(freq) || freq <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(mode > LDG_MATH_PHYS_EM_DRIVE_PULSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->drive_cunt >= LDG_MATH_PHYS_EM_DRIVE_CAP)) { return LDG_ERR_FULL; }

    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(pos[axis]) || pos[axis] < 0.0 || pos[axis] >= ctx->region[region].desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

        cell_idx[axis] = (uint64_t)(pos[axis] / ctx->region[region].desc.spacing + 0.5);
        if (cell_idx[axis] >= dim[axis]) { cell_idx[axis] = dim[axis] - 1; }
    }
    err = ldg_mem_secure_zero(&hdr->drive_slot[hdr->drive_cunt], sizeof(ldg_math_phys_em_drive_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (mode == LDG_MATH_PHYS_EM_DRIVE_CONT) { err = ldg_math_phys_sig_sine_init(amplitude, freq, &hdr->drive_slot[hdr->drive_cunt].sig); }
    else { err = ldg_math_phys_sig_gauss_pulse_init(amplitude, freq, &hdr->drive_slot[hdr->drive_cunt].sig); }

    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr->drive_slot[hdr->drive_cunt].cell = (cell_idx[2] * hdr->ny + cell_idx[1]) * hdr->nx + cell_idx[0];
    hdr->drive_slot[hdr->drive_cunt].comp = comp;
    hdr->drive_slot[hdr->drive_cunt].tick_start = hdr->tick_cuntr;
    *out = (uint32_t)hdr->drive_cunt;
    hdr->drive_cunt++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_em_drive_sig_add(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, uint32_t comp, const ldg_math_phys_sig_t *sig, uint32_t *out)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_math_phys_em_drive_t *slot = 0x0;
    uint64_t cell_idx[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint32_t err = 0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!sig)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;
    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(comp >= LDG_MATH_PHYS_DIM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(sig->kind >= LDG_MATH_PHYS_SIG_KIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(sig->amplitude))) { return LDG_ERR_FUNC_ARG_INVALID; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->drive_cunt >= LDG_MATH_PHYS_EM_DRIVE_CAP)) { return LDG_ERR_FULL; }

    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(pos[axis]) || pos[axis] < 0.0 || pos[axis] >= ctx->region[region].desc.extent[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

        cell_idx[axis] = (uint64_t)(pos[axis] / ctx->region[region].desc.spacing + 0.5);
        if (cell_idx[axis] >= dim[axis]) { cell_idx[axis] = dim[axis] - 1; }
    }

    slot = &hdr->drive_slot[hdr->drive_cunt];
    slot->sig = *sig;
    slot->cell = (cell_idx[2] * hdr->ny + cell_idx[1]) * hdr->nx + cell_idx[0];
    slot->comp = comp;
    slot->tick_start = hdr->tick_cuntr;
    *out = (uint32_t)hdr->drive_cunt;
    hdr->drive_cunt++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_em_drive_series_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t drive, const double *series, uint64_t cunt)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(series != 0x0 && cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(drive >= hdr->drive_cunt)) { return LDG_ERR_MATH_PHYS_KIND_INVALID; }

    hdr->drive_slot[drive].sig.series = series;
    hdr->drive_slot[drive].sig.series_cunt = (series != 0x0) ? cunt : 0;
    hdr->drive_slot[drive].tick_start = hdr->tick_cuntr;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_em_conductor_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi, uint32_t conductor)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    uint16_t *conductor_grid = 0x0;
    double spacing = 0.0;
    uint64_t box_lo[3] = LDG_ARR_ZERO_INIT;
    uint64_t box_hi[3] = LDG_ARR_ZERO_INIT;
    uint64_t dim[3] = LDG_ARR_ZERO_INIT;
    uint64_t pos_x = 0;
    uint64_t pos_y = 0;
    uint64_t pos_z = 0;
    uint64_t cell = 0;
    uint32_t axis = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!lo || !hi)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->conductor_cap == 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(conductor >= hdr->conductor_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    spacing = ctx->region[region].desc.spacing;
    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(lo[axis]) || !ldg_math_finite_is(hi[axis]) || lo[axis] > hi[axis])) { return LDG_ERR_FUNC_ARG_INVALID; }

        box_lo[axis] = (uint64_t)((lo[axis] < 0.0) ? 0.0 : (lo[axis] / spacing + 0.5));
        box_hi[axis] = (uint64_t)(hi[axis] / spacing + 0.5);
        if (box_hi[axis] >= dim[axis]) { box_hi[axis] = dim[axis] - 1; }

        if (LDG_UNLIKELY(box_lo[axis] > box_hi[axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }
    }
    conductor_grid = ldg_math_phys_em_conductor_get(&ctx->region[region]);
    for (pos_z = box_lo[2]; pos_z <= box_hi[2]; pos_z++) { for (pos_y = box_lo[1]; pos_y <= box_hi[1]; pos_y++) { for (pos_x = box_lo[0]; pos_x <= box_hi[0]; pos_x++)
            {
                cell = (pos_z * hdr->ny + pos_y) * hdr->nx + pos_x;
                conductor_grid[cell] = (uint16_t)conductor;
            }
        } }
    if ((uint64_t)conductor + 1 > hdr->conductor_cunt) { hdr->conductor_cunt = (uint64_t)conductor + 1; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_em_capacitance_solve(ldg_math_phys_ctx_t *ctx, uint32_t region, double tol, uint64_t iter_max, double *out)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    double charge_val = 0.0;
    uint64_t excited = 0;
    uint64_t tgt = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->conductor_cap == 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(hdr->conductor_cunt < 2)) { return LDG_ERR_EMPTY; }

    for (excited = 0; excited < hdr->conductor_cunt; excited++)
    {
        err = ldg_math_phys_em_electrostatic_solve(ldg_math_phys_em_potential_get(&ctx->region[region]), ldg_math_phys_em_electrostatic_resid_get(&ctx->region[region]), ldg_math_phys_em_electrostatic_dir_get(&ctx->region[region]), ldg_math_phys_em_electrostatic_a_dir_get(&ctx->region[region]), ldg_math_phys_em_conductor_get(&ctx->region[region]), ldg_math_phys_em_mtl_get(&ctx->region[region]), ldg_math_phys_em_coeff_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, ctx->region[region].desc.spacing, (uint32_t)excited, LDG_MATH_PHYS_EM_COEFF_EPSILON, 0, tol, iter_max);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        for (tgt = 0; tgt < hdr->conductor_cunt; tgt++)
        {
            err = ldg_math_phys_em_electrostatic_charge_get(ldg_math_phys_em_potential_get(&ctx->region[region]), ldg_math_phys_em_conductor_get(&ctx->region[region]), ldg_math_phys_em_mtl_get(&ctx->region[region]), ldg_math_phys_em_coeff_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, ctx->region[region].desc.spacing, (uint32_t)tgt, LDG_MATH_PHYS_EM_COEFF_EPSILON, 0, &charge_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            out[tgt * hdr->conductor_cunt + excited] = charge_val;
        }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_em_extraction_matrix_get(ldg_math_phys_ctx_t *ctx, uint32_t region, uint64_t signal_cunt, uint32_t coeff_sel, uint8_t vacuum_is, double tol, uint64_t iter_max, double *out)
{
    ldg_math_phys_em_hdr_t *hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    double charge_val = 0.0;
    uint64_t excited = 0;
    uint64_t tgt = 0;
    uint32_t err = 0;
    for (excited = 0; excited < signal_cunt; excited++)
    {
        err = ldg_math_phys_em_electrostatic_solve(ldg_math_phys_em_potential_get(&ctx->region[region]), ldg_math_phys_em_electrostatic_resid_get(&ctx->region[region]), ldg_math_phys_em_electrostatic_dir_get(&ctx->region[region]), ldg_math_phys_em_electrostatic_a_dir_get(&ctx->region[region]), ldg_math_phys_em_conductor_get(&ctx->region[region]), ldg_math_phys_em_mtl_get(&ctx->region[region]), ldg_math_phys_em_coeff_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, ctx->region[region].desc.spacing, (uint32_t)excited, coeff_sel, vacuum_is, tol, iter_max);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        for (tgt = 0; tgt < signal_cunt; tgt++)
        {
            err = ldg_math_phys_em_electrostatic_charge_get(ldg_math_phys_em_potential_get(&ctx->region[region]), ldg_math_phys_em_conductor_get(&ctx->region[region]), ldg_math_phys_em_mtl_get(&ctx->region[region]), ldg_math_phys_em_coeff_get(&ctx->region[region]), hdr->nx, hdr->ny, hdr->nz, ctx->region[region].desc.spacing, (uint32_t)tgt, coeff_sel, vacuum_is, &charge_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            out[tgt * signal_cunt + excited] = charge_val;
        }
    }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_em_matrix_invert(double *mat, double *work, uint64_t dim)
{
    double pivot_val = 0.0;
    double factor = 0.0;
    uint64_t row = 0;
    uint64_t col = 0;
    uint64_t pivot_row = 0;
    uint64_t sweep = 0;
    for (row = 0; row < dim; row++) { for (col = 0; col < dim; col++) { work[row * dim + col] = (row == col) ? 1.0 : 0.0; } }
    for (sweep = 0; sweep < dim; sweep++)
    {
        pivot_row = sweep;
        for (row = sweep + 1; row < dim; row++) { if (ldg_math_abs_get(mat[row * dim + sweep]) > ldg_math_abs_get(mat[pivot_row * dim + sweep])) { pivot_row = row; } }
        if (LDG_UNLIKELY(!(ldg_math_abs_get(mat[pivot_row * dim + sweep]) > 0.0))) { return LDG_ERR_MATH_PHYS_DOMAIN; }

        if (pivot_row != sweep) { for (col = 0; col < dim; col++)
            {
                pivot_val = mat[sweep * dim + col];
                mat[sweep * dim + col] = mat[pivot_row * dim + col];
                mat[pivot_row * dim + col] = pivot_val;
                pivot_val = work[sweep * dim + col];
                work[sweep * dim + col] = work[pivot_row * dim + col];
                work[pivot_row * dim + col] = pivot_val;
            }
        }

        pivot_val = mat[sweep * dim + sweep];
        for (col = 0; col < dim; col++)
        {
            mat[sweep * dim + col] /= pivot_val;
            work[sweep * dim + col] /= pivot_val;
        }
        for (row = 0; row < dim; row++)
        {
            if (row == sweep) { continue; }

            factor = mat[row * dim + sweep];
            if (!(ldg_math_abs_get(factor) > 0.0)) { continue; }

            for (col = 0; col < dim; col++)
            {
                mat[row * dim + col] -= factor * mat[sweep * dim + col];
                work[row * dim + col] -= factor * work[sweep * dim + col];
            }
        }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_em_rlgc_solve(ldg_math_phys_ctx_t *ctx, uint32_t region, double freq, double tol, uint64_t iter_max, double *out_r, double *out_l, double *out_g, double *out_c)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    const uint16_t *conductor_grid = 0x0;
    const uint16_t *mtl_grid = 0x0;
    const double *coeff = 0x0;
    const double *mtl_row = 0x0;
    double *inv_work = 0x0;
    double spacing = 0.0;
    double perimeter = 0.0;
    double sigma_val = 0.0;
    double mu_val = 0.0;
    double sheet_resistance = 0.0;
    double series_resistance[LDG_MATH_PHYS_EM_CONDUCTOR_CAP] = LDG_ARR_ZERO_INIT;
    double ref_resistance = 0.0;
    uint64_t signal_cunt = 0;
    uint64_t cell_cunt_tgt = 0;
    uint64_t first_cell = 0;
    uint64_t cell = 0;
    uint64_t row = 0;
    uint64_t col = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_r || !out_l || !out_g || !out_c)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(freq) || freq < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->conductor_cap == 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(hdr->nz != 1)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(hdr->conductor_cunt < 2)) { return LDG_ERR_EMPTY; }

    signal_cunt = hdr->conductor_cunt - 1;
    if (LDG_UNLIKELY(hdr->cell_cunt < signal_cunt * signal_cunt)) { return LDG_ERR_UNSUPPORTED; }

    coeff = ldg_math_phys_em_coeff_get(&ctx->region[region]);
    for (row = 0; row < hdr->mtl_cunt; row++) { if (LDG_UNLIKELY(ldg_math_abs_get(coeff[row * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_MU] - LDG_MATH_PHYS_VACUUM_PERMEABILITY) > 1e-12 * LDG_MATH_PHYS_VACUUM_PERMEABILITY)) { return LDG_ERR_UNSUPPORTED; } }
    err = ldg_math_phys_em_extraction_matrix_get(ctx, region, signal_cunt, LDG_MATH_PHYS_EM_COEFF_EPSILON, 0, tol, iter_max, out_c);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_em_extraction_matrix_get(ctx, region, signal_cunt, LDG_MATH_PHYS_EM_COEFF_SIGMA, 0, tol, iter_max, out_g);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_em_extraction_matrix_get(ctx, region, signal_cunt, LDG_MATH_PHYS_EM_COEFF_EPSILON, 1, tol, iter_max, out_l);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    inv_work = ldg_math_phys_em_potential_get(&ctx->region[region]);
    err = ldg_math_phys_em_matrix_invert(out_l, inv_work, signal_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (row = 0; row < signal_cunt; row++) { for (col = 0; col < signal_cunt; col++) { out_l[row * signal_cunt + col] = LDG_MATH_PHYS_VACUUM_PERMEABILITY * LDG_MATH_PHYS_VACUUM_PERMITTIVITY * inv_work[row * signal_cunt + col]; } }
    conductor_grid = ldg_math_phys_em_conductor_get(&ctx->region[region]);
    mtl_grid = ldg_math_phys_em_mtl_get(&ctx->region[region]);
    spacing = ctx->region[region].desc.spacing;
    for (row = 0; row < hdr->conductor_cunt; row++)
    {
        err = ldg_math_phys_em_electrostatic_perimeter_get(conductor_grid, hdr->nx, hdr->ny, hdr->nz, spacing, (uint32_t)row, &perimeter, &cell_cunt_tgt);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (LDG_UNLIKELY(cell_cunt_tgt == 0 || !(perimeter > 0.0))) { return LDG_ERR_EMPTY; }

        first_cell = 0;
        for (cell = 0; cell < hdr->cell_cunt; cell++) { if (conductor_grid[cell] == (uint16_t)row)
            {
                first_cell = cell;
                break;
            }
        }
        mtl_row = coeff + (uint64_t)mtl_grid[first_cell] * LDG_MATH_PHYS_EM_COEFF_STRIDE;
        sigma_val = mtl_row[LDG_MATH_PHYS_EM_COEFF_SIGMA];
        mu_val = mtl_row[LDG_MATH_PHYS_EM_COEFF_MU];
        if (ldg_math_inf_is(sigma_val)) { sheet_resistance = 0.0; }
        else
        {
            if (LDG_UNLIKELY(!(sigma_val > 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (freq > 0.0) { sheet_resistance = ldg_math_sqrt_get(LDG_MATH_PI * freq * mu_val / sigma_val) / perimeter; }
            else { sheet_resistance = 1.0 / (sigma_val * (double)cell_cunt_tgt * spacing * spacing); }
        }

        if (row + 1 == hdr->conductor_cunt) { ref_resistance = sheet_resistance; }
        else { series_resistance[row] = sheet_resistance; }
    }
    for (row = 0; row < signal_cunt; row++) { for (col = 0; col < signal_cunt; col++) { out_r[row * signal_cunt + col] = ((row == col) ? series_resistance[row] : 0.0) + ref_resistance; } }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_em_port_add(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_em_port_desc_t *desc, uint32_t *out)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_math_phys_em_port_t *slot = 0x0;
    double spacing = 0.0;
    uint64_t dim[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t stride[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint64_t lo_cell[2] = LDG_ARR_ZERO_INIT;
    uint64_t hi_cell[2] = LDG_ARR_ZERO_INIT;
    uint64_t plane_cell = 0;
    uint32_t axis_a = 0;
    uint32_t axis_b = 0;
    uint32_t err = 0;
    uint32_t edge = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;
    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->port_cap == 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(hdr->port_cunt >= hdr->port_cap)) { return LDG_ERR_FULL; }

    if (LDG_UNLIKELY(desc->normal_axis >= LDG_MATH_PHYS_DIM)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->z_ref) || desc->z_ref <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->mode_cap == 0 || desc->mode_cap > LDG_MATH_PHYS_EM_PORT_MODE_CAP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->is_forward > 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    axis_a = (desc->normal_axis + 1) % LDG_MATH_PHYS_DIM;
    axis_b = (desc->normal_axis + 2) % LDG_MATH_PHYS_DIM;
    spacing = ctx->region[region].desc.spacing;
    dim[0] = hdr->nx;
    dim[1] = hdr->ny;
    dim[2] = hdr->nz;
    stride[0] = 1;
    stride[1] = hdr->nx;
    stride[2] = hdr->nx * hdr->ny;
    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->plane_pos) || desc->plane_pos < 0.0 || desc->plane_pos >= ctx->region[region].desc.extent[desc->normal_axis])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

    plane_cell = (uint64_t)(desc->plane_pos / spacing + 0.5);
    if (plane_cell >= dim[desc->normal_axis]) { plane_cell = dim[desc->normal_axis] - 1; }

    for (edge = 0; edge < 2; edge++) { if (LDG_UNLIKELY(!ldg_math_finite_is(desc->lo[edge]) || !ldg_math_finite_is(desc->hi[edge]) || desc->lo[edge] > desc->hi[edge] || desc->lo[edge] < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; } }
    lo_cell[0] = (uint64_t)(desc->lo[0] / spacing + 0.5);
    hi_cell[0] = (uint64_t)(desc->hi[0] / spacing + 0.5);
    lo_cell[1] = (uint64_t)(desc->lo[1] / spacing + 0.5);
    hi_cell[1] = (uint64_t)(desc->hi[1] / spacing + 0.5);
    if (hi_cell[0] >= dim[axis_a]) { hi_cell[0] = dim[axis_a] - 1; }

    if (hi_cell[1] >= dim[axis_b]) { hi_cell[1] = dim[axis_b] - 1; }

    if (LDG_UNLIKELY(lo_cell[0] > hi_cell[0] || lo_cell[1] > hi_cell[1])) { return LDG_ERR_MATH_PHYS_REGION_BOUNDS; }

    slot = &hdr->port_slot[hdr->port_cunt];
    slot->z_ref = desc->z_ref;
    slot->face_area = (hdr->nz == 1) ? spacing : spacing * spacing;
    slot->base_cell = plane_cell * stride[desc->normal_axis] + lo_cell[0] * stride[axis_a] + lo_cell[1] * stride[axis_b];
    slot->stride_a = stride[axis_a];
    slot->stride_b = stride[axis_b];
    slot->grid_a = hi_cell[0] - lo_cell[0] + 1;
    slot->grid_b = hi_cell[1] - lo_cell[1] + 1;
    slot->mode_cunt = 0;
    slot->mode_cap = desc->mode_cap;
    slot->axis_a = axis_a;
    slot->axis_b = axis_b;
    slot->is_forward = desc->is_forward;
    slot->used = 1;
    *out = (uint32_t)hdr->port_cunt;
    hdr->port_cunt++;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_em_port_grid_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t port, uint64_t *out_a, uint64_t *out_b)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_a || !out_b)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_a = 0;
    *out_b = 0;
    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(port >= hdr->port_cunt)) { return LDG_ERR_NOT_FOUND; }

    *out_a = hdr->port_slot[port].grid_a;
    *out_b = hdr->port_slot[port].grid_b;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_em_port_mode_pattern_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t port, uint32_t mode, const double *e_pattern, const double *h_pattern)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_math_phys_em_port_t *slot = 0x0;
    double *store_e_a = 0x0;
    double *store_e_b = 0x0;
    double *store_h_a = 0x0;
    double *store_h_b = 0x0;
    double power_sum = 0.0;
    double scale_factor = 0.0;
    uint64_t face_cunt = 0;
    uint64_t idx = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!e_pattern || !h_pattern)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(port >= hdr->port_cunt)) { return LDG_ERR_NOT_FOUND; }

    slot = &hdr->port_slot[port];
    if (LDG_UNLIKELY(mode >= slot->mode_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    face_cunt = slot->grid_a * slot->grid_b;
    store_e_a = ldg_math_phys_em_port_pattern_get(&ctx->region[region], port, mode, 0);
    store_e_b = ldg_math_phys_em_port_pattern_get(&ctx->region[region], port, mode, 1);
    store_h_a = ldg_math_phys_em_port_pattern_get(&ctx->region[region], port, mode, 2);
    store_h_b = ldg_math_phys_em_port_pattern_get(&ctx->region[region], port, mode, 3);
    power_sum = 0.0;
    for (idx = 0; idx < face_cunt; idx++) { power_sum += (e_pattern[idx] * h_pattern[face_cunt + idx] - e_pattern[face_cunt + idx] * h_pattern[idx]) * slot->face_area; }
    if (LDG_UNLIKELY(!(power_sum > 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    scale_factor = 1.0 / ldg_math_sqrt_get(power_sum);
    for (idx = 0; idx < face_cunt; idx++)
    {
        store_e_a[idx] = e_pattern[idx] * scale_factor;
        store_e_b[idx] = e_pattern[face_cunt + idx] * scale_factor;
        store_h_a[idx] = h_pattern[idx] * scale_factor;
        store_h_b[idx] = h_pattern[face_cunt + idx] * scale_factor;
    }
    if ((uint64_t)mode + 1 > slot->mode_cunt) { slot->mode_cunt = (uint64_t)mode + 1; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_em_port_mode_analytic_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t port, uint32_t mode, uint32_t kind, uint32_t idx_m, uint32_t idx_n)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_math_phys_em_port_t *slot = 0x0;
    double *store_e_a = 0x0;
    double *store_e_b = 0x0;
    double *store_h_a = 0x0;
    double *store_h_b = 0x0;
    double area = 0.0;
    double e_val = 0.0;
    double h_val = 0.0;
    double wave_a = 0.0;
    double wave_b = 0.0;
    double arg_a = 0.0;
    double arg_b = 0.0;
    double shape_a = 0.0;
    double shape_b = 0.0;
    double power_sum = 0.0;
    double scale_e = 0.0;
    double scale_h = 0.0;
    uint64_t face_cunt = 0;
    uint64_t idx = 0;
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(kind != LDG_MATH_PHYS_EM_PORT_MODE_TEM && kind != LDG_MATH_PHYS_EM_PORT_MODE_TE && kind != LDG_MATH_PHYS_EM_PORT_MODE_TM)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(kind == LDG_MATH_PHYS_EM_PORT_MODE_TEM && (idx_m != 0 || idx_n != 0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(kind == LDG_MATH_PHYS_EM_PORT_MODE_TE && idx_m == 0 && idx_n == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(kind == LDG_MATH_PHYS_EM_PORT_MODE_TM && (idx_m == 0 || idx_n == 0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(port >= hdr->port_cunt)) { return LDG_ERR_NOT_FOUND; }

    slot = &hdr->port_slot[port];
    if (LDG_UNLIKELY(mode >= slot->mode_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(kind != LDG_MATH_PHYS_EM_PORT_MODE_TEM && (idx_m >= slot->grid_a || idx_n >= slot->grid_b))) { return LDG_ERR_RANGE; }

    face_cunt = slot->grid_a * slot->grid_b;
    store_e_a = ldg_math_phys_em_port_pattern_get(&ctx->region[region], port, mode, 0);
    store_e_b = ldg_math_phys_em_port_pattern_get(&ctx->region[region], port, mode, 1);
    store_h_a = ldg_math_phys_em_port_pattern_get(&ctx->region[region], port, mode, 2);
    store_h_b = ldg_math_phys_em_port_pattern_get(&ctx->region[region], port, mode, 3);
    if (kind == LDG_MATH_PHYS_EM_PORT_MODE_TEM)
    {
        area = (double)face_cunt * slot->face_area;
        e_val = ldg_math_sqrt_get(slot->z_ref / area);
        h_val = 1.0 / ldg_math_sqrt_get(slot->z_ref * area);
        for (idx = 0; idx < face_cunt; idx++)
        {
            store_e_a[idx] = e_val;
            store_e_b[idx] = 0.0;
            store_h_a[idx] = 0.0;
            store_h_b[idx] = h_val;
        }
        if ((uint64_t)mode + 1 > slot->mode_cunt) { slot->mode_cunt = (uint64_t)mode + 1; }

        return LDG_ERR_AOK;
    }

    wave_a = (double)idx_m / (double)slot->grid_a;
    wave_b = (double)idx_n / (double)slot->grid_b;
    power_sum = 0.0;
    for (idx = 0; idx < face_cunt; idx++)
    {
        idx_a = idx % slot->grid_a;
        idx_b = idx / slot->grid_a;
        arg_a = LDG_MATH_PI * (double)idx_m * ((double)idx_a + 0.5) / (double)slot->grid_a;
        arg_b = LDG_MATH_PI * (double)idx_n * ((double)idx_b + 0.5) / (double)slot->grid_b;
        if (kind == LDG_MATH_PHYS_EM_PORT_MODE_TE)
        {
            shape_a = (0.0 - wave_b) * ldg_math_cos_get(arg_a) * ldg_math_sin_get(arg_b);
            shape_b = wave_a * ldg_math_sin_get(arg_a) * ldg_math_cos_get(arg_b);
        }
        else
        {
            shape_a = wave_a * ldg_math_cos_get(arg_a) * ldg_math_sin_get(arg_b);
            shape_b = wave_b * ldg_math_sin_get(arg_a) * ldg_math_cos_get(arg_b);
        }

        power_sum += (shape_a * shape_a + shape_b * shape_b) * slot->face_area;
    }
    if (LDG_UNLIKELY(!(power_sum > 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    scale_e = ldg_math_sqrt_get(slot->z_ref / power_sum);
    scale_h = 1.0 / ldg_math_sqrt_get(slot->z_ref * power_sum);
    for (idx = 0; idx < face_cunt; idx++)
    {
        idx_a = idx % slot->grid_a;
        idx_b = idx / slot->grid_a;
        arg_a = LDG_MATH_PI * (double)idx_m * ((double)idx_a + 0.5) / (double)slot->grid_a;
        arg_b = LDG_MATH_PI * (double)idx_n * ((double)idx_b + 0.5) / (double)slot->grid_b;
        if (kind == LDG_MATH_PHYS_EM_PORT_MODE_TE)
        {
            shape_a = (0.0 - wave_b) * ldg_math_cos_get(arg_a) * ldg_math_sin_get(arg_b);
            shape_b = wave_a * ldg_math_sin_get(arg_a) * ldg_math_cos_get(arg_b);
        }
        else
        {
            shape_a = wave_a * ldg_math_cos_get(arg_a) * ldg_math_sin_get(arg_b);
            shape_b = wave_b * ldg_math_sin_get(arg_a) * ldg_math_cos_get(arg_b);
        }

        store_e_a[idx] = shape_a * scale_e;
        store_e_b[idx] = shape_b * scale_e;
        store_h_a[idx] = (0.0 - shape_b) * scale_h;
        store_h_b[idx] = shape_a * scale_h;
    }
    if ((uint64_t)mode + 1 > slot->mode_cunt) { slot->mode_cunt = (uint64_t)mode + 1; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_em_port_mode_fem_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t port, uint32_t mode, double freq, uint32_t mode_idx, double *out_beta)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_math_phys_em_port_t *slot = 0x0;
    const uint16_t *mtl = 0x0;
    const uint16_t *conductor = 0x0;
    const double *coeff = 0x0;
    double *eps_arr = 0x0;
    double *mu_arr = 0x0;
    double *e_pat = 0x0;
    double *h_pat = 0x0;
    uint8_t *pec_arr = 0x0;
    void *blk = 0x0;
    double k0 = 0.0;
    double beta_val = 0.0;
    uint64_t face_cunt = 0;
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    uint64_t cell = 0;
    uint64_t face = 0;
    uint16_t mtl_id = 0;
    uint32_t err = 0;
    uint32_t ret = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(freq) || !(freq > 0.0))) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(ldg_mem_locked_is() == LDG_TRUTH_TRUE)) { return LDG_ERR_DENIED; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(port >= hdr->port_cunt)) { return LDG_ERR_NOT_FOUND; }

    slot = &hdr->port_slot[port];
    if (LDG_UNLIKELY(mode >= slot->mode_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (out_beta != 0x0) { *out_beta = 0.0; }

    mtl = ldg_math_phys_em_mtl_get(&ctx->region[region]);
    coeff = ldg_math_phys_em_coeff_get(&ctx->region[region]);
    conductor = (hdr->conductor_cap > 0) ? ldg_math_phys_em_conductor_get(&ctx->region[region]) : 0x0;
    face_cunt = slot->grid_a * slot->grid_b;
    k0 = LDG_MATH_TAU * freq / LDG_MATH_PHYS_VACUUM_LIGHT_SPEED;
    err = ldg_mem_alloc(face_cunt * sizeof(double), &blk);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    eps_arr = (double *)blk;
    err = ldg_mem_alloc(face_cunt * sizeof(double), &blk);
    if (err == LDG_ERR_AOK)
    {
        mu_arr = (double *)blk;
        err = ldg_mem_alloc(face_cunt * sizeof(uint8_t), &blk);
    }

    if (err == LDG_ERR_AOK)
    {
        pec_arr = (uint8_t *)blk;
        err = ldg_mem_alloc(2 * face_cunt * sizeof(double), &blk);
    }

    if (err == LDG_ERR_AOK)
    {
        e_pat = (double *)blk;
        err = ldg_mem_alloc(2 * face_cunt * sizeof(double), &blk);
    }

    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { ret = LDG_ERR_ALLOC_NULL; }
    else
    {
        h_pat = (double *)blk;
        for (idx_b = 0; idx_b < slot->grid_b; idx_b++)
        {
            for (idx_a = 0; idx_a < slot->grid_a; idx_a++)
            {
                face = idx_b * slot->grid_a + idx_a;
                cell = slot->base_cell + idx_a * slot->stride_a + idx_b * slot->stride_b;
                mtl_id = mtl[cell];
                eps_arr[face] = coeff[(uint64_t)mtl_id * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_EPSILON] / LDG_MATH_PHYS_VACUUM_PERMITTIVITY;
                mu_arr[face] = coeff[(uint64_t)mtl_id * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_MU] / LDG_MATH_PHYS_VACUUM_PERMEABILITY;
                pec_arr[face] = 0;
                if (ldg_math_inf_is(coeff[(uint64_t)mtl_id * LDG_MATH_PHYS_EM_COEFF_STRIDE + LDG_MATH_PHYS_EM_COEFF_SIGMA])) { pec_arr[face] = 1; }

                if (conductor != 0x0 && conductor[cell] != UINT16_MAX) { pec_arr[face] = 1; }
            }
        }
        ret = ldg_math_phys_em_portmode_solve(eps_arr, mu_arr, pec_arr, slot->grid_a, slot->grid_b, ctx->region[region].desc.spacing, k0, mode_idx, e_pat, h_pat, &beta_val);
        if (ret == LDG_ERR_AOK)
        {
            ret = ldg_math_phys_em_port_mode_pattern_set(ctx, region, port, mode, e_pat, h_pat);
            if (ret == LDG_ERR_AOK && out_beta != 0x0) { *out_beta = beta_val; }
        }
    }

    if (h_pat != 0x0) { err = ldg_mem_dealloc(h_pat); }

    if (e_pat != 0x0) { err = ldg_mem_dealloc(e_pat); }

    if (pec_arr != 0x0) { err = ldg_mem_dealloc(pec_arr); }

    if (mu_arr != 0x0) { err = ldg_mem_dealloc(mu_arr); }

    err = ldg_mem_dealloc(eps_arr);
    return ret;
}

uint32_t ldg_math_phys_em_port_capture(ldg_math_phys_ctx_t *ctx, uint32_t region)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_math_phys_em_port_t *slot = 0x0;
    const double *electric = 0x0;
    const double *magnetic = 0x0;
    const double *store_e_a = 0x0;
    const double *store_e_b = 0x0;
    const double *store_h_a = 0x0;
    const double *store_h_b = 0x0;
    double v_sum = 0.0;
    double i_sum = 0.0;
    double sign = 0.0;
    uint64_t cell = 0;
    uint64_t idx_a = 0;
    uint64_t idx_b = 0;
    uint64_t face_idx = 0;
    uint64_t port = 0;
    uint64_t mode = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (hdr->port_cunt == 0) { return LDG_ERR_AOK; }

    electric = ldg_math_phys_em_electric_get(&ctx->region[region]);
    magnetic = ldg_math_phys_em_magnetic_get(&ctx->region[region]);
    for (port = 0; port < hdr->port_cunt; port++)
    {
        slot = &hdr->port_slot[port];
        sign = (slot->is_forward == LDG_TRUTH_TRUE) ? 1.0 : (0.0 - 1.0);
        for (mode = 0; mode < slot->mode_cunt; mode++)
        {
            store_e_a = ldg_math_phys_em_port_pattern_get(&ctx->region[region], port, mode, 0);
            store_e_b = ldg_math_phys_em_port_pattern_get(&ctx->region[region], port, mode, 1);
            store_h_a = ldg_math_phys_em_port_pattern_get(&ctx->region[region], port, mode, 2);
            store_h_b = ldg_math_phys_em_port_pattern_get(&ctx->region[region], port, mode, 3);
            v_sum = 0.0;
            i_sum = 0.0;
            for (idx_b = 0; idx_b < slot->grid_b; idx_b++) { for (idx_a = 0; idx_a < slot->grid_a; idx_a++)
                {
                    cell = slot->base_cell + idx_a * slot->stride_a + idx_b * slot->stride_b;
                    face_idx = idx_b * slot->grid_a + idx_a;
                    v_sum += (electric[(uint64_t)slot->axis_a * hdr->cell_cunt + cell] * store_h_b[face_idx] - electric[(uint64_t)slot->axis_b * hdr->cell_cunt + cell] * store_h_a[face_idx]) * slot->face_area;
                    i_sum += (store_e_a[face_idx] * magnetic[(uint64_t)slot->axis_b * hdr->cell_cunt + cell] - store_e_b[face_idx] * magnetic[(uint64_t)slot->axis_a * hdr->cell_cunt + cell]) * slot->face_area;
                }
            }
            ldg_math_phys_em_port_tap_ring_get(&ctx->region[region], port, mode, 0)[hdr->port_tap_hd] = v_sum;
            ldg_math_phys_em_port_tap_ring_get(&ctx->region[region], port, mode, 1)[hdr->port_tap_hd] = i_sum * sign;
        }
    }
    hdr->port_tap_hd = (hdr->port_tap_hd + 1) % hdr->port_tap_depth;
    if (hdr->port_tap_cunt < hdr->port_tap_depth) { hdr->port_tap_cunt++; }

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_em_port_tap_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t port, uint32_t mode, double *out_v, double *out_i, uint64_t cap, uint64_t *out_cunt)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    const double *ring_v = 0x0;
    const double *ring_i = 0x0;
    uint64_t copy_cunt = 0;
    uint64_t start = 0;
    uint64_t idx = 0;
    uint64_t slot_idx = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_v || !out_i || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(port >= hdr->port_cunt)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(mode >= hdr->port_slot[port].mode_cunt)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ring_v = ldg_math_phys_em_port_tap_ring_get(&ctx->region[region], port, mode, 0);
    ring_i = ldg_math_phys_em_port_tap_ring_get(&ctx->region[region], port, mode, 1);
    copy_cunt = (hdr->port_tap_cunt < cap) ? hdr->port_tap_cunt : cap;
    start = (hdr->port_tap_hd + hdr->port_tap_depth - copy_cunt) % hdr->port_tap_depth;
    for (idx = 0; idx < copy_cunt; idx++)
    {
        slot_idx = (start + idx) % hdr->port_tap_depth;
        out_v[idx] = ring_v[slot_idx];
        out_i[idx] = ring_i[slot_idx];
    }
    *out_cunt = copy_cunt;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_em_port_gpu_pack(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t *out_word, double *out_val, const double **out_pattern, uint64_t *out_pattern_cunt, uint32_t *out_pm_cunt)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    const ldg_math_phys_em_port_t *slot = 0x0;
    uint64_t port = 0;
    uint64_t mode = 0;
    uint64_t pm = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !out_word || !out_val || !out_pattern || !out_pattern_cunt || !out_pm_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_pattern = 0x0;
    *out_pattern_cunt = 0;
    *out_pm_cunt = 0;
    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (hdr->port_cunt == 0) { return LDG_ERR_AOK; }

    out_word[0] = (uint32_t)(hdr->port_cunt * LDG_MATH_PHYS_EM_PORT_MODE_CAP);
    out_word[1] = (uint32_t)hdr->face_max;
    out_word[2] = (uint32_t)hdr->cell_cunt;
    out_word[3] = 0;
    for (port = 0; port < LDG_MATH_PHYS_EM_PORT_CAP; port++)
    {
        for (mode = 0; mode < LDG_MATH_PHYS_EM_PORT_MODE_CAP; mode++)
        {
            pm = port * LDG_MATH_PHYS_EM_PORT_MODE_CAP + mode;
            slot = &hdr->port_slot[port];
            out_word[4 + 0 * 8 + pm] = (port < hdr->port_cunt) ? (uint32_t)slot->base_cell : 0;
            out_word[4 + 1 * 8 + pm] = (port < hdr->port_cunt) ? (uint32_t)slot->stride_a : 0;
            out_word[4 + 2 * 8 + pm] = (port < hdr->port_cunt) ? (uint32_t)slot->stride_b : 0;
            out_word[4 + 3 * 8 + pm] = (port < hdr->port_cunt && mode < slot->mode_cunt) ? (uint32_t)slot->grid_a : 0;
            out_word[4 + 4 * 8 + pm] = (port < hdr->port_cunt && mode < slot->mode_cunt) ? (uint32_t)slot->grid_b : 0;
            out_word[4 + 5 * 8 + pm] = (port < hdr->port_cunt) ? slot->axis_a : 0;
            out_word[4 + 6 * 8 + pm] = (port < hdr->port_cunt) ? slot->axis_b : 0;
            out_word[4 + 7 * 8 + pm] = (uint32_t)(pm * 4 * hdr->face_max);
            out_val[pm] = (port < hdr->port_cunt) ? slot->face_area : 0.0;
            out_val[8 + pm] = (port < hdr->port_cunt && slot->is_forward == LDG_TRUTH_FALSE) ? (0.0 - 1.0) : 1.0;
        }
    }
    *out_pattern = ldg_math_phys_em_port_pattern_get(&ctx->region[region], 0, 0, 0);
    *out_pattern_cunt = (uint64_t)LDG_MATH_PHYS_EM_PORT_CAP * LDG_MATH_PHYS_EM_PORT_MODE_CAP * 4 * hdr->face_max;
    *out_pm_cunt = out_word[0];
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_em_port_batch_push(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *span, uint64_t span_ticks)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    uint64_t tick = 0;
    uint64_t port = 0;
    uint64_t mode = 0;
    uint64_t pm = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx || !span)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (hdr->port_cunt == 0) { return LDG_ERR_AOK; }

    for (tick = 0; tick < span_ticks; tick++)
    {
        for (port = 0; port < hdr->port_cunt; port++) { for (mode = 0; mode < hdr->port_slot[port].mode_cunt; mode++)
            {
                pm = port * LDG_MATH_PHYS_EM_PORT_MODE_CAP + mode;
                ldg_math_phys_em_port_tap_ring_get(&ctx->region[region], port, mode, 0)[hdr->port_tap_hd] = span[tick * LDG_MATH_PHYS_EM_PORT_CAP * LDG_MATH_PHYS_EM_PORT_MODE_CAP * 2 + pm * 2];
                ldg_math_phys_em_port_tap_ring_get(&ctx->region[region], port, mode, 1)[hdr->port_tap_hd] = span[tick * LDG_MATH_PHYS_EM_PORT_CAP * LDG_MATH_PHYS_EM_PORT_MODE_CAP * 2 + pm * 2 + 1];
            }
        }
        hdr->port_tap_hd = (hdr->port_tap_hd + 1) % hdr->port_tap_depth;
        if (hdr->port_tap_cunt < hdr->port_tap_depth) { hdr->port_tap_cunt++; }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_em_induct_solve(ldg_math_phys_ctx_t *ctx, uint32_t region, double tol, uint64_t iter_max, double *out)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    double link_val = 0.0;
    uint64_t excited = 0;
    uint64_t tgt = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    if (LDG_UNLIKELY(hdr->conductor_cap == 0)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(hdr->nz != 1)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(hdr->conductor_cunt == 0)) { return LDG_ERR_EMPTY; }

    for (excited = 0; excited < hdr->conductor_cunt; excited++)
    {
        err = ldg_math_phys_em_magnetostatic_solve(ldg_math_phys_em_potential_get(&ctx->region[region]), ldg_math_phys_em_electrostatic_resid_get(&ctx->region[region]), ldg_math_phys_em_electrostatic_dir_get(&ctx->region[region]), ldg_math_phys_em_electrostatic_a_dir_get(&ctx->region[region]), ldg_math_phys_em_conductor_get(&ctx->region[region]), ldg_math_phys_em_mtl_get(&ctx->region[region]), ldg_math_phys_em_coeff_get(&ctx->region[region]), hdr->nx, hdr->ny, ctx->region[region].desc.spacing, (uint32_t)excited, tol, iter_max);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        for (tgt = 0; tgt < hdr->conductor_cunt; tgt++)
        {
            err = ldg_math_phys_em_magnetostatic_link_get(ldg_math_phys_em_potential_get(&ctx->region[region]), ldg_math_phys_em_conductor_get(&ctx->region[region]), hdr->cell_cunt, (uint32_t)tgt, &link_val);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            out[tgt * hdr->conductor_cunt + excited] = link_val;
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_region_info_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, ldg_math_phys_compute_region_info_t *out)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(out, sizeof(ldg_math_phys_compute_region_info_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    hdr = ldg_math_phys_em_hdr_get(&ctx->region[region]);
    out->electric = ldg_math_phys_em_electric_get(&ctx->region[region]);
    out->magnetic = ldg_math_phys_em_magnetic_get(&ctx->region[region]);
    out->mtl = ldg_math_phys_em_mtl_get(&ctx->region[region]);
    out->coeff = ldg_math_phys_em_coeff_get(&ctx->region[region]);
    out->stretch = ldg_math_phys_em_stretch_get(&ctx->region[region]);
    out->drive = hdr->drive_slot;
    out->spacing = ctx->region[region].desc.spacing;
    out->dt_sub = hdr->dt_sub;
    out->nx = hdr->nx;
    out->ny = hdr->ny;
    out->nz = hdr->nz;
    out->cell_cunt = hdr->cell_cunt;
    out->mtl_cunt = hdr->mtl_cunt;
    out->pml_depth = hdr->pml_depth;
    out->drive_cunt = hdr->drive_cunt;
    out->tick_cuntr = hdr->tick_cuntr;
    out->subcycle_cunt = hdr->subcycle_cunt;
    out->face_cunt = hdr->face_cunt;
    out->plane_armed = hdr->plane_armed;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_compute_commit(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt)
{
    ldg_math_phys_em_hdr_t *hdr = 0x0;
    ldg_math_phys_region_t *region_ptr = 0x0;
    ldg_math_phys_conserved_t quantized = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t stamp = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t span = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t tick_val = LDG_STRUCT_ZERO_INIT;
    double *snapshot = 0x0;
    double energy_val = 0.0;
    uint64_t slot = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(tick_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_em_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_ptr = &ctx->region[region];
    hdr = ldg_math_phys_em_hdr_get(region_ptr);
    hdr->tick_cuntr += tick_cunt;
    err = ldg_math_phys_field_em_energy_get(ldg_math_phys_em_electric_get(region_ptr), ldg_math_phys_em_magnetic_get(region_ptr), ldg_math_phys_em_mtl_get(region_ptr), ldg_math_phys_em_coeff_get(region_ptr), hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, &energy_val);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    err = ldg_math_phys_double_to_conserved_convert(energy_val, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY), &quantized);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_region_total_cum(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY, &quantized);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (hdr->snapshot_cunt > 0) { stamp = ldg_math_phys_em_stamp_get(region_ptr)[(hdr->snapshot_hd + hdr->snapshot_depth - 1) % hdr->snapshot_depth]; }
    else { stamp = ctx->clock; }

    tick_val.limb[0] = tick_cunt;
    err = ldg_arith_uint128_mul(&ctx->conf.tick_budget, &tick_val, &span);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint128_add(&stamp, &span, &stamp);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    slot = hdr->snapshot_hd;
    snapshot = ldg_math_phys_em_snapshot_get(region_ptr, slot);
    ldg_math_phys_buff_copy(snapshot, ldg_math_phys_em_electric_get(region_ptr), 3 * hdr->cell_cunt);
    ldg_math_phys_buff_copy(snapshot + 3 * hdr->cell_cunt, ldg_math_phys_em_magnetic_get(region_ptr), 3 * hdr->cell_cunt);
    ldg_math_phys_em_stamp_get(region_ptr)[slot] = stamp;
    if (hdr->snapshot_cunt < hdr->snapshot_depth) { hdr->snapshot_cunt++; }

    LDG_ST_RELEASE(hdr->snapshot_hd, (slot + 1) % hdr->snapshot_depth);
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_acoustic_compute_commit(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt)
{
    ldg_math_phys_acoustic_hdr_t *hdr = 0x0;
    ldg_math_phys_region_t *region_ptr = 0x0;
    ldg_math_phys_conserved_t quantized = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t stamp = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t span = LDG_STRUCT_ZERO_INIT;
    ldg_uint128_t tick_val = LDG_STRUCT_ZERO_INIT;
    double *snapshot = 0x0;
    double energy_val = 0.0;
    uint64_t slot = 0;
    uint32_t err = 0;
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(tick_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_acoustic_region_ck(ctx, region);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    region_ptr = &ctx->region[region];
    hdr = ldg_math_phys_acoustic_hdr_get(region_ptr);
    hdr->tick_cuntr += tick_cunt;
    err = ldg_math_phys_field_acoustic_energy_get(ldg_math_phys_acoustic_pressure_get(region_ptr), ldg_math_phys_acoustic_velocity_get(region_ptr), ldg_math_phys_acoustic_mtl_get(region_ptr), ldg_math_phys_acoustic_coeff_get(region_ptr), hdr->nx, hdr->ny, hdr->nz, region_ptr->desc.spacing, &energy_val);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return LDG_ERR_MATH_PHYS_DOMAIN; }

    err = ldg_math_phys_double_to_conserved_convert(energy_val, ldg_math_phys_region_exponent_get(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY), &quantized);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_region_total_cum(region_ptr, LDG_MATH_PHYS_QUANTITY_ENERGY, &quantized);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (hdr->snapshot_cunt > 0) { stamp = ldg_math_phys_acoustic_stamp_get(region_ptr)[(hdr->snapshot_hd + hdr->snapshot_depth - 1) % hdr->snapshot_depth]; }
    else { stamp = ctx->clock; }

    tick_val.limb[0] = tick_cunt;
    err = ldg_arith_uint128_mul(&ctx->conf.tick_budget, &tick_val, &span);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_arith_uint128_add(&stamp, &span, &stamp);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    slot = hdr->snapshot_hd;
    snapshot = ldg_math_phys_acoustic_snapshot_get(region_ptr, slot);
    ldg_math_phys_buff_copy(snapshot, ldg_math_phys_acoustic_pressure_get(region_ptr), hdr->cell_cunt);
    ldg_math_phys_buff_copy(snapshot + hdr->cell_cunt, ldg_math_phys_acoustic_velocity_get(region_ptr), 3 * hdr->cell_cunt);
    ldg_math_phys_acoustic_stamp_get(region_ptr)[slot] = stamp;
    if (hdr->snapshot_cunt < hdr->snapshot_depth) { hdr->snapshot_cunt++; }

    LDG_ST_RELEASE(hdr->snapshot_hd, (slot + 1) % hdr->snapshot_depth);
    return LDG_ERR_AOK;
}
