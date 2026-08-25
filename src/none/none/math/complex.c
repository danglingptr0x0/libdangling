
#include <dangling/math/complex.h>
#include <dangling/math/math.h>
#include <dangling/core/macros.h>

uint32_t ldg_math_complex_add(const ldg_math_complex_t *lhs, const ldg_math_complex_t *rhs, ldg_math_complex_t *out)
{
    if (LDG_UNLIKELY(!lhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->re = lhs->re + rhs->re;
    out->im = lhs->im + rhs->im;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_complex_sub(const ldg_math_complex_t *lhs, const ldg_math_complex_t *rhs, ldg_math_complex_t *out)
{
    if (LDG_UNLIKELY(!lhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->re = lhs->re - rhs->re;
    out->im = lhs->im - rhs->im;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_complex_mul(const ldg_math_complex_t *lhs, const ldg_math_complex_t *rhs, ldg_math_complex_t *out)
{
    double re_val = 0.0;
    double im_val = 0.0;

    if (LDG_UNLIKELY(!lhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    re_val = lhs->re * rhs->re - lhs->im * rhs->im;
    im_val = lhs->re * rhs->im + lhs->im * rhs->re;

    out->re = re_val;
    out->im = im_val;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_complex_div(const ldg_math_complex_t *lhs, const ldg_math_complex_t *rhs, ldg_math_complex_t *out)
{
    double denom = 0.0;
    double re_val = 0.0;
    double im_val = 0.0;

    if (LDG_UNLIKELY(!lhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    denom = rhs->re * rhs->re + rhs->im * rhs->im;
    if (LDG_UNLIKELY(denom <= LDG_MATH_EPSILON)) { return LDG_ERR_FUNC_ARG_INVALID; }

    re_val = (lhs->re * rhs->re + lhs->im * rhs->im) / denom;
    im_val = (lhs->im * rhs->re - lhs->re * rhs->im) / denom;

    out->re = re_val;
    out->im = im_val;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_complex_scale(const ldg_math_complex_t *val, double factor, ldg_math_complex_t *out)
{
    if (LDG_UNLIKELY(!val)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->re = val->re * factor;
    out->im = val->im * factor;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_complex_conjugate(const ldg_math_complex_t *val, ldg_math_complex_t *out)
{
    if (LDG_UNLIKELY(!val)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->re = val->re;
    out->im = -val->im;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_complex_abs_get(const ldg_math_complex_t *val, double *out)
{
    if (LDG_UNLIKELY(!val)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ldg_math_sqrt_get(val->re * val->re + val->im * val->im);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_complex_arg_get(const ldg_math_complex_t *val, double *out)
{
    if (LDG_UNLIKELY(!val)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ldg_math_arctan2_get(val->im, val->re);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_complex_exp_compute(const ldg_math_complex_t *val, ldg_math_complex_t *out)
{
    double mag = 0.0;
    double re_val = 0.0;
    double im_val = 0.0;

    if (LDG_UNLIKELY(!val)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    mag = ldg_math_exp_get(val->re);
    re_val = mag * ldg_math_cos_get(val->im);
    im_val = mag * ldg_math_sin_get(val->im);

    out->re = re_val;
    out->im = im_val;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_complex_from_polar_compute(double mag, double arg, ldg_math_complex_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->re = mag * ldg_math_cos_get(arg);
    out->im = mag * ldg_math_sin_get(arg);

    return LDG_ERR_AOK;
}
