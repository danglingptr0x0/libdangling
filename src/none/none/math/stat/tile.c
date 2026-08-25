
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/stat.h>
#include <dangling/proto/emiemi.h>

static uint32_t ldg_math_stat_tile_hash(const uint32_t *coords, uint32_t cunt, uint32_t tiling, uint32_t weight_cap)
{
    uint32_t seed[LDG_MATH_STAT_TILE_DIMENSION_MAX + 2] = LDG_ARR_ZERO_INIT;
    uint32_t idx = 0;

    seed[0] = tiling;
    for (idx = 0; idx < cunt; idx++) { seed[idx + 1] = coords[idx]; }

    return ldg_emiemi_fnv1a_compute((const ldg_byte_t *)seed, (ldg_dword_t)((cunt + 1) * (uint32_t)sizeof(uint32_t))) % weight_cap;
}

LDG_EXPORT uint32_t ldg_math_stat_tile_init(ldg_math_stat_tile_t *tile, uint32_t state_dimension, uint32_t tiling_cunt, const uint32_t *tile_cunt_arr, const double *low, const double *high, uint32_t weight_cap, uint32_t action_max)
{
    uint32_t dimension = 0;

    if (LDG_UNLIKELY(!tile || !tile_cunt_arr || !low || !high)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(state_dimension == 0 || state_dimension > LDG_MATH_STAT_TILE_DIMENSION_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(tiling_cunt == 0 || tiling_cunt > LDG_MATH_STAT_TILE_TILING_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(weight_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(action_max == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (dimension = 0; dimension < state_dimension; dimension++) { if (LDG_UNLIKELY(high[dimension] <= low[dimension])) { return LDG_ERR_MATH_STAT_INVALID_DIST; } }
    for (dimension = 0; dimension < state_dimension; dimension++)
    {
        tile->tile_cunt[dimension] = tile_cunt_arr[dimension];
        tile->low[dimension] = low[dimension];
        tile->high[dimension] = high[dimension];
    }
    tile->weight_cap = weight_cap;
    tile->tiling_cunt = tiling_cunt;
    tile->state_dimension = state_dimension;
    tile->action_max = action_max;
    tile->is_init = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_stat_tile_feature_compute(const ldg_math_stat_tile_t *tile, const double *state, uint32_t action, uint32_t *idx_out)
{
    uint32_t base[LDG_MATH_STAT_TILE_DIMENSION_MAX + 1] = LDG_ARR_ZERO_INIT;
    uint32_t coords[LDG_MATH_STAT_TILE_DIMENSION_MAX + 1] = LDG_ARR_ZERO_INIT;
    uint32_t tiling = 0;
    uint32_t dimension = 0;
    double span = 0.0;
    double pos = 0.0;
    uint32_t coord = 0;

    if (LDG_UNLIKELY(!tile || !state || !idx_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(tile->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(action >= tile->action_max)) { return LDG_ERR_MATH_STAT_VFA_FEAT_INVALID; }

    for (dimension = 0; dimension < tile->state_dimension; dimension++)
    {
        span = tile->high[dimension] - tile->low[dimension];
        pos = (state[dimension] - tile->low[dimension]) / span * (double)tile->tile_cunt[dimension];
        if (pos < 0.0) { coord = 0; }
        else
        {
            coord = (uint32_t)pos;
            if (coord >= tile->tile_cunt[dimension]) { coord = tile->tile_cunt[dimension] - 1; }
        }

        base[dimension] = coord;
    }
    base[tile->state_dimension] = action;

    for (tiling = 0; tiling < tile->tiling_cunt; tiling++)
    {
        for (dimension = 0; dimension < tile->state_dimension; dimension++) { coords[dimension] = base[dimension] + ((2 * dimension + 1) * tiling) / tile->tiling_cunt; }
        coords[tile->state_dimension] = base[tile->state_dimension];
        idx_out[tiling] = ldg_math_stat_tile_hash(coords, tile->state_dimension + 1, tiling, tile->weight_cap);
    }
    return LDG_ERR_AOK;
}
