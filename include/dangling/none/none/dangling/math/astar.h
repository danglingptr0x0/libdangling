#ifndef LDG_MATH_ASTAR_H
#define LDG_MATH_ASTAR_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/rational.h>

typedef uint32_t (*ldg_math_astar_succ_cb_t)(uint64_t node, uint64_t *nbr, ldg_math_rational_t *cost, uint64_t max_nbr, uint64_t *nbr_cunt, void *user_ctx);
typedef uint32_t (*ldg_math_astar_heur_cb_t)(uint64_t node, uint64_t goal, void *user_ctx, ldg_math_rational_t *out);

typedef struct ldg_math_astar_ctx
{
    ldg_math_rational_t *g_score;
    ldg_math_rational_t *h_score;
    ldg_math_rational_t *f_score;
    ldg_math_rational_t *cost_buff;
    uint64_t *came_from;
    uint64_t *heap;
    uint64_t *heap_pos;
    uint64_t *nbr_buff;
    uint8_t *state;
    uint64_t max_nodes;
    uint64_t max_degree;
    uint64_t heap_cunt;
} ldg_math_astar_ctx_t;

LDG_EXPORT uint32_t ldg_math_astar_ctx_create(uint64_t max_nodes, uint64_t max_degree, ldg_math_astar_ctx_t **out);
LDG_EXPORT uint32_t ldg_math_astar_ctx_destroy(ldg_math_astar_ctx_t **ctx);
LDG_EXPORT uint32_t ldg_math_astar_solve(ldg_math_astar_ctx_t *ctx, uint64_t start, uint64_t goal, ldg_math_astar_succ_cb_t succ_cb, ldg_math_astar_heur_cb_t heur_cb, void *user_ctx, uint64_t *path_out, uint64_t path_cap, uint64_t *path_cunt, ldg_math_rational_t *total_cost);
LDG_EXPORT uint32_t ldg_math_astar_traverse(ldg_math_astar_ctx_t *ctx, uint64_t start, ldg_math_astar_succ_cb_t succ_cb, ldg_math_astar_heur_cb_t heur_cb, void *user_ctx, uint64_t *order_out, uint64_t order_cap, uint64_t *order_cunt, ldg_math_rational_t *total_cost);

#endif
