#ifndef LDG_HASH_POSEIDON2_H
#define LDG_HASH_POSEIDON2_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/math/field.h>

#define LDG_HASH_POSEIDON2_WIDTH 12
#define LDG_HASH_POSEIDON2_RATE 8
#define LDG_HASH_POSEIDON2_CAP 4
#define LDG_HASH_POSEIDON2_OUT_CUNT 4
#define LDG_HASH_POSEIDON2_FULL_ROUND_CUNT 8
#define LDG_HASH_POSEIDON2_PART_ROUND_CUNT 22
#define LDG_HASH_POSEIDON2_SBOX_DEGREE 7

LDG_EXPORT uint32_t ldg_hash_poseidon2_permute(ldg_math_field_elem_t *state);
LDG_EXPORT uint32_t ldg_hash_poseidon2_compute(const ldg_math_field_elem_t *src, uint64_t cunt, ldg_math_field_elem_t *out);
LDG_EXPORT uint32_t ldg_hash_poseidon2_pair_compress(const ldg_math_field_elem_t *lhs, const ldg_math_field_elem_t *rhs, ldg_math_field_elem_t *out);

#endif
