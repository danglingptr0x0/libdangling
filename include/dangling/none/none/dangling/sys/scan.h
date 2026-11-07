#ifndef LDG_SYS_SCAN_H
#define LDG_SYS_SCAN_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_SYS_SCAN_NAME_MAX 256

LDG_EXPORT uint32_t ldg_sys_scan_ctx_create(const char *app_name, void **out);
LDG_EXPORT uint32_t ldg_sys_scan_ctx_destroy(void **ctx);
LDG_EXPORT uint8_t ldg_sys_scan_clean_is(void *ctx, const ldg_byte_t *buff, uint64_t len, const char *content_name);

#endif
