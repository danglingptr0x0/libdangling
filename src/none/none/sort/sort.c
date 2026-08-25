#include <stdint.h>

#include <dangling/sort/sort.h>
#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_SORT_STEP 1
#define LDG_SORT_CHILD_STRIDE 2
#define LDG_SORT_HALF_SHIFT 1
#define LDG_SORT_ROOT_IDX 0

static void sort_item_swap(uint8_t *lhs, uint8_t *rhs, uint64_t item_size)
{
    uint8_t hold = 0;
    uint64_t idx = 0;

    if (lhs == rhs) { return; }

    for (idx = 0; idx < item_size; idx++)
    {
        hold = lhs[idx];
        lhs[idx] = rhs[idx];
        rhs[idx] = hold;
    }
}

static void sort_sift_down(uint8_t *base, uint64_t root, uint64_t cunt, uint64_t item_size, ldg_sort_cmp_cb_t cmp, void *user_ctx)
{
    uint64_t parent = root;
    uint64_t child = 0;
    uint64_t swap_idx = 0;

    while (LDG_TRUTH_TRUE)
    {
        child = (parent * LDG_SORT_CHILD_STRIDE) + LDG_SORT_STEP;

        if (child >= cunt) { return; }

        swap_idx = parent;

        if (cmp(base + (swap_idx * item_size), base + (child * item_size), user_ctx) == LDG_ARITH_ORD_LT) { swap_idx = child; }

        child++;

        if (child < cunt && cmp(base + (swap_idx * item_size), base + (child * item_size), user_ctx) == LDG_ARITH_ORD_LT) { swap_idx = child; }

        if (swap_idx == parent) { return; }

        sort_item_swap(base + (parent * item_size), base + (swap_idx * item_size), item_size);

        parent = swap_idx;
    }
}

uint32_t ldg_sort_run(void *base, uint64_t cunt, uint64_t item_size, ldg_sort_cmp_cb_t cmp, void *user_ctx)
{
    uint8_t *raw = 0x0;
    uint64_t idx = 0;
    uint64_t tail = 0;

    if (LDG_UNLIKELY(!base || !cmp)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(item_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (cunt < LDG_SORT_CHILD_STRIDE) { return LDG_ERR_AOK; }

    raw = (uint8_t *)base;

    idx = cunt >> LDG_SORT_HALF_SHIFT;
    while (idx > 0)
    {
        idx--;
        sort_sift_down(raw, idx, cunt, item_size, cmp, user_ctx);
    }

    tail = cunt;
    while (tail > LDG_SORT_STEP)
    {
        tail--;
        sort_item_swap(raw, raw + (tail * item_size), item_size);
        sort_sift_down(raw, LDG_SORT_ROOT_IDX, tail, item_size, cmp, user_ctx);
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_sort_binary_find(const void *base, uint64_t cunt, uint64_t item_size, const void *key, ldg_sort_cmp_cb_t cmp, void *user_ctx, uint64_t *out_idx)
{
    const uint8_t *raw = 0x0;
    uint64_t low = 0;
    uint64_t high = 0;
    uint64_t mid = 0;
    uint32_t order = 0;

    if (LDG_UNLIKELY(!out_idx)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx = UINT64_MAX;

    if (LDG_UNLIKELY(!base || !key || !cmp)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(item_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (cunt == 0) { return LDG_ERR_NOT_FOUND; }

    raw = (const uint8_t *)base;
    high = cunt;

    while (low < high)
    {
        mid = low + ((high - low) >> LDG_SORT_HALF_SHIFT);

        order = cmp(raw + (mid * item_size), key, user_ctx);

        if (order == LDG_ARITH_ORD_EQ)
        {
            *out_idx = mid;

            return LDG_ERR_AOK;
        }

        if (order == LDG_ARITH_ORD_LT) { low = mid + LDG_SORT_STEP; }
        else { high = mid; }
    }

    return LDG_ERR_NOT_FOUND;
}

uint8_t ldg_sort_ordered_is(const void *base, uint64_t cunt, uint64_t item_size, ldg_sort_cmp_cb_t cmp, void *user_ctx)
{
    const uint8_t *raw = 0x0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!base || !cmp)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(item_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (cunt < LDG_SORT_CHILD_STRIDE) { return LDG_TRUTH_TRUE; }

    raw = (const uint8_t *)base;

    for (idx = LDG_SORT_STEP; idx < cunt; idx++) { if (cmp(raw + ((idx - LDG_SORT_STEP) * item_size), raw + (idx * item_size), user_ctx) == LDG_ARITH_ORD_GT) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}
