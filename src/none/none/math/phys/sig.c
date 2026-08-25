
#include <dangling/math/phys.h>
#include <dangling/math/math.h>
#include <dangling/mem/secure.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_MATH_PHYS_SIG_RAMP_PERIOD_CUNT 4.0
#define LDG_MATH_PHYS_SIG_PULSE_SIGMA_PERIOD_CUNT 2.0
#define LDG_MATH_PHYS_SIG_PULSE_CENTER_PERIOD_CUNT 6.0
#define LDG_MATH_PHYS_SIG_DERIVATIVE_CENTER_TAU_CUNT 6.0

static uint64_t ldg_math_phys_sig_prbs_mask_get(uint32_t order)
{
    if (order == 7) { return (1ULL << 6) | (1ULL << 5); }

    if (order == 9) { return (1ULL << 8) | (1ULL << 4); }

    if (order == 11) { return (1ULL << 10) | (1ULL << 8); }

    if (order == 15) { return (1ULL << 14) | (1ULL << 13); }

    if (order == 23) { return (1ULL << 22) | (1ULL << 17); }

    return (1ULL << 30) | (1ULL << 27);
}

static void ldg_math_phys_sig_prbs_advance(ldg_math_phys_sig_t *sig, uint64_t bit_idx)
{
    uint64_t feedback_mask = ldg_math_phys_sig_prbs_mask_get(sig->prbs_order);
    uint64_t width_mask = (1ULL << sig->prbs_order) - 1ULL;
    uint64_t out_bit = 0;
    if (bit_idx < sig->prbs_bit_idx || sig->prbs_state == 0)
    {
        sig->prbs_state = sig->prbs_seed & width_mask;
        if (sig->prbs_state == 0) { sig->prbs_state = 1; }

        sig->prbs_bit_idx = 0;
        sig->prbs_prev_bit_val = sig->prbs_state & 1ULL;
    }

    while (sig->prbs_bit_idx < bit_idx)
    {
        out_bit = sig->prbs_state & 1ULL;
        sig->prbs_prev_bit_val = out_bit;
        sig->prbs_state = sig->prbs_state >> 1;
        if (out_bit) { sig->prbs_state = sig->prbs_state ^ feedback_mask; }

        sig->prbs_bit_idx++;
    }

    sig->prbs_bit_val = sig->prbs_state & 1ULL;
}

static uint32_t ldg_math_phys_sig_base_fill(double amplitude, double freq, uint32_t kind, ldg_math_phys_sig_t *out)
{
    uint32_t err = 0;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(out, sizeof(ldg_math_phys_sig_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(amplitude))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(freq) || freq <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out->amplitude = amplitude;
    out->freq = freq;
    out->kind = kind;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_sig_sine_init(double amplitude, double freq, ldg_math_phys_sig_t *out)
{
    uint32_t err = 0;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(out, sizeof(ldg_math_phys_sig_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(amplitude))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(freq) || freq <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out->amplitude = amplitude;
    out->freq = freq;
    out->kind = LDG_MATH_PHYS_SIG_SINE;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_sig_gauss_pulse_init(double amplitude, double freq, ldg_math_phys_sig_t *out)
{
    uint32_t err = 0;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(out, sizeof(ldg_math_phys_sig_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(amplitude))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(freq) || freq <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out->amplitude = amplitude;
    out->freq = freq;
    out->kind = LDG_MATH_PHYS_SIG_GAUSS_PULSE;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_sig_gauss_derivative_init(double amplitude, double freq, ldg_math_phys_sig_t *out)
{
    uint32_t err = 0;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(out, sizeof(ldg_math_phys_sig_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(amplitude))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(freq) || freq <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out->amplitude = amplitude;
    out->freq = freq;
    out->kind = LDG_MATH_PHYS_SIG_GAUSS_DERIVATIVE;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_sig_ricker_init(double amplitude, double freq, ldg_math_phys_sig_t *out)
{
    uint32_t err = 0;
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(out, sizeof(ldg_math_phys_sig_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(amplitude))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(freq) || freq <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out->amplitude = amplitude;
    out->freq = freq;
    out->kind = LDG_MATH_PHYS_SIG_RICKER;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_sig_prbs_init(double amplitude, double freq, uint32_t order, uint64_t seed, double edge_fraction, ldg_math_phys_sig_t *out)
{
    uint64_t width_mask = 0;
    uint32_t err = 0;
    err = ldg_math_phys_sig_base_fill(amplitude, freq, LDG_MATH_PHYS_SIG_PRBS, out);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(order != 7 && order != 9 && order != 11 && order != 15 && order != 23 && order != 31)) { return LDG_ERR_FUNC_ARG_INVALID; }

    width_mask = (1ULL << order) - 1ULL;
    if (LDG_UNLIKELY((seed & width_mask) == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(edge_fraction) || edge_fraction < 0.0 || edge_fraction >= 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out->prbs_order = order;
    out->prbs_seed = seed;
    out->edge_fraction = edge_fraction;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_sig_multitone_init(double amplitude, const double *tone_freq, const double *tone_amplitude, const double *tone_phase, uint32_t tone_cunt, ldg_math_phys_sig_t *out)
{
    uint32_t err = 0;
    uint32_t tone_idx = 0;
    if (LDG_UNLIKELY(!tone_freq)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!tone_amplitude)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!tone_phase)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(tone_cunt == 0 || tone_cunt > LDG_MATH_PHYS_SIG_TONE_CAP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_phys_sig_base_fill(amplitude, tone_freq[0], LDG_MATH_PHYS_SIG_MULTITONE, out);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    for (tone_idx = 0; tone_idx < tone_cunt; tone_idx++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(tone_freq[tone_idx]) || tone_freq[tone_idx] <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(tone_amplitude[tone_idx]))) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(!ldg_math_finite_is(tone_phase[tone_idx]))) { return LDG_ERR_FUNC_ARG_INVALID; }

        out->tone_freq[tone_idx] = tone_freq[tone_idx];
        out->tone_amplitude[tone_idx] = tone_amplitude[tone_idx];
        out->tone_phase[tone_idx] = tone_phase[tone_idx];
    }

    out->tone_cunt = tone_cunt;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_sig_chirp_init(double amplitude, double freq, double freq_end, double sweep_time, ldg_math_phys_sig_t *out)
{
    uint32_t err = 0;
    err = ldg_math_phys_sig_base_fill(amplitude, freq, LDG_MATH_PHYS_SIG_CHIRP, out);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(freq_end) || freq_end <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(sweep_time) || sweep_time <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out->freq_end = freq_end;
    out->sweep_time = sweep_time;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_sig_sampled_init(double amplitude, const double *series, uint64_t cunt, ldg_math_phys_sig_t *out)
{
    uint32_t err = 0;
    if (LDG_UNLIKELY(!series)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(out, sizeof(ldg_math_phys_sig_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(!ldg_math_finite_is(amplitude))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out->amplitude = amplitude;
    out->series = series;
    out->series_cunt = cunt;
    out->kind = LDG_MATH_PHYS_SIG_SAMPLED;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_sig_sample_get(ldg_math_phys_sig_t *sig, uint64_t tick, double dt, double *out)
{
    double phase = 0.0;
    double period_tick = 0.0;
    double time_val = 0.0;
    double envelope = 0.0;
    double ramp_tick = 0.0;
    double ramp = 1.0;
    double tau = 0.0;
    double pulse_arg = 0.0;
    double acc = 0.0;
    double tone_ramp = 0.0;
    double tone_ramp_tick = 0.0;
    double bit_pos = 0.0;
    double bit_frac = 0.0;
    double cur_val = 0.0;
    double prev_val = 0.0;
    double blend = 0.0;
    double tail_time = 0.0;
    uint64_t bit_idx = 0;
    uint32_t tone_idx = 0;
    if (LDG_UNLIKELY(!sig)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;
    if (LDG_UNLIKELY(!ldg_math_finite_is(dt) || dt <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(sig->kind >= LDG_MATH_PHYS_SIG_KIND_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (sig->series != 0x0)
    {
        if (tick < sig->series_cunt) { *out = sig->amplitude * sig->series[tick]; }

        return LDG_ERR_AOK;
    }

    time_val = (double)tick * dt;
    phase = LDG_MATH_TAU * sig->freq * time_val;
    period_tick = 1.0 / (sig->freq * dt);
    if (sig->kind == LDG_MATH_PHYS_SIG_SINE)
    {
        ramp_tick = LDG_MATH_PHYS_SIG_RAMP_PERIOD_CUNT * period_tick;
        if ((double)tick < ramp_tick) { ramp = 0.5 * (1.0 - ldg_math_cos_get(LDG_MATH_PI * (double)tick / ramp_tick)); }

        *out = sig->amplitude * ramp * ldg_math_sin_get(phase);
        return LDG_ERR_AOK;
    }

    if (sig->kind == LDG_MATH_PHYS_SIG_GAUSS_PULSE)
    {
        envelope = ((double)tick - LDG_MATH_PHYS_SIG_PULSE_CENTER_PERIOD_CUNT * period_tick) / (LDG_MATH_PHYS_SIG_PULSE_SIGMA_PERIOD_CUNT * period_tick);
        *out = sig->amplitude * ldg_math_exp_get((0.0 - envelope) * envelope) * ldg_math_sin_get(phase);
        return LDG_ERR_AOK;
    }

    if (sig->kind == LDG_MATH_PHYS_SIG_GAUSS_DERIVATIVE)
    {
        tau = 1.0 / (LDG_MATH_TAU * sig->freq);
        pulse_arg = (time_val - LDG_MATH_PHYS_SIG_DERIVATIVE_CENTER_TAU_CUNT * tau) / tau;
        *out = sig->amplitude * (0.0 - pulse_arg) * ldg_math_exp_get(0.5 - 0.5 * pulse_arg * pulse_arg);
        return LDG_ERR_AOK;
    }

    if (sig->kind == LDG_MATH_PHYS_SIG_RICKER)
    {
        tau = ldg_math_sqrt_get(2.0) / (LDG_MATH_TAU * sig->freq);
        pulse_arg = (time_val - LDG_MATH_PHYS_SIG_DERIVATIVE_CENTER_TAU_CUNT * tau) / tau;
        *out = sig->amplitude * (1.0 - pulse_arg * pulse_arg) * ldg_math_exp_get((0.0 - 0.5) * pulse_arg * pulse_arg);
        return LDG_ERR_AOK;
    }

    if (sig->kind == LDG_MATH_PHYS_SIG_PRBS)
    {
        if (LDG_UNLIKELY(sig->freq * dt > 0.5)) { return LDG_ERR_FUNC_ARG_INVALID; }

        bit_pos = time_val * sig->freq;
        bit_idx = (uint64_t)bit_pos;
        bit_frac = bit_pos - (double)bit_idx;
        ldg_math_phys_sig_prbs_advance(sig, bit_idx);
        cur_val = 2.0 * (double)sig->prbs_bit_val - 1.0;
        prev_val = 2.0 * (double)sig->prbs_prev_bit_val - 1.0;
        if (bit_idx == 0) { prev_val = cur_val; }

        if (sig->edge_fraction > 0.0 && bit_frac < sig->edge_fraction)
        {
            blend = 0.5 * (1.0 - ldg_math_cos_get(LDG_MATH_PI * bit_frac / sig->edge_fraction));
            *out = sig->amplitude * (prev_val + (cur_val - prev_val) * blend);
            return LDG_ERR_AOK;
        }

        *out = sig->amplitude * cur_val;
        return LDG_ERR_AOK;
    }

    if (sig->kind == LDG_MATH_PHYS_SIG_MULTITONE)
    {
        for (tone_idx = 0; tone_idx < sig->tone_cunt; tone_idx++)
        {
            tone_ramp = 1.0;
            tone_ramp_tick = LDG_MATH_PHYS_SIG_RAMP_PERIOD_CUNT / (sig->tone_freq[tone_idx] * dt);
            if ((double)tick < tone_ramp_tick) { tone_ramp = 0.5 * (1.0 - ldg_math_cos_get(LDG_MATH_PI * (double)tick / tone_ramp_tick)); }

            acc += sig->tone_amplitude[tone_idx] * tone_ramp* ldg_math_sin_get(LDG_MATH_TAU *sig->tone_freq[tone_idx] *time_val + sig->tone_phase[tone_idx]);
        }

        *out = sig->amplitude * acc;
        return LDG_ERR_AOK;
    }

    if (sig->kind == LDG_MATH_PHYS_SIG_CHIRP)
    {
        if (time_val >= sig->sweep_time) { return LDG_ERR_AOK; }

        phase = LDG_MATH_TAU * (sig->freq * time_val + 0.5 * (sig->freq_end - sig->freq) * time_val * time_val / sig->sweep_time);
        ramp_tick = LDG_MATH_PHYS_SIG_RAMP_PERIOD_CUNT * period_tick;
        if ((double)tick < ramp_tick) { ramp = 0.5 * (1.0 - ldg_math_cos_get(LDG_MATH_PI * (double)tick / ramp_tick)); }

        tail_time = LDG_MATH_PHYS_SIG_RAMP_PERIOD_CUNT / sig->freq_end;
        if (time_val > sig->sweep_time - tail_time) { ramp = ramp * 0.5 * (1.0 - ldg_math_cos_get(LDG_MATH_PI * (sig->sweep_time - time_val) / tail_time)); }

        *out = sig->amplitude * ramp * ldg_math_sin_get(phase);
        return LDG_ERR_AOK;
    }

    return LDG_ERR_AOK;
}
