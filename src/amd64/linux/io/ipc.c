#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>

#include <dangling/io/ipc.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/cpy.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>

#define LDG_IO_IPC_BACKLOG 16
#define LDG_IO_IPC_ABSTRACT_LEAD 1
#define LDG_IO_IPC_PROTO_DEFAULT 0
#define LDG_IO_IPC_SYSCALL_FAIL (-1)
#define LDG_IO_IPC_WATCH_CUNT 1
#define LDG_IO_IPC_UID_ROOT 0
#define LDG_IO_IPC_OPT_PEER_CRED 17
#define LDG_IO_IPC_WAIT_SLICE_MS 20
#define LDG_IO_IPC_WAIT_MS_MAX 0x7FFFFFFF
#define LDG_IO_IPC_WAIT_UNBOUNDED (-1)
#define LDG_IO_IPC_LIST_PATH "/proc/net/unix"
#define LDG_IO_IPC_LIST_BLK_SIZE 4096
#define LDG_IO_IPC_LIST_LINE_CAP 256
#define LDG_IO_IPC_LIST_ABSTRACT_MARK '@'
#define LDG_IO_IPC_LIST_MARK_LEN 1
#define LDG_IO_IPC_LIST_LINE_END '\n'

typedef struct ldg_io_ipc_peer
{
    uint32_t pid;
    uint32_t uid;
    uint32_t gid;
} ldg_io_ipc_peer_t;

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

static uint32_t io_ipc_peer_ck(int32_t fd)
{
    ldg_io_ipc_peer_t peer = LDG_STRUCT_ZERO_INIT;
    socklen_t len = sizeof(ldg_io_ipc_peer_t);

    if (LDG_UNLIKELY(getsockopt(fd, SOL_SOCKET, LDG_IO_IPC_OPT_PEER_CRED, &peer, &len) != 0)) { return LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(len != sizeof(ldg_io_ipc_peer_t))) { return LDG_ERR_DENIED; }

    if (LDG_UNLIKELY(peer.uid != (uint32_t)geteuid() && peer.uid != LDG_IO_IPC_UID_ROOT)) { return LDG_ERR_DENIED; }

    return LDG_ERR_AOK;
}

static uint32_t io_ipc_wait(int32_t fd, uint64_t timeout_ms)
{
    struct pollfd watch = LDG_STRUCT_ZERO_INIT;
    int32_t wait_ms = 0;
    int32_t ready = 0;

    wait_ms = (timeout_ms > (uint64_t)LDG_IO_IPC_WAIT_MS_MAX) ? LDG_IO_IPC_WAIT_MS_MAX : (int32_t)timeout_ms;
    if (timeout_ms == LDG_IO_IPC_TIMEOUT_NONE) { wait_ms = LDG_IO_IPC_WAIT_UNBOUNDED; }

    watch.fd = fd;
    watch.events = POLLIN;

    ready = (int32_t)poll(&watch, LDG_IO_IPC_WATCH_CUNT, wait_ms);
    if (ready > 0) { return LDG_ERR_AOK; }

    if (ready == 0 || errno == EINTR) { return LDG_ERR_TIMEOUT; }

    return LDG_ERR_IO_RD;
}

static void io_ipc_list_line_take(const char *line, uint64_t len, const char *prefix, uint64_t prefix_len, ldg_io_ipc_name_t *out, uint64_t cap, uint64_t *cunt)
{
    const char *name = 0x0;
    uint64_t mark = 0;
    uint64_t name_len = 0;
    uint64_t idx = 0;
    uint32_t order = 0;

    if (*cunt >= cap) { return; }

    if (ldg_str_char_find(line, LDG_IO_IPC_LIST_ABSTRACT_MARK, len, &mark) != LDG_ERR_AOK) { return; }

    name = line + mark + LDG_IO_IPC_LIST_MARK_LEN;
    name_len = len - mark - LDG_IO_IPC_LIST_MARK_LEN;

    if (name_len < prefix_len || name_len - prefix_len >= LDG_IO_IPC_NAME_MAX) { return; }

    if (prefix_len > 0 && (ldg_mem_cmp(name, prefix, prefix_len, &order) != LDG_ERR_AOK || order != 0)) { return; }

    name += prefix_len;

    for (idx = 0; idx < *cunt; idx++) { if (ldg_str_cmp(out[idx].text, name, LDG_IO_IPC_NAME_MAX, &order) == LDG_ERR_AOK && order == 0) { return; } }

    if (ldg_str_rbrcpy(out[*cunt].text, name, LDG_IO_IPC_NAME_MAX) != LDG_ERR_AOK) { return; }

    (*cunt)++;
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

    fd = (int32_t)socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, LDG_IO_IPC_PROTO_DEFAULT);
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
    if (ret == LDG_ERR_AOK) { ret = io_ipc_peer_ck(fd); }

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

    fd = (int32_t)socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK | SOCK_CLOEXEC, LDG_IO_IPC_PROTO_DEFAULT);
    if (LDG_UNLIKELY(fd < 0)) { return LDG_ERR_IO_OPEN; }

    if (connect(fd, (const struct sockaddr *)(const void *)&addr, addr_len) != 0)
    {
        if (errno == ECONNREFUSED || errno == ENOENT) { ret = LDG_ERR_NOT_FOUND; }
        else if (errno == EAGAIN) { ret = LDG_ERR_BUSY; }
        else { ret = LDG_ERR_IO_OPEN; }

        if (close(fd) != 0) { return LDG_ERR_IO_CLOSE; }

        return ret;
    }

    ret = io_ipc_peer_ck(fd);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK))
    {
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

    got = (int64_t)recv((int32_t)ipc->handle, dst, cap, MSG_TRUNC);
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

    if ((uint64_t)got > cap)
    {
        *out_got = cap;
        return LDG_ERR_OVERFLOW;
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

uint32_t ldg_io_ipc_accept_timed(ldg_io_ipc_t *listener, uint64_t timeout_ms, ldg_io_ipc_t *out)
{
    uint64_t remaining = timeout_ms;
    uint64_t slice = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!listener || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = io_ipc_rst(out);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(listener->live != LDG_TRUTH_TRUE || listener->listening != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    for (;;)
    {
        if (listener->cancelled == LDG_TRUTH_TRUE)
        {
            listener->cancelled = LDG_TRUTH_FALSE;
            return LDG_ERR_CANCELED;
        }

        ret = ldg_io_ipc_accept(listener, out);
        if (ret != LDG_ERR_WOULD_BLOCK) { return ret; }

        if (remaining == 0) { return LDG_ERR_TIMEOUT; }

        slice = (remaining < LDG_IO_IPC_WAIT_SLICE_MS) ? remaining : LDG_IO_IPC_WAIT_SLICE_MS;

        ret = io_ipc_wait((int32_t)listener->handle, slice);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK && ret != LDG_ERR_TIMEOUT)) { return ret; }

        if (timeout_ms != LDG_IO_IPC_TIMEOUT_NONE) { remaining -= slice; }
    }
}

uint32_t ldg_io_ipc_accept_cancel(ldg_io_ipc_t *listener)
{
    if (LDG_UNLIKELY(!listener)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(listener->live != LDG_TRUTH_TRUE || listener->listening != LDG_TRUTH_TRUE)) { return LDG_ERR_NOT_INIT; }

    listener->cancelled = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_io_ipc_rd_timed(ldg_io_ipc_t *ipc, ldg_byte_t *dst, uint64_t cap, uint64_t timeout_ms, uint64_t *out_got)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ipc || !dst || !out_got)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_got = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_io_ipc_rd(ipc, dst, cap, out_got);
    if (ret != LDG_ERR_AOK || *out_got > 0) { return ret; }

    if (timeout_ms == 0) { return LDG_ERR_TIMEOUT; }

    ret = io_ipc_wait((int32_t)ipc->handle, timeout_ms);
    if (ret != LDG_ERR_AOK) { return ret; }

    ret = ldg_io_ipc_rd(ipc, dst, cap, out_got);
    if (ret == LDG_ERR_AOK && *out_got == 0) { return LDG_ERR_TIMEOUT; }

    return ret;
}

uint32_t ldg_io_ipc_list(const char *prefix, ldg_io_ipc_name_t *out, uint64_t cap, uint64_t *out_cunt)
{
    char blk[LDG_IO_IPC_LIST_BLK_SIZE] = LDG_ARR_ZERO_INIT;
    char line[LDG_IO_IPC_LIST_LINE_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t prefix_len = 0;
    uint64_t line_len = 0;
    uint64_t idx = 0;
    int64_t got = 0;
    int32_t fd = 0;
    uint32_t ret = LDG_ERR_AOK;
    uint8_t line_skip = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!prefix || !out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(ldg_str_len_get(prefix, LDG_IO_IPC_NAME_MAX, &prefix_len) != LDG_ERR_AOK)) { return LDG_ERR_STR_TRUNC; }

    fd = (int32_t)open(LDG_IO_IPC_LIST_PATH, O_RDONLY | O_CLOEXEC);
    if (LDG_UNLIKELY(fd < 0)) { return LDG_ERR_IO_OPEN; }

    for (;;)
    {
        got = (int64_t)read(fd, blk, LDG_IO_IPC_LIST_BLK_SIZE);
        if (got < 0 && errno == EINTR) { continue; }

        if (LDG_UNLIKELY(got < 0)) { ret = LDG_ERR_IO_RD; }

        if (got <= 0) { break; }

        for (idx = 0; idx < (uint64_t)got; idx++)
        {
            if (blk[idx] != LDG_IO_IPC_LIST_LINE_END)
            {
                if (line_len + LDG_STR_TERM_SIZE >= LDG_IO_IPC_LIST_LINE_CAP) { line_skip = LDG_TRUTH_TRUE; }
                else { line[line_len++] = blk[idx]; }

                continue;
            }

            line[line_len] = LDG_STR_TERM;
            if (line_skip != LDG_TRUTH_TRUE) { io_ipc_list_line_take(line, line_len, prefix, prefix_len, out, cap, out_cunt); }

            line_len = 0;
            line_skip = LDG_TRUTH_FALSE;
        }
    }

    if (LDG_UNLIKELY(close(fd) != 0) && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}
