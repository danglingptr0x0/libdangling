#include <stdint.h>

#include <dangling/net/sock.h>
#include <dangling/sys/handover.h>
#include <dangling/str/str.h>
#include <dangling/core/bits.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_NET_SOCK_SYM_OPEN "dangling_net_tcp_open"
#define LDG_NET_SOCK_SYM_SEND "dangling_net_tcp_send"
#define LDG_NET_SOCK_SYM_RECV "dangling_net_tcp_recv"
#define LDG_NET_SOCK_SYM_CLOSE "dangling_net_tcp_close"
#define LDG_NET_SOCK_OCTET_CUNT 4
#define LDG_NET_SOCK_OCTET_MAX 255
#define LDG_NET_SOCK_OCTET_DIGIT_MAX 3
#define LDG_NET_SOCK_OCTET_SEP '.'

typedef uint32_t (*ldg_net_sock_open_cb_t)(void *kernel, uint32_t addr, uint32_t port, uint32_t timeout_ms, uint32_t *out_conn);
typedef uint32_t (*ldg_net_sock_send_cb_t)(void *kernel, uint32_t conn, const void *buff, uint32_t len, uint32_t timeout_ms, uint32_t *out_len);
typedef uint32_t (*ldg_net_sock_recv_cb_t)(void *kernel, uint32_t conn, void *buff, uint32_t cap, uint32_t timeout_ms, uint32_t *out_len);
typedef uint32_t (*ldg_net_sock_close_cb_t)(void *kernel, uint32_t conn, uint32_t timeout_ms);

typedef struct ldg_net_sock_state
{
    void *kernel;
    ldg_net_sock_open_cb_t open;
    ldg_net_sock_send_cb_t send;
    ldg_net_sock_recv_cb_t recv;
    ldg_net_sock_close_cb_t close;
    uint8_t is_bound;
    uint8_t pudding[7];
} ldg_net_sock_state_t;

static ldg_net_sock_state_t* sock_state_get(void)
{
    static ldg_net_sock_state_t state = LDG_STRUCT_ZERO_INIT;

    return &state;
}

static uint32_t sock_bind_ensure(ldg_net_sock_state_t *state)
{
    uint64_t addr = 0;
    uint32_t ret = 0;

    if (state->is_bound == LDG_TRUTH_TRUE) { return LDG_ERR_AOK; }

    ret = ldg_sys_kernel_get(&state->kernel);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = ldg_sys_sym_get(LDG_NET_SOCK_SYM_OPEN, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->open = (ldg_net_sock_open_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_NET_SOCK_SYM_SEND, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->send = (ldg_net_sock_send_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_NET_SOCK_SYM_RECV, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->recv = (ldg_net_sock_recv_cb_t)(uintptr_t)addr;

    ret = ldg_sys_sym_get(LDG_NET_SOCK_SYM_CLOSE, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    state->close = (ldg_net_sock_close_cb_t)(uintptr_t)addr;

    state->is_bound = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

static uint32_t sock_timeout_clamp(uint64_t timeout)
{
    if (timeout > UINT32_MAX) { return UINT32_MAX; }

    return (uint32_t)timeout;
}

static uint32_t sock_addr_parse(const char *host, uint32_t *out)
{
    uint64_t pos = 0;
    uint32_t octet = 0;
    uint32_t octet_idx = 0;
    uint32_t digit_cunt = 0;
    uint32_t addr = 0;

    *out = 0;

    while (octet_idx < LDG_NET_SOCK_OCTET_CUNT)
    {
        octet = 0;
        digit_cunt = 0;

        while (ldg_char_digit_is((uint8_t)host[pos]) == LDG_TRUTH_TRUE)
        {
            octet = (octet * LDG_BASE_DEC) + (uint32_t)(host[pos] - LDG_ASCII_DIGIT_ZERO);
            digit_cunt++;
            pos++;

            if (digit_cunt > LDG_NET_SOCK_OCTET_DIGIT_MAX || octet > LDG_NET_SOCK_OCTET_MAX) { return LDG_ERR_NET_DNS; }
        }

        if (digit_cunt == 0) { return LDG_ERR_NET_DNS; }

        addr = (addr << LDG_BYTE_BITS) | octet;
        octet_idx++;

        if (octet_idx == LDG_NET_SOCK_OCTET_CUNT) { break; }

        if (host[pos] != LDG_NET_SOCK_OCTET_SEP) { return LDG_ERR_NET_DNS; }

        pos++;
    }

    if (host[pos] != LDG_STR_TERM) { return LDG_ERR_NET_DNS; }

    *out = addr;

    return LDG_ERR_AOK;
}

static void sock_fail_mark(ldg_net_sock_t *sock, uint32_t ret)
{
    if (ret == LDG_ERR_NET_RST || ret == LDG_ERR_NET_DISCONNECTED || ret == LDG_ERR_CLOSED) { sock->live = LDG_TRUTH_FALSE; }
}

uint32_t ldg_net_sock_init(void)
{
    return sock_bind_ensure(sock_state_get());
}

uint32_t ldg_net_sock_shutdown(void)
{
    return LDG_ERR_AOK;
}

uint32_t ldg_net_sock_open(const char *host, uint16_t port, uint64_t timeout, ldg_net_sock_t *out)
{
    ldg_net_sock_state_t *state = 0x0;
    uint32_t addr = 0;
    uint32_t conn = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!host || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    out->handle = LDG_NET_SOCK_HANDLE_NONE;
    out->wr_total = 0;
    out->rd_total = 0;
    out->live = LDG_TRUTH_FALSE;

    state = sock_state_get();

    ret = sock_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = sock_addr_parse(host, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = state->open(state->kernel, addr, (uint32_t)port, sock_timeout_clamp(timeout), &conn);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    out->handle = (uint64_t)conn;
    out->live = LDG_TRUTH_TRUE;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_sock_close(ldg_net_sock_t *sock, uint64_t timeout)
{
    ldg_net_sock_state_t *state = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!sock)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sock->handle == LDG_NET_SOCK_HANDLE_NONE) { return LDG_ERR_NOT_INIT; }

    state = sock_state_get();

    ret = sock_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = state->close(state->kernel, (uint32_t)sock->handle, sock_timeout_clamp(timeout));

    sock->handle = LDG_NET_SOCK_HANDLE_NONE;
    sock->live = LDG_TRUTH_FALSE;

    return ret;
}

uint32_t ldg_net_sock_wr(ldg_net_sock_t *sock, const ldg_byte_t *src, uint64_t len, uint64_t timeout, uint64_t *bytes_wr)
{
    ldg_net_sock_state_t *state = 0x0;
    uint32_t taken = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!sock || !src || !bytes_wr)) { return LDG_ERR_FUNC_ARG_NULL; }

    *bytes_wr = 0;

    if (LDG_UNLIKELY(sock->handle == LDG_NET_SOCK_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (len == 0) { return LDG_ERR_AOK; }

    if (LDG_UNLIKELY(len > UINT32_MAX)) { return LDG_ERR_RANGE; }

    state = sock_state_get();

    ret = sock_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = state->send(state->kernel, (uint32_t)sock->handle, src, (uint32_t)len, sock_timeout_clamp(timeout), &taken);

    *bytes_wr = (uint64_t)taken;
    sock->wr_total += (uint64_t)taken;
    sock_fail_mark(sock, ret);

    return ret;
}

uint32_t ldg_net_sock_rd(ldg_net_sock_t *sock, ldg_byte_t *dst, uint64_t cap, uint64_t timeout, uint64_t *bytes_rd)
{
    ldg_net_sock_state_t *state = 0x0;
    uint32_t got = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!sock || !dst || !bytes_rd)) { return LDG_ERR_FUNC_ARG_NULL; }

    *bytes_rd = 0;

    if (LDG_UNLIKELY(sock->handle == LDG_NET_SOCK_HANDLE_NONE)) { return LDG_ERR_NOT_INIT; }

    if (cap == 0) { return LDG_ERR_AOK; }

    if (cap > UINT32_MAX) { cap = UINT32_MAX; }

    state = sock_state_get();

    ret = sock_bind_ensure(state);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ret = state->recv(state->kernel, (uint32_t)sock->handle, dst, (uint32_t)cap, sock_timeout_clamp(timeout), &got);

    *bytes_rd = (uint64_t)got;
    sock->rd_total += (uint64_t)got;
    sock_fail_mark(sock, ret);

    return ret;
}

uint8_t ldg_net_sock_live_is(const ldg_net_sock_t *sock)
{
    if (LDG_UNLIKELY(!sock)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (sock->handle == LDG_NET_SOCK_HANDLE_NONE) { return LDG_TRUTH_FALSE; }

    return (sock->live == LDG_TRUTH_TRUE) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}
