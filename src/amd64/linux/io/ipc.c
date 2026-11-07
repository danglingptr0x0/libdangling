#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>

#include <dangling/io/ipc.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>

#define LDG_IO_IPC_BACKLOG 16
#define LDG_IO_IPC_ABSTRACT_LEAD 1
#define LDG_IO_IPC_PROTO_DEFAULT 0
#define LDG_IO_IPC_RECV_NO_FLAG 0
#define LDG_IO_IPC_SYSCALL_FAIL (-1)

static uint32_t io_ipc_addr_build(const char *name, struct sockaddr_un *addr, socklen_t *out_len)
{
    uint64_t len = 0;

    if (LDG_UNLIKELY(ldg_str_len_get(name, LDG_IO_IPC_NAME_MAX, &len) != LDG_ERR_AOK)) { return LDG_ERR_STR_TRUNC; }

    if (LDG_UNLIKELY(len == 0)) { return LDG_ERR_STR_EMPTY; }

    if (LDG_UNLIKELY(len + LDG_IO_IPC_ABSTRACT_LEAD > sizeof(addr->sun_path))) { return LDG_ERR_STR_TRUNC; }

    if (LDG_UNLIKELY(ldg_mem_secure_zero(addr, sizeof(struct sockaddr_un)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    addr->sun_family = AF_UNIX;

    if (LDG_UNLIKELY(ldg_mem_secure_cpy(addr->sun_path + LDG_IO_IPC_ABSTRACT_LEAD, name, len) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    *out_len = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + LDG_IO_IPC_ABSTRACT_LEAD + len);

    return LDG_ERR_AOK;
}

static uint32_t io_ipc_rst(ldg_io_ipc_t *ipc)
{
    if (LDG_UNLIKELY(ldg_mem_secure_zero(ipc, sizeof(ldg_io_ipc_t)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

    ipc->handle = LDG_IO_IPC_HANDLE_NONE;

    return LDG_ERR_AOK;
}

static uint32_t io_ipc_fd_flags_set(int32_t fd)
{
    int32_t flags = 0;

    flags = fcntl(fd, F_GETFL);
    if (LDG_UNLIKELY(flags == LDG_IO_IPC_SYSCALL_FAIL)) { return LDG_ERR_IO_OPEN; }

    if (LDG_UNLIKELY(fcntl(fd, F_SETFL, flags | O_NONBLOCK) == LDG_IO_IPC_SYSCALL_FAIL)) { return LDG_ERR_IO_OPEN; }

    if (LDG_UNLIKELY(fcntl(fd, F_SETFD, FD_CLOEXEC) == LDG_IO_IPC_SYSCALL_FAIL)) { return LDG_ERR_IO_OPEN; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_ipc_listen(const char *name, ldg_io_ipc_t *out)
{
    struct sockaddr_un addr = LDG_STRUCT_ZERO_INIT;
    socklen_t addr_len = 0;
    int32_t fd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!name || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = io_ipc_rst(out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = io_ipc_addr_build(name, &addr, &addr_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fd = (int32_t)socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, LDG_IO_IPC_PROTO_DEFAULT);
    if (LDG_UNLIKELY(fd < 0)) { return LDG_ERR_IO_OPEN; }

    if (LDG_UNLIKELY(bind(fd, (const struct sockaddr *)(const void *)&addr, addr_len) != 0))
    {
        ret = (errno == EADDRINUSE) ? LDG_ERR_EXISTS : LDG_ERR_IO_OPEN;
        if (close(fd) != 0) { return LDG_ERR_IO_CLOSE; }

        return ret;
    }

    if (LDG_UNLIKELY(listen(fd, LDG_IO_IPC_BACKLOG) != 0))
    {
        if (close(fd) != 0) { return LDG_ERR_IO_CLOSE; }

        return LDG_ERR_IO_OPEN;
    }

    out->handle = (uint64_t)fd;
    out->listening = LDG_TRUTH_TRUE;
    out->live = LDG_TRUTH_TRUE;

    return ldg_str_rbrcpy(out->name, name, LDG_IO_IPC_NAME_MAX);
}

uint32_t ldg_io_ipc_accept(ldg_io_ipc_t *listener, ldg_io_ipc_t *out)
{
    int32_t fd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!listener || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = io_ipc_rst(out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(listener->live != LDG_TRUTH_TRUE || listener->listening != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    fd = (int32_t)accept((int32_t)listener->handle, 0x0, 0x0);
    if (fd < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) { return LDG_ERR_WOULD_BLOCK; }

        return LDG_ERR_IO_OPEN;
    }

    ret = io_ipc_fd_flags_set(fd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
        if (close(fd) != 0) { return LDG_ERR_IO_CLOSE; }

        return ret;
    }

    out->handle = (uint64_t)fd;
    out->live = LDG_TRUTH_TRUE;

    return ldg_str_rbrcpy(out->name, listener->name, LDG_IO_IPC_NAME_MAX);
}

uint32_t ldg_io_ipc_connect(const char *name, ldg_io_ipc_t *out)
{
    struct sockaddr_un addr = LDG_STRUCT_ZERO_INIT;
    socklen_t addr_len = 0;
    int32_t fd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!name || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = io_ipc_rst(out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = io_ipc_addr_build(name, &addr, &addr_len);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fd = (int32_t)socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, LDG_IO_IPC_PROTO_DEFAULT);
    if (LDG_UNLIKELY(fd < 0)) { return LDG_ERR_IO_OPEN; }

    if (connect(fd, (const struct sockaddr *)(const void *)&addr, addr_len) != 0)
    {
        if (errno == ECONNREFUSED || errno == ENOENT) { ret = LDG_ERR_NOT_FOUND; }
        else if (errno == EAGAIN) { ret = LDG_ERR_BUSY; }
        else { ret = LDG_ERR_IO_OPEN; }

        if (close(fd) != 0) { return LDG_ERR_IO_CLOSE; }

        return ret;
    }

    out->handle = (uint64_t)fd;
    out->live = LDG_TRUTH_TRUE;

    return ldg_str_rbrcpy(out->name, name, LDG_IO_IPC_NAME_MAX);
}

uint32_t ldg_io_ipc_peek(ldg_io_ipc_t *ipc, uint64_t *out_avail)
{
    int32_t avail = 0;

    if (LDG_UNLIKELY(!ipc || !out_avail)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_avail = 0;

    if (LDG_UNLIKELY(ipc->live != LDG_TRUTH_TRUE || ipc->listening == LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(ioctl((int32_t)ipc->handle, FIONREAD, &avail) != 0)) { return LDG_ERR_IO_RD; }

    if (avail > 0) { *out_avail = (uint64_t)avail; }

    return LDG_ERR_AOK;
}

uint32_t ldg_io_ipc_rd(ldg_io_ipc_t *ipc, ldg_byte_t *dst, uint64_t cap, uint64_t *out_got)
{
    int64_t got = 0;

    if (LDG_UNLIKELY(!ipc || !dst || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(ipc->live != LDG_TRUTH_TRUE || ipc->listening == LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (cap == 0) { return LDG_ERR_AOK; }

    got = (int64_t)recv((int32_t)ipc->handle, dst, cap, LDG_IO_IPC_RECV_NO_FLAG);
    if (got < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) { return LDG_ERR_AOK; }

        if (errno == ECONNRESET)
        {
            ipc->live = LDG_TRUTH_FALSE;
            return LDG_ERR_CLOSED;
        }

        return LDG_ERR_IO_RD;
    }

    if (got == 0)
    {
        ipc->live = LDG_TRUTH_FALSE;
        return LDG_ERR_CLOSED;
    }

    *out_got = (uint64_t)got;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_ipc_wr(ldg_io_ipc_t *ipc, const ldg_byte_t *src, uint64_t len, uint64_t *out_sent)
{
    int64_t sent = 0;

    if (LDG_UNLIKELY(!ipc || !src || !out_sent)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_sent = 0;

    if (LDG_UNLIKELY(ipc->live != LDG_TRUTH_TRUE || ipc->listening == LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    if (len == 0) { return LDG_ERR_AOK; }

    sent = (int64_t)send((int32_t)ipc->handle, src, len, MSG_NOSIGNAL);
    if (sent < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) { return LDG_ERR_AOK; }

        if (errno == EPIPE || errno == ECONNRESET)
        {
            ipc->live = LDG_TRUTH_FALSE;
            return LDG_ERR_CLOSED;
        }

        return LDG_ERR_IO_WR;
    }

    *out_sent = (uint64_t)sent;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_ipc_close(ldg_io_ipc_t *ipc)
{
    uint32_t ret = LDG_ERR_AOK;

    if (LDG_UNLIKELY(!ipc)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ipc->handle == LDG_IO_IPC_HANDLE_NONE) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(close((int32_t)ipc->handle) != 0)) { ret = LDG_ERR_IO_CLOSE; }

    ipc->handle = LDG_IO_IPC_HANDLE_NONE;
    ipc->live = LDG_TRUTH_FALSE;
    ipc->listening = LDG_TRUTH_FALSE;

    return ret;
}
