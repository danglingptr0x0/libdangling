#include <windows.h>

#include <dangling/sys/window.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

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
    FLASHWINFO info = LDG_STRUCT_ZERO_INIT;
    DWORD flags = 0;

    if (LDG_UNLIKELY(!native_window || !conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(conf->tgt == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY((conf->tgt & ~(uint32_t)LDG_SYS_WINDOW_FLASH_TGT_ALL) != 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(conf->until_focus > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(IsWindow((HWND)native_window) == FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if ((conf->tgt & LDG_SYS_WINDOW_FLASH_TGT_CAPTION) != 0) { flags |= FLASHW_CAPTION; }

    if ((conf->tgt & LDG_SYS_WINDOW_FLASH_TGT_TRAY) != 0) { flags |= FLASHW_TRAY; }

    if (conf->until_focus == LDG_TRUTH_TRUE) { flags |= FLASHW_TIMERNOFG; }

    info.cbSize = (UINT)sizeof(FLASHWINFO);
    info.hwnd = (HWND)native_window;
    info.dwFlags = flags;
    info.uCount = (UINT)conf->cunt;
    info.dwTimeout = (DWORD)conf->period;

    FlashWindowEx(&info);

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_window_flash_stop(void *native_window)
{
    FLASHWINFO info = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!native_window)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(IsWindow((HWND)native_window) == FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    info.cbSize = (UINT)sizeof(FLASHWINFO);
    info.hwnd = (HWND)native_window;
    info.dwFlags = FLASHW_STOP;

    FlashWindowEx(&info);

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_window_term_get(void **out)
{
    HWND hwnd = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0x0;

    hwnd = GetConsoleWindow();
    if (LDG_UNLIKELY(!hwnd)) { return LDG_ERR_NOT_FOUND; }

    *out = (void *)hwnd;

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_window_capture_exclude_set(void *native_window, uint8_t en)
{
    if (LDG_UNLIKELY(!native_window)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(en > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(IsWindow((HWND)native_window) == FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (en == LDG_TRUTH_FALSE)
    {
        if (LDG_UNLIKELY(SetWindowDisplayAffinity((HWND)native_window, WDA_NONE) == FALSE)) { return LDG_ERR_DENIED; }

        return LDG_ERR_AOK;
    }

    if (SetWindowDisplayAffinity((HWND)native_window, WDA_EXCLUDEFROMCAPTURE) != FALSE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(GetLastError() == ERROR_ACCESS_DENIED)) { return LDG_ERR_DENIED; }

    if (SetWindowDisplayAffinity((HWND)native_window, WDA_MONITOR) != FALSE) { return LDG_ERR_AOK; }

    return LDG_ERR_UNSUPPORTED;
}

uint8_t ldg_sys_window_capture_active_is(void)
{
    return LDG_TRUTH_INDETERMINATE;
}
