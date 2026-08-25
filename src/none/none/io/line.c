#include <stdint.h>

#include <dangling/io/console.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>

#define LDG_IO_CONSOLE_LINE_END '\n'
#define LDG_IO_CONSOLE_CARRIAGE_RET '\r'

uint32_t ldg_io_console_line_rd(char *dst, uint64_t cap, uint64_t *out_len)
{
    uint64_t len = 0;
    uint8_t byte = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_len = 0;

    if (LDG_UNLIKELY(!dst)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    while (LDG_TRUTH_TRUE)
    {
        ret = ldg_io_console_byte_rd(&byte);

        if (ret == LDG_ERR_WOULD_BLOCK) { continue; }

        if (ret == LDG_ERR_EOF)
        {
            dst[len] = LDG_STR_TERM;
            *out_len = len;

            if (len == 0) { return LDG_ERR_EOF; }

            return LDG_ERR_AOK;
        }

        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (byte == LDG_IO_CONSOLE_CARRIAGE_RET) { continue; }

        if (byte == LDG_IO_CONSOLE_LINE_END) { break; }

        if ((len + LDG_STR_TERM_SIZE) >= cap)
        {
            dst[len] = LDG_STR_TERM;
            *out_len = len;

            return LDG_ERR_STR_TRUNC;
        }

        dst[len] = (char)byte;
        len++;
    }

    dst[len] = LDG_STR_TERM;
    *out_len = len;

    return LDG_ERR_AOK;
}
