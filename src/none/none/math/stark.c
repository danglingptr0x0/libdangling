#include <stdint.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/air.h>
#include <dangling/math/ext.h>
#include <dangling/math/field.h>
#include <dangling/math/fri.h>
#include <dangling/math/merkle.h>
#include <dangling/math/ntt.h>
#include <dangling/math/proof.h>
#include <dangling/math/stark.h>
#include <dangling/math/transcript.h>

#define LDG_MATH_STARK_WORD_SIZE 8
#define LDG_MATH_STARK_SINGLE_ELEM 1
#define LDG_MATH_STARK_PAIR_STEP 2
#define LDG_MATH_STARK_PREV_OFF 1
#define LDG_MATH_STARK_TREE_FACTOR 2
#define LDG_MATH_STARK_SEED_LEN 6
#define LDG_MATH_STARK_OOD_ATTEMPT_MAX 64
#define LDG_MATH_STARK_OOD_POINT_CUNT 2
#define LDG_MATH_STARK_QUERY_TOUCH_CUNT 2
#define LDG_MATH_STARK_DEFAULT_LOG_TRACE 6
#define LDG_MATH_STARK_DEFAULT_COL_CUNT 2
#define LDG_MATH_STARK_DEFAULT_LOG_BLOWUP 3
#define LDG_MATH_STARK_DEFAULT_CONSTRAINT_CUNT 2
#define LDG_MATH_STARK_DEFAULT_CONSTRAINT_DEGREE 2
#define LDG_MATH_STARK_CEIL_BIAS 1
#define LDG_MATH_STARK_DEFAULT_QUERY_CUNT 24
#define LDG_MATH_STARK_DEFAULT_LOG_REMAINDER 3
#define LDG_MATH_STARK_DEFAULT_GRIND_BIT_CUNT 8
#define LDG_MATH_STARK_DEFAULT_SHIFT 0x0000000000000007ULL

typedef struct stark_wr_cursor
{
    uint8_t *base;
    uint64_t cap;
    uint64_t pos;
} stark_wr_cursor_t;

typedef struct stark_rd_cursor
{
    const uint8_t *base;
    uint64_t cap;
    uint64_t pos;
} stark_rd_cursor_t;

typedef struct stark_shape
{
    uint32_t log_domain;
    uint32_t layer_cunt;
    uint64_t trace_cunt;
    uint64_t domain_cunt;
    uint64_t blowup;
    uint64_t remainder_cunt;
    uint64_t col_coeff_cunt;
    uint64_t path_size;
    uint64_t ext_size;
} stark_shape_t;

static uint32_t stark_word_st(stark_wr_cursor_t *curr, uint64_t val)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY((curr->pos + LDG_MATH_STARK_WORD_SIZE) > curr->cap)) { return LDG_ERR_BOUNDS; }

    for (idx = 0; idx < LDG_MATH_STARK_WORD_SIZE; idx++) { curr->base[curr->pos + idx] = (uint8_t)((val >> (idx * LDG_BYTE_BITS)) & LDG_BYTE_MASK); }

    curr->pos += LDG_MATH_STARK_WORD_SIZE;

    return LDG_ERR_AOK;
}

static uint32_t stark_ext_st(stark_wr_cursor_t *curr, ldg_math_ext_t val)
{
    uint32_t ret = LDG_ERR_AOK;

    ret = stark_word_st(curr, val.lo);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return stark_word_st(curr, val.hi);
}

static uint32_t stark_blob_st(stark_wr_cursor_t *curr, const uint8_t *src, uint64_t len)
{
    uint64_t idx = 0;

    if (LDG_UNLIKELY((curr->pos + len) > curr->cap)) { return LDG_ERR_BOUNDS; }

    for (idx = 0; idx < len; idx++) { curr->base[curr->pos + idx] = src[idx]; }

    curr->pos += len;

    return LDG_ERR_AOK;
}

static uint32_t stark_path_st(stark_wr_cursor_t *curr, const uint8_t *node_base, uint64_t leaf_cunt, uint64_t pos, uint64_t path_size)
{
    uint8_t *path_slot = 0x0;

    if (LDG_UNLIKELY((curr->pos + path_size) > curr->cap)) { return LDG_ERR_BOUNDS; }

    path_slot = curr->base + curr->pos;
    curr->pos += path_size;

    return ldg_math_merkle_path_get(node_base, leaf_cunt, pos, path_slot, path_size);
}

static uint32_t stark_word_ld(stark_rd_cursor_t *curr, uint64_t *out)
{
    uint32_t idx = 0;
    uint64_t val = 0;

    if (LDG_UNLIKELY((curr->pos + LDG_MATH_STARK_WORD_SIZE) > curr->cap)) { return LDG_ERR_BOUNDS; }

    for (idx = 0; idx < LDG_MATH_STARK_WORD_SIZE; idx++) { val |= (uint64_t)curr->base[curr->pos + idx] << (idx * LDG_BYTE_BITS); }

    curr->pos += LDG_MATH_STARK_WORD_SIZE;
    *out = val;

    return LDG_ERR_AOK;
}

static uint32_t stark_elem_ld(stark_rd_cursor_t *curr, ldg_math_field_elem_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    uint64_t val = 0;

    ret = stark_word_ld(curr, &val);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ldg_math_field_canonical_is(val) != LDG_TRUTH_TRUE)) { return LDG_ERR_INVALID; }

    *out = val;

    return LDG_ERR_AOK;
}

static uint32_t stark_ext_ld(stark_rd_cursor_t *curr, ldg_math_ext_t *out)
{
    uint32_t ret = LDG_ERR_AOK;

    ret = stark_elem_ld(curr, &out->lo);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return stark_elem_ld(curr, &out->hi);
}

static uint32_t stark_blob_ref_get(stark_rd_cursor_t *curr, uint64_t len, const uint8_t **out)
{
    if (LDG_UNLIKELY((curr->pos + len) > curr->cap)) { return LDG_ERR_BOUNDS; }

    *out = curr->base + curr->pos;
    curr->pos += len;

    return LDG_ERR_AOK;
}

static uint32_t stark_shape_get(const ldg_math_stark_conf_t *conf, stark_shape_t *out)
{
    uint32_t ret = LDG_ERR_AOK;

    out->log_domain = conf->air.log_trace + conf->air.log_blowup;

    ret = ldg_math_ntt_domain_cunt_get(conf->air.log_trace, &out->trace_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_domain_cunt_get(out->log_domain, &out->domain_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_domain_cunt_get(conf->air.log_blowup, &out->blowup);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_domain_cunt_get(conf->log_remainder, &out->remainder_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_air_col_coeff_cunt_get(&conf->air, &out->col_coeff_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->path_size = (uint64_t)out->log_domain * LDG_MATH_MERKLE_NODE_SIZE;
    out->ext_size = LDG_MATH_STARK_WORD_SIZE * LDG_MATH_EXT_DEGREE;

    return ldg_math_fri_layer_cunt_get(out->log_domain, conf->log_remainder, &out->layer_cunt);
}

static uint32_t stark_seed_fill(uint8_t *out)
{
    out[0] = 0x44;
    out[1] = 0x41;
    out[2] = 0x4E;
    out[3] = 0x47;
    out[4] = 0x53;
    out[5] = 0x54;

    return LDG_ERR_AOK;
}

static uint32_t stark_ood_draw(ldg_math_transcript_t *script, const stark_shape_t *shape, ldg_math_field_elem_t shift, ldg_math_ext_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t attempt = 0;
    ldg_math_ext_t candidate = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t domain_pwr = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t trace_pwr = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t shift_pwr = LDG_STRUCT_ZERO_INIT;

    shift_pwr = ldg_math_ext_base_embed(ldg_math_field_pwr_get(shift, shape->domain_cunt));

    for (attempt = 0; attempt < LDG_MATH_STARK_OOD_ATTEMPT_MAX; attempt++)
    {
        ret = ldg_math_transcript_ext_get(script, &candidate);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ldg_math_ext_zero_is(candidate) == LDG_TRUTH_TRUE) { continue; }

        ret = ldg_math_ext_pwr_get(candidate, shape->domain_cunt, &domain_pwr);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_ext_pwr_get(candidate, shape->trace_cunt, &trace_pwr);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (ldg_math_ext_eq_is(domain_pwr, shift_pwr) == LDG_TRUTH_TRUE) { continue; }

        if (ldg_math_ext_eq_is(trace_pwr, ldg_math_ext_base_embed(LDG_MATH_FIELD_MUL_IDENT)) == LDG_TRUTH_TRUE) { continue; }

        *out = candidate;

        return LDG_ERR_AOK;
    }

    return LDG_ERR_NOT_FOUND;
}

static uint32_t stark_segment_open(stark_wr_cursor_t *curr, const uint8_t *node_base, const ldg_math_field_elem_t *lde, const stark_shape_t *shape, uint32_t col_cunt, uint64_t pos)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t col = 0;

    for (col = 0; col < col_cunt; col++)
    {
        ret = stark_word_st(curr, lde[(pos * col_cunt) + col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    return stark_path_st(curr, node_base, shape->domain_cunt, pos, shape->path_size);
}

static uint32_t stark_segment_rd_ck(stark_rd_cursor_t *curr, const uint8_t *root, const stark_shape_t *shape, uint32_t col_cunt, uint64_t pos, ldg_math_field_elem_t *out_val)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t col = 0;
    const uint8_t *path = 0x0;

    for (col = 0; col < col_cunt; col++)
    {
        ret = stark_elem_ld(curr, &out_val[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = stark_blob_ref_get(curr, shape->path_size, &path);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_merkle_path_ck(root, out_val, col_cunt, pos, shape->domain_cunt, path, shape->path_size);
}

static uint32_t stark_ext_rd_ck(stark_rd_cursor_t *curr, const uint8_t *root, uint64_t leaf_cunt, uint64_t pos, uint64_t path_size, ldg_math_ext_t *out_val)
{
    uint32_t ret = LDG_ERR_AOK;
    const uint8_t *path = 0x0;

    ret = stark_ext_ld(curr, out_val);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = stark_blob_ref_get(curr, path_size, &path);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_math_merkle_path_ck(root, &out_val->lo, LDG_MATH_EXT_DEGREE, pos, leaf_cunt, path, path_size);
}

static uint32_t stark_deep_cum(const ldg_math_field_elem_t *lde, const stark_shape_t *shape, uint32_t col_cunt, ldg_math_field_elem_t shift, ldg_math_ext_t point, const ldg_math_ext_t *ood, ldg_math_ext_t *acc, ldg_math_field_elem_t *tmp, ldg_math_ext_t *tmp_ext, ldg_math_ext_t *deep_buff, uint64_t deep_need, ldg_math_ext_t delta, ldg_math_ext_t *pwr)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t col = 0;

    for (col = 0; col < col_cunt; col++)
    {
        ret = ldg_math_air_col_gather(lde, shape->domain_cunt, col_cunt, col, tmp);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_proof_lift_deep_div(tmp, shape->log_domain, shift, point, ood[col], tmp_ext, deep_buff, deep_need);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_proof_ext_poly_cum(acc, tmp_ext, shape->domain_cunt, *pwr);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *pwr = ldg_math_ext_mul(*pwr, delta);
    }

    return LDG_ERR_AOK;
}

static ldg_math_ext_t stark_deep_point_cum(const ldg_math_field_elem_t *val, const ldg_math_ext_t *ood, uint32_t col_cunt, ldg_math_ext_t inverted, ldg_math_ext_t delta, ldg_math_ext_t *pwr, ldg_math_ext_t acc)
{
    uint32_t col = 0;
    ldg_math_ext_t total = LDG_STRUCT_ZERO_INIT;

    total = acc;

    for (col = 0; col < col_cunt; col++)
    {
        total = ldg_math_ext_add(total, ldg_math_ext_mul(*pwr, ldg_math_ext_mul(ldg_math_ext_sub(ldg_math_ext_base_embed(val[col]), ood[col]), inverted)));
        *pwr = ldg_math_ext_mul(*pwr, delta);
    }

    return total;
}

uint32_t ldg_math_stark_conf_default_get(ldg_math_stark_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->air.log_trace = LDG_MATH_STARK_DEFAULT_LOG_TRACE;
    out->air.col_cunt = LDG_MATH_STARK_DEFAULT_COL_CUNT;
    out->air.aux_col_cunt = 0;
    out->air.log_blowup = LDG_MATH_STARK_DEFAULT_LOG_BLOWUP;
    out->air.constraint_cunt = LDG_MATH_STARK_DEFAULT_CONSTRAINT_CUNT;
    out->air.periodic_cunt = 0;
    out->air.pub_cunt = 0;
    out->air.challenge_cunt = 0;
    out->air.blind_cunt = 0;
    out->air.log_period = 0;
    out->air.constraint_degree = LDG_MATH_STARK_DEFAULT_CONSTRAINT_DEGREE;
    out->air.coset_shift = LDG_MATH_STARK_DEFAULT_SHIFT;
    out->query_cunt = LDG_MATH_STARK_DEFAULT_QUERY_CUNT;
    out->log_remainder = LDG_MATH_STARK_DEFAULT_LOG_REMAINDER;
    out->grind_bit_cunt = LDG_MATH_STARK_DEFAULT_GRIND_BIT_CUNT;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stark_conf_ck(const ldg_math_stark_conf_t *conf)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_air_conf_ck(&conf->air);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(conf->query_cunt < LDG_MATH_STARK_QUERY_MIN || conf->query_cunt > LDG_MATH_STARK_QUERY_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->log_remainder < LDG_MATH_STARK_LOG_REMAINDER_MIN)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->log_remainder >= (conf->air.log_trace + conf->air.log_blowup))) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY((conf->air.log_trace + conf->air.log_blowup - conf->log_remainder) > LDG_MATH_STARK_LAYER_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->grind_bit_cunt > LDG_MATH_TRANSCRIPT_GRIND_BIT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (conf->air.blind_cunt > 0) { if (LDG_UNLIKELY((uint64_t)conf->air.blind_cunt < (((uint64_t)conf->query_cunt * LDG_MATH_STARK_QUERY_TOUCH_CUNT) + LDG_MATH_STARK_OOD_POINT_CUNT))) { return LDG_ERR_FUNC_ARG_INVALID; } }

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stark_prove_buff_cunt_get(const ldg_math_stark_conf_t *conf, uint64_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    stark_shape_t shape = LDG_STRUCT_ZERO_INIT;
    uint64_t total = 0;
    uint64_t seg_cunt = 0;
    uint64_t ext_need = 0;
    uint64_t periodic_need = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    ret = ldg_math_stark_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = stark_shape_get(conf, &shape);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_air_composition_ext_buff_cunt_get(&conf->air, &ext_need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_air_periodic_buff_cunt_get(&conf->air, &periodic_need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    seg_cunt = (uint64_t)conf->air.col_cunt + conf->air.aux_col_cunt;
    total = shape.col_coeff_cunt * seg_cunt;
    total += shape.domain_cunt * seg_cunt;
    total += shape.trace_cunt * conf->air.aux_col_cunt;
    total += shape.domain_cunt * LDG_MATH_STARK_LAYER_MAX;
    total += shape.trace_cunt;
    total += shape.remainder_cunt * LDG_MATH_EXT_DEGREE;
    total += ext_need * LDG_MATH_EXT_DEGREE;
    total += (shape.blowup * LDG_MATH_STARK_TREE_FACTOR) + shape.blowup + LDG_MATH_EXT_DEGREE;
    total += shape.domain_cunt * conf->air.periodic_cunt;
    total += periodic_need;

    *out = total;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stark_verify_buff_cunt_get(const ldg_math_stark_conf_t *conf, uint64_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    stark_shape_t shape = LDG_STRUCT_ZERO_INIT;
    uint64_t total = 0;
    uint64_t periodic_need = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    ret = ldg_math_stark_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = stark_shape_get(conf, &shape);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_air_periodic_buff_cunt_get(&conf->air, &periodic_need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    total = shape.remainder_cunt * LDG_MATH_EXT_DEGREE;
    total += shape.remainder_cunt * LDG_MATH_EXT_DEGREE;
    total += shape.remainder_cunt;
    total += (uint64_t)conf->air.constraint_cunt * LDG_MATH_EXT_DEGREE;
    total += (uint64_t)conf->air.periodic_cunt * LDG_MATH_EXT_DEGREE;
    total += (uint64_t)conf->air.pub_cunt * LDG_MATH_EXT_DEGREE;
    total += conf->air.col_cunt;
    total += conf->air.aux_col_cunt;
    total += periodic_need;
    total += LDG_MATH_EXT_DEGREE;

    *out = total;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stark_node_cap_get(const ldg_math_stark_conf_t *conf, uint64_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t layer = 0;
    stark_shape_t shape = LDG_STRUCT_ZERO_INIT;
    uint64_t node_cunt = 0;
    uint64_t curr_cunt = 0;
    uint64_t tree_cunt = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    ret = ldg_math_stark_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = stark_shape_get(conf, &shape);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    tree_cunt = LDG_MATH_STARK_PAIR_STEP;

    if (conf->air.aux_col_cunt > 0) { tree_cunt++; }

    node_cunt = shape.domain_cunt * LDG_MATH_STARK_TREE_FACTOR * tree_cunt;
    curr_cunt = shape.domain_cunt;

    for (layer = 0; layer < shape.layer_cunt; layer++)
    {
        node_cunt += curr_cunt * LDG_MATH_STARK_TREE_FACTOR;
        curr_cunt /= LDG_MATH_STARK_PAIR_STEP;
    }

    *out = node_cunt * LDG_MATH_MERKLE_NODE_SIZE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stark_proof_size_get(const ldg_math_stark_conf_t *conf, uint64_t *out)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t layer = 0;
    stark_shape_t shape = LDG_STRUCT_ZERO_INIT;
    uint64_t total = 0;
    uint64_t per_query = 0;
    uint64_t depth = 0;
    uint64_t seg_cunt = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    ret = ldg_math_stark_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = stark_shape_get(conf, &shape);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    seg_cunt = (uint64_t)conf->air.col_cunt + conf->air.aux_col_cunt;

    total = LDG_MATH_MERKLE_NODE_SIZE * LDG_MATH_STARK_PAIR_STEP;

    if (conf->air.aux_col_cunt > 0) { total += LDG_MATH_MERKLE_NODE_SIZE; }

    total += shape.ext_size;
    total += seg_cunt * shape.ext_size * LDG_MATH_STARK_PAIR_STEP;
    total += shape.ext_size;
    total += (uint64_t)shape.layer_cunt * LDG_MATH_MERKLE_NODE_SIZE;
    total += shape.remainder_cunt * shape.ext_size;
    total += LDG_MATH_STARK_WORD_SIZE;

    per_query = (uint64_t)conf->air.col_cunt * LDG_MATH_STARK_WORD_SIZE;
    per_query += shape.path_size;

    if (conf->air.aux_col_cunt > 0)
    {
        per_query += (uint64_t)conf->air.aux_col_cunt * LDG_MATH_STARK_WORD_SIZE;
        per_query += shape.path_size;
    }

    per_query += shape.ext_size;
    per_query += shape.path_size;

    for (layer = 0; layer < shape.layer_cunt; layer++)
    {
        depth = shape.log_domain - layer;
        per_query += shape.ext_size * LDG_MATH_STARK_PAIR_STEP;
        per_query += depth * LDG_MATH_MERKLE_NODE_SIZE * LDG_MATH_STARK_PAIR_STEP;
    }

    total += per_query * conf->query_cunt;
    *out = total;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stark_prove(const ldg_math_stark_conf_t *conf, ldg_math_air_eval_cb_t air_cb, ldg_math_air_aux_fill_cb_t aux_cb, void *air_ctx, const ldg_math_field_elem_t *pub, const ldg_math_field_elem_t *trace, const ldg_math_field_elem_t *rand, const ldg_math_field_elem_t *periodic, ldg_math_field_elem_t *buff, uint64_t buff_cunt, uint8_t *node, uint64_t node_cap, uint8_t *proof, uint64_t proof_cap, uint64_t *out_len)
{
    uint32_t ret = LDG_ERR_AOK;
    uint32_t col = 0;
    uint32_t layer = 0;
    uint32_t query = 0;
    stark_shape_t shape = LDG_STRUCT_ZERO_INIT;
    stark_wr_cursor_t curr = LDG_STRUCT_ZERO_INIT;
    ldg_math_transcript_t script = LDG_STRUCT_ZERO_INIT;
    uint8_t seed[LDG_MATH_STARK_SEED_LEN] = LDG_ARR_ZERO_INIT;
    uint8_t main_root[LDG_MATH_MERKLE_NODE_SIZE] = LDG_ARR_ZERO_INIT;
    uint8_t aux_root[LDG_MATH_MERKLE_NODE_SIZE] = LDG_ARR_ZERO_INIT;
    uint8_t comp_root[LDG_MATH_MERKLE_NODE_SIZE] = LDG_ARR_ZERO_INIT;
    uint8_t layer_root[LDG_MATH_STARK_LAYER_MAX][LDG_MATH_MERKLE_NODE_SIZE] = LDG_ARR_ZERO_INIT;
    ldg_math_ext_t alpha[LDG_MATH_STARK_LAYER_MAX] = LDG_ARR_ZERO_INIT;
    ldg_math_ext_t challenge[LDG_MATH_AIR_CHALLENGE_MAX] = LDG_ARR_ZERO_INIT;
    ldg_math_ext_t ood_main_curr[LDG_MATH_AIR_COL_MAX] = LDG_ARR_ZERO_INIT;
    ldg_math_ext_t ood_main_next[LDG_MATH_AIR_COL_MAX] = LDG_ARR_ZERO_INIT;
    ldg_math_ext_t ood_aux_curr[LDG_MATH_AIR_AUX_COL_MAX] = LDG_ARR_ZERO_INIT;
    ldg_math_ext_t ood_aux_next[LDG_MATH_AIR_AUX_COL_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t layer_off[LDG_MATH_STARK_LAYER_MAX + LDG_MATH_STARK_PREV_OFF] = LDG_ARR_ZERO_INIT;
    uint64_t node_off[LDG_MATH_STARK_LAYER_MAX + LDG_MATH_STARK_PREV_OFF] = LDG_ARR_ZERO_INIT;
    uint64_t need = 0;
    uint64_t node_need = 0;
    uint64_t proof_need = 0;
    uint64_t ext_need = 0;
    uint64_t tree_size = 0;
    uint64_t idx = 0;
    uint64_t pos = 0;
    uint64_t half = 0;
    uint64_t curr_cunt = 0;
    uint64_t deep_need = 0;
    uint64_t fold_need = 0;
    uint64_t comp_base_cap = 0;
    uint64_t base_used = 0;
    uint64_t main_node_off = 0;
    uint64_t aux_node_off = 0;
    uint64_t comp_node_off = 0;
    uint64_t nonce = 0;
    uint64_t layer_path_size = 0;
    uint64_t periodic_need = 0;
    ldg_math_field_elem_t *periodic_lde = 0x0;
    ldg_math_field_elem_t *periodic_buff = 0x0;
    ldg_math_field_elem_t *coeff = 0x0;
    ldg_math_field_elem_t *aux_coeff = 0x0;
    ldg_math_field_elem_t *lde = 0x0;
    ldg_math_field_elem_t *aux_lde = 0x0;
    ldg_math_field_elem_t *aux_trace = 0x0;
    ldg_math_field_elem_t *tmp = 0x0;
    ldg_math_field_elem_t *lo = 0x0;
    ldg_math_field_elem_t *hi = 0x0;
    ldg_math_field_elem_t *fold_buff = 0x0;
    ldg_math_field_elem_t *twiddle_domain = 0x0;
    ldg_math_field_elem_t *twiddle_domain_inverse = 0x0;
    ldg_math_field_elem_t *twiddle_trace_inverse = 0x0;
    ldg_math_field_elem_t *comp_base_buff = 0x0;
    ldg_math_ext_t *comp = 0x0;
    ldg_math_ext_t *acc = 0x0;
    ldg_math_ext_t *tmp_ext = 0x0;
    ldg_math_ext_t *work_ext = 0x0;
    ldg_math_ext_t *deep_buff = 0x0;
    ldg_math_ext_t *layer_arr = 0x0;
    ldg_math_ext_t *comp_ext_buff = 0x0;
    ldg_math_ext_t beta = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t delta = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t pwr = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t ood_point = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t ood_next_point = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t ood_comp = LDG_STRUCT_ZERO_INIT;
    ldg_math_field_elem_t trace_root_unity = 0;
    ldg_math_field_elem_t curr_shift = 0;
    ldg_math_field_elem_t next_shift = 0;

    if (LDG_UNLIKELY(!air_cb || !trace || !buff || !node || !proof || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;
    ret = ldg_math_stark_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(conf->air.aux_col_cunt > 0 && !aux_cb)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->air.periodic_cunt > 0 && !periodic)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = stark_shape_get(conf, &shape);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stark_prove_buff_cunt_get(conf, &need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(buff_cunt < need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_stark_node_cap_get(conf, &node_need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(node_cap < node_need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_stark_proof_size_get(conf, &proof_need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(proof_cap < proof_need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_air_composition_ext_buff_cunt_get(&conf->air, &ext_need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_air_periodic_buff_cunt_get(&conf->air, &periodic_need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    deep_need = (shape.domain_cunt * LDG_MATH_STARK_TREE_FACTOR) + shape.domain_cunt;
    fold_need = (shape.domain_cunt / LDG_MATH_STARK_PAIR_STEP) * (LDG_MATH_STARK_TREE_FACTOR + LDG_MATH_STARK_PREV_OFF);
    comp_base_cap = (shape.blowup * LDG_MATH_STARK_TREE_FACTOR) + shape.blowup;

    coeff = buff;
    aux_coeff = coeff + (shape.col_coeff_cunt * conf->air.col_cunt);
    lde = aux_coeff + (shape.col_coeff_cunt * conf->air.aux_col_cunt);
    aux_lde = lde + (shape.domain_cunt * conf->air.col_cunt);
    aux_trace = aux_lde + (shape.domain_cunt * conf->air.aux_col_cunt);
    tmp = aux_trace + (shape.trace_cunt * conf->air.aux_col_cunt);
    lo = tmp + shape.domain_cunt;
    hi = lo + shape.domain_cunt;
    fold_buff = hi + shape.domain_cunt;
    twiddle_domain = fold_buff + fold_need;
    twiddle_domain_inverse = twiddle_domain + (shape.domain_cunt / LDG_MATH_STARK_PAIR_STEP);
    twiddle_trace_inverse = twiddle_domain_inverse + (shape.domain_cunt / LDG_MATH_STARK_PAIR_STEP);
    comp_base_buff = twiddle_trace_inverse + (shape.trace_cunt / LDG_MATH_STARK_PAIR_STEP);
    periodic_lde = comp_base_buff + comp_base_cap;
    periodic_buff = periodic_lde + (shape.domain_cunt * conf->air.periodic_cunt);
    base_used = (uint64_t)(periodic_buff - buff) + periodic_need;
    base_used += base_used & LDG_MATH_STARK_PREV_OFF;
    comp = (ldg_math_ext_t *)(buff + base_used);
    acc = comp + shape.domain_cunt;
    tmp_ext = acc + shape.domain_cunt;
    work_ext = tmp_ext + shape.domain_cunt;
    deep_buff = work_ext + shape.domain_cunt;
    layer_arr = deep_buff + deep_need;
    comp_ext_buff = layer_arr + (shape.domain_cunt * LDG_MATH_STARK_TREE_FACTOR);

    ret = ldg_math_ntt_twiddle_fill(shape.log_domain, LDG_TRUTH_FALSE, twiddle_domain, shape.domain_cunt / LDG_MATH_STARK_PAIR_STEP);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_twiddle_fill(shape.log_domain, LDG_TRUTH_TRUE, twiddle_domain_inverse, shape.domain_cunt / LDG_MATH_STARK_PAIR_STEP);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_twiddle_fill(conf->air.log_trace, LDG_TRUTH_TRUE, twiddle_trace_inverse, shape.trace_cunt / LDG_MATH_STARK_PAIR_STEP);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_air_segment_extend(&conf->air, trace, conf->air.col_cunt, rand, conf->air.blind_cunt, twiddle_trace_inverse, twiddle_domain, coeff, lde, tmp, shape.domain_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    tree_size = shape.domain_cunt * LDG_MATH_STARK_TREE_FACTOR * LDG_MATH_MERKLE_NODE_SIZE;
    main_node_off = 0;
    aux_node_off = tree_size;
    comp_node_off = tree_size * LDG_MATH_STARK_PAIR_STEP;

    if (conf->air.aux_col_cunt == 0) { comp_node_off = tree_size; }

    node_off[0] = comp_node_off + tree_size;

    ret = ldg_math_merkle_build(lde, shape.domain_cunt, conf->air.col_cunt, node + main_node_off, tree_size);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_merkle_root_get(node + main_node_off, shape.domain_cunt, main_root);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = stark_seed_fill(seed);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_init(&script, seed, LDG_MATH_STARK_SEED_LEN);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_elem_absorb(&script, pub, conf->air.pub_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_absorb(&script, main_root, LDG_MATH_MERKLE_NODE_SIZE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (conf->air.aux_col_cunt > 0)
    {
        for (idx = 0; idx < conf->air.challenge_cunt; idx++)
        {
            ret = ldg_math_transcript_ext_get(&script, &challenge[idx]);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = aux_cb(air_ctx, pub, challenge, trace, shape.trace_cunt, conf->air.col_cunt, aux_trace, conf->air.aux_col_cunt);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_air_segment_extend(&conf->air, aux_trace, conf->air.aux_col_cunt, rand + ((uint64_t)conf->air.blind_cunt * conf->air.col_cunt), conf->air.blind_cunt, twiddle_trace_inverse, twiddle_domain, aux_coeff, aux_lde, tmp, shape.domain_cunt);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_merkle_build(aux_lde, shape.domain_cunt, conf->air.aux_col_cunt, node + aux_node_off, tree_size);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_merkle_root_get(node + aux_node_off, shape.domain_cunt, aux_root);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_transcript_absorb(&script, aux_root, LDG_MATH_MERKLE_NODE_SIZE);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_math_transcript_ext_get(&script, &beta);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (conf->air.periodic_cunt > 0)
    {
        ret = ldg_math_air_periodic_extend(&conf->air, periodic, periodic_lde, periodic_buff, periodic_need);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_math_air_composition_build(&conf->air, air_cb, air_ctx, pub, challenge, lde, aux_lde, periodic_lde, beta, comp, comp_ext_buff, ext_need, comp_base_buff, comp_base_cap);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_merkle_build(&comp[0].lo, shape.domain_cunt, LDG_MATH_EXT_DEGREE, node + comp_node_off, tree_size);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_merkle_root_get(node + comp_node_off, shape.domain_cunt, comp_root);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_absorb(&script, comp_root, LDG_MATH_MERKLE_NODE_SIZE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = stark_ood_draw(&script, &shape, conf->air.coset_shift, &ood_point);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_field_root_get(conf->air.log_trace, &trace_root_unity);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ood_next_point = ldg_math_ext_scale(ood_point, trace_root_unity);

    for (col = 0; col < conf->air.col_cunt; col++)
    {
        ret = ldg_math_proof_ext_poly_evaluate_get(coeff + ((uint64_t)col * shape.col_coeff_cunt), shape.col_coeff_cunt, ood_point, &ood_main_curr[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_proof_ext_poly_evaluate_get(coeff + ((uint64_t)col * shape.col_coeff_cunt), shape.col_coeff_cunt, ood_next_point, &ood_main_next[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (col = 0; col < conf->air.aux_col_cunt; col++)
    {
        ret = ldg_math_proof_ext_poly_evaluate_get(aux_coeff + ((uint64_t)col * shape.col_coeff_cunt), shape.col_coeff_cunt, ood_point, &ood_aux_curr[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_proof_ext_poly_evaluate_get(aux_coeff + ((uint64_t)col * shape.col_coeff_cunt), shape.col_coeff_cunt, ood_next_point, &ood_aux_next[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < shape.domain_cunt; idx++)
    {
        lo[idx] = comp[idx].lo;
        hi[idx] = comp[idx].hi;
    }

    ret = ldg_math_ntt_coset_interpolate(lo, shape.log_domain, conf->air.coset_shift, twiddle_domain_inverse);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_ntt_coset_interpolate(hi, shape.log_domain, conf->air.coset_shift, twiddle_domain_inverse);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < shape.domain_cunt; idx++)
    {
        work_ext[idx].lo = lo[idx];
        work_ext[idx].hi = hi[idx];
    }

    ret = ldg_math_proof_ext_coeff_evaluate_get(work_ext, shape.domain_cunt, ood_point, &ood_comp);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_ext_absorb(&script, ood_main_curr, conf->air.col_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_ext_absorb(&script, ood_main_next, conf->air.col_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_ext_absorb(&script, ood_aux_curr, conf->air.aux_col_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_ext_absorb(&script, ood_aux_next, conf->air.aux_col_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_ext_absorb(&script, &ood_comp, LDG_MATH_STARK_SINGLE_ELEM);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_ext_get(&script, &delta);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (idx = 0; idx < shape.domain_cunt; idx++) { acc[idx] = ldg_math_ext_base_embed(LDG_MATH_FIELD_ADD_IDENT); }

    pwr = ldg_math_ext_base_embed(LDG_MATH_FIELD_MUL_IDENT);

    ret = stark_deep_cum(lde, &shape, conf->air.col_cunt, conf->air.coset_shift, ood_point, ood_main_curr, acc, tmp, tmp_ext, deep_buff, deep_need, delta, &pwr);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = stark_deep_cum(lde, &shape, conf->air.col_cunt, conf->air.coset_shift, ood_next_point, ood_main_next, acc, tmp, tmp_ext, deep_buff, deep_need, delta, &pwr);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (conf->air.aux_col_cunt > 0)
    {
        ret = stark_deep_cum(aux_lde, &shape, conf->air.aux_col_cunt, conf->air.coset_shift, ood_point, ood_aux_curr, acc, tmp, tmp_ext, deep_buff, deep_need, delta, &pwr);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = stark_deep_cum(aux_lde, &shape, conf->air.aux_col_cunt, conf->air.coset_shift, ood_next_point, ood_aux_next, acc, tmp, tmp_ext, deep_buff, deep_need, delta, &pwr);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < shape.domain_cunt; idx++) { tmp_ext[idx] = comp[idx]; }

    ret = ldg_math_proof_ext_deep_div(tmp_ext, shape.log_domain, conf->air.coset_shift, ood_point, ood_comp, deep_buff, deep_need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_proof_ext_poly_cum(acc, tmp_ext, shape.domain_cunt, pwr);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    layer_off[0] = 0;

    for (idx = 0; idx < shape.domain_cunt; idx++) { layer_arr[idx] = acc[idx]; }

    curr_cunt = shape.domain_cunt;
    curr_shift = ldg_math_field_reduce(conf->air.coset_shift);

    for (layer = 0; layer < shape.layer_cunt; layer++)
    {
        ret = ldg_math_merkle_build(&layer_arr[layer_off[layer]].lo, curr_cunt, LDG_MATH_EXT_DEGREE, node + node_off[layer], curr_cunt * LDG_MATH_STARK_TREE_FACTOR * LDG_MATH_MERKLE_NODE_SIZE);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_merkle_root_get(node + node_off[layer], curr_cunt, layer_root[layer]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_transcript_absorb(&script, layer_root[layer], LDG_MATH_MERKLE_NODE_SIZE);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_transcript_ext_get(&script, &alpha[layer]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        layer_off[layer + LDG_MATH_STARK_PREV_OFF] = layer_off[layer] + curr_cunt;
        node_off[layer + LDG_MATH_STARK_PREV_OFF] = node_off[layer] + (curr_cunt * LDG_MATH_STARK_TREE_FACTOR * LDG_MATH_MERKLE_NODE_SIZE);

        ret = ldg_math_fri_ext_fold(layer_arr + layer_off[layer], curr_cunt, curr_shift, alpha[layer], layer_arr + layer_off[layer + LDG_MATH_STARK_PREV_OFF], fold_buff, fold_need);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_fri_shift_next_get(curr_shift, &next_shift);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        curr_shift = next_shift;
        curr_cunt /= LDG_MATH_STARK_PAIR_STEP;
    }

    ret = ldg_math_transcript_ext_absorb(&script, layer_arr + layer_off[shape.layer_cunt], shape.remainder_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_grind_find(&script, conf->grind_bit_cunt, &nonce);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_elem_absorb(&script, &nonce, LDG_MATH_STARK_SINGLE_ELEM);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    curr.base = proof;
    curr.cap = proof_cap;
    curr.pos = 0;

    ret = stark_blob_st(&curr, main_root, LDG_MATH_MERKLE_NODE_SIZE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (conf->air.aux_col_cunt > 0)
    {
        ret = stark_blob_st(&curr, aux_root, LDG_MATH_MERKLE_NODE_SIZE);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = stark_blob_st(&curr, comp_root, LDG_MATH_MERKLE_NODE_SIZE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = stark_ext_st(&curr, ood_point);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (col = 0; col < conf->air.col_cunt; col++)
    {
        ret = stark_ext_st(&curr, ood_main_curr[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (col = 0; col < conf->air.col_cunt; col++)
    {
        ret = stark_ext_st(&curr, ood_main_next[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (col = 0; col < conf->air.aux_col_cunt; col++)
    {
        ret = stark_ext_st(&curr, ood_aux_curr[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (col = 0; col < conf->air.aux_col_cunt; col++)
    {
        ret = stark_ext_st(&curr, ood_aux_next[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = stark_ext_st(&curr, ood_comp);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (layer = 0; layer < shape.layer_cunt; layer++)
    {
        ret = stark_blob_st(&curr, layer_root[layer], LDG_MATH_MERKLE_NODE_SIZE);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < shape.remainder_cunt; idx++)
    {
        ret = stark_ext_st(&curr, layer_arr[layer_off[shape.layer_cunt] + idx]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = stark_word_st(&curr, nonce);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (query = 0; query < conf->query_cunt; query++)
    {
        ret = ldg_math_transcript_idx_get(&script, shape.domain_cunt, &pos);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = stark_segment_open(&curr, node + main_node_off, lde, &shape, conf->air.col_cunt, pos);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (conf->air.aux_col_cunt > 0)
        {
            ret = stark_segment_open(&curr, node + aux_node_off, aux_lde, &shape, conf->air.aux_col_cunt, pos);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = stark_ext_st(&curr, comp[pos]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = stark_path_st(&curr, node + comp_node_off, shape.domain_cunt, pos, shape.path_size);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        curr_cunt = shape.domain_cunt;

        for (layer = 0; layer < shape.layer_cunt; layer++)
        {
            half = curr_cunt / LDG_MATH_STARK_PAIR_STEP;
            idx = pos & (half - LDG_MATH_STARK_PREV_OFF);
            layer_path_size = (uint64_t)(shape.log_domain - layer) * LDG_MATH_MERKLE_NODE_SIZE;

            ret = stark_ext_st(&curr, layer_arr[layer_off[layer] + idx]);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = stark_ext_st(&curr, layer_arr[layer_off[layer] + idx + half]);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = stark_path_st(&curr, node + node_off[layer], curr_cunt, idx, layer_path_size);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = stark_path_st(&curr, node + node_off[layer], curr_cunt, idx + half, layer_path_size);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            pos = idx;
            curr_cunt = half;
        }
    }

    *out_len = curr.pos;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stark_verify(const ldg_math_stark_conf_t *conf, ldg_math_air_eval_cb_t air_cb, void *air_ctx, const ldg_math_field_elem_t *pub, const ldg_math_field_elem_t *periodic, const uint8_t *proof, uint64_t proof_len, ldg_math_field_elem_t *buff, uint64_t buff_cunt)
{
    uint64_t periodic_need = 0;
    ldg_math_field_elem_t *periodic_buff = 0x0;
    uint32_t ret = LDG_ERR_AOK;
    uint32_t col = 0;
    uint32_t layer = 0;
    uint32_t query = 0;
    uint32_t kdx = 0;
    stark_shape_t shape = LDG_STRUCT_ZERO_INIT;
    stark_rd_cursor_t curr = LDG_STRUCT_ZERO_INIT;
    ldg_math_transcript_t script = LDG_STRUCT_ZERO_INIT;
    ldg_math_air_row_t row = LDG_STRUCT_ZERO_INIT;
    uint8_t seed[LDG_MATH_STARK_SEED_LEN] = LDG_ARR_ZERO_INIT;
    const uint8_t *main_root = 0x0;
    const uint8_t *aux_root = 0x0;
    const uint8_t *comp_root = 0x0;
    const uint8_t *layer_root[LDG_MATH_STARK_LAYER_MAX] = LDG_ARR_ZERO_INIT;
    const uint8_t *pair_path_lo = 0x0;
    const uint8_t *pair_path_hi = 0x0;
    ldg_math_ext_t alpha[LDG_MATH_STARK_LAYER_MAX] = LDG_ARR_ZERO_INIT;
    ldg_math_ext_t challenge[LDG_MATH_AIR_CHALLENGE_MAX] = LDG_ARR_ZERO_INIT;
    ldg_math_ext_t ood_main_curr[LDG_MATH_AIR_COL_MAX] = LDG_ARR_ZERO_INIT;
    ldg_math_ext_t ood_main_next[LDG_MATH_AIR_COL_MAX] = LDG_ARR_ZERO_INIT;
    ldg_math_ext_t ood_aux_curr[LDG_MATH_AIR_AUX_COL_MAX] = LDG_ARR_ZERO_INIT;
    ldg_math_ext_t ood_aux_next[LDG_MATH_AIR_AUX_COL_MAX] = LDG_ARR_ZERO_INIT;
    ldg_math_field_elem_t layer_shift[LDG_MATH_STARK_LAYER_MAX + LDG_MATH_STARK_PREV_OFF] = LDG_ARR_ZERO_INIT;
    uint64_t need = 0;
    uint64_t proof_need = 0;
    uint64_t idx = 0;
    uint64_t pos = 0;
    uint64_t half = 0;
    uint64_t curr_cunt = 0;
    uint64_t nonce = 0;
    uint64_t base_used = 0;
    uint64_t layer_path_size = 0;
    uint64_t deep_bound = 0;
    uint64_t fold_cunt = 0;
    uint64_t remainder_bound = 0;
    ldg_math_field_elem_t *remainder_buff = 0x0;
    ldg_math_field_elem_t *twiddle_remainder_inverse = 0x0;
    ldg_math_field_elem_t *main_val = 0x0;
    ldg_math_field_elem_t *aux_val = 0x0;
    ldg_math_ext_t *remainder = 0x0;
    ldg_math_ext_t *eval_buff = 0x0;
    ldg_math_ext_t *periodic_ext = 0x0;
    ldg_math_ext_t *pub_ext = 0x0;
    ldg_math_ext_t beta = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t delta = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t pwr = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t ood_point = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t ood_next_point = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t ood_comp = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t derived_point = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t vanish = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t batched = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t comp_val = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t pair_lo = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t pair_hi = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t expected = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t folded = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t inverted_curr = LDG_STRUCT_ZERO_INIT;
    ldg_math_ext_t inverted_next = LDG_STRUCT_ZERO_INIT;
    ldg_math_field_elem_t trace_root_unity = 0;
    ldg_math_field_elem_t point = 0;
    ldg_math_field_elem_t next_shift = 0;

    if (LDG_UNLIKELY(!air_cb || !proof || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_math_stark_conf_ck(conf);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(conf->air.periodic_cunt > 0 && !periodic)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->air.pub_cunt > 0 && !pub)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = stark_shape_get(conf, &shape);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_air_periodic_buff_cunt_get(&conf->air, &periodic_need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_stark_verify_buff_cunt_get(conf, &need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(buff_cunt < need)) { return LDG_ERR_BOUNDS; }

    ret = ldg_math_stark_proof_size_get(conf, &proof_need);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(proof_len != proof_need)) { return LDG_ERR_INVALID; }

    remainder_buff = buff;
    twiddle_remainder_inverse = remainder_buff + (shape.remainder_cunt * LDG_MATH_EXT_DEGREE);
    main_val = twiddle_remainder_inverse + (shape.remainder_cunt / LDG_MATH_STARK_PAIR_STEP);
    aux_val = main_val + conf->air.col_cunt;
    periodic_buff = aux_val + conf->air.aux_col_cunt;
    base_used = (uint64_t)(periodic_buff - buff) + periodic_need;
    base_used += base_used & LDG_MATH_STARK_PREV_OFF;
    remainder = (ldg_math_ext_t *)(buff + base_used);
    eval_buff = remainder + shape.remainder_cunt;
    periodic_ext = eval_buff + conf->air.constraint_cunt;
    pub_ext = periodic_ext + conf->air.periodic_cunt;

    ret = ldg_math_ntt_twiddle_fill(conf->log_remainder, LDG_TRUTH_TRUE, twiddle_remainder_inverse, shape.remainder_cunt / LDG_MATH_STARK_PAIR_STEP);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    curr.base = proof;
    curr.cap = proof_len;
    curr.pos = 0;

    ret = stark_blob_ref_get(&curr, LDG_MATH_MERKLE_NODE_SIZE, &main_root);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (conf->air.aux_col_cunt > 0)
    {
        ret = stark_blob_ref_get(&curr, LDG_MATH_MERKLE_NODE_SIZE, &aux_root);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = stark_blob_ref_get(&curr, LDG_MATH_MERKLE_NODE_SIZE, &comp_root);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = stark_ext_ld(&curr, &ood_point);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (col = 0; col < conf->air.col_cunt; col++)
    {
        ret = stark_ext_ld(&curr, &ood_main_curr[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (col = 0; col < conf->air.col_cunt; col++)
    {
        ret = stark_ext_ld(&curr, &ood_main_next[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (col = 0; col < conf->air.aux_col_cunt; col++)
    {
        ret = stark_ext_ld(&curr, &ood_aux_curr[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (col = 0; col < conf->air.aux_col_cunt; col++)
    {
        ret = stark_ext_ld(&curr, &ood_aux_next[col]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = stark_ext_ld(&curr, &ood_comp);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (layer = 0; layer < shape.layer_cunt; layer++)
    {
        ret = stark_blob_ref_get(&curr, LDG_MATH_MERKLE_NODE_SIZE, &layer_root[layer]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    for (idx = 0; idx < shape.remainder_cunt; idx++)
    {
        ret = stark_ext_ld(&curr, &remainder[idx]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = stark_word_ld(&curr, &nonce);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = stark_seed_fill(seed);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_init(&script, seed, LDG_MATH_STARK_SEED_LEN);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_elem_absorb(&script, pub, conf->air.pub_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_absorb(&script, main_root, LDG_MATH_MERKLE_NODE_SIZE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (conf->air.aux_col_cunt > 0)
    {
        for (idx = 0; idx < conf->air.challenge_cunt; idx++)
        {
            ret = ldg_math_transcript_ext_get(&script, &challenge[idx]);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = ldg_math_transcript_absorb(&script, aux_root, LDG_MATH_MERKLE_NODE_SIZE);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    ret = ldg_math_transcript_ext_get(&script, &beta);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_absorb(&script, comp_root, LDG_MATH_MERKLE_NODE_SIZE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = stark_ood_draw(&script, &shape, conf->air.coset_shift, &derived_point);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ldg_math_ext_eq_is(derived_point, ood_point) != LDG_TRUTH_TRUE)) { return LDG_ERR_INVALID; }

    ret = ldg_math_transcript_ext_absorb(&script, ood_main_curr, conf->air.col_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_ext_absorb(&script, ood_main_next, conf->air.col_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_ext_absorb(&script, ood_aux_curr, conf->air.aux_col_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_ext_absorb(&script, ood_aux_next, conf->air.aux_col_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_ext_absorb(&script, &ood_comp, LDG_MATH_STARK_SINGLE_ELEM);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_ext_get(&script, &delta);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    layer_shift[0] = ldg_math_field_reduce(conf->air.coset_shift);

    for (layer = 0; layer < shape.layer_cunt; layer++)
    {
        ret = ldg_math_transcript_absorb(&script, layer_root[layer], LDG_MATH_MERKLE_NODE_SIZE);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_transcript_ext_get(&script, &alpha[layer]);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_fri_shift_next_get(layer_shift[layer], &next_shift);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        layer_shift[layer + LDG_MATH_STARK_PREV_OFF] = next_shift;
    }

    ret = ldg_math_transcript_ext_absorb(&script, remainder, shape.remainder_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_grind_ck(&script, conf->grind_bit_cunt, nonce);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_transcript_elem_absorb(&script, &nonce, LDG_MATH_STARK_SINGLE_ELEM);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_math_field_root_get(conf->air.log_trace, &trace_root_unity);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ood_next_point = ldg_math_ext_scale(ood_point, trace_root_unity);

    for (col = 0; col < conf->air.pub_cunt; col++) { pub_ext[col] = ldg_math_ext_base_embed(pub[col]); }

    if (conf->air.periodic_cunt > 0)
    {
        ret = ldg_math_air_periodic_evaluate(&conf->air, periodic, ood_point, periodic_ext, periodic_buff, periodic_need);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }

    row.point = ood_point;
    row.main_curr = ood_main_curr;
    row.main_next = ood_main_next;
    row.pub = pub_ext;
    row.challenge = challenge;

    if (conf->air.aux_col_cunt > 0)
    {
        row.aux_curr = ood_aux_curr;
        row.aux_next = ood_aux_next;
    }

    if (conf->air.periodic_cunt > 0) { row.periodic = periodic_ext; }

    ret = air_cb(air_ctx, &row, eval_buff, conf->air.constraint_cunt);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    batched = ldg_math_ext_base_embed(LDG_MATH_FIELD_ADD_IDENT);
    pwr = ldg_math_ext_base_embed(LDG_MATH_FIELD_MUL_IDENT);

    for (kdx = 0; kdx < conf->air.constraint_cunt; kdx++)
    {
        batched = ldg_math_ext_add(batched, ldg_math_ext_mul(eval_buff[kdx], pwr));
        pwr = ldg_math_ext_mul(pwr, beta);
    }

    ret = ldg_math_proof_ext_vanish_evaluate_get(conf->air.log_trace, ood_point, &vanish);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(ldg_math_ext_eq_is(batched, ldg_math_ext_mul(ood_comp, vanish)) != LDG_TRUTH_TRUE)) { return LDG_ERR_INVALID; }

    ret = ldg_math_air_deep_degree_bound_get(&conf->air, &deep_bound);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(shape.remainder_cunt == 0 || shape.domain_cunt < shape.remainder_cunt)) { return LDG_ERR_INVALID; }

    fold_cunt = shape.domain_cunt / shape.remainder_cunt;
    remainder_bound = (deep_bound + fold_cunt - LDG_MATH_STARK_CEIL_BIAS) / fold_cunt;

    if (LDG_UNLIKELY(remainder_bound > shape.remainder_cunt)) { return LDG_ERR_INVALID; }

    ret = ldg_math_fri_ext_remainder_ck(remainder, shape.remainder_cunt, layer_shift[shape.layer_cunt], remainder_bound, twiddle_remainder_inverse, remainder_buff, shape.remainder_cunt * LDG_MATH_EXT_DEGREE);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    for (query = 0; query < conf->query_cunt; query++)
    {
        ret = ldg_math_transcript_idx_get(&script, shape.domain_cunt, &pos);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = stark_segment_rd_ck(&curr, main_root, &shape, conf->air.col_cunt, pos, main_val);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (conf->air.aux_col_cunt > 0)
        {
            ret = stark_segment_rd_ck(&curr, aux_root, &shape, conf->air.aux_col_cunt, pos, aux_val);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
        }

        ret = stark_ext_rd_ck(&curr, comp_root, shape.domain_cunt, pos, shape.path_size, &comp_val);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_proof_domain_point_get(shape.log_domain, conf->air.coset_shift, pos, &point);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_ext_invert_get(ldg_math_ext_sub(ldg_math_ext_base_embed(point), ood_point), &inverted_curr);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        ret = ldg_math_ext_invert_get(ldg_math_ext_sub(ldg_math_ext_base_embed(point), ood_next_point), &inverted_next);

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        expected = ldg_math_ext_base_embed(LDG_MATH_FIELD_ADD_IDENT);
        pwr = ldg_math_ext_base_embed(LDG_MATH_FIELD_MUL_IDENT);

        expected = stark_deep_point_cum(main_val, ood_main_curr, conf->air.col_cunt, inverted_curr, delta, &pwr, expected);
        expected = stark_deep_point_cum(main_val, ood_main_next, conf->air.col_cunt, inverted_next, delta, &pwr, expected);

        if (conf->air.aux_col_cunt > 0)
        {
            expected = stark_deep_point_cum(aux_val, ood_aux_curr, conf->air.aux_col_cunt, inverted_curr, delta, &pwr, expected);
            expected = stark_deep_point_cum(aux_val, ood_aux_next, conf->air.aux_col_cunt, inverted_next, delta, &pwr, expected);
        }

        expected = ldg_math_ext_add(expected, ldg_math_ext_mul(pwr, ldg_math_ext_mul(ldg_math_ext_sub(comp_val, ood_comp), inverted_curr)));

        curr_cunt = shape.domain_cunt;

        for (layer = 0; layer < shape.layer_cunt; layer++)
        {
            half = curr_cunt / LDG_MATH_STARK_PAIR_STEP;
            idx = pos & (half - LDG_MATH_STARK_PREV_OFF);
            layer_path_size = (uint64_t)(shape.log_domain - layer) * LDG_MATH_MERKLE_NODE_SIZE;

            ret = stark_ext_ld(&curr, &pair_lo);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = stark_ext_ld(&curr, &pair_hi);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = stark_blob_ref_get(&curr, layer_path_size, &pair_path_lo);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = stark_blob_ref_get(&curr, layer_path_size, &pair_path_hi);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = ldg_math_merkle_path_ck(layer_root[layer], &pair_lo.lo, LDG_MATH_EXT_DEGREE, idx, curr_cunt, pair_path_lo, layer_path_size);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = ldg_math_merkle_path_ck(layer_root[layer], &pair_hi.lo, LDG_MATH_EXT_DEGREE, idx + half, curr_cunt, pair_path_hi, layer_path_size);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            if (pos < half) { if (LDG_UNLIKELY(ldg_math_ext_eq_is(expected, pair_lo) != LDG_TRUTH_TRUE)) { return LDG_ERR_INVALID; } }
            else { if (LDG_UNLIKELY(ldg_math_ext_eq_is(expected, pair_hi) != LDG_TRUTH_TRUE)) { return LDG_ERR_INVALID; } }

            ret = ldg_math_proof_domain_point_get(shape.log_domain - layer, layer_shift[layer], idx, &point);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            ret = ldg_math_fri_ext_fold_pair_get(pair_lo, pair_hi, point, alpha[layer], &folded);

            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            expected = folded;
            pos = idx;
            curr_cunt = half;
        }

        if (LDG_UNLIKELY(ldg_math_ext_eq_is(expected, remainder[pos]) != LDG_TRUTH_TRUE)) { return LDG_ERR_INVALID; }
    }

    if (LDG_UNLIKELY(curr.pos != proof_len)) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}
