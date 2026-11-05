#include <stdio.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <dangling/sys/tty.h>
#include <dangling/io/console.h>
#include <dangling/str/fmt.h>
#include <dangling/core/err.h>

#define LDG_SYS_TTY_KB_ON_FMT "\x1b[>%u32u\x1b[?u"
#define LDG_SYS_TTY_KB_OFF "\x1b[<u"
#define LDG_SYS_TTY_KB_OFF_LEN 4
#define LDG_SYS_TTY_OTHER_KEYS_ON "\x1b[>4;2m"
#define LDG_SYS_TTY_OTHER_KEYS_ON_LEN 7
#define LDG_SYS_TTY_OTHER_KEYS_OFF "\x1b[>4m"
#define LDG_SYS_TTY_OTHER_KEYS_OFF_LEN 5
#define LDG_SYS_TTY_KB_SEQ_CAP 32

#define LDG_SYS_TTY_RAW_BYTE_MIN 0
#define LDG_SYS_TTY_RAW_WAIT_TENTH 1
#define LDG_SYS_TTY_POLL_FD_CUNT 1
#define LDG_SYS_TTY_POLL_WAIT_NONE 0
#define LDG_SYS_TTY_POLL_EVENT_NONE 0
#define LDG_SYS_TTY_POLL_CLOSED (POLLHUP | POLLERR | POLLNVAL)

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

uint32_t ldg_sys_tty_size_get(uint32_t *out_col_cunt, uint32_t *out_row_cunt)
{
    struct winsize ws = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!out_col_cunt || !out_row_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_col_cunt = 0;
    *out_row_cunt = 0;

    if (LDG_UNLIKELY(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0)) { return LDG_ERR_UNSUPPORTED; }

    *out_col_cunt = (uint32_t)ws.ws_col;
    *out_row_cunt = (uint32_t)ws.ws_row;

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
    struct pollfd watch = LDG_STRUCT_ZERO_INIT;
    int32_t ready = 0;

    watch.fd = STDIN_FILENO;
    watch.events = LDG_SYS_TTY_POLL_EVENT_NONE;

    ready = (int32_t)poll(&watch, LDG_SYS_TTY_POLL_FD_CUNT, LDG_SYS_TTY_POLL_WAIT_NONE);
    if (LDG_UNLIKELY(ready < 0)) { return (uint8_t)LDG_ERR_IO_RD; }

    if ((watch.revents & LDG_SYS_TTY_POLL_CLOSED) != 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_sys_tty_vt_en_set(uint8_t en)
{
    if (LDG_UNLIKELY(en != LDG_TRUTH_TRUE && en != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ldg_sys_tty_stdout_is() != LDG_TRUTH_TRUE)) { return LDG_ERR_UNSUPPORTED; }

    return LDG_ERR_AOK;
}

uint32_t ldg_sys_tty_raw_en_set(ldg_sys_tty_raw_t *raw, uint8_t en)
{
    struct termios conf = LDG_STRUCT_ZERO_INIT;
    uint8_t *conf_byte = (uint8_t *)&conf;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!raw)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(en != LDG_TRUTH_TRUE && en != LDG_TRUTH_FALSE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(sizeof(conf) > LDG_SYS_TTY_RAW_STATE_SIZE)) { return LDG_ERR_UNSUPPORTED; }

    if (raw->is_active == en) { return LDG_ERR_AOK; }

    if (en == LDG_TRUTH_FALSE)
    {
        for (idx = 0; idx < sizeof(conf); idx++) { conf_byte[idx] = raw->state[idx]; }

        if (LDG_UNLIKELY(tcsetattr(STDIN_FILENO, TCSAFLUSH, &conf) != 0)) { return LDG_ERR_UNSUPPORTED; }

        raw->is_active = LDG_TRUTH_FALSE;

        return LDG_ERR_AOK;
    }

    if (LDG_UNLIKELY(isatty(STDIN_FILENO) == 0 || tcgetattr(STDIN_FILENO, &conf) != 0)) { return LDG_ERR_UNSUPPORTED; }

    for (idx = 0; idx < sizeof(conf); idx++) { raw->state[idx] = conf_byte[idx]; }

    conf.c_iflag &= ~(tcflag_t)(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    conf.c_oflag &= ~(tcflag_t)OPOST;
    conf.c_lflag &= ~(tcflag_t)(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    conf.c_cflag &= ~(tcflag_t)(CSIZE | PARENB);
    conf.c_cflag |= CS8;
    conf.c_cc[VMIN] = LDG_SYS_TTY_RAW_BYTE_MIN;
    conf.c_cc[VTIME] = LDG_SYS_TTY_RAW_WAIT_TENTH;

    if (LDG_UNLIKELY(tcsetattr(STDIN_FILENO, TCSAFLUSH, &conf) != 0)) { return LDG_ERR_UNSUPPORTED; }

    raw->is_active = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}
