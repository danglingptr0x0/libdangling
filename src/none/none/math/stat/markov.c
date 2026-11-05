#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>

#define MARKOV_NONE 0.0
#define MARKOV_SHARE_TOTAL 1.0
#define MARKOV_STEP_COST 1.0
#define MARKOV_ROW_TOL 1e-9
#define MARKOV_AUG_COL_CUNT 1
#define MARKOV_ADJACENT_OFF 1
#define MARKOV_PAIR_MIN 2

static uintptr_t markov_buff_cunt_get(uint32_t state_cunt)
{
    return (uintptr_t)state_cunt * ((uintptr_t)state_cunt + (uintptr_t)MARKOV_AUG_COL_CUNT);
}

static uint32_t markov_transition_ck(const double *transition, uint32_t state_cunt)
{
    const double *row_ptr = transition;
    double total = MARKOV_NONE;
    uint32_t row = 0;
    uint32_t col = 0;

    while (row < state_cunt)
    {
        total = MARKOV_NONE;
        col = 0;

        while (col < state_cunt)
        {
            if (LDG_UNLIKELY(ldg_math_finite_is(row_ptr[col]) != LDG_TRUTH_TRUE || row_ptr[col] < MARKOV_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

            total += row_ptr[col];
            col++;
        }

        if (LDG_UNLIKELY(ldg_math_abs_get(total - MARKOV_SHARE_TOTAL) > MARKOV_ROW_TOL)) { return LDG_ERR_FUNC_ARG_INVALID; }

        row_ptr += state_cunt;
        row++;
    }

    return LDG_ERR_AOK;
}

static uint32_t markov_pivot_find(const double *aug, uint32_t dim, uint32_t col)
{
    uintptr_t width = (uintptr_t)dim + (uintptr_t)MARKOV_AUG_COL_CUNT;
    double best_abs = MARKOV_NONE;
    double curr_abs = MARKOV_NONE;
    uint32_t best = col;
    uint32_t row = col;

    while (row < dim)
    {
        curr_abs = ldg_math_abs_get(aug[((uintptr_t)row * width) + (uintptr_t)col]);

        if (curr_abs > best_abs)
        {
            best = row;
            best_abs = curr_abs;
        }

        row++;
    }

    return best;
}

static void markov_row_swap(double *aug, uint32_t dim, uint32_t lhs, uint32_t rhs)
{
    uintptr_t width = (uintptr_t)dim + (uintptr_t)MARKOV_AUG_COL_CUNT;
    double *lhs_ptr = aug + ((uintptr_t)lhs * width);
    double *rhs_ptr = aug + ((uintptr_t)rhs * width);
    double held = MARKOV_NONE;
    uintptr_t col = 0;

    while (col < width)
    {
        held = lhs_ptr[col];
        lhs_ptr[col] = rhs_ptr[col];
        rhs_ptr[col] = held;
        col++;
    }
}

static void markov_col_reduce(double *aug, uint32_t dim, uint32_t pivot)
{
    uintptr_t width = (uintptr_t)dim + (uintptr_t)MARKOV_AUG_COL_CUNT;
    const double *pivot_ptr = aug + ((uintptr_t)pivot * width);
    double *row_ptr = 0x0;
    double factor = MARKOV_NONE;
    uintptr_t col = 0;
    uint32_t row = pivot + MARKOV_ADJACENT_OFF;

    while (row < dim)
    {
        row_ptr = aug + ((uintptr_t)row * width);
        factor = row_ptr[pivot] / pivot_ptr[pivot];
        col = (uintptr_t)pivot;

        while (col < width)
        {
            row_ptr[col] -= factor * pivot_ptr[col];
            col++;
        }

        row++;
    }
}

static void markov_back_solve(const double *aug, uint32_t dim, double *out)
{
    uintptr_t width = (uintptr_t)dim + (uintptr_t)MARKOV_AUG_COL_CUNT;
    const double *row_ptr = 0x0;
    double cum = MARKOV_NONE;
    uint32_t remaining = dim;
    uint32_t row = 0;
    uint32_t col = 0;

    while (remaining > 0)
    {
        row = remaining - MARKOV_ADJACENT_OFF;
        row_ptr = aug + ((uintptr_t)row * width);
        cum = row_ptr[dim];
        col = remaining;

        while (col < dim)
        {
            cum -= row_ptr[col] * out[col];
            col++;
        }

        out[row] = cum / row_ptr[row];
        remaining--;
    }
}

static uint32_t markov_solve(double *aug, uint32_t dim, double *out)
{
    uintptr_t width = (uintptr_t)dim + (uintptr_t)MARKOV_AUG_COL_CUNT;
    uint32_t pivot = 0;
    uint32_t col = 0;

    while (col < dim)
    {
        pivot = markov_pivot_find(aug, dim, col);

        if (LDG_UNLIKELY(ldg_math_abs_get(aug[((uintptr_t)pivot * width) + (uintptr_t)col]) <= LDG_MATH_EPSILON)) { return LDG_ERR_MATH_PIVOT; }

        if (pivot != col) { markov_row_swap(aug, dim, pivot, col); }

        markov_col_reduce(aug, dim, col);
        col++;
    }

    markov_back_solve(aug, dim, out);

    return LDG_ERR_AOK;
}

static void markov_stationary_fill(const double *transition, uint32_t state_cunt, double *aug)
{
    uintptr_t width = (uintptr_t)state_cunt + (uintptr_t)MARKOV_AUG_COL_CUNT;
    double *row_ptr = aug;
    uint32_t last = state_cunt - MARKOV_ADJACENT_OFF;
    uint32_t row = 0;
    uint32_t col = 0;

    while (row < state_cunt)
    {
        col = 0;

        while (col < state_cunt)
        {
            row_ptr[col] = (row == last) ? MARKOV_SHARE_TOTAL : (transition[((uintptr_t)col * (uintptr_t)state_cunt) + (uintptr_t)row] - ((row == col) ? MARKOV_SHARE_TOTAL : MARKOV_NONE));
            col++;
        }

        row_ptr[state_cunt] = (row == last) ? MARKOV_SHARE_TOTAL : MARKOV_NONE;
        row_ptr += width;
        row++;
    }
}

static void markov_share_clean(double *share, uint32_t cunt)
{
    double total = MARKOV_NONE;
    uint32_t idx = 0;

    while (idx < cunt)
    {
        if (share[idx] < MARKOV_NONE) { share[idx] = MARKOV_NONE; }

        total += share[idx];
        idx++;
    }

    idx = 0;

    while (total > MARKOV_NONE && idx < cunt)
    {
        share[idx] /= total;
        idx++;
    }
}

static void markov_hitting_fill(const double *transition, uint32_t state_cunt, uint32_t tgt, double *aug)
{
    const double *src_ptr = transition;
    double *row_ptr = aug;
    uint32_t dim = state_cunt - MARKOV_ADJACENT_OFF;
    uint32_t slot = 0;
    uint32_t row = 0;
    uint32_t col = 0;

    while (row < state_cunt)
    {
        slot = 0;
        col = 0;

        while (row != tgt && col < state_cunt)
        {
            if (col != tgt)
            {
                row_ptr[slot] = ((row == col) ? MARKOV_SHARE_TOTAL : MARKOV_NONE) - src_ptr[col];
                slot++;
            }

            col++;
        }

        if (row != tgt)
        {
            row_ptr[dim] = MARKOV_STEP_COST;
            row_ptr += (uintptr_t)dim + (uintptr_t)MARKOV_AUG_COL_CUNT;
        }

        src_ptr += state_cunt;
        row++;
    }
}

uint32_t ldg_math_stat_markov_buff_cunt_get(uint32_t state_cunt, uintptr_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(state_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = markov_buff_cunt_get(state_cunt);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_markov_cunt_cum(const uint32_t *sequence, uint32_t len, uint32_t state_cunt, uint64_t *cunt)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!sequence || !cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(state_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (idx < len)
    {
        if (LDG_UNLIKELY(sequence[idx] >= state_cunt)) { return LDG_ERR_BOUNDS; }

        idx++;
    }

    if (len < MARKOV_PAIR_MIN) { return LDG_ERR_AOK; }

    idx = MARKOV_ADJACENT_OFF;

    while (idx < len)
    {
        cunt[((uintptr_t)sequence[idx - MARKOV_ADJACENT_OFF] * (uintptr_t)state_cunt) + (uintptr_t)sequence[idx]]++;
        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_markov_transition_fill(const uint64_t *cunt, uint32_t state_cunt, double pseudo, double *dst)
{
    const uint64_t *row_ptr = cunt;
    double *dst_ptr = dst;
    double total = MARKOV_NONE;
    uint32_t row = 0;
    uint32_t col = 0;

    if (LDG_UNLIKELY(!cunt || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(state_cunt == 0 || ldg_math_finite_is(pseudo) != LDG_TRUTH_TRUE || pseudo < MARKOV_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (row < state_cunt)
    {
        total = pseudo * (double)state_cunt;
        col = 0;

        while (col < state_cunt)
        {
            total += (double)row_ptr[col];
            col++;
        }

        col = 0;

        while (col < state_cunt)
        {
            dst_ptr[col] = (total > MARKOV_NONE) ? (((double)row_ptr[col] + pseudo) / total) : (MARKOV_SHARE_TOTAL / (double)state_cunt);
            col++;
        }

        row_ptr += state_cunt;
        dst_ptr += state_cunt;
        row++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_markov_stationary_get(const double *transition, uint32_t state_cunt, double *buff, uintptr_t buff_cunt, double *dst)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!transition || !buff || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(state_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(buff_cunt < markov_buff_cunt_get(state_cunt))) { return LDG_ERR_OVERFLOW; }

    ret = markov_transition_ck(transition, state_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    markov_stationary_fill(transition, state_cunt, buff);

    ret = markov_solve(buff, state_cunt, dst);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    markov_share_clean(dst, state_cunt);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_markov_hitting_get(const double *transition, uint32_t state_cunt, uint32_t tgt, double *buff, uintptr_t buff_cunt, double *dst)
{
    uint32_t row = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!transition || !buff || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(state_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(tgt >= state_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(buff_cunt < markov_buff_cunt_get(state_cunt))) { return LDG_ERR_OVERFLOW; }

    ret = markov_transition_ck(transition, state_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    markov_hitting_fill(transition, state_cunt, tgt, buff);

    ret = markov_solve(buff, state_cunt - MARKOV_ADJACENT_OFF, dst);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    row = state_cunt - MARKOV_ADJACENT_OFF;

    while (row > tgt)
    {
        dst[row] = dst[row - MARKOV_ADJACENT_OFF];
        row--;
    }

    dst[tgt] = MARKOV_NONE;

    return LDG_ERR_AOK;
}
