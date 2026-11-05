#ifndef LDG_MATH_TRANSCRIPT_H
#define LDG_MATH_TRANSCRIPT_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/hash/blake2b.h>
#include <dangling/math/ext.h>
#include <dangling/math/field.h>

#define LDG_MATH_TRANSCRIPT_STATE_SIZE LDG_HASH_BLAKE2B_OUT_SIZE
#define LDG_MATH_TRANSCRIPT_GRIND_BIT_MAX 32

typedef struct ldg_math_transcript
{
    uint8_t state[LDG_MATH_TRANSCRIPT_STATE_SIZE];
    uint64_t cuntr;
} ldg_math_transcript_t;

LDG_EXPORT uint32_t ldg_math_transcript_init(ldg_math_transcript_t *ctx, const uint8_t *seed, uint64_t seed_len);
LDG_EXPORT uint32_t ldg_math_transcript_absorb(ldg_math_transcript_t *ctx, const uint8_t *src, uint64_t len);
LDG_EXPORT uint32_t ldg_math_transcript_elem_absorb(ldg_math_transcript_t *ctx, const ldg_math_field_elem_t *src, uint64_t cunt);
LDG_EXPORT uint32_t ldg_math_transcript_elem_get(ldg_math_transcript_t *ctx, ldg_math_field_elem_t *out);
LDG_EXPORT uint32_t ldg_math_transcript_ext_absorb(ldg_math_transcript_t *ctx, const ldg_math_ext_t *src, uint64_t cunt);
LDG_EXPORT uint32_t ldg_math_transcript_ext_get(ldg_math_transcript_t *ctx, ldg_math_ext_t *out);
LDG_EXPORT uint32_t ldg_math_transcript_idx_get(ldg_math_transcript_t *ctx, uint64_t bound, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_transcript_grind_find(ldg_math_transcript_t *ctx, uint32_t bit_cunt, uint64_t *out_nonce);
LDG_EXPORT uint32_t ldg_math_transcript_grind_ck(ldg_math_transcript_t *ctx, uint32_t bit_cunt, uint64_t nonce);

#endif
