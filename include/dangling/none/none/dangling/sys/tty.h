#ifndef LDG_SYS_TTY_H
#define LDG_SYS_TTY_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_SYS_TTY_RAW_STATE_SIZE 64
#define LDG_SYS_TTY_KB_FLAGS_MAX 0x1FU

typedef struct ldg_sys_tty_raw
{
    uint8_t state[LDG_SYS_TTY_RAW_STATE_SIZE];
    uint8_t is_active;
    uint8_t pudding[7];
} ldg_sys_tty_raw_t;

LDG_EXPORT uint8_t ldg_sys_tty_stdout_is(void);
LDG_EXPORT uint32_t ldg_sys_tty_width_get(uint32_t *cols);
LDG_EXPORT uint32_t ldg_sys_tty_size_get(uint32_t *out_col_cunt, uint32_t *out_row_cunt);
LDG_EXPORT uint8_t ldg_sys_tty_open_is(void);
LDG_EXPORT uint32_t ldg_sys_tty_vt_en_set(uint8_t en);
LDG_EXPORT uint32_t ldg_sys_tty_raw_en_set(ldg_sys_tty_raw_t *raw, uint8_t en);
LDG_EXPORT uint32_t ldg_sys_tty_kb_report_en_set(uint32_t flags, uint8_t en);
LDG_EXPORT uint32_t ldg_sys_tty_other_keys_en_set(uint8_t en);

#endif
