#ifndef LDG_MATH_QUATERNION_H
#define LDG_MATH_QUATERNION_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/math.h>

#define LDG_MATH_QUATERNION_DIM 4
#define LDG_MATH_QUATERNION_VEC_DIM 3

LDG_EXPORT uint32_t ldg_math_quaternion_ident_compute(double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_from_axis_angle_compute(const double *axis, double angle, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_to_axis_angle_compute(const double *quaternion, double *out_axis, double *out_angle);
LDG_EXPORT uint32_t ldg_math_quaternion_mul_compute(const double *lhs, const double *rhs, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_conjugate(const double *quaternion, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_invert(const double *quaternion, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_normalize(const double *quaternion, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_dot_get(const double *lhs, const double *rhs, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_norm_get(const double *quaternion, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_vec3_rot(const double *quaternion, const double *vec, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_to_mat3_compute(const double *quaternion, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_from_mat3_compute(const double *mat, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_slerp_compute(const double *lhs, const double *rhs, double param, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_nlerp_compute(const double *lhs, const double *rhs, double param, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_integrate(double *quaternion, const double *omega, double dt);
LDG_EXPORT uint32_t ldg_math_quaternion_from_euler_compute(const double *euler, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_to_euler_compute(const double *quaternion, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_from_two_vec3_compute(const double *src, const double *dst, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_exp_compute(const double *quaternion, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_log_compute(const double *quaternion, double *out);
LDG_EXPORT uint32_t ldg_math_quaternion_squad_compute(const double *lhs, const double *control_a, const double *control_b, const double *rhs, double param, double *out);

#endif
