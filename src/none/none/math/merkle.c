#include <stdint.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/hash/blake2b.h>
#include <dangling/math/field.h>
#include <dangling/math/merkle.h>

#define LDG_MATH_MERKLE_WORD_SIZE 8
#define LDG_MATH_MERKLE_PAIR_STEP 2
#define LDG_MATH_MERKLE_PREV_OFF 1
#define LDG_MATH_MERKLE_SIDE_MASK 1
#define LDG_MATH_MERKLE_ROOT_IDX 1

static void merkle_word_st(uint64_t val, uint8_t *out)
{
    uint32_t idx = 0;

    for (idx = 0; idx < LDG_MATH_MERKLE_WORD_SIZE; idx++) { out[idx] = (uint8_t)((val >> (idx * LDG_BYTE_BITS)) & LDG_BYTE_MASK); }
}

static uint32_t merkle_leaf_hash(const ldg_math_field_elem_t *leaf, uint64_t elem_per_leaf, uint8_t *out)
{
    ldg_hash_blake2b_t hasher = LDG_STRUCT_ZERO_INIT;
    uint8_t word_buff[LDG_MATH_MERKLE_WORD_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = LDG_ERR_AOK;
    uint64_t idx = 0;

    ret = ldg_hash_blake2b_init(&hasher);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < elem_per_leaf; idx++)
    {
        merkle_word_st(ldg_math_field_reduce(leaf[idx]), word_buff);
        ret = ldg_hash_blake2b_update(&hasher, word_buff, LDG_MATH_MERKLE_WORD_SIZE);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return ldg_hash_blake2b_finish(&hasher, out);
}

static uint32_t merkle_pair_hash(const uint8_t *lhs, const uint8_t *rhs, uint8_t *out)
{
    ldg_hash_blake2b_t hasher = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = LDG_ERR_AOK;

    ret = ldg_hash_blake2b_init(&hasher);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_hash_blake2b_update(&hasher, lhs, LDG_MATH_MERKLE_NODE_SIZE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_hash_blake2b_update(&hasher, rhs, LDG_MATH_MERKLE_NODE_SIZE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_hash_blake2b_finish(&hasher, out);
}

static void merkle_node_cpy(uint8_t *dst, const uint8_t *src)
{
    uint32_t idx = 0;

    for (idx = 0; idx < LDG_MATH_MERKLE_NODE_SIZE; idx++) { dst[idx] = src[idx]; }
}

uint32_t ldg_math_merkle_node_cunt_get(uint64_t leaf_cunt, uint64_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(leaf_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!LDG_IS_POW2(leaf_cunt))) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = leaf_cunt * LDG_MATH_MERKLE_PAIR_STEP;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_merkle_depth_get(uint64_t leaf_cunt, uint32_t *out)
{
    uint64_t walk = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(leaf_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!LDG_IS_POW2(leaf_cunt))) { return LDG_ERR_FUNC_ARG_INVALID; }

    walk = leaf_cunt;

    while (walk > LDG_MATH_MERKLE_ROOT_IDX)
    {
        walk /= LDG_MATH_MERKLE_PAIR_STEP;
        (*out)++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_merkle_build(const ldg_math_field_elem_t *leaf, uint64_t leaf_cunt, uint64_t elem_per_leaf, uint8_t *node, uint64_t node_cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t node_cunt = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!leaf || !node)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(elem_per_leaf == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_merkle_node_cunt_get(leaf_cunt, &node_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(node_cap < (node_cunt * LDG_MATH_MERKLE_NODE_SIZE))) { return LDG_ERR_BOUNDS; }

    for (idx = 0; idx < LDG_MATH_MERKLE_NODE_SIZE; idx++) { node[idx] = 0; }

    for (idx = 0; idx < leaf_cunt; idx++)
    {
        ret = merkle_leaf_hash(leaf + (idx * elem_per_leaf), elem_per_leaf, node + ((leaf_cunt + idx) * LDG_MATH_MERKLE_NODE_SIZE));

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    idx = leaf_cunt;

    while (idx > LDG_MATH_MERKLE_ROOT_IDX)
    {
        idx--;
        ret = merkle_pair_hash(node + ((idx * LDG_MATH_MERKLE_PAIR_STEP) * LDG_MATH_MERKLE_NODE_SIZE), node + (((idx * LDG_MATH_MERKLE_PAIR_STEP) + LDG_MATH_MERKLE_PREV_OFF) * LDG_MATH_MERKLE_NODE_SIZE), node + (idx * LDG_MATH_MERKLE_NODE_SIZE));

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_merkle_root_get(const uint8_t *node, uint64_t leaf_cunt, uint8_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t node_cunt = 0;

    if (LDG_UNLIKELY(!node || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_merkle_node_cunt_get(leaf_cunt, &node_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    merkle_node_cpy(out, node + (LDG_MATH_MERKLE_ROOT_IDX * LDG_MATH_MERKLE_NODE_SIZE));

    return LDG_ERR_AOK;
}

uint32_t ldg_math_merkle_path_get(const uint8_t *node, uint64_t leaf_cunt, uint64_t leaf_idx, uint8_t *out_path, uint64_t path_cap)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t depth = 0;
    uint64_t pos = 0;
    uint64_t level = 0;

    if (LDG_UNLIKELY(!node || !out_path)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_merkle_depth_get(leaf_cunt, &depth);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(leaf_idx >= leaf_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(path_cap < ((uint64_t)depth * LDG_MATH_MERKLE_NODE_SIZE))) { return LDG_ERR_BOUNDS; }

    pos = leaf_cunt + leaf_idx;

    while (pos > LDG_MATH_MERKLE_ROOT_IDX)
    {
        merkle_node_cpy(out_path + (level * LDG_MATH_MERKLE_NODE_SIZE), node + ((pos ^ LDG_MATH_MERKLE_SIDE_MASK) * LDG_MATH_MERKLE_NODE_SIZE));
        pos /= LDG_MATH_MERKLE_PAIR_STEP;
        level++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_merkle_path_ck(const uint8_t *root, const ldg_math_field_elem_t *leaf, uint64_t elem_per_leaf, uint64_t leaf_idx, uint64_t leaf_cunt, const uint8_t *path, uint64_t path_cap)
{
    uint8_t curr[LDG_MATH_MERKLE_NODE_SIZE] = LDG_ARR_ZERO_INIT;
    uint8_t next[LDG_MATH_MERKLE_NODE_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = LDG_ERR_AOK;
    uint32_t depth = 0;
    uint64_t pos = 0;
    uint64_t level = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!root || !leaf || !path)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(elem_per_leaf == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_merkle_depth_get(leaf_cunt, &depth);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(leaf_idx >= leaf_cunt)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(path_cap < ((uint64_t)depth * LDG_MATH_MERKLE_NODE_SIZE))) { return LDG_ERR_BOUNDS; }

    ret = merkle_leaf_hash(leaf, elem_per_leaf, curr);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    pos = leaf_cunt + leaf_idx;

    for (level = 0; level < depth; level++)
    {
        if ((pos & LDG_MATH_MERKLE_SIDE_MASK) == 0) { ret = merkle_pair_hash(curr, path + (level * LDG_MATH_MERKLE_NODE_SIZE), next); }
        else { ret = merkle_pair_hash(path + (level * LDG_MATH_MERKLE_NODE_SIZE), curr, next); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        merkle_node_cpy(curr, next);
        pos /= LDG_MATH_MERKLE_PAIR_STEP;
    }

    for (idx = 0; idx < LDG_MATH_MERKLE_NODE_SIZE; idx++) { if (curr[idx] != root[idx]) { return LDG_ERR_INVALID; } }

    return LDG_ERR_AOK;
}
