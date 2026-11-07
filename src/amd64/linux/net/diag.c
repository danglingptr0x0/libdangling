#include <stdint.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>

#include <dangling/net/diag.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/io/file.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>
#include <dangling/time/time.h>

#define LDG_NET_DIAG_RESOLV_PATH "/etc/resolv.conf"
#define LDG_NET_DIAG_RESOLV_KEY "nameserver"
#define LDG_NET_DIAG_RESOLV_KEY_LEN 10
#define LDG_NET_DIAG_RESOLV_COMMENT '#'
#define LDG_NET_DIAG_RESOLV_COMMENT_ALT ';'
#define LDG_NET_DIAG_LINE_CAP 256
#define LDG_NET_DIAG_BLK_SIZE 4096
#define LDG_NET_DIAG_FILE_ACCESS 0
#define LDG_NET_DIAG_ECHO_SEQ 1
#define LDG_NET_DIAG_ECHO_CODE 0
#define LDG_NET_DIAG_REPLY_CAP 128
#define LDG_NET_DIAG_WATCH_CUNT 1
#define LDG_NET_DIAG_NO_FLAG 0
#define LDG_NET_DIAG_TIMEOUT_MAX 60000

static uint32_t net_diag_resolve(const char *host, struct sockaddr_in *out)
{
    struct addrinfo hints = LDG_STRUCT_ZERO_INIT;
    struct addrinfo *res = 0x0;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(host, 0x0, &hints, &res) != 0 || !res) { return LDG_ERR_NET_DNS; }

    if (res->ai_addrlen < sizeof(struct sockaddr_in))
    {
        freeaddrinfo(res);
        return LDG_ERR_NET_DNS;
    }

    *out = *(const struct sockaddr_in *)(const void *)res->ai_addr;
    freeaddrinfo(res);

    return LDG_ERR_AOK;
}

static uint32_t net_diag_reply_wait(int32_t fd, uint64_t start_ns, uint64_t timeout, uint64_t *out_rtt_us)
{
    uint8_t reply[LDG_NET_DIAG_REPLY_CAP] = LDG_ARR_ZERO_INIT;
    struct icmphdr hdr = LDG_STRUCT_ZERO_INIT;
    struct pollfd watch = LDG_STRUCT_ZERO_INIT;
    uint64_t now_ns = 0;
    uint64_t spent_ms = 0;
    int64_t got = 0;
    uint32_t ret = 0;

    watch.fd = fd;
    watch.events = POLLIN;

    while (LDG_TRUTH_TRUE)
    {
        ret = ldg_time_monotonic_ns_get(&now_ns);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        spent_ms = (now_ns - start_ns) / LDG_NS_PER_MS;
        if (spent_ms >= timeout) { return LDG_ERR_TIMEOUT; }

        if (poll(&watch, LDG_NET_DIAG_WATCH_CUNT, (int32_t)(timeout - spent_ms)) <= 0)
        {
            if (errno == EINTR) { continue; }

            return LDG_ERR_TIMEOUT;
        }

        got = (int64_t)recv(fd, reply, sizeof(reply), LDG_NET_DIAG_NO_FLAG);
        if (got < (int64_t)sizeof(hdr)) { continue; }

        if (LDG_UNLIKELY(ldg_mem_secure_cpy(&hdr, reply, sizeof(hdr)) != LDG_ERR_AOK)) { return LDG_ERR_MEM_BAD; }

        if (hdr.type != ICMP_ECHOREPLY || ntohs(hdr.un.echo.sequence) != LDG_NET_DIAG_ECHO_SEQ) { continue; }

        ret = ldg_time_monotonic_ns_get(&now_ns);
        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

        *out_rtt_us = (now_ns - start_ns) / LDG_NS_PER_US;

        return LDG_ERR_AOK;
    }
}

uint32_t ldg_net_diag_ping(const char *host, uint64_t timeout, uint64_t *out_rtt_us)
{
    struct sockaddr_in addr = LDG_STRUCT_ZERO_INIT;
    struct icmphdr req = LDG_STRUCT_ZERO_INIT;
    uint64_t start_ns = 0;
    int32_t fd = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!host || !out_rtt_us)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_rtt_us = 0;

    if (LDG_UNLIKELY(timeout == 0 || timeout > LDG_NET_DIAG_TIMEOUT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = net_diag_resolve(host, &addr);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    fd = (int32_t)socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_ICMP);
    if (fd < 0) { return (errno == EACCES || errno == EPERM || errno == EPROTONOSUPPORT) ? LDG_ERR_DENIED : LDG_ERR_NET_INIT; }

    req.type = ICMP_ECHO;
    req.code = LDG_NET_DIAG_ECHO_CODE;
    req.un.echo.sequence = htons(LDG_NET_DIAG_ECHO_SEQ);

    ret = ldg_time_monotonic_ns_get(&start_ns);
    if (ret == LDG_ERR_AOK && sendto(fd, &req, sizeof(req), LDG_NET_DIAG_NO_FLAG, (const struct sockaddr *)(const void *)&addr, sizeof(addr)) < 0) { ret = LDG_ERR_NET_SEND; }

    if (ret == LDG_ERR_AOK) { ret = net_diag_reply_wait(fd, start_ns, timeout, out_rtt_us); }

    if (LDG_UNLIKELY(close(fd) != 0) && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    return ret;
}

static void net_diag_resolv_line_parse(const char *line, uint64_t len, ldg_net_diag_addr_t *out, uint64_t cap, uint64_t *cunt)
{
    uint64_t pos = 0;
    uint64_t start = 0;

    while (pos < len && (line[pos] == LDG_ASCII_SPACE || line[pos] == LDG_ASCII_TAB)) { pos++; }

    if (len - pos <= LDG_NET_DIAG_RESOLV_KEY_LEN) { return; }

    if (ldg_mem_secure_eq_is(line + pos, LDG_NET_DIAG_RESOLV_KEY, LDG_NET_DIAG_RESOLV_KEY_LEN) != LDG_TRUTH_TRUE) { return; }

    pos += LDG_NET_DIAG_RESOLV_KEY_LEN;

    if (line[pos] != LDG_ASCII_SPACE && line[pos] != LDG_ASCII_TAB) { return; }

    while (pos < len && (line[pos] == LDG_ASCII_SPACE || line[pos] == LDG_ASCII_TAB)) { pos++; }

    start = pos;

    while (pos < len && line[pos] != LDG_ASCII_SPACE && line[pos] != LDG_ASCII_TAB && line[pos] != LDG_NET_DIAG_RESOLV_COMMENT && line[pos] != LDG_NET_DIAG_RESOLV_COMMENT_ALT) { pos++; }

    if (pos == start || pos - start >= LDG_NET_DIAG_ADDR_CAP || *cunt >= cap) { return; }

    if (ldg_mem_secure_cpy(out[*cunt].text, line + start, pos - start) != LDG_ERR_AOK) { return; }

    out[*cunt].text[pos - start] = LDG_STR_TERM;
    (*cunt)++;
}

uint32_t ldg_net_diag_dns_list(ldg_net_diag_addr_t *out, uint64_t cap, uint64_t *out_cunt)
{
    uint8_t blk[LDG_NET_DIAG_BLK_SIZE] __attribute__((aligned(LDG_NET_DIAG_BLK_SIZE))) = LDG_ARR_ZERO_INIT;
    char line[LDG_NET_DIAG_LINE_CAP] = LDG_ARR_ZERO_INIT;
    ldg_io_file_t *file = 0x0;
    uint64_t line_len = 0;
    uint64_t got = 0;
    uint64_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = ldg_io_file_open(LDG_NET_DIAG_RESOLV_PATH, LDG_IO_RD_ONLY | LDG_IO_DIRECT, LDG_NET_DIAG_FILE_ACCESS, &file);
    if (ret != LDG_ERR_AOK) { return ret; }

    do
    {
        ret = ldg_io_file_rd(file, blk, LDG_NET_DIAG_BLK_SIZE, &got);
        if (ret != LDG_ERR_AOK) { break; }

        for (idx = 0; idx < got; idx++)
        {
            if (blk[idx] != LDG_ASCII_LF)
            {
                if (line_len + LDG_STR_TERM_SIZE < LDG_NET_DIAG_LINE_CAP) { line[line_len] = (char)blk[idx]; }

                line_len++;
                continue;
            }

            if (line_len + LDG_STR_TERM_SIZE < LDG_NET_DIAG_LINE_CAP) { net_diag_resolv_line_parse(line, line_len, out, cap, out_cunt); }

            line_len = 0;
        }
    } while (got == LDG_NET_DIAG_BLK_SIZE);

    if (ret == LDG_ERR_AOK && line_len > 0 && line_len + LDG_STR_TERM_SIZE < LDG_NET_DIAG_LINE_CAP) { net_diag_resolv_line_parse(line, line_len, out, cap, out_cunt); }

    if (LDG_UNLIKELY(ldg_io_file_close(file) != LDG_ERR_AOK) && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    if (ret == LDG_ERR_AOK && *out_cunt == 0) { ret = LDG_ERR_NOT_FOUND; }

    return ret;
}
