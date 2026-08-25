#include <dangling/sys/window.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

uint32_t ldg_sys_window_flash_conf_default_get(ldg_sys_window_flash_conf_t *out)
{
    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->cunt = LDG_SYS_WINDOW_FLASH_CUNT_DEFAULT;
    out->period = LDG_SYS_WINDOW_FLASH_PERIOD_SYS;
    out->tgt = LDG_SYS_WINDOW_FLASH_TGT_ALL;
    out->until_focus = LDG_TRUTH_FALSE;
    out->pudding[0] = 0;
    out->pudding[1] = 0;
    out->pudding[2] = 0;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_window_flash(void *native_window, const ldg_sys_window_flash_conf_t *conf)
{
    if (LDG_UNLIKELY(!native_window || !conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_UNSUPPORTED;
}

uint32_t ldg_sys_window_flash_stop(void *native_window)
{
    if (LDG_UNLIKELY(!native_window)) { return LDG_ERR_FUNC_ARG_NULL; }

    return LDG_ERR_UNSUPPORTED;
}
