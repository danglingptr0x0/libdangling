#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>

#define LINKAGE_ROW_CAP_DEFAULT 1024
#define LINKAGE_DIM_DEFAULT 2
#define LINKAGE_NONE 0.0
#define LINKAGE_WARD_SCALE 2.0
#define LINKAGE_LEAF_SIZE 1
#define LINKAGE_PAIR_CUNT 2
#define LINKAGE_ADJACENT_OFF 1
#define LINKAGE_NODE_PER_ROW 2

static uint32_t linkage_alloc(uint64_t size, void **out)
{
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc(size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

static void linkage_free(void *ptr)
{
    if (ptr) { ldg_mem_dealloc(ptr); }
}

static uint32_t linkage_init_ck(const ldg_math_stat_linkage_t *linkage)
{
    if (LDG_UNLIKELY(!linkage)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(linkage->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

static uint32_t linkage_built_ck(const ldg_math_stat_linkage_t *linkage)
{
    uint32_t ret = linkage_init_ck(linkage);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(linkage->is_built != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_STATE; }

    return LDG_ERR_AOK;
}

static uint32_t linkage_elem_ck(const double *elem, uintptr_t cunt)
{
    uintptr_t idx = 0;

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(ldg_math_finite_is(elem[idx]) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

        idx++;
    }

    return LDG_ERR_AOK;
}

static double* linkage_centroid_ptr(const ldg_math_stat_linkage_t *linkage, uint32_t slot)
{
    return linkage->centroid + ((uintptr_t)slot * (uintptr_t)linkage->conf.dim);
}

static double linkage_ward_square_get(const ldg_math_stat_linkage_t *linkage, uint32_t lhs, uint32_t rhs)
{
    const double *lhs_ptr = linkage_centroid_ptr(linkage, lhs);
    const double *rhs_ptr = linkage_centroid_ptr(linkage, rhs);
    double lhs_size = (double)linkage->member_cunt[lhs];
    double rhs_size = (double)linkage->member_cunt[rhs];
    double cum = LINKAGE_NONE;
    double diff = LINKAGE_NONE;
    uint32_t idx = 0;

    while (idx < linkage->conf.dim)
    {
        diff = lhs_ptr[idx] - rhs_ptr[idx];
        cum += diff * diff;
        idx++;
    }

    return ((LINKAGE_WARD_SCALE * lhs_size * rhs_size) / (lhs_size + rhs_size)) * cum;
}

static uint32_t linkage_active_first_get(const ldg_math_stat_linkage_t *linkage, uint32_t row_cunt)
{
    uint32_t slot = 0;

    while (slot < row_cunt && linkage->member_cunt[slot] == 0) { slot++; }

    return slot;
}

static uint32_t linkage_nearest_get(const ldg_math_stat_linkage_t *linkage, uint32_t row_cunt, uint32_t tail, uint32_t prev, double *out_dist)
{
    double best_dist = LDG_MATH_INFINITY;
    double dist = LINKAGE_NONE;
    uint32_t best = prev;
    uint32_t slot = 0;

    if (prev != LDG_MATH_STAT_LINKAGE_NODE_NONE) { best_dist = linkage_ward_square_get(linkage, tail, prev); }

    while (slot < row_cunt)
    {
        if (slot != tail && linkage->member_cunt[slot] != 0)
        {
            dist = linkage_ward_square_get(linkage, tail, slot);

            if (dist < best_dist || best == LDG_MATH_STAT_LINKAGE_NODE_NONE)
            {
                best = slot;
                best_dist = dist;
            }
        }

        slot++;
    }

    *out_dist = best_dist;

    return best;
}

static void linkage_slot_merge(ldg_math_stat_linkage_t *linkage, uint32_t gone, uint32_t kept)
{
    const double *gone_ptr = linkage_centroid_ptr(linkage, gone);
    double *kept_ptr = linkage_centroid_ptr(linkage, kept);
    double gone_size = (double)linkage->member_cunt[gone];
    double kept_size = (double)linkage->member_cunt[kept];
    uint32_t idx = 0;

    while (idx < linkage->conf.dim)
    {
        kept_ptr[idx] = ((gone_size * gone_ptr[idx]) + (kept_size * kept_ptr[idx])) / (gone_size + kept_size);
        idx++;
    }

    linkage->member_cunt[kept] += linkage->member_cunt[gone];
    linkage->member_cunt[gone] = 0;
}

static void linkage_chain_run(ldg_math_stat_linkage_t *linkage, uint32_t row_cunt)
{
    ldg_math_stat_linkage_merge_t *merge = 0x0;
    double dist = LINKAGE_NONE;
    uint32_t remaining = row_cunt;
    uint32_t chain_len = 0;
    uint32_t tail = 0;
    uint32_t prev = 0;
    uint32_t best = 0;
    uint8_t reciprocal_is = LDG_TRUTH_FALSE;

    while (remaining > LINKAGE_LEAF_SIZE)
    {
        if (chain_len == 0)
        {
            linkage->chain[chain_len] = linkage_active_first_get(linkage, row_cunt);
            chain_len++;
        }

        reciprocal_is = LDG_TRUTH_FALSE;

        while (!reciprocal_is)
        {
            tail = linkage->chain[chain_len - LINKAGE_ADJACENT_OFF];
            prev = (chain_len >= LINKAGE_PAIR_CUNT) ? linkage->chain[chain_len - LINKAGE_PAIR_CUNT] : LDG_MATH_STAT_LINKAGE_NODE_NONE;
            best = linkage_nearest_get(linkage, row_cunt, tail, prev, &dist);

            if (best == prev) { reciprocal_is = LDG_TRUTH_TRUE; }
            else
            {
                linkage->chain[chain_len] = best;
                chain_len++;
            }
        }

        merge = linkage->merge + linkage->merge_cunt;
        merge->height = ldg_math_sqrt_get(dist);
        merge->lhs = tail;
        merge->rhs = prev;
        merge->size = 0;
        linkage->merge_cunt++;

        linkage_slot_merge(linkage, tail, prev);
        chain_len -= LINKAGE_PAIR_CUNT;
        remaining--;
    }
}

static void linkage_merge_sort(ldg_math_stat_linkage_t *linkage)
{
    ldg_math_stat_linkage_merge_t held = LDG_STRUCT_ZERO_INIT;
    uint32_t idx = LINKAGE_ADJACENT_OFF;
    uint32_t slot = 0;

    while (idx < linkage->merge_cunt)
    {
        held = linkage->merge[idx];
        slot = idx;

        while (slot > 0 && linkage->merge[slot - LINKAGE_ADJACENT_OFF].height > held.height)
        {
            linkage->merge[slot] = linkage->merge[slot - LINKAGE_ADJACENT_OFF];
            slot--;
        }

        linkage->merge[slot] = held;
        idx++;
    }
}

static uint32_t linkage_root_get(uint32_t *parent, uint32_t node)
{
    uint32_t root = node;
    uint32_t next = 0;

    while (parent[root] != root) { root = parent[root]; }

    while (parent[node] != root)
    {
        next = parent[node];
        parent[node] = root;
        node = next;
    }

    return root;
}

static void linkage_node_assign(ldg_math_stat_linkage_t *linkage, uint32_t row_cunt)
{
    ldg_math_stat_linkage_merge_t *merge = linkage->merge;
    uint32_t lhs_root = 0;
    uint32_t rhs_root = 0;
    uint32_t lhs_node = 0;
    uint32_t rhs_node = 0;
    uint32_t idx = 0;
    uint32_t row = 0;

    while (row < row_cunt)
    {
        linkage->parent[row] = row;
        linkage->map[row] = row;
        linkage->member_cunt[row] = LINKAGE_LEAF_SIZE;
        row++;
    }

    while (idx < linkage->merge_cunt)
    {
        lhs_root = linkage_root_get(linkage->parent, merge->lhs);
        rhs_root = linkage_root_get(linkage->parent, merge->rhs);
        lhs_node = linkage->map[lhs_root];
        rhs_node = linkage->map[rhs_root];

        merge->lhs = (lhs_node < rhs_node) ? lhs_node : rhs_node;
        merge->rhs = (lhs_node < rhs_node) ? rhs_node : lhs_node;
        merge->size = linkage->member_cunt[lhs_root] + linkage->member_cunt[rhs_root];

        linkage->parent[lhs_root] = rhs_root;
        linkage->map[rhs_root] = row_cunt + idx;
        linkage->member_cunt[rhs_root] = merge->size;

        merge++;
        idx++;
    }
}

static void linkage_label_fill(ldg_math_stat_linkage_t *linkage, uint32_t apply_cunt, uint32_t *label)
{
    const ldg_math_stat_linkage_merge_t *merge = linkage->merge;
    uint32_t node_cunt = linkage->row_cunt + apply_cunt;
    uint32_t label_next = 0;
    uint32_t root = 0;
    uint32_t node = 0;
    uint32_t idx = 0;
    uint32_t row = 0;

    while (node < node_cunt)
    {
        linkage->parent[node] = node;
        linkage->map[node] = LDG_MATH_STAT_LINKAGE_NODE_NONE;
        node++;
    }

    while (idx < apply_cunt)
    {
        linkage->parent[merge->lhs] = linkage->row_cunt + idx;
        linkage->parent[merge->rhs] = linkage->row_cunt + idx;
        merge++;
        idx++;
    }

    while (row < linkage->row_cunt)
    {
        root = linkage_root_get(linkage->parent, row);

        if (linkage->map[root] == LDG_MATH_STAT_LINKAGE_NODE_NONE)
        {
            linkage->map[root] = label_next;
            label_next++;
        }

        label[row] = linkage->map[root];
        row++;
    }
}

uint32_t ldg_math_stat_linkage_conf_default_get(ldg_math_stat_linkage_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->row_cap = LINKAGE_ROW_CAP_DEFAULT;
    out->dim = LINKAGE_DIM_DEFAULT;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_linkage_init(ldg_math_stat_linkage_t *linkage, const ldg_math_stat_linkage_conf_t *conf)
{
    void *ptr = 0x0;
    uint64_t row_cap = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!linkage || !conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->row_cap == 0 || conf->row_cap > LDG_MATH_STAT_LINKAGE_ROW_MAX || conf->dim == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_zero(linkage, sizeof(*linkage));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    linkage->conf = *conf;
    row_cap = (uint64_t)conf->row_cap;

    ret = linkage_alloc(row_cap * (uint64_t)sizeof(ldg_math_stat_linkage_merge_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    linkage->merge = (ldg_math_stat_linkage_merge_t *)ptr;

    ret = linkage_alloc(row_cap * (uint64_t)conf->dim * (uint64_t)sizeof(double), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        linkage_free(linkage->merge);
        return ret;
    }

    linkage->centroid = (double *)ptr;

    ret = linkage_alloc(row_cap * (uint64_t)sizeof(uint32_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        linkage_free(linkage->centroid);
        linkage_free(linkage->merge);
        return ret;
    }

    linkage->member_cunt = (uint32_t *)ptr;

    ret = linkage_alloc(row_cap * (uint64_t)sizeof(uint32_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        linkage_free(linkage->member_cunt);
        linkage_free(linkage->centroid);
        linkage_free(linkage->merge);
        return ret;
    }

    linkage->chain = (uint32_t *)ptr;

    ret = linkage_alloc(row_cap * (uint64_t)LINKAGE_NODE_PER_ROW * (uint64_t)sizeof(uint32_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        linkage_free(linkage->chain);
        linkage_free(linkage->member_cunt);
        linkage_free(linkage->centroid);
        linkage_free(linkage->merge);
        return ret;
    }

    linkage->parent = (uint32_t *)ptr;

    ret = linkage_alloc(row_cap * (uint64_t)LINKAGE_NODE_PER_ROW * (uint64_t)sizeof(uint32_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        linkage_free(linkage->parent);
        linkage_free(linkage->chain);
        linkage_free(linkage->member_cunt);
        linkage_free(linkage->centroid);
        linkage_free(linkage->merge);
        return ret;
    }

    linkage->map = (uint32_t *)ptr;

    linkage->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_linkage_shutdown(ldg_math_stat_linkage_t *linkage)
{
    uint32_t ret = linkage_init_ck(linkage);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    linkage_free(linkage->map);
    linkage_free(linkage->parent);
    linkage_free(linkage->chain);
    linkage_free(linkage->member_cunt);
    linkage_free(linkage->centroid);
    linkage_free(linkage->merge);

    return ldg_mem_zero(linkage, sizeof(*linkage));
}

uint32_t ldg_math_stat_linkage_rst(ldg_math_stat_linkage_t *linkage)
{
    uint32_t ret = linkage_init_ck(linkage);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    linkage->row_cunt = 0;
    linkage->merge_cunt = 0;
    linkage->is_built = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_linkage_build(ldg_math_stat_linkage_t *linkage, const double *point, uint32_t row_cunt)
{
    uintptr_t elem_cunt = 0;
    uintptr_t elem = 0;
    uint32_t row = 0;
    uint32_t ret = linkage_init_ck(linkage);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!point)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(row_cunt == 0 || row_cunt > linkage->conf.row_cap)) { return LDG_ERR_FUNC_ARG_INVALID; }

    elem_cunt = (uintptr_t)row_cunt * (uintptr_t)linkage->conf.dim;

    ret = linkage_elem_ck(point, elem_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    linkage->is_built = LDG_TRUTH_FALSE;
    linkage->row_cunt = row_cunt;
    linkage->merge_cunt = 0;

    while (elem < elem_cunt)
    {
        linkage->centroid[elem] = point[elem];
        elem++;
    }

    while (row < row_cunt)
    {
        linkage->member_cunt[row] = LINKAGE_LEAF_SIZE;
        row++;
    }

    linkage_chain_run(linkage, row_cunt);
    linkage_merge_sort(linkage);
    linkage_node_assign(linkage, row_cunt);
    linkage->is_built = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_linkage_merge_cunt_get(const ldg_math_stat_linkage_t *linkage, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!linkage || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = linkage_built_ck(linkage);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = linkage->merge_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_linkage_merge_get(const ldg_math_stat_linkage_t *linkage, uint32_t idx, ldg_math_stat_linkage_merge_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!linkage || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, sizeof(*out));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = linkage_built_ck(linkage);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(idx >= linkage->merge_cunt)) { return LDG_ERR_BOUNDS; }

    *out = linkage->merge[idx];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_linkage_cunt_cut(ldg_math_stat_linkage_t *linkage, uint32_t cluster_cunt, uint32_t *label, uint32_t cap)
{
    uint32_t ret = linkage_built_ck(linkage);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!label)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cluster_cunt == 0 || cluster_cunt > linkage->row_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(cap < linkage->row_cunt)) { return LDG_ERR_OVERFLOW; }

    linkage_label_fill(linkage, linkage->row_cunt - cluster_cunt, label);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_linkage_height_cut(ldg_math_stat_linkage_t *linkage, double height, uint32_t *label, uint32_t cap, uint32_t *out_cunt)
{
    uint32_t apply_cunt = 0;
    uint32_t ret = linkage_built_ck(linkage);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!label || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(ldg_math_nan_is(height) == LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(cap < linkage->row_cunt)) { return LDG_ERR_OVERFLOW; }

    while (apply_cunt < linkage->merge_cunt && linkage->merge[apply_cunt].height <= height) { apply_cunt++; }

    linkage_label_fill(linkage, apply_cunt, label);
    *out_cunt = linkage->row_cunt - apply_cunt;

    return LDG_ERR_AOK;
}
