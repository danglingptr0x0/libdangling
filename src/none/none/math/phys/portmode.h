#ifndef LDG_MATH_PHYS_PORTMODE_H
#define LDG_MATH_PHYS_PORTMODE_H

#include <stdint.h>

#include <dangling/math/phys.h>

uint32_t ldg_math_phys_em_portmode_solve(const double *eps_r, const double *mu_r, const uint8_t *pec, uint64_t grid_a, uint64_t grid_b, double spacing, double k0, uint32_t mode_idx, double *out_e, double *out_h, double *out_beta);

#endif
