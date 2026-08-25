#ifndef LDG_MATH_PHYS_PARTICLE_H
#define LDG_MATH_PHYS_PARTICLE_H

#include <stdint.h>

#include <dangling/math/phys.h>

uint32_t ldg_math_phys_particle_spring_impulse_get(const double *pos_a, const double *pos_b, const double *velocity_a, const double *velocity_b, double rest_len, double compliance, double damping, double dt, double *out);
uint32_t ldg_math_phys_particle_spring_energy_get(const double *pos_a, const double *pos_b, double rest_len, double compliance, double *out);
uint32_t ldg_math_phys_particle_plane_contact_get(const double *pos, const double *velocity, double mass, const ldg_math_phys_collider_desc_t *desc, double *out_impulse, double *out_shift);
uint32_t ldg_math_phys_particle_sphere_contact_get(const double *pos, const double *velocity, double mass, const ldg_math_phys_collider_desc_t *desc, double *out_impulse, double *out_shift);

#endif
