#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <dangling/sys/tty.h>
#include <dangling/core/err.h>

uint8_t ldg_sys_tty_stdout_is(void)
{
    if (isatty(fileno(stdout)) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
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
