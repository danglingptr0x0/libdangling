#include <string.h>

#include <dangling/str/str.h>
#include <dangling/core/err.h>
#include <dangling/core/arith.h>

uint32_t ldg_strrbrcpy(char *dst, const char *src, uint64_t abssize)
{
    uint64_t src_len = 0;
    uint8_t overlap = 0;

    if (LDG_UNLIKELY(!dst || !src || abssize == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    src_len = (uint64_t)strnlen(src, abssize);
    if (LDG_UNLIKELY(src_len >= abssize)) { return LDG_ERR_STR_TRUNC; }

    overlap = (dst < src + src_len) && (src < dst + abssize);

    if (overlap) { if (memmove(dst, src, src_len) != dst) { return LDG_ERR_MEM_BAD; } }
    else { if (memcpy(dst, src, src_len) != dst) { return LDG_ERR_MEM_BAD; } }

    if (memset(dst + src_len, 0, abssize - src_len) != dst + src_len) { return LDG_ERR_MEM_BAD; }

    return overlap ? LDG_ERR_STR_OVERLAP : LDG_ERR_AOK;
}

void ldg_byte_to_hex(byte_t val, char out[3])
{
    const char *diggies = "0123456789ABCDEF";

    if (!out) { return; }

    out[0] = diggies[(val >> 4) & 0xF];
    out[1] = diggies[val & 0xF];
    out[2] = '\0';
}

void ldg_dword_to_hex(dword_t val, char *buff)
{
    uint32_t i = 0;
    byte_t nipple = 0;

    if (!buff) { return; }

    for (i = 0; i < 8; i++)
    {
        nipple = (val >> ((7 - i) * 4)) & 0xF;
        buff[i] = (char)((nipple < 10) ? ('0' + nipple) : ('A' + nipple - 10));
    }

    buff[8] = '\0';
}

uint32_t ldg_str_to_dec(const char *str, dword_t *out)
{
    dword_t tmp = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!str || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    while (*str)
    {
        if (*str < '0' || *str > '9') { *out = 0; return LDG_ERR_FUNC_ARG_INVALID; }

        ret = ldg_arith_32_mul(*out, 10, &tmp);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { *out = 0; return LDG_ERR_OVERFLOW; }

        ret = ldg_arith_32_add(tmp, (dword_t)(*str - '0'), out);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { *out = 0; return LDG_ERR_OVERFLOW; }

        str++;
    }

    return LDG_ERR_AOK;
}

void ldg_hex_to_nipple(char c, byte_t *nipple)
{
    if (!nipple) { return; }

    if (c >= '0' && c <= '9') { *nipple = (byte_t)(c - '0'); }
    else if (c >= 'A' && c <= 'F') { *nipple = (byte_t)(c - 'A' + 10); }
    else if (c >= 'a' && c <= 'f') { *nipple = (byte_t)(c - 'a' + 10); }
    else { *nipple = 0xFF; }
}

uint32_t ldg_hex_to_dword(const char *str, dword_t *out)
{
    byte_t nipple = 0;
    uint32_t digit_cunt = 0;

    if (LDG_UNLIKELY(!str || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) { str += 2; }

    while (*str)
    {
        ldg_hex_to_nipple(*str, &nipple);

        if (nipple == 0xFF) { *out = 0; return LDG_ERR_FUNC_ARG_INVALID; }

        if (LDG_UNLIKELY(digit_cunt >= 8)) { *out = 0; return LDG_ERR_OVERFLOW; }

        *out = (*out << 4) | nipple;
        digit_cunt++;
        str++;
    }

    return LDG_ERR_AOK;
}

void ldg_hex_to_bytes(const char *hex, byte_t *out, uint64_t max)
{
    uint64_t i = 0;
    byte_t high = 0;
    byte_t low = 0;

    if (!hex || !out || max == 0) { return; }

    while (*hex && *(hex + 1) && i < max)
    {
        ldg_hex_to_nipple(*hex, &high);
        ldg_hex_to_nipple(*(hex + 1), &low);

        if (high == 0xFF || low == 0xFF) { break; }

        out[i++] = (uint8_t)((high << 4) | low);
        hex += 2;
    }

    while (i < max) { out[i++] = 0; }
}

uint8_t ldg_hex_str_is(const char *str)
{
    byte_t nipple = 0;

    if (!str) { return 0; }

    if (str[0] != '0' || (str[1] != 'x' && str[1] != 'X')) { return 0; }

    if (!str[2]) { return 0; }

    str += 2;

    while (*str)
    {
        ldg_hex_to_nipple(*str, &nipple);
        if (nipple == 0xFF) { return 0; }

        str++;
    }

    return 1;
}
