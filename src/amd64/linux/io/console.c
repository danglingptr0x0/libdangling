#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <termios.h>

#include <dangling/io/console.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>

#define LDG_IO_CONSOLE_OUT_FD 1
#define LDG_IO_CONSOLE_IN_FD 0
#define LDG_IO_CONSOLE_WATCH_CUNT 1
#define LDG_IO_CONSOLE_POLL_NO_WAIT 0
#define LDG_IO_CONSOLE_RAW_MIN 0
#define LDG_IO_CONSOLE_RAW_TIME 0
#define LDG_IO_CONSOLE_STEP 1
#define LDG_IO_CONSOLE_CSI_LEAD '['
#define LDG_IO_CONSOLE_SS3_LEAD 'O'
#define LDG_IO_CONSOLE_CSI_PARAM_SEP ';'
#define LDG_IO_CONSOLE_CSI_PARAM_MAX 2
#define LDG_IO_CONSOLE_CSI_PARAM_CAP 1000
#define LDG_IO_CONSOLE_CSI_LEN_MAX 16
#define LDG_IO_CONSOLE_CSI_FINAL_MIN 0x40
#define LDG_IO_CONSOLE_CSI_FINAL_MAX 0x7E
#define LDG_IO_CONSOLE_CSI_UP 'A'
#define LDG_IO_CONSOLE_CSI_DOWN 'B'
#define LDG_IO_CONSOLE_CSI_RHS 'C'
#define LDG_IO_CONSOLE_CSI_LHS 'D'
#define LDG_IO_CONSOLE_CSI_HOME 'H'
#define LDG_IO_CONSOLE_CSI_END 'F'
#define LDG_IO_CONSOLE_CSI_F1 'P'
#define LDG_IO_CONSOLE_CSI_F2 'Q'
#define LDG_IO_CONSOLE_CSI_F3 'R'
#define LDG_IO_CONSOLE_CSI_F4 'S'
#define LDG_IO_CONSOLE_CSI_TILDE '~'
#define LDG_IO_CONSOLE_VT_HOME 1
#define LDG_IO_CONSOLE_VT_INSERT 2
#define LDG_IO_CONSOLE_VT_DELETE 3
#define LDG_IO_CONSOLE_VT_END 4
#define LDG_IO_CONSOLE_VT_PAGE_UP 5
#define LDG_IO_CONSOLE_VT_PAGE_DOWN 6
#define LDG_IO_CONSOLE_VT_HOME_ALT 7
#define LDG_IO_CONSOLE_VT_END_ALT 8
#define LDG_IO_CONSOLE_VT_F1 11
#define LDG_IO_CONSOLE_VT_F2 12
#define LDG_IO_CONSOLE_VT_F3 13
#define LDG_IO_CONSOLE_VT_F4 14
#define LDG_IO_CONSOLE_VT_F5 15
#define LDG_IO_CONSOLE_VT_F6 17
#define LDG_IO_CONSOLE_VT_F7 18
#define LDG_IO_CONSOLE_VT_F8 19
#define LDG_IO_CONSOLE_VT_F9 20
#define LDG_IO_CONSOLE_VT_F10 21
#define LDG_IO_CONSOLE_VT_F11 23
#define LDG_IO_CONSOLE_VT_F12 24
#define LDG_IO_CONSOLE_VT_MOD_BIAS 1
#define LDG_IO_CONSOLE_VT_MOD_MASK 0x7U
#define LDG_IO_CONSOLE_BACKSPACE_CTRL 0x08
#define LDG_IO_CONSOLE_CTRL_LETTER_MIN 0x01
#define LDG_IO_CONSOLE_CTRL_LETTER_MAX 0x1A
#define LDG_IO_CONSOLE_CTRL_LETTER_BIAS 0x60

typedef struct ldg_io_console_state
{
    struct termios saved;
    uint8_t raw;
    uint8_t pudding[3];
} ldg_io_console_state_t;

static ldg_io_console_state_t g_io_console = { .raw = LDG_TRUTH_FALSE };

uint32_t ldg_io_console_wr(const uint8_t *data, uint64_t len)
{
    uint64_t done = 0;
    int64_t written = 0;

    if (LDG_UNLIKELY(!data)) { return LDG_ERR_FUNC_ARG_NULL; }

    while (done < len)
    {
        written = (int64_t)write(LDG_IO_CONSOLE_OUT_FD, data + done, len - done);

        if (written < 0)
        {
            if (errno == EINTR) { continue; }

            return LDG_ERR_IO_WR;
        }

        if (written == 0) { return LDG_ERR_IO_WR; }

        done += (uint64_t)written;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_console_byte_wr(uint8_t byte)
{
    int64_t written = 0;

    while (written <= 0)
    {
        written = (int64_t)write(LDG_IO_CONSOLE_OUT_FD, &byte, (uint64_t)sizeof(byte));

        if (written == 0) { return LDG_ERR_IO_WR; }

        if (written < 0) { if (errno != EINTR) { return LDG_ERR_IO_WR; } }
    }

    return LDG_ERR_AOK;
}

uint8_t ldg_io_console_rd_ready_is(void)
{
    struct pollfd watch = LDG_STRUCT_ZERO_INIT;
    int32_t ready = 0;

    watch.fd = LDG_IO_CONSOLE_IN_FD;
    watch.events = POLLIN;

    ready = (int32_t)poll(&watch, LDG_IO_CONSOLE_WATCH_CUNT, LDG_IO_CONSOLE_POLL_NO_WAIT);

    if (ready < 0) { return LDG_ERR_IO_RD; }

    if (ready == 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_io_console_byte_rd(uint8_t *out)
{
    int64_t got = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    got = (int64_t)read(LDG_IO_CONSOLE_IN_FD, out, sizeof(*out));

    if (got < 0)
    {
        if (errno == EAGAIN) { return LDG_ERR_WOULD_BLOCK; }

        return LDG_ERR_IO_RD;
    }

    if (got == 0) { return LDG_ERR_EOF; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_console_rd(uint8_t *dst, uint64_t cap, uint64_t *out_len)
{
    struct pollfd watch = LDG_STRUCT_ZERO_INIT;
    int64_t got = 0;
    int32_t ready = 0;

    if (LDG_UNLIKELY(!dst || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (cap == 0) { return LDG_ERR_AOK; }

    watch.fd = LDG_IO_CONSOLE_IN_FD;
    watch.events = POLLIN;

    ready = (int32_t)poll(&watch, LDG_IO_CONSOLE_WATCH_CUNT, LDG_IO_CONSOLE_POLL_NO_WAIT);

    if (ready < 0)
    {
        if (errno == EINTR) { return LDG_ERR_AOK; }

        return LDG_ERR_IO_RD;
    }

    if (ready == 0) { return LDG_ERR_AOK; }

    got = (int64_t)read(LDG_IO_CONSOLE_IN_FD, dst, cap);

    if (got < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) { return LDG_ERR_AOK; }

        return LDG_ERR_IO_RD;
    }

    if (got == 0) { return LDG_ERR_EOF; }

    *out_len = (uint64_t)got;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_console_raw_set(uint8_t en)
{
    struct termios raw = LDG_STRUCT_ZERO_INIT;

    if (en == LDG_TRUTH_TRUE)
    {
        if (g_io_console.raw == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

        if (tcgetattr(LDG_IO_CONSOLE_IN_FD, &g_io_console.saved) != 0) { return LDG_ERR_UNSUPPORTED; }

        raw = g_io_console.saved;
        raw.c_iflag &= ~(tcflag_t)(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
        raw.c_lflag &= ~(tcflag_t)(ICANON | ECHO | ISIG | IEXTEN);
        raw.c_cc[VMIN] = LDG_IO_CONSOLE_RAW_MIN;
        raw.c_cc[VTIME] = LDG_IO_CONSOLE_RAW_TIME;

        if (tcsetattr(LDG_IO_CONSOLE_IN_FD, TCSANOW, &raw) != 0) { return LDG_ERR_DENIED; }

        g_io_console.raw = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (g_io_console.raw != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (tcsetattr(LDG_IO_CONSOLE_IN_FD, TCSANOW, &g_io_console.saved) != 0) { return LDG_ERR_DENIED; }

    g_io_console.raw = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint32_t console_byte_take(uint8_t *out)
{
    struct pollfd watch = LDG_STRUCT_ZERO_INIT;
    int64_t got = 0;

    watch.fd = LDG_IO_CONSOLE_IN_FD;
    watch.events = POLLIN;

    if (poll(&watch, LDG_IO_CONSOLE_WATCH_CUNT, LDG_IO_CONSOLE_POLL_NO_WAIT) <= 0) { return LDG_ERR_EMPTY; }

    got = (int64_t)read(LDG_IO_CONSOLE_IN_FD, out, sizeof(*out));
    if (got == 0) { return LDG_ERR_EOF; }

    if (got < 0) { return (errno == EAGAIN || errno == EINTR) ? LDG_ERR_EMPTY : LDG_ERR_IO_RD; }

    return LDG_ERR_AOK;
}

static uint32_t console_tilde_kind_get(uint32_t code)
{
    switch (code)
    {
        case LDG_IO_CONSOLE_VT_HOME:
        case LDG_IO_CONSOLE_VT_HOME_ALT:
            return LDG_IO_CONSOLE_KEY_HOME;
        case LDG_IO_CONSOLE_VT_END:
        case LDG_IO_CONSOLE_VT_END_ALT:
            return LDG_IO_CONSOLE_KEY_END;
        case LDG_IO_CONSOLE_VT_INSERT:
            return LDG_IO_CONSOLE_KEY_INSERT;
        case LDG_IO_CONSOLE_VT_DELETE:
            return LDG_IO_CONSOLE_KEY_DELETE;
        case LDG_IO_CONSOLE_VT_PAGE_UP:
            return LDG_IO_CONSOLE_KEY_PAGE_UP;
        case LDG_IO_CONSOLE_VT_PAGE_DOWN:
            return LDG_IO_CONSOLE_KEY_PAGE_DOWN;
        case LDG_IO_CONSOLE_VT_F1:
            return LDG_IO_CONSOLE_KEY_F1;
        case LDG_IO_CONSOLE_VT_F2:
            return LDG_IO_CONSOLE_KEY_F2;
        case LDG_IO_CONSOLE_VT_F3:
            return LDG_IO_CONSOLE_KEY_F3;
        case LDG_IO_CONSOLE_VT_F4:
            return LDG_IO_CONSOLE_KEY_F4;
        case LDG_IO_CONSOLE_VT_F5:
            return LDG_IO_CONSOLE_KEY_F5;
        case LDG_IO_CONSOLE_VT_F6:
            return LDG_IO_CONSOLE_KEY_F6;
        case LDG_IO_CONSOLE_VT_F7:
            return LDG_IO_CONSOLE_KEY_F7;
        case LDG_IO_CONSOLE_VT_F8:
            return LDG_IO_CONSOLE_KEY_F8;
        case LDG_IO_CONSOLE_VT_F9:
            return LDG_IO_CONSOLE_KEY_F9;
        case LDG_IO_CONSOLE_VT_F10:
            return LDG_IO_CONSOLE_KEY_F10;
        case LDG_IO_CONSOLE_VT_F11:
            return LDG_IO_CONSOLE_KEY_F11;
        case LDG_IO_CONSOLE_VT_F12:
            return LDG_IO_CONSOLE_KEY_F12;
        default:
            return LDG_IO_CONSOLE_KEY_OTHER;
    }
}

static uint32_t console_final_kind_get(uint8_t final)
{
    switch (final)
    {
        case LDG_IO_CONSOLE_CSI_UP:
            return LDG_IO_CONSOLE_KEY_UP;
        case LDG_IO_CONSOLE_CSI_DOWN:
            return LDG_IO_CONSOLE_KEY_DOWN;
        case LDG_IO_CONSOLE_CSI_RHS:
            return LDG_IO_CONSOLE_KEY_RHS;
        case LDG_IO_CONSOLE_CSI_LHS:
            return LDG_IO_CONSOLE_KEY_LHS;
        case LDG_IO_CONSOLE_CSI_HOME:
            return LDG_IO_CONSOLE_KEY_HOME;
        case LDG_IO_CONSOLE_CSI_END:
            return LDG_IO_CONSOLE_KEY_END;
        case LDG_IO_CONSOLE_CSI_F1:
            return LDG_IO_CONSOLE_KEY_F1;
        case LDG_IO_CONSOLE_CSI_F2:
            return LDG_IO_CONSOLE_KEY_F2;
        case LDG_IO_CONSOLE_CSI_F3:
            return LDG_IO_CONSOLE_KEY_F3;
        case LDG_IO_CONSOLE_CSI_F4:
            return LDG_IO_CONSOLE_KEY_F4;
        default:
            return LDG_IO_CONSOLE_KEY_OTHER;
    }
}

static void console_byte_decode(ldg_io_console_key_t *out, uint8_t byte)
{
    out->kind = LDG_IO_CONSOLE_KEY_CHAR;
    out->codepoint = byte;

    if (byte == LDG_ASCII_CR || byte == LDG_ASCII_LF) { out->kind = LDG_IO_CONSOLE_KEY_ENTER; }
    else if (byte == LDG_ASCII_TAB) { out->kind = LDG_IO_CONSOLE_KEY_TAB; }
    else if (byte == LDG_ASCII_DELETE || byte == LDG_IO_CONSOLE_BACKSPACE_CTRL) { out->kind = LDG_IO_CONSOLE_KEY_BACKSPACE; }
    else if (byte >= LDG_IO_CONSOLE_CTRL_LETTER_MIN && byte <= LDG_IO_CONSOLE_CTRL_LETTER_MAX)
    {
        out->codepoint = (uint32_t)byte + LDG_IO_CONSOLE_CTRL_LETTER_BIAS;
        out->mods = LDG_IO_CONSOLE_MOD_CTRL;
    }

    if (out->kind != LDG_IO_CONSOLE_KEY_CHAR) { out->codepoint = 0; }
}

static void console_seq_decode(ldg_io_console_key_t *out)
{
    uint32_t param[LDG_IO_CONSOLE_CSI_PARAM_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t param_idx = 0;
    uint32_t len = 0;
    uint8_t seen = LDG_TRUTH_FALSE;
    uint8_t byte = 0;

    out->kind = LDG_IO_CONSOLE_KEY_OTHER;

    for (len = 0; len < LDG_IO_CONSOLE_CSI_LEN_MAX; len++)
    {
        if (console_byte_take(&byte) != LDG_ERR_AOK) { return; }

        if (ldg_char_digit_is(byte) == LDG_TRUTH_TRUE)
        {
            seen = LDG_TRUTH_TRUE;
            if (param[param_idx] < LDG_IO_CONSOLE_CSI_PARAM_CAP) { param[param_idx] = param[param_idx] * LDG_BASE_DEC + (uint32_t)(byte - LDG_ASCII_DIGIT_ZERO); }

            continue;
        }

        if (byte == LDG_IO_CONSOLE_CSI_PARAM_SEP)
        {
            seen = LDG_TRUTH_TRUE;
            if (param_idx + LDG_IO_CONSOLE_STEP < LDG_IO_CONSOLE_CSI_PARAM_MAX) { param_idx++; }

            continue;
        }

        if (byte < LDG_IO_CONSOLE_CSI_FINAL_MIN || byte > LDG_IO_CONSOLE_CSI_FINAL_MAX) { return; }

        out->raw = byte;
        out->kind = (byte == LDG_IO_CONSOLE_CSI_TILDE) ? console_tilde_kind_get(param[0]) : console_final_kind_get(byte);

        if (seen == LDG_TRUTH_TRUE && param_idx > 0 && param[param_idx] >= LDG_IO_CONSOLE_VT_MOD_BIAS) { out->mods = (param[param_idx] - LDG_IO_CONSOLE_VT_MOD_BIAS) & LDG_IO_CONSOLE_VT_MOD_MASK; }

        return;
    }
}

uint32_t ldg_io_console_key_rd(ldg_io_console_key_t *out)
{
    uint8_t byte = 0;
    uint8_t lead = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(out, sizeof(ldg_io_console_key_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    ret = console_byte_take(&byte);
    if (ret != LDG_ERR_AOK) { return ret; }

    out->down = LDG_TRUTH_TRUE;
    out->raw = byte;

    if (byte != LDG_ASCII_ESC)
    {
        console_byte_decode(out, byte);
        return LDG_ERR_AOK;
    }

    if (console_byte_take(&lead) != LDG_ERR_AOK)
    {
        out->kind = LDG_IO_CONSOLE_KEY_ESC;
        return LDG_ERR_AOK;
    }

    if (lead == LDG_IO_CONSOLE_CSI_LEAD || lead == LDG_IO_CONSOLE_SS3_LEAD)
    {
        console_seq_decode(out);
        return LDG_ERR_AOK;
    }

    console_byte_decode(out, lead);
    out->mods |= LDG_IO_CONSOLE_MOD_ALT;
    out->raw = lead;

    return LDG_ERR_AOK;
}
