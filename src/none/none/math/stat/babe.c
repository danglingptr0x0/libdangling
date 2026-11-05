#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/math/stat.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>

#define BABE_THRESHOLD_DEFAULT (1000 * LDG_MATH_STAT_BABE_FIXED_ONE)
#define BABE_RST_DEFAULT 0
#define BABE_ADAPT_STEP_DEFAULT (100 * LDG_MATH_STAT_BABE_FIXED_ONE)
#define BABE_WEIGHT_INIT_DEFAULT (200 * LDG_MATH_STAT_BABE_FIXED_ONE)
#define BABE_STDP_STEP_DEFAULT LDG_MATH_STAT_BABE_FIXED_ONE
#define BABE_INHIBIT_FRAC_DEFAULT (LDG_MATH_STAT_BABE_FIXED_ONE / 5)
#define BABE_TOLERANCE_MARK_DEFAULT (4 * LDG_MATH_STAT_BABE_FIXED_ONE)
#define BABE_LEAK_SHIFT_DEFAULT 4
#define BABE_REFRAC_DEFAULT 2
#define BABE_ADAPT_SHIFT_DEFAULT 6
#define BABE_STDP_WINDOW_DEFAULT 4
#define BABE_LR_SHIFT_DEFAULT 8
#define BABE_LATERAL_RADIUS_DEFAULT 4
#define BABE_DIFFUSE_SHIFT_DEFAULT 5
#define BABE_TOLERANCE_DOWN_DEFAULT 10
#define BABE_TOLERANCE_UP_DEFAULT 12
#define BABE_CLASS_MOTOR 0
#define BABE_CLASS_SENSE 1
#define BABE_CLASS_FOOD 2
#define BABE_CLASS_LATERAL 3
#define BABE_CLASS_CUNT_DEFAULT 4
#define BABE_TRACE_SHIFT_MOTOR 3
#define BABE_TRACE_SHIFT_SENSE 6
#define BABE_TRACE_SHIFT_FOOD 10
#define BABE_TRACE_SHIFT_LATERAL 6
#define BABE_SUBSTANCE_REWARD 0
#define BABE_SUBSTANCE_PAIN 1
#define BABE_SUBSTANCE_NOVELTY 2
#define BABE_SUBSTANCE_SATIETY 3
#define BABE_SUBSTANCE_FATIGUE 4
#define BABE_SUBSTANCE_CUNT_DEFAULT 5
#define BABE_HALF_LIFE_REWARD 4
#define BABE_HALF_LIFE_PAIN 6
#define BABE_HALF_LIFE_NOVELTY 3
#define BABE_HALF_LIFE_SATIETY 10
#define BABE_HALF_LIFE_FATIGUE 12
#define BABE_RECEPTOR_FULL LDG_MATH_STAT_BABE_FIXED_ONE
#define BABE_RECEPTOR_FULL_NEG (-LDG_MATH_STAT_BABE_FIXED_ONE)
#define BABE_RECEPTOR_HALF_NEG (-(LDG_MATH_STAT_BABE_FIXED_ONE / 2))
#define BABE_RECEPTOR_QUARTER (LDG_MATH_STAT_BABE_FIXED_ONE / 4)
#define BABE_EXCITE_FATIGUE (200 * LDG_MATH_STAT_BABE_FIXED_ONE)
#define BABE_EXCITE_SATIETY (100 * LDG_MATH_STAT_BABE_FIXED_ONE)
#define BABE_GRID_DEFAULT 64
#define BABE_IN_CUNT_DEFAULT 16
#define BABE_IN_PER_UNIT_DEFAULT 32
#define BABE_LATERAL_PER_UNIT_DEFAULT 0
#define BABE_FRAC_MASK (LDG_MATH_STAT_BABE_FIXED_ONE - 1)
#define BABE_HIST_MASK (LDG_MATH_STAT_BABE_READOUT_STEP_CUNT - 1)
#define BABE_HIST_WORD_MASK 0xFFFFU
#define BABE_HIST_NEXT 1
#define BABE_ARRIVAL_NEXT 1
#define BABE_WEIGHT_HALF_SHIFT 1
#define BABE_FRAC_SIGN_SHIFT 1
#define BABE_SPAN_CENTER 1
#define BABE_STENCIL_SIDE_FACTOR 2
#define BABE_EXCITE_LANE_CUNT 2
#define BABE_EXCITE_THRESHOLD 0
#define BABE_EXCITE_LEAK 1
#define BABE_NEIGHBOR_CUNT 4
#define BABE_STATE_HDR_CUNT 2
#define BABE_STATE_HDR_HASH 0
#define BABE_STATE_HDR_STEP 1
#define BABE_PROTO_HDR_CUNT 2
#define BABE_FNV_BASIS 2166136261U
#define BABE_FNV_PRIME 16777619U
#define BABE_ZERO_FIXED 0
#define BABE_NO_TRACE 0
#define BABE_TRACE_FLOOR_DEFAULT 0
#define BABE_DELAY_INPUT 0
#define BABE_SLOT_FIRST 0
#define BABE_UNIT_FIRST 0
#define BABE_LANE_NONE UINT16_MAX

typedef struct babe_size
{
    uint64_t total;
    uint32_t unit_cunt;
    uint32_t lateral_cunt;
    uint32_t slot_cunt;
    uint32_t synapse_cunt;
    uint32_t readout_cunt;
} babe_size_t;

static ldg_math_stat_babe_fixed_t babe_sat_add(ldg_math_stat_babe_fixed_t lhs, ldg_math_stat_babe_fixed_t rhs)
{
    int64_t sum = (int64_t)lhs + (int64_t)rhs;

    if (sum > INT32_MAX) { return INT32_MAX; }

    if (sum < INT32_MIN) { return INT32_MIN; }

    return (ldg_math_stat_babe_fixed_t)sum;
}

static ldg_math_stat_babe_fixed_t babe_fixed_mul(ldg_math_stat_babe_fixed_t lhs, ldg_math_stat_babe_fixed_t rhs)
{
    int64_t product = ((int64_t)lhs * (int64_t)rhs) >> LDG_MATH_STAT_BABE_FIXED_SHIFT;

    if (product > INT32_MAX) { return INT32_MAX; }

    if (product < INT32_MIN) { return INT32_MIN; }

    return (ldg_math_stat_babe_fixed_t)product;
}

static ldg_math_stat_babe_fixed_t babe_mul_shift(ldg_math_stat_babe_fixed_t lhs, ldg_math_stat_babe_fixed_t rhs, uint32_t shift)
{
    int64_t product = ((int64_t)lhs * (int64_t)rhs) >> (LDG_MATH_STAT_BABE_FIXED_SHIFT + shift);

    if (product > INT32_MAX) { return INT32_MAX; }

    if (product < INT32_MIN) { return INT32_MIN; }

    return (ldg_math_stat_babe_fixed_t)product;
}

static uint32_t babe_draw(const ldg_math_stat_babe_t *babe)
{
    return (uint32_t)babe->rng_cb(babe->rng_ctx);
}

static ldg_math_stat_babe_fixed_t babe_frac_draw(const ldg_math_stat_babe_t *babe)
{
    return (ldg_math_stat_babe_fixed_t)(babe_draw(babe) & BABE_FRAC_MASK);
}

static uint32_t babe_alloc(uint64_t size, void **out)
{
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc(size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

static void babe_free(void *ptr)
{
    if (ptr) { ldg_mem_dealloc(ptr); }
}

static uint32_t babe_hash_bytes(uint32_t hash, const uint8_t *src, uintptr_t cunt)
{
    const uint8_t *end = src + cunt;

    while (src < end)
    {
        hash = (hash ^ (uint32_t)*src) * BABE_FNV_PRIME;
        src++;
    }

    return hash;
}

static uint32_t babe_pop_get(uint32_t val)
{
    uint32_t cunt = 0;

    while (val != 0)
    {
        val = val & (val - BABE_HIST_NEXT);
        cunt++;
    }

    return cunt;
}

uint32_t ldg_math_stat_babe_species_default_get(ldg_math_stat_babe_species_t *out)
{
    uint32_t kind = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, (uint64_t)sizeof(ldg_math_stat_babe_species_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->threshold = BABE_THRESHOLD_DEFAULT;
    out->rst_val = BABE_RST_DEFAULT;
    out->adapt_step = BABE_ADAPT_STEP_DEFAULT;
    out->weight_init = BABE_WEIGHT_INIT_DEFAULT;
    out->stdp_step = BABE_STDP_STEP_DEFAULT;
    out->lateral_inhibit_frac = BABE_INHIBIT_FRAC_DEFAULT;
    out->trace_floor = BABE_TRACE_FLOOR_DEFAULT;
    out->class_cunt = BABE_CLASS_CUNT_DEFAULT;
    out->substance_cunt = BABE_SUBSTANCE_CUNT_DEFAULT;
    out->motor_class = BABE_CLASS_MOTOR;
    out->lateral_class = BABE_CLASS_LATERAL;
    out->leak_shift = BABE_LEAK_SHIFT_DEFAULT;
    out->refrac_step_cunt = BABE_REFRAC_DEFAULT;
    out->adapt_shift = BABE_ADAPT_SHIFT_DEFAULT;
    out->stdp_window = BABE_STDP_WINDOW_DEFAULT;
    out->lr_shift = BABE_LR_SHIFT_DEFAULT;
    out->delay_max = LDG_MATH_STAT_BABE_DELAY_MAX;
    out->lateral_radius = BABE_LATERAL_RADIUS_DEFAULT;
    out->diffuse_shift = BABE_DIFFUSE_SHIFT_DEFAULT;
    out->tolerance_down_shift = BABE_TOLERANCE_DOWN_DEFAULT;
    out->tolerance_up_shift = BABE_TOLERANCE_UP_DEFAULT;

    out->trace_shift[BABE_CLASS_MOTOR] = BABE_TRACE_SHIFT_MOTOR;
    out->trace_shift[BABE_CLASS_SENSE] = BABE_TRACE_SHIFT_SENSE;
    out->trace_shift[BABE_CLASS_FOOD] = BABE_TRACE_SHIFT_FOOD;
    out->trace_shift[BABE_CLASS_LATERAL] = BABE_TRACE_SHIFT_LATERAL;

    out->half_life_shift[BABE_SUBSTANCE_REWARD] = BABE_HALF_LIFE_REWARD;
    out->half_life_shift[BABE_SUBSTANCE_PAIN] = BABE_HALF_LIFE_PAIN;
    out->half_life_shift[BABE_SUBSTANCE_NOVELTY] = BABE_HALF_LIFE_NOVELTY;
    out->half_life_shift[BABE_SUBSTANCE_SATIETY] = BABE_HALF_LIFE_SATIETY;
    out->half_life_shift[BABE_SUBSTANCE_FATIGUE] = BABE_HALF_LIFE_FATIGUE;

    while (kind < BABE_CLASS_CUNT_DEFAULT)
    {
        out->receptor[kind][BABE_SUBSTANCE_REWARD] = BABE_RECEPTOR_FULL;
        out->tolerance_mark[kind] = BABE_TOLERANCE_MARK_DEFAULT;
        kind++;
    }

    out->tolerance_mark[BABE_SUBSTANCE_FATIGUE] = BABE_TOLERANCE_MARK_DEFAULT;
    out->receptor[BABE_CLASS_FOOD][BABE_SUBSTANCE_PAIN] = BABE_RECEPTOR_FULL_NEG;
    out->receptor[BABE_CLASS_SENSE][BABE_SUBSTANCE_PAIN] = BABE_RECEPTOR_HALF_NEG;
    out->receptor[BABE_CLASS_MOTOR][BABE_SUBSTANCE_PAIN] = BABE_RECEPTOR_HALF_NEG;
    out->receptor[BABE_CLASS_SENSE][BABE_SUBSTANCE_NOVELTY] = BABE_RECEPTOR_QUARTER;
    out->receptor[BABE_CLASS_LATERAL][BABE_SUBSTANCE_NOVELTY] = BABE_RECEPTOR_QUARTER;
    out->excite_threshold[LDG_MATH_STAT_BABE_UNIT_GRID][BABE_SUBSTANCE_FATIGUE] = BABE_EXCITE_FATIGUE;
    out->excite_threshold[LDG_MATH_STAT_BABE_UNIT_GRID][BABE_SUBSTANCE_SATIETY] = BABE_EXCITE_SATIETY;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_conf_default_get(ldg_math_stat_babe_conf_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_zero(out, (uint64_t)sizeof(ldg_math_stat_babe_conf_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->grid_h = BABE_GRID_DEFAULT;
    out->grid_w = BABE_GRID_DEFAULT;
    out->in_cunt = BABE_IN_CUNT_DEFAULT;
    out->in_per_unit = BABE_IN_PER_UNIT_DEFAULT;
    out->lateral_per_unit = BABE_LATERAL_PER_UNIT_DEFAULT;

    return LDG_ERR_AOK;
}

static uint32_t babe_lateral_cunt_get(const ldg_math_stat_babe_species_t *species)
{
    uint32_t side = (BABE_STENCIL_SIDE_FACTOR * species->lateral_radius) + BABE_SPAN_CENTER;

    return (side * side) - BABE_SPAN_CENTER;
}

static uint32_t babe_species_ck(const ldg_math_stat_babe_species_t *species)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(species->class_cunt == 0 || species->class_cunt > LDG_MATH_STAT_BABE_CLASS_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(species->substance_cunt > LDG_MATH_STAT_BABE_SUBSTANCE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(species->motor_class >= species->class_cunt || species->lateral_class >= species->class_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(species->lateral_radius == 0 || species->lateral_radius > LDG_MATH_STAT_BABE_DELAY_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(species->stdp_window >= LDG_MATH_STAT_BABE_HIST_BITS - LDG_MATH_STAT_BABE_DELAY_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(species->trace_floor < BABE_ZERO_FIXED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(species->lateral_inhibit_frac < BABE_ZERO_FIXED || species->lateral_inhibit_frac > LDG_MATH_STAT_BABE_FIXED_ONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(species->leak_shift >= LDG_MATH_STAT_BABE_FIXED_SHIFT || species->adapt_shift >= LDG_MATH_STAT_BABE_FIXED_SHIFT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (idx < species->class_cunt)
    {
        if (LDG_UNLIKELY(species->trace_shift[idx] >= LDG_MATH_STAT_BABE_FIXED_SHIFT)) { return LDG_ERR_FUNC_ARG_INVALID; }

        idx++;
    }

    return LDG_ERR_AOK;
}

static uint32_t babe_conf_ck(const ldg_math_stat_babe_species_t *species, const ldg_math_stat_babe_conf_t *conf)
{
    uint32_t lane = 0;

    if (LDG_UNLIKELY(conf->grid_h == 0 || conf->grid_w == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->in_cunt > LDG_MATH_STAT_BABE_IN_CUNT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->in_per_unit > 0 && (conf->in_cunt == 0 || !conf->in_class))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->lateral_per_unit != 0 && conf->lateral_per_unit != babe_lateral_cunt_get(species))) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (lane < conf->in_cunt)
    {
        if (LDG_UNLIKELY(conf->in_class[lane] >= species->class_cunt)) { return LDG_ERR_FUNC_ARG_INVALID; }

        lane++;
    }

    return LDG_ERR_AOK;
}

static void babe_size_compute(const ldg_math_stat_babe_species_t *species, const ldg_math_stat_babe_conf_t *conf, babe_size_t *out)
{
    uint64_t total = 0;
    uint64_t src_cunt = 0;

    out->unit_cunt = conf->grid_h * conf->grid_w;
    out->lateral_cunt = babe_lateral_cunt_get(species);
    out->slot_cunt = out->lateral_cunt + conf->in_per_unit;
    out->synapse_cunt = out->unit_cunt * out->slot_cunt;
    out->readout_cunt = out->unit_cunt / LDG_MATH_STAT_BABE_READOUT_DIV;
    if (out->readout_cunt == 0) { out->readout_cunt = BABE_SPAN_CENTER; }

    src_cunt = (uint64_t)out->unit_cunt + (uint64_t)conf->in_cunt;

    total = total + ((uint64_t)out->unit_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t) * (uint64_t)BABE_EXCITE_LANE_CUNT);
    total = total + ((uint64_t)out->synapse_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t) * (uint64_t)BABE_EXCITE_LANE_CUNT);
    total = total + ((uint64_t)out->unit_cunt * (uint64_t)sizeof(uint32_t)) + (src_cunt * (uint64_t)sizeof(uint32_t));
    total = total + ((uint64_t)out->unit_cunt * (uint64_t)conf->in_per_unit * (uint64_t)sizeof(uint16_t));
    total = total + (src_cunt * (uint64_t)sizeof(uint16_t)) + (src_cunt * (uint64_t)sizeof(uint8_t));
    total = total + ((uint64_t)out->lateral_cunt * ((uint64_t)sizeof(uint16_t) * (uint64_t)BABE_EXCITE_LANE_CUNT + (uint64_t)sizeof(uint8_t) * (uint64_t)BABE_NEIGHBOR_CUNT));
    total = total + ((uint64_t)out->unit_cunt * (uint64_t)sizeof(uint8_t) * (uint64_t)BABE_EXCITE_LANE_CUNT);
    total = total + ((uint64_t)conf->in_cunt * (uint64_t)sizeof(uint8_t)) + ((uint64_t)out->readout_cunt * (uint64_t)LDG_MATH_STAT_BABE_READOUT_STEP_CUNT);
    total = total + ((uint64_t)out->readout_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t));
    total = total + ((uint64_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX * (uint64_t)LDG_MATH_STAT_BABE_REGION_CUNT * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t) * (uint64_t)BABE_EXCITE_LANE_CUNT);
    total = total + ((uint64_t)LDG_MATH_STAT_BABE_CLASS_MAX * (uint64_t)LDG_MATH_STAT_BABE_REGION_CUNT * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t));
    total = total + ((uint64_t)LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT * (uint64_t)LDG_MATH_STAT_BABE_REGION_CUNT * (uint64_t)BABE_EXCITE_LANE_CUNT * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t));

    out->total = total;
}

uint32_t ldg_math_stat_babe_size_get(const ldg_math_stat_babe_species_t *species, const ldg_math_stat_babe_conf_t *conf, uint64_t *out)
{
    babe_size_t size = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!species || !conf || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = babe_species_ck(species);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = babe_conf_ck(species, conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe_size_compute(species, conf, &size);

    *out = size.total;

    return LDG_ERR_AOK;
}

static uint32_t babe_region_compute(const ldg_math_stat_babe_t *babe, uint32_t unit)
{
    uint32_t row = unit / babe->grid_w;
    uint32_t col = unit % babe->grid_w;
    uint32_t region_row = (row * LDG_MATH_STAT_BABE_REGION_DIV) / babe->grid_h;
    uint32_t region_col = (col * LDG_MATH_STAT_BABE_REGION_DIV) / babe->grid_w;

    return (region_row * LDG_MATH_STAT_BABE_REGION_DIV) + region_col;
}

static ldg_math_stat_babe_fixed_t babe_weight_draw(const ldg_math_stat_babe_t *babe)
{
    ldg_math_stat_babe_fixed_t half = babe->species->weight_init >> BABE_WEIGHT_HALF_SHIFT;

    return babe_sat_add(half, babe_fixed_mul(half, babe_frac_draw(babe)));
}

static uint32_t babe_dist_get(int32_t dx, int32_t dy)
{
    uint32_t adx = (dx < 0) ? (uint32_t)(-dx) : (uint32_t)dx;
    uint32_t ady = (dy < 0) ? (uint32_t)(-dy) : (uint32_t)dy;

    return (adx > ady) ? adx : ady;
}

static void babe_stencil_build(ldg_math_stat_babe_t *babe)
{
    const ldg_math_stat_babe_species_t *species = babe->species;
    int32_t radius = (int32_t)species->lateral_radius;
    uint32_t inhibit_cunt = (uint32_t)(((uint64_t)babe->lateral_per_unit * (uint64_t)species->lateral_inhibit_frac) >> LDG_MATH_STAT_BABE_FIXED_SHIFT);
    uint32_t slot = 0;
    uint32_t dist = 0;
    uint32_t ring = 0;
    uint32_t window = species->stdp_window;
    uint32_t pot = 0;
    uint32_t dep = 0;
    uint32_t bit = 0;
    int32_t dx = 0;
    int32_t dy = 0;

    ring = BABE_SPAN_CENTER;
    while (ring <= (uint32_t)radius)
    {
        dy = -radius;
        while (dy <= radius)
        {
            dx = -radius;
            while (dx <= radius)
            {
                dist = babe_dist_get(dx, dy);
                if (dist == ring)
                {
                    babe->stencil_dx[slot] = (int8_t)dx;
                    babe->stencil_dy[slot] = (int8_t)dy;
                    babe->stencil_delay[slot] = (uint8_t)dist;
                    babe->stencil_inhibit_is[slot] = LDG_TRUTH_FALSE;
                    if (slot + inhibit_cunt >= babe->lateral_per_unit) { babe->stencil_inhibit_is[slot] = LDG_TRUTH_TRUE; }

                    slot++;
                }

                dx++;
            }

            dy++;
        }

        ring++;
    }

    slot = 0;
    while (slot < babe->lateral_per_unit)
    {
        dist = babe->stencil_delay[slot];
        pot = 0;
        dep = 0;

        bit = 0;
        while (bit < LDG_MATH_STAT_BABE_HIST_BITS)
        {
            if (bit >= dist && bit - dist <= window) { pot = pot | LDG_BIT(bit); }

            if (bit < dist && dist - bit <= window) { dep = dep | LDG_BIT(bit); }

            bit++;
        }

        babe->stencil_pot[slot] = (uint16_t)pot;
        babe->stencil_dep[slot] = (uint16_t)dep;
        slot++;
    }
}

static uint8_t babe_neighbor_get(const ldg_math_stat_babe_t *babe, uint32_t unit, uint32_t slot, uint32_t *out)
{
    int32_t row = (int32_t)(unit / babe->grid_w) + (int32_t)babe->stencil_dy[slot];
    int32_t col = (int32_t)(unit % babe->grid_w) + (int32_t)babe->stencil_dx[slot];

    *out = 0;

    if (row < 0 || col < 0 || row >= (int32_t)babe->grid_h || col >= (int32_t)babe->grid_w) { return LDG_TRUTH_FALSE; }

    *out = ((uint32_t)row * babe->grid_w) + (uint32_t)col;

    return LDG_TRUTH_TRUE;
}

static void babe_weight_wire(ldg_math_stat_babe_t *babe)
{
    ldg_math_stat_babe_fixed_t *weight = 0x0;
    uint16_t *lane = 0x0;
    uint32_t unit = 0;
    uint32_t slot = 0;
    uint32_t neighbor = 0;

    while (unit < babe->unit_cunt)
    {
        weight = babe->weight + ((uintptr_t)unit * (uintptr_t)babe->slot_cunt);
        lane = babe->in_lane + ((uintptr_t)unit * (uintptr_t)babe->in_per_unit);

        slot = 0;
        while (slot < babe->lateral_per_unit)
        {
            weight[slot] = BABE_ZERO_FIXED;
            if (babe_neighbor_get(babe, unit, slot, &neighbor) == LDG_TRUTH_TRUE)
            {
                weight[slot] = babe_weight_draw(babe);
                if (babe->stencil_inhibit_is[slot] == LDG_TRUTH_TRUE) { weight[slot] = -weight[slot]; }
            }

            slot++;
        }

        slot = 0;
        while (slot < babe->in_per_unit)
        {
            lane[slot] = (uint16_t)(babe_draw(babe) % babe->in_cunt);
            weight[babe->lateral_per_unit + slot] = babe_weight_draw(babe);
            slot++;
        }

        unit++;
    }
}

static uint32_t babe_hash_compute(const ldg_math_stat_babe_t *babe)
{
    uint32_t hash = BABE_FNV_BASIS;

    hash = babe_hash_bytes(hash, (const uint8_t *)&babe->grid_h, sizeof(babe->grid_h));
    hash = babe_hash_bytes(hash, (const uint8_t *)&babe->grid_w, sizeof(babe->grid_w));
    hash = babe_hash_bytes(hash, (const uint8_t *)&babe->in_cunt, sizeof(babe->in_cunt));
    hash = babe_hash_bytes(hash, (const uint8_t *)&babe->in_per_unit, sizeof(babe->in_per_unit));
    hash = babe_hash_bytes(hash, (const uint8_t *)&babe->lateral_per_unit, sizeof(babe->lateral_per_unit));
    hash = babe_hash_bytes(hash, (const uint8_t *)&babe->species->class_cunt, sizeof(babe->species->class_cunt));
    hash = babe_hash_bytes(hash, (const uint8_t *)&babe->species->substance_cunt, sizeof(babe->species->substance_cunt));
    hash = babe_hash_bytes(hash, babe->in_class, (uintptr_t)babe->in_cunt);
    hash = babe_hash_bytes(hash, (const uint8_t *)babe->in_lane, (uintptr_t)babe->unit_cunt * (uintptr_t)babe->in_per_unit * sizeof(uint16_t));
    hash = babe_hash_bytes(hash, (const uint8_t *)babe->stencil_dx, (uintptr_t)babe->lateral_per_unit);
    hash = babe_hash_bytes(hash, (const uint8_t *)babe->stencil_dy, (uintptr_t)babe->lateral_per_unit);
    hash = babe_hash_bytes(hash, babe->stencil_inhibit_is, (uintptr_t)babe->lateral_per_unit);

    return hash;
}

static void babe_receptor_rst(ldg_math_stat_babe_t *babe)
{
    const ldg_math_stat_babe_species_t *species = babe->species;
    uint32_t kind = 0;
    uint32_t sub = 0;

    while (kind < LDG_MATH_STAT_BABE_CLASS_MAX)
    {
        sub = 0;
        while (sub < LDG_MATH_STAT_BABE_SUBSTANCE_MAX)
        {
            babe->receptor[kind][sub] = species->receptor[kind][sub];
            sub++;
        }

        kind++;
    }

    kind = 0;
    while (kind < LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT)
    {
        sub = 0;
        while (sub < LDG_MATH_STAT_BABE_SUBSTANCE_MAX)
        {
            babe->excite_threshold[kind][sub] = species->excite_threshold[kind][sub];
            babe->excite_leak[kind][sub] = species->excite_leak[kind][sub];
            sub++;
        }

        kind++;
    }
}

static uint32_t babe_dynamic_rst(ldg_math_stat_babe_t *babe)
{
    uintptr_t src_cunt = (uintptr_t)babe->unit_cunt + (uintptr_t)babe->in_cunt;
    uint32_t unit = 0;
    uint32_t ret = 0;

    ret = ldg_mem_zero(babe->membrane, (uint64_t)babe->unit_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(babe->adapt, (uint64_t)babe->unit_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(babe->refrac, (uint64_t)babe->unit_cunt * (uint64_t)sizeof(uint32_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(babe->trace, (uint64_t)babe->synapse_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(babe->concentration, (uint64_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX * (uint64_t)LDG_MATH_STAT_BABE_REGION_CUNT * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(babe->spike_hist, (uint64_t)babe->readout_cunt * (uint64_t)LDG_MATH_STAT_BABE_READOUT_STEP_CUNT);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(babe->readout, (uint64_t)babe->readout_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(babe->hist, (uint64_t)src_cunt * (uint64_t)sizeof(uint16_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(babe->fired_now, (uint64_t)src_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(babe->trace_is, (uint64_t)babe->unit_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (unit < src_cunt)
    {
        babe->last_fire[unit] = LDG_MATH_STAT_BABE_FIRE_NONE;
        unit++;
    }

    babe->fired_cunt = 0;
    babe->step_cunt = 0;

    babe_receptor_rst(babe);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_init(ldg_math_stat_babe_t *babe, const ldg_math_stat_babe_species_t *species, const ldg_math_stat_babe_conf_t *conf, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx)
{
    babe_size_t size = LDG_STRUCT_ZERO_INIT;
    void *ptr = 0x0;
    uint64_t src_cunt = 0;
    uint32_t unit = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!babe || !species || !conf || !rng_cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = babe_species_ck(species);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = babe_conf_ck(species, conf);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_mem_zero(babe, (uint64_t)sizeof(ldg_math_stat_babe_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe_size_compute(species, conf, &size);

    babe->species = species;
    babe->rng_cb = rng_cb;
    babe->rng_ctx = rng_ctx;
    babe->seed = conf->seed;
    babe->grid_h = conf->grid_h;
    babe->grid_w = conf->grid_w;
    babe->unit_cunt = size.unit_cunt;
    babe->in_cunt = conf->in_cunt;
    babe->in_per_unit = conf->in_per_unit;
    babe->lateral_per_unit = size.lateral_cunt;
    babe->slot_cunt = size.slot_cunt;
    babe->synapse_cunt = size.synapse_cunt;
    babe->readout_cunt = size.readout_cunt;
    src_cunt = (uint64_t)babe->unit_cunt + (uint64_t)babe->in_cunt;

    ret = babe_alloc((uint64_t)babe->unit_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->membrane = (ldg_math_stat_babe_fixed_t *)ptr;

    ret = babe_alloc((uint64_t)babe->unit_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->adapt = (ldg_math_stat_babe_fixed_t *)ptr;

    ret = babe_alloc((uint64_t)babe->synapse_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->weight = (ldg_math_stat_babe_fixed_t *)ptr;

    ret = babe_alloc((uint64_t)babe->synapse_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->trace = (ldg_math_stat_babe_fixed_t *)ptr;

    ret = babe_alloc((uint64_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX * (uint64_t)LDG_MATH_STAT_BABE_REGION_CUNT * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->concentration = (ldg_math_stat_babe_fixed_t *)ptr;

    ret = babe_alloc((uint64_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX * (uint64_t)LDG_MATH_STAT_BABE_REGION_CUNT * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->field_buff = (ldg_math_stat_babe_fixed_t *)ptr;

    ret = babe_alloc((uint64_t)LDG_MATH_STAT_BABE_CLASS_MAX * (uint64_t)LDG_MATH_STAT_BABE_REGION_CUNT * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->fx = (ldg_math_stat_babe_fixed_t *)ptr;

    ret = babe_alloc((uint64_t)LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT * (uint64_t)LDG_MATH_STAT_BABE_REGION_CUNT * (uint64_t)BABE_EXCITE_LANE_CUNT * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->excite = (ldg_math_stat_babe_fixed_t *)ptr;

    ret = babe_alloc((uint64_t)babe->readout_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->readout = (ldg_math_stat_babe_fixed_t *)ptr;

    ret = babe_alloc((uint64_t)babe->unit_cunt * (uint64_t)sizeof(uint32_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->refrac = (uint32_t *)ptr;

    ret = babe_alloc(src_cunt * (uint64_t)sizeof(uint32_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->last_fire = (uint32_t *)ptr;

    ret = babe_alloc(((uint64_t)babe->unit_cunt * (uint64_t)babe->in_per_unit * (uint64_t)sizeof(uint16_t)) + (uint64_t)sizeof(uint16_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->in_lane = (uint16_t *)ptr;

    ret = babe_alloc(src_cunt * (uint64_t)sizeof(uint16_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->hist = (uint16_t *)ptr;

    ret = babe_alloc((uint64_t)babe->lateral_per_unit * (uint64_t)sizeof(uint16_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->stencil_pot = (uint16_t *)ptr;

    ret = babe_alloc((uint64_t)babe->lateral_per_unit * (uint64_t)sizeof(uint16_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->stencil_dep = (uint16_t *)ptr;

    ret = babe_alloc((uint64_t)babe->lateral_per_unit, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->stencil_dx = (int8_t *)ptr;

    ret = babe_alloc((uint64_t)babe->lateral_per_unit, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->stencil_dy = (int8_t *)ptr;

    ret = babe_alloc((uint64_t)babe->lateral_per_unit, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->stencil_delay = (uint8_t *)ptr;

    ret = babe_alloc((uint64_t)babe->lateral_per_unit, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->stencil_inhibit_is = (uint8_t *)ptr;

    ret = babe_alloc(src_cunt, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->fired_now = (uint8_t *)ptr;

    ret = babe_alloc((uint64_t)babe->unit_cunt, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->trace_is = (uint8_t *)ptr;

    ret = babe_alloc((uint64_t)babe->unit_cunt, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->region = (uint8_t *)ptr;

    ret = babe_alloc((uint64_t)babe->in_cunt + (uint64_t)BABE_SPAN_CENTER, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->in_class = (uint8_t *)ptr;

    ret = babe_alloc((uint64_t)babe->readout_cunt * (uint64_t)LDG_MATH_STAT_BABE_READOUT_STEP_CUNT, &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->spike_hist = (uint8_t *)ptr;

    if (babe->in_cunt > 0)
    {
        ret = ldg_mem_cpy(babe->in_class, conf->in_class, (uint64_t)babe->in_cunt);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    while (unit < babe->unit_cunt)
    {
        babe->region[unit] = (uint8_t)babe_region_compute(babe, unit);
        unit++;
    }

    babe_stencil_build(babe);
    babe_weight_wire(babe);

    babe->hash = babe_hash_compute(babe);
    babe->is_init = LDG_TRUTH_TRUE;

    return babe_dynamic_rst(babe);
}

uint32_t ldg_math_stat_babe_shutdown(ldg_math_stat_babe_t *babe)
{
    if (LDG_UNLIKELY(!babe)) { return LDG_ERR_FUNC_ARG_NULL; }

    babe_free(babe->spike_hist);
    babe_free(babe->in_class);
    babe_free(babe->region);
    babe_free(babe->trace_is);
    babe_free(babe->fired_now);
    babe_free(babe->stencil_inhibit_is);
    babe_free(babe->stencil_delay);
    babe_free(babe->stencil_dy);
    babe_free(babe->stencil_dx);
    babe_free(babe->stencil_dep);
    babe_free(babe->stencil_pot);
    babe_free(babe->hist);
    babe_free(babe->in_lane);
    babe_free(babe->last_fire);
    babe_free(babe->refrac);
    babe_free(babe->readout);
    babe_free(babe->excite);
    babe_free(babe->fx);
    babe_free(babe->field_buff);
    babe_free(babe->concentration);
    babe_free(babe->trace);
    babe_free(babe->weight);
    babe_free(babe->adapt);
    babe_free(babe->membrane);

    return ldg_mem_zero(babe, (uint64_t)sizeof(ldg_math_stat_babe_t));
}

uint32_t ldg_math_stat_babe_rst(ldg_math_stat_babe_t *babe)
{
    if (LDG_UNLIKELY(!babe)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return babe_dynamic_rst(babe);
}

uint32_t ldg_math_stat_babe_hash_get(const ldg_math_stat_babe_t *babe, uint32_t *out)
{
    if (LDG_UNLIKELY(!babe || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = babe->hash;

    return LDG_ERR_AOK;
}

static ldg_math_stat_babe_fixed_t* babe_field_get(const ldg_math_stat_babe_t *babe, uint32_t substance)
{
    return babe->concentration + ((uintptr_t)substance * (uintptr_t)LDG_MATH_STAT_BABE_REGION_CUNT);
}

static ldg_math_stat_babe_fixed_t* babe_field_buff_get(const ldg_math_stat_babe_t *babe, uint32_t substance)
{
    return babe->field_buff + ((uintptr_t)substance * (uintptr_t)LDG_MATH_STAT_BABE_REGION_CUNT);
}

static ldg_math_stat_babe_fixed_t* babe_excite_get(const ldg_math_stat_babe_t *babe, uint32_t unit_class, uint32_t region)
{
    return babe->excite + ((((uintptr_t)unit_class * (uintptr_t)LDG_MATH_STAT_BABE_REGION_CUNT) + (uintptr_t)region) * (uintptr_t)BABE_EXCITE_LANE_CUNT);
}

static void babe_cascade_step(ldg_math_stat_babe_t *babe)
{
    const ldg_math_stat_babe_species_t *species = babe->species;
    ldg_math_stat_babe_fixed_t *field = 0x0;
    const ldg_math_stat_babe_fixed_t *src_field = 0x0;
    uint32_t src = 0;
    uint32_t dst = 0;
    uint32_t region = 0;

    while (src < species->substance_cunt)
    {
        dst = 0;
        while (dst < species->substance_cunt)
        {
            if (species->cascade[src][dst] != BABE_ZERO_FIXED)
            {
                src_field = babe_field_get(babe, src);
                field = babe_field_get(babe, dst);
                region = 0;
                while (region < LDG_MATH_STAT_BABE_REGION_CUNT)
                {
                    field[region] = babe_sat_add(field[region], babe_fixed_mul(src_field[region], species->cascade[src][dst]));
                    region++;
                }
            }

            dst++;
        }

        src++;
    }
}

static void babe_diffuse_step(ldg_math_stat_babe_t *babe, uint32_t substance)
{
    const ldg_math_stat_babe_species_t *species = babe->species;
    ldg_math_stat_babe_fixed_t *field = babe_field_get(babe, substance);
    ldg_math_stat_babe_fixed_t *buff = babe_field_buff_get(babe, substance);
    ldg_math_stat_babe_fixed_t share = BABE_ZERO_FIXED;
    uint32_t region = 0;
    uint32_t row = 0;
    uint32_t col = 0;

    while (region < LDG_MATH_STAT_BABE_REGION_CUNT)
    {
        buff[region] = field[region];
        region++;
    }

    region = 0;
    while (region < LDG_MATH_STAT_BABE_REGION_CUNT)
    {
        share = buff[region] >> species->diffuse_shift;
        row = region / LDG_MATH_STAT_BABE_REGION_DIV;
        col = region % LDG_MATH_STAT_BABE_REGION_DIV;

        if (col + BABE_SPAN_CENTER < LDG_MATH_STAT_BABE_REGION_DIV)
        {
            field[region] = field[region] - share;
            field[region + BABE_SPAN_CENTER] = babe_sat_add(field[region + BABE_SPAN_CENTER], share);
        }

        if (col > 0)
        {
            field[region] = field[region] - share;
            field[region - BABE_SPAN_CENTER] = babe_sat_add(field[region - BABE_SPAN_CENTER], share);
        }

        if (row + BABE_SPAN_CENTER < LDG_MATH_STAT_BABE_REGION_DIV)
        {
            field[region] = field[region] - share;
            field[region + LDG_MATH_STAT_BABE_REGION_DIV] = babe_sat_add(field[region + LDG_MATH_STAT_BABE_REGION_DIV], share);
        }

        if (row > 0)
        {
            field[region] = field[region] - share;
            field[region - LDG_MATH_STAT_BABE_REGION_DIV] = babe_sat_add(field[region - LDG_MATH_STAT_BABE_REGION_DIV], share);
        }

        region++;
    }
}

static void babe_tolerance_step(ldg_math_stat_babe_t *babe, uint32_t substance)
{
    const ldg_math_stat_babe_species_t *species = babe->species;
    const ldg_math_stat_babe_fixed_t *field = babe_field_get(babe, substance);
    ldg_math_stat_babe_fixed_t mean = BABE_ZERO_FIXED;
    ldg_math_stat_babe_fixed_t gap = BABE_ZERO_FIXED;
    int64_t cum = 0;
    uint32_t region = 0;
    uint32_t kind = 0;

    while (region < LDG_MATH_STAT_BABE_REGION_CUNT)
    {
        cum = cum + (int64_t)field[region];
        region++;
    }

    mean = (ldg_math_stat_babe_fixed_t)(cum / (int64_t)LDG_MATH_STAT_BABE_REGION_CUNT);

    while (kind < species->class_cunt)
    {
        if (species->receptor[kind][substance] != BABE_ZERO_FIXED)
        {
            if (mean > species->tolerance_mark[substance])
            {
                gap = babe->receptor[kind][substance] >> species->tolerance_down_shift;
                babe->receptor[kind][substance] = babe->receptor[kind][substance] - gap;
            }
            else
            {
                gap = (species->receptor[kind][substance] - babe->receptor[kind][substance]) >> species->tolerance_up_shift;
                babe->receptor[kind][substance] = babe_sat_add(babe->receptor[kind][substance], gap);
            }
        }

        kind++;
    }
}

static void babe_fx_compute(ldg_math_stat_babe_t *babe)
{
    const ldg_math_stat_babe_species_t *species = babe->species;
    const ldg_math_stat_babe_fixed_t *field = 0x0;
    ldg_math_stat_babe_fixed_t *fx = 0x0;
    ldg_math_stat_babe_fixed_t *excite = 0x0;
    uint32_t kind = 0;
    uint32_t sub = 0;
    uint32_t region = 0;

    while (kind < species->class_cunt)
    {
        fx = babe->fx + ((uintptr_t)kind * (uintptr_t)LDG_MATH_STAT_BABE_REGION_CUNT);
        region = 0;
        while (region < LDG_MATH_STAT_BABE_REGION_CUNT)
        {
            fx[region] = BABE_ZERO_FIXED;
            region++;
        }

        sub = 0;
        while (sub < species->substance_cunt)
        {
            if (babe->receptor[kind][sub] != BABE_ZERO_FIXED)
            {
                field = babe_field_get(babe, sub);
                region = 0;
                while (region < LDG_MATH_STAT_BABE_REGION_CUNT)
                {
                    fx[region] = babe_sat_add(fx[region], babe_fixed_mul(field[region], babe->receptor[kind][sub]));
                    region++;
                }
            }

            sub++;
        }

        kind++;
    }

    kind = 0;
    while (kind < LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT)
    {
        region = 0;
        while (region < LDG_MATH_STAT_BABE_REGION_CUNT)
        {
            excite = babe_excite_get(babe, kind, region);
            excite[BABE_EXCITE_THRESHOLD] = BABE_ZERO_FIXED;
            excite[BABE_EXCITE_LEAK] = BABE_ZERO_FIXED;

            sub = 0;
            while (sub < species->substance_cunt)
            {
                field = babe_field_get(babe, sub);
                excite[BABE_EXCITE_THRESHOLD] = babe_sat_add(excite[BABE_EXCITE_THRESHOLD], babe_fixed_mul(field[region], babe->excite_threshold[kind][sub]));
                excite[BABE_EXCITE_LEAK] = babe_sat_add(excite[BABE_EXCITE_LEAK], babe_fixed_mul(field[region], babe->excite_leak[kind][sub]));
                sub++;
            }

            if (excite[BABE_EXCITE_LEAK] < BABE_ZERO_FIXED) { excite[BABE_EXCITE_LEAK] = BABE_ZERO_FIXED; }

            if (excite[BABE_EXCITE_LEAK] >= LDG_MATH_STAT_BABE_FIXED_ONE) { excite[BABE_EXCITE_LEAK] = BABE_FRAC_MASK; }

            region++;
        }

        kind++;
    }
}

static void babe_chemistry_step(ldg_math_stat_babe_t *babe)
{
    const ldg_math_stat_babe_species_t *species = babe->species;
    ldg_math_stat_babe_fixed_t *field = 0x0;
    uint32_t sub = 0;
    uint32_t region = 0;

    babe_cascade_step(babe);

    while (sub < species->substance_cunt)
    {
        field = babe_field_get(babe, sub);
        region = 0;
        while (region < LDG_MATH_STAT_BABE_REGION_CUNT)
        {
            field[region] = field[region] - (field[region] >> species->half_life_shift[sub]);
            region++;
        }

        babe_diffuse_step(babe, sub);
        babe_tolerance_step(babe, sub);
        sub++;
    }

    babe_fx_compute(babe);
}

static void babe_input_step(ldg_math_stat_babe_t *babe, const ldg_math_stat_babe_fixed_t *src)
{
    ldg_math_stat_babe_fixed_t rate = BABE_ZERO_FIXED;
    uint32_t lane = 0;

    while (lane < babe->in_cunt)
    {
        rate = src[lane];
        if (rate > LDG_MATH_STAT_BABE_FIXED_ONE) { rate = LDG_MATH_STAT_BABE_FIXED_ONE; }

        babe->fired_now[babe->unit_cunt + lane] = LDG_TRUTH_FALSE;
        if (rate > BABE_ZERO_FIXED && babe_frac_draw(babe) < rate)
        {
            babe->fired_now[babe->unit_cunt + lane] = LDG_TRUTH_TRUE;
            babe->last_fire[babe->unit_cunt + lane] = babe->step_cunt;
        }

        lane++;
    }
}

static uint32_t babe_slot_kind_get(const ldg_math_stat_babe_t *babe, uint32_t unit, uint32_t slot)
{
    const ldg_math_stat_babe_species_t *species = babe->species;

    if (slot >= babe->lateral_per_unit) { return babe->in_class[babe->in_lane[((uintptr_t)unit * (uintptr_t)babe->in_per_unit) + (uintptr_t)(slot - babe->lateral_per_unit)]]; }

    if (unit < babe->readout_cunt) { return species->motor_class; }

    return species->lateral_class;
}

static void babe_stdp_apply(ldg_math_stat_babe_t *babe, uint32_t unit)
{
    const ldg_math_stat_babe_species_t *species = babe->species;
    ldg_math_stat_babe_fixed_t *trace = babe->trace + ((uintptr_t)unit * (uintptr_t)babe->slot_cunt);
    const uint16_t *lane = babe->in_lane + ((uintptr_t)unit * (uintptr_t)babe->in_per_unit);
    uint32_t pot_mask = LDG_BIT(BABE_DELAY_INPUT);
    uint32_t hist = 0;
    uint32_t pot = 0;
    uint32_t dep = 0;
    uint32_t neighbor = 0;
    uint32_t slot = 0;
    uint32_t bit = 0;

    while (bit <= species->stdp_window)
    {
        pot_mask = pot_mask | LDG_BIT(bit);
        bit++;
    }

    while (slot < babe->lateral_per_unit)
    {
        if (babe_neighbor_get(babe, unit, slot, &neighbor) == LDG_TRUTH_TRUE)
        {
            hist = babe->hist[neighbor];
            pot = babe_pop_get(hist & babe->stencil_pot[slot]);
            dep = babe_pop_get(hist & babe->stencil_dep[slot]);
            if (babe->fired_now[neighbor] == LDG_TRUTH_TRUE && (uint32_t)babe->stencil_delay[slot] + BABE_ARRIVAL_NEXT <= species->stdp_window) { dep++; }

            if (pot > dep) { trace[slot] = babe_sat_add(trace[slot], (ldg_math_stat_babe_fixed_t)((int64_t)species->stdp_step * (int64_t)(pot - dep))); }
            else if (dep > pot) { trace[slot] = babe_sat_add(trace[slot], (ldg_math_stat_babe_fixed_t)(-(int64_t)species->stdp_step * (int64_t)(dep - pot))); }
        }

        slot++;
    }

    slot = 0;
    while (slot < babe->in_per_unit)
    {
        hist = babe->hist[babe->unit_cunt + lane[slot]];
        pot = babe_pop_get(hist & pot_mask);
        dep = (babe->fired_now[babe->unit_cunt + lane[slot]] == LDG_TRUTH_TRUE && BABE_ARRIVAL_NEXT <= species->stdp_window) ? BABE_ARRIVAL_NEXT : BABE_DELAY_INPUT;

        if (pot > dep) { trace[babe->lateral_per_unit + slot] = babe_sat_add(trace[babe->lateral_per_unit + slot], (ldg_math_stat_babe_fixed_t)((int64_t)species->stdp_step * (int64_t)(pot - dep))); }
        else if (dep > pot) { trace[babe->lateral_per_unit + slot] = babe_sat_add(trace[babe->lateral_per_unit + slot], -species->stdp_step); }

        slot++;
    }

    babe->trace_is[unit] = LDG_TRUTH_TRUE;
}

static ldg_math_stat_babe_fixed_t babe_incoming_get(const ldg_math_stat_babe_t *babe, uint32_t unit)
{
    const ldg_math_stat_babe_fixed_t *weight = babe->weight + ((uintptr_t)unit * (uintptr_t)babe->slot_cunt);
    const uint16_t *lane = babe->in_lane + ((uintptr_t)unit * (uintptr_t)babe->in_per_unit);
    ldg_math_stat_babe_fixed_t cum = BABE_ZERO_FIXED;
    uint32_t neighbor = 0;
    uint32_t slot = 0;

    while (slot < babe->lateral_per_unit)
    {
        if (weight[slot] != BABE_ZERO_FIXED && babe_neighbor_get(babe, unit, slot, &neighbor) == LDG_TRUTH_TRUE) { if ((babe->hist[neighbor] & LDG_BIT(babe->stencil_delay[slot])) != 0) { cum = babe_sat_add(cum, weight[slot]); } }

        slot++;
    }

    slot = 0;
    while (slot < babe->in_per_unit)
    {
        if ((babe->hist[babe->unit_cunt + lane[slot]] & LDG_BIT(BABE_DELAY_INPUT)) != 0) { cum = babe_sat_add(cum, weight[babe->lateral_per_unit + slot]); }

        slot++;
    }

    return cum;
}

static void babe_unit_update(ldg_math_stat_babe_t *babe, uint32_t unit, uint32_t now, uint32_t hist_slot)
{
    const ldg_math_stat_babe_species_t *species = babe->species;
    const ldg_math_stat_babe_fixed_t *excite = babe_excite_get(babe, LDG_MATH_STAT_BABE_UNIT_GRID, babe->region[unit]);
    ldg_math_stat_babe_fixed_t membrane = babe->membrane[unit];
    ldg_math_stat_babe_fixed_t adapt = babe->adapt[unit];
    ldg_math_stat_babe_fixed_t threshold = BABE_ZERO_FIXED;
    ldg_math_stat_babe_fixed_t snap = (ldg_math_stat_babe_fixed_t)LDG_BIT(species->leak_shift);

    membrane = membrane - (membrane >> species->leak_shift);
    if (excite[BABE_EXCITE_LEAK] > BABE_ZERO_FIXED) { membrane = membrane - babe_fixed_mul(membrane, excite[BABE_EXCITE_LEAK]); }

    if (membrane < snap && membrane > -snap) { membrane = BABE_ZERO_FIXED; }

    if (babe->refrac[unit] > 0) { babe->refrac[unit]--; }
    else { membrane = babe_sat_add(membrane, babe_incoming_get(babe, unit)); }

    adapt = adapt - (adapt >> species->adapt_shift);
    if (adapt < (ldg_math_stat_babe_fixed_t)LDG_BIT(species->adapt_shift)) { adapt = BABE_ZERO_FIXED; }

    babe->adapt[unit] = adapt;
    babe->fired_now[unit] = LDG_TRUTH_FALSE;

    threshold = babe_sat_add(babe_sat_add(species->threshold, adapt), excite[BABE_EXCITE_THRESHOLD]);

    if (babe->refrac[unit] == 0 && membrane >= threshold)
    {
        membrane = species->rst_val;
        babe->refrac[unit] = species->refrac_step_cunt;
        babe->adapt[unit] = babe_sat_add(adapt, species->adapt_step);
        babe->fired_now[unit] = LDG_TRUTH_TRUE;
        babe->fired_cunt++;
        babe->last_fire[unit] = now;

        if (unit < babe->readout_cunt) { babe->spike_hist[((uintptr_t)unit * (uintptr_t)LDG_MATH_STAT_BABE_READOUT_STEP_CUNT) + hist_slot] = LDG_TRUTH_TRUE; }

        babe_stdp_apply(babe, unit);
    }

    babe->membrane[unit] = membrane;
}

static void babe_unit_step(ldg_math_stat_babe_t *babe)
{
    uint32_t now = babe->step_cunt;
    uint32_t hist_slot = now & BABE_HIST_MASK;
    uint32_t unit = 0;

    while (unit < babe->readout_cunt)
    {
        babe->spike_hist[((uintptr_t)unit * (uintptr_t)LDG_MATH_STAT_BABE_READOUT_STEP_CUNT) + hist_slot] = LDG_TRUTH_FALSE;
        unit++;
    }

    babe->fired_cunt = 0;

    unit = 0;
    while (unit < babe->unit_cunt)
    {
        babe_unit_update(babe, unit, now, hist_slot);
        unit++;
    }
}

static void babe_hist_step(ldg_math_stat_babe_t *babe)
{
    uintptr_t src_cunt = (uintptr_t)babe->unit_cunt + (uintptr_t)babe->in_cunt;
    uintptr_t idx = 0;

    while (idx < src_cunt)
    {
        babe->hist[idx] = (uint16_t)((((uint32_t)babe->hist[idx] << BABE_HIST_NEXT) | (uint32_t)babe->fired_now[idx]) & BABE_HIST_WORD_MASK);
        idx++;
    }
}

static uint8_t babe_unit_plasticity_step(ldg_math_stat_babe_t *babe, uint32_t unit)
{
    const ldg_math_stat_babe_species_t *species = babe->species;
    ldg_math_stat_babe_fixed_t *trace = babe->trace + ((uintptr_t)unit * (uintptr_t)babe->slot_cunt);
    ldg_math_stat_babe_fixed_t *weight = babe->weight + ((uintptr_t)unit * (uintptr_t)babe->slot_cunt);
    const ldg_math_stat_babe_fixed_t *fx = babe->fx + (uintptr_t)babe->region[unit];
    ldg_math_stat_babe_fixed_t val = BABE_ZERO_FIXED;
    ldg_math_stat_babe_fixed_t snap = BABE_ZERO_FIXED;
    ldg_math_stat_babe_fixed_t gain = BABE_ZERO_FIXED;
    uint32_t slot = 0;
    uint32_t kind = 0;
    uint8_t alive_is = LDG_TRUTH_FALSE;

    while (slot < babe->slot_cunt)
    {
        val = trace[slot];

        if (val != BABE_NO_TRACE)
        {
            kind = babe_slot_kind_get(babe, unit, slot);
            val = val - (val >> species->trace_shift[kind]);
            snap = (ldg_math_stat_babe_fixed_t)LDG_BIT(species->trace_shift[kind]);
            if (snap < species->trace_floor) { snap = species->trace_floor; }

            if (val < snap && val > -snap) { val = BABE_NO_TRACE; }

            trace[slot] = val;
            if (val != BABE_NO_TRACE) { alive_is = LDG_TRUTH_TRUE; }

            gain = fx[(uintptr_t)kind * (uintptr_t)LDG_MATH_STAT_BABE_REGION_CUNT];

            if (gain != BABE_ZERO_FIXED && val != BABE_NO_TRACE && weight[slot] != BABE_ZERO_FIXED)
            {
                val = babe_sat_add(weight[slot], babe_mul_shift(val, gain, species->lr_shift));

                if (slot < babe->lateral_per_unit && babe->stencil_inhibit_is[slot] == LDG_TRUTH_TRUE) { if (val > BABE_ZERO_FIXED) { val = BABE_ZERO_FIXED; } }
                else if (val < BABE_ZERO_FIXED) { val = BABE_ZERO_FIXED; }

                weight[slot] = val;
            }
        }

        slot++;
    }

    return alive_is;
}

static void babe_plasticity_step(ldg_math_stat_babe_t *babe)
{
    uint32_t unit = 0;

    while (unit < babe->unit_cunt)
    {
        if (babe->trace_is[unit] == LDG_TRUTH_TRUE) { babe->trace_is[unit] = babe_unit_plasticity_step(babe, unit); }

        unit++;
    }
}

static void babe_readout_step(ldg_math_stat_babe_t *babe)
{
    const uint8_t *hist = babe->spike_hist;
    uint32_t unit = 0;
    uint32_t slot = 0;
    uint32_t cunt = 0;

    while (unit < babe->readout_cunt)
    {
        cunt = 0;
        slot = 0;
        while (slot < LDG_MATH_STAT_BABE_READOUT_STEP_CUNT)
        {
            cunt = cunt + hist[slot];
            slot++;
        }

        babe->readout[unit] = (ldg_math_stat_babe_fixed_t)(cunt << LDG_MATH_STAT_BABE_FIXED_SHIFT);
        hist = hist + LDG_MATH_STAT_BABE_READOUT_STEP_CUNT;
        unit++;
    }
}

uint32_t ldg_math_stat_babe_step(ldg_math_stat_babe_t *babe, const ldg_math_stat_babe_fixed_t *src, ldg_math_stat_babe_fixed_t *dst, uint32_t dst_cap)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!babe || !dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(babe->in_cunt > 0 && !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(dst_cap < babe->readout_cunt)) { return LDG_ERR_BOUNDS; }

    babe_chemistry_step(babe);
    babe_input_step(babe, src);
    babe_unit_step(babe);
    babe_plasticity_step(babe);
    babe_hist_step(babe);
    babe_readout_step(babe);

    ret = ldg_mem_cpy(dst, babe->readout, (uint64_t)babe->readout_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->step_cunt++;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_concentration_add(ldg_math_stat_babe_t *babe, uint32_t substance, uint32_t region, ldg_math_stat_babe_fixed_t amount)
{
    ldg_math_stat_babe_fixed_t *field = 0x0;

    if (LDG_UNLIKELY(!babe)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(substance >= babe->species->substance_cunt || region >= LDG_MATH_STAT_BABE_REGION_CUNT)) { return LDG_ERR_BOUNDS; }

    field = babe_field_get(babe, substance);
    field[region] = babe_sat_add(field[region], amount);
    if (field[region] < BABE_ZERO_FIXED) { field[region] = BABE_ZERO_FIXED; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_concentration_get(const ldg_math_stat_babe_t *babe, uint32_t substance, uint32_t region, ldg_math_stat_babe_fixed_t *out)
{
    if (LDG_UNLIKELY(!babe || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = BABE_ZERO_FIXED;

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(substance >= babe->species->substance_cunt || region >= LDG_MATH_STAT_BABE_REGION_CUNT)) { return LDG_ERR_BOUNDS; }

    *out = babe_field_get(babe, substance)[region];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_receptor_get(const ldg_math_stat_babe_t *babe, uint32_t kind, uint32_t substance, ldg_math_stat_babe_fixed_t *out)
{
    if (LDG_UNLIKELY(!babe || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = BABE_ZERO_FIXED;

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(kind >= babe->species->class_cunt || substance >= babe->species->substance_cunt)) { return LDG_ERR_BOUNDS; }

    *out = babe->receptor[kind][substance];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_unit_get(const ldg_math_stat_babe_t *babe, uint32_t unit, ldg_math_stat_babe_fixed_t *out_membrane, ldg_math_stat_babe_fixed_t *out_adapt, uint32_t *out_refrac, uint32_t *out_last_fire)
{
    if (LDG_UNLIKELY(!babe || !out_membrane || !out_adapt || !out_refrac || !out_last_fire)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_membrane = BABE_ZERO_FIXED;
    *out_adapt = BABE_ZERO_FIXED;
    *out_refrac = 0;
    *out_last_fire = LDG_MATH_STAT_BABE_FIRE_NONE;

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(unit >= babe->unit_cunt)) { return LDG_ERR_BOUNDS; }

    *out_membrane = babe->membrane[unit];
    *out_adapt = babe->adapt[unit];
    *out_refrac = babe->refrac[unit];
    *out_last_fire = babe->last_fire[unit];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_unit_span_get(const ldg_math_stat_babe_t *babe, uint32_t unit, uint32_t *out_first, uint32_t *out_cunt)
{
    if (LDG_UNLIKELY(!babe || !out_first || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_first = 0;
    *out_cunt = 0;

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(unit >= babe->unit_cunt)) { return LDG_ERR_BOUNDS; }

    *out_first = unit * babe->slot_cunt;
    *out_cunt = babe->slot_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_synapse_get(const ldg_math_stat_babe_t *babe, uint32_t idx, ldg_math_stat_babe_synapse_t *out)
{
    uint32_t unit = 0;
    uint32_t slot = 0;

    if (LDG_UNLIKELY(!babe || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(idx >= babe->synapse_cunt)) { return LDG_ERR_BOUNDS; }

    unit = idx / babe->slot_cunt;
    slot = idx % babe->slot_cunt;

    out->weight = babe->weight[idx];
    out->trace = babe->trace[idx];
    out->dst = unit;
    out->delay = BABE_DELAY_INPUT;
    out->kind = babe_slot_kind_get(babe, unit, slot);
    out->inhibit_is = LDG_TRUTH_FALSE;

    if (slot < babe->lateral_per_unit)
    {
        out->delay = babe->stencil_delay[slot];
        out->inhibit_is = babe->stencil_inhibit_is[slot];
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_region_get(const ldg_math_stat_babe_t *babe, uint32_t unit, uint32_t *out)
{
    if (LDG_UNLIKELY(!babe || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(unit >= babe->unit_cunt)) { return LDG_ERR_BOUNDS; }

    *out = babe->region[unit];

    return LDG_ERR_AOK;
}

static uintptr_t babe_table_cunt_get(void)
{
    return ((uintptr_t)LDG_MATH_STAT_BABE_CLASS_MAX * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX) + ((uintptr_t)LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX * (uintptr_t)BABE_EXCITE_LANE_CUNT);
}

static uintptr_t babe_state_cunt_compute(const ldg_math_stat_babe_t *babe)
{
    uintptr_t src_cunt = (uintptr_t)babe->unit_cunt + (uintptr_t)babe->in_cunt;
    uintptr_t cunt = BABE_STATE_HDR_CUNT;

    cunt = cunt + ((uintptr_t)babe->unit_cunt * (uintptr_t)(BABE_EXCITE_LANE_CUNT + BABE_SPAN_CENTER));
    cunt = cunt + (uintptr_t)babe->synapse_cunt;
    cunt = cunt + (src_cunt * (uintptr_t)BABE_EXCITE_LANE_CUNT);
    cunt = cunt + ((uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX * (uintptr_t)LDG_MATH_STAT_BABE_REGION_CUNT);
    cunt = cunt + babe_table_cunt_get();
    cunt = cunt + ((uintptr_t)babe->readout_cunt * (uintptr_t)LDG_MATH_STAT_BABE_READOUT_STEP_CUNT);

    return cunt;
}

uint32_t ldg_math_stat_babe_state_cunt_get(const ldg_math_stat_babe_t *babe, uintptr_t *out)
{
    if (LDG_UNLIKELY(!babe || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = babe_state_cunt_compute(babe);

    return LDG_ERR_AOK;
}

static ldg_math_stat_babe_fixed_t* babe_fixed_st(ldg_math_stat_babe_fixed_t *dst, const ldg_math_stat_babe_fixed_t *src, uintptr_t cunt)
{
    const ldg_math_stat_babe_fixed_t *end = src + cunt;

    while (src < end)
    {
        *dst = *src;
        dst++;
        src++;
    }

    return dst;
}

static const ldg_math_stat_babe_fixed_t* babe_fixed_ld(ldg_math_stat_babe_fixed_t *dst, const ldg_math_stat_babe_fixed_t *src, uintptr_t cunt)
{
    const ldg_math_stat_babe_fixed_t *end = src + cunt;

    while (src < end)
    {
        *dst = *src;
        dst++;
        src++;
    }

    return src;
}

static ldg_math_stat_babe_fixed_t* babe_uint_st(ldg_math_stat_babe_fixed_t *dst, const uint32_t *src, uintptr_t cunt)
{
    const uint32_t *end = src + cunt;

    while (src < end)
    {
        *dst = (ldg_math_stat_babe_fixed_t)*src;
        dst++;
        src++;
    }

    return dst;
}

static const ldg_math_stat_babe_fixed_t* babe_uint_ld(uint32_t *dst, const ldg_math_stat_babe_fixed_t *src, uintptr_t cunt)
{
    const ldg_math_stat_babe_fixed_t *end = src + cunt;

    while (src < end)
    {
        *dst = (uint32_t)*src;
        dst++;
        src++;
    }

    return src;
}

static ldg_math_stat_babe_fixed_t* babe_word_st(ldg_math_stat_babe_fixed_t *dst, const uint16_t *src, uintptr_t cunt)
{
    const uint16_t *end = src + cunt;

    while (src < end)
    {
        *dst = (ldg_math_stat_babe_fixed_t)*src;
        dst++;
        src++;
    }

    return dst;
}

static const ldg_math_stat_babe_fixed_t* babe_word_ld(uint16_t *dst, const ldg_math_stat_babe_fixed_t *src, uintptr_t cunt)
{
    const ldg_math_stat_babe_fixed_t *end = src + cunt;

    while (src < end)
    {
        *dst = (uint16_t)((uint32_t)*src & BABE_HIST_WORD_MASK);
        dst++;
        src++;
    }

    return src;
}

static ldg_math_stat_babe_fixed_t* babe_byte_st(ldg_math_stat_babe_fixed_t *dst, const uint8_t *src, uintptr_t cunt)
{
    const uint8_t *end = src + cunt;

    while (src < end)
    {
        *dst = (ldg_math_stat_babe_fixed_t)*src;
        dst++;
        src++;
    }

    return dst;
}

static const ldg_math_stat_babe_fixed_t* babe_byte_ld(uint8_t *dst, const ldg_math_stat_babe_fixed_t *src, uintptr_t cunt)
{
    const ldg_math_stat_babe_fixed_t *end = src + cunt;

    while (src < end)
    {
        *dst = (uint8_t)*src;
        dst++;
        src++;
    }

    return src;
}

static ldg_math_stat_babe_fixed_t* babe_table_st(ldg_math_stat_babe_fixed_t *dst, const ldg_math_stat_babe_fixed_t receptor[LDG_MATH_STAT_BABE_CLASS_MAX][LDG_MATH_STAT_BABE_SUBSTANCE_MAX], const ldg_math_stat_babe_fixed_t excite_threshold[LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT][LDG_MATH_STAT_BABE_SUBSTANCE_MAX], const ldg_math_stat_babe_fixed_t excite_leak[LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT][LDG_MATH_STAT_BABE_SUBSTANCE_MAX])
{
    dst = babe_fixed_st(dst, &receptor[0][0], (uintptr_t)LDG_MATH_STAT_BABE_CLASS_MAX * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX);
    dst = babe_fixed_st(dst, &excite_threshold[0][0], (uintptr_t)LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX);

    return babe_fixed_st(dst, &excite_leak[0][0], (uintptr_t)LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX);
}

static const ldg_math_stat_babe_fixed_t* babe_table_ld(const ldg_math_stat_babe_fixed_t *src, ldg_math_stat_babe_fixed_t receptor[LDG_MATH_STAT_BABE_CLASS_MAX][LDG_MATH_STAT_BABE_SUBSTANCE_MAX], ldg_math_stat_babe_fixed_t excite_threshold[LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT][LDG_MATH_STAT_BABE_SUBSTANCE_MAX], ldg_math_stat_babe_fixed_t excite_leak[LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT][LDG_MATH_STAT_BABE_SUBSTANCE_MAX])
{
    src = babe_fixed_ld(&receptor[0][0], src, (uintptr_t)LDG_MATH_STAT_BABE_CLASS_MAX * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX);
    src = babe_fixed_ld(&excite_threshold[0][0], src, (uintptr_t)LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX);

    return babe_fixed_ld(&excite_leak[0][0], src, (uintptr_t)LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX);
}

uint32_t ldg_math_stat_babe_state_get(const ldg_math_stat_babe_t *babe, ldg_math_stat_babe_fixed_t *dst, uintptr_t cap, uintptr_t *out_cunt)
{
    uintptr_t src_cunt = 0;
    uintptr_t need = 0;

    if (LDG_UNLIKELY(!babe || !dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    need = babe_state_cunt_compute(babe);
    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    src_cunt = (uintptr_t)babe->unit_cunt + (uintptr_t)babe->in_cunt;

    dst[BABE_STATE_HDR_HASH] = (ldg_math_stat_babe_fixed_t)babe->hash;
    dst[BABE_STATE_HDR_STEP] = (ldg_math_stat_babe_fixed_t)babe->step_cunt;
    dst = dst + BABE_STATE_HDR_CUNT;

    dst = babe_fixed_st(dst, babe->membrane, (uintptr_t)babe->unit_cunt);
    dst = babe_fixed_st(dst, babe->adapt, (uintptr_t)babe->unit_cunt);
    dst = babe_uint_st(dst, babe->refrac, (uintptr_t)babe->unit_cunt);
    dst = babe_fixed_st(dst, babe->trace, (uintptr_t)babe->synapse_cunt);
    dst = babe_uint_st(dst, babe->last_fire, src_cunt);
    dst = babe_word_st(dst, babe->hist, src_cunt);
    dst = babe_fixed_st(dst, babe->concentration, (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX * (uintptr_t)LDG_MATH_STAT_BABE_REGION_CUNT);
    dst = babe_table_st(dst, babe->receptor, babe->excite_threshold, babe->excite_leak);
    babe_byte_st(dst, babe->spike_hist, (uintptr_t)babe->readout_cunt * (uintptr_t)LDG_MATH_STAT_BABE_READOUT_STEP_CUNT);

    *out_cunt = need;

    return LDG_ERR_AOK;
}

static void babe_trace_is_rebuild(ldg_math_stat_babe_t *babe)
{
    const ldg_math_stat_babe_fixed_t *trace = babe->trace;
    uint32_t unit = 0;
    uint32_t slot = 0;

    while (unit < babe->unit_cunt)
    {
        babe->trace_is[unit] = LDG_TRUTH_FALSE;

        slot = 0;
        while (slot < babe->slot_cunt)
        {
            if (trace[slot] != BABE_NO_TRACE)
            {
                babe->trace_is[unit] = LDG_TRUTH_TRUE;
                break;
            }

            slot++;
        }

        trace = trace + babe->slot_cunt;
        unit++;
    }
}

uint32_t ldg_math_stat_babe_state_set(ldg_math_stat_babe_t *babe, const ldg_math_stat_babe_fixed_t *src, uintptr_t cunt)
{
    uintptr_t src_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!babe || !src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(cunt != babe_state_cunt_compute(babe))) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY((uint32_t)src[BABE_STATE_HDR_HASH] != babe->hash)) { return LDG_ERR_INVALID; }

    src_cunt = (uintptr_t)babe->unit_cunt + (uintptr_t)babe->in_cunt;

    babe->step_cunt = (uint32_t)src[BABE_STATE_HDR_STEP];
    src = src + BABE_STATE_HDR_CUNT;

    src = babe_fixed_ld(babe->membrane, src, (uintptr_t)babe->unit_cunt);
    src = babe_fixed_ld(babe->adapt, src, (uintptr_t)babe->unit_cunt);
    src = babe_uint_ld(babe->refrac, src, (uintptr_t)babe->unit_cunt);
    src = babe_fixed_ld(babe->trace, src, (uintptr_t)babe->synapse_cunt);
    src = babe_uint_ld(babe->last_fire, src, src_cunt);
    src = babe_word_ld(babe->hist, src, src_cunt);
    src = babe_fixed_ld(babe->concentration, src, (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX * (uintptr_t)LDG_MATH_STAT_BABE_REGION_CUNT);
    src = babe_table_ld(src, babe->receptor, babe->excite_threshold, babe->excite_leak);
    babe_byte_ld(babe->spike_hist, src, (uintptr_t)babe->readout_cunt * (uintptr_t)LDG_MATH_STAT_BABE_READOUT_STEP_CUNT);

    ret = ldg_mem_zero(babe->fired_now, (uint64_t)src_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe->fired_cunt = 0;
    babe_trace_is_rebuild(babe);
    babe_readout_step(babe);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_proto_init(ldg_math_stat_babe_proto_t *proto, const ldg_math_stat_babe_t *babe)
{
    void *ptr = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!proto || !babe)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_mem_zero(proto, (uint64_t)sizeof(ldg_math_stat_babe_proto_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = babe_alloc((uint64_t)babe->synapse_cunt * (uint64_t)sizeof(ldg_math_stat_babe_fixed_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    proto->weight = (ldg_math_stat_babe_fixed_t *)ptr;
    proto->synapse_cunt = babe->synapse_cunt;
    proto->hash = babe->hash;
    proto->is_init = LDG_TRUTH_TRUE;

    return ldg_math_stat_babe_proto_capture(proto, babe);
}

uint32_t ldg_math_stat_babe_proto_shutdown(ldg_math_stat_babe_proto_t *proto)
{
    if (LDG_UNLIKELY(!proto)) { return LDG_ERR_FUNC_ARG_NULL; }

    babe_free(proto->weight);

    return ldg_mem_zero(proto, (uint64_t)sizeof(ldg_math_stat_babe_proto_t));
}

static uint32_t babe_proto_match_ck(const ldg_math_stat_babe_proto_t *proto, const ldg_math_stat_babe_t *babe)
{
    if (LDG_UNLIKELY(proto->is_init != LDG_TRUTH_TRUE || babe->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(proto->hash != babe->hash || proto->synapse_cunt != babe->synapse_cunt)) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_proto_capture(ldg_math_stat_babe_proto_t *proto, const ldg_math_stat_babe_t *babe)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!proto || !babe)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = babe_proto_match_ck(proto, babe);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe_fixed_st(proto->weight, babe->weight, (uintptr_t)babe->synapse_cunt);
    babe_fixed_st(&proto->receptor[0][0], &babe->receptor[0][0], (uintptr_t)LDG_MATH_STAT_BABE_CLASS_MAX * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX);
    babe_fixed_st(&proto->excite_threshold[0][0], &babe->excite_threshold[0][0], (uintptr_t)LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX);
    babe_fixed_st(&proto->excite_leak[0][0], &babe->excite_leak[0][0], (uintptr_t)LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX);

    return LDG_ERR_AOK;
}

static ldg_math_stat_babe_fixed_t babe_weight_clip(const ldg_math_stat_babe_t *babe, uint32_t idx, ldg_math_stat_babe_fixed_t weight)
{
    uint32_t slot = idx % babe->slot_cunt;

    if (slot < babe->lateral_per_unit && babe->stencil_inhibit_is[slot] == LDG_TRUTH_TRUE)
    {
        if (weight > BABE_ZERO_FIXED) { return BABE_ZERO_FIXED; }

        return weight;
    }

    if (weight < BABE_ZERO_FIXED) { return BABE_ZERO_FIXED; }

    return weight;
}

uint32_t ldg_math_stat_babe_inherit(ldg_math_stat_babe_t *babe, const ldg_math_stat_babe_proto_t *proto, ldg_math_stat_babe_fixed_t variation)
{
    ldg_math_stat_babe_fixed_t signed_frac = BABE_ZERO_FIXED;
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!babe || !proto)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = babe_proto_match_ck(proto, babe);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(variation < BABE_ZERO_FIXED)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (idx < babe->synapse_cunt)
    {
        babe->weight[idx] = proto->weight[idx];
        if (proto->weight[idx] != BABE_ZERO_FIXED)
        {
            signed_frac = (babe_frac_draw(babe) << BABE_FRAC_SIGN_SHIFT) - LDG_MATH_STAT_BABE_FIXED_ONE;
            babe->weight[idx] = babe_weight_clip(babe, idx, babe_sat_add(proto->weight[idx], babe_fixed_mul(variation, signed_frac)));
        }

        idx++;
    }

    ret = babe_dynamic_rst(babe);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe_fixed_st(&babe->receptor[0][0], &proto->receptor[0][0], (uintptr_t)LDG_MATH_STAT_BABE_CLASS_MAX * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX);
    babe_fixed_st(&babe->excite_threshold[0][0], &proto->excite_threshold[0][0], (uintptr_t)LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX);
    babe_fixed_st(&babe->excite_leak[0][0], &proto->excite_leak[0][0], (uintptr_t)LDG_MATH_STAT_BABE_UNIT_CLASS_CUNT * (uintptr_t)LDG_MATH_STAT_BABE_SUBSTANCE_MAX);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_delta_get(const ldg_math_stat_babe_t *babe, const ldg_math_stat_babe_proto_t *proto, uint32_t *idx_dst, ldg_math_stat_babe_fixed_t *val_dst, uint32_t cap, uint32_t *out_cunt)
{
    uint32_t idx = 0;
    uint32_t cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!babe || !proto || !idx_dst || !val_dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = babe_proto_match_ck(proto, babe);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (idx < babe->synapse_cunt)
    {
        if (babe->weight[idx] != proto->weight[idx])
        {
            if (LDG_UNLIKELY(cunt >= cap)) { return LDG_ERR_BOUNDS; }

            idx_dst[cunt] = idx;
            val_dst[cunt] = babe->weight[idx];
            cunt++;
        }

        idx++;
    }

    *out_cunt = cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_delta_set(ldg_math_stat_babe_t *babe, const ldg_math_stat_babe_proto_t *proto, const uint32_t *idx_src, const ldg_math_stat_babe_fixed_t *val_src, uint32_t cunt)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!babe || !proto)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt > 0 && (!idx_src || !val_src))) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = babe_proto_match_ck(proto, babe);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    babe_fixed_st(babe->weight, proto->weight, (uintptr_t)babe->synapse_cunt);

    while (idx < cunt)
    {
        if (LDG_UNLIKELY(idx_src[idx] >= babe->synapse_cunt)) { return LDG_ERR_BOUNDS; }

        babe->weight[idx_src[idx]] = babe_weight_clip(babe, idx_src[idx], val_src[idx]);
        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_proto_state_cunt_get(const ldg_math_stat_babe_proto_t *proto, uintptr_t *out)
{
    if (LDG_UNLIKELY(!proto || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(proto->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    *out = (uintptr_t)BABE_PROTO_HDR_CUNT + (uintptr_t)proto->synapse_cunt + babe_table_cunt_get();

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_proto_state_get(const ldg_math_stat_babe_proto_t *proto, ldg_math_stat_babe_fixed_t *dst, uintptr_t cap, uintptr_t *out_cunt)
{
    uintptr_t need = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = ldg_math_stat_babe_proto_state_cunt_get(proto, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    dst[BABE_STATE_HDR_HASH] = (ldg_math_stat_babe_fixed_t)proto->hash;
    dst[BABE_STATE_HDR_STEP] = (ldg_math_stat_babe_fixed_t)proto->synapse_cunt;
    dst = dst + BABE_PROTO_HDR_CUNT;

    dst = babe_fixed_st(dst, proto->weight, (uintptr_t)proto->synapse_cunt);
    babe_table_st(dst, proto->receptor, proto->excite_threshold, proto->excite_leak);

    *out_cunt = need;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_babe_proto_state_set(ldg_math_stat_babe_proto_t *proto, const ldg_math_stat_babe_fixed_t *src, uintptr_t cunt)
{
    uintptr_t need = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_stat_babe_proto_state_cunt_get(proto, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cunt != need)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY((uint32_t)src[BABE_STATE_HDR_HASH] != proto->hash || (uint32_t)src[BABE_STATE_HDR_STEP] != proto->synapse_cunt)) { return LDG_ERR_INVALID; }

    src = src + BABE_PROTO_HDR_CUNT;
    src = babe_fixed_ld(proto->weight, src, (uintptr_t)proto->synapse_cunt);
    babe_table_ld(src, proto->receptor, proto->excite_threshold, proto->excite_leak);

    return LDG_ERR_AOK;
}
