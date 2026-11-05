#ifndef LDG_MATH_CRYPTO_CHACHA_H
#define LDG_MATH_CRYPTO_CHACHA_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_MATH_CRYPTO_CHACHA20_KEY_SIZE 32
#define LDG_MATH_CRYPTO_CHACHA20_NONCE_SIZE 12
#define LDG_MATH_CRYPTO_POLY1305_TAG_SIZE 16

LDG_EXPORT uint32_t ldg_math_crypto_chacha20_poly1305_seal(const uint8_t *key, const uint8_t *nonce, const uint8_t *extra, uint32_t extra_len, const uint8_t *src, uint32_t len, uint8_t *dst, uint8_t *tag);
LDG_EXPORT uint32_t ldg_math_crypto_chacha20_poly1305_open(const uint8_t *key, const uint8_t *nonce, const uint8_t *extra, uint32_t extra_len, const uint8_t *src, uint32_t len, const uint8_t *tag, uint8_t *dst);

#endif
