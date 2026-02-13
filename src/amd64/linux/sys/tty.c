#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <dangling/sys/tty.h>
#include <dangling/core/err.h>

uint32_t ldg_sys_tty_stdout_is(uint8_t *is_tty)
{
    int32_t result = 0;

    if (LDG_UNLIKELY(!is_tty)) { return LDG_ERR_FUNC_ARG_NULL; }

    *is_tty = 0;

    result = (int32_t)isatty(fileno(stdout));
    *is_tty = (result != 0);

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_tty_width_get(uint32_t *cols)
{
    struct winsize ws = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!cols)) { return LDG_ERR_FUNC_ARG_NULL; }

    *cols = 0;

    if (LDG_UNLIKELY(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0)) { return LDG_ERR_UNSUPPORTED; }

    *cols = (uint32_t)ws.ws_col;

    return LDG_ERR_AOK;
}
