#ifndef LDG_IO_IPC_H
#define LDG_IO_IPC_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_IO_IPC_NAME_MAX 96
#define LDG_IO_IPC_HANDLE_NONE UINT64_MAX
#define LDG_IO_IPC_TIMEOUT_NONE UINT64_MAX

typedef struct ldg_io_ipc
{
    uint64_t handle;
    char name[LDG_IO_IPC_NAME_MAX];
    uint8_t listening;
    uint8_t live;
    uint8_t cancelled;
    uint8_t pudding[5];
} ldg_io_ipc_t;

typedef struct ldg_io_ipc_name
{
    char text[LDG_IO_IPC_NAME_MAX];
} ldg_io_ipc_name_t;

LDG_EXPORT uint32_t ldg_io_ipc_listen(const char *name, ldg_io_ipc_t *out);
LDG_EXPORT uint32_t ldg_io_ipc_accept(ldg_io_ipc_t *listener, ldg_io_ipc_t *out);
LDG_EXPORT uint32_t ldg_io_ipc_connect(const char *name, ldg_io_ipc_t *out);
LDG_EXPORT uint32_t ldg_io_ipc_peek(ldg_io_ipc_t *ipc, uint64_t *out_avail);
LDG_EXPORT uint32_t ldg_io_ipc_rd(ldg_io_ipc_t *ipc, ldg_byte_t *dst, uint64_t cap, uint64_t *out_got);
LDG_EXPORT uint32_t ldg_io_ipc_wr(ldg_io_ipc_t *ipc, const ldg_byte_t *src, uint64_t len, uint64_t *out_sent);
LDG_EXPORT uint32_t ldg_io_ipc_close(ldg_io_ipc_t *ipc);
LDG_EXPORT uint32_t ldg_io_ipc_accept_timed(ldg_io_ipc_t *listener, uint64_t timeout_ms, ldg_io_ipc_t *out);
LDG_EXPORT uint32_t ldg_io_ipc_accept_cancel(ldg_io_ipc_t *listener);
LDG_EXPORT uint32_t ldg_io_ipc_rd_timed(ldg_io_ipc_t *ipc, ldg_byte_t *dst, uint64_t cap, uint64_t timeout_ms, uint64_t *out_got);
LDG_EXPORT uint32_t ldg_io_ipc_list(const char *prefix, ldg_io_ipc_name_t *out, uint64_t cap, uint64_t *out_cunt);

#endif
