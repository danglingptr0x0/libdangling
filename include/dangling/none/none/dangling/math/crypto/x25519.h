#ifndef LDG_MATH_CRYPTO_X25519_H
#define LDG_MATH_CRYPTO_X25519_H

#include <stdint.h>

#include <dangling/core/macros.h>

#define LDG_MATH_CRYPTO_X25519_SIZE 32
#define LDG_MATH_CRYPTO_X25519_BASE_COORD 9

LDG_EXPORT uint32_t ldg_math_crypto_x25519_compute(const uint8_t *scalar, const uint8_t *point, uint8_t *out);

#endif
