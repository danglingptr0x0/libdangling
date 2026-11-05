#include <stdint.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/hash/blake2b.h>
#include <dangling/math/field.h>
#include <dangling/math/transcript.h>

#define LDG_MATH_TRANSCRIPT_WORD_SIZE 8
#define LDG_MATH_TRANSCRIPT_GRIND_ATTEMPT_MAX 0x0000001000000000ULL
#define LDG_MATH_TRANSCRIPT_DRAW_ATTEMPT_MAX 64
#define LDG_MATH_TRANSCRIPT_BYTE_TOP 0x0000000000000080ULL
#define LDG_MATH_TRANSCRIPT_PREV_OFF 1

static void transcript_word_st(uint64_t val, uint8_t *out)
{
    uint32_t idx = 0;

    for (idx = 0; idx < LDG_MATH_TRANSCRIPT_WORD_SIZE; idx++) { out[idx] = (uint8_t)((val >> (idx * LDG_BYTE_BITS)) & LDG_BYTE_MASK); }
}

static uint64_t transcript_word_ld(const uint8_t *src)
{
    uint64_t val = 0;
    uint32_t idx = 0;

    for (idx = 0; idx < LDG_MATH_TRANSCRIPT_WORD_SIZE; idx++) { val |= (uint64_t)src[idx] << (idx * LDG_BYTE_BITS); }

    return val;
}

static uint32_t transcript_squeeze(const ldg_math_transcript_t *ctx, uint64_t tag, uint8_t *out)
{
    ldg_hash_blake2b_t hasher = LDG_STRUCT_ZERO_INIT;
    uint8_t tag_buff[LDG_MATH_TRANSCRIPT_WORD_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = LDG_ERR_AOK;

    transcript_word_st(tag, tag_buff);

    ret = ldg_hash_blake2b_init(&hasher);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_hash_blake2b_update(&hasher, ctx->state, LDG_MATH_TRANSCRIPT_STATE_SIZE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_hash_blake2b_update(&hasher, tag_buff, LDG_MATH_TRANSCRIPT_WORD_SIZE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_hash_blake2b_finish(&hasher, out);
}

static uint32_t transcript_lead_zero_cunt_get(const uint8_t *digest)
{
    uint32_t cunt = 0;
    uint32_t idx = 0;
    uint32_t bit = 0;
    uint8_t curr = 0;

    for (idx = 0; idx < LDG_HASH_BLAKE2B_OUT_SIZE; idx++)
    {
        curr = digest[idx];

        if (curr != 0) { for (bit = 0; bit < LDG_BYTE_BITS; bit++)
            {
                if ((curr & (uint8_t)LDG_MATH_TRANSCRIPT_BYTE_TOP) != 0) { return cunt; }

                curr = (uint8_t)(curr << LDG_MATH_TRANSCRIPT_PREV_OFF);
                cunt++;
            }
        }

        cunt += LDG_BYTE_BITS;
    }

    return cunt;
}

uint32_t ldg_math_transcript_init(ldg_math_transcript_t *ctx, const uint8_t *seed, uint64_t seed_len)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    ctx->cuntr = 0;

    return ldg_hash_blake2b_compute(seed, seed_len, ctx->state);
}

uint32_t ldg_math_transcript_absorb(ldg_math_transcript_t *ctx, const uint8_t *src, uint64_t len)
{
    ldg_hash_blake2b_t hasher = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_hash_blake2b_init(&hasher);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_hash_blake2b_update(&hasher, ctx->state, LDG_MATH_TRANSCRIPT_STATE_SIZE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_hash_blake2b_update(&hasher, src, len);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_hash_blake2b_finish(&hasher, ctx->state);
}

uint32_t ldg_math_transcript_elem_absorb(ldg_math_transcript_t *ctx, const ldg_math_field_elem_t *src, uint64_t cunt)
{
    ldg_hash_blake2b_t hasher = LDG_STRUCT_ZERO_INIT;
    uint8_t word_buff[LDG_MATH_TRANSCRIPT_WORD_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = LDG_ERR_AOK;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt > 0 && !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_hash_blake2b_init(&hasher);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_hash_blake2b_update(&hasher, ctx->state, LDG_MATH_TRANSCRIPT_STATE_SIZE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < cunt; idx++)
    {
        transcript_word_st(ldg_math_field_reduce(src[idx]), word_buff);
        ret = ldg_hash_blake2b_update(&hasher, word_buff, LDG_MATH_TRANSCRIPT_WORD_SIZE);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return ldg_hash_blake2b_finish(&hasher, ctx->state);
}

uint32_t ldg_math_transcript_elem_get(ldg_math_transcript_t *ctx, ldg_math_field_elem_t *out)
{
    uint8_t digest[LDG_HASH_BLAKE2B_OUT_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = LDG_ERR_AOK;
    uint32_t attempt = 0;
    uint64_t draw = 0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    for (attempt = 0; attempt < LDG_MATH_TRANSCRIPT_DRAW_ATTEMPT_MAX; attempt++)
    {
        ret = transcript_squeeze(ctx, ctx->cuntr, digest);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->cuntr++;
        draw = transcript_word_ld(digest);

        if (ldg_math_field_canonical_is(draw) == LDG_TRUTH_TRUE)
        {
            *out = draw;

            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_math_transcript_ext_absorb(ldg_math_transcript_t *ctx, const ldg_math_ext_t *src, uint64_t cunt)
{
    ldg_hash_blake2b_t hasher = LDG_STRUCT_ZERO_INIT;
    uint8_t word_buff[LDG_MATH_TRANSCRIPT_WORD_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = LDG_ERR_AOK;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt > 0 && !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_hash_blake2b_init(&hasher);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_hash_blake2b_update(&hasher, ctx->state, LDG_MATH_TRANSCRIPT_STATE_SIZE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < cunt; idx++)
    {
        transcript_word_st(ldg_math_field_reduce(src[idx].lo), word_buff);
        ret = ldg_hash_blake2b_update(&hasher, word_buff, LDG_MATH_TRANSCRIPT_WORD_SIZE);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        transcript_word_st(ldg_math_field_reduce(src[idx].hi), word_buff);
        ret = ldg_hash_blake2b_update(&hasher, word_buff, LDG_MATH_TRANSCRIPT_WORD_SIZE);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return ldg_hash_blake2b_finish(&hasher, ctx->state);
}

uint32_t ldg_math_transcript_ext_get(ldg_math_transcript_t *ctx, ldg_math_ext_t *out)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->lo = LDG_MATH_FIELD_ADD_IDENT;
    out->hi = LDG_MATH_FIELD_ADD_IDENT;

    ret = ldg_math_transcript_elem_get(ctx, &out->lo);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_transcript_elem_get(ctx, &out->hi);
}

uint32_t ldg_math_transcript_idx_get(ldg_math_transcript_t *ctx, uint64_t bound, uint64_t *out)
{
    uint8_t digest[LDG_HASH_BLAKE2B_OUT_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = LDG_ERR_AOK;
    uint64_t draw = 0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(bound == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(!LDG_IS_POW2(bound))) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = transcript_squeeze(ctx, ctx->cuntr, digest);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->cuntr++;
    draw = transcript_word_ld(digest);
    *out = draw & (bound - LDG_MATH_TRANSCRIPT_PREV_OFF);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_transcript_grind_find(ldg_math_transcript_t *ctx, uint32_t bit_cunt, uint64_t *out_nonce)
{
    uint8_t digest[LDG_HASH_BLAKE2B_OUT_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = LDG_ERR_AOK;
    uint64_t nonce = 0;

    if (LDG_UNLIKELY(!ctx || !out_nonce)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_nonce = 0;

    if (LDG_UNLIKELY(bit_cunt > LDG_MATH_TRANSCRIPT_GRIND_BIT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (nonce = 0; nonce < LDG_MATH_TRANSCRIPT_GRIND_ATTEMPT_MAX; nonce++)
    {
        ret = transcript_squeeze(ctx, nonce, digest);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (transcript_lead_zero_cunt_get(digest) >= bit_cunt)
        {
            *out_nonce = nonce;

            return LDG_ERR_AOK;
        }
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_math_transcript_grind_ck(ldg_math_transcript_t *ctx, uint32_t bit_cunt, uint64_t nonce)
{
    uint8_t digest[LDG_HASH_BLAKE2B_OUT_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bit_cunt > LDG_MATH_TRANSCRIPT_GRIND_BIT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = transcript_squeeze(ctx, nonce, digest);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (transcript_lead_zero_cunt_get(digest) < bit_cunt) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}
