#include <stdint.h>
#include <string.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/secure.h>
#include <dangling/net/steamworks_tkt.h>

#define STEAM_CB_ENC_APP_TKT 154
#define STEAM_CB_WEB_TKT 168
#define STEAM_RES_OK 1

typedef struct tkt_resp
{
    int32_t res;
} LDG_PACKED tkt_resp_t;

typedef struct tkt_web_resp
{
    uint32_t handle;
    int32_t res;
    int32_t tkt_len;
    uint8_t tkt[LDG_NET_STEAMWORKS_WEB_TKT_MAX];
} LDG_PACKED tkt_web_resp_t;

#define STEAMWORKS_LAYOUT_CK(name, cond) typedef char steam_layout_ck_ ## name[(cond) ? 1 : -1]

STEAMWORKS_LAYOUT_CK(tkt_resp_size, sizeof(tkt_resp_t) == 4);
STEAMWORKS_LAYOUT_CK(tkt_web_resp_size, sizeof(tkt_web_resp_t) == 2572);

extern void* SteamAPI_SteamUser_v023(void);
extern uint64_t SteamAPI_ISteamUser_RequestEncryptedAppTicket(void *self, void *data, int32_t data_len);
extern uint8_t SteamAPI_ISteamUser_GetEncryptedAppTicket(void *self, void *tkt, int32_t max_len, uint32_t *out_len);
extern uint32_t SteamAPI_ISteamUser_GetAuthTicketForWebApi(void *self, const char *ident);
extern void SteamAPI_ISteamUser_CancelAuthTicket(void *self, uint32_t handle);
extern uint8_t SteamAPI_ManualDispatch_GetAPICallResult(int32_t pipe, uint64_t call, void *cb, int32_t cb_size, int32_t cb_expected, uint8_t *failed);
extern int32_t SteamAPI_GetHSteamPipe(void);

uint32_t ldg_net_steamworks_tkt_init(ldg_net_steamworks_tkt_ctx_t *ctx)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(ctx, sizeof(ldg_net_steamworks_tkt_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ctx->state = LDG_NET_STEAMWORKS_TKT_STATE_NONE;
    ctx->steam_pipe = SteamAPI_GetHSteamPipe();

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_tkt_shutdown(ldg_net_steamworks_tkt_ctx_t *ctx)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!ctx)) { return; }

    err = ldg_mem_secure_zero(ctx, sizeof(ldg_net_steamworks_tkt_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { LDG_ERRLOG_ERR("net: tkt ctx zero failed"); }
}

uint32_t ldg_net_steamworks_tkt_req(ldg_net_steamworks_tkt_ctx_t *ctx, const void *data, uint32_t data_len)
{
    void *user = 0x0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(data_len > 0 && !data)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->state == LDG_NET_STEAMWORKS_TKT_STATE_REQ) { return LDG_ERR_BUSY; }

    user = SteamAPI_SteamUser_v023();
    if (LDG_UNLIKELY(!user)) { return LDG_ERR_NOT_INIT; }

    ctx->api_call = SteamAPI_ISteamUser_RequestEncryptedAppTicket(user, (void *)data, (int32_t)data_len);
    if (ctx->api_call == 0) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->state = LDG_NET_STEAMWORKS_TKT_STATE_REQ;
    return LDG_ERR_AOK;
}

void ldg_net_steamworks_tkt_call_handle(ldg_net_steamworks_tkt_ctx_t *ctx, int32_t pipe, uint64_t call)
{
    void *user = 0x0;
    uint8_t failed = 0;
    tkt_resp_t resp = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!ctx)) { return; }

    if (ctx->state != LDG_NET_STEAMWORKS_TKT_STATE_REQ) { return; }

    if (ctx->api_call == 0 || call != ctx->api_call) { return; }

    if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, ctx->api_call, &resp, (int32_t)sizeof(resp), STEAM_CB_ENC_APP_TKT, &failed) == 0) { return; }

    ctx->api_call = 0;
    if (failed != 0 || resp.res != STEAM_RES_OK)
    {
        ctx->state = LDG_NET_STEAMWORKS_TKT_STATE_ERR;
        return;
    }

    user = SteamAPI_SteamUser_v023();
    if (LDG_UNLIKELY(!user))
    {
        ctx->state = LDG_NET_STEAMWORKS_TKT_STATE_ERR;
        return;
    }

    ctx->enc_len = 0;
    if (SteamAPI_ISteamUser_GetEncryptedAppTicket(user, ctx->enc, LDG_NET_STEAMWORKS_TKT_MAX, &ctx->enc_len) == 0)
    {
        ctx->state = LDG_NET_STEAMWORKS_TKT_STATE_ERR;
        return;
    }

    ctx->state = LDG_NET_STEAMWORKS_TKT_STATE_READY;
}

uint32_t ldg_net_steamworks_tkt_enc_get(ldg_net_steamworks_tkt_ctx_t *ctx, uint8_t *out, uint64_t *out_len)
{
    uint32_t copy_err = 0;

    if (LDG_UNLIKELY(!ctx || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->state != LDG_NET_STEAMWORKS_TKT_STATE_READY) { return LDG_ERR_INVALID; }

    if (*out_len < ctx->enc_len)
    {
        *out_len = ctx->enc_len;
        return LDG_ERR_FULL;
    }

    copy_err = ldg_mem_secure_cpy(out, ctx->enc, ctx->enc_len);
    if (LDG_UNLIKELY(copy_err != LDG_ERR_AOK)) { return copy_err; }

    *out_len = ctx->enc_len;
    return LDG_ERR_AOK;
}

ldg_net_steamworks_tkt_state_t ldg_net_steamworks_tkt_state_get(const ldg_net_steamworks_tkt_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_NET_STEAMWORKS_TKT_STATE_NONE; }

    return ctx->state;
}

uint32_t ldg_net_steamworks_tkt_web_req(ldg_net_steamworks_tkt_ctx_t *ctx, const char *ident)
{
    void *user = 0x0;
    uint32_t handle = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->web_state == LDG_NET_STEAMWORKS_TKT_STATE_REQ) { return LDG_ERR_BUSY; }

    user = SteamAPI_SteamUser_v023();
    if (LDG_UNLIKELY(!user)) { return LDG_ERR_NOT_INIT; }

    handle = SteamAPI_ISteamUser_GetAuthTicketForWebApi(user, ident);
    if (handle == 0) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->web_handle = handle;
    ctx->web_len = 0;
    ctx->web_state = LDG_NET_STEAMWORKS_TKT_STATE_REQ;

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_tkt_cb_handle(ldg_net_steamworks_tkt_ctx_t *ctx, int32_t cb_id, void *param, int32_t param_size)
{
    tkt_web_resp_t *resp = 0x0;
    uint32_t copy_err = 0;

    if (LDG_UNLIKELY(!ctx)) { return; }

    if (cb_id != STEAM_CB_WEB_TKT) { return; }

    if (param == 0x0 || param_size < (int32_t)sizeof(tkt_web_resp_t)) { return; }

    if (ctx->web_state != LDG_NET_STEAMWORKS_TKT_STATE_REQ) { return; }

    resp = (tkt_web_resp_t *)param;
    if (resp->handle != ctx->web_handle) { return; }

    if (resp->res != STEAM_RES_OK || resp->tkt_len <= 0 || resp->tkt_len > (int32_t)LDG_NET_STEAMWORKS_WEB_TKT_MAX)
    {
        ctx->web_state = LDG_NET_STEAMWORKS_TKT_STATE_ERR;
        return;
    }

    copy_err = ldg_mem_secure_cpy(ctx->web, resp->tkt, (uint64_t)resp->tkt_len);
    if (LDG_UNLIKELY(copy_err != LDG_ERR_AOK))
    {
        ctx->web_state = LDG_NET_STEAMWORKS_TKT_STATE_ERR;
        return;
    }

    ctx->web_len = (uint32_t)resp->tkt_len;
    ctx->web_state = LDG_NET_STEAMWORKS_TKT_STATE_READY;
}

uint32_t ldg_net_steamworks_tkt_web_get(ldg_net_steamworks_tkt_ctx_t *ctx, uint8_t *out, uint64_t *out_len)
{
    uint32_t copy_err = 0;

    if (LDG_UNLIKELY(!ctx || !out || !out_len)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->web_state != LDG_NET_STEAMWORKS_TKT_STATE_READY) { return LDG_ERR_INVALID; }

    if (*out_len < ctx->web_len)
    {
        *out_len = ctx->web_len;
        return LDG_ERR_FULL;
    }

    copy_err = ldg_mem_secure_cpy(out, ctx->web, ctx->web_len);
    if (LDG_UNLIKELY(copy_err != LDG_ERR_AOK)) { return copy_err; }

    *out_len = ctx->web_len;
    return LDG_ERR_AOK;
}

void ldg_net_steamworks_tkt_web_cancel(ldg_net_steamworks_tkt_ctx_t *ctx)
{
    void *user = 0x0;

    if (LDG_UNLIKELY(!ctx)) { return; }

    if (ctx->web_handle != 0)
    {
        user = SteamAPI_SteamUser_v023();
        if (user) { SteamAPI_ISteamUser_CancelAuthTicket(user, ctx->web_handle); }
    }

    ctx->web_handle = 0;
    ctx->web_len = 0;
    ctx->web_state = LDG_NET_STEAMWORKS_TKT_STATE_NONE;
}

ldg_net_steamworks_tkt_state_t ldg_net_steamworks_tkt_web_state_get(const ldg_net_steamworks_tkt_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_NET_STEAMWORKS_TKT_STATE_NONE; }

    return ctx->web_state;
}
