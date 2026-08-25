#include <dangling/sys/clipboard.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

uint32_t ldg_sys_clipboard_set(const ldg_byte_t *text, uint64_t len)
{
    if (LDG_UNLIKELY(!text)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_clipboard_get(ldg_byte_t *out, uint64_t cap, uint64_t *out_len)
{
    if (LDG_UNLIKELY(!out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_UNSUPPORTED;
}
