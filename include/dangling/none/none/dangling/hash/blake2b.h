#ifndef LDG_HASH_BLAKE2B_H
#define LDG_HASH_BLAKE2B_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_HASH_BLAKE2B_OUT_SIZE 32
#define LDG_HASH_BLAKE2B_BLK_SIZE 128
#define LDG_HASH_BLAKE2B_LANE_CUNT 8

typedef struct ldg_hash_blake2b
{
    uint64_t lane[LDG_HASH_BLAKE2B_LANE_CUNT];
    uint64_t cuntr_lo;
    uint64_t cuntr_hi;
    uint8_t blk[LDG_HASH_BLAKE2B_BLK_SIZE];
    uint32_t blk_len;
    uint8_t pudding[4];
} ldg_hash_blake2b_t;

LDG_EXPORT uint32_t ldg_hash_blake2b_init(ldg_hash_blake2b_t *ctx);
LDG_EXPORT uint32_t ldg_hash_blake2b_update(ldg_hash_blake2b_t *ctx, const uint8_t *src, uint64_t len);
LDG_EXPORT uint32_t ldg_hash_blake2b_finish(ldg_hash_blake2b_t *ctx, uint8_t *out);
LDG_EXPORT uint32_t ldg_hash_blake2b_compute(const uint8_t *src, uint64_t len, uint8_t *out);

#endif
