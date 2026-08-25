#ifndef LDG_MATH_NURBS_H
#define LDG_MATH_NURBS_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_MATH_NURBS_MAX_DEGREE 16

LDG_EXPORT uint32_t ldg_math_nurbs_span_find(uint32_t degree, const double *knot, uint32_t knot_cunt, double param, uint32_t *out);

LDG_EXPORT uint32_t ldg_math_nurbs_basis_eval(uint32_t degree, const double *knot, uint32_t span, double param, double *out);

LDG_EXPORT uint32_t ldg_math_nurbs_curve_point_eval(uint32_t degree, const double *knot, uint32_t knot_cunt, const double *control, const double *weight, uint32_t control_cunt, double param, double *out);

LDG_EXPORT uint32_t ldg_math_nurbs_curve_derivative_eval(uint32_t degree, const double *knot, uint32_t knot_cunt, const double *control, const double *weight, uint32_t control_cunt, double param, double *out_point, double *out_derivative);

LDG_EXPORT uint32_t ldg_math_nurbs_surface_point_eval(uint32_t degree_u, const double *knot_u, uint32_t knot_u_cunt, uint32_t degree_v, const double *knot_v, uint32_t knot_v_cunt, const double *control, const double *weight, uint32_t cunt_u, uint32_t cunt_v, double param_u, double param_v, double *out);

LDG_EXPORT uint32_t ldg_math_nurbs_surface_derivative_eval(uint32_t degree_u, const double *knot_u, uint32_t knot_u_cunt, uint32_t degree_v, const double *knot_v, uint32_t knot_v_cunt, const double *control, const double *weight, uint32_t cunt_u, uint32_t cunt_v, double param_u, double param_v, double *out_point, double *out_derivative_u, double *out_derivative_v);

LDG_EXPORT uint32_t ldg_math_nurbs_curve_point_invert(uint32_t degree, const double *knot, uint32_t knot_cunt, const double *control, const double *weight, uint32_t control_cunt, const double *target, double param_init, double tol, uint32_t max_iter, double *out);

LDG_EXPORT uint32_t ldg_math_nurbs_surface_point_invert(uint32_t degree_u, const double *knot_u, uint32_t knot_u_cunt, uint32_t degree_v, const double *knot_v, uint32_t knot_v_cunt, const double *control, const double *weight, uint32_t cunt_u, uint32_t cunt_v, const double *target, double param_u_init, double param_v_init, double tol, uint32_t max_iter, double *out_u, double *out_v);

#endif
