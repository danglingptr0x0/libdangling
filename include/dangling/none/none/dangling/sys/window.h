#ifndef LDG_SYS_WINDOW_H
#define LDG_SYS_WINDOW_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_SYS_WINDOW_FLASH_TGT_CAPTION 0x1U
#define LDG_SYS_WINDOW_FLASH_TGT_TRAY 0x2U
#define LDG_SYS_WINDOW_FLASH_TGT_ALL (LDG_SYS_WINDOW_FLASH_TGT_CAPTION | LDG_SYS_WINDOW_FLASH_TGT_TRAY)

#define LDG_SYS_WINDOW_FLASH_CUNT_DEFAULT 3U
#define LDG_SYS_WINDOW_FLASH_PERIOD_SYS 0U

typedef struct ldg_sys_window_flash_conf
{
    uint32_t cunt;
    uint32_t period;
    uint32_t tgt;
    uint8_t until_focus;
    uint8_t pudding[3];
} ldg_sys_window_flash_conf_t;

LDG_EXPORT uint32_t ldg_sys_window_flash_conf_default_get(ldg_sys_window_flash_conf_t *out);
LDG_EXPORT uint32_t ldg_sys_window_flash(void *native_window, const ldg_sys_window_flash_conf_t *conf);
LDG_EXPORT uint32_t ldg_sys_window_flash_stop(void *native_window);

#endif
