#ifndef LDG_MATH_PHYS_FIELD_H
#define LDG_MATH_PHYS_FIELD_H

#include <stdint.h>

#include <dangling/math/phys.h>

#define LDG_MATH_PHYS_EM_COEFF_STRIDE 6
#define LDG_MATH_PHYS_EM_COEFF_EPSILON 0
#define LDG_MATH_PHYS_EM_COEFF_MU 1
#define LDG_MATH_PHYS_EM_COEFF_SIGMA 2
#define LDG_MATH_PHYS_EM_COEFF_CA 3
#define LDG_MATH_PHYS_EM_COEFF_CB 4
#define LDG_MATH_PHYS_EM_COEFF_DB 5

#define LDG_MATH_PHYS_EM_STRETCH_STRIDE 6
#define LDG_MATH_PHYS_EM_STRETCH_KAPPA_INT 0
#define LDG_MATH_PHYS_EM_STRETCH_B_INT 1
#define LDG_MATH_PHYS_EM_STRETCH_C_INT 2
#define LDG_MATH_PHYS_EM_STRETCH_KAPPA_HALF 3
#define LDG_MATH_PHYS_EM_STRETCH_B_HALF 4
#define LDG_MATH_PHYS_EM_STRETCH_C_HALF 5

#define LDG_MATH_PHYS_EM_PSI_EXY 0
#define LDG_MATH_PHYS_EM_PSI_EXZ 1
#define LDG_MATH_PHYS_EM_PSI_EYZ 2
#define LDG_MATH_PHYS_EM_PSI_EYX 3
#define LDG_MATH_PHYS_EM_PSI_EZX 4
#define LDG_MATH_PHYS_EM_PSI_EZY 5
#define LDG_MATH_PHYS_EM_PSI_HXY 6
#define LDG_MATH_PHYS_EM_PSI_HXZ 7
#define LDG_MATH_PHYS_EM_PSI_HYZ 8
#define LDG_MATH_PHYS_EM_PSI_HYX 9
#define LDG_MATH_PHYS_EM_PSI_HZX 10
#define LDG_MATH_PHYS_EM_PSI_HZY 11
#define LDG_MATH_PHYS_EM_PSI_CUNT 12

#define LDG_MATH_PHYS_EM_PHASE_FAR 1.0e30

#define LDG_MATH_PHYS_EM_EVAL_WAVE 0
#define LDG_MATH_PHYS_EM_EVAL_RAY 1

#define LDG_MATH_PHYS_PML_GRADE 3.0
#define LDG_MATH_PHYS_PML_KAPPA_MAX 1.0
#define LDG_MATH_PHYS_PML_ALPHA_MAX 0.2
#define LDG_MATH_PHYS_PML_SIGMA_SCALE 0.8

#define LDG_MATH_PHYS_ACOUSTIC_COEFF_STRIDE 6
#define LDG_MATH_PHYS_ACOUSTIC_COEFF_DENSITY 0
#define LDG_MATH_PHYS_ACOUSTIC_COEFF_SPEED 1
#define LDG_MATH_PHYS_ACOUSTIC_COEFF_LOSS 2
#define LDG_MATH_PHYS_ACOUSTIC_COEFF_CA 3
#define LDG_MATH_PHYS_ACOUSTIC_COEFF_CB 4
#define LDG_MATH_PHYS_ACOUSTIC_COEFF_CV 5

#define LDG_MATH_PHYS_ACOUSTIC_STRETCH_STRIDE 6
#define LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_INT 0
#define LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_INT 1
#define LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_INT 2
#define LDG_MATH_PHYS_ACOUSTIC_STRETCH_KAPPA_HALF 3
#define LDG_MATH_PHYS_ACOUSTIC_STRETCH_B_HALF 4
#define LDG_MATH_PHYS_ACOUSTIC_STRETCH_C_HALF 5

#define LDG_MATH_PHYS_ACOUSTIC_PSI_PX 0
#define LDG_MATH_PHYS_ACOUSTIC_PSI_PY 1
#define LDG_MATH_PHYS_ACOUSTIC_PSI_PZ 2
#define LDG_MATH_PHYS_ACOUSTIC_PSI_VX 3
#define LDG_MATH_PHYS_ACOUSTIC_PSI_VY 4
#define LDG_MATH_PHYS_ACOUSTIC_PSI_VZ 5
#define LDG_MATH_PHYS_ACOUSTIC_PSI_CUNT 6

#define LDG_MATH_PHYS_ACOUSTIC_PML_GRADE 2.0
#define LDG_MATH_PHYS_ACOUSTIC_PML_KAPPA_MAX 1.0
#define LDG_MATH_PHYS_ACOUSTIC_PML_RCOEF 0.001

#define LDG_MATH_PHYS_ACOUSTIC_MONOPOLE_CAP 32
#define LDG_MATH_PHYS_ACOUSTIC_MONOPOLE_RADIUS_CELL 3.0
#define LDG_MATH_PHYS_ACOUSTIC_MONOPOLE_ALPHA 0.7
#define LDG_MATH_PHYS_ACOUSTIC_RAMP_PERIOD_CUNT 4.0
#define LDG_MATH_PHYS_ACOUSTIC_GPU_SRC_MAX 4

uint32_t ldg_math_phys_field_energy_get(const double *cur, const double *prev, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double wave_speed, double dt, double *out);
uint32_t ldg_math_phys_field_em_electric_advance(double *electric, const double *magnetic, const double *drive, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint64_t slab_lo, uint64_t slab_hi);
uint32_t ldg_math_phys_field_em_magnetic_advance(double *magnetic, const double *electric, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint64_t slab_lo, uint64_t slab_hi);
uint32_t ldg_math_phys_field_em_electric_2d_advance(double *electric, const double *magnetic, const double *drive, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, double spacing);
uint32_t ldg_math_phys_field_em_magnetic_2d_advance(double *magnetic, const double *electric, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, double spacing);
uint32_t ldg_math_phys_field_em_energy_get(const double *electric, const double *magnetic, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double *out);
uint32_t ldg_math_phys_field_em_phase_solve(double *phase, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double tol, uint64_t pass_max, uint64_t *out);
uint32_t ldg_math_phys_field_em_intensity_solve(double *intensity, double *reflect, const double *phase, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double tol, uint64_t pass_max, uint64_t *out);
uint32_t ldg_math_phys_field_em_intensity_conserved_get(const double *intensity, const double *reflect, const double *phase, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double *out_absorbed, double *out_escaped, double *out_reflected);
uint32_t ldg_math_phys_field_em_radiance_solve(double *radiance, double *flux, const double *emission, const uint16_t *mtl, const double *coeff, const double *scatter, const double *ordinate, uint64_t ordinate_cunt, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double tol, uint64_t pass_max, uint64_t *out);
uint32_t ldg_math_phys_field_em_radiance_conserved_get(const double *radiance, const double *emission, const uint16_t *mtl, const double *coeff, const double *ordinate, uint64_t ordinate_cunt, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double *out_absorbed, double *out_escaped, double *out_emitted);
uint32_t ldg_math_phys_field_em_bounce_solve(double *phase_next, double *intensity_next, double *reflect_next, const double *reflect, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double tol, uint64_t pass_max, uint64_t *out);
uint32_t ldg_math_phys_field_em_eval_sel(double spacing, double lambda, double wave_err_max, double ray_err_max, uint32_t *out);
uint32_t ldg_math_phys_field_acoustic_velocity_advance(double *velocity, const double *pressure, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint64_t slab_lo, uint64_t slab_hi);
uint32_t ldg_math_phys_field_acoustic_pressure_advance(double *pressure, const double *velocity, const double *drive, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint64_t slab_lo, uint64_t slab_hi);
uint32_t ldg_math_phys_field_acoustic_velocity_2d_advance(double *velocity, const double *pressure, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, double spacing);
uint32_t ldg_math_phys_field_acoustic_pressure_2d_advance(double *pressure, const double *velocity, const double *drive, double *psi, const uint16_t *mtl, const double *coeff, const double *stretch, uint64_t nx, uint64_t ny, double spacing);
uint32_t ldg_math_phys_field_acoustic_energy_get(const double *pressure, const double *velocity, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double *out);

#endif
