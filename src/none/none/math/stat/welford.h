#ifndef LDG_MATH_STAT_WELFORD_H
#define LDG_MATH_STAT_WELFORD_H

#include <stdint.h>

#include <dangling/math/stat.h>

void ldg_math_stat_welford_m2_update(uint64_t *cunt, double *mean, double *m2, double val);

#endif
