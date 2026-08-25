#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include <dangling/io/console.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_IO_CONSOLE_OUT_FD 1
#define LDG_IO_CONSOLE_IN_FD 0
#define LDG_IO_CONSOLE_WATCH_CUNT 1
#define LDG_IO_CONSOLE_POLL_NO_WAIT 0

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
