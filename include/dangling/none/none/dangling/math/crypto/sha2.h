#ifndef LDG_MATH_CRYPTO_SHA2_H
#define LDG_MATH_CRYPTO_SHA2_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_MATH_CRYPTO_SHA256_OUT_SIZE 32
#define LDG_MATH_CRYPTO_SHA384_OUT_SIZE 48
#define LDG_MATH_CRYPTO_SHA512_OUT_SIZE 64
#define LDG_MATH_CRYPTO_SHA256_BLK_SIZE 64
#define LDG_MATH_CRYPTO_SHA512_BLK_SIZE 128
#define LDG_MATH_CRYPTO_SHA_LANE_CUNT 8

typedef struct ldg_math_crypto_sha256
{
    uint32_t lane[LDG_MATH_CRYPTO_SHA_LANE_CUNT];
    uint64_t len;
    uint8_t blk[LDG_MATH_CRYPTO_SHA256_BLK_SIZE];
    uint32_t blk_len;
    uint8_t pudding[4];
} ldg_math_crypto_sha256_t;

typedef struct ldg_math_crypto_sha512
{
    uint64_t lane[LDG_MATH_CRYPTO_SHA_LANE_CUNT];
    uint64_t len_lo;
    uint64_t len_hi;
    uint8_t blk[LDG_MATH_CRYPTO_SHA512_BLK_SIZE];
    uint32_t blk_len;
    uint32_t out_size;
} ldg_math_crypto_sha512_t;

LDG_EXPORT uint32_t ldg_math_crypto_sha256_init(ldg_math_crypto_sha256_t *ctx);
LDG_EXPORT uint32_t ldg_math_crypto_sha256_update(ldg_math_crypto_sha256_t *ctx, const uint8_t *src, uint64_t len);
LDG_EXPORT uint32_t ldg_math_crypto_sha256_finish(ldg_math_crypto_sha256_t *ctx, uint8_t *out);
LDG_EXPORT uint32_t ldg_math_crypto_sha256_compute(const uint8_t *src, uint64_t len, uint8_t *out);

LDG_EXPORT uint32_t ldg_math_crypto_sha384_init(ldg_math_crypto_sha512_t *ctx);
LDG_EXPORT uint32_t ldg_math_crypto_sha512_init(ldg_math_crypto_sha512_t *ctx);
LDG_EXPORT uint32_t ldg_math_crypto_sha512_update(ldg_math_crypto_sha512_t *ctx, const uint8_t *src, uint64_t len);
LDG_EXPORT uint32_t ldg_math_crypto_sha512_finish(ldg_math_crypto_sha512_t *ctx, uint8_t *out);
LDG_EXPORT uint32_t ldg_math_crypto_sha384_compute(const uint8_t *src, uint64_t len, uint8_t *out);
LDG_EXPORT uint32_t ldg_math_crypto_sha512_compute(const uint8_t *src, uint64_t len, uint8_t *out);

#endif
