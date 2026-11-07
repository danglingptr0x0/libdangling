#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#include <dangling/net/diag.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>

#define LDG_NET_DIAG_WSA_VER_MAJOR 2
#define LDG_NET_DIAG_WSA_VER_MINOR 2
#define LDG_NET_DIAG_ECHO_DATA "ldg-diag"
#define LDG_NET_DIAG_ECHO_DATA_LEN 8
#define LDG_NET_DIAG_REPLY_SLACK 64
#define LDG_NET_DIAG_PARAMS_CAP 8192
#define LDG_NET_DIAG_TIMEOUT_MAX 60000
#define LDG_NET_DIAG_US_PER_MS 1000

static uint32_t net_diag_resolve(const char *host, IPAddr *out)
{
    struct addrinfo hints = LDG_STRUCT_ZERO_INIT;
    struct addrinfo *res = 0x0;
    struct sockaddr_in addr = LDG_STRUCT_ZERO_INIT;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(host, 0x0, &hints, &res) != 0 || !res) { return LDG_ERR_NET_DNS; }

    if (res->ai_addrlen < sizeof(struct sockaddr_in))
    {
        freeaddrinfo(res);
        return LDG_ERR_NET_DNS;
    }

    addr = *(const struct sockaddr_in *)(const void *)res->ai_addr;
    freeaddrinfo(res);

    *out = addr.sin_addr.S_un.S_addr;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_diag_ping(const char *host, uint64_t timeout, uint64_t *out_rtt_us)
{
    uint8_t reply[sizeof(ICMP_ECHO_REPLY) + LDG_NET_DIAG_ECHO_DATA_LEN + LDG_NET_DIAG_REPLY_SLACK] LDG_ALIGNED = LDG_ARR_ZERO_INIT;
    char data[] = LDG_NET_DIAG_ECHO_DATA;
    const ICMP_ECHO_REPLY *echo = 0x0;
    WSADATA wsa = LDG_STRUCT_ZERO_INIT;
    HANDLE icmp = INVALID_HANDLE_VALUE;
    IPAddr ip = 0;
    DWORD replies = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!host || !out_rtt_us)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_rtt_us = 0;

    if (LDG_UNLIKELY(timeout == 0 || timeout > LDG_NET_DIAG_TIMEOUT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (WSAStartup(MAKEWORD(LDG_NET_DIAG_WSA_VER_MAJOR, LDG_NET_DIAG_WSA_VER_MINOR), &wsa) != 0) { return LDG_ERR_NET_INIT; }

    ret = net_diag_resolve(host, &ip);

    if (ret == LDG_ERR_AOK)
    {
        icmp = IcmpCreateFile();
        if (icmp == INVALID_HANDLE_VALUE) { ret = LDG_ERR_DENIED; }
    }

    if (ret == LDG_ERR_AOK)
    {
        replies = IcmpSendEcho(icmp, ip, data, LDG_NET_DIAG_ECHO_DATA_LEN, 0x0, reply, (DWORD)sizeof(reply), (DWORD)timeout);
        if (replies == 0) { ret = (GetLastError() == IP_REQ_TIMED_OUT) ? LDG_ERR_TIMEOUT : LDG_ERR_NET_SEND; }
    }

    if (ret == LDG_ERR_AOK)
    {
        echo = (const ICMP_ECHO_REPLY *)(const void *)reply;

        if (echo->Status == IP_SUCCESS) { *out_rtt_us = (uint64_t)echo->RoundTripTime * LDG_NET_DIAG_US_PER_MS; }
        else if (echo->Status == IP_REQ_TIMED_OUT) { ret = LDG_ERR_TIMEOUT; }
        else { ret = LDG_ERR_NET_UNREACHABLE; }
    }

    if (icmp != INVALID_HANDLE_VALUE && !IcmpCloseHandle(icmp) && ret == LDG_ERR_AOK) { ret = LDG_ERR_IO_CLOSE; }

    if (WSACleanup() != 0 && ret == LDG_ERR_AOK) { ret = LDG_ERR_NET_INIT; }

    return ret;
}

uint32_t ldg_net_diag_dns_list(ldg_net_diag_addr_t *out, uint64_t cap, uint64_t *out_cunt)
{
    uint8_t blk[LDG_NET_DIAG_PARAMS_CAP] LDG_ALIGNED = LDG_ARR_ZERO_INIT;
    const FIXED_INFO *info = (const FIXED_INFO *)(const void *)blk;
    const IP_ADDR_STRING *node = 0x0;
    ULONG len = (ULONG)sizeof(blk);
    DWORD code = 0;

    if (LDG_UNLIKELY(!out || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;

    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    code = GetNetworkParams((FIXED_INFO *)(void *)blk, &len);
    if (code == ERROR_BUFFER_OVERFLOW) { return LDG_ERR_OVERFLOW; }

    if (code != ERROR_SUCCESS) { return LDG_ERR_NOT_FOUND; }

    for (node = &info->DnsServerList; node && *out_cunt < cap; node = node->Next)
    {
        if (node->IpAddress.String[0] == LDG_STR_TERM) { continue; }

        if (ldg_str_rbrcpy(out[*out_cunt].text, node->IpAddress.String, LDG_NET_DIAG_ADDR_CAP) != LDG_ERR_AOK) { continue; }

        (*out_cunt)++;
    }

    if (*out_cunt == 0) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}
