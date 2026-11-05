#ifndef LDG_MATH_CRYPTO_RSA_H
#define LDG_MATH_CRYPTO_RSA_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_MATH_CRYPTO_RSA_MODULUS_SIZE_MIN 256
#define LDG_MATH_CRYPTO_RSA_MODULUS_SIZE_MAX 512
#define LDG_MATH_CRYPTO_RSA_BUFF_CUNT 898

typedef enum ldg_math_crypto_rsa_digest
{
    LDG_MATH_CRYPTO_RSA_DIGEST_SHA256,
    LDG_MATH_CRYPTO_RSA_DIGEST_SHA384,
    LDG_MATH_CRYPTO_RSA_DIGEST_SHA512,
    LDG_MATH_CRYPTO_RSA_DIGEST_CUNT
} ldg_math_crypto_rsa_digest_t;

LDG_EXPORT uint8_t ldg_math_crypto_rsa_sig_valid_is(const uint8_t *modulus, uint32_t modulus_len, const uint8_t *exponent, uint32_t exponent_len, ldg_math_crypto_rsa_digest_t kind, const uint8_t *digest, uint32_t digest_len, const uint8_t *sig, uint32_t sig_len, uint32_t *buff, uint32_t buff_cunt);

#endif
