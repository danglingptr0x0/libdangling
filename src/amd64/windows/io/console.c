#include <stdint.h>
#include <windows.h>

#include <dangling/io/console.h>
#include <dangling/str/fmt.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_IO_CONSOLE_REC_CAP 64
#define LDG_IO_CONSOLE_CH_NONE 0
#define LDG_IO_CONSOLE_STEP 1
#define LDG_IO_CONSOLE_REPEAT_MIN 1
#define LDG_IO_CONSOLE_WIN32_FMT "\x1b[%u32;%u32;%u32;%u32;%u32;%u32_"
#define LDG_IO_CONSOLE_WIN32_SEQ_CAP 64

typedef struct ldg_io_console_held
{
    uint32_t cunt;
    uint8_t ch;
    uint8_t pudding[3];
} ldg_io_console_held_t;

static ldg_io_console_held_t* console_held_get(void)
{
    static ldg_io_console_held_t held = LDG_STRUCT_ZERO_INIT;

    return &held;
}

static uint8_t console_rec_ch_is(const INPUT_RECORD *rec)
{
    if (rec->EventType != KEY_EVENT) { return LDG_TRUTH_FALSE; }

    if (!rec->Event.KeyEvent.bKeyDown) { return LDG_TRUTH_FALSE; }

    if ((uint8_t)rec->Event.KeyEvent.uChar.AsciiChar == LDG_IO_CONSOLE_CH_NONE) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static void console_held_drain(ldg_io_console_held_t *held, uint8_t *dst, uint64_t cap, uint64_t *len)
{
    while (held->cunt > 0 && *len < cap)
    {
        dst[*len] = held->ch;
        *len += LDG_IO_CONSOLE_STEP;
        held->cunt--;
    }
}

static uint32_t console_in_rd(uint8_t *dst, uint64_t cap, uint64_t *out_len)
{
    INPUT_RECORD rec[LDG_IO_CONSOLE_REC_CAP] = LDG_ARR_ZERO_INIT;
    ldg_io_console_held_t *held = 0x0;
    HANDLE in = 0x0;
    DWORD pending = 0;
    DWORD want = 0;
    DWORD taken = 0;
    DWORD idx = 0;

    *out_len = 0;
    held = console_held_get();

    console_held_drain(held, dst, cap, out_len);

    in = GetStdHandle(STD_INPUT_HANDLE);
    if (LDG_UNLIKELY(in == INVALID_HANDLE_VALUE || !in)) { return LDG_ERR_CLOSED; }

    while (*out_len < cap)
    {
        if (!GetNumberOfConsoleInputEvents(in, &pending)) { return LDG_ERR_CLOSED; }

        if (pending == 0) { return LDG_ERR_AOK; }

        want = pending;
        if (want > LDG_IO_CONSOLE_REC_CAP) { want = LDG_IO_CONSOLE_REC_CAP; }

        if ((uint64_t)want > cap - *out_len) { want = (DWORD)(cap - *out_len); }

        if (LDG_UNLIKELY(!PeekConsoleInputA(in, rec, want, &taken))) { return LDG_ERR_IO_RD; }

        if (taken == 0) { return LDG_ERR_AOK; }

        for (idx = 0; idx < taken && held->cunt == 0; idx++)
        {
            if (console_rec_ch_is(&rec[idx]) != LDG_TRUTH_TRUE) { continue; }

            held->ch = (uint8_t)rec[idx].Event.KeyEvent.uChar.AsciiChar;
            held->cunt = (uint32_t)rec[idx].Event.KeyEvent.wRepeatCount;
            if (held->cunt < LDG_IO_CONSOLE_REPEAT_MIN) { held->cunt = LDG_IO_CONSOLE_REPEAT_MIN; }

            console_held_drain(held, dst, cap, out_len);
        }

        if (LDG_UNLIKELY(!ReadConsoleInputA(in, rec, idx, &taken))) { return LDG_ERR_IO_RD; }

        if (held->cunt > 0) { return LDG_ERR_AOK; }
    }

    return LDG_ERR_AOK;
}

static uint8_t console_native_is(HANDLE in)
{
    DWORD mode = 0;

    if (!GetConsoleMode(in, &mode)) { return LDG_TRUTH_FALSE; }

    if ((mode & (DWORD)(ENABLE_LINE_INPUT | ENABLE_VIRTUAL_TERMINAL_INPUT)) != 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

static uint32_t console_native_rd(HANDLE in, uint8_t *dst, uint64_t cap, uint64_t *out_len)
{
    INPUT_RECORD rec[LDG_IO_CONSOLE_REC_CAP] = LDG_ARR_ZERO_INIT;
    char seq[LDG_IO_CONSOLE_WIN32_SEQ_CAP] = LDG_ARR_ZERO_INIT;
    const KEY_EVENT_RECORD *key = 0x0;
    uint64_t seq_len = 0;
    uint64_t pos = 0;
    uint32_t rep = 0;
    uint32_t ret = 0;
    DWORD pending = 0;
    DWORD want = 0;
    DWORD taken = 0;
    DWORD drained = 0;
    DWORD idx = 0;
    uint8_t full = LDG_TRUTH_FALSE;

    while (full == LDG_TRUTH_FALSE)
    {
        if (!GetNumberOfConsoleInputEvents(in, &pending)) { return LDG_ERR_CLOSED; }

        if (pending == 0) { return LDG_ERR_AOK; }

        want = pending;
        if (want > LDG_IO_CONSOLE_REC_CAP) { want = LDG_IO_CONSOLE_REC_CAP; }

        if (LDG_UNLIKELY(!PeekConsoleInputW(in, rec, want, &taken))) { return LDG_ERR_IO_RD; }

        if (taken == 0) { return LDG_ERR_AOK; }

        for (idx = 0; idx < taken; idx++)
        {
            if (rec[idx].EventType != KEY_EVENT) { continue; }

            key = &rec[idx].Event.KeyEvent;

            ret = ldg_str_fmt(seq, (uint64_t)sizeof(seq), &seq_len, LDG_IO_CONSOLE_WIN32_FMT, (uint32_t)key->wVirtualKeyCode, (uint32_t)key->wVirtualScanCode, (uint32_t)key->uChar.UnicodeChar, (uint32_t)(key->bKeyDown ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE), (uint32_t)key->dwControlKeyState, (uint32_t)key->wRepeatCount);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

            rep = (uint32_t)key->wRepeatCount;
            if (rep < LDG_IO_CONSOLE_REPEAT_MIN) { rep = LDG_IO_CONSOLE_REPEAT_MIN; }

            if (LDG_UNLIKELY(*out_len == 0 && seq_len > cap)) { return LDG_ERR_FULL; }

            if ((uint64_t)rep * seq_len > cap - *out_len && *out_len > 0)
            {
                full = LDG_TRUTH_TRUE;

                break;
            }

            while (rep > 0 && seq_len <= cap - *out_len)
            {
                for (pos = 0; pos < seq_len; pos++) { dst[*out_len + pos] = (uint8_t)seq[pos]; }

                *out_len += seq_len;
                rep--;
            }
        }

        if (idx == 0) { return LDG_ERR_AOK; }

        if (LDG_UNLIKELY(!ReadConsoleInputW(in, rec, idx, &drained))) { return LDG_ERR_IO_RD; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_console_wr(const uint8_t *data, uint64_t len)
{
    HANDLE out = 0x0;
    DWORD chunk = 0;
    DWORD written = 0;
    uint64_t done = 0;

    if (LDG_UNLIKELY(!data)) { return LDG_ERR_FUNC_ARG_NULL; }

    out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (LDG_UNLIKELY(out == INVALID_HANDLE_VALUE || !out)) { return LDG_ERR_IO_WR; }

    while (done < len)
    {
        chunk = (DWORD)((len - done) > (uint64_t)MAXDWORD ? (uint64_t)MAXDWORD : (len - done));

        if (LDG_UNLIKELY(!WriteFile(out, data + done, chunk, &written, 0x0))) { return LDG_ERR_IO_WR; }

        if (written == 0) { return LDG_ERR_IO_WR; }

        done += (uint64_t)written;
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_console_byte_wr(uint8_t byte)
{
    HANDLE out = 0x0;
    DWORD written = 0;

    out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (LDG_UNLIKELY(out == INVALID_HANDLE_VALUE || !out)) { return LDG_ERR_IO_WR; }

    while (written == 0) { if (LDG_UNLIKELY(!WriteFile(out, &byte, (DWORD)sizeof(byte), &written, 0x0))) { return LDG_ERR_IO_WR; } }

    return LDG_ERR_AOK;
}

uint8_t ldg_io_console_rd_ready_is(void)
{
    INPUT_RECORD rec[LDG_IO_CONSOLE_REC_CAP] = LDG_ARR_ZERO_INIT;
    HANDLE in = 0x0;
    DWORD seen = 0;
    DWORD idx = 0;
    uint8_t native = LDG_TRUTH_FALSE;

    if (console_held_get()->cunt > 0) { return LDG_TRUTH_TRUE; }

    in = GetStdHandle(STD_INPUT_HANDLE);
    if (LDG_UNLIKELY(in == INVALID_HANDLE_VALUE || !in)) { return LDG_ERR_IO_RD; }

    native = console_native_is(in);

    if (!PeekConsoleInputW(in, rec, LDG_IO_CONSOLE_REC_CAP, &seen)) { return LDG_ERR_IO_RD; }

    for (idx = 0; idx < seen; idx++)
    {
        if (native == LDG_TRUTH_TRUE && rec[idx].EventType == KEY_EVENT) { return LDG_TRUTH_TRUE; }

        if (native == LDG_TRUTH_FALSE && console_rec_ch_is(&rec[idx]) == LDG_TRUTH_TRUE) { return LDG_TRUTH_TRUE; }
    }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_io_console_byte_rd(uint8_t *out)
{
    uint64_t got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    ret = console_in_rd(out, (uint64_t)sizeof(*out), &got);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (got == 0) { return LDG_ERR_WOULD_BLOCK; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_console_rd(uint8_t *dst, uint64_t cap, uint64_t *out_len)
{
    HANDLE in = 0x0;

    if (LDG_UNLIKELY(!dst || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (cap == 0) { return LDG_ERR_AOK; }

    in = GetStdHandle(STD_INPUT_HANDLE);
    if (LDG_UNLIKELY(in == INVALID_HANDLE_VALUE || !in)) { return LDG_ERR_CLOSED; }

    if (console_held_get()->cunt == 0 && console_native_is(in) == LDG_TRUTH_TRUE) { return console_native_rd(in, dst, cap, out_len); }

    return console_in_rd(dst, cap, out_len);
}

typedef struct ldg_io_console_mode
{
    DWORD saved;
    uint8_t raw;
    uint8_t pudding[3];
} ldg_io_console_mode_t;

static ldg_io_console_mode_t* console_mode_get(void)
{
    static ldg_io_console_mode_t mode = LDG_STRUCT_ZERO_INIT;

    return &mode;
}

uint32_t ldg_io_console_raw_set(uint8_t en)
{
    ldg_io_console_mode_t *mode = console_mode_get();
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);

    if (LDG_UNLIKELY(in == INVALID_HANDLE_VALUE || !in)) { return LDG_ERR_CLOSED; }

    if (en == LDG_TRUTH_TRUE)
    {
        if (mode->raw == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

        if (!GetConsoleMode(in, &mode->saved)) { return LDG_ERR_UNSUPPORTED; }

        if (!SetConsoleMode(in, mode->saved & ~(DWORD)(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT))) { return LDG_ERR_DENIED; }

        mode->raw = LDG_TRUTH_TRUE;

        return LDG_ERR_AOK;
    }

    if (mode->raw != LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    if (!SetConsoleMode(in, mode->saved)) { return LDG_ERR_DENIED; }

    mode->raw = LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

static uint32_t console_vk_kind_get(WORD vk)
{
    if (vk >= VK_F1 && vk <= VK_F12) { return (uint32_t)LDG_IO_CONSOLE_KEY_F1 + (uint32_t)(vk - VK_F1); }

    switch (vk)
    {
        case VK_ESCAPE:
            return LDG_IO_CONSOLE_KEY_ESC;
        case VK_RETURN:
            return LDG_IO_CONSOLE_KEY_ENTER;
        case VK_TAB:
            return LDG_IO_CONSOLE_KEY_TAB;
        case VK_BACK:
            return LDG_IO_CONSOLE_KEY_BACKSPACE;
        case VK_UP:
            return LDG_IO_CONSOLE_KEY_UP;
        case VK_DOWN:
            return LDG_IO_CONSOLE_KEY_DOWN;
        case VK_LEFT:
            return LDG_IO_CONSOLE_KEY_LHS;
        case VK_RIGHT:
            return LDG_IO_CONSOLE_KEY_RHS;
        case VK_HOME:
            return LDG_IO_CONSOLE_KEY_HOME;
        case VK_END:
            return LDG_IO_CONSOLE_KEY_END;
        case VK_PRIOR:
            return LDG_IO_CONSOLE_KEY_PAGE_UP;
        case VK_NEXT:
            return LDG_IO_CONSOLE_KEY_PAGE_DOWN;
        case VK_INSERT:
            return LDG_IO_CONSOLE_KEY_INSERT;
        case VK_DELETE:
            return LDG_IO_CONSOLE_KEY_DELETE;
        default:
            return LDG_IO_CONSOLE_KEY_KIND_CUNT;
    }
}

uint32_t ldg_io_console_key_rd(ldg_io_console_key_t *out)
{
    INPUT_RECORD rec = LDG_STRUCT_ZERO_INIT;
    HANDLE in = 0x0;
    DWORD pending = 0;
    DWORD got = 0;
    DWORD state = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->kind = LDG_IO_CONSOLE_KEY_OTHER;
    out->codepoint = LDG_IO_CONSOLE_CH_NONE;
    out->mods = 0;
    out->raw = 0;
    out->down = LDG_TRUTH_FALSE;

    in = GetStdHandle(STD_INPUT_HANDLE);
    if (LDG_UNLIKELY(in == INVALID_HANDLE_VALUE || !in)) { return LDG_ERR_CLOSED; }

    for (idx = 0; idx < LDG_IO_CONSOLE_REC_CAP; idx++)
    {
        if (!GetNumberOfConsoleInputEvents(in, &pending)) { return LDG_ERR_IO_RD; }

        if (pending == 0) { return LDG_ERR_EMPTY; }

        if (!ReadConsoleInputW(in, &rec, LDG_IO_CONSOLE_STEP, &got)) { return LDG_ERR_IO_RD; }

        if (got == 0) { return LDG_ERR_EMPTY; }

        if (rec.EventType != KEY_EVENT) { continue; }

        state = rec.Event.KeyEvent.dwControlKeyState;
        out->raw = rec.Event.KeyEvent.wVirtualKeyCode;
        out->down = rec.Event.KeyEvent.bKeyDown ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
        out->codepoint = rec.Event.KeyEvent.uChar.UnicodeChar;

        if ((state & SHIFT_PRESSED) != 0) { out->mods |= LDG_IO_CONSOLE_MOD_SHIFT; }

        if ((state & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0) { out->mods |= LDG_IO_CONSOLE_MOD_ALT; }

        if ((state & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0) { out->mods |= LDG_IO_CONSOLE_MOD_CTRL; }

        out->kind = console_vk_kind_get(rec.Event.KeyEvent.wVirtualKeyCode);
        if (out->kind == LDG_IO_CONSOLE_KEY_KIND_CUNT) { out->kind = (out->codepoint != LDG_IO_CONSOLE_CH_NONE) ? LDG_IO_CONSOLE_KEY_CHAR : LDG_IO_CONSOLE_KEY_OTHER; }

        if (out->kind != LDG_IO_CONSOLE_KEY_CHAR) { out->codepoint = LDG_IO_CONSOLE_CH_NONE; }

        return LDG_ERR_AOK;
    }

    return LDG_ERR_EMPTY;
}
