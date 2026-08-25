#include <float.h>

#define LDG_MATH_ROOT_ZERO 0.0
#define LDG_MATH_ROOT_HALF 0.5
#define LDG_MATH_ROOT_ONE 1.0
#define LDG_MATH_ROOT_TWO 2.0
#define LDG_MATH_ROOT_THREE 3.0

#include <dangling/math/root.h>
#include <dangling/core/err.h>
#include <dangling/math/math.h>
#include <dangling/core/macros.h>

uint32_t ldg_math_root_brent_compute(ldg_math_root_func_cb_t func, void *ctx, double lo, double hi, double tol, uint32_t iter_max, double *out)
{
    double prev = 0.0;
    double cur = 0.0;
    double contra = 0.0;
    double func_prev = 0.0;
    double func_cur = 0.0;
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

    if (LDG_UNLIKELY(!func || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_ROOT_ZERO;

    if (LDG_UNLIKELY(iter_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(tol < LDG_MATH_ROOT_TOL_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(lo >= hi)) { return LDG_ERR_FUNC_ARG_INVALID; }

    prev = lo;
    cur = hi;
    func_prev = func(prev, ctx);
    func_cur = func(cur, ctx);

    if (func_prev == LDG_MATH_ROOT_ZERO)
    {
        *out = prev;
        return LDG_ERR_AOK;
    }

    if (func_cur == LDG_MATH_ROOT_ZERO)
    {
        *out = cur;
        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY((func_prev > LDG_MATH_ROOT_ZERO) == (func_cur > LDG_MATH_ROOT_ZERO))) { return LDG_ERR_MATH_STATE; }

    contra = prev;
    func_contra = func_prev;
    step = cur - prev;
    step_prev = step;

    for (iter = 0; iter < iter_max; iter++)
    {
        if ((func_cur > LDG_MATH_ROOT_ZERO) == (func_contra > LDG_MATH_ROOT_ZERO))
        {
            contra = prev;
            func_contra = func_prev;
            step = cur - prev;
            step_prev = step;
        }

        if (ldg_math_abs_get(func_contra) < ldg_math_abs_get(func_cur))
        {
            prev = cur;
            cur = contra;
            contra = prev;
            func_prev = func_cur;
            func_cur = func_contra;
            func_contra = func_prev;
        }

        tol_act = (LDG_MATH_ROOT_TWO * DBL_EPSILON * ldg_math_abs_get(cur)) + (LDG_MATH_ROOT_HALF * tol);
        mid = LDG_MATH_ROOT_HALF * (contra - cur);

        if (ldg_math_abs_get(mid) <= tol_act || func_cur == LDG_MATH_ROOT_ZERO)
        {
            *out = cur;
            return LDG_ERR_AOK;
        }

        if (ldg_math_abs_get(step_prev) >= tol_act && ldg_math_abs_get(func_prev) > ldg_math_abs_get(func_cur))
        {
            scale = func_cur / func_prev;

            if (prev == contra)
            {
                num = LDG_MATH_ROOT_TWO * mid * scale;
                den = LDG_MATH_ROOT_ONE - scale;
            }
            else
            {
                den = func_prev / func_contra;
                ratio = func_cur / func_contra;
                num = scale * ((LDG_MATH_ROOT_TWO * mid * den * (den - ratio)) - ((cur - prev) * (ratio - LDG_MATH_ROOT_ONE)));
                den = (den - LDG_MATH_ROOT_ONE) * (ratio - LDG_MATH_ROOT_ONE) * (scale - LDG_MATH_ROOT_ONE);
            }

            if (num > LDG_MATH_ROOT_ZERO) { den = -den; }

            num = ldg_math_abs_get(num);

            bound_interp = (LDG_MATH_ROOT_THREE * mid * den) - ldg_math_abs_get(tol_act * den);
            bound_step = ldg_math_abs_get(step_prev * den);

            if ((LDG_MATH_ROOT_TWO * num) < ((bound_interp < bound_step) ? bound_interp : bound_step))
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

        prev = cur;
        func_prev = func_cur;

        if (ldg_math_abs_get(step) > tol_act) { cur += step; }
        else if (mid >= LDG_MATH_ROOT_ZERO) { cur += ldg_math_abs_get(tol_act); }
        else { cur -= ldg_math_abs_get(tol_act); }

        func_cur = func(cur, ctx);
    }

    *out = cur;

    return LDG_ERR_MATH_NOT_CONVERGED;
}
