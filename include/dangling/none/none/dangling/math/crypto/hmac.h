#ifndef LDG_MATH_CRYPTO_HMAC_H
#define LDG_MATH_CRYPTO_HMAC_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/crypto/sha2.h>

#define LDG_MATH_CRYPTO_HKDF_SHA256_OUT_SIZE_MAX 8160

typedef struct ldg_math_crypto_hmac_sha256
{
    ldg_math_crypto_sha256_t inner;
    ldg_math_crypto_sha256_t outer;
} ldg_math_crypto_hmac_sha256_t;

LDG_EXPORT uint32_t ldg_math_crypto_hmac_sha256_init(ldg_math_crypto_hmac_sha256_t *ctx, const uint8_t *key, uint32_t key_len);
LDG_EXPORT uint32_t ldg_math_crypto_hmac_sha256_update(ldg_math_crypto_hmac_sha256_t *ctx, const uint8_t *src, uint64_t len);
LDG_EXPORT uint32_t ldg_math_crypto_hmac_sha256_finish(ldg_math_crypto_hmac_sha256_t *ctx, uint8_t *out);
LDG_EXPORT uint32_t ldg_math_crypto_hmac_sha256_compute(const uint8_t *key, uint32_t key_len, const uint8_t *src, uint64_t len, uint8_t *out);
LDG_EXPORT uint32_t ldg_math_crypto_hkdf_sha256_expand(const uint8_t *key, uint32_t key_len, const uint8_t *info, uint32_t info_len, uint8_t *out, uint32_t out_len);

#endif
