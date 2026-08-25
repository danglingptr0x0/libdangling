#ifndef LDG_MATH_PHYS_MAGNETOSTATIC_H
#define LDG_MATH_PHYS_MAGNETOSTATIC_H

#include <stdint.h>

#include <dangling/math/phys.h>

uint32_t ldg_math_phys_em_magnetostatic_solve(double *potential, double *resid, double *dir, double *a_dir, const uint16_t *conductor, const uint16_t *mtl, const double *coeff, uint64_t nx, uint64_t ny, double spacing, uint32_t excited, double tol, uint64_t iter_max);
uint32_t ldg_math_phys_em_magnetostatic_link_get(const double *potential, const uint16_t *conductor, uint64_t cell_cunt, uint32_t tgt, double *out);

#endif
