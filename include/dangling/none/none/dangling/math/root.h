#ifndef LDG_MATH_ROOT_H
#define LDG_MATH_ROOT_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_MATH_ROOT_ITER_MAX 100
#define LDG_MATH_ROOT_TOL_MIN 1e-15

typedef double (*ldg_math_root_func_cb_t)(double x, void *ctx);

LDG_EXPORT uint32_t ldg_math_root_brent_compute(ldg_math_root_func_cb_t func, void *ctx, double lo, double hi, double tol, uint32_t iter_max, double *out);

#endif
