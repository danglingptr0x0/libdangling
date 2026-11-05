#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/crypto/chacha.h>
#include <dangling/mem/secure.h>

#define CHACHA_WORD_CUNT 16
#define CHACHA_BLK_SIZE 64
#define CHACHA_WORD_SIZE 4
#define CHACHA_WORD_BITS 32
#define CHACHA_BYTE_BITS 8
#define CHACHA_BYTE_MASK 0xFFU
#define CHACHA_SIGMA_WORD_CUNT 4
#define CHACHA_KEY_WORD_CUNT 8
#define CHACHA_KEY_OFF 4
#define CHACHA_CUNTR_OFF 12
#define CHACHA_NONCE_OFF 13
#define CHACHA_NONCE_WORD_CUNT 3
#define CHACHA_DOUBLE_ROUND_CUNT 10
#define CHACHA_LANE_CUNT 4
#define CHACHA_ROW_SECOND 4
#define CHACHA_ROW_THIRD 8
#define CHACHA_ROW_FOURTH 12
#define CHACHA_DIAG_SECOND 1
#define CHACHA_DIAG_THIRD 2
#define CHACHA_DIAG_FOURTH 3
#define CHACHA_SPIN_WIDE 16
#define CHACHA_SPIN_MID 12
#define CHACHA_SPIN_BYTE 8
#define CHACHA_SPIN_NARROW 7
#define CHACHA_TEXT_CUNTR 0x1U
#define CHACHA_ROTL(val, spin) (((val) << (spin)) | ((val) >> (CHACHA_WORD_BITS - (spin))))
#define POLY_LIMB_CUNT 5
#define POLY_LIMB_BITS 26
#define POLY_LIMB_MASK 0x3FFFFFFU
#define POLY_LIMB_STRIDE 3
#define POLY_LIMB_SHIFT 2
#define POLY_FOLD 5
#define POLY_HIGH_BIT 0x1000000U
#define POLY_HIGH_LIMB 4
#define POLY_BLK_SIZE 16
#define POLY_MASK_WORD_CUNT 4
#define POLY_WORD_SPREAD 6
#define POLY_LEN_SIZE 8
#define POLY_ADJACENT_OFF 1

typedef struct chacha_poly
{
    uint32_t key[POLY_LIMB_CUNT];
    uint32_t scaled[POLY_LIMB_CUNT];
    uint32_t acc[POLY_LIMB_CUNT];
    uint32_t mask[POLY_MASK_WORD_CUNT];
} chacha_poly_t;

static uint32_t chacha_word_ld(const uint8_t *src)
{
    uint32_t word = 0;
    uint32_t idx = 0;

    while (idx < CHACHA_WORD_SIZE)
    {
        word |= (uint32_t)src[idx] << (CHACHA_BYTE_BITS * idx);
        idx++;
    }

    return word;
}

static void chacha_word_st(uint8_t *dst, uint32_t word)
{
    uint32_t idx = 0;

    while (idx < CHACHA_WORD_SIZE)
    {
        dst[idx] = (uint8_t)((word >> (CHACHA_BYTE_BITS * idx)) & CHACHA_BYTE_MASK);
        idx++;
    }
}

static void chacha_quarter_apply(uint32_t *state, uint32_t first, uint32_t second, uint32_t third, uint32_t fourth)
{
    state[first] += state[second];
    state[fourth] ^= state[first];
    state[fourth] = CHACHA_ROTL(state[fourth], CHACHA_SPIN_WIDE);
    state[third] += state[fourth];
    state[second] ^= state[third];
    state[second] = CHACHA_ROTL(state[second], CHACHA_SPIN_MID);
    state[first] += state[second];
    state[fourth] ^= state[first];
    state[fourth] = CHACHA_ROTL(state[fourth], CHACHA_SPIN_BYTE);
    state[third] += state[fourth];
    state[second] ^= state[third];
    state[second] = CHACHA_ROTL(state[second], CHACHA_SPIN_NARROW);
}

static void chacha_double_round_apply(uint32_t *state)
{
    uint32_t lane = 0;

    while (lane < CHACHA_LANE_CUNT)
    {
        chacha_quarter_apply(state, lane, lane + CHACHA_ROW_SECOND, lane + CHACHA_ROW_THIRD, lane + CHACHA_ROW_FOURTH);
        lane++;
    }

    lane = 0;

    while (lane < CHACHA_LANE_CUNT)
    {
        chacha_quarter_apply(state, lane, CHACHA_ROW_SECOND + ((lane + CHACHA_DIAG_SECOND) % CHACHA_LANE_CUNT), CHACHA_ROW_THIRD + ((lane + CHACHA_DIAG_THIRD) % CHACHA_LANE_CUNT), CHACHA_ROW_FOURTH + ((lane + CHACHA_DIAG_FOURTH) % CHACHA_LANE_CUNT));
        lane++;
    }
}

static void chacha_base_set(uint32_t *base, const uint8_t *key, const uint8_t *nonce, uint32_t cuntr)
{
    const char *sigma = "expand 32-byte k";
    uint32_t idx = 0;

    while (idx < CHACHA_SIGMA_WORD_CUNT)
    {
        base[idx] = chacha_word_ld((const uint8_t *)sigma + (CHACHA_WORD_SIZE * idx));
        idx++;
    }

    idx = 0;

    while (idx < CHACHA_KEY_WORD_CUNT)
    {
        base[CHACHA_KEY_OFF + idx] = chacha_word_ld(key + (CHACHA_WORD_SIZE * idx));
        idx++;
    }

    base[CHACHA_CUNTR_OFF] = cuntr;
    idx = 0;

    while (idx < CHACHA_NONCE_WORD_CUNT)
    {
        base[CHACHA_NONCE_OFF + idx] = chacha_word_ld(nonce + (CHACHA_WORD_SIZE * idx));
        idx++;
    }
}

static uint32_t chacha_blk_fill(const uint32_t *base, uint8_t *out)
{
    uint32_t state[CHACHA_WORD_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;

    while (idx < CHACHA_WORD_CUNT)
    {
        state[idx] = base[idx];
        idx++;
    }

    idx = 0;

    while (idx < CHACHA_DOUBLE_ROUND_CUNT)
    {
        chacha_double_round_apply(state);
        idx++;
    }

    idx = 0;

    while (idx < CHACHA_WORD_CUNT)
    {
        chacha_word_st(out + (CHACHA_WORD_SIZE * idx), state[idx] + base[idx]);
        idx++;
    }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(state, sizeof(state)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t chacha_stream_apply(uint32_t *base, const uint8_t *src, uint8_t *dst, uint32_t len)
{
    uint8_t blk[CHACHA_BLK_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t done = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    while (done < len && ret == LDG_ERR_AOK)
    {
        ret = chacha_blk_fill(base, blk);
        idx = 0;

        while (ret == LDG_ERR_AOK && idx < CHACHA_BLK_SIZE && done < len)
        {
            dst[done] = (uint8_t)(src[done] ^ blk[idx]);
            done++;
            idx++;
        }

        base[CHACHA_CUNTR_OFF]++;
    }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(blk, sizeof(blk)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    return ret;
}

static void chacha_poly_init(chacha_poly_t *poly, const uint8_t *key)
{
    const uint32_t clamp[POLY_LIMB_CUNT] = { 0x3FFFFFF, 0x3FFFF03, 0x3FFC0FF, 0x3F03FFF, 0x00FFFFF };
    uint32_t idx = 0;

    while (idx < POLY_LIMB_CUNT)
    {
        poly->key[idx] = (chacha_word_ld(key + (POLY_LIMB_STRIDE * idx)) >> (POLY_LIMB_SHIFT * idx)) & clamp[idx];
        poly->scaled[idx] = poly->key[idx] * POLY_FOLD;
        poly->acc[idx] = 0;
        idx++;
    }

    idx = 0;

    while (idx < POLY_MASK_WORD_CUNT)
    {
        poly->mask[idx] = chacha_word_ld(key + POLY_BLK_SIZE + (CHACHA_WORD_SIZE * idx));
        idx++;
    }
}

static uint64_t chacha_poly_row_get(const chacha_poly_t *poly, uint32_t row)
{
    uint64_t cum = 0;
    uint32_t col = 0;

    while (col < POLY_LIMB_CUNT)
    {
        cum += (uint64_t)poly->acc[col] * (uint64_t)((col <= row) ? poly->key[row - col] : poly->scaled[POLY_LIMB_CUNT + row - col]);
        col++;
    }

    return cum;
}

static void chacha_poly_blk_cum(chacha_poly_t *poly, const uint8_t *blk)
{
    uint64_t wide[POLY_LIMB_CUNT] = LDG_ARR_ZERO_INIT;
    uint64_t carry = 0;
    uint32_t idx = 0;

    while (idx < POLY_LIMB_CUNT)
    {
        poly->acc[idx] += (chacha_word_ld(blk + (POLY_LIMB_STRIDE * idx)) >> (POLY_LIMB_SHIFT * idx)) & POLY_LIMB_MASK;
        idx++;
    }

    poly->acc[POLY_HIGH_LIMB] += POLY_HIGH_BIT;
    idx = 0;

    while (idx < POLY_LIMB_CUNT)
    {
        wide[idx] = chacha_poly_row_get(poly, idx);
        idx++;
    }

    idx = 0;

    while (idx < POLY_LIMB_CUNT)
    {
        wide[idx] += carry;
        poly->acc[idx] = (uint32_t)(wide[idx] & POLY_LIMB_MASK);
        carry = wide[idx] >> POLY_LIMB_BITS;
        idx++;
    }

    carry = (carry * POLY_FOLD) + poly->acc[0];
    poly->acc[0] = (uint32_t)(carry & POLY_LIMB_MASK);
    poly->acc[POLY_ADJACENT_OFF] += (uint32_t)(carry >> POLY_LIMB_BITS);
}

static void chacha_poly_span_cum(chacha_poly_t *poly, const uint8_t *src, uint32_t len)
{
    uint8_t blk[POLY_BLK_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t done = 0;
    uint32_t idx = 0;

    while ((len - done) >= POLY_BLK_SIZE)
    {
        chacha_poly_blk_cum(poly, src + done);
        done += POLY_BLK_SIZE;
    }

    if (done == len) { return; }

    while (done < len)
    {
        blk[idx] = src[done];
        done++;
        idx++;
    }

    chacha_poly_blk_cum(poly, blk);
}

static void chacha_poly_len_cum(chacha_poly_t *poly, uint32_t extra_len, uint32_t len)
{
    uint8_t blk[POLY_BLK_SIZE] = LDG_ARR_ZERO_INIT;

    chacha_word_st(blk, extra_len);
    chacha_word_st(blk + POLY_LEN_SIZE, len);
    chacha_poly_blk_cum(poly, blk);
}

static void chacha_poly_reduce(chacha_poly_t *poly)
{
    uint32_t trial[POLY_LIMB_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t carry = 0;
    uint32_t sel = 0;
    uint32_t idx = POLY_ADJACENT_OFF;

    while (idx < POLY_LIMB_CUNT)
    {
        poly->acc[idx] += carry;
        carry = poly->acc[idx] >> POLY_LIMB_BITS;
        poly->acc[idx] &= POLY_LIMB_MASK;
        idx++;
    }

    poly->acc[0] += carry * POLY_FOLD;
    carry = poly->acc[0] >> POLY_LIMB_BITS;
    poly->acc[0] &= POLY_LIMB_MASK;
    poly->acc[POLY_ADJACENT_OFF] += carry;

    carry = POLY_FOLD;
    idx = 0;

    while (idx < POLY_LIMB_CUNT)
    {
        trial[idx] = poly->acc[idx] + carry;
        carry = trial[idx] >> POLY_LIMB_BITS;
        trial[idx] &= POLY_LIMB_MASK;
        idx++;
    }

    sel = ~carry + POLY_ADJACENT_OFF;
    idx = 0;

    while (idx < POLY_LIMB_CUNT)
    {
        poly->acc[idx] = (poly->acc[idx] & ~sel) | (trial[idx] & sel);
        idx++;
    }
}

static void chacha_poly_finish(chacha_poly_t *poly, uint8_t *tag)
{
    uint64_t sum = 0;
    uint32_t word = 0;
    uint32_t idx = 0;

    chacha_poly_reduce(poly);

    while (idx < POLY_MASK_WORD_CUNT)
    {
        word = (poly->acc[idx] >> (POLY_WORD_SPREAD * idx)) | (poly->acc[idx + POLY_ADJACENT_OFF] << (POLY_LIMB_BITS - (POLY_WORD_SPREAD * idx)));
        sum = (uint64_t)word + (uint64_t)poly->mask[idx] + (sum >> CHACHA_WORD_BITS);
        chacha_word_st(tag + (CHACHA_WORD_SIZE * idx), (uint32_t)sum);
        idx++;
    }
}

static uint32_t chacha_tag_compute(uint32_t *base, const uint8_t *extra, uint32_t extra_len, const uint8_t *text, uint32_t len, uint8_t *tag)
{
    chacha_poly_t poly = LDG_STRUCT_ZERO_INIT;
    uint8_t blk[CHACHA_BLK_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    base[CHACHA_CUNTR_OFF] = 0;
    ret = chacha_blk_fill(base, blk);

    if (ret == LDG_ERR_AOK)
    {
        chacha_poly_init(&poly, blk);
        chacha_poly_span_cum(&poly, extra, extra_len);
        chacha_poly_span_cum(&poly, text, len);
        chacha_poly_len_cum(&poly, extra_len, len);
        chacha_poly_finish(&poly, tag);
    }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(blk, sizeof(blk)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(&poly, sizeof(poly)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    return ret;
}

uint32_t ldg_math_crypto_chacha20_poly1305_seal(const uint8_t *key, const uint8_t *nonce, const uint8_t *extra, uint32_t extra_len, const uint8_t *src, uint32_t len, uint8_t *dst, uint8_t *tag)
{
    uint32_t base[CHACHA_WORD_CUNT] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!key || !nonce || !tag || (!extra && extra_len != 0) || ((!src || !dst) && len != 0))) { return LDG_ERR_FUNC_ARG_NULL; }

    chacha_base_set(base, key, nonce, CHACHA_TEXT_CUNTR);

    ret = chacha_stream_apply(base, src, dst, len);

    if (ret == LDG_ERR_AOK) { ret = chacha_tag_compute(base, extra, extra_len, dst, len, tag); }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(base, sizeof(base)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    return ret;
}

uint32_t ldg_math_crypto_chacha20_poly1305_open(const uint8_t *key, const uint8_t *nonce, const uint8_t *extra, uint32_t extra_len, const uint8_t *src, uint32_t len, const uint8_t *tag, uint8_t *dst)
{
    uint32_t base[CHACHA_WORD_CUNT] = LDG_ARR_ZERO_INIT;
    uint8_t expected[LDG_MATH_CRYPTO_POLY1305_TAG_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!key || !nonce || !tag || (!extra && extra_len != 0) || ((!src || !dst) && len != 0))) { return LDG_ERR_FUNC_ARG_NULL; }

    chacha_base_set(base, key, nonce, CHACHA_TEXT_CUNTR);

    ret = chacha_tag_compute(base, extra, extra_len, src, len, expected);

    if (ret == LDG_ERR_AOK && ldg_mem_secure_eq_is(expected, tag, LDG_MATH_CRYPTO_POLY1305_TAG_SIZE) != LDG_TRUTH_TRUE) { ret = LDG_ERR_DENIED; }

    base[CHACHA_CUNTR_OFF] = CHACHA_TEXT_CUNTR;

    if (ret == LDG_ERR_AOK) { ret = chacha_stream_apply(base, src, dst, len); }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(base, sizeof(base)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    return ret;
}
