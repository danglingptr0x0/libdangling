
#include <dangling/math/quaternion.h>
#include <dangling/math/math.h>
#include <dangling/core/macros.h>

#define LDG_MATH_QUATERNION_EPS 1e-12

uint32_t ldg_math_quaternion_ident_compute(double *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = 0.0;
    out[1] = 0.0;
    out[2] = 0.0;
    out[3] = 1.0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_from_axis_angle_compute(const double *axis, double angle, double *out)
{
    double half = 0.0;
    double sine = 0.0;

    if (LDG_UNLIKELY(!axis)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    half = angle * 0.5;
    sine = ldg_math_sin_get(half);

    out[0] = axis[0] * sine;
    out[1] = axis[1] * sine;
    out[2] = axis[2] * sine;
    out[3] = ldg_math_cos_get(half);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_to_axis_angle_compute(const double *quaternion, double *out_axis, double *out_angle)
{
    double vec_len = 0.0;
    double inv = 0.0;

    if (LDG_UNLIKELY(!quaternion)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_axis)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out_angle)) { return LDG_ERR_FUNC_ARG_NULL; }

    vec_len = ldg_math_sqrt_get(quaternion[0] * quaternion[0] + quaternion[1] * quaternion[1] + quaternion[2] * quaternion[2]);

    if (vec_len <= LDG_MATH_QUATERNION_EPS)
    {
        out_axis[0] = 1.0;
        out_axis[1] = 0.0;
        out_axis[2] = 0.0;
        *out_angle = 0.0;
        return LDG_ERR_AOK;
    }

    inv = 1.0 / vec_len;
    out_axis[0] = quaternion[0] * inv;
    out_axis[1] = quaternion[1] * inv;
    out_axis[2] = quaternion[2] * inv;
    *out_angle = 2.0 * ldg_math_arctan2_get(vec_len, quaternion[3]);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_mul_compute(const double *lhs, const double *rhs, double *out)
{
    double tmp[LDG_MATH_QUATERNION_DIM] = { 0 };

    if (LDG_UNLIKELY(!lhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    tmp[3] = lhs[3] * rhs[3] - lhs[0] * rhs[0] - lhs[1] * rhs[1] - lhs[2] * rhs[2];
    tmp[0] = lhs[3] * rhs[0] + lhs[0] * rhs[3] + lhs[1] * rhs[2] - lhs[2] * rhs[1];
    tmp[1] = lhs[3] * rhs[1] - lhs[0] * rhs[2] + lhs[1] * rhs[3] + lhs[2] * rhs[0];
    tmp[2] = lhs[3] * rhs[2] + lhs[0] * rhs[1] - lhs[1] * rhs[0] + lhs[2] * rhs[3];

    out[0] = tmp[0];
    out[1] = tmp[1];
    out[2] = tmp[2];
    out[3] = tmp[3];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_conjugate(const double *quaternion, double *out)
{
    if (LDG_UNLIKELY(!quaternion)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out[0] = -quaternion[0];
    out[1] = -quaternion[1];
    out[2] = -quaternion[2];
    out[3] = quaternion[3];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_invert(const double *quaternion, double *out)
{
    double norm_sq = 0.0;
    double inv = 0.0;

    if (LDG_UNLIKELY(!quaternion)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    norm_sq = quaternion[0] * quaternion[0] + quaternion[1] * quaternion[1] + quaternion[2] * quaternion[2] + quaternion[3] * quaternion[3];
    if (LDG_UNLIKELY(norm_sq <= LDG_MATH_QUATERNION_EPS)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv = 1.0 / norm_sq;
    out[0] = -quaternion[0] * inv;
    out[1] = -quaternion[1] * inv;
    out[2] = -quaternion[2] * inv;
    out[3] = quaternion[3] * inv;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_normalize(const double *quaternion, double *out)
{
    double norm = 0.0;
    double inv = 0.0;

    if (LDG_UNLIKELY(!quaternion)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    norm = ldg_math_sqrt_get(quaternion[0] * quaternion[0] + quaternion[1] * quaternion[1] + quaternion[2] * quaternion[2] + quaternion[3] * quaternion[3]);
    if (LDG_UNLIKELY(norm <= LDG_MATH_QUATERNION_EPS)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv = 1.0 / norm;
    out[0] = quaternion[0] * inv;
    out[1] = quaternion[1] * inv;
    out[2] = quaternion[2] * inv;
    out[3] = quaternion[3] * inv;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_dot_get(const double *lhs, const double *rhs, double *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!lhs || !rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2] + lhs[3] * rhs[3];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_norm_get(const double *quaternion, double *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!quaternion)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ldg_math_sqrt_get(quaternion[0] * quaternion[0] + quaternion[1] * quaternion[1] + quaternion[2] * quaternion[2] + quaternion[3] * quaternion[3]);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_vec3_rot(const double *quaternion, const double *vec, double *out)
{
    double tcross[LDG_MATH_QUATERNION_VEC_DIM] = { 0 };
    double res[LDG_MATH_QUATERNION_VEC_DIM] = { 0 };

    if (LDG_UNLIKELY(!quaternion)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!vec)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    tcross[0] = 2.0 * (quaternion[1] * vec[2] - quaternion[2] * vec[1]);
    tcross[1] = 2.0 * (quaternion[2] * vec[0] - quaternion[0] * vec[2]);
    tcross[2] = 2.0 * (quaternion[0] * vec[1] - quaternion[1] * vec[0]);

    res[0] = vec[0] + quaternion[3] * tcross[0] + (quaternion[1] * tcross[2] - quaternion[2] * tcross[1]);
    res[1] = vec[1] + quaternion[3] * tcross[1] + (quaternion[2] * tcross[0] - quaternion[0] * tcross[2]);
    res[2] = vec[2] + quaternion[3] * tcross[2] + (quaternion[0] * tcross[1] - quaternion[1] * tcross[0]);

    out[0] = res[0];
    out[1] = res[1];
    out[2] = res[2];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_to_mat3_compute(const double *quaternion, double *out)
{
    double norm_sq = 0.0;
    double scale = 0.0;
    double xs = 0.0;
    double ys = 0.0;
    double zs = 0.0;
    double wx = 0.0;
    double wy = 0.0;
    double wz = 0.0;
    double xx = 0.0;
    double xy = 0.0;
    double xz = 0.0;
    double yy = 0.0;
    double yz = 0.0;
    double zz = 0.0;

    if (LDG_UNLIKELY(!quaternion)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    norm_sq = quaternion[0] * quaternion[0] + quaternion[1] * quaternion[1] + quaternion[2] * quaternion[2] + quaternion[3] * quaternion[3];
    if (norm_sq > LDG_MATH_QUATERNION_EPS) { scale = 2.0 / norm_sq; }

    xs = quaternion[0] * scale;
    ys = quaternion[1] * scale;
    zs = quaternion[2] * scale;
    wx = quaternion[3] * xs;
    wy = quaternion[3] * ys;
    wz = quaternion[3] * zs;
    xx = quaternion[0] * xs;
    xy = quaternion[0] * ys;
    xz = quaternion[0] * zs;
    yy = quaternion[1] * ys;
    yz = quaternion[1] * zs;
    zz = quaternion[2] * zs;

    out[0] = 1.0 - (yy + zz);
    out[1] = xy - wz;
    out[2] = xz + wy;
    out[3] = xy + wz;
    out[4] = 1.0 - (xx + zz);
    out[5] = yz - wx;
    out[6] = xz - wy;
    out[7] = yz + wx;
    out[8] = 1.0 - (xx + yy);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_from_mat3_compute(const double *mat, double *out)
{
    double trace = 0.0;
    double scale = 0.0;

    if (LDG_UNLIKELY(!mat)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    trace = mat[0] + mat[4] + mat[8];

    if (trace > 0.0)
    {
        scale = ldg_math_sqrt_get(trace + 1.0) * 2.0;
        out[3] = 0.25 * scale;
        out[0] = (mat[7] - mat[5]) / scale;
        out[1] = (mat[2] - mat[6]) / scale;
        out[2] = (mat[3] - mat[1]) / scale;
    }
    else if (mat[0] > mat[4] && mat[0] > mat[8])
    {
        scale = ldg_math_sqrt_get(1.0 + mat[0] - mat[4] - mat[8]) * 2.0;
        out[3] = (mat[7] - mat[5]) / scale;
        out[0] = 0.25 * scale;
        out[1] = (mat[1] + mat[3]) / scale;
        out[2] = (mat[2] + mat[6]) / scale;
    }
    else if (mat[4] > mat[8])
    {
        scale = ldg_math_sqrt_get(1.0 + mat[4] - mat[0] - mat[8]) * 2.0;
        out[3] = (mat[2] - mat[6]) / scale;
        out[0] = (mat[1] + mat[3]) / scale;
        out[1] = 0.25 * scale;
        out[2] = (mat[5] + mat[7]) / scale;
    }
    else
    {
        scale = ldg_math_sqrt_get(1.0 + mat[8] - mat[0] - mat[4]) * 2.0;
        out[3] = (mat[3] - mat[1]) / scale;
        out[0] = (mat[2] + mat[6]) / scale;
        out[1] = (mat[5] + mat[7]) / scale;
        out[2] = 0.25 * scale;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_slerp_compute(const double *lhs, const double *rhs, double param, double *out)
{
    double cos_theta = 0.0;
    double theta = 0.0;
    double sin_theta = 0.0;
    double weight_lhs = 0.0;
    double weight_rhs = 0.0;
    double norm = 0.0;
    double inv = 0.0;
    double rcopy[LDG_MATH_QUATERNION_DIM] = { 0 };

    if (LDG_UNLIKELY(!lhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    cos_theta = lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2] + lhs[3] * rhs[3];
    rcopy[0] = rhs[0];
    rcopy[1] = rhs[1];
    rcopy[2] = rhs[2];
    rcopy[3] = rhs[3];

    if (cos_theta < 0.0)
    {
        cos_theta = -cos_theta;
        rcopy[0] = -rcopy[0];
        rcopy[1] = -rcopy[1];
        rcopy[2] = -rcopy[2];
        rcopy[3] = -rcopy[3];
    }

    if ((1.0 - cos_theta) < LDG_MATH_QUATERNION_EPS)
    {
        out[0] = lhs[0] + param * (rcopy[0] - lhs[0]);
        out[1] = lhs[1] + param * (rcopy[1] - lhs[1]);
        out[2] = lhs[2] + param * (rcopy[2] - lhs[2]);
        out[3] = lhs[3] + param * (rcopy[3] - lhs[3]);
        norm = ldg_math_sqrt_get(out[0] * out[0] + out[1] * out[1] + out[2] * out[2] + out[3] * out[3]);
        if (norm > LDG_MATH_QUATERNION_EPS)
        {
            inv = 1.0 / norm;
            out[0] = out[0] * inv;
            out[1] = out[1] * inv;
            out[2] = out[2] * inv;
            out[3] = out[3] * inv;
        }

        return LDG_ERR_AOK;
    }

    theta = ldg_math_arccos_get(cos_theta);
    sin_theta = ldg_math_sin_get(theta);
    weight_lhs = ldg_math_sin_get((1.0 - param) * theta) / sin_theta;
    weight_rhs = ldg_math_sin_get(param * theta) / sin_theta;

    out[0] = weight_lhs * lhs[0] + weight_rhs * rcopy[0];
    out[1] = weight_lhs * lhs[1] + weight_rhs * rcopy[1];
    out[2] = weight_lhs * lhs[2] + weight_rhs * rcopy[2];
    out[3] = weight_lhs * lhs[3] + weight_rhs * rcopy[3];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_nlerp_compute(const double *lhs, const double *rhs, double param, double *out)
{
    double cos_theta = 0.0;
    double norm = 0.0;
    double inv = 0.0;
    double rcopy[LDG_MATH_QUATERNION_DIM] = { 0 };

    if (LDG_UNLIKELY(!lhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    cos_theta = lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2] + lhs[3] * rhs[3];
    rcopy[0] = rhs[0];
    rcopy[1] = rhs[1];
    rcopy[2] = rhs[2];
    rcopy[3] = rhs[3];

    if (cos_theta < 0.0)
    {
        rcopy[0] = -rcopy[0];
        rcopy[1] = -rcopy[1];
        rcopy[2] = -rcopy[2];
        rcopy[3] = -rcopy[3];
    }

    out[0] = lhs[0] + param * (rcopy[0] - lhs[0]);
    out[1] = lhs[1] + param * (rcopy[1] - lhs[1]);
    out[2] = lhs[2] + param * (rcopy[2] - lhs[2]);
    out[3] = lhs[3] + param * (rcopy[3] - lhs[3]);

    norm = ldg_math_sqrt_get(out[0] * out[0] + out[1] * out[1] + out[2] * out[2] + out[3] * out[3]);
    if (LDG_UNLIKELY(norm <= LDG_MATH_QUATERNION_EPS)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv = 1.0 / norm;
    out[0] = out[0] * inv;
    out[1] = out[1] * inv;
    out[2] = out[2] * inv;
    out[3] = out[3] * inv;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_integrate(double *quaternion, const double *omega, double dt)
{
    double increment[LDG_MATH_QUATERNION_DIM] = { 0 };
    double rotvec[LDG_MATH_QUATERNION_VEC_DIM] = { 0 };
    double tmp[LDG_MATH_QUATERNION_DIM] = { 0 };
    double phi_len = 0.0;
    double half = 0.0;
    double scale = 0.0;
    double norm = 0.0;
    double inv = 0.0;

    if (LDG_UNLIKELY(!quaternion)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!omega)) { return LDG_ERR_FUNC_ARG_NULL; }

    rotvec[0] = omega[0] * dt;
    rotvec[1] = omega[1] * dt;
    rotvec[2] = omega[2] * dt;
    phi_len = ldg_math_sqrt_get(rotvec[0] * rotvec[0] + rotvec[1] * rotvec[1] + rotvec[2] * rotvec[2]);

    if (phi_len > LDG_MATH_QUATERNION_EPS)
    {
        half = phi_len * 0.5;
        scale = ldg_math_sin_get(half) / phi_len;
        increment[0] = rotvec[0] * scale;
        increment[1] = rotvec[1] * scale;
        increment[2] = rotvec[2] * scale;
        increment[3] = ldg_math_cos_get(half);
    }
    else
    {
        increment[0] = 0.0;
        increment[1] = 0.0;
        increment[2] = 0.0;
        increment[3] = 1.0;
    }

    tmp[3] = quaternion[3] * increment[3] - quaternion[0] * increment[0] - quaternion[1] * increment[1] - quaternion[2] * increment[2];
    tmp[0] = quaternion[3] * increment[0] + quaternion[0] * increment[3] + quaternion[1] * increment[2] - quaternion[2] * increment[1];
    tmp[1] = quaternion[3] * increment[1] - quaternion[0] * increment[2] + quaternion[1] * increment[3] + quaternion[2] * increment[0];
    tmp[2] = quaternion[3] * increment[2] + quaternion[0] * increment[1] - quaternion[1] * increment[0] + quaternion[2] * increment[3];

    norm = ldg_math_sqrt_get(tmp[0] * tmp[0] + tmp[1] * tmp[1] + tmp[2] * tmp[2] + tmp[3] * tmp[3]);
    if (LDG_UNLIKELY(norm <= LDG_MATH_QUATERNION_EPS)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv = 1.0 / norm;
    quaternion[0] = tmp[0] * inv;
    quaternion[1] = tmp[1] * inv;
    quaternion[2] = tmp[2] * inv;
    quaternion[3] = tmp[3] * inv;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_from_euler_compute(const double *euler, double *out)
{
    double cx = 0.0;
    double sx = 0.0;
    double cy = 0.0;
    double sy = 0.0;
    double cz = 0.0;
    double sz = 0.0;

    if (LDG_UNLIKELY(!euler)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    cx = ldg_math_cos_get(euler[0] * 0.5);
    sx = ldg_math_sin_get(euler[0] * 0.5);
    cy = ldg_math_cos_get(euler[1] * 0.5);
    sy = ldg_math_sin_get(euler[1] * 0.5);
    cz = ldg_math_cos_get(euler[2] * 0.5);
    sz = ldg_math_sin_get(euler[2] * 0.5);

    out[0] = sx * cy * cz + cx * sy * sz;
    out[1] = cx * sy * cz - sx * cy * sz;
    out[2] = cx * cy * sz + sx * sy * cz;
    out[3] = cx * cy * cz - sx * sy * sz;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_to_euler_compute(const double *quaternion, double *out)
{
    double sin_pitch = 0.0;
    double half_pi = 0.0;

    if (LDG_UNLIKELY(!quaternion)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    sin_pitch = 2.0 * (quaternion[0] * quaternion[2] + quaternion[3] * quaternion[1]);
    half_pi = ldg_math_arccos_get(0.0);

    if ((1.0 - sin_pitch) < LDG_MATH_QUATERNION_EPS)
    {
        out[0] = 0.0;
        out[1] = half_pi;
        out[2] = ldg_math_arctan2_get(2.0 * (quaternion[0] * quaternion[1] + quaternion[3] * quaternion[2]), 1.0 - 2.0 * (quaternion[0] * quaternion[0] + quaternion[2] * quaternion[2]));
        return LDG_ERR_AOK;
    }

    if ((sin_pitch + 1.0) < LDG_MATH_QUATERNION_EPS)
    {
        out[0] = 0.0;
        out[1] = -half_pi;
        out[2] = ldg_math_arctan2_get(2.0 * (quaternion[0] * quaternion[1] + quaternion[3] * quaternion[2]), 1.0 - 2.0 * (quaternion[0] * quaternion[0] + quaternion[2] * quaternion[2]));
        return LDG_ERR_AOK;
    }

    out[0] = ldg_math_arctan2_get(2.0 * (quaternion[3] * quaternion[0] - quaternion[1] * quaternion[2]), 1.0 - 2.0 * (quaternion[0] * quaternion[0] + quaternion[1] * quaternion[1]));
    out[1] = ldg_math_arcsin_get(sin_pitch);
    out[2] = ldg_math_arctan2_get(2.0 * (quaternion[3] * quaternion[2] - quaternion[0] * quaternion[1]), 1.0 - 2.0 * (quaternion[1] * quaternion[1] + quaternion[2] * quaternion[2]));

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_from_two_vec3_compute(const double *src, const double *dst, double *out)
{
    double dot = 0.0;
    double abs_x = 0.0;
    double abs_y = 0.0;
    double abs_z = 0.0;
    double axis[LDG_MATH_QUATERNION_VEC_DIM] = { 0 };
    double axis_len = 0.0;
    double inv = 0.0;
    double norm = 0.0;

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    dot = src[0] * dst[0] + src[1] * dst[1] + src[2] * dst[2];

    if ((1.0 - dot) < LDG_MATH_QUATERNION_EPS)
    {
        out[0] = 0.0;
        out[1] = 0.0;
        out[2] = 0.0;
        out[3] = 1.0;
        return LDG_ERR_AOK;
    }

    if ((dot + 1.0) < LDG_MATH_QUATERNION_EPS)
    {
        abs_x = ldg_math_abs_get(src[0]);
        abs_y = ldg_math_abs_get(src[1]);
        abs_z = ldg_math_abs_get(src[2]);

        if (abs_x <= abs_y && abs_x <= abs_z)
        {
            axis[0] = 0.0;
            axis[1] = src[2];
            axis[2] = -src[1];
        }
        else if (abs_y <= abs_z)
        {
            axis[0] = -src[2];
            axis[1] = 0.0;
            axis[2] = src[0];
        }
        else
        {
            axis[0] = src[1];
            axis[1] = -src[0];
            axis[2] = 0.0;
        }

        axis_len = ldg_math_sqrt_get(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
        if (LDG_UNLIKELY(axis_len <= LDG_MATH_QUATERNION_EPS)) { return LDG_ERR_FUNC_ARG_INVALID; }

        inv = 1.0 / axis_len;
        out[0] = axis[0] * inv;
        out[1] = axis[1] * inv;
        out[2] = axis[2] * inv;
        out[3] = 0.0;
        return LDG_ERR_AOK;
    }

    out[0] = src[1] * dst[2] - src[2] * dst[1];
    out[1] = src[2] * dst[0] - src[0] * dst[2];
    out[2] = src[0] * dst[1] - src[1] * dst[0];
    out[3] = 1.0 + dot;

    norm = ldg_math_sqrt_get(out[0] * out[0] + out[1] * out[1] + out[2] * out[2] + out[3] * out[3]);
    if (LDG_UNLIKELY(norm <= LDG_MATH_QUATERNION_EPS)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv = 1.0 / norm;
    out[0] = out[0] * inv;
    out[1] = out[1] * inv;
    out[2] = out[2] * inv;
    out[3] = out[3] * inv;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_exp_compute(const double *quaternion, double *out)
{
    double vec_len = 0.0;
    double exp_w = 0.0;
    double scale = 0.0;

    if (LDG_UNLIKELY(!quaternion)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    vec_len = ldg_math_sqrt_get(quaternion[0] * quaternion[0] + quaternion[1] * quaternion[1] + quaternion[2] * quaternion[2]);
    exp_w = ldg_math_exp_get(quaternion[3]);

    if (vec_len > LDG_MATH_QUATERNION_EPS)
    {
        scale = exp_w * ldg_math_sin_get(vec_len) / vec_len;
        out[0] = quaternion[0] * scale;
        out[1] = quaternion[1] * scale;
        out[2] = quaternion[2] * scale;
        out[3] = exp_w * ldg_math_cos_get(vec_len);
    }
    else
    {
        out[0] = 0.0;
        out[1] = 0.0;
        out[2] = 0.0;
        out[3] = exp_w;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_log_compute(const double *quaternion, double *out)
{
    double vec_len = 0.0;
    double norm = 0.0;
    double angle = 0.0;
    double scale = 0.0;

    if (LDG_UNLIKELY(!quaternion)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    vec_len = ldg_math_sqrt_get(quaternion[0] * quaternion[0] + quaternion[1] * quaternion[1] + quaternion[2] * quaternion[2]);
    norm = ldg_math_sqrt_get(vec_len * vec_len + quaternion[3] * quaternion[3]);
    if (LDG_UNLIKELY(norm <= LDG_MATH_QUATERNION_EPS)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[3] = ldg_math_log_get(norm);

    if (vec_len > LDG_MATH_QUATERNION_EPS)
    {
        angle = ldg_math_arctan2_get(vec_len, quaternion[3]);
        scale = angle / vec_len;
        out[0] = quaternion[0] * scale;
        out[1] = quaternion[1] * scale;
        out[2] = quaternion[2] * scale;
    }
    else
    {
        out[0] = 0.0;
        out[1] = 0.0;
        out[2] = 0.0;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_quaternion_squad_compute(const double *lhs, const double *control_a, const double *control_b, const double *rhs, double param, double *out)
{
    double endpoint[LDG_MATH_QUATERNION_DIM] = { 0 };
    double control[LDG_MATH_QUATERNION_DIM] = { 0 };
    double blend = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!lhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!control_a)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!control_b)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_quaternion_slerp_compute(lhs, rhs, param, endpoint);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_quaternion_slerp_compute(control_a, control_b, param, control);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    blend = 2.0 * param * (1.0 - param);

    return ldg_math_quaternion_slerp_compute(endpoint, control, blend, out);
}
