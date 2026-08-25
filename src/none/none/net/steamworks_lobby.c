#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/mem/secure.h>
#include <dangling/net/net.h>
#include <dangling/net/steamworks_lobby.h>

#define STEAM_CB_FRIENDS 300
#define STEAM_CB_PERSONA_STATE_CHANGE (STEAM_CB_FRIENDS + 4)
#define STEAM_CB_MATCHMAKING 500
#define STEAM_CB_LOBBY_ENTER (STEAM_CB_MATCHMAKING + 4)
#define STEAM_CB_LOBBY_DATA_UPDATE (STEAM_CB_MATCHMAKING + 5)
#define STEAM_CB_LOBBY_CHAT_UPDATE (STEAM_CB_MATCHMAKING + 6)
#define STEAM_CB_LOBBY_GAME_CREATED (STEAM_CB_MATCHMAKING + 9)
#define STEAM_CB_LOBBY_MATCH_LIST (STEAM_CB_MATCHMAKING + 10)
#define STEAM_CB_LOBBY_CREATED (STEAM_CB_MATCHMAKING + 13)

#define STEAM_RES_OK 1
#define STEAM_LOBBY_ENTER_RESP_OK 1

typedef struct lobby_created
{
    int32_t res;
#ifdef _WIN32
    uint8_t pudding0[4];
#endif
    uint64_t lobby_id;
} LDG_PACKED lobby_created_t;

typedef struct lobby_match_list
{
    uint32_t cunt;
} LDG_PACKED lobby_match_list_t;

typedef struct lobby_enter
{
    uint64_t lobby_id;
    uint32_t chat_perms;
    uint8_t locked;
    uint8_t pudding0[3];
    uint32_t resp;
#ifdef _WIN32
    uint8_t pudding1[4];
#endif
} LDG_PACKED lobby_enter_t;

typedef struct lobby_data_update
{
    uint64_t lobby_id;
    uint64_t member_id;
    uint8_t success;
#ifdef _WIN32
    uint8_t pudding0[7];
#else
    uint8_t pudding0[3];
#endif
} LDG_PACKED lobby_data_update_t;

typedef struct lobby_chat_update
{
    uint64_t lobby_id;
    uint64_t user_changed;
    uint64_t user_making_change;
    uint32_t state_change;
#ifdef _WIN32
    uint8_t pudding0[4];
#endif
} LDG_PACKED lobby_chat_update_t;

typedef struct lobby_game_created
{
    uint64_t lobby_id;
    uint64_t srv_steam_id;
    uint32_t ip;
    uint16_t port;
    uint8_t pudding0[2];
} LDG_PACKED lobby_game_created_t;

#define STEAMWORKS_LAYOUT_CK(name, cond) typedef char steam_layout_ck_ ## name[(cond) ? 1 : -1]

#ifdef _WIN32
STEAMWORKS_LAYOUT_CK(lobby_created_size, sizeof(lobby_created_t) == 16);
STEAMWORKS_LAYOUT_CK(lobby_enter_size, sizeof(lobby_enter_t) == 24);
STEAMWORKS_LAYOUT_CK(lobby_data_update_size, sizeof(lobby_data_update_t) == 24);
STEAMWORKS_LAYOUT_CK(lobby_chat_update_size, sizeof(lobby_chat_update_t) == 32);
#else
STEAMWORKS_LAYOUT_CK(lobby_created_size, sizeof(lobby_created_t) == 12);
STEAMWORKS_LAYOUT_CK(lobby_enter_size, sizeof(lobby_enter_t) == 20);
STEAMWORKS_LAYOUT_CK(lobby_data_update_size, sizeof(lobby_data_update_t) == 20);
STEAMWORKS_LAYOUT_CK(lobby_chat_update_size, sizeof(lobby_chat_update_t) == 28);
#endif
STEAMWORKS_LAYOUT_CK(lobby_game_created_size, sizeof(lobby_game_created_t) == 24);

extern void* SteamAPI_SteamMatchmaking_v009(void);
extern void* SteamAPI_SteamFriends_v018(void);
extern void* SteamAPI_SteamUser_v023(void);
extern uint64_t SteamAPI_ISteamUser_GetSteamID(void *self);
extern uint64_t SteamAPI_ISteamMatchmaking_CreateLobby(void *self, int32_t type, int32_t max);
extern uint64_t SteamAPI_ISteamMatchmaking_RequestLobbyList(void *self);
extern void SteamAPI_ISteamMatchmaking_AddRequestLobbyListStringFilter(void *self, const char *key, const char *val, int32_t cmp);
extern void SteamAPI_ISteamMatchmaking_AddRequestLobbyListFilterSlotsAvailable(void *self, int32_t slot_cunt);
extern uint64_t SteamAPI_ISteamMatchmaking_JoinLobby(void *self, uint64_t lobby);
extern void SteamAPI_ISteamMatchmaking_LeaveLobby(void *self, uint64_t lobby);
extern int32_t SteamAPI_ISteamMatchmaking_GetNumLobbyMembers(void *self, uint64_t lobby);
extern int32_t SteamAPI_ISteamMatchmaking_GetLobbyMemberLimit(void *self, uint64_t lobby);
extern uint64_t SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex(void *self, uint64_t lobby, int32_t idx);
extern uint64_t SteamAPI_ISteamMatchmaking_GetLobbyByIndex(void *self, int32_t idx);
extern const char* SteamAPI_ISteamMatchmaking_GetLobbyData(void *self, uint64_t lobby, const char *key);
extern uint8_t SteamAPI_ISteamMatchmaking_SetLobbyData(void *self, uint64_t lobby, const char *key, const char *val);
extern const char* SteamAPI_ISteamMatchmaking_GetLobbyMemberData(void *self, uint64_t lobby, uint64_t user, const char *key);
extern void SteamAPI_ISteamMatchmaking_SetLobbyMemberData(void *self, uint64_t lobby, const char *key, const char *val);
extern void SteamAPI_ISteamMatchmaking_SetLobbyGameServer(void *self, uint64_t lobby, uint32_t ip, uint16_t port, uint64_t srv_id);
extern uint64_t SteamAPI_ISteamMatchmaking_GetLobbyOwner(void *self, uint64_t lobby);
extern const char* SteamAPI_ISteamFriends_GetFriendPersonaName(void *self, uint64_t steam_id);
extern uint8_t SteamAPI_ISteamFriends_RequestUserInformation(void *self, uint64_t steam_id, uint8_t name_only);
extern void SteamAPI_ManualDispatch_Init(void);
extern uint8_t SteamAPI_ManualDispatch_GetAPICallResult(int32_t pipe, uint64_t call, void *cb, int32_t cb_size, int32_t cb_expected, uint8_t *failed);
extern int32_t SteamAPI_GetHSteamPipe(void);

static void on_created(ldg_net_steamworks_lobby_ctx_t *ctx, lobby_created_t *cb)
{
    void *user = 0x0;
    uint8_t set_ok = 0;

    if (cb->res != STEAM_RES_OK)
    {
        LDG_ERRLOG_ERR("lobby: creation failed");
        ctx->lobby.state = LDG_NET_LOBBY_STATE_NONE;
        return;
    }

    ctx->lobby.curr_lobby = cb->lobby_id;
    ctx->lobby.is_host = 1;
    ctx->lobby.state = LDG_NET_LOBBY_STATE_HOSTING;

    user = SteamAPI_SteamUser_v023();
    if (user) { ctx->lobby.host_id = SteamAPI_ISteamUser_GetSteamID(user); }

    set_ok = SteamAPI_ISteamMatchmaking_SetLobbyData(ctx->lobby.matchmaking, cb->lobby_id, LDG_NET_LOBBY_KEY_MAP, ctx->map);
    if (LDG_UNLIKELY(set_ok == 0)) { LDG_ERRLOG_WARN("lobby: set map data failed"); }

    set_ok = SteamAPI_ISteamMatchmaking_SetLobbyData(ctx->lobby.matchmaking, cb->lobby_id, LDG_NET_LOBBY_KEY_REV, ctx->rev);
    if (LDG_UNLIKELY(set_ok == 0)) { LDG_ERRLOG_WARN("lobby: set rev data failed"); }

    LDG_ERRLOG_INFO("lobby: created lobby");
}

static void on_match_list(ldg_net_steamworks_lobby_ctx_t *ctx, lobby_match_list_t *cb)
{
    uint32_t cunt = 0;
    uint32_t iter = 0;

    ctx->lobby.list_cunt = 0;

    cunt = cb->cunt;
    if (cunt > LDG_NET_LOBBY_LIST_MAX) { cunt = LDG_NET_LOBBY_LIST_MAX; }

    for (iter = 0; iter < cunt; iter = iter + 1)
    {
        uint64_t lobby_id = 0;
        uint64_t owner_steam_id = 0;
        ldg_net_lobby_info_t *info = 0x0;
        const char *map = 0x0;
        const char *name = 0x0;
        uint32_t fallback_err = 0;
        int32_t member_cunt = 0;
        int32_t member_max = 0;

        lobby_id = SteamAPI_ISteamMatchmaking_GetLobbyByIndex(ctx->lobby.matchmaking, (int32_t)iter);
        if (lobby_id == 0) { continue; }

        info = &ctx->lobby.list[ctx->lobby.list_cunt];
        info->lobby_id = lobby_id;

        map = SteamAPI_ISteamMatchmaking_GetLobbyData(ctx->lobby.matchmaking, lobby_id, LDG_NET_LOBBY_KEY_MAP);
        if (!map || ldg_str_rbrcpy(info->map_name, map, sizeof(info->map_name)) != 0) { info->map_name[0] = LDG_STR_TERM; }

        owner_steam_id = SteamAPI_ISteamMatchmaking_GetLobbyOwner(ctx->lobby.matchmaking, lobby_id);
        name = SteamAPI_ISteamFriends_GetFriendPersonaName(ctx->lobby.friends, owner_steam_id);
        if (!name || ldg_str_rbrcpy(info->host_name, name, sizeof(info->host_name)) != 0)
        {
            fallback_err = ldg_str_rbrcpy(info->host_name, "Unknown", sizeof(info->host_name));
            if (LDG_UNLIKELY(fallback_err != LDG_ERR_AOK && fallback_err != LDG_ERR_STR_TRUNC)) { LDG_ERRLOG_WARN("lobby: host name fallback copy failed"); }
        }

        member_cunt = SteamAPI_ISteamMatchmaking_GetNumLobbyMembers(ctx->lobby.matchmaking, lobby_id);
        if (member_cunt < 0) { member_cunt = 0; }

        if (member_cunt > UINT8_MAX) { member_cunt = UINT8_MAX; }

        info->player_cunt = (uint8_t)member_cunt;

        member_max = SteamAPI_ISteamMatchmaking_GetLobbyMemberLimit(ctx->lobby.matchmaking, lobby_id);
        if (member_max < 0) { member_max = 0; }

        if (member_max > UINT8_MAX) { member_max = UINT8_MAX; }

        info->player_max = (uint8_t)member_max;

        ctx->lobby.list_cunt = ctx->lobby.list_cunt + 1;
    }

    ctx->lobby.state = LDG_NET_LOBBY_STATE_NONE;
    LDG_ERRLOG_INFO("lobby: found");
}

static void on_enter(ldg_net_steamworks_lobby_ctx_t *ctx, lobby_enter_t *cb)
{
    void *user = 0x0;
    uint64_t owner_steam_id = 0;
    uint64_t my_steam_id = 0;

    if (cb->resp != STEAM_LOBBY_ENTER_RESP_OK)
    {
        LDG_ERRLOG_ERR("lobby: couldn't join");
        ctx->lobby.state = LDG_NET_LOBBY_STATE_NONE;
        return;
    }

    ctx->lobby.curr_lobby = cb->lobby_id;

    owner_steam_id = SteamAPI_ISteamMatchmaking_GetLobbyOwner(ctx->lobby.matchmaking, cb->lobby_id);
    user = SteamAPI_SteamUser_v023();
    if (user) { my_steam_id = SteamAPI_ISteamUser_GetSteamID(user); }

    ctx->lobby.is_host = (owner_steam_id == my_steam_id) ? 1 : 0;
    ctx->lobby.host_id = owner_steam_id;

    if (!ctx->lobby.is_host)
    {
        ctx->lobby.state = LDG_NET_LOBBY_STATE_JOINED;
        SteamAPI_ISteamMatchmaking_SetLobbyMemberData(ctx->lobby.matchmaking, cb->lobby_id, LDG_NET_LOBBY_MEMBER_KEY_RDY, "0");
        SteamAPI_ISteamMatchmaking_SetLobbyMemberData(ctx->lobby.matchmaking, cb->lobby_id, LDG_NET_LOBBY_MEMBER_KEY_TEAM, "0");
    }

    ldg_net_steamworks_lobby_member_refresh(ctx);
    LDG_ERRLOG_INFO("lobby: entered lobby");
}

static void on_data_update(ldg_net_steamworks_lobby_ctx_t *ctx, lobby_data_update_t *cb)
{
    if (cb->lobby_id != ctx->lobby.curr_lobby) { return; }

    ldg_net_steamworks_lobby_member_refresh(ctx);
    LDG_ERRLOG_INFO("lobby: data updated");
}

static void on_chat_update(ldg_net_steamworks_lobby_ctx_t *ctx, lobby_chat_update_t *cb)
{
    if (cb->lobby_id != ctx->lobby.curr_lobby) { return; }

    ldg_net_steamworks_lobby_member_refresh(ctx);
    LDG_ERRLOG_INFO("lobby: chat update");
}

static void on_game_created(ldg_net_steamworks_lobby_ctx_t *ctx, lobby_game_created_t *cb)
{
    if (cb->lobby_id != ctx->lobby.curr_lobby) { return; }

    ctx->lobby.host_id = cb->srv_steam_id;
    ctx->lobby.state = LDG_NET_LOBBY_STATE_STARTING;

    LDG_ERRLOG_INFO("lobby: game server set");
}

uint32_t ldg_net_steamworks_lobby_init(ldg_net_steamworks_lobby_ctx_t *ctx, const char *map, const char *rev)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!ctx || !map || !rev)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(ctx, sizeof(ldg_net_steamworks_lobby_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_str_rbrcpy(ctx->map, map, sizeof(ctx->map));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    err = ldg_str_rbrcpy(ctx->rev, rev, sizeof(ctx->rev));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    ctx->lobby.matchmaking = SteamAPI_SteamMatchmaking_v009();
    if (LDG_UNLIKELY(!ctx->lobby.matchmaking))
    {
        LDG_ERRLOG_ERR("lobby: failed to get matchmaking interface");
        return LDG_ERR_NET_RPC_FAIL;
    }

    ctx->lobby.friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!ctx->lobby.friends))
    {
        LDG_ERRLOG_ERR("lobby: failed to get friends interface");
        return LDG_ERR_NET_RPC_FAIL;
    }

    SteamAPI_ManualDispatch_Init();
    ctx->steam_pipe = SteamAPI_GetHSteamPipe();

    ctx->lobby.state = LDG_NET_LOBBY_STATE_NONE;
    ctx->lobby.is_init = 1;

    LDG_ERRLOG_INFO("lobby: init");

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_lobby_shutdown(ldg_net_steamworks_lobby_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->lobby.is_init)) { return; }

    if (ctx->lobby.curr_lobby != 0) { ldg_net_steamworks_lobby_leave(ctx); }

    ctx->lobby.is_init = 0;

    LDG_ERRLOG_INFO("lobby: shutdown");
}

uint32_t ldg_net_steamworks_lobby_create(ldg_net_steamworks_lobby_ctx_t *ctx, uint32_t member_max)
{
    uint64_t call = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->lobby.is_init)) { return LDG_ERR_NOT_INIT; }

    if (ctx->lobby.state != LDG_NET_LOBBY_STATE_NONE)
    {
        LDG_ERRLOG_WARN("lobby: cannot create");
        return LDG_ERR_BUSY;
    }

    call = SteamAPI_ISteamMatchmaking_CreateLobby(ctx->lobby.matchmaking, LDG_NET_LOBBY_TYPE_PUBLIC, (int32_t)member_max);
    if (call == 0)
    {
        LDG_ERRLOG_ERR("lobby: CreateLobby call failed");
        return LDG_ERR_NET_RPC_FAIL;
    }

    ctx->lobby.pending_call = call;
    ctx->lobby.state = LDG_NET_LOBBY_STATE_CREATING;
    ctx->pending_type = LDG_NET_LOBBY_PENDING_CREATE;

    LDG_ERRLOG_INFO("lobby: creating lobby");

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_lobby_list_req(ldg_net_steamworks_lobby_ctx_t *ctx)
{
    uint64_t call = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->lobby.is_init)) { return LDG_ERR_NOT_INIT; }

    SteamAPI_ISteamMatchmaking_AddRequestLobbyListFilterSlotsAvailable(ctx->lobby.matchmaking, 1);
    SteamAPI_ISteamMatchmaking_AddRequestLobbyListStringFilter(ctx->lobby.matchmaking, LDG_NET_LOBBY_KEY_REV, ctx->rev, LDG_NET_LOBBY_CMP_EQUAL);

    call = SteamAPI_ISteamMatchmaking_RequestLobbyList(ctx->lobby.matchmaking);
    if (call == 0)
    {
        LDG_ERRLOG_ERR("lobby: RequestLobbyList call failed");
        return LDG_ERR_NET_RPC_FAIL;
    }

    ctx->lobby.pending_call = call;
    ctx->lobby.state = LDG_NET_LOBBY_STATE_BROWSING;
    ctx->lobby.list_cunt = 0;
    ctx->pending_type = LDG_NET_LOBBY_PENDING_LIST;

    LDG_ERRLOG_INFO("lobby: requesting lobby list");

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_lobby_join(ldg_net_steamworks_lobby_ctx_t *ctx, uint64_t lobby_id)
{
    uint64_t call = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->lobby.is_init)) { return LDG_ERR_NOT_INIT; }

    if (lobby_id == 0)
    {
        LDG_ERRLOG_ERR("lobby: invalid lobby id");
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    call = SteamAPI_ISteamMatchmaking_JoinLobby(ctx->lobby.matchmaking, lobby_id);
    if (call == 0)
    {
        LDG_ERRLOG_ERR("lobby: JoinLobby call failed");
        return LDG_ERR_NET_RPC_FAIL;
    }

    ctx->lobby.pending_call = call;
    ctx->lobby.state = LDG_NET_LOBBY_STATE_JOINING;
    ctx->pending_type = LDG_NET_LOBBY_PENDING_JOIN;

    LDG_ERRLOG_INFO("lobby: joining lobby");

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_lobby_leave(ldg_net_steamworks_lobby_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->lobby.is_init)) { return; }

    if (ctx->lobby.curr_lobby == 0) { return; }

    SteamAPI_ISteamMatchmaking_LeaveLobby(ctx->lobby.matchmaking, ctx->lobby.curr_lobby);

    LDG_ERRLOG_INFO("lobby: left lobby");

    ctx->lobby.curr_lobby = 0;
    ctx->lobby.is_host = 0;
    ctx->lobby.member_cunt = 0;
    ctx->lobby.state = LDG_NET_LOBBY_STATE_NONE;
}

void ldg_net_steamworks_lobby_rdy_set(ldg_net_steamworks_lobby_ctx_t *ctx, uint8_t rdy)
{
    if (LDG_UNLIKELY(!ctx || !ctx->lobby.is_init || ctx->lobby.curr_lobby == 0)) { return; }

    SteamAPI_ISteamMatchmaking_SetLobbyMemberData(ctx->lobby.matchmaking, ctx->lobby.curr_lobby, LDG_NET_LOBBY_MEMBER_KEY_RDY, rdy ? "1" : "0");

    LDG_ERRLOG_INFO("lobby: set rdy");
}

void ldg_net_steamworks_lobby_team_set(ldg_net_steamworks_lobby_ctx_t *ctx, uint8_t team)
{
    char team_str[4] = LDG_ARR_ZERO_INIT;
    int32_t written = 0;

    if (LDG_UNLIKELY(!ctx || !ctx->lobby.is_init || ctx->lobby.curr_lobby == 0)) { return; }

    written = snprintf(team_str, sizeof(team_str), "%u", team);
    if (LDG_UNLIKELY(written < 0))
    {
        LDG_ERRLOG_WARN("lobby: team format failed");
        return;
    }

    SteamAPI_ISteamMatchmaking_SetLobbyMemberData(ctx->lobby.matchmaking, ctx->lobby.curr_lobby, LDG_NET_LOBBY_MEMBER_KEY_TEAM, team_str);

    LDG_ERRLOG_INFO("lobby: set team");
}

uint32_t ldg_net_steamworks_lobby_game_start(ldg_net_steamworks_lobby_ctx_t *ctx)
{
    void *user = 0x0;
    uint64_t my_steam_id = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!ctx->lobby.is_init)) { return LDG_ERR_NOT_INIT; }

    if (!ctx->lobby.is_host)
    {
        LDG_ERRLOG_WARN("lobby: only host can start game");
        return LDG_ERR_DENIED;
    }

    if (ctx->lobby.curr_lobby == 0)
    {
        LDG_ERRLOG_ERR("lobby: no active lobby");
        return LDG_ERR_INVALID;
    }

    user = SteamAPI_SteamUser_v023();
    if (user) { my_steam_id = SteamAPI_ISteamUser_GetSteamID(user); }

    SteamAPI_ISteamMatchmaking_SetLobbyGameServer(ctx->lobby.matchmaking, ctx->lobby.curr_lobby, 0, 0, my_steam_id);

    ctx->lobby.state = LDG_NET_LOBBY_STATE_STARTING;

    LDG_ERRLOG_INFO("lobby: starting game");

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_lobby_member_refresh(ldg_net_steamworks_lobby_ctx_t *ctx)
{
    int32_t cunt = 0;
    int32_t iter = 0;

    if (LDG_UNLIKELY(!ctx || !ctx->lobby.is_init || ctx->lobby.curr_lobby == 0 || !ctx->lobby.matchmaking || !ctx->lobby.friends)) { return; }

    cunt = SteamAPI_ISteamMatchmaking_GetNumLobbyMembers(ctx->lobby.matchmaking, ctx->lobby.curr_lobby);
    if (cunt < 0) { cunt = 0; }

    if (cunt > LDG_NET_LOBBY_MEMBER_MAX) { cunt = LDG_NET_LOBBY_MEMBER_MAX; }

    ctx->lobby.member_cunt = 0;

    for (iter = 0; iter < cunt; iter = iter + 1)
    {
        uint64_t member_steam_id = 0;
        ldg_net_lobby_member_t *member = 0x0;
        uint8_t name_pending = 0;
        const char *name = 0x0;
        const char *rdy_str = 0x0;
        const char *team_str = 0x0;
        uint32_t fallback_err = 0;

        member_steam_id = SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex(ctx->lobby.matchmaking, ctx->lobby.curr_lobby, iter);
        if (member_steam_id == 0) { continue; }

        member = &ctx->lobby.member[ctx->lobby.member_cunt];
        member->user_id = member_steam_id;

        name_pending = SteamAPI_ISteamFriends_RequestUserInformation(ctx->lobby.friends, member_steam_id, 1);
        name = (name_pending == 0) ? SteamAPI_ISteamFriends_GetFriendPersonaName(ctx->lobby.friends, member_steam_id) : 0x0;
        if (!name || ldg_str_rbrcpy(member->name, name, sizeof(member->name)) != 0)
        {
            fallback_err = ldg_str_rbrcpy(member->name, "Player", sizeof(member->name));
            if (LDG_UNLIKELY(fallback_err != LDG_ERR_AOK && fallback_err != LDG_ERR_STR_TRUNC)) { LDG_ERRLOG_WARN("lobby: member name fallback copy failed"); }
        }

        rdy_str = SteamAPI_ISteamMatchmaking_GetLobbyMemberData(ctx->lobby.matchmaking, ctx->lobby.curr_lobby, member_steam_id, LDG_NET_LOBBY_MEMBER_KEY_RDY);
        member->rdy = (rdy_str && rdy_str[0] == '1') ? 1 : 0;

        team_str = SteamAPI_ISteamMatchmaking_GetLobbyMemberData(ctx->lobby.matchmaking, ctx->lobby.curr_lobby, member_steam_id, LDG_NET_LOBBY_MEMBER_KEY_TEAM);
        member->team = (team_str && team_str[0] >= '0' && team_str[0] <= '9') ? (uint8_t)(team_str[0] - '0') : 0;

        ctx->lobby.member_cunt = ctx->lobby.member_cunt + 1;
    }
}

void ldg_net_steamworks_lobby_call_handle(ldg_net_steamworks_lobby_ctx_t *ctx, int32_t pipe, uint64_t call)
{
    uint8_t failed = 0;

    if (LDG_UNLIKELY(!ctx)) { return; }

    if (ctx->pending_type == LDG_NET_LOBBY_PENDING_NONE || call != ctx->lobby.pending_call) { return; }

    if (ctx->pending_type == LDG_NET_LOBBY_PENDING_CREATE)
    {
        lobby_created_t res = LDG_STRUCT_ZERO_INIT;
        if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, call, &res, (int32_t)sizeof(res), STEAM_CB_LOBBY_CREATED, &failed) != 0)
        {
            if (failed != 0)
            {
                LDG_ERRLOG_ERR("lobby: CreateLobby call failed; IO error");
                ctx->lobby.state = LDG_NET_LOBBY_STATE_NONE;
            }
            else { on_created(ctx, &res); }

            ctx->lobby.pending_call = 0;
            ctx->pending_type = LDG_NET_LOBBY_PENDING_NONE;
        }
    }
    else if (ctx->pending_type == LDG_NET_LOBBY_PENDING_LIST)
    {
        lobby_match_list_t res = LDG_STRUCT_ZERO_INIT;
        if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, call, &res, (int32_t)sizeof(res), STEAM_CB_LOBBY_MATCH_LIST, &failed) != 0)
        {
            if (failed != 0)
            {
                LDG_ERRLOG_ERR("lobby: RequestLobbyList call failed; IO error");
                ctx->lobby.state = LDG_NET_LOBBY_STATE_NONE;
            }
            else { on_match_list(ctx, &res); }

            ctx->lobby.pending_call = 0;
            ctx->pending_type = LDG_NET_LOBBY_PENDING_NONE;
        }
    }
    else if (ctx->pending_type == LDG_NET_LOBBY_PENDING_JOIN)
    {
        lobby_enter_t res = LDG_STRUCT_ZERO_INIT;
        if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, call, &res, (int32_t)sizeof(res), STEAM_CB_LOBBY_ENTER, &failed) != 0)
        {
            if (failed != 0)
            {
                LDG_ERRLOG_ERR("lobby: JoinLobby call failed; IO error");
                ctx->lobby.state = LDG_NET_LOBBY_STATE_NONE;
            }
            else { on_enter(ctx, &res); }

            ctx->lobby.pending_call = 0;
            ctx->pending_type = LDG_NET_LOBBY_PENDING_NONE;
        }
    }
}

void ldg_net_steamworks_lobby_cb_handle(ldg_net_steamworks_lobby_ctx_t *ctx, int32_t cb_id, void *param, int32_t param_size)
{
    if (LDG_UNLIKELY(!ctx)) { return; }

    if (cb_id == STEAM_CB_LOBBY_DATA_UPDATE) { if (param != 0x0 && param_size >= (int32_t)sizeof(lobby_data_update_t)) { on_data_update(ctx, (lobby_data_update_t *)param); } }
    else if (cb_id == STEAM_CB_LOBBY_CHAT_UPDATE) { if (param != 0x0 && param_size >= (int32_t)sizeof(lobby_chat_update_t)) { on_chat_update(ctx, (lobby_chat_update_t *)param); } }
    else if (cb_id == STEAM_CB_LOBBY_GAME_CREATED) { if (param != 0x0 && param_size >= (int32_t)sizeof(lobby_game_created_t)) { on_game_created(ctx, (lobby_game_created_t *)param); } }
    else if (cb_id == STEAM_CB_PERSONA_STATE_CHANGE) { ldg_net_steamworks_lobby_member_refresh(ctx); }
}

uint64_t ldg_net_steamworks_lobby_host_steam_id_get(ldg_net_steamworks_lobby_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx || !ctx->lobby.is_init)) { return 0; }

    return ctx->lobby.host_id;
}
