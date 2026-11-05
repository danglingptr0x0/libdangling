#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>

#define ASSOCIATION_NONE 0.0
#define ASSOCIATION_SHARE_TOTAL 1.0
#define ASSOCIATION_RANK_MEAN_WEIGHT 0.5
#define ASSOCIATION_HEAP_ARITY 2
#define ASSOCIATION_HEAP_ROOT 0
#define ASSOCIATION_ADJACENT_OFF 1
#define ASSOCIATION_PAIR_MIN 2
#define ASSOCIATION_RATER_CUNT 2

static uint32_t association_elem_ck(const double *elem, uint32_t cunt)
{
    uint32_t idx = 0;

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(elem[idx]) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        idx++;
    }

    return LDG_ERR_AOK;
}

static uint32_t association_weight_total_get(const double *weight, uintptr_t cunt, double *out)
{
    double total = ASSOCIATION_NONE;
    uintptr_t idx = 0;

    *out = ASSOCIATION_NONE;

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(weight[idx]) != LDG_TRUTH_TRUE || weight[idx] < ASSOCIATION_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        total += weight[idx];
        idx++;
    }

    if (LDG_UNLIKELY(ldg_math_finite_is(total) != LDG_TRUTH_TRUE || total <= ASSOCIATION_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = total;

    return LDG_ERR_AOK;
}

static double association_surprise_get(double weight)
{
    if (weight <= ASSOCIATION_NONE) { return ASSOCIATION_NONE; }

    return weight * ldg_math_log_get(weight);
}

static void association_order_sift(uint32_t *order, const double *key, uint32_t start, uint32_t end)
{
    uint32_t root = start;
    uint32_t child = 0;
    uint32_t pick = 0;
    uint32_t held = 0;

    while (((root * ASSOCIATION_HEAP_ARITY) + ASSOCIATION_ADJACENT_OFF) <= end)
    {
        child = (root * ASSOCIATION_HEAP_ARITY) + ASSOCIATION_ADJACENT_OFF;
        pick = root;

        if (key[order[pick]] < key[order[child]]) { pick = child; }

        if (child < end && key[order[pick]] < key[order[child + ASSOCIATION_ADJACENT_OFF]]) { pick = child + ASSOCIATION_ADJACENT_OFF; }

        if (pick == root) { return; }

        held = order[root];
        order[root] = order[pick];
        order[pick] = held;
        root = pick;
    }
}

static void association_order_sort(uint32_t *order, const double *key, uint32_t cunt)
{
    uint32_t start = cunt / ASSOCIATION_HEAP_ARITY;
    uint32_t end = cunt - ASSOCIATION_ADJACENT_OFF;
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
        association_order_sift(order, key, start, end);
    }

    while (end > 0)
    {
        held = order[0];
        order[0] = order[end];
        order[end] = held;
        end--;
        association_order_sift(order, key, ASSOCIATION_HEAP_ROOT, end);
    }
}

static void association_rank_fill(const double *key, uint32_t cunt, uint32_t *order, double *rank)
{
    double tied = ASSOCIATION_NONE;
    uint32_t run_start = 0;
    uint32_t run_end = 0;
    uint32_t idx = 0;

    association_order_sort(order, key, cunt);

    while (run_start < cunt)
    {
        run_end = run_start + ASSOCIATION_ADJACENT_OFF;

        while (run_end < cunt && key[order[run_end]] <= key[order[run_start]]) { run_end++; }

        tied = ASSOCIATION_RANK_MEAN_WEIGHT * ((double)run_start + (double)(run_end - ASSOCIATION_ADJACENT_OFF));
        idx = run_start;

        while (idx < run_end)
        {
            rank[order[idx]] = tied;
            idx++;
        }

        run_start = run_end;
    }
}

static double association_marginal_surprise_get(const double *joint, uint32_t outer_cunt, uint32_t inner_cunt, uintptr_t outer_stride, uintptr_t inner_stride)
{
    const double *outer_ptr = joint;
    const double *inner_ptr = 0x0;
    double total = ASSOCIATION_NONE;
    double cum = ASSOCIATION_NONE;
    uint32_t outer = 0;
    uint32_t inner = 0;

    while (outer < outer_cunt)
    {
        inner_ptr = outer_ptr;
        cum = ASSOCIATION_NONE;
        inner = 0;

        while (inner < inner_cunt)
        {
            cum += *inner_ptr;
            inner_ptr += inner_stride;
            inner++;
        }

        total += association_surprise_get(cum);
        outer_ptr += outer_stride;
        outer++;
    }

    return total;
}

uint32_t ldg_math_stat_spearman_get(const double *arr_a, const double *arr_b, uint32_t cunt, double *buff, uintptr_t buff_cunt, uint32_t *order, uint32_t order_cunt, double *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!arr_a || !arr_b || !buff || !order || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ASSOCIATION_NONE;

    if (LDG_UNLIKELY(cunt < ASSOCIATION_PAIR_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(buff_cunt < ((uintptr_t)cunt * (uintptr_t)ASSOCIATION_RATER_CUNT) || order_cunt < cunt)) { return LDG_ERR_OVERFLOW; }

    ret = association_elem_ck(arr_a, cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = association_elem_ck(arr_b, cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    association_rank_fill(arr_a, cunt, order, buff);
    association_rank_fill(arr_b, cunt, order, buff + cunt);

    return ldg_math_stat_corr_get(buff, buff + cunt, (uint64_t)cunt, out);
}

uint32_t ldg_math_stat_kappa_get(const uint32_t *label_a, const uint32_t *label_b, uint32_t cunt, uint32_t class_cunt, uint32_t *buff, uint32_t buff_cunt, double *out)
{
    const uint32_t *tally_b = 0x0;
    double total = ASSOCIATION_NONE;
    double observed = ASSOCIATION_NONE;
    double expected = ASSOCIATION_NONE;
    uint32_t agree_cunt = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!label_a || !label_b || !buff || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ASSOCIATION_NONE;

    if (LDG_UNLIKELY(cunt == 0 || class_cunt == 0 || class_cunt > (UINT32_MAX / ASSOCIATION_RATER_CUNT))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(buff_cunt < class_cunt * ASSOCIATION_RATER_CUNT)) { return LDG_ERR_OVERFLOW; }

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(label_a[idx] >= class_cunt || label_b[idx] >= class_cunt)) { return LDG_ERR_BOUNDS; }

        idx++;
    }

    idx = 0;

    while (idx < class_cunt * ASSOCIATION_RATER_CUNT)
    {
        buff[idx] = 0;
        idx++;
    }

    tally_b = buff + class_cunt;
    idx = 0;

    while (idx < cunt)
    {
        buff[label_a[idx]]++;
        buff[class_cunt + label_b[idx]]++;

        if (label_a[idx] == label_b[idx]) { agree_cunt++; }

        idx++;
    }

    total = (double)cunt;
    observed = (double)agree_cunt / total;
    idx = 0;

    while (idx < class_cunt)
    {
        expected += ((double)buff[idx] / total) * ((double)tally_b[idx] / total);
        idx++;
    }

    if (LDG_UNLIKELY(expected >= ASSOCIATION_SHARE_TOTAL)) { return LDG_ERR_MATH_STAT_INVALID_DIST; }

    *out = (observed - expected) / (ASSOCIATION_SHARE_TOTAL - expected);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_entropy_get(const double *weight, uint32_t cunt, double *out)
{
    double total = ASSOCIATION_NONE;
    double cum = ASSOCIATION_NONE;
    double entropy = ASSOCIATION_NONE;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!weight || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ASSOCIATION_NONE;

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = association_weight_total_get(weight, (uintptr_t)cunt, &total);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (idx < cunt)
    {
        cum += association_surprise_get(weight[idx]);
        idx++;
    }

    entropy = ldg_math_log_get(total) - (cum / total);
    *out = (entropy > ASSOCIATION_NONE) ? entropy : ASSOCIATION_NONE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_divergence_get(const double *lhs, const double *rhs, uint32_t cunt, double *out)
{
    double lhs_total = ASSOCIATION_NONE;
    double rhs_total = ASSOCIATION_NONE;
    double cum = ASSOCIATION_NONE;
    double divergence = ASSOCIATION_NONE;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!lhs || !rhs || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ASSOCIATION_NONE;

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = association_weight_total_get(lhs, (uintptr_t)cunt, &lhs_total);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = association_weight_total_get(rhs, (uintptr_t)cunt, &rhs_total);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (idx < cunt)
    {
        if (lhs[idx] > ASSOCIATION_NONE && rhs[idx] <= ASSOCIATION_NONE)
        {
            *out = LDG_MATH_INFINITY;
            return LDG_ERR_AOK;
        }

        if (lhs[idx] > ASSOCIATION_NONE) { cum += lhs[idx] * (ldg_math_log_get(lhs[idx]) - ldg_math_log_get(rhs[idx])); }

        idx++;
    }

    divergence = (cum / lhs_total) - ldg_math_log_get(lhs_total) + ldg_math_log_get(rhs_total);
    *out = (divergence > ASSOCIATION_NONE) ? divergence : ASSOCIATION_NONE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_mutual_info_get(const double *joint, uint32_t row_cunt, uint32_t col_cunt, double *out)
{
    double total = ASSOCIATION_NONE;
    double cell = ASSOCIATION_NONE;
    double info = ASSOCIATION_NONE;
    uintptr_t elem_cunt = 0;
    uintptr_t elem = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!joint || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = ASSOCIATION_NONE;

    if (LDG_UNLIKELY(row_cunt == 0 || col_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    elem_cunt = (uintptr_t)row_cunt * (uintptr_t)col_cunt;

    ret = association_weight_total_get(joint, elem_cunt, &total);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (elem < elem_cunt)
    {
        cell += association_surprise_get(joint[elem]);
        elem++;
    }

    cell -= association_marginal_surprise_get(joint, row_cunt, col_cunt, (uintptr_t)col_cunt, (uintptr_t)ASSOCIATION_ADJACENT_OFF);
    cell -= association_marginal_surprise_get(joint, col_cunt, row_cunt, (uintptr_t)ASSOCIATION_ADJACENT_OFF, (uintptr_t)col_cunt);
    info = ldg_math_log_get(total) + (cell / total);
    *out = (info > ASSOCIATION_NONE) ? info : ASSOCIATION_NONE;

    return LDG_ERR_AOK;
}
