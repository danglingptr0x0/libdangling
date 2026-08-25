#ifndef LDG_MATH_RAND_H
#define LDG_MATH_RAND_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_MATH_RAND_LANE_CUNT 4

typedef struct ldg_math_rand
{
    uint64_t lane[LDG_MATH_RAND_LANE_CUNT];
} ldg_math_rand_t;

LDG_EXPORT uint32_t ldg_math_rand_seed_set(ldg_math_rand_t *src, uint64_t seed);
LDG_EXPORT uint32_t ldg_math_rand_next_get(ldg_math_rand_t *src, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_rand_bounded_get(ldg_math_rand_t *src, uint64_t bound, uint64_t *out);
LDG_EXPORT double ldg_math_rand_unit_get(ldg_math_rand_t *src);

#endif
