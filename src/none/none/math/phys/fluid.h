#ifndef LDG_MATH_PHYS_FLUID_H
#define LDG_MATH_PHYS_FLUID_H

#include <stdint.h>

#include <dangling/math/phys.h>

#define LDG_MATH_PHYS_FLUID_CELL_AIR 0
#define LDG_MATH_PHYS_FLUID_CELL_SOLID 1
#define LDG_MATH_PHYS_FLUID_CELL_WATER 2

#define LDG_MATH_PHYS_FLUID_PARTICLE_STRIDE 6
#define LDG_MATH_PHYS_FLUID_FRACTION_FULL 8

#define LDG_MATH_PHYS_FLUID_CLAMP_MARGIN LDG_MATH_EPSILON
#define LDG_MATH_PHYS_FLUID_CFL_FACTOR 0.45
#define LDG_MATH_PHYS_FLUID_BOUND_MARGIN 1.001
#define LDG_MATH_PHYS_FLUID_ISO_LEVEL 0.5

uint32_t ldg_math_phys_fluid_p2g_xfer(const double *particle, uint64_t particle_cunt, double *vel_x, double *vel_y, double *vel_z, double *weight_x, double *weight_y, double *weight_z, uint16_t *mask, uint64_t nx, uint64_t ny, uint64_t nz, double spacing);
uint32_t ldg_math_phys_fluid_grid_advance(double *vel_x, double *vel_y, double *vel_z, double *pressure, double *divergence, const uint16_t *mask, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double dt, double density, double gravity, uint64_t iter_cunt);
uint32_t ldg_math_phys_fluid_g2p_advance(double *particle, uint64_t particle_cunt, const double *vel_x, const double *vel_y, const double *vel_z, const double *vel_x_old, const double *vel_y_old, const double *vel_z_old, const uint16_t *mask, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double dt, double flip_ratio, uint64_t particle_lo, uint64_t particle_hi);
uint32_t ldg_math_phys_fluid_conserved_get(const double *particle, uint64_t particle_cunt, uint16_t *cunt_scratch, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double density, double *out_mass, double *out_crowd_max);
uint32_t ldg_math_phys_fluid_fraction_build(const double *particle, uint64_t particle_cunt, const uint16_t *mask, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double *out);
uint32_t ldg_math_phys_fluid_caustic_build(const double *fraction, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, const double *light_dir, double refract, double *out, double *out_escaped);

#endif
