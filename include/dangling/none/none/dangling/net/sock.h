#ifndef LDG_NET_SOCK_H
#define LDG_NET_SOCK_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/core/types.h>

#define LDG_NET_SOCK_HANDLE_NONE UINT64_MAX
#define LDG_NET_SOCK_TIMEOUT_DEFAULT 5000
#define LDG_NET_SOCK_PORT_STR_CAP 8
#define LDG_NET_SOCK_WAIT_MAX 64

typedef struct ldg_net_sock
{
    uint64_t handle;
    uint64_t wr_total;
    uint64_t rd_total;
    uint8_t live;
    uint8_t listening;
    uint8_t pudding[6];
} ldg_net_sock_t;

LDG_EXPORT uint32_t ldg_net_sock_init(void);
LDG_EXPORT uint32_t ldg_net_sock_shutdown(void);

LDG_EXPORT uint32_t ldg_net_sock_open(const char *host, uint16_t port, uint64_t timeout, ldg_net_sock_t *out);
LDG_EXPORT uint32_t ldg_net_sock_close(ldg_net_sock_t *sock, uint64_t timeout);
LDG_EXPORT uint32_t ldg_net_sock_wr(ldg_net_sock_t *sock, const ldg_byte_t *src, uint64_t len, uint64_t timeout, uint64_t *bytes_wr);
LDG_EXPORT uint32_t ldg_net_sock_rd(ldg_net_sock_t *sock, ldg_byte_t *dst, uint64_t cap, uint64_t timeout, uint64_t *bytes_rd);
LDG_EXPORT uint8_t ldg_net_sock_live_is(const ldg_net_sock_t *sock);
LDG_EXPORT uint32_t ldg_net_sock_listen(const char *addr, uint16_t port, ldg_net_sock_t *out);
LDG_EXPORT uint32_t ldg_net_sock_accept(ldg_net_sock_t *listener, uint64_t timeout, ldg_net_sock_t *out);
LDG_EXPORT uint32_t ldg_net_sock_wait(const ldg_net_sock_t *sock, uint32_t cunt, uint64_t timeout, uint8_t *ready);

#endif
