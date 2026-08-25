#ifndef LDG_SYS_CLIPBOARD_H
#define LDG_SYS_CLIPBOARD_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_SYS_CLIPBOARD_MAX 65536

LDG_EXPORT uint32_t ldg_sys_clipboard_set(const ldg_byte_t *text, uint64_t len);
LDG_EXPORT uint32_t ldg_sys_clipboard_get(ldg_byte_t *out, uint64_t cap, uint64_t *out_len);

#endif
