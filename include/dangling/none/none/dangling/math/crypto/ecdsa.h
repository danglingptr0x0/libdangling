#ifndef LDG_MATH_CRYPTO_ECDSA_H
#define LDG_MATH_CRYPTO_ECDSA_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_MATH_CRYPTO_ECDSA_COORD_SIZE_MAX 48
#define LDG_MATH_CRYPTO_ECDSA_BUFF_CUNT 470

typedef enum ldg_math_crypto_ecdsa_curve
{
    LDG_MATH_CRYPTO_ECDSA_CURVE_P256,
    LDG_MATH_CRYPTO_ECDSA_CURVE_P384,
    LDG_MATH_CRYPTO_ECDSA_CURVE_SECP256K1,
    LDG_MATH_CRYPTO_ECDSA_CURVE_CUNT
} ldg_math_crypto_ecdsa_curve_t;

LDG_EXPORT uint8_t ldg_math_crypto_ecdsa_sig_valid_is(ldg_math_crypto_ecdsa_curve_t curve, const uint8_t *key, uint32_t key_len, const uint8_t *digest, uint32_t digest_len, const uint8_t *sig_r, uint32_t sig_r_len, const uint8_t *sig_s, uint32_t sig_s_len, uint32_t *buff, uint32_t buff_cunt);
LDG_EXPORT uint8_t ldg_math_crypto_ecdsa_der_valid_is(ldg_math_crypto_ecdsa_curve_t curve, const uint8_t *key, uint32_t key_len, const uint8_t *digest, uint32_t digest_len, const uint8_t *sig, uint32_t sig_len, uint32_t *buff, uint32_t buff_cunt);

#endif
