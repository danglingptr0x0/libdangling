#ifndef LDG_MATH_LINALG_H
#define LDG_MATH_LINALG_H

#include <stdint.h>

static inline void ldg_math_vec3_zero(double vec[3])
{
    vec[0] = 0.0;
    vec[1] = 0.0;
    vec[2] = 0.0;
}

static inline void ldg_math_vec3_cpy(double dst[3], const double src[3])
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

static inline void ldg_math_vec3_add(double dst[3], const double lhs[3], const double rhs[3])
{
    dst[0] = lhs[0] + rhs[0];
    dst[1] = lhs[1] + rhs[1];
    dst[2] = lhs[2] + rhs[2];
}

static inline void ldg_math_vec3_sub(double dst[3], const double lhs[3], const double rhs[3])
{
    dst[0] = lhs[0] - rhs[0];
    dst[1] = lhs[1] - rhs[1];
    dst[2] = lhs[2] - rhs[2];
}

static inline void ldg_math_vec3_scale(double vec[3], double scalar)
{
    vec[0] *= scalar;
    vec[1] *= scalar;
    vec[2] *= scalar;
}

static inline void ldg_math_vec3_scaled_add(double dst[3], const double vec[3], double scalar)
{
    dst[0] += vec[0] * scalar;
    dst[1] += vec[1] * scalar;
    dst[2] += vec[2] * scalar;
}

static inline double ldg_math_vec3_dot(const double lhs[3], const double rhs[3])
{
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}

static inline double ldg_math_vec3_len(const double vec[3])
{
    return __builtin_sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

static inline void ldg_math_vec3_cross(double dst[3], const double lhs[3], const double rhs[3])
{
    dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
    dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
    dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
}

static inline void ldg_math_mat3_zero(double mat[9])
{
    uint32_t idx = 0;
    for (idx = 0; idx < 9; idx++) { mat[idx] = 0.0; }
}

static inline void ldg_math_mat3_identity(double mat[9])
{
    ldg_math_mat3_zero(mat);
    mat[0] = 1.0;
    mat[4] = 1.0;
    mat[8] = 1.0;
}

static inline void ldg_math_mat3_cpy(double dst[9], const double src[9])
{
    uint32_t idx = 0;
    for (idx = 0; idx < 9; idx++) { dst[idx] = src[idx]; }
}

static inline void ldg_math_mat3_transpose(double dst[9], const double src[9])
{
    dst[0] = src[0];
    dst[1] = src[3];
    dst[2] = src[6];
    dst[3] = src[1];
    dst[4] = src[4];
    dst[5] = src[7];
    dst[6] = src[2];
    dst[7] = src[5];
    dst[8] = src[8];
}

static inline void ldg_math_mat3_add(double dst[9], const double lhs[9], const double rhs[9])
{
    uint32_t idx = 0;
    for (idx = 0; idx < 9; idx++) { dst[idx] = lhs[idx] + rhs[idx]; }
}

static inline void ldg_math_mat3_sub(double dst[9], const double lhs[9], const double rhs[9])
{
    uint32_t idx = 0;
    for (idx = 0; idx < 9; idx++) { dst[idx] = lhs[idx] - rhs[idx]; }
}

static inline void ldg_math_mat3_scale(double mat[9], double scalar)
{
    uint32_t idx = 0;
    for (idx = 0; idx < 9; idx++) { mat[idx] *= scalar; }
}

static inline void ldg_math_mat3_mul(double dst[9], const double lhs[9], const double rhs[9])
{
    double tmp[9];
    tmp[0] = lhs[0] * rhs[0] + lhs[1] * rhs[3] + lhs[2] * rhs[6];
    tmp[1] = lhs[0] * rhs[1] + lhs[1] * rhs[4] + lhs[2] * rhs[7];
    tmp[2] = lhs[0] * rhs[2] + lhs[1] * rhs[5] + lhs[2] * rhs[8];
    tmp[3] = lhs[3] * rhs[0] + lhs[4] * rhs[3] + lhs[5] * rhs[6];
    tmp[4] = lhs[3] * rhs[1] + lhs[4] * rhs[4] + lhs[5] * rhs[7];
    tmp[5] = lhs[3] * rhs[2] + lhs[4] * rhs[5] + lhs[5] * rhs[8];
    tmp[6] = lhs[6] * rhs[0] + lhs[7] * rhs[3] + lhs[8] * rhs[6];
    tmp[7] = lhs[6] * rhs[1] + lhs[7] * rhs[4] + lhs[8] * rhs[7];
    tmp[8] = lhs[6] * rhs[2] + lhs[7] * rhs[5] + lhs[8] * rhs[8];
    ldg_math_mat3_cpy(dst, tmp);
}

static inline void ldg_math_mat3_vec_mul(double dst[3], const double mat[9], const double vec[3])
{
    dst[0] = mat[0] * vec[0] + mat[1] * vec[1] + mat[2] * vec[2];
    dst[1] = mat[3] * vec[0] + mat[4] * vec[1] + mat[5] * vec[2];
    dst[2] = mat[6] * vec[0] + mat[7] * vec[1] + mat[8] * vec[2];
}

static inline double ldg_math_mat3_det(const double mat[9])
{
    return mat[0] * (mat[4] * mat[8] - mat[5] * mat[7]) - mat[1] * (mat[3] * mat[8] - mat[5] * mat[6]) + mat[2] * (mat[3] * mat[7] - mat[4] * mat[6]);
}

static inline double ldg_math_mat3_trace(const double mat[9])
{
    return mat[0] + mat[4] + mat[8];
}

#define LDG_MATH_MAT3_DET_EPSILON 1e-14
#define LDG_MATH_MAT3_POLAR_EPSILON 1e-12
#define LDG_MATH_MAT3_POLAR_MAX_ITER 10

static inline uint32_t ldg_math_mat3_inv(double dst[9], const double mat[9])
{
    double det = 0.0;
    double inv_det = 0.0;

    det = ldg_math_mat3_det(mat);
    if (__builtin_fabs(det) < LDG_MATH_MAT3_DET_EPSILON) { return 1; }

    inv_det = 1.0 / det;
    dst[0] = (mat[4] * mat[8] - mat[5] * mat[7]) * inv_det;
    dst[1] = (mat[2] * mat[7] - mat[1] * mat[8]) * inv_det;
    dst[2] = (mat[1] * mat[5] - mat[2] * mat[4]) * inv_det;
    dst[3] = (mat[5] * mat[6] - mat[3] * mat[8]) * inv_det;
    dst[4] = (mat[0] * mat[8] - mat[2] * mat[6]) * inv_det;
    dst[5] = (mat[2] * mat[3] - mat[0] * mat[5]) * inv_det;
    dst[6] = (mat[3] * mat[7] - mat[4] * mat[6]) * inv_det;
    dst[7] = (mat[1] * mat[6] - mat[0] * mat[7]) * inv_det;
    dst[8] = (mat[0] * mat[4] - mat[1] * mat[3]) * inv_det;
    return 0;
}

static inline void ldg_math_mat3_cols_from(double mat[9], const double col0[3], const double col1[3], const double col2[3])
{
    mat[0] = col0[0];
    mat[3] = col0[1];
    mat[6] = col0[2];
    mat[1] = col1[0];
    mat[4] = col1[1];
    mat[7] = col1[2];
    mat[2] = col2[0];
    mat[5] = col2[1];
    mat[8] = col2[2];
}

static inline void ldg_math_mat3_col(double dst[3], const double mat[9], uint32_t col)
{
    dst[0] = mat[col];
    dst[1] = mat[3 + col];
    dst[2] = mat[6 + col];
}

static inline void ldg_math_mat3_polar(double rot[9], double stretch[9], const double mat[9])
{
    uint32_t iter = 0;
    uint32_t idx = 0;
    double rot_t[9];
    double rot_t_inv[9];
    double rot_next[9];
    double diff = 0.0;
    double delta = 0.0;

    ldg_math_mat3_cpy(rot, mat);

    for (iter = 0; iter < LDG_MATH_MAT3_POLAR_MAX_ITER; iter++)
    {
        ldg_math_mat3_transpose(rot_t, rot);

        if (ldg_math_mat3_inv(rot_t_inv, rot_t) != 0)
        {
            ldg_math_mat3_identity(rot);
            ldg_math_mat3_identity(stretch);
            return;
        }

        for (idx = 0; idx < 9; idx++) { rot_next[idx] = 0.5 * (rot[idx] + rot_t_inv[idx]); }

        diff = 0.0;
        for (idx = 0; idx < 9; idx++)
        {
            delta = rot_next[idx] - rot[idx];
            diff += delta * delta;
        }

        ldg_math_mat3_cpy(rot, rot_next);

        if (diff < LDG_MATH_MAT3_POLAR_EPSILON) { break; }
    }

    if (ldg_math_mat3_det(rot) < 0.0) { for (idx = 0; idx < 9; idx++) { rot[idx] = -rot[idx]; } }

    ldg_math_mat3_transpose(rot_t, rot);
    ldg_math_mat3_mul(stretch, rot_t, mat);
}

#endif
