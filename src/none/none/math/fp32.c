#include <stdint.h>

#include <dangling/math/math.h>
#include <dangling/core/macros.h>

#define MATH_FP32_UNDERFLOW 0.0
#define MATH_FP32_ROUND_OFFSET 0.5
#define MATH_FP32_RSQRT_COEF_A 1.5
#define MATH_FP32_RSQRT_COEF_B 0.5
#define MATH_FP32_RSQRT_SEED 0x5F3759DF
#define MATH_FP32_RSQRT_SHIFT 1
#define MATH_FP32_NEWTON_CUNT 3
#define MATH_FP32_MANTISSA_BITS 23
#define MATH_FP32_EXP_BIAS 127
#define MATH_FP32_LOG2E 1.44269504088896340736
#define MATH_FP32_LN2 0.69314718055994530942
#define MATH_FP32_EXP_ARG_MAX 88.0
#define MATH_FP32_EXP_ARG_MIN (-88.0)
#define MATH_FP32_HUGE 3.4028234e38
#define MATH_FP32_TAYLOR_0 1.0
#define MATH_FP32_TAYLOR_1 1.0
#define MATH_FP32_TAYLOR_2 0.5
#define MATH_FP32_TAYLOR_3 0.16666666666666666
#define MATH_FP32_TAYLOR_4 0.041666666666666664
#define MATH_FP32_TAYLOR_5 0.008333333333333333
#define MATH_FP32_TAYLOR_6 0.0013888888888888889

typedef union math_fp32_bits
{
    float val;
    uint32_t raw;
} math_fp32_bits_t;

float ldg_math_fp32_sqrt_get(float val)
{
    math_fp32_bits_t bits = LDG_STRUCT_ZERO_INIT;
    float est = 0.0;
    uint32_t iter = 0;

    if (val <= 0.0) { return MATH_FP32_UNDERFLOW; }

    bits.val = val;
    bits.raw = MATH_FP32_RSQRT_SEED - (bits.raw >> MATH_FP32_RSQRT_SHIFT);
    est = bits.val;

    while (iter < MATH_FP32_NEWTON_CUNT)
    {
        est = est * (MATH_FP32_RSQRT_COEF_A - (MATH_FP32_RSQRT_COEF_B * val * est * est));
        iter++;
    }

    return val * est;
}

float ldg_math_fp32_exp_get(float val)
{
    math_fp32_bits_t scale = LDG_STRUCT_ZERO_INIT;
    float scaled = 0.0;
    float rest = 0.0;
    float poly = 0.0;
    int32_t whole = 0;

    if (val > MATH_FP32_EXP_ARG_MAX) { return MATH_FP32_HUGE; }

    if (val < MATH_FP32_EXP_ARG_MIN) { return MATH_FP32_UNDERFLOW; }

    scaled = val * MATH_FP32_LOG2E;

    if (scaled >= 0.0) { whole = (int32_t)(scaled + MATH_FP32_ROUND_OFFSET); }
    else { whole = (int32_t)(scaled - MATH_FP32_ROUND_OFFSET); }

    rest = val - ((float)whole * MATH_FP32_LN2);

    poly = MATH_FP32_TAYLOR_6;
    poly = MATH_FP32_TAYLOR_5 + (poly * rest);
    poly = MATH_FP32_TAYLOR_4 + (poly * rest);
    poly = MATH_FP32_TAYLOR_3 + (poly * rest);
    poly = MATH_FP32_TAYLOR_2 + (poly * rest);
    poly = MATH_FP32_TAYLOR_1 + (poly * rest);
    poly = MATH_FP32_TAYLOR_0 + (poly * rest);

    scale.raw = (uint32_t)(whole + MATH_FP32_EXP_BIAS) << MATH_FP32_MANTISSA_BITS;

    return poly * scale.val;
}
