#ifndef LDG_MATH_LINALG_H
#define LDG_MATH_LINALG_H

#include <math.h>

static inline void ldg_vec3_zero(double v[3])
{
    v[0] = 0.0;
    v[1] = 0.0;
    v[2] = 0.0;
}

static inline void ldg_vec3_copy(double dst[3], const double src[3])
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

static inline void ldg_vec3_add(double dst[3], const double a[3], const double b[3])
{
    dst[0] = a[0] + b[0];
    dst[1] = a[1] + b[1];
    dst[2] = a[2] + b[2];
}

static inline void ldg_vec3_sub(double dst[3], const double a[3], const double b[3])
{
    dst[0] = a[0] - b[0];
    dst[1] = a[1] - b[1];
    dst[2] = a[2] - b[2];
}

static inline void ldg_vec3_scale(double v[3], double s)
{
    v[0] *= s;
    v[1] *= s;
    v[2] *= s;
}

static inline void ldg_vec3_addscaled(double dst[3], const double v[3], double s)
{
    dst[0] += v[0] * s;
    dst[1] += v[1] * s;
    dst[2] += v[2] * s;
}

static inline double ldg_vec3_dot(const double a[3], const double b[3])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static inline double ldg_vec3_length(const double v[3])
{
    return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static inline void ldg_vec3_cross(double dst[3], const double a[3], const double b[3])
{
    dst[0] = a[1] * b[2] - a[2] * b[1];
    dst[1] = a[2] * b[0] - a[0] * b[2];
    dst[2] = a[0] * b[1] - a[1] * b[0];
}

static inline void ldg_mat3_zero(double M[9])
{
    int i = 0;
    for (i = 0; i < 9; i++) { M[i] = 0.0; }
}

static inline void ldg_mat3_identity(double M[9])
{
    ldg_mat3_zero(M);
    M[0] = 1.0;
    M[4] = 1.0;
    M[8] = 1.0;
}

static inline void ldg_mat3_copy(double dst[9], const double src[9])
{
    int i = 0;
    for (i = 0; i < 9; i++) { dst[i] = src[i]; }
}

static inline void ldg_mat3_transpose(double dst[9], const double src[9])
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

static inline void ldg_mat3_add(double dst[9], const double A[9], const double B[9])
{
    int i = 0;
    for (i = 0; i < 9; i++) { dst[i] = A[i] + B[i]; }
}

static inline void ldg_mat3_sub(double dst[9], const double A[9], const double B[9])
{
    int i = 0;
    for (i = 0; i < 9; i++) { dst[i] = A[i] - B[i]; }
}

static inline void ldg_mat3_scale(double M[9], double s)
{
    int i = 0;
    for (i = 0; i < 9; i++) { M[i] *= s; }
}

static inline void ldg_mat3_mul(double dst[9], const double A[9], const double B[9])
{
    double tmp[9];
    tmp[0] = A[0] * B[0] + A[1] * B[3] + A[2] * B[6];
    tmp[1] = A[0] * B[1] + A[1] * B[4] + A[2] * B[7];
    tmp[2] = A[0] * B[2] + A[1] * B[5] + A[2] * B[8];
    tmp[3] = A[3] * B[0] + A[4] * B[3] + A[5] * B[6];
    tmp[4] = A[3] * B[1] + A[4] * B[4] + A[5] * B[7];
    tmp[5] = A[3] * B[2] + A[4] * B[5] + A[5] * B[8];
    tmp[6] = A[6] * B[0] + A[7] * B[3] + A[8] * B[6];
    tmp[7] = A[6] * B[1] + A[7] * B[4] + A[8] * B[7];
    tmp[8] = A[6] * B[2] + A[7] * B[5] + A[8] * B[8];
    ldg_mat3_copy(dst, tmp);
}

static inline void ldg_mat3_mulvec(double dst[3], const double M[9], const double v[3])
{
    dst[0] = M[0] * v[0] + M[1] * v[1] + M[2] * v[2];
    dst[1] = M[3] * v[0] + M[4] * v[1] + M[5] * v[2];
    dst[2] = M[6] * v[0] + M[7] * v[1] + M[8] * v[2];
}

static inline double ldg_mat3_det(const double M[9])
{
    return M[0] * (M[4] * M[8] - M[5] * M[7]) - M[1] * (M[3] * M[8] - M[5] * M[6]) + M[2] * (M[3] * M[7] - M[4] * M[6]);
}

static inline double ldg_mat3_trace(const double M[9])
{
    return M[0] + M[4] + M[8];
}

static inline int ldg_mat3_inv(double dst[9], const double M[9])
{
    double det = 0.0;
    double inv_det = 0.0;

    det = ldg_mat3_det(M);
    if (fabs(det) < 1e-14) { return -1; }

    inv_det = 1.0 / det;
    dst[0] = (M[4] * M[8] - M[5] * M[7]) * inv_det;
    dst[1] = (M[2] * M[7] - M[1] * M[8]) * inv_det;
    dst[2] = (M[1] * M[5] - M[2] * M[4]) * inv_det;
    dst[3] = (M[5] * M[6] - M[3] * M[8]) * inv_det;
    dst[4] = (M[0] * M[8] - M[2] * M[6]) * inv_det;
    dst[5] = (M[2] * M[3] - M[0] * M[5]) * inv_det;
    dst[6] = (M[3] * M[7] - M[4] * M[6]) * inv_det;
    dst[7] = (M[1] * M[6] - M[0] * M[7]) * inv_det;
    dst[8] = (M[0] * M[4] - M[1] * M[3]) * inv_det;
    return 0;
}

static inline void ldg_mat3_from_cols(double M[9], const double c0[3], const double c1[3], const double c2[3])
{
    M[0] = c0[0];
    M[3] = c0[1];
    M[6] = c0[2];
    M[1] = c1[0];
    M[4] = c1[1];
    M[7] = c1[2];
    M[2] = c2[0];
    M[5] = c2[1];
    M[8] = c2[2];
}

static inline void ldg_mat3_col(double dst[3], const double M[9], int col)
{
    dst[0] = M[col];
    dst[1] = M[3 + col];
    dst[2] = M[6 + col];
}

static inline void ldg_mat3_polar(double R[9], double S[9], const double F[9])
{
    int iter = 0;
    int i = 0;
    double Rt[9];
    double Rt_inv[9];
    double R_next[9];
    double diff = 0.0;
    double d = 0.0;

    ldg_mat3_copy(R, F);

    for (iter = 0; iter < 10; iter++)
    {
        ldg_mat3_transpose(Rt, R);

        if (ldg_mat3_inv(Rt_inv, Rt) != 0)
        {
            ldg_mat3_identity(R);
            ldg_mat3_identity(S);
            return;
        }

        for (i = 0; i < 9; i++) { R_next[i] = 0.5 * (R[i] + Rt_inv[i]); }

        diff = 0.0;
        for (i = 0; i < 9; i++)
        {
            d = R_next[i] - R[i];
            diff += d * d;
        }

        ldg_mat3_copy(R, R_next);

        if (diff < 1e-12) { break; }
    }

    if (ldg_mat3_det(R) < 0.0) { for (i = 0; i < 9; i++) { R[i] = -R[i]; } }

    ldg_mat3_transpose(Rt, R);
    ldg_mat3_mul(S, Rt, F);
}

#endif
