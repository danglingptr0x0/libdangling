
#include <dangling/math/astar.h>
#include <dangling/mem/alloc.h>
#include <dangling/core/err.h>
#include <dangling/mem/cpy.h>
#include <dangling/core/macros.h>

#define LDG_MATH_ASTAR_UNSEEN 0
#define LDG_MATH_ASTAR_STEP 1
#define LDG_MATH_ASTAR_OPEN 1
#define LDG_MATH_ASTAR_CLOSED 2

static uint32_t astar_alloc_doubles(uint64_t cunt, double **out)
{
    void *tmp = 0x0;
    uint32_t ret = 0;

    ret = ldg_mem_alloc(cunt * (uint64_t)sizeof(double), &tmp);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    *out = (double *)tmp;

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

    if (ctx->cost_buff) { ldg_mem_dealloc(ctx->cost_buff); }

    if (ctx->came_from) { ldg_mem_dealloc(ctx->came_from); }

    if (ctx->heap) { ldg_mem_dealloc(ctx->heap); }

    if (ctx->heap_pos) { ldg_mem_dealloc(ctx->heap_pos); }

    if (ctx->nbr_buff) { ldg_mem_dealloc(ctx->nbr_buff); }

    if (ctx->state) { ldg_mem_dealloc(ctx->state); }

    ldg_mem_dealloc(ctx);
}

static double astar_heur_eval(ldg_math_astar_heur_cb_t heur_cb, uint64_t node, uint64_t goal, void *user_ctx)
{
    double val = 0.0;

    if (heur_cb) { val = heur_cb(node, goal, user_ctx); }

    return val;
}

static uint8_t astar_heap_less(const ldg_math_astar_ctx_t *ctx, uint64_t node_a, uint64_t node_b)
{
    double f_a = 0.0;
    double f_b = 0.0;
    uint8_t res = UINT8_MAX;

    f_a = ctx->g_score[node_a] + ctx->h_score[node_a];
    f_b = ctx->g_score[node_b] + ctx->h_score[node_b];

    if (f_a < f_b) { res = LDG_TRUTH_TRUE; }
    else if (f_a > f_b) { res = LDG_TRUTH_FALSE; }
    else if (ctx->g_score[node_a] > ctx->g_score[node_b]) { res = LDG_TRUTH_TRUE; }
    else { res = LDG_TRUTH_FALSE; }

    return res;
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
        parent = (pos - 1) / 2;
        if (!astar_heap_less(ctx, ctx->heap[pos], ctx->heap[parent])) { return; }

        astar_heap_swap(ctx, pos, parent);
        pos = parent;
    }
}

static void astar_heap_sift_down(ldg_math_astar_ctx_t *ctx, uint64_t pos)
{
    uint64_t lhs = 0;
    uint64_t rhs = 0;
    uint64_t best = 0;

    while (1)
    {
        lhs = pos * 2 + 1;
        rhs = pos * 2 + 2;
        best = pos;

        if (lhs < ctx->heap_cunt && astar_heap_less(ctx, ctx->heap[lhs], ctx->heap[best])) { best = lhs; }

        if (rhs < ctx->heap_cunt && astar_heap_less(ctx, ctx->heap[rhs], ctx->heap[best])) { best = rhs; }

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
        astar_heap_sift_down(ctx, 0);
    }

    return top;
}

LDG_EXPORT uint32_t ldg_math_astar_ctx_create(uint64_t max_nodes, uint64_t max_degree, ldg_math_astar_ctx_t **out)
{
    ldg_math_astar_ctx_t *ctx = 0x0;
    void *tmp = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(max_nodes == 0 || max_degree == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(max_nodes > UINT64_MAX / (uint64_t)sizeof(double))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(max_degree > UINT64_MAX / (uint64_t)sizeof(double))) { return LDG_ERR_FUNC_ARG_INVALID; }

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

    ret = astar_alloc_doubles(max_nodes, &ctx->g_score);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        astar_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = astar_alloc_doubles(max_nodes, &ctx->h_score);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        astar_ctx_destroy_internal(ctx);
        return ret;
    }

    ret = astar_alloc_doubles(max_degree, &ctx->cost_buff);
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

LDG_EXPORT uint32_t ldg_math_astar_solve(ldg_math_astar_ctx_t *ctx, uint64_t start, uint64_t goal, ldg_math_astar_succ_cb_t succ_cb, ldg_math_astar_heur_cb_t heur_cb, void *user_ctx, uint64_t *path_out, uint64_t path_cap, uint64_t *path_cunt, double *total_cost)
{
    uint64_t idx = 0;
    uint64_t cur = 0;
    uint64_t nbr = 0;
    uint64_t nbr_cunt = 0;
    uint64_t node = 0;
    uint64_t path_len = 0;
    double edge_cost = 0.0;
    double cand_g = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !succ_cb || !path_out || !path_cunt || !total_cost)) { return LDG_ERR_FUNC_ARG_NULL; }

    *path_cunt = 0;
    *total_cost = 0.0;
    if (LDG_UNLIKELY(start >= ctx->max_nodes || goal >= ctx->max_nodes)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(path_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < ctx->max_nodes; idx++)
    {
        ctx->state[idx] = LDG_MATH_ASTAR_UNSEEN;
        ctx->came_from[idx] = UINT64_MAX;
        ctx->heap_pos[idx] = UINT64_MAX;
    }
    ctx->heap_cunt = 0;

    ctx->g_score[start] = 0.0;
    ctx->h_score[start] = astar_heur_eval(heur_cb, start, goal, user_ctx);
    ctx->state[start] = LDG_MATH_ASTAR_OPEN;
    ret = astar_heap_push(ctx, start);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (ctx->heap_cunt > 0)
    {
        cur = astar_heap_pop(ctx);
        ctx->state[cur] = LDG_MATH_ASTAR_CLOSED;

        if (cur == goal)
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

        ret = succ_cb(cur, ctx->nbr_buff, ctx->cost_buff, ctx->max_degree, &nbr_cunt, user_ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(nbr_cunt > ctx->max_degree)) { return LDG_ERR_BOUNDS; }

        for (idx = 0; idx < nbr_cunt; idx++)
        {
            nbr = ctx->nbr_buff[idx];
            edge_cost = ctx->cost_buff[idx];

            if (LDG_UNLIKELY(nbr >= ctx->max_nodes)) { return LDG_ERR_BOUNDS; }

            if (LDG_UNLIKELY(edge_cost < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            cand_g = ctx->g_score[cur] + edge_cost;

            if (ctx->state[nbr] == LDG_MATH_ASTAR_UNSEEN)
            {
                ctx->g_score[nbr] = cand_g;
                ctx->h_score[nbr] = astar_heur_eval(heur_cb, nbr, goal, user_ctx);
                ctx->came_from[nbr] = cur;
                ctx->state[nbr] = LDG_MATH_ASTAR_OPEN;
                ret = astar_heap_push(ctx, nbr);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
            else if (cand_g < ctx->g_score[nbr])
            {
                ctx->g_score[nbr] = cand_g;
                ctx->came_from[nbr] = cur;
                if (ctx->state[nbr] == LDG_MATH_ASTAR_OPEN) { astar_heap_sift_up(ctx, ctx->heap_pos[nbr]); }
                else
                {
                    ctx->state[nbr] = LDG_MATH_ASTAR_OPEN;
                    ret = astar_heap_push(ctx, nbr);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
                }
            }
        }
    }

    return LDG_ERR_NOT_FOUND;
}

LDG_EXPORT uint32_t ldg_math_astar_traverse(ldg_math_astar_ctx_t *ctx, uint64_t start, ldg_math_astar_succ_cb_t succ_cb, ldg_math_astar_heur_cb_t heur_cb, void *user_ctx, uint64_t *order_out, uint64_t order_cap, uint64_t *order_cunt, double *total_cost)
{
    uint64_t idx = 0;
    uint64_t cur = 0;
    uint64_t nbr = 0;
    uint64_t nbr_cunt = 0;
    uint64_t closed_cunt = 0;
    double edge_cost = 0.0;
    double cand_g = 0.0;
    double max_g = 0.0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !succ_cb || !order_out || !order_cunt || !total_cost)) { return LDG_ERR_FUNC_ARG_NULL; }

    *order_cunt = 0;
    *total_cost = 0.0;
    if (LDG_UNLIKELY(start >= ctx->max_nodes)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(order_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < ctx->max_nodes; idx++)
    {
        ctx->state[idx] = LDG_MATH_ASTAR_UNSEEN;
        ctx->came_from[idx] = UINT64_MAX;
        ctx->heap_pos[idx] = UINT64_MAX;
    }
    ctx->heap_cunt = 0;

    ctx->g_score[start] = 0.0;
    ctx->h_score[start] = astar_heur_eval(heur_cb, start, UINT64_MAX, user_ctx);
    ctx->state[start] = LDG_MATH_ASTAR_OPEN;
    ret = astar_heap_push(ctx, start);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (ctx->heap_cunt > 0)
    {
        cur = astar_heap_pop(ctx);
        ctx->state[cur] = LDG_MATH_ASTAR_CLOSED;

        if (LDG_UNLIKELY(closed_cunt >= order_cap)) { return LDG_ERR_BOUNDS; }

        order_out[closed_cunt] = cur;
        closed_cunt++;

        if (ctx->g_score[cur] > max_g) { max_g = ctx->g_score[cur]; }

        ret = succ_cb(cur, ctx->nbr_buff, ctx->cost_buff, ctx->max_degree, &nbr_cunt, user_ctx);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (LDG_UNLIKELY(nbr_cunt > ctx->max_degree)) { return LDG_ERR_BOUNDS; }

        for (idx = 0; idx < nbr_cunt; idx++)
        {
            nbr = ctx->nbr_buff[idx];
            edge_cost = ctx->cost_buff[idx];

            if (LDG_UNLIKELY(nbr >= ctx->max_nodes)) { return LDG_ERR_BOUNDS; }

            if (LDG_UNLIKELY(edge_cost < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

            if (ctx->state[nbr] == LDG_MATH_ASTAR_CLOSED) { continue; }

            cand_g = ctx->g_score[cur] + edge_cost;

            if (ctx->state[nbr] == LDG_MATH_ASTAR_UNSEEN)
            {
                ctx->g_score[nbr] = cand_g;
                ctx->h_score[nbr] = astar_heur_eval(heur_cb, nbr, UINT64_MAX, user_ctx);
                ctx->came_from[nbr] = cur;
                ctx->state[nbr] = LDG_MATH_ASTAR_OPEN;
                ret = astar_heap_push(ctx, nbr);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
            }
            else if (cand_g < ctx->g_score[nbr])
            {
                ctx->g_score[nbr] = cand_g;
                ctx->came_from[nbr] = cur;
                astar_heap_sift_up(ctx, ctx->heap_pos[nbr]);
            }
        }
    }

    *order_cunt = closed_cunt;
    *total_cost = max_g;

    return LDG_ERR_AOK;
}
