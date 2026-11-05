#ifndef LDG_SYS_ENTROPY_H
#define LDG_SYS_ENTROPY_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT uint32_t ldg_sys_entropy_get(uint8_t *out, uint32_t len);

#endif
