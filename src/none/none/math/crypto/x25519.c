#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/crypto/x25519.h>
#include <dangling/mem/secure.h>

#define X25519_LIMB_CUNT 16
#define X25519_LIMB_BITS 16
#define X25519_LIMB_MASK 0xFFFFULL
#define X25519_LIMB_SIZE 2
#define X25519_WIDE_CUNT 31
#define X25519_FOLD 38
#define X25519_CARRY_PASS_CUNT 2
#define X25519_FINAL_PASS_CUNT 3
#define X25519_REDUCE_PASS_CUNT 2
#define X25519_BIAS_SCALE 8
#define X25519_PRIME_LOW 0xFFEDULL
#define X25519_PRIME_MID 0xFFFFULL
#define X25519_PRIME_TOP 0x7FFFULL
#define X25519_TOP_LIMB 15
#define X25519_CURVE_LOW 0xDB41ULL
#define X25519_IDENT_LIMB 0x1ULL
#define X25519_ADJACENT_OFF 1
#define X25519_BORROW_SHIFT 63
#define X25519_BIT_MASK 0x1U
#define X25519_BYTE_BITS 8
#define X25519_BYTE_MASK 0xFFU
#define X25519_BYTE_BIT_MASK 0x7U
#define X25519_BYTE_SHIFT 3
#define X25519_LADDER_BIT_CUNT 255
#define X25519_INV_BIT_CUNT 254
#define X25519_INV_SKIP_LOW 2
#define X25519_INV_SKIP_HIGH 4
#define X25519_CLAMP_LOW 0xF8
#define X25519_CLAMP_TOP_KEEP 0x7F
#define X25519_CLAMP_TOP_SET 0x40
#define X25519_REG(area, slot) ((area)->reg + ((uint32_t)(slot) * X25519_LIMB_CUNT))

typedef enum x25519_reg
{
    X25519_REG_NEAR_X,
    X25519_REG_FAR_X,
    X25519_REG_NEAR_Z,
    X25519_REG_FAR_Z,
    X25519_REG_TMP_E,
    X25519_REG_TMP_F,
    X25519_REG_POINT,
    X25519_REG_CURVE,
    X25519_REG_CUNT
} x25519_reg_t;

typedef enum x25519_op
{
    X25519_OP_MUL,
    X25519_OP_ADD,
    X25519_OP_SUB
} x25519_op_t;

typedef struct x25519_step
{
    uint8_t op;
    uint8_t dst;
    uint8_t lhs;
    uint8_t rhs;
} x25519_step_t;

typedef struct x25519_area
{
    uint64_t reg[X25519_REG_CUNT * X25519_LIMB_CUNT];
    uint64_t wide[X25519_WIDE_CUNT];
    uint8_t scalar[LDG_MATH_CRYPTO_X25519_SIZE];
} x25519_area_t;

static uint64_t x25519_prime_limb_get(uint32_t idx)
{
    if (idx == 0) { return X25519_PRIME_LOW; }

    if (idx == X25519_TOP_LIMB) { return X25519_PRIME_TOP; }

    return X25519_PRIME_MID;
}

static void x25519_carry_apply(uint64_t *limb, uint32_t pass_cunt)
{
    uint64_t carry = 0;
    uint32_t pass = 0;
    uint32_t idx = 0;

    while (pass < pass_cunt)
    {
        idx = 0;

        while (idx < X25519_TOP_LIMB)
        {
            carry = limb[idx] >> X25519_LIMB_BITS;
            limb[idx] &= X25519_LIMB_MASK;
            limb[idx + X25519_ADJACENT_OFF] += carry;
            idx++;
        }

        carry = limb[X25519_TOP_LIMB] >> X25519_LIMB_BITS;
        limb[X25519_TOP_LIMB] &= X25519_LIMB_MASK;
        limb[0] += carry * X25519_FOLD;
        pass++;
    }
}

static void x25519_add(uint64_t *dst, const uint64_t *lhs, const uint64_t *rhs)
{
    uint32_t idx = 0;

    while (idx < X25519_LIMB_CUNT)
    {
        dst[idx] = lhs[idx] + rhs[idx];
        idx++;
    }

    x25519_carry_apply(dst, X25519_CARRY_PASS_CUNT);
}

static void x25519_sub(uint64_t *dst, const uint64_t *lhs, const uint64_t *rhs)
{
    uint32_t idx = 0;

    while (idx < X25519_LIMB_CUNT)
    {
        dst[idx] = (lhs[idx] + (X25519_BIAS_SCALE * x25519_prime_limb_get(idx))) - rhs[idx];
        idx++;
    }

    x25519_carry_apply(dst, X25519_CARRY_PASS_CUNT);
}

static void x25519_mul(x25519_area_t *area, uint64_t *dst, const uint64_t *lhs, const uint64_t *rhs)
{
    uint32_t row = 0;
    uint32_t col = 0;

    while (row < X25519_WIDE_CUNT)
    {
        area->wide[row] = 0;
        row++;
    }

    row = 0;

    while (row < X25519_LIMB_CUNT)
    {
        col = 0;

        while (col < X25519_LIMB_CUNT)
        {
            area->wide[row + col] += lhs[row] * rhs[col];
            col++;
        }

        row++;
    }

    row = 0;

    while (row < X25519_TOP_LIMB)
    {
        area->wide[row] += X25519_FOLD * area->wide[row + X25519_LIMB_CUNT];
        row++;
    }

    row = 0;

    while (row < X25519_LIMB_CUNT)
    {
        dst[row] = area->wide[row];
        row++;
    }

    x25519_carry_apply(dst, X25519_CARRY_PASS_CUNT);
}

static void x25519_swap(x25519_area_t *area, uint32_t lhs_slot, uint32_t rhs_slot, uint32_t pick)
{
    uint64_t *lhs = X25519_REG(area, lhs_slot);
    uint64_t *rhs = X25519_REG(area, rhs_slot);
    uint64_t mask = ~(uint64_t)pick + X25519_IDENT_LIMB;
    uint64_t diff = 0;
    uint32_t idx = 0;

    while (idx < X25519_LIMB_CUNT)
    {
        diff = mask & (lhs[idx] ^ rhs[idx]);
        lhs[idx] ^= diff;
        rhs[idx] ^= diff;
        idx++;
    }
}

static void x25519_step_run(x25519_area_t *area, const x25519_step_t *step, uint32_t step_cunt)
{
    uint64_t *dst = 0x0;
    const uint64_t *lhs = 0x0;
    const uint64_t *rhs = 0x0;
    uint32_t idx = 0;

    while (idx < step_cunt)
    {
        dst = X25519_REG(area, step[idx].dst);
        lhs = X25519_REG(area, step[idx].lhs);
        rhs = X25519_REG(area, step[idx].rhs);

        if (step[idx].op == X25519_OP_MUL) { x25519_mul(area, dst, lhs, rhs); }
        else if (step[idx].op == X25519_OP_ADD) { x25519_add(dst, lhs, rhs); }
        else { x25519_sub(dst, lhs, rhs); }

        idx++;
    }
}

static void x25519_ladder_step_run(x25519_area_t *area)
{
    const x25519_step_t step[] = {
        { X25519_OP_ADD, X25519_REG_TMP_E, X25519_REG_NEAR_X, X25519_REG_NEAR_Z },
        { X25519_OP_SUB, X25519_REG_NEAR_X, X25519_REG_NEAR_X, X25519_REG_NEAR_Z },
        { X25519_OP_ADD, X25519_REG_NEAR_Z, X25519_REG_FAR_X, X25519_REG_FAR_Z },
        { X25519_OP_SUB, X25519_REG_FAR_X, X25519_REG_FAR_X, X25519_REG_FAR_Z },
        { X25519_OP_MUL, X25519_REG_FAR_Z, X25519_REG_TMP_E, X25519_REG_TMP_E },
        { X25519_OP_MUL, X25519_REG_TMP_F, X25519_REG_NEAR_X, X25519_REG_NEAR_X },
        { X25519_OP_MUL, X25519_REG_NEAR_X, X25519_REG_NEAR_Z, X25519_REG_NEAR_X },
        { X25519_OP_MUL, X25519_REG_NEAR_Z, X25519_REG_FAR_X, X25519_REG_TMP_E },
        { X25519_OP_ADD, X25519_REG_TMP_E, X25519_REG_NEAR_X, X25519_REG_NEAR_Z },
        { X25519_OP_SUB, X25519_REG_NEAR_X, X25519_REG_NEAR_X, X25519_REG_NEAR_Z },
        { X25519_OP_MUL, X25519_REG_FAR_X, X25519_REG_NEAR_X, X25519_REG_NEAR_X },
        { X25519_OP_SUB, X25519_REG_NEAR_Z, X25519_REG_FAR_Z, X25519_REG_TMP_F },
        { X25519_OP_MUL, X25519_REG_NEAR_X, X25519_REG_NEAR_Z, X25519_REG_CURVE },
        { X25519_OP_ADD, X25519_REG_NEAR_X, X25519_REG_NEAR_X, X25519_REG_FAR_Z },
        { X25519_OP_MUL, X25519_REG_NEAR_Z, X25519_REG_NEAR_Z, X25519_REG_NEAR_X },
        { X25519_OP_MUL, X25519_REG_NEAR_X, X25519_REG_FAR_Z, X25519_REG_TMP_F },
        { X25519_OP_MUL, X25519_REG_FAR_Z, X25519_REG_FAR_X, X25519_REG_POINT },
        { X25519_OP_MUL, X25519_REG_FAR_X, X25519_REG_TMP_E, X25519_REG_TMP_E }
    };

    x25519_step_run(area, step, (uint32_t)(sizeof(step) / sizeof(step[0])));
}

static void x25519_ladder_run(x25519_area_t *area)
{
    uint32_t bit = X25519_LADDER_BIT_CUNT;
    uint32_t pick = 0;

    while (bit > 0)
    {
        bit--;
        pick = ((uint32_t)area->scalar[bit >> X25519_BYTE_SHIFT] >> (bit & X25519_BYTE_BIT_MASK)) & X25519_BIT_MASK;

        x25519_swap(area, X25519_REG_NEAR_X, X25519_REG_FAR_X, pick);
        x25519_swap(area, X25519_REG_NEAR_Z, X25519_REG_FAR_Z, pick);
        x25519_ladder_step_run(area);
        x25519_swap(area, X25519_REG_NEAR_X, X25519_REG_FAR_X, pick);
        x25519_swap(area, X25519_REG_NEAR_Z, X25519_REG_FAR_Z, pick);
    }
}

static void x25519_inv_apply(x25519_area_t *area)
{
    uint64_t *acc = X25519_REG(area, X25519_REG_TMP_E);
    const uint64_t *base = X25519_REG(area, X25519_REG_NEAR_Z);
    uint32_t bit = X25519_INV_BIT_CUNT;
    uint32_t idx = 0;

    while (idx < X25519_LIMB_CUNT)
    {
        acc[idx] = base[idx];
        idx++;
    }

    while (bit > 0)
    {
        bit--;
        x25519_mul(area, acc, acc, acc);

        if (bit != X25519_INV_SKIP_LOW && bit != X25519_INV_SKIP_HIGH) { x25519_mul(area, acc, acc, base); }
    }

    x25519_mul(area, X25519_REG(area, X25519_REG_NEAR_X), X25519_REG(area, X25519_REG_NEAR_X), acc);
}

static void x25519_reduce(x25519_area_t *area, uint64_t *limb)
{
    uint64_t diff = 0;
    uint64_t borrow = 0;
    uint64_t keep = 0;
    uint32_t pass = 0;
    uint32_t idx = 0;

    x25519_carry_apply(limb, X25519_FINAL_PASS_CUNT);

    while (pass < X25519_REDUCE_PASS_CUNT)
    {
        borrow = 0;
        idx = 0;

        while (idx < X25519_LIMB_CUNT)
        {
            diff = limb[idx] - x25519_prime_limb_get(idx) - borrow;
            borrow = diff >> X25519_BORROW_SHIFT;
            area->wide[idx] = diff & X25519_LIMB_MASK;
            idx++;
        }

        keep = ~borrow + X25519_IDENT_LIMB;
        idx = 0;

        while (idx < X25519_LIMB_CUNT)
        {
            limb[idx] = (limb[idx] & keep) | (area->wide[idx] & ~keep);
            idx++;
        }

        pass++;
    }
}

static void x25519_area_set(x25519_area_t *area, const uint8_t *scalar, const uint8_t *point)
{
    uint64_t *coord = X25519_REG(area, X25519_REG_POINT);
    uint64_t *far = X25519_REG(area, X25519_REG_FAR_X);
    uint32_t idx = 0;

    while (idx < LDG_MATH_CRYPTO_X25519_SIZE)
    {
        area->scalar[idx] = scalar[idx];
        idx++;
    }

    area->scalar[0] &= X25519_CLAMP_LOW;
    area->scalar[LDG_MATH_CRYPTO_X25519_SIZE - X25519_ADJACENT_OFF] &= X25519_CLAMP_TOP_KEEP;
    area->scalar[LDG_MATH_CRYPTO_X25519_SIZE - X25519_ADJACENT_OFF] |= X25519_CLAMP_TOP_SET;
    idx = 0;

    while (idx < X25519_LIMB_CUNT)
    {
        coord[idx] = (uint64_t)point[X25519_LIMB_SIZE * idx] | ((uint64_t)point[(X25519_LIMB_SIZE * idx) + X25519_ADJACENT_OFF] << X25519_BYTE_BITS);
        idx++;
    }

    coord[X25519_TOP_LIMB] &= X25519_PRIME_TOP;
    idx = 0;

    while (idx < X25519_LIMB_CUNT)
    {
        far[idx] = coord[idx];
        idx++;
    }

    X25519_REG(area, X25519_REG_NEAR_X)[0] = X25519_IDENT_LIMB;
    X25519_REG(area, X25519_REG_FAR_Z)[0] = X25519_IDENT_LIMB;
    X25519_REG(area, X25519_REG_CURVE)[0] = X25519_CURVE_LOW;
    X25519_REG(area, X25519_REG_CURVE)[X25519_ADJACENT_OFF] = X25519_IDENT_LIMB;
}

uint32_t ldg_math_crypto_x25519_compute(const uint8_t *scalar, const uint8_t *point, uint8_t *out)
{
    x25519_area_t area = LDG_STRUCT_ZERO_INIT;
    uint64_t *res = X25519_REG(&area, X25519_REG_NEAR_X);
    uint32_t present = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!scalar || !point || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    x25519_area_set(&area, scalar, point);
    x25519_ladder_run(&area);
    x25519_inv_apply(&area);
    x25519_reduce(&area, res);

    while (idx < X25519_LIMB_CUNT)
    {
        out[X25519_LIMB_SIZE * idx] = (uint8_t)(res[idx] & X25519_BYTE_MASK);
        out[(X25519_LIMB_SIZE * idx) + X25519_ADJACENT_OFF] = (uint8_t)((res[idx] >> X25519_BYTE_BITS) & X25519_BYTE_MASK);
        present |= (uint32_t)res[idx];
        idx++;
    }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(&area, sizeof(area)) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    if (present == 0) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}
