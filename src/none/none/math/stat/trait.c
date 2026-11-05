#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>

#define TRAIT_CAP_DEFAULT 16
#define TRAIT_ROW_CAP_DEFAULT 256
#define TRAIT_MIN_DEFAULT 0.0
#define TRAIT_MAX_DEFAULT 1.0
#define TRAIT_MUTATE_SCALE_DEFAULT 0.0625
#define TRAIT_MUTATE_RATE_DEFAULT 0.0625
#define TRAIT_RATE_MAX 1.0
#define TRAIT_UNIT_SHIFT 11
#define TRAIT_UNIT_SCALE 9007199254740992.0
#define TRAIT_SIGNED_SPAN 2.0
#define TRAIT_SIGNED_OFF 1.0
#define TRAIT_MEAN_WEIGHT 0.5
#define TRAIT_ALIVE_MIN 0.5
#define TRAIT_PICK_BIT 0x1U
#define TRAIT_CUT_INCLUSIVE 1
#define TRAIT_NONE 0.0
#define TRAIT_FIRST 0
#define TRAIT_STATE_HDR_CUNT 2
#define TRAIT_STATE_HDR_TRAIT 0
#define TRAIT_STATE_HDR_ROW 1
#define TRAIT_DESC_LANE_CUNT 6
#define TRAIT_DESC_LANE_MIN 0
#define TRAIT_DESC_LANE_MAX 1
#define TRAIT_DESC_LANE_SCALE 2
#define TRAIT_DESC_LANE_RATE 3
#define TRAIT_DESC_LANE_MIX 4
#define TRAIT_DESC_LANE_DOM 5
#define TRAIT_ROW_HDR_CUNT 1

static double trait_mean_get(double lhs, double rhs)
{
    return (lhs + rhs) * TRAIT_MEAN_WEIGHT;
}

static double trait_clamp(const ldg_math_stat_trait_desc_t *desc, double val)
{
    if (val < desc->min) { return desc->min; }

    if (val > desc->max) { return desc->max; }

    return val;
}

static uint32_t trait_draw(const ldg_math_stat_trait_table_t *table)
{
    return (uint32_t)table->rng_cb(table->rng_ctx);
}

static double trait_unit_draw(const ldg_math_stat_trait_table_t *table)
{
    return (double)(table->rng_cb(table->rng_ctx) >> TRAIT_UNIT_SHIFT) / TRAIT_UNIT_SCALE;
}

static double trait_signed_unit_draw(const ldg_math_stat_trait_table_t *table)
{
    return (trait_unit_draw(table) * TRAIT_SIGNED_SPAN) - TRAIT_SIGNED_OFF;
}

static double* trait_allele_ptr(const ldg_math_stat_trait_table_t *table, uint32_t row, uint32_t trait)
{
    uintptr_t off = (((uintptr_t)row * (uintptr_t)table->trait_cap) + (uintptr_t)trait) * (uintptr_t)LDG_MATH_STAT_TRAIT_ALLELE_CUNT;

    return table->allele + off;
}

static double trait_express(const ldg_math_stat_trait_desc_t *desc, const double *allele)
{
    double lhs = allele[LDG_MATH_STAT_TRAIT_ALLELE_A];
    double rhs = allele[LDG_MATH_STAT_TRAIT_ALLELE_B];

    if (desc->dom == LDG_MATH_STAT_TRAIT_DOM_MAX) { return (lhs > rhs) ? lhs : rhs; }

    if (desc->dom == LDG_MATH_STAT_TRAIT_DOM_MIN) { return (lhs < rhs) ? lhs : rhs; }

    return trait_mean_get(lhs, rhs);
}

static double trait_norm_get(const ldg_math_stat_trait_desc_t *desc, double val)
{
    return (val - desc->min) / (desc->max - desc->min);
}

static uint32_t trait_init_ck(const ldg_math_stat_trait_table_t *table)
{
    if (LDG_UNLIKELY(!table)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(table->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

static uint32_t trait_row_ck(const ldg_math_stat_trait_table_t *table, uint32_t row)
{
    uint32_t ret = trait_init_ck(table);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(row >= table->row_cap)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(table->alive[row] != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}

static uint32_t trait_ck(const ldg_math_stat_trait_table_t *table, uint32_t trait)
{
    if (LDG_UNLIKELY(trait >= table->trait_cunt)) { return LDG_ERR_BOUNDS; }

    return LDG_ERR_AOK;
}

static uint32_t trait_desc_ck(const ldg_math_stat_trait_desc_t *desc)
{
    if (LDG_UNLIKELY(ldg_math_finite_is(desc->min) != LDG_TRUTH_TRUE || ldg_math_finite_is(desc->max) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ldg_math_finite_is(desc->mutate_scale) != LDG_TRUTH_TRUE || ldg_math_finite_is(desc->mutate_rate) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->min >= desc->max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->mutate_scale < TRAIT_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->mutate_rate < TRAIT_NONE || desc->mutate_rate > TRAIT_RATE_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->mix >= LDG_MATH_STAT_TRAIT_MIX_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(desc->dom >= LDG_MATH_STAT_TRAIT_DOM_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t trait_lane_idx_get(double lane, uint32_t max, uint32_t *out)
{
    *out = 0;

    if (ldg_math_finite_is(lane) != LDG_TRUTH_TRUE) { return LDG_ERR_INVALID; }

    if (lane < TRAIT_NONE || lane > (double)max) { return LDG_ERR_INVALID; }

    *out = (uint32_t)lane;

    return LDG_ERR_AOK;
}

static uint32_t trait_alloc(uint64_t size, void **out)
{
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc(size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

static void trait_free(void *ptr)
{
    if (ptr) { ldg_mem_dealloc(ptr); }
}

static void trait_free_list_build(ldg_math_stat_trait_table_t *table)
{
    uint32_t row = 0;
    uint32_t prev = LDG_MATH_STAT_TRAIT_ROW_NONE;

    table->free_hd = LDG_MATH_STAT_TRAIT_ROW_NONE;
    table->row_cunt = 0;

    while (row < table->row_cap)
    {
        table->free_next[row] = LDG_MATH_STAT_TRAIT_ROW_NONE;

        if (table->alive[row] == LDG_TRUTH_TRUE) { table->row_cunt++; }
        else
        {
            if (prev == LDG_MATH_STAT_TRAIT_ROW_NONE) { table->free_hd = row; }
            else { table->free_next[prev] = row; }

            prev = row;
        }

        row++;
    }
}

static void trait_row_fill(ldg_math_stat_trait_table_t *table, uint32_t row)
{
    double *allele = trait_allele_ptr(table, row, TRAIT_FIRST);
    uint32_t trait = 0;

    while (trait < table->trait_cunt)
    {
        allele[LDG_MATH_STAT_TRAIT_ALLELE_A] = table->desc[trait].min;
        allele[LDG_MATH_STAT_TRAIT_ALLELE_B] = table->desc[trait].min;
        allele += LDG_MATH_STAT_TRAIT_ALLELE_CUNT;
        trait++;
    }
}

static double trait_gamete_get(const ldg_math_stat_trait_table_t *table, uint32_t parent, uint32_t trait, uint32_t cut, uint32_t strand)
{
    const ldg_math_stat_trait_desc_t *desc = table->desc + trait;
    const double *allele = trait_allele_ptr(table, parent, trait);
    uint32_t pick = 0;

    if (desc->mix == LDG_MATH_STAT_TRAIT_MIX_MEAN) { return trait_mean_get(allele[LDG_MATH_STAT_TRAIT_ALLELE_A], allele[LDG_MATH_STAT_TRAIT_ALLELE_B]); }

    if (desc->mix == LDG_MATH_STAT_TRAIT_MIX_CROSSOVER)
    {
        pick = (trait < cut) ? strand : (strand ^ TRAIT_PICK_BIT);

        return allele[pick];
    }

    pick = trait_draw(table) & TRAIT_PICK_BIT;

    return allele[pick];
}

static void trait_allele_mutate(const ldg_math_stat_trait_table_t *table, const ldg_math_stat_trait_desc_t *desc, double *allele)
{
    double noise = TRAIT_NONE;

    if (trait_unit_draw(table) >= desc->mutate_rate) { return; }

    noise = desc->mutate_scale * trait_signed_unit_draw(table);
    *allele = trait_clamp(desc, *allele + noise);
}

uint32_t ldg_math_stat_trait_conf_default_get(ldg_math_stat_trait_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->trait_cap = TRAIT_CAP_DEFAULT;
    out->row_cap = TRAIT_ROW_CAP_DEFAULT;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_desc_default_get(ldg_math_stat_trait_desc_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->min = TRAIT_MIN_DEFAULT;
    out->max = TRAIT_MAX_DEFAULT;
    out->mutate_scale = TRAIT_MUTATE_SCALE_DEFAULT;
    out->mutate_rate = TRAIT_MUTATE_RATE_DEFAULT;
    out->mix = LDG_MATH_STAT_TRAIT_MIX_PICK;
    out->dom = LDG_MATH_STAT_TRAIT_DOM_MEAN;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_table_init(ldg_math_stat_trait_table_t *table, const ldg_math_stat_trait_conf_t *conf, ldg_math_stat_rng_cb_t rng_cb, void *rng_ctx)
{
    void *ptr = 0x0;
    uint64_t allele_cunt = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!table || !conf || !rng_cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->trait_cap == 0 || conf->row_cap == 0 || conf->row_cap == LDG_MATH_STAT_TRAIT_ROW_NONE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_zero(table, sizeof(*table));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    table->rng_cb = rng_cb;
    table->rng_ctx = rng_ctx;
    table->trait_cap = conf->trait_cap;
    table->row_cap = conf->row_cap;
    allele_cunt = (uint64_t)conf->row_cap * (uint64_t)conf->trait_cap * (uint64_t)LDG_MATH_STAT_TRAIT_ALLELE_CUNT;

    ret = trait_alloc((uint64_t)conf->trait_cap * (uint64_t)sizeof(ldg_math_stat_trait_desc_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    table->desc = (ldg_math_stat_trait_desc_t *)ptr;

    ret = trait_alloc(allele_cunt * (uint64_t)sizeof(double), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        trait_free(table->desc);
        return ret;
    }

    table->allele = (double *)ptr;

    ret = trait_alloc((uint64_t)conf->row_cap * (uint64_t)sizeof(uint32_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        trait_free(table->allele);
        trait_free(table->desc);
        return ret;
    }

    table->free_next = (uint32_t *)ptr;

    ret = trait_alloc((uint64_t)conf->row_cap * (uint64_t)sizeof(uint8_t), &ptr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        trait_free(table->free_next);
        trait_free(table->allele);
        trait_free(table->desc);
        return ret;
    }

    table->alive = (uint8_t *)ptr;

    trait_free_list_build(table);
    table->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_table_shutdown(ldg_math_stat_trait_table_t *table)
{
    uint32_t ret = trait_init_ck(table);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    trait_free(table->alive);
    trait_free(table->free_next);
    trait_free(table->allele);
    trait_free(table->desc);

    return ldg_mem_zero(table, sizeof(*table));
}

uint32_t ldg_math_stat_trait_add(ldg_math_stat_trait_table_t *table, const ldg_math_stat_trait_desc_t *desc, uint32_t *out)
{
    uint32_t ret = trait_init_ck(table);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!desc || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = UINT32_MAX;

    if (LDG_UNLIKELY(table->row_cunt > 0)) { return LDG_ERR_BUSY; }

    if (LDG_UNLIKELY(table->trait_cunt >= table->trait_cap)) { return LDG_ERR_FULL; }

    ret = trait_desc_ck(desc);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    table->desc[table->trait_cunt] = *desc;
    *out = table->trait_cunt;
    table->trait_cunt++;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_desc_get(const ldg_math_stat_trait_table_t *table, uint32_t trait, ldg_math_stat_trait_desc_t *out)
{
    uint32_t ret = trait_init_ck(table);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = trait_ck(table, trait);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = table->desc[trait];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_cunt_get(const ldg_math_stat_trait_table_t *table, uint32_t *out)
{
    uint32_t ret = trait_init_ck(table);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = table->trait_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_row_alloc(ldg_math_stat_trait_table_t *table, uint32_t *out)
{
    uint32_t row = LDG_MATH_STAT_TRAIT_ROW_NONE;
    uint32_t ret = trait_init_ck(table);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = LDG_MATH_STAT_TRAIT_ROW_NONE;

    if (LDG_UNLIKELY(table->trait_cunt == 0)) { return LDG_ERR_EMPTY; }

    row = table->free_hd;
    if (LDG_UNLIKELY(row == LDG_MATH_STAT_TRAIT_ROW_NONE)) { return LDG_ERR_FULL; }

    table->free_hd = table->free_next[row];
    table->free_next[row] = LDG_MATH_STAT_TRAIT_ROW_NONE;
    table->alive[row] = LDG_TRUTH_TRUE;
    table->row_cunt++;
    trait_row_fill(table, row);
    *out = row;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_row_free(ldg_math_stat_trait_table_t *table, uint32_t row)
{
    uint32_t ret = trait_row_ck(table, row);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    table->alive[row] = LDG_TRUTH_FALSE;
    table->free_next[row] = table->free_hd;
    table->free_hd = row;
    table->row_cunt--;

    return LDG_ERR_AOK;
}

uint8_t ldg_math_stat_trait_row_alive_is(const ldg_math_stat_trait_table_t *table, uint32_t row)
{
    uint32_t ret = trait_init_ck(table);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    if (LDG_UNLIKELY(row >= table->row_cap)) { return LDG_ERR_BOUNDS; }

    return table->alive[row];
}

uint32_t ldg_math_stat_trait_row_cunt_get(const ldg_math_stat_trait_table_t *table, uint32_t *out)
{
    uint32_t ret = trait_init_ck(table);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = table->row_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_row_draw(ldg_math_stat_trait_table_t *table, uint32_t row)
{
    double *allele = 0x0;
    double span = TRAIT_NONE;
    uint32_t trait = 0;
    uint32_t ret = trait_row_ck(table, row);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    allele = trait_allele_ptr(table, row, TRAIT_FIRST);

    while (trait < table->trait_cunt)
    {
        span = table->desc[trait].max - table->desc[trait].min;
        allele[LDG_MATH_STAT_TRAIT_ALLELE_A] = table->desc[trait].min + (span * trait_unit_draw(table));
        allele[LDG_MATH_STAT_TRAIT_ALLELE_B] = table->desc[trait].min + (span * trait_unit_draw(table));
        allele += LDG_MATH_STAT_TRAIT_ALLELE_CUNT;
        trait++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_row_cpy(ldg_math_stat_trait_table_t *table, uint32_t dst, uint32_t src)
{
    double *dst_allele = 0x0;
    const double *src_allele = 0x0;
    uintptr_t cunt = 0;
    uintptr_t idx = 0;
    uint32_t ret = trait_row_ck(table, dst);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = trait_row_ck(table, src);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (dst == src) { return LDG_ERR_AOK; }

    dst_allele = trait_allele_ptr(table, dst, TRAIT_FIRST);
    src_allele = trait_allele_ptr(table, src, TRAIT_FIRST);
    cunt = (uintptr_t)table->trait_cunt * (uintptr_t)LDG_MATH_STAT_TRAIT_ALLELE_CUNT;

    while (idx < cunt)
    {
        dst_allele[idx] = src_allele[idx];
        idx++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_row_mutate(ldg_math_stat_trait_table_t *table, uint32_t row)
{
    double *allele = 0x0;
    uint32_t trait = 0;
    uint32_t ret = trait_row_ck(table, row);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    allele = trait_allele_ptr(table, row, TRAIT_FIRST);

    while (trait < table->trait_cunt)
    {
        trait_allele_mutate(table, table->desc + trait, allele + LDG_MATH_STAT_TRAIT_ALLELE_A);
        trait_allele_mutate(table, table->desc + trait, allele + LDG_MATH_STAT_TRAIT_ALLELE_B);
        allele += LDG_MATH_STAT_TRAIT_ALLELE_CUNT;
        trait++;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_row_mix(ldg_math_stat_trait_table_t *table, uint32_t child, uint32_t parent_a, uint32_t parent_b)
{
    double *allele = 0x0;
    uint32_t cut_a = 0;
    uint32_t cut_b = 0;
    uint32_t strand_a = 0;
    uint32_t strand_b = 0;
    uint32_t trait = 0;
    uint32_t ret = trait_row_ck(table, child);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = trait_row_ck(table, parent_a);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = trait_row_ck(table, parent_b);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(child == parent_a || child == parent_b)) { return LDG_ERR_FUNC_ARG_INVALID; }

    cut_a = trait_draw(table) % (table->trait_cunt + TRAIT_CUT_INCLUSIVE);
    cut_b = trait_draw(table) % (table->trait_cunt + TRAIT_CUT_INCLUSIVE);
    strand_a = trait_draw(table) & TRAIT_PICK_BIT;
    strand_b = trait_draw(table) & TRAIT_PICK_BIT;
    allele = trait_allele_ptr(table, child, TRAIT_FIRST);

    while (trait < table->trait_cunt)
    {
        allele[LDG_MATH_STAT_TRAIT_ALLELE_A] = trait_gamete_get(table, parent_a, trait, cut_a, strand_a);
        allele[LDG_MATH_STAT_TRAIT_ALLELE_B] = trait_gamete_get(table, parent_b, trait, cut_b, strand_b);
        allele += LDG_MATH_STAT_TRAIT_ALLELE_CUNT;
        trait++;
    }

    return ldg_math_stat_trait_row_mutate(table, child);
}

uint32_t ldg_math_stat_trait_allele_get(const ldg_math_stat_trait_table_t *table, uint32_t row, uint32_t trait, uint32_t allele, double *out)
{
    uint32_t ret = trait_row_ck(table, row);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = TRAIT_NONE;

    ret = trait_ck(table, trait);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(allele >= LDG_MATH_STAT_TRAIT_ALLELE_CUNT)) { return LDG_ERR_BOUNDS; }

    *out = trait_allele_ptr(table, row, trait)[allele];

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_allele_set(ldg_math_stat_trait_table_t *table, uint32_t row, uint32_t trait, uint32_t allele, double val)
{
    uint32_t ret = trait_row_ck(table, row);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = trait_ck(table, trait);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(allele >= LDG_MATH_STAT_TRAIT_ALLELE_CUNT)) { return LDG_ERR_BOUNDS; }

    if (LDG_UNLIKELY(ldg_math_finite_is(val) != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(val < table->desc[trait].min || val > table->desc[trait].max)) { return LDG_ERR_RANGE; }

    trait_allele_ptr(table, row, trait)[allele] = val;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_val_get(const ldg_math_stat_trait_table_t *table, uint32_t row, uint32_t trait, double *out)
{
    uint32_t ret = trait_row_ck(table, row);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = TRAIT_NONE;

    ret = trait_ck(table, trait);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = trait_express(table->desc + trait, trait_allele_ptr(table, row, trait));

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_lane_get(const ldg_math_stat_trait_table_t *table, uint32_t row, double *dst, uintptr_t cap, uintptr_t *out_cunt)
{
    const double *allele = 0x0;
    uint32_t trait = 0;
    uint32_t ret = trait_row_ck(table, row);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap < (uintptr_t)table->trait_cunt)) { return LDG_ERR_BOUNDS; }

    allele = trait_allele_ptr(table, row, TRAIT_FIRST);

    while (trait < table->trait_cunt)
    {
        dst[trait] = trait_norm_get(table->desc + trait, trait_express(table->desc + trait, allele));
        allele += LDG_MATH_STAT_TRAIT_ALLELE_CUNT;
        trait++;
    }

    *out_cunt = table->trait_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_dist_get(const ldg_math_stat_trait_table_t *table, uint32_t row_a, uint32_t row_b, double *out)
{
    const double *allele_a = 0x0;
    const double *allele_b = 0x0;
    double gap = TRAIT_NONE;
    double sum = TRAIT_NONE;
    uint32_t trait = 0;
    uint32_t ret = trait_row_ck(table, row_a);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = TRAIT_NONE;

    ret = trait_row_ck(table, row_b);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(table->trait_cunt == 0)) { return LDG_ERR_EMPTY; }

    allele_a = trait_allele_ptr(table, row_a, TRAIT_FIRST);
    allele_b = trait_allele_ptr(table, row_b, TRAIT_FIRST);

    while (trait < table->trait_cunt)
    {
        gap = trait_norm_get(table->desc + trait, trait_express(table->desc + trait, allele_a)) - trait_norm_get(table->desc + trait, trait_express(table->desc + trait, allele_b));
        sum += ldg_math_abs_get(gap);
        allele_a += LDG_MATH_STAT_TRAIT_ALLELE_CUNT;
        allele_b += LDG_MATH_STAT_TRAIT_ALLELE_CUNT;
        trait++;
    }

    *out = sum / (double)table->trait_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_stat_get(const ldg_math_stat_trait_table_t *table, uint32_t trait, double *out_mean, double *out_var)
{
    const ldg_math_stat_trait_desc_t *desc = 0x0;
    double mean = TRAIT_NONE;
    double diff = TRAIT_NONE;
    double sum = TRAIT_NONE;
    double sq = TRAIT_NONE;
    uint32_t row = 0;
    uint32_t ret = trait_init_ck(table);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!out_mean || !out_var)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_mean = TRAIT_NONE;
    *out_var = TRAIT_NONE;

    ret = trait_ck(table, trait);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(table->row_cunt == 0)) { return LDG_ERR_EMPTY; }

    desc = table->desc + trait;

    while (row < table->row_cap)
    {
        if (table->alive[row] == LDG_TRUTH_TRUE) { sum += trait_express(desc, trait_allele_ptr(table, row, trait)); }

        row++;
    }

    mean = sum / (double)table->row_cunt;

    row = 0;
    while (row < table->row_cap)
    {
        if (table->alive[row] == LDG_TRUTH_TRUE)
        {
            diff = trait_express(desc, trait_allele_ptr(table, row, trait)) - mean;
            sq += diff * diff;
        }

        row++;
    }

    *out_mean = mean;
    *out_var = sq / (double)table->row_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_state_cunt_get(const ldg_math_stat_trait_table_t *table, uintptr_t *out)
{
    uintptr_t row_len = 0;
    uint32_t ret = trait_init_ck(table);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    row_len = (uintptr_t)TRAIT_ROW_HDR_CUNT + ((uintptr_t)table->trait_cunt * (uintptr_t)LDG_MATH_STAT_TRAIT_ALLELE_CUNT);
    *out = (uintptr_t)TRAIT_STATE_HDR_CUNT + ((uintptr_t)table->trait_cunt * (uintptr_t)TRAIT_DESC_LANE_CUNT) + ((uintptr_t)table->row_cap * row_len);

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_state_get(const ldg_math_stat_trait_table_t *table, double *dst, uintptr_t cap, uintptr_t *out_cunt)
{
    const double *allele = 0x0;
    uintptr_t need = 0;
    uintptr_t allele_cunt = 0;
    uintptr_t idx = 0;
    uint32_t trait = 0;
    uint32_t row = 0;
    uint32_t ret = trait_init_ck(table);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = ldg_math_stat_trait_state_cunt_get(table, &need);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(cap < need)) { return LDG_ERR_BOUNDS; }

    dst[TRAIT_STATE_HDR_TRAIT] = (double)table->trait_cunt;
    dst[TRAIT_STATE_HDR_ROW] = (double)table->row_cap;
    dst += TRAIT_STATE_HDR_CUNT;

    while (trait < table->trait_cunt)
    {
        dst[TRAIT_DESC_LANE_MIN] = table->desc[trait].min;
        dst[TRAIT_DESC_LANE_MAX] = table->desc[trait].max;
        dst[TRAIT_DESC_LANE_SCALE] = table->desc[trait].mutate_scale;
        dst[TRAIT_DESC_LANE_RATE] = table->desc[trait].mutate_rate;
        dst[TRAIT_DESC_LANE_MIX] = (double)table->desc[trait].mix;
        dst[TRAIT_DESC_LANE_DOM] = (double)table->desc[trait].dom;
        dst += TRAIT_DESC_LANE_CUNT;
        trait++;
    }

    allele_cunt = (uintptr_t)table->trait_cunt * (uintptr_t)LDG_MATH_STAT_TRAIT_ALLELE_CUNT;

    while (row < table->row_cap)
    {
        *dst = (double)table->alive[row];
        dst += TRAIT_ROW_HDR_CUNT;
        allele = trait_allele_ptr(table, row, TRAIT_FIRST);

        idx = 0;
        while (idx < allele_cunt)
        {
            dst[idx] = allele[idx];
            idx++;
        }

        dst += allele_cunt;
        row++;
    }

    *out_cunt = need;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_trait_state_set(ldg_math_stat_trait_table_t *table, const double *src, uintptr_t cunt)
{
    double *allele = 0x0;
    ldg_math_stat_trait_desc_t desc = LDG_STRUCT_ZERO_INIT;
    uintptr_t need = 0;
    uintptr_t allele_cunt = 0;
    uintptr_t idx = 0;
    uint32_t trait_cunt = 0;
    uint32_t row_cap = 0;
    uint32_t trait = 0;
    uint32_t row = 0;
    uint32_t ret = trait_init_ck(table);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(!src)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt < TRAIT_STATE_HDR_CUNT)) { return LDG_ERR_BOUNDS; }

    ret = trait_lane_idx_get(src[TRAIT_STATE_HDR_TRAIT], table->trait_cap, &trait_cunt);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = trait_lane_idx_get(src[TRAIT_STATE_HDR_ROW], table->row_cap, &row_cap);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(row_cap != table->row_cap)) { return LDG_ERR_INVALID; }

    allele_cunt = (uintptr_t)trait_cunt * (uintptr_t)LDG_MATH_STAT_TRAIT_ALLELE_CUNT;
    need = (uintptr_t)TRAIT_STATE_HDR_CUNT + ((uintptr_t)trait_cunt * (uintptr_t)TRAIT_DESC_LANE_CUNT) + ((uintptr_t)table->row_cap * ((uintptr_t)TRAIT_ROW_HDR_CUNT + allele_cunt));
    if (LDG_UNLIKELY(cunt != need)) { return LDG_ERR_BOUNDS; }

    src += TRAIT_STATE_HDR_CUNT;

    while (trait < trait_cunt)
    {
        desc.min = src[TRAIT_DESC_LANE_MIN];
        desc.max = src[TRAIT_DESC_LANE_MAX];
        desc.mutate_scale = src[TRAIT_DESC_LANE_SCALE];
        desc.mutate_rate = src[TRAIT_DESC_LANE_RATE];

        ret = trait_lane_idx_get(src[TRAIT_DESC_LANE_MIX], LDG_MATH_STAT_TRAIT_MIX_CUNT, &desc.mix);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = trait_lane_idx_get(src[TRAIT_DESC_LANE_DOM], LDG_MATH_STAT_TRAIT_DOM_CUNT, &desc.dom);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = trait_desc_ck(&desc);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

        src += TRAIT_DESC_LANE_CUNT;
        trait++;
    }

    src -= (uintptr_t)trait_cunt * (uintptr_t)TRAIT_DESC_LANE_CUNT;
    table->trait_cunt = trait_cunt;

    trait = 0;
    while (trait < trait_cunt)
    {
        table->desc[trait].min = src[TRAIT_DESC_LANE_MIN];
        table->desc[trait].max = src[TRAIT_DESC_LANE_MAX];
        table->desc[trait].mutate_scale = src[TRAIT_DESC_LANE_SCALE];
        table->desc[trait].mutate_rate = src[TRAIT_DESC_LANE_RATE];
        table->desc[trait].mix = (uint32_t)src[TRAIT_DESC_LANE_MIX];
        table->desc[trait].dom = (uint32_t)src[TRAIT_DESC_LANE_DOM];
        src += TRAIT_DESC_LANE_CUNT;
        trait++;
    }

    while (row < table->row_cap)
    {
        table->alive[row] = (*src >= TRAIT_ALIVE_MIN) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        src += TRAIT_ROW_HDR_CUNT;
        allele = trait_allele_ptr(table, row, TRAIT_FIRST);

        idx = 0;
        while (idx < allele_cunt)
        {
            allele[idx] = src[idx];
            idx++;
        }

        src += allele_cunt;
        row++;
    }

    trait_free_list_build(table);

    return LDG_ERR_AOK;
}
