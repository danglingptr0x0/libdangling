#ifndef LDG_NET_STEAMWORKS_LOBBY_H
#define LDG_NET_STEAMWORKS_LOBBY_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/net/net.h>

#define LDG_NET_STEAMWORKS_LOBBY_REV_MAX 16

typedef struct ldg_net_steamworks_lobby_ctx
{
    ldg_net_lobby_ctx_t lobby;
    int32_t steam_pipe;
    int32_t pending_type;
    char map[LDG_NET_LOBBY_MAP_MAX];
    char rev[LDG_NET_STEAMWORKS_LOBBY_REV_MAX];
    uint8_t pudding[40];
} LDG_ALIGNED ldg_net_steamworks_lobby_ctx_t;

LDG_EXPORT uint32_t ldg_net_steamworks_lobby_init(ldg_net_steamworks_lobby_ctx_t *ctx, const char *map, const char *rev);
LDG_EXPORT void ldg_net_steamworks_lobby_shutdown(ldg_net_steamworks_lobby_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_lobby_create(ldg_net_steamworks_lobby_ctx_t *ctx, uint32_t member_max);
LDG_EXPORT uint32_t ldg_net_steamworks_lobby_list_req(ldg_net_steamworks_lobby_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_lobby_join(ldg_net_steamworks_lobby_ctx_t *ctx, uint64_t lobby_id);
LDG_EXPORT void ldg_net_steamworks_lobby_leave(ldg_net_steamworks_lobby_ctx_t *ctx);
LDG_EXPORT void ldg_net_steamworks_lobby_rdy_set(ldg_net_steamworks_lobby_ctx_t *ctx, uint8_t rdy);
LDG_EXPORT void ldg_net_steamworks_lobby_team_set(ldg_net_steamworks_lobby_ctx_t *ctx, uint8_t team);
LDG_EXPORT uint32_t ldg_net_steamworks_lobby_game_start(ldg_net_steamworks_lobby_ctx_t *ctx);
LDG_EXPORT void ldg_net_steamworks_lobby_call_handle(ldg_net_steamworks_lobby_ctx_t *ctx, int32_t pipe, uint64_t call);
LDG_EXPORT void ldg_net_steamworks_lobby_cb_handle(ldg_net_steamworks_lobby_ctx_t *ctx, int32_t cb_id, void *param, int32_t param_size);
LDG_EXPORT void ldg_net_steamworks_lobby_member_refresh(ldg_net_steamworks_lobby_ctx_t *ctx);
LDG_EXPORT uint64_t ldg_net_steamworks_lobby_host_steam_id_get(ldg_net_steamworks_lobby_ctx_t *ctx);

#endif
