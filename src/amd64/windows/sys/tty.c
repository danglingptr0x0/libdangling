#include <stdio.h>
#include <io.h>
#include <windows.h>

#include <dangling/sys/tty.h>
#include <dangling/core/err.h>

uint32_t ldg_sys_tty_stdout_is(uint8_t *is_tty)
{
    if (LDG_UNLIKELY(!is_tty)) { return LDG_ERR_FUNC_ARG_NULL; }

    *is_tty = 0;

    *is_tty = (_isatty(_fileno(stdout)) != 0);

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_tty_width_get(uint32_t *cols)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };

    if (LDG_UNLIKELY(!cols)) { return LDG_ERR_FUNC_ARG_NULL; }

    *cols = 0;

    if (LDG_UNLIKELY(!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))) { return LDG_ERR_UNSUPPORTED; }

    *cols = (uint32_t)(csbi.srWindow.Right - csbi.srWindow.Left + 1);

    return LDG_ERR_AOK;
}
