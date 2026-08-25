#ifndef LDG_IO_PTY_H
#define LDG_IO_PTY_H

#include <stdint.h>

#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_IO_PTY_HANDLE_NONE UINT64_MAX
#define LDG_IO_PTY_COL_DEFAULT 80
#define LDG_IO_PTY_ROW_DEFAULT 24
#define LDG_IO_PTY_COL_MAX 512
#define LDG_IO_PTY_ROW_MAX 256

typedef struct ldg_io_pty_conf
{
    const char *shell;
    const char *cwd;
    const char *term_name;
    uint32_t col_cunt;
    uint32_t row_cunt;
} ldg_io_pty_conf_t;

typedef struct ldg_io_pty
{
    uint64_t rd_handle;
    uint64_t wr_handle;
    uint64_t ctl_handle;
    uint64_t proc_handle;
    uint32_t pid;
    uint32_t exit_code;
    uint8_t open;
    uint8_t alive;
    uint8_t passthrough;
    uint8_t pudding[5];
} ldg_io_pty_t;

LDG_EXPORT uint32_t ldg_io_pty_conf_default_get(ldg_io_pty_conf_t *out);
LDG_EXPORT uint32_t ldg_io_pty_open(const ldg_io_pty_conf_t *conf, ldg_io_pty_t *out);
LDG_EXPORT uint32_t ldg_io_pty_rd(ldg_io_pty_t *pty, ldg_byte_t *buff, uint64_t cap, uint64_t *out_got);
LDG_EXPORT uint32_t ldg_io_pty_wr(ldg_io_pty_t *pty, const ldg_byte_t *buff, uint64_t len, uint64_t *out_sent);
LDG_EXPORT uint32_t ldg_io_pty_resize(ldg_io_pty_t *pty, uint32_t col_cunt, uint32_t row_cunt);
LDG_EXPORT uint32_t ldg_io_pty_exit_code_get(ldg_io_pty_t *pty, uint32_t *out_code);
LDG_EXPORT uint32_t ldg_io_pty_close(ldg_io_pty_t *pty);
LDG_EXPORT uint8_t ldg_io_pty_alive_is(ldg_io_pty_t *pty);

#endif
