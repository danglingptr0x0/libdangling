#ifndef LDG_MATH_PHYS_THERMO_H
#define LDG_MATH_PHYS_THERMO_H

#include <stdint.h>

#include <dangling/math/phys.h>

#define LDG_MATH_PHYS_THERMO_COEFF_STRIDE 7
#define LDG_MATH_PHYS_THERMO_COEFF_K 0
#define LDG_MATH_PHYS_THERMO_COEFF_RHO 1
#define LDG_MATH_PHYS_THERMO_COEFF_CAP 2
#define LDG_MATH_PHYS_THERMO_COEFF_EMIT 3
#define LDG_MATH_PHYS_THERMO_COEFF_ABSORB 4
#define LDG_MATH_PHYS_THERMO_COEFF_MOLAR 5
#define LDG_MATH_PHYS_THERMO_COEFF_CONVECTION 6

#define LDG_MATH_PHYS_STEFAN_BOLTZMANN 5.670374419e-8
#define LDG_MATH_PHYS_BOLTZMANN 1.380649e-23
#define LDG_MATH_PHYS_AVOGADRO 6.02214076e23

uint32_t ldg_math_phys_thermo_conduct_advance(double *temp_next, const double *temp, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double dt);
uint32_t ldg_math_phys_thermo_emission_build(double *emission, const double *temp, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing);
uint32_t ldg_math_phys_thermo_xchg_advance(double *temp, const double *flux, const double *emission, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double dt);
uint32_t ldg_math_phys_thermo_convect_advance(double *temp_next, const double *temp, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, double dt, double ambient, double *out_escaped);
uint32_t ldg_math_phys_thermo_pwr_advance(double *temp_next, const uint16_t *mtl, const double *coeff, const double *pwr, uint64_t cell_cunt, double spacing, double dt, double *out_injected);

#endif
