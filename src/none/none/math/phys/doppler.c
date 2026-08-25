
#include <dangling/math/phys.h>
#include <dangling/math/math.h>
#include <dangling/core/macros.h>
#include <dangling/core/err.h>

static uint32_t ldg_math_phys_doppler_vec_finite_ck(const double *vec)
{
    uint32_t axis = 0;

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { if (LDG_UNLIKELY(!ldg_math_finite_is(vec[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; } }
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_doppler_retarded_sight_get(const double *src_pos, const double *src_velocity, const double *receiver_pos, double wave_speed, double *sight)
{
    double gap[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double gap_dot_vel = 0.0;
    double gap_sq = 0.0;
    double speed_sq = 0.0;
    double denom = 0.0;
    double disc = 0.0;
    double transit = 0.0;
    double sight_len = 0.0;
    uint32_t axis = 0;

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        gap[axis] = receiver_pos[axis] - src_pos[axis];
        gap_dot_vel += gap[axis] * src_velocity[axis];
        gap_sq += gap[axis] * gap[axis];
        speed_sq += src_velocity[axis] * src_velocity[axis];
    }
    if (LDG_UNLIKELY(gap_sq <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    denom = wave_speed * wave_speed - speed_sq;
    if (LDG_UNLIKELY(denom <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    disc = gap_dot_vel * gap_dot_vel + denom * gap_sq;
    transit = (gap_dot_vel + ldg_math_sqrt_get(disc)) / denom;
    sight_len = 0.0;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        sight[axis] = gap[axis] + transit * src_velocity[axis];
        sight_len += sight[axis] * sight[axis];
    }
    if (LDG_UNLIKELY(sight_len <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    sight_len = ldg_math_sqrt_get(sight_len);
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { sight[axis] /= sight_len; }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_doppler_acoustic_freq_get(const double *src_pos, const double *src_velocity, const double *receiver_pos, const double *receiver_velocity, double wave_speed, double freq, double *out)
{
    double sight[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double src_radial = 0.0;
    double receiver_radial = 0.0;
    double shifted = 0.0;
    uint32_t err = 0;
    uint32_t axis = 0;

    if (LDG_UNLIKELY(!src_pos || !src_velocity || !receiver_pos || !receiver_velocity)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!ldg_math_finite_is(wave_speed) || wave_speed <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(freq) || freq <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_doppler_vec_finite_ck(src_pos);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_doppler_vec_finite_ck(src_velocity);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_doppler_vec_finite_ck(receiver_pos);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_doppler_vec_finite_ck(receiver_velocity);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_doppler_retarded_sight_get(src_pos, src_velocity, receiver_pos, wave_speed, sight);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        src_radial += src_velocity[axis] * sight[axis];
        receiver_radial += receiver_velocity[axis] * sight[axis];
    }
    shifted = freq * (wave_speed - receiver_radial) / (wave_speed - src_radial);
    if (LDG_UNLIKELY(shifted < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = shifted;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_doppler_em_freq_get(const double *src_pos, const double *src_velocity, const double *receiver_pos, double freq, double *out)
{
    double sight[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double speed_sq = 0.0;
    double beta_radial = 0.0;
    double gamma_inv = 0.0;
    uint32_t err = 0;
    uint32_t axis = 0;

    if (LDG_UNLIKELY(!src_pos || !src_velocity || !receiver_pos)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!ldg_math_finite_is(freq) || freq <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_doppler_vec_finite_ck(src_pos);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_doppler_vec_finite_ck(src_velocity);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_math_phys_doppler_vec_finite_ck(receiver_pos);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { speed_sq += src_velocity[axis] * src_velocity[axis]; }
    if (LDG_UNLIKELY(speed_sq >= LDG_MATH_PHYS_VACUUM_LIGHT_SPEED * LDG_MATH_PHYS_VACUUM_LIGHT_SPEED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_doppler_retarded_sight_get(src_pos, src_velocity, receiver_pos, LDG_MATH_PHYS_VACUUM_LIGHT_SPEED, sight);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { beta_radial += src_velocity[axis] * sight[axis]; }
    beta_radial /= LDG_MATH_PHYS_VACUUM_LIGHT_SPEED;
    gamma_inv = ldg_math_sqrt_get(1.0 - speed_sq / (LDG_MATH_PHYS_VACUUM_LIGHT_SPEED * LDG_MATH_PHYS_VACUUM_LIGHT_SPEED));
    *out = freq * gamma_inv / (1.0 - beta_radial);
    return LDG_ERR_AOK;
}
