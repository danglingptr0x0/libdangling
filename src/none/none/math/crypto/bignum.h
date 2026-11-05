#ifndef LDG_MATH_CRYPTO_BIGNUM_H
#define LDG_MATH_CRYPTO_BIGNUM_H

#include <stdint.h>

#define LDG_MATH_CRYPTO_BIGNUM_WORK_EXTRA 2

typedef struct ldg_math_crypto_mont
{
    const uint32_t *modulus;
    const uint32_t *square;
    uint32_t inv;
    uint32_t limb_cunt;
} ldg_math_crypto_mont_t;

uint32_t ldg_math_crypto_bignum_bytes_ld(uint32_t *dst, uint32_t limb_cunt, const uint8_t *src, uint32_t len);
uint32_t ldg_math_crypto_bignum_bytes_st(const uint32_t *src, uint32_t limb_cunt, uint8_t *dst, uint32_t len);
uint8_t ldg_math_crypto_bignum_ge_is(const uint32_t *lhs, const uint32_t *rhs, uint32_t limb_cunt);
uint8_t ldg_math_crypto_bignum_none_is(const uint32_t *val, uint32_t limb_cunt);
uint32_t ldg_math_crypto_bignum_bit_len_get(const uint32_t *val, uint32_t limb_cunt);
uint32_t ldg_math_crypto_bignum_add(uint32_t *dst, const uint32_t *lhs, const uint32_t *rhs, uint32_t limb_cunt, uint32_t *out_carry);
uint32_t ldg_math_crypto_bignum_sub(uint32_t *dst, const uint32_t *lhs, const uint32_t *rhs, uint32_t limb_cunt, uint32_t *out_borrow);
uint32_t ldg_math_crypto_mont_init(ldg_math_crypto_mont_t *mont, const uint32_t *modulus, uint32_t *square, uint32_t limb_cunt);
uint32_t ldg_math_crypto_mont_mul(const ldg_math_crypto_mont_t *mont, uint32_t *dst, const uint32_t *lhs, const uint32_t *rhs, uint32_t *work);

#endif
