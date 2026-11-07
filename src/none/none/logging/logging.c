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
#define LDG_LOGGING_ESC_LEAD_IDX 0
#define LDG_LOGGING_ESC_KIND_IDX 1
#define LDG_LOGGING_ESC_LITERAL_LEN 1
#define LDG_LOGGING_ESC_SHORT_LEN 2
#define LDG_LOGGING_ESC_HEX_LEN 4
#define LDG_LOGGING_ESC_LF 'n'
#define LDG_LOGGING_ESC_CR 'r'
#define LDG_LOGGING_ESC_TAB 't'
#define LDG_LOGGING_ESC_HEX 'x'

static uint32_t logging_esc_seq_get(uint8_t byte, char *seq, uint64_t *seq_len)
{
    *seq_len = LDG_LOGGING_ESC_SHORT_LEN;
    seq[LDG_LOGGING_ESC_LEAD_IDX] = LDG_ASCII_BSLASH;

    if (byte == LDG_ASCII_LF) { seq[LDG_LOGGING_ESC_KIND_IDX] = LDG_LOGGING_ESC_LF; return LDG_ERR_AOK; }

    if (byte == LDG_ASCII_CR) { seq[LDG_LOGGING_ESC_KIND_IDX] = LDG_LOGGING_ESC_CR; return LDG_ERR_AOK; }

    if (byte == LDG_ASCII_TAB) { seq[LDG_LOGGING_ESC_KIND_IDX] = LDG_LOGGING_ESC_TAB; return LDG_ERR_AOK; }

    if (byte == LDG_ASCII_BSLASH) { seq[LDG_LOGGING_ESC_KIND_IDX] = LDG_ASCII_BSLASH; return LDG_ERR_AOK; }

    if (byte >= LDG_ASCII_PRINTABLE_MIN && byte <= LDG_ASCII_PRINTABLE_MAX)
    {
        *seq_len = LDG_LOGGING_ESC_LITERAL_LEN;
        seq[LDG_LOGGING_ESC_LEAD_IDX] = (char)byte;
        return LDG_ERR_AOK;
    }

    *seq_len = LDG_LOGGING_ESC_HEX_LEN;
    seq[LDG_LOGGING_ESC_KIND_IDX] = LDG_LOGGING_ESC_HEX;

    return ldg_byte_to_hex(byte, seq + LDG_LOGGING_ESC_SHORT_LEN);
}

static uint32_t logging_esc(char *dst, uint64_t cap, const char *src, uint64_t src_len, uint8_t src_truncd, uint64_t *out_len)
{
    char seq[LDG_LOGGING_ESC_HEX_LEN + LDG_STR_TERM_SIZE] = LDG_ARR_ZERO_INIT;
    uint64_t mark_cap = 0;
    uint64_t seq_len = 0;
    uint64_t fit = 0;
    uint64_t pos = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    *out_len = 0;

    if (cap > LDG_LOGGING_TRUNC_MARKER_LEN) { mark_cap = cap - LDG_LOGGING_TRUNC_MARKER_LEN; }

    for (idx = 0; idx < src_len; idx++)
    {
        ret = logging_esc_seq_get((uint8_t)src[idx], seq, &seq_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        if (pos + seq_len > cap)
        {
            *out_len = fit;
            return LDG_ERR_STR_TRUNC;
        }

        ret = ldg_mem_cpy(dst + pos, seq, seq_len);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        pos += seq_len;
        if (pos <= mark_cap) { fit = pos; }
    }

    if (src_truncd == LDG_TRUTH_TRUE)
    {
        *out_len = fit;
        return LDG_ERR_STR_TRUNC;
    }

    *out_len = pos;

    return LDG_ERR_AOK;
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
    char raw[LDG_LOGGING_LINE_CAP + LDG_STR_TERM_SIZE] = LDG_ARR_ZERO_INIT;
    uint64_t epoch_ms = 0;
    uint64_t prefix_len = 0;
    uint64_t raw_len = 0;
    uint64_t msg_len = 0;
    uint64_t msg_cap = 0;
    uint64_t body_cap = 0;
    uint64_t len = 0;
    uint32_t ret = 0;
    uint8_t raw_truncd = 0;

    if (LDG_UNLIKELY(!path || !fmt)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(lvl >= LDG_LOGGING_LVL_CUNT)) { return LDG_ERR_FUNC_ARG_INVALID; }

    epoch_ms = ldg_time_epoch_ms_get();
    body_cap = (uint64_t)LDG_LOGGING_LINE_CAP - LDG_LOGGING_LINE_END_SIZE;

    ret = ldg_str_fmt(line, body_cap + LDG_STR_TERM_SIZE, &prefix_len, LDG_LOGGING_PREFIX_FMT, epoch_ms, lvl_name[lvl]);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    msg_cap = body_cap - prefix_len;

    ret = ldg_str_variable_fmt(raw, msg_cap + LDG_STR_TERM_SIZE, &raw_len, fmt, args);
    if (ret == LDG_ERR_STR_TRUNC)
    {
        raw_len = msg_cap;
        raw_truncd = LDG_TRUTH_TRUE;
    }
    else if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = logging_esc(line + prefix_len, msg_cap, raw, raw_len, raw_truncd, &msg_len);
    if (ret == LDG_ERR_STR_TRUNC)
    {
        ret = ldg_mem_cpy(line + prefix_len + msg_len, LDG_LOGGING_TRUNC_MARKER, LDG_LOGGING_TRUNC_MARKER_LEN);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        msg_len += LDG_LOGGING_TRUNC_MARKER_LEN;
    }
    else if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    len = prefix_len + msg_len;
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
