#ifndef LDG_MATH_PHYS_ELECTROSTATIC_H
#define LDG_MATH_PHYS_ELECTROSTATIC_H

#include <stdint.h>

#include <dangling/math/phys.h>

#define LDG_MATH_PHYS_ELECTROSTATIC_FREE UINT16_MAX

uint32_t ldg_math_phys_em_electrostatic_solve(double *potential, double *resid, double *dir, double *a_dir, const uint16_t *conductor, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint32_t excited, uint32_t coeff_sel, uint8_t vacuum_is, double tol, uint64_t iter_max);
uint32_t ldg_math_phys_em_electrostatic_charge_get(const double *potential, const uint16_t *conductor, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint32_t tgt, uint32_t coeff_sel, uint8_t vacuum_is, double *out);
uint32_t ldg_math_phys_em_electrostatic_perimeter_get(const uint16_t *conductor, uint64_t nx, uint64_t ny, uint64_t nz, double spacing, uint32_t tgt, double *out_perimeter, uint64_t *out_cell_cunt);

#endif
