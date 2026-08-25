#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/err.h>
#include <dangling/mem/alloc.h>
#include <dangling/math/predicate.h>
#include <dangling/math/tetrahedralize.h>

#define TET_SUPER_SCALE 1000.0
#define TET_CAP_MIN 16u
#define TET_TINY 1.0e-30

typedef struct tet_cell
{
    uint32_t node[4];
    uint8_t alive;
    uint8_t pudding[3];
} tet_cell_t;

static uint8_t tet_super_touch_is(const tet_cell_t *cell, uint32_t super_base)
{
    uint32_t idx = 0x0;

    if (LDG_UNLIKELY(!cell)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < 4; idx++) { if (cell->node[idx] >= super_base && cell->node[idx] < super_base + 4) { return LDG_TRUTH_TRUE; } }

    return LDG_TRUTH_FALSE;
}

static uint8_t tet_face_in_cell_is(uint32_t face_a, uint32_t face_b, uint32_t face_c, const uint32_t *cell_node)
{
    uint8_t has_a = UINT8_MAX;
    uint8_t has_b = UINT8_MAX;
    uint8_t has_c = UINT8_MAX;
    uint32_t idx = 0x0;

    if (LDG_UNLIKELY(!cell_node)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (idx = 0; idx < 4; idx++)
    {
        if (cell_node[idx] == face_a) { has_a = LDG_TRUTH_TRUE; }

        if (cell_node[idx] == face_b) { has_b = LDG_TRUTH_TRUE; }

        if (cell_node[idx] == face_c) { has_c = LDG_TRUTH_TRUE; }
    }

    if (has_a == LDG_TRUTH_TRUE && has_b == LDG_TRUTH_TRUE && has_c == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t tet_make(const double *combined, uint32_t idx_a, uint32_t idx_b, uint32_t idx_c, uint32_t idx_d, ldg_mem_pool_t *predicate_pool, tet_cell_t *cell)
{
    uint8_t sign = 0x0;
    uint32_t err = 0x0;
    err = ldg_math_3d_orient_perturb(combined, idx_a, idx_b, idx_c, idx_d, predicate_pool, &sign);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (sign == LDG_MATH_SIGN_POS)
    {
        cell->node[0] = idx_a;
        cell->node[1] = idx_b;
    }
    else
    {
        cell->node[0] = idx_b;
        cell->node[1] = idx_a;
    }

    cell->node[2] = idx_c;
    cell->node[3] = idx_d;
    cell->alive = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

static uint32_t tet_insert(const double *combined, tet_cell_t *tet, uint32_t *tet_cunt, uint32_t *free_cunt, uint32_t *cavity, uint32_t *freelist, uint32_t tet_cap, uint32_t point, ldg_mem_pool_t *predicate_pool)
{
    uint32_t face[3] = LDG_ARR_ZERO_INIT;
    uint32_t cavity_cunt = 0x0;
    uint32_t slot = 0x0;
    uint32_t entry_lhs = 0x0;
    uint32_t entry_rhs = 0x0;
    uint32_t face_excl = 0x0;
    uint32_t corner = 0x0;
    uint32_t fill = 0x0;
    uint32_t new_slot = 0x0;
    uint8_t sign = 0x0;
    uint8_t shared = UINT8_MAX;
    uint32_t err = 0x0;
    for (slot = 0u; slot < *tet_cunt; slot++)
    {
        if (tet[slot].alive != LDG_TRUTH_TRUE) { continue; }

        err = ldg_math_3d_insphere_perturb(combined, tet[slot].node[0], tet[slot].node[1], tet[slot].node[2], tet[slot].node[3], point, predicate_pool, &sign);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

        if (sign == LDG_MATH_SIGN_POS)
        {
            cavity[cavity_cunt] = slot;
            cavity_cunt++;
        }
    }
    for (entry_lhs = 0u; entry_lhs < cavity_cunt; entry_lhs++)
    {
        for (face_excl = 0u; face_excl < 4u; face_excl++)
        {
            fill = 0u;
            for (corner = 0u; corner < 4u; corner++) { if (corner != face_excl && fill < 3u)
                {
                    face[fill] = tet[cavity[entry_lhs]].node[corner];
                    fill++;
                }
            }
            shared = LDG_TRUTH_FALSE;
            for (entry_rhs = 0u; entry_rhs < cavity_cunt; entry_rhs++)
            {
                if (entry_rhs == entry_lhs) { continue; }

                if (tet_face_in_cell_is(face[0], face[1], face[2], tet[cavity[entry_rhs]].node) == LDG_TRUTH_TRUE)
                {
                    shared = LDG_TRUTH_TRUE;
                    break;
                }
            }
            if (shared == LDG_TRUTH_TRUE) { continue; }

            if (*free_cunt > 0u)
            {
                (*free_cunt)--;
                new_slot = freelist[*free_cunt];
            }
            else
            {
                if (LDG_UNLIKELY(*tet_cunt >= tet_cap)) { return LDG_ERR_MEM_POOL_FULL; }

                new_slot = *tet_cunt;
                (*tet_cunt)++;
            }

            err = tet_make(combined, face[0], face[1], face[2], point, predicate_pool, &tet[new_slot]);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
        }
    }
    for (entry_lhs = 0u; entry_lhs < cavity_cunt; entry_lhs++)
    {
        tet[cavity[entry_lhs]].alive = LDG_TRUTH_FALSE;
        freelist[*free_cunt] = cavity[entry_lhs];
        (*free_cunt)++;
    }
    return LDG_ERR_AOK;
}

static double tet_determinant3_get(const double *mat)
{
    return mat[0] * (mat[4] * mat[8] - mat[5] * mat[7]) - mat[1] * (mat[3] * mat[8] - mat[5] * mat[6]) + mat[2] * (mat[3] * mat[7] - mat[4] * mat[6]);
}

static uint8_t tet_circumcenter_get(const double *point_a, const double *point_b, const double *point_c, const double *point_d, double *out_center)
{
    double mat[9] = LDG_ARR_ZERO_INIT;
    double tmp[9] = LDG_ARR_ZERO_INIT;
    double rhs[3] = LDG_ARR_ZERO_INIT;
    double determinant = 0x0;
    double determinant_absolute = 0x0;
    uint32_t axis = 0x0;
    uint32_t col = 0x0;
    for (axis = 0u; axis < 3u; axis++)
    {
        mat[0u * 3u + axis] = point_b[axis] - point_a[axis];
        mat[1u * 3u + axis] = point_c[axis] - point_a[axis];
        mat[2u * 3u + axis] = point_d[axis] - point_a[axis];
    }
    rhs[0] = 0.5 * ((point_b[0] * point_b[0] + point_b[1] * point_b[1] + point_b[2] * point_b[2]) - (point_a[0] * point_a[0] + point_a[1] * point_a[1] + point_a[2] * point_a[2]));
    rhs[1] = 0.5 * ((point_c[0] * point_c[0] + point_c[1] * point_c[1] + point_c[2] * point_c[2]) - (point_a[0] * point_a[0] + point_a[1] * point_a[1] + point_a[2] * point_a[2]));
    rhs[2] = 0.5 * ((point_d[0] * point_d[0] + point_d[1] * point_d[1] + point_d[2] * point_d[2]) - (point_a[0] * point_a[0] + point_a[1] * point_a[1] + point_a[2] * point_a[2]));
    determinant = tet_determinant3_get(mat);
    determinant_absolute = determinant;
    if (determinant_absolute < 0.0) { determinant_absolute = -determinant_absolute; }

    if (determinant_absolute < TET_TINY) { return LDG_TRUTH_FALSE; }

    for (col = 0u; col < 3u; col++)
    {
        for (axis = 0u; axis < 9u; axis++) { tmp[axis] = mat[axis]; }
        tmp[0u * 3u + col] = rhs[0];
        tmp[1u * 3u + col] = rhs[1];
        tmp[2u * 3u + col] = rhs[2];
        out_center[col] = tet_determinant3_get(tmp) / determinant;
    }
    return LDG_TRUTH_TRUE;
}

static double tet_dist_square_get(const double *lhs, const double *rhs)
{
    double dx = 0x0;
    double dy = 0x0;
    double dz = 0x0;
    dx = lhs[0] - rhs[0];
    dy = lhs[1] - rhs[1];
    dz = lhs[2] - rhs[2];
    return dx * dx + dy * dy + dz * dz;
}

static uint8_t tet_quality_is(const double *combined, const tet_cell_t *cell, double bound, double *out_center)
{
    const double *point_a = 0x0;
    const double *point_b = 0x0;
    const double *point_c = 0x0;
    const double *point_d = 0x0;
    double radius_square = 0x0;
    double short_square = 0x0;
    double edge_square = 0x0;
    uint8_t ok = UINT8_MAX;

    if (LDG_UNLIKELY(!combined || !cell || !out_center)) { return LDG_ERR_FUNC_ARG_NULL; }

    point_a = combined + 3 * cell->node[0];
    point_b = combined + 3 * cell->node[1];
    point_c = combined + 3 * cell->node[2];
    point_d = combined + 3 * cell->node[3];
    ok = tet_circumcenter_get(point_a, point_b, point_c, point_d, out_center);
    if (ok != LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }

    radius_square = tet_dist_square_get(out_center, point_a);
    short_square = tet_dist_square_get(point_a, point_b);
    edge_square = tet_dist_square_get(point_a, point_c);
    if (edge_square < short_square) { short_square = edge_square; }

    edge_square = tet_dist_square_get(point_a, point_d);
    if (edge_square < short_square) { short_square = edge_square; }

    edge_square = tet_dist_square_get(point_b, point_c);
    if (edge_square < short_square) { short_square = edge_square; }

    edge_square = tet_dist_square_get(point_b, point_d);
    if (edge_square < short_square) { short_square = edge_square; }

    edge_square = tet_dist_square_get(point_c, point_d);
    if (edge_square < short_square) { short_square = edge_square; }

    if (radius_square > bound * bound * short_square) { return 1u; }

    return 0u;
}

static uint8_t tet_point_inside_is(const double *combined, const tet_cell_t *cell, const double *point, ldg_mem_pool_t *predicate_pool)
{
    const double *point_a = 0x0;
    const double *point_b = 0x0;
    const double *point_c = 0x0;
    const double *point_d = 0x0;
    uint8_t face_0 = 0x0;
    uint8_t face_1 = 0x0;
    uint8_t face_2 = 0x0;
    uint8_t face_3 = 0x0;
    uint32_t err = 0x0;

    if (LDG_UNLIKELY(!combined || !cell || !point)) { return LDG_ERR_FUNC_ARG_NULL; }

    point_a = combined + 3 * cell->node[0];
    point_b = combined + 3 * cell->node[1];
    point_c = combined + 3 * cell->node[2];
    point_d = combined + 3 * cell->node[3];
    err = ldg_math_3d_orient(point, point_b, point_c, point_d, predicate_pool, &face_0);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return (uint8_t)err; }

    err = ldg_math_3d_orient(point_a, point, point_c, point_d, predicate_pool, &face_1);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return (uint8_t)err; }

    err = ldg_math_3d_orient(point_a, point_b, point, point_d, predicate_pool, &face_2);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return (uint8_t)err; }

    err = ldg_math_3d_orient(point_a, point_b, point_c, point, predicate_pool, &face_3);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return (uint8_t)err; }

    if (face_0 != LDG_MATH_SIGN_NEG && face_1 != LDG_MATH_SIGN_NEG && face_2 != LDG_MATH_SIGN_NEG && face_3 != LDG_MATH_SIGN_NEG) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

static uint32_t tet_build(const double *combined, uint32_t vert_cunt, uint32_t super_base, tet_cell_t *tet, uint32_t *tet_cunt, uint32_t *free_cunt, uint32_t *cavity, uint32_t *freelist, uint32_t tet_cap, ldg_mem_pool_t *predicate_pool)
{
    uint32_t point = 0x0;
    uint32_t err = 0x0;
    *tet_cunt = 0u;
    *free_cunt = 0u;
    err = tet_make(combined, super_base + 0u, super_base + 1u, super_base + 2u, super_base + 3u, predicate_pool, &tet[0]);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    *tet_cunt = 1u;
    for (point = 0u; point < vert_cunt; point++)
    {
        err = tet_insert(combined, tet, tet_cunt, free_cunt, cavity, freelist, tet_cap, point, predicate_pool);
        if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }
    }
    return LDG_ERR_AOK;
}

static uint32_t tet_super_build(const double *vert, uint32_t vert_cunt, double *combined)
{
    double bound_min[3] = LDG_ARR_ZERO_INIT;
    double bound_max[3] = LDG_ARR_ZERO_INIT;
    double center[3] = LDG_ARR_ZERO_INIT;
    double extent = 0x0;
    double span = 0x0;
    double super_scale = 0x0;
    uint32_t super_base = 0x0;
    uint32_t point = 0x0;
    uint32_t axis = 0x0;
    uint32_t idx = 0x0;
    super_base = vert_cunt;
    for (axis = 0u; axis < 3u; axis++)
    {
        bound_min[axis] = vert[axis];
        bound_max[axis] = vert[axis];
    }
    for (point = 1u; point < vert_cunt; point++) { for (axis = 0u; axis < 3u; axis++)
        {
            if (vert[point * 3u + axis] < bound_min[axis]) { bound_min[axis] = vert[point * 3u + axis]; }

            if (vert[point * 3u + axis] > bound_max[axis]) { bound_max[axis] = vert[point * 3u + axis]; }
        }
    }
    extent = 0.0;
    for (axis = 0u; axis < 3u; axis++)
    {
        center[axis] = (bound_min[axis] + bound_max[axis]) * 0.5;
        span = bound_max[axis] - bound_min[axis];
        if (span > extent) { extent = span; }
    }
    if (extent <= 0.0) { extent = 1.0; }

    super_scale = extent * TET_SUPER_SCALE;
    for (idx = 0u; idx < vert_cunt * 3u; idx++) { combined[idx] = vert[idx]; }
    combined[(super_base + 0u) * 3u + 0u] = center[0] + super_scale;
    combined[(super_base + 0u) * 3u + 1u] = center[1] + super_scale;
    combined[(super_base + 0u) * 3u + 2u] = center[2] + super_scale;
    combined[(super_base + 1u) * 3u + 0u] = center[0] + super_scale;
    combined[(super_base + 1u) * 3u + 1u] = center[1] - super_scale;
    combined[(super_base + 1u) * 3u + 2u] = center[2] - super_scale;
    combined[(super_base + 2u) * 3u + 0u] = center[0] - super_scale;
    combined[(super_base + 2u) * 3u + 1u] = center[1] + super_scale;
    combined[(super_base + 2u) * 3u + 2u] = center[2] - super_scale;
    combined[(super_base + 3u) * 3u + 0u] = center[0] - super_scale;
    combined[(super_base + 3u) * 3u + 1u] = center[1] - super_scale;
    combined[(super_base + 3u) * 3u + 2u] = center[2] + super_scale;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_delaunay_tetrahedralize(const double *vert, uint32_t vert_cunt, ldg_mem_pool_t *work_pool, ldg_mem_pool_t *predicate_pool, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt)
{
    double *combined = 0x0;
    void *block = 0x0;
    tet_cell_t *tet = 0x0;
    uint32_t *cavity = 0x0;
    uint32_t *freelist = 0x0;
    uint64_t verts_bytes = 0x0;
    uint64_t remaining = 0x0;
    uint64_t per_tet = 0x0;
    uint64_t cap64 = 0x0;
    uint32_t tet_cap = 0x0;
    uint32_t tet_cunt = 0x0;
    uint32_t free_cunt = 0x0;
    uint32_t super_base = 0x0;
    uint32_t slot = 0x0;
    uint32_t written = 0x0;
    uint32_t err = 0x0;
    if (LDG_UNLIKELY(vert == 0x0 || work_pool == 0x0 || predicate_pool == 0x0 || out_idx == 0x0 || out_idx_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_idx_cunt = 0u;
    if (LDG_UNLIKELY(vert_cunt < 4u)) { return LDG_ERR_FUNC_ARG_INVALID; }

    super_base = vert_cunt;
    verts_bytes = ((uint64_t)vert_cunt + 4u) * 3u * (uint64_t)sizeof(double);
    err = ldg_mem_pool_rst(work_pool);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_pool_alloc(work_pool, verts_bytes, &block);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(block == 0x0)) { return LDG_ERR_ALLOC_NULL; }

    combined = block;
    err = tet_super_build(vert, vert_cunt, combined);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    remaining = ldg_mem_pool_remaining_get(work_pool);
    per_tet = (uint64_t)sizeof(tet_cell_t) + 4u + 4u;
    if (LDG_UNLIKELY(remaining < 128u)) { return LDG_ERR_MEM_POOL_FULL; }

    cap64 = (remaining - 64u) / per_tet;
    if (LDG_UNLIKELY(cap64 < (uint64_t)TET_CAP_MIN)) { return LDG_ERR_MEM_POOL_FULL; }

    if (cap64 > (uint64_t)UINT32_MAX) { cap64 = (uint64_t)UINT32_MAX; }

    tet_cap = (uint32_t)cap64;
    err = ldg_mem_pool_alloc(work_pool, (uint64_t)tet_cap * (uint64_t)sizeof(tet_cell_t), &block);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    tet = block;
    err = ldg_mem_pool_alloc(work_pool, (uint64_t)tet_cap * 4u, &block);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    cavity = block;
    err = ldg_mem_pool_alloc(work_pool, (uint64_t)tet_cap * 4u, &block);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    freelist = block;
    err = tet_build(combined, vert_cunt, super_base, tet, &tet_cunt, &free_cunt, cavity, freelist, tet_cap, predicate_pool);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    written = 0u;
    for (slot = 0u; slot < tet_cunt; slot++)
    {
        if (tet[slot].alive != LDG_TRUTH_TRUE) { continue; }

        if (tet_super_touch_is(&tet[slot], super_base) == LDG_TRUTH_TRUE) { continue; }

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

uint32_t ldg_math_delaunay_refine(const double *vert, uint32_t vert_cunt, double radius_edge_bound, uint32_t max_steiner, ldg_mem_pool_t *work_pool, ldg_mem_pool_t *predicate_pool, double *out_vert, uint32_t out_vert_cap, uint32_t *out_vert_cunt, uint32_t *out_idx, uint32_t out_idx_cap, uint32_t *out_idx_cunt)
{
    double center[3] = LDG_ARR_ZERO_INIT;
    double *combined = 0x0;
    void *block = 0x0;
    tet_cell_t *tet = 0x0;
    uint32_t *cavity = 0x0;
    uint32_t *freelist = 0x0;
    uint64_t verts_bytes = 0x0;
    uint64_t remaining = 0x0;
    uint64_t per_tet = 0x0;
    uint64_t cap64 = 0x0;
    uint32_t tet_cap = 0x0;
    uint32_t tet_cunt = 0x0;
    uint32_t free_cunt = 0x0;
    uint32_t super_base = 0x0;
    uint32_t steiner_base = 0x0;
    uint32_t steiner_cunt = 0x0;
    uint32_t slot = 0x0;
    uint32_t probe = 0x0;
    uint32_t steiner_idx = 0x0;
    uint32_t written = 0x0;
    uint32_t node = 0x0;
    uint32_t mapped = 0x0;
    uint32_t corner = 0x0;
    uint32_t idx = 0x0;
    uint8_t found = UINT8_MAX;
    uint8_t interior = UINT8_MAX;
    uint32_t err = 0x0;
    if (LDG_UNLIKELY(vert == 0x0 || work_pool == 0x0 || predicate_pool == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_vert == 0x0 || out_vert_cunt == 0x0 || out_idx == 0x0 || out_idx_cunt == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_vert_cunt = 0u;
    *out_idx_cunt = 0u;
    if (LDG_UNLIKELY(vert_cunt < 4u)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(radius_edge_bound <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    super_base = vert_cunt;
    steiner_base = vert_cunt + 4u;
    verts_bytes = ((uint64_t)vert_cunt + 4u + (uint64_t)max_steiner) * 3u * (uint64_t)sizeof(double);
    err = ldg_mem_pool_rst(work_pool);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_mem_pool_alloc(work_pool, verts_bytes, &block);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    if (LDG_UNLIKELY(block == 0x0)) { return LDG_ERR_ALLOC_NULL; }

    combined = block;
    err = tet_super_build(vert, vert_cunt, combined);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    remaining = ldg_mem_pool_remaining_get(work_pool);
    per_tet = (uint64_t)sizeof(tet_cell_t) + 4u + 4u;
    if (LDG_UNLIKELY(remaining < 128u)) { return LDG_ERR_MEM_POOL_FULL; }

    cap64 = (remaining - 64u) / per_tet;
    if (LDG_UNLIKELY(cap64 < (uint64_t)TET_CAP_MIN)) { return LDG_ERR_MEM_POOL_FULL; }

    if (cap64 > (uint64_t)UINT32_MAX) { cap64 = (uint64_t)UINT32_MAX; }

    tet_cap = (uint32_t)cap64;
    err = ldg_mem_pool_alloc(work_pool, (uint64_t)tet_cap * (uint64_t)sizeof(tet_cell_t), &block);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    tet = block;
    err = ldg_mem_pool_alloc(work_pool, (uint64_t)tet_cap * 4u, &block);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    cavity = block;
    err = ldg_mem_pool_alloc(work_pool, (uint64_t)tet_cap * 4u, &block);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    freelist = block;
    err = tet_build(combined, vert_cunt, super_base, tet, &tet_cunt, &free_cunt, cavity, freelist, tet_cap, predicate_pool);
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    steiner_cunt = 0u;
    found = LDG_TRUTH_TRUE;
    while (found == LDG_TRUTH_TRUE && steiner_cunt < max_steiner)
    {
        found = LDG_TRUTH_FALSE;
        for (slot = 0u; slot < tet_cunt; slot++)
        {
            if (tet[slot].alive != LDG_TRUTH_TRUE) { continue; }

            if (tet_super_touch_is(&tet[slot], super_base) == LDG_TRUTH_TRUE) { continue; }

            if (tet_quality_is(combined, &tet[slot], radius_edge_bound, center) == LDG_TRUTH_TRUE) { continue; }

            interior = LDG_TRUTH_FALSE;
            for (probe = 0; probe < tet_cunt; probe++)
            {
                if (tet[probe].alive != LDG_TRUTH_TRUE) { continue; }

                if (tet_super_touch_is(&tet[probe], super_base) == LDG_TRUTH_TRUE) { continue; }

                interior = tet_point_inside_is(combined, &tet[probe], center, predicate_pool);
                if (LDG_UNLIKELY(interior != LDG_TRUTH_TRUE && interior != LDG_TRUTH_FALSE)) { return interior; }

                if (interior == LDG_TRUTH_TRUE) { break; }
            }
            if (interior != LDG_TRUTH_TRUE) { continue; }

            steiner_idx = steiner_base + steiner_cunt;
            combined[steiner_idx * 3u + 0u] = center[0];
            combined[steiner_idx * 3u + 1u] = center[1];
            combined[steiner_idx * 3u + 2u] = center[2];
            err = tet_insert(combined, tet, &tet_cunt, &free_cunt, cavity, freelist, tet_cap, steiner_idx, predicate_pool);
            if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

            steiner_cunt++;
            found = LDG_TRUTH_TRUE;
            break;
        }
    }
    if (LDG_UNLIKELY((vert_cunt + steiner_cunt) * 3u > out_vert_cap)) { return LDG_ERR_FULL; }

    for (idx = 0u; idx < vert_cunt * 3u; idx++) { out_vert[idx] = combined[idx]; }
    for (idx = 0u; idx < steiner_cunt * 3u; idx++) { out_vert[vert_cunt * 3u + idx] = combined[steiner_base * 3u + idx]; }
    *out_vert_cunt = (vert_cunt + steiner_cunt) * 3u;
    written = 0u;
    for (slot = 0u; slot < tet_cunt; slot++)
    {
        if (tet[slot].alive != LDG_TRUTH_TRUE) { continue; }

        if (tet_super_touch_is(&tet[slot], super_base) == LDG_TRUTH_TRUE) { continue; }

        if (LDG_UNLIKELY(written + 4u > out_idx_cap)) { return LDG_ERR_FULL; }

        for (corner = 0u; corner < 4u; corner++)
        {
            node = tet[slot].node[corner];
            if (node < vert_cunt) { mapped = node; }
            else { mapped = vert_cunt + (node - steiner_base); }

            out_idx[written + corner] = mapped;
        }
        written += 4u;
    }
    *out_idx_cunt = written;
    return LDG_ERR_AOK;
}
