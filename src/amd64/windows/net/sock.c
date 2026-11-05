#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <dangling/net/sock.h>
#include <dangling/str/str.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>
#include <dangling/time/time.h>

#define LDG_NET_SOCK_REST_NONE 0
#define LDG_NET_SOCK_LINGER_ON 1
#define LDG_NET_SOCK_LINGER_SPAN_NONE 0
#define LDG_NET_SOCK_DRAIN_CAP 512
#define LDG_NET_SOCK_FLAG_NONE 0
#define LDG_NET_SOCK_STEP 1
#define LDG_NET_SOCK_PORT_DIGIT_MAX 6
#define LDG_NET_SOCK_WSA_MAJOR 2
#define LDG_NET_SOCK_WSA_MINOR 2
#define LDG_NET_SOCK_NONBLOCK_ON 1
#define LDG_NET_SOCK_GROUP_NONE 0
#define LDG_NET_SOCK_US_PER_MS 1000
#define LDG_NET_SOCK_OPT_ON 1

static void sock_port_str_set(uint16_t port, char *out)
{
    char digit[LDG_NET_SOCK_PORT_DIGIT_MAX] = LDG_ARR_ZERO_INIT;
    uint32_t cunt = 0;
    uint32_t pos = 0;
    uint32_t val = port;

    if (val == 0)
    {
        out[0] = LDG_ASCII_DIGIT_ZERO;
        out[LDG_NET_SOCK_STEP] = LDG_STR_TERM;
        return;
    }

    while (val > 0)
    {
        digit[cunt] = (char)(LDG_ASCII_DIGIT_ZERO + (val % LDG_BASE_DEC));
        val /= LDG_BASE_DEC;
        cunt++;
    }

    while (cunt > 0)
    {
        cunt--;
        out[pos] = digit[cunt];
        pos++;
    }

    out[pos] = LDG_STR_TERM;
}

static uint32_t sock_wsa_map(int32_t code, uint32_t fallback)
{
    if (code == WSAECONNREFUSED) { return LDG_ERR_NET_REFUSED; }

    if (code == WSAENETUNREACH || code == WSAEHOSTUNREACH || code == WSAENETDOWN) { return LDG_ERR_NET_UNREACHABLE; }

    if (code == WSAECONNRESET || code == WSAECONNABORTED || code == WSAESHUTDOWN) { return LDG_ERR_NET_RST; }

    if (code == WSAETIMEDOUT) { return LDG_ERR_NET_TIMEOUT; }

    return fallback;
}

static uint32_t sock_connect_wait(SOCKET fd, uint64_t timeout)
{
    struct timeval span = LDG_STRUCT_ZERO_INIT;
    fd_set writable = LDG_STRUCT_ZERO_INIT;
    fd_set faulted = LDG_STRUCT_ZERO_INIT;
    int64_t sec = 0;
    int64_t usec = 0;
    int32_t ready = 0;
    int32_t pending = 0;
    int32_t opt_len = 0;

    FD_ZERO(&writable);
    FD_SET(fd, &writable);
    FD_ZERO(&faulted);
    FD_SET(fd, &faulted);

    sec = (int64_t)(timeout / LDG_MS_PER_SEC);
    usec = (int64_t)((timeout % LDG_MS_PER_SEC) * LDG_NET_SOCK_US_PER_MS);
    span.tv_sec = sec;
    span.tv_usec = usec;

    ready = (int32_t)select(FD_SETSIZE, 0x0, &writable, &faulted, &span);

    if (ready == 0) { return LDG_ERR_NET_TIMEOUT; }

    if (ready < 0) { return LDG_ERR_NET_CONN; }

    opt_len = (int32_t)sizeof(pending);

    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&pending, &opt_len) != 0) { return LDG_ERR_NET_CONN; }

    if (pending != 0) { return sock_wsa_map(pending, LDG_ERR_NET_CONN); }

    return LDG_ERR_AOK;
}

static uint32_t sock_node_connect(const struct addrinfo *node, uint64_t timeout, uint64_t *out_handle)
{
    SOCKET fd = INVALID_SOCKET;
    u_long nonblock = LDG_NET_SOCK_NONBLOCK_ON;
    int32_t nodelay = LDG_NET_SOCK_STEP;
    int32_t addr_len = 0;
    uint32_t ret = 0;

    fd = WSASocketW(node->ai_family, node->ai_socktype, node->ai_protocol, 0x0, LDG_NET_SOCK_GROUP_NONE, WSA_FLAG_OVERLAPPED);

    if (fd == INVALID_SOCKET) { return LDG_ERR_NET_CONN; }

    if (ioctlsocket(fd, FIONBIO, &nonblock) != 0)
    {
        closesocket(fd);
        return LDG_ERR_NET_CONN;
    }

    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&nodelay, (int32_t)sizeof(nodelay)) != 0)
    {
        closesocket(fd);
        return LDG_ERR_NET_CONN;
    }

    addr_len = (int32_t)node->ai_addrlen;

    if (connect(fd, node->ai_addr, addr_len) == 0)
    {
        *out_handle = (uint64_t)fd;
        return LDG_ERR_AOK;
    }

    if (WSAGetLastError() != WSAEWOULDBLOCK)
    {
        ret = sock_wsa_map((int32_t)WSAGetLastError(), LDG_ERR_NET_CONN);
        closesocket(fd);
        return ret;
    }

    ret = sock_connect_wait(fd, timeout);

    if (ret != LDG_ERR_AOK)
    {
        closesocket(fd);
        return ret;
    }

    *out_handle = (uint64_t)fd;

    return LDG_ERR_AOK;
}

static uint32_t sock_rest_get(uint64_t start, uint64_t timeout, int32_t *out)
{
    uint64_t now = 0;
    uint64_t spent = 0;
    uint64_t rest = 0;
    uint32_t ret = 0;

    *out = LDG_NET_SOCK_REST_NONE;

    ret = ldg_time_monotonic_get(&now);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (now > start) { spent = now - start; }

    if (spent >= timeout) { return LDG_ERR_AOK; }

    rest = timeout - spent;
    if (rest > INT32_MAX) { rest = INT32_MAX; }

    *out = (int32_t)rest;

    return LDG_ERR_AOK;
}

static uint32_t sock_ready_wait(SOCKET fd, uint8_t wr_want, uint64_t start, uint64_t timeout, uint32_t fallback)
{
    struct timeval span = LDG_STRUCT_ZERO_INIT;
    fd_set want = LDG_STRUCT_ZERO_INIT;
    fd_set faulted = LDG_STRUCT_ZERO_INIT;
    int64_t sec = 0;
    int64_t usec = 0;
    int32_t rest = 0;
    int32_t ready = 0;
    uint32_t ret = 0;

    ret = sock_rest_get(start, timeout, &rest);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (rest == LDG_NET_SOCK_REST_NONE) { return LDG_ERR_TIMEOUT; }

    FD_ZERO(&want);
    FD_SET(fd, &want);
    FD_ZERO(&faulted);
    FD_SET(fd, &faulted);

    sec = (int64_t)((uint64_t)rest / LDG_MS_PER_SEC);
    usec = (int64_t)(((uint64_t)rest % LDG_MS_PER_SEC) * LDG_NET_SOCK_US_PER_MS);
    span.tv_sec = sec;
    span.tv_usec = usec;

    if (wr_want == LDG_TRUTH_TRUE) { ready = (int32_t)select(FD_SETSIZE, 0x0, &want, &faulted, &span); }
    else { ready = (int32_t)select(FD_SETSIZE, &want, 0x0, &faulted, &span); }

    if (ready > 0) { return LDG_ERR_AOK; }

    if (ready == 0) { return LDG_ERR_TIMEOUT; }

    if (WSAGetLastError() == WSAEINTR) { return LDG_ERR_AOK; }

    return fallback;
}

static uint32_t sock_handle_release(ldg_net_sock_t *sock, uint8_t rst, uint32_t ret)
{
    struct linger hold = LDG_STRUCT_ZERO_INIT;

    if (rst == LDG_TRUTH_TRUE)
    {
        hold.l_onoff = LDG_NET_SOCK_LINGER_ON;
        hold.l_linger = LDG_NET_SOCK_LINGER_SPAN_NONE;
        if (setsockopt((SOCKET)sock->handle, SOL_SOCKET, SO_LINGER, (const char *)&hold, (int32_t)sizeof(hold)) != 0 && ret == LDG_ERR_AOK) { ret = LDG_ERR_NET_CONN; }
    }

    if (closesocket((SOCKET)sock->handle) != 0 && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    sock->handle = LDG_NET_SOCK_HANDLE_NONE;
    sock->live = LDG_TRUTH_FALSE;

    return ret;
}

uint32_t ldg_net_sock_init(void)
{
    WSADATA wsa = LDG_STRUCT_ZERO_INIT;

    if (WSAStartup(MAKEWORD(LDG_NET_SOCK_WSA_MAJOR, LDG_NET_SOCK_WSA_MINOR), &wsa) != 0) { return LDG_ERR_NET_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_sock_shutdown(void)
{
    if (WSACleanup() != 0) { return LDG_ERR_NET_INIT; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_sock_open(const char *host, uint16_t port, uint64_t timeout, ldg_net_sock_t *out)
{
    struct addrinfo hints = LDG_STRUCT_ZERO_INIT;
    struct addrinfo *list = 0x0;
    struct addrinfo *node = 0x0;
    char port_str[LDG_NET_SOCK_PORT_STR_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t handle = LDG_NET_SOCK_HANDLE_NONE;
    uint32_t ret = LDG_ERR_NET_CONN;

    if (LDG_UNLIKELY(!host || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->handle = LDG_NET_SOCK_HANDLE_NONE;
    out->wr_total = 0;
    out->rd_total = 0;
    out->live = LDG_TRUTH_FALSE;
    out->listening = LDG_TRUTH_FALSE;

    sock_port_str_set(port, port_str);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host, port_str, &hints, &list) != 0) { return LDG_ERR_NET_DNS; }

    if (LDG_UNLIKELY(!list)) { return LDG_ERR_NET_DNS; }

    for (node = list; node != 0x0; node = node->ai_next)
    {
        ret = sock_node_connect(node, timeout, &handle);

        if (ret == LDG_ERR_AOK) { break; }
    }

    freeaddrinfo(list);

    if (ret != LDG_ERR_AOK) { return ret; }

    out->handle = handle;
    out->live = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_sock_close(ldg_net_sock_t *sock, uint64_t timeout)
{
    char drain[LDG_NET_SOCK_DRAIN_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t start = 0;
    int32_t xfer = 0;
    int32_t rest = 0;
    int32_t code = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!sock)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sock->handle == LDG_NET_SOCK_HANDLE_NONE) { return LDG_ERR_NOT_INIT; }

    if (sock->listening == LDG_TRUTH_TRUE)
    {
        sock->listening = LDG_TRUTH_FALSE;
        return sock_handle_release(sock, LDG_TRUTH_FALSE, LDG_ERR_AOK);
    }

    ret = ldg_time_monotonic_get(&start);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return sock_handle_release(sock, LDG_TRUTH_TRUE, ret); }

    if (shutdown((SOCKET)sock->handle, SD_SEND) != 0) { return sock_handle_release(sock, LDG_TRUTH_TRUE, sock_wsa_map((int32_t)WSAGetLastError(), LDG_ERR_NET_CONN)); }

    while (ret == LDG_ERR_AOK)
    {
        xfer = recv((SOCKET)sock->handle, drain, (int32_t)sizeof(drain), LDG_NET_SOCK_FLAG_NONE);

        if (xfer == 0) { return sock_handle_release(sock, LDG_TRUTH_FALSE, LDG_ERR_AOK); }

        if (xfer > 0)
        {
            ret = sock_rest_get(start, timeout, &rest);
            if (ret == LDG_ERR_AOK && rest == LDG_NET_SOCK_REST_NONE) { ret = LDG_ERR_TIMEOUT; }

            continue;
        }

        code = (int32_t)WSAGetLastError();

        if (code != WSAEWOULDBLOCK && code != WSAEINTR) { return sock_handle_release(sock, LDG_TRUTH_TRUE, sock_wsa_map(code, LDG_ERR_NET_RECV)); }

        ret = sock_ready_wait((SOCKET)sock->handle, LDG_TRUTH_FALSE, start, timeout, LDG_ERR_NET_RECV);
    }

    return sock_handle_release(sock, LDG_TRUTH_TRUE, ret);
}

uint32_t ldg_net_sock_wr(ldg_net_sock_t *sock, const ldg_byte_t *src, uint64_t len, uint64_t timeout, uint64_t *bytes_wr)
{
    uint64_t start = 0;
    uint64_t chunk = 0;
    int32_t xfer = 0;
    int32_t code = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!sock || !src || !bytes_wr)) { return LDG_ERR_FUNC_ARG_NULL; }

    *bytes_wr = 0;

    if (LDG_UNLIKELY(sock->handle == LDG_NET_SOCK_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    ret = ldg_time_monotonic_get(&start);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (*bytes_wr < len)
    {
        chunk = len - *bytes_wr;
        if (chunk > INT32_MAX) { chunk = INT32_MAX; }

        xfer = send((SOCKET)sock->handle, (const char *)(src + *bytes_wr), (int32_t)chunk, LDG_NET_SOCK_FLAG_NONE);

        if (xfer >= 0)
        {
            *bytes_wr += (uint64_t)xfer;
            sock->wr_total += (uint64_t)xfer;
            continue;
        }

        code = (int32_t)WSAGetLastError();

        if (code == WSAECONNRESET || code == WSAECONNABORTED || code == WSAESHUTDOWN)
        {
            sock->live = LDG_TRUTH_FALSE;
            return LDG_ERR_NET_RST;
        }

        if (code != WSAEWOULDBLOCK && code != WSAEINTR) { return sock_wsa_map(code, LDG_ERR_NET_SEND); }

        ret = sock_ready_wait((SOCKET)sock->handle, LDG_TRUTH_TRUE, start, timeout, LDG_ERR_NET_SEND);
        if (ret != LDG_ERR_AOK) { return ret; }
    }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_sock_rd(ldg_net_sock_t *sock, ldg_byte_t *dst, uint64_t cap, uint64_t timeout, uint64_t *bytes_rd)
{
    uint64_t start = 0;
    int32_t xfer = 0;
    int32_t code = 0;
    int32_t span = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!sock || !dst || !bytes_rd)) { return LDG_ERR_FUNC_ARG_NULL; }

    *bytes_rd = 0;

    if (LDG_UNLIKELY(sock->handle == LDG_NET_SOCK_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (cap == 0) { return LDG_ERR_AOK; }

    if (cap > INT32_MAX) { cap = INT32_MAX; }

    span = (int32_t)cap;

    ret = ldg_time_monotonic_get(&start);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (ret == LDG_ERR_AOK)
    {
        xfer = recv((SOCKET)sock->handle, (char *)dst, span, LDG_NET_SOCK_FLAG_NONE);

        if (xfer > 0)
        {
            *bytes_rd = (uint64_t)xfer;
            sock->rd_total += (uint64_t)xfer;
            return LDG_ERR_AOK;
        }

        if (xfer == 0)
        {
            sock->live = LDG_TRUTH_FALSE;
            return LDG_ERR_CLOSED;
        }

        code = (int32_t)WSAGetLastError();

        if (code == WSAECONNRESET || code == WSAECONNABORTED || code == WSAESHUTDOWN)
        {
            sock->live = LDG_TRUTH_FALSE;
            return LDG_ERR_NET_RST;
        }

        if (code != WSAEWOULDBLOCK && code != WSAEINTR) { return sock_wsa_map(code, LDG_ERR_NET_RECV); }

        ret = sock_ready_wait((SOCKET)sock->handle, LDG_TRUTH_FALSE, start, timeout, LDG_ERR_NET_RECV);
    }

    return ret;
}

uint8_t ldg_net_sock_live_is(const ldg_net_sock_t *sock)
{
    if (LDG_UNLIKELY(!sock)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sock->handle == LDG_NET_SOCK_HANDLE_NONE) { return LDG_TRUTH_FALSE; }

    return (sock->live == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static uint32_t sock_node_listen(const struct addrinfo *node, uint64_t *out_handle)
{
    SOCKET fd = INVALID_SOCKET;
    u_long nonblock = LDG_NET_SOCK_NONBLOCK_ON;
    int32_t opt_on = LDG_NET_SOCK_OPT_ON;
    int32_t code = 0;

    fd = WSASocketW(node->ai_family, node->ai_socktype, node->ai_protocol, 0x0, LDG_NET_SOCK_GROUP_NONE, WSA_FLAG_OVERLAPPED);

    if (fd == INVALID_SOCKET) { return LDG_ERR_NET_CONN; }

    if (ioctlsocket(fd, FIONBIO, &nonblock) != 0)
    {
        closesocket(fd);
        return LDG_ERR_NET_CONN;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char *)&opt_on, (int32_t)sizeof(opt_on)) != 0)
    {
        closesocket(fd);
        return LDG_ERR_NET_CONN;
    }

    if (node->ai_family == AF_INET6 && setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&opt_on, (int32_t)sizeof(opt_on)) != 0)
    {
        closesocket(fd);
        return LDG_ERR_NET_CONN;
    }

    if (bind(fd, node->ai_addr, (int32_t)node->ai_addrlen) != 0 || listen(fd, SOMAXCONN) != 0)
    {
        code = (int32_t)WSAGetLastError();
        closesocket(fd);

        if (code == WSAEADDRINUSE) { return LDG_ERR_BUSY; }

        if (code == WSAEACCES) { return LDG_ERR_DENIED; }

        return sock_wsa_map(code, LDG_ERR_NET_CONN);
    }

    *out_handle = (uint64_t)fd;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_sock_listen(const char *addr, uint16_t port, ldg_net_sock_t *out)
{
    struct addrinfo hints = LDG_STRUCT_ZERO_INIT;
    struct addrinfo *list = 0x0;
    struct addrinfo *node = 0x0;
    char port_str[LDG_NET_SOCK_PORT_STR_CAP] = LDG_ARR_ZERO_INIT;
    uint64_t handle = LDG_NET_SOCK_HANDLE_NONE;
    uint32_t ret = LDG_ERR_NET_CONN;

    if (LDG_UNLIKELY(!addr || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->handle = LDG_NET_SOCK_HANDLE_NONE;
    out->wr_total = 0;
    out->rd_total = 0;
    out->live = LDG_TRUTH_FALSE;
    out->listening = LDG_TRUTH_FALSE;

    sock_port_str_set(port, port_str);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(addr, port_str, &hints, &list) != 0) { return LDG_ERR_NET_DNS; }

    if (LDG_UNLIKELY(!list)) { return LDG_ERR_NET_DNS; }

    for (node = list; node != 0x0; node = node->ai_next)
    {
        ret = sock_node_listen(node, &handle);

        if (ret == LDG_ERR_AOK) { break; }
    }

    freeaddrinfo(list);

    if (ret != LDG_ERR_AOK) { return ret; }

    out->handle = handle;
    out->live = LDG_TRUTH_TRUE;
    out->listening = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_sock_accept(ldg_net_sock_t *listener, uint64_t timeout, ldg_net_sock_t *out)
{
    SOCKET fd = INVALID_SOCKET;
    u_long nonblock = LDG_NET_SOCK_NONBLOCK_ON;
    uint64_t start = 0;
    int32_t nodelay = LDG_NET_SOCK_OPT_ON;
    int32_t code = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!listener || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->handle = LDG_NET_SOCK_HANDLE_NONE;
    out->wr_total = 0;
    out->rd_total = 0;
    out->live = LDG_TRUTH_FALSE;
    out->listening = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(listener->handle == LDG_NET_SOCK_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(listener->listening != LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_time_monotonic_get(&start);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    while (ret == LDG_ERR_AOK)
    {
        fd = accept((SOCKET)listener->handle, 0x0, 0x0);

        if (fd != INVALID_SOCKET)
        {
            if (ioctlsocket(fd, FIONBIO, &nonblock) != 0 || setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&nodelay, (int32_t)sizeof(nodelay)) != 0)
            {
                closesocket(fd);
                return LDG_ERR_NET_CONN;
            }

            out->handle = (uint64_t)fd;
            out->live = LDG_TRUTH_TRUE;

            return LDG_ERR_AOK;
        }

        code = (int32_t)WSAGetLastError();

        if (code != WSAEWOULDBLOCK && code != WSAEINTR && code != WSAECONNRESET) { return sock_wsa_map(code, LDG_ERR_NET_CONN); }

        ret = sock_ready_wait((SOCKET)listener->handle, LDG_TRUTH_FALSE, start, timeout, LDG_ERR_NET_CONN);
    }

    return ret;
}

uint32_t ldg_net_sock_wait(const ldg_net_sock_t *sock, uint32_t cunt, uint64_t timeout, uint8_t *ready)
{
    WSAPOLLFD pfd[LDG_NET_SOCK_WAIT_MAX] = LDG_ARR_ZERO_INIT;
    uint64_t span = timeout;
    int32_t got = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!sock || !ready)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(cunt == 0 || cunt > LDG_NET_SOCK_WAIT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    for (idx = 0; idx < cunt; idx++) { ready[idx] = LDG_TRUTH_FALSE; }

    for (idx = 0; idx < cunt; idx++)
    {
        if (LDG_UNLIKELY(sock[idx].handle == LDG_NET_SOCK_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

        pfd[idx].fd = (SOCKET)sock[idx].handle;
        pfd[idx].events = POLLRDNORM;
    }

    if (span > INT32_MAX) { span = INT32_MAX; }

    got = (int32_t)WSAPoll(pfd, (ULONG)cunt, (INT)span);

    if (got == 0) { return LDG_ERR_TIMEOUT; }

    if (got < 0)
    {
        if (WSAGetLastError() == WSAEINTR) { return LDG_ERR_INTERRUPTED; }

        return LDG_ERR_NET_RECV;
    }

    for (idx = 0; idx < cunt; idx++) { if (pfd[idx].revents != 0) { ready[idx] = LDG_TRUTH_TRUE; } }

    return LDG_ERR_AOK;
}
