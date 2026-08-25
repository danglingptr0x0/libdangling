#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <dangling/core/err.h>
#include <dangling/math/math.h>
#include <dangling/core/macros.h>
#include <dangling/mem/alloc.h>
#include <dangling/mem/secure.h>
#include <dangling/str/str.h>
#include <dangling/parse/parse.h>
#include <dangling/time/time.h>
#include <dangling/net/net.h>
#include <dangling/net/steamworks.h>
#include <dangling/net/steamworks_lobby.h>
#include <dangling/net/steamworks_lboard.h>
#include <dangling/net/steamworks_econ.h>
#include <dangling/net/steamworks_wshop.h>
#include <dangling/net/steamworks_tkt.h>
#include <dangling/net/steamworks_srv.h>

#define STEAM_CONN_INVALID ((uint32_t)0)
#define STEAM_ID_INVALID ((uint64_t)0)
#define STEAM_APP_ID_INVALID ((uint32_t)0)
#define STEAM_CALL_INVALID ((uint64_t)0)
#define STEAM_LISTEN_INVALID ((uint32_t)0)
#define STEAM_POLL_GROUP_INVALID ((uint32_t)0)
#define STEAM_RES_OK 1
#define STEAM_SEND_UNRELIABLE_NO_NAGLE 1
#define STEAM_SEND_RELIABLE_NO_NAGLE 9

#define STEAM_CONN_STATE_CONNECTING 1
#define STEAM_CONN_STATE_CONNECTED 3
#define STEAM_CONN_STATE_CLOSED_BY_PEER 4
#define STEAM_CONN_STATE_PROBLEM_LOCAL 5

#define STEAM_CONF_IP_ALLOW_WITHOUT_AUTH 23
#define STEAM_CONF_SEND_BUFF_SIZE 9
#define STEAM_CONF_EN 1
#define STEAM_SEND_BUFF_SIZE ((int64_t)(16 * LDG_MIB))
#define STEAM_CONF_CB_CONN_STATUS 201
#define STEAM_CONF_CONN_USER_DATA 40
#define STEAM_CONF_TYPE_INT32 1
#define STEAM_CONF_TYPE_INT64 2
#define STEAM_CONF_TYPE_PTR 5

#define STEAM_IDENT_TYPE_STEAMID 16
#define STEAM_IDENT_WIDTH 136
#define STEAM_IDENT_DATA_OFFSET 8

#define STEAM_CB_UTILS 700
#define STEAM_CB_API_CALL_COMPLETED (STEAM_CB_UTILS + 3)
#define STEAM_CB_NET_SOCK 1220
#define STEAM_CB_CONN_STATUS_CHANGED (STEAM_CB_NET_SOCK + 1)
#define STEAM_CB_FRIENDS 300
#define STEAM_CB_FRIEND_CHAT_MSG (STEAM_CB_FRIENDS + 43)
#define STEAM_CB_PERSONA_STATE_CHANGE (STEAM_CB_FRIENDS + 4)
#define STEAM_CB_LOBBY_JOIN_REQ (STEAM_CB_FRIENDS + 33)
#define STEAM_CB_RICH_PRESENCE_JOIN_REQ (STEAM_CB_FRIENDS + 37)
#define STEAM_CB_FOLLOWER_CUNT (STEAM_CB_FRIENDS + 44)
#define STEAM_CB_IS_FOLLOWING (STEAM_CB_FRIENDS + 45)
#define STEAM_CB_FOLLOWING_LIST (STEAM_CB_FRIENDS + 46)
#define STEAM_CB_EQUIPPED_PROFILE_ITEMS (STEAM_CB_FRIENDS + 51)
#define STEAM_RES_CALL_OK 1
#define STEAM_FOLLOWING_PAGE_MAX 50

#define STEAM_RICH_PRESENCE_VAL_MAX 256

#define STEAM_CHAT_ENTRY_CHAT_MSG 1
#define STEAM_FRIEND_MSG_RING_STEP 1
#define STEAM_PERSONA_CHANGE_NAME 0x0001
#define STEAM_PERSONA_CHANGE_NAME_FIRST_SET 0x0400
#define STEAM_FRIEND_NAME_UNKNOWN "[unknown]"

#define STEAM_RECV_MSG_MAX 16
#define STEAM_ERR_MSG_SIZE 1024
#define STEAM_CMDLINE_SIZE 512
#define STEAM_SRV_LOGON_TIMEOUT 5.0
#define STEAM_VOICE_PKT_MAX 16384

typedef struct steam_conf_val
{
    int32_t opt;
    int32_t data_type;
    int64_t val;
} steam_conf_val_t;

typedef struct steam_ip_v4
{
    uint64_t zeros;
    uint16_t zero;
    uint16_t ffff;
    uint8_t ip[4];
} LDG_PACKED steam_ip_v4_t;

typedef union steam_ip
{
    uint8_t ipv6[16];
    steam_ip_v4_t v4;
} LDG_PACKED steam_ip_t;

typedef struct steam_ip_addr
{
    steam_ip_t addr;
    uint16_t port;
} LDG_PACKED steam_ip_addr_t;

typedef struct steam_net_ident
{
    int32_t type;
    int32_t cb_size;
    uint8_t data[128];
} steam_net_ident_t;

typedef struct steam_conn_info
{
    uint8_t remote_ident[STEAM_IDENT_WIDTH];
    int64_t user_data;
    uint32_t listen_sock;
    steam_ip_addr_t addr_remote;
    uint16_t pudding0;
    uint32_t pop_remote;
    uint32_t pop_relay;
    int32_t state;
    int32_t end_reason;
    char end_debug[128];
    char conn_description[128];
    int32_t flags;
    uint32_t reserved[63];
} steam_conn_info_t;

typedef struct steam_msg
{
    void *data;
    int32_t size;
    uint32_t conn;
    uint8_t ident_peer[STEAM_IDENT_WIDTH];
    int64_t conn_user_data;
    int64_t time_recv;
    int64_t msg_number;
    void (*free_data)(struct steam_msg *msg);
    void (*release)(struct steam_msg *msg);
    int32_t channel;
    int32_t flags;
    int64_t user_data;
    uint16_t lane_idx;
    uint16_t pudding;
} steam_msg_t;

typedef struct steam_cb_msg
{
    int32_t steam_user;
    int32_t cb_id;
    uint8_t *param;
    int32_t param_size;
} steam_cb_msg_t;

typedef struct steam_call_completed
{
    uint64_t async_call;
    int32_t cb_id;
    uint32_t param_size;
} steam_call_completed_t;

typedef struct steam_conn_status_change
{
    uint32_t conn;
#ifdef _WIN32
    uint8_t pudding0[4];
#endif
    steam_conn_info_t info;
    int32_t old_state;
} LDG_PACKED steam_conn_status_change_t;

typedef struct steam_friend_chat_msg
{
    uint64_t steam_id_user;
    int32_t msg_id;
} LDG_PACKED steam_friend_chat_msg_t;

typedef struct steam_persona_state_change
{
    uint64_t steam_id;
    int32_t change_flags;
} LDG_PACKED steam_persona_state_change_t;

typedef struct steam_lobby_join_req
{
    uint64_t lobby_id;
    uint64_t friend_steam_id;
} LDG_PACKED steam_lobby_join_req_t;

typedef struct steam_rich_presence_join_req
{
    uint64_t friend_steam_id;
    char connect[STEAM_RICH_PRESENCE_VAL_MAX];
} LDG_PACKED steam_rich_presence_join_req_t;

typedef struct steam_friend_game_info
{
    uint64_t game_id;
    uint32_t ip;
    uint16_t port;
    uint16_t query_port;
    uint64_t lobby_id;
} LDG_PACKED steam_friend_game_info_t;

typedef struct steam_follower_cunt_res
{
    int32_t res;
#ifdef _WIN32
    uint8_t pudding0[4];
#endif
    uint64_t steam_id;
    int32_t cunt;
} LDG_PACKED steam_follower_cunt_res_t;

typedef struct steam_is_following_res
{
    int32_t res;
#ifdef _WIN32
    uint8_t pudding0[4];
#endif
    uint64_t steam_id;
    uint8_t is_following;
} LDG_PACKED steam_is_following_res_t;

typedef struct steam_following_list_res
{
    int32_t res;
#ifdef _WIN32
    uint8_t pudding0[4];
#endif
    uint64_t steam_id[STEAM_FOLLOWING_PAGE_MAX];
    int32_t filled_cunt;
    int32_t total_cunt;
} LDG_PACKED steam_following_list_res_t;

typedef struct steam_profile_items_res
{
    int32_t res;
#ifdef _WIN32
    uint8_t pudding0[4];
#endif
    uint64_t steam_id;
    uint8_t item[LDG_NET_STEAMWORKS_PROFILE_ITEM_CUNT];
    uint8_t from_cache;
} LDG_PACKED steam_profile_items_res_t;

#define STEAMWORKS_LAYOUT_CK(name, cond) typedef char steam_layout_ck_ ## name[(cond) ? 1 : -1]

STEAMWORKS_LAYOUT_CK(ip_addr_size, sizeof(steam_ip_addr_t) == 18);
STEAMWORKS_LAYOUT_CK(net_ident_size, sizeof(steam_net_ident_t) == STEAM_IDENT_WIDTH);
STEAMWORKS_LAYOUT_CK(conn_info_user_data, offsetof(steam_conn_info_t, user_data) == 136);
STEAMWORKS_LAYOUT_CK(conn_info_listen_sock, offsetof(steam_conn_info_t, listen_sock) == 144);
STEAMWORKS_LAYOUT_CK(conn_info_addr_remote, offsetof(steam_conn_info_t, addr_remote) == 148);
STEAMWORKS_LAYOUT_CK(conn_info_pop_remote, offsetof(steam_conn_info_t, pop_remote) == 168);
STEAMWORKS_LAYOUT_CK(conn_info_state, offsetof(steam_conn_info_t, state) == 176);
STEAMWORKS_LAYOUT_CK(conn_info_end_debug, offsetof(steam_conn_info_t, end_debug) == 184);
STEAMWORKS_LAYOUT_CK(conn_info_conn_description, offsetof(steam_conn_info_t, conn_description) == 312);
STEAMWORKS_LAYOUT_CK(conn_info_flags, offsetof(steam_conn_info_t, flags) == 440);
STEAMWORKS_LAYOUT_CK(conn_info_size, sizeof(steam_conn_info_t) == 696);
STEAMWORKS_LAYOUT_CK(friend_chat_msg_id, offsetof(steam_friend_chat_msg_t, msg_id) == 8);
STEAMWORKS_LAYOUT_CK(friend_chat_msg_size, sizeof(steam_friend_chat_msg_t) == 12);
STEAMWORKS_LAYOUT_CK(persona_state_change_flags, offsetof(steam_persona_state_change_t, change_flags) == 8);
STEAMWORKS_LAYOUT_CK(persona_state_change_size, sizeof(steam_persona_state_change_t) == 12);
STEAMWORKS_LAYOUT_CK(lobby_join_req_friend, offsetof(steam_lobby_join_req_t, friend_steam_id) == 8);
STEAMWORKS_LAYOUT_CK(lobby_join_req_size, sizeof(steam_lobby_join_req_t) == 16);
STEAMWORKS_LAYOUT_CK(rich_presence_join_req_connect, offsetof(steam_rich_presence_join_req_t, connect) == 8);
STEAMWORKS_LAYOUT_CK(rich_presence_join_req_size, sizeof(steam_rich_presence_join_req_t) == 264);
STEAMWORKS_LAYOUT_CK(friend_game_info_lobby, offsetof(steam_friend_game_info_t, lobby_id) == 16);
STEAMWORKS_LAYOUT_CK(friend_game_info_size, sizeof(steam_friend_game_info_t) == 24);
#ifdef _WIN32
STEAMWORKS_LAYOUT_CK(follower_cunt_res_steam_id, offsetof(steam_follower_cunt_res_t, steam_id) == 8);
STEAMWORKS_LAYOUT_CK(is_following_res_steam_id, offsetof(steam_is_following_res_t, steam_id) == 8);
STEAMWORKS_LAYOUT_CK(following_list_res_steam_id, offsetof(steam_following_list_res_t, steam_id) == 8);
STEAMWORKS_LAYOUT_CK(profile_items_res_steam_id, offsetof(steam_profile_items_res_t, steam_id) == 8);
#else
STEAMWORKS_LAYOUT_CK(follower_cunt_res_steam_id, offsetof(steam_follower_cunt_res_t, steam_id) == 4);
STEAMWORKS_LAYOUT_CK(is_following_res_steam_id, offsetof(steam_is_following_res_t, steam_id) == 4);
STEAMWORKS_LAYOUT_CK(following_list_res_steam_id, offsetof(steam_following_list_res_t, steam_id) == 4);
STEAMWORKS_LAYOUT_CK(profile_items_res_steam_id, offsetof(steam_profile_items_res_t, steam_id) == 4);
#endif
STEAMWORKS_LAYOUT_CK(following_page_agree, STEAM_FOLLOWING_PAGE_MAX == LDG_NET_STEAMWORKS_FOLLOWING_PAGE_MAX);

extern int32_t SteamAPI_InitFlat(char *err_msg);
extern uint8_t SteamAPI_RestartAppIfNecessary(uint32_t app_id);
extern void SteamAPI_Shutdown(void);
extern void SteamAPI_ManualDispatch_Init(void);
extern void SteamAPI_ManualDispatch_RunFrame(int32_t pipe);
extern uint8_t SteamAPI_ManualDispatch_GetNextCallback(int32_t pipe, void *cb_msg);
extern void SteamAPI_ManualDispatch_FreeLastCallback(int32_t pipe);
extern int32_t SteamAPI_GetHSteamPipe(void);
extern int32_t SteamGameServer_GetHSteamPipe(void);
extern void SteamGameServer_Shutdown(void);
extern void* SteamAPI_SteamGameServer_v015(void);
extern void* SteamAPI_SteamNetworkingSockets_SteamAPI_v012(void);
extern void* SteamAPI_SteamGameServerNetworkingSockets_SteamAPI_v012(void);
extern uint32_t SteamAPI_ISteamNetworkingSockets_CreateListenSocketIP(void *self, const steam_ip_addr_t *local_addr, int32_t opt_cunt, const void *opt);
extern uint8_t SteamAPI_ISteamNetworkingSockets_CloseListenSocket(void *self, uint32_t listen_sock);
extern uint32_t SteamAPI_ISteamNetworkingSockets_ConnectByIPAddress(void *self, const steam_ip_addr_t *addr, int32_t opt_cunt, const void *opt);
extern int32_t SteamAPI_ISteamNetworkingSockets_AcceptConnection(void *self, uint32_t conn);
extern uint8_t SteamAPI_ISteamNetworkingSockets_CloseConnection(void *self, uint32_t conn, int32_t reason, const char *debug, uint8_t linger_en);
extern int32_t SteamAPI_ISteamNetworkingSockets_SendMessageToConnection(void *self, uint32_t conn, const void *data, uint32_t size, int32_t send_flags, int64_t *out_msg_number);
extern int32_t SteamAPI_ISteamNetworkingSockets_ReceiveMessagesOnConnection(void *self, uint32_t conn, steam_msg_t **out_msg, int32_t max_msg);
extern int32_t SteamAPI_ISteamNetworkingSockets_ReceiveMessagesOnPollGroup(void *self, uint32_t poll_group, steam_msg_t **out_msg, int32_t max_msg);
extern uint32_t SteamAPI_ISteamNetworkingSockets_CreatePollGroup(void *self);
extern uint8_t SteamAPI_ISteamNetworkingSockets_DestroyPollGroup(void *self, uint32_t poll_group);
extern uint8_t SteamAPI_ISteamNetworkingSockets_SetConnectionPollGroup(void *self, uint32_t conn, uint32_t poll_group);
extern uint8_t SteamAPI_ISteamNetworkingSockets_GetConnectionInfo(void *self, uint32_t conn, steam_conn_info_t *info);
extern uint32_t SteamAPI_ISteamNetworkingSockets_CreateListenSocketP2P(void *self, int32_t virtual_port, int32_t opt_cunt, const void *opt);
extern uint32_t SteamAPI_ISteamNetworkingSockets_ConnectP2P(void *self, const void *ident_remote, int32_t virtual_port, int32_t opt_cunt, const void *opt);
extern void SteamAPI_SteamNetworkingIdentity_SetSteamID64(void *ident, uint64_t steam_id);
extern void* SteamAPI_SteamUser_v023(void);
extern uint64_t SteamAPI_ISteamUser_GetSteamID(void *self);
extern void* SteamAPI_SteamFriends_v018(void);
extern uint8_t SteamAPI_ISteamFriends_SetRichPresence(void *self, const char *key, const char *val);
extern void SteamAPI_ISteamFriends_ClearRichPresence(void *self);
extern const char* SteamAPI_ISteamFriends_GetPersonaName(void *self);
extern const char* SteamAPI_ISteamFriends_GetFriendPersonaName(void *self, uint64_t steam_id_friend);
extern uint8_t SteamAPI_ISteamFriends_RequestUserInformation(void *self, uint64_t steam_id_user, uint8_t require_name_only);
extern int32_t SteamAPI_ISteamFriends_GetMediumFriendAvatar(void *self, uint64_t steam_id);
extern uint8_t SteamAPI_ISteamFriends_SetListenForFriendsMessages(void *self, uint8_t intercept_en);
extern int32_t SteamAPI_ISteamFriends_GetFriendMessage(void *self, uint64_t steam_id_friend, int32_t msg_id, void *data, int32_t data_size, int32_t *entry_type);
extern uint8_t SteamAPI_ISteamFriends_ReplyToFriendMessage(void *self, uint64_t steam_id_friend, const char *text);
extern int32_t SteamAPI_ISteamFriends_GetPersonaState(void *self);
extern int32_t SteamAPI_ISteamFriends_GetFriendCount(void *self, int32_t flags);
extern uint64_t SteamAPI_ISteamFriends_GetFriendByIndex(void *self, int32_t idx, int32_t flags);
extern int32_t SteamAPI_ISteamFriends_GetFriendRelationship(void *self, uint64_t steam_id);
extern int32_t SteamAPI_ISteamFriends_GetFriendPersonaState(void *self, uint64_t steam_id);
extern uint8_t SteamAPI_ISteamFriends_GetFriendGamePlayed(void *self, uint64_t steam_id, steam_friend_game_info_t *info);
extern const char* SteamAPI_ISteamFriends_GetFriendPersonaNameHistory(void *self, uint64_t steam_id, int32_t name_idx);
extern int32_t SteamAPI_ISteamFriends_GetFriendSteamLevel(void *self, uint64_t steam_id);
extern const char* SteamAPI_ISteamFriends_GetPlayerNickname(void *self, uint64_t steam_id);
extern int32_t SteamAPI_ISteamFriends_GetFriendsGroupCount(void *self);
extern int16_t SteamAPI_ISteamFriends_GetFriendsGroupIDByIndex(void *self, int32_t group_idx);
extern const char* SteamAPI_ISteamFriends_GetFriendsGroupName(void *self, int16_t group_id);
extern int32_t SteamAPI_ISteamFriends_GetFriendsGroupMembersCount(void *self, int16_t group_id);
extern void SteamAPI_ISteamFriends_GetFriendsGroupMembersList(void *self, int16_t group_id, uint64_t *out_steam_id, int32_t member_cunt);
extern uint8_t SteamAPI_ISteamFriends_HasFriend(void *self, uint64_t steam_id, int32_t flags);
extern void SteamAPI_ISteamFriends_SetInGameVoiceSpeaking(void *self, uint64_t steam_id, uint8_t speaking);
extern void SteamAPI_ISteamFriends_ActivateGameOverlay(void *self, const char *dialog);
extern void SteamAPI_ISteamFriends_ActivateGameOverlayToUser(void *self, const char *dialog, uint64_t steam_id);
extern void SteamAPI_ISteamFriends_ActivateGameOverlayToWebPage(void *self, const char *url, int32_t mode);
extern void SteamAPI_ISteamFriends_ActivateGameOverlayToStore(void *self, uint32_t app_id, int32_t flag);
extern void SteamAPI_ISteamFriends_SetPlayedWith(void *self, uint64_t steam_id);
extern void SteamAPI_ISteamFriends_ActivateGameOverlayInviteDialog(void *self, uint64_t lobby_id);
extern int32_t SteamAPI_ISteamFriends_GetSmallFriendAvatar(void *self, uint64_t steam_id);
extern int32_t SteamAPI_ISteamFriends_GetLargeFriendAvatar(void *self, uint64_t steam_id);
extern const char* SteamAPI_ISteamFriends_GetFriendRichPresence(void *self, uint64_t steam_id, const char *key);
extern int32_t SteamAPI_ISteamFriends_GetFriendRichPresenceKeyCount(void *self, uint64_t steam_id);
extern const char* SteamAPI_ISteamFriends_GetFriendRichPresenceKeyByIndex(void *self, uint64_t steam_id, int32_t key_idx);
extern void SteamAPI_ISteamFriends_RequestFriendRichPresence(void *self, uint64_t steam_id);
extern uint8_t SteamAPI_ISteamFriends_InviteUserToGame(void *self, uint64_t steam_id, const char *connect);
extern int32_t SteamAPI_ISteamFriends_GetCoplayFriendCount(void *self);
extern uint64_t SteamAPI_ISteamFriends_GetCoplayFriend(void *self, int32_t coplay_idx);
extern int32_t SteamAPI_ISteamFriends_GetFriendCoplayTime(void *self, uint64_t steam_id);
extern uint32_t SteamAPI_ISteamFriends_GetFriendCoplayGame(void *self, uint64_t steam_id);
extern int32_t SteamAPI_ISteamFriends_GetNumChatsWithUnreadPriorityMessages(void *self);
extern void SteamAPI_ISteamFriends_ActivateGameOverlayRemotePlayTogetherInviteDialog(void *self, uint64_t lobby_id);
extern uint8_t SteamAPI_ISteamFriends_RegisterProtocolInOverlayBrowser(void *self, const char *protocol);
extern void SteamAPI_ISteamFriends_ActivateGameOverlayInviteDialogConnectString(void *self, const char *connect);
extern uint8_t SteamAPI_ISteamFriends_BHasEquippedProfileItem(void *self, uint64_t steam_id, int32_t item);
extern const char* SteamAPI_ISteamFriends_GetProfileItemPropertyString(void *self, uint64_t steam_id, int32_t item, int32_t prop);
extern uint32_t SteamAPI_ISteamFriends_GetProfileItemPropertyUint(void *self, uint64_t steam_id, int32_t item, int32_t prop);
extern uint64_t SteamAPI_ISteamFriends_GetFollowerCount(void *self, uint64_t steam_id);
extern uint64_t SteamAPI_ISteamFriends_IsFollowing(void *self, uint64_t steam_id);
extern uint64_t SteamAPI_ISteamFriends_EnumerateFollowingList(void *self, uint32_t start_idx);
extern uint64_t SteamAPI_ISteamFriends_RequestEquippedProfileItems(void *self, uint64_t steam_id);
extern uint8_t SteamAPI_ManualDispatch_GetAPICallResult(int32_t pipe, uint64_t call, void *cb, int32_t cb_size, int32_t cb_expected, uint8_t *failed);
extern void* SteamAPI_SteamMusic_v001(void);
extern uint8_t SteamAPI_ISteamMusic_BIsEnabled(void *self);
extern uint8_t SteamAPI_ISteamMusic_BIsPlaying(void *self);
extern int32_t SteamAPI_ISteamMusic_GetPlaybackStatus(void *self);
extern void SteamAPI_ISteamMusic_Play(void *self);
extern void SteamAPI_ISteamMusic_Pause(void *self);
extern void SteamAPI_ISteamMusic_PlayPrevious(void *self);
extern void SteamAPI_ISteamMusic_PlayNext(void *self);
extern void SteamAPI_ISteamMusic_SetVolume(void *self, float vol);
extern float SteamAPI_ISteamMusic_GetVolume(void *self);
extern void* SteamAPI_SteamApps_v009(void);
extern int32_t SteamAPI_ISteamApps_GetLaunchCommandLine(void *self, char *cmd_line, int32_t cmd_line_size);
extern void SteamAPI_ISteamApps_InstallDLC(void *self, uint32_t app_id);
extern uint8_t SteamAPI_ISteamApps_BIsDlcInstalled(void *self, uint32_t app_id);
extern uint8_t SteamAPI_ISteamApps_GetDlcDownloadProgress(void *self, uint32_t app_id, uint64_t *downloaded, uint64_t *total);
extern uint32_t SteamAPI_ISteamApps_GetAppInstallDir(void *self, uint32_t app_id, char *folder, uint32_t folder_size);
extern void* SteamAPI_SteamUserStats_v013(void);
extern uint8_t SteamAPI_ISteamUserStats_GetStatInt32(void *self, const char *name, int32_t *data);
extern uint8_t SteamAPI_ISteamUserStats_SetStatInt32(void *self, const char *name, int32_t data);
extern uint8_t SteamAPI_ISteamUserStats_StoreStats(void *self);
extern uint8_t SteamAPI_ISteamUserStats_GetAchievement(void *self, const char *name, uint8_t *achieved);
extern uint8_t SteamAPI_ISteamUserStats_SetAchievement(void *self, const char *name);
extern uint8_t SteamAPI_ISteamUserStats_ClearAchievement(void *self, const char *name);
extern uint8_t SteamAPI_ISteamUserStats_IndicateAchievementProgress(void *self, const char *name, uint32_t cur, uint32_t max);
extern uint8_t SteamAPI_ISteamUserStats_UpdateAvgRateStat(void *self, const char *name, float cunt, double sesh_len);
extern void* SteamAPI_SteamUtils_v010(void);
extern uint8_t SteamAPI_ISteamUtils_GetImageSize(void *self, int32_t img, uint32_t *width, uint32_t *height);
extern uint8_t SteamAPI_ISteamUtils_GetImageRGBA(void *self, int32_t img, uint8_t *dst, int32_t dst_size);
extern uint32_t SteamAPI_ISteamUtils_GetAppID(void *self);
extern void* SteamAPI_SteamTimeline_v004(void);
extern void SteamAPI_ISteamTimeline_SetTimelineGameMode(void *self, int32_t mode);
extern void SteamAPI_ISteamTimeline_SetTimelineTooltip(void *self, const char *desc, float offset);
extern void SteamAPI_ISteamTimeline_ClearTimelineTooltip(void *self, float offset);
extern uint64_t SteamAPI_ISteamTimeline_AddInstantaneousTimelineEvent(void *self, const char *title, const char *desc, const char *icon, uint32_t priority, float offset, int32_t clip_priority);
extern void SteamAPI_ISteamTimeline_StartGamePhase(void *self);
extern void SteamAPI_ISteamTimeline_EndGamePhase(void *self);

static void ip_addr_clear(steam_ip_addr_t *addr)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!addr)) { return; }

    ret = ldg_mem_secure_zero(addr, sizeof(steam_ip_addr_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: ip addr zero failed"); }
}

static void ip_addr_v4_set(steam_ip_addr_t *addr, uint32_t ip, uint16_t port)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!addr)) { return; }

    ret = ldg_mem_secure_zero(addr, sizeof(steam_ip_addr_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: ip addr zero failed"); }

    addr->addr.v4.zeros = 0;
    addr->addr.v4.zero = 0;
    addr->addr.v4.ffff = 0xffff;
    addr->addr.v4.ip[0] = (uint8_t)(ip >> 24);
    addr->addr.v4.ip[1] = (uint8_t)(ip >> 16);
    addr->addr.v4.ip[2] = (uint8_t)(ip >> 8);
    addr->addr.v4.ip[3] = (uint8_t)(ip);
    addr->port = port;
}

static uint32_t ipv4_parse(const char *host, uint32_t *out_host)
{
    uint32_t octet[4] = LDG_ARR_ZERO_INIT;
    uint32_t val = 0;
    uint32_t idx = 0;
    uint32_t digit_cunt = 0;
    const char *cur = 0x0;

    if (LDG_UNLIKELY(host == 0x0 || out_host == 0x0)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_host = 0;

    cur = host;
    while (*cur != LDG_STR_TERM)
    {
        if (*cur >= '0' && *cur <= '9')
        {
            val = val * 10 + (uint32_t)(*cur - '0');
            if (val > 255) { return LDG_ERR_FUNC_ARG_INVALID; }

            digit_cunt = digit_cunt + 1;
        }
        else if (*cur == '.')
        {
            if (digit_cunt == 0 || idx >= 3) { return LDG_ERR_FUNC_ARG_INVALID; }

            octet[idx] = val;
            idx = idx + 1;
            val = 0;
            digit_cunt = 0;
        }
        else { return LDG_ERR_FUNC_ARG_INVALID; }

        cur = cur + 1;
    }

    if (idx != 3 || digit_cunt == 0) { return LDG_ERR_FUNC_ARG_INVALID; }

    octet[3] = val;

    *out_host = (octet[0] << 24) | (octet[1] << 16) | (octet[2] << 8) | octet[3];

    return LDG_ERR_AOK;
}

static uint32_t pkt_send(ldg_net_ctx_t *ctx, uint32_t conn, const void *data, uint64_t len, uint8_t reliable)
{
    int64_t out_msg_number = 0;
    int32_t res = 0;
    int32_t send_flags = 0;

    if (LDG_UNLIKELY(!ctx || !ctx->sock || conn == STEAM_CONN_INVALID)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!data || len == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    send_flags = (reliable != 0) ? STEAM_SEND_RELIABLE_NO_NAGLE : STEAM_SEND_UNRELIABLE_NO_NAGLE;
    res = SteamAPI_ISteamNetworkingSockets_SendMessageToConnection(ctx->sock, conn, data, (uint32_t)len, send_flags, &out_msg_number);

    if (LDG_UNLIKELY(res != STEAM_RES_OK))
    {
        LDG_ERRLOG_ERR("net: send failed");
        return LDG_ERR_NET_RPC_FAIL;
    }

    return LDG_ERR_AOK;
}

static uint32_t client_id_by_conn_get(ldg_net_steamworks_ctx_t *steam, uint32_t conn)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!steam || !steam->net.client)) { return 0; }

    for (idx = 0; idx < LDG_NET_CLIENT_MAX; idx = idx + 1) { if (steam->net.client[idx].is_conn != 0 && steam->net.client[idx].conn == conn) { return steam->net.client[idx].id; } }

    return 0;
}

static void conn_state_handle(ldg_net_steamworks_ctx_t *steam, uint32_t conn, const ldg_net_cb_t *cb)
{
    steam_conn_info_t info = LDG_STRUCT_ZERO_INIT;

    if (LDG_UNLIKELY(!steam || !steam->net.sock)) { return; }

    if (SteamAPI_ISteamNetworkingSockets_GetConnectionInfo(steam->net.sock, conn, &info) == 0)
    {
        LDG_ERRLOG_INFO("net: failed to get conn info");
        return;
    }

    LDG_ERRLOG_INFO("net: conn state");

    if (steam->net.mode == LDG_NET_MODE_SRV)
    {
        if (info.state == STEAM_CONN_STATE_CONNECTING)
        {
            uint64_t client_steam_id = 0;
            uint32_t client_id = 0;
            uint32_t ident_type = 0;
            uint32_t ret = 0;
            uint32_t idx = 0;
            uint8_t welcome[8] = LDG_ARR_ZERO_INIT;

            if (SteamAPI_ISteamNetworkingSockets_AcceptConnection(steam->net.sock, conn) != STEAM_RES_OK)
            {
                if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_CloseConnection(steam->net.sock, conn, 0, 0x0, 0) == 0)) { LDG_ERRLOG_WARN("net: close after failed accept failed"); }

                LDG_ERRLOG_WARN("net: failed to accept conn");
                return;
            }

            if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_SetConnectionPollGroup(steam->net.sock, conn, steam->net.poll_group) == 0)) { LDG_ERRLOG_WARN("net: set poll group failed"); }

            ret = ldg_mem_secure_cpy(&ident_type, info.remote_ident, sizeof(uint32_t));
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: ident type copy failed"); }

            if (ident_type == STEAM_IDENT_TYPE_STEAMID)
            {
                ret = ldg_mem_secure_cpy(&client_steam_id, info.remote_ident + STEAM_IDENT_DATA_OFFSET, sizeof(uint64_t));
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: client steam id copy failed"); }
            }

            for (idx = 0; idx < LDG_NET_CLIENT_MAX; idx = idx + 1)
            {
                if (steam->net.client[idx].is_conn == 0)
                {
                    void *friends = 0x0;
                    uint32_t name_err = 0;

                    client_id = idx + 1;
                    steam->net.client[idx].steam_id = client_steam_id;
                    steam->net.client[idx].conn = conn;
                    steam->net.client[idx].id = client_id;
                    steam->net.client[idx].is_conn = 1;

                    friends = SteamAPI_SteamFriends_v018();
                    if (friends && client_steam_id != 0)
                    {
                        const char *persona = SteamAPI_ISteamFriends_GetFriendPersonaName(friends, client_steam_id);
                        if (!persona || ldg_str_rbrcpy(steam->net.client[idx].name, persona, sizeof(steam->net.client[idx].name)) != 0)
                        {
                            name_err = ldg_str_rbrcpy(steam->net.client[idx].name, "N/A", sizeof(steam->net.client[idx].name));
                            if (LDG_UNLIKELY(name_err != LDG_ERR_AOK && name_err != LDG_ERR_STR_TRUNC)) { LDG_ERRLOG_WARN("net: client name fallback copy failed"); }
                        }
                    }
                    else
                    {
                        name_err = ldg_str_rbrcpy(steam->net.client[idx].name, "not-a-friend!", sizeof(steam->net.client[idx].name));
                        if (LDG_UNLIKELY(name_err != LDG_ERR_AOK && name_err != LDG_ERR_STR_TRUNC)) { LDG_ERRLOG_WARN("net: client name copy failed"); }
                    }

                    steam->net.client_cunt = steam->net.client_cunt + 1;
                    break;
                }
            }

            if (client_id == 0)
            {
                if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_CloseConnection(steam->net.sock, conn, 0, "srv full", 0) == 0)) { LDG_ERRLOG_WARN("net: close on full srv failed"); }

                LDG_ERRLOG_WARN("net: srv full; rejecting conn");
                return;
            }

            LDG_ERRLOG_INFO("net: client conn");

            welcome[0] = LDG_NET_SRV_CLIENT_WELCOME;
            welcome[1] = (uint8_t)client_id;
            welcome[2] = (uint8_t)steam->net.client_cunt;
            if (LDG_UNLIKELY(pkt_send(&steam->net, conn, welcome, sizeof(welcome), 1) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: welcome send failed"); }

            if (cb && cb->on_conn) { cb->on_conn(&steam->net, client_id); }
        }
        else if (info.state == STEAM_CONN_STATE_CLOSED_BY_PEER || info.state == STEAM_CONN_STATE_PROBLEM_LOCAL)
        {
            uint32_t client_id = client_id_by_conn_get(steam, conn);

            if (client_id > 0 && client_id <= LDG_NET_CLIENT_MAX)
            {
                uint8_t msg[4] = LDG_ARR_ZERO_INIT;

                steam->net.client[client_id - 1].is_conn = 0;
                steam->net.client[client_id - 1].conn = STEAM_CONN_INVALID;
                if (steam->net.client_cunt > 0) { steam->net.client_cunt = steam->net.client_cunt - 1; }

                LDG_ERRLOG_INFO("net: client dc'd");

                msg[0] = LDG_NET_SRV_CLIENT_LEAVE;
                msg[1] = (uint8_t)client_id;
                if (LDG_UNLIKELY(ldg_net_steamworks_bcast(steam, msg, sizeof(msg), 1) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: client leave bcast failed"); }

                if (cb && cb->on_dc) { cb->on_dc(&steam->net, client_id); }
            }

            if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_CloseConnection(steam->net.sock, conn, 0, 0x0, 0) == 0)) { LDG_ERRLOG_WARN("net: close conn failed"); }
        }
    }
    else if (steam->net.mode == LDG_NET_MODE_CLIENT)
    {
        if (info.state == STEAM_CONN_STATE_CONNECTED)
        {
            steam->net.status = LDG_NET_CONN_STATUS_CONN;
            LDG_ERRLOG_INFO("net: conn to srv");
        }
        else if (info.state == STEAM_CONN_STATE_CLOSED_BY_PEER || info.state == STEAM_CONN_STATE_PROBLEM_LOCAL)
        {
            steam->net.status = LDG_NET_CONN_STATUS_DCD;
            LDG_ERRLOG_INFO("net: dcd from srv");

            if (cb && cb->on_dc) { cb->on_dc(&steam->net, steam->net.local_id); }

            if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_CloseConnection(steam->net.sock, steam->net.srv_conn, 0, 0x0, 0) == 0)) { LDG_ERRLOG_WARN("net: close srv conn failed"); }

            steam->net.srv_conn = STEAM_CONN_INVALID;
        }
    }
}

static void conn_status_cb(steam_conn_status_change_t *change)
{
    ldg_net_steamworks_ctx_t *steam = 0x0;

    if (LDG_UNLIKELY(!change)) { return; }

    steam = (ldg_net_steamworks_ctx_t *)(uintptr_t)change->info.user_data;
    if (LDG_UNLIKELY(!steam || !steam->net.is_init)) { return; }

    LDG_ERRLOG_INFO("net: conn status cb");

    conn_state_handle(steam, change->conn, steam->net.cb);
}

static void recv_srv_handle(ldg_net_steamworks_ctx_t *steam, steam_msg_t *msg, const ldg_net_cb_t *cb)
{
    uint8_t msg_type = 0;
    uint32_t client_id = 0;

    if (LDG_UNLIKELY(!steam || !msg || !msg->data || msg->size < 1)) { return; }

    msg_type = ((uint8_t *)msg->data)[0];
    client_id = client_id_by_conn_get(steam, msg->conn);

    switch (msg_type)
    {
        case LDG_NET_CLIENT_SRV_NAME:
        {
            uint32_t idx = 0;
            char name[32] = LDG_ARR_ZERO_INIT;
            uint32_t ret = 0;

            if (msg->size < 36 || client_id == 0 || client_id > LDG_NET_CLIENT_MAX) { break; }

            idx = client_id - 1;
            ret = ldg_mem_secure_cpy(name, (uint8_t *)msg->data + 1, 31);
            if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: client name copy failed"); break; }

            if (steam->net.client[idx].is_conn == 0 || name[0] == LDG_STR_TERM) { break; }

            if (ldg_str_rbrcpy(steam->net.client[idx].name, name, sizeof(steam->net.client[idx].name)) != 0) { break; }

            LDG_ERRLOG_INFO("net: client sent name");

            break;
        }
        case LDG_NET_CLIENT_SRV_CHAT:
        {
            if (msg->size >= 2 && cb && cb->on_chat)
            {
                uint8_t msg_len = ((uint8_t *)msg->data)[1];
                if ((uint32_t)msg->size >= 2u + (uint32_t)msg_len && msg_len > 0)
                {
                    char msg_text[256] = LDG_ARR_ZERO_INIT;
                    uint32_t ret = 0;
                    uint32_t idx = client_id - 1;
                    const char *sender = (client_id > 0 && client_id <= LDG_NET_CLIENT_MAX && steam->net.client[idx].is_conn) ? steam->net.client[idx].name : "Unknown";
                    uint8_t sender_len = 0;
                    uint64_t bcast_len = 0;
                    uint8_t *bcast_buff = 0x0;

                    ret = ldg_mem_secure_cpy(msg_text, (uint8_t *)msg->data + 2, msg_len);
                    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: chat text copy failed"); break; }

                    msg_text[msg_len] = LDG_STR_TERM;

                    cb->on_chat(&steam->net, sender, msg_text);

                    sender_len = (uint8_t)strlen(sender);
                    if (sender_len > 31) { sender_len = 31; }

                    bcast_len = 3u + (uint64_t)sender_len + (uint64_t)msg_len;
                    if (ldg_mem_alloc(bcast_len, (void **)&bcast_buff) == LDG_ERR_AOK)
                    {
                        uint32_t idx = 0;

                        bcast_buff[0] = LDG_NET_SRV_CLIENT_CHAT;
                        bcast_buff[1] = sender_len;
                        ret = ldg_mem_secure_cpy(bcast_buff + 2, sender, sender_len);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: chat sender copy failed"); }

                        bcast_buff[2 + sender_len] = msg_len;
                        ret = ldg_mem_secure_cpy(bcast_buff + 3 + sender_len, msg_text, msg_len);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: chat body copy failed"); }

                        for (idx = 0; idx < LDG_NET_CLIENT_MAX; idx = idx + 1) { if (steam->net.client[idx].is_conn && steam->net.client[idx].id != client_id) { if (LDG_UNLIKELY(pkt_send(&steam->net, steam->net.client[idx].conn, bcast_buff, bcast_len, 1) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: chat bcast send failed"); } } }

                        ldg_mem_dealloc(bcast_buff);
                        bcast_buff = 0x0;
                    }
                }
            }

            break;
        }
        case LDG_NET_CLIENT_SRV_VOICE:
        {
            if (msg->size >= 5 && cb && cb->on_voice)
            {
                uint32_t data_len = 0;
                uint32_t ret = 0;

                ret = ldg_mem_secure_cpy(&data_len, (uint8_t *)msg->data + 1, 4);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: voice len copy failed"); break; }

                if (data_len > 0 && data_len <= STEAM_VOICE_PKT_MAX && (uint32_t)msg->size >= 5 + data_len)
                {
                    const void *voice_data = (uint8_t *)msg->data + 5;
                    uint8_t *bcast_buff = 0x0;
                    uint64_t bcast_len = 9 + data_len;

                    cb->on_voice(&steam->net, client_id, voice_data, data_len);

                    if (ldg_mem_alloc(bcast_len, (void **)&bcast_buff) == LDG_ERR_AOK)
                    {
                        uint32_t idx = 0;

                        bcast_buff[0] = LDG_NET_SRV_CLIENT_VOICE;
                        ret = ldg_mem_secure_cpy(bcast_buff + 1, &client_id, 4);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: voice client id copy failed"); }

                        ret = ldg_mem_secure_cpy(bcast_buff + 5, &data_len, 4);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: voice len field copy failed"); }

                        ret = ldg_mem_secure_cpy(bcast_buff + 9, voice_data, data_len);
                        if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: voice data copy failed"); }

                        for (idx = 0; idx < LDG_NET_CLIENT_MAX; idx = idx + 1) { if (steam->net.client[idx].is_conn && steam->net.client[idx].id != client_id) { if (LDG_UNLIKELY(pkt_send(&steam->net, steam->net.client[idx].conn, bcast_buff, bcast_len, 0) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: voice bcast send failed"); } } }

                        ldg_mem_dealloc(bcast_buff);
                        bcast_buff = 0x0;
                    }
                }
            }

            break;
        }
        default:
        {
            if (cb && cb->on_data) { cb->on_data(&steam->net, client_id, msg->data, (uint32_t)msg->size); }

            break;
        }
    }
}

static void recv_client_handle(ldg_net_steamworks_ctx_t *steam, steam_msg_t *msg, const ldg_net_cb_t *cb)
{
    uint8_t msg_type = 0;

    if (LDG_UNLIKELY(!steam || !msg || !msg->data || msg->size < 1)) { return; }

    msg_type = ((uint8_t *)msg->data)[0];

    switch (msg_type)
    {
        case LDG_NET_SRV_CLIENT_WELCOME:
        {
            if (msg->size >= 3)
            {
                steam->net.local_id = ((uint8_t *)msg->data)[1];
                steam->net.client_cunt = ((uint8_t *)msg->data)[2];
                LDG_ERRLOG_INFO("net: welcome");
                if (cb && cb->on_conn) { cb->on_conn(&steam->net, steam->net.local_id); }
            }

            break;
        }
        case LDG_NET_SRV_CLIENT_JOIN:
        {
            if (msg->size >= 2)
            {
                uint32_t client_id = ((uint8_t *)msg->data)[1];
                steam->net.client_cunt = steam->net.client_cunt + 1;
                LDG_ERRLOG_INFO("net: client joined");
                if (cb && cb->on_conn) { cb->on_conn(&steam->net, client_id); }
            }

            break;
        }
        case LDG_NET_SRV_CLIENT_LEAVE:
        {
            if (msg->size >= 2)
            {
                uint32_t client_id = ((uint8_t *)msg->data)[1];
                if (steam->net.client_cunt > 0) { steam->net.client_cunt = steam->net.client_cunt - 1; }

                LDG_ERRLOG_INFO("net: client left");
                if (cb && cb->on_dc) { cb->on_dc(&steam->net, client_id); }
            }

            break;
        }
        case LDG_NET_SRV_CLIENT_ACK:
        {
            if (msg->size >= 5)
            {
                uint32_t acked_seq = 0;
                uint32_t ret = ldg_mem_secure_cpy(&acked_seq, (uint8_t *)msg->data + 1, 4);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: ack seq copy failed"); break; }

                steam->net.in_seq = acked_seq;
                LDG_ERRLOG_INFO("net: seq acked");
            }

            break;
        }
        case LDG_NET_SRV_CLIENT_CHAT:
        {
            if (msg->size >= 3 && cb && cb->on_chat)
            {
                uint8_t sender_len = ((uint8_t *)msg->data)[1];
                if ((uint32_t)msg->size >= 3u + (uint32_t)sender_len)
                {
                    char sender[32] = LDG_ARR_ZERO_INIT;
                    uint32_t sender_copy_err = 0;
                    uint8_t msg_len = 0;

                    if (sender_len > 31) { sender_len = 31; }

                    sender_copy_err = ldg_mem_secure_cpy(sender, (uint8_t *)msg->data + 2, sender_len);
                    if (LDG_UNLIKELY(sender_copy_err != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: chat sender copy failed"); break; }

                    sender[sender_len] = LDG_STR_TERM;

                    msg_len = ((uint8_t *)msg->data)[2 + sender_len];
                    if ((uint32_t)msg->size >= 3u + (uint32_t)sender_len + (uint32_t)msg_len)
                    {
                        char msg_text[256] = LDG_ARR_ZERO_INIT;
                        uint32_t body_copy_err = ldg_mem_secure_cpy(msg_text, (uint8_t *)msg->data + 3 + sender_len, msg_len);
                        if (LDG_UNLIKELY(body_copy_err != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: chat body copy failed"); break; }

                        msg_text[msg_len] = LDG_STR_TERM;

                        LDG_ERRLOG_INFO("net: chat from");
                        cb->on_chat(&steam->net, sender, msg_text);
                    }
                }
            }

            break;
        }
        case LDG_NET_SRV_CLIENT_VOICE:
        {
            if (msg->size >= 9 && cb && cb->on_voice)
            {
                uint32_t client_id = 0;
                uint32_t data_len = 0;
                uint32_t ret = 0;

                ret = ldg_mem_secure_cpy(&client_id, (uint8_t *)msg->data + 1, 4);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: voice client id copy failed"); break; }

                ret = ldg_mem_secure_cpy(&data_len, (uint8_t *)msg->data + 5, 4);
                if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: voice len copy failed"); break; }

                if ((uint32_t)msg->size >= 9 + data_len && data_len > 0)
                {
                    const void *voice_data = (uint8_t *)msg->data + 9;
                    cb->on_voice(&steam->net, client_id, voice_data, data_len);
                }
            }

            break;
        }
        default:
        {
            if (cb && cb->on_data) { cb->on_data(&steam->net, steam->net.local_id, msg->data, (uint32_t)msg->size); }

            break;
        }
    }
}

uint8_t ldg_net_steamworks_restart_needed_is(uint32_t app_id)
{
    if (LDG_UNLIKELY(app_id == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (SteamAPI_RestartAppIfNecessary(app_id) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_net_steamworks_init(void)
{
    void *err_msg = 0x0;
    uint32_t ret = 0;
    int32_t res = 0;
    int32_t steam_pipe = 0;

    steam_pipe = SteamAPI_GetHSteamPipe();
    if (LDG_UNLIKELY(steam_pipe != 0)) { return LDG_ERR_AOK; }

    ret = ldg_mem_alloc(STEAM_ERR_MSG_SIZE, &err_msg);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    res = SteamAPI_InitFlat((char *)err_msg);
    if (LDG_UNLIKELY(res != 0))
    {
        fprintf(stderr, "net: steam api init failed; res: %" PRId32 "; msg: %s\n", res, (char *)err_msg);
        ldg_mem_dealloc(err_msg);
        return LDG_ERR_NET_INIT;
    }

    ldg_mem_dealloc(err_msg);

    SteamAPI_ManualDispatch_Init();

    LDG_ERRLOG_INFO("net: steamworks client init");

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_shutdown(void)
{
    void *game_server = 0x0;
    int32_t steam_pipe = 0;

    game_server = SteamAPI_SteamGameServer_v015();
    steam_pipe = SteamAPI_GetHSteamPipe();

    if (game_server != 0x0) { SteamGameServer_Shutdown(); }
    else if (steam_pipe != 0) { SteamAPI_Shutdown(); }
    else { return; }

    LDG_ERRLOG_INFO("net: steamworks shutdown");
}

uint32_t ldg_net_steamworks_hub_init(ldg_net_steamworks_hub_t *hub)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!hub)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(hub, sizeof(ldg_net_steamworks_hub_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    hub->steam_pipe = SteamAPI_GetHSteamPipe();
    hub->srv_pipe = SteamGameServer_GetHSteamPipe();

    return LDG_ERR_AOK;
}

static void pipe_drain(ldg_net_steamworks_hub_t *hub, int32_t pipe)
{
    steam_cb_msg_t cb_msg = LDG_STRUCT_ZERO_INIT;
    steam_call_completed_t *done = 0x0;

    if (pipe == 0) { return; }

    SteamAPI_ManualDispatch_RunFrame(pipe);

    while (SteamAPI_ManualDispatch_GetNextCallback(pipe, &cb_msg) != 0)
    {
        if (cb_msg.cb_id == STEAM_CB_API_CALL_COMPLETED) { if (cb_msg.param != 0x0 && cb_msg.param_size >= (int32_t)sizeof(steam_call_completed_t))
            {
                done = (steam_call_completed_t *)cb_msg.param;
                ldg_net_steamworks_lobby_call_handle(hub->lobby, pipe, done->async_call);
                ldg_net_steamworks_lboard_call_handle(hub->lboard, pipe, done->async_call);
                ldg_net_steamworks_econ_call_handle(hub->econ, pipe, done->async_call);
                ldg_net_steamworks_wshop_call_handle(hub->wshop, pipe, done->async_call);
                ldg_net_steamworks_tkt_call_handle(hub->tkt, pipe, done->async_call);
                ldg_net_steamworks_friend_call_handle(hub->friend_call, pipe, done->async_call);
            }
        }
        else if (cb_msg.cb_id == STEAM_CB_CONN_STATUS_CHANGED) { if (cb_msg.param != 0x0) { conn_status_cb((steam_conn_status_change_t *)cb_msg.param); } }
        else
        {
            ldg_net_steamworks_lobby_cb_handle(hub->lobby, cb_msg.cb_id, cb_msg.param, cb_msg.param_size);
            ldg_net_steamworks_tkt_cb_handle(hub->tkt, cb_msg.cb_id, cb_msg.param, cb_msg.param_size);
            ldg_net_steamworks_srv_cb_handle(hub->srv, cb_msg.cb_id, cb_msg.param, cb_msg.param_size);
            ldg_net_steamworks_friend_msg_cb_handle(hub->friend_msg, cb_msg.cb_id, cb_msg.param, cb_msg.param_size);
            ldg_net_steamworks_join_cb_handle(hub->join, cb_msg.cb_id, cb_msg.param, cb_msg.param_size);
        }

        SteamAPI_ManualDispatch_FreeLastCallback(pipe);
    }
}

void ldg_net_steamworks_tick(ldg_net_steamworks_hub_t *hub)
{
    if (LDG_UNLIKELY(!hub)) { return; }

    if (hub->steam_pipe == 0) { hub->steam_pipe = SteamAPI_GetHSteamPipe(); }

    if (hub->srv_pipe == 0) { hub->srv_pipe = SteamGameServer_GetHSteamPipe(); }

    pipe_drain(hub, hub->steam_pipe);
    pipe_drain(hub, hub->srv_pipe);
}

uint32_t ldg_net_steamworks_srv_start(ldg_net_steamworks_ctx_t *steam, uint16_t port)
{
    steam_ip_addr_t addr = LDG_STRUCT_ZERO_INIT;
    steam_conf_val_t opt[3] = LDG_ARR_ZERO_INIT;
    void *game_server = 0x0;
    uint32_t ret = 0;
    uint64_t client_width = 0;

    if (LDG_UNLIKELY(!steam)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(steam, sizeof(ldg_net_steamworks_ctx_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    game_server = SteamAPI_SteamGameServer_v015();
    steam->net.is_srv = (game_server != 0x0) ? 1 : 0;
    steam->steam_pipe = SteamAPI_GetHSteamPipe();

    if (LDG_UNLIKELY(game_server == 0x0 && steam->steam_pipe == 0)) { return LDG_ERR_NOT_INIT; }

    if (port == 0) { port = LDG_NET_PORT_DEFAULT; }

    steam->net.sock = steam->net.is_srv ? SteamAPI_SteamGameServerNetworkingSockets_SteamAPI_v012() : SteamAPI_SteamNetworkingSockets_SteamAPI_v012();
    if (LDG_UNLIKELY(!steam->net.sock))
    {
        LDG_ERRLOG_ERR("net: failed to get networking sockets interface");
        return LDG_ERR_NET_RPC_FAIL;
    }

    steam->net.poll_group = SteamAPI_ISteamNetworkingSockets_CreatePollGroup(steam->net.sock);
    if (LDG_UNLIKELY(steam->net.poll_group == STEAM_POLL_GROUP_INVALID))
    {
        LDG_ERRLOG_ERR("net: failed to create poll group");
        return LDG_ERR_NET_RPC_FAIL;
    }

    ip_addr_clear(&addr);
    addr.port = port;

    LDG_ERRLOG_INFO("net: listen addr ipv6");

    opt[0].opt = STEAM_CONF_IP_ALLOW_WITHOUT_AUTH;
    opt[0].data_type = STEAM_CONF_TYPE_INT32;
    opt[0].val = STEAM_CONF_EN;
    opt[1].opt = STEAM_CONF_CONN_USER_DATA;
    opt[1].data_type = STEAM_CONF_TYPE_INT64;
    opt[1].val = (int64_t)(uintptr_t)steam;
    opt[2].opt = STEAM_CONF_SEND_BUFF_SIZE;
    opt[2].data_type = STEAM_CONF_TYPE_INT32;
    opt[2].val = STEAM_SEND_BUFF_SIZE;

    steam->net.listen_sock = SteamAPI_ISteamNetworkingSockets_CreateListenSocketIP(steam->net.sock, &addr, 3, opt);
    LDG_ERRLOG_INFO("net: listen sock");
    if (LDG_UNLIKELY(steam->net.listen_sock == STEAM_LISTEN_INVALID))
    {
        if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_DestroyPollGroup(steam->net.sock, steam->net.poll_group) == 0)) { LDG_ERRLOG_WARN("net: destroy poll group failed"); }

        LDG_ERRLOG_ERR("net: failed to create listen sock on port");
        return LDG_ERR_NET_RPC_FAIL;
    }

    client_width = LDG_NET_CLIENT_MAX * sizeof(ldg_net_client_t);
    if (LDG_UNLIKELY(ldg_mem_alloc(client_width, (void **)&steam->net.client) != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_CloseListenSocket(steam->net.sock, steam->net.listen_sock) == 0)) { LDG_ERRLOG_WARN("net: close listen sock failed"); }

        if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_DestroyPollGroup(steam->net.sock, steam->net.poll_group) == 0)) { LDG_ERRLOG_WARN("net: destroy poll group failed"); }

        return LDG_ERR_ALLOC_NULL;
    }

    ret = ldg_mem_secure_zero(steam->net.client, client_width);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    steam->net.mode = LDG_NET_MODE_SRV;
    steam->net.status = LDG_NET_CONN_STATUS_CONN;
    steam->net.port = port;
    steam->net.is_init = 1;
    steam->net.local_id = 0;
    steam->net.client_cunt = 0;
    steam->net.out_seq = 1;
    steam->net.in_seq = 0;
    steam->net.srv_conn = STEAM_CONN_INVALID;
    ldg_net_steamworks_rate_init(steam, LDG_NET_RATE_DEFAULT);

    LDG_ERRLOG_INFO("net: srv started on port");

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_srv_stop(ldg_net_steamworks_ctx_t *steam)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!steam || !steam->net.is_init)) { return; }

    if (LDG_UNLIKELY(steam->net.mode != LDG_NET_MODE_SRV)) { return; }

    if (steam->net.client)
    {
        for (idx = 0; idx < LDG_NET_CLIENT_MAX; idx = idx + 1) { if (steam->net.client[idx].is_conn && steam->net.client[idx].conn != STEAM_CONN_INVALID) { if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_CloseConnection(steam->net.sock, steam->net.client[idx].conn, 0, "srv shutdown", 0) == 0)) { LDG_ERRLOG_WARN("net: close client conn failed"); } } }

        ldg_mem_dealloc(steam->net.client);
        steam->net.client = 0x0;
    }

    if (steam->net.poll_group != STEAM_POLL_GROUP_INVALID)
    {
        if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_DestroyPollGroup(steam->net.sock, steam->net.poll_group) == 0)) { LDG_ERRLOG_WARN("net: destroy poll group failed"); }

        steam->net.poll_group = STEAM_POLL_GROUP_INVALID;
    }

    if (steam->net.listen_sock != STEAM_LISTEN_INVALID)
    {
        if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_CloseListenSocket(steam->net.sock, steam->net.listen_sock) == 0)) { LDG_ERRLOG_WARN("net: close listen sock failed"); }

        steam->net.listen_sock = STEAM_LISTEN_INVALID;
    }

    steam->net.is_init = 0;
    steam->net.status = LDG_NET_CONN_STATUS_DCD;

    LDG_ERRLOG_INFO("net: srv stopped");
}

uint32_t ldg_net_steamworks_srv_p2p_start(ldg_net_steamworks_ctx_t *steam, uint32_t virtual_port)
{
    steam_conf_val_t opt[3] = LDG_ARR_ZERO_INIT;
    void *game_server = 0x0;
    uint32_t ret = 0;
    uint64_t client_width = 0;

    if (LDG_UNLIKELY(!steam)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(steam, sizeof(ldg_net_steamworks_ctx_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    game_server = SteamAPI_SteamGameServer_v015();
    steam->net.is_srv = (game_server != 0x0) ? 1 : 0;
    steam->steam_pipe = SteamAPI_GetHSteamPipe();

    if (LDG_UNLIKELY(game_server == 0x0 && steam->steam_pipe == 0)) { return LDG_ERR_NOT_INIT; }

    steam->net.sock = steam->net.is_srv ? SteamAPI_SteamGameServerNetworkingSockets_SteamAPI_v012() : SteamAPI_SteamNetworkingSockets_SteamAPI_v012();
    if (LDG_UNLIKELY(!steam->net.sock))
    {
        LDG_ERRLOG_ERR("net: failed to get networking sockets interface");
        return LDG_ERR_NET_RPC_FAIL;
    }

    steam->net.poll_group = SteamAPI_ISteamNetworkingSockets_CreatePollGroup(steam->net.sock);
    if (LDG_UNLIKELY(steam->net.poll_group == STEAM_POLL_GROUP_INVALID))
    {
        LDG_ERRLOG_ERR("net: failed to create poll group");
        return LDG_ERR_NET_RPC_FAIL;
    }

    opt[0].opt = STEAM_CONF_CB_CONN_STATUS;
    opt[0].data_type = STEAM_CONF_TYPE_PTR;
    opt[0].val = (int64_t)(uintptr_t)conn_status_cb;
    opt[1].opt = STEAM_CONF_CONN_USER_DATA;
    opt[1].data_type = STEAM_CONF_TYPE_INT64;
    opt[1].val = (int64_t)(uintptr_t)steam;
    opt[2].opt = STEAM_CONF_SEND_BUFF_SIZE;
    opt[2].data_type = STEAM_CONF_TYPE_INT32;
    opt[2].val = STEAM_SEND_BUFF_SIZE;

    steam->net.listen_sock = SteamAPI_ISteamNetworkingSockets_CreateListenSocketP2P(steam->net.sock, (int32_t)virtual_port, 3, opt);
    LDG_ERRLOG_INFO("net: p2p listen sock");
    if (LDG_UNLIKELY(steam->net.listen_sock == STEAM_LISTEN_INVALID))
    {
        LDG_ERRLOG_ERR("net: failed to create P2P listen sock");
        if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_DestroyPollGroup(steam->net.sock, steam->net.poll_group) == 0)) { LDG_ERRLOG_WARN("net: destroy poll group failed"); }

        return LDG_ERR_NET_RPC_FAIL;
    }

    LDG_ERRLOG_INFO("net: registered per-sock conn status callback");

    client_width = LDG_NET_CLIENT_MAX * sizeof(ldg_net_client_t);
    if (LDG_UNLIKELY(ldg_mem_alloc(client_width, (void **)&steam->net.client) != LDG_ERR_AOK))
    {
        if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_CloseListenSocket(steam->net.sock, steam->net.listen_sock) == 0)) { LDG_ERRLOG_WARN("net: close listen sock failed"); }

        if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_DestroyPollGroup(steam->net.sock, steam->net.poll_group) == 0)) { LDG_ERRLOG_WARN("net: destroy poll group failed"); }

        return LDG_ERR_ALLOC_NULL;
    }

    ret = ldg_mem_secure_zero(steam->net.client, client_width);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return LDG_ERR_ALLOC_NULL; }

    steam->net.mode = LDG_NET_MODE_SRV;
    steam->net.status = LDG_NET_CONN_STATUS_CONN;
    steam->net.port = (uint16_t)virtual_port;
    steam->net.is_init = 1;
    steam->net.local_id = 0;
    steam->net.client_cunt = 0;
    steam->net.out_seq = 1;
    steam->net.in_seq = 0;
    steam->net.srv_conn = STEAM_CONN_INVALID;
    ldg_net_steamworks_rate_init(steam, LDG_NET_RATE_DEFAULT);

    if (game_server) { LDG_ERRLOG_INFO("net: P2P srv started"); }
    else { LDG_ERRLOG_INFO("net: P2P srv started on virtual port"); }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_client_conn(ldg_net_steamworks_ctx_t *steam, const char *host, uint16_t port)
{
    steam_ip_addr_t addr = LDG_STRUCT_ZERO_INIT;
    steam_conf_val_t opt[3] = LDG_ARR_ZERO_INIT;
    uint32_t host_order = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!steam || !host)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(steam, sizeof(ldg_net_steamworks_ctx_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    steam->steam_pipe = SteamAPI_GetHSteamPipe();
    if (LDG_UNLIKELY(steam->steam_pipe == 0)) { return LDG_ERR_NOT_INIT; }

    if (port == 0) { port = LDG_NET_PORT_DEFAULT; }

    steam->net.sock = SteamAPI_SteamNetworkingSockets_SteamAPI_v012();
    if (LDG_UNLIKELY(!steam->net.sock))
    {
        LDG_ERRLOG_ERR("net: failed to get networking sockets interface");
        return LDG_ERR_NET_RPC_FAIL;
    }

    if (ipv4_parse(host, &host_order) != LDG_ERR_AOK)
    {
        LDG_ERRLOG_ERR("net: failed to parse host addr");
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    ip_addr_v4_set(&addr, host_order, port);

    LDG_ERRLOG_INFO("net: addr ipv6");

    opt[0].opt = STEAM_CONF_IP_ALLOW_WITHOUT_AUTH;
    opt[0].data_type = STEAM_CONF_TYPE_INT32;
    opt[0].val = STEAM_CONF_EN;
    opt[1].opt = STEAM_CONF_CONN_USER_DATA;
    opt[1].data_type = STEAM_CONF_TYPE_INT64;
    opt[1].val = (int64_t)(uintptr_t)steam;
    opt[2].opt = STEAM_CONF_SEND_BUFF_SIZE;
    opt[2].data_type = STEAM_CONF_TYPE_INT32;
    opt[2].val = STEAM_SEND_BUFF_SIZE;

    steam->net.srv_conn = SteamAPI_ISteamNetworkingSockets_ConnectByIPAddress(steam->net.sock, &addr, 3, opt);
    LDG_ERRLOG_INFO("net: conn handle");
    if (LDG_UNLIKELY(steam->net.srv_conn == STEAM_CONN_INVALID))
    {
        LDG_ERRLOG_ERR("net: failed to init conn");
        return LDG_ERR_NET_CONN;
    }

    steam->net.mode = LDG_NET_MODE_CLIENT;
    steam->net.status = LDG_NET_CONN_STATUS_CONNECTING;
    steam->net.port = port;
    steam->net.is_init = 1;
    steam->net.out_seq = 1;
    steam->net.in_seq = 0;
    steam->net.listen_sock = STEAM_LISTEN_INVALID;
    steam->net.poll_group = STEAM_POLL_GROUP_INVALID;
    steam->net.client = 0x0;
    ldg_net_steamworks_rate_init(steam, LDG_NET_RATE_DEFAULT);

    LDG_ERRLOG_INFO("net: connecting");

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_client_p2p_conn(ldg_net_steamworks_ctx_t *steam, uint64_t srv_steam_id, uint32_t virtual_port)
{
    steam_net_ident_t ident = LDG_STRUCT_ZERO_INIT;
    steam_conf_val_t opt[2] = LDG_ARR_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!steam)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(srv_steam_id == 0))
    {
        LDG_ERRLOG_ERR("net: invalid server Steam ID");
        return LDG_ERR_FUNC_ARG_INVALID;
    }

    ret = ldg_mem_secure_zero(steam, sizeof(ldg_net_steamworks_ctx_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    steam->steam_pipe = SteamAPI_GetHSteamPipe();
    if (LDG_UNLIKELY(steam->steam_pipe == 0))
    {
        LDG_ERRLOG_ERR("net: Steam API not init; cannot connect P2P");
        return LDG_ERR_NOT_INIT;
    }

    steam->net.sock = SteamAPI_SteamNetworkingSockets_SteamAPI_v012();
    if (LDG_UNLIKELY(!steam->net.sock))
    {
        LDG_ERRLOG_ERR("net: failed to get networking sockets interface");
        return LDG_ERR_NET_RPC_FAIL;
    }

    SteamAPI_SteamNetworkingIdentity_SetSteamID64(&ident, srv_steam_id);

    opt[0].opt = STEAM_CONF_CONN_USER_DATA;
    opt[0].data_type = STEAM_CONF_TYPE_INT64;
    opt[0].val = (int64_t)(uintptr_t)steam;
    opt[1].opt = STEAM_CONF_SEND_BUFF_SIZE;
    opt[1].data_type = STEAM_CONF_TYPE_INT32;
    opt[1].val = STEAM_SEND_BUFF_SIZE;

    steam->net.srv_conn = SteamAPI_ISteamNetworkingSockets_ConnectP2P(steam->net.sock, &ident, (int32_t)virtual_port, 2, opt);
    LDG_ERRLOG_INFO("net: P2P conn handle");
    if (LDG_UNLIKELY(steam->net.srv_conn == STEAM_CONN_INVALID))
    {
        LDG_ERRLOG_ERR("net: failed to init P2P conn");
        return LDG_ERR_NET_CONN;
    }

    steam->net.mode = LDG_NET_MODE_CLIENT;
    steam->net.status = LDG_NET_CONN_STATUS_CONNECTING;
    steam->net.port = (uint16_t)virtual_port;
    steam->net.is_init = 1;
    steam->net.out_seq = 1;
    steam->net.in_seq = 0;
    steam->net.listen_sock = STEAM_LISTEN_INVALID;
    steam->net.poll_group = STEAM_POLL_GROUP_INVALID;
    steam->net.client = 0x0;
    ldg_net_steamworks_rate_init(steam, LDG_NET_RATE_DEFAULT);

    LDG_ERRLOG_INFO("net: connecting to Steam ID");

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_client_dc(ldg_net_steamworks_ctx_t *steam)
{
    if (LDG_UNLIKELY(!steam || !steam->net.is_init)) { return; }

    if (LDG_UNLIKELY(steam->net.mode != LDG_NET_MODE_CLIENT)) { return; }

    if (steam->net.srv_conn != STEAM_CONN_INVALID)
    {
        if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_CloseConnection(steam->net.sock, steam->net.srv_conn, 0, "client dc", 1) == 0)) { LDG_ERRLOG_WARN("net: close srv conn failed"); }

        steam->net.srv_conn = STEAM_CONN_INVALID;
    }

    steam->net.is_init = 0;
    steam->net.status = LDG_NET_CONN_STATUS_DCD;

    LDG_ERRLOG_INFO("net: client dcd");
}

uint32_t ldg_net_steamworks_poll(ldg_net_steamworks_ctx_t *steam, const ldg_net_cb_t *cb, uint32_t timeout)
{
    uint32_t event_cunt = 0;
    double now_poll = 0.0;
    double deadline = 0.0;
    steam_conn_info_t info = LDG_STRUCT_ZERO_INIT;
    steam_msg_t *msg[STEAM_RECV_MSG_MAX] = LDG_ARR_ZERO_INIT;
    int32_t msg_cunt = 0;
    int32_t idx = 0;

    if (LDG_UNLIKELY(!steam || !steam->net.is_init || !steam->net.sock)) { return UINT32_MAX; }

    steam->net.cb = cb;

    ldg_time_monotonic_get(&now_poll);
    deadline = now_poll + (double)timeout / LDG_MS_PER_SEC;

    if (steam->net.mode == LDG_NET_MODE_SRV && steam->net.poll_group != STEAM_POLL_GROUP_INVALID)
    {
        do
        {
            msg_cunt = SteamAPI_ISteamNetworkingSockets_ReceiveMessagesOnPollGroup(steam->net.sock, steam->net.poll_group, msg, STEAM_RECV_MSG_MAX);

            for (idx = 0; idx < msg_cunt; idx = idx + 1) { if (msg[idx])
                {
                    if (SteamAPI_ISteamNetworkingSockets_GetConnectionInfo(steam->net.sock, msg[idx]->conn, &info) != 0) { if (info.state == STEAM_CONN_STATE_CONNECTING || info.state == STEAM_CONN_STATE_CLOSED_BY_PEER || info.state == STEAM_CONN_STATE_PROBLEM_LOCAL) { conn_state_handle(steam, msg[idx]->conn, cb); } }

                    recv_srv_handle(steam, msg[idx], cb);
                    msg[idx]->release(msg[idx]);
                    event_cunt = event_cunt + 1;
                }
            }

            ldg_time_monotonic_get(&now_poll);
        }
        while (msg_cunt == STEAM_RECV_MSG_MAX && now_poll < deadline);
    }
    else if (steam->net.mode == LDG_NET_MODE_CLIENT && steam->net.srv_conn != STEAM_CONN_INVALID)
    {
        if (SteamAPI_ISteamNetworkingSockets_GetConnectionInfo(steam->net.sock, steam->net.srv_conn, &info) != 0)
        {
            if (info.state != STEAM_CONN_STATE_CONNECTED || steam->net.status != LDG_NET_CONN_STATUS_CONN) { LDG_ERRLOG_INFO("net: client conn state"); }

            if (info.state == STEAM_CONN_STATE_CONNECTING) { LDG_ERRLOG_INFO("net: client connecting..."); }
            else if (info.state == STEAM_CONN_STATE_CONNECTED && steam->net.status == LDG_NET_CONN_STATUS_CONNECTING) { conn_state_handle(steam, steam->net.srv_conn, cb); }
            else if (info.state == STEAM_CONN_STATE_CLOSED_BY_PEER || info.state == STEAM_CONN_STATE_PROBLEM_LOCAL) { conn_state_handle(steam, steam->net.srv_conn, cb); }
        }
        else { LDG_ERRLOG_INFO("net: failed to get client conn info"); }

        do
        {
            msg_cunt = SteamAPI_ISteamNetworkingSockets_ReceiveMessagesOnConnection(steam->net.sock, steam->net.srv_conn, msg, STEAM_RECV_MSG_MAX);

            for (idx = 0; idx < msg_cunt; idx = idx + 1) { if (msg[idx])
                {
                    recv_client_handle(steam, msg[idx], cb);
                    msg[idx]->release(msg[idx]);
                    event_cunt = event_cunt + 1;
                }
            }

            ldg_time_monotonic_get(&now_poll);
        }
        while (msg_cunt == STEAM_RECV_MSG_MAX && now_poll < deadline);
    }

    return event_cunt;
}

void ldg_net_steamworks_rate_init(ldg_net_steamworks_ctx_t *steam, double rate)
{
    double now = 0.0;

    if (LDG_UNLIKELY(!steam)) { return; }

    ldg_time_monotonic_get(&now);
    steam->net.cleartime = now;
    steam->net.rate = (rate > LDG_NET_RATE_MIN) ? rate : LDG_NET_RATE_DEFAULT;
    steam->net.bytes_sent = 0;
}

uint8_t ldg_net_steamworks_rate_ready_is(ldg_net_steamworks_ctx_t *steam)
{
    double now = 0.0;
    uint8_t ready = LDG_TRUTH_FALSE;

    if (LDG_UNLIKELY(!steam)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (steam->net.rate < LDG_NET_RATE_MIN) { steam->net.rate = LDG_NET_RATE_DEFAULT; }

    ldg_time_monotonic_get(&now);

    ready = (now >= steam->net.cleartime) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return ready;
}

void ldg_net_steamworks_rate_update(ldg_net_steamworks_ctx_t *steam, uint32_t pkt_size)
{
    double now = 0.0;

    if (LDG_UNLIKELY(!steam)) { return; }

    if (steam->net.rate < LDG_NET_RATE_MIN) { steam->net.rate = LDG_NET_RATE_DEFAULT; }

    ldg_time_monotonic_get(&now);

    if (steam->net.cleartime < now) { steam->net.cleartime = now; }

    steam->net.cleartime = steam->net.cleartime + (double)pkt_size / steam->net.rate;
    steam->net.bytes_sent = steam->net.bytes_sent + pkt_size;
}

uint32_t ldg_net_steamworks_bcast(ldg_net_steamworks_ctx_t *steam, const void *data, uint64_t len, uint8_t reliable)
{
    uint32_t idx = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!steam || !steam->net.is_init || !data || len == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(steam->net.mode != LDG_NET_MODE_SRV)) { return LDG_ERR_INVALID; }

    for (idx = 0; idx < LDG_NET_CLIENT_MAX; idx = idx + 1)
    {
        if (steam->net.client[idx].is_conn == 0 || steam->net.client[idx].conn == STEAM_CONN_INVALID) { continue; }

        if (LDG_UNLIKELY(pkt_send(&steam->net, steam->net.client[idx].conn, data, len, reliable) != LDG_ERR_AOK))
        {
            LDG_ERRLOG_WARN("net: bcast send failed");
            ret = LDG_ERR_NET_RPC_FAIL;
        }
    }

    return ret;
}

uint32_t ldg_net_steamworks_send(ldg_net_steamworks_ctx_t *steam, const void *data, uint64_t len, uint8_t reliable)
{
    if (LDG_UNLIKELY(!steam || !steam->net.is_init || !data || len == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (steam->net.mode == LDG_NET_MODE_SRV) { return ldg_net_steamworks_bcast(steam, data, len, reliable); }

    if (LDG_UNLIKELY(steam->net.mode != LDG_NET_MODE_CLIENT)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(steam->net.srv_conn == STEAM_CONN_INVALID)) { return LDG_ERR_NOT_INIT; }

    if (LDG_UNLIKELY(steam->net.status != LDG_NET_CONN_STATUS_CONN)) { return LDG_ERR_CLOSED; }

    return pkt_send(&steam->net, steam->net.srv_conn, data, len, reliable);
}

uint32_t ldg_net_steamworks_client_kick(ldg_net_steamworks_ctx_t *steam, uint32_t client_id, const char *reason)
{
    uint32_t idx = 0;
    uint8_t msg[4] = LDG_ARR_ZERO_INIT;

    if (LDG_UNLIKELY(!steam || !steam->net.is_init)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(steam->net.mode != LDG_NET_MODE_SRV)) { return LDG_ERR_INVALID; }

    if (LDG_UNLIKELY(client_id == 0 || client_id > LDG_NET_CLIENT_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    idx = client_id - 1;

    if (!steam->net.client || !steam->net.client[idx].is_conn)
    {
        LDG_ERRLOG_WARN("net: kick failed, client not connected");
        return LDG_ERR_NOT_FOUND;
    }

    LDG_ERRLOG_INFO("net: kicking client");

    if (LDG_UNLIKELY(SteamAPI_ISteamNetworkingSockets_CloseConnection(steam->net.sock, steam->net.client[idx].conn, 0, reason, 0) == 0)) { LDG_ERRLOG_WARN("net: kick close conn failed"); }

    steam->net.client[idx].is_conn = 0;
    steam->net.client[idx].conn = 0;
    if (steam->net.client_cunt > 0) { steam->net.client_cunt = steam->net.client_cunt - 1; }

    msg[0] = LDG_NET_SRV_CLIENT_LEAVE;
    msg[1] = (uint8_t)client_id;
    if (LDG_UNLIKELY(ldg_net_steamworks_bcast(steam, msg, sizeof(msg), 1) != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: kick leave bcast failed"); }

    return LDG_ERR_AOK;
}

ldg_net_conn_status_t ldg_net_steamworks_status_get(const ldg_net_steamworks_ctx_t *steam)
{
    if (LDG_UNLIKELY(!steam)) { return LDG_NET_CONN_STATUS_DCD; }

    return steam->net.status;
}

uint32_t ldg_net_steamworks_local_id_get(const ldg_net_steamworks_ctx_t *steam)
{
    if (LDG_UNLIKELY(!steam)) { return 0; }

    return steam->net.local_id;
}

uint32_t ldg_net_steamworks_client_cunt_get(const ldg_net_steamworks_ctx_t *steam)
{
    if (LDG_UNLIKELY(!steam)) { return 0; }

    return steam->net.client_cunt;
}

uint64_t ldg_net_steamworks_client_steam_id_get(const ldg_net_steamworks_ctx_t *steam, uint32_t client_id)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!steam || !steam->net.client)) { return 0; }

    if (client_id == 0 || client_id > LDG_NET_CLIENT_MAX) { return 0; }

    idx = client_id - 1;
    if (steam->net.client[idx].is_conn == 0) { return 0; }

    return steam->net.client[idx].steam_id;
}

uint64_t ldg_net_steamworks_local_steam_id_get(void)
{
    void *user = 0x0;

    user = SteamAPI_SteamUser_v023();
    if (LDG_UNLIKELY(!user)) { return 0; }

    return SteamAPI_ISteamUser_GetSteamID(user);
}

const char* ldg_net_steamworks_local_name_get(void)
{
    void *friends = 0x0;
    const char *name = 0x0;

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return "Player"; }

    name = SteamAPI_ISteamFriends_GetPersonaName(friends);
    if (LDG_UNLIKELY(!name || name[0] == LDG_STR_TERM)) { return "Player"; }

    return name;
}

const char* ldg_net_steamworks_player_name_get(const ldg_net_ctx_t *ctx, uint32_t client_id)
{
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!ctx || !ctx->client)) { return "Unknown"; }

    if (client_id == 0 || client_id > LDG_NET_CLIENT_MAX) { return "Unknown"; }

    idx = client_id - 1;
    if (ctx->client[idx].is_conn == 0) { return "Unknown"; }

    if (ctx->client[idx].name[0] == LDG_STR_TERM) { return "Unknown"; }

    return ctx->client[idx].name;
}

uint32_t ldg_net_steamworks_launch_conn_get(char *out_conn, uint64_t out_len)
{
    void *apps = 0x0;
    char cmd_line[STEAM_CMDLINE_SIZE] = LDG_ARR_ZERO_INIT;
    int32_t len = 0;
    const char *connect_arg = 0x0;
    const char *val = 0x0;
    uint64_t idx = 0;

    if (LDG_UNLIKELY(!out_conn || out_len == 0)) { return LDG_ERR_FUNC_ARG_NULL; }

    out_conn[0] = LDG_STR_TERM;

    apps = SteamAPI_SteamApps_v009();
    if (!apps)
    {
        LDG_ERRLOG_INFO("net: failed to get apps interface");
        return LDG_ERR_NET_RPC_FAIL;
    }

    len = SteamAPI_ISteamApps_GetLaunchCommandLine(apps, cmd_line, sizeof(cmd_line));
    if (len <= 0) { return LDG_ERR_NOT_FOUND; }

    LDG_ERRLOG_INFO("net: launch command line");

    connect_arg = strstr(cmd_line, "+connect ");
    if (!connect_arg) { connect_arg = strstr(cmd_line, "--connect "); }

    if (!connect_arg) { connect_arg = strstr(cmd_line, "-c "); }

    if (connect_arg)
    {
        val = strchr(connect_arg, ' ');
        if (val)
        {
            val = val + 1;
            while (*val && *val != ' ' && idx < out_len - 1)
            {
                out_conn[idx] = *val;
                idx = idx + 1;
                val = val + 1;
            }
            out_conn[idx] = LDG_STR_TERM;
            if (idx > 0)
            {
                LDG_ERRLOG_INFO("net: launch connect arg");
                return LDG_ERR_AOK;
            }
        }
    }

    return LDG_ERR_NOT_FOUND;
}

void ldg_net_steamworks_joinable_set(uint64_t srv_steam_id, uint32_t client_cunt)
{
    void *friends = 0x0;
    char connect_str[64] = LDG_ARR_ZERO_INIT;
    char client_cunt_str[16] = LDG_ARR_ZERO_INIT;

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends))
    {
        LDG_ERRLOG_WARN("net: failed to get friends iface");
        return;
    }

    if (srv_steam_id != 0)
    {
        if (LDG_UNLIKELY(snprintf(connect_str, sizeof(connect_str), "steam:%" PRIu64, srv_steam_id) < 0)) { LDG_ERRLOG_WARN("net: connect string format failed"); }

        if (LDG_UNLIKELY(snprintf(client_cunt_str, sizeof(client_cunt_str), "%" PRIu32, client_cunt) < 0)) { LDG_ERRLOG_WARN("net: client cunt string format failed"); }

        if (LDG_UNLIKELY(SteamAPI_ISteamFriends_SetRichPresence(friends, "connect", connect_str) == 0)) { LDG_ERRLOG_WARN("net: connect presence set failed"); }

        if (LDG_UNLIKELY(SteamAPI_ISteamFriends_SetRichPresence(friends, "clients", client_cunt_str) == 0)) { LDG_ERRLOG_WARN("net: clients presence set failed"); }

        if (client_cunt <= 1) { if (LDG_UNLIKELY(SteamAPI_ISteamFriends_SetRichPresence(friends, "steam_display", "#OnlineSolo") == 0)) { LDG_ERRLOG_WARN("net: display presence set failed"); } }
        else { if (LDG_UNLIKELY(SteamAPI_ISteamFriends_SetRichPresence(friends, "steam_display", "#Online") == 0)) { LDG_ERRLOG_WARN("net: display presence set failed"); } }

        LDG_ERRLOG_INFO("net: set joinable via Steam");
    }
    else
    {
        SteamAPI_ISteamFriends_ClearRichPresence(friends);
        LDG_ERRLOG_INFO("net: cleared rich presence");
    }
}

void ldg_net_steamworks_presence_campaign_set(const char *track, const char *mission)
{
    void *friends = 0x0;

    friends = SteamAPI_SteamFriends_v018();
    if (!friends) { return; }

    if (LDG_UNLIKELY(SteamAPI_ISteamFriends_SetRichPresence(friends, "track", track ? track : "") == 0)) { LDG_ERRLOG_WARN("net: track presence set failed"); }

    if (LDG_UNLIKELY(SteamAPI_ISteamFriends_SetRichPresence(friends, "mission", mission ? mission : "") == 0)) { LDG_ERRLOG_WARN("net: mission presence set failed"); }

    if (LDG_UNLIKELY(SteamAPI_ISteamFriends_SetRichPresence(friends, "steam_display", "#Campaign") == 0)) { LDG_ERRLOG_WARN("net: display presence set failed"); }
}

void ldg_net_steamworks_presence_practice_set(void)
{
    void *friends = 0x0;

    friends = SteamAPI_SteamFriends_v018();
    if (!friends) { return; }

    if (LDG_UNLIKELY(SteamAPI_ISteamFriends_SetRichPresence(friends, "steam_display", "#Practice") == 0)) { LDG_ERRLOG_WARN("net: display presence set failed"); }
}

void ldg_net_steamworks_presence_menu_set(void)
{
    void *friends = 0x0;

    friends = SteamAPI_SteamFriends_v018();
    if (!friends) { return; }

    if (LDG_UNLIKELY(SteamAPI_ISteamFriends_SetRichPresence(friends, "steam_display", "#InMenu") == 0)) { LDG_ERRLOG_WARN("net: display presence set failed"); }
}

void ldg_net_steamworks_presence_set(const char *key, const char *val)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(!key)) { return; }

    friends = SteamAPI_SteamFriends_v018();
    if (!friends) { return; }

    if (LDG_UNLIKELY(SteamAPI_ISteamFriends_SetRichPresence(friends, key, val ? val : "") == 0)) { LDG_ERRLOG_WARN("net: presence set failed"); }
}

void ldg_net_steamworks_presence_clear(void)
{
    void *friends = 0x0;

    friends = SteamAPI_SteamFriends_v018();
    if (!friends) { return; }

    SteamAPI_ISteamFriends_ClearRichPresence(friends);
}

uint32_t ldg_net_steamworks_stat_int_get(const char *name, int32_t *out)
{
    void *stats = 0x0;

    if (LDG_UNLIKELY(!name || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    stats = SteamAPI_SteamUserStats_v013();
    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUserStats_GetStatInt32(stats, name, out) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_stat_int_set(const char *name, int32_t val)
{
    void *stats = 0x0;

    if (LDG_UNLIKELY(!name)) { return LDG_ERR_FUNC_ARG_NULL; }

    stats = SteamAPI_SteamUserStats_v013();
    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUserStats_SetStatInt32(stats, name, val) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_stat_st(void)
{
    void *stats = 0x0;

    stats = SteamAPI_SteamUserStats_v013();
    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUserStats_StoreStats(stats) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_stat_avgrate_update(const char *name, double cunt, double sesh_len)
{
    void *stats = 0x0;

    if (LDG_UNLIKELY(!name)) { return LDG_ERR_FUNC_ARG_NULL; }

    stats = SteamAPI_SteamUserStats_v013();
    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUserStats_UpdateAvgRateStat(stats, name, (float)cunt, sesh_len) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_achievement_get(const char *name, uint8_t *out)
{
    void *stats = 0x0;
    uint8_t achieved = 0;

    if (LDG_UNLIKELY(!name || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;
    stats = SteamAPI_SteamUserStats_v013();
    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUserStats_GetAchievement(stats, name, &achieved) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    *out = (achieved != 0) ? 1 : 0;
    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_achievement_set(const char *name)
{
    void *stats = 0x0;

    if (LDG_UNLIKELY(!name)) { return LDG_ERR_FUNC_ARG_NULL; }

    stats = SteamAPI_SteamUserStats_v013();
    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUserStats_SetAchievement(stats, name) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_achievement_clear(const char *name)
{
    void *stats = 0x0;

    if (LDG_UNLIKELY(!name)) { return LDG_ERR_FUNC_ARG_NULL; }

    stats = SteamAPI_SteamUserStats_v013();
    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUserStats_ClearAchievement(stats, name) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_achievement_progress_indicate(const char *name, uint32_t cur, uint32_t max)
{
    void *stats = 0x0;

    if (LDG_UNLIKELY(!name)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(max == 0 || cur > max)) { return LDG_ERR_FUNC_ARG_INVALID; }

    stats = SteamAPI_SteamUserStats_v013();
    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUserStats_IndicateAchievementProgress(stats, name, cur, max) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_avatar_get(uint8_t *rgba_out, uint32_t *width_out, uint32_t *height_out)
{
    void *user = 0x0;
    void *friends = 0x0;
    void *utils = 0x0;
    uint64_t steam_id = 0;
    int32_t avatar_handle = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    int32_t buff_size = 0;

    if (LDG_UNLIKELY(!rgba_out || !width_out || !height_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *width_out = 0;
    *height_out = 0;

    user = SteamAPI_SteamUser_v023();
    if (LDG_UNLIKELY(!user)) { return LDG_ERR_NET_RPC_FAIL; }

    steam_id = SteamAPI_ISteamUser_GetSteamID(user);
    if (steam_id == 0) { return LDG_ERR_NET_RPC_FAIL; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    avatar_handle = SteamAPI_ISteamFriends_GetMediumFriendAvatar(friends, steam_id);
    if (avatar_handle <= 0) { return LDG_ERR_NET_RPC_FAIL; }

    utils = SteamAPI_SteamUtils_v010();
    if (LDG_UNLIKELY(!utils)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUtils_GetImageSize(utils, avatar_handle, &width, &height) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    if (width == 0 || height == 0) { return LDG_ERR_NET_RPC_FAIL; }

    *width_out = width;
    *height_out = height;

    buff_size = (int32_t)(width * height * 4);
    if (SteamAPI_ISteamUtils_GetImageRGBA(utils, avatar_handle, rgba_out, buff_size) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_user_info_req(uint64_t steam_id_user, uint8_t require_name_only, uint8_t *out)
{
    void *friends = 0x0;
    uint8_t fetching = 0;

    if (LDG_UNLIKELY(steam_id_user == 0 || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0;

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    fetching = SteamAPI_ISteamFriends_RequestUserInformation(friends, steam_id_user, require_name_only);
    *out = (fetching != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_friend_msg_listen_set(uint8_t listen)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(listen > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamFriends_SetListenForFriendsMessages(friends, listen) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_friend_msg_init(ldg_net_steamworks_friend_msg_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_mem_secure_zero(ctx, sizeof(ldg_net_steamworks_friend_msg_ctx_t));
}

static void friend_chat_msg_push(ldg_net_steamworks_friend_msg_ctx_t *ctx, void *param, int32_t param_size)
{
    const steam_friend_chat_msg_t *chat = 0x0;
    ldg_net_steamworks_friend_msg_t *slot = 0x0;
    void *friends = 0x0;
    int32_t entry_type = 0;
    int32_t text_len = 0;
    uint32_t next = 0;

    if (LDG_UNLIKELY(param_size < (int32_t)sizeof(steam_friend_chat_msg_t))) { return; }

    next = (ctx->tail + STEAM_FRIEND_MSG_RING_STEP) % LDG_NET_STEAMWORKS_FRIEND_MSG_SLOT_MAX;
    if (LDG_UNLIKELY(next == ctx->hd))
    {
        ctx->drop_cunt++;
        LDG_ERRLOG_WARN("net: friend msg ring full; dropped");
        return;
    }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return; }

    chat = (const steam_friend_chat_msg_t *)param;
    slot = &ctx->slot[ctx->tail];

    text_len = SteamAPI_ISteamFriends_GetFriendMessage(friends, chat->steam_id_user, chat->msg_id, slot->text, (int32_t)sizeof(slot->text), &entry_type);
    if (text_len <= 0) { return; }

    if (entry_type != STEAM_CHAT_ENTRY_CHAT_MSG) { return; }

    slot->text[LDG_NET_STEAMWORKS_FRIEND_MSG_TEXT_MAX - LDG_STR_TERM_SIZE] = LDG_STR_TERM;
    slot->steam_id = chat->steam_id_user;
    slot->len = (uint32_t)strlen(slot->text);

    ctx->tail = next;
}

static void persona_state_handle(ldg_net_steamworks_friend_msg_ctx_t *ctx, void *param, int32_t param_size)
{
    const steam_persona_state_change_t *change = 0x0;
    int32_t name_mask = 0;

    if (LDG_UNLIKELY(param_size < (int32_t)sizeof(steam_persona_state_change_t))) { return; }

    change = (const steam_persona_state_change_t *)param;
    name_mask = STEAM_PERSONA_CHANGE_NAME | STEAM_PERSONA_CHANGE_NAME_FIRST_SET;

    if ((change->change_flags & name_mask) == 0) { return; }

    ctx->persona_rev++;
}

void ldg_net_steamworks_friend_msg_cb_handle(ldg_net_steamworks_friend_msg_ctx_t *ctx, int32_t cb_id, void *param, int32_t param_size)
{
    if (!ctx || !param) { return; }

    if (cb_id == STEAM_CB_FRIEND_CHAT_MSG) { friend_chat_msg_push(ctx, param, param_size); }
    else if (cb_id == STEAM_CB_PERSONA_STATE_CHANGE) { persona_state_handle(ctx, param, param_size); }
}

uint32_t ldg_net_steamworks_friend_msg_get(ldg_net_steamworks_friend_msg_ctx_t *ctx, ldg_net_steamworks_friend_msg_t *out)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (ctx->hd == ctx->tail) { return LDG_ERR_EMPTY; }

    ret = ldg_mem_secure_cpy(out, &ctx->slot[ctx->hd], sizeof(ldg_net_steamworks_friend_msg_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->hd = (ctx->hd + STEAM_FRIEND_MSG_RING_STEP) % LDG_NET_STEAMWORKS_FRIEND_MSG_SLOT_MAX;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_friend_msg_drop_cunt_get(const ldg_net_steamworks_friend_msg_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return UINT32_MAX; }

    return ctx->drop_cunt;
}

uint32_t ldg_net_steamworks_friend_msg_reply(uint64_t steam_id_friend, const char *text)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(steam_id_friend == 0 || !text)) { return LDG_ERR_FUNC_ARG_NULL; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamFriends_ReplyToFriendMessage(friends, steam_id_friend, text) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

const char* ldg_net_steamworks_friend_name_get(uint64_t steam_id_user)
{
    void *friends = 0x0;
    const char *name = 0x0;

    if (LDG_UNLIKELY(steam_id_user == 0)) { return 0x0; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return 0x0; }

    name = SteamAPI_ISteamFriends_GetFriendPersonaName(friends, steam_id_user);
    if (LDG_UNLIKELY(!name || name[0] == LDG_STR_TERM)) { return 0x0; }

    if (ldg_parse_streq_is(name, STEAM_FRIEND_NAME_UNKNOWN) == LDG_TRUTH_TRUE) { return 0x0; }

    return name;
}

uint32_t ldg_net_steamworks_persona_rev_get(const ldg_net_steamworks_friend_msg_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return UINT32_MAX; }

    return ctx->persona_rev;
}

uint32_t ldg_net_steamworks_join_init(ldg_net_steamworks_join_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_mem_secure_zero(ctx, sizeof(ldg_net_steamworks_join_ctx_t));
}

static void join_lobby_req_latch(ldg_net_steamworks_join_ctx_t *ctx, void *param, int32_t param_size)
{
    const steam_lobby_join_req_t *req = 0x0;

    if (LDG_UNLIKELY(param_size < (int32_t)sizeof(steam_lobby_join_req_t))) { return; }

    req = (const steam_lobby_join_req_t *)param;
    if (LDG_UNLIKELY(req->lobby_id == STEAM_ID_INVALID)) { return; }

    ctx->lobby_id = req->lobby_id;
    ctx->friend_steam_id = req->friend_steam_id;
    ctx->state = LDG_NET_STEAMWORKS_JOIN_STATE_LOBBY;
    LDG_ERRLOG_INFO("net: steam lobby join requested");
}

static void join_connect_req_latch(ldg_net_steamworks_join_ctx_t *ctx, void *param, int32_t param_size)
{
    const steam_rich_presence_join_req_t *req = 0x0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(param_size < (int32_t)sizeof(steam_rich_presence_join_req_t))) { return; }

    req = (const steam_rich_presence_join_req_t *)param;

    ret = ldg_mem_secure_cpy(ctx->connect, req->connect, STEAM_RICH_PRESENCE_VAL_MAX);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return; }

    ctx->connect[LDG_NET_STEAMWORKS_JOIN_CONNECT_MAX] = LDG_STR_TERM;
    if (LDG_UNLIKELY(ctx->connect[0] == LDG_STR_TERM)) { return; }

    ctx->friend_steam_id = req->friend_steam_id;
    ctx->state = LDG_NET_STEAMWORKS_JOIN_STATE_CONNECT;
    LDG_ERRLOG_INFO("net: steam rich presence join requested");
}

void ldg_net_steamworks_join_cb_handle(ldg_net_steamworks_join_ctx_t *ctx, int32_t cb_id, void *param, int32_t param_size)
{
    if (!ctx || !param) { return; }

    if (cb_id == STEAM_CB_LOBBY_JOIN_REQ) { join_lobby_req_latch(ctx, param, param_size); }
    else if (cb_id == STEAM_CB_RICH_PRESENCE_JOIN_REQ) { join_connect_req_latch(ctx, param, param_size); }
}

ldg_net_steamworks_join_state_t ldg_net_steamworks_join_state_get(const ldg_net_steamworks_join_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_NET_STEAMWORKS_JOIN_STATE_NONE; }

    return ctx->state;
}

uint32_t ldg_net_steamworks_join_lobby_get(ldg_net_steamworks_join_ctx_t *ctx, uint64_t *out_lobby_id)
{
    if (LDG_UNLIKELY(!ctx || !out_lobby_id)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_lobby_id = STEAM_ID_INVALID;

    if (ctx->state != LDG_NET_STEAMWORKS_JOIN_STATE_LOBBY) { return LDG_ERR_EMPTY; }

    *out_lobby_id = ctx->lobby_id;
    ctx->lobby_id = STEAM_ID_INVALID;
    ctx->state = LDG_NET_STEAMWORKS_JOIN_STATE_NONE;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_join_connect_get(ldg_net_steamworks_join_ctx_t *ctx, char *out, uint64_t out_size)
{
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    if (ctx->state != LDG_NET_STEAMWORKS_JOIN_STATE_CONNECT) { return LDG_ERR_EMPTY; }

    len = (uint64_t)strlen(ctx->connect);
    if (LDG_UNLIKELY(len + LDG_STR_TERM_SIZE > out_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    ret = ldg_mem_secure_cpy(out, ctx->connect, len + LDG_STR_TERM_SIZE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    ctx->connect[0] = LDG_STR_TERM;
    ctx->state = LDG_NET_STEAMWORKS_JOIN_STATE_NONE;

    return LDG_ERR_AOK;
}

uint64_t ldg_net_steamworks_join_friend_steam_id_get(const ldg_net_steamworks_join_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return STEAM_ID_INVALID; }

    return ctx->friend_steam_id;
}

static uint32_t friend_str_copy(const char *src, char *out, uint64_t out_size)
{
    uint64_t len = 0;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;
    if (LDG_UNLIKELY(!src)) { return LDG_ERR_NOT_FOUND; }

    len = (uint64_t)strlen(src);
    if (len == 0) { return LDG_ERR_NOT_FOUND; }

    if (LDG_UNLIKELY(len + LDG_STR_TERM_SIZE > out_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    ret = ldg_mem_secure_cpy(out, src, len + LDG_STR_TERM_SIZE);
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    return LDG_ERR_AOK;
}

ldg_net_steamworks_persona_state_t ldg_net_steamworks_persona_state_get(void)
{
    void *friends = 0x0;
    int32_t state = 0;

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_NET_STEAMWORKS_PERSONA_STATE_OFFLINE; }

    state = SteamAPI_ISteamFriends_GetPersonaState(friends);
    if (state < 0 || state > LDG_NET_STEAMWORKS_PERSONA_STATE_INVISIBLE) { return LDG_NET_STEAMWORKS_PERSONA_STATE_OFFLINE; }

    return (ldg_net_steamworks_persona_state_t)state;
}

ldg_net_steamworks_persona_state_t ldg_net_steamworks_friend_persona_state_get(uint64_t steam_id)
{
    void *friends = 0x0;
    int32_t state = 0;

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_NET_STEAMWORKS_PERSONA_STATE_OFFLINE; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_NET_STEAMWORKS_PERSONA_STATE_OFFLINE; }

    state = SteamAPI_ISteamFriends_GetFriendPersonaState(friends, steam_id);
    if (state < 0 || state > LDG_NET_STEAMWORKS_PERSONA_STATE_INVISIBLE) { return LDG_NET_STEAMWORKS_PERSONA_STATE_OFFLINE; }

    return (ldg_net_steamworks_persona_state_t)state;
}

ldg_net_steamworks_friend_rel_t ldg_net_steamworks_friend_rel_get(uint64_t steam_id)
{
    void *friends = 0x0;
    int32_t rel = 0;

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_NET_STEAMWORKS_FRIEND_REL_NONE; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_NET_STEAMWORKS_FRIEND_REL_NONE; }

    rel = SteamAPI_ISteamFriends_GetFriendRelationship(friends, steam_id);
    if (rel < 0 || rel > LDG_NET_STEAMWORKS_FRIEND_REL_IGNORED_FRIEND) { return LDG_NET_STEAMWORKS_FRIEND_REL_NONE; }

    return (ldg_net_steamworks_friend_rel_t)rel;
}

uint32_t ldg_net_steamworks_friend_cunt_get(uint32_t flags)
{
    void *friends = 0x0;
    int32_t cunt = 0;

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return UINT32_MAX; }

    cunt = SteamAPI_ISteamFriends_GetFriendCount(friends, (int32_t)flags);
    if (cunt < 0) { return UINT32_MAX; }

    return (uint32_t)cunt;
}

uint32_t ldg_net_steamworks_friend_list_get(uint32_t flags, uint64_t *out_steam_id, uint32_t cap, uint32_t *out_cunt)
{
    void *friends = 0x0;
    uint64_t steam_id = 0;
    int32_t total = 0;
    int32_t idx = 0;
    uint32_t filled = 0;

    if (LDG_UNLIKELY(!out_steam_id || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    total = SteamAPI_ISteamFriends_GetFriendCount(friends, (int32_t)flags);
    if (total < 0) { return LDG_ERR_NET_RPC_FAIL; }

    for (idx = 0; idx < total; idx++)
    {
        if (filled >= cap) { break; }

        steam_id = SteamAPI_ISteamFriends_GetFriendByIndex(friends, idx, (int32_t)flags);
        if (steam_id == STEAM_ID_INVALID) { continue; }

        out_steam_id[filled] = steam_id;
        filled++;
    }

    *out_cunt = filled;
    if (filled < (uint32_t)total) { return LDG_ERR_FULL; }

    return LDG_ERR_AOK;
}

uint8_t ldg_net_steamworks_friend_has(uint64_t steam_id, uint32_t flags)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NOT_INIT; }

    if (SteamAPI_ISteamFriends_HasFriend(friends, steam_id, (int32_t)flags) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_net_steamworks_friend_level_get(uint64_t steam_id)
{
    void *friends = 0x0;
    int32_t level = 0;

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return UINT32_MAX; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return UINT32_MAX; }

    level = SteamAPI_ISteamFriends_GetFriendSteamLevel(friends, steam_id);
    if (level < 0) { return UINT32_MAX; }

    return (uint32_t)level;
}

uint32_t ldg_net_steamworks_friend_game_get(uint64_t steam_id, ldg_net_steamworks_friend_game_t *out)
{
    void *friends = 0x0;
    steam_friend_game_info_t info = LDG_STRUCT_ZERO_INIT;
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    ret = ldg_mem_secure_zero(out, sizeof(ldg_net_steamworks_friend_game_t));
    if (LDG_UNLIKELY(ret != LDG_ERR_AOK)) { return ret; }

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamFriends_GetFriendGamePlayed(friends, steam_id, &info) == 0) { return LDG_ERR_NOT_FOUND; }

    out->game_id = info.game_id;
    out->lobby_id = info.lobby_id;
    out->ip = info.ip;
    out->port = info.port;
    out->query_port = info.query_port;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_friend_name_history_get(uint64_t steam_id, uint32_t name_idx, char *out, uint64_t out_size)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;
    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(name_idx > INT32_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    return friend_str_copy(SteamAPI_ISteamFriends_GetFriendPersonaNameHistory(friends, steam_id, (int32_t)name_idx), out, out_size);
}

uint32_t ldg_net_steamworks_player_nickname_get(uint64_t steam_id, char *out, uint64_t out_size)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;
    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    return friend_str_copy(SteamAPI_ISteamFriends_GetPlayerNickname(friends, steam_id), out, out_size);
}

uint32_t ldg_net_steamworks_friends_group_cunt_get(void)
{
    void *friends = 0x0;
    int32_t cunt = 0;

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return UINT32_MAX; }

    cunt = SteamAPI_ISteamFriends_GetFriendsGroupCount(friends);
    if (cunt < 0) { return UINT32_MAX; }

    return (uint32_t)cunt;
}

uint32_t ldg_net_steamworks_friends_group_list_get(int16_t *out_group_id, uint32_t cap, uint32_t *out_cunt)
{
    void *friends = 0x0;
    int32_t total = 0;
    int32_t idx = 0;
    uint32_t filled = 0;

    if (LDG_UNLIKELY(!out_group_id || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    total = SteamAPI_ISteamFriends_GetFriendsGroupCount(friends);
    if (total < 0) { return LDG_ERR_NET_RPC_FAIL; }

    for (idx = 0; idx < total; idx++)
    {
        if (filled >= cap) { break; }

        out_group_id[filled] = SteamAPI_ISteamFriends_GetFriendsGroupIDByIndex(friends, idx);
        filled++;
    }

    *out_cunt = filled;
    if (filled < (uint32_t)total) { return LDG_ERR_FULL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_friends_group_name_get(int16_t group_id, char *out, uint64_t out_size)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;
    if (LDG_UNLIKELY(group_id == LDG_NET_STEAMWORKS_FRIENDS_GROUP_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    return friend_str_copy(SteamAPI_ISteamFriends_GetFriendsGroupName(friends, group_id), out, out_size);
}

uint32_t ldg_net_steamworks_friends_group_member_list_get(int16_t group_id, uint64_t *out_steam_id, uint32_t cap, uint32_t *out_cunt)
{
    void *friends = 0x0;
    int32_t total = 0;

    if (LDG_UNLIKELY(!out_steam_id || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(group_id == LDG_NET_STEAMWORKS_FRIENDS_GROUP_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    total = SteamAPI_ISteamFriends_GetFriendsGroupMembersCount(friends, group_id);
    if (total < 0) { return LDG_ERR_NET_RPC_FAIL; }

    if (total == 0) { return LDG_ERR_AOK; }

    if ((uint32_t)total > cap)
    {
        SteamAPI_ISteamFriends_GetFriendsGroupMembersList(friends, group_id, out_steam_id, (int32_t)cap);
        *out_cunt = cap;
        return LDG_ERR_FULL;
    }

    SteamAPI_ISteamFriends_GetFriendsGroupMembersList(friends, group_id, out_steam_id, total);
    *out_cunt = (uint32_t)total;

    return LDG_ERR_AOK;
}

static int32_t friend_avatar_handle_get(void *friends, uint64_t steam_id, ldg_net_steamworks_avatar_size_t size)
{
    if (size == LDG_NET_STEAMWORKS_AVATAR_SIZE_SMALL) { return SteamAPI_ISteamFriends_GetSmallFriendAvatar(friends, steam_id); }

    if (size == LDG_NET_STEAMWORKS_AVATAR_SIZE_LARGE) { return SteamAPI_ISteamFriends_GetLargeFriendAvatar(friends, steam_id); }

    return SteamAPI_ISteamFriends_GetMediumFriendAvatar(friends, steam_id);
}

uint32_t ldg_net_steamworks_friend_avatar_get(uint64_t steam_id, ldg_net_steamworks_avatar_size_t size, uint8_t *rgba_out, uint32_t *width_out, uint32_t *height_out)
{
    void *friends = 0x0;
    void *utils = 0x0;
    int32_t avatar_handle = 0;
    uint32_t width = 0;
    uint32_t height = 0;

    if (LDG_UNLIKELY(!rgba_out || !width_out || !height_out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *width_out = 0;
    *height_out = 0;
    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(size > LDG_NET_STEAMWORKS_AVATAR_SIZE_LARGE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    avatar_handle = friend_avatar_handle_get(friends, steam_id, size);
    if (avatar_handle <= 0) { return LDG_ERR_NOT_FOUND; }

    utils = SteamAPI_SteamUtils_v010();
    if (LDG_UNLIKELY(!utils)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamUtils_GetImageSize(utils, avatar_handle, &width, &height) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    if (width == 0 || height == 0) { return LDG_ERR_NET_RPC_FAIL; }

    *width_out = width;
    *height_out = height;
    if (SteamAPI_ISteamUtils_GetImageRGBA(utils, avatar_handle, rgba_out, (int32_t)(width * height * 4)) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_friend_presence_get(uint64_t steam_id, const char *key, char *out, uint64_t out_size)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(!key || !out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;
    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    return friend_str_copy(SteamAPI_ISteamFriends_GetFriendRichPresence(friends, steam_id, key), out, out_size);
}

uint32_t ldg_net_steamworks_friend_presence_key_cunt_get(uint64_t steam_id)
{
    void *friends = 0x0;
    int32_t cunt = 0;

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return UINT32_MAX; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return UINT32_MAX; }

    cunt = SteamAPI_ISteamFriends_GetFriendRichPresenceKeyCount(friends, steam_id);
    if (cunt < 0) { return UINT32_MAX; }

    return (uint32_t)cunt;
}

uint32_t ldg_net_steamworks_friend_presence_key_get(uint64_t steam_id, uint32_t key_idx, char *out, uint64_t out_size)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;
    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(key_idx > INT32_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    return friend_str_copy(SteamAPI_ISteamFriends_GetFriendRichPresenceKeyByIndex(friends, steam_id, (int32_t)key_idx), out, out_size);
}

uint32_t ldg_net_steamworks_friend_presence_req(uint64_t steam_id)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamFriends_RequestFriendRichPresence(friends, steam_id);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_friend_game_invite(uint64_t steam_id, const char *connect)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(!connect)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamFriends_InviteUserToGame(friends, steam_id, connect) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_coplay_friend_list_get(uint64_t *out_steam_id, uint32_t cap, uint32_t *out_cunt)
{
    void *friends = 0x0;
    uint64_t steam_id = 0;
    int32_t total = 0;
    int32_t idx = 0;
    uint32_t filled = 0;

    if (LDG_UNLIKELY(!out_steam_id || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    total = SteamAPI_ISteamFriends_GetCoplayFriendCount(friends);
    if (total < 0) { return LDG_ERR_NET_RPC_FAIL; }

    for (idx = 0; idx < total; idx++)
    {
        if (filled >= cap) { break; }

        steam_id = SteamAPI_ISteamFriends_GetCoplayFriend(friends, idx);
        if (steam_id == STEAM_ID_INVALID) { continue; }

        out_steam_id[filled] = steam_id;
        filled++;
    }

    *out_cunt = filled;
    if (filled < (uint32_t)total) { return LDG_ERR_FULL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_coplay_time_get(uint64_t steam_id)
{
    void *friends = 0x0;
    int32_t coplay_time = 0;

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return UINT32_MAX; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return UINT32_MAX; }

    coplay_time = SteamAPI_ISteamFriends_GetFriendCoplayTime(friends, steam_id);
    if (coplay_time < 0) { return UINT32_MAX; }

    return (uint32_t)coplay_time;
}

uint32_t ldg_net_steamworks_coplay_game_get(uint64_t steam_id)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return UINT32_MAX; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return UINT32_MAX; }

    return SteamAPI_ISteamFriends_GetFriendCoplayGame(friends, steam_id);
}

uint32_t ldg_net_steamworks_unread_chat_cunt_get(void)
{
    void *friends = 0x0;
    int32_t cunt = 0;

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return UINT32_MAX; }

    cunt = SteamAPI_ISteamFriends_GetNumChatsWithUnreadPriorityMessages(friends);
    if (cunt < 0) { return UINT32_MAX; }

    return (uint32_t)cunt;
}

uint32_t ldg_net_steamworks_voice_speaking_set(uint64_t steam_id, uint8_t speaking)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(speaking > LDG_TRUTH_TRUE)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamFriends_SetInGameVoiceSpeaking(friends, steam_id, speaking);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_played_with_set(uint64_t steam_id)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamFriends_SetPlayedWith(friends, steam_id);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_overlay_activate(const char *dialog)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(!dialog)) { return LDG_ERR_FUNC_ARG_NULL; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamFriends_ActivateGameOverlay(friends, dialog);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_overlay_user_activate(const char *dialog, uint64_t steam_id)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(!dialog)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamFriends_ActivateGameOverlayToUser(friends, dialog, steam_id);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_overlay_web_page_activate(const char *url, ldg_net_steamworks_web_page_mode_t mode)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(!url)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(mode > LDG_NET_STEAMWORKS_WEB_PAGE_MODE_MODAL)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamFriends_ActivateGameOverlayToWebPage(friends, url, (int32_t)mode);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_overlay_store_activate(uint32_t app_id, ldg_net_steamworks_store_flag_t flag)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(app_id == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(flag > LDG_NET_STEAMWORKS_STORE_FLAG_CART_ADD_SHOW)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamFriends_ActivateGameOverlayToStore(friends, app_id, (int32_t)flag);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_overlay_lobby_invite_activate(uint64_t lobby_id)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(lobby_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamFriends_ActivateGameOverlayInviteDialog(friends, lobby_id);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_overlay_remote_play_invite_activate(uint64_t lobby_id)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(lobby_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamFriends_ActivateGameOverlayRemotePlayTogetherInviteDialog(friends, lobby_id);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_overlay_connect_invite_activate(const char *connect)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(!connect)) { return LDG_ERR_FUNC_ARG_NULL; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamFriends_ActivateGameOverlayInviteDialogConnectString(friends, connect);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_overlay_protocol_reg(const char *protocol)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(!protocol)) { return LDG_ERR_FUNC_ARG_NULL; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamFriends_RegisterProtocolInOverlayBrowser(friends, protocol) == 0) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint8_t ldg_net_steamworks_profile_item_has(uint64_t steam_id, ldg_net_steamworks_profile_item_t item)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(item > LDG_NET_STEAMWORKS_PROFILE_ITEM_MINI_BACKDROP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NOT_INIT; }

    if (SteamAPI_ISteamFriends_BHasEquippedProfileItem(friends, steam_id, (int32_t)item) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_net_steamworks_profile_prop_str_get(uint64_t steam_id, ldg_net_steamworks_profile_item_t item, ldg_net_steamworks_profile_prop_t prop, char *out, uint64_t out_size)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;
    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(item > LDG_NET_STEAMWORKS_PROFILE_ITEM_MINI_BACKDROP)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(prop > LDG_NET_STEAMWORKS_PROFILE_PROP_MOVIE_MP4_SMALL)) { return LDG_ERR_FUNC_ARG_INVALID; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    return friend_str_copy(SteamAPI_ISteamFriends_GetProfileItemPropertyString(friends, steam_id, (int32_t)item, (int32_t)prop), out, out_size);
}

uint32_t ldg_net_steamworks_profile_prop_uint_get(uint64_t steam_id, ldg_net_steamworks_profile_item_t item, ldg_net_steamworks_profile_prop_t prop)
{
    void *friends = 0x0;

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return UINT32_MAX; }

    if (LDG_UNLIKELY(item > LDG_NET_STEAMWORKS_PROFILE_ITEM_MINI_BACKDROP)) { return UINT32_MAX; }

    if (LDG_UNLIKELY(prop > LDG_NET_STEAMWORKS_PROFILE_PROP_MOVIE_MP4_SMALL)) { return UINT32_MAX; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return UINT32_MAX; }

    return SteamAPI_ISteamFriends_GetProfileItemPropertyUint(friends, steam_id, (int32_t)item, (int32_t)prop);
}

uint32_t ldg_net_steamworks_friend_call_init(ldg_net_steamworks_friend_call_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    return ldg_mem_secure_zero(ctx, sizeof(ldg_net_steamworks_friend_call_ctx_t));
}

static ldg_net_steamworks_friend_call_state_t friend_call_state_get(int32_t steam_res, uint8_t failed)
{
    if (failed != 0) { return LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_FAIL; }

    if (steam_res != STEAM_RES_CALL_OK) { return LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_FAIL; }

    return LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_DONE;
}

static void follower_cunt_call_handle(ldg_net_steamworks_friend_call_ctx_t *ctx, int32_t pipe, uint64_t call)
{
    steam_follower_cunt_res_t res = LDG_STRUCT_ZERO_INIT;
    uint8_t failed = 0;

    if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, call, &res, (int32_t)sizeof(res), STEAM_CB_FOLLOWER_CUNT, &failed) == 0) { return; }

    ctx->follower_call = STEAM_CALL_INVALID;
    ctx->follower_state = friend_call_state_get(res.res, failed);
    if (ctx->follower_state != LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_DONE) { return; }

    ctx->follower_steam_id = res.steam_id;
    if (res.cunt > 0) { ctx->follower_cunt = (uint32_t)res.cunt; }
}

static void is_following_call_handle(ldg_net_steamworks_friend_call_ctx_t *ctx, int32_t pipe, uint64_t call)
{
    steam_is_following_res_t res = LDG_STRUCT_ZERO_INIT;
    uint8_t failed = 0;

    if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, call, &res, (int32_t)sizeof(res), STEAM_CB_IS_FOLLOWING, &failed) == 0) { return; }

    ctx->following_call = STEAM_CALL_INVALID;
    ctx->following_state = friend_call_state_get(res.res, failed);
    if (ctx->following_state != LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_DONE) { return; }

    ctx->following_steam_id = res.steam_id;
    ctx->is_following = (res.is_following != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE;
}

static void following_list_call_handle(ldg_net_steamworks_friend_call_ctx_t *ctx, int32_t pipe, uint64_t call)
{
    steam_following_list_res_t res = LDG_STRUCT_ZERO_INIT;
    uint8_t failed = 0;
    uint32_t filled = 0;
    uint32_t idx = 0;

    if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, call, &res, (int32_t)sizeof(res), STEAM_CB_FOLLOWING_LIST, &failed) == 0) { return; }

    ctx->following_list_call = STEAM_CALL_INVALID;
    ctx->following_list_state = friend_call_state_get(res.res, failed);
    if (ctx->following_list_state != LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_DONE) { return; }

    if (res.filled_cunt > 0) { filled = (uint32_t)res.filled_cunt; }

    if (filled > STEAM_FOLLOWING_PAGE_MAX) { filled = STEAM_FOLLOWING_PAGE_MAX; }

    for (idx = 0; idx < filled; idx++) { ctx->following_list[idx] = res.steam_id[idx]; }

    ctx->following_list_cunt = filled;
    if (res.total_cunt > 0) { ctx->following_list_total = (uint32_t)res.total_cunt; }
}

static void profile_items_call_handle(ldg_net_steamworks_friend_call_ctx_t *ctx, int32_t pipe, uint64_t call)
{
    steam_profile_items_res_t res = LDG_STRUCT_ZERO_INIT;
    uint8_t failed = 0;
    uint32_t idx = 0;

    if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, call, &res, (int32_t)sizeof(res), STEAM_CB_EQUIPPED_PROFILE_ITEMS, &failed) == 0) { return; }

    ctx->profile_call = STEAM_CALL_INVALID;
    ctx->profile_state = friend_call_state_get(res.res, failed);
    if (ctx->profile_state != LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_DONE) { return; }

    ctx->profile_steam_id = res.steam_id;
    for (idx = 0; idx < LDG_NET_STEAMWORKS_PROFILE_ITEM_CUNT; idx++) { ctx->profile_item[idx] = (res.item[idx] != 0) ? LDG_TRUTH_TRUE : LDG_TRUTH_FALSE; }
}

void ldg_net_steamworks_friend_call_handle(ldg_net_steamworks_friend_call_ctx_t *ctx, int32_t pipe, uint64_t call)
{
    if (!ctx || pipe == 0 || call == STEAM_CALL_INVALID) { return; }

    if (call == ctx->follower_call) { follower_cunt_call_handle(ctx, pipe, call); }
    else if (call == ctx->following_call) { is_following_call_handle(ctx, pipe, call); }
    else if (call == ctx->following_list_call) { following_list_call_handle(ctx, pipe, call); }
    else if (call == ctx->profile_call) { profile_items_call_handle(ctx, pipe, call); }
}

static uint32_t friend_call_res_ck(ldg_net_steamworks_friend_call_state_t state)
{
    if (state == LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_IDLE) { return LDG_ERR_EMPTY; }

    if (state == LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_PENDING) { return LDG_ERR_AGAIN; }

    if (state == LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_FAIL) { return LDG_ERR_NET_RPC_FAIL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_follower_cunt_req(ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t steam_id)
{
    void *friends = 0x0;
    uint64_t call = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->follower_state == LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_PENDING)) { return LDG_ERR_BUSY; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    call = SteamAPI_ISteamFriends_GetFollowerCount(friends, steam_id);
    if (LDG_UNLIKELY(call == STEAM_CALL_INVALID)) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->follower_call = call;
    ctx->follower_steam_id = STEAM_ID_INVALID;
    ctx->follower_cunt = 0;
    ctx->follower_state = LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_PENDING;

    return LDG_ERR_AOK;
}

ldg_net_steamworks_friend_call_state_t ldg_net_steamworks_follower_cunt_state_get(const ldg_net_steamworks_friend_call_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_IDLE; }

    return ctx->follower_state;
}

uint32_t ldg_net_steamworks_follower_cunt_res_get(const ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t *out_steam_id, uint32_t *out_cunt)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !out_steam_id || !out_cunt)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_steam_id = STEAM_ID_INVALID;
    *out_cunt = 0;

    ret = friend_call_res_ck(ctx->follower_state);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out_steam_id = ctx->follower_steam_id;
    *out_cunt = ctx->follower_cunt;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_following_req(ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t steam_id)
{
    void *friends = 0x0;
    uint64_t call = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->following_state == LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_PENDING)) { return LDG_ERR_BUSY; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    call = SteamAPI_ISteamFriends_IsFollowing(friends, steam_id);
    if (LDG_UNLIKELY(call == STEAM_CALL_INVALID)) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->following_call = call;
    ctx->following_steam_id = STEAM_ID_INVALID;
    ctx->is_following = LDG_TRUTH_FALSE;
    ctx->following_state = LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_PENDING;

    return LDG_ERR_AOK;
}

ldg_net_steamworks_friend_call_state_t ldg_net_steamworks_following_state_get(const ldg_net_steamworks_friend_call_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_IDLE; }

    return ctx->following_state;
}

uint32_t ldg_net_steamworks_following_res_get(const ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t *out_steam_id, uint8_t *out_following)
{
    uint32_t ret = 0;

    if (LDG_UNLIKELY(!ctx || !out_steam_id || !out_following)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_steam_id = STEAM_ID_INVALID;
    *out_following = LDG_TRUTH_FALSE;

    ret = friend_call_res_ck(ctx->following_state);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out_steam_id = ctx->following_steam_id;
    *out_following = ctx->is_following;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_following_list_req(ldg_net_steamworks_friend_call_ctx_t *ctx, uint32_t start_idx)
{
    void *friends = 0x0;
    uint64_t call = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(ctx->following_list_state == LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_PENDING)) { return LDG_ERR_BUSY; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    call = SteamAPI_ISteamFriends_EnumerateFollowingList(friends, start_idx);
    if (LDG_UNLIKELY(call == STEAM_CALL_INVALID)) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->following_list_call = call;
    ctx->following_list_cunt = 0;
    ctx->following_list_total = 0;
    ctx->following_list_state = LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_PENDING;

    return LDG_ERR_AOK;
}

ldg_net_steamworks_friend_call_state_t ldg_net_steamworks_following_list_state_get(const ldg_net_steamworks_friend_call_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_IDLE; }

    return ctx->following_list_state;
}

uint32_t ldg_net_steamworks_following_list_res_get(const ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t *out_steam_id, uint32_t cap, uint32_t *out_cunt, uint32_t *out_total)
{
    uint32_t ret = 0;
    uint32_t filled = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!ctx || !out_steam_id || !out_cunt || !out_total)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_cunt = 0;
    *out_total = 0;
    if (LDG_UNLIKELY(cap == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    ret = friend_call_res_ck(ctx->following_list_state);
    if (ret != LDG_ERR_AOK) { return ret; }

    filled = ctx->following_list_cunt;
    *out_total = ctx->following_list_total;
    if (filled > cap) { filled = cap; }

    for (idx = 0; idx < filled; idx++) { out_steam_id[idx] = ctx->following_list[idx]; }

    *out_cunt = filled;
    if (filled < ctx->following_list_cunt) { return LDG_ERR_FULL; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_profile_items_req(ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t steam_id)
{
    void *friends = 0x0;
    uint64_t call = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!ctx)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(steam_id == STEAM_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(ctx->profile_state == LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_PENDING)) { return LDG_ERR_BUSY; }

    friends = SteamAPI_SteamFriends_v018();
    if (LDG_UNLIKELY(!friends)) { return LDG_ERR_NET_RPC_FAIL; }

    call = SteamAPI_ISteamFriends_RequestEquippedProfileItems(friends, steam_id);
    if (LDG_UNLIKELY(call == STEAM_CALL_INVALID)) { return LDG_ERR_NET_RPC_FAIL; }

    ctx->profile_call = call;
    ctx->profile_steam_id = STEAM_ID_INVALID;
    for (idx = 0; idx < LDG_NET_STEAMWORKS_PROFILE_ITEM_CUNT; idx++) { ctx->profile_item[idx] = LDG_TRUTH_FALSE; }

    ctx->profile_state = LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_PENDING;

    return LDG_ERR_AOK;
}

ldg_net_steamworks_friend_call_state_t ldg_net_steamworks_profile_items_state_get(const ldg_net_steamworks_friend_call_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return LDG_NET_STEAMWORKS_FRIEND_CALL_STATE_IDLE; }

    return ctx->profile_state;
}

uint32_t ldg_net_steamworks_profile_items_res_get(const ldg_net_steamworks_friend_call_ctx_t *ctx, uint64_t *out_steam_id, uint8_t *out_item, uint32_t cap)
{
    uint32_t ret = 0;
    uint32_t idx = 0;

    if (LDG_UNLIKELY(!ctx || !out_steam_id || !out_item)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_steam_id = STEAM_ID_INVALID;
    if (LDG_UNLIKELY(cap < LDG_NET_STEAMWORKS_PROFILE_ITEM_CUNT)) { return LDG_ERR_BOUNDS; }

    ret = friend_call_res_ck(ctx->profile_state);
    if (ret != LDG_ERR_AOK) { return ret; }

    *out_steam_id = ctx->profile_steam_id;
    for (idx = 0; idx < LDG_NET_STEAMWORKS_PROFILE_ITEM_CUNT; idx++) { out_item[idx] = ctx->profile_item[idx]; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_app_id_get(void)
{
    void *utils = 0x0;
    uint32_t app_id = 0;

    utils = SteamAPI_SteamUtils_v010();
    if (LDG_UNLIKELY(!utils)) { return UINT32_MAX; }

    app_id = SteamAPI_ISteamUtils_GetAppID(utils);
    if (LDG_UNLIKELY(app_id == 0)) { return UINT32_MAX; }

    return app_id;
}

uint32_t ldg_net_steamworks_dlc_install(uint32_t app_id)
{
    void *apps = 0x0;

    if (LDG_UNLIKELY(app_id == STEAM_APP_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    apps = SteamAPI_SteamApps_v009();
    if (LDG_UNLIKELY(!apps))
    {
        LDG_ERRLOG_INFO("net: failed to get apps interface");
        return LDG_ERR_NET_RPC_FAIL;
    }

    SteamAPI_ISteamApps_InstallDLC(apps, app_id);

    return LDG_ERR_AOK;
}

uint8_t ldg_net_steamworks_dlc_installed_is(uint32_t app_id)
{
    void *apps = 0x0;

    if (LDG_UNLIKELY(app_id == STEAM_APP_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    apps = SteamAPI_SteamApps_v009();
    if (LDG_UNLIKELY(!apps)) { return LDG_ERR_NOT_INIT; }

    if (SteamAPI_ISteamApps_BIsDlcInstalled(apps, app_id) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint32_t ldg_net_steamworks_dlc_progress_get(uint32_t app_id, uint64_t *out_downloaded, uint64_t *out_total)
{
    void *apps = 0x0;

    if (LDG_UNLIKELY(!out_downloaded || !out_total)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out_downloaded = 0;
    *out_total = 0;

    if (LDG_UNLIKELY(app_id == STEAM_APP_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    apps = SteamAPI_SteamApps_v009();
    if (LDG_UNLIKELY(!apps))
    {
        LDG_ERRLOG_INFO("net: failed to get apps interface");
        return LDG_ERR_NET_RPC_FAIL;
    }

    if (SteamAPI_ISteamApps_GetDlcDownloadProgress(apps, app_id, out_downloaded, out_total) == 0) { return LDG_ERR_NOT_FOUND; }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_app_dir_get(uint32_t app_id, char *out, uint64_t out_size)
{
    void *apps = 0x0;
    uint32_t len = 0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(out_size == 0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    out[0] = LDG_STR_TERM;

    if (LDG_UNLIKELY(app_id == STEAM_APP_ID_INVALID)) { return LDG_ERR_FUNC_ARG_INVALID; }

    if (LDG_UNLIKELY(out_size > UINT32_MAX)) { return LDG_ERR_FUNC_ARG_INVALID; }

    apps = SteamAPI_SteamApps_v009();
    if (LDG_UNLIKELY(!apps))
    {
        LDG_ERRLOG_INFO("net: failed to get apps interface");
        return LDG_ERR_NET_RPC_FAIL;
    }

    len = SteamAPI_ISteamApps_GetAppInstallDir(apps, app_id, out, (uint32_t)out_size);

    out[out_size - LDG_STR_TERM_SIZE] = LDG_STR_TERM;

    if (len == 0)
    {
        out[0] = LDG_STR_TERM;
        return LDG_ERR_NOT_FOUND;
    }

    if (LDG_UNLIKELY(len >= out_size)) { return LDG_ERR_MEM_STR_TRUNC; }

    return LDG_ERR_AOK;
}

uint8_t ldg_net_steamworks_music_en_is(void)
{
    void *music = 0x0;

    music = SteamAPI_SteamMusic_v001();
    if (LDG_UNLIKELY(!music)) { return LDG_ERR_NOT_INIT; }

    if (SteamAPI_ISteamMusic_BIsEnabled(music) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

uint8_t ldg_net_steamworks_music_playing_is(void)
{
    void *music = 0x0;

    music = SteamAPI_SteamMusic_v001();
    if (LDG_UNLIKELY(!music)) { return LDG_ERR_NOT_INIT; }

    if (SteamAPI_ISteamMusic_BIsPlaying(music) != 0) { return LDG_TRUTH_TRUE; }

    return LDG_TRUTH_FALSE;
}

ldg_net_steamworks_music_status_t ldg_net_steamworks_music_status_get(void)
{
    void *music = 0x0;
    int32_t status = 0;

    music = SteamAPI_SteamMusic_v001();
    if (LDG_UNLIKELY(!music)) { return LDG_NET_STEAMWORKS_MUSIC_STATUS_UNDEFINED; }

    status = SteamAPI_ISteamMusic_GetPlaybackStatus(music);
    if (status < 0 || status > LDG_NET_STEAMWORKS_MUSIC_STATUS_IDLE) { return LDG_NET_STEAMWORKS_MUSIC_STATUS_UNDEFINED; }

    return (ldg_net_steamworks_music_status_t)status;
}

uint32_t ldg_net_steamworks_music_play(void)
{
    void *music = 0x0;

    music = SteamAPI_SteamMusic_v001();
    if (LDG_UNLIKELY(!music)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamMusic_BIsEnabled(music) == 0) { return LDG_ERR_UNSUPPORTED; }

    SteamAPI_ISteamMusic_Play(music);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_music_pause(void)
{
    void *music = 0x0;

    music = SteamAPI_SteamMusic_v001();
    if (LDG_UNLIKELY(!music)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamMusic_BIsEnabled(music) == 0) { return LDG_ERR_UNSUPPORTED; }

    SteamAPI_ISteamMusic_Pause(music);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_music_next_play(void)
{
    void *music = 0x0;

    music = SteamAPI_SteamMusic_v001();
    if (LDG_UNLIKELY(!music)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamMusic_BIsEnabled(music) == 0) { return LDG_ERR_UNSUPPORTED; }

    SteamAPI_ISteamMusic_PlayNext(music);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_music_prev_play(void)
{
    void *music = 0x0;

    music = SteamAPI_SteamMusic_v001();
    if (LDG_UNLIKELY(!music)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamMusic_BIsEnabled(music) == 0) { return LDG_ERR_UNSUPPORTED; }

    SteamAPI_ISteamMusic_PlayPrevious(music);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_music_vol_set(double vol)
{
    void *music = 0x0;

    if (LDG_UNLIKELY(!ldg_math_finite_is(vol) || vol < 0.0 || vol > 1.0)) { return LDG_ERR_FUNC_ARG_INVALID; }

    music = SteamAPI_SteamMusic_v001();
    if (LDG_UNLIKELY(!music)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamMusic_BIsEnabled(music) == 0) { return LDG_ERR_UNSUPPORTED; }

    SteamAPI_ISteamMusic_SetVolume(music, (float)vol);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_music_vol_get(double *out)
{
    void *music = 0x0;

    if (LDG_UNLIKELY(!out)) { return LDG_ERR_FUNC_ARG_NULL; }

    *out = 0.0;

    music = SteamAPI_SteamMusic_v001();
    if (LDG_UNLIKELY(!music)) { return LDG_ERR_NET_RPC_FAIL; }

    if (SteamAPI_ISteamMusic_BIsEnabled(music) == 0) { return LDG_ERR_UNSUPPORTED; }

    *out = (double)SteamAPI_ISteamMusic_GetVolume(music);

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_timeline_mode_set(ldg_net_steamworks_timeline_mode_t mode)
{
    void *timeline = 0x0;

    timeline = SteamAPI_SteamTimeline_v004();
    if (LDG_UNLIKELY(!timeline)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamTimeline_SetTimelineGameMode(timeline, (int32_t)mode);
    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_timeline_tooltip_set(const char *desc, double offset)
{
    void *timeline = 0x0;

    if (LDG_UNLIKELY(!desc)) { return LDG_ERR_FUNC_ARG_NULL; }

    timeline = SteamAPI_SteamTimeline_v004();
    if (LDG_UNLIKELY(!timeline)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamTimeline_SetTimelineTooltip(timeline, desc, (float)offset);
    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_timeline_tooltip_clear(double offset)
{
    void *timeline = 0x0;

    timeline = SteamAPI_SteamTimeline_v004();
    if (LDG_UNLIKELY(!timeline)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamTimeline_ClearTimelineTooltip(timeline, (float)offset);
    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_timeline_event_add(const char *title, const char *desc, const char *icon, uint32_t priority)
{
    void *timeline = 0x0;

    if (LDG_UNLIKELY(!title)) { return LDG_ERR_FUNC_ARG_NULL; }

    timeline = SteamAPI_SteamTimeline_v004();
    if (LDG_UNLIKELY(!timeline)) { return LDG_ERR_NET_RPC_FAIL; }

    if (LDG_UNLIKELY(SteamAPI_ISteamTimeline_AddInstantaneousTimelineEvent(timeline, title, desc, icon, priority, 0.0, 2) == 0)) { LDG_ERRLOG_WARN("net: add timeline event failed"); }

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_timeline_phase_start(void)
{
    void *timeline = 0x0;

    timeline = SteamAPI_SteamTimeline_v004();
    if (LDG_UNLIKELY(!timeline)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamTimeline_StartGamePhase(timeline);
    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_timeline_phase_end(void)
{
    void *timeline = 0x0;

    timeline = SteamAPI_SteamTimeline_v004();
    if (LDG_UNLIKELY(!timeline)) { return LDG_ERR_NET_RPC_FAIL; }

    SteamAPI_ISteamTimeline_EndGamePhase(timeline);
    return LDG_ERR_AOK;
}

uint32_t ldg_net_player_cunt_get(const ldg_net_ctx_t *ctx)
{
    if (LDG_UNLIKELY(!ctx)) { return 0; }

    return ctx->client_cunt;
}

uint64_t ldg_net_player_steam_id_get(const ldg_net_ctx_t *ctx, uint32_t player_id)
{
    if (LDG_UNLIKELY(!ctx || !ctx->client || player_id >= LDG_NET_CLIENT_MAX)) { return 0; }

    return ctx->client[player_id].steam_id;
}
