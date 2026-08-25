#ifndef LDG_NET_STEAMWORKS_SRV_BROWSER_H
#define LDG_NET_STEAMWORKS_SRV_BROWSER_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/net/net.h>

typedef struct ldg_net_steamworks_srv_resp
{
    void **vtable;
    void *vtable_slot[3];
    void *ctx;
} ldg_net_steamworks_srv_resp_t;

typedef struct ldg_net_steamworks_srv_browser_ctx
{
    ldg_net_srv_browser_ctx_t browser;
    ldg_net_steamworks_srv_resp_t resp;
    uint8_t pudding[24];
} LDG_ALIGNED ldg_net_steamworks_srv_browser_ctx_t;

LDG_EXPORT uint32_t ldg_net_steamworks_srv_browser_init(ldg_net_steamworks_srv_browser_ctx_t *ctx);
LDG_EXPORT void ldg_net_steamworks_srv_browser_shutdown(ldg_net_steamworks_srv_browser_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_srv_browser_lan_refresh(ldg_net_steamworks_srv_browser_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_srv_browser_internet_refresh(ldg_net_steamworks_srv_browser_ctx_t *ctx);
LDG_EXPORT void ldg_net_steamworks_srv_browser_cancel(ldg_net_steamworks_srv_browser_ctx_t *ctx);
LDG_EXPORT void ldg_net_steamworks_srv_browser_poll(ldg_net_steamworks_srv_browser_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_srv_browser_cunt_get(ldg_net_steamworks_srv_browser_ctx_t *ctx);
LDG_EXPORT const ldg_net_srv_browser_entry_t* ldg_net_steamworks_srv_browser_entry_get(ldg_net_steamworks_srv_browser_ctx_t *ctx, uint32_t idx);

#endif
