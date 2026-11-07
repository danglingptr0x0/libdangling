#ifndef LDG_NET_DIAG_H
#define LDG_NET_DIAG_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_NET_DIAG_ADDR_CAP 64

typedef struct ldg_net_diag_addr
{
    char text[LDG_NET_DIAG_ADDR_CAP];
} ldg_net_diag_addr_t;

LDG_EXPORT uint32_t ldg_net_diag_ping(const char *host, uint64_t timeout, uint64_t *out_rtt_us);
LDG_EXPORT uint32_t ldg_net_diag_dns_list(ldg_net_diag_addr_t *out, uint64_t cap, uint64_t *out_cunt);

#endif
