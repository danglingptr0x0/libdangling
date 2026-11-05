#include <stdint.h>

#include <dangling/term/focus.h>
#include <dangling/term/vt.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/mem.h>

#define LDG_TERM_FOCUS_POOL_ITEM_SIZE_VAR 0
#define LDG_TERM_FOCUS_POOL_ALLOC_CUNT 2

typedef struct focus_node
{
    ldg_term_focus_key_cb_t key_cb;
    void *user;
    uint64_t seq;
    uint32_t id;
    uint32_t parent_id;
    uint8_t used;
    uint8_t pudding[7];
} focus_node_t;

struct ldg_term_focus
{
    ldg_mem_pool_t *pool;
    focus_node_t *node;
    uint64_t seq_next;
    uint32_t node_cap;
    uint32_t node_cunt;
    uint32_t focused;
    uint8_t pudding[4];
};

static uint32_t focus_slot_get(const ldg_term_focus_t *focus, uint32_t node_id)
{
    uint32_t slot = 0;

    for (slot = 0; slot < focus->node_cap; slot++) { if (focus->node[slot].used == LDG_TRUTH_TRUE && focus->node[slot].id == node_id) { return slot; } }

    return UINT32_MAX;
}

static uint32_t focus_slot_free_get(const ldg_term_focus_t *focus)
{
    uint32_t slot = 0;

    for (slot = 0; slot < focus->node_cap; slot++) { if (focus->node[slot].used != LDG_TRUTH_TRUE) { return slot; } }

    return UINT32_MAX;
}

static uint8_t focus_child_has(const ldg_term_focus_t *focus, uint32_t node_id)
{
    uint32_t slot = 0;

    for (slot = 0; slot < focus->node_cap; slot++) { if (focus->node[slot].used == LDG_TRUTH_TRUE && focus->node[slot].parent_id == node_id) { return LDG_TRUTH_TRUE; } }

    return LDG_TRUTH_FALSE;
}

static uint32_t focus_slot_adjacent_get(const ldg_term_focus_t *focus, uint64_t seq, uint8_t after)
{
    uint64_t best_seq = 0;
    uint64_t wrap_seq = 0;
    uint32_t best = UINT32_MAX;
    uint32_t wrap = UINT32_MAX;
    uint32_t slot = 0;
    uint64_t cand = 0;

    for (slot = 0; slot < focus->node_cap; slot++)
    {
        if (focus->node[slot].used != LDG_TRUTH_TRUE) { continue; }

        cand = focus->node[slot].seq;

        if (after == LDG_TRUTH_TRUE)
        {
            if (cand > seq && (best == UINT32_MAX || cand < best_seq))
            {
                best = slot;
                best_seq = cand;
            }

            if (wrap == UINT32_MAX || cand < wrap_seq)
            {
                wrap = slot;
                wrap_seq = cand;
            }

            continue;
        }

        if (cand < seq && (best == UINT32_MAX || cand > best_seq))
        {
            best = slot;
            best_seq = cand;
        }

        if (wrap == UINT32_MAX || cand > wrap_seq)
        {
            wrap = slot;
            wrap_seq = cand;
        }
    }

    return (best != UINT32_MAX) ? best : wrap;
}

uint32_t ldg_term_focus_conf_default_get(ldg_term_focus_conf_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, (uint64_t)sizeof(*out));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->node_cap = LDG_TERM_FOCUS_NODE_CAP_DEFAULT;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_focus_create(const ldg_term_focus_conf_t *conf, ldg_term_focus_t **out)
{
    ldg_mem_pool_t *pool = 0x0;
    ldg_term_focus_t *focus = 0x0;
    void *mem = 0x0;
    uint64_t node_size = 0;
    uint64_t pool_cap = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    if (LDG_UNLIKELY(conf->node_cap == 0 || conf->node_cap > LDG_TERM_FOCUS_NODE_CAP_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    node_size = (uint64_t)conf->node_cap * (uint64_t)sizeof(focus_node_t);
    pool_cap = (uint64_t)sizeof(struct ldg_term_focus) + node_size + ((uint64_t)LDG_TERM_FOCUS_POOL_ALLOC_CUNT * LDG_MEM_POOL_VAR_ALIGN);

    ret = ldg_mem_pool_create(LDG_TERM_FOCUS_POOL_ITEM_SIZE_VAR, pool_cap, &pool);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_pool_alloc(pool, (uint64_t)sizeof(struct ldg_term_focus), &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK)) { return LDG_ERR_MEM_POOL_INVALID; }

        return ret;
    }

    focus = (ldg_term_focus_t *)mem;

    ret = ldg_mem_pool_alloc(pool, node_size, &mem);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK)) { return LDG_ERR_MEM_POOL_INVALID; }

        return ret;
    }

    ret = ldg_mem_zero(mem, node_size);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(ldg_mem_pool_destroy(&pool) != LDG_ERR_AOK)) { return LDG_ERR_MEM_POOL_INVALID; }

        return ret;
    }

    focus->pool = pool;
    focus->node = (focus_node_t *)mem;
    focus->seq_next = 0;
    focus->node_cap = conf->node_cap;
    focus->node_cunt = 0;
    focus->focused = LDG_TERM_FOCUS_NODE_NONE;
    focus->pudding[0] = 0;

    *out = focus;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_focus_destroy(ldg_term_focus_t *focus)
{
    ldg_mem_pool_t *pool = 0x0;

    if (LDG_UNLIKELY(!focus)) { return LDG_ERR_FUNC_ARG_NULL; }

    pool = focus->pool;

    return ldg_mem_pool_destroy(&pool);
}

uint32_t ldg_term_focus_node_add(ldg_term_focus_t *focus, uint32_t node_id, uint32_t parent_id, ldg_term_focus_key_cb_t key_cb, void *user)
{
    focus_node_t *node = 0x0;
    uint32_t slot = 0;

    if (LDG_UNLIKELY(!focus)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(node_id == LDG_TERM_FOCUS_NODE_NONE || node_id == parent_id)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(focus_slot_get(focus, node_id) != UINT32_MAX)) { return LDG_ERR_EXISTS; }

    if (LDG_UNLIKELY(parent_id != LDG_TERM_FOCUS_PARENT_NONE && focus_slot_get(focus, parent_id) == UINT32_MAX)) { return LDG_ERR_NOT_FOUND; }

    slot = focus_slot_free_get(focus);
    if (LDG_UNLIKELY(slot == UINT32_MAX)) { return LDG_ERR_FULL; }

    node = &focus->node[slot];
    node->key_cb = key_cb;
    node->user = user;
    node->seq = focus->seq_next;
    node->id = node_id;
    node->parent_id = parent_id;
    node->used = LDG_TRUTH_TRUE;

    focus->seq_next++;
    focus->node_cunt++;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_focus_node_remove(ldg_term_focus_t *focus, uint32_t node_id)
{
    uint32_t slot = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!focus)) { return LDG_ERR_FUNC_ARG_NULL; }

    slot = focus_slot_get(focus, node_id);
    if (LDG_UNLIKELY(slot == UINT32_MAX)) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(focus_child_has(focus, node_id) == LDG_TRUTH_TRUE)) { return LDG_ERR_BUSY; }

    ret = ldg_mem_zero(&focus->node[slot], (uint64_t)sizeof(focus_node_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    focus->node_cunt--;

    if (focus->focused == node_id) { focus->focused = LDG_TERM_FOCUS_NODE_NONE; }

    return LDG_ERR_AOK;
}

uint32_t ldg_term_focus_set(ldg_term_focus_t *focus, uint32_t node_id)
{
    if (LDG_UNLIKELY(!focus)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(focus_slot_get(focus, node_id) == UINT32_MAX)) { return LDG_ERR_NOT_FOUND; }

    focus->focused = node_id;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_focus_get(const ldg_term_focus_t *focus, uint32_t *out)
{
    if (LDG_UNLIKELY(!focus || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = focus->focused;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_focus_mov(ldg_term_focus_t *focus, uint32_t dir)
{
    uint64_t seq = UINT64_MAX;
    uint32_t slot = 0;
    uint8_t after = LDG_TRUTH_TRUE;

    if (LDG_UNLIKELY(!focus)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(dir != LDG_TERM_FOCUS_DIR_NEXT && dir != LDG_TERM_FOCUS_DIR_PREV)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(focus->node_cunt == 0)) { return LDG_ERR_EMPTY; }

    after = (dir == LDG_TERM_FOCUS_DIR_NEXT) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    slot = focus_slot_get(focus, focus->focused);
    if (slot != UINT32_MAX) { seq = focus->node[slot].seq; }
    else if (after == LDG_TRUTH_TRUE)
    {
        slot = focus_slot_adjacent_get(focus, UINT64_MAX, LDG_TRUTH_FALSE);
        seq = focus->node[slot].seq;
    }

    slot = focus_slot_adjacent_get(focus, seq, after);
    if (LDG_UNLIKELY(slot == UINT32_MAX)) { return LDG_ERR_MEM_CORRUPTION; }

    focus->focused = focus->node[slot].id;

    return LDG_ERR_AOK;
}

uint32_t ldg_term_focus_key_route(ldg_term_focus_t *focus, const ldg_term_key_event_t *event, uint8_t *consumed)
{
    const focus_node_t *node = 0x0;
    uint32_t node_id = 0;
    uint32_t slot = 0;
    uint32_t hop = 0;
    uint32_t ret = 0;
    uint8_t taken = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!focus || !event || !consumed)) { return LDG_ERR_FUNC_ARG_NULL; }

    *consumed = LDG_TRUTH_FALSE;

    node_id = focus->focused;

    for (hop = 0; hop < focus->node_cap && node_id != LDG_TERM_FOCUS_NODE_NONE; hop++)
    {
        slot = focus_slot_get(focus, node_id);
        if (LDG_UNLIKELY(slot == UINT32_MAX)) { return LDG_ERR_MEM_CORRUPTION; }

        node = &focus->node[slot];

        if (node->key_cb)
        {
            taken = LDG_TRUTH_FALSE;

            ret = node->key_cb(node->id, event, node->user, &taken);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (taken == LDG_TRUTH_TRUE)
            {
                *consumed = LDG_TRUTH_TRUE;
                return LDG_ERR_AOK;
            }
        }

        node_id = node->parent_id;
    }

    if (LDG_UNLIKELY(node_id != LDG_TERM_FOCUS_NODE_NONE)) { return LDG_ERR_MEM_CORRUPTION; }

    return LDG_ERR_AOK;
}
