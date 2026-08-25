#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/mem/alloc.h>
#include <dangling/math/triangulate.h>

typedef struct tri_node
{
    double pos_x;
    double pos_y;
    uint32_t vert_idx;
    uint32_t prev;
    uint32_t next;
    uint8_t pudding[4];
} tri_node_t;

typedef struct tri
{
    uint32_t va;
    uint32_t vb;
    uint32_t vc;
    uint8_t alive;
    uint8_t pudding[3];
} tri_t;

static double tri_area_get(double ax, double ay, double bx, double by, double cx, double cy)
{
    return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

static uint8_t tri_inside_is(double ax, double ay, double bx, double by, double cx, double cy, double px, double py)
{
    double side_a = 0.0;
    double side_b = 0.0;
    double side_c = 0.0;
    uint8_t has_neg = UINT8_MAX;
    uint8_t has_pos = UINT8_MAX;

    side_a = tri_area_get(ax, ay, bx, by, px, py);
    side_b = tri_area_get(bx, by, cx, cy, px, py);
    side_c = tri_area_get(cx, cy, ax, ay, px, py);
    has_neg = (side_a < 0.0 || side_b < 0.0 || side_c < 0.0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    has_pos = (side_a > 0.0 || side_b > 0.0 || side_c > 0.0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (has_neg == LDG_TRUTH_TRUE && has_pos == LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint8_t tri_inside_strict_is(double ax, double ay, double bx, double by, double cx, double cy, double px, double py)
{
    double side_a = tri_area_get(ax, ay, bx, by, px, py);
    double side_b = tri_area_get(bx, by, cx, cy, px, py);
    double side_c = tri_area_get(cx, cy, ax, ay, px, py);

    if ((side_a > 0.0 && side_b > 0.0 && side_c > 0.0) || (side_a < 0.0 && side_b < 0.0 && side_c < 0.0)) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t tri_incircle_is(double ax, double ay, double bx, double by, double cx, double cy, double dx, double dy)
{
    double adx = ax - dx;
    double ady = ay - dy;
    double bdx = bx - dx;
    double bdy = by - dy;
    double cdx = cx - dx;
    double cdy = cy - dy;
    double dist_a = adx * adx + ady * ady;
    double dist_b = bdx * bdx + bdy * bdy;
    double dist_c = cdx * cdx + cdy * cdy;
    double det = adx * (bdy * dist_c - cdy * dist_b) - ady * (bdx * dist_c - cdx * dist_b) + dist_a * (bdx * cdy - cdx * bdy);

    if (det > 0.0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t tri_segments_cross_is(double p0x, double p0y, double p1x, double p1y, double q0x, double q0y, double q1x, double q1y)
{
    double side_p0 = tri_area_get(q0x, q0y, q1x, q1y, p0x, p0y);
    double side_p1 = tri_area_get(q0x, q0y, q1x, q1y, p1x, p1y);
    double side_q0 = tri_area_get(p0x, p0y, p1x, p1y, q0x, q0y);
    double side_q1 = tri_area_get(p0x, p0y, p1x, p1y, q1x, q1y);
    uint8_t p_straddles = ((side_p0 > 0.0 && side_p1 < 0.0) || (side_p0 < 0.0 && side_p1 > 0.0)) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t q_straddles = ((side_q0 > 0.0 && side_q1 < 0.0) || (side_q0 < 0.0 && side_q1 > 0.0)) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (p_straddles == LDG_TRUTH_TRUE && q_straddles == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint8_t tri_edge_has(const tri_t *tri, uint32_t edge_from, uint32_t edge_to)
{
    uint8_t has_from = (tri->va == edge_from || tri->vb == edge_from || tri->vc == edge_from) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    uint8_t has_to = (tri->va == edge_to || tri->vb == edge_to || tri->vc == edge_to) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (has_from == LDG_TRUTH_TRUE && has_to == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t tri_apex_get(const tri_t *tri, uint32_t edge_from, uint32_t edge_to)
{
    if (tri->va != edge_from && tri->va != edge_to) { return tri->va; }

    if (tri->vb != edge_from && tri->vb != edge_to) { return tri->vb; }

    return tri->vc;
}

static uint8_t tri_add(tri_t *tri, uint32_t *tri_cunt, uint32_t tri_cap, const double *points, uint32_t va, uint32_t vb, uint32_t vc)
{
    uint32_t slot = 0;
    uint32_t scan = 0;
    uint32_t swap = 0;
    double orient = 0.0;

    orient = tri_area_get(points[va * 2], points[va * 2 + 1], points[vb * 2], points[vb * 2 + 1], points[vc * 2], points[vc * 2 + 1]);
    if (orient < 0.0)
    {
        swap = vb;
        vb = vc;
        vc = swap;
    }

    slot = *tri_cunt;
    for (scan = 0; scan < *tri_cunt; scan++) { if (tri[scan].alive != LDG_TRUTH_TRUE) { slot = scan; break; } }
    if (slot == *tri_cunt)
    {
        if (LDG_UNLIKELY(*tri_cunt >= tri_cap)) { return LDG_TRUTH_FALSE; }

        (*tri_cunt)++;
    }

    tri[slot].va = va;
    tri[slot].vb = vb;
    tri[slot].vc = vc;
    tri[slot].alive = LDG_TRUTH_TRUE;
    return LDG_TRUTH_TRUE;
}

uint32_t ldg_math_polygon_triangulate(const double *vert, uint32_t vert_cunt, const uint32_t *ring_start, uint32_t ring_cunt, ldg_mem_pool_t *pool, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt)
{
    tri_node_t *node = 0x0;
    uint64_t node_cap = 0;
    uint32_t node_cunt = 0;
    uint32_t outer_cunt = 0;
    uint32_t hd = 0;
    uint32_t ring = 0;
    uint32_t offset = 0;
    uint32_t node_at = 0;
    uint32_t hole_start = 0;
    uint32_t hole_end = 0;
    uint32_t hole_size = 0;
    uint32_t hole_base = 0;
    uint32_t bridge_hole = 0;
    uint32_t bridge_outer = 0;
    uint32_t visible = 0;
    uint32_t walk = 0;
    uint32_t walk_next = 0;
    uint32_t node_prev = 0;
    uint32_t node_next = 0;
    uint32_t anchor_copy = 0;
    uint32_t hole_copy = 0;
    uint32_t anchor_next = 0;
    uint32_t hole_prev = 0;
    uint32_t remaining = 0;
    uint32_t out_cunt = 0;
    uint32_t ear = 0;
    uint32_t ear_prev = 0;
    uint32_t ear_next = 0;
    uint32_t probe = 0;
    uint32_t probe_prev = 0;
    uint32_t probe_next = 0;
    uint32_t guard = 0;
    uint32_t status = 0;
    double mx = 0.0;
    double my = 0.0;
    double best_x = 0.0;
    double cross_x = 0.0;
    double view_x = 0.0;
    double view_y = 0.0;
    double min_tan = 0.0;
    double span_dx = 0.0;
    double span_tan = 0.0;
    uint8_t is_ear = UINT8_MAX;
    double ring_area = 0.0;
    uint32_t link_tmp = 0;

    if (LDG_UNLIKELY(!vert || !ring_start || !pool || !out_idx || !out_idx_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx_cunt = 0;

    if (LDG_UNLIKELY(ring_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    outer_cunt = ring_start[1] - ring_start[0];
    if (LDG_UNLIKELY(outer_cunt < 3)) { return LDG_ERR_FUNC_ARG_INVALID; }

    node_cap = (uint64_t)vert_cunt + 2 * (uint64_t)(ring_cunt - 1);
    status = ldg_mem_pool_alloc(pool, node_cap * sizeof(tri_node_t), (void **)&node);
    if (LDG_UNLIKELY(status != 0)) { return LDG_ERR_ALLOC_NULL; }

    for (offset = 0; offset < outer_cunt; offset++)
    {
        node_at = ring_start[0] + offset;
        node[offset].pos_x = vert[node_at * 2];
        node[offset].pos_y = vert[node_at * 2 + 1];
        node[offset].vert_idx = node_at;
        node[offset].prev = (offset + outer_cunt - 1) % outer_cunt;
        node[offset].next = (offset + 1) % outer_cunt;
    }
    node_cunt = outer_cunt;
    hd = 0;

    ring_area = 0.0;
    walk = hd;
    do
    {
        walk_next = node[walk].next;
        ring_area += node[walk].pos_x * node[walk_next].pos_y - node[walk_next].pos_x * node[walk].pos_y;
        walk = walk_next;
    }
    while (walk != hd);
    if (ring_area < 0.0) { for (offset = 0; offset < outer_cunt; offset++)
        {
            link_tmp = node[offset].prev;
            node[offset].prev = node[offset].next;
            node[offset].next = link_tmp;
        }
    }

    for (ring = 1; ring < ring_cunt; ring++)
    {
        hole_start = ring_start[ring];
        hole_end = ring_start[ring + 1];
        hole_size = hole_end - hole_start;
        if (LDG_UNLIKELY(hole_size < 3)) { return LDG_ERR_FUNC_ARG_INVALID; }

        hole_base = node_cunt;
        bridge_hole = hole_base;
        for (offset = 0; offset < hole_size; offset++)
        {
            node_at = hole_start + offset;
            node[hole_base + offset].pos_x = vert[node_at * 2];
            node[hole_base + offset].pos_y = vert[node_at * 2 + 1];
            node[hole_base + offset].vert_idx = node_at;
            node[hole_base + offset].prev = hole_base + (offset + hole_size - 1) % hole_size;
            node[hole_base + offset].next = hole_base + (offset + 1) % hole_size;
            if (node[hole_base + offset].pos_x > node[bridge_hole].pos_x) { bridge_hole = hole_base + offset; }
        }
        node_cunt += hole_size;

        mx = node[bridge_hole].pos_x;
        my = node[bridge_hole].pos_y;
        best_x = LDG_MATH_INFINITY;
        bridge_outer = node_cunt;
        walk = hd;
        do
        {
            walk_next = node[walk].next;
            if ((node[walk].pos_y < node[walk_next].pos_y || node[walk].pos_y > node[walk_next].pos_y) && ((node[walk].pos_y <= my && node[walk_next].pos_y >= my) || (node[walk_next].pos_y <= my && node[walk].pos_y >= my)))
            {
                cross_x = node[walk].pos_x + (my - node[walk].pos_y) / (node[walk_next].pos_y - node[walk].pos_y) * (node[walk_next].pos_x - node[walk].pos_x);
                if (cross_x >= mx && cross_x < best_x)
                {
                    best_x = cross_x;
                    bridge_outer = (node[walk].pos_x > node[walk_next].pos_x) ? walk : walk_next;
                }
            }

            walk = walk_next;
        }
        while (walk != hd);

        if (LDG_UNLIKELY(bridge_outer == node_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

        view_x = node[bridge_outer].pos_x;
        view_y = node[bridge_outer].pos_y;
        min_tan = LDG_MATH_INFINITY;
        visible = bridge_outer;
        walk = hd;
        do
        {
            if (walk != bridge_outer)
            {
                node_prev = node[walk].prev;
                node_next = node[walk].next;
                if (tri_area_get(node[node_prev].pos_x, node[node_prev].pos_y, node[walk].pos_x, node[walk].pos_y, node[node_next].pos_x, node[node_next].pos_y) < 0.0)
                {
                    if (tri_inside_is(mx, my, best_x, my, view_x, view_y, node[walk].pos_x, node[walk].pos_y) == LDG_TRUTH_TRUE)
                    {
                        span_dx = node[walk].pos_x - mx;
                        if (span_dx > 0.0)
                        {
                            span_tan = ((node[walk].pos_y - my) < 0.0 ? -(node[walk].pos_y - my) : (node[walk].pos_y - my)) / span_dx;
                            if (span_tan < min_tan)
                            {
                                min_tan = span_tan;
                                visible = walk;
                            }
                        }
                    }
                }
            }

            walk = node[walk].next;
        }
        while (walk != hd);
        bridge_outer = visible;

        anchor_copy = node_cunt;
        hole_copy = node_cunt + 1;
        node[anchor_copy] = node[bridge_outer];
        node[hole_copy] = node[bridge_hole];
        anchor_next = node[bridge_outer].next;
        hole_prev = node[bridge_hole].prev;
        node[bridge_outer].next = bridge_hole;
        node[bridge_hole].prev = bridge_outer;
        node[anchor_copy].next = anchor_next;
        node[anchor_next].prev = anchor_copy;
        node[hole_copy].next = anchor_copy;
        node[anchor_copy].prev = hole_copy;
        node[hole_prev].next = hole_copy;
        node[hole_copy].prev = hole_prev;
        node_cunt += 2;
    }

    remaining = node_cunt;
    out_cunt = 0;
    ear = hd;
    guard = 0;
    while (remaining > 3)
    {
        ear_prev = node[ear].prev;
        ear_next = node[ear].next;
        is_ear = LDG_TRUTH_FALSE;
        if (tri_area_get(node[ear_prev].pos_x, node[ear_prev].pos_y, node[ear].pos_x, node[ear].pos_y, node[ear_next].pos_x, node[ear_next].pos_y) > 0.0)
        {
            is_ear = LDG_TRUTH_TRUE;
            probe = node[ear_next].next;
            while (probe != ear_prev)
            {
                probe_prev = node[probe].prev;
                probe_next = node[probe].next;
                if (tri_area_get(node[probe_prev].pos_x, node[probe_prev].pos_y, node[probe].pos_x, node[probe].pos_y, node[probe_next].pos_x, node[probe_next].pos_y) < 0.0) { if (tri_inside_strict_is(node[ear_prev].pos_x, node[ear_prev].pos_y, node[ear].pos_x, node[ear].pos_y, node[ear_next].pos_x, node[ear_next].pos_y, node[probe].pos_x, node[probe].pos_y) == LDG_TRUTH_TRUE)
                    {
                        is_ear = LDG_TRUTH_FALSE;
                        break;
                    }
                }

                probe = node[probe].next;
            }
        }

        if (is_ear == LDG_TRUTH_TRUE)
        {
            if (LDG_UNLIKELY(out_cunt + 3 > out_idx_cap)) { return LDG_ERR_BOUNDS; }

            out_idx[out_cunt] = node[ear_prev].vert_idx;
            out_idx[out_cunt + 1] = node[ear].vert_idx;
            out_idx[out_cunt + 2] = node[ear_next].vert_idx;
            out_cunt += 3;
            node[ear_prev].next = ear_next;
            node[ear_next].prev = ear_prev;
            remaining--;
            ear = ear_next;
            guard = 0;
        }
        else
        {
            ear = ear_next;
            guard++;
            if (guard > remaining)
            {
                probe = ear;
                is_ear = LDG_TRUTH_FALSE;
                for (walk = 0; walk < remaining; walk++)
                {
                    probe_prev = node[probe].prev;
                    probe_next = node[probe].next;
                    ring_area = tri_area_get(node[probe_prev].pos_x, node[probe_prev].pos_y, node[probe].pos_x, node[probe].pos_y, node[probe_next].pos_x, node[probe_next].pos_y);
                    if ((ring_area < 0.0 ? -ring_area : ring_area) <= LDG_MATH_EPSILON)
                    {
                        node[probe_prev].next = probe_next;
                        node[probe_next].prev = probe_prev;
                        remaining--;
                        ear = probe_next;
                        guard = 0;
                        is_ear = LDG_TRUTH_TRUE;
                        break;
                    }

                    probe = node[probe].next;
                }
                if (LDG_UNLIKELY(is_ear != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }
            }
        }
    }

    ear_prev = node[ear].prev;
    ear_next = node[ear].next;
    if (LDG_UNLIKELY(out_cunt + 3 > out_idx_cap)) { return LDG_ERR_BOUNDS; }

    out_idx[out_cunt] = node[ear_prev].vert_idx;
    out_idx[out_cunt + 1] = node[ear].vert_idx;
    out_idx[out_cunt + 2] = node[ear_next].vert_idx;
    out_cunt += 3;

    *out_idx_cunt = out_cunt;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_delaunay_triangulate(const double *vert, uint32_t vert_cunt, const uint32_t *segment, uint32_t segment_cunt, ldg_mem_pool_t *pool, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt)
{
    double *points = 0x0;
    tri_t *tri = 0x0;
    uint32_t *bad = 0x0;
    uint32_t *edge_from_list = 0x0;
    uint32_t *edge_to_list = 0x0;
    uint32_t *new_from_list = 0x0;
    uint32_t *new_to_list = 0x0;
    uint32_t point_total = 0;
    uint32_t tri_cap = 0;
    uint32_t tri_cunt = 0;
    uint32_t bad_cunt = 0;
    uint32_t edge_cunt = 0;
    uint32_t cross_hd = 0;
    uint32_t cross_tail = 0;
    uint32_t cross_live = 0;
    uint32_t cross_cap = 0;
    uint32_t new_cunt = 0;
    uint32_t defer_run = 0;
    uint32_t super0 = 0;
    uint32_t super1 = 0;
    uint32_t super2 = 0;
    uint32_t point_idx = 0;
    uint32_t tri_idx = 0;
    uint32_t bad_idx = 0;
    uint32_t bad_idx2 = 0;
    uint32_t bad_tri = 0;
    uint32_t edge_idx = 0;
    uint32_t corner = 0;
    uint32_t edge_from = 0;
    uint32_t edge_to = 0;
    uint32_t seg_idx = 0;
    uint32_t seg_a = 0;
    uint32_t seg_b = 0;
    uint32_t lhs_tri = 0;
    uint32_t rhs_tri = 0;
    uint32_t apex_lhs = 0;
    uint32_t apex_rhs = 0;
    uint32_t guard = 0;
    uint32_t out_cunt = 0;
    uint32_t status = 0;
    uint32_t tri_corner[3] = { 0, 0, 0 };
    double min_x = 0.0;
    double min_y = 0.0;
    double max_x = 0.0;
    double max_y = 0.0;
    double span = 0.0;
    double mid_x = 0.0;
    double mid_y = 0.0;
    double big = 0.0;
    uint8_t shared = UINT8_MAX;
    uint8_t found = UINT8_MAX;

    if (LDG_UNLIKELY(!vert || !pool || !out_idx || !out_idx_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(segment_cunt != 0 && !segment)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx_cunt = 0;

    if (LDG_UNLIKELY(vert_cunt < 3)) { return LDG_ERR_FUNC_ARG_INVALID; }

    point_total = vert_cunt + 3;
    tri_cap = 2 * point_total + 16;

    status = ldg_mem_pool_alloc(pool, (uint64_t)point_total * 2 * sizeof(double), (void **)&points);
    if (LDG_UNLIKELY(status != 0)) { return LDG_ERR_ALLOC_NULL; }

    status = ldg_mem_pool_alloc(pool, (uint64_t)tri_cap * sizeof(tri_t), (void **)&tri);
    if (LDG_UNLIKELY(status != 0)) { return LDG_ERR_ALLOC_NULL; }

    status = ldg_mem_pool_alloc(pool, (uint64_t)tri_cap * sizeof(uint32_t), (void **)&bad);
    if (LDG_UNLIKELY(status != 0)) { return LDG_ERR_ALLOC_NULL; }

    status = ldg_mem_pool_alloc(pool, (uint64_t)tri_cap * 3 * sizeof(uint32_t), (void **)&edge_from_list);
    if (LDG_UNLIKELY(status != 0)) { return LDG_ERR_ALLOC_NULL; }

    status = ldg_mem_pool_alloc(pool, (uint64_t)tri_cap * 3 * sizeof(uint32_t), (void **)&edge_to_list);
    if (LDG_UNLIKELY(status != 0)) { return LDG_ERR_ALLOC_NULL; }

    status = ldg_mem_pool_alloc(pool, (uint64_t)tri_cap * 3 * sizeof(uint32_t), (void **)&new_from_list);
    if (LDG_UNLIKELY(status != 0)) { return LDG_ERR_ALLOC_NULL; }

    status = ldg_mem_pool_alloc(pool, (uint64_t)tri_cap * 3 * sizeof(uint32_t), (void **)&new_to_list);
    if (LDG_UNLIKELY(status != 0)) { return LDG_ERR_ALLOC_NULL; }

    cross_cap = tri_cap * 3;

    min_x = vert[0];
    max_x = vert[0];
    min_y = vert[1];
    max_y = vert[1];
    for (point_idx = 0; point_idx < vert_cunt; point_idx++)
    {
        points[point_idx * 2] = vert[point_idx * 2];
        points[point_idx * 2 + 1] = vert[point_idx * 2 + 1];
        if (vert[point_idx * 2] < min_x) { min_x = vert[point_idx * 2]; }

        if (vert[point_idx * 2] > max_x) { max_x = vert[point_idx * 2]; }

        if (vert[point_idx * 2 + 1] < min_y) { min_y = vert[point_idx * 2 + 1]; }

        if (vert[point_idx * 2 + 1] > max_y) { max_y = vert[point_idx * 2 + 1]; }
    }

    span = (max_x - min_x > max_y - min_y) ? (max_x - min_x) : (max_y - min_y);
    if (span <= 0.0) { span = 1.0; }

    mid_x = (min_x + max_x) * 0.5;
    mid_y = (min_y + max_y) * 0.5;
    big = span * 1000.0;

    super0 = vert_cunt;
    super1 = vert_cunt + 1;
    super2 = vert_cunt + 2;
    points[super0 * 2] = mid_x - 2.0 * big;
    points[super0 * 2 + 1] = mid_y - big;
    points[super1 * 2] = mid_x + 2.0 * big;
    points[super1 * 2 + 1] = mid_y - big;
    points[super2 * 2] = mid_x;
    points[super2 * 2 + 1] = mid_y + 2.0 * big;

    tri[0].va = super0;
    tri[0].vb = super1;
    tri[0].vc = super2;
    tri[0].alive = LDG_TRUTH_TRUE;
    tri_cunt = 1;

    for (point_idx = 0; point_idx < vert_cunt; point_idx++)
    {
        bad_cunt = 0;
        for (tri_idx = 0; tri_idx < tri_cunt; tri_idx++)
        {
            if (tri[tri_idx].alive != LDG_TRUTH_TRUE) { continue; }

            if (tri_incircle_is(points[tri[tri_idx].va * 2], points[tri[tri_idx].va * 2 + 1], points[tri[tri_idx].vb * 2], points[tri[tri_idx].vb * 2 + 1], points[tri[tri_idx].vc * 2], points[tri[tri_idx].vc * 2 + 1], points[point_idx * 2], points[point_idx * 2 + 1]) == LDG_TRUTH_TRUE)
            {
                bad[bad_cunt] = tri_idx;
                bad_cunt++;
            }
        }

        edge_cunt = 0;
        for (bad_idx = 0; bad_idx < bad_cunt; bad_idx++)
        {
            bad_tri = bad[bad_idx];
            tri_corner[0] = tri[bad_tri].va;
            tri_corner[1] = tri[bad_tri].vb;
            tri_corner[2] = tri[bad_tri].vc;
            for (corner = 0; corner < 3; corner++)
            {
                edge_from = tri_corner[corner];
                edge_to = tri_corner[(corner + 1) % 3];
                shared = LDG_TRUTH_FALSE;
                for (bad_idx2 = 0; bad_idx2 < bad_cunt; bad_idx2++)
                {
                    if (bad_idx2 == bad_idx) { continue; }

                    if (tri_edge_has(&tri[bad[bad_idx2]], edge_from, edge_to) == LDG_TRUTH_TRUE) { shared = LDG_TRUTH_TRUE; break; }
                }
                if (shared != LDG_TRUTH_TRUE)
                {
                    edge_from_list[edge_cunt] = edge_from;
                    edge_to_list[edge_cunt] = edge_to;
                    edge_cunt++;
                }
            }
        }

        for (bad_idx = 0; bad_idx < bad_cunt; bad_idx++) { tri[bad[bad_idx]].alive = LDG_TRUTH_FALSE; }

        for (edge_idx = 0; edge_idx < edge_cunt; edge_idx++) { if (LDG_UNLIKELY(tri_add(tri, &tri_cunt, tri_cap, points, edge_from_list[edge_idx], edge_to_list[edge_idx], point_idx) != LDG_TRUTH_TRUE)) { return LDG_ERR_ALLOC_NULL; } }
    }

    for (seg_idx = 0; seg_idx < segment_cunt; seg_idx++)
    {
        seg_a = segment[seg_idx * 2];
        seg_b = segment[seg_idx * 2 + 1];
        if (LDG_UNLIKELY(seg_a >= vert_cunt || seg_b >= vert_cunt || seg_a == seg_b)) { return LDG_ERR_FUNC_ARG_INVALID; }

        found = LDG_TRUTH_FALSE;
        for (tri_idx = 0; tri_idx < tri_cunt; tri_idx++) { if (tri[tri_idx].alive == LDG_TRUTH_TRUE && tri_edge_has(&tri[tri_idx], seg_a, seg_b) == LDG_TRUTH_TRUE) { found = LDG_TRUTH_TRUE; break; } }
        if (found == LDG_TRUTH_TRUE) { continue; }

        cross_hd = 0;
        cross_tail = 0;
        cross_live = 0;
        new_cunt = 0;
        for (tri_idx = 0; tri_idx < tri_cunt; tri_idx++)
        {
            if (tri[tri_idx].alive != LDG_TRUTH_TRUE) { continue; }

            tri_corner[0] = tri[tri_idx].va;
            tri_corner[1] = tri[tri_idx].vb;
            tri_corner[2] = tri[tri_idx].vc;
            for (corner = 0; corner < 3; corner++)
            {
                edge_from = tri_corner[corner];
                edge_to = tri_corner[(corner + 1) % 3];
                if (edge_from >= edge_to) { continue; }

                if (edge_from == seg_a || edge_from == seg_b || edge_to == seg_a || edge_to == seg_b) { continue; }

                if (tri_segments_cross_is(points[seg_a * 2], points[seg_a * 2 + 1], points[seg_b * 2], points[seg_b * 2 + 1], points[edge_from * 2], points[edge_from * 2 + 1], points[edge_to * 2], points[edge_to * 2 + 1]) != LDG_TRUTH_TRUE) { continue; }

                edge_from_list[cross_tail] = edge_from;
                edge_to_list[cross_tail] = edge_to;
                cross_tail = (cross_tail + 1) % cross_cap;
                cross_live++;
            }
        }

        defer_run = 0;
        while (cross_live > 0)
        {
            if (LDG_UNLIKELY(defer_run > cross_live)) { return LDG_ERR_FUNC_ARG_INVALID; }

            edge_from = edge_from_list[cross_hd];
            edge_to = edge_to_list[cross_hd];
            cross_hd = (cross_hd + 1) % cross_cap;
            cross_live--;

            lhs_tri = tri_cap;
            rhs_tri = tri_cap;
            for (tri_idx = 0; tri_idx < tri_cunt; tri_idx++)
            {
                if (tri[tri_idx].alive != LDG_TRUTH_TRUE || tri_edge_has(&tri[tri_idx], edge_from, edge_to) != LDG_TRUTH_TRUE) { continue; }

                if (lhs_tri == tri_cap) { lhs_tri = tri_idx; }
                else { rhs_tri = tri_idx; break; }
            }
            if (lhs_tri == tri_cap || rhs_tri == tri_cap) { continue; }

            apex_lhs = tri_apex_get(&tri[lhs_tri], edge_from, edge_to);
            apex_rhs = tri_apex_get(&tri[rhs_tri], edge_from, edge_to);

            if (tri_segments_cross_is(points[edge_from * 2], points[edge_from * 2 + 1], points[edge_to * 2], points[edge_to * 2 + 1], points[apex_lhs * 2], points[apex_lhs * 2 + 1], points[apex_rhs * 2], points[apex_rhs * 2 + 1]) != LDG_TRUTH_TRUE)
            {
                edge_from_list[cross_tail] = edge_from;
                edge_to_list[cross_tail] = edge_to;
                cross_tail = (cross_tail + 1) % cross_cap;
                cross_live++;
                defer_run++;
                continue;
            }

            tri[lhs_tri].alive = LDG_TRUTH_FALSE;
            tri[rhs_tri].alive = LDG_TRUTH_FALSE;
            if (LDG_UNLIKELY(tri_add(tri, &tri_cunt, tri_cap, points, apex_lhs, apex_rhs, edge_from) != LDG_TRUTH_TRUE)) { return LDG_ERR_ALLOC_NULL; }

            if (LDG_UNLIKELY(tri_add(tri, &tri_cunt, tri_cap, points, apex_lhs, apex_rhs, edge_to) != LDG_TRUTH_TRUE)) { return LDG_ERR_ALLOC_NULL; }

            defer_run = 0;

            if (tri_segments_cross_is(points[seg_a * 2], points[seg_a * 2 + 1], points[seg_b * 2], points[seg_b * 2 + 1], points[apex_lhs * 2], points[apex_lhs * 2 + 1], points[apex_rhs * 2], points[apex_rhs * 2 + 1]) == LDG_TRUTH_TRUE)
            {
                edge_from_list[cross_tail] = apex_lhs;
                edge_to_list[cross_tail] = apex_rhs;
                cross_tail = (cross_tail + 1) % cross_cap;
                cross_live++;
            }
            else
            {
                if (LDG_UNLIKELY(new_cunt >= cross_cap)) { return LDG_ERR_BOUNDS; }

                new_from_list[new_cunt] = apex_lhs;
                new_to_list[new_cunt] = apex_rhs;
                new_cunt++;
            }
        }

        guard = 0;
        for (;;)
        {
            shared = LDG_TRUTH_FALSE;
            for (edge_idx = 0; edge_idx < new_cunt; edge_idx++)
            {
                edge_from = new_from_list[edge_idx];
                edge_to = new_to_list[edge_idx];
                if ((edge_from == seg_a && edge_to == seg_b) || (edge_from == seg_b && edge_to == seg_a)) { continue; }

                lhs_tri = tri_cap;
                rhs_tri = tri_cap;
                for (tri_idx = 0; tri_idx < tri_cunt; tri_idx++)
                {
                    if (tri[tri_idx].alive != LDG_TRUTH_TRUE || tri_edge_has(&tri[tri_idx], edge_from, edge_to) != LDG_TRUTH_TRUE) { continue; }

                    if (lhs_tri == tri_cap) { lhs_tri = tri_idx; }
                    else { rhs_tri = tri_idx; break; }
                }
                if (lhs_tri == tri_cap || rhs_tri == tri_cap) { continue; }

                apex_lhs = tri_apex_get(&tri[lhs_tri], edge_from, edge_to);
                apex_rhs = tri_apex_get(&tri[rhs_tri], edge_from, edge_to);

                if (tri_incircle_is(points[tri[lhs_tri].va * 2], points[tri[lhs_tri].va * 2 + 1], points[tri[lhs_tri].vb * 2], points[tri[lhs_tri].vb * 2 + 1], points[tri[lhs_tri].vc * 2], points[tri[lhs_tri].vc * 2 + 1], points[apex_rhs * 2], points[apex_rhs * 2 + 1]) != LDG_TRUTH_TRUE) { continue; }

                if (tri_segments_cross_is(points[edge_from * 2], points[edge_from * 2 + 1], points[edge_to * 2], points[edge_to * 2 + 1], points[apex_lhs * 2], points[apex_lhs * 2 + 1], points[apex_rhs * 2], points[apex_rhs * 2 + 1]) != LDG_TRUTH_TRUE) { continue; }

                tri[lhs_tri].alive = LDG_TRUTH_FALSE;
                tri[rhs_tri].alive = LDG_TRUTH_FALSE;
                if (LDG_UNLIKELY(tri_add(tri, &tri_cunt, tri_cap, points, apex_lhs, apex_rhs, edge_from) != LDG_TRUTH_TRUE)) { return LDG_ERR_ALLOC_NULL; }

                if (LDG_UNLIKELY(tri_add(tri, &tri_cunt, tri_cap, points, apex_lhs, apex_rhs, edge_to) != LDG_TRUTH_TRUE)) { return LDG_ERR_ALLOC_NULL; }

                new_from_list[edge_idx] = apex_lhs;
                new_to_list[edge_idx] = apex_rhs;
                shared = LDG_TRUTH_TRUE;
            }
            if (shared != LDG_TRUTH_TRUE) { break; }

            guard++;
            if (LDG_UNLIKELY(guard > 4 * tri_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }
        }
    }

    for (tri_idx = 0; tri_idx < tri_cunt; tri_idx++)
    {
        if (tri[tri_idx].alive != LDG_TRUTH_TRUE) { continue; }

        if (tri[tri_idx].va >= vert_cunt || tri[tri_idx].vb >= vert_cunt || tri[tri_idx].vc >= vert_cunt) { continue; }

        if (LDG_UNLIKELY(out_cunt + 3 > out_idx_cap)) { return LDG_ERR_BOUNDS; }

        out_idx[out_cunt] = tri[tri_idx].va;
        out_idx[out_cunt + 1] = tri[tri_idx].vb;
        out_idx[out_cunt + 2] = tri[tri_idx].vc;
        out_cunt += 3;
    }

    *out_idx_cunt = out_cunt;
    return LDG_ERR_AOK;
}
