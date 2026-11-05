
#include <dangling/math/astar.h>
#include <dangling/math/rational.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/arith.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

#define LDG_MATH_ASTAR_UNSEEN 0
#define LDG_MATH_ASTAR_STEP 1
#define LDG_MATH_ASTAR_OPEN 1
#define LDG_MATH_ASTAR_CLOSED 2
#define LDG_MATH_ASTAR_COST_NONE 0
#define LDG_MATH_ASTAR_HEAP_ARITY 2
#define LDG_MATH_ASTAR_HEAP_LHS_OFF 1
#define LDG_MATH_ASTAR_HEAP_RHS_OFF 2
#define LDG_MATH_ASTAR_HEAP_ROOT_POS 0

static uint32_t astar_alloc_rational(uint64_t cunt, ldg_math_rational_t **out)
{
    void *tmp = 0x0;
    uint32_t ret = 0;

    ret = ldg_mem_alloc(cunt * (uint64_t)sizeof(ldg_math_rational_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    *out = (ldg_math_rational_t *)tmp;

    return LDG_ERR_AOK;
}

static uint32_t astar_alloc_u64(uint64_t cunt, uint64_t **out)
{
    void *tmp = 0x0;
    uint32_t ret = 0;

    ret = ldg_mem_alloc(cunt * (uint64_t)sizeof(uint64_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    *out = (uint64_t *)tmp;

    return LDG_ERR_AOK;
}

static uint32_t astar_alloc_u8(uint64_t cunt, uint8_t **out)
{
    void *tmp = 0x0;
    uint32_t ret = 0;

    ret = ldg_mem_alloc(cunt * (uint64_t)sizeof(uint8_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    *out = (uint8_t *)tmp;

    return LDG_ERR_AOK;
}

static void astar_ctx_destroy_internal(ldg_math_astar_ctx_t *ctx)
{
    if (ctx->g_score) { ldg_mem_dealloc(ctx->g_score); }

    if (ctx->h_score) { ldg_mem_dealloc(ctx->h_score); }

    if (ctx->f_score) { ldg_mem_dealloc(ctx->f_score); }

    if (ctx->cost_buff) { ldg_mem_dealloc(ctx->cost_buff); }

    if (ctx->came_from) { ldg_mem_dealloc(ctx->came_from); }

    if (ctx->heap) { ldg_mem_dealloc(ctx->heap); }

    if (ctx->heap_pos) { ldg_mem_dealloc(ctx->heap_pos); }

    if (ctx->nbr_buff) { ldg_mem_dealloc(ctx->nbr_buff); }

    if (ctx->state) { ldg_mem_dealloc(ctx->state); }

    ldg_mem_dealloc(ctx);
}

static uint32_t astar_heur_eval(ldg_math_astar_heur_cb_t heur_cb, uint64_t node, uint64_t goal, void *user_ctx, ldg_math_rational_t *out)
{
    uint32_t ret = 0;

    if (!heur_cb) { return ldg_math_rational_set(LDG_MATH_ASTAR_COST_NONE, LDG_MATH_RATIONAL_WHOLE_DEN, out); }

    ret = heur_cb(node, goal, user_ctx, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(out->den == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t astar_node_score_set(ldg_math_astar_ctx_t *ctx, uint64_t node, const ldg_math_rational_t *g_score)
{
    ctx->g_score[node] = *g_score;

    return ldg_math_rational_add(&ctx->g_score[node], &ctx->h_score[node], &ctx->f_score[node]);
}

static uint8_t astar_heap_less(const ldg_math_astar_ctx_t *ctx, uint64_t node_a, uint64_t node_b)
{
    uint32_t ord = 0;
    uint32_t ret = 0;

    ret = ldg_math_rational_cmp(&ctx->f_score[node_a], &ctx->f_score[node_b], &ord);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    if (ord == LDG_ARITH_ORD_LT) { return LDG_TRUTH_TRUE; }

    if (ord == LDG_ARITH_ORD_GT) { return LDG_TRUTH_FALSE; }

    ret = ldg_math_rational_cmp(&ctx->g_score[node_a], &ctx->g_score[node_b], &ord);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    if (ord == LDG_ARITH_ORD_GT) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static void astar_heap_swap(ldg_math_astar_ctx_t *ctx, uint64_t pos_a, uint64_t pos_b)
{
    uint64_t tmp = 0;

    tmp = ctx->heap[pos_a];
    ctx->heap[pos_a] = ctx->heap[pos_b];
    ctx->heap[pos_b] = tmp;

    ctx->heap_pos[ctx->heap[pos_a]] = pos_a;
    ctx->heap_pos[ctx->heap[pos_b]] = pos_b;
}

static void astar_heap_sift_up(ldg_math_astar_ctx_t *ctx, uint64_t pos)
{
    uint64_t parent = 0;

    while (pos > 0)
    {
        parent = (pos - LDG_MATH_ASTAR_HEAP_LHS_OFF) / LDG_MATH_ASTAR_HEAP_ARITY;
        if (astar_heap_less(ctx, ctx->heap[pos], ctx->heap[parent]) != LDG_TRUTH_TRUE) { return; }

        astar_heap_swap(ctx, pos, parent);
        pos = parent;
    }
}

static void astar_heap_sift_down(ldg_math_astar_ctx_t *ctx, uint64_t pos)
{
    uint64_t lhs = 0;
    uint64_t rhs = 0;
    uint64_t best = 0;

    while (LDG_TRUTH_TRUE)
    {
        lhs = pos * LDG_MATH_ASTAR_HEAP_ARITY + LDG_MATH_ASTAR_HEAP_LHS_OFF;
        rhs = pos * LDG_MATH_ASTAR_HEAP_ARITY + LDG_MATH_ASTAR_HEAP_RHS_OFF;
        best = pos;

        if (lhs < ctx->heap_cunt && astar_heap_less(ctx, ctx->heap[lhs], ctx->heap[best]) == LDG_TRUTH_TRUE) { best = lhs; }

        if (rhs < ctx->heap_cunt && astar_heap_less(ctx, ctx->heap[rhs], ctx->heap[best]) == LDG_TRUTH_TRUE) { best = rhs; }

        if (best == pos) { return; }

        astar_heap_swap(ctx, pos, best);
        pos = best;
    }
}

static uint32_t astar_heap_push(ldg_math_astar_ctx_t *ctx, uint64_t node)
{
    if (LDG_UNLIKELY(ctx->heap_cunt >= ctx->max_nodes)) { return LDG_ERR_FULL; }

    ctx->heap[ctx->heap_cunt] = node;
    ctx->heap_pos[node] = ctx->heap_cunt;
    ctx->heap_cunt++;
    astar_heap_sift_up(ctx, ctx->heap_cunt - LDG_MATH_ASTAR_STEP);

    return LDG_ERR_AOK;
}

static uint64_t astar_heap_pop(ldg_math_astar_ctx_t *ctx)
{
    uint64_t top = 0;

    top = ctx->heap[0];
    ctx->heap_pos[top] = UINT64_MAX;
    ctx->heap_cunt--;

    if (ctx->heap_cunt > 0)
    {
        ctx->heap[0] = ctx->heap[ctx->heap_cunt];
        ctx->heap_pos[ctx->heap[0]] = 0;
        astar_heap_sift_down(ctx, LDG_MATH_ASTAR_HEAP_ROOT_POS);
    }

    return top;
}

static uint32_t astar_search_rst(ldg_math_astar_ctx_t *ctx, uint64_t start, uint64_t goal, ldg_math_astar_heur_cb_t heur_cb, void *user_ctx)
{
    ldg_math_rational_t start_g = LDG_STRUCT_ZERO_INIT;
    uint64_t idx = 0;
    uint32_t ret = 0;

    for (idx = 0; idx < ctx->max_nodes; idx++)
    {
        ctx->state[idx] = LDG_MATH_ASTAR_UNSEEN;
        ctx->came_from[idx] = UINT64_MAX;
        ctx->heap_pos[idx] = UINT64_MAX;
    }
    ctx->heap_cunt = 0;

    ret = ldg_math_rational_set(LDG_MATH_ASTAR_COST_NONE, LDG_MATH_RATIONAL_WHOLE_DEN, &start_g);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = astar_heur_eval(heur_cb, start, goal, user_ctx, &ctx->h_score[start]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = astar_node_score_set(ctx, start, &start_g);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->state[start] = LDG_MATH_ASTAR_OPEN;

    return astar_heap_push(ctx, start);
}

static uint32_t astar_edge_relax(ldg_math_astar_ctx_t *ctx, uint64_t curr, uint64_t nbr, const ldg_math_rational_t *edge_cost, uint64_t goal, ldg_math_astar_heur_cb_t heur_cb, void *user_ctx)
{
    ldg_math_rational_t cand_g = LDG_STRUCT_ZERO_INIT;
    uint32_t ord = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(nbr >= ctx->max_nodes)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(edge_cost->num < 0 || edge_cost->den == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_rational_add(&ctx->g_score[curr], edge_cost, &cand_g);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ctx->state[nbr] == LDG_MATH_ASTAR_UNSEEN)
    {
        ret = astar_heur_eval(heur_cb, nbr, goal, user_ctx, &ctx->h_score[nbr]);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = astar_node_score_set(ctx, nbr, &cand_g);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ctx->came_from[nbr] = curr;
        ctx->state[nbr] = LDG_MATH_ASTAR_OPEN;

        return astar_heap_push(ctx, nbr);
    }

    ret = ldg_math_rational_cmp(&cand_g, &ctx->g_score[nbr], &ord);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (ord != LDG_ARITH_ORD_LT) { return LDG_ERR_AOK; }

    ret = astar_node_score_set(ctx, nbr, &cand_g);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->came_from[nbr] = curr;

    if (ctx->state[nbr] == LDG_MATH_ASTAR_OPEN)
    {
        astar_heap_sift_up(ctx, ctx->heap_pos[nbr]);
        return LDG_ERR_AOK;
    }

    ctx->state[nbr] = LDG_MATH_ASTAR_OPEN;

    return astar_heap_push(ctx, nbr);
}

LDG_EXPORT uint32_t ldg_math_astar_ctx_create(uint64_t max_nodes, uint64_t max_degree, ldg_math_astar_ctx_t **out)
{
    ldg_math_astar_ctx_t *ctx = 0x0;
    void *tmp = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(max_nodes == 0 || max_degree == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(max_nodes > UINT64_MAX / (uint64_t)sizeof(ldg_math_rational_t))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(max_degree > UINT64_MAX / (uint64_t)sizeof(ldg_math_rational_t))) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_alloc((uint64_t)sizeof(ldg_math_astar_ctx_t), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    ctx = (ldg_math_astar_ctx_t *)tmp;
    if (LDG_UNLIKELY(ldg_mem_zero(ctx, sizeof(ldg_math_astar_ctx_t)) != LDG_ERR_AOK))
    {
        ldg_mem_dealloc(ctx);
        return LDG_ERR_ALLOC_NULL;
    }

    ctx->max_nodes = max_nodes;
    ctx->max_degree = max_degree;

    ret = astar_alloc_rational(max_nodes, &ctx->g_score);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        astar_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = astar_alloc_rational(max_nodes, &ctx->h_score);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        astar_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = astar_alloc_rational(max_nodes, &ctx->f_score);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        astar_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = astar_alloc_rational(max_degree, &ctx->cost_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        astar_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = astar_alloc_u64(max_nodes, &ctx->came_from);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        astar_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = astar_alloc_u64(max_nodes, &ctx->heap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        astar_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = astar_alloc_u64(max_nodes, &ctx->heap_pos);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        astar_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = astar_alloc_u64(max_degree, &ctx->nbr_buff);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        astar_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = astar_alloc_u8(max_nodes, &ctx->state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        astar_ctx_destroy_internal(ctx);
        return ret;
    }

    *out = ctx;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_astar_ctx_destroy(ldg_math_astar_ctx_t **ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!*ctx)) { return LDG_ERR_AOK; }

    astar_ctx_destroy_internal(*ctx);
    *ctx = 0x0;

    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_astar_solve(ldg_math_astar_ctx_t *ctx, uint64_t start, uint64_t goal, ldg_math_astar_succ_cb_t succ_cb, ldg_math_astar_heur_cb_t heur_cb, void *user_ctx, uint64_t *path_out, uint64_t path_cap, uint64_t *path_cunt, ldg_math_rational_t *total_cost)
{
    uint64_t idx = 0;
    uint64_t curr = 0;
    uint64_t nbr_cunt = 0;
    uint64_t node = 0;
    uint64_t path_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !succ_cb || !path_out || !path_cunt || !total_cost)) { return LDG_ERR_FUNC_ARG_NULL; }

    *path_cunt = 0;
    ret = ldg_math_rational_set(LDG_MATH_ASTAR_COST_NONE, LDG_MATH_RATIONAL_WHOLE_DEN, total_cost);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(start >= ctx->max_nodes || goal >= ctx->max_nodes)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(path_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = astar_search_rst(ctx, start, goal, heur_cb, user_ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (ctx->heap_cunt > 0)
    {
        curr = astar_heap_pop(ctx);
        ctx->state[curr] = LDG_MATH_ASTAR_CLOSED;

        if (curr == goal)
        {
            node = goal;
            path_len = 0;
            while (path_len <= ctx->max_nodes)
            {
                path_len++;
                if (node == start) { break; }

                node = ctx->came_from[node];

                if (LDG_UNLIKELY(node >= ctx->max_nodes)) { return LDG_ERR_MEM_CORRUPTION; }
            }

            if (LDG_UNLIKELY(path_len > ctx->max_nodes)) { return LDG_ERR_MEM_CORRUPTION; }

            if (LDG_UNLIKELY(path_len > path_cap)) { return LDG_ERR_BOUNDS; }

            node = goal;
            idx = path_len;
            while (idx > 0)
            {
                idx--;
                path_out[idx] = node;
                node = ctx->came_from[node];
            }

            *path_cunt = path_len;
            *total_cost = ctx->g_score[goal];

            return LDG_ERR_AOK;
        }

        ret = succ_cb(curr, ctx->nbr_buff, ctx->cost_buff, ctx->max_degree, &nbr_cunt, user_ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(nbr_cunt > ctx->max_degree)) { return LDG_ERR_BOUNDS; }

        for (idx = 0; idx < nbr_cunt; idx++)
        {
            ret = astar_edge_relax(ctx, curr, ctx->nbr_buff[idx], &ctx->cost_buff[idx], goal, heur_cb, user_ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    return LDG_ERR_NOT_FOUND;
}

LDG_EXPORT uint32_t ldg_math_astar_traverse(ldg_math_astar_ctx_t *ctx, uint64_t start, ldg_math_astar_succ_cb_t succ_cb, ldg_math_astar_heur_cb_t heur_cb, void *user_ctx, uint64_t *order_out, uint64_t order_cap, uint64_t *order_cunt, ldg_math_rational_t *total_cost)
{
    ldg_math_rational_t max_g = LDG_STRUCT_ZERO_INIT;
    uint64_t idx = 0;
    uint64_t curr = 0;
    uint64_t nbr = 0;
    uint64_t nbr_cunt = 0;
    uint64_t closed_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !succ_cb || !order_out || !order_cunt || !total_cost)) { return LDG_ERR_FUNC_ARG_NULL; }

    *order_cunt = 0;
    ret = ldg_math_rational_set(LDG_MATH_ASTAR_COST_NONE, LDG_MATH_RATIONAL_WHOLE_DEN, total_cost);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(start >= ctx->max_nodes)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(order_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_math_rational_set(LDG_MATH_ASTAR_COST_NONE, LDG_MATH_RATIONAL_WHOLE_DEN, &max_g);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = astar_search_rst(ctx, start, UINT64_MAX, heur_cb, user_ctx);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (ctx->heap_cunt > 0)
    {
        curr = astar_heap_pop(ctx);
        ctx->state[curr] = LDG_MATH_ASTAR_CLOSED;

        if (LDG_UNLIKELY(closed_cunt >= order_cap)) { return LDG_ERR_BOUNDS; }

        order_out[closed_cunt] = curr;
        closed_cunt++;

        ret = ldg_math_rational_max(&max_g, &ctx->g_score[curr], &max_g);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = succ_cb(curr, ctx->nbr_buff, ctx->cost_buff, ctx->max_degree, &nbr_cunt, user_ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(nbr_cunt > ctx->max_degree)) { return LDG_ERR_BOUNDS; }

        for (idx = 0; idx < nbr_cunt; idx++)
        {
            nbr = ctx->nbr_buff[idx];

            if (LDG_UNLIKELY(nbr >= ctx->max_nodes)) { return LDG_ERR_BOUNDS; }

            if (ctx->state[nbr] == LDG_MATH_ASTAR_CLOSED) { continue; }

            ret = astar_edge_relax(ctx, curr, nbr, &ctx->cost_buff[idx], UINT64_MAX, heur_cb, user_ctx);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }
    }

    *order_cunt = closed_cunt;
    *total_cost = max_g;

    return LDG_ERR_AOK;
}
