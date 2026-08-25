
#include <dangling/math/phys.h>
#include <dangling/math/math.h>
#include <dangling/core/macros.h>
#include <dangling/core/err.h>

#include "atmosphere.h"

static double ldg_math_phys_atmosphere_density_get(double altitude, double scale_height)
{
    if (altitude < 0.0) { altitude = 0.0; }

    return ldg_math_exp_get((0.0 - altitude) / scale_height);
}

static double ldg_math_phys_atmosphere_fog_density_get(const ldg_math_phys_atmosphere_hdr_t *hdr, double altitude)
{
    if (hdr->fog_scale_height <= 0.0) { return 0.0; }

    return ldg_math_phys_atmosphere_density_get(altitude, hdr->fog_scale_height);
}

static double ldg_math_phys_atmosphere_ozone_density_get(const ldg_math_phys_atmosphere_hdr_t *hdr, double altitude)
{
    double offset = 0.0;

    if (hdr->ozone_half_width <= 0.0) { return 0.0; }

    if (altitude < 0.0) { altitude = 0.0; }

    offset = ldg_math_abs_get(altitude - hdr->ozone_center_altitude);
    if (offset >= hdr->ozone_half_width) { return 0.0; }

    return 1.0 - offset / hdr->ozone_half_width;
}

static double ldg_math_phys_atmosphere_ozone_wedge_depth_get(const ldg_math_phys_atmosphere_hdr_t *hdr, double altitude_lo, double altitude_hi)
{
    double lo = altitude_lo;
    double hi = altitude_hi;
    double base_lo = 0.0;
    double base_hi = 0.0;
    double center = hdr->ozone_center_altitude;
    double depth = 0.0;

    base_lo = center - hdr->ozone_half_width;
    base_hi = center + hdr->ozone_half_width;
    if (lo < base_lo) { lo = base_lo; }

    if (hi > base_hi) { hi = base_hi; }

    if (hi <= lo) { return 0.0; }

    if (lo < center && hi > center)
    {
        depth = 0.5 * (ldg_math_phys_atmosphere_ozone_density_get(hdr, lo) + 1.0) * (center - lo);
        return depth + 0.5 * (1.0 + ldg_math_phys_atmosphere_ozone_density_get(hdr, hi)) * (hi - center);
    }

    return 0.5 * (ldg_math_phys_atmosphere_ozone_density_get(hdr, lo) + ldg_math_phys_atmosphere_ozone_density_get(hdr, hi)) * (hi - lo);
}

static double ldg_math_phys_atmosphere_ozone_segment_depth_get(const ldg_math_phys_atmosphere_hdr_t *hdr, double altitude_lo, double altitude_hi, double len)
{
    double delta = 0.0;
    double lo = altitude_lo;
    double hi = altitude_hi;
    double swap = 0.0;

    if (hdr->ozone_half_width <= 0.0) { return 0.0; }

    if (len <= 0.0) { return 0.0; }

    if (lo < 0.0) { lo = 0.0; }

    if (hi < 0.0) { hi = 0.0; }

    delta = hi - lo;
    if (ldg_math_abs_get(delta) < 1.0e-9 * hdr->ozone_half_width) { return len * ldg_math_phys_atmosphere_ozone_density_get(hdr, 0.5 * (lo + hi)); }

    if (lo > hi)
    {
        swap = lo;
        lo = hi;
        hi = swap;
    }

    return (len / (hi - lo)) * ldg_math_phys_atmosphere_ozone_wedge_depth_get(hdr, lo, hi);
}

static double ldg_math_phys_atmosphere_segment_depth_get(double scale_height, double altitude_lo, double altitude_hi, double len)
{
    double delta = 0.0;

    if (scale_height <= 0.0) { return 0.0; }

    if (len <= 0.0) { return 0.0; }

    if (altitude_lo < 0.0) { altitude_lo = 0.0; }

    if (altitude_hi < 0.0) { altitude_hi = 0.0; }

    delta = altitude_hi - altitude_lo;
    if (ldg_math_abs_get(delta) < 1.0e-9 * scale_height) { return len * ldg_math_exp_get((0.0 - 0.5 * (altitude_lo + altitude_hi)) / scale_height); }

    return len * (scale_height / delta) * (ldg_math_exp_get((0.0 - altitude_lo) / scale_height) - ldg_math_exp_get((0.0 - altitude_hi) / scale_height));
}

static double ldg_math_phys_atmosphere_fog_span_get(const ldg_math_phys_atmosphere_hdr_t *hdr, double radius, double mu, double ray_len, double *out_start)
{
    double limit_radius = 0.0;
    double disc = 0.0;
    double root = 0.0;
    double span_lo = 0.0;
    double span_hi = 0.0;

    *out_start = 0.0;
    if (hdr->fog_scale_height <= 0.0) { return 0.0; }

    limit_radius = hdr->planet_radius + hdr->fog_scale_height * LDG_MATH_PHYS_ATMOSPHERE_FOG_LIMIT_SCALE;
    disc = radius * radius * (mu * mu - 1.0) + limit_radius * limit_radius;
    if (disc <= 0.0) { return 0.0; }

    root = ldg_math_sqrt_get(disc);
    span_lo = 0.0 - radius * mu - root;
    span_hi = 0.0 - radius * mu + root;
    if (span_lo < 0.0) { span_lo = 0.0; }

    if (span_hi > ray_len) { span_hi = ray_len; }

    if (span_hi <= span_lo) { return 0.0; }

    *out_start = span_lo;
    return span_hi - span_lo;
}

static double ldg_math_phys_atmosphere_rayleigh_phase_get(double nu)
{
    return (3.0 / (16.0 * LDG_MATH_PI)) * (1.0 + nu * nu);
}

static double ldg_math_phys_atmosphere_mie_phase_get(double anisotropy, double nu)
{
    double square = anisotropy * anisotropy;
    double denom = 1.0 + square - 2.0 * anisotropy * nu;

    if (denom < LDG_MATH_EPSILON) { denom = LDG_MATH_EPSILON; }

    return (3.0 / (8.0 * LDG_MATH_PI)) * ((1.0 - square) * (1.0 + nu * nu)) / ((2.0 + square) * denom * ldg_math_sqrt_get(denom));
}

static double ldg_math_phys_atmosphere_ray_len_get(const ldg_math_phys_atmosphere_hdr_t *hdr, double radius, double mu, uint8_t *ground_hit)
{
    double top = hdr->planet_radius + hdr->thickness;
    double disc_ground = radius * radius * (mu * mu - 1.0) + hdr->planet_radius * hdr->planet_radius;
    double disc_top = radius * radius * (mu * mu - 1.0) + top * top;

    *ground_hit = LDG_TRUTH_FALSE;
    if (mu < 0.0 && disc_ground >= 0.0)
    {
        *ground_hit = LDG_TRUTH_TRUE;
        return 0.0 - radius * mu - ldg_math_sqrt_get(disc_ground);
    }

    if (disc_top < 0.0) { disc_top = 0.0; }

    return 0.0 - radius * mu + ldg_math_sqrt_get(disc_top);
}

static double ldg_math_phys_atmosphere_horizon_get(const ldg_math_phys_atmosphere_hdr_t *hdr, double altitude)
{
    double radius = 0.0;
    double ratio = 0.0;
    double horizon = 0.0;

    if (altitude < 0.0) { altitude = 0.0; }

    radius = hdr->planet_radius + altitude;
    ratio = hdr->planet_radius / radius;
    horizon = 1.0 - ratio * ratio;
    if (horizon < 0.0) { horizon = 0.0; }

    return 0.0 - ldg_math_sqrt_get(horizon);
}

static void ldg_math_phys_atmosphere_axis_get(double val, double lo, double span, uint64_t cunt, uint64_t *idx, double *frac)
{
    double coord = (val - lo) / span * ((double)cunt - 1.0);

    if (coord < 0.0) { coord = 0.0; }

    if (coord > (double)cunt - 1.0) { coord = (double)cunt - 1.0; }

    *idx = (uint64_t)coord;
    if (*idx >= cunt - 1) { *idx = cunt - 2; }

    *frac = coord - (double)*idx;
}

static double ldg_math_phys_atmosphere_transmittance_corner_get(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *transmittance, uint64_t idx_a, double frac_a, uint64_t idx_v, double frac_v, uint64_t bin)
{
    uint64_t row_lo = (idx_a * hdr->view_cunt + idx_v) * hdr->bin_cunt;
    uint64_t row_hi = ((idx_a + 1) * hdr->view_cunt + idx_v) * hdr->bin_cunt;

    return (transmittance[row_lo + bin] * (1.0 - frac_v) + transmittance[row_lo + hdr->bin_cunt + bin] * frac_v) * (1.0 - frac_a) + (transmittance[row_hi + bin] * (1.0 - frac_v) + transmittance[row_hi + hdr->bin_cunt + bin] * frac_v) * frac_a;
}

static double ldg_math_phys_atmosphere_transmittance_bin_get(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *transmittance, double altitude, double mu, uint64_t bin)
{
    double frac_a = 0.0;
    double frac_v = 0.0;
    uint64_t idx_a = 0;
    uint64_t idx_v = 0;

    ldg_math_phys_atmosphere_axis_get(altitude, 0.0, hdr->thickness, hdr->altitude_cunt, &idx_a, &frac_a);
    ldg_math_phys_atmosphere_axis_get(mu, 0.0 - 1.0, 2.0, hdr->view_cunt, &idx_v, &frac_v);
    return ldg_math_phys_atmosphere_transmittance_corner_get(hdr, transmittance, idx_a, frac_a, idx_v, frac_v, bin);
}

static double ldg_math_phys_atmosphere_sun_transmittance_bin_get(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *transmittance, double altitude, double mu_s, uint64_t bin)
{
    if (mu_s < ldg_math_phys_atmosphere_horizon_get(hdr, altitude)) { return 0.0; }

    return ldg_math_phys_atmosphere_transmittance_bin_get(hdr, transmittance, altitude, mu_s, bin);
}

static void ldg_math_phys_atmosphere_quad_axis_get(const ldg_math_phys_atmosphere_hdr_t *hdr, double altitude, double mu, double mu_s, double nu, uint64_t *idx, double *frac)
{
    ldg_math_phys_atmosphere_axis_get(altitude, 0.0, hdr->thickness, hdr->altitude_cunt, &idx[0], &frac[0]);
    ldg_math_phys_atmosphere_axis_get(mu, 0.0 - 1.0, 2.0, hdr->view_cunt, &idx[1], &frac[1]);
    ldg_math_phys_atmosphere_axis_get(mu_s, 0.0 - 1.0, 2.0, hdr->sun_cunt, &idx[2], &frac[2]);
    ldg_math_phys_atmosphere_axis_get(nu, 0.0 - 1.0, 2.0, hdr->view_sun_cunt, &idx[3], &frac[3]);
}

static double ldg_math_phys_atmosphere_quad_corner_get(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *grid, const uint64_t *idx, const double *frac, uint64_t bin)
{
    double weight = 0.0;
    double val = 0.0;
    uint64_t pick_a = 0;
    uint64_t pick_v = 0;
    uint64_t pick_s = 0;
    uint64_t pick_n = 0;
    uint64_t corner = 0;
    uint64_t base = 0;

    for (corner = 0; corner < 16; corner++)
    {
        pick_a = idx[0] + (corner & 1);
        pick_v = idx[1] + ((corner >> 1) & 1);
        pick_s = idx[2] + ((corner >> 2) & 1);
        pick_n = idx[3] + ((corner >> 3) & 1);
        weight = ((corner & 1) ? frac[0] : 1.0 - frac[0]) * (((corner >> 1) & 1) ? frac[1] : 1.0 - frac[1]) * (((corner >> 2) & 1) ? frac[2] : 1.0 - frac[2]) * (((corner >> 3) & 1) ? frac[3] : 1.0 - frac[3]);
        base = (((pick_a * hdr->view_cunt + pick_v) * hdr->sun_cunt + pick_s) * hdr->view_sun_cunt + pick_n) * hdr->bin_cunt;
        val += weight * grid[base + bin];
    }
    return val;
}

static double ldg_math_phys_atmosphere_delta_radiance_bin_get(const ldg_math_phys_atmosphere_table_t *table, const double *delta_radiance, const uint64_t *idx, const double *frac, double nu, uint64_t bin)
{
    if (delta_radiance != 0x0) { return ldg_math_phys_atmosphere_quad_corner_get(table->hdr, delta_radiance, idx, frac, bin); }

    return ldg_math_phys_atmosphere_quad_corner_get(table->hdr, table->delta_rayleigh, idx, frac, bin) * ldg_math_phys_atmosphere_rayleigh_phase_get(nu) + ldg_math_phys_atmosphere_quad_corner_get(table->hdr, table->delta_mie, idx, frac, bin) * ldg_math_phys_atmosphere_mie_phase_get(table->hdr->mie_anisotropy, nu) + ldg_math_phys_atmosphere_quad_corner_get(table->hdr, table->delta_fog, idx, frac, bin) * ldg_math_phys_atmosphere_mie_phase_get(table->hdr->fog_anisotropy, nu);
}

static double ldg_math_phys_atmosphere_ground_irradiance_bin_get(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *irradiance_table, double mu_s, uint64_t bin)
{
    double frac_s = 0.0;
    uint64_t idx_s = 0;

    ldg_math_phys_atmosphere_axis_get(mu_s, 0.0 - 1.0, 2.0, hdr->sun_cunt, &idx_s, &frac_s);
    return irradiance_table[idx_s * hdr->bin_cunt + bin] * (1.0 - frac_s) + irradiance_table[(idx_s + 1) * hdr->bin_cunt + bin] * frac_s;
}

static double ldg_math_phys_atmosphere_inscatter_bin_get(const ldg_math_phys_atmosphere_table_t *table, double altitude, double mu, double mu_s, double nu, uint64_t bin)
{
    double frac[4] = LDG_ARR_ZERO_INIT;
    uint64_t idx[4] = LDG_ARR_ZERO_INIT;

    ldg_math_phys_atmosphere_quad_axis_get(table->hdr, altitude, mu, mu_s, nu, idx, frac);
    return ldg_math_phys_atmosphere_quad_corner_get(table->hdr, table->delta_rayleigh, idx, frac, bin) * ldg_math_phys_atmosphere_rayleigh_phase_get(nu) + ldg_math_phys_atmosphere_quad_corner_get(table->hdr, table->delta_mie, idx, frac, bin) * ldg_math_phys_atmosphere_mie_phase_get(table->hdr->mie_anisotropy, nu) + ldg_math_phys_atmosphere_quad_corner_get(table->hdr, table->delta_fog, idx, frac, bin) * ldg_math_phys_atmosphere_mie_phase_get(table->hdr->fog_anisotropy, nu) + ldg_math_phys_atmosphere_quad_corner_get(table->hdr, table->multi, idx, frac, bin);
}

static double ldg_math_phys_atmosphere_full_radiance_bin_get(const ldg_math_phys_atmosphere_table_t *table, double altitude, double mu, double mu_s, double nu, uint64_t bin)
{
    double radiance = 0.0;
    double radius = 0.0;
    double ray_len = 0.0;
    double mu_s_ground = 0.0;
    double direct_weight = 0.0;
    double view_transmittance = 0.0;
    double sun_transmittance = 0.0;
    double sky_irradiance = 0.0;
    uint8_t ground_hit = UINT8_MAX;

    radiance = ldg_math_phys_atmosphere_inscatter_bin_get(table, altitude, mu, mu_s, nu, bin);
    radius = table->hdr->planet_radius + altitude;
    ray_len = ldg_math_phys_atmosphere_ray_len_get(table->hdr, radius, mu, &ground_hit);
    if (ground_hit != LDG_TRUTH_TRUE) { return radiance; }

    mu_s_ground = (radius * mu_s + ray_len * nu) / table->hdr->planet_radius;
    if (mu_s_ground > 1.0) { mu_s_ground = 1.0; }

    if (mu_s_ground < 0.0 - 1.0) { mu_s_ground = 0.0 - 1.0; }

    view_transmittance = ldg_math_phys_atmosphere_transmittance_bin_get(table->hdr, table->transmittance, altitude, mu, bin);
    sun_transmittance = ldg_math_phys_atmosphere_sun_transmittance_bin_get(table->hdr, table->transmittance, 0.0, mu_s_ground, bin);
    sky_irradiance = ldg_math_phys_atmosphere_ground_irradiance_bin_get(table->hdr, table->ground_irradiance, mu_s_ground, bin);
    direct_weight = mu_s_ground;
    if (direct_weight < 0.0) { direct_weight = 0.0; }

    return radiance + view_transmittance * (table->albedo[bin] / LDG_MATH_PI) * (sun_transmittance * table->sun_irradiance[bin] * direct_weight + sky_irradiance);
}

uint32_t ldg_math_phys_atmosphere_transmittance_fill(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *rayleigh, const double *ozone, double *transmittance)
{
    double altitude = 0.0;
    double radius = 0.0;
    double mu = 0.0;
    double ray_len = 0.0;
    double step = 0.0;
    double dist = 0.0;
    double radius_dist = 0.0;
    double altitude_dist = 0.0;
    double depth_rayleigh = 0.0;
    double depth_mie = 0.0;
    double depth_fog = 0.0;
    double depth_ozone = 0.0;
    double mie_extinction = 0.0;
    double fog_extinction = 0.0;
    double dist_next = 0.0;
    double radius_next = 0.0;
    double altitude_next = 0.0;
    double altitude_prev = 0.0;
    uint64_t idx_a = 0;
    uint64_t idx_v = 0;
    uint64_t step_idx = 0;
    uint64_t base = 0;
    uint64_t bin = 0;
    uint8_t ground_hit = UINT8_MAX;

    if (LDG_UNLIKELY(!hdr || !rayleigh || !ozone || !transmittance)) { return LDG_ERR_FUNC_ARG_NULL; }

    mie_extinction = hdr->mie_scatter + hdr->mie_absorb;
    fog_extinction = hdr->fog_scatter + hdr->fog_absorb;
    for (idx_a = 0; idx_a < hdr->altitude_cunt; idx_a++)
    {
        altitude = hdr->thickness * (double)idx_a / ((double)hdr->altitude_cunt - 1.0);
        radius = hdr->planet_radius + altitude;
        for (idx_v = 0; idx_v < hdr->view_cunt; idx_v++)
        {
            mu = 0.0 - 1.0 + 2.0 * (double)idx_v / ((double)hdr->view_cunt - 1.0);
            ray_len = ldg_math_phys_atmosphere_ray_len_get(hdr, radius, mu, &ground_hit);
            step = ray_len / (double)LDG_MATH_PHYS_ATMOSPHERE_TRANSMITTANCE_STEP_CUNT;
            depth_rayleigh = 0.0;
            depth_mie = 0.0;
            depth_fog = 0.0;
            depth_ozone = 0.0;
            altitude_prev = altitude;
            for (step_idx = 0; step_idx < LDG_MATH_PHYS_ATMOSPHERE_TRANSMITTANCE_STEP_CUNT; step_idx++)
            {
                dist = ((double)step_idx + 0.5) * step;
                dist_next = ((double)step_idx + 1.0) * step;
                radius_dist = ldg_math_sqrt_get(radius * radius + dist * dist + 2.0 * radius * dist * mu);
                altitude_dist = radius_dist - hdr->planet_radius;
                radius_next = ldg_math_sqrt_get(radius * radius + dist_next * dist_next + 2.0 * radius * dist_next * mu);
                altitude_next = radius_next - hdr->planet_radius;
                depth_rayleigh += ldg_math_phys_atmosphere_density_get(altitude_dist, hdr->rayleigh_scale_height) * step;
                depth_mie += ldg_math_phys_atmosphere_density_get(altitude_dist, hdr->mie_scale_height) * step;
                depth_fog += ldg_math_phys_atmosphere_segment_depth_get(hdr->fog_scale_height, altitude_prev, altitude_dist, step * 0.5) + ldg_math_phys_atmosphere_segment_depth_get(hdr->fog_scale_height, altitude_dist, altitude_next, step * 0.5);
                depth_ozone += ldg_math_phys_atmosphere_ozone_segment_depth_get(hdr, altitude_prev, altitude_dist, step * 0.5) + ldg_math_phys_atmosphere_ozone_segment_depth_get(hdr, altitude_dist, altitude_next, step * 0.5);
                altitude_prev = altitude_next;
            }
            base = (idx_a * hdr->view_cunt + idx_v) * hdr->bin_cunt;
            for (bin = 0; bin < hdr->bin_cunt; bin++) { transmittance[base + bin] = ldg_math_exp_get(0.0 - (rayleigh[bin] * depth_rayleigh + mie_extinction * depth_mie + fog_extinction * depth_fog + ozone[bin] * depth_ozone)); }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_atmosphere_single_fill(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *rayleigh, const double *ozone, const double *sun_irradiance, const double *transmittance, double *delta_rayleigh, double *delta_mie, double *delta_fog)
{
    double altitude = 0.0;
    double radius = 0.0;
    double mu = 0.0;
    double mu_s = 0.0;
    double nu = 0.0;
    double ray_len = 0.0;
    double step = 0.0;
    double dist = 0.0;
    double radius_dist = 0.0;
    double altitude_dist = 0.0;
    double density_rayleigh = 0.0;
    double density_mie = 0.0;
    double depth_rayleigh = 0.0;
    double depth_mie = 0.0;
    double depth_fog = 0.0;
    double depth_ozone = 0.0;
    double depth_rayleigh_mid = 0.0;
    double depth_mie_mid = 0.0;
    double depth_fog_mid = 0.0;
    double depth_ozone_mid = 0.0;
    double dist_next = 0.0;
    double radius_next = 0.0;
    double altitude_next = 0.0;
    double altitude_prev = 0.0;
    double fog_span = 0.0;
    double fog_start = 0.0;
    double fog_step = 0.0;
    double pre_step = 0.0;
    double fog_seg_lo = 0.0;
    double fog_seg_hi = 0.0;
    double ozone_seg_lo = 0.0;
    double ozone_seg_hi = 0.0;
    double mu_s_dist = 0.0;
    double horizon = 0.0;
    double frac_a = 0.0;
    double frac_v = 0.0;
    double sun_transmittance = 0.0;
    double view_transmittance = 0.0;
    double mie_extinction = 0.0;
    double fog_extinction = 0.0;
    uint64_t sun_idx_a = 0;
    uint64_t sun_idx_v = 0;
    uint64_t idx_a = 0;
    uint64_t idx_v = 0;
    uint64_t idx_s = 0;
    uint64_t idx_n = 0;
    uint64_t step_idx = 0;
    uint64_t base = 0;
    uint64_t bin = 0;
    uint8_t ground_hit = UINT8_MAX;
    uint8_t sun_lit = UINT8_MAX;

    if (LDG_UNLIKELY(!hdr || !rayleigh || !ozone || !sun_irradiance || !transmittance || !delta_rayleigh || !delta_mie || !delta_fog)) { return LDG_ERR_FUNC_ARG_NULL; }

    mie_extinction = hdr->mie_scatter + hdr->mie_absorb;
    fog_extinction = hdr->fog_scatter + hdr->fog_absorb;
    for (idx_a = 0; idx_a < hdr->altitude_cunt; idx_a++)
    {
        altitude = hdr->thickness * (double)idx_a / ((double)hdr->altitude_cunt - 1.0);
        radius = hdr->planet_radius + altitude;
        for (idx_v = 0; idx_v < hdr->view_cunt; idx_v++)
        {
            mu = 0.0 - 1.0 + 2.0 * (double)idx_v / ((double)hdr->view_cunt - 1.0);
            ray_len = ldg_math_phys_atmosphere_ray_len_get(hdr, radius, mu, &ground_hit);
            step = ray_len / (double)LDG_MATH_PHYS_ATMOSPHERE_SCATTER_STEP_CUNT;
            for (idx_s = 0; idx_s < hdr->sun_cunt; idx_s++)
            {
                mu_s = 0.0 - 1.0 + 2.0 * (double)idx_s / ((double)hdr->sun_cunt - 1.0);
                for (idx_n = 0; idx_n < hdr->view_sun_cunt; idx_n++)
                {
                    nu = 0.0 - 1.0 + 2.0 * (double)idx_n / ((double)hdr->view_sun_cunt - 1.0);
                    depth_rayleigh = 0.0;
                    depth_mie = 0.0;
                    depth_fog = 0.0;
                    depth_ozone = 0.0;
                    altitude_prev = altitude;
                    base = (((idx_a * hdr->view_cunt + idx_v) * hdr->sun_cunt + idx_s) * hdr->view_sun_cunt + idx_n) * hdr->bin_cunt;
                    for (bin = 0; bin < hdr->bin_cunt; bin++)
                    {
                        delta_rayleigh[base + bin] = 0.0;
                        delta_mie[base + bin] = 0.0;
                        delta_fog[base + bin] = 0.0;
                    }
                    for (step_idx = 0; step_idx < LDG_MATH_PHYS_ATMOSPHERE_SCATTER_STEP_CUNT; step_idx++)
                    {
                        dist = ((double)step_idx + 0.5) * step;
                        dist_next = ((double)step_idx + 1.0) * step;
                        radius_dist = ldg_math_sqrt_get(radius * radius + dist * dist + 2.0 * radius * dist * mu);
                        altitude_dist = radius_dist - hdr->planet_radius;
                        radius_next = ldg_math_sqrt_get(radius * radius + dist_next * dist_next + 2.0 * radius * dist_next * mu);
                        altitude_next = radius_next - hdr->planet_radius;
                        density_rayleigh = ldg_math_phys_atmosphere_density_get(altitude_dist, hdr->rayleigh_scale_height);
                        density_mie = ldg_math_phys_atmosphere_density_get(altitude_dist, hdr->mie_scale_height);
                        fog_seg_lo = ldg_math_phys_atmosphere_segment_depth_get(hdr->fog_scale_height, altitude_prev, altitude_dist, step * 0.5);
                        fog_seg_hi = ldg_math_phys_atmosphere_segment_depth_get(hdr->fog_scale_height, altitude_dist, altitude_next, step * 0.5);
                        ozone_seg_lo = ldg_math_phys_atmosphere_ozone_segment_depth_get(hdr, altitude_prev, altitude_dist, step * 0.5);
                        ozone_seg_hi = ldg_math_phys_atmosphere_ozone_segment_depth_get(hdr, altitude_dist, altitude_next, step * 0.5);
                        depth_rayleigh_mid = depth_rayleigh + density_rayleigh * step * 0.5;
                        depth_mie_mid = depth_mie + density_mie * step * 0.5;
                        depth_fog_mid = depth_fog + fog_seg_lo;
                        depth_ozone_mid = depth_ozone + ozone_seg_lo;
                        depth_rayleigh += density_rayleigh * step;
                        depth_mie += density_mie * step;
                        depth_fog += fog_seg_lo + fog_seg_hi;
                        depth_ozone += ozone_seg_lo + ozone_seg_hi;
                        altitude_prev = altitude_next;
                        mu_s_dist = (radius * mu_s + dist * nu) / radius_dist;
                        if (mu_s_dist > 1.0) { mu_s_dist = 1.0; }

                        if (mu_s_dist < 0.0 - 1.0) { mu_s_dist = 0.0 - 1.0; }

                        horizon = ldg_math_phys_atmosphere_horizon_get(hdr, altitude_dist);
                        sun_lit = (mu_s_dist >= horizon) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
                        if (sun_lit != LDG_TRUTH_TRUE) { continue; }

                        ldg_math_phys_atmosphere_axis_get(altitude_dist, 0.0, hdr->thickness, hdr->altitude_cunt, &sun_idx_a, &frac_a);
                        ldg_math_phys_atmosphere_axis_get(mu_s_dist, 0.0 - 1.0, 2.0, hdr->view_cunt, &sun_idx_v, &frac_v);
                        for (bin = 0; bin < hdr->bin_cunt; bin++)
                        {
                            view_transmittance = ldg_math_exp_get(0.0 - (rayleigh[bin] * depth_rayleigh_mid + mie_extinction * depth_mie_mid + fog_extinction * depth_fog_mid + ozone[bin] * depth_ozone_mid));
                            sun_transmittance = ldg_math_phys_atmosphere_transmittance_corner_get(hdr, transmittance, sun_idx_a, frac_a, sun_idx_v, frac_v, bin);
                            delta_rayleigh[base + bin] += view_transmittance * sun_transmittance * density_rayleigh * step;
                            delta_mie[base + bin] += view_transmittance * sun_transmittance * density_mie * step;
                        }
                    }
                    fog_span = ldg_math_phys_atmosphere_fog_span_get(hdr, radius, mu, ray_len, &fog_start);
                    if (fog_span > 0.0)
                    {
                        depth_rayleigh = 0.0;
                        depth_mie = 0.0;
                        depth_fog = 0.0;
                        depth_ozone = 0.0;
                        altitude_prev = altitude;
                        if (fog_start > 0.0)
                        {
                            pre_step = fog_start / (double)LDG_MATH_PHYS_ATMOSPHERE_SCATTER_STEP_CUNT;
                            for (step_idx = 0; step_idx < LDG_MATH_PHYS_ATMOSPHERE_SCATTER_STEP_CUNT; step_idx++)
                            {
                                dist = ((double)step_idx + 0.5) * pre_step;
                                dist_next = ((double)step_idx + 1.0) * pre_step;
                                radius_dist = ldg_math_sqrt_get(radius * radius + dist * dist + 2.0 * radius * dist * mu);
                                altitude_dist = radius_dist - hdr->planet_radius;
                                radius_next = ldg_math_sqrt_get(radius * radius + dist_next * dist_next + 2.0 * radius * dist_next * mu);
                                altitude_next = radius_next - hdr->planet_radius;
                                depth_rayleigh += ldg_math_phys_atmosphere_density_get(altitude_dist, hdr->rayleigh_scale_height) * pre_step;
                                depth_mie += ldg_math_phys_atmosphere_density_get(altitude_dist, hdr->mie_scale_height) * pre_step;
                                depth_ozone += ldg_math_phys_atmosphere_ozone_segment_depth_get(hdr, altitude_prev, altitude_dist, pre_step * 0.5) + ldg_math_phys_atmosphere_ozone_segment_depth_get(hdr, altitude_dist, altitude_next, pre_step * 0.5);
                                altitude_prev = altitude_next;
                            }
                        }

                        fog_step = fog_span / (double)LDG_MATH_PHYS_ATMOSPHERE_SCATTER_STEP_CUNT;
                        radius_dist = ldg_math_sqrt_get(radius * radius + fog_start * fog_start + 2.0 * radius * fog_start * mu);
                        altitude_prev = radius_dist - hdr->planet_radius;
                        for (step_idx = 0; step_idx < LDG_MATH_PHYS_ATMOSPHERE_SCATTER_STEP_CUNT; step_idx++)
                        {
                            dist = fog_start + ((double)step_idx + 0.5) * fog_step;
                            dist_next = fog_start + ((double)step_idx + 1.0) * fog_step;
                            radius_dist = ldg_math_sqrt_get(radius * radius + dist * dist + 2.0 * radius * dist * mu);
                            altitude_dist = radius_dist - hdr->planet_radius;
                            radius_next = ldg_math_sqrt_get(radius * radius + dist_next * dist_next + 2.0 * radius * dist_next * mu);
                            altitude_next = radius_next - hdr->planet_radius;
                            density_rayleigh = ldg_math_phys_atmosphere_density_get(altitude_dist, hdr->rayleigh_scale_height);
                            density_mie = ldg_math_phys_atmosphere_density_get(altitude_dist, hdr->mie_scale_height);
                            fog_seg_lo = ldg_math_phys_atmosphere_segment_depth_get(hdr->fog_scale_height, altitude_prev, altitude_dist, fog_step * 0.5);
                            fog_seg_hi = ldg_math_phys_atmosphere_segment_depth_get(hdr->fog_scale_height, altitude_dist, altitude_next, fog_step * 0.5);
                            ozone_seg_lo = ldg_math_phys_atmosphere_ozone_segment_depth_get(hdr, altitude_prev, altitude_dist, fog_step * 0.5);
                            ozone_seg_hi = ldg_math_phys_atmosphere_ozone_segment_depth_get(hdr, altitude_dist, altitude_next, fog_step * 0.5);
                            depth_rayleigh_mid = depth_rayleigh + density_rayleigh * fog_step * 0.5;
                            depth_mie_mid = depth_mie + density_mie * fog_step * 0.5;
                            depth_fog_mid = depth_fog + fog_seg_lo;
                            depth_ozone_mid = depth_ozone + ozone_seg_lo;
                            depth_rayleigh += density_rayleigh * fog_step;
                            depth_mie += density_mie * fog_step;
                            depth_fog += fog_seg_lo + fog_seg_hi;
                            depth_ozone += ozone_seg_lo + ozone_seg_hi;
                            altitude_prev = altitude_next;
                            mu_s_dist = (radius * mu_s + dist * nu) / radius_dist;
                            if (mu_s_dist > 1.0) { mu_s_dist = 1.0; }

                            if (mu_s_dist < 0.0 - 1.0) { mu_s_dist = 0.0 - 1.0; }

                            horizon = ldg_math_phys_atmosphere_horizon_get(hdr, altitude_dist);
                            sun_lit = (mu_s_dist >= horizon) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
                            if (sun_lit != LDG_TRUTH_TRUE) { continue; }

                            ldg_math_phys_atmosphere_axis_get(altitude_dist, 0.0, hdr->thickness, hdr->altitude_cunt, &sun_idx_a, &frac_a);
                            ldg_math_phys_atmosphere_axis_get(mu_s_dist, 0.0 - 1.0, 2.0, hdr->view_cunt, &sun_idx_v, &frac_v);
                            for (bin = 0; bin < hdr->bin_cunt; bin++)
                            {
                                view_transmittance = ldg_math_exp_get(0.0 - (rayleigh[bin] * depth_rayleigh_mid + mie_extinction * depth_mie_mid + fog_extinction * depth_fog_mid + ozone[bin] * depth_ozone_mid));
                                sun_transmittance = ldg_math_phys_atmosphere_transmittance_corner_get(hdr, transmittance, sun_idx_a, frac_a, sun_idx_v, frac_v, bin);
                                delta_fog[base + bin] += view_transmittance * sun_transmittance * (fog_seg_lo + fog_seg_hi);
                            }
                        }
                    }

                    for (bin = 0; bin < hdr->bin_cunt; bin++)
                    {
                        delta_rayleigh[base + bin] *= sun_irradiance[bin] * rayleigh[bin];
                        delta_mie[base + bin] *= sun_irradiance[bin] * hdr->mie_scatter;
                        delta_fog[base + bin] *= sun_irradiance[bin] * hdr->fog_scatter;
                    }
                }
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_atmosphere_irradiance_fill(const ldg_math_phys_atmosphere_table_t *table, const double *delta_radiance, uint64_t order, double *delta_irradiance)
{
    double frac[4] = LDG_ARR_ZERO_INIT;
    uint64_t idx[4] = LDG_ARR_ZERO_INIT;
    double mu_s = 0.0;
    double sin_s = 0.0;
    double direct_weight = 0.0;
    double polar = 0.0;
    double cos_polar = 0.0;
    double sin_polar = 0.0;
    double azim = 0.0;
    double dir_x = 0.0;
    double nu = 0.0;
    double weight = 0.0;
    double polar_step = 0.0;
    double azim_step = 0.0;
    uint64_t idx_s = 0;
    uint64_t polar_idx = 0;
    uint64_t azim_idx = 0;
    uint64_t base = 0;
    uint64_t bin = 0;

    if (LDG_UNLIKELY(!table || !delta_irradiance)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(order >= 2 && !delta_radiance)) { return LDG_ERR_FUNC_ARG_NULL; }

    polar_step = (LDG_MATH_PI * 0.5) / (double)LDG_MATH_PHYS_ATMOSPHERE_HEMI_POLAR_CUNT;
    azim_step = LDG_MATH_TAU / (double)LDG_MATH_PHYS_ATMOSPHERE_HEMI_AZIM_CUNT;
    for (idx_s = 0; idx_s < table->hdr->sun_cunt; idx_s++)
    {
        mu_s = 0.0 - 1.0 + 2.0 * (double)idx_s / ((double)table->hdr->sun_cunt - 1.0);
        base = idx_s * table->hdr->bin_cunt;
        if (order == 0)
        {
            direct_weight = mu_s;
            if (direct_weight < 0.0) { direct_weight = 0.0; }

            for (bin = 0; bin < table->hdr->bin_cunt; bin++) { delta_irradiance[base + bin] = ldg_math_phys_atmosphere_sun_transmittance_bin_get(table->hdr, table->transmittance, 0.0, mu_s, bin) * table->sun_irradiance[bin] * direct_weight; }
            continue;
        }

        sin_s = 1.0 - mu_s * mu_s;
        if (sin_s < 0.0) { sin_s = 0.0; }

        sin_s = ldg_math_sqrt_get(sin_s);
        for (bin = 0; bin < table->hdr->bin_cunt; bin++) { delta_irradiance[base + bin] = 0.0; }
        for (polar_idx = 0; polar_idx < LDG_MATH_PHYS_ATMOSPHERE_HEMI_POLAR_CUNT; polar_idx++)
        {
            polar = ((double)polar_idx + 0.5) * polar_step;
            cos_polar = ldg_math_cos_get(polar);
            sin_polar = ldg_math_sin_get(polar);
            for (azim_idx = 0; azim_idx < LDG_MATH_PHYS_ATMOSPHERE_HEMI_AZIM_CUNT; azim_idx++)
            {
                azim = ((double)azim_idx + 0.5) * azim_step;
                dir_x = sin_polar * ldg_math_cos_get(azim);
                nu = dir_x * sin_s + cos_polar * mu_s;
                ldg_math_phys_atmosphere_quad_axis_get(table->hdr, 0.0, cos_polar, mu_s, nu, idx, frac);
                weight = cos_polar * sin_polar * polar_step * azim_step;
                for (bin = 0; bin < table->hdr->bin_cunt; bin++) { delta_irradiance[base + bin] += ldg_math_phys_atmosphere_delta_radiance_bin_get(table, order == 1 ? 0x0 : delta_radiance, idx, frac, nu, bin) * weight; }
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_atmosphere_gather_fill(const ldg_math_phys_atmosphere_table_t *table, const double *delta_radiance, const double *delta_irradiance, double *gather)
{
    double frac[4] = LDG_ARR_ZERO_INIT;
    uint64_t idx[4] = LDG_ARR_ZERO_INIT;
    double altitude = 0.0;
    double radius = 0.0;
    double density_rayleigh = 0.0;
    double density_mie = 0.0;
    double density_fog = 0.0;
    double mu = 0.0;
    double sin_view = 0.0;
    double mu_s = 0.0;
    double nu = 0.0;
    double sun_x = 0.0;
    double sun_y = 0.0;
    double sun_lim = 0.0;
    double polar = 0.0;
    double cos_polar = 0.0;
    double sin_polar = 0.0;
    double azim = 0.0;
    double dir_x = 0.0;
    double dir_y = 0.0;
    double nu_sun = 0.0;
    double nu_view = 0.0;
    double weight = 0.0;
    double polar_step = 0.0;
    double azim_step = 0.0;
    double ray_len = 0.0;
    double mu_s_ground = 0.0;
    double phase_rayleigh = 0.0;
    double phase_mie = 0.0;
    double phase_fog = 0.0;
    double incoming = 0.0;
    double trans_frac_a = 0.0;
    double trans_frac_v = 0.0;
    uint64_t trans_idx_a = 0;
    uint64_t trans_idx_v = 0;
    uint64_t idx_a = 0;
    uint64_t idx_v = 0;
    uint64_t idx_s = 0;
    uint64_t idx_n = 0;
    uint64_t polar_idx = 0;
    uint64_t azim_idx = 0;
    uint64_t base = 0;
    uint64_t bin = 0;
    uint8_t ground_hit = UINT8_MAX;

    if (LDG_UNLIKELY(!table || !delta_irradiance || !gather)) { return LDG_ERR_FUNC_ARG_NULL; }

    polar_step = LDG_MATH_PI / (double)LDG_MATH_PHYS_ATMOSPHERE_GATHER_POLAR_CUNT;
    azim_step = LDG_MATH_TAU / (double)LDG_MATH_PHYS_ATMOSPHERE_GATHER_AZIM_CUNT;
    for (idx_a = 0; idx_a < table->hdr->altitude_cunt; idx_a++)
    {
        altitude = table->hdr->thickness * (double)idx_a / ((double)table->hdr->altitude_cunt - 1.0);
        radius = table->hdr->planet_radius + altitude;
        density_rayleigh = ldg_math_phys_atmosphere_density_get(altitude, table->hdr->rayleigh_scale_height);
        density_mie = ldg_math_phys_atmosphere_density_get(altitude, table->hdr->mie_scale_height);
        density_fog = ldg_math_phys_atmosphere_fog_density_get(table->hdr, altitude);
        for (idx_v = 0; idx_v < table->hdr->view_cunt; idx_v++)
        {
            mu = 0.0 - 1.0 + 2.0 * (double)idx_v / ((double)table->hdr->view_cunt - 1.0);
            sin_view = 1.0 - mu * mu;
            if (sin_view < 0.0) { sin_view = 0.0; }

            sin_view = ldg_math_sqrt_get(sin_view);
            for (idx_s = 0; idx_s < table->hdr->sun_cunt; idx_s++)
            {
                mu_s = 0.0 - 1.0 + 2.0 * (double)idx_s / ((double)table->hdr->sun_cunt - 1.0);
                for (idx_n = 0; idx_n < table->hdr->view_sun_cunt; idx_n++)
                {
                    nu = 0.0 - 1.0 + 2.0 * (double)idx_n / ((double)table->hdr->view_sun_cunt - 1.0);
                    sun_lim = 1.0 - mu_s * mu_s;
                    if (sun_lim < 0.0) { sun_lim = 0.0; }

                    sun_lim = ldg_math_sqrt_get(sun_lim);
                    if (sin_view < LDG_MATH_EPSILON) { sun_x = sun_lim; }
                    else { sun_x = (nu - mu * mu_s) / sin_view; }

                    if (sun_x > sun_lim) { sun_x = sun_lim; }

                    if (sun_x < 0.0 - sun_lim) { sun_x = 0.0 - sun_lim; }

                    sun_y = sun_lim * sun_lim - sun_x * sun_x;
                    if (sun_y < 0.0) { sun_y = 0.0; }

                    sun_y = ldg_math_sqrt_get(sun_y);
                    base = (((idx_a * table->hdr->view_cunt + idx_v) * table->hdr->sun_cunt + idx_s) * table->hdr->view_sun_cunt + idx_n) * table->hdr->bin_cunt;
                    for (bin = 0; bin < table->hdr->bin_cunt; bin++) { gather[base + bin] = 0.0; }
                    for (polar_idx = 0; polar_idx < LDG_MATH_PHYS_ATMOSPHERE_GATHER_POLAR_CUNT; polar_idx++)
                    {
                        polar = ((double)polar_idx + 0.5) * polar_step;
                        cos_polar = ldg_math_cos_get(polar);
                        sin_polar = ldg_math_sin_get(polar);
                        for (azim_idx = 0; azim_idx < LDG_MATH_PHYS_ATMOSPHERE_GATHER_AZIM_CUNT; azim_idx++)
                        {
                            azim = ((double)azim_idx + 0.5) * azim_step;
                            dir_x = sin_polar * ldg_math_cos_get(azim);
                            dir_y = sin_polar * ldg_math_sin_get(azim);
                            nu_sun = dir_x * sun_x + dir_y * sun_y + cos_polar * mu_s;
                            if (nu_sun > 1.0) { nu_sun = 1.0; }

                            if (nu_sun < 0.0 - 1.0) { nu_sun = 0.0 - 1.0; }

                            nu_view = dir_x * sin_view + cos_polar * mu;
                            if (nu_view > 1.0) { nu_view = 1.0; }

                            if (nu_view < 0.0 - 1.0) { nu_view = 0.0 - 1.0; }

                            ldg_math_phys_atmosphere_quad_axis_get(table->hdr, altitude, cos_polar, mu_s, nu_sun, idx, frac);
                            ray_len = ldg_math_phys_atmosphere_ray_len_get(table->hdr, radius, cos_polar, &ground_hit);
                            if (ground_hit == LDG_TRUTH_TRUE)
                            {
                                mu_s_ground = (radius * mu_s + ray_len * nu_sun) / table->hdr->planet_radius;
                                if (mu_s_ground > 1.0) { mu_s_ground = 1.0; }

                                if (mu_s_ground < 0.0 - 1.0) { mu_s_ground = 0.0 - 1.0; }

                                ldg_math_phys_atmosphere_axis_get(altitude, 0.0, table->hdr->thickness, table->hdr->altitude_cunt, &trans_idx_a, &trans_frac_a);
                                ldg_math_phys_atmosphere_axis_get(cos_polar, 0.0 - 1.0, 2.0, table->hdr->view_cunt, &trans_idx_v, &trans_frac_v);
                            }

                            phase_rayleigh = ldg_math_phys_atmosphere_rayleigh_phase_get(nu_view);
                            phase_mie = ldg_math_phys_atmosphere_mie_phase_get(table->hdr->mie_anisotropy, nu_view);
                            phase_fog = ldg_math_phys_atmosphere_mie_phase_get(table->hdr->fog_anisotropy, nu_view);
                            weight = sin_polar * polar_step * azim_step;
                            for (bin = 0; bin < table->hdr->bin_cunt; bin++)
                            {
                                incoming = ldg_math_phys_atmosphere_delta_radiance_bin_get(table, delta_radiance, idx, frac, nu_sun, bin);
                                if (ground_hit == LDG_TRUTH_TRUE) { incoming += ldg_math_phys_atmosphere_transmittance_corner_get(table->hdr, table->transmittance, trans_idx_a, trans_frac_a, trans_idx_v, trans_frac_v, bin) * (table->albedo[bin] / LDG_MATH_PI) * ldg_math_phys_atmosphere_ground_irradiance_bin_get(table->hdr, delta_irradiance, mu_s_ground, bin); }

                                gather[base + bin] += (table->rayleigh[bin] * density_rayleigh * phase_rayleigh + table->hdr->mie_scatter * density_mie * phase_mie + table->hdr->fog_scatter * density_fog * phase_fog) * incoming * weight;
                            }
                        }
                    }
                }
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_atmosphere_order_fill(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *rayleigh, const double *ozone, const double *gather, double *delta_radiance)
{
    double frac[4] = LDG_ARR_ZERO_INIT;
    uint64_t idx[4] = LDG_ARR_ZERO_INIT;
    double altitude = 0.0;
    double radius = 0.0;
    double mu = 0.0;
    double mu_s = 0.0;
    double nu = 0.0;
    double ray_len = 0.0;
    double step = 0.0;
    double dist = 0.0;
    double radius_dist = 0.0;
    double altitude_dist = 0.0;
    double mu_dist = 0.0;
    double mu_s_dist = 0.0;
    double density_rayleigh = 0.0;
    double density_mie = 0.0;
    double depth_rayleigh = 0.0;
    double depth_mie = 0.0;
    double depth_fog = 0.0;
    double depth_ozone = 0.0;
    double depth_rayleigh_mid = 0.0;
    double depth_mie_mid = 0.0;
    double depth_fog_mid = 0.0;
    double depth_ozone_mid = 0.0;
    double dist_next = 0.0;
    double radius_next = 0.0;
    double altitude_next = 0.0;
    double altitude_prev = 0.0;
    double fog_seg_lo = 0.0;
    double fog_seg_hi = 0.0;
    double ozone_seg_lo = 0.0;
    double ozone_seg_hi = 0.0;
    double view_transmittance = 0.0;
    double mie_extinction = 0.0;
    double fog_extinction = 0.0;
    uint64_t idx_a = 0;
    uint64_t idx_v = 0;
    uint64_t idx_s = 0;
    uint64_t idx_n = 0;
    uint64_t step_idx = 0;
    uint64_t base = 0;
    uint64_t bin = 0;
    uint8_t ground_hit = UINT8_MAX;

    if (LDG_UNLIKELY(!hdr || !rayleigh || !ozone || !gather || !delta_radiance)) { return LDG_ERR_FUNC_ARG_NULL; }

    mie_extinction = hdr->mie_scatter + hdr->mie_absorb;
    fog_extinction = hdr->fog_scatter + hdr->fog_absorb;
    for (idx_a = 0; idx_a < hdr->altitude_cunt; idx_a++)
    {
        altitude = hdr->thickness * (double)idx_a / ((double)hdr->altitude_cunt - 1.0);
        radius = hdr->planet_radius + altitude;
        for (idx_v = 0; idx_v < hdr->view_cunt; idx_v++)
        {
            mu = 0.0 - 1.0 + 2.0 * (double)idx_v / ((double)hdr->view_cunt - 1.0);
            ray_len = ldg_math_phys_atmosphere_ray_len_get(hdr, radius, mu, &ground_hit);
            step = ray_len / (double)LDG_MATH_PHYS_ATMOSPHERE_SCATTER_STEP_CUNT;
            for (idx_s = 0; idx_s < hdr->sun_cunt; idx_s++)
            {
                mu_s = 0.0 - 1.0 + 2.0 * (double)idx_s / ((double)hdr->sun_cunt - 1.0);
                for (idx_n = 0; idx_n < hdr->view_sun_cunt; idx_n++)
                {
                    nu = 0.0 - 1.0 + 2.0 * (double)idx_n / ((double)hdr->view_sun_cunt - 1.0);
                    depth_rayleigh = 0.0;
                    depth_mie = 0.0;
                    depth_fog = 0.0;
                    depth_ozone = 0.0;
                    altitude_prev = altitude;
                    base = (((idx_a * hdr->view_cunt + idx_v) * hdr->sun_cunt + idx_s) * hdr->view_sun_cunt + idx_n) * hdr->bin_cunt;
                    for (bin = 0; bin < hdr->bin_cunt; bin++) { delta_radiance[base + bin] = 0.0; }
                    for (step_idx = 0; step_idx < LDG_MATH_PHYS_ATMOSPHERE_SCATTER_STEP_CUNT; step_idx++)
                    {
                        dist = ((double)step_idx + 0.5) * step;
                        dist_next = ((double)step_idx + 1.0) * step;
                        radius_dist = ldg_math_sqrt_get(radius * radius + dist * dist + 2.0 * radius * dist * mu);
                        altitude_dist = radius_dist - hdr->planet_radius;
                        radius_next = ldg_math_sqrt_get(radius * radius + dist_next * dist_next + 2.0 * radius * dist_next * mu);
                        altitude_next = radius_next - hdr->planet_radius;
                        density_rayleigh = ldg_math_phys_atmosphere_density_get(altitude_dist, hdr->rayleigh_scale_height);
                        density_mie = ldg_math_phys_atmosphere_density_get(altitude_dist, hdr->mie_scale_height);
                        fog_seg_lo = ldg_math_phys_atmosphere_segment_depth_get(hdr->fog_scale_height, altitude_prev, altitude_dist, step * 0.5);
                        fog_seg_hi = ldg_math_phys_atmosphere_segment_depth_get(hdr->fog_scale_height, altitude_dist, altitude_next, step * 0.5);
                        ozone_seg_lo = ldg_math_phys_atmosphere_ozone_segment_depth_get(hdr, altitude_prev, altitude_dist, step * 0.5);
                        ozone_seg_hi = ldg_math_phys_atmosphere_ozone_segment_depth_get(hdr, altitude_dist, altitude_next, step * 0.5);
                        depth_rayleigh_mid = depth_rayleigh + density_rayleigh * step * 0.5;
                        depth_mie_mid = depth_mie + density_mie * step * 0.5;
                        depth_fog_mid = depth_fog + fog_seg_lo;
                        depth_ozone_mid = depth_ozone + ozone_seg_lo;
                        depth_rayleigh += density_rayleigh * step;
                        depth_mie += density_mie * step;
                        depth_fog += fog_seg_lo + fog_seg_hi;
                        depth_ozone += ozone_seg_lo + ozone_seg_hi;
                        altitude_prev = altitude_next;
                        mu_dist = (radius * mu + dist) / radius_dist;
                        if (mu_dist > 1.0) { mu_dist = 1.0; }

                        if (mu_dist < 0.0 - 1.0) { mu_dist = 0.0 - 1.0; }

                        mu_s_dist = (radius * mu_s + dist * nu) / radius_dist;
                        if (mu_s_dist > 1.0) { mu_s_dist = 1.0; }

                        if (mu_s_dist < 0.0 - 1.0) { mu_s_dist = 0.0 - 1.0; }

                        ldg_math_phys_atmosphere_quad_axis_get(hdr, altitude_dist, mu_dist, mu_s_dist, nu, idx, frac);
                        for (bin = 0; bin < hdr->bin_cunt; bin++)
                        {
                            view_transmittance = ldg_math_exp_get(0.0 - (rayleigh[bin] * depth_rayleigh_mid + mie_extinction * depth_mie_mid + fog_extinction * depth_fog_mid + ozone[bin] * depth_ozone_mid));
                            delta_radiance[base + bin] += view_transmittance * ldg_math_phys_atmosphere_quad_corner_get(hdr, gather, idx, frac, bin) * step;
                        }
                    }
                }
            }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_atmosphere_transmittance_sample_get(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *transmittance, double altitude, double zenith_cos, double *out)
{
    uint64_t bin = 0;

    if (LDG_UNLIKELY(!hdr || !transmittance || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (bin = 0; bin < hdr->bin_cunt; bin++) { out[bin] = ldg_math_phys_atmosphere_transmittance_bin_get(hdr, transmittance, altitude, zenith_cos, bin); }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_atmosphere_extinction_sample_get(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *rayleigh, const double *ozone, double altitude, double *out)
{
    double density_rayleigh = 0.0;
    double density_mie = 0.0;
    double density_fog = 0.0;
    double density_ozone = 0.0;
    uint64_t bin = 0;

    if (LDG_UNLIKELY(!hdr || !rayleigh || !ozone || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    density_rayleigh = ldg_math_phys_atmosphere_density_get(altitude, hdr->rayleigh_scale_height);
    density_mie = ldg_math_phys_atmosphere_density_get(altitude, hdr->mie_scale_height);
    density_fog = ldg_math_phys_atmosphere_fog_density_get(hdr, altitude);
    density_ozone = ldg_math_phys_atmosphere_ozone_density_get(hdr, altitude);
    for (bin = 0; bin < hdr->bin_cunt; bin++) { out[bin] = rayleigh[bin] * density_rayleigh + (hdr->mie_scatter + hdr->mie_absorb) * density_mie + (hdr->fog_scatter + hdr->fog_absorb) * density_fog + ozone[bin] * density_ozone; }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_atmosphere_medium_sample_get(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *rayleigh, const double *ozone, uint32_t medium, double altitude, double *out_scatter, double *out_absorb, double *out_anisotropy)
{
    double density = 0.0;
    uint64_t bin = 0;

    if (LDG_UNLIKELY(!hdr || !rayleigh || !ozone || !out_scatter || !out_absorb || !out_anisotropy)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_anisotropy = 0.0;
    if (medium == LDG_MATH_PHYS_ATMOSPHERE_MEDIUM_OZONE)
    {
        density = ldg_math_phys_atmosphere_ozone_density_get(hdr, altitude);
        for (bin = 0; bin < hdr->bin_cunt; bin++)
        {
            out_scatter[bin] = 0.0;
            out_absorb[bin] = ozone[bin] * density;
        }
        return LDG_ERR_AOK;
    }

    if (medium == LDG_MATH_PHYS_ATMOSPHERE_MEDIUM_RAYLEIGH)
    {
        density = ldg_math_phys_atmosphere_density_get(altitude, hdr->rayleigh_scale_height);
        for (bin = 0; bin < hdr->bin_cunt; bin++)
        {
            out_scatter[bin] = rayleigh[bin] * density;
            out_absorb[bin] = 0.0;
        }
        return LDG_ERR_AOK;
    }

    if (medium == LDG_MATH_PHYS_ATMOSPHERE_MEDIUM_MIE)
    {
        density = ldg_math_phys_atmosphere_density_get(altitude, hdr->mie_scale_height);
        for (bin = 0; bin < hdr->bin_cunt; bin++)
        {
            out_scatter[bin] = hdr->mie_scatter * density;
            out_absorb[bin] = hdr->mie_absorb * density;
        }
        *out_anisotropy = hdr->mie_anisotropy;
        return LDG_ERR_AOK;
    }

    if (medium == LDG_MATH_PHYS_ATMOSPHERE_MEDIUM_FOG)
    {
        density = ldg_math_phys_atmosphere_fog_density_get(hdr, altitude);
        for (bin = 0; bin < hdr->bin_cunt; bin++)
        {
            out_scatter[bin] = hdr->fog_scatter * density;
            out_absorb[bin] = hdr->fog_absorb * density;
        }
        *out_anisotropy = hdr->fog_anisotropy;
        return LDG_ERR_AOK;
    }

    return LDG_ERR_FUNC_ARG_INVALID;
}

uint32_t ldg_math_phys_atmosphere_radiance_sample_get(const ldg_math_phys_atmosphere_table_t *table, double altitude, double view_cos, double sun_cos, double view_sun_cos, double *out)
{
    uint64_t bin = 0;

    if (LDG_UNLIKELY(!table || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (bin = 0; bin < table->hdr->bin_cunt; bin++) { out[bin] = ldg_math_phys_atmosphere_full_radiance_bin_get(table, altitude, view_cos, sun_cos, view_sun_cos, bin); }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_atmosphere_irradiance_sample_get(const ldg_math_phys_atmosphere_table_t *table, double altitude, double sun_cos, double normal_cos, double normal_sun_cos, double *out)
{
    double sin_s = 0.0;
    double sin_n = 0.0;
    double cos_phi = 0.0;
    double sin_phi = 0.0;
    double normal_x = 0.0;
    double normal_y = 0.0;
    double tangent_a_x = 0.0;
    double tangent_a_y = 0.0;
    double tangent_b_x = 0.0;
    double tangent_b_z = 0.0;
    double polar = 0.0;
    double cos_polar = 0.0;
    double sin_polar = 0.0;
    double azim = 0.0;
    double local_x = 0.0;
    double local_y = 0.0;
    double dir_x = 0.0;
    double dir_z = 0.0;
    double nu = 0.0;
    double weight = 0.0;
    double polar_step = 0.0;
    double azim_step = 0.0;
    uint64_t polar_idx = 0;
    uint64_t azim_idx = 0;
    uint64_t bin = 0;

    if (LDG_UNLIKELY(!table || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (bin = 0; bin < table->hdr->bin_cunt; bin++) { out[bin] = 0.0; }
    sin_s = 1.0 - sun_cos * sun_cos;
    if (sin_s < 0.0) { sin_s = 0.0; }

    sin_s = ldg_math_sqrt_get(sin_s);
    sin_n = 1.0 - normal_cos * normal_cos;
    if (sin_n < 0.0) { sin_n = 0.0; }

    sin_n = ldg_math_sqrt_get(sin_n);
    if (sin_n * sin_s < LDG_MATH_EPSILON) { cos_phi = 1.0; }
    else
    {
        cos_phi = (normal_sun_cos - normal_cos * sun_cos) / (sin_n * sin_s);
        if (cos_phi > 1.0) { cos_phi = 1.0; }

        if (cos_phi < 0.0 - 1.0) { cos_phi = 0.0 - 1.0; }
    }

    sin_phi = 1.0 - cos_phi * cos_phi;
    if (sin_phi < 0.0) { sin_phi = 0.0; }

    sin_phi = ldg_math_sqrt_get(sin_phi);
    normal_x = sin_n * cos_phi;
    normal_y = sin_n * sin_phi;
    if (sin_n < LDG_MATH_EPSILON)
    {
        tangent_a_x = 1.0;
        tangent_a_y = 0.0;
    }
    else
    {
        tangent_a_x = (0.0 - normal_y) / sin_n;
        tangent_a_y = normal_x / sin_n;
    }

    tangent_b_x = (0.0 - normal_cos) * tangent_a_y;
    tangent_b_z = normal_x * tangent_a_y - normal_y * tangent_a_x;
    polar_step = (LDG_MATH_PI * 0.5) / (double)LDG_MATH_PHYS_ATMOSPHERE_HEMI_POLAR_CUNT;
    azim_step = LDG_MATH_TAU / (double)LDG_MATH_PHYS_ATMOSPHERE_HEMI_AZIM_CUNT;
    for (polar_idx = 0; polar_idx < LDG_MATH_PHYS_ATMOSPHERE_HEMI_POLAR_CUNT; polar_idx++)
    {
        polar = ((double)polar_idx + 0.5) * polar_step;
        cos_polar = ldg_math_cos_get(polar);
        sin_polar = ldg_math_sin_get(polar);
        for (azim_idx = 0; azim_idx < LDG_MATH_PHYS_ATMOSPHERE_HEMI_AZIM_CUNT; azim_idx++)
        {
            azim = ((double)azim_idx + 0.5) * azim_step;
            local_x = sin_polar * ldg_math_cos_get(azim);
            local_y = sin_polar * ldg_math_sin_get(azim);
            dir_x = cos_polar * normal_x + local_x * tangent_a_x + local_y * tangent_b_x;
            dir_z = cos_polar * normal_cos + local_y * tangent_b_z;
            if (dir_z > 1.0) { dir_z = 1.0; }

            if (dir_z < 0.0 - 1.0) { dir_z = 0.0 - 1.0; }

            nu = dir_x * sin_s + dir_z * sun_cos;
            if (nu > 1.0) { nu = 1.0; }

            if (nu < 0.0 - 1.0) { nu = 0.0 - 1.0; }

            weight = cos_polar * sin_polar * polar_step * azim_step;
            for (bin = 0; bin < table->hdr->bin_cunt; bin++) { out[bin] += ldg_math_phys_atmosphere_full_radiance_bin_get(table, altitude, dir_z, sun_cos, nu, bin) * weight; }
        }
    }
    return LDG_ERR_AOK;
}

uint32_t ldg_math_phys_atmosphere_vol_sample_get(const ldg_math_phys_atmosphere_table_t *table, const ldg_math_phys_atmosphere_vol_desc_t *desc, double *out_transmittance, double *out_inscatter)
{
    double sun_dir[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double up_dir[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double pos[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double ray[LDG_MATH_PHYS_DIM] = LDG_ARR_ZERO_INIT;
    double sun_norm = 0.0;
    double up_norm = 0.0;
    double cam_altitude = 0.0;
    double pos_altitude = 0.0;
    double ray_norm = 0.0;
    double mu = 0.0;
    double mu_s = 0.0;
    double nu = 0.0;
    double mie_extinction = 0.0;
    double fog_extinction = 0.0;
    double seg_rayleigh = 0.0;
    double seg_mie = 0.0;
    double seg_fog = 0.0;
    double seg_ozone = 0.0;
    double trans_val = 0.0;
    double inscatter_val = 0.0;
    uint64_t idx_x = 0;
    uint64_t idx_y = 0;
    uint64_t idx_z = 0;
    uint64_t cell = 0;
    uint64_t bin = 0;
    uint32_t axis = 0;

    if (LDG_UNLIKELY(!table || !desc || !out_transmittance || !out_inscatter)) { return LDG_ERR_FUNC_ARG_NULL; }

    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        sun_norm += desc->sun[axis] * desc->sun[axis];
        up_norm += desc->up[axis] * desc->up[axis];
    }
    if (LDG_UNLIKELY(sun_norm <= 0.0 || up_norm <= 0.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    sun_norm = ldg_math_sqrt_get(sun_norm);
    up_norm = ldg_math_sqrt_get(up_norm);
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        sun_dir[axis] = desc->sun[axis] / sun_norm;
        up_dir[axis] = desc->up[axis] / up_norm;
    }
    mie_extinction = table->hdr->mie_scatter + table->hdr->mie_absorb;
    fog_extinction = table->hdr->fog_scatter + table->hdr->fog_absorb;
    cam_altitude = desc->ref_altitude;
    mu_s = 0.0;
    for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
    {
        cam_altitude += desc->cam[axis] * up_dir[axis];
        mu_s += sun_dir[axis] * up_dir[axis];
    }
    if (cam_altitude < 0.0) { cam_altitude = 0.0; }

    if (cam_altitude > table->hdr->thickness) { cam_altitude = table->hdr->thickness; }

    if (mu_s > 1.0) { mu_s = 1.0; }

    if (mu_s < 0.0 - 1.0) { mu_s = 0.0 - 1.0; }

    for (idx_z = 0; idx_z < desc->cunt_z; idx_z++)
    {
        for (idx_y = 0; idx_y < desc->cunt_y; idx_y++)
        {
            for (idx_x = 0; idx_x < desc->cunt_x; idx_x++)
            {
                pos[0] = desc->orig[0] + (double)idx_x * desc->spacing;
                pos[1] = desc->orig[1] + (double)idx_y * desc->spacing;
                pos[2] = desc->orig[2] + (double)idx_z * desc->spacing;
                cell = ((idx_z * desc->cunt_y + idx_y) * desc->cunt_x + idx_x) * table->hdr->bin_cunt;
                ray_norm = 0.0;
                for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
                {
                    ray[axis] = pos[axis] - desc->cam[axis];
                    ray_norm += ray[axis] * ray[axis];
                }
                if (ray_norm <= 0.0)
                {
                    for (bin = 0; bin < table->hdr->bin_cunt; bin++)
                    {
                        out_transmittance[cell + bin] = 1.0;
                        out_inscatter[cell + bin] = 0.0;
                    }
                    continue;
                }

                ray_norm = ldg_math_sqrt_get(ray_norm);
                mu = 0.0;
                nu = 0.0;
                pos_altitude = desc->ref_altitude;
                for (axis = 0; axis < LDG_MATH_PHYS_DIM; axis++)
                {
                    mu += (ray[axis] / ray_norm) * up_dir[axis];
                    nu += (ray[axis] / ray_norm) * sun_dir[axis];
                    pos_altitude += pos[axis] * up_dir[axis];
                }
                if (mu > 1.0) { mu = 1.0; }

                if (mu < 0.0 - 1.0) { mu = 0.0 - 1.0; }

                if (nu > 1.0) { nu = 1.0; }

                if (nu < 0.0 - 1.0) { nu = 0.0 - 1.0; }

                if (pos_altitude < 0.0) { pos_altitude = 0.0; }

                if (pos_altitude > table->hdr->thickness) { pos_altitude = table->hdr->thickness; }

                seg_rayleigh = ldg_math_phys_atmosphere_segment_depth_get(table->hdr->rayleigh_scale_height, cam_altitude, pos_altitude, ray_norm);
                seg_mie = ldg_math_phys_atmosphere_segment_depth_get(table->hdr->mie_scale_height, cam_altitude, pos_altitude, ray_norm);
                seg_fog = ldg_math_phys_atmosphere_segment_depth_get(table->hdr->fog_scale_height, cam_altitude, pos_altitude, ray_norm);
                seg_ozone = ldg_math_phys_atmosphere_ozone_segment_depth_get(table->hdr, cam_altitude, pos_altitude, ray_norm);
                for (bin = 0; bin < table->hdr->bin_cunt; bin++)
                {
                    trans_val = ldg_math_exp_get(0.0 - (table->rayleigh[bin] * seg_rayleigh + mie_extinction * seg_mie + fog_extinction * seg_fog + table->ozone[bin] * seg_ozone));
                    inscatter_val = ldg_math_phys_atmosphere_inscatter_bin_get(table, cam_altitude, mu, mu_s, nu, bin) - trans_val * ldg_math_phys_atmosphere_inscatter_bin_get(table, pos_altitude, mu, mu_s, nu, bin);
                    if (inscatter_val < 0.0) { inscatter_val = 0.0; }

                    out_transmittance[cell + bin] = trans_val;
                    out_inscatter[cell + bin] = inscatter_val;
                }
            }
        }
    }
    return LDG_ERR_AOK;
}
