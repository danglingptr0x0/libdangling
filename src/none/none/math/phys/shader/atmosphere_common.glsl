#ifndef LDG_ATMOSPHERE_COMMON
#define LDG_ATMOSPHERE_COMMON

#include "fp64_math.glsl"

const double LDG_ATMOSPHERE_PI = 3.14159265358979323846lf;
const double LDG_ATMOSPHERE_TAU = 6.28318530717958647692lf;
const double LDG_ATMOSPHERE_EPS = 1.0e-12lf;

double atmosphere_density(double altitude, double scale)
{
    double clamped = altitude;
    if (clamped < 0.0lf) { clamped = 0.0lf; }

    return ldg_exp((0.0lf - clamped) / scale);
}

double fog_density(double altitude, double scale)
{
    if (scale <= 0.0lf) { return 0.0lf; }

    return atmosphere_density(altitude, scale);
}

const double LDG_ATMOSPHERE_FOG_LIMIT_SCALE = 20.0lf;

double segment_depth(double altitude_lo, double altitude_hi, double len, double scale)
{
    double lo = altitude_lo;
    double hi = altitude_hi;
    double delta = 0.0lf;
    if (scale <= 0.0lf || len <= 0.0lf) { return 0.0lf; }

    if (lo < 0.0lf) { lo = 0.0lf; }

    if (hi < 0.0lf) { hi = 0.0lf; }

    delta = hi - lo;
    if (abs(delta) < 1.0e-9lf * scale) { return len * ldg_exp((0.0lf - 0.5lf * (lo + hi)) / scale); }

    return len * (scale / delta) * (ldg_exp((0.0lf - lo) / scale) - ldg_exp((0.0lf - hi) / scale));
}

double ozone_density(double altitude, double center, double half_width)
{
    double clamped = altitude;
    double offset = 0.0lf;
    if (half_width <= 0.0lf) { return 0.0lf; }

    if (clamped < 0.0lf) { clamped = 0.0lf; }

    offset = abs(clamped - center);
    if (offset >= half_width) { return 0.0lf; }

    return 1.0lf - offset / half_width;
}

double ozone_wedge_depth(double altitude_lo, double altitude_hi, double center, double half_width)
{
    double lo = altitude_lo;
    double hi = altitude_hi;
    double base_lo = center - half_width;
    double base_hi = center + half_width;
    double depth = 0.0lf;
    if (lo < base_lo) { lo = base_lo; }

    if (hi > base_hi) { hi = base_hi; }

    if (hi <= lo) { return 0.0lf; }

    if (lo < center && hi > center)
    {
        depth = 0.5lf * (ozone_density(lo, center, half_width) + 1.0lf) * (center - lo);
        return depth + 0.5lf * (1.0lf + ozone_density(hi, center, half_width)) * (hi - center);
    }

    return 0.5lf * (ozone_density(lo, center, half_width) + ozone_density(hi, center, half_width)) * (hi - lo);
}

double ozone_segment_depth(double altitude_lo, double altitude_hi, double len, double center, double half_width)
{
    double lo = altitude_lo;
    double hi = altitude_hi;
    double swap = 0.0lf;
    double delta = 0.0lf;
    if (half_width <= 0.0lf || len <= 0.0lf) { return 0.0lf; }

    if (lo < 0.0lf) { lo = 0.0lf; }

    if (hi < 0.0lf) { hi = 0.0lf; }

    delta = hi - lo;
    if (abs(delta) < 1.0e-9lf * half_width) { return len * ozone_density(0.5lf * (lo + hi), center, half_width); }

    if (lo > hi)
    {
        swap = lo;
        lo = hi;
        hi = swap;
    }

    return (len / (hi - lo)) * ozone_wedge_depth(lo, hi, center, half_width);
}

double fog_span(double planet_radius, double fog_scale, double radius, double mu, double ray_len, out double start)
{
    double limit_radius = 0.0lf;
    double disc = 0.0lf;
    double root = 0.0lf;
    double span_lo = 0.0lf;
    double span_hi = 0.0lf;
    start = 0.0lf;
    if (fog_scale <= 0.0lf) { return 0.0lf; }

    limit_radius = planet_radius + fog_scale * LDG_ATMOSPHERE_FOG_LIMIT_SCALE;
    disc = radius * radius * (mu * mu - 1.0lf) + limit_radius * limit_radius;
    if (disc <= 0.0lf) { return 0.0lf; }

    root = sqrt(disc);
    span_lo = 0.0lf - radius * mu - root;
    span_hi = 0.0lf - radius * mu + root;
    if (span_lo < 0.0lf) { span_lo = 0.0lf; }

    if (span_hi > ray_len) { span_hi = ray_len; }

    if (span_hi <= span_lo) { return 0.0lf; }

    start = span_lo;
    return span_hi - span_lo;
}

double atmosphere_ray_len(double planet_radius, double thickness, double radius, double mu, out uint ground_hit)
{
    double top = planet_radius + thickness;
    double disc_ground = radius * radius * (mu * mu - 1.0lf) + planet_radius * planet_radius;
    double disc_top = radius * radius * (mu * mu - 1.0lf) + top * top;
    ground_hit = 0u;
    if (mu < 0.0lf && disc_ground >= 0.0lf)
    {
        ground_hit = 1u;
        return 0.0lf - radius * mu - sqrt(disc_ground);
    }

    if (disc_top < 0.0lf) { disc_top = 0.0lf; }

    return 0.0lf - radius * mu + sqrt(disc_top);
}

double atmosphere_horizon(double planet_radius, double altitude)
{
    double clamped = altitude;
    double radius = 0.0lf;
    double ratio = 0.0lf;
    double horizon = 0.0lf;
    if (clamped < 0.0lf) { clamped = 0.0lf; }

    radius = planet_radius + clamped;
    ratio = planet_radius / radius;
    horizon = 1.0lf - ratio * ratio;
    if (horizon < 0.0lf) { horizon = 0.0lf; }

    return 0.0lf - sqrt(horizon);
}

void atmosphere_axis(double val, double lo, double span, uint cunt, out uint idx, out double frac)
{
    double coord = (val - lo) / span * (double(cunt) - 1.0lf);
    if (coord < 0.0lf) { coord = 0.0lf; }

    if (coord > double(cunt) - 1.0lf) { coord = double(cunt) - 1.0lf; }

    idx = uint(coord);
    if (idx >= cunt - 1u) { idx = cunt - 2u; }

    frac = coord - double(idx);
}

double atmosphere_rayleigh_phase(double nu)
{
    return (3.0lf / (16.0lf * LDG_ATMOSPHERE_PI)) * (1.0lf + nu * nu);
}

double atmosphere_mie_phase(double anisotropy, double nu)
{
    double square = anisotropy * anisotropy;
    double denom = 1.0lf + square - 2.0lf * anisotropy * nu;
    if (denom < LDG_ATMOSPHERE_EPS) { denom = LDG_ATMOSPHERE_EPS; }

    return (3.0lf / (8.0lf * LDG_ATMOSPHERE_PI)) * ((1.0lf - square) * (1.0lf + nu * nu)) / ((2.0lf + square) * denom * sqrt(denom));
}

#endif
