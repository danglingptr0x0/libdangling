#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/crypto/hmac.h>
#include <dangling/math/crypto/sha2.h>
#include <dangling/mem/secure.h>

#define HMAC_INNER_MASK 0x36
#define HMAC_OUTER_MASK 0x5C
#define HKDF_CUNTR_BASE 0x1U

static uint32_t hmac_side_init(ldg_math_crypto_sha256_t *side, const uint8_t *pad, uint8_t mask)
{
    uint8_t blk[LDG_MATH_CRYPTO_SHA256_BLK_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t ret = 0;

    while (idx < LDG_MATH_CRYPTO_SHA256_BLK_SIZE)
    {
        blk[idx] = (uint8_t)(pad[idx] ^ mask);
        idx++;
    }

    ret = ldg_math_crypto_sha256_init(side);

    if (ret == LDG_ERR_AOK) { ret = ldg_math_crypto_sha256_update(side, blk, LDG_MATH_CRYPTO_SHA256_BLK_SIZE); }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(blk, LDG_MATH_CRYPTO_SHA256_BLK_SIZE) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    return ret;
}

uint32_t ldg_math_crypto_hmac_sha256_init(ldg_math_crypto_hmac_sha256_t *ctx, const uint8_t *key, uint32_t key_len)
{
    uint8_t pad[LDG_MATH_CRYPTO_SHA256_BLK_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || (!key && key_len != 0))) { return LDG_ERR_FUNC_ARG_NULL; }

    if (key_len > LDG_MATH_CRYPTO_SHA256_BLK_SIZE) { ret = ldg_math_crypto_sha256_compute(key, key_len, pad); }

    while (key_len <= LDG_MATH_CRYPTO_SHA256_BLK_SIZE && idx < key_len)
    {
        pad[idx] = key[idx];
        idx++;
    }

    if (ret == LDG_ERR_AOK) { ret = hmac_side_init(&ctx->inner, pad, HMAC_INNER_MASK); }

    if (ret == LDG_ERR_AOK) { ret = hmac_side_init(&ctx->outer, pad, HMAC_OUTER_MASK); }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(pad, LDG_MATH_CRYPTO_SHA256_BLK_SIZE) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    return ret;
}

uint32_t ldg_math_crypto_hmac_sha256_update(ldg_math_crypto_hmac_sha256_t *ctx, const uint8_t *src, uint64_t len)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_math_crypto_sha256_update(&ctx->inner, src, len);
}

uint32_t ldg_math_crypto_hmac_sha256_finish(ldg_math_crypto_hmac_sha256_t *ctx, uint8_t *out)
{
    uint8_t digest[LDG_MATH_CRYPTO_SHA256_OUT_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_crypto_sha256_finish(&ctx->inner, digest);

    if (ret == LDG_ERR_AOK) { ret = ldg_math_crypto_sha256_update(&ctx->outer, digest, LDG_MATH_CRYPTO_SHA256_OUT_SIZE); }

    if (ret == LDG_ERR_AOK) { ret = ldg_math_crypto_sha256_finish(&ctx->outer, out); }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(digest, LDG_MATH_CRYPTO_SHA256_OUT_SIZE) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    return ret;
}

uint32_t ldg_math_crypto_hmac_sha256_compute(const uint8_t *key, uint32_t key_len, const uint8_t *src, uint64_t len, uint8_t *out)
{
    ldg_math_crypto_hmac_sha256_t ctx = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out || (!key && key_len != 0) || (!src && len != 0))) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_crypto_hmac_sha256_init(&ctx, key, key_len);

    if (ret == LDG_ERR_AOK) { ret = ldg_math_crypto_hmac_sha256_update(&ctx, src, len); }

    if (ret == LDG_ERR_AOK) { ret = ldg_math_crypto_hmac_sha256_finish(&ctx, out); }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(&ctx, sizeof(ctx)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    return ret;
}

static uint32_t hkdf_blk_compute(ldg_math_crypto_hmac_sha256_t *ctx, const uint8_t *key, uint32_t key_len, uint8_t *blk, uint32_t blk_len, const uint8_t *info, uint32_t info_len, uint8_t cuntr)
{
    uint32_t ret = 0;

    ret = ldg_math_crypto_hmac_sha256_init(ctx, key, key_len);

    if (ret == LDG_ERR_AOK) { ret = ldg_math_crypto_hmac_sha256_update(ctx, blk, blk_len); }

    if (ret == LDG_ERR_AOK) { ret = ldg_math_crypto_hmac_sha256_update(ctx, info, info_len); }

    if (ret == LDG_ERR_AOK) { ret = ldg_math_crypto_hmac_sha256_update(ctx, &cuntr, sizeof(cuntr)); }

    if (ret == LDG_ERR_AOK) { ret = ldg_math_crypto_hmac_sha256_finish(ctx, blk); }

    return ret;
}

uint32_t ldg_math_crypto_hkdf_sha256_expand(const uint8_t *key, uint32_t key_len, const uint8_t *info, uint32_t info_len, uint8_t *out, uint32_t out_len)
{
    ldg_math_crypto_hmac_sha256_t ctx = LDG_STRUCT_ZERO_INIT;
    uint8_t blk[LDG_MATH_CRYPTO_SHA256_OUT_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t blk_len = 0;
    uint32_t done = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t cuntr = HKDF_CUNTR_BASE;

    if (LDG_UNLIKELY(!key || !out || (!info && info_len != 0))) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_len > LDG_MATH_CRYPTO_HKDF_SHA256_OUT_SIZE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (done < out_len && ret == LDG_ERR_AOK)
    {
        ret = hkdf_blk_compute(&ctx, key, key_len, blk, blk_len, info, info_len, cuntr);
        blk_len = LDG_MATH_CRYPTO_SHA256_OUT_SIZE;
        idx = 0;

        while (ret == LDG_ERR_AOK && idx < LDG_MATH_CRYPTO_SHA256_OUT_SIZE && done < out_len)
        {
            out[done] = blk[idx];
            done++;
            idx++;
        }

        cuntr++;
    }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(blk, LDG_MATH_CRYPTO_SHA256_OUT_SIZE) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(&ctx, sizeof(ctx)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    return ret;
}
