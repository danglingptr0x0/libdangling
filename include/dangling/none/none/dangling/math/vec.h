#ifndef LDG_MATH_VEC_H
#define LDG_MATH_VEC_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/math.h>

static inline uint32_t ldg_math_vec_zero(double *vec, uint64_t len)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!vec)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < len; idx++) { vec[idx] = 0.0; }

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_math_vec_set(double *vec, double val, uint64_t len)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!vec)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < len; idx++) { vec[idx] = val; }

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_math_vec_cpy(double *dst, const double *src, uint64_t len)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < len; idx++) { dst[idx] = src[idx]; }

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_math_vec_scale(double *vec, double scalar, uint64_t len)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!vec)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < len; idx++) { vec[idx] *= scalar; }

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_math_vec_axpy(double *dst, const double *src, double alpha, uint64_t len)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < len; idx++) { dst[idx] += alpha * src[idx]; }

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_math_vec_sub(double *out, const double *lhs, const double *rhs, uint64_t len)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!out || !lhs || !rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < len; idx++) { out[idx] = lhs[idx] - rhs[idx]; }

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_math_vec_mul_elem(double *out, const double *lhs, const double *rhs, uint64_t len)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!out || !lhs || !rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < len; idx++) { out[idx] = lhs[idx] * rhs[idx]; }

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_math_vec_dot_get(const double *lhs, const double *rhs, uint64_t len, double *out)
{
    uint64_t idx = 0;
    double acc = 0.0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!lhs || !rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < len; idx++) { acc += lhs[idx] * rhs[idx]; }

    *out = acc;

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_math_vec_norm1_get(const double *vec, uint64_t len, double *out)
{
    uint64_t idx = 0;
    double acc = 0.0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!vec)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < len; idx++) { acc += __builtin_fabs(vec[idx]); }

    *out = acc;

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_math_vec_norm2_get(const double *vec, uint64_t len, double *out)
{
    uint64_t idx = 0;
    double scale = 0.0;
    double sum_sq = 0.0;
    double abs_val = 0.0;
    double ratio = 0.0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!vec)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    sum_sq = 1.0;

    for (idx = 0; idx < len; idx++)
    {
        abs_val = __builtin_fabs(vec[idx]);
        if (abs_val > 0.0)
        {
            if (scale < abs_val)
            {
                ratio = scale / abs_val;
                sum_sq = 1.0 + sum_sq * ratio * ratio;
                scale = abs_val;
            }
            else
            {
                ratio = abs_val / scale;
                sum_sq = sum_sq + ratio * ratio;
            }
        }
    }

    *out = scale * __builtin_sqrt(sum_sq);

    return LDG_ERR_AOK;
}

static inline uint32_t ldg_math_vec_norm_inf_get(const double *vec, uint64_t len, double *out)
{
    uint64_t idx = 0;
    double max_abs = 0.0;
    double abs_val = 0.0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!vec)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < len; idx++)
    {
        abs_val = __builtin_fabs(vec[idx]);
        if (abs_val > max_abs) { max_abs = abs_val; }
    }

    *out = max_abs;

    return LDG_ERR_AOK;
}

#endif
