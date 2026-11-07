#ifndef LDG_IO_CONSOLE_H
#define LDG_IO_CONSOLE_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_IO_CONSOLE_MOD_SHIFT 0x1U
#define LDG_IO_CONSOLE_MOD_ALT 0x2U
#define LDG_IO_CONSOLE_MOD_CTRL 0x4U

typedef enum ldg_io_console_key_kind
{
    LDG_IO_CONSOLE_KEY_CHAR = 0,
    LDG_IO_CONSOLE_KEY_ESC,
    LDG_IO_CONSOLE_KEY_ENTER,
    LDG_IO_CONSOLE_KEY_TAB,
    LDG_IO_CONSOLE_KEY_BACKSPACE,
    LDG_IO_CONSOLE_KEY_UP,
    LDG_IO_CONSOLE_KEY_DOWN,
    LDG_IO_CONSOLE_KEY_LHS,
    LDG_IO_CONSOLE_KEY_RHS,
    LDG_IO_CONSOLE_KEY_HOME,
    LDG_IO_CONSOLE_KEY_END,
    LDG_IO_CONSOLE_KEY_PAGE_UP,
    LDG_IO_CONSOLE_KEY_PAGE_DOWN,
    LDG_IO_CONSOLE_KEY_INSERT,
    LDG_IO_CONSOLE_KEY_DELETE,
    LDG_IO_CONSOLE_KEY_F1,
    LDG_IO_CONSOLE_KEY_F2,
    LDG_IO_CONSOLE_KEY_F3,
    LDG_IO_CONSOLE_KEY_F4,
    LDG_IO_CONSOLE_KEY_F5,
    LDG_IO_CONSOLE_KEY_F6,
    LDG_IO_CONSOLE_KEY_F7,
    LDG_IO_CONSOLE_KEY_F8,
    LDG_IO_CONSOLE_KEY_F9,
    LDG_IO_CONSOLE_KEY_F10,
    LDG_IO_CONSOLE_KEY_F11,
    LDG_IO_CONSOLE_KEY_F12,
    LDG_IO_CONSOLE_KEY_OTHER,
    LDG_IO_CONSOLE_KEY_KIND_CUNT
} ldg_io_console_key_kind_t;

typedef struct ldg_io_console_key
{
    uint32_t kind;
    uint32_t codepoint;
    uint32_t mods;
    uint32_t raw;
    uint8_t down;
    uint8_t pudding[3];
} ldg_io_console_key_t;

LDG_EXPORT uint32_t ldg_io_console_raw_set(uint8_t en);
LDG_EXPORT uint32_t ldg_io_console_key_rd(ldg_io_console_key_t *out);
LDG_EXPORT uint32_t ldg_io_console_byte_wr(uint8_t byte);
LDG_EXPORT uint32_t ldg_io_console_wr(const uint8_t *data, uint64_t len);
LDG_EXPORT uint32_t ldg_io_console_byte_rd(uint8_t *out);
LDG_EXPORT uint32_t ldg_io_console_rd(uint8_t *dst, uint64_t cap, uint64_t *out_len);
LDG_EXPORT uint8_t ldg_io_console_rd_ready_is(void);
LDG_EXPORT uint32_t ldg_io_console_line_rd(char *dst, uint64_t cap, uint64_t *out_len);

#endif
