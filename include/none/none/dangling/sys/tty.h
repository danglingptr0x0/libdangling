#ifndef LDG_SYS_TTY_H
#define LDG_SYS_TTY_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT uint32_t ldg_sys_tty_stdout_is(uint8_t *is_tty);
LDG_EXPORT uint32_t ldg_sys_tty_width_get(uint32_t *cols);

#endif
