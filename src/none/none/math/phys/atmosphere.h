#ifndef LDG_MATH_PHYS_ATMOSPHERE_H
#define LDG_MATH_PHYS_ATMOSPHERE_H

#include <stdint.h>

#include <dangling/math/phys.h>

#define LDG_MATH_PHYS_ATMOSPHERE_TRANSMITTANCE_STEP_CUNT 64
#define LDG_MATH_PHYS_ATMOSPHERE_SCATTER_STEP_CUNT 32
#define LDG_MATH_PHYS_ATMOSPHERE_FOG_LIMIT_SCALE 20.0
#define LDG_MATH_PHYS_ATMOSPHERE_GATHER_POLAR_CUNT 8
#define LDG_MATH_PHYS_ATMOSPHERE_GATHER_AZIM_CUNT 16
#define LDG_MATH_PHYS_ATMOSPHERE_HEMI_POLAR_CUNT 8
#define LDG_MATH_PHYS_ATMOSPHERE_HEMI_AZIM_CUNT 16

typedef struct ldg_math_phys_atmosphere_hdr
{
    double planet_radius;
    double thickness;
    double rayleigh_scale_height;
    double mie_scale_height;
    double mie_scatter;
    double mie_absorb;
    double mie_anisotropy;
    double fog_scale_height;
    double fog_scatter;
    double fog_absorb;
    double fog_anisotropy;
    double ozone_center_altitude;
    double ozone_half_width;
    uint64_t bin_cunt;
    uint64_t order_cunt;
    uint64_t altitude_cunt;
    uint64_t view_cunt;
    uint64_t sun_cunt;
    uint64_t view_sun_cunt;
    uint64_t solved;
} ldg_math_phys_atmosphere_hdr_t;

typedef struct ldg_math_phys_atmosphere_table
{
    const ldg_math_phys_atmosphere_hdr_t *hdr;
    const double *rayleigh;
    const double *ozone;
    const double *sun_irradiance;
    const double *albedo;
    const double *transmittance;
    const double *delta_rayleigh;
    const double *delta_mie;
    const double *delta_fog;
    const double *multi;
    const double *ground_irradiance;
} ldg_math_phys_atmosphere_table_t;

uint32_t ldg_math_phys_atmosphere_transmittance_fill(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *rayleigh, const double *ozone, double *transmittance);
uint32_t ldg_math_phys_atmosphere_single_fill(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *rayleigh, const double *ozone, const double *sun_irradiance, const double *transmittance, double *delta_rayleigh, double *delta_mie, double *delta_fog);
uint32_t ldg_math_phys_atmosphere_irradiance_fill(const ldg_math_phys_atmosphere_table_t *table, const double *delta_radiance, uint64_t order, double *delta_irradiance);
uint32_t ldg_math_phys_atmosphere_gather_fill(const ldg_math_phys_atmosphere_table_t *table, const double *delta_radiance, const double *delta_irradiance, double *gather);
uint32_t ldg_math_phys_atmosphere_order_fill(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *rayleigh, const double *ozone, const double *gather, double *delta_radiance);
typedef struct ldg_math_phys_atmosphere_compute_info
{
    const ldg_math_phys_atmosphere_hdr_t *hdr;
    double *rayleigh;
    double *ozone;
    double *sun_irradiance;
    double *albedo;
    double *transmittance;
    double *delta_rayleigh;
    double *delta_mie;
    double *delta_fog;
    double *multi;
    double *ground;
    double *delta_irradiance;
    double *gather;
    double *ping;
    double *pong;
    uint64_t grid_total;
    uint64_t trans_total;
    uint64_t ground_total;
} ldg_math_phys_atmosphere_compute_info_t;

uint32_t ldg_math_phys_atmosphere_compute_info_get(const ldg_math_phys_ctx_t *ctx, ldg_math_phys_atmosphere_compute_info_t *out);

uint32_t ldg_math_phys_atmosphere_transmittance_sample_get(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *transmittance, double altitude, double zenith_cos, double *out);
uint32_t ldg_math_phys_atmosphere_extinction_sample_get(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *rayleigh, const double *ozone, double altitude, double *out);
uint32_t ldg_math_phys_atmosphere_medium_sample_get(const ldg_math_phys_atmosphere_hdr_t *hdr, const double *rayleigh, const double *ozone, uint32_t medium, double altitude, double *out_scatter, double *out_absorb, double *out_anisotropy);
uint32_t ldg_math_phys_atmosphere_radiance_sample_get(const ldg_math_phys_atmosphere_table_t *table, double altitude, double view_cos, double sun_cos, double view_sun_cos, double *out);
uint32_t ldg_math_phys_atmosphere_irradiance_sample_get(const ldg_math_phys_atmosphere_table_t *table, double altitude, double sun_cos, double normal_cos, double normal_sun_cos, double *out);
uint32_t ldg_math_phys_atmosphere_vol_sample_get(const ldg_math_phys_atmosphere_table_t *table, const ldg_math_phys_atmosphere_vol_desc_t *desc, double *out_transmittance, double *out_inscatter);

#endif
