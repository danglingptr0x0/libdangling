#include <stdint.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/hash/blake2b.h>

#define LDG_HASH_BLAKE2B_WORD_BITS 64
#define LDG_HASH_BLAKE2B_WORD_SIZE 8
#define LDG_HASH_BLAKE2B_MSG_CUNT 16
#define LDG_HASH_BLAKE2B_WORK_CUNT 16
#define LDG_HASH_BLAKE2B_ROUND_CUNT 12
#define LDG_HASH_BLAKE2B_QUARTER_CUNT 4
#define LDG_HASH_BLAKE2B_GROUP_C 2
#define LDG_HASH_BLAKE2B_GROUP_D 3
#define LDG_HASH_BLAKE2B_DIAG_OFF_B 1
#define LDG_HASH_BLAKE2B_DIAG_OFF_C 2
#define LDG_HASH_BLAKE2B_DIAG_OFF_D 3
#define LDG_HASH_BLAKE2B_PAIR_STEP 2
#define LDG_HASH_BLAKE2B_PREV_OFF 1
#define LDG_HASH_BLAKE2B_ROT_FIRST 32
#define LDG_HASH_BLAKE2B_ROT_SECOND 24
#define LDG_HASH_BLAKE2B_ROT_THIRD 16
#define LDG_HASH_BLAKE2B_ROT_FOURTH 63
#define LDG_HASH_BLAKE2B_PARAM_BASE 0x0000000001010000ULL
#define LDG_HASH_BLAKE2B_FINAL_MASK 0xFFFFFFFFFFFFFFFFULL
#define LDG_HASH_BLAKE2B_CUNTR_LANE 12
#define LDG_HASH_BLAKE2B_CUNTR_HI_LANE 13
#define LDG_HASH_BLAKE2B_FINAL_LANE 14

#define LDG_HASH_BLAKE2B_IV_A 0x6A09E667F3BCC908ULL
#define LDG_HASH_BLAKE2B_IV_B 0xBB67AE8584CAA73BULL
#define LDG_HASH_BLAKE2B_IV_C 0x3C6EF372FE94F82BULL
#define LDG_HASH_BLAKE2B_IV_D 0xA54FF53A5F1D36F1ULL
#define LDG_HASH_BLAKE2B_IV_E 0x510E527FADE682D1ULL
#define LDG_HASH_BLAKE2B_IV_F 0x9B05688C2B3E6C1FULL
#define LDG_HASH_BLAKE2B_IV_G 0x1F83D9ABFB41BD6BULL
#define LDG_HASH_BLAKE2B_IV_H 0x5BE0CD19137E2179ULL

#define LDG_HASH_BLAKE2B_SIGMA_A 0xFEDCBA9876543210ULL
#define LDG_HASH_BLAKE2B_SIGMA_B 0x357B20C16DF984AEULL
#define LDG_HASH_BLAKE2B_SIGMA_C 0x491763EADF250C8BULL
#define LDG_HASH_BLAKE2B_SIGMA_D 0x8F04A562EBCD1397ULL
#define LDG_HASH_BLAKE2B_SIGMA_E 0xD386CB1EFA427509ULL
#define LDG_HASH_BLAKE2B_SIGMA_F 0x91EF57D438B0A6C2ULL
#define LDG_HASH_BLAKE2B_SIGMA_G 0xB8293670A4DEF15CULL
#define LDG_HASH_BLAKE2B_SIGMA_H 0xA2684F05931CE7BDULL
#define LDG_HASH_BLAKE2B_SIGMA_I 0x5A417D2C803B9EF6ULL
#define LDG_HASH_BLAKE2B_SIGMA_J 0x0DC3E9BF5167482AULL

static uint64_t blake2b_rotr_get(uint64_t val, uint32_t cunt)
{
    return (val >> cunt) | (val << (LDG_HASH_BLAKE2B_WORD_BITS - cunt));
}

static uint32_t blake2b_sigma_get(uint64_t row, uint32_t slot)
{
    return (uint32_t)((row >> (slot * LDG_NIBBLE_BITS)) & LDG_NIBBLE_MASK);
}

static void blake2b_mix(uint64_t *work, uint32_t ida, uint32_t idb, uint32_t idc, uint32_t idd, uint64_t lhs, uint64_t rhs)
{
    work[ida] = work[ida] + work[idb] + lhs;
    work[idd] = blake2b_rotr_get(work[idd] ^ work[ida], LDG_HASH_BLAKE2B_ROT_FIRST);
    work[idc] = work[idc] + work[idd];
    work[idb] = blake2b_rotr_get(work[idb] ^ work[idc], LDG_HASH_BLAKE2B_ROT_SECOND);
    work[ida] = work[ida] + work[idb] + rhs;
    work[idd] = blake2b_rotr_get(work[idd] ^ work[ida], LDG_HASH_BLAKE2B_ROT_THIRD);
    work[idc] = work[idc] + work[idd];
    work[idb] = blake2b_rotr_get(work[idb] ^ work[idc], LDG_HASH_BLAKE2B_ROT_FOURTH);
}

static void blake2b_iv_fill(uint64_t *out)
{
    const uint64_t iv[LDG_HASH_BLAKE2B_LANE_CUNT] = {
        LDG_HASH_BLAKE2B_IV_A, LDG_HASH_BLAKE2B_IV_B, LDG_HASH_BLAKE2B_IV_C, LDG_HASH_BLAKE2B_IV_D, LDG_HASH_BLAKE2B_IV_E, LDG_HASH_BLAKE2B_IV_F, LDG_HASH_BLAKE2B_IV_G, LDG_HASH_BLAKE2B_IV_H
    };
    uint32_t idx = 0;

    for (idx = 0; idx < LDG_HASH_BLAKE2B_LANE_CUNT; idx++) { out[idx] = iv[idx]; }
}

static void blake2b_compress(ldg_hash_blake2b_t *ctx, uint8_t final_is)
{
    const uint64_t sigma[LDG_HASH_BLAKE2B_ROUND_CUNT] = {
        LDG_HASH_BLAKE2B_SIGMA_A, LDG_HASH_BLAKE2B_SIGMA_B, LDG_HASH_BLAKE2B_SIGMA_C, LDG_HASH_BLAKE2B_SIGMA_D, LDG_HASH_BLAKE2B_SIGMA_E, LDG_HASH_BLAKE2B_SIGMA_F, LDG_HASH_BLAKE2B_SIGMA_G, LDG_HASH_BLAKE2B_SIGMA_H, LDG_HASH_BLAKE2B_SIGMA_I, LDG_HASH_BLAKE2B_SIGMA_J, LDG_HASH_BLAKE2B_SIGMA_A, LDG_HASH_BLAKE2B_SIGMA_B
    };
    uint64_t work[LDG_HASH_BLAKE2B_WORK_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t msg[LDG_HASH_BLAKE2B_MSG_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t bdx = 0;
    uint32_t round = 0;
    uint32_t step = 0;
    uint32_t slot = 0;

    for (idx = 0; idx < LDG_HASH_BLAKE2B_MSG_CUNT; idx++)
    {
        msg[idx] = 0;

        for (bdx = 0; bdx < LDG_HASH_BLAKE2B_WORD_SIZE; bdx++) { msg[idx] |= (uint64_t)ctx->blk[(idx * LDG_HASH_BLAKE2B_WORD_SIZE) + bdx] << (bdx * LDG_BYTE_BITS); }
    }

    for (idx = 0; idx < LDG_HASH_BLAKE2B_LANE_CUNT; idx++) { work[idx] = ctx->lane[idx]; }

    blake2b_iv_fill(work + LDG_HASH_BLAKE2B_LANE_CUNT);

    work[LDG_HASH_BLAKE2B_CUNTR_LANE] ^= ctx->cuntr_lo;
    work[LDG_HASH_BLAKE2B_CUNTR_HI_LANE] ^= ctx->cuntr_hi;

    if (final_is == LDG_TRUTH_TRUE) { work[LDG_HASH_BLAKE2B_FINAL_LANE] ^= LDG_HASH_BLAKE2B_FINAL_MASK; }

    for (round = 0; round < LDG_HASH_BLAKE2B_ROUND_CUNT; round++)
    {
        for (step = 0; step < LDG_HASH_BLAKE2B_QUARTER_CUNT; step++)
        {
            slot = step * LDG_HASH_BLAKE2B_PAIR_STEP;
            blake2b_mix(work, step, step + LDG_HASH_BLAKE2B_QUARTER_CUNT, step + (LDG_HASH_BLAKE2B_QUARTER_CUNT * LDG_HASH_BLAKE2B_GROUP_C), step + (LDG_HASH_BLAKE2B_QUARTER_CUNT * LDG_HASH_BLAKE2B_GROUP_D), msg[blake2b_sigma_get(sigma[round], slot)], msg[blake2b_sigma_get(sigma[round], slot + LDG_HASH_BLAKE2B_PREV_OFF)]);
        }

        for (step = 0; step < LDG_HASH_BLAKE2B_QUARTER_CUNT; step++)
        {
            slot = (LDG_HASH_BLAKE2B_QUARTER_CUNT * LDG_HASH_BLAKE2B_PAIR_STEP) + (step * LDG_HASH_BLAKE2B_PAIR_STEP);
            blake2b_mix(work, step, LDG_HASH_BLAKE2B_QUARTER_CUNT + ((step + LDG_HASH_BLAKE2B_DIAG_OFF_B) % LDG_HASH_BLAKE2B_QUARTER_CUNT), (LDG_HASH_BLAKE2B_QUARTER_CUNT * LDG_HASH_BLAKE2B_GROUP_C) + ((step + LDG_HASH_BLAKE2B_DIAG_OFF_C) % LDG_HASH_BLAKE2B_QUARTER_CUNT), (LDG_HASH_BLAKE2B_QUARTER_CUNT * LDG_HASH_BLAKE2B_GROUP_D) + ((step + LDG_HASH_BLAKE2B_DIAG_OFF_D) % LDG_HASH_BLAKE2B_QUARTER_CUNT), msg[blake2b_sigma_get(sigma[round], slot)], msg[blake2b_sigma_get(sigma[round], slot + LDG_HASH_BLAKE2B_PREV_OFF)]);
        }
    }

    for (idx = 0; idx < LDG_HASH_BLAKE2B_LANE_CUNT; idx++) { ctx->lane[idx] ^= work[idx] ^ work[idx + LDG_HASH_BLAKE2B_LANE_CUNT]; }
}

uint32_t ldg_hash_blake2b_init(ldg_hash_blake2b_t *ctx)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    blake2b_iv_fill(ctx->lane);
    ctx->lane[0] ^= LDG_HASH_BLAKE2B_PARAM_BASE ^ (uint64_t)LDG_HASH_BLAKE2B_OUT_SIZE;
    ctx->cuntr_lo = 0;
    ctx->cuntr_hi = 0;
    ctx->blk_len = 0;

    for (idx = 0; idx < LDG_HASH_BLAKE2B_BLK_SIZE; idx++) { ctx->blk[idx] = 0; }

    return LDG_ERR_AOK;
}

uint32_t ldg_hash_blake2b_update(ldg_hash_blake2b_t *ctx, const uint8_t *src, uint64_t len)
{
    uint64_t taken = 0;
    uint64_t remain = 0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    remain = len;

    while (remain > 0)
    {
        if (ctx->blk_len == LDG_HASH_BLAKE2B_BLK_SIZE)
        {
            ctx->cuntr_lo += LDG_HASH_BLAKE2B_BLK_SIZE;

            if (ctx->cuntr_lo < LDG_HASH_BLAKE2B_BLK_SIZE) { ctx->cuntr_hi++; }

            blake2b_compress(ctx, LDG_TRUTH_FALSE);
            ctx->blk_len = 0;
        }

        taken = LDG_HASH_BLAKE2B_BLK_SIZE - ctx->blk_len;

        if (taken > remain) { taken = remain; }

        for (idx = 0; idx < taken; idx++) { ctx->blk[ctx->blk_len + idx] = src[idx]; }

        ctx->blk_len += (uint32_t)taken;
        src += taken;
        remain -= taken;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_hash_blake2b_finish(ldg_hash_blake2b_t *ctx, uint8_t *out)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->cuntr_lo += ctx->blk_len;

    if (ctx->cuntr_lo < ctx->blk_len) { ctx->cuntr_hi++; }

    for (idx = ctx->blk_len; idx < LDG_HASH_BLAKE2B_BLK_SIZE; idx++) { ctx->blk[idx] = 0; }

    blake2b_compress(ctx, LDG_TRUTH_TRUE);

    for (idx = 0; idx < LDG_HASH_BLAKE2B_OUT_SIZE; idx++) { out[idx] = (uint8_t)((ctx->lane[idx / LDG_HASH_BLAKE2B_WORD_SIZE] >> ((idx % LDG_HASH_BLAKE2B_WORD_SIZE) * LDG_BYTE_BITS)) & LDG_BYTE_MASK); }

    return LDG_ERR_AOK;
}

uint32_t ldg_hash_blake2b_compute(const uint8_t *src, uint64_t len, uint8_t *out)
{
    ldg_hash_blake2b_t ctx = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_hash_blake2b_init(&ctx);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_hash_blake2b_update(&ctx, src, len);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_hash_blake2b_finish(&ctx, out);
}
