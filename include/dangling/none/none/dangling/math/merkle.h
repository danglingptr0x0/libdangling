#ifndef LDG_MATH_MERKLE_H
#define LDG_MATH_MERKLE_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/hash/blake2b.h>
#include <dangling/math/field.h>

#define LDG_MATH_MERKLE_NODE_SIZE LDG_HASH_BLAKE2B_OUT_SIZE

LDG_EXPORT uint32_t ldg_math_merkle_node_cunt_get(uint64_t leaf_cunt, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_merkle_depth_get(uint64_t leaf_cunt, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_merkle_build(const ldg_math_field_elem_t *leaf, uint64_t leaf_cunt, uint64_t elem_per_leaf, uint8_t *node, uint64_t node_cap);
LDG_EXPORT uint32_t ldg_math_merkle_root_get(const uint8_t *node, uint64_t leaf_cunt, uint8_t *out);
LDG_EXPORT uint32_t ldg_math_merkle_path_get(const uint8_t *node, uint64_t leaf_cunt, uint64_t leaf_idx, uint8_t *out_path, uint64_t path_cap);
LDG_EXPORT uint32_t ldg_math_merkle_path_ck(const uint8_t *root, const ldg_math_field_elem_t *leaf, uint64_t elem_per_leaf, uint64_t leaf_idx, uint64_t leaf_cunt, const uint8_t *path, uint64_t path_cap);

#endif
