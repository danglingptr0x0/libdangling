#include <dangling/str/str.h>
#include <dangling/core/err.h>

#include <string.h>

uint32_t ldg_strrbrcpy(char *dst, const char *src, size_t abssize)
{
    size_t src_len = 0;
    int32_t overlap = 0;

    if (LDG_UNLIKELY(!dst || !src || abssize == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    src_len = strnlen(src, abssize);
    if (LDG_UNLIKELY(src_len >= abssize)) { return LDG_ERR_STR_TRUNC; }

    overlap = (dst < src + src_len) && (src < dst + abssize);

    if (overlap) { (void)memmove(dst, src, src_len); }
    else{ (void)memcpy(dst, src, src_len); }

    (void)memset(dst + src_len, 0, abssize - src_len);

    return overlap ? LDG_ERR_STR_OVERLAP : LDG_ERR_AOK;
}

void ldg_byte_to_hex(byte_t val, char out[3])
{
    const char *diggies = "0123456789ABCDEF";
    out[0] = diggies[(val >> 4) & 0xF];
    out[1] = diggies[val & 0xF];
    out[2] = '\0';
}

void ldg_dword_to_hex(dword_t val, char *buff)
{
    int i = 0;
    byte_t nipple = 0;

    for (i = 0; i < 8; i++)
    {
        nipple = (val >> ((7 - i) * 4)) & 0xF;
        buff[i] = (nipple < 10) ? ('0' + nipple) : ('A' + nipple - 10);
    }

    buff[8] = '\0';
}

void ldg_str_to_dec(const char *str, dword_t *out)
{
    *out = 0;

    while (*str)
    {
        if (*str >= '0' && *str <= '9') { *out = *out * 10 + (*str - '0'); }
        else
        {
            *out = 0;
            return;
        }

        str++;
    }
}

void ldg_hex_to_nipple(char c, byte_t *nipple)
{
    if (c >= '0' && c <= '9') { *nipple = c - '0'; }
    else if (c >= 'A' && c <= 'F') { *nipple = c - 'A' + 10; }
    else if (c >= 'a' && c <= 'f') { *nipple = c - 'a' + 10; }
    else { *nipple = 0xFF; }
}

void ldg_hex_to_dword(const char *str, dword_t *out)
{
    byte_t nipple = 0;

    *out = 0;

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) { str += 2; }

    while (*str)
    {
        ldg_hex_to_nipple(*str, &nipple);

        if (nipple == 0xFF)
        {
            *out = 0;
            return;
        }

        *out = (*out << 4) | nipple;
        str++;
    }
}

void ldg_hex_to_bytes(const char *hex, byte_t *out, size_t max)
{
    size_t i = 0;
    byte_t high = 0;
    byte_t low = 0;

    while (*hex && *(hex + 1) && i < max)
    {
        ldg_hex_to_nipple(*hex, &high);
        ldg_hex_to_nipple(*(hex + 1), &low);

        if (high == 0xFF || low == 0xFF) { break; }

        out[i++] = (high << 4) | low;
        hex += 2;
    }

    while (i < max) { out[i++] = 0; }
}

int ldg_hex_str_is(const char *str)
{
    byte_t nipple = 0;

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
