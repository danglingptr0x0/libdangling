#ifndef LDG_MATH_PHYS_QUERY_H
#define LDG_MATH_PHYS_QUERY_H

#include <stdint.h>

#include <dangling/math/phys.h>

uint32_t ldg_math_phys_sweep_solve(const ldg_math_phys_sweep_desc_t *sweep, const ldg_math_phys_collider_desc_t *tgt, ldg_math_phys_sweep_hit_t *out, uint8_t *hit);

#endif
