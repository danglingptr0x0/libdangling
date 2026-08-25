#ifndef LDG_NET_STEAMWORKS_H
#define LDG_NET_STEAMWORKS_H

#include <stdint.h>
#include <dangling/core/macros.h>
#include <dangling/net/net.h>

typedef struct ldg_net_steamworks_ctx
{
    ldg_net_ctx_t net;
    int32_t steam_pipe;
    uint8_t pudding[60];
} LDG_ALIGNED ldg_net_steamworks_ctx_t;

typedef struct ldg_net_steamworks_hub
{
    void *lobby;
    void *lboard;
    void *econ;
    void *wshop;
    void *tkt;
    void *srv;
    void *friend_msg;
    void *join;
    void *friend_call;
    int32_t steam_pipe;
    int32_t srv_pipe;
} ldg_net_steamworks_hub_t;

#define LDG_NET_STEAMWORKS_PERSONA_NAME_MAX 128
#define LDG_NET_STEAMWORKS_FRIENDS_GROUP_NAME_MAX 64
#define LDG_NET_STEAMWORKS_RICH_PRESENCE_KEY_MAX 64
#define LDG_NET_STEAMWORKS_RICH_PRESENCE_VAL_MAX 256
#define LDG_NET_STEAMWORKS_RICH_PRESENCE_KEY_CUNT 30
#define LDG_NET_STEAMWORKS_FOLLOWING_PAGE_MAX 50
#define LDG_NET_STEAMWORKS_FRIENDS_GROUP_INVALID ((int16_t)-1)

#define LDG_NET_STEAMWORKS_FRIEND_FLAG_NONE 0x0000U
#define LDG_NET_STEAMWORKS_FRIEND_FLAG_BLK 0x0001U
#define LDG_NET_STEAMWORKS_FRIEND_FLAG_REQ_RECEIVED 0x0002U
#define LDG_NET_STEAMWORKS_FRIEND_FLAG_IMM 0x0004U
#define LDG_NET_STEAMWORKS_FRIEND_FLAG_CLAN_MEMBER 0x0008U
#define LDG_NET_STEAMWORKS_FRIEND_FLAG_ON_GAME_SRV 0x0010U
#define LDG_NET_STEAMWORKS_FRIEND_FLAG_REQ_SENT 0x0080U
#define LDG_NET_STEAMWORKS_FRIEND_FLAG_REQ_INFO 0x0100U
#define LDG_NET_STEAMWORKS_FRIEND_FLAG_IGNORED 0x0200U
#define LDG_NET_STEAMWORKS_FRIEND_FLAG_IGNORED_FRIEND 0x0400U
#define LDG_NET_STEAMWORKS_FRIEND_FLAG_CHAT_MEMBER 0x1000U
#define LDG_NET_STEAMWORKS_FRIEND_FLAG_ALL 0xFFFFU

#define LDG_NET_STEAMWORKS_QUERY_PORT_UNSET 0xFFFFU
#define LDG_NET_STEAMWORKS_QUERY_PORT_ERR 0xFFFEU

typedef enum ldg_net_steamworks_persona_state
{
    LDG_NET_STEAMWORKS_PERSONA_STATE_OFFLINE = 0,
    LDG_NET_STEAMWORKS_PERSONA_STATE_ONLINE,
    LDG_NET_STEAMWORKS_PERSONA_STATE_BUSY,
    LDG_NET_STEAMWORKS_PERSONA_STATE_AWAY,
    LDG_NET_STEAMWORKS_PERSONA_STATE_SNOOZE,
    LDG_NET_STEAMWORKS_PERSONA_STATE_TRADE,
    LDG_NET_STEAMWORKS_PERSONA_STATE_PLAY,
    LDG_NET_STEAMWORKS_PERSONA_STATE_INVISIBLE
} ldg_net_steamworks_persona_state_t;

typedef enum ldg_net_steamworks_friend_rel
{
    LDG_NET_STEAMWORKS_FRIEND_REL_NONE = 0,
    LDG_NET_STEAMWORKS_FRIEND_REL_BLK,
    LDG_NET_STEAMWORKS_FRIEND_REL_REQ_RECIPIENT,
    LDG_NET_STEAMWORKS_FRIEND_REL_FRIEND,
    LDG_NET_STEAMWORKS_FRIEND_REL_REQ_INITIATOR,
    LDG_NET_STEAMWORKS_FRIEND_REL_IGNORED,
    LDG_NET_STEAMWORKS_FRIEND_REL_IGNORED_FRIEND
} ldg_net_steamworks_friend_rel_t;

typedef enum ldg_net_steamworks_avatar_size
{
    LDG_NET_STEAMWORKS_AVATAR_SIZE_SMALL = 0,
    LDG_NET_STEAMWORKS_AVATAR_SIZE_MEDIUM,
    LDG_NET_STEAMWORKS_AVATAR_SIZE_LARGE
} ldg_net_steamworks_avatar_size_t;

typedef enum ldg_net_steamworks_store_flag
{
    LDG_NET_STEAMWORKS_STORE_FLAG_NONE = 0,
    LDG_NET_STEAMWORKS_STORE_FLAG_CART_ADD,
    LDG_NET_STEAMWORKS_STORE_FLAG_CART_ADD_SHOW
} ldg_net_steamworks_store_flag_t;

typedef enum ldg_net_steamworks_web_page_mode
{
    LDG_NET_STEAMWORKS_WEB_PAGE_MODE_DEFAULT = 0,
    LDG_NET_STEAMWORKS_WEB_PAGE_MODE_MODAL
} ldg_net_steamworks_web_page_mode_t;

typedef enum ldg_net_steamworks_profile_item
{
    LDG_NET_STEAMWORKS_PROFILE_ITEM_AVATAR_ANIMATED = 0,
    LDG_NET_STEAMWORKS_PROFILE_ITEM_AVATAR_FRAME,
    LDG_NET_STEAMWORKS_PROFILE_ITEM_MODIFIER,
    LDG_NET_STEAMWORKS_PROFILE_ITEM_BACKDROP,
    LDG_NET_STEAMWORKS_PROFILE_ITEM_MINI_BACKDROP
} ldg_net_steamworks_profile_item_t;

typedef enum ldg_net_steamworks_profile_prop
{
    LDG_NET_STEAMWORKS_PROFILE_PROP_IMG_SMALL = 0,
    LDG_NET_STEAMWORKS_PROFILE_PROP_IMG_LARGE,
    LDG_NET_STEAMWORKS_PROFILE_PROP_NAME_INTERNAL,
    LDG_NET_STEAMWORKS_PROFILE_PROP_TITLE,
    LDG_NET_STEAMWORKS_PROFILE_PROP_DESC,
    LDG_NET_STEAMWORKS_PROFILE_PROP_APP_ID,
    LDG_NET_STEAMWORKS_PROFILE_PROP_TYPE_ID,
    LDG_NET_STEAMWORKS_PROFILE_PROP_CLASS,
    LDG_NET_STEAMWORKS_PROFILE_PROP_MOVIE_WEBM,
    LDG_NET_STEAMWORKS_PROFILE_PROP_MOVIE_MP4,
    LDG_NET_STEAMWORKS_PROFILE_PROP_MOVIE_WEBM_SMALL,
    LDG_NET_STEAMWORKS_PROFILE_PROP_MOVIE_MP4_SMALL
} ldg_net_steamworks_profile_prop_t;

typedef struct ldg_net_steamworks_friend_game
{
    uint64_t game_id;
    uint64_t lobby_id;
    uint32_t ip;
    uint16_t port;
    uint16_t query_port;
} ldg_net_steamworks_friend_game_t;

#define LDG_NET_STEAMWORKS_PROFILE_ITEM_CUNT 5

typedef enum ldg_net_steamworks_friend_call_state
{
    LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_IDLE = 0,
    LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_PENDING,
    LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_DONE,
    LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_FAIL
} ldg_net_steamworks_friend_call_state_t;

typedef struct ldg_net_steamworks_friend_call_ctx
{
    uint64_t following_list[LDG_NET_STEAMWORKS_FOLLOWING_PAGE_MAX];
    uint64_t follower_call;
    uint64_t following_call;
    uint64_t following_list_call;
    uint64_t profile_call;
    uint64_t follower_steam_id;
    uint64_t following_steam_id;
    uint64_t profile_steam_id;
    uint32_t follower_cunt;
    uint32_t following_list_cunt;
    uint32_t following_list_total;
    ldg_net_steamworks_friend_call_state_t follower_state;
    ldg_net_steamworks_friend_call_state_t following_state;
    ldg_net_steamworks_friend_call_state_t following_list_state;
    ldg_net_steamworks_friend_call_state_t profile_state;
    uint8_t is_following;
    uint8_t profile_item[LDG_NET_STEAMWORKS_PROFILE_ITEM_CUNT];
    uint8_t pudding[22];
} LDG_ALIGNED ldg_net_steamworks_friend_call_ctx_t;

#define LDG_NET_STEAMWORKS_JOIN_CONNECT_MAX 256

typedef enum ldg_net_steamworks_join_state
{
    LDG_NET_STEAMWORKS_JOIN_STATE_NONE = 0,
    LDG_NET_STEAMWORKS_JOIN_STATE_LOBBY,
    LDG_NET_STEAMWORKS_JOIN_STATE_CONNECT
} ldg_net_steamworks_join_state_t;

typedef struct ldg_net_steamworks_join_ctx
{
    uint64_t lobby_id;
    uint64_t friend_steam_id;
    ldg_net_steamworks_join_state_t state;
    uint8_t pudding0[4];
    char connect[LDG_NET_STEAMWORKS_JOIN_CONNECT_MAX + LDG_STR_TERM_SIZE];
    uint8_t pudding[39];
} LDG_ALIGNED ldg_net_steamworks_join_ctx_t;

#define LDG_NET_STEAMWORKS_FRIEND_MSG_TEXT_MAX 2048
#define LDG_NET_STEAMWORKS_FRIEND_MSG_SLOT_MAX 8

typedef struct ldg_net_steamworks_friend_msg
{
    uint64_t steam_id;
    uint32_t len;
    uint8_t pudding[4];
    char text[LDG_NET_STEAMWORKS_FRIEND_MSG_TEXT_MAX];
} ldg_net_steamworks_friend_msg_t;

typedef struct ldg_net_steamworks_friend_msg_ctx
{
    ldg_net_steamworks_friend_msg_t slot[LDG_NET_STEAMWORKS_FRIEND_MSG_SLOT_MAX];
    uint32_t hd;
    uint32_t tail;
    uint32_t drop_cunt;
    uint32_t persona_rev;
    uint8_t pudding[48];
} LDG_ALIGNED ldg_net_steamworks_friend_msg_ctx_t;

typedef enum ldg_net_steamworks_music_status
{
    LDG_NET_STEAMWORKS_MUSIC_STATUS_UNDEFINED = 0,
    LDG_NET_STEAMWORKS_MUSIC_STATUS_PLAYING,
    LDG_NET_STEAMWORKS_MUSIC_STATUS_PAUSED,
    LDG_NET_STEAMWORKS_MUSIC_STATUS_IDLE
} ldg_net_steamworks_music_status_t;

typedef enum ldg_net_steamworks_timeline_mode
{
    LDG_NET_STEAMWORKS_TIMELINE_INVALID = 0,
    LDG_NET_STEAMWORKS_TIMELINE_PLAYING = 1,
    LDG_NET_STEAMWORKS_TIMELINE_STAGING = 2,
    LDG_NET_STEAMWORKS_TIMELINE_MENUS = 3,
    LDG_NET_STEAMWORKS_TIMELINE_LD = 4
} ldg_net_steamworks_timeline_mode_t;

LDG_EXPORT uint8_t ldg_net_steamworks_restart_needed_is(uint32_t app_id);
LDG_EXPORT uint32_t ldg_net_steamworks_init(void);
LDG_EXPORT void ldg_net_steamworks_shutdown(void);

LDG_EXPORT uint32_t ldg_net_steamworks_hub_init(ldg_net_steamworks_hub_t *hub);
LDG_EXPORT void ldg_net_steamworks_tick(ldg_net_steamworks_hub_t *hub);

LDG_EXPORT uint32_t ldg_net_steamworks_srv_start(ldg_net_steamworks_ctx_t *steam, uint16_t port);
LDG_EXPORT uint32_t ldg_net_steamworks_srv_p2p_start(ldg_net_steamworks_ctx_t *steam, uint32_t virtual_port);
LDG_EXPORT void ldg_net_steamworks_srv_stop(ldg_net_steamworks_ctx_t *steam);

LDG_EXPORT uint32_t ldg_net_steamworks_client_conn(ldg_net_steamworks_ctx_t *steam, const char *host, uint16_t port);
LDG_EXPORT uint32_t ldg_net_steamworks_client_p2p_conn(ldg_net_steamworks_ctx_t *steam, uint64_t srv_steam_id, uint32_t virtual_port);
LDG_EXPORT void ldg_net_steamworks_client_dc(ldg_net_steamworks_ctx_t *steam);

LDG_EXPORT uint32_t ldg_net_steamworks_poll(ldg_net_steamworks_ctx_t *steam, const ldg_net_cb_t *cb, uint32_t timeout);
LDG_EXPORT uint32_t ldg_net_steamworks_bcast(ldg_net_steamworks_ctx_t *steam, const void *data, uint64_t len, uint8_t reliable);
LDG_EXPORT uint32_t ldg_net_steamworks_send(ldg_net_steamworks_ctx_t *steam, const void *data, uint64_t len, uint8_t reliable);

LDG_EXPORT uint32_t ldg_net_steamworks_client_kick(ldg_net_steamworks_ctx_t *steam, uint32_t client_id, const char *reason);
LDG_EXPORT uint32_t ldg_net_steamworks_local_id_get(const ldg_net_steamworks_ctx_t *steam);
LDG_EXPORT uint32_t ldg_net_steamworks_client_cunt_get(const ldg_net_steamworks_ctx_t *steam);
LDG_EXPORT uint64_t ldg_net_steamworks_client_steam_id_get(const ldg_net_steamworks_ctx_t *steam, uint32_t client_id);

LDG_EXPORT void ldg_net_steamworks_rate_init(ldg_net_steamworks_ctx_t *steam, double rate);
LDG_EXPORT uint8_t ldg_net_steamworks_rate_ready_is(ldg_net_steamworks_ctx_t *steam);
LDG_EXPORT void ldg_net_steamworks_rate_update(ldg_net_steamworks_ctx_t *steam, uint32_t pkt_size);

LDG_EXPORT ldg_net_conn_status_t ldg_net_steamworks_status_get(const ldg_net_steamworks_ctx_t *steam);
LDG_EXPORT uint64_t ldg_net_steamworks_local_steam_id_get(void);
LDG_EXPORT const char* ldg_net_steamworks_local_name_get(void);
LDG_EXPORT void ldg_net_steamworks_joinable_set(uint64_t srv_steam_id, uint32_t client_cunt);
LDG_EXPORT uint32_t ldg_net_steamworks_launch_conn_get(char *out_conn, uint64_t out_len);

LDG_EXPORT void ldg_net_steamworks_presence_set(const char *key, const char *val);
LDG_EXPORT void ldg_net_steamworks_presence_clear(void);
LDG_EXPORT void ldg_net_steamworks_presence_campaign_set(const char *track, const char *mission);
LDG_EXPORT void ldg_net_steamworks_presence_practice_set(void);
LDG_EXPORT void ldg_net_steamworks_presence_menu_set(void);

LDG_EXPORT const char* ldg_net_steamworks_player_name_get(const ldg_net_ctx_t *ctx, uint32_t client_id);

LDG_EXPORT uint32_t ldg_net_steamworks_stat_int_get(const char *name, int32_t *out);
LDG_EXPORT uint32_t ldg_net_steamworks_stat_int_set(const char *name, int32_t val);
LDG_EXPORT uint32_t ldg_net_steamworks_stat_st(void);
LDG_EXPORT uint32_t ldg_net_steamworks_stat_avgrate_update(const char *name, double cunt, double sesh_len);

LDG_EXPORT uint32_t ldg_net_steamworks_achievement_get(const char *name, uint8_t *out);
LDG_EXPORT uint32_t ldg_net_steamworks_achievement_set(const char *name);
LDG_EXPORT uint32_t ldg_net_steamworks_achievement_clear(const char *name);
LDG_EXPORT uint32_t ldg_net_steamworks_achievement_progress_indicate(const char *name, uint32_t cur, uint32_t max);

LDG_EXPORT uint32_t ldg_net_steamworks_avatar_get(uint8_t *rgba_out, uint32_t *width_out, uint32_t *height_out);

LDG_EXPORT uint32_t ldg_net_steamworks_user_info_req(uint64_t steam_id_user, uint8_t require_name_only, uint8_t *out);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_msg_listen_set(uint8_t listen);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_msg_init(ldg_net_steamworks_friend_msg_ctx_t *ctx);
LDG_EXPORT void ldg_net_steamworks_friend_msg_cb_handle(ldg_net_steamworks_friend_msg_ctx_t *ctx, int32_t cb_id, void *param, int32_t param_size);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_msg_get(ldg_net_steamworks_friend_msg_ctx_t *ctx, ldg_net_steamworks_friend_msg_t *out);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_msg_drop_cunt_get(const ldg_net_steamworks_friend_msg_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_msg_reply(uint64_t steam_id_friend, const char *text);
LDG_EXPORT const char* ldg_net_steamworks_friend_name_get(uint64_t steam_id_user);
LDG_EXPORT uint32_t ldg_net_steamworks_persona_rev_get(const ldg_net_steamworks_friend_msg_ctx_t *ctx);

LDG_EXPORT uint32_t ldg_net_steamworks_join_init(ldg_net_steamworks_join_ctx_t *ctx);
LDG_EXPORT void ldg_net_steamworks_join_cb_handle(ldg_net_steamworks_join_ctx_t *ctx, int32_t cb_id, void *param, int32_t param_size);
LDG_EXPORT ldg_net_steamworks_join_state_t ldg_net_steamworks_join_state_get(const ldg_net_steamworks_join_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_join_lobby_get(ldg_net_steamworks_join_ctx_t *ctx, uint64_t *out_lobby_id);
LDG_EXPORT uint32_t ldg_net_steamworks_join_connect_get(ldg_net_steamworks_join_ctx_t *ctx, char *out, uint64_t out_size);
LDG_EXPORT uint64_t ldg_net_steamworks_join_friend_steam_id_get(const ldg_net_steamworks_join_ctx_t *ctx);

LDG_EXPORT ldg_net_steamworks_persona_state_t ldg_net_steamworks_persona_state_get(void);
LDG_EXPORT ldg_net_steamworks_persona_state_t ldg_net_steamworks_friend_persona_state_get(uint64_t steam_id);
LDG_EXPORT ldg_net_steamworks_friend_rel_t ldg_net_steamworks_friend_rel_get(uint64_t steam_id);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_cunt_get(uint32_t flags);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_list_get(uint32_t flags, uint64_t *out_steam_id, uint32_t cap, uint32_t *out_cunt);
LDG_EXPORT uint8_t ldg_net_steamworks_friend_has(uint64_t steam_id, uint32_t flags);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_level_get(uint64_t steam_id);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_game_get(uint64_t steam_id, ldg_net_steamworks_friend_game_t *out);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_name_history_get(uint64_t steam_id, uint32_t name_idx, char *out, uint64_t out_size);
LDG_EXPORT uint32_t ldg_net_steamworks_player_nickname_get(uint64_t steam_id, char *out, uint64_t out_size);

LDG_EXPORT uint32_t ldg_net_steamworks_friends_group_cunt_get(void);
LDG_EXPORT uint32_t ldg_net_steamworks_friends_group_list_get(int16_t *out_group_id, uint32_t cap, uint32_t *out_cunt);
LDG_EXPORT uint32_t ldg_net_steamworks_friends_group_name_get(int16_t group_id, char *out, uint64_t out_size);
LDG_EXPORT uint32_t ldg_net_steamworks_friends_group_member_list_get(int16_t group_id, uint64_t *out_steam_id, uint32_t cap, uint32_t *out_cunt);

LDG_EXPORT uint32_t ldg_net_steamworks_friend_avatar_get(uint64_t steam_id, ldg_net_steamworks_avatar_size_t size, uint8_t *rgba_out, uint32_t *width_out, uint32_t *height_out);

LDG_EXPORT uint32_t ldg_net_steamworks_friend_presence_get(uint64_t steam_id, const char *key, char *out, uint64_t out_size);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_presence_key_cunt_get(uint64_t steam_id);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_presence_key_get(uint64_t steam_id, uint32_t key_idx, char *out, uint64_t out_size);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_presence_req(uint64_t steam_id);
LDG_EXPORT uint32_t ldg_net_steamworks_friend_game_invite(uint64_t steam_id, const char *connect);

LDG_EXPORT uint32_t ldg_net_steamworks_coplay_friend_list_get(uint64_t *out_steam_id, uint32_t cap, uint32_t *out_cunt);
LDG_EXPORT uint32_t ldg_net_steamworks_coplay_time_get(uint64_t steam_id);
LDG_EXPORT uint32_t ldg_net_steamworks_coplay_game_get(uint64_t steam_id);
LDG_EXPORT uint32_t ldg_net_steamworks_unread_chat_cunt_get(void);
LDG_EXPORT uint32_t ldg_net_steamworks_voice_speaking_set(uint64_t steam_id, uint8_t speaking);
LDG_EXPORT uint32_t ldg_net_steamworks_played_with_set(uint64_t steam_id);

LDG_EXPORT uint32_t ldg_net_steamworks_overlay_activate(const char *dialog);
LDG_EXPORT uint32_t ldg_net_steamworks_overlay_user_activate(const char *dialog, uint64_t steam_id);
LDG_EXPORT uint32_t ldg_net_steamworks_overlay_web_page_activate(const char *url, ldg_net_steamworks_web_page_mode_t mode);
LDG_EXPORT uint32_t ldg_net_steamworks_overlay_store_activate(uint32_t app_id, ldg_net_steamworks_store_flag_t flag);
LDG_EXPORT uint32_t ldg_net_steamworks_overlay_lobby_invite_activate(uint64_t lobby_id);
LDG_EXPORT uint32_t ldg_net_steamworks_overlay_remote_play_invite_activate(uint64_t lobby_id);
LDG_EXPORT uint32_t ldg_net_steamworks_overlay_connect_invite_activate(const char *connect);
LDG_EXPORT uint32_t ldg_net_steamworks_overlay_protocol_reg(const char *protocol);

LDG_EXPORT uint8_t ldg_net_steamworks_profile_item_has(uint64_t steam_id, ldg_net_steamworks_profile_item_t item);
LDG_EXPORT uint32_t ldg_net_steamworks_profile_prop_str_get(uint64_t steam_id, ldg_net_steamworks_profile_item_t item, ldg_net_steamworks_profile_prop_t prop, char *out, uint64_t out_size);
LDG_EXPORT uint32_t ldg_net_steamworks_profile_prop_uint_get(uint64_t steam_id, ldg_net_steamworks_profile_item_t item, ldg_net_steamworks_profile_prop_t prop);

LDG_EXPORT uint32_t ldg_net_steamworks_friend_call_init(ldg_net_steamworks_friend_call_ctx_t *ctx);
LDG_EXPORT void ldg_net_steamworks_friend_call_handle(ldg_net_steamworks_friend_call_ctx_t *ctx, int32_t pipe, uint64_t call);
LDG_EXPORT uint32_t ldg_net_steamworks_follower_cunt_req(ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t steam_id);
LDG_EXPORT ldg_net_steamworks_friend_call_state_t ldg_net_steamworks_follower_cunt_state_get(const ldg_net_steamworks_friend_call_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_follower_cunt_res_get(const ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t *out_steam_id, uint32_t *out_cunt);
LDG_EXPORT uint32_t ldg_net_steamworks_following_req(ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t steam_id);
LDG_EXPORT ldg_net_steamworks_friend_call_state_t ldg_net_steamworks_following_state_get(const ldg_net_steamworks_friend_call_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_following_res_get(const ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t *out_steam_id, uint8_t *out_following);
LDG_EXPORT uint32_t ldg_net_steamworks_following_list_req(ldg_net_steamworks_friend_call_ctx_t *ctx, uint32_t start_idx);
LDG_EXPORT ldg_net_steamworks_friend_call_state_t ldg_net_steamworks_following_list_state_get(const ldg_net_steamworks_friend_call_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_following_list_res_get(const ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t *out_steam_id, uint32_t cap, uint32_t *out_cunt, uint32_t *out_total);
LDG_EXPORT uint32_t ldg_net_steamworks_profile_items_req(ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t steam_id);
LDG_EXPORT ldg_net_steamworks_friend_call_state_t ldg_net_steamworks_profile_items_state_get(const ldg_net_steamworks_friend_call_ctx_t *ctx);
LDG_EXPORT uint32_t ldg_net_steamworks_profile_items_res_get(const ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t *out_steam_id, uint8_t *out_item, uint32_t cap);

LDG_EXPORT uint32_t ldg_net_steamworks_app_id_get(void);
LDG_EXPORT uint32_t ldg_net_steamworks_dlc_install(uint32_t app_id);
LDG_EXPORT uint8_t ldg_net_steamworks_dlc_installed_is(uint32_t app_id);
LDG_EXPORT uint32_t ldg_net_steamworks_dlc_progress_get(uint32_t app_id, uint64_t *out_downloaded, uint64_t *out_total);
LDG_EXPORT uint32_t ldg_net_steamworks_app_dir_get(uint32_t app_id, char *out, uint64_t out_size);

LDG_EXPORT uint8_t ldg_net_steamworks_music_en_is(void);
LDG_EXPORT uint8_t ldg_net_steamworks_music_playing_is(void);
LDG_EXPORT ldg_net_steamworks_music_status_t ldg_net_steamworks_music_status_get(void);
LDG_EXPORT uint32_t ldg_net_steamworks_music_play(void);
LDG_EXPORT uint32_t ldg_net_steamworks_music_pause(void);
LDG_EXPORT uint32_t ldg_net_steamworks_music_next_play(void);
LDG_EXPORT uint32_t ldg_net_steamworks_music_prev_play(void);
LDG_EXPORT uint32_t ldg_net_steamworks_music_vol_set(double vol);
LDG_EXPORT uint32_t ldg_net_steamworks_music_vol_get(double *out);

LDG_EXPORT uint32_t ldg_net_steamworks_timeline_mode_set(ldg_net_steamworks_timeline_mode_t mode);
LDG_EXPORT uint32_t ldg_net_steamworks_timeline_tooltip_set(const char *desc, double offset);
LDG_EXPORT uint32_t ldg_net_steamworks_timeline_tooltip_clear(double offset);
LDG_EXPORT uint32_t ldg_net_steamworks_timeline_event_add(const char *title, const char *desc, const char *icon, uint32_t priority);
LDG_EXPORT uint32_t ldg_net_steamworks_timeline_phase_start(void);
LDG_EXPORT uint32_t ldg_net_steamworks_timeline_phase_end(void);

#endif
