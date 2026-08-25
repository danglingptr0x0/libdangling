#ifndef LDG_IO_CONSOLE_H
#define LDG_IO_CONSOLE_H

#include <stdint.h>
#include <dangling/core/macros.h>

LDG_EXPORT uint32_t ldg_io_console_byte_wr(uint8_t byte);
LDG_EXPORT uint32_t ldg_io_console_wr(const uint8_t *data, uint64_t len);
LDG_EXPORT uint32_t ldg_io_console_byte_rd(uint8_t *out);
LDG_EXPORT uint8_t ldg_io_console_rd_ready_is(void);
LDG_EXPORT uint32_t ldg_io_console_line_rd(char *dst, uint64_t cap, uint64_t *out_len);

#endif
