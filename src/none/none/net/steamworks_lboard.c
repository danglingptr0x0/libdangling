#include <stdint.h>
#include <string.h>
#include <dangling/core/err.h>
#include <dangling/core/macros.h>
#include <dangling/str/str.h>
#include <dangling/mem/secure.h>
#include <dangling/net/steamworks_lboard.h>

#define STEAM_CB_FIND_LBOARD 1104
#define STEAM_CB_DOWNLOAD_ENTRY 1105
#define STEAM_CB_UPLOAD_SCORE 1106

typedef struct lboard_entry_steam
{
    uint64_t steam_id;
    int32_t global_rank;
    int32_t score;
    int32_t detail_cunt;
#ifdef _WIN32
    uint8_t pudding0[4];
#endif
    uint64_t ugc_handle;
} LDG_PACKED lboard_entry_steam_t;

typedef struct lboard_find_res
{
    uint64_t handle;
    uint8_t found;
#ifdef _WIN32
    uint8_t pudding0[7];
#else
    uint8_t pudding0[3];
#endif
} LDG_PACKED lboard_find_res_t;

typedef struct lboard_download_res
{
    uint64_t handle;
    uint64_t entry_handle;
    int32_t cunt;
#ifdef _WIN32
    uint8_t pudding0[4];
#endif
} LDG_PACKED lboard_download_res_t;

typedef struct lboard_upload_res
{
    uint8_t success;
#ifdef _WIN32
    uint8_t pudding0[7];
#else
    uint8_t pudding0[3];
#endif
    uint64_t handle;
    int32_t score;
    uint8_t score_changed;
    uint8_t pudding1[3];
    int32_t new_rank;
    int32_t prev_rank;
} LDG_PACKED lboard_upload_res_t;

#define STEAMWORKS_LAYOUT_CK(name, cond) typedef char steam_layout_ck_ ## name[(cond) ? 1 : -1]

#ifdef _WIN32
STEAMWORKS_LAYOUT_CK(entry_steam_size, sizeof(lboard_entry_steam_t) == 32);
STEAMWORKS_LAYOUT_CK(find_res_size, sizeof(lboard_find_res_t) == 16);
STEAMWORKS_LAYOUT_CK(download_res_size, sizeof(lboard_download_res_t) == 24);
STEAMWORKS_LAYOUT_CK(upload_res_size, sizeof(lboard_upload_res_t) == 32);
#else
STEAMWORKS_LAYOUT_CK(entry_steam_size, sizeof(lboard_entry_steam_t) == 28);
STEAMWORKS_LAYOUT_CK(find_res_size, sizeof(lboard_find_res_t) == 12);
STEAMWORKS_LAYOUT_CK(download_res_size, sizeof(lboard_download_res_t) == 20);
STEAMWORKS_LAYOUT_CK(upload_res_size, sizeof(lboard_upload_res_t) == 28);
#endif

extern void* SteamAPI_SteamUserStats_v013(void);
extern uint64_t SteamAPI_ISteamUserStats_FindLeaderboard(void *self, const char *name);
extern int32_t SteamAPI_ISteamUserStats_GetLeaderboardEntryCount(void *self, uint64_t handle);
extern uint64_t SteamAPI_ISteamUserStats_DownloadLeaderboardEntries(void *self, uint64_t handle, int32_t type, int32_t start, int32_t end);
extern uint64_t SteamAPI_ISteamUserStats_UploadLeaderboardScore(void *self, uint64_t handle, int32_t method, int32_t score, const int32_t *detail, int32_t cunt);
extern uint8_t SteamAPI_ISteamUserStats_GetDownloadedLeaderboardEntry(void *self, uint64_t entry_handle, int32_t idx, void *entry, int32_t *detail, int32_t max);
extern uint8_t SteamAPI_ManualDispatch_GetAPICallResult(int32_t pipe, uint64_t call, void *cb, int32_t cb_size, int32_t cb_expected, uint8_t *failed);

uint32_t ldg_net_steamworks_lboard_init(ldg_net_steamworks_lboard_ctx_t *lboard)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!lboard)) { return LDG_ERR_FUNC_ARG_NULL; }

    err = ldg_mem_secure_zero(lboard, sizeof(ldg_net_steamworks_lboard_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { return err; }

    lboard->state = LDG_NET_STEAMWORKS_LBOARD_STATE_IDLE;

    return LDG_ERR_AOK;
}

void ldg_net_steamworks_lboard_shutdown(ldg_net_steamworks_lboard_ctx_t *lboard)
{
    uint32_t err = 0;

    if (LDG_UNLIKELY(!lboard)) { return; }

    err = ldg_mem_secure_zero(lboard, sizeof(ldg_net_steamworks_lboard_ctx_t));
    if (LDG_UNLIKELY(err != LDG_ERR_AOK)) { LDG_ERRLOG_ERR("net: lboard ctx zero failed"); }
}

uint32_t ldg_net_steamworks_lboard_find(ldg_net_steamworks_lboard_ctx_t *lboard, const char *name)
{
    void *stats = 0x0;
    uint64_t api_call = 0;
    uint32_t copy_err = 0;

    if (LDG_UNLIKELY(!lboard)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (LDG_UNLIKELY(!name || name[0] == LDG_STR_TERM)) { return LDG_ERR_FUNC_ARG_NULL; }

    stats = SteamAPI_SteamUserStats_v013();
    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_NET_RPC_FAIL; }

    if (lboard->api_call != 0) { return LDG_ERR_BUSY; }

    api_call = SteamAPI_ISteamUserStats_FindLeaderboard(stats, name);
    if (api_call == 0) { return LDG_ERR_NET_RPC_FAIL; }

    copy_err = ldg_str_rbrcpy(lboard->name, name, sizeof(lboard->name));
    if (LDG_UNLIKELY(copy_err != LDG_ERR_AOK && copy_err != LDG_ERR_STR_TRUNC)) { LDG_ERRLOG_WARN("net: lboard name copy failed"); }

    lboard->state = LDG_NET_STEAMWORKS_LBOARD_STATE_FINDING;
    lboard->handle = 0;
    lboard->api_call = api_call;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_lboard_download(ldg_net_steamworks_lboard_ctx_t *lboard, ldg_net_steamworks_lboard_req_t type, int32_t start, int32_t end)
{
    void *stats = 0x0;
    uint64_t api_call = 0;

    if (LDG_UNLIKELY(!lboard)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (lboard->handle == 0) { return LDG_ERR_NOT_INIT; }

    if (lboard->api_call != 0) { return LDG_ERR_BUSY; }

    stats = SteamAPI_SteamUserStats_v013();
    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_NET_RPC_FAIL; }

    api_call = SteamAPI_ISteamUserStats_DownloadLeaderboardEntries(stats, lboard->handle, (int32_t)type, start, end);
    if (api_call == 0) { return LDG_ERR_NET_RPC_FAIL; }

    lboard->state = LDG_NET_STEAMWORKS_LBOARD_STATE_DOWNLOADING;
    lboard->entry_cunt = 0;
    lboard->api_call = api_call;

    return LDG_ERR_AOK;
}

uint32_t ldg_net_steamworks_lboard_upload(ldg_net_steamworks_lboard_ctx_t *lboard, int32_t score, ldg_net_steamworks_lboard_upload_t method, const int32_t *detail, uint32_t detail_cunt)
{
    void *stats = 0x0;
    uint64_t api_call = 0;

    if (LDG_UNLIKELY(!lboard)) { return LDG_ERR_FUNC_ARG_NULL; }

    if (lboard->handle == 0) { return LDG_ERR_NOT_INIT; }

    if (lboard->api_call != 0) { return LDG_ERR_BUSY; }

    if (detail_cunt > LDG_NET_STEAMWORKS_LBOARD_DETAIL_MAX) { detail_cunt = LDG_NET_STEAMWORKS_LBOARD_DETAIL_MAX; }

    stats = SteamAPI_SteamUserStats_v013();
    if (LDG_UNLIKELY(!stats)) { return LDG_ERR_NET_RPC_FAIL; }

    api_call = SteamAPI_ISteamUserStats_UploadLeaderboardScore(stats, lboard->handle, (int32_t)method, score, detail, (int32_t)detail_cunt);
    if (api_call == 0) { return LDG_ERR_NET_RPC_FAIL; }

    lboard->state = LDG_NET_STEAMWORKS_LBOARD_STATE_UPLOADING;
    lboard->api_call = api_call;

    return LDG_ERR_AOK;
}

ldg_net_steamworks_lboard_state_t ldg_net_steamworks_lboard_state_get(const ldg_net_steamworks_lboard_ctx_t *lboard)
{
    if (LDG_UNLIKELY(!lboard)) { return LDG_NET_STEAMWORKS_LBOARD_STATE_IDLE; }

    return lboard->state;
}

uint32_t ldg_net_steamworks_lboard_entry_cunt_get(const ldg_net_steamworks_lboard_ctx_t *lboard)
{
    if (LDG_UNLIKELY(!lboard)) { return 0; }

    if (lboard->entry_cunt < 0) { return 0; }

    return (uint32_t)lboard->entry_cunt;
}

const ldg_net_steamworks_lboard_entry_t* ldg_net_steamworks_lboard_entry_get(const ldg_net_steamworks_lboard_ctx_t *lboard, uint32_t idx)
{
    if (LDG_UNLIKELY(!lboard)) { return 0x0; }

    if (lboard->entry_cunt < 0 || idx >= (uint32_t)lboard->entry_cunt) { return 0x0; }

    return &lboard->entry[idx];
}

void ldg_net_steamworks_lboard_call_handle(ldg_net_steamworks_lboard_ctx_t *lboard, int32_t pipe, uint64_t call)
{
    void *stats = 0x0;
    uint8_t failed = 0;
    int32_t iter = 0;

    if (LDG_UNLIKELY(!lboard)) { return; }

    if (lboard->api_call == 0 || call != lboard->api_call) { return; }

    stats = SteamAPI_SteamUserStats_v013();

    if (lboard->state == LDG_NET_STEAMWORKS_LBOARD_STATE_FINDING)
    {
        lboard_find_res_t res = LDG_STRUCT_ZERO_INIT;

        if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, lboard->api_call, &res, (int32_t)sizeof(res), STEAM_CB_FIND_LBOARD, &failed) != 0) { if (failed == 0 && res.found != 0 && res.handle != 0)
            {
                lboard->handle = res.handle;
                lboard->total_cunt = stats ? SteamAPI_ISteamUserStats_GetLeaderboardEntryCount(stats, lboard->handle) : 0;
                lboard->state = LDG_NET_STEAMWORKS_LBOARD_STATE_READY;
            }
            else { lboard->state = LDG_NET_STEAMWORKS_LBOARD_STATE_ERR; } }
        else { lboard->state = LDG_NET_STEAMWORKS_LBOARD_STATE_ERR; }
    }
    else if (lboard->state == LDG_NET_STEAMWORKS_LBOARD_STATE_DOWNLOADING)
    {
        lboard_download_res_t res = LDG_STRUCT_ZERO_INIT;

        if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, lboard->api_call, &res, (int32_t)sizeof(res), STEAM_CB_DOWNLOAD_ENTRY, &failed) != 0)
        {
            if (failed == 0 && res.entry_handle != 0)
            {
                lboard->entry_handle = res.entry_handle;
                lboard->entry_cunt = (res.cunt > LDG_NET_STEAMWORKS_LBOARD_ENTRY_MAX) ? LDG_NET_STEAMWORKS_LBOARD_ENTRY_MAX : res.cunt;

                for (iter = 0; iter < lboard->entry_cunt && stats; iter = iter + 1)
                {
                    lboard_entry_steam_t entry = LDG_STRUCT_ZERO_INIT;
                    int32_t detail[LDG_NET_STEAMWORKS_LBOARD_DETAIL_MAX] = LDG_ARR_ZERO_INIT;
                    uint32_t copy_err = 0;

                    if (SteamAPI_ISteamUserStats_GetDownloadedLeaderboardEntry(stats, lboard->entry_handle, iter, &entry, detail, LDG_NET_STEAMWORKS_LBOARD_DETAIL_MAX) != 0)
                    {
                        lboard->entry[iter].steam_id = entry.steam_id;
                        lboard->entry[iter].global_rank = entry.global_rank;
                        lboard->entry[iter].score = entry.score;
                        lboard->entry[iter].detail_cunt = (entry.detail_cunt > LDG_NET_STEAMWORKS_LBOARD_DETAIL_MAX) ? LDG_NET_STEAMWORKS_LBOARD_DETAIL_MAX : entry.detail_cunt;
                        copy_err = ldg_mem_secure_cpy(lboard->entry[iter].detail, detail, sizeof(int32_t) * (uint64_t)lboard->entry[iter].detail_cunt);
                        if (LDG_UNLIKELY(copy_err != LDG_ERR_AOK)) { LDG_ERRLOG_WARN("net: lboard detail copy failed"); }
                    }
                }

                lboard->state = LDG_NET_STEAMWORKS_LBOARD_STATE_READY;
            }
            else { lboard->state = LDG_NET_STEAMWORKS_LBOARD_STATE_ERR; }
        }
        else { lboard->state = LDG_NET_STEAMWORKS_LBOARD_STATE_ERR; }
    }
    else if (lboard->state == LDG_NET_STEAMWORKS_LBOARD_STATE_UPLOADING)
    {
        lboard_upload_res_t res = LDG_STRUCT_ZERO_INIT;

        if (SteamAPI_ManualDispatch_GetAPICallResult(pipe, lboard->api_call, &res, (int32_t)sizeof(res), STEAM_CB_UPLOAD_SCORE, &failed) != 0)
        {
            if (failed == 0 && res.success != 0)
            {
                lboard->upload_success = 1;
                lboard->score_changed = res.score_changed;
                lboard->new_rank = res.new_rank;
                lboard->prev_rank = res.prev_rank;
                lboard->state = LDG_NET_STEAMWORKS_LBOARD_STATE_READY;
            }
            else
            {
                lboard->upload_success = 0;
                lboard->state = LDG_NET_STEAMWORKS_LBOARD_STATE_ERR;
            }
        }
        else { lboard->state = LDG_NET_STEAMWORKS_LBOARD_STATE_ERR; }
    }

    lboard->api_call = 0;
}
