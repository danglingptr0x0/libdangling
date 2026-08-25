
#include <dangling/core/err.h>
#include <dangling/math/math.h>

#include "particle.h"

static double ldg_math_phys_particle_dot_get(const double *lhs, const double *rhs)
{
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}

uint32_t ldg_math_phys_particle_spring_impulse_get(const double *pos_a, const double *pos_b, const double *velocity_a, const double *velocity_b, double rest_len, double compliance, double damping, double dt, double *out)
{
    double diff[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double rate_vec[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double len = 0.0;
    double rate = 0.0;
    double factor = 0.0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!pos_a)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos_b)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!velocity_a)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!velocity_b)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { out[axis] = 0.0; }
    if (LDG_UNLIKELY(!ldg_math_finite_is(rest_len) || rest_len < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(compliance) || compliance <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(damping) || damping < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(dt) || dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        diff[axis] = pos_b[axis] - pos_a[axis];
        rate_vec[axis] = velocity_b[axis] - velocity_a[axis];
    }
    len = ldg_math_sqrt_get(ldg_math_phys_particle_dot_get(diff, diff));
    if (len <= 0.0) { return LDG_ERR_AOK; }

    rate = ldg_math_phys_particle_dot_get(rate_vec, diff) / len;
    factor = ((len - rest_len) / compliance + damping * rate) * dt / len;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { out[axis] = diff[axis] * factor; }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_particle_spring_energy_get(const double *pos_a, const double *pos_b, double rest_len, double compliance, double *out)
{
    double diff[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double stretch = 0.0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!pos_a)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!pos_b)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!ldg_math_finite_is(rest_len) || rest_len < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(compliance) || compliance <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { diff[axis] = pos_b[axis] - pos_a[axis]; }
    stretch = ldg_math_sqrt_get(ldg_math_phys_particle_dot_get(diff, diff)) - rest_len;
    *out = 0.5 * stretch * stretch / compliance;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_particle_plane_contact_get(const double *pos, const double *velocity, double mass, const ldg_math_phys_collider_desc_t *desc, double *out_impulse, double *out_shift)
{
    double unit[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double norm_len = 0.0;
    double depth = 0.0;
    double vel_normal = 0.0;
    double impulse_mag = 0.0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!velocity)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_impulse)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_shift)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        out_impulse[axis] = 0.0;
        out_shift[axis] = 0.0;
    }
    if (LDG_UNLIKELY(!ldg_math_finite_is(mass) || mass <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    norm_len = ldg_math_sqrt_get(ldg_math_phys_particle_dot_get(desc->normal, desc->normal));
    if (LDG_UNLIKELY(!ldg_math_finite_is(norm_len) || norm_len <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { unit[axis] = desc->normal[axis] / norm_len; }
    depth = desc->offset - ldg_math_phys_particle_dot_get(pos, unit);
    if (depth <= 0.0) { return LDG_ERR_AOK; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { out_shift[axis] = unit[axis] * depth; }
    vel_normal = ldg_math_phys_particle_dot_get(velocity, unit);
    if (vel_normal >= 0.0) { return LDG_ERR_AOK; }

    impulse_mag = (0.0 - (1.0 + desc->restitution)) * mass * vel_normal;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { out_impulse[axis] = unit[axis] * impulse_mag; }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_particle_sphere_contact_get(const double *pos, const double *velocity, double mass, const ldg_math_phys_collider_desc_t *desc, double *out_impulse, double *out_shift)
{
    double diff[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double unit[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double dist = 0.0;
    double depth = 0.0;
    double vel_normal = 0.0;
    double impulse_mag = 0.0;
    uint32_t axis = 0;
    if (LDG_UNLIKELY(!pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!velocity)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_impulse)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_shift)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        out_impulse[axis] = 0.0;
        out_shift[axis] = 0.0;
    }
    if (LDG_UNLIKELY(!ldg_math_finite_is(mass) || mass <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(desc->radius) || desc->radius <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { diff[axis] = pos[axis] - desc->center[axis]; }
    dist = ldg_math_sqrt_get(ldg_math_phys_particle_dot_get(diff, diff));
    if (dist >= desc->radius) { return LDG_ERR_AOK; }

    if (dist > 0.0) { for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { unit[axis] = diff[axis] / dist; } }
    else { unit[2] = 1.0; }

    depth = desc->radius - dist;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { out_shift[axis] = unit[axis] * depth; }
    vel_normal = ldg_math_phys_particle_dot_get(velocity, unit);
    if (vel_normal >= 0.0) { return LDG_ERR_AOK; }

    impulse_mag = (0.0 - (1.0 + desc->restitution)) * mass * vel_normal;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { out_impulse[axis] = unit[axis] * impulse_mag; }
    return LDG_ERR_AOK;
}
