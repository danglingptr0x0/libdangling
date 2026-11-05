#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#include "bignum.h"

#define BIGNUM_LIMB_BITS 32
#define BIGNUM_LIMB_SIZE 4
#define BIGNUM_BYTE_BITS 8
#define BIGNUM_BYTE_MASK 0xFFU
#define BIGNUM_ADJACENT_OFF 1
#define BIGNUM_BORROW_SHIFT 63
#define BIGNUM_TOP_SHIFT 31
#define BIGNUM_ODD_BIT 0x1U
#define BIGNUM_INV_STEP_CUNT 5
#define BIGNUM_INV_BASE 2U
#define BIGNUM_DOUBLE_PER_LIMB 64
#define BIGNUM_UNIT 1U

uint32_t ldg_math_crypto_bignum_bytes_ld(uint32_t *dst, uint32_t limb_cunt, const uint8_t *src, uint32_t len)
{
    uint32_t limb = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (limb < limb_cunt)
    {
        dst[limb] = 0;
        limb++;
    }

    while (idx < len)
    {
        limb = idx / BIGNUM_LIMB_SIZE;

        if (limb < limb_cunt) { dst[limb] |= (uint32_t)src[len - BIGNUM_ADJACENT_OFF - idx] << (BIGNUM_BYTE_BITS * (idx % BIGNUM_LIMB_SIZE)); }
        else if (LDG_UNLIKELY(src[len - BIGNUM_ADJACENT_OFF - idx] != 0)) { return LDG_ERR_OVERFLOW; }

        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_crypto_bignum_bytes_st(const uint32_t *src, uint32_t limb_cunt, uint8_t *dst, uint32_t len)
{
    uint32_t limb = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!dst || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (idx < len)
    {
        limb = idx / BIGNUM_LIMB_SIZE;
        dst[len - BIGNUM_ADJACENT_OFF - idx] = (limb < limb_cunt) ? (uint8_t)((src[limb] >> (BIGNUM_BYTE_BITS * (idx % BIGNUM_LIMB_SIZE))) & BIGNUM_BYTE_MASK) : (uint8_t)LDG_TRUTH_FALSE;
        idx++;
    }

    return LDG_ERR_AOK;
}

uint8_t ldg_math_crypto_bignum_ge_is(const uint32_t *lhs, const uint32_t *rhs, uint32_t limb_cunt)
{
    uint32_t limb = limb_cunt;

    if (LDG_UNLIKELY(!lhs || !rhs)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (limb > 0)
    {
        limb--;

        if (lhs[limb] > rhs[limb]) { return LDG_TRUTH_TRUE; }

        if (lhs[limb] < rhs[limb]) { return LDG_TRUTH_FALSE; }
    }

    return LDG_TRUTH_TRUE;
}

uint8_t ldg_math_crypto_bignum_none_is(const uint32_t *val, uint32_t limb_cunt)
{
    uint32_t limb = 0;

    if (LDG_UNLIKELY(!val)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (limb < limb_cunt)
    {
        if (val[limb] != 0) { return LDG_TRUTH_FALSE; }

        limb++;
    }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_math_crypto_bignum_bit_len_get(const uint32_t *val, uint32_t limb_cunt)
{
    uint32_t limb = limb_cunt;
    uint32_t top = 0;
    uint32_t bits = 0;

    if (LDG_UNLIKELY(!val)) { return UINT32_MAX; }

    while (limb > 0 && val[limb - BIGNUM_ADJACENT_OFF] == 0) { limb--; }

    if (limb == 0) { return bits; }

    top = val[limb - BIGNUM_ADJACENT_OFF];

    while (top != 0)
    {
        top >>= BIGNUM_ADJACENT_OFF;
        bits++;
    }

    return ((limb - BIGNUM_ADJACENT_OFF) * BIGNUM_LIMB_BITS) + bits;
}

uint32_t ldg_math_crypto_bignum_add(uint32_t *dst, const uint32_t *lhs, const uint32_t *rhs, uint32_t limb_cunt, uint32_t *out_carry)
{
    uint64_t cum = 0;
    uint32_t limb = 0;

    if (LDG_UNLIKELY(!dst || !lhs || !rhs || !out_carry)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (limb < limb_cunt)
    {
        cum = (uint64_t)lhs[limb] + (uint64_t)rhs[limb] + (cum >> BIGNUM_LIMB_BITS);
        dst[limb] = (uint32_t)cum;
        limb++;
    }

    *out_carry = (uint32_t)(cum >> BIGNUM_LIMB_BITS);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_crypto_bignum_sub(uint32_t *dst, const uint32_t *lhs, const uint32_t *rhs, uint32_t limb_cunt, uint32_t *out_borrow)
{
    uint64_t diff = 0;
    uint32_t limb = 0;

    if (LDG_UNLIKELY(!dst || !lhs || !rhs || !out_borrow)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (limb < limb_cunt)
    {
        diff = (uint64_t)lhs[limb] - (uint64_t)rhs[limb] - (diff >> BIGNUM_BORROW_SHIFT);
        dst[limb] = (uint32_t)diff;
        limb++;
    }

    *out_borrow = (uint32_t)(diff >> BIGNUM_BORROW_SHIFT);

    return LDG_ERR_AOK;
}

static uint32_t bignum_double(uint32_t *val, uint32_t limb_cunt)
{
    uint32_t carry = 0;
    uint32_t next = 0;
    uint32_t limb = 0;

    while (limb < limb_cunt)
    {
        next = val[limb] >> BIGNUM_TOP_SHIFT;
        val[limb] = (val[limb] << BIGNUM_ADJACENT_OFF) | carry;
        carry = next;
        limb++;
    }

    return carry;
}

uint32_t ldg_math_crypto_mont_init(ldg_math_crypto_mont_t *mont, const uint32_t *modulus, uint32_t *square, uint32_t limb_cunt)
{
    uint32_t inv = 0;
    uint32_t carry = 0;
    uint32_t borrow = 0;
    uint32_t pass = 0;
    uint32_t limb = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!mont || !modulus || !square)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(limb_cunt == 0 || (modulus[0] & BIGNUM_ODD_BIT) == 0 || modulus[limb_cunt - BIGNUM_ADJACENT_OFF] == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(limb_cunt == BIGNUM_ADJACENT_OFF && modulus[0] == BIGNUM_UNIT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    inv = modulus[0];

    while (pass < BIGNUM_INV_STEP_CUNT)
    {
        inv = inv * (BIGNUM_INV_BASE - (modulus[0] * inv));
        pass++;
    }

    while (limb < limb_cunt)
    {
        square[limb] = 0;
        limb++;
    }

    square[0] = BIGNUM_UNIT;
    pass = 0;

    while (pass < (limb_cunt * BIGNUM_DOUBLE_PER_LIMB))
    {
        carry = bignum_double(square, limb_cunt);

        if (carry != 0 || ldg_math_crypto_bignum_ge_is(square, modulus, limb_cunt) == LDG_TRUTH_TRUE)
        {
            ret = ldg_math_crypto_bignum_sub(square, square, modulus, limb_cunt, &borrow);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        pass++;
    }

    mont->modulus = modulus;
    mont->square = square;
    mont->inv = ~inv + BIGNUM_UNIT;
    mont->limb_cunt = limb_cunt;

    return LDG_ERR_AOK;
}

static void bignum_mont_row_cum(const ldg_math_crypto_mont_t *mont, uint32_t *work, const uint32_t *lhs, uint32_t factor)
{
    uint64_t cum = 0;
    uint32_t fold = 0;
    uint32_t cunt = mont->limb_cunt;
    uint32_t limb = 0;

    while (limb < cunt)
    {
        cum = (uint64_t)work[limb] + ((uint64_t)lhs[limb] * (uint64_t)factor) + (cum >> BIGNUM_LIMB_BITS);
        work[limb] = (uint32_t)cum;
        limb++;
    }

    cum = (uint64_t)work[cunt] + (cum >> BIGNUM_LIMB_BITS);
    work[cunt] = (uint32_t)cum;
    work[cunt + BIGNUM_ADJACENT_OFF] = (uint32_t)(cum >> BIGNUM_LIMB_BITS);

    fold = work[0] * mont->inv;
    cum = (uint64_t)work[0] + ((uint64_t)fold * (uint64_t)mont->modulus[0]);
    limb = BIGNUM_ADJACENT_OFF;

    while (limb < cunt)
    {
        cum = (uint64_t)work[limb] + ((uint64_t)fold * (uint64_t)mont->modulus[limb]) + (cum >> BIGNUM_LIMB_BITS);
        work[limb - BIGNUM_ADJACENT_OFF] = (uint32_t)cum;
        limb++;
    }

    cum = (uint64_t)work[cunt] + (cum >> BIGNUM_LIMB_BITS);
    work[cunt - BIGNUM_ADJACENT_OFF] = (uint32_t)cum;
    work[cunt] = work[cunt + BIGNUM_ADJACENT_OFF] + (uint32_t)(cum >> BIGNUM_LIMB_BITS);
}

uint32_t ldg_math_crypto_mont_mul(const ldg_math_crypto_mont_t *mont, uint32_t *dst, const uint32_t *lhs, const uint32_t *rhs, uint32_t *work)
{
    uint32_t borrow = 0;
    uint32_t limb = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!mont || !dst || !lhs || !rhs || !work)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (limb < (mont->limb_cunt + LDG_MATH_CRYPTO_BIGNUM_WORK_EXTRA))
    {
        work[limb] = 0;
        limb++;
    }

    limb = 0;

    while (limb < mont->limb_cunt)
    {
        bignum_mont_row_cum(mont, work, lhs, rhs[limb]);
        limb++;
    }

    if (work[mont->limb_cunt] != 0 || ldg_math_crypto_bignum_ge_is(work, mont->modulus, mont->limb_cunt) == LDG_TRUTH_TRUE)
    {
        ret = ldg_math_crypto_bignum_sub(work, work, mont->modulus, mont->limb_cunt, &borrow);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    limb = 0;

    while (limb < mont->limb_cunt)
    {
        dst[limb] = work[limb];
        limb++;
    }

    return LDG_ERR_AOK;
}
