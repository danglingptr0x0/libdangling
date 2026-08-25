#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/mem/secure.h>
#include <dangling/net/net.h>
#include <dangling/net/steamworks_srv_browser.h>

typedef struct srv_net_adr
{
    uint16_t conn_port;
    uint16_t query_port;
    uint32_t ip;
} srv_net_adr_t;

typedef struct gameserveritem
{
    srv_net_adr_t net_adr;
    int32_t ping;
    uint8_t had_resp;
    uint8_t do_not_refresh;
    char game_dir[32];
    char map[32];
    char game_desc[64];
    uint16_t pudding0;
    uint32_t app_id;
    int32_t player_cunt;
    int32_t player_max;
    int32_t bot_cunt;
    uint8_t password;
    uint8_t secure;
    uint16_t pudding1;
    uint32_t time_last_played;
    int32_t server_version;
    char server_name[64];
    char game_tag[128];
    uint8_t pudding2[4];
    uint64_t steam_id;
} gameserveritem_t;

#define STEAMWORKS_LAYOUT_CK(name, cond) typedef char steam_layout_ck_ ## name[(cond) ? 1 : -1]

STEAMWORKS_LAYOUT_CK(gameserveritem_steam_id, offsetof(gameserveritem_t, steam_id) == 368);
STEAMWORKS_LAYOUT_CK(gameserveritem_size, sizeof(gameserveritem_t) == 376);

extern void* SteamAPI_SteamMatchmakingServers_v002(void);
extern void* SteamAPI_ISteamMatchmakingServers_RequestLANServerList(void *self, uint32_t app_id, void *resp);
extern void* SteamAPI_ISteamMatchmakingServers_RequestInternetServerList(void *self, uint32_t app_id, void **filter, uint32_t filter_cunt, void *resp);
extern void SteamAPI_ISteamMatchmakingServers_ReleaseRequest(void *self, void *req);
extern void SteamAPI_ISteamMatchmakingServers_CancelQuery(void *self, void *req);
extern int32_t SteamAPI_ISteamMatchmakingServers_GetServerCount(void *self, void *req);
extern void* SteamAPI_ISteamMatchmakingServers_GetServerDetails(void *self, void *req, int32_t idx);
extern void* SteamAPI_SteamUtils_v010(void);
extern uint32_t SteamAPI_ISteamUtils_GetAppID(void *self);

typedef void (*srv_resp_cb_t)(ldg_net_steamworks_srv_resp_t *, void *, int32_t);

typedef union vtable_entry
{
    srv_resp_cb_t cb;
    void *ptr;
} vtable_entry_t;

static void on_server_responded_cb(ldg_net_steamworks_srv_resp_t *self, void *req, int32_t idx)
{
    ldg_net_steamworks_srv_browser_ctx_t *ctx = 0x0;
    gameserveritem_t *item = 0x0;
    ldg_net_srv_browser_entry_t *entry = 0x0;
    uint32_t zero_err = 0;
    uint32_t name_err = 0;
    uint32_t map_err = 0;
    uint32_t tag_err = 0;

    if (LDG_UNLIKELY(!self || !self->ctx)) { return; }

    ctx = self->ctx;

    if (LDG_UNLIKELY(!ctx->browser.matchmaking_srv)) { return; }

    if (ctx->browser.entry_cunt >= LDG_NET_SRV_BROWSER_MAX)
    {
        LDG_ERRLOG_INFO("srv_browser: entry max reached");
        return;
    }

    item = (gameserveritem_t *)SteamAPI_ISteamMatchmakingServers_GetServerDetails(ctx->browser.matchmaking_srv, req, idx);

    if (LDG_UNLIKELY(!item))
    {
        LDG_ERRLOG_WARN("srv_browser: get server details returned null");
        return;
    }

    entry = &ctx->browser.entry[ctx->browser.entry_cunt];
    zero_err = ldg_mem_secure_zero(entry, sizeof(ldg_net_srv_browser_entry_t));
    if (LDG_UNLIKELY(zero_err != LDG_ERR_AOK))
    {
        LDG_ERRLOG_WARN("srv_browser: entry zero failed");
        return;
    }

    entry->ip = item->net_adr.ip;
    entry->port = item->net_adr.conn_port;
    entry->query_port = item->net_adr.query_port;
    entry->ping = item->ping;
    entry->client_cunt = item->player_cunt;
    entry->client_max = item->player_max;
    entry->bot_cunt = item->bot_cunt;
    entry->server_id = item->steam_id;
    entry->password = item->password;
    entry->secure = item->secure;
    entry->dedicated = 1;

    name_err = ldg_str_rbrcpy(entry->name, item->server_name, sizeof(entry->name));
    if (LDG_UNLIKELY(name_err != LDG_ERR_AOK && name_err != LDG_ERR_STR_TRUNC)) { LDG_ERRLOG_WARN("srv_browser: name copy failed"); }

    map_err = ldg_str_rbrcpy(entry->map, item->map, sizeof(entry->map));
    if (LDG_UNLIKELY(map_err != LDG_ERR_AOK && map_err != LDG_ERR_STR_TRUNC)) { LDG_ERRLOG_WARN("srv_browser: map copy failed"); }

    tag_err = ldg_str_rbrcpy(entry->tag, item->game_tag, sizeof(entry->tag));
    if (LDG_UNLIKELY(tag_err != LDG_ERR_AOK && tag_err != LDG_ERR_STR_TRUNC)) { LDG_ERRLOG_WARN("srv_browser: tag copy failed"); }

    ctx->browser.entry_cunt = ctx->browser.entry_cunt + 1;

    LDG_ERRLOG_INFO("srv_browser: server responded");
}

static void on_server_failed_cb(ldg_net_steamworks_srv_resp_t *self, void *req, int32_t idx)
{
    ldg_net_steamworks_srv_browser_ctx_t *ctx = 0x0;
    gameserveritem_t *item = 0x0;

    if (LDG_UNLIKELY(!self || !self->ctx)) { return; }

    ctx = self->ctx;

    if (LDG_UNLIKELY(!ctx->browser.matchmaking_srv)) { return; }

    item = (gameserveritem_t *)SteamAPI_ISteamMatchmakingServers_GetServerDetails(ctx->browser.matchmaking_srv, req, idx);
    if (LDG_UNLIKELY(!item))
    {
        LDG_ERRLOG_WARN("srv_browser: failed server details null");
        return;
    }

    fprintf(stderr, "srv_browser: server failed to respond; idx: %" PRId32 "; addr: %" PRIu32 "\n", idx, item->net_adr.ip);
}

static void on_refresh_complete_cb(ldg_net_steamworks_srv_resp_t *self, void *req, int32_t resp_code)
{
    ldg_net_steamworks_srv_browser_ctx_t *ctx = 0x0;
    int32_t total = 0;

    if (LDG_UNLIKELY(!self || !self->ctx)) { return; }

    ctx = self->ctx;

    if (resp_code == 0)
    {
        ctx->browser.state = LDG_NET_SRV_BROWSER_STATE_DONE;
        if (ctx->browser.matchmaking_srv != 0x0) { total = SteamAPI_ISteamMatchmakingServers_GetServerCount(ctx->browser.matchmaking_srv, req); }

        fprintf(stderr, "srv_browser: refresh complete; queried: %" PRId32 "; entry cunt: %" PRIu32 "\n", total, ctx->browser.entry_cunt);
    }
    else if (resp_code == 2)
    {
        ctx->browser.state = LDG_NET_SRV_BROWSER_STATE_DONE;
        LDG_ERRLOG_INFO("srv_browser: no servers found on master");
    }
    else
    {
        ctx->browser.state = LDG_NET_SRV_BROWSER_STATE_ERR;
        LDG_ERRLOG_WARN("srv_browser: could not refresh");
    }
}

static uint32_t app_id_get(void)
{
    void *utils = 0x0;

    utils = SteamAPI_SteamUtils_v010();
    if (LDG_UNLIKELY(!utils)) { return 0; }

    return SteamAPI_ISteamUtils_GetAppID(utils);
}

uint32_t ldg_net_steamworks_srv_browser_init(ldg_net_steamworks_srv_browser_ctx_t *ctx)
{
    uint32_t zero_err = 0;
    vtable_entry_t entry[3] = {
        { .cb = on_server_responded_cb },
        { .cb = on_server_failed_cb },
        { .cb = on_refresh_complete_cb }
    };

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    zero_err = ldg_mem_secure_zero(ctx, sizeof(ldg_net_steamworks_srv_browser_ctx_t));
    if (LDG_UNLIKELY(zero_err != LDG_ERR_AOK)) { return zero_err; }

    ctx->resp.vtable_slot[0] = entry[0].ptr;
    ctx->resp.vtable_slot[1] = entry[1].ptr;
    ctx->resp.vtable_slot[2] = entry[2].ptr;
    ctx->resp.vtable = ctx->resp.vtable_slot;
    ctx->resp.ctx = ctx;

    ctx->browser.matchmaking_srv = SteamAPI_SteamMatchmakingServers_v002();
    if (LDG_UNLIKELY(!ctx->browser.matchmaking_srv))
    {
        LDG_ERRLOG_ERR("srv_browser: could not get matchmaking srv iface");
        return LDG_ERR_NET_RPC_FAIL;
    }

    ctx->browser.state = LDG_NET_SRV_BROWSER_STATE_NONE;
    ctx->browser.is_init = 1;

    LDG_ERRLOG_INFO("srv_browser: init");

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_srv_browser_shutdown(ldg_net_steamworks_srv_browser_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->browser.is_init)) { return; }

    if (ctx->browser.req_handle) { ldg_net_steamworks_srv_browser_cancel(ctx); }

    ctx->browser.is_init = 0;

    LDG_ERRLOG_INFO("srv_browser: shutdown");
}

uint32_t ldg_net_steamworks_srv_browser_lan_refresh(ldg_net_steamworks_srv_browser_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->browser.is_init)) { return LDG_ERR_NOT_INIT; }

    if (ctx->browser.req_handle) { ldg_net_steamworks_srv_browser_cancel(ctx); }

    ctx->browser.entry_cunt = 0;
    ctx->browser.state = LDG_NET_SRV_BROWSER_STATE_REFRESHING;

    ctx->browser.req_handle = SteamAPI_ISteamMatchmakingServers_RequestLANServerList(ctx->browser.matchmaking_srv, app_id_get(), &ctx->resp);

    if (LDG_UNLIKELY(!ctx->browser.req_handle))
    {
        LDG_ERRLOG_ERR("srv_browser: request LAN server list failed");
        ctx->browser.state = LDG_NET_SRV_BROWSER_STATE_ERR;
        return LDG_ERR_NET_RPC_FAIL;
    }

    LDG_ERRLOG_INFO("srv_browser: requesting LAN server list");

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_srv_browser_internet_refresh(ldg_net_steamworks_srv_browser_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->browser.is_init)) { return LDG_ERR_NOT_INIT; }

    if (ctx->browser.req_handle) { ldg_net_steamworks_srv_browser_cancel(ctx); }

    ctx->browser.entry_cunt = 0;
    ctx->browser.state = LDG_NET_SRV_BROWSER_STATE_REFRESHING;

    ctx->browser.req_handle = SteamAPI_ISteamMatchmakingServers_RequestInternetServerList(ctx->browser.matchmaking_srv, app_id_get(), 0x0, 0, &ctx->resp);

    if (LDG_UNLIKELY(!ctx->browser.req_handle))
    {
        LDG_ERRLOG_ERR("srv_browser: request internet server list failed");
        ctx->browser.state = LDG_NET_SRV_BROWSER_STATE_ERR;
        return LDG_ERR_NET_RPC_FAIL;
    }

    LDG_ERRLOG_INFO("srv_browser: requesting internet srv list");

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_srv_browser_cancel(ldg_net_steamworks_srv_browser_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->browser.is_init || !ctx->browser.matchmaking_srv)) { return; }

    if (ctx->browser.req_handle)
    {
        SteamAPI_ISteamMatchmakingServers_CancelQuery(ctx->browser.matchmaking_srv, ctx->browser.req_handle);
        SteamAPI_ISteamMatchmakingServers_ReleaseRequest(ctx->browser.matchmaking_srv, ctx->browser.req_handle);
        ctx->browser.req_handle = 0x0;
    }

    ctx->browser.state = LDG_NET_SRV_BROWSER_STATE_NONE;
}

void ldg_net_steamworks_srv_browser_poll(ldg_net_steamworks_srv_browser_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->browser.is_init)) { return; }
}

uint32_t ldg_net_steamworks_srv_browser_cunt_get(ldg_net_steamworks_srv_browser_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->browser.is_init)) { return 0; }

    return ctx->browser.entry_cunt;
}

const ldg_net_srv_browser_entry_t* ldg_net_steamworks_srv_browser_entry_get(ldg_net_steamworks_srv_browser_ctx_t *ctx, uint32_t idx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->browser.is_init || idx >= ctx->browser.entry_cunt)) { return 0x0; }

    return &ctx->browser.entry[idx];
}
