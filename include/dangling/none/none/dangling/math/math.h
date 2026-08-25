#ifndef LDG_MATH_MATH_H
#define LDG_MATH_MATH_H

#include <stdint.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_MATH_PI 3.14159265358979323846
#define LDG_MATH_TAU 6.28318530717958647692
#define LDG_MATH_HALF_PI 1.57079632679489661923
#define LDG_MATH_NEG_HALF_PI (-LDG_MATH_HALF_PI)
#define LDG_MATH_QUARTER_PI 0.78539816339744830962
#define LDG_MATH_SIXTH_PI 0.52359877559829887308
#define LDG_MATH_TWO_OVER_PI 0.63661977236758134308
#define LDG_MATH_SQRT2 1.41421356237309504880
#define LDG_MATH_SQRT3 1.73205080756887729353
#define LDG_MATH_SQRT5 2.23606797749978969641
#define LDG_MATH_SQRT_HALF 0.70710678118654752440
#define LDG_MATH_LN2 0.69314718055994530942
#define LDG_MATH_LN10 2.30258509299404568402
#define LDG_MATH_LOG2_E 1.44269504088896340736
#define LDG_MATH_E 2.71828182845904523536
#define LDG_MATH_DEG_TO_RAD 0.017453292519943295769
#define LDG_MATH_RAD_TO_DEG 57.295779513082320877

#define LDG_MATH_EPSILON 1e-9
#define LDG_MATH_INFINITY __builtin_inf()
#define LDG_MATH_EXP_ARG_MAX 709.782712893384
#define LDG_MATH_EXP_ARG_MIN -745.133219101941

LDG_EXPORT uint8_t ldg_math_nan_is(double val);
LDG_EXPORT uint8_t ldg_math_inf_is(double val);
LDG_EXPORT uint8_t ldg_math_finite_is(double val);

LDG_EXPORT double ldg_math_abs_get(double val);
LDG_EXPORT double ldg_math_floor_get(double val);
LDG_EXPORT double ldg_math_ceil_get(double val);
LDG_EXPORT double ldg_math_trunc_get(double val);
LDG_EXPORT double ldg_math_min_get(double lhs, double rhs);
LDG_EXPORT double ldg_math_max_get(double lhs, double rhs);
LDG_EXPORT double ldg_math_sqrt_get(double val);
LDG_EXPORT double ldg_math_pwr2_scale_get(double val, int32_t exponent);
LDG_EXPORT double ldg_math_next_toward_get(double val, double toward);
LDG_EXPORT double ldg_math_hypotenuse_get(double lhs, double rhs);
LDG_EXPORT double ldg_math_exp_get(double val);
LDG_EXPORT double ldg_math_log_get(double val);
LDG_EXPORT double ldg_math_pwr_get(double base, double exponent);
LDG_EXPORT double ldg_math_sin_get(double val);
LDG_EXPORT double ldg_math_cos_get(double val);
LDG_EXPORT double ldg_math_arctan_get(double val);
LDG_EXPORT double ldg_math_arctan2_get(double num, double den);
LDG_EXPORT double ldg_math_arcsin_get(double val);
LDG_EXPORT double ldg_math_arccos_get(double val);
LDG_EXPORT double ldg_math_tan_get(double val);
LDG_EXPORT double ldg_math_log10_get(double val);
LDG_EXPORT double ldg_math_remainder_get(double numer, double denom);
LDG_EXPORT uint32_t ldg_math_fraction_split(double val, double *out_fraction, int32_t *out_exponent);
LDG_EXPORT uint32_t ldg_math_whole_split(double val, double *out_whole, double *out_fraction);
LDG_EXPORT uint32_t ldg_math_int64_abs_get(int64_t val, int64_t *out);

#endif
