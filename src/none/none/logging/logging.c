#include <stdint.h>
#include <stdarg.h>

#include <dangling/logging/logging.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/console.h>
#include <dangling/io/file.h>
#include <dangling/mem/cpy.h>
#include <dangling/str/fmt.h>
#include <dangling/str/str.h>
#include <dangling/time/time.h>

#define LDG_LOGGING_LVL_NAME_CAP 6
#define LDG_LOGGING_FILE_ACCESS 0x1A4U
#define LDG_LOGGING_PREFIX_FMT "%u64 %s "
#define LDG_LOGGING_TRUNC_MARKER " (trunc)"
#define LDG_LOGGING_TRUNC_MARKER_LEN 8
#define LDG_LOGGING_LINE_END '\n'
#define LDG_LOGGING_LINE_END_SIZE 1

static void logging_ascii_fold(char *text, uint64_t len)
{
    uint64_t idx = 0;

    for (idx = 0; idx < len; idx++) { if ((uint8_t)text[idx] < LDG_ASCII_PRINTABLE_MIN || (uint8_t)text[idx] > LDG_ASCII_PRINTABLE_MAX) { text[idx] = LDG_ASCII_NONPRINTABLE_CHAR; } }
}

static uint32_t logging_file_append(const char *path, const char *line, uint64_t len)
{
    ldg_io_file_t *file = 0x0;
    uint64_t wr_cunt = 0;
    uint32_t ret = 0;

    ret = ldg_io_file_open(path, LDG_IO_WR_ONLY | LDG_IO_CREATE | LDG_IO_APPEND, LDG_LOGGING_FILE_ACCESS, &file);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_io_file_wr(file, line, len, &wr_cunt);
    if (ret == LDG_ERR_AOK && LDG_UNLIKELY(wr_cunt != len)) { ret = LDG_ERR_IO_WR; }

    if (LDG_UNLIKELY(ldg_io_file_close(file) != LDG_ERR_AOK) && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

uint32_t ldg_logging_variable_line_wr(const char *path, uint32_t lvl, const char *fmt, va_list args)
{
    const char lvl_name[LDG_LOGGING_LVL_CUNT][LDG_LOGGING_LVL_NAME_CAP] = { "ERR", "WARN", "INFO", "DEBUG" };
    char line[LDG_LOGGING_LINE_CAP + LDG_STR_TERM_SIZE] = LDG_ARR_ZERO_INIT;
    uint64_t epoch_ms = 0;
    uint64_t prefix_len = 0;
    uint64_t msg_len = 0;
    uint64_t body_cap = 0;
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !fmt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(lvl >= LDG_LOGGING_LVL_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    epoch_ms = ldg_time_epoch_ms_get();
    body_cap = (uint64_t)LDG_LOGGING_LINE_CAP - LDG_LOGGING_LINE_END_SIZE;

    ret = ldg_str_fmt(line, body_cap + LDG_STR_TERM_SIZE, &prefix_len, LDG_LOGGING_PREFIX_FMT, epoch_ms, lvl_name[lvl]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_str_variable_fmt(line + prefix_len, body_cap + LDG_STR_TERM_SIZE - prefix_len, &msg_len, fmt, args);
    if (ret == LDG_ERR_STR_TRUNC)
    {
        len = body_cap;
        logging_ascii_fold(line + prefix_len, len - prefix_len);

        ret = ldg_mem_cpy(line + body_cap - LDG_LOGGING_TRUNC_MARKER_LEN, LDG_LOGGING_TRUNC_MARKER, LDG_LOGGING_TRUNC_MARKER_LEN);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    }
    else if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }
    else
    {
        len = prefix_len + msg_len;
        logging_ascii_fold(line + prefix_len, msg_len);
    }

    line[len] = LDG_LOGGING_LINE_END;
    len += LDG_LOGGING_LINE_END_SIZE;

    if (path == LDG_LOGGING_CON) { return ldg_io_console_wr((const uint8_t *)line, len); }

    return logging_file_append(path, line, len);
}

uint32_t ldg_logging_line_wr(const char *path, uint32_t lvl, const char *fmt, ...)
{
    va_list args = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!path || !fmt)) { return LDG_ERR_FUNC_ARG_NULL; }

    va_start(args, fmt);
    ret = ldg_logging_variable_line_wr(path, lvl, fmt, args);
    va_end(args);

    return ret;
}
