#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/crypto/rsa.h>
#include <dangling/math/crypto/sha2.h>

#include "bignum.h"

#define RSA_LIMB_SIZE 4
#define RSA_LIMB_ROUND 3
#define RSA_BYTE_BITS 8
#define RSA_BIT_MASK 0x1U
#define RSA_ADJACENT_OFF 1
#define RSA_REGION_CUNT 7
#define RSA_IDENT_LIMB 0x1U
#define RSA_EXPONENT_MIN 3
#define RSA_HDR_SIZE 19
#define RSA_HDR_BODY_SIZE 17
#define RSA_DER_SEQUENCE 0x30
#define RSA_DER_OID 0x06
#define RSA_DER_NULL 0x05
#define RSA_DER_OCTET_STRING 0x04
#define RSA_ALG_SIZE 0x0D
#define RSA_OID_SIZE 0x09
#define RSA_OID_STEM 0x6086480165030402ULL
#define RSA_OID_STEM_SIZE 8
#define RSA_OID_SHA256_TAIL 0x1U
#define RSA_NULL_SIZE 0
#define RSA_BLK_KIND 0x01
#define RSA_BLK_KIND_OFF 1
#define RSA_BLK_FILL 0xFF
#define RSA_BLK_FILL_OFF 2

typedef struct rsa_work
{
    ldg_math_crypto_mont_t mont;
    uint32_t *modulus;
    uint32_t *square;
    uint32_t *base;
    uint32_t *acc;
    uint32_t *unit;
    uint32_t *work;
    uint8_t *blk;
    uint32_t limb_cunt;
    uint32_t size;
} rsa_work_t;

static uint32_t rsa_digest_size_get(ldg_math_crypto_rsa_digest_t kind)
{
    uint32_t size = 0;

    if (kind == LDG_MATH_CRYPTO_RSA_DIGEST_SHA256) { size = LDG_MATH_CRYPTO_SHA256_OUT_SIZE; }

    if (kind == LDG_MATH_CRYPTO_RSA_DIGEST_SHA384) { size = LDG_MATH_CRYPTO_SHA384_OUT_SIZE; }

    if (kind == LDG_MATH_CRYPTO_RSA_DIGEST_SHA512) { size = LDG_MATH_CRYPTO_SHA512_OUT_SIZE; }

    return size;
}

static void rsa_hdr_fill(uint8_t *dst, ldg_math_crypto_rsa_digest_t kind, uint32_t digest_len)
{
    uint32_t pos = 0;
    uint32_t idx = 0;

    dst[pos] = RSA_DER_SEQUENCE;
    pos++;
    dst[pos] = (uint8_t)(RSA_HDR_BODY_SIZE + digest_len);
    pos++;
    dst[pos] = RSA_DER_SEQUENCE;
    pos++;
    dst[pos] = RSA_ALG_SIZE;
    pos++;
    dst[pos] = RSA_DER_OID;
    pos++;
    dst[pos] = RSA_OID_SIZE;
    pos++;

    while (idx < RSA_OID_STEM_SIZE)
    {
        dst[pos] = (uint8_t)(RSA_OID_STEM >> (RSA_BYTE_BITS * (RSA_OID_STEM_SIZE - RSA_ADJACENT_OFF - idx)));
        pos++;
        idx++;
    }

    dst[pos] = (uint8_t)(RSA_OID_SHA256_TAIL + (uint32_t)kind);
    pos++;
    dst[pos] = RSA_DER_NULL;
    pos++;
    dst[pos] = RSA_NULL_SIZE;
    pos++;
    dst[pos] = RSA_DER_OCTET_STRING;
    pos++;
    dst[pos] = (uint8_t)digest_len;
}

static uint8_t rsa_blk_match_is(const uint8_t *blk, uint32_t size, ldg_math_crypto_rsa_digest_t kind, const uint8_t *digest, uint32_t digest_len)
{
    uint8_t hdr[RSA_HDR_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t fill_end = size - RSA_HDR_SIZE - digest_len - RSA_ADJACENT_OFF;
    uint32_t diff = 0;
    uint32_t idx = RSA_BLK_FILL_OFF;

    rsa_hdr_fill(hdr, kind, digest_len);

    diff = (uint32_t)blk[0] | (uint32_t)(blk[RSA_BLK_KIND_OFF] ^ RSA_BLK_KIND) | (uint32_t)blk[fill_end];

    while (idx < fill_end)
    {
        diff |= (uint32_t)(blk[idx] ^ RSA_BLK_FILL);
        idx++;
    }

    idx = 0;

    while (idx < RSA_HDR_SIZE)
    {
        diff |= (uint32_t)(blk[fill_end + RSA_ADJACENT_OFF + idx] ^ hdr[idx]);
        idx++;
    }

    idx = 0;

    while (idx < digest_len)
    {
        diff |= (uint32_t)(blk[fill_end + RSA_ADJACENT_OFF + RSA_HDR_SIZE + idx] ^ digest[idx]);
        idx++;
    }

    return (diff == 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t rsa_exponent_usable_is(const uint8_t *exponent, uint32_t exponent_len)
{
    uint32_t idx = 0;

    while (idx < exponent_len && exponent[idx] == 0) { idx++; }

    if (idx == exponent_len) { return LDG_TRUTH_FALSE; }

    if ((exponent[exponent_len - RSA_ADJACENT_OFF] & RSA_BIT_MASK) == 0) { return LDG_TRUTH_FALSE; }

    if (idx == (exponent_len - RSA_ADJACENT_OFF) && exponent[idx] < RSA_EXPONENT_MIN) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static void rsa_area_set(rsa_work_t *area, uint32_t *buff, uint32_t limb_cunt, uint32_t size)
{
    area->modulus = buff;
    area->square = area->modulus + limb_cunt;
    area->base = area->square + limb_cunt;
    area->acc = area->base + limb_cunt;
    area->unit = area->acc + limb_cunt;
    area->work = area->unit + limb_cunt;
    area->blk = (uint8_t *)(area->work + limb_cunt + LDG_MATH_CRYPTO_BIGNUM_WORK_EXTRA);
    area->limb_cunt = limb_cunt;
    area->size = size;
}

static uint32_t rsa_pwr_bit_apply(const rsa_work_t *area, uint8_t bit_set_is, uint8_t *started_is)
{
    uint32_t limb = 0;
    uint32_t ret = 0;

    if (*started_is)
    {
        ret = ldg_math_crypto_mont_mul(&area->mont, area->acc, area->acc, area->acc, area->work);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (bit_set_is) { ret = ldg_math_crypto_mont_mul(&area->mont, area->acc, area->acc, area->base, area->work); }

        return ret;
    }

    if (!bit_set_is) { return LDG_ERR_AOK; }

    while (limb < area->limb_cunt)
    {
        area->acc[limb] = area->base[limb];
        limb++;
    }

    *started_is = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t rsa_pwr_compute(const rsa_work_t *area, const uint8_t *exponent, uint32_t exponent_len)
{
    uint8_t started_is = 0;
    uint32_t idx = 0;
    uint32_t bit = 0;
    uint32_t ret = 0;

    while (idx < exponent_len)
    {
        bit = RSA_BYTE_BITS;

        while (bit > 0)
        {
            bit--;
            ret = rsa_pwr_bit_apply(area, (uint8_t)((exponent[idx] >> bit) & RSA_BIT_MASK), &started_is);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        idx++;
    }

    return LDG_ERR_AOK;
}

static uint32_t rsa_blk_compute(rsa_work_t *area, const uint8_t *exponent, uint32_t exponent_len)
{
    uint32_t limb = 0;
    uint32_t ret = 0;

    ret = ldg_math_crypto_mont_init(&area->mont, area->modulus, area->square, area->limb_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_crypto_mont_mul(&area->mont, area->base, area->unit, area->square, area->work);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (limb < area->limb_cunt)
    {
        area->unit[limb] = 0;
        limb++;
    }

    area->unit[0] = RSA_IDENT_LIMB;

    ret = rsa_pwr_compute(area, exponent, exponent_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_crypto_mont_mul(&area->mont, area->acc, area->acc, area->unit, area->work);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_crypto_bignum_bytes_st(area->acc, area->limb_cunt, area->blk, area->size);
}

uint8_t ldg_math_crypto_rsa_sig_valid_is(const uint8_t *modulus, uint32_t modulus_len, const uint8_t *exponent, uint32_t exponent_len, ldg_math_crypto_rsa_digest_t kind, const uint8_t *digest, uint32_t digest_len, const uint8_t *sig, uint32_t sig_len, uint32_t *buff, uint32_t buff_cunt)
{
    rsa_work_t area = LDG_STRUCT_ZERO_INIT;
    uint32_t skip = 0;
    uint32_t size = 0;
    uint32_t limb_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!modulus || !exponent || !digest || !sig || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(digest_len == 0 || digest_len != rsa_digest_size_get(kind))) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (skip < modulus_len && modulus[skip] == 0) { skip++; }

    size = modulus_len - skip;

    if (LDG_UNLIKELY(size < LDG_MATH_CRYPTO_RSA_MODULUS_SIZE_MIN || size > LDG_MATH_CRYPTO_RSA_MODULUS_SIZE_MAX)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(exponent_len > modulus_len || rsa_exponent_usable_is(exponent, exponent_len) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    limb_cunt = (size + RSA_LIMB_ROUND) / RSA_LIMB_SIZE;

    if (LDG_UNLIKELY(buff_cunt < ((RSA_REGION_CUNT * limb_cunt) + LDG_MATH_CRYPTO_BIGNUM_WORK_EXTRA))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (sig_len != size) { return LDG_TRUTH_FALSE; }

    rsa_area_set(&area, buff, limb_cunt, size);

    ret = ldg_math_crypto_bignum_bytes_ld(area.modulus, limb_cunt, modulus + skip, size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    ret = ldg_math_crypto_bignum_bytes_ld(area.unit, limb_cunt, sig, sig_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    if (ldg_math_crypto_bignum_ge_is(area.unit, area.modulus, limb_cunt) == LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    ret = rsa_blk_compute(&area, exponent, exponent_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    return rsa_blk_match_is(area.blk, size, kind, digest, digest_len);
}
