#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/crypto/sha2.h>
#include <dangling/mem/cpy.h>

#define SHA2_NARROW_BITS 32
#define SHA2_WIDE_BITS 64
#define SHA2_NARROW_WORD_SIZE 4
#define SHA2_WIDE_WORD_SIZE 8
#define SHA2_NARROW_ROUND_CUNT 64
#define SHA2_WIDE_ROUND_CUNT 80
#define SHA2_SCHED_CUNT 16
#define SHA2_SCHED_MASK 0xFU
#define SHA2_SCHED_NEAR_OFF 14
#define SHA2_SCHED_MID_OFF 9
#define SHA2_SCHED_FAR_OFF 1
#define SHA2_WORK_LAST 7
#define SHA2_WORK_MID 4
#define SHA2_ADJACENT_OFF 1
#define SHA2_BYTE_BITS 8
#define SHA2_BYTE_MASK 0xFFU
#define SHA2_PAD_MARK 0x80U
#define SHA2_NARROW_LEN_SIZE 8
#define SHA2_WIDE_LEN_SIZE 16
#define SHA2_BIT_SHIFT 3
#define SHA2_BIT_CARRY_SHIFT 61

#define SHA2_NARROW_BIG_A_FIRST 2
#define SHA2_NARROW_BIG_A_SECOND 13
#define SHA2_NARROW_BIG_A_THIRD 22
#define SHA2_NARROW_BIG_E_FIRST 6
#define SHA2_NARROW_BIG_E_SECOND 11
#define SHA2_NARROW_BIG_E_THIRD 25
#define SHA2_NARROW_SMALL_FAR_FIRST 7
#define SHA2_NARROW_SMALL_FAR_SECOND 18
#define SHA2_NARROW_SMALL_FAR_SHIFT 3
#define SHA2_NARROW_SMALL_NEAR_FIRST 17
#define SHA2_NARROW_SMALL_NEAR_SECOND 19
#define SHA2_NARROW_SMALL_NEAR_SHIFT 10

#define SHA2_WIDE_BIG_A_FIRST 28
#define SHA2_WIDE_BIG_A_SECOND 34
#define SHA2_WIDE_BIG_A_THIRD 39
#define SHA2_WIDE_BIG_E_FIRST 14
#define SHA2_WIDE_BIG_E_SECOND 18
#define SHA2_WIDE_BIG_E_THIRD 41
#define SHA2_WIDE_SMALL_FAR_FIRST 1
#define SHA2_WIDE_SMALL_FAR_SECOND 8
#define SHA2_WIDE_SMALL_FAR_SHIFT 7
#define SHA2_WIDE_SMALL_NEAR_FIRST 19
#define SHA2_WIDE_SMALL_NEAR_SECOND 61
#define SHA2_WIDE_SMALL_NEAR_SHIFT 6

static uint32_t sha2_narrow_rotr_get(uint32_t val, uint32_t cunt)
{
    return (val >> cunt) | (val << (SHA2_NARROW_BITS - cunt));
}

static uint64_t sha2_wide_rotr_get(uint64_t val, uint32_t cunt)
{
    return (val >> cunt) | (val << (SHA2_WIDE_BITS - cunt));
}

static uint32_t sha2_narrow_const_get(uint32_t round)
{
    const uint32_t table[SHA2_NARROW_ROUND_CUNT] =
    {
        0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5, 0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174, 0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA, 0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967, 0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85, 0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070, 0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3, 0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
    };

    return table[round];
}

static uint64_t sha2_wide_const_get(uint32_t round)
{
    const uint64_t table[SHA2_WIDE_ROUND_CUNT] =
    {
        0x428A2F98D728AE22, 0x7137449123EF65CD, 0xB5C0FBCFEC4D3B2F, 0xE9B5DBA58189DBBC, 0x3956C25BF348B538, 0x59F111F1B605D019, 0x923F82A4AF194F9B, 0xAB1C5ED5DA6D8118, 0xD807AA98A3030242, 0x12835B0145706FBE, 0x243185BE4EE4B28C, 0x550C7DC3D5FFB4E2, 0x72BE5D74F27B896F, 0x80DEB1FE3B1696B1, 0x9BDC06A725C71235, 0xC19BF174CF692694, 0xE49B69C19EF14AD2, 0xEFBE4786384F25E3, 0x0FC19DC68B8CD5B5, 0x240CA1CC77AC9C65, 0x2DE92C6F592B0275, 0x4A7484AA6EA6E483, 0x5CB0A9DCBD41FBD4, 0x76F988DA831153B5, 0x983E5152EE66DFAB, 0xA831C66D2DB43210, 0xB00327C898FB213F, 0xBF597FC7BEEF0EE4, 0xC6E00BF33DA88FC2, 0xD5A79147930AA725, 0x06CA6351E003826F, 0x142929670A0E6E70, 0x27B70A8546D22FFC, 0x2E1B21385C26C926, 0x4D2C6DFC5AC42AED, 0x53380D139D95B3DF, 0x650A73548BAF63DE, 0x766A0ABB3C77B2A8, 0x81C2C92E47EDAEE6, 0x92722C851482353B, 0xA2BFE8A14CF10364, 0xA81A664BBC423001, 0xC24B8B70D0F89791, 0xC76C51A30654BE30, 0xD192E819D6EF5218, 0xD69906245565A910, 0xF40E35855771202A, 0x106AA07032BBD1B8, 0x19A4C116B8D2D0C8, 0x1E376C085141AB53, 0x2748774CDF8EEB99, 0x34B0BCB5E19B48A8, 0x391C0CB3C5C95A63, 0x4ED8AA4AE3418ACB, 0x5B9CCA4F7763E373, 0x682E6FF3D6B2B8A3, 0x748F82EE5DEFB2FC, 0x78A5636F43172F60, 0x84C87814A1F0AB72, 0x8CC702081A6439EC, 0x90BEFFFA23631E28, 0xA4506CEBDE82BDE9, 0xBEF9A3F7B2C67915, 0xC67178F2E372532B, 0xCA273ECEEA26619C, 0xD186B8C721C0C207, 0xEADA7DD6CDE0EB1E, 0xF57D4F7FEE6ED178, 0x06F067AA72176FBA, 0x0A637DC5A2C898A6, 0x113F9804BEF90DAE, 0x1B710B35131C471B, 0x28DB77F523047D84, 0x32CAAB7B40C72493, 0x3C9EBE0A15C9BEBC, 0x431D67C49C100D4C, 0x4CC5D4BECB3E42B6, 0x597F299CFC657E2A, 0x5FCB6FAB3AD6FAEC, 0x6C44198C4A475817
    };

    return table[round];
}

static uint32_t sha2_narrow_word_ld(const uint8_t *src)
{
    uint32_t word = 0;
    uint32_t idx = 0;

    while (idx < SHA2_NARROW_WORD_SIZE)
    {
        word = (word << SHA2_BYTE_BITS) | (uint32_t)src[idx];
        idx++;
    }

    return word;
}

static uint64_t sha2_wide_word_ld(const uint8_t *src)
{
    uint64_t word = 0;
    uint32_t idx = 0;

    while (idx < SHA2_WIDE_WORD_SIZE)
    {
        word = (word << SHA2_BYTE_BITS) | (uint64_t)src[idx];
        idx++;
    }

    return word;
}

static void sha2_wide_word_st(uint8_t *dst, uint64_t word, uint32_t size)
{
    uint32_t idx = size;

    while (idx > 0)
    {
        idx--;
        dst[idx] = (uint8_t)(word & SHA2_BYTE_MASK);
        word >>= SHA2_BYTE_BITS;
    }
}

static uint32_t sha2_narrow_sched_get(uint32_t *sched, const uint8_t *blk, uint32_t round)
{
    uint32_t slot = round & SHA2_SCHED_MASK;
    uint32_t near = 0;
    uint32_t far = 0;

    if (round < SHA2_SCHED_CUNT)
    {
        sched[slot] = sha2_narrow_word_ld(blk + (round * SHA2_NARROW_WORD_SIZE));
        return sched[slot];
    }

    near = sched[(round + SHA2_SCHED_NEAR_OFF) & SHA2_SCHED_MASK];
    far = sched[(round + SHA2_SCHED_FAR_OFF) & SHA2_SCHED_MASK];
    near = sha2_narrow_rotr_get(near, SHA2_NARROW_SMALL_NEAR_FIRST) ^ sha2_narrow_rotr_get(near, SHA2_NARROW_SMALL_NEAR_SECOND) ^ (near >> SHA2_NARROW_SMALL_NEAR_SHIFT);
    far = sha2_narrow_rotr_get(far, SHA2_NARROW_SMALL_FAR_FIRST) ^ sha2_narrow_rotr_get(far, SHA2_NARROW_SMALL_FAR_SECOND) ^ (far >> SHA2_NARROW_SMALL_FAR_SHIFT);
    sched[slot] = sched[slot] + near + sched[(round + SHA2_SCHED_MID_OFF) & SHA2_SCHED_MASK] + far;

    return sched[slot];
}

static uint64_t sha2_wide_sched_get(uint64_t *sched, const uint8_t *blk, uint32_t round)
{
    uint32_t slot = round & SHA2_SCHED_MASK;
    uint64_t near = 0;
    uint64_t far = 0;

    if (round < SHA2_SCHED_CUNT)
    {
        sched[slot] = sha2_wide_word_ld(blk + (round * SHA2_WIDE_WORD_SIZE));
        return sched[slot];
    }

    near = sched[(round + SHA2_SCHED_NEAR_OFF) & SHA2_SCHED_MASK];
    far = sched[(round + SHA2_SCHED_FAR_OFF) & SHA2_SCHED_MASK];
    near = sha2_wide_rotr_get(near, SHA2_WIDE_SMALL_NEAR_FIRST) ^ sha2_wide_rotr_get(near, SHA2_WIDE_SMALL_NEAR_SECOND) ^ (near >> SHA2_WIDE_SMALL_NEAR_SHIFT);
    far = sha2_wide_rotr_get(far, SHA2_WIDE_SMALL_FAR_FIRST) ^ sha2_wide_rotr_get(far, SHA2_WIDE_SMALL_FAR_SECOND) ^ (far >> SHA2_WIDE_SMALL_FAR_SHIFT);
    sched[slot] = sched[slot] + near + sched[(round + SHA2_SCHED_MID_OFF) & SHA2_SCHED_MASK] + far;

    return sched[slot];
}

static void sha2_narrow_compress(uint32_t *lane, const uint8_t *blk)
{
    uint32_t sched[SHA2_SCHED_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t work[LDG_MATH_CRYPTO_SHA_LANE_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t lhs = 0;
    uint32_t rhs = 0;
    uint32_t round = 0;
    uint32_t idx = 0;

    while (idx < LDG_MATH_CRYPTO_SHA_LANE_CUNT)
    {
        work[idx] = lane[idx];
        idx++;
    }

    while (round < SHA2_NARROW_ROUND_CUNT)
    {
        lhs = work[7] + (sha2_narrow_rotr_get(work[4], SHA2_NARROW_BIG_E_FIRST) ^ sha2_narrow_rotr_get(work[4], SHA2_NARROW_BIG_E_SECOND) ^ sha2_narrow_rotr_get(work[4], SHA2_NARROW_BIG_E_THIRD));
        lhs = lhs + ((work[4] & work[5]) ^ (~work[4] & work[6])) + sha2_narrow_const_get(round) + sha2_narrow_sched_get(sched, blk, round);
        rhs = (sha2_narrow_rotr_get(work[0], SHA2_NARROW_BIG_A_FIRST) ^ sha2_narrow_rotr_get(work[0], SHA2_NARROW_BIG_A_SECOND) ^ sha2_narrow_rotr_get(work[0], SHA2_NARROW_BIG_A_THIRD));
        rhs = rhs + ((work[0] & work[1]) ^ (work[0] & work[2]) ^ (work[1] & work[2]));
        idx = SHA2_WORK_LAST;

        while (idx > 0)
        {
            work[idx] = work[idx - SHA2_ADJACENT_OFF];
            idx--;
        }

        work[SHA2_WORK_MID] = work[SHA2_WORK_MID] + lhs;
        work[0] = lhs + rhs;
        round++;
    }

    idx = 0;

    while (idx < LDG_MATH_CRYPTO_SHA_LANE_CUNT)
    {
        lane[idx] = lane[idx] + work[idx];
        idx++;
    }
}

static void sha2_wide_compress(uint64_t *lane, const uint8_t *blk)
{
    uint64_t sched[SHA2_SCHED_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t work[LDG_MATH_CRYPTO_SHA_LANE_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t lhs = 0;
    uint64_t rhs = 0;
    uint32_t round = 0;
    uint32_t idx = 0;

    while (idx < LDG_MATH_CRYPTO_SHA_LANE_CUNT)
    {
        work[idx] = lane[idx];
        idx++;
    }

    while (round < SHA2_WIDE_ROUND_CUNT)
    {
        lhs = work[7] + (sha2_wide_rotr_get(work[4], SHA2_WIDE_BIG_E_FIRST) ^ sha2_wide_rotr_get(work[4], SHA2_WIDE_BIG_E_SECOND) ^ sha2_wide_rotr_get(work[4], SHA2_WIDE_BIG_E_THIRD));
        lhs = lhs + ((work[4] & work[5]) ^ (~work[4] & work[6])) + sha2_wide_const_get(round) + sha2_wide_sched_get(sched, blk, round);
        rhs = (sha2_wide_rotr_get(work[0], SHA2_WIDE_BIG_A_FIRST) ^ sha2_wide_rotr_get(work[0], SHA2_WIDE_BIG_A_SECOND) ^ sha2_wide_rotr_get(work[0], SHA2_WIDE_BIG_A_THIRD));
        rhs = rhs + ((work[0] & work[1]) ^ (work[0] & work[2]) ^ (work[1] & work[2]));
        idx = SHA2_WORK_LAST;

        while (idx > 0)
        {
            work[idx] = work[idx - SHA2_ADJACENT_OFF];
            idx--;
        }

        work[SHA2_WORK_MID] = work[SHA2_WORK_MID] + lhs;
        work[0] = lhs + rhs;
        round++;
    }

    idx = 0;

    while (idx < LDG_MATH_CRYPTO_SHA_LANE_CUNT)
    {
        lane[idx] = lane[idx] + work[idx];
        idx++;
    }
}

static void sha2_wide_lane_fill(ldg_math_crypto_sha512_t *ctx, const uint64_t *seed, uint32_t out_size)
{
    uint32_t idx = 0;

    while (idx < LDG_MATH_CRYPTO_SHA_LANE_CUNT)
    {
        ctx->lane[idx] = seed[idx];
        idx++;
    }

    ctx->len_lo = 0;
    ctx->len_hi = 0;
    ctx->blk_len = 0;
    ctx->out_size = out_size;
}

uint32_t ldg_math_crypto_sha256_init(ldg_math_crypto_sha256_t *ctx)
{
    const uint32_t seed[LDG_MATH_CRYPTO_SHA_LANE_CUNT] = { 0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A, 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19 };
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (idx < LDG_MATH_CRYPTO_SHA_LANE_CUNT)
    {
        ctx->lane[idx] = seed[idx];
        idx++;
    }

    ctx->len = 0;
    ctx->blk_len = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_crypto_sha256_update(ldg_math_crypto_sha256_t *ctx, const uint8_t *src, uint64_t len)
{
    uint64_t done = 0;

    if (LDG_UNLIKELY(!ctx || (!src && len != 0))) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->blk_len >= LDG_MATH_CRYPTO_SHA256_BLK_SIZE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (done < len)
    {
        ctx->blk[ctx->blk_len] = src[done];
        ctx->blk_len++;
        done++;

        if (ctx->blk_len == LDG_MATH_CRYPTO_SHA256_BLK_SIZE)
        {
            sha2_narrow_compress(ctx->lane, ctx->blk);
            ctx->blk_len = 0;
        }
    }

    ctx->len += len;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_crypto_sha256_finish(ldg_math_crypto_sha256_t *ctx, uint8_t *out)
{
    uint64_t bit_cunt = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->blk_len >= LDG_MATH_CRYPTO_SHA256_BLK_SIZE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    bit_cunt = ctx->len << SHA2_BIT_SHIFT;
    ctx->blk[ctx->blk_len] = SHA2_PAD_MARK;
    ctx->blk_len++;

    while (ctx->blk_len != (LDG_MATH_CRYPTO_SHA256_BLK_SIZE - SHA2_NARROW_LEN_SIZE))
    {
        if (ctx->blk_len == LDG_MATH_CRYPTO_SHA256_BLK_SIZE)
        {
            sha2_narrow_compress(ctx->lane, ctx->blk);
            ctx->blk_len = 0;
        }
        else
        {
            ctx->blk[ctx->blk_len] = 0;
            ctx->blk_len++;
        }
    }

    sha2_wide_word_st(ctx->blk + ctx->blk_len, bit_cunt, SHA2_NARROW_LEN_SIZE);
    sha2_narrow_compress(ctx->lane, ctx->blk);

    while (idx < LDG_MATH_CRYPTO_SHA_LANE_CUNT)
    {
        sha2_wide_word_st(out + (idx * SHA2_NARROW_WORD_SIZE), (uint64_t)ctx->lane[idx], SHA2_NARROW_WORD_SIZE);
        idx++;
    }

    return ldg_mem_zero(ctx, sizeof(*ctx));
}

uint32_t ldg_math_crypto_sha256_compute(const uint8_t *src, uint64_t len, uint8_t *out)
{
    ldg_math_crypto_sha256_t ctx = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = ldg_math_crypto_sha256_init(&ctx);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_crypto_sha256_update(&ctx, src, len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_crypto_sha256_finish(&ctx, out);
}

uint32_t ldg_math_crypto_sha384_init(ldg_math_crypto_sha512_t *ctx)
{
    const uint64_t seed[LDG_MATH_CRYPTO_SHA_LANE_CUNT] = { 0xCBBB9D5DC1059ED8, 0x629A292A367CD507, 0x9159015A3070DD17, 0x152FECD8F70E5939, 0x67332667FFC00B31, 0x8EB44A8768581511, 0xDB0C2E0D64F98FA7, 0x47B5481DBEFA4FA4 };

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    sha2_wide_lane_fill(ctx, seed, LDG_MATH_CRYPTO_SHA384_OUT_SIZE);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_crypto_sha512_init(ldg_math_crypto_sha512_t *ctx)
{
    const uint64_t seed[LDG_MATH_CRYPTO_SHA_LANE_CUNT] = { 0x6A09E667F3BCC908, 0xBB67AE8584CAA73B, 0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1, 0x510E527FADE682D1, 0x9B05688C2B3E6C1F, 0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179 };

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    sha2_wide_lane_fill(ctx, seed, LDG_MATH_CRYPTO_SHA512_OUT_SIZE);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_crypto_sha512_update(ldg_math_crypto_sha512_t *ctx, const uint8_t *src, uint64_t len)
{
    uint64_t done = 0;

    if (LDG_UNLIKELY(!ctx || (!src && len != 0))) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->blk_len >= LDG_MATH_CRYPTO_SHA512_BLK_SIZE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (done < len)
    {
        ctx->blk[ctx->blk_len] = src[done];
        ctx->blk_len++;
        done++;

        if (ctx->blk_len == LDG_MATH_CRYPTO_SHA512_BLK_SIZE)
        {
            sha2_wide_compress(ctx->lane, ctx->blk);
            ctx->blk_len = 0;
        }
    }

    ctx->len_lo += len;

    if (ctx->len_lo < len) { ctx->len_hi++; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_crypto_sha512_finish(ldg_math_crypto_sha512_t *ctx, uint8_t *out)
{
    uint8_t tail[LDG_MATH_CRYPTO_SHA512_OUT_SIZE] = LDG_ARR_ZERO_INIT;
    uint64_t bit_hi = 0;
    uint64_t bit_lo = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->blk_len >= LDG_MATH_CRYPTO_SHA512_BLK_SIZE || ctx->out_size > LDG_MATH_CRYPTO_SHA512_OUT_SIZE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    bit_hi = (ctx->len_hi << SHA2_BIT_SHIFT) | (ctx->len_lo >> SHA2_BIT_CARRY_SHIFT);
    bit_lo = ctx->len_lo << SHA2_BIT_SHIFT;
    ctx->blk[ctx->blk_len] = SHA2_PAD_MARK;
    ctx->blk_len++;

    while (ctx->blk_len != (LDG_MATH_CRYPTO_SHA512_BLK_SIZE - SHA2_WIDE_LEN_SIZE))
    {
        if (ctx->blk_len == LDG_MATH_CRYPTO_SHA512_BLK_SIZE)
        {
            sha2_wide_compress(ctx->lane, ctx->blk);
            ctx->blk_len = 0;
        }
        else
        {
            ctx->blk[ctx->blk_len] = 0;
            ctx->blk_len++;
        }
    }

    sha2_wide_word_st(ctx->blk + ctx->blk_len, bit_hi, SHA2_WIDE_WORD_SIZE);
    sha2_wide_word_st(ctx->blk + ctx->blk_len + SHA2_WIDE_WORD_SIZE, bit_lo, SHA2_WIDE_WORD_SIZE);
    sha2_wide_compress(ctx->lane, ctx->blk);

    while (idx < LDG_MATH_CRYPTO_SHA_LANE_CUNT)
    {
        sha2_wide_word_st(tail + (idx * SHA2_WIDE_WORD_SIZE), ctx->lane[idx], SHA2_WIDE_WORD_SIZE);
        idx++;
    }

    idx = 0;

    while (idx < ctx->out_size)
    {
        out[idx] = tail[idx];
        idx++;
    }

    return ldg_mem_zero(ctx, sizeof(*ctx));
}

uint32_t ldg_math_crypto_sha384_compute(const uint8_t *src, uint64_t len, uint8_t *out)
{
    ldg_math_crypto_sha512_t ctx = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = ldg_math_crypto_sha384_init(&ctx);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_crypto_sha512_update(&ctx, src, len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_crypto_sha512_finish(&ctx, out);
}

uint32_t ldg_math_crypto_sha512_compute(const uint8_t *src, uint64_t len, uint8_t *out)
{
    ldg_math_crypto_sha512_t ctx = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = ldg_math_crypto_sha512_init(&ctx);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_crypto_sha512_update(&ctx, src, len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_crypto_sha512_finish(&ctx, out);
}
