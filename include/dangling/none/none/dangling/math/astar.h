#ifndef LDG_MATH_ASTAR_H
#define LDG_MATH_ASTAR_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/math.h>

typedef uint32_t (*ldg_math_astar_succ_cb_t)(uint64_t node, uint64_t *nbr, double *cost, uint64_t max_nbr, uint64_t *nbr_cunt, void *user_ctx);
typedef double (*ldg_math_astar_heur_cb_t)(uint64_t node, uint64_t goal, void *user_ctx);

typedef struct ldg_math_astar_ctx
{
    double *g_score;
    double *h_score;
    double *cost_buff;
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
LDG_EXPORT uint32_t ldg_math_astar_solve(ldg_math_astar_ctx_t *ctx, uint64_t start, uint64_t goal, ldg_math_astar_succ_cb_t succ_cb, ldg_math_astar_heur_cb_t heur_cb, void *user_ctx, uint64_t *path_out, uint64_t path_cap, uint64_t *path_cunt, double *total_cost);
LDG_EXPORT uint32_t ldg_math_astar_traverse(ldg_math_astar_ctx_t *ctx, uint64_t start, ldg_math_astar_succ_cb_t succ_cb, ldg_math_astar_heur_cb_t heur_cb, void *user_ctx, uint64_t *order_out, uint64_t order_cap, uint64_t *order_cunt, double *total_cost);

#endif
