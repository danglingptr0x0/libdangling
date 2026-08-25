#include <stdint.h>

#include <dangling/math/rand.h>
#include <dangling/math/math.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_MATH_RAND_SPLIT_GAMMA 0x9E3779B97F4A7C15ULL
#define LDG_MATH_RAND_SPLIT_MIX_A 0xBF58476D1CE4E5B9ULL
#define LDG_MATH_RAND_SPLIT_MIX_B 0x94D049BB133111EBULL
#define LDG_MATH_RAND_SPLIT_SHIFT_A 30
#define LDG_MATH_RAND_SPLIT_SHIFT_B 27
#define LDG_MATH_RAND_SPLIT_SHIFT_C 31

#define LDG_MATH_RAND_SCRAMBLE_A 5
#define LDG_MATH_RAND_SCRAMBLE_B 9
#define LDG_MATH_RAND_ROTATE_A 7
#define LDG_MATH_RAND_ROTATE_B 45
#define LDG_MATH_RAND_SHIFT_LANE 17

#define LDG_MATH_RAND_WORD_BITS 64
#define LDG_MATH_RAND_MANTISSA_SHIFT 11
#define LDG_MATH_RAND_MANTISSA_SCALE 9007199254740992.0
#define LDG_MATH_RAND_FALLBACK_SEED 0x853C49E6748FEA9BULL

static uint64_t rand_bits_rotate(uint64_t val, uint32_t amt)
{
    return (val << amt) | (val >> (LDG_MATH_RAND_WORD_BITS - amt));
}

static uint64_t rand_split_next(uint64_t *seed)
{
    uint64_t val = 0;

    *seed += LDG_MATH_RAND_SPLIT_GAMMA;
    val = *seed;

    val = (val ^ (val >> LDG_MATH_RAND_SPLIT_SHIFT_A)) * LDG_MATH_RAND_SPLIT_MIX_A;
    val = (val ^ (val >> LDG_MATH_RAND_SPLIT_SHIFT_B)) * LDG_MATH_RAND_SPLIT_MIX_B;

    return val ^ (val >> LDG_MATH_RAND_SPLIT_SHIFT_C);
}

uint32_t ldg_math_rand_seed_set(ldg_math_rand_t *src, uint64_t seed)
{
    uint64_t mix = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    mix = seed;

    if (mix == 0) { mix = LDG_MATH_RAND_FALLBACK_SEED; }

    for (idx = 0; idx < LDG_MATH_RAND_LANE_CUNT; idx++) { src->lane[idx] = rand_split_next(&mix); }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_rand_next_get(ldg_math_rand_t *src, uint64_t *out)
{
    uint64_t draw = 0;
    uint64_t shifted = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    draw = rand_bits_rotate(src->lane[1] * LDG_MATH_RAND_SCRAMBLE_A, LDG_MATH_RAND_ROTATE_A) * LDG_MATH_RAND_SCRAMBLE_B;

    shifted = src->lane[1] << LDG_MATH_RAND_SHIFT_LANE;

    src->lane[2] ^= src->lane[0];
    src->lane[3] ^= src->lane[1];
    src->lane[1] ^= src->lane[2];
    src->lane[0] ^= src->lane[3];
    src->lane[2] ^= shifted;
    src->lane[3] = rand_bits_rotate(src->lane[3], LDG_MATH_RAND_ROTATE_B);

    *out = draw;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_rand_bounded_get(ldg_math_rand_t *src, uint64_t bound, uint64_t *out)
{
    uint64_t draw = 0;
    uint64_t limit = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(bound == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    limit = UINT64_MAX - (UINT64_MAX % bound);

    while (LDG_TRUTH_TRUE)
    {
        ret = ldg_math_rand_next_get(src, &draw);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (draw < limit) { break; }
    }

    *out = draw % bound;

    return LDG_ERR_AOK;
}

double ldg_math_rand_unit_get(ldg_math_rand_t *src)
{
    uint64_t draw = 0;

    if (LDG_UNLIKELY(ldg_math_rand_next_get(src, &draw) != LDG_ERR_AOK)) { return LDG_MATH_INFINITY; }

    return (double)(draw >> LDG_MATH_RAND_MANTISSA_SHIFT) / LDG_MATH_RAND_MANTISSA_SCALE;
}
