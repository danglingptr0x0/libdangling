#ifndef LDG_NET_STEAMWORKS_TKT_H
#define LDG_NET_STEAMWORKS_TKT_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_NET_STEAMWORKS_TKT_MAX 1024
#define LDG_NET_STEAMWORKS_WEB_TKT_MAX 2560

typedef enum ldg_net_steamworks_tkt_state
{
    LDG_NET_STEAMWORKS_TKT_STATE_NONE = 0,
    LDG_NET_STEAMWORKS_TKT_STATE_REQ,
    LDG_NET_STEAMWORKS_TKT_STATE_READY,
    LDG_NET_STEAMWORKS_TKT_STATE_ERR
} ldg_net_steamworks_tkt_state_t;

typedef struct ldg_net_steamworks_tkt_ctx
{
    uint8_t enc[LDG_NET_STEAMWORKS_TKT_MAX];
    uint32_t enc_len;
    uint8_t pudding0[4];
    uint64_t api_call;
    ldg_net_steamworks_tkt_state_t state;
    int32_t steam_pipe;
    uint8_t web[LDG_NET_STEAMWORKS_WEB_TKT_MAX];
    uint32_t web_len;
    uint32_t web_handle;
    ldg_net_steamworks_tkt_state_t web_state;
    uint8_t pudding[28];
} LDG_ALIGNED ldg_net_steamworks_tkt_ctx_t;

LDG_EXPORT uint32_t ldg_net_steamworks_tkt_init(ldg_net_steamworks_tkt_ctx_t *ctx);
LDG_EXPORT void ldg_net_steamworks_tkt_shutdown(ldg_net_steamworks_tkt_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_tkt_req(ldg_net_steamworks_tkt_ctx_t *ctx, const void *data, uint32_t data_len);
LDG_EXPORT void ldg_net_steamworks_tkt_call_handle(ldg_net_steamworks_tkt_ctx_t *ctx, int32_t pipe, uint64_t call);
LDG_EXPORT uint32_t ldg_net_steamworks_tkt_enc_get(ldg_net_steamworks_tkt_ctx_t *ctx, uint8_t *out, uint64_t *out_len);
LDG_EXPORT ldg_net_steamworks_tkt_state_t ldg_net_steamworks_tkt_state_get(const ldg_net_steamworks_tkt_ctx_t *ctx);

LDG_EXPORT uint32_t ldg_net_steamworks_tkt_web_req(ldg_net_steamworks_tkt_ctx_t *ctx, const char *ident);
LDG_EXPORT void ldg_net_steamworks_tkt_cb_handle(ldg_net_steamworks_tkt_ctx_t *ctx, int32_t cb_id, void *param, int32_t param_size);
LDG_EXPORT uint32_t ldg_net_steamworks_tkt_web_get(ldg_net_steamworks_tkt_ctx_t *ctx, uint8_t *out, uint64_t *out_len);
LDG_EXPORT void ldg_net_steamworks_tkt_web_cancel(ldg_net_steamworks_tkt_ctx_t *ctx);
LDG_EXPORT ldg_net_steamworks_tkt_state_t ldg_net_steamworks_tkt_web_state_get(const ldg_net_steamworks_tkt_ctx_t *ctx);

#endif
