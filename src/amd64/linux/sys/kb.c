#include <stdio.h>

#define LDG_SYS_KB_DEVS_PATH "/proc/bus/input/devices"
#define LDG_SYS_KB_RD_MODE "r"
#define LDG_SYS_KB_LN_MAX 1024
#define LDG_SYS_KB_PHYS_MAX 256
#define LDG_SYS_KB_SEEN_MAX 64
#define LDG_SYS_KB_ID_TAG "I:"
#define LDG_SYS_KB_NAME_TAG "N: Name=\""
#define LDG_SYS_KB_PHYS_TAG "P: Phys="
#define LDG_SYS_KB_EV_TAG "B: EV="
#define LDG_SYS_KB_KEY_TAG "B: KEY="
#define LDG_SYS_KB_VENDOR_TAG "Vendor="
#define LDG_SYS_KB_PRODUCT_TAG "Product="
#define LDG_SYS_KB_INPUT_TAG "input"
#define LDG_SYS_KB_ID_DIGIT_MAX 4
#define LDG_SYS_KB_ID_ABSENT 0
#define LDG_SYS_KB_MASK_DIGIT_MAX 16
#define LDG_SYS_KB_MASK_ABSENT 0
#define LDG_SYS_KB_VID_SHIFT 16
#define LDG_SYS_KB_EV_KEY_BIT 1
#define LDG_SYS_KB_TYPING_MASK 0xFFFFFFFEULL
#define LDG_SYS_KB_QUOTE '"'
#define LDG_SYS_KB_PHYS_SEP '/'
#define LDG_SYS_KB_WORD_SEP ' '
#define LDG_SYS_KB_STEP 1

#include <dangling/sys/info.h>
#include <dangling/mem/secure.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/bits.h>
#include <dangling/str/str.h>

typedef struct sys_kb_rec
{
    ldg_sys_kb_t kb;
    char phys[LDG_SYS_KB_PHYS_MAX];
    uint64_t ev;
    uint64_t key;
    uint8_t used;
    uint8_t pudding[7];
} sys_kb_rec_t;

typedef struct sys_kb_tally
{
    uint64_t seen[LDG_SYS_KB_SEEN_MAX];
    uint32_t seen_cunt;
    uint32_t total;
} sys_kb_tally_t;

static uint8_t sys_kb_prefix_is(const char *ln, const char *tag, uint64_t *out_off)
{
    uint64_t idx = 0;

    *out_off = 0;

    while (tag[idx] != LDG_STR_TERM)
    {
        if (ln[idx] != tag[idx]) { return LDG_TRUTH_FALSE; }

        idx++;
    }

    *out_off = idx;

    return LDG_TRUTH_TRUE;
}

static void sys_kb_text_cpy(char *dst, uint64_t cap, const char *src, char stop)
{
    uint64_t idx = 0;

    while (idx + LDG_STR_TERM_SIZE < cap && src[idx] != LDG_STR_TERM && src[idx] != stop)
    {
        dst[idx] = src[idx];
        idx++;
    }

    dst[idx] = LDG_STR_TERM;
}

static uint8_t sys_kb_input_suffix_is(const char *suffix)
{
    uint64_t off = 0;
    uint64_t idx = 0;

    if (sys_kb_prefix_is(suffix, LDG_SYS_KB_INPUT_TAG, &off) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; }

    if (suffix[off] == LDG_STR_TERM) { return LDG_TRUTH_FALSE; }

    for (idx = off; suffix[idx] != LDG_STR_TERM; idx++) { if (ldg_char_digit_is((uint8_t)suffix[idx]) != LDG_TRUTH_TRUE) { return LDG_TRUTH_FALSE; } }

    return LDG_TRUTH_TRUE;
}

static void sys_kb_phys_trim(char *phys)
{
    uint64_t sep = 0;

    if (ldg_str_char_last_find(phys, LDG_SYS_KB_PHYS_SEP, LDG_SYS_KB_PHYS_MAX, &sep) != LDG_ERR_AOK) { return; }

    if (sys_kb_input_suffix_is(&phys[sep + LDG_SYS_KB_STEP]) == LDG_TRUTH_TRUE) { phys[sep] = LDG_STR_TERM; }
}

static uint16_t sys_kb_id_field_get(const char *ln, const char *tag)
{
    uint64_t at = 0;
    uint64_t off = 0;
    uint64_t val = 0;

    if (ldg_str_substring_find(ln, tag, LDG_SYS_KB_LN_MAX, &at) != LDG_ERR_AOK) { return LDG_SYS_KB_ID_ABSENT; }

    if (sys_kb_prefix_is(&ln[at], tag, &off) != LDG_TRUTH_TRUE) { return LDG_SYS_KB_ID_ABSENT; }

    if (ldg_str_uint_parse(&ln[at + off], LDG_BASE_HEX, LDG_SYS_KB_ID_DIGIT_MAX, &val) != LDG_ERR_AOK) { return LDG_SYS_KB_ID_ABSENT; }

    return (uint16_t)(val & UINT16_MAX);
}

static uint64_t sys_kb_mask_low_get(const char *text)
{
    uint64_t sep = 0;
    uint64_t start = 0;
    uint64_t val = 0;

    if (ldg_str_char_last_find(text, LDG_SYS_KB_WORD_SEP, LDG_SYS_KB_LN_MAX, &sep) == LDG_ERR_AOK) { start = sep + LDG_SYS_KB_STEP; }

    if (ldg_str_uint_parse(&text[start], LDG_BASE_HEX, LDG_SYS_KB_MASK_DIGIT_MAX, &val) != LDG_ERR_AOK) { return LDG_SYS_KB_MASK_ABSENT; }

    return val;
}

static uint8_t sys_kb_ln_strip(char *ln)
{
    uint64_t idx = 0;

    while (idx < LDG_SYS_KB_LN_MAX && ln[idx] != LDG_STR_TERM)
    {
        if (ln[idx] == LDG_ASCII_LF)
        {
            ln[idx] = LDG_STR_TERM;
            return LDG_TRUTH_TRUE;
        }

        idx++;
    }

    return LDG_TRUTH_FALSE;
}

static void sys_kb_ln_take(sys_kb_rec_t *rec, const char *ln)
{
    uint64_t off = 0;

    if (sys_kb_prefix_is(ln, LDG_SYS_KB_ID_TAG, &off) == LDG_TRUTH_TRUE)
    {
        rec->kb.vid = sys_kb_id_field_get(ln, LDG_SYS_KB_VENDOR_TAG);
        rec->kb.pid = sys_kb_id_field_get(ln, LDG_SYS_KB_PRODUCT_TAG);
        rec->used = LDG_TRUTH_TRUE;
    }
    else if (sys_kb_prefix_is(ln, LDG_SYS_KB_NAME_TAG, &off) == LDG_TRUTH_TRUE)
    {
        sys_kb_text_cpy(rec->kb.name, LDG_SYS_KB_NAME_MAX, &ln[off], LDG_SYS_KB_QUOTE);
        rec->used = LDG_TRUTH_TRUE;
    }
    else if (sys_kb_prefix_is(ln, LDG_SYS_KB_PHYS_TAG, &off) == LDG_TRUTH_TRUE)
    {
        sys_kb_text_cpy(rec->phys, LDG_SYS_KB_PHYS_MAX, &ln[off], LDG_STR_TERM);
        sys_kb_phys_trim(rec->phys);
        rec->used = LDG_TRUTH_TRUE;
    }
    else if (sys_kb_prefix_is(ln, LDG_SYS_KB_EV_TAG, &off) == LDG_TRUTH_TRUE)
    {
        rec->ev = sys_kb_mask_low_get(&ln[off]);
        rec->used = LDG_TRUTH_TRUE;
    }
    else if (sys_kb_prefix_is(ln, LDG_SYS_KB_KEY_TAG, &off) == LDG_TRUTH_TRUE)
    {
        rec->key = sys_kb_mask_low_get(&ln[off]);
        rec->used = LDG_TRUTH_TRUE;
    }
}

static uint32_t sys_kb_rec_commit(const sys_kb_rec_t *rec, ldg_sys_kb_t *out, uint32_t cap, sys_kb_tally_t *tally)
{
    const char *ident = rec->phys;
    uint64_t ident_len = 0;
    uint64_t hash = 0;
    uint32_t probe = 0;
    uint32_t ret = 0;

    if (rec->used != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if ((rec->ev & LDG_BIT(LDG_SYS_KB_EV_KEY_BIT)) == LDG_SYS_KB_MASK_ABSENT) { return LDG_ERR_AOK; }

    if ((rec->key & LDG_SYS_KB_TYPING_MASK) != LDG_SYS_KB_TYPING_MASK) { return LDG_ERR_AOK; }

    if (ident[0] == LDG_STR_TERM) { ident = rec->kb.name; }

    ret = ldg_str_len_get(ident, LDG_SYS_KB_PHYS_MAX, &ident_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_hash_fnv1a_64_compute((const uint8_t *)ident, ident_len, &hash);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    hash ^= ((uint64_t)rec->kb.vid << LDG_SYS_KB_VID_SHIFT) | (uint64_t)rec->kb.pid;

    for (probe = 0; probe < tally->seen_cunt; probe++) { if (tally->seen[probe] == hash) { return LDG_ERR_AOK; } }

    if (tally->seen_cunt < LDG_SYS_KB_SEEN_MAX)
    {
        tally->seen[tally->seen_cunt] = hash;
        tally->seen_cunt++;
    }

    if (tally->total < cap) { out[tally->total] = rec->kb; }

    tally->total++;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_kb_list(ldg_sys_kb_t *out, uint32_t cap, uint32_t *out_cunt)
{
    char ln[LDG_SYS_KB_LN_MAX] = LDG_ARR_ZERO_INIT;
    sys_kb_rec_t rec = LDG_STRUCT_ZERO_INIT;
    sys_kb_tally_t tally = LDG_STRUCT_ZERO_INIT;
    FILE *devs = 0x0;
    uint32_t ret = 0;
    uint8_t whole = LDG_TRUTH_TRUE;
    uint8_t ln_whole = LDG_TRUTH_TRUE;

    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    devs = fopen(LDG_SYS_KB_DEVS_PATH, LDG_SYS_KB_RD_MODE);
    if (LDG_UNLIKELY(!devs)) { return LDG_ERR_UNSUPPORTED; }

    while (fgets(ln, LDG_SYS_KB_LN_MAX, devs))
    {
        ln_whole = sys_kb_ln_strip(ln);

        if (whole == LDG_TRUTH_TRUE)
        {
            if (ln[0] == LDG_STR_TERM)
            {
                ret = sys_kb_rec_commit(&rec, out, cap, &tally);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { break; }

                if (LDG_UNLIKELY(ldg_mem_secure_zero(&rec, (uint64_t)sizeof(sys_kb_rec_t)) != LDG_ERR_AOK))
                {
                    ret = LDG_ERR_MEM_BAD;
                    break;
                }
            }
            else { sys_kb_ln_take(&rec, ln); }
        }

        whole = ln_whole;
    }

    if (LDG_UNLIKELY(fclose(devs) != 0)) { return LDG_ERR_IO_CLOSE; }

    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sys_kb_rec_commit(&rec, out, cap, &tally);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    *out_cunt = (tally.total > cap) ? cap : tally.total;

    if (tally.total > cap) { return LDG_ERR_FULL; }

    return LDG_ERR_AOK;
}
