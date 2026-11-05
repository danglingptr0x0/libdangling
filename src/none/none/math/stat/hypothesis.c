#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>

#define HYPOTHESIS_NONE 0.0
#define HYPOTHESIS_CERTAIN 1.0
#define HYPOTHESIS_PAIR_MIN 2
#define HYPOTHESIS_ADJACENT_OFF 1
#define HYPOTHESIS_HEAP_ARITY 2
#define HYPOTHESIS_HEAP_ROOT 0
#define HYPOTHESIS_BUFF_PER_PAIR 2
#define HYPOTHESIS_TAIL_SPLIT 2.5
#define HYPOTHESIS_TAIL_DEPTH 60
#define HYPOTHESIS_TAIL_NUMER_STEP 0.5
#define HYPOTHESIS_SERIES_TERM_MAX 200
#define HYPOTHESIS_SERIES_TOL 1e-17
#define HYPOTHESIS_SERIES_ODD_STEP 2.0
#define HYPOTHESIS_SERIES_SCALE 2.0
#define HYPOTHESIS_PARITY_STEP 2
#define HYPOTHESIS_PARITY_BIT 0x1U
#define HYPOTHESIS_ODD_FIRST 3
#define HYPOTHESIS_RANK_BASE 1.0
#define HYPOTHESIS_RANK_MEAN_WEIGHT 0.5
#define HYPOTHESIS_RANK_MEAN_DIV 4.0
#define HYPOTHESIS_RANK_VAR_DIV 24.0
#define HYPOTHESIS_RANK_VAR_SLOPE 2.0
#define HYPOTHESIS_TIE_VAR_DIV 48.0
#define HYPOTHESIS_CONTINUITY 0.5
#define HYPOTHESIS_SIGN_BITS 64
#define HYPOTHESIS_SIGN_BIT 0x1ULL
#define HYPOTHESIS_UNIT_SHIFT 11
#define HYPOTHESIS_UNIT_SCALE 9007199254740992.0
#define HYPOTHESIS_SIDE_CUNT 2.0

static uint32_t hypothesis_pair_ck(const double *arr_a, const double *arr_b, uint32_t cunt)
{
    uint32_t idx = 0;

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(arr_a[idx]) != LDG_TRUTH_TRUE || ldg_math_finite_is(arr_b[idx]) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        idx++;
    }

    return LDG_ERR_AOK;
}

static double hypothesis_share_clamp(double share)
{
    if (share < HYPOTHESIS_NONE) { return HYPOTHESIS_NONE; }

    if (share > HYPOTHESIS_CERTAIN) { return HYPOTHESIS_CERTAIN; }

    return share;
}

static double hypothesis_normal_series_get(double dev)
{
    double square = dev * dev;
    double term = dev;
    double cum = dev;
    double add = dev;
    double order = HYPOTHESIS_CERTAIN;
    uint32_t idx = 0;

    while (idx < HYPOTHESIS_SERIES_TERM_MAX && ldg_math_abs_get(add) > (HYPOTHESIS_SERIES_TOL * ldg_math_abs_get(cum)))
    {
        term *= -square / order;
        add = term / ((HYPOTHESIS_SERIES_ODD_STEP * order) + HYPOTHESIS_CERTAIN);
        cum += add;
        order += HYPOTHESIS_CERTAIN;
        idx++;
    }

    return HYPOTHESIS_CERTAIN - ((HYPOTHESIS_SERIES_SCALE / ldg_math_sqrt_get(LDG_MATH_PI)) * cum);
}

static double hypothesis_normal_fraction_get(double dev)
{
    double level = dev;
    uint32_t depth = HYPOTHESIS_TAIL_DEPTH;

    while (depth > 0)
    {
        level = dev + (((double)depth * HYPOTHESIS_TAIL_NUMER_STEP) / level);
        depth--;
    }

    return ldg_math_exp_get(-(dev * dev)) / (ldg_math_sqrt_get(LDG_MATH_PI) * level);
}

static double hypothesis_normal_tail_get(double deviate)
{
    double dev = deviate * LDG_MATH_SQRT_HALF;

    if (dev < HYPOTHESIS_TAIL_SPLIT) { return hypothesis_share_clamp(hypothesis_normal_series_get(dev)); }

    return hypothesis_share_clamp(hypothesis_normal_fraction_get(dev));
}

static double hypothesis_student_cum_get(double cos_square, double first, uint32_t start, uint32_t freedom)
{
    double term = first;
    double cum = first;
    uint32_t idx = start;

    while ((idx + HYPOTHESIS_PARITY_STEP) <= freedom)
    {
        term *= ((double)(idx - HYPOTHESIS_ADJACENT_OFF) / (double)idx) * cos_square;
        cum += term;
        idx += HYPOTHESIS_PARITY_STEP;
    }

    return cum;
}

static double hypothesis_student_tail_get(double stat_abs, uint32_t freedom)
{
    double span = (double)freedom + (stat_abs * stat_abs);
    double cos_square = (double)freedom / span;
    double sine = stat_abs / ldg_math_sqrt_get(span);
    double cosine = ldg_math_sqrt_get(cos_square);
    double angle = ldg_math_arctan_get(stat_abs / ldg_math_sqrt_get((double)freedom));
    double inside = HYPOTHESIS_NONE;

    if ((freedom & HYPOTHESIS_PARITY_BIT) == 0) { return hypothesis_share_clamp(HYPOTHESIS_CERTAIN - (sine * hypothesis_student_cum_get(cos_square, HYPOTHESIS_CERTAIN, HYPOTHESIS_PARITY_STEP, freedom))); }

    if (freedom >= HYPOTHESIS_ODD_FIRST) { inside = sine * hypothesis_student_cum_get(cos_square, cosine, HYPOTHESIS_ODD_FIRST, freedom); }

    return hypothesis_share_clamp(HYPOTHESIS_CERTAIN - (LDG_MATH_TWO_OVER_PI * (angle + inside)));
}

static void hypothesis_order_sift(uint32_t *order, const double *key, uint32_t start, uint32_t end)
{
    uint32_t root = start;
    uint32_t child = 0;
    uint32_t pick = 0;
    uint32_t held = 0;

    while (((root * HYPOTHESIS_HEAP_ARITY) + HYPOTHESIS_ADJACENT_OFF) <= end)
    {
        child = (root * HYPOTHESIS_HEAP_ARITY) + HYPOTHESIS_ADJACENT_OFF;
        pick = root;

        if (key[order[pick]] < key[order[child]]) { pick = child; }

        if (child < end && key[order[pick]] < key[order[child + HYPOTHESIS_ADJACENT_OFF]]) { pick = child + HYPOTHESIS_ADJACENT_OFF; }

        if (pick == root) { return; }

        held = order[root];
        order[root] = order[pick];
        order[pick] = held;
        root = pick;
    }
}

static void hypothesis_order_sort(uint32_t *order, const double *key, uint32_t cunt)
{
    uint32_t start = cunt / HYPOTHESIS_HEAP_ARITY;
    uint32_t end = cunt - HYPOTHESIS_ADJACENT_OFF;
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
        hypothesis_order_sift(order, key, start, end);
    }

    while (end > 0)
    {
        held = order[HYPOTHESIS_HEAP_ROOT];
        order[HYPOTHESIS_HEAP_ROOT] = order[end];
        order[end] = held;
        end--;
        hypothesis_order_sift(order, key, HYPOTHESIS_HEAP_ROOT, end);
    }
}

static void hypothesis_val_sift(double *val, uint32_t start, uint32_t end)
{
    double held = HYPOTHESIS_NONE;
    uint32_t root = start;
    uint32_t child = 0;
    uint32_t pick = 0;

    while (((root * HYPOTHESIS_HEAP_ARITY) + HYPOTHESIS_ADJACENT_OFF) <= end)
    {
        child = (root * HYPOTHESIS_HEAP_ARITY) + HYPOTHESIS_ADJACENT_OFF;
        pick = root;

        if (val[pick] < val[child]) { pick = child; }

        if (child < end && val[pick] < val[child + HYPOTHESIS_ADJACENT_OFF]) { pick = child + HYPOTHESIS_ADJACENT_OFF; }

        if (pick == root) { return; }

        held = val[root];
        val[root] = val[pick];
        val[pick] = held;
        root = pick;
    }
}

static void hypothesis_val_sort(double *val, uint32_t cunt)
{
    double held = HYPOTHESIS_NONE;
    uint32_t start = cunt / HYPOTHESIS_HEAP_ARITY;
    uint32_t end = cunt - HYPOTHESIS_ADJACENT_OFF;

    while (start > 0)
    {
        start--;
        hypothesis_val_sift(val, start, end);
    }

    while (end > 0)
    {
        held = val[HYPOTHESIS_HEAP_ROOT];
        val[HYPOTHESIS_HEAP_ROOT] = val[end];
        val[end] = held;
        end--;
        hypothesis_val_sift(val, HYPOTHESIS_HEAP_ROOT, end);
    }
}

static double hypothesis_quantile_get(const double *sorted, uint32_t cunt, double level)
{
    double spot = level * (double)(cunt - HYPOTHESIS_ADJACENT_OFF);
    double base = ldg_math_floor_get(spot);
    uint32_t lower = (uint32_t)base;
    uint32_t upper = ((lower + HYPOTHESIS_ADJACENT_OFF) < cunt) ? (lower + HYPOTHESIS_ADJACENT_OFF) : lower;

    return sorted[lower] + ((spot - base) * (sorted[upper] - sorted[lower]));
}

static uint32_t hypothesis_diff_pack(const double *arr_a, const double *arr_b, uint32_t cunt, double *diff, double *size)
{
    double gap = HYPOTHESIS_NONE;
    uint32_t kept = 0;
    uint32_t idx = 0;

    while (idx < cunt)
    {
        gap = arr_a[idx] - arr_b[idx];

        if (gap > HYPOTHESIS_NONE || gap < HYPOTHESIS_NONE)
        {
            diff[kept] = gap;
            size[kept] = ldg_math_abs_get(gap);
            kept++;
        }

        idx++;
    }

    return kept;
}

static double hypothesis_rank_cum(const double *diff, const double *size, const uint32_t *order, uint32_t cunt, double *out_tie)
{
    double tied = HYPOTHESIS_NONE;
    double span = HYPOTHESIS_NONE;
    double pos_cum = HYPOTHESIS_NONE;
    uint32_t run_start = 0;
    uint32_t run_end = 0;
    uint32_t idx = 0;

    *out_tie = HYPOTHESIS_NONE;

    while (run_start < cunt)
    {
        run_end = run_start + HYPOTHESIS_ADJACENT_OFF;

        while (run_end < cunt && size[order[run_end]] <= size[order[run_start]]) { run_end++; }

        span = (double)(run_end - run_start);
        tied = HYPOTHESIS_RANK_BASE + (HYPOTHESIS_RANK_MEAN_WEIGHT * ((double)run_start + (double)(run_end - HYPOTHESIS_ADJACENT_OFF)));
        *out_tie += (span * span * span) - span;
        idx = run_start;

        while (idx < run_end)
        {
            if (diff[order[idx]] > HYPOTHESIS_NONE) { pos_cum += tied; }

            idx++;
        }

        run_start = run_end;
    }

    return pos_cum;
}

static double hypothesis_flip_cum(const double *arr_a, const double *arr_b, uint32_t cunt, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx)
{
    double gap = HYPOTHESIS_NONE;
    double cum = HYPOTHESIS_NONE;
    uint64_t bits = 0;
    uint32_t held = 0;
    uint32_t idx = 0;

    while (idx < cunt)
    {
        if (held == 0)
        {
            bits = rng_cb(rng_ctx);
            held = HYPOTHESIS_SIGN_BITS;
        }

        gap = arr_a[idx] - arr_b[idx];
        cum += ((bits & HYPOTHESIS_SIGN_BIT) != 0) ? gap : -gap;
        bits >>= HYPOTHESIS_ADJACENT_OFF;
        held--;
        idx++;
    }

    return cum;
}

uint32_t ldg_math_stat_paired_t_test(const double *arr_a, const double *arr_b, uint32_t cunt, ldg_math_stat_test_t *out)
{
    double mean = HYPOTHESIS_NONE;
    double var = HYPOTHESIS_NONE;
    double gap = HYPOTHESIS_NONE;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!arr_a || !arr_b || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->stat = HYPOTHESIS_NONE;
    out->p_val = HYPOTHESIS_CERTAIN;
    out->shift = HYPOTHESIS_NONE;
    out->cunt = 0;

    if (LDG_UNLIKELY(cunt < HYPOTHESIS_PAIR_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = hypothesis_pair_ck(arr_a, arr_b, cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (idx < cunt)
    {
        mean += arr_a[idx] - arr_b[idx];
        idx++;
    }

    mean /= (double)cunt;
    idx = 0;

    while (idx < cunt)
    {
        gap = (arr_a[idx] - arr_b[idx]) - mean;
        var += gap * gap;
        idx++;
    }

    var /= (double)(cunt - HYPOTHESIS_ADJACENT_OFF);
    out->shift = mean;
    out->cunt = cunt;

    if (var <= HYPOTHESIS_NONE) { return (ldg_math_abs_get(mean) <= HYPOTHESIS_NONE) ? LDG_ERR_AOK : LDG_ERR_MATH_STAT_INVALID_DIST; }

    out->stat = mean / ldg_math_sqrt_get(var / (double)cunt);
    out->p_val = hypothesis_student_tail_get(ldg_math_abs_get(out->stat), cunt - HYPOTHESIS_ADJACENT_OFF);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_signed_rank_test(const double *arr_a, const double *arr_b, uint32_t cunt, double *buff, uintptr_t buff_cunt, uint32_t *order, uint32_t order_cunt, ldg_math_stat_test_t *out)
{
    double pairs = HYPOTHESIS_NONE;
    double center = HYPOTHESIS_NONE;
    double var = HYPOTHESIS_NONE;
    double tie = HYPOTHESIS_NONE;
    double off = HYPOTHESIS_NONE;
    uint32_t kept = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!arr_a || !arr_b || !buff || !order || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->stat = HYPOTHESIS_NONE;
    out->p_val = HYPOTHESIS_CERTAIN;
    out->shift = HYPOTHESIS_NONE;
    out->cunt = 0;

    if (LDG_UNLIKELY(cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(buff_cunt < ((uintptr_t)cunt * (uintptr_t)HYPOTHESIS_BUFF_PER_PAIR) || order_cunt < cunt)) { return LDG_ERR_OVERFLOW; }

    ret = hypothesis_pair_ck(arr_a, arr_b, cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    kept = hypothesis_diff_pack(arr_a, arr_b, cunt, buff, buff + cunt);
    out->cunt = kept;

    if (kept == 0) { return LDG_ERR_AOK; }

    hypothesis_order_sort(order, buff + cunt, kept);
    out->stat = hypothesis_rank_cum(buff, buff + cunt, order, kept, &tie);

    pairs = (double)kept;
    center = (pairs * (pairs + HYPOTHESIS_CERTAIN)) / HYPOTHESIS_RANK_MEAN_DIV;
    var = ((pairs * (pairs + HYPOTHESIS_CERTAIN) * ((HYPOTHESIS_RANK_VAR_SLOPE * pairs) + HYPOTHESIS_CERTAIN)) / HYPOTHESIS_RANK_VAR_DIV) - (tie / HYPOTHESIS_TIE_VAR_DIV);

    if (var <= HYPOTHESIS_NONE) { return LDG_ERR_AOK; }

    off = out->stat - center;

    if (off > HYPOTHESIS_CONTINUITY) { off -= HYPOTHESIS_CONTINUITY; }
    else if (off + HYPOTHESIS_CONTINUITY < HYPOTHESIS_NONE) { off += HYPOTHESIS_CONTINUITY; }
    else { off = HYPOTHESIS_NONE; }

    out->shift = off / ldg_math_sqrt_get(var);
    out->p_val = hypothesis_normal_tail_get(ldg_math_abs_get(out->shift));

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_permutation_test(const double *arr_a, const double *arr_b, uint32_t cunt, uint32_t round_cunt, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, ldg_math_stat_test_t *out)
{
    double seen = HYPOTHESIS_NONE;
    uint32_t extreme_cunt = 0;
    uint32_t round = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!arr_a || !arr_b || !rng_cb || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->stat = HYPOTHESIS_NONE;
    out->p_val = HYPOTHESIS_CERTAIN;
    out->shift = HYPOTHESIS_NONE;
    out->cunt = 0;

    if (LDG_UNLIKELY(cunt == 0 || round_cunt == 0 || round_cunt == UINT32_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = hypothesis_pair_ck(arr_a, arr_b, cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (idx < cunt)
    {
        seen += arr_a[idx] - arr_b[idx];
        idx++;
    }

    while (round < round_cunt)
    {
        if (ldg_math_abs_get(hypothesis_flip_cum(arr_a, arr_b, cunt, rng_cb, rng_ctx)) >= ldg_math_abs_get(seen)) { extreme_cunt++; }

        round++;
    }

    out->stat = seen / (double)cunt;
    out->shift = out->stat;
    out->cunt = cunt;
    out->p_val = ((double)extreme_cunt + HYPOTHESIS_CERTAIN) / ((double)round_cunt + HYPOTHESIS_CERTAIN);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_bootstrap_interval_get(uint32_t cunt, uint32_t round_cunt, double level, ldg_math_stat_estimate_cb_t estimate_cb, void *estimate_ctx, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx, uint32_t *idx_buff, uint32_t idx_cunt, double *rep_buff, uint32_t rep_cunt, double *out_lower, double *out_upper)
{
    double tail = HYPOTHESIS_NONE;
    uint32_t pick = 0;
    uint32_t round = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!estimate_cb || !rng_cb || !idx_buff || !rep_buff || !out_lower || !out_upper)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_lower = HYPOTHESIS_NONE;
    *out_upper = HYPOTHESIS_NONE;

    if (LDG_UNLIKELY(cunt == 0 || round_cunt < HYPOTHESIS_PAIR_MIN || ldg_math_finite_is(level) != LDG_TRUTH_TRUE || level <= HYPOTHESIS_NONE || level >= HYPOTHESIS_CERTAIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(idx_cunt < cunt || rep_cunt < round_cunt)) { return LDG_ERR_OVERFLOW; }

    while (round < round_cunt)
    {
        idx = 0;

        while (idx < cunt)
        {
            pick = (uint32_t)(((double)(rng_cb(rng_ctx) >> HYPOTHESIS_UNIT_SHIFT) / HYPOTHESIS_UNIT_SCALE) * (double)cunt);
            idx_buff[idx] = (pick < cunt) ? pick : (cunt - HYPOTHESIS_ADJACENT_OFF);
            idx++;
        }

        ret = estimate_cb(idx_buff, cunt, estimate_ctx, rep_buff + round);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(ldg_math_finite_is(rep_buff[round]) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        round++;
    }

    hypothesis_val_sort(rep_buff, round_cunt);
    tail = (HYPOTHESIS_CERTAIN - level) / HYPOTHESIS_SIDE_CUNT;
    *out_lower = hypothesis_quantile_get(rep_buff, round_cunt, tail);
    *out_upper = hypothesis_quantile_get(rep_buff, round_cunt, HYPOTHESIS_CERTAIN - tail);

    return LDG_ERR_AOK;
}
