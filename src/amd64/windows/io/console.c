#include <stdint.h>
#include <windows.h>

#include <dangling/io/console.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

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
    HANDLE in = 0x0;
    DWORD pending = 0;

    in = GetStdHandle(STD_INPUT_HANDLE);
    if (LDG_UNLIKELY(in == INVALID_HANDLE_VALUE || !in)) { return LDG_ERR_IO_RD; }

    if (!GetNumberOfConsoleInputEvents(in, &pending)) { return LDG_ERR_IO_RD; }

    if (pending == 0) { return LDG_TRUTH_FALSE; }

    return LDG_TRUTH_TRUE;
}

uint32_t ldg_io_console_byte_rd(uint8_t *out)
{
    HANDLE in = 0x0;
    DWORD got = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    in = GetStdHandle(STD_INPUT_HANDLE);
    if (LDG_UNLIKELY(in == INVALID_HANDLE_VALUE || !in)) { return LDG_ERR_IO_RD; }

    if (LDG_UNLIKELY(!ReadFile(in, out, sizeof(*out), &got, 0x0))) { return LDG_ERR_IO_RD; }

    if (got == 0) { return LDG_ERR_EOF; }

    return LDG_ERR_AOK;
}
