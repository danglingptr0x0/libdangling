#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/math.h>
#include <dangling/math/stat.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/cpy.h>

#define SENTENCEPIECE_NONE UINT32_MAX
#define SENTENCEPIECE_WIRE_VARINT 0
#define SENTENCEPIECE_WIRE_FIXED64 1
#define SENTENCEPIECE_WIRE_SPAN 2
#define SENTENCEPIECE_WIRE_FIXED32 5
#define SENTENCEPIECE_WIRE_MASK 0x7U
#define SENTENCEPIECE_WIRE_SHIFT 3
#define SENTENCEPIECE_VARINT_DATA_MASK 0x7FU
#define SENTENCEPIECE_VARINT_MORE_BIT 0x80U
#define SENTENCEPIECE_VARINT_SHIFT 7
#define SENTENCEPIECE_VARINT_BITS 64
#define SENTENCEPIECE_FIXED32_SIZE 4
#define SENTENCEPIECE_FIXED64_SIZE 8
#define SENTENCEPIECE_FIELD_PIECE 1
#define SENTENCEPIECE_FIELD_TRAINER 2
#define SENTENCEPIECE_FIELD_NORMALIZER 3
#define SENTENCEPIECE_FIELD_PIECE_TEXT 1
#define SENTENCEPIECE_FIELD_PIECE_SCORE 2
#define SENTENCEPIECE_FIELD_PIECE_KIND 3
#define SENTENCEPIECE_FIELD_MODEL_KIND 3
#define SENTENCEPIECE_FIELD_CHARSMAP 2
#define SENTENCEPIECE_FIELD_DUMMY_PREFIX 3
#define SENTENCEPIECE_FIELD_EXTRA_SPACE 4
#define SENTENCEPIECE_FIELD_ESCAPE_SPACE 5
#define SENTENCEPIECE_MODEL_KIND_UNIGRAM 1
#define SENTENCEPIECE_MODEL_KIND_BPE 2
#define SENTENCEPIECE_KIND_NORMAL 1
#define SENTENCEPIECE_KIND_UNKNOWN 2
#define SENTENCEPIECE_KIND_USER 4
#define SENTENCEPIECE_KIND_UNUSED 5
#define SENTENCEPIECE_KIND_BYTE 6
#define SENTENCEPIECE_SPACE 0x20
#define SENTENCEPIECE_MARKER_LEAD 0xE2
#define SENTENCEPIECE_MARKER_MID 0x96
#define SENTENCEPIECE_MARKER_TAIL 0x81
#define SENTENCEPIECE_MARKER_SIZE 3
#define SENTENCEPIECE_HASH_BASIS 2166136261U
#define SENTENCEPIECE_HASH_PRIME 16777619U
#define SENTENCEPIECE_SLOT_MIN 1024U
#define SENTENCEPIECE_SLOT_PER_PIECE 2U
#define SENTENCEPIECE_DOUBLE_SHIFT 1
#define SENTENCEPIECE_BYTE_SHIFT 8
#define SENTENCEPIECE_SCORE_SIGN_BIT 0x80000000U
#define SENTENCEPIECE_SCORE_EXP_SHIFT 23
#define SENTENCEPIECE_SCORE_EXP_MASK 0xFFU
#define SENTENCEPIECE_SCORE_MANT_MASK 0x7FFFFFU
#define SENTENCEPIECE_SCORE_MANT_TOP 0x800000U
#define SENTENCEPIECE_SCORE_EXP_BIAS 150
#define SENTENCEPIECE_SCORE_EXP_DENORMAL_FIELD 1
#define SENTENCEPIECE_FROZEN_BIT 0x80000000U
#define SENTENCEPIECE_FROZEN_NONE 0x0U
#define SENTENCEPIECE_END_MASK 0x7FFFFFFFU
#define SENTENCEPIECE_TEXT_MAX 0x1FFFFFFFU
#define SENTENCEPIECE_BUFF_PER_BYTE 3
#define SENTENCEPIECE_ADJACENT_OFF 1
#define SENTENCEPIECE_ASCII_END 0x80
#define SENTENCEPIECE_PAIR_LEAD_MIN 0xC2
#define SENTENCEPIECE_TRIPLE_LEAD_MIN 0xE0
#define SENTENCEPIECE_QUAD_LEAD_MIN 0xF0
#define SENTENCEPIECE_LEAD_END 0xF5
#define SENTENCEPIECE_TAIL_MASK 0xC0
#define SENTENCEPIECE_TAIL_MARK 0x80
#define SENTENCEPIECE_SPAN_NONE 0
#define SENTENCEPIECE_PLAIN_SIZE 1
#define SENTENCEPIECE_PAIR_SIZE 2
#define SENTENCEPIECE_TRIPLE_SIZE 3
#define SENTENCEPIECE_QUAD_SIZE 4

typedef struct sentencepiece_cursor
{
    const uint8_t *data;
    uint64_t len;
    uint64_t pos;
} sentencepiece_cursor_t;

typedef struct sentencepiece_piece
{
    const uint8_t *text;
    double score;
    uint32_t len;
    uint32_t kind;
} sentencepiece_piece_t;

typedef struct sentencepiece_spec
{
    uint64_t blob_size;
    uint64_t charsmap_len;
    uint32_t piece_cunt;
    uint32_t model_kind;
    uint8_t dummy_prefix_is;
    uint8_t extra_space_is;
    uint8_t escape_space_is;
    uint8_t pudding[5];
} sentencepiece_spec_t;

static uint32_t sentencepiece_alloc(uint64_t size, void **out)
{
    uint32_t ret = 0;

    *out = 0x0;

    ret = ldg_mem_alloc(size, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_mem_zero(*out, size);
}

static void sentencepiece_free(void *ptr)
{
    if (ptr) { ldg_mem_dealloc(ptr); }
}

static void sentencepiece_buff_free(ldg_math_stat_sentencepiece_t *model)
{
    sentencepiece_free(model->slot);
    sentencepiece_free(model->kind);
    sentencepiece_free(model->score);
    sentencepiece_free(model->piece_off);
    sentencepiece_free(model->blob);
}

static uint32_t sentencepiece_init_ck(const ldg_math_stat_sentencepiece_t *model)
{
    if (LDG_UNLIKELY(!model)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(model->is_init != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    return LDG_ERR_AOK;
}

static uint32_t sentencepiece_varint_rd(sentencepiece_cursor_t *curr, uint64_t *out)
{
    uint64_t val = 0;
    uint32_t shift = 0;
    uint8_t byte = SENTENCEPIECE_VARINT_MORE_BIT;

    *out = 0;

    while ((byte & SENTENCEPIECE_VARINT_MORE_BIT) != 0)
    {
        if (LDG_UNLIKELY(curr->pos >= curr->len || shift >= SENTENCEPIECE_VARINT_BITS)) { return LDG_ERR_INVALID; }

        byte = curr->data[curr->pos];
        curr->pos++;
        val |= (uint64_t)(byte & SENTENCEPIECE_VARINT_DATA_MASK) << shift;
        shift += SENTENCEPIECE_VARINT_SHIFT;
    }

    *out = val;

    return LDG_ERR_AOK;
}

static uint32_t sentencepiece_span_rd(sentencepiece_cursor_t *curr, sentencepiece_cursor_t *out)
{
    uint64_t len = 0;
    uint32_t ret = sentencepiece_varint_rd(curr, &len);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(len > (curr->len - curr->pos))) { return LDG_ERR_INVALID; }

    out->data = curr->data + curr->pos;
    out->len = len;
    out->pos = 0;
    curr->pos += len;

    return LDG_ERR_AOK;
}

static uint32_t sentencepiece_fixed_skip(sentencepiece_cursor_t *curr, uint64_t size)
{
    if (LDG_UNLIKELY(size > (curr->len - curr->pos))) { return LDG_ERR_INVALID; }

    curr->pos += size;

    return LDG_ERR_AOK;
}

static uint32_t sentencepiece_field_skip(sentencepiece_cursor_t *curr, uint32_t wire)
{
    sentencepiece_cursor_t body = LDG_STRUCT_ZERO_INIT;
    uint64_t val = 0;

    if (wire == SENTENCEPIECE_WIRE_VARINT) { return sentencepiece_varint_rd(curr, &val); }

    if (wire == SENTENCEPIECE_WIRE_FIXED64) { return sentencepiece_fixed_skip(curr, SENTENCEPIECE_FIXED64_SIZE); }

    if (wire == SENTENCEPIECE_WIRE_SPAN) { return sentencepiece_span_rd(curr, &body); }

    if (wire == SENTENCEPIECE_WIRE_FIXED32) { return sentencepiece_fixed_skip(curr, SENTENCEPIECE_FIXED32_SIZE); }

    return LDG_ERR_INVALID;
}

static uint32_t sentencepiece_tag_rd(sentencepiece_cursor_t *curr, uint32_t *out_field, uint32_t *out_wire)
{
    uint64_t tag = 0;
    uint32_t ret = sentencepiece_varint_rd(curr, &tag);

    *out_field = (uint32_t)(tag >> SENTENCEPIECE_WIRE_SHIFT);
    *out_wire = (uint32_t)(tag & SENTENCEPIECE_WIRE_MASK);

    return ret;
}

static uint32_t sentencepiece_score_rd(sentencepiece_cursor_t *curr, double *out)
{
    uint32_t bits = 0;
    uint32_t exponent = 0;
    uint32_t mant = 0;
    uint32_t idx = SENTENCEPIECE_FIXED32_SIZE;
    double val = 0.0;

    if (LDG_UNLIKELY((curr->len - curr->pos) < SENTENCEPIECE_FIXED32_SIZE)) { return LDG_ERR_INVALID; }

    while (idx > 0)
    {
        idx--;
        bits = (bits << SENTENCEPIECE_BYTE_SHIFT) | (uint32_t)curr->data[curr->pos + idx];
    }

    curr->pos += SENTENCEPIECE_FIXED32_SIZE;
    exponent = (bits >> SENTENCEPIECE_SCORE_EXP_SHIFT) & SENTENCEPIECE_SCORE_EXP_MASK;
    mant = bits & SENTENCEPIECE_SCORE_MANT_MASK;

    if (LDG_UNLIKELY(exponent == SENTENCEPIECE_SCORE_EXP_MASK)) { return LDG_ERR_INVALID; }

    if (exponent == 0) { exponent = SENTENCEPIECE_SCORE_EXP_DENORMAL_FIELD; }
    else { mant |= SENTENCEPIECE_SCORE_MANT_TOP; }

    val = ldg_math_pwr2_scale_get((double)mant, (int32_t)exponent - SENTENCEPIECE_SCORE_EXP_BIAS);

    *out = ((bits & SENTENCEPIECE_SCORE_SIGN_BIT) != 0) ? -val : val;

    return LDG_ERR_AOK;
}

static uint32_t sentencepiece_piece_rd(sentencepiece_cursor_t *body, sentencepiece_piece_t *out)
{
    sentencepiece_cursor_t text = LDG_STRUCT_ZERO_INIT;
    uint64_t val = 0;
    uint32_t field = 0;
    uint32_t wire = 0;
    uint32_t ret = LDG_ERR_AOK;

    out->text = body->data;
    out->score = 0.0;
    out->len = 0;
    out->kind = SENTENCEPIECE_KIND_NORMAL;

    while (ret == LDG_ERR_AOK && body->pos < body->len)
    {
        ret = sentencepiece_tag_rd(body, &field, &wire);

        if (ret != LDG_ERR_AOK) { return ret; }

        if (field == SENTENCEPIECE_FIELD_PIECE_TEXT && wire == SENTENCEPIECE_WIRE_SPAN)
        {
            ret = sentencepiece_span_rd(body, &text);
            out->text = text.data;
            out->len = (uint32_t)text.len;
        }
        else if (field == SENTENCEPIECE_FIELD_PIECE_SCORE && wire == SENTENCEPIECE_WIRE_FIXED32) { ret = sentencepiece_score_rd(body, &out->score); }
        else if (field == SENTENCEPIECE_FIELD_PIECE_KIND && wire == SENTENCEPIECE_WIRE_VARINT)
        {
            ret = sentencepiece_varint_rd(body, &val);
            out->kind = (uint32_t)val;
        }
        else { ret = sentencepiece_field_skip(body, wire); }
    }

    if (LDG_UNLIKELY(ret == LDG_ERR_AOK && text.len > UINT32_MAX)) { return LDG_ERR_INVALID; }

    return ret;
}

static uint32_t sentencepiece_trainer_rd(sentencepiece_cursor_t *body, sentencepiece_spec_t *spec)
{
    uint64_t val = 0;
    uint32_t field = 0;
    uint32_t wire = 0;
    uint32_t ret = LDG_ERR_AOK;

    while (ret == LDG_ERR_AOK && body->pos < body->len)
    {
        ret = sentencepiece_tag_rd(body, &field, &wire);

        if (ret != LDG_ERR_AOK) { return ret; }

        if (field == SENTENCEPIECE_FIELD_MODEL_KIND && wire == SENTENCEPIECE_WIRE_VARINT)
        {
            ret = sentencepiece_varint_rd(body, &val);
            spec->model_kind = (uint32_t)val;
        }
        else { ret = sentencepiece_field_skip(body, wire); }
    }

    return ret;
}

static uint32_t sentencepiece_normalizer_rd(sentencepiece_cursor_t *body, sentencepiece_spec_t *spec)
{
    sentencepiece_cursor_t map = LDG_STRUCT_ZERO_INIT;
    uint64_t val = 0;
    uint32_t field = 0;
    uint32_t wire = 0;
    uint32_t ret = LDG_ERR_AOK;

    while (ret == LDG_ERR_AOK && body->pos < body->len)
    {
        ret = sentencepiece_tag_rd(body, &field, &wire);

        if (ret != LDG_ERR_AOK) { return ret; }

        if (field == SENTENCEPIECE_FIELD_CHARSMAP && wire == SENTENCEPIECE_WIRE_SPAN)
        {
            ret = sentencepiece_span_rd(body, &map);
            spec->charsmap_len = map.len;
        }
        else if (wire == SENTENCEPIECE_WIRE_VARINT)
        {
            ret = sentencepiece_varint_rd(body, &val);

            if (field == SENTENCEPIECE_FIELD_DUMMY_PREFIX) { spec->dummy_prefix_is = (val != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

            if (field == SENTENCEPIECE_FIELD_EXTRA_SPACE) { spec->extra_space_is = (val != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }

            if (field == SENTENCEPIECE_FIELD_ESCAPE_SPACE) { spec->escape_space_is = (val != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }
        }
        else { ret = sentencepiece_field_skip(body, wire); }
    }

    return ret;
}

static uint32_t sentencepiece_hash_step(uint32_t hash, uint8_t byte)
{
    return (hash ^ (uint32_t)byte) * SENTENCEPIECE_HASH_PRIME;
}

static uint32_t sentencepiece_raw_hash_get(const uint8_t *data, uint32_t len)
{
    uint32_t hash = SENTENCEPIECE_HASH_BASIS;
    uint32_t idx = 0;

    while (idx < len)
    {
        hash = sentencepiece_hash_step(hash, data[idx]);
        idx++;
    }

    return hash;
}

static uint32_t sentencepiece_span_hash_get(const uint8_t *text, uint32_t start, uint32_t end)
{
    uint32_t hash = SENTENCEPIECE_HASH_BASIS;
    uint32_t idx = start;

    while (idx < end)
    {
        if (text[idx] == SENTENCEPIECE_SPACE)
        {
            hash = sentencepiece_hash_step(hash, SENTENCEPIECE_MARKER_LEAD);
            hash = sentencepiece_hash_step(hash, SENTENCEPIECE_MARKER_MID);
            hash = sentencepiece_hash_step(hash, SENTENCEPIECE_MARKER_TAIL);
        }
        else { hash = sentencepiece_hash_step(hash, text[idx]); }

        idx++;
    }

    return hash;
}

static uint8_t sentencepiece_raw_match_is(const ldg_math_stat_sentencepiece_t *model, uint32_t piece, const uint8_t *data, uint32_t len)
{
    const uint8_t *held = model->blob + model->piece_off[piece];
    uint32_t idx = 0;

    if ((model->piece_off[piece + SENTENCEPIECE_ADJACENT_OFF] - model->piece_off[piece]) != len) { return LDG_TRUTH_FALSE; }

    while (idx < len)
    {
        if (held[idx] != data[idx]) { return LDG_TRUTH_FALSE; }

        idx++;
    }

    return LDG_TRUTH_TRUE;
}

static uint8_t sentencepiece_span_match_is(const ldg_math_stat_sentencepiece_t *model, uint32_t piece, const uint8_t *text, uint32_t start, uint32_t end)
{
    const uint8_t *held = model->blob + model->piece_off[piece];
    uint32_t held_len = model->piece_off[piece + SENTENCEPIECE_ADJACENT_OFF] - model->piece_off[piece];
    uint32_t at = 0;
    uint32_t idx = start;

    while (idx < end)
    {
        if (text[idx] == SENTENCEPIECE_SPACE)
        {
            if ((at + SENTENCEPIECE_MARKER_SIZE) > held_len || held[at] != SENTENCEPIECE_MARKER_LEAD || held[at + SENTENCEPIECE_ADJACENT_OFF] != SENTENCEPIECE_MARKER_MID || held[at + SENTENCEPIECE_PAIR_SIZE] != SENTENCEPIECE_MARKER_TAIL) { return LDG_TRUTH_FALSE; }

            at += SENTENCEPIECE_MARKER_SIZE;
        }
        else
        {
            if (at >= held_len || held[at] != text[idx]) { return LDG_TRUTH_FALSE; }

            at++;
        }

        idx++;
    }

    return (at == held_len) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t sentencepiece_span_id_get(const ldg_math_stat_sentencepiece_t *model, const uint8_t *text, uint32_t start, uint32_t end)
{
    uint32_t mask = model->slot_cunt - SENTENCEPIECE_ADJACENT_OFF;
    uint32_t at = sentencepiece_span_hash_get(text, start, end) & mask;

    while (model->slot[at] != SENTENCEPIECE_NONE)
    {
        if (sentencepiece_span_match_is(model, model->slot[at], text, start, end) == LDG_TRUTH_TRUE) { return model->slot[at]; }

        at = (at + SENTENCEPIECE_ADJACENT_OFF) & mask;
    }

    return SENTENCEPIECE_NONE;
}

static void sentencepiece_slot_put(ldg_math_stat_sentencepiece_t *model, uint32_t piece, const uint8_t *data, uint32_t len)
{
    uint32_t mask = model->slot_cunt - SENTENCEPIECE_ADJACENT_OFF;
    uint32_t at = sentencepiece_raw_hash_get(data, len) & mask;

    while (model->slot[at] != SENTENCEPIECE_NONE)
    {
        if (sentencepiece_raw_match_is(model, model->slot[at], data, len) == LDG_TRUTH_TRUE) { return; }

        at = (at + SENTENCEPIECE_ADJACENT_OFF) & mask;
    }

    model->slot[at] = piece;
}

static void sentencepiece_user_first_put(ldg_math_stat_sentencepiece_t *model, const uint8_t *data, uint32_t len)
{
    uint16_t span = (len > UINT16_MAX) ? UINT16_MAX : (uint16_t)len;

    if (len == 0) { return; }

    if (model->user_first[data[0]] < span) { model->user_first[data[0]] = span; }

    if (len >= SENTENCEPIECE_MARKER_SIZE && data[0] == SENTENCEPIECE_MARKER_LEAD && data[SENTENCEPIECE_ADJACENT_OFF] == SENTENCEPIECE_MARKER_MID && data[SENTENCEPIECE_PAIR_SIZE] == SENTENCEPIECE_MARKER_TAIL && model->user_first[SENTENCEPIECE_SPACE] < span) { model->user_first[SENTENCEPIECE_SPACE] = span; }
}

static void sentencepiece_piece_put(ldg_math_stat_sentencepiece_t *model, uint32_t piece, const sentencepiece_piece_t *held, uint32_t *blob_pos)
{
    uint8_t *dst = model->blob + *blob_pos;
    uint32_t idx = 0;

    while (idx < held->len)
    {
        dst[idx] = held->text[idx];
        idx++;
    }

    model->piece_off[piece] = *blob_pos;
    *blob_pos += held->len;
    model->piece_off[piece + SENTENCEPIECE_ADJACENT_OFF] = *blob_pos;
    model->score[piece] = held->score;
    model->kind[piece] = (uint8_t)held->kind;

    if (held->kind == SENTENCEPIECE_KIND_UNKNOWN) { model->unk_id = piece; }

    if (held->kind == SENTENCEPIECE_KIND_BYTE)
    {
        if (model->byte_base == SENTENCEPIECE_NONE) { model->byte_base = piece; }

        model->byte_cunt++;
    }

    if (held->kind == SENTENCEPIECE_KIND_NORMAL || held->kind == SENTENCEPIECE_KIND_USER || held->kind == SENTENCEPIECE_KIND_UNUSED) { sentencepiece_slot_put(model, piece, dst, held->len); }

    if (held->kind == SENTENCEPIECE_KIND_USER) { sentencepiece_user_first_put(model, dst, held->len); }
}

static uint32_t sentencepiece_model_walk(const uint8_t *data, uint64_t len, ldg_math_stat_sentencepiece_t *model, uint8_t fill_is, sentencepiece_spec_t *spec)
{
    sentencepiece_cursor_t curr = LDG_STRUCT_ZERO_INIT;
    sentencepiece_cursor_t body = LDG_STRUCT_ZERO_INIT;
    sentencepiece_piece_t held = LDG_STRUCT_ZERO_INIT;
    uint32_t blob_pos = 0;
    uint32_t piece = 0;
    uint32_t field = 0;
    uint32_t wire = 0;
    uint32_t ret = LDG_ERR_AOK;

    curr.data = data;
    curr.len = len;

    while (ret == LDG_ERR_AOK && curr.pos < curr.len)
    {
        ret = sentencepiece_tag_rd(&curr, &field, &wire);

        if (ret != LDG_ERR_AOK) { return ret; }

        if (wire != SENTENCEPIECE_WIRE_SPAN) { ret = sentencepiece_field_skip(&curr, wire); }
        else
        {
            ret = sentencepiece_span_rd(&curr, &body);

            if (ret == LDG_ERR_AOK && field == SENTENCEPIECE_FIELD_PIECE)
            {
                ret = sentencepiece_piece_rd(&body, &held);

                if (ret == LDG_ERR_AOK && fill_is) { sentencepiece_piece_put(model, piece, &held, &blob_pos); }

                if (ret == LDG_ERR_AOK && !fill_is)
                {
                    spec->piece_cunt++;
                    spec->blob_size += held.len;
                }

                piece++;
            }

            if (ret == LDG_ERR_AOK && field == SENTENCEPIECE_FIELD_TRAINER && !fill_is) { ret = sentencepiece_trainer_rd(&body, spec); }

            if (ret == LDG_ERR_AOK && field == SENTENCEPIECE_FIELD_NORMALIZER && !fill_is) { ret = sentencepiece_normalizer_rd(&body, spec); }
        }
    }

    return ret;
}

static uint32_t sentencepiece_spec_ck(const sentencepiece_spec_t *spec)
{
    if (LDG_UNLIKELY(spec->piece_cunt == 0 || spec->piece_cunt >= (SENTENCEPIECE_NONE / SENTENCEPIECE_SLOT_PER_PIECE) || spec->blob_size > UINT32_MAX)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(spec->model_kind != SENTENCEPIECE_MODEL_KIND_BPE)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(spec->charsmap_len != 0 || spec->dummy_prefix_is || spec->extra_space_is || !spec->escape_space_is)) { return LDG_ERR_UNSUPPORTED; }

    return LDG_ERR_AOK;
}

static uint32_t sentencepiece_table_alloc(ldg_math_stat_sentencepiece_t *model, const sentencepiece_spec_t *spec)
{
    void *ptr = 0x0;
    uint64_t piece_cunt = (uint64_t)spec->piece_cunt;
    uint32_t idx = 0;
    uint32_t ret = 0;

    model->piece_cunt = spec->piece_cunt;
    model->slot_cunt = SENTENCEPIECE_SLOT_MIN;

    while (model->slot_cunt < (spec->piece_cunt * SENTENCEPIECE_SLOT_PER_PIECE)) { model->slot_cunt <<= SENTENCEPIECE_DOUBLE_SHIFT; }

    ret = sentencepiece_alloc(spec->blob_size + (uint64_t)SENTENCEPIECE_ADJACENT_OFF, &ptr);
    model->blob = (uint8_t *)ptr;

    if (ret == LDG_ERR_AOK) { ret = sentencepiece_alloc((piece_cunt + (uint64_t)SENTENCEPIECE_ADJACENT_OFF) * (uint64_t)sizeof(uint32_t), &ptr); }

    model->piece_off = (ret == LDG_ERR_AOK) ? (uint32_t *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = sentencepiece_alloc(piece_cunt * (uint64_t)sizeof(double), &ptr); }

    model->score = (ret == LDG_ERR_AOK) ? (double *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = sentencepiece_alloc(piece_cunt * (uint64_t)sizeof(uint8_t), &ptr); }

    model->kind = (ret == LDG_ERR_AOK) ? (uint8_t *)ptr : 0x0;

    if (ret == LDG_ERR_AOK) { ret = sentencepiece_alloc((uint64_t)model->slot_cunt * (uint64_t)sizeof(uint32_t), &ptr); }

    model->slot = (ret == LDG_ERR_AOK) ? (uint32_t *)ptr : 0x0;

    while (ret == LDG_ERR_AOK && idx < model->slot_cunt)
    {
        model->slot[idx] = SENTENCEPIECE_NONE;
        idx++;
    }

    return ret;
}

static uint32_t sentencepiece_char_len_get(const uint8_t *text, uint32_t len, uint32_t pos)
{
    uint32_t size = 0;
    uint32_t idx = SENTENCEPIECE_ADJACENT_OFF;
    uint8_t lead = text[pos];

    if (lead < SENTENCEPIECE_ASCII_END) { return SENTENCEPIECE_PLAIN_SIZE; }

    if (lead < SENTENCEPIECE_PAIR_LEAD_MIN || lead >= SENTENCEPIECE_LEAD_END) { return SENTENCEPIECE_SPAN_NONE; }

    size = (lead < SENTENCEPIECE_TRIPLE_LEAD_MIN) ? SENTENCEPIECE_PAIR_SIZE : ((lead < SENTENCEPIECE_QUAD_LEAD_MIN) ? SENTENCEPIECE_TRIPLE_SIZE : SENTENCEPIECE_QUAD_SIZE);

    if (size > (len - pos)) { return SENTENCEPIECE_SPAN_NONE; }

    while (idx < size)
    {
        if ((text[pos + idx] & SENTENCEPIECE_TAIL_MASK) != SENTENCEPIECE_TAIL_MARK) { return SENTENCEPIECE_SPAN_NONE; }

        idx++;
    }

    return size;
}

static uint32_t sentencepiece_user_len_get(const ldg_math_stat_sentencepiece_t *model, const uint8_t *text, uint32_t len, uint32_t pos)
{
    uint32_t span = (uint32_t)model->user_first[text[pos]];
    uint32_t piece = 0;

    if (span > (len - pos)) { span = len - pos; }

    while (span > 0)
    {
        piece = sentencepiece_span_id_get(model, text, pos, pos + span);

        if (piece != SENTENCEPIECE_NONE && model->kind[piece] == SENTENCEPIECE_KIND_USER) { return span; }

        span--;
    }

    return SENTENCEPIECE_SPAN_NONE;
}

static uint32_t sentencepiece_split(const ldg_math_stat_sentencepiece_t *model, const uint8_t *text, uint32_t len, uint32_t *end, uint32_t *prev)
{
    uint32_t held = SENTENCEPIECE_NONE;
    uint32_t span = 0;
    uint32_t pos = 0;
    uint32_t frozen = 0;

    while (pos < len)
    {
        span = sentencepiece_user_len_get(model, text, len, pos);
        frozen = (span != 0) ? SENTENCEPIECE_FROZEN_BIT : SENTENCEPIECE_FROZEN_NONE;

        if (span == 0) { span = sentencepiece_char_len_get(text, len, pos); }

        if (LDG_UNLIKELY(span == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

        end[pos] = (pos + span) | frozen;
        prev[pos] = held;
        held = pos;
        pos += span;
    }

    return LDG_ERR_AOK;
}

static void sentencepiece_pair_fill(const ldg_math_stat_sentencepiece_t *model, const uint8_t *text, uint32_t len, const uint32_t *end, uint32_t *pair, uint32_t pos)
{
    uint32_t mid = end[pos] & SENTENCEPIECE_END_MASK;

    pair[pos] = SENTENCEPIECE_NONE;

    if (mid >= len) { return; }

    if (((end[pos] | end[mid]) & SENTENCEPIECE_FROZEN_BIT) != 0) { return; }

    pair[pos] = sentencepiece_span_id_get(model, text, pos, end[mid] & SENTENCEPIECE_END_MASK);
}

static uint32_t sentencepiece_best_get(const ldg_math_stat_sentencepiece_t *model, uint32_t len, const uint32_t *end, const uint32_t *pair)
{
    double best_score = 0.0;
    uint32_t best = SENTENCEPIECE_NONE;
    uint32_t pos = 0;

    while (pos < len)
    {
        if (pair[pos] != SENTENCEPIECE_NONE && (best == SENTENCEPIECE_NONE || model->score[pair[pos]] > best_score))
        {
            best = pos;
            best_score = model->score[pair[pos]];
        }

        pos = end[pos] & SENTENCEPIECE_END_MASK;
    }

    return best;
}

static void sentencepiece_merge_run(const ldg_math_stat_sentencepiece_t *model, const uint8_t *text, uint32_t len, uint32_t *end, uint32_t *prev, uint32_t *pair)
{
    uint32_t best = 0;
    uint32_t mid = 0;
    uint32_t pos = 0;

    while (pos < len)
    {
        sentencepiece_pair_fill(model, text, len, end, pair, pos);
        pos = end[pos] & SENTENCEPIECE_END_MASK;
    }

    best = sentencepiece_best_get(model, len, end, pair);

    while (best != SENTENCEPIECE_NONE)
    {
        mid = end[best] & SENTENCEPIECE_END_MASK;
        end[best] = end[mid] & SENTENCEPIECE_END_MASK;

        if (end[best] < len) { prev[end[best]] = best; }

        sentencepiece_pair_fill(model, text, len, end, pair, best);

        if (prev[best] != SENTENCEPIECE_NONE) { sentencepiece_pair_fill(model, text, len, end, pair, prev[best]); }

        best = sentencepiece_best_get(model, len, end, pair);
    }
}

static uint32_t sentencepiece_tok_put(uint32_t *dst, uint32_t cap, uint32_t cunt, uint32_t tok)
{
    if (cunt < cap) { dst[cunt] = tok; }

    return cunt + SENTENCEPIECE_ADJACENT_OFF;
}

static uint32_t sentencepiece_fallback_put(const ldg_math_stat_sentencepiece_t *model, const uint8_t *text, uint32_t start, uint32_t stop, uint32_t *dst, uint32_t cap, uint32_t cunt)
{
    uint32_t idx = start;

    if (!model->byte_fallback_is) { return sentencepiece_tok_put(dst, cap, cunt, model->unk_id); }

    while (idx < stop)
    {
        if (text[idx] == SENTENCEPIECE_SPACE)
        {
            cunt = sentencepiece_tok_put(dst, cap, cunt, model->byte_base + SENTENCEPIECE_MARKER_LEAD);
            cunt = sentencepiece_tok_put(dst, cap, cunt, model->byte_base + SENTENCEPIECE_MARKER_MID);
            cunt = sentencepiece_tok_put(dst, cap, cunt, model->byte_base + SENTENCEPIECE_MARKER_TAIL);
        }
        else { cunt = sentencepiece_tok_put(dst, cap, cunt, model->byte_base + (uint32_t)text[idx]); }

        idx++;
    }

    return cunt;
}

static uint32_t sentencepiece_emit(const ldg_math_stat_sentencepiece_t *model, const uint8_t *text, uint32_t len, const uint32_t *end, uint32_t *dst, uint32_t cap)
{
    uint32_t piece = 0;
    uint32_t stop = 0;
    uint32_t cunt = 0;
    uint32_t pos = 0;

    while (pos < len)
    {
        stop = end[pos] & SENTENCEPIECE_END_MASK;
        piece = sentencepiece_span_id_get(model, text, pos, stop);

        if (piece != SENTENCEPIECE_NONE) { cunt = sentencepiece_tok_put(dst, cap, cunt, piece); }
        else { cunt = sentencepiece_fallback_put(model, text, pos, stop, dst, cap, cunt); }

        pos = stop;
    }

    return cunt;
}

uint32_t ldg_math_stat_sentencepiece_init(ldg_math_stat_sentencepiece_t *model, const uint8_t *data, uint64_t len)
{
    sentencepiece_spec_t spec = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !data)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_mem_zero(model, sizeof(*model));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    spec.model_kind = SENTENCEPIECE_MODEL_KIND_UNIGRAM;
    spec.dummy_prefix_is = LDG_TRUTH_TRUE;
    spec.extra_space_is = LDG_TRUTH_TRUE;
    spec.escape_space_is = LDG_TRUTH_TRUE;

    ret = sentencepiece_model_walk(data, len, model, LDG_TRUTH_FALSE, &spec);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sentencepiece_spec_ck(&spec);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    model->byte_base = SENTENCEPIECE_NONE;
    model->unk_id = SENTENCEPIECE_NONE;

    ret = sentencepiece_table_alloc(model, &spec);
    if (ret == LDG_ERR_AOK) { ret = sentencepiece_model_walk(data, len, model, LDG_TRUTH_TRUE, &spec); }

    if (ret == LDG_ERR_AOK && model->unk_id == SENTENCEPIECE_NONE) { ret = LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        sentencepiece_buff_free(model);
        return ret;
    }

    model->byte_fallback_is = (model->byte_cunt == LDG_MATH_STAT_SENTENCEPIECE_BYTE_CUNT) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
    model->is_init = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_sentencepiece_shutdown(ldg_math_stat_sentencepiece_t *model)
{
    uint32_t ret = sentencepiece_init_ck(model);

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sentencepiece_buff_free(model);

    return ldg_mem_zero(model, sizeof(*model));
}

uint32_t ldg_math_stat_sentencepiece_buff_cunt_get(uint32_t len, uintptr_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (LDG_UNLIKELY(len > SENTENCEPIECE_TEXT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    *out = (uintptr_t)len * (uintptr_t)SENTENCEPIECE_BUFF_PER_BYTE;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_sentencepiece_piece_cunt_get(const ldg_math_stat_sentencepiece_t *model, uint32_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = sentencepiece_init_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out = model->piece_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_math_stat_sentencepiece_piece_id_get(const ldg_math_stat_sentencepiece_t *model, const uint8_t *piece, uint32_t len, uint32_t *out)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !piece || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = SENTENCEPIECE_NONE;

    ret = sentencepiece_init_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (idx < model->piece_cunt)
    {
        if (sentencepiece_raw_match_is(model, idx, piece, len) == LDG_TRUTH_TRUE)
        {
            *out = idx;
            return LDG_ERR_AOK;
        }

        idx++;
    }

    return LDG_ERR_NOT_FOUND;
}

uint32_t ldg_math_stat_sentencepiece_encode(const ldg_math_stat_sentencepiece_t *model, const uint8_t *text, uint32_t len, uint32_t *buff, uintptr_t buff_cunt, uint32_t *dst, uint32_t cap, uint32_t *out_cunt)
{
    uint32_t *end = buff;
    uint32_t *prev = 0x0;
    uint32_t *pair = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!model || !text || !buff || !dst || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    ret = sentencepiece_init_ck(model);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(len > SENTENCEPIECE_TEXT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(buff_cunt < ((uintptr_t)len * (uintptr_t)SENTENCEPIECE_BUFF_PER_BYTE))) { return LDG_ERR_OVERFLOW; }

    if (len == 0) { return LDG_ERR_AOK; }

    prev = buff + len;
    pair = prev + len;

    ret = sentencepiece_split(model, text, len, end, prev);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    sentencepiece_merge_run(model, text, len, end, prev, pair);
    *out_cunt = sentencepiece_emit(model, text, len, end, dst, cap);

    if (LDG_UNLIKELY(*out_cunt > cap)) { return LDG_ERR_OVERFLOW; }

    return LDG_ERR_AOK;
}
