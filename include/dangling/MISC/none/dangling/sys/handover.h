#ifndef LDG_SYS_HANDOVER_H
#define LDG_SYS_HANDOVER_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT uint32_t ldg_sys_handover_set(const void *handover);
LDG_EXPORT uint32_t ldg_sys_sym_get(const char *name, uint32_t *out);

#endif
