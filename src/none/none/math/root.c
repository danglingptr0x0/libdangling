#include <float.h>

#define LDG_MATH_ROOT_ORIG 0.0
#define LDG_MATH_ROOT_BISECT_RATIO 0.5
#define LDG_MATH_ROOT_RATIO_IDENT 1.0
#define LDG_MATH_ROOT_EPS_SCALE 2.0
#define LDG_MATH_ROOT_BRACKET_SPAN 2.0
#define LDG_MATH_ROOT_INTERP_BOUND 3.0

#include <dangling/math/root.h>
#include <dangling/core/err.h>
#include <dangling/math/math.h>
#include <dangling/core/macros.h>

uint32_t ldg_math_root_brent_compute(ldg_math_root_func_cb_t func, void *ctx, double lo, double hi, double tol, uint32_t iter_max, double *out)
{
    double prev = 0.0;
    double curr = 0.0;
    double contra = 0.0;
    double func_prev = 0.0;
    double func_curr = 0.0;
    double func_contra = 0.0;
    double step = 0.0;
    double step_prev = 0.0;
    double mid = 0.0;
    double tol_act = 0.0;
    double num = 0.0;
    double den = 0.0;
    double ratio = 0.0;
    double scale = 0.0;
    double bound_interp = 0.0;
    double bound_step = 0.0;
    uint32_t iter = 0;
    uint8_t contra_from_prev = 0;

    if (LDG_UNLIKELY(!func || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_ROOT_ORIG;

    if (LDG_UNLIKELY(iter_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(tol < LDG_MATH_ROOT_TOL_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(lo >= hi)) { return LDG_ERR_FUNC_ARG_INVALID; }

    prev = lo;
    curr = hi;
    func_prev = func(prev, ctx);
    func_curr = func(curr, ctx);

    if (func_prev == LDG_MATH_ROOT_ORIG)
    {
        *out = prev;
        return LDG_ERR_AOK;
    }

    if (func_curr == LDG_MATH_ROOT_ORIG)
    {
        *out = curr;
        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY((func_prev > LDG_MATH_ROOT_ORIG) == (func_curr > LDG_MATH_ROOT_ORIG))) { return LDG_ERR_MATH_STATE; }

    contra = prev;
    func_contra = func_prev;
    contra_from_prev = LDG_TRUTH_TRUE;
    step = curr - prev;
    step_prev = step;

    for (iter = 0; iter < iter_max; iter++)
    {
        if ((func_curr > LDG_MATH_ROOT_ORIG) == (func_contra > LDG_MATH_ROOT_ORIG))
        {
            contra = prev;
            func_contra = func_prev;
            contra_from_prev = LDG_TRUTH_TRUE;
            step = curr - prev;
            step_prev = step;
        }

        if (ldg_math_abs_get(func_contra) < ldg_math_abs_get(func_curr))
        {
            prev = curr;
            curr = contra;
            contra = prev;
            func_prev = func_curr;
            func_curr = func_contra;
            func_contra = func_prev;
            contra_from_prev = LDG_TRUTH_TRUE;
        }

        tol_act = (LDG_MATH_ROOT_EPS_SCALE * DBL_EPSILON * ldg_math_abs_get(curr)) + (LDG_MATH_ROOT_BISECT_RATIO * tol);
        mid = LDG_MATH_ROOT_BISECT_RATIO * (contra - curr);

        if (ldg_math_abs_get(mid) <= tol_act || func_curr == LDG_MATH_ROOT_ORIG)
        {
            *out = curr;
            return LDG_ERR_AOK;
        }

        if (ldg_math_abs_get(step_prev) >= tol_act && ldg_math_abs_get(func_prev) > ldg_math_abs_get(func_curr))
        {
            scale = func_curr / func_prev;

            if (contra_from_prev == LDG_TRUTH_TRUE)
            {
                num = LDG_MATH_ROOT_BRACKET_SPAN * mid * scale;
                den = LDG_MATH_ROOT_RATIO_IDENT - scale;
            }
            else
            {
                den = func_prev / func_contra;
                ratio = func_curr / func_contra;
                num = scale * ((LDG_MATH_ROOT_BRACKET_SPAN * mid * den * (den - ratio)) - ((curr - prev) * (ratio - LDG_MATH_ROOT_RATIO_IDENT)));
                den = (den - LDG_MATH_ROOT_RATIO_IDENT) * (ratio - LDG_MATH_ROOT_RATIO_IDENT) * (scale - LDG_MATH_ROOT_RATIO_IDENT);
            }

            if (num > LDG_MATH_ROOT_ORIG) { den = -den; }

            num = ldg_math_abs_get(num);

            bound_interp = (LDG_MATH_ROOT_INTERP_BOUND * mid * den) - ldg_math_abs_get(tol_act * den);
            bound_step = ldg_math_abs_get(step_prev * den);

            if ((LDG_MATH_ROOT_BRACKET_SPAN * num) < ((bound_interp < bound_step) ? bound_interp : bound_step))
            {
                step_prev = step;
                step = num / den;
            }
            else
            {
                step = mid;
                step_prev = step;
            }
        }
        else
        {
            step = mid;
            step_prev = step;
        }

        prev = curr;
        func_prev = func_curr;
        contra_from_prev = LDG_TRUTH_FALSE;

        if (ldg_math_abs_get(step) > tol_act) { curr += step; }
        else if (mid >= LDG_MATH_ROOT_ORIG) { curr += ldg_math_abs_get(tol_act); }
        else { curr -= ldg_math_abs_get(tol_act); }

        func_curr = func(curr, ctx);
    }

    *out = curr;

    return LDG_ERR_MATH_NOT_CONVERGED;
}
