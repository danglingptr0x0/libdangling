#include <stdint.h>

#include <dangling/sys/scan.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

uint32_t ldg_sys_scan_ctx_create(const char *app_name, void **out)
{
    if (LDG_UNLIKELY(!app_name || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_scan_ctx_destroy(void **ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    *ctx = 0x0;

    return LDG_ERR_UNSUPPORTED;
}

uint8_t ldg_sys_scan_clean_is(void *ctx, const ldg_byte_t *buff, uint64_t len, const char *content_name)
{
    if (LDG_UNLIKELY(!ctx || !buff || !content_name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    return LDG_ERR_UNSUPPORTED;
}
