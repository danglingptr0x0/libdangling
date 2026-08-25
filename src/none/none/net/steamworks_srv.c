#include <stdint.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/time/time.h>
#include <dangling/net/steamworks_srv.h>

#define STEAM_CB_VALIDATE_AUTH_TKT 143
#define STEAM_SRV_MODE_AUTH 1
#define STEAM_SRV_LOGON_TIMEOUT 5.0
#define STEAM_ERR_MSG_SIZE 1024
#define STEAM_AUTH_BEGIN_OK 0

typedef struct srv_validate_auth
{
    uint64_t steam_id;
    int32_t resp;
#ifdef _WIN32
    uint8_t pudding0[4];
#endif
    uint64_t owner_steam_id;
} LDG_PACKED srv_validate_auth_t;

#define STEAMWORKS_LAYOUT_CK(name, cond) typedef char steam_layout_ck_ ## name[(cond) ? 1 : -1]

#ifdef _WIN32
STEAMWORKS_LAYOUT_CK(validate_auth_size, sizeof(srv_validate_auth_t) == 24);
#else
STEAMWORKS_LAYOUT_CK(validate_auth_size, sizeof(srv_validate_auth_t) == 20);
#endif

extern int32_t SteamInternal_GameServer_Init_V2(uint32_t ip, uint16_t game_port, uint16_t query_port, int32_t srv_mode, const char *rev, const char *iface_rev, char *err_msg);
extern void* SteamAPI_SteamGameServer_v015(void);
extern void SteamAPI_ISteamGameServer_LogOnAnonymous(void *self);
extern uint8_t SteamAPI_ISteamGameServer_BLoggedOn(void *self);
extern void SteamAPI_ISteamGameServer_SetProduct(void *self, const char *product);
extern void SteamAPI_ISteamGameServer_SetGameDescription(void *self, const char *desc);
extern void SteamAPI_ISteamGameServer_SetModDir(void *self, const char *dir);
extern void SteamAPI_ISteamGameServer_SetDedicatedServer(void *self, uint8_t dedicated);
extern void SteamAPI_ISteamGameServer_SetMaxPlayerCount(void *self, int32_t max);
extern void SteamAPI_ISteamGameServer_SetMapName(void *self, const char *map);
extern void SteamAPI_ISteamGameServer_SetServerName(void *self, const char *name);
extern void SteamAPI_ISteamGameServer_SetGameTags(void *self, const char *tag);
extern void SteamAPI_ISteamGameServer_SetPasswordProtected(void *self, uint8_t password);
extern void SteamAPI_ISteamGameServer_SetBotPlayerCount(void *self, int32_t bot_cunt);
extern void SteamAPI_ISteamGameServer_SetAdvertiseServerActive(void *self, uint8_t active);
extern uint64_t SteamAPI_ISteamGameServer_GetSteamID(void *self);
extern int32_t SteamAPI_ISteamGameServer_BeginAuthSession(void *self, const void *tkt, int32_t tkt_len, uint64_t steam_id);
extern void SteamAPI_ISteamGameServer_EndAuthSession(void *self, uint64_t steam_id);
extern int32_t SteamGameServer_GetHSteamPipe(void);
extern void SteamAPI_ManualDispatch_RunFrame(int32_t pipe);
extern uint8_t SteamAPI_ManualDispatch_GetNextCallback(int32_t pipe, void *cb_msg);
extern void SteamAPI_ManualDispatch_FreeLastCallback(int32_t pipe);

typedef struct srv_cb_msg
{
    int32_t steam_user;
    int32_t cb_id;
    uint8_t *param;
    int32_t param_size;
} srv_cb_msg_t;

static void logon_wait(void *game_server)
{
    double wait_start = 0.0;
    double wait_now = 0.0;
    int32_t srv_pipe = 0;
    srv_cb_msg_t cb_msg = LDG_STRUCT_ZERO_INIT;

    srv_pipe = SteamGameServer_GetHSteamPipe();

    ldg_time_monotonic_get(&wait_start);
    wait_now = wait_start;
    while (wait_now - wait_start < STEAM_SRV_LOGON_TIMEOUT)
    {
        if (srv_pipe != 0)
        {
            SteamAPI_ManualDispatch_RunFrame(srv_pipe);
            while (SteamAPI_ManualDispatch_GetNextCallback(srv_pipe, &cb_msg) != 0) { SteamAPI_ManualDispatch_FreeLastCallback(srv_pipe); }
        }

        if (SteamAPI_ISteamGameServer_BLoggedOn(game_server) != 0)
        {
            LDG_ERRLOG_INFO("net: game server logged on to Steam");
            break;
        }

        ldg_time_monotonic_get(&wait_now);
    }

    if (SteamAPI_ISteamGameServer_BLoggedOn(game_server) == 0) { LDG_ERRLOG_WARN("net: game server login timeout; P2P may not work"); }
}

uint32_t ldg_net_steamworks_srv_init(uint16_t game_port, const ldg_net_steamworks_srv_conf_t *conf)
{
    void *err_msg = 0x0;
    void *game_server_ck = 0x0;
    void *game_server = 0x0;
    uint32_t alloc_err = 0;
    int32_t res = 0;

    if (LDG_UNLIKELY(!conf)) { return LDG_ERR_FUNC_ARG_NULL; }

    game_server_ck = SteamAPI_SteamGameServer_v015();
    if (LDG_UNLIKELY(game_server_ck != 0x0)) { return LDG_ERR_AOK; }

    alloc_err = ldg_mem_alloc(STEAM_ERR_MSG_SIZE, &err_msg);
    if (LDG_UNLIKELY(alloc_err != LDG_ERR_AOK)) { return alloc_err; }

    res = SteamInternal_GameServer_Init_V2(0, game_port, game_port + 1, STEAM_SRV_MODE_AUTH, conf->rev, 0x0, (char *)err_msg);
    if (LDG_UNLIKELY(res != 0))
    {
        LDG_ERRLOG_ERR("net: failed to init steam game server");
        ldg_mem_dealloc(err_msg);
        return LDG_ERR_NET_INIT;
    }

    ldg_mem_dealloc(err_msg);

    LDG_ERRLOG_INFO("net: steamworks server init on port");

    game_server = SteamAPI_SteamGameServer_v015();
    if (game_server)
    {
        SteamAPI_ISteamGameServer_SetProduct(game_server, conf->product);
        SteamAPI_ISteamGameServer_SetModDir(game_server, conf->mod_dir);
        SteamAPI_ISteamGameServer_SetDedicatedServer(game_server, 1);

        SteamAPI_ISteamGameServer_LogOnAnonymous(game_server);
        LDG_ERRLOG_INFO("net: game server logging on anonymously");

        logon_wait(game_server);

        SteamAPI_ISteamGameServer_SetServerName(game_server, conf->name);
        SteamAPI_ISteamGameServer_SetGameDescription(game_server, conf->desc);
        SteamAPI_ISteamGameServer_SetMaxPlayerCount(game_server, (int32_t)conf->player_max);
        SteamAPI_ISteamGameServer_SetMapName(game_server, conf->map);
        SteamAPI_ISteamGameServer_SetGameTags(game_server, conf->tag);
        SteamAPI_ISteamGameServer_SetPasswordProtected(game_server, conf->password);
        SteamAPI_ISteamGameServer_SetBotPlayerCount(game_server, (int32_t)conf->bot_cunt);
        SteamAPI_ISteamGameServer_SetAdvertiseServerActive(game_server, conf->advertise);
        LDG_ERRLOG_INFO("net: game server advertising enabled");
    }
    else { LDG_ERRLOG_WARN("net: failed to get game server interface"); }

    return LDG_ERR_AOK;
}

uint64_t ldg_net_steamworks_srv_steam_id_get(void)
{
    void *game_server = 0x0;

    game_server = SteamAPI_SteamGameServer_v015();
    if (game_server) { return SteamAPI_ISteamGameServer_GetSteamID(game_server); }

    return 0;
}

uint32_t ldg_net_steamworks_srv_auth_begin(const void *tkt, uint32_t tkt_len, uint64_t steam_id)
{
    void *game_server = 0x0;
    int32_t res = 0;

    if (LDG_UNLIKELY(!tkt || tkt_len == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(steam_id == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    game_server = SteamAPI_SteamGameServer_v015();
    if (LDG_UNLIKELY(!game_server)) { return LDG_ERR_NOT_INIT; }

    res = SteamAPI_ISteamGameServer_BeginAuthSession(game_server, tkt, (int32_t)tkt_len, steam_id);
    if (LDG_UNLIKELY(res != STEAM_AUTH_BEGIN_OK))
    {
        LDG_ERRLOG_WARN("net: begin auth session failed");
        return LDG_ERR_NET_AUTH_FAIL;
    }

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_srv_auth_end(uint64_t steam_id)
{
    void *game_server = 0x0;

    if (LDG_UNLIKELY(steam_id == 0)) { return; }

    game_server = SteamAPI_SteamGameServer_v015();
    if (LDG_UNLIKELY(!game_server)) { return; }

    SteamAPI_ISteamGameServer_EndAuthSession(game_server, steam_id);
}

void ldg_net_steamworks_srv_cb_handle(void *srv, int32_t cb_id, void *param, int32_t param_size)
{
    ldg_net_steamworks_srv_auth_t *auth = 0x0;
    srv_validate_auth_t *validate = 0x0;

    if (LDG_UNLIKELY(!srv)) { return; }

    auth = (ldg_net_steamworks_srv_auth_t *)srv;

    if (cb_id != STEAM_CB_VALIDATE_AUTH_TKT) { return; }

    if (param == 0x0 || param_size < (int32_t)sizeof(srv_validate_auth_t)) { return; }

    if (auth->cb == 0x0) { return; }

    validate = (srv_validate_auth_t *)param;
    auth->cb(validate->steam_id, validate->owner_steam_id, validate->resp, auth->user);
}
