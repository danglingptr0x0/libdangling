#ifndef LDG_MATH_PHYS_KERNEL_H
#define LDG_MATH_PHYS_KERNEL_H

#include <stdint.h>

#include <dangling/math/phys.h>

typedef struct ldg_math_phys_dist_constraint
{
    uint32_t idx_a;
    uint32_t idx_b;
    double rest_len;
    double compliance;
} ldg_math_phys_dist_constraint_t;

uint32_t ldg_math_phys_integrate(double *pos, double *prev, const double *accel, const double *inv_mass, uint64_t cunt, double dt, double damping);
uint32_t ldg_math_phys_force_cum(double *accel, uint64_t cunt, const double *gravity, const double *wind);
uint32_t ldg_math_phys_plane_collide(double *pos, double *prev, const double *inv_mass, uint64_t cunt, const double *normal, double offset, double restitution);
uint32_t ldg_math_phys_sphere_collide(double *pos, double *prev, const double *inv_mass, uint64_t cunt, const double *center, double radius, double restitution);
uint32_t ldg_math_phys_box_collide(double *pos, double *prev, const double *inv_mass, uint64_t cunt, const double *center, const double *half, const double *axes, double restitution);
uint32_t ldg_math_phys_capsule_collide(double *pos, double *prev, const double *inv_mass, uint64_t cunt, const double *point_a, const double *point_b, double radius, double restitution);
uint32_t ldg_math_phys_xpbd_dist_solve(double *pos, const double *inv_mass, uint64_t cunt, const ldg_math_phys_dist_constraint_t *constraint, double *lambda, uint64_t constraint_cunt, double dt, uint32_t iter);
uint32_t ldg_math_phys_field_2d_advance(const double *cur, const double *prev, double *next, uint64_t nx, uint64_t ny, double spacing, double wave_speed, double dt, double damping, uint32_t boundary);
uint32_t ldg_math_phys_field_2d_sample_get(const double *field, uint64_t nx, uint64_t ny, double spacing, double pos_x, double pos_y, double *out);
uint32_t ldg_math_phys_field_2d_src_add(double *field, uint64_t nx, uint64_t ny, double spacing, double pos_x, double pos_y, double amount);
uint32_t ldg_math_phys_field_3d_advance(const double *cur, const double *prev, double *next, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double wave_speed, double dt, double damping, uint32_t boundary);
uint32_t ldg_math_phys_field_3d_sample_get(const double *field, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double pos_x, double pos_y, double pos_z, double *out);
uint32_t ldg_math_phys_field_3d_src_add(double *field, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double pos_x, double pos_y, double pos_z, double amount);

#endif
