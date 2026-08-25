#ifndef LDG_MATH_PHYS_H
#define LDG_MATH_PHYS_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/arith.h>
#include <dangling/ecs/ecs.h>
#include <dangling/math/math.h>
#include <dangling/math/quaternion.h>

#define LDG_MATH_PHYS_DIM 3
#define LDG_MATH_PHYS_REGION_WORLD UINT32_MAX
#define LDG_MATH_PHYS_WATER_OPTIC_BIN_MAX 16
#define LDG_MATH_PHYS_ATMOSPHERE_BIN_MAX 64
#define LDG_MATH_PHYS_ATMOSPHERE_MEDIUM_RAYLEIGH 0
#define LDG_MATH_PHYS_ATMOSPHERE_MEDIUM_MIE 1
#define LDG_MATH_PHYS_ATMOSPHERE_MEDIUM_FOG 2
#define LDG_MATH_PHYS_ATMOSPHERE_MEDIUM_OZONE 3
#define LDG_MATH_PHYS_ATMOSPHERE_MEDIUM_CUNT 4
#define LDG_MATH_PHYS_EM_DRIVE_CAP 4
#define LDG_MATH_PHYS_EM_PROBE_CAP 8
#define LDG_MATH_PHYS_EM_CONDUCTOR_CAP 16
#define LDG_MATH_PHYS_EM_PORT_CAP 4
#define LDG_MATH_PHYS_EM_PORT_MODE_CAP 2
#define LDG_MATH_PHYS_EM_PORT_MODE_TEM 0
#define LDG_MATH_PHYS_EM_PORT_MODE_TE 1
#define LDG_MATH_PHYS_EM_PORT_MODE_TM 2
#define LDG_MATH_PHYS_EM_PORT_TAP_DEPTH_DEFAULT 4096
#define LDG_MATH_PHYS_EM_DRIVE_CONT 0
#define LDG_MATH_PHYS_EM_DRIVE_PULSE 1
#define LDG_MATH_PHYS_SIG_TONE_CAP 8

typedef enum ldg_math_phys_sig_kind
{
    LDG_MATH_PHYS_SIG_SINE = 0x0,
    LDG_MATH_PHYS_SIG_GAUSS_PULSE,
    LDG_MATH_PHYS_SIG_GAUSS_DERIVATIVE,
    LDG_MATH_PHYS_SIG_RICKER,
    LDG_MATH_PHYS_SIG_PRBS,
    LDG_MATH_PHYS_SIG_MULTITONE,
    LDG_MATH_PHYS_SIG_CHIRP,
    LDG_MATH_PHYS_SIG_SAMPLED,
    LDG_MATH_PHYS_SIG_KIND_CUNT
} ldg_math_phys_sig_kind_t;

typedef struct ldg_math_phys_sig
{
    double amplitude;
    double freq;
    double freq_end;
    double sweep_time;
    double edge_fraction;
    double tone_freq[LDG_MATH_PHYS_SIG_TONE_CAP];
    double tone_amplitude[LDG_MATH_PHYS_SIG_TONE_CAP];
    double tone_phase[LDG_MATH_PHYS_SIG_TONE_CAP];
    const double *series;
    uint64_t series_cunt;
    uint64_t prbs_seed;
    uint64_t prbs_state;
    uint64_t prbs_bit_idx;
    uint64_t prbs_bit_val;
    uint64_t prbs_prev_bit_val;
    uint32_t prbs_order;
    uint32_t tone_cunt;
    uint32_t kind;
    uint8_t pudding[4];
} ldg_math_phys_sig_t;

typedef enum ldg_math_phys_field_boundary
{
    LDG_MATH_PHYS_FIELD_BOUNDARY_REFLECTIVE = 0x0,
    LDG_MATH_PHYS_FIELD_BOUNDARY_ABSORBING = 0x1
} ldg_math_phys_field_boundary_t;

typedef enum ldg_math_phys_tier
{
    LDG_MATH_PHYS_TIER_PARTICLE = 0x0,
    LDG_MATH_PHYS_TIER_FIELD,
    LDG_MATH_PHYS_TIER_CUNT
} ldg_math_phys_tier_t;

typedef enum ldg_math_phys_field_kind
{
    LDG_MATH_PHYS_FIELD_KIND_SCALAR = 0x0,
    LDG_MATH_PHYS_FIELD_KIND_EM,
    LDG_MATH_PHYS_FIELD_KIND_ACOUSTIC,
    LDG_MATH_PHYS_FIELD_KIND_THERMO,
    LDG_MATH_PHYS_FIELD_KIND_WATER,
    LDG_MATH_PHYS_FIELD_KIND_CUNT
} ldg_math_phys_field_kind_t;

typedef enum ldg_math_phys_quantity
{
    LDG_MATH_PHYS_QUANTITY_MASS = 0x0,
    LDG_MATH_PHYS_QUANTITY_ENERGY,
    LDG_MATH_PHYS_QUANTITY_MOMENTUM_X,
    LDG_MATH_PHYS_QUANTITY_MOMENTUM_Y,
    LDG_MATH_PHYS_QUANTITY_MOMENTUM_Z,
    LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_X,
    LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_Y,
    LDG_MATH_PHYS_QUANTITY_ANGULAR_MOMENTUM_Z,
    LDG_MATH_PHYS_QUANTITY_CHARGE,
    LDG_MATH_PHYS_QUANTITY_CUNT
} ldg_math_phys_quantity_t;

typedef enum ldg_math_phys_collider_shape
{
    LDG_MATH_PHYS_COLLIDER_PLANE = 0x0,
    LDG_MATH_PHYS_COLLIDER_SPHERE,
    LDG_MATH_PHYS_COLLIDER_BOX,
    LDG_MATH_PHYS_COLLIDER_CAPSULE,
    LDG_MATH_PHYS_COLLIDER_CUNT
} ldg_math_phys_collider_shape_t;

#define LDG_MATH_PHYS_PARTICLE_NAME_MAX 32

typedef struct ldg_math_phys_particle
{
    double mass;
    double charge;
    double spin;
    char name[LDG_MATH_PHYS_PARTICLE_NAME_MAX];
} ldg_math_phys_particle_t;

typedef enum ldg_math_phys_particle_id
{
    LDG_MATH_PHYS_PARTICLE_PHOTON = 0x0,
    LDG_MATH_PHYS_PARTICLE_GLUON,
    LDG_MATH_PHYS_PARTICLE_W,
    LDG_MATH_PHYS_PARTICLE_Z,
    LDG_MATH_PHYS_PARTICLE_HIGGS,
    LDG_MATH_PHYS_PARTICLE_UP,
    LDG_MATH_PHYS_PARTICLE_CHARM,
    LDG_MATH_PHYS_PARTICLE_TOP,
    LDG_MATH_PHYS_PARTICLE_DOWN,
    LDG_MATH_PHYS_PARTICLE_STRANGE,
    LDG_MATH_PHYS_PARTICLE_BOTTOM,
    LDG_MATH_PHYS_PARTICLE_ELECTRON,
    LDG_MATH_PHYS_PARTICLE_MUON,
    LDG_MATH_PHYS_PARTICLE_TAU,
    LDG_MATH_PHYS_PARTICLE_ELECTRON_NEUTRINO,
    LDG_MATH_PHYS_PARTICLE_MUON_NEUTRINO,
    LDG_MATH_PHYS_PARTICLE_TAU_NEUTRINO,
    LDG_MATH_PHYS_PARTICLE_CUNT
} ldg_math_phys_particle_id_t;

typedef struct ldg_math_phys_conf
{
    ldg_uint128_t tick_budget;
    double pacing;
    void *pool;
    uint32_t region_cap;
    uint32_t constraint_cap;
    uint32_t collider_cap;
    uint32_t aggregate_cap;
    uint32_t history_depth;
    uint32_t comp_id_base;
    uint32_t taxon_id_base;
    uint8_t pudding[4];
} ldg_math_phys_conf_t;

typedef struct ldg_math_phys_region_desc
{
    double extent[LDG_MATH_PHYS_DIM];
    double spacing;
    double wave_speed;
    double damping;
    double pml_freq;
    double optic_wave_err_max;
    double optic_ray_err_max;
    double thermo_ambient;
    double water_flip;
    int64_t mass_scale_exponent;
    int64_t energy_scale_exponent;
    int64_t momentum_scale_exponent;
    int64_t angular_momentum_scale_exponent;
    uint32_t tier;
    uint32_t boundary;
    uint32_t field_kind;
    uint32_t subcycle_cunt;
    uint32_t pml_depth;
    uint32_t em_history_depth;
    uint32_t mtl_cap;
    uint32_t sheet_face_cap;
    uint32_t acoustic_history_depth;
    uint32_t wall_face_cap;
    uint32_t snapshot_stride;
    uint32_t energy_stride;
    uint32_t optic_bin_cunt;
    uint32_t optic_ord_polar_cunt;
    uint32_t optic_ord_azim_cunt;
    uint32_t water_iter_cunt;
    uint32_t conductor_cap;
    uint32_t port_cap;
    uint32_t port_tap_depth;
    uint8_t step_skip;
    uint8_t pudding[3];
} ldg_math_phys_region_desc_t;

typedef struct ldg_math_phys_sheet_desc
{
    double sigma;
    double thickness;
    double mu_r;
    double residue[4];
    double pole[4];
    uint32_t pole_cunt;
    uint8_t pudding[4];
} ldg_math_phys_sheet_desc_t;

typedef struct ldg_math_phys_mtl_desc
{
    double epsilon_r;
    double mu_r;
    double sigma;
} ldg_math_phys_mtl_desc_t;

typedef struct ldg_math_phys_acoustic_mtl_desc
{
    double density;
    double speed;
    double loss;
} ldg_math_phys_acoustic_mtl_desc_t;

typedef struct ldg_math_phys_thermo_mtl_desc
{
    double conductivity;
    double density;
    double heat_cap;
    double emissivity;
    double absorb;
    double molar_mass;
    double convection;
} ldg_math_phys_thermo_mtl_desc_t;

typedef struct ldg_math_phys_wall_desc
{
    double z_inf;
    double residue[4];
    double pole[4];
    uint32_t pole_cunt;
    uint8_t pudding[4];
} ldg_math_phys_wall_desc_t;

typedef struct ldg_math_phys_atmosphere_desc
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
    const double *rayleigh_scatter;
    const double *ozone_absorb;
    const double *sun_irradiance;
    const double *ground_albedo;
    uint32_t bin_cunt;
    uint32_t order_cunt;
    uint32_t altitude_cunt;
    uint32_t view_cunt;
    uint32_t sun_cunt;
    uint32_t view_sun_cunt;
} ldg_math_phys_atmosphere_desc_t;

typedef struct ldg_math_phys_atmosphere_vol_desc
{
    double orig[LDG_MATH_PHYS_DIM];
    double spacing;
    double cam[LDG_MATH_PHYS_DIM];
    double sun[LDG_MATH_PHYS_DIM];
    double up[LDG_MATH_PHYS_DIM];
    double ref_altitude;
    uint64_t cunt_x;
    uint64_t cunt_y;
    uint64_t cunt_z;
} ldg_math_phys_atmosphere_vol_desc_t;

typedef struct ldg_math_phys_plane_wave_desc
{
    double direction[LDG_MATH_PHYS_DIM];
    double polarization[LDG_MATH_PHYS_DIM];
} ldg_math_phys_plane_wave_desc_t;

typedef struct ldg_math_phys_em_port_desc
{
    double lo[2];
    double hi[2];
    double plane_pos;
    double z_ref;
    uint32_t normal_axis;
    uint32_t mode_cap;
    uint8_t is_forward;
    uint8_t pudding[7];
} ldg_math_phys_em_port_desc_t;


typedef struct ldg_math_phys_slice_desc
{
    double orig[LDG_MATH_PHYS_DIM];
    double step_a;
    double step_b;
    uint64_t cunt_a;
    uint64_t cunt_b;
    uint32_t axis_a;
    uint32_t axis_b;
} ldg_math_phys_slice_desc_t;

typedef struct ldg_math_phys_aggregate_desc
{
    char name[LDG_MATH_PHYS_PARTICLE_NAME_MAX];
} ldg_math_phys_aggregate_desc_t;

typedef struct ldg_math_phys_body_desc
{
    double pos[LDG_MATH_PHYS_DIM];
    double velocity[LDG_MATH_PHYS_DIM];
    double orientation[LDG_MATH_QUATERNION_DIM];
    double spin[LDG_MATH_PHYS_DIM];
    double mass;
    double inertia;
    int64_t charge;
    int64_t lepton;
    int64_t baryon;
} ldg_math_phys_body_desc_t;

typedef struct ldg_math_phys_body_state
{
    double pos[LDG_MATH_PHYS_DIM];
    double velocity[LDG_MATH_PHYS_DIM];
    double momentum[LDG_MATH_PHYS_DIM];
    double angular_momentum[LDG_MATH_PHYS_DIM];
    double orientation[LDG_MATH_QUATERNION_DIM];
    double spin[LDG_MATH_PHYS_DIM];
    double mass;
    double energy;
    double inertia;
    int64_t charge;
    uint32_t tier;
    uint32_t region;
} ldg_math_phys_body_state_t;

typedef struct ldg_math_phys_collider_desc
{
    double normal[LDG_MATH_PHYS_DIM];
    double offset;
    double center[LDG_MATH_PHYS_DIM];
    double half[LDG_MATH_PHYS_DIM];
    double axes[9];
    double point_a[LDG_MATH_PHYS_DIM];
    double point_b[LDG_MATH_PHYS_DIM];
    double radius;
    double restitution;
    uint32_t shape;
    uint8_t pudding[4];
} ldg_math_phys_collider_desc_t;

typedef enum ldg_math_phys_sweep_shape
{
    LDG_MATH_PHYS_SWEEP_RAY = 0x0,
    LDG_MATH_PHYS_SWEEP_SPHERE,
    LDG_MATH_PHYS_SWEEP_CAPSULE,
    LDG_MATH_PHYS_SWEEP_CUNT
} ldg_math_phys_sweep_shape_t;

typedef struct ldg_math_phys_sweep_desc
{
    double origin[LDG_MATH_PHYS_DIM];
    double disp[LDG_MATH_PHYS_DIM];
    double point_b[LDG_MATH_PHYS_DIM];
    double radius;
    uint32_t shape;
    uint8_t pudding[4];
} ldg_math_phys_sweep_desc_t;

typedef struct ldg_math_phys_sweep_hit
{
    double normal[LDG_MATH_PHYS_DIM];
    double point[LDG_MATH_PHYS_DIM];
    double toi;
    uint64_t collider;
} ldg_math_phys_sweep_hit_t;

#define LDG_MATH_PHYS_COMP_MASS 0
#define LDG_MATH_PHYS_COMP_ENERGY 1
#define LDG_MATH_PHYS_COMP_MOMENTUM 2
#define LDG_MATH_PHYS_COMP_ANGULAR_MOMENTUM 3
#define LDG_MATH_PHYS_COMP_CHARGE 4
#define LDG_MATH_PHYS_COMP_POS 5
#define LDG_MATH_PHYS_COMP_KIND 6
#define LDG_MATH_PHYS_COMP_REGION 7
#define LDG_MATH_PHYS_COMP_ORIENTATION 8
#define LDG_MATH_PHYS_COMP_SPIN 9
#define LDG_MATH_PHYS_COMP_INERTIA 10
#define LDG_MATH_PHYS_COMP_TAG 11
#define LDG_MATH_PHYS_COMP_CUNT 12

#define LDG_MATH_PHYS_MASS_FRACTION 112
#define LDG_MATH_PHYS_ENERGY_FRACTION 96
#define LDG_MATH_PHYS_MOMENTUM_FRACTION 96
#define LDG_MATH_PHYS_ANGULAR_MOMENTUM_FRACTION 96
#define LDG_MATH_PHYS_POS_FRACTION 64

#define LDG_MATH_PHYS_CONSERVED_EXPONENT_MAX 16384
#define LDG_MATH_PHYS_CONSERVED_SPLIT_EXP 128
#define LDG_MATH_PHYS_CONSERVED_SPLIT_EXP_NEG (-LDG_MATH_PHYS_CONSERVED_SPLIT_EXP)
#define LDG_MATH_PHYS_AGGREGATE_ID_BASE UINT32_C(0x80000000)
#define LDG_MATH_PHYS_TIME_FRACTION 64
#define LDG_MATH_PHYS_TICK_BUDGET_DEFAULT_SHIFT 58
#define LDG_MATH_PHYS_FIELD_DIM_MIN 3
#define LDG_MATH_PHYS_FRAGMENT_MAX 6
#define LDG_MATH_PHYS_PIN_CATEGORY UINT32_C(0x80000000)

#define LDG_MATH_PHYS_ACCEL_GRAVITY_EARTH 9.80665
#define LDG_MATH_PHYS_GRAVITATION_CONST 6.67430e-11
#define LDG_MATH_PHYS_VACUUM_LIGHT_SPEED 299792458.0
#define LDG_MATH_PHYS_VACUUM_PERMITTIVITY 8.8541878188e-12
#define LDG_MATH_PHYS_VACUUM_PERMEABILITY 1.25663706127e-6
#define LDG_MATH_PHYS_ELEM_CHARGE 1.602176634e-19
#define LDG_MATH_PHYS_BOLTZMANN 1.380649e-23
#define LDG_MATH_PHYS_EV_TO_JOULE 1.602176634e-19
#define LDG_MATH_PHYS_MEV_TO_KG (1.0e6 * LDG_MATH_PHYS_EV_TO_JOULE / (LDG_MATH_PHYS_VACUUM_LIGHT_SPEED * LDG_MATH_PHYS_VACUUM_LIGHT_SPEED))

#define LDG_MATH_PHYS_PARTICLE_MASS_W (80369.2 * LDG_MATH_PHYS_MEV_TO_KG)
#define LDG_MATH_PHYS_PARTICLE_MASS_Z (91188.0 * LDG_MATH_PHYS_MEV_TO_KG)
#define LDG_MATH_PHYS_PARTICLE_MASS_HIGGS (125200.0 * LDG_MATH_PHYS_MEV_TO_KG)
#define LDG_MATH_PHYS_PARTICLE_MASS_UP (2.16 * LDG_MATH_PHYS_MEV_TO_KG)
#define LDG_MATH_PHYS_PARTICLE_MASS_CHARM (1273.0 * LDG_MATH_PHYS_MEV_TO_KG)
#define LDG_MATH_PHYS_PARTICLE_MASS_TOP (172570.0 * LDG_MATH_PHYS_MEV_TO_KG)
#define LDG_MATH_PHYS_PARTICLE_MASS_DOWN (4.70 * LDG_MATH_PHYS_MEV_TO_KG)
#define LDG_MATH_PHYS_PARTICLE_MASS_STRANGE (93.5 * LDG_MATH_PHYS_MEV_TO_KG)
#define LDG_MATH_PHYS_PARTICLE_MASS_BOTTOM (4183.0 * LDG_MATH_PHYS_MEV_TO_KG)
#define LDG_MATH_PHYS_PARTICLE_MASS_ELECTRON (0.51099895000 * LDG_MATH_PHYS_MEV_TO_KG)
#define LDG_MATH_PHYS_PARTICLE_MASS_MUON (105.6583755 * LDG_MATH_PHYS_MEV_TO_KG)
#define LDG_MATH_PHYS_PARTICLE_MASS_TAU (1776.93 * LDG_MATH_PHYS_MEV_TO_KG)

#define LDG_MATH_PHYS_MTL_VACUUM 0
#define LDG_MATH_PHYS_MTL_CAP_DEFAULT 16
#define LDG_MATH_PHYS_SHEET_POLE_MAX 4
#define LDG_MATH_PHYS_SHEET_CAP 8
#define LDG_MATH_PHYS_WALL_POLE_MAX 4
#define LDG_MATH_PHYS_WALL_CAP 8
#define LDG_MATH_PHYS_PML_DEPTH_DEFAULT 10
#define LDG_MATH_PHYS_SUBCYCLE_MAX 65536
#define LDG_MATH_PHYS_AIR_SOUND_SPEED 343.2
#define LDG_MATH_PHYS_AIR_REF_TEMP 293.15

#define LDG_MATH_PHYS_REGION_CAP_DEFAULT 8
#define LDG_MATH_PHYS_CONSTRAINT_CAP_DEFAULT 1024
#define LDG_MATH_PHYS_COLLIDER_CAP_DEFAULT 64
#define LDG_MATH_PHYS_AGGREGATE_CAP_DEFAULT 32
#define LDG_MATH_PHYS_HISTORY_DEPTH_DEFAULT 16
#define LDG_MATH_PHYS_COMP_ID_BASE_DEFAULT 32
#define LDG_MATH_PHYS_TAXON_ID_BASE_DEFAULT 8

typedef struct ldg_math_phys_conserved
{
    ldg_uint256_t magnitude;
    uint8_t sign;
    uint8_t pudding[7];
} ldg_math_phys_conserved_t;

typedef struct ldg_math_phys_tag
{
    int64_t lepton;
    int64_t baryon;
} ldg_math_phys_tag_t;

typedef struct ldg_math_phys_region_policy
{
    double merge_dist;
    double promote_mass;
    double fragment_radius;
    uint64_t high_water;
    uint64_t low_water;
    uint32_t fragment_cunt;
    uint8_t pudding[4];
} ldg_math_phys_region_policy_t;

typedef struct ldg_math_phys_region
{
    ldg_math_phys_region_desc_t desc;
    ldg_math_phys_region_policy_t policy;
    ldg_math_phys_conserved_t total[LDG_MATH_PHYS_QUANTITY_CUNT];
    double accel[LDG_MATH_PHYS_DIM];
    void *field;
    uint64_t body_cunt;
    uint8_t used;
    uint8_t pudding[7];
} ldg_math_phys_region_t;

typedef struct ldg_math_phys_aggregate
{
    char name[LDG_MATH_PHYS_PARTICLE_NAME_MAX];
    uint8_t used;
    uint8_t pudding[7];
} ldg_math_phys_aggregate_t;

typedef struct ldg_math_phys_constraint
{
    uint64_t body_a;
    uint64_t body_b;
    double rest_len;
    double compliance;
    double damping;
    uint32_t generation;
    uint8_t used;
    uint8_t pudding[3];
} ldg_math_phys_constraint_t;

typedef struct ldg_math_phys_collider
{
    ldg_math_phys_collider_desc_t desc;
    uint32_t generation;
    uint8_t used;
    uint8_t pudding[3];
} ldg_math_phys_collider_t;

typedef struct ldg_math_phys_ctx
{
    ldg_math_phys_particle_t particle[LDG_MATH_PHYS_PARTICLE_CUNT];
    ldg_math_phys_conf_t conf;
    ldg_ecs_ctx_t *world;
    ldg_math_phys_region_t *region;
    ldg_math_phys_aggregate_t *aggregate;
    ldg_math_phys_constraint_t *constraint;
    ldg_math_phys_collider_t *collider;
    void *history;
    void *atmosphere;
    void *compute;
    ldg_uint128_t clock;
    double pacing;
    uint64_t body_cunt;
    uint8_t is_init;
    uint8_t pudding[7];
} ldg_math_phys_ctx_t;

LDG_EXPORT uint32_t ldg_math_phys_conf_default_get(ldg_math_phys_conf_t *out);
LDG_EXPORT uint32_t ldg_math_phys_init(ldg_math_phys_ctx_t *ctx, ldg_ecs_ctx_t *world, const ldg_math_phys_conf_t *conf);
LDG_EXPORT uint32_t ldg_math_phys_shutdown(ldg_math_phys_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_math_phys_particle_get(const ldg_math_phys_ctx_t *ctx, uint32_t particle_id, ldg_math_phys_particle_t *out);
LDG_EXPORT uint32_t ldg_math_phys_aggregate_reg(ldg_math_phys_ctx_t *ctx, const ldg_math_phys_aggregate_desc_t *desc, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_phys_region_add(ldg_math_phys_ctx_t *ctx, const ldg_math_phys_region_desc_t *desc, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_phys_optic_lambda_set(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lambda, uint32_t cunt);
LDG_EXPORT uint32_t ldg_math_phys_optic_mtl_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t mtl, const double *absorb, const double *scatter_val, const double *refract);
LDG_EXPORT uint32_t ldg_math_phys_optic_src_add(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, const double *power);
LDG_EXPORT uint32_t ldg_math_phys_optic_solve(ldg_math_phys_ctx_t *ctx, uint32_t region);
LDG_EXPORT uint32_t ldg_math_phys_thermo_mtl_reg(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_thermo_mtl_desc_t *desc, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_phys_thermo_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi, uint32_t mtl, double temp_k, uint32_t hold);
LDG_EXPORT uint32_t ldg_math_phys_thermo_step_advance(ldg_math_phys_ctx_t *ctx, uint32_t region, double dt, uint32_t step_cunt);
LDG_EXPORT uint32_t ldg_math_phys_thermo_pwr_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi, double pwr);
LDG_EXPORT uint32_t ldg_math_phys_thermo_sample_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, double *out_temp, double *out_flux);
LDG_EXPORT uint32_t ldg_math_phys_thermo_vol_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t vol_stride, double *out_temp, double *out_flux);
LDG_EXPORT uint32_t ldg_math_phys_thermo_slice_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_slice_desc_t *desc, double *out_temp, double *out_flux);
LDG_EXPORT uint32_t ldg_math_phys_thermo_conserved_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_heat, double *out_escaped_cum, double *out_injected_cum);
LDG_EXPORT uint32_t ldg_math_phys_water_solid_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi);
LDG_EXPORT uint32_t ldg_math_phys_water_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi);
LDG_EXPORT uint32_t ldg_math_phys_water_step_advance(ldg_math_phys_ctx_t *ctx, uint32_t region, double dt, uint32_t step_cunt);
LDG_EXPORT uint32_t ldg_math_phys_water_sample_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, uint32_t *out_state, double *out_vel);
LDG_EXPORT uint32_t ldg_math_phys_water_conserved_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_mass, double *out_crowd_max);
LDG_EXPORT uint32_t ldg_math_phys_region_grid_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_phys_water_fraction_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, double *out);
LDG_EXPORT uint32_t ldg_math_phys_water_optic_set(ldg_math_phys_ctx_t *ctx, uint32_t region, double refract, const double *absorb, uint32_t bin_cunt);
LDG_EXPORT uint32_t ldg_math_phys_water_optic_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_refract, double *out_absorb, uint32_t *out_bin_cunt);
LDG_EXPORT uint32_t ldg_math_phys_water_caustic_get(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *light_dir, double *out, double *out_escaped);
LDG_EXPORT uint32_t ldg_math_phys_optic_conserved_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t bin, double *out_injected, double *out_absorbed, double *out_escaped, double *out_residual);
LDG_EXPORT uint32_t ldg_math_phys_optic_flux_sample_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t bin, const double *pos, double *out);
LDG_EXPORT uint32_t ldg_math_phys_optic_flux_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t bin, double *out);
LDG_EXPORT uint32_t ldg_math_phys_atmosphere_set(ldg_math_phys_ctx_t *ctx, const ldg_math_phys_atmosphere_desc_t *desc);
LDG_EXPORT uint32_t ldg_math_phys_atmosphere_solve(ldg_math_phys_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_math_phys_atmosphere_transmittance_get(const ldg_math_phys_ctx_t *ctx, double altitude, double zenith_cos, double *out);
LDG_EXPORT uint32_t ldg_math_phys_atmosphere_radiance_get(const ldg_math_phys_ctx_t *ctx, double altitude, double view_cos, double sun_cos, double view_sun_cos, double *out);
LDG_EXPORT uint32_t ldg_math_phys_atmosphere_irradiance_get(const ldg_math_phys_ctx_t *ctx, double altitude, double sun_cos, double normal_cos, double normal_sun_cos, double *out);
LDG_EXPORT uint32_t ldg_math_phys_atmosphere_extinction_get(const ldg_math_phys_ctx_t *ctx, double altitude, double *out);
LDG_EXPORT uint32_t ldg_math_phys_atmosphere_vol_get(const ldg_math_phys_ctx_t *ctx, const ldg_math_phys_atmosphere_vol_desc_t *desc, double *out_transmittance, double *out_inscatter);
LDG_EXPORT uint32_t ldg_math_phys_atmosphere_medium_get(const ldg_math_phys_ctx_t *ctx, uint32_t medium, double altitude, double *out_scatter, double *out_absorb, double *out_anisotropy);
LDG_EXPORT uint32_t ldg_math_phys_doppler_acoustic_freq_get(const double *src_pos, const double *src_velocity, const double *receiver_pos, const double *receiver_velocity, double wave_speed, double freq, double *out);
LDG_EXPORT uint32_t ldg_math_phys_doppler_em_freq_get(const double *src_pos, const double *src_velocity, const double *receiver_pos, double freq, double *out);
LDG_EXPORT uint32_t ldg_math_phys_em_drive_add(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, uint32_t comp, double amplitude, double freq, uint32_t mode, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_phys_em_drive_sig_add(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, uint32_t comp, const ldg_math_phys_sig_t *sig, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_phys_em_conductor_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi, uint32_t conductor);
LDG_EXPORT uint32_t ldg_math_phys_em_capacitance_solve(ldg_math_phys_ctx_t *ctx, uint32_t region, double tol, uint64_t iter_max, double *out);
LDG_EXPORT uint32_t ldg_math_phys_em_induct_solve(ldg_math_phys_ctx_t *ctx, uint32_t region, double tol, uint64_t iter_max, double *out);
LDG_EXPORT uint32_t ldg_math_phys_em_rlgc_solve(ldg_math_phys_ctx_t *ctx, uint32_t region, double freq, double tol, uint64_t iter_max, double *out_r, double *out_l, double *out_g, double *out_c);
LDG_EXPORT uint32_t ldg_math_phys_em_port_add(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_em_port_desc_t *desc, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_phys_em_port_grid_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t port, uint64_t *out_a, uint64_t *out_b);
LDG_EXPORT uint32_t ldg_math_phys_em_port_mode_analytic_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t port, uint32_t mode, uint32_t kind, uint32_t idx_m, uint32_t idx_n);
LDG_EXPORT uint32_t ldg_math_phys_em_port_mode_fem_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t port, uint32_t mode, double freq, uint32_t mode_idx, double *out_beta);
LDG_EXPORT uint32_t ldg_math_phys_em_port_mode_pattern_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t port, uint32_t mode, const double *e_pattern, const double *h_pattern);
LDG_EXPORT uint32_t ldg_math_phys_em_port_tap_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t port, uint32_t mode, double *out_v, double *out_i, uint64_t cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_phys_em_drive_series_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t drive, const double *series, uint64_t cunt);
LDG_EXPORT uint32_t ldg_math_phys_sig_sine_init(double amplitude, double freq, ldg_math_phys_sig_t *out);
LDG_EXPORT uint32_t ldg_math_phys_sig_gauss_pulse_init(double amplitude, double freq, ldg_math_phys_sig_t *out);
LDG_EXPORT uint32_t ldg_math_phys_sig_gauss_derivative_init(double amplitude, double freq, ldg_math_phys_sig_t *out);
LDG_EXPORT uint32_t ldg_math_phys_sig_ricker_init(double amplitude, double freq, ldg_math_phys_sig_t *out);
LDG_EXPORT uint32_t ldg_math_phys_sig_prbs_init(double amplitude, double freq, uint32_t order, uint64_t seed, double edge_fraction, ldg_math_phys_sig_t *out);
LDG_EXPORT uint32_t ldg_math_phys_sig_multitone_init(double amplitude, const double *tone_freq, const double *tone_amplitude, const double *tone_phase, uint32_t tone_cunt, ldg_math_phys_sig_t *out);
LDG_EXPORT uint32_t ldg_math_phys_sig_chirp_init(double amplitude, double freq, double freq_end, double sweep_time, ldg_math_phys_sig_t *out);
LDG_EXPORT uint32_t ldg_math_phys_sig_sampled_init(double amplitude, const double *series, uint64_t cunt, ldg_math_phys_sig_t *out);
LDG_EXPORT uint32_t ldg_math_phys_sig_sample_get(ldg_math_phys_sig_t *sig, uint64_t tick, double dt, double *out);
LDG_EXPORT uint32_t ldg_math_phys_compute_set(ldg_math_phys_ctx_t *ctx, void *compute);
LDG_EXPORT uint32_t ldg_math_phys_compute_clear(ldg_math_phys_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_math_phys_compute_region_arm(ldg_math_phys_ctx_t *ctx, uint32_t region);
LDG_EXPORT uint32_t ldg_math_phys_compute_burst_advance(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt, double *out_tap);
LDG_EXPORT uint32_t ldg_math_phys_compute_burst_begin(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t tick_cunt, const double *probe_pos, uint32_t probe_cunt);
LDG_EXPORT uint32_t ldg_math_phys_compute_burst_poll(ldg_math_phys_ctx_t *ctx, uint32_t region, uint8_t *out);
LDG_EXPORT uint32_t ldg_math_phys_compute_burst_await(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_tap);
LDG_EXPORT uint32_t ldg_math_phys_compute_sync(ldg_math_phys_ctx_t *ctx, uint32_t region);
LDG_EXPORT uint32_t ldg_math_phys_compute_pack_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t vol_stride, uint32_t slice_z);
LDG_EXPORT uint32_t ldg_math_phys_compute_pack_get(ldg_math_phys_ctx_t *ctx, uint32_t region, double *out_vol, double *out_slice);
LDG_EXPORT uint32_t ldg_math_phys_region_accel_set(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *accel);
LDG_EXPORT uint32_t ldg_math_phys_body_create(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t kind, const ldg_math_phys_body_desc_t *desc, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_phys_body_destroy(ldg_math_phys_ctx_t *ctx, uint64_t body);
LDG_EXPORT uint32_t ldg_math_phys_tier_pin_set(ldg_math_phys_ctx_t *ctx, uint64_t body, uint32_t tier);
LDG_EXPORT uint32_t ldg_math_phys_constraint_add(ldg_math_phys_ctx_t *ctx, uint64_t body_a, uint64_t body_b, double rest_len, double compliance, double damping, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_phys_constraint_remove(ldg_math_phys_ctx_t *ctx, uint64_t constraint);
LDG_EXPORT uint32_t ldg_math_phys_collider_add(ldg_math_phys_ctx_t *ctx, const ldg_math_phys_collider_desc_t *desc, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_phys_collider_remove(ldg_math_phys_ctx_t *ctx, uint64_t collider);
LDG_EXPORT uint32_t ldg_math_phys_sweep_cast(const ldg_math_phys_ctx_t *ctx, const ldg_math_phys_sweep_desc_t *sweep, ldg_math_phys_sweep_hit_t *out, uint64_t out_cap, uint64_t *out_cunt);
LDG_EXPORT uint32_t ldg_math_phys_src_add(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, double amount);
LDG_EXPORT uint32_t ldg_math_phys_mtl_reg(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_mtl_desc_t *desc, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_phys_mtl_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi, uint32_t mtl);
LDG_EXPORT uint32_t ldg_math_phys_acoustic_mtl_reg(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_acoustic_mtl_desc_t *desc, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_phys_acoustic_mtl_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *lo, const double *hi, uint32_t mtl);
LDG_EXPORT uint32_t ldg_math_phys_wall_reg(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_wall_desc_t *desc, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_phys_wall_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t normal_axis, double plane_pos, const double *vert, uint64_t vert_cunt, uint32_t wall);
LDG_EXPORT uint32_t ldg_math_phys_field_acoustic_sample_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, const ldg_uint128_t *time, double *out_pressure, double *out_velocity);
LDG_EXPORT uint32_t ldg_math_phys_field_acoustic_slice_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_slice_desc_t *desc, const ldg_uint128_t *time, double *out_pressure, double *out_velocity);
LDG_EXPORT uint32_t ldg_math_phys_acoustic_monopole_add(ldg_math_phys_ctx_t *ctx, uint32_t region, uint64_t body, double amplitude, double freq, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_phys_acoustic_monopole_series_set(ldg_math_phys_ctx_t *ctx, uint32_t region, uint64_t monopole, const double *series, uint64_t cunt);
LDG_EXPORT uint32_t ldg_math_phys_sheet_reg(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_sheet_desc_t *desc, uint32_t *out);
LDG_EXPORT uint32_t ldg_math_phys_sheet_fill(ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t normal_axis, double plane_pos, const double *vert, uint64_t vert_cunt, uint32_t sheet);
LDG_EXPORT uint32_t ldg_math_phys_current_add(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, const double *current);
LDG_EXPORT uint32_t ldg_math_phys_plane_wave_set(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_plane_wave_desc_t *desc);
LDG_EXPORT uint32_t ldg_math_phys_plane_wave_drive(ldg_math_phys_ctx_t *ctx, uint32_t region, double amplitude);
LDG_EXPORT uint32_t ldg_math_phys_field_em_sample_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, const ldg_uint128_t *time, double *out_electric, double *out_magnetic);
LDG_EXPORT uint32_t ldg_math_phys_field_em_slice_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_slice_desc_t *desc, const ldg_uint128_t *time, double *out_electric, double *out_magnetic);
LDG_EXPORT uint32_t ldg_math_phys_field_em_vol_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t vol_stride, double *out);
LDG_EXPORT uint32_t ldg_math_phys_pacing_set(ldg_math_phys_ctx_t *ctx, double ratio);
LDG_EXPORT uint32_t ldg_math_phys_step(ldg_math_phys_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_math_phys_advance(ldg_math_phys_ctx_t *ctx, const ldg_uint128_t *until);
LDG_EXPORT uint32_t ldg_math_phys_time_get(const ldg_math_phys_ctx_t *ctx, ldg_uint128_t *out);
LDG_EXPORT uint32_t ldg_math_phys_body_sample_get(const ldg_math_phys_ctx_t *ctx, uint64_t body, const ldg_uint128_t *time, ldg_math_phys_body_state_t *out);
LDG_EXPORT uint32_t ldg_math_phys_body_bulk_sample_get(const ldg_math_phys_ctx_t *ctx, const uint64_t *body, uint64_t cunt, const ldg_uint128_t *time, ldg_math_phys_body_state_t *out);
LDG_EXPORT uint32_t ldg_math_phys_field_sample_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, const ldg_uint128_t *time, double *out);
LDG_EXPORT uint32_t ldg_math_phys_conserved_get(const ldg_math_phys_ctx_t *ctx, uint32_t region, uint32_t quantity, ldg_uint256_t *magnitude, uint8_t *sign);
LDG_EXPORT uint64_t ldg_math_phys_body_cunt_get(const ldg_math_phys_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_math_phys_body_split(ldg_math_phys_ctx_t *ctx, uint64_t body, const uint32_t *kind, const ldg_math_phys_body_desc_t *desc, uint64_t cunt, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_phys_body_merge(ldg_math_phys_ctx_t *ctx, const uint64_t *body, uint64_t cunt, uint32_t kind, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_phys_region_couple_set(ldg_math_phys_ctx_t *ctx, uint32_t region, double emit, double absorb);
LDG_EXPORT uint32_t ldg_math_phys_region_policy_set(ldg_math_phys_ctx_t *ctx, uint32_t region, const ldg_math_phys_region_policy_t *desc);
LDG_EXPORT uint32_t ldg_math_phys_body_region_set(ldg_math_phys_ctx_t *ctx, uint64_t body, uint32_t region, const double *pos, uint64_t *out);
LDG_EXPORT uint32_t ldg_math_phys_body_dissolve(ldg_math_phys_ctx_t *ctx, uint64_t body);
LDG_EXPORT uint32_t ldg_math_phys_body_condense(ldg_math_phys_ctx_t *ctx, uint32_t region, const double *pos, double energy, uint64_t *out);

#endif
