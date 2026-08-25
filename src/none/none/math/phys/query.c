
#include <dangling/core/err.h>
#include <dangling/math/math.h>
#include <dangling/core/macros.h>
#include <dangling/math/phys.h>

#include "query.h"

#define LDG_MATH_PHYS_SWEEP_GOLDEN_ITER 96
#define LDG_MATH_PHYS_SWEEP_BISECT_ITER 72
#define LDG_MATH_PHYS_SWEEP_GOLDEN_RATIO 0.6180339887498949
#define LDG_MATH_PHYS_SWEEP_EPS 1.0e-12

static double ldg_math_phys_sweep_dot(const double *lhs, const double *rhs)
{
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}

static double ldg_math_phys_sweep_seg_point_dist(const double *seg_a, const double *seg_b, const double *point, double *closest)
{
    double edge[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double rel[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double diff[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double edge_len2 = 0.0;
    double param = 0.0;
    double dist2 = 0.0;
    uint32_t axis = 0;

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        edge[axis] = seg_b[axis] - seg_a[axis];
        rel[axis] = point[axis] - seg_a[axis];
        edge_len2 += edge[axis] * edge[axis];
        param += rel[axis] * edge[axis];
    }

    if (edge_len2 > 0.0)
    {
        param = param / edge_len2;
        if (param < 0.0) { param = 0.0; }

        if (param > 1.0) { param = 1.0; }
    }
    else { param = 0.0; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        closest[axis] = seg_a[axis] + param * edge[axis];
        diff[axis] = point[axis] - closest[axis];
        dist2 += diff[axis] * diff[axis];
    }
    return ldg_math_sqrt_get(dist2);
}

static double ldg_math_phys_sweep_sphere_seg_gap(const ldg_math_phys_sweep_desc_t *sweep, const double *center, double thr, double frac, double *closest)
{
    double seg_a[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double seg_b[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    uint32_t axis = 0;

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        seg_a[axis] = sweep->origin[axis] + frac * sweep->disp[axis];
        seg_b[axis] = sweep->point_b[axis] + frac * sweep->disp[axis];
    }
    return ldg_math_phys_sweep_seg_point_dist(seg_a, seg_b, center, closest) - thr;
}

static void ldg_math_phys_sweep_sphere_normal_set(const double *closest, const double *center, double radius, double *out_normal, double *out_point)
{
    double dir[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double len = 0.0;
    uint32_t axis = 0;

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        dir[axis] = closest[axis] - center[axis];
        len += dir[axis] * dir[axis];
    }
    len = ldg_math_sqrt_get(len);

    if (len > LDG_MATH_PHYS_SWEEP_EPS) { for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { dir[axis] = dir[axis] / len; } }
    else
    {
        dir[0] = 0.0;
        dir[1] = 0.0;
        dir[2] = 1.0;
    }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        out_normal[axis] = dir[axis];
        out_point[axis] = center[axis] + dir[axis] * radius;
    }
}

static uint32_t ldg_math_phys_sweep_plane_solve(const ldg_math_phys_sweep_desc_t *sweep, const ldg_math_phys_collider_desc_t *tgt, uint8_t is_segment, double rm, ldg_math_phys_sweep_hit_t *out, uint8_t *hit)
{
    double unit[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double start[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double contact[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double norm_len = 0.0;
    double dot_start = 0.0;
    double dot_b = 0.0;
    double rate = 0.0;
    double gap0 = 0.0;
    double toi = 0.0;
    double surface = 0.0;
    uint32_t axis = 0;

    norm_len = ldg_math_sqrt_get(ldg_math_phys_sweep_dot(tgt->normal, tgt->normal));
    if (LDG_UNLIKELY(norm_len <= LDG_MATH_PHYS_SWEEP_EPS)) { *hit = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { unit[axis] = tgt->normal[axis] / norm_len; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { start[axis] = sweep->origin[axis]; }
    if (is_segment == LDG_TRUTH_TRUE)
    {
        dot_start = ldg_math_phys_sweep_dot(sweep->origin, unit);
        dot_b = ldg_math_phys_sweep_dot(sweep->point_b, unit);
        if (dot_b < dot_start) { for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { start[axis] = sweep->point_b[axis]; } }
    }

    gap0 = ldg_math_phys_sweep_dot(start, unit) - tgt->offset - rm;
    rate = ldg_math_phys_sweep_dot(sweep->disp, unit);

    if (gap0 <= 0.0) { toi = 0.0; }
    else if (rate < 0.0)
    {
        toi = (0.0 - gap0) / rate;
        if (toi > 1.0) { *hit = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }
    }
    else { *hit = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { contact[axis] = start[axis] + toi * sweep->disp[axis]; }
    surface = ldg_math_phys_sweep_dot(contact, unit) - tgt->offset;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        out->normal[axis] = unit[axis];
        out->point[axis] = contact[axis] - unit[axis] * surface;
    }
    out->toi = toi;
    *hit = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_sweep_sphere_point_solve(const ldg_math_phys_sweep_desc_t *sweep, const ldg_math_phys_collider_desc_t *tgt, double rm, ldg_math_phys_sweep_hit_t *out, uint8_t *hit)
{
    double rel[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double contact[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double thr = 0.0;
    double coeff_a = 0.0;
    double coeff_b = 0.0;
    double coeff_c = 0.0;
    double disc = 0.0;
    double rel2 = 0.0;
    double toi = 0.0;
    uint32_t axis = 0;

    thr = tgt->radius + rm;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        rel[axis] = sweep->origin[axis] - tgt->center[axis];
        rel2 += rel[axis] * rel[axis];
    }

    if (rel2 <= thr * thr) { toi = 0.0; }
    else
    {
        coeff_a = ldg_math_phys_sweep_dot(sweep->disp, sweep->disp);
        if (LDG_UNLIKELY(coeff_a <= LDG_MATH_PHYS_SWEEP_EPS)) { *hit = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

        coeff_b = 2.0 * ldg_math_phys_sweep_dot(rel, sweep->disp);
        coeff_c = rel2 - thr * thr;
        disc = coeff_b * coeff_b - 4.0 * coeff_a * coeff_c;
        if (disc < 0.0) { *hit = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

        toi = (0.0 - coeff_b - ldg_math_sqrt_get(disc)) / (2.0 * coeff_a);
        if (toi < 0.0 || toi > 1.0) { *hit = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }
    }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++) { contact[axis] = sweep->origin[axis] + toi * sweep->disp[axis]; }
    ldg_math_phys_sweep_sphere_normal_set(contact, tgt->center, tgt->radius, out->normal, out->point);
    out->toi = toi;
    *hit = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

static uint32_t ldg_math_phys_sweep_sphere_seg_solve(const ldg_math_phys_sweep_desc_t *sweep, const ldg_math_phys_collider_desc_t *tgt, double rm, ldg_math_phys_sweep_hit_t *out, uint8_t *hit)
{
    double closest[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double thr = 0.0;
    double gap0 = 0.0;
    double lo = 0.0;
    double hi = 0.0;
    double probe_lo = 0.0;
    double probe_hi = 0.0;
    double gap_lo = 0.0;
    double gap_hi = 0.0;
    double mid = 0.0;
    double gap_mid = 0.0;
    double toi = 0.0;
    uint32_t iter = 0;

    thr = tgt->radius + rm;
    gap0 = ldg_math_phys_sweep_sphere_seg_gap(sweep, tgt->center, thr, 0.0, closest);
    if (gap0 <= 0.0)
    {
        ldg_math_phys_sweep_sphere_normal_set(closest, tgt->center, tgt->radius, out->normal, out->point);
        out->toi = 0.0;
        *hit = LDG_TRUTH_TRUE;
        return LDG_ERR_AOK;
    }

    lo = 0.0;
    hi = 1.0;
    probe_lo = hi - LDG_MATH_PHYS_SWEEP_GOLDEN_RATIO * (hi - lo);
    probe_hi = lo + LDG_MATH_PHYS_SWEEP_GOLDEN_RATIO * (hi - lo);
    gap_lo = ldg_math_phys_sweep_sphere_seg_gap(sweep, tgt->center, thr, probe_lo, closest);
    gap_hi = ldg_math_phys_sweep_sphere_seg_gap(sweep, tgt->center, thr, probe_hi, closest);
    for (iter = 0; iter < LDG_MATH_PHYS_SWEEP_GOLDEN_ITER; iter++)
    {
        if (gap_lo < gap_hi)
        {
            hi = probe_hi;
            probe_hi = probe_lo;
            gap_hi = gap_lo;
            probe_lo = hi - LDG_MATH_PHYS_SWEEP_GOLDEN_RATIO * (hi - lo);
            gap_lo = ldg_math_phys_sweep_sphere_seg_gap(sweep, tgt->center, thr, probe_lo, closest);
        }
        else
        {
            lo = probe_lo;
            probe_lo = probe_hi;
            gap_lo = gap_hi;
            probe_hi = lo + LDG_MATH_PHYS_SWEEP_GOLDEN_RATIO * (hi - lo);
            gap_hi = ldg_math_phys_sweep_sphere_seg_gap(sweep, tgt->center, thr, probe_hi, closest);
        }
    }

    mid = 0.5 * (lo + hi);
    gap_mid = ldg_math_phys_sweep_sphere_seg_gap(sweep, tgt->center, thr, mid, closest);
    if (gap_mid > 0.0) { *hit = LDG_TRUTH_FALSE; return LDG_ERR_AOK; }

    lo = 0.0;
    hi = mid;
    for (iter = 0; iter < LDG_MATH_PHYS_SWEEP_BISECT_ITER; iter++)
    {
        toi = 0.5 * (lo + hi);
        gap_mid = ldg_math_phys_sweep_sphere_seg_gap(sweep, tgt->center, thr, toi, closest);
        if (gap_mid > 0.0) { lo = toi; }
        else { hi = toi; }
    }

    toi = hi;
    gap_mid = ldg_math_phys_sweep_sphere_seg_gap(sweep, tgt->center, thr, toi, closest);
    ldg_math_phys_sweep_sphere_normal_set(closest, tgt->center, tgt->radius, out->normal, out->point);
    out->toi = toi;
    *hit = LDG_TRUTH_TRUE;
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_sweep_solve(const ldg_math_phys_sweep_desc_t *sweep, const ldg_math_phys_collider_desc_t *tgt, ldg_math_phys_sweep_hit_t *out, uint8_t *hit)
{
    double rm = 0.0;
    uint8_t is_segment = UINT8_MAX;
    uint32_t axis = 0;

    if (LDG_UNLIKELY(!sweep || !tgt || !out || !hit)) { return LDG_ERR_FUNC_ARG_NULL; }

    *hit = LDG_TRUTH_FALSE;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        out->normal[axis] = 0.0;
        out->point[axis] = 0.0;
    }
    out->toi = 0.0;
    out->collider = UINT64_MAX;

    if (LDG_UNLIKELY(sweep->shape >= LDG_MATH_PHYS_SWEEP_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (sweep->shape != LDG_MATH_PHYS_SWEEP_RAY)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(sweep->radius) || sweep->radius < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        rm = sweep->radius;
    }

    is_segment = (sweep->shape == LDG_MATH_PHYS_SWEEP_CAPSULE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    if (tgt->shape == LDG_MATH_PHYS_COLLIDER_PLANE) { return ldg_math_phys_sweep_plane_solve(sweep, tgt, is_segment, rm, out, hit); }

    if (tgt->shape == LDG_MATH_PHYS_COLLIDER_SPHERE)
    {
        if (is_segment == LDG_TRUTH_TRUE) { return ldg_math_phys_sweep_sphere_seg_solve(sweep, tgt, rm, out, hit); }

        return ldg_math_phys_sweep_sphere_point_solve(sweep, tgt, rm, out, hit);
    }

    return LDG_ERR_UNSUPPORTED;
}

static uint32_t ldg_math_phys_sweep_desc_ck(const ldg_math_phys_sweep_desc_t *sweep)
{
    uint32_t axis = 0;

    if (LDG_UNLIKELY(sweep->shape >= LDG_MATH_PHYS_SWEEP_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (sweep->shape != LDG_MATH_PHYS_SWEEP_RAY) { if (LDG_UNLIKELY(!ldg_math_finite_is(sweep->radius) || sweep->radius < 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; } }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        if (LDG_UNLIKELY(!ldg_math_finite_is(sweep->origin[axis]) || !ldg_math_finite_is(sweep->disp[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(sweep->shape == LDG_MATH_PHYS_SWEEP_CAPSULE && !ldg_math_finite_is(sweep->point_b[axis]))) { return LDG_ERR_FUNC_ARG_INVALID; }
    }
    return LDG_ERR_AOK;
}

LDG_EXPORT uint32_t ldg_math_phys_sweep_cast(const ldg_math_phys_ctx_t *ctx, const ldg_math_phys_sweep_desc_t *sweep, ldg_math_phys_sweep_hit_t *out, uint64_t out_cap, uint64_t *out_cunt)
{
    ldg_math_phys_sweep_hit_t cand = LDG_STRUCT_ZERO_INIT;
    uint64_t cunt = 0;
    uint64_t pos = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;
    uint8_t hit = UINT8_MAX;

    if (LDG_UNLIKELY(!ctx || !sweep || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    if (LDG_UNLIKELY(out_cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_MATH_PHYS_NOT_INIT; }

    ret = ldg_math_phys_sweep_desc_ck(sweep);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < ctx->conf.collider_cap; idx++)
    {
        if (!ctx->collider[idx].used) { continue; }

        ret = ldg_math_phys_sweep_solve(sweep, &ctx->collider[idx].desc, &cand, &hit);
        if (ret == LDG_ERR_UNSUPPORTED) { continue; }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (hit != LDG_TRUTH_TRUE) { continue; }

        cand.collider = ((uint64_t)ctx->collider[idx].generation << 32) | idx;

        if (cunt < out_cap)
        {
            pos = cunt;
            while (pos > 0 && out[pos - 1].toi > cand.toi) { out[pos] = out[pos - 1]; pos--; }
            out[pos] = cand;
            cunt++;
        }
        else if (cand.toi < out[cunt - 1].toi)
        {
            pos = cunt - 1;
            while (pos > 0 && out[pos - 1].toi > cand.toi) { out[pos] = out[pos - 1]; pos--; }
            out[pos] = cand;
        }
    }

    *out_cunt = cunt;
    return LDG_ERR_AOK;
}
