#include <stdint.h>

#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/math/crypto/ecdsa.h>
#include <dangling/math/crypto/rsa.h>
#include <dangling/math/crypto/sha2.h>
#include <dangling/math/crypto/x509.h>
#include <dangling/str/str.h>

#define X509_TAG_SEQUENCE 0x30
#define X509_TAG_INTEGER 0x02
#define X509_TAG_BIT_STRING 0x03
#define X509_TAG_OID 0x06
#define X509_TAG_VERSION 0xA0
#define X509_TAG_NUMBER_MASK 0x1F
#define X509_LEN_FORM_BIT 0x80
#define X509_LEN_FORM_NARROW 0x81
#define X509_LEN_FORM_WIDE 0x82
#define X509_LEN_NARROW_MIN 0x80
#define X509_LEN_WIDE_MIN 0x100
#define X509_LEN_EXT_OFF 2
#define X509_HDR_SIZE 2
#define X509_HDR_NARROW_SIZE 3
#define X509_HDR_WIDE_SIZE 4
#define X509_BYTE_BITS 8
#define X509_ADJACENT_OFF 1
#define X509_SIGN_BIT 0x80
#define X509_OID_SIZE_MAX 16
#define X509_HEX_PER_BYTE 2
#define X509_OID_TEXT_CAP (X509_OID_SIZE_MAX * X509_HEX_PER_BYTE + LDG_STR_TERM_SIZE)
#define X509_TAG_BOOLEAN 0x01
#define X509_TAG_OCTET_STRING 0x04
#define X509_TAG_UTC_TIME 0x17
#define X509_TAG_GENERAL_TIME 0x18
#define X509_TAG_EXT_LIST 0xA3
#define X509_TAG_DNS_NAME 0x82
#define X509_BOOLEAN_SIZE 1
#define X509_TIME_UTC_SIZE 13
#define X509_TIME_GENERAL_SIZE 15
#define X509_TIME_UTC_PAIR_CUNT 6
#define X509_TIME_GENERAL_PAIR_CUNT 7
#define X509_TIME_PAIR_MAX 7
#define X509_TIME_ZULU 0x5A
#define X509_PAIR_SIZE 2
#define X509_DIGIT_BASE 0x30
#define X509_DIGIT_CUNT 10
#define X509_UTC_PIVOT 50
#define X509_CENTURY_OLD 1900
#define X509_CENTURY_NEW 2000
#define X509_CENTURY_YEAR_CUNT 100
#define X509_YEAR_MIN 1970
#define X509_MONTH_MAX 12
#define X509_DAY_MAX 31
#define X509_HOUR_CUNT 24
#define X509_SIXTIETH_CUNT 60
#define X509_ERA_YEAR_CUNT 400
#define X509_ERA_DAY_CUNT 146097
#define X509_YEAR_DAY_CUNT 365
#define X509_LEAP_YEAR_STEP 4
#define X509_EPOCH_DAY_OFF 719468
#define X509_MONTH_SPLIT 2
#define X509_MONTH_BACK 3
#define X509_MONTH_FORWARD 9
#define X509_MONTH_DAY_SCALE 153
#define X509_MONTH_DAY_ROUND 2
#define X509_MONTH_DAY_DIV 5
#define X509_DAY_HOUR_CUNT 24
#define X509_CHAR_STAR 0x2A
#define X509_CHAR_DOT 0x2E
#define X509_UPPER_FIRST 0x41
#define X509_UPPER_LAST 0x5A
#define X509_CASE_BIT 0x20
#define X509_WILD_PREFIX_SIZE 2
#define X509_PATH_LEN_SIZE_MAX 4
#define X509_USAGE_SIZE_MIN 2

typedef enum x509_family
{
    X509_FAMILY_RSA,
    X509_FAMILY_EC
} x509_family_t;

typedef enum x509_ext
{
    X509_EXT_BASIC,
    X509_EXT_ALT_NAME,
    X509_EXT_USAGE,
    X509_EXT_PURPOSE
} x509_ext_t;

typedef enum x509_part
{
    X509_PART_MONTH,
    X509_PART_DAY,
    X509_PART_HOUR,
    X509_PART_MINUTE,
    X509_PART_SEC,
    X509_PART_CUNT
} x509_part_t;

typedef struct x509_cursor
{
    const uint8_t *src;
    uint32_t len;
    uint32_t pos;
} x509_cursor_t;

typedef struct x509_elem
{
    const uint8_t *full;
    const uint8_t *body;
    uint32_t full_len;
    uint32_t body_len;
    uint8_t tag;
    uint8_t pudding[7];
} x509_elem_t;

static void x509_cursor_set(x509_cursor_t *cursor, const uint8_t *src, uint32_t len)
{
    cursor->src = src;
    cursor->len = len;
    cursor->pos = 0;
}

static uint32_t x509_len_rd(const uint8_t *at, uint32_t avail, uint32_t *out_hdr, uint32_t *out_body_len)
{
    uint32_t form = at[X509_ADJACENT_OFF];

    *out_hdr = X509_HDR_SIZE;
    *out_body_len = form;

    if (form < X509_LEN_FORM_BIT) { return LDG_ERR_AOK; }

    if (form == X509_LEN_FORM_NARROW && avail >= X509_HDR_NARROW_SIZE)
    {
        *out_hdr = X509_HDR_NARROW_SIZE;
        *out_body_len = at[X509_LEN_EXT_OFF];

        return (*out_body_len >= X509_LEN_NARROW_MIN) ? LDG_ERR_AOK : LDG_ERR_INVALID;
    }

    if (form == X509_LEN_FORM_WIDE && avail >= X509_HDR_WIDE_SIZE)
    {
        *out_hdr = X509_HDR_WIDE_SIZE;
        *out_body_len = ((uint32_t)at[X509_LEN_EXT_OFF] << X509_BYTE_BITS) | (uint32_t)at[X509_LEN_EXT_OFF + X509_ADJACENT_OFF];

        return (*out_body_len >= X509_LEN_WIDE_MIN) ? LDG_ERR_AOK : LDG_ERR_INVALID;
    }

    return LDG_ERR_INVALID;
}

static uint32_t x509_elem_rd(x509_cursor_t *cursor, x509_elem_t *out)
{
    const uint8_t *at = cursor->src + cursor->pos;
    uint32_t avail = cursor->len - cursor->pos;
    uint32_t hdr = 0;
    uint32_t body_len = 0;
    uint32_t ret = 0;

    if (avail < X509_HDR_SIZE || (at[0] & X509_TAG_NUMBER_MASK) == X509_TAG_NUMBER_MASK) { return LDG_ERR_INVALID; }

    ret = x509_len_rd(at, avail, &hdr, &body_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (body_len > (avail - hdr)) { return LDG_ERR_INVALID; }

    out->tag = at[0];
    out->full = at;
    out->full_len = hdr + body_len;
    out->body = at + hdr;
    out->body_len = body_len;
    cursor->pos += hdr + body_len;

    return LDG_ERR_AOK;
}

static uint32_t x509_tagged_elem_rd(x509_cursor_t *cursor, uint8_t tag, x509_elem_t *out)
{
    uint32_t ret = 0;

    ret = x509_elem_rd(cursor, out);
    if (ret != LDG_ERR_AOK) { return ret; }

    return (out->tag == tag) ? LDG_ERR_AOK : LDG_ERR_INVALID;
}

static uint32_t x509_integer_rd(x509_cursor_t *cursor, const uint8_t **out, uint32_t *out_len)
{
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    ret = x509_tagged_elem_rd(cursor, X509_TAG_INTEGER, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (elem.body_len == 0 || elem.body[0] >= X509_SIGN_BIT) { return LDG_ERR_INVALID; }

    *out = elem.body;
    *out_len = elem.body_len;

    return LDG_ERR_AOK;
}

static uint32_t x509_bits_get(const x509_elem_t *elem, const uint8_t **out, uint32_t *out_len)
{
    if (elem->body_len == 0 || elem->body[0] != 0) { return LDG_ERR_INVALID; }

    *out = elem->body + X509_ADJACENT_OFF;
    *out_len = elem->body_len - X509_ADJACENT_OFF;

    return LDG_ERR_AOK;
}

static uint8_t x509_span_eq_is(const uint8_t *lhs, uint32_t lhs_len, const uint8_t *rhs, uint32_t rhs_len)
{
    uint32_t diff = 0;
    uint32_t idx = 0;

    if (lhs_len != rhs_len) { return LDG_TRUTH_FALSE; }

    while (idx < lhs_len)
    {
        diff |= (uint32_t)(lhs[idx] ^ rhs[idx]);
        idx++;
    }

    return (diff == 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t x509_oid_match_is(const x509_elem_t *oid, const char *text)
{
    uint8_t raw[X509_OID_SIZE_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t size = 0;

    while (text[size * X509_HEX_PER_BYTE] != LDG_STR_TERM) { size++; }

    if (size != oid->body_len || size > X509_OID_SIZE_MAX) { return LDG_TRUTH_FALSE; }

    if (LDG_UNLIKELY(ldg_hex_to_bytes(text, raw, size) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

    return x509_span_eq_is(oid->body, oid->body_len, raw, size);
}

static uint32_t x509_oid_find(x509_cursor_t *cursor, const char (*text)[X509_OID_TEXT_CAP], uint32_t text_cunt, uint32_t *out_idx)
{
    x509_elem_t oid = LDG_STRUCT_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t ret = 0;

    ret = x509_tagged_elem_rd(cursor, X509_TAG_OID, &oid);
    if (ret != LDG_ERR_AOK) { return ret; }

    while (idx < text_cunt)
    {
        if (x509_oid_match_is(&oid, text[idx]) == LDG_TRUTH_TRUE)
        {
            *out_idx = idx;
            return LDG_ERR_AOK;
        }

        idx++;
    }

    return LDG_ERR_UNSUPPORTED;
}

static uint32_t x509_sig_alg_get(const x509_elem_t *alg, ldg_math_crypto_x509_sig_t *out)
{
    const char text[][X509_OID_TEXT_CAP] = { "2a864886f70d01010b", "2a864886f70d01010c", "2a864886f70d01010d", "2a8648ce3d040302", "2a8648ce3d040303", "2a8648ce3d040304" };
    x509_cursor_t cursor = LDG_STRUCT_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t ret = 0;

    x509_cursor_set(&cursor, alg->body, alg->body_len);

    ret = x509_oid_find(&cursor, text, (uint32_t)(sizeof(text) / sizeof(text[0])), &idx);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out = (ldg_math_crypto_x509_sig_t)idx;

    return LDG_ERR_AOK;
}

static uint32_t x509_rsa_key_parse(const uint8_t *bits, uint32_t bits_len, ldg_math_crypto_x509_cert_t *out)
{
    x509_cursor_t cursor = LDG_STRUCT_ZERO_INIT;
    x509_elem_t pair = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    x509_cursor_set(&cursor, bits, bits_len);

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_SEQUENCE, &pair);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cursor.pos != cursor.len) { return LDG_ERR_INVALID; }

    x509_cursor_set(&cursor, pair.body, pair.body_len);

    ret = x509_integer_rd(&cursor, &out->key, &out->key_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = x509_integer_rd(&cursor, &out->exponent, &out->exponent_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cursor.pos != cursor.len) { return LDG_ERR_INVALID; }

    out->key_alg = LDG_MATH_CRYPTO_X509_KEY_RSA;

    return LDG_ERR_AOK;
}

static uint32_t x509_curve_key_parse(x509_cursor_t *alg, const uint8_t *bits, uint32_t bits_len, ldg_math_crypto_x509_cert_t *out)
{
    const char text[][X509_OID_TEXT_CAP] = { "2a8648ce3d030107", "2b81040022", "2b8104000a" };
    uint32_t idx = 0;
    uint32_t ret = 0;

    ret = x509_oid_find(alg, text, (uint32_t)(sizeof(text) / sizeof(text[0])), &idx);
    if (ret != LDG_ERR_AOK) { return ret; }

    out->key_alg = (ldg_math_crypto_x509_key_t)((uint32_t)LDG_MATH_CRYPTO_X509_KEY_P256 + idx);
    out->key = bits;
    out->key_len = bits_len;

    return LDG_ERR_AOK;
}

static uint32_t x509_key_parse(const x509_elem_t *key_info, ldg_math_crypto_x509_cert_t *out)
{
    const char text[][X509_OID_TEXT_CAP] = { "2a864886f70d010101", "2a8648ce3d0201" };
    x509_cursor_t cursor = LDG_STRUCT_ZERO_INIT;
    x509_elem_t alg = LDG_STRUCT_ZERO_INIT;
    x509_elem_t packed = LDG_STRUCT_ZERO_INIT;
    const uint8_t *bits = 0x0;
    uint32_t bits_len = 0;
    uint32_t family = 0;
    uint32_t ret = 0;

    x509_cursor_set(&cursor, key_info->body, key_info->body_len);

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_SEQUENCE, &alg);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_BIT_STRING, &packed);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = x509_bits_get(&packed, &bits, &bits_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    x509_cursor_set(&cursor, alg.body, alg.body_len);

    ret = x509_oid_find(&cursor, text, (uint32_t)(sizeof(text) / sizeof(text[0])), &family);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (family == X509_FAMILY_RSA) { return x509_rsa_key_parse(bits, bits_len, out); }

    return x509_curve_key_parse(&cursor, bits, bits_len, out);
}

static uint32_t x509_pair_get(const uint8_t *text, uint32_t *out)
{
    uint32_t high = (uint32_t)text[0] - X509_DIGIT_BASE;
    uint32_t low = (uint32_t)text[X509_ADJACENT_OFF] - X509_DIGIT_BASE;

    if (high >= X509_DIGIT_CUNT || low >= X509_DIGIT_CUNT) { return LDG_ERR_INVALID; }

    *out = (high * X509_DIGIT_CUNT) + low;

    return LDG_ERR_AOK;
}

static uint32_t x509_epoch_get(uint32_t year, const uint32_t *part, uint64_t *out)
{
    uint64_t month = part[X509_PART_MONTH];
    uint64_t day = part[X509_PART_DAY];
    uint64_t shifted = year;
    uint64_t era = 0;
    uint64_t era_year = 0;
    uint64_t year_day = 0;
    uint64_t era_day = 0;

    if (year < X509_YEAR_MIN || month == 0 || month > X509_MONTH_MAX || day == 0 || day > X509_DAY_MAX) { return LDG_ERR_INVALID; }

    if (part[X509_PART_HOUR] >= X509_HOUR_CUNT || part[X509_PART_MINUTE] >= X509_SIXTIETH_CUNT || part[X509_PART_SEC] >= X509_SIXTIETH_CUNT) { return LDG_ERR_INVALID; }

    if (month <= X509_MONTH_SPLIT) { shifted--; }

    month = (month > X509_MONTH_SPLIT) ? (month - X509_MONTH_BACK) : (month + X509_MONTH_FORWARD);
    era = shifted / X509_ERA_YEAR_CUNT;
    era_year = shifted - (era * X509_ERA_YEAR_CUNT);
    year_day = (((X509_MONTH_DAY_SCALE * month) + X509_MONTH_DAY_ROUND) / X509_MONTH_DAY_DIV) + day - X509_ADJACENT_OFF;
    era_day = (era_year * X509_YEAR_DAY_CUNT) + (era_year / X509_LEAP_YEAR_STEP) - (era_year / X509_CENTURY_YEAR_CUNT) + year_day;

    *out = ((((era * X509_ERA_DAY_CUNT) + era_day) - X509_EPOCH_DAY_OFF) * X509_DAY_HOUR_CUNT * LDG_SECS_PER_HOUR) + ((uint64_t)part[X509_PART_HOUR] * LDG_SECS_PER_HOUR) + ((uint64_t)part[X509_PART_MINUTE] * LDG_SECS_PER_MIN) + part[X509_PART_SEC];

    return LDG_ERR_AOK;
}

static uint32_t x509_time_rd(x509_cursor_t *cursor, uint64_t *out)
{
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;
    uint32_t field[X509_TIME_PAIR_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t pair_cunt = 0;
    uint32_t year = 0;
    uint32_t idx = 0;
    uint32_t ret = 0;

    ret = x509_elem_rd(cursor, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (elem.tag == X509_TAG_UTC_TIME && elem.body_len == X509_TIME_UTC_SIZE) { pair_cunt = X509_TIME_UTC_PAIR_CUNT; }

    if (elem.tag == X509_TAG_GENERAL_TIME && elem.body_len == X509_TIME_GENERAL_SIZE) { pair_cunt = X509_TIME_GENERAL_PAIR_CUNT; }

    if (pair_cunt == 0 || elem.body[elem.body_len - X509_ADJACENT_OFF] != X509_TIME_ZULU) { return LDG_ERR_INVALID; }

    while (idx < pair_cunt)
    {
        ret = x509_pair_get(elem.body + (X509_PAIR_SIZE * idx), &field[idx]);
        if (ret != LDG_ERR_AOK) { return ret; }

        idx++;
    }

    if (pair_cunt == X509_TIME_GENERAL_PAIR_CUNT) { year = (field[0] * X509_CENTURY_YEAR_CUNT) + field[X509_ADJACENT_OFF]; }
    else { year = field[0] + ((field[0] >= X509_UTC_PIVOT) ? X509_CENTURY_OLD : X509_CENTURY_NEW); }

    return x509_epoch_get(year, field + (pair_cunt - X509_PART_CUNT), out);
}

static uint32_t x509_validity_parse(const x509_elem_t *validity, ldg_math_crypto_x509_cert_t *out)
{
    x509_cursor_t cursor = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    x509_cursor_set(&cursor, validity->body, validity->body_len);

    ret = x509_time_rd(&cursor, &out->not_before);
    if (ret != LDG_ERR_AOK) { return ret; }

    return x509_time_rd(&cursor, &out->not_after);
}

static uint32_t x509_basic_parse(const x509_elem_t *val, ldg_math_crypto_x509_cert_t *out)
{
    x509_cursor_t cursor = LDG_STRUCT_ZERO_INIT;
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;
    uint32_t idx = 0;
    uint32_t ret = 0;

    x509_cursor_set(&cursor, val->body, val->body_len);

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_SEQUENCE, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (elem.body_len == 0) { return LDG_ERR_AOK; }

    x509_cursor_set(&cursor, elem.body, elem.body_len);

    ret = x509_elem_rd(&cursor, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (elem.tag == X509_TAG_BOOLEAN)
    {
        if (elem.body_len == X509_BOOLEAN_SIZE && elem.body[0] != 0) { out->ca_is = LDG_TRUTH_TRUE; }

        if (cursor.pos >= cursor.len) { return LDG_ERR_AOK; }

        ret = x509_elem_rd(&cursor, &elem);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    if (elem.tag != X509_TAG_INTEGER || elem.body_len == 0 || elem.body_len > X509_PATH_LEN_SIZE_MAX || elem.body[0] >= X509_SIGN_BIT) { return LDG_ERR_INVALID; }

    if (cursor.pos != cursor.len) { return LDG_ERR_INVALID; }

    out->path_len_max = 0;

    for (idx = 0; idx < elem.body_len; idx++) { out->path_len_max = (out->path_len_max << X509_BYTE_BITS) | (uint32_t)elem.body[idx]; }

    return LDG_ERR_AOK;
}

static uint32_t x509_usage_parse(const x509_elem_t *val, ldg_math_crypto_x509_cert_t *out)
{
    x509_cursor_t cursor = LDG_STRUCT_ZERO_INIT;
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    x509_cursor_set(&cursor, val->body, val->body_len);

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_BIT_STRING, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (elem.body_len < X509_USAGE_SIZE_MIN || elem.body[0] >= X509_BYTE_BITS) { return LDG_ERR_INVALID; }

    out->key_usage = (uint16_t)elem.body[X509_ADJACENT_OFF];
    out->key_usage_has = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t x509_purpose_parse(const x509_elem_t *val, ldg_math_crypto_x509_cert_t *out)
{
    const char text[][X509_OID_TEXT_CAP] = { "2b06010505070301", "551d2500" };
    x509_cursor_t cursor = LDG_STRUCT_ZERO_INIT;
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;
    uint32_t kind = 0;
    uint32_t ret = 0;

    x509_cursor_set(&cursor, val->body, val->body_len);

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_SEQUENCE, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    x509_cursor_set(&cursor, elem.body, elem.body_len);
    out->ext_usage_has = LDG_TRUTH_TRUE;

    while (cursor.pos < cursor.len)
    {
        ret = x509_oid_find(&cursor, text, (uint32_t)(sizeof(text) / sizeof(text[0])), &kind);
        if (ret == LDG_ERR_AOK) { out->ext_usage_server_is = LDG_TRUTH_TRUE; }
        else if (ret != LDG_ERR_UNSUPPORTED) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t x509_alt_name_parse(const x509_elem_t *val, ldg_math_crypto_x509_cert_t *out)
{
    x509_cursor_t cursor = LDG_STRUCT_ZERO_INIT;
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    x509_cursor_set(&cursor, val->body, val->body_len);

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_SEQUENCE, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    out->alt_name = elem.body;
    out->alt_name_len = elem.body_len;

    return LDG_ERR_AOK;
}

static uint32_t x509_ext_parse(const x509_elem_t *ext, ldg_math_crypto_x509_cert_t *out)
{
    const char text[][X509_OID_TEXT_CAP] = { "551d13", "551d11", "551d0f", "551d25" };
    x509_cursor_t cursor = LDG_STRUCT_ZERO_INIT;
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;
    uint32_t kind = 0;
    uint32_t ret = 0;
    uint8_t known_is = 0;
    uint8_t crit_is = 0;

    x509_cursor_set(&cursor, ext->body, ext->body_len);

    ret = x509_oid_find(&cursor, text, (uint32_t)(sizeof(text) / sizeof(text[0])), &kind);
    if (ret != LDG_ERR_AOK && ret != LDG_ERR_UNSUPPORTED) { return ret; }

    known_is = (ret == LDG_ERR_AOK) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    ret = x509_elem_rd(&cursor, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (elem.tag == X509_TAG_BOOLEAN)
    {
        crit_is = (elem.body_len == X509_BOOLEAN_SIZE && elem.body[0] != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        ret = x509_elem_rd(&cursor, &elem);
    }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (elem.tag != X509_TAG_OCTET_STRING) { return LDG_ERR_INVALID; }

    if (!known_is) { return crit_is ? LDG_ERR_UNSUPPORTED : LDG_ERR_AOK; }

    if (kind == X509_EXT_BASIC) { return x509_basic_parse(&elem, out); }

    if (kind == X509_EXT_ALT_NAME) { return x509_alt_name_parse(&elem, out); }

    if (kind == X509_EXT_USAGE) { return x509_usage_parse(&elem, out); }

    if (kind == X509_EXT_PURPOSE) { return x509_purpose_parse(&elem, out); }

    return LDG_ERR_AOK;
}

static uint32_t x509_ext_seq_parse(const x509_elem_t *wrap, ldg_math_crypto_x509_cert_t *out)
{
    x509_cursor_t cursor = LDG_STRUCT_ZERO_INIT;
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    x509_cursor_set(&cursor, wrap->body, wrap->body_len);

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_SEQUENCE, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    x509_cursor_set(&cursor, elem.body, elem.body_len);

    while (cursor.pos < cursor.len)
    {
        ret = x509_tagged_elem_rd(&cursor, X509_TAG_SEQUENCE, &elem);
        if (ret != LDG_ERR_AOK) { return ret; }

        ret = x509_ext_parse(&elem, out);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint32_t x509_tail_parse(x509_cursor_t *cursor, ldg_math_crypto_x509_cert_t *out)
{
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    while (cursor->pos < cursor->len)
    {
        ret = x509_elem_rd(cursor, &elem);
        if (ret != LDG_ERR_AOK) { return ret; }

        if (elem.tag == X509_TAG_EXT_LIST) { ret = x509_ext_seq_parse(&elem, out); }

        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

static uint8_t x509_char_fold(uint8_t ch)
{
    return (ch >= X509_UPPER_FIRST && ch <= X509_UPPER_LAST) ? (uint8_t)(ch | X509_CASE_BIT) : ch;
}

static uint8_t x509_text_eq_is(const uint8_t *lhs, uint32_t lhs_len, const uint8_t *rhs, uint32_t rhs_len)
{
    uint32_t idx = 0;

    if (lhs_len != rhs_len) { return LDG_TRUTH_FALSE; }

    while (idx < lhs_len)
    {
        if (x509_char_fold(lhs[idx]) != x509_char_fold(rhs[idx])) { return LDG_TRUTH_FALSE; }

        idx++;
    }

    return LDG_TRUTH_TRUE;
}

static uint8_t x509_host_match_is(const uint8_t *pattern, uint32_t pattern_len, const uint8_t *host, uint32_t host_len)
{
    uint32_t skip = 0;

    if (pattern_len < X509_WILD_PREFIX_SIZE || pattern[0] != X509_CHAR_STAR || pattern[X509_ADJACENT_OFF] != X509_CHAR_DOT) { return x509_text_eq_is(pattern, pattern_len, host, host_len); }

    while (skip < host_len && host[skip] != X509_CHAR_DOT) { skip++; }

    if (skip == 0 || skip == host_len) { return LDG_TRUTH_FALSE; }

    return x509_text_eq_is(pattern + X509_ADJACENT_OFF, pattern_len - X509_ADJACENT_OFF, host + skip, host_len - skip);
}

static uint32_t x509_name_rd(x509_cursor_t *cursor, const uint8_t **out, uint32_t *out_len)
{
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    ret = x509_tagged_elem_rd(cursor, X509_TAG_SEQUENCE, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out = elem.full;
    *out_len = elem.full_len;

    return LDG_ERR_AOK;
}

static uint32_t x509_tbs_hdr_rd(x509_cursor_t *cursor, const x509_elem_t *sig_alg)
{
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    ret = x509_elem_rd(cursor, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (elem.tag == X509_TAG_VERSION) { ret = x509_elem_rd(cursor, &elem); }

    if (ret != LDG_ERR_AOK) { return ret; }

    if (elem.tag != X509_TAG_INTEGER) { return LDG_ERR_INVALID; }

    ret = x509_tagged_elem_rd(cursor, X509_TAG_SEQUENCE, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (x509_span_eq_is(elem.full, elem.full_len, sig_alg->full, sig_alg->full_len) != LDG_TRUTH_TRUE) { return LDG_ERR_INVALID; }

    return LDG_ERR_AOK;
}

static uint32_t x509_tbs_parse(const x509_elem_t *tbs, const x509_elem_t *sig_alg, ldg_math_crypto_x509_cert_t *out)
{
    x509_cursor_t cursor = LDG_STRUCT_ZERO_INIT;
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    x509_cursor_set(&cursor, tbs->body, tbs->body_len);

    ret = x509_tbs_hdr_rd(&cursor, sig_alg);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = x509_name_rd(&cursor, &out->issuer, &out->issuer_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_SEQUENCE, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = x509_validity_parse(&elem, out);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = x509_name_rd(&cursor, &out->subject, &out->subject_len);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_SEQUENCE, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = x509_key_parse(&elem, out);
    if (ret != LDG_ERR_AOK) { return ret; }

    return x509_tail_parse(&cursor, out);
}

static void x509_cert_clear(ldg_math_crypto_x509_cert_t *cert)
{
    cert->tbs = 0x0;
    cert->sig = 0x0;
    cert->issuer = 0x0;
    cert->subject = 0x0;
    cert->key = 0x0;
    cert->exponent = 0x0;
    cert->alt_name = 0x0;
    cert->not_before = 0;
    cert->not_after = 0;
    cert->alt_name_len = 0;
    cert->ca_is = LDG_TRUTH_FALSE;
    cert->path_len_max = LDG_MATH_CRYPTO_X509_PATH_LEN_NONE;
    cert->key_usage = 0;
    cert->key_usage_has = LDG_TRUTH_FALSE;
    cert->ext_usage_has = LDG_TRUTH_FALSE;
    cert->ext_usage_server_is = LDG_TRUTH_FALSE;
    cert->tbs_len = 0;
    cert->sig_len = 0;
    cert->issuer_len = 0;
    cert->subject_len = 0;
    cert->key_len = 0;
    cert->exponent_len = 0;
    cert->sig_alg = LDG_MATH_CRYPTO_X509_SIG_CUNT;
    cert->key_alg = LDG_MATH_CRYPTO_X509_KEY_CUNT;
}

static uint32_t x509_outer_parse(const uint8_t *der, uint32_t len, x509_elem_t *out_tbs, x509_elem_t *out_sig_alg, ldg_math_crypto_x509_cert_t *out)
{
    x509_cursor_t cursor = LDG_STRUCT_ZERO_INIT;
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    x509_cursor_set(&cursor, der, len);

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_SEQUENCE, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cursor.pos != cursor.len) { return LDG_ERR_INVALID; }

    x509_cursor_set(&cursor, elem.body, elem.body_len);

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_SEQUENCE, out_tbs);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_SEQUENCE, out_sig_alg);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = x509_tagged_elem_rd(&cursor, X509_TAG_BIT_STRING, &elem);
    if (ret != LDG_ERR_AOK) { return ret; }

    if (cursor.pos != cursor.len) { return LDG_ERR_INVALID; }

    return x509_bits_get(&elem, &out->sig, &out->sig_len);
}

uint32_t ldg_math_crypto_x509_cert_parse(const uint8_t *der, uint32_t len, ldg_math_crypto_x509_cert_t *out)
{
    x509_elem_t tbs = LDG_STRUCT_ZERO_INIT;
    x509_elem_t sig_alg = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!der || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    x509_cert_clear(out);

    ret = x509_outer_parse(der, len, &tbs, &sig_alg, out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->tbs = tbs.full;
    out->tbs_len = tbs.full_len;

    ret = x509_sig_alg_get(&sig_alg, &out->sig_alg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return x509_tbs_parse(&tbs, &sig_alg, out);
}

static uint32_t x509_digest_compute(const ldg_math_crypto_x509_cert_t *cert, uint8_t *digest, uint32_t *out_len, ldg_math_crypto_rsa_digest_t *out_kind)
{
    if (cert->sig_alg == LDG_MATH_CRYPTO_X509_SIG_RSA_SHA256 || cert->sig_alg == LDG_MATH_CRYPTO_X509_SIG_ECDSA_SHA256)
    {
        *out_len = LDG_MATH_CRYPTO_SHA256_OUT_SIZE;
        *out_kind = LDG_MATH_CRYPTO_RSA_DIGEST_SHA256;

        return ldg_math_crypto_sha256_compute(cert->tbs, cert->tbs_len, digest);
    }

    if (cert->sig_alg == LDG_MATH_CRYPTO_X509_SIG_RSA_SHA384 || cert->sig_alg == LDG_MATH_CRYPTO_X509_SIG_ECDSA_SHA384)
    {
        *out_len = LDG_MATH_CRYPTO_SHA384_OUT_SIZE;
        *out_kind = LDG_MATH_CRYPTO_RSA_DIGEST_SHA384;

        return ldg_math_crypto_sha384_compute(cert->tbs, cert->tbs_len, digest);
    }

    if (cert->sig_alg == LDG_MATH_CRYPTO_X509_SIG_RSA_SHA512 || cert->sig_alg == LDG_MATH_CRYPTO_X509_SIG_ECDSA_SHA512)
    {
        *out_len = LDG_MATH_CRYPTO_SHA512_OUT_SIZE;
        *out_kind = LDG_MATH_CRYPTO_RSA_DIGEST_SHA512;

        return ldg_math_crypto_sha512_compute(cert->tbs, cert->tbs_len, digest);
    }

    return LDG_ERR_FUNC_ARG_INVALID;
}

uint8_t ldg_math_crypto_x509_cert_signed_is(const ldg_math_crypto_x509_cert_t *cert, const ldg_math_crypto_x509_cert_t *issuer, uint32_t *buff, uint32_t buff_cunt)
{
    uint8_t digest[LDG_MATH_CRYPTO_SHA512_OUT_SIZE] = LDG_ARR_ZERO_INIT;
    ldg_math_crypto_rsa_digest_t kind = LDG_MATH_CRYPTO_RSA_DIGEST_SHA256;
    ldg_math_crypto_ecdsa_curve_t curve = LDG_MATH_CRYPTO_ECDSA_CURVE_P256;
    uint32_t digest_len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!cert || !issuer || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!cert->tbs || !cert->sig || !cert->issuer || !issuer->subject || !issuer->key)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(buff_cunt < LDG_MATH_CRYPTO_X509_BUFF_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (x509_span_eq_is(cert->issuer, cert->issuer_len, issuer->subject, issuer->subject_len) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    ret = x509_digest_compute(cert, digest, &digest_len, &kind);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return (uint8_t)ret; }

    if (cert->sig_alg <= LDG_MATH_CRYPTO_X509_SIG_RSA_SHA512)
    {
        if (issuer->key_alg != LDG_MATH_CRYPTO_X509_KEY_RSA || !issuer->exponent) { return LDG_TRUTH_FALSE; }

        return ldg_math_crypto_rsa_sig_valid_is(issuer->key, issuer->key_len, issuer->exponent, issuer->exponent_len, kind, digest, digest_len, cert->sig, cert->sig_len, buff, buff_cunt);
    }

    if (issuer->key_alg == LDG_MATH_CRYPTO_X509_KEY_RSA || issuer->key_alg >= LDG_MATH_CRYPTO_X509_KEY_CUNT) { return LDG_TRUTH_FALSE; }

    if (issuer->key_alg == LDG_MATH_CRYPTO_X509_KEY_P384) { curve = LDG_MATH_CRYPTO_ECDSA_CURVE_P384; }

    if (issuer->key_alg == LDG_MATH_CRYPTO_X509_KEY_SECP256K1) { curve = LDG_MATH_CRYPTO_ECDSA_CURVE_SECP256K1; }

    return ldg_math_crypto_ecdsa_der_valid_is(curve, issuer->key, issuer->key_len, digest, digest_len, cert->sig, cert->sig_len, buff, buff_cunt);
}

uint8_t ldg_math_crypto_x509_cert_name_has(const ldg_math_crypto_x509_cert_t *cert, const char *host, uint32_t host_len)
{
    x509_cursor_t cursor = LDG_STRUCT_ZERO_INIT;
    x509_elem_t elem = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!cert || !host)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(host_len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (!cert->alt_name) { return LDG_TRUTH_FALSE; }

    x509_cursor_set(&cursor, cert->alt_name, cert->alt_name_len);

    while (cursor.pos < cursor.len)
    {
        if (LDG_UNLIKELY(x509_elem_rd(&cursor, &elem) != LDG_ERR_AOK)) { return LDG_ERR_INVALID; }

        if (elem.tag == X509_TAG_DNS_NAME && x509_host_match_is(elem.body, elem.body_len, (const uint8_t *)host, host_len) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }
    }

    return LDG_TRUTH_FALSE;
}

uint8_t ldg_math_crypto_x509_cert_current_is(const ldg_math_crypto_x509_cert_t *cert, uint64_t now)
{
    if (LDG_UNLIKELY(!cert)) { return LDG_ERR_FUNC_ARG_NULL; }

    return (now >= cert->not_before && now <= cert->not_after) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint8_t x509_issuer_allowed_is(const ldg_math_crypto_x509_cert_t *ca, uint32_t below_cunt)
{
    if (ca->key_usage_has == LDG_TRUTH_TRUE && (ca->key_usage & LDG_MATH_CRYPTO_X509_USAGE_CERT_SIGN) == 0) { return LDG_TRUTH_FALSE; }

    if (ca->path_len_max != LDG_MATH_CRYPTO_X509_PATH_LEN_NONE && below_cunt > ca->path_len_max) { return LDG_TRUTH_FALSE; }

    if (ca->ext_usage_has == LDG_TRUTH_TRUE && ca->ext_usage_server_is != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint8_t x509_leaf_server_is(const ldg_math_crypto_x509_cert_t *leaf)
{
    if (leaf->ext_usage_has == LDG_TRUTH_TRUE && leaf->ext_usage_server_is != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (leaf->key_usage_has == LDG_TRUTH_TRUE && (leaf->key_usage & (LDG_MATH_CRYPTO_X509_USAGE_SIG | LDG_MATH_CRYPTO_X509_USAGE_KEY_ENC | LDG_MATH_CRYPTO_X509_USAGE_KEY_AGREE)) == 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint8_t x509_anchor_signed_is(const ldg_math_crypto_x509_cert_t *cert, uint32_t below_cunt, const ldg_math_crypto_x509_cert_t *anchor, uint32_t anchor_cunt, uint64_t now, uint32_t *buff, uint32_t buff_cunt)
{
    uint32_t idx = 0;
    uint8_t verdict = 0;

    while (idx < anchor_cunt)
    {
        verdict = LDG_TRUTH_FALSE;

        if (ldg_math_crypto_x509_cert_current_is(&anchor[idx], now) == LDG_TRUTH_TRUE && x509_issuer_allowed_is(&anchor[idx], below_cunt) == LDG_TRUTH_TRUE) { verdict = ldg_math_crypto_x509_cert_signed_is(cert, &anchor[idx], buff, buff_cunt); }

        if (verdict != LDG_TRUTH_FALSE) { return verdict; }

        idx++;
    }

    return LDG_TRUTH_FALSE;
}

uint8_t ldg_math_crypto_x509_chain_valid_is(const ldg_math_crypto_x509_cert_t *chain, uint32_t chain_cunt, const ldg_math_crypto_x509_cert_t *anchor, uint32_t anchor_cunt, const char *host, uint32_t host_len, uint64_t now, uint32_t *buff, uint32_t buff_cunt)
{
    uint32_t idx = 0;
    uint8_t verdict = 0;

    if (LDG_UNLIKELY(!chain || !anchor || !host || !buff)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(chain_cunt == 0 || anchor_cunt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    verdict = ldg_math_crypto_x509_cert_name_has(&chain[0], host, host_len);
    if (verdict != LDG_TRUTH_TRUE) { return verdict; }

    if (x509_leaf_server_is(&chain[0]) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    while (idx < chain_cunt)
    {
        if (ldg_math_crypto_x509_cert_current_is(&chain[idx], now) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

        if (idx > 0 && !chain[idx].ca_is) { return LDG_TRUTH_FALSE; }

        if (idx > 0 && x509_issuer_allowed_is(&chain[idx], idx - X509_ADJACENT_OFF) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

        verdict = x509_anchor_signed_is(&chain[idx], idx, anchor, anchor_cunt, now, buff, buff_cunt);
        if (verdict != LDG_TRUTH_FALSE) { return verdict; }

        if ((idx + X509_ADJACENT_OFF) == chain_cunt) { return LDG_TRUTH_FALSE; }

        verdict = ldg_math_crypto_x509_cert_signed_is(&chain[idx], &chain[idx + X509_ADJACENT_OFF], buff, buff_cunt);
        if (verdict != LDG_TRUTH_TRUE) { return verdict; }

        idx++;
    }

    return LDG_TRUTH_FALSE;
}
