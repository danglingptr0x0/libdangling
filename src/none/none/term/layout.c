#include <stdint.h>

#include <dangling/term/layout.h>
#include <dangling/math/rational.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_TERM_LAYOUT_SHARE_NONE 0

static uint32_t layout_size_ck(const ldg_term_layout_size_t *size, uint32_t size_cunt, uint64_t *fixed_total, uint32_t *share_last)
{
    uint32_t idx = 0;

    *fixed_total = 0;
    *share_last = UINT32_MAX;

    for (idx = 0; idx < size_cunt; idx++)
    {
        if (size[idx].kind == LDG_TERM_LAYOUT_SIZE_FIXED)
        {
            *fixed_total += size[idx].cell_cunt;
            continue;
        }

        if (LDG_UNLIKELY(size[idx].kind != LDG_TERM_LAYOUT_SIZE_SHARE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(size[idx].share.num < 0 || size[idx].share.den == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        *share_last = idx;
    }

    return LDG_ERR_AOK;
}

static uint32_t layout_share_total_get(const ldg_term_layout_size_t *size, uint32_t size_cunt, ldg_math_rational_t *out)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    ret = ldg_math_rational_set(LDG_TERM_LAYOUT_SHARE_NONE, LDG_MATH_RATIONAL_WHOLE_DEN, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < size_cunt; idx++)
    {
        if (size[idx].kind != LDG_TERM_LAYOUT_SIZE_SHARE) { continue; }

        ret = ldg_math_rational_add(out, &size[idx].share, out);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t layout_share_cell_get(const ldg_math_rational_t *share, const ldg_math_rational_t *share_total, uint64_t rest, uint64_t *out)
{
    ldg_math_rational_t portion = LDG_STRUCT_ZERO_INIT;
    int64_t cell_cunt = 0;
    uint32_t ret = 0;

    *out = 0;

    ret = ldg_math_rational_div(share, share_total, &portion);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_rational_uint64_mul(&portion, rest, &portion);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_rational_floor_get(&portion, &cell_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cell_cunt < 0)) { return LDG_ERR_MATH_VAL_INVALID; }

    *out = (uint64_t)cell_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_layout_split(const ldg_term_rect_t *rect, uint32_t axis, const ldg_term_layout_size_t *size, uint32_t size_cunt, ldg_term_rect_t *out)
{
    ldg_math_rational_t share_total = LDG_STRUCT_ZERO_INIT;
    ldg_term_rect_t base = LDG_STRUCT_ZERO_INIT;
    uint64_t span = 0;
    uint64_t fixed_total = 0;
    uint64_t rest = 0;
    uint64_t assigned = 0;
    uint64_t cell_cunt = 0;
    uint64_t pos = 0;
    uint32_t share_last = UINT32_MAX;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!rect || !size || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(size_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(axis != LDG_TERM_LAYOUT_AXIS_COL && axis != LDG_TERM_LAYOUT_AXIS_ROW)) { return LDG_ERR_FUNC_ARG_INVALID; }

    base = *rect;
    span = (axis == LDG_TERM_LAYOUT_AXIS_COL) ? base.col_cunt : base.row_cunt;

    ret = layout_size_ck(size, size_cunt, &fixed_total, &share_last);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(fixed_total > span)) { return LDG_ERR_BOUNDS; }

    rest = span - fixed_total;

    ret = layout_share_total_get(size, size_cunt, &share_total);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(share_last != UINT32_MAX && share_total.num == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < size_cunt; idx++)
    {
        if (size[idx].kind != LDG_TERM_LAYOUT_SIZE_SHARE) { continue; }

        ret = layout_share_cell_get(&size[idx].share, &share_total, rest, &cell_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        assigned += cell_cunt;
    }

    if (LDG_UNLIKELY(assigned > rest)) { return LDG_ERR_MATH_VAL_INVALID; }

    pos = (axis == LDG_TERM_LAYOUT_AXIS_COL) ? base.col : base.row;

    for (idx = 0; idx < size_cunt; idx++)
    {
        cell_cunt = size[idx].cell_cunt;

        if (size[idx].kind == LDG_TERM_LAYOUT_SIZE_SHARE)
        {
            ret = layout_share_cell_get(&size[idx].share, &share_total, rest, &cell_cunt);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (idx == share_last) { cell_cunt += rest - assigned; }
        }

        out[idx] = base;

        if (axis == LDG_TERM_LAYOUT_AXIS_COL)
        {
            out[idx].col = (uint32_t)pos;
            out[idx].col_cunt = (uint32_t)cell_cunt;
        }
        else
        {
            out[idx].row = (uint32_t)pos;
            out[idx].row_cunt = (uint32_t)cell_cunt;
        }

        pos += cell_cunt;
    }

    return LDG_ERR_AOK;
}
