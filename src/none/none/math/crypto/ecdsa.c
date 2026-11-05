#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/crypto/ecdsa.h>
#include <dangling/str/str.h>

#include "bignum.h"

#define ECDSA_LIMB_SIZE 4
#define ECDSA_LIMB_BITS 32
#define ECDSA_BIT_MASK 0x1U
#define ECDSA_ADJACENT_OFF 1
#define ECDSA_POINT_REG_CUNT 3
#define ECDSA_SINGLE_REG_CUNT 1
#define ECDSA_KEY_TAG 0x04
#define ECDSA_KEY_TAG_SIZE 1
#define ECDSA_COORD_PER_KEY 2
#define ECDSA_FERMAT_OFF 0x2U
#define ECDSA_IDENT_LIMB 0x1U
#define ECDSA_KEY_SEL_SHIFT 1
#define ECDSA_NARROW_SIZE 32
#define ECDSA_WIDE_SIZE 48
#define ECDSA_DER_SEQUENCE 0x30
#define ECDSA_DER_INTEGER 0x02
#define ECDSA_DER_LEN_CAP 0x80
#define ECDSA_DER_SIGN_BIT 0x80
#define ECDSA_DER_HDR_SIZE 2
#define ECDSA_REG(area, slot) ((area)->reg + ((uint32_t)(slot) * (area)->limb_cunt))

typedef enum ecdsa_reg
{
    ECDSA_REG_PRIME,
    ECDSA_REG_SQUARE,
    ECDSA_REG_COEFF_A,
    ECDSA_REG_COEFF_B,
    ECDSA_REG_UNIT,
    ECDSA_REG_ACC_X,
    ECDSA_REG_ACC_Y,
    ECDSA_REG_ACC_Z,
    ECDSA_REG_ARG_X,
    ECDSA_REG_ARG_Y,
    ECDSA_REG_ARG_Z,
    ECDSA_REG_OUT_X,
    ECDSA_REG_OUT_Y,
    ECDSA_REG_OUT_Z,
    ECDSA_REG_GEN_X,
    ECDSA_REG_GEN_Y,
    ECDSA_REG_GEN_Z,
    ECDSA_REG_KEY_X,
    ECDSA_REG_KEY_Y,
    ECDSA_REG_KEY_Z,
    ECDSA_REG_SUM_X,
    ECDSA_REG_SUM_Y,
    ECDSA_REG_SUM_Z,
    ECDSA_REG_TMP_A,
    ECDSA_REG_TMP_B,
    ECDSA_REG_TMP_C,
    ECDSA_REG_TMP_D,
    ECDSA_REG_TMP_E,
    ECDSA_REG_TMP_F,
    ECDSA_REG_ORDER,
    ECDSA_REG_ORDER_SQUARE,
    ECDSA_REG_SIG_R,
    ECDSA_REG_SIG_S,
    ECDSA_REG_DIGEST,
    ECDSA_REG_INV,
    ECDSA_REG_EXP,
    ECDSA_REG_U_GEN,
    ECDSA_REG_U_KEY,
    ECDSA_REG_CUNT
} ecdsa_reg_t;

typedef enum ecdsa_op
{
    ECDSA_OP_MUL,
    ECDSA_OP_ADD,
    ECDSA_OP_SUB
} ecdsa_op_t;

typedef struct ecdsa_step
{
    uint8_t op;
    uint8_t dst;
    uint8_t lhs;
    uint8_t rhs;
} ecdsa_step_t;

typedef struct ecdsa_curve_text
{
    const char *prime;
    const char *coeff_a;
    const char *coeff_b;
    const char *gen_x;
    const char *gen_y;
    const char *order;
} ecdsa_curve_text_t;

typedef struct ecdsa_area
{
    ldg_math_crypto_mont_t field;
    ldg_math_crypto_mont_t scalar;
    uint32_t *reg;
    uint32_t *work;
    uint32_t limb_cunt;
    uint32_t size;
} ecdsa_area_t;

static void ecdsa_p256_text_get(ecdsa_curve_text_t *out)
{
    out->prime = "ffffffff00000001000000000000000000000000ffffffffffffffffffffffff";
    out->coeff_a = "ffffffff00000001000000000000000000000000fffffffffffffffffffffffc";
    out->coeff_b = "5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b";
    out->gen_x = "6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296";
    out->gen_y = "4" "fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5";
    out->order = "ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551";
}

static void ecdsa_p384_text_get(ecdsa_curve_text_t *out)
{
    out->prime = "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff";
    out->coeff_a = "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000fffffffc";
    out->coeff_b = "b3312fa7e23ee7e4988e056be3f82d19181d9c6efe8141120314088f5013875ac656398d8a2ed19d2a85c8edd3ec2aef";
    out->gen_x = "aa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7";
    out->gen_y = "3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f";
    out->order = "ffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973";
}

static void ecdsa_secp256k1_text_get(ecdsa_curve_text_t *out)
{
    out->prime = "fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f";
    out->coeff_a = "0000000000000000000000000000000000000000000000000000000000000000";
    out->coeff_b = "0000000000000000000000000000000000000000000000000000000000000007";
    out->gen_x = "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798";
    out->gen_y = "483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8";
    out->order = "fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141";
}

static void ecdsa_curve_text_get(ldg_math_crypto_ecdsa_curve_t curve, ecdsa_curve_text_t *out)
{
    if (curve == LDG_MATH_CRYPTO_ECDSA_CURVE_P256) { ecdsa_p256_text_get(out); }

    if (curve == LDG_MATH_CRYPTO_ECDSA_CURVE_P384) { ecdsa_p384_text_get(out); }

    if (curve == LDG_MATH_CRYPTO_ECDSA_CURVE_SECP256K1) { ecdsa_secp256k1_text_get(out); }
}

static void ecdsa_reg_cpy(const ecdsa_area_t *area, uint32_t dst_slot, uint32_t src_slot, uint32_t reg_cunt)
{
    uint32_t *dst = ECDSA_REG(area, dst_slot);
    const uint32_t *src = ECDSA_REG(area, src_slot);
    uint32_t limb = 0;

    while (limb < (reg_cunt * area->limb_cunt))
    {
        dst[limb] = src[limb];
        limb++;
    }
}

static void ecdsa_reg_clear(const ecdsa_area_t *area, uint32_t slot, uint32_t reg_cunt)
{
    uint32_t *dst = ECDSA_REG(area, slot);
    uint32_t limb = 0;

    while (limb < (reg_cunt * area->limb_cunt))
    {
        dst[limb] = 0;
        limb++;
    }
}

static uint8_t ecdsa_reg_eq_is(const ecdsa_area_t *area, uint32_t lhs_slot, uint32_t rhs_slot)
{
    const uint32_t *lhs = ECDSA_REG(area, lhs_slot);
    const uint32_t *rhs = ECDSA_REG(area, rhs_slot);
    uint32_t diff = 0;
    uint32_t limb = 0;

    while (limb < area->limb_cunt)
    {
        diff |= lhs[limb] ^ rhs[limb];
        limb++;
    }

    return (diff == 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t ecdsa_bit_get(const uint32_t *val, uint32_t bit)
{
    return (val[bit / ECDSA_LIMB_BITS] >> (bit % ECDSA_LIMB_BITS)) & ECDSA_BIT_MASK;
}

static uint32_t ecdsa_field_add(const ecdsa_area_t *area, uint32_t *dst, const uint32_t *lhs, const uint32_t *rhs)
{
    uint32_t carry = 0;
    uint32_t borrow = 0;
    uint32_t ret = 0;

    ret = ldg_math_crypto_bignum_add(dst, lhs, rhs, area->limb_cunt, &carry);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (carry != 0 || ldg_math_crypto_bignum_ge_is(dst, area->field.modulus, area->limb_cunt) == LDG_TRUTH_TRUE) { ret = ldg_math_crypto_bignum_sub(dst, dst, area->field.modulus, area->limb_cunt, &borrow); }

    return ret;
}

static uint32_t ecdsa_field_sub(const ecdsa_area_t *area, uint32_t *dst, const uint32_t *lhs, const uint32_t *rhs)
{
    uint32_t carry = 0;
    uint32_t borrow = 0;
    uint32_t ret = 0;

    ret = ldg_math_crypto_bignum_sub(dst, lhs, rhs, area->limb_cunt, &borrow);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (borrow != 0) { ret = ldg_math_crypto_bignum_add(dst, dst, area->field.modulus, area->limb_cunt, &carry); }

    return ret;
}

static uint32_t ecdsa_step_run(const ecdsa_area_t *area, const ecdsa_step_t *step, uint32_t step_cunt)
{
    uint32_t *dst = 0x0;
    const uint32_t *lhs = 0x0;
    const uint32_t *rhs = 0x0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    while (idx < step_cunt)
    {
        dst = ECDSA_REG(area, step[idx].dst);
        lhs = ECDSA_REG(area, step[idx].lhs);
        rhs = ECDSA_REG(area, step[idx].rhs);

        if (step[idx].op == ECDSA_OP_MUL) { ret = ldg_math_crypto_mont_mul(&area->field, dst, lhs, rhs, area->work); }
        else if (step[idx].op == ECDSA_OP_ADD) { ret = ecdsa_field_add(area, dst, lhs, rhs); }
        else { ret = ecdsa_field_sub(area, dst, lhs, rhs); }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        idx++;
    }

    return LDG_ERR_AOK;
}

static uint32_t ecdsa_point_double(const ecdsa_area_t *area)
{
    const ecdsa_step_t step[] = {
        { ECDSA_OP_MUL, ECDSA_REG_TMP_A, ECDSA_REG_ACC_X, ECDSA_REG_ACC_X },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_B, ECDSA_REG_ACC_Y, ECDSA_REG_ACC_Y },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_C, ECDSA_REG_TMP_B, ECDSA_REG_TMP_B },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_D, ECDSA_REG_ACC_Z, ECDSA_REG_ACC_Z },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_E, ECDSA_REG_ACC_X, ECDSA_REG_TMP_B },
        { ECDSA_OP_ADD, ECDSA_REG_TMP_E, ECDSA_REG_TMP_E, ECDSA_REG_TMP_E },
        { ECDSA_OP_ADD, ECDSA_REG_TMP_E, ECDSA_REG_TMP_E, ECDSA_REG_TMP_E },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_D, ECDSA_REG_TMP_D, ECDSA_REG_TMP_D },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_D, ECDSA_REG_COEFF_A, ECDSA_REG_TMP_D },
        { ECDSA_OP_ADD, ECDSA_REG_TMP_F, ECDSA_REG_TMP_A, ECDSA_REG_TMP_A },
        { ECDSA_OP_ADD, ECDSA_REG_TMP_F, ECDSA_REG_TMP_F, ECDSA_REG_TMP_A },
        { ECDSA_OP_ADD, ECDSA_REG_TMP_F, ECDSA_REG_TMP_F, ECDSA_REG_TMP_D },
        { ECDSA_OP_MUL, ECDSA_REG_OUT_Z, ECDSA_REG_ACC_Y, ECDSA_REG_ACC_Z },
        { ECDSA_OP_ADD, ECDSA_REG_OUT_Z, ECDSA_REG_OUT_Z, ECDSA_REG_OUT_Z },
        { ECDSA_OP_MUL, ECDSA_REG_OUT_X, ECDSA_REG_TMP_F, ECDSA_REG_TMP_F },
        { ECDSA_OP_SUB, ECDSA_REG_OUT_X, ECDSA_REG_OUT_X, ECDSA_REG_TMP_E },
        { ECDSA_OP_SUB, ECDSA_REG_OUT_X, ECDSA_REG_OUT_X, ECDSA_REG_TMP_E },
        { ECDSA_OP_SUB, ECDSA_REG_TMP_E, ECDSA_REG_TMP_E, ECDSA_REG_OUT_X },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_E, ECDSA_REG_TMP_F, ECDSA_REG_TMP_E },
        { ECDSA_OP_ADD, ECDSA_REG_TMP_C, ECDSA_REG_TMP_C, ECDSA_REG_TMP_C },
        { ECDSA_OP_ADD, ECDSA_REG_TMP_C, ECDSA_REG_TMP_C, ECDSA_REG_TMP_C },
        { ECDSA_OP_ADD, ECDSA_REG_TMP_C, ECDSA_REG_TMP_C, ECDSA_REG_TMP_C },
        { ECDSA_OP_SUB, ECDSA_REG_OUT_Y, ECDSA_REG_TMP_E, ECDSA_REG_TMP_C }
    };
    uint32_t ret = 0;

    if (ldg_math_crypto_bignum_none_is(ECDSA_REG(area, ECDSA_REG_ACC_Z), area->limb_cunt) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ecdsa_step_run(area, step, (uint32_t)(sizeof(step) / sizeof(step[0])));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ecdsa_reg_cpy(area, ECDSA_REG_ACC_X, ECDSA_REG_OUT_X, ECDSA_POINT_REG_CUNT);

    return LDG_ERR_AOK;
}

static uint32_t ecdsa_point_add_head_run(const ecdsa_area_t *area)
{
    const ecdsa_step_t step[] = {
        { ECDSA_OP_MUL, ECDSA_REG_TMP_A, ECDSA_REG_ACC_Z, ECDSA_REG_ACC_Z },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_B, ECDSA_REG_ARG_Z, ECDSA_REG_ARG_Z },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_C, ECDSA_REG_ACC_X, ECDSA_REG_TMP_B },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_D, ECDSA_REG_ARG_X, ECDSA_REG_TMP_A },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_E, ECDSA_REG_ACC_Y, ECDSA_REG_ARG_Z },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_E, ECDSA_REG_TMP_E, ECDSA_REG_TMP_B },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_F, ECDSA_REG_ARG_Y, ECDSA_REG_ACC_Z },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_F, ECDSA_REG_TMP_F, ECDSA_REG_TMP_A },
        { ECDSA_OP_SUB, ECDSA_REG_TMP_D, ECDSA_REG_TMP_D, ECDSA_REG_TMP_C },
        { ECDSA_OP_SUB, ECDSA_REG_TMP_F, ECDSA_REG_TMP_F, ECDSA_REG_TMP_E }
    };

    return ecdsa_step_run(area, step, (uint32_t)(sizeof(step) / sizeof(step[0])));
}

static uint32_t ecdsa_point_add_tail_run(const ecdsa_area_t *area)
{
    const ecdsa_step_t step[] = {
        { ECDSA_OP_MUL, ECDSA_REG_TMP_A, ECDSA_REG_TMP_D, ECDSA_REG_TMP_D },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_B, ECDSA_REG_TMP_D, ECDSA_REG_TMP_A },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_C, ECDSA_REG_TMP_C, ECDSA_REG_TMP_A },
        { ECDSA_OP_MUL, ECDSA_REG_OUT_Z, ECDSA_REG_ACC_Z, ECDSA_REG_ARG_Z },
        { ECDSA_OP_MUL, ECDSA_REG_OUT_Z, ECDSA_REG_OUT_Z, ECDSA_REG_TMP_D },
        { ECDSA_OP_MUL, ECDSA_REG_OUT_X, ECDSA_REG_TMP_F, ECDSA_REG_TMP_F },
        { ECDSA_OP_SUB, ECDSA_REG_OUT_X, ECDSA_REG_OUT_X, ECDSA_REG_TMP_B },
        { ECDSA_OP_SUB, ECDSA_REG_OUT_X, ECDSA_REG_OUT_X, ECDSA_REG_TMP_C },
        { ECDSA_OP_SUB, ECDSA_REG_OUT_X, ECDSA_REG_OUT_X, ECDSA_REG_TMP_C },
        { ECDSA_OP_SUB, ECDSA_REG_TMP_C, ECDSA_REG_TMP_C, ECDSA_REG_OUT_X },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_C, ECDSA_REG_TMP_F, ECDSA_REG_TMP_C },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_E, ECDSA_REG_TMP_E, ECDSA_REG_TMP_B },
        { ECDSA_OP_SUB, ECDSA_REG_OUT_Y, ECDSA_REG_TMP_C, ECDSA_REG_TMP_E }
    };

    return ecdsa_step_run(area, step, (uint32_t)(sizeof(step) / sizeof(step[0])));
}

static uint32_t ecdsa_point_add(const ecdsa_area_t *area)
{
    uint32_t ret = 0;

    if (ldg_math_crypto_bignum_none_is(ECDSA_REG(area, ECDSA_REG_ARG_Z), area->limb_cunt) == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (ldg_math_crypto_bignum_none_is(ECDSA_REG(area, ECDSA_REG_ACC_Z), area->limb_cunt) == LDG_TRUTH_TRUE)
    {
        ecdsa_reg_cpy(area, ECDSA_REG_ACC_X, ECDSA_REG_ARG_X, ECDSA_POINT_REG_CUNT);
        return LDG_ERR_AOK;
    }

    ret = ecdsa_point_add_head_run(area);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ldg_math_crypto_bignum_none_is(ECDSA_REG(area, ECDSA_REG_TMP_D), area->limb_cunt) == LDG_TRUTH_TRUE)
    {
        if (ldg_math_crypto_bignum_none_is(ECDSA_REG(area, ECDSA_REG_TMP_F), area->limb_cunt) == LDG_TRUTH_TRUE) { return ecdsa_point_double(area); }

        ecdsa_reg_clear(area, ECDSA_REG_ACC_Z, ECDSA_SINGLE_REG_CUNT);
        return LDG_ERR_AOK;
    }

    ret = ecdsa_point_add_tail_run(area);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ecdsa_reg_cpy(area, ECDSA_REG_ACC_X, ECDSA_REG_OUT_X, ECDSA_POINT_REG_CUNT);

    return LDG_ERR_AOK;
}

static uint8_t ecdsa_arg_valid_is(const ecdsa_area_t *area)
{
    const ecdsa_step_t step[] = {
        { ECDSA_OP_MUL, ECDSA_REG_TMP_A, ECDSA_REG_ARG_X, ECDSA_REG_ARG_X },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_A, ECDSA_REG_TMP_A, ECDSA_REG_ARG_X },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_B, ECDSA_REG_COEFF_A, ECDSA_REG_ARG_X },
        { ECDSA_OP_ADD, ECDSA_REG_TMP_A, ECDSA_REG_TMP_A, ECDSA_REG_TMP_B },
        { ECDSA_OP_ADD, ECDSA_REG_TMP_A, ECDSA_REG_TMP_A, ECDSA_REG_COEFF_B },
        { ECDSA_OP_MUL, ECDSA_REG_TMP_B, ECDSA_REG_ARG_Y, ECDSA_REG_ARG_Y }
    };
    uint32_t ret = 0;

    ret = ecdsa_step_run(area, step, (uint32_t)(sizeof(step) / sizeof(step[0])));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    return ecdsa_reg_eq_is(area, ECDSA_REG_TMP_A, ECDSA_REG_TMP_B);
}

static uint32_t ecdsa_text_ld(const ecdsa_area_t *area, uint32_t slot, const char *text)
{
    uint8_t raw[ECDSA_WIDE_SIZE] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    ret = ldg_hex_to_bytes(text, raw, area->size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_crypto_bignum_bytes_ld(ECDSA_REG(area, slot), area->limb_cunt, raw, area->size);
}

static uint32_t ecdsa_field_ld(const ecdsa_area_t *area, uint32_t slot, const char *text)
{
    uint32_t ret = 0;

    ret = ecdsa_text_ld(area, slot, text);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_crypto_mont_mul(&area->field, ECDSA_REG(area, slot), ECDSA_REG(area, slot), ECDSA_REG(area, ECDSA_REG_SQUARE), area->work);
}

static uint32_t ecdsa_modulus_ld(ecdsa_area_t *area, const ecdsa_curve_text_t *text)
{
    uint32_t ret = 0;

    ret = ecdsa_text_ld(area, ECDSA_REG_PRIME, text->prime);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_crypto_mont_init(&area->field, ECDSA_REG(area, ECDSA_REG_PRIME), ECDSA_REG(area, ECDSA_REG_SQUARE), area->limb_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ecdsa_text_ld(area, ECDSA_REG_ORDER, text->order);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_crypto_mont_init(&area->scalar, ECDSA_REG(area, ECDSA_REG_ORDER), ECDSA_REG(area, ECDSA_REG_ORDER_SQUARE), area->limb_cunt);
}

static uint32_t ecdsa_curve_ld(ecdsa_area_t *area, ldg_math_crypto_ecdsa_curve_t curve)
{
    ecdsa_curve_text_t text = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    ecdsa_curve_text_get(curve, &text);

    ret = ecdsa_modulus_ld(area, &text);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ecdsa_field_ld(area, ECDSA_REG_COEFF_A, text.coeff_a);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ecdsa_field_ld(area, ECDSA_REG_COEFF_B, text.coeff_b);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ecdsa_field_ld(area, ECDSA_REG_GEN_X, text.gen_x);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ecdsa_field_ld(area, ECDSA_REG_GEN_Y, text.gen_y);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ecdsa_reg_clear(area, ECDSA_REG_UNIT, ECDSA_SINGLE_REG_CUNT);
    ECDSA_REG(area, ECDSA_REG_UNIT)[0] = ECDSA_IDENT_LIMB;

    ret = ldg_math_crypto_mont_mul(&area->field, ECDSA_REG(area, ECDSA_REG_UNIT), ECDSA_REG(area, ECDSA_REG_UNIT), ECDSA_REG(area, ECDSA_REG_SQUARE), area->work);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ecdsa_reg_cpy(area, ECDSA_REG_GEN_Z, ECDSA_REG_UNIT, ECDSA_SINGLE_REG_CUNT);
    ecdsa_reg_cpy(area, ECDSA_REG_ARG_X, ECDSA_REG_GEN_X, ECDSA_POINT_REG_CUNT);

    if (LDG_UNLIKELY(ecdsa_arg_valid_is(area) != LDG_TRUTH_TRUE)) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t ecdsa_coord_ld(const ecdsa_area_t *area, uint32_t slot, const uint8_t *src)
{
    uint32_t ret = 0;

    ret = ldg_math_crypto_bignum_bytes_ld(ECDSA_REG(area, slot), area->limb_cunt, src, area->size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ldg_math_crypto_bignum_ge_is(ECDSA_REG(area, slot), ECDSA_REG(area, ECDSA_REG_PRIME), area->limb_cunt) == LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return ldg_math_crypto_mont_mul(&area->field, ECDSA_REG(area, slot), ECDSA_REG(area, slot), ECDSA_REG(area, ECDSA_REG_SQUARE), area->work);
}

static uint32_t ecdsa_key_ld(const ecdsa_area_t *area, const uint8_t *key, uint32_t key_len)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(key_len != (ECDSA_KEY_TAG_SIZE + (ECDSA_COORD_PER_KEY * area->size)) || key[0] != ECDSA_KEY_TAG)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ecdsa_coord_ld(area, ECDSA_REG_KEY_X, key + ECDSA_KEY_TAG_SIZE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ecdsa_coord_ld(area, ECDSA_REG_KEY_Y, key + ECDSA_KEY_TAG_SIZE + area->size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ecdsa_reg_cpy(area, ECDSA_REG_KEY_Z, ECDSA_REG_UNIT, ECDSA_SINGLE_REG_CUNT);
    ecdsa_reg_cpy(area, ECDSA_REG_ARG_X, ECDSA_REG_KEY_X, ECDSA_POINT_REG_CUNT);

    if (LDG_UNLIKELY(ecdsa_arg_valid_is(area) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ecdsa_reg_cpy(area, ECDSA_REG_ACC_X, ECDSA_REG_GEN_X, ECDSA_POINT_REG_CUNT);

    ret = ecdsa_point_add(area);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ecdsa_reg_cpy(area, ECDSA_REG_SUM_X, ECDSA_REG_ACC_X, ECDSA_POINT_REG_CUNT);

    return LDG_ERR_AOK;
}

static uint32_t ecdsa_scalar_ld(const ecdsa_area_t *area, uint32_t slot, const uint8_t *src, uint32_t len)
{
    uint32_t ret = 0;

    ret = ldg_math_crypto_bignum_bytes_ld(ECDSA_REG(area, slot), area->limb_cunt, src, len);
    if (ret == LDG_ERR_OVERFLOW) { return LDG_ERR_RANGE; }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ldg_math_crypto_bignum_none_is(ECDSA_REG(area, slot), area->limb_cunt) == LDG_TRUTH_TRUE) { return LDG_ERR_RANGE; }

    if (ldg_math_crypto_bignum_ge_is(ECDSA_REG(area, slot), ECDSA_REG(area, ECDSA_REG_ORDER), area->limb_cunt) == LDG_TRUTH_TRUE) { return LDG_ERR_RANGE; }

    return LDG_ERR_AOK;
}

static uint32_t ecdsa_digest_ld(const ecdsa_area_t *area, const uint8_t *digest, uint32_t digest_len)
{
    uint32_t *dst = ECDSA_REG(area, ECDSA_REG_DIGEST);
    uint32_t len = (digest_len < area->size) ? digest_len : area->size;
    uint32_t borrow = 0;
    uint32_t ret = 0;

    ret = ldg_math_crypto_bignum_bytes_ld(dst, area->limb_cunt, digest, len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ldg_math_crypto_bignum_ge_is(dst, ECDSA_REG(area, ECDSA_REG_ORDER), area->limb_cunt) == LDG_TRUTH_TRUE) { ret = ldg_math_crypto_bignum_sub(dst, dst, ECDSA_REG(area, ECDSA_REG_ORDER), area->limb_cunt, &borrow); }

    return ret;
}

static uint32_t ecdsa_inv_bit_apply(const ecdsa_area_t *area, uint32_t bit)
{
    uint32_t *inv = ECDSA_REG(area, ECDSA_REG_INV);
    uint32_t ret = 0;

    ret = ldg_math_crypto_mont_mul(&area->scalar, inv, inv, inv, area->work);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ecdsa_bit_get(ECDSA_REG(area, ECDSA_REG_EXP), bit) != 0) { ret = ldg_math_crypto_mont_mul(&area->scalar, inv, inv, ECDSA_REG(area, ECDSA_REG_TMP_B), area->work); }

    return ret;
}

static uint32_t ecdsa_inv_compute(const ecdsa_area_t *area)
{
    uint32_t *inv = ECDSA_REG(area, ECDSA_REG_INV);
    uint32_t borrow = 0;
    uint32_t bit = area->limb_cunt * ECDSA_LIMB_BITS;
    uint32_t ret = 0;

    ecdsa_reg_clear(area, ECDSA_REG_TMP_A, ECDSA_SINGLE_REG_CUNT);
    ECDSA_REG(area, ECDSA_REG_TMP_A)[0] = ECDSA_FERMAT_OFF;

    ret = ldg_math_crypto_bignum_sub(ECDSA_REG(area, ECDSA_REG_EXP), ECDSA_REG(area, ECDSA_REG_ORDER), ECDSA_REG(area, ECDSA_REG_TMP_A), area->limb_cunt, &borrow);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_crypto_mont_mul(&area->scalar, ECDSA_REG(area, ECDSA_REG_TMP_B), ECDSA_REG(area, ECDSA_REG_SIG_S), ECDSA_REG(area, ECDSA_REG_ORDER_SQUARE), area->work);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ecdsa_reg_clear(area, ECDSA_REG_INV, ECDSA_SINGLE_REG_CUNT);
    inv[0] = ECDSA_IDENT_LIMB;

    ret = ldg_math_crypto_mont_mul(&area->scalar, inv, inv, ECDSA_REG(area, ECDSA_REG_ORDER_SQUARE), area->work);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (bit > 0)
    {
        bit--;
        ret = ecdsa_inv_bit_apply(area, bit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t ecdsa_scalar_compute(const ecdsa_area_t *area)
{
    uint32_t ret = 0;

    ret = ecdsa_inv_compute(area);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_crypto_mont_mul(&area->scalar, ECDSA_REG(area, ECDSA_REG_U_GEN), ECDSA_REG(area, ECDSA_REG_DIGEST), ECDSA_REG(area, ECDSA_REG_INV), area->work);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_crypto_mont_mul(&area->scalar, ECDSA_REG(area, ECDSA_REG_U_KEY), ECDSA_REG(area, ECDSA_REG_SIG_R), ECDSA_REG(area, ECDSA_REG_INV), area->work);
}

static uint32_t ecdsa_combo_bit_apply(const ecdsa_area_t *area, uint32_t bit)
{
    uint32_t sel = 0;
    uint32_t ret = 0;

    ret = ecdsa_point_double(area);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sel = ecdsa_bit_get(ECDSA_REG(area, ECDSA_REG_U_GEN), bit) | (ecdsa_bit_get(ECDSA_REG(area, ECDSA_REG_U_KEY), bit) << ECDSA_KEY_SEL_SHIFT);

    if (sel == 0) { return LDG_ERR_AOK; }

    ecdsa_reg_cpy(area, ECDSA_REG_ARG_X, ECDSA_REG_GEN_X + (ECDSA_POINT_REG_CUNT * (sel - ECDSA_ADJACENT_OFF)), ECDSA_POINT_REG_CUNT);

    return ecdsa_point_add(area);
}

static uint32_t ecdsa_combo_compute(const ecdsa_area_t *area)
{
    uint32_t bit = area->limb_cunt * ECDSA_LIMB_BITS;
    uint32_t ret = 0;

    ecdsa_reg_clear(area, ECDSA_REG_ACC_X, ECDSA_POINT_REG_CUNT);

    while (bit > 0)
    {
        bit--;
        ret = ecdsa_combo_bit_apply(area, bit);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint8_t ecdsa_acc_match_is(const ecdsa_area_t *area)
{
    uint32_t *square = ECDSA_REG(area, ECDSA_REG_TMP_A);
    uint32_t *lhs = ECDSA_REG(area, ECDSA_REG_TMP_B);
    uint32_t *wrapped = ECDSA_REG(area, ECDSA_REG_TMP_C);
    uint32_t carry = 0;
    uint32_t ret = 0;

    if (ldg_math_crypto_bignum_none_is(ECDSA_REG(area, ECDSA_REG_ACC_Z), area->limb_cunt) == LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    ret = ldg_math_crypto_mont_mul(&area->field, square, ECDSA_REG(area, ECDSA_REG_ACC_Z), ECDSA_REG(area, ECDSA_REG_ACC_Z), area->work);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    ret = ldg_math_crypto_mont_mul(&area->field, lhs, ECDSA_REG(area, ECDSA_REG_SIG_R), ECDSA_REG(area, ECDSA_REG_SQUARE), area->work);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    ret = ldg_math_crypto_mont_mul(&area->field, lhs, lhs, square, area->work);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    if (ecdsa_reg_eq_is(area, ECDSA_REG_TMP_B, ECDSA_REG_ACC_X) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    ret = ldg_math_crypto_bignum_add(wrapped, ECDSA_REG(area, ECDSA_REG_SIG_R), ECDSA_REG(area, ECDSA_REG_ORDER), area->limb_cunt, &carry);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    if (carry != 0 || ldg_math_crypto_bignum_ge_is(wrapped, ECDSA_REG(area, ECDSA_REG_PRIME), area->limb_cunt) == LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    ret = ldg_math_crypto_mont_mul(&area->field, lhs, wrapped, ECDSA_REG(area, ECDSA_REG_SQUARE), area->work);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    ret = ldg_math_crypto_mont_mul(&area->field, lhs, lhs, square, area->work);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    return ecdsa_reg_eq_is(area, ECDSA_REG_TMP_B, ECDSA_REG_ACC_X);
}

static uint32_t ecdsa_der_integer_get(const uint8_t *sig, uint32_t len, uint32_t *pos, const uint8_t **out, uint32_t *out_len)
{
    const uint8_t *body = 0x0;
    uint32_t body_len = 0;

    if ((len - *pos) < ECDSA_DER_HDR_SIZE || sig[*pos] != ECDSA_DER_INTEGER) { return LDG_ERR_INVALID; }

    body_len = sig[*pos + ECDSA_ADJACENT_OFF];

    if (body_len == 0 || body_len >= ECDSA_DER_LEN_CAP || body_len > (len - *pos - ECDSA_DER_HDR_SIZE)) { return LDG_ERR_INVALID; }

    body = sig + *pos + ECDSA_DER_HDR_SIZE;

    if (body[0] >= ECDSA_DER_SIGN_BIT) { return LDG_ERR_INVALID; }

    if (body_len > ECDSA_ADJACENT_OFF && body[0] == 0 && body[ECDSA_ADJACENT_OFF] < ECDSA_DER_SIGN_BIT) { return LDG_ERR_INVALID; }

    *out = body;
    *out_len = body_len;
    *pos += ECDSA_DER_HDR_SIZE + body_len;

    return LDG_ERR_AOK;
}

static uint32_t ecdsa_der_split(const uint8_t *sig, uint32_t len, const uint8_t **out_r, uint32_t *out_r_len, const uint8_t **out_s, uint32_t *out_s_len)
{
    uint32_t pos = ECDSA_DER_HDR_SIZE;
    uint32_t ret = 0;

    if (len < ECDSA_DER_HDR_SIZE || sig[0] != ECDSA_DER_SEQUENCE) { return LDG_ERR_INVALID; }

    if (sig[ECDSA_ADJACENT_OFF] >= ECDSA_DER_LEN_CAP || sig[ECDSA_ADJACENT_OFF] != (len - ECDSA_DER_HDR_SIZE)) { return LDG_ERR_INVALID; }

    ret = ecdsa_der_integer_get(sig, len, &pos, out_r, out_r_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ecdsa_der_integer_get(sig, len, &pos, out_s, out_s_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (pos != len) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}

static void ecdsa_area_set(ecdsa_area_t *area, ldg_math_crypto_ecdsa_curve_t curve, uint32_t *buff)
{
    area->size = (curve == LDG_MATH_CRYPTO_ECDSA_CURVE_P384) ? ECDSA_WIDE_SIZE : ECDSA_NARROW_SIZE;
    area->limb_cunt = area->size / ECDSA_LIMB_SIZE;
    area->reg = buff;
    area->work = buff + ((uint32_t)ECDSA_REG_CUNT * area->limb_cunt);
}

uint8_t ldg_math_crypto_ecdsa_sig_valid_is(ldg_math_crypto_ecdsa_curve_t curve, const uint8_t *key, uint32_t key_len, const uint8_t *digest, uint32_t digest_len, const uint8_t *sig_r, uint32_t sig_r_len, const uint8_t *sig_s, uint32_t sig_s_len, uint32_t *buff, uint32_t buff_cunt)
{
    ecdsa_area_t area = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!key || !digest || !sig_r || !sig_s || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY((uint32_t)curve >= (uint32_t)LDG_MATH_CRYPTO_ECDSA_CURVE_CUNT || digest_len == 0 || buff_cunt < LDG_MATH_CRYPTO_ECDSA_BUFF_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ecdsa_area_set(&area, curve, buff);

    ret = ecdsa_curve_ld(&area, curve);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    ret = ecdsa_key_ld(&area, key, key_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    ret = ecdsa_scalar_ld(&area, ECDSA_REG_SIG_R, sig_r, sig_r_len);
    if (ret == LDG_ERR_RANGE) { return LDG_TRUTH_FALSE; }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    ret = ecdsa_scalar_ld(&area, ECDSA_REG_SIG_S, sig_s, sig_s_len);
    if (ret == LDG_ERR_RANGE) { return LDG_TRUTH_FALSE; }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    ret = ecdsa_digest_ld(&area, digest, digest_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    ret = ecdsa_scalar_compute(&area);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    ret = ecdsa_combo_compute(&area);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    return ecdsa_acc_match_is(&area);
}

uint8_t ldg_math_crypto_ecdsa_der_valid_is(ldg_math_crypto_ecdsa_curve_t curve, const uint8_t *key, uint32_t key_len, const uint8_t *digest, uint32_t digest_len, const uint8_t *sig, uint32_t sig_len, uint32_t *buff, uint32_t buff_cunt)
{
    const uint8_t *sig_r = 0x0;
    const uint8_t *sig_s = 0x0;
    uint32_t sig_r_len = 0;
    uint32_t sig_s_len = 0;

    if (LDG_UNLIKELY(!sig)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ecdsa_der_split(sig, sig_len, &sig_r, &sig_r_len, &sig_s, &sig_s_len) != LDG_ERR_AOK) { return LDG_TRUTH_FALSE; }

    return ldg_math_crypto_ecdsa_sig_valid_is(curve, key, key_len, digest, digest_len, sig_r, sig_r_len, sig_s, sig_s_len, buff, buff_cunt);
}
