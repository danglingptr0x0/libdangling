#include <stdio.h>
#include <io.h>
#include <windows.h>

#include <dangling/sys/tty.h>
#include <dangling/io/console.h>
#include <dangling/str/fmt.h>
#include <dangling/core/err.h>
#include <dangling/logging/logging.h>

#define LDG_SYS_TTY_KB_ON_FMT "\x1b[>%u32u\x1b[?u"
#define LDG_SYS_TTY_KB_OFF "\x1b[<u"
#define LDG_SYS_TTY_KB_OFF_LEN 4
#define LDG_SYS_TTY_KB_SEQ_CAP 32
#define LDG_SYS_TTY_OTHER_KEYS_ON "\x1b[>4;2m"
#define LDG_SYS_TTY_OTHER_KEYS_ON_LEN 7
#define LDG_SYS_TTY_OTHER_KEYS_OFF "\x1b[>4m"
#define LDG_SYS_TTY_OTHER_KEYS_OFF_LEN 5

#define LDG_SYS_TTY_INCLUSIVE_SPAN 1

#define LDG_SYS_TTY_BREAK_REC_CUNT 2
#define LDG_SYS_TTY_BREAK_REPEAT 1

static BOOL WINAPI tty_break_cb(DWORD type)
{
    INPUT_RECORD rec[LDG_SYS_TTY_BREAK_REC_CUNT] = LDG_ARR_ZERO_INIT;
    DWORD written = 0;
    uint32_t idx = 0;

    if (type != CTRL_BREAK_EVENT) { return FALSE; }

    for (idx = 0; idx < LDG_SYS_TTY_BREAK_REC_CUNT; idx++)
    {
        rec[idx].EventType = KEY_EVENT;
        rec[idx].Event.KeyEvent.wRepeatCount = LDG_SYS_TTY_BREAK_REPEAT;
        rec[idx].Event.KeyEvent.wVirtualKeyCode = VK_PAUSE;
        rec[idx].Event.KeyEvent.dwControlKeyState = LEFT_CTRL_PRESSED;
    }

    rec[0].Event.KeyEvent.bKeyDown = TRUE;

    if (LDG_UNLIKELY(WriteConsoleInputW(GetStdHandle(STD_INPUT_HANDLE), rec, LDG_SYS_TTY_BREAK_REC_CUNT, &written) == FALSE)) { if (LDG_UNLIKELY(ldg_logging_line_wr(LDG_LOGGING_CON, LDG_LOGGING_LVL_WARN, "tty: break key injection failed; code: %u32", (uint32_t)GetLastError()) != LDG_ERR_AOK)) { return TRUE; } }

    return TRUE;
}

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

    *cols = (uint32_t)(csbi.srWindow.Right - csbi.srWindow.Left + LDG_SYS_TTY_INCLUSIVE_SPAN);

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_tty_size_get(uint32_t *out_col_cunt, uint32_t *out_row_cunt)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!out_col_cunt || !out_row_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_col_cunt = 0;
    *out_row_cunt = 0;

    if (LDG_UNLIKELY(!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))) { return LDG_ERR_UNSUPPORTED; }

    *out_col_cunt = (uint32_t)(csbi.srWindow.Right - csbi.srWindow.Left + LDG_SYS_TTY_INCLUSIVE_SPAN);
    *out_row_cunt = (uint32_t)(csbi.srWindow.Bottom - csbi.srWindow.Top + LDG_SYS_TTY_INCLUSIVE_SPAN);

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_tty_kb_report_en_set(uint32_t flags, uint8_t en)
{
    char seq[LDG_SYS_TTY_KB_SEQ_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(en != LDG_TRUTH_TRUE && en != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(flags > LDG_SYS_TTY_KB_FLAGS_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (en == LDG_TRUTH_FALSE) { return ldg_io_console_wr((const uint8_t *)LDG_SYS_TTY_KB_OFF, LDG_SYS_TTY_KB_OFF_LEN); }

    ret = ldg_str_fmt(seq, (uint64_t)sizeof(seq), &len, LDG_SYS_TTY_KB_ON_FMT, flags);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return ldg_io_console_wr((const uint8_t *)seq, len);
}

uint32_t ldg_sys_tty_other_keys_en_set(uint8_t en)
{
    if (LDG_UNLIKELY(en != LDG_TRUTH_TRUE && en != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (en == LDG_TRUTH_FALSE) { return ldg_io_console_wr((const uint8_t *)LDG_SYS_TTY_OTHER_KEYS_OFF, LDG_SYS_TTY_OTHER_KEYS_OFF_LEN); }

    return ldg_io_console_wr((const uint8_t *)LDG_SYS_TTY_OTHER_KEYS_ON, LDG_SYS_TTY_OTHER_KEYS_ON_LEN);
}

uint8_t ldg_sys_tty_open_is(void)
{
    HANDLE src = 0x0;
    DWORD mode = 0;

    src = GetStdHandle(STD_INPUT_HANDLE);
    if (src == INVALID_HANDLE_VALUE || !src) { return LDG_TRUTH_FALSE; }

    if (!GetConsoleMode(src, &mode)) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_sys_tty_vt_en_set(uint8_t en)
{
    HANDLE out = 0x0;
    DWORD mode = 0;

    if (LDG_UNLIKELY(en != LDG_TRUTH_TRUE && en != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (LDG_UNLIKELY(out == INVALID_HANDLE_VALUE || !out)) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(!GetConsoleMode(out, &mode))) { return LDG_ERR_UNSUPPORTED; }

    if (en == LDG_TRUTH_TRUE) { mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN; }
    else { mode &= ~(DWORD)(ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN); }

    if (LDG_UNLIKELY(!SetConsoleMode(out, mode))) { return LDG_ERR_UNSUPPORTED; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_tty_raw_en_set(ldg_sys_tty_raw_t *raw, uint8_t en)
{
    HANDLE src = 0x0;
    DWORD mode = 0;
    uint8_t *mode_byte = (uint8_t *)&mode;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!raw)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(en != LDG_TRUTH_TRUE && en != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    src = GetStdHandle(STD_INPUT_HANDLE);
    if (LDG_UNLIKELY(src == INVALID_HANDLE_VALUE || !src)) { return LDG_ERR_UNSUPPORTED; }

    if (raw->is_active == en) { return LDG_ERR_AOK; }

    if (en == LDG_TRUTH_FALSE)
    {
        for (idx = 0; idx < sizeof(mode); idx++) { mode_byte[idx] = raw->state[idx]; }

        if (LDG_UNLIKELY(!SetConsoleMode(src, mode))) { return LDG_ERR_UNSUPPORTED; }

        raw->is_active = LDG_TRUTH_FALSE;

        if (LDG_UNLIKELY(!SetConsoleCtrlHandler(tty_break_cb, FALSE))) { return LDG_ERR_UNSUPPORTED; }

        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY(!GetConsoleMode(src, &mode))) { return LDG_ERR_UNSUPPORTED; }

    for (idx = 0; idx < sizeof(mode); idx++) { raw->state[idx] = mode_byte[idx]; }

    mode &= ~(DWORD)(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT);

    if (LDG_UNLIKELY(!SetConsoleMode(src, mode))) { return LDG_ERR_UNSUPPORTED; }

    if (LDG_UNLIKELY(!SetConsoleCtrlHandler(tty_break_cb, TRUE)))
    {
        for (idx = 0; idx < sizeof(mode); idx++) { mode_byte[idx] = raw->state[idx]; }

        if (LDG_UNLIKELY(!SetConsoleMode(src, mode)) && LDG_UNLIKELY(ldg_logging_line_wr(LDG_LOGGING_CON, LDG_LOGGING_LVL_WARN, "tty: console mode restore failed; code: %u32", (uint32_t)GetLastError()) != LDG_ERR_AOK)) { return LDG_ERR_UNSUPPORTED; }

        return LDG_ERR_UNSUPPORTED;
    }

    raw->is_active = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}
