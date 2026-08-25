#ifndef LDG_NET_H
#define LDG_NET_H

#include <stdint.h>
#include <dangling/core/macros.h>

#define LDG_NET_TICK_RATE 16
#define LDG_NET_TIMEOUT 5000
#define LDG_NET_CHANNEL_RELIABLE 0
#define LDG_NET_CHANNEL_UNRELIABLE 1
#define LDG_NET_CHANNEL_CUNT 2

#define LDG_NET_RATE_DEFAULT 10240.0
#define LDG_NET_RATE_MIN 1024.0

#define LDG_NET_CLIENT_MAX 10
#define LDG_NET_CLIENT_NAME_MAX 64
#define LDG_NET_PORT_DEFAULT 27015

#define LDG_NET_MSG_TYPE_RESERVED_MIN 0xF0

#define LDG_NET_SRV_CLIENT_WELCOME 0xF1
#define LDG_NET_SRV_CLIENT_JOIN 0xF2
#define LDG_NET_SRV_CLIENT_LEAVE 0xF3
#define LDG_NET_SRV_CLIENT_CHAT 0xF4
#define LDG_NET_SRV_CLIENT_VOICE 0xF5
#define LDG_NET_SRV_CLIENT_ACK 0xF8

#define LDG_NET_CLIENT_SRV_NAME 0xF1
#define LDG_NET_CLIENT_SRV_CHAT 0xF2
#define LDG_NET_CLIENT_SRV_VOICE 0xF3

#define LDG_NET_LOBBY_MEMBER_MAX 10
#define LDG_NET_LOBBY_LIST_MAX 16
#define LDG_NET_LOBBY_NAME_MAX 64
#define LDG_NET_LOBBY_MAP_MAX 64

#define LDG_NET_LOBBY_KEY_MAP "map"
#define LDG_NET_LOBBY_KEY_REV "ver"
#define LDG_NET_LOBBY_MEMBER_KEY_RDY "rdy"
#define LDG_NET_LOBBY_MEMBER_KEY_TEAM "team"

#define LDG_NET_LOBBY_TYPE_PUBLIC 2
#define LDG_NET_LOBBY_CMP_EQUAL 0

#define LDG_NET_LOBBY_PENDING_NONE 0
#define LDG_NET_LOBBY_PENDING_CREATE 1
#define LDG_NET_LOBBY_PENDING_LIST 2
#define LDG_NET_LOBBY_PENDING_JOIN 3

typedef enum ldg_net_mode
{
    LDG_NET_MODE_NONE = 0,
    LDG_NET_MODE_SRV = 1,
    LDG_NET_MODE_CLIENT = 2
} ldg_net_mode_t;

typedef enum ldg_net_lobby_state
{
    LDG_NET_LOBBY_STATE_NONE = 0,
    LDG_NET_LOBBY_STATE_BROWSING = 1,
    LDG_NET_LOBBY_STATE_CREATING = 2,
    LDG_NET_LOBBY_STATE_HOSTING = 3,
    LDG_NET_LOBBY_STATE_JOINING = 4,
    LDG_NET_LOBBY_STATE_JOINED = 5,
    LDG_NET_LOBBY_STATE_STARTING = 6
} ldg_net_lobby_state_t;

typedef struct ldg_net_lobby_member
{
    uint64_t user_id;
    char name[LDG_NET_LOBBY_NAME_MAX];
    uint8_t rdy;
    uint8_t team;
    uint8_t pudding[6];
} ldg_net_lobby_member_t;

typedef struct ldg_net_lobby_info
{
    uint64_t lobby_id;
    char host_name[LDG_NET_LOBBY_NAME_MAX];
    char map_name[LDG_NET_LOBBY_MAP_MAX];
    uint8_t player_cunt;
    uint8_t player_max;
    uint8_t pudding[6];
} ldg_net_lobby_info_t;

typedef struct ldg_net_lobby_ctx
{
    void *matchmaking;
    void *friends;
    ldg_net_lobby_state_t state;
    uint8_t pudding0[4];
    uint64_t curr_lobby;
    uint64_t pending_call;
    uint64_t host_id;
    ldg_net_lobby_info_t list[LDG_NET_LOBBY_LIST_MAX];
    uint32_t list_cunt;
    ldg_net_lobby_member_t member[LDG_NET_LOBBY_MEMBER_MAX];
    uint32_t member_cunt;
    uint8_t is_host;
    uint8_t is_init;
    uint8_t pudding[46];
} LDG_ALIGNED ldg_net_lobby_ctx_t;

typedef enum ldg_net_conn_status
{
    LDG_NET_CONN_STATUS_DCD = 0,
    LDG_NET_CONN_STATUS_CONNECTING = 1,
    LDG_NET_CONN_STATUS_CONN = 2
} ldg_net_conn_status_t;

#define LDG_NET_SRV_BROWSER_MAX 16
#define LDG_NET_SRV_BROWSER_NAME_MAX 64
#define LDG_NET_SRV_BROWSER_MAP_MAX 32
#define LDG_NET_SRV_BROWSER_TAG_MAX 128

typedef enum ldg_net_srv_browser_state
{
    LDG_NET_SRV_BROWSER_STATE_NONE = 0,
    LDG_NET_SRV_BROWSER_STATE_REFRESHING = 1,
    LDG_NET_SRV_BROWSER_STATE_DONE = 2,
    LDG_NET_SRV_BROWSER_STATE_ERR = 3
} ldg_net_srv_browser_state_t;

typedef struct ldg_net_srv_browser_entry
{
    uint64_t server_id;
    uint32_t ip;
    uint16_t port;
    uint16_t query_port;
    int32_t ping;
    int32_t client_cunt;
    int32_t client_max;
    int32_t bot_cunt;
    char name[LDG_NET_SRV_BROWSER_NAME_MAX];
    char map[LDG_NET_SRV_BROWSER_MAP_MAX];
    char tag[LDG_NET_SRV_BROWSER_TAG_MAX];
    uint8_t secure;
    uint8_t password;
    uint8_t dedicated;
    uint8_t pudding[5];
} ldg_net_srv_browser_entry_t;

typedef struct ldg_net_srv_browser_ctx
{
    void *matchmaking_srv;
    void *req_handle;
    ldg_net_srv_browser_state_t state;
    uint32_t entry_cunt;
    ldg_net_srv_browser_entry_t entry[LDG_NET_SRV_BROWSER_MAX];
    uint8_t is_init;
    uint8_t pudding[55];
} LDG_ALIGNED ldg_net_srv_browser_ctx_t;

typedef struct ldg_net_client
{
    uint64_t steam_id;
    uint32_t conn;
    uint32_t id;
    uint8_t is_conn;
    uint8_t pudding0[3];
    char name[LDG_NET_CLIENT_NAME_MAX];
    void *gstate;
    uint8_t pudding[40];
} LDG_ALIGNED ldg_net_client_t;

typedef struct ldg_net_ctx
{
    void *sock;
    ldg_net_client_t *client;
    uint32_t listen_sock;
    uint32_t srv_conn;
    uint32_t poll_group;
    const struct ldg_net_cb *cb;
    ldg_net_mode_t mode;
    ldg_net_conn_status_t status;
    uint32_t local_id;
    uint32_t client_cunt;
    uint8_t is_srv;
    uint32_t out_seq;
    uint32_t in_seq;
    uint16_t port;
    uint8_t is_init;
    uint8_t pudding0[9];
    double cleartime;
    double rate;
    uint32_t bytes_sent;
    uint8_t pudding[92];
} LDG_ALIGNED ldg_net_ctx_t;

typedef void (*ldg_net_conn_cb_t)(struct ldg_net_ctx *ctx, uint32_t conn_id);
typedef void (*ldg_net_dc_cb_t)(struct ldg_net_ctx *ctx, uint32_t conn_id);
typedef void (*ldg_net_data_cb_t)(struct ldg_net_ctx *ctx, uint32_t conn_id, const void *data, uint32_t len);
typedef void (*ldg_net_chat_cb_t)(struct ldg_net_ctx *ctx, const char *sender, const char *msg);
typedef void (*ldg_net_voice_cb_t)(struct ldg_net_ctx *ctx, uint32_t client_id, const void *data, uint32_t len);

typedef struct ldg_net_cb
{
    ldg_net_conn_cb_t on_conn;
    ldg_net_dc_cb_t on_dc;
    ldg_net_data_cb_t on_data;
    ldg_net_chat_cb_t on_chat;
    ldg_net_voice_cb_t on_voice;
    void *user;
    uint8_t pudding[16];
} LDG_ALIGNED ldg_net_cb_t;

LDG_EXPORT uint32_t ldg_net_player_cunt_get(const ldg_net_ctx_t *ctx);
LDG_EXPORT uint64_t ldg_net_player_steam_id_get(const ldg_net_ctx_t *ctx, uint32_t player_id);

#endif
