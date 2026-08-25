#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/math/predicate.h>
#include <dangling/math/tetrahedralize.h>
#include <dangling/math/cdt.h>

#define CDT_NONE 0xFFFFFFFFu
#define CDT_SLACK 8u

typedef struct cdt_tet
{
    uint32_t node[4];
    uint32_t neigh[4];
    uint8_t alive;
    uint8_t pudding[3];
} cdt_tet_t;

static uint8_t cdt_face_match(const cdt_tet_t *cell, uint32_t node_a, uint32_t node_b, uint32_t node_c, uint32_t *out_face)
{
    uint32_t val = 0x0;
    uint32_t miss = 0x0;
    uint32_t miss_cunt = 0x0;
    uint32_t idx = 0x0;
    for (idx = 0u; idx < 4u; idx++)
    {
        val = cell->node[idx];
        if (val != node_a && val != node_b && val != node_c)
        {
            miss = idx;
            miss_cunt++;
        }
    }
    if (miss_cunt == 1u)
    {
        *out_face = miss;
        return LDG_TRUTH_TRUE;
    }

    return LDG_TRUTH_FALSE;
}

static uint32_t cdt_apex_get(const cdt_tet_t *cell, uint32_t node_a, uint32_t node_b, uint32_t node_c)
{
    uint32_t idx = 0x0;
    for (idx = 0u; idx < 4u; idx++) { if (cell->node[idx] != node_a && cell->node[idx] != node_b && cell->node[idx] != node_c) { return cell->node[idx]; } }
    return CDT_NONE;
}

static uint32_t cdt_make(const double *vert, uint32_t node_a, uint32_t node_b, uint32_t node_c, uint32_t node_d, ldg_mem_pool_t *predicate_pool, cdt_tet_t *cell)
{
    uint8_t sign = 0x0;
    uint32_t err = 0x0;
    err = ldg_math_3d_orient_perturb(vert, node_a, node_b, node_c, node_d, predicate_pool, &sign);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (sign == LDG_MATH_SIGN_POS)
    {
        cell->node[0] = node_a;
        cell->node[1] = node_b;
    }
    else
    {
        cell->node[0] = node_b;
        cell->node[1] = node_a;
    }

    cell->node[2] = node_c;
    cell->node[3] = node_d;
    cell->neigh[0] = CDT_NONE;
    cell->neigh[1] = CDT_NONE;
    cell->neigh[2] = CDT_NONE;
    cell->neigh[3] = CDT_NONE;
    cell->alive = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

static void cdt_adjacency_build(cdt_tet_t *tet, uint32_t tet_cunt)
{
    uint32_t face[3] = LDG_ARR_ZERO_INIT;
    uint32_t lhs = 0x0;
    uint32_t rhs = 0x0;
    uint32_t face_lhs = 0x0;
    uint32_t face_rhs = 0x0;
    uint32_t fill = 0x0;
    uint32_t corner = 0x0;
    uint8_t matched = UINT8_MAX;
    for (lhs = 0u; lhs < tet_cunt; lhs++)
    {
        if (tet[lhs].alive != LDG_TRUTH_TRUE) { continue; }

        for (face_lhs = 0u; face_lhs < 4u; face_lhs++)
        {
            tet[lhs].neigh[face_lhs] = CDT_NONE;
            fill = 0u;
            for (corner = 0u; corner < 4u; corner++) { if (corner != face_lhs && fill < 3u)
                {
                    face[fill] = tet[lhs].node[corner];
                    fill++;
                }
            }
            for (rhs = 0u; rhs < tet_cunt; rhs++)
            {
                if (rhs == lhs || tet[rhs].alive != LDG_TRUTH_TRUE) { continue; }

                matched = cdt_face_match(&tet[rhs], face[0], face[1], face[2], &face_rhs);
                if (matched == LDG_TRUTH_TRUE)
                {
                    tet[lhs].neigh[face_lhs] = rhs;
                    break;
                }
            }
        }
    }
}

static uint8_t cdt_consistent_is(const cdt_tet_t *tet, uint32_t tet_cunt)
{
    uint32_t lhs = 0x0;
    uint32_t face_lhs = 0x0;
    uint32_t face_rhs = 0x0;
    uint32_t partner = 0x0;
    uint8_t mutual = UINT8_MAX;

    if (LDG_UNLIKELY(!tet)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (lhs = 0u; lhs < tet_cunt; lhs++)
    {
        if (tet[lhs].alive != LDG_TRUTH_TRUE) { continue; }

        for (face_lhs = 0u; face_lhs < 4u; face_lhs++)
        {
            partner = tet[lhs].neigh[face_lhs];
            if (partner == CDT_NONE) { continue; }

            if (partner >= tet_cunt || tet[partner].alive != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

            mutual = LDG_TRUTH_FALSE;
            for (face_rhs = 0u; face_rhs < 4u; face_rhs++) { if (tet[partner].neigh[face_rhs] == lhs) { mutual = LDG_TRUTH_TRUE; } }
            if (mutual != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }
        }
    }

    return LDG_TRUTH_TRUE;
}

static uint8_t cdt_point_inside_is(const double *vert, const cdt_tet_t *cell, const double *query, ldg_mem_pool_t *predicate_pool)
{
    const double *node_a = 0x0;
    const double *node_b = 0x0;
    const double *node_c = 0x0;
    const double *node_d = 0x0;
    uint8_t face_0 = 0x0;
    uint8_t face_1 = 0x0;
    uint8_t face_2 = 0x0;
    uint8_t face_3 = 0x0;
    uint32_t err = 0x0;

    if (LDG_UNLIKELY(!vert || !cell || !query)) { return LDG_ERR_FUNC_ARG_NULL; }

    node_a = vert + 3u * cell->node[0];
    node_b = vert + 3u * cell->node[1];
    node_c = vert + 3u * cell->node[2];
    node_d = vert + 3u * cell->node[3];
    err = ldg_math_3d_orient(query, node_b, node_c, node_d, predicate_pool, &face_0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return (uint8_t)err; }

    err = ldg_math_3d_orient(node_a, query, node_c, node_d, predicate_pool, &face_1);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return (uint8_t)err; }

    err = ldg_math_3d_orient(node_a, node_b, query, node_d, predicate_pool, &face_2);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return (uint8_t)err; }

    err = ldg_math_3d_orient(node_a, node_b, node_c, query, predicate_pool, &face_3);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return (uint8_t)err; }

    if (face_0 != LDG_MATH_SIGN_NEG && face_1 != LDG_MATH_SIGN_NEG && face_2 != LDG_MATH_SIGN_NEG && face_3 != LDG_MATH_SIGN_NEG) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t cdt_point_locate(const double *vert, const cdt_tet_t *tet, uint32_t tet_cunt, uint32_t start, const double *query, ldg_mem_pool_t *predicate_pool, uint32_t *located)
{
    const double *node_a = 0x0;
    const double *node_b = 0x0;
    const double *node_c = 0x0;
    const double *node_d = 0x0;
    uint32_t current = 0x0;
    uint32_t next = 0x0;
    uint32_t step = 0x0;
    uint8_t face_0 = 0x0;
    uint8_t face_1 = 0x0;
    uint8_t face_2 = 0x0;
    uint8_t face_3 = 0x0;
    uint32_t err = 0x0;
    *located = CDT_NONE;
    current = start;
    for (step = 0u; step <= tet_cunt; step++)
    {
        node_a = vert + 3u * tet[current].node[0];
        node_b = vert + 3u * tet[current].node[1];
        node_c = vert + 3u * tet[current].node[2];
        node_d = vert + 3u * tet[current].node[3];
        err = ldg_math_3d_orient(query, node_b, node_c, node_d, predicate_pool, &face_0);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_3d_orient(node_a, query, node_c, node_d, predicate_pool, &face_1);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_3d_orient(node_a, node_b, query, node_d, predicate_pool, &face_2);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        err = ldg_math_3d_orient(node_a, node_b, node_c, query, predicate_pool, &face_3);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        next = CDT_NONE;
        if (face_0 == LDG_MATH_SIGN_NEG) { next = tet[current].neigh[0]; }
        else if (face_1 == LDG_MATH_SIGN_NEG) { next = tet[current].neigh[1]; }
        else if (face_2 == LDG_MATH_SIGN_NEG) { next = tet[current].neigh[2]; }
        else if (face_3 == LDG_MATH_SIGN_NEG) { next = tet[current].neigh[3]; }
        else
        {
            *located = current;
            return LDG_ERR_AOK;
        }

        if (next == CDT_NONE) { return LDG_ERR_AOK; }

        current = next;
    }
    return LDG_ERR_AOK;
}

static uint32_t cdt_edge_ring_collect(const cdt_tet_t *tet, uint32_t tet_cunt, uint32_t node_a, uint32_t node_b, uint32_t *ring, uint32_t ring_cap, uint32_t *ring_cunt)
{
    uint32_t slot = 0x0;
    uint32_t corner = 0x0;
    uint8_t has_a = UINT8_MAX;
    uint8_t has_b = UINT8_MAX;
    *ring_cunt = 0u;
    for (slot = 0u; slot < tet_cunt; slot++)
    {
        if (tet[slot].alive != LDG_TRUTH_TRUE) { continue; }

        has_a = LDG_TRUTH_FALSE;
        has_b = LDG_TRUTH_FALSE;
        for (corner = 0u; corner < 4u; corner++)
        {
            if (tet[slot].node[corner] == node_a) { has_a = LDG_TRUTH_TRUE; }

            if (tet[slot].node[corner] == node_b) { has_b = LDG_TRUTH_TRUE; }
        }
        if (has_a == LDG_TRUTH_TRUE && has_b == LDG_TRUTH_TRUE)
        {
            if (*ring_cunt >= ring_cap) { return LDG_ERR_FULL; }

            ring[*ring_cunt] = slot;
            (*ring_cunt)++;
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_cdt_tetrahedralize(const double *vert, uint32_t vert_cunt, ldg_mem_pool_t *work_pool, ldg_mem_pool_t *predicate_pool, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt)
{
    void *block = 0x0;
    cdt_tet_t *tet = 0x0;
    uint64_t tet_bytes = 0x0;
    uint32_t new_slot[5] = LDG_ARR_ZERO_INIT;
    uint32_t shared[3] = LDG_ARR_ZERO_INIT;
    uint32_t flat_cunt = 0x0;
    uint32_t tet_cunt = 0x0;
    uint32_t tet_cap = 0x0;
    uint32_t flip_slot = 0x0;
    uint32_t flip_partner = 0x0;
    uint32_t apex_p = 0x0;
    uint32_t apex_d = 0x0;
    uint32_t slot = 0x0;
    uint32_t face_idx = 0x0;
    uint32_t partner = 0x0;
    uint32_t fill = 0x0;
    uint32_t corner = 0x0;
    uint32_t written = 0x0;
    uint8_t flippable = UINT8_MAX;
    uint8_t sign_ab = 0x0;
    uint8_t sign_bc = 0x0;
    uint8_t sign_ca = 0x0;
    uint32_t err = 0x0;
    double centroid[3] = LDG_ARR_ZERO_INIT;
    uint32_t ring[32] = LDG_ARR_ZERO_INIT;
    uint32_t located = 0x0;
    uint32_t ring_cunt = 0x0;
    uint32_t ring_idx = 0x0;
    uint32_t axis = 0x0;
    uint8_t inside = UINT8_MAX;
    uint8_t ring_found = UINT8_MAX;
    if (LDG_UNLIKELY(vert == 0x0 || work_pool == 0x0 || predicate_pool == 0x0 || out_idx == 0x0 || out_idx_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx_cunt = 0u;
    if (LDG_UNLIKELY(vert_cunt < 4u)) { return LDG_ERR_FUNC_ARG_INVALID; }

    err = ldg_math_delaunay_tetrahedralize(vert, vert_cunt, work_pool, predicate_pool, out_idx, out_idx_cap, &flat_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    tet_cunt = flat_cunt / 4u;
    if (tet_cunt == 0u) { return LDG_ERR_AOK; }

    err = ldg_mem_pool_rst(work_pool);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    tet_cap = tet_cunt + CDT_SLACK;
    tet_bytes = (uint64_t)tet_cap * (uint64_t)sizeof(cdt_tet_t);
    err = ldg_mem_pool_alloc(work_pool, tet_bytes, &block);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(block == 0x0)) { return LDG_ERR_ALLOC_NULL; }

    tet = block;
    for (slot = 0u; slot < tet_cunt; slot++)
    {
        tet[slot].node[0] = out_idx[slot * 4u + 0u];
        tet[slot].node[1] = out_idx[slot * 4u + 1u];
        tet[slot].node[2] = out_idx[slot * 4u + 2u];
        tet[slot].node[3] = out_idx[slot * 4u + 3u];
        tet[slot].alive = LDG_TRUTH_TRUE;
    }
    cdt_adjacency_build(tet, tet_cunt);
    if (cdt_consistent_is(tet, tet_cunt) != LDG_TRUTH_TRUE) { return LDG_ERR_MEM_CORRUPTION; }

    for (axis = 0u; axis < 3u; axis++) { centroid[axis] = (vert[tet[0].node[0] * 3u + axis] + vert[tet[0].node[1] * 3u + axis] + vert[tet[0].node[2] * 3u + axis] + vert[tet[0].node[3] * 3u + axis]) * 0.25; }
    err = cdt_point_locate(vert, tet, tet_cunt, tet_cunt - 1u, centroid, predicate_pool, &located);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (located == CDT_NONE) { return LDG_ERR_MEM_CORRUPTION; }

    inside = cdt_point_inside_is(vert, &tet[located], centroid, predicate_pool);
    if (LDG_UNLIKELY(inside != LDG_TRUTH_TRUE && inside != LDG_TRUTH_FALSE)) { return inside; }

    if (inside != LDG_TRUTH_TRUE) { return LDG_ERR_MEM_CORRUPTION; }

    err = cdt_edge_ring_collect(tet, tet_cunt, tet[0].node[0], tet[0].node[1], ring, 32u, &ring_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (ring_cunt == 0u) { return LDG_ERR_MEM_CORRUPTION; }

    ring_found = LDG_TRUTH_FALSE;
    for (ring_idx = 0u; ring_idx < ring_cunt; ring_idx++) { if (ring[ring_idx] == 0u) { ring_found = LDG_TRUTH_TRUE; } }
    if (ring_found != LDG_TRUTH_TRUE) { return LDG_ERR_MEM_CORRUPTION; }

    flippable = LDG_TRUTH_FALSE;
    for (slot = 0u; slot < tet_cunt; slot++)
    {
        if (tet[slot].alive != LDG_TRUTH_TRUE) { continue; }

        for (face_idx = 0u; face_idx < 4u; face_idx++)
        {
            partner = tet[slot].neigh[face_idx];
            if (partner == CDT_NONE || partner < slot) { continue; }

            fill = 0u;
            for (corner = 0u; corner < 4u; corner++) { if (corner != face_idx && fill < 3u)
                {
                    shared[fill] = tet[slot].node[corner];
                    fill++;
                }
            }
            apex_p = tet[slot].node[face_idx];
            apex_d = cdt_apex_get(&tet[partner], shared[0], shared[1], shared[2]);
            if (apex_d == CDT_NONE) { continue; }

            err = ldg_math_3d_orient_perturb(vert, shared[0], shared[1], apex_p, apex_d, predicate_pool, &sign_ab);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_math_3d_orient_perturb(vert, shared[1], shared[2], apex_p, apex_d, predicate_pool, &sign_bc);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_math_3d_orient_perturb(vert, shared[2], shared[0], apex_p, apex_d, predicate_pool, &sign_ca);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            if (sign_ab == sign_bc && sign_bc == sign_ca)
            {
                flippable = LDG_TRUTH_TRUE;
                flip_slot = slot;
                flip_partner = partner;
                break;
            }
        }
        if (flippable == LDG_TRUTH_TRUE) { break; }
    }
    if (flippable == LDG_TRUTH_TRUE && tet_cunt + 5u <= tet_cap)
    {
        tet[flip_slot].alive = LDG_TRUTH_FALSE;
        tet[flip_partner].alive = LDG_TRUTH_FALSE;
        new_slot[0] = tet_cunt;
        err = cdt_make(vert, apex_p, apex_d, shared[0], shared[1], predicate_pool, &tet[new_slot[0]]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        tet_cunt++;
        new_slot[1] = tet_cunt;
        err = cdt_make(vert, apex_p, apex_d, shared[1], shared[2], predicate_pool, &tet[new_slot[1]]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        tet_cunt++;
        new_slot[2] = tet_cunt;
        err = cdt_make(vert, apex_p, apex_d, shared[2], shared[0], predicate_pool, &tet[new_slot[2]]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        tet_cunt++;
        cdt_adjacency_build(tet, tet_cunt);
        if (cdt_consistent_is(tet, tet_cunt) != LDG_TRUTH_TRUE) { return LDG_ERR_MEM_CORRUPTION; }

        tet[new_slot[0]].alive = LDG_TRUTH_FALSE;
        tet[new_slot[1]].alive = LDG_TRUTH_FALSE;
        tet[new_slot[2]].alive = LDG_TRUTH_FALSE;
        new_slot[3] = tet_cunt;
        err = cdt_make(vert, shared[0], shared[1], shared[2], apex_p, predicate_pool, &tet[new_slot[3]]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        tet_cunt++;
        new_slot[4] = tet_cunt;
        err = cdt_make(vert, shared[0], shared[1], shared[2], apex_d, predicate_pool, &tet[new_slot[4]]);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        tet_cunt++;
        cdt_adjacency_build(tet, tet_cunt);
        if (cdt_consistent_is(tet, tet_cunt) != LDG_TRUTH_TRUE) { return LDG_ERR_MEM_CORRUPTION; }
    }

    written = 0u;
    for (slot = 0u; slot < tet_cunt; slot++)
    {
        if (tet[slot].alive != LDG_TRUTH_TRUE) { continue; }

        if (LDG_UNLIKELY(written + 4u > out_idx_cap)) { return LDG_ERR_FULL; }

        out_idx[written + 0u] = tet[slot].node[0];
        out_idx[written + 1u] = tet[slot].node[1];
        out_idx[written + 2u] = tet[slot].node[2];
        out_idx[written + 3u] = tet[slot].node[3];
        written += 4u;
    }
    *out_idx_cunt = written;
    return LDG_ERR_AOK;
}

static uint8_t cdt_edge_in_flat_is(const uint32_t *idx, uint32_t idx_cunt, uint32_t node_a, uint32_t node_b)
{
    uint32_t tet = 0x0;
    uint32_t corner = 0x0;
    uint8_t has_a = UINT8_MAX;
    uint8_t has_b = UINT8_MAX;

    if (LDG_UNLIKELY(!idx)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (tet = 0u; tet < idx_cunt / 4u; tet++)
    {
        has_a = LDG_TRUTH_FALSE;
        has_b = LDG_TRUTH_FALSE;
        for (corner = 0u; corner < 4u; corner++)
        {
            if (idx[tet * 4u + corner] == node_a) { has_a = LDG_TRUTH_TRUE; }

            if (idx[tet * 4u + corner] == node_b) { has_b = LDG_TRUTH_TRUE; }
        }
        if (has_a == LDG_TRUTH_TRUE && has_b == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }
    }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_math_cdt_build(const double *vert, uint32_t vert_cunt, const uint32_t *segment, uint32_t segment_cunt, uint32_t max_steiner, ldg_mem_pool_t *work_pool, ldg_mem_pool_t *segment_pool, ldg_mem_pool_t *predicate_pool, double *out_vert, uint32_t out_vert_cap, uint32_t *out_vert_cunt, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt)
{
    void *block = 0x0;
    uint32_t *active = 0x0;
    uint32_t *active_next = 0x0;
    uint32_t *hold = 0x0;
    uint64_t active_bytes = 0x0;
    uint32_t point_cunt = 0x0;
    uint32_t active_cunt = 0x0;
    uint32_t next_cunt = 0x0;
    uint32_t active_cap = 0x0;
    uint32_t steiner = 0x0;
    uint32_t flat_cunt = 0x0;
    uint32_t round = 0x0;
    uint32_t idx = 0x0;
    uint32_t segment_idx = 0x0;
    uint32_t node_a = 0x0;
    uint32_t node_b = 0x0;
    uint32_t midpoint = 0x0;
    uint32_t axis = 0x0;
    uint32_t added = 0x0;
    uint8_t is_edge = UINT8_MAX;
    uint32_t err = 0x0;
    if (LDG_UNLIKELY(vert == 0x0 || work_pool == 0x0 || segment_pool == 0x0 || predicate_pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_vert == 0x0 || out_vert_cunt == 0x0 || out_idx == 0x0 || out_idx_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(segment == 0x0 && segment_cunt != 0u)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_vert_cunt = 0u;
    *out_idx_cunt = 0u;
    if (LDG_UNLIKELY(vert_cunt < 4u)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY((uint64_t)vert_cunt * 3u > (uint64_t)out_vert_cap)) { return LDG_ERR_FULL; }

    for (idx = 0u; idx < vert_cunt * 3u; idx++) { out_vert[idx] = vert[idx]; }
    point_cunt = vert_cunt;
    active_cap = segment_cunt + max_steiner + 1u;
    err = ldg_mem_pool_rst(segment_pool);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    active_bytes = (uint64_t)active_cap * 2u * (uint64_t)sizeof(uint32_t);
    err = ldg_mem_pool_alloc(segment_pool, active_bytes, &block);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    active = block;
    err = ldg_mem_pool_alloc(segment_pool, active_bytes, &block);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    active_next = block;
    for (segment_idx = 0u; segment_idx < segment_cunt; segment_idx++)
    {
        active[segment_idx * 2u + 0u] = segment[segment_idx * 2u + 0u];
        active[segment_idx * 2u + 1u] = segment[segment_idx * 2u + 1u];
    }
    active_cunt = segment_cunt;
    steiner = 0u;
    for (round = 0u; round <= max_steiner; round++)
    {
        err = ldg_math_delaunay_tetrahedralize(out_vert, point_cunt, work_pool, predicate_pool, out_idx, out_idx_cap, &flat_cunt);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        next_cunt = 0u;
        added = 0u;
        for (segment_idx = 0u; segment_idx < active_cunt; segment_idx++)
        {
            node_a = active[segment_idx * 2u + 0u];
            node_b = active[segment_idx * 2u + 1u];
            is_edge = cdt_edge_in_flat_is(out_idx, flat_cunt, node_a, node_b);
            if (is_edge == LDG_TRUTH_TRUE || steiner >= max_steiner)
            {
                if (LDG_UNLIKELY(next_cunt >= active_cap)) { return LDG_ERR_FULL; }

                active_next[next_cunt * 2u + 0u] = node_a;
                active_next[next_cunt * 2u + 1u] = node_b;
                next_cunt++;
                continue;
            }

            if (LDG_UNLIKELY((uint64_t)(point_cunt + 1u) * 3u > (uint64_t)out_vert_cap)) { return LDG_ERR_FULL; }

            if (LDG_UNLIKELY(next_cunt + 2u > active_cap)) { return LDG_ERR_FULL; }

            midpoint = point_cunt;
            for (axis = 0u; axis < 3u; axis++) { out_vert[midpoint * 3u + axis] = (out_vert[node_a * 3u + axis] + out_vert[node_b * 3u + axis]) * 0.5; }
            point_cunt++;
            steiner++;
            active_next[next_cunt * 2u + 0u] = node_a;
            active_next[next_cunt * 2u + 1u] = midpoint;
            next_cunt++;
            active_next[next_cunt * 2u + 0u] = midpoint;
            active_next[next_cunt * 2u + 1u] = node_b;
            next_cunt++;
            added++;
        }
        hold = active;
        active = active_next;
        active_next = hold;
        active_cunt = next_cunt;
        if (added == 0u) { break; }
    }
    err = ldg_math_delaunay_tetrahedralize(out_vert, point_cunt, work_pool, predicate_pool, out_idx, out_idx_cap, &flat_cunt);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    *out_vert_cunt = point_cunt * 3u;
    *out_idx_cunt = flat_cunt;
    return LDG_ERR_AOK;
}

static uint32_t cdt_flip_23_apply(const double *vert, cdt_tet_t *tet, uint32_t *tet_cunt, uint32_t tet_cap, uint32_t slot, uint32_t partner, const uint32_t *shared, uint32_t apex_p, uint32_t apex_d, ldg_mem_pool_t *predicate_pool)
{
    uint32_t base = 0x0;
    uint32_t err = 0x0;
    if (LDG_UNLIKELY(*tet_cunt + 3u > tet_cap)) { return LDG_ERR_FULL; }

    tet[slot].alive = LDG_TRUTH_FALSE;
    tet[partner].alive = LDG_TRUTH_FALSE;
    base = *tet_cunt;
    err = cdt_make(vert, apex_p, apex_d, shared[0], shared[1], predicate_pool, &tet[base + 0u]);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = cdt_make(vert, apex_p, apex_d, shared[1], shared[2], predicate_pool, &tet[base + 1u]);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = cdt_make(vert, apex_p, apex_d, shared[2], shared[0], predicate_pool, &tet[base + 2u]);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    *tet_cunt = base + 3u;
    cdt_adjacency_build(tet, *tet_cunt);
    if (cdt_consistent_is(tet, *tet_cunt) != LDG_TRUTH_TRUE) { return LDG_ERR_MEM_CORRUPTION; }

    return LDG_ERR_AOK;
}

static uint32_t cdt_pierce_face_find(const double *vert, const cdt_tet_t *tet, uint32_t tet_cunt, uint32_t node_a, uint32_t node_b, ldg_mem_pool_t *predicate_pool, uint32_t *out_slot, uint32_t *out_partner, uint32_t *out_shared, uint8_t *flippable)
{
    uint32_t shared[3] = LDG_ARR_ZERO_INIT;
    uint32_t slot = 0x0;
    uint32_t face_idx = 0x0;
    uint32_t partner = 0x0;
    uint32_t apex_d = 0x0;
    uint32_t fill = 0x0;
    uint32_t corner = 0x0;
    uint8_t has_a = UINT8_MAX;
    uint8_t sign_ab = 0x0;
    uint8_t sign_bc = 0x0;
    uint8_t sign_ca = 0x0;
    uint32_t err = 0x0;
    *out_slot = CDT_NONE;
    *out_partner = CDT_NONE;
    *flippable = LDG_TRUTH_FALSE;
    for (slot = 0u; slot < tet_cunt; slot++)
    {
        if (tet[slot].alive != LDG_TRUTH_TRUE) { continue; }

        has_a = LDG_TRUTH_FALSE;
        for (corner = 0u; corner < 4u; corner++) { if (tet[slot].node[corner] == node_a) { has_a = LDG_TRUTH_TRUE; } }
        if (has_a != LDG_TRUTH_TRUE) { continue; }

        for (face_idx = 0u; face_idx < 4u; face_idx++)
        {
            if (tet[slot].node[face_idx] != node_a) { continue; }

            partner = tet[slot].neigh[face_idx];
            if (partner == CDT_NONE) { continue; }

            fill = 0u;
            for (corner = 0u; corner < 4u; corner++) { if (corner != face_idx && fill < 3u)
                {
                    shared[fill] = tet[slot].node[corner];
                    fill++;
                }
            }
            apex_d = cdt_apex_get(&tet[partner], shared[0], shared[1], shared[2]);
            if (apex_d != node_b) { continue; }

            err = ldg_math_3d_orient_perturb(vert, shared[0], shared[1], node_a, node_b, predicate_pool, &sign_ab);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_math_3d_orient_perturb(vert, shared[1], shared[2], node_a, node_b, predicate_pool, &sign_bc);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            err = ldg_math_3d_orient_perturb(vert, shared[2], shared[0], node_a, node_b, predicate_pool, &sign_ca);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            *out_slot = slot;
            *out_partner = partner;
            out_shared[0] = shared[0];
            out_shared[1] = shared[1];
            out_shared[2] = shared[2];
            if (sign_ab == sign_bc && sign_bc == sign_ca) { *flippable = LDG_TRUTH_TRUE; }

            return LDG_ERR_AOK;
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_cdt_segment_flip_recover(const double *vert, uint32_t node_a, uint32_t node_b, const uint32_t *in_idx, uint32_t in_idx_cunt, ldg_mem_pool_t *work_pool, ldg_mem_pool_t *predicate_pool, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt, uint8_t *recovered)
{
    void *block = 0x0;
    cdt_tet_t *tet = 0x0;
    uint32_t shared[3] = LDG_ARR_ZERO_INIT;
    uint64_t tet_bytes = 0x0;
    uint32_t tet_cunt = 0x0;
    uint32_t tet_cap = 0x0;
    uint32_t slot = 0x0;
    uint32_t corner = 0x0;
    uint32_t written = 0x0;
    uint32_t flip_slot = 0x0;
    uint32_t flip_partner = 0x0;
    uint8_t has_a = UINT8_MAX;
    uint8_t has_b = UINT8_MAX;
    uint8_t already = UINT8_MAX;
    uint8_t flippable = UINT8_MAX;
    uint32_t err = 0x0;
    if (LDG_UNLIKELY(vert == 0x0 || in_idx == 0x0 || work_pool == 0x0 || predicate_pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_idx == 0x0 || out_idx_cunt == 0x0 || recovered == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx_cunt = 0u;
    *recovered = 0u;
    if (LDG_UNLIKELY(in_idx_cunt < 4u || in_idx_cunt % 4u != 0u)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(node_a == node_b)) { return LDG_ERR_FUNC_ARG_INVALID; }

    tet_cunt = in_idx_cunt / 4u;
    err = ldg_mem_pool_rst(work_pool);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    tet_cap = tet_cunt + CDT_SLACK;
    tet_bytes = (uint64_t)tet_cap * (uint64_t)sizeof(cdt_tet_t);
    err = ldg_mem_pool_alloc(work_pool, tet_bytes, &block);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(block == 0x0)) { return LDG_ERR_ALLOC_NULL; }

    tet = block;
    for (slot = 0u; slot < tet_cunt; slot++)
    {
        tet[slot].node[0] = in_idx[slot * 4u + 0u];
        tet[slot].node[1] = in_idx[slot * 4u + 1u];
        tet[slot].node[2] = in_idx[slot * 4u + 2u];
        tet[slot].node[3] = in_idx[slot * 4u + 3u];
        tet[slot].alive = LDG_TRUTH_TRUE;
    }
    cdt_adjacency_build(tet, tet_cunt);
    if (cdt_consistent_is(tet, tet_cunt) != LDG_TRUTH_TRUE) { return LDG_ERR_MEM_CORRUPTION; }

    already = LDG_TRUTH_FALSE;
    for (slot = 0u; slot < tet_cunt; slot++)
    {
        has_a = LDG_TRUTH_FALSE;
        has_b = LDG_TRUTH_FALSE;
        for (corner = 0u; corner < 4u; corner++)
        {
            if (tet[slot].node[corner] == node_a) { has_a = LDG_TRUTH_TRUE; }

            if (tet[slot].node[corner] == node_b) { has_b = LDG_TRUTH_TRUE; }
        }
        if (has_a == LDG_TRUTH_TRUE && has_b == LDG_TRUTH_TRUE) { already = LDG_TRUTH_TRUE; }
    }
    if (already != LDG_TRUTH_TRUE)
    {
        err = cdt_pierce_face_find(vert, tet, tet_cunt, node_a, node_b, predicate_pool, &flip_slot, &flip_partner, shared, &flippable);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (flip_slot != CDT_NONE && flippable == LDG_TRUTH_TRUE)
        {
            err = cdt_flip_23_apply(vert, tet, &tet_cunt, tet_cap, flip_slot, flip_partner, shared, node_a, node_b, predicate_pool);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            already = LDG_TRUTH_TRUE;
        }
    }

    *recovered = already;
    written = 0u;
    for (slot = 0u; slot < tet_cunt; slot++)
    {
        if (tet[slot].alive != LDG_TRUTH_TRUE) { continue; }

        if (LDG_UNLIKELY(written + 4u > out_idx_cap)) { return LDG_ERR_FULL; }

        out_idx[written + 0u] = tet[slot].node[0];
        out_idx[written + 1u] = tet[slot].node[1];
        out_idx[written + 2u] = tet[slot].node[2];
        out_idx[written + 3u] = tet[slot].node[3];
        written += 4u;
    }
    *out_idx_cunt = written;
    return LDG_ERR_AOK;
}
