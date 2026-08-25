#include <stdio.h>
#include <io.h>
#include <windows.h>

#include <dangling/sys/tty.h>
#include <dangling/core/err.h>

uint8_t ldg_sys_tty_stdout_is(void)
{
    if (_isatty(_fileno(stdout)) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
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
