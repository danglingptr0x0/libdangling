#ifndef LDG_NET_STEAMWORKS_SRV_H
#define LDG_NET_STEAMWORKS_SRV_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_NET_STEAMWORKS_SRV_SECRET_LEN 32

typedef struct ldg_net_steamworks_srv_conf
{
    const char *product;
    const char *mod_dir;
    const char *name;
    const char *desc;
    const char *map;
    const char *tag;
    const char *rev;
    uint32_t player_max;
    uint32_t bot_cunt;
    uint8_t password;
    uint8_t advertise;
    uint8_t pudding[6];
} ldg_net_steamworks_srv_conf_t;

typedef struct ldg_net_steamworks_srv_ident
{
    uint64_t steam_id;
    uint32_t app_id;
    uint32_t issue_time;
    uint8_t vac_banned;
    uint8_t license_borrowed;
    uint8_t license_temporary;
    uint8_t pudding[45];
} LDG_ALIGNED ldg_net_steamworks_srv_ident_t;

typedef void (*ldg_net_steamworks_srv_auth_cb_t)(uint64_t steam_id, uint64_t owner_steam_id, int32_t resp, void *user);

typedef struct ldg_net_steamworks_srv_auth
{
    ldg_net_steamworks_srv_auth_cb_t cb;
    void *user;
} ldg_net_steamworks_srv_auth_t;

LDG_EXPORT uint32_t ldg_net_steamworks_srv_init(uint16_t game_port, const ldg_net_steamworks_srv_conf_t *conf);
LDG_EXPORT uint64_t ldg_net_steamworks_srv_steam_id_get(void);
LDG_EXPORT uint32_t ldg_net_steamworks_srv_auth_begin(const void *tkt, uint32_t tkt_len, uint64_t steam_id);
LDG_EXPORT void ldg_net_steamworks_srv_auth_end(uint64_t steam_id);
LDG_EXPORT uint32_t ldg_net_steamworks_srv_tkt_dec(const uint8_t *enc, uint32_t enc_len, const uint8_t *secret, uint32_t secret_len, ldg_net_steamworks_srv_ident_t *out);
LDG_EXPORT void ldg_net_steamworks_srv_cb_handle(void *srv, int32_t cb_id, void *param, int32_t param_size);

#endif
