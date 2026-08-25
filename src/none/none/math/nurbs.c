#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/nurbs.h>

#define LDG_MATH_NURBS_STEP 1
#define LDG_MATH_NURBS_PAIR_STEP 2

static void nurbs_basis_fill(uint32_t degree, const double *knot, uint32_t span, double param, double *out)
{
    double lhs[LDG_MATH_NURBS_MAX_DEGREE + 1] = { 0.0 };
    double rhs[LDG_MATH_NURBS_MAX_DEGREE + 1] = { 0.0 };
    double saved = 0.0;
    double temp = 0.0;
    uint32_t lvl = 0;
    uint32_t pos = 0;

    out[0] = 1.0;
    for (lvl = 1; lvl <= degree; lvl++)
    {
        lhs[lvl] = param - knot[span + 1 - lvl];
        rhs[lvl] = knot[span + lvl] - param;
        saved = 0.0;
        for (pos = 0; pos < lvl; pos++)
        {
            temp = out[pos] / (rhs[pos + 1] + lhs[lvl - pos]);
            out[pos] = saved + rhs[pos + 1] * temp;
            saved = lhs[lvl - pos] * temp;
        }
        out[lvl] = saved;
    }
}

static void nurbs_basis_derivative_fill(uint32_t degree, const double *knot, uint32_t span, double param, double *basis_out, double *derivative_out)
{
    double basis_low[LDG_MATH_NURBS_MAX_DEGREE + 1] = { 0.0 };
    double denom_a = 0.0;
    double denom_b = 0.0;
    double term_a = 0.0;
    double term_b = 0.0;
    uint32_t idx = 0;

    nurbs_basis_fill(degree, knot, span, param, basis_out);
    nurbs_basis_fill(degree - 1, knot, span, param, basis_low);
    for (idx = 0; idx <= degree; idx++)
    {
        denom_a = knot[span + idx] - knot[span - degree + idx];
        denom_b = knot[span + idx + 1] - knot[span - degree + idx + 1];
        term_a = (idx >= 1 && denom_a > 0.0) ? basis_low[idx - 1] / denom_a : 0.0;
        term_b = (idx <= degree - 1 && denom_b > 0.0) ? basis_low[idx] / denom_b : 0.0;
        derivative_out[idx] = (double)degree * (term_a - term_b);
    }
}

uint32_t ldg_math_nurbs_span_find(uint32_t degree, const double *knot, uint32_t knot_cunt, double param, uint32_t *out)
{
    uint32_t control_cunt = 0;
    uint32_t last = 0;
    uint32_t low = 0;
    uint32_t high = 0;
    uint32_t mid = 0;
    uint32_t guard = 0;

    if (LDG_UNLIKELY(!knot || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(degree == 0 || degree > LDG_MATH_NURBS_MAX_DEGREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(knot_cunt < degree + 2)) { return LDG_ERR_FUNC_ARG_INVALID; }

    control_cunt = knot_cunt - degree - LDG_MATH_NURBS_STEP;
    last = control_cunt - LDG_MATH_NURBS_STEP;

    if (LDG_UNLIKELY(last < degree)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (guard = 0; guard + LDG_MATH_NURBS_STEP < knot_cunt; guard++) { if (LDG_UNLIKELY(knot[guard] > knot[guard + LDG_MATH_NURBS_STEP])) { return LDG_ERR_FUNC_ARG_INVALID; } }

    if (LDG_UNLIKELY(param < knot[degree] || param > knot[knot_cunt - LDG_MATH_NURBS_STEP - degree])) { return LDG_ERR_BOUNDS; }

    if (param >= knot[last + LDG_MATH_NURBS_STEP])
    {
        *out = last;
        return LDG_ERR_AOK;
    }

    low = degree;
    high = last + LDG_MATH_NURBS_STEP;
    mid = (low + high) / LDG_MATH_NURBS_PAIR_STEP;
    guard = 0;

    while (param < knot[mid] || param >= knot[mid + LDG_MATH_NURBS_STEP])
    {
        if (LDG_UNLIKELY(guard >= knot_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

        guard++;

        if (param < knot[mid]) { high = mid; }
        else { low = mid; }

        mid = (low + high) / LDG_MATH_NURBS_PAIR_STEP;
    }

    if (LDG_UNLIKELY(mid < degree)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = mid;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_nurbs_basis_eval(uint32_t degree, const double *knot, uint32_t span, double param, double *out)
{
    if (LDG_UNLIKELY(!knot || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = 0.0;

    if (LDG_UNLIKELY(degree == 0 || degree > LDG_MATH_NURBS_MAX_DEGREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    nurbs_basis_fill(degree, knot, span, param, out);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_nurbs_curve_point_eval(uint32_t degree, const double *knot, uint32_t knot_cunt, const double *control, const double *weight, uint32_t control_cunt, double param, double *out)
{
    double basis[LDG_MATH_NURBS_MAX_DEGREE + 1] = { 0.0 };
    double accum[4] = { 0.0, 0.0, 0.0, 0.0 };
    double weight_val = 0.0;
    double inv = 0.0;
    uint32_t span = 0;
    uint32_t term = 0;
    uint32_t idx = 0;
    uint32_t coord = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!knot || !control || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = 0.0;
    out[1] = 0.0;
    out[2] = 0.0;

    if (LDG_UNLIKELY(degree == 0 || degree > LDG_MATH_NURBS_MAX_DEGREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(knot_cunt != control_cunt + degree + 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_nurbs_span_find(degree, knot, knot_cunt, param, &span);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    nurbs_basis_fill(degree, knot, span, param, basis);

    for (term = 0; term <= degree; term++)
    {
        idx = span - degree + term;
        weight_val = weight ? weight[idx] : 1.0;
        if (LDG_UNLIKELY(weight && weight_val <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        for (coord = 0; coord < 3; coord++) { accum[coord] += basis[term] * weight_val * control[idx * 3 + coord]; }
        accum[3] += basis[term] * weight_val;
    }

    if (LDG_UNLIKELY(accum[3] <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv = 1.0 / accum[3];
    out[0] = accum[0] * inv;
    out[1] = accum[1] * inv;
    out[2] = accum[2] * inv;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_nurbs_curve_derivative_eval(uint32_t degree, const double *knot, uint32_t knot_cunt, const double *control, const double *weight, uint32_t control_cunt, double param, double *out_point, double *out_derivative)
{
    double basis[LDG_MATH_NURBS_MAX_DEGREE + 1] = { 0.0 };
    double basis_derivative[LDG_MATH_NURBS_MAX_DEGREE + 1] = { 0.0 };
    double numerator[4] = { 0.0, 0.0, 0.0, 0.0 };
    double numerator_derivative[4] = { 0.0, 0.0, 0.0, 0.0 };
    double weight_val = 0.0;
    double inv = 0.0;
    double point = 0.0;
    uint32_t span = 0;
    uint32_t term = 0;
    uint32_t idx = 0;
    uint32_t coord = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!knot || !control || !out_point || !out_derivative)) { return LDG_ERR_FUNC_ARG_NULL; }

    out_point[0] = 0.0;
    out_point[1] = 0.0;
    out_point[2] = 0.0;
    out_derivative[0] = 0.0;
    out_derivative[1] = 0.0;
    out_derivative[2] = 0.0;

    if (LDG_UNLIKELY(degree == 0 || degree > LDG_MATH_NURBS_MAX_DEGREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(knot_cunt != control_cunt + degree + 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_nurbs_span_find(degree, knot, knot_cunt, param, &span);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    nurbs_basis_derivative_fill(degree, knot, span, param, basis, basis_derivative);

    for (term = 0; term <= degree; term++)
    {
        idx = span - degree + term;
        weight_val = weight ? weight[idx] : 1.0;
        if (LDG_UNLIKELY(weight && weight_val <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        for (coord = 0; coord < 3; coord++)
        {
            numerator[coord] += basis[term] * weight_val * control[idx * 3 + coord];
            numerator_derivative[coord] += basis_derivative[term] * weight_val * control[idx * 3 + coord];
        }
        numerator[3] += basis[term] * weight_val;
        numerator_derivative[3] += basis_derivative[term] * weight_val;
    }

    if (LDG_UNLIKELY(numerator[3] <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv = 1.0 / numerator[3];
    for (coord = 0; coord < 3; coord++)
    {
        point = numerator[coord] * inv;
        out_point[coord] = point;
        out_derivative[coord] = (numerator_derivative[coord] - numerator_derivative[3] * point) * inv;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_nurbs_surface_point_eval(uint32_t degree_u, const double *knot_u, uint32_t knot_u_cunt, uint32_t degree_v, const double *knot_v, uint32_t knot_v_cunt, const double *control, const double *weight, uint32_t cunt_u, uint32_t cunt_v, double param_u, double param_v, double *out)
{
    double basis_u[LDG_MATH_NURBS_MAX_DEGREE + 1] = { 0.0 };
    double basis_v[LDG_MATH_NURBS_MAX_DEGREE + 1] = { 0.0 };
    double accum[4] = { 0.0, 0.0, 0.0, 0.0 };
    double row[4] = { 0.0, 0.0, 0.0, 0.0 };
    double weight_val = 0.0;
    double inv = 0.0;
    uint32_t span_u = 0;
    uint32_t span_v = 0;
    uint32_t ju = 0;
    uint32_t jv = 0;
    uint32_t iu = 0;
    uint32_t iv = 0;
    uint32_t idx = 0;
    uint32_t coord = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!knot_u || !knot_v || !control || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = 0.0;
    out[1] = 0.0;
    out[2] = 0.0;

    if (LDG_UNLIKELY(degree_u == 0 || degree_u > LDG_MATH_NURBS_MAX_DEGREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(degree_v == 0 || degree_v > LDG_MATH_NURBS_MAX_DEGREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(knot_u_cunt != cunt_u + degree_u + 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(knot_v_cunt != cunt_v + degree_v + 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_nurbs_span_find(degree_u, knot_u, knot_u_cunt, param_u, &span_u);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_nurbs_span_find(degree_v, knot_v, knot_v_cunt, param_v, &span_v);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    nurbs_basis_fill(degree_u, knot_u, span_u, param_u, basis_u);
    nurbs_basis_fill(degree_v, knot_v, span_v, param_v, basis_v);

    for (ju = 0; ju <= degree_u; ju++)
    {
        iu = span_u - degree_u + ju;
        row[0] = 0.0;
        row[1] = 0.0;
        row[2] = 0.0;
        row[3] = 0.0;
        for (jv = 0; jv <= degree_v; jv++)
        {
            iv = span_v - degree_v + jv;
            idx = iu * cunt_v + iv;
            weight_val = weight ? weight[idx] : 1.0;
            if (LDG_UNLIKELY(weight && weight_val <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            for (coord = 0; coord < 3; coord++) { row[coord] += basis_v[jv] * weight_val * control[idx * 3 + coord]; }
            row[3] += basis_v[jv] * weight_val;
        }
        for (coord = 0; coord < 4; coord++) { accum[coord] += basis_u[ju] * row[coord]; }
    }

    if (LDG_UNLIKELY(accum[3] <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv = 1.0 / accum[3];
    out[0] = accum[0] * inv;
    out[1] = accum[1] * inv;
    out[2] = accum[2] * inv;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_nurbs_surface_derivative_eval(uint32_t degree_u, const double *knot_u, uint32_t knot_u_cunt, uint32_t degree_v, const double *knot_v, uint32_t knot_v_cunt, const double *control, const double *weight, uint32_t cunt_u, uint32_t cunt_v, double param_u, double param_v, double *out_point, double *out_derivative_u, double *out_derivative_v)
{
    double basis_u[LDG_MATH_NURBS_MAX_DEGREE + 1] = { 0.0 };
    double basis_u_derivative[LDG_MATH_NURBS_MAX_DEGREE + 1] = { 0.0 };
    double basis_v[LDG_MATH_NURBS_MAX_DEGREE + 1] = { 0.0 };
    double basis_v_derivative[LDG_MATH_NURBS_MAX_DEGREE + 1] = { 0.0 };
    double surface[4] = { 0.0, 0.0, 0.0, 0.0 };
    double surface_u[4] = { 0.0, 0.0, 0.0, 0.0 };
    double surface_v[4] = { 0.0, 0.0, 0.0, 0.0 };
    double row[4] = { 0.0, 0.0, 0.0, 0.0 };
    double row_derivative[4] = { 0.0, 0.0, 0.0, 0.0 };
    double weight_val = 0.0;
    double control_weighted = 0.0;
    double inv = 0.0;
    double point = 0.0;
    uint32_t span_u = 0;
    uint32_t span_v = 0;
    uint32_t ju = 0;
    uint32_t jv = 0;
    uint32_t iu = 0;
    uint32_t iv = 0;
    uint32_t idx = 0;
    uint32_t coord = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!knot_u || !knot_v || !control || !out_point || !out_derivative_u || !out_derivative_v)) { return LDG_ERR_FUNC_ARG_NULL; }

    out_point[0] = 0.0;
    out_point[1] = 0.0;
    out_point[2] = 0.0;
    out_derivative_u[0] = 0.0;
    out_derivative_u[1] = 0.0;
    out_derivative_u[2] = 0.0;
    out_derivative_v[0] = 0.0;
    out_derivative_v[1] = 0.0;
    out_derivative_v[2] = 0.0;

    if (LDG_UNLIKELY(degree_u == 0 || degree_u > LDG_MATH_NURBS_MAX_DEGREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(degree_v == 0 || degree_v > LDG_MATH_NURBS_MAX_DEGREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(knot_u_cunt != cunt_u + degree_u + 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(knot_v_cunt != cunt_v + degree_v + 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_nurbs_span_find(degree_u, knot_u, knot_u_cunt, param_u, &span_u);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_nurbs_span_find(degree_v, knot_v, knot_v_cunt, param_v, &span_v);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    nurbs_basis_derivative_fill(degree_u, knot_u, span_u, param_u, basis_u, basis_u_derivative);
    nurbs_basis_derivative_fill(degree_v, knot_v, span_v, param_v, basis_v, basis_v_derivative);

    for (ju = 0; ju <= degree_u; ju++)
    {
        iu = span_u - degree_u + ju;
        row[0] = 0.0;
        row[1] = 0.0;
        row[2] = 0.0;
        row[3] = 0.0;
        row_derivative[0] = 0.0;
        row_derivative[1] = 0.0;
        row_derivative[2] = 0.0;
        row_derivative[3] = 0.0;
        for (jv = 0; jv <= degree_v; jv++)
        {
            iv = span_v - degree_v + jv;
            idx = iu * cunt_v + iv;
            weight_val = weight ? weight[idx] : 1.0;
            if (LDG_UNLIKELY(weight && weight_val <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            for (coord = 0; coord < 3; coord++)
            {
                control_weighted = weight_val * control[idx * 3 + coord];
                row[coord] += basis_v[jv] * control_weighted;
                row_derivative[coord] += basis_v_derivative[jv] * control_weighted;
            }
            row[3] += basis_v[jv] * weight_val;
            row_derivative[3] += basis_v_derivative[jv] * weight_val;
        }
        for (coord = 0; coord < 4; coord++)
        {
            surface[coord] += basis_u[ju] * row[coord];
            surface_u[coord] += basis_u_derivative[ju] * row[coord];
            surface_v[coord] += basis_u[ju] * row_derivative[coord];
        }
    }

    if (LDG_UNLIKELY(surface[3] <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv = 1.0 / surface[3];
    for (coord = 0; coord < 3; coord++)
    {
        point = surface[coord] * inv;
        out_point[coord] = point;
        out_derivative_u[coord] = (surface_u[coord] - surface_u[3] * point) * inv;
        out_derivative_v[coord] = (surface_v[coord] - surface_v[3] * point) * inv;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_nurbs_curve_point_invert(uint32_t degree, const double *knot, uint32_t knot_cunt, const double *control, const double *weight, uint32_t control_cunt, const double *target, double param_init, double tol, uint32_t max_iter, double *out)
{
    double point[3] = { 0.0, 0.0, 0.0 };
    double derivative[3] = { 0.0, 0.0, 0.0 };
    double residual[3] = { 0.0, 0.0, 0.0 };
    double param = 0.0;
    double param_min = 0.0;
    double param_max = 0.0;
    double numerator = 0.0;
    double denominator = 0.0;
    double step = 0.0;
    double prev = 0.0;
    double dist = 0.0;
    uint32_t iter = 0;
    uint32_t coord = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!knot || !control || !target || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    if (LDG_UNLIKELY(degree == 0 || degree > LDG_MATH_NURBS_MAX_DEGREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(knot_cunt != control_cunt + degree + 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(max_iter == 0 || tol <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    param_min = knot[degree];
    param_max = knot[knot_cunt - 1 - degree];
    param = param_init;
    if (param < param_min) { param = param_min; }

    if (param > param_max) { param = param_max; }

    for (iter = 0; iter < max_iter; iter++)
    {
        ret = ldg_math_nurbs_curve_derivative_eval(degree, knot, knot_cunt, control, weight, control_cunt, param, point, derivative);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        dist = 0.0;
        numerator = 0.0;
        denominator = 0.0;
        for (coord = 0; coord < 3; coord++)
        {
            residual[coord] = target[coord] - point[coord];
            dist += residual[coord] * residual[coord];
            numerator += derivative[coord] * residual[coord];
            denominator += derivative[coord] * derivative[coord];
        }

        if (dist <= tol * tol)
        {
            *out = param;
            return LDG_ERR_AOK;
        }

        if (LDG_UNLIKELY(denominator <= 0.0)) { return LDG_ERR_MATH_PIVOT; }

        prev = param;
        step = numerator / denominator;
        param += step;
        if (param < param_min) { param = param_min; }

        if (param > param_max) { param = param_max; }

        step = param - prev;
        if (step * step * denominator <= tol * tol)
        {
            *out = param;
            return LDG_ERR_AOK;
        }
    }

    *out = param;
    return LDG_ERR_MATH_NOT_CONVERGED;
}

uint32_t ldg_math_nurbs_surface_point_invert(uint32_t degree_u, const double *knot_u, uint32_t knot_u_cunt, uint32_t degree_v, const double *knot_v, uint32_t knot_v_cunt, const double *control, const double *weight, uint32_t cunt_u, uint32_t cunt_v, const double *target, double param_u_init, double param_v_init, double tol, uint32_t max_iter, double *out_u, double *out_v)
{
    double point[3] = { 0.0, 0.0, 0.0 };
    double derivative_u[3] = { 0.0, 0.0, 0.0 };
    double derivative_v[3] = { 0.0, 0.0, 0.0 };
    double residual[3] = { 0.0, 0.0, 0.0 };
    double param_u = 0.0;
    double param_v = 0.0;
    double u_min = 0.0;
    double u_max = 0.0;
    double v_min = 0.0;
    double v_max = 0.0;
    double metric_uu = 0.0;
    double metric_uv = 0.0;
    double metric_vv = 0.0;
    double proj_u = 0.0;
    double proj_v = 0.0;
    double det = 0.0;
    double step_u = 0.0;
    double step_v = 0.0;
    double prev_u = 0.0;
    double prev_v = 0.0;
    double move = 0.0;
    double dist = 0.0;
    uint32_t iter = 0;
    uint32_t coord = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!knot_u || !knot_v || !control || !target || !out_u || !out_v)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_u = 0.0;
    *out_v = 0.0;

    if (LDG_UNLIKELY(degree_u == 0 || degree_u > LDG_MATH_NURBS_MAX_DEGREE || degree_v == 0 || degree_v > LDG_MATH_NURBS_MAX_DEGREE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(knot_u_cunt != cunt_u + degree_u + 1 || knot_v_cunt != cunt_v + degree_v + 1)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(max_iter == 0 || tol <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    u_min = knot_u[degree_u];
    u_max = knot_u[knot_u_cunt - 1 - degree_u];
    v_min = knot_v[degree_v];
    v_max = knot_v[knot_v_cunt - 1 - degree_v];
    param_u = param_u_init;
    param_v = param_v_init;
    if (param_u < u_min) { param_u = u_min; }

    if (param_u > u_max) { param_u = u_max; }

    if (param_v < v_min) { param_v = v_min; }

    if (param_v > v_max) { param_v = v_max; }

    for (iter = 0; iter < max_iter; iter++)
    {
        ret = ldg_math_nurbs_surface_derivative_eval(degree_u, knot_u, knot_u_cunt, degree_v, knot_v, knot_v_cunt, control, weight, cunt_u, cunt_v, param_u, param_v, point, derivative_u, derivative_v);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        dist = 0.0;
        metric_uu = 0.0;
        metric_uv = 0.0;
        metric_vv = 0.0;
        proj_u = 0.0;
        proj_v = 0.0;
        for (coord = 0; coord < 3; coord++)
        {
            residual[coord] = target[coord] - point[coord];
            dist += residual[coord] * residual[coord];
            metric_uu += derivative_u[coord] * derivative_u[coord];
            metric_uv += derivative_u[coord] * derivative_v[coord];
            metric_vv += derivative_v[coord] * derivative_v[coord];
            proj_u += derivative_u[coord] * residual[coord];
            proj_v += derivative_v[coord] * residual[coord];
        }

        if (dist <= tol * tol)
        {
            *out_u = param_u;
            *out_v = param_v;
            return LDG_ERR_AOK;
        }

        det = metric_uu * metric_vv - metric_uv * metric_uv;
        if (LDG_UNLIKELY(det <= 0.0)) { return LDG_ERR_MATH_PIVOT; }

        step_u = (metric_vv * proj_u - metric_uv * proj_v) / det;
        step_v = (metric_uu * proj_v - metric_uv * proj_u) / det;
        prev_u = param_u;
        prev_v = param_v;
        param_u += step_u;
        param_v += step_v;
        if (param_u < u_min) { param_u = u_min; }

        if (param_u > u_max) { param_u = u_max; }

        if (param_v < v_min) { param_v = v_min; }

        if (param_v > v_max) { param_v = v_max; }

        step_u = param_u - prev_u;
        step_v = param_v - prev_v;
        move = step_u * step_u * metric_uu + 2.0 * step_u * step_v * metric_uv + step_v * step_v * metric_vv;
        if (move <= tol * tol)
        {
            *out_u = param_u;
            *out_v = param_v;
            return LDG_ERR_AOK;
        }
    }

    *out_u = param_u;
    *out_v = param_v;
    return LDG_ERR_MATH_NOT_CONVERGED;
}
