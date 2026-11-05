#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>

#define CALIBRATION_NONE 0.0
#define CALIBRATION_FULL 1.0
#define CALIBRATION_PASS_MAX 100
#define CALIBRATION_GRAD_TOL 1e-9
#define CALIBRATION_RIDGE 1e-12
#define CALIBRATION_ARMIJO 1e-4
#define CALIBRATION_BACKOFF 0.5
#define CALIBRATION_STEP_MIN 1e-10
#define CALIBRATION_STEP_TOL 1e-8
#define CALIBRATION_PRIOR_ADD 1.0
#define CALIBRATION_PRIOR_SPAN 2.0
#define CALIBRATION_SCALE_MIN 1e-3
#define CALIBRATION_SCALE_MAX 1e3
#define CALIBRATION_CURVE_MIN 1e-12
#define CALIBRATION_CLASS_MIN 2

typedef struct calibration_platt_step
{
    double grad_slope;
    double grad_offset;
    double curve_slope;
    double curve_cross;
    double curve_offset;
} calibration_platt_step_t;

typedef struct calibration_scale_step
{
    double loss;
    double grad;
    double curve;
} calibration_scale_step_t;

static double calibration_softplus_get(double val)
{
    if (val > CALIBRATION_NONE) { return val + ldg_math_log_get(CALIBRATION_FULL + ldg_math_exp_get(-val)); }

    return ldg_math_log_get(CALIBRATION_FULL + ldg_math_exp_get(val));
}

static double calibration_sigmoid_get(double val)
{
    double grow = CALIBRATION_NONE;

    if (val >= CALIBRATION_NONE) { return CALIBRATION_FULL / (CALIBRATION_FULL + ldg_math_exp_get(-val)); }

    grow = ldg_math_exp_get(val);

    return grow / (CALIBRATION_FULL + grow);
}

static uint32_t calibration_binary_ck(const double *score, const uint8_t *label, uint32_t cunt, uint32_t *out_pos_cunt)
{
    uint32_t pos_cunt = 0;
    uint32_t idx = 0;

    *out_pos_cunt = 0;

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(score[idx]) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(label[idx] != LDG_TRUTH_TRUE && label[idx] != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (label[idx] == LDG_TRUTH_TRUE) { pos_cunt++; }

        idx++;
    }

    *out_pos_cunt = pos_cunt;

    return LDG_ERR_AOK;
}

static double calibration_platt_loss_get(const double *score, const uint8_t *label, uint32_t cunt, double slope, double offset, double hi_tgt, double lo_tgt)
{
    double logit = CALIBRATION_NONE;
    double goal = CALIBRATION_NONE;
    double loss = CALIBRATION_NONE;
    uint32_t idx = 0;

    while (idx < cunt)
    {
        logit = (slope * score[idx]) + offset;
        goal = (label[idx] == LDG_TRUTH_TRUE) ? hi_tgt : lo_tgt;
        loss += (goal * calibration_softplus_get(-logit)) + ((CALIBRATION_FULL - goal) * calibration_softplus_get(logit));
        idx++;
    }

    return loss;
}

static void calibration_platt_step_fill(const double *score, const uint8_t *label, uint32_t cunt, double slope, double offset, double hi_tgt, double lo_tgt, calibration_platt_step_t *out)
{
    double share = CALIBRATION_NONE;
    double miss = CALIBRATION_NONE;
    double weight = CALIBRATION_NONE;
    uint32_t idx = 0;

    out->grad_slope = CALIBRATION_NONE;
    out->grad_offset = CALIBRATION_NONE;
    out->curve_slope = CALIBRATION_RIDGE;
    out->curve_cross = CALIBRATION_NONE;
    out->curve_offset = CALIBRATION_RIDGE;

    while (idx < cunt)
    {
        share = calibration_sigmoid_get((slope * score[idx]) + offset);
        miss = share - ((label[idx] == LDG_TRUTH_TRUE) ? hi_tgt : lo_tgt);
        weight = share * (CALIBRATION_FULL - share);
        out->grad_slope += miss * score[idx];
        out->grad_offset += miss;
        out->curve_slope += weight * score[idx] * score[idx];
        out->curve_cross += weight * score[idx];
        out->curve_offset += weight;
        idx++;
    }
}

static uint8_t calibration_platt_step_take(const double *score, const uint8_t *label, uint32_t cunt, double hi_tgt, double lo_tgt, const calibration_platt_step_t *step, ldg_math_stat_platt_t *fit)
{
    double span = (step->curve_slope * step->curve_offset) - (step->curve_cross * step->curve_cross);
    double mov_slope = -((step->curve_offset * step->grad_slope) - (step->curve_cross * step->grad_offset)) / span;
    double mov_offset = -((step->curve_slope * step->grad_offset) - (step->curve_cross * step->grad_slope)) / span;
    double descent = (step->grad_slope * mov_slope) + (step->grad_offset * mov_offset);
    double scale = CALIBRATION_FULL;
    double trial = CALIBRATION_NONE;

    while (scale >= CALIBRATION_STEP_MIN)
    {
        trial = calibration_platt_loss_get(score, label, cunt, fit->slope + (scale * mov_slope), fit->offset + (scale * mov_offset), hi_tgt, lo_tgt);

        if (trial < (fit->loss + (CALIBRATION_ARMIJO * scale * descent)))
        {
            fit->slope += scale * mov_slope;
            fit->offset += scale * mov_offset;
            fit->loss = trial;
            return LDG_TRUTH_TRUE;
        }

        scale *= CALIBRATION_BACKOFF;
    }

    return LDG_TRUTH_FALSE;
}

static void calibration_row_cum(const double *row, uint32_t class_cunt, uint32_t label, double scale, calibration_scale_step_t *step)
{
    double peak = row[0] * scale;
    double weight = CALIBRATION_NONE;
    double total = CALIBRATION_NONE;
    double mean = CALIBRATION_NONE;
    double square = CALIBRATION_NONE;
    uint32_t idx = 0;

    while (idx < class_cunt)
    {
        if ((row[idx] * scale) > peak) { peak = row[idx] * scale; }

        idx++;
    }

    idx = 0;

    while (idx < class_cunt)
    {
        weight = ldg_math_exp_get((row[idx] * scale) - peak);
        total += weight;
        mean += weight * row[idx];
        square += weight * row[idx] * row[idx];
        idx++;
    }

    mean /= total;
    step->loss += (peak + ldg_math_log_get(total)) - (scale * row[label]);
    step->grad += mean - row[label];
    step->curve += (square / total) - (mean * mean);
}

static void calibration_scale_step_fill(const double *logit, const uint32_t *label, uint32_t row_cunt, uint32_t class_cunt, double scale, calibration_scale_step_t *out)
{
    const double *row_ptr = logit;
    uint32_t row = 0;

    out->loss = CALIBRATION_NONE;
    out->grad = CALIBRATION_NONE;
    out->curve = CALIBRATION_NONE;

    while (row < row_cunt)
    {
        calibration_row_cum(row_ptr, class_cunt, label[row], scale, out);
        row_ptr += class_cunt;
        row++;
    }
}

static double calibration_scale_clamp(double scale)
{
    if (scale < CALIBRATION_SCALE_MIN) { return CALIBRATION_SCALE_MIN; }

    if (scale > CALIBRATION_SCALE_MAX) { return CALIBRATION_SCALE_MAX; }

    return scale;
}

static uint8_t calibration_scale_step_take(const double *logit, const uint32_t *label, uint32_t row_cunt, uint32_t class_cunt, double *scale, calibration_scale_step_t *step)
{
    calibration_scale_step_t trial = LDG_STRUCT_ZERO_INIT;
    double mov = (step->curve > CALIBRATION_CURVE_MIN) ? (-step->grad / step->curve) : ((step->grad > CALIBRATION_NONE) ? -(*scale) : *scale);
    double share = CALIBRATION_FULL;
    double next = CALIBRATION_NONE;

    while (share >= CALIBRATION_STEP_MIN)
    {
        next = calibration_scale_clamp(*scale + (share * mov));
        calibration_scale_step_fill(logit, label, row_cunt, class_cunt, next, &trial);

        if (trial.loss < step->loss)
        {
            *scale = next;
            *step = trial;
            return LDG_TRUTH_TRUE;
        }

        share *= CALIBRATION_BACKOFF;
    }

    return LDG_TRUTH_FALSE;
}

static uint32_t calibration_logit_ck(const double *logit, const uint32_t *label, uint32_t row_cunt, uint32_t class_cunt)
{
    uintptr_t elem_cunt = (uintptr_t)row_cunt * (uintptr_t)class_cunt;
    uintptr_t elem = 0;
    uint32_t row = 0;

    while (elem < elem_cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(logit[elem]) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        elem++;
    }

    while (row < row_cunt)
    {
        if (LDG_UNLIKELY(label[row] >= class_cunt)) { return LDG_ERR_BOUNDS; }

        row++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_platt_fit(const double *score, const uint8_t *label, uint32_t cunt, ldg_math_stat_platt_t *out)
{
    calibration_platt_step_t step = LDG_STRUCT_ZERO_INIT;
    double hi_tgt = CALIBRATION_NONE;
    double lo_tgt = CALIBRATION_NONE;
    double pos = CALIBRATION_NONE;
    double neg = CALIBRATION_NONE;
    uint32_t pos_cunt = 0;
    uint32_t ret = 0;
    uint8_t advance_is = LDG_TRUTH_TRUE;

    if (LDG_UNLIKELY(!score || !label || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->slope = CALIBRATION_NONE;
    out->offset = CALIBRATION_NONE;
    out->loss = CALIBRATION_NONE;
    out->pass_cunt = 0;
    out->converged_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = calibration_binary_ck(score, label, cunt, &pos_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(pos_cunt == 0 || pos_cunt == cunt)) { return LDG_ERR_MATH_STAT_INVALID_DIST; }

    pos = (double)pos_cunt;
    neg = (double)(cunt - pos_cunt);
    hi_tgt = (pos + CALIBRATION_PRIOR_ADD) / (pos + CALIBRATION_PRIOR_SPAN);
    lo_tgt = CALIBRATION_PRIOR_ADD / (neg + CALIBRATION_PRIOR_SPAN);
    out->offset = ldg_math_log_get((pos + CALIBRATION_PRIOR_ADD) / (neg + CALIBRATION_PRIOR_ADD));
    out->loss = calibration_platt_loss_get(score, label, cunt, out->slope, out->offset, hi_tgt, lo_tgt);

    while (out->pass_cunt < CALIBRATION_PASS_MAX && advance_is && !out->converged_is)
    {
        calibration_platt_step_fill(score, label, cunt, out->slope, out->offset, hi_tgt, lo_tgt, &step);

        if (ldg_math_abs_get(step.grad_slope) < (CALIBRATION_GRAD_TOL * (double)cunt) && ldg_math_abs_get(step.grad_offset) < (CALIBRATION_GRAD_TOL * (double)cunt)) { out->converged_is = LDG_TRUTH_TRUE; }
        else { advance_is = calibration_platt_step_take(score, label, cunt, hi_tgt, lo_tgt, &step, out); }

        out->pass_cunt++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_platt_apply(const ldg_math_stat_platt_t *fit, double score, double *out)
{
    if (LDG_UNLIKELY(!fit || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = CALIBRATION_NONE;

    if (LDG_UNLIKELY(ldg_math_finite_is(score) != LDG_TRUTH_TRUE || ldg_math_finite_is(fit->slope) != LDG_TRUTH_TRUE || ldg_math_finite_is(fit->offset) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = calibration_sigmoid_get((fit->slope * score) + fit->offset);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_temperature_fit(const double *logit, const uint32_t *label, uint32_t row_cunt, uint32_t class_cunt, ldg_math_stat_temperature_t *out)
{
    calibration_scale_step_t step = LDG_STRUCT_ZERO_INIT;
    double scale = CALIBRATION_FULL;
    uint32_t ret = 0;
    uint8_t advance_is = LDG_TRUTH_TRUE;

    if (LDG_UNLIKELY(!logit || !label || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->temperature = CALIBRATION_FULL;
    out->loss = CALIBRATION_NONE;
    out->pass_cunt = 0;
    out->converged_is = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(row_cunt == 0 || class_cunt < CALIBRATION_CLASS_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = calibration_logit_ck(logit, label, row_cunt, class_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    calibration_scale_step_fill(logit, label, row_cunt, class_cunt, scale, &step);

    while (out->pass_cunt < CALIBRATION_PASS_MAX && advance_is && !out->converged_is)
    {
        if (ldg_math_abs_get(step.grad) < (CALIBRATION_GRAD_TOL * (double)row_cunt) || (step.curve > CALIBRATION_CURVE_MIN && ldg_math_abs_get(step.grad / step.curve) < (CALIBRATION_STEP_TOL * scale))) { out->converged_is = LDG_TRUTH_TRUE; }
        else { advance_is = calibration_scale_step_take(logit, label, row_cunt, class_cunt, &scale, &step); }

        out->pass_cunt++;
    }

    out->temperature = CALIBRATION_FULL / scale;
    out->loss = step.loss;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_temperature_apply(const ldg_math_stat_temperature_t *fit, const double *logit, uint32_t class_cunt, double *dst, uint32_t cap)
{
    double peak = CALIBRATION_NONE;
    double total = CALIBRATION_NONE;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!fit || !logit || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(class_cunt < CALIBRATION_CLASS_MIN || ldg_math_finite_is(fit->temperature) != LDG_TRUTH_TRUE || fit->temperature <= CALIBRATION_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(cap < class_cunt)) { return LDG_ERR_OVERFLOW; }

    while (idx < class_cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(logit[idx]) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        idx++;
    }

    peak = logit[0];
    idx = 0;

    while (idx < class_cunt)
    {
        if (logit[idx] > peak) { peak = logit[idx]; }

        idx++;
    }

    idx = 0;

    while (idx < class_cunt)
    {
        dst[idx] = ldg_math_exp_get((logit[idx] - peak) / fit->temperature);
        total += dst[idx];
        idx++;
    }

    idx = 0;

    while (idx < class_cunt)
    {
        dst[idx] /= total;
        idx++;
    }

    return LDG_ERR_AOK;
}
