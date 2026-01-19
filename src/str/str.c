#include <dangling/str/str.h>
#include <dangling/core/err.h>

#include <string.h>

int32_t ldg_strrbrcpy(char *dst, const char *src, size_t abssize)
{
    size_t src_len = 0;
    int32_t overlap = 0;

    if (LDG_UNLIKELY(!dst || !src || abssize == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    src_len = strnlen(src, abssize);
    if (LDG_UNLIKELY(src_len >= abssize)) { return LDG_ERR_STR_TRUNC; }

    overlap = (dst < src + src_len) && (src < dst + abssize);

    if (overlap)
    {
        (void)memmove(dst, src, src_len);
    }
    else
    {
        (void)memcpy(dst, src, src_len);
    }

    (void)memset(dst + src_len, 0, abssize - src_len);

    return overlap ? LDG_ERR_STR_OVERLAP : LDG_ERR_OK;
}
