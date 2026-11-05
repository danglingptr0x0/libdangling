#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>

#define EVALUATION_NONE 0.0
#define EVALUATION_HARMONIC_SCALE 2.0
#define EVALUATION_RANK_BASE 1.0
#define EVALUATION_RANK_MEAN_WEIGHT 0.5
#define EVALUATION_RANK_FLOOR_DIV 2.0
#define EVALUATION_HEAP_ARITY 2
#define EVALUATION_HEAP_ROOT 0
#define EVALUATION_ADJACENT_OFF 1

static void evaluation_order_sift(uint32_t *order, const double *key, uint32_t start, uint32_t end)
{
    uint32_t root = start;
    uint32_t child = 0;
    uint32_t pick = 0;
    uint32_t held = 0;

    while (((root * EVALUATION_HEAP_ARITY) + EVALUATION_ADJACENT_OFF) <= end)
    {
        child = (root * EVALUATION_HEAP_ARITY) + EVALUATION_ADJACENT_OFF;
        pick = root;

        if (key[order[pick]] < key[order[child]]) { pick = child; }

        if (child < end && key[order[pick]] < key[order[child + EVALUATION_ADJACENT_OFF]]) { pick = child + EVALUATION_ADJACENT_OFF; }

        if (pick == root) { return; }

        held = order[root];
        order[root] = order[pick];
        order[pick] = held;
        root = pick;
    }
}

static void evaluation_order_sort(uint32_t *order, const double *key, uint32_t cunt)
{
    uint32_t start = cunt / EVALUATION_HEAP_ARITY;
    uint32_t end = cunt - EVALUATION_ADJACENT_OFF;
    uint32_t held = 0;
    uint32_t idx = 0;

    while (idx < cunt)
    {
        order[idx] = idx;
        idx++;
    }

    while (start > 0)
    {
        start--;
        evaluation_order_sift(order, key, start, end);
    }

    while (end > 0)
    {
        held = order[EVALUATION_HEAP_ROOT];
        order[EVALUATION_HEAP_ROOT] = order[end];
        order[end] = held;
        end--;
        evaluation_order_sift(order, key, EVALUATION_HEAP_ROOT, end);
    }
}

static uint32_t evaluation_input_ck(const double *score, const uint8_t *label, uint32_t cunt, uint32_t *out_pos_cunt)
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

static double evaluation_pos_rank_cum(const double *score, const uint8_t *label, const uint32_t *order, uint32_t cunt)
{
    double tied = EVALUATION_NONE;
    double cum = EVALUATION_NONE;
    uint32_t run_start = 0;
    uint32_t run_end = 0;
    uint32_t idx = 0;

    while (run_start < cunt)
    {
        run_end = run_start + EVALUATION_ADJACENT_OFF;

        while (run_end < cunt && score[order[run_end]] <= score[order[run_start]]) { run_end++; }

        tied = EVALUATION_RANK_BASE + (EVALUATION_RANK_MEAN_WEIGHT * ((double)run_start + (double)(run_end - EVALUATION_ADJACENT_OFF)));
        idx = run_start;

        while (idx < run_end)
        {
            if (label[order[idx]] == LDG_TRUTH_TRUE) { cum += tied; }

            idx++;
        }

        run_start = run_end;
    }

    return cum;
}

static double evaluation_ratio_get(uint64_t part, uint64_t whole)
{
    if (whole == 0) { return EVALUATION_NONE; }

    return (double)part / (double)whole;
}

uint32_t ldg_math_stat_confusion_cum(const uint32_t *truth, const uint32_t *predicted, uint32_t cunt, uint32_t class_cunt, uint64_t *mat)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!truth || !predicted || !mat)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(class_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(truth[idx] >= class_cunt || predicted[idx] >= class_cunt)) { return LDG_ERR_BOUNDS; }

        idx++;
    }

    idx = 0;

    while (idx < cunt)
    {
        mat[((uintptr_t)truth[idx] * (uintptr_t)class_cunt) + (uintptr_t)predicted[idx]]++;
        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_confusion_score_get(const uint64_t *mat, uint32_t class_cunt, uint32_t label, ldg_math_stat_score_t *out)
{
    uint64_t hit_cunt = 0;
    uint64_t truth_cunt = 0;
    uint64_t predicted_cunt = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!mat || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->precision = EVALUATION_NONE;
    out->recall = EVALUATION_NONE;
    out->f1 = EVALUATION_NONE;
    out->support = 0;

    if (LDG_UNLIKELY(class_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(label >= class_cunt)) { return LDG_ERR_BOUNDS; }

    while (idx < class_cunt)
    {
        truth_cunt += mat[((uintptr_t)label * (uintptr_t)class_cunt) + (uintptr_t)idx];
        predicted_cunt += mat[((uintptr_t)idx * (uintptr_t)class_cunt) + (uintptr_t)label];
        idx++;
    }

    hit_cunt = mat[((uintptr_t)label * (uintptr_t)class_cunt) + (uintptr_t)label];
    out->precision = evaluation_ratio_get(hit_cunt, predicted_cunt);
    out->recall = evaluation_ratio_get(hit_cunt, truth_cunt);
    out->support = truth_cunt;

    if ((out->precision + out->recall) > EVALUATION_NONE) { out->f1 = (EVALUATION_HARMONIC_SCALE * out->precision * out->recall) / (out->precision + out->recall); }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_confusion_accuracy_get(const uint64_t *mat, uint32_t class_cunt, double *out)
{
    uint64_t hit_cunt = 0;
    uint64_t total = 0;
    uintptr_t elem_cunt = 0;
    uintptr_t elem = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!mat || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = EVALUATION_NONE;

    if (LDG_UNLIKELY(class_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    elem_cunt = (uintptr_t)class_cunt * (uintptr_t)class_cunt;

    while (elem < elem_cunt)
    {
        total += mat[elem];
        elem++;
    }

    while (idx < class_cunt)
    {
        hit_cunt += mat[((uintptr_t)idx * (uintptr_t)class_cunt) + (uintptr_t)idx];
        idx++;
    }

    if (LDG_UNLIKELY(total == 0)) { return LDG_ERR_MATH_STAT_INVALID_DIST; }

    *out = (double)hit_cunt / (double)total;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_roc_area_get(const double *score, const uint8_t *label, uint32_t cunt, uint32_t *order, uint32_t order_cunt, double *out)
{
    double pos = EVALUATION_NONE;
    double neg = EVALUATION_NONE;
    uint32_t pos_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!score || !label || !order || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = EVALUATION_NONE;

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(order_cunt < cunt)) { return LDG_ERR_OVERFLOW; }

    ret = evaluation_input_ck(score, label, cunt, &pos_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(pos_cunt == 0 || pos_cunt == cunt)) { return LDG_ERR_MATH_STAT_INVALID_DIST; }

    evaluation_order_sort(order, score, cunt);

    pos = (double)pos_cunt;
    neg = (double)(cunt - pos_cunt);
    *out = (evaluation_pos_rank_cum(score, label, order, cunt) - ((pos * (pos + EVALUATION_RANK_BASE)) / EVALUATION_RANK_FLOOR_DIV)) / (pos * neg);

    return LDG_ERR_AOK;
}
